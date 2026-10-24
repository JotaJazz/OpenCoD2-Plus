/*
 * httpFetch and webSocket*, with the script API of zk_libcod
 * (https://github.com/ibuddieat/zk_libcod, code/gsc_http.cpp and
 * code/gsc_websocket.cpp), under its MIT license:
 *
 * MIT License
 *
 * Copyright (c) 2022 iBuddie
 * Copyright (c) 2022 Maxim Kuzmichev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * The network code is our own (net_http.cpp) and runs on worker threads: one
 * per HTTP request and one per WebSocket. The workers never touch the script
 * VM; they queue their results, which gsc_http_frame() hands to the script
 * callbacks on the main thread once per frame.
 */

// the STL before the engine headers
#include "net_http.hpp"
#include <deque>

#include "gsc_http.hpp"

#if LIBCOD_COMPILE_HTTP == 1

#include <pthread.h>

#define MAX_HTTP_REQUESTS 16
#define MAX_WEBSOCKETS 16
#define HTTP_THREAD_STACK (512 * 1024)
#define HTTP_MAX_TIMEOUT 86400
#define WS_CONNECT_TIMEOUT 10000
#define WS_READ_WAIT 50
#define WS_CLOSE_WAIT 1000
#define WS_MAX_QUEUED_BYTES (1024 * 1024)
#define WS_MAX_EVENTS 1024
#define WS_MAX_INCOMING_BYTES (1024 * 1024)
#define WS_EVENTS_PER_FRAME 64

static dvar_t *sv_httpAllowedHosts;
static dvar_t *sv_httpCaFile;
static bool http_allowPrivate;

// guards the fields the workers share with the main thread
static pthread_mutex_t http_mutex = PTHREAD_MUTEX_INITIALIZER;

struct http_job_t
{
	// set before the worker starts, then only read
	NetHttpRequest req;
	// written by the worker, read by the main thread once finished is set
	NetHttpResponse res;
	volatile int cancel;
	// main thread
	std::string logUrl;
	unsigned int levelId;
	int onDone;
	int onError;
	int64_t deadline;
	bool timedOut;
	http_job_t *next;
	// http_mutex
	int refs;
	bool finished;
};

// main thread: in progress, newest first
static http_job_t *http_jobs;
// main thread: finished or timed out, oldest first, waiting for their callbacks
static http_job_t *http_ready;
// main thread: the job whose callback is running
static http_job_t *http_delivering;
// http_mutex: workers still running, cancelled ones included
static int http_running;

enum
{
	WS_EVENT_OPEN,
	WS_EVENT_MESSAGE,
	WS_EVENT_CLOSE,
	WS_EVENT_ERROR,
	WS_EVENT_LOG,
	WS_EVENT_FINISHED
};

struct ws_event_t
{
	int type;
	std::string text;
	bool closedByRemote;
	bool fullyDisconnected;
};

struct ws_conn_t
{
	// set before the worker starts, then only read
	NetUrl url;
	std::string headers;
	NetTlsOptions tls;
	int reconnectMs;
	int pingMs;
	volatile int stop;
	// main thread
	std::string logUrl;
	unsigned int levelId;
	int onConnect;
	int onMessage;
	int onClose;
	int onError;
	// http_mutex
	int refs;
	bool connected;
	std::deque<std::string> outgoing;
	size_t outgoingBytes;
	std::deque<ws_event_t> events;
	size_t incomingBytes;
	int droppedMessages;
};

// main thread
static ws_conn_t *ws_slots[MAX_WEBSOCKETS];

void gsc_http_register_dvars()
{
	sv_httpAllowedHosts = Dvar_RegisterString("sv_httpAllowedHosts", "", DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_httpCaFile = Dvar_RegisterString("sv_httpCaFile", "", DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);

	// only from the command line, and read once: setCvar from a script can change any dvar
	http_allowPrivate = Dvar_RegisterBool("sv_httpAllowPrivate", false, DVAR_INIT | DVAR_CHANGEABLE_RESET)->current.boolean;
}

static bool Http_StartThread(void *(*func)(void *), void *arg)
{
	pthread_t thread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr, HTTP_THREAD_STACK);
	int r = Sys_CreatePthread(&thread, &attr, func, arg);
	pthread_attr_destroy(&attr);

	return r == 0;
}

// Checks the URL and the host against sv_httpAllowedHosts, and fills the TLS options.
static const char *Http_PrepareUrl(const char *text, bool websocket, int allowInsecure, NetUrl *url, NetTlsOptions *tls, const char *function)
{
	if ( !NetHttp_ParseUrl(text, websocket, url) )
		return "Invalid URL";

	if ( !NetHttp_HostAllowed(url->host.c_str(), sv_httpAllowedHosts->current.string) )
		return "Host not allowed by sv_httpAllowedHosts";

	tls->allowInsecure = allowInsecure != 0;
	tls->caFile = sv_httpCaFile->current.string;
	tls->allowPrivate = http_allowPrivate;

	if ( url->tls && tls->allowInsecure )
		Com_Printf("^3%s: certificate verification is off for %s\n", function, url->host.c_str());

	return NULL;
}

static void Http_ReleaseJob(http_job_t *job)
{
	pthread_mutex_lock(&http_mutex);
	bool last = --job->refs == 0;
	pthread_mutex_unlock(&http_mutex);

	if ( last )
		delete job;
}

static void * Http_Worker(void *arg)
{
	http_job_t *job = (http_job_t *)arg;

	NetHttp_Fetch(&job->req, &job->cancel, &job->res);

	pthread_mutex_lock(&http_mutex);
	job->finished = true;
	http_running--;
	pthread_mutex_unlock(&http_mutex);

	Http_ReleaseJob(job);
	return NULL;
}

/*
 * httpFetch(url, method, data, headers, timeout, ::onDone, ::onError, [allowInsecure])
 *
 * Exactly one callback runs, on a later frame:
 *   onDone(status, body, headers)  headers is an array keyed by lowercase name
 *   onError(error)
 * timeout is in seconds for the whole request, 0 for the longest (a day). Redirects are not
 * followed: a 3xx comes to onDone with its "location" header.
 */
void gsc_http_fetch()
{
	const char *url;
	const char *method;
	const char *data;
	const char *headers;
	int timeout;
	int onDone;
	int onError;
	int allowInsecure = 0;

	if ( Scr_GetNumParam() < 7 )
	{
		stackError("httpFetch() takes at least 7 arguments: url, method, data, headers, timeout, onDone, onError, [allowInsecure]");
		stackPushUndefined();
		return;
	}

	if ( !stackGetParamString(0, &url) ||
	     !stackGetParamString(1, &method) ||
	     !stackGetParamString(2, &data) ||
	     !stackGetParamString(3, &headers) ||
	     !stackGetParamInt(4, &timeout) ||
	     !stackGetParamFunction(5, &onDone) ||
	     !stackGetParamFunction(6, &onError) ||
	     ( Scr_GetNumParam() > 7 && !stackGetParamInt(7, &allowInsecure) ) )
	{
		stackError("httpFetch() has an undefined or wrong-typed argument");
		stackPushUndefined();
		return;
	}

	http_job_t *job = new http_job_t;
	job->req.method = *method ? method : "GET";
	job->req.headers = headers;
	job->req.body = data;
	// even with no timeout, a server that sends a byte now and then can't hold a slot forever
	if ( timeout <= 0 || timeout > HTTP_MAX_TIMEOUT )
		timeout = HTTP_MAX_TIMEOUT;
	job->req.timeoutMs = timeout * 1000;
	job->res.status = 0;
	job->cancel = 0;
	job->levelId = libcodLevelGeneration;
	job->onDone = onDone;
	job->onError = onError;
	job->deadline = NetHttp_Milliseconds() + job->req.timeoutMs;
	job->timedOut = false;
	job->refs = 1;
	job->finished = true;

	// errors found here still go to onError on a later frame, as in zk_libcod
	const char *error = Http_PrepareUrl(url, false, allowInsecure, &job->req.url, &job->req.tls, "httpFetch");
	if ( !error && !NetHttp_ValidMethod(job->req.method.c_str()) )
		error = "Invalid method";

	if ( !error )
	{
		job->logUrl = NetHttp_LogUrl(&job->req.url);

		pthread_mutex_lock(&http_mutex);
		if ( http_running >= MAX_HTTP_REQUESTS )
			error = "Too many requests in progress";
		else
		{
			http_running++;
			job->refs = 2;
			job->finished = false;
		}
		pthread_mutex_unlock(&http_mutex);

		if ( !error && !Http_StartThread(Http_Worker, job) )
		{
			pthread_mutex_lock(&http_mutex);
			http_running--;
			job->refs = 1;
			job->finished = true;
			pthread_mutex_unlock(&http_mutex);
			error = "Cannot start a thread";
		}
	}

	if ( error )
		job->res.error = error;

	job->next = http_jobs;
	http_jobs = job;
}

static void Http_Deliver(http_job_t *job, const char *timeoutError)
{
	if ( !Scr_IsSystemActive() || libcodLevelGeneration != job->levelId )
		return;

	if ( timeoutError || !job->res.error.empty() )
	{
		if ( job->onError )
		{
			stackPushString(timeoutError ? timeoutError : job->res.error.c_str());
			short ret = Scr_ExecThread(job->onError, 1);
			Scr_FreeThread(ret);
		}
		return;
	}

	if ( !job->onDone )
		return;

	stackPushArray();
	for ( size_t i = 0; i < job->res.headers.size(); i++ )
	{
		// a NUL would make two names the same key, which the array can't hold twice
		if ( strlen(job->res.headers[i].first.c_str()) != job->res.headers[i].first.size() )
			continue;
		stackPushString(job->res.headers[i].second.c_str());
		// the array keeps its own reference to the key
		unsigned int key = SL_GetString(job->res.headers[i].first.c_str(), 0);
		Scr_AddArrayStringIndexed(key);
		SL_RemoveRefToString(key);
	}
	stackPushString(job->res.body.c_str());
	stackPushInt(job->res.status);
	short ret = Scr_ExecThread(job->onDone, 3);
	Scr_FreeThread(ret);
}

static void Http_RunFrame()
{
	int64_t now = NetHttp_Milliseconds();
	http_job_t **link = &http_jobs;
	http_job_t *ready = NULL;

	// The callbacks may start new requests, so the list is sorted out first
	// and the new requests wait for the next frame.
	while ( *link )
	{
		http_job_t *job = *link;

		pthread_mutex_lock(&http_mutex);
		bool finished = job->finished;
		pthread_mutex_unlock(&http_mutex);

		if ( !finished && job->deadline && now >= job->deadline )
		{
			// the worker stops on its own; its result is thrown away
			__atomic_store_n(&job->cancel, 1, __ATOMIC_RELEASE);
			job->timedOut = true;
		}
		else if ( !finished )
		{
			link = &job->next;
			continue;
		}

		// http_jobs is newest first, so this puts the oldest first
		*link = job->next;
		job->next = ready;
		ready = job;
	}

	link = &http_ready;
	while ( *link )
		link = &(*link)->next;
	*link = ready;

	// One at a time, each released after its callback: an error that leaves a
	// callback with a longjmp leaves the others to gsc_http_cleanup_on_level_load.
	while ( http_ready )
	{
		http_job_t *job = http_ready;
		http_ready = job->next;

		http_delivering = job;
		Http_Deliver(job, job->timedOut ? "Timeout" : NULL);
		// a level load from the callback has released it already
		if ( http_delivering == job )
		{
			http_delivering = NULL;
			Http_ReleaseJob(job);
		}
	}
}

static bool Ws_Stopped(ws_conn_t *ws)
{
	return __atomic_load_n(&ws->stop, __ATOMIC_ACQUIRE) != 0;
}

static void Ws_Push(ws_conn_t *ws, int type, const std::string &text = "", bool closedByRemote = false, bool fullyDisconnected = false)
{
	ws_event_t ev;
	ev.type = type;
	ev.text = text;
	ev.closedByRemote = closedByRemote;
	ev.fullyDisconnected = fullyDisconnected;

	pthread_mutex_lock(&http_mutex);
	// messages the script hasn't taken yet are kept up to a count and a size, the rest are dropped
	if ( type == WS_EVENT_MESSAGE && ( ws->events.size() >= WS_MAX_EVENTS || ws->incomingBytes + text.size() > WS_MAX_INCOMING_BYTES ) )
		ws->droppedMessages++;
	else
	{
		ws->events.push_back(ev);
		if ( type == WS_EVENT_MESSAGE )
			ws->incomingBytes += text.size();
	}
	pthread_mutex_unlock(&http_mutex);
}

static void Ws_Release(ws_conn_t *ws)
{
	pthread_mutex_lock(&http_mutex);
	bool last = --ws->refs == 0;
	pthread_mutex_unlock(&http_mutex);

	if ( last )
		delete ws;
}

// Runs one connected session. Returns true when it was closed from our side.
static bool Ws_Session(ws_conn_t *ws, NetWebSocket *sock)
{
	std::string error;
	std::string message;
	int messageType = 0;
	bool oversized = false;
	bool waitingPong = false;
	int64_t pongDeadline = 0;
	int64_t nextPing = NetHttp_Milliseconds() + ws->pingMs;
	NetWsFrame frame;

	for ( ;; )
	{
		if ( Ws_Stopped(ws) )
		{
			// 1000, normal closure; then wait a moment for the server's reply
			static const char normal[2] = { 0x03, (char)0xe8 };
			if ( NetWs_SendFrame(sock, NET_WS_CLOSE, normal, 2, &error) )
			{
				int64_t end = NetHttp_Milliseconds() + WS_CLOSE_WAIT;
				while ( NetHttp_Milliseconds() < end )
				{
					int r = NetWs_ReadFrame(sock, &frame, WS_READ_WAIT, &error);
					if ( r < 0 || ( r > 0 && frame.opcode == NET_WS_CLOSE ) )
						break;
				}
			}
			return true;
		}

		std::deque<std::string> outgoing;
		pthread_mutex_lock(&http_mutex);
		outgoing.swap(ws->outgoing);
		ws->outgoingBytes = 0;
		pthread_mutex_unlock(&http_mutex);

		for ( size_t i = 0; i < outgoing.size(); i++ )
		{
			if ( !NetWs_SendFrame(sock, NET_WS_TEXT, outgoing[i].data(), outgoing[i].size(), &error) )
			{
				Ws_Push(ws, WS_EVENT_ERROR, error);
				return false;
			}
		}

		if ( ws->pingMs > 0 )
		{
			int64_t now = NetHttp_Milliseconds();

			if ( waitingPong && now >= pongDeadline )
			{
				Ws_Push(ws, WS_EVENT_ERROR, "Ping timeout (no pong received within time limit)");
				return false;
			}

			if ( !waitingPong && now >= nextPing )
			{
				if ( !NetWs_SendFrame(sock, NET_WS_PING, "", 0, &error) )
				{
					Ws_Push(ws, WS_EVENT_ERROR, error);
					return false;
				}
				waitingPong = true;
				// as zk_libcod: half the interval, but at least a second
				pongDeadline = now + ( ws->pingMs / 2 > 1000 ? ws->pingMs / 2 : 1000 );
				nextPing = now + ws->pingMs;
			}
		}

		int r = NetWs_ReadFrame(sock, &frame, WS_READ_WAIT, &error);
		if ( r < 0 )
		{
			// an empty error is a plain hang-up
			if ( !error.empty() )
				Ws_Push(ws, WS_EVENT_ERROR, error);
			return false;
		}
		if ( r == 0 )
			continue;

		switch ( frame.opcode )
		{
		case NET_WS_TEXT:
		case NET_WS_BINARY:
		case NET_WS_CONT:
			if ( ( frame.opcode == NET_WS_CONT ) != ( messageType != 0 ) )
			{
				Ws_Push(ws, WS_EVENT_ERROR, "Protocol error: unexpected continuation frame");
				return false;
			}
			if ( frame.opcode != NET_WS_CONT )
			{
				messageType = frame.opcode;
				message.clear();
				oversized = false;
			}
			// binary messages are ignored, as in zk_libcod
			if ( messageType == NET_WS_TEXT && !oversized )
			{
				if ( message.size() + frame.payload.size() > NET_HTTP_MAX_STRING )
				{
					oversized = true;
					message.clear();
				}
				else
					message += frame.payload;
			}
			if ( frame.fin )
			{
				if ( messageType == NET_WS_TEXT )
				{
					if ( oversized )
						Ws_Push(ws, WS_EVENT_LOG, "dropped a message over 65000 bytes");
					else
						Ws_Push(ws, WS_EVENT_MESSAGE, message);
				}
				messageType = 0;
				message.clear();
			}
			break;

		case NET_WS_PING:
			if ( !NetWs_SendFrame(sock, NET_WS_PONG, frame.payload.data(), frame.payload.size(), &error) )
			{
				Ws_Push(ws, WS_EVENT_ERROR, error);
				return false;
			}
			break;

		case NET_WS_PONG:
			waitingPong = false;
			break;

		case NET_WS_CLOSE:
			// echo the status code, then hang up
			NetWs_SendFrame(sock, NET_WS_CLOSE, frame.payload.data(), frame.payload.size() >= 2 ? 2 : 0, &error);
			return false;

		default:
			Ws_Push(ws, WS_EVENT_ERROR, "Protocol error: unknown opcode");
			return false;
		}
	}
}

static void * Ws_Worker(void *arg)
{
	ws_conn_t *ws = (ws_conn_t *)arg;

	for ( ;; )
	{
		NetWebSocket sock;
		std::string error;

		if ( Ws_Stopped(ws) )
			break;

		if ( !NetWs_Connect(&sock, &ws->url, ws->headers.c_str(), &ws->tls, WS_CONNECT_TIMEOUT, &ws->stop, &error) )
		{
			if ( !Ws_Stopped(ws) )
				Ws_Push(ws, WS_EVENT_ERROR, error);
		}
		else
		{
			pthread_mutex_lock(&http_mutex);
			ws->connected = true;
			pthread_mutex_unlock(&http_mutex);

			Ws_Push(ws, WS_EVENT_OPEN);
			bool local = Ws_Session(ws, &sock);

			pthread_mutex_lock(&http_mutex);
			ws->connected = false;
			ws->outgoing.clear();
			ws->outgoingBytes = 0;
			pthread_mutex_unlock(&http_mutex);

			NetWs_Close(&sock);
			Ws_Push(ws, WS_EVENT_CLOSE, "", !local, local || ws->reconnectMs <= 0 || Ws_Stopped(ws));
		}

		if ( ws->reconnectMs <= 0 )
			break;

		int64_t end = NetHttp_Milliseconds() + ws->reconnectMs;
		while ( !Ws_Stopped(ws) && NetHttp_Milliseconds() < end )
			usleep(WS_READ_WAIT * 1000);
	}

	Ws_Push(ws, WS_EVENT_FINISHED);
	Ws_Release(ws);
	return NULL;
}

/*
 * webSocketConnect(url, headers, ::onConnect, ::onMessage, ::onClose, ::onError, [reconnectDelayMs], [pingIntervalMs], [allowInsecure])
 *
 * Returns the connection id (0-15), or -1. Callbacks:
 *   onConnect()
 *   onMessage(text)                            a whole TEXT message
 *   onClose(closedByRemote, fullyDisconnected)
 *   onError(error)
 * reconnectDelayMs defaults to 2000 (0 connects once), pingIntervalMs to
 * 15000 (0 for no pings). The connections belong to the level: they are all
 * closed on map load.
 */
void gsc_websocket_connect()
{
	const char *url;
	const char *headers;
	int onConnect;
	int onMessage;
	int onClose;
	int onError;
	int reconnectMs = 2000;
	int pingMs = 15000;
	int allowInsecure = 0;
	int slot;

	if ( Scr_GetNumParam() < 6 )
	{
		stackError("webSocketConnect() takes at least 6 arguments: url, headers, onConnect, onMessage, onClose, onError, [reconnectDelayMs], [pingIntervalMs], [allowInsecure]");
		stackPushInt(-1);
		return;
	}

	if ( !stackGetParamString(0, &url) ||
	     !stackGetParamString(1, &headers) ||
	     !stackGetParamFunction(2, &onConnect) ||
	     !stackGetParamFunction(3, &onMessage) ||
	     !stackGetParamFunction(4, &onClose) ||
	     !stackGetParamFunction(5, &onError) ||
	     ( Scr_GetNumParam() > 6 && !stackGetParamInt(6, &reconnectMs) ) ||
	     ( Scr_GetNumParam() > 7 && !stackGetParamInt(7, &pingMs) ) ||
	     ( Scr_GetNumParam() > 8 && !stackGetParamInt(8, &allowInsecure) ) )
	{
		stackError("webSocketConnect() has an undefined or wrong-typed argument");
		stackPushInt(-1);
		return;
	}

	for ( slot = 0; slot < MAX_WEBSOCKETS; slot++ )
	{
		if ( !ws_slots[slot] )
			break;
	}
	if ( slot == MAX_WEBSOCKETS )
	{
		stackError("webSocketConnect() has no free websocket client slots available");
		stackPushInt(-1);
		return;
	}

	ws_conn_t *ws = new ws_conn_t;
	const char *error = Http_PrepareUrl(url, true, allowInsecure, &ws->url, &ws->tls, "webSocketConnect");
	if ( error )
	{
		Com_Printf("webSocketConnect: %s\n", error);
		delete ws;
		stackPushInt(-1);
		return;
	}

	ws->headers = headers;
	ws->reconnectMs = reconnectMs > 0 ? reconnectMs : 0;
	ws->pingMs = pingMs > 0 ? pingMs : 0;
	ws->stop = 0;
	ws->logUrl = NetHttp_LogUrl(&ws->url);
	ws->levelId = libcodLevelGeneration;
	ws->onConnect = onConnect;
	ws->onMessage = onMessage;
	ws->onClose = onClose;
	ws->onError = onError;
	ws->refs = 2;
	ws->connected = false;
	ws->outgoingBytes = 0;
	ws->incomingBytes = 0;
	ws->droppedMessages = 0;

	if ( !Http_StartThread(Ws_Worker, ws) )
	{
		Com_Printf("webSocketConnect: cannot start a thread\n");
		delete ws;
		stackPushInt(-1);
		return;
	}

	ws_slots[slot] = ws;
	stackPushInt(slot);
}

/*
 * webSocketSendText(connectionId, message)
 * Queues a TEXT message. Returns false when not connected.
 */
void gsc_websocket_sendtext()
{
	int id;
	const char *message;

	if ( Scr_GetNumParam() != 2 )
	{
		stackError("webSocketSendText() takes 2 arguments: connectionId, message");
		stackPushBool(false);
		return;
	}

	if ( !stackGetParamInt(0, &id) || !stackGetParamString(1, &message) )
	{
		stackError("webSocketSendText() has an undefined or wrong-typed argument");
		stackPushBool(false);
		return;
	}

	if ( id < 0 || id >= MAX_WEBSOCKETS || !ws_slots[id] || Ws_Stopped(ws_slots[id]) )
	{
		stackPushBool(false);
		return;
	}

	ws_conn_t *ws = ws_slots[id];
	size_t len = strlen(message);
	bool queued = false;

	pthread_mutex_lock(&http_mutex);
	if ( ws->connected && ws->outgoingBytes + len <= WS_MAX_QUEUED_BYTES )
	{
		ws->outgoing.push_back(std::string(message, len));
		ws->outgoingBytes += len;
		queued = true;
	}
	pthread_mutex_unlock(&http_mutex);

	stackPushBool(queued);
}

/*
 * webSocketClose(connectionId)
 * Closes the connection and stops reconnecting. Returns true if the id was valid.
 */
void gsc_websocket_close()
{
	int id;

	if ( Scr_GetNumParam() != 1 || !stackGetParamInt(0, &id) )
	{
		stackPushBool(false);
		return;
	}

	if ( id < 0 || id >= MAX_WEBSOCKETS || !ws_slots[id] )
	{
		stackPushBool(false);
		return;
	}

	__atomic_store_n(&ws_slots[id]->stop, 1, __ATOMIC_RELEASE);
	stackPushBool(true);
}

static void Ws_Deliver(ws_conn_t *ws, const ws_event_t *ev)
{
	short ret;

	if ( !Scr_IsSystemActive() || libcodLevelGeneration != ws->levelId )
		return;

	switch ( ev->type )
	{
	case WS_EVENT_OPEN:
		if ( ws->onConnect )
		{
			ret = Scr_ExecThread(ws->onConnect, 0);
			Scr_FreeThread(ret);
		}
		break;

	case WS_EVENT_MESSAGE:
		if ( ws->onMessage )
		{
			stackPushString(ev->text.c_str());
			ret = Scr_ExecThread(ws->onMessage, 1);
			Scr_FreeThread(ret);
		}
		break;

	case WS_EVENT_CLOSE:
		if ( ws->onClose )
		{
			stackPushBool(ev->fullyDisconnected);
			stackPushBool(ev->closedByRemote);
			ret = Scr_ExecThread(ws->onClose, 2);
			Scr_FreeThread(ret);
		}
		break;

	case WS_EVENT_ERROR:
		if ( ws->onError )
		{
			stackPushString(ev->text.c_str());
			ret = Scr_ExecThread(ws->onError, 1);
			Scr_FreeThread(ret);
		}
		break;
	}
}

static void Ws_RunFrame()
{
	for ( int i = 0; i < MAX_WEBSOCKETS; i++ )
	{
		ws_conn_t *ws = ws_slots[i];
		int dropped;

		if ( !ws )
			continue;

		pthread_mutex_lock(&http_mutex);
		dropped = ws->droppedMessages;
		ws->droppedMessages = 0;
		pthread_mutex_unlock(&http_mutex);

		if ( dropped )
			Com_Printf("webSocket %s: dropped %d messages, the script can't keep up\n", ws->logUrl.c_str(), dropped);

		// a few per frame, so a flood can't stall the frame; the rest wait in the queue
		for ( int j = 0; j < WS_EVENTS_PER_FRAME; j++ )
		{
			ws_event_t ev;

			pthread_mutex_lock(&http_mutex);
			bool have = !ws->events.empty();
			if ( have )
			{
				ws_event_t &front = ws->events.front();
				ev.type = front.type;
				ev.text.swap(front.text);
				ev.closedByRemote = front.closedByRemote;
				ev.fullyDisconnected = front.fullyDisconnected;
				ws->events.pop_front();
				if ( ev.type == WS_EVENT_MESSAGE )
					ws->incomingBytes -= ev.text.size();
			}
			pthread_mutex_unlock(&http_mutex);

			if ( !have )
				break;

			if ( ev.type == WS_EVENT_FINISHED )
			{
				// the slot is free again
				ws_slots[i] = NULL;
				Ws_Release(ws);
				break;
			}

			if ( ev.type == WS_EVENT_LOG )
				Com_Printf("webSocket %s: %s\n", ws->logUrl.c_str(), ev.text.c_str());
			else
				Ws_Deliver(ws, &ev);

			// a level load from the callback has let the connection go
			if ( ws_slots[i] != ws )
				break;
		}
	}
}

// Called at the start of every game frame, from Scr_RunFrameCallbacks.
void gsc_http_frame()
{
	if ( http_jobs || http_ready )
		Http_RunFrame();
	Ws_RunFrame();
}

// The callbacks belong to the level's scripts, so a new level (map change or
// restart) cancels the requests and closes the connections of the old one.
// The workers finish on their own and free what they hold.
void gsc_http_cleanup_on_level_load()
{
	while ( http_jobs )
	{
		http_job_t *job = http_jobs;
		http_jobs = job->next;
		__atomic_store_n(&job->cancel, 1, __ATOMIC_RELEASE);
		Http_ReleaseJob(job);
	}

	while ( http_ready )
	{
		http_job_t *job = http_ready;
		http_ready = job->next;
		Http_ReleaseJob(job);
	}

	if ( http_delivering )
	{
		Http_ReleaseJob(http_delivering);
		http_delivering = NULL;
	}

	for ( int i = 0; i < MAX_WEBSOCKETS; i++ )
	{
		if ( !ws_slots[i] )
			continue;
		__atomic_store_n(&ws_slots[i]->stop, 1, __ATOMIC_RELEASE);
		Ws_Release(ws_slots[i]);
		ws_slots[i] = NULL;
	}
}

#endif
