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

#include <glib.h>
#include <glib/gprintf.h>

#include "video_decoder.h"

#include "test_common.h"
#include "vid_test_tools.h"
#include "dec_test.h"


#define DEC_TEST_REPORT_FILE 	"VideoDecoder_TestReport.csv"
#define DEC_TEST_REPORT_DEPT 	"LFT"
#define DEC_TEST_REPORT_MODULE 	"Video Decoder"


const char *strDecodeExt[VideoDecoder_Codec_Max] =
{
	"yuv",
	"mpeg2",
	"mpeg4",
	"h263",
	"h264",
	"h265",
	"vp9",
	"vp8",
	"jpeg",
	"wmv",
	"-"
};


static void vdt_init_tester_data(VideoDecoder_Tester_T *ptTester);
static void vdt_set_video_dump_files(VideoDecoder_Tester_T *ptTester, bool bIsClose);
static int vdt_is_data_available(const VideoDecoder_Tester_T *ptTester);
static void *vdt_running_test_thread(void *param);


bool vdt_auto_decoder_test(void)
{
	int iTestRet = 0;
	bool bError = false;

	TEST_COMMON_PRINT("----------------------------------------------------------------- \n");
	TEST_COMMON_PRINT("[%s(%d)] TEST VIDEO DECODER \n", __func__, __LINE__);
	TEST_COMMON_PRINT("----------------------------------------------------------------- \n");

	if (access(TEST_COMMON_CODEC_FILE_PATH, 0) == 0) {
		VideoDecoder_Tester_Init_T tInit = {0, };
		VideoDecoder_Tester_T tTester = {0, };
		#ifdef GET_CSV_REPORT_INCLUDE
		TUCR_Init_T tReportInit = {0, };
		TUCR_Handle_T *ptTUCRHandle = NULL;

		(void)g_snprintf(tReportInit.strReportFileName, 	TUCR_MAX_SHORT_STR_LEN, "%s", DEC_TEST_REPORT_FILE);
		(void)g_snprintf(tReportInit.strDept, 				TUCR_MAX_SHORT_STR_LEN, "%s", DEC_TEST_REPORT_DEPT);
		(void)g_snprintf(tReportInit.strModule, 			TUCR_MAX_SHORT_STR_LEN, "%s", DEC_TEST_REPORT_MODULE);

		ptTUCRHandle = tucr_init(&tReportInit);
		#endif

		tInit.iMaxCnt = 10;

		tInit.eType = VID_Tester_Type_H264_FHD;
		tInit.iIndex = 0;
		bError = vdt_run_test(&tInit, &tTester);
		iTestRet += bError;
		#ifdef GET_CSV_REPORT_INCLUDE
		vt_write_a_result(ptTUCRHandle, "H264", "FHD", (int)bError);
		#endif

		tInit.eType = VID_Tester_Type_H265_FHD;
		tInit.iIndex += 1;
		bError = vdt_run_test(&tInit, &tTester);
		iTestRet += bError;
		#ifdef GET_CSV_REPORT_INCLUDE
		vt_write_a_result(ptTUCRHandle, "H265", "FHD", (int)bError);
		#endif

		tInit.eType = VID_Tester_Type_H265_QHD;
		tInit.iIndex += 1;
		bError = vdt_run_test(&tInit, &tTester);
		iTestRet += bError;
		#ifdef GET_CSV_REPORT_INCLUDE
		vt_write_a_result(ptTUCRHandle, "H265", "QHD", (int)bError);
		#endif

		tInit.eType = VID_Tester_Type_H265_UHD;
		tInit.iIndex += 1;
		bError = vdt_run_test(&tInit, &tTester);
		iTestRet += bError;
		#ifdef GET_CSV_REPORT_INCLUDE
		vt_write_a_result(ptTUCRHandle, "H265", "UHD", (int)bError);
		#endif

		tInit.eType = VID_Tester_Type_JPEG_FHD;
		tInit.iIndex += 1;
		bError = vdt_run_test(&tInit, &tTester);
		iTestRet += bError;
		#ifdef GET_CSV_REPORT_INCLUDE
		vt_write_a_result(ptTUCRHandle, "JPEG", "FHD", (int)bError);
		#endif

		tInit.eType = VID_Tester_Type_JPEG_UHD;
		tInit.iIndex += 1;
		bError = vdt_run_test(&tInit, &tTester);
		iTestRet += bError;
		#ifdef GET_CSV_REPORT_INCLUDE
		vt_write_a_result(ptTUCRHandle, "JPEG", "UHD", (int)bError);
		#endif

		tInit.eType = VID_Tester_Type_JPEG_MAX_RES;
		tInit.iIndex += 1;
		bError = vdt_run_test(&tInit, &tTester);
		iTestRet += bError;
		#ifdef GET_CSV_REPORT_INCLUDE
		vt_write_a_result(ptTUCRHandle, "JPEG", "MAX", (int)bError);
		#endif

		#ifdef GET_CSV_REPORT_INCLUDE
		tucr_deinit(ptTUCRHandle);
		#endif
	} else {
		TEST_ERROR_MSG("[%s][%d] OPEN FAILED: %s\n", __func__, __LINE__, TEST_COMMON_CODEC_FILE_PATH);
		bError = true;
	}

	if (iTestRet != 0) {
		TEST_ERROR_MSG("[%s][%d] RET: %d \n", __func__, __LINE__, iTestRet);
		bError = true;
	}

	return bError;
}

void vdt_init(VideoDecoder_Tester_T *ptTester)
{
	if (ptTester != NULL) {
		VideoDecoder_InitConfig_T tInitConfig = {0, };

		tInitConfig.eCodec 						= ptTester->eCodec;
		tInitConfig.number_of_surface_frames 	= 4U;

		ptTester->ptVideoDecoder 	= VideoDecoder_Create();
		ptTester->eDecError 		= VideoDecoder_Init(ptTester->ptVideoDecoder, &tInitConfig);

		if (ptTester->eDecError == (VideoDecoder_Error_E)VideoDecoder_ErrorNone) {
			ptTester->bIsInit = true;

		#if (USE_DEC_PERF_REPORT == 1)
			if ((ptTester->iMaxCnt > 0) && (ptTester->iMaxCnt <= TEST_COMMON_MAX_MEM_BUFFER_CNT))  {
				ptTester->ptDecStart 	= (long long *)g_malloc(sizeof(long long) * (unsigned int)(ptTester->iMaxCnt + 1));
				ptTester->ptDecEnd 		= (long long *)g_malloc(sizeof(long long) * (unsigned int)(ptTester->iMaxCnt + 1));
				ptTester->ptDecDiff 	= (long long *)g_malloc(sizeof(long long) * (unsigned int)(ptTester->iMaxCnt + 1));
			}
		#endif
		} else {
			ptTester->bIsInit = false;
		}

		vdt_set_video_dump_files(ptTester, false);
	}
}

void vdt_deinit(VideoDecoder_Tester_T *ptTester)
{
	if (ptTester != NULL) {
		if (ptTester->bIsInit == true) {
			TEST_DETAIL_MSG("[%s][%d] VideoDecoder_Deinit \n", __func__, __LINE__);
			(void)VideoDecoder_Deinit(ptTester->ptVideoDecoder);

			if (ptTester->ptVideoDecoder != NULL) {
				TEST_DETAIL_MSG("[%s][%d] VideoDecoder_Destroy \n", __func__, __LINE__);
				(void)VideoDecoder_Destroy(ptTester->ptVideoDecoder);
			}

		#if (USE_ENC_PERF_REPORT == 1)
			if ((ptTester->iMaxCnt > 0) && (ptTester->iMaxCnt <= TEST_COMMON_MAX_MEM_BUFFER_CNT))  {
				g_free(ptTester->ptDecStart);
				g_free(ptTester->ptDecEnd);
				g_free(ptTester->ptDecDiff);
			}
		#endif

			TEST_DETAIL_MSG("[%s][%d] vdt_set_video_dump_files \n", __func__, __LINE__);
			vdt_set_video_dump_files(ptTester, true);
		}
	}
}

void vdt_set_tester(VideoDecoder_Tester_T *ptTester, VideoDecoder_Tester_Init_T *ptInit)
{
	if (ptTester != NULL) {
		vdt_init_tester_data(ptTester);

		ptTester->eTest 		= (VID_Tester_Type_E)ptInit->eType;
		ptTester->iTesterIdx 	= ptInit->iIndex;
		ptTester->iMaxCnt 		= ptInit->iMaxCnt;
		ptTester->iFrameRate 	= TEST_COMMON_30FPS;
		ptTester->bIsRunning 	= true;

		ptTester->bIsInputDumpOn 	= true;
		ptTester->bIsOutputDumpOn 	= true;

		if (ptTester->eTest == VID_Tester_Type_None) {
			ptTester->eCodec 					= (VideoDecoder_Codec_E)VideoDecoder_Codec_H264;

			ptTester->iWidth 					= TEST_COMMON_HD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_HD_HEIGHT;
		} else if (ptTester->eTest == VID_Tester_Type_H264_FHD) {
			ptTester->eCodec 					= (VideoDecoder_Codec_E)VideoDecoder_Codec_H264;

			ptTester->iWidth 					= TEST_COMMON_FHD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_FHD_HEIGHT;
		} else if (ptTester->eTest == VID_Tester_Type_H265_FHD) {
			ptTester->eCodec 					= (VideoDecoder_Codec_E)VideoDecoder_Codec_H265;

			ptTester->iWidth 					= TEST_COMMON_FHD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_FHD_HEIGHT;
		} else if (ptTester->eTest == VID_Tester_Type_H265_QHD) {
			ptTester->eCodec 					= (VideoDecoder_Codec_E)VideoDecoder_Codec_H265;

			ptTester->iWidth 					= TEST_COMMON_QHD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_QHD_HEIGHT;
		} else if (ptTester->eTest == VID_Tester_Type_H265_UHD) {
			ptTester->eCodec 					= (VideoDecoder_Codec_E)VideoDecoder_Codec_H265;

			ptTester->iWidth 					= TEST_COMMON_UHD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_UHD_HEIGHT;
		} else if (ptTester->eTest == VID_Tester_Type_JPEG_FHD) {
			ptTester->eCodec 					= (VideoDecoder_Codec_E)VideoDecoder_Codec_MJPEG;

			ptTester->iWidth 					= TEST_COMMON_FHD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_FHD_HEIGHT;
		}  else if (ptTester->eTest == VID_Tester_Type_JPEG_UHD) {
			ptTester->eCodec 					= (VideoDecoder_Codec_E)VideoDecoder_Codec_MJPEG;

			ptTester->iWidth 					= TEST_COMMON_UHD_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_UHD_HEIGHT;
		}  else if (ptTester->eTest == VID_Tester_Type_JPEG_MAX_RES) {
			ptTester->eCodec 					= (VideoDecoder_Codec_E)VideoDecoder_Codec_MJPEG;

			ptTester->iWidth 					= TEST_COMMON_MAX_WIDTH;
			ptTester->iHeight 					= TEST_COMMON_MAX_HEIGHT;
		} else {
			TEST_DONOTHING(0);
		}

		if (strlen(ptInit->strInputFileName) > 3) {
			(void)g_snprintf(ptTester->strDecInputFileName, TEST_COMMON_PATH_STR, "%s", ptInit->strInputFileName);
		} else {
			(void)g_snprintf(ptTester->strDecInputFileName, TEST_COMMON_PATH_STR, "%s/%dX%d.%s", TEST_COMMON_CODEC_FILE_PATH, ptTester->iWidth, ptTester->iHeight, strDecodeExt[(int)ptTester->eCodec]);
		}

		TEST_DETAIL_MSG("[%s(%d)] FILE(%d): %s \n", __func__, __LINE__, ptInit->iIndex, ptTester->strDecInputFileName);
	}
}

void vdt_report_test_result(const VideoDecoder_Tester_T *ptTester)
{
	if (ptTester != NULL) {
		if (ptTester->eDecError != (VideoDecoder_Error_E)VideoDecoder_ErrorNone) {
			TEST_ERROR_MSG("[%s(%d)] ERROR: DECODER \n", __func__, __LINE__);
		} else {
		#if (USE_DEC_PERF_REPORT == 1)
			if ((ptTester->iMaxCnt > 0) && (ptTester->iMaxCnt <= TEST_COMMON_MAX_MEM_BUFFER_CNT))  {
				char strPerfReport[TEST_COMMON_CMD_STR] = {0, };
				char strSystemCmd[TEST_COMMON_CMD_STR] = {0, };

				(void)g_snprintf(strPerfReport, TEST_COMMON_CMD_STR, "vdt_perf_report_%d_%d.csv", ptTester->iTesterIdx, ptTester->eTest);

				for (unsigned int i = 0U; i < ptTester->uiCnt; i++) {
					(void)g_snprintf(strSystemCmd, TEST_COMMON_CMD_STR, "echo %d, %lld, %lld, %lld >> %s", i, ptTester->ptDecDiff[i], ptTester->ptDecStart[i], ptTester->ptDecEnd[i], strPerfReport);
					TEST_DETAIL_MSG("[%s(%d)] CMD: %s \n", __func__, __LINE__, strSystemCmd);
					(void)system(strSystemCmd);
				}
			}
		#endif
		}
	}
}

void vdt_dump_result_data(VideoDecoder_Tester_T *ptTester, VideoDecoder_StreamInfo_T *ptStreamInfo, VideoDecoder_Frame_T *ptFrame)
{
#if (USE_DEC_DUMP == 1)
	int iFrameSize = ptTester->iWidth * ptTester->iHeight;

	if (ptFrame->pucVirAddr[VideoDecoder_Comp_Y] == NULL) {
		TEST_ERROR_MSG("[%s(%d)] ERROR: VideoDecoder_Comp_Y  \n", __func__, __LINE__);
	}

	if (ptFrame->pucVirAddr[VideoDecoder_Comp_U] == NULL) {
		TEST_ERROR_MSG("[%s(%d)] ERROR: VideoDecoder_Comp_U  \n", __func__, __LINE__);
	}

	if (ptFrame->pucVirAddr[VideoDecoder_Comp_V] == NULL) {
		TEST_ERROR_MSG("[%s(%d)] ERROR: VideoDecoder_Comp_V  \n", __func__, __LINE__);
	}

	vdt_write_video_dump(ptTester, true, ptStreamInfo->pucStream, (int)ptStreamInfo->uiStreamLen);
	vdt_write_video_dump(ptTester, false, ptFrame->pucVirAddr[VideoDecoder_Comp_Y], iFrameSize);

	if (ptTester->eCodec == VideoDecoder_Codec_H265) {
		vdt_write_video_dump(ptTester, false, ptFrame->pucVirAddr[VideoDecoder_Comp_U], iFrameSize / 2);
	} else {
		vdt_write_video_dump(ptTester, false, ptFrame->pucVirAddr[VideoDecoder_Comp_U], iFrameSize / 4);
		vdt_write_video_dump(ptTester, false, ptFrame->pucVirAddr[VideoDecoder_Comp_V], iFrameSize / 4);
	}
#endif
}

bool vdt_run_test(VideoDecoder_Tester_Init_T *ptInit, VideoDecoder_Tester_T *ptTester)
{
	bool bError = false;

	vdt_set_tester(ptTester, ptInit);

	if (vdt_is_data_available(ptTester) == 1) {
		(void)pthread_create(&ptTester->hDecThread, NULL, vdt_running_test_thread, ptTester);
		ptTester->bIsRunning = true;
	}

	(void)usleep(20 * 1000);

	if (vdt_is_data_available(ptTester) == 1) {
		(void)pthread_join(ptTester->hDecThread, NULL);
	}

	if (ptTester->eDecError != (VideoDecoder_Error_E)VideoDecoder_ErrorNone) {
		bError = true;
	}

	TEST_COMMON_PRINT("[%s(%d)] TEST %d(%d): %u Frames \n", __func__, __LINE__, ptInit->iIndex, (int)bError, (int)ptTester->uiCnt);
	TEST_COMMON_PRINT("[%s(%d)]   - Index: %d, Type: %d \n", __func__, __LINE__, ptInit->iMaxCnt, (int)ptInit->eType);
	TEST_COMMON_PRINT("[%s(%d)]   - MAX. Decoding Time: %lldus \n", __func__, __LINE__, ptTester->llMaxDec);
	TEST_COMMON_PRINT("[%s(%d)]   - MIN. Decoding Time: %lldus \n", __func__, __LINE__, ptTester->llMinDec);
	TEST_COMMON_PRINT("[%s(%d)]   - AVR. Decoding Time: %lldus \n", __func__, __LINE__, ptTester->llAvrDecDiff);

	return bError;
}

static unsigned char *vdt_read_data_from_input_bs_file(FILE *hFile, int *iBufferSize, VideoDecoder_Tester_T *ptTester)
{
	int i = 0;
	int iMaxTry = 2;
	unsigned char *pucBuffer = NULL;

	for (i = 0; i < iMaxTry; i++) {
		size_t readbyte = 0U;

		if (ptTester->eCodec == VideoDecoder_Codec_MJPEG) {
			int fileSize = 0;

			// get file size
			fseek(hFile, 0L, SEEK_END);
			fileSize = ftell(hFile);
			rewind(hFile);
			pucBuffer = g_malloc0((size_t)fileSize);

			readbyte = fread(pucBuffer, 1U, (size_t)fileSize, hFile);

			*iBufferSize = (int)readbyte;

			if (readbyte > 0U) {
				break;
			} else {
				if (pucBuffer != NULL) {
					g_free(pucBuffer);
					pucBuffer = NULL;
				}
			}
		} else {
			int iTmpBufferSize = 0;

			readbyte = fread(&iTmpBufferSize, sizeof(int), 1U, hFile);

			if (readbyte > 0U) {
				size_t uiReadSize = 0U;

				if ((iTmpBufferSize > 0) && (iTmpBufferSize < INT32_MAX)) {
					readbyte = 0U;
					uiReadSize = (size_t)iTmpBufferSize;
					pucBuffer = g_malloc0(uiReadSize);

					readbyte = fread(pucBuffer, uiReadSize, 1U, hFile);
					if (readbyte == 0U) {
						TEST_ERROR_MSG("[%s(%d)] ERROR: READ BUFFER \n", __func__, __LINE__);
					} else {
						*iBufferSize = iTmpBufferSize;
						break;
					}
				}
			} else {
				//TEST_ERROR_MSG("[%s(%d)] RETRY: Read(%d)  \n", __func__, __LINE__, i);
				(void)fseek(hFile, 0L, SEEK_SET);
			}
		}
	}

	return pucBuffer;
}

static void *vdt_running_test_thread(void *param)
{
	VideoDecoder_Tester_T *ptTester = (VideoDecoder_Tester_T *)param;

	if (ptTester != NULL) {
		FILE *hBSFile = NULL;
		char strBSFileName[TEST_COMMON_PATH_STR];
		unsigned char *pucSeqBuffer = NULL;
		int iSeqBufferSize = 0;

		TEST_DETAIL_MSG("[%s(%d)] OPEN: %s \n", __func__, __LINE__, ptTester->strDecInputFileName);
		hBSFile = fopen(ptTester->strDecInputFileName, "r");

		if (hBSFile != NULL) {
			pucSeqBuffer = vdt_read_data_from_input_bs_file(hBSFile, &iSeqBufferSize, ptTester);
		}

		if ((pucSeqBuffer != NULL) && (iSeqBufferSize >= 0)) {
			VideoDecoder_SequenceHeaderInputParam_T tHeader = {0, };
			VideoDecoder_StreamInfo_T tSeqStreamInfo = {0, };
			VideoDecoder_Frame_T tSeqFrame = {0, };

			static int64_t iTmpTimeStamp = 0;

			vdt_init(ptTester);

			tHeader.pucStream 				= pucSeqBuffer;
			tHeader.uiStreamLen 			= (unsigned int)iSeqBufferSize;
			tHeader.uiNumOutputFrames 		= 4U;

			ptTester->eDecError = VideoDecoder_DecodeSequenceHeader(ptTester->ptVideoDecoder, &tHeader);

			if (ptTester->eDecError == (VideoDecoder_Error_E)VideoDecoder_ErrorNone) {
				tSeqStreamInfo.iTimeStamp 	= 0;
				tSeqStreamInfo.pucStream 	= tHeader.pucStream;
				tSeqStreamInfo.uiStreamLen 	= tHeader.uiStreamLen;

				ptTester->eDecError = VideoDecoder_Decode(ptTester->ptVideoDecoder, &tSeqStreamInfo, &tSeqFrame);

				if (tHeader.uiStreamLen > 0U) {
					vdt_write_video_dump(ptTester, true, tHeader.pucStream, (int)tHeader.uiStreamLen);
				} else {
					TEST_ERROR_MSG("[%s(%d)] ERROR: BUFFER SIZE \n", __func__, __LINE__);
				}

				if (pucSeqBuffer != NULL) {
					g_free(pucSeqBuffer);
				}
			}

			if (ptTester->eDecError == (VideoDecoder_Error_E)VideoDecoder_ErrorNone) {
				while (ptTester->bIsRunning == true) {
					if ((ptTester->uiCnt >= ((unsigned int)ptTester->iMaxCnt)) && (ptTester->iMaxCnt != -1)) {
						ptTester->bIsRunning = false;
					} else {
						VideoDecoder_StreamInfo_T tDecStreamInfo = {0, };
						VideoDecoder_Frame_T tDecFrame = {0, };

						unsigned char *pucVideoBuffer = NULL;
						int iVideoBufferSize = 0;

						pucVideoBuffer = vdt_read_data_from_input_bs_file(hBSFile, &iVideoBufferSize, ptTester);

						if ((pucVideoBuffer == NULL) || (iVideoBufferSize <= 0)) {
							ptTester->bIsRunning = false;
							TEST_ERROR_MSG("[%s(%d)] ERROR: READ VIDEO BUFFER(%d) \n", __func__, __LINE__, iVideoBufferSize);
						} else {
							long long llDecodingStart = 0;
							long long llDecodingEnd = 0;
							long long llDecodingDiff = 0;

							tDecStreamInfo.iTimeStamp 	= iTmpTimeStamp;
							tDecStreamInfo.pucStream 	= pucVideoBuffer;
							tDecStreamInfo.uiStreamLen 	= (unsigned int)iVideoBufferSize;

							llDecodingStart = vt_tools_get_time();
							ptTester->eDecError = VideoDecoder_Decode(ptTester->ptVideoDecoder, &tDecStreamInfo, &tDecFrame);
							llDecodingEnd = vt_tools_get_time();

							if (ptTester->eDecError != (VideoDecoder_Error_E)VideoDecoder_ErrorNone) {
								TEST_ERROR_MSG("[%s(%d)] ERROR: %d \n", __func__, __LINE__, (int)ptTester->eDecError);
								ptTester->bIsRunning = false;
							} else {
								vdt_dump_result_data(ptTester, &tDecStreamInfo, &tDecFrame);

								llDecodingDiff = llDecodingEnd - llDecodingStart;
								ptTester->llAccDecDiff += llDecodingDiff;
								ptTester->llAvrDecDiff = ptTester->llAccDecDiff / ((long long)ptTester->uiCnt + 1);

								if (ptTester->llMaxDec < llDecodingDiff) {
									ptTester->llMaxDec = llDecodingDiff;
								}

								if ((ptTester->llMinDec > llDecodingDiff) || (ptTester->llMinDec == 0)) {
									ptTester->llMinDec = llDecodingDiff;
								}

							#if (USE_DEC_PERF_REPORT == 1)
								if ((ptTester->iMaxCnt > 0) && (ptTester->iMaxCnt <= TEST_COMMON_MAX_MEM_BUFFER_CNT))  {
									ptTester->ptDecDiff[ptTester->uiCnt] = llDecodingDiff;
									ptTester->ptDecStart[ptTester->uiCnt] = llDecodingStart;
									ptTester->ptDecEnd[ptTester->uiCnt] = llDecodingEnd;
								}
							#endif

							#if 0
								TEST_COMMON_PRINT("[%s(%d)] DEC(%d): %ld \n", __func__, __LINE__, ptTester->eDecError, ptTester->uiCnt);
								TEST_COMMON_PRINT("[%s(%d)]   - %p: %u X %u \n", __func__, __LINE__, tDecFrame.pucPhyAddr[VideoDecoder_Comp_Y], tDecFrame.uiWidth, tDecFrame.uiHeight);
							#endif

							#if 0
								if ((ptTester->iMaxCnt == -1) && (((ptTester->uiCnt + 1U) % 300U) == 0U))
									TEST_COMMON_PRINT("[%s(%d)] %d: %ld Decoded(AVR.: %lldus) \n", __func__, __LINE__, ptTester->iTesterIdx, (ptTester->uiCnt + 1U), ptTester->llAvrDecDiff);
							#endif

								TEST_DETAIL_MSG("[%s(%d)] PERF: DEC(%u / %lld) \n", __func__, __LINE__, ptTester->uiCnt, ptTester->llAvrDecDiff);
								ptTester->uiCnt++;
								iTmpTimeStamp += 33333;
							}
						}

						if (pucVideoBuffer != NULL) {
							g_free(pucVideoBuffer);
						} else {
							TEST_ERROR_MSG("[%s(%d)] ERROR: pucVideoBuffer(%d)  \n", __func__, __LINE__, ptTester->iTesterIdx);
						}
					}
				}
			}
		} else {
			TEST_ERROR_MSG("[%s(%d)] ERROR: OPEN(%s) \n", __func__, __LINE__, ptTester->strDecInputFileName);
			ptTester->eDecError = (VideoDecoder_Error_E)VideoDecoder_ErrorInitFail;
		}

		if (hBSFile != NULL) {
			TEST_DETAIL_MSG("[%s(%d)] CLOSE: %s \n", __func__, __LINE__, ptTester->strDecInputFileName);
			(void)fclose(hBSFile);
		}

		vdt_report_test_result(ptTester);
		vdt_deinit(ptTester);
	}

	return NULL;
}

static int vdt_is_data_available(const VideoDecoder_Tester_T *ptTester)
{
	int iRet = -1;

	if (ptTester != NULL) {
		if ((ptTester->iWidth > 0) && (ptTester->iHeight > 0)) {
			iRet = 1;
		}
	}

	return iRet;
}

static void vdt_init_tester_data(VideoDecoder_Tester_T *ptTester)
{
	if (ptTester != NULL) {
		ptTester->iTesterIdx 			= 0;
		ptTester->uiCnt 				= 0U;

		ptTester->eDecError 			= (VideoDecoder_Error_E)VideoDecoder_ErrorNone;

		ptTester->bIsInit 				= false;
		ptTester->bIsRunning 			= false;

		ptTester->bIsInputDumpOn 		= false;
		ptTester->bIsOutputDumpOn 		= false;

		ptTester->llMinDec 				= 0;
		ptTester->llMaxDec 				= 0;
		ptTester->llAccDecDiff 			= 0;
		ptTester->llAvrDecDiff 			= 0;
	}
}

void vdt_write_video_dump(const VideoDecoder_Tester_T *ptTester, bool bIsInput, const unsigned char *pucBuffer, int iBufferSize)
{
#if (USE_DEC_DUMP == 1)
	if ((pucBuffer != NULL) && (iBufferSize > 0)) {
		if ((bIsInput == true) && (ptTester->bIsInputDumpOn == true)) {
			(void)fwrite(pucBuffer, (size_t)1U, (size_t)iBufferSize, ptTester->hInputFile);
		} else if ((bIsInput == false) && (ptTester->bIsOutputDumpOn == true)) {
			(void)fwrite(pucBuffer, (size_t)1U, (size_t)iBufferSize, ptTester->hOutputFile);
		} else {
			TEST_DONOTHING();
		}
	}
#endif
}

static void vdt_set_video_dump_files(VideoDecoder_Tester_T *ptTester, bool bIsClose)
{
#if (USE_DEC_DUMP == 1)
	if (ptTester != NULL) {
		if ((ptTester->bIsInputDumpOn == true) || (ptTester->bIsOutputDumpOn == true)) {
			int iTmp = (int)ptTester->eCodec;

			if (iTmp >= 0) {
				if (bIsClose == false) {
					TEST_DETAIL_MSG("[%s(%d)] OPEN DUMP FILE(S) \n", __func__, __LINE__);

					if (ptTester->bIsInputDumpOn == true) {
						(void)g_snprintf(ptTester->strInputFileName, TEST_COMMON_PATH_STR, "vdt_input_%d_%d.%s", ptTester->iTesterIdx, ptTester->eTest, strDecodeExt[iTmp]);
						ptTester->hInputFile = fopen(ptTester->strInputFileName, "wb");

						if (ptTester->hInputFile != NULL) {
							TEST_DETAIL_MSG("[%s(%d)]   - %s \n", __func__, __LINE__, ptTester->strInputFileName);
						} else {
							TEST_ERROR_MSG("[%s(%d)] ERROR: OPEN(%s) \n", __func__, __LINE__, ptTester->strInputFileName);}
					}

					if (ptTester->bIsOutputDumpOn == true) {
						(void)g_snprintf(ptTester->strOutputFileName, TEST_COMMON_PATH_STR, "vdt_output_%d_%d_%dX%d.%s", ptTester->iTesterIdx, ptTester->eTest, ptTester->iWidth, ptTester->iHeight, strDecodeExt[0]);
						ptTester->hOutputFile = fopen(ptTester->strOutputFileName, "wb");

						if (ptTester->hOutputFile != NULL) {
							TEST_DETAIL_MSG("[%s(%d)]   - %s \n", __func__, __LINE__, ptTester->strOutputFileName);
						} else {
							TEST_ERROR_MSG("[%s(%d)] ERROR: OPEN(%s) \n", __func__, __LINE__, ptTester->strOutputFileName);
						}
					}
				}

				if (bIsClose == true) {
					TEST_DETAIL_MSG("[%s(%d)] CLOSE DUMP FILE(S) \n", __func__, __LINE__);

					if (ptTester->bIsInputDumpOn == true) {
						if (ptTester->hInputFile != NULL) {
							TEST_DETAIL_MSG("[%s(%d)]   - %s \n", __func__, __LINE__, ptTester->strInputFileName);
							(void)fclose(ptTester->hInputFile);
						}
					}

					if (ptTester->bIsOutputDumpOn == true) {
						if (ptTester->hInputFile != NULL) {
							TEST_DETAIL_MSG("[%s(%d)]   - %s \n", __func__, __LINE__, ptTester->strOutputFileName);
							(void)fclose(ptTester->hOutputFile);
						}
					}
				}
			}
		}
	}
#endif
}