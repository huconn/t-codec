/*******************************************************************************

*   Copyright (c) Telechips Inc.
*   TCC Version 1.0

This source code contains confidential information of Telechips.

Any unauthorized use without a written permission of Telechips including not
limited to re-distribution in source or binary form is strictly prohibited.

This source code is provided "AS IS" and nothing contained in this source code
shall constitute any express or implied warranty of any kind, including without
limitation, any warranty of merchantability, fitness for a particular purpose
or non-infringement of any patent, copyright or other third party intellectual
property right.
No warranty is made, express or implied, regarding the information's accuracy,
completeness, or performance.

In no event shall Telechips be liable for any claim, damages or other
liability arising from, out of or in connection with this source code or
the use in the source code.

This source code is provided subject to the terms of a Mutual Non-Disclosure
Agreement between Telechips and Company.
*
*******************************************************************************/


#include "vpu_v3/tcc_vpu_v3_ioctl.h"
#include "vpu_v3/tcc_vpu_v3_encoder.h"
#include "venc_v3.h"

#include <sys/mman.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/poll.h>

//features
#define MULTI_SLICES_AVC

//logs
#include <time.h>
#define TC_RESET		"\033""[39m"
#define DBG_PRINT(fmt, args...) \
		do { \
			struct timespec _now;    \
			struct tm _tm;  \
			clock_gettime(CLOCK_REALTIME, &_now);   \
			localtime_r(&_now.tv_sec, &_tm);     \
			printf("[%04d-%02d-%02d %02d:%02d:%02d.%03ld][%-20s %5d]" fmt "\n" TC_RESET, \
				_tm.tm_year + 1900, _tm.tm_mon+1, _tm.tm_mday, _tm.tm_hour, _tm.tm_min, _tm.tm_sec, _now.tv_nsec/1000000, \
				__FUNCTION__, __LINE__, ## args); \
		}while(0)

#define ALIGNED_BUFF(buf, mul) (((unsigned long)buf + (mul - 1)) & ~(mul - 1))
#define ALIGN_LEN (4 * 1024)
#define STABILITY_GAP (512)

#define DLOGV(...)	{ if(venc_log_level >= VENC_LOG_VERBOSE) { DBG_PRINT(__VA_ARGS__); } }
#define DLOGD(...)  { if(venc_log_level >= VENC_LOG_DEBUG) { DBG_PRINT(__VA_ARGS__); } }
#define DLOGI(...)  { if(venc_log_level >= VENC_LOG_INFO) { DBG_PRINT(__VA_ARGS__); } }
#define DLOGW(...)  { if(venc_log_level >= VENC_LOG_WARN) { DBG_PRINT(__VA_ARGS__); } }
#define DLOGE(...)  { if(venc_log_level >= VENC_LOG_ERROR) { DBG_PRINT(__VA_ARGS__); } }

#define VPU_ENCODER_DEVICE "/dev/vpu_venc"

#ifdef MULTI_SLICES_AVC
#define AVC_AUD_SIZE	8
const unsigned char avcAudData[AVC_AUD_SIZE] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x50, 0x00, 0x00};
//#define INSERT_AUD_HEVC
#ifdef INSERT_AUD_HEVC
#define HEVC_AUD_SIZE	7
const unsigned char hevcAudData[HEVC_AUD_SIZE] = {0x00, 0x00, 0x00, 0x01, 0x46, 0x01, 0x10};
#endif
#endif

static pthread_mutex_t instance_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned int total_opened_encoder = 0;
static int venc_log_level = VENC_LOG_ERROR;

const char* vpu_codec_string[] =
{
    "NONE",
    "AVC",
    "VC1",
    "MPEG2",
    "MPEG4",
    "H263",
    "DIVX",
    "AVS",
    "MJPG",
    "VP8",
    "MVC",
    "HEVC",
    "VP9"
};

typedef struct venc_t
{
	int venc_id;

	unsigned long enc_fd;
	int codec_format;

	unsigned int total_frm;

	venc_v3_init_t init_info;
	venc_v3_putheader_t putheader_info;
	venc_v3_encode_t encode_info;
	venc_v3_close_t close_info;

	unsigned int seq_header_count;

	unsigned int frame_index;
	int bitstream_buf_size;
	vpu_addr_t bitstream_buf_addr[3];

	int encoded_buf_size;
	vpu_addr_t encoded_buf_base_pos[3];
	vpu_addr_t encoded_buf_end_pos[3];
	vpu_addr_t encoded_buf_cur_pos[3];
	int keyInterval_cnt;

	int enc_aud_enable;

#ifdef MULTI_SLICES_AVC
	vpu_addr_t enc_slice_info_addr[3];
	unsigned int enc_slice_info_size;
#endif

	struct pollfd tcc_event[1];

	unsigned char *seq_backup;
	unsigned int seq_len;

	int enc_avc_idr_frame_enable;

	long long sum_of_time;
} venc_t;

static const char* codec_id_to_string(enum vpu_codec_id codec_id)
{
	const char* codec_string = NULL;
	int nb_codec = sizeof(vpu_codec_string) / sizeof(vpu_codec_string[0]);

	if((codec_id >= 0) && (codec_id < nb_codec))
	{
		codec_string = vpu_codec_string[codec_id];
	}

	return codec_string;
}

static int venc_memcpy(char *d, char *s, int size)
{
    int cnt = 0;
    //printf("In %s, [0x%08x] -> [0x%08x], size[%d]", __func__, s, d, size);
    while (size--)
    {
        *d++ = *s++;
        cnt++;
    }
    return cnt;
}

static unsigned long *venck_sys_malloc_virtual_addr(vpu_addr_t pPtr, int uiSize, venc_t *pInst)
{
    unsigned long *map_ptr = MAP_FAILED;

    map_ptr = (unsigned long *)mmap(NULL, uiSize, PROT_READ | PROT_WRITE, MAP_SHARED, pInst->enc_fd, pPtr);
    if (MAP_FAILED == map_ptr)
    {
        DLOGE("mmap failed. fd(%d), addr(0x%x), size(%d) err %d %s", pInst->enc_fd, pPtr, uiSize, errno, strerror(errno));
        return NULL;
    }

    return (unsigned long *)(map_ptr);
}

static int venck_sys_free_virtual_addr(unsigned long *pPtr, int uiSize)
{
    return munmap((unsigned long *)pPtr, uiSize);
}

static int venc_cmd_process(venc_t *pInst, int cmd, unsigned long *args)
{
    int ret = 0;
    int success = 0;
    int retry_cnt = 10;
    int all_retry_cnt = 3;

    ret = ioctl(pInst->enc_fd, cmd, args);

    if (ret < 0)
    {
        if (ret == -0x999)
        {
            DLOGE("VPU[%d] Invalid command(0x%x) ", pInst->venc_id, cmd);
            return VPU_RETCODE_INVALID_PARAM;
        }
        else
        {
            DLOGE("VPU[%d] ioctl err[%s] : cmd = 0x%x", pInst->venc_id, strerror(errno), cmd);
        }
    }

Retry:
    while (retry_cnt > 0)
    {
        int ret;
        memset(pInst->tcc_event, 0, sizeof(pInst->tcc_event));
        pInst->tcc_event[0].fd = pInst->enc_fd;
        pInst->tcc_event[0].events = POLLIN;

        ret = poll((struct pollfd *)&pInst->tcc_event, 1, 10000); // 100 msec
        if (ret < 0)
        {
            DLOGE("vpu(0x%x)-retry(%d:cmd(%d)) poll error '%s'", cmd, retry_cnt, cmd, strerror(errno));
            retry_cnt--;
            continue;
        }
        else if (ret == 0)
        {
            DLOGE("vpu(0x%x)-retry(%d:cmd(%d)) poll timeout", cmd, retry_cnt, cmd);
            retry_cnt--;
            continue;
        }
        else if (ret > 0)
        {
            if (pInst->tcc_event[0].revents & POLLERR)
            {
                DLOGE("vpu(0x%x) poll POLLERR", cmd);
                break;
            }
            else if (pInst->tcc_event[0].revents & POLLIN)
            {
                success = 1;
                break;
            }
        }
    }

    switch (cmd)
    {
		case VENC_V3_INIT:
		{
			venc_v3_init_t *init_info = args;

			ioctl(pInst->enc_fd, VENC_V3_INIT_RESULT, args);
			ret = init_info->result;
		}
		break;

		case VENC_V3_PUT_HEADER:
		{
			venc_v3_putheader_t *putheader_info = args;

			ioctl(pInst->enc_fd, VENC_V3_PUT_HEADER_RESULT, args);
			ret = putheader_info->result;
		}
		break;

		case VENC_V3_ENCODE:
		{
			venc_v3_encode_t *encode_info = args;

			ioctl(pInst->enc_fd, VENC_V3_ENCODE_RESULT, args);
			ret = encode_info->result;
		}
		break;

		case VENC_V3_CLOSE:
		{
			venc_v3_close_t *close_info = args;

			ioctl(pInst->enc_fd, VENC_V3_CLOSE_RESULT, args);
			ret = close_info->result;
		}
		break;

		default:
			DLOGE("Invalid ioctl:%d", cmd);
			ret = -1;
			break;
    }


    if ((ret & 0xf000) != 0x0000)
    { //If there is an invalid return, we skip it because this return means that vpu didn't process current command yet.
        all_retry_cnt--;
        if (all_retry_cnt > 0)
        {
            retry_cnt = 10;
            goto Retry;
        }
        else
        {
            DLOGE("abnormal exception!!");
        }
    }

    /* todo */
    if (!success || ((ret & 0xf000) != 0x0000))
    { /* vpu can not start or finish its processing with unknown reason!! */
        DLOGE("VENC command(0x%x) didn't work properly. maybe hangup(no return(0x%x))!!", cmd, ret);

        if (ret != VPU_RETCODE_CODEC_EXIT && ret != VPU_RETCODE_MULTI_CODEC_EXIT_TIMEOUT)
        {
            //ioctl(pInst->mgr_fd, VPU_HW_RESET, (unsigned long *)NULL);
        }

        return VPU_RETCODE_CODEC_EXIT;
    }

    return ret;
}

static enum vpu_codec_id venc_get_bitstream_format(enum venc_codec_id vpu_codec)
{
	enum vpu_codec_id codec_id = VCODEC_ID_MAX;

	switch(vpu_codec)
	{
		case VENC_CODEC_H264:
			codec_id = VCODEC_ID_AVC;
		break;

		case VENC_CODEC_HEVC:
			codec_id = VCODEC_ID_HEVC;
		break;

		case VENC_CODEC_MJPG:
			codec_id = VCODEC_ID_MJPG;
		break;

		default:
			codec_id = VCODEC_ID_MAX;
		break;
	}

	DLOGI( "get codec id, input:%d, output:%d", vpu_codec, codec_id);
	return codec_id;
}

static enum venc_codec_id convert_vpu_codec_id(enum vpu_codec_id codec_id)
{
	enum venc_codec_id venc_codec = VENC_CODEC_NONE;

	switch(codec_id)
	{
		case VCODEC_ID_AVC:
			venc_codec = VENC_CODEC_H264;
		break;

		case VCODEC_ID_MPEG4:
			venc_codec = VENC_CODEC_MPEG4;
		break;

		case VCODEC_ID_H263:
			venc_codec = VENC_CODEC_H263;
		break;

		case VCODEC_ID_HEVC:
			venc_codec = VENC_CODEC_HEVC;
		break;

		case VCODEC_ID_MJPG:
			venc_codec = VENC_CODEC_MJPG;
		break;

		default:
			venc_codec = VENC_CODEC_NONE;
		break;
	}

	//DLOGD( "get support codec, input id:%d to venc_codec:%d", codec_id, venc_codec);
	return venc_codec;
}

int venc_get_capability(venc_t *pInst, unsigned long* pParam1, unsigned long* pParam2)
{
	int ret = 0;

	vpu_drv_version_t vpu_version;
	vpu_capability_t vpu_capability;

	ret = ioctl(pInst->enc_fd, VPU_V3_GET_DRV_VERSION_SYNC, &vpu_version);
	if(ret == 0)
	{
		DLOGI( "[VENC-%d]", pInst->venc_id);
		DLOGI( "VPU Driver Ver %d.%d.%d - %s", vpu_version.major, vpu_version.minor, vpu_version.revision, (char*)vpu_version.reserved);
	}

	memset(&vpu_capability, 0x00, sizeof(vpu_capability_t));
	ret = ioctl(pInst->enc_fd, VPU_V3_GET_CAPABILITY_SYNC, &vpu_capability);
	if(ret == 0)
	{
		int ii;
		for(ii=0; ii<vpu_capability.num_of_codec_cap; ii++)
		{
			vpu_codec_cap_t* pcap = &vpu_capability.codec_caps[ii];

			if(pcap->support_codec != NULL)
			{
				DLOGI("\t%s %s %s %ux%u@%u", pcap->support_codec,
					(pcap->support_profile != NULL) ? pcap->support_profile : " ",
					(pcap->support_level != NULL) ? pcap->support_level : " ",
					pcap->max_width, pcap->max_height, pcap->max_fps);
			}
		}
	}

	return ret;
}

int venc_init(venc_handle_h handle, venc_init_t* p_init)
{
	venc_t* pInst = (venc_t*)handle;
	enum vpu_return_code vpu_ret = VPU_RETCODE_SUCCESS;
	int ret = 0;
	venc_init_t *p_init_param = (venc_init_t *)p_init;

	(void)memset(&pInst->init_info, 0x00, sizeof(venc_v3_init_t));

	pInst->init_info.input.codec_id = venc_get_bitstream_format(p_init_param->codec);
	pInst->init_info.input.pic_width = p_init_param->pic_width;
	pInst->init_info.input.pic_height = p_init_param->pic_height;
	pInst->init_info.input.frame_rate = p_init_param->framerate;
	pInst->init_info.input.target_kbps = p_init_param->bitrateKbps;
	pInst->init_info.input.key_interval = p_init_param->key_interval;
	pInst->init_info.input.enc_quality = p_init->encoding_quality;

	switch(p_init->source_format)
	{
		case VENC_SOURCE_YUV420P:
			pInst->init_info.input.yuv_format = VPU_SOURCE_YUV420;
			pInst->init_info.input.cbcr_interleave_mode = 0;
		break;

		case VENC_SOURCE_NV12:
			pInst->init_info.input.yuv_format = VPU_SOURCE_YUV420;
			pInst->init_info.input.cbcr_interleave_mode = 1;
		break;

		default:
			DLOGE("invalid source format");
		break;
	}

	pInst->init_info.input.use_specific_rc_option =  1;
	pInst->init_info.input.rc.avc_fast_encoding = p_init_param->avc_fast_encoding;
	pInst->init_info.input.rc.pic_qp_y = (p_init_param->initial_qp > 0) ? p_init_param->initial_qp : -1;
	pInst->init_info.input.rc.intra_mb_refresh = 0;
	pInst->init_info.input.rc.deblk_disable = p_init_param->deblk_disable;
	pInst->init_info.input.rc.deblk_alpha = 0;
	pInst->init_info.input.rc.deblk_beta = 0;
	pInst->init_info.input.rc.deblk_ch_qp_offset = 0;
	pInst->init_info.input.rc.constrained_intra = 0;
	pInst->init_info.input.rc.vbv_buffer_size = p_init_param->vbv_buffer_size;
	pInst->init_info.input.rc.search_range = 2;
	pInst->init_info.input.rc.pvm_disable = 0;
	pInst->init_info.input.rc.weight_intra_cost = 0;
	pInst->init_info.input.rc.rc_interval_mode = 1;
	pInst->init_info.input.rc.rc_interval_mbnum = 0;

	DLOGI("[VENC-%d] Init Encoding, codec:%s", pInst->venc_id, codec_id_to_string(pInst->init_info.input.codec_id));

	if(p_init_param->max_p_qp > 0)
	{
		pInst->init_info.input.rc.enc_quality_level = (p_init_param->max_p_qp << 16U);
	}
	else
	{
		pInst->init_info.input.rc.enc_quality_level = 11;
	}

	pInst->init_info.input.enc_opt_flags = 0;

#ifdef MULTI_SLICES_AVC
	if (pInst->init_info.input.codec_id == VCODEC_ID_AVC)
	{
		pInst->init_info.input.rc.slice_mode = p_init_param->slice_mode;
		pInst->init_info.input.rc.slice_size_mode = p_init_param->slice_size_mode;
		pInst->init_info.input.rc.slice_size = p_init_param->slice_size;

		if (pInst->init_info.input.rc.slice_mode == 1)
			pInst->enc_aud_enable = 1;
		else
			pInst->enc_aud_enable = 0;
	}
	else
#endif
	{
		pInst->init_info.input.rc.slice_mode = 0;
		pInst->init_info.input.rc.slice_size_mode = 0;
		pInst->init_info.input.rc.slice_size = 0;
	}
	DLOGI("SliceMode[%d] - SizeMode[%d] - %d", pInst->init_info.input.rc.slice_mode, pInst->init_info.input.rc.slice_size_mode, pInst->init_info.input.rc.slice_size);

	pInst->keyInterval_cnt = 0;

	if (pInst->init_info.input.frame_rate < 1)
	{
		DLOGI("[VENC-%d] Set framerate into minimum value (1). The encoded bitrate value will not match the target value", pInst->venc_id);
		pInst->init_info.input.frame_rate = 1;
	}

	DLOGI("[VENC-%d] %d x %d, %d fps, %d key-interval, %d target kbps(%s)",
		pInst->venc_id,
		pInst->init_info.input.pic_width, pInst->init_info.input.pic_height,
		pInst->init_info.input.frame_rate,
		pInst->init_info.input.key_interval,
		pInst->init_info.input.target_kbps,
		pInst->init_info.input.target_kbps == 0 ? "VBR mode" : "CBR mode");

	if (pInst->init_info.input.use_specific_rc_option == 1)
	{
		DLOGI("[VENC-%d] fast encoding %s, deblk %s, %s mode",
		pInst->venc_id,
		pInst->init_info.input.rc.avc_fast_encoding == 1 ? "enable" : "disable",
		pInst->init_info.input.rc.deblk_disable == 1 ? "enable" : "disable",
		pInst->init_info.input.rc.slice_mode == 0 ? "frame" : "slice");

		if (pInst->init_info.input.rc.pic_qp_y > 0)
		{
			DLOGI("[VENC-%d] initial qp: %d", pInst->venc_id, pInst->init_info.input.rc.pic_qp_y);
		}

		/*
		if (p_init_param->max_p_qp > 0)
		{
			DLOGE("[VENC-%d] maximum inter qp: %d(0x%x)", pInst->venc_id, pInst->init_info.input.rc.enc_quality_level);
		}
		else
		{
			DLOGE("[VENC-%d] quality level: %d", pInst->venc_id, pInst->init_info.input.rc.enc_quality_level);
		}
		*/
	}

	vpu_ret = venc_cmd_process(pInst, VENC_V3_INIT, &pInst->init_info);
	if (vpu_ret != VPU_RETCODE_SUCCESS)
	{
		DLOGE("[VENC-%d,Err:0x%x] venc_vpu VPU_ENC_INIT failed", pInst->venc_id, ret);
		return -1;
	}
	DLOGI("[VENC-%d] venc_vpu VPU_ENC_INIT (Success)", pInst->venc_id);

	pInst->bitstream_buf_addr[VENC_PA] = pInst->init_info.output.bitstream_out[VPU_PA];
	pInst->bitstream_buf_addr[VENC_KVA] = pInst->init_info.output.bitstream_out[VPU_KVA];
	pInst->bitstream_buf_size = pInst->init_info.output.bitstream_outsize;
	pInst->encoded_buf_size = pInst->init_info.output.bitstream_outsize;

	DLOGI("[VENC-%d] bitstream_buf_addr[PA] = 0x%x, 0x%x ", pInst->venc_id, (vpu_addr_t)pInst->bitstream_buf_addr[VENC_PA], pInst->encoded_buf_size);
	pInst->bitstream_buf_addr[VENC_VA] = (vpu_addr_t)venck_sys_malloc_virtual_addr(pInst->bitstream_buf_addr[VENC_PA], pInst->encoded_buf_size, pInst);
	if (pInst->bitstream_buf_addr[VENC_VA] == 0)
	{
		DLOGE("[VENC] bitstream_buf_addr[VA] malloc() failed");
		return -1;
	}

	memset( (unsigned long *)pInst->bitstream_buf_addr[VENC_VA], 0x00 , pInst->bitstream_buf_size);
	DLOGI("[VENC-%d] bitstream_buf_addr[VA] = 0x%x, 0x%x ", pInst->venc_id, (vpu_addr_t)pInst->bitstream_buf_addr[VENC_VA], pInst->encoded_buf_size);

	pInst->encoded_buf_base_pos[VENC_PA] = pInst->bitstream_buf_addr[VENC_PA];
	pInst->encoded_buf_base_pos[VENC_VA] = pInst->bitstream_buf_addr[VENC_VA];
	pInst->encoded_buf_base_pos[VENC_KVA] = pInst->bitstream_buf_addr[VENC_KVA];
	pInst->encoded_buf_cur_pos[VENC_PA] = pInst->encoded_buf_base_pos[VENC_PA];
	pInst->encoded_buf_cur_pos[VENC_VA] = pInst->encoded_buf_base_pos[VENC_VA];
	pInst->encoded_buf_cur_pos[VENC_KVA] = pInst->encoded_buf_base_pos[VENC_KVA];
	pInst->encoded_buf_end_pos[VENC_PA] = pInst->encoded_buf_base_pos[VENC_PA] + pInst->encoded_buf_size;
	pInst->encoded_buf_end_pos[VENC_VA] = pInst->encoded_buf_base_pos[VENC_VA] + pInst->encoded_buf_size;
	pInst->encoded_buf_end_pos[VENC_KVA] = pInst->encoded_buf_base_pos[VENC_KVA] + pInst->encoded_buf_size;

	(void)memset((void*)pInst->encoded_buf_base_pos[VENC_VA], 0x0, pInst->encoded_buf_size);

	DLOGI("[VENC-%d] PA = 0x%x, VA = 0x%x, size = 0x%x!!", pInst->venc_id, pInst->encoded_buf_base_pos[VENC_PA], pInst->encoded_buf_base_pos[VENC_VA], pInst->encoded_buf_size);
	DLOGI("VENC_V3_INIT ok ret:%d", ret);
	return ret;
}

int venc_put_seqheader(venc_handle_h handle, venc_seq_header_t* p_seqhead)
{
	venc_t* pInst = (venc_t*)handle;
	enum vpu_return_code vpu_ret = VPU_RETCODE_SUCCESS;
	int ret = 0;
	venc_seq_header_t *p_seq_param = (venc_seq_header_t *)p_seqhead;
	unsigned char *p_dest = NULL;
	int i_dest_size = 0;
	int aud_size = 0;

	DLOGI("[VENC-%d] Put Header, enc_aud_enable:%d", pInst->venc_id, pInst->enc_aud_enable);

	(void)memset(&pInst->putheader_info, 0x00, sizeof(venc_v3_putheader_t));

	if (pInst->seq_backup == NULL)
	{
		pInst->seq_backup = (unsigned char *)malloc(ALIGNED_BUFF(100 * 1024, ALIGN_LEN));
		pInst->seq_len = 0;
		DLOGI("[VENC-%d] alloc seq_backup:%x", pInst->venc_id, pInst->seq_backup);
	}

	//When it is the Nth sequence header instead of the 0th, the next sequence data is appended after the preceding sequence data
	p_dest = (unsigned char *)pInst->seq_backup + pInst->seq_len;

	DLOGI("[VENC-%d] Put Header, codec:%s", pInst->venc_id, codec_id_to_string(pInst->init_info.input.codec_id));
	if (pInst->init_info.input.codec_id == VCODEC_ID_AVC)
	{
		pInst->putheader_info.header_type = VPU_HEADER_AVC_SPS | VPU_HEADER_AVC_PPS;
		pInst->putheader_info.bitstream_buffer_addr[VPU_PA] = pInst->bitstream_buf_addr[VENC_PA];
		pInst->putheader_info.bitstream_buffer_addr[VPU_KVA] = pInst->bitstream_buf_addr[VENC_KVA];
		pInst->putheader_info.bitstream_buffer_size = pInst->bitstream_buf_size;

		DLOGI("[VENC-%d] VENC_V3_PUT_HEADER for SPS/PPS, dest:0x%x, prev seq_len:%d", pInst->venc_id, p_dest, pInst->seq_len);
		vpu_ret = venc_cmd_process(pInst, VENC_V3_PUT_HEADER, &pInst->putheader_info);
		if (vpu_ret != VPU_RETCODE_SUCCESS)
		{
			DLOGE("[VENC-%d:Err:0x%x] venc_vpu VENC_V3_PUT_HEADER SPS/PPS failed", pInst->venc_id, ret);
			return -1;
		}

		venc_memcpy(p_dest, pInst->bitstream_buf_addr[VENC_VA], pInst->putheader_info.bitstream_buffer_size);
		pInst->seq_len += pInst->putheader_info.bitstream_buffer_size;
	}
	else if (pInst->init_info.input.codec_id == VCODEC_ID_HEVC)
	{
		pInst->putheader_info.header_type = VPU_HEADER_HEVC_VPS | VPU_HEADER_HEVC_SPS | VPU_HEADER_HEVC_PPS;
		pInst->putheader_info.bitstream_buffer_addr[VPU_PA] = pInst->bitstream_buf_addr[VENC_PA];
		pInst->putheader_info.bitstream_buffer_addr[VPU_KVA] = pInst->bitstream_buf_addr[VENC_KVA];
		pInst->putheader_info.bitstream_buffer_size = pInst->bitstream_buf_size;

		DLOGI("[VENC-%d] VENC_V3_PUT_HEADER, dest:0x%x, prev seq_len:%d", pInst->venc_id, p_dest, pInst->seq_len);
		vpu_ret = venc_cmd_process(pInst, VENC_V3_PUT_HEADER, &pInst->putheader_info);
		if (vpu_ret != VPU_RETCODE_SUCCESS)
		{
			DLOGE("[VENC-%d:Err:0x%x] venc_vpu VENC_V3_PUT_HEADER failed", pInst->venc_id, ret);
			return -1;
		}

		venc_memcpy(p_dest, pInst->bitstream_buf_addr[VENC_VA], pInst->putheader_info.bitstream_buffer_size + aud_size);
		pInst->seq_len += pInst->putheader_info.bitstream_buffer_size + aud_size;
	}

	if (venc_log_level == VENC_LOG_INFO)
	{
		int ii;

		DLOGI("[VENC-%d] Sequence Header, size:%d ----", pInst->venc_id, pInst->seq_len);
		for (ii=0; ii<pInst->seq_len; ii++)
		{
			printf("0x%X ", pInst->seq_backup[ii]);
		}
		DLOGI("[VENC-%d] Sequence Header ++++", pInst->venc_id);
	}

	// output
	p_seq_param->seq_header_out = p_dest;
	p_seq_param->seq_header_out_size = pInst->seq_len;

	pInst->seq_header_count++;

	DLOGI("[VENC-%d] VENC_ENC_PUT_HEADER - OK, ret:%d", pInst->venc_id, ret);

	return ret;
}

int venc_encode(venc_handle_h handle, venc_input_t* p_input, venc_output_t* p_output)
{
	venc_t* pInst = (venc_t*)handle;
	enum vpu_return_code vpu_ret = VPU_RETCODE_SUCCESS;
	int ret = 0;
	venc_input_t* p_input_param = (venc_input_t *)p_input;
	venc_output_t* p_output_param = (venc_output_t *)p_output;
	int bChanged_fps = 0; //for only MPEG4.
	struct timespec start_time;
	struct timespec end_time;
	long long elapsed_us;
	//! Start encoding a frame.

	DLOGI("[VENC-%d] VENC_V3_ENCODE --", pInst->venc_id);

	clock_gettime(CLOCK_MONOTONIC, &start_time);

	(void)memset(&pInst->encode_info, 0x00, sizeof(venc_v3_encode_t));

	//pInst->encode_info.input.change_rc_param_flag = p_input_param->change_rc_param_flag;
	pInst->encode_info.input.change_target_kbps = p_input_param->change_target_bitrate_kbps;
	if(pInst->encode_info.input.change_target_kbps != 0)
	{
		pInst->encode_info.input.change_rc_param_flag |= VENC_V3_RC_FLAG_ENABLE;
	}

	pInst->encode_info.input.change_framerate = p_input_param->change_framerate;
	if(pInst->encode_info.input.change_framerate != 0)
	{
		pInst->encode_info.input.change_rc_param_flag |= VENC_V3_RC_FLAG_FRAMERATE;
	}

	if(pInst->encode_info.input.change_rc_param_flag != 0)
	{
		pInst->encode_info.input.change_rc_param_flag |= VENC_V3_RC_FLAG_ENABLE;
	}

	if (p_input_param->request_IntraFrame == 1)
	{
		pInst->encode_info.input.force_i_picture = 1; //set 1 For IDR-Type I-Frame without P-Frame!!
	}
	else
	{
		pInst->encode_info.input.force_i_picture= 0;
	}

	pInst->encode_info.input.skip_picture = 0;
	if (pInst->init_info.input.target_kbps == 0) // no rate control
	{
		pInst->encode_info.input.quant_param = 23;
	}
	else
	{
		if (p_input_param->quant_param > 0)
			pInst->encode_info.input.quant_param = p_input_param->quant_param;
		else
			pInst->encode_info.input.quant_param = 10;
	}

	if (pInst->encoded_buf_cur_pos[VENC_PA] + pInst->encoded_buf_size / 2 > pInst->encoded_buf_end_pos[VENC_PA])
	{
		pInst->encoded_buf_cur_pos[VENC_PA] = pInst->encoded_buf_base_pos[VENC_PA];
		pInst->encoded_buf_cur_pos[VENC_VA] = pInst->encoded_buf_base_pos[VENC_VA];
		pInst->encoded_buf_cur_pos[VENC_KVA] = pInst->encoded_buf_base_pos[VENC_KVA];
	}

#ifdef INSERT_SEQ_HEADER_IN_FORCED_IFRAME
	//if (0) // !bChanged_fps && (/*pInst->frame_index != 0 && */ ((pInst->frame_index % pInst->gsVpuEncInit_Info.gsVpuEncInit.framerate) == 0)) && pInst->gsVpuEncInit_Info.gsVpuEncInit.m_stRcInit.slice_mode != 1)
	if (pInst->enc_avc_idr_frame_enable == 1 && pInst->frame_index != 0 && pInst->frame_index % pInst->gsVpuEncInit_Info.gsVpuEncInit.key_interval == 0)
	{
		pInst->encoded_buf_cur_pos[VENC_PA] = pInst->encoded_buf_base_pos[PA] + ALIGNED_BUFF(pInst->seq_len, ALIGN_LEN);
		pInst->encoded_buf_cur_pos[VENC_VA] = pInst->encoded_buf_base_pos[VA] + ALIGNED_BUFF(pInst->seq_len, ALIGN_LEN);
		pInst->encoded_buf_cur_pos[VENC_KVA] = pInst->encoded_buf_base_pos[K_VA] + ALIGNED_BUFF(pInst->seq_len, ALIGN_LEN);
		pInst->encode_info.input.bitstream_buffer_size -= ALIGNED_BUFF(pInst->seq_len, ALIGN_LEN);
	}
#endif

	(void)memset((void*)pInst->encoded_buf_cur_pos[VENC_VA], 0x0, pInst->encoded_buf_end_pos[VENC_VA] - pInst->encoded_buf_cur_pos[VENC_VA]);

	pInst->encode_info.input.bitstream_buffer_addr[VPU_PA] = (vpu_addr_t)pInst->encoded_buf_cur_pos[VENC_PA];
	pInst->encode_info.input.bitstream_buffer_addr[VPU_KVA] = (vpu_addr_t)pInst->encoded_buf_cur_pos[VENC_KVA];
	pInst->encode_info.input.bitstream_buffer_size = pInst->bitstream_buf_size;

	DLOGI("[VENC-%d] bitstream addr:0x%x/0x%x, size:%d, end:0x%x, cur:0x%x, calc size:%d", pInst->venc_id,
			pInst->encode_info.input.bitstream_buffer_addr[VPU_PA], pInst->encode_info.input.bitstream_buffer_addr[VPU_KVA], pInst->encode_info.input.bitstream_buffer_size,
			pInst->encoded_buf_end_pos[VENC_VA], pInst->encoded_buf_cur_pos[VENC_VA], (pInst->encoded_buf_end_pos[VENC_VA] - pInst->encoded_buf_cur_pos[VENC_VA]));

	pInst->encode_info.input.pic_y_addr = (vpu_addr_t)p_input_param->input_y;
	if (pInst->init_info.input.cbcr_interleave_mode == 0)
	{
		pInst->encode_info.input.pic_cb_addr = (vpu_addr_t)p_input_param->input_crcb[0];
		pInst->encode_info.input.pic_cr_addr = (vpu_addr_t)p_input_param->input_crcb[1];
	}
	else
	{
		pInst->encode_info.input.pic_cb_addr = (vpu_addr_t)p_input_param->input_crcb[0];
	}

#ifdef MULTI_SLICES_AVC
	//H.264 AUD RBSP
	if (pInst->enc_aud_enable == 1 && pInst->frame_index > 0)
	{
		venc_memcpy((unsigned long *)pInst->encoded_buf_cur_pos[VENC_VA], (unsigned long *)avcAudData, 8);
		pInst->encode_info.input.bitstream_buffer_addr[VPU_PA] += 8;
		pInst->encode_info.input.bitstream_buffer_size -= 8;

		DLOGI("[VENC-%d] Insert H.264 AUD, bitstream PA addr:0x%x, size:%d", pInst->venc_id, pInst->encode_info.input.bitstream_buffer_addr[VPU_PA], pInst->encode_info.input.bitstream_buffer_size);
	}
#endif

	DLOGI("[VENC-%d] ioctl VENC_V3_ENCODE --", pInst->venc_id);
	vpu_ret = venc_cmd_process(pInst, VENC_V3_ENCODE, &pInst->encode_info);
	DLOGI("[VENC-%d] ioctl VENC_V3_ENCODE ++,. ret:%d", pInst->venc_id, ret);
	pInst->total_frm++;

	if (vpu_ret != VPU_RETCODE_SUCCESS)
	{
		DLOGE("[VENC:Err:0x%x] %d'th VPU_ENC_ENCODE failed", ret, pInst->total_frm);
		return -1;
	}

#ifdef MULTI_SLICES_AVC
	if (pInst->init_info.input.rc.slice_mode == 1)
	{
		if (pInst->enc_aud_enable == 1 && pInst->frame_index > 0)
		{
			DLOGI("[VENC-%d] enc_aud_enabled, offset -8, size +8", pInst->venc_id);
			pInst->encode_info.output.encoded_stream_addr[VPU_PA] -= 8;
			pInst->encode_info.output.encoded_stream_addr[VPU_KVA] -= 8;
			pInst->encode_info.output.encoded_stream_size += 8;
		}
	}
#endif

	if(0)
	{
		unsigned char* ptr = (unsigned char*)pInst->encoded_buf_cur_pos[VENC_VA];
		DLOGI("[VENC-%d]  picture type:%d, size:%d, %x %x %x %x %x %x %x %x", pInst->venc_id,
			pInst->encode_info.output.pic_type, pInst->encode_info.output.encoded_stream_size, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
	}

	// output
	if (pInst->init_info.input.codec_id == VCODEC_ID_AVC)
	{
		//p_output_param->bitstream_out = (unsigned char *)pInst->encoded_buf_cur_pos[VENC_VA];
		//p_output_param->bitstream_out += 4;
		//p_output_param->bitstream_out_size = pInst->encode_info.output.encoded_stream_size - 4;
		p_output_param->bitstream_out = (unsigned char *)pInst->encoded_buf_cur_pos[VENC_VA];
		p_output_param->bitstream_out_size = pInst->encode_info.output.encoded_stream_size;
	}
	else
	{
		p_output_param->bitstream_out = (unsigned char *)pInst->encoded_buf_cur_pos[VENC_VA];
		p_output_param->bitstream_out_size = pInst->encode_info.output.encoded_stream_size;
	}

#ifdef INSERT_SEQ_HEADER_IN_FORCED_IFRAME
	//if (0) //!bChanged_fps && /*pInst->frame_index != 0 && */ ((pInst->frame_index % pInst->gsVpuEncInit_Info.gsVpuEncInit.framerate) == 0))
	if (pInst->enc_avc_idr_frame_enable == 1 && pInst->frame_index != 0 && pInst->frame_index % pInst->gsVpuEncInit_Info.gsVpuEncInit.key_interval == 0)
	{
		DLOGI("Inserted sequence header prior to Stream.")
		p_output_param->bitstream_out -= pInst->seq_len;
		venc_memcpy((unsigned long *)p_output_param->bitstream_out, (unsigned long *)pInst->seq_backup, pInst->seq_len);
		p_output_param->bitstream_out_size += pInst->seq_len;
	}
#endif
	pInst->encoded_buf_cur_pos[VENC_PA] += ALIGNED_BUFF(p_output_param->bitstream_out_size + (STABILITY_GAP * 2), ALIGN_LEN);
	pInst->encoded_buf_cur_pos[VENC_VA] += ALIGNED_BUFF(p_output_param->bitstream_out_size + (STABILITY_GAP * 2), ALIGN_LEN);
	pInst->encoded_buf_cur_pos[VENC_KVA] += ALIGNED_BUFF(p_output_param->bitstream_out_size + (STABILITY_GAP * 2), ALIGN_LEN);

	switch(pInst->encode_info.output.pic_type)
	{
		case VPU_PICTURE_I:
			p_output_param->pic_type = VENC_PICTYPE_I;
		break;

		case VPU_PICTURE_P:
			p_output_param->pic_type = VENC_PICTYPE_P;
		break;

		case VPU_PICTURE_B:
			p_output_param->pic_type = VENC_PICTYPE_B;
		break;

		default:
			p_output_param->pic_type = VENC_PICTYPE_NONE;
		break;
	}

	pInst->frame_index++;

#ifdef INSERT_SEQ_HEADER_IN_FORCED_IFRAME

	if (pInst->gsVpuEncInit_Info.gsVpuEncInit.codec == STD_MPEG4 && bChanged_fps)
	{
		pInst->gsVpuEncPutHeader_Info.gsVpuEncHeader.m_iHeaderType = MPEG4_VOL_HEADER;
		pInst->gsVpuEncPutHeader_Info.gsVpuEncHeader.m_HeaderAddr = pInst->gspSeqHeaderAddr[VENC_PA];
		pInst->gsVpuEncPutHeader_Info.gsVpuEncHeader.m_iHeaderSize = pInst->gsiSeqHeaderSize;
#if defined(TCC_VPU_C7_INCLUDE)
		pInst->gsVpuEncPutHeader_Info.gsVpuEncHeader.m_HeaderAddr_VA = pInst->gspSeqHeaderAddr[VENC_KVA];
#endif

		DSTATUS("[VENC] VPU_ENC_PUT_HEADER for MPEG4_VOL_HEADER");
		ret = venc_cmd_process(pInst, V_ENC_PUT_HEADER, &pInst->gsVpuEncPutHeader_Info);

		if (ret != VPU_RETCODE_SUCCESS)
		{
			DLOGE("[VENC:Err:0x%x] venc_vpu MPEG4_VOL_HEADER failed ", ret);
		}
		else
		{
			if (pInst->seq_backup == NULL)
			{
				pInst->seq_backup = (unsigned char *)malloc(pInst->gsVpuEncPutHeader_Info.gsVpuEncHeader.m_iHeaderSize + (STABILITY_GAP * 2));
			}
			venc_memcpy((unsigned long *)pInst->seq_backup, (unsigned long *)pInst->gspSeqHeaderAddr[VENC_VA], pInst->gsVpuEncPutHeader_Info.gsVpuEncHeader.m_iHeaderSize);
			pInst->seq_len = pInst->gsVpuEncPutHeader_Info.gsVpuEncHeader.m_iHeaderSize;
		}
	}
#endif

	if (0)
	{
		unsigned char *ps = (unsigned char *)p_output_param->bitstream_out;
		DLOGI("[VPU_ENC-%d] "
				"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x "
				"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x "
				"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x "
				"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x "
				"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x ",
				pInst->venc_id,
				ps[0], ps[1], ps[2], ps[3], ps[4], ps[5], ps[6], ps[7], ps[8], ps[9], ps[10], ps[11], ps[12], ps[13], ps[14], ps[15],
				ps[16], ps[17], ps[18], ps[19], ps[20], ps[21], ps[22], ps[23], ps[24], ps[25], ps[26], ps[27], ps[28], ps[29], ps[30], ps[31],
				ps[32], ps[33], ps[34], ps[35], ps[36], ps[37], ps[38], ps[39], ps[40], ps[41], ps[42], ps[43], ps[44], ps[45], ps[46], ps[47],
				ps[48], ps[49], ps[50], ps[51], ps[52], ps[53], ps[54], ps[55], ps[56], ps[57], ps[58], ps[59], ps[60], ps[61], ps[62], ps[63],
				ps[64], ps[65], ps[66], ps[67], ps[68], ps[69], ps[70], ps[71], ps[72], ps[73], ps[74], ps[75], ps[76], ps[77], ps[78], ps[79]);
	}

	clock_gettime(CLOCK_MONOTONIC, &end_time);
	elapsed_us = (end_time.tv_sec - start_time.tv_sec) * 1000000LL +
                 (end_time.tv_nsec - start_time.tv_nsec) / 1000LL;
#if 0
	pInst->sum_of_time += elapsed_us;

	if((pInst->total_frm % 30) == 0)
	{
		long long avg_time_us = (long long)(pInst->sum_of_time / pInst->total_frm);
		DLOGE("[VENC-%d] avg elapsed %lld us", pInst->venc_id, avg_time_us);
	}
#endif

	DLOGI("[VENC-%d] VENC_V3_ENCODE +++, ret:%d", pInst->venc_id, ret);
	return ret;
}

int venc_close(venc_handle_h handle)
{
	venc_t* pInst = (venc_t*)handle;
	enum vpu_return_code vpu_ret = VPU_RETCODE_SUCCESS;
	int ret = 0;

	DLOGI("[VENC-%d] VENC_V3_CLOSE ---", pInst->venc_id);

	(void)memset(&pInst->close_info, 0x00, sizeof(venc_v3_close_t));

	vpu_ret = venc_cmd_process(pInst, VENC_V3_CLOSE, &pInst->close_info);
	if (vpu_ret != VPU_RETCODE_SUCCESS)
	{
		DLOGE("[VENC] VPU_ENC_CLOSE failed Error code is 0x%x ", ret);
		ret = -1;
	}

	if (pInst->bitstream_buf_addr[VENC_VA])
	{
		venck_sys_free_virtual_addr((unsigned long *)pInst->bitstream_buf_addr[VENC_VA], pInst->bitstream_buf_size);
		pInst->bitstream_buf_addr[VENC_VA] = 0;
	}

	if (pInst->seq_backup != NULL)
    {
        free(pInst->seq_backup);
        pInst->seq_backup = NULL;
        pInst->seq_len = 0;
    }

	pInst->frame_index = 0;

	DLOGI("[VENC-%d] VENC_V3_CLOSE done, ret:%d", pInst->venc_id, ret);
	return ret;
}

venc_handle_h venc_alloc_instance(venc_info_t* p_info)
{
    venc_t *pInst = NULL;
    int fd;
	int nInstance;
	int ret = 0;

    pInst = (venc_t *)calloc(1, sizeof(venc_t));
    if (pInst != NULL)
    {
        memset(pInst, 0x00, sizeof(venc_t));
        pInst->enc_fd = -1;

		pthread_mutex_lock(&instance_mutex);

		DLOGI("open: %s \n",VPU_ENCODER_DEVICE);
        pInst->enc_fd = open(VPU_ENCODER_DEVICE, O_RDWR);
        if (pInst->enc_fd < 0)
        {
            DLOGE("%s open error[%s]", VPU_ENCODER_DEVICE, strerror(errno));
            ret = -1;
        }

		if((ret == 0) && (p_info != NULL))
		{
			vpu_drv_version_t vpu_version;
			vpu_capability_t vpu_capability;

			memset(&vpu_version, 0x00, sizeof(vpu_drv_version_t));
			ret = ioctl(pInst->enc_fd, VPU_V3_GET_DRV_VERSION_SYNC, &vpu_version);
			if(ret == 0)
			{
				DLOGI( "VPU Driver Ver %d.%d.%d - %s", vpu_version.major, vpu_version.minor, vpu_version.revision, (char*)vpu_version.reserved);
			}
			else
			{
				DLOGE("VPU_V3_GET_DRV_VERSION_SYNC error, ret:%d", ret);
			}

			memset(&vpu_capability, 0x00, sizeof(vpu_capability_t));
			ret = ioctl(pInst->enc_fd, VPU_V3_GET_CAPABILITY_SYNC, &vpu_capability);
			if(ret == 0)
			{
				int ii;

				p_info->num_of_codec_cap = vpu_capability.num_of_codec_cap;

				for(ii=0; ii<vpu_capability.num_of_codec_cap; ii++)
				{
					vpu_codec_cap_t* pcap = &vpu_capability.codec_caps[ii];

					if(pcap->support_codec != NULL)
					{
						venc_codec_cap_t* pEncCaps = &p_info->encoder_caps[ii];

						memset(pEncCaps, 0x00, sizeof(venc_codec_cap_t));
						pEncCaps->codec_id = convert_vpu_codec_id(pcap->codec_id);
						pEncCaps->max_fps = pcap->max_fps;
						pEncCaps->max_width = pcap->max_width;
						pEncCaps->max_height = pcap->max_height;

						strcpy(pEncCaps->support_codec, pcap->support_codec);

						if(pcap->support_profile != NULL)
						{
							strcpy(pEncCaps->support_profile, pcap->support_profile);
						}

						if(pcap->support_level != NULL)
						{
							strcpy(pEncCaps->support_level, pcap->support_level);
						}

						DLOGI("\t%s %s %s %ux%u@%u", pEncCaps->support_codec,
							(pEncCaps->support_profile != NULL) ? pEncCaps->support_profile : " ",
							(pEncCaps->support_level != NULL) ? pEncCaps->support_level : " ",
							pEncCaps->max_width, pEncCaps->max_height, pEncCaps->max_fps);
					}
				}
			}

			p_info->encoder_id = vpu_capability.drv_id;

			p_info->ver_major = vpu_version.major;
			p_info->ver_minor = vpu_version.minor;
			p_info->ver_revision = vpu_version.revision;

			pInst->venc_id = p_info->encoder_id;

			DLOGI("alloc Instance[%d] = %s\n", pInst->venc_id, VPU_ENCODER_DEVICE);
			total_opened_encoder++;
			DLOGI("[%d] venc_alloc_instance, opend:%d", pInst->venc_id, total_opened_encoder);
		}
		else
		{
			DLOGE("%s open error[%s]", VPU_ENCODER_DEVICE, strerror(errno));
			free(pInst);
			pInst = NULL;
		}

		pthread_mutex_unlock(&instance_mutex);
    }

    return (venc_handle_h)pInst;
}

void venc_release_instance(venc_handle_h handle)
{
	venc_t* pInst = (venc_t*)handle;

    if (pInst)
    {
		pthread_mutex_lock(&instance_mutex);

        if (pInst->enc_fd)
        {
            if (close(pInst->enc_fd) < 0)
            {
               // DLOGE("%s close error[%s]", VPU_ENCODER_DEVICE);
            }
            pInst->enc_fd = -1;
        }

        free(pInst);
        pInst = NULL;

        if (total_opened_encoder > 0)
            total_opened_encoder--;

        DLOGI("venc_release_instance, total opended encoder:%d", total_opened_encoder);
		pthread_mutex_unlock(&instance_mutex);
    }
}

int venc_set_log_level(enum venc_log_level log_level)
{
	venc_log_level = log_level;
	DLOGI("set log level:%d", venc_log_level);
	return venc_log_level;
}
