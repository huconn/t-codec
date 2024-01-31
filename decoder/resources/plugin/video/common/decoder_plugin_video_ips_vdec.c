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


void vpu_vdec_set_init_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VDEC_INIT_t *ptInit = &ptDriverInfo->tInit;

		int tmpUserData = (int)ptDriverInfo->bEnableUserData;
		int tmpInterlevemode = (int)ptDriverInfo->bCbCrInterleaveMode;

		ptInit->gsVpuDecInit.m_RegBaseVirtualAddr 	= ptDriverInfo->uiRegBaseVirtualAddr;
		ptInit->gsVpuDecInit.m_iBitstreamFormat 	= ptDriverInfo->iBitstreamFormat;
		ptInit->gsVpuDecInit.m_iPicWidth			= ptDriverInfo->iPicWidth;
		ptInit->gsVpuDecInit.m_iPicHeight			= ptDriverInfo->iPicHeight;

		if ((tmpUserData >= 0) && (tmpInterlevemode >= 0)) {
			ptInit->gsVpuDecInit.m_bEnableUserData		= (unsigned int)tmpUserData;
			ptInit->gsVpuDecInit.m_bCbCrInterleaveMode = (unsigned int)tmpInterlevemode;
		}
		ptInit->gsVpuDecInit.m_iFilePlayEnable 		= ptDriverInfo->iFilePlayEnable;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_vdec_set_seq_header_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, int iBitstreamBufSize, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1])
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VDEC_DECODE_t *ptDecode = &ptDriverInfo->tDecode;

		ptDecode->gsVpuDecInput.m_BitstreamDataAddr[VPK_PA] = (uint64_t)pucBitstreamBuffer[VPK_PA];
		ptDecode->gsVpuDecInput.m_BitstreamDataAddr[VPK_VA] = (uint64_t)pucBitstreamBuffer[VPK_K_VA];
		ptDecode->gsVpuDecInput.m_iBitstreamDataSize 		= iBitstreamBufSize;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

int vpu_vdec_is_pic_resolution_ok(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_RESOLUTION_INFO_T tMaxResInfo, VPC_RESOLUTION_INFO_T tMinResInfo)
{
	int iRetVal = -1;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if ((tMaxResInfo.iWidth < INT16_MAX) && (tMaxResInfo.iHeight < INT16_MAX)) {
			const VPK_VDEC_SEQ_HEADER_t *ptSeqHeader = &ptDriverInfo->tSeqHeader;

			unsigned int tmpWidth = (((unsigned int)tMaxResInfo.iWidth + 15U) & (unsigned int)0xFFF0);

			if ((ptSeqHeader->gsVpuDecInitialInfo.m_iPicWidth > (int)tmpWidth) ||
				((ptSeqHeader->gsVpuDecInitialInfo.m_iPicWidth * ptSeqHeader->gsVpuDecInitialInfo.m_iPicHeight) > (tMaxResInfo.iWidth * tMaxResInfo.iHeight)) ||
				(ptSeqHeader->gsVpuDecInitialInfo.m_iPicWidth < tMinResInfo.iWidth) ||
				(ptSeqHeader->gsVpuDecInitialInfo.m_iPicHeight < tMinResInfo.iHeight)) {
				DPV_ERROR_MSG("[%s(%d)] [ERROR] (%d X %d) \n", __func__, __LINE__, ptSeqHeader->gsVpuDecInitialInfo.m_iPicWidth, ptSeqHeader->gsVpuDecInitialInfo.m_iPicHeight);
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

void vpu_vdec_get_decode_result_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	const VPK_VDEC_DECODE_t *ptDecoded = &ptDriverInfo->tDecode;
	VPC_DECODE_RESULT_INFO_T *ptDecResult = &ptDriverInfo->tDecodeResultInfo;

	ptDecResult->iDecodingResult 				= ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iDecodingStatus;
	ptDecResult->iDecodingOutResult				= ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iOutputStatus;
	ptDecResult->iDecodedIdx 					= ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iDecodedIdx;
	ptDecResult->iDispOutIdx 					= ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iDispOutIdx;

	ptDecResult->iPictureStructure 				= ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iPictureStructure;
	ptDecResult->iPictureType 					= ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iPicType;
	ptDecResult->iInterlace 					= ptDecoded->gsVpuDecInitialInfo.m_iInterlace;
	ptDecResult->iInterlacedFrame 				= ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iInterlacedFrame;
	ptDecResult->iTopFieldFirst 				= ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iTopFieldFirst;
	ptDecResult->iM2vProgressiveFrame 			= ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iM2vProgressiveFrame;
	ptDecResult->iM2vAspectRatio 				= ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iM2vAspectRatio;

	if ((ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_CropInfo.m_iCropLeft   < (unsigned int)UINT16_MAX) &&
		(ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_CropInfo.m_iCropRight  < (unsigned int)UINT16_MAX) &&
		(ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_CropInfo.m_iCropTop    < (unsigned int)UINT16_MAX) &&
		(ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_CropInfo.m_iCropBottom < (unsigned int)UINT16_MAX)) {
		ptDecResult->tCropRect.iLeft 				= (int)ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_CropInfo.m_iCropLeft;
		ptDecResult->tCropRect.iRight 				= (int)ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_CropInfo.m_iCropRight;
		ptDecResult->tCropRect.iTop 				= (int)ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_CropInfo.m_iCropTop;
		ptDecResult->tCropRect.iBottom 				= (int)ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_CropInfo.m_iCropBottom;
	}

	ptDecResult->tVideoResolution.iWidth 		= ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iWidth;
	ptDecResult->tVideoResolution.iHeight 		= ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iHeight;

	ptDecResult->pucDispOut[VPK_PA][VPK_COMP_Y] = ptDecoded->gsVpuDecOutput.m_pDispOut[VPK_PA][VPK_COMP_Y];
	ptDecResult->pucDispOut[VPK_PA][VPK_COMP_U] = ptDecoded->gsVpuDecOutput.m_pDispOut[VPK_PA][VPK_COMP_U];
	ptDecResult->pucDispOut[VPK_PA][VPK_COMP_V] = ptDecoded->gsVpuDecOutput.m_pDispOut[VPK_PA][VPK_COMP_V];

	ptDecResult->pucDispOut[VPK_VA][VPK_COMP_Y] = ptDecoded->gsVpuDecOutput.m_pDispOut[VPK_VA][VPK_COMP_Y];
	ptDecResult->pucDispOut[VPK_VA][VPK_COMP_U] = ptDecoded->gsVpuDecOutput.m_pDispOut[VPK_VA][VPK_COMP_U];
	ptDecResult->pucDispOut[VPK_VA][VPK_COMP_V] = ptDecoded->gsVpuDecOutput.m_pDispOut[VPK_VA][VPK_COMP_V];

	ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_Y] = ptDecoded->gsVpuDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_Y];
	ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_U] = ptDecoded->gsVpuDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_U];
	ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_V] = ptDecoded->gsVpuDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_V];

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
	DPV_DETAIL_MSG("[%s(%d)]   - PIC: %d, %d  \n", __func__, __LINE__, ptDecResult->iPictureStructure, ptDecoded->gsVpuDecOutput.m_DecOutInfo.m_iPicType);
	DPV_DETAIL_MSG("[%s(%d)]     - I: %d(%d, %d)  \n", __func__, __LINE__, ptDecResult->iInterlace, ptDecResult->iInterlacedFrame, ptDecResult->iTopFieldFirst);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %d, %d  \n", __func__, __LINE__, ptDecResult->iM2vProgressiveFrame, ptDecResult->iM2vAspectRatio);
	DPV_DETAIL_MSG("[%s(%d)]   - RES: %d X %d  \n", __func__, __LINE__, ptDecResult->tVideoResolution.iWidth, ptDecResult->tVideoResolution.iHeight);
	DPV_DETAIL_MSG("[%s(%d)]     - CROP: %d, %d, %d, %d  \n", __func__, __LINE__, ptDecResult->tCropRect.iLeft, ptDecResult->tCropRect.iTop, ptDecResult->tCropRect.iRight, ptDecResult->tCropRect.iBottom);
	DPV_DETAIL_MSG("[%s(%d)]   - DIS:  \n", __func__, __LINE__);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucDispOut[VPK_PA][VPK_COMP_Y], ptDecResult->pucDispOut[VPK_PA][VPK_COMP_U], ptDecResult->pucDispOut[VPK_PA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucDispOut[VPK_VA][VPK_COMP_Y], ptDecResult->pucDispOut[VPK_VA][VPK_COMP_U], ptDecResult->pucDispOut[VPK_VA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsVpuDecOutput.m_pDispOut[VPK_PA][VPK_COMP_Y], ptDecoded->gsVpuDecOutput.m_pDispOut[VPK_PA][VPK_COMP_U], ptDecoded->gsVpuDecOutput.m_pDispOut[VPK_PA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsVpuDecOutput.m_pDispOut[VPK_VA][VPK_COMP_Y], ptDecoded->gsVpuDecOutput.m_pDispOut[VPK_VA][VPK_COMP_U], ptDecoded->gsVpuDecOutput.m_pDispOut[VPK_VA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]   - CUR:  \n", __func__, __LINE__);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_Y], ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_U], ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_Y], ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_U], ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsVpuDecOutput.m_pCurrOut[VPK_PA][VPK_COMP_Y], ptDecoded->gsVpuDecOutput.m_pCurrOut[VPK_PA][VPK_COMP_U], ptDecoded->gsVpuDecOutput.m_pCurrOut[VPK_PA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsVpuDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_Y], ptDecoded->gsVpuDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_U], ptDecoded->gsVpuDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_V]);
	DPV_DETAIL_MSG("[%s(%d)] ============================================================================== \n", __func__, __LINE__);
	#endif
}

void vpu_vdec_set_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1], int iBitstreamBufSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VDEC_INIT_t *ptInit = &ptDriverInfo->tInit;

		ptInit->gsVpuDecInit.m_BitstreamBufAddr[VPK_PA] 	= (uint64_t)pucBitstreamBuffer[VPK_PA];
		ptInit->gsVpuDecInit.m_BitstreamBufAddr[VPK_VA] 	= (uint64_t)pucBitstreamBuffer[VPK_K_VA];
		ptInit->gsVpuDecInit.m_iBitstreamBufSize 			= iBitstreamBufSize;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_vdec_set_decoding_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VPK_VDEC_INIT_t *ptInit = &ptDriverInfo->tInit;
		VPK_VDEC_DECODE_t *ptDecode = &ptDriverInfo->tDecode;

		ptDecode->gsVpuDecInput.m_iBitstreamDataSize		= ptDriverInfo->iInputBufSize;
		ptDecode->gsVpuDecInput.m_BitstreamDataAddr[VPK_PA] = (uint64_t)ptDriverInfo->pucBitstreamBuffer[VPK_PA];
		ptDecode->gsVpuDecInput.m_BitstreamDataAddr[VPK_VA] = (uint64_t)ptDriverInfo->pucBitstreamBuffer[VPK_K_VA];

		if (ptInit->gsVpuDecInit.m_iFilePlayEnable != 0 ) {
			if ((ptDriverInfo->pucInputBuffer[VPK_PA] != ptDriverInfo->pucBitstreamBuffer[VPK_PA]) || (ptDriverInfo->pucInputBuffer[VPK_VA] != ptDriverInfo->pucBitstreamBuffer[VPK_VA])) {
				if ((ptDriverInfo->pucBitstreamBuffer[VPK_VA] != NULL) && (ptDriverInfo->pucInputBuffer[VPK_VA] != NULL) && (ptDriverInfo->iInputBufSize > 0)) {
					VDEC_MEMCPY(ptDriverInfo->pucBitstreamBuffer[VPK_VA], ptDriverInfo->pucInputBuffer[VPK_VA], ptDriverInfo->iInputBufSize);
				} else {
					DPV_ERROR_MSG("[%s(%d)] [ERROR] pucBitstreamBuffer[VPK_VA] or pucInputBuffer[VPK_VA] is NULL \n", __func__, __LINE__);
				}
			} else {
				DPV_PRINT("[%s(%d)] Same Value \n", __func__, __LINE__);
			}
		} else {
			ptDecode->gsVpuDecInput.m_iBitstreamDataSize = 1;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_vdec_set_spspps_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSpsPpsBuffer, int iSpsPpsSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VDEC_INIT_t *ptInit = &ptDriverInfo->tInit;

		ptInit->gsVpuDecInit.m_pSpsPpsSaveBuffer 		= pucSpsPpsBuffer;
		ptInit->gsVpuDecInit.m_iSpsPpsSaveBufferSize 	= iSpsPpsSize;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_vdec_set_bitwork_addr(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VDEC_INIT_t *ptInit = &ptDriverInfo->tInit;
		const VPK_MEM_ALLOC_INFO_t *ptAllocMem = &ptDriverInfo->tAllocMem;
		VPK_CODEC_ADDR_t *ptTmpAddr = &ptInit->gsVpuDecInit.m_BitWorkAddr[VPK_VA];

		ptInit->gsVpuDecInit.m_BitWorkAddr[VPK_PA] = ptAllocMem->phy_addr;
		*ptTmpAddr = vpu_cast_void_uintptr(ptAllocMem->kernel_remap_addr);
		DPV_DETAIL_MSG("[%s(%d)] remap_addr: 0x%x, kernel_remap_addr: 0x%x \n", __func__, __LINE__, ptTmpAddr, ptAllocMem->kernel_remap_addr);
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_vdec_set_stream_size(VPC_DRIVER_INFO_T *ptDriverInfo, int iStreamLen)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VDEC_SEQ_HEADER_t *ptSeqHeader = &ptDriverInfo->tSeqHeader;

		if (iStreamLen >= 0) {
			ptSeqHeader->stream_size = (unsigned int)iStreamLen;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_vdec_get_header_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VDEC_SEQ_HEADER_t *ptSeqHeader = &ptDriverInfo->tSeqHeader;

		unsigned int tmpWidth = 0U;
		unsigned int tmpHeight = 0U;

		if ((ptSeqHeader->gsVpuDecInitialInfo.m_iPicWidth > 0) && (ptSeqHeader->gsVpuDecInitialInfo.m_iPicHeight > 0)) {
			tmpWidth = (unsigned int)ptSeqHeader->gsVpuDecInitialInfo.m_iPicWidth - ptSeqHeader->gsVpuDecInitialInfo.m_iAvcPicCrop.m_iCropLeft - ptSeqHeader->gsVpuDecInitialInfo.m_iAvcPicCrop.m_iCropRight;
			tmpHeight = (unsigned int)ptSeqHeader->gsVpuDecInitialInfo.m_iPicHeight - ptSeqHeader->gsVpuDecInitialInfo.m_iAvcPicCrop.m_iCropBottom - ptSeqHeader->gsVpuDecInitialInfo.m_iAvcPicCrop.m_iCropTop;
		}

		tmpWidth = ((tmpWidth + 15U) >> 4U) << 4U;

		if ((tmpWidth < (UINT32_MAX / 2U)) && (tmpHeight < (UINT32_MAX / 2U))) {
			ptSeqHeader->gsVpuDecInitialInfo.m_iPicWidth 	= (int)tmpWidth;
			ptSeqHeader->gsVpuDecInitialInfo.m_iPicHeight 	= (int)tmpHeight;
		}

		ptDriverInfo->iPicWidth = ptSeqHeader->gsVpuDecInitialInfo.m_iPicWidth;
		ptDriverInfo->iPicHeight = ptSeqHeader->gsVpuDecInitialInfo.m_iPicHeight;

		ptDriverInfo->iMinFrameBufferCount = ptSeqHeader->gsVpuDecInitialInfo.m_iMinFrameBufferCount;
		ptDriverInfo->iMinFrameBufferSize = ptSeqHeader->gsVpuDecInitialInfo.m_iMinFrameBufferSize;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_vdec_get_slice_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSliceBuffer, int iSliceBufSize, int iExtraFrameCount)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VDEC_SET_BUFFER_t *ptSetBuffer = &ptDriverInfo->tSetBuffer;
		uint64_t tmpPtr = (uint64_t)pucSliceBuffer;

		if (tmpPtr <= (UINT32_MAX)) {
			ptSetBuffer->gsVpuDecBuffer.m_AvcSliceSaveBufferAddr = (uint32_t)tmpPtr;
		}
		ptSetBuffer->gsVpuDecBuffer.m_iAvcSliceSaveBufferSize = iSliceBufSize;

		if ((INT32_MAX - ptDriverInfo->iMinFrameBufferCount) >= iExtraFrameCount) {
			ptSetBuffer->gsVpuDecBuffer.m_iFrameBufferCount = ptDriverInfo->iMinFrameBufferCount + iExtraFrameCount;
		}

		ptDriverInfo->pucSliceBuffer = pucSliceBuffer;
		ptDriverInfo->iSliceBufSize = iSliceBufSize;
		ptDriverInfo->iFrameBufferCount = ptSetBuffer->gsVpuDecBuffer.m_iFrameBufferCount;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_vdec_set_frame_buffer_start_addr_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucFrameBuffer[VPK_K_VA + 1], int iFrameBufferSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_VDEC_SET_BUFFER_t *ptSetBuffer = &ptDriverInfo->tSetBuffer;

		ptSetBuffer->gsVpuDecBuffer.m_iFrameBufferCount = iFrameBufferSize;
		ptSetBuffer->gsVpuDecBuffer.m_FrameBufferStartAddr[VPK_PA] = (uint64_t)pucFrameBuffer[VPK_PA];
		ptSetBuffer->gsVpuDecBuffer.m_FrameBufferStartAddr[VPK_VA] = (uint64_t)pucFrameBuffer[VPK_K_VA];
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}