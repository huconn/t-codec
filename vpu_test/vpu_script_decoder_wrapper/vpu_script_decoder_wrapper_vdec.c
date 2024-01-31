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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "vpu_script_decoder_wrapper.h"
#include "vdec_v3.h"

//logs
#include <time.h>
#define TC_RESET		"\033""[39m"
#define DBG(fmt, args...) \
		do { \
			struct timespec _now;    \
			struct tm _tm;  \
			clock_gettime(CLOCK_REALTIME, &_now);   \
			localtime_r(&_now.tv_sec, &_tm);     \
			printf("[%04d-%02d-%02d %02d:%02d:%02d.%03ld][%-20s %5d]" fmt "\n" TC_RESET, \
				_tm.tm_year + 1900, _tm.tm_mon+1, _tm.tm_mday, _tm.tm_hour, _tm.tm_min, _tm.tm_sec, _now.tv_nsec/1000000, \
				__FUNCTION__, __LINE__, ## args); \
		}while(0)

#define DLOGD(...) 		//DBG(__VA_ARGS__)	//debug
#define DLOGI(...) 		//DBG(__VA_ARGS__)	//info
#define DLOGE(...) 		DBG(__VA_ARGS__)	//error

typedef int (*fp_set_param)(char* str_token, long* handle, void* param);

typedef union ConvHandle {
	vdec_handle_h handle;
	long address;
} ConvHandle;

typedef union ConvBuffer {
	unsigned char* buffer;
	long buffer_address;
} ConvBuffer;

typedef struct VideoDecoderStringCodecMap_t
{
	const char* strCodec;
	enum vdec_codec_id codec_id;
}VideoDecoderStringCodecMap_t;

static VideoDecoderStringCodecMap_t videodecoder_str_codec_map[] =
{
	{"h264", VDEC_CODEC_AVC},
	{"vc1", VDEC_CODEC_VC1},
	{"mpeg2", VDEC_CODEC_MPEG2},
	{"mpeg4", VDEC_CODEC_MPEG4},
	{"h263", VDEC_CODEC_H263},
	{"divx", VDEC_CODEC_DIVX},
	{"avs", VDEC_CODEC_AVS},
	{"mjpeg", VDEC_CODEC_MJPG},
	{"vp8", VDEC_CODEC_VP8},
	{"mvc", VDEC_CODEC_MVC},
	{"h265", VDEC_CODEC_HEVC},
	{"vp9", VDEC_CODEC_VP9},
};

static enum vdec_codec_id convert_str_to_codec(char* strCodec)
{
	unsigned int codec_idx = 0U;
	enum vdec_codec_id ret_codec = VDEC_CODEC_NONE;

	for(codec_idx=0U; codec_idx < (sizeof(videodecoder_str_codec_map) / sizeof(videodecoder_str_codec_map[0])); codec_idx++)
	{
		if(strcmp(strCodec, videodecoder_str_codec_map[codec_idx].strCodec) == 0)
		{
			ret_codec = videodecoder_str_codec_map[codec_idx].codec_id;
		}
	}

	return ret_codec;
}

int vdec_get_codec_id(const char* strCodec)
{
	return (int)convert_str_to_codec((char*)strCodec);
}

static int vdec_command_tokenize(const char* str_parameter, const unsigned int param_size, long* handle, void* param, fp_set_param cb_set_param)
{
	int ret = 0;
	char* savedptr = NULL;
	char* token = NULL;

	//DLOGI("input size->%u, param->%s", param_size, str_parameter);

	//parsing parameters
	savedptr = strdup(str_parameter);
	token = strtok(savedptr, ",");
	while (token != NULL)
	{
		int is_found = 0;
		//DLOGI("next token->%s", token);

		is_found = cb_set_param(token, handle, param);
		if(is_found < 0)
		{
			DLOGE("unknown parameter:%s", token);
		}

		token = strtok(NULL, ",");
	};

	if(savedptr != NULL)
	{
		free(savedptr);
		savedptr = NULL;
	}

	return ret;
}

int alloc_fill_output(int vdec_ret, void* handle, void* param, void* output_param, unsigned int* output_size)
{
	int approx_size = 0;
	unsigned int ii = 0U;

	for(ii=0U; ii < (sizeof(vdec_alloc_output_param)/sizeof(vdec_alloc_output_param[0])); ii++)
	{
		approx_size += strlen(vdec_alloc_output_param[ii]) + 1; //approx value size to minimum 1
	}

	if((output_size != NULL) && (*output_size > approx_size))
	{
		snprintf(output_param, *output_size, "return:%d,handle:0x%lx", vdec_ret, handle);
		*output_size = strlen(output_param);
	}

	return 0;
}

int vdec_command_alloc(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size)
{
	int ret = 0;

	vdec_info_t vdec_info;
	vdec_handle_h handle;

	char str_env_log_level[] = "ENV_VDEC_TEST_LOG_LEVEL";
	char* strLogLevel = getenv(str_env_log_level);

	if(strLogLevel != NULL)
	{
		unsigned int loglevel = 0U;
		loglevel = atoi(strLogLevel);
		DLOGI("[%s:%d] dec log level:%d, strLogLevel:%s\n", __FUNCTION__, __LINE__, loglevel, strLogLevel);
		vdec_set_log_level(loglevel);
	}

	handle = vdec_alloc_instance(&vdec_info);

	DLOGI("handle: %p, 0x%lx", handle, handle);
	alloc_fill_output(ret, (void*)handle, NULL, output_param, output_size);
	DLOGI("output size->%d, string->%s", *output_size, output_param);

	return ret;
}

static int release_fill_input(char* str_token, long* handle, void* param)
{
	int ret = -1;
	const char* each_param = NULL;
	unsigned int param_size = 0;
	unsigned int param_idx = 0U;

	//DLOGI("token->%s", str_token);

	for(param_idx=0U; param_idx < sizeof(vdec_release_input_param) / sizeof(vdec_release_input_param[0]); param_idx++)
	{
		each_param = vdec_release_input_param[param_idx];
		param_size = strlen(each_param);

		if (strncmp(str_token, each_param, param_size) == 0)
		{
			char* strValue = str_token + param_size + 1;
			DLOGI("%s:%s", each_param, strValue);

			if(strcmp(each_param, "handle") == 0)
			{
				*handle = strtol(strValue, NULL, 16);
				DLOGI("set video handle:0x%lx, value:%s", *handle, strValue);
			}
			else
			{
				DLOGE("unknown parameter:%s", each_param);
			}

			ret = 0;
		}
	}

	if(ret == -1)
	{
		DLOGE("unknown parameter:%s", str_token);
	}

	return ret;
}

int release_fill_output(int vdec_ret, void* param, void* output_param, unsigned int* output_size)
{
	int approx_size = 0;
	unsigned int ii = 0U;

	for(ii=0U; ii < (sizeof(vdec_release_output_param)/sizeof(vdec_release_output_param[0])); ii++)
	{
		approx_size += strlen(vdec_release_output_param[ii]) + 1; //approx value size to minimum 1
	}

	if((output_size != NULL) && (*output_size > approx_size))
	{
		snprintf(output_param, *output_size, "return:%d", vdec_ret);

		*output_size = strlen(output_param);
	}

	return 0;
}

int vdec_command_release(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size)
{
	int ret = 0;
	ConvHandle conv_handle;

	conv_handle.handle = NULL;
	vdec_command_tokenize(input_param, input_size, &conv_handle.address, NULL, release_fill_input);

	vdec_release_instance(conv_handle.handle);

	release_fill_output(ret, NULL, output_param, output_size);
	return ret;
}

static int init_fill_input(char* str_token, long* handle, void* param)
{
	int ret = -1;
	const char* each_param = NULL;
	unsigned int param_size = 0;
	unsigned int param_idx = 0U;

	vdec_init_t* init_info = (vdec_init_t*)param;

	//DLOGI("token->%s", str_token);

	for(param_idx=0U; param_idx < sizeof(vdec_init_input_param) / sizeof(vdec_init_input_param[0]); param_idx++)
	{
		each_param = vdec_init_input_param[param_idx];
		param_size = strlen(each_param);

		if (strncmp(str_token, each_param, param_size) == 0)
		{
			char* strValue = str_token + param_size + 1;
			DLOGI("%s:%s", each_param, strValue);

			if(strcmp(each_param, "handle") == 0)
			{
				*handle = strtol(strValue, NULL, 16);
				DLOGI("set video handle:0x%lx, value:%s", *handle, strValue);
			}
			else if(strcmp(each_param, "codec_id") == 0)
			{
				init_info->codec_id = convert_str_to_codec(strValue);
				DLOGI("set video codec_id:%d, value:%s", init_info->codec_id, strValue);
			}
			else if(strcmp(each_param, "max_support_width") == 0)
			{
				init_info->max_support_width = atoi(strValue);
				DLOGI("set max resolution width:%d, value:%s", init_info->max_support_width, strValue);
			}
			else if(strcmp(each_param, "max_support_height") == 0)
			{
				init_info->max_support_height = atoi(strValue);
				DLOGI("set max resolution height:%d, value:%s", init_info->max_support_height, strValue);
			}
			else if(strcmp(each_param, "output_format") == 0)
			{
				if(strcmp(strValue, "yuv420") == 0)
				{
					init_info->output_format = VDEC_OUTPUT_LINEAR_YUV420;
					DLOGI("set compressmode:%d(yuv420), value:%s", init_info->output_format, strValue);
				}
				else if(strcmp(strValue, "nv12") == 0)
				{
					//DLOGE("not support nv12");
					init_info->output_format = VDEC_OUTPUT_LINEAR_NV12;
					DLOGI("set compressmode:%d(nv12), value:%s", init_info->output_format, strValue);
				}
				else if(strcmp(strValue, "mapconv") == 0)
				{
					init_info->output_format = VDEC_OUTPUT_COMPRESSED_MAPCONV;
					DLOGI("set compressmode:%d(map converter), value:%s", init_info->output_format, strValue);
				}
				else if(strcmp(strValue, "afbc") == 0)
				{
					DLOGE("not support afbc");
					ret = -1;
				}
			}
			else if(strcmp(each_param, "additional_frame_count") == 0)
			{
				init_info->additional_frame_count = atoi(strValue);
				DLOGI("set num of surface frame:%d, value:%s", init_info->additional_frame_count, strValue);
			}
			else if(strcmp(each_param, "use_forced_pmap_idx") == 0)
			{
				init_info->use_forced_pmap_idx = atoi(strValue);
				DLOGI("set parameter for use_forced_pmap_idx:%d, value:%s", init_info->use_forced_pmap_idx, strValue);
			}
			else if(strcmp(each_param, "forced_pmap_idx") == 0)
			{
				init_info->forced_pmap_idx = atoi(strValue);
				DLOGI("set parameter for forced_pmap_idx:%d, value:%s", init_info->forced_pmap_idx, strValue);
			}
			else if(strcmp(each_param, "enable_user_data") == 0)
			{
				init_info->enable_user_data = atoi(strValue);
				DLOGI("set enable_user_data:%d, value:%s", init_info->enable_user_data, strValue);
			}
			else if(strcmp(each_param, "enable_user_framebuffer") == 0)
			{
				init_info->enable_user_framebuffer = atoi(strValue);
				DLOGI("set parameter for enable_user_framebuffer:%d, value:%s", init_info->enable_user_framebuffer, strValue);
			}
			else if(strcmp(each_param, "enable_max_framebuffer") == 0)
			{
				init_info->enable_max_framebuffer = atoi(strValue);
				DLOGI("set parameter for enable_max_framebuffer:%d, value:%s", init_info->enable_max_framebuffer, strValue);
			}
			else if(strcmp(each_param, "enable_user_data") == 0)
			{
				init_info->enable_user_data = atoi(strValue);
				DLOGI("set parameter for enable_user_data:%d, value:%s", init_info->enable_user_data, strValue);
			}
			else if(strcmp(each_param, "enable_ringbuffer_mode") == 0)
			{
				init_info->enable_ringbuffer_mode = atoi(strValue);
				DLOGI("set parameter for enable_ringbuffer_mode:%d, value:%s", init_info->enable_ringbuffer_mode, strValue);
			}
			else if(strcmp(each_param, "enable_no_buffer_delay") == 0)
			{
				init_info->enable_no_buffer_delay = atoi(strValue);
				DLOGI("set parameter for enable_no_buffer_delay:%d, value:%s", init_info->enable_no_buffer_delay, strValue);
			}
			else if(strcmp(each_param, "enable_avc_field_display") == 0)
			{
				init_info->enable_avc_field_display = atoi(strValue);
				DLOGI("set parameter for enable_avc_field_display:%d, value:%s", init_info->enable_avc_field_display, strValue);
			}
			else if(strcmp(each_param, "enable_dma_buf_id") == 0)
			{
				init_info->enable_dma_buf_id = atoi(strValue);
				DLOGI("set parameter for enable_dma_buf_id:%d, value:%s", init_info->enable_dma_buf_id, strValue);
			}
			else
			{
				DLOGE("unknown parameter:%s", each_param);
			}

			ret = 0;
		}
	}

	if(ret == -1)
	{
		DLOGE("unknown parameter:%s", str_token);
	}

	return ret;
}

int init_fill_output(int vdec_ret, void* param, void* output_param, unsigned int* output_size)
{
	int approx_size = 0;
	unsigned int ii = 0U;

	for(ii=0U; ii < (sizeof(vdec_init_output_param)/sizeof(vdec_init_output_param[0])); ii++)
	{
		approx_size += strlen(vdec_init_output_param[ii]) + 1; //approx value size to minimum 1
	}

	if((output_size != NULL) && (*output_size > approx_size))
	{
		snprintf(output_param, *output_size, "return:%d", vdec_ret);

		*output_size = strlen(output_param);
	}

	return 0;
}

int vdec_command_init(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size)
{
	int ret = 0;
	vdec_init_t init_info;
	ConvHandle conv_handle;

	memset(&init_info, 0x00, sizeof(vdec_init_t));
	conv_handle.handle = NULL;
	vdec_command_tokenize(input_param, input_size, &conv_handle.address, &init_info, init_fill_input);

	//video init
	DLOGI("handle:0x%lx, codec:%d, num frame:%d, max width:%d, height:%d", conv_handle.handle, init_info.codec_id, init_info.additional_frame_count, init_info.max_support_width, init_info.max_support_height);
	ret = vdec_init(conv_handle.handle, &init_info);

	init_fill_output(ret, NULL, output_param, output_size);

	return ret;
}

static int close_fill_input(char* str_token, long* handle, void* param)
{
	int ret = -1;
	const char* each_param = NULL;
	unsigned int param_size = 0;
	unsigned int param_idx = 0U;

	//DLOGI("token->%s", str_token);

	for(param_idx=0U; param_idx < sizeof(vdec_close_input_param) / sizeof(vdec_close_input_param[0]); param_idx++)
	{
		each_param = vdec_close_input_param[param_idx];
		param_size = strlen(each_param);

		if (strncmp(str_token, each_param, param_size) == 0)
		{
			char* strValue = str_token + param_size + 1;
			DLOGI("%s:%s", each_param, strValue);

			if(strcmp(each_param, "handle") == 0)
			{
				*handle = strtol(strValue, NULL, 16);
				DLOGI("set video handle:0x%lx, value:%s", *handle, strValue);
			}
			else
			{
				DLOGE("unknown parameter:%s", each_param);
			}

			ret = 0;
		}
	}

	if(ret == -1)
	{
		DLOGE("unknown parameter:%s", str_token);
	}

	return ret;
}

int close_fill_output(int vdec_ret, void* param, void* output_param, unsigned int* output_size)
{
	int approx_size = 0;
	unsigned int ii = 0U;

	for(ii=0U; ii < (sizeof(vdec_close_output_param)/sizeof(vdec_close_output_param[0])); ii++)
	{
		approx_size += strlen(vdec_close_output_param[ii]) + 1; //approx value size to minimum 1
	}

	if((output_size != NULL) && (*output_size > approx_size))
	{
		snprintf(output_param, *output_size, "return:%d", vdec_ret);

		*output_size = strlen(output_param);
	}

	return 0;
}

int vdec_command_close(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size)
{
	int ret = 0;
	ConvHandle conv_handle;

	conv_handle.handle = NULL;
	vdec_command_tokenize(input_param, input_size, &conv_handle.address, NULL, close_fill_input);

	//video init
	if(conv_handle.handle != NULL)
	{
		ret = vdec_close(conv_handle.handle);

		close_fill_output(ret, NULL, output_param, output_size);
	}

	return ret;
}

static int seqheader_fill_input(char* str_token, long* handle, void* param)
{
	int ret = -1;
	const char* each_param = NULL;
	unsigned int param_size = 0;
	unsigned int param_idx = 0U;

	vdec_seqheader_input_t* seqheader_input = (vdec_seqheader_input_t*)param;

	//DLOGI("token->%s", str_token);

	for(param_idx=0U; param_idx < sizeof(vdec_seqheader_input_param) / sizeof(vdec_seqheader_input_param[0]); param_idx++)
	{
		each_param = vdec_seqheader_input_param[param_idx];
		param_size = strlen(each_param);

		if (strncmp(str_token, each_param, param_size) == 0)
		{
			char* strValue = str_token + param_size + 1;
			//DLOGI("%s:%s", each_param, strValue);

			if(strcmp(each_param, "handle") == 0)
			{
				*handle = strtol(strValue, NULL, 16);
				DLOGI("set video handle:0x%lx, value:%s", *handle, strValue);
			}
			else if(strcmp(each_param, "buffer") == 0)
			{
				ConvBuffer cov_buffer;
				cov_buffer.buffer_address = strtol(strValue, NULL, 16);
				seqheader_input->input_buffer = cov_buffer.buffer;
				DLOGI("set buffer:0x%x(%d), value:%s", seqheader_input->input_buffer, cov_buffer.buffer_address, strValue);
			}
			else if(strcmp(each_param, "size") == 0)
			{
				seqheader_input->input_size = (unsigned int)atoi(strValue);
				DLOGI("set buffer size:%u, value:%s", seqheader_input->input_size, strValue);
			}
			else
			{
				DLOGE("unknown parameter:%s", each_param);
			}

			ret = 0;
		}
	}

	if(ret == -1)
	{
		DLOGE("unknown parameter:%s", str_token);
	}

	return ret;
}

int seqheader_fill_output(int vdec_ret, void* param, void* output_param, unsigned int* output_size)
{
	int approx_size = 0;
	unsigned int ii = 0U;

	for(ii=0U; ii < (sizeof(vdec_seqheader_output_param)/sizeof(vdec_seqheader_output_param[0])); ii++)
	{
		approx_size += strlen(vdec_seqheader_output_param[ii]) + 1; //approx value size to minimum 1
	}

	if((output_size != NULL) && (*output_size > approx_size))
	{
		snprintf(output_param, *output_size, "return:%d", vdec_ret);
		*output_size = strlen(output_param);
	}

	return 0;
}

int vdec_command_seqheader(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size)
{
	int ret = 0;
	ConvHandle conv_handle;
	vdec_seqheader_input_t seqheader_input;
	vdec_seqheader_output_t seqheader_output;

	memset(&seqheader_input, 0x00, sizeof(vdec_seqheader_input_t));
	memset(&seqheader_output, 0x00, sizeof(vdec_seqheader_output_t));

	conv_handle.handle = NULL;
	vdec_command_tokenize(input_param, input_size, &conv_handle.address, &seqheader_input, seqheader_fill_input);

	DLOGI("seqheader_input, addresss:0x%lx, handle:0x%lx, pucStream:0x%lx, size:%d", conv_handle.address, conv_handle.handle, seqheader_input.input_buffer, seqheader_input.input_size);

	if(seqheader_input.input_buffer != NULL)
	{
		unsigned char* readBuffer = seqheader_input.input_buffer;

		DLOGI("VideoDecoder_DecodeSequenceHeader, handle:0x%lx, size:%d, %02X %02X %02X %02X %02X %02X %02X %02X", conv_handle.handle, seqheader_input.input_size, readBuffer[0], readBuffer[1], readBuffer[2], readBuffer[3], readBuffer[4], readBuffer[5], readBuffer[6], readBuffer[7]);
		ret = vdec_seq_header(conv_handle.handle, &seqheader_input, &seqheader_output);
		if(ret != 0)
		{
			DLOGE("VPU sequence header init error:%d", ret);
		}
	}

	seqheader_fill_output(ret, NULL, output_param, output_size);

	return ret;
}

static int decode_fill_input(char* str_token, long* handle, void* param)
{
	int ret = -1;
	const char* each_param = NULL;
	unsigned int param_size = 0;
	unsigned int param_idx = 0U;

	vdec_decode_input_t* decode_input = (vdec_decode_input_t*)param;

	//DLOGI("token->%s", str_token);

	for(param_idx=0U; param_idx < sizeof(vdec_decode_input_param) / sizeof(vdec_decode_input_param[0]); param_idx++)
	{
		each_param = vdec_decode_input_param[param_idx];
		param_size = strlen(each_param);

		if (strncmp(str_token, each_param, param_size) == 0)
		{
			char* strValue = str_token + param_size + 1;
			//DLOGI("%s:%s", each_param, strValue);

			if(strcmp(each_param, "handle") == 0)
			{
				*handle = strtol(strValue, NULL, 16);
				DLOGI("set video handle:0x%lx, value:%s", *handle, strValue);
			}
			else if(strcmp(each_param, "buffer") == 0)
			{
				ConvBuffer cov_buffer;
				cov_buffer.buffer_address = strtol(strValue, NULL, 16);
				decode_input->input_buffer = cov_buffer.buffer;
				DLOGI("set buffer:0x%x(%d), value:%s", decode_input->input_buffer, cov_buffer.buffer_address, strValue);
			}
			else if(strcmp(each_param, "size") == 0)
			{
				decode_input->input_size = (unsigned int)atoi(strValue);
				DLOGI("set buffer size:%u, value:%s", decode_input->input_size, strValue);
			}
			else if(strcmp(each_param, "skip_mode") == 0)
			{
				decode_input->skip_mode = (unsigned int)atoi(strValue);
				DLOGI("set skip_mode:%d, value:%s", decode_input->skip_mode, strValue);
			}
			else
			{
				DLOGE("unknown parameter:%s", each_param);
			}

			ret = 0;
		}
	}

	if(ret == -1)
	{
		DLOGE("unknown parameter:%s", str_token);
	}

	return ret;
}

int decode_fill_output(int vdec_ret, void* param, void* output_param, unsigned int* output_size)
{
	int approx_size = 0;
	unsigned int ii = 0U;
	vdec_decode_output_t* decode_out = (vdec_decode_output_t*)param;

	for(ii=0U; ii < (sizeof(vdec_decode_output_param)/sizeof(vdec_decode_output_param[0])); ii++)
	{
		approx_size += strlen(vdec_decode_output_param[ii]) + 1; //approx value size to minimum 1
	}

	DLOGI("*output_size:%d, approx_size:%d", *output_size, approx_size);
	if((output_size != NULL) && (*output_size > approx_size))
	{
		snprintf(output_param, *output_size, "return:%d,display_output_pa:0x%lx,0x%lx,0x%lx,display_output_va:0x%lx,0x%lx,0x%lx,"
			"display_idx:%d,dma_buf_id:%d,interlaced_frame:%d,display_width:%d,display_height:%d,"
			"display_crop:%d,%d,%d,%d,userdata_buf_addr:%d,userdata_buffer_size:%d,top_field_first:%d",
			vdec_ret,
			decode_out->display_output[VDEC_PA][VDEC_COMP_Y], decode_out->display_output[VDEC_PA][VDEC_COMP_U], decode_out->display_output[VDEC_PA][VDEC_COMP_V],
			decode_out->display_output[VDEC_VA][VDEC_COMP_Y], decode_out->display_output[VDEC_VA][VDEC_COMP_U], decode_out->display_output[VDEC_VA][VDEC_COMP_V],
			decode_out->display_idx, decode_out->dma_buf_id, decode_out->interlaced_frame, decode_out->display_width, decode_out->display_height,
			decode_out->display_crop.left, decode_out->display_crop.right, decode_out->display_crop.top, decode_out->display_crop.bottom,
			decode_out->userdata_buf_addr[VDEC_PA], decode_out->userdata_buffer_size, decode_out->top_field_first);

		*output_size = strlen(output_param);
	}

	return 0;
}

int vdec_command_decode(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size)
{
	int ret = 0;
	ConvHandle conv_handle;
	vdec_decode_input_t decode_input;
	vdec_decode_output_t decode_output;

	memset(&decode_input, 0x00, sizeof(vdec_decode_input_t));
	memset(&decode_output, 0x00, sizeof(vdec_decode_output_t));

	conv_handle.handle = NULL;
	vdec_command_tokenize(input_param, input_size, &conv_handle.address, &decode_input, decode_fill_input);

	//DLOGI("decode input pucStream:%p, size:%d", decode_input.pucStream, decode_input.uiStreamLen);

	if(decode_input.input_buffer != NULL)
	{
		unsigned char* readBuffer = decode_input.input_buffer;

		//DLOGI("VideoDecoder_Decode, size:%d, %02X %02X %02X %02X %02X %02X %02X %02X", iBufferSize, readBuffer[0], readBuffer[1], readBuffer[2], readBuffer[3], readBuffer[4], readBuffer[5], readBuffer[6], readBuffer[7]);
		ret = vdec_decode(conv_handle.handle, &decode_input, &decode_output);
		if(ret != 0)
		{
			DLOGE("VPU decode error:%d", ret);
		}
	}

#if 0 //dump ok checked
	{
		{
			FILE *pFs;
			char fname[256];
			int dumpSize = 0;

			memset(fname, 0x00, 256);
			sprintf(fname, "/run/media/sda1/vpu_decode_output_%dx%d.yuv", decode_output.uiWidth, decode_output.uiHeight);

			DLOGI("write to dump:%s", fname);

			pFs = fopen(fname, "ab+");
			if (!pFs) {
				DLOGE("Cannot open '%s'", fname);
				return;
			}

			fwrite(decode_output.pucVirAddr[VideoDecoder_Comp_Y], decode_output.uiWidth * decode_output.uiHeight, 1, pFs);
			dumpSize = decode_output.uiWidth * decode_output.uiHeight >> 2;
			fwrite(decode_output.pucVirAddr[VideoDecoder_Comp_U], dumpSize, 1, pFs);
			fwrite(decode_output.pucVirAddr[VideoDecoder_Comp_V], dumpSize, 1, pFs);
			/* uv interleave mode
			{
				dumpSize = width * height >> 1;
				fwrite( U, dumpSize, 1, pFs);
			}
			*/
			fclose(pFs);
		}
	}
#endif

	decode_fill_output(ret, &decode_output, output_param, output_size);

	return ret;
}

static int bufclear_fill_input(char* str_token, long* handle, void* param)
{
	int ret = -1;
	const char* each_param = NULL;
	unsigned int param_size = 0;
	unsigned int param_idx = 0U;

	vdec_buf_clear_t* init_info = (vdec_init_t*)param;

	//DLOGI("token->%s", str_token);

	for(param_idx=0U; param_idx < sizeof(vdec_bufclear_input_param) / sizeof(vdec_bufclear_input_param[0]); param_idx++)
	{
		each_param = vdec_bufclear_input_param[param_idx];
		param_size = strlen(each_param);

		if (strncmp(str_token, each_param, param_size) == 0)
		{
			char* strValue = str_token + param_size + 1;
			DLOGI("%s:%s", each_param, strValue);

			if(strcmp(each_param, "handle") == 0)
			{
				*handle = strtol(strValue, NULL, 16);
				DLOGI("set video handle:0x%lx, value:%s", *handle, strValue);
			}
			else if(strcmp(each_param, "display_index") == 0)
			{
				init_info->display_index = strtol(strValue, NULL, 16);
				DLOGI("set display_index:%d, value:%s", init_info->display_index, strValue);
			}
			else if(strcmp(each_param, "dma_buf_id") == 0)
			{
				init_info->dma_buf_id = strtol(strValue, NULL, 16);
				DLOGI("set dma_buf_id:%d, value:%s", init_info->dma_buf_id, strValue);
			}
			else
			{
				DLOGE("unknown parameter:%s", each_param);
			}

			ret = 0;
		}
	}

	if(ret == -1)
	{
		DLOGE("unknown parameter:%s", str_token);
	}

	return ret;
}

int bufclear_fill_output(int vdec_ret, void* param, void* output_param, unsigned int* output_size)
{
	int approx_size = 0;
	unsigned int ii = 0U;

	for(ii=0U; ii < (sizeof(vdec_bufclear_output_param)/sizeof(vdec_bufclear_output_param[0])); ii++)
	{
		approx_size += strlen(vdec_flush_output_param[ii]) + 1; //approx value size to minimum 1
	}

	if((output_size != NULL) && (*output_size > approx_size))
	{
		snprintf(output_param, *output_size, "return:%d", vdec_ret);

		*output_size = strlen(output_param);
	}

	return 0;
}

int vdec_command_bufclear(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size) //not implemented in t-codec
{
	int ret = 0;
	ConvHandle conv_handle;
	vdec_buf_clear_t bufclear_input;

	memset(&bufclear_input, 0x00, sizeof(vdec_buf_clear_t));

	conv_handle.handle = NULL;
	vdec_command_tokenize(input_param, input_size, &conv_handle.address, &bufclear_input, bufclear_fill_input);

	ret = vdec_buf_clear(conv_handle.handle, &bufclear_input);

	bufclear_fill_output(ret, NULL, output_param, output_size);

	return ret;
}

static int flush_fill_input(char* str_token, long* handle, void* param)
{
	int ret = -1;
	const char* each_param = NULL;
	unsigned int param_size = 0;
	unsigned int param_idx = 0U;

	//DLOGI("token->%s", str_token);

	for(param_idx=0U; param_idx < sizeof(vdec_flush_input_param) / sizeof(vdec_flush_input_param[0]); param_idx++)
	{
		each_param = vdec_flush_input_param[param_idx];
		param_size = strlen(each_param);

		if (strncmp(str_token, each_param, param_size) == 0)
		{
			char* strValue = str_token + param_size + 1;
			DLOGI("%s:%s", each_param, strValue);

			if(strcmp(each_param, "handle") == 0)
			{
				*handle = strtol(strValue, NULL, 16);
				DLOGI("set video handle:0x%lx, value:%s", *handle, strValue);
			}
			else
			{
				DLOGE("unknown parameter:%s", each_param);
			}

			ret = 0;
		}
	}

	if(ret == -1)
	{
		DLOGE("unknown parameter:%s", str_token);
	}

	return ret;
}

int flush_fill_output(int vdec_ret, void* param, void* output_param, unsigned int* output_size)
{
	int approx_size = 0;
	unsigned int ii = 0U;

	for(ii=0U; ii < (sizeof(vdec_flush_output_param)/sizeof(vdec_flush_output_param[0])); ii++)
	{
		approx_size += strlen(vdec_flush_output_param[ii]) + 1; //approx value size to minimum 1
	}

	if((output_size != NULL) && (*output_size > approx_size))
	{
		snprintf(output_param, *output_size, "return:%d", vdec_ret);

		*output_size = strlen(output_param);
	}

	return 0;
}

int vdec_command_flush(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size)
{
	int ret = 0;
	ConvHandle conv_handle;

	conv_handle.handle = NULL;
	vdec_command_tokenize(input_param, input_size, &conv_handle.address, NULL, flush_fill_input);

	ret = vdec_flush(conv_handle.handle);

	flush_fill_output(ret, NULL, output_param, output_size);

	return ret;
}

static int drain_fill_input(char* str_token, long* handle, void* param)
{
	int ret = -1;
	const char* each_param = NULL;
	unsigned int param_size = 0;
	unsigned int param_idx = 0U;

	//DLOGI("token->%s", str_token);

	for(param_idx=0U; param_idx < sizeof(vdec_drain_input_param) / sizeof(vdec_drain_input_param[0]); param_idx++)
	{
		each_param = vdec_drain_input_param[param_idx];
		param_size = strlen(each_param);

		if (strncmp(str_token, each_param, param_size) == 0)
		{
			char* strValue = str_token + param_size + 1;
			DLOGI("%s:%s", each_param, strValue);

			if(strcmp(each_param, "handle") == 0)
			{
				*handle = strtol(strValue, NULL, 16);
				DLOGI("set video handle:0x%lx, value:%s", *handle, strValue);
			}
			else
			{
				DLOGE("unknown parameter:%s", each_param);
			}

			ret = 0;
		}
	}

	if(ret == -1)
	{
		DLOGE("unknown parameter:%s", str_token);
	}

	return ret;
}

int vdec_command_drain(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size) //not implemented in t-codec
{
	int ret = 0;
	ConvHandle conv_handle;
	vdec_decode_output_t decode_output;

	memset(&decode_output, 0x00, sizeof(vdec_decode_output_t));

	conv_handle.handle = NULL;
	vdec_command_tokenize(input_param, input_size, &conv_handle.address, NULL, drain_fill_input);

	ret = vdec_drain(conv_handle.handle, &decode_output);

	decode_fill_output(ret, &decode_output, output_param, output_size);
}

enum VPU_DecodeWrapper_Interface_Type vdec_get_interface_type(void)
{
	return VPU_WRAPPER_INTERFACE_VDEC;
}