/*=============================================================================
    NCActionWnd.cpp - the action window (Basic and Clan tabs). Stock: 0x100364c0 ..
    0x1003679e, 0x10036870 .. 0x1003687a, 0x10036e20 .. 0x10036e61, 0x10037250 ..
    0x1003735b, 0x1005f200 .. 0x1005f24f (OnDestroy is NCMenuWnd's body).
=============================================================================*/
#include "nwindow/NCActionWnd.h"
#include "nwindow/NCActionItemListWnd.h"
#include "nwindow/NCActionItemWnd.h"
#include "nwindow/NCPledgeWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x100364c0 - only BackTex is cleared
NCActionWnd::NCActionWnd()
{
    BackTex = NULL;
}
NW_HOOK_CTOR(0x100364c0, NCActionWnd)

// 0x10036510
NW_HOOK_DTOR(0x10036510, NCActionWnd)

// stock body shared with NControl's (0x10004e90)
INT NCActionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// a page of the window (repeated inline in the stock OnCreate)
static FORCEINLINE void PlacePage(NCWnd* Parent, NCWnd* Page)
{
    Page->SetWindowRect(0, 16, 240, 284);
    Parent->CreateChild(Page, NWS_VISIBLE);
}

// 0x10036560 (vslot 35) - tabs "Basic" / "Clan" (sysstring 129 / 314)
INT NCActionWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("sek.cbui304"));
    ActionList = new NCActionItemListWnd;
    PlacePage(this, ActionList);
    PledgeWnd = new NCPledgeWnd;
    PlacePage(this, PledgeWnd);
    PledgeWnd->HideWindow();
    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_ACTION_TABS;
    TabCtrl->SetWindowRect(15, 1, 138, 15);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(0x81, ActionList, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"),
                    0, -1);
    TabCtrl->AddTab(0x13A, PledgeWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"),
                    0, -1);
    return 1;
}
NW_HOOK_METHOD(0x10036560, NCActionWnd, OnCreate)

// stock body shared with NCMenuWnd's (0x100367a0)
INT NCActionWnd::OnDestroy()
{
    if (ActionList)
    {
        RemoveChildWindow(ActionList->Entry);
        delete ActionList;
        ActionList = NULL;
    }
    if (PledgeWnd)
    {
        RemoveChildWindow(PledgeWnd->Entry);
        delete PledgeWnd;
        PledgeWnd = NULL;
    }
    if (TabCtrl)
    {
        RemoveChildWindow(TabCtrl->Entry);
        delete TabCtrl;
        TabCtrl = NULL;
    }
    if (UnusedA4)
    {
        RemoveChildWindow(UnusedA4->Entry);
        delete UnusedA4;
        UnusedA4 = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}

// 0x10037330 (vslot 42) - with a clan, the member list is asked for first
void NCActionWnd::ShowWindow()
{
    if (PledgeWnd->GetPledgeName())
        GConsole->RequestPledgeMemberList();
    NCWnd::ShowWindow();
    UpdatePledgeTab();
}
NW_HOOK_METHOD(0x10037330, NCActionWnd, ShowWindow)

// 0x1005f200 (vslot 50) - draws nothing (BackTex is not used)
INT NCActionWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x1005f200, NCActionWnd, OnPaint)

// 0x10036870
void NCActionWnd::ResetPledgeWnd()
{
    PledgeWnd->Reset();
}
NW_HOOK_METHOD(0x10036870, NCActionWnd, ResetPledgeWnd)

// 0x10036e20 - after a language change (NConsoleWnd::ReloadGameTexts)
void NCActionWnd::ReloadActions()
{
    ActionList->BasicGrid->ClearItems();
    ActionList->PartyGrid->ClearItems();
    ActionList->SocialGrid->ClearItems();
    ActionList->LoadActions();
}
NW_HOOK_METHOD(0x10036e20, NCActionWnd, ReloadActions)

// 0x10037250 - no clan: the Clan tab is disabled and Basic selected. A hidden window hides
// again (NCWnd's, called directly), which also hides the page just shown.
__declspec(noinline) void NCActionWnd::UpdatePledgeTab()
{
    TArray<NCRadioButton*> Tabs = TabCtrl->GetTabs();
    if (Tabs.Num() > 0)
    {
        if (!PledgeWnd->GetPledgeName())
        {
            Tabs(1)->EnableWindow(0);
            TabCtrl->SetTab(0);
        }
        else
            Tabs(1)->EnableWindow(1);
        TabCtrl->ShowCurrentPage();
    }
    if (!IsShowWindow())
        NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x10037250, NCActionWnd, UpdatePledgeTab)

NW_CHECK_VTABLE(NCActionWnd, 0x100dfb30, 62)
