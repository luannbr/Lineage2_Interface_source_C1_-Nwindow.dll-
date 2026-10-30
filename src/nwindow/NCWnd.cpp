/*=============================================================================
    NCWnd.cpp - base window: tree, geometry, focus, visibility, message routing.
    Stock: 0x10022b60 .. 0x10024080, plus the defaults folded at 0x100016e0.. and
    DefWindowProc (0x10004270, an inline emitted with NCComboBox).
=============================================================================*/
#include "nwindow/NCVirtualWndMain.h"
#include "nwindow/NControl.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NClasses.gen.h"
#if NW_FEATURES
#include "features/WindowFix.h"   // phase 6: WindowFix
#endif

#include <wchar.h>

/*-----------------------------------------------------------------------------
    NCCmdTarget
-----------------------------------------------------------------------------*/

// 0x10001690 - NW_DECLARE_CLASS destructor
NW_HOOK_DTOR(0x10001690, NCCmdTarget)

/*-----------------------------------------------------------------------------
    Construction
-----------------------------------------------------------------------------*/

// 0x10023160
NCWnd::NCWnd()
{
    ChildHead = NULL;
    ChildTail = NULL;
    LocalY = 0;
    LocalX = 0;
    LocalW = 0;
    LocalH = 0;
    bFocus = 0;
    ScreenY = 0;
    ScreenX = 0;
    Width = 0;
    Height = 0;
    Style = 0;
    Entry = NULL;
    bEnabled = 1;
    DragMouseY = 0;
    DragMouseX = 0;
    NextTab = NULL;
    PrevTab = NULL;
    bStuckTop = 0;
    bStuckBottom = 0;
    bStuckLeft = 0;
    bStuckRight = 0;
}
NW_HOOK_CTOR(0x10023160, NCWnd)

// 0x10001750 - NW_DECLARE_CLASS destructor
NW_HOOK_DTOR(0x10001750, NCWnd)

// Accessors and the defaults the linker folded (one stock body serves several slots/classes).
NW_HOOK_METHOD(0x100017a0, NCWnd, IsEnabled)
NW_HOOK_METHOD(0x100017b0, NCWnd, IsFocused)
NW_HOOK_METHOD(0x100017c0, NCWnd, SetStyle)
NW_HOOK_METHOD(0x100017d0, NCWnd, GetStyle)
NW_HOOK_METHOD(0x100016e0, NCWnd, vfn032)   // xor eax,eax; ret    - also vslot 33, NControl vslot 62
NW_HOOK_METHOD(0x100016f0, NCWnd, OnChar)   // xor eax,eax; ret 8  - every default mouse/key handler
NW_HOOK_METHOD(0x10001710, NCWnd, OnTimer)  // xor eax,eax; ret 4  - also OnPaint, OnDrawToolTip

// 0x10023e50 - creates the title bar (NWS_TITLEBAR) and the minimized icon (NWS_MINIMIZABLE)
INT NCWnd::OnCreate()
{
    INT W = LocalW;
    FrameCtrl = NULL;
    Icon = NULL;
    if (Style & NWS_TITLEBAR)
    {
        FrameCtrl = new NCFrameCtrl(0);
        FrameID = GNextFrameID++;
        FrameCtrl->ID = FrameID;
        FrameCtrl->SetWindowRect(0, -15, W, 15);
        CreateChild(FrameCtrl, NWS_VISIBLE);
    }
    if (Style & NWS_MINIMIZABLE)
    {
        Icon = new NCIcon;
        Icon->ID = 15;
        Icon->SetWindowRect(0, 0, 32, 32);
        CreateChild(Icon, NWS_TOPMOST | NWS_VISIBLE);
        Icon->HideWindow();
    }
    return 1;
}
NW_HOOK_METHOD(0x10023e50, NCWnd, OnCreate)

// 0x10024020
INT NCWnd::OnDestroy()
{
    if (FrameCtrl)
    {
        RemoveChildWindow(FrameCtrl->Entry);
        delete FrameCtrl;
        FrameCtrl = NULL;
    }
    if (Icon)
    {
        RemoveChildWindow(Icon->Entry);
        delete Icon;
        Icon = NULL;
    }
    return 1;
}
NW_HOOK_METHOD(0x10024020, NCWnd, OnDestroy)

/*-----------------------------------------------------------------------------
    Messages
-----------------------------------------------------------------------------*/

// 0x10004270 - WM_CREATE / WM_DESTROY / WM_TIMER
INT NCWnd::DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (Msg == WM_CREATE)
        return OnCreate() != 0;
    if (Msg == WM_DESTROY)
        return OnDestroy() != 0;
    if (Msg == WM_TIMER)
        return OnTimer((INT)wParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10004270, NCWnd, DefWindowProc)

// 0x10022b60 (vslot 24) - WM_PAINT carries the NCGDevice in wParam
INT NCWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (DefWindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_PAINT)
    {
        if (Style & NWS_VISIBLE)
            OnPaint((NCGDevice*)wParam);
        return 1;
    }
    if (Msg == WM_CHAR)
        return OnChar(wParam, lParam) != 0;
    if (Msg == WM_KEYDOWN)
        return OnKeyDown(wParam, lParam) != 0;
    if (Msg == WM_KEYUP)
        return OnKeyUp(wParam, lParam) != 0;
    if (Msg == WM_MOUSEMOVE)
        return OnMouseMove(wParam, lParam) != 0;
    if (Msg == WM_LBUTTONDOWN)
        return OnLButtonDown(wParam, lParam) != 0;
    if (Msg == WM_RBUTTONDOWN)
        return OnRButtonDown(wParam, lParam) != 0;
    if (Msg == WM_LBUTTONUP)
        return OnLButtonUp(wParam, lParam) != 0;
    if (Msg == WM_RBUTTONUP)
        return OnRButtonUp(wParam, lParam) != 0;
    if (Msg == WM_MOVE)
        return OnMove(wParam, lParam) != 0;
    if (Msg == WM_MOUSEWHEEL)
        return OnMouseWheel(wParam, lParam) != 0;
    if (Msg == NWM_DRAWTOOLTIP)
    {
        if (Style & NWS_VISIBLE)
            OnDrawToolTip((NCGDevice*)wParam);
        return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x10022b60, NCWnd, WindowProc)

// 0x10022ff0 (vslot 53) - TAB / Shift+TAB move the focus along NextTab / PrevTab
INT NCWnd::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_TAB)
    {
        NWndEntry* Target;
        if (GShiftDown)
        {
            if (!PrevTab)
                return vfn032() != 0;
            KillFocus();
            Target = PrevTab;
        }
        else
        {
            if (!NextTab)
                return vfn032() != 0;
            KillFocus();
            Target = NextTab;
        }
        if (Target->Wnd)
            Target->Wnd->SetFocus();
    }
    else if (wParam == VK_SHIFT)
    {
        GShiftDown = 1;
    }
    return vfn032() != 0;
}
NW_HOOK_METHOD(0x10022ff0, NCWnd, OnKeyDown)

// 0x10023070 (vslot 54)
INT NCWnd::OnKeyUp(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_SHIFT)
        GShiftDown = 0;
    return vfn032() != 0;
}
NW_HOOK_METHOD(0x10023070, NCWnd, OnKeyUp)

// 0x10022fc0 (vslot 60) - lParam holds a signed (dx, dy) pair
INT NCWnd::OnMove(WPARAM wParam, LPARAM lParam)
{
    MoveWindow((SWORD)(lParam & 0xFFFF), (SWORD)((DWORD)lParam >> 16));
    return 1;
}
NW_HOOK_METHOD(0x10022fc0, NCWnd, OnMove)

/*-----------------------------------------------------------------------------
    Focus
-----------------------------------------------------------------------------*/

// 0x10022e70 (vslot 28)
void NCWnd::SetFocus()
{
    if (GFocusEntry == Entry)
        return;
    if (GFocusEntry)
    {
        GFocusEntry->Wnd->KillFocus();
        GFocusEntry = NULL;
    }
    bFocus = 1;
    GFocusEntry = Entry;
    if (Entry->Parent)
        Entry->Parent->Wnd->BringChildToTop(Entry);
}
NW_HOOK_METHOD(0x10022e70, NCWnd, SetFocus)

// 0x10022ec0 (vslot 29)
void NCWnd::KillFocus()
{
    if (GFocusEntry == Entry)
        GFocusEntry = NULL;
    bFocus = 0;
}
NW_HOOK_METHOD(0x10022ec0, NCWnd, KillFocus)

// 0x10022d30 (vslot 37)
void NCWnd::SetTabOrder(NWndEntry* Next, NWndEntry* Prev)
{
    if (Next)
        NextTab = Next;
    if (Prev)
        PrevTab = Prev;
}
NW_HOOK_METHOD(0x10022d30, NCWnd, SetTabOrder)

/*-----------------------------------------------------------------------------
    Visibility and state
-----------------------------------------------------------------------------*/

// 0x10022d50 (vslot 38)
void NCWnd::EnableWindow(UBOOL bEnable)
{
    bEnabled = bEnable;
    for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
        Node->Entry->Wnd->EnableWindow(bEnable);
}
NW_HOOK_METHOD(0x10022d50, NCWnd, EnableWindow)

// 0x10022df0 (vslot 39)
UBOOL NCWnd::IsShowWindow()
{
    return (Style & NWS_VISIBLE) ? 1 : 0;
}
NW_HOOK_METHOD(0x10022df0, NCWnd, IsShowWindow)

// 0x10022d80 (vslot 42) - a minimized window's icon goes away when the window shows
void NCWnd::ShowWindow()
{
    if (Style & NWS_MINIMIZABLE)
        Icon->HideWindow();
    Style |= NWS_VISIBLE;
    for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
        Node->Entry->Wnd->ShowWindow();
}
NW_HOOK_METHOD(0x10022d80, NCWnd, ShowWindow)

// 0x10022dc0 (vslot 43)
void NCWnd::HideWindow()
{
    Style &= ~NWS_VISIBLE;
    for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
        Node->Entry->Wnd->HideWindow();
}
NW_HOOK_METHOD(0x10022dc0, NCWnd, HideWindow)

// 0x10023fd0 (vslot 46) - back from the icon, at the icon's place
void NCWnd::Restore()
{
    SetWindowRect(Icon->ScreenX, Icon->ScreenY, Width, Height);
    Icon->SetWindowRect(0, 0, 0, 0);
    ShowWindow();
    Icon->HideWindow();
}
NW_HOOK_METHOD(0x10023fd0, NCWnd, Restore)

// 0x10023b10 (vslot 47) - the icon of each minimizable window (texture, SysString id)
void NCWnd::Minimize()
{
    UObject* Wnd = Entry->Wnd;
    if (Wnd->IsA(NClass::NCInventoryWnd()))
        Icon->SetIcon(TEXT("L2UI.WindowIcon.MainInventory_large"), 0x8a, 1);
    else if (Wnd->IsA(NClass::NCMatchWnd()))
        Icon->SetIcon(TEXT("L2UI.WindowIcon.MainPartyMatching_large"), 0x185, 1);
    else if (Wnd->IsA(NClass::NCSystemMenuWnd()))
        Icon->SetIcon(TEXT("L2UI.WindowIcon.MainSystem_large"), 0xc1, 1);
    else if (Wnd->IsA(NClass::NCBoardWnd()))
        Icon->SetIcon(TEXT("L2UI.WindowIcon.MainCommunity_large"), 0x186, 1);
    else if (Wnd->IsA(NClass::NCPetitionWnd()))
        Icon->SetIcon(TEXT("L2UI.WindowIcon.GM_icon"), 0x1d6, 1);
    else if (Wnd->IsA(NClass::NCMinimapWnd()))
        Icon->SetIcon(TEXT("L2UI.WindowIcon.Minimap_icon"), 0x1bf, 1);
    else if (Wnd->IsA(NClass::NCPetWnd()))
        Icon->SetIcon(TEXT("L2UI.WindowIcon.MainStatus_large"), 0x1fa, 1);
    else if (Wnd->IsA(NClass::NCSummonedWnd()))
        Icon->SetIcon(TEXT("L2UI.WindowIcon.MainStatus_large"), 0x1f9, 1);
    if (ScreenX < 0)
        Icon->OffsetWindow(-ScreenX, 0);
    HideWindow();
    Icon->ShowWindow();
}
NW_HOOK_METHOD(0x10023b10, NCWnd, Minimize)

// 0x100230a0 (vslot 49)
void NCWnd::SetTitle(const TCHAR* Title)
{
    if (FrameCtrl && Title && *Title)
        FrameCtrl->SetTitleText(Title);
}
NW_HOOK_METHOD(0x100230a0, NCWnd, SetTitle)

/*-----------------------------------------------------------------------------
    Geometry
-----------------------------------------------------------------------------*/

// 0x10022f20 (vslot 40) - edges included
UBOOL NCWnd::PtInRect(INT X, INT Y, INT RectX, INT RectY, INT RectW, INT RectH)
{
    if (X < RectX || X > RectX + RectW)
        return 0;
    if (Y < RectY || Y > RectY + RectH)
        return 0;
    return 1;
}
NW_HOOK_METHOD(0x10022f20, NCWnd, PtInRect)

// 0x10022f60 (vslot 26)
UBOOL NCWnd::IsInWindow(INT X, INT Y)
{
    if (!(Style & NWS_VISIBLE))
        return 0;
    return PtInRect(X, Y, ScreenX, ScreenY, Width, Height);
}
NW_HOOK_METHOD(0x10022f60, NCWnd, IsInWindow)

// 0x10022fb0
NWndEntry* NCWnd::FindWindowAt(INT X, INT Y)
{
    return Entry->Main->FindNWindow(X, Y);
}
NW_HOOK_METHOD(0x10022fb0, NCWnd, FindWindowAt)

// 0x10022ee0 - screen position of the window and of its whole subtree
void NCWnd::OffsetScreen(INT DX, INT DY)
{
    for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
        Node->Entry->Wnd->OffsetScreen(DX, DY);
    ScreenX += DX;
    ScreenY += DY;
}
NW_HOOK_METHOD(0x10022ee0, NCWnd, OffsetScreen)

// 0x10039080 (vslot 45)
void NCWnd::OffsetWindow(INT DX, INT DY)
{
    LocalX += DX;
    LocalY += DY;
    OffsetScreen(DX, DY);
}
NW_HOOK_METHOD(0x10039080, NCWnd, OffsetWindow)

// 0x10023d30 (vslot 44) - with the sticky-window option the move snaps to edges
void NCWnd::MoveWindow(INT DX, INT DY)
{
    if (!DX && !DY)
        return;
    POINT Delta;
    Delta.x = DX;
    Delta.y = DY;
    if (GIsStickyWindow)
        Snap(&Delta);
    LocalX += Delta.x;
    LocalY += Delta.y;
    OffsetScreen(Delta.x, Delta.y);
}
NW_HOOK_METHOD(0x10023d30, NCWnd, MoveWindow)

// abs() the way the stock code inlines it: (x ^ (x >> 31)) - (x >> 31)
static inline INT SnapAbs(INT X)
{
    return (X ^ (X >> 31)) - (X >> 31);
}

// 0x100232a0 - sticky windows (GIsStickyWindow): while a NWS_TOPMOST window is dragged, its
// edges stick to the screen edges and to the edges of the other top-most windows closer than
// 10 px, and come loose once the cursor pulls 10 px away. bStuckTop..8F say which edge is stuck
// (top, left, right, bottom), StuckTopTo..88 to which window entry (NULL = screen edge).
// Delta is the move being applied, in and out.
void NCWnd::Snap(POINT* Delta)
{
    if (!(GetStyle() & NWS_TOPMOST))
        return;
    const INT H = Height;
    const INT L = ScreenX;
    const INT T = ScreenY;
    const INT W = Width;

    // where the drag grabbed the window: the minimized icon's own grab point, else the
    // title bar's
    INT AnchorX, AnchorY;
    if (IsA(NCIcon::StaticClass()))
    {
        AnchorY = static_cast<NCIcon*>(this)->DragY;
        AnchorX = static_cast<NCIcon*>(this)->DragX;
    }
    else if (FrameCtrl)
    {
        AnchorX = FrameCtrl->GrabX;
        AnchorY = FrameCtrl->GrabY;
    }
    else
        return;

    POINT Cursor;
#if NW_FEATURES
    // phase 6: WindowFix (NWindow_C1 src/inject/WindowFix.cpp): the cursor in the game
    // window's client space
    nwf::WindowFix_GetCursorPos(&Cursor);
#else
    GetCursorPos(&Cursor);
#endif

    // stuck to a screen edge: keep still until the cursor is far enough
    if (bStuckLeft && !StuckLeftTo)
    {
        INT D = Cursor.x - AnchorX;
        if (SnapAbs(D) > 9)
        {
            Delta->x = D;
            bStuckLeft = 0;
            return;
        }
        Delta->x = 0;
    }
    if (bStuckTop && !StuckTopTo)
    {
        INT D = Cursor.y - AnchorY;
        if (SnapAbs(D) > 9)
        {
            Delta->y = D;
            bStuckTop = 0;
            return;
        }
        Delta->y = 0;
    }
    if (bStuckRight && !StuckRightTo)
    {
        INT D = Cursor.x - ScreenX - AnchorX;
        if (SnapAbs(D) > 9)
        {
            Delta->x = D;
            bStuckRight = 0;
            return;
        }
        Delta->x = 0;
    }
    if (bStuckBottom && !StuckBottomTo)
    {
        INT D = Cursor.y - ScreenY - AnchorY;
        if (SnapAbs(D) > 10)
        {
            Delta->y = D;
            bStuckBottom = 0;
            return;
        }
        Delta->y = 0;
    }

    // stick to a screen edge
    if (!bStuckLeft && L > -10 && L < 10)
    {
        Delta->x = -L;
        bStuckLeft = 1;
        StuckLeftTo = NULL;
    }
    if (!bStuckTop && T > -10 && T < 10)
    {
        Delta->y = -T;
        bStuckTop = 1;
        StuckTopTo = NULL;
    }
    if (!bStuckRight)
    {
        INT SW = GSEKScreenX;
        if (SW - 10 < W + L && W + L < SW + 10)
        {
            Delta->x = SW - W - L;
            bStuckRight = 1;
            StuckRightTo = NULL;
        }
    }
    if (!bStuckBottom)
    {
        INT SH = GSEKScreenY;
        if (SH - 10 < H + T && H + T < SH + 10)
        {
            Delta->y = SH - H - T;
            bStuckBottom = 1;
            StuckBottomTo = NULL;
        }
    }

    // the other top-most windows (the window itself included, as in the stock code)
    INT Count = Entry->Main->TopMost.Num();
    for (INT i = 0; i < Count; i++)
    {
        NWndEntry* E = Entry->Main->TopMost(i);
        NCWnd* O = E->Wnd;
        const INT OH = O->Height;
        const INT OW = O->Width;
        const INT OL = O->ScreenX;
        const INT OT = O->ScreenY;
        if (!O->IsShowWindow())
            continue;

        // stuck to this window: come loose when the cursor pulls away or the windows no
        // longer overlap along the other axis
        if (bStuckLeft && StuckLeftTo == E)
        {
            INT D = Cursor.x - OW - OL - AnchorX;
            if (SnapAbs(D) > 9 || OH + H < SnapAbs(OT - T + OH) || OH + H < SnapAbs(H - OT + T))
            {
                Delta->x = D;
                bStuckLeft = 0;
                continue;
            }
            Delta->x = 0;
        }
        if (bStuckTop && StuckTopTo == E)
        {
            INT D = Cursor.y - OT - OH - AnchorY;
            if (SnapAbs(D) > 9 || W + OW < SnapAbs(OW - L + OL) || W + OW < SnapAbs(W - OL + L))
            {
                Delta->y = D;
                bStuckTop = 0;
                continue;
            }
            Delta->y = 0;
        }
        if (bStuckRight && StuckRightTo == E)
        {
            INT D = Cursor.x - ScreenX - AnchorX;
            if (SnapAbs(D) > 9 || OH + H < SnapAbs(OT - T + OH) || OH + H < SnapAbs(H - OT + T))
            {
                Delta->x = D;
                bStuckRight = 0;
                continue;
            }
            Delta->x = 0;
        }
        if (bStuckBottom && StuckBottomTo == E)
        {
            INT D = Cursor.y - ScreenY - AnchorY;
            if (SnapAbs(D) > 9 || OW + W < SnapAbs(OW - L + OL) || OW + W < SnapAbs(W - OL + L))
            {
                Delta->y = D;
                bStuckBottom = 0;
                continue;
            }
            Delta->y = 0;
        }

        // stick: left edge to its right edge, top to its bottom, right to its left, bottom to its top
        if (!bStuckLeft && OW - 10 + OL < L && L < OW + 10 + OL && SnapAbs(OT - T + OH) <= H + OH &&
            SnapAbs(H - OT + T) <= H + OH)
        {
            Delta->x = OW - L + OL;
            bStuckLeft = 1;
            StuckLeftTo = E;
        }
        if (!bStuckTop && OT - 10 + OH < T && T < OT + 10 + OH && SnapAbs(OW - L + OL) <= OW + W &&
            SnapAbs(W - OL + L) <= OW + W)
        {
            Delta->y = OT - T + OH;
            bStuckTop = 1;
            StuckTopTo = E;
        }
        if (!bStuckRight && OL - 10 < Width + L && Width + L < OL + 10 && SnapAbs(OT - T + OH) <= OH + H &&
            SnapAbs(H - OT + T) <= OH + H)
        {
            Delta->x = OL - W - L;
            bStuckRight = 1;
            StuckRightTo = E;
        }
        if (!bStuckBottom && OT - 10 < H + T && H + T < OT + 10 && SnapAbs(OW - L + OL) <= OW + W &&
            SnapAbs(W - OL + L) <= OW + W)
        {
            Delta->y = OT - H - T;
            bStuckBottom = 1;
            StuckBottomTo = E;
        }
    }
}
NW_HOOK_METHOD(0x100232a0, NCWnd, Snap)

// 0x10023a40 (vslot 41)
void NCWnd::SetSize(INT W, INT H)
{
    ResetSticky();
    if ((Style & NWS_TITLEBAR) && FrameCtrl)
        FrameCtrl->SetSize(W, FrameCtrl->LocalH);
    Width = LocalW = W;
    Height = LocalH = H;
}
NW_HOOK_METHOD(0x10023a40, NCWnd, SetSize)

// 0x10023d90 - X, Y relative to the parent; W or H 0 keeps the size. The children move by
// the difference between the new position and the current screen position.
void NCWnd::SetWindowRect(INT X, INT Y, INT W, INT H)
{
    ResetSticky();
    for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
        Node->Entry->Wnd->OffsetWindow(X - ScreenX, Y - ScreenY);
    ScreenX = X;
    ScreenY = Y;
    if (W && H)
    {
        Width = W;
        Height = H;
    }
    LocalX = ScreenX;
    LocalY = ScreenY;
    LocalW = Width;
    LocalH = Height;
    if (Entry && Entry->Parent)
    {
        NCWnd* Parent = Entry->Parent->Wnd;
        ScreenX += Parent->ScreenX;
        ScreenY += Parent->ScreenY;
    }
}
NW_HOOK_METHOD(0x10023d90, NCWnd, SetWindowRect)

// 0x10002a80 - CreateChild's tail, on the child
void NCWnd::AttachEntry(NWndEntry* E)
{
    Entry = E;
    if (E->Parent)
    {
        NCWnd* Parent = E->Parent->Wnd;
        ScreenX += Parent->ScreenX;
        ScreenY += Parent->ScreenY;
    }
    WindowProc(WM_CREATE, 0, 0);
}
NW_HOOK_METHOD(0x10002a80, NCWnd, AttachEntry)

// 0x100239d0 - screen position Origin + (X, Y)
void NCWnd::SetScreenPos(INT X, INT Y, const POINT* Origin)
{
    ResetSticky();
    INT DY = ScreenY - Origin->y;
    OffsetWindow(Origin->x - ScreenX + X, 0);
    OffsetWindow(0, Y - DY);
}
NW_HOOK_METHOD(0x100239d0, NCWnd, SetScreenPos)

// 0x10023ab0 (vslot 48) - horizontally centered in the parent
void NCWnd::CenterInParent()
{
    NWndEntry* E = Entry;
    ResetSticky();
    if (E->Parent && E->Parent->Wnd)
        OffsetWindow((INT)(((DWORD)E->Parent->Wnd->LocalW >> 1) - ((DWORD)Width >> 1)) - LocalX, 0);
}
NW_HOOK_METHOD(0x10023ab0, NCWnd, CenterInParent)

/*-----------------------------------------------------------------------------
    Children
-----------------------------------------------------------------------------*/

// 0x100231d0
UBOOL NCWnd::AddChildNode(NWndEntry* Child)
{
    NWndListNode* Node = new NWndListNode;
    Node->Entry = Child;
    if (!ChildTail && !ChildHead)
    {
        ChildHead = Node;
        ChildTail = Node;
        return 1;
    }
    ChildTail->Next = Node;
    Node->Prev = ChildTail;
    ChildTail = Node;
    return 1;
}
NW_HOOK_METHOD(0x100231d0, NCWnd, AddChildNode)

// 0x10023230
void NCWnd::RemoveChildNode(NWndEntry* Child)
{
    NWndListNode* Node = ChildHead;
    while (Node && Node->Entry != Child)
        Node = Node->Next;
    if (!Node)
        return;
    if (Node->Prev)
        Node->Prev->Next = Node->Next;
    if (Node->Next)
        Node->Next->Prev = Node->Prev;
    if (ChildHead == Node)
        ChildHead = Node->Next;
    if (ChildTail == Node)
        ChildTail = Node->Prev;
    delete Node;
}
NW_HOOK_METHOD(0x10023230, NCWnd, RemoveChildNode)

// 0x10022e00 - moves the child to the front of this list, then this window to the front of
// its parent's, up to the top level. The list must not be empty (no check in the stock code).
void NCWnd::BringChildToTop(NWndEntry* Child)
{
    NWndListNode* Node = ChildHead;
    if (Node->Entry != Child)
    {
        for (Node = Node->Next; Node && Node->Entry != Child; Node = Node->Next)
        {
        }
        if (Node)
        {
            if (Node->Next)
            {
                Node->Next->Prev = Node->Prev;
                Node->Prev->Next = Node->Next;
            }
            else
            {
                ChildTail = Node->Prev;
                Node->Prev->Next = NULL;
            }
            ChildHead->Prev = Node;
            Node->Next = ChildHead;
            ChildHead = Node;
            Node->Prev = NULL;
        }
    }
    if (Entry->Parent)
        Entry->Parent->Wnd->BringChildToTop(Entry);
}
NW_HOOK_METHOD(0x10022e00, NCWnd, BringChildToTop)

// 0x10023ca0
NWndEntry* NCWnd::AddChildWindow(NCWnd* Child, DWORD ChildStyle)
{
    if (!Entry)
        return NULL;
    NWndEntry* E = Entry->Main->RegisterNWindow(Child, ChildStyle, Entry);
    E->Parent = Entry;
    AddChildNode(E);
    return E;
}
NW_HOOK_METHOD(0x10023ca0, NCWnd, AddChildWindow)

// 0x10023ce0
void NCWnd::RemoveChildWindow(NWndEntry* Child)
{
    if (Child == GFocusEntry)
        GFocusEntry = NULL;
    if (!(Style & NWS_NOTINPARENTLIST))
        RemoveChildNode(Child);
    Entry->Main->UnRegisterNWindow(Child);
}
NW_HOOK_METHOD(0x10023ce0, NCWnd, RemoveChildWindow)

/*-----------------------------------------------------------------------------
    Helpers
-----------------------------------------------------------------------------*/

// 0x100230d0 - "1234567" -> "1,234,567" (does not use the window)
void NCWnd::FormatNumber(TCHAR* Out, const TCHAR* Digits)
{
    if (!Digits || !*Digits)
        return;
    INT Len = (INT)wcslen(Digits);
    INT i = Len - 1;
    INT OutLen = i / 3 + Len;
    Out[OutLen] = 0;
    TCHAR* Dst = &Out[OutLen - 1];
    for (INT n = 0; i > -1; i--, n++)
    {
        if (n % 3 == 0 && i < (INT)wcslen(Digits) - 1)
            *Dst-- = ',';
        *Dst-- = Digits[i];
    }
}
NW_HOOK_METHOD(0x100230d0, NCWnd, FormatNumber)

// NCCmdTarget's own vtable is the one its destructor stores (its constructor is NCObject's,
// folded by the linker). The original's has 37 slots: 24..36 are declared in NCWnd here.
NW_CHECK_VTABLE(NCCmdTarget, 0x100d7d18, 24)
NW_CHECK_VTABLE(NCWnd, 0x100d7db0, 62)
