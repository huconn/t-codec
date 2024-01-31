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


#if defined(TCC_VPU_4K_D2_INCLUDE)
void vpu_4kd2_set_init_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VPU_4K_D2_INIT_t *ptInit = &ptDriverInfo->tVpu4KD2Init;

		int tmpUserData = (int)ptDriverInfo->bEnableUserData;
		int tmpInterlevemode = (int)ptDriverInfo->bCbCrInterleaveMode;

		ptDriverInfo->iCurrStreamIdx = -1;

		ptInit->gsV4kd2DecInit.m_RegBaseVirtualAddr 	= ptDriverInfo->uiRegBaseVirtualAddr;
		ptInit->gsV4kd2DecInit.m_iBitstreamFormat 		= ptDriverInfo->iBitstreamFormat;

		if (ptDriverInfo->bEnable10Bits == false) {
			ptInit->gsV4kd2DecInit.m_uiDecOptFlags 		= ptInit->gsV4kd2DecInit.m_uiDecOptFlags | (unsigned int)VPK_WAVE5_10BITS_DISABLE;
		}

		if (ptDriverInfo->bEnableMapConverter == false) {
			ptInit->gsV4kd2DecInit.m_uiDecOptFlags 		= ptInit->gsV4kd2DecInit.m_uiDecOptFlags | (unsigned int)VPK_WAVE5_WTL_ENABLE;
		}

		if ((tmpUserData >= 0) && (tmpInterlevemode >= 0)) {
			ptInit->gsV4kd2DecInit.m_bEnableUserData		= (unsigned int)tmpUserData;
			ptInit->gsV4kd2DecInit.m_bCbCrInterleaveMode 	= (unsigned int)tmpInterlevemode;
		}
		ptInit->gsV4kd2DecInit.m_iFilePlayEnable 			= ptDriverInfo->iFilePlayEnable;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4kd2_set_seq_header_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, int iBitstreamBufSize, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1])
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPU_4K_D2_DECODE_t *ptDecode = &ptDriverInfo->tVpu4KD2Decode;

		ptDecode->gsV4kd2DecInput.m_BitstreamDataAddr[VPK_PA] 	= (uint64_t)pucBitstreamBuffer[VPK_PA];
		ptDecode->gsV4kd2DecInput.m_BitstreamDataAddr[VPK_VA] 	= (uint64_t)pucBitstreamBuffer[VPK_K_VA];
		ptDecode->gsV4kd2DecInput.m_iBitstreamDataSize 			= iBitstreamBufSize;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

int vpu_4kd2_is_pic_resolution_ok(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_RESOLUTION_INFO_T tMaxResInfo, VPC_RESOLUTION_INFO_T tMinResInfo)
{
	int iRetVal = -1;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if ((tMaxResInfo.iWidth < INT16_MAX) && (tMaxResInfo.iHeight < INT16_MAX)) {
			const VPU_4K_D2_SEQ_HEADER_t *ptSeqHeader = &ptDriverInfo->tVpu4KD2SeqHeader;

			unsigned int tmpWidth = (((unsigned int)tMaxResInfo.iWidth + 15U) & (unsigned int)0xFFF0);

			DPV_DETAIL_MSG("[%s(%d)] MAX: (%d X %d) \n", __func__, __LINE__, tmpWidth, tMaxResInfo.iHeight);
			DPV_DETAIL_MSG("[%s(%d)] MIN: (%d X %d) \n", __func__, __LINE__, tMinResInfo.iWidth, tMinResInfo.iHeight);
			DPV_DETAIL_MSG("[%s(%d)] SEQ: (%d X %d) \n", __func__, __LINE__, ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicWidth, ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicHeight);

			if ((ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicWidth > (int)tmpWidth) ||
				((ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicWidth * ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicHeight) > (tMaxResInfo.iWidth * tMaxResInfo.iHeight)) ||
				(ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicWidth < tMinResInfo.iWidth) ||
				(ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicHeight < tMinResInfo.iHeight)) {
				DPV_ERROR_MSG("[%s(%d)] [ERROR] (%d X %d) \n", __func__, __LINE__, ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicWidth, ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicHeight);
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

void vpu_4kd2_get_decode_result_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	const VPK_VPU_4K_D2_DECODE_t *ptDecoded = &ptDriverInfo->tVpu4KD2Decode;
	VPC_DECODE_RESULT_INFO_T *ptDecResult 	= &ptDriverInfo->tDecodeResultInfo;

	ptDecResult->iDecodingResult 				= ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_iDecodingStatus;
	ptDecResult->iDecodingOutResult				= ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_iOutputStatus;
	ptDecResult->iDecodedIdx 					= ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_iDecodedIdx;
	ptDecResult->iDispOutIdx 					= ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_iDispOutIdx;
	ptDecResult->iNumOfErrMBs 					= ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_iNumOfErrMBs;

	ptDecResult->iPictureType 					= ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_iPicType;
	ptDecResult->iInterlace 					= ptDecoded->gsV4kd2DecInitialInfo.m_iInterlace;

	if ((ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropLeft   < (unsigned int)UINT16_MAX) &&
		(ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropRight  < (unsigned int)UINT16_MAX) &&
		(ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropTop    < (unsigned int)UINT16_MAX) &&
		(ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropBottom < (unsigned int)UINT16_MAX)) {
		ptDecResult->tCropRect.iLeft 				= (int)ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropLeft;
		ptDecResult->tCropRect.iRight 				= (int)ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropRight;
		ptDecResult->tCropRect.iTop 				= (int)ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropTop;
		ptDecResult->tCropRect.iBottom 				= (int)ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_DisplayCropInfo.m_iCropBottom;
	}

	ptDecResult->tVideoResolution.iWidth 		= ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_iDecodedWidth;
	ptDecResult->tVideoResolution.iHeight 		= ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_iDecodedHeight;

	ptDecResult->pucDispOut[VPK_PA][VPK_COMP_Y] = ptDecoded->gsV4kd2DecOutput.m_pDispOut[VPK_PA][VPK_COMP_Y];
	ptDecResult->pucDispOut[VPK_PA][VPK_COMP_U] = ptDecoded->gsV4kd2DecOutput.m_pDispOut[VPK_PA][VPK_COMP_U];
	ptDecResult->pucDispOut[VPK_PA][VPK_COMP_V] = ptDecoded->gsV4kd2DecOutput.m_pDispOut[VPK_PA][VPK_COMP_V];

	ptDecResult->pucDispOut[VPK_VA][VPK_COMP_Y] = ptDecoded->gsV4kd2DecOutput.m_pDispOut[VPK_VA][VPK_COMP_Y];
	ptDecResult->pucDispOut[VPK_VA][VPK_COMP_U] = ptDecoded->gsV4kd2DecOutput.m_pDispOut[VPK_VA][VPK_COMP_U];
	ptDecResult->pucDispOut[VPK_VA][VPK_COMP_V] = ptDecoded->gsV4kd2DecOutput.m_pDispOut[VPK_VA][VPK_COMP_V];

	ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_Y] = ptDecoded->gsV4kd2DecOutput.m_pCurrOut[VPK_VA][VPK_COMP_Y];
	ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_U] = ptDecoded->gsV4kd2DecOutput.m_pCurrOut[VPK_VA][VPK_COMP_U];
	ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_V] = ptDecoded->gsV4kd2DecOutput.m_pCurrOut[VPK_VA][VPK_COMP_V];

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
	DPV_DETAIL_MSG("[%s(%d)] DECODING RESULTS: %d \n", __func__, __LINE__, ptDecResult->eFrameType);
	DPV_DETAIL_MSG("[%s(%d)] ------------------------------------------------------------------------ \n", __func__, __LINE__);
	DPV_DETAIL_MSG("[%s(%d)]   - DEC: %d(%d)  \n", __func__, __LINE__, ptDecResult->iDecodingResult, ptDecResult->iDecodedIdx);
	DPV_DETAIL_MSG("[%s(%d)]   - OUT: %d(%d)  \n", __func__, __LINE__, ptDecResult->iDecodingOutResult, ptDecResult->iDispOutIdx);
	DPV_DETAIL_MSG("[%s(%d)]   - PIC: %d, %d  \n", __func__, __LINE__, ptDecResult->iPictureStructure, ptDecoded->gsV4kd2DecOutput.m_DecOutInfo.m_iPicType);
	DPV_DETAIL_MSG("[%s(%d)]     - I: %d(%d, %d)  \n", __func__, __LINE__, ptDecResult->iInterlace, ptDecResult->iInterlacedFrame, ptDecResult->iTopFieldFirst);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %d, %d  \n", __func__, __LINE__, ptDecResult->iM2vProgressiveFrame, ptDecResult->iM2vAspectRatio);
	DPV_DETAIL_MSG("[%s(%d)]   - RES: %d X %d  \n", __func__, __LINE__, ptDecResult->tVideoResolution.iWidth, ptDecResult->tVideoResolution.iHeight);
	DPV_DETAIL_MSG("[%s(%d)]     - CROP: %d, %d, %d, %d  \n", __func__, __LINE__, ptDecResult->tCropRect.iLeft, ptDecResult->tCropRect.iTop, ptDecResult->tCropRect.iRight, ptDecResult->tCropRect.iBottom);
	DPV_DETAIL_MSG("[%s(%d)]   - DIS:  \n", __func__, __LINE__);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucDispOut[VPK_PA][VPK_COMP_Y], ptDecResult->pucDispOut[VPK_PA][VPK_COMP_U], ptDecResult->pucDispOut[VPK_PA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucDispOut[VPK_VA][VPK_COMP_Y], ptDecResult->pucDispOut[VPK_VA][VPK_COMP_U], ptDecResult->pucDispOut[VPK_VA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsV4kd2DecOutput.m_pDispOut[VPK_PA][VPK_COMP_Y], ptDecoded->gsV4kd2DecOutput.m_pDispOut[VPK_PA][VPK_COMP_U], ptDecoded->gsV4kd2DecOutput.m_pDispOut[VPK_PA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsV4kd2DecOutput.m_pDispOut[VPK_VA][VPK_COMP_Y], ptDecoded->gsV4kd2DecOutput.m_pDispOut[VPK_VA][VPK_COMP_U], ptDecoded->gsV4kd2DecOutput.m_pDispOut[VPK_VA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]   - CUR:  \n", __func__, __LINE__);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_Y], ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_U], ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_Y], ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_U], ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsV4kd2DecOutput.m_pCurrOut[VPK_PA][VPK_COMP_Y], ptDecoded->gsV4kd2DecOutput.m_pCurrOut[VPK_PA][VPK_COMP_U], ptDecoded->gsV4kd2DecOutput.m_pCurrOut[VPK_PA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsV4kd2DecOutput.m_pCurrOut[VPK_VA][VPK_COMP_Y], ptDecoded->gsV4kd2DecOutput.m_pCurrOut[VPK_VA][VPK_COMP_U], ptDecoded->gsV4kd2DecOutput.m_pCurrOut[VPK_VA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)] ============================================================================== \n", __func__, __LINE__);
	#endif
}

void vpu_4kd2_set_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1], int iBitstreamBufSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VPU_4K_D2_INIT_t *ptInit = &ptDriverInfo->tVpu4KD2Init;

		ptInit->gsV4kd2DecInit.m_BitstreamBufAddr[VPK_PA] 	= (uint64_t)pucBitstreamBuffer[VPK_PA];
		ptInit->gsV4kd2DecInit.m_BitstreamBufAddr[VPK_VA] 	= (uint64_t)pucBitstreamBuffer[VPK_K_VA];
		ptInit->gsV4kd2DecInit.m_iBitstreamBufSize 			= iBitstreamBufSize;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4kd2_set_decoding_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VPK_VPU_4K_D2_INIT_t *ptInit = &ptDriverInfo->tVpu4KD2Init;
		VPU_4K_D2_DECODE_t *ptDecode = &ptDriverInfo->tVpu4KD2Decode;

		ptDecode->gsV4kd2DecInput.m_iBitstreamDataSize		= ptDriverInfo->iInputBufSize;
		ptDecode->gsV4kd2DecInput.m_BitstreamDataAddr[VPK_PA] = (uint64_t)ptDriverInfo->pucBitstreamBuffer[VPK_PA];
		ptDecode->gsV4kd2DecInput.m_BitstreamDataAddr[VPK_VA] = (uint64_t)ptDriverInfo->pucBitstreamBuffer[VPK_K_VA];

		if (ptInit->gsV4kd2DecInit.m_iFilePlayEnable != 0 ) {
			if ((ptDriverInfo->pucInputBuffer[VPK_PA] != ptDriverInfo->pucBitstreamBuffer[VPK_PA]) || (ptDriverInfo->pucInputBuffer[VPK_VA] != ptDriverInfo->pucBitstreamBuffer[VPK_VA])) {
				if ((ptDriverInfo->pucBitstreamBuffer[VPK_VA] != NULL) && (ptDriverInfo->pucInputBuffer[VPK_VA] != NULL) && (ptDriverInfo->iInputBufSize > 0) && (ptDriverInfo->iBitstreamBufSize > 0)) {
					if ((ptDriverInfo->iCurrStreamIdx == 0) || (ptDriverInfo->iCurrStreamIdx == -1)) {
						VDEC_MEMCPY(ptDriverInfo->pucBitstreamBuffer[VPK_VA], ptDriverInfo->pucInputBuffer[VPK_VA], ptDriverInfo->iInputBufSize);
					} else {
						uint64_t tmpBufferSize = (uint64_t)ptDriverInfo->iBitstreamBufSize / 2UL;

						ptDecode->gsV4kd2DecInput.m_BitstreamDataAddr[VPK_PA] 	= (uint64_t)ptDriverInfo->pucBitstreamBuffer[VPK_PA] + tmpBufferSize;
						ptDecode->gsV4kd2DecInput.m_BitstreamDataAddr[VPK_VA] 	= (uint64_t)ptDriverInfo->pucBitstreamBuffer[VPK_K_VA] + tmpBufferSize;

						VDEC_MEMCPY(ptDriverInfo->pucBitstreamBuffer[VPK_VA] + (ptDriverInfo->iBitstreamBufSize / 2), ptDriverInfo->pucInputBuffer[VPK_VA], ptDriverInfo->iInputBufSize);
					}
				} else {
					DPV_ERROR_MSG("[%s(%d)] [ERROR] pucBitstreamBuffer is NULL \n", __func__, __LINE__);
				}
			} else {
				DPV_PRINT("[%s(%d)] Same Value \n", __func__, __LINE__);
			}
		} else {
			ptDecode->gsV4kd2DecInput.m_iBitstreamDataSize = 1;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4kd2_set_bitwork_addr(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VPU_4K_D2_INIT_t *ptInit = &ptDriverInfo->tVpu4KD2Init;
		const VPK_MEM_ALLOC_INFO_t *ptAllocMem = &ptDriverInfo->tAllocMem;
		VPK_CODEC_ADDR_t *ptTmpAddr = &ptInit->gsV4kd2DecInit.m_BitWorkAddr[VPK_VA];

		ptInit->gsV4kd2DecInit.m_BitWorkAddr[VPK_PA] = ptAllocMem->phy_addr;
		*ptTmpAddr = vpu_cast_void_uintptr(ptAllocMem->kernel_remap_addr);
		DPV_DETAIL_MSG("[%s(%d)] remap_addr: 0x%x, kernel_remap_addr: 0x%x \n", __func__, __LINE__, ptTmpAddr, ptAllocMem->kernel_remap_addr);
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4kd2_set_stream_size(VPC_DRIVER_INFO_T *ptDriverInfo, int iStreamLen)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VPU_4K_D2_SEQ_HEADER_t *ptSeqHeader = &ptDriverInfo->tVpu4KD2SeqHeader;

		if (iStreamLen >= 0) {
			ptSeqHeader->stream_size = (unsigned int)iStreamLen;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4kd2_get_header_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VPK_VPU_4K_D2_SEQ_HEADER_t *ptSeqHeader = &ptDriverInfo->tVpu4KD2SeqHeader;

		ptDriverInfo->iPicWidth 	= ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicWidth;
		ptDriverInfo->iPicHeight 	= ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicHeight;
		ptDriverInfo->iAspectRatio 	= ptSeqHeader->gsV4kd2DecInitialInfo.m_iAspectRateInfo;

		ptDriverInfo->iMinFrameBufferCount 	= ptSeqHeader->gsV4kd2DecInitialInfo.m_iMinFrameBufferCount;
		ptDriverInfo->iMinFrameBufferSize 	= ptSeqHeader->gsV4kd2DecInitialInfo.m_iMinFrameBufferSize;

		#if (DPV_PRINT_DETAIL_INFO == 1)
		{
			uint32_t fRateInfoRes = ptSeqHeader->gsV4kd2DecInitialInfo.m_uiFrameRateRes;
			uint32_t fRateInfoDiv = ptSeqHeader->gsV4kd2DecInitialInfo.m_uiFrameRateDiv;

			int32_t profile = ptSeqHeader->gsV4kd2DecInitialInfo.m_iProfile;
			int32_t level 	= ptSeqHeader->gsV4kd2DecInitialInfo.m_iLevel;
			int32_t tier 	= ptSeqHeader->gsV4kd2DecInitialInfo.m_iTier;

			DPV_DETAIL_MSG("[%s(%d)] ==================================================== \n", __func__, __LINE__);
			DPV_DETAIL_MSG("[%s(%d)] [4KD2-%d] INITIAL INFO \n", __func__, __LINE__, ptDriverInfo->iInstIdx);
			DPV_DETAIL_MSG("[%s(%d)] --------------------------------------------------- \n", __func__, __LINE__);
			DPV_DETAIL_MSG("[%s(%d)]  - profile: %d level: %d tier: %d \n", __func__, __LINE__, profile, level, tier);
			DPV_DETAIL_MSG("[%s(%d)]  - Aspect Ratio: %1d(%u, %u), FrameBuffer Format: %s Bit \n", __func__, __LINE__, ptSeqHeader->gsV4kd2DecInitialInfo.m_iAspectRateInfo, fRateInfoRes, fRateInfoDiv, (ptSeqHeader->gsV4kd2DecInitialInfo.m_iFrameBufferFormat == 0) ? ("8") : ("10"));
			DPV_DETAIL_MSG("[%s(%d)]  - MinFrameBufferCount: %u, frameBufDelay: %u \n", __func__, __LINE__, ptSeqHeader->gsV4kd2DecInitialInfo.m_iMinFrameBufferCount, ptSeqHeader->gsV4kd2DecInitialInfo.m_iFrameBufDelay);
			DPV_DETAIL_MSG("[%s(%d)]  - Crop Info: %d(%d - %d), %d(%d - %d) \n", __func__, __LINE__,
												ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicWidth, ptSeqHeader->gsV4kd2DecInitialInfo.m_PicCrop.m_iCropLeft, ptSeqHeader->gsV4kd2DecInitialInfo.m_PicCrop.m_iCropRight,
												ptSeqHeader->gsV4kd2DecInitialInfo.m_iPicHeight, ptSeqHeader->gsV4kd2DecInitialInfo.m_PicCrop.m_iCropTop, ptSeqHeader->gsV4kd2DecInitialInfo.m_PicCrop.m_iCropBottom);
			DPV_DETAIL_MSG("[%s(%d)] ==================================================== \n", __func__, __LINE__);
		}
		#endif
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4kd2_get_slice_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSliceBuffer, int iSliceBufSize, int iExtraFrameCount)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VPU_4K_D2_SET_BUFFER_t *ptSetBuffer = &ptDriverInfo->tVpu4KD2SetBuffer;

		if ((INT32_MAX - ptDriverInfo->iMinFrameBufferCount) >= iExtraFrameCount) {
			ptSetBuffer->gsV4kd2DecBuffer.m_iFrameBufferCount = ptDriverInfo->iMinFrameBufferCount + iExtraFrameCount;
		}

		ptDriverInfo->pucSliceBuffer = pucSliceBuffer;
		ptDriverInfo->iSliceBufSize = iSliceBufSize;
		ptDriverInfo->iFrameBufferCount = ptSetBuffer->gsV4kd2DecBuffer.m_iFrameBufferCount;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4kd2_set_frame_buffer_start_addr_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucFrameBuffer[VPK_K_VA + 1], int iFrameBufferSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VPU_4K_D2_SET_BUFFER_t *ptSetBuffer = &ptDriverInfo->tVpu4KD2SetBuffer;

		ptSetBuffer->gsV4kd2DecBuffer.m_iFrameBufferCount = iFrameBufferSize;
		ptSetBuffer->gsV4kd2DecBuffer.m_FrameBufferStartAddr[VPK_PA] = (uint64_t)pucFrameBuffer[VPK_PA];
		ptSetBuffer->gsV4kd2DecBuffer.m_FrameBufferStartAddr[VPK_VA] = (uint64_t)pucFrameBuffer[VPK_K_VA];
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}
#endif