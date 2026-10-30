/*=============================================================================
    NCSkillWnd.cpp - the skill list and its GM copy. Stock: 0x10072980 .. 0x1007299d,
    0x10073190 .. 0x100732ee, 0x10073af0 .. 0x10073d18, 0x1005bde0 .. 0x1005be38 (WindowProc
    is NCButton's body, OnPaint NCActionWnd's), NCGMSkillWnd's 0x1005ca50 .. 0x1005cb0f and
    0x100604d0 .. 0x100606f8.
=============================================================================*/
#include "nwindow/NCSkillWnd.h"
#include "nwindow/NCSkillItemWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NCGDevice.h"

/*-----------------------------------------------------------------------------
    NCSkillWnd
-----------------------------------------------------------------------------*/

// 0x10073190 - NControl's constructor is inlined; the lists are left as allocated
NCSkillWnd::NCSkillWnd()
{
    TabCtrl = NULL;
}
NW_HOOK_CTOR(0x10073190, NCSkillWnd)

// 0x10073200
NW_HOOK_DTOR(0x10073200, NCSkillWnd)

// stock body shared with NCButton's (0x100051f0)
INT NCSkillWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}

// a list of the window (repeated inline in the stock OnCreate)
static FORCEINLINE NCSkillItemWnd* CreateSkillList(NCWnd* Parent)
{
    NCSkillItemWnd* List = new NCSkillItemWnd(6, 7, 80, 32);
    List->SetWindowRect(16, 33, 222, 242);
    Parent->CreateChild(List, NWS_VISIBLE | 0x20);
    return List;
}

// 0x10073af0 (vslot 35) - tabs "Active" / "Passive" (sysstring 120 / 121)
INT NCSkillWnd::OnCreate()
{
    NControl::OnCreate();
    ActiveList = CreateSkillList(this);
    PassiveList = CreateSkillList(this);
    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_SKILL_TABS;
    TabCtrl->SetWindowRect(15, 2, 138, 15);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(0x78, ActiveList, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"),
                    0, -1);
    TabCtrl->AddTab(0x79, PassiveList, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"),
                    0, -1);
    return 1;
}
NW_HOOK_METHOD(0x10073af0, NCSkillWnd, OnCreate)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x10073250 (vslot 36) - also NCGMSkillWnd's
INT NCSkillWnd::OnDestroy()
{
    DestroyChild(this, ActiveList);
    DestroyChild(this, PassiveList);
    DestroyChild(this, TabCtrl);
    NControl::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10073250, NCSkillWnd, OnDestroy)

// 0x1005bde0 (vslot 42) - also NCGMSkillWnd's
void NCSkillWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
}
NW_HOOK_METHOD(0x1005bde0, NCSkillWnd, ShowWindow)

// stock body shared with NCActionWnd's (0x1005f200): draws nothing
INT NCSkillWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}

// 0x1005be00 - into the passive list when NItemInfo::Param is 1; also NCGMSkillWnd's
INT NCSkillWnd::AddSkill(NItemInfo* Info)
{
    if (Info->Param == 1)
        PassiveList->AddSkill(Info);
    else
        ActiveList->AddSkill(Info);
    return 1;
}
NW_HOOK_METHOD(0x1005be00, NCSkillWnd, AddSkill)

// 0x10072980 - the console empties the lists before the server's list comes again; also
// NCGMSkillWnd's
void NCSkillWnd::ClearLists()
{
    ActiveList->Clear();
    PassiveList->Clear();
}
NW_HOOK_METHOD(0x10072980, NCSkillWnd, ClearLists)

NW_CHECK_VTABLE(NCSkillWnd, 0x100e71d8, 63)

/*-----------------------------------------------------------------------------
    NCGMSkillWnd
-----------------------------------------------------------------------------*/

// 0x1005ca50 - NControl's constructor is inlined; the lists are left as allocated
NCGMSkillWnd::NCGMSkillWnd()
{
    TabCtrl = NULL;
}
NW_HOOK_CTOR(0x1005ca50, NCGMSkillWnd)

// 0x1005cac0
NW_HOOK_DTOR(0x1005cac0, NCGMSkillWnd)

// stock body shared with NCButton's (0x100051f0)
INT NCGMSkillWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}

// a list of the GM window (repeated inline in the stock OnCreate)
static FORCEINLINE NCGMSkillItemWnd* CreateGMSkillList(NCWnd* Parent)
{
    NCGMSkillItemWnd* List = new NCGMSkillItemWnd(6, 7, 80, 32);
    List->SetWindowRect(16, 33, 222, 242);
    Parent->CreateChild(List, NWS_VISIBLE | 0x20);
    return List;
}

// 0x100604d0 (vslot 35) - NCSkillWnd's with GM grids and another tab control id
INT NCGMSkillWnd::OnCreate()
{
    NControl::OnCreate();
    ActiveList = CreateGMSkillList(this);
    PassiveList = CreateGMSkillList(this);
    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_GMSKILL_TABS;
    TabCtrl->SetWindowRect(15, 2, 138, 15);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(0x78, ActiveList, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"),
                    0, -1);
    TabCtrl->AddTab(0x79, PassiveList, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"),
                    0, -1);
    return 1;
}
NW_HOOK_METHOD(0x100604d0, NCGMSkillWnd, OnCreate)

// stock body shared with NCSkillWnd's (0x10073250)
INT NCGMSkillWnd::OnDestroy()
{
    DestroyChild(this, ActiveList);
    DestroyChild(this, PassiveList);
    DestroyChild(this, TabCtrl);
    NControl::OnDestroy();
    return 1;
}

// stock body shared with NCSkillWnd's (0x1005bde0)
void NCGMSkillWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
}

// stock body shared with NCActionWnd's (0x1005f200): draws nothing
INT NCGMSkillWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}

// stock body shared with NCSkillWnd's (0x1005be00)
INT NCGMSkillWnd::AddSkill(NItemInfo* Info)
{
    if (Info->Param == 1)
        PassiveList->AddSkill(Info);
    else
        ActiveList->AddSkill(Info);
    return 1;
}

// stock body shared with NCSkillWnd's (0x10072980)
void NCGMSkillWnd::ClearLists()
{
    ActiveList->Clear();
    PassiveList->Clear();
}

NW_CHECK_VTABLE(NCGMSkillWnd, 0x100e3fd8, 63)
