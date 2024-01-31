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

#ifndef DECODER_PLUGIN_COMMON_H
#define DECODER_PLUGIN_COMMON_H


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
#if defined(TCC_HEVC_INCLUDE)
#include <tcc_hevc_ioctl.h>
#endif
#if defined(TCC_VPU_4K_D2_INCLUDE)
#include <tcc_4k_d2_ioctl.h>
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

#endif // USE_COMMON_KERNEL_LOCATION

#include <tcc_video_common.h>


#define VPC_VDEC_SKIP_FRAME_DISABLE		(0)	//!< Skip disable
#define VPC_VDEC_SKIP_FRAME_EXCEPT_I	(1)	//!< Skip except I(IDR) picture
#define VPC_VDEC_SKIP_FRAME_ONLY_B		(2)	//!< Skip B picture(skip if nal_ref_idc==0 in H.264)
#define VPC_VDEC_SKIP_FRAME_UNCOND		(3)	//!< Unconditionally Skip one picture

#define VPC_AVC_IDR_PICTURE_SEARCH_MODE			(0x001)
#define VPC_AVC_NONIDR_PICTURE_SEARCH_MODE 		(0x201)


// --------------------------------------------------------------------------------------
// translate driver defines and types
// --------------------------------------------------------------------------------------
#define VPK_PA								(PA) // 0
#define VPK_VA								(VA) // 1
#define VPK_K_VA							(2)

#define VPK_COMP_Y 							(COMP_Y) // 0
#define VPK_COMP_U 							(COMP_U) // 1
#define VPK_COMP_V 							(COMP_V) // 2

#define VPK_VPU_DEC 						VPU_DEC
#define VPK_DEC_WITH_ENC 					DEC_WITH_ENC
#define VPK_DEC_ONLY						DEC_ONLY

#define VPK_STD_MJPG						STD_MJPG
#define VPK_STD_H263						STD_H263
#define VPK_STD_H264						STD_AVC
#define VPK_STD_H265						STD_HEVC
#define VPK_STD_MPEG2						STD_MPEG2
#define VPK_STD_MPEG4						STD_MPEG4
#define VPK_STD_VP8							STD_VP8
#define VPK_STD_VP9							STD_VP9

#define VPK_PIC_TYPE_I						PIC_TYPE_I
#define VPK_PIC_TYPE_P						PIC_TYPE_P
#define VPK_PIC_TYPE_B						PIC_TYPE_B
#define VPK_PIC_TYPE_IDR 					PIC_TYPE_IDR
#define VPK_PIC_TYPE_B_PB 					PIC_TYPE_B_PB

#define VPK_BUFFER_FRAMEBUFFER				BUFFER_FRAMEBUFFER
#define VPK_BUFFER_SLICE					BUFFER_SLICE
#define VPK_SLICE_SAVE_SIZE					SLICE_SAVE_SIZE
#define VPK_BUFFER_WORK 					BUFFER_WORK
#define VPK_WORK_CODE_PARA_BUF_SIZE 		WORK_CODE_PARA_BUF_SIZE
#define VPK_BUFFER_PS 						BUFFER_PS
#define VPK_PS_SAVE_SIZE 					PS_SAVE_SIZE
#define VPK_BUFFER_STREAM 					BUFFER_STREAM
#define VPK_LARGE_STREAM_BUF_SIZE 			LARGE_STREAM_BUF_SIZE

#if defined(TCC_HEVC_INCLUDE)
#define VPK_WAVE4_10BITS_DISABLE			WAVE4_10BITS_DISABLE
#define VPK_WAVE4_WTL_ENABLE				WAVE4_WTL_ENABLE
#define VPK_WAVE4_STREAM_BUF_SIZE			WAVE4_STREAM_BUF_SIZE
#define VPK_WAVE4_WORK_CODE_BUF_SIZE 		WAVE4_WORK_CODE_BUF_SIZE
#endif

#if defined(TCC_VPU_4K_D2_INCLUDE)
#define VPK_WAVE5_WTL_ENABLE				WAVE5_WTL_ENABLE
#define VPK_WAVE5_10BITS_DISABLE			WAVE5_10BITS_DISABLE
#define VPK_WAVE5_STREAM_BUF_SIZE			WAVE5_STREAM_BUF_SIZE
#define VPK_WAVE5_WORK_CODE_BUF_SIZE 		WAVE5_WORK_CODE_BUF_SIZE
#endif

// TCCxxxx_VPU_CODEC_COMMON.h
#define VPK_RETCODE_SUCCESS						RETCODE_SUCCESS
#define VPK_RETCODE_CODEC_EXIT					RETCODE_CODEC_EXIT
#define VPK_RETCODE_MULTI_CODEC_EXIT_TIMEOUT	RETCODE_MULTI_CODEC_EXIT_TIMEOUT
#define VPK_RETCODE_CODEC_SPECOUT				RETCODE_CODEC_SPECOUT
#define VPK_RETCODE_NOT_INITIALIZED				RETCODE_NOT_INITIALIZED
#define VPK_RETCODE_INVALID_STRIDE				RETCODE_INVALID_STRIDE
#define VPK_RETCODE_INVALID_COMMAND				RETCODE_INVALID_COMMAND

#define VPK_VPU_DEC_SUCCESS_FIELD_PICTURE 		VPU_DEC_SUCCESS_FIELD_PICTURE
#define VPK_VPU_DEC_SUCCESS 					VPU_DEC_SUCCESS
#define VPK_VPU_DEC_BUF_FULL 					VPU_DEC_BUF_FULL
#define VPK_VPU_DEC_RESOLUTION_CHANGED			(6)

#define VPK_VPU_DEC_OUTPUT_FAIL					VPU_DEC_OUTPUT_FAIL
#define VPK_VPU_DEC_OUTPUT_SUCCESS				VPU_DEC_OUTPUT_SUCCESS

// tcc_video_common.h
#define VPK_DEVICE_INITIALIZE				DEVICE_INITIALIZE

#define VPK_V_DEC_INIT						V_DEC_INIT
#define VPK_V_DEC_CLOSE						V_DEC_CLOSE
#define VPK_V_DEC_SEQ_HEADER				V_DEC_SEQ_HEADER
#define VPK_V_DEC_SWRESET					V_DEC_SWRESET
#define VPK_V_DEC_DECODE					V_DEC_DECODE
#define VPK_V_DEC_INIT_RESULT				V_DEC_INIT_RESULT
#define VPK_V_DEC_SEQ_HEADER_RESULT			V_DEC_SEQ_HEADER_RESULT
#define VPK_V_DEC_DECODE_RESULT				V_DEC_DECODE_RESULT
#define VPK_V_DEC_GENERAL_RESULT			V_DEC_GENERAL_RESULT
#define VPK_V_DEC_REG_FRAME_BUFFER			V_DEC_REG_FRAME_BUFFER
#define VPK_V_GET_VPU_VERSION 				V_GET_VPU_VERSION
#define VPK_V_GET_RING_BUFFER_STATUS 		V_GET_RING_BUFFER_STATUS
#define VPK_V_FILL_RING_BUFFER_AUTO 		V_FILL_RING_BUFFER_AUTO
#define VPK_V_DEC_UPDATE_RINGBUF_WP 		V_DEC_UPDATE_RINGBUF_WP
#define VPK_V_DEC_BUF_FLAG_CLEAR			V_DEC_BUF_FLAG_CLEAR
#define VPK_V_DEC_FLUSH_OUTPUT 				V_DEC_FLUSH_OUTPUT
#define VPK_V_DEC_FREE_MEMORY 				V_DEC_FREE_MEMORY
#define VPK_V_DEC_ALLOC_MEMORY 				V_DEC_ALLOC_MEMORY

#define VPK_VPU_GET_FREEMEM_SIZE 			VPU_GET_FREEMEM_SIZE
#define VPK_VPU_SET_MEM_ALLOC_MODE 			VPU_SET_MEM_ALLOC_MODE
#define VPK_VPU_GET_INSTANCE_IDX 			VPU_GET_INSTANCE_IDX
#define VPK_VPU_CLEAR_INSTANCE_IDX 			VPU_CLEAR_INSTANCE_IDX
#define VPK_VPU_SET_CLK 					VPU_SET_CLK

// tcc_video_common.h
#define VPK_BUFFER_TYPE 					Buffer_Type
#define VPK_BUFFER_TYPE_MIN 				BUFFER_ELSE
#define VPK_BUFFER_TYPE_MAX 				BUFFER_USERDATA
#define VPK_OPEN_TYPE	 					Open_Type
#define VPK_OPEN_TYPE_MIN 					VPK_DEC_WITH_ENC
#define VPK_OPEN_TYPE_MAX 					VPK_DEC_ONLY

typedef codec_addr_t 						VPK_CODEC_ADDR_t;

// tcc_vpu_ioctl.h
typedef VDEC_INIT_t 						VPK_VDEC_INIT_t;
typedef VDEC_SEQ_HEADER_t 					VPK_VDEC_SEQ_HEADER_t;
typedef VDEC_SET_BUFFER_t 					VPK_VDEC_SET_BUFFER_t;
typedef VDEC_DECODE_t 						VPK_VDEC_DECODE_t;
typedef MEM_ALLOC_INFO_t 					VPK_MEM_ALLOC_INFO_t;

#if defined(TCC_JPU_C6_INCLUDE)
typedef JDEC_INIT_t 						VPK_JDEC_INIT_t;
typedef JDEC_SEQ_HEADER_t 					VPK_JDEC_SEQ_HEADER_t;
typedef JPU_SET_BUFFER_t 					VPK_JPU_SET_BUFFER_t;
typedef JPU_DECODE_t 						VPK_JPU_DECODE_t;
#endif

#if defined(TCC_HEVC_INCLUDE)
typedef HEVC_INIT_t 						VPK_HEVC_INIT_t;
typedef HEVC_SEQ_HEADER_t					VPK_HEVC_SEQ_HEADER_t;
typedef HEVC_SET_BUFFER_t					VPK_HEVC_SET_BUFFER_t;
typedef HEVC_DECODE_t						VPK_HEVC_DECODE_t;
#endif

#if defined(TCC_VPU_4K_D2_INCLUDE)
typedef VPU_4K_D2_INIT_t 					VPK_VPU_4K_D2_INIT_t;
typedef VPU_4K_D2_SEQ_HEADER_t 				VPK_VPU_4K_D2_SEQ_HEADER_t;
typedef VPU_4K_D2_SET_BUFFER_t 				VPK_VPU_4K_D2_SET_BUFFER_t;
typedef VPU_4K_D2_DECODE_t 					VPK_VPU_4K_D2_DECODE_t;
#endif

// tcc_video_common.h
typedef INSTANCE_INFO 						VPK_INSTANCE_INFO_t;
typedef OPENED_sINFO 						VPK_OPENED_sINFO_t;
typedef CONTENTS_INFO						VPK_CONTENTS_INFO_t;
// --------------------------------------------------------------------------------------


typedef enum
{
	VPC_DRIVER_TYPE_NONE = 0,
	VPC_DRIVER_TYPE_VPU,
	VPC_DRIVER_TYPE_MGR,

	VPC_DRIVER_TYPE_MAX
} VPC_DRIVER_TYPE_E;
typedef enum
{
	VPC_DRIVER_POLL_ERROR_NONE = 0,

	VPC_DRIVER_POLL_ERROR_UNKNOWN,
	VPC_DRIVER_POLL_ERROR_RET,
	VPC_DRIVER_POLL_ERROR_TIMEOUT,
	VPC_DRIVER_POLL_ERROR_POLLERR,

	VPC_DRIVER_POLL_ERROR_MAX
} VPC_DRIVER_POLL_ERROR_E;

typedef enum
{
	VPC_COMMAND_None 							= 0,

	VPC_COMMAND_V_DEC_INIT 						= 101,
	VPC_COMMAND_V_DEC_CLOSE,
	VPC_COMMAND_V_DEC_SWRESET,
	VPC_COMMAND_V_DEC_SEQ_HEADER,
	VPC_COMMAND_V_DEC_DECODE,
	VPC_COMMAND_V_DEC_REG_FRAME_BUFFER,
	VPC_COMMAND_V_DEC_UPDATE_RINGBUF_WP,
	VPC_COMMAND_V_DEC_BUF_FLAG_CLEAR,
	VPC_COMMAND_V_DEC_FLUSH_OUTPUT,
	VPC_COMMAND_V_DEC_FREE_MEMORY,
	VPC_COMMAND_V_DEC_ALLOC_MEMORY,

	VPC_COMMAND_V_GET_VPU_VERSION 				= 201,
	VPC_COMMAND_V_GET_RING_BUFFER_STATUS,
	VPC_COMMAND_V_FILL_RING_BUFFER_AUTO,

	VPC_COMMAND_VPU_GET_FREEMEM_SIZE 			= 301,
	VPC_COMMAND_VPU_SET_MEM_ALLOC_MODE,
	VPC_COMMAND_VPU_GET_INSTANCE_IDX,
	VPC_COMMAND_VPU_CLEAR_INSTANCE_IDX,
	VPC_COMMAND_VPU_SET_CLK,

	VPC_COMMAND_DEVICE_INITIALIZE 				= 401,

	VPC_COMMAND_V_DEC_INIT_RESULT 				= 1001,
	VPC_COMMAND_V_DEC_GENERAL_RESULT,
	VPC_COMMAND_V_DEC_SEQ_HEADER_RESULT,
	VPC_COMMAND_V_DEC_DECODE_RESULT,
	VPC_COMMAND_Max
} VPC_COMMAND_E;

typedef enum
{
	VPC_FRAME_SKIP_MODE_NONE,

	VPC_FRAME_SKIP_MODE_B_SKIP = 100,
	VPC_FRAME_SKIP_MODE_NEXT_B_SKIP,
	VPC_FRAME_SKIP_MODE_EXCEPT_I_SKIP,

	VPC_FRAME_SKIP_MODE_B_WAIT = 200,

	VPC_FRAME_SKIP_MODE_I_SEARCH = 300,
	VPC_FRAME_SKIP_MODE_IDR_SEARCH,
	VPC_FRAME_SKIP_MODE_NONIDR_SEARCH,

	VPC_FRAME_SKIP_MODE_NEXT_STEP = 400,
} VPC_FRAME_SKIP_MODE_E;

typedef enum
{
	VPC_FRAME_TYPE_UNKNOWN = 0,
	VPC_FRAME_TYPE_FIELD,
	VPC_FRAME_TYPE_FRAME
} VPC_FRAME_TYPE_E;

typedef struct
{
	int iMgrFd;
	int iVpuFd;
	int iMemFd;
} VPC_DRIVER_FD_T;

typedef struct
{
	VPC_FRAME_SKIP_MODE_E eFrameSkipMode;

	int iSkipFrameNum;
	int iFrameSearchEnable;
	int iSkipFrameMode;
} VPC_FRAME_SKIP_T;

typedef struct
{
	int iLeft;
	int iTop;
	int iRight;
	int iBottom;
} VPC_RECT_INFO_T;

typedef struct
{
	int iWidth;
	int iHeight;
} VPC_RESOLUTION_INFO_T;

typedef struct
{
	VPC_FRAME_TYPE_E eFrameType;

	int iDecodingResult;
	int iDecodingOutResult;
	int iDecodedIdx;
	int iDispOutIdx;
	int iNumOfErrMBs;
	int iPictureStructure;
	int iPictureType;

	int iInterlace;
	int iInterlacedFrame;
	int iM2vProgressiveFrame;
	int iTopFieldFirst;

	int iM2vAspectRatio;

	unsigned char *pucDispOut[VPK_VA + 1][VPK_COMP_V + 1];
	unsigned char *pucCurrOut[VPK_VA + 1][VPK_COMP_V + 1];
	unsigned char *pucPrevOut[VPK_VA + 1][VPK_COMP_V + 1];

	VPC_RECT_INFO_T tCropRect;
	VPC_RESOLUTION_INFO_T tVideoResolution;
} VPC_DECODE_RESULT_INFO_T;

typedef struct
{
	VPC_DRIVER_FD_T tDriverFD;
	VPC_FRAME_SKIP_T tFrameSkip;
	VPC_DECODE_RESULT_INFO_T tDecodeResultInfo;

	int iInstIdx;
	unsigned int uiCodec;

	int iInstType;
	int iNumOfInst;

	int iOpenType;
	int iNumOfOpen;

	int iCurrStreamIdx;

	int iDecodedFrames;
	int iDisplyedFrames;

	uint32_t uiRegBaseVirtualAddr;

	int iPicWidth;
	int iPicHeight;
	int iAspectRatio;
	int iFilePlayEnable;
	int iFrameRate;
	int iBitRateMbps;
	int iExtFunction;
	int iExtraFrameCount;

	unsigned char *pucSliceBuffer;
	int iSliceBufSize;

	int iMinFrameBufferCount;
	int iMinFrameBufferSize;
	int iFrameBufferCount;
	int iFrameBufferSize;
	unsigned char *pucFrameBuffer[VPK_K_VA + 1];

	int iBitstreamFormat;
	int iBitstreamBufSize;
	unsigned char *pucBitstreamBuffer[VPK_K_VA + 1];

	int iInputBufSize;
	unsigned char *pucInputBuffer[VPK_VA + 1];

	bool bEnableUserData;
	bool bCbCrInterleaveMode;

	bool bEnable10Bits;
	bool bEnableMapConverter;

	// ============================================

	// ============================================
	// Ref. Driver
	// ----------------------------------------
	VPK_MEM_ALLOC_INFO_t tAllocMem;

	VPK_VDEC_INIT_t tInit;
	VPK_VDEC_SEQ_HEADER_t tSeqHeader;
	VPK_VDEC_SET_BUFFER_t tSetBuffer;
	VPK_VDEC_DECODE_t tDecode;

	#if defined(TCC_JPU_C6_INCLUDE)
	VPK_JDEC_INIT_t tJdecInit;
	VPK_JDEC_SEQ_HEADER_t tJdecSeqHeader;
	VPK_JPU_SET_BUFFER_t tJdecSetBuffer;
	VPK_JPU_DECODE_t tJdecDecode;
	#endif

	#if defined(TCC_HEVC_INCLUDE)
	VPK_HEVC_INIT_t tHevcInit;
	VPK_HEVC_SEQ_HEADER_t tHevcSeqHeader;
	VPK_HEVC_SET_BUFFER_t tHevcSetBuffer;
	VPK_HEVC_DECODE_t tHevcDecode;
	#endif

	#if  defined(TCC_VPU_4K_D2_INCLUDE)
	VPK_VPU_4K_D2_INIT_t tVpu4KD2Init;
	VPK_VPU_4K_D2_SEQ_HEADER_t tVpu4KD2SeqHeader;
	VPK_VPU_4K_D2_SET_BUFFER_t tVpu4KD2SetBuffer;
	VPK_VPU_4K_D2_DECODE_t tVpu4KD2Decode;
	#endif
	// ============================================
} VPC_DRIVER_INFO_T;


int video_plugin_cmd(VPC_DRIVER_INFO_T *ptDriverInfo, VPC_DRIVER_TYPE_E eDriverType, VPC_COMMAND_E eCmd, void *pvParam);
int video_plugin_poll_cmd(VPC_DRIVER_INFO_T *ptDriverInfo, VPC_DRIVER_TYPE_E eDriverType, VPC_COMMAND_E eCmd, void *pvParam);

DP_Video_Error_E video_plugin_open_drivers(VPC_DRIVER_INFO_T *ptDriverInfo);
DP_Video_Error_E video_plugin_close_drivers(VPC_DRIVER_INFO_T *ptDriverInfo);

void video_plugin_set_init_info(VPC_DRIVER_INFO_T *ptDriverInfo);
int video_plugin_set_seq_hearder(VPC_DRIVER_INFO_T *ptDriverInfo, DP_Video_Config_SequenceHeader_T *ptSequenceHeader);
int video_plugin_is_pic_resolution_ok(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_RESOLUTION_INFO_T tMaxResInfo, VPC_RESOLUTION_INFO_T tMinResInfo);
int video_plugin_alloc_bitstream_buffer(VPC_DRIVER_INFO_T *ptDriverInfo);
void video_plugin_free_alloc_buffers(VPC_DRIVER_INFO_T *ptDriverInfo);
void video_plugin_set_decoding_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer, int iBufSize);
void video_plugin_get_header_info(VPC_DRIVER_INFO_T *ptDriverInfo);
int video_plugin_alloc_spspps_buffer(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSpsPpsBuffer);
void video_plugin_set_frame_skip_mode(VPC_DRIVER_INFO_T *ptDriverInfo, VPC_FRAME_SKIP_MODE_E eFrameSkipMode);
int video_plugin_alloc_bitwork_buffer(VPC_DRIVER_INFO_T *ptDriverInfo);
unsigned long video_plugin_get_video_decode_result(VPC_DRIVER_INFO_T *ptDriverInfo, DP_Video_DecodingResult_T *ptDecodingResult);

void video_plugin_get_decode_result_info(VPC_DRIVER_INFO_T *ptDriverInfo);
int video_plugin_alloc_frame_buffer(VPC_DRIVER_INFO_T *ptDriverInfo, int iAvailBufSize, int iMaxFrameBufferCnt);
int video_plugin_alloc_slice_buffer(VPC_DRIVER_INFO_T *ptDriverInfo);


#define VDEC_MEMCPY(dst, src, size) \
{ \
	unsigned long *dst_l = (unsigned long *)((uintptr_t)(dst)); \
	const unsigned long *src_l = (unsigned long *)((uintptr_t)(src)); \
	size_t size_l = (size_t)(size); \
	(void)memcpy(dst_l, src_l, size_l); \
}

#endif	// DECODER_PLUGIN_COMMON_H