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

#include "../video/h264/include/encoder_plugin_h264_version.h"
#include "../video/h265/include/encoder_plugin_h265_version.h"
#include "../video/mjpeg/include/encoder_plugin_mjpeg_version.h"


#define TENCODER_CODEC_INDEX_MAX		(4)
#define TENCODER_CODEC_INFO_NAME 	(0)
#define TENCODER_CODEC_INFO_VERSION 	(2)

static char *tEncoderCodec_Version[TENCODER_CODEC_INDEX_MAX][TENCODER_CODEC_INFO_VERSION] = {
	// video
	{"h264", LIB_DECODERPLUGIN_H264_SOVERSION},
	{"h265", LIB_DECODERPLUGIN_H265_SOVERSION},
	{"mjpeg", LIB_DECODERPLUGIN_MJPEG_SOVERSION},
	{NULL, NULL},
};

static char *get_encoder_plugin_name(const char *EncModule)
{
	char *media_enc_so_ver = NULL;

	if (strlen(EncModule) > 1U) {
		int i = 0;

		for (i = 0; i < TENCODER_CODEC_INDEX_MAX; i++) {
			if (tEncoderCodec_Version[i][0] != NULL) {
				if (strcmp(tEncoderCodec_Version[i][0], EncModule) == 0) {
					media_enc_so_ver = tEncoderCodec_Version[i][1];

					break;
				}
			}
		}
	}

	return media_enc_so_ver;
}