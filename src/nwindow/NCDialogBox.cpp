/*=============================================================================
    NCDialogBox.cpp - the desktop's message box. Stock: 0x10005bf0 .. 0x10005dda,
    0x10005de0 .. 0x10006216, 0x10006220 .. 0x1000622d, 0x10006290 .. 0x10006a69,
    0x10006a70 .. 0x10007f7f (NDialogInfo keeps 0x10005ab0 .. 0x10005be6 and 0x10006230).
=============================================================================*/
#include "nwindow/NCDialogBox.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCProgressCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCrt.h"
#if NW_FEATURES
#include "features/ReturnToLogin.h"   // phase 6: ReturnToLogin
#endif

// 0x10006290 - NControl's constructor is inlined; the keypad, the edit line, Type and
// bLargeAmount are left as allocated
NCDialogBox::NCDialogBox()
{
    BackTex = NULL;
    InputBackTex = NULL;
    OKButton = NULL;
    CancelButton = NULL;
    Progress = NULL;
    Owner = NULL;
    bProgress = 0;
    bInput = 0;
    Message[0] = 0;
    ParamText[0] = 0;
    Param1 = 0;
    Param2 = 0;
    DialogID = -1;
}
NW_HOOK_CTOR(0x10006290, NCDialogBox)

// 0x10006340
NW_HOOK_DTOR(0x10006340, NCDialogBox)

// 0x10005bf0 (vslot 24)
INT NCDialogBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_DIALOG_OK)
            return OnOK() != 0;
        if (wParam == NID_DIALOG_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_DIALOG_KEYPAD)
            return OnKeypad(lParam) != 0;
        if (wParam == NID_DIALOG_EDIT)
            return OnEditNotify(lParam) != 0;
        if (wParam == NID_DIALOG_PROGRESS)
            return OnProgressEnd() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10005bf0, NCDialogBox, WindowProc)

// 0x10005cf0 (vslot 28)
void NCDialogBox::SetFocus()
{
    NCWnd::SetFocus();
    if (bInput)
        EditBox->SetFocus();
}
NW_HOOK_METHOD(0x10005cf0, NCDialogBox, SetFocus)

// 0x10006390 (vslot 35) - NControl::OnCreate is not called
INT NCDialogBox::OnCreate()
{
    BackTex = FindTexture(TEXT("L2UI.EtcWnd.Popup"));
    InputBackTex = FindTexture(TEXT("L2UI.EtcWnd.Popup"));
    KeypadTex = FindTexture(TEXT("sek.cbui99"));

    EditBox = new NCEditBox(3, NULL, 1, 150);
    EditBox->ID = NID_DIALOG_EDIT;
    EditBox->SetWindowRect(44, 41, 116, 16);
    CreateChild(EditBox, NWS_VISIBLE);

    // the keypad right of the box: 1..9 in rows of three, then 0 (double width) and backspace
    INT X = 6;
    INT Y = 8;
    INT W = 16;
    for (INT i = 0; i < 11; i++)
    {
        if (i >= 0 && i < 9)
        {
            if (i % 3 == 0)
            {
                X = 6;
                if (i != 0)
                    Y += 18;
            }
            else
                X += 18;
        }
        else if (i == 9)
        {
            Y += 18;
            X = 6;
            W = 34;
        }
        else if (i == 10)
        {
            X += 36;
            W = 16;
        }
        Keypad[i] = new NCButton(GKeypadTextures[i][0], GKeypadTextures[i][1], 0);
        Keypad[i]->ID = NID_DIALOG_KEYPAD;
        Keypad[i]->NotifyID = i;
        Keypad[i]->SetWindowRect(X + 205, Y, W, 16);
        CreateChild(Keypad[i], NWS_VISIBLE);
    }

    Progress = new NCProgressCtrl();
    Progress->ID = NID_DIALOG_PROGRESS;
    Progress->SetWindowRect(20, 3, 168, 6);
    CreateChild(Progress, NWS_VISIBLE);
    Progress->SetMax(10000);
    Progress->Reset();

    OKButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                            TEXT("L2UI.Control.DefaultButton_click"), 0);
    OKButton->ID = NID_DIALOG_OK;
    OKButton->SetWindowRect(20, 65, 67, 19);
    CreateChild(OKButton, NWS_VISIBLE);

    CancelButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    CancelButton->ID = NID_DIALOG_CANCEL;
    CancelButton->SetWindowRect(121, 65, 67, 19);
    CreateChild(CancelButton, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10006390, NCDialogBox, OnCreate)

// 0x10006970 (vslot 36)
INT NCDialogBox::OnDestroy()
{
    for (INT i = 0; i < 11; i++)
    {
        if (Keypad[i])
        {
            RemoveChildWindow(Keypad[i]->Entry);
            delete Keypad[i];
            Keypad[i] = NULL;
        }
    }
    if (OKButton)
    {
        RemoveChildWindow(OKButton->Entry);
        delete OKButton;
        OKButton = NULL;
    }
    if (CancelButton)
    {
        RemoveChildWindow(CancelButton->Entry);
        delete CancelButton;
        CancelButton = NULL;
    }
    if (EditBox)
    {
        RemoveChildWindow(EditBox->Entry);
        delete EditBox;
        EditBox = NULL;
    }
    if (Progress)
    {
        RemoveChildWindow(Progress->Entry);
        delete Progress;
        Progress = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10006970, NCDialogBox, OnDestroy)

// 0x10007f70 (vslot 42)
void NCDialogBox::ShowWindow()
{
    NCWnd::ShowWindow();
    UpdateLayout();
}
NW_HOOK_METHOD(0x10007f70, NCDialogBox, ShowWindow)

// 0x10005da0 (vslot 43)
void NCDialogBox::HideWindow()
{
    Reset();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x10005da0, NCDialogBox, HideWindow)

// 0x100067b0 (vslot 50) - the message may carry escaped newlines
INT NCDialogBox::OnPaint(NCGDevice* Device)
{
    guard(NCDialogBox::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (bInput)
    {
        if (InputBackTex)
            Device->DrawTexture(0, 0, 205, Height, 0, 0, 205, Height, InputBackTex);
        if (KeypadTex)
            Device->DrawTexture(205, 0, 64, Height, 0, 0, 64, Height, KeypadTex);
    }
    else if (BackTex)
        Device->DrawTexture(0, 0, 205, Height, 0, 0, 205, Height, BackTex);
    FString Text;
    GConsole->ConvertNewLines(Message, Text);
    TCHAR Buffer[512];
    // the stock code passes the FString itself: %s reads its data pointer (NULL when empty)
    appSprintf(Buffer, TEXT("%s"), Text.GetCharData());
    Device->DrawText(28, bInput ? 12 : 14, (DWORD)-1, Buffer, 168, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                     0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100067b0, NCDialogBox, OnPaint)

// 0x10005d10 (vslot 53) - Enter answers yes only where something was typed or the box is a
// notice
INT NCDialogBox::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN)
    {
        if (bInput || DialogID == 0x1D || DialogID == 0x1F || DialogID == 0x10)
            OnOK();
        else
            OnCancel();
    }
    return 1;
}
NW_HOOK_METHOD(0x10005d10, NCDialogBox, OnKeyDown)

// 0x10005db0 (vslot 65) - Enter in the edit line (1), or on an empty one (0)
INT NCDialogBox::OnEditNotify(INT Code)
{
    if (Code == 1)
    {
        OnOK();
        return 1;
    }
    if (Code == 0)
        OnCancel();
    return 1;
}
NW_HOOK_METHOD(0x10005db0, NCDialogBox, OnEditNotify)

// 0x10007eb0 (vslot 66)
INT NCDialogBox::OnKeypad(INT Key)
{
    TCHAR Digit[10];
    if (Key >= 0 && Key < 9)
    {
        appSprintf(Digit, TEXT("%d"), Key + 1);
        EditBox->InsertText(Digit, 1);
        return 1;
    }
    if (Key == 9)
    {
        appSprintf(Digit, TEXT("%d"), 0);
        EditBox->InsertText(Digit, 1);
        return 1;
    }
    if (Key == 10)
        SendTo(EditBox->Entry, WM_KEYDOWN, VK_BACK, 0);
    return 1;
}
NW_HOOK_METHOD(0x10007eb0, NCDialogBox, OnKeypad)

// 0x10006220 (vslot 67) - time is up
INT NCDialogBox::OnProgressEnd()
{
    OnCancel();
    return 1;
}
NW_HOOK_METHOD(0x10006220, NCDialogBox, OnProgressEnd)

// 0x10005d60 - ParamText and the params are kept. Not inlined: the stock callers call it.
__declspec(noinline) void NCDialogBox::Reset()
{
    DialogID = -1;
    bInput = 0;
    Owner = NULL;
    Message[0] = 0;
    bProgress = 0;
    EditBox->ClearText();
}
NW_HOOK_METHOD(0x10005d60, NCDialogBox, Reset)

// 0x100061a0
void NCDialogBox::SetButtonTexts(INT OKTextID, INT CancelTextID)
{
    if (OKButton && *GL2GameData.SysString[OKTextID])
        OKButton->SetTextID(OKTextID);
    if (CancelButton && *GL2GameData.SysString[CancelTextID])
        CancelButton->SetTextID(CancelTextID);
}
NW_HOOK_METHOD(0x100061a0, NCDialogBox, SetButtonTexts)

// 0x10006210
NWndEntry* NCDialogBox::GetOwner()
{
    return Owner;
}
NW_HOOK_METHOD(0x10006210, NCDialogBox, GetOwner)

// 0x10006a70 - 203 x 87, with the keypad 269 wide; notices have OK alone, centered
void NCDialogBox::UpdateLayout()
{
    SetSize(203, 87);
    // the stock code fills { LocalX, LocalY, LocalW, LocalH }; SetScreenPos reads x and y
    POINT Origin;
    Origin.x = LocalX;
    Origin.y = LocalY;
    OKButton->SetScreenPos(27, 60, &Origin);
    CancelButton->SetScreenPos(114, 60, &Origin);
    if (bInput)
    {
        if (Type == 3)
            SetSize(269, 87);
        else
        {
            SetSize(203, 87);
            for (INT i = 0; i < 11; i++)
                Keypad[i]->HideWindow();
        }
    }
    else
    {
        SetSize(203, 87);
        EditBox->HideWindow();
        for (INT i = 0; i < 11; i++)
            Keypad[i]->HideWindow();
    }
    if (DialogID == 0x1D || DialogID == 0x1F || DialogID == 0x10)
    {
        CancelButton->HideWindow();
        OKButton->SetScreenPos((INT)((DWORD)Width >> 1) - 27, 60, &Origin);
    }
    if (bProgress)
    {
        Progress->ShowWindow();
        if (DialogID == 0x13 || DialogID == 0x30)
            Progress->SetMax(900000);
        else
            Progress->SetMax(10000);
    }
    else
        Progress->HideWindow();
}
NW_HOOK_METHOD(0x10006a70, NCDialogBox, UpdateLayout)

// 0x10006c10 - Text2 wins over Text; an empty ParamText or a zero param keeps the box's
void NCDialogBox::SetInfo(NDialogInfo* Info)
{
    Reset();
    DialogID = Info->ID;
    Type = Info->Type;
    EditBox->SetType(Info->Type);
    bInput = Info->bInput;
    bLargeAmount = Info->bLargeAmount;
    Owner = Info->Owner;
    bProgress = Info->bProgress;
    if (bProgress)
    {
        Progress->ShowWindow();
        Progress->Reset();
        Progress->Start();
    }
    else
        Progress->HideWindow();
    if (Info->Text2[0])
        appStrcpy(Message, Info->Text2);
    else if (Info->Text[0])
        appStrcpy(Message, Info->Text);
    if (Info->ParamText[0])
        appStrcpy(ParamText, Info->ParamText);
    if (Info->Param1)
        Param1 = Info->Param1;
    if (Info->Param2)
        Param2 = Info->Param2;
}
NW_HOOK_METHOD(0x10006c10, NCDialogBox, SetInfo)

/*-----------------------------------------------------------------------------
    The answers.
-----------------------------------------------------------------------------*/

// the amount typed, 0 .. 1,000,000 (100,000,000 with bLargeAmount)
FORCEINLINE INT NCDialogBox::GetAmount()
{
    INT Amount = NWtol(EditBox->CommitText());
    INT Max = bLargeAmount ? 100000000 : 1000000;
    if (Amount < 0)
        Amount = 0;
    else if (Amount > Max)
        Amount = Max;
    return Amount;
}

// the text typed is refused: a system message, and the box stays open for another try
FORCEINLINE INT NCDialogBox::RejectText(INT MsgID)
{
    GConsole->ShowSystemMessage(MsgID, 0xFFB09B79);
    EditBox->ClearText();
    EditBox->SetFocus();
    return 1;
}

// system message 80: "Please insert 1~16 English characters."
FORCEINLINE INT NCDialogBox::TooLongName()
{
    GConsole->ShowSystemMessage(80, 0xFFB09B79);
    EditBox->SetFocus();
    return 1;
}

// the name typed as the stock code measures it: bytes in the ANSI code page
static FORCEINLINE INT AnsiLength(const TCHAR* Text)
{
    ANSICHAR Ansi[512];
    WideCharToMultiByte(CP_ACP, 0, Text, -1, Ansi, 512, NULL, NULL);
    return (INT)strlen(Ansi);
}

// a request with one argument, the text typed
#define TEXT_REQUEST(Request)                                                                  \
    {                                                                                          \
        L2ParamStack Params(10);                                                               \
        Params.PushBack((void*)EditBox->CommitText());                                         \
        GConsole->Request(Params);                                                             \
    }
// a request with one number
#define VALUE_REQUEST(Request, Value)                                                          \
    {                                                                                          \
        L2ParamStack Params(10);                                                               \
        Params.PushBack((void*)(INT_PTR)(Value));                                              \
        GConsole->Request(Params);                                                             \
    }
// a request with ParamText and a number
#define NAMED_REQUEST(Request, Value)                                                          \
    {                                                                                          \
        L2ParamStack Params(10);                                                               \
        Params.PushBack(ParamText);                                                            \
        Params.PushBack((void*)(INT_PTR)(Value));                                              \
        GConsole->Request(Params);                                                             \
    }
// a request with Param1 (and Param2) and flags
#define PARAM_REQUEST3(Request, A, B, C)                                                       \
    {                                                                                          \
        L2ParamStack Params(10);                                                               \
        Params.PushBack((void*)(INT_PTR)(A));                                                  \
        Params.PushBack((void*)(INT_PTR)(B));                                                  \
        Params.PushBack((void*)(INT_PTR)(C));                                                  \
        GConsole->Request(Params);                                                             \
    }

// 0x10006d10 (vslot 63) - OK. The box closes afterwards unless noted.
INT NCDialogBox::OnOK()
{
    if (bProgress)
        Progress->Stop();
    switch (DialogID)
    {
    case 0: // yes to the owner
    case 1:
        if (Owner)
            SendTo(Owner, NWM_DIALOG_RESULT, 1, 0);
        break;
    case 2: // amounts for the owner (items, adena...); without an edit line, yes
    case 3:
    case 4:
    case 8:
    case 0x1E:
    case 0x32:
    case 0x33:
    case 0x34:
        if (!Owner)
            break;
        if (!bInput)
        {
            SendTo(Owner, NWM_DIALOG_RESULT, 1, 0);
            break;
        }
        if (!EditBox->CommitText())
            break;
        SendTo(Owner, NWM_DIALOG_RESULT, GetAmount(), 0);
        break;
    case 5: // the same, with a sound
    case 6:
        if (!Owner)
            break;
        if (bInput)
        {
            if (!EditBox->CommitText())
                break;
            SendTo(Owner, NWM_DIALOG_RESULT, GetAmount(), 0);
        }
        else
            SendTo(Owner, NWM_DIALOG_RESULT, 1, 0);
        GConsole->Engine->OnInterfacePlaySound(4);
        break;
    case 7: // trade request
        VALUE_REQUEST(AnswerTradeRequest, 1)
        break;
    case 0xA: // party invitation
        VALUE_REQUEST(RequestAnswerJoinParty, 1)
        break;
    case 0xB:
        TEXT_REQUEST(RequestNet103)
        break;
    case 0xC:
        GConsole->RequestNet106();
        break;
    case 0xE: // clan invitation
        VALUE_REQUEST(RequestAnswerJoinPledge, 1)
        break;
    case 0xF: // title for the selected clan member
    {
        User* Target = GConsole->GetSelectedActor();
        if (!Target || Target->Unknown8)
            break;
        INT Len = AnsiLength(EditBox->CommitText());
        if (!GConsole->HasInvalidChars((TCHAR*)EditBox->CommitText()) &&
            GConsole->IsValidName((TCHAR*)EditBox->CommitText()))
        {
            if (Len > 16)
                return TooLongName();
            L2ParamStack Params(10);
            Params.PushBack(Target->GetName());
            Params.PushBack((void*)EditBox->CommitText());
            GConsole->RequestGiveNickName(Params);
            break;
        }
        return RejectText(204);
    }
    case 0x10: // clan crest notice: then the file name (dialog 0x11)
    {
        NDialogInfo Info;
        Info.Owner = Entry;
        Info.ID = 0x11;
        Info.bMainBox = 1;
        Info.bInput = 1;
        Info.Type = 0;
        Info.SetSystemMessage(208);
        GConsole->ShowDialog(&Info);
        return 1;
    }
    case 0x11:
        TEXT_REQUEST(RegisterPledgeCrest)
        break;
    case 0x12: // clan war: declare, answer, stop, answer, surrender
        TEXT_REQUEST(RequestStartPledgeWar)
        break;
    case 0x13:
        NAMED_REQUEST(RequestReplyStartPledgeWar, 1)
        break;
    case 0x14:
        TEXT_REQUEST(RequestStopPledgeWar)
        break;
    case 0x15:
        NAMED_REQUEST(RequestReplyStopPledgeWar, 1)
        break;
    case 0x17:
        TEXT_REQUEST(RequestSurrenderPledgeWar)
        break;
    case 0x18: // expel a member
    {
        L2ParamStack Params(10);
        Params.PushBack(ParamText);
        GConsole->RequestOustPledgeMember(Params);
        break;
    }
    case 0x19:
        TEXT_REQUEST(RequestSurrenderPersonally)
        break;
    case 0x1A: // leave the clan
        GConsole->RequestWithdrawalPledge();
        break;
    case 0x1B: // restart / quit (the system menu)
        if (Owner)
            SendTo(Owner, NWM_DIALOG_RESULT, 0x1B, 0);
        break;
    case 0x1C:
        if (Owner)
            SendTo(Owner, NWM_DIALOG_RESULT, 0x1C, 0);
        break;
    case 0x1D: // disconnected: the game ends
        GConsole->QuitGame();
        break;
    case 0x20: // revival
        GConsole->AnswerReviveRequest(1, Param1, Param2);
        break;
    case 0x21: // private store (sell) amounts; the box stays when the owner refuses
        if (!Owner)
            break;
        if (!bInput)
        {
            SendTo(Owner, 0xF0000006, 1, 0);
            break;
        }
        if (!EditBox->CommitText())
            break;
        if (!SendTo(Owner, 0xF0000006, GetAmount(), 0))
            return 1;
        break;
    case 0x22:
        if (!Owner)
            break;
        if (bInput)
        {
            if (!EditBox->CommitText())
                break;
            SendTo(Owner, 0xF0000007, GetAmount(), 0);
        }
        else
            SendTo(Owner, 0xF0000007, 1, 0);
        break;
    case 0x23: // private store message
        if (!EditBox->CommitText())
            break;
        if (GConsole->HasInvalidChars((TCHAR*)EditBox->CommitText()) ||
            GConsole->HasBadWord((TCHAR*)EditBox->CommitText()))
            return RejectText(786);
        GConsole->SetPrivateStoreMsg((TCHAR*)EditBox->CommitText());
        break;
    case 0x24:
        if (!Owner)
            break;
        if (SendTo(Owner, 0xF0000008, 0, 0))
        {
            Reset();
            HideWindow();
        }
        return 1;
    case 0x25:
        if (Owner)
            SendTo(Owner, 0xF000000A, 0, 0);
        break;
    case 0x26: // private store (buy) amounts
        if (!Owner)
            break;
        if (!bInput)
        {
            SendTo(Owner, 0xF000000B, 1, 0);
            break;
        }
        if (!EditBox->CommitText())
            break;
        if (!SendTo(Owner, 0xF000000B, GetAmount(), 0))
            return 1;
        break;
    case 0x27:
        if (!Owner)
            break;
        if (bInput)
        {
            if (!EditBox->CommitText())
                break;
            SendTo(Owner, 0xF000000C, GetAmount(), 0);
        }
        else
            SendTo(Owner, 0xF000000C, 1, 0);
        break;
    case 0x28: // private store (buy) message
        if (!EditBox->CommitText())
            break;
        if (GConsole->HasInvalidChars((TCHAR*)EditBox->CommitText()) ||
            GConsole->HasBadWord((TCHAR*)EditBox->CommitText()))
            return RejectText(786);
        GConsole->SetPrivateStoreBuyMsg((TCHAR*)EditBox->CommitText());
        break;
    case 0x29:
        if (!Owner)
            break;
        if (!SendTo(Owner, 0xF000000D, 0, 0))
            return 1;
        break;
    case 0x2A:
        if (Owner)
            SendTo(Owner, 0xF000000F, 0, 0);
        break;
    case 0x2B: // alliance: crest, dismiss, leave, expel a clan
        if (!EditBox->CommitText())
            break;
        TEXT_REQUEST(RegisterAllyCrest)
        break;
    case 0x2C:
        GConsole->RequestDismissAlly();
        break;
    case 0x2D:
        GConsole->RequestWithdrawAlly();
        break;
    case 0x2E:
        if (!EditBox->CommitText())
            break;
        TEXT_REQUEST(RequestOustAlly)
        break;
    case 0x2F: // alliance invitation
        VALUE_REQUEST(RequestAnswerJoinAlly, 1)
        break;
    case 0x30: // alliance war: start / stop asked by the other side
        NAMED_REQUEST(RequestReplyStartAllianceWar, 1)
        break;
    case 0x31:
        NAMED_REQUEST(RequestReplyStopAllianceWar, 1)
        break;
    case 0x35: // pet name
    {
        INT Len = AnsiLength(EditBox->CommitText());
        if (!GConsole->HasInvalidChars((TCHAR*)EditBox->CommitText()) &&
            GConsole->IsValidName((TCHAR*)EditBox->CommitText()))
        {
            if (Len > 16)
                return TooLongName();
            GConsole->RequestChangePetName((TCHAR*)EditBox->CommitText());
            break;
        }
        return RejectText(591);
    }
    case 0x36: // action 20, with the modifier keys held
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)20);
        Params.PushBack((void*)(INT_PTR)GConsole->IsCtrlDown());
        Params.PushBack((void*)(INT_PTR)GConsole->Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
        GConsole->RequestActionUse(Params);
        break;
    }
    case 0x37: // friend request
        VALUE_REQUEST(RequestAnswerFriendInvite, 1)
        break;
    case 0x38:
        if (Owner)
            SendTo(Owner, 0xF0000010, 0, 0);
        break;
    case 0x39:
        if (Owner)
            SendTo(Owner, 0xF0000011, 0, 0);
        break;
    case 0x3A: // siege: register / cancel as attacker / defender (castle in Param1)
        PARAM_REQUEST3(RequestJoinSiege, Param1, 1, 1)
        break;
    case 0x3B:
        PARAM_REQUEST3(RequestJoinSiege, Param1, 1, 0)
        break;
    case 0x3C:
        PARAM_REQUEST3(RequestJoinSiege, Param1, 0, 1)
        break;
    case 0x3D:
        PARAM_REQUEST3(RequestJoinSiege, Param1, 0, 0)
        break;
    case 0x3E: // siege: approve / refuse a defending clan
        PARAM_REQUEST3(RequestConfirmSiegeWaitingList, Param1, Param2, 1)
        break;
    case 0x3F:
        PARAM_REQUEST3(RequestConfirmSiegeWaitingList, Param1, Param2, 0)
        break;
    }
    Reset();
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10006d10, NCDialogBox, OnOK)

// 0x10005de0 (vslot 64) - Cancel: the refusals; the box closes
INT NCDialogBox::OnCancel()
{
#if NW_FEATURES
    // phase 6: ReturnToLogin (NWindow_C1 src/inject/ReturnToLogin.cpp): Esc on the
    // "disconnected" box goes back to the login screen too (stock: the box closes over a dead
    // world); the id is gone once the box is reset
    const INT ClosedID = DialogID;
#endif
    if (bProgress)
        Progress->Stop();
    switch (DialogID)
    {
    case 1:
        if (Owner)
            SendTo(Owner, NWM_DIALOG_RESULT, 0, 0);
        break;
    case 7:
        VALUE_REQUEST(AnswerTradeRequest, 0)
        break;
    case 0xA:
        VALUE_REQUEST(RequestAnswerJoinParty, 0)
        break;
    case 0xE:
        VALUE_REQUEST(RequestAnswerJoinPledge, 0)
        break;
    case 0x13:
        NAMED_REQUEST(RequestReplyStartPledgeWar, 0)
        break;
    case 0x15:
        VALUE_REQUEST(RequestReplyStopPledgeWar, 0)   // no clan name, unlike OK
        break;
    case 0x20:
        GConsole->AnswerReviveRequest(0, Param1, Param2);
        break;
    case 0x25:
        if (Owner)
            Owner->Wnd->HideWindow();
        break;
    case 0x2F:
        VALUE_REQUEST(RequestAnswerJoinAlly, 0)
        break;
    case 0x30:
        NAMED_REQUEST(RequestReplyStartAllianceWar, 0)
        break;
    case 0x31:
        NAMED_REQUEST(RequestReplyStopAllianceWar, 0)
        break;
    case 0x37:
        VALUE_REQUEST(RequestAnswerFriendInvite, 0)
        // falls through: the owner is told too
    case 0x24:
        if (Owner)
            SendTo(Owner, 0xF0000009, 0, 0);
        break;
    }
    Reset();
    HideWindow();
#if NW_FEATURES
    // phase 6: ReturnToLogin (NWindow_C1 src/inject/ReturnToLogin.cpp)
    nwf::ReturnToLogin_DialogCancelled(ClosedID);
#endif
    return 1;
}
NW_HOOK_METHOD(0x10005de0, NCDialogBox, OnCancel)

#undef TEXT_REQUEST
#undef VALUE_REQUEST
#undef NAMED_REQUEST
#undef PARAM_REQUEST3

NW_CHECK_VTABLE(NCDialogBox, 0x100d9510, 68)
