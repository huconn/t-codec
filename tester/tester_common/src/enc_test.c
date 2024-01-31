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
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#include "vid_test_tools.h"

#include "enc_test.h"


#define ENC_TEST_REPORT_FILE 	"VideoEncoder_TestReport.csv"
#define ENC_TEST_REPORT_DEPT 	"LFT"
#define ENC_TEST_REPORT_MODULE 	"Video Encoder"


const char *strEncodeExt[VideoEncoder_Codec_MAX] =
{
	"yuv",
	"h264",
	"h265",
	"mjpeg"
};


static void vet_check_init_deinit_memory(void);
static void *vet_running_test_thread(void *param);

static void vet_set_video_dump_files(VideoEncoder_Tester_T *ptTester, bool bIsClose);
static void vet_init_tester_data(VideoEncoder_Tester_T *ptTester);

static unsigned char *vet_read_data_from_file(VideoEncoder_Tester_T *ptTester, int *iBufferSize);
static int vet_make_black_frame(unsigned char *pcBuffer, int resolution, int size);
static unsigned char *vet_malloc_a_video_mem(int idx, int width, int height);


bool vet_auto_encoder_test(void)
{
	int iTestRet = 0;
	bool bError = false;

	TEST_COMMON_PRINT("----------------------------------------------------------------- \n");
	TEST_COMMON_PRINT("[%s(%d)] TEST VIDEO ENCODER \n", __func__, __LINE__);
	TEST_COMMON_PRINT("----------------------------------------------------------------- \n");

	if (access(TEST_COMMON_YUV_FILE_PATH, 0) == 0) {
		VideoEncoder_Tester_Init_T tTestInit = {0, };
		VideoEncoder_Tester_T tTester = {0, };

		#ifdef GET_CSV_REPORT_INCLUDE
		TUCR_Init_T tReportInit = {0, };
		TUCR_Handle_T *ptTUCRHandle = NULL;

		(void)g_snprintf(tReportInit.strReportFileName, 	TUCR_MAX_SHORT_STR_LEN, "%s", ENC_TEST_REPORT_FILE);
		(void)g_snprintf(tReportInit.strDept, 				TUCR_MAX_SHORT_STR_LEN, "%s", ENC_TEST_REPORT_DEPT);
		(void)g_snprintf(tReportInit.strModule, 			TUCR_MAX_SHORT_STR_LEN, "%s", ENC_TEST_REPORT_MODULE);

		ptTUCRHandle = tucr_init(&tReportInit);
		#endif

		tTestInit.iMaxCnt 		= 10;
		tTestInit.bIsDumpInput 	= false;
		tTestInit.bIsDumpOutput = false;

		tTestInit.eType = VID_Tester_Type_H264_FHD;
		tTestInit.iIndex = 0;
		bError = vet_run_test(&tTestInit, &tTester);
		iTestRet += (int)bError;
		#ifdef GET_CSV_REPORT_INCLUDE
		vt_write_a_result(ptTUCRHandle, "H264", "FHD", (int)bError);
		#endif

		tTestInit.eType = VID_Tester_Type_H265_QHD;
		tTestInit.iIndex = 1;
		bError = vet_run_test(&tTestInit, &tTester);
		iTestRet += (int)bError;
		#ifdef GET_CSV_REPORT_INCLUDE
		vt_write_a_result(ptTUCRHandle, "H265", "QHD", (int)bError);
		#endif

		tTestInit.eType = VID_Tester_Type_H265_UHD;
		tTestInit.iIndex = 2;
		bError = vet_run_test(&tTestInit, &tTester);
		iTestRet += (int)bError;
		#ifdef GET_CSV_REPORT_INCLUDE
		vt_write_a_result(ptTUCRHandle, "H265", "UHD", (int)bError);
		#endif

		tTestInit.iMaxCnt = 1;

		tTestInit.eType = VID_Tester_Type_JPEG_FHD;
		tTestInit.iIndex = 3;
		bError = vet_run_test(&tTestInit, &tTester);
		iTestRet += (int)bError;
		#ifdef GET_CSV_REPORT_INCLUDE
		vt_write_a_result(ptTUCRHandle, "JPEG", "FHD", (int)bError);
		#endif

		tTestInit.eType = VID_Tester_Type_JPEG_UHD;
		tTestInit.iIndex = 4;
		bError = vet_run_test(&tTestInit, &tTester);
		iTestRet += (int)bError;
		#ifdef GET_CSV_REPORT_INCLUDE
		vt_write_a_result(ptTUCRHandle, "JPEG", "UHD", (int)bError);
		#endif

		vet_set_video_memory(NULL, true);

		#ifdef GET_CSV_REPORT_INCLUDE
		tucr_deinit(ptTUCRHandle);
		#endif
	} else {
		TEST_ERROR_MSG("[%s][%d] OPEN FAILED: %s\n", __func__, __LINE__, TEST_COMMON_YUV_FILE_PATH);
		bError = true;
	}

	if (iTestRet != 0) {
		TEST_ERROR_MSG("[%s][%d] RET: %d \n", __func__, __LINE__, iTestRet);
		bError = true;
	}

	return bError;
}

void vet_init(VideoEncoder_Tester_T *ptTester)
{
	if (ptTester != NULL) {
		VideoEncoder_InitConfig_T tConfig = {0, };

		tConfig.eCodec 					= ptTester->eCodec;

		tConfig.iImageWidth 			= ptTester->iWidth;
		tConfig.iImageHeight 			= ptTester->iHeight;

		tConfig.iFrameRate 				= ptTester->iFrameRate;
		tConfig.iKeyFrameInterval 		= ptTester->iKeyFrameInterval;
		tConfig.iBitRate 				= ptTester->iBitRate;

		tConfig.iSliceMode 				= ptTester->iSliceMode;
		tConfig.iSliceSizeUnit 			= ptTester->iSliceSizeUnit;
		tConfig.iSliceSize 				= ptTester->iSliceSize;

		ptTester->ptVideoEncoder 		= VideoEncoder_Create();
		ptTester->eEncError 			= VideoEncoder_Init(ptTester->ptVideoEncoder, &tConfig);

		if (ptTester->eEncError == (VideoEncoder_Error_E)VideoEncoder_Error_None) {
			ptTester->eEncError = VideoEncoder_Set_Header(ptTester->ptVideoEncoder, &ptTester->tHeader);
		}

		if (ptTester->eEncError == (VideoEncoder_Error_E)VideoEncoder_Error_None) {
			ptTester->bIsInit = true;

		#if (USE_ENC_PERF_REPORT == 1)
			if ((ptTester->iMaxCnt > 0) && (ptTester->iMaxCnt <= TEST_COMMON_MAX_MEM_BUFFER_CNT))  {
				ptTester->ptEncStart 	= (long long *)g_malloc(sizeof(long long) * (unsigned int)(ptTester->iMaxCnt + 1));
				ptTester->ptEncEnd 		= (long long *)g_malloc(sizeof(long long) * (unsigned int)(ptTester->iMaxCnt + 1));
				ptTester->ptEncDiff 	= (long long *)g_malloc(sizeof(long long) * (unsigned int)(ptTester->iMaxCnt + 1));
			}
		#endif
			vet_set_video_dump_files(ptTester, false);
		} else {
			ptTester->bIsInit = false;
		}
	}
}

void vet_deinit(VideoEncoder_Tester_T *ptTester)
{
	if (ptTester != NULL) {
		if (ptTester->bIsInit == true) {
			TEST_DETAIL_MSG("[%s][%d] VideoEncoder_Deinit \n", __func__, __LINE__);
			(void)VideoEncoder_Deinit(ptTester->ptVideoEncoder);

			if (ptTester->ptVideoEncoder != NULL) {
				TEST_DETAIL_MSG("[%s][%d] VideoEncoder_Destroy \n", __func__, __LINE__);
				(void)VideoEncoder_Destroy(ptTester->ptVideoEncoder);
			}

		#if (USE_ENC_PERF_REPORT == 1)
			if ((ptTester->iMaxCnt > 0) && (ptTester->iMaxCnt <= TEST_COMMON_MAX_MEM_BUFFER_CNT))  {
				g_free(ptTester->ptEncStart);
				g_free(ptTester->ptEncEnd);
				g_free(ptTester->ptEncDiff);
			}
		#endif

			vet_set_video_dump_files(ptTester, true);
		}
	}
}

void vet_encode(VideoEncoder_Tester_T *ptTester)
{
	if (ptTester != NULL) {
		int iBufIdx = (int)ptTester->uiCnt % TEST_COMMON_MAX_MEM_BUFFER;
		int iBufSize = ((ptTester->iWidth * ptTester->iHeight) * 3) / 2;

		long long llEncodingStart = 0;
		long long llEncodingEnd = 0;
		long long llEncodingDiff = 0;

		VideoEncoder_Encode_Input_T tEncodeInput = {0, };
		VideoEncoder_Encode_Result_T tEncodeResult = {0, };

		llEncodingStart = vt_tools_get_time();

		if (ptTester->hEncInputFile != NULL) {
			tEncodeInput.pucVideoBuffer[VE_IN_Y] = vet_read_data_from_file(ptTester, &iBufSize);
		} else {
			tEncodeInput.pucVideoBuffer[VE_IN_Y] = ptTester->pucVideoBuffer[iBufIdx];
		}
		tEncodeInput.pucVideoBuffer[VE_IN_U] = NULL;
		tEncodeInput.pucVideoBuffer[VE_IN_V] = NULL;

		tEncodeInput.llTimeStamp += 33333;

		ptTester->eEncError = VideoEncoder_Encode(ptTester->ptVideoEncoder, &tEncodeInput, &tEncodeResult);
		llEncodingEnd = vt_tools_get_time();
		llEncodingDiff = llEncodingEnd - llEncodingStart;

	#if (USE_ENC_PERF_REPORT == 1)
		if ((ptTester->iMaxCnt > 0) && (ptTester->iMaxCnt <= TEST_COMMON_MAX_MEM_BUFFER_CNT))  {
			ptTester->ptEncStart[ptTester->uiCnt] = llEncodingStart;
			ptTester->ptEncEnd[ptTester->uiCnt] = llEncodingEnd;
			ptTester->ptEncDiff[ptTester->uiCnt] = llEncodingDiff;
		}
	#endif

		ptTester->llAccEncDiff += llEncodingDiff;
		ptTester->llAvrEncDiff = ptTester->llAccEncDiff / ((long long)ptTester->uiCnt + 1);

		if (ptTester->llMaxEnc < llEncodingDiff) {
			ptTester->llMaxEnc = llEncodingDiff;
		}

		if ((ptTester->llMinEnc > llEncodingDiff) || (ptTester->llMinEnc == 0)) {
			ptTester->llMinEnc = llEncodingDiff;
		}

	#if 0
		if ((ptTester->iMaxCnt == -1) && (((ptTester->uiCnt + 1U) % 300U) == 0U))
			TEST_COMMON_PRINT("[%s(%d)] %d: %ld Encoded(AVR.: %lldus) \n", __func__, __LINE__, ptTester->iTesterIdx, (ptTester->uiCnt + 1U), ptTester->llAvrEncDiff);
	#endif

		TEST_DETAIL_MSG("[%s(%d)] PERF: ENC(%u / %lld, %lld) \n", __func__, __LINE__, ptTester->uiCnt, ptTester->llAccEncDiff, ptTester->llAvrEncDiff);

		ptTester->uiCnt++;

		if (ptTester->eEncError != (VideoEncoder_Error_E)VideoEncoder_Error_None) {
			TEST_ERROR_MSG("[%s][%d] eEncError: %d\n", __func__, __LINE__, ptTester->eEncError);
		} else {
			TEST_DETAIL_MSG("[%s(%d)] IN BUFF: %d, %d \n", __func__, __LINE__, iBufIdx, iBufSize);
			vet_write_video_dump(ptTester, true, tEncodeInput.pucVideoBuffer[VE_IN_Y], iBufSize);
			TEST_DETAIL_MSG("[%s(%d)] OUT BUFF: %d \n", __func__, __LINE__, tEncodeResult.iFrameSize);
			vet_write_video_dump(ptTester, false, tEncodeResult.pucBuffer, tEncodeResult.iFrameSize);
		}

		if (ptTester->hEncInputFile != NULL) {
			g_free(tEncodeInput.pucVideoBuffer[VE_IN_Y]);
		}
	}
}

void vet_set_tester(VideoEncoder_Tester_T *ptTester, VideoEncoder_Tester_Init_T *ptTestInit)
{
	if (ptTester != NULL) {
		vet_init_tester_data(ptTester);

		ptTester->eTest 		= (VID_Tester_Type_E)ptTestInit->eType;
		ptTester->iTesterIdx 	= ptTestInit->iIndex;
		ptTester->iMaxCnt 		= ptTestInit->iMaxCnt;
		ptTester->bIsRunning 	= true;

		//ptTester->bIsInputDumpOn = true;

		if (ptTester->eTest == VID_Tester_Type_None) {
			ptTester->eCodec 					= (VideoEncoder_Codec_E)VideoEncoder_Codec_None;

			ptTester->iWidth 					= TEST_COMMON_HD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_HD_HEIGHT;

			ptTester->iFrameRate 				= TEST_COMMON_30FPS;
			ptTester->iKeyFrameInterval 		= TEST_COMMON_30FPS;
			ptTester->iBitRate 					= TEST_COMMON_FHD_BITRATES;

			ptTester->iSliceMode 				= (int)VideoEncoder_SliceMode_Single;
			ptTester->iSliceSizeUnit 			= (int)VideoEncoder_SliceSizeUnit_Byte;
			ptTester->iSliceSize 				= TEST_COMMON_DEFAULT_SLICE_SIZE;
		} else if (ptTester->eTest == VID_Tester_Type_H264_FHD) {
			ptTester->eCodec 					= (VideoEncoder_Codec_E)VideoEncoder_Codec_H264;

			ptTester->iWidth 					= TEST_COMMON_FHD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_FHD_HEIGHT;

			ptTester->iFrameRate 				= TEST_COMMON_30FPS;
			ptTester->iKeyFrameInterval 		= TEST_COMMON_30FPS;
			ptTester->iBitRate 					= TEST_COMMON_FHD_BITRATES;

			ptTester->iSliceMode 				= (int)VideoEncoder_SliceMode_Single;
			ptTester->iSliceSizeUnit 			= (int)VideoEncoder_SliceSizeUnit_Byte;
			ptTester->iSliceSize 				= TEST_COMMON_DEFAULT_SLICE_SIZE;
		} else if (ptTester->eTest == VID_Tester_Type_H265_FHD) {
			ptTester->eCodec 					= (VideoEncoder_Codec_E)VideoEncoder_Codec_H265;

			ptTester->iWidth 					= TEST_COMMON_FHD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_FHD_HEIGHT;

			ptTester->iFrameRate 				= TEST_COMMON_30FPS;
			ptTester->iKeyFrameInterval 		= TEST_COMMON_30FPS;
			ptTester->iBitRate 					= TEST_COMMON_FHD_BITRATES;

			ptTester->iSliceMode 				= (int)VideoEncoder_SliceMode_Single;
			ptTester->iSliceSizeUnit 			= (int)VideoEncoder_SliceSizeUnit_Byte;
			ptTester->iSliceSize 				= TEST_COMMON_DEFAULT_SLICE_SIZE;
		} else if (ptTester->eTest == VID_Tester_Type_H265_QHD) {
			ptTester->eCodec 					= (VideoEncoder_Codec_E)VideoEncoder_Codec_H265;

			ptTester->iWidth 					= TEST_COMMON_QHD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_QHD_HEIGHT;

			ptTester->iFrameRate 				= TEST_COMMON_30FPS;
			ptTester->iKeyFrameInterval 		= TEST_COMMON_30FPS;
			ptTester->iBitRate 					= TEST_COMMON_QHD_BITRATES;

			ptTester->iSliceMode 				= (int)VideoEncoder_SliceMode_Single;
			ptTester->iSliceSizeUnit 			= (int)VideoEncoder_SliceSizeUnit_Byte;
			ptTester->iSliceSize 				= TEST_COMMON_DEFAULT_SLICE_SIZE;
		} else if (ptTester->eTest == VID_Tester_Type_H265_UHD) {
			ptTester->eCodec 					= (VideoEncoder_Codec_E)VideoEncoder_Codec_H265;

			ptTester->iWidth 					= TEST_COMMON_UHD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_UHD_HEIGHT;

			ptTester->iFrameRate 				= TEST_COMMON_30FPS;
			ptTester->iKeyFrameInterval 		= TEST_COMMON_30FPS;
			ptTester->iBitRate 					= TEST_COMMON_UHD_BITRATES;

			ptTester->iSliceMode 				= (int)VideoEncoder_SliceMode_Single;
			ptTester->iSliceSizeUnit 			= (int)VideoEncoder_SliceSizeUnit_Byte;
			ptTester->iSliceSize 				= TEST_COMMON_DEFAULT_SLICE_SIZE;
		} else if (ptTester->eTest == VID_Tester_Type_JPEG_FHD) {
			ptTester->eCodec 					= (VideoEncoder_Codec_E)VideoEncoder_Codec_JPEG;

			ptTester->iWidth 					= TEST_COMMON_FHD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_FHD_HEIGHT;

			ptTester->iFrameRate 				= TEST_COMMON_30FPS;
			ptTester->iKeyFrameInterval 		= TEST_COMMON_30FPS;
			ptTester->iBitRate 					= TEST_COMMON_FHD_BITRATES;

			ptTester->iSliceMode 				= (int)VideoEncoder_SliceMode_Single;
			ptTester->iSliceSizeUnit 			= (int)VideoEncoder_SliceSizeUnit_Byte;
			ptTester->iSliceSize 				= TEST_COMMON_DEFAULT_SLICE_SIZE;
		}  else if (ptTester->eTest == VID_Tester_Type_JPEG_UHD) {
			ptTester->eCodec 					= (VideoEncoder_Codec_E)VideoEncoder_Codec_JPEG;

			ptTester->iWidth 					= TEST_COMMON_UHD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_UHD_HEIGHT;

			ptTester->iFrameRate 				= TEST_COMMON_30FPS;
			ptTester->iKeyFrameInterval 		= TEST_COMMON_30FPS;
			ptTester->iBitRate 					= TEST_COMMON_UHD_BITRATES;

			ptTester->iSliceMode 				= (int)VideoEncoder_SliceMode_Single;
			ptTester->iSliceSizeUnit 			= (int)VideoEncoder_SliceSizeUnit_Byte;
			ptTester->iSliceSize 				= TEST_COMMON_DEFAULT_SLICE_SIZE;
		} else {
			TEST_DONOTHING();
		}

		if (strlen(ptTestInit->strInputFileName) > 3) {
			(void)g_snprintf(ptTester->strEncInputFileName, TEST_COMMON_PATH_STR, "%s", ptTestInit->strInputFileName);
			TEST_DETAIL_MSG("[%s(%d)] FILE(%d): %s \n", __func__, __LINE__, ptTestInit->iIndex, ptTester->strEncInputFileName);
		}
	}
}

void vet_write_video_dump(const VideoEncoder_Tester_T *ptTester, bool bIsInput, const unsigned char *pucBuffer, int iBufferSize)
{
#if (USE_ENC_DUMP == 1)
	if ((pucBuffer != NULL) && (iBufferSize > 0)) {
		if ((bIsInput == true) && (ptTester->bIsInputDumpOn == true)) {
			(void)fwrite(pucBuffer, 1U, (size_t)iBufferSize, ptTester->hInputFile);
		} else if ((bIsInput == false) && (ptTester->bIsOutputDumpOn == true)) {
			(void)fwrite(pucBuffer, 1U, (size_t)iBufferSize, ptTester->hOutputFile);

			(void)fwrite(&iBufferSize, 1U, sizeof(int), ptTester->hOutputSizeFile);
			(void)fwrite(pucBuffer, 1U, (size_t)iBufferSize, ptTester->hOutputSizeFile);
		} else {
			TEST_DONOTHING();
		}
	}
#endif
}

void vet_report_test_result(const VideoEncoder_Tester_T *ptTester)
{
	if (ptTester != NULL) {
		if (ptTester->eEncError != (VideoEncoder_Error_E)VideoEncoder_Error_None) {
			TEST_ERROR_MSG("[%s(%d)] ERROR: ENCODER \n", __func__, __LINE__);
		} else {
		#if (USE_ENC_PERF_REPORT == 1)
			if ((ptTester->iMaxCnt > 0) && (ptTester->iMaxCnt <= TEST_COMMON_MAX_MEM_BUFFER_CNT))  {
				char strPerfReport[TEST_COMMON_CMD_STR] = {0, };
				char strSystemCmd[TEST_COMMON_CMD_STR] = {0, };

				(void)g_snprintf(strPerfReport, TEST_COMMON_CMD_STR, "vet_perf_report_%d_%d.csv", ptTester->iTesterIdx, ptTester->eTest);
				TEST_COMMON_PRINT("[%s(%d)] strPerfReport: %s \n", __func__, __LINE__, strPerfReport);

				for (unsigned int i = 0; i < ptTester->uiCnt; i++) {
					(void)g_snprintf(strSystemCmd, TEST_COMMON_CMD_STR, "echo %d, %lld, %lld, %lld >> %s", i, ptTester->ptEncDiff[i], ptTester->ptEncStart[i], ptTester->ptEncEnd[i], strPerfReport);
					TEST_DETAIL_MSG("[%s(%d)] CMD: %s \n", __func__, __LINE__, strSystemCmd);
					(void)system(strSystemCmd);
				}
			}
		#endif
		}
	}
}

int vet_is_data_available(const VideoEncoder_Tester_T *ptTester)
{
	int iRet = -1;

	if (ptTester != NULL) {
		if ((ptTester->iWidth > 0) && (ptTester->iHeight > 0)) {
			iRet = 1;
		}
	}

	return iRet;
}

void vet_set_video_memory(VideoEncoder_Tester_T *ptTester, bool bIsFree)
{
	static bool bIsUHDSet = false;
	static bool bIsQHDSet = false;
	static bool bIsFHDSet = false;
	static bool bIsHDSet = false;

	static unsigned char *pucHDVideoBuffer[TEST_COMMON_MAX_MEM_BUFFER_CNT];
	static unsigned char *pucFHDVideoBuffer[TEST_COMMON_MAX_MEM_BUFFER_CNT];
	static unsigned char *pucQHDVideoBuffer[TEST_COMMON_MAX_MEM_BUFFER_CNT];
	static unsigned char *pucUHDVideoBuffer[TEST_COMMON_MAX_MEM_BUFFER_CNT];

	if (ptTester != NULL) {
		TEST_DETAIL_MSG("[%s(%d)] START: %d \n", __func__, __LINE__, ptTester->iTesterIdx);

		if ((ptTester->iWidth == TEST_COMMON_UHD_WIDTH) && (ptTester->iHeight == TEST_COMMON_UHD_HEIGHT)) {
			TEST_DETAIL_MSG("[%s(%d)] SET UHD MEMORY: %d \n", __func__, __LINE__, ptTester->iTesterIdx);

			if (bIsUHDSet == false) {
				TEST_DETAIL_MSG("[%s(%d)] INIT UHD MEMORY \n", __func__, __LINE__);

				for (int i = 0; i < TEST_COMMON_MAX_MEM_BUFFER_CNT; i++) {
					pucUHDVideoBuffer[i] = vet_malloc_a_video_mem((i + 1), TEST_COMMON_UHD_WIDTH, TEST_COMMON_UHD_HEIGHT);
				}

				bIsUHDSet = true;
			}

			for (int i = 0; i < TEST_COMMON_MAX_MEM_BUFFER_CNT; i++) {
				ptTester->pucVideoBuffer[i] = pucUHDVideoBuffer[i];
			}
		} else if ((ptTester->iWidth == TEST_COMMON_QHD_WIDTH) && (ptTester->iHeight == TEST_COMMON_QHD_HEIGHT)) {
			TEST_DETAIL_MSG("[%s(%d)] SET QHD MEMORY: %d \n", __func__, __LINE__, ptTester->iTesterIdx);

			if (bIsQHDSet == false) {
				TEST_DETAIL_MSG("[%s(%d)] INIT QHD MEMORY \n", __func__, __LINE__);

				for (int i = 0; i < TEST_COMMON_MAX_MEM_BUFFER_CNT; i++) {
					pucQHDVideoBuffer[i] = vet_malloc_a_video_mem((i + 1), TEST_COMMON_QHD_WIDTH, TEST_COMMON_QHD_HEIGHT);
				}

				bIsQHDSet = true;
			}

			for (int i = 0; i < TEST_COMMON_MAX_MEM_BUFFER_CNT; i++) {
				ptTester->pucVideoBuffer[i] = pucQHDVideoBuffer[i];
			}
		} else if ((ptTester->iWidth == TEST_COMMON_FHD_WIDTH) && (ptTester->iHeight == TEST_COMMON_FHD_HEIGHT)) {
			TEST_DETAIL_MSG("[%s(%d)] SET FHD MEMORY: %d \n", __func__, __LINE__, ptTester->iTesterIdx);

			if (bIsFHDSet == false) {
				TEST_DETAIL_MSG("[%s(%d)] INIT FHD MEMORY \n", __func__, __LINE__);

				for (int i = 0; i < TEST_COMMON_MAX_MEM_BUFFER_CNT; i++) {
					pucFHDVideoBuffer[i] = vet_malloc_a_video_mem((i + 1), TEST_COMMON_FHD_WIDTH, TEST_COMMON_FHD_HEIGHT);
				}

				bIsFHDSet = true;
			}

			for (int i = 0; i < TEST_COMMON_MAX_MEM_BUFFER_CNT; i++) {
				ptTester->pucVideoBuffer[i] = pucFHDVideoBuffer[i];
			}
		} else if ((ptTester->iWidth == TEST_COMMON_HD_WIDTH) && (ptTester->iHeight == TEST_COMMON_HD_HEIGHT)) {
			TEST_DETAIL_MSG("[%s(%d)] SET HD MEMORY: %d \n", __func__, __LINE__, ptTester->iTesterIdx);

			if (bIsHDSet == false) {
				TEST_DETAIL_MSG("[%s(%d)] INIT HD MEMORY \n", __func__, __LINE__);

				for (int i = 0; i < TEST_COMMON_MAX_MEM_BUFFER_CNT; i++) {
					pucHDVideoBuffer[i] = vet_malloc_a_video_mem((i + 1), TEST_COMMON_HD_WIDTH, TEST_COMMON_HD_HEIGHT);
				}

				bIsHDSet = true;
			}

			for (int i = 0; i < TEST_COMMON_MAX_MEM_BUFFER_CNT; i++) {
				ptTester->pucVideoBuffer[i] = pucHDVideoBuffer[i];
			}
		} else {
			TEST_DONOTHING();
		}
	} else {
		if (bIsFree == true) {
			TEST_DETAIL_MSG("[%s(%d)] DEINIT MEMORIES \n", __func__, __LINE__);

			for (int i = 0; i < TEST_COMMON_MAX_MEM_BUFFER_CNT; i++) {
				if (bIsHDSet == true) {
					TEST_DETAIL_MSG("[%s(%d)] pucHDVideoBuffer: %d \n", __func__, __LINE__, i);
					g_free(pucHDVideoBuffer[i]);
					pucHDVideoBuffer[i] = NULL;
				}

				if (bIsFHDSet == true) {
					TEST_DETAIL_MSG("[%s(%d)] pucFHDVideoBuffer: %d \n", __func__, __LINE__, i);
					g_free(pucFHDVideoBuffer[i]);
					pucFHDVideoBuffer[i] = NULL;
				}

				if (bIsQHDSet == true) {
					TEST_DETAIL_MSG("[%s(%d)] pucQHDVideoBuffer: %d \n", __func__, __LINE__, i);
					g_free(pucQHDVideoBuffer[i]);
					pucQHDVideoBuffer[i] = NULL;
				}

				if (bIsUHDSet == true) {
					TEST_DETAIL_MSG("[%s(%d)] pucUHDVideoBuffer: %d \n", __func__, __LINE__, i);
					g_free(pucUHDVideoBuffer[i]);
					pucUHDVideoBuffer[i] = NULL;
				}
			}

			bIsHDSet = false;
			bIsFHDSet = false;
			bIsQHDSet = false;
			bIsUHDSet = false;
			TEST_DETAIL_MSG("[%s(%d)] DEINIT MEMORIES \n", __func__, __LINE__);
		} else {
			TEST_DETAIL_MSG("[%s(%d)] INIT MEMORIES \n", __func__, __LINE__);

			for (int i = 0; i < TEST_COMMON_MAX_MEM_BUFFER_CNT; i++) {
				if (bIsHDSet == false) {
					pucHDVideoBuffer[i] = vet_malloc_a_video_mem((i + 1), TEST_COMMON_HD_WIDTH, TEST_COMMON_HD_HEIGHT);
					bIsHDSet = true;
				}

				if (bIsFHDSet == false) {
					pucFHDVideoBuffer[i] = vet_malloc_a_video_mem((i + 1), TEST_COMMON_FHD_WIDTH, TEST_COMMON_FHD_HEIGHT);
					bIsFHDSet = true;
				}

				if (bIsQHDSet == false) {
					pucQHDVideoBuffer[i] = vet_malloc_a_video_mem((i + 1), TEST_COMMON_QHD_WIDTH, TEST_COMMON_QHD_HEIGHT);
					bIsQHDSet = true;
				}

				if (bIsUHDSet == false) {
					pucUHDVideoBuffer[i] = vet_malloc_a_video_mem((i + 1), TEST_COMMON_UHD_WIDTH, TEST_COMMON_UHD_HEIGHT);
					bIsUHDSet = true;
				}
			}
		}
	}
}

static unsigned char *vet_read_data_from_file(VideoEncoder_Tester_T *ptTester, int *iBufferSize)
{
	int i = 0;
	int iMaxTry = 2;
	unsigned char *pucBuffer = NULL;

	for (i = 0; i < iMaxTry; i++) {
		size_t readbyte = 0U;
		int fileSize = ptTester->iWidth * ptTester->iHeight * 3 / 2;

		pucBuffer = g_malloc0((size_t)fileSize);
		readbyte = fread(pucBuffer, 1U, (size_t)fileSize, ptTester->hEncInputFile);
		*iBufferSize = (int)readbyte;

		if (readbyte > 0U) {
			break;
		} else {
			if (pucBuffer != NULL) {
				g_free(pucBuffer);
				pucBuffer = NULL;
			}

			//TEST_ERROR_MSG("[%s(%d)] RETRY: Read(%d)  \n", __func__, __LINE__, i);
			(void)fseek(ptTester->hEncInputFile, 0L, SEEK_SET);
		}
	}

	return pucBuffer;
}

static void vet_init_tester_data(VideoEncoder_Tester_T *ptTester)
{
	if (ptTester != NULL) {
		ptTester->uiCnt 					= 0U;
		ptTester->iTesterIdx 				= 0;
		ptTester->eEncError 				= (VideoEncoder_Error_E)VideoEncoder_Error_None;
		ptTester->eCodec 					= (VideoEncoder_Codec_E)0;
		ptTester->iWidth 					= -1;
		ptTester->iHeight 					= -1;
		ptTester->iFrameRate 				= -1;
		ptTester->iKeyFrameInterval 		= -1;
		ptTester->iBitRate 					= -1;
		ptTester->iSliceMode 				= -1;
		ptTester->iSliceSizeUnit 			= -1;
		ptTester->iSliceSize 				= -1;

		ptTester->bIsInputDumpOn 			= false;
		ptTester->bIsOutputDumpOn 			= false;

		ptTester->bIsRunning 				= false;
		ptTester->bIsInit	 				= false;

		ptTester->llMinEnc 					= 0;
		ptTester->llMaxEnc 					= 0;
		ptTester->llAccEncDiff 				= 0;
		ptTester->llAvrEncDiff 				= 0;
	}
}

static void *vet_running_test_thread(void *param)
{
	VideoEncoder_Tester_T *ptTester = (VideoEncoder_Tester_T *)param;

	if (ptTester != NULL) {
		vet_init(ptTester);

		if (ptTester->eEncError == (VideoEncoder_Error_E)VideoEncoder_Error_None) {
			if (strlen(ptTester->strEncInputFileName) > 3) {
				TEST_DETAIL_MSG("[%s(%d)] OPEN: %s \n", __func__, __LINE__, ptTester->strEncInputFileName);
				ptTester->hEncInputFile = fopen(ptTester->strEncInputFileName, "r");
			} else {
				ptTester->hEncInputFile = NULL;
			}

			vet_write_video_dump(ptTester, false, ptTester->tHeader.pucBuffer, ptTester->tHeader.iHeaderSize);

			while (ptTester->bIsRunning == true) {
				if ((ptTester->uiCnt >= (unsigned int)ptTester->iMaxCnt) && (ptTester->iMaxCnt != -1)) {
					ptTester->bIsRunning = false;
				} else {
					vet_encode(ptTester);

					if (ptTester->eEncError != (VideoEncoder_Error_E)VideoEncoder_Error_None) {
						TEST_ERROR_MSG("[%s(%d)] ERROR: %d \n", __func__, __LINE__, (int)ptTester->eEncError);
						ptTester->bIsRunning = false;
					}
				}
			}
		}

		vet_report_test_result(ptTester);

		if (ptTester->hEncInputFile != NULL) {
			TEST_DETAIL_MSG("[%s(%d)] CLOSE: %s \n", __func__, __LINE__, ptTester->strEncInputFileName);
			fclose(ptTester->hEncInputFile);
		}
		vet_deinit(ptTester);
	}

	return NULL;
}

bool vet_run_test(VideoEncoder_Tester_Init_T *ptTestInit, VideoEncoder_Tester_T *ptTester)
{
	bool bError = false;

	vet_set_tester(ptTester, ptTestInit);

	if (strlen(ptTester->strEncInputFileName) < 3) {
		vet_set_video_memory(ptTester, false);
	}

	(void)pthread_create(&ptTester->hEncThread, NULL, vet_running_test_thread, ptTester);
	ptTester->bIsRunning = true;

	(void)usleep(20 * 1000);

	if (vet_is_data_available(ptTester) == 1) {
		(void)pthread_join(ptTester->hEncThread, NULL);
	}

	if (ptTester->eEncError != (VideoEncoder_Error_E)VideoEncoder_Error_None) {
		bError = true;
	}

	TEST_COMMON_PRINT("[%s(%d)] TEST %d(%d): %u Frames \n", __func__, __LINE__, ptTestInit->iIndex, (int)bError, ptTester->uiCnt);
	TEST_COMMON_PRINT("[%s(%d)]   - Index: %d, Type: %d  \n", __func__, __LINE__, ptTestInit->iMaxCnt, (int)ptTestInit->eType);
	TEST_COMMON_PRINT("[%s(%d)]   - MAX. Encoding Time: %lldus \n", __func__, __LINE__, ptTester->llMaxEnc);
	TEST_COMMON_PRINT("[%s(%d)]   - MIN. Encoding Time: %lldus \n", __func__, __LINE__, ptTester->llMinEnc);
	TEST_COMMON_PRINT("[%s(%d)]   - AVR. Encoding Time: %lldus \n", __func__, __LINE__, ptTester->llAvrEncDiff);

	return bError;
}

static void vet_check_init_deinit_memory(void)
{
	unsigned int uiCnt = 0U;

	VideoEncoder_Tester_T tTester = {0, };
	VideoEncoder_Tester_T *ptTester = &tTester;
	VideoEncoder_Tester_Init_T tTestInit = {0, };

	tTestInit.eType = VID_Tester_Type_None;
	vet_set_tester(ptTester, &tTestInit);

	while (1) {
		if (uiCnt > 1000U) {
			break;
		} else {
			TEST_COMMON_PRINT("[%s(%d)] vet_init: %u \n", __func__, __LINE__, uiCnt);
			vet_init(ptTester);

			(void)usleep(500 * 1000);

			TEST_DETAIL_MSG("[%s(%d)] vet_deinit: %u \n", __func__, __LINE__, uiCnt);
			vet_deinit(ptTester);

			uiCnt++;
		}
	}
}

static void vet_set_video_dump_files(VideoEncoder_Tester_T *ptTester, bool bIsClose)
{
#if (USE_ENC_DUMP == 1)
	if (ptTester != NULL) {
		if ((ptTester->bIsInputDumpOn == true) || (ptTester->bIsOutputDumpOn == true)) {
			int iTmp = (int)ptTester->eCodec;

			if (iTmp >= 0) {
				if (bIsClose == false) {
					TEST_DETAIL_MSG("[%s(%d)] OPEN DUMP FILE(S): %d \n", __func__, __LINE__, ptTester->iTesterIdx);

					if (ptTester->bIsInputDumpOn == true) {
						(void)g_snprintf(ptTester->strInputFileName, TEST_COMMON_PATH_STR, "vet_input_%d_%d_%dX%d.%s", ptTester->iTesterIdx, ptTester->eTest, ptTester->iWidth, ptTester->iHeight, strEncodeExt[0]);
						ptTester->hInputFile = fopen(ptTester->strInputFileName, "wb");

						if (ptTester->hInputFile == NULL) {
							TEST_ERROR_MSG("[%s(%d)] ERROR: %s \n", __func__, __LINE__, ptTester->strInputFileName);
						} else {
							TEST_DETAIL_MSG("[%s(%d)]   - %s \n", __func__, __LINE__, ptTester->strInputFileName);
						}
					}

					if (ptTester->bIsOutputDumpOn == true) {
						(void)g_snprintf(ptTester->strOutputSizeFileName, TEST_COMMON_PATH_STR, "vet_output_%d_%d_%dX%d_with_size.%s", ptTester->iTesterIdx, ptTester->eTest, ptTester->iWidth, ptTester->iHeight, strEncodeExt[iTmp]);
						ptTester->hOutputSizeFile = fopen(ptTester->strOutputSizeFileName, "wb");

						if (ptTester->hOutputSizeFile == NULL) {
							TEST_ERROR_MSG("[%s(%d)] ERROR: %s \n", __func__, __LINE__, ptTester->strOutputSizeFileName);
						} else {
							TEST_DETAIL_MSG("[%s(%d)]   - %s \n", __func__, __LINE__, ptTester->strOutputSizeFileName);
						}

						(void)g_snprintf(ptTester->strOutputFileName, TEST_COMMON_PATH_STR, "vet_output_%d_%d_%dX%d.%s", ptTester->iTesterIdx, ptTester->eTest, ptTester->iWidth, ptTester->iHeight, strEncodeExt[iTmp]);
						ptTester->hOutputFile = fopen(ptTester->strOutputFileName, "wb");

						if (ptTester->hOutputFile == NULL) {
							TEST_ERROR_MSG("[%s(%d)] ERROR: %s \n", __func__, __LINE__, ptTester->strOutputFileName);
						} else {
							TEST_DETAIL_MSG("[%s(%d)]   - %s \n", __func__, __LINE__, ptTester->strOutputFileName);
						}
					}
				}

				if (bIsClose == true) {
					TEST_DETAIL_MSG("[%s(%d)] CLOSE DUMP FILE(S): %d \n", __func__, __LINE__, ptTester->iTesterIdx);

					if (ptTester->bIsInputDumpOn == true) {
						TEST_DETAIL_MSG("[%s(%d)]   - %s \n", __func__, __LINE__, ptTester->strInputFileName);
						(void)fclose(ptTester->hInputFile);
					}

					if (ptTester->bIsOutputDumpOn == true) {
						TEST_DETAIL_MSG("[%s(%d)]   - %s \n", __func__, __LINE__, ptTester->strOutputFileName);
						(void)fclose(ptTester->hOutputFile);

						TEST_DETAIL_MSG("[%s(%d)]   - %s \n", __func__, __LINE__, ptTester->strOutputSizeFileName);
						(void)fclose(ptTester->hOutputSizeFile);
					}
				}
			}
		}

		TEST_DETAIL_MSG("[%s(%d)] DONE: %d \n", __func__, __LINE__, ptTester->iTesterIdx);
	}
#endif
}

static unsigned char *vet_malloc_a_video_mem(int idx, int width, int height)
{
	unsigned char *retPtr = NULL;
	char resolutionStr[TEST_COMMON_RESOLUTION_STR];

	bool bIsRun = false;

	if (width == TEST_COMMON_UHD_WIDTH) {
		(void)g_snprintf(resolutionStr, TEST_COMMON_SHORT_STR, "%s", "UHD");
		bIsRun = true;
	} else if (width == TEST_COMMON_QHD_WIDTH) {
		(void)g_snprintf(resolutionStr, TEST_COMMON_SHORT_STR, "%s", "QHD");
		bIsRun = true;
	} else if (width == TEST_COMMON_FHD_WIDTH) {
		(void)g_snprintf(resolutionStr, TEST_COMMON_SHORT_STR, "%s", "FHD");
		bIsRun = true;
	} else if (width == TEST_COMMON_HD_WIDTH) {
		(void)g_snprintf(resolutionStr, TEST_COMMON_SHORT_STR, "%s", "HD");
		bIsRun = true;
	} else {
		TEST_ERROR_MSG("[%s][%d] NOT SUPPORTED: %d X %d \n", __func__, __LINE__, width, height);
	}

	if (bIsRun == true) {
		char YUVFileName[TEST_COMMON_PATH_STR];
		FILE *hYUVFile = NULL;

		int resolution = width * height;

		if (resolution > 0) {
			int frameSize = (resolution * 3) / 2;
			int bufferSize = frameSize;

			retPtr = (unsigned char *)g_malloc(sizeof(unsigned char) * (unsigned int)bufferSize);

			(void)g_snprintf(YUVFileName, TEST_COMMON_PATH_STR, "%s/%d_%s.yuv", TEST_COMMON_YUV_FILE_PATH, idx, resolutionStr);

			hYUVFile = fopen(YUVFileName, "rb");

			if (hYUVFile != NULL) {
				while (1) {
					size_t readbyte = fread(retPtr, (size_t)bufferSize, 1, hYUVFile);

					if (readbyte == 0U) {
						break;
					}
				}

				(void)fclose(hYUVFile);
			} else {
				TEST_ERROR_MSG("[%s][%d] OPEN FAILED: %s(%d)\n", __func__, __LINE__, YUVFileName, idx);
				(void)vet_make_black_frame(retPtr, resolution, frameSize);
			}
		}
	}

	return retPtr;
}

static int vet_make_black_frame(unsigned char *pcBuffer, int resolution, int size)
{
	int ret = -1;

	if (pcBuffer != NULL) {
		unsigned int uiYOffset = 0U;

		if (resolution > 0) {
			uiYOffset = (unsigned int)resolution;

			if (memset(pcBuffer, 0x00, (size_t)uiYOffset) == NULL) {
				ret = -1;
			} else {
				long lYBufferSize = size - (long)uiYOffset;

				if (lYBufferSize < LONG_MAX) {
					void *pcYBuffer = pcBuffer;

					for (unsigned int uiYOffsetIdx = 0; uiYOffsetIdx < uiYOffset; uiYOffsetIdx++) {
						pcYBuffer++;
					}

					if (memset(pcYBuffer, 0x80, (size_t)lYBufferSize) == NULL) {
						ret = -1;
					} else {
						ret = 0;
					}
				}
			}
		}
	}

	return ret;
}