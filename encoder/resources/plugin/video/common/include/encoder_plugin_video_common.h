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

#ifndef VIDEO_ENCODER_PLUGIN_COMMON_H
#define VIDEO_ENCODER_PLUGIN_COMMON_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/mman.h>


#if defined(USE_COMMON_KERNEL_LOCATION)

#if defined(TCC_VPU_D6_INCLUDE)
#include <TCC_VPU_D6.h>
#else
#include <TCC_VPU_C7_CODEC.h>
#endif
#if defined(TCC_HEVC_INCLUDE)
#include <TCC_HEVC_CODEC.h>
#endif
#if defined(TCC_JPU_C6_INCLUDE)
#include <TCC_JPU_C6.h>
#endif
#if defined(TCC_VPU_4K_D2_INCLUDE)
#include <TCC_VPU_4K_D2_CODEC.h>
#endif
#if defined(TCC_VP9_INCLUDE)
#include <TCC_VP9_CODEC.h>
#endif

#include <tcc_video_private.h>

#include <tcc_vpu_ioctl.h>
#if defined(TCC_JPU_C6_INCLUDE)
#include <tcc_jpu_ioctl.h>
#endif

#if defined(TCC_VPU_4K_E1_INCLUDE)
#include "TCC_VPU_HEVC_ENC.h"
#include "tcc_vpu_hevc_enc_ioctl.h"
#endif

#else // USE_COMMON_KERNEL_LOCATION: use chipset folder

#if defined(TCC_VPU_D6_INCLUDE)
#include <mach/TCC_VPU_D6.h>
#else
#include <mach/TCC_VPU_C7_CODEC.h>
#endif
#if defined(TCC_HEVC_INCLUDE)
#include <mach/TCC_HEVC_CODEC.h>
#endif
#if defined(TCC_JPU_C6_INCLUDE)
#include <mach/TCC_JPU_C6.h>
#endif
#include <mach/tcc_video_private.h>

#include <mach/tcc_vpu_ioctl.h>
#if defined(TCC_JPU_C6_INCLUDE)
#include <mach/tcc_jpu_ioctl.h>
#endif
#if defined(TCC_HEVC_INCLUDE)
#include <mach/tcc_hevc_ioctl.h>
#endif

#if defined(TCC_VPU_4K_E1_INCLUDE)
#include <mach/TCC_VPU_HEVC_ENC.h>
#include <mach/tcc_vpu_hevc_enc_ioctl.h>
#endif

#endif // USE_COMMON_KERNEL_LOCATION

#include <tcc_video_common.h>



// --------------------------------------------------------------------------------------
// translate driver defines and types
// --------------------------------------------------------------------------------------
#define VEPK_PA									(PA) // 0
#define VEPK_VA									(VA) // 1
#define VEPK_K_VA								(2)

#define VEPK_COMP_Y 							(COMP_Y) // 0
#define VEPK_COMP_U 							(COMP_U) // 1
#define VEPK_COMP_V 							(COMP_V) // 2

#define VEPK_PIC_TYPE_I 						(PIC_TYPE_I)
#define VEPK_PIC_TYPE_P 						(PIC_TYPE_P)

#define VEPK_STD_MJPG							STD_MJPG
#define VEPK_STD_MPEG4							STD_MPEG4
#define VEPK_STD_H264							STD_AVC
#define VEPK_STD_H265							STD_HEVC_ENC

#define VEPK_AVC_SPS_RBSP						AVC_SPS_RBSP
#define VEPK_AVC_PPS_RBSP 						AVC_PPS_RBSP
#define VEPK_MPEG4_VOS_HEADER					MPEG4_VOS_HEADER
#define VEPK_MPEG4_VIS_HEADER					MPEG4_VIS_HEADER

#if defined(TCC_VPU_4K_E1_INCLUDE)
#define VEPK_HEVC_ENC_CODEOPT_ENC_VPS 			HEVC_ENC_CODEOPT_ENC_VPS
#define VEPK_HEVC_ENC_CODEOPT_ENC_SPS 			HEVC_ENC_CODEOPT_ENC_SPS
#define VEPK_HEVC_ENC_CODEOPT_ENC_PPS 			HEVC_ENC_CODEOPT_ENC_PPS
#endif

#define VEPK_LARGE_STREAM_BUF_SIZE 				LARGE_STREAM_BUF_SIZE
#define VEPK_WORK_CODE_PARA_BUF_SIZE			WORK_CODE_PARA_BUF_SIZE
#define VEPK_VPU_HEVC_ENC_WORK_CODE_BUF_SIZE	VPU_HEVC_ENC_WORK_CODE_BUF_SIZE

#define VEPK_BUFFER_TYPE 						Buffer_Type
#define VEPK_BUFFER_TYPE_MIN 					BUFFER_ELSE
#define VEPK_BUFFER_TYPE_MAX 					BUFFER_USERDATA
#define VEPK_BUFFER_WORK 						BUFFER_WORK
#define VEPK_BUFFER_SEQHEADER 					BUFFER_SEQHEADER
#define VEPK_BUFFER_ELSE 						BUFFER_ELSE

#define VEPK_VPU_ENC							VPU_ENC

#define VEPK_RETCODE_SUCCESS					RETCODE_SUCCESS
#define VEPK_RETCODE_CODEC_EXIT					RETCODE_CODEC_EXIT

// tcc_video_common.h
#define VEPK_DEVICE_INITIALIZE					DEVICE_INITIALIZE

#define VEPK_V_ENC_INIT							V_ENC_INIT
#define VEPK_V_ENC_INIT_RESULT					V_ENC_INIT_RESULT
#define VEPK_V_ENC_CLOSE						V_ENC_CLOSE
#define VEPK_V_ENC_ALLOC_MEMORY 				V_ENC_ALLOC_MEMORY
#define VEPK_V_ENC_FREE_MEMORY 					V_ENC_FREE_MEMORY
#define VEPK_V_ENC_REG_FRAME_BUFFER				V_ENC_REG_FRAME_BUFFER
#define VEPK_V_ENC_PUT_HEADER					V_ENC_PUT_HEADER
#define VEPK_V_ENC_PUT_HEADER_RESULT			V_ENC_PUT_HEADER_RESULT
#define VEPK_V_ENC_ENCODE						V_ENC_ENCODE
#define VEPK_V_ENC_ENCODE_RESULT 				V_ENC_ENCODE_RESULT
#define VEPK_V_ENC_GENERAL_RESULT				V_ENC_GENERAL_RESULT

#define VEPK_VPU_GET_INSTANCE_IDX 				VPU_GET_INSTANCE_IDX
#define VEPK_VPU_CLEAR_INSTANCE_IDX 			VPU_CLEAR_INSTANCE_IDX
#define VEPK_VPU_SET_CLK 						VPU_SET_CLK

typedef VENC_INIT_t 							VEPK_VENC_INIT_t;
typedef MEM_ALLOC_INFO_t 						VEPK_MEM_ALLOC_INFO_t;
typedef VENC_SET_BUFFER_t						VEPK_VENC_SET_BUFFER_t;
typedef VENC_PUT_HEADER_t						VEPK_VENC_PUT_HEADER_t;
typedef VENC_ENCODE_t 							VEPK_VENC_ENCODE_t;

#if defined(TCC_VPU_4K_E1_INCLUDE)
typedef VENC_HEVC_INIT_t 						VEPK_HEVC_INIT_t;
typedef VENC_HEVC_SET_BUFFER_t					VEPK_HEVC_SET_BUFFER_t;
typedef VENC_HEVC_PUT_HEADER_t					VEPK_HEVC_PUT_HEADER_t;
typedef VENC_HEVC_ENCODE_t 						VEPK_HEVC_ENCODE_t;
#endif

#if defined(TCC_JPU_C6_INCLUDE)
typedef JENC_INIT_t 							VEPK_JENC_INIT_t;
typedef JPU_ENCODE_t 							VEPK_JPU_ENCODE_t;
#endif

// tcc_video_common.h
typedef INSTANCE_INFO 							VEPK_INSTANCE_INFO_t;
typedef CONTENTS_INFO							VEPK_CONTENTS_INFO_t;


typedef enum
{
	VEPC_DRIVER_TYPE_NONE = 0,
	VEPC_DRIVER_TYPE_VPU,
	VEPC_DRIVER_TYPE_MGR,

	VEPC_DRIVER_TYPE_MAX
} VEPC_DRIVER_TYPE_E;

typedef enum
{
	VEPC_FRAME_TYPE_UNKNOWN = 0,
	VEPC_FRAME_TYPE_I,
	VEPC_FRAME_TYPE_P,

	VEPC_FRAME_TYPE_MAX
} VEPC_FRAME_TYPE_E;
typedef enum
{
	VEPC_DRIVER_POLL_ERROR_NONE = 0,

	VEPC_DRIVER_POLL_ERROR_UNKNOWN,
	VEPC_DRIVER_POLL_ERROR_RET,
	VEPC_DRIVER_POLL_ERROR_TIMEOUT,
	VEPC_DRIVER_POLL_ERROR_POLLERR,

	VEPC_DRIVER_POLL_ERROR_MAX
} VEPC_DRIVER_POLL_ERROR_E;

typedef enum
{
	VEPC_COMMAND_None 							= 0,

	VEPC_COMMAND_V_ENC_INIT 					= 101,
	VEPC_COMMAND_V_ENC_CLOSE,
	VEPC_COMMAND_V_ENC_ALLOC_MEMORY,
	VEPC_COMMAND_V_ENC_FREE_MEMORY,
	VEPC_COMMAND_V_ENC_REG_FRAME_BUFFER,
	VEPC_COMMAND_V_ENC_PUT_HEADER,
	VEPC_COMMAND_V_ENC_ENCODE,

	VEPC_COMMAND_V_GET_VPU_VERSION 				= 201,

	VEPC_COMMAND_VPU_GET_FREEMEM_SIZE 			= 301,
	VEPC_COMMAND_VPU_SET_MEM_ALLOC_MODE,
	VEPC_COMMAND_VPU_GET_INSTANCE_IDX,
	VEPC_COMMAND_VPU_CLEAR_INSTANCE_IDX,
	VEPC_COMMAND_VPU_SET_CLK,

	VEPC_COMMAND_DEVICE_INITIALIZE 				= 401,

	VEPC_COMMAND_V_ENC_INIT_RESULT 				= 1001,
	VEPC_COMMAND_V_ENC_PUT_HEADER_RESULT,
	VEPC_COMMAND_V_ENC_ENCODE_RESULT,
	VEPC_COMMAND_V_ENC_GENERAL_RESULT,
	VEPC_COMMAND_Max
} VEPC_COMMAND_E;

typedef struct
{
	int iMgrFd;
	int iVpuFd;
	int iMemFd;
} VEPC_DRIVER_FD_T;

typedef struct
{
	VEPC_DRIVER_FD_T tDriverFD;

	int iInstIdx;
	unsigned int uiCodec;

	int iInstType;
	int iNumOfInst;

	bool isSeqHeaderDone;

	int iWidth;
	int iHeight;
	int iFrameRate;
	int iTargetKbps;
	int iKeyInterval;
	int iAvcFastEncoding;
	int iInitialQp;
	int iMaxPQp;
	int iMaxIQp;
	int iMinPQp;
	int iMinIQp;
	int iDeblkDisable;
	int iVbvBufferSize;

	int iSliceMode;
	int iSliceSizeUnit;
	int iSliceSize;

	bool bNalStartCode;

	int iMinFrameBufferCount;
	int iMinFrameBufferSize;
	int iFrameBufferSize;
	unsigned char *pucFrameBuffer[VEPK_K_VA + 1];

	int iSeqHeaderBufferSize;
	unsigned char *pucSeqHeaderBuffer[VEPK_K_VA + 1];

	int iSeqHeaderBufferOutSize;
	unsigned char *pucSeqHeaderBufferOutVA;

	int iBitstreamFormat;
	int iBitstreamBufSize;
	unsigned char *pucBitstreamBuffer[VEPK_K_VA + 1];
	int iBitStreamOutSize;
	unsigned char *pucBitStreamOutBuffer;

	int iBitWorkBufSize;
	unsigned char *pucBitWorkBuffer[VEPK_K_VA + 1];

	int iMESearchBufSize;
	unsigned char *pucMESearchBuffer[VEPK_K_VA + 1];

	int iSliceBufSize;
	unsigned char *pucSliceBuffer[VEPK_K_VA + 1];

	int iRawImageBufSize;
	unsigned char *pucRawImageBuffer[VEPK_K_VA + 1];

	VEPC_FRAME_TYPE_E eFrameType;
	int iTotalFrameCount;
	int iTotalIFrameCount;
	int iTotalPFrameCount;

	// ============================================
	// Ref. Driver
	// --------------------------------------------
	VEPK_MEM_ALLOC_INFO_t tALLOC_MEM;

	VEPK_VENC_INIT_t tVENC_INIT;
	VEPK_VENC_PUT_HEADER_t tVENC_PUT_HEADER;
	VEPK_VENC_SET_BUFFER_t tVENC_SET_BUFFER;
	VEPK_VENC_ENCODE_t tVENC_ENCODE;

	#if defined(TCC_VPU_4K_E1_INCLUDE)
	VEPK_HEVC_INIT_t tHEVC_INIT;
	VEPK_HEVC_PUT_HEADER_t tHEVC_PUT_HEADER;
	VEPK_HEVC_SET_BUFFER_t tHEVC_SET_BUFFER;
	VEPK_HEVC_ENCODE_t tHEVC_ENCODE;
	#endif

	#if defined(TCC_JPU_C6_INCLUDE)
	VEPK_JENC_INIT_t tJENC_INIT;
	VEPK_JPU_ENCODE_t tJENC_ENCODE;
	#endif
	// ============================================
} VEPC_DRIVER_INFO_T;


int video_encode_plugin_cmd(VEPC_DRIVER_INFO_T *ptDriverInfo, VEPC_DRIVER_TYPE_E eDriverType, VEPC_COMMAND_E eCmd, void *pvParam);
int video_encode_plugin_poll_cmd(VEPC_DRIVER_INFO_T *ptDriverInfo, VEPC_DRIVER_TYPE_E eDriverType, VEPC_COMMAND_E eCmd, void *pvParam);

void video_encode_plugin_set_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo);
void video_encode_plugin_set_encode_info(VEPC_DRIVER_INFO_T *ptDriverInfo, EP_Video_Encode_Input_T *ptEncodeInput);

void video_encode_plugin_get_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo);
void video_encode_plugin_get_header_info(VEPC_DRIVER_INFO_T *ptDriverInfo);
void video_encode_plugin_get_encode_result_info(VEPC_DRIVER_INFO_T *ptDriverInfo);

int video_encode_plugin_process_seq_header(VEPC_DRIVER_INFO_T *ptDriverInfo, int iType);

int video_encode_plugin_alloc_bitstream_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo);
int video_encode_plugin_alloc_bitwork_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo);
int video_encode_plugin_alloc_me_search_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo);
int video_encode_plugin_alloc_slice_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo);
int video_encode_plugin_alloc_raw_image_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo);
int video_encode_plugin_alloc_frame_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo);
int video_encode_plugin_alloc_seq_header_buffer(VEPC_DRIVER_INFO_T *ptDriverInfo);

void video_encode_plugin_free_alloc_buffers(VEPC_DRIVER_INFO_T *ptDriverInfo);

EP_Video_Error_E video_encode_plugin_open_drivers(VEPC_DRIVER_INFO_T *ptDriverInfo);
EP_Video_Error_E video_encode_plugin_close_drivers(VEPC_DRIVER_INFO_T *ptDriverInfo);


#endif	// VIDEO_ENCODER_PLUGIN_COMMON_H