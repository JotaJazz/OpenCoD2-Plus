/* What the machine has, from Windows itself: the engine's Mac/Linux code
 * reads /proc, which Windows lacks (it then took 128 MB and 0.8 GHz, which
 * lowered the texture detail). Apart from the engine's headers, which clash
 * with windows.h. */
#include <windows.h>
#include <string.h>

/* the processor's speed in MHz, 0 when unknown */
unsigned int Win_CpuMHz(void)
{
    HKEY key;
    DWORD mhz = 0, size = sizeof(mhz);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0,
                      KEY_READ, &key) == ERROR_SUCCESS) {
        if (RegQueryValueExA(key, "~MHz", NULL, NULL, (LPBYTE)&mhz, &size) != ERROR_SUCCESS)
            mhz = 0;
        RegCloseKey(key);
    }
    return mhz;
}

/* the physical memory in MB, 0 when unknown */
unsigned int Win_MemoryMB(void)
{
    MEMORYSTATUSEX status;

    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status))
        return 0;
    return (unsigned int)(status.ullTotalPhys >> 20);
}

/* the primary display device's name, as the Windows game reports the card;
 * 0 when there is none */
int Win_DisplayDeviceName(char *name, int size)
{
    DISPLAY_DEVICEA device;
    DWORD i;

    for (i = 0;; i++) {
        memset(&device, 0, sizeof(device));
        device.cb = sizeof(device);
        if (!EnumDisplayDevicesA(NULL, i, &device, 0))
            return 0;
        if (device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
            strncpy(name, device.DeviceString, size);
            name[size - 1] = '\0';
            return 1;
        }
    }
}
