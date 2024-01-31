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

#include <TU_JP_Lib_C_Std.h>
#include <TU_JP_Lib_C_D_LinkedList.h>

#include "video_clock_control.h"
#include "video_index_control.h"

#include "video_decoder.h"

#define DEFAULT_OUTPUT_WIDTH_MAX				1920
#define DEFAULT_OUTPUT_HEIGHT_MAX				1088
#define DEFAULT_OUTPUT_FRAME_RATE   			30000

#define MODE_DECODED_FRAME_OUTPUT	(1 << 1)

#define SET_MODE(private_data, flag)    ((private_data)->ulModeFlags |= (flag))
#define CLEAR_MODE(private_data, flag)  ((private_data)->ulModeFlags &= ~(flag))
#define CHECK_MODE(private_data, flag)  ((private_data)->ulModeFlags & (flag))

#define VDEC_FUNC(pt_video_decoder, opcode, handle, param1, param2)									\
	(pt_video_decoder)->pfVDecFunc(opcode, handle, param1, param2, (pt_video_decoder)->pvVDecInst)

#define SKIPMODE_NONE               0
#define SKIPMODE_I_SEARCH			1
#define SKIPMODE_B_SKIP				2
#define SKIPMODE_EXCEPT_I_SKIP		3
#define SKIPMODE_NEXT_B_SKIP        4
#define SKIPMODE_B_WAIT             5
#define SKIPMODE_NEXT_STEP          10

#define SKIPMODE_IDR_SEARCH         21
#define SKIPMODE_NONIDR_SEARCH      22

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
	void *pvVDecInst;

	bool bCodecSet;
	bool bFristFrameDecoded;

	unsigned long ulModeFlags;
	unsigned long ulFrameSkipMode;

	cdk_func_t *pfVDecFunc;

	vdec_init_t tVDecInit;
	vdec_input_t tVDecInput;
	vdec_output_t tVDecOutput;
	vdec_user_info_t tVDecUserInfo;

	VideoDecoder_Rect_T tCropRect;

	VideoClock_T tVideoClock;

	JP_D_LinkedList_IF_T *ptListDecFrame;
	VideoIndexCtrl_T *ptVideoIndexCtrl;

	uint64_t uiFrameID;
};

static unsigned long SF_VDec_GetFrameType(unsigned long ulCodecType, unsigned long ulPictureType, unsigned long ulPictureStructure)
{
	unsigned long ulFrameType;

	switch(ulCodecType)
	{
    case STD_AVC:
		{
			if(ulPictureStructure == 1)
			{
				// FIELD_INTERLACED
				if((ulPictureType >> 3) == PIC_TYPE_I)
				{
					ulFrameType = PIC_TYPE_I;
				}
				else if((ulPictureType >> 3) == PIC_TYPE_P)
				{
					ulFrameType = PIC_TYPE_P;
				}
				else if((ulPictureType >> 3) == 2)
				{
					ulFrameType = PIC_TYPE_B;
				}
				else
				{
					ulFrameType = PIC_TYPE_I;
				}

				if((ulPictureType & 0x7) == PIC_TYPE_I)
				{
					ulFrameType = PIC_TYPE_I;
				}
				else if((ulPictureType & 0x7) == PIC_TYPE_P)
				{
					ulFrameType = PIC_TYPE_P;
				}
				else if((ulPictureType & 0x7) == 2)
				{
					ulFrameType = PIC_TYPE_B;
				}
			}
			else
			{
				if(ulPictureType == PIC_TYPE_I)
				{
					ulFrameType = PIC_TYPE_I;
				}
				else if(ulPictureType == PIC_TYPE_P)
				{
					ulFrameType = PIC_TYPE_P;
				}
				else if(ulPictureType == 2)
				{
					ulFrameType = PIC_TYPE_B;
				}
			}
		}
		break;
	case STD_MPEG2:
	default:
		switch(ulPictureType & 0xf)
		{
		case PIC_TYPE_I:
			ulFrameType = PIC_TYPE_I;
			break;
		case PIC_TYPE_P:
			ulFrameType = PIC_TYPE_P;
			break;
		case PIC_TYPE_B:
			ulFrameType = PIC_TYPE_B;
			break;
		}
	}

	return ulFrameType;
}

static unsigned long SF_VDec_GetDecodedFrameType(VideoDecoder_T *ptVideoDecoder)
{
	vdec_init_t *ptVDecInit 		= &ptVideoDecoder->tVDecInit;
	vdec_output_t *ptVDecOutput 	= &ptVideoDecoder->tVDecOutput;

    return SF_VDec_GetFrameType(ptVDecInit->m_iBitstreamFormat, ptVDecOutput->m_DecOutInfo.m_iPicType, ptVDecOutput->m_DecOutInfo.m_iPictureStructure);
}

static void SF_VDec_SetFrameSkipMode(VideoDecoder_T *ptVideoDecoder, unsigned long ulSkipMode)
{
	vdec_init_t *ptVDecInit 		= &ptVideoDecoder->tVDecInit;
	vdec_input_t *ptVDecInput 		= &ptVideoDecoder->tVDecInput;
	vdec_output_t *ptVDecOutput 	= &ptVideoDecoder->tVDecOutput;

	switch(ulSkipMode)
	{
	case SKIPMODE_NONE:
	case SKIPMODE_NEXT_B_SKIP:
	case SKIPMODE_B_WAIT:
		{
			ptVideoDecoder->ulFrameSkipMode = ulSkipMode;

			ptVDecInput->m_iSkipFrameNum      	= 0;
			ptVDecInput->m_iFrameSearchEnable 	= 0;
			ptVDecInput->m_iSkipFrameMode     	= 0;
		}
		break;
	case SKIPMODE_IDR_SEARCH:
		{
			ptVideoDecoder->ulFrameSkipMode = SKIPMODE_I_SEARCH;

			ptVDecInput->m_iSkipFrameNum      = 0;
			ptVDecInput->m_iSkipFrameMode     = 0;

			if(CHECK_MODE(ptVideoDecoder, MODE_DECODED_FRAME_OUTPUT))
			{
				ptVDecInput->m_iFrameSearchEnable = AVC_NONIDR_PICTURE_SEARCH_MODE;

				//printf("I-frame Search (non-IDR slice search)\n");
			}
			else
			{
				ptVDecInput->m_iFrameSearchEnable = AVC_IDR_PICTURE_SEARCH_MODE;

				//printf("I-frame Search (IDR slice search)\n");
			}
		}
		break;
	case SKIPMODE_NONIDR_SEARCH:
		{
			ptVideoDecoder->ulFrameSkipMode = SKIPMODE_I_SEARCH;

			ptVDecInput->m_iSkipFrameNum 	= 0;
			ptVDecInput->m_iSkipFrameMode 	= 0;

			ptVDecInput->m_iFrameSearchEnable = AVC_NONIDR_PICTURE_SEARCH_MODE;

			//printf("I-frame Search (non-IDR slice search)\n");
		}
		break;
	case SKIPMODE_I_SEARCH:
		{
			ptVideoDecoder->ulFrameSkipMode = SKIPMODE_I_SEARCH;

			ptVDecInput->m_iSkipFrameNum 	= 0;
			ptVDecInput->m_iSkipFrameMode 	= 0;

			if(ptVDecInit->m_iBitstreamFormat == STD_AVC)
			{
				if(CHECK_MODE(ptVideoDecoder, MODE_DECODED_FRAME_OUTPUT))
				{
					ptVDecInput->m_iFrameSearchEnable = AVC_NONIDR_PICTURE_SEARCH_MODE;

					//printf("I-frame Search (non-IDR slice search)\n");
				}
				else
				{
					ptVDecInput->m_iFrameSearchEnable = AVC_NONIDR_PICTURE_SEARCH_MODE;

					//printf("I-frame Search (non-IDR slice search)\n");
				}
			}
			else
			{
				ptVDecInput->m_iFrameSearchEnable = AVC_IDR_PICTURE_SEARCH_MODE;

				//printf("I-frame Search\n");
			}
		}
		break;
	case SKIPMODE_B_SKIP:
		{
			ptVideoDecoder->ulFrameSkipMode = ulSkipMode;

			ptVDecInput->m_iSkipFrameNum 		= 1;
			ptVDecInput->m_iFrameSearchEnable 	= 0;
			ptVDecInput->m_iSkipFrameMode 		= VDEC_SKIP_FRAME_ONLY_B;

			//printf("B-frame Skip\n");
		}
		break;
	case SKIPMODE_EXCEPT_I_SKIP:
		{
			ptVideoDecoder->ulFrameSkipMode = ulSkipMode;

			ptVDecInput->m_iSkipFrameNum 		= 1;
			ptVDecInput->m_iFrameSearchEnable 	= 0;
			ptVDecInput->m_iSkipFrameMode 		= VDEC_SKIP_FRAME_EXCEPT_I;

			//printf("Except-I-frame Skip\n");
		}
		break;
	case SKIPMODE_NEXT_STEP:
		{
			switch(ptVideoDecoder->ulFrameSkipMode)
			{
			case SKIPMODE_I_SEARCH:
				{
					if(ptVDecInit->m_iBitstreamFormat == STD_HEVC)
					{
						SF_VDec_SetFrameSkipMode(ptVideoDecoder, SKIPMODE_B_WAIT);
					}
					else
					{
						if(ptVDecOutput->m_DecOutInfo.m_iDecodingStatus == VPU_DEC_SUCCESS_FIELD_PICTURE)
						{
							SF_VDec_SetFrameSkipMode(ptVideoDecoder, SKIPMODE_NEXT_B_SKIP);
						}
						else
						{
							SF_VDec_SetFrameSkipMode(ptVideoDecoder, SKIPMODE_B_SKIP);
						}
					}
				}
				break;
			case SKIPMODE_NEXT_B_SKIP:
				{
					SF_VDec_SetFrameSkipMode(ptVideoDecoder, SKIPMODE_B_SKIP);
				}
				break;
			case SKIPMODE_B_SKIP:
				{
                    if(SF_VDec_GetDecodedFrameType(ptVideoDecoder) != PIC_TYPE_B)
					{
                        SF_VDec_SetFrameSkipMode(ptVideoDecoder, SKIPMODE_NONE);
					}
                }
				break;
			case SKIPMODE_EXCEPT_I_SKIP:
				{
					SF_VDec_SetFrameSkipMode(ptVideoDecoder, SKIPMODE_NONE);
				}
				break;
			case SKIPMODE_B_WAIT:
				{
					if(SF_VDec_GetDecodedFrameType(ptVideoDecoder) == PIC_TYPE_B)
					{
						SF_VDec_SetFrameSkipMode(ptVideoDecoder, SKIPMODE_NONE);
					}
				}
				break;
			default:
				{
					SF_VDec_SetFrameSkipMode(ptVideoDecoder, SKIPMODE_NONE);
				}
				break;
			}
		}
		break;
	}
}

static void SF_VDec_EnableIFrameSearch(VideoDecoder_T *ptVideoDecoder)
{
	vdec_init_t *ptVDecInit = &ptVideoDecoder->tVDecInit;

	if(ptVDecInit->m_iBitstreamFormat == STD_AVC)
	{
		SF_VDec_SetFrameSkipMode(ptVideoDecoder, SKIPMODE_NONIDR_SEARCH);
	}
	else
	{
		SF_VDec_SetFrameSkipMode(ptVideoDecoder, SKIPMODE_I_SEARCH);
	}
}

static int SF_VDec_GetRefFrameCount(VideoDecoder_T *ptVideoDecoder)
{
	if(ptVideoDecoder == NULL)
	{
		return -1;
	}
	return vpu_get_refframe_count(ptVideoDecoder->pvVDecInst);
}

static VideoDecoder_Error_E SF_VDec_SetCodec(VideoDecoder_T *ptVideoDecoder, VideoDecoder_Codec_E eCodec)
{
	VideoDecoder_Error_E eRetErr = VideoDecoder_ErrorNone;

	vdec_init_t *ptVDecInit;

	if(ptVideoDecoder == NULL)
	{
		return VideoDecoder_ErrorBadParameter;
	}

	ptVDecInit = &ptVideoDecoder->tVDecInit;

	switch(eCodec)
	{
	case VideoDecoder_Codec_MPEG2:
		ptVDecInit->m_iBitstreamFormat = STD_MPEG2;
		break;
	case VideoDecoder_Codec_H264:
		ptVDecInit->m_iBitstreamFormat = STD_AVC;
		break;
	case VideoDecoder_Codec_MPEG4:
		ptVDecInit->m_iBitstreamFormat = STD_MPEG4;
		break;
#if defined(TCC_HEVC_INCLUDE) || defined(TCC_VPU_4K_D2_INCLUDE)
	case VideoDecoder_Codec_H265:
		ptVDecInit->m_iBitstreamFormat = STD_HEVC;
		break;
#endif
#if defined(TCC_VP9_INCLUDE) || defined(TCC_VPU_4K_D2_INCLUDE)
	case VideoDecoder_Codec_VP9:
		ptVDecInit->m_iBitstreamFormat = STD_VP9;
		break;
#endif
	case VideoDecoder_Codec_VP8:
		ptVDecInit->m_iBitstreamFormat = STD_VP8;
		break;
	case VideoDecoder_Codec_H263:
		ptVDecInit->m_iBitstreamFormat = STD_H263;
		break;
	case VideoDecoder_Codec_MJPEG:
		ptVDecInit->m_iBitstreamFormat = STD_MJPG;
		break;
	default:
		eRetErr = VideoDecoder_ErrorNotSupportedCodec;
		break;
	}

	if(eRetErr == VideoDecoder_ErrorNone)
	{
		ptVideoDecoder->bCodecSet = true;
	}

	return eRetErr;
}

static void SF_VDec_GetOutputFrame(VideoDecoder_T *ptVideoDecoder, VideoDecoder_Frame_T *ptFrame)
{
	char strName[50];

	JP_D_LinkedList_T* hList;

	VideoClock_Clock_T tClock;

	ST_DecFrameListData_T *ptListData;

	vdec_init_t *ptVDecInit;
	vdec_output_t *ptVDecOutput;

	VideoClock_T *ptVideoClock;

	JP_D_LinkedList_IF_T *ptListDecFrame;

	ptVDecInit 		= &ptVideoDecoder->tVDecInit;
	ptVDecOutput 	= &ptVideoDecoder->tVDecOutput;

	ptVideoClock = &ptVideoDecoder->tVideoClock;

	ptListDecFrame = ptVideoDecoder->ptListDecFrame;

	sprintf(strName, "%d", ptVDecOutput->m_DecOutInfo.m_iDispOutIdx);

	hList = ptListDecFrame->FindList(ptListDecFrame->ptListSt, strName);

	ptListData = (ST_DecFrameListData_T *)ptListDecFrame->GetListData(hList);

	memset(ptFrame, 0, sizeof(VideoDecoder_Frame_T));

	VideoClockCtrl_GetClock(ptVideoClock, &tClock);

	ptFrame->uiID 			= ptListData->uiID;
	ptFrame->iTimeStamp 	= tClock.iClock;

	ptFrame->iDisplayIndex = ptVDecOutput->m_DecOutInfo.m_iDispOutIdx;

	ptFrame->bInterlaced 		= ptListData->bInterlaced;
	ptFrame->bOddFieldFirst 	= ptListData->bOddFieldFirst;

	ptFrame->fAspectRatio = ptListData->fAspectRatio;

	ptFrame->pucPhyAddr[0] = ptVDecOutput->m_pDispOut[PA][0];
	ptFrame->pucPhyAddr[1] = ptVDecOutput->m_pDispOut[PA][1];
	ptFrame->pucPhyAddr[2] = ptVDecOutput->m_pDispOut[PA][2];

	ptFrame->pucVirAddr[0] = ptVDecOutput->m_pDispOut[VA][0];
	ptFrame->pucVirAddr[1] = ptVDecOutput->m_pDispOut[VA][1];
	ptFrame->pucVirAddr[2] = ptVDecOutput->m_pDispOut[VA][2];

	ptFrame->uiWidth 	= ptListData->uiWidth;
	ptFrame->uiHeight 	= ptListData->uiHeight;

	ptFrame->tCropRect.uiTop 		= ptListData->tCropRect.uiTop;
	ptFrame->tCropRect.uiLeft 		= ptListData->tCropRect.uiLeft;
	ptFrame->tCropRect.uiWidth 		= ptListData->tCropRect.uiWidth;
	ptFrame->tCropRect.uiHeight 	= ptListData->tCropRect.uiHeight;
}

unsigned int VideoDecoder_GetCbCrInterleaveMode(VideoDecoder_T *ptVideoDecoder)
{
	vdec_init_t *ptVDecInit = NULL;

	if(ptVideoDecoder == NULL)
	{
		return 0U;
	}
	ptVDecInit = &ptVideoDecoder->tVDecInit;

	return (unsigned int)ptVDecInit->m_bCbCrInterleaveMode;
}

VideoDecoder_Error_E VideoDecoder_Flush(VideoDecoder_T *ptVideoDecoder)
{
	return VideoDecoder_ErrorNotSupportedCodec;
}

VideoDecoder_Error_E VideoDecoder_EnableIFrameSearch(VideoDecoder_T *ptVideoDecoder)
{
	return VideoDecoder_ErrorNotSupportedCodec;
}

VideoDecoder_Error_E VideoDecoder_DecodeSequenceHeader(VideoDecoder_T *ptVideoDecoder, VideoDecoder_SequenceHeaderInputParam_T* ptParam)
{
	int iVal;
	unsigned int uiFrameRate;
	unsigned int uiFrameRateRes = 0;
	unsigned int uiFrameRateDiv = 0;
	int framebufferdelayCount = 0;
	vdec_init_t *ptVDecInit;
	vdec_input_t *ptVDecInput;
	vdec_output_t *ptVDecOutput;

	if((ptVideoDecoder == NULL) || (ptParam == NULL))
	{
		return VideoDecoder_ErrorBadParameter;
	}

	if((ptParam->pucStream == NULL) || (ptParam->uiStreamLen == 0))
	{
		return VideoDecoder_ErrorBadParameter;
	}

	ptVDecInit 		= &ptVideoDecoder->tVDecInit;
	ptVDecInput 	= &ptVideoDecoder->tVDecInput;
	ptVDecOutput 	= &ptVideoDecoder->tVDecOutput;

	ptVDecInput->m_pInp[PA] 	= ptParam->pucStream;
	ptVDecInput->m_pInp[VA] 	= ptParam->pucStream;
	ptVDecInput->m_iInpLen 		= ptParam->uiStreamLen;

	ptVDecInput->m_iIsThumbnail = 0;
	vpu_set_additional_refframe_count(ptParam->uiNumOutputFrames, ptVideoDecoder->pvVDecInst);

	iVal = VDEC_FUNC(ptVideoDecoder, VDEC_DEC_SEQ_HEADER, NULL, ptVDecInput, ptVDecOutput);

	if(iVal < 0)
	{
		printf("[VDEC_ERROR] [OP: VDEC_DEC_SEQ_HEADER] [RET_CODE: %d]\n", iVal);
		return VideoDecoder_ErrorDecodingFail;
	}

	if(ptVideoDecoder->ptVideoIndexCtrl != NULL)
	{
		VideoIndexCtrl_MaxIndexCnt_T tMaxIndexCnt;
		tMaxIndexCnt.iMaxIndexCnt = ptParam->uiNumOutputFrames;
		if(tMaxIndexCnt.iMaxIndexCnt < 1)
		{
			iVal = -1;
		}
		else
		{
			iVal = VideoIndexCtrl_SetConfig(ptVideoDecoder->ptVideoIndexCtrl, VideoIndexCtrl_Config_MaxIndexCnt, &tMaxIndexCnt);
		}
	}
	else
	{
		iVal = -1;
	}

	if(iVal < 0)
	{
		return VideoDecoder_ErrorDecodingFail;
	}
	return VideoDecoder_ErrorNone;
}

VideoDecoder_Error_E VideoDecoder_Decode(
	VideoDecoder_T *ptVideoDecoder,
	VideoDecoder_StreamInfo_T *ptStream,
	VideoDecoder_Frame_T *ptFrame)
{
	int iVal;
	unsigned long ulResult = 0;
	bool bField = false;

	VideoClock_T *ptVideoClock;
	vdec_init_t *ptVDecInit;
	vdec_input_t *ptVDecInput;
	vdec_output_t *ptVDecOutput;
	dec_output_info_t *ptDecOutInfo;

	if(ptVideoDecoder == NULL)
	{
		return VideoDecoder_ErrorBadParameter;
	}

	ptVideoClock = &ptVideoDecoder->tVideoClock;

	if(ptVideoClock == NULL)
	{
		return VideoDecoder_ErrorBadParameter;
	}

	if(ptVideoDecoder->ptVideoIndexCtrl == NULL)
	{
		return VideoDecoder_ErrorBadParameter;
	}

	VideoIndexCtrl_Update(ptVideoDecoder->ptVideoIndexCtrl);
	VideoIndexCtrl_Clear(ptVideoDecoder->ptVideoIndexCtrl);

	ptVDecInit 		= &ptVideoDecoder->tVDecInit;
	ptVDecInput 	= &ptVideoDecoder->tVDecInput;
	ptVDecOutput 	= &ptVideoDecoder->tVDecOutput;

	ptDecOutInfo = &ptVDecOutput->m_DecOutInfo;

	ptVDecInput->m_pInp[PA] 	= ptStream->pucStream;
	ptVDecInput->m_pInp[VA] 	= ptStream->pucStream;
	ptVDecInput->m_iInpLen 		= ptStream->uiStreamLen;

	/* resolution with re-init step3 - flush delayed output frame */
	iVal = VDEC_FUNC(ptVideoDecoder, VDEC_DECODE, NULL, ptVDecInput, ptVDecOutput);

	if(iVal < 0)
	{
		printf("[VDEC_ERROR] [OP: VDEC_DECODE] [RET_CODE: %d]\n", iVal);
		return VideoDecoder_ErrorDecodingFail;
	}
	else
	{
		if(ptVideoDecoder->bFristFrameDecoded == false)
		{
			struct timespec tspec;
			(void)clock_gettime(CLOCK_MONOTONIC , &tspec);

			printf("[%lld.%3ld] [VDEC] Decode 1st video Frame Done. timestamp=%lld\n", (int64_t)tspec.tv_sec, tspec.tv_nsec / 1000, ptStream->iTimeStamp);
			ptVideoDecoder->bFristFrameDecoded = true;
		}
	}

	if((ptVideoDecoder->ulFrameSkipMode != SKIPMODE_NONE) && (ptDecOutInfo->m_iDecodedIdx >= 0))
	{
		SF_VDec_SetFrameSkipMode(ptVideoDecoder, SKIPMODE_NEXT_STEP);
	}

	//printf("[CHK] %s %d m_iDecodingStatus=0x%x iTimeStamp=%lld m_iDecodedIdx=%d \n",__func__, __LINE__, ptDecOutInfo->m_iDecodingStatus, ptStream->iTimeStamp, ptDecOutInfo->m_iDecodedIdx);

	if((ptDecOutInfo->m_iDecodingStatus == VPU_DEC_SUCCESS_FIELD_PICTURE) ||
		(ptDecOutInfo->m_iDecodingStatus == VPU_DEC_SUCCESS))
	{
		if(ptDecOutInfo->m_iDecodingStatus == VPU_DEC_SUCCESS_FIELD_PICTURE)
		{
			switch(ptVDecInit->m_iBitstreamFormat)
			{
			case STD_MPEG2:
				{
					if(ptDecOutInfo->m_iM2vProgressiveFrame == 0)
					{
						bField = true;
					}
				}
				break;
			case STD_AVC:
				{
					if(ptDecOutInfo->m_iInterlacedFrame != 0)
					{
						bField = true;
					}
				}
				break;
			default:
				{
				}
				break;
			}
		}
		else
		{
			char strName[50];

			ST_DecFrameListData_T tListData;

			VideoDecoder_Rect_T tCropRect;

			JP_D_LinkedList_IF_T *ptListDecFrame = ptVideoDecoder->ptListDecFrame;

			sprintf(strName, "%d", ptDecOutInfo->m_iDecodedIdx);

			tListData.uiID = ptVideoDecoder->uiFrameID;

			switch(ptVDecInit->m_iBitstreamFormat)
			{
			case STD_MPEG2:
				{
					if(((ptDecOutInfo->m_iM2vProgressiveFrame == 0) && (ptDecOutInfo->m_iPictureStructure == 3))
						|| ((ptDecOutInfo->m_iInterlacedFrame != 0) && (ptVDecOutput->m_pInitialInfo->m_iInterlace != 0)))
					{
						tListData.bInterlaced = true;

						if(ptDecOutInfo->m_iTopFieldFirst == 0)
						{
							tListData.bOddFieldFirst = true;
						}
					}
				}
				break;
			case STD_AVC:
				{
					if(((ptDecOutInfo->m_iM2vProgressiveFrame == 0) && (ptDecOutInfo->m_iPictureStructure == 3))
						|| ((ptDecOutInfo->m_iInterlacedFrame != 0) && (ptVDecOutput->m_pInitialInfo->m_iInterlace != 0)))
					{
						tListData.bInterlaced = true;

						if(ptDecOutInfo->m_iTopFieldFirst == 0)
						{
							tListData.bOddFieldFirst = true;
						}
					}
				}
				break;
			default:
				{
				}
				break;
			}

			if((ptDecOutInfo->m_iWidth == 1920) && (ptDecOutInfo->m_iHeight > 1080))
			{
				ptDecOutInfo->m_iHeight = 1080;
			}

			tListData.uiWidth 	= ptDecOutInfo->m_iWidth;
			tListData.uiHeight 	= ptDecOutInfo->m_iHeight;

			tCropRect.uiTop 	= ptDecOutInfo->m_CropInfo.m_iCropTop;
			tCropRect.uiLeft 	= ptDecOutInfo->m_CropInfo.m_iCropLeft;
			tCropRect.uiWidth 	= ptDecOutInfo->m_iWidth - ptDecOutInfo->m_CropInfo.m_iCropLeft - ptDecOutInfo->m_CropInfo.m_iCropRight;
			tCropRect.uiHeight 	= ptDecOutInfo->m_iHeight - ptDecOutInfo->m_CropInfo.m_iCropTop - ptDecOutInfo->m_CropInfo.m_iCropBottom;

			tListData.fAspectRatio = vdec_getAspectRatio(ptVideoDecoder->pvVDecInst, tCropRect.uiWidth, tCropRect.uiHeight);

			tListData.tCropRect.uiTop 		= tCropRect.uiTop;
			tListData.tCropRect.uiLeft 		= tCropRect.uiLeft;
			tListData.tCropRect.uiWidth 	= tCropRect.uiWidth;
			tListData.tCropRect.uiHeight 	= tCropRect.uiHeight;

			ptListDecFrame->InsertList(ptListDecFrame->ptListSt, strName, &tListData, sizeof(ST_DecFrameListData_T));
		}
	#if 0
		printf("[PUSH] %s %d PTS=%lld Pic0x%x Status(0x%x,0x%x) DecodedIdx=%d DisplayedIdx=%d\n",
			__func__, __LINE__, ptStream->iTimeStamp, ptVDecOutput->m_DecOutInfo.m_iPicType, ptDecOutInfo->m_iDecodingStatus,
			ptDecOutInfo->m_iOutputStatus, ptDecOutInfo->m_iDecodedIdx, ptDecOutInfo->m_iDispOutIdx);
	#endif
		VideoClockCtrl_InsertClock(ptVideoClock, ptStream->iTimeStamp, 0, bField, ptDecOutInfo->m_iDecodedIdx, false);
		ptVideoDecoder->uiFrameID++;
	}

	/* set decoding status */
	switch(ptDecOutInfo->m_iDecodingStatus)
	{
	case VPU_DEC_SUCCESS:
		{
			if(ptDecOutInfo->m_iDecodedIdx >= 0)
			{
				ulResult |= DECODING_SUCCESS_FRAME;
			}
			else if(ptDecOutInfo->m_iDecodedIdx == -2)
			{
				ulResult |= DECODING_SUCCESS_SKIPPED;
			}
		}
		break;
	case VPU_DEC_SUCCESS_FIELD_PICTURE:
		{
			ulResult |= DECODING_SUCCESS_FIELD;
		}
		break;
	case VPU_DEC_BUF_FULL:
		{
			ulResult |= DECODING_BUFFER_FULL;
		}
		break;
	case 6: //FIXME - resolution change (not defined value)
		{
			if(ptDecOutInfo->m_iDecodedIdx >= 0)
			{
				ulResult |= DECODING_SUCCESS_FRAME;
			}

			ulResult |= RESOLUTION_CHANGED;
		}
		break;
	}

	/* set output status */
	if(ptDecOutInfo->m_iOutputStatus == VPU_DEC_OUTPUT_SUCCESS)
	{
		ulResult |= DISPLAY_OUTPUT_SUCCESS;

		if(ptVideoDecoder->ptVideoIndexCtrl == NULL)
		{
			return VideoDecoder_ErrorDecodingFail;
		}
		SF_VDec_GetOutputFrame(ptVideoDecoder, ptFrame);
	}

	VideoIndexCtrl_Push(ptVideoDecoder->ptVideoIndexCtrl, ptVDecOutput->m_DecOutInfo.m_iDispOutIdx, ptVDecOutput->m_pDispOut[PA][0]);

	ptFrame->ulResult = ulResult;

	return VideoDecoder_ErrorNone;
}

VideoDecoder_Error_E VideoDecoder_Init(VideoDecoder_T *ptVideoDecoder, VideoDecoder_InitConfig_T *ptConfig)
{
	int iVal;
	VideoDecoder_Error_E ret = VideoDecoder_ErrorNone;

	vdec_init_t *ptVDecInit;
	vdec_user_info_t *ptVDecUserInfo;

	if(ptVideoDecoder == NULL)
	{
		return VideoDecoder_ErrorBadParameter;
	}

	ptVDecInit 		= &ptVideoDecoder->tVDecInit;
	ptVDecUserInfo 	= &ptVideoDecoder->tVDecUserInfo;

	ret = SF_VDec_SetCodec(ptVideoDecoder, ptConfig->eCodec);

	if(ret != VideoDecoder_ErrorNone)
	{
		return VideoDecoder_ErrorInitFail;
	}

	ptVDecInit->m_iPicWidth 			= ptVideoDecoder->tCropRect.uiWidth;
	ptVDecInit->m_iPicHeight 			= ptVideoDecoder->tCropRect.uiHeight;
	ptVDecInit->m_bEnableVideoCache 	= 0;
	ptVDecInit->m_pExtraData 			= NULL;
	ptVDecInit->m_iExtraDataLen 		= 0;
	ptVDecInit->m_uiMaxResolution 		= 0;
	ptVDecInit->m_bEnableUserData 		= 0;
	ptVDecInit->m_bFilePlayEnable 		= 1;
	ptVDecInit->m_bCbCrInterleaveMode 	= 1;

	ptVDecUserInfo->bitrate_mbps 		= 10;
	ptVDecUserInfo->frame_rate 			= DEFAULT_OUTPUT_FRAME_RATE;
	ptVDecUserInfo->extFunction 		= 0;

	if(ptVideoDecoder->pvVDecInst == NULL)
	{
		ptVideoDecoder->pvVDecInst = vdec_alloc_instance(ptVDecInit->m_iBitstreamFormat, 0);
		ptVideoDecoder->pfVDecFunc = gspfVDecList[ptVDecInit->m_iBitstreamFormat];
	}

	iVal = VDEC_FUNC(ptVideoDecoder, VDEC_INIT, NULL, ptVDecInit, ptVDecUserInfo);

	if(iVal < 0)
	{
		printf("[VDEC_ERROR] [OP: VDEC_INIT] [RET_CODE: %d]\n", iVal);

		if(iVal != -VPU_ENV_INIT_ERROR)
		{
			VDEC_FUNC(ptVideoDecoder, VDEC_CLOSE, NULL, NULL, &ptVideoDecoder->tVDecOutput);
		}

		return VideoDecoder_ErrorInitFail;
	}

	vpu_update_sizeinfo(
		ptVDecInit->m_iBitstreamFormat,
		ptVDecUserInfo->bitrate_mbps,
		ptVDecUserInfo->frame_rate,
		ptVDecInit->m_iPicWidth,
		ptVDecInit->m_iPicHeight,
		ptVideoDecoder->pvVDecInst);

	SF_VDec_EnableIFrameSearch(ptVideoDecoder);

	VideoClockCtrl_ResetClock(&ptVideoDecoder->tVideoClock);

	ptVideoDecoder->uiFrameID = 0U;

	return VideoDecoder_ErrorNone;
}

VideoDecoder_Error_E VideoDecoder_Deinit(VideoDecoder_T *ptVideoDecoder)
{
	JP_D_LinkedList_IF_T *ptListDecFrame;

	if(ptVideoDecoder == NULL)
	{
		return VideoDecoder_ErrorBadParameter;
	}

	ptListDecFrame = ptVideoDecoder->ptListDecFrame;

	VDEC_FUNC(ptVideoDecoder, VDEC_CLOSE, NULL, NULL, &ptVideoDecoder->tVDecOutput);

	vdec_release_instance(ptVideoDecoder->pvVDecInst);

	ptListDecFrame->DeleteListAll(ptListDecFrame->ptListSt);

	VideoIndexCtrl_Reset(ptVideoDecoder->ptVideoIndexCtrl);

	return VideoDecoder_ErrorNone;
}

static void SF_VideoDecoder_ReleaseFrame(int iDisplayIndex, void *pvUserPrivate)
{
	VideoDecoder_T *ptVideoDecoder = (VideoDecoder_T *)pvUserPrivate;
	char strName[50];

	JP_D_LinkedList_T* hList;

	JP_D_LinkedList_IF_T *ptListDecFrame;

	if((ptVideoDecoder != NULL) && (iDisplayIndex >= 0))
	{
		ptListDecFrame = ptVideoDecoder->ptListDecFrame;

		sprintf(strName, "%d", iDisplayIndex);

		hList = ptListDecFrame->FindList(ptListDecFrame->ptListSt, strName);

		ptListDecFrame->DeleteList(hList);

		VDEC_FUNC(ptVideoDecoder, VDEC_BUF_FLAG_CLEAR, NULL, &iDisplayIndex, NULL);
	}
}

VideoDecoder_T *VideoDecoder_Create(void)
{
	VideoDecoder_T *ptVideoDecoder;
	int iRetVal = 0;

	ptVideoDecoder = (VideoDecoder_T *)calloc(1, sizeof(VideoDecoder_T));

	if(ptVideoDecoder == NULL)
	{
		return NULL;
	}

	ptVideoDecoder->tCropRect.uiTop 	= 0;
	ptVideoDecoder->tCropRect.uiLeft 	= 0;
	ptVideoDecoder->tCropRect.uiWidth 	= DEFAULT_OUTPUT_WIDTH_MAX;
	ptVideoDecoder->tCropRect.uiHeight 	= DEFAULT_OUTPUT_HEIGHT_MAX;

	ptVideoDecoder->ptListDecFrame = JP_CreateList();

	if(ptVideoDecoder->ptListDecFrame == NULL)
	{
		iRetVal = -1;
	}
	else
	{
		ptVideoDecoder->ptVideoIndexCtrl = VideoIndexCtrl_Create();

		if(ptVideoDecoder->ptVideoIndexCtrl != NULL)
		{
			VideoIndexCtrl_CallBack_T tCallBack;
			tCallBack.pvUserPrivate 	= ptVideoDecoder;
			tCallBack.fnReleaseFrame 	= SF_VideoDecoder_ReleaseFrame;
			iRetVal = VideoIndexCtrl_SetConfig(ptVideoDecoder->ptVideoIndexCtrl, VideoIndexCtrl_Config_CallBack, &tCallBack);
		}
		else
		{
			iRetVal = -2;
		}
	}

	if(iRetVal != 0)
	{
		printf("[%s:%d] Failed to create video decoder \n", __func__, __LINE__);
		if(ptVideoDecoder->ptListDecFrame != NULL)
		{
			JP_DestroyList(ptVideoDecoder->ptListDecFrame);
			ptVideoDecoder->ptListDecFrame = NULL;
		}
		if(ptVideoDecoder->ptVideoIndexCtrl != NULL)
		{
			VideoIndexCtrl_Destroy(ptVideoDecoder->ptVideoIndexCtrl);
			ptVideoDecoder->ptVideoIndexCtrl = NULL;
		}
		free(ptVideoDecoder);
		ptVideoDecoder = NULL;
	}
	return ptVideoDecoder;
}

VideoDecoder_Error_E VideoDecoder_Destroy(VideoDecoder_T *ptVideoDecoder)
{
	if(ptVideoDecoder == NULL)
	{
		return VideoDecoder_ErrorBadParameter;
	}

	if(ptVideoDecoder->ptListDecFrame != NULL)
	{
		JP_DestroyList(ptVideoDecoder->ptListDecFrame);
		ptVideoDecoder->ptListDecFrame = NULL;
	}

	if(ptVideoDecoder->ptVideoIndexCtrl != NULL)
	{
		VideoIndexCtrl_Destroy(ptVideoDecoder->ptVideoIndexCtrl);
		ptVideoDecoder->ptVideoIndexCtrl = NULL;
	}

	free(ptVideoDecoder);

	return VideoDecoder_ErrorNone;
}