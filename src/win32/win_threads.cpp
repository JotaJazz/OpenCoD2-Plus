/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../qcommon/qcommon.h"
#include "win_local.h"
#include <windows.h>

threadid_t mainthread;
void *g_threadValues[THREAD_VALUE_COUNT];

/*
================
Sys_InitializeCriticalSections
================
*/
static CRITICAL_SECTION crit_sections[CRITSECT_COUNT];
static void Sys_InitializeCriticalSections( void )
{
	int i;

	for (i = 0; i < CRITSECT_COUNT; i++)
	{
		InitializeCriticalSection( &crit_sections[i] );
	}
}

/*
================
Sys_InitMainThread
================
*/
void Sys_InitMainThread()
{
	Sys_InitializeCriticalSections();
	mainthread = GetCurrentThreadId();
	Com_InitThreadData(THREAD_CONTEXT_MAIN);
}

/*
================
Sys_IsMainThread
================
*/
qboolean Sys_IsMainThread( void )
{
	return mainthread == GetCurrentThreadId();
}

/*
================
Sys_SetValue
================
*/
void Sys_SetValue( int valueIndex, void *data )
{
	g_threadValues[valueIndex] = data;
}

/*
================
Sys_GetValue
================
*/
void* Sys_GetValue( int valueIndex )
{
	return g_threadValues[valueIndex];
}

// CreateThread calls a WINAPI (stdcall) function, which pops its argument;
// the thread functions are cdecl, so they are called through this
struct sysThreadStart_t
{
	void* (*func)(void*);
	void* arg;
};

static DWORD WINAPI Sys_ThreadStart(LPVOID param)
{
	sysThreadStart_t start = *(sysThreadStart_t *)param;

	free(param);
	start.func(start.arg);
	return 0;
}

HANDLE Sys_CreateThreadWithHandle(void* (*ThreadMain)(void*), threadid_t *tid, void* arg)
{
	char errMessageBuf[512];
	DWORD lastError;
	sysThreadStart_t *start = (sysThreadStart_t *)malloc(sizeof(*start));

	if(start == NULL)
	{
		Com_Printf("Failed to start thread!\n");
		return NULL;
	}

	start->func = ThreadMain;
	start->arg = arg;

	HANDLE thid = CreateThread(	NULL, // LPSECURITY_ATTRIBUTES lpsa,
	                            0, // DWORD cbStack,
	                            Sys_ThreadStart, // LPTHREAD_START_ROUTINE lpStartAddr,
	                            start, // LPVOID lpvThreadParm,
	                            0, // DWORD fdwCreate,
	                            tid );

	if(thid == NULL)
	{
		free(start);
		lastError = GetLastError();

		if(lastError != 0)
		{
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lastError, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR)errMessageBuf, sizeof(errMessageBuf) -1, NULL);
			Com_Printf("Failed to start thread with error: %s\n", errMessageBuf);

		}
		else
		{
			Com_Printf("Failed to start thread!\n");
		}
		return NULL;
	}
	return thid;
}

/*
================
Sys_CreateNewThread
================
*/
qboolean Sys_CreateNewThread(void* (*ThreadMain)(void*), threadid_t *tid, void* arg)
{
	HANDLE thid = Sys_CreateThreadWithHandle(ThreadMain, tid, arg);

	if(thid == NULL)
	{
		return qfalse;
	}

	CloseHandle(thid);
	return qtrue;
}

/*
================
Sys_ExitThread
================
*/
void Sys_ExitThread(int code)
{
	ExitThread( code );
}

/*
================
Sys_SleepMSec
================
*/
void Sys_SleepMSec(int msec)
{
	Sleep(msec);
}

/*
================
Sys_EnterCriticalSection
================
*/
void Sys_EnterCriticalSection(int section)
{
	EnterCriticalSection(&crit_sections[section]);
}

/*
================
Sys_TryEnterCriticalSection
================
*/
int Sys_TryEnterCriticalSection(int section)
{
	return TryEnterCriticalSection(&crit_sections[section]) == 0;
}

/*
================
Sys_LeaveCriticalSection
================
*/
void Sys_LeaveCriticalSection(int section)
{
	LeaveCriticalSection(&crit_sections[section]);
}
