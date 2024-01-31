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


#ifndef VPU_SCRIPT_COMMON_H
#define VPU_SCRIPT_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdbool.h>

enum vpu_script_level {
	VS_LOG_NONE = 0,
	VS_LOG_ERROR,
	VS_LOG_WARN,
	VS_LOG_INFO,
	VS_LOG_DEBUG,
	VS_LOG_MAX
};

#define DLOG(level, part, fmt, args...)	vpu_script_log_print(__FUNCTION__, __LINE__, level, part, fmt, ##args)

int vpu_script_log_env(void);

void vpu_script_log_print(const char* function, const int line, enum vpu_script_level log_level, const char* part_name, const char* format, ...);

int vpu_cmd_extract_substring(const char *inputString, const char *inputWord, char *outputSubstring, int *outputLength);

int vpu_cmd_extract_substring_output_address(const char *inputString, const char *inputWord, char* addr1, char* addr2, char* addr3);

#endif //VPU_SCRIPT_COMMON_H
