/*=============================================================================
    NCPetInventoryWnd.cpp - the pet window's Items page and its item grid.
    Stock: 0x1008eea0 .. 0x1008ef50, 0x1008ef60 .. 0x10090b4c, 0x10091830 .. 0x10091d06,
    0x10091d10 .. 0x10091f6f, 0x10091f70 .. 0x10091fa0, 0x10093a10 .. 0x10093e45,
    0x10093e50 .. 0x100940a6, 0x100941c0 .. 0x1009436d (ClearItems: a copy of the body
    NCTrainListWnd keeps the address of, 0x100bab50).
=============================================================================*/
#include "nwindow/NCPetInventoryWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NItemToolTip.h"

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_ItemPick = 3 };

enum { DIALOG_PET_ITEM_TO_OWNER = 0x33 };  // NDialogInfo::ID (name ours)
enum { ITEM_Adena = 57 };

// a system line in the desktop's chat window (NCConsole::AddSystemText): Text when not empty
static FORCEINLINE void DesktopChatMessage(const TCHAR* Text, INT P2, INT P3)
{
    GDesktop->AddSystemText(Text, P2, P3);
}

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

/*-----------------------------------------------------------------------------
    NCPetInvenItemWnd.
-----------------------------------------------------------------------------*/

// 0x100941c0 - EquippedTex and DropLocation are left as allocated
NCPetInvenItemWnd::NCPetInvenItemWnd()
{
    bDropPending = 0;
    bToOwnerPending = 0;
    bDblClicked = 0;
}
NW_HOOK_CTOR(0x100941c0, NCPetInvenItemWnd)

// 0x10094270 - the pet inventory's is 6 x 2 icons of 32 pixels, 160 at most
NCPetInvenItemWnd::NCPetInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems,
                                     INT InCellSize)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
    bDropPending = 0;
    bToOwnerPending = 0;
    bDblClicked = 0;
}
NW_HOOK_CTOR_ARGS(0x10094270, NCPetInvenItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x10094220
NW_HOOK_DTOR(0x10094220, NCPetInvenItemWnd)

// 0x1008eea0 (vslot 24) - the dialog box's answer and the double click
INT NCPetInvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_DIALOG_RESULT)
        return OnDialogResult(wParam, lParam) != 0;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1008eea0, NCPetInvenItemWnd, WindowProc)

// 0x1008ef30 (vslot 35)
INT NCPetInvenItemWnd::OnCreate()
{
    NCItemWnd::OnCreate();
    EquippedTex = FindTexture(TEXT("l2ui.PetWnd.petitem_click"));
    return 1;
}
NW_HOOK_METHOD(0x1008ef30, NCPetInvenItemWnd, OnCreate)

// 0x1008ef60 - the inventory's tool tip texts, with the count
void NCPetInvenItemWnd::SetToolTip(NItemEntry* Item)
{
    ItemTip::SetTexts<128, 0>(this, Item);
}
NW_HOOK_METHOD(0x1008ef60, NCPetInvenItemWnd, SetToolTip)

// 0x1008f650 (vslot 67) - the inventory's tool tip box. The stock frame gives the text buffer
// 128 characters and prints the description into it too: a longer description overruns the
// two FStrings above it (heap corruption); here the buffer has 1024, as the player's
// inventory copy, so the /GS check does not end the game (same drawing when it fits)
void NCPetInvenItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
    ItemTip::DrawLines<1024, 0, 128>(this, Device, Item, X, Y);
}
NW_HOOK_METHOD(0x1008f650, NCPetInvenItemWnd, DrawItemToolTip)

// 0x10091830 (vslot 50) - NCItemWnd's icons with a frame behind the equipped items (and the
// selected one); no guard in the stock code
INT NCPetInvenItemWnd::OnPaint(NCGDevice* Device)
{
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
        if (Item->bEquipped)
            Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, EquippedTex);
        if (Item->IsSelected())
            Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, SelectTex);
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon());
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10091830, NCPetInvenItemWnd, OnPaint)

// 0x100919e0 (vslot 56) - selects the item and picks it up (unless a double click just
// happened)
INT NCPetInvenItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    Selected = HoverIndex = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
    {
        UnselectAll();
        Items(Selected)->Select();
        if (!bNoDrag)
        {
            bDragging = 1;
            GConsole->Engine->OnInterfacePlaySound(SOUND_ItemPick);
            return 1;
        }
        bNoDrag = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x100919e0, NCPetInvenItemWnd, OnLButtonDown)

// 0x10091a80 (vslot 68) - the pet uses the item
INT NCPetInvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    bDragging = 0;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
        GConsole->RequestPetUseItem(Items(Selected)->ObjectID);
    bDblClicked = 1;
    UnselectAll();
    Selected = -1;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10091a80, NCPetInvenItemWnd, OnLButtonDblClick)

// 0x10091b10 (vslot 58) - the pet uses the item (the selection stays)
INT NCPetInvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
        GConsole->RequestPetUseItem(Items(Selected)->ObjectID);
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10091b10, NCPetInvenItemWnd, OnRButtonDown)

// 0x10091d10 (vslot 69) - the answer (wParam: the amount) to the question asked last: back
// to the player, or onto the ground (systemmsg 179 in the chat instead when the player's
// pawn has its flag +0x690 bit 0 set)
INT NCPetInvenItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    if (Selected >= 0 && Selected < Items.Num())
    {
        if (bToOwnerPending)
        {
            L2ParamStack Params(10);
            Params.PushBack((void*)(INT_PTR)Items(Selected)->ObjectID);
            Params.PushBack((void*)(INT_PTR)wParam);
            Params.PushBack((void*)0);
            GConsole->RequestGetItemFromPet(Params);
            bToOwnerPending = 0;
        }
        if (bDropPending)
        {
            NItemRequest Request;
            Request.ObjectID = Items(Selected)->ObjectID;
            Request.Count = (INT)wParam;
            if (GConsole->GetMyUser()->Pawn->Unknown690 & 1)
                DesktopChatMessage(*GL2GameData.SystemMsg[179].Message, 0, -1);
            else
                NRequestDropItem(Request, DropLocation);
            bDropPending = 0;
        }
    }
    UnselectAll();
    Selected = -1;
    return 1;
}
NW_HOOK_METHOD(0x10091d10, NCPetInvenItemWnd, OnDialogResult)

// 0x10093a10 (vslot 57) - the carried item is put down: inside the grid on another place (past
// the last item: at the end), onto the player's inventory or paperdoll back to the player (how
// many first, for a stack; the adena's question allows large amounts), onto the shortcut bar
// a shortcut of the pet's item. The name buffer is the stock frame's 64 characters.
INT NCPetInvenItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (!GConsole->IsCtrlDown())
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
        if (IsInWindow(X, Y))
        {
            INT Target = HitTest(X, Y);
            if (Target != Selected && Target >= 0 && Selected >= 0 && Selected < Items.Num())
            {
                if (Target >= Items.Num())
                {
                    Items.Remove(Selected, 1);
                    Items.AddItem(Item);
                    Selected = Items.Num() - 1;
                }
                else
                {
                    Items.Remove(Selected, 1);
                    Items.Insert(Target, 1);
                    Items(Target) = Item;
                }
            }
        }
        else
        {
            NWndEntry* E = FindWindowAt(X, Y);
            if (E && E->Wnd)
            {
                if (E->Wnd->IsA(NClass::NCInvenItemWnd()) || E->Wnd->IsA(NClass::NCEquipItemWnd()))
                {
                    bToOwnerPending = 1;
                    if (Item->IsStackable() && Item->Count > 1)
                    {
                        TCHAR Name[64];
                        appSprintf(Name, TEXT("%s"), Item->GetName());
                        NDialogInfo Info;
                        Info.Owner = Entry;
                        Info.Type = 3;
                        Info.bInput = 1;
                        Info.bMainBox = 1;
                        Info.ID = DIALOG_PET_ITEM_TO_OWNER;
                        if (Item->ID == ITEM_Adena)
                        {
                            Info.SetSystemMessage(0x218);
                            Info.SetButtons(Name, NULL);
                            Info.bLargeAmount = 1;
                        }
                        else
                        {
                            Info.SetSystemMessage(0x219);
                            Info.SetButtons(Name, NULL);
                            Info.bLargeAmount = 0;
                        }
                        GConsole->ShowDialog(&Info);
                    }
                    else
                    {
                        L2ParamStack Params(10);
                        Params.PushBack((void*)(INT_PTR)Items(Selected)->ObjectID);
                        Params.PushBack((void*)1);
                        Params.PushBack((void*)0);
                        GConsole->RequestGetItemFromPet(Params);
                    }
                }
                else if (E->Wnd->IsA(NClass::NCShortCutWnd()))
                {
                    INT Slot = ((NCShortCutWnd*)E->Wnd)->SlotAt(X, Y);
                    if (Slot >= 0)
                    {
                        L2ParamStack Params(10);
                        Params.PushBack((void*)1);                // an item
                        Params.PushBack((void*)(INT_PTR)Slot);
                        Params.PushBack((void*)(INT_PTR)Items(Selected)->ObjectID);
                        Params.PushBack((void*)2);                // the pet's
                        GConsole->RequestShortCutReg(Params);
                    }
                    UnselectAll();
                    Selected = -1;
                }
            }
        }
    }
    bDblClicked = 0;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10093a10, NCPetInvenItemWnd, OnLButtonUp)

// 0x10093fb0 (vslot 65) - frees the items; unlike NCItemWnd's, the scroll position and the
// selection stay
void NCPetInvenItemWnd::Clear()
{
    for (INT i = 0; i < Items.Num(); i++)
        if (Items(i))
            delete Items(i);
    Items.Empty();
}
NW_HOOK_METHOD(0x10093fb0, NCPetInvenItemWnd, Clear)

// 0x10093e50 - the icon comes from its name ("NWindow.BlackTexture" when missing) and is
// written back into Info; the scroll range is not updated here
UBOOL NCPetInvenItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= MaxItems)
        return 0;
    TCHAR* IconName = Info->IconName;
    if (IconName)
        Info->Icon = FindTexture(IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = new NItemEntry(Info);
    SetToolTip(Item);
    Items.AddItem(Item);
    return 1;
}
NW_HOOK_METHOD(0x10093e50, NCPetInvenItemWnd, AddItem)

// 0x10091b70 - the entry with the same object id takes Info's values (through a new entry
// that is never freed, as in the stock code)
UBOOL NCPetInvenItemWnd::UpdateItem(NItemInfo* Info)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Old = Items(i);
        if (Info->ObjectID == Old->ObjectID)
        {
            TCHAR* IconName = Info->IconName;
            if (IconName)
                Info->Icon = FindTexture(IconName);
            if (!Info->Icon)
                Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
            NItemEntry* New = new NItemEntry(Info);
            SetToolTip(New);
            Old->Copy(New);
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x10091b70, NCPetInvenItemWnd, UpdateItem)

// 0x10093f40 - one body with NCTradeItemWnd's
UBOOL NCPetInvenItemWnd::RemoveItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (Items(i)->ObjectID == ObjectID)
        {
            NItemEntry* Item = Items(i);
            Items.Remove(i, 1);
            if (Item)
                delete Item;
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x10093f40, NCPetInvenItemWnd, RemoveItem)

// 0x10091c70 (NCInvenItemWnd has a copy)
UBOOL NCPetInvenItemWnd::HasItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
        if (ObjectID == Items(i)->ObjectID)
            return 1;
    return 0;
}
NW_HOOK_METHOD(0x10091c70, NCPetInvenItemWnd, HasItem)

// 0x10091cc0 (NCInvenItemWnd has a copy)
UBOOL NCPetInvenItemWnd::FindItem(INT ObjectID, INT* Index)
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
NW_HOOK_METHOD(0x10091cc0, NCPetInvenItemWnd, FindItem)

NW_CHECK_VTABLE(NCPetInvenItemWnd, 0x100ec488, 70)

/*-----------------------------------------------------------------------------
    NCPetInventoryWnd.
-----------------------------------------------------------------------------*/

// 0x10091ed0
NCPetInventoryWnd::NCPetInventoryWnd()
{
}
NW_HOOK_CTOR(0x10091ed0, NCPetInventoryWnd)

// 0x10091f20
NW_HOOK_DTOR(0x10091f20, NCPetInventoryWnd)

// stock body shared with NControl's (0x10004e90)
INT NCPetInventoryWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100942b0 (vslot 35) - the grid: 6 x 2 icons of 32 pixels, 160 items at most
INT NCPetInventoryWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.PetWnd.pet03"));
    Items = new NCPetInvenItemWnd(6, 2, 160, 32);
    Items->SetWindowRect(16, 29, 223, 69);
    CreateChild(Items, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    return 1;
}
NW_HOOK_METHOD(0x100942b0, NCPetInventoryWnd, OnCreate)

// stock body shared with NCPetActionWnd's (0x100bb540)
INT NCPetInventoryWnd::OnDestroy()
{
    DestroyChild(this, Items);
    NCWnd::OnDestroy();
    return 1;
}

// stock body shared with NCPetActionWnd's (0x10091680)
INT NCPetInventoryWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}

// 0x10094010
UBOOL NCPetInventoryWnd::AddItem(NItemInfo* Info)
{
    Items->AddItem(Info);
    return 1;
}
NW_HOOK_METHOD(0x10094010, NCPetInventoryWnd, AddItem)

// 0x10094030 - the server's change of one item: 1 added, 2 changed (when here), 3 removed
UBOOL NCPetInventoryWnd::UpdateItem(INT Op, NItemInfo* Info)
{
    if (Op == 1)
    {
        Items->AddItem(Info);
        return 1;
    }
    if (Op == 2)
    {
        if (Items->HasItem(Info->ObjectID))
            Items->UpdateItem(Info);
        return 1;
    }
    if (Op == 3)
        Items->RemoveItem(Info->ObjectID);
    return 1;
}
NW_HOOK_METHOD(0x10094030, NCPetInventoryWnd, UpdateItem)

// 0x10091f70 - the pet's item with that object id (the shortcut bar and the console)
NItemEntry* NCPetInventoryWnd::GetItem(INT ObjectID)
{
    INT Index;
    if (Items->FindItem(ObjectID, &Index))
        return Items->Items(Index);
    return NULL;
}
NW_HOOK_METHOD(0x10091f70, NCPetInventoryWnd, GetItem)

// stock body shared with NCTrainListWnd's Clear (0x100bab50)
void NCPetInventoryWnd::ClearItems()
{
    Items->Clear();
}

NW_CHECK_VTABLE(NCPetInventoryWnd, 0x100ebe00, 62)
