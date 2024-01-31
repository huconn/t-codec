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
#include "video_decoder.h"

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
	VideoDecoder_T* handle;
	long address;
} ConvHandle;

typedef union ConvBuffer {
	unsigned char* buffer;
	long buffer_address;
} ConvBuffer;

typedef struct VideoDecoderStringCodecMap_t
{
	const char* strCodec;
	VideoDecoder_Codec_E codec_id;
}VideoDecoderStringCodecMap_t;

static VideoDecoderStringCodecMap_t videodecoder_str_codec_map[] =
{
	{"mpeg2", VideoDecoder_Codec_MPEG2},
	{"mpeg4", VideoDecoder_Codec_MPEG4},
	{"h263", VideoDecoder_Codec_H263},
	{"h264", VideoDecoder_Codec_H264},
	{"h265", VideoDecoder_Codec_H265},
	{"vp9", VideoDecoder_Codec_VP9},
	{"vp8", VideoDecoder_Codec_VP8},
	{"mjpeg", VideoDecoder_Codec_MJPEG},
	{"wmv", VideoDecoder_Codec_WMV},
};

static VideoDecoder_Codec_E convert_str_to_codec(char* strCodec)
{
	unsigned int codec_idx = 0U;
	VideoDecoder_Codec_E ret_codec = VideoDecoder_Codec_None;

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

	VideoDecoder_T* handle = NULL;

	DLOGI("call VideoDecoder_Create()");
	handle = VideoDecoder_Create();
	if(handle == NULL)
	{
		ret = -1;
	}

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

	ret = VideoDecoder_Destroy(conv_handle.handle);

	release_fill_output(ret, NULL, output_param, output_size);
	return ret;
}

static int init_fill_input(char* str_token, long* handle, void* param)
{
	int ret = -1;
	const char* each_param = NULL;
	unsigned int param_size = 0;
	unsigned int param_idx = 0U;

	VideoDecoder_InitConfig_T* init_config = (VideoDecoder_InitConfig_T*)param;

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
				init_config->eCodec = convert_str_to_codec(strValue);
				DLOGI("set video codec:%d, value:%s", init_config->eCodec, strValue);
			}
			else if(strcmp(each_param, "max_support_width") == 0)
			{
				init_config->uiMax_resolution_width = atoi(strValue);
				DLOGI("set max resolution width:%d, value:%s", init_config->uiMax_resolution_width, strValue);
			}
			else if(strcmp(each_param, "max_support_height") == 0)
			{
				init_config->uiMax_resolution_height = atoi(strValue);
				DLOGI("set max resolution height:%d, value:%s", init_config->uiMax_resolution_height, strValue);
			}
			else if(strcmp(each_param, "output_format") == 0)
			{
				if(strcmp(strValue, "yuv420") == 0)
				{
					init_config->tCompressMode.eMode = VideoDecoder_FrameBuffer_CompressMode_None;
					DLOGI("set compressmode:%d(yuv420), value:%s", init_config->tCompressMode.eMode, strValue);
				}
				else if(strcmp(strValue, "nv12") == 0)
				{
					//DLOGE("not support nv12");
					init_config->tCompressMode.eMode = VideoDecoder_FrameBuffer_CompressMode_None;
					DLOGI("set compressmode:%d(nv12), value:%s", init_config->tCompressMode.eMode, strValue);
				}
				else if(strcmp(strValue, "mapconv") == 0)
				{
					init_config->tCompressMode.eMode = VideoDecoder_FrameBuffer_CompressMode_MapConverter;
					DLOGI("set compressmode:%d(map converter), value:%s", init_config->tCompressMode.eMode, strValue);
				}
				else if(strcmp(strValue, "afbc") == 0)
				{
					DLOGE("not support afbc");
					ret = -1;
				}
			}
			else if(strcmp(each_param, "additional_frame_count") == 0)
			{
				init_config->number_of_surface_frames = atoi(strValue);
				DLOGI("set num of surface frame:%d, value:%s", init_config->number_of_surface_frames, strValue);
			}
			else if(strcmp(each_param, "use_forced_pmap_idx") == 0)
			{
				DLOGI("no parameter for use_forced_pmap_idx, value:%s", strValue);
			}
			else if(strcmp(each_param, "forced_pmap_idx") == 0)
			{
				DLOGI("no parameter for forced_pmap_idx, value:%s", strValue);
			}
			else if(strcmp(each_param, "enable_user_data") == 0)
			{
				init_config->tUserdataMode.bEnable = atoi(strValue);
				DLOGI("set enable_user_data:%d, value:%s", init_config->tUserdataMode.bEnable, strValue);
			}
			else if(strcmp(each_param, "enable_user_framebuffer") == 0)
			{
				DLOGI("no parameter for enable_user_framebuffer, value:%s", strValue);
			}
			else if(strcmp(each_param, "enable_ringbuffer_mode") == 0)
			{
				DLOGI("no parameter for enable_ringbuffer_mode, value:%s", strValue);
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
	VideoDecoder_InitConfig_T init_config;
	ConvHandle conv_handle;

	memset(&init_config, 0x00, sizeof(VideoDecoder_InitConfig_T));
	conv_handle.handle = NULL;
	vdec_command_tokenize(input_param, input_size, &conv_handle.address, &init_config, init_fill_input);

	//video init
	DLOGI("handle:0x%lx, codec:%d, num frame:%d, max width:%d, height:%d", conv_handle.handle, init_config.eCodec, init_config.number_of_surface_frames, init_config.uiMax_resolution_width, init_config.uiMax_resolution_height);
	ret = VideoDecoder_Init(conv_handle.handle, &init_config);

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
	ret = VideoDecoder_Deinit(conv_handle.handle);

	close_fill_output(ret, NULL, output_param, output_size);

	return ret;
}

static int seqheader_fill_input(char* str_token, long* handle, void* param)
{
	int ret = -1;
	const char* each_param = NULL;
	unsigned int param_size = 0;
	unsigned int param_idx = 0U;

	VideoDecoder_SequenceHeaderInputParam_T* seqheader_input = (VideoDecoder_SequenceHeaderInputParam_T*)param;

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
				seqheader_input->pucStream = cov_buffer.buffer;
				DLOGI("set buffer:0x%x(%d), value:%s", seqheader_input->pucStream, cov_buffer.buffer_address, strValue);
			}
			else if(strcmp(each_param, "size") == 0)
			{
				seqheader_input->uiStreamLen = (unsigned int)atoi(strValue);
				DLOGI("set buffer size:%u, value:%s", seqheader_input->uiStreamLen, strValue);
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
	VideoDecoder_SequenceHeaderInputParam_T seqheader_input;

	memset(&seqheader_input, 0x00, sizeof(VideoDecoder_SequenceHeaderInputParam_T));

	conv_handle.handle = NULL;
	vdec_command_tokenize(input_param, input_size, &conv_handle.address, &seqheader_input, seqheader_fill_input);

	DLOGI("seqheader_input, addresss:0x%lx, handle:0x%lx, pucStream:0x%lx, size:%d", conv_handle.address, conv_handle.handle, seqheader_input.pucStream, seqheader_input.uiStreamLen);

	if(seqheader_input.pucStream != NULL)
	{
		unsigned char* readBuffer = seqheader_input.pucStream;
		seqheader_input.uiNumOutputFrames = 4; //temporarily workaround

		DLOGI("VideoDecoder_DecodeSequenceHeader, handle:0x%lx, size:%d, %02X %02X %02X %02X %02X %02X %02X %02X", conv_handle.handle, seqheader_input.uiStreamLen, readBuffer[0], readBuffer[1], readBuffer[2], readBuffer[3], readBuffer[4], readBuffer[5], readBuffer[6], readBuffer[7]);
		ret = VideoDecoder_DecodeSequenceHeader(conv_handle.handle, &seqheader_input);
		if(ret != VideoDecoder_ErrorNone)
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

	VideoDecoder_StreamInfo_T* decode_input = (VideoDecoder_StreamInfo_T*)param;

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
				decode_input->pucStream = cov_buffer.buffer;
				DLOGI("set buffer:0x%x(%d), value:%s", decode_input->pucStream, cov_buffer.buffer_address, strValue);
			}
			else if(strcmp(each_param, "size") == 0)
			{
				decode_input->uiStreamLen = (unsigned int)atoi(strValue);
				DLOGI("set buffer size:%u, value:%s", decode_input->uiStreamLen, strValue);
			}
			else if(strcmp(each_param, "skip_mode") == 0)
			{
				//decode_input->uiStreamLen = (unsigned int)atoi(strValue);
				DLOGI("no parameter for skip_mode, value:%s", strValue);
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
	VideoDecoder_Frame_T* decode_out = (VideoDecoder_Frame_T*)param;

	for(ii=0U; ii < (sizeof(vdec_decode_output_param)/sizeof(vdec_decode_output_param[0])); ii++)
	{
		approx_size += strlen(vdec_decode_output_param[ii]) + 1; //approx value size to minimum 1
	}

	DLOGI("*output_size:%d, approx_size:%d", *output_size, approx_size);
	if((output_size != NULL) && (*output_size > approx_size))
	{
		snprintf(output_param, *output_size, "return:%d,display_output_pa:0x%lx,0x%lx,0x%lx,display_output_va:0x%lx,0x%lx,0x%lx,"
			"display_idx:%d,interlaced_frame:%d,display_width:%d,display_height:%d,"
			"display_crop:%d,%d,%d,%d,userdata_buf_addr:%d,userdata_buffer_size:%d,top_field_first:%d",
			vdec_ret,
			decode_out->pucPhyAddr[VideoDecoder_Comp_Y], decode_out->pucPhyAddr[VideoDecoder_Comp_U], decode_out->pucPhyAddr[VideoDecoder_Comp_V],
			decode_out->pucVirAddr[VideoDecoder_Comp_Y], decode_out->pucVirAddr[VideoDecoder_Comp_U], decode_out->pucVirAddr[VideoDecoder_Comp_V],
			decode_out->iDisplayIndex, decode_out->bInterlaced, decode_out->uiWidth, decode_out->uiHeight,
			decode_out->tCropRect.uiLeft, decode_out->tCropRect.uiWidth, decode_out->tCropRect.uiTop, decode_out->tCropRect.uiHeight,
			decode_out->tUserData.pucData, decode_out->tUserData.uiLen, (!decode_out->bOddFieldFirst));

		*output_size = strlen(output_param);
	}

	return 0;
}

int vdec_command_decode(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size)
{
	int ret = 0;
	ConvHandle conv_handle;
	VideoDecoder_StreamInfo_T decode_input;
	VideoDecoder_Frame_T decode_output;

	memset(&decode_input, 0x00, sizeof(VideoDecoder_StreamInfo_T));
	memset(&decode_output, 0x00, sizeof(VideoDecoder_Frame_T));

	conv_handle.handle = NULL;
	vdec_command_tokenize(input_param, input_size, &conv_handle.address, &decode_input, decode_fill_input);

	//DLOGI("decode input pucStream:%p, size:%d", decode_input.pucStream, decode_input.uiStreamLen);

	if(decode_input.pucStream != NULL)
	{
		unsigned char* readBuffer = decode_input.pucStream;

		//DLOGI("VideoDecoder_Decode, size:%d, %02X %02X %02X %02X %02X %02X %02X %02X", iBufferSize, readBuffer[0], readBuffer[1], readBuffer[2], readBuffer[3], readBuffer[4], readBuffer[5], readBuffer[6], readBuffer[7]);
		ret = VideoDecoder_Decode(conv_handle.handle, &decode_input, &decode_output);
		if(ret != VideoDecoder_ErrorNone)
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

int vdec_command_bufclear(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size) //not implemented in t-codec
{
	int ret = 0;

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

	ret = VideoDecoder_Flush(conv_handle.handle);

	flush_fill_output(ret, NULL, output_param, output_size);

	return ret;
}

int vdec_command_drain(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size) //not implemented in t-codec
{
	int ret = 0;

	return ret;
}

enum VPU_DecodeWrapper_Interface_Type vdec_get_interface_type(void)
{
	return VPU_WRAPPER_INTERFACE_TCODEC;
}