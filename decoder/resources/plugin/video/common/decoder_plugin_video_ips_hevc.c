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


#include <decoder_plugin_video_ips.h>


#if defined(TCC_HEVC_INCLUDE)
static void vpu_hevc_cast_codec_addr_to_puc(unsigned char *pucOut[VPK_COMP_V + 1], VPK_CODEC_ADDR_t codecIn[VPK_COMP_V + 1]);


void vpu_hevc_set_init_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_HEVC_INIT_t *ptInit = &ptDriverInfo->tHevcInit;

		int tmpUserData = (int)ptDriverInfo->bEnableUserData;
		int tmpInterlevemode = (int)ptDriverInfo->bCbCrInterleaveMode;

		ptInit->gsHevcDecInit.m_iBitstreamFormat 		= ptDriverInfo->iBitstreamFormat;
		ptInit->gsHevcDecInit.m_iBitstreamBufSize 		= ptDriverInfo->iBitstreamBufSize;

		if (ptDriverInfo->bEnable10Bits == false) {
			ptInit->gsHevcDecInit.m_uiDecOptFlags 		= ptInit->gsHevcDecInit.m_uiDecOptFlags | (unsigned int)VPK_WAVE4_10BITS_DISABLE;
		}

		if (ptDriverInfo->bEnableMapConverter == false) {
			ptInit->gsHevcDecInit.m_uiDecOptFlags 			= ptInit->gsHevcDecInit.m_uiDecOptFlags | (unsigned int)VPK_WAVE4_WTL_ENABLE;
		}

		if ((tmpUserData >= 0) && (tmpInterlevemode >= 0)) {
			ptInit->gsHevcDecInit.m_bEnableUserData			= (unsigned int)tmpUserData;
			ptInit->gsHevcDecInit.m_bCbCrInterleaveMode 	= (unsigned int)tmpInterlevemode;
		}
		ptInit->gsHevcDecInit.m_iFilePlayEnable 			= ptDriverInfo->iFilePlayEnable;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_hevc_set_seq_header_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, int iBitstreamBufSize, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1])
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_HEVC_DECODE_t *ptDecode = &ptDriverInfo->tHevcDecode;

		ptDecode->gsHevcDecInput.m_BitstreamDataAddr[VPK_PA] 	= (uint64_t)pucBitstreamBuffer[VPK_PA];
		ptDecode->gsHevcDecInput.m_BitstreamDataAddr[VPK_VA] 	= (uint64_t)pucBitstreamBuffer[VPK_K_VA];
		ptDecode->gsHevcDecInput.m_iBitstreamDataSize 			= iBitstreamBufSize;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

int vpu_hevc_is_pic_resolution_ok(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_RESOLUTION_INFO_T tMaxResInfo, VPC_RESOLUTION_INFO_T tMinResInfo)
{
	int iRetVal = -1;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if ((tMaxResInfo.iWidth < INT16_MAX) && (tMaxResInfo.iHeight < INT16_MAX)) {
			const VPK_HEVC_SEQ_HEADER_t *ptSeqHeader = &ptDriverInfo->tHevcSeqHeader;

			unsigned int tmpWidth = (((unsigned int)tMaxResInfo.iWidth + 15U) & (unsigned int)0xFFF0);

			if ((ptSeqHeader->gsHevcDecInitialInfo.m_iPicWidth > (int)tmpWidth) ||
				((ptSeqHeader->gsHevcDecInitialInfo.m_iPicWidth * ptSeqHeader->gsHevcDecInitialInfo.m_iPicHeight) > (tMaxResInfo.iWidth * tMaxResInfo.iHeight)) ||
				(ptSeqHeader->gsHevcDecInitialInfo.m_iPicWidth < tMinResInfo.iWidth) ||
				(ptSeqHeader->gsHevcDecInitialInfo.m_iPicHeight < tMinResInfo.iHeight)) {
				DPV_ERROR_MSG("[%s(%d)] [ERROR] (%d X %d) \n", __func__, __LINE__, ptSeqHeader->gsHevcDecInitialInfo.m_iPicWidth, ptSeqHeader->gsHevcDecInitialInfo.m_iPicHeight);
				iRetVal = -1;
			} else {
				iRetVal = 0;
			}
		} else {
			iRetVal = -1;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return iRetVal;
}

void vpu_hevc_get_decode_result_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	VPK_HEVC_DECODE_t *ptDecoded = &ptDriverInfo->tHevcDecode;
	VPC_DECODE_RESULT_INFO_T *ptDecResult 	= &ptDriverInfo->tDecodeResultInfo;

	ptDecResult->iDecodingResult 				= ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_iDecodingStatus;
	ptDecResult->iDecodingOutResult				= ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_iOutputStatus;
	ptDecResult->iDecodedIdx 					= ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_iDecodedIdx;
	ptDecResult->iDispOutIdx 					= ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_iDispOutIdx;
	ptDecResult->iNumOfErrMBs 					= ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_iNumOfErrMBs;

	ptDecResult->iPictureType 					= ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_iPicType;

	if ((ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropLeft   < (unsigned int)UINT16_MAX) &&
		(ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropRight  < (unsigned int)UINT16_MAX) &&
		(ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropTop    < (unsigned int)UINT16_MAX) &&
		(ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropBottom < (unsigned int)UINT16_MAX)) {
		ptDecResult->tCropRect.iLeft 				= (int)ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropLeft;
		ptDecResult->tCropRect.iRight 				= (int)ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropRight;
		ptDecResult->tCropRect.iTop 				= (int)ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropTop;
		ptDecResult->tCropRect.iBottom 				= (int)ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropBottom;
	}

	ptDecResult->tVideoResolution.iWidth 		= ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_iDisplayWidth;
	ptDecResult->tVideoResolution.iHeight 		= ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_iDisplayHeight;

	vpu_hevc_cast_codec_addr_to_puc(ptDecResult->pucDispOut[VPK_PA], ptDecoded->gsHevcDecOutput.m_pDispOut[VPK_PA]);
	vpu_hevc_cast_codec_addr_to_puc(ptDecResult->pucDispOut[VPK_VA], ptDecoded->gsHevcDecOutput.m_pDispOut[VPK_VA]);

	vpu_hevc_cast_codec_addr_to_puc(ptDecResult->pucCurrOut[VPK_PA], ptDecoded->gsHevcDecOutput.m_pCurrOut[VPK_PA]);
	vpu_hevc_cast_codec_addr_to_puc(ptDecResult->pucCurrOut[VPK_VA], ptDecoded->gsHevcDecOutput.m_pCurrOut[VPK_VA]);

	vpu_hevc_cast_codec_addr_to_puc(ptDecResult->pucPrevOut[VPK_PA], ptDecoded->gsHevcDecOutput.m_pPrevOut[VPK_PA]);
	vpu_hevc_cast_codec_addr_to_puc(ptDecResult->pucPrevOut[VPK_VA], ptDecoded->gsHevcDecOutput.m_pPrevOut[VPK_VA]);

	ptDecResult->pucDispOut[VPK_VA][VPK_COMP_Y] = vpu_get_framebuf_virtaddr(ptDriverInfo->pucFrameBuffer, ptDecResult->pucDispOut[VPK_VA][VPK_COMP_Y], VPK_K_VA);
	ptDecResult->pucDispOut[VPK_VA][VPK_COMP_U] = vpu_get_framebuf_virtaddr(ptDriverInfo->pucFrameBuffer, ptDecResult->pucDispOut[VPK_VA][VPK_COMP_U], VPK_K_VA);
	ptDecResult->pucDispOut[VPK_VA][VPK_COMP_V] = vpu_get_framebuf_virtaddr(ptDriverInfo->pucFrameBuffer, ptDecResult->pucDispOut[VPK_VA][VPK_COMP_V], VPK_K_VA);

	ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_Y] = vpu_get_framebuf_virtaddr(ptDriverInfo->pucFrameBuffer, ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_Y], VPK_K_VA);
	ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_U] = vpu_get_framebuf_virtaddr(ptDriverInfo->pucFrameBuffer, ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_U], VPK_K_VA);
	ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_V] = vpu_get_framebuf_virtaddr(ptDriverInfo->pucFrameBuffer, ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_V], VPK_K_VA);

	if (ptDecResult->iDecodingResult == VPK_VPU_DEC_SUCCESS) {
		ptDecResult->eFrameType = (VPC_FRAME_TYPE_E)VPC_FRAME_TYPE_FRAME;
	} else if (ptDecResult->iDecodingResult == VPK_VPU_DEC_SUCCESS_FIELD_PICTURE) {
		ptDecResult->eFrameType = (VPC_FRAME_TYPE_E)VPC_FRAME_TYPE_FIELD;
	} else {
		ptDecResult->eFrameType = (VPC_FRAME_TYPE_E)VPC_FRAME_TYPE_UNKNOWN;
	}

	#if (DPV_PRINT_DETAIL_INFO == 1)
	vpu_check_picture_type(ptDecResult->iPictureType);

	DPV_DETAIL_MSG("[%s(%d)] ============================================================================== \n", __func__, __LINE__);
	DPV_DETAIL_MSG("[%s(%d)] [HEVC-%d] DECODING RESULTS: %d \n", __func__, __LINE__, ptDriverInfo->iInstIdx, ptDecResult->eFrameType);
	DPV_DETAIL_MSG("[%s(%d)] ------------------------------------------------------------------------ \n", __func__, __LINE__);
	DPV_DETAIL_MSG("[%s(%d)]   - DEC: %d(%d)  \n", __func__, __LINE__, ptDecResult->iDecodingResult, ptDecResult->iDecodedIdx);
	DPV_DETAIL_MSG("[%s(%d)]   - OUT: %d(%d)  \n", __func__, __LINE__, ptDecResult->iDecodingOutResult, ptDecResult->iDispOutIdx);
	DPV_DETAIL_MSG("[%s(%d)]   - PIC: %d, %d  \n", __func__, __LINE__, ptDecResult->iPictureStructure, ptDecoded->gsHevcDecOutput.m_DecOutInfo.m_iPicType);
	DPV_DETAIL_MSG("[%s(%d)]     - I: %d(%d, %d)  \n", __func__, __LINE__, ptDecResult->iInterlace, ptDecResult->iInterlacedFrame, ptDecResult->iTopFieldFirst);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %d, %d  \n", __func__, __LINE__, ptDecResult->iM2vProgressiveFrame, ptDecResult->iM2vAspectRatio);
	DPV_DETAIL_MSG("[%s(%d)]   - RES: %d X %d  \n", __func__, __LINE__, ptDecResult->tVideoResolution.iWidth, ptDecResult->tVideoResolution.iHeight);
	DPV_DETAIL_MSG("[%s(%d)]     - CROP: %d, %d, %d, %d  \n", __func__, __LINE__, ptDecResult->tCropRect.iLeft, ptDecResult->tCropRect.iTop, ptDecResult->tCropRect.iRight, ptDecResult->tCropRect.iBottom);
	DPV_DETAIL_MSG("[%s(%d)]   - DIS:  \n", __func__, __LINE__);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucDispOut[VPK_PA][VPK_COMP_Y], ptDecResult->pucDispOut[VPK_PA][VPK_COMP_U], ptDecResult->pucDispOut[VPK_PA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucDispOut[VPK_VA][VPK_COMP_Y], ptDecResult->pucDispOut[VPK_VA][VPK_COMP_U], ptDecResult->pucDispOut[VPK_VA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsHevcDecOutput.m_pDispOut[VPK_PA][VPK_COMP_Y], ptDecoded->gsHevcDecOutput.m_pDispOut[VPK_PA][VPK_COMP_U], ptDecoded->gsHevcDecOutput.m_pDispOut[VPK_PA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsHevcDecOutput.m_pDispOut[VPK_VA][VPK_COMP_Y], ptDecoded->gsHevcDecOutput.m_pDispOut[VPK_VA][VPK_COMP_U], ptDecoded->gsHevcDecOutput.m_pDispOut[VPK_VA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]   - CUR:  \n", __func__, __LINE__);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_Y], ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_U], ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_Y], ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_U], ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsHevcDecOutput.m_pCurrOut[VPK_PA][VPK_COMP_Y], ptDecoded->gsHevcDecOutput.m_pCurrOut[VPK_PA][VPK_COMP_U], ptDecoded->gsHevcDecOutput.m_pCurrOut[VPK_PA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsHevcDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_Y], ptDecoded->gsHevcDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_U], ptDecoded->gsHevcDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)] ============================================================================== \n", __func__, __LINE__);
	#endif
}

void vpu_hevc_set_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1], int iBitstreamBufSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_HEVC_INIT_t *ptInit = &ptDriverInfo->tHevcInit;

		ptInit->gsHevcDecInit.m_BitstreamBufAddr[VPK_PA] 	= (uint64_t)pucBitstreamBuffer[VPK_PA];
		ptInit->gsHevcDecInit.m_BitstreamBufAddr[VPK_VA] 	= (uint64_t)pucBitstreamBuffer[VPK_K_VA];
		ptInit->gsHevcDecInit.m_iBitstreamBufSize 			= iBitstreamBufSize;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_hevc_set_decoding_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VPK_HEVC_INIT_t *ptInit = &ptDriverInfo->tHevcInit;
		VPK_HEVC_DECODE_t *ptDecode = &ptDriverInfo->tHevcDecode;

		ptDecode->gsHevcDecInput.m_iBitstreamDataSize			= ptDriverInfo->iInputBufSize;
		ptDecode->gsHevcDecInput.m_BitstreamDataAddr[VPK_PA] 	= (uint64_t)ptDriverInfo->pucBitstreamBuffer[VPK_PA];
		ptDecode->gsHevcDecInput.m_BitstreamDataAddr[VPK_VA] 	= (uint64_t)ptDriverInfo->pucBitstreamBuffer[VPK_K_VA];

		if (ptDriverInfo->iInputBufSize > (ptDriverInfo->iBitstreamBufSize / 2)) {
			ptDecode->gsHevcDecInput.m_iBitstreamDataSize = ptDriverInfo->iBitstreamBufSize / 2;
		}

		if (ptInit->gsHevcDecInit.m_iFilePlayEnable != 0 ) {
			if ((ptDriverInfo->pucInputBuffer[VPK_PA] != ptDriverInfo->pucBitstreamBuffer[VPK_PA]) || (ptDriverInfo->pucInputBuffer[VPK_VA] != ptDriverInfo->pucBitstreamBuffer[VPK_VA])) {
				if ((ptDriverInfo->pucBitstreamBuffer[VPK_VA] != NULL) && (ptDriverInfo->pucInputBuffer[VPK_VA] != NULL) && (ptDriverInfo->iInputBufSize > 0) && (ptDriverInfo->iBitstreamBufSize > 0)) {
					if ((ptDriverInfo->iCurrStreamIdx == 0) || (ptDriverInfo->iCurrStreamIdx == -1)) {
						VDEC_MEMCPY(ptDriverInfo->pucBitstreamBuffer[VPK_VA], ptDriverInfo->pucInputBuffer[VPK_VA], ptDriverInfo->iInputBufSize);
					} else {
						uint64_t tmpBufferSize = (uint64_t)ptDriverInfo->iBitstreamBufSize / 2UL;

						ptDecode->gsHevcDecInput.m_BitstreamDataAddr[VPK_PA] 	= (uint64_t)ptDriverInfo->pucBitstreamBuffer[VPK_PA] + tmpBufferSize;
						ptDecode->gsHevcDecInput.m_BitstreamDataAddr[VPK_VA] 	= (uint64_t)ptDriverInfo->pucBitstreamBuffer[VPK_K_VA] + tmpBufferSize;

						VDEC_MEMCPY(ptDriverInfo->pucBitstreamBuffer[VPK_VA] + (ptDriverInfo->iBitstreamBufSize / 2), ptDriverInfo->pucInputBuffer[VPK_VA], ptDriverInfo->iInputBufSize);
					}
				} else {
					DPV_ERROR_MSG("[%s(%d)] [ERROR] pucBitstreamBuffer is NULL \n", __func__, __LINE__);
				}
			} else {
				DPV_PRINT("[%s(%d)] Same Value \n", __func__, __LINE__);
			}
		} else {
			ptDecode->gsHevcDecInput.m_iBitstreamDataSize = 1;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_hevc_set_bitwork_addr(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_HEVC_INIT_t *ptInit = &ptDriverInfo->tHevcInit;
		const VPK_MEM_ALLOC_INFO_t *ptAllocMem = &ptDriverInfo->tAllocMem;
		VPK_CODEC_ADDR_t *ptTmpAddr = &ptInit->gsHevcDecInit.m_BitWorkAddr[VPK_VA];

		ptInit->gsHevcDecInit.m_BitWorkAddr[VPK_PA] = ptAllocMem->phy_addr;
		*ptTmpAddr = vpu_cast_void_uintptr(ptAllocMem->kernel_remap_addr);
		DPV_DETAIL_MSG("[%s(%d)] remap_addr: 0x%x, kernel_remap_addr: 0x%x \n", __func__, __LINE__, ptTmpAddr, ptAllocMem->kernel_remap_addr);
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_hevc_set_stream_size(VPC_DRIVER_INFO_T *ptDriverInfo, int iStreamLen)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_HEVC_SEQ_HEADER_t *ptSeqHeader = &ptDriverInfo->tHevcSeqHeader;

		if (iStreamLen >= 0) {
			ptSeqHeader->stream_size = (unsigned int)iStreamLen;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_hevc_get_header_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_HEVC_SEQ_HEADER_t *ptSeqHeader = &ptDriverInfo->tHevcSeqHeader;

		if ((ptSeqHeader->gsHevcDecInitialInfo.m_iPicWidth > 0) && (ptSeqHeader->gsHevcDecInitialInfo.m_iPicHeight > 0)) {
			unsigned int iCropWidth = ptSeqHeader->gsHevcDecInitialInfo.m_PicCrop.m_iCropLeft - ptSeqHeader->gsHevcDecInitialInfo.m_PicCrop.m_iCropRight;
			unsigned int iCropHeight = ptSeqHeader->gsHevcDecInitialInfo.m_PicCrop.m_iCropBottom - ptSeqHeader->gsHevcDecInitialInfo.m_PicCrop.m_iCropTop;

			unsigned int tmpWidth = (unsigned int)ptSeqHeader->gsHevcDecInitialInfo.m_iPicWidth - iCropWidth;
			unsigned int tmpHeight = (unsigned int)ptSeqHeader->gsHevcDecInitialInfo.m_iPicHeight - iCropHeight;

			if ((tmpWidth < (UINT32_MAX / 2U)) && (tmpHeight < (UINT32_MAX / 2U))) {
				ptDriverInfo->iPicWidth 	= (int)tmpWidth;
				ptDriverInfo->iPicHeight 	= (int)tmpHeight;

				tmpWidth = ((tmpWidth + 31U) >> 5U) << 5U;

				ptSeqHeader->gsHevcDecInitialInfo.m_iPicWidth = (int)tmpWidth;
				ptSeqHeader->gsHevcDecInitialInfo.m_iPicHeight = (int)tmpHeight;
			}
		}

		ptDriverInfo->iAspectRatio 	= ptSeqHeader->gsHevcDecInitialInfo.m_iAspectRateInfo;

		ptDriverInfo->iMinFrameBufferCount 	= ptSeqHeader->gsHevcDecInitialInfo.m_iMinFrameBufferCount;
		ptDriverInfo->iMinFrameBufferSize 	= ptSeqHeader->gsHevcDecInitialInfo.m_iMinFrameBufferSize;

		#if (DPV_PRINT_DETAIL_INFO == 1)
		{
			uint32_t fRateInfoRes = ptSeqHeader->gsHevcDecInitialInfo.m_uiFrameRateRes;
			uint32_t fRateInfoDiv = ptSeqHeader->gsHevcDecInitialInfo.m_uiFrameRateDiv;

			int32_t profile = ptSeqHeader->gsHevcDecInitialInfo.m_iProfile;
			int32_t level 	= ptSeqHeader->gsHevcDecInitialInfo.m_iLevel;
			int32_t tier	= ptSeqHeader->gsHevcDecInitialInfo.m_iTier;

			DPV_DETAIL_MSG("[%s(%d)] ==================================================== \n", __func__, __LINE__);
			DPV_DETAIL_MSG("[%s(%d)] [HEVC-%d] INITIAL INFO \n", __func__, __LINE__, ptDriverInfo->iInstIdx);
			DPV_DETAIL_MSG("[%s(%d)] --------------------------------------------------- \n", __func__, __LINE__);
			DPV_DETAIL_MSG("[%s(%d)]  - profile: %d level: %d tier: %d \n", __func__, __LINE__, profile, level, tier);
			DPV_DETAIL_MSG("[%s(%d)]  - Aspect Ratio: %1d(%u, %u), FrameBuffer Format: %s Bit \n", __func__, __LINE__, ptSeqHeader->gsHevcDecInitialInfo.m_iAspectRateInfo, fRateInfoRes, fRateInfoDiv, (ptSeqHeader->gsHevcDecInitialInfo.m_iFrameBufferFormat == 0) ? ("8") : ("10"));
			DPV_DETAIL_MSG("[%s(%d)]  - MinFrameBufferCount: %u, frameBufDelay: %u \n", __func__, __LINE__, ptSeqHeader->gsHevcDecInitialInfo.m_iMinFrameBufferCount, ptSeqHeader->gsHevcDecInitialInfo.m_iFrameBufDelay);
			DPV_DETAIL_MSG("[%s(%d)]  - Crop Info: %d(%d - %d), %d(%d - %d) \n", __func__, __LINE__,
												ptSeqHeader->gsHevcDecInitialInfo.m_iPicWidth, ptSeqHeader->gsHevcDecInitialInfo.m_PicCrop.m_iCropLeft, ptSeqHeader->gsHevcDecInitialInfo.m_PicCrop.m_iCropRight,
												ptSeqHeader->gsHevcDecInitialInfo.m_iPicHeight, ptSeqHeader->gsHevcDecInitialInfo.m_PicCrop.m_iCropTop, ptSeqHeader->gsHevcDecInitialInfo.m_PicCrop.m_iCropBottom);
			DPV_DETAIL_MSG("[%s(%d)] ==================================================== \n", __func__, __LINE__);
		}
		#endif
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_hevc_get_slice_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSliceBuffer, int iSliceBufSize, int iExtraFrameCount)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_HEVC_SET_BUFFER_t *ptSetBuffer = &ptDriverInfo->tHevcSetBuffer;

		if ((INT32_MAX - ptDriverInfo->iMinFrameBufferCount) >= iExtraFrameCount) {
			ptSetBuffer->gsHevcDecBuffer.m_iFrameBufferCount = ptDriverInfo->iMinFrameBufferCount + iExtraFrameCount;
		}

		ptDriverInfo->pucSliceBuffer = pucSliceBuffer;
		ptDriverInfo->iSliceBufSize = iSliceBufSize;
		ptDriverInfo->iFrameBufferCount = ptSetBuffer->gsHevcDecBuffer.m_iFrameBufferCount;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_hevc_set_frame_buffer_start_addr_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucFrameBuffer[VPK_K_VA + 1], int iFrameBufferSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_HEVC_SET_BUFFER_t *ptSetBuffer = &ptDriverInfo->tHevcSetBuffer;

		ptSetBuffer->gsHevcDecBuffer.m_iFrameBufferCount = iFrameBufferSize;
		ptSetBuffer->gsHevcDecBuffer.m_FrameBufferStartAddr[VPK_PA] = (uint64_t)pucFrameBuffer[VPK_PA];
		ptSetBuffer->gsHevcDecBuffer.m_FrameBufferStartAddr[VPK_VA] = (uint64_t)pucFrameBuffer[VPK_K_VA];
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static void vpu_hevc_cast_codec_addr_to_puc(unsigned char *pucOut[VPK_COMP_V + 1], VPK_CODEC_ADDR_t codecIn[VPK_COMP_V + 1])
{
	DPV_CAST(pucOut[VPK_COMP_Y], codecIn[VPK_COMP_Y]);
	DPV_CAST(pucOut[VPK_COMP_U], codecIn[VPK_COMP_U]);
	DPV_CAST(pucOut[VPK_COMP_V], codecIn[VPK_COMP_V]);
}
#endif