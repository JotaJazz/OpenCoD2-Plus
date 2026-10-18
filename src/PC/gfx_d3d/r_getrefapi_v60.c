#include "common_types.h"
#include <stdlib.h>


#    include "PC/client_mp/gfx_dll_v60_map.h"
#    include "PC/gfx_d3d/r_xmodelsurfs_load.h"

extern refexport_t *GetRefAPI(int apiVersion, refimport_t *rimp);
extern refimport_t ri;

static int v60_noop(void)
{
    return 0;
}

extern void XModelOptimize(XModel *xmodel);
static void *gfxv60_XModelRegister(void *xmodel)
{
    if (xmodel)
        XModelOptimize((XModel *)xmodel);
    return xmodel;
}

static void *gfxv60_XSurfHandle(void *surf, int a, int b, int c)
{
    (void)a;
    (void)b;
    (void)c;
    return surf;
}

static void v60_diag(int slot)
{
    ((void (*)(int, const char *, ...))(*(void **)&ri))(
        0, "[v60-re] engine called UNFILLED refexport slot %d\n", slot);
}
#    define DIAG(n)              \
        static int v60t##n(void) \
        {                        \
            v60_diag(n);         \
            return 0;            \
        }
DIAG(22)
DIAG(23) DIAG(24) DIAG(26) DIAG(27) DIAG(87) DIAG(88) DIAG(89)
    DIAG(90) DIAG(91) DIAG(92) DIAG(93) DIAG(94) DIAG(95) DIAG(96) DIAG(97)
        DIAG(98) DIAG(99) DIAG(100) DIAG(101) DIAG(102)
#    undef DIAG

            refexport_t *GetRefAPI_v60_adapt(refimport_t *rimp_v60)
{
    static refimport_t ri_v59;
    static unsigned char re_v60[2048];  /* v60 export slots; 8-byte slots on x64 (768 was x86-only) */
    refexport_t *re59;
    const intptr_t *src;
    intptr_t *dst;
    int i, k;

    memset(&ri_v59, 0, sizeof(ri_v59));
    src = (const intptr_t *)rimp_v60;
    dst = (intptr_t *)&ri_v59;
    for (i = 0; i < 181; i++)
        if (kV60ToV59[i] >= 0)
            dst[kV60ToV59[i]] = src[i];

    dst[18] = src[21];
    dst[21] = src[22];
    dst[107] = src[111];
    dst[135] = src[159];

    /* Sun/dvar members used at map load (v60 index -> v59 index; a clean +2).
     * With these bridged, R_LoadWorld pushes the BSP sun into the r_lightTweak* dvars
     * and the optional sun-dvar file loads correctly. */
    dst[39] = src[41];   /* Dvar_SetFloat */
    dst[41] = src[43];   /* Dvar_SetColor */
    dst[43] = src[45];   /* Dvar_SetVec3 */
    dst[68] = src[70];   /* Com_LoadDvarsFromBuffer */

    re59 = GetRefAPI(59, &ri_v59);
    if (!re59)
        return (refexport_t *)0;

    {
#    include "PC/client_mp/gfx_dll_v60_re_map.h"
        memset(re_v60, 0, sizeof(re_v60));
        src = (const intptr_t *)re59;
        dst = (intptr_t *)re_v60;
        for (k = 0; k < (int)(sizeof(kReV60) / sizeof(kReV60[0])); k++)
            dst[(kReV60[k] >= 0) ? kReV60[k] : k] = src[k];

        dst[81] = src[75];

        {
            extern GfxEntity *R_AddRefEntityToScene(const GfxEntity *, GfxModel,
                                                    const struct centity_s *);
            dst[20] = (intptr_t)R_AddRefEntityToScene;
        }

        R_XModelSurfs_SetRefImport(rimp_v60);
        dst[97] = (intptr_t)R_LoadXModelSurfsSurface;
        dst[98] = (intptr_t)R_RegisterXModelSurfs;
        (void)gfxv60_XSurfHandle;
        (void)gfxv60_XModelRegister;

        dst[24] = (intptr_t)v60_noop;

        {
            const intptr_t *rimp = (const intptr_t *)rimp_v60;
            dst[22] = rimp[178];
            dst[23] = rimp[177];
        }

        {
            const intptr_t *re59arr = (const intptr_t *)re59;
            dst[26] = re59arr[20];
            dst[27] = re59arr[21];
        }

    }
    return (refexport_t *)re_v60;
}
