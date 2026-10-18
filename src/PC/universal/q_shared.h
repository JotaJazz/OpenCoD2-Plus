#ifndef PC_UNIVERSAL_Q_SHARED_H_H
#define PC_UNIVERSAL_Q_SHARED_H_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Mac/DirectX 9/MacOpenGLUtils.h"

struct ping_t;
struct qtime_s;
struct serverStatus_s;
struct weaponParms;

struct ping_t {
    netadr_t adr;
    int start;
    int time;
    char info[1024];
};

struct qtime_s {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

struct serverStatus_s {
    pinglist_t pingList[16];
    int numqueriedservers;
    int currentping;
    int nextpingtime;
    int maxservers;
    int refreshtime;
    int numServers;
    int sortKey;
    int sortDir;
    int lastCount;
    qboolean refreshActive;
    int currentServer;
    int displayServers[20000];
    int numDisplayServers;
    int serverCount;
    int numPlayersOnServers;
    int nextDisplayRefresh;
    int nextSortTime;
    MaterialHandle currentServerPreview;
    int currentServerCinematic;
    int motdLen;
    int motdWidth;
    int motdPaintX;
    int motdPaintX2;
    int motdOffset;
    int motdTime;
    char motd[1024];
};

struct weaponParms {
    vec3_t forward;
    vec3_t right;
    vec3_t up;
    vec3_t muzzleTrace;
    vec3_t gunForward;
    WeaponDef *weapDef;
};

#ifdef __cplusplus
}
#endif
#endif
