/*=============================================================================
    NCActionItemWnd.cpp - a grid of action icons. Stock: 0x10033990 .. 0x10033d95 (but for
    the accessors the stock code shares with other structs), 0x100343e0 .. 0x10034d09,
    0x100368b0 .. 0x10036c64, 0x10036f00 .. 0x1003705f.
=============================================================================*/
#include "nwindow/NCActionItemWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NItemInfo.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NClasses.gen.h"

#if NW_FEATURES
// phase 6: ManufactureCmd (NWindow_C1 src/inject/ManufactureCmd.cpp)
#include "features/ManufactureCmd.h"
#endif

// the tool tip box: 184 pixels wide, text wrapped at 168
enum { TIP_Width = 184, TIP_TextWidth = 168 };

// the chat window
static NCChatWnd* ChatWnd()
{
    return GDesktop->ChatWnd;
}

/*-----------------------------------------------------------------------------
    NActionItem.
-----------------------------------------------------------------------------*/

// 0x10033990
NActionItem::NActionItem(const NItemInfo& Info)
{
    ActionID = -1;
    Param = -1;
    Icon = NULL;
    Name[0] = 0;
    Description[0] = 0;
    ToolTip[0] = 0;
    UnknownA0C[0] = 0;
    bSelected = 0;
    ActionID = Info.ID;
    Param = Info.Param;
    Icon = Info.Icon;
    appStrcpy(Name, Info.Name);
    const TCHAR* Src = Info.IconName;
    if (Src)
        appStrcpy(IconName, Src);
    Src = Info.Description;
    if (Src)
        appStrcpy(Description, Src);
}
NW_HOOK_CTOR_ARGS(0x10033990, NActionItem, (const NItemInfo& A), (A))

// 0x10033a60
TCHAR* NActionItem::GetDescription()
{
    TCHAR* S = Description;
    if (!S || !*S)
        return NULL;
    return S;
}
NW_HOOK_METHOD(0x10033a60, NActionItem, GetDescription)

// 0x10033a80
TCHAR* NActionItem::GetToolTip()
{
    TCHAR* S = ToolTip;
    if (!S || !*S)
        return NULL;
    return S;
}
NW_HOOK_METHOD(0x10033a80, NActionItem, GetToolTip)

// 0x10033aa0
void NActionItem::Select()
{
    bSelected = 1;
}
NW_HOOK_METHOD(0x10033aa0, NActionItem, Select)

// 0x10033ab0
void NActionItem::Deselect()
{
    bSelected = 0;
}
NW_HOOK_METHOD(0x10033ab0, NActionItem, Deselect)

/*-----------------------------------------------------------------------------
    NCActionItemWnd.
-----------------------------------------------------------------------------*/

// 0x10036f00 - the tool tip textures are left as allocated (OnCreate loads them)
NCActionItemWnd::NCActionItemWnd(INT InCols, INT InRows, INT InMaxItems, INT InIconSize)
{
    Cols = InCols;
    MaxItems = InMaxItems;
    Rows = InRows;
    PressedIndex = -1;
    HoverIndex = -1;
    ItemClickTex = NULL;
    ToolTipTex = NULL;
    IconSize = InIconSize;
    bDragging = 0;
    bPressed = 0;
    IconBackTex = FindTexture(TEXT("l2ui.nwindow.icon_back"));
    IconClickTex = FindTexture(TEXT("l2ui.nwindow.icon_click"));
}
NW_HOOK_CTOR_ARGS(0x10036f00, NCActionItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x10037000
NW_HOOK_DTOR(0x10037000, NCActionItemWnd)

// stock body shared with NControl's (0x10004e90)
INT NCActionItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10033ae0 (vslot 35)
INT NCActionItemWnd::OnCreate()
{
    NCWnd::OnCreate();
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    ItemClickTex = FindTexture(TEXT("l2ui.nwindow.item_click"));
    TipTopTex = FindTexture(TEXT("L2UI.Tooltip.Tool02"));
    TipMidTex = FindTexture(TEXT("L2UI.Tooltip.Tool01"));
    TipBottomTex = FindTexture(TEXT("L2UI.Tooltip.Tool03"));
    return 1;
}
NW_HOOK_METHOD(0x10033ae0, NCActionItemWnd, OnCreate)

// 0x10036af0 (vslot 36)
INT NCActionItemWnd::OnDestroy()
{
    for (INT i = 0; i < Items.Num(); i++)
        delete Items(i);
    Items.Empty();
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10036af0, NCActionItemWnd, OnDestroy)

// 0x100343e0 (vslot 50) - icons IconSize + 3 apart (HitTest counts IconSize + 4), each in its
// frame; the pressed one darkened. The clip is set before the origin, as in the stock code.
INT NCActionItemWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX - 1, ScreenY - 1, Width + 3, Height + 2);
    Device->BeginPaint(ScreenX, ScreenY);
    INT X = 0;
    INT Y = 1;
    for (INT i = 0; i < Items.Num(); i++)
    {
        NActionItem* Item = Items(i);
        if (!Item)
            continue;
        if (i % Cols == 0)
        {
            X = 1;
            if (i)
                Y += IconSize + 3;
        }
        else
            X += IconSize + 3;
        if (Item->GetIcon())
        {
            Device->DrawTexture(X, Y, IconSize, IconSize, 0, 0, IconSize, IconSize,
                                Item->GetIcon());
            Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, IconBackTex);
            if (PressedIndex == i && bPressed)
                Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, IconClickTex);
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100343e0, NCActionItemWnd, OnPaint)

// 0x10034550 (vslot 51) - the box goes above the icon (below when there is no room), kept on
// the screen; its height counts text lines by the height of 'A'
INT NCActionItemWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (HoverIndex >= 0 && HoverIndex < Items.Num())
    {
        INT X = (IconSize + 4) * (HoverIndex % Cols);
        INT Y = (IconSize + 1) * (HoverIndex / Cols);
        NActionItem* Item = Items(HoverIndex);
        if (Item->GetToolTip())
        {
            SIZE Size;
            NCGDevice::GetTextExtent(&Size, Item->GetToolTip(), TIP_TextWidth, 0, 0, 0, 0);
            SIZE CharSize;
            NCGDevice::GetCharSize(&CharSize, 'A');
            Size.cy += Size.cy / CharSize.cy - 1;
            INT DesktopW = GConsole->Desktop->LocalW;
            if (ScreenX + X + TIP_Width > DesktopW)
                X = DesktopW - ScreenX - TIP_Width;
            else if (ScreenX + X < 0)
                X = -ScreenX;
            Y += -15 - Size.cy;
            if (ScreenY + Y < 0)
                Y += IconSize + Size.cy + 16;
            Device->SetClip(ScreenX + X, ScreenY + Y, TIP_Width, Size.cy / CharSize.cy * 13 + 16);
            if (TipTopTex)
                Device->DrawTexture(X, Y, TIP_Width, 8, 0, 0, TIP_Width, 8, TipTopTex);
            INT Strips = Size.cy / CharSize.cy;
            for (INT k = 0; k < Strips; k++)
            {
                if (TipMidTex)
                    Device->DrawTexture(X, Y + 8 + k * 13, TIP_Width, 13, 0, 0, TIP_Width, 13,
                                        TipMidTex);
            }
            if (TipBottomTex)
                Device->DrawTexture(X, Strips * 13 + Y + 8, TIP_Width, 8, 0, 0, TIP_Width, 8,
                                    TipBottomTex);
            DrawItemToolTip(Device, Items(HoverIndex), X + 8, Y + 8);
            Device->ReleaseClip();
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10034550, NCActionItemWnd, OnDrawToolTip)

// 0x100347a0 (vslot 55) - while an icon is dragged the cursor carries it
INT NCActionItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    HoverIndex = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (IsFocused())
    {
        if (bDragging)
        {
            if (PressedIndex >= 0 && PressedIndex < Items.Num())
            {
                SetCursor(4, Items(PressedIndex)->GetIcon());
                LockCursor();
                return 1;
            }
        }
        else if (!GConsole->IsCtrlDown())
        {
            UnlockCursor();
            SetCursor(0, 0);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100347a0, NCActionItemWnd, OnMouseMove)

// 0x100368b0 (vslot 56) - selects the icon and starts a drag (sound 3)
INT NCActionItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Index = HitTest(LOWORD(lParam), HIWORD(lParam));
    PressedIndex = Index;
    HoverIndex = Index;
    if (Index >= 0 && Index < Items.Num())
    {
        ClearSelection();
        Items(PressedIndex)->Select();
        bDragging = 1;
        bPressed = 1;
        GConsole->Engine->OnInterfacePlaySound(3);
    }
    return 1;
}
NW_HOOK_METHOD(0x100368b0, NCActionItemWnd, OnLButtonDown)

// 0x10036930 (vslot 57) - released on the same icon: the action runs; dropped on a slot of
// the shortcut bar: the server registers it there (type 3, then 1 for actions 0..14, 2 for
// 15..27, nothing for the others)
INT NCActionItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    bPressed = 0;
    if (!GConsole->IsCtrlDown())
    {
        UnlockCursor();
        SetCursor(0, 0);
    }
    INT Index = PressedIndex;
    if (Index >= 0 && Index < Items.Num())
    {
        INT X = LOWORD(lParam);
        INT Y = HIWORD(lParam);
        if (HitTest(X, Y) == Index)
            UseAction();
        else if (bDragging)
        {
            NActionItem* Item = Items(Index);
            if (Item)
            {
                NWndEntry* Target = FindWindowAt(X, Y);
                if (Target && Target->Wnd && Target->Wnd->IsA(NClass::NCShortCutWnd()))
                {
                    INT Slot = static_cast<NCShortCutWnd*>(Target->Wnd)->SlotAt(X, Y);
                    if (Slot >= 0)
                    {
                        L2ParamStack Params(10);
                        Params.PushBack((void*)3);
                        Params.PushBack((void*)(INT_PTR)Slot);
                        Params.PushBack((void*)(INT_PTR)Item->GetActionID());
                        if (Item->GetActionID() >= 0 && Item->GetActionID() <= 14)
                            Params.PushBack((void*)1);
                        else if (Item->GetActionID() >= 15 && Item->GetActionID() <= 27)
                            Params.PushBack((void*)2);
                        GConsole->RequestShortCutReg(Params);
                    }
                }
            }
        }
        ClearSelection();
        PressedIndex = -1;
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10036930, NCActionItemWnd, OnLButtonUp)

// a party request on someone who is not a player shows a system message in the chat
static FORCEINLINE void ChatSystemMessage(INT MsgID)
{
    ChatWnd()->Lists[CHATLIST_All]->AddString(*GL2GameData.SystemMsg[MsgID].Message, 0xFFB09B79,
                                              -1);
}

// 0x10034840 (vslot 62) - the pressed icon's action (no bounds check on PressedIndex, as in
// the stock code); ranges 200
void NCActionItemWnd::UseAction()
{
    NActionItem* Item = Items(PressedIndex);
    if (!Item)
        return;
#if NW_FEATURES
    // phase 6: ManufactureCmd (NWindow_C1 src/inject/ManufactureCmd.cpp): ids 32 .. 52, past
    // the stock switch (37 Dwarven Manufacture, 52 servitor Return)
    if (nwf::ManufactureCmd_UseAction(Item->GetActionID(), 1))
        return;
#endif
    switch (Item->GetActionID())
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
        Params.PushBack((void*)(INT_PTR)Item->GetActionID());
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
    case ACTION_PartyMatching:
        GDesktop->TogglePartyMatch();
        break;
    case ACTION_Greeting:
    case ACTION_Victory:
    case ACTION_Advance:
    case ACTION_SocialYes:
    case ACTION_SocialNo:
    case ACTION_SocialBow:
    case ACTION_SocialUnaware:
    case ACTION_SocialWaiting:
    case ACTION_SocialLaugh:
        GConsole->RequestSocialAction(Item->GetParam());
        break;
    case ACTION_PetPickUp:
        GConsole->PetPickUp(200, -1);
        break;
    case ACTION_PrivateStoreBuy:
        GConsole->RequestBuy();
        break;
    }
}
NW_HOOK_METHOD(0x10034840, NCActionItemWnd, UseAction)

// 0x10033c70 (vslot 63) - the name, then the description (its escaped newlines converted)
// wrapped at 168 pixels
void NCActionItemWnd::DrawItemToolTip(NCGDevice* Device, NActionItem* Item, INT X, INT Y)
{
    if (!Item)
        return;
    FString Text;
    GConsole->ConvertNewLines(Item->GetDescription(), Text);
    TCHAR Buffer[1024];
    appSprintf(Buffer, TEXT("%s"), Item->GetName());
    Device->DrawText(X, Y, 0xFFFFFFFF, Buffer, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Y += 13;
    appSprintf(Buffer, TEXT("%s"), *Text);
    Device->DrawText(X, Y, 0xFFB2BECF, Buffer, TIP_TextWidth, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                     1);
}
NW_HOOK_METHOD(0x10033c70, NCActionItemWnd, DrawItemToolTip)

// 0x10033b50 - the icon under a screen point (cells IconSize + 4 wide)
INT NCActionItemWnd::HitTest(INT X, INT Y)
{
    INT RX = X - ScreenX;
    INT RY = Y - ScreenY;
    if (RX < 0 || RX > Cols * (IconSize + 4) - 1 || RY < 0 || RY > Rows * (IconSize + 4) - 1)
        return -1;
    INT Col = RX / (IconSize + 4);
    if (Col >= Cols)
        Col--;
    INT Row = RY / (IconSize + 4);
    if (Row >= Rows)
        Row--;
    return Cols * Row + Col;
}
NW_HOOK_METHOD(0x10033b50, NCActionItemWnd, HitTest)

// 0x10033bd0 - "name\ndescription" for the tool tip
void NCActionItemWnd::SetItemToolTip(NActionItem* Item)
{
    if (Item)
    {
        FString Text;
        GConsole->ConvertNewLines(Item->GetDescription(), Text);
        appSprintf(Item->ToolTip, TEXT("%s\n%s"), Item->GetName(), *Text);
    }
}
NW_HOOK_METHOD(0x10033bd0, NCActionItemWnd, SetItemToolTip)

// 0x10034ce0
void NCActionItemWnd::ClearSelection()
{
    for (INT i = 0; i < Items.Num(); i++)
        Items(i)->Deselect();
}
NW_HOOK_METHOD(0x10034ce0, NCActionItemWnd, ClearSelection)

// 0x10036b60 - a full grid refuses the icon
INT NCActionItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= MaxItems)
        return 0;
    NActionItem* Item = new NActionItem(*Info);
    SetItemToolTip(Item);
    Items.AddItem(Item);
    return 1;
}
NW_HOOK_METHOD(0x10036b60, NCActionItemWnd, AddItem)

// 0x10036c10
void NCActionItemWnd::ClearItems()
{
    for (INT i = 0; i < Items.Num(); i++)
        delete Items(i);
    Items.Empty();
}
NW_HOOK_METHOD(0x10036c10, NCActionItemWnd, ClearItems)

// the accessors, bodies the linker folded with other structs' getters (NItemEntry, NString,
// NLineList, NShortCutItem...)
NW_HOOK_METHOD(0x10033a40, NActionItem, GetIcon)
NW_HOOK_METHOD(0x10033a50, NActionItem, GetName)
NW_HOOK_METHOD(0x10033ac0, NActionItem, GetActionID)
NW_HOOK_METHOD(0x10033ad0, NActionItem, GetParam)

NW_CHECK_VTABLE(NCActionItemWnd, 0x100dfce8, 64)
