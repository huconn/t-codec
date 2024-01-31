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

#ifndef DECODER_PLUGIN_IPS_H
#define DECODER_PLUGIN_IPS_H

#include <decoder_plugin_video.h>
#include <decoder_plugin_video_common.h>


#define VDEC_DUMP_ON (0)


union VPCastAddr
{
	intptr_t 		intptrAddr;
	uintptr_t 		uintptrAddr;
	unsigned char 	*pucAddr;
	char 			*pcAddr;
	unsigned int 	*puiAddr;
	int 			*piAddr;
	void 			*pvAddr;
	unsigned long 	*pulAddr;
	long 			*plAddr;
};


typedef struct
{
	unsigned int uiCodec;
	int iWidth;
	int iHeight;
	bool bInterleaveMode;
	int iDumpMax;

	unsigned char *pucY;
	unsigned char *pucU;
	unsigned char *pucV;
} VP_DUMP_DISP_INFO_T;


// =========================================================================================
// decoder_plugin_video_ips.c
// -------------------------------------------------------------------
void *vpu_get_cmd_param(VPC_DRIVER_INFO_T *ptDriverInfo, VPC_COMMAND_E eCmd, void *pvParam);
int vpu_get_cmd_result(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_DRIVER_TYPE_E eDriverType, VPC_COMMAND_E eCmd, void *pvParam);

int vpu_translate_cmd(VPC_COMMAND_E eCmd, unsigned int *puiCmd, char *pstrCmd);
int vpu_get_driverfd(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_DRIVER_TYPE_E eDriverType);

DP_Video_Error_E vpu_open_mgr(VPC_DRIVER_INFO_T *ptDriverInfo);
DP_Video_Error_E vpu_open_vpu(VPC_DRIVER_INFO_T *ptDriverInfo);
DP_Video_Error_E vpu_open_mem(VPC_DRIVER_INFO_T *ptDriverInfo);

DP_Video_Error_E vpu_close_mgr(VPC_DRIVER_INFO_T *ptDriverInfo);
DP_Video_Error_E vpu_close_vpu(VPC_DRIVER_INFO_T *ptDriverInfo);
DP_Video_Error_E vpu_close_mem(VPC_DRIVER_INFO_T *ptDriverInfo);

void vpu_set_skip_mode(VPC_DRIVER_INFO_T *ptDriverInfo, VPC_COMMAND_E eCmd);

void vpu_check_picture_type(int iPicType);
void vpu_check_return_value(VPC_DRIVER_TYPE_E eDriverType, VPC_COMMAND_E eCmd, int iRetVal);
unsigned char *vpu_get_framebuf_virtaddr(unsigned char *pucFrameBufAddr[VPK_K_VA + 1], unsigned char *convert_addr, uint32_t base_index);

void vpu_dump_disp_frame(VP_DUMP_DISP_INFO_T *ptDumpInfo);

static inline uintptr_t vpu_cast_void_uintptr(void *pvAddr)
{
    union VPCastAddr un_cast;
    un_cast.pvAddr = NULL;
    un_cast.pvAddr = pvAddr;
    return un_cast.uintptrAddr;
}
// =========================================================================================

// =========================================================================================
// decoder_plugin_video_ips_vdec.c
// -------------------------------------------------------------------
void vpu_vdec_set_init_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_vdec_set_seq_header_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, int iBitstreamBufSize, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1]);
void vpu_vdec_set_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1], int iBitstreamBufSize);
int vpu_vdec_is_pic_resolution_ok(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_RESOLUTION_INFO_T tMaxResInfo, VPC_RESOLUTION_INFO_T tMinResInfo);
void vpu_vdec_get_decode_result_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_vdec_set_decoding_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_vdec_set_bitwork_addr(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_vdec_set_spspps_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSpsPpsBuffer, int iSpsPpsSize);
void vpu_vdec_set_stream_size(VPC_DRIVER_INFO_T *ptDriverInfo, int iStreamLen);
void vpu_vdec_get_header_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_vdec_get_slice_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSliceBuffer, int iSliceBufSize, int iExtraFrameCount);
void vpu_vdec_set_frame_buffer_start_addr_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucFrameBuffer[VPK_K_VA + 1], int iFrameBufferSize);
// =========================================================================================

// =========================================================================================
// decoder_plugin_video_ips_jdec.c
// -------------------------------------------------------------------
#if defined(TCC_JPU_C6_INCLUDE)
void vpu_jdec_set_init_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_jdec_set_seq_header_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, int iBitstreamBufSize, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1]);
void vpu_jdec_set_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1], int iBitstreamBufSize);
int vpu_jdec_is_pic_resolution_ok(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_RESOLUTION_INFO_T tMaxResInfo, VPC_RESOLUTION_INFO_T tMinResInfo);
void vpu_jdec_get_decode_result_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_jdec_set_decoding_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_jdec_set_stream_size(VPC_DRIVER_INFO_T *ptDriverInfo, int iStreamLen);
void vpu_jdec_get_header_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_jdec_set_frame_buffer_start_addr_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucFrameBuffer[VPK_K_VA + 1], int iFrameBufferSize);
#endif
// =========================================================================================

// =========================================================================================
// decoder_plugin_video_ips_4kd2.c
// -------------------------------------------------------------------
#if defined(TCC_VPU_4K_D2_INCLUDE)
void vpu_4kd2_set_init_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_4kd2_set_seq_header_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, int iBitstreamBufSize, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1]);
void vpu_4kd2_set_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1], int iBitstreamBufSize);
int vpu_4kd2_is_pic_resolution_ok(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_RESOLUTION_INFO_T tMaxResInfo, VPC_RESOLUTION_INFO_T tMinResInfo);
void vpu_4kd2_get_decode_result_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_4kd2_set_decoding_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_4kd2_set_bitwork_addr(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_4kd2_set_stream_size(VPC_DRIVER_INFO_T *ptDriverInfo, int iStreamLen);
void vpu_4kd2_get_header_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_4kd2_get_slice_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSliceBuffer, int iSliceBufSize, int iExtraFrameCount);
void vpu_4kd2_set_frame_buffer_start_addr_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucFrameBuffer[VPK_K_VA + 1], int iFrameBufferSize);
#endif
// =========================================================================================

// =========================================================================================
// decoder_plugin_video_ips_hevc.c
// -------------------------------------------------------------------
#if defined(TCC_HEVC_INCLUDE)
void vpu_hevc_set_init_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_hevc_set_seq_header_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, int iBitstreamBufSize, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1]);
void vpu_hevc_set_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBitstreamBuffer[VPK_K_VA + 1], int iBitstreamBufSize);
int vpu_hevc_is_pic_resolution_ok(const VPC_DRIVER_INFO_T *ptDriverInfo, VPC_RESOLUTION_INFO_T tMaxResInfo, VPC_RESOLUTION_INFO_T tMinResInfo);
void vpu_hevc_get_decode_result_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_hevc_set_decoding_bitstream_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_hevc_set_bitwork_addr(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_hevc_set_stream_size(VPC_DRIVER_INFO_T *ptDriverInfo, int iStreamLen);
void vpu_hevc_get_header_info(VPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_hevc_get_slice_buffer_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucSliceBuffer, int iSliceBufSize, int iExtraFrameCount);
void vpu_hevc_set_frame_buffer_start_addr_info(VPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucFrameBuffer[VPK_K_VA + 1], int iFrameBufferSize);
#endif
// =========================================================================================

#endif	// DECODER_PLUGIN_IPS_H