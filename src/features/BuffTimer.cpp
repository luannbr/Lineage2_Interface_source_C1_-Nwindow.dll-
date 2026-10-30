/*=============================================================================
    BuffTimer.cpp - phase 6: the time left on the buff icons and in their tool tip
    (include/features/BuffTimer.h).

    Port of NWindow_C1 src/inject/BuffTimer.cpp (HEAD 4e56326). NWindow_C1 put a jmp on the
    stock NCAbnormalStatusWnd::OnPaint (0x100aed40), drew after the original returned and reached
    the stock code by RVA; here:
      0xd120 "Paint_DrawText", font type 1    NCGDevice::DrawSpecialDigit(1 = the large digits,
                                                "L2UI.NWindow.Number")
      self +0x44 / +0x48                      ScreenX / ScreenY
      self +0x94 / +0x98, entries of 0x1214   Statuses (FAbnormalStatus)
      entry +0x1200 / +0x1204 / +0x1210       Icon / RemainTime / bVisible
      paint +0x2c/+0x30/+0x5c/+0x48/+0x4c     OriginX / OriginY / Unknown5C / DirtyTop /
                                                DirtyBottom (set as NCGDevice::BeginPaint does)
      paint +0x54 / +0x58                     SmallDigits / LargeDigits

    One difference (docs/notas/F6_BuffTimer.md): NWindow_C1 called 0x1000d100 and 0x1000d110
    as a "BeginClip" / "EndClip" pair with no argument. 0x1000d100 is NCGDevice::SetClip(X, Y,
    XL, YL), a jump to UCanvas::SetClip: it took four stack slots of the caller as the rectangle
    and returned with `ret 0x10`, 16 bytes off the caller's stack. Its SEH frame put esp back, so
    what went wrong was only OnPaint's return value (which nothing reads) and a canvas clip left
    on a garbage rectangle (reviewer, round 12). The digits do not depend
    on that rectangle (UCanvas::DrawSpecialDigit draws where it is told), so here the canvas is
    left as the stock paint leaves it: no SetClip and no ReleaseClip around the digits.

    Beyond NWindow_C1 (the user's request after the first in-game test, 27/09/2026), as newer
    chronicles show it: the tool tip of an icon has a line "Remaining Time : N Minute(s)" ("N
    Second(s)" under a minute) under its title, and the icon counts only the last minute (60 to
    1), nothing before. LineageWindow.ini [BuffTimer] IconMinutes=1 brings NWindow_C1's icon
    back ("14M" from a minute up); the tool tip line stays.
=============================================================================*/
#include "nwindow/NWindow.h"
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCGDevice.h"
#include "features/BuffTimer.h"

static const INT MaxSlots = 30;          // ten icons in each of three rows
static const INT SlotPitch = 26;
static const INT TextPadX = 1;           // from the left of the icon
static const INT TextY = 15;             // from its top
static const UBOOL DigitsLarge = 1;      // NCGDevice::DrawSpecialDigit's font: the large digits
static const DWORD TextColor = 0xFFFFFFFF;
static const DWORD OutlineColor = 0xFF000000;

// the tool tip's line, in DrawStatusToolTip's font and colors: the label as the title's "Lv",
// the time as the title's level
static const TCHAR RemainLabel[] = TEXT("Remaining Time :");
static const DWORD RemainLabelColor = 0xFFA3A3A3;
static const DWORD RemainValueColor = 0xFFB09B79;
static const INT RemainGap = 4;          // between the label and the time (a space)
static const INT ToolTipLine = 13;       // DrawStatusToolTip's step from a line to the next
static const INT ToolTipExtent = 12;     // a line in GetTextExtent's height (the box adds a strip)

// [BuffTimer] IconMinutes (0): 1 = NWindow_C1's icon (the minutes and an M from a minute up).
// Read once.
static UBOOL IconMinutes()
{
    static INT Value = -1;
    if (Value < 0)
        Value = nwf::IniInt("BuffTimer", "IconMinutes", 0) != 0;
    return Value;
}

// Value in decimal.
static void FormatNumber(DWORD Value, TCHAR* Buf, INT BufChars)
{
    TCHAR Digits[12];
    INT D = 0;
    do
    {
        Digits[D++] = (TCHAR)('0' + Value % 10);
        Value /= 10;
    } while (Value > 0 && D < 11);
    INT N = 0;
    while (D > 0 && N < BufChars - 1)
        Buf[N++] = Digits[--D];
    Buf[N] = 0;
}

// The icon's text. By default only the last minute, "60" to "1" (the tool tip has the time
// before). bMinutes (IconMinutes, NWindow_C1): from a minute on the minutes and an M ("14M"),
// below that the seconds ("59": the digit texture has no S). Nothing at 0.
static void FormatCountdown(INT RemainSeconds, TCHAR* Buf, INT BufChars, UBOOL bMinutes)
{
    if (!Buf || BufChars < 4)
    {
        if (Buf && BufChars > 0)
            Buf[0] = 0;
        return;
    }
    Buf[0] = 0;
    if (RemainSeconds <= 0)
        return;
    if (!bMinutes)
    {
        if (RemainSeconds <= 60)
            FormatNumber((DWORD)RemainSeconds, Buf, BufChars);
        return;
    }
    if (RemainSeconds >= 60)
    {
        FormatNumber((DWORD)(RemainSeconds / 60), Buf, BufChars - 1);
        INT N = 0;
        while (Buf[N])
            ++N;
        if (N < BufChars - 1)
        {
            Buf[N++] = 'M';
            Buf[N] = 0;
        }
    }
    else
        FormatNumber((DWORD)RemainSeconds, Buf, BufChars);
}

// Top left corner of icon Index in the window, as NCAbnormalStatusWnd::OnPaint places it.
static void IconXY(INT Index, INT* X, INT* Y)
{
    if (Index < 10)
    {
        *X = Index * SlotPitch + 0x0D;
        *Y = 1;
    }
    else if (Index < 20)
    {
        *X = Index * SlotPitch - 0xF7;
        *Y = 0x1A;
    }
    else
    {
        *X = Index * SlotPitch - 0x1FB;
        *Y = 0x33;
    }
}

// Text with a one pixel black outline and a shadow under it.
static void DrawLabel(NCGDevice* Device, INT X, INT Y, TCHAR* Text)
{
    if (!Text || !Text[0])
        return;
    if (!Device->LargeDigits && !Device->SmallDigits)
        return;
    static const INT DX[4] = {-1, 1, 0, 0};
    static const INT DY[4] = {0, 0, -1, 1};
    for (INT i = 0; i < 4; ++i)
        Device->DrawSpecialDigit(DigitsLarge, X + DX[i], Y + DY[i], OutlineColor, Text);
    Device->DrawSpecialDigit(DigitsLarge, X + 1, Y + 1, OutlineColor, Text);
    Device->DrawSpecialDigit(DigitsLarge, X, Y, TextColor, Text);
}

// The digits of every shown icon with 1 to 999999 seconds left (by default the last 60 only);
// none at all past 30 icons.
static void DrawCountdowns(NCAbnormalStatusWnd* Wnd, NCGDevice* Device, INT Count,
                           FAbnormalStatus* Statuses)
{
    __try
    {
        Device->BeginPaint(Wnd->ScreenX, Wnd->ScreenY);
        for (INT i = 0; i < Count && i < MaxSlots; ++i)
        {
            FAbnormalStatus* Status = &Statuses[i];
            if (!Status->Icon || !Status->bVisible)
                continue;
            const INT Remain = Status->RemainTime;
            if (Remain <= 0 || Remain > 999999)
                continue;
            TCHAR Buf[16];
            FormatCountdown(Remain, Buf, 16, IconMinutes());
            if (!Buf[0])
                continue;
            INT X, Y;
            IconXY(i, &X, &Y);
            DrawLabel(Device, X + TextPadX, Y + TextY, Buf);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

// The tool tip's time: "N Minute(s)" from a minute on (whole minutes), "N Second(s)" below.
static void FormatRemaining(INT RemainSeconds, TCHAR* Buf)
{
    if (RemainSeconds >= 60)
        appSprintf(Buf, TEXT("%d Minute(s)"), RemainSeconds / 60);
    else
        appSprintf(Buf, TEXT("%d Second(s)"), RemainSeconds);
}

namespace nwf {

INT BuffTimer_ToolTipExtraHeight(INT RemainTime)
{
    return RemainTime > 0 ? ToolTipExtent : 0;
}

INT BuffTimer_DrawToolTipTime(NCGDevice* Device, INT RemainTime, INT X, INT Y)
{
    if (!Device || RemainTime <= 0)
        return 0;
    TCHAR Value[32];
    FormatRemaining(RemainTime, Value);
    Device->DrawText(X, Y, RemainLabelColor, RemainLabel, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, RemainLabel, 0, 0, 0, 0, 0);
    Device->DrawText(X + Size.cx + RemainGap, Y, RemainValueColor, Value, 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);
    return ToolTipLine;
}

void BuffTimer_DrawCountdowns(NCAbnormalStatusWnd* Wnd, NCGDevice* Device)
{
    if (!Wnd || !Device)
        return;
    const INT Count = Wnd->Statuses.Num();
    FAbnormalStatus* Statuses = (FAbnormalStatus*)Wnd->Statuses.GetData();
    if (Count <= 0 || Count > MaxSlots || !Statuses)
        return;

    // the device's paint state as the stock paint left it, put back after the digits
    const INT OriginX = Device->OriginX;
    const INT OriginY = Device->OriginY;
    const INT Unknown5C = Device->Unknown5C;
    const INT DirtyTop = Device->DirtyTop;
    const INT DirtyBottom = Device->DirtyBottom;
    UTexture* const SmallDigits = Device->SmallDigits;
    UTexture* const LargeDigits = Device->LargeDigits;

    DrawCountdowns(Wnd, Device, Count, Statuses);

    Device->OriginX = OriginX;
    Device->OriginY = OriginY;
    Device->Unknown5C = Unknown5C;
    Device->DirtyTop = DirtyTop;
    Device->DirtyBottom = DirtyBottom;
    Device->SmallDigits = SmallDigits;
    Device->LargeDigits = LargeDigits;
}

}  // namespace nwf
