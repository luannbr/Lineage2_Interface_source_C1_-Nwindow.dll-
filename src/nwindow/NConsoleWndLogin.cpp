/*=============================================================================
    NConsoleWndLogin.cpp - what the engine tells the console before the game and around it
    (UGameEngine::On* handlers, one slot each): the login and its failures, the announcements,
    the server list, the character selection and creation, entering the world, the restart,
    the logout, a closed server, a blocked account, the protocol version.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCAuthWnd.h"
#include "nwindow/NCNoticeWnd.h"
#include "nwindow/NCWarningWnd.h"
#include "nwindow/NCServerSelectWnd.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCPawnCreateWnd.h"
#include "nwindow/NCLoadingWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCRestartMenuWnd.h"

// system messages (systemmsg ids)
enum
{
    MSG_Disconnected = 0,              // "Your connection has been disconnected from the server."
    MSG_TooManyCharacters = 77,        // "You cannot create another character. Please delete the
                                       //   existing character and try again."
    MSG_NameExists = 79,               // "Name already exists."
    MSG_NameInvalid = 80,              // "Please insert 1~16 English characters."
    MSG_ProtocolQuit = 88,             // "Protocol version is different. Please quit the program."
    MSG_ProtocolContinue = 89,         // "Protocol version is different. Please continue."
    MSG_DisconnectedRetry = 127,       // "Your connection has been disconnected from the server.
                                       //   Please try again."
    MSG_CreateFailed = 128,            // "You have failed to create a character."
    MSG_DeleteFailed = 306,            // "You have failed to delete the character."
    MSG_AskRevive = 332,               // "Do you want to be restored?"
    MSG_TemporaryPassword = 396,       // "Please login after changing your temporary password."
    MSG_NoTimeLeft = 398,              // "You have no more time left on your account."
    MSG_SystemError = 399,             // "System error."
    MSG_BlockReason = 428,             // .. 443: a reason each, a bit of BlockedAccount's
    MSG_Withdrawn = 444,               // "The following account may not be accessed due to a
                                       //   withdrawal request that has been submitted." and
                                       //   445..447 after it
    MSG_LoginSystemError = 448,        // "System error, please log in again later."
    MSG_WrongPassword = 449,           // "Password does not match this acount."
    MSG_ConfirmAccount = 450,          // "Confirm your account information and log in again later."
    MSG_WrongAccount = 453,            // "Your account information is incorrect."
    MSG_ContactSupport = 454,          // "Please contact the Customer Support Center or the 1: 1
                                       //   inquiry service of the Lineage II official website."
    MSG_AccountInUse = 455,            // "The account is already in use. Access denied."
    MSG_AdultsOnly = 456,              // "Lineage II game services may only be used by adults 18
                                       //   years of age and older."
    MSG_Maintenance = 457,             // "Server under maintenance. Please try again later."
    MSG_TermExpired = 458,             // "Your usage term has been expired." 459, 460 after it
    MSG_AccessFailed = 461,            // "Access failed." 462, 463 after it
    MSG_ClanMemberDelete = 540,        // "Clan leaders cannot be deleted. DD Dissolve the clan and
                                       //   try again."
    MSG_ClanLeaderDelete = 541,        // "You cannot delete a clan member. Withdraw from the clan
                                       //   and try again."
    MSG_KoreaOnly = 621,               // "This server is reserved for players in Korea..."
    MSG_WeekTimeOver = 756,            // "This week's usage time has finished."
};

// the dialog boxes opened here (NDialogInfo::ID; NCDialogBox sends the answer)
enum
{
    DIALOG_DISCONNECTED = 0x1D,        // OK only: the game ends
    DIALOG_REVIVE = 0x20,
};

// a system message's text
static FORCEINLINE const TCHAR* SysMsg(INT ID)
{
    return *GL2GameData.SystemMsg[ID].Message;
}

// back to the login box, the server list and the announcement boxes hidden (after the login
// failures)
static FORCEINLINE void ShowAuthWnd(NConsoleWnd* Console)
{
    Console->LoginWnd->ServerSelectWnd->HideWindow();
    Console->LoginWnd->NoticeWnd->HideWindow();
    Console->LoginWnd->WarningWnd->HideWindow();
    Console->LoginWnd->AuthWnd->ShowWindow();
    Console->LoginWnd->AuthWnd->AccountEdit->SetFocus();
    Console->SetState(L2CS_Login);
}

/*-----------------------------------------------------------------------------
    The login.
-----------------------------------------------------------------------------*/

// 0x10047790 - the login server refused: the reason's message, then the login box again (for
// the reasons without a message the stock shows an uninitialized buffer; empty here)
void NConsoleWnd::AuthLoginFail(INT Reason)
{
    TCHAR Text[256];
    Text[0] = 0;
    switch (Reason)
    {
    case 1:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_LoginSystemError));
        break;
    case 2:
    case 3:
        appSprintf(Text, TEXT("%s %s"), SysMsg(MSG_WrongPassword), SysMsg(MSG_ConfirmAccount));
        break;
    case 5:
        appSprintf(Text, TEXT("%s %s"), SysMsg(MSG_WrongAccount), SysMsg(MSG_ContactSupport));
        break;
    case 7:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_AccountInUse));
        break;
    case 12:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_AdultsOnly));
        break;
    case 16:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_Maintenance));
        break;
    case 17:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_TemporaryPassword));
        break;
    case 18:
        appSprintf(Text, TEXT("%s %s %s"), SysMsg(MSG_TermExpired), SysMsg(MSG_TermExpired + 1),
                   SysMsg(MSG_TermExpired + 2));
        break;
    case 19:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_NoTimeLeft));
        break;
    case 20:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_SystemError));
        break;
    case 21:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_AccessFailed));
        break;
    case 22:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_KoreaOnly));
        break;
    case 30:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_WeekTimeOver));
        break;
    case 4:
    case 6:
    case 8:
    case 9:
    case 10:
    case 11:
    case 13:
    case 14:
    case 15:
        appSprintf(Text, TEXT("%s %s %s"), SysMsg(MSG_AccessFailed), SysMsg(MSG_AccessFailed + 1),
                   SysMsg(MSG_AccessFailed + 2));
        break;
    }
    ShowMessageWnd(Text);
    ShowAuthWnd(this);
}
NW_HOOK_METHOD(0x10047790, NConsoleWnd, AuthLoginFail)

// 0x10047a70 - after the login: the announcements (NoticeText, from the game data), if there
// are any, else straight to the server list; the value after them goes to the box (its bit 0:
// the warning box follows)
void NConsoleWnd::StartNotice(L2ParamStack& Params)
{
    if (GL2GameData.NoticeText.Len() > 0)
    {
        LoginWnd->AuthWnd->HideWindow();
        LoginWnd->ServerSelectWnd->HideWindow();
        LoginWnd->NoticeWnd->ShowWindow();
        LoginWnd->NoticeWnd->SetFocus();
        LoginWnd->NoticeWnd->ClearText();
        LoginWnd->NoticeWnd->SetText(*GL2GameData.NoticeText);
        LoginWnd->NoticeWnd->Type = (INT)Params.Top();
        SetState(L2CS_Login);
    }
    else
        RequestServerList();
}
NW_HOOK_METHOD(0x10047a70, NConsoleWnd, StartNotice)

// 0x10043280 - the server list follows (AddServerList), with the server last played on
void NConsoleWnd::ReceiveServerList(INT LastServerID)
{
    guard(NConsoleWnd::ReceiveServerList);
    MessageWnd->ClearText();
    OptionWnd->HideWindow();
    LoginWnd->ResetWindows();
    LoginWnd->AuthWnd->HideWindow();
    LoginWnd->NoticeWnd->HideWindow();
    LoginWnd->WarningWnd->HideWindow();
    LoginWnd->ServerSelectWnd->SetLastServerID(LastServerID);
    LoginWnd->ServerSelectWnd->ShowWindow();
    LoginWnd->ServerSelectWnd->SetFocus();
    unguard;
}
NW_HOOK_METHOD(0x10043280, NConsoleWnd, ReceiveServerList)

// 0x10043390
void NConsoleWnd::AddServerList(L2ParamStack& Params)
{
    guard(NConsoleWnd::AddServerList);
    LoginWnd->ServerSelectWnd->AddServer(Params);
    unguard;
}
NW_HOOK_METHOD(0x10043390, NConsoleWnd, AddServerList)

// 0x10043420
void NConsoleWnd::EndServerList()
{
    guard(NConsoleWnd::EndServerList);
    LoginWnd->ServerSelectWnd->OnServerListEnd();
    unguard;
}
NW_HOOK_METHOD(0x10043420, NConsoleWnd, EndServerList)

// 0x10047170 - the ping of a server of the list
void NConsoleWnd::SetServerPing(INT ServerID, DWORD Ping)
{
    if (LoginWnd->ServerSelectWnd)
        LoginWnd->ServerSelectWnd->SetPing(ServerID, Ping);
}
NW_HOOK_METHOD(0x10047170, NConsoleWnd, SetServerPing)

// 0x10047490 - the account is blocked, with the server's text: the message is built from
// four lines of systemmsg and takes the text
void NConsoleWnd::BlockedAccountWithMsg(TCHAR* Msg)
{
    TCHAR Format[256];
    TCHAR Text[256];
    if (Msg && *Msg)
    {
        appSprintf(Format, TEXT("%s %s %s %s"), SysMsg(MSG_Withdrawn), SysMsg(MSG_Withdrawn + 1),
                   SysMsg(MSG_Withdrawn + 2), SysMsg(MSG_Withdrawn + 3));
        appSprintf(Text, Format, Msg);
        ShowMessageWnd(Text);
    }
    ShowAuthWnd(this);
}
NW_HOOK_METHOD(0x10047490, NConsoleWnd, BlockedAccountWithMsg)

// 0x100475c0 - the account is blocked: the message of the lowest reason bit set (0..15)
void NConsoleWnd::BlockedAccount(INT Reason)
{
    if (Reason & 0x0001)
        ShowMessageWnd(SysMsg(MSG_BlockReason));
    else if (Reason & 0x0002)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 1));
    else if (Reason & 0x0004)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 2));
    else if (Reason & 0x0008)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 3));
    else if (Reason & 0x0010)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 4));
    else if (Reason & 0x0020)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 5));
    else if (Reason & 0x0040)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 6));
    else if (Reason & 0x0080)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 7));
    else if (Reason & 0x0100)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 8));
    else if (Reason & 0x0200)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 9));
    else if (Reason & 0x0400)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 10));
    else if (Reason & 0x0800)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 11));
    else if (Reason & 0x1000)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 12));
    else if (Reason & 0x2000)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 13));
    else if (Reason & 0x4000)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 14));
    else if (Reason & 0x8000)
        ShowMessageWnd(SysMsg(MSG_BlockReason + 15));
    ShowAuthWnd(this);
}
NW_HOOK_METHOD(0x100475c0, NConsoleWnd, BlockedAccount)

// 0x10048050 - the server's protocol version: 0 when it differs. With [URL] L2VersionCheck
// (1 when not set) the client logs out.
void NConsoleWnd::ReceiveVersionCheck(L2ParamStack& Params)
{
    BYTE bSame = (BYTE)(INT)Params.Top();
    UBOOL bVersionCheck = 1;
    if (!GConfig->GetBool(TEXT("URL"), TEXT("L2VersionCheck"), bVersionCheck, NULL))
        bVersionCheck = 1;
    if (bVersionCheck)
    {
        if (!bSame)
        {
            Engine->NetHandler->SendLogOutPacket();  // RequestLogout's
            ShowMessageWnd(SysMsg(MSG_ProtocolQuit));
        }
    }
    else if (!bSame)
        ShowMessageWnd(SysMsg(MSG_ProtocolContinue));
}
NW_HOOK_METHOD(0x10048050, NConsoleWnd, ReceiveVersionCheck)

/*-----------------------------------------------------------------------------
    The character selection and creation, entering the world.
-----------------------------------------------------------------------------*/

// 0x100434a0 - the character list follows (AddCharacterInfo): the selection replaces the
// login screen (not while in the world)
void NConsoleWnd::ReceiveCharacterInfo()
{
    guard(NConsoleWnd::ReceiveCharacterInfo);
    if (State != L2CS_InGame)
    {
        LoginWnd->ResetWindows();
        LoginWnd->HideWindow();
        OptionWnd->HideWindow();
        PawnCreateWnd->HideWindow();
        LobbyWnd->ClearCharacters();
        LobbyWnd->ShowWindow();
        MoveCameraByState(L2CS_None, L2CS_Lobby);
        SetState(L2CS_Lobby);
    }
    unguard;
}
NW_HOOK_METHOD(0x100434a0, NConsoleWnd, ReceiveCharacterInfo)

// 0x10043580
void NConsoleWnd::AddCharacterInfo(User Info)
{
    guard(NConsoleWnd::AddCharacterInfo);
    check(LobbyWnd);
    if (State != L2CS_InGame)
        LobbyWnd->AddCharacter(Info);
    unguard;
}
NW_HOOK_METHOD(0x10043580, NConsoleWnd, AddCharacterInfo)

// 0x10047c80 - into the world: the desktop replaces the loading screen, and the screen takes
// the game's resolution if it differs (the result is not read)
void NConsoleWnd::AcceptGameStart(INT Result)
{
    guard(NConsoleWnd::AcceptGameStart);
    check(Desktop);
    LoadingWnd->HideWindow();
    Desktop->ShowWindow();
    UViewport* Viewport = Engine->Client->Viewports(0);
    INT ScreenX = GSEKScreenX;
    if (Viewport->SizeX != ScreenX)
        Engine->GRenDev->SetRes(Viewport, ScreenX, GSEKScreenY, GColorBits / 8, GRefreshRate,
                                Unknown3720, 1);  // +0x3720: fullscreen
    SetState(L2CS_InGame);
    unguard;
}
NW_HOOK_METHOD(0x10047c80, NConsoleWnd, AcceptGameStart)

// 0x10047d90 - only the failures are answered
void NConsoleWnd::AcceptCharacterDelete(INT Result)
{
    guard(NConsoleWnd::AcceptCharacterDelete);
    switch (Result)
    {
    case 1:
        ShowMessageWnd(SysMsg(MSG_DeleteFailed));
        break;
    case 2:
        ShowMessageWnd(SysMsg(MSG_ClanLeaderDelete));
        break;
    case 3:
        ShowMessageWnd(SysMsg(MSG_ClanMemberDelete));
        break;
    }
    unguard;
}
NW_HOOK_METHOD(0x10047d90, NConsoleWnd, AcceptCharacterDelete)

// 0x10047e40 - the creation may start: its screen replaces the selection
void NConsoleWnd::AcceptNewCharacter(INT Result)
{
    guard(NConsoleWnd::AcceptNewCharacter);
    if (Result)
    {
        LobbyWnd->HideWindow();
        PawnCreateWnd->Reset();
        PawnCreateWnd->ShowWindow();
        MoveCameraByState(L2CS_None, L2CS_Create);
        SetState(L2CS_Create);
    }
    else
    {
        SetState(L2CS_Lobby);
        ShowMessageWnd(SysMsg(MSG_CreateFailed));
    }
    unguard;
}
NW_HOOK_METHOD(0x10047e40, NConsoleWnd, AcceptNewCharacter)

// 0x10047f30 - only a failure is answered (the list follows a success)
void NConsoleWnd::AcceptCharacterCreate(INT Result)
{
    guard(NConsoleWnd::AcceptCharacterCreate);
    if (!Result)
        ShowMessageWnd(SysMsg(MSG_CreateFailed));
    unguard;
}
NW_HOOK_METHOD(0x10047f30, NConsoleWnd, AcceptCharacterCreate)

// 0x10047b90 - the creation was refused: 1 too many characters (the choices start over), 2 the
// name is taken, 3 the name is not valid (the name is typed again)
void NConsoleWnd::CharacterCreateFail(INT Reason)
{
    switch (Reason)
    {
    case 0:
        ShowMessageWnd(SysMsg(MSG_CreateFailed));
        break;
    case 1:
        ShowMessageWnd(SysMsg(MSG_TooManyCharacters));
        PawnCreateWnd->SetupWnd->Reset();
        break;
    case 2:
        ShowMessageWnd(SysMsg(MSG_NameExists));
        PawnCreateWnd->SetupWnd->NameEdit->ClearText();
        PawnCreateWnd->SetupWnd->NameEdit->SetFocus();
        break;
    case 3:
        ShowMessageWnd(SysMsg(MSG_NameInvalid));
        PawnCreateWnd->SetupWnd->NameEdit->ClearText();
        PawnCreateWnd->SetupWnd->NameEdit->SetFocus();
        break;
    }
    SetState(L2CS_Create);
}
NW_HOOK_METHOD(0x10047b90, NConsoleWnd, CharacterCreateFail)

// 0x100438e0 - the template of a class for the creation
void NConsoleWnd::AddRecommandedData(RecommandedDataStr Data)
{
    guard(NConsoleWnd::AddRecommandedData);
    PawnCreateWnd->SetRecommendedData(Data);
    unguard;
}
NW_HOOK_METHOD(0x100438e0, NConsoleWnd, AddRecommandedData)

// 0x10043970
void NConsoleWnd::EndRecommandedData()
{
    guard(NConsoleWnd::EndRecommandedData);
    PawnCreateWnd->EndRecommendedData();
    unguard;
}
NW_HOOK_METHOD(0x10043970, NConsoleWnd, EndRecommandedData)

/*-----------------------------------------------------------------------------
    The restart, the revival, the logout, a closed server.
-----------------------------------------------------------------------------*/

// 0x10043640 - the restart was accepted (first): the game's windows give way to the loading
// screen
void NConsoleWnd::RestartResetWindows()
{
    GetDesktop()->Reset();
    Desktop->HideWindow();
    OptionWnd->HideWindow();
    PetitionWnd->HideWindow();
    LoadingWnd->ShowWindow();
    RestartMenuWnd->HideWindow();
}
NW_HOOK_METHOD(0x10043640, NConsoleWnd, RestartResetWindows)

// 0x100436a0 - (then) back to the character selection: the engine travels to the lobby map
// ("Lobby?L2Restart", from the [DefaultPlayer] URL) and the selection is shown again
void NConsoleWnd::RestartToLobby()
{
    FString Error;
    FURL DefaultURL(NULL);
    DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), TEXT("User"));
    FURL URL(&DefaultURL, TEXT("Lobby"), TRAVEL_Partial);
    URL.AddOption(TEXT("L2Restart"));
    if (!static_cast<UGameEngine*>(Engine)->Browse(URL, NULL, Error))
        GError->Logf(LocalizeError(TEXT("FailedBrowse"), TEXT("Engine")), *Error);
    SpawnDefaultCharacter();
    LoadingWnd->HideWindow();
    LobbyWnd->ClearCharacters();
    LobbyWnd->ShowWindow();
    SetState(L2CS_Lobby);
    MoveCameraByState(L2CS_None, L2CS_Lobby);
}
NW_HOOK_METHOD(0x100436a0, NConsoleWnd, RestartToLobby)

// 0x1004f830 - someone offers to revive the dead player (the offer's two values go back with
// the answer); only in the world
void NConsoleWnd::ReviveRequest(INT Param1, INT Param2)
{
    if (State != L2CS_InGame)
        return;
    RestartMenuWnd->HideWindow();
    NDialogInfo Info;
    Info.ID = DIALOG_REVIVE;
    Info.bMainBox = 1;
    Info.bInput = 0;
    Info.Owner = NULL;
    Info.SetParams(Param1, Param2);
    Info.SetSystemMessage(MSG_AskRevive);
    ShowDialog(&Info);
}
NW_HOOK_METHOD(0x1004f830, NConsoleWnd, ReviveRequest)

// 0x1004f820 - the server let the player go: the window positions are saved and the game
// ends
void NConsoleWnd::AcceptLogOut()
{
    SaveWindowPositions();
    appRequestExit(0);
}
NW_HOOK_METHOD(0x1004f820, NConsoleWnd, AcceptLogOut)

#if NW_FEATURES
// phase 6: ReturnToLogin (included here, below the check()s: their __LINE__ stays the faithful
// build's)
#include "features/ReturnToLogin.h"
#endif

// the stock body is AcceptLogOut's (folded)
void NConsoleWnd::QuitGame()
{
#if NW_FEATURES
    // phase 6: ReturnToLogin (NWindow_C1 src/inject/ReturnToLogin.cpp): OK on the
    // "disconnected" box (the only caller) goes back to the login screen from the next Tick
    // instead of ending the game
    if (nwf::ReturnToLogin_QuitGame())
        return;
#endif
    SaveWindowPositions();
    appRequestExit(0);
}

// 0x1004f6b0 - the server closed the connection (reason 2 marks the text): in the world a
// dialog box whose OK ends the game, before it the login stages' own box
void NConsoleWnd::ServerClose(INT Reason)
{
    if (State == L2CS_InGame)
    {
        NDialogInfo Info;
        Info.ID = DIALOG_DISCONNECTED;
        Info.bMainBox = 1;
        Info.bInput = 0;
        Info.Owner = NULL;
        if (Reason == 0 || Reason == 1)
            appStrcpy(Info.Text, SysMsg(MSG_DisconnectedRetry));
        else if (Reason == 2)
            appSprintf(Info.Text, TEXT("!%s"), SysMsg(MSG_DisconnectedRetry));
        ShowDialog(&Info);
    }
    else
    {
        NDialogInfo Info;
        Info.ID = DIALOG_DISCONNECTED;
        Info.bInput = 0;
        Info.Owner = NULL;
        if (Reason == 0 || Reason == 1)
            appStrcpy(Info.Text, SysMsg(MSG_DisconnectedRetry));
        else if (Reason == 2)
            appSprintf(Info.Text, TEXT("  %s"), SysMsg(MSG_DisconnectedRetry));
        DialogBox->SetInfo(&Info);
        DialogBox->ShowWindow();
        DialogBox->SetFocus();
    }
}
NW_HOOK_METHOD(0x1004f6b0, NConsoleWnd, ServerClose)

// 0x10047fc0 - the console's own: the connection is gone (on the message line)
void NConsoleWnd::ReceiveServerClose()
{
    guard(NConsoleWnd::ReceiveServerClose);
    ShowMessageWnd(SysMsg(MSG_Disconnected));
    unguard;
}
NW_HOOK_METHOD(0x10047fc0, NConsoleWnd, ReceiveServerClose)
