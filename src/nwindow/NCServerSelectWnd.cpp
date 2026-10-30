/*=============================================================================
    NCServerSelectWnd.cpp - the server list of the login screen and its list control.
    Stock: 0x100a1e60 .. 0x100a24c7, 0x100a2500 .. 0x100a39cf (OnDestroy is a body the
    linker folded with other classes).
=============================================================================*/
#include "nwindow/NCServerSelectWnd.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCAuthWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCrt.h"

#include <wchar.h>

// NWM_COMMAND ids, timer id (names ours)
enum
{
    NID_SERVER_OK = 0xF007,
    NID_SERVER_FRAME = 0xF008,
    NID_SERVER_CANCEL = 0xF017,
    NID_SERVER_LIST = 0xF018,
    TIMER_SERVER_RETRY = 0xF019,    // 3 s: the next OK may log in again
    NID_SERVER_QUALITY = 0xF020,
    NID_SERVERLIST_SORT = 0xF021,   // no control of the list sends it
};

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_Click = 0, SOUND_ClickDisabled = 2 };

// ping of a server that did not answer, or was not asked
enum { PING_None = 9999 };

// NConsoleWnd::SetLoadingTexture's screens, from the server's age rating (names ours)
enum
{
    LOADING_AllAges = 10001,
    LOADING_Over15 = 10002,
    LOADING_Over18 = 10003,
};

// names of NCsoft's servers 0..14, shown instead of sysstring 506 + id when the login server
// is theirs
#if NW_HOOK_MODE
#define GServerNames (reinterpret_cast<const TCHAR* const*>(::nw::Addr(0x101148e4)))
#else
extern const TCHAR* const GServerNames[];
#endif

// the console's login window and loading screen
static NCLoginWnd* ConsoleLoginWnd() { return GConsole->LoginWnd; }
static INT& ConsoleLoadingScreen() { return GConsole->LoadingScreenID; }

/*-----------------------------------------------------------------------------
    NCServerSelectListCtrl.
-----------------------------------------------------------------------------*/

// 0x100a3320
NCServerSelectListCtrl::NCServerSelectListCtrl()
{
}
NW_HOOK_CTOR(0x100a3320, NCServerSelectListCtrl)

// 0x100a33c0
NCServerSelectListCtrl::NCServerSelectListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns,
                                               INT Unused)
    : NCListCtrl(InMaxRows, InVisibleRows, InMaxColumns, Unused)
{
}
NW_HOOK_CTOR_ARGS(0x100a33c0, NCServerSelectListCtrl, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x100a3370
NW_HOOK_DTOR(0x100a3370, NCServerSelectListCtrl)

// 0x100a1e60 (vslot 24)
INT NCServerSelectListCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCListCtrl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_SERVERLIST_SORT)
        return SortByColumn((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100a1e60, NCServerSelectListCtrl, WindowProc)

// 0x100a30d0 (vslot 68) - NCListCtrl::SortByColumn's code, and the window's remembered row
// follows its server too
INT NCServerSelectListCtrl::SortByColumn(INT Column)
{
    NListRow* SelRow = GetSelectedRow();
    NCServerSelectWnd* Owner = (NCServerSelectWnd*)Entry->Parent->Wnd;
    NListRow* LastRow = NULL;
    if (Owner && Owner->LastServerRow >= 0 && Owner->LastServerRow < Rows.Num())
        LastRow = Rows(Owner->LastServerRow);
    for (INT i = 0; i < Rows.Num(); i++)
    {
        for (INT j = 0; j < Rows.Num() - i && j < Rows.Num() - 1; j++)
        {
            const TCHAR* A = Rows(j)->Cells[Column].Text;
            if (!*A)
                continue;
            const TCHAR* B = Rows(j + 1)->Cells[Column].Text;
            if (!*B)
                continue;
            NCHeaderCtrl* Header = Columns(Column);
            UBOOL bSwap;
            if (Header->bNumeric)
            {
                if (Header->bAscending)
                    bSwap = NWtol(A) > NWtol(B);
                else
                    bSwap = NWtol(A) < NWtol(B);
            }
            else
            {
                if (Header->bAscending)
                    bSwap = _wcsicmp(A, B) > 0;
                else
                    bSwap = _wcsicmp(A, B) < 0;
            }
            if (bSwap)
            {
                NListRow* Temp = Rows(j);
                Rows(j) = Rows(j + 1);
                Rows(j + 1) = Temp;
            }
        }
    }
    Columns(Column)->bAscending = !Columns(Column)->bAscending;
    if (SelRow)
    {
        for (Selected = 0; Selected < Rows.Num(); Selected++)
        {
            if (Rows(Selected) == SelRow)
                break;
        }
    }
    if (Owner && LastRow)
    {
        for (Owner->LastServerRow = 0; Owner->LastServerRow < Rows.Num(); Owner->LastServerRow++)
        {
            if (Rows(Owner->LastServerRow) == LastRow)
                break;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100a30d0, NCServerSelectListCtrl, SortByColumn)

NW_CHECK_VTABLE(NCServerSelectListCtrl, 0x100edcb0, 70)

/*-----------------------------------------------------------------------------
    NCServerSelectWnd.
-----------------------------------------------------------------------------*/

// 0x100a2360
NCServerSelectWnd::NCServerSelectWnd()
{
    BackTex = NULL;
    LastServerID = 0;
    bRequested = 0;
    PendingPings = 0;
    LastServerRow = -1;
}
NW_HOOK_CTOR(0x100a2360, NCServerSelectWnd)

// 0x100a23d0
NW_HOOK_DTOR(0x100a23d0, NCServerSelectWnd)

// 0x100a1ed0 (vslot 24)
INT NCServerSelectWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_SERVER_OK)
            return OnOK() != 0;
        if (wParam == NID_SERVER_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_SERVER_QUALITY)
            return SortByQuality() != 0;
        if (wParam == NID_SERVER_LIST)
            return OnListNotify((INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100a1ed0, NCServerSelectWnd, WindowProc)

// 0x100a2080 (vslot 34)
INT NCServerSelectWnd::OnTimer(INT TimerID)
{
    if (TimerID == TIMER_SERVER_RETRY)
        bRequested = 0;
    return 1;
}
NW_HOOK_METHOD(0x100a2080, NCServerSelectWnd, OnTimer)

// 0x100a33f0 (vslot 35) - the title bar gets its id but FrameID is left alone
INT NCServerSelectWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.lobbywnd.serverback"));
    TimerResult = SetTimer(TIMER_SERVER_RETRY, 3000, NULL);

    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_SERVER_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 300, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    ServerList = new NCServerSelectListCtrl(100, 40, 5, 12);
    ServerList->ID = NID_SERVER_LIST;
    ServerList->SetWindowRect(15, 26, 281, 472);
    CreateChild(ServerList, NWS_VISIBLE);
    ServerList->SetTextOffset(0, 3);

    NListColumnInfo NameColumn;
    NameColumn.Width = 138;
    NameColumn.Height = 15;
    NameColumn.SetTitle(*GL2GameData.SysString[451]);          // "Name"
    NameColumn.SetNormalTex(TEXT("L2UI.control.listButton"));
    NameColumn.SetPushedTex(TEXT("L2UI.control.listButton_click"));
    ServerList->AddColumn(0, &NameColumn);

    NListColumnInfo StatusColumn;
    StatusColumn.Width = 50;
    StatusColumn.Height = 15;
    StatusColumn.SetTitle(*GL2GameData.SysString[452]);        // "Status"
    StatusColumn.SetNormalTex(TEXT("L2UI.control.listButton"));
    StatusColumn.SetPushedTex(TEXT("L2UI.control.listButton_click"));
    ServerList->AddColumn(1, &StatusColumn);

    NListColumnInfo PingColumn;
    PingColumn.Width = 50;
    PingColumn.Height = 15;
    PingColumn.bNumeric = 1;
    PingColumn.SetTitle(*GL2GameData.SysString[601]);          // "Ping"
    PingColumn.SetNormalTex(TEXT("L2UI.control.listButton"));
    PingColumn.SetPushedTex(TEXT("L2UI.control.listButton_click"));
    ServerList->AddColumn(2, &PingColumn);

    NListColumnInfo TypeColumn;
    TypeColumn.Width = 30;
    TypeColumn.Height = 15;
    TypeColumn.SetTitle(*GL2GameData.SysString[602]);          // "Type"
    TypeColumn.SetNormalTex(TEXT("L2UI.control.listButton"));
    TypeColumn.SetPushedTex(TEXT("L2UI.control.listButton_click"));
    ServerList->AddColumn(3, &TypeColumn);

    OKButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                            TEXT("L2UI.Control.DefaultButton_click"), 0);
    OKButton->ID = NID_SERVER_OK;
    OKButton->SetWindowRect(117, 501, 67, 19);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(140);                                  // "OK"

    CancelButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    CancelButton->ID = NID_SERVER_CANCEL;
    CancelButton->SetWindowRect(211, 501, 67, 19);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(141);                              // "Cancel"

    QualityButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                 TEXT("L2UI.Control.DefaultButton_click"), 0);
    QualityButton->ID = NID_SERVER_QUALITY;
    QualityButton->SetWindowRect(25, 501, 67, 19);
    CreateChild(QualityButton, NWS_VISIBLE);
    QualityButton->SetTextID(603);                             // "Quality"

    bPingCacheOld = IsPingCacheOld();
    bPingSaved = 0;
    UnknownC4 = 0;
    return 1;
}
NW_HOOK_METHOD(0x100a33f0, NCServerSelectWnd, OnCreate)

// stock body shared with NCMenuWnd's (0x100367a0; NCActionWnd's too)
INT NCServerSelectWnd::OnDestroy()
{
    if (ServerList)
    {
        RemoveChildWindow(ServerList->Entry);
        delete ServerList;
        ServerList = NULL;
    }
    if (OKButton)
    {
        RemoveChildWindow(OKButton->Entry);
        delete OKButton;
        OKButton = NULL;
    }
    if (CancelButton)
    {
        RemoveChildWindow(CancelButton->Entry);
        delete CancelButton;
        CancelButton = NULL;
    }
    if (QualityButton)
    {
        RemoveChildWindow(QualityButton->Entry);
        delete QualityButton;
        QualityButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}

// 0x100a2420 (vslot 50) - under the 21-pixel title bar; the clip goes on before BeginPaint
INT NCServerSelectWnd::OnPaint(NCGDevice* Device)
{
    guard(NCServerSelectWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 21, 300, 505, 0, 0, 300, 505, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100a2420, NCServerSelectWnd, OnPaint)

// 0x100a2050 (vslot 53) - Enter logs in
INT NCServerSelectWnd::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN)
        OnOK();
    NCWnd::OnKeyDown(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x100a2050, NCServerSelectWnd, OnKeyDown)

// 0x100a2b30 (vslot 62) - log in to the selected server (address, port and id from its row);
// its age rating picks the Korean loading screen
INT NCServerSelectWnd::OnOK()
{
    NListRow* Row = ServerList->GetSelectedRow();
    if (Row)
    {
        const TCHAR* Age = Row->Cells[3].Text2;
        INT Screen = LOADING_AllAges;
        if (Age)
        {
            if (!appStrcmp(Age, *GL2GameData.SysString[462]))       // "Over 15 years"
                Screen = LOADING_Over15;
            else if (!appStrcmp(Age, *GL2GameData.SysString[461]))  // "Over 18 years"
                Screen = LOADING_Over18;
        }
        ConsoleLoadingScreen() = Screen;
        L2ParamStack Params(10);
        INT ServerID = Row->Param1;
        INT IP = Row->Param2;
        INT Port = appAtoi(Row->Text);
        Params.PushBack((void*)ServerID);
        Params.PushBack((void*)IP);
        Params.PushBack((void*)Port);
        if (!bRequested)
        {
            GConsole->RequestServerLogin(Params);
            SavePingTime();
            bRequested = 1;
        }
        GConsole->Engine->OnInterfacePlaySound(SOUND_Click);
    }
    else
        GConsole->Engine->OnInterfacePlaySound(SOUND_ClickDisabled);
    PendingPings = 0;
    return 1;
}
NW_HOOK_METHOD(0x100a2b30, NCServerSelectWnd, OnOK)

// 0x100a2cb0 (vslot 63) - back to the login box, account line focused
INT NCServerSelectWnd::OnCancel()
{
    GConsole->Engine->NetHandler->AuthReconnect();
    ConsoleLoginWnd()->ResetWindows();
    ConsoleLoginWnd()->ShowWindow();
    ConsoleLoginWnd()->AuthWnd->AccountEdit->SetFocus();
    GConsole->SetState(L2CS_Login);
    PendingPings = 0;
    SavePingTime();
    return 1;
}
NW_HOOK_METHOD(0x100a2cb0, NCServerSelectWnd, OnCancel)

// Bubble sort of rows First..Last by users (Cells[1].Param1, compared unsigned); inlined three
// times in SortByQuality in the stock code (name ours).
static void SortRowsByUsers(TArray<NListRow*>& Rows, INT First, INT Last)
{
    for (INT i = First; i < Last + 1; i++)
    {
        for (INT j = First; j < Last; j++)
        {
            if ((DWORD)Rows(j)->Cells[1].Param1 > (DWORD)Rows(j + 1)->Cells[1].Param1)
            {
                NListRow* Temp = Rows(j);
                Rows(j) = Rows(j + 1);
                Rows(j + 1) = Temp;
            }
        }
    }
}

// 0x100a2d30 (vslot 64) - "Quality": the server played last goes to the top (swapped with the
// first row), the others are sorted by ping, then each ping class (under 10 ms, 10 to 50, over
// 50) by users; unanswered servers (9999 ms) stay last, unsorted
INT NCServerSelectWnd::SortByQuality()
{
    INT NumFast = 0;
    INT NumMid = 0;
    INT NumSlow = 0;
    TArray<NListRow*>& Rows = ServerList->Rows;
    if (LastServerRow >= 0 && LastServerRow < ServerList->GetRowCount())
    {
        NListRow* Temp = Rows(0);
        Rows(0) = Rows(LastServerRow);
        Rows(LastServerRow) = Temp;
        ServerList->SetSelected(0);
        LastServerRow = 0;
    }
    for (INT i = 1; i < Rows.Num(); i++)
    {
        for (INT j = 1; j < Rows.Num() - i && j < Rows.Num() - 1; j++)
        {
            if (appAtoi(Rows(j)->Cells[2].Text) > appAtoi(Rows(j + 1)->Cells[2].Text))
            {
                NListRow* Temp = Rows(j);
                Rows(j) = Rows(j + 1);
                Rows(j + 1) = Temp;
            }
        }
    }
    for (INT i = 1; i < Rows.Num(); i++)
    {
        if (appAtoi(Rows(i)->Cells[2].Text) < 10)
            NumFast++;
        else if (appAtoi(Rows(i)->Cells[2].Text) > 50 && appAtoi(Rows(i)->Cells[2].Text) < PING_None)
            NumSlow++;
        else if (appAtoi(Rows(i)->Cells[2].Text) < PING_None)
            NumMid++;
    }
    SortRowsByUsers(Rows, 1, NumFast);
    SortRowsByUsers(Rows, NumFast + 1, NumFast + NumMid);
    SortRowsByUsers(Rows, NumFast + NumMid + 1, NumFast + NumMid + NumSlow);
    return 1;
}
NW_HOOK_METHOD(0x100a2d30, NCServerSelectWnd, SortByQuality)

// 0x100a2040 (vslot 65) - double click on a row
INT NCServerSelectWnd::OnListNotify(INT Row)
{
    OnOK();
    return 1;
}
NW_HOOK_METHOD(0x100a2040, NCServerSelectWnd, OnListNotify)

// 0x100a1fa0 - from the login server's list (NConsoleWnd::ReceiveServerList)
void NCServerSelectWnd::SetLastServerID(INT ServerID)
{
    LastServerID = ServerID;
}
NW_HOOK_METHOD(0x100a1fa0, NCServerSelectWnd, SetLastServerID)

// 0x100a1fb0 - NConsoleWnd::EndServerList: the last server gets selected, then the list is
// ordered
void NCServerSelectWnd::OnServerListEnd()
{
    if (LastServerRow >= 0 && LastServerRow < ServerList->GetRowCount())
        ServerList->SetSelected(LastServerRow);
    GConsole->Engine->NetHandler->EndServerPingList();
    if (bPingSaved)
        UnknownC4 = 1;
    SortByQuality();
}
NW_HOOK_METHOD(0x100a1fb0, NCServerSelectWnd, OnServerListEnd)

// 0x100a2010 - NCLoginWnd::ResetWindows
void NCServerSelectWnd::ClearServers()
{
    ServerList->ClearRows();
    LastServerID = 0;
    LastServerRow = -1;
    PendingPings = 0;
}
NW_HOOK_METHOD(0x100a2010, NCServerSelectWnd, ClearServers)

// 0x100a2500 - one server of the login server's list (NConsoleWnd::AddServerList): id,
// address, port, age limit, PvP, users, max users, state (0 down, 1 up) and flags, in that
// order. Cells: "[NN]name", load (users / max in its params), ping (PvP in Text2), type (a
// sandglass for flag 2, age limit in Text2). The saved ping is shown when there is a fresh one,
// else the server is asked (one more answer awaited). For other states the load text is left
// as it was on the stack, as in the stock code.
void NCServerSelectWnd::AddServer(L2ParamStack& Params)
{
    INT ServerID = (INT)Params.Top();
    INT IP = (INT)Params.Top();
    INT Port = (INT)Params.Top();
    INT AgeLimit = (INT)Params.Top();
    INT bPvP = (INT)Params.Top();
    INT Users = (INT)Params.Top();
    INT MaxUsers = (INT)Params.Top();
    INT State = (INT)Params.Top();
    INT Flags = (INT)Params.Top();
    TCHAR LoadText[32];
    TCHAR NameText[32];
    TCHAR AgeText[32];
    TCHAR Name[32];
    TCHAR PingText[32];
    TCHAR PvPText[32];

    NListRow* Row = new NListRow(5);
    if (ServerID == LastServerID)
        LastServerRow = ServerList->GetRowCount();
    if (ServerList->GetRowCount() < 9)
        appSprintf(NameText, TEXT("[0%d]"), ServerList->GetRowCount() + 1);
    else
        appSprintf(NameText, TEXT("[%d]"), ServerList->GetRowCount() + 1);
    const TCHAR* ServerName;
    if (ServerID <= 14 &&
        (!appStrcmp(GConfig->GetStr(TEXT("URL"), TEXT("ServerAddr")), TEXT("L2Server")) ||
         !appStrcmp(GConfig->GetStr(TEXT("URL"), TEXT("ServerAddr")), TEXT("l2server.ncsoft.co.kr"))))
        ServerName = GServerNames[ServerID];
    else
        ServerName = *GL2GameData.SysString[506 + ServerID];
    appSprintf(Name, TEXT("%s"), ServerName);
    appStrcat(NameText, Name);
    Row->Cells[0].SetText(NameText);
    Row->Cells[2].SetText(TEXT("9999ms"));

    if (State == 0)
        appSprintf(LoadText, TEXT("%s"), *GL2GameData.SysString[456]);
    else if (State == 1)
    {
        if ((SWORD)MaxUsers > 0)
        {
            FLOAT Load = (FLOAT)(SWORD)Users / (SWORD)MaxUsers * 100.f;
            if (Load < 95.f && (SWORD)Users < 4000)
            {
                if (!(Load < 90.f))
                    appSprintf(LoadText, TEXT("%s"), *GL2GameData.SysString[458]);
                else if (Load > 60.f)
                    appSprintf(LoadText, TEXT("%s"), *GL2GameData.SysString[457]);
                else
                    appSprintf(LoadText, TEXT("%s"), *GL2GameData.SysString[466]);
            }
            else
                appSprintf(LoadText, TEXT("%s"), *GL2GameData.SysString[467]);
        }
        else
            appSprintf(LoadText, TEXT("%s"), *GL2GameData.SysString[457]);

        if (!bPingCacheOld && !HasNoPing(ServerID - 1))
        {
            INT Ping = PING_None;
            GConfig->GetInt(appItoa(ServerID - 1), TEXT("Time"), Ping, TEXT("s_info.ini"));
            if (Ping == PING_None)
                appSprintf(PingText, TEXT("9999ms"));
            else
                appSprintf(PingText, TEXT("%dms"), Ping);
            Row->Cells[2].SetText(PingText);
        }
        else
        {
            Row->Cells[2].SetText(TEXT("9999ms"));
            GConsole->Engine->NetHandler->AddServerPingList(ServerID - 1, IP, Port);
            PendingPings++;
        }
    }

    Row->Cells[1].SetText(LoadText);
    if (bPvP == 1)
        appSprintf(PvPText, TEXT("%s"), *GL2GameData.SysString[459]);      // "On"
    else
        appSprintf(PvPText, TEXT("%s"), *GL2GameData.SysString[460]);      // "Off"
    if (AgeLimit == 18)
        appSprintf(AgeText, TEXT("%s"), *GL2GameData.SysString[461]);      // "Over 18 years"
    else if (AgeLimit == 15)
        appSprintf(AgeText, TEXT("%s"), *GL2GameData.SysString[462]);      // "Over 15 years"
    else
        appSprintf(AgeText, TEXT("%s"), *GL2GameData.SysString[539]);      // "Teen"
    if (Flags & 2)
        Row->Cells[3].SetIcon(FindTexture(TEXT("symbol.sandglass")), 10, 12);
    Row->Cells[2].SetText2(PvPText);
    Row->Cells[3].SetText2(AgeText);
    Row->SetParams(ServerID, IP);
    Row->SetText(appItoa(Port));
    Row->Cells[1].SetParams((SWORD)Users, (SWORD)MaxUsers);
    ServerList->AddRow(Row);
}
NW_HOOK_METHOD(0x100a2500, NCServerSelectWnd, AddServer)

// 0x100a2a50 - a ping answer (NConsoleWnd vslot 260): the rows of that address show it and
// save it; the list is ordered again when the last awaited answer arrives
void NCServerSelectWnd::SetPing(INT IP, INT Ping)
{
    TCHAR Text[32];
    if (!ServerList)
        return;
    for (INT i = 0; i < ServerList->Rows.Num(); i++)
    {
        NListRow* Row = ServerList->Rows(i);
        if (Row->Param2 == IP && Row->Cells)
        {
            if (Ping == PING_None)
                appSprintf(Text, TEXT("9999ms"));
            else
                appSprintf(Text, TEXT("%dms"), Ping);
            Row->Cells[2].SetText(Text);
            SavePing(Row->Param1 - 1, Ping);
            if (--PendingPings == 0)
            {
                SortByQuality();
                PendingPings = 0;
            }
        }
    }
}
NW_HOOK_METHOD(0x100a2a50, NCServerSelectWnd, SetPing)

// 0x100a2220 - no saved time, or more than 12 units of GetTimeStamp old
UBOOL NCServerSelectWnd::IsPingCacheOld()
{
    INT Stamp;
    if (GConfig->GetInt(TEXT("Date"), TEXT("Time"), Stamp, TEXT("s_info.ini")))
        return Stamp + 12 < GetTimeStamp();
    return 1;
}
NW_HOOK_METHOD(0x100a2220, NCServerSelectWnd, IsPingCacheOld)

// 0x100a2270 - no saved ping for the server, or 9999
UBOOL NCServerSelectWnd::HasNoPing(INT ServerIndex)
{
    INT Ping = 0;
    if (GConfig->GetInt(appItoa(ServerIndex), TEXT("Time"), Ping, TEXT("s_info.ini")))
        return Ping == PING_None;
    return 1;
}
NW_HOOK_METHOD(0x100a2270, NCServerSelectWnd, HasNoPing)

// 0x100a22d0 - not inlined: the stock caller calls it
__declspec(noinline) void NCServerSelectWnd::SavePing(INT ServerIndex, INT Ping)
{
    TCHAR Section[8];
    appSprintf(Section, TEXT("%d"), ServerIndex);
    GConfig->SetInt(Section, TEXT("Time"), Ping, TEXT("s_info.ini"));
    bPingSaved = 1;
}
NW_HOOK_METHOD(0x100a22d0, NCServerSelectWnd, SavePing)

// 0x100a2320 - after new pings were asked: when they were taken. Not inlined: the stock
// callers call it.
__declspec(noinline) void NCServerSelectWnd::SavePingTime()
{
    if (bPingCacheOld)
    {
        GConfig->SetInt(TEXT("Date"), TEXT("Time"), GetTimeStamp(), TEXT("s_info.ini"));
        bPingCacheOld = 0;
    }
}
NW_HOOK_METHOD(0x100a2320, NCServerSelectWnd, SavePingTime)

// 0x100a20a0
UBOOL NCServerSelectWnd::IsLeapYear(INT Year)
{
    return (Year % 4 == 0 && Year % 100 != 0) || Year % 400 == 0;
}
NW_HOOK_FUNC(0x100a20a0, NCServerSelectWnd::IsLeapYear)

// 0x100a20e0 - days of the years 1 .. Years
INT NCServerSelectWnd::DaysInYears(INT Years)
{
    INT Days = 0;
    for (INT i = 1; i <= Years; i++)
        Days += IsLeapYear(i) ? 366 : 365;
    return Days;
}
NW_HOOK_FUNC(0x100a20e0, NCServerSelectWnd::DaysInYears)

// 0x100a2120 - days of the months before Month (January = 1)
INT NCServerSelectWnd::DaysInMonths(INT Year, INT Month)
{
    INT February = 28 + (IsLeapYear(Year) != 0);
    INT Days = 0;
    for (INT i = 1; i < Month; i++)
    {
        if (i == 1 || i == 3 || i == 5 || i == 7 || i == 8 || i == 10 || i == 12)
            Days += 31;
        else if (i == 2)
            Days += February;
        else
            Days += 30;
    }
    return Days;
}
NW_HOOK_FUNC(0x100a2120, NCServerSelectWnd::DaysInMonths)

// 0x100a2190 - hours of the days before Day, plus Hour
INT NCServerSelectWnd::HoursInDays(INT Day, INT Hour)
{
    INT Hours = 0;
    if (Day > 1)
        Hours = (Day - 1) * 24;
    return Hours + Hour;
}
NW_HOOK_FUNC(0x100a2190, NCServerSelectWnd::HoursInDays)

// 0x100a21c0 - "now" for the ping cache: the days of the past years and of the months before
// last month plus the hours of this month, added as they are (the stock code mixes units)
INT NCServerSelectWnd::GetTimeStamp()
{
    INT Year, Month, Day, Hour, Min;
    appLocalTime(Year, Month, Day, Hour, Min);
    INT Stamp = DaysInMonths(Year, Month - 1);
    Stamp += DaysInYears(Year - 1);
    return HoursInDays(Day, Hour) + Stamp;
}
NW_HOOK_FUNC(0x100a21c0, NCServerSelectWnd::GetTimeStamp)

NW_CHECK_VTABLE(NCServerSelectWnd, 0x100edac8, 66)
