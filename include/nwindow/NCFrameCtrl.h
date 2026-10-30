#pragma once
// NCFrameCtrl - title bar of a window (UClass "CFrameCtrl", 0x3D4) and NCResizeFrame, the
// edge that resizes a window (UClass "CResizeFrame", 0x3E8).
//
// Dragging the bar sends WM_MOVE(delta) to the parent, which moves (and snaps). A decorated
// frame paints the three title bar textures and the title, and adds a minimize button
// (parent NWS_MINIMIZABLE) and a close button (unless NWS_NOCLOSE); NCWnd::OnCreate makes
// undecorated ones: an invisible 15 px drag area above windows with NWS_TITLEBAR.

#include "nwindow/NControl.h"

class NCButton;

// NWM_COMMAND ids of the frame buttons
enum { NID_FRAME_MINIMIZE = 0xF0F0, NID_FRAME_CLOSE = 0xF0F1 };

class NCFrameCtrl : public NControl
{
    NW_DECLARE_CLASS(NCFrameCtrl, NControl, 0, 0x1011ba78)

    TCHAR Title[256];          // +0x1A0
    UTexture* BlackTex;        // +0x3A0 "NWindow.BlackTexture" (not drawn)
    UTexture* TitleLeftTex;    // +0x3A4 "L2UI.NWindow.WindowTitlebar1", 24x21
    UTexture* TitleMidTex;     // +0x3A8 "L2UI.NWindow.WindowTitlebar2", stretched
    UTexture* TitleRightTex;   // +0x3AC "L2UI.NWindow.WindowTitlebar3", 8x21
    NCButton* CloseButton;     // +0x3B0
    NCButton* MinimizeButton;  // +0x3B4
    INT LastMouseX;            // +0x3B8 mouse position of the drag
    INT LastMouseY;            // +0x3BC
    INT TitleX;                // +0x3C0 (28)
    INT TitleY;                // +0x3C4 title centered vertically
    INT GrabX;                 // +0x3C8 where the drag grabbed the bar (NCWnd::Snap)
    INT GrabY;                 // +0x3CC
    UBOOL bDecorated;          // +0x3D0 textures, title and buttons

    NCFrameCtrl();                                                   // 0x1000beb0
    NCFrameCtrl(UBOOL bInDecorated);                                 // 0x1000bfa0
    // ~NCFrameCtrl                                                  // 0x1000be30

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1000bb20
    virtual INT OnCreate();                                          // 35 0x1000c180
    virtual INT OnDestroy();                                         // 36 0x1000c320
    virtual void SetSize(INT W, INT H);                              // 41 0x1000bbd0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1000c390
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x1000bbe0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x1000bc50
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x1000bcb0
    virtual INT OnClose();                                           // 63 0x1000c090 (name ours)
    virtual INT OnMinimize();                                        // 64 0x1000bbb0 (name ours)

    void SetTitleText(const TCHAR* InTitle);                         // 0x1000bcd0 (name ours)
};
NW_ASSERT_OFFSET(NCFrameCtrl, Title, 0x1A0);
NW_ASSERT_OFFSET(NCFrameCtrl, BlackTex, 0x3A0);
NW_ASSERT_OFFSET(NCFrameCtrl, CloseButton, 0x3B0);
NW_ASSERT_OFFSET(NCFrameCtrl, GrabX, 0x3C8);
NW_ASSERT_OFFSET(NCFrameCtrl, bDecorated, 0x3D0);
NW_ASSERT_SIZE(NCFrameCtrl, 0x3D4);

// resize direction: which mouse delta goes to the parent (NWM_COMMAND, lParam = delta)
enum { RESIZE_Horizontal = 1, RESIZE_Vertical = 2 };

class NCResizeFrame : public NCFrameCtrl
{
    NW_DECLARE_CLASS(NCResizeFrame, NCFrameCtrl, 0, 0x1011bf98)

    UBOOL bPressed;            // +0x3D4 cleared with bDragging, never read here
    INT Direction;             // +0x3D8 RESIZE_*
    INT Unknown3DC;            // +0x3DC
    UBOOL bDragging;           // +0x3E0 button went down inside
    UBOOL bCursorLocked;       // +0x3E4 resize cursor shown and locked

    NCResizeFrame(INT InDirection = 0);                              // 0x1000c4f0
    // ~NCResizeFrame                                                // 0x1000c560

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1000bd40
    virtual void KillFocus();                                        // 29 0x1000be10
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x1000c5b0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x1000bd80
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x1000bdf0
};
NW_ASSERT_OFFSET(NCResizeFrame, Direction, 0x3D8);
NW_ASSERT_OFFSET(NCResizeFrame, bCursorLocked, 0x3E4);
NW_ASSERT_SIZE(NCResizeFrame, 0x3E8);
