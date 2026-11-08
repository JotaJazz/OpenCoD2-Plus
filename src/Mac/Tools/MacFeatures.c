#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common_types.h"
#include "imports.h"

SInt16 MacFeatures_GetSystemVersion(void)
{

    return 0x1040;
}

Boolean MacFeatures_HasGestaltAttribute(OSType inSelector, UInt32 inAttribute)
{

    if (inSelector == 0x78383666 && inAttribute == 0x19) {
        return 1;
    }
    return 0;
}

Boolean MacFeatures_IsAltiVecAvailable(UInt8 *outMajor, UInt8 *outMinor, UInt8 *outBug)
{

    return 0;
}

/* the CPU's speed in MHz: its rated maximum (cpufreq's cpuinfo_max_freq, in
 * kHz), steady from run to run as Windows' ~MHz; else cpuinfo's "cpu MHz",
 * the current clock, which moves more than the 10% Sys_HasInfoChanged
 * allows; else 800 */
float MacFeatures_CPUMHzFrom(const char *maxFreqPath, const char *cpuinfoPath)
{
    FILE *f = fopen(maxFreqPath, "r");
    char line[256];
    float mhz = 800.0f;

    if (f) {
        unsigned long khz = 0;
        int got = fscanf(f, "%lu", &khz);

        fclose(f);
        if (got == 1 && khz > 0)
            return (float)khz / 1000.0f;
    }

    f = fopen(cpuinfoPath, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "cpu MHz", 7) == 0) {
                char *p = strchr(line, ':');
                if (p) {
                    mhz = (float)atof(p + 1);
                }
                break;
            }
        }
        fclose(f);
    }
    return mhz;
}

float MacFeatures_GetCPUSpeedInGHz(void)
{
#ifdef _WIN32
    extern unsigned int Win_CpuMHz(void);
    unsigned int winMHz = Win_CpuMHz();

    if (winMHz)
        return (float)winMHz / 1000.0f;
#endif
    return MacFeatures_CPUMHzFrom("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq",
                                  "/proc/cpuinfo") / 1000.0f;
}

UInt32 MacFeatures_GetMemorySizeInMB(void)
{
#ifdef _WIN32
    extern unsigned int Win_MemoryMB(void);
    unsigned int winMB = Win_MemoryMB();

    if (winMB)
        return (UInt32)winMB;
#endif
    FILE *f = fopen("/proc/meminfo", "r");
    char line[256];
    unsigned long kb = 128 * 1024;

    if (f) {
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                kb = strtoul(line + 9, NULL, 10);
                break;
            }
        }
        fclose(f);
    }
    return (UInt32)(kb / 1024);
}
