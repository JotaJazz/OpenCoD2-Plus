#include "common_types.h"
extern void Com_Printf(const char *fmt, ...);
#include "imports.h"

#if !defined(__EMSCRIPTEN__) && defined(COD2_ENABLE_NULL_CALL_RECOVERY)
#include <signal.h>
#include <sys/ucontext.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__EMSCRIPTEN__) && defined(COD2_ENABLE_NULL_CALL_RECOVERY)
#define __USE_GNU
#include <ucontext.h>
#endif

extern int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

typedef struct {
    const char *value;
    UInt32 reserved[6];
} MacGameEngineProductTagStorage;

__attribute__((used)) static const char *kBuildTag = "Posted-04/18/06";
__attribute__((used)) static MacGameEngineProductTagStorage kProductTag = {
    "2FF037E8990B11DAA29C000A95675086",
    {0, 0, 0, 0, 0, 0},
};

#if !defined(__EMSCRIPTEN__) && defined(COD2_ENABLE_NULL_CALL_RECOVERY)
static int null_call_count = 0;
static int segfault_count = 0;

static void null_call_handler(int sig, siginfo_t *info, void *ucontext)
{
    ucontext_t *ctx = (ucontext_t *)ucontext;
    unsigned int eip = ctx->uc_mcontext.gregs[14];

    if (eip == 0) {
        unsigned int *esp = (unsigned int *)(unsigned long)ctx->uc_mcontext.gregs[7];
        unsigned int retaddr = *esp;
        null_call_count++;
        if (null_call_count <= 50) {
            fprintf(stderr, "NULL_CALL #%d from 0x%08x\n", null_call_count, retaddr);
        }
        ctx->uc_mcontext.gregs[7] = (unsigned int)(unsigned long)(esp + 1);
        ctx->uc_mcontext.gregs[14] = retaddr;
        ctx->uc_mcontext.gregs[11] = 0;
        return;
    }

    segfault_count++;
    if (segfault_count > 50000) {
        fprintf(stderr, "Too many segfaults (%d), aborting at EIP=0x%08x\n", segfault_count, eip);
        signal(SIGSEGV, SIG_DFL);
        raise(SIGSEGV);
        return;
    }

    {
        unsigned int ebp = ctx->uc_mcontext.gregs[6];
        if (ebp > 0x08000000 && ebp < 0xfffff000) {
            unsigned int *frame = (unsigned int *)(unsigned long)ebp;
            unsigned int saved_ebp = frame[0];
            unsigned int ret_addr = frame[1];
            if (ret_addr > 0x08000000 && ret_addr < 0x09000000) {
                if (segfault_count <= 50)
                    fprintf(stderr, "SEGFAULT #%d at EIP=0x%08x, returning to 0x%08x\n", segfault_count, eip, ret_addr);
                ctx->uc_mcontext.gregs[6] = saved_ebp;
                ctx->uc_mcontext.gregs[7] = (unsigned int)(unsigned long)(frame + 2);
                ctx->uc_mcontext.gregs[14] = ret_addr;
                ctx->uc_mcontext.gregs[11] = 0;
                return;
            }
        }

        fprintf(stderr, "SEGFAULT at 0x%08x, cannot recover (EBP=0x%08x)\n", eip, ebp);
    }

    signal(SIGSEGV, SIG_DFL);
    raise(SIGSEGV);
}

static void install_null_call_handler(void)
{
    struct sigaction sa;
    sa.sa_sigaction = null_call_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
}
#endif

static void Linux_FixupDataPointers(void)
{
    extern unsigned char legacyHacks[32];
    extern unsigned char legacyHacksArray[1792];
    extern void *cmd_text;
    extern char cmd_texts[];

    *(void **)legacyHacks = legacyHacksArray;
    cmd_text = cmd_texts;
}

Boolean CMacGameEngine_ProcessAllEvents(void)
{
    return 0;
}

void CMacGameEngine_Quit(void)
{
}

void CMacGameEngine_DrawSplashScreen(const char *inFileName)
{
    (void)inFileName;
}

int UpdateSystemActivity(int activity)
{
    (void)activity;
    return 0;
}

#if COD2_IS_PATCH_13

void Sys_OpenURL(const char *url, int activate)
{
    char cmd[1100];
    (void)activate;
    if (!url || !url[0])
        return;

    if (strchr(url, '\''))
        return;
    snprintf(cmd, sizeof(cmd), "xdg-open '%s' >/dev/null 2>&1 &", url);
    if (system(cmd) != 0)
        Com_Printf("Sys_OpenURL: failed to open %s\n", url);
}
#endif

int main(int argc, char **argv)
{
    char cmdLine[4096];
    int i;
    int offset;

    cmdLine[0] = '\0';
    offset = 0;
    for (i = 1; i < argc; i++) {
        int len = strlen(argv[i]);
        if (offset + len + 2 >= (int)sizeof(cmdLine))
            break;
        if (offset > 0)
            cmdLine[offset++] = ' ';
        memcpy(cmdLine + offset, argv[i], len);
        offset += len;
    }
    cmdLine[offset] = '\0';

#if !defined(__EMSCRIPTEN__) && defined(COD2_ENABLE_NULL_CALL_RECOVERY)
    install_null_call_handler();
#endif

    Linux_FixupDataPointers();
    return WinMain(0, 0, cmdLine, 0);
}
