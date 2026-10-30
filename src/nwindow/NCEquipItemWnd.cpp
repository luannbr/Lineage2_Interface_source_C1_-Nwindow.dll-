/*=============================================================================
    NCEquipItemWnd.cpp - the inventory's paperdoll. Stock: 0x10060730 .. 0x10062357 (up to
    the tool tip's jump table), 0x100642d0 .. 0x10065056, 0x100661a0 .. 0x10067684 (the item
    pages' and the inventory's functions sit in between), plus the constructor (0x10068380)
    and the destructor (0x1005fd70).
=============================================================================*/
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NItemToolTip.h"
#if NW_FEATURES
// phase 6: ItemHoverTip (not in NWindow_C1: the hover tip of the item grids in the recent style)
#include "features/ItemHoverTip.h"
#endif

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
    MSG_DestroyHowMany = 73,    // "How many $s1(s) do you want to destroy?"
    MSG_Destroy = 74,           // "Destroy $s1. Do you want to continue?"
    MSG_CannotDropHere = 179,   // "You cannot discard items here."
    MSG_Crystallize = 336,      // "$s1 is crystallized. Do you want to continue?"
    MSG_Drop = 400,             // "Discard $s1. Do you wish to continue?"
    MSG_PetAdenaHowMuch = 575,  // "How much adena do you wish to transfer to your pet?"
    MSG_PetHowMuch = 576,       // "How much do you wish to transfer?"
};

// the slots' places: a grid of 35 pixels, six columns by three rows without the fourth
// column and the fifth of the middle row; 14 and 15 (the full armor over 6 and 10, the
// two-handed weapon over 5 and 7) are drawn apart
static const INT SlotX[14] = { 0, 35, 70, 140, 175, 0, 35, 70, 175, 0, 35, 70, 140, 175 };
static const INT SlotY[14] = { 0, 0, 0, 0, 0, 35, 35, 35, 35, 70, 70, 70, 70, 70 };

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

static FORCEINLINE INT EquipSlotOf(NItemEntry* Item)
{
    return GConsole->GetEquipSlot(Item->BodyPart, Item->ObjectID);
}

// is the item in slot ItemSlot the one of Slot? The two-slot items answer for 5 and 6
static FORCEINLINE UBOOL IsInSlot(INT ItemSlot, INT Slot)
{
    if (ItemSlot == 15)
        return Slot == 5;
    if (ItemSlot == 14)
        return Slot == 6;
    return Slot == ItemSlot;
}

// takes Item off when it is the one of Slot (OnRButtonDown, OnLButtonDblClick): by its body
// part bits, but an earring or a ring by the bit of the ear or the finger that wears it
static FORCEINLINE UBOOL UnEquipIfInSlot(NItemEntry* Item, INT Slot)
{
    INT ItemSlot = EquipSlotOf(Item);
    if (ItemSlot == 15 || ItemSlot == 14)
    {
        if (Slot != (ItemSlot == 15 ? 5 : 6))
            return 0;
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Item->BodyPart);
        GConsole->RequestUnEquipItem(Params);
        return 1;
    }
    if (Slot != ItemSlot)
        return 0;
    L2ParamStack Params(10);
    INT BodyPart = Item->BodyPart;
    if (BodyPart & 6)
    {
        User* Me = GConsole->GetMyUser();
        if (Me->Equipped[1] == Item->ObjectID)
            Params.PushBack((void*)2);
        if (Me->Equipped[2] == Item->ObjectID)
            Params.PushBack((void*)4);
    }
    else if (Item->BodyPart & 0x30)
    {
        User* Me = GConsole->GetMyUser();
        if (Me->Equipped[4] == Item->ObjectID)
            Params.PushBack((void*)0x10);
        if (Me->Equipped[5] == Item->ObjectID)
            Params.PushBack((void*)0x20);
    }
    else
        Params.PushBack((void*)(INT_PTR)BodyPart);
    GConsole->RequestUnEquipItem(Params);
    return 1;
}

// 0x10068380 - the inventory makes it with 32-pixel cells; DualCapTex is left as allocated
NCEquipItemWnd::NCEquipItemWnd(INT InCellSize)
{
    CellSize = InCellSize;
    SelectTex = NULL;
    ToolTipTex = NULL;
    TipTopTex = NULL;
    TipMidTex = NULL;
    TipBottomTex = NULL;
    bDragging = 0;
    bDropPending = 0;
    bDestroyPending = 0;
    bCrystallizePending = 0;
    bPetPending = 0;
    bEnchantMode = 0;
    bNoDrag = 0;
    bHasUnknown1C98 = 0;
    Unknown104 = 5;
    Unknown108 = 3;
    NumSlots = 14;
    Selected = -1;
    HoverSlot = -1;
    SelectedIndex = -1;
}
NW_HOOK_CTOR_ARGS(0x10068380, NCEquipItemWnd, (INT A), (A))

// 0x1005fd70
NW_HOOK_DTOR(0x1005fd70, NCEquipItemWnd)

// 0x10060730 (vslot 24)
INT NCEquipItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    if (Msg == NWM_DIALOG_RESULT)
        return OnDialogResult(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10060730, NCEquipItemWnd, WindowProc)

// 0x100607c0 (vslot 35)
INT NCEquipItemWnd::OnCreate()
{
    NCWnd::OnCreate();
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    TipTopTex = FindTexture(TEXT("L2UI.Tooltip.Tool02"));
    TipMidTex = FindTexture(TEXT("L2UI.Tooltip.Tool01"));
    TipBottomTex = FindTexture(TEXT("L2UI.Tooltip.Tool03"));
    SelectTex = FindTexture(TEXT("l2ui.nwindow.item_click"));
    DualCapTex = FindTexture(TEXT("L2UI.InventoryWnd.Icon_dualcap"));
    return 1;
}
NW_HOOK_METHOD(0x100607c0, NCEquipItemWnd, OnCreate)

// 0x10060840
void NCEquipItemWnd::SetEnchantMode(UBOOL bOn)
{
    bEnchantMode = bOn;
}
NW_HOOK_METHOD(0x10060840, NCEquipItemWnd, SetEnchantMode)

// 0x10060850 - the item pages' tool tip texts, but the amounts without thousands separators
// and no pet collar level (NItemToolTip.h; that copy has no buffers)
void NCEquipItemWnd::SetToolTip(NItemEntry* Item)
{
    ItemTip::SetTexts<1024, 0, 1024, 1>(this, Item);
}
NW_HOOK_METHOD(0x10060850, NCEquipItemWnd, SetToolTip)

// 0x10060e60 (vslot 64) - the selected item's box, the same differences (NItemToolTip.h)
void NCEquipItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
    ItemTip::DrawLines<1024, 0, 1024, 1>(this, Device, Item, X, Y);
}
NW_HOOK_METHOD(0x10060e60, NCEquipItemWnd, DrawItemToolTip)

// 0x100642d0 (vslot 50) - the icons in their slots, the two-slot items' in both (the second
// under the "dual" cap), with the second and third icons when the item has them; a bow is
// drawn in the right hand alone while an item with Unknown1C98 is equipped
INT NCEquipItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCEquipItemWnd::OnPaint);
    Device->SetClip(ScreenX - 1, ScreenY - 1, Width + 2, Height + 2);
    Device->BeginPaint(ScreenX, ScreenY);
    INT X = 0;
    INT Y = 0;
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (!Item)
            continue;
        INT Slot = EquipSlotOf(Item);
        if ((DWORD)Slot > 15)
            continue;
        UBOOL bTwoSlots = 0;
        if (Slot < 14)
        {
            X = SlotX[Slot];
            Y = SlotY[Slot];
        }
        else
            bTwoSlots = 1;
        if (Item->UnknownC30 == 6 && bHasUnknown1C98)
        {
            X = 0;
            Y = 35;
        }
        else if (bTwoSlots)
        {
            UTexture* First;
            UTexture* Second;
            if (Item->Unknown30C[0])
            {
                First = Item->GetIcon2();
                Second = Item->GetIcon3();
            }
            else if (Item->Unknown20C[0])
            {
                First = Item->GetIcon2();
                Second = Item->GetIcon2();
            }
            else
            {
                First = Item->GetIcon();
                Second = Item->GetIcon();
            }
            if (Slot == 15)
            {
                Device->DrawTexture(0, 35, CellSize, CellSize, 0, 0, CellSize, CellSize, First);
                Device->DrawTexture(70, 35, CellSize, CellSize, 0, 0, CellSize, CellSize, Second);
                Device->DrawTexture(70, 35, CellSize, CellSize, 0, 0, CellSize, CellSize, DualCapTex);
                if (Item->IsSelected())
                    Device->DrawTexture(-1, 34, 34, 34, 0, 0, 34, 34, SelectTex);
            }
            else if (Slot == 14)
            {
                Device->DrawTexture(35, 35, CellSize, CellSize, 0, 0, CellSize, CellSize, First);
                Device->DrawTexture(35, 70, CellSize, CellSize, 0, 0, CellSize, CellSize, Second);
                Device->DrawTexture(35, 70, CellSize, CellSize, 0, 0, CellSize, CellSize, DualCapTex);
                if (Item->IsSelected())
                    Device->DrawTexture(34, 34, 34, 34, 0, 0, 34, 34, SelectTex);
            }
            continue;
        }
        Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon());
        if (Item->IsSelected())
            Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, SelectTex);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100642d0, NCEquipItemWnd, OnPaint)

// 0x100646e0 (vslot 51) - NCItemWnd's tool tips for the item of the slot under the mouse
// (the box's strips counted in text lines, the name tip 168 pixels wide at most); the tips
// start from a grid of CellSize + 4 pixels, not the slots' places. Out of the slots it leaves
// the clip and the origin set, as in the stock code.
INT NCEquipItemWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    INT X = 0;
    INT Y = 0;
    if (HoverSlot < 0 || HoverSlot > NumSlots)
        return 1;
    if (HoverSlot < 5)
    {
        X = (CellSize + 4) * HoverSlot;
        Y = 4;
    }
    else if (HoverSlot == 5)
        Y = CellSize + 4;
    else if (HoverSlot < 8)
    {
        Y = CellSize + 4;
        X = (HoverSlot - 5) * Y;
    }
    else if (HoverSlot == 8)
    {
        Y = CellSize + 4;
        X = 3 * Y + 19;
    }
    else if (HoverSlot == 9)
        Y = 2 * CellSize + 8;
    else if (HoverSlot <= 13)
    {
        X = (HoverSlot - 9) * (CellSize + 4);
        Y = 2 * (CellSize + 4);
    }
    for (INT i = 0; i < Items.Num(); i++)
    {
        INT Slot = EquipSlotOf(Items(i));
        if (HoverSlot != Slot && !(Slot == 15 && HoverSlot == 5) && !(Slot == 14 && HoverSlot == 6))
            continue;
        if (Items(i)->IsSelected())
        {
            if (!Items(i)->GetToolTipText())
                continue;
            SIZE Size;
            SIZE Char;
            NCGDevice::GetTextExtent(&Size, Items(i)->GetToolTipText(), 168, 0, 0, 0, 0);
            NCGDevice::GetCharSize(&Char, 'A');
            INT TextH = Size.cy + Size.cy / Char.cy - 1;
            INT ScreenW = GConsole->Desktop->LocalW;
            if (ScreenX + X + 184 > ScreenW)
                X = ScreenW - ScreenX - 184;
            else if (ScreenX + X < 0)
                X = -ScreenX;
            Y += -15 - TextH;
            if (ScreenY + Y < 0)
                Y += CellSize + TextH + 16;
            INT Lines = TextH / Char.cy;
            Device->SetClip(ScreenX + X, ScreenY + Y, 184, Lines * Char.cy + 16);
            if (TipTopTex)
                Device->DrawTexture(X, Y, 184, 8, 0, 0, 184, 8, TipTopTex);
            for (INT Line = 0; Line < Lines; Line++)
                if (TipMidTex)
                    Device->DrawTexture(X, Line * Char.cy + Y + 8, 184, Char.cy, 0, 0, 184, Char.cy,
                                        TipMidTex);
            if (TipBottomTex)
                Device->DrawTexture(X, Lines * Char.cy + Y + 8, 184, 8, 0, 0, 184, 8, TipBottomTex);
            DrawItemToolTip(Device, Items(i), X + 8, Y + 8);
            Device->ReleaseClip();
        }
        else if (Items(i)->GetToolTipName())
#if NW_FEATURES
        // phase 6: ItemHoverTip: the recent chronicles' name tip (a dark box, the grade symbol);
        // with [ItemTip] HoverStyle=0 the stock strip below
        if (nwf::ItemHoverTip_Draw(this, Device, Items(i), X, Y, CellSize, 168))
            break;
        else
#endif
        {
            SIZE Size;
            NCGDevice::GetTextExtent(&Size, Items(i)->GetToolTipName(), 168, 0, 0, 0, 0);
            INT ScreenW = GConsole->Desktop->LocalW;
            if (Size.cx + ScreenX + X > ScreenW)
                X = ScreenW - ScreenX - Size.cx - 3;
            else if (ScreenX + X < 0)
                X -= ScreenX;
            Y -= Size.cy;
            if (ScreenY + Y < 0)
                Y += CellSize + Size.cy + 1;
            Device->SetClip(ScreenX + X, ScreenY + Y, Size.cx, Size.cy);
            if (ToolTipTex)
                Device->DrawStretchedTex(X, Y, Size.cx, Size.cy, ToolTipTex);
            Device->DrawText(X, Y, 0xFFFFFFFF, Items(i)->GetToolTipName(), 0, 0, 0, L2FONT_Normal, 0, 0,
                             0, 0, 0, 0);
            Device->ReleaseClip();
            break;
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x100646e0, NCEquipItemWnd, OnDrawToolTip)

// 0x10064b40
void NCEquipItemWnd::UnselectAll()
{
    for (INT i = 0; i < Items.Num(); i++)
        Items(i)->Unselect();
}
NW_HOOK_METHOD(0x10064b40, NCEquipItemWnd, UnselectAll)

// 0x10064b70 - the slot's 32 x 32 pixels (whatever CellSize); out of every slot the answer
// is 0, the first slot, as in the stock code
INT NCEquipItemWnd::HitTest(INT X, INT Y)
{
    for (INT Slot = 0; Slot < 14; Slot++)
        if (PtInRect(X, Y, ScreenX + SlotX[Slot], ScreenY + SlotY[Slot], 32, 32))
            return Slot;
    return 0;
}
NW_HOOK_METHOD(0x10064b70, NCEquipItemWnd, HitTest)

// 0x10064c70 (vslot 63) - NCInvenItemWnd::OnDialogResult for the item of the selected slot
INT NCEquipItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    INT i;
    for (i = 0; i < Items.Num(); i++)
        if (IsInSlot(EquipSlotOf(Items(i)), Selected))
            break;
    if (i < Items.Num())
    {
        NItemRequest Request;
        Request.ObjectID = Items(i)->ObjectID;
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
NW_HOOK_METHOD(0x10064c70, NCEquipItemWnd, OnDialogResult)

// 0x10064ec0 - the item with Info's object id takes Info's fields and new tool tips (the
// other two icons are not looked up again); the entry built for that is never freed, as in
// the stock code
UBOOL NCEquipItemWnd::UpdateItem(NItemInfo* Info)
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
NW_HOOK_METHOD(0x10064ec0, NCEquipItemWnd, UpdateItem)

// 0x10064fc0
UBOOL NCEquipItemWnd::HasItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
        if (ObjectID == Items(i)->ObjectID)
            return 1;
    return 0;
}
NW_HOOK_METHOD(0x10064fc0, NCEquipItemWnd, HasItem)

// 0x10065010
UBOOL NCEquipItemWnd::FindItem(INT ObjectID, INT* Index)
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
NW_HOOK_METHOD(0x10065010, NCEquipItemWnd, FindItem)

// 0x100661a0 (vslot 56) - selects the item of the slot under the mouse and picks it up
// (unless bNoDrag, which lasts one click); a two-slot item met on the way unselects all
INT NCEquipItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    guard(NCEquipItemWnd::OnLButtonDown);
    bDropPending = 0;
    bDestroyPending = 0;
    bCrystallizePending = 0;
    bPetPending = 0;
    INT Slot = HitTest(LOWORD(lParam), HIWORD(lParam));
    Selected = Slot;
    HoverSlot = Slot;
    CloseItemDialog();
    UBOOL bFound = 0;
    for (INT i = 0; i < Items.Num(); i++)
    {
        INT ItemSlot = EquipSlotOf(Items(i));
        if (ItemSlot == 15)
        {
            UnselectAll();
            if (Selected != 5)
                continue;
        }
        else if (ItemSlot == 14)
        {
            UnselectAll();
            if (Selected != 6)
                continue;
        }
        else if (Selected == ItemSlot)
            UnselectAll();
        else
            continue;
        Items(i)->Select();
        SelectedIndex = i;
        bFound = 1;
        break;
    }
    if (!bFound)
        SelectedIndex = -1;
    if (!bNoDrag)
    {
        bDragging = 1;
        GConsole->Engine->OnInterfacePlaySound(SOUND_PickUp);
    }
    else
        bNoDrag = 0;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100661a0, NCEquipItemWnd, OnLButtonDown)

// 0x100663b0 (vslot 55) - NCInvenItemWnd::OnMouseMove for the paperdoll
INT NCEquipItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    HoverSlot = HitTest(X, Y);
    if (IsFocused())
    {
        if (bDragging)
        {
            if (Selected >= 0 && Selected < NumSlots && SelectedIndex < Items.Num() && SelectedIndex >= 0)
            {
                SetCursor(4, Items(SelectedIndex)->GetIcon());
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
NW_HOOK_METHOD(0x100663b0, NCEquipItemWnd, OnMouseMove)

// 0x10066530 (vslot 57) - where the carried item is put down (see the header): on an item
// page it is taken off (the ring from one finger only); the name buffers are the stock
// frame's 64 characters
INT NCEquipItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT Y = HIWORD(lParam);
    if (!GConsole->IsCtrlDown() && !bEnchantMode)
    {
        UnlockCursor();
        SetCursor(0, NULL);
    }
    if (Selected < 0 || Selected >= NumSlots)
    {
        bDragging = 0;
        return 1;
    }
    INT Index;
    for (Index = 0; Index < Items.Num(); Index++)
        if (IsInSlot(EquipSlotOf(Items(Index)), Selected))
            break;
    NItemEntry* Item = NULL;
    if (Index == Items.Num() || !bDragging || (Item = Items(Index)) == NULL)
    {
        bDragging = 0;
        return 1;
    }
    if (bEnchantMode)
    {
        GConsole->RequestEnchantItem(Item->ObjectID);
        bDragging = 0;
        return 1;
    }
    INT X = LOWORD(lParam);
    NWndEntry* E = FindWindowAt(X, Y);
    NCWnd* Wnd = E ? E->Wnd : NULL;
    TCHAR Name[64];
    TCHAR DropName[64];
    if (!Wnd)
        ;
    else if (Wnd->IsA(NCInvenItemWnd::StaticClass()))
    {
        // taken off
        if (Item->Unknown1C98)
        {
            bDragging = 0;
            return 1;
        }
        L2ParamStack Params(10);
        INT BodyPart = Items(Index)->BodyPart;
        if (BodyPart & 6)
        {
            User* Me = GConsole->GetMyUser();
            if (Me->Equipped[1] == Items(Index)->ObjectID)
                Params.PushBack((void*)2);
            if (Me->Equipped[2] == Items(Index)->ObjectID)
                Params.PushBack((void*)4);
        }
        else if (BodyPart & 0x30)
        {
            User* Me = GConsole->GetMyUser();
            if (Me->Equipped[4] == Items(Index)->ObjectID)
                Params.PushBack((void*)0x10);
            else if (Me->Equipped[5] == Items(Index)->ObjectID)
                Params.PushBack((void*)0x20);
        }
        else
            Params.PushBack((void*)(INT_PTR)BodyPart);
        GConsole->RequestUnEquipItem(Params);
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
            Params.PushBack((void*)(INT_PTR)Items(Index)->ObjectID);
            Params.PushBack((void*)1);
            GConsole->RequestShortCutReg(Params);
        }
        UnselectAll();
        Selected = -1;
    }
    else if (Wnd->IsA(NClass::NCPetInvenItemWnd()))
    {
        bPetPending = 1;
        if (Item->IsStackable() && Item->Count > 1)
        {
            appSprintf(Name, TEXT("%s"), Item->GetName());
            NDialogInfo Info;
            Info.Type = 3;
            Info.bInput = 1;
            Info.bMainBox = 1;
            Info.ID = DIALOG_GIVE_TO_PET;
            Info.Owner = Entry;
            Info.SetSystemMessage(Item->ID == ITEM_ADENA ? MSG_PetAdenaHowMuch : MSG_PetHowMuch);
            Info.SetButtons(Name, NULL);
            Info.bLargeAmount = Item->ID == ITEM_ADENA;
            GConsole->ShowDialog(&Info);
        }
        else
        {
            // the slot taken for the item's index, as in the stock code
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
                Info.Type = 3;
                Info.bInput = 1;
                Info.bMainBox = 1;
                Info.ID = DIALOG_DESTROY_ITEM;
                Info.Owner = Entry;
                Info.SetSystemMessage(MSG_DestroyHowMany);
                Info.SetButtons(Name, NULL);
                Info.bLargeAmount = Item->ID == ITEM_ADENA;
                GConsole->ShowDialog(&Info);
            }
            else
            {
                appSprintf(Name, TEXT("%s"), Item->GetName());
                NDialogInfo Info;
                Info.bMainBox = 1;
                Info.ID = DIALOG_DESTROY_ITEM;
                Info.Owner = Entry;
                Info.bInput = 0;
                Info.SetSystemMessage(MSG_Destroy);
                Info.SetButtons(Name, NULL);
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
                appSprintf(Name, TEXT("%s"), Item->GetName());
                NDialogInfo Info;
                Info.bMainBox = 1;
                Info.ID = DIALOG_CRYSTALLIZE_ITEM;
                Info.bInput = 0;
                Info.Owner = Entry;
                Info.SetSystemMessage(MSG_Crystallize);
                Info.SetButtons(Name, NULL);
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
            if (Item->IsStackable() && Item->Count > 1)
            {
                NDialogInfo Info;
                Info.Type = 3;
                Info.bInput = 1;
                Info.bMainBox = 1;
                Info.ID = DIALOG_DROP_ITEM;
                Info.Owner = Entry;
                Info.SetSystemMessage(MSG_DropHowMany);
                Info.SetButtons(DropName, NULL);
                Info.bLargeAmount = Item->ID == ITEM_ADENA;
                GConsole->ShowDialog(&Info);
            }
            else
            {
                NDialogInfo Info;
                Info.ID = DIALOG_DROP_ITEM;
                Info.bMainBox = 1;
                Info.bInput = 0;
                Info.Owner = Entry;
                Info.SetSystemMessage(MSG_Drop);
                Info.SetButtons(DropName, NULL);
                Info.bLargeAmount = 0;
                GConsole->ShowDialog(&Info);
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10066530, NCEquipItemWnd, OnLButtonUp)

// 0x10066e80 (vslot 58) - takes off the item of the slot under the mouse; nothing is taken
// off once an item with Unknown1C98 comes first in the list
INT NCEquipItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    CloseItemDialog();
    if (Selected >= 0 && Selected < NumSlots)
    {
        for (INT i = 0; i < Items.Num(); i++)
        {
            NItemEntry* Item = Items(i);
            if (Item && Item->Unknown1C98)
                break;
            if (UnEquipIfInSlot(Item, Selected))
                break;
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10066e80, NCEquipItemWnd, OnRButtonDown)

// 0x10067140 (vslot 62) - takes off the item of the slot under the mouse; the release that
// follows moves nothing
INT NCEquipItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < NumSlots)
    {
        for (INT i = 0; i < Items.Num(); i++)
            if (UnEquipIfInSlot(Items(i), Selected))
                break;
    }
    UnselectAll();
    Selected = -1;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10067140, NCEquipItemWnd, OnLButtonDblClick)

// 0x100673a0 (vslot 36)
INT NCEquipItemWnd::OnDestroy()
{
    for (INT i = 0; i < Items.Num(); i++)
        if (Items(i))
            delete Items(i);
    Items.Empty();
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100673a0, NCEquipItemWnd, OnDestroy)

// 0x10067410 - the three icons come from their names ("NWindow.BlackTexture" for those
// missing), written back into Info; 0 when the fourteen slots are taken. Every add checks
// again whether an item with Unknown1C98 is equipped.
UBOOL NCEquipItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= NumSlots)
        return 0;
    if (Info->IconName[0])
        Info->Icon = FindTexture(Info->IconName);
    if (Info->Unknown200[0])
        Info->UnknownC04 = (INT)(INT_PTR)FindTexture(Info->Unknown200);
    if (Info->Unknown300[0])
        Info->UnknownC08 = (INT)(INT_PTR)FindTexture(Info->Unknown300);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    if (!Info->UnknownC04)
        Info->UnknownC04 = (INT)(INT_PTR)FindTexture(TEXT("NWindow.BlackTexture"));
    if (!Info->UnknownC08)
        Info->UnknownC08 = (INT)(INT_PTR)FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = new NItemEntry(Info);
    SetToolTip(Item);
    Items.AddItem(Item);
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (Items(i)->Unknown1C98)
        {
            bHasUnknown1C98 = 1;
            break;
        }
        bHasUnknown1C98 = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x10067410, NCEquipItemWnd, AddItem)

// 0x100675c0 - bHasUnknown1C98 stays as it was
UBOOL NCEquipItemWnd::RemoveItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (Items(i)->ObjectID == ObjectID)
        {
            NItemEntry* Item = Items(i);
            Items.Remove(i);
            if (Item)
                delete Item;
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x100675c0, NCEquipItemWnd, RemoveItem)

// 0x10067630
void NCEquipItemWnd::Clear()
{
    for (INT i = 0; i < Items.Num(); i++)
        if (Items(i))
            delete Items(i);
    Items.Empty();
}
NW_HOOK_METHOD(0x10067630, NCEquipItemWnd, Clear)

// stock body shared with NCEditBox's (0x100a5d70)
void NCEquipItemWnd::ShowWindow()
{
    NCWnd::ShowWindow();
}

// stock body shared with NCChatWnd's (0x1003dab0)
void NCEquipItemWnd::HideWindow()
{
    NCWnd::HideWindow();
}

NW_CHECK_VTABLE(NCEquipItemWnd, 0x100e4a70, 65)
