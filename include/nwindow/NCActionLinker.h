#pragma once
// NCActionLinker - a link of the HTML viewer that runs an action (UClass "CActionLinker",
// 0x5A4): a click hands its code to the viewer (NWM_COMMAND, lParam = Action), or first asks
// Message in a yes / no box (dialog 0x38), whose OK comes back as NWM_ACTIONLINK_CONFIRM.

#include "nwindow/NControl.h"

enum { NWM_ACTIONLINK_CONFIRM = (INT)0xF0000010 };   // from the dialog box (dialog 0x38)

class NCActionLinker : public NControl
{
    NW_DECLARE_CLASS(NCActionLinker, NControl, 0, 0x10128458)

    INT Action;                        // +0x1A0 code handed to the parent
    TCHAR Message[512];                // +0x1A4 asked first when not empty (set by the viewer)

    NCActionLinker(INT InAction = 0);                                    // 0x100306e0
    // ~NCActionLinker                                               // 0x10030750

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1002fe70
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100307a0
    virtual INT OnConfirm(WPARAM wParam, LPARAM lParam);             // 63 0x1002fe50 (name ours)
};
NW_ASSERT_OFFSET(NCActionLinker, Action, 0x1A0);
NW_ASSERT_OFFSET(NCActionLinker, Message, 0x1A4);
NW_ASSERT_SIZE(NCActionLinker, 0x5A4);
