#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
extern clipMap_t cm;

extern const char *Com_Parse(const char **buf);
extern int stricmp(const char *s1, const char *s2);
extern struct XModel * CM_XModelPrecache(const char *name);
extern qboolean Com_ValidXModelName(const char *name);
extern void AnglesToAxis(const vec_t *angles, vec3_t *axis);
extern void MatrixInverse(float *src, float *dst);
extern int XModelGetStaticBounds(const XModel *model, vec3_t *axis, vec_t *outMins, vec_t *outMaxs);
extern void Com_Error(int level, const char *fmt, ...);
extern void *CM_Hunk_Alloc(int size, const char *name, int align);
extern int sscanf(const char *str, const char *fmt, ...);
extern void I_strncpyz(char *dest, const char *src, int destsize);

void CM_LoadStaticModels(void)
{
    const char *ptr = cm.entityString;

    cm.numStaticModels = 0;
    cm.staticModelList = NULL;

    char modelName[64];
    char key[64];
    char value[64];
    int bMiscModel;

    for (;;) {
        const char *token = Com_Parse(&ptr);
        if (ptr == NULL)
            break;
        if (*token != '{')
            break;

        modelName[0] = '\0';
        bMiscModel = 0;

        for (;;) {
            token = Com_Parse(&ptr);
            if (ptr == NULL || *token == '}')
                break;

            I_strncpyz(key, token, sizeof(key));

            token = Com_Parse(&ptr);
            if (ptr == NULL)
                break;

            I_strncpyz(value, token, sizeof(value));

            if (stricmp(key, "classname") == 0) {
                if (stricmp(value, "misc_model") == 0) {
                    bMiscModel = 1;
                }
            } else if (stricmp(key, "model") == 0) {
                I_strncpyz(modelName, value, sizeof(modelName));
            }
        }

        if (!bMiscModel)
            continue;

        if (!Com_ValidXModelName(modelName))
            continue;

        cm.numStaticModels += 1;
    }

    int numStaticModels = cm.numStaticModels;
    if (numStaticModels == 0)
        return;

#if defined(__x86_64__) || defined(_M_X64)

    cm.staticModelList = (cStaticModel_t *)CM_Hunk_Alloc(numStaticModels * (int)sizeof(cStaticModel_t), "CM_CreateStaticModel", 0x19);
#else
    cm.staticModelList = (cStaticModel_t *)CM_Hunk_Alloc(numStaticModels * 80, "CM_CreateStaticModel", 0x19);
#endif

    ptr = cm.entityString;
    int modelIdx = 0;

    for (;;) {
        const char *token = Com_Parse(&ptr);
        if (ptr == NULL)
            return;
        if (*token != '{')
            return;

        modelName[0] = '\0';
        vec3_t origin = { 0, 0, 0 };
        vec3_t angles = { 0, 0, 0 };
        vec3_t scale = { 1.0f, 1.0f, 1.0f };
        int found = 0;

        for (;;) {
            token = Com_Parse(&ptr);
            if (ptr == NULL || *token == '}')
                break;

            I_strncpyz(key, token, sizeof(key));

            token = Com_Parse(&ptr);
            if (ptr == NULL)
                break;

            I_strncpyz(value, token, sizeof(value));

            if (stricmp(key, "classname") == 0) {
                if (stricmp(value, "misc_model") == 0) {
                    found = 1;
                }
            } else if (stricmp(key, "model") == 0) {
                I_strncpyz(modelName, value, sizeof(modelName));
            } else if (stricmp(key, "origin") == 0) {
                sscanf(value, "%f %f %f", &origin[0], &origin[1], &origin[2]);
            } else if (stricmp(key, "angles") == 0) {
                sscanf(value, "%f %f %f", &angles[0], &angles[1], &angles[2]);
            } else if (stricmp(key, "modelscale_vec") == 0) {
                sscanf(value, "%f %f %f", &scale[0], &scale[1], &scale[2]);
            } else if (stricmp(key, "modelscale") == 0) {
                float s = (float)atof(value);
                scale[0] = s;
                scale[1] = s;
                scale[2] = s;
            }
        }

        if (!found)
            continue;

        if (!Com_ValidXModelName(modelName))
            continue;

        cStaticModel_t *staticModel = &cm.staticModelList[modelIdx];

        if (modelName[7] == '\0') {
            Com_Error(1, "CM_LoadStaticModels: empty model name");
        }

        if (scale[0] == 0.0f) {
            Com_Error(1, "Static model [%s] has x scale of 0.0\n", modelName + 7);
        }
        if (scale[1] == 0.0f) {
            Com_Error(1, "Static model [%s] has y scale of 0.0\n", modelName + 7);
        }
        if (scale[2] == 0.0f) {
            Com_Error(1, "Static model [%s] has z scale of 0.0\n", modelName + 7);
        }

        void *model = CM_XModelPrecache(modelName + 7);
        if (model == NULL) {
            cm.numStaticModels -= 1;
            continue;
        }

        staticModel->xmodel = (struct XModel *)model;

        staticModel->origin[0] = origin[0];
        staticModel->origin[1] = origin[1];
        staticModel->origin[2] = origin[2];

        float axis[9];
        AnglesToAxis(angles, (vec3_t *)axis);

        axis[0] *= scale[0];
        axis[1] *= scale[0];
        axis[2] *= scale[0];

        axis[3] *= scale[1];
        axis[4] *= scale[1];
        axis[5] *= scale[1];

        axis[6] *= scale[2];
        axis[7] *= scale[2];
        axis[8] *= scale[2];

        MatrixInverse(axis, (float *)staticModel->invAxis);

        if (XModelGetStaticBounds((const XModel *)model, (vec3_t *)axis, staticModel->absmin, staticModel->absmax)) {

            staticModel->absmin[0] += origin[0];
            staticModel->absmin[1] += origin[1];
            staticModel->absmin[2] += origin[2];
            staticModel->absmax[0] += origin[0];
            staticModel->absmax[1] += origin[1];
            staticModel->absmax[2] += origin[2];
        }

        modelIdx++;
    }
}
