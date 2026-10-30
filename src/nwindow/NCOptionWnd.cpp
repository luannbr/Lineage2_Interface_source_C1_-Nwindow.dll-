/*=============================================================================
    NCOptionWnd.cpp - the options window. Stock: 0x1007de70 .. 0x1007df36, 0x100817c0 ..
    0x100820fa, plus OnCancel 0x10059d00 (the body NCMatchOptionWnd and two other windows
    share).
=============================================================================*/
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCVideoOptionWnd.h"
#include "nwindow/NCAudioOptionWnd.h"
#include "nwindow/NCGameOptionWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x100817c0 - only BackTex is cleared
NCOptionWnd::NCOptionWnd()
{
    BackTex = NULL;
}
NW_HOOK_CTOR(0x100817c0, NCOptionWnd)

// 0x10081810
NW_HOOK_DTOR(0x10081810, NCOptionWnd)

// 0x1007de70 (vslot 24)
INT NCOptionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_OPTION_OK)
            return OnOK() != 0;
        if (wParam == NID_OPTION_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_OPTION_APPLY)
            return OnApply() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1007de70, NCOptionWnd, WindowProc)

// a page of the window (repeated inline in the stock OnCreate)
static FORCEINLINE void PlacePage(NCWnd* Parent, NCWnd* Page)
{
    Page->SetWindowRect(0, 53, 208, 270);
    Parent->CreateChild(Page, NWS_VISIBLE);
}

// one button along the bottom (repeated inline in the stock OnCreate)
static FORCEINLINE NCButton* CreateOptionButton(NCWnd* Parent, INT ID, INT X, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                    TEXT("L2UI.Control.DefaultButton_click"), 0);
    Button->ID = ID;
    Button->SetWindowRect(X, 337, 67, 19);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->SetTextID(TextID);
    return Button;
}

// 0x10081860 (vslot 35) - tabs "Video", "Audio", "Game" (sysstring 206 .. 208)
INT NCOptionWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_OPTION_FRAME;
    FrameCtrl->ID = NID_OPTION_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI.etcWnd.OptionBack"));

    VideoWnd = new NCVideoOptionWnd;
    PlacePage(this, VideoWnd);
    AudioWnd = new NCAudioOptionWnd;
    PlacePage(this, AudioWnd);
    GameWnd = new NCGameOptionWnd;
    PlacePage(this, GameWnd);

    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_OPTION_TABS;
    TabCtrl->SetWindowRect(15, 23, 207, 16);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(0xCE, VideoWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"),
                    0, -1);
    TabCtrl->AddTab(0xCF, AudioWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"),
                    0, -1);
    TabCtrl->AddTab(0xD0, GameWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"),
                    0, -1);

    OKButton = CreateOptionButton(this, NID_OPTION_OK, 14, 0x8C);
    CancelButton = CreateOptionButton(this, NID_OPTION_CANCEL, 87, 0x8D);
    ApplyButton = CreateOptionButton(this, NID_OPTION_APPLY, 159, 0xDA);
    return 1;
}
NW_HOOK_METHOD(0x10081860, NCOptionWnd, OnCreate)

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

// 0x10081ed0 (vslot 36)
INT NCOptionWnd::OnDestroy()
{
    DestroyChild(this, VideoWnd);
    DestroyChild(this, AudioWnd);
    DestroyChild(this, GameWnd);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    DestroyChild(this, ApplyButton);
    DestroyChild(this, TabCtrl);
    DestroyChild(this, UnusedB4);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10081ed0, NCOptionWnd, OnDestroy)

// 0x1007df20 (vslot 42)
void NCOptionWnd::ShowWindow()
{
#if NW_FEATURES
    VideoWnd->SyncModeBox();   // phase 6: the "Full Screen" box shows the mode of the viewport now
#endif
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
}
NW_HOOK_METHOD(0x1007df20, NCOptionWnd, ShowWindow)

// 0x10081df0 (vslot 50) - under the title bar; the clip is set before the origin
INT NCOptionWnd::OnPaint(NCGDevice* Device)
{
    guard(NCOptionWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 344, 0, 0, 240, 344, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10081df0, NCOptionWnd, OnPaint)

// every page applies its settings (repeated in OnOK and OnApply)
static FORCEINLINE void ApplyPages(NCOptionWnd* Wnd)
{
    Wnd->VideoWnd->Apply();
    Wnd->AudioWnd->Apply();
    Wnd->GameWnd->Apply();
    GConfig->SetString(TEXT("FirstRun"), TEXT("FirstRun"), TEXT("1"), TEXT("Option.ini"));
}

// 0x10082050 (vslot 62)
INT NCOptionWnd::OnOK()
{
    ApplyPages(this);
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10082050, NCOptionWnd, OnOK)

// 0x10059d00 (vslot 63) - body shared with NCMatchOptionWnd's slot 63 and two other windows'
// (slots 64 and 72)
INT NCOptionWnd::OnCancel()
{
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10059d00, NCOptionWnd, OnCancel)

// 0x100820b0 (vslot 64)
INT NCOptionWnd::OnApply()
{
    ApplyPages(this);
    return 1;
}
NW_HOOK_METHOD(0x100820b0, NCOptionWnd, OnApply)

NW_CHECK_VTABLE(NCOptionWnd, 0x100e9c70, 65)
