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

#include "vid_test_tools.h"
#include "enc_test.h"
#include "dec_test.h"
#include "vid_test.h"


typedef struct
{
	VideoEncoder_Tester_T tEncoderTester;
	VideoDecoder_Tester_T tDecoderTester;
} Video_Tester_T;


static bool vt_run_test(VID_Tester_Type_E eType);
static bool vt_process_video(Video_Tester_T *ptTester);
static void *vt_test_thread(void *param);


bool vt_video_test(void)
{
	bool bError = false;
	TEST_DETAIL_MSG("[%s(%d)] TEST VIDEO: ENCODER TO DECODER\n", __func__, __LINE__);

	if (access(TEST_COMMON_YUV_FILE_PATH, 0) == 0) {
		bError = vt_run_test(VID_Tester_Type_H264_FHD);

		if (bError == false) {
			bError = vt_run_test(VID_Tester_Type_H265_FHD);
		}

		if (bError == false) {
			bError = vt_run_test(VID_Tester_Type_H265_QHD);
		}

		if (bError == false) {
			bError = vt_run_test(VID_Tester_Type_H265_UHD);
		}

		vet_set_video_memory(NULL, true);
	} else {
		TEST_ERROR_MSG("[%s][%d] OPEN FAILED: %s\n", __func__, __LINE__, TEST_COMMON_YUV_FILE_PATH);
		bError = true;
	}

	return bError;
}

static bool vt_run_test(VID_Tester_Type_E eType)
{
	bool bError = false;

	Video_Tester_T tTester;
	Video_Tester_T *ptTester = &tTester;
	VideoEncoder_Tester_T *ptEncTester = &ptTester->tEncoderTester;
	VideoDecoder_Tester_T *ptDecTester = &ptTester->tDecoderTester;

	VideoEncoder_Tester_Init_T tTestInit = {0, eType, 0, };
	VideoDecoder_Tester_Init_T tDecTestInit = {0, eType, 0, };

	vet_set_tester(ptEncTester, &tTestInit);
	vdt_set_tester(ptDecTester, &tDecTestInit);

	if (vet_is_data_available(ptEncTester) == 1) {
		vet_set_video_memory(ptEncTester, false);
		(void)pthread_create(&ptEncTester->hEncThread, NULL, vt_test_thread, ptTester);
	}

	(void)pthread_join(ptEncTester->hEncThread, NULL);

	vet_report_test_result(ptEncTester);
	vdt_report_test_result(ptDecTester);

	TEST_DETAIL_MSG("[%s(%d)] PERF: DEC(%lld, %d), ENC(%lld, %d) \n", __func__, __LINE__, ptDecTester->llAvrDecDiff, ((1000 * 1000) / ptDecTester->iFrameRate), ptEncTester->llAvrEncDiff, ((1000 * 1000) / ptEncTester->iFrameRate));

	return bError;
}

static void *vt_test_thread(void *param)
{
	Video_Tester_T *ptTester = (VideoEncoder_Tester_T *)param;

	if (ptTester != NULL) {
		VideoEncoder_Tester_T *ptEncTester = &ptTester->tEncoderTester;
		VideoDecoder_Tester_T *ptDecTester = &ptTester->tDecoderTester;

		if ((ptEncTester != NULL) && (ptDecTester != NULL) && (ptEncTester->tHeader.iHeaderSize >= 0)) {
			VideoDecoder_SequenceHeaderInputParam_T tHeader = {0, };

			vet_init(ptEncTester);
			vdt_init(ptDecTester);

			TEST_DETAIL_MSG("[%s][%d] eDecError: %d \n", __func__, __LINE__, ptDecTester->eDecError);
			TEST_DETAIL_MSG("[%s][%d] eEncError: %d \n", __func__, __LINE__, ptEncTester->eEncError);

			tHeader.pucStream 				= ptEncTester->tHeader.pucBuffer;
			tHeader.uiStreamLen 			= (unsigned int)ptEncTester->tHeader.iHeaderSize;
			tHeader.uiNumOutputFrames 		= 4;

			ptDecTester->eDecError 			= VideoDecoder_DecodeSequenceHeader(ptDecTester->ptVideoDecoder, &tHeader);

			if (ptDecTester->eDecError == (VideoDecoder_Error_E)VideoDecoder_ErrorNone) {
				VideoDecoder_StreamInfo_T tStreamInfo = {0, };
				VideoDecoder_Frame_T tFrame = {0, };

				tStreamInfo.iTimeStamp 	= 0;
				tStreamInfo.pucStream 	= tHeader.pucStream;
				tStreamInfo.uiStreamLen = tHeader.uiStreamLen;

				ptDecTester->eDecError = VideoDecoder_Decode(ptDecTester->ptVideoDecoder, &tStreamInfo, &tFrame);
				vdt_write_video_dump(ptDecTester, true, tHeader.pucStream, (int)tHeader.uiStreamLen);
			}

			if ((ptEncTester->eEncError == (VideoEncoder_Error_E)VideoEncoder_Error_None) && (ptDecTester->eDecError == (VideoDecoder_Error_E)VideoDecoder_ErrorNone)) {
				vet_write_video_dump(ptEncTester, false, ptEncTester->tHeader.pucBuffer, ptEncTester->tHeader.iHeaderSize);

				while (ptEncTester->bIsRunning == true) {
					if (ptEncTester->uiCnt > ((unsigned int)TEST_COMMON_MAX_FRAME_CNT)) {
						ptEncTester->bIsRunning = false;
					} else {
						if (vt_process_video(ptTester) == true) {
							ptEncTester->bIsRunning = false;
						}
					}
				}
			} else {
				TEST_ERROR_MSG("[%s][%d] ERROR: INIT\n", __func__, __LINE__);
			}

			TEST_DETAIL_MSG("[%s][%d] eDecError: %d \n", __func__, __LINE__, ptDecTester->eDecError);
			vet_deinit(ptEncTester);
			TEST_DETAIL_MSG("[%s][%d] eEncError: %d \n", __func__, __LINE__, ptEncTester->eEncError);
			vdt_deinit(ptDecTester);
		}
	}

	TEST_DETAIL_MSG("[%s][%d] DONE \n", __func__, __LINE__);

	return NULL;
}

static bool vt_process_video(Video_Tester_T *ptTester)
{
	bool bError = false;

	if (ptTester != NULL) {
		VideoEncoder_Tester_T *ptEncTester = &ptTester->tEncoderTester;
		VideoDecoder_Tester_T *ptDecTester = &ptTester->tDecoderTester;

		if ((ptDecTester != NULL) && (ptEncTester != NULL)) {
			static int64_t iTmpTimeStamp = 0;

			int iBufIdx = (int)ptEncTester->uiCnt % TEST_COMMON_MAX_MEM_BUFFER;

			VideoEncoder_Encode_Input_T tEncodeInput = {0, };
			VideoEncoder_Encode_Result_T tEncodeResult = {0, };
			VideoDecoder_StreamInfo_T tStreamInfo = {0, };
			VideoDecoder_Frame_T tFrame = {0, };

			long long llEncodingStart = 0;
			long long llEncodingEnd = 0;
			long long llEncodingDiff = 0;

			long long llDecodingStart = 0;
			long long llDecodingEnd = 0;
			long long llDecodingDiff = 0;

			llEncodingStart = vt_tools_get_time();

			tEncodeInput.pucVideoBuffer[VE_IN_Y] = ptEncTester->pucVideoBuffer[iBufIdx];
			tEncodeInput.pucVideoBuffer[VE_IN_U] = NULL;
			tEncodeInput.pucVideoBuffer[VE_IN_V] = NULL;

			tEncodeInput.llTimeStamp += 3300;

			ptEncTester->eEncError = VideoEncoder_Encode(ptEncTester->ptVideoEncoder, &tEncodeInput, &tEncodeResult);
			llEncodingEnd = vt_tools_get_time();

			if (ptEncTester->eEncError != (VideoEncoder_Error_E)VideoEncoder_Error_None) {
				TEST_ERROR_MSG("[%s][%d] eEncError: %d\n", __func__, __LINE__, ptEncTester->eEncError);
				bError = true;
			} else {
				int iBufSize = ((ptEncTester->iWidth * ptEncTester->iHeight) * 3) / 2;

				vet_write_video_dump(ptEncTester, true, ptEncTester->pucVideoBuffer[iBufIdx], iBufSize);
				vet_write_video_dump(ptEncTester, false, tEncodeResult.pucBuffer, tEncodeResult.iFrameSize);

				llEncodingDiff = llEncodingEnd - llEncodingStart;

				ptEncTester->llAccEncDiff += llEncodingDiff;
				ptEncTester->llAvrEncDiff = ptEncTester->llAccEncDiff / ((long long)ptEncTester->uiCnt + 1);
				TEST_DETAIL_MSG("[%s(%d)] PERF: ENC(%u / %lld) \n", __func__, __LINE__, ptEncTester->uiCnt, ptEncTester->llAvrEncDiff);
				ptEncTester->uiCnt++;
			}

			if (tEncodeResult.iFrameSize > 0) {
				tStreamInfo.iTimeStamp 	= iTmpTimeStamp;
				tStreamInfo.pucStream 	= tEncodeResult.pucBuffer;
				tStreamInfo.uiStreamLen = (unsigned int)tEncodeResult.iFrameSize;
			}

			llDecodingStart = vt_tools_get_time();
			ptDecTester->eDecError = VideoDecoder_Decode(ptDecTester->ptVideoDecoder, &tStreamInfo, &tFrame);
			llDecodingEnd = vt_tools_get_time();

			if (ptDecTester->eDecError != (VideoDecoder_Error_E)VideoDecoder_ErrorNone) {
				TEST_ERROR_MSG("[%s][%d] eDecError: %d\n", __func__, __LINE__, ptDecTester->eDecError);
				bError = true;
			} else {
				vdt_dump_result_data(ptDecTester, &tStreamInfo, &tFrame);

				llDecodingDiff = llDecodingEnd - llDecodingStart;
				ptDecTester->llAccDecDiff += llDecodingDiff;
				ptDecTester->llAvrDecDiff = ptDecTester->llAccDecDiff / ((long long)ptDecTester->uiCnt + 1);
				TEST_DETAIL_MSG("[%s(%d)] PERF: DEC(%u / %lld) \n", __func__, __LINE__, ptDecTester->uiCnt, ptDecTester->llAvrDecDiff);
				ptDecTester->uiCnt++;

				iTmpTimeStamp += 33333;
			}
		}
	}

	return bError;
}