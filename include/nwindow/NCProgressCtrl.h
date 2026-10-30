#pragma once
// NCProgressCtrl - count-down bar (UClass "CProgressCtrl", 0x1C0). While running, each paint
// adds the frame time (ms) to Pos; the bar shrinks from full to empty, and at Max the control
// stops and reports NWM_COMMAND(ID, 0) to the parent.

#include "nwindow/NControl.h"

class NCProgressCtrl : public NControl
{
    NW_DECLARE_CLASS(NCProgressCtrl, NControl, 0, 0x1011ab08)

    UTexture* BarTex;          // +0x1A0 "NWindow.WhiteTexture" by default
    UTexture* BackTex;         // +0x1A4 "NWindow.BlackTexture" by default
    INT Unknown1A8;            // +0x1A8 (1)
    INT Border;                // +0x1AC bar inset (2)
    UBOOL bStopped;            // +0x1B0 (1: created stopped)
    DWORD Pos;                 // +0x1B4 ms elapsed
    DWORD Max;                 // +0x1B8 ms
    INT Unknown1BC;            // +0x1BC

    NCProgressCtrl();                                                        // 0x10005730
    NCProgressCtrl(const TCHAR* BackTexName, const TCHAR* BarTexName, INT InBorder); // 0x10005870
    // ~NCProgressCtrl                                                       // 0x10005820

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);          // 24 0x100051f0 (folded)
    virtual INT OnCreate() { return 1; }                                     // 35 0x10005230 (folded)
    virtual INT OnDestroy() { return 1; }                                    // 36 0x10005230 (folded)
    virtual INT OnPaint(NCGDevice* Device);                                  // 50 0x10005960

    // names ours
    void SetRemaining(DWORD Remaining);                                      // 0x10005240
    void SetMax(DWORD InMax);                                                // 0x10005260
    void Reset();                                                            // 0x10005270
    void Stop();                                                             // 0x10005290
    void Start();                                                            // 0x100052a0
};
NW_ASSERT_OFFSET(NCProgressCtrl, Border, 0x1AC);
NW_ASSERT_OFFSET(NCProgressCtrl, Max, 0x1B8);
NW_ASSERT_SIZE(NCProgressCtrl, 0x1C0);
