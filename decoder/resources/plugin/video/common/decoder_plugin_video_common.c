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


#include <decoder_plugin.h>
#include <decoder_plugin_video.h>
#include <decoder_plugin_video_common.h>

#include <decoder_plugin_video_ips.h>
#include <decoder_plugin_video_frame.h>


static void video_plugin_set_stream_size(VPC_DRIVER_INFO_T *ptDriverInfo, int iStreamLen);
static void video_plugin_set_frame_buffer_start_addr_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucFrameBuffer[VPK_K_VA + 1], int iFrameBufferSize);
static void video_plugin_set_slice_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSliceBuffer, int iSliceBufSize, int iExtraFrameCount);
static int video_plugin_get_bitwork_buffer_size(const VPC_DRIVER_INFO_T *ptDriverInfo);
static void video_plugin_set_bitwork_addr(VPC_DRIVER_INFO_T *ptDriverInfo);
static void video_plugin_set_spspps_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSpsPpsBuffer, int iSpsPpsSize);
static void video_plugin_set_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1], int iBitstreamBufSize);
static void video_plugin_free_alloc_buffer(int iBufferSize, unsigned char *pucBuffer);
static unsigned char *video_plugin_get_remap_addr(VPC_DRIVER_INFO_T *ptDriverInfo);
static unsigned char *video_plugin_get_phy_addr(const VPC_DRIVER_INFO_T *ptDriverInfo);
static void *video_plugin_mmap_bitstream_buffer(VPC_DRIVER_INFO_T *ptDriverInfo, int iBitstreamBufSize, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1]);
static void video_plugin_set_alloc_mem_info(VPC_DRIVER_INFO_T *ptDriverInfo, int iReqSize, int iReqType);
static int video_plugin_align_buffer(int iBuf, int iMul);
static int video_plugin_get_large_bitstream_buffer_size(const VPC_DRIVER_INFO_T *ptDriverInfo);
static void video_plugin_set_seq_header_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, int iBitstreamBufSize, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1]);

static int video_plugin_process_cmd_result(VPC_DRIVER_INFO_T *ptDriverInfo, VPC_DRIVER_TYPE_E eDriverType, VPC_COMMAND_E eCmd, void *pvParam);
static void video_plugin_fill_decoding_result(VPC_DRIVER_INFO_T *ptDriverInfo, DP_Video_DecodingResult_T *ptDecodingResult);
static void video_plugin_check_running_driver_info(const VPC_DRIVER_INFO_T *ptDriverInfo);


void video_plugin_get_decode_result_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
		#if defined(TCC_HEVC_INCLUDE)
		vpu_hevc_get_decode_result_info(ptDriverInfo);
		#elif defined(TCC_VPU_4K_D2_INCLUDE)
		vpu_4kd2_get_decode_result_info(ptDriverInfo);
		#endif
	} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
		#if defined(TCC_JPU_C6_INCLUDE)
		vpu_jdec_get_decode_result_info(ptDriverInfo);
		#endif
	} else {
		vpu_vdec_get_decode_result_info(ptDriverInfo);
	}

	#if (VDEC_DUMP_ON == 1)
	{
		VPC_DECODE_RESULT_INFO_T *ptDecResult = &ptDriverInfo->tDecodeResultInfo;
		VP_DUMP_DISP_INFO_T tDump = {0, };

		tDump.iWidth = ptDecResult->tVideoResolution.iWidth;
		tDump.iHeight = ptDecResult->tVideoResolution.iHeight;
		tDump.uiCodec = ptDriverInfo->uiCodec;
		tDump.bInterleaveMode = ptDriverInfo->bCbCrInterleaveMode;
		tDump.iDumpMax = 10;

		tDump.pucY = vpu_get_framebuf_virtaddr(ptDriverInfo->pucFrameBuffer, ptDecResult->pucDispOut[VPK_PA][VPK_COMP_Y], VPK_PA);
		tDump.pucU = vpu_get_framebuf_virtaddr(ptDriverInfo->pucFrameBuffer, ptDecResult->pucDispOut[VPK_PA][VPK_COMP_U], VPK_PA);
		tDump.pucV = vpu_get_framebuf_virtaddr(ptDriverInfo->pucFrameBuffer, ptDecResult->pucDispOut[VPK_PA][VPK_COMP_V], VPK_PA);

		vpu_dump_disp_frame(&tDump);
	}
	#endif
}

int video_plugin_cmd(VPC_DRIVER_INFO_T *ptDriverInfo, VPC_DRIVER_TYPE_E eDriverType, VPC_COMMAND_E eCmd, void *pvParam)
{
	int ret = 0;
	unsigned int uiCmd = 0;
	char strCmd[DPV_CMD_STR_MAX] = {0, };

	DPV_FUNC_LOG();

	vpu_set_skip_mode(ptDriverInfo, eCmd);

	if (vpu_translate_cmd(eCmd, &uiCmd, strCmd) == 0) {
		int iDriverFd = vpu_get_driverfd(ptDriverInfo, eDriverType);

		if (iDriverFd >= 0) {
			void *pvCmdParam = vpu_get_cmd_param(ptDriverInfo, eCmd, pvParam);
			ret = ioctl(iDriverFd, uiCmd, pvCmdParam);
		} else {
			ret = -1;
		}

		if (ret != 0) {
			DPV_ERROR_MSG("[%s(%d)] [ERROR] ioctl(%s): %d \n", __func__, __LINE__, strCmd, ret);
			ret = -1;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] UNKNOWN COMMAND: %d \n", __func__, __LINE__, (int)eCmd);
	}

	return ret;
}

int video_plugin_poll_cmd(VPC_DRIVER_INFO_T *ptDriverInfo, VPC_DRIVER_TYPE_E eDriverType, VPC_COMMAND_E eCmd, void *pvParam)
{
	int iRetVal = 0;
	void *pvCmdParam = vpu_get_cmd_param(ptDriverInfo, eCmd, pvParam);

	DPV_FUNC_LOG();

	if (video_plugin_cmd(ptDriverInfo, eDriverType, eCmd, pvCmdParam) < 0) {
		iRetVal = -1;
	} else {
		int iMaxPollTryCnt = 3;
		VPC_DRIVER_POLL_ERROR_E eErrorType = (VPC_DRIVER_POLL_ERROR_E)VPC_DRIVER_POLL_ERROR_NONE;

		while (iMaxPollTryCnt > 0) {
			int iVal = 0;
			int iRetryCnt = 2;
			int iDriverFd = vpu_get_driverfd(ptDriverInfo, eDriverType);

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
					eErrorType = (VPC_DRIVER_POLL_ERROR_E)VPC_DRIVER_POLL_ERROR_RET;
					continue;
				} else if (iVal == 0) {
					iRetryCnt--;
					eErrorType = (VPC_DRIVER_POLL_ERROR_E)VPC_DRIVER_POLL_ERROR_TIMEOUT;
					continue;
				} else {
					if (tPollFd[0].revents >= 0) {
						if (((unsigned int)tPollFd[0].revents & (unsigned int)POLLERR) == (unsigned int)POLLERR) {
							bIsLoopEnd = true;
							eErrorType = (VPC_DRIVER_POLL_ERROR_E)VPC_DRIVER_POLL_ERROR_POLLERR;
						} else if (((unsigned int)tPollFd[0].revents & (unsigned int)POLLIN) == (unsigned int)POLLIN) {
							bIsLoopEnd = true;
							iMaxPollTryCnt = 0;
							eErrorType = (VPC_DRIVER_POLL_ERROR_E)VPC_DRIVER_POLL_ERROR_NONE;
						} else {
							eErrorType = (VPC_DRIVER_POLL_ERROR_E)VPC_DRIVER_POLL_ERROR_UNKNOWN;
						}
					} else {
						bIsLoopEnd = true;
					}
				}

				if (bIsLoopEnd == true) {
					break;
				}
			}

			iRetVal = video_plugin_process_cmd_result(ptDriverInfo, eDriverType, eCmd, pvCmdParam);

			if (iRetVal >= 0) {
				if (((unsigned int)iRetVal & (unsigned int)0xf000) != (unsigned int)0x0000) {
					DPV_ERROR_MSG("[%s(%d)] [ERROR] VPK_RETCODE_CODEC_EXIT: %d, %d \n", __func__, __LINE__, (int)eDriverType, (int)eCmd);
					iRetVal = VPK_RETCODE_CODEC_EXIT;
				}
			}
		}

		if (eErrorType == (VPC_DRIVER_POLL_ERROR_E)VPC_DRIVER_POLL_ERROR_RET) {
			DPV_ERROR_MSG("[%s(%d)] [ERROR] RET: poll(%d, %d) \n", __func__, __LINE__, (int)eDriverType, (int)eCmd);
		} else if (eErrorType == (VPC_DRIVER_POLL_ERROR_E)VPC_DRIVER_POLL_ERROR_POLLERR) {
			DPV_ERROR_MSG("[%s(%d)] [ERROR] POLLERR: poll(%d, %d)  \n", __func__, __LINE__, (int)eDriverType, (int)eCmd);
		} else {
			DPV_DONOTHING(0);
		}
	}

	return iRetVal;
}

DP_Video_Error_E video_plugin_open_drivers(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DP_Video_Error_E ret = (DP_Video_Error_E)DP_Video_ErrorNone;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		video_plugin_check_running_driver_info(ptDriverInfo);

		ret = vpu_open_mgr(ptDriverInfo);

		if (ret == (DP_Video_Error_E)DP_Video_ErrorNone) {
			ret = vpu_open_vpu(ptDriverInfo);
		}

		if (ret == (DP_Video_Error_E)DP_Video_ErrorNone) {
			ret = vpu_open_mem(ptDriverInfo);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
		ret = (DP_Video_Error_E)DP_Video_ErrorInit;
	}

	return ret;
}

DP_Video_Error_E video_plugin_close_drivers(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DP_Video_Error_E ret = (DP_Video_Error_E)DP_Video_ErrorNone;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		(void)vpu_close_mgr(ptDriverInfo);
		(void)vpu_close_vpu(ptDriverInfo);
		(void)vpu_close_mem(ptDriverInfo);
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
		ret = (DP_Video_Error_E)DP_Video_ErrorDeinit;
	}

	return ret;
}

void video_plugin_set_init_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			vpu_hevc_set_init_info(ptDriverInfo);
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			vpu_4kd2_set_init_info(ptDriverInfo);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			vpu_jdec_set_init_info(ptDriverInfo);
			#endif
		} else {
			vpu_vdec_set_init_info(ptDriverInfo);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

int video_plugin_is_pic_resolution_ok(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_RESOLUTION_INFO_T tMaxResInfo, VPC_RESOLUTION_INFO_T tMinResInfo)
{
	int iRetVal = -1;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if ((tMaxResInfo.iWidth < INT16_MAX) && (tMaxResInfo.iHeight < INT16_MAX)) {
			if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
				#if defined(TCC_HEVC_INCLUDE)
				iRetVal = vpu_hevc_is_pic_resolution_ok(ptDriverInfo, tMaxResInfo, tMinResInfo);
				#elif defined(TCC_VPU_4K_D2_INCLUDE)
				iRetVal = vpu_4kd2_is_pic_resolution_ok(ptDriverInfo, tMaxResInfo, tMinResInfo);
				#endif
			} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
				#if defined(TCC_JPU_C6_INCLUDE)
				iRetVal = vpu_jdec_is_pic_resolution_ok(ptDriverInfo, tMaxResInfo, tMinResInfo);
				#endif
			} else {
				iRetVal = vpu_vdec_is_pic_resolution_ok(ptDriverInfo, tMaxResInfo, tMinResInfo);
			}
		} else {
			iRetVal = -1;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return iRetVal;
}

int video_plugin_alloc_bitstream_buffer(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iBitstreamBufSize = -1;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		int iTmpBitstreamBufSize = ptDriverInfo->iBitstreamBufSize;
		int iLargeBitstreamBufSize = video_plugin_get_large_bitstream_buffer_size(ptDriverInfo);

		if (iTmpBitstreamBufSize > iLargeBitstreamBufSize) {
			iBitstreamBufSize = video_plugin_align_buffer(iTmpBitstreamBufSize, (64 * 1024));
		} else {
			iBitstreamBufSize = iLargeBitstreamBufSize;
		}

		iBitstreamBufSize = video_plugin_align_buffer(iBitstreamBufSize, DPV_ALIGN_LEN);

		if (iBitstreamBufSize > 0) {
			int iTmpVal = -1;
			video_plugin_set_alloc_mem_info(ptDriverInfo, iBitstreamBufSize, (int)VPK_BUFFER_STREAM);
			iTmpVal = video_plugin_cmd(ptDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_ALLOC_MEMORY, NULL);

			if (iTmpVal >= 0) {
				const void *ptTmpMMapResult = video_plugin_mmap_bitstream_buffer(ptDriverInfo, iBitstreamBufSize, ptDriverInfo->pucBitstreamBuffer);

				if (ptTmpMMapResult == NULL) {
					DPV_ERROR_MSG("[%s(%d)] [ERROR] mmap() \n", __func__, __LINE__);
					ptDriverInfo->pucBitstreamBuffer[VPK_VA] = NULL;
					(void)video_plugin_cmd(ptDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_FREE_MEMORY, NULL);

					iBitstreamBufSize = -1;
				} else {
					video_plugin_set_bitstream_buffer_info(ptDriverInfo, ptDriverInfo->pucBitstreamBuffer, iBitstreamBufSize);
					ptDriverInfo->iBitstreamBufSize = iBitstreamBufSize;
				}
			} else {
				iBitstreamBufSize = -1;
			}
		}
	}

	DPV_DETAIL_MSG("[%s(%d)] %d: %p, %p, %p \n", __func__, __LINE__,
			ptDriverInfo->iBitstreamBufSize, ptDriverInfo->pucBitstreamBuffer[VPK_PA], ptDriverInfo->pucBitstreamBuffer[VPK_VA], ptDriverInfo->pucBitstreamBuffer[VPK_K_VA]);

	return iBitstreamBufSize;
}

void video_plugin_free_alloc_buffers(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->pucBitstreamBuffer[VPK_VA] != NULL) {
			video_plugin_free_alloc_buffer(ptDriverInfo->iBitstreamBufSize, ptDriverInfo->pucBitstreamBuffer[VPK_VA]);
			ptDriverInfo->pucBitstreamBuffer[VPK_VA] = NULL;
		}

		if (ptDriverInfo->pucFrameBuffer[VPK_VA] != NULL) {
			video_plugin_free_alloc_buffer(ptDriverInfo->iFrameBufferSize, ptDriverInfo->pucFrameBuffer[VPK_VA]);
			ptDriverInfo->pucFrameBuffer[VPK_VA] = NULL;
		}
	}
}

void video_plugin_set_decoding_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer, int iBufSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		ptDriverInfo->pucInputBuffer[VPK_PA] 	= pucBuffer;
		ptDriverInfo->pucInputBuffer[VPK_VA] 	= pucBuffer;
		ptDriverInfo->iInputBufSize 			= iBufSize;

		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			vpu_hevc_set_decoding_bitstream_buffer_info(ptDriverInfo);
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			vpu_4kd2_set_decoding_bitstream_buffer_info(ptDriverInfo);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			vpu_jdec_set_decoding_bitstream_buffer_info(ptDriverInfo);
			#endif
		} else {
			vpu_vdec_set_decoding_bitstream_buffer_info(ptDriverInfo);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

int video_plugin_alloc_spspps_buffer(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSpsPpsBuffer)
{
	int iSpsPpsSize = -1;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		iSpsPpsSize = VPK_PS_SAVE_SIZE;
		iSpsPpsSize = video_plugin_align_buffer(iSpsPpsSize, DPV_ALIGN_LEN);
		video_plugin_set_alloc_mem_info(ptDriverInfo, iSpsPpsSize, (int)VPK_BUFFER_PS);

		if (iSpsPpsSize > 0) {
			int iTmpVal = -1;

			iTmpVal = video_plugin_cmd(ptDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_ALLOC_MEMORY, NULL);

			if (iTmpVal >= 0) {
				pucSpsPpsBuffer = video_plugin_get_phy_addr(ptDriverInfo);
				video_plugin_set_spspps_buffer_info(ptDriverInfo, pucSpsPpsBuffer, iSpsPpsSize);
			} else {
				iSpsPpsSize = -1;
			}
		}
	}

	return iSpsPpsSize;
}

int video_plugin_alloc_bitwork_buffer(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iBitWorkBufSize = -1;

	if (ptDriverInfo != NULL) {
		int iTmpVal = -1;

		iBitWorkBufSize = video_plugin_get_bitwork_buffer_size(ptDriverInfo);
		iBitWorkBufSize = video_plugin_align_buffer(iBitWorkBufSize, DPV_ALIGN_LEN);
		video_plugin_set_alloc_mem_info(ptDriverInfo, iBitWorkBufSize, (int)VPK_BUFFER_WORK);

		iTmpVal = video_plugin_cmd(ptDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_ALLOC_MEMORY, NULL);

		if (iTmpVal >= 0) {
			video_plugin_set_bitwork_addr(ptDriverInfo);
		} else {
			iBitWorkBufSize = -1;
		}
	}

	return iBitWorkBufSize;
}

void video_plugin_get_header_info(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			vpu_hevc_get_header_info(ptDriverInfo);
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			vpu_4kd2_get_header_info(ptDriverInfo);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			vpu_jdec_get_header_info(ptDriverInfo);
			#endif
		} else {
			vpu_vdec_get_header_info(ptDriverInfo);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

int video_plugin_alloc_slice_buffer(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iSliceBufSize = -1;

	if (ptDriverInfo != NULL) {
		int iTmpVal = -1;

		iSliceBufSize = VPK_SLICE_SAVE_SIZE;
		iSliceBufSize = video_plugin_align_buffer(iSliceBufSize, DPV_ALIGN_LEN);
		video_plugin_set_alloc_mem_info(ptDriverInfo, iSliceBufSize, (int)VPK_BUFFER_SLICE);

		iTmpVal = video_plugin_cmd(ptDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_ALLOC_MEMORY, NULL);

		if (iTmpVal >= 0) {
			ptDriverInfo->pucSliceBuffer = video_plugin_get_phy_addr(ptDriverInfo);
			ptDriverInfo->iSliceBufSize = iSliceBufSize;
			video_plugin_set_slice_buffer_info(ptDriverInfo, ptDriverInfo->pucSliceBuffer, ptDriverInfo->iSliceBufSize, ptDriverInfo->iExtraFrameCount);
		} else {
			iSliceBufSize = -1;
		}
	}

	return iSliceBufSize;
}

int video_plugin_set_seq_hearder(VPC_DRIVER_INFO_T *ptDriverInfo, DP_Video_Config_SequenceHeader_T *ptSequenceHeader)
{
	int iRet = -1;

	if (ptDriverInfo != NULL) {
		if (ptSequenceHeader->iStreamLen > ptDriverInfo->iBitstreamBufSize) {
			ptSequenceHeader->iStreamLen = ptDriverInfo->iBitstreamBufSize;
		}

		video_plugin_set_seq_header_bitstream_buffer_info(ptDriverInfo, ptDriverInfo->iBitstreamBufSize, ptDriverInfo->pucBitstreamBuffer);

		if (ptSequenceHeader->iStreamLen > 0) {
			VDEC_MEMCPY(ptDriverInfo->pucBitstreamBuffer[VPK_VA], ptSequenceHeader->pucStream, (size_t)ptSequenceHeader->iStreamLen);
			video_plugin_set_stream_size(ptDriverInfo, ptSequenceHeader->iStreamLen);
			iRet = video_plugin_poll_cmd(ptDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_SEQ_HEADER, NULL);
		}
	}

	return iRet;
}

int video_plugin_alloc_frame_buffer(VPC_DRIVER_INFO_T *ptDriverInfo, int iAvailBufSize, int iMaxFrameBufferCnt)
{
	int ret = -1;

	if (ptDriverInfo != NULL) {
		int iTmpVal = -1;
		int iMaxBufCount = iAvailBufSize / ptDriverInfo->iMinFrameBufferSize;

		if (ptDriverInfo->iFrameBufferCount > iMaxBufCount) {
			ptDriverInfo->iFrameBufferCount = iMaxBufCount;
		}

		if (ptDriverInfo->iFrameBufferCount > iMaxFrameBufferCnt) {
			ptDriverInfo->iFrameBufferCount = iMaxFrameBufferCnt;
		}

		ptDriverInfo->iFrameBufferSize = ptDriverInfo->iFrameBufferCount * ptDriverInfo->iMinFrameBufferSize;
		ptDriverInfo->iFrameBufferSize = video_plugin_align_buffer(ptDriverInfo->iFrameBufferSize, DPV_ALIGN_LEN);

		video_plugin_set_alloc_mem_info(ptDriverInfo, ptDriverInfo->iFrameBufferSize, (int)VPK_BUFFER_FRAMEBUFFER);
		iTmpVal = video_plugin_cmd(ptDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_ALLOC_MEMORY, NULL);

		if (iTmpVal >= 0) {
			if (ptDriverInfo->iFrameBufferSize > 0) {
				void *ptTmpMMapResult = NULL;
				unsigned int uiMapProt = (unsigned int)PROT_READ | (unsigned int)PROT_WRITE;

				size_t tTmpSize = (size_t)ptDriverInfo->iFrameBufferSize;

				ptDriverInfo->pucFrameBuffer[VPK_PA] 	= video_plugin_get_phy_addr(ptDriverInfo);
				ptDriverInfo->pucFrameBuffer[VPK_K_VA] 	= video_plugin_get_remap_addr(ptDriverInfo);

				ptTmpMMapResult = (void *)mmap(NULL, tTmpSize, (int)uiMapProt, MAP_SHARED, ptDriverInfo->tDriverFD.iVpuFd, (off_t)ptDriverInfo->pucFrameBuffer[VPK_PA]);

				if (ptTmpMMapResult != MAP_FAILED) {
					DPV_CAST(ptDriverInfo->pucFrameBuffer[VPK_VA], ptTmpMMapResult);
				} else {
					DPV_ERROR_MSG("[%s(%d)] [ERROR] MMAP: Framebuffer \n", __func__, __LINE__);
				}

				video_plugin_set_frame_buffer_start_addr_info(ptDriverInfo, ptDriverInfo->pucFrameBuffer, ptDriverInfo->iFrameBufferCount);

				ret = video_plugin_poll_cmd(ptDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_REG_FRAME_BUFFER, NULL);
			} else {
				ret = -1;
			}
		} else {
			ret = -1;
		}
	}

	return ret;
}

void video_plugin_set_frame_skip_mode(VPC_DRIVER_INFO_T *ptDriverInfo, VPC_FRAME_SKIP_MODE_E eFrameSkipMode)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPC_FRAME_SKIP_T *ptFrameSkip = &ptDriverInfo->tFrameSkip;
		const VPC_DECODE_RESULT_INFO_T *ptDecResult = &ptDriverInfo->tDecodeResultInfo;

		if ((ptDecResult->iPictureType >= 0) && (ptDecResult->iPictureStructure >= 0)) {
			video_plugin_frame_set_frame_skip_mode(ptFrameSkip, eFrameSkipMode, ptDriverInfo->uiCodec, ptDecResult->iDecodingOutResult,
														(unsigned long)ptDecResult->iPictureType, (unsigned long)ptDecResult->iPictureStructure);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

unsigned long video_plugin_get_video_decode_result(VPC_DRIVER_INFO_T *ptDriverInfo, DP_Video_DecodingResult_T *ptDecodingResult)
{
	unsigned long ulResult = 0UL;

	DPV_FUNC_LOG();

	video_plugin_fill_decoding_result(ptDriverInfo, ptDecodingResult);

	switch (ptDecodingResult->iDecodingStatus) {
		case VPK_VPU_DEC_SUCCESS: {
			if (ptDecodingResult->iDecodedIdx >= 0) {
				ulResult |= (unsigned long)DPV_DECODING_SUCCESS_FRAME;
				DPV_CHECK_STR_LOG("DPV_DECODING_SUCCESS_FRAME");
			} else if (ptDecodingResult->iDecodedIdx == -2) {
				ulResult |= (unsigned long)DPV_DECODING_SUCCESS_SKIPPED;
				DPV_CHECK_STR_LOG("DPV_DECODING_SUCCESS_SKIPPED");
			} else {
				DPV_DONOTHING(0);
			}
			break;
		}

		case VPK_VPU_DEC_SUCCESS_FIELD_PICTURE: {
			ulResult |= (unsigned long)DPV_DECODING_SUCCESS_FIELD;
			DPV_CHECK_STR_LOG("DPV_DECODING_SUCCESS_FIELD");
			break;
		}

		case VPK_VPU_DEC_BUF_FULL: {
			ulResult |= (unsigned long)DPV_DECODING_BUFFER_FULL;
			DPV_ERROR_MSG("[%s(%d)] [ERROR] DPV_DECODING_BUFFER_FULL \n", __func__, __LINE__);
			break;
		}

		case VPK_VPU_DEC_RESOLUTION_CHANGED: {
			if (ptDecodingResult->iDecodedIdx >= 0) {
				ulResult |= (unsigned long)DPV_DECODING_SUCCESS_FRAME;
				DPV_CHECK_STR_LOG("DPV_DECODING_SUCCESS_FRAME");
			}

			ulResult |= (unsigned long)DPV_RESOLUTION_CHANGED;
			DPV_CHECK_STR_LOG("DPV_RESOLUTION_CHANGED");
			break;
		}

		default:
			DPV_CHECK_STR_LOG("DPV_DECODING_UKNOWN");
			ulResult = 0UL;
			break;
	}

	if (ptDecodingResult->iDecodingStatus != VPK_VPU_DEC_BUF_FULL) {
		if (ptDriverInfo->iCurrStreamIdx != -1) {
			if (ptDriverInfo->iCurrStreamIdx == 0) {
				ptDriverInfo->iCurrStreamIdx = 1;
			} else {
				ptDriverInfo->iCurrStreamIdx = 0;
			}
		}
	}

	if ((ulResult & (unsigned long)DPV_DECODING_SUCCESS_FRAME) == (unsigned long)DPV_DECODING_SUCCESS_FRAME) {
		if (ptDriverInfo->iDecodedFrames < (INT32_MAX - 1)) {
			ptDriverInfo->iDecodedFrames++;
		} else {
			ptDriverInfo->iDecodedFrames = 0;
		}

		if (ptDriverInfo->iDisplyedFrames == 0) {
			ptDecodingResult->iOutputStatus = VPK_VPU_DEC_OUTPUT_SUCCESS;
		}
	}

	if (ptDecodingResult->iOutputStatus == VPK_VPU_DEC_OUTPUT_SUCCESS) {
		ulResult |= (unsigned long)DPV_DISPLAY_OUTPUT_SUCCESS;
	} else {
		DPV_DETAIL_MSG("[%s(%d)] Out: %d, Dec: %d(%d) \n", __func__, __LINE__, ptDecodingResult->iOutputStatus, ptDecodingResult->iDecodingStatus, ptDecodingResult->iDecodedIdx);
	}

	if ((ulResult & (unsigned long)DPV_DISPLAY_OUTPUT_SUCCESS) == (unsigned long)DPV_DISPLAY_OUTPUT_SUCCESS) {
		if (ptDriverInfo->iDisplyedFrames < (INT32_MAX - 1)) {
			ptDriverInfo->iDisplyedFrames++;
		} else {
			ptDriverInfo->iDisplyedFrames = 0;
		}
	}

	if (ptDecodingResult->iOutputStatus == VPK_VPU_DEC_OUTPUT_SUCCESS) {
		ptDecodingResult->bIsFrameOut = true;
	} else {
		ptDecodingResult->bIsFrameOut = false;
	}

	DPV_DETAIL_MSG("[%s(%d)] (%lu) \n", __func__, __LINE__, ulResult);
	DPV_DETAIL_MSG("[%s(%d)]  - O[%d, %d] D[%d, %d] \n", __func__, __LINE__, ptDecodingResult->iOutputStatus, ptDecodingResult->iDispOutIdx, ptDecodingResult->iDecodingStatus, ptDecodingResult->iDecodedIdx);("[%s(%d)]  O[%d, %d] D[%d, %d] \n", __func__, __LINE__, ptDecodingResult->iOutputStatus, ptDecodingResult->iDispOutIdx, ptDecodingResult->iDecodingStatus, ptDecodingResult->iDecodedIdx);("[%s(%d)]  O[%d, %d] D[%d, %d] \n", __func__, __LINE__, ptDecodingResult->iOutputStatus, ptDecodingResult->iDispOutIdx, ptDecodingResult->iDecodingStatus, ptDecodingResult->iDecodedIdx);
	DPV_DETAIL_MSG("[%s(%d)]  - DEC: %d, %d \n", __func__, __LINE__, ptDecodingResult->iOutputStatus, ptDriverInfo->iDisplyedFrames);
	DPV_DETAIL_MSG("[%s(%d)]  - OUT: %d, %d \n", __func__, __LINE__, ptDecodingResult->iDecodingStatus, ptDriverInfo->iDecodedFrames);
	DPV_DETAIL_MSG("[%s(%d)]  - RES: %d X %d \n", __func__, __LINE__, ptDecodingResult->uiWidth, ptDecodingResult->uiHeight);
	DPV_DETAIL_MSG("[%s(%d)]  - PY: %p : %p \n", __func__, __LINE__, ptDecodingResult->pucDispOut[DPV_PA][DPV_COMP_Y], ptDecodingResult->pucDispOut[DPV_PA][DPV_COMP_Y]);
	DPV_DETAIL_MSG("[%s(%d)]  - VY: %p : %p \n", __func__, __LINE__, ptDecodingResult->pucDispOut[DPV_VA][DPV_COMP_Y], ptDecodingResult->pucDispOut[DPV_VA][DPV_COMP_Y]);

	return ulResult;
}

static int video_plugin_process_cmd_result(VPC_DRIVER_INFO_T *ptDriverInfo, VPC_DRIVER_TYPE_E eDriverType, VPC_COMMAND_E eCmd, void *pvParam)
{
	int iRetVal = -1;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		switch ((int)eCmd) {
			case (int)VPC_COMMAND_V_DEC_INIT: {
				DPV_CHECK_STR_LOG("VPC_COMMAND_V_DEC_INIT");
				(void)video_plugin_cmd(ptDriverInfo, eDriverType, VPC_COMMAND_V_DEC_INIT_RESULT, pvParam);
				iRetVal = vpu_get_cmd_result(ptDriverInfo, eDriverType, eCmd, pvParam);
				break;
			}

			case (int)VPC_COMMAND_V_DEC_SEQ_HEADER: {
				DPV_CHECK_STR_LOG("VPC_COMMAND_V_DEC_SEQ_HEADER");
				(void)video_plugin_cmd(ptDriverInfo, eDriverType, VPC_COMMAND_V_DEC_SEQ_HEADER_RESULT, pvParam);
				iRetVal = vpu_get_cmd_result(ptDriverInfo, eDriverType, eCmd, pvParam);
				break;
			}

			case (int)VPC_COMMAND_V_DEC_DECODE: {
				DPV_CHECK_STR_LOG("VPC_COMMAND_V_DEC_DECODE");
				(void)video_plugin_cmd(ptDriverInfo, eDriverType, VPC_COMMAND_V_DEC_DECODE_RESULT, pvParam);
				iRetVal = vpu_get_cmd_result(ptDriverInfo, eDriverType, eCmd, pvParam);
				break;
			}

			case (int)VPC_COMMAND_V_DEC_CLOSE:
			case (int)VPC_COMMAND_V_DEC_BUF_FLAG_CLEAR:
			case (int)VPC_COMMAND_V_DEC_REG_FRAME_BUFFER:
			default: {
				DPV_CHECK_STR_LOG("default");
				int iVal = 0;
				(void)video_plugin_cmd(ptDriverInfo, eDriverType, VPC_COMMAND_V_DEC_GENERAL_RESULT,  &iVal);
				iRetVal = iVal;
				break;
			}
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return iRetVal;
}

static void video_plugin_fill_decoding_result(VPC_DRIVER_INFO_T *ptDriverInfo, DP_Video_DecodingResult_T *ptDecodingResult)
{
	const VPC_FRAME_SKIP_T *ptFrameSkip = &ptDriverInfo->tFrameSkip;
	VPC_DECODE_RESULT_INFO_T *ptDecResult = &ptDriverInfo->tDecodeResultInfo;

	ptDecodingResult->iIntanceIdx 		= ptDriverInfo->iInstIdx;

	ptDecodingResult->iDecodingStatus 	= ptDecResult->iDecodingResult;
	ptDecodingResult->iOutputStatus 	= ptDecResult->iDecodingOutResult;

	ptDecodingResult->iDecodedIdx 		= ptDecResult->iDecodedIdx;
	ptDecodingResult->iDispOutIdx 		= ptDecResult->iDispOutIdx;

	ptDecodingResult->fAspectRatio 		= 0.0F;

	if ((ptFrameSkip->eFrameSkipMode != (VPC_FRAME_SKIP_MODE_E)VPC_FRAME_SKIP_MODE_NONE) && (ptDecodingResult->iDecodedIdx >= 0)) {
		video_plugin_set_frame_skip_mode(ptDriverInfo, VPC_FRAME_SKIP_MODE_NEXT_STEP);
	}

	if (ptDecResult->eFrameType == (VPC_FRAME_TYPE_E)VPC_FRAME_TYPE_FIELD) {
		ptDecodingResult->bField = true;
	}

	if (((ptDecResult->iM2vProgressiveFrame == 0) && (ptDecResult->iPictureStructure == 3)) ||
			((ptDecResult->iInterlacedFrame != 0) && (ptDecResult->iInterlace != 0))) {
		ptDecodingResult->bInterlaced = true;

		if (ptDecResult->iTopFieldFirst == 0) {
			ptDecodingResult->bOddFieldFirst = true;
		}
	}

	if ((ptDecResult->tVideoResolution.iWidth > 0) && (ptDecResult->tVideoResolution.iHeight > 0)) {
		ptDecodingResult->uiWidth 	= (unsigned int)ptDecResult->tVideoResolution.iWidth;
		ptDecodingResult->uiHeight 	= (unsigned int)ptDecResult->tVideoResolution.iHeight;
	}

	if ((ptDecResult->tCropRect.iLeft >= 0) && (ptDecResult->tCropRect.iTop >= 0) && (ptDecResult->tCropRect.iRight >= 0) && (ptDecResult->tCropRect.iBottom >= 0)) {
		ptDecodingResult->uiCropLeft 	= (unsigned int)ptDecResult->tCropRect.iLeft;
		ptDecodingResult->uiCropTop 	= (unsigned int)ptDecResult->tCropRect.iTop;
		ptDecodingResult->uiCropRight 	= (unsigned int)ptDecResult->tCropRect.iRight;
		ptDecodingResult->uiCropBottom 	= (unsigned int)ptDecResult->tCropRect.iBottom;

		if (ptDecodingResult->uiWidth >= (ptDecodingResult->uiCropLeft + ptDecodingResult->uiCropRight)) {
			ptDecodingResult->uiCropWidth = ptDecodingResult->uiWidth  - ptDecodingResult->uiCropLeft - ptDecodingResult->uiCropRight;
		}

		if (ptDecodingResult->uiHeight >= (ptDecodingResult->uiCropTop + ptDecodingResult->uiCropBottom)) {
			ptDecodingResult->uiCropHeight = ptDecodingResult->uiHeight - ptDecodingResult->uiCropTop  - ptDecodingResult->uiCropBottom;
		}
	}

	ptDecodingResult->pucDispOut[DPV_VA][DPV_COMP_Y] = ptDecResult->pucDispOut[VPK_VA][VPK_COMP_Y];
	ptDecodingResult->pucDispOut[DPV_VA][DPV_COMP_U] = ptDecResult->pucDispOut[VPK_VA][VPK_COMP_U];
	ptDecodingResult->pucDispOut[DPV_VA][DPV_COMP_V] = ptDecResult->pucDispOut[VPK_VA][VPK_COMP_V];

	ptDecodingResult->pucDispOut[DPV_PA][DPV_COMP_Y] = ptDecResult->pucDispOut[VPK_PA][VPK_COMP_Y];
	ptDecodingResult->pucDispOut[DPV_PA][DPV_COMP_U] = ptDecResult->pucDispOut[VPK_PA][VPK_COMP_U];
	ptDecodingResult->pucDispOut[DPV_PA][DPV_COMP_V] = ptDecResult->pucDispOut[VPK_PA][VPK_COMP_V];

	if ((ptDecResult->iDecodedIdx >= 0) &&
			((ptDecodingResult->iDecodingStatus == VPK_VPU_DEC_SUCCESS_FIELD_PICTURE) || (ptDecodingResult->iDecodingStatus == VPK_VPU_DEC_SUCCESS))) {
		ptDecodingResult->bIsInsertClock = true;

		if (ptDecodingResult->iDecodingStatus == VPK_VPU_DEC_SUCCESS) {
			ptDecodingResult->bIsInsertFrame = true;

			if ((ptDecResult->tVideoResolution.iWidth == 1920) && (ptDecResult->tVideoResolution.iHeight > 1080)) {
				ptDecResult->tVideoResolution.iHeight = 1080;
			}
		}
	} else {
		ptDecodingResult->bIsInsertClock = false;
		ptDecodingResult->bIsInsertFrame = false;
	}
}

static void video_plugin_check_running_driver_info(const VPC_DRIVER_INFO_T *ptDriverInfo)
{
	if (ptDriverInfo != NULL) {
		DPV_DETAIL_MSG("[%s(%d)] ==================================================== \n", __func__, __LINE__);

		#if defined(TCC_803X_INCLUDE)
		DPV_DETAIL_MSG("[%s(%d)] TCC_803X_INCLUDE !! \n", __func__, __LINE__);
		#endif

		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			DPV_DETAIL_MSG("[%s(%d)] TCC_HEVC IS WORKING \n", __func__, __LINE__);
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			DPV_DETAIL_MSG("[%s(%d)] TCC_VPU_4K_D2 IS WORKING \n", __func__, __LINE__);
			#endif
		}

		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			DPV_DETAIL_MSG("[%s(%d)] TCC_JPU IS WORKING \n", __func__, __LINE__);
			#endif
		}

		if (ptDriverInfo->bEnable10Bits == true) {
			DPV_DETAIL_MSG("[%s(%d)] bEnable10Bits \n", __func__, __LINE__);
		}

		if (ptDriverInfo->bEnableMapConverter == true) {
			DPV_DETAIL_MSG("[%s(%d)] bEnableMapConverter \n", __func__, __LINE__);
		}

		#ifdef USE_PREV_STREAMBUFF_DECODING
		DPV_DETAIL_MSG("[%s(%d)] USE_PREV_STREAMBUFF_DECODING \n", __func__, __LINE__);
		#endif

		#ifdef USE_VPU_HANGUP_RELEASE
		DPV_DETAIL_MSG("[%s(%d)] USE_VPU_HANGUP_RELEASE \n", __func__, __LINE__);
		#endif

		#if defined(SUPPORT_HDMI_DRM_DIRECT_CONFIG)
		DPV_DETAIL_MSG("[%s(%d)] SUPPORT_HDMI_DRM_DIRECT_CONFIG \n", __func__, __LINE__);
		#endif

		#if defined(TC_SECURE_MEMORY_COPY)
		DPV_DETAIL_MSG("[%s(%d)] TC_SECURE_MEMORY_COPY \n", __func__, __LINE__);
		#endif

		DPV_DETAIL_MSG("[%s(%d)] ==================================================== \n", __func__, __LINE__);
	}
}

static int video_plugin_get_large_bitstream_buffer_size(const VPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iRetVal = -1;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			iRetVal = VPK_WAVE4_STREAM_BUF_SIZE;
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			iRetVal = VPK_WAVE5_STREAM_BUF_SIZE;
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			iRetVal = VPK_LARGE_STREAM_BUF_SIZE * 4;
			#endif
		} else {
			iRetVal = VPK_LARGE_STREAM_BUF_SIZE;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return iRetVal;
}

static int video_plugin_align_buffer(int iBuf, int iMul)
{
	int iRetVal = 0;

	unsigned int uiTmpBuf = 0U;
	unsigned int uiTmpMul = 0U;
	unsigned int uiTmpRet = 0U;

	DPV_FUNC_LOG();

	if ((iBuf >= 0) && (iMul > 0)) {
		uiTmpBuf = (unsigned int)iBuf;
		uiTmpMul = (unsigned int)iMul;

		uiTmpRet = ((uiTmpBuf + (uiTmpMul - 1U)) & ~(uiTmpMul - 1U));
	}

	iRetVal = (int)uiTmpRet;

	return iRetVal;
}

static void video_plugin_set_alloc_mem_info(VPC_DRIVER_INFO_T *ptDriverInfo, int iReqSize, int iReqType)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_MEM_ALLOC_INFO_t *ptAllocMem = &ptDriverInfo->tAllocMem;

		(void)memset(ptAllocMem, 0x00, sizeof(VPK_MEM_ALLOC_INFO_t));

		if ((iReqSize >= 0) &&
				((iReqType >= (int)VPK_BUFFER_TYPE_MIN) && (iReqType < (int)VPK_BUFFER_TYPE_MAX))) {
			ptAllocMem->request_size 	= (unsigned int)iReqSize;
			ptAllocMem->buffer_type 	= (VPK_BUFFER_TYPE)iReqType;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static void *video_plugin_mmap_bitstream_buffer(VPC_DRIVER_INFO_T *ptDriverInfo, int iBitstreamBufSize, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1])
{
	void *ptRetPtr = NULL;

	unsigned int uiMapProt = (unsigned int)PROT_READ | (unsigned int)PROT_WRITE;
	size_t tTmpSize = 0UL;

	DPV_FUNC_LOG();

	if (iBitstreamBufSize > 0) {
		tTmpSize = (size_t)iBitstreamBufSize;
	}

	pucBitstreamBuffer[VPK_PA] 		= video_plugin_get_phy_addr(ptDriverInfo);
	pucBitstreamBuffer[VPK_K_VA] 	= video_plugin_get_remap_addr(ptDriverInfo);

	ptRetPtr = (void *)mmap(NULL, tTmpSize, (int)uiMapProt, MAP_SHARED, ptDriverInfo->tDriverFD.iVpuFd, (off_t)pucBitstreamBuffer[VPK_PA]);

	if (ptRetPtr == NULL) {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] mmap \n", __func__, __LINE__);

		if (pucBitstreamBuffer[VPK_VA] != NULL) {
			if (iBitstreamBufSize > 0) {
				(void)munmap(pucBitstreamBuffer[VPK_VA], (size_t)iBitstreamBufSize);
			}
			pucBitstreamBuffer[VPK_VA] = NULL;
		}

		ptRetPtr = NULL;
	} else {
		DPV_CAST(pucBitstreamBuffer[VPK_VA], ptRetPtr);
	}

	return ptRetPtr;
}

static void video_plugin_free_alloc_buffer(int iBufferSize, unsigned char *pucBuffer)
{
	DPV_FUNC_LOG();

	if (pucBuffer != NULL) {
		if (iBufferSize > 0) {
			(void)munmap(pucBuffer, (size_t)iBufferSize);
		}
		pucBuffer = NULL;
	}
}

static unsigned char *video_plugin_get_phy_addr(const VPC_DRIVER_INFO_T *ptDriverInfo)
{
	unsigned char *pucRetVal = NULL;
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const VPK_MEM_ALLOC_INFO_t *ptAllocMem = &ptDriverInfo->tAllocMem;

		uintptr_t tmpPtr = ptAllocMem->phy_addr;
		DPV_CAST(pucRetVal, tmpPtr);
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return pucRetVal;
}

static unsigned char *video_plugin_get_remap_addr(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	unsigned char *pucRetVal = NULL;
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		VPK_MEM_ALLOC_INFO_t *ptAllocMem = &ptDriverInfo->tAllocMem;

		DPV_CAST(pucRetVal, ptAllocMem->kernel_remap_addr);
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return pucRetVal;
}

static void video_plugin_set_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1], int iBitstreamBufSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			vpu_hevc_set_bitstream_buffer_info(ptDriverInfo, pucBitstreamBuffer, iBitstreamBufSize);
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			vpu_4kd2_set_bitstream_buffer_info(ptDriverInfo, pucBitstreamBuffer, iBitstreamBufSize);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			vpu_jdec_set_bitstream_buffer_info(ptDriverInfo, pucBitstreamBuffer, iBitstreamBufSize);
			#endif
		} else {
			vpu_vdec_set_bitstream_buffer_info(ptDriverInfo, pucBitstreamBuffer, iBitstreamBufSize);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static void video_plugin_set_seq_header_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, int iBitstreamBufSize, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1])
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			vpu_hevc_set_seq_header_bitstream_buffer_info(ptDriverInfo, iBitstreamBufSize, pucBitstreamBuffer);
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			vpu_4kd2_set_seq_header_bitstream_buffer_info(ptDriverInfo, iBitstreamBufSize, pucBitstreamBuffer);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			vpu_jdec_set_seq_header_bitstream_buffer_info(ptDriverInfo, iBitstreamBufSize, pucBitstreamBuffer);
			#endif
		} else {
			vpu_vdec_set_seq_header_bitstream_buffer_info(ptDriverInfo, iBitstreamBufSize, pucBitstreamBuffer);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static void video_plugin_set_spspps_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSpsPpsBuffer, int iSpsPpsSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			DPV_DONOTHING(0);
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			DPV_DONOTHING(0);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			DPV_DONOTHING(0);
			#endif
		} else {
			vpu_vdec_set_spspps_buffer_info(ptDriverInfo, pucSpsPpsBuffer, iSpsPpsSize);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static void video_plugin_set_bitwork_addr(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			vpu_hevc_set_bitwork_addr(ptDriverInfo);
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			vpu_4kd2_set_bitwork_addr(ptDriverInfo);
			#endif
		} else {
			vpu_vdec_set_bitwork_addr(ptDriverInfo);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static int video_plugin_get_bitwork_buffer_size(const VPC_DRIVER_INFO_T *ptDriverInfo)
{
	int iRetVal = -1;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			iRetVal = VPK_WAVE4_WORK_CODE_BUF_SIZE;
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			iRetVal = VPK_WAVE5_WORK_CODE_BUF_SIZE;
			#endif
		} else {
			iRetVal = VPK_WORK_CODE_PARA_BUF_SIZE;
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return iRetVal;
}

static void video_plugin_set_slice_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSliceBuffer, int iSliceBufSize, int iExtraFrameCount)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			vpu_hevc_get_slice_buffer_info(ptDriverInfo, pucSliceBuffer, iSliceBufSize, iExtraFrameCount);
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			vpu_4kd2_get_slice_buffer_info(ptDriverInfo, pucSliceBuffer, iSliceBufSize, iExtraFrameCount);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			DPV_DONOTHING(0);
			#endif
		} else {
			vpu_vdec_get_slice_buffer_info(ptDriverInfo, pucSliceBuffer, iSliceBufSize, iExtraFrameCount);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static void video_plugin_set_frame_buffer_start_addr_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucFrameBuffer[VPK_K_VA + 1], int iFrameBufferSize)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			vpu_hevc_set_frame_buffer_start_addr_info(ptDriverInfo, pucFrameBuffer, iFrameBufferSize);
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			vpu_4kd2_set_frame_buffer_start_addr_info(ptDriverInfo, pucFrameBuffer, iFrameBufferSize);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			vpu_jdec_set_frame_buffer_start_addr_info(ptDriverInfo, pucFrameBuffer, iFrameBufferSize);
			#endif
		} else {
			vpu_vdec_set_frame_buffer_start_addr_info(ptDriverInfo, pucFrameBuffer, iFrameBufferSize);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}

static void video_plugin_set_stream_size(VPC_DRIVER_INFO_T *ptDriverInfo, int iStreamLen)
{
	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			vpu_hevc_set_stream_size(ptDriverInfo, iStreamLen);
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			vpu_4kd2_set_stream_size(ptDriverInfo, iStreamLen);
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			vpu_jdec_set_stream_size(ptDriverInfo, iStreamLen);
			#endif
		} else {
			vpu_vdec_set_stream_size(ptDriverInfo, iStreamLen);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}
}