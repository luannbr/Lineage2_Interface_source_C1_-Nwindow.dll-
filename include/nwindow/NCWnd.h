#pragma once
// NCCmdTarget / NCWnd - base of every window and control (UClass "CCmdTarget", "CWnd").
//
// A window lives in a tree: NCVirtualWndMain::RegisterNWindow gives it an NWndEntry, its
// parent keeps the entries of its children in a linked list (NWndListNode), messages arrive
// through WindowProc (vslot 24) as Win32 message numbers. Coordinates: Local* are relative
// to the parent, Screen* absolute; SetWindowRect keeps both.

#include "nwindow/NCObject.h"

class NCWnd;
class NCVirtualWndMain;
class NCFrameCtrl;
class NCIcon;
class NCGDevice;

// Registration record of a window (NCVirtualWndMain::RegisterNWindow, 16 bytes) (name ours).
struct NWndEntry
{
    INT bValid;              // +0x00 1 while registered
    NCWnd* Wnd;              // +0x04
    NCVirtualWndMain* Main;  // +0x08 the manager that registered it
    NWndEntry* Parent;       // +0x0C entry of the parent window, NULL for top-level windows

    NWndEntry() : bValid(0), Wnd(NULL), Main(NULL), Parent(NULL) {}
};
NW_ASSERT_SIZE(NWndEntry, 0x10);

// Node of NCWnd's child list, allocated with new (12 bytes) (name ours).
struct NWndListNode
{
    NWndEntry* Entry;        // +0x00
    NWndListNode* Prev;      // +0x04
    NWndListNode* Next;      // +0x08

    NWndListNode() : Entry(NULL), Prev(NULL), Next(NULL) {}
};
NW_ASSERT_SIZE(NWndListNode, 0xC);

// NCWnd::Style bits (names ours)
enum
{
    NWS_VISIBLE = 0x0002,
    NWS_NOHITTEST = 0x0008,        // skipped by NCVirtualWndMain hit tests
    NWS_NOTINPARENTLIST = 0x0040,  // RemoveChildWindow leaves the parent's list alone
    NWS_TITLEBAR = 0x0080,         // OnCreate adds an NCFrameCtrl title bar
    NWS_TOPMOST = 0x1000,          // NCVirtualWndMain keeps these in a separate array
    NWS_MINIMIZABLE = 0x2000,      // OnCreate adds an NCIcon shown while minimized; a
                                   //   decorated NCFrameCtrl adds a minimize button
    NWS_NOCLOSE = 0x4000,          // a decorated NCFrameCtrl leaves out its close button
};

// Sent by NCVirtualWndMain after each paint to the window under the cursor, wParam = the
// NCGDevice, handled by vslot 51 (tool tips) (name ours).
enum { NWM_COMMAND = 0x501, NWM_DRAWTOOLTIP = 0x505 };

// NCWnd statics (names ours)
#if NW_HOOK_MODE
#define GFocusEntry  NW_GLOBAL(NWndEntry*, 0x10123d54)  // entry of the window with the keyboard focus
#define GShiftDown   NW_GLOBAL(INT, 0x10123d50)         // Shift held (set by OnKeyDown/OnKeyUp)
#define GNextFrameID NW_GLOBAL(INT, 0x1010a998)         // next id for a title bar (NCWnd::OnCreate)
#else
extern NWndEntry* GFocusEntry;
extern INT GShiftDown;
extern INT GNextFrameID;
#endif

class NCCmdTarget : public NCObject
{
    NW_DECLARE_ABSTRACT_CLASS(NCCmdTarget, NCObject, 0, 0x10119670)
    // ctor: NCObject's (0x10020ca0, identical code folded); ~NCCmdTarget 0x10001690
};
NW_ASSERT_SIZE(NCCmdTarget, 0x28);

class NCWnd : public NCCmdTarget
{
    NW_DECLARE_CLASS(NCWnd, NCCmdTarget, 0, 0x10123d60)

    NWndListNode* ChildHead;   // +0x28
    NWndListNode* ChildTail;   // +0x2C
    INT LocalX;                // +0x30 relative to the parent
    INT LocalY;                // +0x34
    INT LocalW;                // +0x38
    INT LocalH;                // +0x3C
    UBOOL bFocus;              // +0x40
    INT ScreenX;               // +0x44 absolute
    INT ScreenY;               // +0x48
    INT Width;                 // +0x4C
    INT Height;                // +0x50
    DWORD Style;               // +0x54 NWS_*
    NWndEntry* Entry;          // +0x58 own registration
    UBOOL bEnabled;            // +0x5C
    NCFrameCtrl* FrameCtrl;    // +0x60 title bar (NWS_TITLEBAR)
    INT FrameID;               // +0x64 id given to FrameCtrl (GNextFrameID++)
    INT DragMouseX;            // +0x68 last mouse position of a drag (NCIcon)
    INT DragMouseY;            // +0x6C
    NWndEntry* NextTab;        // +0x70 TAB goes there
    NWndEntry* PrevTab;        // +0x74 Shift+TAB goes there
    INT Unknown78;             // +0x78
    NWndEntry* StuckTopTo;     // +0x7C Snap (sticky windows): window each edge is stuck
    NWndEntry* StuckLeftTo;    // +0x80   to, NULL = screen edge; meaningful only while
    NWndEntry* StuckRightTo;   // +0x84   the matching bStuck* is set. Explicit placement
    NWndEntry* StuckBottomTo;  // +0x88   (SetWindowRect, SetSize...) clears everything.
    BYTE bStuckTop;            // +0x8C
    BYTE bStuckLeft;           // +0x8D
    BYTE bStuckRight;          // +0x8E
    BYTE bStuckBottom;         // +0x8F
    NCIcon* Icon;              // +0x90 shown instead of the window while minimized

    NCWnd();                                                     // 0x10023160
    // ~NCWnd                                                    // 0x10001750

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam); // 24 0x10022b60
    virtual UBOOL IsEnabled() { return bEnabled; }               // 25 0x100017a0
    virtual UBOOL IsInWindow(INT X, INT Y);                      // 26 0x10022f60 (name ours)
    virtual UBOOL IsFocused() { return bFocus; }                 // 27 0x100017b0 (name ours)
    virtual void SetFocus();                                     // 28 0x10022e70
    virtual void KillFocus();                                    // 29 0x10022ec0
    virtual void SetStyle(DWORD NewStyle) { Style = NewStyle; }  // 30 0x100017c0
    virtual DWORD GetStyle() { return Style; }                   // 31 0x100017d0
    virtual INT vfn032() { return 0; }                           // 32 0x100016e0 (folded)
    virtual INT vfn033() { return 0; }                           // 33 0x100016e0 (folded)
    virtual INT OnTimer(INT TimerID) { return 0; }               // 34 0x10001710 (folded) WM_TIMER
    virtual INT OnCreate();                                      // 35 0x10023e50
    virtual INT OnDestroy();                                     // 36 0x10024020
    virtual void SetTabOrder(NWndEntry* Next, NWndEntry* Prev);  // 37 0x10022d30 (name ours)
    virtual void EnableWindow(UBOOL bEnable);                    // 38 0x10022d50 (name ours)
    virtual UBOOL IsShowWindow();                                // 39 0x10022df0 (name ours)
    virtual UBOOL PtInRect(INT X, INT Y, INT RectX, INT RectY, INT RectW, INT RectH); // 40 0x10022f20
    virtual void SetSize(INT W, INT H);                          // 41 0x10023a40 (name ours)
    virtual void ShowWindow();                                   // 42 0x10022d80
    virtual void HideWindow();                                   // 43 0x10022dc0
    virtual void MoveWindow(INT DX, INT DY);                     // 44 0x10023d30 (name ours) snaps
    virtual void OffsetWindow(INT DX, INT DY);                   // 45 0x10039080 (name ours)
    virtual void Restore();                                      // 46 0x10023fd0 (name ours)
    virtual void Minimize();                                     // 47 0x10023b10 (name ours)
    virtual void CenterInParent();                               // 48 0x10023ab0 (name ours)
    virtual void SetTitle(const TCHAR* Title);                   // 49 0x100230a0 (name ours)
    virtual INT OnPaint(NCGDevice* Device) { return 0; }         // 50 0x10001710 (folded) WM_PAINT
    virtual INT OnDrawToolTip(NCGDevice* Device) { return 0; }   // 51 0x10001710 (folded) (name ours)
    virtual INT OnChar(WPARAM wParam, LPARAM lParam) { return 0; }          // 52 0x100016f0 (folded)
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);                    // 53 0x10022ff0
    virtual INT OnKeyUp(WPARAM wParam, LPARAM lParam);                      // 54 0x10023070
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam) { return 0; }     // 55 0x100016f0 (folded)
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam) { return 0; }   // 56 0x100016f0 (folded)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam) { return 0; }     // 57 0x100016f0 (folded)
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam) { return 0; }   // 58 0x100016f0 (folded)
    virtual INT OnRButtonUp(WPARAM wParam, LPARAM lParam) { return 0; }     // 59 0x100016f0 (folded)
    virtual INT OnMove(WPARAM wParam, LPARAM lParam);                       // 60 0x10022fc0 WM_MOVE
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam) { return 0; }    // 61 0x100016f0 (folded)

    // non-virtual
    INT DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);   // 0x10004270 (name ours)
    void BringChildToTop(NWndEntry* Child);                      // 0x10022e00 (name ours)
    void OffsetScreen(INT DX, INT DY);                           // 0x10022ee0 (name ours)
    NWndEntry* FindWindowAt(INT X, INT Y);                       // 0x10022fb0 (name ours)
    void FormatNumber(TCHAR* Out, const TCHAR* Digits);          // 0x100230d0 (name ours)
    UBOOL AddChildNode(NWndEntry* Child);                        // 0x100231d0 (name ours)
    void RemoveChildNode(NWndEntry* Child);                      // 0x10023230 (name ours)
    void Snap(POINT* Delta);                                     // 0x100232a0 (name ours) sticky windows
    void SetScreenPos(INT X, INT Y, const POINT* Origin);        // 0x100239d0 (name ours)
    NWndEntry* AddChildWindow(NCWnd* Child, DWORD ChildStyle);   // 0x10023ca0 (name ours)
    void RemoveChildWindow(NWndEntry* Child);                    // 0x10023ce0 (name ours)
    void SetWindowRect(INT X, INT Y, INT W, INT H);              // 0x10023d90 (name ours)

    // The second half of CreateChild on the child (name ours): an out-of-line copy of the
    // inline code for the call sites where the stock compiler stopped inlining it.
    void AttachEntry(NWndEntry* E);                              // 0x10002a80

    // Registers a newly created child the way every OnCreate does (inlined in the stock
    // code, name ours): entry under this window, screen position made absolute, WM_CREATE.
    // Forced inline: inside a guard (/EHa) our compiler kept it out of line.
    FORCEINLINE NWndEntry* CreateChild(NCWnd* Child, DWORD ChildStyle)
    {
        NWndEntry* E = AddChildWindow(Child, ChildStyle);
        Child->Entry = E;
        if (E->Parent)
        {
            NCWnd* Parent = E->Parent->Wnd;
            Child->ScreenX += Parent->ScreenX;
            Child->ScreenY += Parent->ScreenY;
        }
        Child->WindowProc(WM_CREATE, 0, 0);
        return E;
    }

    void ResetSticky()
    {
        StuckBottomTo = NULL;
        StuckTopTo = NULL;
        StuckLeftTo = NULL;
        StuckRightTo = NULL;
        bStuckBottom = 0;
        bStuckLeft = 0;
        bStuckRight = 0;
        bStuckTop = 0;
    }
};
NW_ASSERT_OFFSET(NCWnd, ChildHead, 0x28);
NW_ASSERT_OFFSET(NCWnd, LocalX, 0x30);
NW_ASSERT_OFFSET(NCWnd, bFocus, 0x40);
NW_ASSERT_OFFSET(NCWnd, ScreenX, 0x44);
NW_ASSERT_OFFSET(NCWnd, Style, 0x54);
NW_ASSERT_OFFSET(NCWnd, Entry, 0x58);
NW_ASSERT_OFFSET(NCWnd, FrameCtrl, 0x60);
NW_ASSERT_OFFSET(NCWnd, NextTab, 0x70);
NW_ASSERT_OFFSET(NCWnd, StuckTopTo, 0x7C);
NW_ASSERT_OFFSET(NCWnd, bStuckTop, 0x8C);
NW_ASSERT_OFFSET(NCWnd, Icon, 0x90);
NW_ASSERT_SIZE(NCWnd, 0x94);
