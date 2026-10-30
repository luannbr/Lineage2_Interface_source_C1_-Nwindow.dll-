/*=============================================================================
    NConsoleWndWindows.cpp - the console's windows: their creation for the game and for the
    pawn viewer mode, their destruction, their positions (WindowsInfo.ini and the defaults),
    the dialog boxes, the message and warning lines of the login screens, the option window,
    the way back from the character creation, the texts after a language change, and the
    debug window's commands.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCActionWnd.h"
#include "nwindow/NCAuthWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDebugWnd.h"
#include "nwindow/NCDetailStatusWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCGMWnd.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCLoadingWnd.h"
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCMainWnd.h"
#include "nwindow/NCMatchWnd.h"
#include "nwindow/NCMenuWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NCMSViewrWnd.h"
#include "nwindow/NCNoticeWnd.h"
#include "nwindow/NCNpcViewrWnd.h"
#include "nwindow/NCNPCHtmlViewer.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCPawnCreateWnd.h"
#include "nwindow/NCPawnSelectWnd.h"
#include "nwindow/NCPawnViewrWnd.h"
#include "nwindow/NCPetitionWnd.h"
#include "nwindow/NCPetWnd.h"
#include "nwindow/NCPrivateShopWnd.h"
#include "nwindow/NCRestartMenuWnd.h"
#include "nwindow/NCServerSelectWnd.h"
#include "nwindow/NCShopWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NCSummonedWnd.h"
#include "nwindow/NCSystemMenuWnd.h"
#include "nwindow/NCTradeWnd.h"
#include "nwindow/NCTrainWnd.h"
#include "nwindow/NCTutorialViewerWnd.h"
#include "nwindow/NCVirtualWndMain.h"
#include "nwindow/NCWarningWnd.h"
#include "nwindow/NDialogInfo.h"

#if NW_FEATURES
// phase 6: WindowFix (the mode the game starts in)
#include "features/WindowFix.h"
#endif

#include <stdarg.h>
#include <stdio.h>

// interface sounds (FL2NetNotify::OnInterfacePlaySound; names ours)
enum { SOUND_WindowOpen = 5, SOUND_WindowClose = 6 };

// sysstring lines
enum { STR_OK = 140, STR_Cancel = 141, STR_Options = 146, STR_Petition = 470 };

// the copyright line under the login screens (a systemmsg line)
enum { SYSMSG_Copyright = 94 };

// package globals (names ours): the debug log switch ("L2Debug", the debug window's lines)
// and the auto login of AutoLog.txt
#if NW_HOOK_MODE
#define GL2DebugLog     NW_GLOBAL(UBOOL, 0x1012d7b8)
#define GIsL2AutoLogOn  NW_GLOBAL(UBOOL, 0x1012d7c0)
#else
extern UBOOL GL2DebugLog;
extern UBOOL GIsL2AutoLogOn;
#endif

// the viewport's window (UViewport::GetWindow, pure in Engine.dll: WinDrv.dll's)
static FORCEINLINE HWND ViewportWindow(UViewport* Viewport) { return (HWND)Viewport->GetWindow(); }

// The window files: WindowsInfo.ini keeps each desktop window's place in the section of its
// number (NCWnd +0x78, given by the desktop's OnCreate).
static const TCHAR GWindowsInfoIni[] = TEXT("WindowsInfo.ini");

// A virtual variadic method: NW_HOOK_VARARGS cannot take its address (MSVC gives a vcall
// thunk), but our vtable's slot holds the method itself.
extern "C" const void* const nwvt_NConsoleWnd[];
#define NW_HOOK_VARARGS_SLOT(va, Class, Method, Slot)                                            \
    namespace {                                                                                  \
    ::nw::HookReg NW_CAT(nwHook_, __LINE__)(va, (void*)nwvt_##Class[Slot], #Class "::" #Method,  \
                                            __FILE__);                                           \
    }

/*-----------------------------------------------------------------------------
    Creation and destruction.
-----------------------------------------------------------------------------*/

// a top window registered with the window manager and attached to its entry (NCWnd's
// AttachEntry, a call here)
static FORCEINLINE void AttachTopWindow(NConsoleWnd* Console, NCWnd* Wnd, DWORD Style)
{
    Wnd->AttachEntry(Console->WndMain->RegisterNWindow(Wnd, Style, NULL));
}

// the same with AttachEntry inline (as the stock code has it for some windows)
static FORCEINLINE void CreateTopWnd(NConsoleWnd* Console, NCWnd* Wnd, DWORD Style)
{
    NWndEntry* E = Console->WndMain->RegisterNWindow(Wnd, Style, NULL);
    Wnd->Entry = E;
    if (E->Parent)
    {
        NCWnd* Parent = E->Parent->Wnd;
        Wnd->ScreenX += Parent->ScreenX;
        Wnd->ScreenY += Parent->ScreenY;
    }
    Wnd->WindowProc(WM_CREATE, 0, 0);
}

// the debug window, as both initializations make it
static FORCEINLINE void SetupDebugWnd(NCDebugWnd* DebugWnd)
{
    DebugWnd->SetBackTexture(TEXT("NWindow.ChatBack"));
    DebugWnd->LineGap = 0;
    DebugWnd->TextX = 30;
    DebugWnd->TextY = 3;
    DebugWnd->LoadCategories();
    DebugWnd->bAutoScroll = 1;
    DebugWnd->HideWindow();
}

// the renderer's video mode (URenderDevice::SetRes, pure in Engine.dll: D3DDrv.dll's): the
// viewport, the game play size, the color bytes, the refresh rate, fullscreen
static FORCEINLINE UBOOL SetVideoMode(NConsoleWnd* Console, UBOOL bFullscreen)
{
    return Console->Engine->GRenDev->SetRes(Console->Engine->Client->Viewports(0), GSEKScreenX,
                                            GSEKScreenY, GColorBits / 8, GRefreshRate,
                                            bFullscreen, 1);
}

// 0x1004de70 - once: the globals (the game window, the console, the canvas), the video mode
// of Option.ini (else 1024 x 768, 800 x 600, 640 x 480 in 32 bits at 60 Hz, until one
// works), the debug switch, and every top window - the window manager, (but for the test
// consoles) the message line, the login screens, the restart menu, the character selection
// and creation, the loading screen, the debug window and the login stages' dialog box, then
// the petition window, the desktop and the option window. The auto login of AutoLog.txt
// fills the login in (its texts are sprintf formats) and disables the buttons that would
// leave it. The saved window positions last.
UBOOL NConsoleWnd::Initialize(HWND hWnd)
{
    guard(NConsoleWnd::Initialize);
    if (bInitialized)
        return 1;
    GViewportHWnd = hWnd;
    GConsole = this;
    GCanvas = Engine->Client->Viewports(0)->Canvas;
    TCHAR OptionIni[] = TEXT("Option.ini");
    TCHAR Video[] = TEXT("Video");
    INT ShortCutDelay = 500;
    GConfig->GetInt(Video, TEXT("GamePlayViewportX"), GSEKScreenX, OptionIni);
    GConfig->GetInt(Video, TEXT("GamePlayViewportY"), GSEKScreenY, OptionIni);
    GConfig->GetInt(Video, TEXT("ColorBits"), GColorBits, OptionIni);
    GConfig->GetInt(Video, TEXT("RefreshRate"), GRefreshRate, OptionIni);
    if (!GConfig->GetInt(TEXT("URL"), TEXT("ShortCutDelay"), ShortCutDelay))
        ShortCutDelay = 500;
    // +0x3720: the client's StartupFullscreen
    if (!GConfig->GetBool(TEXT("WinDrv.WindowsClient"), TEXT("StartupFullscreen"), Unknown3720))
        Unknown3720 = 1;
#if NW_FEATURES
    // phase 6: the mode the player left the game in; fullscreen only at a size the monitor has
    Unknown3720 = nwf::WindowFix_StartupFullscreen(Unknown3720 != 0);
#endif
    if (!SetVideoMode(this, Unknown3720))
    {
        GSEKScreenX = 1024;
        GSEKScreenY = 768;
        GColorBits = 32;
        GRefreshRate = 60;
        if (!SetVideoMode(this, Unknown3720))
        {
            GSEKScreenX = 800;
            GSEKScreenY = 600;
            GColorBits = 32;
            GRefreshRate = 60;
            if (!SetVideoMode(this, Unknown3720))
            {
                GSEKScreenX = 640;
                GSEKScreenY = 480;
                GColorBits = 32;
                GRefreshRate = 60;
                SetVideoMode(this, Unknown3720);
            }
        }
    }
    if (!GConfig->GetBool(TEXT("URL"), TEXT("L2Debug"), GL2DebugLog))
        GL2DebugLog = 0;

    WndMain = new NCVirtualWndMain;
    WndMain->LoadCursors();
    INT ScreenX = GSEKScreenX;
    INT ScreenY = GSEKScreenY;
    if (!GIsOnlyL2Console && !GIsTestHtml)
    {
        MessageWnd = new NCMessageWnd;
        MessageWnd->SetWindowRect(0, ScreenY - 52, ScreenX, 52);
        AttachTopWindow(this, MessageWnd, NWS_VISIBLE);
        MessageWnd->SetText(*GL2GameData.SystemMsg[SYSMSG_Copyright].Message);
        MessageWnd->ShowWindow();

        LoginWnd = new NCLoginWnd;
        LoginWnd->SetWindowRect(0, 0, ScreenX, ScreenY - 52);
        AttachTopWindow(this, LoginWnd, NWS_VISIBLE);
        LoginWnd->ShowWindow();

        RestartMenuWnd = new NCRestartMenuWnd;
        RestartMenuWnd->SetWindowRect(ScreenX / 2 - 70, ScreenY / 2 - 64, 141, 131);
        AttachTopWindow(this, RestartMenuWnd, NWS_VISIBLE);
        RestartMenuWnd->HideWindow();

        LobbyWnd = new NCLobbyWnd;
        LobbyWnd->SetWindowRect(0, 0, ScreenX, ScreenY - 52);
        AttachTopWindow(this, LobbyWnd, NWS_NOHITTEST);
        LobbyWnd->HideWindow();

        PawnCreateWnd = new NCPawnCreateWnd;
        PawnCreateWnd->SetWindowRect(0, 0, ScreenX, ScreenY - 52);
        AttachTopWindow(this, PawnCreateWnd, NWS_VISIBLE);
        PawnCreateWnd->HideWindow();

        LoadingWnd = new NCLoadingWnd;
        LoadingWnd->SetWindowRect(0, 0, ScreenX, ScreenY);
        AttachTopWindow(this, LoadingWnd, NWS_VISIBLE);
        LoadingWnd->HideWindow();

        DebugWnd = new NCDebugWnd(150, 20);
        DebugWnd->SetWindowRect(0, 0, GSEKScreenX, 246);
        AttachTopWindow(this, DebugWnd, NWS_VISIBLE);
        SetupDebugWnd(DebugWnd);

        DialogBox = new NCDialogBox;
        DialogBox->SetWindowRect(GSEKScreenX / 2 - 101, GSEKScreenY / 2 - 43, 203, 87);
        AttachTopWindow(this, DialogBox, NWS_VISIBLE);
        DialogBox->HideWindow();
        DialogBox->SetButtonTexts(STR_OK, STR_Cancel);
    }

    PetitionWnd = new NCPetitionWnd;
    PetitionWnd->SetWindowRect(GSEKScreenX - 300, GSEKScreenY - 333, 300, 301);
    CreateTopWnd(this, PetitionWnd, NWS_VISIBLE | NWS_MINIMIZABLE);
    PetitionWnd->SetTitle(*GL2GameData.SysString[STR_Petition]);
    PetitionWnd->HideWindow();

    Desktop = new NCConsole(this);
    Desktop->SetWindowRect(0, 0, GSEKScreenX, GSEKScreenY);
    CreateTopWnd(this, Desktop, NWS_VISIBLE | NWS_NOHITTEST);
    Desktop->HideWindow();
    // the shortcut bar's key delay: the original calls the setter only when Desktop->ShortCutWnd
    // exists, and the setter is the empty folded `ret 4` (0x10001700), so nothing happens
    (void)ShortCutDelay;

    OptionWnd = new NCOptionWnd;
    OptionWnd->SetWindowRect(ScreenX / 2 - 120, ScreenY / 2 - 182, 240, 365);
    CreateTopWnd(this, OptionWnd, NWS_VISIBLE | NWS_TOPMOST);
    OptionWnd->HideWindow();
    OptionWnd->SetTitle(*GL2GameData.SysString[STR_Options]);
    if (GIsOnlyL2Console || GIsTestHtml)
        Desktop->ShowWindow();

    if (!GConfig->GetBool(TEXT("AutoLogOn"), TEXT("IsL2AutoLogOn"), GIsL2AutoLogOn, TEXT("AutoLog.txt")))
        GIsL2AutoLogOn = 0;
    else if (GIsL2AutoLogOn)
    {
        TCHAR Account[64];
        TCHAR Password[64];
        appSprintf(Account, GConfig->GetStr(TEXT("AutoLogOn"), TEXT("L2ID"), TEXT("AutoLog.txt")));
        appSprintf(Password, GConfig->GetStr(TEXT("AutoLogOn"), TEXT("L2Passwd"), TEXT("AutoLog.txt")));
        LoginWnd->AuthWnd->AccountEdit->SetText(Account);
        LoginWnd->AuthWnd->PasswordEdit->SetText(Password);
        LoginWnd->AuthWnd->OKButton->SetFocus();
        LobbyWnd->CreateButton->EnableWindow(0);
        LobbyWnd->DeleteButton->EnableWindow(0);
        LobbyWnd->PreviousButton->EnableWindow(0);
        GetDesktop()->SystemMenuWnd->Buttons[SYSMENU_Restart]->EnableWindow(0);
    }
    if (!GIsOnlyL2Console)
        LoadWindowPositions();
    bInitialized = 1;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1004de70, NConsoleWnd, Initialize)

// 0x1004d6c0 - the pawn viewer mode (the engine's InitForPawnViewer): the viewport size of
// .\L2.ini (fullscreen or windowed; 1024 x 768 by default), the debug switch, the game play
// size of the client's section (not Option.ini), then the window manager, the debug window,
// the desktop and the three viewers at the right edge of the game window, all hidden (the
// desktop shows for the test consoles)
UBOOL NConsoleWnd::InitializePawnViewer(HWND hWnd)
{
    guard(NConsoleWnd::InitializePawnViewer);
    if (bInitialized)
        return 1;
    GViewportHWnd = hWnd;
    GConsole = this;
    GCanvas = Engine->Client->Viewports(0)->Canvas;
    TCHAR L2Ini[] = TEXT(".\\L2.ini");
    UBOOL bFullscreen = 0;
    INT ViewportX, ViewportY;
    if (!GConfig->GetBool(TEXT("WinDrv.WindowsClient"), TEXT("StartupFullscreen"), bFullscreen))
        bFullscreen = 1;
    if (bFullscreen)
    {
        if (!GConfig->GetInt(TEXT("WinDrv.WindowsClient"), TEXT("FullscreenViewportX"), ViewportX, L2Ini))
            ViewportX = 1024;
        if (!GConfig->GetInt(TEXT("WinDrv.WindowsClient"), TEXT("FullscreenViewportY"), ViewportY, L2Ini))
            ViewportY = 768;
    }
    else
    {
        if (!GConfig->GetInt(TEXT("WinDrv.WindowsClient"), TEXT("WindowedViewportX"), ViewportX, L2Ini))
            ViewportX = 1024;
        if (!GConfig->GetInt(TEXT("WinDrv.WindowsClient"), TEXT("WindowedViewportY"), ViewportY, L2Ini))
            ViewportY = 768;
    }
    if (!GConfig->GetBool(TEXT("URL"), TEXT("L2Debug"), GL2DebugLog))
        GL2DebugLog = 0;
    if (!GConfig->GetInt(TEXT("WinDrv.WindowsClient"), TEXT("GamePlayViewportX"), GSEKScreenX))
        GSEKScreenX = 1024;
    if (!GConfig->GetInt(TEXT("WinDrv.WindowsClient"), TEXT("GamePlayViewportY"), GSEKScreenY))
        GSEKScreenY = 768;

    WndMain = new NCVirtualWndMain;
    WndMain->LoadCursors();

    DebugWnd = new NCDebugWnd(150, 20);
    DebugWnd->SetWindowRect(0, 0, ViewportX, 246);
    CreateTopWnd(this, DebugWnd, NWS_VISIBLE);
    SetupDebugWnd(DebugWnd);

    Desktop = new NCConsole(this);
    Desktop->SetWindowRect(0, 0, GSEKScreenX, GSEKScreenY);
    CreateTopWnd(this, Desktop, NWS_VISIBLE | NWS_NOHITTEST);
    Desktop->HideWindow();

    RECT Client;
    GetClientRect(hWnd, &Client);
    PawnViewerWnd[0] = new NCPawnViewrWnd;
    PawnViewerWnd[0]->SetWindowRect(Client.right - 241, 100, 240, 599);
    CreateTopWnd(this, PawnViewerWnd[0], NWS_VISIBLE);
    PawnViewerWnd[0]->SetTitle(TEXT("PAWNVIEWER"));
    PawnViewerWnd[0]->HideWindow();
    PawnViewerWnd[1] = new NCNpcViewrWnd;
    PawnViewerWnd[1]->SetWindowRect(Client.right - 241, 100, 240, 275);
    CreateTopWnd(this, PawnViewerWnd[1], NWS_VISIBLE);
    PawnViewerWnd[1]->SetTitle(TEXT("NPCVIEWER"));
    PawnViewerWnd[1]->HideWindow();
    PawnViewerWnd[2] = new NCMSViewrWnd;
    PawnViewerWnd[2]->SetWindowRect(Client.right - 241, 100, 240, 400);
    CreateTopWnd(this, PawnViewerWnd[2], NWS_VISIBLE);
    PawnViewerWnd[2]->SetTitle(TEXT("SKILLVIEWER"));
    PawnViewerWnd[2]->HideWindow();

    if (GIsOnlyL2Console || GIsTestHtml)
        Desktop->ShowWindow();
    bInitialized = 1;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1004d6c0, NConsoleWnd, InitializePawnViewer)

// a top window unregistered and deleted
template <class T> static FORCEINLINE void DestroyTopWnd(NConsoleWnd* Console, T*& Wnd)
{
    if (Wnd)
    {
        Console->WndMain->UnRegisterNWindow(Wnd->Entry);
        delete Wnd;
        Wnd = NULL;
    }
}

// 0x1004eb30 - the timers, the game window and every top window gone, the window manager's
// cursors and device with it
void NConsoleWnd::Finalize()
{
    guard(NConsoleWnd::Finalize);
    if (!bInitialized)
        return;
    Timers.Empty();
    GViewportHWnd = NULL;
    DestroyTopWnd(this, PetitionWnd);
    DestroyTopWnd(this, OptionWnd);
    DestroyTopWnd(this, LoginWnd);
    DestroyTopWnd(this, LobbyWnd);
    DestroyTopWnd(this, PawnCreateWnd);
    DestroyTopWnd(this, LoadingWnd);
    DestroyTopWnd(this, MessageWnd);
    DestroyTopWnd(this, Desktop);
    DestroyTopWnd(this, DebugWnd);
    DestroyTopWnd(this, UnusedWnd);
    DestroyTopWnd(this, DialogBox);
    DestroyTopWnd(this, RestartMenuWnd);
    DestroyTopWnd(this, PawnViewerWnd[0]);
    DestroyTopWnd(this, PawnViewerWnd[1]);
    DestroyTopWnd(this, PawnViewerWnd[2]);
    if (WndMain)
    {
        WndMain->DestroyCursors();
        WndMain->DeleteGDevice();
        delete WndMain;
        WndMain = NULL;
    }
    bInitialized = 0;
    unguard;
}
NW_HOOK_METHOD(0x1004eb30, NConsoleWnd, Finalize)

// 0x1004eee0 (vslot 236) - the engine hands the console over, with its viewport's window
UBOOL NConsoleWnd::Init(UEngine* InEngine)
{
    guard(NConsoleWnd::Init);
    if (!InEngine)
        return 0;
    Engine = InEngine;
    Initialize(ViewportWindow(InEngine->Client->Viewports(0)));
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1004eee0, NConsoleWnd, Init)

// 0x100550f0 (vslot 253) - the same for the pawn viewer mode
UBOOL NConsoleWnd::InitForPawnViewer(UEngine* InEngine)
{
    guard(NConsoleWnd::InitForPawnViewer);
    if (!InEngine)
        return 0;
    Engine = InEngine;
    InitializePawnViewer(ViewportWindow(InEngine->Client->Viewports(0)));
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100550f0, NConsoleWnd, InitForPawnViewer)

// 0x100551a0 (vslot 10)
void NConsoleWnd::Destroy()
{
    Finalize();
    UObject::Destroy();
}
NW_HOOK_METHOD(0x100551a0, NConsoleWnd, Destroy)

/*-----------------------------------------------------------------------------
    The window positions.
-----------------------------------------------------------------------------*/

// a desktop window's saved place, when both keys are there
static FORCEINLINE void LoadWindowPos(NCWnd* Wnd, INT& X, INT& Y)
{
    if (GConfig->GetInt(appItoa(Wnd->Unknown78), TEXT("posX"), X, GWindowsInfoIni) &&
        GConfig->GetInt(appItoa(Wnd->Unknown78), TEXT("posY"), Y, GWindowsInfoIni))
        Wnd->SetWindowRect(X, Y, 0, 0);
}

// 0x1004b020 - the desktop windows where WindowsInfo.ini left them (Initialize): the chat's
// height (from its default, 174), the player window's width, the shortcut bar's direction
void NConsoleWnd::LoadWindowPositions()
{
    INT X, Y;
    if (GConfig->GetInt(appItoa(GetDesktop()->ChatWnd->Unknown78), TEXT("Height"), X, GWindowsInfoIni))
        GetDesktop()->ChatWnd->ResizeBy(174 - X);
    LoadWindowPos(GetDesktop()->InventoryWnd, X, Y);
    LoadWindowPos(GetDesktop()->NpcHtmlWnd, X, Y);
    LoadWindowPos(GetDesktop()->PlayerStatusWnd, X, Y);
    if (GConfig->GetInt(appItoa(GetDesktop()->PlayerStatusWnd->Unknown78), TEXT("width"), X,
                        GWindowsInfoIni))
        GetDesktop()->PlayerStatusWnd->Resize(X - 152);
    LoadWindowPos(GetDesktop()->BoardWnd, X, Y);
    UBOOL bHorizontal = 0;
    if (GConfig->GetBool(appItoa(GetDesktop()->ShortCutWnd->Unknown78), TEXT("rotate"), bHorizontal,
                         GWindowsInfoIni))
    {
        GetDesktop()->ShortCutWnd->bHorizontal = bHorizontal;
        GetDesktop()->ShortCutWnd->UpdateLayout();
    }
    LoadWindowPos(GetDesktop()->ShortCutWnd, X, Y);
    LoadWindowPos(GetDesktop()->MainWnd, X, Y);
    LoadWindowPos(GetDesktop()->TargetStatusWnd, X, Y);
    LoadWindowPos(GetDesktop()->TradeWnd, X, Y);
    LoadWindowPos(GetDesktop()->HelpWnd, X, Y);
    LoadWindowPos(GetDesktop()->MatchWnd, X, Y);
    LoadWindowPos(GetDesktop()->MinimapWnd, X, Y);
    LoadWindowPos(GetDesktop()->PrivateShopWnd, X, Y);
    LoadWindowPos(GetDesktop()->PartyWnd, X, Y);
    LoadWindowPos(GetDesktop()->PetStatusWnd, X, Y);
    LoadWindowPos(GetDesktop()->SummonedStatusWnd, X, Y);
    LoadWindowPos(GetDesktop()->StoreWnd, X, Y);
    LoadWindowPos(GetDesktop()->ShopWnd, X, Y);
    LoadWindowPos(GetDesktop()->TrainWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMPetitionWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMActionWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMStoreWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMQuestWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMDetailStatusWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMMagicSkillWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMInventoryWnd, X, Y);
    LoadWindowPos(GetDesktop()->GMWnd, X, Y);
    LoadWindowPos(GetDesktop()->AbnormalStatusWnd, X, Y);
    LoadWindowPos(GetDesktop()->MenuWnd, X, Y);
}
NW_HOOK_METHOD(0x1004b020, NConsoleWnd, LoadWindowPositions)

// a desktop window's place saved
static FORCEINLINE void SaveWindowPos(NCWnd* Wnd)
{
    INT Number = Wnd->Unknown78;
    INT X = Wnd->LocalX;
    INT Y = Wnd->LocalY;
    GConfig->SetInt(appItoa(Number), TEXT("posX"), X, GWindowsInfoIni);
    GConfig->SetInt(appItoa(Number), TEXT("posY"), Y, GWindowsInfoIni);
}

// 0x1004a3d0 - the desktop windows' places into WindowsInfo.ini (the logout, AcceptLogOut),
// with the shortcut bar's direction, the player window's width and the chat's height
void NConsoleWnd::SaveWindowPositions()
{
    SaveWindowPos(GetDesktop()->InventoryWnd);
    SaveWindowPos(GetDesktop()->NpcHtmlWnd);
    SaveWindowPos(GetDesktop()->HelpWnd);
    SaveWindowPos(GetDesktop()->ShortCutWnd);
    GConfig->SetBool(appItoa(GetDesktop()->ShortCutWnd->Unknown78), TEXT("rotate"),
                     GetDesktop()->ShortCutWnd->bHorizontal, GWindowsInfoIni);
    SaveWindowPos(GetDesktop()->TargetStatusWnd);
    SaveWindowPos(GetDesktop()->BoardWnd);
    SaveWindowPos(GetDesktop()->TradeWnd);
    SaveWindowPos(GetDesktop()->MatchWnd);
    SaveWindowPos(GetDesktop()->MinimapWnd);
    SaveWindowPos(GetDesktop()->PrivateShopWnd);
    SaveWindowPos(GetDesktop()->MainWnd);
    {
        NCWnd* Player = GetDesktop()->PlayerStatusWnd;
        INT Number = Player->Unknown78;
        INT X = Player->LocalX;
        INT Y = Player->LocalY;
        INT W = Player->LocalW;
        GConfig->SetInt(appItoa(Number), TEXT("posX"), X, GWindowsInfoIni);
        GConfig->SetInt(appItoa(Number), TEXT("posY"), Y, GWindowsInfoIni);
        GConfig->SetInt(appItoa(Number), TEXT("width"), W, GWindowsInfoIni);
    }
    SaveWindowPos(GetDesktop()->PartyWnd);
    SaveWindowPos(GetDesktop()->PetStatusWnd);
    SaveWindowPos(GetDesktop()->SummonedStatusWnd);
    SaveWindowPos(GetDesktop()->StoreWnd);
    SaveWindowPos(GetDesktop()->ShopWnd);
    SaveWindowPos(GetDesktop()->TrainWnd);
    SaveWindowPos(GetDesktop()->GMPetitionWnd);
    SaveWindowPos(GetDesktop()->GMActionWnd);
    SaveWindowPos(GetDesktop()->GMStoreWnd);
    SaveWindowPos(GetDesktop()->GMQuestWnd);
    SaveWindowPos(GetDesktop()->GMDetailStatusWnd);
    SaveWindowPos(GetDesktop()->GMMagicSkillWnd);
    SaveWindowPos(GetDesktop()->GMInventoryWnd);
    SaveWindowPos(GetDesktop()->GMWnd);
    SaveWindowPos(GetDesktop()->AbnormalStatusWnd);
    SaveWindowPos(GetDesktop()->MenuWnd);
    GConfig->SetInt(appItoa(GetDesktop()->ChatWnd->Unknown78), TEXT("Height"),
                    GetDesktop()->ChatWnd->LocalH, GWindowsInfoIni);
}
NW_HOOK_METHOD(0x1004a3d0, NConsoleWnd, SaveWindowPositions)

// a window's local rectangle (a copy of LocalX .. LocalH): SetScreenPos takes its corner as
// the origin
struct NWndRect
{
    INT X;
    INT Y;
    INT W;
    INT H;

    NWndRect() : X(0), Y(0), W(0), H(0) {}
    void Set(NCWnd* Wnd)
    {
        X = Wnd->LocalX;
        Y = Wnd->LocalY;
        H = Wnd->LocalH;
        W = Wnd->LocalW;
    }
    const POINT* Origin() const { return reinterpret_cast<const POINT*>(this); }
};

// a minimized window back to its size
static FORCEINLINE void RestoreIfMinimized(NCWnd* Wnd)
{
    if (Wnd->Icon->IsShowWindow())
        Wnd->Restore();
}

// 0x1004c060 - every window back to its default place for the current game play size (the
// options' reset and a video mode change): the minimized ones restored first; the login
// screens, the message line, the option window and the restart menu; the character
// selection's list and question box keep their place in proportion to the old screen size,
// the creation's panels too (both from its first panel's place), their buttons go to the
// corners; the desktop windows to fixed places (the chat at the bottom, the shortcut bar
// vertical at the right edge)
void NConsoleWnd::ResetWindowPositions()
{
    RestoreIfMinimized(GetDesktop()->MainWnd);
    RestoreIfMinimized(GetDesktop()->MatchWnd);
    RestoreIfMinimized(GetDesktop()->SystemMenuWnd);
    RestoreIfMinimized(GetDesktop()->BoardWnd);
    RestoreIfMinimized(PetitionWnd);
    RestoreIfMinimized(GetDesktop()->MinimapWnd);
    RestoreIfMinimized(GetDesktop()->PetWnd);
    RestoreIfMinimized(GetDesktop()->SummonedWnd);
    RestoreIfMinimized(GetDesktop()->InventoryWnd);

    // the login screens (the old desktop size is kept for the proportional places)
    NWndRect Rect;
    Rect.Set(Desktop);
    INT OldHeight = Rect.H;
    INT OldWidth = Rect.W;
    LoginWnd->SetSize(GSEKScreenX, GSEKScreenY - 52);
    Rect.Set(LoginWnd);
    LoginWnd->AuthWnd->SetScreenPos(GSEKScreenX / 2 - 104, GSEKScreenY / 2 - 53, Rect.Origin());
    LoginWnd->NewAccountButton->SetScreenPos(GSEKScreenX - 132, GSEKScreenY - 148, Rect.Origin());
    LoginWnd->LostAccountButton->SetScreenPos(GSEKScreenX - 132, GSEKScreenY - 124, Rect.Origin());
    LoginWnd->OptionsButton->SetScreenPos(GSEKScreenX - 132, GSEKScreenY - 100, Rect.Origin());
    LoginWnd->NoticeWnd->SetScreenPos(GSEKScreenX / 2 - 266, GSEKScreenY / 2 - 213, Rect.Origin());
    LoginWnd->ServerSelectWnd->SetScreenPos(GSEKScreenX / 2 - 150, GSEKScreenY / 2 - 113,
                                            Rect.Origin());
    Rect.H = GSEKScreenY;
    MessageWnd->SetSize(GSEKScreenX, 52);
    MessageWnd->SetScreenPos(0, GSEKScreenY - 52, Rect.Origin());
    OptionWnd->SetScreenPos(GSEKScreenX / 2 - 120, GSEKScreenY / 2 - 182, Rect.Origin());
    RestartMenuWnd->SetWindowRect(GSEKScreenX / 2 - 104, GSEKScreenY / 2 - 76, 208, 152);

    // the character selection
    LobbyWnd->SetSize(GSEKScreenX, GSEKScreenY - 52);
    FLOAT OldH = (FLOAT)OldHeight;
    FLOAT OldW = (FLOAT)OldWidth;
    Rect.Set(LobbyWnd);
    {
        NCWnd* Wnd = LobbyWnd->PawnSelectWnd;
        INT LocalY = Wnd->LocalY;
        INT LocalX = Wnd->LocalX;
        INT Y = (INT)((DOUBLE)LocalY / OldH * GSEKScreenY);
        Wnd->SetScreenPos((INT)((DOUBLE)LocalX / OldW * GSEKScreenX), Y, Rect.Origin());
    }
    {
        NCWnd* Wnd = LobbyWnd->DialogBox;
        INT LocalY = Wnd->LocalY;
        INT LocalX = Wnd->LocalX;
        INT Y = (INT)((DOUBLE)LocalY / OldH * GSEKScreenY);
        Wnd->SetScreenPos((INT)((DOUBLE)LocalX / OldW * GSEKScreenX), Y, Rect.Origin());
    }
    LobbyWnd->CreateButton->SetScreenPos(GSEKScreenX - 131, GSEKScreenY - 172, Rect.Origin());
    LobbyWnd->DeleteButton->SetScreenPos(GSEKScreenX - 131, GSEKScreenY - 148, Rect.Origin());
    LobbyWnd->StartButton->SetScreenPos(GSEKScreenX - 131, GSEKScreenY - 124, Rect.Origin());
    LobbyWnd->PreviousButton->SetScreenPos(GSEKScreenX - 131, GSEKScreenY - 100, Rect.Origin());

    // the character creation: both panels from the first one's place
    PawnCreateWnd->SetSize(GSEKScreenX, GSEKScreenY - 52);
    Rect.Set(PawnCreateWnd);
    {
        NCWnd* Wnd = PawnCreateWnd->SetupWnd;
        INT LocalY = Wnd->LocalY;
        INT LocalX = Wnd->LocalX;
        FLOAT YRatio = (FLOAT)((DOUBLE)LocalY / OldH);
        FLOAT XRatio = (FLOAT)((DOUBLE)LocalX / OldW);
        INT Y = (INT)((DOUBLE)GSEKScreenY * YRatio);
        Wnd->SetScreenPos((INT)((DOUBLE)GSEKScreenX * XRatio), Y, Rect.Origin());
        Y = (INT)((DOUBLE)GSEKScreenY * YRatio + 244.0f);
        PawnCreateWnd->InfoWnd->SetScreenPos((INT)((DOUBLE)GSEKScreenX * XRatio), Y, Rect.Origin());
    }
    PawnCreateWnd->CreateButton->SetScreenPos(GSEKScreenX - 131, GSEKScreenY - 124, Rect.Origin());
    PawnCreateWnd->PreviousButton->SetScreenPos(GSEKScreenX - 131, GSEKScreenY - 100, Rect.Origin());
    PawnCreateWnd->ZoomInButton->SetScreenPos(GSEKScreenX / 2 - 22, GSEKScreenY - 100, Rect.Origin());
    PawnCreateWnd->LeftButton->SetScreenPos(GSEKScreenX / 2 - 88, GSEKScreenY - 100, Rect.Origin());
    PawnCreateWnd->RightButton->SetScreenPos(GSEKScreenX / 2 + 44, GSEKScreenY - 100, Rect.Origin());
    PawnCreateWnd->ZoomOutButton->SetScreenPos(GSEKScreenX / 2 - 22, GSEKScreenY - 100, Rect.Origin());
    LoadingWnd->SetSize(GSEKScreenX, GSEKScreenY);

    // the desktop
    Desktop->SetSize(GSEKScreenX, GSEKScreenY);
    Rect.Set(Desktop);
    NCConsole* Desk = GetDesktop();
    INT ChatHeight = Desk->ChatWnd->LocalH;
    Desk->ChatWnd->SetScreenPos(0, GSEKScreenY - ChatHeight, Rect.Origin());
    GetDesktop()->NpcHtmlWnd->SetScreenPos(300, 250, Rect.Origin());
    GetDesktop()->HelpWnd->SetScreenPos(300, 250, Rect.Origin());
    GetDesktop()->ShortCutWnd->bHorizontal = 0;
    GetDesktop()->ShortCutWnd->UpdateLayout();
    GetDesktop()->ShortCutWnd->SetScreenPos(GSEKScreenX - 45, GSEKScreenY / 2 - 218, Rect.Origin());
    GetDesktop()->InventoryWnd->SetScreenPos(GSEKScreenX - 290, 205, Rect.Origin());
    GetDesktop()->MainWnd->SetScreenPos(50, 207, Rect.Origin());
    GetDesktop()->PlayerStatusWnd->SetScreenPos(0, 0, Rect.Origin());
    GetDesktop()->TargetStatusWnd->SetScreenPos(GSEKScreenX / 2 - 84, 0, Rect.Origin());
    GetDesktop()->AbnormalStatusWnd->SetScreenPos(154, 2, Rect.Origin());
    GetDesktop()->AbnormalStatusWnd->UpdateSize();
    GetDesktop()->PrivateShopWnd->SetScreenPos(GSEKScreenX - 290, 205, Rect.Origin());
    GetDesktop()->MinimapWnd->SetScreenPos(GSEKScreenX / 2 - 167, GSEKScreenY / 2 - 178,
                                           Rect.Origin());
    GetDesktop()->TutorialWnd->SetScreenPos(605, 250, Rect.Origin());
    GetDesktop()->BoardWnd->SetScreenPos(60, 20, Rect.Origin());
    GetDesktop()->MatchWnd->SetScreenPos(GSEKScreenX - 350, 215, Rect.Origin());
    GetDesktop()->StoreWnd->SetScreenPos(GSEKScreenX - 290, 205, Rect.Origin());
    GetDesktop()->ShopWnd->SetScreenPos(GSEKScreenX - 290, 205, Rect.Origin());
    GetDesktop()->TradeWnd->SetScreenPos(GSEKScreenX - 290, 205, Rect.Origin());
    GetDesktop()->MenuWnd->SetScreenPos(GSEKScreenX - 166, GSEKScreenY - 44, Rect.Origin());
    GetDesktop()->SystemMenuWnd->SetScreenPos(GSEKScreenX - 290, 205, Rect.Origin());
    GetDesktop()->HelpWnd->SetScreenPos(GSEKScreenX - 209, GSEKScreenY / 2 - 253, Rect.Origin());
    GetDesktop()->MainDialogBox->SetScreenPos(GSEKScreenX / 2 - 101, GSEKScreenY / 2 - 43,
                                              Rect.Origin());
    GetDesktop()->SecondDialogBox->SetScreenPos(347, GSEKScreenY - 89, Rect.Origin());
    GetDesktop()->PartyWnd->SetScreenPos(0, 72, Rect.Origin());
    GetDesktop()->PetStatusWnd->SetScreenPos(152, 0, Rect.Origin());
    GetDesktop()->SummonedStatusWnd->SetScreenPos(152, 0, Rect.Origin());
    GetDesktop()->TutorialQuestionWnd->SetScreenPos(5, GSEKScreenY - ChatHeight - 37,
                                                    Rect.Origin());
    GetDesktop()->TrainWnd->SetScreenPos(GSEKScreenX - 258, 64, Rect.Origin());
    PetitionWnd->SetScreenPos(GSEKScreenX - 301, GSEKScreenY - 334, Rect.Origin());
    GetDesktop()->PetWnd->SetScreenPos(207, 50, Rect.Origin());
    GetDesktop()->SummonedWnd->SetScreenPos(207, 50, Rect.Origin());
    GetDesktop()->GMWnd->SetScreenPos(GSEKScreenX - 209, GSEKScreenY - 327, Rect.Origin());
    GetDesktop()->GMInventoryWnd->SetScreenPos(GSEKScreenX - 209, 100, Rect.Origin());
    GetDesktop()->GMMagicSkillWnd->SetScreenPos(GSEKScreenX - 209, 100, Rect.Origin());
    GetDesktop()->GMDetailStatusWnd->SetScreenPos(0, 100, Rect.Origin());
    GetDesktop()->GMQuestWnd->SetScreenPos(GSEKScreenX - 209, 100, Rect.Origin());
    GetDesktop()->GMStoreWnd->SetScreenPos(GSEKScreenX - 209, 100, Rect.Origin());
    GetDesktop()->GMActionWnd->SetScreenPos(GSEKScreenX - 209, 100, Rect.Origin());
    GetDesktop()->GMPetitionWnd->SetScreenPos(200, 200, Rect.Origin());
}
NW_HOOK_METHOD(0x1004c060, NConsoleWnd, ResetWindowPositions)

/*-----------------------------------------------------------------------------
    Dialogs and the login screens.
-----------------------------------------------------------------------------*/

// 0x1004a0e0 - a dialog box: its text with the parameters put in (FormatSystemMessage, into
// Text2) in the desktop's main box or its second one; the main box takes the focus either way
void NConsoleWnd::ShowDialog(NDialogInfo* Info)
{
    FormatSystemMessage(Info->Text2, Info->Text, Info->Button1, Info->Button2);
    if (Info->bMainBox)
    {
        GetDesktop()->MainDialogBox->SetInfo(Info);
        GetDesktop()->MainDialogBox->ShowWindow();
        GetDesktop()->MainDialogBox->SetFocus();
    }
    else
    {
        GetDesktop()->SecondDialogBox->SetInfo(Info);
        GetDesktop()->SecondDialogBox->ShowWindow();
        GetDesktop()->MainDialogBox->SetFocus();
    }
}
NW_HOOK_METHOD(0x1004a0e0, NConsoleWnd, ShowDialog)

// 0x10046070 - the line of the message window under the login screens (NULL or empty: only
// cleared); the window shows if hidden
void NConsoleWnd::ShowMessageWnd(const TCHAR* Text)
{
    if (!MessageWnd)
        return;
    MessageWnd->ClearText();
    if (Text && *Text)
        MessageWnd->SetText(Text);
    if (!MessageWnd->IsShowWindow())
        MessageWnd->ShowWindow();
}
NW_HOOK_METHOD(0x10046070, NConsoleWnd, ShowMessageWnd)

// 0x100430c0 - after the announcements: the warning page, when the data has a warning text
void NConsoleWnd::ShowWarningWnd()
{
    if (GL2GameData.WarningNotice.Len() <= 0)
        return;
    LoginWnd->AuthWnd->HideWindow();
    LoginWnd->ServerSelectWnd->HideWindow();
    LoginWnd->NoticeWnd->HideWindow();
    LoginWnd->WarningWnd->ShowWindow();
    LoginWnd->WarningWnd->SetFocus();
    LoginWnd->WarningWnd->ClearText();
    LoginWnd->WarningWnd->SetText(*GL2GameData.WarningNotice);
}
NW_HOOK_METHOD(0x100430c0, NConsoleWnd, ShowWarningWnd)

// 0x100460c0 - the option window on or off, with its sound (the engine through GConsole)
void NConsoleWnd::ToggleOptionWnd()
{
    if (OptionWnd->IsShowWindow())
    {
        GConsole->Engine->OnInterfacePlaySound(SOUND_WindowClose);
        OptionWnd->HideWindow();
    }
    else
    {
        GConsole->Engine->OnInterfacePlaySound(SOUND_WindowOpen);
        OptionWnd->ShowWindow();
    }
}
NW_HOOK_METHOD(0x100460c0, NConsoleWnd, ToggleOptionWnd)

// 0x10045f70 - the character creation cancelled: back to the selection and its camera
void NConsoleWnd::ReturnToCharacterSelect()
{
    PawnCreateWnd->HideWindow();
    LobbyWnd->ShowWindow();
    MoveCameraByState(L2CS_None, L2CS_Lobby);
    SetState(L2CS_Lobby);
}
NW_HOOK_METHOD(0x10045f70, NConsoleWnd, ReturnToCharacterSelect)

// 0x100463f0 - the names of the items, skills and actions again after the data tables were
// reloaded (a language change): inventory, shop, warehouse, trade, shortcut bar, skills,
// actions
void NConsoleWnd::ReloadGameTexts()
{
    GetDesktop()->InventoryWnd->ReloadTexts();
    GetDesktop()->ShopWnd->ReloadTexts();
    GetDesktop()->StoreWnd->ReloadTexts();
    GetDesktop()->TradeWnd->ReloadTexts();
    GetDesktop()->ShortCutWnd->ReloadNames();
    static_cast<NCMagicSkillWnd*>(GetDesktop()->MainWnd->SkillWnd)->ReloadSkillTexts();
    GetDesktop()->MainWnd->ActionWnd->ReloadActions();
}
NW_HOOK_METHOD(0x100463f0, NConsoleWnd, ReloadGameTexts)

/*-----------------------------------------------------------------------------
    The rest of the windows' slots.
-----------------------------------------------------------------------------*/

// 0x100447e0 (vslot 262) - no handler: the character window
void NConsoleWnd::ShowCharacterStatus()
{
    GetDesktop()->ToggleCharacter(0);
}
NW_HOOK_METHOD(0x100447e0, NConsoleWnd, ShowCharacterStatus)

// 0x1004d680 (vslot 266) - the desktop's view rectangle (NCConsole +0x170: left, top,
// width, height)
void NConsoleWnd::SetViewRect(INT Left, INT Top, INT Right, INT Bottom)
{
    if (!Desktop)
        return;
    GetDesktop()->Unknown170[0] = Left;
    GetDesktop()->Unknown170[2] = Right - Left;
    GetDesktop()->Unknown170[1] = Top;
    GetDesktop()->Unknown170[3] = Bottom - Top;
}
NW_HOOK_METHOD(0x1004d680, NConsoleWnd, SetViewRect)

// 0x10046f50 (vslot 254) - the pawn viewer mode's windows: 0 / 1 show / hide the pawn viewer,
// 3 / 4 the NPC viewer, 5 / 6 the skill viewer
void NConsoleWnd::PawnViewerFunction(INT Function)
{
    guard(NConsoleWnd::PawnViewerFunction);
    switch (Function)
    {
    case 0:
        if (PawnViewerWnd[0])
            PawnViewerWnd[0]->ShowWindow();
        break;
    case 1:
        if (PawnViewerWnd[0])
            PawnViewerWnd[0]->HideWindow();
        break;
    case 3:
        if (PawnViewerWnd[1])
            PawnViewerWnd[1]->ShowWindow();
        break;
    case 4:
        if (PawnViewerWnd[1])
            PawnViewerWnd[1]->HideWindow();
        break;
    case 5:
        if (PawnViewerWnd[2])
            PawnViewerWnd[2]->ShowWindow();
        break;
    case 6:
        if (PawnViewerWnd[2])
            PawnViewerWnd[2]->HideWindow();
        break;
    }
    unguard;
}
NW_HOOK_METHOD(0x10046f50, NConsoleWnd, PawnViewerFunction)

// 0x10047190 (vslot 241) - "L2Debug" turns the debug log on or off, "L2DebugWindow" shows or
// hides the debug window
UBOOL NConsoleWnd::DebugCommand(const TCHAR* Cmd)
{
    if (ParseCommand(&Cmd, TEXT("L2Debug")))
    {
        GL2DebugLog = !GL2DebugLog;
        return 1;
    }
    if (ParseCommand(&Cmd, TEXT("L2DebugWindow")))
    {
        if (DebugWnd)
        {
            if (DebugWnd->IsShowWindow())
                DebugWnd->HideWindow();
            else
                DebugWnd->ShowWindow();
        }
        return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x10047190, NConsoleWnd, DebugCommand)

// 0x10045e00 (vslot 127) - no handler: a line of the category in the debug window, while the
// debug log is on. The stock buffer has 256 characters and the formatting has no limit (a longer
// line overruns the stack); here it has 1024, so the /GS check does not end the game (AddLog
// formats into 0x1F74 bytes; same output when it fits)
void VARARGS NConsoleWnd::DebugLogf(INT Category, const TCHAR* Fmt, ...)
{
    TCHAR Line[1024];
    if (!GL2DebugLog || !Fmt)
        return;
    va_list ArgPtr;
    va_start(ArgPtr, Fmt);
    _vswprintf(Line, Fmt, ArgPtr);
    va_end(ArgPtr);
    DebugWnd->AddLog(Category, Line, -1);
}
NW_HOOK_VARARGS_SLOT(0x10045e00, NConsoleWnd, DebugLogf, 127)

// 0x10045db0 (vslot 231) - no handler: a message box. The stock buffer has 256 characters and
// the formatting has no limit; here it has 1024, as in DebugLogf (same box when it fits)
void VARARGS NConsoleWnd::MessageBoxf(const TCHAR* Caption, const TCHAR* Fmt, ...)
{
    TCHAR Text[1024];
    if (!Caption || !Fmt)
        return;
    va_list ArgPtr;
    va_start(ArgPtr, Fmt);
    _vswprintf(Text, Fmt, ArgPtr);
    va_end(ArgPtr);
    MessageBoxW(NULL, Text, Caption, MB_OK);
}
NW_HOOK_VARARGS_SLOT(0x10045db0, NConsoleWnd, MessageBoxf, 231)

// 0x1004d190 (vslot 257) - the ids (NItemEntry::ID) of the items on the paperdoll, then on the
// inventory's first page, in an array the engine frees (Free, slot 258); NULL for none
INT* NConsoleWnd::GetInventoryItemIDs(INT& Count)
{
    Count = 0;
    NCEquipItemWnd* Equip = GetDesktop()->InventoryWnd->EquipWnd;
    NCInvenItemWnd* Items = GetDesktop()->InventoryWnd->ItemWnd[0];
    INT NumEquip = Equip ? Equip->Items.Num() : 0;
    INT NumItems = Items ? Items->Items.Num() : 0;
    Count = NumEquip + NumItems;
    if (Count <= 0)
        return NULL;
    INT* IDs = new INT[Count];
    INT i;
    for (i = 0; i < NumEquip; i++)
        IDs[i] = Equip->Items(i)->ID;
    for (; i < Count; i++)
        IDs[i] = Items->Items(i - NumEquip)->ID;
    return IDs;
}
NW_HOOK_METHOD(0x1004d190, NConsoleWnd, GetInventoryItemIDs)
