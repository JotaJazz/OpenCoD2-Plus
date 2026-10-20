#ifndef _NET_HTTP_HPP_
#define _NET_HTTP_HPP_

/*
 * Blocking HTTP/1.1 and WebSocket (RFC 6455) client over plain TCP or Mbed TLS,
 * for the httpFetch and webSocket* script functions (gsc_http.cpp). Everything
 * here runs on worker threads and never touches the engine: it only gets
 * copies of what the script passed and returns plain data or error strings.
 */

#if LIBCOD_COMPILE_HTTP == 1

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

// The longest string the script VM takes (zk_libcod uses the same bound):
// bodies, header blocks and WebSocket messages above it are refused.
#define NET_HTTP_MAX_STRING 65000

// read result when nothing arrived in time
#define NET_AGAIN -2

struct NetUrl
{
	bool tls;
	bool websocket;
	std::string host;     // without the brackets of an IPv6 literal
	int port;
	std::string target;   // path and query, at least "/"
	std::string userinfo; // "user:password" before the host, sent as basic authorization
};

struct NetTlsOptions
{
	bool allowInsecure;
	std::string caFile;   // PEM file with the trusted certificates, empty for the system ones
};

struct NetHttpRequest
{
	NetUrl url;
	std::string method;
	std::string headers;  // "Name: value" lines separated by "\r\n"
	std::string body;
	int timeoutMs;        // whole request, 0 for none
	NetTlsOptions tls;
};

struct NetHttpResponse
{
	int status;
	std::vector<std::pair<std::string, std::string> > headers; // lowercase names
	std::string body;
	std::string error;    // set when the request failed
};

struct NetConn;

int64_t NetHttp_Milliseconds();
bool NetHttp_ParseUrl(const char *text, bool websocket, NetUrl *url);
std::string NetHttp_LogUrl(const NetUrl *url);
bool NetHttp_HostAllowed(const char *host, const char *allowed);
bool NetHttp_ValidMethod(const char *method);

// cancel is polled while waiting, so another thread can stop a request
bool NetHttp_Fetch(const NetHttpRequest *req, const volatile int *cancel, NetHttpResponse *res);

// WebSocket session; the connection is owned by one thread
struct NetWebSocket
{
	NetConn *conn;
	std::string rx;       // received bytes not parsed yet
};

enum
{
	NET_WS_CONT = 0,
	NET_WS_TEXT = 1,
	NET_WS_BINARY = 2,
	NET_WS_CLOSE = 8,
	NET_WS_PING = 9,
	NET_WS_PONG = 10
};

struct NetWsFrame
{
	bool fin;
	int opcode;
	std::string payload;
};

bool NetWs_Connect(NetWebSocket *ws, const NetUrl *url, const char *headers, const NetTlsOptions *tls,
                   int timeoutMs, const volatile int *cancel, std::string *error);
// 1 with a frame, 0 when none arrived within waitMs, -1 on errors and when the server hung up
int NetWs_ReadFrame(NetWebSocket *ws, NetWsFrame *frame, int waitMs, std::string *error);
bool NetWs_SendFrame(NetWebSocket *ws, int opcode, const char *data, size_t len, std::string *error);
void NetWs_Close(NetWebSocket *ws);

#endif

#endif
