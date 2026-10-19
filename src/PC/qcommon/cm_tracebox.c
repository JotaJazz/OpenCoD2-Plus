#include "common_types.h"
#include "imports.h"

void CM_CalcTraceEntents(TraceExtents *extents)
{
    int i;
    float delta;

    for (i = 0; i < 3; i++) {
        delta = extents->start[i] - extents->end[i];
        if (delta == 0.0f)
            extents->invDelta[i] = 0.0f;
        else
            extents->invDelta[i] = 1.0f / delta;
    }
}

/* 1 when the segment from start to end, cut at fraction, misses the box.
 * The first pass tests the min planes (s = -1), the second the max planes
 * (s = 1); o is the latest entry and fraction the earliest exit. */
qboolean CM_TraceBox(const TraceExtents *extents, const vec_t *mins, const vec_t *maxs, float fraction)
{
    const vec_t *bounds;
    float o, s, t1, t2, f;
    int i;

    o = 0.0f;
    s = -1.0f;
    bounds = mins;

    for (;;) {
        for (i = 0; i < 3; i++) {
            t1 = (extents->start[i] - bounds[i]) * s;
            t2 = (extents->end[i] - bounds[i]) * s;

            if (t1 > 0.0f) {
                if (t2 > 0.0f)
                    return 1;

                f = t1 * extents->invDelta[i] * s;
                if (f >= fraction)
                    return 1;
                if (f > o)
                    o = f;
            } else if (t2 > 0.0f) {
                f = t1 * extents->invDelta[i] * s;
                if (o >= f)
                    return 1;
                if (f < fraction)
                    fraction = f;
            }
        }

        if (s == 1.0f)
            break;

        s = 1.0f;
        bounds = maxs;
    }

    return 0;
}
