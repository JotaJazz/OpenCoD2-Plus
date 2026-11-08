#pragma once

// The Plus server serving its own mod downloads over HTTP (sv_wwwBaseURL auto).
// docs/architecture/plus-protocol.md, Downloads.

#if defined(COD2_PLUS) && !defined(_WIN32)

#include <stdint.h>

#define WWW_PLUS_GRANT_MS 300000 // a grant lets a request start for 5 minutes

// sv_wwwBaseURL is "auto"
bool SV_WwwPlus_IsAuto(void);
// Lets adr's IP fetch downloadName; false if the file has no path or the table is full
bool SV_WwwPlus_Grant(netadr_t adr, const char *downloadName, int size);
// For the worker: a live grant of this IP and exactly this name
bool SV_WwwPlus_FindGrant(uint32_t ip, const char *name, char *ospath, int ospathSize, int *size);
// The listener on net_ip:the game port and its worker thread; Start only with sv_wwwBaseURL auto
void SV_WwwPlus_Start(void);
void SV_WwwPlus_Stop(void);
// Copies the limits for the worker and prints its log lines
void SV_WwwPlus_Frame(void);

#endif
