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

#ifndef VPU_SCRIPT_DECODER_WRAPPER_H
#define VPU_SCRIPT_DECODER_WRAPPER_H

static const char* vdec_alloc_output_param[] =
{
	"return", // vdec alloc return
	"handle", //address(integer) of decoder handle
	"driver_id", //driver id
	"version", //version of vpu driver
	"capabilities", //driver's capabilities
};

static const char* vdec_release_input_param[] =
{
	"handle", //address(integer) of decoder handle
};

static const char* vdec_release_output_param[] =
{
	"return", // vdec release return
};

static const char* vdec_init_input_param[] =
{
	"handle", //address(integer) of decoder handle
	"codec_id", //"mpeg2, mpeg4, h263, h264, h265, vp9, vp8, mjpeg, wmv"
	"max_support_width", // width
	"max_support_height", // height
	"output_format", // yuv420, nv12, mapconv, afbc
	"additional_frame_count", // count
	"use_forced_pmap_idx", // 0 or 1
	"forced_pmap_idx", // 0 ~ max pmap(16)
	"enable_user_data", // 0 or 1
	"enable_user_framebuffer", // 0 or 1
	"enable_max_framebuffer",
	"enable_user_data",
	"enable_ringbuffer_mode", // 0 or 1
	"enable_no_buffer_delay",
	"enable_avc_field_display",
	"enable_dma_buf_id",
};

static const char* vdec_init_output_param[] =
{
	"return", // vdec init return
};

static const char* vdec_close_input_param[] =
{
	"handle", //address(integer) of decoder handle
};

static const char* vdec_close_output_param[] =
{
	"return", // vdec close return
};

static const char* vdec_seqheader_input_param[] =
{
	"handle", //address(integer) of decoder handle
	"buffer", //input buffer
	"size", //size of buffer
};

static const char* vdec_seqheader_output_param[] =
{
	"return", // vdec seqheader return
	"pic_width", //address(integer) of decoder handle
	"pic_height", //input buffer
	"frame_rate_res", //size of buffer
	"frame_rate_div",
	"min_frame_buffer_count",
	"min_frame_buffer_size",
	"frame_buffer_format",
	"pic_crop",
	"eotf",
	"frame_buf_delay",
	"profile",
	"level",
	"tier",
	"interlace",
	"aspectratio",
	"report_error_reason",
	"bitdepth",
};

static const char* vdec_decode_input_param[] =
{
	"handle", //address(integer) of decoder handle
	"buffer", //input buffer
	"size", //size of buffer
	"skip_mode", //0:audo, 1:manual
};

static const char* vdec_decode_output_param[] =
{
	"return", // vdec decode return
	"display_output_pa", //output of decoding, display_output_physical_address:0x123,0x233,0x344
	"display_output_va", //output of decoding, display_output_virtual_address:0x123,0x233,0x344
	//"pic_type", //type of picture, I, P, B
	"display_idx", //index
	//"decoded_idx", //index
	//"display_status",
	//"decoded_status",
	"interlaced_frame",
	//"num_of_err_mbs",
	//"decoded_width",
	//"decoded_height",
	"display_width",
	"display_height",
	//"decoded_crop", //crop value, decoded_crop:100,200,300,400  -> left,right,top,bottom
	"display_crop", //crop value, decoded_crop:100,200,300,400  -> left,right,top,bottom
	"userdata_buf_addr", //address, userdata_buf_addr:0x123,0x125  pa,va
	"userdata_buffer_size", //size
	//"picture_structure",
	"top_field_first",
};

static const char* vdec_bufclear_input_param[] =
{
	"handle", //address(integer) of decoder handle
	"display_index", //display index
	"dma_buf_id", //dma buff id
};

static const char* vdec_bufclear_output_param[] =
{
	"return", // vdec return
};

static const char* vdec_flush_input_param[] =
{
	"handle", //address(integer) of decoder handle
};

static const char* vdec_flush_output_param[] =
{
	"return", // vdec flush return
};

static const char* vdec_drain_input_param[] =
{
	"handle", //address(integer) of decoder handle
};

//vdec_drain_output_param = vdec_decode_output_param;

enum VPU_DecodeWrapper_Interface_Type
{
	VPU_WRAPPER_INTERFACE_NONE = 0,
	VPU_WRAPPER_INTERFACE_TCODEC,
	VPU_WRAPPER_INTERFACE_VDEC,

	VPU_WRAPPER_INTERFACE_MAX
};

/**
 * @brief Function to perform an operation with string parameters.
 *
 * @param input_param   Array of strings representing the input parameters.
 *                      The strings are converted into a single character array.
 *                      If there are no input parameters, pass an empty array with size 0.
 * @param input_size    The size of the input_param array.
 *                      If there are no input parameters, set input_size to 0.
 * @param output_param  Caller-allocated array of characters representing the output parameters.
 *                      The function fills this array with the result, and the caller must allocate
 *                      sufficient space for the output. The content of output_param is overwritten.
 *                      If there are no output parameters, pass an empty array with size 0.
 * @param output_size   The size of the output_param array. The caller should pass the allocated size
 *                      before the function call, and the function returns the actual size of the
 *                      filled content after execution. If there are no output parameters,
 *                      set output_size to 0.
 */
int vdec_command_alloc(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size);

int vdec_command_release(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size);

int vdec_command_init(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size);

int vdec_command_close(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size);

int vdec_command_seqheader(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size);

int vdec_command_decode(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size);

int vdec_command_bufclear(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size);

int vdec_command_flush(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size);

int vdec_command_drain(const char* input_param, const unsigned int input_size, char* output_param, unsigned int* output_size);

//some utility
int vdec_get_codec_id(const char* strCodec);

enum VPU_DecodeWrapper_Interface_Type vdec_get_interface_type(void);

#endif //VPU_SCRIPT_DECODER_WRAPPER_H