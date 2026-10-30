/*=============================================================================
    BuffTimer.h - phase 6: the time left on each buff / debuff icon (NWindow_C1
    src/inject/BuffTimer.cpp, docs SESSION_2026-07-19_BUFFTIMER.md and
    NCAbnormalStatusWnd_PORT.md), and in its tool tip.

    Over each icon of NCAbnormalStatusWnd, in the large digits of NCGDevice::DrawSpecialDigit
    with a black outline: the seconds of the last minute ("60" to "1"), nothing before and at
    0. The digit texture has 0-9, H, M, P, /, % and . only: no S, and no lower case letters.
    LineageWindow.ini [BuffTimer] IconMinutes=1: NWindow_C1's icon, the minutes and an M from
    one minute up ("14M"), the seconds below.
    The tool tip (newer chronicles): "Remaining Time : N Minute(s)", "N Second(s)" under a
    minute, under the title.

    NWindow_C1 detoured the stock NCAbnormalStatusWnd::OnPaint (0x100aed40) and drew after
    it; here the decompiled OnPaint calls this at its end, and OnDrawToolTip and
    DrawStatusToolTip call the tool tip part (#if NW_FEATURES blocks). The differences:
    src/features/BuffTimer.cpp, docs/notas/F6_BuffTimer.md.
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

#include "nwindow/NWindow.h"

class NCAbnormalStatusWnd;
class NCGDevice;

namespace nwf {

// The end of NCAbnormalStatusWnd::OnPaint, after the icons: the countdown of every icon shown.
void BuffTimer_DrawCountdowns(NCAbnormalStatusWnd* Wnd, NCGDevice* Device);

// NCAbnormalStatusWnd::OnDrawToolTip, added to the height GetTextExtent gave for the tool tip
// text: 12 (one line, so one more strip of the box) when there is time left to show, else 0.
INT BuffTimer_ToolTipExtraHeight(INT RemainTime);

// NCAbnormalStatusWnd::DrawStatusToolTip, on the line under the title: "Remaining Time :" and
// "N Minute(s)" / "N Second(s)" at (X, Y); returns how far the next line moves down (13, or 0
// when there is no time left to show).
INT BuffTimer_DrawToolTipTime(NCGDevice* Device, INT RemainTime, INT X, INT Y);

}  // namespace nwf

#endif  // NW_FEATURES
