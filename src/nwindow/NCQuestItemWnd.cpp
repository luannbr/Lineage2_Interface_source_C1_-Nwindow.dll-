/*=============================================================================
    NCQuestItemWnd.cpp - the quest window's item page. Stock: 0x1009fe70 .. 0x100a036d,
    0x100a0b40 .. 0x100a0fc5, 0x100a1490 .. 0x100a1578 and 0x100a17c0 .. 0x100a1889.
=============================================================================*/
#include "nwindow/NCQuestItemWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCQuestWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"

enum { DIALOG_DESTROY_ITEM = 5 };  // NDialogInfo::ID (name ours)

// 0x100a17c0
NCQuestItemWnd::NCQuestItemWnd()
{
}
NW_HOOK_CTOR(0x100a17c0, NCQuestItemWnd)

// 0x100a1860 - the quest window's is 6 x 4 icons of 32 pixels, 80 at most
NCQuestItemWnd::NCQuestItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
}
NW_HOOK_CTOR_ARGS(0x100a1860, NCQuestItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x100a1810
NW_HOOK_DTOR(0x100a1810, NCQuestItemWnd)

// 0x1009fe70 (vslot 24) - calls NCWnd's, past NCScrollWnd's: the scroll buttons and the
// slider of this page do nothing (the wheel still scrolls)
INT NCQuestItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_DIALOG_RESULT)
        return OnDialogResult(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1009fe70, NCQuestItemWnd, WindowProc)

// 0x1009fee0 (vslot 35)
INT NCQuestItemWnd::OnCreate()
{
    NCItemWnd::OnCreate();
    return 1;
}
NW_HOOK_METHOD(0x1009fee0, NCQuestItemWnd, OnCreate)

// 0x1009fef0 - the name tip ("Name(Count)" for a stack) and the text that sizes the selected
// item's box: name line, slot line and description
void NCQuestItemWnd::SetToolTip(NItemEntry* Item)
{
    if (!Item)
        return;
    if (Item->IsStackable())
        appSprintf(Item->ToolTipName, TEXT("%s(%d)"), Item->GetName(), Item->Count);
    else
        appSprintf(Item->ToolTipName, TEXT("%s"), Item->GetName());
    FString Desc;
    GConsole->ConvertNewLines((TCHAR*)Item->GetDescription(), Desc);
    FString Slot;
    GConsole->GetItemSlotText(Item->Type, Item->BodyPart, Slot, Item->SubType);
    if (Item->IsStackable())
        appSprintf(Item->ToolTipText, TEXT("%s(%d)\n%s\n%s"), Item->GetName(), Item->Count, *Slot, *Desc);
    else
        appSprintf(Item->ToolTipText, TEXT("%s\n%s\n%s"), Item->GetName(), *Slot, *Desc);
}
NW_HOOK_METHOD(0x1009fef0, NCQuestItemWnd, SetToolTip)

// 0x100a0070 (vslot 67) - the lines 13 pixels apart: the name, after the enchant in its own
// color (types 0..3), the slot line, the description wrapped at 168 pixels
void NCQuestItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
    if (!Item)
        return;
    FString Desc;
    GConsole->ConvertNewLines((TCHAR*)Item->GetDescription(), Desc);
    FString Slot;
    GConsole->GetItemSlotText(Item->Type, Item->BodyPart, Slot, Item->SubType);
    SIZE Size;
    TCHAR Text[1024];
    if (Item->Enchant > 0
        && (Item->Type == 0 || Item->Type == 1 || Item->Type == 3 || Item->Type == 2))
    {
        appSprintf(Text, TEXT("+%d "), Item->Enchant);
        Device->DrawText(X, Y, 0xFFC7DEFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
        if (Item->IsStackable())
            appSprintf(Text, TEXT("%s(%d)"), Item->GetName(), Item->Count);
        else
            appSprintf(Text, TEXT("%s"), Item->GetName());
        Device->DrawText(X + Size.cx, Y, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else
    {
        if (Item->IsStackable())
            appSprintf(Text, TEXT("%s(%d)"), Item->GetName(), Item->Count);
        else
            appSprintf(Text, TEXT("%s"), Item->GetName());
        Device->DrawText(X, Y, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    appSprintf(Text, TEXT("%s"), *Slot);
    Device->DrawText(X, Y + 13, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT("%s"), *Desc);
    Device->DrawText(X, Y + 26, 0xFFB2BECF, Text, 168, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 1);
}
NW_HOOK_METHOD(0x100a0070, NCQuestItemWnd, DrawItemToolTip)

// 0x100a0b40 (vslot 50) - NCItemWnd's, but the frame of the selected item goes over its icon
INT NCQuestItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCQuestItemWnd::OnPaint);
    Device->SetClip(ScreenX - 1, ScreenY - 1, Width + 2, Height + 2);
    Device->BeginPaint(ScreenX, ScreenY);
    INT X = 0;
    INT Y = 0;
    for (INT i = TopRow * Columns; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (!Item)
            continue;
        if (i % Columns == 0)
        {
            X = 0;
            if (i != TopRow * Columns)
                Y += CellSize + 3;
            if (VisibleRows * (CellSize + 1) <= Y)
                break;
        }
        else
            X += CellSize + 3;
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon());
        if (Item->IsSelected())
            Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, SelectTex);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100a0b40, NCQuestItemWnd, OnPaint)

// 0x100a0d00 (vslot 57) - the carried item is put down; over the quest window's drop zone it
// asks whether to destroy it (systemmsg 73 with an amount for a stack, else 74); the answer
// comes back as NWM_DIALOG_RESULT. The name buffer is the stock frame's 64 characters.
INT NCQuestItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (!GConsole->IsCtrlDown())
    {
        UnlockCursor();
        SetCursor(0, NULL);
    }
    if (Selected >= 0 && Selected < Items.Num() && bDragging)
    {
        INT X = LOWORD(lParam);
        INT Y = HIWORD(lParam);
        NWndEntry* E = FindWindowAt(X, Y);
        if (E && E->Wnd && E->Wnd->IsA(NCQuestWnd::StaticClass())
            && ((NCQuestWnd*)E->Wnd)->IsInDropZone(X, Y))
        {
            NItemEntry* Item = Items(Selected);
            TCHAR Name[64];
            if (Item->IsStackable() && Item->Count > 1)
            {
                appSprintf(Name, TEXT("%s"), Item->GetName());
                NDialogInfo Info;
                Info.ID = DIALOG_DESTROY_ITEM;
                Info.Type = 3;
                Info.Owner = Entry;
                Info.bMainBox = 1;
                Info.bInput = 1;
                Info.SetSystemMessage(73);  // "How many $s1(s) do you want to destroy?"
                Info.SetButtons(Name, NULL);
                GConsole->ShowDialog(&Info);
            }
            else
            {
                appSprintf(Name, TEXT("%s"), Item->GetName());
                NDialogInfo Info;
                Info.Owner = Entry;
                Info.ID = DIALOG_DESTROY_ITEM;
                Info.bMainBox = 1;
                Info.bInput = 0;
                Info.SetSystemMessage(74);  // "Destroy $s1. Do you want to continue?"
                Info.SetButtons(Name, NULL);
                GConsole->ShowDialog(&Info);
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100a0d00, NCQuestItemWnd, OnLButtonUp)

// 0x100a0f10 (vslot 68) - the destroy dialog's answer (wParam: the amount typed, 1 without an
// edit line): the request for the selected item
INT NCQuestItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemRequest Request;
        Request.ObjectID = Items(Selected)->ObjectID;
        Request.Count = (INT)wParam;
        Request.Unknown02 = 0;       // the stock code leaves it as the stack held it
        NRequestDestroyItem(Request);
        UnselectAll();
        Selected = -1;
    }
    return 1;
}
NW_HOOK_METHOD(0x100a0f10, NCQuestItemWnd, OnDialogResult)

// 0x100a1490 - the icon comes from its name ("NWindow.BlackTexture" when missing) and is
// written back into Info; 0 when the page is full
UBOOL NCQuestItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= MaxItems)
        return 0;
    Info->Icon = FindTexture(Info->IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = new NItemEntry(Info);
    SetToolTip(Item);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100a1490, NCQuestItemWnd, AddItem)

NW_CHECK_VTABLE(NCQuestItemWnd, 0x100ed728, 69)
