#pragma once
// NControl - base of every control (buttons, edit boxes, lists...) (UClass "Control", 0x1A0).
// A control reports to its parent with NWM_COMMAND (wParam = the control's ID, lParam = a code
// that depends on the control: NotifyID for buttons, 1/0 for press/release...).

#include "nwindow/NCWnd.h"

class NControl : public NCWnd
{
    NW_DECLARE_CLASS(NControl, NCWnd, 0, 0x1011a0b8)

    INT ID;                    // +0x94 set by the parent that creates the control
    UTexture* DisableTexture;  // +0x98 "NWindow.Disable" (OnCreate)
    INT NotifyID;              // +0x9C id sent to the parent, -1 = ID (name ours)
    TCHAR Caption[128];        // +0xA0 (name ours)

    NControl();                                                     // 0x10001860
    // ~NControl                                                    // 0x10001810

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam); // 24 0x10004e90 (folded with
                                                                    //    identical overrides)
    virtual INT OnCreate();                                         // 35 0x10004ff0
    virtual INT OnDestroy();                                        // 36 0x10005020
    virtual const TCHAR* GetValue() { return NULL; }                // 62 0x100016e0 (folded) (name
                                                                    //    ours) what an HTML form
                                                                    //    control holds

    // talking to the parent window (names ours)
    UBOOL NotifyParent(LPARAM Code);                                // 0x10004ed0 NWM_COMMAND
    void ReleaseFocus();                                            // 0x10004f10
    void HideParent();                                              // 0x10004f40
    UBOOL SendTo(NWndEntry* Target, UINT Msg, WPARAM wParam, LPARAM lParam); // 0x10004f70
    UBOOL SendToParent(UINT Msg, WPARAM wParam, LPARAM lParam);    // 0x10004fb0
    void SetCaption(const TCHAR* Text);                             // 0x10005050
    const TCHAR* GetCaption();                                      // 0x10005070

protected:
    NCWnd* ParentWnd() { return Entry->Parent ? Entry->Parent->Wnd : NULL; }
};
NW_ASSERT_OFFSET(NControl, ID, 0x94);
NW_ASSERT_OFFSET(NControl, NotifyID, 0x9C);
NW_ASSERT_OFFSET(NControl, Caption, 0xA0);
NW_ASSERT_SIZE(NControl, 0x1A0);
