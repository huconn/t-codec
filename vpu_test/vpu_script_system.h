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

#ifndef VPU_SCRIPT_SYSTEM_H
#define VPU_SCRIPT_SYSTEM_H

enum VPU_SYSTEM_SUPPORT_CODEC
{
	VPU_SYSTEM_CODEC_NONE = 0,
	VPU_SYSTEM_CODEC_H263,
	VPU_SYSTEM_CODEC_H264,
	VPU_SYSTEM_CODEC_H265,
	VPU_SYSTEM_CODEC_MPEG2,
	VPU_SYSTEM_CODEC_MPEG4,
	VPU_SYSTEM_CODEC_MJPEG,
	VPU_SYSTEM_CODEC_VP8,
	VPU_SYSTEM_CODEC_VP9,
	VPU_SYSTEM_CODEC_WMV,

	VPU_SYSTEM_CODEC_MAX
};

//codec_id is from VideoDecoder_Codec_E codec in video_decoder.h
int parse_video_frame(void* file_handle, unsigned char* read_buffer, unsigned int buffer_size, enum VPU_SYSTEM_SUPPORT_CODEC codec_id, int* search_seqheader);

int scan_sequence_header(unsigned char* read_buffer, unsigned int buffer_size, enum VPU_SYSTEM_SUPPORT_CODEC codec_id);

//output_buffer needs to free
char* readUntilNewline(char* input_buffer, char** output_buffer, unsigned int* output_size);

char* readLineFromBuffer(char* input_buffer, char* outputBuffer, int buffer_size);

char* readCommandUntilNewline(char* input_buffer, char* command, int cmd_size, char* parameter, int param_size);

long get_time_us(void);

#endif //VPU_SCRIPT_SYSTEM_H