/*=============================================================================
    ReturnToLogin.h - phase 6: the C1 client goes back to the login screen instead
    of getting stuck or closing (NWindow_C1 src/inject/ReturnToLogin.cpp, its
    docs/C1_DISCONNECT_RE.md sections 2-5; our notes:
    docs/notas/F6_ReturnToLogin.md).

     1. The game server picked on the server list cannot be reached: stock sits
        silently on a list whose login connection is already closed. Now: login
        screen + "Unable to connect to server." (systemmsg 90).
     2. The "disconnected from the server" box (dialog 0x1D, systemmsg 127):
        stock OK closes the game. Now OK (or Esc) takes the world down like a
        Restart and goes to the login screen. A server that just closes the
        connection (crash, proxy drop) now also shows that box; stock never
        noticed it and left a dead world on screen.

    Settings: LineageWindow.ini [Connect] ReturnToLogin (1) / TimeoutSeconds (25)
    for 1, DisconnectToLogin (1) for 2. Log tag "returntologin".

    Network.dll (the game socket's two handlers) and Engine.dll
    (UGameEngine::OnServerClose's ILT thunk) stay runtime-patched, checked at
    install: another client build keeps its stock behavior. The NWindow side
    (NWindow_C1: console slot +0x444, the dialog box's Cancel slot, the Tick
    port, stock routines called by address) is our code calling these.
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

class NConsoleWnd;

namespace nwf {

// From our DllMain, then from every Tick while a module is missing: reads [Connect] and hooks
// Network.dll and Engine.dll (Network.dll is mapped with WinDrv, before NWindow.dll).
bool ReturnToLogin_Install();

// NConsoleWnd::Tick, before the engine's tick: the connect watchdog and the disconnect
// transition.
void ReturnToLogin_OnTick(NConsoleWnd* Console);

// NConsoleWnd::QuitGame (console slot 273, called only by the OK of the "disconnected" box):
// true when the transition to the login screen takes over, and the game does not end.
bool ReturnToLogin_QuitGame();

// NCDialogBox::OnCancel, once the box has closed: DialogID is the id it had.
void ReturnToLogin_DialogCancelled(INT DialogID);

}  // namespace nwf

#endif  // NW_FEATURES
