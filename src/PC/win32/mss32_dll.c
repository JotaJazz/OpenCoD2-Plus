/* Miles Sound System for the Windows client: the game's own mss32.dll.
 *
 * snd_driver.c is the Windows binary's sound driver, written against Miles
 * (AIL_*). The Mac port's MacMSS.c answers those calls with its own engine,
 * which these builds do not have, so sound never started ("Miles sound system
 * initialization failed"). On Windows the game ships mss32.dll (and its
 * providers in miles/): each AIL_ function here calls the DLL's, resolved by
 * its __stdcall name (_AIL_name@N, N checked against the DLL's exports when
 * this file was written) on first use. The DLL is loaded from fs_basepath,
 * where the game is, or else from the usual search path.
 *
 * Two functions snd_driver.c calls are the Mac port's and not Miles's: 3D
 * streams, which the Windows binary does not have (it plays streams with
 * volume and pan). And the Windows binary gives Miles the game's window for
 * DirectSound (AIL_set_DirectSound_HWND, from its window procedure), which
 * AIL_open_digital_driver does here.
 */
#if defined(_WIN32) && defined(COD2_MSS32_DLL)
#include "common_types.h"
#include "imports.h"
#include <stdio.h>
#include <string.h>

extern void *__stdcall LoadLibraryA(const char *name);
extern void *__stdcall GetProcAddress(void *module, const char *name);
extern const char *Dvar_GetString(const char *dvarName);
extern void Com_Printf(const char *fmt, ...);
extern void *Sys_GetGameWindow(void);

static void *mss_module;
static int mss_loadTried;

static void *MSS_Load(void)
{
    char path[512];
    const char *base;

    if (mss_loadTried)
        return mss_module;
    mss_loadTried = 1;
    base = Dvar_GetString("fs_basepath");
    if (base && base[0]) {
        snprintf(path, sizeof(path), "%s\\mss32.dll", base);
        mss_module = LoadLibraryA(path);
    }
    if (!mss_module)
        mss_module = LoadLibraryA("mss32.dll");
    if (!mss_module)
        Com_Printf("Miles: could not load mss32.dll from %s\n", base ? base : "");
    return mss_module;
}

static void *MSS_Get(const char *decorated)
{
    void *module = MSS_Load();
    void *fn = module ? GetProcAddress(module, decorated) : NULL;
    if (module && !fn)
        Com_Printf("Miles: mss32.dll has no %s\n", decorated);
    return fn;
}

/* the DLL's function, looked up once */
#define MSS_FN(name, size)     ({ static void *fn_##name; static int got_##name;        if (!got_##name) { fn_##name = MSS_Get("_" #name "@" #size); got_##name = 1; }        fn_##name; })

/* "miles" is the game's folder's, not the working directory's */
static const char *MSS_GamePath(const char *dir)
{
    static char path[512];
    const char *base = Dvar_GetString("fs_basepath");
    if (!dir || !base || !base[0] || dir[0] == '\\' || dir[0] == '/' || (dir[0] && dir[1] == ':'))
        return dir;
    snprintf(path, sizeof(path), "%s\\%s", base, dir);
    return path;
}

static void MSS_SetWindow(HDIGDRIVER dig)
{
    void *fn;
    void *hwnd;

    if (!dig)
        return;
    hwnd = Sys_GetGameWindow();
    fn = MSS_FN(AIL_set_DirectSound_HWND, 8);
    if (fn && hwnd)
        ((void (__stdcall *)(HDIGDRIVER, void *))fn)(dig, hwnd);
}

void AIL_3D_position(H3DPOBJECT obj, float *X, float *Y, float *Z)
{
    void *fn = MSS_FN(AIL_3D_position, 16);
    if (fn)
        ((void (__stdcall *)(H3DPOBJECT obj, float *X, float *Y, float *Z))fn)(obj, X, Y, Z);
}

void AIL_3D_provider_attribute(HPROVIDER lib, const char *name, void *val)
{
    void *fn = MSS_FN(AIL_3D_provider_attribute, 12);
    if (fn)
        ((void (__stdcall *)(HPROVIDER lib, const char *name, void *val))fn)(lib, name, val);
}

long unsigned int AIL_3D_sample_length(H3DSAMPLE S)
{
    void *fn = MSS_FN(AIL_3D_sample_length, 4);
    return fn ? ((long unsigned int (__stdcall *)(H3DSAMPLE S))fn)(S) : (long unsigned int)0;
}

long unsigned int AIL_3D_sample_offset(H3DSAMPLE S)
{
    void *fn = MSS_FN(AIL_3D_sample_offset, 4);
    return fn ? ((long unsigned int (__stdcall *)(H3DSAMPLE S))fn)(S) : (long unsigned int)0;
}

long int AIL_3D_sample_playback_rate(H3DSAMPLE S)
{
    void *fn = MSS_FN(AIL_3D_sample_playback_rate, 4);
    return fn ? ((long int (__stdcall *)(H3DSAMPLE S))fn)(S) : (long int)0;
}

long unsigned int AIL_3D_sample_status(H3DSAMPLE S)
{
    void *fn = MSS_FN(AIL_3D_sample_status, 4);
    return fn ? ((long unsigned int (__stdcall *)(H3DSAMPLE S))fn)(S) : (long unsigned int)0;
}

float AIL_3D_sample_volume(H3DSAMPLE S)
{
    void *fn = MSS_FN(AIL_3D_sample_volume, 4);
    return fn ? ((float (__stdcall *)(H3DSAMPLE S))fn)(S) : (float)0;
}

H3DSAMPLE AIL_allocate_3D_sample_handle(HPROVIDER lib)
{
    void *fn = MSS_FN(AIL_allocate_3D_sample_handle, 4);
    return fn ? ((H3DSAMPLE (__stdcall *)(HPROVIDER lib))fn)(lib) : (H3DSAMPLE)0;
}

HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER dig)
{
    void *fn = MSS_FN(AIL_allocate_sample_handle, 4);
    return fn ? ((HSAMPLE (__stdcall *)(HDIGDRIVER dig))fn)(dig) : (HSAMPLE)0;
}

void AIL_close_3D_provider(HPROVIDER lib)
{
    void *fn = MSS_FN(AIL_close_3D_provider, 4);
    if (fn)
        ((void (__stdcall *)(HPROVIDER lib))fn)(lib);
}

void AIL_close_stream(HSTREAM stream)
{
    void *fn = MSS_FN(AIL_close_stream, 4);
    if (fn)
        ((void (__stdcall *)(HSTREAM stream))fn)(stream);
}

long int AIL_digital_CPU_percent(HDIGDRIVER dig)
{
    void *fn = MSS_FN(AIL_digital_CPU_percent, 4);
    return fn ? ((long int (__stdcall *)(HDIGDRIVER dig))fn)(dig) : (long int)0;
}

void AIL_end_3D_sample(H3DSAMPLE S)
{
    void *fn = MSS_FN(AIL_end_3D_sample, 4);
    if (fn)
        ((void (__stdcall *)(H3DSAMPLE S))fn)(S);
}

void AIL_end_sample(HSAMPLE S)
{
    void *fn = MSS_FN(AIL_end_sample, 4);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S))fn)(S);
}

long int AIL_enumerate_3D_providers(HPROENUM *next, HPROVIDER *dest, char **name)
{
    void *fn = MSS_FN(AIL_enumerate_3D_providers, 12);
    return fn ? ((long int (__stdcall *)(HPROENUM *next, HPROVIDER *dest, char **name))fn)(next, dest, name) : (long int)0;
}

void AIL_init_sample(HSAMPLE S)
{
    void *fn = MSS_FN(AIL_init_sample, 4);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S))fn)(S);
}

long int AIL_is_3D_stream(HSTREAM S)
{
    /* the Windows binary plays streams in 2D (volume and pan): no 3D streams */
    (void)S;
    return 0;
}

char * AIL_last_error(void)
{
    void *fn = MSS_FN(AIL_last_error, 0);
    return fn ? ((char * (__stdcall *)(void))fn)() : (char *)0;
}

void AIL_load_sample_buffer(HSAMPLE S, long unsigned int buff_num, const void *buffer, long unsigned int len)
{
    void *fn = MSS_FN(AIL_load_sample_buffer, 16);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S, long unsigned int buff_num, const void *buffer, long unsigned int len))fn)(S, buff_num, buffer, len);
}

long int AIL_minimum_sample_buffer_size(HDIGDRIVER dig, long int playback_rate, long int format)
{
    void *fn = MSS_FN(AIL_minimum_sample_buffer_size, 12);
    return fn ? ((long int (__stdcall *)(HDIGDRIVER dig, long int playback_rate, long int format))fn)(dig, playback_rate, format) : (long int)0;
}

M3DRESULT AIL_open_3D_provider(HPROVIDER lib)
{
    void *fn = MSS_FN(AIL_open_3D_provider, 4);
    return fn ? ((M3DRESULT (__stdcall *)(HPROVIDER lib))fn)(lib) : (M3DRESULT)0;
}

HDIGDRIVER AIL_open_digital_driver(long unsigned int frequency, long int bits, long int channel, long unsigned int flags)
{
    void *fn = MSS_FN(AIL_open_digital_driver, 16);
    HDIGDRIVER dig = fn ? ((HDIGDRIVER (__stdcall *)(long unsigned int frequency, long int bits, long int channel, long unsigned int flags))fn)(frequency, bits, channel, flags) : (HDIGDRIVER)0;
    MSS_SetWindow(dig);
    return dig;
}

HSTREAM AIL_open_stream(HDIGDRIVER dig, const char *filename, long int stream_mem)
{
    void *fn = MSS_FN(AIL_open_stream, 12);
    return fn ? ((HSTREAM (__stdcall *)(HDIGDRIVER dig, const char *filename, long int stream_mem))fn)(dig, filename, stream_mem) : (HSTREAM)0;
}

void AIL_pause_stream(HSTREAM stream, long int onoff)
{
    void *fn = MSS_FN(AIL_pause_stream, 8);
    if (fn)
        ((void (__stdcall *)(HSTREAM stream, long int onoff))fn)(stream, onoff);
}

long int AIL_process_digital_audio(void *dest, long int size, long unsigned int rate, long unsigned int format, long int num, void *info)
{
    void *fn = MSS_FN(AIL_process_digital_audio, 24);
    return fn ? ((long int (__stdcall *)(void *dest, long int size, long unsigned int rate, long unsigned int format, long int num, void *info))fn)(dest, size, rate, format, num, info) : (long int)0;
}

void AIL_release_sample_handle(HSAMPLE S)
{
    void *fn = MSS_FN(AIL_release_sample_handle, 4);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S))fn)(S);
}

void AIL_resume_3D_sample(H3DSAMPLE S)
{
    void *fn = MSS_FN(AIL_resume_3D_sample, 4);
    if (fn)
        ((void (__stdcall *)(H3DSAMPLE S))fn)(S);
}

void AIL_resume_sample(HSAMPLE S)
{
    void *fn = MSS_FN(AIL_resume_sample, 4);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S))fn)(S);
}

long int AIL_sample_buffer_ready(HSAMPLE S)
{
    void *fn = MSS_FN(AIL_sample_buffer_ready, 4);
    return fn ? ((long int (__stdcall *)(HSAMPLE S))fn)(S) : (long int)0;
}

void AIL_sample_ms_position(HSAMPLE S, long int *total_milliseconds, long int *current_milliseconds)
{
    void *fn = MSS_FN(AIL_sample_ms_position, 12);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S, long int *total_milliseconds, long int *current_milliseconds))fn)(S, total_milliseconds, current_milliseconds);
}

long int AIL_sample_playback_rate(HSAMPLE S)
{
    void *fn = MSS_FN(AIL_sample_playback_rate, 4);
    return fn ? ((long int (__stdcall *)(HSAMPLE S))fn)(S) : (long int)0;
}

long unsigned int AIL_sample_position(HSAMPLE S)
{
    void *fn = MSS_FN(AIL_sample_position, 4);
    return fn ? ((long unsigned int (__stdcall *)(HSAMPLE S))fn)(S) : (long unsigned int)0;
}

long unsigned int AIL_sample_status(HSAMPLE S)
{
    void *fn = MSS_FN(AIL_sample_status, 4);
    return fn ? ((long unsigned int (__stdcall *)(HSAMPLE S))fn)(S) : (long unsigned int)0;
}

void AIL_sample_volume_levels(HSAMPLE S, float *left_level, float *right_level)
{
    void *fn = MSS_FN(AIL_sample_volume_levels, 12);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S, float *left_level, float *right_level))fn)(S, left_level, right_level);
}

void AIL_sample_volume_pan(HSAMPLE S, float *volume, float *pan)
{
    void *fn = MSS_FN(AIL_sample_volume_pan, 12);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S, float *volume, float *pan))fn)(S, volume, pan);
}

void AIL_set_3D_distance_factor(HPROVIDER lib, float factor)
{
    void *fn = MSS_FN(AIL_set_3D_distance_factor, 8);
    if (fn)
        ((void (__stdcall *)(HPROVIDER lib, float factor))fn)(lib, factor);
}

void AIL_set_3D_position(H3DPOBJECT obj, float X, float Y, float Z)
{
    void *fn = MSS_FN(AIL_set_3D_position, 16);
    if (fn)
        ((void (__stdcall *)(H3DPOBJECT obj, float X, float Y, float Z))fn)(obj, X, Y, Z);
}

void AIL_set_3D_rolloff_factor(HPROVIDER lib, float factor)
{
    void *fn = MSS_FN(AIL_set_3D_rolloff_factor, 8);
    if (fn)
        ((void (__stdcall *)(HPROVIDER lib, float factor))fn)(lib, factor);
}

void AIL_set_3D_room_type(HPROVIDER lib, long int room_type)
{
    void *fn = MSS_FN(AIL_set_3D_room_type, 8);
    if (fn)
        ((void (__stdcall *)(HPROVIDER lib, long int room_type))fn)(lib, room_type);
}

void AIL_set_3D_sample_distances(H3DSAMPLE S, float max_dist, float min_dist)
{
    void *fn = MSS_FN(AIL_set_3D_sample_distances, 12);
    if (fn)
        ((void (__stdcall *)(H3DSAMPLE S, float max_dist, float min_dist))fn)(S, max_dist, min_dist);
}

void AIL_set_3D_sample_effects_level(H3DSAMPLE S, float effects_level)
{
    void *fn = MSS_FN(AIL_set_3D_sample_effects_level, 8);
    if (fn)
        ((void (__stdcall *)(H3DSAMPLE S, float effects_level))fn)(S, effects_level);
}

long int AIL_set_3D_sample_info(H3DSAMPLE S, const AILSOUNDINFO *info)
{
    void *fn = MSS_FN(AIL_set_3D_sample_info, 8);
    return fn ? ((long int (__stdcall *)(H3DSAMPLE S, const AILSOUNDINFO *info))fn)(S, info) : (long int)0;
}

void AIL_set_3D_sample_loop_count(H3DSAMPLE S, long unsigned int loops)
{
    void *fn = MSS_FN(AIL_set_3D_sample_loop_count, 8);
    if (fn)
        ((void (__stdcall *)(H3DSAMPLE S, long unsigned int loops))fn)(S, loops);
}

void AIL_set_3D_sample_offset(H3DSAMPLE S, long unsigned int offset)
{
    void *fn = MSS_FN(AIL_set_3D_sample_offset, 8);
    if (fn)
        ((void (__stdcall *)(H3DSAMPLE S, long unsigned int offset))fn)(S, offset);
}

void AIL_set_3D_sample_playback_rate(H3DSAMPLE S, long int playback_rate)
{
    void *fn = MSS_FN(AIL_set_3D_sample_playback_rate, 8);
    if (fn)
        ((void (__stdcall *)(H3DSAMPLE S, long int playback_rate))fn)(S, playback_rate);
}

void AIL_set_3D_sample_volume(H3DSAMPLE S, float volume)
{
    void *fn = MSS_FN(AIL_set_3D_sample_volume, 8);
    if (fn)
        ((void (__stdcall *)(H3DSAMPLE S, float volume))fn)(S, volume);
}

void AIL_set_3D_stream_position(HSTREAM S, float X, float Y, float Z)
{
    (void)S;
    (void)X;
    (void)Y;
    (void)Z;
}

void AIL_set_digital_master_reverb_levels(HDIGDRIVER dig, float dry_level, float wet_level)
{
    void *fn = MSS_FN(AIL_set_digital_master_reverb_levels, 12);
    if (fn)
        ((void (__stdcall *)(HDIGDRIVER dig, float dry_level, float wet_level))fn)(dig, dry_level, wet_level);
}

void AIL_set_digital_master_room_type(HDIGDRIVER dig, long int room_type)
{
    void *fn = MSS_FN(AIL_set_digital_master_room_type, 8);
    if (fn)
        ((void (__stdcall *)(HDIGDRIVER dig, long int room_type))fn)(dig, room_type);
}

void AIL_set_file_callbacks(AIL_file_open_callback opencb, AIL_file_close_callback closecb, AIL_file_seek_callback seekcb, AIL_file_read_callback readcb)
{
    void *fn = MSS_FN(AIL_set_file_callbacks, 16);
    if (fn)
        ((void (__stdcall *)(AIL_file_open_callback opencb, AIL_file_close_callback closecb, AIL_file_seek_callback seekcb, AIL_file_read_callback readcb))fn)(opencb, closecb, seekcb, readcb);
}

long int AIL_set_preference(long unsigned int number, long int value)
{
    void *fn = MSS_FN(AIL_set_preference, 8);
    return fn ? ((long int (__stdcall *)(long unsigned int number, long int value))fn)(number, value) : (long int)0;
}

char * AIL_set_redist_directory(const char *dir)
{
    void *fn = MSS_FN(AIL_set_redist_directory, 4);
    return fn ? ((char *(__stdcall *)(const char *))fn)(MSS_GamePath(dir)) : (char *)0;
}

void AIL_set_sample_address(HSAMPLE S, const void *start, long unsigned int len)
{
    void *fn = MSS_FN(AIL_set_sample_address, 12);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S, const void *start, long unsigned int len))fn)(S, start, len);
}

void AIL_set_sample_adpcm_block_size(HSAMPLE S, long unsigned int blocksize)
{
    void *fn = MSS_FN(AIL_set_sample_adpcm_block_size, 8);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S, long unsigned int blocksize))fn)(S, blocksize);
}

void AIL_set_sample_loop_count(HSAMPLE S, long int loop_count)
{
    void *fn = MSS_FN(AIL_set_sample_loop_count, 8);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S, long int loop_count))fn)(S, loop_count);
}

void AIL_set_sample_ms_position(HSAMPLE S, long int milliseconds)
{
    void *fn = MSS_FN(AIL_set_sample_ms_position, 8);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S, long int milliseconds))fn)(S, milliseconds);
}

void AIL_set_sample_playback_rate(HSAMPLE S, long int playback_rate)
{
    void *fn = MSS_FN(AIL_set_sample_playback_rate, 8);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S, long int playback_rate))fn)(S, playback_rate);
}

void AIL_set_sample_reverb_levels(HSAMPLE S, float dry_level, float wet_level)
{
    void *fn = MSS_FN(AIL_set_sample_reverb_levels, 12);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S, float dry_level, float wet_level))fn)(S, dry_level, wet_level);
}

void AIL_set_sample_type(HSAMPLE S, long int format, long unsigned int flags)
{
    void *fn = MSS_FN(AIL_set_sample_type, 12);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S, long int format, long unsigned int flags))fn)(S, format, flags);
}

void AIL_set_sample_volume_levels(HSAMPLE S, float left_level, float right_level)
{
    void *fn = MSS_FN(AIL_set_sample_volume_levels, 12);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S, float left_level, float right_level))fn)(S, left_level, right_level);
}

void AIL_set_stream_loop_count(HSTREAM stream, long int count)
{
    void *fn = MSS_FN(AIL_set_stream_loop_count, 8);
    if (fn)
        ((void (__stdcall *)(HSTREAM stream, long int count))fn)(stream, count);
}

void AIL_set_stream_ms_position(HSTREAM S, long int milliseconds)
{
    void *fn = MSS_FN(AIL_set_stream_ms_position, 8);
    if (fn)
        ((void (__stdcall *)(HSTREAM S, long int milliseconds))fn)(S, milliseconds);
}

void AIL_set_stream_playback_rate(HSTREAM stream, long int rate)
{
    void *fn = MSS_FN(AIL_set_stream_playback_rate, 8);
    if (fn)
        ((void (__stdcall *)(HSTREAM stream, long int rate))fn)(stream, rate);
}

void AIL_set_stream_reverb_levels(HSTREAM S, float dry_level, float wet_level)
{
    void *fn = MSS_FN(AIL_set_stream_reverb_levels, 12);
    if (fn)
        ((void (__stdcall *)(HSTREAM S, float dry_level, float wet_level))fn)(S, dry_level, wet_level);
}

void AIL_set_stream_volume_levels(HSTREAM S, float left_level, float right_level)
{
    void *fn = MSS_FN(AIL_set_stream_volume_levels, 12);
    if (fn)
        ((void (__stdcall *)(HSTREAM S, float left_level, float right_level))fn)(S, left_level, right_level);
}

void AIL_shutdown(void)
{
    void *fn = MSS_FN(AIL_shutdown, 0);
    if (fn)
        ((void (__stdcall *)(void))fn)();
}

long int AIL_size_processed_digital_audio(long unsigned int dest_rate, long unsigned int dest_format, long int num_srcs, const AILMIXINFO *src)
{
    void *fn = MSS_FN(AIL_size_processed_digital_audio, 16);
    return fn ? ((long int (__stdcall *)(long unsigned int dest_rate, long unsigned int dest_format, long int num_srcs, const AILMIXINFO *src))fn)(dest_rate, dest_format, num_srcs, src) : (long int)0;
}

long int AIL_startup(void)
{
    void *fn = MSS_FN(AIL_startup, 0);
    return fn ? ((long int (__stdcall *)(void))fn)() : (long int)0;
}

void AIL_stop_3D_sample(H3DSAMPLE S)
{
    void *fn = MSS_FN(AIL_stop_3D_sample, 4);
    if (fn)
        ((void (__stdcall *)(H3DSAMPLE S))fn)(S);
}

void AIL_stop_sample(HSAMPLE S)
{
    void *fn = MSS_FN(AIL_stop_sample, 4);
    if (fn)
        ((void (__stdcall *)(HSAMPLE S))fn)(S);
}

void AIL_stream_info(HSTREAM stream, long int *datarate, long int *sndtype, long int *length, long int *memory)
{
    void *fn = MSS_FN(AIL_stream_info, 20);
    if (fn)
        ((void (__stdcall *)(HSTREAM stream, long int *datarate, long int *sndtype, long int *length, long int *memory))fn)(stream, datarate, sndtype, length, memory);
}

void AIL_stream_ms_position(HSTREAM S, long int *total_milliseconds, long int *current_milliseconds)
{
    void *fn = MSS_FN(AIL_stream_ms_position, 12);
    if (fn)
        ((void (__stdcall *)(HSTREAM S, long int *total_milliseconds, long int *current_milliseconds))fn)(S, total_milliseconds, current_milliseconds);
}

long int AIL_stream_playback_rate(HSTREAM stream)
{
    void *fn = MSS_FN(AIL_stream_playback_rate, 4);
    return fn ? ((long int (__stdcall *)(HSTREAM stream))fn)(stream) : (long int)0;
}

long int AIL_stream_status(HSTREAM stream)
{
    void *fn = MSS_FN(AIL_stream_status, 4);
    return fn ? ((long int (__stdcall *)(HSTREAM stream))fn)(stream) : (long int)0;
}

void AIL_stream_volume_levels(HSTREAM S, float *left_level, float *right_level)
{
    void *fn = MSS_FN(AIL_stream_volume_levels, 12);
    if (fn)
        ((void (__stdcall *)(HSTREAM S, float *left_level, float *right_level))fn)(S, left_level, right_level);
}

void AIL_stream_volume_pan(HSTREAM stream, float *volume, float *pan)
{
    void *fn = MSS_FN(AIL_stream_volume_pan, 12);
    if (fn)
        ((void (__stdcall *)(HSTREAM stream, float *volume, float *pan))fn)(stream, volume, pan);
}

long int AIL_WAV_info(const void *data, void *info)
{
    void *fn = MSS_FN(AIL_WAV_info, 8);
    return fn ? ((long int (__stdcall *)(const void *data, void *info))fn)(data, info) : (long int)0;
}
#endif
