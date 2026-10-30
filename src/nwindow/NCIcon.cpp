/*=============================================================================
    NCIcon.cpp - icon of a minimized window. Stock: 0x100154f0 .. 0x10015b6e.
=============================================================================*/
#include "nwindow/NCIcon.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

enum { SOUND_Click = 0 };

// 0x10015720 - Name is left as allocated (zero)
NCIcon::NCIcon()
{
    DragY = 0;
    DragX = 0;
    IconTex = NULL;
    bMouseOver = 0;
    Unknown2B0 = -1;
    bToolTipAbove = 0;
    ToolTipTex = FindTexture(TEXT("NWindow.ChatBack"));
    bDragged = 0;
}
NW_HOOK_CTOR(0x10015720, NCIcon)

// 0x100157f0
NW_HOOK_DTOR(0x100157f0, NCIcon)

// 0x100154f0 (vslot 24)
INT NCIcon::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100154f0, NCIcon, WindowProc)

// 0x100098f0 (vslot 63) - `return 1` body shared with 11 slots of other windows
NW_HOOK_METHOD(0x100098f0, NCIcon, OnLButtonDblClick)

// 0x10015840 (vslot 50)
INT NCIcon::OnPaint(NCGDevice* Device)
{
    guard(NCIcon::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (IconTex)
        Device->DrawTexture(0, 0, 32, 32, 0, 0, 32, 32, IconTex);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10015840, NCIcon, OnPaint)

// 0x10015910 (vslot 51) - same box as the buttons' (NCGDevice::DrawToolTipBox)
INT NCIcon::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (Name[0] && bMouseOver)
        Device->DrawToolTipBox(this, Name, ToolTipTex, bToolTipAbove);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10015910, NCIcon, OnDrawToolTip)

// 0x100155d0 (vslot 55) - dragged: WM_MOVE(delta) to itself (NCWnd::OnMove snaps it);
// DragMouse holds the delta during the call
INT NCIcon::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = (SWORD)LOWORD(lParam);
    INT Y = (SWORD)HIWORD(lParam);
    if (wParam & MK_LBUTTON)
    {
        DragMouseX = X - DragMouseX;
        DragMouseY = Y - DragMouseY;
        SendTo(Entry, WM_MOVE, 0, MAKELPARAM(DragMouseX, DragMouseY));
        DragMouseX = X;
        DragMouseY = Y;
        bDragged = 1;
        return 1;
    }
    bMouseOver = PtInRect(X, Y, ScreenX, ScreenY, Width, Height) != 0;
    return 1;
}
NW_HOOK_METHOD(0x100155d0, NCIcon, OnMouseMove)

// 0x10015680 (vslot 56)
INT NCIcon::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    LockCursor();
    DragMouseX = (SWORD)LOWORD(lParam);
    DragX = DragMouseX - ScreenX;
    DragMouseY = (SWORD)HIWORD(lParam);
    DragY = DragMouseY - ScreenY;
    return 1;
}
NW_HOOK_METHOD(0x10015680, NCIcon, OnLButtonDown)

// 0x100156c0 (vslot 57) - a click without a drag restores the window
INT NCIcon::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (bDragged)
    {
        UnlockCursor();
        ReleaseFocus();
        bDragged = 0;
        return 1;
    }
    GConsole->Engine->OnInterfacePlaySound(SOUND_Click);
    Entry->Parent->Wnd->Restore();
    return 1;
}
NW_HOOK_METHOD(0x100156c0, NCIcon, OnLButtonUp)

// 0x10015560 - Core's appStrcpy here (the other copies of NWindow call the CRT's wcscpy)
void NCIcon::SetIcon(const TCHAR* TexName, INT NameID, BYTE bAbove)
{
    if (TexName)
        IconTex = FindTexture(TexName);
    if (NameID >= 0)
        appStrcpy(Name, *GL2GameData.SysString[NameID]);
    bToolTipAbove = bAbove;
}
NW_HOOK_METHOD(0x10015560, NCIcon, SetIcon)

NW_CHECK_VTABLE(NCIcon, 0x100dc698, 64)
