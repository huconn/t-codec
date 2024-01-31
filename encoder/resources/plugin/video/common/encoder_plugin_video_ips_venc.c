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


#include <encoder_plugin_video_ips.h>


void vpu_venc_set_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_VENC_INIT_t *ptInit = &ptDriverInfo->tVENC_INIT;

		ptInit->gsVpuEncInit.m_RegBaseVirtualAddr 	= 0;

		if (ptDriverInfo->uiCodec < (UINT32_MAX / 2U)) {
			ptInit->gsVpuEncInit.m_iBitstreamFormat 	= ptDriverInfo->iBitstreamFormat;
		}
		ptInit->gsVpuEncInit.m_iPicWidth 			= ptDriverInfo->iWidth;
		ptInit->gsVpuEncInit.m_iPicHeight 			= ptDriverInfo->iHeight;

		ptInit->gsVpuEncInit.m_iFrameRate 			= ptDriverInfo->iFrameRate;
		ptInit->gsVpuEncInit.m_iTargetKbps 			= ptDriverInfo->iTargetKbps;
		ptInit->gsVpuEncInit.m_iKeyInterval			= ptDriverInfo->iKeyInterval;

		ptInit->gsVpuEncInit.m_iUseSpecificRcOption 			= 1;

		if (ptInit->gsVpuEncInit.m_iBitstreamFormat == VEPK_STD_H264) {
			ptInit->gsVpuEncInit.m_stRcInit.m_iSliceMode 		= ptDriverInfo->iSliceMode; // multi-slices per picture
			ptInit->gsVpuEncInit.m_stRcInit.m_iSliceSizeMode 	= ptDriverInfo->iSliceSizeUnit;
			ptInit->gsVpuEncInit.m_stRcInit.m_iSliceSize 		= ptDriverInfo->iSliceSize;
		}

		ptInit->gsVpuEncInit.m_stRcInit.m_iAvcFastEncoding 		= ptDriverInfo->iAvcFastEncoding;
		ptInit->gsVpuEncInit.m_stRcInit.m_iPicQpY 				= (ptDriverInfo->iInitialQp > 0) ? ptDriverInfo->iInitialQp : -1;
		ptInit->gsVpuEncInit.m_stRcInit.m_iDeblkDisable 		= ptDriverInfo->iDeblkDisable;
		ptInit->gsVpuEncInit.m_stRcInit.m_iVbvBufferSize 		= ptDriverInfo->iVbvBufferSize;

		ptInit->gsVpuEncInit.m_stRcInit.m_iIntraMBRefresh 		= 0;
		ptInit->gsVpuEncInit.m_stRcInit.m_iDeblkAlpha 			= 0;
		ptInit->gsVpuEncInit.m_stRcInit.m_iDeblkBeta 			= 0;
		ptInit->gsVpuEncInit.m_stRcInit.m_iDeblkChQpOffset 		= 0;
		ptInit->gsVpuEncInit.m_stRcInit.m_iConstrainedIntra 	= 0;
		ptInit->gsVpuEncInit.m_stRcInit.m_iSearchRange 			= 2;
		ptInit->gsVpuEncInit.m_stRcInit.m_iPVMDisable 			= 0;
		ptInit->gsVpuEncInit.m_stRcInit.m_iWeightIntraCost 		= 0;
		ptInit->gsVpuEncInit.m_stRcInit.m_iRCIntervalMode 		= 1;
		ptInit->gsVpuEncInit.m_stRcInit.m_iRCIntervalMBNum 		= 0;

		if (ptDriverInfo->iMaxPQp > 0) {
			unsigned int tmpQp = 0U;

			tmpQp = (unsigned int)ptDriverInfo->iMaxPQp;
			tmpQp = tmpQp << 16U;

			ptInit->gsVpuEncInit.m_stRcInit.m_iEncQualityLevel 	= (int)tmpQp; //(ptDriverInfo->iMaxPQp << 16U);
		} else {
			ptInit->gsVpuEncInit.m_stRcInit.m_iEncQualityLevel 	= 11;
		}

		ptInit->gsVpuEncInit.m_bEnableVideoCache 	= 0;
		ptInit->gsVpuEncInit.m_bCbCrInterleaveMode 	= 0;
		ptInit->gsVpuEncInit.m_uiEncOptFlags 		= 0;

		ptInit->gsVpuEncInit.m_Memcpy 		= (unsigned long *(*)(unsigned long *, const unsigned long *, unsigned int))memcpy;
		ptInit->gsVpuEncInit.m_Memset 		= (unsigned long (*)(unsigned long *, int, unsigned int))memset;
		ptInit->gsVpuEncInit.m_Interrupt 	= (int (*)(void))NULL;

		TEPV_DETAIL_MSG("[%s(%d)] =========================================== \n", __func__, __LINE__);
		TEPV_DETAIL_MSG("[%s(%d)] INIT INFORMATION: %d \n", __func__, __LINE__, ptDriverInfo->iInstIdx);
		TEPV_DETAIL_MSG("[%s(%d)] ------------------------------------- \n", __func__, __LINE__);
		TEPV_DETAIL_MSG("[%s(%d)]   - %d X %d @ %d(%d) \n", __func__, __LINE__, ptInit->gsVpuEncInit.m_iPicWidth, ptInit->gsVpuEncInit.m_iPicHeight, ptInit->gsVpuEncInit.m_iFrameRate, ptInit->gsVpuEncInit.m_iTargetKbps);
		TEPV_DETAIL_MSG("[%s(%d)]   - m_iEncQualityLevel: %d(%d, %d) \n", __func__, __LINE__, ptInit->gsVpuEncInit.m_stRcInit.m_iEncQualityLevel, ptDriverInfo->iMaxPQp);
		TEPV_DETAIL_MSG("[%s(%d)] =========================================== \n", __func__, __LINE__);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_venc_set_encode_info(VEPC_DRIVER_INFO_T *ptDriverInfo, EP_Video_Encode_Input_T *ptEncodeInput)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_VENC_ENCODE_t *ptEncode = &ptDriverInfo->tVENC_ENCODE;

		int iResolution = ptDriverInfo->iWidth * ptDriverInfo->iHeight;

		if (ptDriverInfo->iBitstreamBufSize > 0) {
			(void)memset((void*)ptDriverInfo->pucBitstreamBuffer[VEPK_VA], 0x0, (size_t)ptDriverInfo->iBitstreamBufSize);
		}

		if (iResolution > 0) {
			unsigned int uiTmpFlag = ((unsigned int)0x20 | (unsigned int)0x1);

			int iOffsetUV = iResolution / 4;
			int iMemSize = iResolution + (iOffsetUV * 2);

			if (ptEncodeInput->pucVideoBuffer[VEPK_COMP_U] == NULL) {
				;
			}
			ptEncode->gsVpuEncInput.m_iChangeRcParamFlag 	= (int)uiTmpFlag;
			ptEncode->gsVpuEncInput.m_iQuantParam 			= 23;

			if (ptEncodeInput->pucVideoBuffer[VEPK_COMP_U] == NULL) {
				(void)memcpy(ptDriverInfo->pucRawImageBuffer[VEPK_VA], (unsigned char *)ptEncodeInput->pucVideoBuffer[VEPK_COMP_Y], (size_t)iMemSize);
				ptEncode->gsVpuEncInput.m_PicYAddr 		= vpu_enc_cast_void_uint(ptDriverInfo->pucRawImageBuffer[VEPK_PA]);
				ptEncode->gsVpuEncInput.m_PicCbAddr		= ptEncode->gsVpuEncInput.m_PicYAddr + (unsigned int)iResolution;
			} else {
				ptEncode->gsVpuEncInput.m_PicYAddr		= vpu_enc_cast_void_uint(ptEncodeInput->pucVideoBuffer[VEPK_COMP_Y]);
				ptEncode->gsVpuEncInput.m_PicCbAddr		= vpu_enc_cast_void_uint(ptEncodeInput->pucVideoBuffer[VEPK_COMP_U]);
			}

			if (iOffsetUV > 0) {
				if (ptEncodeInput->pucVideoBuffer[VEPK_COMP_V] == NULL) {
					ptEncode->gsVpuEncInput.m_PicCrAddr		= ptEncode->gsVpuEncInput.m_PicCbAddr + (unsigned int)iOffsetUV;
				} else {
					ptEncode->gsVpuEncInput.m_PicCrAddr		= vpu_enc_cast_void_uint(ptEncodeInput->pucVideoBuffer[VEPK_COMP_V]);
				}
			}
		}

		#if defined(TCC_VPU_D6_INCLUDE)
		ptEncode->gsVpuEncInput.m_iReportSliceInfoEnable	= 0;
		ptEncode->gsVpuEncInput.m_SliceInfoAddr[VEPK_PA]	= 0;
		ptEncode->gsVpuEncInput.m_SliceInfoAddr[VEPK_VA]	= 0;
		#endif

		ptEncode->gsVpuEncInput.m_iBitstreamBufferSize		= ptDriverInfo->iBitstreamBufSize;
		ptEncode->gsVpuEncInput.m_BitstreamBufferAddr		= vpu_enc_cast_void_uint(ptDriverInfo->pucBitstreamBuffer[VEPK_PA]);

	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_venc_upate_get_result_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VEPK_VENC_ENCODE_t *ptEncode = &ptDriverInfo->tVENC_ENCODE;

		ptDriverInfo->iBitStreamOutSize 	= ptEncode->gsVpuEncOutput.m_iBitstreamOutSize;
		ptDriverInfo->pucBitStreamOutBuffer = ptDriverInfo->pucBitstreamBuffer[VEPK_VA];

		if (ptEncode->gsVpuEncOutput.m_iPicType == VEPK_PIC_TYPE_I) {
			ptDriverInfo->iTotalIFrameCount++;

			ptDriverInfo->eFrameType = VEPC_FRAME_TYPE_I;
		} else if (ptEncode->gsVpuEncOutput.m_iPicType == VEPK_PIC_TYPE_P) {
			ptDriverInfo->iTotalPFrameCount++;

			ptDriverInfo->eFrameType = VEPC_FRAME_TYPE_P;
		} else {
			TEPV_DONOTHING(0);
		}

		ptDriverInfo->iTotalFrameCount++;

	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_venc_get_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VEPK_VENC_INIT_t *ptInit = &ptDriverInfo->tVENC_INIT;

		ptDriverInfo->iMinFrameBufferCount 	= ptInit->gsVpuEncInitialInfo.m_iMinFrameBufferCount;
		ptDriverInfo->iMinFrameBufferSize 	= ptInit->gsVpuEncInitialInfo.m_iMinFrameBufferSize;

		TEPV_DETAIL_MSG("[%s(%d)] =========================================== \n", __func__, __LINE__);
		TEPV_DETAIL_MSG("[%s(%d)] INIT RESULT INFORMATION: %d \n", __func__, __LINE__, ptDriverInfo->iInstIdx);
		TEPV_DETAIL_MSG("[%s(%d)] ------------------------------------- \n", __func__, __LINE__);
		TEPV_DETAIL_MSG("[%s(%d)]   - FrameBuffer: %d, %d \n", __func__, __LINE__, ptDriverInfo->iMinFrameBufferCount, ptDriverInfo->iMinFrameBufferSize);
		TEPV_DETAIL_MSG("[%s(%d)] =========================================== \n", __func__, __LINE__);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_venc_get_header_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VEPK_VENC_PUT_HEADER_t *ptHeader = &ptDriverInfo->tVENC_PUT_HEADER;
		unsigned char *pucDest = ptDriverInfo->pucSeqHeaderBufferOutVA;

		if (pucDest != NULL) {
			if ((ptDriverInfo->iSeqHeaderBufferOutSize > 0) && (ptDriverInfo->iSeqHeaderBufferOutSize < (INT32_MAX - 1))) {
				pucDest = ptDriverInfo->pucSeqHeaderBufferOutVA + ptDriverInfo->iSeqHeaderBufferOutSize;
			} else {
				pucDest = ptDriverInfo->pucSeqHeaderBufferOutVA;
			}

			if (ptHeader->gsVpuEncHeader.m_iHeaderSize > 0) {
				if (ptDriverInfo->bNalStartCode == true) {
					(void)memcpy(pucDest, ptDriverInfo->pucSeqHeaderBuffer[VEPK_VA], (size_t)ptHeader->gsVpuEncHeader.m_iHeaderSize);
				}

				(void)memcpy(pucDest, ptDriverInfo->pucSeqHeaderBuffer[VEPK_VA], (size_t)ptHeader->gsVpuEncHeader.m_iHeaderSize);
			}
		}

		if (((INT32_MAX / 2) > ptDriverInfo->iSeqHeaderBufferOutSize) && ((INT32_MAX / 2) > ptHeader->gsVpuEncHeader.m_iHeaderSize)) {
			ptDriverInfo->iSeqHeaderBufferOutSize += ptHeader->gsVpuEncHeader.m_iHeaderSize;
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_venc_set_bitstream_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_VENC_INIT_t *ptInit = &ptDriverInfo->tVENC_INIT;

		ptInit->gsVpuEncInit.m_BitstreamBufferAddr 		= vpu_enc_cast_void_uint(pucBuffer[VEPK_PA]);
		ptInit->gsVpuEncInit.m_BitstreamBufferAddr_VA 	= vpu_enc_cast_void_uintptr(pucBuffer[VEPK_K_VA]);
		ptInit->gsVpuEncInit.m_iBitstreamBufferSize		= iBufSize;

		if (iBufSize > 0) {
			(void)memset((void*)pucBuffer[VEPK_VA], 0x0, (size_t)iBufSize);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_venc_set_bitwork_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_VENC_INIT_t *ptInit = &ptDriverInfo->tVENC_INIT;

		ptInit->gsVpuEncInit.m_BitWorkAddr[VEPK_PA]		= vpu_enc_cast_void_uintptr(pucBuffer[VEPK_PA]);
		ptInit->gsVpuEncInit.m_BitWorkAddr[VEPK_VA] 	= vpu_enc_cast_void_uintptr(pucBuffer[VEPK_K_VA]);
		(void)iBufSize;
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_venc_set_frame_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_VENC_SET_BUFFER_t *ptSetBuffer = &ptDriverInfo->tVENC_SET_BUFFER;

		ptSetBuffer->gsVpuEncBuffer.m_FrameBufferStartAddr[VEPK_PA]		= vpu_enc_cast_void_uintptr(pucBuffer[VEPK_PA]);
		ptSetBuffer->gsVpuEncBuffer.m_FrameBufferStartAddr[VEPK_VA] 	= vpu_enc_cast_void_uintptr(pucBuffer[VEPK_K_VA]);
		(void)iBufSize;
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_venc_set_seq_header_info(VEPC_DRIVER_INFO_T *ptDriverInfo, int iType, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_VENC_PUT_HEADER_t *ptHeader = &ptDriverInfo->tVENC_PUT_HEADER;

		ptHeader->gsVpuEncHeader.m_iHeaderType 		= iType;
		ptHeader->gsVpuEncHeader.m_iHeaderSize 		= iBufSize;
		ptHeader->gsVpuEncHeader.m_HeaderAddr 		= vpu_enc_cast_void_uint(pucBuffer[VEPK_PA]);
		#if defined(TCC_VPU_C7_INCLUDE)
		ptHeader->gsVpuEncHeader.m_HeaderAddr_VA 	= vpu_enc_cast_void_uint(pucBuffer[VEPK_VA]);
		#endif
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}