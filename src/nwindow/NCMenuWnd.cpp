/*=============================================================================
    NCMenuWnd.cpp - the main menu bar. Stock: 0x10076db0 .. 0x10076ef5, 0x10077ab0 ..
    0x10077f67, plus OnDestroy 0x100367a0 (the body NCActionWnd and NCServerSelectWnd share).
=============================================================================*/
#include "nwindow/NCMenuWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x10077ab0
NCMenuWnd::NCMenuWnd()
{
    BackTex = NULL;
    SystemButton = NULL;
    InventoryButton = NULL;
    CharacterButton = NULL;
    CommunityButton = NULL;
}
NW_HOOK_CTOR(0x10077ab0, NCMenuWnd)

// 0x10077b20
NW_HOOK_DTOR(0x10077b20, NCMenuWnd)

// 0x10076db0 (vslot 24)
INT NCMenuWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_MENU_INVENTORY)
            return OnInventory() != 0;
        if (wParam == NID_MENU_CHARACTER)
            return OnCharacter() != 0;
        if (wParam == NID_MENU_SYSTEM)
            return OnSystem() != 0;
        if (wParam == NID_MENU_COMMUNITY)
            return OnCommunity() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10076db0, NCMenuWnd, WindowProc)

// one icon of the bar (repeated inline in the stock OnCreate)
static FORCEINLINE NCButton* CreateMenuIcon(NCWnd* Parent, const TCHAR* NormalTexName,
                                const TCHAR* PushedTexName, INT ID, INT X, INT ToolTipID)
{
    NCButton* Button = new NCButton(NormalTexName, PushedTexName, 0);
    Button->ID = ID;
    Button->SetWindowRect(X, 5, 34, 34);
    Parent->CreateChild(Button, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    Button->SetToolTip(ToolTipID, 1);
    return Button;
}

// 0x10077bf0 (vslot 35) - NCWnd::OnCreate is not called
INT NCMenuWnd::OnCreate()
{
    BackTex = FindTexture(TEXT("L2UI.EtcWndBack.SysMenuBack"));
    FrameCtrl = new NCFrameCtrl(0);
    FrameID = NID_MENU_FRAME;
    FrameCtrl->ID = NID_MENU_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 11, 44);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    CharacterButton = CreateMenuIcon(this, TEXT("L2UI.WindowIcon.MainCharacter_large"),
                                     TEXT("L2UI.WindowIcon.MainCharacter_large_click"),
                                     NID_MENU_CHARACTER, 16, 0x1B2);
    InventoryButton = CreateMenuIcon(this, TEXT("L2UI.WindowIcon.MainInventory_large"),
                                     TEXT("L2UI.WindowIcon.MainInventory_large_click"),
                                     NID_MENU_INVENTORY, 53, 0xC3);
    CommunityButton = CreateMenuIcon(this, TEXT("L2UI.WindowIcon.MainCommunity_large"),
                                     TEXT("L2UI.WindowIcon.MainCommunity_large_click"),
                                     NID_MENU_COMMUNITY, 90, 0x186);
    SystemButton = CreateMenuIcon(this, TEXT("L2UI.WindowIcon.MainSystem_large"),
                                  TEXT("L2UI.WindowIcon.MainSystem_large_click"),
                                  NID_MENU_SYSTEM, 127, 0xC1);
    return 1;
}
NW_HOOK_METHOD(0x10077bf0, NCMenuWnd, OnCreate)

// 0x100367a0 (vslot 36) - body shared with NCActionWnd and NCServerSelectWnd (their four
// controls sit at the same +0x98 .. +0xA4)
INT NCMenuWnd::OnDestroy()
{
    if (SystemButton)
    {
        RemoveChildWindow(SystemButton->Entry);
        delete SystemButton;
        SystemButton = NULL;
    }
    if (InventoryButton)
    {
        RemoveChildWindow(InventoryButton->Entry);
        delete InventoryButton;
        InventoryButton = NULL;
    }
    if (CharacterButton)
    {
        RemoveChildWindow(CharacterButton->Entry);
        delete CharacterButton;
        CharacterButton = NULL;
    }
    if (CommunityButton)
    {
        RemoveChildWindow(CommunityButton->Entry);
        delete CommunityButton;
        CommunityButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100367a0, NCMenuWnd, OnDestroy)

// 0x10077b70 (vslot 50) - the clip is set before the origin, as in the stock code
INT NCMenuWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10077b70, NCMenuWnd, OnPaint)

// 0x10076e80 (vslot 62)
INT NCMenuWnd::OnSystem()
{
    GDesktop->ToggleSystemMenu();
    return 1;
}
NW_HOOK_METHOD(0x10076e80, NCMenuWnd, OnSystem)

// 0x10076ea0 (vslot 63)
INT NCMenuWnd::OnCharacter()
{
    GDesktop->ToggleCharacter(-1);
    return 1;
}
NW_HOOK_METHOD(0x10076ea0, NCMenuWnd, OnCharacter)

// 0x10076ec0 (vslot 64)
INT NCMenuWnd::OnInventory()
{
    GDesktop->ToggleInventory();
    return 1;
}
NW_HOOK_METHOD(0x10076ec0, NCMenuWnd, OnInventory)

// 0x10076ee0 (vslot 65)
INT NCMenuWnd::OnCommunity()
{
    GDesktop->ToggleBoard();
    return 1;
}
NW_HOOK_METHOD(0x10076ee0, NCMenuWnd, OnCommunity)

NW_CHECK_VTABLE(NCMenuWnd, 0x100e7d50, 66)
