/*=============================================================================
    NCMainWnd.cpp - the character window (status, skills, actions, quests).
    Stock: 0x1003e720 .. 0x1003e7f7, 0x1003e8b0 .. 0x1003e94f, 0x10040f30 .. 0x100417e6.
=============================================================================*/
#include "nwindow/NCMainWnd.h"
#include "nwindow/NCActionWnd.h"
#include "nwindow/NCDetailStatusWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCQuestWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NClasses.gen.h"

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_WindowOpen = 5, SOUND_WindowClose = 6 };

// the title of each page (inlined twice in the stock code, name ours); other tabs keep it
static FORCEINLINE void SetTabTitle(NCFrameCtrl* Frame, INT Tab)
{
    if (Tab == MAINTAB_Status)
        Frame->SetTitleText(*GL2GameData.SysString[433]);    // "Status"
    else if (Tab == MAINTAB_Skills)
        Frame->SetTitleText(*GL2GameData.SysString[119]);    // "Skills"
    else if (Tab == MAINTAB_Actions)
        Frame->SetTitleText(*GL2GameData.SysString[127]);    // "Actions"
    else if (Tab == MAINTAB_Quests)
        Frame->SetTitleText(*GL2GameData.SysString[118]);    // "Quest"
}

// 0x1003e8b0
NCMainWnd::NCMainWnd()
{
}
NW_HOOK_CTOR(0x1003e8b0, NCMainWnd)

// 0x1003e900
NW_HOOK_DTOR(0x1003e900, NCMainWnd)

// 0x1003e720 (vslot 24)
INT NCMainWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == NID_MAIN_TABS)
        return OnTabChanged((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1003e720, NCMainWnd, WindowProc)

// 0x1003e790 (vslot 62) - a tab clicked: the page's title
INT NCMainWnd::OnTabChanged(INT Tab)
{
    SetTabTitle(FrameCtrl, Tab);
    return 1;
}
NW_HOOK_METHOD(0x1003e790, NCMainWnd, OnTabChanged)

// 0x10040f30 (vslot 35) - the quest page starts hidden
INT NCMainWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_MAIN_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI.NWindow.mainwndback"));

    DetailWnd = new NCDetailStatusWnd;
    DetailWnd->SetWindowRect(0, 65, 240, 300);
    CreateChild(DetailWnd, NWS_VISIBLE);

    SkillWnd = new NCMagicSkillWnd;
    SkillWnd->SetWindowRect(0, 64, 240, 300);
    CreateChild(SkillWnd, NWS_VISIBLE);

    ActionWnd = new NCActionWnd;
    ActionWnd->SetWindowRect(0, 65, 240, 300);
    CreateChild(ActionWnd, NWS_VISIBLE);

    // CreateChild with the page hidden before its entry is set
    QuestWnd = new NCQuestWnd;
    QuestWnd->SetWindowRect(0, 65, 240, 300);
    NWndEntry* E = AddChildWindow(QuestWnd, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    QuestWnd->HideWindow();
    QuestWnd->Entry = E;
    if (E->Parent)
    {
        NCWnd* Parent = E->Parent->Wnd;
        QuestWnd->ScreenX += Parent->ScreenX;
        QuestWnd->ScreenY += Parent->ScreenY;
    }
    QuestWnd->WindowProc(WM_CREATE, 0, 0);

    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_MAIN_TABS;
    TabCtrl->SetWindowRect(30, 24, 179, 38);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(TEXT(" "), DetailWnd, 38, 38, TEXT("L2UI.NWindow.mainwndbutton1"),
                    TEXT("L2UI.NWindow.mainwndbutton1-1"), 9, 0xC2);   // "Status (Alt+T)"
    TabCtrl->AddTab(TEXT(" "), SkillWnd, 38, 38, TEXT("L2UI.NWindow.mainwndbutton2"),
                    TEXT("L2UI.NWindow.mainwndbutton2-1"), 9, 0xC4);   // "Skills & Magic (Alt+K)"
    TabCtrl->AddTab(TEXT(" "), ActionWnd, 38, 38, TEXT("L2UI.NWindow.mainwndbutton3"),
                    TEXT("L2UI.NWindow.mainwndbutton3-1"), 9, 0xC5);   // "Actions (Alt+C)"
    TabCtrl->AddTab(TEXT(" "), QuestWnd, 38, 38, TEXT("L2UI.NWindow.mainwndbutton4"),
                    TEXT("L2UI.NWindow.mainwndbutton4-1"), 9, 0xC6);   // "Quests (Alt+U)"
    return 1;
}
NW_HOOK_METHOD(0x10040f30, NCMainWnd, OnCreate)

// 0x10041380 (vslot 36)
INT NCMainWnd::OnDestroy()
{
    if (DetailWnd)
    {
        RemoveChildWindow(DetailWnd->Entry);
        delete DetailWnd;
        DetailWnd = NULL;
    }
    if (SkillWnd)
    {
        RemoveChildWindow(SkillWnd->Entry);
        delete SkillWnd;
        SkillWnd = NULL;
    }
    if (ActionWnd)
    {
        RemoveChildWindow(ActionWnd->Entry);
        delete ActionWnd;
        ActionWnd = NULL;
    }
    if (QuestWnd)
    {
        RemoveChildWindow(QuestWnd->Entry);
        delete QuestWnd;
        QuestWnd = NULL;
    }
    if (TabCtrl)
    {
        RemoveChildWindow(TabCtrl->Entry);
        delete TabCtrl;
        TabCtrl = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10041380, NCMainWnd, OnDestroy)

// 0x10041470 - the shortcut of a page: hides the window when that page is already showing,
// else shows the window on it (from its icon when minimized)
void NCMainWnd::ShowTab(INT Tab)
{
    UBOOL bShown = 0;
    if (Tab == MAINTAB_Status)
        bShown = DetailWnd->IsShowWindow();
    else if (Tab == MAINTAB_Skills)
        bShown = SkillWnd->IsShowWindow();
    else if (Tab == MAINTAB_Actions)
        bShown = ActionWnd->IsShowWindow();
    else if (Tab == MAINTAB_Quests)
        bShown = QuestWnd->IsShowWindow();
    if (bShown)
    {
        GConsole->Engine->OnInterfacePlaySound(SOUND_WindowClose);
        HideWindow();
        return;
    }
    Style |= NWS_VISIBLE;
    FrameCtrl->ShowWindow();
    TabCtrl->ShowWindow();
    TabCtrl->SetTab(Tab);
    SetTabTitle(FrameCtrl, Tab);
    if (Icon)
    {
        SetWindowRect(Icon->ScreenX, Icon->ScreenY, Width, Height);
        Icon->SetWindowRect(0, 0, 0, 0);
        Icon->HideWindow();
    }
    GConsole->Engine->OnInterfacePlaySound(SOUND_WindowOpen);
}
NW_HOOK_METHOD(0x10041470, NCMainWnd, ShowTab)

// 0x100415e0 (vslot 42) - only the title bar, the tabs and the current page (and back from the
// icon at its place)
void NCMainWnd::ShowWindow()
{
    Style |= NWS_VISIBLE;
    FrameCtrl->ShowWindow();
    TabCtrl->ShowWindow();
    TabCtrl->ShowCurrentPage();
    if (Icon)
    {
        SetWindowRect(Icon->ScreenX, Icon->ScreenY, Width, Height);
        Icon->SetWindowRect(0, 0, 0, 0);
        Icon->HideWindow();
    }
}
NW_HOOK_METHOD(0x100415e0, NCMainWnd, ShowWindow)

// 0x10041660 (vslot 50)
INT NCMainWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 44, 0, 0, 240, 44, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10041660, NCMainWnd, OnPaint)

// 0x100416f0 (vslot 47) - NCWnd's with the icon of the current page (the tests are not
// exclusive; a page is one class). Unlike NCWnd's, the icon is brought back on the screen
// with MoveWindow (vslot 44), which snaps.
void NCMainWnd::Minimize()
{
    NCWnd* Page = TabCtrl->CurrentPage;
    if (Page->IsA(NClass::NCDetailStatusWnd()))
        Icon->SetIcon(TEXT("L2UI.WindowIcon.MainStatus_large"), 0xC2, 1);
    if (Page->IsA(NClass::NCMagicSkillWnd()))
        Icon->SetIcon(TEXT("L2UI.WindowIcon.MainSkill_large"), 0xC4, 1);
    if (Page->IsA(NClass::NCActionWnd()))
        Icon->SetIcon(TEXT("L2UI.WindowIcon.MainAction_large"), 0xC5, 1);
    if (Page->IsA(NClass::NCQuestWnd()))
        Icon->SetIcon(TEXT("L2UI.WindowIcon.MainQuest_large"), 0xC6, 1);
    if (ScreenX < 0)
        Icon->MoveWindow(-ScreenX, 0);
    HideWindow();
    Icon->ShowWindow();
}
NW_HOOK_METHOD(0x100416f0, NCMainWnd, Minimize)

NW_CHECK_VTABLE(NCMainWnd, 0x100e0c00, 63)
