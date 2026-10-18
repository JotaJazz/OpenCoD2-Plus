#ifndef PC_XANIM_XANIM_LOCAL_H_H
#define PC_XANIM_XANIM_LOCAL_H_H

#ifdef __cplusplus
extern "C" {
#endif

struct XBoneHierarchy;
struct XModelConfig;
struct XModelConfigEntry;
struct XModelLodInfo;

struct XBoneHierarchy {
    short unsigned int *names;
    unsigned char parentList[1];
};

struct XModelConfig {
    XModelConfigEntry entries[4];
    float mins[3];
    float maxs[3];
    int collLod;
    unsigned char flags;
};

struct XModelConfigEntry {
    char filename[1024];
    float dist;
};

struct XModelLodInfo {
    float dist;
    const char *filename;
    short int numsurfs;
    short unsigned int *surfNames;
    XModelSurfs *surfs;
};

#ifdef __cplusplus
}
#endif
#endif
