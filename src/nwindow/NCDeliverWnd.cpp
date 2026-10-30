/*=============================================================================
    NCDeliverWnd.cpp - the package windows: the choice of the character and the package
    window with its item grids.
    Stock: 0x10057bb0 .. 0x10059aec, 0x10059af0 .. 0x10059d23, 0x10059d30 .. 0x1005b76e.
    WindowProc (0x10057b10), OnDestroy (0x100a39e0) and SetTarget (0x10057ba0) of the grid,
    WindowProc (0x100b56e0) of the package window and OnCancel (0x10059d00) of the choice are
    copies of bodies other classes keep the addresses of.
=============================================================================*/
#include "nwindow/NCDeliverWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NItemToolTip.h"

enum { DIALOG_DELIVER_AMOUNT = 3 };  // NDialogInfo::ID (name ours): an amount for the owner
enum { MSG_HowManyToMove = 72 };     // systemmsg "How many $s1(s) do you want to move?"
enum { MSG_ChooseReceiver = 664 };   // systemmsg "Please choose a person to receive."
enum { ITEM_Adena = 57 };
enum { DELIVER_FEE_PER_SLOT = 1000 };  // adena (name ours)
enum { MAX_DELIVER_TARGETS = 8 };

// sysstring labels
enum
{
    STR_Adena = 134,
    STR_Inventory = 138,
    STR_OK = 140,
    STR_Cancel = 141,
    STR_SendingList = 560,
    STR_SendingFee = 561,
};

// the package window
static NCDeliverWnd* DeliverWnd()
{
    return GDesktop->DeliverWnd;
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

// how many of a stack to move (the adena's question allows large amounts); the answer goes
// to Owner's OnDialogResult. Inline in each stock caller; the name buffer is the stock
// frame's 64 characters.
static FORCEINLINE void AskAmount(NCWnd* Owner, NItemEntry* Item)
{
    TCHAR Name[64];
    appSprintf(Name, TEXT("%s"), Item->GetName());
    NDialogInfo Info;
    Info.ID = DIALOG_DELIVER_AMOUNT;
    Info.Type = 3;
    Info.bInput = 1;
    Info.bMainBox = 1;
    Info.Owner = Owner->Entry;
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
}

/*-----------------------------------------------------------------------------
    NCDeliverItemWnd.
-----------------------------------------------------------------------------*/

// 0x1005ae50
NCDeliverItemWnd::NCDeliverItemWnd()
{
    Target = NULL;
}
NW_HOOK_CTOR(0x1005ae50, NCDeliverItemWnd)

// 0x1005aef0 - the package window's grids are 6 x 3 icons of 32 pixels, 200 items at most
NCDeliverItemWnd::NCDeliverItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems,
                                   INT InCellSize)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
    Target = NULL;
}
NW_HOOK_CTOR_ARGS(0x1005aef0, NCDeliverItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x1005aea0
NW_HOOK_DTOR(0x1005aea0, NCDeliverItemWnd)

// stock body shared with NCInvenItemWnd's (0x10057b10): the double click and the dialog box's
// answer
INT NCDeliverItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
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

// stock body shared with NCShopItemWnd's (0x100a39e0)
INT NCDeliverItemWnd::OnDestroy()
{
    NCItemWnd::OnDestroy();
    return 1;
}

// stock body shared with NCTradeItemWnd::SetKind (0x10057ba0)
void NCDeliverItemWnd::SetTarget(NCDeliverItemWnd* InTarget)
{
    Target = InTarget;
}

// 0x10057bb0 - the warehouse's tool tip texts with 128-character buffers
void NCDeliverItemWnd::SetToolTip(NItemEntry* Item)
{
    ItemTip::SetTexts<128, 1, 128, 0, 1>(this, Item);
}
NW_HOOK_METHOD(0x10057bb0, NCDeliverItemWnd, SetToolTip)

// 0x10058260 (vslot 67) - the warehouse's tool tip box. The stock frame gives the text buffer
// 128 characters and prints the description into it too: a longer description overruns the
// FStrings above it (as in the pet inventory's copy); here the buffer has 1024, like the
// warehouse's, so the /GS check does not end the game (same drawing when it fits)
void NCDeliverItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
    ItemTip::DrawLines<1024, 1, 128, 0, 0>(this, Device, Item, X, Y);
}
NW_HOOK_METHOD(0x10058260, NCDeliverItemWnd, DrawItemToolTip)

// 0x1005af30 (vslot 57) - the carried item dropped onto another package grid goes there (that
// grid becomes the target): how many first for a stack, else one
INT NCDeliverItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (!GConsole->IsCtrlDown())
    {
        UnlockCursor();
        SetCursor(0, NULL);
    }
    if (Selected >= 0 && Selected < Items.Num() && bDragging && !IsInWindow(X, Y))
    {
        NWndEntry* E = FindWindowAt(X, Y);
        if (E && E->Wnd && E->Wnd->IsA(NCDeliverItemWnd::StaticClass()))
        {
            Target = (NCDeliverItemWnd*)E->Wnd;
            NItemEntry* Item = Items(Selected);
            if (Item->IsStackable() && Item->TradeCount > 1)
                AskAmount(this, Item);
            else
                MoveItem(Item, 1);
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1005af30, NCDeliverItemWnd, OnLButtonUp)

// 0x1005b110 (vslot 68) - the item goes to the target: how many first for a stack, else one
INT NCDeliverItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemEntry* Item = Items(Selected);
        if (Item->IsStackable() && Item->TradeCount > 1)
            AskAmount(this, Item);
        else
            MoveItem(Item, 1);
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1005b110, NCDeliverItemWnd, OnLButtonDblClick)

// 0x1005b260 (vslot 69) - the amount typed (wParam; at most the entry's) goes to the target.
// The stock code reads Items(Selected) without a bounds check.
INT NCDeliverItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    if (Item && wParam)
    {
        INT Count = (INT)wParam;
        if (Count >= Item->TradeCount)
            Count = Item->TradeCount;
        MoveItem(Item, Count);
    }
    return 1;
}
NW_HOOK_METHOD(0x1005b260, NCDeliverItemWnd, OnDialogResult)

// 0x1005a830 - a new entry, its icon from its name ("NWindow.BlackTexture" when missing,
// written back into Info), its amount the item's count
UBOOL NCDeliverItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= MaxItems)
        return 0;
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
NW_HOOK_METHOD(0x1005a830, NCDeliverItemWnd, AddItem)

// 0x1005a930 - a copy of another grid's entry holding Count
UBOOL NCDeliverItemWnd::AddCopy(NItemEntry* Src, INT Count)
{
    if (Items.Num() >= MaxItems)
        return 0;
    NItemEntry* Item = new NItemEntry;
    Item->Copy(Src);
    Item->TradeCount = Count;
    SetToolTip(Item);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x1005a930, NCDeliverItemWnd, AddCopy)

// a stack joins the target's entry of the same item id when there is one (inline in the
// stock MoveItem)
static FORCEINLINE UBOOL AddToStack(NCDeliverItemWnd* Target, NItemEntry* Item, INT Count)
{
    if (!Item->IsStackable())
        return 0;
    for (INT i = 0; i < Target->Items.Num(); i++)
    {
        if (Target->Items(i)->ID == Item->ID)
        {
            Target->Items(i)->TradeCount += Count;
            Target->SetToolTip(Target->Items(i));
            return 1;
        }
    }
    return 0;
}

// 0x1005aa00 - Count of the item go to the target (onto its stack, or as a new entry); what
// is left stays, an emptied entry leaves the grid (not freed, as in the stock code); then the
// window's fee
void NCDeliverItemWnd::MoveItem(NItemEntry* Item, INT Count)
{
    if (!Target)
        return;
    UnselectAll();
    if (!AddToStack(Target, Item, Count))
        Target->AddCopy(Item, Count);
    if (Item->IsStackable() && Item->TradeCount > Count)
    {
        Item->TradeCount -= Count;
        SetToolTip(Item);
    }
    else
        Items.Remove(Selected, 1);
    DeliverWnd()->UpdateFee();
    Selected = -1;
    UpdateScrollRange();
}
NW_HOOK_METHOD(0x1005aa00, NCDeliverItemWnd, MoveItem)

NW_CHECK_VTABLE(NCDeliverItemWnd, 0x100e3760, 70)

/*-----------------------------------------------------------------------------
    NCDeliverWnd.
-----------------------------------------------------------------------------*/

// 0x10059d30
NCDeliverWnd::NCDeliverWnd()
{
    InventoryItems = NULL;
    SendItems = NULL;
    BackTex = NULL;
    UpButton = NULL;
    DownButton = NULL;
    OKButton = NULL;
    CancelButton = NULL;
    UnknownB0 = NULL;
    Adena = 0;
    Fee = 0;
    TargetID = -1;
}
NW_HOOK_CTOR(0x10059d30, NCDeliverWnd)

// 0x10059dc0
NW_HOOK_DTOR(0x10059dc0, NCDeliverWnd)

// stock body shared with NCStoreWnd's (0x100b56e0): the buttons (nothing sends
// NID_STORE_CLOSE)
INT NCDeliverWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_STORE_OK)
            return OnOK() != 0;
        if (wParam == NID_STORE_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_STORE_UP)
            return OnUp() != 0;
        if (wParam == NID_STORE_DOWN)
            return OnDown() != 0;
        if (wParam == NID_STORE_CLOSE)
            return OnClose() != 0;
    }
    return 0;
}

// 0x1005b2a0 (vslot 35) - title bar, the grids (6 x 3 icons, 200 items at most: the
// inventory on top, what is sent below), the arrows between them, OK and Cancel
INT NCDeliverWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.StoreWnd.StoreBack"));

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_STORE_FRAME;
    FrameCtrl->ID = NID_STORE_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    SendItems = new NCDeliverItemWnd(6, 3, 200, 32);
    SendItems->SetWindowRect(16, 180, 222, 102);
    CreateChild(SendItems, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    InventoryItems = new NCDeliverItemWnd(6, 3, 200, 32);
    InventoryItems->SetWindowRect(16, 46, 222, 102);
    CreateChild(InventoryItems, NWS_VISIBLE | 0x20);
    SendItems->SetTarget(InventoryItems);
    InventoryItems->SetTarget(SendItems);

    // the up arrow's pushed texture is its normal one, as in the stock code
    UpButton = new NCButton(TEXT("L2UI.StoreWnd.StoreUpBtn1"), TEXT("L2UI.StoreWnd.StoreUpBtn1"),
                            0);
    UpButton->ID = NID_STORE_UP;
    UpButton->SetWindowRect(194, 158, 14, 13);
    CreateChild(UpButton, NWS_VISIBLE);
    DownButton = new NCButton(TEXT("L2UI.StoreWnd.StoreDownBtn1"),
                              TEXT("L2UI.StoreWnd.StoreDownBtn2"), 0);
    DownButton->ID = NID_STORE_DOWN;
    DownButton->SetWindowRect(210, 158, 14, 13);
    CreateChild(DownButton, NWS_VISIBLE);

    OKButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                            TEXT("L2UI.Control.DefaultButton_click"), 0);
    OKButton->ID = NID_STORE_OK;
    OKButton->SetWindowRect(85, 331, 67, 19);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(STR_OK);
    CancelButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    CancelButton->ID = NID_STORE_CANCEL;
    CancelButton->SetWindowRect(157, 331, 67, 19);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(STR_Cancel);
    return 1;
}
NW_HOOK_METHOD(0x1005b2a0, NCDeliverWnd, OnCreate)

// 0x1005a110 (vslot 36)
INT NCDeliverWnd::OnDestroy()
{
    DestroyChild(this, InventoryItems);
    DestroyChild(this, SendItems);
    DestroyChild(this, UpButton);
    DestroyChild(this, DownButton);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    DestroyChild(this, UnknownB0);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1005a110, NCDeliverWnd, OnDestroy)

// 0x10059a80 (vslot 42) - opens emptied (the console fills it after)
void NCDeliverWnd::ShowWindow()
{
    Clear();
    NCWnd::ShowWindow();
}
NW_HOOK_METHOD(0x10059a80, NCDeliverWnd, ShowWindow)

// 0x10059a90 (vslot 43)
void NCDeliverWnd::HideWindow()
{
    Clear();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x10059a90, NCDeliverWnd, HideWindow)

// 0x10059e10 (vslot 50) - the grids' labels, the fee's and the adena's labels right-aligned at
// 132, their values at 222, thousands separated; the buffers are the stock frame's 128
// characters
INT NCDeliverWnd::OnPaint(NCGDevice* Device)
{
    TCHAR Text[128];
    TCHAR FeeText[128];
    TCHAR AdenaText[128];
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 344, 0, 0, 240, 344, BackTex);
    Device->DrawText(15, 25, 0xFFFFFFFF, *GL2GameData.SysString[STR_Inventory], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(15, 159, 0xFFFFFFFF, *GL2GameData.SysString[STR_SendingList], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_SendingFee], 0, 0, 0, 0, 0);
    Device->DrawText(132 - Size.cx, 294, 0xFFFFFFFF, *GL2GameData.SysString[STR_SendingFee], 0,
                     0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_Adena], 0, 0, 0, 0, 0);
    Device->DrawText(132 - Size.cx, 312, 0xFFFFFFFF, *GL2GameData.SysString[STR_Adena], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);

    appSprintf(Text, TEXT("%d"), Fee);
    FormatNumber(FeeText, Text);
    appSprintf(Text, TEXT("%s"), FeeText);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    Device->DrawText(222 - Size.cx, 294, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                     0);
    appSprintf(Text, TEXT("%d"), Adena);
    FormatNumber(AdenaText, Text);
    appSprintf(Text, TEXT("%s"), AdenaText);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    Device->DrawText(222 - Size.cx, 311, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                     0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10059e10, NCDeliverWnd, OnPaint)

// 0x1005a260 (vslot 62) - the package to the server: target id, count, then (id, amount) of
// each entry below that holds something; the window closes
INT NCDeliverWnd::OnOK()
{
    INT Num = 0;
    for (INT i = 0; i < SendItems->Items.Num(); i++)
        if (SendItems->Items(i)->TradeCount > 0)
            Num++;
    L2ParamStack Params(Num * 2 + 2);
    Params.PushBack((void*)(INT_PTR)TargetID);
    Params.PushBack((void*)(INT_PTR)Num);
    for (INT i = 0; i < SendItems->Items.Num(); i++)
    {
        if (SendItems->Items(i)->TradeCount > 0)
        {
            Params.PushBack((void*)(INT_PTR)SendItems->Items(i)->UnknownC34);
            Params.PushBack((void*)(INT_PTR)SendItems->Items(i)->TradeCount);
        }
    }
    GConsole->RequestPackageSend(Params);
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x1005a260, NCDeliverWnd, OnOK)

// 0x10059ad0 (vslot 63)
INT NCDeliverWnd::OnCancel()
{
    Clear();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10059ad0, NCDeliverWnd, OnCancel)

// vslot 66 - OnCancel's stock body (0x10059ad0 serves both slots)
INT NCDeliverWnd::OnClose()
{
    Clear();
    HideWindow();
    return 1;
}

// the arrows: the selected item of Grid goes to its target, how many first for a stack
// (inline in the stock OnUp / OnDown)
static FORCEINLINE void MoveSelected(NCDeliverItemWnd* Grid)
{
    INT Sel = Grid->GetSelected();
    if (Sel < 0 || Sel >= Grid->Items.Num())
        return;
    NItemEntry* Item = Grid->Items(Sel);
    if (!Item)
        return;
    if (Item->IsStackable() && Item->TradeCount > 1)
        AskAmount(Grid, Item);
    else
        Grid->MoveItem(Item, 1);
}

// 0x1005ab30 (vslot 64) - the up arrow: back from the package
INT NCDeliverWnd::OnUp()
{
    MoveSelected(SendItems);
    return 1;
}
NW_HOOK_METHOD(0x1005ab30, NCDeliverWnd, OnUp)

// 0x1005acc0 (vslot 65) - the down arrow: into the package
INT NCDeliverWnd::OnDown()
{
    MoveSelected(InventoryItems);
    return 1;
}
NW_HOOK_METHOD(0x1005acc0, NCDeliverWnd, OnDown)

// 0x10059a20
void NCDeliverWnd::SetAdena(INT InAdena)
{
    Adena = InAdena;
}
NW_HOOK_METHOD(0x10059a20, NCDeliverWnd, SetAdena)

// 0x10059a30
void NCDeliverWnd::SetTargetID(INT InTargetID)
{
    TargetID = InTargetID;
}
NW_HOOK_METHOD(0x10059a30, NCDeliverWnd, SetTargetID)

// 0x10059a40 - kept out of line like the stock code (ShowWindow, HideWindow, OnCancel)
__declspec(noinline) void NCDeliverWnd::Clear()
{
    TargetID = -1;
    Adena = 0;
    Fee = 0;
    InventoryItems->Clear();
    SendItems->Clear();
}
NW_HOOK_METHOD(0x10059a40, NCDeliverWnd, Clear)

// 0x10059aa0 - 1000 adena for each slot sent
void NCDeliverWnd::UpdateFee()
{
    Fee = 0;
    Fee = SendItems->Items.Num() * DELIVER_FEE_PER_SLOT;
}
NW_HOOK_METHOD(0x10059aa0, NCDeliverWnd, UpdateFee)

// 0x1005ab20
UBOOL NCDeliverWnd::AddInventoryItem(NItemInfo* Info)
{
    return InventoryItems->AddItem(Info);
}
NW_HOOK_METHOD(0x1005ab20, NCDeliverWnd, AddInventoryItem)

NW_CHECK_VTABLE(NCDeliverWnd, 0x100e3548, 67)

/*-----------------------------------------------------------------------------
    NCSelectDeliverWnd.
-----------------------------------------------------------------------------*/

// 0x1005a3a0 - the targets are left as allocated
NCSelectDeliverWnd::NCSelectDeliverWnd()
{
    NumTargets = 0;
    Selected = -1;
}
NW_HOOK_CTOR(0x1005a3a0, NCSelectDeliverWnd)

// 0x1005a400
NW_HOOK_DTOR(0x1005a400, NCSelectDeliverWnd)

// 0x10059af0 (vslot 24) - the buttons and the combo box
INT NCSelectDeliverWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_SELDELIVER_OK)
            return OnOK() != 0;
        if (wParam == NID_SELDELIVER_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_SELDELIVER_TARGETS)
            return OnSelect(lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10059af0, NCSelectDeliverWnd, WindowProc)

// 0x1005a450 (vslot 35) - title bar (its id given to the frame only), the combo box of the
// characters, OK and Cancel; the background of the NPC dialogs
INT NCSelectDeliverWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_SELDELIVER_FRAME;
    FrameCtrl->SetWindowRect(0, 0, Width, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    TargetCombo = new NCComboBox;
    TargetCombo->ID = NID_SELDELIVER_TARGETS;
    TargetCombo->SetWindowRect(20, 80, 126, 15);
    CreateChild(TargetCombo, NWS_VISIBLE);

    OKButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                            TEXT("L2UI.Control.DefaultButton_click"), 0);
    OKButton->ID = NID_SELDELIVER_OK;
    OKButton->SetWindowRect(70, 240, 67, 19);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(STR_OK);
    CancelButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    CancelButton->ID = NID_SELDELIVER_CANCEL;
    CancelButton->SetWindowRect(167, 240, 67, 19);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(STR_Cancel);

    BackTex = FindTexture(TEXT("L2UI.etcwnd.NPCHtml_BACK"));
    return 1;
}
NW_HOOK_METHOD(0x1005a450, NCSelectDeliverWnd, OnCreate)

// 0x1005a7d0 (vslot 36) - only the combo box goes; the buttons stay and NCWnd::OnDestroy is
// not called, as in the stock code
INT NCSelectDeliverWnd::OnDestroy()
{
    DestroyChild(this, TargetCombo);
    return 1;
}
NW_HOOK_METHOD(0x1005a7d0, NCSelectDeliverWnd, OnDestroy)

// 0x10059c40 (vslot 42) - opens emptied (the console adds the characters after)
void NCSelectDeliverWnd::ShowWindow()
{
    Clear();
    NCWnd::ShowWindow();
}
NW_HOOK_METHOD(0x10059c40, NCSelectDeliverWnd, ShowWindow)

// 0x10059c50 (vslot 43)
void NCSelectDeliverWnd::HideWindow()
{
    Clear();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x10059c50, NCSelectDeliverWnd, HideWindow)

// 0x1005a710 (vslot 50) - the clip is set before BeginPaint, as in the stock code: the
// background stretched under the title bar and the question
INT NCSelectDeliverWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawStretchedTex(0, 21, Width, Height - 21, 0, 0, 300, 256, BackTex);
    Device->DrawText(20, 40, 0xFFFFFFFF, *GL2GameData.SystemMsg[MSG_ChooseReceiver].Message, 0, 0,
                     0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x1005a710, NCSelectDeliverWnd, OnPaint)

// 0x10059d10 (vslot 62) - the combo box's choice
INT NCSelectDeliverWnd::OnSelect(LPARAM Index)
{
    Selected = (INT)Index;
    return 1;
}
NW_HOOK_METHOD(0x10059d10, NCSelectDeliverWnd, OnSelect)

// 0x10059c60 (vslot 63) - with a character chosen, the server is asked what can be sent to it
// and the window closes
INT NCSelectDeliverWnd::OnOK()
{
    if (Selected >= 0)
    {
        L2ParamStack Params(1);
        Params.PushBack((void*)(INT_PTR)Targets[Selected].ID);
        GConsole->RequestPackageSendableItemList(Params);
        HideWindow();
    }
    return 1;
}
NW_HOOK_METHOD(0x10059c60, NCSelectDeliverWnd, OnOK)

// stock body shared with NCOptionWnd's (0x10059d00)
INT NCSelectDeliverWnd::OnCancel()
{
    HideWindow();
    return 1;
}

// 0x10059ba0 - 8 at most; the rest are dropped
void NCSelectDeliverWnd::AddTarget(INT ID, const TCHAR* Name)
{
    if (NumTargets >= MAX_DELIVER_TARGETS)
        return;
    Targets[NumTargets].ID = ID;
    appStrcpy(Targets[NumTargets].Name, Name);
    TargetCombo->AddItem(Targets[NumTargets].Name);
    NumTargets++;
}
NW_HOOK_METHOD(0x10059ba0, NCSelectDeliverWnd, AddTarget)

// 0x10059c10 - kept out of line like the stock code (ShowWindow, HideWindow)
__declspec(noinline) void NCSelectDeliverWnd::Clear()
{
    TargetCombo->ClearItems();
    Selected = -1;
    NumTargets = 0;
}
NW_HOOK_METHOD(0x10059c10, NCSelectDeliverWnd, Clear)

NW_CHECK_VTABLE(NCSelectDeliverWnd, 0x100e3658, 65)
