/*=============================================================================
    NCVirtualWndMain.cpp - window manager: registration, routing, painting, cursors.
    Stock: 0x10024290 .. 0x10025bb0 (+ ~NCAbstractMain 0x10024690).
=============================================================================*/
#include "nwindow/NCVirtualWndMain.h"
#include "re/NwDebug.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NClasses.gen.h"
#if NW_FEATURES
#include "features/WindowFix.h"   // phase 6: WindowFix
#endif

#include <math.h>

// Messages DispatchWndMsg accepts (stock table at 0x1010b268, 27 entries). The ones without a
// case below (WM_CREATE, WM_DESTROY, WM_TIMER, WM_SYSKEYDOWN, NWM_DRAWTOOLTIP) end as "not
// handled".
static const UINT GDispatchedMsgs[] = {
    WM_LBUTTONDBLCLK, WM_RBUTTONDBLCLK, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_RBUTTONDOWN, WM_RBUTTONUP,
    WM_MOUSEMOVE, WM_KEYDOWN, WM_KEYUP, WM_CHAR, WM_PAINT, WM_INPUTLANGCHANGE, WM_IME_SETCONTEXT,
    WM_IME_STARTCOMPOSITION, WM_IME_COMPOSITION, WM_IME_ENDCOMPOSITION, WM_IME_COMPOSITIONFULL,
    WM_IME_NOTIFY, WM_IME_CONTROL, WM_CREATE, WM_DESTROY, WM_TIMER, WM_MOUSEWHEEL, WM_MOVE,
    WM_SYSKEYDOWN, WM_SETCURSOR, NWM_DRAWTOOLTIP,
};

// Cursor resources of the package (stock table at 0x1010b2d4), index = CursorType.
static const WORD GCursorIDs[7] = {0x66, 0x65, 0x6a, 0x6b, 0x7f, 0x82, 0x83};

/*-----------------------------------------------------------------------------
    Construction
-----------------------------------------------------------------------------*/

// 0x10024690 - NW_DECLARE_CLASS destructor
NW_HOOK_DTOR(0x10024690, NCAbstractMain)

// 0x10025a80
NCVirtualWndMain::NCVirtualWndMain()
{
    GDevice = NULL;
    NumWindows = 0;
    CursorType = 0;
    bCursorEnabled = 1;
    bColorCursor = 0;
    GDevice = new (GetOuter()) NCGDevice;
    GDevice->Init();
    bToolTipShown = 0;
    bMouseMoveTaken = 0;
    bCursorEnabled = 1;
}
NW_HOOK_CTOR(0x10025a80, NCVirtualWndMain)

// 0x10025bb0 - NW_DECLARE_CLASS destructor + the three arrays
NW_HOOK_DTOR(0x10025bb0, NCVirtualWndMain)

// 0x10024670
void NCVirtualWndMain::DeleteGDevice()
{
    if (GDevice)
    {
        delete GDevice;
        GDevice = NULL;
    }
}
NW_HOOK_METHOD(0x10024670, NCVirtualWndMain, DeleteGDevice)

// 0x100242c0 - the canvas of the frame being drawn
void NCVirtualWndMain::SetGDevice(UCanvas* Canvas)
{
    guard(NCVirtualWndMain::SetGDevice);
    GDevice->SetCanvas(Canvas);
    unguard;
}
NW_HOOK_METHOD(0x100242c0, NCVirtualWndMain, SetGDevice)

// 0x10024340
void NCVirtualWndMain::ReleaseGDevice()
{
    guard(NCVirtualWndMain::ReleaseGDevice);
    GDevice->ClearCanvas();
    unguard;
}
NW_HOOK_METHOD(0x10024340, NCVirtualWndMain, ReleaseGDevice)

/*-----------------------------------------------------------------------------
    Registration
-----------------------------------------------------------------------------*/

// 0x100257d0 (vslot 24)
NWndEntry* NCVirtualWndMain::RegisterNWindow(NCWnd* Wnd, DWORD Style, NWndEntry* Parent)
{
    guard(NCVirtualWndMain::RegisterNWindow);
    if (NumWindows >= 1000)
        return NULL;
    NWndEntry* E = new NWndEntry;
    E->bValid = 1;
    E->Wnd = Wnd;
    Wnd->SetStyle(Style);
    E->Main = this;
    E->Parent = Parent;
    Windows.AddItem(E);
    NumWindows++;
    if (E->Wnd && (E->Wnd->GetStyle() & NWS_TOPMOST))
        TopMost.AddItem(E);
    if (!E->Parent)
        TopLevel.AddItem(E);
    return E;
    unguard;
}
NW_HOOK_METHOD(0x100257d0, NCVirtualWndMain, RegisterNWindow)

// 0x10025910 (vslot 25) - the window gets WM_DESTROY, the entry is freed
void NCVirtualWndMain::UnRegisterNWindow(NWndEntry* Entry)
{
    guard(NCVirtualWndMain::UnRegisterNWindow);
    if (!Entry->Parent)
    {
        for (INT i = 0; i < TopLevel.Num(); i++)
        {
            if (TopLevel(i) == Entry)
            {
                TopLevel.Remove(i);
                break;
            }
        }
    }
    for (INT i = 0; i < Windows.Num(); i++)
    {
        if (Windows(i) == Entry)
        {
            Entry->Wnd->WindowProc(WM_DESTROY, 0, 0);
            Windows.Remove(i);
            delete Entry;
            NumWindows--;
            break;
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x10025910, NCVirtualWndMain, UnRegisterNWindow)

// 0x100257a0 (vslot 28)
void NCVirtualWndMain::AddTopLevel(NWndEntry* Entry)
{
    Entry->Parent = NULL;
    TopLevel.AddItem(Entry);
}
NW_HOOK_METHOD(0x100257a0, NCVirtualWndMain, AddTopLevel)

// 0x10025180 (vslot 27)
void NCVirtualWndMain::ClearFocus()
{
    guard(NCVirtualWndMain::ClearFocus);
    for (DWORD i = 0; i < NumWindows; i++)
    {
        NWndEntry* E = Windows(i);
        if (E->bValid && E->Wnd->IsFocused())
            Windows(i)->Wnd->KillFocus();
    }
    unguard;
}
NW_HOOK_METHOD(0x10025180, NCVirtualWndMain, ClearFocus)

/*-----------------------------------------------------------------------------
    Hit tests
-----------------------------------------------------------------------------*/

// 0x100252a0 (vslot 26) - searches the first visible top-level window only
NWndEntry* NCVirtualWndMain::FindNWindow(INT X, INT Y)
{
    guard(NCVirtualWndMain::FindNWindow);
    for (INT i = 0; i < TopLevel.Num(); i++)
    {
        if ((TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE) && TopLevel(i)->Wnd->IsShowWindow())
            return FindChildWindowAt(TopLevel(i)->Wnd, X, Y);
    }
    return NULL;
    unguard;
}
NW_HOOK_METHOD(0x100252a0, NCVirtualWndMain, FindNWindow)

// 0x10025220 (vslot 29) - deepest window of the tree under (X, Y), front child first
NWndEntry* NCVirtualWndMain::FindChildWindowAt(NCWnd* Wnd, INT X, INT Y)
{
    NWndEntry* Found = Wnd->Entry;
    for (NWndListNode* Node = Wnd->ChildHead; Node; Node = Node->Next)
    {
        NCWnd* Child = Node->Entry->Wnd;
        if (!(Child->GetStyle() & NWS_NOHITTEST) && Child->IsShowWindow() && Child->IsInWindow(X, Y))
            return FindChildWindowAt(Child, X, Y);
    }
    return Found;
}
NW_HOOK_METHOD(0x10025220, NCVirtualWndMain, FindChildWindowAt)

// 0x10025720 - is any window at (X, Y)? (clicks elsewhere belong to the game world)
UBOOL NCVirtualWndMain::IsOverWindow(INT X, INT Y)
{
    for (DWORD i = 0; i < NumWindows; i++)
    {
        NWndEntry* E = Windows(i);
        if (E->bValid && !(E->Wnd->GetStyle() & NWS_NOHITTEST) && Windows(i)->Wnd->IsShowWindow() &&
            Windows(i)->Wnd->IsInWindow(X, Y))
            return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x10025720, NCVirtualWndMain, IsOverWindow)

// 0x10024980 - packed position inside the window rectangle
UBOOL NCVirtualWndMain::HitTest(NWndEntry* Entry, LPARAM Pos)
{
    INT X = (SWORD)Pos;
    INT Y = (SWORD)((DWORD)Pos >> 16);
    if (Entry->bValid && Entry->Wnd->IsShowWindow())
    {
        NCWnd* W = Entry->Wnd;
        if (W->PtInRect(X, Y, W->ScreenX, W->ScreenY, W->Width, W->Height))
            return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x10024980, NCVirtualWndMain, HitTest)

// 0x10024710 - style 0x200 windows only take clicks inside their parent too (a drop-down
// NCHtmlComboBox list is tested against its own rectangle)
UBOOL NCVirtualWndMain::HitTestClick(NWndEntry* Entry, INT X, INT Y)
{
    if (Entry->Wnd->IsA(NClass::NCHtmlComboBox()) && Entry->bValid)
    {
        if ((Entry->Wnd->GetStyle() & 0x200) && Entry->Wnd->IsShowWindow() && Entry->Wnd->IsInWindow(X, Y))
        {
            NCWnd* W = Entry->Wnd;
            if (W->PtInRect(X, Y, W->ScreenX, W->ScreenY, W->Width, W->Height))
                return 1;
            goto Default;
        }
    }
    else if (!Entry->bValid)
    {
        goto Default;
    }
    if ((Entry->Wnd->GetStyle() & 0x200) && Entry->Wnd->IsShowWindow() && Entry->Wnd->IsInWindow(X, Y))
    {
        NCWnd* Parent = Entry->Parent->Wnd;
        if (Parent)
            return Entry->Wnd->PtInRect(X, Y, Parent->ScreenX, Parent->ScreenY, Parent->Width, Parent->Height) != 0;
    }
Default:
    if (Entry->Wnd->IsInWindow(X, Y) && (Entry->Wnd->GetStyle() & NWS_VISIBLE) &&
        !(Entry->Wnd->GetStyle() & NWS_NOHITTEST))
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x10024710, NCVirtualWndMain, HitTestClick)

/*-----------------------------------------------------------------------------
    Routing
-----------------------------------------------------------------------------*/

// 0x100243c0 - client position packed in an LPARAM -> screen position packed the same way
LPARAM NCVirtualWndMain::ClientToScreenPacked(LPARAM Pos)
{
    POINT P;
    P.x = (SWORD)Pos;
    P.y = Pos >> 16;
#if NW_FEATURES
    // phase 6: WindowFix (NWindow_C1 src/inject/WindowFix.cpp): the viewport window's
    // client position stays as it is (the UI works in client space)
    nwf::WindowFix_ClientToScreen(GViewportHWnd, &P);
#else
    ClientToScreen(GViewportHWnd, &P);
#endif
    return (LPARAM)(((DWORD)(_WORD)P.y << 16) | (_WORD)P.x);
}
NW_HOOK_METHOD(0x100243c0, NCVirtualWndMain, ClientToScreenPacked)

// 0x10024290
UBOOL NCVirtualWndMain::IsDispatchedMsg(UINT Msg)
{
    for (DWORD i = 0; i < ARRAY_COUNT(GDispatchedMsgs); i++)
        if (GDispatchedMsgs[i] == Msg)
            return 1;
    return 0;
}
NW_HOOK_METHOD(0x10024290, NCVirtualWndMain, IsDispatchedMsg)

// 0x100248a0 - the deepest window hit takes the click and the focus
INT NCVirtualWndMain::RouteClick(NWndEntry* Entry, INT X, INT Y, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    INT Result = 0;
    if (HitTestClick(Entry, X, Y))
    {
        for (NWndListNode* Node = Entry->Wnd->ChildHead; Node; Node = Node->Next)
        {
            Result = RouteClick(Node->Entry, X, Y, Msg, wParam, lParam);
            if (Result)
                return Result;
        }
        Entry->Wnd->SetFocus();
        Entry->Wnd->WindowProc(Msg, wParam, lParam);
        return 1;
    }
    for (NWndListNode* Node = Entry->Wnd->ChildHead; Node; Node = Node->Next)
    {
        Result = RouteClick(Node->Entry, X, Y, Msg, wParam, lParam);
        if (Result)
            return Result;
    }
    return Result;
}
NW_HOOK_METHOD(0x100248a0, NCVirtualWndMain, RouteClick)

// 0x10024a10 - while the left button is down the focused window gets every move (drags);
// otherwise the first window under the cursor (NCResizeFrame: always). Always returns 1.
INT NCVirtualWndMain::RouteMouseMove(NWndEntry* Entry, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    for (NWndListNode* Node = Entry->Wnd->ChildHead; Node; Node = Node->Next)
        RouteMouseMove(Node->Entry, Msg, wParam, lParam);
    UINT bLeftDown = wParam & MK_LBUTTON;
    if (bLeftDown && Entry->Wnd && Entry->Wnd->IsFocused())
    {
        Entry->Wnd->WindowProc(Msg, wParam, lParam);
        return 1;
    }
    if (bMouseMoveTaken)
        return 1;
    if (Entry->Wnd->IsA(NClass::NCResizeFrame()))
    {
        Entry->Wnd->WindowProc(Msg, wParam, lParam);
        return 1;
    }
    if (!HitTest(Entry, lParam) || bLeftDown)
        return 1;
    Entry->Wnd->WindowProc(Msg, wParam, lParam);
    if (!Entry->Wnd->IsA(NClass::NCConsole()))
        bMouseMoveTaken = 1;
    return 1;
}
NW_HOOK_METHOD(0x10024a10, NCVirtualWndMain, RouteMouseMove)

// 0x10024400 - to the focused window of the tree. IME messages to a window whose vfn032 says
// so go to the grandchildren below the children that answer vfn033 instead.
INT NCVirtualWndMain::SendToFocused(NWndEntry* Entry, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    INT Result = 0;
    if (Entry && Entry->Wnd && Entry->Wnd->IsFocused())
    {
        UBOOL bIme = (Msg >= WM_IME_SETCONTEXT && Msg <= WM_IME_COMPOSITIONFULL) || Msg == WM_IME_CHAR ||
                     Msg == WM_INPUTLANGCHANGE || (Msg >= WM_IME_STARTCOMPOSITION && Msg <= WM_IME_COMPOSITION);
        if (bIme && Entry->Wnd->vfn032())
        {
            for (NWndListNode* Node = Entry->Wnd->ChildHead; Node; Node = Node->Next)
            {
                if (!Node->Entry->Wnd->vfn033())
                    continue;
                for (NWndListNode* Sub = Node->Entry->Wnd->ChildHead; Sub; Sub = Sub->Next)
                    if (Sub->Entry->Wnd->WindowProc(Msg, wParam, lParam))
                        Result = 1;
            }
            return Result;
        }
        return Entry->Wnd->WindowProc(Msg, wParam, lParam) ? 1 : 0;
    }
    for (NWndListNode* Node = Entry->Wnd->ChildHead; Node; Node = Node->Next)
    {
        Result = SendToFocused(Node->Entry, Msg, wParam, lParam);
        if (Result)
            break;
    }
    return Result;
}
NW_HOOK_METHOD(0x10024400, NCVirtualWndMain, SendToFocused)

/*-----------------------------------------------------------------------------
    Painting
-----------------------------------------------------------------------------*/

// 0x10024570 - back to front: the list tail is the bottom of the z-order
void NCVirtualWndMain::PaintChildren(NWndEntry* Entry)
{
    if (!Entry)
        return;
    for (NWndListNode* Node = Entry->Wnd->ChildTail; Node; Node = Node->Prev)
    {
        Node->Entry->Wnd->WindowProc(WM_PAINT, (WPARAM)GDevice, 0);
        PaintChildren(Node->Entry);
    }
}
NW_HOOK_METHOD(0x10024570, NCVirtualWndMain, PaintChildren)

// 0x10024b00 - the first window under the cursor (children first) draws its tool tip; the
// cursor comes from GetCursorPos, i.e. desktop coordinates, as in the stock code
void NCVirtualWndMain::PaintToolTips(NWndEntry* Entry)
{
    if (!Entry)
        return;
    NCWnd* Wnd = Entry->Wnd;
    for (NWndListNode* Node = Wnd->ChildHead; Node; Node = Node->Next)
        PaintToolTips(Node->Entry);
    if (bToolTipShown)
        return;
    INT L = Wnd->ScreenX;
    INT T = Wnd->ScreenY;
    INT W = Wnd->Width;
    INT H = Wnd->Height;
    POINT Cursor;
#if NW_FEATURES
    // phase 6: WindowFix (NWindow_C1 src/inject/WindowFix.cpp): the cursor in the game
    // window's client space
    nwf::WindowFix_GetCursorPos(&Cursor);
#else
    GetCursorPos(&Cursor);
#endif
    if (!Entry->bValid)
        return;
    if (!Entry->Wnd->PtInRect(Cursor.x, Cursor.y, L, T, W, H))
        return;
    if (!Entry->Wnd->IsShowWindow())
        return;
    Wnd->WindowProc(NWM_DRAWTOOLTIP, (WPARAM)GDevice, 0);
    if (Entry->Wnd->IsA(NClass::NCConsole()))
        return;
    bToolTipShown = 1;
}
NW_HOOK_METHOD(0x10024b00, NCVirtualWndMain, PaintToolTips)

/*-----------------------------------------------------------------------------
    Dispatch
-----------------------------------------------------------------------------*/

// 0x10024bf0 - entry point of every message for the UI; top-level windows are tried from
// the front (last index) except for painting, which goes from the back
INT NCVirtualWndMain::DispatchWndMsg(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    guard(NCVirtualWndMain::DispatchWndMsg);
    if (!IsDispatchedMsg(Msg))
        return 0;
    switch (Msg)
    {
    case WM_PAINT:
    {
        guard(WM_PAINT);
        for (INT i = 0; i < TopLevel.Num(); i++)
        {
            if (TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE)
            {
                TopLevel(i)->Wnd->WindowProc(Msg, (WPARAM)GDevice, lParam);
                PaintChildren(TopLevel(i));
                PaintToolTips(TopLevel(i));
                bToolTipShown = 0;
            }
        }
        if (NW_DEBUG_MARKER())  // proof mode, not stock (re/NwDebug.h)
            ::nw::DrawDebugMarker(GDevice);
        unguard;
        return 0;
    }
    case WM_SETCURSOR:
        if (GL2UseColorCursor && (_WORD)lParam == HTCLIENT)
            return 1;
        return 0;
    case WM_MOUSEMOVE:
        lParam = ClientToScreenPacked(lParam);
        for (INT i = TopLevel.Num() - 1; i >= 0; i--)
            if ((TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE) && RouteMouseMove(TopLevel(i), Msg, wParam, lParam))
                bMouseMoveTaken = 0;
        return 0;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    {
        LPARAM Pos = ClientToScreenPacked(lParam);
        INT Y = (SWORD)((DWORD)Pos >> 16);
        for (INT i = TopLevel.Num() - 1; i >= 0; i--)
            if ((TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE) && RouteClick(TopLevel(i), (SWORD)Pos, Y, Msg, wParam, Pos))
                return 1;
        break;
    }
    case WM_RBUTTONDOWN:
    {
        LPARAM Pos = ClientToScreenPacked(lParam);
        INT Y = (SWORD)((DWORD)Pos >> 16);
        for (INT i = TopLevel.Num() - 1; i >= 0; i--)
            if ((TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE) && RouteClick(TopLevel(i), (SWORD)Pos, Y, Msg, wParam, Pos))
                return 1;
        if (GL2UseColorCursor)
        {
            UnlockCursor();
            ::SetCursor(Cursors[0]);
        }
        break;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        lParam = ClientToScreenPacked(lParam);
        // fall through
    case WM_MOVE:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    case WM_MOUSEWHEEL:
    case WM_INPUTLANGCHANGE:
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_IME_CONTROL:
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_CHAR:
        for (INT i = TopLevel.Num() - 1; i >= 0; i--)
            if ((TopLevel(i)->Wnd->GetStyle() & NWS_VISIBLE) && SendToFocused(TopLevel(i), Msg, wParam, lParam))
                return 1;
        return 0;
    default:
        return 0;
    }
    // a click nobody took: whoever has the focus loses it
    if (GFocusEntry)
    {
        GFocusEntry->Wnd->KillFocus();
        GFocusEntry = NULL;
    }
    return 0;
    unguard;
}
NW_HOOK_METHOD(0x10024bf0, NCVirtualWndMain, DispatchWndMsg)

/*-----------------------------------------------------------------------------
    Cursors
-----------------------------------------------------------------------------*/

// 0x100245b0 - resources of this package (the stock module in hook mode)
void NCVirtualWndMain::LoadCursors()
{
    for (INT i = 0; i < 7; i++)
    {
        HCURSOR Cursor;
        if (!GUnicodeOS)
            Cursor = LoadCursorA(GNWindowInstance, MAKEINTRESOURCEA(GCursorIDs[i]));
        else
            Cursor = LoadCursorW(GNWindowInstance, MAKEINTRESOURCEW(GCursorIDs[i]));
        Cursors[i] = Cursor;
        if (i == 0)
            ::SetCursor(Cursor);
    }
    if (GUnicodeOS)
        ArrowCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(0x7f00));  // IDC_ARROW
    else
        ArrowCursor = LoadCursorA(NULL, MAKEINTRESOURCEA(0x7f00));
}
NW_HOOK_METHOD(0x100245b0, NCVirtualWndMain, LoadCursors)

// 0x10024630
void NCVirtualWndMain::DestroyCursors()
{
    for (INT i = 0; i < 7; i++)
        DestroyCursor(Cursors[i]);
    if (bColorCursor)
    {
        DestroyCursor(ColorCursor);
        ColorCursor = NULL;
        bColorCursor = 0;
    }
}
NW_HOOK_METHOD(0x10024630, NCVirtualWndMain, DestroyCursors)

// 0x100256b0 - type 4 with a texture: a color cursor built from it
void NCVirtualWndMain::SetCursorType(INT Type, UTexture* Texture)
{
    if (!bCursorEnabled)
        return;
    if (!GL2UseColorCursor)
    {
        ::SetCursor(ArrowCursor);
        CursorType = -1;
        return;
    }
    if (Type != CursorType && Type < 7)
    {
        if (Type == 4 && Texture)
        {
            CreateColorCursor(Texture);
            CursorType = 4;
            return;
        }
        ::SetCursor(Cursors[Type]);
        CursorType = Type;
    }
}
NW_HOOK_METHOD(0x100256b0, NCVirtualWndMain, SetCursorType)

// 0x10025380 - a DXT1 / DXT3 texture as the cursor: at 32 bits of color its pixels with an empty
// mask, else RGB565 pixels with a mask of the transparent ones. The mask is never freed, the
// 16-bit pixels only at 16 bits of color (stock leaks).
void NCVirtualWndMain::CreateColorCursor(UTexture* Texture)
{
    if (Texture->Format != TEXF_DXT1 && Texture->Format != TEXF_DXT3)
        return;
    if (bColorCursor)
    {
        DestroyCursor(ColorCursor);
        ColorCursor = NULL;
        bColorCursor = 0;
    }
    INT ColorBits = GColorBits;
    BYTE* Pixels = NULL;
    BYTE* Pixels16 = NULL;
    Texture->ConvertDXT(0, 0, 0, &Pixels);
    INT W = Texture->USize;
    INT H = Texture->VSize;
    if (ColorBits == 32)
    {
        INT Size = H * W * 4;
        BYTE* Mask = (BYTE*)appMalloc(Size, TEXT("new"));
        appMemset(Mask, 0, Size);
        ColorCursor = CreateIcon(NULL, W, H, 1, 32, Mask, Pixels);
    }
    else
    {
        BYTE* Mask = (BYTE*)appMalloc(W / 8 * H, TEXT("new"));
        Pixels16 = (BYTE*)appMalloc(H * W * 2, TEXT("new"));
        BYTE* Pixel = (BYTE*)appMalloc(4, TEXT("new"));
        BYTE Bits = 0;
        for (INT Y = 0; Y < H; Y++)
        {
            INT Bit = 1;
            for (INT X = 0; X < W; X++)
            {
                if (Y < H && X < W)
                    appMemcpy(Pixel, Pixels + (Y * W + X) * 4, 4);
                else
                    appMemset(Pixel, 0, 4);
                DWORD Color;
                if (Pixel[3] == 0)
                    Color = 0;
                else
                    Color = (Pixel[0] >> 3) + (((Pixel[2] & 0xF8) << 5) + (Pixel[1] & 0xFC)) * 8;
                appMemcpy(Pixels16 + (Y * W + X) * 2, &Color, 2);
                // a transparent pixel sets its bit of the mask, the high bit first
                if (Pixel[3] == 0)
                    Bits = (BYTE)(INT)(pow(2.0, (DOUBLE)(8 - Bit)) + Bits);
                Bit++;
                if (Bit == 9)
                {
                    appMemcpy(Mask + Y * W / 8 + X / 8, &Bits, 1);
                    Bit = 1;
                    Bits = 0;
                }
            }
        }
        appFree(Pixel);
        ColorCursor = CreateIcon(NULL, W, H, 1, 16, Mask, Pixels16);
    }
    bColorCursor = 1;
    SetClassLongW(GViewportHWnd, GCL_HCURSOR, (LONG)ColorCursor);
    ::SetCursor(ColorCursor);
    appFree(Pixels);
    if (ColorBits == 16)
        appFree(Pixels16);
}
NW_HOOK_METHOD(0x10025380, NCVirtualWndMain, CreateColorCursor)

// 0x10024080, the package's DllMain (GNWindowInstance = hInstance; return 1), stays stock: the
// CRT entry that calls it keeps ecx across the call as far as tools/regcheck.py can tell (a
// register call follows), and ours would not preserve it

NW_CHECK_VTABLE(NCAbstractMain, 0x100dd878, 29)
NW_CHECK_VTABLE(NCVirtualWndMain, 0x100ddaa0, 30)
