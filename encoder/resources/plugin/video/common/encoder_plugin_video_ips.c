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


#include <unistd.h>

#include <encoder_plugin_video_ips.h>


#define VENC_MAX_CNT (16)

#define VPU_ENC_MGR 			"/dev/vpu_dev_mgr"

#if defined(TCC_JPU_C6_INCLUDE)
#define JPU_ENC_MGR				"/dev/jpu_dev_mgr"
#endif

#if defined(TCC_VPU_4K_E1_INCLUDE)
#define VPU_ENC_HEVC_MGR	 	"/dev/vpu_hevc_enc_dev_mgr"
#endif


static char *pstrGVpuEncDevs[VENC_MAX_CNT] = {
	"/dev/vpu_venc",
	"/dev/vpu_venc_ext",
	"/dev/vpu_venc_ext2",
	"/dev/vpu_venc_ext3",
	"/dev/vpu_venc_ext4",
	"/dev/vpu_venc_ext5",
	"/dev/vpu_venc_ext6",
	"/dev/vpu_venc_ext7",
	"/dev/vpu_venc_ext8",
	"/dev/vpu_venc_ext9",
	"/dev/vpu_venc_ext10",
	"/dev/vpu_venc_ext11",
	"/dev/vpu_venc_ext12",
	"/dev/vpu_venc_ext13",
	"/dev/vpu_venc_ext14",
	"/dev/vpu_venc_ext15"
};


int vpu_enc_get_driverfd(const VEPC_DRIVER_INFO_T *ptDriverInfo, VEPC_DRIVER_TYPE_E eDriverType)
{
	int iDriverFd = -1;

	TEPV_FUNC_LOG();

	if (eDriverType == (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU) {
		iDriverFd = ptDriverInfo->tDriverFD.iVpuFd;
	} else if (eDriverType == (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_MGR) {
		iDriverFd = ptDriverInfo->tDriverFD.iMgrFd;
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] Driver: %d \n", __func__, __LINE__, (int)eDriverType);
		iDriverFd = -1;
	}

	return iDriverFd;
}


int vpu_enc_get_cmd_result(const VEPC_DRIVER_INFO_T *ptDriverInfo, VEPC_DRIVER_TYPE_E eDriverType, VEPC_COMMAND_E eCmd, void *pvParam)
{
	int iRetVal = -1;

	TEPV_FUNC_LOG();

	(void)eDriverType;

	if (ptDriverInfo != NULL) {
		switch ((int)eCmd) {
			case (int)VEPC_COMMAND_V_ENC_INIT: {
				if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
					#if defined(TCC_VPU_4K_E1_INCLUDE)
					VEPK_HEVC_INIT_t *ptInit = NULL;
					TEPV_CAST(ptInit, pvParam);
					iRetVal = ptInit->result;
					#endif
				} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
					#if defined(TCC_JPU_C6_INCLUDE)
					VEPK_JENC_INIT_t *ptInit = NULL;
					TEPV_CAST(ptInit, pvParam);
					iRetVal = ptInit->result;
					#endif
				} else {
					VEPK_VENC_INIT_t *ptInit = NULL;
					TEPV_CAST(ptInit, pvParam);
					iRetVal = ptInit->result;
				}
				TEPV_DETAIL_MSG("[%s(%d)] VEPC_COMMAND_V_ENC_INIT: %d \n", __func__, __LINE__, iRetVal);
				break;
			}

			case (int)VEPC_COMMAND_V_ENC_PUT_HEADER: {
				if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
					#if defined(TCC_VPU_4K_E1_INCLUDE)
					VEPK_HEVC_PUT_HEADER_t *ptHeader = NULL;
					TEPV_CAST(ptHeader, pvParam);
					iRetVal = ptHeader->result;
					#endif
				} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H264) {
					VEPK_VENC_PUT_HEADER_t *ptHeader = NULL;
					TEPV_CAST(ptHeader, pvParam);
					iRetVal = ptHeader->result;
				} else {
					TEPV_DONOTHING();
				}
				TEPV_DETAIL_MSG("[%s(%d)] VEPC_COMMAND_V_ENC_PUT_HEADER: %d \n", __func__, __LINE__, iRetVal);
				break;
			}

			case (int)VEPC_COMMAND_V_ENC_ENCODE: {
				if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
					#if defined(TCC_VPU_4K_E1_INCLUDE)
					VEPK_HEVC_ENCODE_t *ptEncode = NULL;
					TEPV_CAST(ptEncode, pvParam);
					iRetVal = ptEncode->result;
					#endif
				} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
					#if defined(TCC_JPU_C6_INCLUDE)
					JPU_ENCODE_t *ptEncode = NULL;
					TEPV_CAST(ptEncode, pvParam);
					iRetVal = ptEncode->result;
					#endif
				} else {
					VEPK_VENC_ENCODE_t *ptEncode = NULL;
					TEPV_CAST(ptEncode, pvParam);
					iRetVal = ptEncode->result;
				}
				TEPV_DETAIL_MSG("[%s(%d)] VEPC_COMMAND_V_ENC_ENCODE: %d \n", __func__, __LINE__, iRetVal);
				break;
			}

			default: {
				TEPV_DONOTHING(0);
				break;
			}
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	return iRetVal;
}

void *vpu_enc_get_cmd_param(VEPC_DRIVER_INFO_T *ptDriverInfo, VEPC_COMMAND_E eCmd, void *pvParam)
{
	void *pvCmdParam = NULL;

	TEPV_FUNC_LOG();

	TEPV_CHECK_INT_LOG((int)eCmd);

	if (pvParam == NULL) {
		if (eCmd == (VEPC_COMMAND_E)VEPC_COMMAND_V_ENC_INIT) {
			TEPV_CHECK_STR_LOG("VEPC_COMMAND_V_ENC_INIT");
			if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
				#if defined(TCC_VPU_4K_E1_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tHEVC_INIT;
				#endif
			} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
				#if defined(TCC_JPU_C6_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tJENC_INIT;
				#endif
			} else {
				pvCmdParam = (void *)&ptDriverInfo->tVENC_INIT;
			}
		} else if (eCmd == (VEPC_COMMAND_E)VEPC_COMMAND_V_ENC_ALLOC_MEMORY) {
			TEPV_CHECK_STR_LOG("VEPC_COMMAND_V_ENC_ALLOC_MEMORY");
			pvCmdParam = (void *)&ptDriverInfo->tALLOC_MEM;
		} else if (eCmd == (VEPC_COMMAND_E)VEPC_COMMAND_V_ENC_REG_FRAME_BUFFER) {
			TEPV_CHECK_STR_LOG("VEPC_COMMAND_V_ENC_REG_FRAME_BUFFER");
			if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
				#if defined(TCC_VPU_4K_E1_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tHEVC_SET_BUFFER;
				#endif
			} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H264) {
				pvCmdParam = (void *)&ptDriverInfo->tVENC_SET_BUFFER;
			} else {
				TEPV_DONOTHING();
			}
		} else if (eCmd == (VEPC_COMMAND_E)VEPC_COMMAND_V_ENC_PUT_HEADER) {
			TEPV_CHECK_STR_LOG("VEPC_COMMAND_V_ENC_PUT_HEADER");
			if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
				#if defined(TCC_VPU_4K_E1_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tHEVC_PUT_HEADER;
				#endif
			} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H264) {
				pvCmdParam = (void *)&ptDriverInfo->tVENC_PUT_HEADER;
			} else {
				TEPV_DONOTHING();
			}
		} else if (eCmd == (VEPC_COMMAND_E)VEPC_COMMAND_V_ENC_CLOSE) {
			TEPV_CHECK_STR_LOG("VEPC_COMMAND_V_ENC_CLOSE");
			if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
				#if defined(TCC_VPU_4K_E1_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tHEVC_ENCODE;
				#endif
			} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
				#if defined(TCC_JPU_C6_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tJENC_ENCODE;
				#endif
			} else {
				pvCmdParam = (void *)&ptDriverInfo->tVENC_ENCODE;
			}
		} else if (eCmd == (VEPC_COMMAND_E)VEPC_COMMAND_V_ENC_ENCODE) {
			TEPV_CHECK_STR_LOG("VEPC_COMMAND_V_ENC_ENCODE");
			if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
				#if defined(TCC_VPU_4K_E1_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tHEVC_ENCODE;
				#endif
			} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
				#if defined(TCC_JPU_C6_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tJENC_ENCODE;
				#endif
			} else {
				pvCmdParam = (void *)&ptDriverInfo->tVENC_ENCODE;
			}
		} else {
			TEPV_CHECK_STR_LOG("NULL");
			pvCmdParam = NULL;
		}
	} else {
		pvCmdParam = pvParam;
	}

	return pvCmdParam;
}

int vpu_enc_translate_cmd(VEPC_COMMAND_E eCmd, unsigned int *puiCmd, char *pstrCmd)
{
	int ret = 0;

	TEPV_FUNC_LOG();

	if ((puiCmd != NULL) && (pstrCmd != NULL)) {
		if ((int)eCmd == (int)VEPC_COMMAND_V_ENC_INIT) {
			*puiCmd = VEPK_V_ENC_INIT;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_V_ENC_INIT");
		} else if ((int)eCmd == (int)VEPC_COMMAND_V_ENC_INIT_RESULT) {
			*puiCmd = VEPK_V_ENC_INIT_RESULT;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_V_ENC_INIT_RESULT");
		} else if ((int)eCmd == (int)VEPC_COMMAND_VPU_GET_INSTANCE_IDX) {
			*puiCmd = VEPK_VPU_GET_INSTANCE_IDX;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_VPU_GET_INSTANCE_IDX");
		} else if ((int)eCmd == (int)VEPC_COMMAND_VPU_CLEAR_INSTANCE_IDX) {
			*puiCmd = VEPK_VPU_CLEAR_INSTANCE_IDX;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_VPU_CLEAR_INSTANCE_IDX");
		} else if ((int)eCmd == (int)VEPC_COMMAND_DEVICE_INITIALIZE) {
			*puiCmd = VEPK_DEVICE_INITIALIZE;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_DEVICE_INITIALIZE");
		} else if ((int)eCmd == (int)VEPC_COMMAND_VPU_SET_CLK) {
			*puiCmd = VEPK_VPU_SET_CLK;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_VPU_SET_CLK");
		} else if ((int)eCmd == (int)VEPC_COMMAND_V_ENC_ALLOC_MEMORY) {
			*puiCmd = VEPK_V_ENC_ALLOC_MEMORY;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_V_ENC_ALLOC_MEMORY");
		} else if ((int)eCmd == (int)VEPC_COMMAND_V_ENC_FREE_MEMORY) {
			*puiCmd = VEPK_V_ENC_FREE_MEMORY;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_V_ENC_FREE_MEMORY");
		} else if ((int)eCmd == (int)VEPC_COMMAND_V_ENC_REG_FRAME_BUFFER) {
			*puiCmd = VEPK_V_ENC_REG_FRAME_BUFFER;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_V_ENC_REG_FRAME_BUFFER");
		} else if ((int)eCmd == (int)VEPC_COMMAND_V_ENC_PUT_HEADER) {
			*puiCmd = VEPK_V_ENC_PUT_HEADER;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_V_ENC_PUT_HEADER");
		} else if ((int)eCmd == (int)VEPC_COMMAND_V_ENC_PUT_HEADER_RESULT) {
			*puiCmd = VEPK_V_ENC_PUT_HEADER_RESULT;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_V_ENC_PUT_HEADER_RESULT");
		} else if ((int)eCmd == (int)VEPC_COMMAND_V_ENC_GENERAL_RESULT) {
			*puiCmd = VEPK_V_ENC_GENERAL_RESULT;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_V_ENC_GENERAL_RESULT");
		} else if ((int)eCmd == (int)VEPC_COMMAND_V_ENC_CLOSE) {
			*puiCmd = VEPK_V_ENC_CLOSE;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_V_ENC_CLOSE");
		} else if ((int)eCmd == (int)VEPC_COMMAND_V_ENC_ENCODE) {
			*puiCmd = VEPK_V_ENC_ENCODE;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_V_ENC_ENCODE");
		} else if ((int)eCmd == (int)VEPC_COMMAND_V_ENC_ENCODE_RESULT) {
			*puiCmd = VEPK_V_ENC_ENCODE_RESULT;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "VEPK_V_ENC_ENCODE_RESULT");
		} else {
			ret = -1;
			TEPV_SNPRINT(pstrCmd, TEPV_CMD_STR_MAX, "UNKNOWN_COMMAND");
			TEPV_ERROR_MSG("[%s(%d)] [ERROR] UNKNOWN_COMMAND \n", __func__, __LINE__);
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] NULL \n", __func__, __LINE__);
	}

	return ret;
}

EP_Video_Error_E vpu_enc_open_mgr(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	EP_Video_Error_E ret = (EP_Video_Error_E)EP_Video_ErrorNone;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const char *pstrMgrName = NULL;

		if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H265) {
			#if defined(TCC_VPU_4K_E1_INCLUDE)
			pstrMgrName = VPU_ENC_HEVC_MGR;
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			pstrMgrName = JPU_ENC_MGR;
			#endif
		}  else {
			pstrMgrName = VPU_ENC_MGR;
		}

		ptDriverInfo->tDriverFD.iMgrFd = -1;
		ptDriverInfo->tDriverFD.iMgrFd = open(pstrMgrName, O_RDWR);

		if (ptDriverInfo->tDriverFD.iMgrFd < 0) {
			TEPV_ERROR_MSG("[%s(%d)] [ERROR] open(): %s \n", __func__, __LINE__, pstrMgrName);
			ret = (EP_Video_Error_E)EP_Video_ErrorInit;
		} else {
			VEPK_INSTANCE_INFO_t tInstInfo = {0, };

			tInstInfo.type 			= ptDriverInfo->iInstType;
			tInstInfo.nInstance 	= ptDriverInfo->iNumOfInst;
			(void)tInstInfo.type;
			(void)tInstInfo.nInstance;

			if (video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_MGR, VEPC_COMMAND_VPU_GET_INSTANCE_IDX, &tInstInfo) < 0) {
				TEPV_ERROR_MSG("[%s(%d)] [ERROR]VEPC_COMMAND_VPU_GET_INSTANCE_IDX \n", __func__, __LINE__);
				ret = (EP_Video_Error_E)EP_Video_ErrorInit;
			}

			if (tInstInfo.nInstance < 0) {
				ret = (EP_Video_Error_E)EP_Video_ErrorInit;
			} else {
				ptDriverInfo->iNumOfInst = tInstInfo.nInstance;
				ptDriverInfo->iInstIdx = ptDriverInfo->iNumOfInst;
			}
		}
	} else {
		ret = (EP_Video_Error_E)EP_Video_ErrorInit;
	}

	return ret;
}

EP_Video_Error_E vpu_enc_open_vpu(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	EP_Video_Error_E ret = (EP_Video_Error_E)EP_Video_ErrorNone;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->iInstIdx >= 0) {
			ptDriverInfo->tDriverFD.iVpuFd = open(pstrGVpuEncDevs[ptDriverInfo->iInstIdx], O_RDWR);

			if (ptDriverInfo->tDriverFD.iVpuFd < 0) {
				TEPV_ERROR_MSG("[%s(%d)] [ERROR] open(): %s  \n", __func__, __LINE__, pstrGVpuEncDevs[ptDriverInfo->iInstIdx]);
				ret = (EP_Video_Error_E)EP_Video_ErrorInit;
			}

			if (video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_DEVICE_INITIALIZE, &ptDriverInfo->uiCodec) < 0) {
				ret = (EP_Video_Error_E)EP_Video_ErrorInit;
			}
		} else {
			ret = (EP_Video_Error_E)EP_Video_ErrorInit;
		}
	} else {
		ret = (EP_Video_Error_E)EP_Video_ErrorInit;
	}

	return ret;
}

EP_Video_Error_E vpu_enc_close_mgr(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	EP_Video_Error_E ret = (EP_Video_Error_E)EP_Video_ErrorNone;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->tDriverFD.iMgrFd >= 0) {
			VEPK_INSTANCE_INFO_t tInstInfo = {0, };

			tInstInfo.type 			= ptDriverInfo->iInstType;
			tInstInfo.nInstance 	= ptDriverInfo->iNumOfInst;
			(void)tInstInfo.type;
			(void)tInstInfo.nInstance;

			(void)video_encode_plugin_cmd(ptDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_MGR, VEPC_COMMAND_VPU_CLEAR_INSTANCE_IDX, &tInstInfo);

			ptDriverInfo->iNumOfInst = -1;

			(void)close(ptDriverInfo->tDriverFD.iMgrFd);
			ptDriverInfo->tDriverFD.iMgrFd = -1;
		}
	} else {
		ret = (EP_Video_Error_E)EP_Video_ErrorInit;
	}

	return ret;
}

EP_Video_Error_E vpu_enc_close_vpu(VEPC_DRIVER_INFO_T *ptDriverInfo)
{
	EP_Video_Error_E ret = (EP_Video_Error_E)EP_Video_ErrorNone;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->tDriverFD.iVpuFd >= 0) {
			(void)close(ptDriverInfo->tDriverFD.iVpuFd);
			ptDriverInfo->tDriverFD.iVpuFd = -1;
		}
	} else {
		ret = (EP_Video_Error_E)EP_Video_ErrorInit;
	}

	return ret;
}

unsigned char *vpu_enc_get_virtaddr(unsigned char *pucBitStreamBufAddr[VEPK_K_VA + 1], codec_addr_t convert_addr, uint32_t base_index)
{
	unsigned char *pRetAddr = NULL;
	unsigned char *pTargetBaseAddr = NULL;

	uintptr_t pBaseAddr;
	uintptr_t szAddrGap = 0;

	pTargetBaseAddr = pucBitStreamBufAddr[VEPK_VA];

	if (base_index == (uint32_t)VEPK_K_VA) {
		pBaseAddr = (uintptr_t)pucBitStreamBufAddr[VEPK_K_VA];
	} else {
		pBaseAddr = (uintptr_t)pucBitStreamBufAddr[VEPK_PA];
	}

	if ((uintptr_t)convert_addr >= (pBaseAddr)) {
		szAddrGap = (uintptr_t)convert_addr - pBaseAddr;
	}

	pRetAddr = pTargetBaseAddr + szAddrGap;

	return (pRetAddr);
}

void vpu_enc_dump_data(int width, int height, int size, unsigned char *pucInput)
{
	#if (VENC_DUMP_ON == 1)
	static int dumpCnt = 0;

	FILE *pFs = NULL;
	char fname[TEPV_FILENAME_STR_MAX];

	TEPV_SNPRINT(fname, TEPV_FILENAME_STR_MAX, "dump_enc_input_%dx%d_%02d.yuv", width, height, dumpCnt);
	TEPV_PRINT("[%s(%d)] DUMP: %s \n", __func__, __LINE__, fname);

	pFs = fopen(fname, "wb");
	if (pFs == NULL) {
		TEPV_ERROR_MSG("[%s(%d)] ERROR: %s \n", __func__, __LINE__, fname);
	} else {
		fwrite(pucInput, 1, size, pFs);

		fclose(pFs);
	}

	dumpCnt++;
	#else
	(void)width;
	(void)height;
	(void)size;
	(void)pucInput;
	#endif
}

void vpu_enc_dump_result_frame(VEP_DUMP_DISP_INFO_T *ptDumpInfo)
{
	#if (VENC_DUMP_ON == 1)
	static int dumpCnt = 0;

	if ((ptDumpInfo != NULL) && (dumpCnt >= 0)) {
		if (dumpCnt < ptDumpInfo->iDumpMax) {
			FILE *pFs = NULL;
			char fname[TEPV_FILENAME_STR_MAX];

			TEPV_SNPRINT(fname, TEPV_FILENAME_STR_MAX, "dump_enc_%u_%dx%d_%02d.yuv", ptDumpInfo->uiCodec, ptDumpInfo->iWidth, ptDumpInfo->iHeight, dumpCnt);
			TEPV_PRINT("[%s(%d)] DUMP: %s \n", __func__, __LINE__, fname);

			pFs = fopen(fname, "wb");
			if (pFs == NULL) {
				TEPV_ERROR_MSG("[%s(%d)] ERROR: %s \n", __func__, __LINE__, fname);
			} else {
				int resolution = ptDumpInfo->iWidth * ptDumpInfo->iHeight;
				int dumpSize = 0;

				if (ptDumpInfo->pucY != NULL) {
					fwrite(ptDumpInfo->pucY, 1, resolution, pFs);
				}

				if (ptDumpInfo->bInterleaveMode == true) {
					dumpSize = resolution / 4;
					if (ptDumpInfo->pucU != NULL) {
						fwrite(ptDumpInfo->pucU, 1, dumpSize, pFs);
					}

					if (ptDumpInfo->pucV != NULL) {
						fwrite(ptDumpInfo->pucV, 1, dumpSize, pFs);
					}
				} else {
					dumpSize = resolution / 2;
					if (ptDumpInfo->pucU != NULL) {
						fwrite(ptDumpInfo->pucU, 1, dumpSize, pFs);
					}
				}

				fclose(pFs);
			}
		}
	}

	dumpCnt++;
	#else
	(void)ptDumpInfo;
	#endif
}

#if 0
int vpu_enc_memcpy(char *pcDest, char *pcSource, int iSize)
{
	int iCnt = 0;

	while (iSize--) {
		*pcDest++ = *pcSource++;
		iCnt++;

		if (iSize == 0) {
			break;
		}
	}

	return iCnt;
}
#endif