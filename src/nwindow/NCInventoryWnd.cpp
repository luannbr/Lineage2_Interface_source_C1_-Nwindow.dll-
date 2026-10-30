/*=============================================================================
    NCInventoryWnd.cpp - the inventory. Stock: 0x100641f0 .. 0x10064289, 0x10065920 ..
    0x10066182, 0x100681a0 .. 0x10068373 and OnCreate (0x100684d0); the item pages' and the
    paperdoll's functions sit in between.
=============================================================================*/
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NCTabControl.h"

#include <stdio.h>
#include <wchar.h>

// ids (names ours)
enum
{
    NID_INVENTORY_FRAME = 0xF2,
    NID_INVENTORY_TAB = 0xFE001A,
};

// sysstring
enum
{
    STR_Item = 2,
    STR_Quest = 118,
};

// 0x10065920 - the paperdoll and most textures are left as allocated
NCInventoryWnd::NCInventoryWnd()
{
    BackTex = NULL;
    TabCtrl = NULL;
    ItemWnd[0] = NULL;
    ItemWnd[1] = NULL;
    bOverCrystallize = 0;
    bOverTrash = 0;
    TrashW = 32;
    TrashH = 32;
    TrashX = 190;
    TrashY = 315;
    CrystallizeW = 32;
    CrystallizeH = 32;
    CrystallizeX = 16;
    CrystallizeY = 315;
}
NW_HOOK_CTOR(0x10065920, NCInventoryWnd)

// 0x10065a00
NW_HOOK_DTOR(0x10065a00, NCInventoryWnd)

// stock body shared with NControl's (0x10004e90)
INT NCInventoryWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100641f0 - edges included
UBOOL NCInventoryWnd::IsInTrash(INT X, INT Y)
{
    X -= ScreenX + TrashX;
    Y -= ScreenY + TrashY;
    return X >= 0 && X <= TrashW && Y >= 0 && Y <= TrashH;
}
NW_HOOK_METHOD(0x100641f0, NCInventoryWnd, IsInTrash)

// 0x10064240 - edges included
UBOOL NCInventoryWnd::IsInCrystallize(INT X, INT Y)
{
    X -= ScreenX + CrystallizeX;
    Y -= ScreenY + CrystallizeY;
    return X >= 0 && X <= CrystallizeW && Y >= 0 && Y <= CrystallizeH;
}
NW_HOOK_METHOD(0x10064240, NCInventoryWnd, IsInCrystallize)

// 0x10065a50 (vslot 50) - the background, the "Item" page's adena (thousands separators, right
// aligned to 184), the weight bar (87 pixels at full load; its texture by the load: up to 50%,
// 66.66%, 80%, above; the first texture sampled 10 rows high, the others 12) with the percent
// centered on it, and the crystallize (with the skill) and trash icons, lit while an item is
// carried over them
INT NCInventoryWnd::OnPaint(NCGDevice* Device)
{
    guard(NCInventoryWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 344, 0, 0, 240, 344, BackTex);

    TCHAR Buf[1024];
    TCHAR Amount[128];
    SIZE Size;
    appSprintf(Buf, TEXT("%d"), ItemWnd[0]->GetAdena());
    FormatNumber(Amount, Buf);
    appSprintf(Buf, TEXT("%s"), Amount);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawText(184 - Size.cx, 317, 0xFFFFFFFF, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (AdenaTex)
        Device->DrawTexture(79, 317, 16, 12, 0, 0, 16, 12, AdenaTex);
    if (WeightIconTex)
        Device->DrawTexture(79, 333, 16, 12, 0, 0, 16, 12, WeightIconTex);

    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        FLOAT Percent = 0.0f;
        if (Me->MaxLoad)
        {
            INT W = Me->Load * 87 / Me->MaxLoad;
            if (W > 0)
            {
                // the first test uses the unrounded quotient (fst, then fcomp); the others
                // reload the stored float, as the stock code does
                DOUBLE Pct = (DOUBLE)Me->Load / Me->MaxLoad * 100.0;
                Percent = (FLOAT)Pct;
                if (Pct <= 50.0)
                    Device->DrawTexture(100, 333, W, 12, 0, 0, W, 10, WeightTex[0]);
                else if (Percent <= 66.66)
                    Device->DrawTexture(100, 333, W, 12, 0, 0, W, 12, WeightTex[1]);
                else if (Percent >= 66.66 && Percent <= 80.0f)
                    Device->DrawTexture(100, 333, W, 12, 0, 0, W, 12, WeightTex[2]);
                else if (Percent > 80.0f)
                    Device->DrawTexture(100, 333, W, 12, 0, 0, W, 12, WeightTex[3]);
            }
        }
        TCHAR PercentText[1024];
        swprintf(PercentText, ARRAY_COUNT(PercentText), TEXT("%.2f%s"), (DOUBLE)Percent, TEXT("%"));
        NCGDevice::GetTextExtent(&Size, PercentText, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 141 - Size.cx / 2, 334, 0xFFFFFFFF, PercentText);
        if (Me->bCanCrystallize)
            Device->DrawTexture(CrystallizeX, CrystallizeY, 32, 32, 0, 0, 32, 32,
                                bOverCrystallize ? CrystallizeActiveTex : CrystallizeTex);
    }
    Device->DrawTexture(TrashX, TrashY, 32, 32, 0, 0, 32, 32, bOverTrash ? TrashActiveTex : TrashTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10065a50, NCInventoryWnd, OnPaint)

// 0x10065de0 (vslot 36)
INT NCInventoryWnd::OnDestroy()
{
    if (ItemWnd[0])
    {
        RemoveChildWindow(ItemWnd[0]->Entry);
        delete ItemWnd[0];
        ItemWnd[0] = NULL;
    }
    if (ItemWnd[1])
    {
        RemoveChildWindow(ItemWnd[1]->Entry);
        delete ItemWnd[1];
        ItemWnd[1] = NULL;
    }
    if (EquipWnd)
    {
        RemoveChildWindow(EquipWnd->Entry);
        delete EquipWnd;
        EquipWnd = NULL;
    }
    if (TabCtrl)
    {
        RemoveChildWindow(TabCtrl->Entry);
        delete TabCtrl;
        TabCtrl = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10065de0, NCInventoryWnd, OnDestroy)

// 0x10065eb0 (vslot 42) - the tab's page shows again; coming back from the icon the window
// opens where the icon was
void NCInventoryWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
    if (Icon)
    {
        SetWindowRect(Icon->ScreenX, Icon->ScreenY, Width, Height);
        Icon->SetWindowRect(0, 0, 0, 0);
        Icon->HideWindow();
    }
}
NW_HOOK_METHOD(0x10065eb0, NCInventoryWnd, ShowWindow)

// 0x10065f10 - an enchant scroll in use (the console): the item windows keep the cursor, which
// shows the icon of the item picked up
void NCInventoryWnd::SetEnchantMode(UBOOL bOn)
{
    EquipWnd->SetEnchantMode(bOn);
    ItemWnd[0]->SetEnchantMode(bOn);
    if (bOn)
    {
        NCInvenItemWnd* Page = ItemWnd[0];
        if (Page && Page->Selected >= 0 && Page->Selected < Page->Items.Num())
        {
            SetCursor(4, Page->Items(Page->Selected)->GetIcon());
            LockCursor();
        }
    }
    else
    {
        UnlockCursor();
        SetCursor(0, NULL);
    }
}
NW_HOOK_METHOD(0x10065f10, NCInventoryWnd, SetEnchantMode)

// 0x10065f90 - names and descriptions again from the item tables (the language changed),
// then the tool tips
void NCInventoryWnd::ReloadTexts()
{
    for (INT i = 0; i < EquipWnd->Items.Num(); i++)
    {
        NItemEntry* Item = EquipWnd->Items(i);
        if (!Item)
            continue;
        appStrcpy(Item->Name, GL2GameData.GetItemName(Item->ID));
        appStrcpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
        EquipWnd->SetToolTip(Item);
    }
    for (INT i = 0; i < ItemWnd[0]->Items.Num(); i++)
    {
        NItemEntry* Item = ItemWnd[0]->Items(i);
        if (!Item)
            continue;
        appStrcpy(Item->Name, GL2GameData.GetItemName(Item->ID));
        appStrcpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
        ItemWnd[0]->SetToolTip(Item);
    }
    for (INT i = 0; i < ItemWnd[1]->Items.Num(); i++)
    {
        NItemEntry* Item = ItemWnd[1]->Items(i);
        if (!Item)
            continue;
        appStrcpy(Item->Name, GL2GameData.GetItemName(Item->ID));
        appStrcpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
        ItemWnd[1]->SetToolTip(Item);
    }
}
NW_HOOK_METHOD(0x10065f90, NCInventoryWnd, ReloadTexts)

// 0x10066120 - the quest page is not searched
NItemEntry* NCInventoryWnd::FindItem(INT ObjectID)
{
    INT Index;
    if (EquipWnd->FindItem(ObjectID, &Index))
        return EquipWnd->Items(Index);
    if (ItemWnd[0]->FindItem(ObjectID, &Index))
        return ItemWnd[0]->Items(Index);
    return NULL;
}
NW_HOOK_METHOD(0x10066120, NCInventoryWnd, FindItem)

// 0x100681a0
void NCInventoryWnd::ClearItems()
{
    EquipWnd->Clear();
    ItemWnd[0]->Clear();
    ItemWnd[1]->Clear();
}
NW_HOOK_METHOD(0x100681a0, NCInventoryWnd, ClearItems)

// 0x100681d0 - equipped items (NItemInfo::UnknownC8C) to the paperdoll, quest items (type 3)
// to the "Quest" page, the rest to the "Item" page
UBOOL NCInventoryWnd::AddItem(NItemInfo* Info)
{
    INT Page = Info->Param == 3;
    if (Info->UnknownC8C)
    {
        EquipWnd->AddItem(Info);
        return 1;
    }
    if (ItemWnd[Page])
        ItemWnd[Page]->AddItem(Info);
    return 1;
}
NW_HOOK_METHOD(0x100681d0, NCInventoryWnd, AddItem)

// 0x10068220 - a changed item moves between the paperdoll and its page when it was put on or
// taken off (put on, it is looked for on the "Item" page only)
UBOOL NCInventoryWnd::UpdateItem(NItemInfo* Info, INT Change)
{
    INT Page = Info->Param == 3;
    if (Change == 1)
    {
        if (Info->UnknownC8C)
            EquipWnd->AddItem(Info);
        else
            ItemWnd[Page]->AddItem(Info);
    }
    else if (Change == 2)
    {
        if (Info->UnknownC8C)
        {
            if (EquipWnd->HasItem(Info->ObjectID))
                EquipWnd->UpdateItem(Info);
            else
            {
                ItemWnd[0]->RemoveItem(Info->ObjectID);
                EquipWnd->AddItem(Info);
            }
        }
        else
        {
            NCInvenItemWnd* Wnd = ItemWnd[Page];
            if (Wnd->HasItem(Info->ObjectID))
                Wnd->UpdateItem(Info);
            else
            {
                EquipWnd->RemoveItem(Info->ObjectID);
                ItemWnd[Page]->AddItem(Info);
            }
        }
    }
    else if (Change == 3)
    {
        if (Info->UnknownC8C)
        {
            if (EquipWnd)
                EquipWnd->RemoveItem(Info->ObjectID);
        }
        else
            ItemWnd[Page]->RemoveItem(Info->ObjectID);
    }
    return 1;
}
NW_HOOK_METHOD(0x10068220, NCInventoryWnd, UpdateItem)

// 0x100684d0 (vslot 35) - the title bar, the paperdoll (32-pixel cells), the two pages (6 x 4
// icons of 32 pixels, 160 items at most) and the tabs that switch them
INT NCInventoryWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.InventoryWnd.InventoryBack"));
    CrystallizeTex = FindTexture(TEXT("L2UI.WindowIcon.Crystalize"));
    CrystallizeActiveTex = FindTexture(TEXT("L2UI.WindowIcon.Crystalize_active"));
    TrashTex = FindTexture(TEXT("L2UI.WindowIcon.DestroyItem"));
    TrashActiveTex = FindTexture(TEXT("L2UI.WindowIcon.DestroyItem_active"));
    WeightTex[0] = FindTexture(TEXT("L2UI.InventoryWnd.InventoryWT1"));
    WeightTex[1] = FindTexture(TEXT("L2UI.InventoryWnd.InventoryWT2"));
    WeightTex[2] = FindTexture(TEXT("L2UI.InventoryWnd.InventoryWT3"));
    WeightTex[3] = FindTexture(TEXT("L2UI.InventoryWnd.InventoryWT4"));
    AdenaTex = FindTexture(TEXT("L2UI.WindowIcon.WindowIconAdena"));
    WeightIconTex = FindTexture(TEXT("L2UI.WindowIcon.Weight"));

    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_INVENTORY_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    EquipWnd = new NCEquipItemWnd(32);
    EquipWnd->SetWindowRect(16, 29, 207, 102);
    CreateChild(EquipWnd, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown

    ItemWnd[0] = new NCInvenItemWnd(6, 4, 160, 32);
    ItemWnd[0]->SetWindowRect(16, 166, 222, 137);
    CreateChild(ItemWnd[0], NWS_VISIBLE | 0x20);

    ItemWnd[1] = new NCInvenItemWnd(6, 4, 160, 32);
    ItemWnd[1]->SetWindowRect(16, 166, 222, 137);
    CreateChild(ItemWnd[1], NWS_VISIBLE | 0x20);

    TabCtrl = new NCTabControl();
    TabCtrl->ID = NID_INVENTORY_TAB;
    TabCtrl->SetWindowRect(15, 141, 138, 16);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(STR_Item, ItemWnd[0], 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0,
                    -1);
    TabCtrl->AddTab(STR_Quest, ItemWnd[1], 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0,
                    -1);
    return 1;
}
NW_HOOK_METHOD(0x100684d0, NCInventoryWnd, OnCreate)

NW_CHECK_VTABLE(NCInventoryWnd, 0x100e5378, 62)
