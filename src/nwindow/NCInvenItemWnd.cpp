/*=============================================================================
    NCInvenItemWnd.cpp - a page of the inventory's items. Stock: 0x10062360 .. 0x100641e7,
    0x10065060 .. 0x1006590f, 0x10067690 .. 0x1006819f and 0x10068440 .. 0x100684c9 (the
    paperdoll and the inventory sit in between), plus the destructor (0x1005fe00) and
    WindowProc (0x10057b10).
=============================================================================*/
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NItemToolTip.h"

enum { ITEM_ADENA = 57 };
enum { SOUND_PickUp = 3 };

// NDialogInfo::ID of the questions (names ours)
enum
{
    DIALOG_DROP_ITEM = 4,
    DIALOG_DESTROY_ITEM = 5,
    DIALOG_CRYSTALLIZE_ITEM = 6,
    DIALOG_GIVE_TO_PET = 50,
};

// systemmsg
enum
{
    MSG_DropHowMany = 71,       // "How many $s1(s) do you want to discard?"
    MSG_MoveHowMany = 72,       // "How many $s1(s) do you want to move?"
    MSG_DestroyHowMany = 73,    // "How many $s1(s) do you want to destroy?"
    MSG_Destroy = 74,           // "Destroy $s1. Do you want to continue?"
    MSG_CannotDropHere = 179,   // "You cannot discard items here."
    MSG_AreYouSure = 365,       // "Are you sure?"
    MSG_Drop = 400,             // "Discard $s1. Do you wish to continue?"
    MSG_Place = 651,            // "Place $s1 in the current location and direction..."
};

static FORCEINLINE NCDialogBox* MainDialogBox()
{
    return GDesktop->MainDialogBox;
}

// the desktop's box closes when it asks about an item (repeated inline in the stock code)
static FORCEINLINE void CloseItemDialog()
{
    if (MainDialogBox()->IsShowWindow())
    {
        INT ID = MainDialogBox()->DialogID;
        if (ID == DIALOG_DESTROY_ITEM || ID == DIALOG_DROP_ITEM || ID == DIALOG_CRYSTALLIZE_ITEM)
        {
            MainDialogBox()->Reset();
            MainDialogBox()->HideWindow();
        }
    }
}

// 0x10068440
NCInvenItemWnd::NCInvenItemWnd()
{
    bDropPending = 0;
    bDestroyPending = 0;
    bCrystallizePending = 0;
    bPetPending = 0;
    bEnchantMode = 0;
    bDblClicked = 0;
}
NW_HOOK_CTOR(0x10068440, NCInvenItemWnd)

// 0x10068480 - the inventory's pages are 6 x 4 icons of 32 pixels, 160 at most;
// bCrystallizePending is left as allocated
NCInvenItemWnd::NCInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
    bDropPending = 0;
    bDestroyPending = 0;
    bEnchantMode = 0;
    bDblClicked = 0;
    bPetPending = 0;
}
NW_HOOK_CTOR_ARGS(0x10068480, NCInvenItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x1005fe00
NW_HOOK_DTOR(0x1005fe00, NCInvenItemWnd)

// 0x10057b10 (vslot 24) - stock body shared with NCTradeItemWnd
INT NCInvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    if (Msg == NWM_DIALOG_RESULT)
        return OnDialogResult(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10057b10, NCInvenItemWnd, WindowProc)

// 0x10062360
void NCInvenItemWnd::SetEnchantMode(UBOOL bOn)
{
    bEnchantMode = bOn;
}
NW_HOOK_METHOD(0x10062360, NCInvenItemWnd, SetEnchantMode)

// 0x10062370 - the name tip (the amount with thousands separators) and the text that sizes
// the selected item's box (NItemToolTip.h)
void NCInvenItemWnd::SetToolTip(NItemEntry* Item)
{
    ItemTip::SetTexts<1024, 0, 128>(this, Item);
}
NW_HOOK_METHOD(0x10062370, NCInvenItemWnd, SetToolTip)

// 0x10062a60 (vslot 67) - the selected item's box (NItemToolTip.h)
void NCInvenItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
    ItemTip::DrawLines<1024, 0, 128>(this, Device, Item, X, Y);
}
NW_HOOK_METHOD(0x10062a60, NCInvenItemWnd, DrawItemToolTip)

// 0x10065060 (vslot 58) - uses the item under the mouse
INT NCInvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
        GConsole->RequestUseItem(Items(Selected)->ObjectID);
    CloseItemDialog();
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10065060, NCInvenItemWnd, OnRButtonDown)

// 0x10065120 (vslot 56) - NCItemWnd's, but the questions pending are forgotten, the desktop's
// box closes only when it asks about an item, and it keeps no focus
INT NCInvenItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    bDropPending = 0;
    bDestroyPending = 0;
    bCrystallizePending = 0;
    bPetPending = 0;
    INT Index = HitTest(LOWORD(lParam), HIWORD(lParam));
    Selected = Index;
    HoverIndex = Index;
    if (Index >= 0 && Index < Items.Num())
    {
        UnselectAll();
        Items(Selected)->Select();
        CloseItemDialog();
        if (!bNoDrag)
        {
            bDragging = 1;
            GConsole->Engine->OnInterfacePlaySound(SOUND_PickUp);
            return 1;
        }
        bNoDrag = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x10065120, NCInvenItemWnd, OnLButtonDown)

// 0x10065240 (vslot 68) - uses the item; the release that follows moves nothing
INT NCInvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    bDragging = 0;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
        GConsole->RequestUseItem(Items(Selected)->ObjectID);
    bDblClicked = 1;
    UnselectAll();
    Selected = -1;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10065240, NCInvenItemWnd, OnLButtonDblClick)

// 0x100652d0 (vslot 69) - the answer to the question asked (wParam: the amount, 1 without an
// edit line): the request for the selected item. Items cannot be dropped where the player's
// pawn has bit 0 of +0x690.
INT NCInvenItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemRequest Request;
        Request.ObjectID = Items(Selected)->ObjectID;
        Request.Count = (INT)wParam;
        Request.Unknown02 = 0;       // the stock code leaves it as the stack held it
        if (bDropPending)
        {
            User* Me = GConsole->GetMyUser();
            if (Me->Pawn->Unknown690 & 1)
                DesktopSystemMessage(*GL2GameData.SystemMsg[MSG_CannotDropHere].Message);
            else
                NRequestDropItem(Request, DropLocation);
            bDropPending = 0;
        }
        if (bDestroyPending)
        {
            NRequestDestroyItem(Request);
            bDestroyPending = 0;
        }
        if (bCrystallizePending)
        {
            GConsole->RequestCrystallizeItem(Request.ObjectID, (INT)wParam);
            bCrystallizePending = 0;
        }
        if (bPetPending)
        {
            L2ParamStack Params(10);
            Params.PushBack((void*)(INT_PTR)Request.ObjectID);
            Params.PushBack((void*)(INT_PTR)wParam);
            GConsole->RequestGiveItemToPet(Params);
            bPetPending = 0;
        }
    }
    UnselectAll();
    Selected = -1;
    return 1;
}
NW_HOOK_METHOD(0x100652d0, NCInvenItemWnd, OnDialogResult)

// 0x100654c0 - the item with Info's object id takes Info's fields and new tool tips; the entry
// built for that is never freed, as in the stock code
UBOOL NCInvenItemWnd::UpdateItem(NItemInfo* Info)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Found = Items(i);
        if (Info->ObjectID == Found->ObjectID)
        {
            Info->Icon = FindTexture(Info->IconName);
            if (!Info->Icon)
                Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
            NItemEntry* Updated = new NItemEntry(Info);
            SetToolTip(Updated);
            Found->Copy(Updated);
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x100654c0, NCInvenItemWnd, UpdateItem)

// 0x100655c0
INT NCInvenItemWnd::GetAdena()
{
    INT Adena = 0;
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (Item && Item->ID == ITEM_ADENA)
            Adena += Item->Count;
    }
    return Adena;
}
NW_HOOK_METHOD(0x100655c0, NCInvenItemWnd, GetAdena)

// 0x100655f0 (vslot 50) - NCQuestItemWnd's drawing with a clip one pixel taller; the stock
// guard carries NCItemWnd's name
INT NCInvenItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCItemWnd::OnPaint);
    Device->SetClip(ScreenX - 1, ScreenY - 1, Width + 2, Height + 3);
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
NW_HOOK_METHOD(0x100655f0, NCInvenItemWnd, OnPaint)

// 0x100657b0 (vslot 55) - NCItemWnd's cursor handling (the enchant scroll's cursor stays too),
// then the inventory's trash / crystallize icons light up under the mouse
INT NCInvenItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    HoverIndex = HitTest(X, Y);
    if (IsFocused())
    {
        if (bDragging)
        {
            if (Selected >= 0 && Selected < Items.Num())
            {
                SetCursor(4, Items(Selected)->GetIcon());
                LockCursor();
            }
        }
        else if (!GConsole->IsCtrlDown() && !bEnchantMode)
        {
            UnlockCursor();
            SetCursor(0, NULL);
        }
        NCInventoryWnd* Inventory = (NCInventoryWnd*)Entry->Parent->Wnd;
        Inventory->bOverTrash = PtInRect(X, Y, Inventory->ScreenX + Inventory->TrashX,
                                         Inventory->ScreenY + Inventory->TrashY, Inventory->TrashW,
                                         Inventory->TrashH) != 0;
        Inventory->bOverCrystallize = PtInRect(X, Y, Inventory->ScreenX + Inventory->CrystallizeX,
                                               Inventory->ScreenY + Inventory->CrystallizeY,
                                               Inventory->CrystallizeW, Inventory->CrystallizeH) != 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x100657b0, NCInvenItemWnd, OnMouseMove)

// 0x10067690 (vslot 57) - where the carried item is put down (see the header); the name
// buffers are the stock frame's 64 characters
INT NCInvenItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (!GConsole->IsCtrlDown() && !bEnchantMode)
    {
        UnlockCursor();
        SetCursor(0, NULL);
    }
    if (Selected < 0 || Selected >= Items.Num())
    {
        bDragging = 0;
        return 1;
    }
    if (bDragging && !bDblClicked)
    {
        NItemEntry* Item = Items(Selected);
        if (!Item)
        {
            bDragging = 0;
            return 1;
        }
        if (bEnchantMode)
        {
            GConsole->RequestEnchantItem(Item->ObjectID);
            bDblClicked = 0;
            bDragging = 0;
            return 1;
        }
        INT X = LOWORD(lParam);
        INT Y = HIWORD(lParam);
        if (IsInWindow(X, Y))
        {
            // moved inside the page
            INT Target = HitTest(X, Y);
            if (Target != Selected && Target >= 0 && Selected >= 0 && Selected < Items.Num())
            {
                if (Target >= Items.Num())
                {
                    Items.Remove(Selected);
                    Items.AddItem(Item);
                    Selected = Items.Num() - 1;
                }
                else
                {
                    Items.Remove(Selected);
                    Items.Insert(Target);
                    Items(Target) = Item;
                }
            }
        }
        else if (NWndEntry* E = FindWindowAt(X, Y))
        {
            NCWnd* Wnd = E->Wnd;
            TCHAR Name[64];
            TCHAR DropName[64];
            if (!Wnd)
                ;
            else if (Wnd->IsA(NCEquipItemWnd::StaticClass()))
            {
                // equipped
                ((NCEquipItemWnd*)Wnd)->HitTest(X, Y);
                GConsole->RequestUseItem(Items(Selected)->ObjectID);
                UnselectAll();
                Selected = -1;
            }
            else if (Wnd->IsA(NCShortCutWnd::StaticClass()))
            {
                INT Slot = ((NCShortCutWnd*)Wnd)->SlotAt(X, Y);
                if (Slot >= 0)
                {
                    L2ParamStack Params(10);
                    Params.PushBack((void*)1);  // an item
                    Params.PushBack((void*)(INT_PTR)Slot);
                    Params.PushBack((void*)(INT_PTR)Items(Selected)->ObjectID);
                    Params.PushBack((void*)1);
                    GConsole->RequestShortCutReg(Params);
                }
                UnselectAll();
                Selected = -1;
            }
            else if (Wnd->IsA(NClass::NCPetWnd()))
            {
                bPetPending = 1;
                if (Item->IsStackable() && Item->Count > 1)
                {
                    appSprintf(Name, TEXT("%s"), Item->GetName());
                    NDialogInfo Info;
                    Info.Owner = Entry;
                    Info.Type = 3;
                    Info.bInput = 1;
                    Info.bMainBox = 1;
                    Info.ID = DIALOG_GIVE_TO_PET;
                    Info.SetSystemMessage(MSG_MoveHowMany);
                    Info.SetButtons(Name, NULL);
                    Info.bLargeAmount = Item->ID == ITEM_ADENA;
                    GConsole->ShowDialog(&Info);
                }
                else
                {
                    L2ParamStack Params(10);
                    Params.PushBack((void*)(INT_PTR)Items(Selected)->ObjectID);
                    Params.PushBack((void*)1);
                    GConsole->RequestGiveItemToPet(Params);
                }
            }
            else if (Wnd->IsA(NCInventoryWnd::StaticClass()))
            {
                NCInventoryWnd* Inventory = (NCInventoryWnd*)Wnd;
                Inventory->bOverTrash = 0;
                Inventory->bOverCrystallize = 0;
                if (Inventory->IsInTrash(X, Y))
                {
                    bDestroyPending = 1;
                    if (Item->IsStackable() && Item->Count > 1)
                    {
                        appSprintf(Name, TEXT("%s"), Item->GetName());
                        NDialogInfo Info;
                        Info.Owner = Entry;
                        Info.Type = 3;
                        Info.bInput = 1;
                        Info.bMainBox = 1;
                        Info.ID = DIALOG_DESTROY_ITEM;
                        Info.SetSystemMessage(MSG_DestroyHowMany);
                        Info.SetButtons(Name, NULL);
                        Info.bLargeAmount = Item->ID == ITEM_ADENA;
                        GConsole->ShowDialog(&Info);
                    }
                    else
                    {
                        // the message and the name copied in directly
                        appSprintf(Name, TEXT("%s"), Item->GetName());
                        NDialogInfo Info;
                        Info.bMainBox = 1;
                        Info.ID = DIALOG_DESTROY_ITEM;
                        Info.Owner = Entry;
                        Info.bInput = 0;
                        appStrcpy(Info.Text, *GL2GameData.SystemMsg[MSG_Destroy].Message);
                        appStrcpy(Info.Button1, Name);
                        Info.bLargeAmount = Item->ID == ITEM_ADENA;
                        GConsole->ShowDialog(&Info);
                    }
                }
                else if (Inventory->IsInCrystallize(X, Y))
                {
                    NViewItemData* Data = NItemTable::Find(&Item->ID);
                    User* Me = GConsole->GetMyUser();
                    if (Me->bCanCrystallize && Data && Data->CrystalCount)
                    {
                        bCrystallizePending = 1;
                        appSprintf(Name, TEXT("%s"), Item->GetName());  // not shown
                        NDialogInfo Info;
                        Info.bMainBox = 1;
                        Info.ID = DIALOG_CRYSTALLIZE_ITEM;
                        Info.bInput = 0;
                        Info.Owner = Entry;
                        Info.SetSystemMessage(MSG_AreYouSure);
                        Info.bLargeAmount = 0;
                        GConsole->ShowDialog(&Info);
                    }
                }
            }
            else if (Wnd->IsA(NClass::NCConsole()))
            {
                // on the ground, unless a pawn is under the mouse
                if (!GConsole->GetTargetedActor(1))
                {
                    bDropPending = 1;
                    appSprintf(DropName, TEXT("%s"), Item->GetName());
                    DropLocation = static_cast<UGameEngine*>(GConsole->Engine)->MouseHitLocation;
                    UBOOL bPlace = 0;
                    NViewItemData* Data = NItemTable::Find(&Item->ID);
                    if (Data && Data->Class == 2 && Data->EtcType == 8)
                        bPlace = 1;
                    if (Item->IsStackable() && Item->Count > 1 && !bPlace)
                    {
                        NDialogInfo Info;
                        Info.Owner = Entry;
                        Info.Type = 3;
                        Info.bInput = 1;
                        Info.bMainBox = 1;
                        Info.ID = DIALOG_DROP_ITEM;
                        Info.SetSystemMessage(MSG_DropHowMany);
                        Info.SetButtons(DropName, NULL);
                        Info.bLargeAmount = Item->ID == ITEM_ADENA;
                        GConsole->ShowDialog(&Info);
                    }
                    else if (bPlace)
                    {
                        NDialogInfo Info;
                        Info.Owner = Entry;
                        Info.ID = DIALOG_DROP_ITEM;
                        Info.bMainBox = 1;
                        Info.bInput = 0;
                        Info.SetSystemMessage(MSG_Place);
                        Info.SetButtons(DropName, NULL);
                        Info.bLargeAmount = 0;
                        GConsole->ShowDialog(&Info);
                    }
                    else
                    {
                        NDialogInfo Info;
                        Info.Owner = Entry;
                        Info.ID = DIALOG_DROP_ITEM;
                        Info.bMainBox = 1;
                        Info.bInput = 0;
                        Info.SetSystemMessage(MSG_Drop);
                        Info.SetButtons(DropName, NULL);
                        Info.bLargeAmount = 0;
                        GConsole->ShowDialog(&Info);
                    }
                }
            }
        }
    }
    bDblClicked = 0;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10067690, NCInvenItemWnd, OnLButtonUp)

// 0x10068030 - the icon comes from its name ("NWindow.BlackTexture" when missing), written
// back into Info; 0 when the page is full
UBOOL NCInvenItemWnd::AddItem(NItemInfo* Info)
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
NW_HOOK_METHOD(0x10068030, NCInvenItemWnd, AddItem)

// 0x10068120
UBOOL NCInvenItemWnd::RemoveItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (Items(i)->ObjectID == ObjectID)
        {
            NItemEntry* Item = Items(i);
            Items.Remove(i);
            if (Item)
                delete Item;
            UpdateScrollRange();
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x10068120, NCInvenItemWnd, RemoveItem)

// stock body shared with NCPetInvenItemWnd (0x10091c70), which keeps the address
UBOOL NCInvenItemWnd::HasItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
        if (ObjectID == Items(i)->ObjectID)
            return 1;
    return 0;
}

// stock body shared with NCPetInvenItemWnd (0x10091cc0), which keeps the address
UBOOL NCInvenItemWnd::FindItem(INT ObjectID, INT* Index)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (ObjectID == Items(i)->ObjectID)
        {
            *Index = i;
            return 1;
        }
    }
    return 0;
}

NW_CHECK_VTABLE(NCInvenItemWnd, 0x100e4b78, 70)
