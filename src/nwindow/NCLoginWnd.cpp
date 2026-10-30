/*=============================================================================
    NCLoginWnd.cpp - the login screen. Stock: 0x1006a960 .. 0x1006aa5f,
    0x1006b160 .. 0x1006ba8f (OnNewAccount / OnOptions are bodies the linker folded with
    other classes).
=============================================================================*/
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCAuthWnd.h"
#include "nwindow/NCNoticeWnd.h"
#include "nwindow/NCServerSelectWnd.h"
#include "nwindow/NCWarningWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// NWM_COMMAND ids of the buttons (names ours; "Lost Account" has none)
enum
{
    NID_LOGIN_OPTIONS = (INT)0xAFF12011,
    NID_LOGIN_NEWACCOUNT = (INT)0xAFF12012,
};

// 0x1006b160 - only the logo is cleared; the rest is left as allocated
NCLoginWnd::NCLoginWnd()
{
    LogoTex = NULL;
}
NW_HOOK_CTOR(0x1006b160, NCLoginWnd)

// 0x1006b1b0
NW_HOOK_DTOR(0x1006b1b0, NCLoginWnd)

// 0x1006a960 (vslot 24)
INT NCLoginWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_LOGIN_NEWACCOUNT)
            return OnNewAccount() != 0;
        if ((INT)wParam == NID_LOGIN_OPTIONS)
            return OnOptions() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1006a960, NCLoginWnd, WindowProc)

// 0x1006b200 (vslot 35) - the boxes are centered on the screen (Width / Height halved
// unsigned, as in the stock code), the buttons stacked in the lower right corner
INT NCLoginWnd::OnCreate()
{
    NCWnd::OnCreate();
    FString LogoName;
    TCHAR Key[256];
    appSprintf(Key, TEXT("%s_LogoTexture"), GCountryName);
    if (!GConfig->GetString(GCountryName, Key, LogoName, TEXT("FontInfo.ini")))
        LogoName = TEXT("L2Font.start_logo-k");
    LogoTex = FindTexture(*LogoName);
    MiniLogoTex = FindTexture(TEXT("L2UI.LoginWnd.mini_logo"));

    AuthWnd = new NCAuthWnd;
    AuthWnd->SetWindowRect(((DWORD)Width >> 1) - 107, ((DWORD)Height >> 1) - 63, 214, 148);
    CreateChild(AuthWnd, NWS_NOCLOSE | NWS_VISIBLE);
    AuthWnd->SetTitle(*GL2GameData.SysString[149]);            // "Log In"

    NoticeWnd = new NCNoticeWnd;
    NoticeWnd->SetWindowRect(((DWORD)Width >> 1) - 266, ((DWORD)Height >> 1) - 213, 532, 424);
    CreateChild(NoticeWnd, NWS_NOCLOSE | NWS_VISIBLE);
    NoticeWnd->SetTitle(*GL2GameData.SysString[328]);          // "Announcements"

    WarningWnd = new NCWarningWnd;
    WarningWnd->SetWindowRect(((DWORD)Width >> 1) - 266, ((DWORD)Height >> 1) - 213, 532, 424);
    CreateChild(WarningWnd, NWS_NOCLOSE | NWS_VISIBLE);
    WarningWnd->SetTitle(*GL2GameData.SysString[538]);         // "Warning"

    ServerSelectWnd = new NCServerSelectWnd;
    ServerSelectWnd->SetWindowRect(((DWORD)Width >> 1) - 150, ((DWORD)Height >> 1) - 173, 300,
                                   616);
    CreateChild(ServerSelectWnd, NWS_NOCLOSE | NWS_VISIBLE);
    ServerSelectWnd->SetTitle(*GL2GameData.SysString[449]);    // "Choose Server"

    NewAccountButton = new NCButton(TEXT("L2UI.ActionWnd.PledgeButten1"),
                                    TEXT("L2UI.ActionWnd.PledgeButten2"), 0);
    NewAccountButton->ID = NID_LOGIN_NEWACCOUNT;
    NewAccountButton->SetWindowRect(Width - 132, Height - 148, 99, 19);
    CreateChild(NewAccountButton, NWS_VISIBLE);
    NewAccountButton->SetTextID(154);                          // "New Account"
    NewAccountButton->EnableWindow(0);

    LostAccountButton = new NCButton(TEXT("L2UI.ActionWnd.PledgeButten1"),
                                     TEXT("L2UI.ActionWnd.PledgeButten2"), 0);
    LostAccountButton->SetWindowRect(Width - 132, Height - 124, 99, 19);
    CreateChild(LostAccountButton, NWS_VISIBLE);
    LostAccountButton->SetTextID(155);                         // "Lost Account"
    LostAccountButton->EnableWindow(0);

    OptionsButton = new NCButton(TEXT("L2UI.ActionWnd.PledgeButten1"),
                                 TEXT("L2UI.ActionWnd.PledgeButten2"), 0);
    OptionsButton->ID = NID_LOGIN_OPTIONS;
    OptionsButton->SetWindowRect(Width - 132, Height - 100, 99, 19);
    CreateChild(OptionsButton, NWS_VISIBLE);
    OptionsButton->SetTextID(156);                             // "Options"
    return 1;
}
NW_HOOK_METHOD(0x1006b200, NCLoginWnd, OnCreate)

// 0x1006b850 (vslot 36)
INT NCLoginWnd::OnDestroy()
{
    if (AuthWnd)
    {
        RemoveChildWindow(AuthWnd->Entry);
        delete AuthWnd;
        AuthWnd = NULL;
    }
    if (NoticeWnd)
    {
        RemoveChildWindow(NoticeWnd->Entry);
        delete NoticeWnd;
        NoticeWnd = NULL;
    }
    if (WarningWnd)
    {
        RemoveChildWindow(WarningWnd->Entry);
        delete WarningWnd;
        WarningWnd = NULL;
    }
    if (ServerSelectWnd)
    {
        RemoveChildWindow(ServerSelectWnd->Entry);
        delete ServerSelectWnd;
        ServerSelectWnd = NULL;
    }
    if (NewAccountButton)
    {
        RemoveChildWindow(NewAccountButton->Entry);
        delete NewAccountButton;
        NewAccountButton = NULL;
    }
    if (LostAccountButton)
    {
        RemoveChildWindow(LostAccountButton->Entry);
        delete LostAccountButton;
        LostAccountButton = NULL;
    }
    if (OptionsButton)
    {
        RemoveChildWindow(OptionsButton->Entry);
        delete OptionsButton;
        OptionsButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1006b850, NCLoginWnd, OnDestroy)

// 0x1006a9f0 (vslot 42) - only the login box stays up
void NCLoginWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    WarningWnd->HideWindow();
    NoticeWnd->HideWindow();
    ServerSelectWnd->HideWindow();
}
NW_HOOK_METHOD(0x1006a9f0, NCLoginWnd, ShowWindow)

// 0x1006b9a0 (vslot 50) - the small logo in the upper left corner, the big one centered at
// a twentieth of the height (neither is checked for NULL)
INT NCLoginWnd::OnPaint(NCGDevice* Device)
{
    guard(NCLoginWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, 200, 65, 0, 0, 200, 65, MiniLogoTex);
    Device->DrawTexture(((DWORD)Width >> 1) - 128, (DWORD)Height / 20, 256, 256, 0, 0, 256, 256,
                        LogoTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1006b9a0, NCLoginWnd, OnPaint)

// stock body shared with NCSystemMenuWnd's (0x10076d00): "Options" opens or closes the option
// window
INT NCLoginWnd::OnOptions()
{
    GConsole->ToggleOptionWnd();
    return 1;
}

// 0x1006aa30 - back to an empty login screen (failed login, server list cancelled...)
void NCLoginWnd::ResetWindows()
{
    AuthWnd->ClearPassword();
    NoticeWnd->ClearText();
    WarningWnd->ClearText();
    ServerSelectWnd->ClearServers();
}
NW_HOOK_METHOD(0x1006aa30, NCLoginWnd, ResetWindows)

NW_CHECK_VTABLE(NCLoginWnd, 0x100e5fe8, 64)
