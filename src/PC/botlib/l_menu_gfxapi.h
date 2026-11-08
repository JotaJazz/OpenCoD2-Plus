#ifndef L_MENU_GFXAPI_H
#define L_MENU_GFXAPI_H

#include <string.h>

/* The game's "Rendering Method Preference" item picks r_gfxApi: the dvar it
 * sets means nothing to this client, and a pure server would refuse a menu in
 * an iwd of ours. In place, the rest padded with spaces; a menu that differs
 * from the game's stays as it is. */
static inline void Script_PatchGraphicsMenu(char *buffer)
{
    static const char oldDvar[] = "dvar \"ui_r_rendererpreference\"";
    static const char newDvar[] = "dvar \"r_gfxApi\"";
    static const char list[] = "dvarStrList";
    static const char newList[] = "dvarStrList { \"Direct3D 9\", \"d3d9\", \"Vulkan\", \"vulkan\" }";
    char *dvar = strstr(buffer, oldDvar);
    char *start = dvar ? strstr(dvar, list) : NULL;
    char *end = start ? strchr(start, '}') : NULL;
    char *next = dvar ? strstr(dvar, "itemDef") : NULL;

    /* the list must be this item's */
    if (!end || (next && next < start) || (size_t)(end + 1 - start) < sizeof(newList) - 1)
        return;
    memset(dvar, ' ', sizeof(oldDvar) - 1);
    memcpy(dvar, newDvar, sizeof(newDvar) - 1);
    memset(start, ' ', (size_t)(end + 1 - start));
    memcpy(start, newList, sizeof(newList) - 1);
}

#endif
