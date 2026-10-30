/*=============================================================================
    NCTestUIWnd.cpp - a test window. Stock: 0x100759d0 .. 0x10075bb1; WindowProc is a body
    shared with other classes.
=============================================================================*/
#include "nwindow/NCTestUIWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"

enum { NID_TESTUI_FRAME = 0xF6 };  // title bar id (name ours)

// 0x100759d0
NCTestUIWnd::NCTestUIWnd()
{
}
NW_HOOK_CTOR(0x100759d0, NCTestUIWnd)

// 0x10075a20
NW_HOOK_DTOR(0x10075a20, NCTestUIWnd)

// stock body shared with NCItemWnd's (0x1006bb90)
INT NCTestUIWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCScrollWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10075a70 (vslot 35) - the title bar gets its id, the window's FrameID stays unset
INT NCTestUIWnd::OnCreate()
{
    NCScrollWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_TESTUI_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 512, 24);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("NWindow.ChatBack"));
    return 1;
}
NW_HOOK_METHOD(0x10075a70, NCTestUIWnd, OnCreate)

// 0x10075b30 (vslot 50) - a 32 x 32 texture stretched to 512 x 512 under the title bar. The
// clip goes on before the origin.
INT NCTestUIWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawStretchedTex(0, 24, 512, 512, 0, 0, 32, 32, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10075b30, NCTestUIWnd, OnPaint)

NW_CHECK_VTABLE(NCTestUIWnd, 0x100e7780, 65)
