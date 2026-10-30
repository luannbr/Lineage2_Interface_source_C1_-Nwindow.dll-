/*=============================================================================
    NCGMWnd.cpp - the GM's windows. Stock: 0x1005bc40 .. 0x1005bd7c, 0x1005be70 .. 0x1005bf22,
    0x1005bfc0 .. 0x1005c5a4, 0x1005c730 .. 0x1005c974, 0x1005c980 .. 0x1005ca42,
    0x1005e310 .. 0x1005e9a3, 0x1005eac0 .. 0x1005f1fd, 0x1005f250 .. 0x1005fc4c,
    0x1005ff10 .. 0x1006004f, 0x10060050 .. 0x100603f1 (NCGMStoreWnd's functions sit in
    between; NCGMActionWnd's OnDestroy is NCPetActionWnd's body, its OnPaint NCActionWnd's).
=============================================================================*/
#include "nwindow/NCGMWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDetailStatusWnd.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCListBox.h"
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCQuestInfoWnd.h"
#include "nwindow/NCQuestItemWnd.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NItemInfo.h"

// sysstring
enum
{
    STR_Item = 2,
    STR_Name = 50,
    STR_Lv = 88,
    STR_Quest = 118,
    STR_Skills = 119,
    STR_Warehouse = 131,
    STR_Inventory = 138,
    STR_AcceptedQuest = 324,
    STR_Description = 325,
    STR_QuestItem = 326,
    STR_Cancel = 327,
    STR_Leader = 342,
    STR_Base = 343,
    STR_HideoutExists = 344,
    STR_CastleExists = 345,
    STR_Status = 433,
    STR_Clan = 439,
};

enum { MSG_EnterUserName = 364 };      // systemmsg "Enter user's name to search."

enum { CLAN_TEXT_COLOR = 0xFFB09B79, CLAN_LABEL_COLOR = 0xFFA3A3A3 };

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
    NCGMWnd
-----------------------------------------------------------------------------*/

// 0x1005f250 - CloseButton is left as allocated
NCGMWnd::NCGMWnd()
{
    TopTex = NULL;
    MidTex = NULL;
    BottomTex = NULL;
    NameEdit = NULL;
    InventoryButton = NULL;
    SkillsButton = NULL;
    StatusButton = NULL;
    QuestButton = NULL;
    WarehouseButton = NULL;
    ClanButton = NULL;
}
NW_HOOK_CTOR(0x1005f250, NCGMWnd)

// 0x1005f2d0
NW_HOOK_DTOR(0x1005f2d0, NCGMWnd)

// 0x1005c0e0 (vslot 24)
INT NCGMWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_GM_INVENTORY)
            return OnInventory() != 0;
        if (wParam == NID_GM_SKILLS)
            return OnSkills() != 0;
        if (wParam == NID_GM_STATUS)
            return OnStatus() != 0;
        if (wParam == NID_GM_QUEST)
            return OnQuest() != 0;
        if (wParam == NID_GM_WAREHOUSE)
            return OnWarehouse() != 0;
        if (wParam == NID_GM_CLAN)
            return OnClan() != 0;
        if (wParam == NID_GM_CLOSE)
            return OnClose() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1005c0e0, NCGMWnd, WindowProc)

// the six view buttons: the desktop's toggle with the name typed, or a message without one
// (repeated in the stock code; the text is fetched twice)
#define NCGMWND_VIEW_BUTTON(Method, Toggle)                                                   \
    INT NCGMWnd::Method()                                                                     \
    {                                                                                         \
        if (NameEdit->CommitText())                                                           \
            GDesktop->Toggle((TCHAR*)NameEdit->CommitText());                                 \
        else                                                                                  \
            GConsole->ShowSystemMessage(MSG_EnterUserName, 0xFFFFFFFF);                       \
        return 1;                                                                             \
    }

// 0x1005c210 (vslot 62)
NCGMWND_VIEW_BUTTON(OnInventory, ToggleGMInventory)
NW_HOOK_METHOD(0x1005c210, NCGMWnd, OnInventory)
// 0x1005c270 (vslot 63)
NCGMWND_VIEW_BUTTON(OnSkills, ToggleGMMagicSkill)
NW_HOOK_METHOD(0x1005c270, NCGMWnd, OnSkills)
// 0x1005c2d0 (vslot 64)
NCGMWND_VIEW_BUTTON(OnStatus, ToggleGMDetailStatus)
NW_HOOK_METHOD(0x1005c2d0, NCGMWnd, OnStatus)
// 0x1005c330 (vslot 65)
NCGMWND_VIEW_BUTTON(OnQuest, ToggleGMQuest)
NW_HOOK_METHOD(0x1005c330, NCGMWnd, OnQuest)
// 0x1005c390 (vslot 66)
NCGMWND_VIEW_BUTTON(OnWarehouse, ToggleGMStore)
NW_HOOK_METHOD(0x1005c390, NCGMWnd, OnWarehouse)
// 0x1005c3f0 (vslot 67)
NCGMWND_VIEW_BUTTON(OnClan, ToggleGMAction)
NW_HOOK_METHOD(0x1005c3f0, NCGMWnd, OnClan)

// 0x1005c450 (vslot 68) - the view windows shown are hidden, then this one. The first two
// tests are crossed in the stock code: the inventory being shown hides the skill window and
// the skill window being shown hides the inventory, so the inventory never closes here and
// the skill window only along with it.
INT NCGMWnd::OnClose()
{
    if (GDesktop->GMInventoryWnd->IsShowWindow())
        GDesktop->GMMagicSkillWnd->HideWindow();
    if (GDesktop->GMMagicSkillWnd->IsShowWindow())
        GDesktop->GMInventoryWnd->HideWindow();
    if (GDesktop->GMDetailStatusWnd->IsShowWindow())
        GDesktop->GMDetailStatusWnd->HideWindow();
    if (GDesktop->GMQuestWnd->IsShowWindow())
        GDesktop->GMQuestWnd->HideWindow();
    if (GDesktop->GMStoreWnd->IsShowWindow())
        GDesktop->GMStoreWnd->HideWindow();
    if (GDesktop->GMActionWnd->IsShowWindow())
        GDesktop->GMActionWnd->HideWindow();
    NCWnd::HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x1005c450, NCGMWnd, OnClose)

// a view button (repeated in the stock OnCreate): 54 x 18, labelled with a sysstring
static FORCEINLINE NCButton* CreateViewButton(NCWnd* Parent, INT ID, INT X, INT Y, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("sek.cbui91"), TEXT("sek.cbui93"), 0);
    Button->ID = ID;
    Button->SetWindowRect(X, Y, 54, 18);
    Parent->CreateChild(Button, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    Button->SetText(*GL2GameData.SysString[TextID]);
    return Button;
}

// 0x1005f320 (vslot 35) - a plain title bar 25 pixels high, 19 below the top, the name box and
// the buttons in two columns
INT NCGMWnd::OnCreate()
{
    NCWnd::OnCreate();
    TopTex = FindTexture(TEXT("sek.ui02"));
    MidTex = FindTexture(TEXT("sek.cbui157"));
    BottomTex = FindTexture(TEXT("sek.ui03"));

    FrameCtrl = new NCFrameCtrl(0);
    FrameCtrl->ID = NID_GM_FRAME;
    FrameCtrl->SetWindowRect(0, 19, Width, 25);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    NameEdit = new NCEditBox(0, TEXT("NWindow.ChatBack"), 1, 128);
    NameEdit->ID = NID_GM_NAME;
    NameEdit->SetWindowRect(35, 55, 112, 14);
    CreateChild(NameEdit, NWS_VISIBLE);

    StatusButton = CreateViewButton(this, NID_GM_STATUS, 35, 83, STR_Status);
    InventoryButton = CreateViewButton(this, NID_GM_INVENTORY, 99, 83, STR_Inventory);
    SkillsButton = CreateViewButton(this, NID_GM_SKILLS, 35, 111, STR_Skills);
    QuestButton = CreateViewButton(this, NID_GM_QUEST, 99, 111, STR_Quest);
    WarehouseButton = CreateViewButton(this, NID_GM_WAREHOUSE, 35, 139, STR_Warehouse);
    ClanButton = CreateViewButton(this, NID_GM_CLAN, 99, 139, STR_Clan);

    CloseButton = new NCButton();
    CloseButton->ID = NID_GM_CLOSE;
    CloseButton->SetWindowRect(79, 194, 50, 16);
    CreateChild(CloseButton, NWS_VISIBLE);
    CloseButton->SetPushedTex(TEXT("sek.cbui27"));
    return 1;
}
NW_HOOK_METHOD(0x1005f320, NCGMWnd, OnCreate)

// 0x1005f9c0 (vslot 36)
INT NCGMWnd::OnDestroy()
{
    DestroyChild(this, NameEdit);
    DestroyChild(this, InventoryButton);
    DestroyChild(this, SkillsButton);
    DestroyChild(this, StatusButton);
    DestroyChild(this, QuestButton);
    DestroyChild(this, WarehouseButton);
    DestroyChild(this, ClanButton);
    DestroyChild(this, CloseButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1005f9c0, NCGMWnd, OnDestroy)

// 0x1005fb40 (vslot 50) - the middle first, the top and bottom over its edges, "Name" before
// the box
INT NCGMWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (MidTex)
        Device->DrawTexture(0, 43, 208, 144, 0, 0, 208, 144, MidTex);
    if (TopTex)
        Device->DrawTexture(0, 0, 208, 47, 0, 0, 208, 47, TopTex);
    if (BottomTex)
        Device->DrawTexture(0, 184, 208, 48, 0, 0, 208, 48, BottomTex);
    Device->DrawText(7, 55, 0xFFFFFFFF, *GL2GameData.SysString[STR_Name], 0, 0, 0, L2FONT_Normal,
                     0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x1005fb40, NCGMWnd, OnPaint)

NW_CHECK_VTABLE(NCGMWnd, 0x100e4900, 69)

/*-----------------------------------------------------------------------------
    NCGMInvenItemWnd, NCGMEquipItemWnd
-----------------------------------------------------------------------------*/

// 0x1005ffb0
NCGMInvenItemWnd::NCGMInvenItemWnd()
{
}
NW_HOOK_CTOR(0x1005ffb0, NCGMInvenItemWnd)

// 0x10060050
NCGMInvenItemWnd::NCGMInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems,
                                   INT InCellSize)
    : NCInvenItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
}
NW_HOOK_CTOR_ARGS(0x10060050, NCGMInvenItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x10060000
NW_HOOK_DTOR(0x10060000, NCGMInvenItemWnd)

// 0x1005bca0 (vslot 24)
INT NCGMInvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCInvenItemWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1005bca0, NCGMInvenItemWnd, WindowProc)

// stock body shared with NCGMSkillItemWnd's OnLButtonUp (0x1005bdc0): the carried item is only
// put down
INT NCGMInvenItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    bDragging = 0;
    return 1;
}

// stock body shared with NCGMSkillItemWnd's OnLButtonUp (0x1005bdc0)
INT NCGMInvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    bDragging = 0;
    return 1;
}

// stock body shared with NCGMSkillItemWnd's OnLButtonUp (0x1005bdc0)
INT NCGMInvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bDragging = 0;
    return 1;
}

NW_CHECK_VTABLE(NCGMInvenItemWnd, 0x100e4ea8, 70)

// 0x1005ff10
NCGMEquipItemWnd::NCGMEquipItemWnd(INT InCellSize)
    : NCEquipItemWnd(InCellSize)
{
}
NW_HOOK_CTOR_ARGS(0x1005ff10, NCGMEquipItemWnd, (INT A), (A))

// 0x1005ff60
NW_HOOK_DTOR(0x1005ff60, NCGMEquipItemWnd)

// 0x1005bc40 (vslot 24)
INT NCGMEquipItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCEquipItemWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1005bc40, NCGMEquipItemWnd, WindowProc)

// 0x1005bc80 (vslot 57) - the carried item is only put down; also slots 58 and 62
INT NCGMEquipItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1005bc80, NCGMEquipItemWnd, OnLButtonUp)

// stock body shared with OnLButtonUp (0x1005bc80)
INT NCGMEquipItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    bDragging = 0;
    return 1;
}

// stock body shared with OnLButtonUp (0x1005bc80)
INT NCGMEquipItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bDragging = 0;
    return 1;
}

NW_CHECK_VTABLE(NCGMEquipItemWnd, 0x100e4da0, 65)

/*-----------------------------------------------------------------------------
    NCGMInventoryWnd
-----------------------------------------------------------------------------*/

// 0x1005c730 - the textures other than BackTex and the paperdoll are left as allocated
NCGMInventoryWnd::NCGMInventoryWnd()
{
    BackTex = NULL;
    TabCtrl = NULL;
    ItemWnd[0] = NULL;
    ItemWnd[1] = NULL;
}
NW_HOOK_CTOR(0x1005c730, NCGMInventoryWnd)

// 0x1005c790
NW_HOOK_DTOR(0x1005c790, NCGMInventoryWnd)

// stock body shared with NControl's (0x10004e90)
INT NCGMInventoryWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x1005bce0
void NCGMInventoryWnd::ClearItems()
{
    EquipWnd->Clear();
    ItemWnd[0]->Clear();
    ItemWnd[1]->Clear();
}
NW_HOOK_METHOD(0x1005bce0, NCGMInventoryWnd, ClearItems)

// 0x1005bd10 (vslot 42)
void NCGMInventoryWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
}
NW_HOOK_METHOD(0x1005bd10, NCGMInventoryWnd, ShowWindow)

// 0x1005bd30 - as the inventory's: equipped items (NItemInfo::UnknownC8C) to the paperdoll,
// quest items (type 3) to the "Quest" page, the rest to the "Item" page
UBOOL NCGMInventoryWnd::AddItem(NItemInfo* Info)
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
NW_HOOK_METHOD(0x1005bd30, NCGMInventoryWnd, AddItem)

// 0x1005c7e0 (vslot 50) - the background and the "Item" page's adena (thousands separators,
// right aligned to 184)
INT NCGMInventoryWnd::OnPaint(NCGDevice* Device)
{
    guard(NCGMInventoryWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 344, 0, 0, 240, 344, BackTex);

    TCHAR Buf[128];
    TCHAR Amount[128];
    SIZE Size;
    appSprintf(Buf, TEXT("%d"), ItemWnd[0]->GetAdena());
    FormatNumber(Amount, Buf);
    appSprintf(Buf, TEXT("%s"), Amount);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawText(184 - Size.cx, 317, 0xFFFFFFFF, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (AdenaTex)
        Device->DrawTexture(79, 317, 16, 12, 0, 0, 16, 12, AdenaTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1005c7e0, NCGMInventoryWnd, OnPaint)

// 0x1005c980 (vslot 36)
INT NCGMInventoryWnd::OnDestroy()
{
    DestroyChild(this, ItemWnd[0]);
    DestroyChild(this, ItemWnd[1]);
    DestroyChild(this, EquipWnd);
    DestroyChild(this, TabCtrl);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1005c980, NCGMInventoryWnd, OnDestroy)

// 0x10060080 (vslot 35) - the inventory's layout: the title bar, the paperdoll (32-pixel
// cells), the two pages (6 x 4 icons of 32 pixels, 160 items at most) and their tabs
INT NCGMInventoryWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.InventoryWnd.InventoryBack"));
    AdenaTex = FindTexture(TEXT("L2UI.WindowIcon.WindowIconAdena"));

    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_GMINVENTORY_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    EquipWnd = new NCGMEquipItemWnd(32);
    EquipWnd->SetWindowRect(16, 29, 207, 102);
    CreateChild(EquipWnd, NWS_VISIBLE | 0x20);

    ItemWnd[0] = new NCGMInvenItemWnd(6, 4, 160, 32);
    ItemWnd[0]->SetWindowRect(16, 166, 222, 137);
    CreateChild(ItemWnd[0], NWS_VISIBLE | 0x20);

    ItemWnd[1] = new NCGMInvenItemWnd(6, 4, 160, 32);
    ItemWnd[1]->SetWindowRect(16, 166, 222, 137);
    CreateChild(ItemWnd[1], NWS_VISIBLE | 0x20);

    TabCtrl = new NCTabControl();
    TabCtrl->ID = NID_GMINVENTORY_TABS;
    TabCtrl->SetWindowRect(15, 141, 138, 16);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(STR_Item, ItemWnd[0], 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0,
                    -1);
    TabCtrl->AddTab(STR_Quest, ItemWnd[1], 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0,
                    -1);
    return 1;
}
NW_HOOK_METHOD(0x10060080, NCGMInventoryWnd, OnCreate)

NW_CHECK_VTABLE(NCGMInventoryWnd, 0x100e3ea8, 62)

/*-----------------------------------------------------------------------------
    NCGMQuestWnd
-----------------------------------------------------------------------------*/

// 0x1005e310
NCGMQuestWnd::NCGMQuestWnd()
{
}
NW_HOOK_CTOR(0x1005e310, NCGMQuestWnd)

// 0x1005e360
NW_HOOK_DTOR(0x1005e360, NCGMQuestWnd)

// 0x1005be70 (vslot 24) - the player's window procedure first; of the GM's own ids only the
// tabs'
INT NCGMQuestWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCQuestWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_GMQUEST_TABS)
        return OnTab((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1005be70, NCGMQuestWnd, WindowProc)

// 0x1005bee0 (vslot 66) - the player's without the tutorial event
INT NCGMQuestWnd::OnTab(INT Page)
{
    if (Page == 0)
    {
        CurTab = 0;
        CancelButton->ShowWindow();
        return 1;
    }
    if (Page == 1)
    {
        CurTab = 1;
        CancelButton->HideWindow();
    }
    return 1;
}
NW_HOOK_METHOD(0x1005bee0, NCGMQuestWnd, OnTab)

// 0x1005e3b0 (vslot 35) - the player's layout under a title bar, 21 pixels lower (the list 16
// lines long and 6 seen); Cancel disabled
INT NCGMQuestWnd::OnCreate()
{
    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_GMQUEST_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    TopTex = FindTexture(TEXT("L2UI.QuestWnd.questBack1"));
    InfoBackTex = FindTexture(TEXT("L2UI.QuestWnd.questBack2"));
    ItemBackTex = FindTexture(TEXT("L2UI.QuestWnd.questBack3"));
    ItemPageIconTex = FindTexture(TEXT("sek.cbui303"));

    QuestList = new NCListBox(16, 6, 12);
    QuestList->ID = NID_GMQUEST_LIST;
    QuestList->SetWindowRect(17, 52, 220, 74);
    CreateChild(QuestList, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    QuestList->TextX = 1;
    QuestList->TextY = 1;
    QuestList->LineGap = 1;

    InfoWnd = new NCQuestInfoWnd();
    InfoWnd->SetWindowRect(18, 168, 220, 128);
    CreateChild(InfoWnd, NWS_VISIBLE | 0x20);

    ItemWnd = new NCQuestItemWnd(6, 4, 80, 32);
    ItemWnd->SetWindowRect(16, 165, 220, 139);
    CreateChild(ItemWnd, NWS_VISIBLE | 0x20);

    TabCtrl = new NCTabControl();
    TabCtrl->ID = NID_GMQUEST_TABS;
    TabCtrl->SetWindowRect(15, 137, 138, 15);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(STR_Description, InfoWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"),
                    0, -1);
    TabCtrl->AddTab(STR_QuestItem, ItemWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0,
                    -1);

    CancelButton = new NCButton(TEXT("L2UI.Control.Defaultbutton"), TEXT("L2UI.Control.Defaultbutton_click"), 0);
    CancelButton->ID = NID_GMQUEST_CANCEL;
    CancelButton->SetWindowRect(87, 298, 67, 19);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(STR_Cancel);
    CancelButton->EnableWindow(0);
    return 1;
}
NW_HOOK_METHOD(0x1005e3b0, NCGMQuestWnd, OnCreate)

// 0x1005e830 (vslot 50) - the player's painting 21 pixels lower (the title 24), the clip 11
// pixels past the bottom
INT NCGMQuestWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height + 11);
    Device->BeginPaint(ScreenX, ScreenY);
    if (TopTex)
        Device->DrawTexture(0, 21, 240, 115, 0, 0, 240, 115, TopTex);
    if (CurTab == 0 && InfoBackTex)
        Device->DrawTexture(0, 135, 240, 185, 0, 0, 240, 185, InfoBackTex);
    else if (CurTab != 0 && ItemBackTex)
        Device->DrawTexture(0, 135, 240, 185, 0, 0, 240, 185, ItemBackTex);
    if (ItemPageIconTex && CurTab)
        Device->DrawTexture(220, 136, 14, 14, 0, 0, 32, 32, ItemPageIconTex);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_AcceptedQuest], 0, 0, 0, 0, 0);
    Device->DrawText((105 - Size.cx) / 2 + 22, 27, 0xFFFFFFFF, *GL2GameData.SysString[STR_AcceptedQuest],
                     0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x1005e830, NCGMQuestWnd, OnPaint)

NW_CHECK_VTABLE(NCGMQuestWnd, 0x100e4360, 67)

/*-----------------------------------------------------------------------------
    NCGMPledgeWnd
-----------------------------------------------------------------------------*/

// 0x1005eac0
NCGMPledgeWnd::NCGMPledgeWnd()
{
}
NW_HOOK_CTOR(0x1005eac0, NCGMPledgeWnd)

// 0x1005eb10
NW_HOOK_DTOR(0x1005eb10, NCGMPledgeWnd)

// 0x1005bfc0 (vslot 24)
INT NCGMPledgeWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCPledgeWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1005bfc0, NCGMPledgeWnd, WindowProc)

// 0x1005c000 (vslot 36)
INT NCGMPledgeWnd::OnDestroy()
{
    NCPledgeWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1005c000, NCGMPledgeWnd, OnDestroy)

// 0x1005eb60 (vslot 50) - the player's page one pixel to the left: the clip starts at the top
// of the window (the background drawn 17 pixels above it loses its first 17 rows, 300 of them
// drawn instead of 301) and runs 17 past the bottom; the crest by its texture name when the
// network handler knows it (UL2NetHandler slot 41), kept in CrestTex
INT NCGMPledgeWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height + 17);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, -17, 240, 300, 0, 0, 240, 300, BackTex);

    INT NameX = 17;
    if (Info.CrestID > 0 && GConsole->Engine->NetHandler->IsImportedPledgeCrestID(Info.CrestID))
    {
        TCHAR CrestName[64];
        appSprintf(CrestName, TEXT("crest.%d_%d"), GConsole->GetServerID(), Info.CrestID);
        CrestTex = FindTexture(CrestName);
        if (CrestTex)
        {
            Device->DrawTexture(17, 18, 16, 12, 0, 4, 16, 12, CrestTex);
            NameX = 37;
        }
    }
    SIZE Size;
    const TCHAR* Name = Info.Name;
    if (Name && *Name)
    {
        NCGDevice::GetTextExtent(&Size, Name, 0, 0, 0, 0, 0);  // not used
        Device->DrawText(NameX, 19, CLAN_TEXT_COLOR, Name, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }

    if (*GL2GameData.SysString[STR_Leader])
        Device->DrawText(17, 33, CLAN_LABEL_COLOR, *GL2GameData.SysString[STR_Leader], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    const TCHAR* Leader = Info.LeaderName;
    if (Leader && *Leader)
        Device->DrawText(60, 33, CLAN_TEXT_COLOR, Leader, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    const TCHAR* Status = Info.StatusText;
    if (Status && *Status)
    {
        NCGDevice::GetTextExtent(&Size, Leader, 0, 0, 0, 0, 0);
        Device->DrawText(Size.cx + 48, 33, CLAN_TEXT_COLOR, Status, 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                         0, 0, 0);
    }
    if (*GL2GameData.SysString[STR_Lv])
        Device->DrawText(139, 33, CLAN_LABEL_COLOR, *GL2GameData.SysString[STR_Lv], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    TCHAR Number[10];
    appSprintf(Number, TEXT("%d"), Info.Level);
    Device->DrawText(158, 33, CLAN_TEXT_COLOR, Number, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);

    if (*GL2GameData.SysString[STR_Base])
        Device->DrawText(17, 47, CLAN_LABEL_COLOR, *GL2GameData.SysString[STR_Base], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (Info.bHasCastle)
    {
        Device->DrawText(60, 47, CLAN_TEXT_COLOR, *GL2GameData.SysString[STR_CastleExists], 0, 0,
                         0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        if (Info.bHasHideout)
        {
            NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_CastleExists], 0, 0, 0, 0,
                                     0);
            Device->DrawText(Size.cx + 62, 47, CLAN_TEXT_COLOR,
                             *GL2GameData.SysString[STR_HideoutExists], 0, 0, 0, L2FONT_Normal, 0,
                             0, 0, 0, 0, 0);
        }
    }
    else if (Info.bHasHideout)
        Device->DrawText(60, 47, CLAN_TEXT_COLOR, *GL2GameData.SysString[STR_HideoutExists], 0, 0,
                         0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x1005eb60, NCGMPledgeWnd, OnPaint)

NW_CHECK_VTABLE(NCGMPledgeWnd, 0x100e46b0, 72)

/*-----------------------------------------------------------------------------
    NCGMActionWnd
-----------------------------------------------------------------------------*/

// the clan page's nine buttons disabled (repeated in OnCreate and ShowWindow)
static FORCEINLINE void DisablePledgeButtons(NCPledgeWnd* Pledge)
{
    Pledge->TitleButton->EnableWindow(0);
    Pledge->CrestButton->EnableWindow(0);
    Pledge->WarButton->EnableWindow(0);
    Pledge->TruceButton->EnableWindow(0);
    Pledge->SurrenderButton->EnableWindow(0);
    Pledge->ExpelButton->EnableWindow(0);
    Pledge->YieldButton->EnableWindow(0);
    Pledge->LeaveButton->EnableWindow(0);
    Pledge->InviteButton->EnableWindow(0);
}

// 0x1005ef40 - PledgeWnd is left as allocated
NCGMActionWnd::NCGMActionWnd()
{
    BackTex = NULL;
}
NW_HOOK_CTOR(0x1005ef40, NCGMActionWnd)

// 0x1005ef90
NW_HOOK_DTOR(0x1005ef90, NCGMActionWnd)

// stock body shared with NControl's (0x10004e90)
INT NCGMActionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x1005efe0 (vslot 35) - the title bar and the clan page, hidden, its buttons disabled
INT NCGMActionWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_GMACTION_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI.AcitonWnd.PledgeBack"));

    PledgeWnd = new NCGMPledgeWnd();
    PledgeWnd->SetWindowRect(0, 21, 240, 300);
    CreateChild(PledgeWnd, NWS_VISIBLE);
    PledgeWnd->HideWindow();
    DisablePledgeButtons(PledgeWnd);
    return 1;
}
NW_HOOK_METHOD(0x1005efe0, NCGMActionWnd, OnCreate)

// stock body shared with NCPetActionWnd's (0x100bb540, hooked there)
INT NCGMActionWnd::OnDestroy()
{
    DestroyChild(this, PledgeWnd);
    NCWnd::OnDestroy();
    return 1;
}

// 0x1005c010 (vslot 42) - the clan page's buttons disabled again (its own ShowWindow enables
// them by rank)
void NCGMActionWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    DisablePledgeButtons(PledgeWnd);
}
NW_HOOK_METHOD(0x1005c010, NCGMActionWnd, ShowWindow)

// stock body shared with NCActionWnd's (0x1005f200): draws nothing
INT NCGMActionWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}

NW_CHECK_VTABLE(NCGMActionWnd, 0x100e47d0, 62)
