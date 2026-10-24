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

#endif
