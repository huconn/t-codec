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

#include <encoder_plugin.h>
#include <encoder_plugin_video.h>
#include <encoder_plugin_video_common.h>

#include <encoder_plugin_video_ips.h>


static unsigned char *video_encode_plugin_get_remap_addr(VEPC_DRIVER_INFO_T *ptDriverInfo);
static unsigned char *video_encode_plugin_get_phy_addr(const VEPC_DRIVER_INFO_T *ptDriverInfo);

static int video_encode_plugin_get_bitwork_buffer_size(const VEPC_DRIVER_INFO_T *ptDriverInfo);
static int video_encode_plugin_get_large_bitstream_buffer_size(const VEPC_DRIVER_INFO_T *ptDriverInfo);

static void video_encode_plugin_set_bitwork_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);
static void video_encode_plugin_set_bitstream_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);
static void video_encode_plugin_set_frame_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);
static void video_encode_plugin_set_seq_header_info(VEPC_DRIVER_INFO_T *ptDriverInfo, int iType, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);
static void video_encode_plugin_set_alloc_mem_info(VEPC_DRIVER_INFO_T *ptDriverInfo, int iReqSize, int iReqType);

static void video_encode_plugin_free_alloc_buffer(int iBufferSize, unsigned char *pucBuffer);
static int video_encode_plugin_align_buffer(int iBuf, int iMul);
static void *video_encode_plugin_mmap_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo, int iBufSize, unsigned char *pucBuffer[VEPK_K_VA + 1]);
static int video_encode_plugin_process_cmd_result(VEPC_DRIVER_INFO_T *ptDriverInfo, VEPC_DRIVER_TYPE_E eDriverType, VEPC_COMMAND_E eCmd, void *pvParam);


int video_encode_plugin_cmd(VEPC_DRIVER_INFO_T *ptDriverInfo, VEPC_DRIVER_TYPE_E eDriverType, VEPC_COMMAND_E eCmd, void *pvParam)
{
	int ret = 0;
	unsigned int uiCmd = 0;
	char strCmd[TEPV_CMD_STR_MAX] = {0, };

	TEPV_FUNC_LOG();

	if (vpu_enc_translate_cmd(eCmd, &uiCmd, strCmd) == 0) {
		int iDriverFd = vpu_enc_get_driverfd(ptDriverInfo, eDriverType);

		if (iDriverFd >= 0) {
			void *pvCmdParam = vpu_enc_get_cmd_param(ptDriverInfo, eCmd, pvParam);

			if (pvCmdParam != NULL) {
				ret = ioctl(iDriverFd, uiCmd, pvCmdParam);
			}
		} else {
			ret = -1;
		}

		if (ret != 0) {
			TEPV_ERROR_MSG("[%s(%d)] [ERROR] ioctl(%s): %d \n", __func__, __LINE__, strCmd, ret);
			ret = -1;
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] UNKNOWN COMMAND: %d \n", __func__, __LINE__, (int)eCmd);
	}

	return ret;
}


int video_encode_plugin_poll_cmd(VEPC_DRIVER_INFO_T *ptDriverInfo, VEPC_DRIVER_TYPE_E eDriverType, VEPC_COMMAND_E eCmd, void *pvParam)
{
	int iRetVal = 0;
	void *pvCmdParam = vpu_enc_get_cmd_param(ptDriverInfo, eCmd, pvParam);

	TEPV_FUNC_LOG();

	if (video_encode_plugin_cmd(ptDriverInfo, eDriverType, eCmd, pvCmdParam) < 0) {
		iRetVal = -1;
	} else {
		int iMaxPollTryCnt = 3;
		VEPC_DRIVER_POLL_ERROR_E eErrorType = (VEPC_DRIVER_POLL_ERROR_E)VEPC_DRIVER_POLL_ERROR_NONE;

		while (iMaxPollTryCnt > 0) {
			int iVal = 0;
			int iRetryCnt = 2;
			int iDriverFd = vpu_enc_get_driverfd(ptDriverInfo, eDriverType);

			iMaxPollTryCnt--;

			while (iRetryCnt > 0) {
				bool bIsLoopEnd = false;
				struct pollfd tPollFd[1];

				(void)memset(tPollFd, 0, sizeof(tPollFd));

				tPollFd[0].fd 		= iDriverFd;
				tPollFd[0].events 	= POLLIN;

				// polling one sec
				iVal = poll((struct pollfd *)&tPollFd, 1, 1000);

				if (iVal < 0) {
					iRetryCnt--;
					eErrorType = (VEPC_DRIVER_POLL_ERROR_E)VEPC_DRIVER_POLL_ERROR_RET;
					continue;
				} else if (iVal == 0) {
					iRetryCnt--;
					eErrorType = (VEPC_DRIVER_POLL_ERROR_E)VEPC_DRIVER_POLL_ERROR_TIMEOUT;
					continue;
				} else {
					if (tPollFd[0].revents >= 0) {
						if (((unsigned int)tPollFd[0].revents & (unsigned int)POLLERR) == (unsigned int)POLLERR) {
							bIsLoopEnd = true;
							eErrorType = (VEPC_DRIVER_POLL_ERROR_E)VEPC_DRIVER_POLL_ERROR_POLLERR;
						} else if (((unsigned int)tPollFd[0].revents & (unsigned int)POLLIN) == (unsigned int)POLLIN) {
							bIsLoopEnd = true;
							iMaxPollTryCnt = 0;
							eErrorType = (VEPC_DRIVER_POLL_ERROR_E)VEPC_DRIVER_POLL_ERROR_NONE;
						} else {
							eErrorType = (VEPC_DRIVER_POLL_ERROR_E)VEPC_DRIVER_POLL_ERROR_UNKNOWN;
						}
					} else {
						bIsLoopEnd = true;
					}
				}

				if (bIsLoopEnd == true) {
					break;
				}
			}

			iRetVal = video_encode_plugin_process_cmd_result(ptDriverInfo, eDriverType, eCmd, pvCmdParam);

			if (iRetVal >= 0) {
				if (((unsigned int)iRetVal & (unsigned int)0xf000) != (unsigned int)0x0000) {
					TEPV_ERROR_MSG("[%s(%d)] [ERROR] VEPK_RETCODE_CODEC_EXIT: %d, %d, %d \n", __func__, __LINE__, iRetVal, (int)eDriverType, (int)eCmd);
					iRetVal = VEPK_RETCODE_CODEC_EXIT;
				}
			}
		}

		if (eErrorType == (VEPC_DRIVER_POLL_ERROR_E)VEPC_DRIVER_POLL_ERROR_RET) {
			TEPV_ERROR_MSG("[%s(%d)] [ERROR] RET: poll(%d, %d) \n", __func__, __LINE__, (int)eDriverType, (int)eCmd);
		} else if (eErrorType == (VEPC_DRIVER_POLL_ERROR_E)VEPC_DRIVER_POLL_ERROR_POLLERR) {
			TEPV_ERROR_MSG("[%s(%d)] [ERROR] POLLERR: poll(%d, %d)  \n", __func__, __LINE__, (int)eDriverType, (int)eCmd);
		} else {
			TEPV_DONOTHING(0);
		}
	}

	return iRetVal;
}

void video_encode_plugin_set_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
			#if defined(TCC_VPU_4K_E1_INCLUDE)
			vpu_4ke1_set_init_info(ptDriverInfo);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			vpu_jenc_set_init_info(ptDriverInfo);
			#endif
		} else {
			vpu_venc_set_init_info(ptDriverInfo);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void video_encode_plugin_set_encode_info(VEPC_DRIVER_INFO_T *ptDriverInfo, EP_Video_Encode_Input_T *ptEncodeInput)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
			#if defined(TCC_VPU_4K_E1_INCLUDE)
			vpu_4ke1_set_encode_info(ptDriverInfo, ptEncodeInput);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			vpu_jenc_set_encode_info(ptDriverInfo, ptEncodeInput);
			#endif
		} else {
			vpu_venc_set_encode_info(ptDriverInfo, ptEncodeInput);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void video_encode_plugin_get_encode_result_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
			#if defined(TCC_VPU_4K_E1_INCLUDE)
			vpu_4ke1_get_encode_result_info(ptDriverInfo);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			vpu_jenc_get_encode_result_info(ptDriverInfo);
			#endif
		} else {
			vpu_venc_upate_get_result_info(ptDriverInfo);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void video_encode_plugin_get_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
			#if defined(TCC_VPU_4K_E1_INCLUDE)
			vpu_4ke1_get_init_info(ptDriverInfo);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			vpu_jenc_get_init_info(ptDriverInfo);
			#endif
		} else {
			vpu_venc_get_init_info(ptDriverInfo);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

EP_Video_Error_E video_encode_plugin_open_drivers(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	EP_Video_Error_E ret = (EP_Video_Error_E)EP_Video_ErrorNone;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		ret = vpu_enc_open_mgr(ptDriverInfo);

		if (ret == (EP_Video_Error_E)EP_Video_ErrorNone) {
			ret = vpu_enc_open_vpu(ptDriverInfo);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
		ret = (EP_Video_Error_E)EP_Video_ErrorInit;
	}

	return ret;
}

EP_Video_Error_E video_encode_plugin_close_drivers(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	EP_Video_Error_E ret = (EP_Video_Error_E)EP_Video_ErrorNone;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		(void)vpu_enc_close_mgr(ptDriverInfo);
		(void)vpu_enc_close_vpu(ptDriverInfo);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
		ret = (EP_Video_Error_E)EP_Video_ErrorInit;
	}

	return ret;
}

int video_encode_plugin_alloc_bitstream_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iMemSize = -1;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		iMemSize = video_encode_plugin_get_large_bitstream_buffer_size(ptDriverInfo);
		iMemSize = video_encode_plugin_align_buffer(iMemSize, TEPV_ALIGN_LEN);

		if (iMemSize > 0) {
			int iTmpVal = -1;

			video_encode_plugin_set_alloc_mem_info(ptDriverInfo, iMemSize, (int)VEPK_BUFFER_ELSE);
			iTmpVal = video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_ALLOC_MEMORY, NULL);

			if (iTmpVal >= 0) {
				const void *ptTmpMMapResult = video_encode_plugin_mmap_buffer(ptDriverInfo, iMemSize, ptDriverInfo->pucBitstreamBuffer);

				if (ptTmpMMapResult == NULL) {
					TEPV_ERROR_MSG("[%s(%d)] [ERROR] mmap() \n", __func__, __LINE__);
					ptDriverInfo->pucBitstreamBuffer[VEPK_VA] = NULL;
					(void)video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_FREE_MEMORY, NULL);

					iMemSize = -1;
				} else {
					video_encode_plugin_set_bitstream_buffer_info(ptDriverInfo, ptDriverInfo->pucBitstreamBuffer, iMemSize);
					ptDriverInfo->iBitstreamBufSize = iMemSize;
				}
			}
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	TEPV_DETAIL_MSG("[%s(%d)] \n\tpucBitstreamBuffer(%d): %p, %p, %p \n", __func__, __LINE__,
						iMemSize, ptDriverInfo->pucBitstreamBuffer[VEPK_PA], ptDriverInfo->pucBitstreamBuffer[VEPK_VA], ptDriverInfo->pucBitstreamBuffer[VEPK_K_VA]);

	return iMemSize;
}

int video_encode_plugin_alloc_bitwork_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iMemSize = -1;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		iMemSize = video_encode_plugin_get_bitwork_buffer_size(ptDriverInfo);
		iMemSize = video_encode_plugin_align_buffer(iMemSize, TEPV_ALIGN_LEN);

		if (iMemSize > 0) {
			int iTmpVal = -1;

			video_encode_plugin_set_alloc_mem_info(ptDriverInfo, iMemSize, (int)VEPK_BUFFER_WORK);
			iTmpVal = video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_ALLOC_MEMORY, NULL);

			if (iTmpVal >= 0) {
				const void *ptTmpMMapResult = video_encode_plugin_mmap_buffer(ptDriverInfo, iMemSize, ptDriverInfo->pucBitWorkBuffer);

				if (ptTmpMMapResult == NULL) {
					TEPV_ERROR_MSG("[%s(%d)] [ERROR] mmap() \n", __func__, __LINE__);
					ptDriverInfo->pucBitWorkBuffer[VEPK_VA] = NULL;
					(void)video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_FREE_MEMORY, NULL);

					iMemSize = -1;
				} else {
					video_encode_plugin_set_bitwork_buffer_info(ptDriverInfo, ptDriverInfo->pucBitWorkBuffer, iMemSize);
					ptDriverInfo->iBitWorkBufSize = iMemSize;
				}
			}
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	TEPV_DETAIL_MSG("[%s(%d)] \n\tpucBitWorkBuffer(%d): : %p, %p, %p \n", __func__, __LINE__,
						iMemSize, ptDriverInfo->pucBitWorkBuffer[VEPK_PA], ptDriverInfo->pucBitWorkBuffer[VEPK_VA], ptDriverInfo->pucBitWorkBuffer[VEPK_K_VA]);

	return iMemSize;
}

int video_encode_plugin_alloc_me_search_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iMemSize = -1;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		unsigned int uiTmpWidth = 0U;
		unsigned int uiTmpBufSize = 0U;

		if (ptDriverInfo->iWidth > 0) {
			uiTmpWidth = (unsigned int)ptDriverInfo->iWidth;
			uiTmpBufSize = (((uiTmpWidth + 15U) & ~15U) * 36U) + 2048U;

			if (uiTmpBufSize < (UINT32_MAX / 2U)) {
				iMemSize = (int)uiTmpBufSize;
				iMemSize = video_encode_plugin_align_buffer(iMemSize, TEPV_ALIGN_LEN);
			}
		}

		if (iMemSize > 0) {
			int iTmpVal = -1;

			video_encode_plugin_set_alloc_mem_info(ptDriverInfo, iMemSize, (int)VEPK_BUFFER_ELSE);
			iTmpVal = video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_ALLOC_MEMORY, NULL);

			if (iTmpVal >= 0) {
				const void *ptTmpMMapResult = video_encode_plugin_mmap_buffer(ptDriverInfo, iMemSize, ptDriverInfo->pucMESearchBuffer);

				if (ptTmpMMapResult == NULL) {
					TEPV_ERROR_MSG("[%s(%d)] [ERROR] mmap() \n", __func__, __LINE__);
					ptDriverInfo->pucMESearchBuffer[VEPK_VA] = NULL;
					(void)video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_FREE_MEMORY, NULL);

					iMemSize = -1;
				} else {
					ptDriverInfo->iMESearchBufSize = iMemSize;
				}
			}
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	TEPV_DETAIL_MSG("[%s(%d)] \n\tpucMESearchBuffer(%d): %p, %p, %p \n", __func__, __LINE__,
						iMemSize, ptDriverInfo->pucMESearchBuffer[VEPK_PA], ptDriverInfo->pucMESearchBuffer[VEPK_VA], ptDriverInfo->pucMESearchBuffer[VEPK_K_VA]);

	return iMemSize;
}

int video_encode_plugin_alloc_slice_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iMemSize = -1;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		unsigned int uiTmpWidth = 0U;
		unsigned int uiTmpHeight = 0U;
		unsigned int uiTmpBufSize = 0U;

		if ((ptDriverInfo->iWidth >= 0) && (ptDriverInfo->iHeight >= 0)) {
			uiTmpWidth = (unsigned int)ptDriverInfo->iWidth;
			uiTmpHeight = (unsigned int)ptDriverInfo->iHeight;

			uiTmpWidth = (uiTmpWidth + 15U) >> 4U;
			uiTmpHeight = (uiTmpHeight + 15U) >> 4U;

			if ((uiTmpWidth < (unsigned int)TEPV_WIDTH_LIMIT) && (uiTmpHeight < (unsigned int)TEPV_HEIGHT_LIMIT)) {
				uiTmpBufSize = ((uiTmpWidth * uiTmpHeight) * 8U) + 48U;
			}
		}

		if (uiTmpBufSize < (UINT32_MAX / 2U)) {
			iMemSize = (int)uiTmpBufSize;
			iMemSize = video_encode_plugin_align_buffer(iMemSize, TEPV_ALIGN_LEN);
		}

		if (iMemSize > 0) {
			int iTmpVal = -1;

			video_encode_plugin_set_alloc_mem_info(ptDriverInfo, iMemSize, (int)VEPK_BUFFER_ELSE);
			iTmpVal = video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_ALLOC_MEMORY, NULL);

			if (iTmpVal >= 0) {
				const void *ptTmpMMapResult = video_encode_plugin_mmap_buffer(ptDriverInfo, iMemSize, ptDriverInfo->pucSliceBuffer);

				if (ptTmpMMapResult == NULL) {
					TEPV_ERROR_MSG("[%s(%d)] [ERROR] mmap() \n", __func__, __LINE__);
					ptDriverInfo->pucSliceBuffer[VEPK_VA] = NULL;
					(void)video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_FREE_MEMORY, NULL);

					iMemSize = -1;
				} else {
					ptDriverInfo->iSliceBufSize = iMemSize;
				}
			}
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	TEPV_DETAIL_MSG("[%s(%d)] \n\tpucSliceBuffer(%d): %p, %p, %p \n", __func__, __LINE__,
						iMemSize, ptDriverInfo->pucSliceBuffer[VEPK_PA], ptDriverInfo->pucSliceBuffer[VEPK_VA], ptDriverInfo->pucSliceBuffer[VEPK_K_VA]);

	return iMemSize;
}

int video_encode_plugin_alloc_raw_image_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iMemSize = -1;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		unsigned int uiTmpWidth = 0U;
		unsigned int uiTmpHeight = 0U;
		unsigned int uiTmpBufSize = 0U;

		if ((ptDriverInfo->iWidth >= 0) && (ptDriverInfo->iHeight >= 0)) {
			uiTmpWidth = (unsigned int)ptDriverInfo->iWidth;
			uiTmpHeight = (unsigned int)ptDriverInfo->iHeight;

			uiTmpWidth = (uiTmpWidth + 15U) >> 4U << 4U;
			uiTmpHeight = (uiTmpHeight + 15U) >> 4U << 4U;

			if ((uiTmpWidth < (unsigned int)TEPV_WIDTH_LIMIT) && (uiTmpHeight < (unsigned int)TEPV_HEIGHT_LIMIT)) {
				uiTmpBufSize = ((uiTmpWidth * uiTmpHeight) * 3U) / 2U;
			}
		}

		if (uiTmpBufSize < (UINT32_MAX / 2U)) {
			iMemSize = (int)uiTmpBufSize;
			iMemSize = video_encode_plugin_align_buffer(iMemSize, TEPV_ALIGN_LEN);
		}

		if (iMemSize > 0) {
			int iTmpVal = -1;

			video_encode_plugin_set_alloc_mem_info(ptDriverInfo, iMemSize, (int)VEPK_BUFFER_ELSE);
			iTmpVal = video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_ALLOC_MEMORY, NULL);

			if (iTmpVal >= 0) {
				const void *ptTmpMMapResult = video_encode_plugin_mmap_buffer(ptDriverInfo, iMemSize, ptDriverInfo->pucRawImageBuffer);

				if (ptTmpMMapResult == NULL) {
					TEPV_ERROR_MSG("[%s(%d)] [ERROR] mmap() \n", __func__, __LINE__);
					ptDriverInfo->pucRawImageBuffer[VEPK_VA] = NULL;
					(void)video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_FREE_MEMORY, NULL);

					iMemSize = -1;
				} else {
					ptDriverInfo->iRawImageBufSize = iMemSize;
				}
			}
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	TEPV_DETAIL_MSG("[%s(%d)] \n\tpucRawImageBuffer(%d): %p, %p, %p \n", __func__, __LINE__,
						iMemSize, ptDriverInfo->pucRawImageBuffer[VEPK_PA], ptDriverInfo->pucRawImageBuffer[VEPK_VA], ptDriverInfo->pucRawImageBuffer[VEPK_K_VA]);

	return iMemSize;
}

int video_encode_plugin_alloc_frame_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iMemSize = -1;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		iMemSize = ptDriverInfo->iMinFrameBufferSize * ptDriverInfo->iMinFrameBufferCount;
		iMemSize = video_encode_plugin_align_buffer(iMemSize, TEPV_ALIGN_LEN);

		if (iMemSize > 0) {
			int iTmpVal = -1;

			video_encode_plugin_set_alloc_mem_info(ptDriverInfo, iMemSize, (int)VEPK_BUFFER_ELSE);
			iTmpVal = video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_ALLOC_MEMORY, NULL);

			if (iTmpVal >= 0) {
				const void *ptTmpMMapResult = video_encode_plugin_mmap_buffer(ptDriverInfo, iMemSize, ptDriverInfo->pucFrameBuffer);

				if (ptTmpMMapResult == NULL) {
					TEPV_ERROR_MSG("[%s(%d)] [ERROR] mmap() \n", __func__, __LINE__);
					ptDriverInfo->pucFrameBuffer[VEPK_VA] = NULL;
					(void)video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_FREE_MEMORY, NULL);

					iMemSize = -1;
				} else {
					int ret = -1;
					video_encode_plugin_set_frame_buffer_info(ptDriverInfo, ptDriverInfo->pucFrameBuffer, iMemSize);
					ret = video_encode_plugin_poll_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_REG_FRAME_BUFFER, NULL);
					ptDriverInfo->iFrameBufferSize = iMemSize;
				}
			}
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	TEPV_DETAIL_MSG("[%s(%d)] \n\tpucFrameBuffer(%d): %p, %p, %p \n", __func__, __LINE__,
						iMemSize, ptDriverInfo->pucFrameBuffer[VEPK_PA], ptDriverInfo->pucFrameBuffer[VEPK_VA], ptDriverInfo->pucFrameBuffer[VEPK_K_VA]);

	return iMemSize;
}

int video_encode_plugin_alloc_seq_header_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iMemSize = -1;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		iMemSize = video_encode_plugin_align_buffer(TEPV_SEQ_HEADER_LEN, TEPV_ALIGN_LEN);

		if (iMemSize > 0) {
			int iTmpVal = -1;

			video_encode_plugin_set_alloc_mem_info(ptDriverInfo, iMemSize, (int)VEPK_BUFFER_SEQHEADER);
			iTmpVal = video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_ALLOC_MEMORY, NULL);

			if (iTmpVal >= 0) {
				const void *ptTmpMMapResult = video_encode_plugin_mmap_buffer(ptDriverInfo, iMemSize, ptDriverInfo->pucSeqHeaderBuffer);

				if (ptTmpMMapResult == NULL) {
					TEPV_ERROR_MSG("[%s(%d)] [ERROR] mmap() \n", __func__, __LINE__);
					ptDriverInfo->pucSeqHeaderBuffer[VEPK_VA] = NULL;
					(void)video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_FREE_MEMORY, NULL);

					iMemSize = -1;
				} else {
					void *pvTmpBuffer = NULL;

					pvTmpBuffer = TEPV_MALLOC((size_t)iMemSize);

					ptDriverInfo->iSeqHeaderBufferSize = iMemSize;
					TEPV_CAST(ptDriverInfo->pucSeqHeaderBufferOutVA, pvTmpBuffer);
				}
			}
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	TEPV_DETAIL_MSG("[%s(%d)] \n\tpucSeqHeaderBuffer(%d): %p, %p, %p \n", __func__, __LINE__,
						iMemSize, ptDriverInfo->pucSeqHeaderBuffer[VEPK_PA], ptDriverInfo->pucSeqHeaderBuffer[VEPK_VA], ptDriverInfo->pucSeqHeaderBuffer[VEPK_K_VA]);

	return iMemSize;
}

int video_encode_plugin_process_seq_header(VEPC_DRIVER_INFO_T *ptDriverInfo, int iType)
{
	int ret = -1;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		video_encode_plugin_set_seq_header_info(ptDriverInfo, iType, ptDriverInfo->pucSeqHeaderBuffer, ptDriverInfo->iSeqHeaderBufferSize);
		ret = video_encode_plugin_poll_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_PUT_HEADER, NULL);

		video_encode_plugin_get_header_info(ptDriverInfo);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return ret;
}

void video_encode_plugin_get_header_info(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
			#if defined(TCC_VPU_4K_E1_INCLUDE)
			vpu_4ke1_get_header_info(ptDriverInfo);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			;
			#endif
		} else {
			vpu_venc_get_header_info(ptDriverInfo);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

void video_encode_plugin_free_alloc_buffers(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		video_encode_plugin_free_alloc_buffer(ptDriverInfo->iBitstreamBufSize, ptDriverInfo->pucBitstreamBuffer[VEPK_VA]);
		video_encode_plugin_free_alloc_buffer(ptDriverInfo->iRawImageBufSize, ptDriverInfo->pucRawImageBuffer[VEPK_VA]);

		video_encode_plugin_free_alloc_buffer(ptDriverInfo->iFrameBufferSize, ptDriverInfo->pucFrameBuffer[VEPK_VA]);
		video_encode_plugin_free_alloc_buffer(ptDriverInfo->iSeqHeaderBufferSize, ptDriverInfo->pucSeqHeaderBuffer[VEPK_VA]);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static void video_encode_plugin_free_alloc_buffer(int iBufferSize, unsigned char *pucBuffer)
{
	TEPV_FUNC_LOG();

	if (pucBuffer != NULL) {
		if (iBufferSize > 0) {
			(void)munmap(pucBuffer, (size_t)iBufferSize);
		}
		pucBuffer = NULL;
	}
}

static int video_encode_plugin_process_cmd_result(VEPC_DRIVER_INFO_T *ptDriverInfo, VEPC_DRIVER_TYPE_E eDriverType, VEPC_COMMAND_E eCmd, void *pvParam)
{
	int iRetVal = 0;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		switch ((int)eCmd) {
			case (int)VEPC_COMMAND_V_ENC_INIT: {
				TEPV_CHECK_STR_LOG("VEPC_COMMAND_V_ENC_INIT");
				(void)video_encode_plugin_cmd(ptDriverInfo, eDriverType, VEPC_COMMAND_V_ENC_INIT_RESULT, pvParam);
				iRetVal = vpu_enc_get_cmd_result(ptDriverInfo, eDriverType, eCmd, pvParam);
				break;
			}

			case (int)VEPC_COMMAND_V_ENC_PUT_HEADER: {
				TEPV_CHECK_STR_LOG("VEPC_COMMAND_V_ENC_PUT_HEADER");
				(void)video_encode_plugin_cmd(ptDriverInfo, eDriverType, VEPC_COMMAND_V_ENC_PUT_HEADER_RESULT, pvParam);
				iRetVal = vpu_enc_get_cmd_result(ptDriverInfo, eDriverType, eCmd, pvParam);
				break;
			}

			case (int)VEPC_COMMAND_V_ENC_ENCODE: {
				TEPV_CHECK_STR_LOG("VEPC_COMMAND_V_ENC_ENCODE");
				(void)video_encode_plugin_cmd(ptDriverInfo, eDriverType, VEPC_COMMAND_V_ENC_ENCODE_RESULT, pvParam);
				iRetVal = vpu_enc_get_cmd_result(ptDriverInfo, eDriverType, eCmd, pvParam);
				break;
			}

			case (int)VEPC_COMMAND_V_ENC_REG_FRAME_BUFFER:
			case (int)VEPC_COMMAND_V_ENC_CLOSE:
			default: {
				TEPV_CHECK_STR_LOG("default");
				int iVal = 0;
				(void)video_encode_plugin_cmd(ptDriverInfo, eDriverType, VEPC_COMMAND_V_ENC_GENERAL_RESULT,  &iVal);
				iRetVal = iVal;
				break;
			}
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return iRetVal;
}

static int video_encode_plugin_get_large_bitstream_buffer_size(const VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iRetVal = -1;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
			#if defined(TCC_VPU_4K_E1_INCLUDE)
			if ((ptDriverInfo->iWidth > 1920) && (ptDriverInfo->iHeight > 1080)) {
				//iRetVal = VEPK_LARGE_STREAM_BUF_SIZE * 4;
				iRetVal = VEPK_LARGE_STREAM_BUF_SIZE * 3;
			} else {
				iRetVal = VEPK_LARGE_STREAM_BUF_SIZE;
			}
			#endif
		} else {
			iRetVal = VEPK_LARGE_STREAM_BUF_SIZE;
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return iRetVal;
}

static int video_encode_plugin_get_bitwork_buffer_size(const VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iRetVal = -1;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
			#if defined(TCC_VPU_4K_E1_INCLUDE)
			iRetVal = VEPK_VPU_HEVC_ENC_WORK_CODE_BUF_SIZE;
			#endif
		} else {
			iRetVal = VEPK_WORK_CODE_PARA_BUF_SIZE;
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return iRetVal;
}

static int video_encode_plugin_align_buffer(int iBuf, int iMul)
{
	int iRetVal = 0;

	unsigned int uiTmpBuf = 0U;
	unsigned int uiTmpMul = 0U;
	unsigned int uiTmpRet = 0U;

	TEPV_FUNC_LOG();

	if ((iBuf >= 0) && (iMul > 0)) {
		uiTmpBuf = (unsigned int)iBuf;
		uiTmpMul = (unsigned int)iMul;

		uiTmpRet = ((uiTmpBuf + (uiTmpMul - 1U)) & ~(uiTmpMul - 1U));
	}

	iRetVal = (int)uiTmpRet;

	return iRetVal;
}

static void video_encode_plugin_set_alloc_mem_info(VEPC_DRIVER_INFO_T *ptDriverInfo, int iReqSize, int iReqType)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_MEM_ALLOC_INFO_t *tALLOC_MEM = &ptDriverInfo->tALLOC_MEM;

		(void)memset(tALLOC_MEM, 0x00, sizeof(VEPK_MEM_ALLOC_INFO_t));

		if ((iReqSize >= 0) &&
				((iReqType >= (int)VEPK_BUFFER_TYPE_MIN) && (iReqType < (int)VEPK_BUFFER_TYPE_MAX))) {
			tALLOC_MEM->request_size 	= (unsigned int)iReqSize;
			tALLOC_MEM->buffer_type 	= (VEPK_BUFFER_TYPE)iReqType;
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static void *video_encode_plugin_mmap_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo, int iBufSize, unsigned char *pucBuffer[VEPK_K_VA + 1])
{
	void *ptRetPtr = NULL;

	unsigned int uiMapProt = (unsigned int)PROT_READ | (unsigned int)PROT_WRITE;
	size_t tTmpSize = 0UL;

	TEPV_FUNC_LOG();

	if (iBufSize > 0) {
		tTmpSize = (size_t)iBufSize;
	}

	pucBuffer[VEPK_PA] 		= video_encode_plugin_get_phy_addr(ptDriverInfo);
	pucBuffer[VEPK_K_VA] 	= video_encode_plugin_get_remap_addr(ptDriverInfo);

	ptRetPtr = (void *)mmap(NULL, tTmpSize, (int)uiMapProt, MAP_SHARED, ptDriverInfo->tDriverFD.iVpuFd, (off_t)pucBuffer[VEPK_PA]);

	if (ptRetPtr == NULL) {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] mmap \n", __func__, __LINE__);

		if (pucBuffer[VEPK_VA] != NULL) {
			if (iBufSize > 0) {
				(void)munmap(pucBuffer[VEPK_VA], (size_t)iBufSize);
			}
			pucBuffer[VEPK_VA] = NULL;
		}

		ptRetPtr = NULL;
	} else {
		TEPV_CAST(pucBuffer[VEPK_VA], ptRetPtr);
	}

	return ptRetPtr;
}

static unsigned char *video_encode_plugin_get_phy_addr(const VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	unsigned char *pucRetVal = NULL;
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VEPK_MEM_ALLOC_INFO_t *ptAllocMem = &ptDriverInfo->tALLOC_MEM;

		uintptr_t tmpPtr = ptAllocMem->phy_addr;
		TEPV_CAST(pucRetVal, tmpPtr);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return pucRetVal;
}

static unsigned char *video_encode_plugin_get_remap_addr(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	unsigned char *pucRetVal = NULL;
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VEPK_MEM_ALLOC_INFO_t *ptAllocMem = &ptDriverInfo->tALLOC_MEM;

		TEPV_CAST(pucRetVal, ptAllocMem->kernel_remap_addr);
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return pucRetVal;
}

static void video_encode_plugin_set_bitstream_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
			#if defined(TCC_VPU_4K_E1_INCLUDE)
			vpu_4ke1_set_bitstream_buffer_info(ptDriverInfo, pucBuffer, iBufSize);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			vpu_jenc_set_bitstream_buffer_info(ptDriverInfo, pucBuffer, iBufSize);
			#endif
		} else {
			vpu_venc_set_bitstream_buffer_info(ptDriverInfo, pucBuffer, iBufSize);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static void video_encode_plugin_set_bitwork_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
			#if defined(TCC_VPU_4K_E1_INCLUDE)
			vpu_4ke1_set_bitwork_buffer_info(ptDriverInfo, pucBuffer, iBufSize);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			;
			#endif
		} else {
			vpu_venc_set_bitwork_buffer_info(ptDriverInfo, pucBuffer, iBufSize);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static void video_encode_plugin_set_frame_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
			#if defined(TCC_VPU_4K_E1_INCLUDE)
			vpu_4ke1_set_frame_buffer_info(ptDriverInfo, pucBuffer, iBufSize);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			;
			#endif
		} else {
			vpu_venc_set_frame_buffer_info(ptDriverInfo, pucBuffer, iBufSize);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static void video_encode_plugin_set_seq_header_info(VEPC_DRIVER_INFO_T *ptDriverInfo, int iType, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize)
{
	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
			#if defined(TCC_VPU_4K_E1_INCLUDE)
			vpu_4ke1_set_seq_header_info(ptDriverInfo, iType, pucBuffer, iBufSize);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			;
			#endif
		} else {
			vpu_venc_set_seq_header_info(ptDriverInfo, iType, pucBuffer, iBufSize);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}