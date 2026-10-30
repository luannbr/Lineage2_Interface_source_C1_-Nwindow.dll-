/*=============================================================================
    NCSiegeInfoMainWnd.cpp - the castle siege window, its tabs and its clan list.
    Stock: 0x100ab3a0 .. 0x100adf1c (the TArray<NSiegeClanInfo*> code at 0x100ad570 is the
    template's).
=============================================================================*/
#include "nwindow/NCSiegeInfoMainWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"

// NWM_COMMAND ids (names ours)
enum
{
    NID_SIEGE_FRAME = 0xF22035,
    NID_SIEGE_TABS = 0xF22036,
    NID_SIEGE_APPLY = 0xF023A0,
    NID_SIEGE_CANCELAPPLY = 0xF023A1,
    NID_SIEGE_APPROVE = 0xF023A2,
    NID_SIEGE_REFUSE = 0xF023A3,
    NID_SIEGE_TIMECOMBO = 0xA0F311,
};

// what the date's dialog sends its owner on OK (NCDialogBox::OnOK, name ours)
enum { NWM_SIEGE_DATE_OK = (INT)0xF0000011 };

// NDialogInfo::ID (names ours): 0x39 answers the date tab, the others the server (NCDialogBox)
enum
{
    DIALOG_SIEGE_DATE = 0x39,
    DIALOG_SIEGE_ATTACK = 0x3A,
    DIALOG_SIEGE_CANCEL_ATTACK = 0x3B,
    DIALOG_SIEGE_DEFEND = 0x3C,
    DIALOG_SIEGE_CANCEL_DEFEND = 0x3D,
    DIALOG_SIEGE_APPROVE = 0x3E,
    DIALOG_SIEGE_REFUSE = 0x3F,
};

// systemmsg lines
enum
{
    MSG_ConfirmSiegeTime = 663,      // "The siege time has been declared for $s. ..."
    MSG_ApplyAttack = 667,           // "You are registering on the attacking side of the $s1 siege. ..."
    MSG_ApplyDefense = 668,          // "... on the defending side of the $s1 siege. ..."
    MSG_CancelApplication = 669,
    MSG_RefuseDefender = 670,
    MSG_ApproveDefender = 671,
};

// sysstring lines
enum
{
    STR_Leader = 342,
    STR_Approved = 567,
    STR_Waiting = 568,
    STR_AllianceLeader = 569,
    STR_ApplyTime = 570,
    STR_AllianceName = 571,
    STR_Apply = 572,
    STR_CancelApply = 573,
    STR_Approve = 574,
    STR_Cancel = 575,                // the refuse button's (579 "Refuse" goes unused)
    STR_AttackingClans = 576,
    STR_DefendingClans = 577,
    STR_WaitingClans = 578,
    STR_ClanName = 580,
    STR_Status = 581,
    STR_CurrentTime = 582,
    STR_SiegeDate = 583,
    STR_Undecided = 584,
    STR_AutoSetting = 585,
    STR_SiegeInfo = 586,
    STR_DefenseInfo = 587,
    STR_Lord = 588,
    STR_Clan = 589,
    STR_Alliance = 590,
    STR_NoAlliance = 591,
    STR_NoCastleOwner = 595,
};

enum { SOUND_WindowOpen = 5 };  // interface sound (FL2NetNotify::OnInterfacePlaySound)

enum { SIEGE_TITLE_COLOR = 0xFFB09B79, SIEGE_LABEL_COLOR = 0xFFA3A3A3 };

// a child window goes (repeated inline in the stock OnDestroys)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// the siege window a tab belongs to
static FORCEINLINE NCSiegeInfoMainWnd* MainOf(NCWnd* Tab)
{
    return (NCSiegeInfoMainWnd*)Tab->Entry->Parent->Wnd;
}

// a cell's text, NULL when empty (inlined in the stock code)
static FORCEINLINE const TCHAR* CellText(NListCell& Cell)
{
    return (Cell.Text && Cell.Text[0]) ? Cell.Text : NULL;
}

/*-----------------------------------------------------------------------------
    NSiegeCastleInfo, NSiegeClanInfo.
-----------------------------------------------------------------------------*/

// 0x100ab750
__declspec(noinline) NSiegeCastleInfo& NSiegeCastleInfo::operator=(const NSiegeCastleInfo& Other)
{
    appStrcpy(AllianceName, Other.AllianceName);
    appStrcpy(LordName, Other.LordName);
    appStrcpy(ClanName, Other.ClanName);
    CastleID = Other.CastleID;
    bIsOwner = Other.bIsOwner;
    PledgeID = Other.PledgeID;
    AllianceID = Other.AllianceID;
    CurrentTime = Other.CurrentTime;
    SiegeTime = Other.SiegeTime;
    return *this;
}
NW_HOOK_METHOD(0x100ab750, NSiegeCastleInfo, operator=)

// 0x100ab7e0
__declspec(noinline) NSiegeClanInfo::NSiegeClanInfo(const NSiegeClanInfo& Other)
{
    appStrcpy(ClanName, Other.ClanName);
    appStrcpy(LeaderName, Other.LeaderName);
    appStrcpy(AllianceName, Other.AllianceName);
    appStrcpy(AllianceLeader, Other.AllianceLeader);
    PledgeID = Other.PledgeID;
    CrestID = Other.CrestID;
    ApplyTime = Other.ApplyTime;
    Type = Other.Type;
    AllianceID = Other.AllianceID;
}
NW_HOOK_CTOR_ARGS(0x100ab7e0, NSiegeClanInfo, (const NSiegeClanInfo& A), (A))

/*-----------------------------------------------------------------------------
    NCSiegeInfoListCtrl.
-----------------------------------------------------------------------------*/

// 0x100ad610
NCSiegeInfoListCtrl::NCSiegeInfoListCtrl()
{
}
NW_HOOK_CTOR(0x100ad610, NCSiegeInfoListCtrl)

// 0x100ad6b0
NCSiegeInfoListCtrl::NCSiegeInfoListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns,
                                         INT Unused)
    : NCListCtrl(InMaxRows, InVisibleRows, InMaxColumns, Unused)
{
}
NW_HOOK_CTOR_ARGS(0x100ad6b0, NCSiegeInfoListCtrl, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x100ad660
NW_HOOK_DTOR(0x100ad660, NCSiegeInfoListCtrl)

// 0x100ab3a0 (vslot 24)
INT NCSiegeInfoListCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCListCtrl::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x100ab3a0, NCSiegeInfoListCtrl, WindowProc)

// 0x100ab940 (vslot 50) - NCListCtrl's paint (with its guard name), but the first column
// starts with the clan's crest and, on the attacking side, the second one with the
// alliance's; the cell icons are not drawn
INT NCSiegeInfoListCtrl::OnPaint(NCGDevice* Device)
{
    guard(NCListCtrl::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    INT Line = 0;
    for (INT i = TopRow; i < TopRow + VisibleRows && i <= NumRows && i < Rows.Num(); i++, Line++)
    {
        Device->SetClip(ScreenX, ScreenY + HeaderHeight, Width, Height);
        if (i == Selected)
            Device->DrawRect(0, (LineGap + 12) * Line + TextY + HeaderHeight - 1, Width - 18,
                             LineGap + 12, 0xFF646464);
        Device->ReleaseClip();
        INT X = TextX;
        NListRow* Row = Rows(i);
        for (INT c = 0; c < Columns.Num(); c++)
        {
            NCHeaderCtrl* Column = Columns(c);
            INT Y = (LineGap + 12) * Line + TextY + HeaderHeight;
            Device->SetClip(ScreenX + X, ScreenY + Y - 1, Column->ColumnWidth, 14);
            if (c == 0)
            {
                UTexture* Crest = GConsole->GetPledgeCrestTexture(Row->Param1);
                if (Crest)
                    Device->DrawTexture(0, Y, 16, 12, 0, 4, 16, 12, Crest);
                Device->DrawText(17, Y, Row->Cells[0].Color, Row->Cells[0].Text, 0, 0, 0,
                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            }
            else if (Row->Cells[0].Param2 == 0)
            {
                UTexture* Crest = GConsole->GetAllyCrestTexture(Row->Param1);
                if (Crest)
                    Device->DrawTexture(Row->Cells[c].IconW + X, Y, 8, 12, 0, 4, 8, 12, Crest);
                Device->DrawText(X + 9, Y, Row->Cells[c].Color, Row->Cells[c].Text, 0, 0, 0,
                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            }
            else
            {
                Device->DrawText(X, Y, Row->Cells[c].Color, Row->Cells[c].Text, 0, 0, 0,
                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            }
            X += Column->ColumnWidth;
            Device->ReleaseClip();
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100ab940, NCSiegeInfoListCtrl, OnPaint)

// 0x100ab530 (vslot 69) - the text BuildToolTip made, wrapped at 168 pixels
void NCSiegeInfoListCtrl::DrawRowToolTip(NCGDevice* Device, NListRow* Row, INT X, INT Y)
{
    if (Row)
        Device->DrawText(X, Y, 0xFFFFFFFF, Row->ToolTip, 168, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                         1);
}
NW_HOOK_METHOD(0x100ab530, NCSiegeInfoListCtrl, DrawRowToolTip)

// 0x100ab3e0 - the clan's leader, the alliance's leader (defenders: the alliance's name too)
// and when the clan applied ("Undecided" without a time; the time is tested unsigned)
__declspec(noinline) void NCSiegeInfoListCtrl::BuildToolTip(NListRow* Row)
{
    TCHAR Time[128];
    if (!Row)
        return;
    if ((DWORD)Row->Cells[0].Param1 > 0)
        GConsole->FormatDateTime(Row->Cells[0].Param1, Time);
    else
        appStrcpy(Time, *GL2GameData.SysString[STR_Undecided]);
    if (Row->Cells[0].Param2 == 0)
        appSprintf(Row->ToolTip, TEXT("%s : %s\n%s : %s\n%s\n: %s"),
                   *GL2GameData.SysString[STR_Leader], Row->Cells[0].Text2,
                   *GL2GameData.SysString[STR_AllianceLeader], Row->Cells[1].Text2,
                   *GL2GameData.SysString[STR_ApplyTime], Time);
    else
        appSprintf(Row->ToolTip, TEXT("%s : %s\n%s : %s\n%s : %s\n%s\n: %s"),
                   *GL2GameData.SysString[STR_Leader], Row->Cells[0].Text2,
                   *GL2GameData.SysString[STR_AllianceLeader], Row->Cells[1].Text2,
                   *GL2GameData.SysString[STR_AllianceName], Row->Text,
                   *GL2GameData.SysString[STR_ApplyTime], Time);
}
NW_HOOK_METHOD(0x100ab3e0, NCSiegeInfoListCtrl, BuildToolTip)

// 0x100ab870
__declspec(noinline) INT NCSiegeInfoListCtrl::GetSelectedPledgeID()
{
    if (Selected >= 0 && Selected < Rows.Num() && Rows(Selected))
        return Rows(Selected)->Param1;
    return 0;
}
NW_HOOK_METHOD(0x100ab870, NCSiegeInfoListCtrl, GetSelectedPledgeID)

// 0x100ab8a0 - read back from the status text of the selected row. An empty status goes to
// appStrcmp as NULL, as in the stock code.
__declspec(noinline) INT NCSiegeInfoListCtrl::GetSelectedStatus()
{
    if (Selected >= 0 && Selected < Rows.Num())
    {
        NListRow* Row = Rows(Selected);
        if (Row)
        {
            if (!appStrcmp(*GL2GameData.SysString[STR_Approved], CellText(Row->Cells[1])))
                return SIEGECLAN_Approved;
            if (!appStrcmp(*GL2GameData.SysString[STR_Waiting], CellText(Row->Cells[1])))
                return SIEGECLAN_Waiting;
        }
    }
    return -1;
}
NW_HOOK_METHOD(0x100ab8a0, NCSiegeInfoListCtrl, GetSelectedStatus)

NW_CHECK_VTABLE(NCSiegeInfoListCtrl, 0x100eea38, 70)

/*-----------------------------------------------------------------------------
    NCSiegePartInfoWnd.
-----------------------------------------------------------------------------*/

// 0x100ad6e0
NCSiegePartInfoWnd::NCSiegePartInfoWnd()
{
    List = NULL;
    Side = SIEGESIDE_Attacker;
    BackTex = FindTexture(TEXT("L2UI.SiegeWnd.siege_back2"));
}
NW_HOOK_CTOR(0x100ad6e0, NCSiegePartInfoWnd)

// 0x100ad790
NW_HOOK_DTOR(0x100ad790, NCSiegePartInfoWnd)

// 0x100ab570 (vslot 24)
INT NCSiegePartInfoWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_SIEGE_APPLY)
            return OnApply() != 0;
        if (wParam == NID_SIEGE_CANCELAPPLY)
            return OnCancelApply() != 0;
        if (wParam == NID_SIEGE_APPROVE)
            return OnApprove() != 0;
        if (wParam == NID_SIEGE_REFUSE)
            return OnRefuse() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100ab570, NCSiegePartInfoWnd, WindowProc)

// 0x100ad7f0 (vslot 35) - the list (256 rows, 13 shown, 2 columns) and the buttons: apply
// and cancel share one place (UpdateButtons shows one of them), approve sits left of refuse.
// NCWnd's OnCreate is not called.
INT NCSiegePartInfoWnd::OnCreate()
{
    List = new NCSiegeInfoListCtrl(256, 13, 2, 12);
    List->SetWindowRect(15, 15, 223, 192);
    CreateChild(List, NWS_VISIBLE);
    List->SetTextOffset(0, 3);

    ApplyButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                               TEXT("L2UI.Control.DefaultButton_click"), 0);
    ApplyButton->ID = NID_SIEGE_APPLY;
    ApplyButton->SetWindowRect(158, 204, 67, 19);
    CreateChild(ApplyButton, NWS_VISIBLE);
    ApplyButton->SetTextID(STR_Apply);

    CancelApplyButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                     TEXT("L2UI.Control.DefaultButton_click"), 0);
    CancelApplyButton->ID = NID_SIEGE_CANCELAPPLY;
    CancelApplyButton->SetWindowRect(158, 204, 67, 19);
    CreateChild(CancelApplyButton, NWS_VISIBLE);
    CancelApplyButton->SetTextID(STR_CancelApply);

    ApproveButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                 TEXT("L2UI.Control.DefaultButton_click"), 0);
    ApproveButton->ID = NID_SIEGE_APPROVE;
    ApproveButton->SetWindowRect(80, 204, 67, 19);
    CreateChild(ApproveButton, NWS_VISIBLE);
    ApproveButton->SetTextID(STR_Approve);

    RefuseButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    RefuseButton->ID = NID_SIEGE_REFUSE;
    RefuseButton->SetWindowRect(158, 204, 67, 19);
    CreateChild(RefuseButton, NWS_VISIBLE);
    RefuseButton->SetTextID(STR_Cancel);
    return 1;
}
NW_HOOK_METHOD(0x100ad7f0, NCSiegePartInfoWnd, OnCreate)

// 0x100acea0 (vslot 36) - the copies of the clans are freed here and in Reset
INT NCSiegePartInfoWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    for (INT i = 0; i < Clans.Num(); i++)
    {
        if (Clans(i))
            appFree(Clans(i));
    }
    Clans.Empty();
    DestroyChild(this, List);
    DestroyChild(this, ApplyButton);
    DestroyChild(this, CancelApplyButton);
    DestroyChild(this, ApproveButton);
    DestroyChild(this, RefuseButton);
    return 1;
}
NW_HOOK_METHOD(0x100acea0, NCSiegePartInfoWnd, OnDestroy)

// 0x100abc70 (vslot 50) - the background from under the tab buttons (18 px up), then the
// count: clans attacking, or defending clans approved / waiting. The origin goes on before
// the clip; the background texture is not checked.
INT NCSiegePartInfoWnd::OnPaint(NCGDevice* Device)
{
    TCHAR Text[128];
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY - 18, Width, Height);
    Device->DrawTexture(0, -18, 240, 246, 0, 0, 240, 246, BackTex);
    if (Side == SIEGESIDE_Attacker)
        appSprintf(Text, TEXT("%s : %d"), *GL2GameData.SysString[STR_AttackingClans],
                   Clans.Num());
    else
        appSprintf(Text, TEXT("%s/%s : %d/%d"), *GL2GameData.SysString[STR_DefendingClans],
                   *GL2GameData.SysString[STR_WaitingClans], NumApproved, NumWaiting);
    Device->DrawText(14, 2, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100abc70, NCSiegePartInfoWnd, OnPaint)

// 0x100abf20 (vslot 62) - registration on this side of the castle, asked first
INT NCSiegePartInfoWnd::OnApply()
{
    NDialogInfo Info;
    if (Side == SIEGESIDE_Defender)
    {
        Info.ID = DIALOG_SIEGE_DEFEND;
        Info.SetSystemMessage(MSG_ApplyDefense);
    }
    else
    {
        Info.ID = DIALOG_SIEGE_ATTACK;
        Info.SetSystemMessage(MSG_ApplyAttack);
    }
    Info.bMainBox = 1;
    Info.bInput = 0;
    Info.SetButtons(GConsole->GetCastleName(CastleID), NULL);
    Info.SetParams(CastleID, -1);
    GConsole->ShowDialog(&Info);
    return 1;
}
NW_HOOK_METHOD(0x100abf20, NCSiegePartInfoWnd, OnApply)

// 0x100abfd0 (vslot 63)
INT NCSiegePartInfoWnd::OnCancelApply()
{
    NDialogInfo Info;
    Info.ID = (Side == SIEGESIDE_Defender) ? DIALOG_SIEGE_CANCEL_DEFEND : DIALOG_SIEGE_CANCEL_ATTACK;
    Info.bMainBox = 1;
    Info.bInput = 0;
    Info.SetSystemMessage(MSG_CancelApplication);
    Info.SetButtons(GConsole->GetCastleName(CastleID), NULL);
    Info.SetParams(CastleID, -1);
    GConsole->ShowDialog(&Info);
    return 1;
}
NW_HOOK_METHOD(0x100abfd0, NCSiegePartInfoWnd, OnCancelApply)

// 0x100ac070 (vslot 64) - the owner approves a clan waiting on the defending side
INT NCSiegePartInfoWnd::OnApprove()
{
    if (List->GetSelectedPledgeID() > 0 && List->GetSelectedStatus() == SIEGECLAN_Waiting)
    {
        PledgeInfo* Pledge = GConsole->GetPledgeInfo(List->GetSelectedPledgeID());
        if (Pledge)
        {
            NDialogInfo Info;
            Info.ID = DIALOG_SIEGE_APPROVE;
            Info.bMainBox = 1;
            Info.bInput = 0;
            Info.SetSystemMessage(MSG_ApproveDefender);
            if (Pledge->Name && Pledge->Name[0])
                Info.SetButtons(Pledge->Name, NULL);
            Info.SetParams(CastleID, List->GetSelectedPledgeID());
            GConsole->ShowDialog(&Info);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100ac070, NCSiegePartInfoWnd, OnApprove)

// 0x100ac150 (vslot 65) - or refuses one, waiting or already approved
INT NCSiegePartInfoWnd::OnRefuse()
{
    if (List->GetSelectedPledgeID() > 0 &&
        (List->GetSelectedStatus() == SIEGECLAN_Waiting ||
         List->GetSelectedStatus() == SIEGECLAN_Approved))
    {
        PledgeInfo* Pledge = GConsole->GetPledgeInfo(List->GetSelectedPledgeID());
        if (Pledge)
        {
            NDialogInfo Info;
            Info.ID = DIALOG_SIEGE_REFUSE;
            Info.bMainBox = 1;
            Info.bInput = 0;
            Info.SetSystemMessage(MSG_RefuseDefender);
            if (Pledge->Name && Pledge->Name[0])
                Info.SetButtons(Pledge->Name, NULL);
            Info.SetParams(CastleID, List->GetSelectedPledgeID());
            GConsole->ShowDialog(&Info);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100ac150, NCSiegePartInfoWnd, OnRefuse)

// 0x100abdc0 - and the list's columns: the clan's name, then the alliance's name (attackers)
// or the status (defenders). The second column sorts as numbers on both sides.
__declspec(noinline) void NCSiegePartInfoWnd::SetSide(INT InSide)
{
    Side = InSide;

    NListColumnInfo NameColumn;
    NameColumn.Width = 109;
    NameColumn.Height = 15;
    NameColumn.SetTitleID(STR_ClanName);
    NameColumn.SetNormalTex(TEXT("L2UI.Control.listbutton"));
    NameColumn.SetPushedTex(TEXT("L2UI.Control.listbutton_click"));
    List->AddColumn(0, &NameColumn);

    NListColumnInfo SecondColumn;
    SecondColumn.Width = 100;
    SecondColumn.Height = 15;
    SecondColumn.bNumeric = 1;
    if (Side == SIEGESIDE_Attacker)
    {
        SecondColumn.SetTitleID(STR_AllianceName);
        SecondColumn.SetNormalTex(TEXT("L2UI.Control.listbutton"));
        SecondColumn.SetPushedTex(TEXT("L2UI.Control.listbutton_click"));
    }
    else
    {
        SecondColumn.SetTitleID(STR_Status);
        SecondColumn.SetNormalTex(TEXT("L2UI.Control.listbutton"));
        SecondColumn.SetPushedTex(TEXT("L2UI.Control.listbutton_click"));
    }
    List->AddColumn(0, &SecondColumn);
}
NW_HOOK_METHOD(0x100abdc0, NCSiegePartInfoWnd, SetSide)

// 0x100ac240 - apply or cancel by whether the player's clan is listed; the owner gets
// neither, and approves or refuses the defenders instead
__declspec(noinline) void NCSiegePartInfoWnd::UpdateButtons()
{
    bListed = 1;
    if (Side == SIEGESIDE_Attacker)
    {
        if (bRegistered)
        {
            ApplyButton->HideWindow();
            CancelApplyButton->ShowWindow();
        }
        else
        {
            ApplyButton->ShowWindow();
            CancelApplyButton->HideWindow();
        }
        if (MainOf(this)->Castle.bIsOwner)
        {
            ApplyButton->HideWindow();
            CancelApplyButton->HideWindow();
        }
        ApproveButton->HideWindow();
        RefuseButton->HideWindow();
    }
    else
    {
        if (bRegistered)
        {
            ApplyButton->HideWindow();
            CancelApplyButton->ShowWindow();
        }
        else
        {
            ApplyButton->ShowWindow();
            CancelApplyButton->HideWindow();
        }
        if (MainOf(this)->Castle.bIsOwner)
        {
            ApplyButton->HideWindow();
            CancelApplyButton->HideWindow();
            ApproveButton->ShowWindow();
            RefuseButton->ShowWindow();
        }
        else
        {
            ApproveButton->HideWindow();
            RefuseButton->HideWindow();
        }
    }
}
NW_HOOK_METHOD(0x100ac240, NCSiegePartInfoWnd, UpdateButtons)

// 0x100acfe0 - the window closing or showing another castle
__declspec(noinline) void NCSiegePartInfoWnd::Reset()
{
    for (INT i = 0; i < Clans.Num(); i++)
    {
        if (Clans(i))
            appFree(Clans(i));
    }
    Clans.Empty();
    List->ClearRows();
    bRegistered = 0;
    NumApproved = 0;
    NumWaiting = 0;
    bListed = 0;
    CastleID = 0;
}
NW_HOOK_METHOD(0x100acfe0, NCSiegePartInfoWnd, Reset)

// 0x100ad060 - the console, before a new list: the copies of the clans are dropped without
// being freed (they leak, as in the stock code)
void NCSiegePartInfoWnd::ClearList()
{
    Clans.Empty();
    List->ClearRows();
    bRegistered = 0;
    NumApproved = 0;
    NumWaiting = 0;
}
NW_HOOK_METHOD(0x100ad060, NCSiegePartInfoWnd, ClearList)

// 0x100ad0a0 - the clan's row (its data in the header) and a copy of the record. On the
// defending side the owner counts as waiting, and "refused" is a Korean literal, not a
// sysstring line. The player's own clan turns apply into cancel.
__declspec(noinline) void NCSiegePartInfoWnd::AddClan(NSiegeClanInfo* Info)
{
    NListRow* Row = new NListRow(2);
    if (Side == SIEGESIDE_Attacker)
    {
        Row->Cells[0].SetText(Info->ClanName);
        Row->Cells[0].SetParams(Info->ApplyTime, 0);
        Row->Cells[1].SetText(Info->AllianceName);
    }
    else
    {
        Row->Cells[0].SetText(Info->ClanName);
        Row->Cells[0].SetParams(Info->ApplyTime, 1);
        switch (Info->Type)
        {
        case SIEGECLAN_Owner:
            Row->Cells[1].SetText(*GL2GameData.SysString[STR_Lord]);
            NumWaiting++;
            break;
        case SIEGECLAN_Waiting:
            Row->Cells[1].SetText(*GL2GameData.SysString[STR_Waiting]);
            NumWaiting++;
            break;
        case SIEGECLAN_Approved:
            Row->Cells[1].SetText(*GL2GameData.SysString[STR_Approved]);
            NumApproved++;
            break;
        case SIEGECLAN_Refused:
            Row->Cells[1].SetText(TEXT("\xAC70\xBD80"));
            break;
        }
    }
    Row->Cells[0].SetText2(Info->LeaderName);
    Row->Cells[1].SetText2(Info->AllianceLeader);
    Row->SetText(Info->AllianceName);
    Row->SetParams(Info->PledgeID, Info->CrestID);
    List->AddRow(Row);
    List->BuildToolTip(Row);
    Clans.AddItem(new NSiegeClanInfo(*Info));
    if (Info->PledgeID == GConsole->GetMyUser()->PledgeID)
        bRegistered = 1;
}
NW_HOOK_METHOD(0x100ad0a0, NCSiegePartInfoWnd, AddClan)

NW_CHECK_VTABLE(NCSiegePartInfoWnd, 0x100eeb88, 66)

/*-----------------------------------------------------------------------------
    NCSiegeDateInfoWnd.
-----------------------------------------------------------------------------*/

// 0x100ad2d0
NCSiegeDateInfoWnd::NCSiegeDateInfoWnd()
{
    BackTex = FindTexture(TEXT("L2UI.SiegeWnd.siege_back3"));
}
NW_HOOK_CTOR(0x100ad2d0, NCSiegeDateInfoWnd)

// 0x100ad370
NW_HOOK_DTOR(0x100ad370, NCSiegeDateInfoWnd)

// 0x100ab640 (vslot 24)
INT NCSiegeDateInfoWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_SIEGE_TIMECOMBO)
            return OnSelectTime((INT)lParam) != 0;
    }
    else if (Msg == (UINT)NWM_SIEGE_DATE_OK)
        return OnConfirmTime(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100ab640, NCSiegeDateInfoWnd, WindowProc)

// 0x100ac380 (vslot 35) - the combo box of the dates, next to "Siege Date" (NCWnd's
// OnCreate is not called)
INT NCSiegeDateInfoWnd::OnCreate()
{
    TimeCombo = new NCComboBox;
    TimeCombo->ID = NID_SIEGE_TIMECOMBO;
    TimeCombo->SetWindowRect(80, 60, 126, 15);
    CreateChild(TimeCombo, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100ac380, NCSiegeDateInfoWnd, OnCreate)

// 0x100ac430 (vslot 36)
INT NCSiegeDateInfoWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    DestroyChild(this, TimeCombo);
    return 1;
}
NW_HOOK_METHOD(0x100ac430, NCSiegeDateInfoWnd, OnDestroy)

// 0x100ac470 (vslot 50) - the current time and the siege's; without a date, "Undecided" for
// all but the owner, who has the combo box there. The clip goes on before the origin; the
// background texture is not checked.
INT NCSiegeDateInfoWnd::OnPaint(NCGDevice* Device)
{
    TCHAR Now[128];
    TCHAR When[128];
    Device->SetClip(ScreenX, ScreenY - 18, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, -18, 240, 246, 0, 0, 240, 246, BackTex);
    GConsole->FormatDateTime(CurrentTime, Now);
    Device->DrawText(10, 30, 0xFFFFFFFF, *GL2GameData.SysString[STR_CurrentTime], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(80, 30, 0xFFFFFFFF, Now, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(10, 60, 0xFFFFFFFF, *GL2GameData.SysString[STR_SiegeDate], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (SiegeTime > 0)
    {
        GConsole->FormatDateTime(SiegeTime, When);
        Device->DrawText(80, 60, 0xFFFFFFFF, When, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else if (SiegeTime == 0 && !MainOf(this)->Castle.bIsOwner)
    {
        Device->DrawText(80, 60, 0xFFFFFFFF, *GL2GameData.SysString[STR_Undecided], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100ac470, NCSiegeDateInfoWnd, OnPaint)

// 0x100ac6a0 (vslot 62) - a date picked (not the automatic setting) is confirmed first; OK
// comes back as NWM_SIEGE_DATE_OK
INT NCSiegeDateInfoWnd::OnSelectTime(INT Index)
{
    if (Index > 0)
    {
        SelectedIndex = Index;
        NDialogInfo Info;
        Info.ID = DIALOG_SIEGE_DATE;
        Info.Owner = Entry;
        Info.bMainBox = 1;
        Info.bInput = 0;
        Info.SetSystemMessage(MSG_ConfirmSiegeTime);
        Info.SetButtons(TimeCombo->GetItemText(Index), NULL);
        GConsole->ShowDialog(&Info);
    }
    return 1;
}
NW_HOOK_METHOD(0x100ac6a0, NCSiegeDateInfoWnd, OnSelectTime)

// 0x100ac730 (vslot 63) - the castle and the date chosen (0: the automatic setting)
INT NCSiegeDateInfoWnd::OnConfirmTime(WPARAM wParam, LPARAM lParam)
{
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)CastleID);
    Params.PushBack((void*)(INT_PTR)(SelectedIndex > 0 ? Times(SelectedIndex - 1) : 0));
    GConsole->RequestSetSiegeTime(Params);
    return 1;
}
NW_HOOK_METHOD(0x100ac730, NCSiegeDateInfoWnd, OnConfirmTime)

// 0x100ac630 - the combo box back to its one "Automatic setting" item
__declspec(noinline) void NCSiegeDateInfoWnd::Reset()
{
    Times.Empty();
    TimeCombo->ClearItems();
    TimeCombo->AddItem(*GL2GameData.SysString[STR_AutoSetting]);
    CastleID = 0;
    SelectedIndex = 0;
    CurrentTime = 0;
    SiegeTime = 0;
    TimeCombo->SetSelected(0);
}
NW_HOOK_METHOD(0x100ac630, NCSiegeDateInfoWnd, Reset)

// 0x100ac7d0 - the combo box shows to the owner only, while no date is set
__declspec(noinline) void NCSiegeDateInfoWnd::SetTimes(INT InCurrentTime, INT InSiegeTime)
{
    CurrentTime = InCurrentTime;
    SiegeTime = InSiegeTime;
    if (InSiegeTime == 0)
    {
        if (MainOf(this)->Castle.bIsOwner)
            TimeCombo->ShowWindow();
        else
            TimeCombo->HideWindow();
    }
    else
        TimeCombo->HideWindow();
}
NW_HOOK_METHOD(0x100ac7d0, NCSiegeDateInfoWnd, SetTimes)

// 0x100ad3d0 - one more date the owner can choose (the console, from the server's offer)
void NCSiegeDateInfoWnd::AddTime(INT Time)
{
    TCHAR Text[128];
    GConsole->FormatDateTime(Time, Text);
    TimeCombo->AddItem(Text);
    Times.AddItem(Time);
}
NW_HOOK_METHOD(0x100ad3d0, NCSiegeDateInfoWnd, AddTime)

NW_CHECK_VTABLE(NCSiegeDateInfoWnd, 0x100ee938, 64)

/*-----------------------------------------------------------------------------
    NCSiegeInfoMainWnd.
-----------------------------------------------------------------------------*/

// 0x100ac830
NCSiegeInfoMainWnd::NCSiegeInfoMainWnd()
{
    BackTex = FindTexture(TEXT("L2UI.SiegeWnd.Siege_Back1"));
}
NW_HOOK_CTOR(0x100ac830, NCSiegeInfoMainWnd)

// 0x100ac8f0
NW_HOOK_DTOR(0x100ac8f0, NCSiegeInfoMainWnd)

// 0x100ab6e0 (vslot 24)
INT NCSiegeInfoMainWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_SIEGE_TABS)
        return OnTab((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100ab6e0, NCSiegeInfoMainWnd, WindowProc)

// 0x100adb80 (vslot 35) - title bar, the tabs' windows (the sides hidden) and the tab
// buttons: date, attackers, defenders. NCWnd's OnCreate is not called.
INT NCSiegeInfoMainWnd::OnCreate()
{
    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_SIEGE_FRAME;
    FrameCtrl->ID = NID_SIEGE_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    DateInfo = new NCSiegeDateInfoWnd;
    DateInfo->SetWindowRect(0, 138, 240, 246);
    CreateChild(DateInfo, NWS_VISIBLE);

    AttackerInfo = new NCSiegePartInfoWnd;
    AttackerInfo->SetWindowRect(0, 138, 240, 246);
    CreateChild(AttackerInfo, NWS_VISIBLE);
    AttackerInfo->SetSide(SIEGESIDE_Attacker);
    AttackerInfo->HideWindow();

    DefenderInfo = new NCSiegePartInfoWnd;
    DefenderInfo->SetWindowRect(0, 138, 240, 246);
    CreateChild(DefenderInfo, NWS_VISIBLE);
    DefenderInfo->SetSide(SIEGESIDE_Defender);
    DefenderInfo->HideWindow();

    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_SIEGE_TABS;
    TabCtrl->SetWindowRect(15, 122, 207, 15);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(STR_SiegeDate, DateInfo, 69, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);
    TabCtrl->AddTab(STR_SiegeInfo, AttackerInfo, 69, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);
    TabCtrl->AddTab(STR_DefenseInfo, DefenderInfo, 69, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);
    return 1;
}
NW_HOOK_METHOD(0x100adb80, NCSiegeInfoMainWnd, OnCreate)

// 0x100ac940 (vslot 36)
INT NCSiegeInfoMainWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    DestroyChild(this, TabCtrl);
    DestroyChild(this, DateInfo);
    DestroyChild(this, AttackerInfo);
    DestroyChild(this, DefenderInfo);
    return 1;
}
NW_HOOK_METHOD(0x100ac940, NCSiegeInfoMainWnd, OnDestroy)

// 0x100ad530 (vslot 42) - opens emptied on the date tab (the console's SetCastleInfo
// follows)
void NCSiegeInfoMainWnd::ShowWindow()
{
    GConsole->Engine->OnInterfacePlaySound(SOUND_WindowOpen);
    NCWnd::ShowWindow();
    Reset();
    TabCtrl->SetTab(0);
}
NW_HOOK_METHOD(0x100ad530, NCSiegeInfoMainWnd, ShowWindow)

// 0x100ad520 (vslot 43)
void NCSiegeInfoMainWnd::HideWindow()
{
    NCWnd::HideWindow();
    Reset();
}
NW_HOOK_METHOD(0x100ad520, NCSiegeInfoMainWnd, HideWindow)

// 0x100aca10 (vslot 50) - the castle's name centered on the background's title, then lord,
// clan and alliance (the crests before the names), or "No castle owner". The clip goes on
// before the origin; the background texture is not checked. The alliance crest is looked up
// by the owner clan's id too.
INT NCSiegeInfoMainWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 21, 240, 99, 0, 0, 240, 99, BackTex);
    const TCHAR* Name = GConsole->GetCastleName(Castle.CastleID);
    if (Name)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Name, 0, 0, 0, 0, 0);
        Device->DrawText(120 - Size.cx / 2, 27, SIEGE_TITLE_COLOR, Name, 0, 0, 0, L2FONT_Normal, 0,
                         0, 0, 0, 0, 0);
    }
    Device->DrawText(20, 60, SIEGE_LABEL_COLOR, *GL2GameData.SysString[STR_Lord], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(20, 75, SIEGE_LABEL_COLOR, *GL2GameData.SysString[STR_Clan], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(20, 90, SIEGE_LABEL_COLOR, *GL2GameData.SysString[STR_Alliance], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (Castle.LordName[0])
    {
        Device->DrawText(80, 60, 0xFFFFFFFF, Castle.LordName, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                         0, 0);
        if (Device->DrawPledgeCrest(80, 75, Castle.PledgeID))
            Device->DrawText(100, 75, 0xFFFFFFFF, Castle.ClanName, 0, 0, 0, L2FONT_Normal, 0, 0,
                             0, 0, 0, 0);
        else
            Device->DrawText(80, 75, 0xFFFFFFFF, Castle.ClanName, 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                             0, 0, 0);
        if (Castle.AllianceName[0])
        {
            if (Device->DrawAllyCrest(88, 90, Castle.PledgeID))
                Device->DrawText(100, 90, 0xFFFFFFFF, Castle.AllianceName, 0, 0, 0,
                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            else
                Device->DrawText(80, 90, 0xFFFFFFFF, Castle.AllianceName, 0, 0, 0, L2FONT_Normal,
                                 0, 0, 0, 0, 0, 0);
        }
        else
        {
            Device->DrawText(80, 90, 0xFFFFFFFF, *GL2GameData.SysString[STR_NoAlliance], 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
    }
    else
    {
        Device->DrawText(80, 60, 0xFFFFFFFF, *GL2GameData.SysString[STR_NoCastleOwner], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100aca10, NCSiegeInfoMainWnd, OnPaint)

// 0x100accd0 (vslot 62) - a side's list is asked for once (until Reset)
INT NCSiegeInfoMainWnd::OnTab(INT Tab)
{
    switch (Tab)
    {
    case 0:
        DateInfo->ShowWindow();
        AttackerInfo->HideWindow();
        DefenderInfo->HideWindow();
        DateInfo->SetTimes(Castle.CurrentTime, Castle.SiegeTime);
        break;
    case 1:
        if (!AttackerInfo->bListed)
        {
            L2ParamStack Params(10);
            Params.PushBack((void*)(INT_PTR)Castle.CastleID);
            GConsole->RequestSiegeAttackerList(Params);
        }
        DateInfo->HideWindow();
        AttackerInfo->ShowWindow();
        DefenderInfo->HideWindow();
        AttackerInfo->UpdateButtons();
        break;
    case 2:
        if (!DefenderInfo->bListed)
        {
            L2ParamStack Params(10);
            Params.PushBack((void*)(INT_PTR)Castle.CastleID);
            GConsole->RequestSiegeDefenderList(Params);
        }
        DateInfo->HideWindow();
        AttackerInfo->HideWindow();
        DefenderInfo->ShowWindow();
        DefenderInfo->UpdateButtons();
        break;
    }
    return 1;
}
NW_HOOK_METHOD(0x100accd0, NCSiegeInfoMainWnd, OnTab)

// 0x100ad430 - the castle's data and the three tabs emptied
__declspec(noinline) void NCSiegeInfoMainWnd::Reset()
{
    Castle.Clear();
    AttackerInfo->Reset();
    DefenderInfo->Reset();
    DateInfo->Reset();
}
NW_HOOK_METHOD(0x100ad430, NCSiegeInfoMainWnd, Reset)

// 0x100ad490 - the console, with the server's siege info (after ShowWindow)
void NCSiegeInfoMainWnd::SetCastleInfo(NSiegeCastleInfo* Info)
{
    Reset();
    Castle = *Info;
    DateInfo->CastleID = Info->CastleID;
    AttackerInfo->CastleID = Info->CastleID;
    DefenderInfo->CastleID = Info->CastleID;
    DateInfo->SetTimes(Info->CurrentTime, Info->SiegeTime);
}
NW_HOOK_METHOD(0x100ad490, NCSiegeInfoMainWnd, SetCastleInfo)

// 0x100ad500
void NCSiegeInfoMainWnd::AddAttacker(NSiegeClanInfo* Info)
{
    AttackerInfo->AddClan(Info);
}
NW_HOOK_METHOD(0x100ad500, NCSiegeInfoMainWnd, AddAttacker)

// 0x100ad510
void NCSiegeInfoMainWnd::AddDefender(NSiegeClanInfo* Info)
{
    DefenderInfo->AddClan(Info);
}
NW_HOOK_METHOD(0x100ad510, NCSiegeInfoMainWnd, AddDefender)

NW_CHECK_VTABLE(NCSiegeInfoMainWnd, 0x100ee800, 63)
