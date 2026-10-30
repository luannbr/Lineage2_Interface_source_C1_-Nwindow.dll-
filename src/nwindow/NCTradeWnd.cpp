/*=============================================================================
    NCTradeWnd.cpp - the trade window and its item grids.
    Stock: 0x100b72f0 .. 0x100b9152, 0x100b9170 .. 0x100ba99e; 0x10057ba0 (SetKind, shared).
    NCTradeItemWnd's WindowProc (0x10057b10) and RemoveItem (0x10093f40) are copies of bodies
    NCInvenItemWnd and NCPetInvenItemWnd keep the addresses of.
=============================================================================*/
#include "nwindow/NCTradeWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NItemToolTip.h"

// NDialogInfo::ID (names ours): both ask the inventory grid for an amount (NCDialogBox::OnOK)
enum
{
    DIALOG_TRADE_ADD = 2,            // the down arrow's question
    DIALOG_TRADE_MOVE = 8,           // the grid's (item carried onto the offer, double click)
};

enum { MSG_HowManyToMove = 72 };     // systemmsg "How many $s1(s) do you want to move?"
enum { ITEM_Adena = 57 };

// sysstring labels
enum
{
    STR_OK = 140,
    STR_Cancel = 141,
};

// NConsoleWnd::RequestAddTradeItem (0x10043e60, guard name): the item offered, by value
static FORCEINLINE void RequestAddTradeItem(NItemRequest Request)
{
    GConsole->RequestAddTradeItem(Request);
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
    NCTradeItemWnd.
-----------------------------------------------------------------------------*/

// 0x100ba410
NCTradeItemWnd::NCTradeItemWnd()
{
}
NW_HOOK_CTOR(0x100ba410, NCTradeItemWnd)

// 0x100ba4b0 - the trade window's grids are 6 icons of 32 pixels wide, 80 items at most
NCTradeItemWnd::NCTradeItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
}
NW_HOOK_CTOR_ARGS(0x100ba4b0, NCTradeItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x100ba460
NW_HOOK_DTOR(0x100ba460, NCTradeItemWnd)

// stock body shared with NCInvenItemWnd's (0x10057b10): the double click and the dialog box's
// answer
INT NCTradeItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
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

// 0x10057ba0 - one body with the setters of the shop windows' grids (NCShopWnd, NCStoreWnd,
// NCPrivateShopWnd and NCDeliverWnd call it on theirs, which also keep a kind at +0x118)
void NCTradeItemWnd::SetKind(INT InKind)
{
    Kind = InKind;
}
NW_HOOK_METHOD(0x10057ba0, NCTradeItemWnd, SetKind)

// 0x100b72f0 - the inventory's tool tip texts, with the amount offered
void NCTradeItemWnd::SetToolTip(NItemEntry* Item)
{
    ItemTip::SetTexts<1024, 1>(this, Item);
}
NW_HOOK_METHOD(0x100b72f0, NCTradeItemWnd, SetToolTip)

// 0x100b79e0 (vslot 67) - the inventory's tool tip box, with the amount offered
void NCTradeItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
    ItemTip::DrawLines<1024, 1>(this, Device, Item, X, Y);
}
NW_HOOK_METHOD(0x100b79e0, NCTradeItemWnd, DrawItemToolTip)

// 0x100b9330 (vslot 50) - NCItemWnd's icons; only the inventory grid shows the selection. No
// guard in the stock code.
INT NCTradeItemWnd::OnPaint(NCGDevice* Device)
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
        if (!Kind && Item->IsSelected())
            Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, SelectTex);
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon());
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100b9330, NCTradeItemWnd, OnPaint)

// 0x100b94c0 (vslot 55) - NCItemWnd's (the carried item as the cursor), in the inventory grid
// only
INT NCTradeItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    HoverIndex = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (!Kind && IsFocused())
    {
        if (bDragging)
        {
            if (Selected >= 0 && Selected < Items.Num())
            {
                SetCursor(4, Items(Selected)->GetIcon());
                LockCursor();
                return 1;
            }
        }
        else if (!GConsole->IsCtrlDown())
        {
            UnlockCursor();
            SetCursor(0, NULL);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100b94c0, NCTradeItemWnd, OnMouseMove)

// 0x100b9570 (vslot 57) - an inventory item carried onto the player's offer grid is offered:
// how many first for a stack (the adena's question allows large amounts), else one. The name
// buffer is the stock frame's 64 characters.
INT NCTradeItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (Kind)
        return 1;
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
    if (bDragging && !IsInWindow(X, Y))
    {
        NWndEntry* E = FindWindowAt(X, Y);
        if (E && E->Wnd && E->Wnd->IsA(NCTradeItemWnd::StaticClass())
            && ((NCTradeItemWnd*)E->Wnd)->Kind == TRADEGRID_MyOffer)
        {
            NItemEntry* Item = Items(Selected);
            if (Item->IsStackable() && Item->TradeCount > 1)
            {
                TCHAR Name[64];
                appSprintf(Name, TEXT("%s"), Item->GetName());
                NDialogInfo Info;
                Info.Type = 3;
                Info.bInput = 1;
                Info.bMainBox = 1;
                Info.ID = DIALOG_TRADE_MOVE;
                Info.Owner = Entry;
                if (Item->ID == ITEM_Adena)
                {
                    Info.SetSystemMessage(MSG_HowManyToMove);
                    Info.SetButtons(Name, NULL);
                    Info.bLargeAmount = 1;
                }
                else
                {
                    Info.SetSystemMessage(MSG_HowManyToMove);
                    Info.SetButtons(Name, NULL);
                    Info.bLargeAmount = 0;
                }
                GConsole->ShowDialog(&Info);
                bDragging = 0;
                return 1;
            }
            NItemRequest Request;
            Request.ObjectID = Item->ObjectID;
            Request.Count = 1;
            RequestAddTradeItem(Request);
            UnselectAll();
            Selected = -1;
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100b9570, NCTradeItemWnd, OnLButtonUp)

// 0x100b97e0 (vslot 68) - the inventory item is offered: how many first for a stack (the
// systemmsg and the name copied into the box's texts directly), else one. In the offer grids
// nothing happens and the selection stays.
INT NCTradeItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    bDragging = 0;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
    {
        if (Kind)
        {
            bDragging = 0;
            return 1;
        }
        NItemEntry* Item = Items(Selected);
        if (Item->IsStackable() && Item->TradeCount > 1)
        {
            TCHAR Name[64];
            appSprintf(Name, TEXT("%s"), Item->GetName());
            NDialogInfo Info;
            Info.Type = 3;
            Info.bInput = 1;
            Info.bMainBox = 1;
            Info.ID = DIALOG_TRADE_MOVE;
            Info.Owner = Entry;
            if (Item->ID == ITEM_Adena)
            {
                appStrcpy(Info.Text, *GL2GameData.SystemMsg[MSG_HowManyToMove].Message);
                appStrcpy(Info.Button1, Name);
                Info.bLargeAmount = 1;
            }
            else
            {
                appStrcpy(Info.Text, *GL2GameData.SystemMsg[MSG_HowManyToMove].Message);
                appStrcpy(Info.Button1, Name);
                Info.bLargeAmount = 0;
            }
            GConsole->ShowDialog(&Info);
        }
        else
        {
            NItemRequest Request;
            Request.ObjectID = Item->ObjectID;
            Request.Count = 1;
            RequestAddTradeItem(Request);
            UnselectAll();
        }
    }
    Selected = -1;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100b97e0, NCTradeItemWnd, OnLButtonDblClick)

// 0x100b9ad0 (vslot 69) - the amount typed (wParam; at most the entry's count) of the selected
// item is offered. The stock code reads Items(Selected) without a bounds check.
INT NCTradeItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    if (Item && wParam)
    {
        INT Count = (INT)wParam;
        if (Count >= Item->TradeCount)
            Count = Item->TradeCount;
        NItemRequest Request;
        Request.ObjectID = Item->ObjectID;
        Request.Count = Count;
        RequestAddTradeItem(Request);
        UnselectAll();
        Selected = -1;
    }
    return 1;
}
NW_HOOK_METHOD(0x100b9ad0, NCTradeItemWnd, OnDialogResult)

// 0x100ba1d0 (vslot 65) - frees the items and drops the selection; unlike NCItemWnd's, the
// scroll position stays
void NCTradeItemWnd::Clear()
{
    for (INT i = 0; i < Items.Num(); i++)
        if (Items(i))
            delete Items(i);
    Items.Empty();
    Selected = -1;
}
NW_HOOK_METHOD(0x100ba1d0, NCTradeItemWnd, Clear)

// 0x100ba230 - an entry of the same object id that stacks gets the amount added (the full
// check comes first); otherwise a new entry, its icon from its name ("NWindow.BlackTexture"
// when missing, written back into Info), its count the amount
UBOOL NCTradeItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= MaxItems)
        return 0;
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Old = Items(i);
        if (Old->ObjectID == Info->ObjectID && Old->IsStackable())
        {
            Old->TradeCount += Info->Count;
            SetToolTip(Old);
            return 1;
        }
    }
    TCHAR* IconName = Info->IconName;
    if (IconName)
        Info->Icon = FindTexture(IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = new NItemEntry(Info);
    Item->TradeCount = Info->Count;
    SetToolTip(Item);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x100ba230, NCTradeItemWnd, AddItem)

// 0x100b99d0 - the entry with the same object id takes Info's values and amount (through a new
// entry that is never freed, as in the stock code)
UBOOL NCTradeItemWnd::UpdateItem(NItemInfo* Info)
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
            New->TradeCount = Info->Count;
            SetToolTip(New);
            Old->Copy(New);
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x100b99d0, NCTradeItemWnd, UpdateItem)

// stock body shared with NCPetInvenItemWnd's (0x10093f40)
UBOOL NCTradeItemWnd::RemoveItem(INT ObjectID)
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

NW_CHECK_VTABLE(NCTradeItemWnd, 0x100f0388, 70)

/*-----------------------------------------------------------------------------
    NCTradeWnd.
-----------------------------------------------------------------------------*/

// 0x100b9b90 - the names are left as allocated
NCTradeWnd::NCTradeWnd()
{
    InventoryItems = NULL;
    MyItems = NULL;
    PartnerItems = NULL;
    BackTex = NULL;
    AddButton = NULL;
    OKButton = NULL;
    CancelButton = NULL;
}
NW_HOOK_CTOR(0x100b9b90, NCTradeWnd)

// 0x100b9c00
NW_HOOK_DTOR(0x100b9c00, NCTradeWnd)

// 0x100b9170 (vslot 24) - the buttons
INT NCTradeWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_TRADE_OK)
            return OnOK() != 0;
        if (wParam == NID_TRADE_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_TRADE_ADD)
            return OnAdd() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100b9170, NCTradeWnd, WindowProc)

// 0x100ba4e0 (vslot 35) - title bar, the grids (the inventory 6 x 4 icons, the offers one row
// each), the down arrow between the inventory and the player's offer, OK and Cancel
INT NCTradeWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.storeWnd.TradeBack"));

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_TRADE_FRAME;
    FrameCtrl->ID = NID_TRADE_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    InventoryItems = new NCTradeItemWnd(6, 4, 80, 32);
    InventoryItems->SetWindowRect(16, 33, 220, 137);
    CreateChild(InventoryItems, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    InventoryItems->SetKind(TRADEGRID_Inventory);

    MyItems = new NCTradeItemWnd(6, 1, 80, 32);
    MyItems->SetWindowRect(16, 210, 220, 32);
    CreateChild(MyItems, NWS_VISIBLE | 0x20);
    MyItems->SetKind(TRADEGRID_MyOffer);

    PartnerItems = new NCTradeItemWnd(6, 1, 80, 32);
    PartnerItems->SetWindowRect(16, 282, 220, 32);
    CreateChild(PartnerItems, NWS_VISIBLE | 0x20);
    PartnerItems->SetKind(TRADEGRID_PartnerOffer);

    AddButton = new NCButton(TEXT("L2UI.StoreWnd.StoreDownBtn1"),
                             TEXT("L2UI.StoreWnd.StoreDownBtn2"), 0);
    AddButton->ID = NID_TRADE_ADD;
    AddButton->SetWindowRect(210, 183, 14, 13);
    CreateChild(AddButton, NWS_VISIBLE);

    OKButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                            TEXT("L2UI.Control.DefaultButton_click"), 0);
    OKButton->ID = NID_TRADE_OK;
    OKButton->SetWindowRect(85, 336, 67, 19);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(STR_OK);

    CancelButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    CancelButton->ID = NID_TRADE_CANCEL;
    CancelButton->SetWindowRect(157, 336, 67, 19);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(STR_Cancel);
    return 1;
}
NW_HOOK_METHOD(0x100ba4e0, NCTradeWnd, OnCreate)

// 0x100b9d50 (vslot 36)
INT NCTradeWnd::OnDestroy()
{
    DestroyChild(this, InventoryItems);
    DestroyChild(this, MyItems);
    DestroyChild(this, PartnerItems);
    DestroyChild(this, AddButton);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100b9d50, NCTradeWnd, OnDestroy)

// 0x100b9260 (vslot 42) - the grids' scroll ranges follow what they got while hidden
void NCTradeWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    InventoryItems->UpdateScrollRange();
    MyItems->UpdateScrollRange();
    PartnerItems->UpdateScrollRange();
}
NW_HOOK_METHOD(0x100b9260, NCTradeWnd, ShowWindow)

// stock body shared with NCChatWnd's (0x1003dab0)
void NCTradeWnd::HideWindow()
{
    NCWnd::HideWindow();
}

// 0x100b9c50 (vslot 50) - the background under the title bar and the two names in white
INT NCTradeWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 344, 0, 0, 240, 344, BackTex);
    if (*MyName)
        Device->DrawText(16, 185, 0xFFFFFFFF, MyName, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (*PartnerName)
        Device->DrawText(16, 256, 0xFFFFFFFF, PartnerName, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                         0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100b9c50, NCTradeWnd, OnPaint)

// 0x100b92f0 (vslot 62)
INT NCTradeWnd::OnOK()
{
    GConsole->RequestTradeDone(1);
    return 1;
}
NW_HOOK_METHOD(0x100b92f0, NCTradeWnd, OnOK)

// 0x100b9310 (vslot 63)
INT NCTradeWnd::OnCancel()
{
    GConsole->RequestTradeDone(0);
    return 1;
}
NW_HOOK_METHOD(0x100b9310, NCTradeWnd, OnCancel)

// 0x100b9e70 (vslot 64) - the down arrow offers the selected inventory item: how many first
// for a stack (the inventory grid asks and gets the answer), else one. The name buffer is the
// stock frame's 64 characters.
INT NCTradeWnd::OnAdd()
{
    INT Sel = InventoryItems->GetSelected();
    if (Sel < 0 || Sel >= InventoryItems->Items.Num())
        return 1;
    NItemEntry* Item = InventoryItems->Items(Sel);
    if (!Item)
        return 1;
    if (Item->IsStackable() && Item->TradeCount > 1)
    {
        TCHAR Name[64];
        appSprintf(Name, TEXT("%s"), Item->GetName());
        NDialogInfo Info;
        Info.Type = 3;
        Info.bInput = 1;
        Info.bMainBox = 1;
        Info.ID = DIALOG_TRADE_ADD;
        Info.Owner = InventoryItems->Entry;
        if (Item->ID == ITEM_Adena)
        {
            Info.SetSystemMessage(MSG_HowManyToMove);
            Info.SetButtons(Name, NULL);
            Info.bLargeAmount = 1;
        }
        else
        {
            Info.SetSystemMessage(MSG_HowManyToMove);
            Info.SetButtons(Name, NULL);
            Info.bLargeAmount = 0;
        }
        GConsole->ShowDialog(&Info);
        return 1;
    }
    NItemRequest Request;
    Request.ObjectID = Item->ObjectID;
    Request.Count = 1;
    RequestAddTradeItem(Request);
    return 1;
}
NW_HOOK_METHOD(0x100b9e70, NCTradeWnd, OnAdd)

// 0x100b9220 - a trade starts or ends: the grids emptied, the partner's name cleared (the
// player's is set again by the start)
void NCTradeWnd::Reset()
{
    InventoryItems->Clear();
    MyItems->Clear();
    PartnerItems->Clear();
    PartnerName[0] = 0;
}
NW_HOOK_METHOD(0x100b9220, NCTradeWnd, Reset)

// 0x100b9290
void NCTradeWnd::SetMyName(const TCHAR* Name)
{
    if (Name && *Name)
        appStrcpy(MyName, Name);
    else
        MyName[0] = 0;
}
NW_HOOK_METHOD(0x100b9290, NCTradeWnd, SetMyName)

// 0x100b92c0
void NCTradeWnd::SetPartnerName(const TCHAR* Name)
{
    if (Name && *Name)
        appStrcpy(PartnerName, Name);
    else
        PartnerName[0] = 0;
}
NW_HOOK_METHOD(0x100b92c0, NCTradeWnd, SetPartnerName)

// 0x100ba370
UBOOL NCTradeWnd::AddInventoryItem(NItemInfo* Info)
{
    return InventoryItems->AddItem(Info);
}
NW_HOOK_METHOD(0x100ba370, NCTradeWnd, AddInventoryItem)

// 0x100ba380
UBOOL NCTradeWnd::AddMyItem(NItemInfo* Info)
{
    return MyItems->AddItem(Info);
}
NW_HOOK_METHOD(0x100ba380, NCTradeWnd, AddMyItem)

// 0x100ba390
UBOOL NCTradeWnd::AddPartnerItem(NItemInfo* Info)
{
    return PartnerItems->AddItem(Info);
}
NW_HOOK_METHOD(0x100ba390, NCTradeWnd, AddPartnerItem)

// 0x100ba3a0 - the server's change of one inventory item: 1 added, 3 changed, 2 removed (the
// pet inventory has 2 and 3 the other way round)
UBOOL NCTradeWnd::UpdateInventoryItem(INT Op, NItemInfo* Info)
{
    if (Op == 1)
    {
        InventoryItems->AddItem(Info);
        return 1;
    }
    if (Op == 3)
    {
        InventoryItems->UpdateItem(Info);
        return 1;
    }
    if (Op == 2)
        InventoryItems->RemoveItem(Info->ObjectID);
    return 1;
}
NW_HOOK_METHOD(0x100ba3a0, NCTradeWnd, UpdateInventoryItem)

// one grid's entries get their name and description again from the item tables, and their
// tool tips (the stock code repeats this for each grid)
static FORCEINLINE void ReloadItemTexts(NCTradeItemWnd* Grid)
{
    for (INT i = 0; i < Grid->Items.Num(); i++)
    {
        NItemEntry* Item = Grid->Items(i);
        if (Item)
        {
            appStrcpy(Item->Name, GL2GameData.GetItemName(Item->ID));
            appStrcpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
            Grid->SetToolTip(Item);
        }
    }
}

// 0x100ba030 - after the item texts were reloaded (NConsoleWnd::ReloadGameTexts)
void NCTradeWnd::ReloadTexts()
{
    ReloadItemTexts(InventoryItems);
    ReloadItemTexts(MyItems);
    ReloadItemTexts(PartnerItems);
}
NW_HOOK_METHOD(0x100ba030, NCTradeWnd, ReloadTexts)

NW_CHECK_VTABLE(NCTradeWnd, 0x100f0280, 65)
