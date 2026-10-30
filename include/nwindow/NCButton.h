#pragma once
// NCButton and its subclasses (UClass "CButton" 0x21C, "CExButton", "CPushButton",
// "CRadioButton").
//
// A button paints NormalTex, or PushedTex while bReleased is 0, stretched over its rectangle,
// with its label centered (a SysString index, or Text). Clicks play interface sound 0 (2 when
// disabled) and report NWM_COMMAND(ID, NotifyID) to the parent on release inside the button.
// The tool tip (ToolTipID, a SysString index) is drawn above the button, or below it, while
// the mouse is over it.
//   NCExButton     reports 1 on press and 0 on release instead.
//   NCPushButton   reports on press and, while held, again every RepeatDelay ms (timer).
//   NCRadioButton  stays down once clicked; reports Value.

#include "nwindow/NControl.h"

class NCButton : public NControl
{
    NW_DECLARE_CLASS(NCButton, NControl, 0, 0x10117260)

    UBOOL bReleased;           // +0x1A0 0 while pressed (paints PushedTex)
    UBOOL bMouseOver;          // +0x1A4 set by OnMouseMove, shows the tool tip
    UBOOL bToolTipAbove;       // +0x1A8 1 = tool tip above the button, 0 = below
    UTexture* NormalTex;       // +0x1AC
    UTexture* PushedTex;       // +0x1B0
    UTexture* ToolTipTex;      // +0x1B4 "L2UI.tooltip.mini-tool01", stretched under the tip
    INT NormalTexH;            // +0x1B8 VSize of NormalTex
    INT NormalTexW;            // +0x1BC USize
    INT PushedTexH;            // +0x1C0
    INT PushedTexW;            // +0x1C4
    UBOOL bAcceptKey;          // +0x1C8 1: Enter / Space click it while it has the focus
    TCHAR Text[32];            // +0x1CC label when TextID < 0
    INT TextID;                // +0x20C SysString index of the label, -1 = Text
    INT ToolTipID;             // +0x210 SysString index of the tool tip, -1 = none
    INT TextOffsetX;           // +0x214 label offset from the center
    INT TextOffsetY;           // +0x218

    NCButton();                                                                 // 0x10001950
    NCButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName, UBOOL bInAcceptKey); // 0x10001a20
    // ~NCButton                                                                // 0x100018d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);             // 24 0x100051f0
    virtual INT OnDestroy();                                                    // 36 0x100189e0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                                     // 50 0x10001b30
    virtual INT OnDrawToolTip(NCGDevice* Device);                               // 51 0x10001dd0
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);                        // 53 0x10001150
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);                      // 55 0x100010c0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);                    // 56 0x10001000
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);                      // 57 0x10001070

    // names ours
    void SetTextID(INT ID);                                                     // 0x100011b0
    void SetText(const TCHAR* InText);                                          // 0x100011e0
    UBOOL SetNormalTex(const TCHAR* Name);                                      // 0x10001210
    UBOOL SetPushedTex(const TCHAR* Name);                                      // 0x10001260
    void SetToolTip(INT ID, UBOOL bAbove);                                      // 0x100012b0
};
NW_ASSERT_OFFSET(NCButton, bReleased, 0x1A0);
NW_ASSERT_OFFSET(NCButton, NormalTex, 0x1AC);
NW_ASSERT_OFFSET(NCButton, bAcceptKey, 0x1C8);
NW_ASSERT_OFFSET(NCButton, Text, 0x1CC);
NW_ASSERT_OFFSET(NCButton, TextID, 0x20C);
NW_ASSERT_OFFSET(NCButton, TextOffsetY, 0x218);
NW_ASSERT_SIZE(NCButton, 0x21C);

class NCExButton : public NCButton
{
    NW_DECLARE_CLASS(NCExButton, NCButton, 0, 0x10117780)

    NCExButton();                                                               // 0x100020e0
    NCExButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName, UBOOL bInAcceptKey); // 0x10002180
    // ~NCExButton                                                              // 0x10002130

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);             // 24 0x100301f0 (folded)
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);                    // 56 0x100012f0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);                      // 57 0x10001330
};
NW_ASSERT_SIZE(NCExButton, 0x21C);

class NCPushButton : public NCButton
{
    NW_DECLARE_CLASS(NCPushButton, NCButton, 0, 0x10117ca8)

    UBOOL bHeld;               // +0x21C pressed and still inside
    INT RepeatDelay;           // +0x220 ms between repeats (100)
    UBOOL bAutoRepeat;         // +0x224 (1)

    NCPushButton();                                                             // 0x100021b0
    NCPushButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName);       // 0x10002260
    // ~NCPushButton                                                            // 0x10002210

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);             // 24 0x100301f0 (folded)
    virtual INT OnTimer(INT TimerID);                                           // 34 0x10001480
    virtual INT OnDestroy();                                                    // 36 0x100015f0
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);                      // 55 0x10001410
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);                    // 56 0x10001350
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);                      // 57 0x100013d0
};
NW_ASSERT_OFFSET(NCPushButton, bHeld, 0x21C);
NW_ASSERT_SIZE(NCPushButton, 0x228);

class NCRadioButton : public NCButton
{
    NW_DECLARE_CLASS(NCRadioButton, NCButton, 0, 0x101181c8)

    INT Value;                 // +0x21C sent to the parent when selected (-1)
    INT CreateWidth;           // +0x220 size at OnCreate
    INT CreateHeight;          // +0x224
    UBOOL bToggle;             // +0x228 a second click releases it; never reports

    NCRadioButton();                                                            // 0x100022c0
    NCRadioButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName);      // 0x10002380
    // ~NCRadioButton                                                           // 0x10002330

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);             // 24 0x100301f0 (folded)
    virtual INT OnCreate();                                                     // 35 0x100015c0
    virtual INT OnDestroy();                                                    // 36 0x100015f0 (folded)
    virtual INT OnDrawToolTip(NCGDevice* Device);                               // 51 0x10002410
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);                      // 55 0x10001570
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);                    // 56 0x100014b0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);                      // 57 0x10001550
};
NW_ASSERT_OFFSET(NCRadioButton, Value, 0x21C);
NW_ASSERT_OFFSET(NCRadioButton, bToggle, 0x228);
NW_ASSERT_SIZE(NCRadioButton, 0x22C);
