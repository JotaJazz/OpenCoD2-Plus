#include "common_types.h"
#include <stdio.h>
#include <stdlib.h>
#include "imports.h"


extern int R_BeginDelayedDrawing(void);
extern void R_EndDelayedDrawing(int handle);
extern void R_IssueDelayedDrawing(int handle);

extern GfxDrawGroupCommands delayedGroup[5];
extern GfxCmdArray *s_cmdList;

static int R_DrawGroupTraceUsed(void)
{
    return s_cmdList ? s_cmdList->usedTotal : -1;
}

void R_InitDrawGroups(void)
{
    int group;
    for (group = 0; group < 5; group++) {
        delayedGroup[group].isIssuingGlue = 0;
        delayedGroup[group].begin = -1;
        int i;
        for (i = 0; i < 4; i++) {
            delayedGroup[group].perView[i] = -1;
        }
        delayedGroup[group].end = -1;
    }
}

GfxDrawGroupGlueBehavior R_BeginDrawGroupSection(GfxDrawGroupType group)
{
    if (delayedGroup[group].begin != -1) {
        return (GfxDrawGroupGlueBehavior)(1);
    }

    delayedGroup[group].isIssuingGlue = 1;
    delayedGroup[group].begin = R_BeginDelayedDrawing();
    if (group == 4) {
        if (getenv("DBGSPAM"))
            printf("[drawgroup] begin section group=%d marker=%d used=%d\n",
                   group, delayedGroup[group].begin, R_DrawGroupTraceUsed());
    }
    return (GfxDrawGroupGlueBehavior)(0);
}

void R_BeginDrawGroupLoop(GfxDrawGroupType group, int viewIndex)
{
    if (delayedGroup[group].isIssuingGlue) {
        R_EndDelayedDrawing(delayedGroup[group].begin);
        delayedGroup[group].isIssuingGlue = 0;
    }
    delayedGroup[group].perView[viewIndex] = R_BeginDelayedDrawing();
    if (group == 4) {
        if (getenv("DBGSPAM"))
            printf("[drawgroup] begin loop group=%d view=%d marker=%d used=%d\n",
                   group, viewIndex, delayedGroup[group].perView[viewIndex], R_DrawGroupTraceUsed());
    }
}

GfxDrawGroupGlueBehavior R_EndDrawGroupLoop(GfxDrawGroupType group, int viewIndex)
{
    if (delayedGroup[group].perView[viewIndex] != -1) {
        if (group == 4) {
            if (getenv("DBGSPAM"))
                printf("[drawgroup] end loop group=%d view=%d marker=%d usedBefore=%d\n",
                       group, viewIndex, delayedGroup[group].perView[viewIndex], R_DrawGroupTraceUsed());
        }
        R_EndDelayedDrawing(delayedGroup[group].perView[viewIndex]);
        if (group == 4) {
            if (getenv("DBGSPAM"))
                printf("[drawgroup] end loop done group=%d view=%d usedAfter=%d\n",
                       group, viewIndex, R_DrawGroupTraceUsed());
        }
    }

    if (delayedGroup[group].end != -1)
        return (GfxDrawGroupGlueBehavior)(1);

    delayedGroup[group].isIssuingGlue = 1;
    delayedGroup[group].end = R_BeginDelayedDrawing();
    if (group == 4) {
        if (getenv("DBGSPAM"))
            printf("[drawgroup] begin end-glue group=%d marker=%d used=%d\n",
                   group, delayedGroup[group].end, R_DrawGroupTraceUsed());
    }
    return (GfxDrawGroupGlueBehavior)(0);
}

void R_EndDrawGroupSection(GfxDrawGroupType group)
{
    if (delayedGroup[group].isIssuingGlue) {
        R_EndDelayedDrawing(delayedGroup[group].end);
        delayedGroup[group].isIssuingGlue = 0;
    }
}

void R_IssueDrawGroups(void)
{
    int group;
    for (group = 0; group < 5; group++) {
        if (delayedGroup[group].begin != -1) {
            if (group == 4) {
                if (getenv("DBGSPAM"))
                    printf("[drawgroup] issue begin group=%d marker=%d used=%d\n",
                           group, delayedGroup[group].begin, R_DrawGroupTraceUsed());
            }
            R_IssueDelayedDrawing(delayedGroup[group].begin);
            delayedGroup[group].begin = -1;
        }

        int i;
        for (i = 0; i < 4; i++) {
            if (delayedGroup[group].perView[i] != -1) {
                if (group == 4) {
                    if (getenv("DBGSPAM"))
                        printf("[drawgroup] issue loop group=%d view=%d marker=%d used=%d\n",
                               group, i, delayedGroup[group].perView[i], R_DrawGroupTraceUsed());
                }
                R_IssueDelayedDrawing(delayedGroup[group].perView[i]);
                delayedGroup[group].perView[i] = -1;
            }
        }

        if (delayedGroup[group].end != -1) {
            if (group == 4) {
                if (getenv("DBGSPAM"))
                    printf("[drawgroup] issue end group=%d marker=%d used=%d\n",
                           group, delayedGroup[group].end, R_DrawGroupTraceUsed());
            }
            R_IssueDelayedDrawing(delayedGroup[group].end);
            delayedGroup[group].end = -1;
        }
    }
}
