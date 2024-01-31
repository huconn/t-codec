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


#if defined(TCC_JPU_C6_INCLUDE)
void vpu_jdec_set_init_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_JDEC_INIT_t *ptInit = &ptDriverInfo->tJdecInit;

		ptInit->gsJpuDecInit.m_iCbCrInterleaveMode 	= (int)ptDriverInfo->bCbCrInterleaveMode;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_jdec_set_seq_header_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, int iBitstreamBufSize, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1])
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_JDEC_INIT_t *ptInit = &ptDriverInfo->tJdecInit;

		#if 0
		VPK_JPU_DECODE_t *ptDecode = &ptDriverInfo->tJdecDecode;

		ptDecode->gsJpuDecInput.m_BitstreamDataAddr[VPK_PA] = (uint64_t)pucBitstreamBuffer[VPK_PA];
		ptDecode->gsJpuDecInput.m_BitstreamDataAddr[VPK_VA] = (uint64_t)pucBitstreamBuffer[VPK_K_VA];
		ptDecode->gsJpuDecInput.m_iBitstreamDataSize 		= iBitstreamBufSize;
		#endif

		ptInit->gsJpuDecInit.m_BitstreamBufAddr[VPK_PA] 	= (uint64_t)pucBitstreamBuffer[VPK_PA];
		ptInit->gsJpuDecInit.m_BitstreamBufAddr[VPK_VA] 	= (uint64_t)pucBitstreamBuffer[VPK_K_VA];
		ptInit->gsJpuDecInit.m_iBitstreamBufSize 			= iBitstreamBufSize;

		vpu_jdec_set_stream_size(ptDriverInfo, iBitstreamBufSize);
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

int vpu_jdec_is_pic_resolution_ok(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_RESOLUTION_INFO_T tMaxResInfo, VPC_RESOLUTION_INFO_T tMinResInfo)
{
	int iRetVal = -1;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if ((tMaxResInfo.iWidth < INT16_MAX) && (tMaxResInfo.iHeight < INT16_MAX)) {
			const VPK_JDEC_SEQ_HEADER_t *ptSeqHeader = &ptDriverInfo->tJdecSeqHeader;

			unsigned int tmpWidth = (((unsigned int)tMaxResInfo.iWidth + 15U) & (unsigned int)0xFFF0);

			if ((ptSeqHeader->gsJpuDecInitialInfo.m_iPicWidth > (int)tmpWidth) ||
				((ptSeqHeader->gsJpuDecInitialInfo.m_iPicWidth * ptSeqHeader->gsJpuDecInitialInfo.m_iPicHeight) > (tMaxResInfo.iWidth * tMaxResInfo.iHeight)) ||
				(ptSeqHeader->gsJpuDecInitialInfo.m_iPicWidth < tMinResInfo.iWidth) ||
				(ptSeqHeader->gsJpuDecInitialInfo.m_iPicHeight < tMinResInfo.iHeight)) {
				DPV_ERROR_MSG("[%s(%d)] [ERROR] (%d X %d) \n", __func__, __LINE__, ptSeqHeader->gsJpuDecInitialInfo.m_iPicWidth, ptSeqHeader->gsJpuDecInitialInfo.m_iPicHeight);
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

void vpu_jdec_get_decode_result_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	const VPK_JPU_DECODE_t *ptDecoded = &ptDriverInfo->tJdecDecode;
	VPC_DECODE_RESULT_INFO_T *ptDecResult = &ptDriverInfo->tDecodeResultInfo;

	ptDecResult->iDecodingResult = ptDecoded->gsJpuDecOutput.m_DecOutInfo.m_iDecodingStatus;

	if (ptDecResult->iDecodingResult == VPK_VPU_DEC_SUCCESS) {
		ptDecResult->iDecodingOutResult 			= VPK_VPU_DEC_OUTPUT_SUCCESS;

		ptDecResult->tVideoResolution.iWidth 		= ptDecoded->gsJpuDecOutput.m_DecOutInfo.m_iWidth;
		ptDecResult->tVideoResolution.iHeight 		= ptDecoded->gsJpuDecOutput.m_DecOutInfo.m_iHeight;

		ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_Y] = ptDecoded->gsJpuDecOutput.m_pCurrOut[VPK_PA][VPK_COMP_Y];
		ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_U] = ptDecoded->gsJpuDecOutput.m_pCurrOut[VPK_PA][VPK_COMP_U];
		ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_V] = ptDecoded->gsJpuDecOutput.m_pCurrOut[VPK_PA][VPK_COMP_V];

		ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_Y] = ptDecoded->gsJpuDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_Y];
		ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_U] = ptDecoded->gsJpuDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_U];
		ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_V] = ptDecoded->gsJpuDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_V];

		ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_Y] = vpu_get_framebuf_virtaddr(ptDriverInfo->pucFrameBuffer, ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_Y], VPK_K_VA);
		ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_U] = vpu_get_framebuf_virtaddr(ptDriverInfo->pucFrameBuffer, ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_U], VPK_K_VA);
		ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_V] = vpu_get_framebuf_virtaddr(ptDriverInfo->pucFrameBuffer, ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_V], VPK_K_VA);

		ptDecResult->pucDispOut[VPK_PA][VPK_COMP_Y] = ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_Y];
		ptDecResult->pucDispOut[VPK_PA][VPK_COMP_U] = ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_U];
		ptDecResult->pucDispOut[VPK_PA][VPK_COMP_V] = ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_V];

		ptDecResult->pucDispOut[VPK_VA][VPK_COMP_Y] = ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_Y];
		ptDecResult->pucDispOut[VPK_VA][VPK_COMP_U] = ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_U];
		ptDecResult->pucDispOut[VPK_VA][VPK_COMP_V] = ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_V];

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
		DPV_DETAIL_MSG("[%s(%d)]     - I: %d(%d, %d)  \n", __func__, __LINE__, ptDecResult->iInterlace, ptDecResult->iInterlacedFrame, ptDecResult->iTopFieldFirst);
		DPV_DETAIL_MSG("[%s(%d)]     - P: %d, %d  \n", __func__, __LINE__, ptDecResult->iM2vProgressiveFrame, ptDecResult->iM2vAspectRatio);
		DPV_DETAIL_MSG("[%s(%d)]   - RES: %d X %d  \n", __func__, __LINE__, ptDecResult->tVideoResolution.iWidth, ptDecResult->tVideoResolution.iHeight);
		DPV_DETAIL_MSG("[%s(%d)]     - CROP: %d, %d, %d, %d  \n", __func__, __LINE__, ptDecResult->tCropRect.iLeft, ptDecResult->tCropRect.iTop, ptDecResult->tCropRect.iRight, ptDecResult->tCropRect.iBottom);
		DPV_DETAIL_MSG("[%s(%d)]   - CUR:  \n", __func__, __LINE__);
		DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_Y], ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_U], ptDecResult->pucCurrOut[VPK_PA][VPK_COMP_V]);
		DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_Y], ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_U], ptDecResult->pucCurrOut[VPK_VA][VPK_COMP_V]);
		DPV_DETAIL_MSG("[%s(%d)]     - P: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsJpuDecOutput.m_pCurrOut[VPK_PA][VPK_COMP_Y], ptDecoded->gsJpuDecOutput.m_pCurrOut[VPK_PA][VPK_COMP_U], ptDecoded->gsJpuDecOutput.m_pCurrOut[VPK_PA][VPK_COMP_V]);
		DPV_DETAIL_MSG("[%s(%d)]     - V: %p, %p %p  \n", __func__, __LINE__, ptDecoded->gsJpuDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_Y], ptDecoded->gsJpuDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_U], ptDecoded->gsJpuDecOutput.m_pCurrOut[VPK_VA][VPK_COMP_V]);
		DPV_DETAIL_MSG("[%s(%d)] ============================================================================== \n", __func__, __LINE__);
		#endif
	}
}

void vpu_jdec_set_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1], int iBitstreamBufSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_JDEC_INIT_t *ptInit = &ptDriverInfo->tJdecInit;

		ptInit->gsJpuDecInit.m_BitstreamBufAddr[VPK_PA] 	= (uint64_t)pucBitstreamBuffer[VPK_PA];
		ptInit->gsJpuDecInit.m_BitstreamBufAddr[VPK_VA] 	= (uint64_t)pucBitstreamBuffer[VPK_K_VA];
		ptInit->gsJpuDecInit.m_iBitstreamBufSize 			= iBitstreamBufSize;
		ptInit->gsJpuDecInit.m_uiDecOptFlags 				= 0U;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_jdec_set_decoding_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_JPU_DECODE_t *ptDecode = &ptDriverInfo->tJdecDecode;

		ptDecode->gsJpuDecInput.m_iBitstreamDataSize		= ptDriverInfo->iInputBufSize;
		ptDecode->gsJpuDecInput.m_BitstreamDataAddr[VPK_PA] = (uint64_t)ptDriverInfo->pucBitstreamBuffer[VPK_PA];
		ptDecode->gsJpuDecInput.m_BitstreamDataAddr[VPK_VA] = (uint64_t)ptDriverInfo->pucBitstreamBuffer[VPK_K_VA];

		if (ptDriverInfo->iInputBufSize > 0) {
			VDEC_MEMCPY(ptDriverInfo->pucBitstreamBuffer[VPK_VA], ptDriverInfo->pucInputBuffer[VPK_VA], (size_t)ptDriverInfo->iInputBufSize);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_jdec_set_stream_size(VPC_DRIVER_INFO_T *ptDriverInfo, int iStreamLen)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_JDEC_SEQ_HEADER_t *ptSeqHeader = &ptDriverInfo->tJdecSeqHeader;

		if (iStreamLen >= 0) {
			ptSeqHeader->stream_size = (unsigned int)iStreamLen;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_jdec_get_header_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_JDEC_SEQ_HEADER_t *ptSeqHeader = &ptDriverInfo->tJdecSeqHeader;

		unsigned int tmpWidth = 0U;
		unsigned int tmpHeight = 0U;

		if ((ptSeqHeader->gsJpuDecInitialInfo.m_iPicWidth > 0) && (ptSeqHeader->gsJpuDecInitialInfo.m_iPicHeight > 0)) {
			tmpWidth = (unsigned int)ptSeqHeader->gsJpuDecInitialInfo.m_iPicWidth;
			tmpHeight = (unsigned int)ptSeqHeader->gsJpuDecInitialInfo.m_iPicHeight;
		}

		tmpWidth = ((tmpWidth + 15U) >> 4U) << 4U;

		if ((tmpWidth < (UINT32_MAX / 2U)) && (tmpHeight < (UINT32_MAX / 2U))) {
			ptSeqHeader->gsJpuDecInitialInfo.m_iPicWidth 	= (int)tmpWidth;
			ptSeqHeader->gsJpuDecInitialInfo.m_iPicHeight 	= (int)tmpHeight;
		}

		ptDriverInfo->iPicWidth = ptSeqHeader->gsJpuDecInitialInfo.m_iPicWidth;
		ptDriverInfo->iPicHeight = ptSeqHeader->gsJpuDecInitialInfo.m_iPicHeight;

		ptDriverInfo->iMinFrameBufferCount = ptSeqHeader->gsJpuDecInitialInfo.m_iMinFrameBufferCount;
		ptDriverInfo->iMinFrameBufferSize = ptSeqHeader->gsJpuDecInitialInfo.m_iMinFrameBufferSize[0];
		ptDriverInfo->iFrameBufferCount = ptDriverInfo->iMinFrameBufferSize;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_jdec_set_frame_buffer_start_addr_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucFrameBuffer[VPK_K_VA + 1], int iFrameBufferSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_JPU_SET_BUFFER_t *ptSetBuffer = &ptDriverInfo->tJdecSetBuffer;

		ptSetBuffer->gsJpuDecBuffer.m_iFrameBufferCount = iFrameBufferSize;
		ptSetBuffer->gsJpuDecBuffer.m_FrameBufferStartAddr[VPK_PA] = (uint64_t)pucFrameBuffer[VPK_PA];
		ptSetBuffer->gsJpuDecBuffer.m_FrameBufferStartAddr[VPK_VA] = (uint64_t)pucFrameBuffer[VPK_K_VA];
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}
#endif