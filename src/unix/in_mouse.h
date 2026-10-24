#ifndef COD2_IN_MOUSE_H
#define COD2_IN_MOUSE_H

/* Whether the client captures the mouse, as the 1.3 binary's IN_Frame
 * (0x464b30): captured with the window focused, in menus too; let go with
 * the console open (key catcher 1) in a window; never with in_mouse 0, which
 * leaves the mouse inactive. */
static inline int IN_WantMouse(int haveClient, int focused, int keyCatchers, int fullscreen, int inMouse)
{
    if (!haveClient || !focused || !inMouse)
        return 0;
    return !((keyCatchers & 1) && !fullscreen);
}

/* Whether a mouse button counts: as the 1.3 binary's IN_MouseEvent
 * (0x464940), which returns at once when IN_Init found in_mouse 0. */
static inline int IN_MouseButtons(int inMouse)
{
    return inMouse != 0;
}

/* The X11 fallback's deltas from the pointer's positions: the first motion
 * after IN_MotionReset, which IN_Frame calls when the capture changes and
 * the pointer warps to the window's center, moves nothing. */
typedef struct {
    int x;
    int y;
    int valid;
} IN_MotionState;

static inline void IN_MotionReset(IN_MotionState *s)
{
    s->valid = 0;
}

static inline void IN_MotionDelta(IN_MotionState *s, int x, int y, int *dx, int *dy)
{
    *dx = s->valid ? x - s->x : 0;
    *dy = s->valid ? y - s->y : 0;
    s->x = x;
    s->y = y;
    s->valid = 1;
}

#endif
