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


#if defined(TCC_JPU_C6_INCLUDE)
void vpu_jenc_set_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_JENC_INIT_t *ptInit = &ptDriverInfo->tJENC_INIT;

		ptInit->gsJpuEncInit.m_RegBaseVirtualAddr 		= 0U;
		ptInit->gsJpuEncInit.m_iPicWidth 				= ptDriverInfo->iWidth;
		ptInit->gsJpuEncInit.m_iPicHeight 				= ptDriverInfo->iHeight;

		ptInit->gsJpuEncInit.m_iSourceFormat 			= 0;
		ptInit->gsJpuEncInit.m_iEncQuality 				= 3;
		ptInit->gsJpuEncInit.m_iCbCrInterleaveMode 		= 0;
		ptInit->gsJpuEncInit.m_uiEncOptFlags 			= 0;

		ptInit->gsJpuEncInit.m_Memcpy 					= (int (*)(void))NULL;
		ptInit->gsJpuEncInit.m_Memset 					= (int (*)(void))NULL;
		ptInit->gsJpuEncInit.m_Interrupt 				= (int (*)(void))NULL;


		TEPV_DETAIL_MSG("[%s(%d)] =========================================== \n", __func__, __LINE__);
		TEPV_DETAIL_MSG("[%s(%d)] INIT INFORMATION: %d \n", __func__, __LINE__, ptDriverInfo->iInstIdx);
		TEPV_DETAIL_MSG("[%s(%d)] ------------------------------------- \n", __func__, __LINE__);
		TEPV_DETAIL_MSG("[%s(%d)]   - %d X %d \n", __func__, __LINE__, ptInit->gsJpuEncInit.m_iPicWidth, ptInit->gsJpuEncInit.m_iPicHeight);
		TEPV_DETAIL_MSG("[%s(%d)] =========================================== \n", __func__, __LINE__);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_jenc_get_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		#if 0
		VEPK_JENC_INIT_t *ptInit = &ptDriverInfo->tJENC_INIT;
		#endif
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_jenc_set_encode_info(VEPC_DRIVER_INFO_T *ptDriverInfo, EP_Video_Encode_Input_T *ptEncodeInput)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_JPU_ENCODE_t *ptEncode = &ptDriverInfo->tJENC_ENCODE;

		int iResolution = ptDriverInfo->iWidth * ptDriverInfo->iHeight;

		if (iResolution > 0) {
			int iOffsetUV = iResolution / 4;
			int iMemSize = iResolution + (iOffsetUV * 2);

			if (ptEncodeInput->pucVideoBuffer[VEPK_COMP_U] == NULL) {
				(void)memcpy(ptDriverInfo->pucRawImageBuffer[VEPK_VA], (unsigned char *)ptEncodeInput->pucVideoBuffer[VEPK_COMP_Y], (size_t)iMemSize);
				ptEncode->gsJpuEncInput.m_PicYAddr 		= vpu_enc_cast_void_uint(ptDriverInfo->pucRawImageBuffer[VEPK_PA]);
				ptEncode->gsJpuEncInput.m_PicCbAddr		= ptEncode->gsJpuEncInput.m_PicYAddr + (unsigned int)iResolution;
			} else {
				ptEncode->gsJpuEncInput.m_PicYAddr 		= vpu_enc_cast_void_uint(ptEncodeInput->pucVideoBuffer[VEPK_COMP_Y]);
				ptEncode->gsJpuEncInput.m_PicCbAddr		= vpu_enc_cast_void_uint(ptEncodeInput->pucVideoBuffer[VEPK_COMP_U]);
			}

			if (iOffsetUV > 0) {
				if (ptEncodeInput->pucVideoBuffer[VEPK_COMP_V] == NULL) {
					ptEncode->gsJpuEncInput.m_PicCrAddr		= ptEncode->gsJpuEncInput.m_PicCbAddr + (unsigned int)iOffsetUV;
				} else {
					ptEncode->gsJpuEncInput.m_PicCrAddr		= vpu_enc_cast_void_uint(ptEncodeInput->pucVideoBuffer[VEPK_COMP_V]);
				}
			}
		}

		#if (VENC_DUMP_ON == 1)
		VEP_DUMP_DISP_INFO_T tDump = {0, };

		tDump.iWidth 	= ptDriverInfo->iWidth;
		tDump.iHeight 	= ptDriverInfo->iHeight;
		tDump.pucY 		= ptEncode->gsJpuEncInput.m_PicYAddr;
		tDump.pucU 		= ptEncode->gsJpuEncInput.m_PicCbAddr;
		tDump.pucV 		= ptEncode->gsJpuEncInput.m_PicCrAddr;
		tDump.bInterleaveMode 	= true;
		tDump.iDumpMax 	= 10;
		//vpu_enc_dump_result_frame(&tDump);
		//vpu_enc_dump_data(ptDriverInfo->iWidth, ptDriverInfo->iHeight, iMemSize, (unsigned char *)ptEncode->gsJpuEncInput.m_PicYAddr);
		vpu_enc_dump_data(ptDriverInfo->iWidth, ptDriverInfo->iHeight, iMemSize, ptDriverInfo->pucRawImageBuffer[VEPK_VA]);
		#endif

		ptEncode->gsJpuEncInput.m_iBitstreamBufferSize			= ptDriverInfo->iBitstreamBufSize;
		ptEncode->gsJpuEncInput.m_BitstreamBufferAddr[VEPK_PA]	= vpu_enc_cast_void_uint(ptDriverInfo->pucBitstreamBuffer[VEPK_PA]);
		ptEncode->gsJpuEncInput.m_BitstreamBufferAddr[VEPK_VA]	= vpu_enc_cast_void_uint(ptDriverInfo->pucBitstreamBuffer[VEPK_K_VA]);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_jenc_set_bitstream_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_JENC_INIT_t *ptInit = &ptDriverInfo->tJENC_INIT;

		ptInit->gsJpuEncInit.m_BitstreamBufferAddr[VEPK_PA] 	= vpu_enc_cast_void_uint(pucBuffer[VEPK_PA]);
		ptInit->gsJpuEncInit.m_BitstreamBufferAddr[VEPK_VA] 	= vpu_enc_cast_void_uintptr(pucBuffer[VEPK_K_VA]);
		ptInit->gsJpuEncInit.m_iBitstreamBufferSize				= iBufSize;

		if (iBufSize > 0) {
			(void)memset((void*)pucBuffer[VEPK_VA], 0x0, (size_t)iBufSize);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void vpu_jenc_get_encode_result_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VEPK_JPU_ENCODE_t *ptEncode = &ptDriverInfo->tJENC_ENCODE;

		if (ptEncode->gsJpuEncOutput.m_iBitstreamOutSize > 0) {
			ptDriverInfo->iBitStreamOutSize 	= ptEncode->gsJpuEncOutput.m_iBitstreamOutSize;
			ptDriverInfo->pucBitStreamOutBuffer = vpu_enc_get_virtaddr(ptDriverInfo->pucBitstreamBuffer, ptEncode->gsJpuEncOutput.m_BitstreamOut[VEPK_PA], VEPK_PA);
			//ptDriverInfo->pucBitStreamOutBuffer = ptDriverInfo->pucBitstreamBuffer[VEPK_VA];

			ptDriverInfo->iTotalFrameCount++;
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}
#endif