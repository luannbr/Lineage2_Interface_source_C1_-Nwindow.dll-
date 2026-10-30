#pragma once
// NCCountSpin - a number with two small buttons at its right that count it up and down between
// a minimum and a maximum (UClass "CCountSpin", 0x3E4). No window of NWindow.dll creates one;
// only the class's InternalConstructor builds it (0 .. 100, no label). A label is drawn first,
// the number after it; each step tells the parent (NWM_COMMAND, 1 up / 2 down), which takes it
// back by setting bCancelStep meanwhile.

#include "nwindow/NControl.h"

class NCPushButton;

class NCCountSpin : public NControl
{
    NW_DECLARE_CLASS(NCCountSpin, NControl, 0, 0x1011a5e0)

    TCHAR ValueText[16];             // +0x1A0 the value in decimal
    TCHAR Label[256];                // +0x1C0 drawn before it
    INT TextX;                       // +0x3C0 (5)
    INT TextY;                       // +0x3C4 (5)
    UTexture* BackTex;               // +0x3C8 "NWindow.EditBack", over the whole control
    INT Min;                         // +0x3CC
    INT Max;                         // +0x3D0
    INT Value;                       // +0x3D4
    NCPushButton* UpButton;          // +0x3D8
    NCPushButton* DownButton;        // +0x3DC
    UBOOL bCancelStep;               // +0x3E0 (name ours) set by the parent while it is told of
                                     //   a step: the step is undone

    NCCountSpin(INT InMin = 0, INT InMax = 100, const TCHAR* InLabel = NULL);         // 0x100052b0
    // ~NCCountSpin                                                  // 0x100053b0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100050a0
    virtual INT OnCreate();                                          // 35 0x10005520
    virtual INT OnDestroy();                                         // 36 0x100056c0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10005400
    // names ours
    virtual INT OnUp();                                              // 63 0x10005130
    virtual INT OnDown();                                            // 64 0x10005190
};
NW_ASSERT_OFFSET(NCCountSpin, ValueText, 0x1A0);
NW_ASSERT_OFFSET(NCCountSpin, Label, 0x1C0);
NW_ASSERT_OFFSET(NCCountSpin, TextX, 0x3C0);
NW_ASSERT_OFFSET(NCCountSpin, Min, 0x3CC);
NW_ASSERT_OFFSET(NCCountSpin, UpButton, 0x3D8);
NW_ASSERT_SIZE(NCCountSpin, 0x3E4);
