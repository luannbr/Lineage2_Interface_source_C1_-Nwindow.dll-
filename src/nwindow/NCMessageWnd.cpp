/*=============================================================================
    NCMessageWnd.cpp - the console's one-line message window. Stock: 0x10077fa0 .. 0x100781d0.
=============================================================================*/
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCGDevice.h"

#include <wchar.h>

// 0x10078000
NCMessageWnd::NCMessageWnd()
{
    BackTex = NULL;
    Text[0] = 0;
}
NW_HOOK_CTOR(0x10078000, NCMessageWnd)

// 0x10078060
NW_HOOK_DTOR(0x10078060, NCMessageWnd)

// 0x10004e90 (vslot 24) - stock body shared with NControl (hooked there)
INT NCMessageWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10077fa0 (vslot 35)
INT NCMessageWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("sek.cbui141"));
    return 1;
}
NW_HOOK_METHOD(0x10077fa0, NCMessageWnd, OnCreate)

// 0x10083930 (vslot 36) - stock body shared with NCCompositeBox (hooked there)
INT NCMessageWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    return 1;
}

// 0x10077fd0 - also used on other windows with a text at +0x98 (folded)
void NCMessageWnd::ClearText()
{
    Text[0] = 0;
}
NW_HOOK_METHOD(0x10077fd0, NCMessageWnd, ClearText)

// 0x10077fe0
void NCMessageWnd::SetText(const TCHAR* NewText)
{
    if (NewText)
        wcscpy(Text, NewText);
}
NW_HOOK_METHOD(0x10077fe0, NCMessageWnd, SetText)

// 0x100780b0 (vslot 50) - the clip opens before BeginPaint and closes after EndPaint; the
// halves of the window are unsigned, those of the text signed, as in the stock code
INT NCMessageWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    SIZE Size;
    const TCHAR* S = Text;
    NCGDevice::GetTextExtent(&Size, S, 0, 0, 0, 0, 0);
    if (S && *S && Size.cx < Width)
        Device->DrawText(((DWORD)Width >> 1) - Size.cx / 2, ((DWORD)Height >> 1) - Size.cy / 2,
                         0xFFFFFFFF, S, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    else if (S && *S)
        Device->DrawText(50, 8, 0xFFFFFFFF, S, GSEKScreenX - 100, 0, 0, L2FONT_Normal, 0, 0, 0,
                         0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100780b0, NCMessageWnd, OnPaint)

NW_CHECK_VTABLE(NCMessageWnd, 0x100e80a0, 62)
