/*=============================================================================
    NCPrivateShopWnd.cpp - the private store (sell) window and its grids. Stock:
    0x1009ad90 .. 0x1009d7c8 (the grids' tool tips), 0x1009d7e0 .. 0x1009fdbd; the shared
    bodies (OnDestroy, ShowWindow, HideWindow, UpdateTotal) are hooked as NCPrivateBuyWnd's.
=============================================================================*/
#include "nwindow/NCPrivateShopWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NItemToolTip.h"

// NWM_COMMAND ids (names ours)
enum
{
    NID_STORE_ABORT = 0xF0,
    NID_STORE_FRAME = 0xF3,
    NID_STORE_START = 0xF4,
    NID_STORE_UP = 0xF6,
    NID_STORE_DOWN = 0xF7,
    NID_STORE_CLOSE = 0xF8,
    NID_STORE_MESSAGE = 0xF9,
};

// NDialogInfo::ID of the questions (names ours)
enum
{
    DIALOG_STORE_PRICE = 0x21,
    DIALOG_STORE_AMOUNT = 0x22,
    DIALOG_STORE_MESSAGE = 0x23,
    DIALOG_STORE_PRICE_CAUTION = 0x24,
    DIALOG_STORE_BUY_CAUTION = 0x25,
};

// systemmsg
enum
{
    MSG_MoveHowMany = 72,       // "How many $s1(s) do you want to move?"
    MSG_SetPrice = 322,         // "Please decide on the sales price."
    MSG_StoreMessage = 334,     // "Please enter store message."
    MSG_PriceCaution = 569,     // "Caution--the item price greatly differs from the shop's
                                //   standard price. Do you wish to continue?"
};

// sysstring
enum
{
    STR_Adena = 134,
    STR_ItemsOnSale = 137,
    STR_Inventory = 138,
    STR_ItemsToBuy = 139,
    STR_OK = 140,
    STR_Price = 142,
    STR_PriceSell = 143,
    STR_Message = 384,
    STR_Abort = 385,
    STR_Start = 428,
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

// the desktop's box asks how many of the item to move (NWM_STORE_AMOUNT answers the grid)
static FORCEINLINE void AskAmount(NCWnd* Grid, NItemEntry* Item)
{
    TCHAR Name[64];
    appSprintf(Name, TEXT("%s"), Item->GetName());
    NDialogInfo Info;
    Info.Owner = Grid->Entry;
    Info.ID = DIALOG_STORE_AMOUNT;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.Type = 3;
    Info.SetSystemMessage(MSG_MoveHowMany);
    Info.SetButtons(Name, NULL);
    Info.bLargeAmount = 0;
    GConsole->ShowDialog(&Info);
}

// ... the price, up to 100,000,000 (NWM_STORE_PRICE answers)
static FORCEINLINE void AskPrice(NCWnd* Grid)
{
    NDialogInfo Info;
    Info.ID = DIALOG_STORE_PRICE;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.Type = 3;
    Info.Owner = Grid->Entry;
    Info.SetSystemMessage(MSG_SetPrice);
    Info.bLargeAmount = 1;
    GConsole->ShowDialog(&Info);
}

/*-----------------------------------------------------------------------------
    NCPrivateShopItemWnd.
-----------------------------------------------------------------------------*/

// 0x1009eeb0
NCPrivateShopItemWnd::NCPrivateShopItemWnd()
{
    Partner = NULL;
    Unknown124 = 0;
}
NW_HOOK_CTOR(0x1009eeb0, NCPrivateShopItemWnd)

// 0x1009ef50 - the store window's grids are 6 x 3 icons of 32 pixels, 80 items at most
NCPrivateShopItemWnd::NCPrivateShopItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
    Partner = NULL;
    Unknown124 = 0;
}
NW_HOOK_CTOR_ARGS(0x1009ef50, NCPrivateShopItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x1009ef00
NW_HOOK_DTOR(0x1009ef00, NCPrivateShopItemWnd)

// 0x1009ad90 (vslot 24) - NCItemWnd's, the double click and the dialog box's answers
INT NCPrivateShopItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    if (Msg == NWM_STORE_PRICE_CANCEL)
        return OnPriceCancel(wParam, lParam) != 0;
    if (Msg == NWM_STORE_PRICE)
        return OnPrice(wParam, lParam) != 0;
    if (Msg == NWM_STORE_AMOUNT)
        return OnAmount(wParam, lParam) != 0;
    if (Msg == NWM_STORE_PRICE_OK)
        return OnPriceOK(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1009ad90, NCPrivateShopItemWnd, WindowProc)

// stock body shared with NCDeliverItemWnd's (0x100a39e0)
INT NCPrivateShopItemWnd::OnDestroy()
{
    NCItemWnd::OnDestroy();
    return 1;
}

// 0x1009ef90 (vslot 57) - an item carried onto the other grid moves there: at someone's store
// (Mode 0) a stack asks how many; setting one's own up, going on sale asks the price, a stack
// going back how many (with more than one)
INT NCPrivateShopItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
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
            if (E && E->Wnd && E->Wnd->IsA(NCPrivateShopItemWnd::StaticClass()))
            {
                Partner = (NCPrivateShopItemWnd*)E->Wnd;
                NItemEntry* Item = Items(Selected);
                if (Mode == 0)
                {
                    if (Item->IsStackable())
                        AskAmount(this, Item);
                    else
                        MoveItem(Item, 1);
                }
                else if (Partner->Side == 0)
                    AskPrice(this);
                else if (Item->IsStackable() && Item->Count > 1)
                    AskAmount(this, Item);
                else
                    MoveItem(Item, 1);
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1009ef90, NCPrivateShopItemWnd, OnLButtonUp)

// 0x1009bf30 (vslot 67) - the tool tip box with the price on the store's list (NItemToolTip.h)
void NCPrivateShopItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
    ItemTip::DrawLines<1024, 0, 1024, 0, 0, 1>(this, Device, Item, X, Y, Side == 0, 0);
}
NW_HOOK_METHOD(0x1009bf30, NCPrivateShopItemWnd, DrawItemToolTip)

// 0x1009f290 (vslot 68) - OnLButtonUp's moves, onto Partner (a stack going back asks how many
// whatever its count)
INT NCPrivateShopItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemEntry* Item = Items(Selected);
        if (Mode == 0)
        {
            if (Item->IsStackable())
                AskAmount(this, Item);
            else
                MoveItem(Item, 1);
        }
        else if (Partner->Side == 0)
            AskPrice(this);
        else if (Item->IsStackable())
            AskAmount(this, Item);
        else
            MoveItem(Item, 1);
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1009f290, NCPrivateShopItemWnd, OnLButtonDblClick)

// 0x1009f600 (vslot 70) - the price typed for the selected item (setting one's own store up):
// it takes it; a price 5 times away from the reference asks again (0: the box stays), then a
// stack how many
INT NCPrivateShopItemWnd::OnPrice(WPARAM wParam, LPARAM lParam)
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
        Info.ID = DIALOG_STORE_PRICE_CAUTION;
        Info.bMainBox = 1;
        Info.bInput = 0;
        Info.Owner = Entry;
        Info.SetSystemMessage(MSG_PriceCaution);
        Info.bLargeAmount = 0;
        GConsole->ShowDialog(&Info);
        return 0;
    }
    if (Item->IsStackable() && Item->TradeCount > 1)
    {
        AskAmount(this, Item);
        return 0;
    }
    MoveItem(Item, 1);
    return 1;
}
NW_HOOK_METHOD(0x1009f600, NCPrivateShopItemWnd, OnPrice)

// 0x1009f7e0 (vslot 71) - the amount typed: that many of the selected item move (at most all)
INT NCPrivateShopItemWnd::OnAmount(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    INT Amount = (INT)wParam;
    if (Item && Amount)
    {
        if (Mode == 1 && Amount > 0)
            Unknown124 = 0;
        else if (Mode != 0 || Amount <= 0)
            return 1;
        MoveItem(Item, Amount < Item->TradeCount ? Amount : Item->TradeCount);
    }
    return 1;
}
NW_HOOK_METHOD(0x1009f7e0, NCPrivateShopItemWnd, OnAmount)

// 0x1009f4f0 (vslot 72) - the unusual price confirmed: a stack asks how many, anything else
// moves (a missing item too, as in the stock code)
INT NCPrivateShopItemWnd::OnPriceOK(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    if (Item && Item->IsStackable() && Item->TradeCount > 1)
    {
        AskAmount(this, Item);
        return 0;
    }
    MoveItem(Item, 1);
    return 1;
}
NW_HOOK_METHOD(0x1009f4f0, NCPrivateShopItemWnd, OnPriceOK)

// stock body shared with NCTradeItemWnd::SetKind (0x10057ba0)
void NCPrivateShopItemWnd::SetPartner(NCPrivateShopItemWnd* InPartner)
{
    Partner = InPartner;
}

// stock body shared with NCStoreItemWnd::SetMode (0x100b3880)
void NCPrivateShopItemWnd::SetMode(INT InMode)
{
    Mode = InMode;
}

// stock body shared with NCStoreItemWnd::SetSide (0x100b3890)
void NCPrivateShopItemWnd::SetSide(INT InSide)
{
    Side = InSide;
}

// 0x1009ae80 - the entry's name tip and text, with the price on the store's list
// (NItemToolTip.h)
void NCPrivateShopItemWnd::SetToolTip(NItemEntry* Item)
{
    ItemTip::SetStoreTexts<0>(this, Item, Mode, Side);
}
NW_HOOK_METHOD(0x1009ae80, NCPrivateShopItemWnd, SetToolTip)

// 0x1009e560 - the icon comes from its name ("NWindow.BlackTexture" when missing), written back
// into Info; the whole count is there
UBOOL NCPrivateShopItemWnd::AddItem(NItemInfo* Info)
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
NW_HOOK_METHOD(0x1009e560, NCPrivateShopItemWnd, AddItem)

// 0x1009e660 - a copy of Src with Count there
UBOOL NCPrivateShopItemWnd::AddEntry(NItemEntry* Src, INT Count)
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
NW_HOOK_METHOD(0x1009e660, NCPrivateShopItemWnd, AddEntry)

// 0x1009e730 - Count of the selected Item to Partner: a stack joins the same item there (going
// on sale it takes this price), anything else a copy; what is left stays, or the entry leaves
// this grid (not freed, as in the stock code). The window adds up its price again.
void NCPrivateShopItemWnd::MoveItem(NItemEntry* Item, INT Count)
{
    if (!Partner)
        return;
    UnselectAll();
    if ((Mode == 1 || Mode == 0) && (Partner->Side == 0 || Partner->Side == 1))
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
            if (Mode == 1 && Partner->Side == 0)
                Same->UnknownC20 = Item->UnknownC20;
            Partner->SetToolTip(Same);
        }
        else
            Partner->AddEntry(Item, Count);
        if (Item->IsStackable() && Item->TradeCount > Count)
        {
            Item->TradeCount -= Count;
            SetToolTip(Item);
        }
        else
            Items.Remove(Selected);
    }
    GDesktop->PrivateShopWnd->UpdateTotal();
    Selected = -1;
    UpdateScrollRange();
}
NW_HOOK_METHOD(0x1009e730, NCPrivateShopItemWnd, MoveItem)

NW_CHECK_VTABLE(NCPrivateShopItemWnd, 0x100ed340, 73)

/*-----------------------------------------------------------------------------
    NCPrivateShopWnd.
-----------------------------------------------------------------------------*/

// 0x1009d9d0 - Mode and the total are left as allocated
NCPrivateShopWnd::NCPrivateShopWnd()
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
NW_HOOK_CTOR(0x1009d9d0, NCPrivateShopWnd)

// 0x1009da70
NW_HOOK_DTOR(0x1009da70, NCPrivateShopWnd)

// 0x1009d7e0 (vslot 24)
INT NCPrivateShopWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_STORE_START)
            return OnStart() != 0;
        if (wParam == NID_STORE_UP)
            return OnUp() != 0;
        if (wParam == NID_STORE_DOWN)
            return OnDown() != 0;
        if (wParam == NID_STORE_CLOSE)
            return OnClose() != 0;
        if (wParam == NID_STORE_MESSAGE)
            return OnMessage() != 0;
        if (wParam == NID_STORE_ABORT)
            return OnAbort() != 0;
    }
    else if (Msg == NWM_STORE_BUY_OK)
        return OnBuyConfirmed(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1009d7e0, NCPrivateShopWnd, WindowProc)

// 0x1009d910
void NCPrivateShopWnd::SetAdena(INT InAdena)
{
    Adena = InAdena;
}
NW_HOOK_METHOD(0x1009d910, NCPrivateShopWnd, SetAdena)

// stock body shared with NCShopWnd::SetAdena (0x100a5d20); the console sets it
void NCPrivateShopWnd::SetOwnerID(INT InOwnerID)
{
    OwnerID = InOwnerID;
}

// 0x1009d920 - one body with the buy window's (its fields sit at the same places)
void NCPrivateShopWnd::Reset()
{
    OwnerID = -1;
    Adena = 0;
    TotalPrice = 0;
    PlayerItems->Clear();
    StoreItems->Clear();
}
NW_HOOK_METHOD(0x1009d920, NCPrivateShopWnd, Reset)

// 0x1009d970 (vslot 65) - closing one's own store also tells the server (NetHandler slot 197)
INT NCPrivateShopWnd::OnClose()
{
    if (Mode == 1)
        GConsole->RequestPrivateStoreManageCancel();
    Reset();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x1009d970, NCPrivateShopWnd, OnClose)

// 0x1009d9a0 (vslot 67) - Abort: the store closes (NetHandler slot 198)
INT NCPrivateShopWnd::OnAbort()
{
    GConsole->RequestPrivateStoreQuitSell();
    Reset();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x1009d9a0, NCPrivateShopWnd, OnAbort)

// 0x1009dac0 (vslot 50) - the background, the grids' labels by mode, then the list's price and
// the player's adena (thousands separated, right-aligned to 222). No guard in the stock code.
INT NCPrivateShopWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 344, 0, 0, 240, 344, BackTex);
    SIZE Size;
    if (Mode == 0)
    {
        Device->DrawText(15, 25, 0xFFFFFFFF, *GL2GameData.SysString[STR_ItemsOnSale], 0, 0, 0, L2FONT_Normal,
                         0, 0, 0, 0, 0, 0);
        Device->DrawText(15, 159, 0xFFFFFFFF, *GL2GameData.SysString[STR_ItemsToBuy], 0, 0, 0, L2FONT_Normal,
                         0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_Price], 0, 0, 0, 0, 0);
        Device->DrawText(132 - Size.cx, 294, 0xFFFFFFFF, *GL2GameData.SysString[STR_Price], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_Adena], 0, 0, 0, 0, 0);
        Device->DrawText(132 - Size.cx, 312, 0xFFFFFFFF, *GL2GameData.SysString[STR_Adena], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else if (Mode == 1)
    {
        Device->DrawText(15, 25, 0xFFFFFFFF, *GL2GameData.SysString[STR_Inventory], 0, 0, 0, L2FONT_Normal,
                         0, 0, 0, 0, 0, 0);
        Device->DrawText(15, 159, 0xFFFFFFFF, *GL2GameData.SysString[STR_ItemsOnSale], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_PriceSell], 0, 0, 0, 0, 0);
        Device->DrawText(132 - Size.cx, 294, 0xFFFFFFFF, *GL2GameData.SysString[STR_PriceSell], 0, 0, 0,
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
NW_HOOK_METHOD(0x1009dac0, NCPrivateShopWnd, OnPaint)

// 0x1009df20 - the store's list on top at someone's store, below when setting one's own up
void NCPrivateShopWnd::UpdateLayout()
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
NW_HOOK_METHOD(0x1009df20, NCPrivateShopWnd, UpdateLayout)

// 0x1009e020 (vslot 62) - Start: the store opens with the items on sale (their count, then the
// object id, count and price of each; NetHandler slot 196). OK at someone's store: the items
// chosen are bought (the owner's object id first; slot 201), unusual prices asking first.
INT NCPrivateShopWnd::OnStart()
{
    if (Mode == 1)
    {
        INT Count = 0;
        for (INT i = 0; i < StoreItems->Items.Num(); i++)
            if (StoreItems->Items(i)->TradeCount > 0)
                Count++;
        L2ParamStack Params(Count * 3 + 1);
        Params.PushBack((void*)(INT_PTR)Count);
        for (INT i = 0; i < StoreItems->Items.Num(); i++)
        {
            NItemEntry* Item = StoreItems->Items(i);
            if (Item->TradeCount > 0)
            {
                Params.PushBack((void*)(INT_PTR)Item->ObjectID);
                Params.PushBack((void*)(INT_PTR)Item->TradeCount);
                Params.PushBack((void*)(INT_PTR)Item->UnknownC20);
            }
        }
        GConsole->RequestSetPrivateStoreListSell(Params);
        Reset();
        HideWindow();
    }
    else if (Mode == 0)
    {
        INT Count = 0;
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
            if (PlayerItems->Items(i)->TradeCount > 0)
                Count++;
        L2ParamStack Params(Count * 3 + 2);
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
                Params.PushBack((void*)(INT_PTR)Item->TradeCount);
                Params.PushBack((void*)(INT_PTR)Item->UnknownC20);
            }
        }
        if (bUsual)
        {
            GConsole->RequestPrivateStoreBuy(Params);
            Reset();
            HideWindow();
        }
        else
        {
            NDialogInfo Info;
            Info.Type = 3;
            Info.ID = DIALOG_STORE_BUY_CAUTION;
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
NW_HOOK_METHOD(0x1009e020, NCPrivateShopWnd, OnStart)

// 0x1009e380 (vslot 66) - Message: the store's message, typed in the desktop's box (dialog
// 0x23 hands it to the console)
INT NCPrivateShopWnd::OnMessage()
{
    NDialogInfo Info;
    Info.ID = DIALOG_STORE_MESSAGE;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.Owner = PlayerItems->Entry;
    Info.SetSystemMessage(MSG_StoreMessage);
    Info.bLargeAmount = 0;
    Info.Type = 0;
    GConsole->ShowDialog(&Info);
    return 1;
}
NW_HOOK_METHOD(0x1009e380, NCPrivateShopWnd, OnMessage)

// 0x1009e3f0 (vslot 68) - buying at the unusual prices confirmed
INT NCPrivateShopWnd::OnBuyConfirmed(WPARAM wParam, LPARAM lParam)
{
    if (Mode == 0)
    {
        INT Count = 0;
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
            if (PlayerItems->Items(i)->TradeCount > 0)
                Count++;
        L2ParamStack Params(Count * 3 + 2);
        Params.PushBack((void*)(INT_PTR)OwnerID);
        Params.PushBack((void*)(INT_PTR)Count);
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
        {
            NItemEntry* Item = PlayerItems->Items(i);
            if (Item->TradeCount > 0)
            {
                Params.PushBack((void*)(INT_PTR)Item->ObjectID);
                Params.PushBack((void*)(INT_PTR)Item->TradeCount);
                Params.PushBack((void*)(INT_PTR)Item->UnknownC20);
            }
        }
        GConsole->RequestPrivateStoreBuy(Params);
    }
    Reset();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x1009e3f0, NCPrivateShopWnd, OnBuyConfirmed)

// 0x1009ea30 - both grids take the mode; they move to its places
void NCPrivateShopWnd::SetMode(INT InMode)
{
    Mode = InMode;
    StoreItems->SetMode(Mode);
    PlayerItems->SetMode(Mode);
    UpdateLayout();
}
NW_HOOK_METHOD(0x1009ea30, NCPrivateShopWnd, SetMode)

// 0x1009ea70 - one's own list adds up again
void NCPrivateShopWnd::AddStoreItem(NItemInfo* Info)
{
    StoreItems->AddItem(Info);
    if (Mode == 1)
        UpdateTotal();
}
NW_HOOK_METHOD(0x1009ea70, NCPrivateShopWnd, AddStoreItem)

// 0x1009eaa0
UBOOL NCPrivateShopWnd::AddPlayerItem(NItemInfo* Info)
{
    return PlayerItems->AddItem(Info);
}
NW_HOOK_METHOD(0x1009eaa0, NCPrivateShopWnd, AddPlayerItem)

// 0x1009eab0 (vslot 63) - the up arrow: the selected item of the lower grid goes up (a stack
// of more than one asks how many)
INT NCPrivateShopWnd::OnUp()
{
    if (Mode == 0)
    {
        INT Sel = PlayerItems->GetSelected();
        if (Sel >= 0 && Sel < PlayerItems->Items.Num())
        {
            NItemEntry* Item = PlayerItems->Items(Sel);
            if (Item)
            {
                if (Item->IsStackable() && Item->TradeCount > 1)
                    AskAmount(PlayerItems, Item);
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
                    AskAmount(StoreItems, Item);
                else
                    StoreItems->MoveItem(Item, 1);
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x1009eab0, NCPrivateShopWnd, OnUp)

// 0x1009ed00 (vslot 64) - the down arrow: at someone's store the selected item comes down (a
// stack asks how many); setting one's own up, the price is asked first
INT NCPrivateShopWnd::OnDown()
{
    if (Mode == 0)
    {
        INT Sel = StoreItems->GetSelected();
        if (Sel >= 0 && Sel < StoreItems->Items.Num())
        {
            NItemEntry* Item = StoreItems->Items(Sel);
            if (Item)
            {
                if (Item->IsStackable())
                    AskAmount(StoreItems, Item);
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
NW_HOOK_METHOD(0x1009ed00, NCPrivateShopWnd, OnDown)

// 0x1009f840 (vslot 35) - the title bar, the two grids (partners of each other), the arrows
// and the three buttons
INT NCPrivateShopWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.StoreWnd.StoreBack"));

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_STORE_FRAME;
    FrameCtrl->ID = NID_STORE_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    StoreItems = new NCPrivateShopItemWnd(6, 3, 80, 32);
    StoreItems->SetWindowRect(17, 180, 222, 102);
    CreateChild(StoreItems, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    StoreItems->SetSide(0);

    PlayerItems = new NCPrivateShopItemWnd(6, 3, 80, 32);
    PlayerItems->SetWindowRect(17, 46, 222, 102);
    CreateChild(PlayerItems, NWS_VISIBLE | 0x20);
    PlayerItems->SetSide(1);

    StoreItems->SetPartner(PlayerItems);
    PlayerItems->SetPartner(StoreItems);

    UpButton = new NCButton(TEXT("L2UI.StoreWnd.StoreUpBtn1"), TEXT("L2UI.StoreWnd.StoreUpBtn1"), 0);
    UpButton->ID = NID_STORE_UP;
    UpButton->SetWindowRect(194, 158, 14, 13);
    CreateChild(UpButton, NWS_VISIBLE);

    DownButton = new NCButton(TEXT("L2UI.StoreWnd.StoreDownBtn1"), TEXT("L2UI.StoreWnd.StoreDownBtn2"), 0);
    DownButton->ID = NID_STORE_DOWN;
    DownButton->SetWindowRect(210, 158, 14, 13);
    CreateChild(DownButton, NWS_VISIBLE);

    StartButton = new NCButton(TEXT("L2UI.Control.DefaultButton"), TEXT("L2UI.Control.DefaultButton_click"), 0);
    StartButton->ID = NID_STORE_START;
    StartButton->SetWindowRect(164, 331, 67, 19);
    CreateChild(StartButton, NWS_VISIBLE);
    StartButton->SetTextID(STR_Start);

    MessageButton = new NCButton(TEXT("L2UI.Control.DefaultButton"), TEXT("L2UI.Control.DefaultButton_click"), 0);
    MessageButton->ID = NID_STORE_MESSAGE;
    MessageButton->SetWindowRect(87, 331, 67, 19);
    CreateChild(MessageButton, NWS_VISIBLE);
    MessageButton->SetTextID(STR_Message);

    AbortButton = new NCButton(TEXT("L2UI.Control.DefaultButton"), TEXT("L2UI.Control.DefaultButton_click"), 0);
    AbortButton->ID = NID_STORE_ABORT;
    AbortButton->SetWindowRect(10, 331, 67, 19);
    CreateChild(AbortButton, NWS_VISIBLE);
    AbortButton->SetTextID(STR_Abort);
    return 1;
}
NW_HOOK_METHOD(0x1009f840, NCPrivateShopWnd, OnCreate)

// stock body shared with NCPrivateBuyWnd's (0x10098e10)
INT NCPrivateShopWnd::OnDestroy()
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

// stock body shared with NCPrivateBuyWnd's (0x100982c0)
void NCPrivateShopWnd::ShowWindow()
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

// stock body shared with NCPrivateBuyWnd's (0x1009d960)
void NCPrivateShopWnd::HideWindow()
{
    Reset();
    NCWnd::HideWindow();
}

// stock body shared with NCPrivateBuyWnd's (0x10099360): the list's price, each entry's price
// times its count, in 64 bits
void NCPrivateShopWnd::UpdateTotal()
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

NW_CHECK_VTABLE(NCPrivateShopWnd, 0x100ed228, 69)
