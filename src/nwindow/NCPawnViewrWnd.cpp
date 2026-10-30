/*=============================================================================
    NCPawnViewrWnd.cpp - the pawn viewer's main window and its item pages. Stock:
    0x10087420 .. 0x10088108, 0x10088830 .. 0x10089102, 0x10089380 .. 0x100895e8,
    0x1008c2d0 .. 0x1008c3b8, 0x1008c750 .. 0x1008cd1d, 0x1008e230 .. 0x1008ee32 (the other
    PawnView classes sit in between); the item grid's window procedure is a body shared with
    the MSView / NpcView grids (hooked in NCNpcViewrWnd.cpp).
=============================================================================*/
#include "nwindow/NCPawnViewrWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NItemInfo.h"

#include <string.h>
#include <wchar.h>

// NWM_COMMAND ids (names ours)
enum
{
    NID_PAWNVIEW_FRAME = 0xF2,
    NID_PAWNVIEW_F3 = 0xF3,          // handled (vfn062), no control sends it
    NID_PAWNVIEW_GUI = 0xF4,
    NID_PAWNVIEW_TEXTLIST = 0xF5,    // each text page's list
    NID_PAWNVIEW_SEARCH = 0x102,
    NID_PAWNVIEW_VIEWLIST = 0x103,
    NID_PAWNVIEW_WEAPONTYPE = 0x114,
    NID_PAWNVIEW_USE = 0x115,
    NID_PAWNVIEW_CATEGORY = 0xFE001A,
    NID_PAWNVIEW_PAGES = 0xFE002A,
};

// sysstring lines
enum
{
    STR_Gloves = 37,
    STR_Chest = 38,
    STR_Legs = 39,
    STR_Feet = 40,
    STR_Etc = 49,
    STR_Human = 170,
    STR_Elf = 171,
    STR_DarkElf = 172,
    STR_Orc = 173,
    STR_Dwarf = 174,
    STR_Warrior = 175,
    STR_Spellcaster = 176,
    STR_Male = 177,
    STR_Female = 178,
    STR_Headgear = 230,
    STR_Search = 420,
    STR_ViewList = 422,
    STR_Use = 443,
};

// item categories: the pages of CategoryTabs (names ours)
enum
{
    PAWNVIEW_Chest,
    PAWNVIEW_Legs,
    PAWNVIEW_Gloves,
    PAWNVIEW_Feet,
    PAWNVIEW_Headgear,
    PAWNVIEW_Etc,
    PAWNVIEW_NUM_CATEGORIES,
};

// the 18 default characters of the creation screen (0x101133b8, only the viewer reads it)
struct NDefaultCharacter
{
    INT Race;
    INT Sex;
    INT ClassID;
};
#if NW_HOOK_MODE
#define GDefaultCharacters (reinterpret_cast<const NDefaultCharacter*>(::nw::Addr(0x101133b8)))
#else
extern const NDefaultCharacter GDefaultCharacters[];
#endif

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

// The character chosen in the character window (0..19: the check lets one past the 19
// records through) puts the item on, or takes it off when it wears it already; a two-handed
// item frees both hands, a one-handed one the two-handed slot. The player's pawn is dressed
// as that character. Inline in both stock callers (name ours).
static FORCEINLINE void ToggleItem(NItemEntry* Item)
{
    NCPawnViewrWnd* Viewer = NPawnView::Viewer();
    INT Index = Viewer->CharSelectWnd->Selected;
    if (Index >= 0 && Index <= 19)
    {
        User& Character = Viewer->Characters[Index];
        if (Item->BodyPart == BODYSLOT_LRHand)
        {
            Character.Equipped[BODYSLOT_RHand] = 0;
            Character.Equipped[BODYSLOT_LHand] = 0;
        }
        else if (Item->BodyPart == BODYSLOT_RHand || Item->BodyPart == BODYSLOT_LHand)
        {
            Character.Equipped[BODYSLOT_LRHand] = 0;
        }
        INT& Worn = Character.Equipped[Item->BodyPart];
        Worn = (Worn == Item->ID) ? 0 : Item->ID;
        NPawnView::Viewer()->Characters[Index].Pawn = NPawnView::PlayerPawn();
        NPawnView::Viewer()->Characters[Index].SetPawnResource();
    }
}

// an item of the table as the pages show it: icon (black when the icon is missing) and name
static FORCEINLINE void FillItemInfo(NCObject* Owner, NItemInfo& Info, INT ItemID)
{
    if (GConsole->GetIconName(ItemID))
    {
        wcscpy(Info.IconName, GConsole->GetIconName(ItemID));
        Info.Icon = Owner->FindTexture(GConsole->GetIconName(ItemID));
    }
    if (GConsole->GetIconName(ItemID))
        Info.Icon = Owner->FindTexture(GConsole->GetIconName(ItemID));
    if (!Info.Icon)
        Info.Icon = Owner->FindTexture(TEXT("NWindow.BlackTexture"));
    if (GConsole->GetItemName(ItemID))
        wcscpy(Info.Name, GConsole->GetItemName(ItemID));
}

/*-----------------------------------------------------------------------------
    NCPawnView_InvenItemWnd.
-----------------------------------------------------------------------------*/

// 0x1008c750 - the pending questions of NCInvenItemWnd cleared again
NCPawnView_InvenItemWnd::NCPawnView_InvenItemWnd()
{
    bDropPending = 0;
    bDestroyPending = 0;
}
NW_HOOK_CTOR(0x1008c750, NCPawnView_InvenItemWnd)

// 0x1008c7f0
NCPawnView_InvenItemWnd::NCPawnView_InvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems,
                                                 INT InCellSize)
    : NCInvenItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
    bDropPending = 0;
    bDestroyPending = 0;
}
NW_HOOK_CTOR_ARGS(0x1008c7f0, NCPawnView_InvenItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x1008c7a0
NW_HOOK_DTOR(0x1008c7a0, NCPawnView_InvenItemWnd)

// stock body shared with NCNpcView_InvenItemWnd's (0x1007a1a0)
INT NCPawnView_InvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCInvenItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}

// 0x100888d0 (vslot 58) - the item under the mouse on or off the chosen character
INT NCPawnView_InvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    guard(NCPawnView_InvenItemWnd::OnRButtonDown);
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
        ToggleItem(Items(Selected));
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100888d0, NCPawnView_InvenItemWnd, OnRButtonDown)

// 0x10087420 (vslot 68) - as the right button, nothing carried
INT NCPawnView_InvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    guard(NCPawnView_InvenItemWnd::OnLButtonDblClick);
    bDragging = 0;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    UnselectAll();
    OnRButtonDown(wParam, lParam);
    bDragging = 0;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10087420, NCPawnView_InvenItemWnd, OnLButtonDblClick)

NW_CHECK_VTABLE(NCPawnView_InvenItemWnd, 0x100eb4d0, 70)

/*-----------------------------------------------------------------------------
    NCPawnView_InvenTextWnd.
-----------------------------------------------------------------------------*/

// 0x10088830
NCPawnView_InvenTextWnd::NCPawnView_InvenTextWnd()
{
}
NW_HOOK_CTOR(0x10088830, NCPawnView_InvenTextWnd)

// 0x10088880
NW_HOOK_DTOR(0x10088880, NCPawnView_InvenTextWnd)

// 0x100881f0 (vslot 24)
INT NCPawnView_InvenTextWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_PAWNVIEW_TEXTLIST)
        return OnInvenItemList((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100881f0, NCPawnView_InvenTextWnd, WindowProc)

// 0x100894e0 (vslot 35) - the list (3000 lines, 14 shown), created without the visible bit
// (0x21: bits of unknown meaning); NCWnd's OnCreate is not called
INT NCPawnView_InvenTextWnd::OnCreate()
{
    guard(NCPawnView_InvenTextWnd::OnCreate);
    List = new NCListBox(3000, 14, 16);
    List->ID = NID_PAWNVIEW_TEXTLIST;
    List->SetWindowRect(20, 20, 150, 180);
    CreateChild(List, 0x21);
    List->TextX = 1;
    List->TextY = 1;
    List->LineGap = 1;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100894e0, NCPawnView_InvenTextWnd, OnCreate)

// 0x10089380 (vslot 62) - a click on a line: that item on or off the chosen character (the
// list of the viewer's current page, whichever page this is)
INT NCPawnView_InvenTextWnd::OnInvenItemList(INT Index)
{
    guard(NCPawnView_InvenTextWnd::OnInvenItemList);
    NCPawnViewrWnd* Viewer = NPawnView::Viewer();
    NItemEntry* Item = (NItemEntry*)Viewer->TextWnds[Viewer->CurrentPage]->List->GetItemParam(Index);
    ToggleItem(Item);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10089380, NCPawnView_InvenTextWnd, OnInvenItemList)

NW_CHECK_VTABLE(NCPawnView_InvenTextWnd, 0x100eaa40, 63)

/*-----------------------------------------------------------------------------
    NCPawnViewrWnd.
-----------------------------------------------------------------------------*/

// 0x10088a50 - the character records are the engine's User (its constructor)
NCPawnViewrWnd::NCPawnViewrWnd() : CategoryTabs(NULL), GridBackTex(NULL)
{
    for (INT i = 0; i < 7; i++)
        ItemWnds[i] = NULL;
    Level = NULL;
}
NW_HOOK_CTOR(0x10088a50, NCPawnViewrWnd)

// 0x10088b20
NW_HOOK_DTOR(0x10088b20, NCPawnViewrWnd)

// 0x100874e0 (vslot 24)
INT NCPawnViewrWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_PAWNVIEW_CATEGORY)
            return OnCategoryTab((INT)lParam) != 0;
        if (wParam == NID_PAWNVIEW_F3)
            return vfn062() != 0;
        if (wParam == NID_PAWNVIEW_GUI)
            return OnGUICheck() != 0;
        if (wParam == NID_PAWNVIEW_SEARCH)
            return OnSearch() != 0;
        if (wParam == NID_PAWNVIEW_VIEWLIST)
            return OnViewList() != 0;
        if (wParam == NID_PAWNVIEW_USE)
            return OnUseWeapon() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100874e0, NCPawnViewrWnd, WindowProc)

// 0x1008e230 (vslot 35) - title bar, the three pages and their tabs, a grid and a text list
// per category and the category tabs, "GUI", the character window (registered at the top
// level), the weapon type and "Use", the search line, "Search" and "View List"; then the
// items and the characters. NCWnd's OnCreate is not called.
INT NCPawnViewrWnd::OnCreate()
{
    guard(NCPawnViewrWnd::OnCreate);
    for (INT i = 0; i < 8; i++)
        DefaultWeapons[i] = -1;

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_PAWNVIEW_FRAME;
    FrameCtrl->ID = NID_PAWNVIEW_FRAME;
    FrameCtrl->SetWindowRect(0, 19, Width, 25);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    GridBackTex = FindTexture(TEXT("L2UI.QuestWnd.questBack3"));
    BackTex = FindTexture(TEXT("L2UI.etcWnd.OptionBack"));

    MixAnimWnd = new NCPawnViewr_MixAnimWnd;
    MixAnimWnd->SetWindowRect(0, 53, 240, 236);
    CreateChild(MixAnimWnd, NWS_VISIBLE);
    HairChangeWnd = new NCPawnViewr_HairChangeWnd;
    HairChangeWnd->SetWindowRect(0, 53, 240, 236);
    CreateChild(HairChangeWnd, NWS_VISIBLE);
    Simulator = new NCPawnViewr_Simulator;
    Simulator->SetWindowRect(0, 53, 240, 236);
    CreateChild(Simulator, NWS_VISIBLE);

    // "combined skin", "hair mesh", "skin test"
    PageTabs = new NCTabControl;
    PageTabs->ID = NID_PAWNVIEW_PAGES;
    PageTabs->SetWindowRect(15, 42, 207, 16);
    CreateChild(PageTabs, NWS_VISIBLE);
    PageTabs->AddTab(TEXT("\xC870\xD569\xC2A4\xD0AC"), MixAnimWnd, 69, 16,
                     TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0, -1);
    PageTabs->AddTab(TEXT("\xD5E4\xC5B4\xBA54\xC26C"), HairChangeWnd, 69, 16,
                     TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0, -1);
    PageTabs->AddTab(TEXT("\xC2A4\xD0AC\xD14C\xC2A4\xD2B8"), Simulator, 69, 16,
                     TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0, -1);

    for (INT i = 0; i < PAWNVIEW_NUM_CATEGORIES; i++)
    {
        ItemWnds[i] = new NCPawnView_InvenItemWnd(6, 4, 80, 32);
        ItemWnds[i]->SetWindowRect(16, 319, 218, 150);
        CreateChild(ItemWnds[i], 0x21);   // 0x21: bits of unknown meaning, not visible
        TextWnds[i] = new NCPawnView_InvenTextWnd;
        TextWnds[i]->SetWindowRect(16, 309, 218, 189);
        CreateChild(TextWnds[i], 0x21);
    }

    CategoryTabs = new NCTabControl;
    CategoryTabs->ID = NID_PAWNVIEW_CATEGORY;
    CategoryTabs->SetWindowRect(7, 289, 138, 20);
    CreateChild(CategoryTabs, NWS_VISIBLE);
    CategoryTabs->AddTab(STR_Chest, NULL, 25, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);
    CategoryTabs->AddTab(STR_Legs, NULL, 25, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);
    CategoryTabs->AddTab(STR_Gloves, NULL, 25, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);
    CategoryTabs->AddTab(STR_Feet, NULL, 25, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);
    CategoryTabs->AddTab(STR_Headgear, NULL, 25, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);
    CategoryTabs->AddTab(STR_Etc, NULL, 25, 20, TEXT("L2UI.NWindow.Tab2"),
                         TEXT("L2UI.NWindow.Tab1"), 0, -1);

    GUICheck = new NCCheckBox(TEXT("GUI"), 0, 0);
    GUICheck->ID = NID_PAWNVIEW_GUI;
    GUICheck->SetWindowRect(160, 291, 39, 16);
    CreateChild(GUICheck, NWS_VISIBLE);
    if (!GUICheck->bDisabled)
        GUICheck->bChecked = 1;

    for (INT i = 0; i < 19; i++)
        Characters[i].Pawn = NULL;

    CharSelectWnd = new NCPawnView_CharacterSelectWnd;
    CharSelectWnd->SetWindowRect(34, 34, 234, 503);
    CreateTopWindow(CharSelectWnd, NWS_VISIBLE | NWS_NOHITTEST);

    WeaponTypeCombo = new NCComboBox;
    WeaponTypeCombo->ID = NID_PAWNVIEW_WEAPONTYPE;
    WeaponTypeCombo->SetWindowRect(25, 500, 70, 18);
    CreateChild(WeaponTypeCombo, NWS_VISIBLE);
    WeaponTypeCombo->AddItem(TEXT("0(HAND)"));
    WeaponTypeCombo->AddItem(TEXT("1(1HS)"));
    WeaponTypeCombo->AddItem(TEXT("2(2HS)"));
    WeaponTypeCombo->AddItem(TEXT("3(DUAL)"));
    WeaponTypeCombo->AddItem(TEXT("4(POLE)"));
    WeaponTypeCombo->AddItem(TEXT("5(BOW)"));
    WeaponTypeCombo->AddItem(TEXT("7(DUALFIST)"));
    UseButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                             TEXT("L2UI.Control.DefaultButton_click"), 0);
    UseButton->ID = NID_PAWNVIEW_USE;
    UseButton->SetWindowRect(100, 500, 28, 18);
    CreateChild(UseButton, NWS_VISIBLE);
    UseButton->SetTextID(STR_Use);

    SearchEdit = new NCEditBox(1, NULL, 1, 64);
    SearchEdit->SetWindowRect(25, 530, 60, 16);
    CreateChild(SearchEdit, NWS_VISIBLE);
    SearchButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    SearchButton->ID = NID_PAWNVIEW_SEARCH;
    SearchButton->SetWindowRect(95, 530, 28, 16);
    CreateChild(SearchButton, NWS_VISIBLE);
    SearchButton->SetTextID(STR_Search);
    ListButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                              TEXT("L2UI.Control.DefaultButton_click"), 0);
    ListButton->ID = NID_PAWNVIEW_VIEWLIST;
    ListButton->SetWindowRect(130, 530, 45, 16);
    CreateChild(ListButton, NWS_VISIBLE);
    ListButton->SetTextID(STR_ViewList);

    AddAllItem();
    AddAllCharacter();
    Level = GameLevel();
    CharSelectWnd->OnNameComboBox(0);
    OnCategoryTab(0);
    PageTabs->SetTab(0);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008e230, NCPawnViewrWnd, OnCreate)

// 0x10088dd0 (vslot 36) - the character window and two pages are deleted without leaving
// the window manager; the text pages, the simulator, the page tabs, the search line, its
// buttons, the weapon combo and "Use" are left alone
INT NCPawnViewrWnd::OnDestroy()
{
    guard(NCPawnViewrWnd::OnDestroy);
    for (INT i = 0; i < PAWNVIEW_NUM_CATEGORIES; i++)
        DestroyChild(this, ItemWnds[i]);
    DestroyChild(this, GUICheck);
    DestroyChild(this, CategoryTabs);
    DestroyChild(this, UnknownF8);
    DestroyChild(this, FrameCtrl);
    if (CharSelectWnd)
        delete CharSelectWnd;
    if (MixAnimWnd)
        delete MixAnimWnd;
    if (HairChangeWnd)
        delete HairChangeWnd;
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10088dd0, NCPawnViewrWnd, OnDestroy)

// 0x10089030 (vslot 42) - the character window shows too; only the current page's grid (or
// list) stays up
void NCPawnViewrWnd::ShowWindow()
{
    guard(NCPawnViewrWnd::ShowWindow);
    if (CharSelectWnd)
        CharSelectWnd->ShowWindow();
    OnGUICheck();
    NCWnd::ShowWindow();
    for (INT i = 0; i < PAWNVIEW_NUM_CATEGORIES; i++)
    {
        if (CurrentPage != i || !GUICheck->bChecked)
            ItemWnds[i]->HideWindow();
        if (CurrentPage != i || GUICheck->bChecked)
            TextWnds[i]->HideWindow();
    }
    CategoryTabs->ShowCurrentPage();
    PageTabs->ShowCurrentPage();
    unguard;
}
NW_HOOK_METHOD(0x10089030, NCPawnViewrWnd, ShowWindow)

// 0x10087fd0 (vslot 43)
void NCPawnViewrWnd::HideWindow()
{
    guard(NCPawnViewrWnd::HideWindow);
    if (CharSelectWnd)
        CharSelectWnd->HideWindow();
    NCWnd::HideWindow();
    unguard;
}
NW_HOOK_METHOD(0x10087fd0, NCPawnViewrWnd, HideWindow)

// 0x10088c10 (vslot 50) - the background of the item part (the grids' or the lists'), then
// the upper part and the bottom of the option background; the origin goes on before the clip
INT NCPawnViewrWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPawnViewrWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    UTexture* ItemBack = GUICheck->bChecked ? GridBackTex : BackTex;
    if (ItemBack)
        Device->DrawTexture(0, 289, 240, 185, 0, 0, 240, 185, ItemBack);
    if (BackTex)
    {
        Device->DrawTexture(0, 40, 240, 249, 0, 0, 240, 249, BackTex);
        Device->DrawTexture(0, 474, 240, 125, 0, 219, 240, 125, BackTex);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10088c10, NCPawnViewrWnd, OnPaint)

// 0x10088b70 (vslot 63) - a category: its grid, or its list (the text page itself does not
// show: its ShowWindow is empty)
INT NCPawnViewrWnd::OnCategoryTab(INT Index)
{
    HidePages();
    if (GUICheck->bChecked)
    {
        if (Index >= 0 && Index <= 6 && ItemWnds[Index])
        {
            CurrentPage = Index;
            ItemWnds[Index]->ShowWindow();
        }
    }
    else if (Index >= 0 && Index <= 6 && ItemWnds[Index])
    {
        CurrentPage = Index;
        TextWnds[Index]->List->ShowWindow();
        TextWnds[CurrentPage]->ShowWindow();
    }
    return 1;
}
NW_HOOK_METHOD(0x10088b70, NCPawnViewrWnd, OnCategoryTab)

// 0x10088d60 (vslot 64) - the current category again, as a grid or as a list
INT NCPawnViewrWnd::OnGUICheck()
{
    HidePages();
    if (GUICheck->bChecked)
    {
        ItemWnds[CurrentPage]->ShowWindow();
        return 1;
    }
    TextWnds[CurrentPage]->List->ShowWindow();
    TextWnds[CurrentPage]->ShowWindow();
    return 1;
}
NW_HOOK_METHOD(0x10088d60, NCPawnViewrWnd, OnGUICheck)

// 0x1008ca60 (vslot 65) - the current category's grid refilled with the items whose name
// holds the text typed (the lists keep their lines and get the matches again)
INT NCPawnViewrWnd::OnSearch()
{
    const TCHAR* Text = SearchEdit->CommitText();
    ItemWnds[CurrentPage]->Clear();
    if (!Text)
        return 1;
    NDataMap<NViewItemData>* Map = GL2GameData.ItemTable;
    for (INT i = 0; i < Map->Pairs.Num(); i++)
    {
        NViewItemData* Data = Map->Pairs(i).Value;
        NItemInfo Info;
        if (!Data)
            continue;
        INT Page = GetItemCategory(Data, &Info.BodyPart, -1);
        if (Page != CurrentPage || !appStrstr(*Data->Name, Text))
            continue;
        FillItemInfo(this, Info, Map->Pairs(i).Key);
        Info.ID = Map->Pairs(i).Key;
        AddItem(Page, &Info);
        ItemWnds[Page]->MaxItems++;
    }
    return 1;
}
NW_HOOK_METHOD(0x1008ca60, NCPawnViewrWnd, OnSearch)

// 0x1008cc90 (vslot 66) - every category refilled from the item table (the items of the
// lists' lines are not freed)
INT NCPawnViewrWnd::OnViewList()
{
    Clear();
    for (INT i = 0; i < PAWNVIEW_NUM_CATEGORIES; i++)
        TextWnds[i]->List->Clear();
    AddAllItem();
    return 1;
}
NW_HOOK_METHOD(0x1008cc90, NCPawnViewrWnd, OnViewList)

// 0x1008c2d0 (vslot 67) - the chosen character takes the first weapon found of the type
// picked ("7(DUALFIST)", the seventh line, is type 7); bare hands for type 0 or none found
INT NCPawnViewrWnd::OnUseWeapon()
{
    INT Type = WeaponTypeCombo->Selected;
    INT Index = CharSelectWnd->Selected;
    if (Index < 0 || Index > 19)
        return 1;
    if (Type == 6)
        Type = 7;
    User& Character = Characters[Index];
    NViewItemData* Data = NItemTable::Find(&DefaultWeapons[Type]);
    if (Data && Type != 0)
    {
        if (Data->BodyPart == BODYSLOT_LRHand)
        {
            Character.Equipped[BODYSLOT_RHand] = 0;
            Character.Equipped[BODYSLOT_LHand] = 0;
            Character.Equipped[Data->BodyPart] = DefaultWeapons[Type];
        }
        else
        {
            if (Data->BodyPart == BODYSLOT_RHand || Data->BodyPart == BODYSLOT_LHand)
                Character.Equipped[BODYSLOT_LRHand] = 0;
            Character.Equipped[Data->BodyPart] = DefaultWeapons[Type];
        }
    }
    else
    {
        Character.Equipped[BODYSLOT_RHand] = 0;
        Character.Equipped[BODYSLOT_LHand] = 0;
        Character.Equipped[BODYSLOT_LRHand] = 0;
    }
    Character.Pawn = NPawnView::PlayerPawn();
    Character.SetPawnResource();
    return 1;
}
NW_HOOK_METHOD(0x1008c2d0, NCPawnViewrWnd, OnUseWeapon)

// 0x100875f0 - every category's grid and list hidden
__declspec(noinline) void NCPawnViewrWnd::HidePages()
{
    for (INT i = 0; i < PAWNVIEW_NUM_CATEGORIES; i++)
    {
        ItemWnds[i]->HideWindow();
        TextWnds[i]->HideWindow();
        TextWnds[i]->List->HideWindow();
    }
}
NW_HOOK_METHOD(0x100875f0, NCPawnViewrWnd, HidePages)

// "<race><gender><class type>" in the character window's combo box
static FORCEINLINE void AddCharacterName(NCPawnViewrWnd* Viewer, TCHAR* Text, INT Race,
                                         INT Sex, INT Type)
{
    appSprintf(Text, TEXT("%s%s%s"), *GL2GameData.SysString[Race], *GL2GameData.SysString[Sex],
               *GL2GameData.SysString[Type]);
    Viewer->CharSelectWnd->NameCombo->AddItem(Text);
}

// 0x10087630 - the records emptied and given the creation screen's race, gender and class
// (the 19th reads past the 18 of the table), and the 18 names in the character window
__declspec(noinline) void NCPawnViewrWnd::AddAllCharacter()
{
    guard(NCPawnViewrWnd::AddAllCharacter);
    TCHAR Text[100];
    for (INT i = 0; i < 19; i++)
    {
        memset(&Characters[i], 0, sizeof(User));
        Characters[i].Race = GDefaultCharacters[i].Race;
        Characters[i].Sex = GDefaultCharacters[i].Sex;
        Characters[i].ClassID = GDefaultCharacters[i].ClassID;
    }
    AddCharacterName(this, Text, STR_Human, STR_Male, STR_Warrior);
    AddCharacterName(this, Text, STR_Human, STR_Female, STR_Warrior);
    AddCharacterName(this, Text, STR_Human, STR_Male, STR_Spellcaster);
    AddCharacterName(this, Text, STR_Human, STR_Female, STR_Spellcaster);
    AddCharacterName(this, Text, STR_Elf, STR_Male, STR_Warrior);
    AddCharacterName(this, Text, STR_Elf, STR_Female, STR_Warrior);
    AddCharacterName(this, Text, STR_Elf, STR_Male, STR_Spellcaster);
    AddCharacterName(this, Text, STR_Elf, STR_Female, STR_Spellcaster);
    AddCharacterName(this, Text, STR_DarkElf, STR_Male, STR_Warrior);
    AddCharacterName(this, Text, STR_DarkElf, STR_Female, STR_Warrior);
    AddCharacterName(this, Text, STR_DarkElf, STR_Male, STR_Spellcaster);
    AddCharacterName(this, Text, STR_DarkElf, STR_Female, STR_Spellcaster);
    AddCharacterName(this, Text, STR_Orc, STR_Male, STR_Warrior);
    AddCharacterName(this, Text, STR_Orc, STR_Female, STR_Warrior);
    AddCharacterName(this, Text, STR_Orc, STR_Male, STR_Spellcaster);
    AddCharacterName(this, Text, STR_Orc, STR_Female, STR_Spellcaster);
    AddCharacterName(this, Text, STR_Dwarf, STR_Male, STR_Warrior);
    AddCharacterName(this, Text, STR_Dwarf, STR_Female, STR_Warrior);
    unguard;
}
NW_HOOK_METHOD(0x10087630, NCPawnViewrWnd, AddAllCharacter)

// 0x10087e60 - the page of an item entry: armor by its body part, the rest under etc; the
// first weapon of each type met is kept for OnUseWeapon (a dual only when two-handed)
__declspec(noinline) INT NCPawnViewrWnd::GetItemCategory(NViewItemData* Data, INT* BodyPart,
                                                         INT ItemID)
{
    if (Data->Class == 1)
    {
        *BodyPart = Data->BodyPart;
        switch (Data->BodyPart)
        {
        case 6:
            return PAWNVIEW_Headgear;
        case 9:
            return PAWNVIEW_Gloves;
        case 10:
            return PAWNVIEW_Chest;
        case 11:
            return PAWNVIEW_Legs;
        case 12:
            return PAWNVIEW_Feet;
        default:
            return PAWNVIEW_Etc;
        }
    }
    *BodyPart = Data->BodyPart;
    if (Data->Class == 0 && ItemID != -1)
    {
        INT Type = Data->PawnWeaponType;
        if (DefaultWeapons[Type] == -1 && (Type != 3 || Data->BodyPart == BODYSLOT_LRHand))
            DefaultWeapons[Type] = ItemID;
    }
    return PAWNVIEW_Etc;
}
NW_HOOK_METHOD(0x10087e60, NCPawnViewrWnd, GetItemCategory)

// 0x10087f10 - the grids emptied (not the lists)
__declspec(noinline) void NCPawnViewrWnd::Clear()
{
    guard(NCPawnViewrWnd::Clear);
    ItemWnds[PAWNVIEW_Chest]->Clear();
    ItemWnds[PAWNVIEW_Legs]->Clear();
    ItemWnds[PAWNVIEW_Gloves]->Clear();
    ItemWnds[PAWNVIEW_Feet]->Clear();
    ItemWnds[PAWNVIEW_Headgear]->Clear();
    ItemWnds[PAWNVIEW_Etc]->Clear();
    unguard;
}
NW_HOOK_METHOD(0x10087f10, NCPawnViewrWnd, Clear)

// 0x10088060 - an item into a category's grid and list (the list line gets an NItemEntry of
// its own, never freed); the page check lets 7 (the unused eighth pointer) through
__declspec(noinline) INT NCPawnViewrWnd::AddItem(INT Page, NItemInfo* Info)
{
    guard(NCPawnViewrWnd::AddItem);
    if (Page >= 0 && Page < 8 && ItemWnds[Page])
    {
        ItemWnds[Page]->AddItem(Info);
        NItemEntry* Entry = new NItemEntry(Info);
        TextWnds[Page]->List->AddString(Info->Name, 0xFFFFFFFF, (INT)Entry);
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10088060, NCPawnViewrWnd, AddItem)

// 0x1008c830 - every entry of the item table into its category; each grid grows by one for
// every item added, so none ever fills
__declspec(noinline) void NCPawnViewrWnd::AddAllItem()
{
    guard(NCPawnViewrWnd::AddAllItem);
    NDataMap<NViewItemData>* Map = GL2GameData.ItemTable;
    for (INT i = 0; i < Map->Pairs.Num(); i++)
    {
        NViewItemData* Data = Map->Pairs(i).Value;
        NItemInfo Info;
        if (!Data)
            continue;
        FillItemInfo(this, Info, Map->Pairs(i).Key);
        Info.ID = Map->Pairs(i).Key;
        INT Page = GetItemCategory(Data, &Info.BodyPart, Map->Pairs(i).Key);
        AddItem(Page, &Info);
        ItemWnds[Page]->MaxItems++;
    }
    unguard;
}
NW_HOOK_METHOD(0x1008c830, NCPawnViewrWnd, AddAllItem)

NW_CHECK_VTABLE(NCPawnViewrWnd, 0x100eab90, 68)
