
#if defined(_WIN32) || defined(COD2_VC71_COMPAT_H)

#    include <stdio.h>
#    include <string.h>
#    include <stdlib.h>
#    include <float.h>

#    if defined(COD2_VC71_COMPAT_H)
#        include "common_types.h"
#    else
typedef int qboolean;
typedef unsigned char Boolean;
#        define qfalse 0
#        define qtrue 1
#        define ERR_FATAL 0
#    endif
#    include "headers/PC/win32/win32_syscon_api.h"

#    define SYSCON_CLASS "CoD2 WinConsole"
#    define SYSCON_TITLE "CoD2 Console"
#    define SYSCON_FONT "Courier New"
#    define SYSCON_LOGO "codlogo.bmp"

#    define ERRORBOX_ID 10
#    define ERRORTEXT_ID 11
#    define QUIT_ID 12
#    define CLEAR_ID 13
#    define COPY_ID 14
#    define EDIT_ID 100
#    define INPUT_ID 101

extern void Com_Error(int code, const char *fmt, ...);
extern void Sys_Error(const char *error, ...);
extern void Com_Printf(const char *fmt, ...);
extern void Cbuf_AddText(const char *text);
extern void Sys_Quit(void);

void Sys_CreateConsole(HINSTANCE hInstance);
void Sys_DestroyConsole(void);
void Sys_ShowConsole(int visLevel, qboolean quitOnClose);
char *Sys_ConsoleInput(void);
void Conbuf_AppendText(const char *pMsg);
void Sys_SetErrorText(const char *buf);

extern Boolean gConsoleRunning;

typedef struct {
    HWND hWnd;
    HWND hwndBuffer;
    HWND hwndButtonClear;
    HWND hwndButtonCopy;
    HWND hwndButtonQuit;
    HWND hwndErrorBox;
    HWND hwndErrorText;
    HWND hwndLogo;
    HBITMAP hbmLogo;
    HDC hDC;
    HFONT hfBufferFont;
    HWND hwndInputLine;
    char errorString[80];
    char consoleText[512];
    char returnedText[512];
    int windowVisLevel;
    qboolean quitOnClose;
    int windowWidth, windowHeight;
    WNDPROC SysInputLineWndProc;
} WinConData;

static WinConData s_wcd;

#    define CONSOLE_BUFFER_SIZE 16384

static LONG WINAPI ConWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE)
            SetFocus(s_wcd.hwndInputLine);
        break;

    case WM_CLOSE:

        if (s_wcd.quitOnClose) {
            PostQuitMessage(0);
        } else {
            Sys_ShowConsole(0, qfalse);
        }
        return 0;

    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam == s_wcd.hwndBuffer) {
            SetBkColor((HDC)wParam, RGB(0x00, 0x00, 0x80));
            SetTextColor((HDC)wParam, RGB(0xff, 0xff, 0xff));
            return (LONG)(LONG_PTR)GetStockObject(BLACK_BRUSH);
        }
        break;

    case WM_COMMAND:
        if (wParam == COPY_ID) {
            SendMessage(s_wcd.hwndBuffer, EM_SETSEL, 0, -1);
            SendMessage(s_wcd.hwndBuffer, WM_COPY, 0, 0);
        } else if (wParam == QUIT_ID) {
            if (s_wcd.quitOnClose)
                PostQuitMessage(0);
            else
                Cbuf_AddText("quit\n");
        } else if (wParam == CLEAR_ID) {
            SendMessage(s_wcd.hwndBuffer, EM_SETSEL, 0, -1);
            SendMessage(s_wcd.hwndBuffer, EM_REPLACESEL, FALSE, (LPARAM) "");
            UpdateWindow(s_wcd.hwndBuffer);
        }
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static LONG WINAPI InputLineWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_KILLFOCUS:
        if ((HWND)wParam == s_wcd.hWnd) {
            SetFocus(hWnd);
            return 0;
        }
        break;

    case WM_CHAR:
        if (wParam == 13) {
            GetWindowText(s_wcd.hwndInputLine, s_wcd.consoleText, sizeof(s_wcd.consoleText) - 2);
            SetWindowText(s_wcd.hwndInputLine, "");
            Com_Printf("]%s\n", s_wcd.consoleText);
            strcat(s_wcd.consoleText, "\n");

            return 0;
        }
        break;
    }

    return CallWindowProc(s_wcd.SysInputLineWndProc, hWnd, uMsg, wParam, lParam);
}

void Sys_CreateConsole(HINSTANCE hInstance)
{
    WNDCLASS wc;
    RECT rect;
    HDC hDC;
    int nHeight;
    int swidth, sheight;
    int DEDSTYLE = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX;

    memset(&wc, 0, sizeof(wc));
    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC)ConWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszMenuName = 0;
    wc.lpszClassName = SYSCON_CLASS;

    if (!RegisterClass(&wc))
        return;

    rect.left = 0;
    rect.right = 620;
    rect.top = 0;
    rect.bottom = 450;
    AdjustWindowRect(&rect, DEDSTYLE, FALSE);

    hDC = GetDC(GetDesktopWindow());
    swidth = GetDeviceCaps(hDC, HORZRES);
    sheight = GetDeviceCaps(hDC, VERTRES);
    ReleaseDC(GetDesktopWindow(), hDC);

    s_wcd.windowWidth = rect.right - rect.left + 1;
    s_wcd.windowHeight = rect.bottom - rect.top + 1;

    s_wcd.hWnd = CreateWindowEx(0, SYSCON_CLASS, SYSCON_TITLE, DEDSTYLE,
                                (swidth - 600) / 2, (sheight - 450) / 2,
                                s_wcd.windowWidth, s_wcd.windowHeight,
                                NULL, NULL, hInstance, NULL);

    _controlfp(_PC_24, _MCW_PC);
    _controlfp(_MCW_EM, _MCW_EM);

    if (s_wcd.hWnd == NULL)
        return;

    s_wcd.hDC = GetDC(s_wcd.hWnd);
    nHeight = -MulDiv(8, GetDeviceCaps(s_wcd.hDC, LOGPIXELSY), 72);
    s_wcd.hfBufferFont = CreateFont(nHeight, 0, 0, 0, FW_LIGHT, 0, 0, 0, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                    FF_MODERN | FIXED_PITCH, SYSCON_FONT);
    ReleaseDC(s_wcd.hWnd, s_wcd.hDC);

    s_wcd.hbmLogo = (HBITMAP)LoadImageA(NULL, SYSCON_LOGO, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    if (s_wcd.hbmLogo) {
        s_wcd.hwndLogo = CreateWindowEx(0, "Static", NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP,
                                        5, 5, 0, 0, s_wcd.hWnd, (HMENU)1, hInstance, NULL);
        SendMessage(s_wcd.hwndLogo, STM_SETIMAGE, 0, (LPARAM)s_wcd.hbmLogo);
    }

    s_wcd.hwndInputLine = CreateWindowEx(0, "edit", NULL,
                                         WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                         6, 400, 608, 20, s_wcd.hWnd, (HMENU)INPUT_ID,
                                         hInstance, NULL);

    s_wcd.hwndBuffer = CreateWindowEx(0, "edit", NULL,
                                      WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
                                          ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                                      6, 70, 606, 324, s_wcd.hWnd, (HMENU)EDIT_ID,
                                      hInstance, NULL);
    SendMessage(s_wcd.hwndBuffer, WM_SETFONT, (WPARAM)s_wcd.hfBufferFont, 0);

    s_wcd.SysInputLineWndProc =
        (WNDPROC)SetWindowLongPtr(s_wcd.hwndInputLine, GWLP_WNDPROC, (LONG_PTR)InputLineWndProc);
    SendMessage(s_wcd.hwndInputLine, WM_SETFONT, (WPARAM)s_wcd.hfBufferFont, 0);
    SetFocus(s_wcd.hwndInputLine);

    s_wcd.windowVisLevel = 0;
}

void Sys_DestroyConsole(void)
{
    if (s_wcd.hWnd) {
        ShowWindow(s_wcd.hWnd, SW_HIDE);
        CloseWindow(s_wcd.hWnd);
        DestroyWindow(s_wcd.hWnd);
        s_wcd.hWnd = 0;
    }
    gConsoleRunning = 0;
}

void Sys_ShowConsole(int visLevel, qboolean quitOnClose)
{
    s_wcd.quitOnClose = quitOnClose;

    if (visLevel == s_wcd.windowVisLevel)
        return;

    s_wcd.windowVisLevel = visLevel;

    if (!s_wcd.hWnd)
        return;

    switch (visLevel) {
    case 0:
        ShowWindow(s_wcd.hWnd, SW_HIDE);
        break;
    case 1:
        ShowWindow(s_wcd.hWnd, SW_SHOWNORMAL);
        SendMessage(s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff);
        break;
    case 2:
        ShowWindow(s_wcd.hWnd, SW_MINIMIZE);
        break;
    default:

        Sys_Error("Invalid visLevel %d sent to Sys_ShowConsole\n", visLevel);
        break;
    }
}

char *Sys_ConsoleInput(void)
{
    if (s_wcd.consoleText[0] == '\0')
        return NULL;

    strcpy(s_wcd.returnedText, s_wcd.consoleText);
    s_wcd.consoleText[0] = '\0';
    return s_wcd.returnedText;
}

void Conbuf_AppendText(const char *pMsg)
{
    char buffer[CONSOLE_BUFFER_SIZE * 2];
    char *b = buffer;
    const char *msg;
    int bufLen;
    int i = 0;

    if (!pMsg)
        return;

    for (msg = pMsg; *msg && ((b - buffer) < (int)sizeof(buffer) - 2); msg++) {
        if (*msg == '\n' && msg[1] == '\r') {
            *b++ = '\r';
            *b++ = '\n';
            msg++;
        } else if (*msg == '\r') {
            *b++ = '\r';
            *b++ = '\n';
        } else if (*msg == '\n') {
            *b++ = '\r';
            *b++ = '\n';
        } else if (msg[0] == '^' && msg[1] && msg[1] != '^') {
            msg++;
        } else {
            *b++ = *msg;
        }
    }
    *b = '\0';
    bufLen = (int)(b - buffer);
    (void)i;

    {
        /* without its window the console goes to stdout; with it,
         * COD2_CONSOLE_STDOUT copies it there too, to read a client run
         * under Wine or from a script */
        static int toStdout = -1;
        if (toStdout < 0)
            toStdout = getenv("COD2_CONSOLE_STDOUT") != NULL;
        if (!s_wcd.hwndBuffer || toStdout) {
            fputs(buffer, stdout);
            fflush(stdout);
        }
        if (!s_wcd.hwndBuffer)
            return;
    }

    if (bufLen > 0) {
        SendMessage(s_wcd.hwndBuffer, EM_SETREADONLY, FALSE, 0);

        if (GetWindowTextLength(s_wcd.hwndBuffer) + bufLen >= CONSOLE_BUFFER_SIZE) {
            SendMessage(s_wcd.hwndBuffer, EM_SETSEL, 0, CONSOLE_BUFFER_SIZE / 2);
            SendMessage(s_wcd.hwndBuffer, EM_REPLACESEL, FALSE, (LPARAM) "");
        }

        SendMessage(s_wcd.hwndBuffer, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        SendMessage(s_wcd.hwndBuffer, EM_REPLACESEL, FALSE, (LPARAM)buffer);
        SendMessage(s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff);
        SendMessage(s_wcd.hwndBuffer, EM_SCROLLCARET, 0, 0);

        SendMessage(s_wcd.hwndBuffer, EM_SETREADONLY, TRUE, 0);
    }
}

void Sys_SetErrorText(const char *buf)
{
    strncpy(s_wcd.errorString, buf, sizeof(s_wcd.errorString) - 1);
    s_wcd.errorString[sizeof(s_wcd.errorString) - 1] = '\0';

    if (!s_wcd.hwndErrorBox) {
        s_wcd.hwndErrorBox = CreateWindow("static", NULL,
                                          WS_CHILD | WS_VISIBLE | SS_SUNKEN,
                                          6, 5, 526, 30, s_wcd.hWnd, (HMENU)ERRORBOX_ID,
                                          GetModuleHandle(NULL), NULL);
        SendMessage(s_wcd.hwndErrorBox, WM_SETFONT, (WPARAM)s_wcd.hfBufferFont, 0);
        SetWindowText(s_wcd.hwndErrorBox, s_wcd.errorString);

        DestroyWindow(s_wcd.hwndInputLine);
        s_wcd.hwndInputLine = NULL;
    }
}

#endif
