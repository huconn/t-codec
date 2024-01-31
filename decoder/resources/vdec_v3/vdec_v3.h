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


#ifndef VDEC_V3_H
#define VDEC_V3_H

typedef void* vdec_handle_h;

#if defined(CONFIG_ARM64)
typedef long long vdec_addr_t;   //!< handle - 64bit
#else
typedef long vdec_addr_t;        //!< handle - 32bit
#endif

enum vdec_return
{
	VDEC_RETURN_FAIL = -1,
	VDEC_RETURN_SUCCESS = 0,
	VDEC_RETURN_NEED_MORE_DATA,
	VDEC_RETURN_BUFFER_NOT_CONSUMED,
	VDEC_RETURN_CODEC_FINISH,

	VDEC_RETURN_MAX
};

enum vdec_address
{
    VDEC_PA = 0, // Physical address
	VDEC_VA,  // User virtual address
	VDEC_KVA, // Kernel virtual address
	VDEC_ADDR_MAX,
};

enum vdec_log_level {
	VDEC_LOG_NONE = 0,
	VDEC_LOG_ERROR,
	VDEC_LOG_WARN,
	VDEC_LOG_INFO,
	VDEC_LOG_DEBUG,
	VDEC_LOG_VERBOSE
};

enum vdec_codec_id
{
	VDEC_CODEC_NONE = 0,
    VDEC_CODEC_AVC,
    VDEC_CODEC_VC1,
    VDEC_CODEC_MPEG2,
    VDEC_CODEC_MPEG4,
    VDEC_CODEC_H263,
    VDEC_CODEC_DIVX,
    VDEC_CODEC_AVS,
    VDEC_CODEC_MJPG,
    VDEC_CODEC_VP8,
    VDEC_CODEC_MVC,
    VDEC_CODEC_HEVC,
    VDEC_CODEC_VP9,

	VDEC_CODEC_MAX
};

enum vdec_pmap_type
{
	VDEC_PMAP_DEC = 0,
	VDEC_PMAP_DEC_EXT,
	VDEC_PMAP_DEC_EXT2,
	VDEC_PMAP_DEC_EXT3,
	VDEC_PMAP_DEC_EXT4,
	VDEC_PMAP_ENC,
	VDEC_PMAP_ENC_EXT,
	VDEC_PMAP_ENC_EXT2,
	VDEC_PMAP_ENC_EXT3,
	VDEC_PMAP_ENC_EXT4,
	VDEC_PMAP_ENC_EXT5,
	VDEC_PMAP_ENC_EXT6,
	VDEC_PMAP_ENC_EXT7,
	VDEC_PMAP_ENC_EXT8,
	VDEC_PMAP_ENC_EXT9,
	VDEC_PMAP_ENC_EXT10,
	VDEC_PMAP_ENC_EXT11,
	VDEC_PMAP_ENC_EXT12,
	VDEC_PMAP_ENC_EXT13,
	VDEC_PMAP_ENC_EXT14,
	VDEC_PMAP_ENC_EXT15,

	VDEC_PMAP_MAX,
};

enum vdec_component
{
    VDEC_COMP_Y = 0,
    VDEC_COMP_U,
    VDEC_COMP_V,

    VDEC_COMP_MAX,
};

enum vdec_picture_type
{
	VDEC_PICTYPE_I = 0,
    VDEC_PICTYPE_P,
	VDEC_PICTYPE_B,
	VDEC_PICTURE_B_PB,
	VDEC_PICTURE_BI,
	VDEC_PICTURE_IDR,
	VDEC_PICTURE_SKIP,
	VDEC_PICTYPE_UNKNOWN,

	VDEC_PICTYPE_MAX,
};

enum vdec_display_status
{
	VDEC_DISP_STAT_FAIL = 0,
	VDEC_DISP_STAT_SUCCESS,

	VDEC_DISPLAY_MAX,
};

enum vdec_decoded_status
{
	VDEC_STAT_NONE = 0,
	VDEC_STAT_SUCCESS,
	VDEC_STAT_INFO_NOT_SUFFICIENT_SPS_PPS_BUFF,
	VDEC_STAT_INFO_NOT_SUFFICIENT_SLICE_BUFF,
	VDEC_STAT_BUF_FULL,
	VDEC_STAT_SUCCESS_FIELD_PICTURE,
	VDEC_STAT_DETECT_RESOLUTION_CHANGE,
	VDEC_STAT_INVALID_INSTANCE,
	VDEC_STAT_DETECT_DPB_CHANGE,
	VDEC_STAT_QUEUEING_FAIL,
	VDEC_STAT_VP9_SUPER_FRAME,
	VDEC_STAT_CQ_EMPTY,
	VDEC_STAT_REPORT_NOT_READY,

	VDEC_STAT_MAX,
};

enum vdec_bs_buffer_mode
{
	VDEC_BS_MODE_RINGBUFFER = (1 << 0),
	VDEC_BS_MODE_LINEARBUFFR = (1 << 1),

	VDEC_BS_MODE_MAX,
};

enum vdec_output_format
{
	VDEC_OUTPUT_LINEAR_YUV420 = 0,
	VDEC_OUTPUT_LINEAR_NV12,
	VDEC_OUTPUT_LINEAR_10_TO_8_BIT_YUV420,
	VDEC_OUTPUT_LINEAR_10_TO_8_BIT_NV12,
	VDEC_OUTPUT_COMPRESSED_MAPCONV,
	VDEC_OUTPUT_COMPRESSED_AFBC,

	VDEC_OUTPUT_MAX,
};

enum vdec_frame_skip_mode
{
	VDEC_FRAMESKIP_AUTO = 0,
	VDEC_FRAMESKIP_DISABLED,
	VDEC_FRAMESKIP_NON_I,
	VDEC_FRAMESKIP_B,

	VDEC_FRAMESKIP_MAX,
};

typedef struct vdec_crop_t
{
	int left;
	int right;
	int top;
	int bottom;
} vdec_crop_t;


#define VDEC_MAX_CAP_STR  (64)
#define VDEC_MAX_CAP  (64)

typedef struct vdec_codec_cap_t
{
	enum vdec_codec_id codec_id;
	char support_codec[VDEC_MAX_CAP_STR];
	char support_profile[VDEC_MAX_CAP_STR];
	char support_level[VDEC_MAX_CAP_STR];
	unsigned int max_width;
	unsigned int max_height;
	unsigned int max_fps;
	enum vdec_bs_buffer_mode support_buffer_mode; // rinbbuffer = 0x01, linearbuffer = 0x10, both 0x11
} vdec_codec_cap_t;

typedef struct vdec_info_t
{
	unsigned int num_of_codec_cap;
	vdec_codec_cap_t decoder_caps[VDEC_MAX_CAP];

	unsigned int max_supported_instance;
	unsigned int current_instance_count;

	unsigned int decoder_id;

	unsigned int ver_major;
	unsigned int ver_minor;
	unsigned int ver_revision;
} vdec_info_t;

typedef struct vdec_init_t
{
	enum vdec_codec_id codec_id;

	unsigned int max_support_width;
	unsigned int max_support_height;

	enum vdec_output_format output_format;
	unsigned int additional_frame_count;

	unsigned int use_forced_pmap_idx;
	enum vdec_pmap_type forced_pmap_idx;

	unsigned int enable_max_framebuffer;
	unsigned int enable_user_data;
	unsigned int enable_user_framebuffer;
	unsigned int enable_ringbuffer_mode;
	unsigned int enable_no_buffer_delay;
	unsigned int enable_avc_field_display;
	unsigned int enable_dma_buf_id;
} vdec_init_t;

typedef struct vdec_seqheader_input_t
{
	unsigned char* input_buffer;
	int input_size;
} vdec_seqheader_input_t;

typedef struct vdec_seqheader_output_t
{
	int pic_width;
	int pic_height;

	unsigned int frame_rate_res;
	unsigned int frame_rate_div;

	int min_frame_buffer_count;
	int min_frame_buffer_size;
	int frame_buffer_format;

	vdec_crop_t pic_crop;

	unsigned int eotf;
	int frame_buf_delay;
	int profile;
	int level;
	int tier;
	int interlace;
	int aspectratio;
	int report_error_reason;
	int bitdepth;
} vdec_seqheader_output_t;

typedef struct vdec_decode_input_t
{
	unsigned char* input_buffer;
	int input_size;

	enum vdec_frame_skip_mode skip_mode;
} vdec_decode_input_t;

typedef struct vdec_mapconv_info_t {
	vdec_addr_t compressed_y[VDEC_ADDR_MAX];
    vdec_addr_t compressed_cb[VDEC_ADDR_MAX];

    vdec_addr_t fbc_y_offset_addr[VDEC_ADDR_MAX];
    vdec_addr_t fbc_c_offset_addr[VDEC_ADDR_MAX];

    unsigned int compression_table_luma_size;
    unsigned int compression_table_chroma_size;

    unsigned int luma_stride;
    unsigned int chroma_stride;

    unsigned int luma_bit_depth;
    unsigned int chroma_bit_depth;

    unsigned int frame_endian;
} vdec_mapconv_info_t;

typedef struct vdec_decode_output_t
{
	unsigned char* display_output[VDEC_ADDR_MAX][VDEC_COMP_MAX]; // pa/va,  y/u/v
	vdec_mapconv_info_t compressed_output;

	//Refers to the values of enum vdec_picture_type.
	//if interlaced_frame == 1, the upper 3 bits represent the type of the top field,
	//while the lower 3 bits represent the type of the bottom field.
	//The format is | 3 bits (top) | 3 bits (bottom) |.
	int pic_type;

	int display_idx;
	int decoded_idx;
	int dma_buf_id;

	enum vdec_display_status display_status;
	enum vdec_decoded_status decoded_status;

	int interlaced_frame;
	int num_of_err_mbs;
	int decoded_width;
	int decoded_height;
	int display_width;
	int display_height;
	int dma_buf_align_width;
	int dma_buf_align_height;

	vdec_crop_t decoded_crop;
	vdec_crop_t display_crop;

	vdec_addr_t userdata_buf_addr[VDEC_ADDR_MAX];
	int userdata_buffer_size;

	int picture_structure;
	int top_field_first;
} vdec_decode_output_t;

typedef struct vdec_buf_clear_t
{
	unsigned int display_index;
	int dma_buf_id;
} vdec_buf_clear_t;

enum vdec_return vdec_init(vdec_handle_h handle, vdec_init_t* p_init_param);

enum vdec_return vdec_seq_header(vdec_handle_h handle, vdec_seqheader_input_t* p_input_param, vdec_seqheader_output_t* p_output_param);

enum vdec_return vdec_decode(vdec_handle_h handle, vdec_decode_input_t* p_input_param, vdec_decode_output_t* p_output_param);

enum vdec_return vdec_buf_clear(vdec_handle_h handle, vdec_buf_clear_t* p_input_param);

enum vdec_return vdec_drain(vdec_handle_h handle, vdec_decode_output_t* p_output_param);

enum vdec_return vdec_flush(vdec_handle_h handle);

enum vdec_return vdec_close(vdec_handle_h handle);

vdec_handle_h vdec_alloc_instance(vdec_info_t* p_info);

void vdec_release_instance(vdec_handle_h handle);

int vdec_set_log_level(enum vdec_log_level log_level);

#endif //VDEC_V3_H

