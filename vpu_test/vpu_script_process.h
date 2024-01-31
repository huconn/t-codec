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

#ifndef VPU_SCRIPT_PROCESS_H
#define VPU_SCRIPT_PROCESS_H

typedef void* VPUCmdProc_h;

typedef struct VPUScriptResult_t {
	int test_id;

	int reult;
	int output_count; //display frame or encoded count
	int operation_count; //operation count can be equal to or greater than the output count
	long long elapsed_time_avg;
	long long elapsed_time_max;
} VPUScriptResult_t;

VPUCmdProc_h vpu_proc_alloc(int id);

void vpu_proc_help(void);

void vpu_proc_release(VPUCmdProc_h hProc);

int vpu_proc_parser(VPUCmdProc_h hProc, char* script_buffer, const unsigned int script_size, VPUScriptResult_t* result);

#endif //VPU_SCRIPT_PROCESS_H