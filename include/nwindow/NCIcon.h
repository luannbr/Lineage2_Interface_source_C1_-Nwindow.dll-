#pragma once
// NCIcon - the 32x32 icon a minimized window leaves on the screen (UClass "CIcon", 0x2C0).
// NCWnd::OnCreate creates one for NWS_MINIMIZABLE windows; NCWnd::Minimize picks its texture
// and tool tip. It can be dragged (WM_MOVE to itself, sticky like windows); a click without a
// drag restores the window.

#include "nwindow/NControl.h"

class NCIcon : public NControl
{
    NW_DECLARE_CLASS(NCIcon, NControl, 0, 0x10120988)

    UTexture* IconTex;         // +0x1A0
    INT DragX;                 // +0x1A4 where the drag grabbed the icon (NCWnd::Snap)
    INT DragY;                 // +0x1A8
    TCHAR Name[128];           // +0x1AC tool tip (a SysString, SetIcon)
    BYTE bMouseOver;           // +0x2AC
    INT Unknown2B0;            // +0x2B0 (-1)
    UBOOL bToolTipAbove;       // +0x2B4 (SetIcon)
    UTexture* ToolTipTex;      // +0x2B8 "NWindow.ChatBack"
    UBOOL bDragged;            // +0x2BC moved since the button went down

    NCIcon();                                                        // 0x10015720
    // ~NCIcon                                                       // 0x100157f0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100154f0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10015840
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 51 0x10015910
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x100155d0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x10015680
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x100156c0
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam) { return 1; } // 63 0x100098f0
                                                                     //   (folded; name ours)

    void SetIcon(const TCHAR* TexName, INT NameID, BYTE bAbove);     // 0x10015560 (name ours)
};
NW_ASSERT_OFFSET(NCIcon, Name, 0x1AC);
NW_ASSERT_OFFSET(NCIcon, bMouseOver, 0x2AC);
NW_ASSERT_OFFSET(NCIcon, bToolTipAbove, 0x2B4);
NW_ASSERT_OFFSET(NCIcon, bDragged, 0x2BC);
NW_ASSERT_SIZE(NCIcon, 0x2C0);
