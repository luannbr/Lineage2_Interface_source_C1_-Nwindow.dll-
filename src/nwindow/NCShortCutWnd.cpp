/*=============================================================================
    NCShortCutWnd.cpp - the shortcut bar. Stock: 0x100a7ff0 .. 0x100a8b1a, 0x100a8b20 ..
    0x100aaf22 (with the jump tables), 0x100aaf30 .. 0x100ab37f.
=============================================================================*/
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCActionItemWnd.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCPetWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCPetInventoryWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#if NW_FEATURES
// phase 6: AutoShot, ManufactureCmd (NWindow_C1 src/inject/)
#include "features/AutoShot.h"
#include "features/ManufactureCmd.h"
// phase 6: ItemHoverTip (not in NWindow_C1: the hover tip in the recent style)
#include "features/ItemHoverTip.h"
#endif

// the two map items: using them shows the minimap instead of asking the server
enum { ITEM_Map1 = 1665, ITEM_Map2 = 1863 };

// an item of the player's inventory, of the pet's (names ours)
static FORCEINLINE NItemEntry* FindInventoryItem(INT ObjectID)
{
    return GDesktop->InventoryWnd->FindItem(ObjectID);
}
static FORCEINLINE NItemEntry* FindPetInventoryItem(INT ObjectID)
{
    return GDesktop->PetWnd->InventoryWnd->GetItem(ObjectID);
}

/*-----------------------------------------------------------------------------
    NShortCutItem.
-----------------------------------------------------------------------------*/

// 0x100a7ff0 - the rest is left as allocated
NShortCutItem::NShortCutItem()
{
    bUsed = 0;
}
NW_HOOK_CTOR(0x100a7ff0, NShortCutItem)

// 0x100a8b10 - only the cool time's (empty) destructor; the arrays' new[] and delete[] use it
NW_HOOK_DTOR(0x100a8b10, NShortCutItem)

// 0x100a8010
NShortCutItem::NShortCutItem(const NItemInfo& Info)
{
    Icon = NULL;
    Name[0] = 0;
    IconName[0] = 0;
    DisplayName[0] = 0;
    Icon = Info.Icon;
    appStrcpy(Name, Info.Name);
    const TCHAR* Src = Info.IconName;
    if (Src)
        appStrcpy(IconName, Src);
    Type = Info.Param;
    ObjectID = Info.ObjectID;
    ID = Info.ID;
    Count = Info.Count;
    Level = Info.UnknownC20;
    MpConsume = Info.UnknownC34[12];
    Enchant = Info.Enchant;
    bStackable = Info.bStackable;
    bUsed = 0;
}
NW_HOOK_CTOR_ARGS(0x100a8010, NShortCutItem, (const NItemInfo& A), (A))

// 0x100a8140
UBOOL NShortCutItem::IsStackable()
{
    return bStackable;
}
NW_HOOK_METHOD(0x100a8140, NShortCutItem, IsStackable)

// 0x100a8110
TCHAR* NShortCutItem::GetName()
{
    TCHAR* S = Name;
    if (!S || !*S)
        return NULL;
    return S;
}
NW_HOOK_METHOD(0x100a8110, NShortCutItem, GetName)

// 0x100a8120
TCHAR* NShortCutItem::GetDisplayName()
{
    TCHAR* S = DisplayName;
    if (!S || !*S)
        return NULL;
    return S;
}
NW_HOOK_METHOD(0x100a8120, NShortCutItem, GetDisplayName)

// 0x100a8150 - IconName is not copied; the cool time stops
void NShortCutItem::CopyFrom(NShortCutItem* Src)
{
    if (Src->GetIcon())
        Icon = Src->GetIcon();
    if (Src->GetName())
        appStrcpy(Name, Src->GetName());
    if (Src->GetDisplayName())
        appStrcpy(DisplayName, Src->GetDisplayName());
    Type = Src->Type;
    ObjectID = Src->ObjectID;
    ID = Src->ID;
    Count = Src->Count;
    Level = Src->Level;
    Enchant = Src->Enchant;
    bStackable = Src->bStackable;
    Unknown260 = Src->Unknown260;
    MpConsume = Src->MpConsume;
    bUsed = Src->bUsed;
    Owner = Src->Owner;
    CoolTime.Stop();
}
NW_HOOK_METHOD(0x100a8150, NShortCutItem, CopyFrom)

// 0x100a8240 - DisplayName is left as it was
void NShortCutItem::Clear()
{
    Name[0] = 0;
    IconName[0] = 0;
    Icon = NULL;
    Type = -1;
    ObjectID = 0;
    ID = 0;
    Count = 0;
    Level = -1;
    MpConsume = 0;
    Enchant = 0;
    bStackable = 0;
    Unknown260 = -1;
    bUsed = 0;
    CoolTime.Stop();
}
NW_HOOK_METHOD(0x100a8240, NShortCutItem, Clear)

/*-----------------------------------------------------------------------------
    NCShortCutWnd.
-----------------------------------------------------------------------------*/

// 0x100ab1f0 - Items and the other textures are left as allocated (OnCreate)
NCShortCutWnd::NCShortCutWnd(INT InCols, INT InPages, INT InIconSize)
{
    Keys = NULL;
    Cols = InCols;
    IconSize = InIconSize;
    BackTex = NULL;
    MaskTex = NULL;
    ToolTipTex = NULL;
    Pages = InPages;
    PressedSlot = -1;
    HoverSlot = -1;
    Page = 0;
    bHorizontal = 0;
    IconBackTex = FindTexture(TEXT("l2ui.nwindow.icon_back"));
    IconClickTex = FindTexture(TEXT("l2ui.nwindow.icon_click"));
    bPadShift1 = 0;
    bPadShift2 = 0;
    bPressed = 0;
}
NW_HOOK_CTOR_ARGS(0x100ab1f0, NCShortCutWnd, (INT A, INT B, INT C), (A, B, C))

// 0x100ab320
NW_HOOK_DTOR(0x100ab320, NCShortCutWnd)

// 0x100a82a0 (vslot 24)
INT NCShortCutWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == (WPARAM)NID_SHORTCUT_NEXT)
            return OnNextClicked() != 0;
        if (wParam == (WPARAM)NID_SHORTCUT_PREV)
            return OnPrevClicked() != 0;
        if (wParam == (WPARAM)NID_SHORTCUT_ROTATE)
            return OnRotateClicked() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100a82a0, NCShortCutWnd, WindowProc)

// an arrow or the turn button, repeating every 300 ms while held (repeated inline in the stock
// OnCreate)
static FORCEINLINE NCPushButton* CreateBarButton(NCWnd* Parent, const TCHAR* NormalTex,
                                                 const TCHAR* PushedTex, INT ID, INT X, INT Y)
{
    NCPushButton* Button = new NCPushButton(NormalTex, PushedTex);
    Button->ID = ID;
    Button->SetWindowRect(X, Y, 12, 12);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->RepeatDelay = 300;
    return Button;
}

// 0x100aab00 (vslot 35) - NControl's OnCreate is not called; the title bar is made here
INT NCShortCutWnd::OnCreate()
{
    PadButtonTex[0] = FindTexture(TEXT("L2UI.ShortcutWnd.pad_button_1"));
    PadButtonTex[1] = FindTexture(TEXT("L2UI.ShortcutWnd.pad_button_2"));
    PadButtonTex[2] = FindTexture(TEXT("L2UI.ShortcutWnd.pad_button_3"));
    PadButtonTex[3] = FindTexture(TEXT("L2UI.ShortcutWnd.pad_button_4"));
    FrameCtrl = new NCFrameCtrl(0);
    FrameID = NID_SHORTCUT_FRAME;
    FrameCtrl->ID = NID_SHORTCUT_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 44, 11);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI.ShortcutWnd.Back"));
    MaskTex = FindTexture(TEXT("L2UI.ShortcutWnd.mask"));
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    ToggleTex = FindTexture(TEXT("L2_SkillTime.ToggleEffect.ToggleEffect001"));
    Items = new NShortCutItem[Cols * Pages];
    Keys = new NShortCutKey[Cols * Pages];
    NextButton = CreateBarButton(this, TEXT("L2UI.ShortcutWnd.RightBtn1"),
                                 TEXT("L2UI.ShortcutWnd.RightBtn2"), NID_SHORTCUT_NEXT, 31, 11);
    PrevButton = CreateBarButton(this, TEXT("L2UI.ShortcutWnd.LeftBtn1"),
                                 TEXT("L2UI.ShortcutWnd.LeftBtn2"), NID_SHORTCUT_PREV, 1, 11);
    RotateButton = CreateBarButton(this, TEXT("L2UI.ShortcutWnd.Shorcutbtn1"),
                                   TEXT("L2UI.ShortcutWnd.Shorcutbtn2"), NID_SHORTCUT_ROTATE, 16,
                                   403);
    return 1;
}
NW_HOOK_METHOD(0x100aab00, NCShortCutWnd, OnCreate)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x100a8b20 (vslot 36)
INT NCShortCutWnd::OnDestroy()
{
    if (Items)
    {
        delete[] Items;
        Items = NULL;
    }
    if (Keys)
    {
        delete[] Keys;
        Keys = NULL;
    }
    DestroyChild(this, NextButton);
    DestroyChild(this, PrevButton);
    DestroyChild(this, RotateButton);
    NControl::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100a8b20, NCShortCutWnd, OnDestroy)

// a slot of the current page: icon, frame, pressed mark, toggle skill mark, the flash when
// its cool time ends (with a sound), the cool time itself, then its number (1 .. 9, 0); (X, Y)
// is the icon's corner (repeated for both layouts in the stock OnPaint)
static FORCEINLINE void DrawSlot(NCShortCutWnd* Wnd, NCGDevice* Device, INT Slot, INT Number,
                                 INT X, INT Y)
{
    NShortCutItem& Item = Wnd->Items[Slot];
    if (Item.bUsed)
    {
        INT Size = Wnd->IconSize;
        Device->DrawTexture(X, Y, Size, Size, 0, 0, Size, Size, Item.GetIcon());
        Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, Wnd->IconBackTex);
        if (Wnd->PressedSlot == Slot && Wnd->bPressed)
            Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, Wnd->IconClickTex);
        if (GConsole->IsToggleSkill(Item.ID, Item.Level) && GConsole->HasAbnormal(Item.ID))
        {
            Wnd->ToggleTex->UnknownA4 = 9.0f;
            Device->DrawTexture(X, Y, 32, 32, 0, 0, 32, 32, Wnd->ToggleTex);
        }
        if (Item.CoolTime.bFlashing)
        {
            Size = Wnd->IconSize;
            Device->DrawBrightenTexture(X, Y, Size, Size, 0, 0, Size, Size,
                                        Item.CoolTime.GetFlashTexture(), 0xFF008080);
            if (Wnd->Items[Slot].CoolTime.bSoundDue)
            {
                GConsole->Engine->OnInterfacePlaySound(9);
                Wnd->Items[Slot].CoolTime.bSoundDue = 0;
            }
        }
        NShortCutItem& Cool = Wnd->Items[Slot];
        if (Cool.CoolTime.bRunning && !Cool.CoolTime.IsIdle() &&
            Wnd->Items[Slot].CoolTime.GetTexture())
        {
            Size = Wnd->IconSize;
            Device->DrawTexture(X, Y, Size, Size, 0, 0, Size, Size,
                                Wnd->Items[Slot].CoolTime.GetTexture());
        }
    }
    TCHAR Text[6];
    appSprintf(Text, TEXT("%d"), Number % 10);
    Device->DrawSpecialDigit(1, X + 1, Y + 1, 0xFFB09B79, Text);
}

// 0x100a8c10 (vslot 50) - the page number (1 .. 10) at the top, the slots, and the game pad's
// button marks on the slots its buttons use (the horizontal marks are not quite mirrored)
INT NCShortCutWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    SIZE Size;
    TCHAR Text[6];
    if (!bHorizontal)
    {
        if (BackTex)
            Device->DrawTexture(0, 0, 44, 417, 0, 0, 44, 417, BackTex);
        appSprintf(Text, TEXT("%d"), Page + 1);
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
        if (Page + 1 < 10)
            Device->DrawSpecialDigit(1, 22 - Size.cx / 2, 14, 0xFFFFFFFF, Text);
        else if (Page + 1 == 10)
        {
            appSprintf(Text, TEXT("%d"), 1);
            Device->DrawSpecialDigit(1, 16, 14, 0xFFFFFFFF, Text);
            appSprintf(Text, TEXT("%d"), 0);
            Device->DrawSpecialDigit(1, 20, 14, 0xFFFFFFFF, Text);
        }
        INT First = Page * Cols;
        INT Y = 31;
        for (INT i = 1; i - 1 < Cols; i++)
        {
            DrawSlot(this, Device, First + i - 1, i, 6, Y);
            Y += IconSize + 5;
        }
        if (GL2UsingJoystick && GL2CanUseJoystick)
        {
            if (!bPadShift1 && !bPadShift2)
            {
                Device->DrawTexture(5, 32, 34, 34, 0, 0, 34, 34, PadButtonTex[0]);
                Device->DrawTexture(5, 68, 34, 34, 0, 0, 34, 34, PadButtonTex[1]);
                Device->DrawTexture(5, 104, 34, 34, 0, 0, 34, 34, PadButtonTex[2]);
                Device->DrawTexture(5, 140, 34, 34, 0, 0, 34, 34, PadButtonTex[3]);
            }
            else if (bPadShift1 && !bPadShift2)
            {
                Device->DrawTexture(5, 180, 34, 34, 0, 0, 34, 34, PadButtonTex[0]);
                Device->DrawTexture(5, 216, 34, 34, 0, 0, 34, 34, PadButtonTex[1]);
                Device->DrawTexture(5, 252, 34, 34, 0, 0, 34, 34, PadButtonTex[2]);
                Device->DrawTexture(5, 288, 34, 34, 0, 0, 34, 34, PadButtonTex[3]);
            }
            else if (!bPadShift1 && bPadShift2)
            {
                Device->DrawTexture(5, 328, 34, 34, 0, 0, 34, 34, PadButtonTex[0]);
                Device->DrawTexture(5, 364, 34, 34, 0, 0, 34, 34, PadButtonTex[1]);
            }
        }
    }
    else
    {
        if (BackTex)
            Device->DrawTexture(0, 0, 417, 44, 0, 0, 417, 44, BackTex);
        appSprintf(Text, TEXT("%d"), Page + 1);
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
        if (Page + 1 < 10)
            Device->DrawSpecialDigit(1, 17 - Size.cx / 2, 18, 0xFFFFFFFF, Text);
        else if (Page + 1 == 10)
        {
            appSprintf(Text, TEXT("%d"), 1);
            Device->DrawSpecialDigit(1, 12, 18, 0xFFFFFFFF, Text);
            appSprintf(Text, TEXT("%d"), 0);
            Device->DrawSpecialDigit(1, 16, 18, 0xFFFFFFFF, Text);
        }
        INT First = Page * Cols;
        INT X = 31;
        for (INT i = 1; i - 1 < Cols; i++)
        {
            DrawSlot(this, Device, First + i - 1, i, X, 6);
            X += IconSize + 5;
        }
        if (GL2UsingJoystick && GL2CanUseJoystick)
        {
            if (!bPadShift1 && !bPadShift2)
            {
                Device->DrawTexture(30, 5, 34, 34, 0, 0, 34, 34, PadButtonTex[0]);
                Device->DrawTexture(68, 5, 34, 34, 0, 0, 34, 34, PadButtonTex[1]);
                Device->DrawTexture(104, 5, 34, 34, 0, 0, 34, 34, PadButtonTex[2]);
                Device->DrawTexture(142, 5, 34, 34, 0, 0, 34, 34, PadButtonTex[3]);
            }
            else if (bPadShift1 && !bPadShift2)
            {
                Device->DrawTexture(178, 5, 34, 34, 0, 0, 34, 34, PadButtonTex[0]);
                Device->DrawTexture(216, 5, 34, 34, 0, 0, 34, 34, PadButtonTex[1]);
                Device->DrawTexture(252, 5, 34, 34, 0, 0, 34, 34, PadButtonTex[2]);
                Device->DrawTexture(290, 5, 34, 34, 0, 0, 34, 34, PadButtonTex[3]);
            }
            else if (!bPadShift1 && bPadShift2)
            {
                Device->DrawTexture(326, 5, 34, 34, 0, 0, 34, 34, PadButtonTex[0]);
                Device->DrawTexture(364, 5, 34, 34, 0, 0, 34, 34, PadButtonTex[1]);
            }
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
#if NW_FEATURES
    // phase 6: AutoShot (NWindow_C1 src/inject/AutoShot.cpp): the mark over the automatic shots
    nwf::AutoShot_OnPaint(this, Device);
#endif
    return 1;
}
NW_HOOK_METHOD(0x100a8c10, NCShortCutWnd, OnPaint)

// 0x100a9660 (vslot 51) - the slot's label beside it (next to the vertical bar's left edge,
// below the horizontal bar's top), kept on the screen
INT NCShortCutWnd::OnDrawToolTip(NCGDevice* Device)
{
    if (!bHorizontal)
    {
        if (HoverSlot < 0 || !Items[HoverSlot].bUsed || !Items[HoverSlot].GetDisplayName())
            return 1;
        Device->BeginPaint(ScreenX, ScreenY);
        INT Y = (HoverSlot - Page * Cols) * (IconSize + 5) + 31;
        INT X = 6;
#if NW_FEATURES
        // phase 6: ItemHoverTip: the recent chronicles' tip (a dark box, the grade symbol) above the
        // slot; with [ItemTip] HoverStyle=0 the stock label below
        if (nwf::ItemHoverTip_DrawShortcut(this, Device, Items[HoverSlot].GetDisplayName(),
                                           Items[HoverSlot].Type, Items[HoverSlot].ID, X, Y, IconSize, 168))
        {
            Device->EndPaint();
            return 1;
        }
#endif
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Items[HoverSlot].GetDisplayName(), 168, 0, 0, 0, 0);
        INT ScreenW = GConsole->Desktop->LocalW;
        if (ScreenX + Size.cx + 6 > ScreenW)
            X = ScreenW - ScreenX - Size.cx;
        else if (ScreenX + 6 < 0)
            X = 6 - ScreenX;
        if (ScreenY + Y < 0)
            Y += Size.cy + 34;
        Device->SetClip(ScreenX + X, ScreenY + Y, Size.cx, Size.cy);
        Device->DrawStretchedTex(X, Y, Size.cx, Size.cy, ToolTipTex);
        Device->DrawText(X, Y, 0xFFFFFFFF, Items[HoverSlot].GetDisplayName(), 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else
    {
        if (HoverSlot < 0 || !Items[HoverSlot].bUsed || !Items[HoverSlot].GetDisplayName())
            return 1;
        Device->BeginPaint(ScreenX, ScreenY);
        INT X = (HoverSlot - Page * Cols) * (IconSize + 5) + 31;
        INT Y = 6;
#if NW_FEATURES
        // phase 6: ItemHoverTip, as above
        if (nwf::ItemHoverTip_DrawShortcut(this, Device, Items[HoverSlot].GetDisplayName(),
                                           Items[HoverSlot].Type, Items[HoverSlot].ID, X, Y, IconSize, 168))
        {
            Device->EndPaint();
            return 1;
        }
#endif
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Items[HoverSlot].GetDisplayName(), 168, 0, 0, 0, 0);
        INT ScreenH = GConsole->Desktop->LocalH;
        if (ScreenY + Size.cy + 6 > ScreenH)
            Y = ScreenH - ScreenY - Size.cy;
        else if (ScreenY + 6 < 0)
            Y = 6 - ScreenY;
        if (ScreenX + X < 0)
            X += Size.cx + 33;
        Device->SetClip(ScreenX + X, ScreenY + Y, Size.cx, Size.cy);
        Device->DrawStretchedTex(X, Y, Size.cx, Size.cy, ToolTipTex);
        Device->DrawText(X, Y, 0xFFFFFFFF, Items[HoverSlot].GetDisplayName(), 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x100a9660, NCShortCutWnd, OnDrawToolTip)

// 0x100a9940 (vslot 56)
INT NCShortCutWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Slot = SlotAt(LOWORD(lParam), HIWORD(lParam));
    PressedSlot = Slot;
    HoverSlot = Slot;
    if (Slot >= 0)
    {
        bDragging = 1;
        bPressed = 1;
    }
    return 1;
}
NW_HOOK_METHOD(0x100a9940, NCShortCutWnd, OnLButtonDown)

// 0x100a9980 (vslot 55) - a dragged slot's icon rides on the cursor
INT NCShortCutWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    HoverSlot = SlotAt(LOWORD(lParam), HIWORD(lParam));
    if (IsFocused())
    {
        if (bDragging)
        {
            if (PressedSlot >= 0 && Items[PressedSlot].GetIcon())
            {
                SetCursor(4, Items[PressedSlot].GetIcon());
                LockCursor();
            }
        }
        else if (!GConsole->IsCtrlDown())
        {
            UnlockCursor();
            SetCursor(0, NULL);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100a9980, NCShortCutWnd, OnMouseMove)

// a party request on someone who is not a player: a system message in the chat, red (the
// action window's are another color)
static FORCEINLINE void ChatSystemMessage(INT MsgID)
{
    NCChatWnd* ChatWnd = GDesktop->ChatWnd;
    ChatWnd->Lists[CHATLIST_All]->AddString(*GL2GameData.SystemMsg[MsgID].Message, 0xFFFF0000, -1);
}

// what an action slot does (repeated inline in OnLButtonUp and OnShortCutKeyDown): the action
// window's cases (NCActionItemWnd::UseAction), the social motions by a fixed table; ranges 200
static FORCEINLINE void UseAction(INT ActionID)
{
#if NW_FEATURES
    // phase 6: ManufactureCmd (NWindow_C1 src/inject/ManufactureCmd.cpp): ids 32 .. 52, past
    // the stock switch (37 Dwarven Manufacture, 52 servitor Return)
    if (nwf::ManufactureCmd_UseAction(ActionID, 0))
        return;
#endif
    switch (ActionID)
    {
    case ACTION_SitStand:
    case ACTION_WalkRun:
    case ACTION_PetStay:
    case ACTION_PetAttack:
    case ACTION_PetStop:
    case ACTION_PetReturn:
    case ACTION_PetSkill:
    case ACTION_SummonStay:
    case ACTION_SummonAttack:
    case ACTION_SummonStop:
    case ACTION_SummonSkill:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)ActionID);
        Params.PushBack((void*)(INT_PTR)GConsole->IsCtrlDown());
        Params.PushBack(
            (void*)(INT_PTR)GConsole->Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
        GConsole->RequestActionUse(Params);
        break;
    }
    case ACTION_Attack:
    {
        User* Target = GConsole->GetSelectedActor();
        User* Me = GConsole->GetMyUser();
        if (Me && Target)
            GConsole->RequestAttack(Target->ObjectID, Me->Pawn->Location);
        break;
    }
    case ACTION_Exchange:
    {
        User* Target = GConsole->GetSelectedActor();
        if (Target && Target->Pawn)
            GConsole->RequestTrade(Target->Pawn->CreatureID);
        break;
    }
    case ACTION_NextTarget:
    {
        User* Target = GConsole->GetSelectedActor();
        if (Target && Target->Pawn)
            GConsole->RequestTargetNext(200, Target->Pawn->CreatureID);
        else
            GConsole->RequestTargetNext(200, -1);
        break;
    }
    case ACTION_PickUp:
        GConsole->RequestPickUp(200, -1);
        break;
    case ACTION_PetPickUp:
        GConsole->PetPickUp(200, -1);
        break;
    case ACTION_Assist:
        GConsole->RequestAssist();
        break;
    case ACTION_Invite:
    {
        User* Target = GConsole->GetSelectedActor();
        if (Target && !Target->Unknown8)
        {
            L2ParamStack Params(10);
            Params.PushBack((void*)(INT_PTR)Target->Pawn->CreatureID);
            Params.PushBack((void*)(INT_PTR)GConsole->PartyItemDistribution);
            GConsole->RequestPartyInvite(Params);
        }
        else
            ChatSystemMessage(185);
        break;
    }
    case ACTION_LeaveParty:
    {
        L2ParamStack Params(10);
        GConsole->RequestPartyLeave(Params);
        break;
    }
    case ACTION_Dismiss:
    {
        User* Target = GConsole->GetSelectedActor();
        if (Target && !Target->Unknown8)
        {
            L2ParamStack Params(10);
            Params.PushBack(Target->GetName());
            GConsole->RequestPartyDismiss(Params);
        }
        else
            ChatSystemMessage(187);
        break;
    }
    case ACTION_PrivateStoreSell:
        GConsole->RequestVendor();
        break;
    case ACTION_PrivateStoreBuy:
        GConsole->RequestBuy();
        break;
    case ACTION_PartyMatching:
        GDesktop->TogglePartyMatch();
        break;
    case ACTION_Greeting: GConsole->RequestSocialAction(2); break;
    case ACTION_Victory: GConsole->RequestSocialAction(3); break;
    case ACTION_Advance: GConsole->RequestSocialAction(4); break;
    case ACTION_SocialYes: GConsole->RequestSocialAction(6); break;
    case ACTION_SocialNo: GConsole->RequestSocialAction(5); break;
    case ACTION_SocialBow: GConsole->RequestSocialAction(7); break;
    case ACTION_SocialUnaware: GConsole->RequestSocialAction(8); break;
    case ACTION_SocialWaiting: GConsole->RequestSocialAction(9); break;
    case ACTION_SocialLaugh: GConsole->RequestSocialAction(10); break;
    }
}

// a skill: its id, Ctrl (force) and Shift (don't move) (repeated inline in OnLButtonUp and
// OnShortCutKeyDown)
static FORCEINLINE void UseSkill(INT SkillID)
{
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)SkillID);
    Params.PushBack((void*)(INT_PTR)GConsole->IsCtrlDown());
    Params.PushBack((void*)(INT_PTR)GConsole->Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
    GConsole->RequestMagicSkillUse(Params);
}

// 0x100a9a20 (vslot 57) - released on the pressed slot: it is used (an item always from the
// player's inventory, even a pet's); dropped off the bar: deleted on the server. PressedSlot
// is kept but for a deletion.
INT NCShortCutWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    bPressed = 0;
    if (PressedSlot >= 0)
    {
        if (!GConsole->IsCtrlDown())
        {
            UnlockCursor();
            SetCursor(0, NULL);
        }
        if (SlotAt(X, Y) == PressedSlot && Items[PressedSlot].bUsed)
        {
            NShortCutItem& Item = Items[PressedSlot];
            if (Item.Type == SHORTCUT_Item)
            {
                if (Item.ID == ITEM_Map1 || Item.ID == ITEM_Map2)
                    GConsole->ShowMapItem(Item.ID);
                else
                    GConsole->RequestUseItem(Item.ObjectID);
            }
            else if (Item.Type == SHORTCUT_Skill)
                UseSkill(Items[PressedSlot].ID);
            else if (Item.Type == SHORTCUT_Action)
                UseAction(Item.ID);
        }
        else if (bDragging && !IsInWindow(X, Y))
        {
            L2ParamStack Params(10);
            Params.PushBack((void*)(INT_PTR)PressedSlot);
            GConsole->RequestShortCutDel(Params);
            Items[PressedSlot].Clear();
            PressedSlot = -1;
        }
    }
    bDragging = 0;
#if NW_FEATURES
    // phase 6: AutoShot (NWindow_C1 src/inject/AutoShot.cpp): dropped on another slot, the
    // shortcut moves there
    nwf::AutoShot_OnLButtonUp(this, lParam);
#endif
    return 1;
}
NW_HOOK_METHOD(0x100a9a20, NCShortCutWnd, OnLButtonUp)

#if NW_FEATURES
// phase 6: AutoShot (NWindow_C1 src/inject/AutoShot.cpp): a right click on a shot toggles its
// automatic use (not stock: NCWnd's no-op otherwise, which a 0 from AutoShot stays)
INT NCShortCutWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    return nwf::AutoShot_OnRButtonDown(this, wParam, lParam);
}
#endif

// 0x100aaa80 (vslot 63)
INT NCShortCutWnd::OnNextClicked()
{
    bDragging = 0;
    NextPage();
    return 1;
}
NW_HOOK_METHOD(0x100aaa80, NCShortCutWnd, OnNextClicked)

// 0x100aaaa0 (vslot 64)
INT NCShortCutWnd::OnPrevClicked()
{
    bDragging = 0;
    PrevPage();
    return 1;
}
NW_HOOK_METHOD(0x100aaaa0, NCShortCutWnd, OnPrevClicked)

// 0x100a88b0 (vslot 65)
INT NCShortCutWnd::OnRotateClicked()
{
    bDragging = 0;
    bHorizontal = !bHorizontal;
    UpdateLayout();
    return 1;
}
NW_HOOK_METHOD(0x100a88b0, NCShortCutWnd, OnRotateClicked)

// 0x100a8350 - an inventory item changed: its slots show the new enchant or count; slots
// whose item is gone are cleared
INT NCShortCutWnd::UpdateItem(INT ObjectID)
{
    for (INT i = 0; i < Cols * Pages; i++)
    {
        NShortCutItem& Item = Items[i];
        if (Item.bUsed && Item.Type == SHORTCUT_Item && Item.ObjectID == ObjectID)
        {
            NItemEntry* Found = FindInventoryItem(ObjectID);
            if (Found)
            {
                if (Found->Enchant > 0)
                {
                    Items[i].Enchant = Found->Enchant;
                    appSprintf(Items[i].DisplayName, TEXT("+%d %s"), Items[i].Enchant,
                               Items[i].Name);
                }
                else if (Found->IsStackable())
                {
                    Items[i].Count = Found->Count;
                    appSprintf(Items[i].DisplayName, TEXT("%s(%d)"), Items[i].Name,
                               Items[i].Count);
                }
            }
            else
                Items[i].Clear();
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100a8350, NCShortCutWnd, UpdateItem)

// 0x100a84a0 - the same for the pet's inventory
INT NCShortCutWnd::UpdatePetItem(INT ObjectID)
{
    for (INT i = 0; i < Cols * Pages; i++)
    {
        NShortCutItem& Item = Items[i];
        if (Item.bUsed && Item.Type == SHORTCUT_Item && Item.ObjectID == ObjectID)
        {
            NItemEntry* Found = FindPetInventoryItem(ObjectID);
            if (Found)
            {
                if (Found->Enchant > 0)
                {
                    Items[i].Enchant = Found->Enchant;
                    appSprintf(Items[i].DisplayName, TEXT("+%d %s"), Items[i].Enchant,
                               Items[i].Name);
                }
                else if (Found->IsStackable())
                {
                    Items[i].Count = Found->Count;
                    appSprintf(Items[i].DisplayName, TEXT("%s(%d)"), Items[i].Name,
                               Items[i].Count);
                }
            }
            else
                Items[i].Clear();
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100a84a0, NCShortCutWnd, UpdatePetItem)

// 0x100a85f0 - the slot of the current page under a screen point: icons IconSize wide from
// 31 on, 5 apart, at 6 across the bar
INT NCShortCutWnd::SlotAt(INT X, INT Y)
{
    INT Slot = -1;
    if (!bHorizontal)
    {
        INT RX = X - ScreenX;
        INT RY = Y - ScreenY;
        INT Top = 31;
        if (RX < 6 || RX > IconSize + 6)
            return -1;
        for (INT i = 0; i < Cols; i++)
        {
            if (RY >= Top && RY <= IconSize + Top)
                Slot = Page * Cols + i;
            Top += IconSize + 5;
        }
    }
    else
    {
        INT RX = X - ScreenX;
        INT RY = Y - ScreenY;
        INT Left = 31;
        if (RY < 6 || RY > IconSize + 6)
            return -1;
        for (INT i = 0; i < Cols; i++)
        {
            if (RX >= Left && RX <= IconSize + Left)
                Slot = Page * Cols + i;
            Left += IconSize + 5;
        }
    }
    return Slot;
}
NW_HOOK_METHOD(0x100a85f0, NCShortCutWnd, SlotAt)

// 0x100a86e0 - the bar turned: the new size keeps the other corner, the buttons and the
// background follow
void NCShortCutWnd::UpdateLayout()
{
    INT W = Width;
    INT H = Height;
    if (!bHorizontal)
    {
        INT NewX = ScreenX - H + W;
        INT NewY = ScreenY - W + H;
        if (NewY < 0)
            NewY = 0;
        SetWindowRect(NewX, NewY, 44, 417);
        SetSize(44, 417);
        FrameCtrl->SetWindowRect(0, 0, 44, 11);
        BackTex = FindTexture(TEXT("L2UI.ShortcutWnd.Back"));
        NextButton->SetWindowRect(31, 11, 12, 12);
        PrevButton->SetWindowRect(1, 11, 12, 12);
        RotateButton->SetWindowRect(16, 403, 12, 12);
        NextButton->SetNormalTex(TEXT("L2UI.ShortcutWnd.RightBtn1"));
        NextButton->SetPushedTex(TEXT("L2UI.ShortcutWnd.RightBtn2"));
        PrevButton->SetNormalTex(TEXT("L2UI.ShortcutWnd.LeftBtn1"));
        PrevButton->SetPushedTex(TEXT("L2UI.ShortcutWnd.LeftBtn2"));
    }
    else
    {
        INT NewX = ScreenX - H + W;
        INT NewY = ScreenY - W + H;
        if (NewX < 0)
            NewX = 0;
        SetWindowRect(NewX, NewY, 417, 44);
        SetSize(417, 44);
        FrameCtrl->SetWindowRect(0, 0, 11, 44);
        BackTex = FindTexture(TEXT("L2UI.ShortcutWnd.Back1"));
        PrevButton->SetWindowRect(11, 31, 12, 12);
        NextButton->SetWindowRect(11, 1, 12, 12);
        RotateButton->SetWindowRect(403, 16, 12, 12);
        PrevButton->SetNormalTex(TEXT("L2UI.ShortcutWnd.DownBtn1"));
        PrevButton->SetPushedTex(TEXT("L2UI.ShortcutWnd.DownBtn2"));
        NextButton->SetNormalTex(TEXT("L2UI.ShortcutWnd.UpBtn1"));
        NextButton->SetPushedTex(TEXT("L2UI.ShortcutWnd.UpBtn2"));
    }
}
NW_HOOK_METHOD(0x100a86e0, NCShortCutWnd, UpdateLayout)

// 0x100a88e0 - after a language change: the names from the new tables, the labels too (an
// item's enchant or count label comes back when it changes)
void NCShortCutWnd::ReloadNames()
{
    for (INT i = 0; i < Cols * Pages; i++)
    {
        if (!Items[i].bUsed)
            continue;
        if (Items[i].Type == SHORTCUT_Item && GConsole->GetItemName(Items[i].ID))
        {
            appStrcpy(Items[i].Name, GConsole->GetItemName(Items[i].ID));
            appStrcpy(Items[i].DisplayName, GConsole->GetItemName(Items[i].ID));
        }
        else if (Items[i].Type == SHORTCUT_Skill &&
                 GConsole->GetSkillName(Items[i].ID, Items[i].Level))
        {
            appStrcpy(Items[i].Name, GConsole->GetSkillName(Items[i].ID, Items[i].Level));
            appStrcpy(Items[i].DisplayName, GConsole->GetSkillName(Items[i].ID, Items[i].Level));
        }
        else if (Items[i].Type == SHORTCUT_Action && GConsole->GetActionName(Items[i].ID))
        {
            appStrcpy(Items[i].Name, GConsole->GetActionName(Items[i].ID));
            appStrcpy(Items[i].DisplayName, GConsole->GetActionName(Items[i].ID));
        }
    }
}
NW_HOOK_METHOD(0x100a88e0, NCShortCutWnd, ReloadNames)

// 0x100a8ae0 - the slot of the current page's Index
INT NCShortCutWnd::PageSlot(INT Index)
{
    if (!Page)
        return Index;
    return Cols * Page + Index;
}
NW_HOOK_METHOD(0x100a8ae0, NCShortCutWnd, PageSlot)

// 0x100aa0d0
INT NCShortCutWnd::DeleteShortCut(INT Slot)
{
    if (Slot >= Pages * Cols)
        return 0;
    Items[Slot].Clear();
    Keys[Slot] = NShortCutKey();
    return 1;
}
NW_HOOK_METHOD(0x100aa0d0, NCShortCutWnd, DeleteShortCut)

// 0x100aa130 - leaving a page makes its keys ready again
void NCShortCutWnd::SetPage(INT NewPage)
{
    if (NewPage < 0 || NewPage >= Pages)
        return;
    if (NewPage != Page)
    {
        for (INT i = 0; i < Cols; i++)
        {
            Keys[Page * Cols + i].bReady = 1;
            Keys[Page * Cols + i].Elapsed = 0;
        }
    }
    Page = NewPage;
}
NW_HOOK_METHOD(0x100aa130, NCShortCutWnd, SetPage)

// 0x100aa1a0 - the keys waiting for a cool time or a fixed time become ready
void NCShortCutWnd::Tick(FLOAT DeltaTime)
{
    INT Ms = (INT)(DeltaTime * 1000.f);
    INT Count = Pages * Cols;
    for (INT i = 0; i < Count; i++)
    {
        if (Keys[i].bOnCoolTime)
        {
            if (Items[i].CoolTime.IsIdle() && Keys[i].bOnCoolTime && !Keys[i].bReady)
                Keys[i].bReady = 1;
        }
        else if (!Keys[i].bOnKeyUp && !Keys[i].bOnCoolTime && !Keys[i].bReady)
        {
            Keys[i].Elapsed += Ms;
            if (Keys[i].Elapsed > Keys[i].Interval)
            {
                Keys[i].Elapsed = 0;
                Keys[i].bReady = 1;
            }
        }
    }
}
NW_HOOK_METHOD(0x100aa1a0, NCShortCutWnd, Tick)

// the slot a key uses on the current page: F1 .. F10, or 1 .. 9, 0 while the chat line needs
// Enter (and no edit box has the focus); -1 for other keys (repeated inline in both key
// handlers)
static FORCEINLINE INT KeySlot(NCShortCutWnd* Wnd, UINT Key)
{
    INT Slot = -1;
    if (Key >= VK_F1 && Key <= VK_F10)
        Slot = Wnd->Page * Wnd->Cols + Key - VK_F1;
    if (GIsEnterChatting && (!GFocusEntry || !GFocusEntry->Wnd->vfn032()))
    {
        if (Key == '0')
            Slot = Wnd->Page * Wnd->Cols + 9;
        else if (Key > '0' && Key <= '9')
            Slot = Wnd->Page * Wnd->Cols + Key - '1';
    }
    return Slot;
}

// 0x100aa260 - a released key makes its slot ready when it waits for that
INT NCShortCutWnd::OnShortCutKeyUp(UINT Key, LPARAM lParam)
{
    INT Slot = KeySlot(this, Key);
    if (Slot >= 0 && Keys[Slot].bOnKeyUp && !Keys[Slot].bReady)
        Keys[Slot].bReady = 1;
    return 0;
}
NW_HOOK_METHOD(0x100aa260, NCShortCutWnd, OnShortCutKeyUp)

// 0x100aa310 - a pressed key uses its slot when ready (an empty slot beeps); 1 when the key
// was taken
INT NCShortCutWnd::OnShortCutKeyDown(UINT Key, LPARAM lParam)
{
    INT Slot = KeySlot(this, Key);
    if (Slot < 0)
        return 0;
    if (!Keys[Slot].bReady)
        return 1;
    INT Count = Pages * Cols;
    if (Slot < Count && Items[Slot].bUsed)
    {
        NShortCutItem& Item = Items[Slot];
        if (Item.Type == SHORTCUT_Item)
        {
            if (Item.ID == ITEM_Map1 || Item.ID == ITEM_Map2)
                GConsole->ShowMapItem(Item.ID);
            else if (Item.Owner == 1)
                GConsole->RequestUseItem(Item.ObjectID);
            else if (Item.Owner == 2)
                GConsole->RequestPetUseItem(Item.ObjectID);
            Keys[Slot].bReady = 0;
            return 1;
        }
        if (Item.Type == SHORTCUT_Skill)
        {
            UseSkill(Items[Slot].ID);
            Keys[Slot].bReady = 0;
            return 1;
        }
        if (Item.Type == SHORTCUT_Action)
        {
            UseAction(Item.ID);
            Keys[Slot].bReady = 0;
            return 1;
        }
        return 0;
    }
    if (Slot < Count && !Items[Slot].bUsed)
        GConsole->Engine->OnInterfacePlaySound(2);
    return 0;
}
NW_HOOK_METHOD(0x100aa310, NCShortCutWnd, OnShortCutKeyDown)

// the slots of the page just shown stop flashing (repeated inline in NextPage and PrevPage)
static FORCEINLINE void StopPageFlashes(NCShortCutWnd* Wnd)
{
    INT First = Wnd->Page * Wnd->Cols;
    for (INT i = 0; i < Wnd->Cols; i++)
        Wnd->Items[First + i].CoolTime.bFlashing = 0;
}

// 0x100aa9a0 - after the last page back to the first (page 9 fixed)
void NCShortCutWnd::NextPage()
{
    if (Page < Pages - 1)
        Page++;
    else if (Page == 9)
        Page = 0;
    StopPageFlashes(this);
}
NW_HOOK_METHOD(0x100aa9a0, NCShortCutWnd, NextPage)

// 0x100aaa10 - before the first page to page 9
void NCShortCutWnd::PrevPage()
{
    if (Page > 0)
        Page--;
    else if (Page == 0)
        Page = 9;
    StopPageFlashes(this);
}
NW_HOOK_METHOD(0x100aaa10, NCShortCutWnd, PrevPage)

// 0x100aaac0
void NCShortCutWnd::ClearShortCuts()
{
    for (INT i = 0; i < Cols * Pages; i++)
        DeleteShortCut(i);
}
NW_HOOK_METHOD(0x100aaac0, NCShortCutWnd, ClearShortCuts)

// 0x100aaf30 - the server's shortcut (its slot: NItemInfo +0xC90, not range-checked below 0;
// the type: +0xC0C) into its slot, with the label and the key's wait: items until released
// (shots every 200 ms), skills until the cool time is over (toggles until released), actions
// until released (attack every 200 ms, pick up every 500 ms)
INT NCShortCutWnd::SetShortCut(NItemInfo* Info, INT Owner)
{
    if (Info->UnknownC90 >= Pages * Cols)
        return 0;
    const TCHAR* IconName = Info->IconName;
    if (IconName)
        Info->Icon = FindTexture(IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NShortCutItem* Item = new NShortCutItem(*Info);
    Item->Owner = Owner;
    Item->CoolTime.Stop();
    switch (Info->Param)
    {
    case SHORTCUT_Item:
        if (Item->Enchant > 0)
            appSprintf(Item->DisplayName, TEXT("+%d %s"), Item->Enchant, Item->Name);
        else if (Item->IsStackable())
            appSprintf(Item->DisplayName, TEXT("%s(%d)"), Item->Name, Item->Count);
        else
            appSprintf(Item->DisplayName, TEXT("%s"), Item->Name);
        if (GConsole->IsShotItem(Item->ID))
            Keys[Info->UnknownC90].Interval = 200;
        else
            Keys[Info->UnknownC90].bOnKeyUp = 1;
        break;
    case SHORTCUT_Skill:
        if (Item->MpConsume > 0)
            appSprintf(Item->DisplayName, TEXT("%s(MP:%d)"), Item->Name, Item->MpConsume);
        else
            appSprintf(Item->DisplayName, TEXT("%s"), Item->Name);
        if (GConsole->IsToggleSkill(Item->ID, Item->Level))
            Keys[Info->UnknownC90].bOnKeyUp = 1;
        else
            Keys[Info->UnknownC90].bOnCoolTime = 1;
        break;
    case SHORTCUT_Action:
        appSprintf(Item->DisplayName, TEXT("%s"), Item->Name);
        if (Item->ID == ACTION_Attack)
            Keys[Info->UnknownC90].Interval = 200;
        else if (Item->ID == ACTION_PickUp)
            Keys[Info->UnknownC90].Interval = 500;
        else
            Keys[Info->UnknownC90].bOnKeyUp = 1;
        break;
    }
    Items[Info->UnknownC90].CopyFrom(Item);
    Items[Info->UnknownC90].bUsed = 1;
    delete Item;
    return 1;
}
NW_HOOK_METHOD(0x100aaf30, NCShortCutWnd, SetShortCut)

NW_CHECK_VTABLE(NCShortCutWnd, 0x100ee5f0, 66)
