#include "cod2_feature_config.h"

#if COD2_FEATURE_NET_HARDENING

#    include "common_types.h"
#    include <string.h>

extern void Com_Printf(const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);

extern const dvar_t *Dvar_FindVar(const char *dvarName);
extern int I_stricmp(const char *s1, const char *s2);
extern int I_strnicmp(const char *s1, const char *s2, size_t n);

#    define NH_DVAR_FLAG_SYSTEMINFO 0x08
#    define NH_DVAR_FLAG_CODINFO 0x100
#    define NH_DVAR_FLAG_READONLY 0x40
#    define NH_DVAR_FLAG_WRITEPROTECTED 0x10

static int NetHardening_HasBadSubstring(const char *name)
{
    const char *p;

    for (p = name; *p; ++p) {
        if (p[0] == '.' && p[1] == '.')
            return 1;
    }

    for (p = name; *p; ++p) {
        if (*p == '~')
            return 1;
    }

    return 0;
}

int NetHardening_ValidDownloadName(const char *name, int require_iwd_ext)
{
    size_t len;
    const char *p;

    if (!name || name[0] == '\0')
        return 0;

    if (name[0] == '/' || name[0] == '\\')
        return 0;

    for (p = name; *p; ++p) {
        if (*p == ':')
            return 0;

        if ((unsigned char)*p < 0x20)
            return 0;
    }

    if (NetHardening_HasBadSubstring(name))
        return 0;

    if (require_iwd_ext) {
        len = strlen(name);
        if (len >= 4 && (strcmp(name + len - 4, ".iwd") == 0 ||
                         strcmp(name + len - 4, ".pk3") == 0))
            return 1;
        if (len >= 5 && strcmp(name + len - 5, ".dm_1") == 0)
            return 1;
        return 0;
    }

    return 1;
}

int NetHardening_ValidUrl(const char *url)
{
    if (!url || url[0] == '\0')
        return 0;

    if (strncmp(url, "http://", 7) == 0)
        return 1;
    if (strncmp(url, "https://", 8) == 0)
        return 1;

    return 0;
}

/* Not in 1.3, which lets a server set any dvar: these run a command or decide
 * what the client trusts. */
int NetHardening_ServerMaySetCvar(const char *key)
{
    static const char *const names[] = {"com_writeConfig", "activeAction", "nextdemo", "cl_allowDownload", "cl_wwwDownload"};
    static const char *const prefixes[] = {"fs_", "sv_", "net_", "rcon", "in_"};
    const dvar_t *dvar;
    unsigned int flags;
    size_t i;

    if (!key || key[0] == '\0')
        return 0;

    dvar = Dvar_FindVar(key);
    flags = dvar ? (unsigned int)dvar->flags : 0;

    /* what a server sends every client: fs_game, sv_pure, sv_serverid..., and
     * the physics prediction needs, some of them read-only for the player */
    if (flags & (NH_DVAR_FLAG_SYSTEMINFO | NH_DVAR_FLAG_CODINFO))
        return 1;

    for (i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        if (I_stricmp(key, names[i]) == 0)
            return 0;
    }
    for (i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i) {
        if (I_strnicmp(key, prefixes[i], strlen(prefixes[i])) == 0)
            return 0;
    }

    return !(flags & (NH_DVAR_FLAG_READONLY | NH_DVAR_FLAG_WRITEPROTECTED));
}

typedef struct {
    int lastTimeMsec;
    float tokens;
    byte ip[4];
    byte inUse;
} nh_bucket_t;

#    define NH_ADDR_BUCKETS 256

static nh_bucket_t nh_addrBuckets[NH_ADDR_BUCKETS];
static int nh_globalLastMsec;
static float nh_globalTokens;
static int nh_initialized;

#    define NH_ADDR_PERIOD_MSEC 100
#    define NH_ADDR_BURST 8.0f

#    define NH_GLOBAL_PERIOD_MSEC 20
#    define NH_GLOBAL_BURST 100.0f

static void NetHardening_RateInit(int nowMsec)
{
    int i;
    for (i = 0; i < NH_ADDR_BUCKETS; ++i) {
        nh_addrBuckets[i].inUse = 0;
        nh_addrBuckets[i].tokens = 0.0f;
        nh_addrBuckets[i].lastTimeMsec = nowMsec;
    }
    nh_globalLastMsec = nowMsec;
    nh_globalTokens = NH_GLOBAL_BURST;
    nh_initialized = 1;
}

static int NetHardening_BucketTake(float *tokens, int *lastMsec, int nowMsec,
                                   int periodMsec, float burst)
{
    int elapsed = nowMsec - *lastMsec;
    float refill;

    if (elapsed < 0)
        elapsed = periodMsec;

    refill = (float)elapsed / (float)periodMsec;
    *tokens += refill;
    if (*tokens > burst)
        *tokens = burst;
    *lastMsec = nowMsec;

    if (*tokens >= 1.0f) {
        *tokens -= 1.0f;
        return 1;
    }
    return 0;
}

static nh_bucket_t *NetHardening_FindAddrBucket(const byte ip[4], int nowMsec)
{
    unsigned int hash;
    int i, slot;
    nh_bucket_t *oldest = 0;
    int oldestTime = 0;

    hash = (unsigned int)ip[0] * 2654435761u;
    hash ^= (unsigned int)ip[1] * 40503u;
    hash ^= (unsigned int)ip[2] * 2246822519u;
    hash ^= (unsigned int)ip[3] * 3266489917u;
    slot = (int)(hash % NH_ADDR_BUCKETS);

    for (i = 0; i < 8; ++i) {
        int idx = (slot + i) & (NH_ADDR_BUCKETS - 1);
        nh_bucket_t *b = &nh_addrBuckets[idx];

        if (b->inUse &&
            b->ip[0] == ip[0] && b->ip[1] == ip[1] &&
            b->ip[2] == ip[2] && b->ip[3] == ip[3])
            return b;

        if (!b->inUse)
            return b;

        if (oldest == 0 || b->lastTimeMsec < oldestTime) {
            oldest = b;
            oldestTime = b->lastTimeMsec;
        }
    }

    return oldest ? oldest : &nh_addrBuckets[slot];
}

int NetHardening_SVC_RateLimit(int nowMsec)
{
    if (!nh_initialized)
        NetHardening_RateInit(nowMsec);

    if (NetHardening_BucketTake(&nh_globalTokens, &nh_globalLastMsec, nowMsec,
                                NH_GLOBAL_PERIOD_MSEC, NH_GLOBAL_BURST))
        return 0;

    return 1;
}

int NetHardening_SVC_RateLimitAddress(netadr_t from, int nowMsec)
{
    nh_bucket_t *b;

    if (from.type != NA_IP)
        return 0;

    if (!nh_initialized)
        NetHardening_RateInit(nowMsec);

    b = NetHardening_FindAddrBucket(from.ip, nowMsec);

    if (!b->inUse ||
        b->ip[0] != from.ip[0] || b->ip[1] != from.ip[1] ||
        b->ip[2] != from.ip[2] || b->ip[3] != from.ip[3]) {

        b->inUse = 1;
        b->ip[0] = from.ip[0];
        b->ip[1] = from.ip[1];
        b->ip[2] = from.ip[2];
        b->ip[3] = from.ip[3];
        b->tokens = NH_ADDR_BURST;
        b->lastTimeMsec = nowMsec;
    }

    if (NetHardening_BucketTake(&b->tokens, &b->lastTimeMsec, nowMsec,
                                NH_ADDR_PERIOD_MSEC, NH_ADDR_BURST))
        return 0;

    return 1;
}

#else

typedef int net_hardening_translation_unit_not_empty;

#endif
