/*=============================================================================
    NCCompositeBox.cpp - IME composition window. Stock: 0x100029e0 .. 0x10002a7a,
    0x10003380 .. 0x100036b8; OnDestroy body shared (0x10083930).
=============================================================================*/
#include "nwindow/NCCompositeBox.h"
#include "nwindow/NCGDevice.h"

// 0x10003380
NCCompositeBox::NCCompositeBox()
    : BackTex(NULL)
{
    CaretPos = 0;
    bShowCaret = 0;
    FirstChar = 0;
}
NW_HOOK_CTOR(0x10003380, NCCompositeBox)

// 0x10003490
NCCompositeBox::NCCompositeBox(INT W, INT H, INT X, INT Y, UBOOL bInShowCaret)
{
    bShowCaret = bInShowCaret;
    FirstChar = 0;
    CaretPos = 0;
    SetWindowRect(X, Y, W, H);
}
NW_HOOK_CTOR_ARGS(0x10003490, NCCompositeBox, (INT A, INT B, INT C, INT D, UBOOL E), (A, B, C, D, E))

// 0x10003420
NW_HOOK_DTOR(0x10003420, NCCompositeBox)

// stock body shared with NControl's (0x10004e90)
INT NCCompositeBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100029e0 (vslot 35)
INT NCCompositeBox::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("NWindow.BlackTexture"));
    Text.Empty();
    return 1;
}
NW_HOOK_METHOD(0x100029e0, NCCompositeBox, OnCreate)

// 0x10083930 (vslot 36) - one body for every class whose OnDestroy only calls NCWnd's
// (NCAutoLogWnd, the status windows...)
INT NCCompositeBox::OnDestroy()
{
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10083930, NCCompositeBox, OnDestroy)

// 0x10003520 (vslot 50) - the width drawn is unsigned in the stock code
INT NCCompositeBox::OnPaint(NCGDevice* Device)
{
    guard(NCCompositeBox::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawStretchedTex(0, 0, Width, Height, BackTex);
    DWORD X = 0;
    INT i;
    for (i = FirstChar; i < Text.Len(); i++)
    {
        if (i == CaretPos && bShowCaret)
            Device->DrawText(X + 1, 3, 0xFFFFFFFF, TEXT("|"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                             0);
        X += Device->DrawText(X + 3, 3, 0xFFFFFFFF, *Text.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                              0, 0, 0);
        if (X > 97)
            break;
    }
    if (i == CaretPos && bShowCaret)
        Device->DrawText(X + 1, 3, 0xFFFFFFFF, TEXT("|"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10003520, NCCompositeBox, OnPaint)

// 0x10002a10
void NCCompositeBox::CaretLeft()
{
    if (CaretPos > 0)
        CaretPos--;
    if (FirstChar > CaretPos)
        FirstChar = CaretPos;
}
NW_HOOK_METHOD(0x10002a10, NCCompositeBox, CaretLeft)

// 0x100036f0 - measures with a device created (and destroyed) just for the call, as in the
// stock code
void NCCompositeBox::CaretRight()
{
    if (Text.Len() > CaretPos)
        CaretPos++;
    NCGDevice* Device = new NCGDevice;
    SIZE Size;
    Device->GetTextExtent(&Size, *Text.Mid(FirstChar, CaretPos - FirstChar), 0, 0, 0, 0, 0);
    while (Size.cx > 97)
    {
        FirstChar++;
        Device->GetTextExtent(&Size, *Text.Mid(FirstChar, CaretPos - FirstChar), 0, 0, 0, 0, 0);
    }
    delete Device;
}
NW_HOOK_METHOD(0x100036f0, NCCompositeBox, CaretRight)

// 0x10003860 - scrolls back to the start, then forward until the caret fits
void NCCompositeBox::SetText(const TCHAR* InText)
{
    if (!InText)
        return;
    FirstChar = 0;
    Text = FString(InText);
    NCGDevice* Device = new NCGDevice;
    SIZE Size;
    Device->GetTextExtent(&Size, *Text.Mid(FirstChar, CaretPos - FirstChar), 0, 0, 0, 0, 0);
    while (Size.cx > 97)
    {
        FirstChar++;
        Device->GetTextExtent(&Size, *Text.Mid(FirstChar, CaretPos - FirstChar), 0, 0, 0, 0, 0);
    }
    delete Device;
}
NW_HOOK_METHOD(0x10003860, NCCompositeBox, SetText)

// 0x10002a40
FString NCCompositeBox::GetText()
{
    return Text;
}
NW_HOOK_METHOD(0x10002a40, NCCompositeBox, GetText)

// 0x10002a70
void NCCompositeBox::SetShowCaret(UBOOL bShow)
{
    bShowCaret = bShow;
}
NW_HOOK_METHOD(0x10002a70, NCCompositeBox, SetShowCaret)

NW_CHECK_VTABLE(NCCompositeBox, 0x100d87a0, 62)
