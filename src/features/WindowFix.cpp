/*=============================================================================
    WindowFix.cpp - phase 6: windowed-mode fixes for the C1 client, ported from NWindow_C1
    src/inject/WindowFix.cpp (HEAD 4e56326; RE notes in its docs/WINDOW_FIX.md, ours in
    docs/notas/F6_WindowFix.md).

    Stock C1 assumes the viewport client area sits at screen (0,0):
      - WinDrv OpenWindow (body RVA 0x4310) creates the viewport as a bare WS_POPUP at
        CW_USEDEFAULT, which is (0,0) for popups: no caption, the window can't be moved.
        ResizeViewport only puts WS_CAPTION|WS_THICKFRAME back after a trip through
        fullscreen. (C2 WinDrv creates WS_OVERLAPPEDWINDOW instead.)
      - NWindow hit-tests its UI in *screen* coordinates while every UI rect is in
        *client* coordinates: DirectInput clicks become WM_xBUTTONDOWN with
        lParam = GetCursorPos() (NConsoleWnd::MasterConsoleEventProcess), WM_MOUSEMOVE goes
        through ClientToScreen (NCVirtualWndMain::ClientToScreenPacked), hover / drag-snap /
        enchant check read GetCursorPos (NCVirtualWndMain::PaintToolTips, NCWnd::Snap,
        NConsoleWnd::UpdateCursor). C2 NWindow fixed the same spots with ScreenToClient.
      - Mouse buttons and wheel come only from DirectInput (NONEXCLUSIVE|FOREGROUND,
        UpdateInput -> CauseInputEvent): a click on the caption or outside the window
        still reaches the game while it is the foreground window.
      - WM_SIZE(SIZE_MINIMIZED) calls RenDev->SetRes(0,0): D3DDrv clamps to 1x1 and the
        window comes back 1x1.

    Fixes (LineageWindow.ini [Window] keys, now compiled in: src/features/NwFeatureConfig.cpp):
      Mode              Auto|Framed|Borderless - movable captioned window; Auto goes
                        borderless when the framed window does not fit the work area
      FixUiMouse        NWindow's GetCursorPos -> client coords, ClientToScreen -> identity
                        (NWindow_C1 patched NWindow's IAT; here the call sites in our NWindow
                        code call WindowFix_GetCursorPos / WindowFix_ClientToScreen)
      FilterClicks      mouse presses count only over the client area
      ConfineDrag       cursor stays in the client area while an accepted button is held
      ConfineFullscreen cursor stays on the game screen in fullscreen (multi-monitor)
      SafeMinimize      minimize/restore without touching the render device
      ConfirmClose      ask before the caption close button ends the game
      X, Y              last window position (saved in the registry when the user moves the window)
      Fullscreen        the mode the game starts in (not in NWindow_C1; the registry only: the last
                        mode the player used, from Alt+Enter or the options page "Full Screen")

    Window <-> fullscreen (not in NWindow_C1): WinDrv's ToggleFullscreen opens the device at the
    l2.ini sizes of the client (UClient Windowed/FullscreenViewportX/Y), not at the one of
    Option.ini the UI is laid out for. D3DDrv's fullscreen SetRes takes the nearest display mode
    and overwrites the size it was given (FindBestMatchingMode, "Best-match display mode"), so on
    a monitor without the mode of Option.ini the viewport comes out another size than the UI; with
    no mode at all it fails and the viewport loses its render device. See docs/notas/F6_WindowFix.md.
=============================================================================*/
#include "nwindow/NWindow.h"
#include "nwindow/NConsoleWnd.h"
#include "features/WindowFix.h"

#include <windowsx.h>

namespace {

// WinDrv.dll (C1, ImageBase 0x11100000) is an /INCREMENTAL build: every call, direct or
// through the vtable, goes through an ILT thunk "E9 rel32" -> body. Retargeting the rel32
// hooks the function without touching its body.
const DWORD RVA_ResizeViewportThunk = 0x100A;    // vtable +0x78
const DWORD RVA_ResizeViewportBody = 0x6570;
const DWORD RVA_OpenWindowThunk = 0x1136;        // vtable +0x84
const DWORD RVA_OpenWindowBody = 0x4310;
const DWORD RVA_CauseInputEventThunk = 0x11A9;   // 28 call sites, all via the thunk
const DWORD RVA_CauseInputEventBody = 0x51F0;
// UWindowsViewport::SetMouseCapture(capture, clip, onlyFocus) - called directly, never hooked
const DWORD RVA_SetMouseCaptureBody = 0x52B0;
// UWindowsViewport::ToggleFullscreen() - the three callers (Exec TOGGLEFULLSCREEN, ViewportWndProc
// twice: Alt+Enter) all go through the thunk
const DWORD RVA_ToggleFullscreenThunk = 0x101E;
const DWORD RVA_ToggleFullscreenBody = 0x6440;

// UWindowsViewport fields (C1 WinDrv disasm: OpenWindow, ResizeViewport, WM_SIZE). WinDrv's
// object, not NWindow's: SizeX is UViewport::SizeX of include/ue2/UE2Engine.h.
const DWORD OFF_SizeX = 0x88;
const DWORD OFF_SizeY = 0x8C;
const DWORD OFF_Window = 0x198;       // WWindowsViewportWindow*
const DWORD OFF_WindowHwnd = 0x04;    // WWindow::hWnd
const DWORD OFF_HoldCount = 0x1A8;    // ViewportWndProc only does DefWindowProc while != 0
const DWORD OFF_BlitFlags = 0x1AC;
// SetMouseCapture stores the cursor position here while captured (-1 = not captured) and hides
// the cursor with ShowCursor(0) until the display count is negative. Only the matching release
// shows it again, so a capture lost to a focus change leaves the cursor invisible for good.
const DWORD OFF_CaptureX = 0x1CC;
const DWORD BLIT_Fullscreen = 0x01;
const DWORD BLIT_Temporary = 0x02;

// EInputKey / EInputAction (UE2)
const INT IK_LeftMouse = 0x01;
const INT IK_RightMouse = 0x02;
const INT IK_MiddleMouse = 0x04;
const INT IK_MouseW = 0xE7;
const INT IK_MouseWheelUp = 0xEC;
const INT IK_MouseWheelDown = 0xED;
const INT IST_Press = 1;
const INT IST_Release = 3;

const INT NO_POS = -32000;
const LONG FRAMED_ADD = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
const LONG FRAMED_DEL = WS_POPUP | WS_THICKFRAME | WS_MAXIMIZEBOX;
const LONG FRAME_BITS = WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
const DWORD CLICK_GUARD_MS = 250;   // after our own modal UI, drop the clicks it received

const WCHAR PROP_ORIG_PROC[] = L"LineageWindow.OrigWndProc";

enum EWindowMode
{
    MODE_Auto,
    MODE_Framed,
    MODE_Borderless,
};

struct FWindowConfig
{
    bool bEnable;
    EWindowMode Mode;
    bool bFixUiMouse;
    bool bFilterClicks;
    bool bConfineDrag;
    bool bConfineFullscreen;
    bool bSafeMinimize;
    bool bConfirmClose;
    INT SavedX;
    INT SavedY;
};

// [Window] Log is read by nwf::LogEnabled (NwFeatureConfig.cpp), the gate of every "wfix:" line
FWindowConfig GCfg = {true, MODE_Auto, true, true, true, true, true, true, NO_POS, NO_POS};
volatile LONG GConfigLoaded = 0;

typedef INT(__thiscall* ResizeViewportFn)(void* Vp, DWORD BlitFlags, INT X, INT Y, INT SaveSize);
typedef void(__thiscall* OpenWindowFn)(void* Vp, DWORD Parent, INT Temporary, INT X, INT Y, INT OpenX,
                                       INT OpenY);
typedef INT(__thiscall* CauseInputEventFn)(void* Vp, INT Key, INT Action, FLOAT Delta);
typedef void(__thiscall* SetMouseCaptureFn)(void* Vp, INT Capture, INT Clip, INT OnlyFocus);
typedef void(__thiscall* ToggleFullscreenFn)(void* Vp);

ResizeViewportFn GOrigResize = NULL;
OpenWindowFn GOrigOpen = NULL;
CauseInputEventFn GOrigInput = NULL;
SetMouseCaptureFn GSetMouseCapture = NULL;
ToggleFullscreenFn GOrigToggle = NULL;

volatile LONG GWinDrvHooked = 0;
volatile LONG GToggleHooked = 0;   // the fullscreen switch is ours (checked apart from the other three)
bool GUiMouse = false;   // NWindow's cursor calls in client space (NWindow_C1: the IAT patch)

void* GVp = NULL;
HWND GHwnd = NULL;

bool GUserMoved = false;       // user dragged the window this session
bool GMinimized = false;
bool GInSizeMove = false;
INT GSelfMove = 0;             // our own SetWindowPos is in flight
DWORD GHeldButtons = 0;        // accepted presses still held (bit per button)
DWORD GRejected = 0;           // swallowed presses whose release must be swallowed too
DWORD GClickGuardUntil = 0;
bool GClipOwned = false;
RECT GClipRect = {0, 0, 0, 0};

/*-----------------------------------------------------------------------------
    Config.
-----------------------------------------------------------------------------*/

// [Window] Key: the default when missing, empty or not a number (NWindow_C1's own parse, not
// GetPrivateProfileInt's, which reads "yes" as 0)
INT IniWindowInt(const char* Key, INT Default)
{
    char Buf[32];
    nwf::IniString("Window", Key, "", Buf, sizeof(Buf));
    if (!Buf[0])
        return Default;
    const char* P = Buf;
    while (*P == ' ' || *P == '\t')
        ++P;
    const bool bNeg = (*P == '-');
    if (*P == '-' || *P == '+')
        ++P;
    if (*P < '0' || *P > '9')
        return Default;
    INT Value = 0;
    for (; *P >= '0' && *P <= '9'; ++P)
        Value = Value * 10 + (*P - '0');
    return bNeg ? -Value : Value;
}

bool IniWindowBool(const char* Key, bool Default)
{
    return IniWindowInt(Key, Default ? 1 : 0) != 0;
}

void LoadConfig()
{
    GCfg.bEnable = IniWindowBool("Enable", true);
    char Mode[32];
    nwf::IniString("Window", "Mode", "Auto", Mode, sizeof(Mode));
    if (lstrcmpiA(Mode, "Framed") == 0)
        GCfg.Mode = MODE_Framed;
    else if (lstrcmpiA(Mode, "Borderless") == 0)
        GCfg.Mode = MODE_Borderless;
    else
        GCfg.Mode = MODE_Auto;
    GCfg.bFixUiMouse = IniWindowBool("FixUiMouse", true);
    GCfg.bFilterClicks = IniWindowBool("FilterClicks", true);
    GCfg.bConfineDrag = IniWindowBool("ConfineDrag", true);
    GCfg.bConfineFullscreen = IniWindowBool("ConfineFullscreen", true);
    GCfg.bSafeMinimize = IniWindowBool("SafeMinimize", true);
    GCfg.bConfirmClose = IniWindowBool("ConfirmClose", true);
    GCfg.SavedX = IniWindowInt("X", NO_POS);
    GCfg.SavedY = IniWindowInt("Y", NO_POS);

    nwf::Log("wfix", "config enable=%d mode=%d uiMouse=%d filter=%d confineDrag=%d confineFs=%d "
             "safeMin=%d confirmClose=%d saved=%d,%d",
             GCfg.bEnable, (INT)GCfg.Mode, GCfg.bFixUiMouse, GCfg.bFilterClicks, GCfg.bConfineDrag,
             GCfg.bConfineFullscreen, GCfg.bSafeMinimize, GCfg.bConfirmClose, GCfg.SavedX, GCfg.SavedY);
}

void SavePosition(INT X, INT Y)
{
    nwf::IniWriteInt("Window", "X", X);
    nwf::IniWriteInt("Window", "Y", Y);
}

/*-----------------------------------------------------------------------------
    Viewport helpers (WinDrv's UWindowsViewport).
-----------------------------------------------------------------------------*/

INT& VpInt(void* Vp, DWORD Offset)
{
    return *(INT*)((BYTE*)Vp + Offset);
}

DWORD VpBlitFlags(void* Vp)
{
    return *(DWORD*)((BYTE*)Vp + OFF_BlitFlags);
}

HWND ViewportHwnd(void* Vp)
{
    if (!Vp)
        return NULL;
    __try
    {
        BYTE* Wnd = *(BYTE**)((BYTE*)Vp + OFF_Window);
        return Wnd ? *(HWND*)(Wnd + OFF_WindowHwnd) : NULL;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }
}

bool ViewportFullscreen(void* Vp)
{
    return Vp && (VpBlitFlags(Vp) & BLIT_Fullscreen) != 0;
}

INT RectW(const RECT& R)
{
    return R.right - R.left;
}

INT RectH(const RECT& R)
{
    return R.bottom - R.top;
}

bool ClientRectScreen(HWND h, RECT* R)
{
    if (!GetClientRect(h, R))
        return false;
    MapWindowPoints(h, NULL, (POINT*)R, 2);
    return RectW(*R) > 0 && RectH(*R) > 0;
}

bool CursorInClient(HWND h)
{
    if (!h)
        return true;   // viewport unknown: behave like stock
    if (GetForegroundWindow() != h)
        return false;
    POINT P;
    if (!GetCursorPos(&P))
        return true;
    HWND At = WindowFromPoint(P);
    if (At != h && GetAncestor(At, GA_ROOT) != h)
        return false;   // another window covers that spot
    RECT Rc;
    return ClientRectScreen(h, &Rc) && PtInRect(&Rc, P);
}

// Single owner of ClipCursor: drag confinement or fullscreen confinement, else none.
void UpdateClip()
{
    HWND h = GHwnd;
    bool bWant = false;
    RECT R = {0, 0, 0, 0};
    if (h && !GMinimized && !GInSizeMove && !IsIconic(h) && GetForegroundWindow() == h)
    {
        if (GHeldButtons && GCfg.bConfineDrag)
            bWant = ClientRectScreen(h, &R);
        else if (GCfg.bConfineFullscreen && ViewportFullscreen(GVp))
            bWant = GetWindowRect(h, &R) != FALSE;
    }
    if (bWant)
    {
        RECT Cur = {0, 0, 0, 0};
        const bool bLost = GetClipCursor(&Cur) && !EqualRect(&Cur, &R);
        if (!GClipOwned || !EqualRect(&GClipRect, &R) || bLost)
        {
            if (ClipCursor(&R))
            {
                if (!GClipOwned)
                    nwf::Log("wfix", "clip ON %d,%d-%d,%d (%s)", R.left, R.top, R.right, R.bottom,
                             GHeldButtons ? "drag" : "fullscreen");
                GClipOwned = true;
                GClipRect = R;
            }
        }
    }
    else if (GClipOwned)
    {
        ClipCursor(NULL);
        GClipOwned = false;
        nwf::Log("wfix", "clip OFF");
    }
}

void ResetButtons()
{
    GHeldButtons = 0;
    GRejected = 0;
}

bool AnyMouseButtonDown()
{
    return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) || (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ||
           (GetAsyncKeyState(VK_MBUTTON) & 0x8000);
}

// Undo a mouse capture the game cannot release itself.
//
// Mouse look captures the mouse and hides the cursor; the release only happens when the game
// sees the button go up. Lose the window in between (Alt+Tab, minimize, the close prompt) and
// that event never arrives with DirectInput in foreground mode: the capture is gone but the
// cursor stays hidden until the process exits. Calling the stock SetMouseCapture(0) restores
// the cursor position, the display count and the clip.
void ReleaseStuckCapture(const char* Why)
{
    void* Vp = GVp;
    if (!GSetMouseCapture || !Vp || ViewportFullscreen(Vp))
        return;   // fullscreen keeps the mouse captured on purpose
    if (VpInt(Vp, OFF_CaptureX) == -1)
        return;   // not captured
    __try
    {
        GSetMouseCapture(Vp, 0, 0, 0);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return;
    }
    nwf::Log("wfix", "mouse capture released (%s): cursor restored", Why);
}

/*-----------------------------------------------------------------------------
    Window layout.
-----------------------------------------------------------------------------*/

void ClampInto(RECT& Wr, const RECT& Work)
{
    INT X = Wr.left;
    INT Y = Wr.top;
    const INT W = RectW(Wr);
    const INT H = RectH(Wr);
    if (W <= RectW(Work))
    {
        if (X < Work.left)
            X = Work.left;
        if (X + W > Work.right)
            X = Work.right - W;
    }
    else
        X = Work.left;
    if (H <= RectH(Work))
    {
        if (Y < Work.top)
            Y = Work.top;
        if (Y + H > Work.bottom)
            Y = Work.bottom - H;
    }
    else
        Y = Work.top;   // caption stays reachable
    OffsetRect(&Wr, X - Wr.left, Y - Wr.top);
}

void LayoutWindowed(void* Vp, HWND h, INT Cw, INT Ch)
{
    const LONG Style = GetWindowLongW(h, GWL_STYLE);
    const LONG ExStyle = GetWindowLongW(h, GWL_EXSTYLE);
    RECT Cur = {0, 0, 0, 0};
    GetWindowRect(h, &Cur);

    // Saved spot until the user moves the window this session, then wherever it is.
    bool bUseSaved = !GUserMoved && GCfg.SavedX != NO_POS && GCfg.SavedY != NO_POS;
    HMONITOR Mon = NULL;
    if (bUseSaved)
    {
        POINT P = {GCfg.SavedX + 32, GCfg.SavedY + 8};   // a point on the saved caption
        Mon = MonitorFromPoint(P, MONITOR_DEFAULTTONULL);
        bUseSaved = Mon != NULL;
    }
    if (!Mon)
        Mon = MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST);
    MONITORINFO Mi;
    ZeroMemory(&Mi, sizeof(Mi));
    Mi.cbSize = sizeof(Mi);
    if (!GetMonitorInfoW(Mon, &Mi))
        return;
    const RECT& Work = Mi.rcWork;
    const RECT& Full = Mi.rcMonitor;

    const LONG Framed = (Style & ~FRAMED_DEL) | FRAMED_ADD;
    RECT Fr = {0, 0, Cw, Ch};
    AdjustWindowRectEx(&Fr, (DWORD)Framed, FALSE, (DWORD)ExStyle);
    const bool bBorderless =
        GCfg.Mode == MODE_Borderless ||
        (GCfg.Mode == MODE_Auto && (RectW(Fr) > RectW(Work) || RectH(Fr) > RectH(Work)));

    LONG NewStyle;
    RECT Wr;
    if (bBorderless)
    {
        NewStyle = (Style & ~FRAME_BITS) | WS_POPUP;
        const INT X = Full.left + (RectW(Full) > Cw ? (RectW(Full) - Cw) / 2 : 0);
        const INT Y = Full.top + (RectH(Full) > Ch ? (RectH(Full) - Ch) / 2 : 0);
        SetRect(&Wr, X, Y, X + Cw, Y + Ch);
    }
    else
    {
        NewStyle = Framed;
        INT X, Y;
        if (bUseSaved)
        {
            X = GCfg.SavedX;
            Y = GCfg.SavedY;
        }
        else if (!GUserMoved)
        {
            X = Work.left + (RectW(Work) - RectW(Fr)) / 2;
            Y = Work.top + (RectH(Work) - RectH(Fr)) / 2;
        }
        else
        {
            X = Cur.left;
            Y = Cur.top;
        }
        SetRect(&Wr, X, Y, X + RectW(Fr), Y + RectH(Fr));
        ClampInto(Wr, Work);
    }

    if (NewStyle == Style && EqualRect(&Wr, &Cur))
        return;

    // HoldCount makes ViewportWndProc leave the resulting messages to DefWindowProc
    // (same guard WinDrv uses around its own MoveWindow calls).
    INT& Hold = VpInt(Vp, OFF_HoldCount);
    ++Hold;
    ++GSelfMove;
    if (NewStyle != Style)
        SetWindowLongW(h, GWL_STYLE, NewStyle);
    SetWindowPos(h, NULL, Wr.left, Wr.top, RectW(Wr), RectH(Wr),
                 SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    --GSelfMove;
    --Hold;

    RECT Cr = {0, 0, 0, 0};
    GetClientRect(h, &Cr);
    nwf::Log("wfix", "layout %s %dx%d at %d,%d style %08X->%08X client %dx%d%s",
             bBorderless ? "borderless" : "framed", Cw, Ch, Wr.left, Wr.top, Style, NewStyle, RectW(Cr),
             RectH(Cr), (RectW(Cr) != Cw || RectH(Cr) != Ch) ? " CLIENT MISMATCH" : "");
}

void StripFrameForFullscreen(HWND h)
{
    const LONG Style = GetWindowLongW(h, GWL_STYLE);
    const LONG NewStyle = Style & ~(WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    if (NewStyle != Style)
        SetWindowLongW(h, GWL_STYLE, NewStyle);
}

void RememberPosition(HWND h)
{
    if (!GVp || ViewportFullscreen(GVp) || IsIconic(h))
        return;
    if ((GetWindowLongW(h, GWL_STYLE) & WS_CAPTION) != WS_CAPTION)
        return;   // only framed placement is the user's choice
    RECT R;
    if (!GetWindowRect(h, &R))
        return;
    GUserMoved = true;
    if (R.left == GCfg.SavedX && R.top == GCfg.SavedY)
        return;
    GCfg.SavedX = R.left;
    GCfg.SavedY = R.top;
    SavePosition(R.left, R.top);
    nwf::Log("wfix", "position saved %d,%d", R.left, R.top);
}

bool ConfirmClose(HWND h)
{
    if (!GCfg.bConfirmClose || ViewportFullscreen(GVp))
        return true;
    ResetButtons();
    UpdateClip();
    const INT Answer = MessageBoxW(h, L"Deseja fechar o Lineage II?", L"Lineage II",
                                   MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SETFOREGROUND);
    // DirectInput buffered the clicks on the message box; UpdateInput will replay them
    // over the client area on the next frame.
    GClickGuardUntil = GetTickCount() + CLICK_GUARD_MS;
    return Answer == IDYES;
}

void __fastcall HookToggleFullscreen(void* Vp, void*);   // below, with the other WinDrv hooks

LRESULT CALLBACK ViewportProc(HWND h, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    const WNDPROC Orig = (WNDPROC)GetPropW(h, PROP_ORIG_PROC);
    if (!Orig)
        return DefWindowProcW(h, Msg, wParam, lParam);
    if (Msg == WM_MOVE)
    {
        // Never forward: WM_MOVE (3) is in NWindow's handled-message table (GDispatchedMsgs,
        // stock 0x1010B268) and NControl::WndProc turns it into "move the topmost UI window to
        // (x,y)" (vt+0xF0 -> vt+0xB0) with lParam = the viewport's *screen* origin, so each
        // drag threw a UI window across (and out of) the client area. WinDrv ignores WM_MOVE and
        // the stock (0,0) popup never produced one after NWindow came up.
        if (!GSelfMove)
            UpdateClip();
        return DefWindowProcW(h, Msg, wParam, lParam);
    }
    if (GSelfMove)
        return CallWindowProcW(Orig, h, Msg, wParam, lParam);

    switch (Msg)
    {
    case WM_SIZE:
        // Fullscreen WM_SIZE never reaches SetRes in stock; only the windowed path does.
        if (wParam == SIZE_MINIMIZED)
        {
            GMinimized = true;
            ResetButtons();
            ReleaseStuckCapture("minimized");
            UpdateClip();
            if (GCfg.bSafeMinimize && GVp && !ViewportFullscreen(GVp))
            {
                nwf::Log("wfix", "minimized (render device untouched)");
                return 0;   // stock: SetRes(0,0) -> 1x1 back buffer and window
            }
        }
        else if (GMinimized)
        {
            GMinimized = false;
            if (GCfg.bSafeMinimize && GVp && !ViewportFullscreen(GVp) &&
                LOWORD(lParam) == VpInt(GVp, OFF_SizeX) && HIWORD(lParam) == VpInt(GVp, OFF_SizeY))
            {
                nwf::Log("wfix", "restored %dx%d (same size, no SetRes)", LOWORD(lParam), HIWORD(lParam));
                UpdateClip();
                return 0;
            }
        }
        break;

    case WM_ACTIVATEAPP:
    case WM_ACTIVATE:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    {
        const bool bLeaving = Msg == WM_KILLFOCUS || (Msg == WM_ACTIVATEAPP && !wParam) ||
                              (Msg == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE);
        if (bLeaving)
            ResetButtons();
        const LRESULT Result = CallWindowProcW(Orig, h, Msg, wParam, lParam);
        // The button that started a mouse look is not held any more (or cannot be seen while
        // we are in the background): drop a capture the game would never release.
        if (bLeaving || !AnyMouseButtonDown())
            ReleaseStuckCapture(bLeaving ? "focus lost" : "focus gained, no button held");
        UpdateClip();
        return Result;
    }

    case WM_ENTERSIZEMOVE:
        GInSizeMove = true;
        UpdateClip();
        break;

    case WM_EXITSIZEMOVE:
        GInSizeMove = false;
        RememberPosition(h);
        UpdateClip();
        break;

    case WM_DISPLAYCHANGE:
    {
        const LRESULT Result = CallWindowProcW(Orig, h, Msg, wParam, lParam);
        UpdateClip();
        return Result;
    }

    case WM_MOUSEWHEEL:
        // Real wheel messages reach NWindow with screen coordinates; the rest of its
        // input is now in client space.
        if (GCfg.bFixUiMouse)
        {
            POINT P = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(h, &P);
            lParam = MAKELPARAM(P.x, P.y);
        }
        break;

    case WM_SYSKEYDOWN:
        // Alt+Enter. WinDrv's own handler never sees it in the game: it asks NWindow's console first
        // (vtable +0x3B8, DispatchWndMsgX), which hands every key to HookShortCutKey while playing
        // and that one takes every system key (returns 1); in the lobby the console takes every
        // message but a left click. Only the login screen let it through. It is ours here, before
        // the console: one switch whatever the screen. A held key repeats the message: once.
        if (wParam == VK_RETURN && GToggleHooked && GVp)
        {
            const bool bRepeat = (lParam & 0x40000000) != 0;
            nwf::Log("wfix", "Alt+Enter%s", bRepeat ? " (key repeat, ignored)" : (IsIconic(h) ? " (minimized, ignored)" : ""));
            if (!bRepeat && !IsIconic(h))
                HookToggleFullscreen(GVp, NULL);
            return 0;
        }
        break;

    case WM_SYSCOMMAND:
        switch (wParam & 0xFFF0)
        {
        case SC_KEYMENU:
            // A system menu now exists: Alt / Alt+letter would enter menu mode (and beep).
            // Keep Alt+Space for the window menu while framed.
            if (lParam != VK_SPACE || ViewportFullscreen(GVp) || !(GetWindowLongW(h, GWL_STYLE) & WS_SYSMENU))
                return 0;
            break;
        case SC_CLOSE:
            if (!ConfirmClose(h))
                return 0;
            break;
        }
        break;

    case WM_MENUCHAR:
        return MAKELRESULT(0, MNC_CLOSE);   // no beep for Alt+key

    case WM_DESTROY:
    {
        ResetButtons();
        if (GClipOwned)
        {
            ClipCursor(NULL);
            GClipOwned = false;
        }
        const LRESULT Result = CallWindowProcW(Orig, h, Msg, wParam, lParam);
        if (GHwnd == h)
        {
            GHwnd = NULL;
            GVp = NULL;
        }
        nwf::Log("wfix", "viewport window destroyed");
        return Result;
    }

    case WM_NCDESTROY:
    {
        const LRESULT Result = CallWindowProcW(Orig, h, Msg, wParam, lParam);
        RemovePropW(h, PROP_ORIG_PROC);
        return Result;
    }
    }
    return CallWindowProcW(Orig, h, Msg, wParam, lParam);
}

void Attach(void* Vp, HWND h)
{
    GVp = Vp;
    if (GHwnd == h)
        return;
    GHwnd = h;
    const WNDPROC Cur = (WNDPROC)GetWindowLongPtrW(h, GWLP_WNDPROC);
    if (Cur != ViewportProc && !GetPropW(h, PROP_ORIG_PROC))
    {
        SetPropW(h, PROP_ORIG_PROC, (HANDLE)Cur);
        SetWindowLongPtrW(h, GWLP_WNDPROC, (LONG_PTR)ViewportProc);
    }
    nwf::Log("wfix", "viewport %p hwnd %p attached", Vp, h);
}

void AfterResize(void* Vp)
{
    HWND h = ViewportHwnd(Vp);
    if (!h || !IsWindow(h))
        return;
    if (GetWindowLongW(h, GWL_STYLE) & WS_CHILD)
        return;   // editor / HWND= embedding
    Attach(Vp, h);

    const DWORD Blit = VpBlitFlags(Vp);
    if (Blit & BLIT_Temporary)
        return;
    if (Blit & BLIT_Fullscreen)
    {
        StripFrameForFullscreen(h);
        UpdateClip();
        return;
    }
    if (IsIconic(h))
        return;
    const INT Cw = VpInt(Vp, OFF_SizeX);
    const INT Ch = VpInt(Vp, OFF_SizeY);
    if (Cw <= 0 || Ch <= 0)
        return;
    LayoutWindowed(Vp, h, Cw, Ch);
    UpdateClip();
}

/*-----------------------------------------------------------------------------
    WinDrv hooks (__fastcall + dummy EDX == __thiscall).
-----------------------------------------------------------------------------*/

INT __fastcall HookResizeViewport(void* Vp, void*, DWORD BlitFlags, INT X, INT Y, INT SaveSize)
{
    const INT Result = GOrigResize(Vp, BlitFlags, X, Y, SaveSize);
    AfterResize(Vp);
    return Result;
}

void __fastcall HookOpenWindow(void* Vp, void*, DWORD Parent, INT Temporary, INT X, INT Y, INT OpenX,
                               INT OpenY)
{
    GOrigOpen(Vp, Parent, Temporary, X, Y, OpenX, OpenY);
    AfterResize(Vp);
}

/*-----------------------------------------------------------------------------
    Window <-> fullscreen.
-----------------------------------------------------------------------------*/

// The UI has its size (Option.ini, through GSEKScreenX/Y) once NConsoleWnd::Initialize is done.
// Before that the stock switch runs untouched.
bool UiSizeKnown()
{
    return GConsole && GConsole->bInitialized && GConsole->Engine && GConsole->Engine->Client &&
           GSEKScreenX > 0 && GSEKScreenY > 0;
}

// The monitor lists a display mode of this size (any depth). With no size to check, or no list
// to check it against, the device decides, as in the stock client.
bool FullscreenModeExists(INT X, INT Y)
{
    if (X <= 0 || Y <= 0 || !GConsole || !GConsole->Engine)
        return true;
    __try
    {
        URenderDevice* Device = GConsole->Engine->GRenDev;
        if (!Device)
            return true;
        INT Widths[100], Heights[100], Bits[100];
        const INT Count = Device->GetRes(100, Widths, Heights, Bits);
        if (Count <= 0)
            return true;
        for (INT i = 0; i < Count && i < 100; ++i)
            if (Widths[i] == X && Heights[i] == Y)
                return true;
        return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return true;
    }
}

// ToggleFullscreen / EndFullscreen read these four from the client: both ways of the switch now
// end at the size of the UI
void SyncClientSizes()
{
    UClient* Client = GConsole->Engine->Client;
    Client->WindowedViewportX = GSEKScreenX;
    Client->WindowedViewportY = GSEKScreenY;
    Client->FullscreenViewportX = GSEKScreenX;
    Client->FullscreenViewportY = GSEKScreenY;
}

void TellNoFullscreenMode(HWND h, INT X, INT Y)
{
    WCHAR Text[512];
    wsprintfW(Text,
              L"O modo de tela cheia não existe em %dx%d neste monitor.\n\n"
              L"Escolha outra resolução em Options > Video (a lista só mostra as que o "
              L"monitor aceita) e tente de novo.",
              X, Y);
    ResetButtons();
    UpdateClip();
    MessageBoxW(h, Text, L"Lineage II", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
    GClickGuardUntil = GetTickCount() + CLICK_GUARD_MS;   // as after the close prompt
}

// diagnosis (builds with the log only): where the window and the display are around a switch
void LogWindowState(const char* When, void* Vp)
{
    if (!nwf::LogEnabled())
        return;
    HWND h = ViewportHwnd(Vp);
    RECT R = {0, 0, 0, 0};
    if (h)
        GetWindowRect(h, &R);
    DEVMODEW Dm;
    ZeroMemory(&Dm, sizeof(Dm));
    Dm.dmSize = sizeof(Dm);
    EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &Dm);
    nwf::Log("wfix", "%s: hwnd %p style %08X ex %08X rect %d,%d-%d,%d fg=%d iconic=%d blit %08X desktop %lux%lu@%lu "
             "thread %lu (window thread %lu)",
             When, h, h ? GetWindowLongW(h, GWL_STYLE) : 0, h ? GetWindowLongW(h, GWL_EXSTYLE) : 0, R.left, R.top,
             R.right, R.bottom, h && GetForegroundWindow() == h, h && IsIconic(h), VpBlitFlags(Vp), Dm.dmPelsWidth,
             Dm.dmPelsHeight, Dm.dmDisplayFrequency, GetCurrentThreadId(),
             h ? GetWindowThreadProcessId(h, NULL) : 0);
}

void __fastcall HookToggleFullscreen(void* Vp, void*)
{
    const bool bUi = UiSizeKnown();
    LogWindowState("toggle begins", Vp);
    if (bUi)
    {
        if (!ViewportFullscreen(Vp) && !FullscreenModeExists(GSEKScreenX, GSEKScreenY))
        {
            nwf::Log("wfix", "fullscreen refused: no %dx%d display mode", GSEKScreenX, GSEKScreenY);
            TellNoFullscreenMode(ViewportHwnd(Vp), GSEKScreenX, GSEKScreenY);
            return;
        }
        SyncClientSizes();
    }
    GOrigToggle(Vp);
    LogWindowState("toggle ended", Vp);
    if (bUi)
    {
        const bool bFullscreen = ViewportFullscreen(Vp);
        nwf::IniWriteInt("Window", "Fullscreen", bFullscreen ? 1 : 0);
        nwf::Log("wfix", "switched to %s: viewport %dx%d, UI %dx%d%s", bFullscreen ? "fullscreen" : "window",
                 VpInt(Vp, OFF_SizeX), VpInt(Vp, OFF_SizeY), GSEKScreenX, GSEKScreenY,
                 (VpInt(Vp, OFF_SizeX) != GSEKScreenX || VpInt(Vp, OFF_SizeY) != GSEKScreenY)
                     ? " SIZE MISMATCH" : "");
    }
}

DWORD ButtonBit(INT Key)
{
    switch (Key)
    {
    case IK_LeftMouse:
        return 1;
    case IK_RightMouse:
        return 2;
    case IK_MiddleMouse:
        return 4;
    }
    return 0;
}

bool IsWheel(INT Key)
{
    return Key == IK_MouseW || Key == IK_MouseWheelUp || Key == IK_MouseWheelDown;
}

INT __fastcall HookCauseInputEvent(void* Vp, void*, INT Key, INT Action, FLOAT Delta)
{
    const DWORD Bit = ButtonBit(Key);
    if (Bit)
    {
        if (Action == IST_Press)
        {
            HWND h = ViewportHwnd(Vp);
            const bool bGuard = GClickGuardUntil && (INT)(GetTickCount() - GClickGuardUntil) < 0;
            if (GCfg.bFilterClicks && (bGuard || !CursorInClient(h)))
            {
                GRejected |= Bit;
                nwf::Log("wfix", "press key %d dropped (%s)", Key, bGuard ? "modal guard" : "outside client");
                return 0;
            }
            GRejected &= ~Bit;
            GHeldButtons |= Bit;
            UpdateClip();
        }
        else if (Action == IST_Release)
        {
            if (GRejected & Bit)
            {
                GRejected &= ~Bit;
                return 0;
            }
            GHeldButtons &= ~Bit;
            UpdateClip();
        }
    }
    else if (IsWheel(Key) && GCfg.bFilterClicks && !CursorInClient(ViewportHwnd(Vp)))
        return 0;
    return GOrigInput(Vp, Key, Action, Delta);
}

// push ebp / mov ebp,esp / push -1 / push <scope table>  (SEH frame prologue)
const BYTE SEH_PROLOGUE[] = {0x55, 0x8B, 0xEC, 0x6A, 0xFF, 0x68};

bool HasSehPrologue(const BYTE* Body)
{
    for (DWORD i = 0; i < sizeof(SEH_PROLOGUE); ++i)
        if (Body[i] != SEH_PROLOGUE[i])
            return false;
    return true;
}

// The thunk at ThunkRva is "E9 rel32" to BodyRva, which starts with the SEH prologue - or it is
// already ours
bool CheckIlt(BYTE* Mod, DWORD ThunkRva, DWORD BodyRva, void* Hook)
{
    BYTE* Thunk = Mod + ThunkRva;
    if (Thunk[0] != 0xE9)
        return false;
    BYTE* Target = Thunk + 5 + *(INT*)(Thunk + 1);
    if (Target == (BYTE*)Hook)
        return true;
    if (Target != Mod + BodyRva)
        return false;
    return HasSehPrologue(Target);
}

bool RetargetIlt(BYTE* Mod, DWORD ThunkRva, void* Hook)
{
    BYTE* Thunk = Mod + ThunkRva;
    DWORD Old = 0;
    if (!VirtualProtect(Thunk, 5, PAGE_EXECUTE_READWRITE, &Old))
        return false;
    *(INT*)(Thunk + 1) = (INT)((BYTE*)Hook - (Thunk + 5));
    VirtualProtect(Thunk, 5, Old, &Old);
    FlushInstructionCache(GetCurrentProcess(), Thunk, 5);
    return true;
}

bool HookWinDrv()
{
    if (GWinDrvHooked)
        return true;
    BYTE* Mod = (BYTE*)GetModuleHandleW(L"WinDrv.dll");
    if (!Mod)
    {
        nwf::Log("wfix", "WinDrv.dll not loaded");
        return false;
    }
    void* HResize = (void*)&HookResizeViewport;
    void* HOpen = (void*)&HookOpenWindow;
    void* HInput = (void*)&HookCauseInputEvent;
    // All three or none: another WinDrv build keeps its stock behaviour.
    if (!CheckIlt(Mod, RVA_ResizeViewportThunk, RVA_ResizeViewportBody, HResize) ||
        !CheckIlt(Mod, RVA_OpenWindowThunk, RVA_OpenWindowBody, HOpen) ||
        !CheckIlt(Mod, RVA_CauseInputEventThunk, RVA_CauseInputEventBody, HInput))
    {
        nwf::Log("wfix", "WinDrv.dll at %p is not the expected C1 build - hooks skipped", Mod);
        return false;
    }
    GOrigResize = (ResizeViewportFn)(Mod + RVA_ResizeViewportBody);
    GOrigOpen = (OpenWindowFn)(Mod + RVA_OpenWindowBody);
    GOrigInput = (CauseInputEventFn)(Mod + RVA_CauseInputEventBody);
    // SetMouseCapture is only called by us; check its SEH prologue before trusting the RVA
    if (HasSehPrologue(Mod + RVA_SetMouseCaptureBody))
        GSetMouseCapture = (SetMouseCaptureFn)(Mod + RVA_SetMouseCaptureBody);
    else
    {
        GSetMouseCapture = NULL;
        nwf::Log("wfix", "SetMouseCapture not where expected - stuck-cursor recovery off");
    }
    if (!RetargetIlt(Mod, RVA_ResizeViewportThunk, HResize) || !RetargetIlt(Mod, RVA_OpenWindowThunk, HOpen) ||
        !RetargetIlt(Mod, RVA_CauseInputEventThunk, HInput))
    {
        nwf::Log("wfix", "WinDrv ILT retarget failed");
        return false;
    }
    InterlockedExchange(&GWinDrvHooked, 1);
    nwf::Log("wfix", "WinDrv hooks OK (ResizeViewport, OpenWindow, CauseInputEvent) base=%p", Mod);

    // The fullscreen switch is checked on its own: without it the window keeps working, Alt+Enter
    // is the stock one and the options page has no "Full Screen" box.
    void* HToggle = (void*)&HookToggleFullscreen;
    if (!CheckIlt(Mod, RVA_ToggleFullscreenThunk, RVA_ToggleFullscreenBody, HToggle))
        nwf::Log("wfix", "WinDrv ToggleFullscreen not where expected - Alt+Enter stays stock");
    else
    {
        GOrigToggle = (ToggleFullscreenFn)(Mod + RVA_ToggleFullscreenBody);
        if (RetargetIlt(Mod, RVA_ToggleFullscreenThunk, HToggle))
        {
            InterlockedExchange(&GToggleHooked, 1);
            nwf::Log("wfix", "WinDrv ToggleFullscreen hook OK");
        }
        else
            nwf::Log("wfix", "WinDrv ToggleFullscreen ILT retarget failed - Alt+Enter stays stock");
    }
    return true;
}

}  // namespace

namespace nwf {

void WindowFix_Install()
{
    if (InterlockedCompareExchange(&GConfigLoaded, 1, 0) == 0)
    {
        LoadConfig();
        if (GCfg.bEnable)
            DisableProcessWindowsGhosting();   // long loads must not swap in a ghost window
    }
    if (!GCfg.bEnable)
    {
        Log("windowfix", "install @attach FAIL (disabled or other WinDrv build)");
        return;
    }
    const bool bHooked = HookWinDrv();
    // NWindow_C1 patched NWindow's IAT here, whatever became of the WinDrv hooks (without them
    // the viewport window stays unknown and the calls stay plain)
    GUiMouse = GCfg.bFixUiMouse;
    if (GUiMouse)
        Log("wfix", "NWindow: GetCursorPos/ClientToScreen -> client space");
    Log("windowfix", bHooked ? "install @attach OK" : "install @attach FAIL (disabled or other WinDrv build)");
}

BOOL WindowFix_GetCursorPos(LPPOINT Point)
{
    const BOOL Ok = GetCursorPos(Point);
    HWND h = GUiMouse ? GHwnd : NULL;
    if (Ok && Point && h)
        ScreenToClient(h, Point);
    return Ok;
}

BOOL WindowFix_ClientToScreen(HWND hWnd, LPPOINT Point)
{
    if (GUiMouse && hWnd && hWnd == GHwnd)
        return Point != NULL;   // NWindow already works in client space
    return ClientToScreen(hWnd, Point);
}

bool WindowFix_FullscreenSwitchAvailable()
{
    return GToggleHooked != 0;
}

bool WindowFix_IsFullscreen(bool Stock)
{
    void* Vp = GVp;
    return Vp ? ViewportFullscreen(Vp) : Stock;
}

bool WindowFix_SetFullscreen(bool Fullscreen)
{
    void* Vp = GVp;
    if (!GToggleHooked || !Vp)
        return false;
    if (ViewportFullscreen(Vp) != Fullscreen)
        HookToggleFullscreen(Vp, NULL);
    return ViewportFullscreen(Vp) == Fullscreen;
}

bool WindowFix_StartupFullscreen(bool Stock)
{
    if (!GToggleHooked)
        return Stock;
    const bool bWant = IniInt("Window", "Fullscreen", Stock ? 1 : 0) != 0;
    if (bWant && !FullscreenModeExists(GSEKScreenX, GSEKScreenY))
    {
        Log("wfix", "start in a window: no %dx%d display mode for fullscreen", GSEKScreenX, GSEKScreenY);
        return false;
    }
    return bWant;
}

HWND WindowFix_GameHwnd()
{
    return GHwnd;
}

bool WindowFix_CursorClientPos(POINT* Point)
{
    HWND h = GHwnd;
    if (!h || !Point || !GetCursorPos(Point))
        return false;
    return ScreenToClient(h, Point) != FALSE;
}

bool WindowFix_GameHasFocus()
{
    HWND h = GHwnd;
    return !h || GetForegroundWindow() == h;
}

bool WindowFix_CursorInClient()
{
    return CursorInClient(GHwnd);
}

}  // namespace nwf
