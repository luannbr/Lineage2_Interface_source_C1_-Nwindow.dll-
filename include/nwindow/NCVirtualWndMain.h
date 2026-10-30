#pragma once
// NCAbstractMain / NCVirtualWndMain - owner of every window: registration, z-order lists,
// hit tests, focus, message dispatch, cursors (UClass "CAbstractMain", "CVirtualWndMain").
//
// Input path: the console calls DispatchWndMsg with the Win32 message; mouse coordinates are
// made screen-relative (ClientToScreenPacked), then the top-level windows are tried from the
// front (TopLevel is painted from index 0, so the last one is on top): clicks go to the deepest
// window hit (RouteClick, which also gives it the focus), mouse moves to the one under the
// cursor or to the focused one while the left button is held (RouteMouseMove), keyboard and IME
// messages to the focused window (SendToFocused). WM_PAINT paints each tree back to front and
// then lets the window under the cursor draw its tool tip.

#include "nwindow/NCWnd.h"

class UTexture;

class NCAbstractMain : public NCObject
{
    NW_DECLARE_ABSTRACT_CLASS(NCAbstractMain, NCObject, 0, 0x10124cd0)
    // constructor: NCObject's (0x10020ca0, identical code folded); ~NCAbstractMain 0x10024690

    virtual NWndEntry* RegisterNWindow(NCWnd* Wnd, DWORD Style, NWndEntry* Parent) = 0;  // 24
    virtual void UnRegisterNWindow(NWndEntry* Entry) = 0;                                 // 25
    virtual NWndEntry* FindNWindow(INT X, INT Y) = 0;                                     // 26
    virtual void ClearFocus() = 0;                                                        // 27
    virtual void AddTopLevel(NWndEntry* Entry) = 0;                                       // 28 (name ours)
};
NW_ASSERT_SIZE(NCAbstractMain, 0x28);

class NCVirtualWndMain : public NCAbstractMain
{
    NW_DECLARE_CLASS(NCVirtualWndMain, NCAbstractMain, 0, 0x101247b0)

    NCGDevice* GDevice;              // +0x28 created by the constructor
    DWORD NumWindows;                // +0x2C registered windows (RegisterNWindow refuses 1000+)
    TArray<NWndEntry*> Windows;      // +0x30 every registered window
    TArray<NWndEntry*> TopLevel;     // +0x3C windows without a parent, back to front
    TArray<NWndEntry*> TopMost;      // +0x48 windows registered with NWS_TOPMOST
    HCURSOR Cursors[7];              // +0x54 from this package's resources (LoadCursors)
    HCURSOR ArrowCursor;             // +0x70 IDC_ARROW
    INT CursorType;                  // +0x74 index in Cursors, 4 = color cursor, -1 = arrow
    UBOOL bCursorEnabled;            // +0x78
    HCURSOR ColorCursor;             // +0x7C built from a texture (CreateColorCursor)
    UBOOL bColorCursor;              // +0x80 ColorCursor is ours to destroy
    UBOOL bMouseMoveTaken;           // +0x84 a window already took this WM_MOUSEMOVE
    UBOOL bToolTipShown;             // +0x88 a window already drew its tool tip this frame

    NCVirtualWndMain();                                                    // 0x10025a80
    // ~NCVirtualWndMain                                                   // 0x10025bb0

    virtual NWndEntry* RegisterNWindow(NCWnd* Wnd, DWORD Style, NWndEntry* Parent); // 24 0x100257d0
    virtual void UnRegisterNWindow(NWndEntry* Entry);                      // 25 0x10025910
    virtual NWndEntry* FindNWindow(INT X, INT Y);                          // 26 0x100252a0
    virtual void ClearFocus();                                             // 27 0x10025180
    virtual void AddTopLevel(NWndEntry* Entry);                            // 28 0x100257a0 (name ours)
    virtual NWndEntry* FindChildWindowAt(NCWnd* Wnd, INT X, INT Y);        // 29 0x10025220 (name ours)

    // names ours unless marked
    UBOOL IsDispatchedMsg(UINT Msg);                                       // 0x10024290
    void SetGDevice(UCanvas* Canvas);                                      // 0x100242c0 (guard name)
    void ReleaseGDevice();                                                 // 0x10024340 (guard name)
    LPARAM ClientToScreenPacked(LPARAM Pos);                               // 0x100243c0
    INT SendToFocused(NWndEntry* Entry, UINT Msg, WPARAM wParam, LPARAM lParam); // 0x10024400
    void PaintChildren(NWndEntry* Entry);                                  // 0x10024570
    void LoadCursors();                                                    // 0x100245b0
    void DestroyCursors();                                                 // 0x10024630
    void DeleteGDevice();                                                  // 0x10024670
    UBOOL HitTestClick(NWndEntry* Entry, INT X, INT Y);                    // 0x10024710
    INT RouteClick(NWndEntry* Entry, INT X, INT Y, UINT Msg, WPARAM wParam, LPARAM lParam); // 0x100248a0
    UBOOL HitTest(NWndEntry* Entry, LPARAM Pos);                           // 0x10024980
    INT RouteMouseMove(NWndEntry* Entry, UINT Msg, WPARAM wParam, LPARAM lParam); // 0x10024a10
    void PaintToolTips(NWndEntry* Entry);                                  // 0x10024b00
    INT DispatchWndMsg(UINT Msg, WPARAM wParam, LPARAM lParam);            // 0x10024bf0 (guard name)
    void CreateColorCursor(UTexture* Texture);                             // 0x10025380
    void SetCursorType(INT Type, UTexture* Texture);                       // 0x100256b0
    UBOOL IsOverWindow(INT X, INT Y);                                      // 0x10025720
};
NW_ASSERT_OFFSET(NCVirtualWndMain, Windows, 0x30);
NW_ASSERT_OFFSET(NCVirtualWndMain, TopLevel, 0x3C);
NW_ASSERT_OFFSET(NCVirtualWndMain, TopMost, 0x48);
NW_ASSERT_OFFSET(NCVirtualWndMain, Cursors, 0x54);
NW_ASSERT_OFFSET(NCVirtualWndMain, ArrowCursor, 0x70);
NW_ASSERT_OFFSET(NCVirtualWndMain, ColorCursor, 0x7C);
NW_ASSERT_OFFSET(NCVirtualWndMain, bToolTipShown, 0x88);
NW_ASSERT_SIZE(NCVirtualWndMain, 0x8C);
