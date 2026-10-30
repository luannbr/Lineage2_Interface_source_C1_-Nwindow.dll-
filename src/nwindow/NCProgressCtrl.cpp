/*=============================================================================
    NCProgressCtrl.cpp - count-down bar. Stock: 0x10005240 .. 0x100052ab,
    0x10005730 .. 0x10005a7e.
=============================================================================*/
#include "nwindow/NCProgressCtrl.h"
#include "nwindow/NCGDevice.h"

// 0x10005730
NCProgressCtrl::NCProgressCtrl()
{
    Unknown1A8 = 1;
    bStopped = 1;
    BarTex = NULL;
    BackTex = NULL;
    Border = 2;
    Pos = 0;
    Max = 0;
    BackTex = FindTexture(TEXT("NWindow.BlackTexture"));
    BarTex = FindTexture(TEXT("NWindow.WhiteTexture"));
}
NW_HOOK_CTOR(0x10005730, NCProgressCtrl)

// 0x10005870
NCProgressCtrl::NCProgressCtrl(const TCHAR* BackTexName, const TCHAR* BarTexName, INT InBorder)
{
    Unknown1A8 = 1;
    bStopped = 1;
    BarTex = NULL;
    BackTex = NULL;
    Border = 2;
    Pos = 0;
    Max = 0;
    BackTex = FindTexture(BackTexName ? BackTexName : TEXT("NWindow.BlackTexture"));
    BarTex = FindTexture(BarTexName ? BarTexName : TEXT("NWindow.WhiteTexture"));
    Border = InBorder;
}
NW_HOOK_CTOR_ARGS(0x10005870, NCProgressCtrl, (const TCHAR* A, const TCHAR* B, INT C), (A, B, C))

// 0x10005820
NW_HOOK_DTOR(0x10005820, NCProgressCtrl)

// stock body shared with NCButton's (0x100051f0)
INT NCProgressCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10005230 - `return 1` shared by 6 slots of several classes
NW_HOOK_METHOD(0x10005230, NCProgressCtrl, OnCreate)

// 0x10005960 (vslot 50) - clip first, origin after, as in the stock code
INT NCProgressCtrl::OnPaint(NCGDevice* Device)
{
    DWORD Elapsed = (INT)(GetDeltaTime() * 1000.f);
    INT BarW = Width - Border * 2;
    if (!bStopped)
    {
        Pos += Elapsed;
        if (Pos >= Max)
        {
            NotifyParent(0);
            Pos = Max;
            bStopped = 1;
        }
    }
    DWORD Done = Pos * BarW / Max;
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    if (BarTex)
        Device->DrawTexture(Border, Border, BarW - Done, Height - Border * 2, 0, 0, Width, Height,
                            BarTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10005960, NCProgressCtrl, OnPaint)

// 0x10005240
void NCProgressCtrl::SetRemaining(DWORD Remaining)
{
    Pos = Max - Remaining;
}
NW_HOOK_METHOD(0x10005240, NCProgressCtrl, SetRemaining)

// 0x10005260
void NCProgressCtrl::SetMax(DWORD InMax)
{
    Max = InMax;
}
NW_HOOK_METHOD(0x10005260, NCProgressCtrl, SetMax)

// 0x10005270
void NCProgressCtrl::Reset()
{
    Pos = 0;
    bStopped = 1;
}
NW_HOOK_METHOD(0x10005270, NCProgressCtrl, Reset)

// 0x10005290
void NCProgressCtrl::Stop()
{
    bStopped = 1;
}
NW_HOOK_METHOD(0x10005290, NCProgressCtrl, Stop)

// 0x100052a0
void NCProgressCtrl::Start()
{
    bStopped = 0;
}
NW_HOOK_METHOD(0x100052a0, NCProgressCtrl, Start)

NW_CHECK_VTABLE(NCProgressCtrl, 0x100d91c8, 63)
