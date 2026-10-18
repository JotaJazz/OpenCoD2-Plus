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
#include "linux_local.h"
#include <errno.h>
#include <pthread.h>
#include <signal.h>

threadid_t mainthread;
void *g_threadValues[THREAD_VALUE_COUNT];

/*
================
Sys_InitializeCriticalSections
================
*/
static pthread_mutex_t crit_sections[CRITSECT_COUNT];
static void Sys_InitializeCriticalSections( void )
{
	int i;
	pthread_mutexattr_t muxattr;

	pthread_mutexattr_init(&muxattr);
	pthread_mutexattr_settype(&muxattr, PTHREAD_MUTEX_RECURSIVE);

	for (i = 0; i < CRITSECT_COUNT; i++)
	{
		pthread_mutex_init( &crit_sections[i], &muxattr );

	}

	pthread_mutexattr_destroy(&muxattr);
}

/*
================
Sys_InitMainThread
================
*/
void Sys_InitMainThread()
{
	Sys_InitializeCriticalSections();
	mainthread = pthread_self();
	Com_InitThreadData(THREAD_CONTEXT_MAIN);
}

/*
================
Sys_IsMainThread
================
*/
qboolean Sys_IsMainThread( void )
{
	return mainthread == pthread_self();
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

#define THREAD_ALTSTACK_SIZE ( 64 * 1024 )

typedef struct
{
	void *(*func)(void *);
	void *arg;
} threadStart_t;

static pthread_key_t sys_altStackKey;
static pthread_once_t sys_altStackOnce = PTHREAD_ONCE_INIT;

static void Sys_FreeAltStack( void *stack )
{
	stack_t ss;

	memset( &ss, 0, sizeof( ss ) );
	ss.ss_flags = SS_DISABLE;
	sigaltstack( &ss, NULL );
	free( stack );
}

static void Sys_CreateAltStackKey( void )
{
	pthread_key_create( &sys_altStackKey, Sys_FreeAltStack );
}

static void *Sys_ThreadStart( void *data )
{
	threadStart_t start = *(threadStart_t *)data;
	stack_t ss;

	free( data );

	ss.ss_sp = malloc( THREAD_ALTSTACK_SIZE );
	ss.ss_size = THREAD_ALTSTACK_SIZE;
	ss.ss_flags = 0;

	// the key's destructor frees it when the thread ends
	if ( ss.ss_sp && sigaltstack( &ss, NULL ) == 0 )
		pthread_setspecific( sys_altStackKey, ss.ss_sp );
	else
		free( ss.ss_sp );

	return start.func( start.arg );
}

/*
================
Sys_CreatePthread

pthread_create for worker threads. The alternate signal stack set up in
Sys_InstallCrashHandler is the main thread's only, so each worker gets its
own, or a stack overflow there would leave no crash report.
================
*/
int Sys_CreatePthread( pthread_t *thread, const pthread_attr_t *attr, void *(*func)(void *), void *arg )
{
	threadStart_t *start;
	int err;

	pthread_once( &sys_altStackOnce, Sys_CreateAltStackKey );

	start = (threadStart_t *)malloc( sizeof( *start ) );
	if ( !start )
		return ENOMEM;

	start->func = func;
	start->arg = arg;

	err = pthread_create( thread, attr, Sys_ThreadStart, start );
	if ( err != 0 )
		free( start );

	return err;
}

/*
================
Sys_CreateNewThread
================
*/
#define MIN_STACKSIZE 256*1024
qboolean Sys_CreateNewThread(void* (*ThreadMain)(void*), threadid_t *tid, void* arg)
{
	int err;
	pthread_attr_t tattr;

	err = pthread_attr_init(&tattr);
	if(err != 0)
	{
		Com_Printf("pthread_attr_init(): Thread creation failed with the following error: %s\n", strerror(err));
		return qfalse;
	}

	err = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
	if(err != 0)
	{
		pthread_attr_destroy(&tattr);
		Com_Printf("pthread_attr_setdetachstate(): Thread creation failed with the following error: %s\n", strerror(err));
		return qfalse;
	}

	err = pthread_attr_setstacksize(&tattr, MIN_STACKSIZE);
	if(err != 0)
	{
		pthread_attr_destroy(&tattr);
		Com_Printf("pthread_attr_setstacksize(): Thread creation failed with the following error: %s\n", strerror(err));
		return qfalse;
	}

	err = Sys_CreatePthread(tid, &tattr, ThreadMain, arg);
	if(err != 0)
	{
		pthread_attr_destroy(&tattr);
		Com_Printf("pthread_create(): Thread creation failed with the following error: %s\n", strerror(err));
		return qfalse;
	}
	pthread_attr_destroy(&tattr);
	return qtrue;
}

/*
================
Sys_ExitThread
================
*/
void Sys_ExitThread(int code)
{
	pthread_exit(&code);
}

/*
================
Sys_SleepMSec
================
*/
void Sys_SleepMSec(int msec)
{
	struct timespec ts;
	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

/*
================
Sys_EnterCriticalSection
================
*/
void Sys_EnterCriticalSection(int section)
{
	pthread_mutex_lock(&crit_sections[section]);
}

/*
================
Sys_TryEnterCriticalSection
================
*/
int Sys_TryEnterCriticalSection(int section)
{
	return pthread_mutex_trylock(&crit_sections[section]);
}

/*
================
Sys_LeaveCriticalSection
================
*/
void Sys_LeaveCriticalSection(int section)
{
	pthread_mutex_unlock(&crit_sections[section]);
}
