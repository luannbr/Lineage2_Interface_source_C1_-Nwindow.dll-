#pragma once
// NDialogInfo - what a dialog box shows (name ours; 0x162C bytes, built on the stack):
// windows fill one and hand it to NConsoleWnd::ShowDialog, which opens one of the desktop's
// dialog boxes (NCDialogBox's source file keeps these helpers). The answer goes to Owner with
// ID. Field meanings after the callers seen so far.

#include "nwindow/NWindow.h"

struct NWndEntry;

struct NDialogInfo
{
    INT ID;                    // +0x000 (-1) what the box does with the answer (NCDialogBox)
    INT Type;                  // +0x004 (3) type of the box's edit line (NCEditBox: 3 digits,
                               //   with the keypad)
    NWndEntry* Owner;          // +0x008 window that gets the answer
    UBOOL bMainBox;            // +0x00C (1) ShowDialog: 1 = the desktop's box at +0x148,
                               //   0 = the one at +0x14C
    UBOOL bInput;              // +0x010 (0) the box has an edit line
    UBOOL bLargeAmount;        // +0x014 amounts up to 100,000,000 instead of 1,000,000
    UBOOL bProgress;           // +0x018 a progress bar runs; when it ends the box cancels
    TCHAR Text[1024];          // +0x01C the message
    TCHAR Text2[1024];         // +0x81C shown instead of Text when set
    TCHAR Button1[256];        // +0x101C
    TCHAR Button2[256];        // +0x121C
    TCHAR ParamText[256];      // +0x141C a name the answer carries (not cleared by the
                               //   constructor)
    INT Param1;                // +0x161C numbers the answer carries (0 = keep the box's)
    INT Param2;                // +0x1620
    INT Unknown1624;           // +0x1624
    INT Unknown1628;           // +0x1628

    NDialogInfo();                                                   // 0x10006230
    void SetText(const TCHAR* InText);                               // 0x10005b70
    void SetSystemMessage(INT MsgID);                                // 0x10005b10 systemmsg line
    void SetButtons(const TCHAR* InButton1, const TCHAR* InButton2); // 0x10005ab0
    void SetParamText(const TCHAR* InText);                          // 0x10005ba0
    void SetParams(INT InParam1, INT InParam2);                      // 0x10005bd0
};
NW_ASSERT_OFFSET(NDialogInfo, Text, 0x1C);
NW_ASSERT_OFFSET(NDialogInfo, Text2, 0x81C);
NW_ASSERT_OFFSET(NDialogInfo, Button1, 0x101C);
NW_ASSERT_OFFSET(NDialogInfo, Button2, 0x121C);
NW_ASSERT_OFFSET(NDialogInfo, ParamText, 0x141C);
NW_ASSERT_OFFSET(NDialogInfo, Param1, 0x161C);
NW_ASSERT_SIZE(NDialogInfo, 0x162C);
