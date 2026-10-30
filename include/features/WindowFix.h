/*=============================================================================
    WindowFix.h - phase 6: windowed mode for the C1 viewport (NWindow_C1
    src/inject/WindowFix.cpp, its docs/WINDOW_FIX.md; our notes:
    docs/notas/F6_WindowFix.md).

    A movable frame (Mode Auto / Framed / Borderless), the UI's mouse in client
    space, a click filter, the cursor confined while dragging / in fullscreen,
    minimize without resetting the render device, a confirmation before the
    close button ends the game and the saved window position:
    LineageWindow.ini [Window] (Enable, Mode, FixUiMouse, FilterClicks,
    ConfineDrag, ConfineFullscreen, SafeMinimize, ConfirmClose, X, Y, Log),
    log tag "wfix".

    - WinDrv.dll stays runtime-hooked, as in NWindow_C1: three ILT thunks
      (ResizeViewport, OpenWindow, CauseInputEvent), checked at install
      (another WinDrv build keeps its stock behavior), and the viewport window
      subclassed when WinDrv opens it.
    - NWindow_C1 patched NWindow's IAT (GetCursorPos to client space,
      ClientToScreen of the viewport as identity). Here NWindow is our code:
      its calls go to WindowFix_GetCursorPos / WindowFix_ClientToScreen
      (#if NW_FEATURES blocks at each call site).
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

namespace nwf {

// From our DllMain (OnDllMain): reads [Window] (writing the commented template on the first
// run), then hooks WinDrv.dll, loaded long before NWindow.dll: its viewport window only opens
// after the lobby map, so the hooks are in before it exists.
void WindowFix_Install();

// NWindow's own calls of GetCursorPos / ClientToScreen (NWindow_C1: the IAT patch). With the
// fix on and the viewport window known, the cursor comes in the viewport's client coordinates
// and ClientToScreen of the viewport window leaves the point as it is (every UI rectangle is
// in client coordinates); otherwise the plain API.
BOOL WindowFix_GetCursorPos(LPPOINT Point);
BOOL WindowFix_ClientToScreen(HWND hWnd, LPPOINT Point);

// Window <-> fullscreen (not in NWindow_C1). WinDrv's ToggleFullscreen (Alt+Enter, the TOGGLEFULLSCREEN
// command) is hooked: the stock one opens the device at the l2.ini sizes of the client, and on a
// monitor without that mode D3DDrv takes the nearest one while the UI stays laid out for the size of
// Option.ini; with no mode at all it drops the viewport's render device. The hook first makes both
// sizes the UI's (GSEKScreenX/Y) and refuses the switch to fullscreen when the monitor has no such
// mode, with a message. Every mode the player ends up in is remembered ([Window] Fullscreen).
//
// The switch can be made (the WinDrv hooks are in): the options page shows its "Full Screen" box
// only then.
bool WindowFix_FullscreenSwitchAvailable();
// The mode the viewport is in; Stock while the viewport is not known (no hooks).
bool WindowFix_IsFullscreen(bool Stock);
// Puts the viewport in the wanted mode (the hooked switch); true when it is in it afterwards.
bool WindowFix_SetFullscreen(bool Fullscreen);
// The mode the game starts in (NConsoleWnd::Initialize, after Option.ini gave the UI size): the
// last one the player used, else Stock (the client's StartupFullscreen) - fullscreen only when the
// monitor has a mode of the UI's size, as the stock start would end on another one.
bool WindowFix_StartupFullscreen(bool Stock);

// For the other features (NWindow_C1's WindowFix.h: combat log, craft windows).
// The viewport window (NULL until WinDrv opens it).
HWND WindowFix_GameHwnd();
// The real cursor position in viewport client coordinates (false without a viewport yet).
bool WindowFix_CursorClientPos(POINT* Point);
// The game window is the foreground window (true while the viewport is unknown).
bool WindowFix_GameHasFocus();
// The game is foreground and the cursor is over its client area, not over another window.
bool WindowFix_CursorInClient();

}  // namespace nwf

#endif  // NW_FEATURES
