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
#include <stdbool.h>
#include <time.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <TU_JP_Lib_C_Std.h>
#include <TU_JP_Lib_C_D_LinkedList.h>

#include "video_decode_resource.h"

#include "video_clock_control.h"
#include "video_index_control.h"

#include "video_decoder.h"


#define MODE_DECODED_FRAME_OUTPUT				(1 << 1)

#define SET_MODE(private_data, flag)    ((private_data)->ulModeFlags |= (flag))
#define CLEAR_MODE(private_data, flag)  ((private_data)->ulModeFlags &= ~(flag))
#define CHECK_MODE(private_data, flag)  ((private_data)->ulModeFlags & (flag))

#define VPC_SKIPMODE_NONE 				(0)
#define VPC_SKIPMODE_I_SEARCH			(1)
#define VPC_SKIPMODE_B_SKIP				(2)
#define VPC_SKIPMODE_EXCEPT_I_SKIP		(3)
#define VPC_SKIPMODE_NEXT_B_SKIPc		(4)
#define VPC_SKIPMODE_B_WAIT 			(5)
#define VPC_SKIPMODE_NEXT_STEP 			(10)

#define VPC_SKIPMODE_IDR_SEARCH 		(21)
#define VPC_SKIPMODE_NONIDR_SEARCH 		(22)

#define VD_PRINT 						(void)g_printf
#define VD_SNPRINT 						(void)g_snprintf
#define VD_CAST(tar, src) 				(void)memcpy((void*)&(tar), (void*)&(src), sizeof(void*))
#define VD_CALLOC						g_malloc0
#define VD_FREE							(void)g_free

#define VD_DEFAULT_OUTFRAMES			(2)

#define VD_ERROR_MSG 					VD_PRINT
#define VD_SHORT_STR_MAX				(32)
#define VD_NAME_STR_MAX					(64)


typedef struct
{
	bool bInterlaced;
	bool bOddFieldFirst;

	float fAspectRatio;

	int64_t iFeedTime;

	unsigned int uiWidth;
	unsigned int uiHeight;

	VideoDecoder_Rect_T tCropRect;

	uint64_t uiID;
} ST_DecFrameListData_T;

struct VideoDecoder_T
{
	bool bFristFrameDecoded;

	unsigned long ulModeFlags;

	VideoDecoder_Codec_E eCodec;

	VideoClock_T tVideoClock;

	JP_D_LinkedList_IF_T *ptListDecFrame;
	VideoIndexCtrl_T *ptVideoIndexCtrl;
	VD_RESOURCE_T *ptResource;
	VD_RESOURCE_TYPE_T eType;

	unsigned int uiNumOutputFrames;

	uint64_t uiFrameID;
};


static VideoDecoder_Error_E SF_VDec_SetCodec(VideoDecoder_T *ptVideoDecoder, VideoDecoder_Codec_E eCodec);
static VideoDecoder_Error_E SF_VDec_GetOutputFrame(VideoDecoder_T *ptVideoDecoder, VideoDecoder_Frame_T *ptFrame, const VideoDecoder_DecodingResult_T *ptDecodingResult);


unsigned int VideoDecoder_GetCbCrInterleaveMode(const VideoDecoder_T *ptVideoDecoder)
{
	return vd_res_get_CbCrInterleaveMode(ptVideoDecoder->ptResource);
}

VideoDecoder_Error_E VideoDecoder_Flush(const VideoDecoder_T *ptVideoDecoder)
{
	(void)ptVideoDecoder;
	return VideoDecoder_ErrorNotSupportedCodec;
}

VideoDecoder_Error_E VideoDecoder_DecodeSequenceHeader(const VideoDecoder_T *ptVideoDecoder, const VideoDecoder_SequenceHeaderInputParam_T *ptParam)
{
	VideoDecoder_Error_E ret = (VideoDecoder_Error_E)VideoDecoder_ErrorNone;

	if ((ptVideoDecoder == NULL) || (ptParam == NULL)) {
		ret = VideoDecoder_ErrorBadParameter;
	} else if ((ptParam->pucStream == NULL) || (ptParam->uiStreamLen == 0U)) {
		ret = VideoDecoder_ErrorBadParameter;
	} else {
		int iVal = 0;
		bool bSeq = vd_res_decode_seq_header(ptVideoDecoder->ptResource, ptParam);

		unsigned int uiNumOutputFrames = 0U;

		if ((ptVideoDecoder->uiNumOutputFrames < (UINT32_MAX / 2U) && (ptVideoDecoder->uiNumOutputFrames > 0U))) {
			uiNumOutputFrames = ptVideoDecoder->uiNumOutputFrames;
		} else if ((ptParam->uiNumOutputFrames < (UINT32_MAX / 2U) && (ptParam->uiNumOutputFrames > 0U))) {
			uiNumOutputFrames = ptParam->uiNumOutputFrames;
		}

		if (bSeq == false) {
			iVal = -1;
			ret = VideoDecoder_ErrorSequenceHeaderDecoding;
		}

		if (ret == VideoDecoder_ErrorNone) {
			if (ptVideoDecoder->ptVideoIndexCtrl != NULL) {
				VideoIndexCtrl_MaxIndexCnt_T tMaxIndexCnt = {0, };

				tMaxIndexCnt.iMaxIndexCnt = VD_DEFAULT_OUTFRAMES;

				if (uiNumOutputFrames > 0U) {
					tMaxIndexCnt.iMaxIndexCnt = (int)uiNumOutputFrames;
				}

				iVal = VideoIndexCtrl_SetConfig(ptVideoDecoder->ptVideoIndexCtrl, VideoIndexCtrl_Config_MaxIndexCnt, &tMaxIndexCnt);
			} else {
				iVal = -1;
			}

			if (iVal < 0) {
				ret = VideoDecoder_ErrorDecodingFail;
			}
		}
	}

	return ret;
}

VideoDecoder_Error_E VideoDecoder_Decode(VideoDecoder_T *ptVideoDecoder, const VideoDecoder_StreamInfo_T *ptStream, VideoDecoder_Frame_T *ptFrame)
{
	VideoDecoder_Error_E ret = (VideoDecoder_Error_E)VideoDecoder_ErrorNone;

	if (ptVideoDecoder == NULL) {
		ret = VideoDecoder_ErrorBadParameter;
	} else {
		VideoClock_T *ptVideoClock = &ptVideoDecoder->tVideoClock;

		if (ptVideoClock == NULL) {
			ret = VideoDecoder_ErrorBadParameter;
		} else if (ptVideoDecoder->ptVideoIndexCtrl == NULL) {
			ret = VideoDecoder_ErrorBadParameter;
		} else {
			int iVal = 0;
			VideoDecoder_DecodingResult_T tDecodingResult = {0, };

			(void)VideoIndexCtrl_Update(ptVideoDecoder->ptVideoIndexCtrl);
			(void)VideoIndexCtrl_Clear(ptVideoDecoder->ptVideoIndexCtrl);

			iVal = vd_res_decode(ptVideoDecoder->ptResource, ptStream, &tDecodingResult);

			if (iVal == 0) {
				if (ptVideoDecoder->bFristFrameDecoded == false) {
					struct timespec tspec;
					(void)clock_gettime(CLOCK_MONOTONIC, &tspec);

					VD_PRINT("===================================================================== \n");
					VD_PRINT("[%ld.%3ld] [VIDEO: %d-%d] Decode 1st Video Frame Done at %ld. \n", (int64_t)tspec.tv_sec, (tspec.tv_nsec / 1000), tDecodingResult.iIntanceIdx, (int)ptVideoDecoder->eCodec, ptStream->iTimeStamp);
					VD_PRINT("===================================================================== \n");
					ptVideoDecoder->bFristFrameDecoded = true;
				}

				if (tDecodingResult.bIsInsertFrame == true) {
					char strName[VD_SHORT_STR_MAX];

					ST_DecFrameListData_T tListData;
					const JP_D_LinkedList_IF_T *ptListDecFrame = ptVideoDecoder->ptListDecFrame;

					VD_SNPRINT(strName, VD_SHORT_STR_MAX, "%d", tDecodingResult.iDecodedIdx);

					tListData.uiID 					= ptVideoDecoder->uiFrameID;
					tListData.bInterlaced 			= tDecodingResult.bInterlaced;
					tListData.bOddFieldFirst 		= tDecodingResult.bOddFieldFirst;

					tListData.uiWidth 				= tDecodingResult.uiWidth;
					tListData.uiHeight 				= tDecodingResult.uiHeight;

					tListData.fAspectRatio 			= tDecodingResult.fAspectRatio;

					tListData.tCropRect.uiTop 		= tDecodingResult.uiCropTop;
					tListData.tCropRect.uiLeft 		= tDecodingResult.uiCropLeft;
					tListData.tCropRect.uiWidth 	= tDecodingResult.uiCropWidth;
					tListData.tCropRect.uiHeight 	= tDecodingResult.uiCropHeight;

					(void)ptListDecFrame->InsertList(ptListDecFrame->ptListSt, strName, &tListData, sizeof(ST_DecFrameListData_T));
				}

				if (tDecodingResult.bIsInsertClock == true) {
					VideoClockCtrl_InsertClock(ptVideoClock, ptStream->iTimeStamp, 0, tDecodingResult.bField, tDecodingResult.iDecodedIdx, false);
					ptVideoDecoder->uiFrameID++;
				}

				if (ptVideoDecoder->ptVideoIndexCtrl != NULL) {
					if (tDecodingResult.bIsFrameOut == true) {
						ret = SF_VDec_GetOutputFrame(ptVideoDecoder, ptFrame, &tDecodingResult);
					}

					if (tDecodingResult.iDispOutIdx >= 0) {
						(void)VideoIndexCtrl_Push(ptVideoDecoder->ptVideoIndexCtrl, tDecodingResult.iDispOutIdx, tDecodingResult.pucDispOut[VideoDecoder_Addr_PA][VideoDecoder_Comp_Y]);
					}
					ptFrame->ulResult = tDecodingResult.ulResult;
				} else {
					ret = (VideoDecoder_Error_E)VideoDecoder_ErrorDecodingFail;
				}
			}
		}
	}

	return ret;
}

VideoDecoder_Error_E VideoDecoder_Init(VideoDecoder_T *ptVideoDecoder, const VideoDecoder_InitConfig_T *ptConfig)
{
	VideoDecoder_Error_E ret = (VideoDecoder_Error_E)VideoDecoder_ErrorNone;

	if (ptVideoDecoder == NULL) {
		ret = VideoDecoder_ErrorBadParameter;
	} else {
		if (SF_VDec_SetCodec(ptVideoDecoder, ptConfig->eCodec) != VideoDecoder_ErrorNone) {
			ret = VideoDecoder_ErrorInitFail;
		} else {
			(void)vd_res_init_resource(ptVideoDecoder->ptResource, ptConfig);
			VideoClockCtrl_ResetClock(&ptVideoDecoder->tVideoClock);

			ptVideoDecoder->uiFrameID = 0U;
			ptVideoDecoder->uiNumOutputFrames = ptConfig->number_of_surface_frames;
		}
	}

	return ret;
}

VideoDecoder_Error_E VideoDecoder_Deinit(const VideoDecoder_T *ptVideoDecoder)
{
	VideoDecoder_Error_E ret = (VideoDecoder_Error_E)VideoDecoder_ErrorNone;

	if (ptVideoDecoder == NULL) {
		ret = VideoDecoder_ErrorBadParameter;
	} else {
		const JP_D_LinkedList_IF_T *ptListDecFrame = ptVideoDecoder->ptListDecFrame;

		vd_res_deinit_resource(ptVideoDecoder->ptResource);

		(void)ptListDecFrame->DeleteListAll(ptListDecFrame->ptListSt);
		(void)VideoIndexCtrl_Reset(ptVideoDecoder->ptVideoIndexCtrl);
	}

	return ret;
}

static void SF_VideoDecoder_ReleaseFrame(int iDisplayIndex, void *pvUserPrivate)
{
	if ((pvUserPrivate != NULL) && (iDisplayIndex >= 0)) {
		char strName[VD_SHORT_STR_MAX] = "";

		VideoDecoder_T *ptVideoDecoder = NULL;
		JP_D_LinkedList_T *hList = NULL;
		const JP_D_LinkedList_IF_T *ptListDecFrame = NULL;

		VD_CAST(ptVideoDecoder, pvUserPrivate);
		ptListDecFrame = ptVideoDecoder->ptListDecFrame;

		VD_SNPRINT(strName, VD_SHORT_STR_MAX, "%d", iDisplayIndex);

		hList = ptListDecFrame->FindList(ptListDecFrame->ptListSt, strName);

		(void)ptListDecFrame->DeleteList(hList);
		(void)vd_res_release_frame(ptVideoDecoder->ptResource, iDisplayIndex);
	}
}

VideoDecoder_T *VideoDecoder_Create(void)
{
	VideoDecoder_T *ptVideoDecoder = NULL;

	void *tmpPtr = VD_CALLOC(sizeof(VideoDecoder_T));

	if (tmpPtr != NULL) {
		int iRetVal = 0;

		VD_CAST(ptVideoDecoder, tmpPtr);
		ptVideoDecoder->ptListDecFrame = JP_CreateList();

		if (ptVideoDecoder->ptListDecFrame == NULL) {
			iRetVal = -1;
		} else {
			ptVideoDecoder->ptVideoIndexCtrl = VideoIndexCtrl_Create();

			if (ptVideoDecoder->ptVideoIndexCtrl != NULL) {
				VideoIndexCtrl_CallBack_T tCallBack;
				tCallBack.pvUserPrivate 	= ptVideoDecoder;
				tCallBack.fnReleaseFrame 	= SF_VideoDecoder_ReleaseFrame;
				iRetVal = VideoIndexCtrl_SetConfig(ptVideoDecoder->ptVideoIndexCtrl, VideoIndexCtrl_Config_CallBack, &tCallBack);
			} else {
				iRetVal = -2;
			}
		}

		if (iRetVal != 0) {
			VD_ERROR_MSG("[%s(%d)] [ERROR] Failed to create video decoder \n", __func__, __LINE__);

			if (ptVideoDecoder->ptListDecFrame != NULL) {
				(void)JP_DestroyList(ptVideoDecoder->ptListDecFrame);
				ptVideoDecoder->ptListDecFrame = NULL;
			}

			if (ptVideoDecoder->ptVideoIndexCtrl != NULL) {
				(void)VideoIndexCtrl_Destroy(ptVideoDecoder->ptVideoIndexCtrl);
				ptVideoDecoder->ptVideoIndexCtrl = NULL;
			}

			VD_FREE(ptVideoDecoder);
			ptVideoDecoder = NULL;
		}
	}

	return ptVideoDecoder;
}

VideoDecoder_Error_E VideoDecoder_Destroy(VideoDecoder_T *ptVideoDecoder)
{
	VideoDecoder_Error_E ret = (VideoDecoder_Error_E)VideoDecoder_ErrorNone;

	if (ptVideoDecoder == NULL) {
		ret = VideoDecoder_ErrorBadParameter;
	} else {
		if (ptVideoDecoder->eCodec != VideoDecoder_Codec_None) {
			vd_res_destory_resource(ptVideoDecoder->ptResource);
		}

		if (ptVideoDecoder->ptListDecFrame != NULL) {
			(void)JP_DestroyList(ptVideoDecoder->ptListDecFrame);
			ptVideoDecoder->ptListDecFrame = NULL;
		}

		if (ptVideoDecoder->ptVideoIndexCtrl != NULL) {
			(void)VideoIndexCtrl_Destroy(ptVideoDecoder->ptVideoIndexCtrl);
			ptVideoDecoder->ptVideoIndexCtrl = NULL;
		}

		VD_FREE(ptVideoDecoder);
	}

	return ret;
}

static VideoDecoder_Error_E SF_VDec_SetCodec(VideoDecoder_T *ptVideoDecoder, VideoDecoder_Codec_E eCodec)
{
	VideoDecoder_Error_E eRetErr = VideoDecoder_ErrorNone;

	if (ptVideoDecoder == NULL) {
		eRetErr = VideoDecoder_ErrorBadParameter;
	} else {
#if defined(TCC_VPU_LEGACY)
		ptVideoDecoder->eType = VD_TYPE_VPU_LEGACY;
#elif defined(TCC_VPU_V3)
		ptVideoDecoder->eType = VD_TYPE_VPU_V3;
#endif
		ptVideoDecoder->ptResource 	= vd_res_create_resource(eCodec, ptVideoDecoder->eType);

		if (ptVideoDecoder->ptResource != NULL) {
			ptVideoDecoder->eCodec = eCodec;
		} else {
			VD_ERROR_MSG("[%s(%d)] [ERROR] RESSOURCE \n", __func__, __LINE__);
			ptVideoDecoder->ptResource = NULL;
			ptVideoDecoder->eCodec = VideoDecoder_Codec_None;
			eRetErr = VideoDecoder_ErrorNotSupportedCodec;
		}
	}

	return eRetErr;
}

static VideoDecoder_Error_E SF_VDec_GetOutputFrame(VideoDecoder_T *ptVideoDecoder, VideoDecoder_Frame_T *ptFrame, const VideoDecoder_DecodingResult_T *ptDecodingResult)
{
	VideoDecoder_Error_E ret = (VideoDecoder_Error_E)VideoDecoder_ErrorNone;

	char strName[VD_SHORT_STR_MAX];
	void *ptTmpPtr = NULL;

	VideoClock_Clock_T tClock = {0, };
	VideoClock_T *ptVideoClock = NULL;

	ST_DecFrameListData_T *ptListData = NULL;
	const JP_D_LinkedList_T *hList = NULL;
	const JP_D_LinkedList_IF_T *ptListDecFrame = NULL;

	ptVideoClock = &ptVideoDecoder->tVideoClock;
	ptListDecFrame = ptVideoDecoder->ptListDecFrame;

	VD_SNPRINT(strName, VD_SHORT_STR_MAX, "%d", ptDecodingResult->iDispOutIdx);

	hList = ptListDecFrame->FindList(ptListDecFrame->ptListSt, strName);

	if (hList != NULL) {
		ptTmpPtr = ptListDecFrame->GetListData(hList);
	}

	if (ptTmpPtr != NULL) {
		VD_CAST(ptListData, ptTmpPtr);

		(void)memset(ptFrame, 0, sizeof(VideoDecoder_Frame_T));

		(void)VideoClockCtrl_GetClock(ptVideoClock, &tClock);

		ptFrame->uiID 				= ptListData->uiID;
		ptFrame->bInterlaced 		= ptListData->bInterlaced;
		ptFrame->bOddFieldFirst 	= ptListData->bOddFieldFirst;

		ptFrame->fAspectRatio 		= ptListData->fAspectRatio;
		ptFrame->uiWidth 			= ptListData->uiWidth;
		ptFrame->uiHeight 			= ptListData->uiHeight;

		ptFrame->tCropRect.uiTop 	= ptListData->tCropRect.uiTop;
		ptFrame->tCropRect.uiLeft 	= ptListData->tCropRect.uiLeft;
		ptFrame->tCropRect.uiWidth 	= ptListData->tCropRect.uiWidth;
		ptFrame->tCropRect.uiHeight = ptListData->tCropRect.uiHeight;

		ptFrame->iTimeStamp 		= tClock.iClock;
		ptFrame->iDisplayIndex 		= ptDecodingResult->iDispOutIdx;

		ptFrame->pucPhyAddr[VideoDecoder_Comp_Y] = ptDecodingResult->pucDispOut[VideoDecoder_Addr_PA][VideoDecoder_Comp_Y];
		ptFrame->pucPhyAddr[VideoDecoder_Comp_U] = ptDecodingResult->pucDispOut[VideoDecoder_Addr_PA][VideoDecoder_Comp_U];
		ptFrame->pucPhyAddr[VideoDecoder_Comp_V] = ptDecodingResult->pucDispOut[VideoDecoder_Addr_PA][VideoDecoder_Comp_V];

		ptFrame->pucVirAddr[VideoDecoder_Comp_Y] = ptDecodingResult->pucDispOut[VideoDecoder_Addr_VA][VideoDecoder_Comp_Y];
		ptFrame->pucVirAddr[VideoDecoder_Comp_U] = ptDecodingResult->pucDispOut[VideoDecoder_Addr_VA][VideoDecoder_Comp_U];
		ptFrame->pucVirAddr[VideoDecoder_Comp_V] = ptDecodingResult->pucDispOut[VideoDecoder_Addr_VA][VideoDecoder_Comp_V];
	} else {
		ret = (VideoDecoder_Error_E)VideoDecoder_ErrorGetOutputFrame;
	}

	return ret;
}