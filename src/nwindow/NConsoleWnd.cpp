/*=============================================================================
    NConsoleWnd.cpp - the in-game console (the NWindow root): its construction and vtable,
    and the system message lines. The requests to the server are in NConsoleWndNet.cpp, the
    data getters in NConsoleWndData.cpp, the engine's callbacks in NConsoleWndEvents.cpp /
    NConsoleWndItems.cpp / NConsoleWndLogin.cpp, the windows in NConsoleWndWindows.cpp, the
    frame and the input in NConsoleWndInput.cpp, the text services in NConsoleWndText.cpp
    and the key actions in NConsoleWndActions.cpp.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NDialogInfo.h"

// 0x100557c0 - the object is zeroed on allocation; these the stock clears again
NConsoleWnd::NConsoleWnd()
{
    bInitialized = 0;
    WndMain = NULL;
    LoginWnd = NULL;
    LobbyWnd = NULL;
    Desktop = NULL;
    DebugWnd = NULL;
    UnusedWnd = NULL;
    MouseWheelDelta = 0;
    DeltaTime = 0;
    bSuspended = 0;
    bWheelHandled = 0;
    bCtrlDown = 0;
    bCtrlDownAlt = 0;
    TutorialEvents = 0;
    bNoChatOnEnter = 0;
    LoadingScreenID = 10001;
}
NW_HOOK_CTOR(0x100557c0, NConsoleWnd)
NW_HOOK_DTOR(0x100558c0, NConsoleWnd)
NW_CHECK_VTABLE_FOR(NConsoleWnd, UObject, 0x100e2978, 285)

// the dialog box of a system message (NDialogInfo::ID): OK only
enum { DIALOG_SYSTEM_MESSAGE = 0x1F };

// 0x10054b60 - a system message into the chat window, or in a dialog box for the ones of
// type 4. The line's validity is read before the id is checked.
void NConsoleWnd::ShowSystemMessage(INT ID, DWORD Color)
{
    FL2SystemMsgData& Msg = GL2GameData.SystemMsg[ID];
    if (!Msg.Unknown0 || ID < 0 || ID >= 3000)
        return;
    if (Msg.Type == 4)
    {
        NDialogInfo Info;
        Info.ID = DIALOG_SYSTEM_MESSAGE;
        Info.bMainBox = 1;
        Info.bInput = 0;
        Info.Owner = NULL;
        appStrcpy(Info.Text, *Msg.Message);
        ShowDialog(&Info);
    }
    else
        GetDesktop()->AddSystemMessage(ID, Color);
}
NW_HOOK_METHOD(0x10054b60, NConsoleWnd, ShowSystemMessage)
