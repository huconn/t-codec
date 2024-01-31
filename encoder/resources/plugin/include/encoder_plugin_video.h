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

#ifndef TENCODER_PLUGIN_VIDEO_H
#define TENCODER_PLUGIN_VIDEO_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/mman.h>

#include <glib.h>
#include <glib/gprintf.h>


// --------------------------------------------------------------------------------------
// map some functions to avoid coverity warning
// --------------------------------------------------------------------------------------
#define TEPV_SNPRINT 				(void)g_snprintf
#define TEPV_PRINT 					(void)g_printf
#define TEPV_DONOTHING(fmt, ...) 	;
#define TEPV_CAST(tar, src) 		(void)memcpy((void*)&(tar), (void*)&(src), sizeof(void*))

#define TEPV_FREE					(void)g_free
#define TEPV_MALLOC					g_malloc
#define TEPV_CALLOC					g_malloc0
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// logs
// --------------------------------------------------------------------------------------
#define TEPV_DEBUG_MSG TEPV_PRINT
#define TEPV_ERROR_MSG TEPV_PRINT
#define TEPV_TEMP_MSG  TEPV_PRINT

#define TEPV_PRINT_DETAIL_INFO (0)

#if (TEPV_PRINT_DETAIL_INFO == 1)
#define TEPV_DETAIL_MSG 					(void)g_printf
#define TEPV_FUNC_LOG() 					TEPV_DETAIL_MSG("[%s(%d)] [FUNC] \n", __func__, __LINE__)
#define TEPV_CHECK_LOG() 				TEPV_DETAIL_MSG("[%s(%d)] [CHECK] \n", __func__, __LINE__)
#define TEPV_CHECK_STR_LOG(val) 			TEPV_DETAIL_MSG("[%s(%d)] [CHECK] %s \n", __func__, __LINE__, val)
#define TEPV_CHECK_INT_LOG(val) 			TEPV_DETAIL_MSG("[%s(%d)] [CHECK] %d \n", __func__, __LINE__, val)
#else
#define TEPV_DETAIL_MSG(fmt, ...) 		;
#define TEPV_FUNC_LOG(fmt, ...) 			;
#define TEPV_CHECK_LOG(fmt, ...) 		;
#define TEPV_CHECK_STR_LOG(fmt, ...) 	;
#define TEPV_CHECK_INT_LOG(fmt, ...) 	;
#endif
// --------------------------------------------------------------------------------------

#define TEPV_SHORT_STR_MAX 		(4)
#define TEPV_NAME_STR_MAX 		(TEPV_SHORT_STR_MAX * 2)
#define TEPV_CMD_STR_MAX 		(TEPV_NAME_STR_MAX * 4)
#define TEPV_FILENAME_STR_MAX 	(256)

#define TEPV_MJPEG			"mjpeg"
#define TEPV_H264			"h264"
#define TEPV_H265			"h265"

#define TEPV_ALIGN_LEN 		(4 * 1024)
#define TEPV_SEQ_HEADER_LEN	(100 * 1024)

#define TEPV_PA				(0)
#define TEPV_VA				(1)
#define TEPV_K_VA			(2)

#define TEPV_COMP_Y			(0)
#define TEPV_COMP_U			(1)
#define TEPV_COMP_V			(2)

#define TEPV_WIDTH_LIMIT	(50000)
#define TEPV_HEIGHT_LIMIT	(50000)


typedef enum
{
	EP_Video_ErrorNone 		= 0,
	EP_Video_ErrorInit 		= -1,
	EP_Video_ErrorDeinit 	= -2,
	EP_Video_ErrorHeader 	= -3,
	EP_Video_ErrorEncode 	= -4,
	EP_Video_ErrorMoreData 	= -5
} EP_Video_Error_E;

typedef enum
{
	EP_Video_FrameUnKnown = 0,
	EP_Video_FrameI,
	EP_Video_FrameP
} EP_Video_Frame_E;

typedef enum
{
	EP_Video_Config_None = 0,

	// Set
	EP_Video_Config_Set_SequenceHeader,

	// Get
	EP_Video_Config_Get_CbCrInterleaveMode
} EP_Video_Config_E;

typedef struct
{
	signed long long llTimeStamp;
	unsigned char *pucVideoBuffer[TEPV_K_VA + 1];
} EP_Video_Encode_Input_T;

typedef struct
{
	EP_Video_Frame_E eType;
	int iFrameSize;
	unsigned char *pucBuffer;
} EP_Video_Encode_Result_T;

typedef struct
{
	int iHeaderSize;
	unsigned char *pucBuffer;
} EP_Video_SequenceHeader_Result_T;

#endif	// TENCODER_PLUGIN_VIDEO_H