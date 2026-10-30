/*=============================================================================
    NConsoleWndInput.cpp - the console's part of each frame and of the input: the timers and
    the tick, the cursor, the painting of the windows, the Windows messages and the engine's
    mouse events handed to the window manager, the shortcut keys, and the shortcut bar and
    targeting slots the engine calls.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCMainWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCPawnSelectWnd.h"
#include "nwindow/NCPetitionWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCVirtualWndMain.h"
#include "nwindow/NViewerData.h"

#if NW_FEATURES
// phase 6: the LineageWindow features called from here (docs/notas/Fase6.md)
#include "features/MinimapHotkey.h"
#include "features/WindowFix.h"
#endif

#include <imm.h>

// set while a window or an item is dragged: SetCursor leaves the cursor alone (name ours)
#if NW_HOOK_MODE
#define GCursorLocked NW_GLOBAL(UBOOL, 0x1012d7bc)
#else
extern UBOOL GCursorLocked;
#endif

// SetCursor's types (the window manager's cursors; names ours)
enum { CURSOR_Normal, CURSOR_Attack, CURSOR_Talk, CURSOR_PickUp };

// the engine's input (UE2 EInputKey / EInputAction values the console tests)
enum
{
    IK_LeftMouse = 0x01,
    IK_RightMouse = 0x02,
    IK_MouseW = 0xE7,
    IK_MouseWheelUp = 0xEC,
    IK_MouseWheelDown = 0xED,
};
enum { IST_Press = 1, IST_Release = 3 };

// the key the shortcut bar leaves to the console on its release (VK_OEM_3, '`')
enum { VK_BACKQUOTE = 0xC0 };

// the actor under the mouse
static FORCEINLINE AActor* HoveredActor(UEngine* Engine)
{
    return static_cast<UGameEngine*>(Engine)->MouseHitActor;
}

// whether the window with the keyboard focus takes the keys itself (NCWnd slot 32: the edit
// controls)
static FORCEINLINE UBOOL FocusTakesKeys()
{
    return GFocusEntry && GFocusEntry->Wnd->vfn032();
}

/*-----------------------------------------------------------------------------
    The cursor.
-----------------------------------------------------------------------------*/

// 0x10045fb0 - the window manager's cursor, unless a drag locked it
void NConsoleWnd::SetCursor(INT Type, UTexture* Texture)
{
    if (!GCursorLocked)
        WndMain->SetCursorType(Type, Texture);
}
NW_HOOK_METHOD(0x10045fb0, NConsoleWnd, SetCursor)

// 0x10045fd0
void NConsoleWnd::LockCursor()
{
    GCursorLocked = 1;
}
NW_HOOK_METHOD(0x10045fd0, NConsoleWnd, LockCursor)

// 0x10045fe0
void NConsoleWnd::UnlockCursor()
{
    GCursorLocked = 0;
}
NW_HOOK_METHOD(0x10045fe0, NConsoleWnd, UnlockCursor)

// 0x10045ff0 (vslot 267) - Ctrl pressed (the engine's key): the attack cursor, locked until
// Ctrl is released
void NConsoleWnd::PressCtrl()
{
    if (!IsCtrlDown())
    {
        SetCursor(CURSOR_Attack, NULL);
        LockCursor();
    }
    bCtrlDown = 1;
}
NW_HOOK_METHOD(0x10045ff0, NConsoleWnd, PressCtrl)

// 0x10046020 (vslot 268)
void NConsoleWnd::ReleaseCtrl()
{
    bCtrlDown = 0;
    if (!IsCtrlDown())
    {
        UnlockCursor();
        SetCursor(CURSOR_Normal, NULL);
    }
}
NW_HOOK_METHOD(0x10046020, NConsoleWnd, ReleaseCtrl)

// 0x10046050 (vslot 230)
UBOOL NConsoleWnd::IsCtrlDown()
{
    return bCtrlDown || bCtrlDownAlt;
}
NW_HOOK_METHOD(0x10046050, NConsoleWnd, IsCtrlDown)

// 0x10054e20 - once a frame (ConsolePostRender): with color cursors, the cursor for what is
// under the mouse - the target: a member of a clan at war with the player's (the network
// handler's IsEnemyPledge) gets attack, or the normal cursor when either side has surrendered
// (IsSurrender), else attack / talk as the actor allows or talk for a private store; an item on the ground: its name drawn above it and the pick up cursor.
// An enchant in progress (the paperdoll's) is cancelled once the mouse leaves the inventory.
void NConsoleWnd::UpdateCursor()
{
    AActor* Hovered = GetTargetedActor(0);
    User* Target = GetSelectedActor();
    if (Hovered)
    {
        User* HoverUser = FindUserByID(Hovered->CreatureID);
        if (HoverUser && GL2UseColorCursor && HoverUser == Target)
        {
            User* Me = GetMyUser();
            INT MyPledge = 0;
            INT MyAlly = 0;
            if (Me)
            {
                MyAlly = Me->Unknown1E8;
                MyPledge = Me->PledgeID;
            }
            INT HisPledge = HoverUser->PledgeID;
            INT HisAlly = HoverUser->Unknown1E8;
            if (Engine->NetHandler->IsEnemyPledge(MyPledge, HisPledge))
            {
                if (Engine->NetHandler->IsSurrender(MyPledge, MyAlly) ||
                    Engine->NetHandler->IsSurrender(HisPledge, HisAlly))
                    SetCursor(CURSOR_Normal, NULL);
                else
                    SetCursor(CURSOR_Attack, NULL);
            }
            else if (Hovered->CanBeAttacked())
                SetCursor(CURSOR_Attack, NULL);
            else if (Hovered->CanBeTold())
                SetCursor(CURSOR_Talk, NULL);
            else if (Target->PrivateStoreType == 1 || Target->PrivateStoreType == 3)
                SetCursor(CURSOR_Talk, NULL);
            else
                SetCursor(CURSOR_Normal, NULL);
        }
    }
    else
    {
        AActor* Pickup = GetPickupUnderMouse();
        if (Pickup)
        {
            Item* Info = FindItemByID(Pickup->CreatureID);
            if (Info)
            {
                UMesh* Mesh = Pickup->Mesh;
                if (Mesh)
                {
                    FBox Box = Mesh->GetRenderBoundingBox(Pickup);
                    FVector Location = Pickup->Location;
                    Location.Z = Box.Max.Z * 0.5f + Location.Z;
                    const TCHAR* Name = GetItemName(Info->ItemID);
                    if (Name && *Name)
                    {
                        TCHAR Text[256];
                        if (Info->bStackable)
                            appSprintf(Text, TEXT("%s(%d)"), Name, Info->Count);
                        else
                            appSprintf(Text, TEXT("%s"), Name);
                        UCanvas* Canvas = Engine->Client->Viewports(0)->Canvas;
                        Canvas->Draw3DCoordText(Location, 0xFFFFFFFF, Text, NULL, 2, 0,
                                                L2FONT_Normal, 0);
                    }
                }
            }
        }
        if (GL2UseColorCursor)
        {
            if (Pickup)
                SetCursor(CURSOR_PickUp, NULL);
            else if (!IsCtrlDown())
                SetCursor(CURSOR_Normal, NULL);
        }
    }

    NCInventoryWnd* Inventory = GetDesktop()->InventoryWnd;
    if (Inventory->EquipWnd->bEnchantMode)
    {
        POINT Mouse;
#if NW_FEATURES
        // phase 6: WindowFix (NWindow_C1 src/inject/WindowFix.cpp): the cursor in the game
        // window's client space, as the windows' rectangles are
        nwf::WindowFix_GetCursorPos(&Mouse);
#else
        GetCursorPos(&Mouse);
#endif
        if (!GetDesktop()->PtInRect(Mouse.x, Mouse.y, Inventory->LocalX, Inventory->LocalY,
                                    Inventory->LocalW, Inventory->LocalH))
            RequestEnchantItem(-1);
    }
}
NW_HOOK_METHOD(0x10054e20, NConsoleWnd, UpdateCursor)

/*-----------------------------------------------------------------------------
    The timers and the tick.
-----------------------------------------------------------------------------*/

// 0x1004ef90 - NCObject::SetTimer: the owner gets WindowProc(WM_TIMER, ID) each Elapse ms
INT NConsoleWnd::NSetTimer(UINT ID, UINT Elapse, NCObject* Owner)
{
    guard(NConsoleWnd::NSetTimer);
    NTimer Timer;
    Timer.ID = ID;
    Timer.Elapse = Elapse;
    Timer.Elapsed = 0;
    Timer.bKilled = 0;
    Timer.Owner = Owner;
    Timers.AddItem(Timer);
    return ID;
    unguard;
}
NW_HOOK_METHOD(0x1004ef90, NConsoleWnd, NSetTimer)

// 0x10047440 - NCObject::KillTimer: the owner's first live timer of that id is marked, the
// next Tick drops it
UBOOL NConsoleWnd::NKillTimer(UINT ID, NCObject* Owner)
{
    for (INT i = 0; i < Timers.Num(); i++)
    {
        NTimer& Timer = Timers(i);
        if (Timer.ID == ID && Timer.Owner == Owner && !Timer.bKilled)
        {
            Timer.bKilled = 1;
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x10047440, NConsoleWnd, NKillTimer)

// 0x10054c20 - every tick: the target window is up while there is a target, a creature or a
// static object the data knows
void NConsoleWnd::UpdateTargetWindow()
{
    User* Target = GetSelectedActor();
    FServerStaticObject* StaticInfo = NULL;
    AActor* Static = GConsole->GetSelectedStaticActor();
    if (Static)
        StaticInfo = GConsole->FindStaticObjectInfo(Static->CreatureID);
    if (Target || StaticInfo)
        GetDesktop()->TargetStatusWnd->ShowWindow();
    else
        GetDesktop()->TargetStatusWnd->HideWindow();
}
NW_HOOK_METHOD(0x10054c20, NConsoleWnd, UpdateTargetWindow)

// 0x10055cf0 (vslot 24) - the engine's tick, then the timers: the killed ones are dropped
// (the one after each is skipped this tick), the others count the time and send WM_TIMER to
// their owner once their interval has passed, keeping the time over; the shortcut bar's cool
// times, the target window and the chat's text capture
void NConsoleWnd::Tick(FLOAT InDeltaTime)
{
    guard(NConsoleWnd::Tick);
#if NW_FEATURES
    // phase 6: the LineageWindow features' per-frame work, before the stock Tick, in NWindow_C1's
    // order and under one handler (src/features/NwFeatureTick.cpp)
    nwf::OnConsoleTick(this, InDeltaTime);
#endif
    UL2ConsoleWnd::Tick(InDeltaTime);
    DeltaTime = InDeltaTime;
    for (INT i = 0; i < Timers.Num(); i++)
    {
        NTimer& Timer = Timers(i);
        if (Timer.bKilled)
        {
            Timers.Remove(i, 1);
            continue;
        }
        DOUBLE Elapsed = (DOUBLE)InDeltaTime + Timer.Elapsed;
        Timer.Elapsed = (FLOAT)Elapsed;
        DOUBLE Interval = (DOUBLE)Timer.Elapse * 0.001f;
        if (Elapsed > Interval)
        {
            Timer.Elapsed = (FLOAT)(Elapsed - Interval);
            if (Timer.Owner)   // a window's (NCObject::SetTimer)
                static_cast<NCWnd*>(Timer.Owner)->WindowProc(WM_TIMER, Timer.ID, 0);
        }
    }
    if (Desktop && GetDesktop()->ShortCutWnd)
        GetDesktop()->ShortCutWnd->Tick(InDeltaTime);
    UpdateTargetWindow();
    if ((GL2TextCapture == 1 || GL2TextCapture == 3) && Desktop && GetDesktop()->ChatWnd)
        GetDesktop()->ChatWnd->UpdateTextCapture();
    unguard;
}
NW_HOOK_METHOD(0x10055cf0, NConsoleWnd, Tick)

/*-----------------------------------------------------------------------------
    Painting and the messages.
-----------------------------------------------------------------------------*/

// 0x10055e50 (vslot 237) - once a frame after the scene: the cursor (UpdateCursor), then
// every window painted on the canvas; what the paint returned, else the canvas pointer
INT NConsoleWnd::ConsolePostRender(UCanvas* Canvas)
{
    guard(NConsoleWnd::ConsolePostRender);
    if (Desktop)
        UpdateCursor();
    if (!WndMain || !bInitialized || bSuspended)
        return (INT)(INT_PTR)Canvas;
    WndMain->SetGDevice(Canvas);
    INT Result = WndMain->DispatchWndMsg(WM_PAINT, 0, 0);
    WndMain->ReleaseGDevice();
    return Result;
    unguard;
}
NW_HOOK_METHOD(0x10055e50, NConsoleWnd, ConsolePostRender)

// 0x10057500 (vslot 238) - a message of the game window
INT NConsoleWnd::DispatchWndMsg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    guard(NConsoleWnd::DispatchWndMsg);
    return DispatchWndMsgX(hWnd, Msg, wParam, lParam);
    unguard;
}
NW_HOOK_METHOD(0x10057500, NConsoleWnd, DispatchWndMsg)

// 0x10055940 - a message for the windows (hWnd is not read): none while a request of the
// character stages waits (taken), nor before Initialize; some keys never go to the windows,
// the arrows only to a focused edit control. In the game (or the test console) the shortcut
// keys go first; a right click over the focused chat line is dropped, and a left button
// released there ends a drag of the color cursors. Closing messages finalize the console.
// On the character selection a click that no window took picks the character under the
// mouse, or plays its pose again when it is the chosen one.
INT NConsoleWnd::DispatchWndMsgX(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    guard(NConsoleWnd::DispatchWndMsgX);
    if (State == L2CS_Waiting)
        return 1;
    if (!bInitialized || bSuspended)
        return 0;
    if (Msg == WM_KEYDOWN || Msg == WM_KEYUP)
    {
        if (wParam == VK_PRIOR || wParam == VK_NEXT || wParam == VK_INSERT || wParam == VK_SHIFT ||
            wParam == VK_SNAPSHOT)
            return 0;
        if ((wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN ||
             (wParam >= 0xC8 && wParam <= 0xD7)) &&
            !FocusTakesKeys())
            return 0;
    }
    if (GIsOnlyL2Console)
    {
        if (HookShortCutKey(Msg, wParam, lParam))
            return 1;
    }
    else if (State == L2CS_InGame)
    {
        NCEditBox* ChatLine = GetDesktop()->ChatWnd->EditBox;
        if (ChatLine->IsFocused() && (Msg == WM_RBUTTONUP || Msg == WM_RBUTTONDOWN))
            return 0;
        if (ChatLine->IsFocused() && Msg == WM_LBUTTONUP &&
            (WndMain->CursorType == 5 || WndMain->CursorType == 4))
        {
            UnlockCursor();
            SetCursor(CURSOR_Normal, NULL);
        }
        if (HookShortCutKey(Msg, wParam, lParam))
            return 1;
    }
    switch (Msg)
    {
    case WM_DESTROY:
    case WM_CLOSE:
    case WM_QUIT:
        Finalize();
        return 0;
    case WM_PAINT:
        return 0;
    }
    INT Result = WndMain->DispatchWndMsg(Msg, wParam, lParam);
    if (State != L2CS_Lobby)
        return Result;
    if (Result || Msg != WM_LBUTTONDOWN || !Engine || !HoveredActor(Engine))
        return 1;
    APawn* Pawn = CastToPawn(HoveredActor(Engine));
    if (!Pawn)
        return 1;
    MessageWnd->ClearText();
    LobbyWnd->DialogBox->HideWindow();
    // a character of the selection: chosen, or its third social action when it was already
    if (Pawn->SelectFlags & 1)
    {
        INT Chosen = -1;
        if (LobbyWnd && LobbyWnd->PawnSelectWnd)
        {
            Chosen = LobbyWnd->PawnSelectWnd->GetSelected();
            LobbyWnd->PawnSelectWnd->SelectCharacter(Pawn->SelectIndex);
        }
        if (Chosen != Pawn->SelectIndex)
            SelectedCharacterNum(Pawn->SelectIndex);
        else
            Pawn->PlayAnim(0, Pawn->SocialAnimName[3], Pawn->SpeedRate, 0.3f, 0, 1);
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10055940, NConsoleWnd, DispatchWndMsgX)

// 0x10055f20 (vslot 240) - the engine's mouse: the buttons as the Windows messages at the
// cursor, the wheel as WM_MOUSEWHEEL with the last delta (IK_MouseW). A wheel step no
// window takes is sent a second time; one a window took eats the release that follows.
UBOOL NConsoleWnd::MasterConsoleEventProcess(INT Key, INT Action, FLOAT Delta)
{
    guard(NConsoleWnd::MasterConsoleEventProcess);
    UINT Msg;
    WPARAM wParam = 0;
    POINT Mouse;
    LPARAM lParam;
    switch (Key)
    {
    case IK_LeftMouse:
        Msg = (Action == IST_Press) ? WM_LBUTTONDOWN : WM_LBUTTONUP;
#if NW_FEATURES
        // phase 6: WindowFix (NWindow_C1 src/inject/WindowFix.cpp): the click at the cursor
        // in the game window's client space
        nwf::WindowFix_GetCursorPos(&Mouse);
#else
        GetCursorPos(&Mouse);
#endif
        lParam = (Mouse.y << 16) | Mouse.x;
        break;
    case IK_RightMouse:
        Msg = (Action == IST_Press) ? WM_RBUTTONDOWN : WM_RBUTTONUP;
#if NW_FEATURES
        // phase 6: WindowFix (NWindow_C1 src/inject/WindowFix.cpp): the same
        nwf::WindowFix_GetCursorPos(&Mouse);
#else
        GetCursorPos(&Mouse);
#endif
        lParam = (Mouse.y << 16) | Mouse.x;
        break;
    case IK_MouseW:
        MouseWheelDelta = (INT)Delta;
        return 0;
    case IK_MouseWheelUp:
    case IK_MouseWheelDown:
        if (Action == IST_Press)
        {
            Msg = WM_MOUSEWHEEL;
            wParam = MouseWheelDelta;
#if NW_FEATURES
            // phase 6: WindowFix (NWindow_C1 src/inject/WindowFix.cpp): the same
            nwf::WindowFix_GetCursorPos(&Mouse);
#else
            GetCursorPos(&Mouse);
#endif
            lParam = (Mouse.y << 16) | Mouse.x;
            if (DispatchWndMsgX(GViewportHWnd, Msg, wParam, lParam))
            {
                bWheelHandled = 1;
                return 1;
            }
            break;
        }
        if (Action == IST_Release && bWheelHandled)
        {
            bWheelHandled = 0;
            return 1;
        }
        return 0;
    default:
        return 0;
    }
    return DispatchWndMsgX(GViewportHWnd, Msg, wParam, lParam);
    unguard;
}
NW_HOOK_METHOD(0x10055f20, NConsoleWnd, MasterConsoleEventProcess)

/*-----------------------------------------------------------------------------
    The shortcut keys.
-----------------------------------------------------------------------------*/

// a key the windows left puts the focus in the chat line, but over an edit control, or for
// Tab, Ctrl or Esc
static FORCEINLINE void FocusChatLine(NConsoleWnd* Console, WPARAM Key)
{
    if (FocusTakesKeys())
        return;
    NCEditBox* ChatLine = Console->GetDesktop()->ChatWnd->EditBox;
    if (ChatLine->IsFocused())
        return;
    if (Key == VK_TAB || Key == VK_CONTROL || Key == VK_ESCAPE)
        return;
    ChatLine->SetFocus();
}

// whether the player has GM rights (the GM windows' keys)
static FORCEINLINE UBOOL IsBuilder(NConsoleWnd* Console)
{
    return Console->GetMyUser() && Console->GetMyUser()->BuilderLevel > 0 &&
           Console->GetMyUser()->BuilderLevel < 7;
}

// 0x1004f050 - the keys before the windows get them (1: taken). F10 comes as a system key:
// taken as a plain one when pressed (HIWORD(lParam) 0x44 / 0x4044) or released (0xC044).
// The shortcut bar's keys first; Esc drops the target, Ctrl is the engine's attack key,
// Enter puts the focus in the chat line when the chat needs Enter, Tab the inventory in the
// game. Letters open windows while the chat needs Enter; otherwise any key or character
// (below 0x80) puts the focus in the chat line. Alt + a key opens windows, Alt + F1 .. F10
// (and 1 .. 8 when the chat needs Enter) turns the shortcut bar's page. The IME's start
// switches the Korean IME to alphanumeric when the chat needs Enter, else focuses the chat
// line.
UBOOL NConsoleWnd::HookShortCutKey(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    guard(NConsoleWnd::HookShortCutKey);
    UBOOL bHandled = 0;
    if ((Msg == WM_SYSKEYDOWN || Msg == WM_SYSKEYUP) && wParam == VK_F10)
    {
        WORD Flags = HIWORD(lParam);
        if (Flags == 0x44 || Flags == 0x4044)
            Msg = WM_KEYDOWN;
        else if (Flags == 0xC044)
            Msg = WM_KEYUP;
    }
    switch (Msg)
    {
    case WM_KEYUP:
        if (GetDesktop()->ShortCutWnd->OnShortCutKeyUp(wParam, lParam))
            return 1;
        if (wParam == VK_CONTROL)
            ReleaseCtrl();
        else if (wParam == VK_BACKQUOTE)
            bHandled = 1;
        return bHandled;

    case WM_KEYDOWN:
        if (GetDesktop()->ShortCutWnd->OnShortCutKeyDown(wParam, lParam))
            return 1;
        if (wParam == VK_ESCAPE)
        {
            if (HasSelectedTarget())
            {
                L2ParamStack Params(10);
                RequestTargetCancel(Params);
            }
            bHandled = 1;
        }
        else if (wParam == VK_CONTROL)
            PressCtrl();
        else if (wParam == VK_RETURN)
        {
            if (!GIsEnterChatting)
            {
                FocusChatLine(this, wParam);
                return bHandled;
            }
            if (!GetDesktop()->ChatWnd->EditBox->IsFocused() && !bNoChatOnEnter &&
                !FocusTakesKeys())
            {
                GetDesktop()->ChatWnd->EditBox->SetFocus();
                bHandled = 1;
            }
        }
        else if (wParam == VK_TAB)
        {
            if (State == L2CS_InGame)
            {
                GetDesktop()->ToggleInventory();
                bHandled = 1;
            }
        }
        if (GIsEnterChatting)
        {
            if (!FocusTakesKeys())
            {
                switch (wParam)
                {
                case 'B': GetDesktop()->ToggleBoard(); bHandled = 1; break;
                case 'C': GetDesktop()->ToggleCharacter(2); bHandled = 1; break;
                case 'I': GetDesktop()->ToggleCharacter(-1); bHandled = 1; break;
                case 'K': GetDesktop()->ToggleCharacter(1); bHandled = 1; break;
                case 'T': GetDesktop()->ToggleCharacter(0); bHandled = 1; break;
                case 'U': GetDesktop()->ToggleCharacter(3); bHandled = 1; break;
                case 'V': GetDesktop()->ToggleInventory(); bHandled = 1; break;
                case 'X': GetDesktop()->ToggleSystemMenu(); bHandled = 1; break;
#if NW_FEATURES
                // phase 6: MinimapHotkey (NWindow_C1 src/inject/MinimapHotkey.cpp): M toggles
                // the map
                case 'M': bHandled = nwf::MinimapHotkey_BareM(this, lParam); break;
#endif
                }
            }
            return bHandled;
        }
        if (wParam <= 0x7F)
            FocusChatLine(this, wParam);
        return bHandled;

    case WM_CHAR:
        if (!GIsEnterChatting && wParam <= 0x7F)
            FocusChatLine(this, wParam);
        return bHandled;

    case WM_SYSKEYDOWN:
        switch (wParam)
        {
        case VK_PRIOR: GetDesktop()->ChatWnd->CycleTab(0); break;
        case VK_NEXT: GetDesktop()->ChatWnd->CycleTab(1); break;
        case 'B': GetDesktop()->ToggleBoard(); break;
        case 'C': GetDesktop()->MainWnd->ShowTab(2); break;
        case 'G':
            if (IsBuilder(this))
                GetDesktop()->ToggleGMWnd();
            break;
        case 'H': GetDesktop()->ToggleInterface(); break;
        case 'I': GetDesktop()->ToggleCharacter(-1); break;
        case 'J': GetDesktop()->ToggleChat(); break;
        case 'K': GetDesktop()->MainWnd->ShowTab(1); break;
        case 'M':
#if NW_FEATURES
            // phase 6: MinimapHotkey (NWindow_C1 src/inject/MinimapHotkey.cpp): Alt+M toggles
            // the map
            if (nwf::MinimapHotkey_AltM(this, lParam))
                break;
#endif
            if (IsBuilder(this))
                GetDesktop()->GMPetitionWnd->ShowWindow();
            break;
        case 'T': GetDesktop()->MainWnd->ShowTab(0); break;
        case 'U': GetDesktop()->MainWnd->ShowTab(3); break;
        case 'V': GetDesktop()->ToggleInventory(); break;
        case 'W': GetDesktop()->CloseWindows(); break;
        case 'X': GetDesktop()->ToggleSystemMenu(); break;
        case VK_F1: case VK_F2: case VK_F3: case VK_F4: case VK_F5:
        case VK_F6: case VK_F7: case VK_F8: case VK_F9: case VK_F10:
            GetDesktop()->ShortCutWnd->SetPage(wParam - VK_F1);
            break;
        }
        if (GIsEnterChatting && wParam >= '1' && wParam <= '8')
            GetDesktop()->ShortCutWnd->SetPage(wParam - '1');
        return 1;

    case WM_IME_STARTCOMPOSITION:
    case WM_IME_NOTIFY:
        if (GIsEnterChatting && GLanguageType == 0)
        {
            if (!FocusTakesKeys())
                ImmSetConversionStatus(ImmGetContext(GViewportHWnd), IME_CMODE_ALPHANUMERIC,
                                       IME_SMODE_NONE);
        }
        else if (!FocusTakesKeys() && !GetDesktop()->ChatWnd->EditBox->IsFocused())
            GetDesktop()->ChatWnd->EditBox->SetFocus();
        return bHandled;
    }
    return bHandled;
    unguard;
}
NW_HOOK_METHOD(0x1004f050, NConsoleWnd, HookShortCutKey)

/*-----------------------------------------------------------------------------
    Slots of the shortcut bar and the targeting the engine calls.
-----------------------------------------------------------------------------*/

// 0x10042990 (vslot 26)
UBOOL NConsoleWnd::IsOverWindow(INT X, INT Y)
{
    if (!WndMain)
        return 0;
    return WndMain->IsOverWindow(X, Y);
}
NW_HOOK_METHOD(0x10042990, NConsoleWnd, IsOverWindow)

// 0x10057590 (vslot 263) - the slot of the current page
void NConsoleWnd::UseShortCutSlot(INT Index)
{
    UseShortCut(GetDesktop()->ShortCutWnd->PageSlot(Index));
}
NW_HOOK_METHOD(0x10057590, NConsoleWnd, UseShortCutSlot)

// 0x100447f0 (vslot 264) - 0: the page before, 1: the next one
void NConsoleWnd::ChangeShortCutPage(INT bNext)
{
    if (bNext == 0)
        GetDesktop()->ShortCutWnd->PrevPage();
    else if (bNext == 1)
        GetDesktop()->ShortCutWnd->NextPage();
}
NW_HOOK_METHOD(0x100447f0, NConsoleWnd, ChangeShortCutPage)

// 0x10047b50 (vslot 269)
void NConsoleWnd::SetPadShift1(BYTE bShift)
{
    GetDesktop()->ShortCutWnd->bPadShift1 = bShift;
}
NW_HOOK_METHOD(0x10047b50, NConsoleWnd, SetPadShift1)

// 0x10047b70 (vslot 270)
void NConsoleWnd::SetPadShift2(BYTE bShift)
{
    GetDesktop()->ShortCutWnd->bPadShift2 = bShift;
}
NW_HOOK_METHOD(0x10047b70, NConsoleWnd, SetPadShift2)

// 0x1004d4c0 (vslot 265) - no handler in NWindow: among the pawns the level lists around the
// player (ULevel::GetPawnListExtent on GMem, given P4 .. P7 and the radius), the user of the
// one after CreatureID (bNext 1) or before it (0; before the first: the last); the first
// one's when CreatureID is not above 0 or not in the list, NULL for no list
User* NConsoleWnd::FindNextPawnInArea(FLOAT Radius, INT CreatureID, INT bNext, INT P4, INT P5,
                                      INT P6, INT P7)
{
    FMemMark Mark(GMem);
    User* Result = NULL;
    ULevel* Level = NULL;
    UViewport* Viewport = GConsole->Engine->Client->Viewports(0);
    if (Viewport)
        Level = Viewport->Actor->XLevel;
    if (Viewport->Actor->Pawn && Level)
    {
        FCheckResult* List = Level->GetPawnListExtent(GMem, P4, P5, P6, P7, (INT)Radius);
        if (List)
        {
            Result = FindUserByID(List->Actor->CreatureID);
            if (CreatureID > 0)
            {
                INT Count = 0;
                for (FCheckResult* Hit = List; Hit; Hit = Hit->Next)
                    Count++;
                FCheckResult* Last = &List[Count - 1];   // the list is contiguous on GMem
                FCheckResult* Prev = NULL;
                FCheckResult* Hit = List;
                for (INT i = 0; i < Count; i++)
                {
                    if (Hit->Actor->CreatureID == CreatureID)
                    {
                        if (bNext == 1)
                        {
                            if (Hit->Next)
                            {
                                Result = FindUserByID(Hit->Next->Actor->CreatureID);
                                break;
                            }
                        }
                        else if (bNext == 0)
                        {
                            if (Prev)
                            {
                                Result = FindUserByID(Prev->Actor->CreatureID);
                                break;
                            }
                            if (Last)
                            {
                                Result = FindUserByID(Last->Actor->CreatureID);
                                break;
                            }
                        }
                    }
                    Prev = Hit;
                    if (Hit)
                        Hit = Hit->Next;
                }
            }
        }
    }
    Mark.Pop();
    return Result;
}
NW_HOOK_METHOD(0x1004d4c0, NConsoleWnd, FindNextPawnInArea)
