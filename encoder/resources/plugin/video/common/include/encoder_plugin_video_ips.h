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

#ifndef VIDEO_ENCODER_PLUGIN_IPS_H
#define VIDEO_ENCODER_PLUGIN_IPS_H

#include <encoder_plugin_video.h>
#include <encoder_plugin_video_common.h>


#define VENC_DUMP_ON (0)


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
} VEP_DUMP_DISP_INFO_T;

union VEPCastAddr
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

static inline uintptr_t vpu_enc_cast_void_uintptr(void *pvAddr)
{
    union VEPCastAddr un_cast;
    un_cast.pvAddr = NULL;
    un_cast.pvAddr = pvAddr;
    return un_cast.uintptrAddr;
}

static inline unsigned int vpu_enc_cast_void_uint(void *pvAddr)
{
	unsigned int uiAddr = 0U;
    union VEPCastAddr un_cast;
    un_cast.pvAddr = NULL;
    un_cast.pvAddr = pvAddr;

	if (un_cast.uintptrAddr < UINT32_MAX) {
		uiAddr = (unsigned int)un_cast.uintptrAddr;
	}

    return uiAddr;
}


// =========================================================================================
// encoder_plugin_video_ips.c
// -------------------------------------------------------------------
EP_Video_Error_E vpu_enc_open_mgr(VEPC_DRIVER_INFO_T *ptDriverInfo);
EP_Video_Error_E vpu_enc_close_mgr(VEPC_DRIVER_INFO_T *ptDriverInfo);

EP_Video_Error_E vpu_enc_open_vpu(VEPC_DRIVER_INFO_T *ptDriverInfo);
EP_Video_Error_E vpu_enc_close_vpu(VEPC_DRIVER_INFO_T *ptDriverInfo);

int vpu_enc_translate_cmd(VEPC_COMMAND_E eCmd, unsigned int *puiCmd, char *pstrCmd);

void *vpu_enc_get_cmd_param(VEPC_DRIVER_INFO_T *ptDriverInfo, VEPC_COMMAND_E eCmd, void *pvParam);
int vpu_enc_get_driverfd(const VEPC_DRIVER_INFO_T *ptDriverInfo, VEPC_DRIVER_TYPE_E eDriverType);
int vpu_enc_get_cmd_result(const VEPC_DRIVER_INFO_T *ptDriverInfo, VEPC_DRIVER_TYPE_E eDriverType, VEPC_COMMAND_E eCmd, void *pvParam);
unsigned char *vpu_enc_get_virtaddr(unsigned char *pucBitStreamBufAddr[VEPK_K_VA + 1], codec_addr_t convert_addr, uint32_t base_index);

void vpu_enc_dump_data(int width, int height, int size, unsigned char *pucInput);
void vpu_enc_dump_result_frame(VEP_DUMP_DISP_INFO_T *ptDumpInfo);

// =========================================================================================

// =========================================================================================
// encoder_plugin_video_ips_venc.c
// -------------------------------------------------------------------
void vpu_venc_get_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_venc_get_header_info(VEPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_venc_upate_get_result_info(VEPC_DRIVER_INFO_T *ptDriverInfo);

void vpu_venc_set_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_venc_set_encode_info(VEPC_DRIVER_INFO_T *ptDriverInfo, EP_Video_Encode_Input_T *ptEncodeInput);
void vpu_venc_set_bitstream_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);
void vpu_venc_set_bitwork_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);
void vpu_venc_set_frame_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);
void vpu_venc_set_seq_header_info(VEPC_DRIVER_INFO_T *ptDriverInfo, int iType, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);
// =========================================================================================

// =========================================================================================
// encoder_plugin_video_ips_4ke1.c
// -------------------------------------------------------------------
#if defined(TCC_VPU_4K_E1_INCLUDE)
unsigned int vpu_4ke1_reg_read(unsigned long *base_addr, unsigned int offset);
void vpu_4ke1_reg_write(unsigned long *base_addr, unsigned int offset, unsigned int data);

void vpu_4ke1_get_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_4ke1_get_encode_result_info(VEPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_4ke1_get_header_info(VEPC_DRIVER_INFO_T *ptDriverInfo);

void vpu_4ke1_set_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_4ke1_set_bitwork_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);
void vpu_4ke1_set_bitstream_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);
void vpu_4ke1_set_frame_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);
void vpu_4ke1_set_seq_header_info(VEPC_DRIVER_INFO_T *ptDriverInfo, int iType, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);
void vpu_4ke1_set_encode_info(VEPC_DRIVER_INFO_T *ptDriverInfo, EP_Video_Encode_Input_T *ptEncodeInput);
#endif
// =========================================================================================

// =========================================================================================
// encoder_plugin_video_ips_jenc.c
// -------------------------------------------------------------------
#if defined(TCC_JPU_C6_INCLUDE)
void vpu_jenc_set_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_jenc_set_encode_info(VEPC_DRIVER_INFO_T *ptDriverInfo,  EP_Video_Encode_Input_T *ptEncodeInput);
void vpu_jenc_set_bitstream_buffer_info(VEPC_DRIVER_INFO_T *ptDriverInfo, unsigned char *pucBuffer[VEPK_K_VA + 1], int iBufSize);

void vpu_jenc_get_init_info(VEPC_DRIVER_INFO_T *ptDriverInfo);
void vpu_jenc_get_encode_result_info(VEPC_DRIVER_INFO_T *ptDriverInfo);
#endif
// =========================================================================================

#endif	// VIDEO_ENCODER_PLUGIN_IPS_H