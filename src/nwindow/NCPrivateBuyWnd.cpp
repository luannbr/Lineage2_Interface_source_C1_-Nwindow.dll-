/*=============================================================================
    NCPrivateBuyWnd.cpp - the private store (buy) window and its grids. Stock:
    0x100955a0 .. 0x100981a0 (the grids' tool tips), 0x10098190 .. 0x1009ad8d, and
    HideWindow (0x1009d960); the bodies it shares with the sell window are hooked here or
    there (NCPrivateShopWnd.cpp).
=============================================================================*/
#include "nwindow/NCPrivateBuyWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NItemToolTip.h"

// NWM_COMMAND ids (names ours; the sell window's)
enum
{
    NID_BUYSTORE_ABORT = 0xF0,
    NID_BUYSTORE_FRAME = 0xF3,
    NID_BUYSTORE_START = 0xF4,
    NID_BUYSTORE_UP = 0xF6,
    NID_BUYSTORE_DOWN = 0xF7,
    NID_BUYSTORE_CLOSE = 0xF8,
    NID_BUYSTORE_MESSAGE = 0xF9,
};

// NDialogInfo::ID of the questions (names ours)
enum
{
    DIALOG_BUYSTORE_PRICE = 0x26,
    DIALOG_BUYSTORE_AMOUNT = 0x27,
    DIALOG_BUYSTORE_MESSAGE = 0x28,
    DIALOG_BUYSTORE_PRICE_CAUTION = 0x29,
    DIALOG_BUYSTORE_SELL_CAUTION = 0x2A,
};

// systemmsg
enum
{
    MSG_MoveHowMany = 72,       // "How many $s1(s) do you want to move?"
    MSG_StoreMessage = 334,     // "Please enter store message."
    MSG_PriceCaution = 569,     // "Caution--the item price greatly differs from the shop's
                                //   standard price. Do you wish to continue?"
    MSG_BuyHowMany = 570,       // "How many $s1 (s) do you wish to purchase?"
    MSG_BuyHowMany2 = 571,      // "How many $s1 (s) do you want to purchase?"
    MSG_SetBuyPrice = 585,      // "Please decide on the price."
};

// sysstring
enum
{
    STR_Adena = 134,
    STR_ItemsOnSale = 137,
    STR_Inventory = 138,
    STR_OK = 140,
    STR_Price = 142,
    STR_PriceSell = 143,
    STR_Message = 384,
    STR_Abort = 385,
    STR_Start = 428,
    STR_WishList = 502,
    STR_ForSale = 503,
};

// a child window goes (repeated inline in the stock OnDestroys)
template <class T> static FORCEINLINE void DestroyStoreChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// a price 5 times away from the item's reference price, either way (the reference 0: none)
static FORCEINLINE UBOOL IsUnusualPrice(NItemEntry* Item)
{
    return Item->Unknown1CA8 > 0
        && (Item->UnknownC20 <= Item->Unknown1CA8 / 5 || Item->UnknownC20 >= Item->Unknown1CA8 * 5);
}

// the desktop's box asks how many of the item (NWM_BUYSTORE_AMOUNT answers the grid)
static FORCEINLINE void AskAmount(NCWnd* Grid, NItemEntry* Item, INT MsgID)
{
    TCHAR Name[64];
    appSprintf(Name, TEXT("%s"), Item->GetName());
    NDialogInfo Info;
    Info.Owner = Grid->Entry;
    Info.ID = DIALOG_BUYSTORE_AMOUNT;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.Type = 3;
    Info.SetSystemMessage(MsgID);
    Info.SetButtons(Name, NULL);
    Info.bLargeAmount = 0;
    GConsole->ShowDialog(&Info);
}

// ... the price offered, up to 100,000,000 (NWM_BUYSTORE_PRICE answers)
static FORCEINLINE void AskPrice(NCWnd* Grid)
{
    NDialogInfo Info;
    Info.ID = DIALOG_BUYSTORE_PRICE;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.Type = 3;
    Info.Owner = Grid->Entry;
    Info.SetSystemMessage(MSG_SetBuyPrice);
    Info.bLargeAmount = 1;
    GConsole->ShowDialog(&Info);
}

/*-----------------------------------------------------------------------------
    NCPrivateBuyItemWnd.
-----------------------------------------------------------------------------*/

// 0x1009a090
NCPrivateBuyItemWnd::NCPrivateBuyItemWnd()
{
    CoverTex = FindTexture(TEXT("NWindow.Chat"));
    Partner = NULL;
    Unknown128 = 0;
}
NW_HOOK_CTOR(0x1009a090, NCPrivateBuyItemWnd)

// 0x1009a180 - the store window's grids are 6 x 3 icons of 32 pixels, 80 items at most
NCPrivateBuyItemWnd::NCPrivateBuyItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
    CoverTex = FindTexture(TEXT("NWindow.Chat"));
    Partner = NULL;
    Unknown128 = 0;
}
NW_HOOK_CTOR_ARGS(0x1009a180, NCPrivateBuyItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x1009a130
NW_HOOK_DTOR(0x1009a130, NCPrivateBuyItemWnd)

// 0x100955a0 (vslot 24) - NCItemWnd's, the double click and the dialog box's answers
INT NCPrivateBuyItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    if (Msg == NWM_BUYSTORE_PRICE_CANCEL)
        return OnPriceCancel(wParam, lParam) != 0;
    if (Msg == NWM_BUYSTORE_PRICE)
        return OnPrice(wParam, lParam) != 0;
    if (Msg == NWM_BUYSTORE_AMOUNT)
        return OnAmount(wParam, lParam) != 0;
    if (Msg == NWM_BUYSTORE_PRICE_OK)
        return OnPriceOK(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100955a0, NCPrivateBuyItemWnd, WindowProc)

// stock body shared with NCDeliverItemWnd's (0x100a39e0)
INT NCPrivateBuyItemWnd::OnDestroy()
{
    NCItemWnd::OnDestroy();
    return 1;
}

// 0x100983a0 (vslot 50) - NCItemWnd's icons; at someone's store the items the player lacks
// (count 0) get a cover, its texture looked up again each time. The stock guard carries
// NCItemWnd's name.
INT NCPrivateBuyItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCItemWnd::OnPaint);
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
        if (Mode == 0 && Item->Count == 0)
        {
            CoverTex = FindTexture(TEXT("NWindow.ChatBack"));
            if (Item->IsSelected())
                Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, SelectTex);
            if (Item->GetIcon())
                Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon());
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, CoverTex);
        }
        else
        {
            if (Item->IsSelected())
                Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, SelectTex);
            if (Item->GetIcon())
                Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon());
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100983a0, NCPrivateBuyItemWnd, OnPaint)

// 0x1009a200 (vslot 57) - NCPrivateShopItemWnd::OnLButtonUp for the buy store: at someone's
// store an item the player lacks stays; going on the wish list asks the price, a stack coming
// off it how many
INT NCPrivateBuyItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT Y = HIWORD(lParam);
    if (!GConsole->IsCtrlDown())
    {
        UnlockCursor();
        SetCursor(0, NULL);
    }
    if (Selected >= 0 && Selected < Items.Num() && bDragging)
    {
        INT X = LOWORD(lParam);
        if (!IsInWindow(X, Y))
        {
            NWndEntry* E = FindWindowAt(X, Y);
            if (E && E->Wnd && E->Wnd->IsA(NCPrivateBuyItemWnd::StaticClass()))
            {
                Partner = (NCPrivateBuyItemWnd*)E->Wnd;
                NItemEntry* Item = Items(Selected);
                if (Mode == 0)
                {
                    if (Item->Count > 0)
                    {
                        if (Item->IsStackable())
                            AskAmount(this, Item, MSG_MoveHowMany);
                        else
                            MoveItem(Item, 1);
                    }
                }
                else if (Partner->Side == 0)
                    AskPrice(this);
                else if (Item->IsStackable() && Item->Count > 1)
                    AskAmount(this, Item, MSG_BuyHowMany2);
                else
                    MoveItem(Item, 1);
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1009a200, NCPrivateBuyItemWnd, OnLButtonUp)

// 0x10096790 (vslot 67) - the tool tip box with the price on the store's list and, at
// someone's store, an etc stack's amount wanted (NItemToolTip.h)
void NCPrivateBuyItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
    ItemTip::DrawLines<1024, 0, 1024, 0, 0, 1>(this, Device, Item, X, Y, Side == 0,
                                              Side == 0 && Mode == 0);
}
NW_HOOK_METHOD(0x10096790, NCPrivateBuyItemWnd, DrawItemToolTip)

// 0x1009a500 (vslot 68) - OnLButtonUp's moves, onto Partner (a stack coming off the list asks
// how many whatever its count)
INT NCPrivateBuyItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemEntry* Item = Items(Selected);
        if (Mode == 0)
        {
            if (Item->Count > 0)
            {
                if (Item->IsStackable())
                    AskAmount(this, Item, MSG_MoveHowMany);
                else
                    MoveItem(Item, 1);
            }
        }
        else if (Partner->Side == 0)
            AskPrice(this);
        else if (Item->IsStackable())
            AskAmount(this, Item, MSG_BuyHowMany2);
        else
            MoveItem(Item, 1);
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1009a500, NCPrivateBuyItemWnd, OnLButtonDblClick)

// 0x10098730 (vslot 70) - the price offered for the selected item (setting one's own store
// up): it takes it; a price 5 times away from the reference asks again, then how many are
// wanted (always; 0: the box stays)
INT NCPrivateBuyItemWnd::OnPrice(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    INT Price = (INT)wParam;
    if (!Item || !Price)
        return 1;
    if (Mode != 1 || Price <= 0)
        return 1;
    Item->UnknownC20 = Price;
    if (Item->Unknown1CA8 > 0 && (Price <= Item->Unknown1CA8 / 5 || Price >= Item->Unknown1CA8 * 5))
    {
        NDialogInfo Info;
        Info.ID = DIALOG_BUYSTORE_PRICE_CAUTION;
        Info.bMainBox = 1;
        Info.bInput = 0;
        Info.Owner = Entry;
        Info.SetSystemMessage(MSG_PriceCaution);
        Info.bLargeAmount = 0;
        GConsole->ShowDialog(&Info);
        return 0;
    }
    AskAmount(this, Item, MSG_BuyHowMany);
    return 0;
}
NW_HOOK_METHOD(0x10098730, NCPrivateBuyItemWnd, OnPrice)

// 0x1009a770 (vslot 71) - the amount typed: that many of the selected item move (setting up,
// at most 100,000,000; at someone's store, at most what it wants)
INT NCPrivateBuyItemWnd::OnAmount(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    INT Amount = (INT)wParam;
    if (Item && Amount)
    {
        if (Mode == 1 && Amount > 0)
        {
            Unknown128 = 0;
            MoveItem(Item, Amount < 100000000 ? Amount : 100000000);
        }
        else if (Mode == 0 && Amount > 0)
            MoveItem(Item, Amount < Item->UnknownC34 ? Amount : Item->UnknownC34);
    }
    return 1;
}
NW_HOOK_METHOD(0x1009a770, NCPrivateBuyItemWnd, OnAmount)

// 0x10098660 (vslot 72) - the unusual price confirmed: how many are wanted
INT NCPrivateBuyItemWnd::OnPriceOK(WPARAM wParam, LPARAM lParam)
{
    AskAmount(this, Items(Selected), MSG_BuyHowMany);
    return 0;
}
NW_HOOK_METHOD(0x10098660, NCPrivateBuyItemWnd, OnPriceOK)

// stock body shared with NCStoreItemWnd::SetMode (0x100b3880)
void NCPrivateBuyItemWnd::SetPartner(NCPrivateBuyItemWnd* InPartner)
{
    Partner = InPartner;
}

// stock body shared with NCStoreItemWnd::SetSide (0x100b3890)
void NCPrivateBuyItemWnd::SetMode(INT InMode)
{
    Mode = InMode;
}

// 0x10095690
void NCPrivateBuyItemWnd::SetSide(INT InSide)
{
    Side = InSide;
}
NW_HOOK_METHOD(0x10095690, NCPrivateBuyItemWnd, SetSide)

// 0x100956a0 - the entry's name tip and text, with the price on the store's list
// (NItemToolTip.h)
void NCPrivateBuyItemWnd::SetToolTip(NItemEntry* Item)
{
    ItemTip::SetStoreTexts<1>(this, Item, Mode, Side);
}
NW_HOOK_METHOD(0x100956a0, NCPrivateBuyItemWnd, SetToolTip)

// 0x10099680 - NCPrivateShopItemWnd::AddItem with this grid's texts
UBOOL NCPrivateBuyItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= MaxItems)
        return 0;
    Info->Icon = FindTexture(Info->IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = new NItemEntry(Info);
    Item->TradeCount = Info->Count;
    SetToolTip(Item);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x10099680, NCPrivateBuyItemWnd, AddItem)

// 0x10099780 - a copy of Src with Count there
UBOOL NCPrivateBuyItemWnd::AddEntry(NItemEntry* Src, INT Count)
{
    if (Items.Num() >= MaxItems)
        return 0;
    NItemEntry* Item = new NItemEntry();
    Item->Copy(Src);
    Item->TradeCount = Count;
    SetToolTip(Item);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x10099780, NCPrivateBuyItemWnd, AddEntry)

// 0x10099850 - Count of the selected Item to Partner. Setting one's own store up, the catalog
// keeps its item: a stack joins the same item on the wish list (taking this price), single
// items replace the entries of the same item and enchant there (not freed; the loop skips the
// entry after each one removed) and come as one entry each, 100 at most; off the list the
// entry just shrinks or leaves. At someone's store a stack joins the same item on the other
// side or comes as a copy, the amount wanted moving along, and shrinks or leaves (not freed).
void NCPrivateBuyItemWnd::MoveItem(NItemEntry* Item, INT Count)
{
    if (!Partner)
        return;
    UnselectAll();
    if (Mode == 1)
    {
        if (Partner->Side == 0)
        {
            if (Item->IsStackable())
            {
                NItemEntry* Same = NULL;
                for (INT i = 0; i < Partner->Items.Num(); i++)
                {
                    if (Partner->Items(i)->ID == Item->ID)
                    {
                        Same = Partner->Items(i);
                        break;
                    }
                }
                if (Same)
                {
                    Same->TradeCount += Count;
                    Same->UnknownC20 = Item->UnknownC20;
                    Partner->SetToolTip(Same);
                }
                else
                    Partner->AddEntry(Item, Count);
            }
            else
            {
                for (INT i = 0; i < Partner->Items.Num(); i++)
                {
                    NItemEntry* Listed = Partner->Items(i);
                    if (Listed->ID == Item->ID && Listed->Enchant == Item->Enchant)
                        Partner->Items.Remove(i);
                }
                if (Count > 100)
                    Count = 100;
                for (; Count > 0; Count--)
                    Partner->AddEntry(Item, 1);
            }
        }
        else if (Partner->Side == 1)
        {
            if (Item->IsStackable() && Item->TradeCount > Count)
            {
                Item->TradeCount -= Count;
                SetToolTip(Item);
            }
            else
                Items.Remove(Selected);
        }
    }
    else if (Mode == 0 && (Partner->Side == 0 || Partner->Side == 1))
    {
        NItemEntry* Same = NULL;
        if (Item->IsStackable())
        {
            for (INT i = 0; i < Partner->Items.Num(); i++)
            {
                if (Partner->Items(i)->ID == Item->ID)
                {
                    Same = Partner->Items(i);
                    break;
                }
            }
        }
        if (Same)
        {
            Same->TradeCount += Count;
            Same->UnknownC34 += Count;
            Partner->SetToolTip(Same);
        }
        else
            Partner->AddEntry(Item, Count);
        if (Item->IsStackable() && Item->TradeCount > Count)
        {
            Item->UnknownC34 -= Count;
            Item->TradeCount -= Count;
            SetToolTip(Item);
        }
        else
            Items.Remove(Selected);
    }
    GDesktop->PrivateBuyWnd->UpdateTotal();
    Selected = -1;
    UpdateScrollRange();
}
NW_HOOK_METHOD(0x10099850, NCPrivateBuyItemWnd, MoveItem)

NW_CHECK_VTABLE(NCPrivateBuyItemWnd, 0x100ed018, 73)

/*-----------------------------------------------------------------------------
    NCPrivateBuyWnd.
-----------------------------------------------------------------------------*/

// 0x100988c0 - Mode and the total are left as allocated
NCPrivateBuyWnd::NCPrivateBuyWnd()
{
    PlayerItems = NULL;
    StoreItems = NULL;
    Unknown9C = 0;
    BackTex = NULL;
    UnknownA4 = 0;
    UpButton = NULL;
    DownButton = NULL;
    StartButton = NULL;
    MessageButton = NULL;
    AbortButton = NULL;
    UnknownBC = NULL;
    Adena = 0;
    OwnerID = -1;
}
NW_HOOK_CTOR(0x100988c0, NCPrivateBuyWnd)

// 0x10098960
NW_HOOK_DTOR(0x10098960, NCPrivateBuyWnd)

// 0x10098190 (vslot 24)
INT NCPrivateBuyWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_BUYSTORE_START)
            return OnStart() != 0;
        if (wParam == NID_BUYSTORE_UP)
            return OnUp() != 0;
        if (wParam == NID_BUYSTORE_DOWN)
            return OnDown() != 0;
        if (wParam == NID_BUYSTORE_CLOSE)
            return OnClose() != 0;
        if (wParam == NID_BUYSTORE_MESSAGE)
            return OnMessage() != 0;
        if (wParam == NID_BUYSTORE_ABORT)
            return OnAbort() != 0;
    }
    else if (Msg == NWM_BUYSTORE_SELL_OK)
        return OnSellConfirmed(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10098190, NCPrivateBuyWnd, WindowProc)

// 0x100982c0 (vslot 42) - also the sell window's: Message and Abort only for one's own store,
// whose button starts it ("OK" at someone's)
void NCPrivateBuyWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Mode == 1)
    {
        AbortButton->ShowWindow();
        MessageButton->ShowWindow();
        StartButton->SetTextID(STR_Start);
    }
    else if (Mode == 0)
    {
        AbortButton->HideWindow();
        MessageButton->HideWindow();
        StartButton->SetTextID(STR_OK);
    }
}
NW_HOOK_METHOD(0x100982c0, NCPrivateBuyWnd, ShowWindow)

// 0x10098340 (vslot 65) - closing one's own store also tells the server (NetHandler slot 204)
INT NCPrivateBuyWnd::OnClose()
{
    if (Mode == 1)
        GConsole->RequestPrivateStoreBuyManageCancel();
    Reset();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10098340, NCPrivateBuyWnd, OnClose)

// 0x10098370 (vslot 67) - Abort: the store closes (NetHandler slot 205)
INT NCPrivateBuyWnd::OnAbort()
{
    GConsole->RequestPrivateStoreQuitBuy();
    Reset();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10098370, NCPrivateBuyWnd, OnAbort)

// 0x100989b0 (vslot 50) - NCPrivateShopWnd::OnPaint with the buy store's labels. No guard in
// the stock code.
INT NCPrivateBuyWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 344, 0, 0, 240, 344, BackTex);
    SIZE Size;
    if (Mode == 0)
    {
        Device->DrawText(15, 25, 0xFFFFFFFF, *GL2GameData.SysString[STR_ForSale], 0, 0, 0, L2FONT_Normal, 0,
                         0, 0, 0, 0, 0);
        Device->DrawText(15, 159, 0xFFFFFFFF, *GL2GameData.SysString[STR_ItemsOnSale], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_PriceSell], 0, 0, 0, 0, 0);
        Device->DrawText(132 - Size.cx, 294, 0xFFFFFFFF, *GL2GameData.SysString[STR_PriceSell], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_Adena], 0, 0, 0, 0, 0);
        Device->DrawText(132 - Size.cx, 312, 0xFFFFFFFF, *GL2GameData.SysString[STR_Adena], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else if (Mode == 1)
    {
        Device->DrawText(15, 25, 0xFFFFFFFF, *GL2GameData.SysString[STR_Inventory], 0, 0, 0, L2FONT_Normal,
                         0, 0, 0, 0, 0, 0);
        Device->DrawText(15, 159, 0xFFFFFFFF, *GL2GameData.SysString[STR_WishList], 0, 0, 0, L2FONT_Normal,
                         0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_Price], 0, 0, 0, 0, 0);
        Device->DrawText(132 - Size.cx, 294, 0xFFFFFFFF, *GL2GameData.SysString[STR_Price], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_Adena], 0, 0, 0, 0, 0);
        Device->DrawText(132 - Size.cx, 312, 0xFFFFFFFF, *GL2GameData.SysString[STR_Adena], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }

    // "%d" takes the total's low half, as in the stock code
    TCHAR Buf[1024];
    TCHAR Total[1024];
    TCHAR AdenaText[1024];
    appSprintf(Buf, TEXT("%d"), TotalPrice);
    FormatNumber(Total, Buf);
    appSprintf(Buf, TEXT("%s"), Total);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawText(222 - Size.cx, 294, 0xFFFFFFFF, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    appSprintf(Buf, TEXT("%d"), Adena);
    FormatNumber(AdenaText, Buf);
    appSprintf(Buf, TEXT("%s"), AdenaText);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawText(222 - Size.cx, 311, 0xFFFFFFFF, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100989b0, NCPrivateBuyWnd, OnPaint)

// 0x10098e10 (vslot 36) - also the sell window's
INT NCPrivateBuyWnd::OnDestroy()
{
    DestroyStoreChild(this, PlayerItems);
    DestroyStoreChild(this, StoreItems);
    DestroyStoreChild(this, UpButton);
    DestroyStoreChild(this, DownButton);
    DestroyStoreChild(this, StartButton);
    DestroyStoreChild(this, UnknownBC);
    DestroyStoreChild(this, MessageButton);
    DestroyStoreChild(this, AbortButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10098e10, NCPrivateBuyWnd, OnDestroy)

// 0x10098f90 (vslot 62) - Start: the store opens with the wish list (its count, then the item
// id, enchant, count and price of each; NetHandler slot 203). OK at someone's store: the items
// chosen are sold (the owner's object id first, then object id, item id, enchant, count and
// price; slot 208), unusual prices asking first.
INT NCPrivateBuyWnd::OnStart()
{
    if (Mode == 1)
    {
        INT Count = 0;
        for (INT i = 0; i < StoreItems->Items.Num(); i++)
            if (StoreItems->Items(i)->TradeCount > 0)
                Count++;
        L2ParamStack Params(Count * 4 + 1);
        Params.PushBack((void*)(INT_PTR)Count);
        for (INT i = 0; i < StoreItems->Items.Num(); i++)
        {
            NItemEntry* Item = StoreItems->Items(i);
            if (Item->TradeCount > 0)
            {
                Params.PushBack((void*)(INT_PTR)Item->ID);
                Params.PushBack((void*)(INT_PTR)Item->Enchant);
                Params.PushBack((void*)(INT_PTR)Item->TradeCount);
                Params.PushBack((void*)(INT_PTR)Item->UnknownC20);
            }
        }
        GConsole->RequestSetPrivateStoreListBuy(Params);
        Reset();
        HideWindow();
    }
    else if (Mode == 0)
    {
        INT Count = 0;
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
            if (PlayerItems->Items(i)->TradeCount > 0)
                Count++;
        L2ParamStack Params(Count * 5 + 2);
        Params.PushBack((void*)(INT_PTR)OwnerID);
        Params.PushBack((void*)(INT_PTR)Count);
        UBOOL bUsual = 1;
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
        {
            NItemEntry* Item = PlayerItems->Items(i);
            if (Item->TradeCount > 0)
            {
                if (IsUnusualPrice(Item))
                    bUsual = 0;
                Params.PushBack((void*)(INT_PTR)Item->ObjectID);
                Params.PushBack((void*)(INT_PTR)Item->ID);
                Params.PushBack((void*)(INT_PTR)Item->Enchant);
                Params.PushBack((void*)(INT_PTR)Item->TradeCount);
                Params.PushBack((void*)(INT_PTR)Item->UnknownC20);
            }
        }
        if (bUsual)
        {
            GConsole->RequestPrivateStoreSell(Params);
            Reset();
            HideWindow();
        }
        else
        {
            NDialogInfo Info;
            Info.Type = 3;
            Info.ID = DIALOG_BUYSTORE_SELL_CAUTION;
            Info.bMainBox = 1;
            Info.bInput = 0;
            Info.Owner = Entry;
            Info.SetSystemMessage(MSG_PriceCaution);
            Info.bLargeAmount = 0;
            GConsole->ShowDialog(&Info);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10098f90, NCPrivateBuyWnd, OnStart)

// 0x10099360 - also the sell window's: the list's price, each entry's price times its count,
// in 64 bits
void NCPrivateBuyWnd::UpdateTotal()
{
    TotalPrice = 0;
    if (Mode == 0)
    {
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
            TotalPrice += (SQWORD)PlayerItems->Items(i)->GetUnknownC20() * PlayerItems->Items(i)->TradeCount;
    }
    else if (Mode == 1)
    {
        for (INT i = 0; i < StoreItems->Items.Num(); i++)
            TotalPrice += (SQWORD)StoreItems->Items(i)->GetUnknownC20() * StoreItems->Items(i)->TradeCount;
    }
}
NW_HOOK_METHOD(0x10099360, NCPrivateBuyWnd, UpdateTotal)

// 0x10099450 (vslot 66) - Message: the store's message, typed in the desktop's box (dialog
// 0x28 hands it to the console)
INT NCPrivateBuyWnd::OnMessage()
{
    NDialogInfo Info;
    Info.ID = DIALOG_BUYSTORE_MESSAGE;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.Owner = PlayerItems->Entry;
    Info.SetSystemMessage(MSG_StoreMessage);
    Info.bLargeAmount = 0;
    Info.Type = 0;
    GConsole->ShowDialog(&Info);
    return 1;
}
NW_HOOK_METHOD(0x10099450, NCPrivateBuyWnd, OnMessage)

// 0x100994c0 (vslot 68) - selling at the unusual prices confirmed
INT NCPrivateBuyWnd::OnSellConfirmed(WPARAM wParam, LPARAM lParam)
{
    if (Mode == 0)
    {
        INT Count = 0;
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
            if (PlayerItems->Items(i)->TradeCount > 0)
                Count++;
        L2ParamStack Params(Count * 5 + 2);
        Params.PushBack((void*)(INT_PTR)OwnerID);
        Params.PushBack((void*)(INT_PTR)Count);
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
        {
            NItemEntry* Item = PlayerItems->Items(i);
            if (Item->TradeCount > 0)
            {
                Params.PushBack((void*)(INT_PTR)Item->ObjectID);
                Params.PushBack((void*)(INT_PTR)Item->ID);
                Params.PushBack((void*)(INT_PTR)Item->Enchant);
                Params.PushBack((void*)(INT_PTR)Item->TradeCount);
                Params.PushBack((void*)(INT_PTR)Item->UnknownC20);
            }
        }
        GConsole->RequestPrivateStoreSell(Params);
    }
    Reset();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100994c0, NCPrivateBuyWnd, OnSellConfirmed)

// 0x10099bf0 - both grids take the mode; they move to its places
void NCPrivateBuyWnd::SetMode(INT InMode)
{
    Mode = InMode;
    StoreItems->SetMode(Mode);
    PlayerItems->SetMode(Mode);
    UpdateLayout();
}
NW_HOOK_METHOD(0x10099bf0, NCPrivateBuyWnd, SetMode)

// 0x10099c30 - one's own list adds up again
void NCPrivateBuyWnd::AddStoreItem(NItemInfo* Info)
{
    StoreItems->AddItem(Info);
    if (Mode == 1)
        UpdateTotal();
}
NW_HOOK_METHOD(0x10099c30, NCPrivateBuyWnd, AddStoreItem)

// 0x10099c60
UBOOL NCPrivateBuyWnd::AddPlayerItem(NItemInfo* Info)
{
    return PlayerItems->AddItem(Info);
}
NW_HOOK_METHOD(0x10099c60, NCPrivateBuyWnd, AddPlayerItem)

// 0x10099c70 (vslot 63) - the up arrow: the selected item of the lower grid goes up (a stack
// of more than one asks how many; at someone's store an item the player lacks stays)
INT NCPrivateBuyWnd::OnUp()
{
    if (Mode == 0)
    {
        INT Sel = PlayerItems->GetSelected();
        if (Sel >= 0 && Sel < PlayerItems->Items.Num())
        {
            NItemEntry* Item = PlayerItems->Items(Sel);
            if (Item && Item->Count > 0)
            {
                if (Item->IsStackable() && Item->TradeCount > 1)
                    AskAmount(PlayerItems, Item, MSG_MoveHowMany);
                else
                    PlayerItems->MoveItem(Item, 1);
            }
        }
    }
    else if (Mode == 1)
    {
        INT Sel = StoreItems->GetSelected();
        if (Sel >= 0 && Sel < StoreItems->Items.Num())
        {
            NItemEntry* Item = StoreItems->Items(Sel);
            if (Item)
            {
                if (Item->IsStackable() && Item->TradeCount > 1)
                    AskAmount(StoreItems, Item, MSG_BuyHowMany2);
                else
                    StoreItems->MoveItem(Item, 1);
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10099c70, NCPrivateBuyWnd, OnUp)

// 0x10099ed0 (vslot 64) - the down arrow: at someone's store the selected item comes down (a
// stack asks how many; one the player lacks stays); setting one's own up, the price first
INT NCPrivateBuyWnd::OnDown()
{
    if (Mode == 0)
    {
        INT Sel = StoreItems->GetSelected();
        if (Sel >= 0 && Sel < StoreItems->Items.Num())
        {
            NItemEntry* Item = StoreItems->Items(Sel);
            if (Item && Item->Count > 0)
            {
                if (Item->IsStackable())
                    AskAmount(StoreItems, Item, MSG_MoveHowMany);
                else
                    StoreItems->MoveItem(Item, 1);
            }
        }
    }
    else if (Mode == 1)
    {
        INT Sel = PlayerItems->GetSelected();
        if (Sel >= 0 && Sel < PlayerItems->Items.Num() && PlayerItems->Items(Sel))
            AskPrice(PlayerItems);
    }
    return 1;
}
NW_HOOK_METHOD(0x10099ed0, NCPrivateBuyWnd, OnDown)

// 0x1009a7f0 (vslot 35) - NCPrivateShopWnd::OnCreate with the buy grids
INT NCPrivateBuyWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.StoreWnd.StoreBack"));

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_BUYSTORE_FRAME;
    FrameCtrl->ID = NID_BUYSTORE_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    StoreItems = new NCPrivateBuyItemWnd(6, 3, 80, 32);
    StoreItems->SetWindowRect(17, 180, 222, 102);
    CreateChild(StoreItems, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    StoreItems->SetSide(0);

    PlayerItems = new NCPrivateBuyItemWnd(6, 3, 80, 32);
    PlayerItems->SetWindowRect(17, 46, 222, 102);
    CreateChild(PlayerItems, NWS_VISIBLE | 0x20);
    PlayerItems->SetSide(1);

    StoreItems->SetPartner(PlayerItems);
    PlayerItems->SetPartner(StoreItems);

    UpButton = new NCButton(TEXT("L2UI.StoreWnd.StoreUpBtn1"), TEXT("L2UI.StoreWnd.StoreUpBtn1"), 0);
    UpButton->ID = NID_BUYSTORE_UP;
    UpButton->SetWindowRect(194, 158, 14, 13);
    CreateChild(UpButton, NWS_VISIBLE);

    DownButton = new NCButton(TEXT("L2UI.StoreWnd.StoreDownBtn1"), TEXT("L2UI.StoreWnd.StoreDownBtn2"), 0);
    DownButton->ID = NID_BUYSTORE_DOWN;
    DownButton->SetWindowRect(210, 158, 14, 13);
    CreateChild(DownButton, NWS_VISIBLE);

    StartButton = new NCButton(TEXT("L2UI.Control.DefaultButton"), TEXT("L2UI.Control.DefaultButton_click"), 0);
    StartButton->ID = NID_BUYSTORE_START;
    StartButton->SetWindowRect(164, 331, 67, 19);
    CreateChild(StartButton, NWS_VISIBLE);
    StartButton->SetTextID(STR_Start);

    MessageButton = new NCButton(TEXT("L2UI.Control.DefaultButton"), TEXT("L2UI.Control.DefaultButton_click"), 0);
    MessageButton->ID = NID_BUYSTORE_MESSAGE;
    MessageButton->SetWindowRect(87, 331, 67, 19);
    CreateChild(MessageButton, NWS_VISIBLE);
    MessageButton->SetTextID(STR_Message);

    AbortButton = new NCButton(TEXT("L2UI.Control.DefaultButton"), TEXT("L2UI.Control.DefaultButton_click"), 0);
    AbortButton->ID = NID_BUYSTORE_ABORT;
    AbortButton->SetWindowRect(10, 331, 67, 19);
    CreateChild(AbortButton, NWS_VISIBLE);
    AbortButton->SetTextID(STR_Abort);
    return 1;
}
NW_HOOK_METHOD(0x1009a7f0, NCPrivateBuyWnd, OnCreate)

// 0x1009d960 (vslot 43) - also the sell window's
void NCPrivateBuyWnd::HideWindow()
{
    Reset();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x1009d960, NCPrivateBuyWnd, HideWindow)

// stock body shared with NCPrivateShopWnd's (0x1009d910)
void NCPrivateBuyWnd::SetAdena(INT InAdena)
{
    Adena = InAdena;
}

// stock body shared with NCShopWnd::SetAdena (0x100a5d20); the console sets it
void NCPrivateBuyWnd::SetOwnerID(INT InOwnerID)
{
    OwnerID = InOwnerID;
}

// stock body shared with NCPrivateShopWnd's (0x1009d920)
void NCPrivateBuyWnd::Reset()
{
    OwnerID = -1;
    Adena = 0;
    TotalPrice = 0;
    PlayerItems->Clear();
    StoreItems->Clear();
}

// stock body shared with NCPrivateShopWnd's (0x1009df20)
void NCPrivateBuyWnd::UpdateLayout()
{
    // the stock code fills { LocalX, LocalY, LocalW, LocalH }; SetScreenPos reads x and y
    POINT Origin;
    if (Mode == 0)
    {
        StoreItems->UpdateScrollRange();
        PlayerItems->UpdateScrollRange();
        Origin.x = LocalX;
        Origin.y = LocalY;
        StoreItems->SetScreenPos(16, 46, &Origin);
        PlayerItems->SetScreenPos(16, 180, &Origin);
    }
    else if (Mode == 1)
    {
        StoreItems->UpdateScrollRange();
        PlayerItems->UpdateScrollRange();
        Origin.x = LocalX;
        Origin.y = LocalY;
        StoreItems->SetScreenPos(16, 180, &Origin);
        PlayerItems->SetScreenPos(16, 46, &Origin);
    }
    else
        return;
    StoreItems->SetSize(222, 102);
    PlayerItems->SetSize(222, 102);
}

NW_CHECK_VTABLE(NCPrivateBuyWnd, 0x100ecf00, 69)
