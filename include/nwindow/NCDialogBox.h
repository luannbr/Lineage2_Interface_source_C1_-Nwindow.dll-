#pragma once
// NCDialogBox - the desktop's message box (UClass "CDialogBox", 0x608): a message, OK /
// Cancel, and optionally an edit line (with a keypad when it takes digits) and a progress
// bar that cancels the box when it ends. NConsoleWnd::ShowDialog fills it from an
// NDialogInfo (SetInfo). What OK and Cancel do depends on DialogID: answer the owner window
// (amounts and yes/no, messages 0xF00000xx), or answer the server directly (invitations,
// wars, trade, revival...), checking names first where one is typed.

#include "nwindow/NControl.h"

class NCButton;
class NCProgressCtrl;
class NCEditBox;
struct NDialogInfo;

// NWM_COMMAND ids of the box's controls
enum
{
    NID_DIALOG_OK = 0x1004,
    NID_DIALOG_CANCEL = 0x1005,
    NID_DIALOG_KEYPAD = 0xF5,          // NotifyID = the key: 0..8 = 1..9, 9 = 0, 10 = backspace
    NID_DIALOG_EDIT = 0x1504,
    NID_DIALOG_PROGRESS = 0xF00DE,
};

// what the owner window gets (NControl::SendTo), names ours
enum
{
    NWM_DIALOG_RESULT = (INT)0xF0000003,   // wParam: 1 / 0 (OK / Cancel) or the amount typed
};

#if NW_HOOK_MODE
#define GKeypadTextures (reinterpret_cast<const TCHAR* const (*)[2]>(::nw::Addr(0x10107d40)))  // "sek.cbui100".. / "sek.cbui111"..
#else
extern const TCHAR* const GKeypadTextures[][2];
#endif

class NCDialogBox : public NControl
{
    NW_DECLARE_CLASS(NCDialogBox, NControl, 0, 0x1011b028)

    UTexture* BackTex;               // +0x1A0 "L2UI.EtcWnd.Popup"
    UTexture* InputBackTex;          // +0x1A4 "L2UI.EtcWnd.Popup" (with an edit line)
    UTexture* KeypadTex;             // +0x1A8 "sek.cbui99"
    NCButton* OKButton;              // +0x1AC
    NCButton* CancelButton;          // +0x1B0
    NCButton* Keypad[11];            // +0x1B4
    NCProgressCtrl* Progress;        // +0x1E0
    NCEditBox* EditBox;              // +0x1E4
    INT DialogID;                    // +0x1E8 (-1 = none)
    INT Type;                        // +0x1EC edit line type (3: digits, the keypad shows)
    NWndEntry* Owner;                // +0x1F0 window that gets the answer
    UBOOL bInput;                    // +0x1F4 the edit line shows
    UBOOL bLargeAmount;              // +0x1F8 amounts up to 100,000,000 (else 1,000,000)
    UBOOL bProgress;                 // +0x1FC the progress bar runs
    TCHAR Message[256];              // +0x200
    TCHAR ParamText[256];            // +0x400 a name some answers carry
    INT Param1;                      // +0x600 numbers some answers carry
    INT Param2;                      // +0x604

    NCDialogBox();                                                   // 0x10006290
    // ~NCDialogBox                                                  // 0x10006340

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10005bf0
    virtual void SetFocus();                                         // 28 0x10005cf0
    virtual INT OnCreate();                                          // 35 0x10006390
    virtual INT OnDestroy();                                         // 36 0x10006970
    virtual void ShowWindow();                                       // 42 0x10007f70
    virtual void HideWindow();                                       // 43 0x10005da0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100067b0
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 53 0x10005d10
    // names ours
    virtual INT OnOK();                                              // 63 0x10006d10
    virtual INT OnCancel();                                          // 64 0x10005de0
    virtual INT OnEditNotify(INT Code);                              // 65 0x10005db0
    virtual INT OnKeypad(INT Key);                                   // 66 0x10007eb0
    virtual INT OnProgressEnd();                                     // 67 0x10006220

    // names ours
    void Reset();                                                    // 0x10005d60
    void SetButtonTexts(INT OKTextID, INT CancelTextID);             // 0x100061a0 sysstring ids
    NWndEntry* GetOwner();                                           // 0x10006210
    void UpdateLayout();                                             // 0x10006a70
    void SetInfo(NDialogInfo* Info);                                 // 0x10006c10

private:
    INT GetAmount();                 // inlined in the stock code (name ours)
    INT RejectText(INT MsgID);       // inlined in the stock code (name ours)
    INT TooLongName();               // inlined in the stock code (name ours)
};
NW_ASSERT_OFFSET(NCDialogBox, BackTex, 0x1A0);
NW_ASSERT_OFFSET(NCDialogBox, Keypad, 0x1B4);
NW_ASSERT_OFFSET(NCDialogBox, Progress, 0x1E0);
NW_ASSERT_OFFSET(NCDialogBox, DialogID, 0x1E8);
NW_ASSERT_OFFSET(NCDialogBox, Owner, 0x1F0);
NW_ASSERT_OFFSET(NCDialogBox, Message, 0x200);
NW_ASSERT_OFFSET(NCDialogBox, ParamText, 0x400);
NW_ASSERT_OFFSET(NCDialogBox, Param1, 0x600);
NW_ASSERT_SIZE(NCDialogBox, 0x608);
