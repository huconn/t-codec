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
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "vpu_script_common.h"
#include "vpu_script_system.h"
#include "video_decoder.h"


#define SCAN_MAX_SIZE 1024 /** THe sequnce header is positioned in 1024 bytes */

static const char* part_name = "SYSTEM";
#define DLOGD(...) 		DLOG(VS_LOG_DEBUG, part_name, __VA_ARGS__)	//debug
#define DLOGI(...) 		DLOG(VS_LOG_INFO, part_name, __VA_ARGS__)	//info
#define DLOGE(...) 		DLOG(VS_LOG_ERROR, part_name, __VA_ARGS__)	//error

int parse_video_frame(void* file_handle, unsigned char* read_buffer, unsigned int buffer_size, enum VPU_SYSTEM_SUPPORT_CODEC codec_id, int* search_seqheader)
{
	int iReadByte = 0;
	int iDataSize = 0;
	VideoDecoder_Codec_E codec = (VideoDecoder_Codec_E)codec_id;
	unsigned char* readPtr = read_buffer;
	FILE* hInputFile = (FILE*)file_handle;

	do
	{
		if(iDataSize < buffer_size)
		{
			if ((iReadByte = fread(readPtr + iDataSize, 1, 1, hInputFile)) <= 0)
			{
				if (iReadByte == 0)
				{
					DLOGE("End of file!");
					iDataSize = -1;
					break;
				}
				else
				{
					DLOGE("read error!");
					iDataSize = -1;
					break;
				}
			}
		}
		else
		{
			DLOGE("buffer is not insufficient, size:%d, read:%d, iReadByte:%d", buffer_size, iDataSize, iReadByte);
		}

		if(iReadByte > 0)
		{
			iDataSize += iReadByte;

			if (iDataSize > 8)
			{
				if (codec == VPU_SYSTEM_CODEC_H263)
				{
					if (readPtr[iDataSize - 3] == 0x00 && readPtr[iDataSize - 2] == 0x00 && (readPtr[iDataSize - 1] >> 4) == 0x08) // H263 start code (22 bit)
					{
						if (*search_seqheader == 1)
						{
							fseek(hInputFile, -3, SEEK_CUR);

							iDataSize -= 3;

							break;
						}
						else
						{
							*search_seqheader = 1;
						}
					}
				}
				else  if (codec == VPU_SYSTEM_CODEC_H264)
				{
					if(readPtr[iDataSize - 5] == 0x00 &&  readPtr[iDataSize - 4] == 0x00 && readPtr[iDataSize - 3] == 0x00
						&& readPtr[iDataSize - 2] == 0x01 /* && readPtr[iDataSize - 1] == 0x09 */) // AUD start code
					{
						if (*search_seqheader == 1)
						{
							fseek(hInputFile, -5, SEEK_CUR);

							iDataSize -= 5;

							break;
						}
						else
						{
							*search_seqheader = 1;
						}
					}
				}
				else  if (codec == VPU_SYSTEM_CODEC_H265)
				{
					if(readPtr[iDataSize - 5] == 0x00 &&  readPtr[iDataSize - 4] == 0x00 && readPtr[iDataSize - 3] == 0x00
						&& readPtr[iDataSize - 2] == 0x01 /* && readPtr[iDataSize - 1] == 0x46 */ )
					{
						if (*search_seqheader == 1)
						{
							fseek(hInputFile, -5, SEEK_CUR);

							iDataSize -= 5;

							break;
						}
						else
						{
							*search_seqheader = 1;
						}
					}
				}
				else  if (codec == VPU_SYSTEM_CODEC_MPEG4)
				{
				if(readPtr[iDataSize - 4] == 0x00 && readPtr[iDataSize - 3] == 0x00
					&& readPtr[iDataSize - 2] == 0x01 && readPtr[iDataSize - 1] == 0xB6) // MPEG-4 start code
					{
						if (*search_seqheader == 1)
						{
							fseek(hInputFile, -4, SEEK_CUR);

							iDataSize -= 4;

							break;
						}
						else
						{
							*search_seqheader = 1;
						}
					}
				}
				else  if (codec == VPU_SYSTEM_CODEC_MJPEG)
				{
					if (readPtr[iDataSize - 3] == 0xFF && readPtr[iDataSize - 2] == 0xD8 && readPtr[iDataSize - 1] == 0xFF) // jpg start code
					{
						//DLOGI("find jpeg start code %x %x %x, size:%d", readPtr[iDataSize - 3], readPtr[iDataSize - 2], readPtr[iDataSize - 1], iDataSize-3);
						fseek(hInputFile, -3, SEEK_CUR);
						iDataSize -= 3;
						break;
					}
				}
				else
				{
					DLOGE("Not supported yet, codec:%d", codec);
					break;
				}
			} // if (iDataSize > 8)
		} // if(iReadByte > 0)
	} while (1);

	return iDataSize;
}

int scan_sequence_header(unsigned char* read_buffer, unsigned int buffer_size, enum VPU_SYSTEM_SUPPORT_CODEC codec_id)
{
    unsigned char *p_buff = read_buffer;
    unsigned char *p_buff_start = p_buff;
    unsigned char *p_buff_end = p_buff + (buffer_size > SCAN_MAX_SIZE ? SCAN_MAX_SIZE : buffer_size) - 4;
    unsigned char *p_buff_data_end = p_buff + buffer_size - 4;
    unsigned int syncword = 0xFFFFFFFF;
    unsigned int nal_type = 0;
	VideoDecoder_Codec_E codec = (VideoDecoder_Codec_E)codec_id;

    syncword = ((unsigned int)p_buff[0] << 16) | ((unsigned int)p_buff[1] << 8) | (unsigned int)p_buff[2];

    if(codec == VPU_SYSTEM_CODEC_H264)
    {
        while(p_buff < p_buff_end)
        {
            syncword <<= 8;
            syncword |= p_buff[3];

            if((syncword >> 8) == 0x000001)
            {
                p_buff_end = p_buff_data_end;

                if((syncword & 0x1F) == 7) // nal_type: SPS
                    return (int)(p_buff - p_buff_start);
            }
            p_buff++;
        }
    }
    else if(codec == VPU_SYSTEM_CODEC_H265)
    {
        while(p_buff < p_buff_end)
        {
            syncword <<= 8;
            syncword |= p_buff[3];

            if((syncword >> 8) == 0x000001)
            {
                p_buff_end = p_buff_data_end;

                nal_type = (syncword >> 1) & 0x3F;

                if(nal_type == 32 || nal_type == 33) // nal_type: VPS/SPS
                    return (int)(p_buff - p_buff_start);
            }
            p_buff++;
        }
    }
    else if(codec == VPU_SYSTEM_CODEC_MPEG4)
    {
        while(p_buff < p_buff_end)
        {
            syncword <<= 8;
            syncword |= p_buff[3];

            if((syncword >> 8) == 0x000001 )
            {
                p_buff_end = p_buff_data_end;

                if(syncword >= 0x00000120 && syncword <= 0x0000012F) // video_object_layer_start_code
                    return (int)(p_buff - p_buff_start);
            }


            p_buff++;
        }
    }
    else
    {
        unsigned int seqhead_sig = 0;

        switch(codec)
        {
        //case VideoDecoder_Codec_H264:
        //    seqhead_sig = 0x0000010F; //VC-1 sequence start code
        //    break;
        case VPU_SYSTEM_CODEC_MPEG2:
            seqhead_sig = 0x000001B3; //MPEG video sequence start code
            break;
        case VPU_SYSTEM_CODEC_VP9:
            return 0;
        default:
            DLOGE("Stream type is not supported (type: %ld)", codec);
            return 0;
        }

        while(p_buff < p_buff_end)
        {
            syncword <<= 8;
            syncword |= p_buff[3];

            if(syncword == seqhead_sig)
                return (int)(p_buff - p_buff_start);

            p_buff++;
        }
    }

    return -1;
}

char* readUntilNewline(char* input_buffer, char** output_buffer, unsigned int* output_size)
{
	int length = 0;
	int index = 0;
	char* next_buffer = NULL;
	char* output = NULL;

	if(input_buffer != NULL)
	{
		//calculate length
		while (input_buffer[length] != '\0' && input_buffer[length] != '\n')
		{
			length++;
		}

		output = (char*)malloc((length + 1) * sizeof(char));
		if (output == NULL)
		{
			DLOGE("allocation failed\n");
		}
		else
		{
			while (input_buffer[index] != '\0' && input_buffer[index] != '\n')
			{
				output[index] = input_buffer[index];
				index++;
			}

			output[index] = '\0';

			*output_buffer = output;
			if(output_size != NULL)
			{
				*output_size = length;
			}

			if(input_buffer[index] == '\0')
			{
				next_buffer = NULL;
			}
			else
			{
				next_buffer = &input_buffer[index+1];
			}
		}
	}

	return next_buffer;
}

char* readLineFromBuffer(char* input_buffer, char* outputBuffer, int buffer_size)
{
	int length = 0;
	int input_index = 0;
	int cmd_index = 0;
	char* next_buffer = NULL;
	char* output = NULL;

	if(input_buffer != NULL)
	{
		if(input_buffer[input_index] == '#')
		{
			while (input_buffer[input_index] != '\0' && input_buffer[input_index] != '\n')
			{
				input_index++;
			}
		}
		else
		{
			while (input_buffer[input_index] != '\0' && input_buffer[input_index] != '\n')
			{
				if(cmd_index < buffer_size)
				{
					if(input_buffer[input_index] != 0x09)
					{
						//printf("char:0x%x %c\n", input_buffer[input_index], input_buffer[input_index]);
						outputBuffer[cmd_index] = input_buffer[input_index];
						cmd_index++;
					}
				}

				input_index++;
			}

			outputBuffer[cmd_index] = '\0';
			//DLOGI("output:%s", outputBuffer);
		}

		if(input_buffer[input_index] == '\0')
		{
			next_buffer = NULL;
		}
		else
		{
			next_buffer = &input_buffer[input_index+1];
		}
	}

	return next_buffer;
}

char* readCommandUntilNewline(char* input_buffer, char* command, int cmd_size, char* parameter, int param_size)
{
	int length = 0;
	int input_index = 0;
	int cmd_index = 0;
	int param_index = 0;
	int found_space = 0;
	char* next_buffer = NULL;
	char* output = NULL;

	if(input_buffer != NULL)
	{
		if(input_buffer[input_index] == '#')
		{
			while (input_buffer[input_index] != '\0' && input_buffer[input_index] != '\n')
			{
				input_index++;
			}
		}
		else
		{
			while (input_buffer[input_index] != '\0' && input_buffer[input_index] != '\n')
			{
				if(input_buffer[input_index] == ' ')
				{
					found_space = 1;
				}

				if(found_space == 0) //command
				{
					if(cmd_index < cmd_size)
					{
						if(input_buffer[input_index] != 0x09)
						{
							//printf("char:0x%x %c\n", input_buffer[input_index], input_buffer[input_index]);
							command[cmd_index] = input_buffer[input_index];
							cmd_index++;
						}
					}
				}
				else
				{
					//printf("char:0x%x %c\n", input_buffer[input_index], input_buffer[input_index]);
					if((param_index == 0 && input_buffer[input_index] != 0x20) || (param_index > 0)) //remove first space
					{
						//if(input_buffer[input_index] != 0x20)
						{
							if(param_index < param_size)
							{
								parameter[param_index] = input_buffer[input_index];
							}
						}

						param_index++;
					}
				}

				input_index++;
			}

			command[cmd_index] = '\0';
			parameter[param_index] = '\0';
		}

		if(input_buffer[input_index] == '\0')
		{
			next_buffer = NULL;
		}
		else
		{
			next_buffer = &input_buffer[input_index+1];
		}
	}

	return next_buffer;
}

long get_time_us(void)
{
	struct timespec check_time;
	long now = 0;

	clock_gettime(CLOCK_MONOTONIC, &check_time);

	now = check_time.tv_sec * 1000000L + check_time.tv_nsec / 1000L;

	return now;
}
