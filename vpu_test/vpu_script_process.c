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

#include "vpu_script_common.h"
#include "vpu_script_decoder_wrapper.h"
#include "vpu_script_process.h"
#include "vpu_script_system.h"
#include "vdec_v3.h"
#include "video_decoder.h"

static const char* part_name = "proc";
#define DLOGD(fmt, args...) 	DLOG(VS_LOG_DEBUG, part_name, "[id:%d]:" fmt, ctx->id, ## args)	//debug
#define DLOGI(fmt, args...)  	DLOG(VS_LOG_INFO, part_name, "[id:%d]:" fmt, ctx->id, ## args)
#define DLOGE(fmt, args...) 	DLOG(VS_LOG_ERROR, part_name, "[id:%d]:" fmt, ctx->id, ## args)	//error

#define CMD_BUF_SIZE 			(64)
#define PARAM_BUF_SIZE			(512)
#define SHORT_PARAM_SIZE		(32)
#define READ_BUFFER_SIZE		(2 * (1024) * (1024))
#define TIMER_NAME_MAX			(64)
#define MAX_URI_SIZE			(1024)

typedef union ConvHandle {
	void* handle;
	long address;
} ConvHandle;

typedef struct TimeCheck_t {
	char name[TIMER_NAME_MAX];
	long start;
	long finish;
}TimeCheck_t;

typedef struct RepeatInfo_t {
	unsigned char* start_pos; //repeat_start_position
	int count; // default:1, -1:unlimited, n>0 repeat until n-- > 0
}RepeatInfo_t;

//for each thread
#define MAX_TIME_CHECK		(30)
#define MAX_REPEAT_DEPTH	(4)
typedef struct VPUCommandContext_t
{
	int id;

	int test_id;
	char* name;
	char* desc;

	int search_seqheader;

	int repeat_index;
	RepeatInfo_t repeat_info[MAX_REPEAT_DEPTH];
	int repeat_start_trigger; //repeat start detected
	int repeat_end_trigger; //repeat end detected
	int repeat_count_trigger; //repeat input count

	FILE* fp_contents;
	FILE* fp_dump;

	unsigned char *readBuffer; //file read buffer
	int readsize; //readsize from the buffer
	char* nextbuffer; //indicate next line

	char cmd_input_param[PARAM_BUF_SIZE];
	unsigned int size_input_param;
	char cmd_output_param[PARAM_BUF_SIZE];
	unsigned int size_output_param;

//vdec parameters
	ConvHandle conv_handle;
	int codec_id;
	char output_format[SHORT_PARAM_SIZE];
	int seqheader_init; // initial 0, after seqheader set to 1

	//output information
	int display_index;
	int dma_buf_id;

	//time check
	TimeCheck_t time_check_stack[MAX_TIME_CHECK];
	int time_check_stack_index;
	long total_cumulative_time;

	//result
	VPUScriptResult_t result;
} VPUCommandContext_t;

/*
contents uri:h264_test.es

vdec_alloc
vdec_init codec_id:h264,max_support_width:1920,max_support_height:1080,output_format:yuv420,additional_frame_count:2,enable_ringbuffer_mode:0

//new buffer for read
#vsys_get_video_frame file:0x1234,buffer:0x1212,size:1048576

vdec_seqheader buffer:0x1212,size:37 return:0

repeat_start -1 // -1 until eof or number
vdec_decode buffer:0x1212,size:14534
//return:0,display_output_pa:0x123,0x345,0x456,display_output_va:0x678,0x890,0x987,display_idx:1,interlaced_frame:0,
//display_width:1920,display_height:1080,display_crop:0,0,1920,1088,userdata_buf_addr:0x00,userdata_buffer_size:0,top_field_first:0
vsys_dump
vdec_clear_buffer
repeat_end

vdec_close
vdec_release
*/


typedef int (*fp_cmd_process)(VPUCommandContext_t* ctx, char* command, char* parameter);
typedef struct VPUCommandProcessMap_t
{
	char command[CMD_BUF_SIZE];
	fp_cmd_process cb_cmd_proc;
	char help_str[PARAM_BUF_SIZE];
} VPUCommandProcessMap_t;

static enum VPU_SYSTEM_SUPPORT_CODEC system_support_codec_map(int codec_id)
{
	enum VPU_SYSTEM_SUPPORT_CODEC system_codec;
	enum VPU_DecodeWrapper_Interface_Type interface_type;
	interface_type = vdec_get_interface_type();

	if(interface_type == VPU_WRAPPER_INTERFACE_TCODEC)
	{
		switch(codec_id)
		{
			case VideoDecoder_Codec_None:
				system_codec = VPU_SYSTEM_CODEC_NONE;
			break;
			case VideoDecoder_Codec_MPEG2:
				system_codec = VPU_SYSTEM_CODEC_MPEG2;
			break;
			case VideoDecoder_Codec_MPEG4:
				system_codec = VPU_SYSTEM_CODEC_MPEG4;
			break;
			case VideoDecoder_Codec_H263:
				system_codec = VPU_SYSTEM_CODEC_H263;
			break;
			case VideoDecoder_Codec_H264:
				system_codec = VPU_SYSTEM_CODEC_H264;
			break;
			case VideoDecoder_Codec_H265:
				system_codec = VPU_SYSTEM_CODEC_H265;
			break;
			case VideoDecoder_Codec_VP9:
				system_codec = VPU_SYSTEM_CODEC_VP9;
			break;
			case VideoDecoder_Codec_VP8:
				system_codec = VPU_SYSTEM_CODEC_VP8;
			break;
			case VideoDecoder_Codec_MJPEG:
				system_codec = VPU_SYSTEM_CODEC_MJPEG;
			break;
			case VideoDecoder_Codec_WMV:
				system_codec = VPU_SYSTEM_CODEC_WMV;
			break;
			default:
			break;
		}
	}
	else if(interface_type == VPU_WRAPPER_INTERFACE_VDEC)
	{
		switch(codec_id)
		{
			case VDEC_CODEC_NONE:
				system_codec = VPU_SYSTEM_CODEC_NONE;
			break;
			case VDEC_CODEC_AVC:
				system_codec = VPU_SYSTEM_CODEC_H264;
			break;
			case VDEC_CODEC_MPEG2:
				system_codec = VPU_SYSTEM_CODEC_MPEG2;
			break;
			case VDEC_CODEC_MPEG4:
				system_codec = VPU_SYSTEM_CODEC_MPEG4;
			break;
			case VDEC_CODEC_H263:
				system_codec = VPU_SYSTEM_CODEC_H263;
			break;
			case VDEC_CODEC_MJPG:
				system_codec = VPU_SYSTEM_CODEC_MJPEG;
			break;
			case VDEC_CODEC_VP8:
				system_codec = VPU_SYSTEM_CODEC_VP8;
			break;
			case VDEC_CODEC_HEVC:
				system_codec = VPU_SYSTEM_CODEC_H265;
			break;
			case VDEC_CODEC_VP9:
				system_codec = VPU_SYSTEM_CODEC_VP9;
			break;
			default:
				system_codec = VPU_SYSTEM_CODEC_NONE;
			break;
		}
	}

	return system_codec;
}

static int cmd_proc_title(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	char str_value[256];
	int str_size = 256;
	//DLOGI("param:%s", parameter);

	memset(str_value, 0x00, 256);
	ret = vpu_cmd_extract_substring(parameter, "id", str_value, &str_size);
	if(ret == 0)
	{
		ctx->test_id = atoi(str_value);
		DLOGI("id:%d, %s", ctx->test_id, str_value);

	}

	memset(str_value, 0x00, 256);
	ret = vpu_cmd_extract_substring(parameter, "name", str_value, &str_size);
	if(ret == 0)
	{
		ctx->name = strdup(str_value);
		DLOGI("name:%s, %s", ctx->name, str_value);

	}

	memset(str_value, 0x00, 256);
	ret = vpu_cmd_extract_substring(parameter, "desc", str_value, &str_size);
	if(ret == 0)
	{
		ctx->desc = strdup(str_value);
		DLOGI("desc:%s, %s", ctx->desc, str_value);
	}

	return ret;
}

static int cmd_proc_contents(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	char uri[MAX_URI_SIZE];
	int uri_size = MAX_URI_SIZE;
	//DLOGI("param:%s", parameter);

	memset(uri, 0x00, MAX_URI_SIZE);
	ret = vpu_cmd_extract_substring(parameter, "uri", uri, &uri_size);
	if(ret == 0)
	{
		DLOGI("uri:%s", uri);
		ctx->fp_contents = fopen(uri, "r");
		if(ctx->fp_contents != NULL)
		{
			DLOGI("contents opend:0x%p", ctx->fp_contents);

			ctx->readBuffer = malloc(READ_BUFFER_SIZE);
			if (ctx->readBuffer == NULL)
			{
				DLOGE("readBuffer allocation failed");
				ret = -1;
				fclose(ctx->fp_contents);
				ctx->fp_contents = NULL;
			}
		}
		else
		{
			DLOGE("contents open failed, %s", uri);
		}
	}

	return ret;
}

static int cmd_proc_readfile(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	char* withsize = NULL;
	unsigned int enable_read_data_with_size = 0;

	if(parameter != NULL)
	{
		withsize = strstr(parameter, "withsize");
		if(withsize != NULL)
		{
			enable_read_data_with_size = 1U;
		}
	}

	ctx->readsize = -1;
	if(ctx->fp_contents != NULL)
	{
		if(enable_read_data_with_size == 0U)
		{
			ctx->readsize = parse_video_frame(ctx->fp_contents, ctx->readBuffer, READ_BUFFER_SIZE, system_support_codec_map(ctx->codec_id), &ctx->search_seqheader);
		}
		else
		{
			int read_byte = 0;
			int data_size = 0;
			if ((read_byte = fread(&data_size, 1, 4, ctx->fp_contents)) > 0)
			{
				DLOGI("data size:%d", data_size);
				ctx->readsize = fread(ctx->readBuffer, 1, data_size, ctx->fp_contents);
			}
		}

		DLOGI("readsize:%d", ctx->readsize);
		if(ctx->readsize < 0)
		{
			DLOGE("end of file");
			DLOGE("finish to repeat mode");
			ctx->repeat_info[ctx->repeat_index].count = 0;
		}
	}
	else
	{
		DLOGE("not opend contents");
	}

	return ret;
}

static int cmd_proc_wait(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	int wait_time_ms;
	//DLOGI("param:%s", parameter);

	wait_time_ms = atoi(parameter);
	DLOGI("wait %d ms", wait_time_ms);
	usleep(wait_time_ms * 1000);

	return ret;
}

static int cmd_proc_decode_output_dump(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	//decode_output_dump uri:output_dump.yuv,width:1280,height:720

	int ret = 0;
	char param[MAX_URI_SIZE];
	int param_size = MAX_URI_SIZE;
	//DLOGI("param:%s", parameter);

	memset(param, 0x00, MAX_URI_SIZE);
	ret = vpu_cmd_extract_substring(parameter, "uri", param, &param_size);
	if(ret == 0)
	{
		DLOGI("uri:%s", param);
		ctx->fp_dump = fopen(param, "w");
		if(ctx->fp_dump != NULL)
		{
			DLOGI("dump opend:0x%p", ctx->fp_dump);
		}
		else
		{
			DLOGE("dump open failed, %s", param);
		}
	}

	return ret;
}

static int cmd_proc_timecheck_start(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	//DLOGI("param:%s", parameter);

	ctx->time_check_stack_index++;
	ctx->time_check_stack[ctx->time_check_stack_index].start = get_time_us();
	strncpy(ctx->time_check_stack[ctx->time_check_stack_index].name, parameter, TIMER_NAME_MAX);

	return ret;
}

static int cmd_proc_timecheck_end(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	long elapsed;
	//DLOGI("param:%s", parameter);

	ctx->time_check_stack[ctx->time_check_stack_index].finish = get_time_us();
	elapsed = ctx->time_check_stack[ctx->time_check_stack_index].finish - ctx->time_check_stack[ctx->time_check_stack_index].start;
	DLOGI("==> time checked, idx:%d, name:%s, elapsed:%ld us", ctx->time_check_stack_index, ctx->time_check_stack[ctx->time_check_stack_index].name, elapsed);

	if (ctx->time_check_stack_index == 1) //top index of timecheck
	{
		ctx->result.operation_count++;
		ctx->total_cumulative_time += elapsed;

		if ((ctx->total_cumulative_time > 0) && (ctx->result.operation_count> 0))
		{
			ctx->result.elapsed_time_avg = ctx->total_cumulative_time / ctx->result.operation_count;
		}

		if (ctx->result.elapsed_time_max < elapsed)
		{
			ctx->result.elapsed_time_max = elapsed;
		}

		DLOGD("index:%d, oper_count:%d, cumulative_time:%ld, avg_time:%ld, max_time:%ld",
			ctx->time_check_stack_index,
			ctx->result.operation_count,
			ctx->total_cumulative_time,
			ctx->result.elapsed_time_avg,
			ctx->result.elapsed_time_max);
	}

	ctx->time_check_stack_index--;

	return ret;
}


static int cmd_proc_repeat_start(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	//DLOGI("param:%s", parameter);

	ctx->repeat_count_trigger = atoi(parameter);
	ctx->repeat_start_trigger = 1;
	DLOGI("repeat_count_trigger:%d", ctx->repeat_count_trigger);

	return ret;
}

static int cmd_proc_repeat_end(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	ctx->repeat_end_trigger = 1;

	DLOGI("repeat_index:%d, repeat_count:%d", ctx->repeat_index, ctx->repeat_info[ctx->repeat_index].count);
	return ret;
}

static int cmd_proc_vdec_alloc(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;

	//DLOGI("param:%s", parameter);

	memset(ctx->cmd_input_param, 0x00, CMD_BUF_SIZE);
	memset(ctx->cmd_output_param, 0x00, CMD_BUF_SIZE);
	ctx->size_output_param = CMD_BUF_SIZE;

	ret = vdec_command_alloc(NULL, 0, ctx->cmd_output_param, &ctx->size_output_param);
	DLOGI("output size->%d, output param->%s", ctx->size_output_param, ctx->cmd_output_param);

	if(ctx->size_output_param > 0)
	{
		char strHandle[16];
		unsigned int sizeStrHandle = 0;

		vpu_cmd_extract_substring(ctx->cmd_output_param, "handle", strHandle, &sizeStrHandle);

		ctx->conv_handle.address = strtol(strHandle, NULL, 16);
		DLOGI("set handle:0x%lx", ctx->conv_handle.handle);
	}

	return ret;
}

static int cmd_proc_vdec_release(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;

	//DLOGI("param:%s", parameter);

	memset(ctx->cmd_input_param, 0x00, PARAM_BUF_SIZE);
	memset(ctx->cmd_output_param, 0x00, PARAM_BUF_SIZE);

	if(ctx->conv_handle.handle != NULL)
	{
		snprintf(ctx->cmd_input_param, PARAM_BUF_SIZE, "handle:0x%lx", ctx->conv_handle.handle);

		ctx->size_input_param = strlen(ctx->cmd_input_param);
		ctx->size_output_param = PARAM_BUF_SIZE;
		ret = vdec_command_release(ctx->cmd_input_param, ctx->size_input_param, NULL, NULL);
	}

	ctx->conv_handle.handle = NULL;

	if(ctx->fp_contents != NULL)
	{
		fclose(ctx->fp_contents);
		ctx->fp_contents = NULL;
		DLOGI("contents closed");
	}

	if(ctx->fp_dump != NULL)
	{
		fclose(ctx->fp_dump);
		ctx->fp_dump = NULL;
		DLOGI("dump closed");
	}

	return ret;
}

static int cmd_proc_vdec_init(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	char strParam[SHORT_PARAM_SIZE];
	unsigned int sizeParam = 0;

	//DLOGI("param:%s", parameter);
	ctx->seqheader_init = 0;

	memset(ctx->cmd_input_param, 0x00, PARAM_BUF_SIZE);
	memset(ctx->cmd_output_param, 0x00, PARAM_BUF_SIZE);

	snprintf(ctx->cmd_input_param, PARAM_BUF_SIZE, "handle:0x%lx,%s", ctx->conv_handle.handle, parameter);

	DLOGI("input param:%s", ctx->cmd_input_param);

	memset(strParam, 0x00, SHORT_PARAM_SIZE);
	vpu_cmd_extract_substring(parameter, "codec_id", strParam, &sizeParam);
	if(sizeParam > 0)
	{
		ctx->codec_id = vdec_get_codec_id(strParam);
		DLOGI("strcodec:%s, codec_id:%d", strParam, ctx->codec_id);
	}

	memset(ctx->output_format, 0x00, SHORT_PARAM_SIZE);
	vpu_cmd_extract_substring(parameter, "output_format", ctx->output_format, &sizeParam);
	if(sizeParam > 0)
	{
		DLOGI("output format:%s", ctx->output_format);
	}

	ctx->size_input_param = strlen(ctx->cmd_input_param);
	ctx->size_output_param = PARAM_BUF_SIZE;
	ret = vdec_command_init(ctx->cmd_input_param, ctx->size_input_param, ctx->cmd_output_param, &ctx->size_output_param);

	return ret;
}

static int cmd_proc_vdec_close(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	//DLOGI("param:%s", parameter);

	memset(ctx->cmd_input_param, 0x00, PARAM_BUF_SIZE);
	memset(ctx->cmd_output_param, 0x00, PARAM_BUF_SIZE);

	snprintf(ctx->cmd_input_param, PARAM_BUF_SIZE, "handle:0x%lx", ctx->conv_handle.handle);

	ctx->size_input_param = strlen(ctx->cmd_input_param);
	ctx->size_output_param = PARAM_BUF_SIZE;
	DLOGI("close input param size:%d, %s", ctx->size_input_param, ctx->cmd_input_param);
	ret = vdec_command_close(ctx->cmd_input_param, ctx->size_input_param, NULL, NULL);

	return ret;
}

static int cmd_proc_vdec_seqheader(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	int found_seq_header = 0;
	//DLOGI("param:%s", parameter);

	if(ctx->readsize > 0)
	{
		if(ctx->seqheader_init == 0)
		{
			if (found_seq_header == 0)
			{
				if (scan_sequence_header(ctx->readBuffer, ctx->readsize, system_support_codec_map(ctx->codec_id)) < 0)
				{
					DLOGE("Not Found sequence header");
					ret = -1;
				}
				else
				{
					unsigned char* ptr = ctx->readBuffer;
					DLOGI("Found sequence header:0x%lx, 0x%x %x %x %x %x %x %x", ptr, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6]);
					found_seq_header = 1;
				}
			}

			if(found_seq_header == 1)
			{
				memset(ctx->cmd_input_param, 0x00, PARAM_BUF_SIZE);
				memset(ctx->cmd_output_param, 0x00, PARAM_BUF_SIZE);

				snprintf(ctx->cmd_input_param, PARAM_BUF_SIZE, "handle:0x%lx,buffer:0x%lx,size:%d", ctx->conv_handle.handle, ctx->readBuffer, ctx->readsize);

				ctx->size_input_param = strlen(ctx->cmd_input_param);
				ctx->size_output_param = PARAM_BUF_SIZE;
				DLOGI("sequence header input param size:%d, %s", ctx->size_input_param, ctx->cmd_input_param);
				ret = vdec_command_seqheader(ctx->cmd_input_param, ctx->size_input_param, ctx->cmd_output_param, &ctx->size_output_param);
				DLOGI("sequence header output param size:%d, %s", ctx->size_output_param, ctx->cmd_output_param);
				ctx->seqheader_init = 1;
			}
		}
	}
	else
	{
		DLOGE("no data to sequence header init");
	}

	return ret;
}

static void decode_dump(VPUCommandContext_t* ctx, char* dec_out_param)
{
	int ret = 0;
	int param_size;
	char str_addr1[16];
	char str_addr2[16];
	char str_addr3[16];
	char str_value[16];

	memset(str_value, 0x00, 16);

	if(ctx->fp_dump != NULL)
	{
		int write_ret = 0;

		unsigned char* virtual_y = NULL;
		unsigned char* virtual_u = NULL;
		unsigned char* virtual_v = NULL;
		unsigned int width;
		unsigned int height;
		unsigned int framesize;

		memset(str_addr1, 0x00, 16);
		memset(str_addr2, 0x00, 16);
		memset(str_addr3, 0x00, 16);

		//snprintf(output_param, *output_size, "return:%d,display_output_pa:0x%lx,%0x%lx,%0x%lx,display_output_va:%0x%lx,%0x%lx,%0x%lx,"
		//	"display_idx:%d,interlaced_frame:%d,display_width:%d,display_height:%d,"
		//	"display_crop:%d,%d,%d,%d,userdata_buf_addr:%d,userdata_buffer_size:%d,top_field_first:%d",

		ret = vpu_cmd_extract_substring_output_address(dec_out_param, "display_output_va", str_addr1, str_addr2, str_addr3);

		if(strlen(str_addr1) > 0)
		{
			virtual_y = (char*)strtol(str_addr1, NULL, 16);
		}

		if(strlen(str_addr2) > 0)
		{
			virtual_u = (char*)strtol(str_addr2, NULL, 16);
		}

		if(strlen(str_addr3) > 0)
		{
			virtual_v = (char*)strtol(str_addr3, NULL, 16);
		}

		DLOGI("output addr va:%s,%s,%s, conveted y:0x%lx, u:0x%lx, v:0x%lx", str_addr1, str_addr2, str_addr3, virtual_y, virtual_u, virtual_v);

		param_size = 16;
		memset(str_value, 0x00, 16);
		ret = vpu_cmd_extract_substring(dec_out_param, "display_width", str_value, &param_size);
		if(strlen(str_value) > 0)
		{
			width = atoi(str_value);
		}

		param_size = 16;
		memset(str_value, 0x00, 16);
		ret = vpu_cmd_extract_substring(dec_out_param, "display_height", str_value, &param_size);
		if(strlen(str_value) > 0)
		{
			height = atoi(str_value);
		}

		DLOGI("display resoution %u x %u", width, height);

#if defined(TCC_VPU_V3)
		DLOGI("dump output format:%s", ctx->output_format);
		if (strcmp(ctx->output_format, "yuv420") == 0)
		{
			if ((virtual_y != NULL) && (virtual_u != NULL) && (virtual_v != NULL))
			{
				framesize = width * height;
				DLOGI("dump yuv:0x%lx,0x%lx,0x%lx,  %dx%d", virtual_y, virtual_u, virtual_v, width, height);
				write_ret = fwrite(virtual_y, 1, framesize, ctx->fp_dump);
				write_ret = fwrite(virtual_u, 1, framesize / 4, ctx->fp_dump);
				write_ret = fwrite(virtual_v, 1, framesize / 4, ctx->fp_dump);
				DLOGI("dump frame");
			}
		}
		else if (strcmp(ctx->output_format, "nv12") == 0)
		{
			if ((virtual_y != NULL) && (virtual_u != NULL) && (virtual_v != NULL))
			{
				framesize = width * height;
				write_ret = fwrite(virtual_y, framesize, 1, ctx->fp_dump);

				framesize = width * (height / 2);
				write_ret = fwrite(virtual_u, framesize, 1, ctx->fp_dump);
				DLOGI("dump frame");
			}
		}
		else if (strcmp(ctx->output_format, "mapconv") == 0)
		{
			DLOGI("not implemented yet for mapconverter output");
		}
#endif
	}
}

#define DISPLAY_INDEX_BUF	(16)
static int cmd_proc_vdec_decode(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	int param_size;
	char str_outputinfo[DISPLAY_INDEX_BUF];

	//DLOGI("param:%s", parameter);

	if(ctx->readsize > 0)
	{
memset(ctx->cmd_input_param, 0x00, PARAM_BUF_SIZE);
		memset(ctx->cmd_output_param, 0x00, PARAM_BUF_SIZE);

		snprintf(ctx->cmd_input_param, PARAM_BUF_SIZE, "handle:0x%lx,buffer:0x%lx,size:%d,skip_mode:0", ctx->conv_handle.handle, ctx->readBuffer, ctx->readsize);

		ctx->size_input_param = strlen(ctx->cmd_input_param);
		ctx->size_output_param = PARAM_BUF_SIZE;
		DLOGI("decode input param size:%d, %s", ctx->size_input_param, ctx->cmd_input_param);
		ret = vdec_command_decode(ctx->cmd_input_param, ctx->size_input_param, ctx->cmd_output_param, &ctx->size_output_param);
		DLOGI("decode output param size:%d, %s", ctx->size_output_param, ctx->cmd_output_param);

		param_size = DISPLAY_INDEX_BUF;
		memset(str_outputinfo, 0x00, DISPLAY_INDEX_BUF);
		ret = vpu_cmd_extract_substring(ctx->cmd_output_param, "display_idx", str_outputinfo, &param_size);
		if(strlen(str_outputinfo) > 0)
		{
			ctx->display_index = atoi(str_outputinfo);
			DLOGI("display_index:%d, %s", ctx->display_index , str_outputinfo);
		}

		param_size = DISPLAY_INDEX_BUF;
		memset(str_outputinfo, 0x00, DISPLAY_INDEX_BUF);
		ret = vpu_cmd_extract_substring(ctx->cmd_output_param, "dma_buf_id", str_outputinfo, &param_size);
		if(strlen(str_outputinfo) > 0)
		{
			ctx->dma_buf_id = atoi(str_outputinfo);
			DLOGI("dma_buf_id:%d, %s", ctx->dma_buf_id , str_outputinfo);
		}
	}
	else
	{
		DLOGE("no data to decode");
	}

	return ret;
}

static int cmd_proc_vdec_clear_buffer(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	int param_size;

	//DLOGI("param:%s", parameter);
	memset(ctx->cmd_input_param, 0x00, PARAM_BUF_SIZE);
	snprintf(ctx->cmd_input_param, PARAM_BUF_SIZE, "handle:0x%lx,display_index:%d,dma_buf_id:%d", ctx->conv_handle.handle, ctx->display_index, ctx->dma_buf_id);

	ctx->size_input_param = strlen(ctx->cmd_input_param);
	ctx->size_output_param = PARAM_BUF_SIZE;
	DLOGI("bufclear input param size:%d, %s", ctx->size_input_param, ctx->cmd_input_param);
	ret = vdec_command_bufclear(ctx->cmd_input_param, ctx->size_input_param, ctx->cmd_output_param, &ctx->size_output_param);
	DLOGI("bufclear output param size:%d, %s", ctx->size_output_param, ctx->cmd_output_param);

	return ret;
}

static int cmd_proc_vdec_drain(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	//DLOGI("param:%s", parameter);

	return ret;
}

static int cmd_proc_vdec_flush(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	//DLOGI("param:%s", parameter);

	return ret;
}

static VPUCommandProcessMap_t cmd_process_map[] =
{
	{"title", cmd_proc_title, "ex) title id:1,name:h264_decoding,desc:decoding test of h.264 hd resolution"},
	{"contents", cmd_proc_contents, "ex) contents uri:/run/media/sda1/1080p_es.h264"},
	{"readfile", cmd_proc_readfile, "readfile to internal buffer, :read data size of each frame, ex)readfile withsize"},
	{"wait", cmd_proc_wait, "wait 500 #500ms second wait"},
	{"decode_output_dump", cmd_proc_decode_output_dump, "video decode output dump, ex) decode_output_dump uri:output_dump.yuv"},
	{"timecheck_start", cmd_proc_timecheck_start, "time check start"},
	{"timecheck_end", cmd_proc_timecheck_end, "time check end, show elapsed time"},
	{"repeat_start", cmd_proc_repeat_start, "repeat_start 10 #repeat count"},
	{"repeat_end", cmd_proc_repeat_end, "repeat_end until count to 0"},
	{"vdec_alloc", cmd_proc_vdec_alloc, "decoder alloc"},
	{"vdec_release", cmd_proc_vdec_release, "decoder release"},
	{"vdec_init", cmd_proc_vdec_init, "decoder init, ex)vdec_init codec_id:h264,max_support_width:1920,max_support_height:1080,output_format:yuv420,additional_frame_count:2,enable_ringbuffer_mode:0"},
	{"vdec_close", cmd_proc_vdec_close, "decoder close"},
	{"vdec_seqheader", cmd_proc_vdec_seqheader, "decoder sequence header init"},
	{"vdec_decode", cmd_proc_vdec_decode, "decode"},
	{"vdec_clear_buffer", cmd_proc_vdec_clear_buffer, "decoded display buffer clear"},
	{"vdec_drain", cmd_proc_vdec_drain, "decoder drain"},
	{"vdec_flush", cmd_proc_vdec_flush, "decoder flush"},
};

static int cmd_process(VPUCommandContext_t* ctx, char* command, char* parameter)
{
	int ret = 0;
	int ii;
	int cmd_count = sizeof(cmd_process_map) / sizeof(cmd_process_map[0]);

	for(ii = 0; ii < cmd_count; ii++)
	{
		VPUCommandProcessMap_t* cmd_map = &cmd_process_map[ii];

		if(strcmp(command, cmd_map->command) == 0)
		{
			ret = cmd_map->cb_cmd_proc(ctx, command, parameter);
			break;
		}
	}

	return ret;
}

VPUCmdProc_h vpu_proc_alloc(int id)
{
	int ii = 0;
	VPUCommandContext_t* ctx = NULL;

	ctx = malloc(sizeof(VPUCommandContext_t));
	if(ctx != NULL)
	{
		memset(ctx, 0x00, sizeof(VPUCommandContext_t));

		//initial values
		ctx->id = id;
		ctx->search_seqheader = 0;
		ctx->repeat_index = -1;
		for(ii=0; ii<MAX_REPEAT_DEPTH; ii++)
		{
			ctx->repeat_info[ii].count = 1;
		}
	}

	return ctx;
}

void vpu_proc_release(VPUCmdProc_h hProc)
{
	VPUCommandContext_t* ctx = (VPUCommandContext_t*)hProc;

	if (ctx != NULL)
	{
		(void)cmd_proc_vdec_close(ctx, "vdec_close", NULL);

		if (ctx->name != NULL)
		{
			free(ctx->name);
			ctx->name = NULL;
		}

		if (ctx->desc != NULL)
		{
			free(ctx->desc);
			ctx->desc = NULL;
		}

		free(ctx);
		ctx = NULL;
	}
}

void vpu_proc_help(void)
{
	int ret = 0;
	int ii;
	int cmd_count = sizeof(cmd_process_map) / sizeof(cmd_process_map[0]);

	printf("---------------------------------- help ------------------------------------------\n");
	for(ii = 0; ii < cmd_count; ii++)
	{
		VPUCommandProcessMap_t* cmd_map = &cmd_process_map[ii];

		printf("%s \t %s\n", cmd_map->command, cmd_map->help_str);
	}

	printf("------------------------------------------------------------------------------------\n");
}

int vpu_proc_parser(VPUCmdProc_h hProc, char* script_buffer, const unsigned int script_size, VPUScriptResult_t* result)
{
	int ret = 0;
	char command[CMD_BUF_SIZE];
	char parameter[PARAM_BUF_SIZE];
	int cmd_size = CMD_BUF_SIZE;
	int param_size = PARAM_BUF_SIZE;

	VPUCommandContext_t* ctx = (VPUCommandContext_t*)hProc;

	memset(command, 0x00, CMD_BUF_SIZE);
	memset(parameter, 0x00, PARAM_BUF_SIZE);

	if(ctx->nextbuffer == NULL)
	{
		ctx->nextbuffer = (char*)script_buffer;
	}

	ctx->nextbuffer = readCommandUntilNewline(ctx->nextbuffer, command, cmd_size, parameter, param_size);
	if((ctx->nextbuffer != NULL) || (strlen(command) > 0))
	{
		if(strlen(command) > 0)
		{
			int cmd_ret = 0;

			DLOGI("line command:%s, param:%s", command, parameter);
			cmd_ret = cmd_process(ctx, command, parameter);
			if (cmd_ret != 0)
			{
				DLOGE("error cmd:%s, param:%s", command, parameter);
			}

			if (ctx->repeat_start_trigger == 1)
			{
				ctx->repeat_index++;
				ctx->repeat_info[ctx->repeat_index].count = ctx->repeat_count_trigger;
				ctx->repeat_info[ctx->repeat_index].start_pos = ctx->nextbuffer;
				ctx->repeat_start_trigger = 0;

				DLOGI("repeat start idx:%d, count:%d", ctx->repeat_index, ctx->repeat_info[ctx->repeat_index].count);
			}

			if (ctx->repeat_end_trigger == 1)
			{
				if(ctx->repeat_info[ctx->repeat_index].count == -1)
				{
					ctx->nextbuffer = ctx->repeat_info[ctx->repeat_index].start_pos;
					DLOGI("repeat end idx:%d, count:%d, seek to pos:0x%x", ctx->repeat_index, ctx->repeat_info[ctx->repeat_index].count, ctx->nextbuffer);
				}
				else
				{
					ctx->repeat_info[ctx->repeat_index].count--;
					if (ctx->repeat_info[ctx->repeat_index].count > 0)
					{
						ctx->nextbuffer = ctx->repeat_info[ctx->repeat_index].start_pos;
						DLOGI("repeat end idx:%d, count:%d, seek to pos:0x%x", ctx->repeat_index, ctx->repeat_info[ctx->repeat_index].count, ctx->nextbuffer);
					}
					else
					{
						if(ctx->repeat_index > 0)
						{
							ctx->repeat_info[ctx->repeat_index].start_pos = NULL;
							DLOGI("repeat end idx:%d", ctx->repeat_index);
							ctx->repeat_index--;
						}
					}
				}

				ctx->repeat_end_trigger = 0;
			}
		}

		memset(command, 0x00, CMD_BUF_SIZE);
		memset(parameter, 0x00, PARAM_BUF_SIZE);
	}
	else
	{
		DLOGI("script process done");
		ret = -1; //return -1 will finish the task

		if(result != NULL)
		{
			//return result
			memcpy(result, &ctx->result, sizeof(VPUScriptResult_t));

			DLOGI("---- script process reult ----");
			DLOGI("test id : %d", result->test_id);
			DLOGI("result : %d", result->reult);
			DLOGI("output count : %d", result->output_count);
			DLOGI("operation count : %d", result->operation_count);
			DLOGI("elapsed time avg : %d", result->elapsed_time_avg);
			DLOGI("elapsed time max : %d", result->elapsed_time_max);
		}
	}

	return ret;
}
