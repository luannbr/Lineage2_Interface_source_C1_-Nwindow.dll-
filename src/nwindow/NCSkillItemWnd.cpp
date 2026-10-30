/*=============================================================================
    NCSkillItemWnd.cpp - the skill grids of the skill windows. Stock: 0x10071fc0 ..
    0x10073155, 0x100737e0 .. 0x10073a32, 0x10073a40 .. 0x10073aeb, 0x10073770 ..
    0x100737d9, 0x1005fe80 .. 0x1005fedf, and NCGMSkillItemWnd's 0x1005bd80 .. 0x1005bdd1,
    0x10060400 .. 0x100604c7.
=============================================================================*/
#include "nwindow/NCSkillItemWnd.h"
#include "nwindow/NCCoolTimeIcon.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NConsoleWnd.h"

enum { SOUND_CoolTimeEnd = 9 };  // interface sound of a cool time ending (name ours)

/*-----------------------------------------------------------------------------
    NCSkillItemWnd
-----------------------------------------------------------------------------*/

// 0x100739a0
NCSkillItemWnd::NCSkillItemWnd()
{
    bPressed = 0;
    IconBackTex = FindTexture(TEXT("l2ui.nwindow.icon_back"));
    IconClickTex = FindTexture(TEXT("l2ui.nwindow.icon_click"));
    ToggleTex = FindTexture(TEXT("L2_SkillTime.ToggleEffect.ToggleEffect001"));
}
NW_HOOK_CTOR(0x100739a0, NCSkillItemWnd)

// 0x10073a40 - the skill window's are 6 x 7 icons of 32 pixels, 80 at most
NCSkillItemWnd::NCSkillItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
    bPressed = 0;
    IconBackTex = FindTexture(TEXT("l2ui.nwindow.icon_back"));
    IconClickTex = FindTexture(TEXT("l2ui.nwindow.icon_click"));
    ToggleTex = FindTexture(TEXT("L2_SkillTime.ToggleEffect.ToggleEffect001"));
}
NW_HOOK_CTOR_ARGS(0x10073a40, NCSkillItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x1005fe80 - CoolTimes' destructor is TArray<NCCoolTimeIcon*>'s (0x1005fce0)
NW_HOOK_DTOR(0x1005fe80, NCSkillItemWnd)

// 0x10071fc0 (vslot 24)
INT NCSkillItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCItemWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x10071fc0, NCSkillItemWnd, WindowProc)

// the cool time icons go (repeated inline in the stock OnDestroy and Clear)
static FORCEINLINE void DeleteCoolTimes(TArray<NCCoolTimeIcon*>& CoolTimes)
{
    for (INT i = 0; i < CoolTimes.Num(); i++)
        if (CoolTimes(i))
            delete CoolTimes(i);
    CoolTimes.Empty();
}

// 0x10073770 (vslot 36)
INT NCSkillItemWnd::OnDestroy()
{
    DeleteCoolTimes(CoolTimes);
    NCItemWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10073770, NCSkillItemWnd, OnDestroy)

// 0x100737e0 (vslot 65) - the items are deleted here and NCItemWnd's finds none left
void NCSkillItemWnd::Clear()
{
    for (INT i = 0; i < Items.Num(); i++)
        if (Items(i))
            delete Items(i);
    Items.Empty();
    DeleteCoolTimes(CoolTimes);
    NCItemWnd::Clear();
}
NW_HOOK_METHOD(0x100737e0, NCSkillItemWnd, Clear)

// 0x10073890 - from NCSkillWnd::AddSkill; MaxItems is not checked. The skill's cool time icon
// is added stopped (the console starts it).
INT NCSkillItemWnd::AddSkill(NItemInfo* Info)
{
    const TCHAR* IconName = Info->IconName;
    if (IconName)
        Info->Icon = FindTexture(IconName);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"));
    NItemEntry* Item = new NItemEntry(Info);
    CoolTimes.AddItem(new NCCoolTimeIcon);
    SetItemToolTip(Item);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x10073890, NCSkillItemWnd, AddSkill)

// 0x10072030 - the box text, whose lines size the box: name, type ("Active Skill", "Passive
// Skill" or "Magic"), "MP cost : n" when above 0, "Range : n" when not negative, description.
// Left as it was when the "MP cost" or "Range" string is missing. The level is NItemEntry
// +0xC18; the skill data is asked again for each test and value, as in the stock code.
void NCSkillItemWnd::SetItemToolTip(NItemEntry* Item)
{
    if (!Item)
        return;
    FString Desc;
    GConsole->ConvertNewLines((TCHAR*)Item->GetDescription(), Desc);
    if (*GL2GameData.SysString[320] && *GL2GameData.SysString[321])  // "MP cost", "Range"
    {
        if (GConsole->GetSkillRange(Item->ID, Item->UnknownC18) >= 0 &&
            GConsole->GetSkillMpCost(Item->ID, Item->UnknownC18) > 0)
            appSprintf(Item->ToolTipText, TEXT("%s\n%s\n%s : %d\n%s : %d\n%s"), Item->GetName(),
                       GConsole->GetSkillTypeText(Item->ID, Item->UnknownC18),
                       *GL2GameData.SysString[320], GConsole->GetSkillMpCost(Item->ID, Item->UnknownC18),
                       *GL2GameData.SysString[321], GConsole->GetSkillRange(Item->ID, Item->UnknownC18), *Desc);
        else if (GConsole->GetSkillRange(Item->ID, Item->UnknownC18) >= 0 &&
                 GConsole->GetSkillMpCost(Item->ID, Item->UnknownC18) <= 0)
            appSprintf(Item->ToolTipText, TEXT("%s\n%s\n%s : %d\n%s"), Item->GetName(),
                       GConsole->GetSkillTypeText(Item->ID, Item->UnknownC18),
                       *GL2GameData.SysString[321], GConsole->GetSkillRange(Item->ID, Item->UnknownC18), *Desc);
        else if (GConsole->GetSkillRange(Item->ID, Item->UnknownC18) < 0 &&
                 GConsole->GetSkillMpCost(Item->ID, Item->UnknownC18) > 0)
            appSprintf(Item->ToolTipText, TEXT("%s\n%s\n%s : %d\n%s"), Item->GetName(),
                       GConsole->GetSkillTypeText(Item->ID, Item->UnknownC18),
                       *GL2GameData.SysString[320], GConsole->GetSkillMpCost(Item->ID, Item->UnknownC18),
                       *Desc);
        else
            appSprintf(Item->ToolTipText, TEXT("%s\n%s\n%s"), Item->GetName(),
                       GConsole->GetSkillTypeText(Item->ID, Item->UnknownC18), *Desc);
    }
}
NW_HOOK_METHOD(0x10072030, NCSkillItemWnd, SetItemToolTip)

// "label : " (the label a sysstring, empty when missing) in grey, the colon in white; returns
// the X of the value, past "label : " (repeated inline for each such line of the stock code)
static FORCEINLINE INT DrawLabel(NCGDevice* Device, INT X, INT Y, TCHAR* Text, INT LabelID)
{
    SIZE Size;
    if (*GL2GameData.SysString[LabelID])
        appSprintf(Text, TEXT("%s"), *GL2GameData.SysString[LabelID]);
    else
        Text[0] = 0;
    Device->DrawText(X, Y, 0xFFA3A3A3, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT(" : "));
    Device->DrawText(X + Size.cx, Y, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (*GL2GameData.SysString[LabelID])
        appSprintf(Text, TEXT("%s : "), *GL2GameData.SysString[LabelID]);
    else
        Text[0] = 0;
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    return X + Size.cx;
}

// 0x10072360 (vslot 67) - lines 13 pixels apart: the name, "Lv" and the level after it; the
// type; "MP cost : n" when not 0 (a negative cost would show); "Range : n" when not negative
// (a range of 0 leaves the number out); the description wrapped at 168 pixels
void NCSkillItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
    if (!Item)
        return;
    FString Desc;
    GConsole->ConvertNewLines((TCHAR*)Item->GetDescription(), Desc);
    TCHAR Text[1024];
    SIZE Size;
    if (Item->GetName())
        appSprintf(Text, TEXT("%s"), Item->GetName());
    else
        Text[0] = 0;
    Device->DrawText(X, Y, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    INT LevelX = X + Size.cx + 6;
    if (*GL2GameData.SysString[88])                                  // "Lv"
        appSprintf(Text, TEXT("%s"), *GL2GameData.SysString[88]);
    else
        Text[0] = 0;
    Device->DrawText(LevelX, Y, 0xFFA3A3A3, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    LevelX += Size.cx + 2;
    appSprintf(Text, TEXT("%d"), Item->UnknownC18);
    Device->DrawText(LevelX, Y, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Y += 13;
    if (GConsole->GetSkillTypeText(Item->ID, Item->UnknownC18))
        appSprintf(Text, TEXT("%s"), GConsole->GetSkillTypeText(Item->ID, Item->UnknownC18));
    else
        Text[0] = 0;
    Device->DrawText(X, Y, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Y += 13;
    if (GConsole->GetSkillMpCost(Item->ID, Item->UnknownC18))
    {
        INT ValueX = DrawLabel(Device, X, Y, Text, 320);             // "MP cost"
        appSprintf(Text, TEXT("%d"), GConsole->GetSkillMpCost(Item->ID, Item->UnknownC18));
        Device->DrawText(ValueX, Y, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Y += 13;
    }
    if (GConsole->GetSkillRange(Item->ID, Item->UnknownC18) >= 0)
    {
        INT ValueX = DrawLabel(Device, X, Y, Text, 321);             // "Range"
        if (GConsole->GetSkillRange(Item->ID, Item->UnknownC18))
            appSprintf(Text, TEXT("%d"), GConsole->GetSkillRange(Item->ID, Item->UnknownC18));
        else
            Text[0] = 0;
        Device->DrawText(ValueX, Y, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Y += 13;
    }
    appSprintf(Text, TEXT("%s"), *Desc);
    Device->DrawText(X, Y, 0xFFB2BECF, Text, 168, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 1);
}
NW_HOOK_METHOD(0x10072360, NCSkillItemWnd, DrawItemToolTip)

// 0x100729b0 (vslot 50) - NCItemWnd's grid (its guard name too), with the selection frame the
// size of the icon and over each skill, in order: the icon frame of an active skill (NItemEntry
// Type 0) and the click frame of the pressed one, the toggle animation of a toggle skill whose
// effect is on the player, the end flash with its sound once, the cool time.
INT NCSkillItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCItemWnd::OnPaint);
    Device->SetClip(ScreenX - 1, ScreenY - 1, Width + 2, Height + 2);
    Device->BeginPaint(ScreenX, ScreenY);
    INT X = 0;
    INT Y = 0;
    for (INT i = TopRow * Columns; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (!Item)
            continue;
        if (i % Columns == 0)
        {
            X = 0;
            if (i != TopRow * Columns)
                Y += CellSize + 3;
            if (VisibleRows * (CellSize + 1) <= Y)
                break;
        }
        else
            X += CellSize + 3;
        if (Item->IsSelected())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, SelectTex);
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon());
        if (Item->Type == 0)
        {
            if (IconBackTex)
                Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, IconBackTex);
            if (IconClickTex && Selected == i && bPressed)
                Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, IconClickTex);
        }
        if (GConsole->IsToggleSkill(Item->ID, Item->UnknownC18) && GConsole->HasAbnormal(Item->ID))
            Device->DrawTexture(X, Y, 32, 32, 0, 0, 32, 32, ToggleTex);
        if (CoolTimes(i)->bFlashing)
        {
            Device->DrawBrightenTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize,
                                        CoolTimes(i)->GetFlashTexture(), 0xFF008080);
            if (CoolTimes(i)->bSoundDue)
            {
                GConsole->Engine->OnInterfacePlaySound(SOUND_CoolTimeEnd);
                CoolTimes(i)->bSoundDue = 0;
            }
        }
        if (CoolTimes(i)->bRunning && !CoolTimes(i)->IsIdle() && CoolTimes(i)->GetTexture())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize,
                                CoolTimes(i)->GetTexture());
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100729b0, NCSkillItemWnd, OnPaint)

// 0x10072cf0 (vslot 51) - NCItemWnd's box over any item with a tool tip text (not only the
// selected one; no name tip), the cells counted CellSize + 1 apart here and the strips 13
// pixels high whatever the font
INT NCSkillItemWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (HoverIndex >= 0 && HoverIndex < Items.Num())
    {
        INT Index = HoverIndex - TopRow * Columns;
        INT Y = (CellSize + 1) * (Index / Columns);
        INT X = (CellSize + 1) * (Index % Columns);
        if (Items(HoverIndex)->GetToolTipText())
        {
            SIZE Size;
            SIZE Char;
            NCGDevice::GetTextExtent(&Size, Items(HoverIndex)->GetToolTipText(), 168, 0, 0, 0, 0);
            NCGDevice::GetCharSize(&Char, 'A');
            INT TextH = Size.cy + Size.cy / Char.cy - 1;
            INT ScreenW = GConsole->Desktop->LocalW;
            if (ScreenX + X + 184 > ScreenW)
                X = ScreenW - ScreenX - 184;
            else if (ScreenX + X < 0)
                X = -ScreenX;
            Y += -15 - TextH;
            if (ScreenY + Y < 0)
                Y += CellSize + TextH + 16;
            Device->SetClip(ScreenX + X, ScreenY + Y, 184, TextH / Char.cy * 13 + 16);
            if (TipTopTex)
                Device->DrawTexture(X, Y, 184, 8, 0, 0, 184, 8, TipTopTex);
            INT Lines = TextH / Char.cy;
            for (INT i = 0; i < Lines; i++)
                if (TipMidTex)
                    Device->DrawTexture(X, i * 13 + Y + 8, 184, 13, 0, 0, 184, 13, TipMidTex);
            if (TipBottomTex)
                Device->DrawTexture(X, Lines * 13 + Y + 8, 184, 8, 0, 0, 184, 8, TipBottomTex);
            DrawItemToolTip(Device, Items(HoverIndex), X + 8, Y + 8);
            Device->ReleaseClip();
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10072cf0, NCSkillItemWnd, OnDrawToolTip)

// 0x10072000 (vslot 56)
INT NCSkillItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    NCItemWnd::OnLButtonDown(wParam, lParam);
    bPressed = 1;
    return 1;
}
NW_HOOK_METHOD(0x10072000, NCSkillItemWnd, OnLButtonDown)

// 0x10072f60 (vslot 57) - released on the icon clicked: the skill is used (the server gets
// its id, Ctrl and Shift); dropped on a slot of the shortcut bar: the server registers it
// there (type 2, then 1)
INT NCSkillItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    guard(NCSkillItemWnd::OnLButtonUp);
    bPressed = 0;
    if (!GConsole->IsCtrlDown())
    {
        UnlockCursor();
        SetCursor(0, NULL);
    }
    if (Selected >= 0 && Selected < Items.Num())
    {
        INT X = LOWORD(lParam);
        INT Y = HIWORD(lParam);
        if (HitTest(X, Y) == Selected)
        {
            L2ParamStack Params(10);
            Params.PushBack((void*)(INT_PTR)Items(Selected)->ID);
            Params.PushBack((void*)(INT_PTR)GConsole->IsCtrlDown());
            Params.PushBack(
                (void*)(INT_PTR)GConsole->Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
            GConsole->RequestMagicSkillUse(Params);
        }
        else if (bDragging)
        {
            NWndEntry* Target = FindWindowAt(X, Y);
            if (Target && Target->Wnd && Target->Wnd->IsA(NCShortCutWnd::StaticClass()))
            {
                INT Slot = static_cast<NCShortCutWnd*>(Target->Wnd)->SlotAt(X, Y);
                if (Slot >= 0)
                {
                    L2ParamStack Params(10);
                    Params.PushBack((void*)(INT_PTR)SHORTCUT_Skill);
                    Params.PushBack((void*)(INT_PTR)Slot);
                    Params.PushBack((void*)(INT_PTR)Items(Selected)->ID);
                    Params.PushBack((void*)1);
                    GConsole->RequestShortCutReg(Params);
                }
            }
        }
        UnselectAll();
        Selected = -1;
    }
    bDragging = 0;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10072f60, NCSkillItemWnd, OnLButtonUp)

NW_CHECK_VTABLE(NCSkillItemWnd, 0x100e4c90, 68)

/*-----------------------------------------------------------------------------
    NCGMSkillItemWnd
-----------------------------------------------------------------------------*/

// 0x10060400
NCGMSkillItemWnd::NCGMSkillItemWnd()
{
}
NW_HOOK_CTOR(0x10060400, NCGMSkillItemWnd)

// 0x100604a0
NCGMSkillItemWnd::NCGMSkillItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems,
                                   INT InCellSize)
    : NCSkillItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
}
NW_HOOK_CTOR_ARGS(0x100604a0, NCGMSkillItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x10060450
NW_HOOK_DTOR(0x10060450, NCGMSkillItemWnd)

// 0x1005bd80 (vslot 24)
INT NCGMSkillItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCSkillItemWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1005bd80, NCGMSkillItemWnd, WindowProc)

// 0x1005bdc0 (vslot 57) - the carried skill is only put down; also NCGMInvenItemWnd's slots
// 57, 58 and 68
INT NCGMSkillItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x1005bdc0, NCGMSkillItemWnd, OnLButtonUp)

NW_CHECK_VTABLE(NCGMSkillItemWnd, 0x100e5040, 68)
