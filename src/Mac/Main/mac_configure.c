#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common_types.h"
#include "imports.h"

extern float MacFeatures_GetCPUSpeedInGHz(void);
extern UInt32 MacFeatures_GetMemorySizeInMB(void);

double Sys_CpuGHz(void)
{
    return (double)MacFeatures_GetCPUSpeedInGHz();
}

int Sys_SystemMemoryMB(void)
{
    int mb = (int)MacFeatures_GetMemorySizeInMB();
    if (mb >= 0x8000001)
        mb = 0x8000000;
    return mb;
}

void Sys_DetectVideoCard(int descLimit, char *description)
{
#ifdef _WIN32
    extern int Win_DisplayDeviceName(char *name, int size);

    if (Win_DisplayDeviceName(description, descLimit))
        return;
#endif

    const char *renderer = "Linux OpenGL";
    strncpy(description, renderer, descLimit);
    description[descLimit - 1] = '\0';
    return;
}

bool Sys_SupportsSSE(void)
{
#ifdef BYTE_MATCH_DARWIN
    extern Boolean MacFeatures_HasGestaltAttribute(OSType inSelector, UInt32 inAttribute);
    return MacFeatures_HasGestaltAttribute(0x78383666, 25) != 0;
#else

    return 1;
#endif
}
