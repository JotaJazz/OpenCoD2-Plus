// the STL before the engine headers
#include "net_http.hpp"
#include <algorithm>
#include <exception>
#include <string>
#include <vector>

#include "gsc.hpp"
#include "plusid.hpp"
#include "mbedtls/base64.h"
#include "yyjson/yyjson.h"

#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

#ifdef LIBCOD

// Shared ban lists (docs/architecture/plus-id.md): sv_banLists is
// "url|pubhex[;url|pubhex...]". Each list is a JSON file and its <url>.sig,
// the base64 of an ECDSA P-256 r||s over SHA-256 of the exact bytes, signed by
// the list's maintainer. A thread per list fetches both every
// sv_banListRefresh minutes, checks the signature and parses; the main thread
// takes the result in PlusIdLists_Frame, keeps it in plusid_lists/ and drops
// the players it bans. A version that fails in any way keeps the last good one.

#if LIBCOD_COMPILE_HTTP == 1

#include "gsc_http.hpp"

#define PLUSID_MAX_LISTS 8
#define PLUSID_LIST_BYTES ( 4 * 1024 * 1024 )
#define PLUSID_LIST_ENTRIES 100000
#define PLUSID_SIG_BYTES 1024
#define PLUSID_LIST_TIMEOUT 60000

static dvar_t *sv_banLists;
static dvar_t *sv_banListRefresh;

// what a refusal needs of an entry, a third of a PlusIdBan: a full list
// holds 100 000 of them, and a shared list never has guid or ip
struct PlusIdListBan
{
	char id[22];
	char reason[128];
	long long expires;
};

// a version whose signature and format were checked
struct PlusIdListVersion
{
	std::string json, sig;      // only until cached
	unsigned char hash[32];     // SHA-256 of json
	char name[64];
	char appeal[256];
	long long updated;
	std::vector<PlusIdListBan> bans; // sorted by id
};

// a fetch on its own thread; the thread and the main thread each hold a reference
struct PlusIdListJob
{
	// set before the thread starts, then only read
	NetHttpRequest list, sig;
	unsigned char pk[33];
	bool haveHash;
	unsigned char hash[32];     // of the copy in use: the same bytes are not parsed again
	volatile int cancel;
	// lists_mutex
	int refs;
	bool done;
	// the thread's until done is set, then the main thread's
	bool ok;
	bool unchanged;
	PlusIdListVersion version;
	std::string error;
};

// main thread
struct PlusIdList
{
	std::string url;
	unsigned char pk[33];
	char cache[MAX_OSPATH * 2]; // the cached files' path without .json or .sig
	bool have;
	PlusIdListVersion current;
	std::string error;          // of the last fetch, empty when it worked
	time_t next;
	PlusIdListJob *job;
};

static pthread_mutex_t lists_mutex = PTHREAD_MUTEX_INITIALIZER;
static std::vector<PlusIdList> lists;
static std::string listsText;
static bool listsConfigured;

void PlusIdLists_RegisterDvars()
{
	sv_banLists = Dvar_RegisterString("sv_banLists", "", DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_banListRefresh = Dvar_RegisterInt("sv_banListRefresh", 30, 1, 10080, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
}

static bool PlusIdLists_ById(const PlusIdListBan &a, const PlusIdListBan &b)
{
	return strcmp(a.id, b.id) < 0;
}

// Checks the signature, then the format: the JSON is hostile. With same, a
// version whose hash is that one is only verified (*unchanged). Thread-safe;
// throws std::bad_alloc.
static bool PlusIdLists_Read(const unsigned char pk[33], const unsigned char *same, PlusIdListVersion *v, bool *unchanged, std::string *error)
{
	unsigned char *hash = v->hash, rs[64];
	size_t length, start = v->sig.find_first_not_of(" \t\r\n"), end = v->sig.find_last_not_of(" \t\r\n");

	if ( v->json.size() > PLUSID_LIST_BYTES )
	{
		*error = "too large";
		return false;
	}
	if ( start == std::string::npos
	     || mbedtls_base64_decode(rs, sizeof(rs), &length, (const unsigned char *)v->sig.data() + start, end + 1 - start) != 0 || length != sizeof(rs)
	     || mbedtls_sha256((const unsigned char *)v->json.data(), v->json.size(), hash, 0) != 0 || !PlusId_VerifyDigest(pk, hash, rs) )
	{
		*error = "bad signature";
		return false;
	}
	*unchanged = same && !memcmp(same, hash, 32);
	if ( *unchanged )
		return true;

	yyjson_doc *doc = yyjson_read(v->json.data(), v->json.size(), 0);
	yyjson_val *root = yyjson_doc_get_root(doc), *format = yyjson_obj_get(root, "format"), *name = yyjson_obj_get(root, "name");
	yyjson_val *appeal = yyjson_obj_get(root, "appeal"), *updated = yyjson_obj_get(root, "updated"), *entries = yyjson_obj_get(root, "entries");
	size_t i, max;
	yyjson_val *e;

	v->bans.clear();
	if ( !yyjson_is_obj(root) || !yyjson_is_int(format) || yyjson_get_sint(format) != 1 )
		*error = "bad format: format is not 1";
	else if ( !yyjson_is_str(name) || !yyjson_get_len(name) || yyjson_get_len(name) >= sizeof(v->name) )
		*error = "bad format: name";
	else if ( appeal && !yyjson_is_null(appeal) && ( !yyjson_is_str(appeal) || yyjson_get_len(appeal) >= sizeof(v->appeal) ) )
		*error = "bad format: appeal";
	else if ( !yyjson_is_str(updated) || !PlusId_ParseTime(yyjson_get_str(updated), &v->updated) )
		*error = "bad format: updated";
	else if ( !yyjson_is_arr(entries) || yyjson_arr_size(entries) > PLUSID_LIST_ENTRIES )
		*error = "bad format: entries missing or over 100000";
	else
	{
		error->clear();
		try
		{
			v->bans.reserve(yyjson_arr_size(entries));
			yyjson_arr_foreach(entries, i, max, e)
			{
				PlusIdBan ban;
				PlusIdListBan entry;

				if ( !PlusId_ParseBan(e, &ban, true) )
				{
					char text[64];
					snprintf(text, sizeof(text), "bad format: entry %u", (unsigned int)i);
					*error = text;
					break;
				}
				memcpy(entry.id, ban.id, sizeof(entry.id));
				memcpy(entry.reason, ban.reason, sizeof(entry.reason));
				entry.expires = ban.expires;
				v->bans.push_back(entry);
			}
		}
		catch ( ... )
		{
			yyjson_doc_free(doc);
			throw;
		}
	}

	if ( error->empty() )
	{
		Q_strncpyz(v->name, yyjson_get_str(name), sizeof(v->name));
		Q_strncpyz(v->appeal, yyjson_is_str(appeal) ? yyjson_get_str(appeal) : "", sizeof(v->appeal));
		// the text goes into a quoted refusal: no control characters, ' for "
		for ( char *p = v->name; *p; p++ )
			*p = (unsigned char)*p < 32 ? ' ' : *p == '"' ? '\'' : *p;
		for ( char *p = v->appeal; *p; p++ )
			*p = (unsigned char)*p < 32 ? ' ' : *p == '"' ? '\'' : *p;
		std::sort(v->bans.begin(), v->bans.end(), PlusIdLists_ById);
	}
	yyjson_doc_free(doc);
	return error->empty();
}

static bool PlusIdLists_Get(const NetHttpRequest *req, const volatile int *cancel, const char *what, std::string *body, std::string *error)
{
	NetHttpResponse res;
	char status[32];

	if ( !NetHttp_Fetch(req, cancel, &res) )
		*error = what + res.error;
	else if ( res.status != 200 )
	{
		snprintf(status, sizeof(status), "HTTP %d", res.status);
		*error = what + std::string(status);
	}
	else
		body->swap(res.body);
	return error->empty();
}

static void PlusIdLists_Release(PlusIdListJob *job)
{
	pthread_mutex_lock(&lists_mutex);
	bool last = --job->refs == 0;
	pthread_mutex_unlock(&lists_mutex);

	if ( last )
		delete job;
}

// never touches the engine; writes only the job's results, which the main thread reads once done is set
static void *PlusIdLists_Worker(void *arg)
{
	PlusIdListJob *job = (PlusIdListJob *)arg;

	// out of memory is a failed fetch: an exception left here would end the server
	try
	{
		job->ok = PlusIdLists_Get(&job->list, &job->cancel, "", &job->version.json, &job->error)
		          && PlusIdLists_Get(&job->sig, &job->cancel, "signature file: ", &job->version.sig, &job->error)
		          && PlusIdLists_Read(job->pk, job->haveHash ? job->hash : NULL, &job->version, &job->unchanged, &job->error);
	}
	catch ( const std::exception &e )
	{
		job->ok = false;
		std::vector<PlusIdListBan>().swap(job->version.bans);
		std::string().swap(job->version.json);
		job->error = e.what();
	}

	pthread_mutex_lock(&lists_mutex);
	job->done = true;
	pthread_mutex_unlock(&lists_mutex);

	PlusIdLists_Release(job);
	return NULL;
}

static bool PlusIdLists_ReadFile(const char *path, size_t max, std::string *out)
{
	FILE *f = fopen(path, "rb");
	char buf[65536];
	size_t n;

	if ( !f )
		return false;
	out->clear();
	while ( out->size() <= max && ( n = fread(buf, 1, sizeof(buf), f) ) > 0 )
		out->append(buf, n);
	bool ok = !ferror(f) && out->size() <= max;
	fclose(f);
	return ok;
}

static bool PlusIdLists_WriteFile(const char *path, const std::string &data)
{
	char tmp[MAX_OSPATH * 2 + 32];

	Com_sprintf(tmp, sizeof(tmp), "%s.tmp.%d", path, (int)getpid());
	FILE *f = fopen(tmp, "wb");
	bool ok = f && fwrite(data.data(), 1, data.size(), f) == data.size();
	ok = f && fclose(f) == 0 && ok;
	ok = ok && PlusId_Replace(tmp, path);
	if ( !ok )
		remove(tmp);
	return ok;
}

static void PlusIdLists_Start(PlusIdList *l)
{
	const dvar_t *caFile = Dvar_FindVar("sv_httpCaFile"), *allowPrivate = Dvar_FindVar("sv_httpAllowPrivate");
	PlusIdListJob *job = new PlusIdListJob;

	NetHttp_ParseUrl(l->url.c_str(), false, &job->list.url);
	NetHttp_ParseUrl(( l->url + ".sig" ).c_str(), false, &job->sig.url);
	job->list.method = job->sig.method = "GET";
	job->list.timeoutMs = job->sig.timeoutMs = PLUSID_LIST_TIMEOUT;
	job->list.tls.allowInsecure = false;
	job->list.tls.caFile = caFile ? caFile->current.string : "";
	job->list.tls.allowPrivate = allowPrivate && allowPrivate->current.boolean;
	job->sig.tls = job->list.tls;
	job->list.maxBody = PLUSID_LIST_BYTES;
	job->sig.maxBody = PLUSID_SIG_BYTES;
	memcpy(job->pk, l->pk, sizeof(job->pk));
	job->haveHash = l->have;
	memcpy(job->hash, l->current.hash, sizeof(job->hash));
	job->cancel = 0;
	job->refs = 2;
	job->done = job->ok = job->unchanged = false;

	if ( !Http_StartThread(PlusIdLists_Worker, job) )
	{
		delete job;
		l->error = "cannot start a thread";
		l->next = time(NULL) + sv_banListRefresh->current.integer * 60;
		return;
	}
	l->job = job;
}

static void PlusIdLists_Failed(PlusIdList *l, const std::string &error)
{
	char when[21];

	l->error = error;
	if ( !l->have )
	{
		Com_Printf("plusid: list %s not updated: %s; it does not apply until a good copy arrives\n", l->url.c_str(), error.c_str());
		return;
	}
	PlusId_FormatTime(l->current.updated, false, when);
	Com_Printf("plusid: list %s not updated: %s; keeping the copy of %s\n", l->url.c_str(), error.c_str(), when);
}

// main thread: the result of a finished fetch
static void PlusIdLists_Adopt(PlusIdList *l, PlusIdListJob *job)
{
	char was[21], now[21];

	if ( !job->ok )
	{
		PlusIdLists_Failed(l, job->error);
		return;
	}
	// the same bytes again
	if ( job->unchanged )
	{
		l->error.clear();
		return;
	}
	// an equal updated is the same content served again
	if ( l->have && job->version.updated < l->current.updated )
	{
		PlusId_FormatTime(job->version.updated, false, now);
		PlusId_FormatTime(l->current.updated, false, was);
		PlusIdLists_Failed(l, std::string("updated ") + now + " is older than " + was);
		return;
	}
	l->error.clear();
	std::swap(l->current, job->version);
	l->have = true;
	PlusId_FormatTime(l->current.updated, false, now);
	Com_Printf("plusid: list %s (%s) updated %s, %d entries\n", l->current.name, l->url.c_str(), now, (int)l->current.bans.size());
	if ( !PlusIdLists_WriteFile(va("%s.json", l->cache), l->current.json) || !PlusIdLists_WriteFile(va("%s.sig", l->cache), l->current.sig) )
		Com_Printf("plusid: cannot write %s.json, the list is not cached\n", l->cache);
	std::string().swap(l->current.json);
	std::string().swap(l->current.sig);
	PlusId_KickBanned();
}

static void PlusIdLists_Configure()
{
	char dir[MAX_OSPATH * 2];
	std::string text = sv_banLists->current.string;

	for ( size_t k = 0; k < lists.size(); k++ )
	{
		if ( lists[k].job )
		{
			__atomic_store_n(&lists[k].job->cancel, 1, __ATOMIC_RELEASE);
			PlusIdLists_Release(lists[k].job);
		}
	}
	lists.clear();
	listsText = text;
	listsConfigured = true;

	PlusId_Path("plusid_lists", dir, sizeof(dir));
	for ( size_t pos = 0; pos < text.size(); )
	{
		size_t semi = text.find(';', pos);
		std::string item = text.substr(pos, semi == std::string::npos ? std::string::npos : semi - pos);
		pos = semi == std::string::npos ? text.size() : semi + 1;

		size_t first = item.find_first_not_of(" \t"), last = item.find_last_not_of(" \t");
		if ( first == std::string::npos )
			continue;
		item = item.substr(first, last + 1 - first);

		size_t bar = item.rfind('|');
		PlusIdList l;
		NetUrl url;
		unsigned char hash[32];
		char hex[65];

		l.url = item.substr(0, bar == std::string::npos ? item.size() : bar);
		if ( bar == std::string::npos || !PlusId_FromHex(item.c_str() + bar + 1, l.pk, sizeof(l.pk)) || ( l.pk[0] != 2 && l.pk[0] != 3 ) )
		{
			Com_Printf("plusid: malformed key for list %s in sv_banLists, skipped\n", l.url.c_str());
			continue;
		}
		if ( !NetHttp_ParseUrl(l.url.c_str(), false, &url) )
		{
			Com_Printf("plusid: bad URL %s in sv_banLists, skipped\n", l.url.c_str());
			continue;
		}
		if ( lists.size() == PLUSID_MAX_LISTS )
		{
			Com_Printf("plusid: more than %d lists in sv_banLists, %s skipped\n", PLUSID_MAX_LISTS, l.url.c_str());
			continue;
		}

		mbedtls_sha256((const unsigned char *)l.url.data(), l.url.size(), hash, 0);
		PlusId_ToHex(hash, sizeof(hash), hex);
		Com_sprintf(l.cache, sizeof(l.cache), "%s/%s", dir, hex);
		l.have = false;
		l.current.updated = 0;
		l.next = 0;
		l.job = NULL;

		// the last good copy applies until a fetch brings a newer one
		std::string error;
		char when[21];
		bool unchanged, found = false, read = false;
		try
		{
			found = PlusIdLists_ReadFile(va("%s.json", l.cache), PLUSID_LIST_BYTES, &l.current.json)
			        && PlusIdLists_ReadFile(va("%s.sig", l.cache), PLUSID_SIG_BYTES, &l.current.sig);
			read = found && PlusIdLists_Read(l.pk, NULL, &l.current, &unchanged, &error);
		}
		catch ( const std::exception &e )
		{
			error = e.what();
			std::vector<PlusIdListBan>().swap(l.current.bans);
		}
		std::string().swap(l.current.json);
		std::string().swap(l.current.sig);
		if ( !found && error.empty() )
			Com_Printf("plusid: no cached copy of list %s, it applies once fetched\n", l.url.c_str());
		else if ( !read )
			Com_Printf("plusid: the cached copy of list %s is not valid (%s), it applies once fetched\n", l.url.c_str(), error.c_str());
		else
		{
			l.have = true;
			PlusId_FormatTime(l.current.updated, false, when);
			Com_Printf("plusid: list %s (%s) from the cache, updated %s, %d entries\n", l.current.name, l.url.c_str(), when, (int)l.current.bans.size());
		}
		lists.push_back(std::move(l));
	}

	if ( !lists.empty() )
	{
#ifdef _WIN32
		_mkdir(dir);
#else
		mkdir(dir, 0755);
#endif
		PlusId_KickBanned();
	}
}

void PlusIdLists_Frame()
{
	time_t now = time(NULL);

	if ( !listsConfigured || listsText != sv_banLists->current.string )
		PlusIdLists_Configure();

	for ( size_t k = 0; k < lists.size(); k++ )
	{
		PlusIdList *l = &lists[k];

		if ( l->job )
		{
			pthread_mutex_lock(&lists_mutex);
			bool done = l->job->done;
			pthread_mutex_unlock(&lists_mutex);
			if ( !done )
				continue;

			PlusIdLists_Adopt(l, l->job);
			PlusIdLists_Release(l->job);
			l->job = NULL;
			l->next = now + sv_banListRefresh->current.integer * 60;
		}
		else if ( now >= l->next )
			PlusIdLists_Start(l);
	}
}

const PlusIdBan *PlusIdLists_Find(const char *id, const char **listName, const char **appeal)
{
	static PlusIdBan found;
	long long now = time(NULL);
	PlusIdListBan key;

	Q_strncpyz(key.id, id, sizeof(key.id));
	for ( size_t k = 0; k < lists.size(); k++ )
	{
		const PlusIdListVersion &v = lists[k].current;

		if ( !lists[k].have )
			continue;
		for ( std::vector<PlusIdListBan>::const_iterator b = std::lower_bound(v.bans.begin(), v.bans.end(), key, PlusIdLists_ById);
		      b != v.bans.end() && !strcmp(b->id, id); ++b )
		{
			if ( !b->expires || b->expires > now )
			{
				memset(&found, 0, sizeof(found));
				memcpy(found.id, b->id, sizeof(found.id));
				memcpy(found.reason, b->reason, sizeof(found.reason));
				found.expires = b->expires;
				*listName = v.name;
				*appeal = v.appeal;
				return &found;
			}
		}
	}
	return NULL;
}

void PlusIdLists_Status_f()
{
	char when[21];

	Com_Printf("num url name entries updated status\n");
	for ( size_t k = 0; k < lists.size(); k++ )
	{
		const PlusIdList &l = lists[k];

		PlusId_FormatTime(l.current.updated, false, when);
		Com_Printf("%d %s \"%s\" %d %s %s\n", (int)k, l.url.c_str(), l.have ? l.current.name : "-", l.have ? (int)l.current.bans.size() : 0,
		           l.have ? when : "-", !l.error.empty() ? l.error.c_str() : l.have ? "ok" : "fetching");
	}
}

#else

// without the HTTP client there are no shared lists
void PlusIdLists_RegisterDvars() {}
void PlusIdLists_Frame() {}
const PlusIdBan *PlusIdLists_Find(const char *, const char **, const char **) { return NULL; }
void PlusIdLists_Status_f() { Com_Printf("shared ban lists need a build with WITH_HTTP=true\n"); }

#endif

#endif
