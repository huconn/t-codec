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


#ifndef VENC_V3_H
#define VENC_V3_H

typedef void* venc_handle_h;

enum venc_address
{
    VENC_PA = 0, // Physical address
    VENC_KVA = 1, // Kernel virtual address
	VENC_VA = 2,  // User virtual address
	VENC_ADDR_MAX,
};

enum venc_log_level {
	VENC_LOG_NONE = 0,
	VENC_LOG_ERROR,
	VENC_LOG_WARN,
	VENC_LOG_INFO,
	VENC_LOG_DEBUG,
	VENC_LOG_VERBOSE
};

enum venc_codec_id
{
	VENC_CODEC_NONE = 0,
    VENC_CODEC_H264,
	VENC_CODEC_H263,
	VENC_CODEC_MPEG4,
    VENC_CODEC_HEVC,
    VENC_CODEC_MJPG,

	VENC_CODEC_MAX
};

enum venc_source_format
{
	VENC_SOURCE_NONE = 0,
    VENC_SOURCE_YUV420P, /* YUV420 planar (full planer) : Y field + U field + V field */
    VENC_SOURCE_NV12, /* NV12, YUV420SP(semi planer), YUV420 interleaved : Y field + UV field. */

	VENC_SOURCE_MAX,
};

enum venc_picture_type
{
	VENC_PICTYPE_NONE = 0,
    VENC_PICTYPE_I,
    VENC_PICTYPE_P,
	VENC_PICTYPE_B,

	VENC_PICTYPE_MAX,
};

#define VENC_MAX_CAP_STR  (64)
#define VENC_MAX_CAP  (64)

typedef struct venc_codec_cap_t
{
    enum venc_codec_id codec_id;
    char support_codec[VENC_MAX_CAP_STR];
    char support_profile[VENC_MAX_CAP_STR];
    char support_level[VENC_MAX_CAP_STR];
    unsigned int max_width;
    unsigned int max_height;
    unsigned int max_fps;
} venc_codec_cap_t;

typedef struct venc_info_t
{
    unsigned int num_of_codec_cap;
	venc_codec_cap_t encoder_caps[VENC_MAX_CAP];

    unsigned int max_supported_instance;
    unsigned int current_instance_count;

	unsigned int encoder_id;

	unsigned int ver_major;
	unsigned int ver_minor;
	unsigned int ver_revision;
} venc_info_t;

typedef struct venc_init_t
{
    enum venc_codec_id codec;
	enum venc_source_format source_format;

    int pic_width;                    //!< Width  : multiple of 16
    int pic_height;                   //!< Height : multiple of 16
    int framerate;                   //!< Frame rate
    int bitrateKbps;                  //!< Target bit rate in Kbps. if 0, there will be no rate control,
                                        //!< and pictures will be encoded with a quantization parameter equal to quantParam

    int key_interval;                 //!< Key interval : max 32767
    int avc_fast_encoding;             //!< fast encoding for AVC( 0: default, 1: encode intra 16x16 only )

    //! Options
    int slice_mode;
    int slice_size_mode;
    int slice_size;

    int encoding_quality;                  //!< jpeg encoding quality

    int deblk_disable;                //!< 0 : Enable, 1 : Disable, 2 Disable at slice boundary
    int vbv_buffer_size;               //!< Reference decoder buffer size in bits(0 : ignore)

    int initial_qp;
    int max_i_qp;
    int max_p_qp;
    int min_i_qp;
    int min_p_qp;
    int idr_frame_encoding;            //!< 1 : Enable, 0 : Disable, default is 0 (H.264 only)
} venc_init_t;

typedef struct venc_seq_header_t
{
    unsigned char* seq_header_out;     //!< [out] Seqence header pointer
    int seq_header_out_size;       	    //!< [out] Seqence header size
} venc_seq_header_t;

typedef struct venc_input_t
{
    unsigned char* input_y;
    unsigned char* input_crcb[2];

    int change_rc_param_flag;   //0: disable, 3:enable(change a bitrate), 5: enable(change a framerate), 7:enable(change bitrate and framerate)
    int change_target_bitrate_kbps;
    int change_framerate;
    int quant_param;

    unsigned char request_IntraFrame;
} venc_input_t;

typedef struct venc_output_t
{
    unsigned char* bitstream_out;
    int bitstream_out_size;

    enum venc_picture_type pic_type;
} venc_output_t;

int venc_init(venc_handle_h handle, venc_init_t* p_init);

int venc_put_seqheader(venc_handle_h handle, venc_seq_header_t* p_seqhead);

int venc_encode(venc_handle_h handle, venc_input_t* p_input, venc_output_t* p_output);

int venc_close(venc_handle_h handle);

venc_handle_h venc_alloc_instance(venc_info_t* p_output);

void venc_release_instance(venc_handle_h handle);

int venc_set_log_level(enum venc_log_level log_level);

#endif //VENC_V3_H