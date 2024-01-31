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


#ifndef TEST_COMMON_H
#define TEST_COMMON_H


#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#include <glib.h>
#include <glib/gprintf.h>


#define TEST_MAJOR_VER 						(1)
#define TEST_MINOR_VER 						(0)
#define TEST_PATACH_VER 					(0)

#define TEST_COMMON_PRINT 					(void)g_printf
#define TEST_ERROR_MSG 						TEST_COMMON_PRINT
#define TEST_TEMP_MSG 						TEST_COMMON_PRINT

#define TEST_DONOTHING(fmt, ...) 			;

#if 0
#define TEST_DETAIL_MSG 					TEST_COMMON_PRINT
#else
#define TEST_DETAIL_MSG(fmt, ...) 			;
#endif

#define TEST_COMMON_SHORT_STR 				(4)
#define TEST_COMMON_RESOLUTION_STR			(TEST_COMMON_SHORT_STR * 4)
#define TEST_COMMON_PATH_STR				(TEST_COMMON_SHORT_STR * 32)
#define TEST_COMMON_CMD_STR					(512)

#define TEST_COMMON_CNT 					(2)
#define TEST_COMMON_MAX_FRAME_CNT			(10)
#define TEST_COMMON_MAX_MEM_BUFFER_CNT 		(10)
#define TEST_COMMON_DEC_OUT_BUFFER_COUNT	(4)
#define TEST_COMMON_MIN_OUT_CNT_FOR_DIFF	(5)

#define TEST_MAX_READ_BUFFER_SIZE  			((1024 * 1024) * 5)
#define TEST_COMMON_DEFAULT_SLICE_SIZE  	(4 * 1024)

#define TEST_COMMON_MAX_MEM_BUFFER 			(10)
#define TEST_COMMON_30FPS 					(30)
#define TEST_COMMON_MAKE_MB 				(1024 * 1024)

#define TEST_COMMON_HD_WIDTH	 			(1280)
#define TEST_COMMON_HD_HEIGHT	 			(720)
#define TEST_COMMON_HD_BITRATES 			(6 * TEST_COMMON_MAKE_MB)

#define TEST_COMMON_FHD_WIDTH	 			(1920)
#define TEST_COMMON_FHD_HEIGHT	 			(1080)
#define TEST_COMMON_FHD_BITRATES 			(10 * TEST_COMMON_MAKE_MB)

#define TEST_COMMON_QHD_WIDTH	 			(2560)
#define TEST_COMMON_QHD_HEIGHT	 			(1440)
#define TEST_COMMON_QHD_BITRATES 			(14 * TEST_COMMON_MAKE_MB)

#define TEST_COMMON_UHD_WIDTH				(3840)
#define TEST_COMMON_UHD_HEIGHT	 			(2160)
#define TEST_COMMON_UHD_BITRATES 			(20 * TEST_COMMON_MAKE_MB)

#define TEST_COMMON_MAX_WIDTH				(8192)
#define TEST_COMMON_MAX_HEIGHT	 			(8192)

#define TEST_COMMON_YUV_FILE_PATH 			"/usr/share/test_contents/YUV_FILES"
#define TEST_COMMON_CODEC_FILE_PATH 		"/usr/share/test_contents/CODEC_FILES"
#define TEST_COMMON_ES_FILE_PATH 			"/usr/share/test_contents/ES_FILES"


typedef enum
{
	VID_Tester_Type_None = 0,
	VID_Tester_Type_H264_FHD,
	VID_Tester_Type_H265_FHD,
	VID_Tester_Type_H265_QHD,
	VID_Tester_Type_H265_UHD,
	VID_Tester_Type_JPEG_FHD,
	VID_Tester_Type_JPEG_UHD,
	VID_Tester_Type_JPEG_MAX_RES,
	VID_Tester_Type_Max
} VID_Tester_Type_E;


#endif	// TEST_COMMON_H