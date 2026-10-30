/*=============================================================================
    NCStatusWnd.cpp - the HP / MP windows: base, player, target, party member, pet, summon.
    Stock: 0x100ae300 .. 0x100ae9ac, 0x100aeb70 .. 0x100aebe3, 0x100af2b0 .. 0x100b16e0.
=============================================================================*/
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCProgressCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <stdio.h>
#include <wchar.h>

// exp at the start of each level (index = level), a table of the stock image
#if NW_HOOK_MODE
#define GExpTable (reinterpret_cast<const INT*>(::nw::Addr(0x100eed2c)))
#else
extern const INT GExpTable[];
#endif

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_WindowClose = 6 };

// title bar ids of the status windows, the summon's time bar
enum
{
    NID_PLAYERSTATUS_FRAME = 0xFFB000,
    NID_TARGETSTATUS_FRAME = 0xF8,
    NID_PETSTATUS_FRAME = 0xFFA0B1,
    NID_SUMMONSTATUS_FRAME = 0xFFA0B2,
    NID_SUMMONSTATUS_TIME = 0xF0A0B0,
};

// the name when it is not empty, else NULL (inlined at each use in the stock paints)
static FORCEINLINE const TCHAR* NonEmpty(const TCHAR* Text)
{
    return Text && *Text ? Text : NULL;
}

/*-----------------------------------------------------------------------------
    NCStatusWnd.
-----------------------------------------------------------------------------*/

// 0x100af2b0 - the other textures are left as allocated
NCStatusWnd::NCStatusWnd(INT InBarWidth)
{
    MPTex = NULL;
    HPTex = NULL;
    EXPTex = NULL;
    BackTex = NULL;
    BarWidth = InBarWidth;
}
NW_HOOK_CTOR_ARGS(0x100af2b0, NCStatusWnd, (INT A), (A))

// 0x100aeb70
NW_HOOK_DTOR(0x100aeb70, NCStatusWnd)

// stock body shared with NControl's (0x10004e90)
INT NCStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100af2f0 (vslot 35)
INT NCStatusWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("NWindow.WhiteTexture"));
    Clear();
    return 1;
}
NW_HOOK_METHOD(0x100af2f0, NCStatusWnd, OnCreate)

// stock body shared with NCCompositeBox's (0x10083930)
INT NCStatusWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    return 1;
}

// 0x100af320 (vslot 50) - draws nothing
INT NCStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100af320, NCStatusWnd, OnPaint)

// 0x100ae300
UBOOL NCStatusWnd::IsName(const TCHAR* InName)
{
    if (!InName || !Name[0])
        return 0;
    return wcscmp(Name, InName) == 0;
}
NW_HOOK_METHOD(0x100ae300, NCStatusWnd, IsName)

// 0x100ae330
void NCStatusWnd::Clear()
{
    Name[0] = 0;
    HP = 0;
    MaxHP = 0;
    MP = 0;
    MaxMP = 0;
}
NW_HOOK_METHOD(0x100ae330, NCStatusWnd, Clear)

// 0x100ae9a0
UBOOL NCStatusWnd::IsEmpty()
{
    return Name[0] == 0;
}
NW_HOOK_METHOD(0x100ae9a0, NCStatusWnd, IsEmpty)

/*-----------------------------------------------------------------------------
    NCPlayerStatusWnd.
-----------------------------------------------------------------------------*/

// 0x100aff90
NCPlayerStatusWnd::NCPlayerStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    Unknown300 = 0;
    ExtraWidth = 0;
}
NW_HOOK_CTOR_ARGS(0x100aff90, NCPlayerStatusWnd, (INT A), (A))

// 0x100afff0
NW_HOOK_DTOR(0x100afff0, NCPlayerStatusWnd)

// 0x100ae5b0 (vslot 24) - the right edge reports how far it was dragged
INT NCPlayerStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCStatusWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == NID_PLAYERSTATUS_RESIZE)
        return OnResize((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100ae5b0, NCPlayerStatusWnd, WindowProc)

// 0x100b0040 (vslot 35) - a title bar on the left, the resize edge on the right
INT NCPlayerStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    MaxExtraWidth = 202;

    FrameCtrl = new NCFrameCtrl(0);
    FrameCtrl->ID = NID_PLAYERSTATUS_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 12, 71);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    ResizeFrame = new NCResizeFrame(RESIZE_Horizontal);
    ResizeFrame->ID = NID_PLAYERSTATUS_RESIZE;
    ResizeFrame->SetWindowRect(Width - 10, 0, 10, Height);
    CreateChild(ResizeFrame, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown

    BackTex = FindTexture(TEXT("L2UI.PlayerStatusWnd.PlayerStatusBack"));
    HPTex = FindTexture(TEXT("L2UI.PlayerStatusWnd.HP"));
    MPTex = FindTexture(TEXT("L2UI.PlayerStatusWnd.MP"));
    EXPTex = FindTexture(TEXT("L2UI.PlayerStatusWnd.EXP"));
    LeftBackTex = FindTexture(TEXT("L2UI.PlayerStatusWnd.LeftBack"));
    RightBackTex = FindTexture(TEXT("L2UI.PlayerStatusWnd.RightBack"));
    return 1;
}
NW_HOOK_METHOD(0x100b0040, NCPlayerStatusWnd, OnCreate)

// 0x100b0970 (vslot 36)
INT NCPlayerStatusWnd::OnDestroy()
{
    NCStatusWnd::OnDestroy();
    if (ResizeFrame)
    {
        RemoveChildWindow(ResizeFrame->Entry);
        delete ResizeFrame;
        ResizeFrame = NULL;
    }
    return 1;
}
NW_HOOK_METHOD(0x100b0970, NCPlayerStatusWnd, OnDestroy)

// 0x100b0200 (vslot 50) - bars 128 + ExtraWidth pixels wide (their texture is 119 texels);
// exp in percent with two decimals, HP and MP as "now / max". A level of two digits is drawn
// digit by digit.
INT NCPlayerStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPlayerStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (LeftBackTex)
        Device->DrawTexture(0, 0, 36, 71, 0, 0, 36, 71, LeftBackTex);
    if (BackTex)
        Device->DrawTexture(36, 0, ExtraWidth + 106, 71, 1, 0, 95, 71, BackTex);
    if (RightBackTex)
        Device->DrawTexture(ExtraWidth + 142, 0, 15, 71, 0, 0, 15, 71, RightBackTex);
    Player = GConsole->GetMyUser();
    if (Player)
    {
        INT BarW = ExtraWidth + 128;
        INT HPW = 0;
        INT MPW = 0;
        INT EXPW = 0;
        INT HPU = 0;
        INT MPU = 0;
        INT EXPU = 0;
        if (Player->MaxHP)
            HPW = (INT)((DOUBLE)Player->HP * BarW / Player->MaxHP);
        if (Player->MaxMP)
            MPW = (INT)((DOUBLE)Player->MP * BarW / Player->MaxMP);
        INT NextExp = GExpTable[Player->Level + 1];
        if (NextExp)
            EXPW = (INT)((DOUBLE)(Player->Exp - GExpTable[Player->Level]) * BarW /
                         (NextExp - GExpTable[Player->Level]));
        if (Player->MaxHP)
            HPU = (INT)(Player->HP * 119.0 / Player->MaxHP);
        if (Player->MaxMP)
            MPU = (INT)(Player->MP * 119.0 / Player->MaxMP);
        if (NextExp)
            EXPU = (INT)((DOUBLE)(Player->Exp - GExpTable[Player->Level]) * 119.0 /
                         (NextExp - GExpTable[Player->Level]));

        if (HPW > 0 && HPW <= BarW)
            Device->DrawTexture(14, 25, HPW, 10, 0, 0, HPU, 10, HPTex);
        else if (HPW > BarW)
            Device->DrawTexture(14, 25, BarW, 10, 0, 0, 119, 10, HPTex);
        if (MPW > 0 && MPW <= BarW)
            Device->DrawTexture(14, 38, MPW, 10, 0, 0, MPU, 10, MPTex);
        else if (MPW > BarW)
            Device->DrawTexture(14, 38, BarW, 10, 0, 0, 119, 10, MPTex);
        if (EXPW > 0 && EXPW <= BarW)
            Device->DrawTexture(14, 52, EXPW, 10, 0, 0, EXPU, 10, EXPTex);
        else if (EXPW > BarW)
            Device->DrawTexture(14, 52, BarW, 10, 0, 0, 119, 10, EXPTex);

        TCHAR Buf[64];
        SIZE Size;
        INT LevelExp = GExpTable[Player->Level];
        swprintf(Buf, ARRAY_COUNT(Buf), TEXT("%.2f%s"),
                 (DOUBLE)(Player->Exp - LevelExp) * 100.0 / (GExpTable[Player->Level + 1] - LevelExp),
                 TEXT("%"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, BarW / 2 - Size.cx / 2 + 12, 53, 0xFFFFFFFF, Buf);

        appSprintf(Buf, TEXT("%s"), Player->GetName());
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawText(45, 8, 0xFFFFFFFF, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);

        if (Player->Level <= 9)
        {
            appSprintf(Buf, TEXT("%d"), Player->Level);
            NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
            Device->DrawSpecialDigit(1, 25 - Size.cx / 2, 9, 0xFFB09B79, Buf);
        }
        else
        {
            appSprintf(Buf, TEXT("%d"), Player->Level);
            TCHAR Digit[2];
            Digit[0] = Buf[0];
            Digit[1] = 0;
            Device->DrawSpecialDigit(1, 19, 9, 0xFFB09B79, Digit);
            Digit[0] = Buf[1];
            Digit[1] = 0;
            Device->DrawSpecialDigit(1, 25, 9, 0xFFB09B79, Digit);
        }

        // "HP  now / max", the slash centered in the free width
        Device->DrawSpecialDigit(1, 25, 26, 0xFFFFFFFF, (TCHAR*)TEXT("HP"));
        appSprintf(Buf, TEXT("%s"), TEXT("/"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, (ExtraWidth + 84) / 2 - Size.cx / 2 + 37, 26, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%d"), Player->HP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, (ExtraWidth + 84) / 2 - Size.cx + 29, 26, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%d"), Player->MaxHP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, (ExtraWidth + 84) / 2 + 41, 26, 0xFFFFFFFF, Buf);

        Device->DrawSpecialDigit(1, 25, 39, 0xFFFFFFFF, (TCHAR*)TEXT("MP"));
        appSprintf(Buf, TEXT("%s"), TEXT("/"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, (ExtraWidth + 84) / 2 - Size.cx / 2 + 37, 39, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%d"), Player->MP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, (ExtraWidth + 84) / 2 - Size.cx + 29, 39, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%d"), Player->MaxMP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, (ExtraWidth + 84) / 2 + 41, 39, 0xFFFFFFFF, Buf);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b0200, NCPlayerStatusWnd, OnPaint)

// 0x100ae630 (vslot 56) - targets the player
INT NCPlayerStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    GConsole->TargetMyself();
    return 1;
}
NW_HOOK_METHOD(0x100ae630, NCPlayerStatusWnd, OnLButtonDown)

// 0x100b09b0 (vslot 62) - widens by Delta within 0 .. MaxExtraWidth extra pixels
INT NCPlayerStatusWnd::OnResize(INT Delta)
{
    INT W = LocalW;
    INT H = LocalH;
    if (Delta > 0 && ExtraWidth < MaxExtraWidth)
    {
        ExtraWidth += Delta;
        if (ExtraWidth > MaxExtraWidth)
        {
            Delta += MaxExtraWidth - ExtraWidth;
            ExtraWidth = MaxExtraWidth;
        }
    }
    else if (Delta < 0 && ExtraWidth > 0)
    {
        ExtraWidth += Delta;
        if (ExtraWidth < 0)
        {
            Delta -= ExtraWidth;
            ExtraWidth = 0;
        }
    }
    else
        return 1;
    SetSize(W + Delta, H);
    ResizeFrame->SetWindowRect(Width - ResizeFrame->LocalW, 0, ResizeFrame->LocalW,
                               ResizeFrame->LocalH);
    return 1;
}
NW_HOOK_METHOD(0x100b09b0, NCPlayerStatusWnd, OnResize)

// 0x100ae620 - the console restores the saved width with it
INT NCPlayerStatusWnd::Resize(INT Delta)
{
    return OnResize(Delta);
}
NW_HOOK_METHOD(0x100ae620, NCPlayerStatusWnd, Resize)

NW_CHECK_VTABLE(NCPlayerStatusWnd, 0x100ef6d8, 63)

/*-----------------------------------------------------------------------------
    NCTargetStatusWnd.
-----------------------------------------------------------------------------*/

// 0x100af3d0
NCTargetStatusWnd::NCTargetStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    LevelDiff = 0;
    bExpanded = 0;
}
NW_HOOK_CTOR_ARGS(0x100af3d0, NCTargetStatusWnd, (INT A), (A))

// 0x100af430
NW_HOOK_DTOR(0x100af430, NCTargetStatusWnd)

// 0x100ae360 (vslot 24)
INT NCTargetStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCStatusWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_TARGETSTATUS_CLOSE)
            return OnClose() != 0;
        if ((INT)wParam == NID_TARGETSTATUS_TOGGLE)
            return OnToggle() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100ae360, NCTargetStatusWnd, WindowProc)

// 0x100af480 (vslot 35)
INT NCTargetStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    BarWidth = 132;

    FrameCtrl = new NCFrameCtrl(0);
    FrameID = NID_TARGETSTATUS_FRAME;
    FrameCtrl->ID = NID_TARGETSTATUS_FRAME;
    FrameCtrl->SetWindowRect(0, 11, 12, Height - 11);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    BackTopTex = FindTexture(TEXT("L2UI.TargetWnd.TargetBackTop"));
    BackCenterTex = FindTexture(TEXT("L2UI.TargetWnd.TargetBackCenter"));
    BackBottomTex = FindTexture(TEXT("L2UI.TargetWnd.TargetBackBottom"));
    FrameTopTex = FindTexture(TEXT("L2UI.TargetWnd.TargetFrameTop"));
    FrameCenterTex = FindTexture(TEXT("L2UI.TargetWnd.TargetFrameCenter"));
    FrameBottomTex = FindTexture(TEXT("L2UI.TargetWnd.TargetFrameBottom"));
    UpBtnTex = FindTexture(TEXT("L2UI.TargetWnd.TargetUpBtn"));
    UpBtnClickTex = FindTexture(TEXT("L2UI.TargetWnd.TargetUpBtn_click"));
    DownBtnTex = FindTexture(TEXT("L2UI.TargetWnd.TargetDownBtn"));
    DownBtnClickTex = FindTexture(TEXT("L2UI.TargetWnd.TargetDownBtn_click"));
    HPTex = FindTexture(TEXT("L2UI.TargetWnd.MiniHP1"));
    MPTex = FindTexture(TEXT("L2UI.TargetWnd.MiniMP1"));
    HPBackTex = FindTexture(TEXT("L2UI.TargetWnd.MiniHP2"));
    MPBackTex = FindTexture(TEXT("L2UI.TargetWnd.MiniMP2"));

    CloseButton = new NCButton(TEXT("L2UI.TargetWnd.TargetCloseButton"),
                               TEXT("L2UI.TargetWnd.TargetCloseButton_click"), 0);
    CloseButton->ID = NID_TARGETSTATUS_CLOSE;
    CloseButton->SetWindowRect(Width - 18, 5, 12, 12);
    CreateChild(CloseButton, NWS_VISIBLE);

    ToggleButton = new NCButton(TEXT("L2UI.TargetWnd.TargetDownBtn"),
                                TEXT("L2UI.TargetWnd.TargetDownBtn_click"), 0);
    ToggleButton->ID = NID_TARGETSTATUS_TOGGLE;
    ToggleButton->SetWindowRect(3, 4, 7, 7);
    CreateChild(ToggleButton, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100af480, NCTargetStatusWnd, OnCreate)

// 0x100af780 (vslot 36) - the toggle button is not deleted here, as in the stock code
INT NCTargetStatusWnd::OnDestroy()
{
    if (CloseButton)
    {
        RemoveChildWindow(CloseButton->Entry);
        delete CloseButton;
        CloseButton = NULL;
    }
    NCStatusWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100af780, NCTargetStatusWnd, OnDestroy)

// inlined twice in the stock OnPaint: back to one half height, the toggle button hidden
FORCEINLINE void NCTargetStatusWnd::Collapse()
{
    if (bExpanded)
    {
        ToggleButton->SetNormalTex(TEXT("L2UI.TargetWnd.TargetDownBtn"));
        ToggleButton->SetPushedTex(TEXT("L2UI.TargetWnd.TargetDownBtn_click"));
        SetSize(Width, (DWORD)Height >> 1);
        FrameCtrl->SetWindowRect(0, 11, 12, Height - 11);
        bExpanded = 0;
    }
    ToggleButton->EnableWindow(0);
    ToggleButton->HideWindow();
}

// 0x100af7c0 (vslot 50) - HP / MP bars for monsters, the player, its pet and siege NPCs;
// the name colored by LevelDiff; for players the clan and alliance lines. Without a target
// the window hides itself, leaving the clip and the origin set, as in the stock code. The
// "clan" / "alliance" labels are Korean literals of the stock code.
INT NCTargetStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCTargetStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTopTex)
        Device->DrawTexture(12, 0, Width - 12, 8, 0, 0, 140, 8, BackTopTex);
    if (BackCenterTex)
        Device->DrawTexture(12, 8, Width - 12, Height - 16, 0, 0, 140, 8, BackCenterTex);
    if (BackBottomTex)
        Device->DrawTexture(12, Height - 8, Width - 12, 8, 0, 0, 140, 8, BackBottomTex);
    if (FrameTopTex)
        Device->DrawTexture(0, 0, 12, 8, 0, 0, 12, 8, FrameTopTex);
    if (FrameCenterTex)
        Device->DrawTexture(0, 7, 12, Height - 13, 0, 0, 12, 16, FrameCenterTex);
    if (FrameBottomTex)
        Device->DrawTexture(0, Height - 7, 12, 7, 0, 0, 12, 7, FrameBottomTex);

    User* Target = GConsole->GetSelectedActor();
    User* Me = GConsole->GetMyUser();
    AActor* StaticActor = GConsole->GetSelectedStaticActor();
    if (!Target)
    {
        if (!StaticActor)
        {
            HideWindow();
            return 1;
        }
        // a door or other static object: its name only
        FServerStaticObject* Info = GConsole->FindStaticObjectInfo(StaticActor->CreatureID);
        if (Info)
        {
            TCHAR* ObjectName = GL2GameData.GetStaticObjectName(Info->StaticObjectID);
            if (ObjectName)
            {
                SIZE Size;
                NCGDevice::GetTextExtent(&Size, ObjectName, 0, 0, 0, 0, 0);
                Device->DrawText(((DWORD)(Width - 30) >> 1) - Size.cx / 2 + 12, 5, 0xFFFFFFFF,
                                 ObjectName, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            }
            Collapse();
        }
        Device->EndPaint();
        Device->ReleaseClip();
        return 1;
    }

    UBOOL bShowHP = 0;
    if (Target->Unknown8 && !Target->Unknown0C && Target->Unknown10)
        bShowHP = 1;
    else if (Me && Target == Me)
        bShowHP = 1;
    else if (Target->Unknown8 && Target->Unknown0C &&
             Target->ObjectID == GConsole->GetPetInfo().ObjectID)
        bShowHP = 1;
    else if (Target->Unknown8 && GConsole->IsSiegeNpc(Target->NpcID))
        bShowHP = 1;

    SIZE Size;
    if (bShowHP)
    {
        INT HPW = 0;
        INT MPW = 0;
        if (Target->MaxHP)
            HPW = (INT)((DOUBLE)BarWidth * Target->HP / Target->MaxHP);
        if (Target->MaxMP)
            MPW = (INT)((DOUBLE)BarWidth * Target->MP / Target->MaxMP);
        if (HPW >= BarWidth)
            HPW = BarWidth;
        if (MPW >= BarWidth)
            MPW = BarWidth;
        NCGDevice::GetTextExtent(&Size, Target->GetName(), 0, 0, 0, 0, 0);
        // the player's level minus the target's: red when the target is much stronger,
        // blue when much weaker
        DWORD Color;
        if (LevelDiff <= -9)
            Color = 0xFFFF0000;
        else if (LevelDiff <= -6)
            Color = 0xFFFF9191;
        else if (LevelDiff <= -3)
            Color = 0xFFFAFE91;
        else if (LevelDiff <= 2)
            Color = 0xFFFFFFFF;
        else if (LevelDiff <= 5)
            Color = 0xFFA2FFAB;
        else if (LevelDiff <= 8)
            Color = 0xFFA2A8FC;
        else
            Color = 0xFF0000FF;
        Device->DrawText(((DWORD)(Width - 30) >> 1) - Size.cx / 2 + 12, 5, Color,
                         Target->GetName(), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        if (HPW >= 0)
        {
            Device->DrawTexture(16, 20, BarWidth, 5, 0, 0, 106, 5, HPBackTex);
            Device->DrawTexture(16, 20, HPW, 5, 0, 0, 106, 5, HPTex);
        }
        if (MPW >= 0)
        {
            Device->DrawTexture(16, 27, BarWidth, 5, 0, 0, 106, 5, MPBackTex);
            Device->DrawTexture(16, 27, MPW, 5, 0, 0, 106, 5, MPTex);
        }
    }
    else
    {
        NCGDevice::GetTextExtent(&Size, Target->GetName(), 0, 0, 0, 0, 0);
        Device->DrawText(((DWORD)(Width - 30) >> 1) - Size.cx / 2 + 12, 5, 0xFFFFFFFF,
                         Target->GetName(), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }

    if (Target->Unknown8)
    {
        // not a player: no clan lines
        Collapse();
        Device->EndPaint();
        Device->ReleaseClip();
        return 1;
    }

    Device->DrawText(15, 39, 0xFFA3A3A3, TEXT("\xD608\xB9F9"), 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                     0, 0, 0);
    Device->DrawText(15, 55, 0xFFA3A3A3, TEXT("\xB3D9\xB9F9"), 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                     0, 0, 0);
    ToggleButton->EnableWindow(1);
    ToggleButton->ShowWindow();
    if (bExpanded)
    {
        UBOOL bCrest = 0;
        UBOOL bAllyCrest = 0;
        PledgeInfo* Pledge = GConsole->GetPledgeInfo(Target->PledgeID);
        if (Pledge)
        {
            if (Pledge->CrestTex)
            {
                Device->DrawPledgeCrest(40, 39, Target->PledgeID);
                bCrest = 1;
            }
            const TCHAR* PledgeName = Pledge->Name;
            if (PledgeName && *PledgeName)
                Device->DrawText(bCrest ? 58 : 40, 39, 0xFFB09B79, PledgeName, 0, 0, 0,
                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            if (Pledge->AllyCrestTex)
            {
                Device->DrawAllyCrest(48, 55, Target->PledgeID);
                bAllyCrest = 1;
            }
            const TCHAR* AllyName = Pledge->AllyName;
            if (AllyName && *AllyName)
                Device->DrawText(bAllyCrest ? 58 : 40, 55, 0xFFB09B79, AllyName, 0, 0, 0,
                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            else
                Device->DrawText(40, 55, 0xFF808080, *GL2GameData.SysString[591], 0, 0, 0,
                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);   // "No alliance"
        }
        else
        {
            Device->DrawText(40, 39, 0xFF808080, *GL2GameData.SysString[431], 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);       // "No Clan"
            Device->DrawText(40, 55, 0xFF808080, *GL2GameData.SysString[591], 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100af7c0, NCTargetStatusWnd, OnPaint)

// 0x100ae3f0 (vslot 56) - targets again; with Ctrl, attacks
INT NCTargetStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        FVector Origin = Me->Pawn->Location;
        if (GConsole->IsCtrlDown())
            GConsole->RequestAttack(ObjectID, Origin);
        else
            GConsole->RequestTarget(ObjectID);
    }
    return 1;
}
NW_HOOK_METHOD(0x100ae3f0, NCTargetStatusWnd, OnLButtonDown)

// 0x100ae530 (vslot 62) - the close button: the server drops the target
INT NCTargetStatusWnd::OnClose()
{
    L2ParamStack Params(10);
    GConsole->RequestTargetCancel(Params);
    GConsole->Engine->OnInterfacePlaySound(SOUND_WindowClose);
    return 1;
}
NW_HOOK_METHOD(0x100ae530, NCTargetStatusWnd, OnClose)

// 0x100ae490 (vslot 63) - doubles or halves the height for the clan and alliance lines
INT NCTargetStatusWnd::OnToggle()
{
    INT H;
    if (bExpanded)
    {
        ToggleButton->SetNormalTex(TEXT("L2UI.TargetWnd.TargetDownBtn"));
        ToggleButton->SetPushedTex(TEXT("L2UI.TargetWnd.TargetDownBtn_click"));
        H = (DWORD)Height >> 1;
    }
    else
    {
        ToggleButton->SetNormalTex(TEXT("L2UI.TargetWnd.TargetUpBtn"));
        ToggleButton->SetPushedTex(TEXT("L2UI.TargetWnd.TargetUpBtn_click"));
        H = Height * 2;
    }
    SetSize(Width, H);
    FrameCtrl->SetWindowRect(0, 11, 12, Height - 11);
    bExpanded = !bExpanded;
    return 1;
}
NW_HOOK_METHOD(0x100ae490, NCTargetStatusWnd, OnToggle)

// 0x100ae480 - from the console with each new target
void NCTargetStatusWnd::SetLevelDiff(INT Diff)
{
    LevelDiff = Diff;
}
NW_HOOK_METHOD(0x100ae480, NCTargetStatusWnd, SetLevelDiff)

NW_CHECK_VTABLE(NCTargetStatusWnd, 0x100ef2b0, 64)

/*-----------------------------------------------------------------------------
    NCPartyStatusWnd.
-----------------------------------------------------------------------------*/

// 0x100b13e0
NCPartyStatusWnd::NCPartyStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    MemberID = -1;
}
NW_HOOK_CTOR_ARGS(0x100b13e0, NCPartyStatusWnd, (INT A), (A))

// 0x100b1440
NW_HOOK_DTOR(0x100b1440, NCPartyStatusWnd)

// 0x100b1490 (vslot 35)
INT NCPartyStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.TargetWnd.PartyBack"));
    HPTex = FindTexture(TEXT("L2UI.TargetWnd.MiniHP1"));
    MPTex = FindTexture(TEXT("L2UI.TargetWnd.MiniMP1"));
    HPBackTex = FindTexture(TEXT("L2UI.TargetWnd.MiniHP2"));
    MPBackTex = FindTexture(TEXT("L2UI.TargetWnd.MiniMP2"));
    return 1;
}
NW_HOOK_METHOD(0x100b1490, NCPartyStatusWnd, OnCreate)

// 0x100b1500 (vslot 50) - bars 132 pixels wide whatever BarWidth says; HP and MP above the
// maximum are clamped in place. With a maximum of 0 the stock code computes no width: the HP
// one lives in the Device argument's stack slot ([ebp+8], written only when MaxHP != 0), so
// that bar is as wide as the NCGDevice pointer - positive, it passes the "> 0" test and the
// bar is drawn full, cut by the clip at the window's right edge - reproduced; the MP one
// ([ebp-0x14], written only when MaxMP != 0) is a local left unwritten, stack garbage - not
// reproducible, 0 here.
INT NCPartyStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPartyStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawStretchedTex(0, 0, Width, Height, 0, 0, 140, 36, BackTex);
    const TCHAR* Text = Name;
    if (Text && *Text)
    {
        INT HPW = (INT)(INT_PTR)Device;  // the stock value with MaxHP 0 (see above)
        INT MPW = 0;                     // the stock code reads stack garbage here
        if (MaxHP)
        {
            if (HP >= MaxHP)
                HP = MaxHP;
            HPW = HP * 132 / MaxHP;
        }
        if (MaxMP)
        {
            if (MP >= MaxMP)
                MP = MaxMP;
            MPW = MP * 132 / MaxMP;
        }
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
        Device->DrawText(((DWORD)Width >> 1) - Size.cx / 2, 5, 0xFFFFFFFF, Text, 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Device->DrawTexture(4, 20, 132, 5, 0, 0, 106, 5, HPBackTex);
        if (HPW > 0)
            Device->DrawTexture(4, 20, HPW, 5, 0, 0, 106, 5, HPTex);
        Device->DrawTexture(4, 27, 132, 5, 0, 0, 106, 5, MPBackTex);
        if (MPW > 0)
            Device->DrawTexture(4, 27, MPW, 5, 0, 0, 106, 5, MPTex);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b1500, NCPartyStatusWnd, OnPaint)

// 0x100ae820 (vslot 56) - targets the member; with Ctrl, attacks it
INT NCPartyStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        FVector Origin = Me->Pawn->Location;
        if (GConsole->IsCtrlDown())
            GConsole->RequestAttack(MemberID, Origin);
        else
            GConsole->RequestTarget(MemberID);
    }
    return 1;
}
NW_HOOK_METHOD(0x100ae820, NCPartyStatusWnd, OnLButtonDown)

// 0x100ae8b0 (vslot 59) - targets what the member targets (signed coordinates here)
INT NCPartyStatusWnd::OnRButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (PtInRect((SWORD)LOWORD(lParam), (SWORD)HIWORD(lParam), ScreenX, ScreenY, Width, Height))
    {
        User* Me = GConsole->GetMyUser();
        if (Me)
            GConsole->AssistMember(MemberID, Me->Pawn->Location);
    }
    return 1;
}
NW_HOOK_METHOD(0x100ae8b0, NCPartyStatusWnd, OnRButtonUp)

// 0x100ae940 - nothing without a name
void NCPartyStatusWnd::SetInfo(INT InMemberID, const TCHAR* InName, INT InHP, INT InMaxHP,
                               INT InMP, INT InMaxMP)
{
    if (!InName)
        return;
    MemberID = InMemberID;
    wcscpy(Name, InName);
    HP = InHP;
    MaxHP = InMaxHP;
    MP = InMP;
    MaxMP = InMaxMP;
}
NW_HOOK_METHOD(0x100ae940, NCPartyStatusWnd, SetInfo)

NW_CHECK_VTABLE(NCStatusWnd, 0x100ef050, 62)
NW_CHECK_VTABLE(NCPartyStatusWnd, 0x100efc88, 62)

/*-----------------------------------------------------------------------------
    NCPetStatusWnd.
-----------------------------------------------------------------------------*/

// 0x100b0a50
NCPetStatusWnd::NCPetStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    Pet.Name[0] = 0;
    Pet.UnknownB0 = 0;
    Unknown3B8 = 0;
}
NW_HOOK_CTOR_ARGS(0x100b0a50, NCPetStatusWnd, (INT A), (A))

// 0x100b0ab0
NW_HOOK_DTOR(0x100b0ab0, NCPetStatusWnd)

// stock body shared with NCButton's (0x100051f0)
INT NCPetStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCStatusWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100b0b00 (vslot 35)
INT NCPetStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.PetWnd.PetStatusBack"));
    HPTex = FindTexture(TEXT("L2UI.TargetWnd.MiniHP1"));
    MPTex = FindTexture(TEXT("L2UI.TargetWnd.MiniMP1"));
    HPBackTex = FindTexture(TEXT("L2UI.TargetWnd.MiniHP2"));
    MPBackTex = FindTexture(TEXT("L2UI.TargetWnd.MiniMP2"));
    FedTex = FindTexture(TEXT("L2UI.PetWnd.PetGreenBar1"));
    FedBackTex = FindTexture(TEXT("L2UI.PetWnd.PetGreenBar2"));

    FrameCtrl = new NCFrameCtrl(0);
    FrameCtrl->SetWindowRect(0, 0, 12, 43);
    FrameCtrl->ID = NID_PETSTATUS_FRAME;
    CreateChild(FrameCtrl, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100b0b00, NCPetStatusWnd, OnCreate)

// 0x100ae730 (vslot 42) - hidden again unless the player has a pet
void NCPetStatusWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (GConsole->GetPetInfo().Type != 2)
        HideWindow();
}
NW_HOOK_METHOD(0x100ae730, NCPetStatusWnd, ShowWindow)

// 0x100b0c30 (vslot 50) - bars 132 pixels wide. The stock code leaves a width uninitialized
// when its maximum is 0 (a local holding whatever an earlier paint left: not reproducible);
// here it is 0.
INT NCPetStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPetStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    Pet = GConsole->GetPetInfo();
    INT HPW = 0;
    INT MPW = 0;
    INT FedW = 0;
    if (Pet.MaxHP)
        HPW = Pet.HP * 132 / Pet.MaxHP;
    if (Pet.MaxMP)
        MPW = Pet.MP * 132 / Pet.MaxMP;
    if (Pet.MaxFed)
        FedW = Pet.Fed * 132 / Pet.MaxFed;
    const TCHAR* Text = Pet.Name;
    if (Text && *Text)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, NonEmpty(Pet.Name), 0, 0, 0, 0, 0);
        Device->DrawText(((DWORD)(Width - 12) >> 1) - Size.cx / 2 + 12, 5, 0xFFFFFFFF,
                         NonEmpty(Pet.Name), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->DrawTexture(16, 20, 132, 5, 0, 0, 106, 5, HPBackTex);
    if (HPW > 0)
        Device->DrawTexture(16, 20, HPW, 5, 0, 0, 106, 5, HPTex);
    Device->DrawTexture(16, 27, 132, 5, 0, 0, 106, 5, MPBackTex);
    if (MPW > 0)
        Device->DrawTexture(16, 27, MPW, 5, 0, 0, 106, 5, MPTex);
    Device->DrawTexture(16, 34, 132, 5, 0, 0, 106, 5, FedBackTex);
    if (FedW > 0)
        Device->DrawTexture(16, 34, FedW, 5, 0, 0, 106, 5, FedTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b0c30, NCPetStatusWnd, OnPaint)

// 0x100ae6d0 (vslot 56) - right of the title bar: acts on the pet (as a click on it)
INT NCPetStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if ((SWORD)LOWORD(lParam) > ScreenX + 13)
    {
        User* Me = GConsole->GetMyUser();
        if (Me)
            GConsole->RequestAction(Pet.ObjectID, Me->Pawn->Location);
    }
    return 1;
}
NW_HOOK_METHOD(0x100ae6d0, NCPetStatusWnd, OnLButtonDown)

// 0x100ae650 - from the console (vslot 171); the paint reads the network handler's record
// instead of these
void NCPetStatusWnd::SetInfo(L2ParamStack& Params)
{
    ObjectID = (INT)Params.Top();
    TCHAR* InName = (TCHAR*)Params.Top();
    if (InName)
        wcscpy(Name, InName);
    HP = (INT)Params.Top();
    MaxHP = (INT)Params.Top();
    MP = (INT)Params.Top();
    MaxMP = (INT)Params.Top();
    Unknown3C4 = (INT)Params.Top();
    Unknown3C8 = (INT)Params.Top();
    Unknown3CC = (INT)Params.Top();
}
NW_HOOK_METHOD(0x100ae650, NCPetStatusWnd, SetInfo)

NW_CHECK_VTABLE(NCPetStatusWnd, 0x100ef988, 62)

/*-----------------------------------------------------------------------------
    NCSummonedStatusWnd.
-----------------------------------------------------------------------------*/

// 0x100b0ec0
NCSummonedStatusWnd::NCSummonedStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    Summon.Name[0] = 0;
    Summon.UnknownB0 = 0;
    Unknown3B8 = 0;
}
NW_HOOK_CTOR_ARGS(0x100b0ec0, NCSummonedStatusWnd, (INT A), (A))

// 0x100b0f20
NW_HOOK_DTOR(0x100b0f20, NCSummonedStatusWnd)

// stock body shared with NCButton's (0x100051f0)
INT NCSummonedStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCStatusWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100b0f70 (vslot 35) - the time bar sits where the pet window has its food bar
INT NCSummonedStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.PetWnd.PetStatusBack"));
    HPTex = FindTexture(TEXT("L2UI.TargetWnd.MiniHP1"));
    MPTex = FindTexture(TEXT("L2UI.TargetWnd.MiniMP1"));
    HPBackTex = FindTexture(TEXT("L2UI.TargetWnd.MiniHP2"));
    MPBackTex = FindTexture(TEXT("L2UI.TargetWnd.MiniMP2"));

    FrameCtrl = new NCFrameCtrl(0);
    FrameCtrl->SetWindowRect(0, 0, 12, 43);
    FrameCtrl->ID = NID_SUMMONSTATUS_FRAME;
    CreateChild(FrameCtrl, NWS_VISIBLE);

    TimeBar = new NCProgressCtrl(TEXT("L2UI.PetWnd.PetGreenBar2"), TEXT("L2UI.PetWnd.PetGreenBar1"),
                                 0);
    TimeBar->ID = NID_SUMMONSTATUS_TIME;
    TimeBar->SetWindowRect(16, 33, 132, 5);
    CreateChild(TimeBar, NWS_VISIBLE);
    TimeBar->SetMax(10000);
    TimeBar->Reset();
    return 1;
}
NW_HOOK_METHOD(0x100b0f70, NCSummonedStatusWnd, OnCreate)

// 0x100b1140 (vslot 36)
INT NCSummonedStatusWnd::OnDestroy()
{
    if (TimeBar)
    {
        RemoveChildWindow(TimeBar->Entry);
        delete TimeBar;
        TimeBar = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100b1140, NCSummonedStatusWnd, OnDestroy)

// 0x100ae7e0 (vslot 42) - hidden again unless the player has a summon
void NCSummonedStatusWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (GConsole->GetPetInfo().Type != 1)
        HideWindow();
}
NW_HOOK_METHOD(0x100ae7e0, NCSummonedStatusWnd, ShowWindow)

// 0x100b11b0 (vslot 50) - as the pet window without the food bar (the time bar is a child);
// the same uninitialized widths, 0 here
INT NCSummonedStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCSummonedStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    Summon = GConsole->GetPetInfo();
    INT HPW = 0;
    INT MPW = 0;
    if (Summon.MaxHP)
        HPW = Summon.HP * 132 / Summon.MaxHP;
    if (Summon.MaxMP)
        MPW = Summon.MP * 132 / Summon.MaxMP;
    const TCHAR* Text = Summon.Name;
    if (Text && *Text)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, NonEmpty(Summon.Name), 0, 0, 0, 0, 0);
        Device->DrawText(((DWORD)(Width - 12) >> 1) - Size.cx / 2 + 12, 5, 0xFFFFFFFF,
                         NonEmpty(Summon.Name), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->DrawTexture(16, 20, 132, 5, 0, 0, 106, 5, HPBackTex);
    if (HPW > 0)
        Device->DrawTexture(16, 20, HPW, 5, 0, 0, 106, 5, HPTex);
    Device->DrawTexture(16, 27, 132, 5, 0, 0, 106, 5, MPBackTex);
    if (MPW > 0)
        Device->DrawTexture(16, 27, MPW, 5, 0, 0, 106, 5, MPTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b11b0, NCSummonedStatusWnd, OnPaint)

// 0x100ae780 (vslot 56) - right of the title bar: acts on the summon
INT NCSummonedStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if ((SWORD)LOWORD(lParam) > ScreenX + 13)
    {
        User* Me = GConsole->GetMyUser();
        if (Me)
            GConsole->RequestAction(Summon.ObjectID, Me->Pawn->Location);
    }
    return 1;
}
NW_HOOK_METHOD(0x100ae780, NCSummonedStatusWnd, OnLButtonDown)

// 0x100ae770 - from the console (vslot 229) once the timer runs
void NCSummonedStatusWnd::SetRemaining(DWORD Remaining)
{
    TimeBar->SetRemaining(Remaining);
}
NW_HOOK_METHOD(0x100ae770, NCSummonedStatusWnd, SetRemaining)

// 0x100b1180 - from the console (vslot 229) the first time
void NCSummonedStatusWnd::StartTimer(DWORD Max)
{
    bTimerStarted = 1;
    TimeBar->SetMax(Max);
    TimeBar->Start();
}
NW_HOOK_METHOD(0x100b1180, NCSummonedStatusWnd, StartTimer)

NW_CHECK_VTABLE(NCSummonedStatusWnd, 0x100efb50, 62)
