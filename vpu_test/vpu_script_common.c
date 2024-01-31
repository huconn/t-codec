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

#include "vpu_script_common.h"

#include <stdarg.h>

static const char* part_name = "COMMON";
#define DLOGD(...) 		DLOG(VS_LOG_DEBUG, part_name, __VA_ARGS__)	//debug
#define DLOGI(...) 		DLOG(VS_LOG_INFO, part_name, __VA_ARGS__)	//info
#define DLOGE(...) 		DLOG(VS_LOG_ERROR, part_name, __VA_ARGS__)	//error

#define CDBG_DEFAULT	"\x1b[0m"
#define CDBG_BLACK		"\x1b[1;30m"
#define CDBG_RED		"\x1b[1;31m"
#define CDBG_GREEN		"\x1b[1;32m"
#define CDBG_YELLOW		"\x1b[1;33m"
#define CDBG_BLUE		"\x1b[1;34m"
#define CDBG_MAGENTA	"\x1b[1;35m"
#define CDBG_CYAN		"\x1b[1;36m"
#define CDBG_WHITE		"\x1b[1;37m"
#define CDBG_RESET		CDBG_DEFAULT

#define LOG_ENV_NAME		"VS_LOG"

static int vpu_script_log_level = VS_LOG_ERROR;

int vpu_script_log_env(void)
{
	char* env_value = NULL;

	env_value = getenv(LOG_ENV_NAME);
	if(env_value == NULL)
	{
		//none of debug level.
		return -1;
	}

	vpu_script_log_level = atoi(env_value);
	DLOG(VS_LOG_NONE, "LOG", "set vpu script log level : %d", vpu_script_log_level);
	return vpu_script_log_level;
}

void vpu_script_log_print(const char* function, const int line, enum vpu_script_level log_level, const char* part_name, const char* format, ...)
{
	char* end_color = CDBG_DEFAULT;
	va_list arg;

	char* str_loglv = NULL;

	int log_lv = 0;
	char* start_color;
	int level = 0;

	//time
	int hh, mm, ss, ms;
	int tmp_m;
	struct timespec now;

	if(log_level <= vpu_script_log_level)
	{
		switch(log_level)
		{
			case VS_LOG_NONE:
				str_loglv = "NONE";
				start_color = CDBG_BLUE;
			break;

			case VS_LOG_ERROR:
				str_loglv = "ERROR";
				start_color = CDBG_RED;
			break;

			case VS_LOG_WARN:
				str_loglv = "WARN";
				start_color = CDBG_YELLOW;
			break;

			case VS_LOG_INFO:
				str_loglv = "INFO";
				start_color = CDBG_DEFAULT;
			break;

			case VS_LOG_DEBUG:
				str_loglv = "DEBUG";
				start_color = CDBG_GREEN;
			break;

			default:
				return -1;
		}

		printf("%s", start_color);

		if(log_level > VS_LOG_NONE)
		{
			clock_gettime(CLOCK_REALTIME, &now);

			ss = (now.tv_sec % 60);
			tmp_m = now.tv_sec / 60;
			mm = tmp_m % 60;
			hh = (tmp_m / 60) % 24;
			ms = now.tv_nsec / 1000000;

			printf("[%02d:%02d:%02d.%03d][%s][%s][%s:%d]", hh, mm, ss, ms, part_name, str_loglv, function, line);
		}

		va_start(arg, format);
		vprintf(format, arg);
		va_end(arg);

		printf("%s\n", end_color);
	}
}

int vpu_cmd_extract_substring(const char *inputString, const char *inputWord, char *outputSubstring, int *outputLength)
{
	int ret = 0;
	char *wordStart = NULL;
	char *colonPosition = NULL;
	char *endPosition = NULL;
	int length = 0;

	if ((inputString == NULL) || (inputWord ==  NULL) || (outputSubstring == NULL) || (outputLength == NULL))
	{
		DLOGE("null parameter, inputString:%s, inputWord:%s, outputSubstring:%s, outputLength:%p", inputString, inputWord, outputSubstring, outputLength);
		ret = -1;
	}
	else
	{
		wordStart = strstr(inputString, inputWord);
		if (wordStart == NULL)
		{
			DLOGE("Word not found.");
			ret = -1;
		}
		else
		{
			colonPosition = strchr(wordStart, ':');
			if (colonPosition == NULL)
			{
				DLOGE("':' not found.");
				ret = -1;
			}
			else
			{
				endPosition = colonPosition+1;

				while (*endPosition != ',' && *endPosition != ':' /*&& *endPosition != ' '*/ && *endPosition != '\0' && *endPosition != '\n')
				{
					//DLOGI("*endPosition:%c", *endPosition);
					++endPosition;
				}

				//DLOGI("*endPosition:%c, 0x%x", *endPosition, *endPosition);
				length = endPosition - colonPosition - 1;

				strncpy(outputSubstring, colonPosition+1, length);
				outputSubstring[length] = '\0'; // Add the NULL character to signify the end of the string

				*outputLength = strlen(outputSubstring);
			}
		}
	}

	return ret;
}

int vpu_cmd_extract_substring_output_address(const char *inputString, const char *inputWord, char* addr1, char* addr2, char* addr3)
{
	int ret = 0;
	char *wordStart = NULL;
	char *delimiterPosition = NULL;
	char *endPosition = NULL;
	int length = 0;

	if ((inputString == NULL) || (inputWord ==  NULL) || (addr1 == NULL) || (addr2 == NULL) || (addr3 == NULL))
	{
		DLOGE("null parameter, inputString:%s, inputWord:%s, addr1:%p, addr2:%p, addr3:%p", inputString, inputWord, addr1, addr2, addr3);
		ret = -1;
	}
	else
	{
		wordStart = strstr(inputString, inputWord);
		if (wordStart == NULL)
		{
			DLOGE("Word not found.");
			ret = -1;
		}
		else
		{
			delimiterPosition = strchr(wordStart, ':');
			if (delimiterPosition == NULL)
			{
				DLOGE("':' not found.");
				ret = -1;
			}
			else
			{
				endPosition = delimiterPosition+1;

				//addr1
				//DLOGI("start 1:%c", *endPosition);
				while (*endPosition != ',' && *endPosition != ':' && *endPosition != ' ' && *endPosition != '\0')
				{
					//DLOGI("*endPosition:%c", *endPosition);
					++endPosition;
				}

				//DLOGI("*endPosition1:%c", *endPosition);
				length = endPosition - delimiterPosition - 1;

				strncpy(addr1, delimiterPosition+1, length);
				addr1[length] = '\0'; // Add the NULL character to signify the end of the string

				//addr2
				delimiterPosition = endPosition;
				endPosition = delimiterPosition+1;
				//DLOGI("start 2:%c", *endPosition);
				while (*endPosition != ',' && *endPosition != ':' && *endPosition != ' ' && *endPosition != '\0')
				{
					//DLOGI("*endPosition:%c", *endPosition);
					++endPosition;
				}


				length = endPosition - delimiterPosition - 1;

				strncpy(addr2, delimiterPosition+1, length);
				addr2[length] = '\0'; // Add the NULL character to signify the end of the string

				//addr3
				delimiterPosition = endPosition;
				endPosition = delimiterPosition+1;
				//DLOGI("start 3:%c", *endPosition);

				while (*endPosition != ',' && *endPosition != ':' && *endPosition != ' ' && *endPosition != '\0')
				{
					//DLOGI("*endPosition:%c", *endPosition);
					++endPosition;
				}

				//DLOGI("*endPosition3:%c", *endPosition);
				length = endPosition - delimiterPosition - 1;

				strncpy(addr3, delimiterPosition+1, length);
				addr3[length] = '\0'; // Add the NULL character to signify the end of the string
			}
		}
	}

	return ret;
}
