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

#include <string.h>

#include "../audio/aac/include/decoder_plugin_aac_version.h"
#include "../audio/mp3/include/decoder_plugin_mp3_version.h"
#include "../audio/pcm/include/decoder_plugin_pcm_version.h"
#include "../audio/ddp/include/decoder_plugin_ddp_version.h"

#include "../video/mjpeg/include/decoder_plugin_mjpeg_version.h"
#include "../video/h263/include/decoder_plugin_h263_version.h"
#include "../video/h264/include/decoder_plugin_h264_version.h"
#include "../video/h265/include/decoder_plugin_h265_version.h"
#include "../video/mpeg2/include/decoder_plugin_mpeg2_version.h"
#include "../video/mpeg4/include/decoder_plugin_mpeg4_version.h"
#include "../video/vp8/include/decoder_plugin_vp8_version.h"
#include "../video/vp9/include/decoder_plugin_vp9_version.h"


#define MEDIA_CODEC_INDEX_MAX		(12)
#define MEDIA_CODEC_INFO_NAME 		(0)
#define MEDIA_CODEC_INFO_VERSION 	(2)

static char *tMediaCodec_Version[MEDIA_CODEC_INDEX_MAX][MEDIA_CODEC_INFO_VERSION] = {
	// audio
	{"aac", LIB_DECODERPLUGIN_AAC_SOVERSION},
	{"mp3", LIB_DECODERPLUGIN_MP3_SOVERSION},
	{"pcm", LIB_DECODERPLUGIN_PCM_SOVERSION},
	{"ddp", LIB_DECODERPLUGIN_DDP_SOVERSION},

	// video
	{"mjpeg", LIB_DECODERPLUGIN_MJPEG_SOVERSION},
	{"h263", LIB_DECODERPLUGIN_H263_SOVERSION},
	{"h264", LIB_DECODERPLUGIN_H264_SOVERSION},
	{"h265", LIB_DECODERPLUGIN_H265_SOVERSION},
	{"mpeg2", LIB_DECODERPLUGIN_MPEG2_SOVERSION},
	{"mpeg4", LIB_DECODERPLUGIN_MPEG4_SOVERSION},
	{"vp8", LIB_DECODERPLUGIN_VP8_SOVERSION},
	{"vp9", LIB_DECODERPLUGIN_VP9_SOVERSION},
	{NULL, NULL},
};

static char *get_decoder_plugin_name(const char *module) {
	char *media_dec_so_ver = NULL;

	if (strlen(module) > 1U) {
		int i = 0;

		for (i = 0; i < MEDIA_CODEC_INDEX_MAX; i++) {
			if (tMediaCodec_Version[i][0] != NULL) {
				if (strcmp(tMediaCodec_Version[i][0], module) == 0) {
					media_dec_so_ver = tMediaCodec_Version[i][1];

					break;
				}
			}
		}
	}

	return media_dec_so_ver;
}
