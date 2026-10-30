#pragma once
// NCRadarWnd - the radar (UClass "CRadarWnd", 0xA0): the canvas draws the radar itself
// (NCGDevice::DrawRadar) and a mark per target. The console fills Targets by calling
// NCMinimapWnd::AddTarget / RemoveTarget on this window (the same array, at the same place),
// as it does for the map window.
//
// Desktop (NCConsole) +0x10C is the radar window.

#include "nwindow/NCMinimapWnd.h"

class NCRadarWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCRadarWnd, NCWnd, 0, 0x101497a8)

    TArray<FMinimapTarget> Targets;  // +0x94 world coordinates

    NCRadarWnd();                                                    // 0x100a1d90
    // ~NCRadarWnd                                                   // 0x100a1df0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x100a1c90
    virtual INT OnDestroy();                                         // 36 0x100a1d40
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100a1ca0

    void ClearTargets();                                             // 0x100a1d70 (name ours)
    // the map window's, folded (the console calls them on both windows)
    void AddTarget(INT X, INT Y, INT Z);                             // 0x10079230
    void RemoveTarget(INT X, INT Y, INT Z);                          // 0x10079270
};
NW_ASSERT_OFFSET(NCRadarWnd, Targets, 0x94);
NW_ASSERT_SIZE(NCRadarWnd, 0xA0);
