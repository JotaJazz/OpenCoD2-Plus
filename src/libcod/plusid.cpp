#include "gsc.hpp"
#include "plusid.hpp"
#include "yyjson/yyjson.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <string>
#include <unordered_map>
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
// ID, "id first_seen seconds", kept in memory (seen) as last read or written.
// A slot keeps what the table and pending said when its player connected
// (first, stored) and the seconds only this server counted since (unsaved),
// which go to pending at the drop. A save reads the file again only when its
// size or time changed and adds only those seconds, so two servers sharing the
// file do not lose each other's time; connects and drops do no file I/O.
struct PlusId_Seen
{
	long long first, seconds;
};

typedef std::unordered_map<std::string, PlusId_Seen> PlusId_SeenTable;

static struct { long long first, stored, unsaved; } slots[MAX_CLIENTS];
static time_t lastTick, lastSave;

// A dvar's address, resolved only when its text changes: a host name would
// block the frame on every connect. False when the dvar is empty, not an
// address, 0.0.0.0 or the "localhost" of net_ip, which binds every interface.
struct PlusId_CachedAddress
{
	const char *name;
	char text[256];
	bool valid, hasPort;
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
		cache->hasPort = strchr(text, ':') != NULL;
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
	bool netPort;
	netadr_t own;

	if ( signedAdr.type != NA_IP )
		return false;
	netPort = port && (unsigned short)BigShort(signedAdr.port) == port->current.integer;

	// a private address names this server only for a packet that comes from a
	// private network too, even when it is net_ip: from a public one it may be
	// another server's relay
	if ( PlusId_IsPrivate(signedAdr) && !PlusId_IsPrivate(from) )
		return false;

	// a port in sv_publicAddress is the one players dial (docker -p, a NAT remap)
	if ( PlusId_DvarAddress(&publicAddress, &own) && !memcmp(own.ip, signedAdr.ip, 4)
	     && ( publicAddress.hasPort ? own.port == signedAdr.port : netPort ) )
		return true;

	if ( !netPort )
		return false;

	if ( PlusId_DvarAddress(&netIp, &own) && !memcmp(own.ip, signedAdr.ip, 4) )
		return true;

	// a private address of this host, not any: another host of the LAN may be
	// relaying a LAN player's signature
	return PlusId_IsPrivate(signedAdr) && ( signedAdr.ip[0] == 127 || Sys_IsLocalIP(signedAdr) );
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

// 1 when the file is there, 0 when it is missing, -1 when it cannot be told (errno)
static int PlusId_Stat(const char *path, long long *size, long long *mtime)
{
	struct stat st;

	if ( stat(path, &st) )
		return errno == ENOENT ? 0 : -1;
	*size = st.st_size;
	// in nanoseconds where there are: two writes in one second with one size still differ
#ifdef __linux__
	*mtime = st.st_mtim.tv_sec * 1000000000LL + st.st_mtim.tv_nsec;
#else
	// shortcut: seconds elsewhere, so a server sharing the seen file on Windows can miss a write of the same size in the same second; use the file times if groups report it
	*mtime = st.st_mtime;
#endif
	return 1;
}

// the first_seen kept is the smallest, the seconds add up
static void PlusId_AddSeen(PlusId_SeenTable &table, const std::string &id, long long first, long long seconds)
{
	PlusId_Seen e = { first, 0 };
	PlusId_Seen &to = table.insert(std::make_pair(id, e)).first->second;

	to.first = to.first < first ? to.first : first;
	to.seconds += seconds;
}

// False when the file cannot be read whole
static bool PlusId_ReadSeen(const char *path, PlusId_SeenTable &table)
{
	FILE *f = fopen(path, "r");
	char id[64];
	long long first, seconds;
	int n;

	if ( !f )
		return false;

	while ( ( n = fscanf(f, "%63s %lld %lld", id, &first, &seconds) ) == 3 )
	{
		if ( strlen(id) >= 22 )
			break;
		PlusId_AddSeen(table, id, first, seconds);
	}

	bool ok = n == EOF && !ferror(f);
	fclose(f);
	return ok;
}

// the file as last read or written; the time of dropped players that a save has not written yet
static PlusId_SeenTable seen, pending;
static std::string seenPath;
static long long seenSize = -1, seenTime;

// the table as the file is now, read again only when it changed (another
// server sharing it wrote); false when it exists and cannot be read: it is
// then never overwritten
static bool PlusId_LoadSeen(const char *path)
{
	PlusId_SeenTable table;
	long long size = -1, mtime = 0;
	int found = PlusId_Stat(path, &size, &mtime);

	if ( found < 0 )
		return false;
	if ( seenPath == path && size == seenSize && mtime == seenTime )
		return true;
	if ( found && !PlusId_ReadSeen(path, table) )
		return false;
	seen.swap(table);
	seenPath = path;
	seenSize = size;
	seenTime = mtime;
	return true;
}

// Servers sharing the file take turns through <file>.lock, made with O_EXCL;
// one older than 10 s was left by a crash and is taken over
static bool PlusId_LockSeen(const char *lock)
{
	struct stat st;

	for ( int tries = 0; tries < 2; tries++ )
	{
		int fd = open(lock, O_WRONLY | O_CREAT | O_EXCL, 0644);

		if ( fd >= 0 )
		{
			close(fd);
			return true;
		}
		if ( errno != EEXIST || stat(lock, &st) || time(NULL) - st.st_mtime < 10 )
			return false;
		remove(lock);
	}
	return false;
}

// the pending and unsaved seconds added to the file, under the lock
static void PlusId_WriteSeen(const char *path)
{
	char tmp[MAX_OSPATH * 2 + 24];

	if ( !PlusId_LoadSeen(path) )
	{
		Com_Printf("plusid: cannot read %s, playtime not saved\n", path);
		return;
	}

	// the file is only changed in a copy here; the slots are marked saved after the rename
	PlusId_SeenTable table = seen;
	for ( PlusId_SeenTable::const_iterator it = pending.begin(); it != pending.end(); ++it )
		PlusId_AddSeen(table, it->first, it->second.first, it->second.seconds);
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( plusIds[i][0] && slots[i].unsaved > 0 )
			PlusId_AddSeen(table, plusIds[i], slots[i].first, slots[i].unsaved);
	}

	// a name of its own: servers sharing the file never write the same temporary file
	Com_sprintf(tmp, sizeof(tmp), "%s.tmp.%d", path, (int)getpid());
	FILE *f = fopen(tmp, "w");
	bool ok = f != NULL;
	for ( PlusId_SeenTable::const_iterator it = table.begin(); ok && it != table.end(); ++it )
		ok = fprintf(f, "%s %lld %lld\n", it->first.c_str(), it->second.first, it->second.seconds) > 0;
	ok = f && fclose(f) == 0 && ok;
	ok = ok && PlusId_Replace(tmp, path);
	if ( !ok )
	{
		Com_Printf("plusid: cannot write %s, playtime not saved\n", path);
		return;
	}

	seen.swap(table);
	if ( PlusId_Stat(path, &seenSize, &seenTime) <= 0 )
		seenSize = -2;
	pending.clear();
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		PlusId_SeenTable::const_iterator it = plusIds[i][0] ? seen.find(plusIds[i]) : seen.end();

		if ( it == seen.end() )
			continue;
		slots[i].first = it->second.first;
		slots[i].stored = it->second.seconds;
		slots[i].unsaved = 0;
	}
}

// false when another server holds the lock: nothing is lost, the caller tries again
static bool PlusId_Save()
{
	char path[MAX_OSPATH * 2], lock[MAX_OSPATH * 2 + 8];
	bool any = !pending.empty();

	lastSave = time(NULL);
	for ( int i = 0; i < MAX_CLIENTS; i++ )
		any = any || ( plusIds[i][0] && slots[i].unsaved > 0 );

	PlusId_Path(sv_plusIdSeenFile->current.string, path, sizeof(path));
	// read also with nothing to save, so that connects see what other servers wrote
	if ( !any )
	{
		PlusId_LoadSeen(path);
		return true;
	}
	Com_sprintf(lock, sizeof(lock), "%s.lock", path);
	if ( !PlusId_LockSeen(lock) )
		return false;
	PlusId_WriteSeen(path);
	remove(lock);
	return true;
}

// adds the seconds that passed to every connected player with an ID; a
// dropped one has none, SV_DropClient frees it before the slot is a zombie
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
	// a save the lock put off is tried again 5 s later
	if ( time(NULL) - lastSave >= 60 && !PlusId_Save() )
		lastSave -= 55;
}

// at the end of the map and at shutdown
void PlusId_Shutdown()
{
	PlusId_Tick();
	// the last chance to save: wait up to 2 s for another server's lock
	for ( int tries = 0; tries < 20 && !PlusId_Save(); tries++ )
		Sys_SleepMSec(100);
}

void PlusId_ClientConnected(int clientNum, const char *id, int flags)
{
	Q_strncpyz(plusIds[clientNum], id, sizeof(plusIds[clientNum]));
	plusFlags[clientNum] = flags;
	slots[clientNum].first = time(NULL);
	slots[clientNum].stored = slots[clientNum].unsaved = 0;
	if ( !*id )
		return;

	PlusId_Tick();
	// what the last save read and the time of an earlier visit not saved yet
	PlusId_SeenTable *tables[] = { &seen, &pending };
	for ( int k = 0; k < 2; k++ )
	{
		PlusId_SeenTable::const_iterator it = tables[k]->find(id);

		if ( it == tables[k]->end() )
			continue;
		slots[clientNum].first = slots[clientNum].first < it->second.first ? slots[clientNum].first : it->second.first;
		slots[clientNum].stored += it->second.seconds;
	}
}

void PlusId_ClientDropped(int clientNum)
{
	if ( plusIds[clientNum][0] )
	{
		PlusId_Tick();
		// pending only, which the next save writes: the slot must not write them again
		if ( slots[clientNum].unsaved > 0 )
			PlusId_AddSeen(pending, plusIds[clientNum], slots[clientNum].first, slots[clientNum].unsaved);
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

// Dates as Unix seconds in long long, without time_t, whose 32 bits on the
// x86 build end in 2038; days from and to a proleptic Gregorian date, after
// http://howardhinnant.github.io/date_algorithms.html, for years from 1970
static long long PlusId_DaysFromCivil(long long y, int m, int d)
{
	y -= m <= 2;
	long long era = y / 400, yoe = y - era * 400, doy = ( 153 * ( m > 2 ? m - 3 : m + 9 ) + 2 ) / 5 + d - 1;

	return era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468;
}

// "YYYY-MM-DD" (00:00 UTC) or "YYYY-MM-DDTHH:MM:SSZ", nothing else, a real date from 1970
bool PlusId_ParseTime(const char *text, long long *out)
{
	int n = (int)strlen(text), y, m, d, hh = 0, mm = 0, ss = 0;
	char again[21];

	if ( ( n != 10 && n != 20 ) || strspn(text, "0123456789-T:Z") != (size_t)n )
		return false;
	int fields = n == 10 ? sscanf(text, "%4d-%2d-%2d", &y, &m, &d) : sscanf(text, "%4d-%2d-%2dT%2d:%2d:%2dZ", &y, &m, &d, &hh, &mm, &ss);
	if ( fields != ( n == 10 ? 3 : 6 ) || y < 1970 || m < 1 || m > 12 || d < 1 || d > 31 || hh > 23 || mm > 59 || ss > 59 )
		return false;
	long long t = PlusId_DaysFromCivil(y, m, d) * 86400 + hh * 3600 + mm * 60 + ss;
	// 31 February comes back as 3 March: the same text back means it was real
	PlusId_FormatTime(t, n == 10, again);
	if ( strcmp(again, text) )
		return false;
	*out = t;
	return true;
}

void PlusId_FormatTime(long long t, bool dateOnly, char out[21])
{
	// 1970 to 9999, the years PlusId_ParseTime reads
	if ( t < 0 || t > 253402300799LL )
		t = 0;

	long long z = t / 86400 + 719468, era = z / 146097, doe = z - era * 146097;
	long long yoe = ( doe - doe / 1460 + doe / 36524 - doe / 146096 ) / 365, doy = doe - ( 365 * yoe + yoe / 4 - yoe / 100 );
	int mp = (int)( ( 5 * doy + 2 ) / 153 ), d = (int)( doy - ( 153 * mp + 2 ) / 5 + 1 ), m = mp < 10 ? mp + 3 : mp - 9;
	int y = (int)( yoe + era * 400 + ( m <= 2 ) ), s = (int)( t % 86400 );
	// room for any int, which the compiler cannot bound
	char text[64];

	if ( dateOnly )
		snprintf(text, sizeof(text), "%04d-%02d-%02d", y, m, d);
	else
		snprintf(text, sizeof(text), "%04d-%02d-%02dT%02d:%02d:%02dZ", y, m, d, s / 3600, s / 60 % 60, s % 60);
	Q_strncpyz(out, text, 21);
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
// (the \x15 that localizes an error, line breaks) as spaces and " as ', which
// would end the quoted text of a disconnect or print command
static void PlusId_CopyText(char *to, size_t size, const char *from)
{
	Q_strncpyz(to, from, size);
	for ( char *p = to; *p; p++ )
	{
		if ( (unsigned char)*p < 32 )
			*p = ' ';
		else if ( *p == '"' )
			*p = '\'';
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
	// an expiry not after the date is a typo, and 1970-01-01 would read as permanent
	yyjson_val *expires = yyjson_obj_get(e, "expires");
	if ( expires && !yyjson_is_null(expires) && ban->expires <= ban->date )
		return false;
	ban->guid = (int)guid;
	strcpy(ban->id, yyjson_get_str(id));
	return true;
}

// reads the file when it changed; one that is missing is an empty list, one
// that does not parse or cannot be stat'ed keeps the last good list and is logged once
static void PlusId_LoadBans()
{
	char path[MAX_OSPATH * 2];
	long long size = -1, mtime = 0;

	PlusId_Path("plusid_bans.json", path, sizeof(path));
	int found = PlusId_Stat(path, &size, &mtime);
	if ( found < 0 )
	{
		// -2: the size of a file that cannot be stat'ed, so that it is logged once
		if ( bansSize != -2 )
			Com_Printf("plusid: cannot read %s (%s), keeping the bans in memory\n", path, strerror(errno));
		bansSize = -2;
		bansBad = true;
		return;
	}
	if ( size == bansSize && mtime == bansTime )
		return;
	bansSize = size;
	bansTime = mtime;
	if ( !found )
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
		PlusId_Stat(path, &bansSize, &bansTime);
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
