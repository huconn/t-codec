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

#ifndef DECODER_PLUGIN_VIDEO_H
#define DECODER_PLUGIN_VIDEO_H


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
#define DPV_SNPRINT 				(void)g_snprintf
#define DPV_PRINT 					(void)g_printf
#define DPV_DONOTHING(fmt, ...) 	;
#define DPV_CAST(tar, src) 			(void)memcpy((void*)&(tar), (void*)&(src), sizeof(void*))

#define DPV_FREE					(void)g_free
#define DPV_MALLOC					g_malloc
#define DPV_CALLOC					g_malloc0
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// logs
// --------------------------------------------------------------------------------------
#define DPV_DEBUG_MSG DPV_PRINT
#define DPV_ERROR_MSG DPV_PRINT
#define DPV_TEMP_MSG  DPV_PRINT

#define DPV_PRINT_DETAIL_INFO (0)

#if (DPV_PRINT_DETAIL_INFO == 1)
#define DPV_DETAIL_MSG 					(void)g_printf
#define DPV_FUNC_LOG() 					DPV_DETAIL_MSG("[%s(%d)] [FUNC] \n", __func__, __LINE__)
#define DPV_CHECK_LOG() 				DPV_DETAIL_MSG("[%s(%d)] [CHECK] \n", __func__, __LINE__)
#define DPV_CHECK_STR_LOG(val) 			DPV_DETAIL_MSG("[%s(%d)] [CHECK] %s \n", __func__, __LINE__, val)
#define DPV_CHECK_INT_LOG(val) 			DPV_DETAIL_MSG("[%s(%d)] [CHECK] %d \n", __func__, __LINE__, val)
#else
#define DPV_DETAIL_MSG(fmt, ...) 		;
#define DPV_FUNC_LOG(fmt, ...) 			;
#define DPV_CHECK_LOG(fmt, ...) 		;
#define DPV_CHECK_STR_LOG(fmt, ...) 	;
#define DPV_CHECK_INT_LOG(fmt, ...) 	;
#endif
// --------------------------------------------------------------------------------------

#define DPV_SHORT_STR_MAX 		(4)
#define DPV_NAME_STR_MAX 		(DPV_SHORT_STR_MAX * 2)
#define DPV_CMD_STR_MAX 		(DPV_NAME_STR_MAX * 4)
#define DPV_FILENAME_STR_MAX 	(256)

#define DPV_MJPEG			"mjpeg"
#define DPV_H264			"h264"
#define DPV_H265			"h265"
#define DPV_MPG2			"mpeg2"

#define DPV_ALIGN_LEN 		(4 * 1024)

#define DPV_PA				(0)
#define DPV_VA				(1)
#define DPV_K_VA			(2)

#define DPV_COMP_Y			(0)
#define DPV_COMP_U			(1)
#define DPV_COMP_V			(2)

#define DPV_DECODING_SUCCESS_FRAME		(0x0001)
#define DPV_DECODING_SUCCESS_FIELD		(0x0002)
#define DPV_DECODING_SUCCESS			(0x0003)
#define DPV_DECODING_SUCCESS_SKIPPED	(0x0004)
#define DPV_DISPLAY_OUTPUT_SUCCESS		(0x0010)
#define DPV_DECODED_OUTPUT_SUCCESS		(0x0020)
#define DPV_OUTPUT_SUCCESS				(0x0030)
#define DPV_DECODING_BUFFER_FULL		(0x0100)
#define DPV_RESOLUTION_CHANGED			(0x1000)


typedef enum
{
	DP_Video_ErrorNone 		= 0,
	DP_Video_ErrorInit 		= -1,
	DP_Video_ErrorDeinit 	= -2,
	DP_Video_ErrorDecode 	= -3,
	DP_Video_ErrorMoreData 	= -4
} DP_Video_Error_E;

typedef enum
{
	DP_Video_Config_None = 0,

	// Set
	DP_Video_Config_Set_ExtraFrameCount,
	DP_Video_Config_Set_SequenceHeader,
	DP_Video_Config_Set_ReleaseFrame,
	DP_Video_Config_Set_VPU_Clock,

	// Get
	DP_Video_Config_Get_ExtraFrameCount,
	DP_Video_Config_Get_CbCrInterleaveMode
} DP_Video_Config_E;

typedef struct
{
	int iExtraFrameCount;
} DP_Video_Config_ExtraFrameCount_T;

typedef struct
{
	bool bCbCrInterleaveMode;
} DP_Video_Config_CbCrInterleaveMode_T;

typedef struct
{
	int iDisplayIndex;
} DP_Video_Config_ReleaseFrame_T;

typedef struct
{
	int iStreamLen;
	unsigned char *pucStream;
} DP_Video_Config_SequenceHeader_T;

typedef struct
{
	int iDummy;
} DP_Video_Config_Init_T;


typedef struct
{
	unsigned long ulResult;

	int iIntanceIdx;
	bool bIsFrameOut;
	bool bIsInsertClock;
	bool bIsInsertFrame;

	int iDecodingStatus;
	int iOutputStatus;
	int iDecodedIdx;
	int iDispOutIdx;

	bool bField;
	bool bInterlaced;
	bool bOddFieldFirst;

	float fAspectRatio;

	unsigned int uiWidth;
	unsigned int uiHeight;

	unsigned int uiCropLeft;
	unsigned int uiCropRight;
	unsigned int uiCropTop;
	unsigned int uiCropBottom;
	unsigned int uiCropWidth;
	unsigned int uiCropHeight;

	unsigned char *pucDispOut[DPV_K_VA + 1][DPV_COMP_V + 1];
} DP_Video_DecodingResult_T;

#endif	// DECODER_PLUGIN_VIDEO_H