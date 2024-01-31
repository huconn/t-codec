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


#define LOG_TAG "VPU_DEC_K_COMMON"

#include "vdec.h"

#include <sys/mman.h>
#include <errno.h>

#include <sys/ioctl.h>
#if defined(USE_COMMON_KERNEL_LOCATION)
#include <tcc_vpu_ioctl.h>
#if defined(TCC_JPU_INCLUDE)
#include <tcc_jpu_ioctl.h>
#endif
#if defined(TCC_HEVC_INCLUDE)
#include <tcc_hevc_ioctl.h>
#endif
#if defined(TCC_VPU_4K_D2_INCLUDE)
#include <tcc_4k_d2_ioctl.h>
#endif
#else //use chipset folder
#include <mach/tcc_vpu_ioctl.h>
#if defined(TCC_JPU_INCLUDE)
#include <mach/tcc_jpu_ioctl.h>
#endif
#if defined(TCC_HEVC_INCLUDE)
#include <mach/tcc_hevc_ioctl.h>
#endif
#endif

#include <dlfcn.h>

#include <memory.h>
#include <stdio.h>
#include <string.h>

#if defined(TC_SECURE_MEMORY_COPY)
extern int
TC_SecureMemoryCopy(
  uint32_t paTarget,
  uint32_t paSource,
  uint32_t nSize
);
#endif

static uint32_t total_opened_decoder = 0;
static uint32_t vpu_opened_count = 0;
#define VPU_MGR_NAME  "/dev/vpu_dev_mgr"
char *dec_devices[4] =
{
  "/dev/vpu_vdec",
  "/dev/vpu_vdec_ext",
  "/dev/vpu_vdec_ext2",
  "/dev/vpu_vdec_ext3"
};

#ifdef TCC_HEVC_INCLUDE
static uint32_t hevc_opened_count = 0;
#define HEVC_MGR_NAME "/dev/hevc_dev_mgr"
#endif

#ifdef TCC_VPU_4K_D2_INCLUDE
static uint32_t vpu_4kd2_opened_count = 0;
#define VPU_4K_D2_MGR_NAME  "/dev/vpu_4k_d2_dev_mgr"
#endif

#ifdef TCC_JPU_INCLUDE
static uint32_t jpu_opened_count = 0;
#define JPU_MGR_NAME  "/dev/jpu_dev_mgr"
#endif
#include <fcntl.h>     // O_RDWR
#include <sys/poll.h>

#if defined(TCC_VPU_4K_D2_INCLUDE) && defined(USE_HDR_CONTROL) //temp-helena!!
/** for HDR Control !! **/
#define u8 unsigned char
#define u16 unsigned short
#define u32 uint32_t
#include <stdint.h>
#include "hdmi_ioctls.h"
#define HDMI_DEV_NAME    "/dev/dw-hdmi-tx"
/***********************/
#endif

#define LEVEL_MAX  (11)
#define PROFILE_MAX  (6)

static const char *strProfile[VCODEC_MAX][PROFILE_MAX] =
{
  //STD_AVC
  { "Base Profile", "Main Profile", "Extended Profile", "High Profile", "Reserved Profile", "Reserved Profile" },
  //STD_VC1
  { "Simple Profile", "Main Profile", "Advance Profile", "Reserved Profile", "Reserved Profile", "Reserved Profile" },
  //STD_MPEG2
  { "High Profile", "Spatial Scalable Profile", "SNR Scalable Profile", "Main Profile", "Simple Profile", "Reserved Profile" },
  //STD_MPEG4
  { "Simple Profile", "Advanced Simple Profile", "Advanced Code Efficiency Profile", "Reserved Profile", "Reserved Profile", "Reserved Profile" },
  //STD_H263
  { " ", " ", " ", " ", " ", " " },
  //STD_DIV3
  { " ", " ", " ", " ", " ", " " },
  //STD_EXT
  { "Real video Version 8", "Real video Version 9", "Real video Version 10", " ", " ", " " },
  //STD_AVS
  { "Jizhun Profile", " ", " ", " ", " ", " " },
  //STD_WMV78
  { " ", " ", " ", " ", " ", " " },
  //STD_SORENSON263
  { " ", " ", " ", " ", " ", " " },
  //STD_MJPG
  { " ", " ", " ", " ", " ", " " },
  //STD_VP8
  { " ", " ", " ", " ", " ", " " },
  //STD_THEORA
  { " ", " ", " ", " ", " ", " " },
  //???
  { " ", " ", " ", " ", " ", " " },
  //STD_MVC
  { "Base Profile", "Main Profile", "Extended Profile", "High Profile", "Reserved Profile", "Reserved Profile" }
};

static const char *strLevel[VCODEC_MAX][LEVEL_MAX] =
{
  //STD_AVC
  { "Level_1B", "Level_", "Reserved Level", "Reserved Level", "Reserved Level", "Reserved Level", "Reserved Level", "Reserved Level" },
  //STD_VC1
  { "Level_L0(LOW)", "Level_L1", "Level_L2(MEDIUM)", "Level_L3", "Level_L4(HIGH)", "Reserved Level", "Reserved Level", "Reserved Level" },
  //STD_MPEG2
  { "High Level", "High 1440 Level", "Main Level", "Low Level", "Reserved Level", "Reserved Level", "Reserved Level", "Reserved Level" },
  //STD_MPEG4
  { "Level_L0", "Level_L1", "Level_L2", "Level_L3", "Level_L4", "Level_L5", "Level_L6", "Reserved Level" },
  //STD_H263
  { "", "", "", "", "", "", "", "" },
  //STD_DIV3
  { "", "", "", "", "", "", "", "" },
  //STD_EXT
  { "", "", "", "", "", "", "", "" },
  //STD_AVS
  {"2.0 Level", "4.0 Level", "4.2 Level", "6.0 Level", "6.2 Level", "", "", ""},
  //STD_WMV78
  { "", "", "", "", "", "", "", "" },
  //STD_SORENSON263
  { "", "", "", "", "", "", "", "" },
  //STD_MJPG
  { "", "", "", "", "", "", "", "" },
  //STD_VP8
  { "", "", "", "", "", "", "", "" },
  //STD_THEORA
  { "", "", "", "", "", "", "", "" },
  //???
  { "", "", "", "", "", "", "", "" },
  //STD_MVC
  { "Level_1B", "Level_", "Reserved Level", "Reserved Level", "Reserved Level", "Reserved Level", "Reserved Level", "Reserved Level" }
};

cdk_func_t* gspfVDecList[VCODEC_MAX] =
{
  vdec_vpu //STD_AVC
  ,vdec_vpu //STD_VC1
  ,vdec_vpu //STD_MPEG2
  ,vdec_vpu //STD_MPEG4
  ,vdec_vpu //STD_H263
  ,vdec_vpu //STD_DIV3  // Should get proper License
#if defined(TCC_VPU_D6_INCLUDE)
  ,0
  ,0
#else
  ,vdec_vpu //STD_EXT // Should get proper License
  ,vdec_vpu //STD_AVS
#endif
#ifdef INCLUDE_WMV78_DEC
  ,vdec_WMV78 //STD_WMV78
#else
  ,0
#endif
#ifdef INCLUDE_SORENSON263_DEC
  ,vdec_sorensonH263dec //STD_SORENSON263   // Should get proper License
#else
  ,vdec_vpu //STD_SH263   // Should get proper License
#endif
#if defined(TCC_JPU_INCLUDE)
  , 0 //vdec_mjpeg_jpu //STD_MJPG
#else
  #if defined(TCC_VPU_D6_INCLUDE)
  ,0
  #else
  ,vdec_vpu //STD_MJPG
  #endif
#endif
  ,vdec_vpu //STD_VP8
#if defined(TCC_VPU_D6_INCLUDE)
  ,0
#else
  ,vdec_vpu //STD_THEORA  // Should get proper License
#endif
  ,vdec_vpu //???   // Should get proper License
  ,vdec_vpu //STD_MVC
#ifdef TCC_HEVC_INCLUDE
  ,vdec_hevc//STD_HEVC
#elif defined(TCC_VPU_4K_D2_INCLUDE)
  ,0 //vdec_vpu_4k_d2//STD_HEVC
#else
  ,0
#endif
#if defined(TCC_VPU_4K_D2_INCLUDE)
  , 0//vdec_vpu_4k_d2 //STD_VP9
#else
  ,0
#endif
  ,0
  ,0
  ,0
};

static unsigned long*vdec_malloc(uint32_t size)
{
  unsigned long* ptr = malloc(size);
  return ptr;
}

static void vdec_free(unsigned long* ptr )
{
  free(ptr);
}

typedef struct tag_MH_Pmap_T
{
  int32_t iSize;

  int32_t iAddrPhy;
  int32_t iAddrVir;
} MH_Pmap_T;

int32_t vdec_sys_remain_memory_size( tc_vdec_ * pVdec )
{
  int32_t sz_freeed_mem = pVdec->vdec_instance_index + VPU_DEC;
  if (ioctl(pVdec->dec_fd, VPU_GET_FREEMEM_SIZE, &sz_freeed_mem) < 0)
  {
    char errmsg[STRERR_BUFFERSIZE];
    if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
      LOGE("can't get errmsg from strerror_r");
    } else {
      LOGE("ioctl(0x%x) error[%s]!!", VPU_GET_FREEMEM_SIZE, errmsg);
    }
  }

  return sz_freeed_mem;
}

uint32_t vdec_sys_final_free_mem( tc_vdec_ * pVdec )
{
  uint32_t sz_freeed_mem = pVdec->vdec_instance_index + VPU_DEC;
  if (ioctl(pVdec->mgr_fd, VPU_GET_FREEMEM_SIZE, &sz_freeed_mem) < 0)
  {
    char errmsg[STRERR_BUFFERSIZE];
    if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
      LOGE("can't get errmsg from strerror_r");
    } else {
      LOGE("ioctl(0x%x) error[%s]!!", VPU_GET_FREEMEM_SIZE, errmsg);
    }
  }

  return sz_freeed_mem;
}

unsigned long * vdec_sys_malloc_physical_addr(unsigned long *remap_addr, int32_t iSize, Buffer_Type type, tc_vdec_ *pVdec)
{
  MEM_ALLOC_INFO_t alloc_mem;
  uint32_t uiSize = tcomx_s32_to_u32(iSize);
  tc_vdec_ * pInst = pVdec;
  tcomx_zero_memset(&alloc_mem, 0x00, sizeof(MEM_ALLOC_INFO_t));

  alloc_mem.request_size = uiSize;
  alloc_mem.buffer_type = type;
  if(ioctl(pInst->dec_fd, V_DEC_ALLOC_MEMORY, &alloc_mem) < 0){
    char errmsg[STRERR_BUFFERSIZE];
    if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
      LOGE("can't get errmsg from strerror_r");
    } else {
      LOGE("ioctl(0x%x) error[%s]!!  request(0x%x)/free(0x%x)",
        V_DEC_ALLOC_MEMORY, errmsg, uiSize, vdec_sys_remain_memory_size(pInst));
    }
  }
  if(remap_addr != NULL){
    *remap_addr = alloc_mem.kernel_remap_addr;
  }

  return (unsigned long *)CONV_PTR( alloc_mem.phy_addr );
}


unsigned long * vdec_sys_malloc_virtual_addr(unsigned long * pPtr, int32_t iSize, tc_vdec_ *pVdec)
{
  unsigned long * map_ptr = MAP_FAILED;
  uint32_t uiSize = tcomx_s32_to_u32(iSize);

  map_ptr = (void *)mmap(NULL, uiSize, PROT_READ | PROT_WRITE, MAP_SHARED, pVdec->dec_fd, pPtr);
  if(MAP_FAILED == map_ptr)
  {
    char errmsg[STRERR_BUFFERSIZE];
    if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
      LOGE("can't get errmsg from strerror_r");
    } else {
      LOGE("mmap failed. fd(%d), addr(%p), size(%d) error %d %s",
        pVdec->dec_fd, pPtr, iSize, errno, errmsg);
    }
  }

  return (MAP_FAILED == map_ptr) ? NULL : (unsigned long *)(map_ptr);
}

int32_t vdec_sys_malloc_dual_addr(codec_addr_t *gsBufAddr, int32_t gsBufSize, uint32_t AlignLen, Buffer_Type type, tc_vdec_ *pInst)
{
  int ret_FrameBufSize;
  uint32_t temp_uiSize = tcomx_s32_to_u32(gsBufSize);
  temp_uiSize = ALIGN_BUFF( temp_uiSize, AlignLen );
  ret_FrameBufSize = tcomx_u32_to_s32(temp_uiSize);
  gsBufAddr[PA] = (codec_addr_t)vdec_sys_malloc_physical_addr(&gsBufAddr[K_VA], ret_FrameBufSize, type, pInst);
  if (gsBufAddr[PA] != 0u) {
    gsBufAddr[VA] = (codec_addr_t)vdec_sys_malloc_virtual_addr((unsigned long *)gsBufAddr[PA], ret_FrameBufSize, pInst);
  }
  return ret_FrameBufSize;
}

void vdec_sys_free_virtual_addr(codec_addr_t pPtr, int32_t iSize)
{
  uint32_t uiSize = tcomx_s32_to_u32(iSize);
  if (pPtr != 0u) {
    if((munmap((unsigned long*)pPtr, uiSize)) < 0)
    {
      LOGE("munmap failed. addr(%lu), size(%u)", pPtr, uiSize);
    }
    //pPtr = NULL;
  }
}

void vpu_update_sizeinfo(int32_t format, uint32_t bps, uint32_t fps,
                         int32_t image_width, int32_t image_height, unsigned long* pVdec)
{
  CONTENTS_INFO info;

  if(pVdec != NULL){
    const tc_vdec_ * pInst = (tc_vdec_ *)CONV_PTR(pVdec);

    info.type = (E_VPU_TYPE)(pInst->vdec_instance_index - (int32_t)VPU_DEC);

#if defined(INCLUDE_WMV78_DEC) || defined(INCLUDE_SORENSON263_DEC)
    if(
  #ifdef INCLUDE_WMV78_DEC
      (format == STD_WMV78)
  #else
      0
  #endif
  #ifdef INCLUDE_SORENSON263_DEC
      || (format == STD_SORENSON263)
  #endif
    )
    {
      bps = 50;
    }
#endif

    info.isSWCodec = 1;
    info.width = AVAILABLE_MAX_WIDTH;
    info.height = AVAILABLE_MAX_HEIGHT;
    info.bitrate = bps;
    info.framerate = (uint32_t)(fps/1000);
    if(ioctl(pInst->mgr_fd, VPU_SET_CLK, &info) < 0){
      char errmsg[STRERR_BUFFERSIZE];
      if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
        LOGE("can't get errmsg from strerror_r");
      } else {
        LOGE("ioctl(VPU_SET_CLK) error[%s]!!", errmsg);
      }
    }
  } else {
    LOGE("vpu_update_sizeinfo :: Instance is null!!");
  }
  return;
}

static int32_t vpu_check_for_video(unsigned char open_status, tc_vdec_ *pVdec)
{
  const tc_vdec_ * pInst = pVdec;
  OPENED_sINFO sInfo;

  sInfo.type = DEC_WITH_ENC;
  if(open_status)
  {
    sInfo.opened_cnt = 1;

    if(ioctl(pInst->mgr_fd, VPU_SET_MEM_ALLOC_MODE, &sInfo) < 0){
      char errmsg[STRERR_BUFFERSIZE];
      if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
        LOGE("can't get errmsg from strerror_r");
      } else {
        LOGE("ioctl(0x%x) error[%s]!!", VPU_SET_MEM_ALLOC_MODE, errmsg);
      }
    }
    if(ioctl(pInst->dec_fd, DEVICE_INITIALIZE, &(pInst->codec_format)) < 0){
      char errmsg[STRERR_BUFFERSIZE];
      if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
        LOGE("can't get errmsg from strerror_r");
      } else {
        LOGE("ioctl(0x%x) error[%s]!!", DEVICE_INITIALIZE, errmsg);
      }
    }
  }

  return 0;
}

int32_t vpu_env_open(int32_t format, uint32_t bps, uint32_t fps, int32_t image_width, int32_t image_height, tc_vdec_ *pVdec)
{
  int32_t vpu_reset = 0;
  int32_t ret = 0;
  tc_vdec_ * pInst = pVdec;
  DSTATUS("In  %s ",__func__);

  vpu_reset = vpu_check_for_video(1, pInst);
  if(vpu_reset < 0){
    LOGE("vpu_env_open error");
    vpu_env_close(pInst);
	  ret = (-1);
    goto end;
  }
  pInst->prev_codec = format;
  vpu_update_sizeinfo(format, bps, fps, image_width, image_height, (unsigned long *)CONV_PTR(pInst));

  pInst->vdec_env_opened = 1;
  pInst->renderered  = 0;
  DSTATUS("Out  %s ",__func__);

  pInst->gsAdditionalFrameCount = VPU_BUFF_COUNT;
  pInst->total_frm = 0;
  pInst->gsFrameBufCount = 0;

  pInst->aspect_ratio = 0;
  pInst->bMaxfb_Mode = 0;

#if defined(VPU_OUT_FRAME_DUMP) || defined(CHANGE_INPUT_STREAM)
  pInst->pFs = NULL;
  pInst->is1st_dec = 1;
  pInst->backup_data = NULL;
  pInst->backup_len = 0;
#endif

#ifdef DEBUG_TIME_LOG
  time_cnt = 0;
  total_dec_time = 0;
#endif

end:
  return ret;
}

void vpu_env_close(tc_vdec_ *pVdec)
{
  DSTATUS("In  %s ",__func__);

  tc_vdec_ * pInst = pVdec;

  pInst->vdec_env_opened = 0;
  (void)vpu_check_for_video(0, pInst);

  if( 0 > ioctl(pInst->dec_fd, V_DEC_FREE_MEMORY, NULL))
  {
    char errmsg[STRERR_BUFFERSIZE];
    if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
      LOGE("can't get errmsg from strerror_r");
    } else {
      LOGE("ioctl(0x%x) error[%s]!!", V_DEC_FREE_MEMORY, errmsg);
    }
  }

#if defined(VPU_OUT_FRAME_DUMP) || defined(CHANGE_INPUT_STREAM)
  if(pInst->pFs){
    fclose(pInst->pFs);
    pInst->pFs = NULL;
  }

  if(pInst->backup_data){
    vdec_free((unsigned long*) pInst->backup_data);
    pInst->backup_data = NULL;
  }
#endif

  DSTATUS("Out  %s ",__func__);
}

void vpu_set_additional_refframe_count(int32_t set_count, unsigned long* pInst)
{
  tc_vdec_ *pInst_dec = (tc_vdec_ *)CONV_PTR(pInst);

  if(pInst == NULL){
    LOGE("vpu_set_additional_refframe_count :: Instance is null!!");
  }
  else {
    pInst_dec->gsAdditionalFrameCount = set_count;
    DSTATUS( "[VDEC-%d] gsAdditionalFrameCount %d", pInst_dec->vdec_instance_index, pInst_dec->gsAdditionalFrameCount );
  }
}

int32_t vpu_get_refframe_count(const unsigned long* pInst)
{
  const tc_vdec_ *pInst_dec = (tc_vdec_ *)CONV_PTR(pInst);
  return pInst_dec->gsTotalFrameCount;
}

#if defined(TCC_HEVC_INCLUDE) || defined(TCC_VPU_4K_D2_INCLUDE)
void notify_hdmi_drm_config(const tc_vdec_ *pInst, const hevc_userdata_output_t *dst)
{
  //LOGD("notify_hdmi_drm_config: handle (%d) eotf (%d/%d) ", pInst->hHDMIDevice, pInst->gsHEVCUserDataEOTF, dst->eotf);

  if (pInst->gsHEVCUserDataEOTF == -1) {
    if (dst->eotf == 0 /* Neither HDR nor HLG */) {
      return; // Do nothing
    }
  }
}
#endif

void save_input_stream(char* name, int32_t size, tc_vdec_ * pVdec, int32_t check_size)
{
#ifdef VPU_IN_FRAME_DUMP
  int32_t i;
  tc_vdec_ * pInst = pVdec;
  unsigned char* ps = (unsigned char*)pInst->gsBitstreamBufAddr[VA];
  uint32_t len = size;
  unsigned char pl[4];
  char pName[256] = {0,};
  int32_t oldDelete_file = 0;
  int32_t newOpen_file = 0;
  uint32_t prev_LenDump = 0;

  pl[3] = (unsigned char)((size & 0xFF000000) >> 24);
  pl[2] = (unsigned char)((size & 0x00FF0000) >> 16);
  pl[1] = (unsigned char)((size & 0x0000FF00) >> 8);
  pl[0] = (unsigned char)((size & 0x000000FF));

  if((check_size!=0) && (MAX_DUMP_CNT > 1)){
    if( pInst->lenDump > MAX_DUMP_LEN ){
      newOpen_file = 1;
      pInst->IdxDump += 1;
      pInst->IdxDump = pInst->IdxDump%MAX_DUMP_CNT;
      prev_LenDump = pInst->lenDump;
      pInst->lenDump = 0;
      if(pInst->IdxDump == 0 && pInst->DelDump == 0){
        pInst->DelDump = 1;
      }

      if( pInst->DelDump ){
        oldDelete_file = 1;
      }
    }
    sprintf(pName, "%s/%d_%d_%s", TARGET_STORAGE, pInst->vdec_instance_index, pInst->IdxDump, name);
    if(newOpen_file){
      LOGD(" Stream-Dump : previous Dump length = %ld -> new file: %s", prev_LenDump, pName);
    }
  }
  else
  {
    sprintf(pName, "%s/%d_%s\n", TARGET_STORAGE, pInst->vdec_instance_index, name);
  }
  //LOGD( "[VDEC - Stream] size: 0x%x 0x%x 0x%x 0x%x, stream: 0x%x 0x%x 0x%x 0x%x 0x%x ", pl[3], pl[2], pl[1], pl[0], ps[0], ps[1], ps[2], ps[3], ps[4]);

  if(1) {
    FILE *pFs;

    if(oldDelete_file){
      pFs = fopen(pName, "w+");
    }
    else{
      pFs = fopen(pName, "ab+");
    }
    if (!pFs) {
      char errmsg[STRERR_BUFFERSIZE];
      if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
        LOGE("can't get errmsg from strerror_r");
      } else {
        LOGE("Cannot open %s [Err: %s]", pName, errmsg);
      }
      return;
    }

    //save length of stream!!
    fwrite( &pl[3], 1, 1, pFs);
    fwrite( &pl[2], 1, 1, pFs);
    fwrite( &pl[1], 1, 1, pFs);
    fwrite( &pl[0], 1, 1, pFs);
    //save stream itself!!
    fwrite( ps, size, 1, pFs);
    fclose(pFs);

    if(check_size){
      pInst->lenDump += (size+4);
    }
    return;
  }

  for(i=0; (i+10 <size) && (i+10 < 100); i += 10){
    DPRINTF_FRAME( "[VDEC - Stream] 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", ps[i], ps[i+1], ps[i+2], ps[i+3], ps[i+4], ps[i+5], ps[i+6], ps[i+7], ps[i+8], ps[i+9] );
  }
#endif
}

#ifdef VPU_OUT_FRAME_DUMP
void save_decoded_frame(unsigned char* Y, unsigned char* U, unsigned char *V, int32_t width, int32_t height, tc_vdec_ *pVdec)
{
  FILE *pFs;
  char filename[128];
  unsigned char *Umem = vdec_malloc(width*height/4);
  unsigned char *Vmem = vdec_malloc(width*height/4);
  int copy_size = 0;
  unsigned char *pCbCr = U;

  sprintf(filename, "/home/root/frame-%d-%d.yuv", width, height);
  pFs = fopen(filename, "ab+");
  if (!pFs) {
    LOGE("Cannot open %s",filename);
  }
  else {
    for(copy_size=0; copy_size<width*height/4; copy_size++){
      Umem[copy_size] = *pCbCr;
      pCbCr++;
      Vmem[copy_size] = *pCbCr;
      pCbCr++;
    }

    fwrite( Y, width*height, 1, pFs);
    fwrite( Umem, width*height/4, 1, pFs);
    fwrite( Vmem, width*height/4, 1, pFs);

    fclose(pFs);
  }
  vdec_free(Umem);
  vdec_free(Vmem);
}
#endif


#ifdef CHANGE_INPUT_STREAM
void change_input_stream(unsigned char* out, int* len, int32_t type, tc_vdec_ *pVdec)
{
  tc_vdec_ * pInst = pVdec;
  char length[4] = {0,};
  int32_t log = 1;

  if(!pInst->pFs)
  {
    pInst->pFs = fopen(STREAM_NAME, "rb");
    if (!pInst->pFs)
    {
      LOGE("Cannot open '%s'", STREAM_NAME);
      return;
    }
  }

  if(type == VDEC_DECODE && pInst->is1st_dec)
  {
    pInst->is1st_dec = 0;
    tcomx_memcpy( out, pInst->backup_data, pInst->backup_len);
    *len = pInst->backup_len;
    if(log){
      LOGD("DEC => read[%d] :: %p %p %p %p %p %p %p %p %p %p",
        *len, out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7], out[8], out[9]);
    }
    return;
  }

  //read stream-length
  fread( length, 4, 1, pInst->pFs);
  len = (int*)length;
  //*len = (length[3]) | (length[2] << 8) | (length[1] << 16) | (length[0] << 24);

  if(log){
    LOGD("length(0x%x) => :: %p %p %p %p", *len, length[0], length[1], length[2], length[3]);
  }
  //read stream
  fread( out, *len, 1, pInst->pFs);

  if(pInst->is1st_dec){
    if(pInst->backup_data == NULL){
      pInst->backup_data = vdec_malloc(*len);
    }
    else if( len > pInst->backup_len ){
      TCC_realloc(pInst->backup_data, *len);
    }
    tcomx_memcpy(pInst->backup_data, out, *len);
    pInst->backup_len = *len;
  }

  if(log){
    LOGD("read[%d] :: %p %p %p %p %p %p %p %p %p %p",
      *len, out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7], out[8], out[9]);
  }
}
#endif

unsigned char *vpu_getBitstreamBufAddr(uint32_t index, const unsigned long* pVdec)
{
  unsigned char *pBitstreamBufAddr = NULL;
  const tc_vdec_ * pInst = (tc_vdec_ *)CONV_PTR(pVdec);

  if(pInst == NULL){
    LOGE("vpu_getBitstreamBufAddr :: Instance is null!!");
  }
  else {
    if (index == (uint32_t)PA)
    {
      pBitstreamBufAddr = (unsigned char *)CONV_PTR(pInst->gsBitstreamBufAddr[PA]);
    }
    else if (index == (uint32_t)VA)
    {
      pBitstreamBufAddr = (unsigned char *)CONV_PTR(pInst->gsBitstreamBufAddr[VA]);
    }
    else /* default : PA */
    {
      pBitstreamBufAddr = (unsigned char *)CONV_PTR(pInst->gsBitstreamBufAddr[PA]);
    }
  }
  return pBitstreamBufAddr;
}

unsigned char *vpu_getFrameBufVirtAddr(unsigned char *convert_addr, uint32_t base_index, const tc_vdec_* pVdec)
{
  unsigned char *pRetAddr;

  if(pVdec == NULL){
    LOGE("vpu_getFrameBufVirtAddr :: Instance is null!!");
    pRetAddr = NULL;
  }
  else {
    unsigned char *pTargetBaseAddr = NULL;
    uintptr_t baseAddr = 0;
    uintptr_t szAddrGap = 0;
    const tc_vdec_ * pInst = pVdec;

	pTargetBaseAddr = (unsigned char*)CONV_PTR(pInst->gsFrameBufAddr[VA]);

    if (base_index == (uint32_t)K_VA)
    {
      baseAddr = CONV_PTR(pInst->gsFrameBufAddr[K_VA]);
    }
    else /* default : PA */
    {
      baseAddr = CONV_PTR(pInst->gsFrameBufAddr[PA]);
    }

    szAddrGap = (uintptr_t)convert_addr - baseAddr;

    pRetAddr = &pTargetBaseAddr[szAddrGap];
  }
  return pRetAddr;
}

int32_t vpu_getBitstreamBufSize(const unsigned long* pVdec)
{
  const tc_vdec_ * pInst = (tc_vdec_ *)CONV_PTR(pVdec);
  int32_t bufsize;

  if(pInst == NULL){
    LOGE("vpu_getBitstreamBufSize :: Instance is null!!");
    bufsize = 0;
  }
  else {
    bufsize = pInst->gsBitstreamBufSize;
  }

  return bufsize;
}

void set_max_adaptive_resolution(int32_t max_width, int32_t max_height, tc_vdec_ *pVdec) {
  tc_vdec_ * pInst = pVdec;

  if (pInst != NULL) {
    LOGD("set_max_adaptive_resolution: %d x %d", max_width, max_height);
    pInst->mMaxAdaptiveWidth  = max_width;
    pInst->mMaxAdaptiveHeight = max_height;
  }
}

void set_dec_common_info(int32_t w, int32_t h, const pic_crop_t* crop, int32_t interlace, int32_t mjpgFmt, tc_vdec_ *pVdec)
{
  tc_vdec_ * pInst = pVdec;
  uint32_t u_w, u_h, u_real_w, u_real_h;

  pInst->gsCommDecInfo.m_iPicWidth = w;
  pInst->gsCommDecInfo.m_iPicHeight= h;

  u_w = tcomx_s32_to_u32(w);
  u_h = tcomx_s32_to_u32(h);
  u_real_w = tcomx_s32_to_u32(pInst->mRealPicWidth);
  u_real_h = tcomx_s32_to_u32(pInst->mRealPicHeight);

  if( (pInst->gsCommDecInfo.m_iPicCrop.m_iCropRight == 0u) && (pInst->gsCommDecInfo.m_iPicCrop.m_iCropBottom == 0u) )
  {
    pInst->gsCommDecInfo.m_iPicCrop.m_iCropLeft   = crop->m_iCropLeft;
    pInst->gsCommDecInfo.m_iPicCrop.m_iCropTop  = crop->m_iCropTop;
    pInst->gsCommDecInfo.m_iPicCrop.m_iCropRight  = tcomx_usub32(u_w, tcomx_uadd32(u_real_w, crop->m_iCropLeft));
    pInst->gsCommDecInfo.m_iPicCrop.m_iCropBottom   = tcomx_usub32(u_h, tcomx_uadd32(u_real_h, crop->m_iCropTop));
  }

  pInst->gsCommDecInfo.m_iInterlace = interlace;
  pInst->gsCommDecInfo.m_iMjpg_sourceFormat= mjpgFmt;

  DSTATUS("[COMM-%d] common-Info %d - %d - %d, %d - %d - %d, inter(%d)/mjpgFmt(%d) ",
    pInst->vdec_instance_index, pInst->gsCommDecInfo.m_iPicWidth,
    pInst->gsCommDecInfo.m_iPicCrop.m_iCropLeft, pInst->gsCommDecInfo.m_iPicCrop.m_iCropRight,
    pInst->gsCommDecInfo.m_iPicHeight, pInst->gsCommDecInfo.m_iPicCrop.m_iCropTop, pInst->gsCommDecInfo.m_iPicCrop.m_iCropBottom,
    pInst->gsCommDecInfo.m_iInterlace, pInst->gsCommDecInfo.m_iMjpg_sourceFormat);
}

void print_dec_initial_info( dec_init_t* pDecInit, dec_initial_info_t* pInitialInfo, tc_vdec_ *pVdec)
{
  uint32_t fRateInfoRes = pInitialInfo->m_uiFrameRateRes;
  uint32_t fRateInfoDiv = pInitialInfo->m_uiFrameRateDiv;
  const tc_vdec_ * pInst = pVdec;
  int32_t profile = 0;
  int32_t level =0;

  DSTATUS("[VDEC-%d]---------------VIDEO INITIAL INFO-----------------", pInst->vdec_instance_index);
  if (pDecInit->m_iBitstreamFormat == STD_MPEG4) {
    DSTATUS("[VDEC-%d] Data Partition Enable Flag [%1d]", pInst->vdec_instance_index , pInitialInfo->m_iM4vDataPartitionEnable);
    DSTATUS("[VDEC-%d] Reversible VLC Enable Flag [%1d]", pInst->vdec_instance_index , pInitialInfo->m_iM4vReversibleVlcEnable);
  if (pInitialInfo->m_iM4vShortVideoHeader != 0) {
    DSTATUS("[VDEC-%d] Short Video Header", pInst->vdec_instance_index);
    DSTATUS("[VDEC-%d] AnnexJ Enable Flag [%1d]", pInst->vdec_instance_index , pInitialInfo->m_iM4vH263AnnexJEnable);
  } else
    DSTATUS("[VDEC-%d] Not Short Video", pInst->vdec_instance_index);
  }

  switch(pDecInit->m_iBitstreamFormat) {
  case STD_MPEG2:
    profile = ((pInitialInfo->m_iProfile==0) || (pInitialInfo->m_iProfile>5)) ? 5 : (pInitialInfo->m_iProfile-1);
    level = (pInitialInfo->m_iLevel==4) ? 0 :((pInitialInfo->m_iLevel==6) ? 1 : ((pInitialInfo->m_iLevel==8) ? 2 : ((pInitialInfo->m_iLevel==10) ? 3 : 4)));
    break;
  case STD_MPEG4:
    if ((pInitialInfo->m_iLevel & 0x80) != 0)
    {
      // if VOS Header
      if ((pInitialInfo->m_iLevel == 8) && (pInitialInfo->m_iProfile == 0)) {
        level = 0;
        profile = 0; // Simple, Level_L0
      } else {
        switch(pInitialInfo->m_iProfile) {
          case 0xB:
            profile = 2;
            break;
          case 0xF:
            if( (pInitialInfo->m_iLevel & 8) == 0){
              profile = 1;
            } else {
              profile = 5;
            }
            break;
          case 0x0:
          	profile = 0;
          	break;
          default :
          	profile = 5;
          	break;
        }
        level = pInitialInfo->m_iLevel;
      }
      DSTATUS("[VDEC-%d] VOS Header:%d, %d", pInst->vdec_instance_index , profile, level);
    }
    else
    {
      // Vol Header Only
      level = 7; // reserved level
      switch(pInitialInfo->m_iProfile) {
      case  0x1:
      	profile = 0;
      	break; // simple object
      case  0xC:
      	profile = 2;
      	break; // advanced coding efficiency object
      case 0x11:
      	profile = 1;
      	break; // advanced simple object
      default:
      	profile = 5;
      	break; // reserved
      }
      DSTATUS("[VDEC-%d] VOL Header:%d, %d", pInst->vdec_instance_index , profile, level);
    }

    if( level > 7 ){
      level = 0;
    }
    break;
  case STD_VC1:
    profile = pInitialInfo->m_iProfile;
    level = pInitialInfo->m_iLevel;
    break;
  case STD_AVC:
  case STD_MVC:
    profile = (pInitialInfo->m_iProfile==66) ? 0 : (pInitialInfo->m_iProfile==77) ? 1 : (pInitialInfo->m_iProfile==88) ? 2 : (pInitialInfo->m_iProfile==100) ? 3 : 4;
    if(profile<3) {
      // BP, MP, EP
      level = ((pInitialInfo->m_iLevel==11) && (pInitialInfo->m_iAvcConstraintSetFlag[3] == 1)) ? 0 /*1b*/
      : (((pInitialInfo->m_iLevel>=10) && (pInitialInfo->m_iLevel <= 51)) ? 1 : 2);
    } else {
      // HP
      level = (pInitialInfo->m_iLevel==9) ? 0 : (((pInitialInfo->m_iLevel>=10) && (pInitialInfo->m_iLevel <= 51)) ? 1 : 2);
    }
    break;
  case STD_EXT:
    profile = pInitialInfo->m_iProfile - 8;
    level = pInitialInfo->m_iLevel;
    break;
  case STD_H263:
    profile = pInitialInfo->m_iProfile;
    level = pInitialInfo->m_iLevel;
    break;
  case STD_DIV3:
    profile = pInitialInfo->m_iProfile;
    level = pInitialInfo->m_iLevel;
    break;
#if defined(INCLUDE_WMV78_DEC) || defined(INCLUDE_SORENSON263_DEC)
#ifdef INCLUDE_SORENSON263_DEC
  case STD_SORENSON263:
#endif
#ifdef INCLUDE_WMV78_DEC
  case STD_WMV78:
#endif
    profile = 0;
    level = 0;
    break;
#endif
  default: // STD_MJPG
    break;
  }

  if( (level >= LEVEL_MAX) || (level < 0) )
  {
    DSTATUS("[VDEC-%d] Invalid \"level\" value: %d", pInst->vdec_instance_index , level);
    level = 0;
  }
  if( (profile >= PROFILE_MAX) || (profile < 0) )
  {
    DSTATUS("[VDEC-%d] Invalid \"profile\" value: %d", pInst->vdec_instance_index , profile);
    profile = 0;
  }
  if( (pDecInit->m_iBitstreamFormat >= VCODEC_MAX) || (pDecInit->m_iBitstreamFormat < 0) )
  {
    DSTATUS("[VDEC-%d] Invalid \"m_iBitstreamFormat\" value: %d", pInst->vdec_instance_index , pDecInit->m_iBitstreamFormat);
    pDecInit->m_iBitstreamFormat = 0;
  }

  // No Profile and Level information in WMV78
  if(
#ifdef INCLUDE_WMV78_DEC
  (pDecInit->m_iBitstreamFormat != STD_WMV78) &&
#endif
  (pDecInit->m_iBitstreamFormat != STD_MJPG)
  )
  {
    DSTATUS("[VDEC-%d] %s", pInst->vdec_instance_index , strProfile[pDecInit->m_iBitstreamFormat][profile]);
    if (pDecInit->m_iBitstreamFormat != STD_EXT) { // No level information in Ext.
      if (((pDecInit->m_iBitstreamFormat == STD_AVC) || (pDecInit->m_iBitstreamFormat == STD_MVC))
      	    && (level != 0) && (level != 2)){
        DSTATUS("[VDEC-%d] %s%.1f", pInst->vdec_instance_index , strLevel[pDecInit->m_iBitstreamFormat][level], (float)pInitialInfo->m_iLevel/10);
      }
      else{
        DSTATUS("[VDEC-%d] %s", pInst->vdec_instance_index , strLevel[pDecInit->m_iBitstreamFormat][level]);
      }
    }
  }

  if((pDecInit->m_iBitstreamFormat == STD_AVC) || (pDecInit->m_iBitstreamFormat == STD_MVC)) {
    DSTATUS("[VDEC-%d] frame_mbs_only_flag : %d", pInst->vdec_instance_index , pInitialInfo->m_iInterlace);
  } else if (pDecInit->m_iBitstreamFormat != STD_EXT) {// No interlace information in Rv.
    if (pInitialInfo->m_iInterlace != 0){
      DSTATUS("[VDEC-%d] %s", pInst->vdec_instance_index , "Interlaced Sequence");
    }
    else{
      DSTATUS("[VDEC-%d] %s", pInst->vdec_instance_index , "Progressive Sequence");
    }
  }

  if (pDecInit->m_iBitstreamFormat == STD_VC1) {
    if (pInitialInfo->m_iVc1Psf != 0){
      DSTATUS("[VDEC-%d] %s", pInst->vdec_instance_index , "VC1 - Progressive Segmented Frame");
    }
    else{
      DSTATUS("[VDEC-%d] %s", pInst->vdec_instance_index , "VC1 - Not Progressive Segmented Frame");
    }
  }

  DSTATUS("[VDEC-%d] Aspect Ratio [%1d]", pInst->vdec_instance_index , pInitialInfo->m_iAspectRateInfo);

  switch (pDecInit->m_iBitstreamFormat) {
  case STD_AVC:
  case STD_MVC:
    DSTATUS("[VDEC-%d] H.264 Profile:%d Level:%d FrameMbsOnlyFlag:%d", pInst->vdec_instance_index ,
      pInitialInfo->m_iProfile, pInitialInfo->m_iLevel, pInitialInfo->m_iInterlace);

    if(pInitialInfo->m_iAspectRateInfo != 0) {
      int32_t aspect_ratio_idc;
      int32_t sar_width, sar_height;

      if( (pInitialInfo->m_iAspectRateInfo>>16)==0 ) {
        aspect_ratio_idc = (pInitialInfo->m_iAspectRateInfo & 0xFF);
        DSTATUS("[VDEC-%d] aspect_ratio_idc :%d", pInst->vdec_instance_index , aspect_ratio_idc);
      }
      else {
        sar_width  = (pInitialInfo->m_iAspectRateInfo >> 16);
        sar_height  = (pInitialInfo->m_iAspectRateInfo & 0xFFFF);
        DSTATUS("[VDEC-%d] sar_width : %d sar_height : %d", pInst->vdec_instance_index , sar_width, sar_height);
      }
    } else {
      LOGE("[VDEC-%d] Aspect Ratio is not present", pInst->vdec_instance_index);
    }
    break;
  case STD_VC1:
    switch(pInitialInfo->m_iProfile) {
      case 0:
        DSTATUS("[VDEC-%d] VC1 Profile: Simple", pInst->vdec_instance_index);
        break;
	  case 1:
        DSTATUS("[VDEC-%d] VC1 Profile: Main", pInst->vdec_instance_index);
        break;
	  case 2:
        DSTATUS("[VDEC-%d] VC1 Profile: Advanced", pInst->vdec_instance_index);
        break;
	  default:
        DSTATUS("[VDEC-%d] VC1 Profile: Unknown (%d)", pInst->vdec_instance_index, pInitialInfo->m_iProfile);
        break;
	}

    DSTATUS("[VDEC-%d] Level: %d Interlace: %d PSF: %d", pInst->vdec_instance_index ,
      pInitialInfo->m_iLevel, pInitialInfo->m_iInterlace, pInitialInfo->m_iVc1Psf);

    if(pInitialInfo->m_iAspectRateInfo != 0){
      DSTATUS("[VDEC-%d] Aspect Ratio [X, Y]:[%3d, %3d]", pInst->vdec_instance_index ,
        (pInitialInfo->m_iAspectRateInfo>>8)&0xffu, (pInitialInfo->m_iAspectRateInfo)&0xffu);
    }
    else{
      DSTATUS("[VDEC-%d] Aspect Ratio is not present", pInst->vdec_instance_index);
    }
    break;
  case STD_MPEG2:
    DSTATUS("[VDEC-%d] Mpeg2 Profile:%d Level:%d Progressive Sequence Flag:%d", pInst->vdec_instance_index ,
      pInitialInfo->m_iProfile, pInitialInfo->m_iLevel, pInitialInfo->m_iInterlace);
    // Profile: 3'b101: Simple, 3'b100: Main, 3'b011: SNR Scalable,
    // 3'b10: Spatially Scalable, 3'b001: High
    // Level: 4'b1010: Low, 4'b1000: Main, 4'b0110: High 1440, 4'b0100: High
    if(pInitialInfo->m_iAspectRateInfo != 0){
      DSTATUS("[VDEC-%d] Aspect Ratio Table index :%d", pInst->vdec_instance_index , pInitialInfo->m_iAspectRateInfo);
    }
    else{
      DSTATUS("[VDEC-%d] Aspect Ratio is not present", pInst->vdec_instance_index);
    }
    break;
  case STD_MPEG4:
    DSTATUS("[VDEC-%d] Mpeg4 Profile: %d Level: %d Interlaced: %d", pInst->vdec_instance_index ,
      pInitialInfo->m_iProfile, pInitialInfo->m_iLevel, pInitialInfo->m_iInterlace);
    // Profile: 8'b00000000: SP, 8'b00010001: ASP
    // Level: 4'b0000: L0, 4'b0001: L1, 4'b0010: L2, 4'b0011: L3, ...
    // SP: 1/2/3/4a/5/6, ASP: 0/1/2/3/4/5

    if(pInitialInfo->m_iAspectRateInfo != 0){
      DSTATUS("[VDEC-%d] Aspect Ratio Table index :%d", pInst->vdec_instance_index , pInitialInfo->m_iAspectRateInfo);
    }
    else{
      DSTATUS("[VDEC-%d] Aspect Ratio is not present", pInst->vdec_instance_index);
    }
    break;
  case STD_EXT:
    DSTATUS("[VDEC-%d] Real Video Version %d", pInst->vdec_instance_index , pInitialInfo->m_iProfile);
    break;
#ifdef INCLUDE_SORENSON263_DEC
  case STD_SORENSON263:
    DSTATUS("[VDEC-%d] Sorenson's H.263 ", pInst->vdec_instance_index);
    break;
#endif
  }

  DSTATUS("[VDEC-%d] Dec InitialInfo => m_iPicWidth: %u m_iPicHeight: %u frameRate: %.2f frRes: %u frDiv: %u",
    pInst->vdec_instance_index, pInitialInfo->m_iPicWidth, pInitialInfo->m_iPicHeight,
    (double)fRateInfoRes/fRateInfoDiv, fRateInfoRes, fRateInfoDiv);

#ifdef INCLUDE_SORENSON263_DEC
  if (pDecInit->m_iBitstreamFormat == STD_SORENSON263) {
    int32_t disp_width = pInitialInfo->m_iPicWidth;
    int32_t disp_height = pInitialInfo->m_iPicHeight;
    int32_t crop_left = pInitialInfo->m_iAvcPicCrop.m_iCropLeft;
    int32_t crop_right = pInitialInfo->m_iAvcPicCrop.m_iCropRight;
    int32_t crop_top = pInitialInfo->m_iAvcPicCrop.m_iCropTop;
    int32_t crop_bottom = pInitialInfo->m_iAvcPicCrop.m_iCropBottom;

    if( crop_left | crop_right | crop_top | crop_bottom ){
      disp_width = disp_width - ( crop_left + crop_right );
      disp_height = disp_height - ( crop_top + crop_bottom );

      DSTATUS("[VDEC-%d] Dec InitialInfo => Display_PicWidth: %u Display_PicHeight: %u", pInst->vdec_instance_index ,
      disp_width, disp_height);
    }
  }
#endif

  DSTATUS("[VDEC-%d] ---------------------------------------------------", pInst->vdec_instance_index);
}


unsigned long* vdec_alloc_instance(int32_t codec_format, int32_t refer_instance)
{
  tc_vdec_ *pInst = NULL;
  char *mgr_name;
  INSTANCE_INFO iInst_info;

  printf("[%s(%d)] [CHECK] \n", __func__, __LINE__);

  pInst = (tc_vdec_*)vdec_malloc(sizeof(tc_vdec_));
  if( pInst != NULL)
  {
    printf("[%s(%d)] [CHECK] tcomx_zero_memset \n", __func__, __LINE__);
    tcomx_zero_memset(pInst, 0x00, sizeof(tc_vdec_));

#if defined (TCC_HEVC_INCLUDE) || defined (TCC_VPU_4K_D2_INCLUDE)
    pInst->gsHEVCUserDataEOTF = -1;
    pInst->hHDMIDevice = -1;
    pInst->gsHLGFLAG = 0;
#endif
    pInst->mgr_fd = -1;
    pInst->dec_fd = -1;
    pInst->codec_format = codec_format;

    pInst->mMaxAdaptiveWidth  = -1;
    pInst->mMaxAdaptiveHeight = -1;

    switch(pInst->codec_format) {
#ifdef TCC_JPU_INCLUDE
      case STD_MJPG:
        mgr_name = JPU_MGR_NAME;
        break;
#endif
#ifdef TCC_HEVC_INCLUDE
      case STD_HEVC:
        mgr_name = HEVC_MGR_NAME;
        break;
#endif
#ifdef TCC_VPU_4K_D2_INCLUDE
      case STD_HEVC:
      case STD_VP9:
        mgr_name = VPU_4K_D2_MGR_NAME;
        break;
#endif
      default:
        mgr_name = VPU_MGR_NAME;
        break;
    }

    printf("[%s(%d)] %s \n", __func__, __LINE__, VPU_MGR_NAME);
    printf("[%s(%d)] mgr_name: %s \n", __func__, __LINE__, mgr_name);

    pInst->mgr_fd = open(VPU_MGR_NAME, O_RDWR);
    if(pInst->mgr_fd < 0)
    {
      char errmsg[STRERR_BUFFERSIZE];
      if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
        LOGE("can't get errmsg from strerror_r");
      } else {
        LOGE("%s open error[%s]!!", VPU_MGR_NAME, errmsg);
      }
      goto MGR_OPEN_ERR;
    }

    iInst_info.type = VPU_DEC;
    iInst_info.nInstance = refer_instance;
    if(ioctl(pInst->mgr_fd, VPU_GET_INSTANCE_IDX, &iInst_info) < 0){
      char errmsg[STRERR_BUFFERSIZE];
      if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
        LOGE("can't get errmsg from strerror_r");
      } else {
        LOGE("%s ioctl(0x%x) error[%s]!!", mgr_name, VPU_GET_INSTANCE_IDX, errmsg);
      }
    }
    if( (iInst_info.nInstance < 0) || (iInst_info.nInstance > 3) )
    {
      goto INST_GET_ERR;
    }

    printf("[%s(%d)] %s \n", __func__, __LINE__, dec_devices[pInst->vdec_instance_index]);
    pInst->vdec_instance_index = iInst_info.nInstance;
    pInst->dec_fd = open(dec_devices[pInst->vdec_instance_index], O_RDWR);
    if(pInst->dec_fd < 0)
    {
      char errmsg[STRERR_BUFFERSIZE];
      if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
        LOGE("can't get errmsg from strerror_r");
      } else {
        LOGE("%s open error[%s]", dec_devices[pInst->vdec_instance_index], errmsg);
      }
      goto DEC_OPEN_ERR;
    }

    total_opened_decoder = tcomx_uadd32(total_opened_decoder, 1u);
    tcc_printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@[%d] alloc Instance[%d] = %s\n", refer_instance, pInst->vdec_instance_index, dec_devices[pInst->vdec_instance_index]);

	switch(pInst->codec_format) {
#ifdef TCC_JPU_INCLUDE
      case STD_MJPG:
        jpu_opened_count = tcomx_uadd32(jpu_opened_count, 1u);
        LOGI("[JPU-%d] %d :: vdec_alloc_instance total %d", pInst->vdec_instance_index, jpu_opened_count, total_opened_decoder);
        break;
#endif
#ifdef TCC_HEVC_INCLUDE
      case STD_HEVC:
        hevc_opened_count = tcomx_uadd32(hevc_opened_count, 1u);
        LOGI("[HEVC-%d] %d :: vdec_alloc_instance total %d", pInst->vdec_instance_index, hevc_opened_count, total_opened_decoder);
        break;
#endif
#ifdef TCC_VPU_4K_D2_INCLUDE
      case STD_HEVC:
      case STD_VP9:
        vpu_4kd2_opened_count = tcomx_uadd32(vpu_4kd2_opened_count, 1u);
        LOGI("[4KD2-%d] %u :: vdec_alloc_instance total %u", pInst->vdec_instance_index, vpu_4kd2_opened_count, total_opened_decoder);
        break;
#endif
      default:
        vpu_opened_count = tcomx_uadd32(vpu_opened_count, 1u);
        LOGI("[VDEC-%d] %u :: vdec_alloc_instance total %u", pInst->vdec_instance_index, vpu_opened_count, total_opened_decoder);
        break;
    }
  }

  return (unsigned long *)CONV_PTR(pInst);

DEC_OPEN_ERR:
  iInst_info.type = VPU_DEC;
  iInst_info.nInstance = pInst->vdec_instance_index;
  if( ioctl(pInst->mgr_fd, VPU_CLEAR_INSTANCE_IDX, &iInst_info) < 0){
    char errmsg[STRERR_BUFFERSIZE];
    if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
      LOGE("can't get errmsg from strerror_r");
    } else {
      LOGE("%s ioctl(0x%x) error[%s]!!", mgr_name, VPU_CLEAR_INSTANCE_IDX, errmsg);
    }
  }
INST_GET_ERR:
  if(close(pInst->mgr_fd) < 0){
    char errmsg[STRERR_BUFFERSIZE];
    if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
      LOGE("can't get errmsg from strerror_r");
    } else {
      LOGE("%s close error[%s]", mgr_name, errmsg);
    }
  }
MGR_OPEN_ERR:
  vdec_free(pInst);
  return NULL;
}

void vdec_release_instance(unsigned long* pInst)
{
  if(pInst != NULL)
  {
    tc_vdec_ * pVdec = (tc_vdec_ *)CONV_PTR(pInst);
    int32_t used_instance = pVdec->vdec_instance_index;
    char *mgr_name;
    INSTANCE_INFO iInst_info;

#if defined(TCC_VPU_4K_D2_INCLUDE) && defined(USE_HDR_CONTROL) //temp-helena
    if (pVdec->hHDMIDevice >= 0) {
      DRM_Packet_t drm_info;
      tcomx_zero_memset(&drm_info, 0x0, sizeof(DRM_Packet_t));

      if (ioctl(pVdec->hHDMIDevice, HDMI_API_DRM_CONFIG, &drm_info) < 0) {
        char errmsg[STRERR_BUFFERSIZE];
        if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
          LOGE("can't get errmsg from strerror_r");
        } else {
          LOGE("HDR: HDMI_API_DRM_CONFIG ioctl Error !!(%d: %s)", errno, errmsg);
        }
      } else {
        LOGE("HDR: HDMI_API_DRM_CONFIG end ioctl !!");
      }

      if (close(pVdec->hHDMIDevice) < 0) {
        char errmsg[STRERR_BUFFERSIZE];
        if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
          LOGE("can't get errmsg from strerror_r");
        } else {
          LOGE("%s close error[%s]", HDMI_DEV_NAME, errmsg);
        }
      }
      pVdec->hHDMIDevice = -1;
      pVdec->gsHEVCUserDataEOTF = -1;
      pVdec->gsHLGFLAG = 0;
    }
#endif

#ifdef TCC_JPU_INCLUDE
    if( pVdec->codec_format == STD_MJPG ){
      mgr_name = JPU_MGR_NAME;
    } else
#endif
#ifdef TCC_HEVC_INCLUDE
  if( pVdec->codec_format == STD_HEVC ){
    mgr_name = HEVC_MGR_NAME;
    } else
#endif
#ifdef TCC_VPU_4K_D2_INCLUDE
    if( pVdec->codec_format == STD_HEVC ){
      mgr_name = VPU_4K_D2_MGR_NAME;
    } else
    if( pVdec->codec_format == STD_VP9 ){
      mgr_name = VPU_4K_D2_MGR_NAME;
    } else
#endif
    {
      mgr_name = VPU_MGR_NAME;
    }

    iInst_info.type = VPU_DEC;
    iInst_info.nInstance = used_instance;
    if( ioctl(pVdec->mgr_fd, VPU_CLEAR_INSTANCE_IDX, &iInst_info) < 0){
      char errmsg[STRERR_BUFFERSIZE];
      if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
        LOGE("can't get errmsg from strerror_r");
      } else {
        LOGE("%s ioctl(0x%x) error[%s]!!", mgr_name, VPU_CLEAR_INSTANCE_IDX, errmsg);
      }
    }

    if(pVdec->dec_fd)
    {
      if(close(pVdec->dec_fd) < 0)
      {
        if ((pVdec->vdec_instance_index < 0) || (pVdec->vdec_instance_index > 3)) {
          LOGE("### vdec_instance_index (%d) is invalid", pVdec->vdec_instance_index);
        }
        else {
          LOGE("%s close error", dec_devices[pVdec->vdec_instance_index]);
        }
      }
      pVdec->dec_fd = -1;
    }

    if(pVdec->mgr_fd)
    {
      if(close(pVdec->mgr_fd) < 0){
        char errmsg[STRERR_BUFFERSIZE];
        if (strerror_r(errno, errmsg, STRERR_BUFFERSIZE) != 0){
          LOGE("can't get errmsg from strerror_r");
        } else {
          LOGE("%s close error[%s]", mgr_name, errmsg);
        }
      }
      pVdec->mgr_fd = -1;
    }

	total_opened_decoder = tcomx_usub32(total_opened_decoder, 1u);
	if ((used_instance < 0) || (used_instance > 3)) {
      LOGE("############################ used_instance(%d) is invalid",used_instance);
    } else {
      LOGD("############################ <==== free Instance[%d] = %s", used_instance, dec_devices[used_instance]);
	}

#ifdef TCC_JPU_INCLUDE
    if( pVdec->codec_format == STD_MJPG ){
      if(jpu_opened_count > 0) {
        jpu_opened_count--;
      }
      LOGI("[JPU-%d] %d :: vdec_release_instance total %d", used_instance, jpu_opened_count, total_opened_decoder);
    }
    else
#endif
#ifdef TCC_HEVC_INCLUDE
    if( pVdec->codec_format == STD_HEVC){
      hevc_opened_count = tcomx_usub32(hevc_opened_count, 1u);
      LOGI("[HEVC-%d] %d :: vdec_release_instance total %d", used_instance, hevc_opened_count, total_opened_decoder);
    }
    else
#endif
#ifdef TCC_VPU_4K_D2_INCLUDE
    if( pVdec->codec_format == STD_HEVC){
      vpu_4kd2_opened_count  = tcomx_usub32(vpu_4kd2_opened_count, 1u);
      LOGI("[4KD2-%d] %u :: vdec_release_instance total %u", used_instance, vpu_4kd2_opened_count, total_opened_decoder);
    }
    else
    if( pVdec->codec_format == STD_VP9){
      vpu_4kd2_opened_count = tcomx_usub32(vpu_4kd2_opened_count, 1u);
      LOGI("[4KD2-%d] %u :: vdec_release_instance total %u", used_instance, vpu_4kd2_opened_count, total_opened_decoder);
    }
    else
#endif
    {
      vpu_opened_count = tcomx_usub32(vpu_opened_count, 1u);
      LOGI("[VDEC-%d] %u :: vdec_release_instance total %u", used_instance, vpu_opened_count, total_opened_decoder);
    }

    vdec_free(pInst);
    pInst = NULL;
  }
}

int32_t vdec_get_instance_index(unsigned long* pInst)
{
  const tc_vdec_ * pVdec = (tc_vdec_ *) CONV_PTR(pInst);
  int32_t idx;

  if(pInst == NULL){
    LOGE("vdec_get_instance_index :: Instance is null!!");
    idx = -1;
  } else {
    idx = pVdec->vdec_instance_index;
  }
  return idx;
}
