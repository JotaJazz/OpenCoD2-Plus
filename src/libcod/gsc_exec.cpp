#include "gsc_exec.hpp"

#if LIBCOD_COMPILE_EXEC == 1

enum
{
	INT_VALUE,
	FLOAT_VALUE,
	STRING_VALUE,
	VECTOR_VALUE,
	OBJECT_VALUE
};

struct exec_outputline
{
	char content[COD2_MAX_STRINGLENGTH];
	exec_outputline *next;
};

struct exec_async_task
{
	exec_async_task *prev;
	exec_async_task *next;
	char command[COD2_MAX_STRINGLENGTH];
	int callback;
	bool done;
	bool save;
	bool error;
	exec_outputline *output;
	unsigned int levelId;
	bool hasargument;
	int valueType;
	int intValue;
	float floatValue;
	char stringValue[COD2_MAX_STRINGLENGTH];
	vec3_t vectorValue;
	unsigned int objectValue;
};

exec_async_task *first_exec_async_task = NULL;

// The task holds a reference to an object argument until it is delivered
// or dropped, so the script can't free it in between
static void exec_async_release_argument(exec_async_task *task)
{
	if (task->hasargument && task->valueType == OBJECT_VALUE && task->objectValue)
	{
		RemoveRefToObject(task->objectValue);
		task->objectValue = 0;
	}
}

static void exec_async_unlink_task(exec_async_task *task)
{
	if (task->next != NULL)
		task->next->prev = task->prev;

	if (task->prev != NULL)
		task->prev->next = task->next;
	else
		first_exec_async_task = task->next;

	task->next = task->prev = NULL;
}

static void exec_async_delete_task(exec_async_task *task)
{
	exec_outputline *output = task->output;

	while (output != NULL)
	{
		exec_outputline *next = output->next;
		delete output;
		output = next;
	}

	exec_async_release_argument(task);
	delete task;
}

static void exec_async_free_task(exec_async_task *task)
{
	exec_async_unlink_task(task);
	exec_async_delete_task(task);
}

void gsc_exec()
{
	const char *command;

	if (!stackGetParamString(0, &command))
	{
		stackError("gsc_exec() argument is undefined or has wrong type");
		stackPushUndefined();
		return;
	}

	Com_DPrintf("gsc_exec() executing: %s\n", command);

	FILE *fp;

	fp = popen(command, "r");

	if (fp == NULL)
	{
		stackPushUndefined();
		return;
	}

	int c;
	int curpos = 0;
	char content[COD2_MAX_STRINGLENGTH];

	stackPushArray();

	while ((c = getc(fp)) != EOF)
	{
		if (c == '\n' || curpos == COD2_MAX_STRINGLENGTH - 1)
		{
			content[curpos] = '\0';
			stackPushString(content);
			stackPushArrayLast();
			curpos = 0;
		}
		else
		{
			content[curpos] = c;
			curpos++;
		}
	}

	content[curpos] = '\0';

	stackPushString(content);
	stackPushArrayLast();

	pclose(fp);
}

void *exec_async(void *input_c)
{
	exec_async_task *task = (exec_async_task*)input_c;
	FILE *fp;

	fp = popen(task->command, "r");

	if (fp == NULL)
	{
		task->error = true;
		task->done = true;
		return NULL;
	}

	if (task->save)
	{
		exec_outputline *output = new exec_outputline;
		task->output = output;
		output->next = NULL;

		int c;
		int curpos = 0;

		while ((c = getc(fp)) != EOF)
		{
			if (c == '\n' || curpos == COD2_MAX_STRINGLENGTH - 1)
			{
				output->content[curpos] = '\0';
				output->next = new exec_outputline;
				output = output->next;
				output->next = NULL;
				curpos = 0;
			}
			else
			{
				output->content[curpos] = c;
				curpos++;
			}
		}

		output->content[curpos] = '\0';
	}
	else
		while(getc(fp) != EOF); //make thread wait for function to finish

	pclose(fp);
	task->done = true;
	return NULL;
}

void gsc_exec_async_create()
{
	const char *command;
	int callback;

	if (!stackGetParamString(0, &command))
	{
		stackError("gsc_exec_async_create() argument is undefined or has wrong type");
		stackPushUndefined();
		return;
	}

	Com_DPrintf("gsc_exec_async_create() executing: %s\n", command);

	exec_async_task *current = first_exec_async_task;

	while (current != NULL && current->next != NULL)
		current = current->next;

	exec_async_task *newtask = new exec_async_task;

	strncpy(newtask->command, command, COD2_MAX_STRINGLENGTH - 1);
	newtask->command[COD2_MAX_STRINGLENGTH - 1] = '\0';
	newtask->output = NULL;
	newtask->prev = current;
	newtask->next = NULL;

	if (!stackGetParamFunction(1, &callback))
		newtask->callback = 0;
	else
		newtask->callback = callback;

	newtask->done = false;
	newtask->save = true;
	newtask->error = false;
	newtask->levelId = libcodLevelGeneration;
	newtask->hasargument = true;

	int valueInt;
	float valueFloat;
	const char *valueString;
	vec3_t valueVector;
	unsigned int valueObject;

	if (stackGetParamInt(2, &valueInt))
	{
		newtask->valueType = INT_VALUE;
		newtask->intValue = valueInt;
	}
	else if (stackGetParamFloat(2, &valueFloat))
	{
		newtask->valueType = FLOAT_VALUE;
		newtask->floatValue = valueFloat;
	}
	else if (stackGetParamString(2, &valueString))
	{
		newtask->valueType = STRING_VALUE;
		I_strncpyz(newtask->stringValue, valueString, sizeof(newtask->stringValue));
	}
	else if (stackGetParamVector(2, valueVector))
	{
		newtask->valueType = VECTOR_VALUE;
		newtask->vectorValue[0] = valueVector[0];
		newtask->vectorValue[1] = valueVector[1];
		newtask->vectorValue[2] = valueVector[2];
	}
	else if (stackGetParamObject(2, &valueObject))
	{
		newtask->valueType = OBJECT_VALUE;
		newtask->objectValue = valueObject;
		AddRefToObject(valueObject);
	}
	else
		newtask->hasargument = false;

	if (current != NULL)
		current->next = newtask;
	else
		first_exec_async_task = newtask;

	threadid_t exec_doer;

	if (!Sys_CreateNewThread(exec_async, &exec_doer, newtask))
	{
		exec_async_free_task(newtask);
		stackError("gsc_exec_async_create() error creating exec async handler thread!");
		stackPushUndefined();
		return;
	}

	stackPushInt(1);
}

void gsc_exec_async_create_nosave()
{
	const char *command;
	int callback;

	if (!stackGetParamString(0, &command))
	{
		stackError("gsc_exec_async_create_nosave() argument is undefined or has wrong type");
		stackPushUndefined();
		return;
	}

	Com_DPrintf("gsc_exec_async_create_nosave() executing: %s\n", command);

	exec_async_task *current = first_exec_async_task;

	while (current != NULL && current->next != NULL)
		current = current->next;

	exec_async_task *newtask = new exec_async_task;

	strncpy(newtask->command, command, COD2_MAX_STRINGLENGTH - 1);
	newtask->command[COD2_MAX_STRINGLENGTH - 1] = '\0';
	newtask->output = NULL;
	newtask->prev = current;
	newtask->next = NULL;

	if (!stackGetParamFunction(1, &callback))
		newtask->callback = 0;
	else
		newtask->callback = callback;

	newtask->done = false;
	newtask->save = false;
	newtask->error = false;
	newtask->levelId = libcodLevelGeneration;
	newtask->hasargument = true;

	int valueInt;
	float valueFloat;
	const char *valueString;
	vec3_t valueVector;
	unsigned int valueObject;

	if (stackGetParamInt(2, &valueInt))
	{
		newtask->valueType = INT_VALUE;
		newtask->intValue = valueInt;
	}
	else if (stackGetParamFloat(2, &valueFloat))
	{
		newtask->valueType = FLOAT_VALUE;
		newtask->floatValue = valueFloat;
	}
	else if (stackGetParamString(2, &valueString))
	{
		newtask->valueType = STRING_VALUE;
		I_strncpyz(newtask->stringValue, valueString, sizeof(newtask->stringValue));
	}
	else if (stackGetParamVector(2, valueVector))
	{
		newtask->valueType = VECTOR_VALUE;
		newtask->vectorValue[0] = valueVector[0];
		newtask->vectorValue[1] = valueVector[1];
		newtask->vectorValue[2] = valueVector[2];
	}
	else if (stackGetParamObject(2, &valueObject))
	{
		newtask->valueType = OBJECT_VALUE;
		newtask->objectValue = valueObject;
		AddRefToObject(valueObject);
	}
	else
		newtask->hasargument = false;

	if (current != NULL)
		current->next = newtask;
	else
		first_exec_async_task = newtask;

	threadid_t exec_doer;

	if (!Sys_CreateNewThread(exec_async, &exec_doer, newtask))
	{
		exec_async_free_task(newtask);
		stackError("gsc_exec_async_create_nosave() error creating exec async handler thread!");
		stackPushUndefined();
		return;
	}

	stackPushInt(1);
}

void gsc_exec_async_checkdone()
{
	for (;;)
	{
		exec_async_task *task = first_exec_async_task;

		while (task != NULL && !task->done)
			task = task->next;

		if (task == NULL)
			break;

		// out of the list before its callback, which may run checkdone again
		exec_async_unlink_task(task);

		{
			//push to cod
			if (Scr_IsSystemActive() && task->save && task->callback && !task->error && (libcodLevelGeneration == task->levelId))
			{
				if (task->hasargument)
				{
					switch(task->valueType)
					{
					case INT_VALUE:
						stackPushInt(task->intValue);
						break;

					case FLOAT_VALUE:
						stackPushFloat(task->floatValue);
						break;

					case STRING_VALUE:
						stackPushString(task->stringValue);
						break;

					case VECTOR_VALUE:
						stackPushVector(task->vectorValue);
						break;

					case OBJECT_VALUE:
						if (task->objectValue)
							stackPushObject(task->objectValue);
						else
							stackPushUndefined();
						break;

					default:
						stackPushUndefined();
						break;
					}
				}

				stackPushArray();

				for (exec_outputline *output = task->output; output != NULL; output = output->next)
				{
					stackPushString(output->content);
					stackPushArrayLast();
				}

				short ret = Scr_ExecThread(task->callback, task->save + task->hasargument);
				Scr_FreeThread(ret);
			}

			exec_async_delete_task(task);
		}
	}
}

void gsc_exec_async_shutdown_level()
{
	for (exec_async_task *task = first_exec_async_task; task != NULL; task = task->next)
		exec_async_release_argument(task);
}

#endif
