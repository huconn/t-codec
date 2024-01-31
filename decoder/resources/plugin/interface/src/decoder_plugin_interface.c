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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <dlfcn.h>

#include <decoder_plugin.h>

#include <decoder_plugin_interface.h>

#include <decoder_plugin_versions.h>

//#include <glib/gprintf.h>


DEC_PLUGIN_IF_H decoder_plugin_interface_Load(char *pstrPlugin, DecoderPluginDesc_T **pptPluginDesc)
{
	const char *strPluginFile = NULL;
	DEC_PLUGIN_IF_H hPlugin_IF = NULL;

	strPluginFile = get_decoder_plugin_name(pstrPlugin);

	if (strPluginFile != NULL) {
		hPlugin_IF = dlopen(strPluginFile, RTLD_LAZY);
		*pptPluginDesc = NULL;

		if (hPlugin_IF != NULL) {
			*pptPluginDesc = (DecoderPluginDesc_T *)DecPlugin_CastAddr_VOID_UINTPTR(dlsym(hPlugin_IF, "G_tDecoderPluginDesc"));

			if (*pptPluginDesc == NULL) {
				(void)dlclose(hPlugin_IF);

				hPlugin_IF = NULL;
			}
		}
	}

	return hPlugin_IF;
}

void decoder_plugin_interface_Unload(DEC_PLUGIN_IF_H hPlugin_IF)
{
	if(hPlugin_IF != NULL)
	{
		(void)dlclose(hPlugin_IF);
	}
}