#include "net_http.hpp"

#if LIBCOD_COMPILE_HTTP == 1

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>
typedef SOCKET net_socket_t;
#define NET_INVALID_SOCKET INVALID_SOCKET
#define net_closesocket closesocket
#define NET_SEND_FLAGS 0
#else
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
typedef int net_socket_t;
#define NET_INVALID_SOCKET -1
#define net_closesocket close
// a peer that hangs up must not kill the server with SIGPIPE
#define NET_SEND_FLAGS MSG_NOSIGNAL
#endif

#include "mbedtls/base64.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/sha1.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "psa/crypto.h"

#define NET_CONNECT_TIMEOUT 10000
#define NET_ADDRESS_TIMEOUT 4000
#define NET_IDLE_TIMEOUT 60000
#define NET_WAIT_SLICE 100
#define NET_WS_MAX_FRAME (1024 * 1024)
#define NET_WS_MAX_HANDSHAKE 16384
#define NET_MAX_HEADERS 100
#define NET_MAX_INTERIM 5
#define NET_CA_RELOAD 3600000

// A parsed CA list, shared by the connections that trust it. It is only read
// once published in net_ca, so the handshakes of several workers can use it.
struct NetCaList
{
	mbedtls_x509_crt crt;
	std::string file;   // sv_httpCaFile, empty for the system's list
	int64_t loaded;
	int refs;           // net_ca_mutex
};

static pthread_mutex_t net_ca_mutex = PTHREAD_MUTEX_INITIALIZER;
// net_ca_mutex: the latest list, which holds a reference of its own
static NetCaList *net_ca;

struct NetConn
{
	net_socket_t fd;
	bool tls;
	const volatile int *cancel;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	NetCaList *ca;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context drbg;
};

static pthread_once_t net_init_once = PTHREAD_ONCE_INIT;
static int net_init_status = -1;

static void NetHttp_InitOnce()
{
#ifdef _WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
	// TLS 1.3 and the random numbers below go through PSA
	net_init_status = psa_crypto_init();
}

static bool NetHttp_Init()
{
	pthread_once(&net_init_once, NetHttp_InitOnce);
	return net_init_status == PSA_SUCCESS;
}

int64_t NetHttp_Milliseconds()
{
#ifdef _WIN32
	return (int64_t)GetTickCount64();
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

static bool Net_Cancelled(const volatile int *cancel)
{
	return cancel && __atomic_load_n(cancel, __ATOMIC_ACQUIRE);
}

static int Net_LastError()
{
#ifdef _WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

static bool Net_WouldBlock(int err)
{
#ifdef _WIN32
	return err == WSAEWOULDBLOCK || err == WSAEINPROGRESS || err == WSAEINTR;
#else
	return err == EAGAIN || err == EWOULDBLOCK || err == EINPROGRESS || err == EINTR;
#endif
}

// 1 when the socket is ready, 0 when the time ran out, -1 on errors
static int Net_Wait(net_socket_t fd, bool write, int ms)
{
#ifdef _WIN32
	fd_set set, errors;
	struct timeval tv;
	FD_ZERO(&set);
	FD_ZERO(&errors);
	FD_SET(fd, &set);
	FD_SET(fd, &errors);
	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000;
	int r = select(0, write ? NULL : &set, write ? &set : NULL, &errors, &tv);
	if ( r < 0 )
		return -1;
	// a failed connect is reported in the error set
	return r > 0 ? 1 : 0;
#else
	struct pollfd p;
	p.fd = fd;
	p.events = write ? POLLOUT : POLLIN;
	p.revents = 0;
	int r = poll(&p, 1, ms);
	if ( r < 0 )
		return errno == EINTR ? 0 : -1;
	return r > 0 ? 1 : 0;
#endif
}

// waits in short slices until the socket is ready, the deadline passes or the request is cancelled
static bool Net_WaitDeadline(net_socket_t fd, bool write, int64_t deadline, const volatile int *cancel, std::string *error)
{
	for ( ;; )
	{
		if ( Net_Cancelled(cancel) )
		{
			*error = "Cancelled";
			return false;
		}

		int64_t left = deadline - NetHttp_Milliseconds();
		if ( left <= 0 )
		{
			*error = "Timeout";
			return false;
		}

		int r = Net_Wait(fd, write, left < NET_WAIT_SLICE ? (int)left : NET_WAIT_SLICE);
		if ( r > 0 )
			return true;
		if ( r < 0 )
		{
			*error = "Socket error";
			return false;
		}
	}
}

static void Net_SetNonBlocking(net_socket_t fd)
{
#ifdef _WIN32
	u_long on = 1;
	ioctlsocket(fd, FIONBIO, &on);
#else
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
#endif
	int one = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof(one));
}

// Loopback, private, shared (CGNAT), link-local, multicast and reserved
// addresses, IPv4-mapped ones included; checked after resolving, so a public
// name that resolves to one is refused as well
static bool Net_IsPrivateIPv4(const unsigned char *a)
{
	return a[0] == 0 || a[0] == 10 || a[0] == 127 || a[0] >= 224
		|| ( a[0] == 100 && ( a[1] & 0xc0 ) == 64 )
		|| ( a[0] == 169 && a[1] == 254 )
		|| ( a[0] == 172 && ( a[1] & 0xf0 ) == 16 )
		|| ( a[0] == 192 && a[1] == 168 );
}

static bool Net_IsPrivateAddress(const struct sockaddr *sa)
{
	if ( sa->sa_family == AF_INET )
		return Net_IsPrivateIPv4((const unsigned char *)&((const struct sockaddr_in *)sa)->sin_addr);

	if ( sa->sa_family != AF_INET6 )
		return true;

	const unsigned char *a = (const unsigned char *)&((const struct sockaddr_in6 *)sa)->sin6_addr;
	static const unsigned char mapped[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };
	static const unsigned char zero[15] = { 0 };

	if ( !memcmp(a, mapped, 12) )
		return Net_IsPrivateIPv4(a + 12);

	// :: and ::1
	if ( !memcmp(a, zero, 15) && a[15] <= 1 )
		return true;

	// fc00::/7 unique local, fe80::/10 link-local, fec0::/10 site-local, ff00::/8 multicast
	return ( a[0] & 0xfe ) == 0xfc || a[0] == 0xff || ( a[0] == 0xfe && ( a[1] & 0x80 ) );
}

static net_socket_t Net_ConnectTcp(const char *host, int port, bool allowPrivate, int64_t deadline, const volatile int *cancel, std::string *error)
{
	struct addrinfo hints, *list = NULL, *ai;
	char service[16];
	net_socket_t fd = NET_INVALID_SOCKET;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	snprintf(service, sizeof(service), "%d", port);

	// can't be interrupted, but it runs on a worker and a late result is thrown away
	if ( getaddrinfo(host, service, &hints, &list) != 0 || !list )
	{
		*error = "Cannot resolve host";
		return NET_INVALID_SOCKET;
	}

	*error = "Connection refused";
	for ( ai = list; ai; ai = ai->ai_next )
	{
		if ( !allowPrivate && Net_IsPrivateAddress(ai->ai_addr) )
		{
			*error = "Private address not allowed by sv_httpAllowPrivate";
			continue;
		}

		fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if ( fd == NET_INVALID_SOCKET )
			continue;

		Net_SetNonBlocking(fd);

		if ( connect(fd, ai->ai_addr, (int)ai->ai_addrlen) == 0 )
			break;

		// an address that doesn't answer (often a broken IPv6 route) leaves time for the next one
		int64_t limit = deadline;
		if ( ai->ai_next && NetHttp_Milliseconds() + NET_ADDRESS_TIMEOUT < deadline )
			limit = NetHttp_Milliseconds() + NET_ADDRESS_TIMEOUT;

		if ( Net_WouldBlock(Net_LastError()) && Net_WaitDeadline(fd, true, limit, cancel, error) )
		{
			int err = 0;
			socklen_t len = sizeof(err);
			if ( getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len) == 0 && err == 0 )
				break;
			*error = "Connection refused";
		}

		net_closesocket(fd);
		fd = NET_INVALID_SOCKET;

		if ( *error == "Cancelled" || NetHttp_Milliseconds() >= deadline )
			break;
	}

	freeaddrinfo(list);
	return fd;
}

static int Net_BioSend(void *ctx, const unsigned char *buf, size_t len)
{
	NetConn *c = (NetConn *)ctx;
	int n = send(c->fd, (const char *)buf, (int)len, NET_SEND_FLAGS);

	if ( n >= 0 )
		return n;
	if ( Net_WouldBlock(Net_LastError()) )
		return MBEDTLS_ERR_SSL_WANT_WRITE;
	return MBEDTLS_ERR_NET_SEND_FAILED;
}

static int Net_BioRecv(void *ctx, unsigned char *buf, size_t len)
{
	NetConn *c = (NetConn *)ctx;
	int n = recv(c->fd, (char *)buf, (int)len, 0);

	if ( n >= 0 )
		return n;
	if ( Net_WouldBlock(Net_LastError()) )
		return MBEDTLS_ERR_SSL_WANT_READ;
	return MBEDTLS_ERR_NET_RECV_FAILED;
}

static void Net_LoadSystemCAs(mbedtls_x509_crt *ca)
{
#ifdef _WIN32
	HCERTSTORE store = CertOpenSystemStoreA(0, "ROOT");
	if ( !store )
		return;

	PCCERT_CONTEXT cert = NULL;
	while ( (cert = CertEnumCertificatesInStore(store, cert)) != NULL )
	{
		if ( cert->dwCertEncodingType & X509_ASN_ENCODING )
			mbedtls_x509_crt_parse_der(ca, cert->pbCertEncoded, cert->cbCertEncoded);
	}
	CertCloseStore(store, 0);
#else
	static const char *bundles[] =
	{
		"/etc/ssl/certs/ca-certificates.crt",  // Debian, Ubuntu, Arch, Alpine
		"/etc/pki/tls/certs/ca-bundle.crt",    // Fedora, RHEL, CentOS
	};

	for ( size_t i = 0; i < sizeof(bundles) / sizeof(bundles[0]); i++ )
	{
		// a positive result means some certificates of the bundle were skipped
		if ( mbedtls_x509_crt_parse_file(ca, bundles[i]) >= 0 )
			return;
	}
#endif
}

static void Net_ReleaseCa(NetCaList *ca)
{
	if ( !ca )
		return;

	pthread_mutex_lock(&net_ca_mutex);
	bool last = --ca->refs == 0;
	pthread_mutex_unlock(&net_ca_mutex);

	if ( last )
	{
		mbedtls_x509_crt_free(&ca->crt);
		delete ca;
	}
}

// the list for file, parsed again when sv_httpCaFile changes or after an hour
static NetCaList *Net_AcquireCa(const std::string &file, std::string *error)
{
	int64_t now = NetHttp_Milliseconds();
	NetCaList *ca;

	pthread_mutex_lock(&net_ca_mutex);
	ca = net_ca;
	if ( ca && ca->file == file && now - ca->loaded < NET_CA_RELOAD )
	{
		ca->refs++;
		pthread_mutex_unlock(&net_ca_mutex);
		return ca;
	}
	pthread_mutex_unlock(&net_ca_mutex);

	// parsed outside the lock; when two workers both parse, the later list is kept
	ca = new NetCaList;
	mbedtls_x509_crt_init(&ca->crt);
	ca->file = file;
	ca->loaded = now;
	ca->refs = 1;

	if ( !file.empty() )
	{
		if ( mbedtls_x509_crt_parse_file(&ca->crt, file.c_str()) < 0 )
		{
			*error = "TLS: cannot read the CA file in sv_httpCaFile";
			Net_ReleaseCa(ca);
			return NULL;
		}
	}
	else
		Net_LoadSystemCAs(&ca->crt);

	// an empty chain has version 0
	if ( ca->crt.version == 0 )
	{
		*error = "TLS: no CA certificates found, set sv_httpCaFile";
		Net_ReleaseCa(ca);
		return NULL;
	}

	pthread_mutex_lock(&net_ca_mutex);
	NetCaList *old = net_ca;
	net_ca = ca;
	ca->refs++;
	pthread_mutex_unlock(&net_ca_mutex);

	Net_ReleaseCa(old);
	return ca;
}

static void Net_Close(NetConn *c)
{
	if ( !c )
		return;

	if ( c->tls )
	{
		mbedtls_ssl_free(&c->ssl);
		mbedtls_ssl_config_free(&c->conf);
		mbedtls_ctr_drbg_free(&c->drbg);
		mbedtls_entropy_free(&c->entropy);
	}
	// after the configuration that points to it
	Net_ReleaseCa(c->ca);
	if ( c->fd != NET_INVALID_SOCKET )
		net_closesocket(c->fd);
	delete c;
}

static bool Net_StartTls(NetConn *c, const NetUrl *url, const NetTlsOptions *tls, int64_t deadline, std::string *error)
{
	char msg[512];
	int r;

	c->tls = true;
	mbedtls_ssl_init(&c->ssl);
	mbedtls_ssl_config_init(&c->conf);
	mbedtls_entropy_init(&c->entropy);
	mbedtls_ctr_drbg_init(&c->drbg);

	if ( mbedtls_ctr_drbg_seed(&c->drbg, mbedtls_entropy_func, &c->entropy, (const unsigned char *)"cod2rev", 7) != 0 )
	{
		*error = "TLS: no random source";
		return false;
	}

	if ( mbedtls_ssl_config_defaults(&c->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0 )
	{
		*error = "TLS: setup failed";
		return false;
	}

	if ( tls->allowInsecure )
		mbedtls_ssl_conf_authmode(&c->conf, MBEDTLS_SSL_VERIFY_NONE);
	else
	{
		c->ca = Net_AcquireCa(tls->caFile, error);
		if ( !c->ca )
			return false;

		mbedtls_ssl_conf_authmode(&c->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
		mbedtls_ssl_conf_ca_chain(&c->conf, &c->ca->crt, NULL);
	}

	mbedtls_ssl_conf_rng(&c->conf, mbedtls_ctr_drbg_random, &c->drbg);

	if ( mbedtls_ssl_setup(&c->ssl, &c->conf) != 0 || mbedtls_ssl_set_hostname(&c->ssl, url->host.c_str()) != 0 )
	{
		*error = "TLS: setup failed";
		return false;
	}

	mbedtls_ssl_set_bio(&c->ssl, c, Net_BioSend, Net_BioRecv, NULL);

	while ( (r = mbedtls_ssl_handshake(&c->ssl)) != 0 )
	{
		if ( r == MBEDTLS_ERR_SSL_WANT_READ || r == MBEDTLS_ERR_SSL_WANT_WRITE )
		{
			if ( !Net_WaitDeadline(c->fd, r == MBEDTLS_ERR_SSL_WANT_WRITE, deadline, c->cancel, error) )
			{
				if ( *error == "Timeout" )
					*error = "TLS handshake timeout";
				return false;
			}
			continue;
		}

		uint32_t flags = mbedtls_ssl_get_verify_result(&c->ssl);
		if ( r == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED && flags != 0 && flags != (uint32_t)-1 )
		{
			char info[256];
			mbedtls_x509_crt_verify_info(info, sizeof(info), "", flags);
			// one line: the flags are listed one per line
			for ( char *p = info; *p; p++ )
			{
				if ( *p == '\n' )
					*p = ( p[1] ? ';' : '\0' );
			}
			snprintf(msg, sizeof(msg), "TLS: certificate verification failed: %s", info);
		}
		else
			snprintf(msg, sizeof(msg), "TLS handshake failed (-0x%04X)", (unsigned int)-r);
		*error = msg;
		return false;
	}

	return true;
}

static NetConn *Net_Open(const NetUrl *url, const NetTlsOptions *tls, int64_t deadline, const volatile int *cancel, std::string *error)
{
	if ( !NetHttp_Init() )
	{
		*error = "TLS: crypto init failed";
		return NULL;
	}

	NetConn *c = new NetConn;
	c->fd = NET_INVALID_SOCKET;
	c->tls = false;
	c->ca = NULL;
	c->cancel = cancel;

	// the connection and the TLS handshake each have at most NET_CONNECT_TIMEOUT
	int64_t limit = NetHttp_Milliseconds() + NET_CONNECT_TIMEOUT;
	c->fd = Net_ConnectTcp(url->host.c_str(), url->port, tls->allowPrivate, deadline < limit ? deadline : limit, cancel, error);
	if ( c->fd == NET_INVALID_SOCKET )
	{
		if ( *error == "Timeout" )
			*error = "Connect timeout";
		Net_Close(c);
		return NULL;
	}

	error->clear();

	limit = NetHttp_Milliseconds() + NET_CONNECT_TIMEOUT;
	if ( url->tls && !Net_StartTls(c, url, tls, deadline < limit ? deadline : limit, error) )
	{
		Net_Close(c);
		return NULL;
	}

	return c;
}

static bool Net_Write(NetConn *c, const char *data, size_t len, int64_t deadline, std::string *error)
{
	while ( len > 0 )
	{
		int n;
		bool wantWrite = true;

		if ( c->tls )
		{
			n = mbedtls_ssl_write(&c->ssl, (const unsigned char *)data, len);
			if ( n == MBEDTLS_ERR_SSL_WANT_READ )
				wantWrite = false;
			else if ( n < 0 && n != MBEDTLS_ERR_SSL_WANT_WRITE )
			{
				*error = "Connection closed unexpectedly";
				return false;
			}
		}
		else
		{
			n = send(c->fd, data, (int)len, NET_SEND_FLAGS);
			if ( n < 0 && !Net_WouldBlock(Net_LastError()) )
			{
				*error = "Connection closed unexpectedly";
				return false;
			}
		}

		if ( n > 0 )
		{
			data += n;
			len -= n;
			continue;
		}

		if ( !Net_WaitDeadline(c->fd, wantWrite, deadline, c->cancel, error) )
			return false;
	}

	return true;
}

// > 0 bytes read, 0 at the end of the stream, NET_AGAIN when nothing came within waitMs, -1 on errors
static int Net_Read(NetConn *c, char *buf, size_t len, int waitMs, std::string *error)
{
	int64_t end = NetHttp_Milliseconds() + waitMs;

	for ( ;; )
	{
		int n;
		bool wantWrite = false;

		if ( Net_Cancelled(c->cancel) )
		{
			*error = "Cancelled";
			return -1;
		}

		if ( c->tls )
		{
			n = mbedtls_ssl_read(&c->ssl, (unsigned char *)buf, len);
			if ( n == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY )
				return 0;
			if ( n == MBEDTLS_ERR_SSL_WANT_WRITE )
				wantWrite = true;
			else if ( n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET )
				n = NET_AGAIN;
			else if ( n < 0 )
			{
				*error = ( n == MBEDTLS_ERR_SSL_CONN_EOF || n == MBEDTLS_ERR_NET_RECV_FAILED ) ? "Connection closed unexpectedly" : "TLS error";
				return -1;
			}
			if ( wantWrite )
				n = NET_AGAIN;
		}
		else
		{
			n = recv(c->fd, buf, (int)len, 0);
			if ( n < 0 )
			{
				if ( !Net_WouldBlock(Net_LastError()) )
				{
					*error = "Connection closed unexpectedly";
					return -1;
				}
				n = NET_AGAIN;
			}
		}

		if ( n != NET_AGAIN )
			return n;

		int64_t left = end - NetHttp_Milliseconds();
		if ( left <= 0 )
			return NET_AGAIN;

		if ( Net_Wait(c->fd, wantWrite, left < NET_WAIT_SLICE ? (int)left : NET_WAIT_SLICE) < 0 )
		{
			*error = "Socket error";
			return -1;
		}
	}
}

static bool Net_IsHostChar(int c)
{
	return isalnum(c) || c == '-' || c == '.' || c == '_';
}

bool NetHttp_ParseUrl(const char *text, bool websocket, NetUrl *url)
{
	const char *p;
	const char *end;

	url->tls = false;
	url->websocket = websocket;
	url->host.clear();
	url->port = 0;
	url->target.clear();
	url->userinfo.clear();

	if ( !text )
		return false;

	if ( websocket )
	{
		if ( !strncasecmp(text, "ws://", 5) )
			p = text + 5;
		else if ( !strncasecmp(text, "wss://", 6) )
		{
			p = text + 6;
			url->tls = true;
		}
		else
			return false;
	}
	else
	{
		if ( !strncasecmp(text, "http://", 7) )
			p = text + 7;
		else if ( !strncasecmp(text, "https://", 8) )
		{
			p = text + 8;
			url->tls = true;
		}
		else
			return false;
	}

	// no spaces or control characters anywhere: they would end up in the request line
	for ( const char *q = text; *q; q++ )
	{
		if ( (unsigned char)*q <= 0x20 || *q == 0x7f )
			return false;
	}

	end = p + strcspn(p, "/?#");

	// user:password@
	const char *at = (const char *)memchr(p, '@', end - p);
	if ( at )
	{
		url->userinfo.assign(p, at - p);
		p = at + 1;
	}

	const char *portStart = NULL;
	if ( *p == '[' )
	{
		const char *close = (const char *)memchr(p, ']', end - p);
		if ( !close || close == p + 1 )
			return false;
		for ( const char *q = p + 1; q < close; q++ )
		{
			if ( !isxdigit((unsigned char)*q) && *q != ':' && *q != '.' )
				return false;
		}
		url->host.assign(p + 1, close - p - 1);
		if ( close + 1 < end )
		{
			if ( close[1] != ':' )
				return false;
			portStart = close + 2;
		}
	}
	else
	{
		const char *colon = (const char *)memchr(p, ':', end - p);
		const char *hostEnd = colon ? colon : end;
		if ( hostEnd == p )
			return false;
		for ( const char *q = p; q < hostEnd; q++ )
		{
			if ( !Net_IsHostChar((unsigned char)*q) )
				return false;
		}
		url->host.assign(p, hostEnd - p);
		if ( colon )
			portStart = colon + 1;
	}

	if ( url->host.size() > 253 )
		return false;

	if ( portStart )
	{
		int port = 0;
		if ( portStart == end )
			return false;
		for ( const char *q = portStart; q < end; q++ )
		{
			if ( !isdigit((unsigned char)*q) )
				return false;
			port = port * 10 + (*q - '0');
			if ( port > 65535 )
				return false;
		}
		if ( port == 0 )
			return false;
		url->port = port;
	}
	else
		url->port = url->tls ? 443 : 80;

	// the fragment stays on the client
	const char *fragment = strchr(end, '#');
	url->target.assign(end, fragment ? fragment - end : strlen(end));
	if ( url->target.empty() || url->target[0] != '/' )
		url->target.insert(0, "/");

	return true;
}

// for the server log: no credentials and no query string, which often carries keys
std::string NetHttp_LogUrl(const NetUrl *url)
{
	std::string s = url->websocket ? ( url->tls ? "wss://" : "ws://" ) : ( url->tls ? "https://" : "http://" );

	if ( url->host.find(':') != std::string::npos )
		s += "[" + url->host + "]";
	else
		s += url->host;

	if ( url->port != ( url->tls ? 443 : 80 ) )
	{
		char port[16];
		snprintf(port, sizeof(port), ":%d", url->port);
		s += port;
	}

	s += url->target.substr(0, url->target.find('?'));
	return s;
}

// allowed: host names separated by spaces or commas, "*.example.com" for the subdomains
bool NetHttp_HostAllowed(const char *host, const char *allowed)
{
	const char *p = allowed;
	size_t hostLen = strlen(host);

	if ( !p || !*p )
		return true;

	while ( *p )
	{
		while ( *p == ' ' || *p == ',' || *p == '\t' )
			p++;
		size_t len = strcspn(p, " ,\t");
		if ( !len )
			break;

		if ( len > 2 && p[0] == '*' && p[1] == '.' )
		{
			// the suffix, dot included
			size_t suffix = len - 1;
			if ( hostLen > suffix && !strncasecmp(host + hostLen - suffix, p + 1, suffix) )
				return true;
		}
		else if ( len == hostLen && !strncasecmp(host, p, len) )
			return true;

		p += len;
	}

	return false;
}

bool NetHttp_ValidMethod(const char *method)
{
	size_t len = strlen(method);

	if ( len > 16 )
		return false;
	for ( size_t i = 0; i < len; i++ )
	{
		if ( !isupper((unsigned char)method[i]) )
			return false;
	}
	return true;
}

static std::string Net_HostHeader(const NetUrl *url)
{
	std::string s = url->host.find(':') != std::string::npos ? "[" + url->host + "]" : url->host;

	if ( url->port != ( url->tls ? 443 : 80 ) )
	{
		char port[16];
		snprintf(port, sizeof(port), ":%d", url->port);
		s += port;
	}
	return s;
}

static bool Net_HeaderIs(const std::string &line, const char *name)
{
	size_t len = strlen(name);
	return line.size() > len && line[len] == ':' && !strncasecmp(line.c_str(), name, len);
}

// the script's headers, one per line; the ones that frame the request are ours
static void Net_AppendHeaders(std::string *out, const char *headers, const char **reserved, bool *hasAuthorization)
{
	const char *p = headers;

	while ( p && *p )
	{
		const char *eol = strstr(p, "\r\n");
		std::string line = eol ? std::string(p, eol - p) : std::string(p);
		p = eol ? eol + 2 : p + line.size();

		// a lone CR or LF would start another header or the body
		if ( line.empty() || line.find_first_of("\r\n") != std::string::npos )
			continue;
		size_t colon = line.find(':');
		if ( colon == 0 || colon == std::string::npos || line.find_first_of(" \t") < colon )
			continue;

		bool skip = false;
		for ( int i = 0; reserved[i]; i++ )
		{
			if ( Net_HeaderIs(line, reserved[i]) )
				skip = true;
		}
		if ( skip )
			continue;

		if ( Net_HeaderIs(line, "Authorization") )
			*hasAuthorization = true;

		*out += line;
		*out += "\r\n";
	}
}

static void Net_AppendAuthorization(std::string *out, const NetUrl *url)
{
	unsigned char encoded[1024];
	size_t len = 0;

	if ( url->userinfo.empty() )
		return;
	if ( mbedtls_base64_encode(encoded, sizeof(encoded) - 1, &len, (const unsigned char *)url->userinfo.data(), url->userinfo.size()) != 0 )
		return;
	encoded[len] = '\0';
	*out += "Authorization: Basic ";
	*out += (const char *)encoded;
	*out += "\r\n";
}

// Buffered reader for the response: fills buf from the connection until a
// condition holds, with the request deadline and an idle limit.
struct NetReader
{
	NetConn *conn;
	std::string buf;
	int64_t deadline;   // 0 for none
	bool eof;
};

// reads some more bytes; false at the end of the stream (error empty) or on errors
static bool Net_ReadMore(NetReader *r, std::string *error)
{
	char chunk[8192];
	int64_t idleEnd = NetHttp_Milliseconds() + NET_IDLE_TIMEOUT;

	if ( r->eof )
		return false;

	for ( ;; )
	{
		int64_t now = NetHttp_Milliseconds();
		int64_t end = idleEnd;
		if ( r->deadline && r->deadline < end )
			end = r->deadline;
		if ( now >= end )
		{
			*error = "Timeout";
			return false;
		}

		int n = Net_Read(r->conn, chunk, sizeof(chunk), (int)(end - now), error);
		if ( n > 0 )
		{
			r->buf.append(chunk, n);
			return true;
		}
		if ( n == 0 )
		{
			r->eof = true;
			return false;
		}
		if ( n != NET_AGAIN )
			return false;
	}
}

static bool Net_ContainsNoCase(const char *s, const char *word)
{
	size_t len = strlen(word);

	for ( ; *s; s++ )
	{
		if ( !strncasecmp(s, word, len) )
			return true;
	}
	return false;
}

static void Net_ToLower(std::string *s)
{
	for ( size_t i = 0; i < s->size(); i++ )
		(*s)[i] = tolower((unsigned char)(*s)[i]);
}

static std::string Net_Trim(const std::string &s)
{
	size_t a = s.find_first_not_of(" \t");
	if ( a == std::string::npos )
		return "";
	size_t b = s.find_last_not_of(" \t");
	return s.substr(a, b - a + 1);
}

// Reads a header block ending with an empty line and parses it. Status line
// in *status, headers with lowercase names (repeated ones joined with ", ").
static bool Net_ReadHeaderBlock(NetReader *r, size_t limit, int *status, std::vector<std::pair<std::string, std::string> > *headers, std::string *error)
{
	size_t end;

	while ( (end = r->buf.find("\r\n\r\n")) == std::string::npos )
	{
		if ( r->buf.size() > limit )
		{
			*error = "Response headers too large";
			return false;
		}
		if ( !Net_ReadMore(r, error) )
		{
			if ( error->empty() )
				*error = "Connection closed unexpectedly";
			return false;
		}
	}
	if ( end > limit )
	{
		*error = "Response headers too large";
		return false;
	}

	std::string block = r->buf.substr(0, end + 2);
	r->buf.erase(0, end + 4);

	// the script would cut a name at a NUL, and two names that differ after it would be one array key
	if ( block.find('\0') != std::string::npos )
	{
		*error = "Invalid response";
		return false;
	}

	size_t eol = block.find("\r\n");
	std::string statusLine = block.substr(0, eol);
	int major, minor, code;
	if ( sscanf(statusLine.c_str(), "HTTP/%d.%d %d", &major, &minor, &code) != 3 || major != 1 || code < 100 || code > 999 )
	{
		*error = "Invalid response";
		return false;
	}
	*status = code;

	headers->clear();
	size_t pos = eol + 2;
	int lines = 0;
	size_t last = std::string::npos;
	while ( pos < block.size() )
	{
		size_t next = block.find("\r\n", pos);
		std::string line = block.substr(pos, next - pos);
		pos = next + 2;

		if ( ++lines > NET_MAX_HEADERS )
		{
			*error = "Response headers too large";
			return false;
		}

		// obsolete line folding continues the previous value
		if ( !line.empty() && ( line[0] == ' ' || line[0] == '\t' ) )
		{
			if ( last != std::string::npos )
				(*headers)[last].second += " " + Net_Trim(line);
			continue;
		}

		size_t colon = line.find(':');
		if ( colon == std::string::npos || colon == 0 )
			continue;
		std::string name = line.substr(0, colon);
		for ( size_t i = 0; i < name.size(); i++ )
		{
			if ( (unsigned char)name[i] <= 0x20 || name[i] == 0x7f )
			{
				*error = "Invalid response";
				return false;
			}
		}
		std::string value = Net_Trim(line.substr(colon + 1));
		Net_ToLower(&name);

		last = headers->size();
		for ( size_t i = 0; i < headers->size(); i++ )
		{
			if ( (*headers)[i].first == name )
			{
				(*headers)[i].second += ", " + value;
				last = i;
				break;
			}
		}
		if ( last == headers->size() )
			headers->push_back(std::make_pair(name, value));
	}

	return true;
}

static const std::string *Net_FindHeader(const std::vector<std::pair<std::string, std::string> > *headers, const char *name)
{
	for ( size_t i = 0; i < headers->size(); i++ )
	{
		if ( (*headers)[i].first == name )
			return &(*headers)[i].second;
	}
	return NULL;
}

static bool Net_TooLarge(std::string *error, size_t size)
{
	char msg[96];
	snprintf(msg, sizeof(msg), "Response body too large: %u bytes", (unsigned int)size);
	*error = msg;
	return false;
}

// digits only; repeated headers, joined with commas, must agree
static bool Net_ParseContentLength(const std::string &text, uint64_t *size)
{
	size_t pos = 0;
	bool have = false;

	for ( ;; )
	{
		size_t comma = text.find(',', pos);
		std::string part = Net_Trim(text.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos));
		uint64_t value = 0;

		if ( part.empty() )
			return false;
		for ( size_t i = 0; i < part.size(); i++ )
		{
			if ( !isdigit((unsigned char)part[i]) )
				return false;
			unsigned int digit = part[i] - '0';
			if ( value > ( (uint64_t)-1 - digit ) / 10 )
				return false;
			value = value * 10 + digit;
		}

		if ( have && value != *size )
			return false;
		*size = value;
		have = true;

		if ( comma == std::string::npos )
			return true;
		pos = comma + 1;
	}
}

static bool Net_ReadChunked(NetReader *r, std::string *body, std::string *error)
{
	for ( ;; )
	{
		size_t eol;
		while ( (eol = r->buf.find("\r\n")) == std::string::npos )
		{
			if ( r->buf.size() > 1024 || !Net_ReadMore(r, error) )
			{
				if ( error->empty() )
					*error = "Invalid chunked response";
				return false;
			}
		}

		char *end;
		unsigned long size = strtoul(r->buf.c_str(), &end, 16);
		if ( end == r->buf.c_str() )
		{
			*error = "Invalid chunked response";
			return false;
		}
		r->buf.erase(0, eol + 2);

		if ( size == 0 )
		{
			// trailers up to the empty line
			for ( ;; )
			{
				while ( (eol = r->buf.find("\r\n")) == std::string::npos )
				{
					if ( r->buf.size() > 8192 || !Net_ReadMore(r, error) )
					{
						// some servers close right after the last chunk
						error->clear();
						return true;
					}
				}
				r->buf.erase(0, eol + 2);
				if ( eol == 0 )
					return true;
			}
		}

		if ( size > NET_HTTP_MAX_STRING || body->size() + size > NET_HTTP_MAX_STRING )
			return Net_TooLarge(error, body->size() + size);

		while ( r->buf.size() < size + 2 )
		{
			if ( !Net_ReadMore(r, error) )
			{
				if ( error->empty() )
					*error = "Connection closed unexpectedly";
				return false;
			}
		}
		body->append(r->buf, 0, size);
		r->buf.erase(0, size + 2);
	}
}

bool NetHttp_Fetch(const NetHttpRequest *req, const volatile int *cancel, NetHttpResponse *res)
{
	static const char *reserved[] = { "Host", "Content-Length", "Transfer-Encoding", "Connection", NULL };
	int64_t deadline = req->timeoutMs > 0 ? NetHttp_Milliseconds() + req->timeoutMs : 0;
	bool hasAuthorization = false;
	std::string request;

	res->status = 0;
	res->headers.clear();
	res->body.clear();
	res->error.clear();

	request = req->method + " " + req->url.target + " HTTP/1.1\r\n";
	request += "Host: " + Net_HostHeader(&req->url) + "\r\n";
	request += "User-Agent: CoD2rev\r\n";
	request += "Connection: close\r\n";
	if ( !req->body.empty() || req->method == "POST" || req->method == "PUT" || req->method == "PATCH" )
	{
		char len[48];
		snprintf(len, sizeof(len), "Content-Length: %u\r\n", (unsigned int)req->body.size());
		request += len;
	}
	Net_AppendHeaders(&request, req->headers.c_str(), reserved, &hasAuthorization);
	if ( !hasAuthorization )
		Net_AppendAuthorization(&request, &req->url);
	request += "\r\n";
	request += req->body;

	// with no request timeout the connection still has its own limits
	NetConn *conn = Net_Open(&req->url, &req->tls, deadline ? deadline : NetHttp_Milliseconds() + NET_CONNECT_TIMEOUT * 2, cancel, &res->error);
	if ( !conn )
		return false;

	bool ok = Net_Write(conn, request.data(), request.size(), deadline ? deadline : NetHttp_Milliseconds() + NET_IDLE_TIMEOUT, &res->error);

	NetReader reader;
	reader.conn = conn;
	reader.deadline = deadline;
	reader.eof = false;

	// skip interim responses such as 100 Continue, but not an endless stream of them
	for ( int interim = 0; ok; interim++ )
	{
		if ( interim > NET_MAX_INTERIM )
		{
			res->error = "Too many interim responses";
			ok = false;
			break;
		}
		ok = Net_ReadHeaderBlock(&reader, NET_HTTP_MAX_STRING, &res->status, &res->headers, &res->error);
		if ( !ok || res->status >= 200 || res->status == 101 )
			break;
	}

	if ( ok && req->method != "HEAD" && res->status != 204 && res->status != 304 && !(res->status >= 100 && res->status < 200) )
	{
		const std::string *encoding = Net_FindHeader(&res->headers, "transfer-encoding");
		const std::string *length = Net_FindHeader(&res->headers, "content-length");

		if ( encoding && Net_ContainsNoCase(encoding->c_str(), "chunked") )
			ok = Net_ReadChunked(&reader, &res->body, &res->error);
		else if ( length )
		{
			uint64_t size;
			if ( !Net_ParseContentLength(*length, &size) )
			{
				res->error = "Invalid response";
				ok = false;
			}
			else if ( size > NET_HTTP_MAX_STRING )
				ok = Net_TooLarge(&res->error, (size_t)( size > 4000000000u ? 4000000000u : size ));
			else
			{
				while ( ok && reader.buf.size() < (size_t)size )
				{
					ok = Net_ReadMore(&reader, &res->error);
					if ( !ok && res->error.empty() )
						res->error = "Connection closed unexpectedly";
				}
				if ( ok )
					res->body.assign(reader.buf, 0, (size_t)size);
			}
		}
		else
		{
			// until the server closes the connection
			while ( ok )
			{
				if ( reader.buf.size() > NET_HTTP_MAX_STRING )
					ok = Net_TooLarge(&res->error, reader.buf.size());
				else if ( !Net_ReadMore(&reader, &res->error) )
				{
					ok = res->error.empty();
					break;
				}
			}
			if ( ok )
				res->body = reader.buf;
		}
	}

	Net_Close(conn);
	if ( !ok && res->error.empty() )
		res->error = "Connection closed unexpectedly";
	return ok;
}

static bool Net_Random(unsigned char *out, size_t len)
{
	return NetHttp_Init() && psa_generate_random(out, len) == PSA_SUCCESS;
}

bool NetWs_Connect(NetWebSocket *ws, const NetUrl *url, const char *headers, const NetTlsOptions *tls,
                   int timeoutMs, const volatile int *cancel, std::string *error)
{
	static const char *reserved[] = { "Host", "Upgrade", "Connection", "Content-Length", "Transfer-Encoding",
		"Sec-WebSocket-Key", "Sec-WebSocket-Version", "Sec-WebSocket-Extensions", "Sec-WebSocket-Accept", NULL };
	static const char *guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	int64_t deadline = NetHttp_Milliseconds() + timeoutMs;
	unsigned char nonce[16], digest[20], encoded[64];
	char key[32];
	size_t len;
	bool hasAuthorization = false;

	ws->conn = NULL;
	ws->rx.clear();

	if ( !Net_Random(nonce, sizeof(nonce)) || mbedtls_base64_encode((unsigned char *)key, sizeof(key), &len, nonce, sizeof(nonce)) != 0 )
	{
		*error = "No random source";
		return false;
	}

	std::string request = "GET " + url->target + " HTTP/1.1\r\n";
	request += "Host: " + Net_HostHeader(url) + "\r\n";
	request += "User-Agent: CoD2rev\r\n";
	request += "Upgrade: websocket\r\n";
	request += "Connection: Upgrade\r\n";
	request += std::string("Sec-WebSocket-Key: ") + key + "\r\n";
	request += "Sec-WebSocket-Version: 13\r\n";
	Net_AppendHeaders(&request, headers, reserved, &hasAuthorization);
	if ( !hasAuthorization )
		Net_AppendAuthorization(&request, url);
	request += "\r\n";

	NetConn *conn = Net_Open(url, tls, deadline, cancel, error);
	if ( !conn )
		return false;

	NetReader reader;
	reader.conn = conn;
	reader.deadline = deadline;
	reader.eof = false;

	int status = 0;
	std::vector<std::pair<std::string, std::string> > response;
	if ( !Net_Write(conn, request.data(), request.size(), deadline, error)
	        || !Net_ReadHeaderBlock(&reader, NET_WS_MAX_HANDSHAKE, &status, &response, error) )
	{
		if ( *error == "Timeout" )
			*error = "Connect timeout";
		Net_Close(conn);
		return false;
	}

	std::string accept = std::string(key) + guid;
	mbedtls_sha1((const unsigned char *)accept.data(), accept.size(), digest);
	mbedtls_base64_encode(encoded, sizeof(encoded), &len, digest, sizeof(digest));

	const std::string *upgrade = Net_FindHeader(&response, "upgrade");
	const std::string *connection = Net_FindHeader(&response, "connection");
	const std::string *serverAccept = Net_FindHeader(&response, "sec-websocket-accept");

	if ( status != 101 )
	{
		char msg[64];
		snprintf(msg, sizeof(msg), "Handshake failed: HTTP status %d", status);
		*error = msg;
	}
	else if ( !upgrade || strcasecmp(upgrade->c_str(), "websocket") || !connection || !Net_ContainsNoCase(connection->c_str(), "upgrade") )
		*error = "Handshake failed: not a WebSocket upgrade";
	else if ( !serverAccept || *serverAccept != std::string((const char *)encoded, len) )
		*error = "Handshake failed: wrong Sec-WebSocket-Accept";
	else if ( Net_FindHeader(&response, "sec-websocket-extensions") )
		*error = "Handshake failed: unrequested extension";
	else
	{
		// from here on the session stops itself, with a close frame
		conn->cancel = NULL;
		ws->conn = conn;
		ws->rx = reader.buf;
		return true;
	}

	Net_Close(conn);
	return false;
}

// 1 with a frame, 0 when more bytes are needed, -1 on protocol errors
static int Net_ParseFrame(std::string *rx, NetWsFrame *frame, std::string *error)
{
	const unsigned char *p = (const unsigned char *)rx->data();
	size_t have = rx->size();
	size_t header = 2;
	uint64_t len;

	if ( have < 2 )
		return 0;

	if ( p[0] & 0x70 )
	{
		*error = "Protocol error: reserved bits set";
		return -1;
	}
	if ( p[1] & 0x80 )
	{
		*error = "Protocol error: masked frame from the server";
		return -1;
	}

	len = p[1] & 0x7f;
	if ( len == 126 )
	{
		header = 4;
		if ( have < header )
			return 0;
		len = ((uint64_t)p[2] << 8) | p[3];
	}
	else if ( len == 127 )
	{
		header = 10;
		if ( have < header )
			return 0;
		len = 0;
		for ( int i = 2; i < 10; i++ )
			len = (len << 8) | p[i];
	}

	frame->fin = ( p[0] & 0x80 ) != 0;
	frame->opcode = p[0] & 0x0f;

	if ( frame->opcode >= NET_WS_CLOSE && ( len > 125 || !frame->fin ) )
	{
		*error = "Protocol error: invalid control frame";
		return -1;
	}
	if ( len > NET_WS_MAX_FRAME )
	{
		*error = "Frame too large";
		return -1;
	}
	if ( have < header + len )
		return 0;

	frame->payload.assign(*rx, header, (size_t)len);
	rx->erase(0, header + (size_t)len);
	return 1;
}

int NetWs_ReadFrame(NetWebSocket *ws, NetWsFrame *frame, int waitMs, std::string *error)
{
	char chunk[8192];
	int64_t end = NetHttp_Milliseconds() + waitMs;

	for ( ;; )
	{
		int r = Net_ParseFrame(&ws->rx, frame, error);
		if ( r != 0 )
			return r;

		int64_t left = end - NetHttp_Milliseconds();
		int n = Net_Read(ws->conn, chunk, sizeof(chunk), left > 0 ? (int)left : 0, error);
		if ( n > 0 )
		{
			ws->rx.append(chunk, n);
			continue;
		}
		if ( n == NET_AGAIN )
			return 0;
		if ( n == 0 )
			*error = "";
		return -1;
	}
}

bool NetWs_SendFrame(NetWebSocket *ws, int opcode, const char *data, size_t len, std::string *error)
{
	unsigned char header[14];
	size_t headerLen;
	unsigned char mask[4];

	if ( !Net_Random(mask, sizeof(mask)) )
	{
		*error = "No random source";
		return false;
	}

	header[0] = 0x80 | opcode;
	if ( len < 126 )
	{
		header[1] = 0x80 | (unsigned char)len;
		headerLen = 2;
	}
	else if ( len <= 0xffff )
	{
		header[1] = 0x80 | 126;
		header[2] = (unsigned char)(len >> 8);
		header[3] = (unsigned char)len;
		headerLen = 4;
	}
	else
	{
		header[1] = 0x80 | 127;
		for ( int i = 0; i < 8; i++ )
			header[2 + i] = (unsigned char)((uint64_t)len >> (56 - 8 * i));
		headerLen = 10;
	}
	memcpy(header + headerLen, mask, 4);
	headerLen += 4;

	// client frames are always masked
	std::string out((const char *)header, headerLen);
	out.append(data, len);
	for ( size_t i = 0; i < len; i++ )
		out[headerLen + i] ^= mask[i & 3];

	return Net_Write(ws->conn, out.data(), out.size(), NetHttp_Milliseconds() + NET_CONNECT_TIMEOUT, error);
}

void NetWs_Close(NetWebSocket *ws)
{
	if ( ws->conn && ws->conn->tls )
		mbedtls_ssl_close_notify(&ws->conn->ssl);
	Net_Close(ws->conn);
	ws->conn = NULL;
	ws->rx.clear();
}

#endif
