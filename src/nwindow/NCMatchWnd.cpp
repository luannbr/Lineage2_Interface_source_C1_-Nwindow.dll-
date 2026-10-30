/*=============================================================================
    NCMatchWnd.cpp - party matching. Stock: 0x100744f0 .. 0x100745d5, 0x10075110 .. 0x10075141,
    0x10075180 .. 0x100757a0, 0x100757b0 .. 0x100759c5, 0x10075f70 .. 0x100767a6
    (ShowWindow's and HideWindow's bodies are shared at 0x10078950 and 0x1003dab0).
=============================================================================*/
#include "nwindow/NCMatchWnd.h"
#include "nwindow/NCMatchListCtrl.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <stdlib.h>
#include <wchar.h>

// stock globals (names ours)
#if NW_HOOK_MODE
#define GTellPrefix NW_GLOBAL(const TCHAR*, 0x10111890)  // L"\"", the tell marker of the chat line
#define GClassMagicType (reinterpret_cast<const INT*>(::nw::Addr(0x10111b68)))  // per class
                                                                                 //   ID: 0 fighter, 1 mystic
#else
extern const TCHAR* GTellPrefix;
extern const INT GClassMagicType[];
#endif

// sysstring lines
enum
{
    STR_Cls = 391,
    STR_Lv = 392,
    STR_Name = 393,
    STR_Range = 394,
    STR_PartyStatus = 395,      // "P/S"
    STR_Invite = 396,
    STR_Options = 397,
    STR_Whisper = 398,
    STR_Targeting = 399,
    STR_Short = 405,
    STR_Mid = 406,
    STR_Long = 407,
    STR_Leader = 408,
    STR_Member = 409,
    STR_Solo = 410,
    STR_FindersKeepers = 487,
    STR_Random = 488,
    STR_LootingMethod = 489,
};

enum { MSG_SelectTarget = 326 };  // "Select target from list."

// one button of the window (repeated inline in the stock OnCreate)
static NCButton* CreateButton(NCWnd* Parent, INT ID, INT X, INT Y, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                    TEXT("L2UI.Control.DefaultButton_click"), 0);
    Button->ID = ID;
    Button->SetWindowRect(X, Y, 67, 19);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->SetTextID(TextID);
    return Button;
}

// the list columns (repeated inline in the stock OnCreate)
static void AddMatchColumn(NCListCtrl* List, INT Width, INT TitleID, UBOOL bNumeric)
{
    NListColumnInfo Info;
    Info.Width = Width;
    Info.Height = 15;
    Info.bNumeric = bNumeric;
    Info.SetTitleID(TitleID);
    Info.SetNormalTex(TEXT("L2UI.Control.listButton"));
    Info.SetPushedTex(TEXT("L2UI.Control.listButton_click"));
    List->AddColumn(0, &Info);
}

// one child of OnDestroy (repeated inline in the stock code)
template <class T> static void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x10075110
NCMatchWnd::NCMatchWnd()
{
    MatchList = NULL;
    InviteButton = NULL;
    OptionsButton = NULL;
    WhisperButton = NULL;
    TargetingButton = NULL;
}
NW_HOOK_CTOR(0x10075110, NCMatchWnd)

// 0x10075180
NW_HOOK_DTOR(0x10075180, NCMatchWnd)

// 0x100744f0 (vslot 24)
INT NCMatchWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_MATCH_INVITE)
            return OnInvite() != 0;
        if ((INT)wParam == NID_MATCH_OPTIONS)
            return OnOptions() != 0;
        if ((INT)wParam == NID_MATCH_WHISPER)
            return OnWhisper() != 0;
        if ((INT)wParam == NID_MATCH_TARGETING)
            return OnTargeting() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100744f0, NCMatchWnd, WindowProc)

// 0x10075f70 (vslot 35)
INT NCMatchWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = 0xF6;
    FrameCtrl->SetWindowRect(0, 0, 300, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI.MatchWnd.MatchingBack"));

    MatchList = new NCMatchListCtrl(40, 13, 5, 16);
    MatchList->SetWindowRect(15, 42, 269, 196);
    CreateChild(MatchList, NWS_VISIBLE);
    MatchList->SetTextOffset(0, 3);
    AddMatchColumn(MatchList, 29, STR_Cls, 1);
    AddMatchColumn(MatchList, 29, STR_Lv, 1);
    AddMatchColumn(MatchList, 113, STR_Name, 0);
    AddMatchColumn(MatchList, 40, STR_Range, 0);
    AddMatchColumn(MatchList, 40, STR_PartyStatus, 0);

    InviteButton = CreateButton(this, NID_MATCH_INVITE, 147, 239, STR_Invite);
    OptionsButton = CreateButton(this, NID_MATCH_OPTIONS, 217, 239, STR_Options);
    WhisperButton = CreateButton(this, NID_MATCH_WHISPER, 147, 258, STR_Whisper);
    TargetingButton = CreateButton(this, NID_MATCH_TARGETING, 217, 258, STR_Targeting);
    return 1;
}
NW_HOOK_METHOD(0x10075f70, NCMatchWnd, OnCreate)

// 0x100751d0 (vslot 36)
INT NCMatchWnd::OnDestroy()
{
    DestroyChild(this, MatchList);
    DestroyChild(this, InviteButton);
    DestroyChild(this, OptionsButton);
    DestroyChild(this, WhisperButton);
    DestroyChild(this, TargetingButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100751d0, NCMatchWnd, OnDestroy)

// stock body shared with NCSystemMenuWnd's (0x10078950) and four other windows': shown again
// from its minimized icon, the window takes the icon's place and the icon goes away
void NCMatchWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Icon)
    {
        SetWindowRect(Icon->ScreenX, Icon->ScreenY, Width, Height);
        Icon->SetWindowRect(0, 0, 0, 0);
        Icon->HideWindow();
    }
}

// stock body shared with NCChatWnd's (0x1003dab0)
void NCMatchWnd::HideWindow()
{
    NCWnd::HideWindow();
}

// 0x100752c0 (vslot 50) - the clip is set before BeginPaint and released after EndPaint, as
// in the stock code
INT NCMatchWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 21, 300, 256, 0, 0, 300, 256, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100752c0, NCMatchWnd, OnPaint)

// 0x100757d0 (vslot 62)
INT NCMatchWnd::OnInvite()
{
    INT ID = MatchList->GetSelectedPlayerID();
    if (ID > 0)
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)ID);
        GConsole->RequestPartyInvite(Params);
    }
    else
        GConsole->ShowSystemMessage(MSG_SelectTarget, 0xFFB09B79);
    return 1;
}
NW_HOOK_METHOD(0x100757d0, NCMatchWnd, OnInvite)

// 0x100745c0 (vslot 63) - the desktop shows or hides NCMatchOptionWnd
INT NCMatchWnd::OnOptions()
{
    GDesktop->ToggleMatchOption();
    return 1;
}
NW_HOOK_METHOD(0x100745c0, NCMatchWnd, OnOptions)

// 0x10075870 (vslot 64) - starts a tell to the selected player in the chat line
INT NCMatchWnd::OnWhisper()
{
    const TCHAR* Name = MatchList->GetSelectedName();
    if (Name && *Name)
    {
        GDesktop->ChatWnd->EditBox->ClearText();
        GDesktop->ChatWnd->SetLastTellName(Name);
        GDesktop->ChatWnd->AddTellName(Name);
        TCHAR Buffer[32];
        wcscpy(Buffer, Name);
        INT Len = (INT)wcslen(Buffer);
        Buffer[Len] = ' ';
        Buffer[Len + 1] = 0;
        if (Buffer[0])
        {
            GDesktop->ChatWnd->EditBox->InsertText(GTellPrefix, 1);
            GDesktop->ChatWnd->EditBox->InsertText(Buffer, 1);
        }
        return 1;
    }
    GConsole->ShowSystemMessage(MSG_SelectTarget, 0xFFB09B79);
    return 1;
}
NW_HOOK_METHOD(0x10075870, NCMatchWnd, OnWhisper)

// 0x10075990 (vslot 65)
INT NCMatchWnd::OnTargeting()
{
    INT ID = MatchList->GetSelectedPlayerID();
    if (ID > 0)
    {
        GConsole->RequestTarget(ID);
        return 1;
    }
    GConsole->ShowSystemMessage(MSG_SelectTarget, 0xFFB09B79);
    return 1;
}
NW_HOOK_METHOD(0x10075990, NCMatchWnd, OnTargeting)

// 0x10075350 - one player: class icon (fighter / mystic), level icon for the difference from
// the player's own level (up to 15 either way), name (red when flagged), range from the
// player (up to 10000) and party status. Anything out of range drops the row.
void NCMatchWnd::AddPlayer(L2ParamStack& Params)
{
    INT ObjectID = (INT)Params.Top();
    const TCHAR* Name = (const TCHAR*)Params.Top();
    INT Level = (INT)Params.Top();
    INT ClassID = (INT)Params.Top();
    INT bRedName = (INT)Params.Top();
    Params.Top();  // not used
    INT PartyStatus = (INT)Params.Top();
    INT X = (INT)Params.Top();
    INT Y = (INT)Params.Top();
    INT Z = (INT)Params.Top();
    User* Me = GConsole->GetMyUser();
    if (!Me)
        return;
    NListRow* Row = new NListRow(5);

    INT MagicType = GClassMagicType[ClassID];
    if (MagicType == 0)
    {
        Row->Cells[0].SetIcon(FindTexture(TEXT("sek.cbui348")), 12, 12);
        Row->Cells[0].SetParams(0, ClassID);
    }
    else if (MagicType == 1)
    {
        Row->Cells[0].SetIcon(FindTexture(TEXT("sek.cbui349")), 12, 12);
        Row->Cells[0].SetParams(1, ClassID);
    }
    else
    {
        delete Row;
        return;
    }

    // the sort key orders from far above (0) to far below (4)
    INT Diff = Level - Me->Level;
    if (abs(Diff) <= 3)
    {
        Row->Cells[1].SetIcon(FindTexture(TEXT("sek.cbui345")), 12, 12);
        Row->Cells[1].SetParams(2, Level);
    }
    else if (Diff >= 4 && Diff <= 7)
    {
        Row->Cells[1].SetIcon(FindTexture(TEXT("sek.cbui344")), 12, 12);
        Row->Cells[1].SetParams(1, Level);
    }
    else if (Diff >= -7 && Diff <= -4)
    {
        Row->Cells[1].SetIcon(FindTexture(TEXT("sek.cbui346")), 12, 12);
        Row->Cells[1].SetParams(3, Level);
    }
    else if (Diff >= 8 && Diff <= 15)
    {
        Row->Cells[1].SetIcon(FindTexture(TEXT("sek.cbui343")), 12, 12);
        Row->Cells[1].SetParams(0, Level);
    }
    else if (Diff >= -15 && Diff <= -8)
    {
        Row->Cells[1].SetIcon(FindTexture(TEXT("sek.cbui347")), 12, 12);
        Row->Cells[1].SetParams(4, Level);
    }
    else
    {
        delete Row;
        return;
    }

    Row->Cells[2].SetText(Name);
    if (bRedName)
        Row->Cells[2].SetColor(0xFFFF0000);
    Row->Cells[2].SetParams(ObjectID, -1);

    FVector MyLoc = GConsole->GetMyLocation();
    FVector Delta(MyLoc.X - (FLOAT)X, MyLoc.Y - (FLOAT)Y, MyLoc.Z - (FLOAT)Z);
    INT Dist = (INT)Delta.Size();
    if (Dist >= 0 && Dist <= 1500)
    {
        Row->Cells[3].SetText(*GL2GameData.SysString[STR_Short]);
        Row->Cells[3].SetParams(0, -1);
    }
    else if (Dist > 1500 && Dist <= 3000)
    {
        Row->Cells[3].SetText(*GL2GameData.SysString[STR_Mid]);
        Row->Cells[3].SetParams(1, -1);
    }
    else if (Dist > 3000 && Dist <= 10000)
    {
        Row->Cells[3].SetText(*GL2GameData.SysString[STR_Long]);
        Row->Cells[3].SetParams(2, -1);
    }
    else
    {
        delete Row;
        return;
    }

    if (PartyStatus == 0)
        Row->Cells[4].SetText(*GL2GameData.SysString[STR_Solo]);
    else if (PartyStatus == 1)
        Row->Cells[4].SetText(*GL2GameData.SysString[STR_Member]);
    else if (PartyStatus == 2)
        Row->Cells[4].SetText(*GL2GameData.SysString[STR_Leader]);
    else
    {
        delete Row;
        return;
    }

    MatchList->AddRow(Row);
    if (ObjectID == SelectedID)
        MatchList->SetSelected(MatchList->GetRowCount() - 1);
}
NW_HOOK_METHOD(0x10075350, NCMatchWnd, AddPlayer)

// 0x100757b0
void NCMatchWnd::ClearList()
{
    SelectedID = MatchList->GetSelectedPlayerID();
    MatchList->ClearRows();
}
NW_HOOK_METHOD(0x100757b0, NCMatchWnd, ClearList)

// 0x100765f0 - the details a click asked for: looting method and memo (the row text), level
// and class open (the row params), then the tool tip text. The row is taken from a copy of
// the row array, as in the stock code; the search stops at the first row without a player.
void NCMatchWnd::UpdatePlayerInfo(L2ParamStack& Params)
{
    INT ObjectID = (INT)Params.Top();
    INT bLevelOpen = (INT)Params.Top();
    INT bClassOpen = (INT)Params.Top();
    INT LootType = (INT)Params.Top();
    const TCHAR* Memo = (const TCHAR*)Params.Top();
    for (INT i = 0; i < MatchList->GetRows()->Num(); i++)
    {
        if (MatchList->GetRowPlayerID(i) < 0)
            return;
        if (MatchList->GetRowPlayerID(i) == ObjectID)
        {
            TArray<NListRow*> Rows(*MatchList->GetRows());
            NListRow* Row = Rows(i);
            TCHAR Text[512];
            switch (LootType)
            {
            case 0:
                appSprintf(Text, TEXT("%s: %s\n%s"), *GL2GameData.SysString[STR_LootingMethod],
                           *GL2GameData.SysString[STR_FindersKeepers], Memo);
                break;
            case 1:
                appSprintf(Text, TEXT("%s: %s\n%s"), *GL2GameData.SysString[STR_LootingMethod],
                           *GL2GameData.SysString[STR_Random], Memo);
                break;
            default:
                appStrcpy(Text, Memo);
                break;
            }
            Row->SetText(Text);
            Row->SetParams(bLevelOpen, bClassOpen);
            MatchList->BuildToolTip(Row);
            return;
        }
    }
}
NW_HOOK_METHOD(0x100765f0, NCMatchWnd, UpdatePlayerInfo)

NW_CHECK_VTABLE(NCMatchWnd, 0x100e75d0, 66)
