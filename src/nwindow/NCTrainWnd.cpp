/*=============================================================================
    NCTrainWnd.cpp - the skill trainer window and its two pages. Stock: 0x100baae0 ..
    0x100bb4bf, 0x100bb4f0 .. 0x100bb53f, 0x100bb580 .. 0x100bc50a, 0x100bc540 .. 0x100bc54a,
    0x100bc7a0 .. 0x100bca96, 0x100bcc50 .. 0x100bcd19, 0x10089310 .. 0x1008937e and
    NTrainSkillInfo's 0x10047260 .. 0x100472de, 0x1004ddf0 .. 0x1004de67.
=============================================================================*/
#include "nwindow/NCTrainWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCTrainListBox.h"
#include "nwindow/NConsoleWnd.h"

#include <wchar.h>

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

// the player's SP at the bottom right of both pages (repeated in the stock OnPaints)
static FORCEINLINE void DrawPlayerSP(NCGDevice* Device)
{
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        TCHAR Text[1024];
        SIZE Size;
        appSprintf(Text, TEXT("%d"), Me->SP);
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
        Device->DrawText(220 - Size.cx, 290, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                         0, 0);
    }
}

/*-----------------------------------------------------------------------------
    NTrainSkillInfo
-----------------------------------------------------------------------------*/

// 0x10047260
NTrainSkillInfo::NTrainSkillInfo()
{
    Name[0] = 0;
    IconName[0] = 0;
    Unknown20C[0] = 0;
    Icon = NULL;
    for (INT i = 0; i < 4; i++)
        Items[i] = NULL;
    SP = 0;
    MpCost = 0;
    Range = 0;
    Unknown2B8 = 0;
    SkillID = -1;
    Level = -1;
    Unknown2A8 = -1;
    Unknown2BC = -1;
}
NW_HOOK_CTOR(0x10047260, NTrainSkillInfo)

// 0x1004ddf0 - the items are freed without a destructor (NItemEntry has none)
NTrainSkillInfo::~NTrainSkillInfo()
{
    for (INT i = 0; i < 4; i++)
    {
        if (Items[i])
        {
            appFree(Items[i]);
            Items[i] = NULL;
        }
    }
}
NW_HOOK_DTOR(0x1004ddf0, NTrainSkillInfo)

/*-----------------------------------------------------------------------------
    NCTrainListWnd
-----------------------------------------------------------------------------*/

// 0x100bb4a0
NCTrainListWnd::NCTrainListWnd()
{
    BackTex = NULL;
    ListBox = NULL;
}
NW_HOOK_CTOR(0x100bb4a0, NCTrainListWnd)

// 0x100bb4f0
NW_HOOK_DTOR(0x100bb4f0, NCTrainListWnd)

// 0x100baae0 (vslot 24)
INT NCTrainListWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_TRAIN_LISTBOX)
        return OnListCommand(lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100baae0, NCTrainListWnd, WindowProc)

// 0x100bcc50 (vslot 35) - 7 rows seen, 100 at most
INT NCTrainListWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.SkillTrain.TrainerBack1"));
    ListBox = new NCTrainListBox(100, 7, 16);
    ListBox->ID = NID_TRAIN_LISTBOX;
    ListBox->SetWindowRect(20, 24, 215, 248);
    CreateChild(ListBox, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100bcc50, NCTrainListWnd, OnCreate)

// stock body shared with NCPetActionWnd's (0x100bb540, hooked there); BackTex is left
INT NCTrainListWnd::OnDestroy()
{
    DestroyChild(this, ListBox);
    NCWnd::OnDestroy();
    return 1;
}

// 0x100bb580 (vslot 50)
INT NCTrainListWnd::OnPaint(NCGDevice* Device)
{
    guard(NCTrainListWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 0, 240, 344, 0, 0, 240, 344, BackTex);
    if (*GL2GameData.SysString[364])                                 // "Skill list"
        Device->DrawText(20, 5, 0xFFFFFFFF, *GL2GameData.SysString[364], 0, 0, 0, L2FONT_Normal, 0,
                         0, 0, 0, 0, 0);
    DrawPlayerSP(Device);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100bb580, NCTrainListWnd, OnPaint)

// 0x100bb710 (vslot 62) - the selected row's skill and level to the server; lParam (the row)
// is not used
INT NCTrainListWnd::OnListCommand(LPARAM lParam)
{
    NTrainEntry Entry;
    if (ListBox->GetSelectedEntry(&Entry))
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Entry.SkillID);
        Params.PushBack((void*)(INT_PTR)Entry.Level);
        GConsole->RequestAcquireSkillInfo(Params);
    }
    return 1;
}
NW_HOOK_METHOD(0x100bb710, NCTrainListWnd, OnListCommand)

// 0x100bab50 - also NCPetInventoryWnd's (its list at +0x98 too)
void NCTrainListWnd::Clear()
{
    ListBox->Clear();
}
NW_HOOK_METHOD(0x100bab50, NCTrainListWnd, Clear)

// 0x100bc7a0
void NCTrainListWnd::AddEntry(const NTrainEntry* Entry)
{
    ListBox->AddEntry(Entry);
}
NW_HOOK_METHOD(0x100bc7a0, NCTrainListWnd, AddEntry)

NW_CHECK_VTABLE(NCTrainListWnd, 0x100f0538, 63)

/*-----------------------------------------------------------------------------
    NCTrainInfoWnd
-----------------------------------------------------------------------------*/

// 0x100bc7b0
NCTrainInfoWnd::NCTrainInfoWnd()
{
    BackTex = NULL;
    LearnButton = NULL;
    ListButton = NULL;
}
NW_HOOK_CTOR(0x100bc7b0, NCTrainInfoWnd)

// 0x100bc840 - Info's destructor frees the required item entries
NW_HOOK_DTOR(0x100bc840, NCTrainInfoWnd)

// 0x100bab60 (vslot 24)
INT NCTrainInfoWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_TRAIN_LEARN)
            return OnLearn() != 0;
        if (wParam == NID_TRAIN_LIST)
            return OnList() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100bab60, NCTrainInfoWnd, WindowProc)

// a button of the page (repeated inline in the stock OnCreate)
static FORCEINLINE NCButton* CreatePageButton(NCWnd* Parent, INT ID, INT X, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("L2UI.control.DefaultButton"),
                                    TEXT("L2UI.control.DefaultButton_click"), 0);
    Button->ID = ID;
    Button->SetWindowRect(X, 313, 67, 19);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->SetTextID(TextID);
    return Button;
}

// 0x100bb810 (vslot 35) - the four required item entries, then "Learn" and "List"
INT NCTrainInfoWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.SkillTrain.TrainerBack3"));
    for (INT i = 0; i < 4; i++)
        Info.Items[i] = new NItemEntry;
    LearnButton = CreatePageButton(this, NID_TRAIN_LEARN, 85, 368);
    ListButton = CreatePageButton(this, NID_TRAIN_LIST, 153, 369);
    return 1;
}
NW_HOOK_METHOD(0x100bb810, NCTrainInfoWnd, OnCreate)

// 0x100bba20 (vslot 36) - NCWnd's comes first here; the item entries stay (Info's destructor)
INT NCTrainInfoWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    DestroyChild(this, LearnButton);
    DestroyChild(this, ListButton);
    return 1;
}
NW_HOOK_METHOD(0x100bba20, NCTrainInfoWnd, OnDestroy)

// "label : " (the label a sysstring, empty when missing) in grey, the colon in white; returns
// the X of the value, past "label : " (repeated inline for each such line of the stock code)
static FORCEINLINE INT DrawLabel(NCGDevice* Device, INT X, INT Y, TCHAR* Text, INT LabelID)
{
    SIZE Size;
    if (*GL2GameData.SysString[LabelID])
        appSprintf(Text, TEXT("%s"), *GL2GameData.SysString[LabelID]);
    else
        Text[0] = 0;
    Device->DrawText(X, Y, 0xFFA3A3A3, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT(" : "));
    Device->DrawText(X + Size.cx, Y, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (*GL2GameData.SysString[LabelID])
        appSprintf(Text, TEXT("%s : "), *GL2GameData.SysString[LabelID]);
    else
        Text[0] = 0;
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    return X + Size.cx;
}

// 0x100bba90 (vslot 50) - "Skill explanation": icon, name, "Lv" and the level (Unknown20C after
// it), "MP cost : n", "Range : n" unless negative, the description wrapped at 192 pixels;
// "Learning condition": "Required SP : n" and the required items shown ("name X count"); the
// player's SP. The stock line buffer is 32 characters, which a long item name ran past
// ("%s X %d"); a larger one draws the same.
INT NCTrainInfoWnd::OnPaint(NCGDevice* Device)
{
    guard(NCTrainInfoWnd::OnPaint);
    TCHAR Text[256];
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 0, 240, 344, 0, 0, 240, 344, BackTex);
    if (*GL2GameData.SysString[366])                                 // "Skill explanation"
        Device->DrawText(20, 4, 0xFFFFFFFF, *GL2GameData.SysString[366], 0, 0, 0, L2FONT_Normal, 0,
                         0, 0, 0, 0, 0);
    if (*GL2GameData.SysString[367])                                 // "Learning condition"
        Device->DrawText(20, 158, 0xFFFFFFFF, *GL2GameData.SysString[367], 0, 0, 0, L2FONT_Normal,
                         0, 0, 0, 0, 0, 0);
    if (Info.Icon)
        Device->DrawTexture(20, 22, 32, 32, 0, 0, 32, 32, Info.Icon);
    if (Info.Name && Info.Name[0])
        Device->DrawText(57, 24, 0xFFFFFFFF, Info.Name, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (*GL2GameData.SysString[88])                                  // "Lv"
        appSprintf(Text, TEXT("%s"), *GL2GameData.SysString[88]);
    else
        Text[0] = 0;
    Device->DrawText(57, 40, 0xFFA3A3A3, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    INT X = Size.cx + 59;
    appSprintf(Text, TEXT("%d"), Info.Level);
    Device->DrawText(X, 40, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    if (Info.Unknown20C && Info.Unknown20C[0])
        Device->DrawText(X + Size.cx + 10, 40, 0xFFB09B79, Info.Unknown20C, 0, 0, 0, L2FONT_Normal,
                         0, 0, 0, 0, 0, 0);
    INT ValueX = DrawLabel(Device, 20, 58, Text, 320);               // "MP cost"
    appSprintf(Text, TEXT("%d"), Info.MpCost);
    Device->DrawText(ValueX, 58, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (Info.Range >= 0)
    {
        ValueX = DrawLabel(Device, 20, 70, Text, 321);               // "Range"
        appSprintf(Text, TEXT("%d"), Info.Range);
        Device->DrawText(ValueX, 70, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    if (*Info.Desc)
        Device->DrawText(20, 92, 0xFFB2BECF, *Info.Desc, 192, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    ValueX = DrawLabel(Device, 20, 182, Text, 365);                  // "Required SP"
    appSprintf(Text, TEXT("%d"), Info.SP);
    Device->DrawText(ValueX, 182, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    for (INT i = 0; i < 4; i++)
    {
        if (Info.Items[i]->GetUnknownC34() > 0)
        {
            if (Info.Items[i]->GetIcon())
                Device->DrawTexture(20, (i + 6) * 34, 32, 32, 0, 0, 32, 32,
                                    Info.Items[i]->GetIcon());
            if (Info.Items[i]->GetName())
                appSprintf(Text, TEXT("%s X %d"), Info.Items[i]->GetName(), Info.Items[i]->Count);
            else
                Text[0] = 0;
            Device->DrawText(56, (i + 6) * 34, 0xFFFFFFFF, Text, 140, 0, 0, L2FONT_Normal, 0, 0,
                             0, 0, 0, 0);
        }
    }
    DrawPlayerSP(Device);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100bba90, NCTrainInfoWnd, OnPaint)

// 0x100bad80 (vslot 62) - "Learn"
INT NCTrainInfoWnd::OnLearn()
{
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)Info.SkillID);
    Params.PushBack((void*)(INT_PTR)Info.Level);
    GConsole->RequestAcquireSkill(Params);
    return 1;
}
NW_HOOK_METHOD(0x100bad80, NCTrainInfoWnd, OnLearn)

// 0x100bae10 (vslot 63) - "List": back to the list page (the buttons are hidden after the
// page)
INT NCTrainInfoWnd::OnList()
{
    HideWindow();
    GDesktop->TrainWnd->ListWnd->ShowWindow();
    LearnButton->HideWindow();
    ListButton->HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100bae10, NCTrainInfoWnd, OnList)

// 0x100babf0 - empty strings and a NULL icon are not copied, the description always is; the
// icon is looked up again by name (NWindow.BlackTexture when missing). Info.Items stays.
void NCTrainInfoWnd::SetInfo(NTrainSkillInfo* Src)
{
    if (Src->Name && Src->Name[0])
        wcscpy(Info.Name, Src->Name);
    const TCHAR* IconName = Src->IconName;
    if (IconName && IconName[0])
        wcscpy(Info.IconName, IconName);
    Info.Desc = Src->Desc;
    if (Src->Unknown20C && Src->Unknown20C[0])
        wcscpy(Info.Unknown20C, Src->Unknown20C);
    if (Src->Icon)
        Info.Icon = Src->Icon;
    Info.SkillID = Src->SkillID;
    Info.Level = Src->Level;
    Info.Unknown2A8 = Src->Unknown2A8;
    Info.SP = Src->SP;
    Info.MpCost = Src->MpCost;
    Info.Range = Src->Range;
    Info.Unknown2B8 = Src->Unknown2B8;
    Info.Unknown2BC = Src->Unknown2BC;
    if (IconName && IconName[0])
        Info.Icon = FindTexture(IconName);
    if (!Info.Icon)
        Info.Icon = FindTexture(TEXT("NWindow.BlackTexture"));
}
NW_HOOK_METHOD(0x100babf0, NCTrainInfoWnd, SetInfo)

// 0x100bad10 - before the server's answer; the description, Unknown2A8, MP cost and range are
// left, the item rows hidden (NItemEntry::UnknownC34 -1)
void NCTrainInfoWnd::Reset()
{
    Info.Name[0] = 0;
    Info.IconName[0] = 0;
    Info.Unknown20C[0] = 0;
    Info.Icon = NULL;
    Info.SP = 0;
    Info.SkillID = -1;
    Info.Level = -1;
    Info.Unknown2B8 = 0;
    Info.Unknown2BC = -1;
    for (INT i = 0; i < 4; i++)
        Info.Items[i]->SetUnknownC34(-1);
}
NW_HOOK_METHOD(0x100bad10, NCTrainInfoWnd, Reset)

// 0x100bc300 - into the first hidden row (none free: dropped). The item goes through a
// temporary NItemEntry that is never deleted (a leak per required item).
void NCTrainInfoWnd::AddRequiredItem(NItemInfo* Item)
{
    for (INT i = 0; i < 4; i++)
    {
        if (Info.Items[i]->GetUnknownC34() < 0)
        {
            const TCHAR* IconName = Item->IconName;
            if (IconName)
                Item->Icon = FindTexture(IconName);
            if (!Item->Icon)
                Item->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
            Info.Items[i]->Copy(new NItemEntry(Item));
            Info.Items[i]->SetUnknownC34(1);
            return;
        }
    }
}
NW_HOOK_METHOD(0x100bc300, NCTrainInfoWnd, AddRequiredItem)

NW_CHECK_VTABLE(NCTrainInfoWnd, 0x100f0800, 64)

/*-----------------------------------------------------------------------------
    NCTrainWnd
-----------------------------------------------------------------------------*/

// 0x100bc3f0
NCTrainWnd::NCTrainWnd()
{
    ListWnd = NULL;
    InfoWnd = NULL;
}
NW_HOOK_CTOR(0x100bc3f0, NCTrainWnd)

// 0x100bc440
NW_HOOK_DTOR(0x100bc440, NCTrainWnd)

// stock body shared with NControl's (0x10004e90)
INT NCTrainWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100bc8a0 (vslot 35) - the pages on top of each other under the title bar
INT NCTrainWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_TRAIN_FRAME;
    FrameCtrl->ID = NID_TRAIN_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    FrameCtrl->SetTitleText(*GL2GameData.SysString[477]);            // "Skill Trainer"
    ListWnd = new NCTrainListWnd;
    ListWnd->SetWindowRect(0, 21, 240, 344);
    CreateChild(ListWnd, NWS_VISIBLE);
    InfoWnd = new NCTrainInfoWnd;
    InfoWnd->SetWindowRect(0, 21, 240, 344);
    CreateChild(InfoWnd, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100bc8a0, NCTrainWnd, OnCreate)

// 0x10089310 (vslot 36) - also NCMSView_CharacterSelectWnd's and
// NCPawnView_CharacterSelectWnd's (their children at +0x94 and +0x98)
INT NCTrainWnd::OnDestroy()
{
    DestroyChild(this, ListWnd);
    DestroyChild(this, InfoWnd);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10089310, NCTrainWnd, OnDestroy)

// stock body shared with NCEditBox's (0x100a5d70)
void NCTrainWnd::ShowWindow()
{
    NCWnd::ShowWindow();
}

// stock body shared with NCChatWnd's (0x1003dab0)
void NCTrainWnd::HideWindow()
{
    NCWnd::HideWindow();
}

// 0x100bc490 (vslot 50) - draws nothing (the pages do)
INT NCTrainWnd::OnPaint(NCGDevice* Device)
{
    guard(NCTrainWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100bc490, NCTrainWnd, OnPaint)

// 0x100baeb0 - a new list is coming
void NCTrainWnd::Reset()
{
    ListWnd->Clear();
    InfoWnd->Reset();
}
NW_HOOK_METHOD(0x100baeb0, NCTrainWnd, Reset)

// 0x100bae60
void NCTrainWnd::ShowList()
{
    InfoWnd->HideWindow();
    ListWnd->ShowWindow();
}
NW_HOOK_METHOD(0x100bae60, NCTrainWnd, ShowList)

// 0x100bae80
void NCTrainWnd::ShowInfo()
{
    ListWnd->HideWindow();
    InfoWnd->ShowWindow();
}
NW_HOOK_METHOD(0x100bae80, NCTrainWnd, ShowInfo)

// 0x100baea0
void NCTrainWnd::SetSkillInfo(NTrainSkillInfo* Info)
{
    InfoWnd->SetInfo(Info);
}
NW_HOOK_METHOD(0x100baea0, NCTrainWnd, SetSkillInfo)

// 0x100bc540
void NCTrainWnd::AddRequiredItem(NItemInfo* Item)
{
    InfoWnd->AddRequiredItem(Item);
}
NW_HOOK_METHOD(0x100bc540, NCTrainWnd, AddRequiredItem)

// 0x100bca90
void NCTrainWnd::AddEntry(const NTrainEntry* Entry)
{
    ListWnd->AddEntry(Entry);
}
NW_HOOK_METHOD(0x100bca90, NCTrainWnd, AddEntry)

NW_CHECK_VTABLE(NCTrainWnd, 0x100f06e0, 62)
