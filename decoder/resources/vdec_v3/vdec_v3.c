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
#include "vpu_v3/tcc_vpu_v3_decoder.h"
#include "vdec_v3.h"

#include <dlfcn.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>         // O_RDWR
#include <sys/poll.h>
#include <pthread.h>

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

#define DLOGV(...)	{ if(vdec_log_level >= VDEC_LOG_VERBOSE) { DBG_PRINT(__VA_ARGS__); } }
#define DLOGD(...)  { if(vdec_log_level >= VDEC_LOG_DEBUG) { DBG_PRINT(__VA_ARGS__); } }
#define DLOGI(...)  { if(vdec_log_level >= VDEC_LOG_INFO) { DBG_PRINT(__VA_ARGS__); } }
#define DLOGW(...)  { if(vdec_log_level >= VDEC_LOG_WARN) { DBG_PRINT(__VA_ARGS__); } }
#define DLOGE(...)  { if(vdec_log_level >= VDEC_LOG_ERROR) { DBG_PRINT(__VA_ARGS__); } }

#define VPU_DECODER_DEVICE "/dev/vpu_vdec"

#define VDEC_ALIGN32(_X)             ((_X+0x1f)&~0x1f)
#define VDEC_ALIGN64(_X)             ((_X+0x3f)&~0x3f)
#define ALIGN_LEN (4*1024)

//#define VPU_OUT_FRAME_DUMP // Change the output format into YUV420 seperated to play on PC.
//#define HEVC_OUT_FRAME_DUMP //HEVC Data save

static pthread_mutex_t instance_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t total_opened_decoder = 0;
static int vdec_log_level = VDEC_LOG_ERROR;

typedef struct vdec_t {
    int32_t vdec_id;
    uint32_t total_frm;

    int32_t dec_fd; // default -1

#if defined(VPU_OUT_FRAME_DUMP)
    FILE *pFs;
    uint32_t is1st_dec;
    unsigned char* backup_data;
    uint32_t backup_len;
#endif

    int32_t bitstream_buf_size;
    int32_t userdata_buf_size;
    int32_t max_bitstream_size;
    int32_t additional_frame_count;
    int32_t total_frame_count;

    vpu_addr_t bitstream_buf_addr[VDEC_ADDR_MAX];
    vpu_addr_t userdata_buf_addr[VDEC_ADDR_MAX];
	vpu_addr_t bitstream_buf_addr_end[VDEC_ADDR_MAX];

	//next input bitstream buffer information
	vpu_addr_t bitstream_buf_next[VDEC_ADDR_MAX];
	int32_t bitstream_buf_next_size;

	//second next input bitstream buffer information for ring buffer mode used
	vpu_addr_t bitstream_buf_next_2[VDEC_ADDR_MAX];
	int32_t bitstream_buf_next_size_2;

    int32_t frame_buf_size;
    vpu_addr_t frame_buf_addr[VDEC_ADDR_MAX];

	//vpu drvier's ioctl parameter
	vpu_drv_version_t vpu_version;
	vpu_capability_t vpu_capability;

	vdec_v3_init_t init_info;
	vdec_v3_seqheader_t seqenceheader_info;
	vdec_v3_decode_t decode_info;
	vdec_v3_buf_clear_t bufclear_info;
	vdec_v3_drain_t drain_info;
	vdec_v3_flush_t flush_info;
	vdec_v3_close_t close_info;

	int use_ringbuffer_mode; //default 0(=fileplay), 1(=ringbuffer)
} vdec_t;

static char* picture_type_name[VDEC_PICTYPE_MAX] =
{
	"I",
	"P",
	"B",
	"B_PB",
	"BI",
	"IDR",
	"SKIP",
	"UNKNOWN",
};

static char* get_pic_type_name(int pic_type)
{
	const char* name = NULL;

	if((pic_type >= 0) && (pic_type < VDEC_PICTYPE_MAX))
	{
		name = picture_type_name[pic_type];
	}
	else
	{
		name = picture_type_name[VDEC_PICTYPE_UNKNOWN];
	}

	return name;
}

static enum vpu_codec_id convert_vdec_codec_id(enum vdec_codec_id vcodec_id)
{
	enum vpu_codec_id ret_codec = VCODEC_ID_NONE;

	switch(vcodec_id)
	{
		case VDEC_CODEC_AVC:
			ret_codec = VCODEC_ID_AVC;
		break;

		case VDEC_CODEC_VC1:
			ret_codec = VCODEC_ID_VC1;
		break;

		case VDEC_CODEC_MPEG2:
			ret_codec = VCODEC_ID_MPEG2;
		break;

		case VDEC_CODEC_MPEG4:
			ret_codec = VCODEC_ID_MPEG4;
		break;

		case VDEC_CODEC_H263:
			ret_codec = VCODEC_ID_H263;
		break;

		case VDEC_CODEC_DIVX:
			ret_codec = VCODEC_ID_DIVX;
		break;

		case VDEC_CODEC_AVS:
			ret_codec = VCODEC_ID_AVS;
		break;

		case VDEC_CODEC_MJPG:
			ret_codec = VCODEC_ID_MJPG;
		break;

		case VDEC_CODEC_VP8:
			ret_codec = VCODEC_ID_VP8;
		break;

		case VDEC_CODEC_MVC:
			ret_codec = VCODEC_ID_MVC;
		break;

		case VDEC_CODEC_HEVC:
			ret_codec = VCODEC_ID_HEVC;
		break;

		case VDEC_CODEC_VP9:
			ret_codec = VCODEC_ID_VP9;
		break;

		default:
			ret_codec = VCODEC_ID_NONE;
		break;
	}

	return ret_codec;
}

static enum vpu_output_format convert_vdec_output_format(enum vdec_output_format vdec_out_fmt)
{
	enum vpu_output_format ret_output = 0;

	switch(vdec_out_fmt)
	{
		case VDEC_OUTPUT_LINEAR_YUV420:
			ret_output = VPU_OUTPUT_LINEAR_YUV420;
		break;

		case VDEC_OUTPUT_LINEAR_NV12:
			ret_output = VPU_OUTPUT_LINEAR_NV12;
		break;

		case VDEC_OUTPUT_LINEAR_10_TO_8_BIT_YUV420:
			ret_output = VPU_OUTPUT_LINEAR_10_TO_8_BIT_YUV420;
		break;

		case VDEC_OUTPUT_LINEAR_10_TO_8_BIT_NV12:
			ret_output = VPU_OUTPUT_LINEAR_10_TO_8_BIT_NV12;
		break;

		case VDEC_OUTPUT_COMPRESSED_MAPCONV:
			ret_output = VPU_OUTPUT_COMPRESSED_MAPCONV;
		break;

		case VDEC_OUTPUT_COMPRESSED_AFBC:
			ret_output = VPU_OUTPUT_COMPRESSED_AFBC;
		break;

		default:
			ret_output  = VPU_OUTPUT_LINEAR_YUV420;
		break;
	};

	return ret_output;
}

static enum vdec_codec_id convert_vpu_codec_id(enum vpu_codec_id codec_id)
{
	enum vdec_codec_id vdec_codec = VDEC_CODEC_NONE;

	switch(codec_id)
	{
		case VCODEC_ID_AVC:
			vdec_codec = VDEC_CODEC_AVC;
		break;

		case VCODEC_ID_VC1:
			vdec_codec = VDEC_CODEC_VC1;
		break;

		case VCODEC_ID_MPEG2:
			vdec_codec = VDEC_CODEC_MPEG2;
		break;

		case VCODEC_ID_MPEG4:
			vdec_codec = VDEC_CODEC_MPEG4;
		break;

		case VCODEC_ID_H263:
			vdec_codec = VDEC_CODEC_H263;
		break;

		case VCODEC_ID_DIVX:
			vdec_codec = VDEC_CODEC_DIVX;
		break;

		case VCODEC_ID_AVS:
			vdec_codec = VDEC_CODEC_AVS;
		break;

		case VCODEC_ID_MJPG:
			vdec_codec = VDEC_CODEC_MJPG;
		break;

		case VCODEC_ID_VP8:
			vdec_codec = VDEC_CODEC_VP8;
		break;

		case VCODEC_ID_MVC:
			vdec_codec = VDEC_CODEC_MVC;
		break;

		case VCODEC_ID_HEVC:
			vdec_codec = VDEC_CODEC_HEVC;
		break;

		case VCODEC_ID_VP9:
			vdec_codec = VDEC_CODEC_VP9;
		break;

		default:
			vdec_codec = VDEC_CODEC_NONE;
		break;
	}

	return vdec_codec;
}

static enum vdec_display_status convert_vpu_display_status(enum vpu_display_status vpu_disp_stat)
{
	enum vdec_display_status ret_stat = 0;

	switch(vpu_disp_stat)
	{
		case VPU_DISP_STAT_FAIL:
			ret_stat = VDEC_DISP_STAT_FAIL;
		break;

		case VPU_DISP_STAT_SUCCESS:
			ret_stat = VDEC_DISP_STAT_SUCCESS;
		break;

		default:
			ret_stat = VDEC_DISP_STAT_FAIL;
		break;
	}

	return ret_stat;
}

static enum vdec_decoded_status convert_vpu_decoded_status(enum vpu_decoded_status vpu_disp_stat)
{
	enum vdec_decoded_status ret_stat = 0;

	switch(vpu_disp_stat)
	{
		case VPU_DEC_STAT_NONE:
			ret_stat = VDEC_STAT_NONE;
		break;

		case VPU_DEC_STAT_SUCCESS:
			ret_stat = VDEC_STAT_SUCCESS;
		break;

		case VPU_DEC_STAT_INFO_NOT_SUFFICIENT_SPS_PPS_BUFF:
			ret_stat = VDEC_STAT_INFO_NOT_SUFFICIENT_SPS_PPS_BUFF;
		break;

		case VPU_DEC_STAT_INFO_NOT_SUFFICIENT_SLICE_BUFF:
			ret_stat = VDEC_STAT_INFO_NOT_SUFFICIENT_SLICE_BUFF;
		break;

		case VPU_DEC_STAT_BUF_FULL:
			ret_stat = VDEC_STAT_BUF_FULL;
		break;

		case VPU_DEC_STAT_SUCCESS_FIELD_PICTURE:
			ret_stat = VDEC_STAT_SUCCESS_FIELD_PICTURE;
		break;

		case VPU_DEC_STAT_DETECT_RESOLUTION_CHANGE:
			ret_stat = VDEC_STAT_DETECT_RESOLUTION_CHANGE;
		break;

		case VPU_DEC_STAT_INVALID_INSTANCE:
			ret_stat = VDEC_STAT_INVALID_INSTANCE;
		break;

		case VPU_DEC_STAT_DETECT_DPB_CHANGE:
			ret_stat = VDEC_STAT_DETECT_DPB_CHANGE;
		break;

		case VPU_DEC_STAT_QUEUEING_FAIL:
			ret_stat = VDEC_STAT_QUEUEING_FAIL;
		break;

		case VPU_DEC_STAT_VP9_SUPER_FRAME:
			ret_stat = VDEC_STAT_VP9_SUPER_FRAME;
		break;

		case VPU_DEC_STAT_CQ_EMPTY:
			ret_stat = VDEC_STAT_CQ_EMPTY;
		break;

		case VPU_DEC_STAT_REPORT_NOT_READY:
			ret_stat = VDEC_STAT_REPORT_NOT_READY;
		break;

		default:
			ret_stat = VDEC_STAT_NONE;
		break;
	}

	return ret_stat;
}

static vdec_crop_t convert_vpu_crop(vpu_crop_t crop)
{
	vdec_crop_t vdec_crop;

	vdec_crop.left = crop.left;
	vdec_crop.right = crop.right;
	vdec_crop.top = crop.top;
	vdec_crop.bottom = crop.bottom;

	return vdec_crop;
}

static unsigned long * cdk_sys_malloc_virtual_addr(unsigned long * pPtr, uint32_t uiSize, vdec_t *pVdec)
{
    unsigned long * map_ptr = MAP_FAILED;

    map_ptr = (void *)mmap(NULL, uiSize, PROT_READ | PROT_WRITE, MAP_SHARED, pVdec->dec_fd, pPtr);
    if (MAP_FAILED == map_ptr)
    {
        DLOGE("mmap failed. fd(%d), addr(0x%x), size(%d) err %d %s", pVdec->dec_fd, pPtr, uiSize, errno, strerror(errno));
        return NULL;
    }
    DLOGI("mmap ok. fd(%d), addr(0x%x), size(%d)", pVdec->dec_fd, pPtr, uiSize);

    return (unsigned long *)(map_ptr);
}

static int32_t cdk_sys_free_virtual_addr(unsigned long* pPtr, uint32_t uiSize)
{
    int ret = 0;

    if((ret = munmap((unsigned long*)pPtr, uiSize)) < 0)
    {
        DLOGE("munmap failed. addr(0x%x), size(%d)", (uint32_t)pPtr, uiSize);
    }

    return ret;
}

static enum vpu_return_code vdec_cmd_process(int32_t cmd, unsigned long* args, vdec_t *pVdec)
{
    enum vpu_return_code ret;
    int32_t success = 0;
    int32_t retry_cnt = 2;
    int32_t all_retry_cnt = 3;
	struct pollfd tcc_event[1];

    vdec_t * pInst = pVdec;

    if((ret = ioctl(pInst->dec_fd, cmd, args)) < 0)
    {
        if( ret == -0x999 )
        {
            DLOGE("[VDEC-%d] Invalid command(0x%x) ", pInst->vdec_id, cmd);
            return VPU_RETCODE_INVALID_PARAM;
        }
        else
        {
            DLOGE("[VDEC-%d] ioctl err[%s] : cmd = 0x%x(%d)", pInst->vdec_id, strerror(errno), cmd, cmd);
        }
    }

Retry:
    while (retry_cnt > 0) {
        memset(tcc_event, 0, sizeof(tcc_event));
        tcc_event[0].fd = pInst->dec_fd;
        tcc_event[0].events = POLLIN;

        ret = poll((struct pollfd *)&tcc_event, 1, 1000); // 1 sec
        if (ret < 0) {
            DLOGE("[VDEC-%d] -retry(%d:cmd(%d)) poll error '%s'", pInst->vdec_id, retry_cnt, cmd, strerror(errno));
            retry_cnt--;
            continue;
        }else if (ret == 0) {
            DLOGE("[VDEC-%d] -retry(%d:cmd(%d)) poll timeout: %d'th frames", pInst->vdec_id, retry_cnt, cmd, pInst->total_frm);
            retry_cnt--;
            continue;
        }else if (ret > 0) {
            if (tcc_event[0].revents & POLLERR) {
                DLOGE("[VDEC-%d]  poll POLLERR", pInst->vdec_id);
                break;
            } else if (tcc_event[0].revents & POLLIN) {
                success = 1;
                break;
            }
        }
    }
    /* todo */

#if 0 //get next cmd result
	{
		vdec_v3_get_next_result_t next_result;
		vdec_v3_get_next_result_t* pNext = &next_result;

		if((ret = ioctl(pInst->dec_fd, VDEC_V3_GET_NEXT_RESULT_SYNC, pNext)) < 0)
		{
			return VPU_RETCODE_INVALID_PARAM;
		}
		else
		{
			if(pNext->next_result_cmd != cmd)
			{
				 DLOGE("[VDEC-%d] ioctl(0x%x) not match to next result cmd(0x%x)", pInst->vdec_id, cmd, pNext->next_result_cmd);
				 return VPU_RETCODE_INVALID_PARAM;
			}

			DLOGI("[VDEC-%d] next result:0x%x, cmd:0x%x, remain:%d", pInst->vdec_id, pNext->next_result_cmd, cmd, pNext->remain_result);
		}

		cmd = pNext->next_result_cmd;
	}
#endif

    switch(cmd)
    {
        case VDEC_V3_INIT:
            {
                vdec_v3_init_t* init_info = args;

				DLOGI("[VDEC-%d] VDEC_V3_INIT_RESULT", pInst->vdec_id);
                if(ioctl(pInst->dec_fd, VDEC_V3_INIT_RESULT, init_info) < 0){
                    DLOGE("[VDEC-%d] ioctl(0x%x) error[%s]!!", pInst->vdec_id, VDEC_V3_INIT_RESULT, strerror(errno));
                }
                ret = init_info->result;
            }
            break;

        case VDEC_V3_SEQ_HEADER:
            {
                vdec_v3_seqheader_t* seq_info = args;
				DLOGI("[VDEC-%d] VDEC_V3_SEQ_HEADER_RESULT", pInst->vdec_id);
                if(ioctl(pInst->dec_fd, VDEC_V3_SEQ_HEADER_RESULT, seq_info) < 0)
				{
                    DLOGE("[VDEC-%d] ioctl(0x%x) error[%s]!!", pInst->vdec_id, VDEC_V3_SEQ_HEADER_RESULT, strerror(errno));
                }
                ret = seq_info->result;
            }
            break;

		case VDEC_V3_DECODE:
            {
                vdec_v3_decode_t* dec_info = args;
				DLOGI("[VDEC-%d] VDEC_V3_DECODE_RESULT", pInst->vdec_id);
                if(ioctl(pInst->dec_fd, VDEC_V3_DECODE_RESULT, dec_info) < 0)
				{
                    DLOGE("[VDEC-%d] ioctl(0x%x) error[%s]!!", pInst->vdec_id, VDEC_V3_DECODE_RESULT, strerror(errno));
                }
                ret = dec_info->result;
            }
            break;

		case VDEC_V3_BUF_CLEAR:
			{
				vdec_v3_buf_clear_t* bufclear_info = args;
				DLOGI("[VDEC-%d] VDEC_V3_BUF_CLEAR_RESULT", pInst->vdec_id);
                if(ioctl(pInst->dec_fd, VDEC_V3_BUF_CLEAR_RESULT, bufclear_info) < 0)
				{
                    DLOGE("[VDEC-%d] ioctl(0x%x) error[%s]!!", pInst->vdec_id, VDEC_V3_BUF_CLEAR_RESULT, strerror(errno));
                }
                ret = bufclear_info->result;
			}
			break;

		case VDEC_V3_FLUSH:
			{
				vdec_v3_flush_t* flush_info = args;
				DLOGI("[VDEC-%d] VDEC_V3_FLUSH_RESULT", pInst->vdec_id);
                if(ioctl(pInst->dec_fd, VDEC_V3_FLUSH_RESULT, flush_info) < 0)
				{
                    DLOGE("[VDEC-%d] ioctl(0x%x) error[%s]!!", pInst->vdec_id, VDEC_V3_FLUSH_RESULT, strerror(errno));
                }
                ret = flush_info->result;
			}
		break;

		case VDEC_V3_DRAIN:
			{
				vdec_v3_drain_t* drain_info = args;
				DLOGI("[VDEC-%d] VDEC_V3_DRAIN_RESULT", pInst->vdec_id);
                if(ioctl(pInst->dec_fd, VDEC_V3_DRAIN_RESULT, drain_info) < 0)
				{
                    DLOGE("[VDEC-%d] ioctl(0x%x) error[%s]!!", pInst->vdec_id, VDEC_V3_DRAIN_RESULT, strerror(errno));
                }
                ret = drain_info->result;
			}
		break;

		case VDEC_V3_CLOSE:
			{
				vdec_v3_close_t* close_info = args;
				DLOGI("[VDEC-%d] VDEC_V3_CLOSE_RESULT", pInst->vdec_id);
                if(ioctl(pInst->dec_fd, VDEC_V3_CLOSE_RESULT, close_info) < 0)
				{
                    DLOGE("[VDEC-%d] ioctl(0x%x) error[%s]!!", pInst->vdec_id, VDEC_V3_CLOSE_RESULT, strerror(errno));
                }
                ret = close_info->result;
			}
			break;

        default:
            DLOGE("[VDEC-%d] invalid cmd:%d, no result case", pInst->vdec_id, cmd);
            break;
    }

    if((ret&0xf000) != 0x0000)
	{ //If there is an invalid return, we skip it because this return means that vpu didn't process current command yet.
        all_retry_cnt--;
        if( all_retry_cnt > 0)
        {
            retry_cnt = 2;
            goto Retry;
        }
        else
        {
            DLOGE("abnormal exception!!");
        }
    }

    if(!success
        || ((ret&0xf000) != 0x0000) /* vpu can not start or finish its processing with unknown reason!! */
    )
    {
        DLOGE("[VDEC-%d] command(0x%x) didn't work properly. maybe hangup(no return(0x%x))!!", pInst->vdec_id, cmd, ret);

        if(ret != VPU_RETCODE_CODEC_EXIT && ret != VPU_RETCODE_MULTI_CODEC_EXIT_TIMEOUT){
//          ioctl(pInst->mgr_fd, VPU_HW_RESET, (void*)NULL);
        }

        return VPU_RETCODE_CODEC_EXIT;
    }

    return ret;
}

static void save_decoded_frame(unsigned char* Y, unsigned char* U, unsigned char *V, int width, int height, vdec_t *pVdec)
{
    FILE *pFs;

    {
        vdec_t *pInst = (vdec_t*) pVdec;
        char fname[256];
        int dumpSize = 0;

        if(pInst == NULL)
            return;

        sprintf(fname, "/run/media/sda1/dumpframe_%dx%d.yuv", width, height);

        pFs = fopen(fname, "ab+");
        if (!pFs) {
            DLOGE("Cannot open '%s'", fname);
            return;
        }

        fwrite( Y, width*height, 1, pFs);
		dumpSize = width * height >> 2;
		fwrite( U, dumpSize, 1, pFs);
		fwrite( V, dumpSize, 1, pFs);
		/* uv interleave mode
        {
            dumpSize = width * height >> 1;
            fwrite( U, dumpSize, 1, pFs);
        }
		*/
        fclose(pFs);
    }
}

#ifdef HEVC_OUT_FRAME_DUMP
static void save_MapConverter_frame(unsigned char* Y, unsigned char* CB, unsigned char *OffY,  unsigned char *OffCB, int width, int height)
{
    FILE *pComY, *pComCB, *pOffY, *pOffCB;

    DLOGW("save_MapConverter_frame: Y[%p] CB[%p] OffY[%p] OffCB[%p] size:%d x %d", Y, CB, OffY, OffCB, width, height);

    pComY = fopen("/data/frameComY.data", "ab+");
    pComCB = fopen("/data/frameComCB.data", "ab+");
    pOffY = fopen("/data/frameComOffY.data", "ab+");
    pOffCB = fopen("/data/frameComOffCB.data", "ab+");

    if (!pComY || !pComCB || !pOffY || !pOffCB) {
        DLOGE("Cannot open '/data folder %p %p %p %p", pComY, pComCB, pOffY, pOffCB);
        return;
    }

    fwrite( Y, width*height, 1, pComY);
    fwrite( CB, width*height, 1, pComCB);
    fwrite( OffY, width*height/4, 1, pOffY);
    fwrite( OffCB, width*height/4, 1, pOffCB);

    fclose(pComY);
    fclose(pComCB);
    fclose(pOffY);
    fclose(pOffCB);
}
#endif//HEVC_OUT_FRAME_DUMP

static unsigned char *vpu_getFrameBufVirtAddr(unsigned char *convert_addr, uint32_t base_index, unsigned long*pVdec)
{
    unsigned char *pBaseAddr;
    unsigned char *pTargetBaseAddr = NULL;
    uint32_t szAddrGap = 0;
    vdec_t * pInst = pVdec;

    if(!pInst){
        DLOGE("vpu_getFrameBufVirtAddr :: Instance is null!!");
        return NULL;
    }

    pTargetBaseAddr = (unsigned char*)pInst->frame_buf_addr[VDEC_VA];

    if (base_index == VDEC_KVA)
    {
        pBaseAddr = (unsigned char*)pInst->frame_buf_addr[VDEC_KVA];
    }
    else /* default : PA */
    {
        pBaseAddr = (unsigned char*)pInst->frame_buf_addr[VDEC_PA];
    }

    szAddrGap = convert_addr - pBaseAddr;

    //DLOGE("Output VirtAddr:%x, szAddrGap:%d, convert_addr:%x, pBaseAddr:%x", (pTargetBaseAddr+szAddrGap), szAddrGap, convert_addr, pBaseAddr);
    return (pTargetBaseAddr+szAddrGap);
}

enum vdec_return vdec_init(vdec_handle_h handle, vdec_init_t* p_init_param)
{
	enum vpu_return_code vpu_ret = VPU_RETCODE_SUCCESS;
	enum vdec_return ret = VDEC_RETURN_SUCCESS;
	int ii;
	vdec_t *pInst = (vdec_t *)handle;

	memset(&pInst->init_info, 0x00, sizeof(vdec_v3_init_t));

	pInst->init_info.input.codec_id = convert_vdec_codec_id(p_init_param->codec_id);
	pInst->init_info.input.output_format = convert_vdec_output_format(p_init_param->output_format);
	pInst->init_info.input.enable_ringbuffer_mode = p_init_param->enable_ringbuffer_mode;

	if(p_init_param->enable_user_data == 1U)
	{
		pInst->init_info.input.enable_user_data = 1U;
	}

	if(p_init_param->enable_dma_buf_id == 1U)
	{
		pInst->init_info.input.enable_dma_buf_id = 1U;
	}

	if(p_init_param->enable_max_framebuffer == 1U)
	{
		pInst->init_info.input.dec_opt_flags |= VDEC_V3_USE_MAX_FRAMEBUFFER;
		DLOGI("set VDEC_USE_MAX_FRAMEBUFFER");
	}

	if(p_init_param->enable_no_buffer_delay == 1U)
	{
		pInst->init_info.input.dec_opt_flags |= VDEC_V3_NO_BUFFER_DELAY;
		DLOGI("set VDEC_NO_BUFFER_DELAY");
	}

	if(p_init_param->enable_avc_field_display == 1U)
	{
		pInst->init_info.input.dec_opt_flags |= VDEC_V3_AVC_FIELD_DISPLAY;
		DLOGI("set VDEC_AVC_FIELD_DISPLAY");
	}

	pInst->init_info.input.max_support_width = p_init_param->max_support_width;
	pInst->init_info.input.max_support_height = p_init_param->max_support_height;
	pInst->init_info.input.additional_frame_count = p_init_param->additional_frame_count;

	DLOGI( "[VDEC-%d] codec id:%d, max w:%d, h:%d, output:%d, addtional frame count:%d, enable ringbuffer mode:%d, forced_pmap:%d, userdata:%d, enable dmabuff:%d",
		pInst->vdec_id, pInst->init_info.input.codec_id, pInst->init_info.input.max_support_width, pInst->init_info.input.max_support_height,
		pInst->init_info.input.output_format, pInst->init_info.input.additional_frame_count, pInst->init_info.input.enable_ringbuffer_mode,
		pInst->init_info.input.use_forced_pmap_idx, pInst->init_info.input.enable_user_data, pInst->init_info.input.enable_dma_buf_id);

	DLOGI( "[VDEC-%d] VDEC_V3_INIT", pInst->vdec_id);
	vpu_ret = vdec_cmd_process(VDEC_V3_INIT, &pInst->init_info, pInst);
	if( vpu_ret != VPU_RETCODE_SUCCESS )
	{
		DLOGE( "[VDEC-%d] VDEC_V3_INIT failed Error code is 0x%x ", pInst->vdec_id, ret );
		ret = VDEC_RETURN_FAIL;
	}
	else
	{
		pInst->bitstream_buf_addr[VDEC_PA] = pInst->init_info.output.next_bitstream_buf_addr[VPU_PA];
		pInst->bitstream_buf_addr[VDEC_KVA] = pInst->init_info.output.next_bitstream_buf_addr[VPU_KVA];
		pInst->bitstream_buf_size = pInst->init_info.output.next_bitstream_buf_size;
		pInst->bitstream_buf_addr[VDEC_VA] = (vpu_addr_t)cdk_sys_malloc_virtual_addr( (uint32_t)pInst->bitstream_buf_addr[VDEC_PA], pInst->bitstream_buf_size, pInst );
		if(pInst->bitstream_buf_addr[VDEC_VA] == NULL)
		{
			DLOGE( "[VDEC-%d] bitstream_buf_addr mmap failed", pInst->vdec_id);
			ret = VDEC_RETURN_FAIL;
		}

		//calculate for end address of bitstream buffer
		pInst->bitstream_buf_addr_end[VDEC_PA] = pInst->bitstream_buf_addr[VDEC_PA] + pInst->bitstream_buf_size;
		pInst->bitstream_buf_addr_end[VDEC_KVA] = pInst->bitstream_buf_addr[VDEC_KVA] + pInst->bitstream_buf_size;
		pInst->bitstream_buf_addr_end[VDEC_VA] = pInst->bitstream_buf_addr[VDEC_VA] + pInst->bitstream_buf_size;

		//set next bitstream buffer
		pInst->bitstream_buf_next[VDEC_PA] = pInst->bitstream_buf_addr[VDEC_PA];
		pInst->bitstream_buf_next[VDEC_KVA] = pInst->bitstream_buf_addr[VDEC_KVA];
		pInst->bitstream_buf_next[VDEC_VA] = pInst->bitstream_buf_addr[VDEC_VA];
		pInst->bitstream_buf_next_size = pInst->bitstream_buf_size;

		pInst->max_bitstream_size = pInst->bitstream_buf_size;

		if(pInst->init_info.input.enable_user_data == 1U)
		{
			pInst->userdata_buf_addr[VDEC_PA] = pInst->init_info.output.userdata_buf_addr[VPU_PA];
			pInst->userdata_buf_addr[VDEC_KVA] = pInst->init_info.output.userdata_buf_addr[VPU_KVA];
			pInst->userdata_buf_size = pInst->init_info.output.userdata_buf_size;
			pInst->userdata_buf_addr[VDEC_VA] = (vpu_addr_t)cdk_sys_malloc_virtual_addr( (uint32_t)pInst->userdata_buf_addr[VDEC_PA], pInst->userdata_buf_size, pInst );
			if(pInst->userdata_buf_addr[VDEC_VA] == NULL)
			{
				DLOGE( "[VDEC-%d] userdata_buf_addr mmap failed", pInst->vdec_id);
				ret = VDEC_RETURN_FAIL;
			}
		}

		DLOGI( "[VDEC-%d] VDEC_V3_INIT, ret:%d", pInst->vdec_id, ret);
		DLOGI( "[VDEC-%d] bitstream_buf pa:0x%x, va:0x%x, size:%d, userdata buf pa:0x%x, va:0x%x, size:%d, ringbuffer enabled:%d",
			pInst->vdec_id,
			pInst->bitstream_buf_addr[VDEC_PA],
			pInst->bitstream_buf_addr[VDEC_VA],
			pInst->bitstream_buf_size,
			pInst->userdata_buf_addr[VDEC_PA],
			pInst->userdata_buf_addr[VDEC_VA],
			pInst->userdata_buf_size,
			pInst->init_info.output.is_ringbuffer_mode
			);

		if(pInst->init_info.output.is_ringbuffer_mode == 1)
		{
			DLOGI( "[VDEC-%d] enabled RINGBUFFER mode", pInst->vdec_id);
			pInst->use_ringbuffer_mode = 1;
		}
		else
		{
			DLOGI( "[VDEC-%d] disabled RINGBUFFER mode, use LINEAR BUFFER(fileplay) mode", pInst->vdec_id);
			pInst->use_ringbuffer_mode = 0;
		}
	}

	return ret;
}

enum vdec_return vdec_seq_header(vdec_handle_h handle, vdec_seqheader_input_t* p_input_param, vdec_seqheader_output_t* p_output_param)
{
	enum vpu_return_code vpu_ret = VPU_RETCODE_SUCCESS;
    enum vdec_return ret = VDEC_RETURN_SUCCESS;

	vdec_t *pInst = (vdec_t *)handle;
	int seq_stream_size = (p_input_param->input_size > pInst->max_bitstream_size) ? pInst->max_bitstream_size : p_input_param->input_size;
	unsigned char* srcbuf = (unsigned char*)p_input_param->input_buffer;

	memset(&pInst->seqenceheader_info, 0x00, sizeof(vdec_v3_seqheader_t));

	DLOGI("[VDEC-%d] VDEC_V3_SEQ_HEADER start, size:%d, 0x%X %X %X %X %X %X %X %X", pInst->vdec_id, seq_stream_size, srcbuf[0], srcbuf[1], srcbuf[2], srcbuf[3], srcbuf[4], srcbuf[5], srcbuf[6], srcbuf[7]);

	pInst->seqenceheader_info.input.bitstream_addr[VPU_PA] = pInst->bitstream_buf_addr[VDEC_PA];
	pInst->seqenceheader_info.input.bitstream_addr[VPU_KVA] = pInst->bitstream_buf_addr[VDEC_KVA];
	memcpy( (unsigned long*)pInst->bitstream_buf_addr[VDEC_VA], (unsigned long*)p_input_param->input_buffer, seq_stream_size);
	pInst->seqenceheader_info.input.bitstream_size = seq_stream_size;

	if(0)
	{
		char* seqdata = (char*)pInst->bitstream_buf_addr[VDEC_VA];
		DLOGE("VDEC_V3_SEQ_HEADER, seq data:%x %x %x %x", seqdata[0], seqdata[1], seqdata[2], seqdata[3]);
	}

	vpu_ret = vdec_cmd_process(VDEC_V3_SEQ_HEADER, &pInst->seqenceheader_info, pInst);
	if( vpu_ret != VPU_RETCODE_SUCCESS )
	{
		DLOGE( "[VDEC-%d] VPU_DEC_SEQ_HEADER failed Error code is 0x%x. ErrorReason is %d", pInst->vdec_id, ret, pInst->seqenceheader_info.output.initial_info.report_error_reason);
		if(vpu_ret == VPU_RETCODE_CODEC_SPECOUT)
			DLOGE("[VDEC-%d] NOT SUPPORTED CODEC. VPU SPEC OUT!!", pInst->vdec_id);

		ret = VDEC_RETURN_FAIL;
	}
	else
	{
		int addr_offset;

		pInst->frame_buf_addr[VDEC_PA] = pInst->seqenceheader_info.output.framebuf_addr[VPU_PA];
		pInst->frame_buf_addr[VDEC_KVA] = pInst->seqenceheader_info.output.framebuf_addr[VPU_KVA];
		pInst->frame_buf_size = pInst->seqenceheader_info.output.framebuf_size;

		DLOGI( "[VDEC-%d] frame_buf_addr[VDEC_PA] = 0x%x, 0x%x , index = %d ", pInst->vdec_id, (vpu_addr_t)pInst->frame_buf_addr[VDEC_PA], pInst->frame_buf_size, pInst->vdec_id );
		pInst->frame_buf_addr[VDEC_VA] = (vpu_addr_t)cdk_sys_malloc_virtual_addr( (uint32_t)pInst->frame_buf_addr[VDEC_PA], pInst->frame_buf_size, pInst );
		if( pInst->frame_buf_addr[VDEC_VA] == 0 )
		{
			DLOGE( "[VDEC-%d] frame_buf_addr[VDEC_VA] malloc() failed ", pInst->vdec_id);
			ret = VDEC_RETURN_FAIL;
		}
		DLOGI("[VDEC-%d] frame_buf_addr[VDEC_VA] = 0x%x, frame_buf_addr[VDEC_KVA] = 0x%x ", pInst->vdec_id, (vpu_addr_t)pInst->frame_buf_addr[VDEC_VA], pInst->frame_buf_addr[VDEC_KVA] );

		//set next bitstream buffer
		addr_offset = pInst->seqenceheader_info.output.next_bitstream_buf_addr[VPU_PA] - pInst->bitstream_buf_addr[VDEC_PA];
		pInst->bitstream_buf_next[VDEC_PA] = pInst->seqenceheader_info.output.next_bitstream_buf_addr[VDEC_PA];
		pInst->bitstream_buf_next[VDEC_KVA] = pInst->seqenceheader_info.output.next_bitstream_buf_addr[VDEC_KVA];
		pInst->bitstream_buf_next[VDEC_VA] = pInst->bitstream_buf_addr[VDEC_VA] + addr_offset;
		pInst->bitstream_buf_next_size = pInst->bitstream_buf_size;
		DLOGI("[VDEC-%d] next bitstream addr:0x%x/0x%x, size:%d", pInst->vdec_id, pInst->bitstream_buf_next[VDEC_PA], pInst->bitstream_buf_next[VDEC_VA], pInst->bitstream_buf_next_size);
	}

	if(ret == VDEC_RETURN_SUCCESS)
	{
		DLOGI("[VDEC-%d] VDEC_V3_SEQ_HEADER OK!", pInst->vdec_id);
		DLOGI("[VDEC-%d] VDEC_V3_SEQ_HEADER, metadata:%d", pInst->vdec_id, pInst->seqenceheader_info.output.initial_info.metadata);
		if(pInst->seqenceheader_info.output.initial_info.metadata == 1)
		{
			vpu_metadata_info_t* meta = &pInst->seqenceheader_info.output.initial_info.metadata_info;
			DLOGI("[VDEC-%d] metadata info, colour_primaries:%u, transfer_characteristics:%u, matrix_coefficients:%u",
				pInst->vdec_id,
				meta->vui_param.colour_primaries,
				meta->vui_param.transfer_characteristics,
				meta->vui_param.matrix_coefficients);
		}

		//passing the output
		p_output_param->pic_width = pInst->seqenceheader_info.output.initial_info.pic_width;
		p_output_param->pic_height = pInst->seqenceheader_info.output.initial_info.pic_height;

		p_output_param->frame_rate_res = pInst->seqenceheader_info.output.initial_info.frame_rate_res;
		p_output_param->frame_rate_div = pInst->seqenceheader_info.output.initial_info.frame_rate_div;

		p_output_param->min_frame_buffer_count = pInst->seqenceheader_info.output.initial_info.min_frame_buffer_count;
		p_output_param->min_frame_buffer_size = pInst->seqenceheader_info.output.initial_info.min_frame_buffer_size;
		p_output_param->frame_buffer_format = pInst->seqenceheader_info.output.initial_info.frame_buffer_format;

		p_output_param->pic_crop.left = pInst->seqenceheader_info.output.initial_info.pic_crop.left;
		p_output_param->pic_crop.right = pInst->seqenceheader_info.output.initial_info.pic_crop.right;
		p_output_param->pic_crop.top = pInst->seqenceheader_info.output.initial_info.pic_crop.top;
		p_output_param->pic_crop.bottom = pInst->seqenceheader_info.output.initial_info.pic_crop.bottom;

		p_output_param->eotf = pInst->seqenceheader_info.output.initial_info.eotf;
		p_output_param->frame_buf_delay = pInst->seqenceheader_info.output.initial_info.frame_buf_delay;
		p_output_param->profile = pInst->seqenceheader_info.output.initial_info.profile;
		p_output_param->level = pInst->seqenceheader_info.output.initial_info.level;
		p_output_param->tier = pInst->seqenceheader_info.output.initial_info.tier;
		p_output_param->interlace = pInst->seqenceheader_info.output.initial_info.interlace;
		p_output_param->aspectratio = pInst->seqenceheader_info.output.initial_info.aspectratio;
		p_output_param->report_error_reason = pInst->seqenceheader_info.output.initial_info.report_error_reason;
		p_output_param->bitdepth = pInst->seqenceheader_info.output.initial_info.bitdepth;

#if 0 //No parameters have been provided yet
		vdec_v3_avc_vui_info_t avc_vui_info;
    	vdec_v3_mpeg2_seq_display_ext_info_t mpeg2_disp_info;

		vdec_v3_mjpeg_specific_info_t mjpg_spec_info;
		unsigned int metadata;
		vpu_metadata_info_t metadata_info;
#endif

	}
	else
	{
		DLOGE("[VDEC-%d] VDEC_V3_SEQ_HEADER Failed, ret:%d", pInst->vdec_id, ret);
	}

    return ret;
}

static void vdec_fill_output(vdec_t *pInst, vdec_v3_decode_out_t* vpu_output, vdec_decode_output_t* vdec_output)
{
	if( pInst->init_info.input.codec_id != VDEC_CODEC_MJPG)
	{
		vdec_output->display_output[VDEC_PA][VDEC_COMP_Y] = vpu_output->display_out[VPU_PA][VPU_COMP_Y];
		vdec_output->display_output[VDEC_PA][VDEC_COMP_U] = vpu_output->display_out[VPU_PA][VPU_COMP_U];
		vdec_output->display_output[VDEC_PA][VDEC_COMP_V] = vpu_output->display_out[VPU_PA][VPU_COMP_V];

		vdec_output->display_output[VDEC_VA][VDEC_COMP_Y] = (unsigned char *)vpu_getFrameBufVirtAddr(vpu_output->display_out[VPU_KVA][VPU_COMP_Y], VDEC_KVA, pInst);
		vdec_output->display_output[VDEC_VA][VDEC_COMP_U] = (unsigned char *)vpu_getFrameBufVirtAddr(vpu_output->display_out[VPU_KVA][VPU_COMP_U], VDEC_KVA, pInst);
		vdec_output->display_output[VDEC_VA][VDEC_COMP_V] = (unsigned char *)vpu_getFrameBufVirtAddr(vpu_output->display_out[VPU_KVA][VPU_COMP_V], VDEC_KVA, pInst);
	}
	else
	{
		vdec_output->display_output[VDEC_PA][VDEC_COMP_Y] = vpu_output->decoded_out[VPU_PA][VPU_COMP_Y];
		vdec_output->display_output[VDEC_PA][VDEC_COMP_U] = vpu_output->decoded_out[VPU_PA][VPU_COMP_U];
		vdec_output->display_output[VDEC_PA][VDEC_COMP_V] = vpu_output->decoded_out[VPU_PA][VPU_COMP_V];

		vdec_output->display_output[VDEC_VA][VDEC_COMP_Y] = (unsigned char *)vpu_getFrameBufVirtAddr(vpu_output->decoded_out[VPU_KVA][VPU_COMP_Y], VDEC_KVA, pInst);
		vdec_output->display_output[VDEC_VA][VDEC_COMP_U] = (unsigned char *)vpu_getFrameBufVirtAddr(vpu_output->decoded_out[VPU_KVA][VPU_COMP_U], VDEC_KVA, pInst);
		vdec_output->display_output[VDEC_VA][VDEC_COMP_V] = (unsigned char *)vpu_getFrameBufVirtAddr(vpu_output->decoded_out[VPU_KVA][VPU_COMP_V], VDEC_KVA, pInst);
	}

	vdec_output->pic_type = vpu_output->out_info.pic_type;
	vdec_output->display_idx = vpu_output->out_info.display_idx;
	vdec_output->decoded_idx = vpu_output->out_info.decoded_idx;
	vdec_output->dma_buf_id = vpu_output->dma_buf_id;

	vdec_output->display_status = convert_vpu_display_status(vpu_output->out_info.display_status);
	vdec_output->decoded_status = convert_vpu_decoded_status(vpu_output->out_info.decoded_status);

	vdec_output->interlaced_frame = vpu_output->out_info.interlaced_frame;
	vdec_output->num_of_err_mbs = vpu_output->out_info.num_of_err_mbs;
	vdec_output->decoded_width = vpu_output->out_info.decoded_width;
	vdec_output->decoded_height = vpu_output->out_info.decoded_height;
	vdec_output->display_width = vpu_output->out_info.display_width;
	vdec_output->display_height = vpu_output->out_info.display_height;
	vdec_output->dma_buf_align_width = vpu_output->out_info.dma_buf_align_width;
	vdec_output->dma_buf_align_height = vpu_output->out_info.dma_buf_align_height;

	vdec_output->decoded_crop = convert_vpu_crop(vpu_output->out_info.decoded_crop);
	vdec_output->display_crop = convert_vpu_crop(vpu_output->out_info.display_crop);

	vdec_output->userdata_buf_addr[VDEC_PA] = vpu_output->out_info.userdata_buf_addr[VPU_PA];
	vdec_output->userdata_buf_addr[VDEC_KVA] = vpu_output->out_info.userdata_buf_addr[VPU_KVA];
	vdec_output->userdata_buffer_size = vpu_output->out_info.userdata_buffer_size;

	vdec_output->picture_structure = vpu_output->out_info.specific_info.picture_structure;
	vdec_output->top_field_first = vpu_output->out_info.specific_info.top_field_first;

	//map converter info
	vdec_output->compressed_output.compressed_y[VDEC_PA] = vpu_output->out_info.disp_map_conv_info.compressed_y[VPU_PA];
	vdec_output->compressed_output.compressed_y[VDEC_KVA] = vpu_output->out_info.disp_map_conv_info.compressed_y[VPU_KVA];

	vdec_output->compressed_output.compressed_cb[VDEC_PA] = vpu_output->out_info.disp_map_conv_info.compressed_cb[VPU_PA];
	vdec_output->compressed_output.compressed_cb[VDEC_KVA] = vpu_output->out_info.disp_map_conv_info.compressed_cb[VPU_KVA];

	vdec_output->compressed_output.fbc_y_offset_addr[VDEC_PA] = vpu_output->out_info.disp_map_conv_info.fbc_y_offset_addr[VPU_PA];
	vdec_output->compressed_output.fbc_y_offset_addr[VDEC_KVA] = vpu_output->out_info.disp_map_conv_info.fbc_y_offset_addr[VPU_KVA];

	vdec_output->compressed_output.fbc_c_offset_addr[VDEC_PA] = vpu_output->out_info.disp_map_conv_info.fbc_c_offset_addr[VPU_PA];
	vdec_output->compressed_output.fbc_c_offset_addr[VDEC_KVA] = vpu_output->out_info.disp_map_conv_info.fbc_c_offset_addr[VPU_KVA];

	vdec_output->compressed_output.compression_table_luma_size = vpu_output->out_info.disp_map_conv_info.compression_table_luma_size;
	vdec_output->compressed_output.compression_table_chroma_size = vpu_output->out_info.disp_map_conv_info.compression_table_chroma_size;

	vdec_output->compressed_output.luma_stride = vpu_output->out_info.disp_map_conv_info.luma_stride;
	vdec_output->compressed_output.chroma_stride = vpu_output->out_info.disp_map_conv_info.chroma_stride;

	vdec_output->compressed_output.luma_bit_depth = vpu_output->out_info.disp_map_conv_info.luma_bit_depth;
	vdec_output->compressed_output.chroma_bit_depth = vpu_output->out_info.disp_map_conv_info.chroma_bit_depth;

	vdec_output->compressed_output.frame_endian = vpu_output->out_info.disp_map_conv_info.frame_endian;

#ifdef VPU_OUT_FRAME_DUMP
	if(vdec_output->display_status == VPU_DISP_STAT_SUCCESS)
	{
		DLOGE("DUMP Displayed addr 0x%x,0x%x,0x%x, %dx%d", vdec_output->display_output[VDEC_VA][VDEC_COMP_Y], vdec_output->display_output[VDEC_VA][VDEC_COMP_U], vdec_output->display_output[VDEC_VA][VDEC_COMP_V], vdec_output->display_width, vdec_output->display_height);
		save_decoded_frame(vdec_output->display_output[VDEC_VA][VDEC_COMP_Y],
							vdec_output->display_output[VDEC_VA][VDEC_COMP_U],
							vdec_output->display_output[VDEC_VA][VDEC_COMP_V],
							vdec_output->display_width, vdec_output->display_height, pInst);

	}
#endif

}

enum vdec_return vdec_decode(vdec_handle_h handle, vdec_decode_input_t* p_input_param, vdec_decode_output_t* p_output_param)
{
	enum vpu_return_code vpu_ret = VPU_RETCODE_SUCCESS;
	enum vdec_return ret = VDEC_RETURN_SUCCESS;
	int addr_offset;
	int stream_size;

	vdec_t *pInst = (vdec_t *)handle;
	unsigned char* srcbuf = (unsigned char*)p_input_param->input_buffer;

	memset(&pInst->decode_info, 0x00, sizeof(vdec_v3_decode_t));

	stream_size = (p_input_param->input_size > pInst->max_bitstream_size) ? pInst->max_bitstream_size : p_input_param->input_size;

	DLOGI("[VDEC-%d] VDEC_V3_DECODE start, stream_size:%d, 0x%X %X %X %X %X %X %X %X %X %X %X %X",
		pInst->vdec_id, stream_size,
		srcbuf[0], srcbuf[1], srcbuf[2], srcbuf[3], srcbuf[4], srcbuf[5], srcbuf[6], srcbuf[7], srcbuf[8], srcbuf[9], srcbuf[10], srcbuf[11]);

	if(p_input_param->skip_mode != VDEC_FRAMESKIP_AUTO)
	{
		if(p_input_param->skip_mode == VDEC_FRAMESKIP_NON_I)
		{
			pInst->decode_info.input.skip_mode = VPU_FRAMESKIP_NON_I;
			DLOGI("[VDEC-%d] ===> SkipMode VPU_FRAMESKIP_NON_I", pInst->vdec_id);
		}
		else if(p_input_param->skip_mode == VDEC_FRAMESKIP_B)
		{
			pInst->decode_info.input.skip_mode = VPU_FRAMESKIP_B;
			DLOGI("[VDEC-%d] ===> SkipMode VPU_FRAMESKIP_B", pInst->vdec_id);
		}
		else
		{
			pInst->decode_info.input.skip_mode = VPU_FRAMESKIP_DISABLED;
			DLOGI("[VDEC-%d] ===> SkipMode VPU_FRAMESKIP_DISABLED", pInst->vdec_id);
		}
	}
	else
	{
		pInst->decode_info.input.skip_mode = VPU_FRAMESKIP_AUTO;
	}

	{
		if(stream_size > pInst->bitstream_buf_next_size)
		{
			DLOGE("[VDEC-%d] write overflow, input_size:%d, buf_next_size:%d", pInst->vdec_id, stream_size, pInst->bitstream_buf_next_size);
			pInst->decode_info.input.bitstream_size = 0;
			ret = VDEC_RETURN_BUFFER_NOT_CONSUMED;
		}
		else
		{
			pInst->decode_info.input.bitstream_addr[VPU_PA] = pInst->bitstream_buf_next[VDEC_PA];
			pInst->decode_info.input.bitstream_addr[VPU_KVA] = pInst->bitstream_buf_next[VDEC_KVA];

			memcpy( (unsigned long*)pInst->bitstream_buf_next[VDEC_VA], (unsigned long*)p_input_param->input_buffer, stream_size);

			pInst->decode_info.input.bitstream_size = stream_size;
		}
	}

	vpu_ret = vdec_cmd_process(VDEC_V3_DECODE, &pInst->decode_info, pInst);

	pInst->total_frm++;
//      if(gsVpuDecInOut_Info.gsVpuDecOutput.m_DecOutInfo.m_display_status != VPU_DISP_STAT_SUCCESS)
//          DLOGD("systemtime:: ## decoded frame but no-output");
//      else
//          DLOGD("systemtime:: ## decoded frame");

	DLOGD("[VDEC-%d] decode ret:%d, pic_type:%d, disp_idx:%d, dec_idx:%d, disp_status:%d, dec_status:%d", pInst->vdec_id, vpu_ret,
								pInst->decode_info.output.out_info.pic_type,
								pInst->decode_info.output.out_info.display_idx, pInst->decode_info.output.out_info.decoded_idx,
								pInst->decode_info.output.out_info.display_status, pInst->decode_info.output.out_info.decoded_status);

	if(vpu_ret != VPU_RETCODE_INFO_INSUFFICIENT_DATA)
	{
		if( (pInst->decode_info.output.out_info.decoded_status == VPU_DEC_STAT_BUF_FULL)
			|| ( (pInst->decode_info.output.out_info.decoded_status != VPU_DEC_STAT_SUCCESS_FIELD_PICTURE)
				&& (pInst->decode_info.output.out_info.display_width <= 64 || pInst->decode_info.output.out_info.display_height <= 64))
			||  (vpu_ret == VPU_RETCODE_CODEC_EXIT)
		)
		{
			if(pInst->decode_info.output.out_info.decoded_status == VPU_DEC_STAT_BUF_FULL)
			{
				DLOGE("[VDEC-%d] Buffer full", pInst->vdec_id);
			}
			else if (vpu_ret == VPU_RETCODE_CODEC_EXIT)
			{
				DLOGE("[VDEC-%d] Codec Exit", pInst->vdec_id);
				ret = VDEC_RETURN_FAIL;
			}
			else
			{
				DLOGD("[VDEC-%d] Strange resolution", pInst->vdec_id);
			}

			DLOGD("[VDEC-%d] Dec In 0x%x - 0x%x, %d", pInst->vdec_id,
								pInst->decode_info.input.bitstream_addr[VDEC_PA], pInst->decode_info.input.bitstream_addr[VDEC_VA], pInst->decode_info.input.bitstream_size);

			DLOGD("[VDEC-%d] %d - %d - %d, %d - %d - %d", pInst->vdec_id, pInst->decode_info.output.out_info.display_width,
								pInst->decode_info.output.out_info.display_crop.left, pInst->decode_info.output.out_info.display_crop.right,
								pInst->decode_info.output.out_info.display_height,
								pInst->decode_info.output.out_info.display_crop.top, pInst->decode_info.output.out_info.display_crop.bottom);
		}
	}

	//set next bitstream buffer
	addr_offset = pInst->decode_info.output.next_bitstream_buf_addr[VPU_PA] - pInst->bitstream_buf_addr[VDEC_PA];
	pInst->bitstream_buf_next[VDEC_PA] = pInst->decode_info.output.next_bitstream_buf_addr[VPU_PA];
	pInst->bitstream_buf_next[VDEC_KVA] = pInst->decode_info.output.next_bitstream_buf_addr[VPU_KVA];
	pInst->bitstream_buf_next[VDEC_VA] = pInst->bitstream_buf_addr[VDEC_VA] + addr_offset;
	pInst->bitstream_buf_next_size = pInst->decode_info.output.next_bitstream_buf_size;
	DLOGD("[VDEC-%d] next bitstream addr:0x%x/0x%x, size:%d, offset:0x%x", pInst->vdec_id, pInst->bitstream_buf_next[VDEC_PA], pInst->bitstream_buf_next[VDEC_VA], pInst->bitstream_buf_next_size, addr_offset);

	//DLOGE("[VDEC-%d] input addr:0x%x, next addr:0x%x ", pInst->vdec_id, pInst->decode_info.input.bitstream_addr[VPU_PA], pInst->decode_info.output.next_bitstream_buf_addr[VPU_PA]);
	//passing the output
	vdec_fill_output(pInst, &pInst->decode_info.output, p_output_param);

	if( vpu_ret == VPU_RETCODE_INFO_INSUFFICIENT_DATA )
	{
		DLOGE( "[VDEC-%d] VPU_DEC_DECODE failed 0x%x, Need more data", pInst->vdec_id, vpu_ret);
		ret = VDEC_RETURN_NEED_MORE_DATA;
	}
	else if( vpu_ret == VPU_RETCODE_FRAME_NOT_COMPLETE )
	{
		DLOGE( "[VDEC-%d] VPU_DEC_DECODE is not completed, push next frame", pInst->vdec_id, vpu_ret);
		ret = VDEC_RETURN_NEED_MORE_DATA;
	}
	else if( vpu_ret == VPU_RETCODE_CODEC_FINISH )
	{
		DLOGE( "[VDEC-%d] VPU_DEC_DECODE failed Error code is 0x%x ", pInst->vdec_id, vpu_ret );
		ret = VDEC_RETURN_CODEC_FINISH;
	}
	else if( vpu_ret != VPU_RETCODE_SUCCESS )
	{
		DLOGE( "[VDEC-%d] VPU_DEC_DECODE failed Error code is 0x%x ", pInst->vdec_id, vpu_ret );
		ret = VDEC_RETURN_FAIL;
	}
	else
	{

		if(p_output_param->picture_structure == 1)
		{
			int top_field_type = 0;
			int bottom_field_type = 0;

			top_field_type = ((unsigned int)p_output_param->pic_type >> 3U) & 0x07U;
			bottom_field_type = (unsigned int)p_output_param->pic_type & 0x07U;
			DLOGD("[VDEC-%d] Dec ret:%d Interlace PicType[%s(%d),%s(%d)], OutIdx[%d/%d], OutStatus[%d/%d] \n", pInst->vdec_id, ret,
								get_pic_type_name(top_field_type), top_field_type, get_pic_type_name(bottom_field_type), bottom_field_type,
								p_output_param->display_idx,p_output_param->decoded_idx,
								p_output_param->display_status, p_output_param->decoded_status);
		}
		else
		{
			DLOGD("[VDEC-%d] Dec ret:%d Progressive PicType[%s(%d)], OutIdx[%d/%d], OutStatus[%d/%d] \n", pInst->vdec_id, ret,
								get_pic_type_name(p_output_param->pic_type), p_output_param->pic_type,
								p_output_param->display_idx,p_output_param->decoded_idx,
								p_output_param->display_status, p_output_param->decoded_status);
		}

		DLOGI("[VDEC-%d] VDEC_V3_DECODE OK, ret:%d", pInst->vdec_id, ret);
	}

	return ret;
}

enum vdec_return vdec_buf_clear(vdec_handle_h handle, vdec_buf_clear_t* p_input_param)
{
	enum vpu_return_code vpu_ret = VPU_RETCODE_SUCCESS;
	enum vdec_return ret = VDEC_RETURN_SUCCESS;
	vdec_t *pInst = (vdec_t *)handle;

	if(pInst->init_info.input.codec_id != VCODEC_ID_MJPG)
	{
		DLOGD("[VDEC-%d] cleared idx = %d, dmf_buf_id:%d", pInst->vdec_id, p_input_param->display_index, p_input_param->dma_buf_id);

		memset(&pInst->bufclear_info, 0x00, sizeof(vdec_v3_buf_clear_t));

		pInst->bufclear_info.index = p_input_param->display_index;
		pInst->bufclear_info.dma_buf_id = p_input_param->dma_buf_id;

		vpu_ret = vdec_cmd_process(VDEC_V3_BUF_CLEAR, &pInst->bufclear_info, pInst);
		if( vpu_ret != VPU_RETCODE_SUCCESS )
		{
			DLOGE( "[VDEC-%d] VDEC_V3_BUF_CLEAR failed Error code is 0x%x ", pInst->vdec_id, ret );
			ret = -1;
		}
	}

	return ret;
}

enum vdec_return vdec_drain(vdec_handle_h handle, vdec_decode_output_t* p_output_param)
{
	enum vpu_return_code vpu_ret = VPU_RETCODE_SUCCESS;
	enum vdec_return ret = VDEC_RETURN_SUCCESS;
	vdec_t *pInst = (vdec_t *)handle;

	memset(&pInst->drain_info, 0x00, sizeof(vdec_v3_drain_t));
	vpu_ret = vdec_cmd_process(VDEC_V3_DRAIN, &pInst->drain_info, pInst);
	if( vpu_ret != VPU_RETCODE_SUCCESS && ret != VPU_RETCODE_CODEC_FINISH)
	{
		DLOGE( "[VDEC-%d] VDEC_V3_DRAIN failed Error code is 0x%x ", pInst->vdec_id, ret);
		ret = VDEC_RETURN_FAIL;
	}
	else if( vpu_ret != VPU_RETCODE_SUCCESS )
	{
		DLOGE( "[VDEC-%d] VDEC_V3_DRAIN failed Error code is 0x%x ", pInst->vdec_id, ret );
		return VDEC_RETURN_FAIL;
	}
	else
	{
		if(vpu_ret != VPU_RETCODE_CODEC_FINISH)
		{
			pInst->total_frm++;

			if( (pInst->decode_info.output.out_info.decoded_status == VPU_DEC_STAT_BUF_FULL)
				|| ( (pInst->decode_info.output.out_info.decoded_status != VPU_DEC_STAT_SUCCESS_FIELD_PICTURE)
					&& (pInst->decode_info.output.out_info.display_width <= 64 || pInst->decode_info.output.out_info.display_height <= 64))
				||  (vpu_ret == VPU_RETCODE_CODEC_EXIT)
			)
			{
				if(pInst->decode_info.output.out_info.decoded_status == VPU_DEC_STAT_BUF_FULL)
				{
					DLOGE("Buffer full");
				}
				else if (vpu_ret == VPU_RETCODE_CODEC_EXIT)
				{
					DLOGE("Codec Exit");
				}
				else
				{
					DLOGE("Strange resolution");
				}

				DLOGE("Dec In 0x%x - 0x%x, %d\n",
									pInst->decode_info.input.bitstream_addr[VDEC_PA], pInst->decode_info.input.bitstream_addr[VDEC_VA], pInst->decode_info.input.bitstream_size);

				DLOGE("%d - %d - %d, %d - %d - %d \n", pInst->decode_info.output.out_info.display_width,
									pInst->decode_info.output.out_info.display_crop.left, pInst->decode_info.output.out_info.display_crop.right,
									pInst->decode_info.output.out_info.display_height,
									pInst->decode_info.output.out_info.display_crop.top, pInst->decode_info.output.out_info.display_crop.bottom);

				DLOGE("@@ Dec Out[%d] !! PicType[%d], OutIdx[%d/%d], OutStatus[%d/%d] \n", ret, pInst->decode_info.output.out_info.pic_type,
									pInst->decode_info.output.out_info.display_idx, pInst->decode_info.output.out_info.decoded_idx,
									pInst->decode_info.output.out_info.display_status, pInst->decode_info.output.out_info.decoded_status);
				DLOGE("DispOutIdx : %d \n", pInst->decode_info.output.out_info.display_idx);
			}

			if( pInst->decode_info.output.out_info.pic_type == VPU_PICTURE_I)
			{
				DLOGD( "[VDEC-%d] I-Frame (%d)", pInst->vdec_id, pInst->total_frm);
			}

			//passing the output
			vdec_fill_output(pInst, &pInst->decode_info.output, p_output_param);
		}
		else
		{
			ret = VDEC_RETURN_CODEC_FINISH;
		}
	}

	DLOGI("[VDEC-%d] VDEC_V3_DRAIN OK, ret:%d", pInst->vdec_id, ret);
	return ret;
}

enum vdec_return vdec_flush(vdec_handle_h handle)
{
	enum vpu_return_code vpu_ret = VPU_RETCODE_SUCCESS;
	enum vdec_return ret = VDEC_RETURN_SUCCESS;
	vdec_t *pInst = (vdec_t *)handle;

	memset(&pInst->flush_info, 0x00, sizeof(vdec_v3_flush_t));
	vpu_ret = vdec_cmd_process(VDEC_V3_FLUSH, &pInst->flush_info, pInst);
	if( (vpu_ret != VPU_RETCODE_SUCCESS) && (vpu_ret != VPU_RETCODE_CODEC_FINISH) )
	{
		DLOGE( "[VDEC-%d] VDEC_V3_FLUSH failed Error code is 0x%x ", pInst->vdec_id, ret);
		ret = -1;
	}

	return ret;
}

enum vdec_return vdec_close(vdec_handle_h handle)
{
	enum vpu_return_code vpu_ret = VPU_RETCODE_SUCCESS;
	enum vdec_return ret = VDEC_RETURN_SUCCESS;
	vdec_t *pInst = (vdec_t *)handle;

	memset(&pInst->close_info, 0x00, sizeof(vdec_v3_close_t));

	vpu_ret = vdec_cmd_process(VDEC_V3_CLOSE, &pInst->close_info, pInst);
	if( vpu_ret != VPU_RETCODE_SUCCESS )
	{
		DLOGE( "[VDEC-%d] VPU_DEC_CLOSE failed Error code is 0x%x ", pInst->vdec_id, ret );
		ret = VDEC_RETURN_FAIL;
	}

	if( pInst->bitstream_buf_addr[VDEC_VA] )
	{
		if(cdk_sys_free_virtual_addr( (unsigned long*)pInst->bitstream_buf_addr[VDEC_VA], pInst->bitstream_buf_size)  >= 0)
		{
			pInst->bitstream_buf_addr[VDEC_VA] = 0;
		}
	}

	if( pInst->userdata_buf_addr[VDEC_VA] )
	{
		if(cdk_sys_free_virtual_addr( (unsigned long*)pInst->userdata_buf_addr[VDEC_VA], pInst->userdata_buf_size )  >= 0)
		{
			pInst->userdata_buf_addr[VDEC_VA] = 0;
		}

	}

	if( pInst->frame_buf_addr[VDEC_VA] )
	{
		if(cdk_sys_free_virtual_addr( (unsigned long*)pInst->frame_buf_addr[VDEC_VA], pInst->frame_buf_size )  >= 0)
		{
			pInst->frame_buf_addr[VDEC_VA] = 0;
		}
	}

	return ret;
}

vdec_handle_h vdec_alloc_instance(vdec_info_t* p_info)
{
    vdec_t *pInst = NULL;
    int fd;
	int nInstance;
	int ret = 0;

    pInst = (vdec_t*)malloc(sizeof(vdec_t));
    if(pInst != NULL)
    {
        memset(pInst, 0x00, sizeof(vdec_t));
        pInst->dec_fd = -1;

		pthread_mutex_lock(&instance_mutex);

		DLOGI("open: %s \n",VPU_DECODER_DEVICE);
        pInst->dec_fd = open(VPU_DECODER_DEVICE, O_RDWR);
        if (pInst->dec_fd < 0)
        {
            DLOGE("%s open error[%s]", VPU_DECODER_DEVICE, strerror(errno));
            ret = -1;
        }

		if((ret == 0) && (p_info != NULL))
        {
			vpu_drv_version_t vpu_version;
			vpu_capability_t vpu_capability;

			memset(p_info, 0x00, sizeof(vdec_info_t));
			memset(&vpu_version, 0x00, sizeof(vpu_drv_version_t));
			ret = ioctl(pInst->dec_fd, VPU_V3_GET_DRV_VERSION_SYNC, &vpu_version);
			if(ret == 0)
			{
				DLOGI( "VPU Driver Ver %d.%d.%d", vpu_version.major, vpu_version.minor, vpu_version.revision);
			}
			else
			{
				DLOGE("VPU_V3_GET_DRV_VERSION_SYNC error, ret:%d", ret);
			}

			memset(&vpu_capability, 0x00, sizeof(vpu_capability_t));
			ret = ioctl(pInst->dec_fd, VPU_V3_GET_CAPABILITY_SYNC, &vpu_capability);
			if(ret == 0)
			{
				int ii;

				p_info->num_of_codec_cap = vpu_capability.num_of_codec_cap;

				for(ii=0; ii<vpu_capability.num_of_codec_cap; ii++)
				{
					vpu_codec_cap_t* pcap = &vpu_capability.codec_caps[ii];

					if(pcap->support_codec != NULL)
					{
						vdec_codec_cap_t* pDecCaps = &p_info->decoder_caps[ii];

						memset(pDecCaps, 0x00, sizeof(vdec_codec_cap_t));
						pDecCaps->codec_id = convert_vpu_codec_id(pcap->codec_id);
						pDecCaps->max_fps = pcap->max_fps;
						pDecCaps->max_width = pcap->max_width;
						pDecCaps->max_height = pcap->max_height;

						strcpy(pDecCaps->support_codec, pcap->support_codec);

						if(pcap->support_profile != NULL)
						{
							strcpy(pDecCaps->support_profile, pcap->support_profile);
						}

						if(pcap->support_level != NULL)
						{
							strcpy(pDecCaps->support_level, pcap->support_level);
						}

						if((pcap->support_buffer_mode & VPU_BS_MODE_RINGBUFFER) != 0)
						{
							pDecCaps->support_buffer_mode |= VDEC_BS_MODE_RINGBUFFER;
						}

						if((pcap->support_buffer_mode & VPU_BS_MODE_LINEARBUFFR) != 0)
						{
							pDecCaps->support_buffer_mode |= VDEC_BS_MODE_LINEARBUFFR;
						}

						DLOGI("\t%s %s %s %ux%u@%u, mode:0x%x", pDecCaps->support_codec,
							(pDecCaps->support_profile != NULL) ? pDecCaps->support_profile : " ",
							(pDecCaps->support_level != NULL) ? pDecCaps->support_level : " ",
							pDecCaps->max_width, pDecCaps->max_height, pDecCaps->max_fps,
							(unsigned int)(pDecCaps->support_buffer_mode));
					}
				}
			}

			p_info->decoder_id = vpu_capability.drv_id;

			p_info->ver_major = vpu_version.major;
			p_info->ver_minor = vpu_version.minor;
			p_info->ver_revision = vpu_version.revision;

			pInst->vdec_id = 0;
			pInst->vdec_id = p_info->decoder_id;

			DLOGI("alloc Instance[%d] = %s\n", pInst->vdec_id, VPU_DECODER_DEVICE);
			total_opened_decoder++;
			DLOGI("[%d] venc_alloc_instance, opend:%d", pInst->vdec_id, total_opened_decoder);
		}
		else
		{
			DLOGE("%s open error[%s]", VPU_DECODER_DEVICE, strerror(errno));
			free(pInst);
			pInst = NULL;
		}

		pthread_mutex_unlock(&instance_mutex);
	}

	return (vdec_handle_h)pInst;
}

void vdec_release_instance(vdec_handle_h handle)
{
	vdec_t *pInst = (vdec_t *)handle;

    if(pInst)
    {
		pthread_mutex_lock(&instance_mutex);

        if(pInst->dec_fd)
        {
            if(close(pInst->dec_fd) < 0)
            {
                DLOGE("%s close error", VPU_DECODER_DEVICE);
            }
            pInst->dec_fd = -1;
        }

		free(pInst);
        pInst = NULL;

        if (total_opened_decoder > 0)
        {
            total_opened_decoder--;
        }

		DLOGI("[VDEC] vdec_release_instance total, total_opened_decoder:%d", total_opened_decoder);
		pthread_mutex_unlock(&instance_mutex);
    }
}

int vdec_set_log_level(enum vdec_log_level log_level)
{
	vdec_log_level = log_level;
	DLOGI("set log level:%d", vdec_log_level);
	return vdec_log_level;
}