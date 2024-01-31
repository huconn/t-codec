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


#include "vdec.h"

#ifdef INCLUDE_WMV78_DEC

#define LOG_TAG "VPU_DEC_K_WMV78"

#include "TCCMemory.h"
#include "wmv78dec/TCC_WMV78_DEC.h"
#include "wmv78dec/TCC_WMV78_DEC_Huff_table.h"

#include <sys/mman.h>
#include <errno.h>

#include <sys/ioctl.h>
#if defined(USE_COMMON_KERNEL_LOCATION)
#include <tcc_vpu_ioctl.h>

#else //use chipset folder
#include <mach/tcc_vpu_ioctl.h>
#endif

#include <dlfcn.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>

#define WMV78_LIB_NAME ("libtccwmv78dec.so")
#if 0
//! Callback Func
typedef struct vdec_callback_func_t
{
  unsigned long*    (*m_pfMalloc        ) ( size_t );                            //!< malloc
  unsigned long*    (*m_pfNonCacheMalloc) ( size_t );                            //!< non-cacheable malloc
  void     (*m_pfFree          ) ( unsigned long* );                             //!< free
  void     (*m_pfNonCacheFree  ) ( unsigned long* );                             //!< non-cacheable free
  unsigned long*    (*m_pfMemcpy        ) ( unsigned long*, const unsigned long*, size_t );        //!< memcpy
  void     (*m_pfMemset        ) ( unsigned long*, int32_t, size_t );                //!< memset
  unsigned long*    (*m_pfRealloc       ) ( unsigned long*, size_t );                     //!< realloc
  unsigned long*    (*m_pfMemmove       ) ( unsigned long*, const unsigned long*, size_t );        //!< memmove

  unsigned long*    (*m_pfPhysicalAlloc ) ( size_t );
  void     (*m_pfPhysicalFree  ) ( unsigned long*, size_t );
  unsigned long*    (*m_pfVirtualAlloc  ) ( unsigned long*, size_t, size_t );
  void     (*m_pfVirtualFree   ) ( unsigned long*, size_t, size_t );
  int32_t  m_Reserved1[16-12];

  unsigned long*    (*m_pfFopen         ) (const char *, const char *);          //!< fopen
  size_t   (*m_pfFread         ) (unsigned long*, size_t, size_t, unsigned long* );       //!< fread
  int32_t  (*m_pfFseek         ) (unsigned long*, long, int32_t );               //!< fseek
  long     (*m_pfFtell         ) (unsigned long* );                              //!< ftell
  size_t   (*m_pfFwrite        ) (const unsigned long*, size_t, size_t, unsigned long*);  //!< fwrite
  int32_t  (*m_pfFclose        ) (unsigned long*);                              //!< fclose
  int32_t  (*m_pfUnlink        ) (const char *);                       //!< _unlink
  uint32_t (*m_pfFeof          ) (unsigned long*);                              //!< feof
  uint32_t (*m_pfFflush        ) (unsigned long*);                              //!< fflush

  int32_t  (*m_pfFseek64       ) (unsigned long*, int64_t, int32_t );            //!< fseek 64bi io
  int64_t  (*m_pfFtell64       ) (unsigned long* );                              //!< ftell 64bi io
  int32_t  m_Reserved2[16-11];
} vdec_callback_func_t;
#endif
static int WMV78_dec_seq_header(tc_vdec_ *pVdec)
{
  int ret = 0;
  tc_vdec_ * pInst = pVdec;
  tcomx_zero_memset( &pInst->gsWMV78DecInitialInfo, 0, sizeof(pInst->gsWMV78DecInitialInfo) );

  pInst->gsWMV78DecInitialInfo.m_iAvcPicCrop.m_iCropBottom = 0;
  pInst->gsWMV78DecInitialInfo.m_iAvcPicCrop.m_iCropLeft = 0;
  pInst->gsWMV78DecInitialInfo.m_iAvcPicCrop.m_iCropRight = 0;
  pInst->gsWMV78DecInitialInfo.m_iAvcPicCrop.m_iCropTop = 0;
  pInst->gsWMV78DecInitialInfo.m_iMinFrameBufferCount = 1;
  pInst->gsWMV78DecInitialInfo.m_iPicWidth = pInst->gsWMV78DecInit.m_iWidth;
  pInst->gsWMV78DecInitialInfo.m_iPicHeight = pInst->gsWMV78DecInit.m_iHeight;
  pInst->gsWMV78DecInitialInfo.m_iAspectRateInfo = 0;
  pInst->gsWMV78DecInitialInfo.m_iInterlace = 0;
  print_dec_initial_info( &pInst->gsVpuDecInit, &pInst->gsWMV78DecInitialInfo, pVdec );
  set_dec_common_info(pInst->gsWMV78DecInitialInfo.m_iPicWidth, pInst->gsWMV78DecInitialInfo.m_iPicHeight,
            &pInst->gsWMV78DecInitialInfo.m_iAvcPicCrop, pInst->gsWMV78DecInitialInfo.m_iInterlace,
            0, pInst );

  pInst->mRealPicWidth = pInst->gsWMV78DecInitialInfo.m_iPicWidth;
  pInst->mRealPicHeight = pInst->gsWMV78DecInitialInfo.m_iPicHeight;

  return ret;
}

static int WMV78_LoadLibrary(tc_vdec_ *pVdec)
{
  int ret = 0;
  pVdec->pExtDLLModule = dlopen(WMV78_LIB_NAME, RTLD_LAZY | RTLD_GLOBAL);
  if( pVdec->pExtDLLModule == NULL ) {
    LOGE("[SW-WMV12] Load library '%s' failed: %s", WMV78_LIB_NAME, dlerror());
    ret = -1;
  } else {
    LOGI("[SW-WMV12] Library '%s' Loaded", WMV78_LIB_NAME);
  }

  if (ret == 0) {
    pVdec->gExtDecFunc = dlsym(pVdec->pExtDLLModule, "Video_Proc");
    if( pVdec->gExtDecFunc == NULL ) {
      LOGE("[SW-WMV12] pVdec->gExtDecFunc Error");
      ret = -1;
    }
  }

  return ret;
}

static void WMV78_CloseLibrary(const tc_vdec_ *pVdec)
{
  if( pVdec->pExtDLLModule != NULL){
    (void)dlclose(pVdec->pExtDLLModule);
  }
}

int32_t
vdec_WMV78( int32_t iOpCode, unsigned long* pHandle, unsigned long* pParam1, unsigned long* pParam2, unsigned long* pParam3 )
{
  int ret = 0;
  tc_vdec_ *pInst = (tc_vdec_ *)CONV_PTR(pParam3);
  int buf_idx;
  unsigned int temp_uiSize;
  (void)pHandle;

  if(pInst == NULL){
    LOGE("vdec_WMV78(OP:%d) :: Instance is null!!", iOpCode);
    ret = (-RETCODE_NOT_INITIALIZED);
    goto vdec_WMV78_end;
  }

  if( (iOpCode != VDEC_INIT) && (iOpCode != VDEC_CLOSE) && (pInst->vdec_codec_opened == 0u)){
    ret = -RETCODE_NOT_INITIALIZED;
    goto vdec_WMV78_end;
  }
  if( iOpCode == VDEC_INIT )
  {
    uint32_t ChromaSize, uiWidth, uiHeight;
    const vdec_init_t* p_init_param = (vdec_init_t*)CONV_PTR(pParam1);
//    vdec_callback_func_t* pf_callback = (vdec_callback_func_t*)pParam2;

    if( 0 > WMV78_LoadLibrary(pInst)){
      ret = -(VPU_ENV_INIT_ERROR);
      goto vdec_WMV78_end;
    }
    pInst->codec_format = p_init_param->m_iBitstreamFormat;
    ret = vpu_env_open(p_init_param->m_iBitstreamFormat, 0u, 0u,
                       p_init_param->m_iPicWidth, p_init_param->m_iPicHeight, pInst );
    if(ret < 0) // to operate Max-clock for s/w codec!!
    {
      ret = -(VPU_ENV_INIT_ERROR);
      goto vdec_WMV78_end;
    }
    uiWidth = ((tcomx_s32_to_u32(p_init_param->m_iPicWidth)+15u)>>4u)<<4u;
    pInst->gsWMV78DecInit.m_iWidth  = tcomx_u32_to_s32(uiWidth);
    pInst->gsWMV78DecInit.m_iHeight = p_init_param->m_iPicHeight;

    uiHeight = ((tcomx_s32_to_u32(p_init_param->m_iPicHeight)+15u)>>4u)<<4u;
    pInst->gsWMV78FrameSize = ( pInst->gsWMV78DecInit.m_iWidth ) * ( tcomx_u32_to_s32(uiHeight) );
    ChromaSize = ((uiWidth) * (uiHeight))>>2u;

    temp_uiSize = ChromaSize * 6u;//tcomx_s32_to_u32(pInst->gsWMV78FrameSize*1.5);
    pInst->gsWMV78NCFrameSize = tcomx_u32_to_s32(ALIGN_BUFF( temp_uiSize, DPV_ALIGN_LEN ));

    pInst->gsWMV78CurYFrameAddress = ((unsigned char*)vdec_malloc((uint32_t)pInst->gsWMV78NCFrameSize));
    pInst->gsWMV78CurUFrameAddress  = ((unsigned char*)&pInst->gsWMV78CurYFrameAddress[pInst->gsWMV78FrameSize]);
    pInst->gsWMV78CurVFrameAddress  = ((unsigned char*)&pInst->gsWMV78CurUFrameAddress[ChromaSize]);

    pInst->gsWMV78Ref0YFrameAddress = ((unsigned char*)vdec_malloc((uint32_t)pInst->gsWMV78NCFrameSize));
    pInst->gsWMV78Ref0UFrameAddress = ((unsigned char*)&pInst->gsWMV78Ref0YFrameAddress[pInst->gsWMV78FrameSize]);
    pInst->gsWMV78Ref0VFrameAddress = ((unsigned char*)&pInst->gsWMV78Ref0UFrameAddress[ChromaSize]);
    pInst->gsWMV78DecInit.m_pExtraData      = p_init_param->m_pExtraData;
    pInst->gsWMV78DecInit.m_iExtraDataLen   = p_init_param->m_iExtraDataLen;
    pInst->gsWMV78DecInit.m_iFourCC     = p_init_param->m_iFourCC;
    pInst->gsWMV78DecInit.m_pHeapAddress    = (unsigned char*)vdec_malloc((uint32_t)sizeof(unsigned char)*200*1024);
    pInst->gsWMV78DecInit.m_pHuff_tbl_address = (unsigned char*)WMV78_Huff_table;

    pInst->gsWMV78DecInit.m_pCurFrameAddress  = (tYUV420Frame_WMV*)vdec_malloc((uint32_t)sizeof(tYUV420Frame_WMV));
    pInst->gsWMV78DecInit.m_pRef0FrameAddress = (tYUV420Frame_WMV*)vdec_malloc((uint32_t)sizeof(tYUV420Frame_WMV));
    pInst->gsWMV78DecInit.m_pCurFrameAddress->m_pucYPlane   = (unsigned char*)pInst->gsWMV78CurYFrameAddress;
    pInst->gsWMV78DecInit.m_pCurFrameAddress->m_pucUPlane   = (unsigned char*)pInst->gsWMV78CurUFrameAddress;
    pInst->gsWMV78DecInit.m_pCurFrameAddress->m_pucVPlane   = (unsigned char*)pInst->gsWMV78CurVFrameAddress;
    pInst->gsWMV78DecInit.m_pRef0FrameAddress->m_pucYPlane    = (unsigned char*)pInst->gsWMV78Ref0YFrameAddress;
    pInst->gsWMV78DecInit.m_pRef0FrameAddress->m_pucUPlane    = (unsigned char*)pInst->gsWMV78Ref0UFrameAddress;
    pInst->gsWMV78DecInit.m_pRef0FrameAddress->m_pucVPlane    = (unsigned char*)pInst->gsWMV78Ref0VFrameAddress;
    pInst->gsWMV78DecOutput.m_pDecodedData = (tYUV420Frame_WMV*)vdec_malloc((uint32_t)sizeof(tYUV420Frame_WMV));

    pInst->gsWMV78DecInit.m_sCallbackFunc.m_pMalloc  = malloc;
    pInst->gsWMV78DecInit.m_sCallbackFunc.m_pFree    = free;
    pInst->gsWMV78DecInit.m_sCallbackFunc.m_pMemcpy  = memcpy;
    pInst->gsWMV78DecInit.m_sCallbackFunc.m_pMemset  = memset;
    pInst->gsWMV78DecInit.m_sCallbackFunc.m_pRealloc = realloc;
    pInst->gsWMV78DecInit.m_sCallbackFunc.m_pMemmove = memmove;

    pInst->decoded_buf_curIdx = 0;
    pInst->decoded_buf_size = pInst->gsWMV78FrameSize * 1.5;
    pInst->decoded_buf_size = ALIGN_BUFF(pInst->decoded_buf_size, DPV_ALIGN_LEN);

    for(buf_idx=0; buf_idx < (pInst->gsAdditionalFrameCount + 1); buf_idx++)
    {
      pInst->decoded_phyAddr[buf_idx] = (codec_addr_t)vdec_sys_malloc_physical_addr( NULL, tcomx_u32_to_s32(pInst->decoded_buf_size), BUFFER_ELSE, pInst );
      if( pInst->decoded_phyAddr[buf_idx] == 0u )
      {
        DPRINTF( "[SW-WMV12,Err:%d] vdec_vpu pInst->decoded_virtAddr[PA] alloc failed ", ret );
        ret = -(VPU_NOT_ENOUGH_MEM);
        goto vdec_WMV78_end;
      }
      pInst->decoded_virtAddr[buf_idx] = (codec_addr_t)vdec_sys_malloc_virtual_addr( (unsigned long *)pInst->decoded_phyAddr[buf_idx], tcomx_u32_to_s32(pInst->decoded_buf_size), pInst );
      if( pInst->decoded_virtAddr[buf_idx] == 0u )
      {
        DPRINTF( "[SW-WMV12,Err:%d] vdec_vpu pInst->decoded_virtAddr[VA] alloc failed ", ret );
        ret = -(VPU_NOT_ENOUGH_MEM);
        goto vdec_WMV78_end;
      }

      pInst->decoded_buf_maxcnt = tcomx_s32_to_u32(pInst->gsAdditionalFrameCount) + 1u;
      DSTATUS("OUT-Buffer %d ::   PA = %lx, VA = %lx, size = 0x%x!!",
        buf_idx, pInst->decoded_phyAddr[buf_idx], pInst->decoded_virtAddr[buf_idx],   pInst->decoded_buf_size);
    }

    pInst->gsVpuDecInit.m_iBitstreamFormat  = p_init_param->m_iBitstreamFormat;
    pInst->gsFirstFrame = 1;

    DSTATUS( "[SW-WMV12] WMV78_DEC_INIT Enter " );
    ret = pInst->gExtDecFunc( VDEC_INIT, &pInst->gsWMV78DecHandle, (unsigned long *)CONV_PTR(&pInst->gsWMV78DecInit), NULL );
    if( ret != RETCODE_SUCCESS )
    {
      DPRINTF( "[SW-WMV12] WMV78_DEC_INIT failed Error code is 0x%x ", ret );
      ret = -ret;
    } else {
      pInst->gsIsINITdone = 1;
      pInst->vdec_codec_opened = 1;
      DSTATUS( "[SW-WMV12] WMV78_DEC_INIT OK " );
    }
  }
  else if( iOpCode == VDEC_DEC_SEQ_HEADER )
  {
//    vdec_input_t* p_input_param = (vdec_input_t*)pParam1;
    vdec_output_t* p_output_param = (vdec_output_t*)CONV_PTR(pParam2);

    if( pInst->gsFirstFrame != 0)
    {
      DSTATUS( "[SW-WMV12] VDEC_DEC_SEQ_HEADER start " );
      p_output_param->m_pInitialInfo = (vdec_initial_info_t *)(&pInst->gsCommDecInfo);
      ret = WMV78_dec_seq_header(pInst);
      DSTATUS( "[SW-WMV12] VDEC_DEC_SEQ_HEADER - Success " );
      pInst->gsFirstFrame = 0;
    } else {
      ret = RETCODE_SUCCESS;
    }
  }
  else if( iOpCode == VDEC_DECODE )
  {
    const vdec_input_t* p_input_param = (vdec_input_t*)CONV_PTR(pParam1);
    vdec_output_t* p_output_param = (vdec_output_t*)CONV_PTR(pParam2);

    #ifdef PRINT_VPU_INPUT_STREAM
    {
      int kkk;
      unsigned char* p_input = p_input_param->m_pInp[VA];
      int input_size = p_input_param->m_iInpLen;
      tcc_printf("FS = %7d :", input_size);
      for( kkk = 0; kkk < PRINT_BYTES; kkk++ ){
        tcc_printf("%02X ", p_input[kkk] );
      }
      tcc_printf("");
    }
    #endif

    if( pInst->gsFirstFrame != 0)
    {
      DSTATUS( "[SW-WMV12] VDEC_DEC_SEQ_HEADER start " );
      p_output_param->m_pInitialInfo = (vdec_initial_info_t *)(&pInst->gsCommDecInfo);
      (void)WMV78_dec_seq_header(pInst);
      DSTATUS( "[SW-WMV12] VDEC_DEC_SEQ_HEADER - Success " );
      pInst->gsFirstFrame = 0;
    }

    pInst->gsWMV78DecInput.m_pPacketBuff = (unsigned char*)p_input_param->m_pInp[VA];
    pInst->gsWMV78DecInput.m_iPacketBuffSize = p_input_param->m_iInpLen;

    if (pInst->gsWMV78DecInput.m_iPacketBuffSize == 0)
    {
      DPRINTF( "[SW-WMV12] END_OF_FILE ");
    }

    // Start decoding a frame.
    ret = pInst->gExtDecFunc( VDEC_DECODE, &pInst->gsWMV78DecHandle, (unsigned long *)CONV_PTR(&pInst->gsWMV78DecInput), (unsigned long *)CONV_PTR(&pInst->gsWMV78DecOutput) );
    if( ret < 0 )
    {
      DPRINTF( "[SW-WMV12] VDEC_DECODE failed Error code is 0x%x ", ret );
      ret = -ret;
    }
    else {
      tcomx_memcpy( pInst->decoded_virtAddr[pInst->decoded_buf_curIdx], pInst->gsWMV78DecOutput.m_pDecodedData->m_pucYPlane, pInst->gsWMV78FrameSize*1.5*sizeof(unsigned char) );
      p_output_param->m_pDispOut[0][0] = (unsigned char*)pInst->decoded_phyAddr[pInst->decoded_buf_curIdx];
      p_output_param->m_pDispOut[0][1] = (unsigned char*)p_output_param->m_pDispOut[0][0] + pInst->gsWMV78FrameSize;
      p_output_param->m_pDispOut[0][2] = (unsigned char*)p_output_param->m_pDispOut[0][1] + pInst->gsWMV78FrameSize/4;
      p_output_param->m_pDispOut[1][0] = (unsigned char*)pInst->decoded_virtAddr[pInst->decoded_buf_curIdx];
      p_output_param->m_pDispOut[1][1] = (unsigned char*)p_output_param->m_pDispOut[1][0] + pInst->gsWMV78FrameSize;
      p_output_param->m_pDispOut[1][2] = (unsigned char*)p_output_param->m_pDispOut[1][1] + pInst->gsWMV78FrameSize/4;
      p_output_param->m_pCurrOut[0][0] = p_output_param->m_pDispOut[0][0];
      p_output_param->m_pCurrOut[0][1] = p_output_param->m_pDispOut[0][1];
      p_output_param->m_pCurrOut[0][2] = p_output_param->m_pDispOut[0][2];
      p_output_param->m_pCurrOut[1][0] = p_output_param->m_pDispOut[1][0];
      p_output_param->m_pCurrOut[1][1] = p_output_param->m_pDispOut[1][1];
      p_output_param->m_pCurrOut[1][2] = p_output_param->m_pDispOut[1][2];

      pInst->decoded_buf_curIdx = tcomx_uadd32(pInst->decoded_buf_curIdx, 1u);
      if(pInst->decoded_buf_curIdx >= pInst->decoded_buf_maxcnt){
        pInst->decoded_buf_curIdx = 0;
      }
      p_output_param->m_DecOutInfo.m_iPicType          = pInst->gsWMV78DecOutput.m_iPictureType;
      p_output_param->m_DecOutInfo.m_iDecodedIdx       = 0;
      p_output_param->m_DecOutInfo.m_iDispOutIdx       = 0;
      p_output_param->m_DecOutInfo.m_iDecodingStatus   = 1;
      p_output_param->m_DecOutInfo.m_iOutputStatus     = 1;
      p_output_param->m_DecOutInfo.m_iInterlacedFrame  = 0;
      p_output_param->m_DecOutInfo.m_iPictureStructure = 0;

      p_output_param->m_pInitialInfo = &pInst->gsCommDecInfo;
    }
  }
  else if( iOpCode == VDEC_BUF_FLAG_CLEAR )
  {
    ret = RETCODE_SUCCESS;
  }
  else if( iOpCode == VDEC_DEC_FLUSH_OUTPUT)
  {
    ret = RETCODE_SUCCESS;
  }
  else if( iOpCode == VDEC_CLOSE )
  {
    if(pInst->vdec_codec_opened != 0u)
    {
      if ( pInst->gsIsINITdone != 0 )
      {
        ret = pInst->gExtDecFunc( VDEC_CLOSE, &pInst->gsWMV78DecHandle, NULL, NULL );
        if( ret != RETCODE_SUCCESS )
        {
          DPRINTF( "[SW-WMV12] WMV78_DEC_CLOSE failed Error code is 0x%x ", ret );
          ret = -ret;
        }
      }

      pInst->vdec_codec_opened = 0;
      pInst->gsIsINITdone = 0;

      if ( pInst->gsWMV78DecInit.m_pHeapAddress != NULL ){
        vdec_free(pInst->gsWMV78DecInit.m_pHeapAddress);
      }
      if ( pInst->gsWMV78DecOutput.m_pDecodedData != NULL ){
        vdec_free(pInst->gsWMV78DecOutput.m_pDecodedData);
      }
      if ( pInst->gsWMV78DecInit.m_pCurFrameAddress != NULL ){
        vdec_free(pInst->gsWMV78DecInit.m_pCurFrameAddress);
      }
      if ( pInst->gsWMV78DecInit.m_pRef0FrameAddress != NULL ){
        vdec_free(pInst->gsWMV78DecInit.m_pRef0FrameAddress);
      }

      pInst->gsWMV78DecInit.m_pHeapAddress = NULL;
      pInst->gsWMV78DecOutput.m_pDecodedData = NULL;
      pInst->gsWMV78DecInit.m_pCurFrameAddress = NULL;
      pInst->gsWMV78DecInit.m_pRef0FrameAddress = NULL;

      vdec_sys_free_virtual_addr(pInst->gsFrameBufAddr[VA], pInst->gsFrameBufSize);
      pInst->gsBitstreamBufAddr[VA] = 0u;

      if( pInst->gsWMV78CurYFrameAddress != NULL ){
        vdec_free( pInst->gsWMV78CurYFrameAddress );
      }
      if( pInst->gsWMV78Ref0YFrameAddress != NULL ){
        vdec_free( pInst->gsWMV78Ref0YFrameAddress );
      }
      pInst->gsWMV78CurYFrameAddress = NULL;
      pInst->gsWMV78Ref0YFrameAddress = NULL;

      for(buf_idx =0; buf_idx < pInst->decoded_buf_maxcnt; buf_idx++)
      {
        vdec_sys_free_virtual_addr( pInst->decoded_virtAddr[buf_idx], tcomx_u32_to_s32(pInst->decoded_buf_size) );
        pInst->decoded_virtAddr[buf_idx] = 0u;
      }

      WMV78_CloseLibrary(pInst);
      vpu_env_close(pInst);
    } else {
      ret = -RETCODE_NOT_INITIALIZED;
    }
  }
  else
  {
    DPRINTF( "[SW-WMV12] Invaild Operation!!" );
    ret = (-ret);
  }
vdec_WMV78_end:
  return ret;
}
#endif //INCLUDE_WMV78_DEC
