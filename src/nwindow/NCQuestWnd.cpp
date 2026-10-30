/*=============================================================================
    NCQuestWnd.cpp - the quest window. Stock: 0x100a0370 .. 0x100a04e8, 0x100a0fd0 ..
    0x100a13b6, 0x100a1580 .. 0x100a1656 and OnCreate (0x100a1890), plus the destructor
    (0x1005c6b0, among NCGMQuestWnd's code).
=============================================================================*/
#include "nwindow/NCQuestWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCListBox.h"
#include "nwindow/NCQuestInfoWnd.h"
#include "nwindow/NCQuestItemWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"

// ids (names ours)
enum
{
    NID_QUEST_TAB = 0xF6,
    NID_QUEST_CANCEL = 0xF7,
    NID_QUEST_LIST = 0x1A03,
};
enum { DIALOG_CANCEL_QUEST = 30 };      // NDialogInfo::ID (name ours)
enum { TUTORIAL_QUEST_ITEMS = 0x2000 }; // the item page opened (name ours)

// sysstring
enum
{
    STR_AcceptedQuest = 324,
    STR_Description = 325,
    STR_Item = 326,
    STR_Cancel = 327,
};

// 0x100a0fd0 - the textures and the Cancel button are left as allocated
NCQuestWnd::NCQuestWnd()
{
    QuestList = NULL;
    InfoWnd = NULL;
    ItemWnd = NULL;
    TabCtrl = NULL;
    CurTab = 0;
    DropW = 27;
    DropH = 27;
    DropX = 160;
    DropY = 420;
}
NW_HOOK_CTOR(0x100a0fd0, NCQuestWnd)

// 0x1005c6b0
NW_HOOK_DTOR(0x1005c6b0, NCQuestWnd)

// 0x100a0370 (vslot 24)
INT NCQuestWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_DIALOG_RESULT)
        return OnDialogResult(wParam, lParam) != 0;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_QUEST_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_QUEST_LIST)
            return OnSelectQuest((INT)lParam) != 0;
        if (wParam == NID_QUEST_TAB)
            return OnTab((INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100a0370, NCQuestWnd, WindowProc)

// 0x100a0440 (vslot 66) - Cancel only on the description page; the item page counts as a
// tutorial event
INT NCQuestWnd::OnTab(INT Page)
{
    if (Page == 0)
    {
        CurTab = 0;
        CancelButton->ShowWindow();
        return 1;
    }
    if (Page == 1)
    {
        CurTab = 1;
        CancelButton->HideWindow();
        GConsole->SendTutorialEvent(TUTORIAL_QUEST_ITEMS);
    }
    return 1;
}
NW_HOOK_METHOD(0x100a0440, NCQuestWnd, OnTab)

// 0x100a04a0 (vslot 62) - edges included
UBOOL NCQuestWnd::IsInDropZone(INT X, INT Y)
{
    X -= ScreenX + DropX;
    Y -= ScreenY + DropY;
    return X >= 0 && X <= DropW && Y >= 0 && Y <= DropH;
}
NW_HOOK_METHOD(0x100a04a0, NCQuestWnd, IsInDropZone)

// 0x100a1040 (vslot 36)
INT NCQuestWnd::OnDestroy()
{
    if (QuestList)
    {
        RemoveChildWindow(QuestList->Entry);
        delete QuestList;
        QuestList = NULL;
    }
    if (InfoWnd)
    {
        RemoveChildWindow(InfoWnd->Entry);
        delete InfoWnd;
        InfoWnd = NULL;
    }
    if (ItemWnd)
    {
        RemoveChildWindow(ItemWnd->Entry);
        delete ItemWnd;
        ItemWnd = NULL;
    }
    if (TabCtrl)
    {
        RemoveChildWindow(TabCtrl->Entry);
        delete TabCtrl;
        TabCtrl = NULL;
    }
    if (CancelButton)
    {
        RemoveChildWindow(CancelButton->Entry);
        delete CancelButton;
        CancelButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100a1040, NCQuestWnd, OnDestroy)

// 0x100a1130 (vslot 50) - BeginPaint comes before the clip here; the title is centered in
// the 105 pixels from x 22
INT NCQuestWnd::OnPaint(NCGDevice* Device)
{
    guard(NCQuestWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (TopTex)
        Device->DrawTexture(0, 0, 240, 115, 0, 0, 240, 115, TopTex);
    if (CurTab == 0 && InfoBackTex)
        Device->DrawTexture(0, 115, 240, 185, 0, 0, 240, 185, InfoBackTex);
    else if (CurTab != 0 && ItemBackTex)
        Device->DrawTexture(0, 115, 240, 185, 0, 0, 240, 185, ItemBackTex);
    if (ItemPageIconTex && CurTab)
        Device->DrawTexture(220, 127, 14, 14, 0, 0, 32, 32, ItemPageIconTex);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_AcceptedQuest], 0, 0, 0, 0, 0);
    Device->DrawText((105 - Size.cx) / 2 + 22, 3, 0xFFFFFFFF, *GL2GameData.SysString[STR_AcceptedQuest], 0,
                     0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100a1130, NCQuestWnd, OnPaint)

// 0x100a12e0 (vslot 63) - whatever the answer: the Cancel question only reports OK
INT NCQuestWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    GConsole->RequestDestroyQuest(QuestList->GetItemParam(QuestList->Selected));
    return 1;
}
NW_HOOK_METHOD(0x100a12e0, NCQuestWnd, OnDialogResult)

// 0x100a1310 (vslot 64) - asks before giving up the selected quest (systemmsg 182)
INT NCQuestWnd::OnCancel()
{
    if (QuestList->Selected >= 0 && QuestList->Selected < QuestList->NumLines
        && QuestList->GetItemParam(QuestList->Selected) >= 0)
    {
        NDialogInfo Info;
        Info.ID = DIALOG_CANCEL_QUEST;
        Info.bMainBox = 1;
        Info.bInput = 0;
        Info.Owner = Entry;
        Info.SetSystemMessage(182);  // "Do you want to quit the current quest?"
        GConsole->ShowDialog(&Info);
    }
    return 1;
}
NW_HOOK_METHOD(0x100a1310, NCQuestWnd, OnCancel)

// 0x100a13a0 (vslot 65) - a line of the list clicked
INT NCQuestWnd::OnSelectQuest(INT Index)
{
    InfoWnd->ShowQuest(Index);
    return 1;
}
NW_HOOK_METHOD(0x100a13a0, NCQuestWnd, OnSelectQuest)

// 0x100a1580
void NCQuestWnd::Clear()
{
    QuestList->Clear();
    InfoWnd->ClearQuests();
    ItemWnd->Clear();
}
NW_HOOK_METHOD(0x100a1580, NCQuestWnd, Clear)

// 0x100a15b0 (vslot 42) - emptied, then filled again by the server's answer
void NCQuestWnd::ShowWindow()
{
    Clear();
    GConsole->RequestQuestList();
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
    if (CurTab)
        CancelButton->HideWindow();
    else
        CancelButton->ShowWindow();
}
NW_HOOK_METHOD(0x100a15b0, NCQuestWnd, ShowWindow)

// 0x100a1600 - white in the list, the quest id as the line's parameter
INT NCQuestWnd::AddQuest(INT QuestID, const TCHAR* Name, INT Step)
{
    QuestList->AddString(Name, 0xFFFFFFFF, QuestID);
    InfoWnd->AddQuest(QuestID, Step);
    return 1;
}
NW_HOOK_METHOD(0x100a1600, NCQuestWnd, AddQuest)

// 0x100a1640
INT NCQuestWnd::AddQuestItem(NItemInfo* Info)
{
    ItemWnd->AddItem(Info);
    return 1;
}
NW_HOOK_METHOD(0x100a1640, NCQuestWnd, AddQuestItem)

// 0x100a1890 (vslot 35) - NCWnd::OnCreate is not called (no title bar); the list and both
// pages are created with the unnamed style bit 0x20
INT NCQuestWnd::OnCreate()
{
    TopTex = FindTexture(TEXT("L2UI.QuestWnd.questBack1"));
    InfoBackTex = FindTexture(TEXT("L2UI.QuestWnd.questBack2"));
    ItemBackTex = FindTexture(TEXT("L2UI.QuestWnd.questBack3"));
    ItemPageIconTex = FindTexture(TEXT("sek.cbui303"));

    QuestList = new NCListBox(165, 5, 16);
    QuestList->ID = NID_QUEST_LIST;
    QuestList->SetWindowRect(17, 31, 220, 74);
    CreateChild(QuestList, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    QuestList->TextX = 1;
    QuestList->TextY = 1;
    QuestList->LineGap = 1;

    InfoWnd = new NCQuestInfoWnd();
    InfoWnd->SetWindowRect(18, 150, 220, 122);
    CreateChild(InfoWnd, NWS_VISIBLE | 0x20);

    ItemWnd = new NCQuestItemWnd(6, 4, 80, 32);
    ItemWnd->SetWindowRect(16, 144, 220, 139);
    CreateChild(ItemWnd, NWS_VISIBLE | 0x20);

    TabCtrl = new NCTabControl();
    TabCtrl->ID = NID_QUEST_TAB;
    TabCtrl->SetWindowRect(15, 117, 138, 15);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(STR_Description, InfoWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"),
                    0, -1);
    TabCtrl->AddTab(STR_Item, ItemWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0,
                    -1);

    CancelButton = new NCButton(TEXT("L2UI.Control.Defaultbutton"), TEXT("L2UI.Control.Defaultbutton_click"), 0);
    CancelButton->ID = NID_QUEST_CANCEL;
    CancelButton->SetWindowRect(87, 277, 67, 19);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(STR_Cancel);
    return 1;
}
NW_HOOK_METHOD(0x100a1890, NCQuestWnd, OnCreate)

NW_CHECK_VTABLE(NCQuestWnd, 0x100e3d98, 67)
