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


#include <decoder_plugin_video.h>
#include <decoder_plugin_video_common.h>
#include <decoder_plugin_video_frame.h>


static void video_plugin_frame_set_next_frame_skip_mode(VPC_FRAME_SKIP_T *ptFrameSkip, unsigned int uiVideoCodec, int iDecodingStatus, unsigned long ulPictureType, unsigned long ulPictureStructure);
static unsigned long video_plugin_frame_get_frame_type(unsigned long uiVideoCodec, unsigned long ulPictureType, unsigned long ulPictureStructure);


void video_plugin_frame_set_frame_skip_mode(VPC_FRAME_SKIP_T *ptFrameSkip, VPC_FRAME_SKIP_MODE_E eFrameSkipMode, unsigned int uiVideoCodec, int iDecodingStatus, unsigned long ulPictureType, unsigned long ulPictureStructure)
{
	DPV_FUNC_LOG();

	if (ptFrameSkip != NULL) {
		switch ((int)eFrameSkipMode) {
			case (int)VPC_FRAME_SKIP_MODE_NONE:
			case (int)VPC_FRAME_SKIP_MODE_NEXT_B_SKIP:
			case (int)VPC_FRAME_SKIP_MODE_B_WAIT: {
				ptFrameSkip->eFrameSkipMode = eFrameSkipMode;
				ptFrameSkip->iSkipFrameNum = 0;
				ptFrameSkip->iSkipFrameMode = VPC_VDEC_SKIP_FRAME_DISABLE;
				ptFrameSkip->iFrameSearchEnable = 0;
				break;
			}

			case (int)VPC_FRAME_SKIP_MODE_IDR_SEARCH: {
				ptFrameSkip->eFrameSkipMode = VPC_FRAME_SKIP_MODE_I_SEARCH;
				ptFrameSkip->iSkipFrameNum = 0;
				ptFrameSkip->iSkipFrameMode = VPC_VDEC_SKIP_FRAME_DISABLE;
				ptFrameSkip->iFrameSearchEnable = VPC_AVC_IDR_PICTURE_SEARCH_MODE;
				break;
			}

			case (int)VPC_FRAME_SKIP_MODE_NONIDR_SEARCH: {
				ptFrameSkip->eFrameSkipMode = VPC_FRAME_SKIP_MODE_I_SEARCH;
				ptFrameSkip->iSkipFrameNum = 0;
				ptFrameSkip->iSkipFrameMode = VPC_VDEC_SKIP_FRAME_DISABLE;
				ptFrameSkip->iFrameSearchEnable = VPC_AVC_NONIDR_PICTURE_SEARCH_MODE;
				break;
			}

			case (int)VPC_FRAME_SKIP_MODE_I_SEARCH: {
				ptFrameSkip->eFrameSkipMode = eFrameSkipMode;
				ptFrameSkip->iSkipFrameNum = 0;
				ptFrameSkip->iSkipFrameMode = VPC_VDEC_SKIP_FRAME_DISABLE;
				if (uiVideoCodec == (unsigned int)VPK_STD_H264) {
					ptFrameSkip->iFrameSearchEnable = VPC_AVC_NONIDR_PICTURE_SEARCH_MODE;
				} else {
					ptFrameSkip->iFrameSearchEnable = VPC_AVC_IDR_PICTURE_SEARCH_MODE;
				}
				break;
			}

			case (int)VPC_FRAME_SKIP_MODE_B_SKIP: {
				ptFrameSkip->eFrameSkipMode = eFrameSkipMode;
				ptFrameSkip->iSkipFrameNum = 1;
				ptFrameSkip->iSkipFrameMode = VPC_VDEC_SKIP_FRAME_ONLY_B;
				ptFrameSkip->iFrameSearchEnable = 0;
				break;
			}

			case (int)VPC_FRAME_SKIP_MODE_EXCEPT_I_SKIP: {
				ptFrameSkip->eFrameSkipMode = eFrameSkipMode;
				ptFrameSkip->iSkipFrameNum = 1;
				ptFrameSkip->iSkipFrameMode = VPC_VDEC_SKIP_FRAME_EXCEPT_I;
				ptFrameSkip->iFrameSearchEnable = 0;
				break;
			}

			case (int)VPC_FRAME_SKIP_MODE_NEXT_STEP: {
				video_plugin_frame_set_next_frame_skip_mode(ptFrameSkip, uiVideoCodec, iDecodingStatus, ulPictureType, ulPictureStructure);
				break;
			}

			default: {
				DPV_DONOTHING(0);
				break;
			}
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptFrameSkip is NULL \n", __func__, __LINE__);
	}
}

static void video_plugin_frame_check_frame_type(unsigned long ulFrameType)
{
	switch (ulFrameType) {
		case VPK_PIC_TYPE_I:
			DPV_PRINT("[%s(%d)] VPK_PIC_TYPE_I \n", __func__, __LINE__);
			break;

		case VPK_PIC_TYPE_P:
			DPV_PRINT("[%s(%d)] VPK_PIC_TYPE_P \n", __func__, __LINE__);
			break;

		case VPK_PIC_TYPE_B:
			DPV_PRINT("[%s(%d)] VPK_PIC_TYPE_B \n", __func__, __LINE__);
			break;

		case VPK_PIC_TYPE_IDR:
			DPV_PRINT("[%s(%d)] VPK_PIC_TYPE_IDR \n", __func__, __LINE__);
			break;

		case VPK_PIC_TYPE_B_PB:
			DPV_PRINT("[%s(%d)] VPK_PIC_TYPE_B_PB \n", __func__, __LINE__);
			break;

		default:
			DPV_ERROR_MSG("[%s(%d)] Invalid frame type: %lu \n", __func__, __LINE__, ulFrameType);
			break;
	}
}

static unsigned long video_plugin_frame_get_frame_type(unsigned long uiVideoCodec, unsigned long ulPictureType, unsigned long ulPictureStructure)
{
	unsigned long ulFrameType = 0UL;
	unsigned long ulMaskedPictureType = (ulPictureType & (unsigned long)0xf);

	switch (uiVideoCodec) {
		case VPK_STD_H264: {
			if (ulPictureStructure == 1UL) {
				// FIELD_INTERLACED
				if ((ulPictureType >> 3UL) == (unsigned long)VPK_PIC_TYPE_I) {
					ulFrameType = VPK_PIC_TYPE_I;
				} else if((ulPictureType >> 3UL) == (unsigned long)VPK_PIC_TYPE_P) {
					ulFrameType = (unsigned long)VPK_PIC_TYPE_P;
				} else if((ulPictureType >> 3UL) == 2UL) {
					ulFrameType = (unsigned long)VPK_PIC_TYPE_B;
				} else {
					ulFrameType = (unsigned long)VPK_PIC_TYPE_I;
				}

				if ((ulPictureType & (unsigned long)0x7) == (unsigned long)VPK_PIC_TYPE_I) {
					ulFrameType = (unsigned long)VPK_PIC_TYPE_I;
				} else if ((ulPictureType & (unsigned long)0x7) == (unsigned long)VPK_PIC_TYPE_P) {
					ulFrameType = (unsigned long)VPK_PIC_TYPE_P;
				} else if ((ulPictureType & (unsigned long)0x7) == 2UL) {
					ulFrameType = (unsigned long)VPK_PIC_TYPE_B;
				} else {
					DPV_ERROR_MSG("[%s(%d)] Invalid picture type: %lu \n", __func__, __LINE__, ulPictureType);
				}
			} else {
				if (ulPictureType == (unsigned long)VPK_PIC_TYPE_I) {
					ulFrameType = (unsigned long)VPK_PIC_TYPE_I;
				} else if (ulPictureType == (unsigned long)VPK_PIC_TYPE_P) {
					ulFrameType = (unsigned long)VPK_PIC_TYPE_P;
				} else if (ulPictureType == 2UL) {
					ulFrameType = (unsigned long)VPK_PIC_TYPE_B;
				} else {
					DPV_ERROR_MSG("[%s(%d)] Invalid picture type: %lu \n", __func__, __LINE__, ulPictureType);
				}
			}
			break;
		}

		case VPK_STD_MPEG2:
		default: {
			switch (ulMaskedPictureType) {
				case VPK_PIC_TYPE_I:
					ulFrameType = (unsigned long)VPK_PIC_TYPE_I;
					break;
				case VPK_PIC_TYPE_P:
					ulFrameType = (unsigned long)VPK_PIC_TYPE_P;
					break;
				case VPK_PIC_TYPE_B:
					ulFrameType = (unsigned long)VPK_PIC_TYPE_B;
					break;
				default:
					//DPV_ERROR_MSG("[%s(%d)] Invalid picture type: %lu \n", __func__, __LINE__, ulMaskedPictureType);
					DPV_DONOTHING(0);
					break;

			}

			break;
		}
	}

	//video_plugin_frame_check_frame_type(ulFrameType);

	return ulFrameType;
}

static void video_plugin_frame_set_next_frame_skip_mode(VPC_FRAME_SKIP_T *ptFrameSkip, unsigned int uiVideoCodec, int iDecodingStatus, unsigned long ulPictureType, unsigned long ulPictureStructure)
{
	DPV_FUNC_LOG();

	if (ptFrameSkip != NULL) {
		switch ((int)ptFrameSkip->eFrameSkipMode) {
			case (int)VPC_FRAME_SKIP_MODE_I_SEARCH: {
				if (uiVideoCodec == (unsigned int)VPK_STD_H265) {
					video_plugin_frame_set_frame_skip_mode(ptFrameSkip, VPC_FRAME_SKIP_MODE_B_WAIT, uiVideoCodec, iDecodingStatus, ulPictureType, ulPictureStructure);
				} else {
					if (iDecodingStatus == VPK_VPU_DEC_SUCCESS_FIELD_PICTURE) {
						video_plugin_frame_set_frame_skip_mode(ptFrameSkip, VPC_FRAME_SKIP_MODE_NEXT_B_SKIP, uiVideoCodec, iDecodingStatus, ulPictureType, ulPictureStructure);
					} else {
						video_plugin_frame_set_frame_skip_mode(ptFrameSkip, VPC_FRAME_SKIP_MODE_B_SKIP, uiVideoCodec, iDecodingStatus, ulPictureType, ulPictureStructure);
					}
				}
				break;
			}

			case (int)VPC_FRAME_SKIP_MODE_NEXT_B_SKIP: {
				video_plugin_frame_set_frame_skip_mode(ptFrameSkip, VPC_FRAME_SKIP_MODE_B_SKIP, uiVideoCodec, iDecodingStatus, ulPictureType, ulPictureStructure);
				break;
			}

			case (int)VPC_FRAME_SKIP_MODE_B_SKIP: {
				if (video_plugin_frame_get_frame_type(uiVideoCodec, ulPictureType, ulPictureStructure) != (unsigned long)VPK_PIC_TYPE_B) {
					video_plugin_frame_set_frame_skip_mode(ptFrameSkip, VPC_FRAME_SKIP_MODE_NONE, uiVideoCodec, iDecodingStatus, ulPictureType, ulPictureStructure);
				}
				break;
			}

			case (int)VPC_FRAME_SKIP_MODE_EXCEPT_I_SKIP: {
				video_plugin_frame_set_frame_skip_mode(ptFrameSkip, VPC_FRAME_SKIP_MODE_NONE, uiVideoCodec, iDecodingStatus, ulPictureType, ulPictureStructure);
				break;
			}

			case (int)VPC_FRAME_SKIP_MODE_B_WAIT: {
				if (video_plugin_frame_get_frame_type(uiVideoCodec, ulPictureType, ulPictureStructure) == (unsigned long)VPK_PIC_TYPE_B) {
					video_plugin_frame_set_frame_skip_mode(ptFrameSkip, VPC_FRAME_SKIP_MODE_NONE, uiVideoCodec, iDecodingStatus, ulPictureType, ulPictureStructure);
				}
				break;
			}

			default: {
				video_plugin_frame_set_frame_skip_mode(ptFrameSkip, VPC_FRAME_SKIP_MODE_NONE, uiVideoCodec, iDecodingStatus, ulPictureType, ulPictureStructure);
				break;
			}
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptFrameSkip is NULL \n", __func__, __LINE__);
	}
}