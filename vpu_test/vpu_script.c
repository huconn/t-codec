#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "vpu_script.h"
#include "vpu_script_common.h"
#include "vpu_script_system.h"
#include "vpu_script_process.h"

static const char* part_name = "MAIN";
#define DLOGD(...) 		DLOG(VS_LOG_DEBUG, part_name, __VA_ARGS__)	//debug
#define DLOGI(...) 		DLOG(VS_LOG_INFO, part_name, __VA_ARGS__)	//info
#define DLOGE(...) 		DLOG(VS_LOG_ERROR, part_name, __VA_ARGS__)	//error

#define CMD_BUF_SIZE 			(1024)

#define BUFFER_ALLOC_SIZE	(512)
#define MAX_TASK	(32)

typedef struct VPUScriptTask_t {
	int id;
	pthread_t handle;
	unsigned int is_running;

	unsigned char* buffer;
	unsigned int buffer_alloc_size;
	unsigned int buffer_filled_size;

	VPUCmdProc_h h_proc;

	VPUScriptResult_t result;
} VPUScriptTask_t;

//global task manager
typedef struct VPUScriptMain_t {
	VPUScriptTask_t* task[MAX_TASK];
	int curr_index;
} VPUScriptMain_t;

static void* thread_func(void* private)
{
	int ret = 0;
	VPUScriptTask_t* task = (VPUScriptTask_t*)private;

	DLOGI("thread started, ID:%d", task->id);
	//DLOGI("buffer:\n%s", task->buffer);

	task->h_proc = vpu_proc_alloc(task->id);

	while(task->is_running == 1)
	{
		ret = vpu_proc_parser(task->h_proc, task->buffer, task->buffer_filled_size, &task->result);
		if(ret == -1)
		{
			DLOGI("script executing finished");
			task->is_running = 0;
			break;
		}
	}

	vpu_proc_release(task->h_proc);
	task->h_proc = NULL;

	DLOGI("thread stopped, ID:%d", task->id);
	return NULL;
}

int wait_user_exit(VPUScriptMain_t* main)
{
	int ii;
	VPUScriptTask_t* task = NULL;

	//waiting user comand
	DLOGI("====== wait user input ======");
	while (1)
	{
		 char userInput = getchar();
		 if(userInput == 'q')
		 {
			//send thread stop
			DLOGI("Send a stop signal to all tasks");
			for(ii = 0; ii < MAX_TASK; ii++)
			{
				task = main->task[ii];
				if(task != NULL)
				{
					task->is_running = 0;
				}
			}

			DLOGI("exit script");
			break;
		 }
    }
}

int vpu_script_main(const char* script_buffer)
{
	int ii;
	int ret = 0;
	char* linebuffer = NULL;
	char* nextbuffer = NULL;
	unsigned int linelength = 0;
	char command[CMD_BUF_SIZE];
	int cmd_size = CMD_BUF_SIZE;
	int repeat = 0;
	int inside_new_block = 0;
	VPUScriptMain_t main_task;
	VPUScriptMain_t* main = &main_task;
	VPUScriptTask_t* task = NULL;

	vpu_script_log_env();

	main->curr_index = 0;

	memset(main, 0x00, sizeof(VPUScriptMain_t));
	memset(command, 0x00, CMD_BUF_SIZE);

	nextbuffer = (char*)script_buffer;

	//parse script and execute
	while(1)
	{
		nextbuffer = readLineFromBuffer(nextbuffer, command, cmd_size);
		if((nextbuffer != NULL) || (strlen(command) > 0))
		{
			int cmd_length = strlen(command);
			if (cmd_length> 0)
			{
				int cmd_ret = 0;
				//DLOGI("command:%s", command);

				//check decoder_start or encoder_start with id
				if(strncmp(command, "task_start", strlen("task_start")) == 0 )
				{
					inside_new_block = 1;

					main->task[main->curr_index] = malloc(sizeof(VPUScriptTask_t));
					if (main->task[main->curr_index] != NULL)
					{
						DLOGI("New Task, index:%d", main->curr_index);
						task = main->task[main->curr_index];

						//assign buffer to thread
						task->id = main->curr_index;
						task->buffer = malloc(BUFFER_ALLOC_SIZE);
						task->buffer_alloc_size = BUFFER_ALLOC_SIZE;
						task->buffer_filled_size = 0;
						task->is_running = 0;
					}
				}

				//if decoder_start or encoder_start is detected, allocate new buffer
				//   copy text to new buffer until meet decoder_end or encoder_end
				if(inside_new_block == 1)
				{
					if(strncmp(command, "task_end", strlen("task_end")) == 0)
					{
						inside_new_block = 0;

						//alloc new thread
						//DLOGI("script buffer:%s", task->buffer);

						task->is_running = 1;
						pthread_create(&task->handle, NULL, thread_func, task);
						main->curr_index++;
						if(main->curr_index >= MAX_TASK)
						{
							DLOGE("too many task are created");
						}
					}
					else
					{
						//alloc new buffer
						//copy text
						if(task->buffer_alloc_size < (task->buffer_filled_size + cmd_length))
						{
							//DLOGI("realloc, alloc_size:%d, filled_size:%d, cmd_length:%d", task->buffer_alloc_size, task->buffer_filled_size, cmd_length);
							task->buffer = realloc(task->buffer, task->buffer_alloc_size + BUFFER_ALLOC_SIZE);
							if (task->buffer != NULL)
							{
								task->buffer_alloc_size += BUFFER_ALLOC_SIZE + 1;
							}
							else
							{
								DLOGE("allocation failed");
								ret = -1;
								break;
							}
						}

						if(ret == 0)
						{
							strcpy(task->buffer + task->buffer_filled_size, command);
							task->buffer_filled_size += cmd_length;
							task->buffer[task->buffer_filled_size] = '\n';
							task->buffer_filled_size += 1;

							//DLOGI("buffer:%s", task->buffer);
						}
					}
				}
			}

			memset(command, 0x00, CMD_BUF_SIZE);
		}
		else
		{
			//end of script
			break;
		}
	}

	//wait_user_exit(main);

	//join
	{
		int ii;
		DLOGI("Wait for all the tasks to stop");
		for(ii = 0; ii < MAX_TASK; ii++)
		{
			task = main->task[ii];
			if(task != NULL)
			{
				pthread_join(task->handle, NULL);
			}
		}
	}

	//release
	for(ii = 0; ii < MAX_TASK; ii++)
	{
		task = main->task[ii];
		if(task != NULL)
		{
			if(task->buffer != NULL)
			{
				free(task->buffer);
				task->buffer = NULL;
			}

			free(task);
			task = NULL;

			main->task[ii] = NULL;
		}
	}

	return ret;
}
