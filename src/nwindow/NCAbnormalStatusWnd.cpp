/*=============================================================================
    NCAbnormalStatusWnd.cpp - the buff / debuff icons. Stock: 0x100adf40 .. 0x100ae2fb,
    0x100aebf0 .. 0x100af2a7, 0x100b3380 .. 0x100b34e9, 0x100b3780 .. 0x100b386f
    (0x100b36e0 is the TArray<FAbnormalStatus> destructor).
=============================================================================*/
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#if NW_FEATURES
// phase 6: BuffTimer (NWindow_C1 src/inject/BuffTimer.cpp)
#include "features/BuffTimer.h"
#endif

// title bar id
enum { NID_ABNORMALSTATUS_FRAME = 0xFFA0B3 };

// the tool tip box: 184 pixels wide, text wrapped at 168
enum { TIP_Width = 184, TIP_TextWidth = 168 };

// 0x100b3780 - the icon textures are left as allocated
NCAbnormalStatusWnd::NCAbnormalStatusWnd(INT InIconSize)
{
    TimerID = 0;
    ToolTipTex = NULL;
    TipTopTex = NULL;
    TipMidTex = NULL;
    TipBottomTex = NULL;
    IconSize = InIconSize;
    HoverIndex = -1;
}
NW_HOOK_CTOR_ARGS(0x100b3780, NCAbnormalStatusWnd, (INT A), (A))

// 0x100b3810
NW_HOOK_DTOR(0x100b3810, NCAbnormalStatusWnd)

// stock body shared with NControl's (0x10004e90)
INT NCAbnormalStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100af230 (vslot 34) - every second: the time runs down; in the last ten seconds the
// icon blinks
INT NCAbnormalStatusWnd::OnTimer(INT InTimerID)
{
    if (InTimerID == TIMER_AbnormalStatus)
    {
        for (INT i = 0; i < Statuses.Num(); i++)
        {
            FAbnormalStatus& Status = Statuses(i);
            if (Status.RemainTime > 0)
                Status.RemainTime--;
            if (Status.RemainTime >= 0 && Status.RemainTime <= 10)
                Status.bVisible = !Status.bVisible;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100af230, NCAbnormalStatusWnd, OnTimer)

// 0x100aebf0 (vslot 35) - NCWnd::OnCreate is not called
INT NCAbnormalStatusWnd::OnCreate()
{
    TimerID = SetTimer(TIMER_AbnormalStatus, 1000, NULL);
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    TipTopTex = FindTexture(TEXT("L2UI.Tooltip.Tool02"));
    TipMidTex = FindTexture(TEXT("L2UI.Tooltip.Tool01"));
    TipBottomTex = FindTexture(TEXT("L2UI.Tooltip.Tool03"));
    BackTex = FindTexture(TEXT("L2UI.EtcWndBack.AbnormalBack"));
    Frame1Tex = FindTexture(TEXT("L2UI.EtcWndBack.AbnormalFrame1"));
    Frame2Tex = FindTexture(TEXT("L2UI.EtcWndBack.AbnormalFrame2"));
    Frame3Tex = FindTexture(TEXT("L2UI.EtcWndBack.AbnormalFrame3"));

    FrameCtrl = new NCFrameCtrl(0);
    FrameID = NID_ABNORMALSTATUS_FRAME;
    FrameCtrl->ID = NID_ABNORMALSTATUS_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 12, 26);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100aebf0, NCAbnormalStatusWnd, OnCreate)

// 0x100b3380 (vslot 36)
INT NCAbnormalStatusWnd::OnDestroy()
{
    if (TimerID)
    {
        KillTimer(TimerID);
        TimerID = 0;
    }
    Statuses.Empty();
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100b3380, NCAbnormalStatusWnd, OnDestroy)

// stock body shared with NCChatWnd's (0x1003dab0)
void NCAbnormalStatusWnd::HideWindow()
{
    NCWnd::HideWindow();
}

// 0x100aed40 (vslot 50) - ten icons per row after the 12-pixel title bar; an icon without a
// texture leaves its place empty, and a blinking one hides every other second
INT NCAbnormalStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCAbnormalStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    for (INT i = 0; i < Statuses.Num(); i++)
    {
        FAbnormalStatus Status = Statuses(i);
        if (!Status.Icon)
            continue;
        if (i < 10)
        {
            Device->DrawTexture(0, 0, 12, 26, 0, 0, 12, 26, Frame1Tex);
            Device->DrawTexture(i * 26 + 12, 0, 26, 26, 0, 0, 26, 26, BackTex);
            if (Status.bVisible)
                Device->DrawStretchedTex(i * 26 + 13, 1, 24, 24, Status.Icon);
        }
        else if (i < 20)
        {
            Device->DrawTexture(0, 0, 12, 51, 0, 0, 12, 51, Frame2Tex);
            Device->DrawTexture((i - 10) * 26 + 12, 25, 26, 26, 0, 0, 26, 26, BackTex);
            if (Status.bVisible)
                Device->DrawStretchedTex((i - 10) * 26 + 13, 26, 24, 24, Status.Icon);
        }
        else if (i < 30)
        {
            Device->DrawTexture(0, 0, 12, 76, 0, 0, 12, 76, Frame3Tex);
            Device->DrawTexture((i - 20) * 26 + 12, 50, 26, 26, 0, 0, 26, 26, BackTex);
            if (Status.bVisible)
                Device->DrawStretchedTex((i - 20) * 26 + 13, 51, 24, 24, Status.Icon);
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
#if NW_FEATURES
    // phase 6: BuffTimer (NWindow_C1 src/inject/BuffTimer.cpp): the time left over the icons
    nwf::BuffTimer_DrawCountdowns(this, Device);
#endif
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100aed40, NCAbnormalStatusWnd, OnPaint)

// 0x100aef90 (vslot 51) - the tool tip box over the icon under the mouse, while the icon is
// shown. Without an icon under the mouse the clip and the origin stay set, as in the stock
// code; for a 31st icon the stock code reads X from a slot never written, which holds the
// NCGDevice pointer (the box lands off screen), reproduced.
INT NCAbnormalStatusWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    IconSize = 26;
    if (HoverIndex < 0 || HoverIndex > 30)
        return 1;
    if (HoverIndex < Statuses.Num())
    {
        FAbnormalStatus& Status = Statuses(HoverIndex);
        const TCHAR* Tip = Status.ToolTip;
        if (Tip && *Tip && Status.bVisible)
        {
            SIZE Size;
            NCGDevice::GetTextExtent(&Size, Tip, TIP_TextWidth, 0, 0, 0, 0);
#if NW_FEATURES
            // phase 6: BuffTimer (NWindow_C1 src/inject/BuffTimer.cpp; beyond it): a line more
            // for the time left
            Size.cy += nwf::BuffTimer_ToolTipExtraHeight(Status.RemainTime);
#endif
            INT X;
            if (HoverIndex < 10)
                X = IconSize * HoverIndex + 12;
            else if (HoverIndex < 20)
                X = (HoverIndex - 10) * IconSize + 12;
            else if (HoverIndex < 30)
                X = (HoverIndex - 20) * IconSize + 12;
            else
                X = (INT)Device;
            INT Y = HoverIndex / 10 * (IconSize - 1);
            Device->SetClip(ScreenX + X, ScreenY + Y, TIP_Width, Size.cy / 12 * 13 + 16);
            if (TipTopTex)
                Device->DrawTexture(X, Y, TIP_Width, 8, 0, 0, TIP_Width, 8, TipTopTex);
            INT Strips = Size.cy / 12;
            for (INT k = 0; k < Strips; k++)
            {
                if (TipMidTex)
                    Device->DrawTexture(X, Y + 8 + k * 13, TIP_Width, 13, 0, 0, TIP_Width, 13,
                                        TipMidTex);
            }
            if (TipBottomTex)
                Device->DrawTexture(X, Strips * 13 + Y + 8, TIP_Width, 8, 0, 0, TIP_Width, 8,
                                    TipBottomTex);
            DrawStatusToolTip(Device, Statuses(HoverIndex), X + 8, Y + 8);
            Device->ReleaseClip();
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x100aef90, NCAbnormalStatusWnd, OnDrawToolTip)

// 0x100af200 (vslot 55)
INT NCAbnormalStatusWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    HoverIndex = HitTest(LOWORD(lParam), HIWORD(lParam));
    return 1;
}
NW_HOOK_METHOD(0x100af200, NCAbnormalStatusWnd, OnMouseMove)

// 0x100ae0f0 (vslot 62) - "name  Lv n" on the first line, the description wrapped under it
void NCAbnormalStatusWnd::DrawStatusToolTip(NCGDevice* Device, FAbnormalStatus Status, INT X,
                                            INT Y)
{
    FString Desc;
    TCHAR Buf[1024];
    SIZE Size;
    GConsole->ConvertNewLines(Status.Desc, Desc);
    if (Status.Name[0])
        appSprintf(Buf, TEXT("%s"), Status.Name);
    else
        Buf[0] = 0;
    Device->DrawText(X, Y, 0xFFFFFFFF, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    INT TX = Size.cx + X + 6;
    if (*GL2GameData.SysString[88])                                  // "Lv"
        appSprintf(Buf, TEXT("%s"), *GL2GameData.SysString[88]);
    else
        Buf[0] = 0;
    Device->DrawText(TX, Y, 0xFFA3A3A3, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    appSprintf(Buf, TEXT("%d"), Status.Level);
    TX += Size.cx + 2;
    Device->DrawText(TX, Y, 0xFFB09B79, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Y += 13;
#if NW_FEATURES
    // phase 6: BuffTimer (NWindow_C1 src/inject/BuffTimer.cpp; beyond it): "Remaining Time :
    // N Minute(s)" under the title, the description under it
    Y += nwf::BuffTimer_DrawToolTipTime(Device, Status.RemainTime, X, Y);
#endif
    appSprintf(Buf, TEXT("%s"), *Desc);
    Device->DrawText(X, Y, 0xFFB2BECF, Buf, TIP_TextWidth, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                     1);
}
NW_HOOK_METHOD(0x100ae0f0, NCAbnormalStatusWnd, DrawStatusToolTip)

// 0x100adf40 - the icon under a screen point, row by row of ten; the title bar is not taken
// off, so the hit areas sit 12 pixels left of the icons, as in the stock code
INT NCAbnormalStatusWnd::HitTest(INT X, INT Y)
{
    INT RX = X - ScreenX;
    INT RY = Y - ScreenY;
    if (RX < 0 || RX > (IconSize + 1) * 10 || RY < 0 || RY > (IconSize + 1) * 3)
        return -1;
    return RY / (IconSize + 1) * 10 + RX / (IconSize + 1);
}
NW_HOOK_METHOD(0x100adf40, NCAbnormalStatusWnd, HitTest)

// 0x100adf90 - one to three rows (nothing changes from 30 icons on)
void NCAbnormalStatusWnd::UpdateSize()
{
    INT Count = Statuses.Num();
    if (Count < 10)
    {
        SetSize(Count * 26 + 12, 26);
        FrameCtrl->SetSize(12, 26);
    }
    else if (Count < 20)
    {
        SetSize(272, 51);
        FrameCtrl->SetSize(12, 51);
    }
    else if (Count < 30)
    {
        SetSize(272, 76);
        FrameCtrl->SetSize(12, 76);
    }
}
NW_HOOK_METHOD(0x100adf90, NCAbnormalStatusWnd, UpdateSize)

// 0x100ae010 - "name\ndescription"; the "MP cost" / "Range" sysstrings are checked and the
// skill's range fetched, but neither ends up in the text
void NCAbnormalStatusWnd::MakeToolTip(FAbnormalStatus* Status)
{
    FString Desc;
    const TCHAR* Text = Status->Desc;
    if (Text && *Text)
        GConsole->ConvertNewLines(Status->Desc, Desc);
    if (*GL2GameData.SysString[320] && *GL2GameData.SysString[321])
    {
        GConsole->GetSkillRange(Status->SkillID, Status->Level);
        appSprintf(Status->ToolTip, TEXT("%s\n%s"), Status->Name, *Desc);
    }
}
NW_HOOK_METHOD(0x100ae010, NCAbnormalStatusWnd, MakeToolTip)

// 0x100b33d0 - from the console (vslot 122); refused past 30 (so a 31st still gets in).
// The name and description come from the skill data.
INT NCAbnormalStatusWnd::AddStatus(FAbnormalStatus* Status)
{
    if (Statuses.Num() > 30)
        return 0;
    const TCHAR* Icon = Status->IconName;
    if (Icon)
        Status->Icon = FindTexture(Icon);
    if (GConsole->GetSkillName(Status->SkillID, Status->Level))
        appStrcpy(Status->Name, GConsole->GetSkillName(Status->SkillID, Status->Level));
    if (GConsole->GetSkillDesc(Status->SkillID, Status->Level))
        appStrcpy(Status->Desc, GConsole->GetSkillDesc(Status->SkillID, Status->Level));
    MakeToolTip(Status);
    Statuses.AddItem(*Status);
    UpdateSize();
    return 1;
}
NW_HOOK_METHOD(0x100b33d0, NCAbnormalStatusWnd, AddStatus)

// 0x100b34d0 - the size is left as it was
void NCAbnormalStatusWnd::ClearStatus()
{
    Statuses.Empty();
}
NW_HOOK_METHOD(0x100b34d0, NCAbnormalStatusWnd, ClearStatus)

NW_CHECK_VTABLE(NCAbnormalStatusWnd, 0x100efef0, 63)
