/*
 * The part of libcurl's easy and multi interfaces that the redirected
 * download (src/PC/qcommon/dl_main.c) uses, on WinINet, for the MinGW
 * client: the 1.3 binary downloads with its own HTTP library over WSOCK32,
 * the Linux client links libcurl, and Windows has no libcurl of its own.
 *
 * Each transfer runs on a thread of its own; curl_multi_perform hands the
 * bytes it has received to the write callback on the caller's thread, so
 * dl_main.c sees the same order of calls as with libcurl.
 */
#if defined(W32_CLIENT) && defined(__MINGW32__)

#    include <windows.h>
#    include <wininet.h>
#    include <process.h>
#    include <stdarg.h>
#    include <stdlib.h>
#    include <string.h>

/* the values dl_main.c passes, as libcurl's headers define them */
#    define CURLOPT_WRITEDATA 10001
#    define CURLOPT_URL 10002
#    define CURLOPT_USERAGENT 10018
#    define CURLOPT_WRITEFUNCTION 20011
#    define CURLOPT_NOPROGRESS 43
#    define CURLOPT_FAILONERROR 45
#    define CURLOPT_FOLLOWLOCATION 52
#    define CURLOPT_CONNECTTIMEOUT 78
#    define CURLINFO_RESPONSE_CODE 0x200002

#    define CURLE_OK 0
#    define CURLE_COULDNT_CONNECT 7
#    define CURLE_HTTP_RETURNED_ERROR 22
#    define CURLE_WRITE_ERROR 23
#    define CURLE_OUT_OF_MEMORY 27
#    define CURLE_ABORTED_BY_CALLBACK 42
#    define CURLE_RECV_ERROR 56

#    define CURLMSG_DONE 1
#    define WININET_MAX_HANDLES 4

typedef size_t (*curl_write_cb)(void *ptr, size_t size, size_t nmemb, void *stream);

typedef struct {
    int msg;
    void *easy_handle;
    union {
        void *whatever;
        int result;
    } data;
} CURLMsg;

typedef struct {
    char *url;
    char userAgent[128];
    curl_write_cb write;
    void *writeData;
    long connectTimeout;
    long failOnError;
    long followLocation;

    /* shared with the transfer thread, under lock */
    CRITICAL_SECTION lock;
    HINTERNET session;
    HINTERNET request;
    char *buf;
    size_t len, cap;
    int finished;
    int aborted;
    int result;
    long responseCode;

    HANDLE thread;
    int doneReported;
} wininet_easy_t;

typedef struct {
    wininet_easy_t *handles[WININET_MAX_HANDLES];
    CURLMsg msg;
} wininet_multi_t;

static void WinInet_Finish(wininet_easy_t *e, int result)
{
    EnterCriticalSection(&e->lock);
    if (e->result == CURLE_OK)
        e->result = result;
    e->finished = 1;
    LeaveCriticalSection(&e->lock);
}

static unsigned __stdcall WinInet_Transfer(void *arg)
{
    wininet_easy_t *e = (wininet_easy_t *)arg;
    HINTERNET session, request;
    DWORD flags, code = 0, codeLen = sizeof(code), timeout;
    char chunk[16384];

    session = InternetOpenA(e->userAgent[0] ? e->userAgent : "CoD2 wwwdl", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!session) {
        WinInet_Finish(e, CURLE_COULDNT_CONNECT);
        return 0;
    }
    timeout = e->connectTimeout > 0 ? (DWORD)e->connectTimeout * 1000 : 30000;
    InternetSetOptionA(session, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));

    EnterCriticalSection(&e->lock);
    if (e->aborted) {
        LeaveCriticalSection(&e->lock);
        InternetCloseHandle(session);
        WinInet_Finish(e, CURLE_ABORTED_BY_CALLBACK);
        return 0;
    }
    e->session = session;
    LeaveCriticalSection(&e->lock);

    flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES;
    if (!e->followLocation)
        flags |= INTERNET_FLAG_NO_AUTO_REDIRECT;
    request = InternetOpenUrlA(session, e->url, NULL, 0, flags, 0);
    if (!request) {
        WinInet_Finish(e, CURLE_COULDNT_CONNECT);
        return 0;
    }
    EnterCriticalSection(&e->lock);
    e->request = request;
    LeaveCriticalSection(&e->lock);

    if (HttpQueryInfoA(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &code, &codeLen, NULL)) {
        EnterCriticalSection(&e->lock);
        e->responseCode = (long)code;
        LeaveCriticalSection(&e->lock);
        if (e->failOnError && code >= 400) {
            WinInet_Finish(e, CURLE_HTTP_RETURNED_ERROR);
            return 0;
        }
    }

    for (;;) {
        DWORD got = 0;

        if (!InternetReadFile(request, chunk, sizeof(chunk), &got)) {
            WinInet_Finish(e, CURLE_RECV_ERROR);
            return 0;
        }
        if (got == 0)
            break;

        EnterCriticalSection(&e->lock);
        if (e->aborted) {
            LeaveCriticalSection(&e->lock);
            WinInet_Finish(e, CURLE_ABORTED_BY_CALLBACK);
            return 0;
        }
        if (e->len + got > e->cap) {
            size_t cap = e->cap ? e->cap : 65536;
            char *grown;

            while (cap < e->len + got)
                cap *= 2;
            grown = (char *)realloc(e->buf, cap);
            if (!grown) {
                LeaveCriticalSection(&e->lock);
                WinInet_Finish(e, CURLE_OUT_OF_MEMORY);
                return 0;
            }
            e->buf = grown;
            e->cap = cap;
        }
        memcpy(e->buf + e->len, chunk, got);
        e->len += got;
        LeaveCriticalSection(&e->lock);
    }

    WinInet_Finish(e, CURLE_OK);
    return 0;
}

/* stops the transfer: closing the session ends a blocking WinINet call */
static void WinInet_Stop(wininet_easy_t *e)
{
    HINTERNET session, request;

    if (!e->thread)
        return;
    EnterCriticalSection(&e->lock);
    e->aborted = 1;
    session = e->session;
    request = e->request;
    e->session = 0;
    e->request = 0;
    LeaveCriticalSection(&e->lock);

    if (request)
        InternetCloseHandle(request);
    if (session)
        InternetCloseHandle(session);
    WaitForSingleObject(e->thread, INFINITE);
    CloseHandle(e->thread);
    e->thread = 0;

    /* handles the thread opened after the abort was seen */
    EnterCriticalSection(&e->lock);
    request = e->request;
    session = e->session;
    e->request = 0;
    e->session = 0;
    LeaveCriticalSection(&e->lock);
    if (request)
        InternetCloseHandle(request);
    if (session)
        InternetCloseHandle(session);
}

void *curl_easy_init(void)
{
    wininet_easy_t *e = (wininet_easy_t *)calloc(1, sizeof(*e));

    if (!e)
        return 0;
    InitializeCriticalSection(&e->lock);
    e->failOnError = 0;
    e->followLocation = 0;
    return e;
}

int curl_easy_setopt(void *handle, int option, ...)
{
    wininet_easy_t *e = (wininet_easy_t *)handle;
    va_list ap;

    if (!e)
        return 1;
    va_start(ap, option);
    switch (option) {
    case CURLOPT_URL: {
        const char *url = va_arg(ap, const char *);
        free(e->url);
        e->url = url ? _strdup(url) : 0;
        break;
    }
    case CURLOPT_USERAGENT: {
        const char *ua = va_arg(ap, const char *);
        if (ua) {
            strncpy(e->userAgent, ua, sizeof(e->userAgent) - 1);
            e->userAgent[sizeof(e->userAgent) - 1] = '\0';
        }
        break;
    }
    case CURLOPT_WRITEFUNCTION:
        e->write = va_arg(ap, curl_write_cb);
        break;
    case CURLOPT_WRITEDATA:
        e->writeData = va_arg(ap, void *);
        break;
    case CURLOPT_CONNECTTIMEOUT:
        e->connectTimeout = va_arg(ap, long);
        break;
    case CURLOPT_FAILONERROR:
        e->failOnError = va_arg(ap, long);
        break;
    case CURLOPT_FOLLOWLOCATION:
        e->followLocation = va_arg(ap, long);
        break;
    case CURLOPT_NOPROGRESS:
        (void)va_arg(ap, long);
        break;
    default:
        break;
    }
    va_end(ap);
    return CURLE_OK;
}

int curl_easy_getinfo(void *handle, int info, ...)
{
    wininet_easy_t *e = (wininet_easy_t *)handle;
    va_list ap;

    if (!e)
        return 1;
    va_start(ap, info);
    if (info == CURLINFO_RESPONSE_CODE) {
        long *out = va_arg(ap, long *);
        EnterCriticalSection(&e->lock);
        if (out)
            *out = e->responseCode;
        LeaveCriticalSection(&e->lock);
    }
    va_end(ap);
    return CURLE_OK;
}

void curl_easy_cleanup(void *handle)
{
    wininet_easy_t *e = (wininet_easy_t *)handle;

    if (!e)
        return;
    WinInet_Stop(e);
    DeleteCriticalSection(&e->lock);
    free(e->url);
    free(e->buf);
    free(e);
}

const char *curl_easy_strerror(int code)
{
    switch (code) {
    case CURLE_OK:
        return "No error";
    case CURLE_COULDNT_CONNECT:
        return "Couldn't connect to server";
    case CURLE_HTTP_RETURNED_ERROR:
        return "HTTP response code said error";
    case CURLE_WRITE_ERROR:
        return "Failed writing received data to disk/application";
    case CURLE_OUT_OF_MEMORY:
        return "Out of memory";
    case CURLE_ABORTED_BY_CALLBACK:
        return "Operation was aborted";
    case CURLE_RECV_ERROR:
        return "Failure when receiving data from the peer";
    default:
        return "Unknown error";
    }
}

void *curl_multi_init(void)
{
    return calloc(1, sizeof(wininet_multi_t));
}

int curl_multi_add_handle(void *multi, void *easy)
{
    wininet_multi_t *m = (wininet_multi_t *)multi;
    wininet_easy_t *e = (wininet_easy_t *)easy;
    int i;

    if (!m || !e || !e->url || e->thread)
        return 1;
    for (i = 0; i < WININET_MAX_HANDLES; i++) {
        if (!m->handles[i])
            break;
    }
    if (i == WININET_MAX_HANDLES)
        return 1;

    e->finished = 0;
    e->aborted = 0;
    e->result = CURLE_OK;
    e->responseCode = 0;
    e->len = 0;
    e->doneReported = 0;
    e->thread = (HANDLE)_beginthreadex(NULL, 0, WinInet_Transfer, e, 0, NULL);
    if (!e->thread)
        return 1;
    m->handles[i] = e;
    return 0;
}

int curl_multi_remove_handle(void *multi, void *easy)
{
    wininet_multi_t *m = (wininet_multi_t *)multi;
    wininet_easy_t *e = (wininet_easy_t *)easy;
    int i;

    if (!m || !e)
        return 1;
    for (i = 0; i < WININET_MAX_HANDLES; i++) {
        if (m->handles[i] == e)
            m->handles[i] = 0;
    }
    WinInet_Stop(e);
    return 0;
}

int curl_multi_cleanup(void *multi)
{
    wininet_multi_t *m = (wininet_multi_t *)multi;
    int i;

    if (!m)
        return 0;
    for (i = 0; i < WININET_MAX_HANDLES; i++) {
        if (m->handles[i])
            WinInet_Stop(m->handles[i]);
    }
    free(m);
    return 0;
}

/* the bytes received so far go to the write callback; a short write fails
   the transfer, as in libcurl */
static void WinInet_Deliver(wininet_easy_t *e)
{
    char *data;
    size_t len, written;

    EnterCriticalSection(&e->lock);
    data = e->buf;
    len = e->len;
    e->buf = 0;
    e->len = 0;
    e->cap = 0;
    LeaveCriticalSection(&e->lock);

    if (!len) {
        free(data);
        return;
    }
    written = e->write ? e->write(data, 1, len, e->writeData) : len;
    free(data);
    if (written != len) {
        WinInet_Stop(e);
        EnterCriticalSection(&e->lock);
        e->result = CURLE_WRITE_ERROR;
        e->finished = 1;
        LeaveCriticalSection(&e->lock);
    }
}

int curl_multi_perform(void *multi, int *running)
{
    wininet_multi_t *m = (wininet_multi_t *)multi;
    int i, count = 0;

    if (!m)
        return 1;
    for (i = 0; i < WININET_MAX_HANDLES; i++) {
        wininet_easy_t *e = m->handles[i];
        int finished;

        if (!e)
            continue;
        EnterCriticalSection(&e->lock);
        finished = e->finished;
        LeaveCriticalSection(&e->lock);

        /* read the flag first: after it, no more bytes arrive */
        WinInet_Deliver(e);
        if (!finished)
            count++;
    }
    if (running)
        *running = count;
    return 0;
}

void *curl_multi_info_read(void *multi, int *queued)
{
    wininet_multi_t *m = (wininet_multi_t *)multi;
    int i;

    if (queued)
        *queued = 0;
    if (!m)
        return 0;
    for (i = 0; i < WININET_MAX_HANDLES; i++) {
        wininet_easy_t *e = m->handles[i];
        int finished, result;

        if (!e || e->doneReported)
            continue;
        EnterCriticalSection(&e->lock);
        finished = e->finished && e->len == 0;
        result = e->result;
        LeaveCriticalSection(&e->lock);
        if (!finished)
            continue;

        e->doneReported = 1;
        m->msg.msg = CURLMSG_DONE;
        m->msg.easy_handle = e;
        m->msg.data.result = result;
        return &m->msg;
    }
    return 0;
}

#endif
