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

#include <decoder_plugin_video_ips.h>


#define VDEC_MAX_CNT (5)

#define VPU_MGR 			"/dev/vpu_dev_mgr"
#define VPU_MEM 			"/dev/vpu_mem"

#if defined(TCC_JPU_C6_INCLUDE)
#define JPU_MGR				"/dev/jpu_dev_mgr"
#endif

#ifdef TCC_HEVC_INCLUDE
#define VPU_HEVC_MGR	 	"/dev/hevc_dev_mgr"
#endif

#ifdef TCC_VPU_4K_D2_INCLUDE
#define VPU_4K_D2_MGR		"/dev/vpu_4k_d2_dev_mgr"
#endif


static char *pstrGVpuDevs[VDEC_MAX_CNT] = {
	"/dev/vpu_vdec",
	"/dev/vpu_vdec_ext",
	"/dev/vpu_vdec_ext2",
	"/dev/vpu_vdec_ext3",
	"/dev/vpu_vdec_ext4"
};



int vpu_get_driverfd(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_DRIVER_TYPE_E eDriverType)
{
	int iDriverFd = -1;

	DPV_FUNC_LOG();

	if (eDriverType == (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU) {
		iDriverFd = ptDriverInfo->tDriverFD.iVpuFd;
	} else if (eDriverType == (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_MGR) {
		iDriverFd = ptDriverInfo->tDriverFD.iMgrFd;
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] Driver: %d \n", __func__, __LINE__, (int)eDriverType);
		iDriverFd = -1;
	}

	return iDriverFd;
}

DP_Video_Error_E vpu_open_mgr(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DP_Video_Error_E ret = (DP_Video_Error_E)DP_Video_ErrorNone;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		const char *pstrMgrName = NULL;

		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			pstrMgrName = VPU_HEVC_MGR;
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			pstrMgrName = VPU_4K_D2_MGR;
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			pstrMgrName = JPU_MGR;
			#endif
		}  else {
			pstrMgrName = VPU_MGR;
		}

		ptDriverInfo->tDriverFD.iMgrFd = -1;
		ptDriverInfo->tDriverFD.iMgrFd = open(pstrMgrName, O_RDWR);

		if (ptDriverInfo->tDriverFD.iMgrFd < 0) {
			DPV_ERROR_MSG("[%s(%d)] [ERROR] open(): %s \n", __func__, __LINE__, pstrMgrName);
			ret = (DP_Video_Error_E)DP_Video_ErrorInit;
		} else {
			VPK_INSTANCE_INFO_t tInstInfo = {0, };

			tInstInfo.type 			= ptDriverInfo->iInstType;
			tInstInfo.nInstance 	= ptDriverInfo->iNumOfInst;
			(void)tInstInfo.type;
			(void)tInstInfo.nInstance;

			if (video_plugin_cmd(ptDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_MGR, VPC_COMMAND_VPU_GET_INSTANCE_IDX, &tInstInfo) < 0) {
				ret = (DP_Video_Error_E)DP_Video_ErrorInit;
			}

			if (tInstInfo.nInstance < 0) {
				DPV_ERROR_MSG("[%s(%d)] [ERROR] VPU instance(): %d \n", __func__, __LINE__, tInstInfo.nInstance);
				ret = (DP_Video_Error_E)DP_Video_ErrorInit;
			} else {
				VPK_OPENED_sINFO_t tOpenInfo = {0, };

				ptDriverInfo->iNumOfInst = tInstInfo.nInstance;
				ptDriverInfo->iInstIdx = ptDriverInfo->iNumOfInst;

				if ((ptDriverInfo->iNumOfOpen >= 0) && ((ptDriverInfo->iOpenType >= (int)VPK_OPEN_TYPE_MIN) && (ptDriverInfo->iNumOfOpen <= (int)VPK_OPEN_TYPE_MAX))) {
					tOpenInfo.type 			= (VPK_OPEN_TYPE)ptDriverInfo->iOpenType;
					tOpenInfo.opened_cnt 	= (unsigned int)ptDriverInfo->iNumOfOpen;

					if (video_plugin_cmd(ptDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_MGR, VPC_COMMAND_VPU_SET_MEM_ALLOC_MODE, &tOpenInfo) < 0) {
						ret = (DP_Video_Error_E)DP_Video_ErrorInit;
					}
				}
			}
		}
	} else {
		ret = (DP_Video_Error_E)DP_Video_ErrorInit;
	}

	return ret;
}

DP_Video_Error_E vpu_open_vpu(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DP_Video_Error_E ret = (DP_Video_Error_E)DP_Video_ErrorNone;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->iInstIdx >= 0) {
			ptDriverInfo->tDriverFD.iVpuFd = open(pstrGVpuDevs[ptDriverInfo->iInstIdx], O_RDWR);

			if (ptDriverInfo->tDriverFD.iVpuFd < 0) {
				DPV_ERROR_MSG("[%s(%d)] [ERROR] open(): %s  \n", __func__, __LINE__, pstrGVpuDevs[ptDriverInfo->iInstIdx]);
				ret = (DP_Video_Error_E)DP_Video_ErrorInit;
			}

			if (video_plugin_cmd(ptDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_DEVICE_INITIALIZE, &ptDriverInfo->uiCodec) < 0) {
				ret = (DP_Video_Error_E)DP_Video_ErrorInit;
			}
		} else {
			ret = (DP_Video_Error_E)DP_Video_ErrorInit;
		}
	} else {
		ret = (DP_Video_Error_E)DP_Video_ErrorInit;
	}

	return ret;
}

DP_Video_Error_E vpu_open_mem(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DP_Video_Error_E ret = (DP_Video_Error_E)DP_Video_ErrorNone;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		ptDriverInfo->tDriverFD.iMemFd = open(VPU_MEM, O_RDWR);

		if (ptDriverInfo->tDriverFD.iMemFd < 0) {
			DPV_ERROR_MSG("[%s(%d)] [ERROR] open(): %s \n", __func__, __LINE__, VPU_MEM);
			ret = (DP_Video_Error_E)DP_Video_ErrorInit;
		}
	} else {
		ret = (DP_Video_Error_E)DP_Video_ErrorInit;
	}

	return ret;
}

DP_Video_Error_E vpu_close_mgr(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DP_Video_Error_E ret = (DP_Video_Error_E)DP_Video_ErrorNone;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->tDriverFD.iMgrFd >= 0) {
			VPK_INSTANCE_INFO_t tInstInfo = {0, };

			tInstInfo.type 			= ptDriverInfo->iInstType;
			tInstInfo.nInstance 	= ptDriverInfo->iNumOfInst;
			(void)tInstInfo.type;
			(void)tInstInfo.nInstance;

			(void)video_plugin_cmd(ptDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_MGR, VPC_COMMAND_VPU_CLEAR_INSTANCE_IDX, &tInstInfo);

			ptDriverInfo->iNumOfInst = -1;

			(void)close(ptDriverInfo->tDriverFD.iMgrFd);
			ptDriverInfo->tDriverFD.iMgrFd = -1;
		}
	} else {
		ret = (DP_Video_Error_E)DP_Video_ErrorInit;
	}

	return ret;
}

DP_Video_Error_E vpu_close_vpu(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DP_Video_Error_E ret = (DP_Video_Error_E)DP_Video_ErrorNone;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->tDriverFD.iVpuFd >= 0) {
			(void)close(ptDriverInfo->tDriverFD.iVpuFd);
			ptDriverInfo->tDriverFD.iVpuFd = -1;
		}
	} else {
		ret = (DP_Video_Error_E)DP_Video_ErrorInit;
	}

	return ret;
}

DP_Video_Error_E vpu_close_mem(VPC_DRIVER_INFO_T *ptDriverInfo)
{
	DP_Video_Error_E ret = (DP_Video_Error_E)DP_Video_ErrorNone;

	DPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->tDriverFD.iMemFd >= 0) {
			(void)close(ptDriverInfo->tDriverFD.iMemFd);
			ptDriverInfo->tDriverFD.iMemFd = -1;
		}
	} else {
		ret = (DP_Video_Error_E)DP_Video_ErrorInit;
	}

	return ret;
}

int vpu_get_cmd_result(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_DRIVER_TYPE_E eDriverType, VPC_COMMAND_E eCmd, void *pvParam)
{
	int iRetVal = 0;

	DPV_FUNC_LOG();

	(void)eDriverType;

	if (ptDriverInfo != NULL) {
		switch ((int)eCmd) {
			case (int)VPC_COMMAND_V_DEC_INIT: {
				if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
					#if defined(TCC_HEVC_INCLUDE)
					VPK_HEVC_INIT_t *ptInit = NULL;
					DPV_CAST(ptInit, pvParam);
					iRetVal = ptInit->result;
					#elif defined(TCC_VPU_4K_D2_INCLUDE)
					VPK_VPU_4K_D2_INIT_t *ptInit = NULL;
					DPV_CAST(ptInit, pvParam);
					iRetVal = ptInit->result;
					#endif
				} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
					#if defined(TCC_JPU_C6_INCLUDE)
					VPK_JDEC_INIT_t *ptInit = NULL;
					DPV_CAST(ptInit, pvParam);
					iRetVal = ptInit->result;
					#endif
				} else {
					VPK_VDEC_INIT_t *ptInit = NULL;
					DPV_CAST(ptInit, pvParam);
					iRetVal = ptInit->result;
				}
				break;
			}

			case (int)VPC_COMMAND_V_DEC_SEQ_HEADER: {
				if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
					#if defined(TCC_HEVC_INCLUDE)
					VPK_HEVC_SEQ_HEADER_t *ptSeqHeader = NULL;
					DPV_CAST(ptSeqHeader, pvParam);
					iRetVal = ptSeqHeader->result;
					#elif defined(TCC_VPU_4K_D2_INCLUDE)
					VPK_VPU_4K_D2_SEQ_HEADER_t *ptSeqHeader = NULL;
					DPV_CAST(ptSeqHeader, pvParam);
					iRetVal = ptSeqHeader->result;
					#endif
				} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
					#if defined(TCC_JPU_C6_INCLUDE)
					VPK_JDEC_SEQ_HEADER_t *ptSeqHeader = NULL;
					DPV_CAST(ptSeqHeader, pvParam);
					iRetVal = ptSeqHeader->result;
					#endif
				} else {
					VPK_VDEC_SEQ_HEADER_t *ptSeqHeader = NULL;
					DPV_CAST(ptSeqHeader, pvParam);
					iRetVal = ptSeqHeader->result;
				}
				break;
			}

			case (int)VPC_COMMAND_V_DEC_DECODE: {
				if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
					#if defined(TCC_HEVC_INCLUDE)
					VPK_HEVC_DECODE_t *ptDecoded = NULL;
					DPV_CAST(ptDecoded, pvParam);
					iRetVal = ptDecoded->result;
					#elif defined(TCC_VPU_4K_D2_INCLUDE)
					VPK_VPU_4K_D2_DECODE_t *ptDecoded = NULL;
					DPV_CAST(ptDecoded, pvParam);
					iRetVal = ptDecoded->result;
					#endif
				} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
					#if defined(TCC_JPU_C6_INCLUDE)
					VPK_JPU_DECODE_t *ptDecoded = NULL;
					DPV_CAST(ptDecoded, pvParam);
					iRetVal = ptDecoded->result;
					#endif
				} else {
					VPK_VDEC_DECODE_t *ptDecoded = NULL;
					DPV_CAST(ptDecoded, pvParam);
					iRetVal = ptDecoded->result;
				}
				break;
			}
			default: {
				DPV_DONOTHING(0);
				break;
			}
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] ptDriverInfo is NULL \n", __func__, __LINE__);
	}

	//vpu_check_return_value(eDriverType, eCmd, iRetVal);

	return iRetVal;
}

void *vpu_get_cmd_param(VPC_DRIVER_INFO_T *ptDriverInfo, VPC_COMMAND_E eCmd, void *pvParam)
{
	void *pvCmdParam = NULL;

	DPV_FUNC_LOG();

	DPV_CHECK_INT_LOG((int)eCmd);

	if (pvParam == NULL) {
		if (eCmd == (VPC_COMMAND_E)VPC_COMMAND_V_DEC_CLOSE) {
			DPV_CHECK_STR_LOG("VPC_COMMAND_V_DEC_CLOSE");
			if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
				#if defined(TCC_HEVC_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tHevcDecode;
				#elif defined(TCC_VPU_4K_D2_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tVpu4KD2Decode;
				#endif
			} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
				#if defined(TCC_JPU_C6_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tJdecDecode;
				#endif
			} else {
				pvCmdParam = (void *)&ptDriverInfo->tDecode;
			}
		} else if (eCmd == (VPC_COMMAND_E)VPC_COMMAND_V_DEC_DECODE) {
			DPV_CHECK_STR_LOG("VPC_COMMAND_V_DEC_DECODE");
			if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
				#if defined(TCC_HEVC_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tHevcDecode;
				#elif defined(TCC_VPU_4K_D2_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tVpu4KD2Decode;
				#endif
			} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
				#if defined(TCC_JPU_C6_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tJdecDecode;
				#endif
			} else {
				pvCmdParam = (void *)&ptDriverInfo->tDecode;
			}
		} else if (eCmd == (VPC_COMMAND_E)VPC_COMMAND_V_DEC_INIT) {
			DPV_CHECK_STR_LOG("VPC_COMMAND_V_DEC_INIT");
			if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
				#if defined(TCC_HEVC_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tHevcInit;
				#elif defined(TCC_VPU_4K_D2_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tVpu4KD2Init;
				#endif
			} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
				#if defined(TCC_JPU_C6_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tJdecInit;
				#endif
			} else {
				pvCmdParam = (void *)&ptDriverInfo->tInit;
			}
		} else if (eCmd == (VPC_COMMAND_E)VPC_COMMAND_V_DEC_ALLOC_MEMORY) {
			DPV_CHECK_STR_LOG("VPC_COMMAND_V_DEC_ALLOC_MEMORY");
			pvCmdParam = (void *)&ptDriverInfo->tAllocMem;
		} else if (eCmd == (VPC_COMMAND_E)VPC_COMMAND_V_DEC_SEQ_HEADER) {
			DPV_CHECK_STR_LOG("VPC_COMMAND_V_DEC_SEQ_HEADER");
			if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
				#if defined(TCC_HEVC_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tHevcSeqHeader;
				#elif defined(TCC_VPU_4K_D2_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tVpu4KD2SeqHeader;
				#endif
			} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
				#if defined(TCC_JPU_C6_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tJdecSeqHeader;
				#endif
			} else {
				pvCmdParam = (void *)&ptDriverInfo->tSeqHeader;
			}
		} else if (eCmd == (VPC_COMMAND_E)VPC_COMMAND_V_DEC_REG_FRAME_BUFFER) {
			DPV_CHECK_STR_LOG("VPC_COMMAND_V_DEC_REG_FRAME_BUFFER");
			if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
				#if defined(TCC_HEVC_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tHevcSetBuffer;
				#elif defined(TCC_VPU_4K_D2_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tVpu4KD2SetBuffer;
				#endif
			} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
				#if defined(TCC_JPU_C6_INCLUDE)
				pvCmdParam = (void *)&ptDriverInfo->tJdecSetBuffer;
				#endif
			} else {
				pvCmdParam = (void *)&ptDriverInfo->tSetBuffer;
			}
		} else {
			DPV_CHECK_STR_LOG("NULL");
			pvCmdParam = NULL;
		}
	} else {
		pvCmdParam = pvParam;
	}

	return pvCmdParam;
}

int vpu_translate_cmd(VPC_COMMAND_E eCmd, unsigned int *puiCmd, char *pstrCmd)
{
	int ret = 0;

	DPV_FUNC_LOG();

	if ((puiCmd != NULL) && (pstrCmd != NULL)) {
		if ((int)eCmd == (int)VPC_COMMAND_V_DEC_INIT) {
			*puiCmd = VPK_V_DEC_INIT;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_INIT");
		} else if ((int)eCmd == (int)VPC_COMMAND_DEVICE_INITIALIZE) {
			*puiCmd = VPK_DEVICE_INITIALIZE;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_DEVICE_INITIALIZE");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_CLOSE) {
			*puiCmd = VPK_V_DEC_CLOSE;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_CLOSE");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_SEQ_HEADER) {
			*puiCmd = VPK_V_DEC_SEQ_HEADER;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_SEQ_HEADER");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_SWRESET) {
			*puiCmd = VPK_V_DEC_SWRESET;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_SWRESET");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_DECODE) {
			*puiCmd = VPK_V_DEC_DECODE;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_DECODE");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_INIT_RESULT) {
			*puiCmd = VPK_V_DEC_INIT_RESULT;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_INIT_RESULT");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_SEQ_HEADER_RESULT) {
			*puiCmd = VPK_V_DEC_SEQ_HEADER_RESULT;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_SEQ_HEADER_RESULT");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_DECODE_RESULT) {
			*puiCmd = VPK_V_DEC_DECODE_RESULT;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_DECODE_RESULT");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_GENERAL_RESULT) {
			*puiCmd = VPK_V_DEC_GENERAL_RESULT;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_GENERAL_RESULT");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_REG_FRAME_BUFFER) {
			*puiCmd = VPK_V_DEC_REG_FRAME_BUFFER;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_REG_FRAME_BUFFER");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_GET_VPU_VERSION) {
			*puiCmd = VPK_V_GET_VPU_VERSION;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_GET_VPU_VERSION");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_GET_RING_BUFFER_STATUS) {
			*puiCmd = VPK_V_GET_RING_BUFFER_STATUS;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_GET_RING_BUFFER_STATUS");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_FILL_RING_BUFFER_AUTO) {
			*puiCmd = VPK_V_FILL_RING_BUFFER_AUTO;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_FILL_RING_BUFFER_AUTO");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_UPDATE_RINGBUF_WP) {
			*puiCmd = VPK_V_DEC_UPDATE_RINGBUF_WP;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_UPDATE_RINGBUF_WP");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_BUF_FLAG_CLEAR) {
			*puiCmd = VPK_V_DEC_BUF_FLAG_CLEAR;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_BUF_FLAG_CLEAR");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_FLUSH_OUTPUT) {
			*puiCmd = VPK_V_DEC_FLUSH_OUTPUT;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_FLUSH_OUTPUT");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_FREE_MEMORY) {
			*puiCmd = VPK_V_DEC_FREE_MEMORY;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_FREE_MEMORY");
		} else if ((int)eCmd == (int)VPC_COMMAND_VPU_GET_FREEMEM_SIZE) {
			*puiCmd = VPK_VPU_GET_FREEMEM_SIZE;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_VPU_GET_FREEMEM_SIZE");
		} else if ((int)eCmd == (int)VPC_COMMAND_VPU_SET_MEM_ALLOC_MODE) {
			*puiCmd = VPK_VPU_SET_MEM_ALLOC_MODE;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_VPU_SET_MEM_ALLOC_MODE");
		} else if ((int)eCmd == (int)VPC_COMMAND_V_DEC_ALLOC_MEMORY) {
			*puiCmd = VPK_V_DEC_ALLOC_MEMORY;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_V_DEC_ALLOC_MEMORY");
		} else if ((int)eCmd == (int)VPC_COMMAND_VPU_GET_INSTANCE_IDX) {
			*puiCmd = VPK_VPU_GET_INSTANCE_IDX;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_VPU_GET_INSTANCE_IDX");
		} else if ((int)eCmd == (int)VPC_COMMAND_VPU_CLEAR_INSTANCE_IDX) {
			*puiCmd = VPK_VPU_CLEAR_INSTANCE_IDX;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_VPU_CLEAR_INSTANCE_IDX");
		} else if ((int)eCmd == (int)VPC_COMMAND_VPU_SET_CLK) {
			*puiCmd = VPK_VPU_SET_CLK;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "VPK_VPU_SET_CLK");
		} else {
			ret = -1;
			DPV_SNPRINT(pstrCmd, DPV_CMD_STR_MAX, "UNKNOWN_COMMAND");
			DPV_ERROR_MSG("[%s(%d)] [ERROR] UNKNOWN_COMMAND \n", __func__, __LINE__);
		}
	} else {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] NULL \n", __func__, __LINE__);
	}

	return ret;
}

void vpu_set_skip_mode(VPC_DRIVER_INFO_T *ptDriverInfo, VPC_COMMAND_E eCmd)
{
	if (eCmd == VPC_COMMAND_V_DEC_DECODE) {
		if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_H265) {
			#if defined(TCC_HEVC_INCLUDE)
			VPK_HEVC_DECODE_t *ptDecode = &ptDriverInfo->tHevcDecode;
			const VPC_FRAME_SKIP_T *ptFrameSkip = &ptDriverInfo->tFrameSkip;

			ptDecode->gsHevcDecInput.m_iSkipFrameMode = VPC_VDEC_SKIP_FRAME_DISABLE;

			if (ptFrameSkip->iFrameSearchEnable != 0) {
				ptDecode->gsHevcDecInput.m_iSkipFrameMode = VPC_VDEC_SKIP_FRAME_EXCEPT_I;
				DPV_DETAIL_MSG("[%s(%d)] [HEVC-%d] I-Frame Search Mode Enabled. \n", __func__, __LINE__, ptDriverInfo->iInstIdx);
			} else {
				if (ptFrameSkip->iSkipFrameMode == VPC_VDEC_SKIP_FRAME_ONLY_B) {
					ptDecode->gsHevcDecInput.m_iSkipFrameMode = VPC_VDEC_SKIP_FRAME_ONLY_B;
				}
			}
			#elif defined(TCC_VPU_4K_D2_INCLUDE)
			VPK_VPU_4K_D2_DECODE_t *ptDecode = &ptDriverInfo->tVpu4KD2Decode;
			const VPC_FRAME_SKIP_T *ptFrameSkip = &ptDriverInfo->tFrameSkip;

			if (ptFrameSkip->iFrameSearchEnable != 0) {
				ptDecode->gsV4kd2DecInput.m_iSkipFrameMode = VPC_VDEC_SKIP_FRAME_EXCEPT_I;
				DPV_DETAIL_MSG("[%s(%d)] [4KD2-%d] I-Frame Search Mode Enabled. \n", __func__, __LINE__, ptDriverInfo->iInstIdx);
			} else {
				ptDecode->gsV4kd2DecInput.m_iSkipFrameMode = VPC_VDEC_SKIP_FRAME_DISABLE;
			}
			#endif
		} else if (ptDriverInfo->uiCodec == (unsigned int)VPK_STD_MJPG) {
			#if defined(TCC_JPU_C6_INCLUDE)
			DPV_DONOTHING(0);
			#endif
		} else {
			VPK_VDEC_DECODE_t*ptDecode = &ptDriverInfo->tDecode;
			const VPC_FRAME_SKIP_T *ptFrameSkip = &ptDriverInfo->tFrameSkip;

			ptDecode->gsVpuDecInput.m_iSkipFrameMode 		= ptFrameSkip->iSkipFrameMode;
			ptDecode->gsVpuDecInput.m_iFrameSearchEnable	= ptFrameSkip->iFrameSearchEnable;
			ptDecode->gsVpuDecInput.m_iSkipFrameNum 		= ptFrameSkip->iSkipFrameNum;
		}
	}
}

unsigned char *vpu_get_framebuf_virtaddr(unsigned char *pucFrameBufAddr[VPK_K_VA + 1], unsigned char *convert_addr, uint32_t base_index)
{
	unsigned char *pRetAddr = NULL;
	unsigned char *pTargetBaseAddr = NULL;

	uintptr_t pBaseAddr;
	uintptr_t szAddrGap = 0;

	pTargetBaseAddr = pucFrameBufAddr[VPK_VA];

	if (base_index == (uint32_t)VPK_K_VA) {
		pBaseAddr = (uintptr_t)pucFrameBufAddr[VPK_K_VA];
	} else {
		// default: PA
		pBaseAddr = (uintptr_t)pucFrameBufAddr[VPK_PA];
	}

	szAddrGap = (uintptr_t)convert_addr - pBaseAddr;

	pRetAddr = &pTargetBaseAddr[szAddrGap];

	return (pRetAddr);
}

void vpu_check_picture_type(int iPicType)
{
	if (iPicType == VPK_PIC_TYPE_I) {
		DPV_DEBUG_MSG("[%s(%d)] VPK_PIC_TYPE_I \n", __func__, __LINE__);
	} else if (iPicType == VPK_PIC_TYPE_P) {
		DPV_DEBUG_MSG("[%s(%d)] VPK_PIC_TYPE_P \n", __func__, __LINE__);
	} else if (iPicType == VPK_PIC_TYPE_B) {
		DPV_DEBUG_MSG("[%s(%d)] VPK_PIC_TYPE_B \n", __func__, __LINE__);
	} else if (iPicType == VPK_PIC_TYPE_IDR) {
		DPV_DEBUG_MSG("[%s(%d)] VPK_PIC_TYPE_IDR \n", __func__, __LINE__);
	} else if (iPicType == VPK_PIC_TYPE_B_PB) {
		DPV_DEBUG_MSG("[%s(%d)] VPK_PIC_TYPE_B_PB \n", __func__, __LINE__);
	} else {
		DPV_DEBUG_MSG("[%s(%d)] VPK_PIC_TYPE_UNKNOWN: %d \n", __func__, __LINE__, iPicType);
	}
}

void vpu_check_return_value(VPC_DRIVER_TYPE_E eDriverType, VPC_COMMAND_E eCmd, int iRetVal)
{
	if (iRetVal == (int)VPK_RETCODE_SUCCESS) {
		DPV_DEBUG_MSG("[%s(%d)] VPK_RETCODE_SUCCESS: %d, %d \n", __func__, __LINE__, (int)eDriverType, (int)eCmd);
	} else if (iRetVal == (int)VPK_RETCODE_CODEC_EXIT) {
		DPV_DEBUG_MSG("[%s(%d)] VPK_RETCODE_CODEC_EXIT: %d, %d \n", __func__, __LINE__, (int)eDriverType, (int)eCmd);
	} else if (iRetVal == (int)VPK_RETCODE_MULTI_CODEC_EXIT_TIMEOUT) {
		DPV_DEBUG_MSG("[%s(%d)] VPK_RETCODE_MULTI_CODEC_EXIT_TIMEOUT: %d, %d \n", __func__, __LINE__, (int)eDriverType, (int)eCmd);
	} else if (iRetVal == (int)VPK_RETCODE_CODEC_SPECOUT) {
		DPV_DEBUG_MSG("[%s(%d)] VPK_RETCODE_CODEC_SPECOUT: %d, %d \n", __func__, __LINE__, (int)eDriverType, (int)eCmd);
	} else if (iRetVal == (int)VPK_RETCODE_NOT_INITIALIZED) {
		DPV_DEBUG_MSG("[%s(%d)] VPK_RETCODE_NOT_INITIALIZED: %d, %d \n", __func__, __LINE__, (int)eDriverType, (int)eCmd);
	} else if (iRetVal == (int)VPK_RETCODE_INVALID_STRIDE) {
		DPV_DEBUG_MSG("[%s(%d)] VPK_RETCODE_INVALID_STRIDE: %d, %d \n", __func__, __LINE__, (int)eDriverType, (int)eCmd);
	} else if (iRetVal == (int)VPK_RETCODE_INVALID_COMMAND) {
		DPV_DEBUG_MSG("[%s(%d)] VPK_RETCODE_INVALID_COMMAND: %d, %d \n", __func__, __LINE__, (int)eDriverType, (int)eCmd);
	} else {
		DPV_DEBUG_MSG("[%s(%d)] UNKNOWN: %d, %d, %d\n", __func__, __LINE__, (int)eDriverType, (int)eCmd, iRetVal);
	}
}

void vpu_dump_disp_frame(VP_DUMP_DISP_INFO_T *ptDumpInfo)
{
	#if (VDEC_DUMP_ON == 1)
	static int dumpCnt = 0;

	if ((ptDumpInfo != NULL) && (dumpCnt >= 0)) {
		if (dumpCnt < ptDumpInfo->iDumpMax) {
			FILE *pFs = NULL;
			char fname[DPV_FILENAME_STR_MAX];

			DPV_SNPRINT(fname, DPV_FILENAME_STR_MAX, "dump_disp_%u_%dx%d_%02d.yuv", ptDumpInfo->uiCodec, ptDumpInfo->iWidth, ptDumpInfo->iHeight, dumpCnt);
			DPV_PRINT("[%s(%d)] DUMP: %s \n", __func__, __LINE__, fname);

			pFs = fopen(fname, "wb");
			if (pFs == NULL) {
				DPV_ERROR_MSG("[%s(%d)] ERROR: %s \n", __func__, __LINE__, fname);
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