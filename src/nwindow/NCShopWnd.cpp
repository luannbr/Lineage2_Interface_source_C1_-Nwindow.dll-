/*=============================================================================
    NCShopWnd.cpp - an NPC's shop window and its item grids.
    Stock: 0x100a39e0 .. 0x100a7fee. WindowProc (0x10057b10) and SetTarget (0x10057ba0) of
    the grid, WindowProc (0x100b56e0), ShowWindow (0x100a5d70) and SetListID (0x100b5800) of
    the window are copies of bodies other classes keep the addresses of, and so are SetMode
    (0x100b3880) and SetSide (0x100b3890) of the grid (hooked as NCStoreItemWnd's).
=============================================================================*/
#include "nwindow/NCShopWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NItemToolTip.h"

enum { DIALOG_SHOP_AMOUNT = 2 };     // NDialogInfo::ID (name ours): an amount for the owner
enum { MSG_HowManyToMove = 72 };     // systemmsg "How many $s1(s) do you want to move?"
enum { ITEM_Adena = 57 };

// sysstring labels
enum
{
    STR_Adena = 134,
    STR_ItemsOnSale = 137,
    STR_Inventory = 138,
    STR_ItemsToBuy = 139,
    STR_OK = 140,
    STR_Cancel = 141,
    STR_BuyPrice = 142,              // "Price"
    STR_SellPrice = 143,             // "Price"
    STR_TipPrice = 322,              // "Price"
    STR_ForEach = 468,
    STR_TipAdena = 469,              // "Adena"
};

// the shop window
static NCShopWnd* ShopWnd()
{
    return GDesktop->ShopWnd;
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
    Info.ID = DIALOG_SHOP_AMOUNT;
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
    NCShopItemWnd.
-----------------------------------------------------------------------------*/

// 0x100a7480 - Mode and Side are left as allocated
NCShopItemWnd::NCShopItemWnd()
{
    Target = NULL;
}
NW_HOOK_CTOR(0x100a7480, NCShopItemWnd)

// 0x100a7520 - the shop's grids are 6 x 3 icons of 32 pixels, 200 items at most
NCShopItemWnd::NCShopItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
    Target = NULL;
}
NW_HOOK_CTOR_ARGS(0x100a7520, NCShopItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x100a74d0
NW_HOOK_DTOR(0x100a74d0, NCShopItemWnd)

// stock body shared with NCInvenItemWnd's (0x10057b10): the double click and the dialog box's
// answer
INT NCShopItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
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

// 0x100a39e0 (vslot 36) - one body with the warehouse's and other item grids' (NCStoreItemWnd,
// NCDeliverItemWnd...)
INT NCShopItemWnd::OnDestroy()
{
    NCItemWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100a39e0, NCShopItemWnd, OnDestroy)

// stock body shared with NCTradeItemWnd::SetKind (0x10057ba0)
void NCShopItemWnd::SetTarget(NCShopItemWnd* InTarget)
{
    Target = InTarget;
}

// stock body shared with NCStoreItemWnd's (0x100b3880)
void NCShopItemWnd::SetMode(INT InMode)
{
    Mode = InMode;
}

// stock body shared with NCStoreItemWnd's (0x100b3890)
void NCShopItemWnd::SetSide(INT InSide)
{
    Side = InSide;
}

// 0x100a39f0 - the name tip with the price ("For Each" on a stack; when buying, the NPC's
// goods show no amount), and the text the box is sized with: name, price, then the
// inventory's lines (stacks outside the NPC's list: their amount and the price for each, the
// weapon without the spiritshots, the shield with a stray "%s" in its format, as in the stock
// code). Korean labels written in the code; the buffers are the stock frame's.
void NCShopItemWnd::SetToolTip(NItemEntry* Item)
{
    if (!Item)
        return;
    TCHAR Num[1024];
    TCHAR Formatted[1024];
    TCHAR Num2[1024];
    TCHAR Formatted2[1024];
    const TCHAR* Adena = *GL2GameData.SysString[STR_TipAdena];
    if (Mode == SHOPMODE_Buy && Side == SHOPSIDE_Shop)
    {
        appSprintf(Num, TEXT("%d"), Item->GetUnknownC20());
        FormatNumber(Formatted, Num);
        if (Item->IsStackable())
            appSprintf(Item->ToolTipName, TEXT("%s\n\xAC00\xACA9 : %s %s %s"), Item->GetName(),
                       *GL2GameData.SysString[STR_ForEach], Formatted, Adena);
        else
            appSprintf(Item->ToolTipName, TEXT("%s\n\xAC00\xACA9 : %s %s"), Item->GetName(), Formatted,
                       Adena);
    }
    else if (Item->IsStackable())
    {
        appSprintf(Num, TEXT("%d"), Item->GetUnknownC20());
        FormatNumber(Formatted, Num);
        appSprintf(Item->ToolTipName, TEXT("%s(%d)\n\xAC00\xACA9 : %s %s %s"), Item->GetName(),
                   Item->TradeCount, *GL2GameData.SysString[STR_ForEach], Formatted, Adena);
    }
    else
    {
        appSprintf(Num2, TEXT("%d"), Item->GetUnknownC20());
        FormatNumber(Formatted2, Num2);
        appSprintf(Item->ToolTipName, TEXT("%s\n\xAC00\xACA9 : %s %s"), Item->GetName(), Formatted2,
                   Adena);
    }

    FString Desc;
    GConsole->ConvertNewLines((TCHAR*)Item->GetDescription(), Desc);
    FString Slot;
    GConsole->GetItemSlotText(Item->Type, Item->BodyPart, Slot, Item->SubType);
    TCHAR* Out = Item->ToolTipText;
    if ((Mode == SHOPMODE_Buy && Side == SHOPSIDE_Shop) || !Item->IsStackable())
    {
        // the price of one
        INT Price = Item->GetUnknownC20();
        if (Item->Type == 0)
            appSprintf(Out, TEXT("%s\n\xAC00\xACA9 : %d %s\n%s / %s\n\xACF5\xACA9\xB825 : %d\n\xB9C8\xBC95\xB825 : %d\n\xACF5\xACA9\xC18D\xB3C4 : %s\n\xC815\xB839\xD0C4\xC18C\xBAA8 : %d\n\xB9C8\xC815\xD0C4\xC18C\xBAA8 : %d\n\xBB34\xAC8C : %d\n%s"),
                       Item->GetName(), Price, Adena, ItemTip::GetWeaponTypeName(Item->UnknownC30),
                       *Slot, Item->Unknown1C80, Item->Unknown1C84,
                       ItemTip::GetAttackSpeedText(Item->Unknown1C6C), Item->Unknown1CA0,
                       Item->Unknown1CA4, Item->Weight, *Desc);
        else if (Item->Type == 1)
        {
            if ((Item->BodyPart & 0x100) || (Item->BodyPart & 0x80))
                appSprintf(Out, TEXT("%s\n\xAC00\xACA9 : %d %s\n%s\n\xBC29\xC5B4\xB825 : %d\n\xBC29\xC5B4\xC131\xACF5 : %d\n\xD68C\xD53C : %d\n\xBB34\xAC8C : %d\n%s"),
                           Item->GetName(), Price, Adena, *Slot, Item->Unknown1C90, Item->Unknown1C94,
                           Item->Unknown1C78, Item->Weight, *Desc);
            else if (ItemTip::IsMagicArmor(Item->ID))
                appSprintf(Out, TEXT("%s\n\xAC00\xACA9 : %d %s\n%s\nMP\xC99D\xAC00 : %d\n\xBC29\xC5B4\xB825 : %d\n\xBB34\xAC8C : %d\n%s"),
                           Item->GetName(), Price, Adena, *Slot, Item->Unknown1C9C, Item->Unknown1C88,
                           Item->Weight, *Desc);
            else
                appSprintf(Out, TEXT("%s\n\xAC00\xACA9 : %d %s\n%s\n\xBC29\xC5B4\xB825 : %d\n\xBB34\xAC8C : %d\n%s"),
                           Item->GetName(), Price, Adena, *Slot, Item->Unknown1C88, Item->Weight,
                           *Desc);
        }
        else if (Item->Type == 2)
            appSprintf(Out, TEXT("%s\n\xAC00\xACA9 : %d %s\n%s\n\xBC29\xC5B4\xB825 : %d\n\xBB34\xAC8C : %d\n%s"),
                       Item->GetName(), Price, Adena, *Slot, Item->Unknown1C8C, Item->Weight, *Desc);
        else if (Item->Type == 3)
            appSprintf(Out, TEXT("%s\n\xAC00\xACA9 : %d %s\n%s\n%s"), Item->GetName(), Price, Adena,
                       *Slot, *Desc);
        else if (Item->Type == 5)
            appSprintf(Out, TEXT("%s\n\xAC00\xACA9 : %d %s\n\xBB34\xAC8C : %d\n%s"), Item->GetName(),
                       Price, Adena, Item->Weight, *Desc);
        else
            appSprintf(Out, TEXT("%s\n\xAC00\xACA9 : %d %s\n%s"), Item->GetName(), Price, Adena,
                       *Desc);
    }
    else
    {
        // a stack moved: its amount, the price for each
        const TCHAR* ForEach = *GL2GameData.SysString[STR_ForEach];
        INT Price = Item->GetUnknownC20();
        if (Item->Type == 0)
            appSprintf(Out, TEXT("%s(%d)\n\xAC00\xACA9 : %s %d %s\n%s / %s\n\xACF5\xACA9\xB825 : %d\n\xB9C8\xBC95\xB825 : %d\n\xACF5\xACA9\xC18D\xB3C4 : %s\n\xC815\xB839\xD0C4\xC18C\xBAA8 : %d\n\xBB34\xAC8C : %d\n%s"),
                       Item->GetName(), Item->TradeCount, ForEach, Price, Adena,
                       ItemTip::GetWeaponTypeName(Item->UnknownC30), *Slot, Item->Unknown1C80,
                       Item->Unknown1C84, ItemTip::GetAttackSpeedText(Item->Unknown1C6C),
                       Item->Unknown1CA0, Item->Weight, *Desc);
        else if (Item->Type == 1)
        {
            if ((Item->BodyPart & 0x100) || (Item->BodyPart & 0x80))
                appSprintf(Out, TEXT("%s(%d)\n%s \xAC00\xACA9 : %s %d %s\n%s\n\xBC29\xC5B4\xB825 : %d\n\xBC29\xC5B4\xC131\xACF5 : %d\n\xD68C\xD53C : %d\n\xBB34\xAC8C : %d\n%s"),
                           Item->GetName(), Item->TradeCount, ForEach, Price, Adena, *Slot,
                           Item->Unknown1C90, Item->Unknown1C94, Item->Unknown1C78, Item->Weight,
                           *Desc);
            else if (ItemTip::IsMagicArmor(Item->ID))
                appSprintf(Out, TEXT("%s(%d)\n\xAC00\xACA9 : %s %d %s\n%s\nMP\xC99D\xAC00 : %d\n\xBC29\xC5B4\xB825 : %d\n\xBB34\xAC8C : %d\n%s"),
                           Item->GetName(), Item->TradeCount, ForEach, Price, Adena, *Slot,
                           Item->Unknown1C9C, Item->Unknown1C88, Item->Weight, *Desc);
            else
                appSprintf(Out, TEXT("%s(%d)\n\xAC00\xACA9 : %s %d %s\n%s\n\xBC29\xC5B4\xB825 : %d\n\xBB34\xAC8C : %d\n%s"),
                           Item->GetName(), Item->TradeCount, ForEach, Price, Adena, *Slot,
                           Item->Unknown1C88, Item->Weight, *Desc);
        }
        else if (Item->Type == 2)
            appSprintf(Out, TEXT("%s(%d)\n\xAC00\xACA9 : %s %d %s\n%s\n\xBC29\xC5B4\xB825 : %d\n\xBB34\xAC8C : %d\n%s"),
                       Item->GetName(), Item->TradeCount, ForEach, Price, Adena, *Slot,
                       Item->Unknown1C8C, Item->Weight, *Desc);
        else if (Item->Type == 3)
            appSprintf(Out, TEXT("%s(%d)\n\xAC00\xACA9 : %s %d %s\n%s\n%s"), Item->GetName(),
                       Item->TradeCount, ForEach, Price, Adena, *Slot, *Desc);
        else if (Item->Type == 5)
            appSprintf(Out, TEXT("%s(%d)\n\xAC00\xACA9 : %s %d %s\n\xBB34\xAC8C : %d\n%s"),
                       Item->GetName(), Item->TradeCount, ForEach, Price, Adena, Item->Weight, *Desc);
        else
            appSprintf(Out, TEXT("%s(%d)\n\xAC00\xACA9 : %s %d %s\n%s"), Item->GetName(),
                       Item->TradeCount, ForEach, Price, Adena, *Desc);
    }
}
NW_HOOK_METHOD(0x100a39f0, NCShopItemWnd, SetToolTip)

// the name ("name(amount)" for a stack, no thousands separators) after the enchant's width,
// the grade after it (inline three times in the stock DrawItemToolTip)
static FORCEINLINE void DrawNameLine(NCGDevice* Device, NItemEntry* Item, TCHAR* Text,
                                     SIZE& Size, INT X, INT Y, INT EnchantW)
{
    if (Item->IsStackable())
        appSprintf(Text, TEXT("%s(%d)"), Item->GetName(), Item->TradeCount);
    else
        appSprintf(Text, TEXT("%s"), Item->GetName());
    ItemTip::DrawString(Device, X + EnchantW, Y, 0xFFFFFFFF, Text);
    if (ItemTip::GetGradeText(Item->CrystalGrade))
    {
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
        appSprintf(Text, TEXT("`%s`"), ItemTip::GetGradeText(Item->CrystalGrade));
        Device->DrawText(X + EnchantW + Size.cx + 2, Y, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 1,
                         0, 0, 0, 0, 0);
    }
}

// 0x100a4540 (vslot 67) - the inventory's tool tip box with a price line under the name:
// "+enchant " (in the NPC's list when buying, for any type; elsewhere for weapons, armor,
// accessories and quest items), the name, "Price : " ("Price : For Each" for a stack) and the
// price with thousands separators, then the lines of the type, the weight (no pet collar
// level) and the description. The buffers are the stock frame's.
void NCShopItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
    if (!Item)
        return;
    FString Desc;
    GConsole->ConvertNewLines((TCHAR*)Item->GetDescription(), Desc);
    FString Slot;
    GConsole->GetItemSlotText(Item->Type, Item->BodyPart, Slot, Item->SubType);
    TCHAR Text[1024];
    TCHAR Formatted[1024];
    SIZE Size;

    UBOOL bEnchant;
    if (Mode == SHOPMODE_Buy && Side == SHOPSIDE_Shop)
        bEnchant = Item->Enchant > 0;
    else
        bEnchant = Item->Enchant > 0
                   && (Item->Type == 0 || Item->Type == 1 || Item->Type == 3 || Item->Type == 2);
    INT EnchantW = 0;
    if (bEnchant)
    {
        appSprintf(Text, TEXT("+%d "), Item->Enchant);
        ItemTip::DrawString(Device, X, Y, 0xFFB09B79, Text);
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
        EnchantW = Size.cx;
    }
    DrawNameLine(Device, Item, Text, Size, X, Y, EnchantW);
    Y += 13;

    if (Item->IsStackable())
        appSprintf(Text, TEXT("%s : %s"), *GL2GameData.SysString[STR_TipPrice],
                   *GL2GameData.SysString[STR_ForEach]);
    else
        appSprintf(Text, TEXT("%s : "), *GL2GameData.SysString[STR_TipPrice]);
    ItemTip::DrawString(Device, X, Y, 0xFFA3A3A3, Text);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT("%d"), Item->UnknownC20);
    FormatNumber(Formatted, Text);
    appSprintf(Text, TEXT("%s %s"), Formatted, *GL2GameData.SysString[STR_TipAdena]);
    ItemTip::DrawString(Device, X + Size.cx, Y, 0xFFB09B79, Text);
    Y += 13;

    ItemTip::DrawTypeLines(Device, Item, Text, Size, Slot, X, Y);
    ItemTip::DrawTailLines<1>(Device, Item, Text, Size, Slot, Desc, X, Y);
}
NW_HOOK_METHOD(0x100a4540, NCShopItemWnd, DrawItemToolTip)

// 0x100a7560 (vslot 57) - the carried item dropped onto another shop grid goes there (that
// grid becomes the target): how many first for a stack (from the NPC's goods when buying,
// whatever its amount), else one
INT NCShopItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
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
        if (E && E->Wnd && E->Wnd->IsA(NCShopItemWnd::StaticClass()))
        {
            Target = (NCShopItemWnd*)E->Wnd;
            NItemEntry* Item = Items(Selected);
            if (Mode == SHOPMODE_Buy && Target->Side == SHOPSIDE_Player)
            {
                if (Item->IsStackable())
                    AskAmount(this, Item);
                else
                    MoveItem(Item, 1);
            }
            else if (Item->IsStackable() && Item->TradeCount > 1)
                AskAmount(this, Item);
            else
                MoveItem(Item, 1);
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100a7560, NCShopItemWnd, OnLButtonUp)

// 0x100a7860 (vslot 68) - the item goes to the target: how many first for a stack (any stack
// when buying), else one
INT NCShopItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
    {
        NItemEntry* Item = Items(Selected);
        if (Mode == SHOPMODE_Buy)
        {
            if (Item->IsStackable())
                AskAmount(this, Item);
            else
                MoveItem(Item, 1);
        }
        else if (Item->IsStackable() && Item->TradeCount > 1)
            AskAmount(this, Item);
        else
            MoveItem(Item, 1);
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100a7860, NCShopItemWnd, OnLButtonDblClick)

// 0x100a7ab0 (vslot 69) - the amount typed (wParam; when selling at most the entry's) goes to
// the target. The stock code reads Items(Selected) without a bounds check.
INT NCShopItemWnd::OnDialogResult(WPARAM wParam, LPARAM lParam)
{
    NItemEntry* Item = Items(Selected);
    if (Item && wParam)
    {
        INT Count = (INT)wParam;
        if (Mode == SHOPMODE_Sell && Count >= Item->TradeCount)
            Count = Item->TradeCount;
        MoveItem(Item, Count);
    }
    return 1;
}
NW_HOOK_METHOD(0x100a7ab0, NCShopItemWnd, OnDialogResult)

// 0x100a69b0 - a new entry, its icon from its name ("NWindow.BlackTexture" when missing,
// written back into Info), its amount the item's count
UBOOL NCShopItemWnd::AddItem(NItemInfo* Info)
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
NW_HOOK_METHOD(0x100a69b0, NCShopItemWnd, AddItem)

// 0x100a6ab0 - a copy of another grid's entry holding Count
UBOOL NCShopItemWnd::AddCopy(NItemEntry* Src, INT Count)
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
NW_HOOK_METHOD(0x100a6ab0, NCShopItemWnd, AddCopy)

// a stack joins the target's entry of the same item id when there is one, else a copy
// holding Count joins the target (inline three times in the stock MoveItem)
static FORCEINLINE void AddToTarget(NCShopItemWnd* Target, NItemEntry* Item, INT Count)
{
    if (Item->IsStackable())
    {
        for (INT i = 0; i < Target->Items.Num(); i++)
        {
            if (Target->Items(i)->ID == Item->ID)
            {
                Target->Items(i)->TradeCount += Count;
                Target->SetToolTip(Target->Items(i));
                return;
            }
        }
    }
    Target->AddCopy(Item, Count);
}

// 0x100a6b80 - Count of the item go to the target. Selling, either way the target gets them
// and this entry loses them; buying, the NPC's goods stay (the list of purchases gets them)
// and what goes back is only taken off. An emptied entry leaves the grid (not freed, as in
// the stock code); then the window's total.
void NCShopItemWnd::MoveItem(NItemEntry* Item, INT Count)
{
    if (!Target)
        return;
    UnselectAll();
    if (Mode == SHOPMODE_Sell)
    {
        // the stock code has one identical branch for each side
        if (Target->Side == SHOPSIDE_Shop || Target->Side == SHOPSIDE_Player)
        {
            AddToTarget(Target, Item, Count);
            if (Item->IsStackable() && Item->TradeCount > Count)
            {
                Item->TradeCount -= Count;
                SetToolTip(Item);
            }
            else
                Items.Remove(Selected, 1);
        }
    }
    else if (Mode == SHOPMODE_Buy)
    {
        if (Target->Side == SHOPSIDE_Shop)
        {
            if (Item->IsStackable() && Item->TradeCount > Count)
            {
                Item->TradeCount -= Count;
                SetToolTip(Item);
            }
            else
                Items.Remove(Selected, 1);
        }
        else if (Target->Side == SHOPSIDE_Player)
            AddToTarget(Target, Item, Count);
    }
    ShopWnd()->UpdateTotalPrice();
    Selected = -1;
    UpdateScrollRange();
}
NW_HOOK_METHOD(0x100a6b80, NCShopItemWnd, MoveItem)

NW_CHECK_VTABLE(NCShopItemWnd, 0x100ee1c8, 70)

/*-----------------------------------------------------------------------------
    NCShopWnd.
-----------------------------------------------------------------------------*/

// 0x100a5db0 - Mode is left as allocated
NCShopWnd::NCShopWnd()
{
    PlayerItems = NULL;
    ShopItems = NULL;
    Unknown9C = 0;
    BackTex = NULL;
    UnknownA4 = 0;
    UpButton = NULL;
    DownButton = NULL;
    OKButton = NULL;
    CancelButton = NULL;
    UnknownB8 = NULL;
    Adena = 0;
    TotalPrice = 0;
    ListID = -1;
}
NW_HOOK_CTOR(0x100a5db0, NCShopWnd)

// 0x100a5e50
NW_HOOK_DTOR(0x100a5e50, NCShopWnd)

// stock body shared with NCStoreWnd's (0x100b56e0): the buttons
INT NCShopWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
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

// 0x100a7b00 (vslot 35) - title bar, the grids (6 x 3 icons, 200 items at most: the NPC's on
// top, the player's below), the arrows between them, OK and Cancel
INT NCShopWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.StoreWnd.StoreBack"));

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_STORE_FRAME;
    FrameCtrl->ID = NID_STORE_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    ShopItems = new NCShopItemWnd(6, 3, 200, 32);
    ShopItems->SetWindowRect(16, 46, 222, 102);
    CreateChild(ShopItems, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    ShopItems->SetSide(SHOPSIDE_Shop);
    PlayerItems = new NCShopItemWnd(6, 3, 200, 32);
    PlayerItems->SetWindowRect(16, 180, 222, 102);
    CreateChild(PlayerItems, NWS_VISIBLE | 0x20);
    PlayerItems->SetSide(SHOPSIDE_Player);
    ShopItems->SetTarget(PlayerItems);
    PlayerItems->SetTarget(ShopItems);

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
NW_HOOK_METHOD(0x100a7b00, NCShopWnd, OnCreate)

// 0x100a6300 (vslot 36) - one body with NCStoreWnd's (and NCGMStoreWnd's)
INT NCShopWnd::OnDestroy()
{
    DestroyChild(this, PlayerItems);
    DestroyChild(this, ShopItems);
    DestroyChild(this, UpButton);
    DestroyChild(this, DownButton);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    DestroyChild(this, UnknownB8);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100a6300, NCShopWnd, OnDestroy)

// stock body shared with NCEditBox's (0x100a5d70): the layout comes with SetMode
void NCShopWnd::ShowWindow()
{
    NCWnd::ShowWindow();
}

// 0x100a5d80 (vslot 43)
void NCShopWnd::HideWindow()
{
    Clear();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x100a5d80, NCShopWnd, HideWindow)

// 0x100a5ea0 (vslot 50) - the grids' labels by mode, the total's and the adena's labels
// right-aligned at 132, their values at 222, thousands separated (the total, 64 bits, through
// "%d": its low half); the buffers are the stock frame's
INT NCShopWnd::OnPaint(NCGDevice* Device)
{
    TCHAR Text[1024];
    TCHAR TotalText[1024];
    TCHAR AdenaText[1024];
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 344, 0, 0, 240, 344, BackTex);
    if (Mode == SHOPMODE_Buy)
    {
        Device->DrawText(15, 25, 0xFFFFFFFF, *GL2GameData.SysString[STR_ItemsOnSale], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Device->DrawText(15, 159, 0xFFFFFFFF, *GL2GameData.SysString[STR_ItemsToBuy], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_BuyPrice], 0, 0, 0, 0, 0);
        Device->DrawText(132 - Size.cx, 294, 0xFFFFFFFF, *GL2GameData.SysString[STR_BuyPrice], 0, 0,
                         0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_Adena], 0, 0, 0, 0, 0);
        Device->DrawText(132 - Size.cx, 312, 0xFFFFFFFF, *GL2GameData.SysString[STR_Adena], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else if (Mode == SHOPMODE_Sell)
    {
        Device->DrawText(15, 25, 0xFFFFFFFF, *GL2GameData.SysString[STR_Inventory], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Device->DrawText(15, 159, 0xFFFFFFFF, *GL2GameData.SysString[STR_ItemsOnSale], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_SellPrice], 0, 0, 0, 0, 0);
        Device->DrawText(132 - Size.cx, 294, 0xFFFFFFFF, *GL2GameData.SysString[STR_SellPrice], 0,
                         0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_Adena], 0, 0, 0, 0, 0);
        Device->DrawText(132 - Size.cx, 312, 0xFFFFFFFF, *GL2GameData.SysString[STR_Adena], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    appSprintf(Text, TEXT("%d"), TotalPrice);
    FormatNumber(TotalText, Text);
    appSprintf(Text, TEXT("%s"), TotalText);
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
NW_HOOK_METHOD(0x100a5ea0, NCShopWnd, OnPaint)

// the entries of a grid that hold something
static FORCEINLINE INT CountFilled(NCShopItemWnd* Grid)
{
    INT Num = 0;
    for (INT i = 0; i < Grid->Items.Num(); i++)
        if (Grid->Items(i)->TradeCount > 0)
            Num++;
    return Num;
}

// 0x100a6550 (vslot 62) - the lower grid's list to the server: list id, count, then (object
// id, item id, amount) of each item sold or (item id, amount) of each item bought; the window
// closes
INT NCShopWnd::OnOK()
{
    if (Mode == SHOPMODE_Sell)
    {
        INT Num = CountFilled(ShopItems);
        L2ParamStack Params(Num * 3 + 2);
        Params.PushBack((void*)(INT_PTR)ListID);
        Params.PushBack((void*)(INT_PTR)Num);
        for (INT i = 0; i < ShopItems->Items.Num(); i++)
        {
            if (ShopItems->Items(i)->TradeCount > 0)
            {
                Params.PushBack((void*)(INT_PTR)ShopItems->Items(i)->ObjectID);
                Params.PushBack((void*)(INT_PTR)ShopItems->Items(i)->ID);
                Params.PushBack((void*)(INT_PTR)ShopItems->Items(i)->TradeCount);
            }
        }
        GConsole->RequestSellItem(Params);
    }
    else if (Mode == SHOPMODE_Buy)
    {
        INT Num = CountFilled(PlayerItems);
        L2ParamStack Params(Num * 3 + 2);
        Params.PushBack((void*)(INT_PTR)ListID);
        Params.PushBack((void*)(INT_PTR)Num);
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
        {
            if (PlayerItems->Items(i)->TradeCount > 0)
            {
                Params.PushBack((void*)(INT_PTR)PlayerItems->Items(i)->ID);
                Params.PushBack((void*)(INT_PTR)PlayerItems->Items(i)->TradeCount);
            }
        }
        GConsole->RequestBuyItem(Params);
    }
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100a6550, NCShopWnd, OnOK)

// 0x100a5d90 (vslot 63)
INT NCShopWnd::OnCancel()
{
    Clear();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100a5d90, NCShopWnd, OnCancel)

// vslot 66 - OnCancel's stock body (0x100a5d90 serves both slots)
INT NCShopWnd::OnClose()
{
    Clear();
    HideWindow();
    return 1;
}

// the arrows: the selected item of Grid goes to its target, how many first for a stack of more
// than one - or for any stack with bAnyStack, the down arrow on the NPC's list, whose stock
// copy has no count test (inline in the stock OnUp / OnDown, twice each)
static FORCEINLINE void MoveSelected(NCShopItemWnd* Grid, UBOOL bAnyStack = 0)
{
    INT Sel = Grid->GetSelected();
    if (Sel < 0 || Sel >= Grid->Items.Num())
        return;
    NItemEntry* Item = Grid->Items(Sel);
    if (!Item)
        return;
    if (Item->IsStackable() && (bAnyStack || Item->TradeCount > 1))
        AskAmount(Grid, Item);
    else
        Grid->MoveItem(Item, 1);
}

// 0x100a6ed0 (vslot 64) - the up arrow: from the lower grid
INT NCShopWnd::OnUp()
{
    if (Mode == SHOPMODE_Buy)
        MoveSelected(PlayerItems);
    else if (Mode == SHOPMODE_Sell)
        MoveSelected(ShopItems);
    return 1;
}
NW_HOOK_METHOD(0x100a6ed0, NCShopWnd, OnUp)

// 0x100a71b0 (vslot 65) - the down arrow: from the upper grid
INT NCShopWnd::OnDown()
{
    if (Mode == SHOPMODE_Buy)
        MoveSelected(ShopItems, 1);
    else if (Mode == SHOPMODE_Sell)
        MoveSelected(PlayerItems);
    return 1;
}
NW_HOOK_METHOD(0x100a71b0, NCShopWnd, OnDown)

// 0x100a5d20 - one body with the setters of other windows' +0xC4 (the private stores')
void NCShopWnd::SetAdena(INT InAdena)
{
    Adena = InAdena;
}
NW_HOOK_METHOD(0x100a5d20, NCShopWnd, SetAdena)

// stock body shared with NCStoreWnd::SetAdena (0x100b5800)
void NCShopWnd::SetListID(INT InListID)
{
    ListID = InListID;
}

// 0x100a5d30 - kept out of line like the stock code (HideWindow, OnCancel)
__declspec(noinline) void NCShopWnd::Clear()
{
    ListID = -1;
    Adena = 0;
    TotalPrice = 0;
    PlayerItems->Clear();
    ShopItems->Clear();
}
NW_HOOK_METHOD(0x100a5d30, NCShopWnd, Clear)

// 0x100a6450 (out of line like the stock code) - buying, the NPC's grid on top and the
// purchases below; selling, the inventory on top and what is sold below
__declspec(noinline) void NCShopWnd::UpdateLayout()
{
    if (Mode == SHOPMODE_Buy)
    {
        ShopItems->UpdateScrollRange();
        PlayerItems->UpdateScrollRange();
        // the stock code fills { LocalX, LocalY, LocalW, LocalH }; SetScreenPos reads x and y
        POINT Origin;
        Origin.x = LocalX;
        Origin.y = LocalY;
        ShopItems->SetScreenPos(16, 46, &Origin);
        PlayerItems->SetScreenPos(16, 180, &Origin);
    }
    else if (Mode == SHOPMODE_Sell)
    {
        ShopItems->UpdateScrollRange();
        PlayerItems->UpdateScrollRange();
        POINT Origin;
        Origin.x = LocalX;
        Origin.y = LocalY;
        ShopItems->SetScreenPos(16, 180, &Origin);
        PlayerItems->SetScreenPos(16, 46, &Origin);
    }
    else
        return;
    ShopItems->SetSize(222, 102);
    PlayerItems->SetSize(222, 102);
}
NW_HOOK_METHOD(0x100a6450, NCShopWnd, UpdateLayout)

// 0x100a67a0 - the sum of price x amount over the lower grid, in 64 bits
void NCShopWnd::UpdateTotalPrice()
{
    TotalPrice = 0;
    if (Mode == SHOPMODE_Buy)
    {
        for (INT i = 0; i < PlayerItems->Items.Num(); i++)
            TotalPrice += (SQWORD)PlayerItems->Items(i)->GetUnknownC20()
                          * PlayerItems->Items(i)->TradeCount;
    }
    else if (Mode == SHOPMODE_Sell)
    {
        for (INT i = 0; i < ShopItems->Items.Num(); i++)
            TotalPrice += (SQWORD)ShopItems->Items(i)->GetUnknownC20()
                          * ShopItems->Items(i)->TradeCount;
    }
}
NW_HOOK_METHOD(0x100a67a0, NCShopWnd, UpdateTotalPrice)

// one grid's entries get their name and description again from the item tables, and their
// tool tips (the stock code repeats this for each grid, with the CRT's wcscpy)
static FORCEINLINE void ReloadItemTexts(NCShopItemWnd* Grid)
{
    for (INT i = 0; i < Grid->Items.Num(); i++)
    {
        NItemEntry* Item = Grid->Items(i);
        if (Item)
        {
            wcscpy(Item->Name, GL2GameData.GetItemName(Item->ID));
            wcscpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
            Grid->SetToolTip(Item);
        }
    }
}

// 0x100a6890 - after the item texts were reloaded (NConsoleWnd::ReloadGameTexts)
void NCShopWnd::ReloadTexts()
{
    ReloadItemTexts(ShopItems);
    ReloadItemTexts(PlayerItems);
}
NW_HOOK_METHOD(0x100a6890, NCShopWnd, ReloadTexts)

// 0x100a6e70
void NCShopWnd::SetMode(INT InMode)
{
    Mode = InMode;
    ShopItems->SetMode(Mode);
    PlayerItems->SetMode(Mode);
    UpdateLayout();
}
NW_HOOK_METHOD(0x100a6e70, NCShopWnd, SetMode)

// 0x100a6eb0
UBOOL NCShopWnd::AddShopItem(NItemInfo* Info)
{
    return ShopItems->AddItem(Info);
}
NW_HOOK_METHOD(0x100a6eb0, NCShopWnd, AddShopItem)

// 0x100a6ec0
UBOOL NCShopWnd::AddPlayerItem(NItemInfo* Info)
{
    return PlayerItems->AddItem(Info);
}
NW_HOOK_METHOD(0x100a6ec0, NCShopWnd, AddPlayerItem)

NW_CHECK_VTABLE(NCShopWnd, 0x100ee0b8, 67)
