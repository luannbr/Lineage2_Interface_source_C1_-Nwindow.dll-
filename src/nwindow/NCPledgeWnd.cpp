/*=============================================================================
    NCPledgeWnd.cpp - the clan page. Stock: 0x10033da0 .. 0x10034053, 0x10034060 .. 0x10034203,
    0x10034210 .. 0x100342b9, 0x10034f50 .. 0x1003504f, 0x10035050 .. 0x100360d9,
    0x100360e0 .. 0x100364b5, 0x10036dc0 .. 0x10036e14 (the rest of the range is
    NCActionWnd's and NCActionItemWnd's).
=============================================================================*/
#include "nwindow/NCPledgeWnd.h"
#include "nwindow/NCListCtrl.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCDialogBox.h"

#include <wchar.h>

// systemmsg lines
enum
{
    MSG_TargetNotFound = 50,          // "Target can not be found."
    MSG_ConfirmWithdraw = 68,         // "Withdraw from the $s1 Clan. Do you want to continue?"
    MSG_ConfirmExpel = 69,            // "Expel $s1 from the clan. Do you want to continue?"
    MSG_SelectInvitee = 186,          // "Select user to invite to your clan."
    MSG_EnterWarTarget = 206,         // "Enter name of clan to declare war on."
    MSG_CrestFormat = 211,            // "Can only register 16*12 sized bmp files of 256 colors."
    MSG_LeaderCannotWithdraw = 239,   // "The clan leader cannot withdraw."
    MSG_EnterSurrenderTarget = 252,   // "Enter the name of clan to surrender to."
    MSG_EnterEndWarTarget = 253,      // "Enter the name of clan to request end of war."
    MSG_EnterName = 256,              // "Enter name."
    MSG_SelectMember = 259,           // "Select clan members from list."
};

// sysstring lines
enum
{
    STR_Name = 50,
    STR_Lv = 88,
    STR_Leader = 342,
    STR_Base = 343,
    STR_HideoutExists = 344,
    STR_CastleExists = 345,
    STR_Status = 346,
    STR_On = 347,
    STR_Out = 348,
};

// dialogs of the buttons (NCDialogBox::OnOK / OnCancel act on the ID)
enum
{
    DIALOG_GiveTitle = 0xF,
    DIALOG_RegisterCrest = 0x10,
    DIALOG_DeclareWar = 0x12,
    DIALOG_EndWar = 0x14,
    DIALOG_Surrender = 0x17,
    DIALOG_Expel = 0x18,
    DIALOG_SurrenderPersonally = 0x19,
    DIALOG_Withdraw = 0x1A,
};

enum { CLAN_TEXT_COLOR = 0xFFB09B79, CLAN_LABEL_COLOR = 0xFFA3A3A3 };

// one button of the page (repeated inline in the stock OnCreate)
static NCButton* CreateButton(NCWnd* Parent, INT ID, INT X, INT Y, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("L2UI.control.DefaultButton"),
                                    TEXT("L2UI.control.DefaultButton_click"), 0);
    Button->ID = ID;
    Button->SetWindowRect(X, Y, 67, 19);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->SetTextID(TextID);
    return Button;
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

// the member list columns (repeated inline in the stock OnCreate)
static void AddMemberColumn(NCListCtrl* List, INT Width, INT TitleID, UBOOL bNumeric)
{
    NListColumnInfo Info;
    Info.Width = Width;
    Info.Height = 15;
    Info.bNumeric = bNumeric;
    Info.SetTitleID(TitleID);
    Info.SetNormalTex(TEXT("L2UI.Control.listbutton"));
    Info.SetPushedTex(TEXT("L2UI.Control.listbutton_click"));
    List->AddColumn(0, &Info);
}

// 0x10034f50
NCPledgeWnd::NCPledgeWnd()
{
    BackTex = NULL;
    CrestTex = NULL;
    Info.Name[0] = 0;
    Info.LeaderName[0] = 0;
    Info.StatusText[0] = 0;
    Info.PledgeID = 0;
    Info.CrestID = 0;
    Info.UnknownE4 = 0;
    Info.Level = 0;
    Info.bHasCastle = 0;
    Info.bHasHideout = 0;
    Info.Unknown94 = 0;
    Info.Unknown98 = 0;
    Info.Status = 0;
    Info.UnknownE0 = 0;
    MemberList = NULL;
}
NW_HOOK_CTOR(0x10034f50, NCPledgeWnd)

// 0x10035000
NW_HOOK_DTOR(0x10035000, NCPledgeWnd)

// 0x10033da0 (vslot 24)
INT NCPledgeWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_PLEDGE_INVITE)
            return OnInvite() != 0;
        if ((INT)wParam == NID_PLEDGE_TITLE)
            return OnGiveTitle() != 0;
        if ((INT)wParam == NID_PLEDGE_CREST)
            return OnSetCrest() != 0;
        if ((INT)wParam == NID_PLEDGE_WAR)
            return OnDeclareWar() != 0;
        if ((INT)wParam == NID_PLEDGE_TRUCE)
            return OnEndWar() != 0;
        if ((INT)wParam == NID_PLEDGE_SURRENDER)
            return OnSurrender() != 0;
        if ((INT)wParam == NID_PLEDGE_EXPEL)
            return OnExpel() != 0;
        if ((INT)wParam == NID_PLEDGE_YIELD)
            return OnSurrenderPersonally() != 0;
        if ((INT)wParam == NID_PLEDGE_LEAVE)
            return OnWithdraw() != 0;
    }
    else if (Msg == (UINT)NWM_DIALOG_RESULT)
        return OnDialogResult(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10033da0, NCPledgeWnd, WindowProc)

// 0x10035050 (vslot 35) - the member list (name, level, on / out) and the nine buttons
INT NCPledgeWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.ActionWnd.PledgeBack"));

    MemberList = new NCListCtrl(1000, 8, 3, 12);
    MemberList->SetWindowRect(15, 82, 221, 125);
    CreateChild(MemberList, NWS_VISIBLE);
    MemberList->SetTextOffset(0, 3);
    AddMemberColumn(MemberList, 128, STR_Name, 0);
    AddMemberColumn(MemberList, 28, STR_Lv, 1);
    AddMemberColumn(MemberList, 53, STR_Status, 0);

    InviteButton = CreateButton(this, NID_PLEDGE_INVITE, 15, 215, 330);        // "Invite"
    TitleButton = CreateButton(this, NID_PLEDGE_TITLE, 15, 234, 331);          // "Title"
    CrestButton = CreateButton(this, NID_PLEDGE_CREST, 15, 253, 332);          // "Set Crest"
    WarButton = CreateButton(this, NID_PLEDGE_WAR, 86, 215, 333);              // "Battle"
    TruceButton = CreateButton(this, NID_PLEDGE_TRUCE, 86, 234, 334);          // "Truce"
    SurrenderButton = CreateButton(this, NID_PLEDGE_SURRENDER, 86, 253, 335);  // "Surrender"
    ExpelButton = CreateButton(this, NID_PLEDGE_EXPEL, 157, 215, 338);         // "Dismiss"
    YieldButton = CreateButton(this, NID_PLEDGE_YIELD, 157, 234, 336);         // "Yield"
    LeaveButton = CreateButton(this, NID_PLEDGE_LEAVE, 157, 253, 337);         // "Leave"
    return 1;
}
NW_HOOK_METHOD(0x10035050, NCPledgeWnd, OnCreate)

// 0x10035c10 (vslot 36)
INT NCPledgeWnd::OnDestroy()
{
    DestroyChild(this, MemberList);
    DestroyChild(this, InviteButton);
    DestroyChild(this, TitleButton);
    DestroyChild(this, CrestButton);
    DestroyChild(this, WarButton);
    DestroyChild(this, TruceButton);
    DestroyChild(this, SurrenderButton);
    DestroyChild(this, ExpelButton);
    DestroyChild(this, YieldButton);
    DestroyChild(this, LeaveButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10035c10, NCPledgeWnd, OnDestroy)

// 0x10033f30 (vslot 42) - leader: everything but Leave; members: only Leave (and Yield,
// which is never touched)
void NCPledgeWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    User* Me = GConsole->GetMyUser();
    if (!Me)
        return;
    if ((Me->StatusFlags >> 6) & 1)
    {
        InviteButton->EnableWindow(1);
        TitleButton->EnableWindow(1);
        CrestButton->EnableWindow(1);
        WarButton->EnableWindow(1);
        TruceButton->EnableWindow(1);
        SurrenderButton->EnableWindow(1);
        ExpelButton->EnableWindow(1);
        LeaveButton->EnableWindow(0);
    }
    else
    {
        InviteButton->EnableWindow(0);
        TitleButton->EnableWindow(0);
        CrestButton->EnableWindow(0);
        WarButton->EnableWindow(0);
        TruceButton->EnableWindow(0);
        SurrenderButton->EnableWindow(0);
        ExpelButton->EnableWindow(0);
        LeaveButton->EnableWindow(1);
    }
}
NW_HOOK_METHOD(0x10033f30, NCPledgeWnd, ShowWindow)

// 0x10035880 (vslot 50) - the clip opens 17 pixels above the window (the tab row), before
// BeginPaint, and closes after EndPaint, as in the stock code. StatusText is drawn after the
// leader's name, measured from the leader's name, but nothing sets it.
INT NCPledgeWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY - 17, Width, Height + 17);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, -17, 240, 301, 0, 0, 240, 301, BackTex);

    INT NameX = 18;
    if (Info.CrestID > 0)
    {
        UTexture* Crest = GConsole->GetPledgeCrestTexture(Info.PledgeID);
        if (Crest)
        {
            Device->DrawTexture(18, 18, 16, 12, 0, 4, 16, 12, Crest);
            NameX = 38;
        }
    }
    SIZE Size;
    const TCHAR* Name = Info.Name;
    if (Name && *Name)
    {
        NCGDevice::GetTextExtent(&Size, Name, 0, 0, 0, 0, 0);  // not used
        Device->DrawText(NameX, 19, CLAN_TEXT_COLOR, Name, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }

    if (*GL2GameData.SysString[STR_Leader])
        Device->DrawText(17, 33, CLAN_LABEL_COLOR, *GL2GameData.SysString[STR_Leader], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    const TCHAR* Leader = Info.LeaderName;
    if (Leader && *Leader)
        Device->DrawText(60, 33, CLAN_TEXT_COLOR, Leader, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    const TCHAR* Status = Info.StatusText;
    if (Status && *Status)
    {
        NCGDevice::GetTextExtent(&Size, Leader, 0, 0, 0, 0, 0);
        Device->DrawText(Size.cx + 48, 33, CLAN_TEXT_COLOR, Status, 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                         0, 0, 0);
    }
    if (*GL2GameData.SysString[STR_Lv])
        Device->DrawText(139, 33, CLAN_LABEL_COLOR, *GL2GameData.SysString[STR_Lv], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    TCHAR Number[10];
    appSprintf(Number, TEXT("%d"), Info.Level);
    Device->DrawText(158, 33, CLAN_TEXT_COLOR, Number, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);

    if (*GL2GameData.SysString[STR_Base])
        Device->DrawText(17, 47, CLAN_LABEL_COLOR, *GL2GameData.SysString[STR_Base], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (Info.bHasCastle)
    {
        Device->DrawText(60, 47, CLAN_TEXT_COLOR, *GL2GameData.SysString[STR_CastleExists], 0, 0,
                         0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        if (Info.bHasHideout)
        {
            NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_CastleExists], 0, 0, 0, 0,
                                     0);
            Device->DrawText(Size.cx + 62, 47, CLAN_TEXT_COLOR,
                             *GL2GameData.SysString[STR_HideoutExists], 0, 0, 0, L2FONT_Normal, 0,
                             0, 0, 0, 0, 0);
        }
    }
    else if (Info.bHasHideout)
        Device->DrawText(60, 47, CLAN_TEXT_COLOR, *GL2GameData.SysString[STR_HideoutExists], 0, 0,
                         0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10035880, NCPledgeWnd, OnPaint)

// 0x10034210 (vslot 62) - the target, when it is a player
INT NCPledgeWnd::OnInvite()
{
    User* Target = GConsole->GetSelectedActor();
    if (Target && !Target->Unknown8)
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)Target->Pawn->CreatureID);
        GConsole->RequestJoinPledge(Params);
    }
    else
        GConsole->ShowSystemMessage(MSG_SelectInvitee, CLAN_TEXT_COLOR);
    return 1;
}
NW_HOOK_METHOD(0x10034210, NCPledgeWnd, OnInvite)

// 0x10036180 (vslot 63) - the file name is typed in the box
INT NCPledgeWnd::OnSetCrest()
{
    NDialogInfo Dialog;
    Dialog.ID = DIALOG_RegisterCrest;
    Dialog.bMainBox = 1;
    Dialog.bInput = 0;
    Dialog.Type = 0;
    Dialog.SetSystemMessage(MSG_CrestFormat);
    GConsole->ShowDialog(&Dialog);
    return 1;
}
NW_HOOK_METHOD(0x10036180, NCPledgeWnd, OnSetCrest)

// 0x100361e0 (vslot 64)
INT NCPledgeWnd::OnDeclareWar()
{
    NDialogInfo Dialog;
    Dialog.ID = DIALOG_DeclareWar;
    Dialog.bMainBox = 1;
    Dialog.bInput = 1;
    Dialog.Type = 0;
    Dialog.SetSystemMessage(MSG_EnterWarTarget);
    GConsole->ShowDialog(&Dialog);
    return 1;
}
NW_HOOK_METHOD(0x100361e0, NCPledgeWnd, OnDeclareWar)

// 0x10036240 (vslot 65)
INT NCPledgeWnd::OnEndWar()
{
    NDialogInfo Dialog;
    Dialog.ID = DIALOG_EndWar;
    Dialog.bMainBox = 1;
    Dialog.bInput = 1;
    Dialog.Type = 0;
    Dialog.SetSystemMessage(MSG_EnterEndWarTarget);
    GConsole->ShowDialog(&Dialog);
    return 1;
}
NW_HOOK_METHOD(0x10036240, NCPledgeWnd, OnEndWar)

// 0x100362a0 (vslot 66)
INT NCPledgeWnd::OnSurrender()
{
    NDialogInfo Dialog;
    Dialog.ID = DIALOG_Surrender;
    Dialog.bMainBox = 1;
    Dialog.bInput = 1;
    Dialog.Type = 0;
    Dialog.SetSystemMessage(MSG_EnterSurrenderTarget);
    GConsole->ShowDialog(&Dialog);
    return 1;
}
NW_HOOK_METHOD(0x100362a0, NCPledgeWnd, OnSurrender)

// 0x10036300 (vslot 67) - the selected member's name goes in the first button's text as well
// as in the parameter, as in the stock code
INT NCPledgeWnd::OnExpel()
{
    NListRow* Row = MemberList->GetSelectedRow();
    if (!Row)
    {
        GConsole->ShowSystemMessage(MSG_SelectMember, CLAN_TEXT_COLOR);
        return 1;
    }
    const TCHAR* Member = Row->Cells->Text;
    if (Member && *Member)
    {
        NDialogInfo Dialog;
        Dialog.ID = DIALOG_Expel;
        Dialog.bMainBox = 1;
        Dialog.bInput = 0;
        Dialog.SetSystemMessage(MSG_ConfirmExpel);
        Dialog.SetButtons(Row->Cells->Text, NULL);
        Dialog.SetParamText(Row->Cells->Text);
        GConsole->ShowDialog(&Dialog);
    }
    return 1;
}
NW_HOOK_METHOD(0x10036300, NCPledgeWnd, OnExpel)

// 0x100363b0 (vslot 68) - the same message as OnSurrender
INT NCPledgeWnd::OnSurrenderPersonally()
{
    NDialogInfo Dialog;
    Dialog.ID = DIALOG_SurrenderPersonally;
    Dialog.bMainBox = 1;
    Dialog.bInput = 1;
    Dialog.Type = 0;
    Dialog.SetSystemMessage(MSG_EnterSurrenderTarget);
    GConsole->ShowDialog(&Dialog);
    return 1;
}
NW_HOOK_METHOD(0x100363b0, NCPledgeWnd, OnSurrenderPersonally)

// 0x10036410 (vslot 69) - the clan's name goes in the first button's text
INT NCPledgeWnd::OnWithdraw()
{
    User* Me = GConsole->GetMyUser();
    if (Me && !((Me->StatusFlags >> 6) & 1))
    {
        NDialogInfo Dialog;
        Dialog.ID = DIALOG_Withdraw;
        Dialog.bMainBox = 1;
        Dialog.bInput = 0;
        Dialog.SetSystemMessage(MSG_ConfirmWithdraw);
        Dialog.SetButtons(Info.Name, NULL);
        GConsole->ShowDialog(&Dialog);
        return 1;
    }
    GConsole->ShowSystemMessage(MSG_LeaderCannotWithdraw, CLAN_TEXT_COLOR);
    return 1;
}
NW_HOOK_METHOD(0x10036410, NCPledgeWnd, OnWithdraw)

// 0x100360e0 (vslot 70) - the title of the target, when it is a player
INT NCPledgeWnd::OnGiveTitle()
{
    User* Target = GConsole->GetSelectedActor();
    if (Target && !Target->Unknown8)
    {
        NDialogInfo Dialog;
        Dialog.ID = DIALOG_GiveTitle;
        Dialog.bMainBox = 1;
        Dialog.bInput = 1;
        Dialog.Type = 0;
        Dialog.SetSystemMessage(MSG_EnterName);
        GConsole->ShowDialog(&Dialog);
        return 1;
    }
    GConsole->ShowSystemMessage(MSG_TargetNotFound, CLAN_TEXT_COLOR);
    return 1;
}
NW_HOOK_METHOD(0x100360e0, NCPledgeWnd, OnGiveTitle)

// 0x10034060 - the status text, UnknownE0 and UnknownE4 are left out; the names only when
// given
void NCPledgeWnd::SetPledgeInfo(NPledgeInfo* InInfo)
{
    Info.PledgeID = InInfo->PledgeID;
    if (*InInfo->Name)
        appStrcpy(Info.Name, InInfo->Name);
    const TCHAR* Leader = InInfo->LeaderName;
    if (Leader && *Leader)
        appStrcpy(Info.LeaderName, Leader);
    Info.CrestID = InInfo->CrestID;
    Info.Level = InInfo->Level;
    Info.bHasCastle = InInfo->bHasCastle;
    Info.bHasHideout = InInfo->bHasHideout;
    Info.Unknown94 = InInfo->Unknown94;
    Info.Unknown98 = InInfo->Unknown98;
    Info.Status = InInfo->Status;
    Info.UnknownE8 = InInfo->UnknownE8;
}
NW_HOOK_METHOD(0x10034060, NCPledgeWnd, SetPledgeInfo)

// 0x10034110 - the same without the ID and the names
void NCPledgeWnd::UpdatePledgeInfo(NPledgeInfo* InInfo)
{
    Info.CrestID = InInfo->CrestID;
    Info.Level = InInfo->Level;
    Info.bHasCastle = InInfo->bHasCastle;
    Info.bHasHideout = InInfo->bHasHideout;
    Info.Unknown94 = InInfo->Unknown94;
    Info.Unknown98 = InInfo->Unknown98;
    Info.Status = InInfo->Status;
    Info.UnknownE8 = InInfo->UnknownE8;
}
NW_HOOK_METHOD(0x10034110, NCPledgeWnd, UpdatePledgeInfo)

// 0x10034180
const TCHAR* NCPledgeWnd::GetPledgeName()
{
    const TCHAR* Name = Info.Name;
    if (!Name || !*Name)
        return NULL;
    return Name;
}
NW_HOOK_METHOD(0x10034180, NCPledgeWnd, GetPledgeName)

// 0x100341a0
void NCPledgeWnd::ClearMembers()
{
    MemberList->ClearRows();
}
NW_HOOK_METHOD(0x100341a0, NCPledgeWnd, ClearMembers)

// 0x100341b0 - UnknownE8 is kept
void NCPledgeWnd::ClearInfo()
{
    Info.Name[0] = 0;
    Info.LeaderName[0] = 0;
    Info.StatusText[0] = 0;
    Info.PledgeID = 0;
    Info.CrestID = 0;
    Info.UnknownE4 = 0;
    Info.Level = 0;
    Info.bHasCastle = 0;
    Info.bHasHideout = 0;
    Info.Unknown94 = 0;
    Info.Unknown98 = 0;
    Info.Status = 0;
    Info.UnknownE0 = 0;
}
NW_HOOK_METHOD(0x100341b0, NCPledgeWnd, ClearInfo)

// 0x10035de0
void NCPledgeWnd::Reset()
{
    ClearInfo();
    ClearMembers();
}
NW_HOOK_METHOD(0x10035de0, NCPledgeWnd, Reset)

// 0x10035df0 - a member already listed (same name) is updated instead
void NCPledgeWnd::AddMember(NPledgeMember* Member)
{
    TArray<NListRow*>* Rows = MemberList->GetRows();
    for (INT i = 0; i < Rows->Num(); i++)
    {
        const TCHAR* Name = Member->Name;
        const TCHAR* RowName = (*Rows)(i)->Cells->Text;
        if (Name && *Name && RowName && *RowName && !_wcsicmp(RowName, Name))
        {
            (*Rows)(i)->Cells[1].SetNumber(Member->Level);
            if (Member->bOnline)
            {
                if (*GL2GameData.SysString[STR_On])
                    (*Rows)(i)->Cells[2].SetText(*GL2GameData.SysString[STR_On]);
            }
            else if (*GL2GameData.SysString[STR_Out])
                (*Rows)(i)->Cells[2].SetText(*GL2GameData.SysString[STR_Out]);
            return;
        }
    }
    NListRow* Row = new NListRow(3);
    Row->Cells[0].SetText(Member->Name);
    Row->Cells[1].SetNumber(Member->Level);
    if (Member->bOnline)
    {
        if (*GL2GameData.SysString[STR_On])
            Row->Cells[2].SetText(*GL2GameData.SysString[STR_On]);
    }
    else if (*GL2GameData.SysString[STR_Out])
        Row->Cells[2].SetText(*GL2GameData.SysString[STR_Out]);
    MemberList->AddRow(Row);
}
NW_HOOK_METHOD(0x10035df0, NCPledgeWnd, AddMember)

// 0x10035fe0 - members not listed are ignored
void NCPledgeWnd::UpdateMember(NPledgeMember* Member)
{
    TArray<NListRow*>* Rows = MemberList->GetRows();
    for (INT i = 0; i < Rows->Num(); i++)
    {
        const TCHAR* Name = Member->Name;
        const TCHAR* RowName = (*Rows)(i)->Cells->Text;
        if (Name && *Name && RowName && *RowName && !_wcsicmp(RowName, Name))
        {
            (*Rows)(i)->Cells[1].SetNumber(Member->Level);
            if (Member->bOnline)
            {
                if (*GL2GameData.SysString[STR_On])
                    (*Rows)(i)->Cells[2].SetText(*GL2GameData.SysString[STR_On]);
            }
            else if (*GL2GameData.SysString[STR_Out])
                (*Rows)(i)->Cells[2].SetText(*GL2GameData.SysString[STR_Out]);
            return;
        }
    }
}
NW_HOOK_METHOD(0x10035fe0, NCPledgeWnd, UpdateMember)

// 0x10036dc0 - the row leaves the list but is not deleted, and the list's row count and
// scroll range stay as they were, as in the stock code
void NCPledgeWnd::DeleteMember(const TCHAR* Name)
{
    TArray<NListRow*>* Rows = MemberList->GetRows();
    for (INT i = 0; i < Rows->Num(); i++)
    {
        if (Name && !_wcsicmp((*Rows)(i)->Cells->Text, Name))
        {
            Rows->Remove(i, 1);
            return;
        }
    }
}
NW_HOOK_METHOD(0x10036dc0, NCPledgeWnd, DeleteMember)

NW_CHECK_VTABLE(NCPledgeWnd, 0x100df8f0, 72)
