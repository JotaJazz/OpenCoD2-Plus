#include "gsc.hpp"
#include "plusid.hpp"
#include "yyjson/yyjson.h"

#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef LIBCOD

// The Plus ID check at connect (docs/architecture/plus-id.md): the key
// proof is in plusid.hpp, the server's side (which address a signature may
// name, the dvars, the ID of each client) here.

#define PLUSID_NOTICE "This server requires a Plus ID: play with the OpenCoD2-Plus client, https://github.com/JotaJazz/OpenCoD2-Plus/releases"

static dvar_t *sv_publicAddress;
static dvar_t *sv_requirePlusId;
static dvar_t *sv_plusIdNotice;
static dvar_t *sv_plusIdSeenFile;
static char plusIds[MAX_CLIENTS][22];
// ban-evasion marks of each player: PLUSID_FLAG_GUID, PLUSID_FLAG_IP
static int plusFlags[MAX_CLIENTS];

// Playtime per ID (docs/architecture/plus-id.md): the file has one line per
// ID, "id first_seen seconds". A slot keeps what the file said when its player
// connected (seen, stored) and the seconds only this server counted since
// (unsaved); a save adds only those to whatever the file holds then, so two
// servers sharing the file do not lose each other's time.
struct PlusId_Seen
{
	char id[22];
	long long first, seconds;
};

static struct { long long first, stored, unsaved; } slots[MAX_CLIENTS];
static time_t lastTick, lastSave;

// A dvar's address, resolved only when its text changes: a host name would
// block the frame on every connect. False when the dvar is empty, not an
// address, 0.0.0.0 or the "localhost" of net_ip, which binds every interface.
struct PlusId_CachedAddress
{
	const char *name;
	char text[256];
	bool valid;
	netadr_t adr;
};

static PlusId_CachedAddress publicAddress = { "sv_publicAddress" }, netIp = { "net_ip" };

static bool PlusId_DvarAddress(PlusId_CachedAddress *cache, netadr_t *adr)
{
	const dvar_t *dvar = Dvar_FindVar(cache->name);
	const char *text = dvar ? dvar->current.string : "";

	if ( !cache->text[0] || strncmp(cache->text + 1, text, sizeof(cache->text) - 2) )
	{
		// a leading '+' keeps an empty dvar from looking unresolved
		Com_sprintf(cache->text, sizeof(cache->text), "+%s", text);
		cache->valid = *text && Q_stricmp(text, "localhost") && NET_StringToAdr(text, &cache->adr)
		               && cache->adr.type == NA_IP && ( cache->adr.ip[0] | cache->adr.ip[1] | cache->adr.ip[2] | cache->adr.ip[3] );
	}

	*adr = cache->adr;
	return cache->valid;
}

// RFC 1918, loopback and link-local only; not Sys_IsLANAddress, which also
// takes anyone in the /24 of one of the host's own IPs (a VPN neighbour)
static bool PlusId_IsPrivate(netadr_t adr)
{
	const unsigned char *ip = adr.ip;

	return adr.type == NA_IP && ( ip[0] == 10 || ip[0] == 127 || ( ip[0] == 169 && ip[1] == 254 )
	                              || ( ip[0] == 172 && ( ip[1] & 0xF0 ) == 16 ) || ( ip[0] == 192 && ip[1] == 168 ) );
}

bool PlusId_AddressAccepted(netadr_t signedAdr, netadr_t from)
{
	const dvar_t *port = Dvar_FindVar("net_port");
	netadr_t own;

	if ( signedAdr.type != NA_IP || !port || (unsigned short)BigShort(signedAdr.port) != port->current.integer )
		return false;

	// a private address names this server only for a packet that comes from a
	// private network too, even when it is net_ip: from a public one it may be
	// another server's relay
	if ( PlusId_IsPrivate(signedAdr) && !PlusId_IsPrivate(from) )
		return false;

	if ( PlusId_DvarAddress(&publicAddress, &own) && !memcmp(own.ip, signedAdr.ip, 4) )
		return true;

	if ( PlusId_DvarAddress(&netIp, &own) && !memcmp(own.ip, signedAdr.ip, 4) )
		return true;

	return PlusId_IsPrivate(signedAdr);
}

// the ip:port the client signed, only in the form NET_AdrToString writes, so
// that it is never resolved and the server checks the same text
static bool PlusId_SignedAddress(const char *text, netadr_t *adr)
{
	return *text && strspn(text, "0123456789.:") == strlen(text) && NET_StringToAdr(text, adr)
	       && adr->type == NA_IP && !strcmp(NET_AdrToString(*adr), text);
}

static int PlusId_EvasionFlags(const char *id, netadr_t from, int guid);

// the refusal of an ID's live ban, NULL without one; a static buffer, valid until the next call
static const char *PlusId_BanText(const char *id, const char **listName)
{
	static char text[512];
	const char *appeal;
	const PlusIdBan *ban = PlusId_FindBan(id, listName, &appeal);

	if ( !ban )
		return NULL;
	if ( !*listName )
		Com_sprintf(text, sizeof(text), "You are banned from this server: %s", ban->reason);
	else if ( *appeal )
		Com_sprintf(text, sizeof(text), "You are banned by %s: %s. Appeal: %s", *listName, ban->reason, appeal);
	else
		Com_sprintf(text, sizeof(text), "You are banned by %s: %s", *listName, ban->reason);
	return text;
}

// a ban refusal is a static buffer, which the caller sends at once
const char *PlusId_CheckConnect(netadr_t from, char *userinfo, int challenge, int guid, char idOut[22], int *flagsOut)
{
	const char *banText, *listName;
	static leakyBucket_t bucket;
	char pk[PLUSID_PK_HEX + 2], ps[PLUSID_PS_HEX + 2], pa[64];
	netadr_t signedAdr;

	idOut[0] = '\0';
	*flagsOut = 0;
	Q_strncpyz(pk, Info_ValueForKey(userinfo, "pk"), sizeof(pk));
	Q_strncpyz(ps, Info_ValueForKey(userinfo, "ps"), sizeof(ps));
	Q_strncpyz(pa, Info_ValueForKey(userinfo, "pa"), sizeof(pa));
	// only for this check: not kept in the userinfo nor shown to scripts
	Info_RemoveKey(userinfo, "pk");
	Info_RemoveKey(userinfo, "ps");
	Info_RemoveKey(userinfo, "pa");

	if ( *pk || *ps || *pa )
	{
		if ( !PlusId_SignedAddress(pa, &signedAdr) )
			Com_Printf("plusid: malformed signed address from %s\n", NET_AdrToString(from));
		else if ( !PlusId_AddressAccepted(signedAdr, from) )
			Com_Printf("plusid: %s signed for another address, %s\n", NET_AdrToString(from), pa);
		// the challenge and SV_DirectConnect's limit per address already
		// passed; this caps the CPU all addresses together can take
		else if ( SVC_RateLimit(&bucket, 50, 20) )
			Com_Printf("plusid: too many verifications, %s connects without a Plus ID\n", NET_AdrToString(from));
		else if ( !PlusId_Verify(pk, ps, challenge, pa, idOut) )
		{
			idOut[0] = '\0';
			Com_Printf("plusid: invalid signature from %s\n", NET_AdrToString(from));
		}
	}

	if ( idOut[0] && ( banText = PlusId_BanText(idOut, &listName) ) )
	{
		Com_Printf("plusid: refused banned %s from %s%s%s\n", idOut, NET_AdrToString(from), listName ? " by " : "", listName ? listName : "");
		return banText;
	}

	if ( idOut[0] )
		*flagsOut = PlusId_EvasionFlags(idOut, from, guid);

	if ( !idOut[0] && sv_requirePlusId->current.boolean )
		return *sv_plusIdNotice->current.string ? sv_plusIdNotice->current.string : PLUSID_NOTICE;

	return NULL;
}

// a file name from a dvar or a fixed one: absolute as it is, else under fs_homepath
void PlusId_Path(const char *name, char *path, size_t size)
{
	const char *home = Dvar_FindVar("fs_homepath") ? Dvar_FindVar("fs_homepath")->current.string : "";

	if ( name[0] == '/' || name[0] == '\\' || name[1] == ':' || !*home )
		Q_strncpyz(path, name, size);
	else
		Com_sprintf(path, size, "%s/%s", home, name);
}

// the tmp file renamed over the file, which Windows only does with this call
bool PlusId_Replace(const char *tmp, const char *path)
{
#ifdef _WIN32
	return MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING);
#else
	return rename(tmp, path) == 0;
#endif
}

// False when the file exists and cannot be read whole: it is then never
// overwritten. A missing file is an empty list.
static bool PlusId_ReadSeen(const char *path, std::vector<PlusId_Seen> &list)
{
	FILE *f = fopen(path, "r");
	PlusId_Seen e;
	char id[64];
	long long first, seconds;
	int n;

	if ( !f )
		return errno == ENOENT;

	while ( ( n = fscanf(f, "%63s %lld %lld", id, &first, &seconds) ) == 3 )
	{
		if ( strlen(id) >= sizeof(e.id) )
			break;
		strcpy(e.id, id);
		e.first = first;
		e.seconds = seconds;
		list.push_back(e);
	}

	bool ok = n == EOF && !ferror(f);
	fclose(f);
	return ok;
}

// the time of dropped players that a save has not written yet: kept until one does
static std::vector<PlusId_Seen> pending;

static PlusId_Seen *PlusId_Find(std::vector<PlusId_Seen> &list, const char *id, long long first)
{
	for ( size_t k = 0; k < list.size(); k++ )
	{
		if ( !strcmp(list[k].id, id) )
			return &list[k];
	}

	PlusId_Seen e = { "", first, 0 };
	strcpy(e.id, id);
	list.push_back(e);
	return &list.back();
}

static void PlusId_Save()
{
	std::vector<PlusId_Seen> list;
	char path[MAX_OSPATH * 2], tmp[MAX_OSPATH * 2 + 24];
	bool any = !pending.empty();

	lastSave = time(NULL);
	for ( int i = 0; i < MAX_CLIENTS; i++ )
		any = any || ( plusIds[i][0] && slots[i].unsaved > 0 );
	if ( !any )
		return;

	PlusId_Path(sv_plusIdSeenFile->current.string, path, sizeof(path));
	if ( !PlusId_ReadSeen(path, list) )
	{
		Com_Printf("plusid: cannot read %s, playtime not saved\n", path);
		return;
	}

	// the file is only changed in memory here; the slots are marked saved after the rename
	for ( size_t k = 0; k < pending.size(); k++ )
	{
		PlusId_Seen *e = PlusId_Find(list, pending[k].id, pending[k].first);
		e->first = e->first < pending[k].first ? e->first : pending[k].first;
		e->seconds += pending[k].seconds;
	}
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( !plusIds[i][0] || slots[i].unsaved <= 0 )
			continue;

		PlusId_Seen *e = PlusId_Find(list, plusIds[i], slots[i].first);
		e->first = e->first < slots[i].first ? e->first : slots[i].first;
		e->seconds += slots[i].unsaved;
	}

	// a name of its own: servers sharing the file never write the same temporary file
	// shortcut: two servers saving in the same instant can lose one interval; add a lock if groups report it
	Com_sprintf(tmp, sizeof(tmp), "%s.tmp.%d", path, (int)getpid());
	FILE *f = fopen(tmp, "w");
	bool ok = f != NULL;
	for ( size_t k = 0; ok && k < list.size(); k++ )
		ok = fprintf(f, "%s %lld %lld\n", list[k].id, list[k].first, list[k].seconds) > 0;
	ok = f && fclose(f) == 0 && ok;
	ok = ok && PlusId_Replace(tmp, path);
	if ( !ok )
	{
		Com_Printf("plusid: cannot write %s, playtime not saved\n", path);
		return;
	}

	pending.clear();
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( !plusIds[i][0] || slots[i].unsaved <= 0 )
			continue;

		PlusId_Seen *e = PlusId_Find(list, plusIds[i], 0);
		slots[i].first = e->first;
		slots[i].stored = e->seconds;
		slots[i].unsaved = 0;
	}
}

// adds the seconds that passed to every connected player with an ID; a
// zombie slot keeps its ID until freed but is not playing
static void PlusId_Tick()
{
	time_t now = time(NULL);

	if ( !lastTick || now < lastTick )
		lastTick = now;
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( plusIds[i][0] && svs.clients[i].state >= CS_CONNECTED )
			slots[i].unsaved += now - lastTick;
	}
	lastTick = now;
}

void PlusId_Frame()
{
	PlusIdLists_Frame();
	PlusId_Tick();
	if ( time(NULL) - lastSave >= 60 )
		PlusId_Save();
}

// at the end of the map and at shutdown
void PlusId_Shutdown()
{
	PlusId_Tick();
	PlusId_Save();
}

void PlusId_ClientConnected(int clientNum, const char *id, int flags)
{
	std::vector<PlusId_Seen> list;
	char path[MAX_OSPATH * 2];

	Q_strncpyz(plusIds[clientNum], id, sizeof(plusIds[clientNum]));
	plusFlags[clientNum] = flags;
	slots[clientNum].first = time(NULL);
	slots[clientNum].stored = slots[clientNum].unsaved = 0;
	if ( !*id )
		return;

	PlusId_Tick();
	PlusId_Path(sv_plusIdSeenFile->current.string, path, sizeof(path));
	PlusId_ReadSeen(path, list);
	for ( size_t k = 0; k < list.size(); k++ )
	{
		if ( !strcmp(list[k].id, id) )
		{
			slots[clientNum].first = list[k].first;
			slots[clientNum].stored = list[k].seconds;
		}
	}
}

void PlusId_ClientDropped(int clientNum)
{
	if ( plusIds[clientNum][0] )
	{
		PlusId_Tick();
		if ( slots[clientNum].unsaved > 0 )
		{
			PlusId_Seen *e = PlusId_Find(pending, plusIds[clientNum], slots[clientNum].first);
			e->seconds += slots[clientNum].unsaved;
			// now pending only: the save must not write them again from the slot
			slots[clientNum].unsaved = 0;
		}
		PlusId_Save();
	}
	plusIds[clientNum][0] = '\0';
	plusFlags[clientNum] = 0;
	slots[clientNum].unsaved = 0;
}

int PlusId_Playtime(int clientNum)
{
	return plusIds[clientNum][0] ? (int)( slots[clientNum].stored + slots[clientNum].unsaved ) : 0;
}

int PlusId_FirstSeenAgo(int clientNum)
{
	return plusIds[clientNum][0] ? (int)( time(NULL) - slots[clientNum].first ) : 0;
}

const char *PlusId_ForClient(int clientNum)
{
	return plusIds[clientNum];
}

const char *PlusId_FlagsForClient(int clientNum)
{
	static const char *names[] = { "", "guid", "ip", "guid,ip" };

	return names[plusFlags[clientNum] & 3];
}

void PlusId_RegisterDvars()
{
	sv_publicAddress = Dvar_RegisterString("sv_publicAddress", "", DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_requirePlusId = Dvar_RegisterBool("sv_requirePlusId", false, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_plusIdNotice = Dvar_RegisterString("sv_plusIdNotice", "", DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_plusIdSeenFile = Dvar_RegisterString("sv_plusIdSeenFile", "plusid_seen.txt", DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	PlusIdLists_RegisterDvars();
}

// at the first map, when the config has run and net_ip is set
void PlusId_SpawnServer()
{
	static bool warned;
	netadr_t own;

	if ( !warned && !PlusId_DvarAddress(&publicAddress, &own) && !PlusId_DvarAddress(&netIp, &own) )
		Com_Printf("plusid: no sv_publicAddress, only LAN players get a Plus ID\n");
	warned = true;
}

// Local bans (docs/architecture/plus-id.md): plusid_bans.json in fs_homepath,
// {"format":1,"entries":[{id, reason, by, evidence, date, expires, guid, ip}]}
// with date and expires as UTC text (expires null: permanent). The file
// is read again when its size or time changed, so an edit is applied at the
// next connect; it is read as hostile.

#define PLUSID_FLAG_GUID 1
#define PLUSID_FLAG_IP 2
#define PLUSID_MAX_BANS 100000
#define PLUSID_MAX_BANS_BYTES ( 4 * 1024 * 1024 )

static std::vector<PlusIdBan> localBans;
static long long bansSize = -1, bansTime;
// the file exists and could not be read: nothing is written over it until it reads
static bool bansBad;

// "YYYY-MM-DD" (00:00 UTC) or "YYYY-MM-DDTHH:MM:SSZ", nothing else, a real date
bool PlusId_ParseTime(const char *text, long long *out)
{
	struct tm tm;
	int n = (int)strlen(text);

	if ( ( n != 10 && n != 20 ) || strspn(text, "0123456789-T:Z") != (size_t)n )
		return false;
	memset(&tm, 0, sizeof(tm));
	int fields = n == 10 ? sscanf(text, "%4d-%2d-%2d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday)
	                     : sscanf(text, "%4d-%2d-%2dT%2d:%2d:%2dZ", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
	char again[21];
	if ( fields != ( n == 10 ? 3 : 6 ) || tm.tm_year < 1970 )
		return false;
	tm.tm_year -= 1900;
	tm.tm_mon--;
#ifdef _WIN32
	time_t t = _mkgmtime(&tm);
#else
	time_t t = timegm(&tm);
#endif
	// timegm normalizes 31 February; the same text back means it was real
	PlusId_FormatTime(t, n == 10, again);
	if ( t == (time_t)-1 || strcmp(again, text) )
		return false;
	*out = t;
	return true;
}

void PlusId_FormatTime(long long t, bool dateOnly, char out[21])
{
	time_t tt = (time_t)t;
	struct tm tm;
	// the reentrant gmtime: the list threads parse dates too
#ifdef _WIN32
	bool ok = gmtime_s(&tm, &tt) == 0;
#else
	bool ok = gmtime_r(&tt, &tm) != NULL;
#endif

	if ( !ok || tm.tm_year > 8099 )
		strcpy(out, dateOnly ? "1970-01-01" : "1970-01-01T00:00:00Z");
	else
		strftime(out, 21, dateOnly ? "%Y-%m-%d" : "%Y-%m-%dT%H:%M:%SZ", &tm);
}

static bool PlusId_ValidId(const char *id)
{
	if ( strlen(id) != 21 || id[0] != 'P' || id[1] != '-' )
		return false;
	for ( int i = 2; i < 21; i++ )
	{
		if ( i % 5 == 1 ? id[i] != '-' : !( ( id[i] >= 'A' && id[i] <= 'Z' ) || ( id[i] >= '2' && id[i] <= '7' ) ) )
			return false;
	}
	return true;
}

static bool PlusId_ValidIp(const char *ip)
{
	int a, b, c, d, n = 0;

	return !*ip || ( strlen(ip) < 16 && strspn(ip, "0123456789.") == strlen(ip) && sscanf(ip, "%3d.%3d.%3d.%3d%n", &a, &b, &c, &d, &n) == 4
	                 && !ip[n] && (unsigned)a < 256 && (unsigned)b < 256 && (unsigned)c < 256 && (unsigned)d < 256 );
}

// text from the file or rcon into a field: cut to size, control characters
// (the \x15 that localizes an error, line breaks) as spaces
static void PlusId_CopyText(char *to, size_t size, const char *from)
{
	Q_strncpyz(to, from, size);
	for ( char *p = to; *p; p++ )
	{
		if ( (unsigned char)*p < 32 )
			*p = ' ';
	}
}

// a string field that must be absent or shorter than size
static bool PlusId_JsonText(yyjson_val *obj, const char *key, char *to, size_t size)
{
	yyjson_val *v = yyjson_obj_get(obj, key);

	to[0] = '\0';
	if ( !v )
		return true;
	if ( !yyjson_is_str(v) || yyjson_get_len(v) >= size )
		return false;
	PlusId_CopyText(to, size, yyjson_get_str(v));
	return true;
}

// absent or null is 0
static bool PlusId_JsonTime(yyjson_val *obj, const char *key, long long *to)
{
	yyjson_val *v = yyjson_obj_get(obj, key);

	*to = 0;
	return !v || yyjson_is_null(v) || ( yyjson_is_str(v) && PlusId_ParseTime(yyjson_get_str(v), to) );
}

static bool PlusId_JsonNumber(yyjson_val *obj, const char *key, long long *to, long long min, long long max)
{
	yyjson_val *v = yyjson_obj_get(obj, key);

	*to = 0;
	if ( !v || yyjson_is_null(v) )
		return true;
	if ( !yyjson_is_int(v) || yyjson_get_sint(v) < min || yyjson_get_sint(v) > max )
		return false;
	*to = yyjson_get_sint(v);
	return true;
}

// an entry of the bans file or of a shared list; a shared one must not have guid or ip
bool PlusId_ParseBan(yyjson_val *e, PlusIdBan *ban, bool shared)
{
	yyjson_val *id = yyjson_obj_get(e, "id");
	long long guid;

	memset(ban, 0, sizeof(*ban));
	if ( !yyjson_is_obj(e) || !yyjson_is_str(id) || !PlusId_ValidId(yyjson_get_str(id))
	     || ( shared && ( yyjson_obj_get(e, "guid") || yyjson_obj_get(e, "ip") ) )
	     || !PlusId_JsonText(e, "reason", ban->reason, sizeof(ban->reason)) || !PlusId_JsonText(e, "by", ban->by, sizeof(ban->by))
	     || !PlusId_JsonText(e, "evidence", ban->evidence, sizeof(ban->evidence)) || !PlusId_JsonText(e, "ip", ban->ip, sizeof(ban->ip))
	     || !PlusId_ValidIp(ban->ip) || !PlusId_JsonTime(e, "date", &ban->date)
	     || !PlusId_JsonTime(e, "expires", &ban->expires) || !PlusId_JsonNumber(e, "guid", &guid, INT_MIN, INT_MAX) )
		return false;
	ban->guid = (int)guid;
	strcpy(ban->id, yyjson_get_str(id));
	return true;
}

static bool PlusId_StatBans(const char *path, long long *size, long long *mtime)
{
	struct stat st;

	if ( stat(path, &st) )
		return false;
	*size = st.st_size;
	*mtime = st.st_mtime;
	return true;
}

// reads the file when it changed; one that is missing is an empty list, one
// that does not parse keeps the last good list and is logged once
static void PlusId_LoadBans()
{
	char path[MAX_OSPATH * 2];
	long long size, mtime;

	PlusId_Path("plusid_bans.json", path, sizeof(path));
	if ( !PlusId_StatBans(path, &size, &mtime) )
	{
		size = -1;
		mtime = 0;
	}
	if ( size == bansSize && mtime == bansTime )
		return;
	bansSize = size;
	bansTime = mtime;
	if ( size < 0 )
	{
		bansBad = false;
		localBans.clear();
		return;
	}

	std::vector<PlusIdBan> list;
	yyjson_doc *doc = size <= PLUSID_MAX_BANS_BYTES ? yyjson_read_file(path, 0, NULL, NULL) : NULL;
	yyjson_val *entries = doc ? yyjson_obj_get(yyjson_doc_get_root(doc), "entries") : NULL;
	bool ok = yyjson_is_arr(entries) && yyjson_arr_size(entries) <= PLUSID_MAX_BANS;
	size_t i, max;
	yyjson_val *e;

	yyjson_arr_foreach(entries, i, max, e)
	{
		PlusIdBan ban;

		ok = ok && PlusId_ParseBan(e, &ban, false);
		if ( !ok )
			break;
		list.push_back(ban);
	}
	yyjson_doc_free(doc);

	bansBad = !ok;
	if ( ok )
		localBans.swap(list);
	else
		Com_Printf("plusid: cannot read %s, keeping the bans in memory\n", path);
}

static bool PlusId_WriteBans()
{
	char path[MAX_OSPATH * 2], tmp[MAX_OSPATH * 2 + 24];
	yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
	yyjson_mut_val *root = yyjson_mut_obj(doc), *entries = yyjson_mut_arr(doc);

	yyjson_mut_doc_set_root(doc, root);
	yyjson_mut_obj_add_int(doc, root, "format", 1);
	yyjson_mut_obj_add_val(doc, root, "entries", entries);
	for ( size_t k = 0; k < localBans.size(); k++ )
	{
		const PlusIdBan &b = localBans[k];
		yyjson_mut_val *e = yyjson_mut_arr_add_obj(doc, entries);

		yyjson_mut_obj_add_strcpy(doc, e, "id", b.id);
		yyjson_mut_obj_add_strcpy(doc, e, "reason", b.reason);
		yyjson_mut_obj_add_strcpy(doc, e, "by", b.by);
		yyjson_mut_obj_add_strcpy(doc, e, "evidence", b.evidence);
		char when[21];
		PlusId_FormatTime(b.date, true, when);
		yyjson_mut_obj_add_strcpy(doc, e, "date", when);
		if ( b.expires )
		{
			PlusId_FormatTime(b.expires, false, when);
			yyjson_mut_obj_add_strcpy(doc, e, "expires", when);
		}
		else
			yyjson_mut_obj_add_null(doc, e, "expires");
		yyjson_mut_obj_add_int(doc, e, "guid", b.guid);
		yyjson_mut_obj_add_strcpy(doc, e, "ip", b.ip);
	}

	size_t length = 0;
	char *text = yyjson_mut_write(doc, YYJSON_WRITE_PRETTY, &length);
	PlusId_Path("plusid_bans.json", path, sizeof(path));
	Com_sprintf(tmp, sizeof(tmp), "%s.tmp.%d", path, (int)getpid());
	FILE *f = text ? fopen(tmp, "wb") : NULL;
	bool ok = f != NULL;
	ok = ok && fwrite(text, 1, length, f) == length;
	ok = f && fclose(f) == 0 && ok;
	ok = ok && PlusId_Replace(tmp, path);
	free(text);
	yyjson_mut_doc_free(doc);
	if ( ok )
		PlusId_StatBans(path, &bansSize, &bansTime);
	else
		Com_Printf("plusid: cannot write %s, the ban was not saved\n", path);
	return ok;
}

const PlusIdBan *PlusId_FindBan(const char *id, const char **listName, const char **appeal)
{
	long long now = time(NULL);
	const char *name, *link;

	if ( listName )
		*listName = NULL;
	if ( appeal )
		*appeal = "";
	PlusId_LoadBans();
	for ( size_t k = 0; k < localBans.size(); k++ )
	{
		if ( !strcmp(localBans[k].id, id) && ( !localBans[k].expires || localBans[k].expires > now ) )
			return &localBans[k];
	}

	const PlusIdBan *ban = PlusIdLists_Find(id, &name, &link);
	if ( ban && listName )
		*listName = name;
	if ( ban && appeal )
		*appeal = link;
	return ban;
}

// the marks a player gets for matching a live local ban of another ID by guid or by IP
static int PlusId_EvasionFlags(const char *id, netadr_t from, int guid)
{
	static const char *how[] = { "", "guid", "ip", "guid,ip" };
	const char *ip = NET_AdrToStringNoPort(from);
	long long now = time(NULL);
	int flags = 0;

	PlusId_LoadBans();
	for ( size_t k = 0; k < localBans.size(); k++ )
	{
		const PlusIdBan &b = localBans[k];
		int match = ( guid && b.guid == guid ? PLUSID_FLAG_GUID : 0 ) | ( b.ip[0] && !strcmp(b.ip, ip) ? PLUSID_FLAG_IP : 0 );

		if ( !match || strcmp(b.id, id) == 0 || ( b.expires && b.expires <= now ) )
			continue;
		Com_Printf("plusid: possible evasion %s matches %s by %s\n", id, b.id, how[match]);
		flags |= match;
	}
	return flags;
}

// drops the connected players whose ID is banned
void PlusId_KickBanned()
{
	for ( int i = 0; i < sv_maxclients->current.integer; i++ )
	{
		client_t *cl = &svs.clients[i];
		const char *text, *listName;

		if ( cl->state >= CS_CONNECTED && plusIds[i][0] && ( text = PlusId_BanText(plusIds[i], &listName) ) )
		{
			Com_Printf("plusid: dropping banned %s (%s)%s%s\n", plusIds[i], cl->name, listName ? " by " : "", listName ? listName : "");
			SV_DropClient(cl, text);
		}
	}
}

static void PlusId_Unlist(const char *id)
{
	for ( size_t k = localBans.size(); k-- > 0; )
	{
		if ( !strcmp(localBans[k].id, id) )
			localBans.erase(localBans.begin() + k);
	}
}

// a file that did not read is never overwritten with the list in memory
static bool PlusId_CanChangeBans()
{
	char path[MAX_OSPATH * 2];

	PlusId_LoadBans();
	if ( !bansBad )
		return true;
	PlusId_Path("plusid_bans.json", path, sizeof(path));
	Com_Printf("plusid: cannot read %s, fix it first\n", path);
	return false;
}

static void PlusId_Ban_f()
{
	PlusIdBan ban;
	const char *who = Cmd_Argv(1), *minutes = Cmd_Argv(2);

	if ( !com_sv_running->current.boolean || Cmd_Argc() < 3 || !*minutes || strspn(minutes, "0123456789") != strlen(minutes) || strlen(minutes) > 7 )
	{
		Com_Printf("Usage: banPlusId <id|clientnum> <minutes, 0 permanent> <reason...>\n");
		return;
	}

	memset(&ban, 0, sizeof(ban));
	ban.date = time(NULL);
	ban.expires = atoi(minutes) ? ban.date + atoi(minutes) * 60LL : 0;
	PlusId_CopyText(ban.reason, sizeof(ban.reason), Cmd_Argc() > 3 ? Cmd_Args(3) : "banned");
	strcpy(ban.by, "rcon");

	if ( *who && strspn(who, "0123456789") == strlen(who) )
	{
		int num = atoi(who);

		if ( strlen(who) > 3 || num >= sv_maxclients->current.integer || svs.clients[num].state < CS_CONNECTED || !plusIds[num][0] )
		{
			Com_Printf("Client %s is not connected or has no Plus ID\n", who);
			return;
		}
		strcpy(ban.id, plusIds[num]);
		ban.guid = svs.clients[num].guid;
		Q_strncpyz(ban.ip, NET_AdrToStringNoPort(svs.clients[num].netchan.remoteAddress), sizeof(ban.ip));
		if ( !PlusId_ValidIp(ban.ip) )
			ban.ip[0] = '\0';
	}
	else if ( PlusId_ValidId(who) )
		strcpy(ban.id, who);
	else
	{
		Com_Printf("%s is not a Plus ID (P-XXXX-XXXX-XXXX-XXXX) or a client number\n", who);
		return;
	}

	// a new ban of an ID replaces the old one
	if ( !PlusId_CanChangeBans() )
		return;
	std::vector<PlusIdBan> before = localBans;
	PlusId_Unlist(ban.id);
	localBans.push_back(ban);
	if ( !PlusId_WriteBans() )
	{
		localBans = before;
		return;
	}
	Com_Printf("Banned %s\n", ban.id);
	PlusId_KickBanned();
}

static void PlusId_Unban_f()
{
	const char *id = Cmd_Argv(1);

	if ( !PlusId_ValidId(id) )
	{
		Com_Printf("Usage: unbanPlusId <id>\n");
		return;
	}

	if ( !PlusId_CanChangeBans() )
		return;
	std::vector<PlusIdBan> before = localBans;
	PlusId_Unlist(id);
	if ( localBans.size() == before.size() )
		Com_Printf("%s is not banned\n", id);
	else if ( !PlusId_WriteBans() )
		localBans = before;
	else
		Com_Printf("Unbanned %s\n", id);
}

static void PlusId_Status_f()
{
	if ( !com_sv_running->current.boolean )
	{
		Com_Printf("Server is not running.\n");
		return;
	}

	PlusId_Tick();
	Com_Printf("num name id playtime first_seen_ago flags\n");
	for ( int i = 0; i < sv_maxclients->current.integer; i++ )
	{
		if ( svs.clients[i].state >= CS_CONNECTED )
			Com_Printf("%i %s %s %i %i %s\n", i, svs.clients[i].name, plusIds[i][0] ? plusIds[i] : "-", PlusId_Playtime(i), PlusId_FirstSeenAgo(i),
			           plusFlags[i] ? PlusId_FlagsForClient(i) : "-");
	}
}

void PlusId_AddCommands()
{
	Cmd_AddCommand("plusIdStatus", PlusId_Status_f);
	Cmd_AddCommand("banPlusId", PlusId_Ban_f);
	Cmd_AddCommand("unbanPlusId", PlusId_Unban_f);
	Cmd_AddCommand("banListStatus", PlusIdLists_Status_f);
}

#endif
