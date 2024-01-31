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


#if defined(TCC_VPU_4K_E1_INCLUDE)
unsigned int vpu_4ke1_reg_read(unsigned long *base_addr, unsigned int offset)
{
	TEPV_DETAIL_MSG("[%s(%d)] [vetc_reg_read] address: %p \n", __func__, __LINE__, ((unsigned long)base_addr + offset));

	return *((volatile unsigned long *)(base_addr + offset));
}

void vpu_4ke1_reg_write(unsigned long *base_addr, unsigned int offset, unsigned int data)
{
	*((volatile unsigned long *)(base_addr + offset)) = (unsigned int)(data);
	TEPV_DETAIL_MSG("[%s(%d)] [vetc_reg_read] address: %p, data: %p \n", __func__, __LINE__, ((unsigned long)base_addr + offset), data);
	return;
}

void vpu_4ke1_set_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_HEVC_INIT_t *ptInit = &ptDriverInfo->tHEVC_INIT;

		ptInit->encInit.m_RegBaseAddr[VEPK_PA] 			= (codec_addr_t)0;
		ptInit->encInit.m_RegBaseAddr[VEPK_VA] 			= (codec_addr_t)0;

		ptInit->encInit.m_BitstreamBufferAddr[VEPK_PA]	= (codec_addr_t)ptDriverInfo->pucBitstreamBuffer[VEPK_PA];
		ptInit->encInit.m_BitstreamBufferAddr[VEPK_VA]	= (codec_addr_t)ptDriverInfo->pucBitstreamBuffer[VEPK_VA];
		ptInit->encInit.m_iBitstreamBufferSize 			= ptDriverInfo->iBitstreamBufSize;
		ptInit->encInit.m_iBitstreamFormat 				= ptDriverInfo->iBitstreamFormat;

		ptInit->encInit.m_iPicWidth 					= ptDriverInfo->iWidth;
		ptInit->encInit.m_iPicHeight 					= ptDriverInfo->iHeight;

		ptInit->encInit.m_iFrameRate 					= 1;

		if (ptDriverInfo->iFrameRate > 0) {
			ptInit->encInit.m_iFrameRate 				= ptDriverInfo->iFrameRate;
		}
		ptInit->encInit.m_iTargetKbps 					= ptDriverInfo->iTargetKbps;
		ptInit->encInit.m_iKeyInterval 					= ptDriverInfo->iKeyInterval;

		ptInit->encInit.m_bCbCrInterleaveMode			= 0;
		ptInit->encInit.m_uiEncOptFlags 				= 0;

		ptInit->encInit.m_Memcpy		 				= (unsigned long *(*)(unsigned long *, const unsigned long *, unsigned int, unsigned int))memcpy;
		ptInit->encInit.m_Memset 						= (unsigned long (*)(unsigned long *, int, unsigned int, unsigned int))memset;
		ptInit->encInit.m_Interrupt 					= (int (*)(void))NULL;
		ptInit->encInit.m_Usleep 						= (void (*)(unsigned int, unsigned int))NULL;
		ptInit->encInit.m_reg_read  					= (unsigned int (*)(unsigned long *base_addr, unsigned int offset))vpu_4ke1_reg_read;
		ptInit->encInit.m_reg_write 					= (void (*)(unsigned long *base_addr, unsigned int offset, unsigned int data))vpu_4ke1_reg_write;

		if ((ptDriverInfo->iInitialQp == 0) &&
			(ptDriverInfo->iMaxPQp == 0) && (ptDriverInfo->iMaxIQp == 0) &&
			(ptDriverInfo->iMinPQp == 0) && (ptDriverInfo->iMinIQp == 0)) {
			ptInit->encInit.m_iUseSpecificRcOption 		= 0;
			ptInit->encInit.m_stRcInit.m_Reserved[0] 	= 0U;
		} else {
			ptInit->encInit.m_iUseSpecificRcOption 		= 1;
			ptInit->encInit.m_stRcInit.m_Reserved[0] 	= 1U;

			if (ptDriverInfo->iInitialQp > 0) {
				ptInit->encInit.m_stRcInit.m_Reserved[0] |= 0x0100U;
				ptInit->encInit.m_stRcInit.m_Reserved[5] =  (unsigned int)ptDriverInfo->iInitialQp;
			}

			if (ptDriverInfo->iMaxIQp > 0) {
				ptInit->encInit.m_stRcInit.m_Reserved[0] |= 0x0020U;
				ptInit->encInit.m_stRcInit.m_Reserved[2] =  (unsigned int)ptDriverInfo->iMaxIQp;
			}
			if (ptDriverInfo->iMaxPQp > 0) {
				ptInit->encInit.m_stRcInit.m_Reserved[0] |= 0x0080U;
				ptInit->encInit.m_stRcInit.m_Reserved[4] =  (unsigned int)ptDriverInfo->iMaxPQp;
			}

			if (ptDriverInfo->iMinIQp > 0) {
				ptInit->encInit.m_stRcInit.m_Reserved[0] |= 0x0010U;
				ptInit->encInit.m_stRcInit.m_Reserved[1] = (unsigned int)ptDriverInfo->iInitialQp;
			}
			if (ptDriverInfo->iMinPQp > 0) {
				ptInit->encInit.m_stRcInit.m_Reserved[0] |= 0x0040U;
				ptInit->encInit.m_stRcInit.m_Reserved[3] =  (unsigned int)ptDriverInfo->iInitialQp;
			}
		}

		TEPV_DETAIL_MSG("[%s(%d)] =========================================== \n", __func__, __LINE__);
		TEPV_DETAIL_MSG("[%s(%d)] INIT INFORMATION: %d \n", __func__, __LINE__, ptDriverInfo->iInstIdx);
		TEPV_DETAIL_MSG("[%s(%d)] ------------------------------------- \n", __func__, __LINE__);
		TEPV_DETAIL_MSG("[%s(%d)]   - %d X %d @ %d(%d) \n", __func__, __LINE__, ptInit->encInit.m_iPicWidth, ptInit->encInit.m_iPicHeight, ptInit->encInit.m_iFrameRate, ptInit->encInit.m_iTargetKbps);
		TEPV_DETAIL_MSG("[%s(%d)] =========================================== \n", __func__, __LINE__);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4ke1_set_bitstream_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_HEVC_INIT_t *ptInit = &ptDriverInfo->tHEVC_INIT;

		ptInit->encInit.m_BitstreamBufferAddr[VEPK_PA] 	= vpu_enc_cast_void_uint(pucBuffer[VEPK_PA]);
		ptInit->encInit.m_BitstreamBufferAddr[VEPK_VA] 	= vpu_enc_cast_void_uintptr(pucBuffer[VEPK_K_VA]);
		ptInit->encInit.m_iBitstreamBufferSize			= iBufSize;

		if (iBufSize > 0) {
			(void)memset((void*)pucBuffer[VEPK_VA], 0x0, (size_t)iBufSize);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4ke1_set_bitwork_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_HEVC_INIT_t *ptInit = &ptDriverInfo->tHEVC_INIT;

		ptInit->encInit.m_BitWorkAddr[VEPK_PA]		= vpu_enc_cast_void_uintptr(pucBuffer[VEPK_PA]);
		ptInit->encInit.m_BitWorkAddr[VEPK_VA] 		= vpu_enc_cast_void_uintptr(pucBuffer[VEPK_K_VA]);
		(void)iBufSize;
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4ke1_set_frame_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_HEVC_SET_BUFFER_t *ptSetBuffer = &ptDriverInfo->tHEVC_SET_BUFFER;

		ptSetBuffer->encBuffer.m_FrameBufferStartAddr[VEPK_PA]	= vpu_enc_cast_void_uintptr(pucBuffer[VEPK_PA]);
		ptSetBuffer->encBuffer.m_FrameBufferStartAddr[VEPK_VA] 	= vpu_enc_cast_void_uintptr(pucBuffer[VEPK_K_VA]);
		(void)iBufSize;
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4ke1_set_seq_header_info(VEPC_DRIVER_INFO_T *ptDriverInfo, int iType, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_HEVC_PUT_HEADER_t *ptHeader = &ptDriverInfo->tHEVC_PUT_HEADER;

		ptHeader->encHeader.m_iHeaderType 			= iType;
		ptHeader->encHeader.m_iHeaderSize 			= iBufSize;
		ptHeader->encHeader.m_HeaderAddr[VEPK_PA] 	= vpu_enc_cast_void_uint(pucBuffer[VEPK_PA]);
		ptHeader->encHeader.m_HeaderAddr[VEPK_VA] 	= vpu_enc_cast_void_uint(pucBuffer[VEPK_VA]);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4ke1_get_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VEPK_HEVC_INIT_t *ptInit = &ptDriverInfo->tHEVC_INIT;

		ptDriverInfo->iMinFrameBufferCount 	= ptInit->encInitialInfo.m_iMinFrameBufferCount;
		ptDriverInfo->iMinFrameBufferSize 	= ptInit->encInitialInfo.m_iMinFrameBufferSize;

		TEPV_DETAIL_MSG("[%s(%d)] =========================================== \n", __func__, __LINE__);
		TEPV_DETAIL_MSG("[%s(%d)] INIT RESULT INFORMATION: %d \n", __func__, __LINE__, ptDriverInfo->iInstIdx);
		TEPV_DETAIL_MSG("[%s(%d)] ------------------------------------- \n", __func__, __LINE__);
		TEPV_DETAIL_MSG("[%s(%d)]   - FrameBuffer: %d, %d \n", __func__, __LINE__, ptDriverInfo->iMinFrameBufferCount, ptDriverInfo->iMinFrameBufferSize);
		TEPV_DETAIL_MSG("[%s(%d)] =========================================== \n", __func__, __LINE__);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4ke1_set_encode_info(VEPC_DRIVER_INFO_T *ptDriverInfo,  EP_Video_Encode_Input_T *ptEncodeInput)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_HEVC_ENCODE_t *ptEncode = &ptDriverInfo->tHEVC_ENCODE;

		int iResolution = ptDriverInfo->iWidth * ptDriverInfo->iHeight;

		if (ptDriverInfo->iBitstreamBufSize > 0) {
			(void)memset((void*)ptDriverInfo->pucBitstreamBuffer[VEPK_VA], 0x0, (size_t)ptDriverInfo->iBitstreamBufSize);
		}

		if (iResolution > 0) {
			//unsigned int uiTmpFlag = ((unsigned int)0x20 | (unsigned int)0x1);

			int iOffsetUV = iResolution / 4;
			int iMemSize = iResolution + (iOffsetUV * 2);

			if (ptEncodeInput->pucVideoBuffer[VEPK_COMP_U] == NULL) {
				(void)memcpy(ptDriverInfo->pucRawImageBuffer[VEPK_VA], (unsigned char *)ptEncodeInput->pucVideoBuffer[VEPK_COMP_Y], (size_t)iMemSize);
				ptEncode->encInput.m_PicYAddr 					= vpu_enc_cast_void_uint(ptDriverInfo->pucRawImageBuffer[VEPK_PA]);
				ptEncode->encInput.m_PicCbAddr					= ptEncode->encInput.m_PicYAddr + (unsigned int)iResolution;
			} else {
				ptEncode->encInput.m_PicYAddr 					= vpu_enc_cast_void_uint(ptEncodeInput->pucVideoBuffer[VEPK_COMP_Y]);
				ptEncode->encInput.m_PicCbAddr					= vpu_enc_cast_void_uint(ptEncodeInput->pucVideoBuffer[VEPK_COMP_U]);
			}

			if (ptEncodeInput->pucVideoBuffer[VEPK_COMP_V] == NULL) {
				ptEncode->encInput.m_PicCrAddr					= ptEncode->encInput.m_PicCbAddr + (unsigned int)iOffsetUV;
			} else {
				ptEncode->encInput.m_PicCrAddr					= vpu_enc_cast_void_uint(ptEncodeInput->pucVideoBuffer[VEPK_COMP_V]);
			}
		}

		ptEncode->encInput.m_iBitstreamBufferSize			= ptDriverInfo->iBitstreamBufSize;
		ptEncode->encInput.m_BitstreamBufferAddr[VEPK_PA]	= vpu_enc_cast_void_uint(ptDriverInfo->pucBitstreamBuffer[VEPK_PA]);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_4ke1_get_encode_result_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VEPK_HEVC_ENCODE_t *ptEncode = &ptDriverInfo->tHEVC_ENCODE;

		ptDriverInfo->iBitStreamOutSize 	= ptEncode->encOutput.m_iBitstreamOutSize;
		ptDriverInfo->pucBitStreamOutBuffer = ptDriverInfo->pucBitstreamBuffer[VEPK_VA];

		if (ptEncode->encOutput.m_iPicType == VEPK_PIC_TYPE_I) {
			ptDriverInfo->iTotalIFrameCount++;

			ptDriverInfo->eFrameType = VEPC_FRAME_TYPE_I;
		} else if (ptEncode->encOutput.m_iPicType == VEPK_PIC_TYPE_P) {
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

void vpu_4ke1_get_header_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VEPK_HEVC_PUT_HEADER_t *ptHeader = &ptDriverInfo->tHEVC_PUT_HEADER;
		unsigned char *pucDest = ptDriverInfo->pucSeqHeaderBufferOutVA;

		if (pucDest != NULL) {
			if ((ptDriverInfo->iSeqHeaderBufferOutSize > 0) && (ptDriverInfo->iSeqHeaderBufferOutSize < (INT32_MAX - 1))) {
				pucDest = ptDriverInfo->pucSeqHeaderBufferOutVA + ptDriverInfo->iSeqHeaderBufferOutSize;
			} else {
				pucDest = ptDriverInfo->pucSeqHeaderBufferOutVA;
			}

			if (ptHeader->encHeader.m_iHeaderSize > 0) {
				if (ptDriverInfo->bNalStartCode == true) {
					(void)memcpy(pucDest, ptDriverInfo->pucSeqHeaderBuffer[VEPK_VA], (size_t)ptHeader->encHeader.m_iHeaderSize);
				}

				(void)memcpy(pucDest, ptDriverInfo->pucSeqHeaderBuffer[VEPK_VA], (size_t)ptHeader->encHeader.m_iHeaderSize);
			}
		}

		if (((INT32_MAX / 2) > ptDriverInfo->iSeqHeaderBufferOutSize) && ((INT32_MAX / 2) > ptHeader->encHeader.m_iHeaderSize)) {
			ptDriverInfo->iSeqHeaderBufferOutSize += ptHeader->encHeader.m_iHeaderSize;
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}
#endif