#ifndef GFX_DLL_V60_H
#define GFX_DLL_V60_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"

#if defined(COD2_GFX_DLL) && defined(COD2_GFX_DLL_RETAIL_V60)

void gfxV60_LoadRetailRenderer(refimport_t *ri_local, refexport_t *out_re);
#endif

#ifdef __cplusplus
}
#endif
#endif
