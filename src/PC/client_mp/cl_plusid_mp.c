/* The Plus ID: the client's ECDSA P-256 key in fs_homepath/plusid.key, which
 * signs the connect challenge so that OpenCoD2-Plus Server knows the player
 * (docs/architecture/plus-id.md). The key never leaves this file but into
 * plusid.key. Not in the web build. */
#include "common_types.h"
#include <stdio.h>
#include <string.h>

void CL_PlusId_Init(void);
void CL_PlusId_AddToInfo(char *info, int size, int challenge, netadr_t server);

#ifdef __EMSCRIPTEN__

void CL_PlusId_Init(void)
{
}

void CL_PlusId_AddToInfo(char *info, int size, int challenge, netadr_t server)
{
}

#else

#include <errno.h>
#ifdef _WIN32
#include <io.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "../../libcod/plusid.hpp"

extern void Com_Printf(const char *fmt, ...);
extern void Cmd_AddCommand(const char *cmdName, void (*function)(void));
extern const char *Dvar_GetString(const char *dvarName);
extern void Info_SetValueForKey(char *s, const char *key, const char *value);
extern qboolean FS_CreatePath(char *OSPath);

static unsigned char plusIdKey[32];
static char plusId[22]; /* empty without a usable key */
static char plusIdPath[256];
static int plusIdWarned; /* the challenge whose userinfo was too long, warned once */

/* takes priv as the key when it is one; its ID from a test signature */
static bool CL_PlusId_SetKey(const unsigned char priv[32])
{
    char pkHex[PLUSID_PK_HEX + 1], psHex[PLUSID_PS_HEX + 1];
    unsigned char pk[33];

    if (!PlusId_Sign(priv, 0, "", pkHex, psHex) || !PlusId_FromHex(pkHex, pk, sizeof(pk))
        || !PlusId_FromPublicKey(pk, plusId)) {
        plusId[0] = '\0';
        return false;
    }
    memcpy(plusIdKey, priv, sizeof(plusIdKey));
    return true;
}

/* 1 when the file holds a key, 0 when there is no file, -1 when it is unreadable */
static int CL_PlusId_Read(void)
{
    char text[80];
    unsigned char priv[32];
    FILE *f = fopen(plusIdPath, "rb");
    size_t n;
    bool ok;

    if (!f)
        return errno == ENOENT ? 0 : -1;
    n = fread(text, 1, sizeof(text) - 1, f);
    fclose(f);
    text[n] = '\0';
    text[strcspn(text, "\r\n")] = '\0';
    ok = PlusId_FromHex(text, priv, sizeof(priv)) && CL_PlusId_SetKey(priv);
    memset(text, 0, sizeof(text));
    memset(priv, 0, sizeof(priv));
    return ok ? 1 : -1;
}

/* plusid.key, only when it does not exist: written to .tmp, synced, then
 * linked (POSIX) or renamed (Windows), neither of which replaces a file */
static bool CL_PlusId_Write(const unsigned char priv[32])
{
    char tmp[sizeof(plusIdPath) + 4], text[65];
    FILE *f;
    bool ok;

    snprintf(tmp, sizeof(tmp), "%s.tmp", plusIdPath);
    FS_CreatePath(tmp);
    remove(tmp);
#ifdef _WIN32
    f = fopen(tmp, "wb");
#else
    {
        int fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL, 0600); /* a private key: its owner only */
        f = fd >= 0 ? fdopen(fd, "wb") : NULL;
    }
#endif
    if (!f)
        return false;
    PlusId_ToHex(priv, 32, text);
    text[64] = '\n';
    ok = fwrite(text, 1, sizeof(text), f) == sizeof(text) && fflush(f) == 0;
#ifdef _WIN32
    ok = ok && _commit(_fileno(f)) == 0;
#else
    ok = ok && fsync(fileno(f)) == 0;
#endif
    ok = fclose(f) == 0 && ok;
    memset(text, 0, sizeof(text));
#ifdef _WIN32
    ok = ok && rename(tmp, plusIdPath) == 0;
#else
    if (ok && link(tmp, plusIdPath) != 0)
    {
        /* vfat, exFAT and some FUSE homes have no hard links: rename, only
         * while there is no key. shortcut: another client creating it between
         * the stat and the rename loses its key to this one; both just started */
        struct stat st;

        ok = (errno == EPERM || errno == ENOTSUP || errno == ENOSYS || errno == EXDEV)
             && stat(plusIdPath, &st) != 0 && errno == ENOENT && rename(tmp, plusIdPath) == 0;
    }
#endif
    remove(tmp);
    return ok;
}

static void CL_PlusId_f(void)
{
    if (plusId[0])
        Com_Printf("Plus ID: %s (key: %s; back it up to keep your playtime)\n", plusId, plusIdPath);
    else
        Com_Printf("No Plus ID (key: %s)\n", plusIdPath);
}

void CL_PlusId_Init(void)
{
    unsigned char priv[32];
    int state;

    Cmd_AddCommand("plusid", CL_PlusId_f);
    snprintf(plusIdPath, sizeof(plusIdPath), "%s/plusid.key", Dvar_GetString("fs_homepath"));
    state = CL_PlusId_Read();
    if (state == 0) {
        /* when the write fails, another client may just have made the file */
        if (!PlusId_Generate(priv) || !CL_PlusId_SetKey(priv) || !CL_PlusId_Write(priv))
            state = CL_PlusId_Read();
        else
            state = 1;
        memset(priv, 0, sizeof(priv));
        if (state != 1) {
            plusId[0] = '\0';
            Com_Printf("plusid: cannot write %s, playing without a Plus ID\n", plusIdPath);
        }
    } else if (state < 0) {
        Com_Printf("plusid: plusid.key is unreadable, playing without a Plus ID\n");
    }
}

void CL_PlusId_AddToInfo(char *info, int size, int challenge, netadr_t server)
{
    char adr[64], pkHex[PLUSID_PK_HEX + 1], psHex[PLUSID_PS_HEX + 1];

    if (!plusId[0] || server.type != NA_IP)
        return;
    /* as the server's NET_AdrToString writes it: the engine's prints a port over 32767 negative */
    snprintf(adr, sizeof(adr), "%i.%i.%i.%i:%u", server.ip[0], server.ip[1], server.ip[2], server.ip[3],
             (unsigned)(unsigned short)((server.port >> 8) | (server.port << 8)));
    if ((int)(strlen(info) + 4 + PLUSID_PK_HEX + 4 + PLUSID_PS_HEX + 4 + strlen(adr)) > size - 1) {
        if (plusIdWarned != challenge)
            Com_Printf("plusid: userinfo too long, connecting without a Plus ID\n");
        plusIdWarned = challenge;
        return;
    }
    if (!PlusId_Sign(plusIdKey, challenge, adr, pkHex, psHex))
        return;
    Info_SetValueForKey(info, "pk", pkHex);
    Info_SetValueForKey(info, "ps", psHex);
    Info_SetValueForKey(info, "pa", adr);
}

#endif
