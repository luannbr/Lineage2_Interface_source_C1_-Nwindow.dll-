/*=============================================================================
    NCMatchOptionWnd.cpp - party matching options. Stock: 0x10073d30 .. 0x10073db6,
    0x10074600 .. 0x10074631, 0x10074670 .. 0x10074ffe, 0x10075c80 .. 0x10075c8f
    (OnCancel's body is shared at 0x10059d00).
=============================================================================*/
#include "nwindow/NCMatchOptionWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCMultiEdit.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <string.h>

// NWM_COMMAND ids (names ours)
enum
{
    NID_MATCHOPT_LEVELOPEN = 0xF9,
    NID_MATCHOPT_CLASSOPEN = 0xF10,
    NID_MATCHOPT_AUTOREGISTER = 0xF11,
    NID_MATCHOPT_MEMO = 0xB00F12,     // as in the stock code
    NID_MATCHOPT_OK = 0xF13,
    NID_MATCHOPT_CANCEL = 0xF14,
};

enum { STR_AutoRegister = 400, STR_LevelOpen = 401, STR_ClassOpen = 402, STR_Memo = 403 };
enum { MSG_MemoTooLong = 327 };  // "You cannot exceed 80 characters."

#define MATCH_OPTION_INI TEXT("MatchOption.ini")

// one check box of the window (repeated inline in the stock OnCreate)
static NCCheckBox* CreateOptionBox(NCWnd* Parent, INT TextID, INT ID, INT Y)
{
    NCCheckBox* Box = new NCCheckBox(TextID, 0, 0);
    Box->ID = ID;
    Box->SetWindowRect(17, Y, 200, 12);
    Parent->CreateChild(Box, NWS_VISIBLE);
    return Box;
}

// one child of OnDestroy (repeated inline in the stock code)
template <class T> static void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x10074600
NCMatchOptionWnd::NCMatchOptionWnd()
{
    BackTex = NULL;
    LevelOpenCheck = NULL;
    ClassOpenCheck = NULL;
    AutoRegisterCheck = NULL;
    MemoEdit = NULL;
}
NW_HOOK_CTOR(0x10074600, NCMatchOptionWnd)

// 0x10074670
NW_HOOK_DTOR(0x10074670, NCMatchOptionWnd)

// 0x10073d30 (vslot 24)
INT NCMatchOptionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_MATCHOPT_OK)
            return OnOK() != 0;
        if ((INT)wParam == NID_MATCHOPT_CANCEL)
            return OnCancel() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10073d30, NCMatchOptionWnd, WindowProc)

// 0x100746c0 (vslot 35)
INT NCMatchOptionWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.MatchWnd.option"));
    AutoRegisterCheck = CreateOptionBox(this, STR_AutoRegister, NID_MATCHOPT_AUTOREGISTER, 13);
    LevelOpenCheck = CreateOptionBox(this, STR_LevelOpen, NID_MATCHOPT_LEVELOPEN, 30);
    ClassOpenCheck = CreateOptionBox(this, STR_ClassOpen, NID_MATCHOPT_CLASSOPEN, 47);

    MemoEdit = new NCMultiEdit;
    MemoEdit->ID = NID_MATCHOPT_MEMO;
    MemoEdit->SetWindowRect(15, 83, 209, 42);
    CreateChild(MemoEdit, NWS_VISIBLE);

    OKButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                            TEXT("L2UI.Control.DefaultButton_click"), 0);
    OKButton->ID = NID_MATCHOPT_OK;
    OKButton->SetWindowRect(87, 139, 67, 19);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(0x8C);

    CancelButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    CancelButton->ID = NID_MATCHOPT_CANCEL;
    CancelButton->SetWindowRect(163, 139, 67, 19);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(0x8D);
    return 1;
}
NW_HOOK_METHOD(0x100746c0, NCMatchOptionWnd, OnCreate)

// 0x10074ad0 (vslot 36)
INT NCMatchOptionWnd::OnDestroy()
{
    DestroyChild(this, LevelOpenCheck);
    DestroyChild(this, ClassOpenCheck);
    DestroyChild(this, AutoRegisterCheck);
    DestroyChild(this, MemoEdit);
    DestroyChild(this, OKButton);
    DestroyChild(this, CancelButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10074ad0, NCMatchOptionWnd, OnDestroy)

// 0x10075c80 (vslot 42)
void NCMatchOptionWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    LoadOptions();
}
NW_HOOK_METHOD(0x10075c80, NCMatchOptionWnd, ShowWindow)

// 0x10074bf0 (vslot 50) - the clip is set before BeginPaint and released after EndPaint, as
// in the stock code
INT NCMatchOptionWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, 240, 161, 0, 0, 240, 161, BackTex);
    if (*GL2GameData.SysString[STR_Memo])
        Device->DrawText(15, 65, 0xFFFFFFFF, *GL2GameData.SysString[STR_Memo], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10074bf0, NCMatchOptionWnd, OnPaint)

// 0x10074cd0 (vslot 62) - saves the options under the character's name and sends them. The
// memo is checked by converting it into 80 bytes: only a conversion that did not fit (no
// terminator) measures more than 80, and then nothing is saved and the window stays open.
INT NCMatchOptionWnd::OnOK()
{
    if (GConsole->GetMyUser())
    {
        const TCHAR* Section = GConsole->GetMyUser()->GetName();
        bAutoRegister = AutoRegisterCheck->bChecked;
        bLevelOpen = LevelOpenCheck->bChecked;
        bClassOpen = ClassOpenCheck->bChecked;
        TCHAR* Memo = MemoEdit->GetText();
        if (Memo)
        {
            ANSICHAR Ansi[80];
            WideCharToMultiByte(CP_ACP, 0, Memo, -1, Ansi, 80, NULL, NULL);
            if (strlen(Ansi) > 80)
            {
                GConsole->ShowSystemMessage(MSG_MemoTooLong, 0xFFB09B79);
                return 1;
            }
            GConfig->SetString(Section, TEXT("memo"), Memo, MATCH_OPTION_INI);
        }
        else
            GConfig->EmptySection(Section, MATCH_OPTION_INI);
        GConfig->SetBool(Section, TEXT("autoregister"), bAutoRegister, MATCH_OPTION_INI);
        GConfig->SetBool(Section, TEXT("levelopen"), bLevelOpen, MATCH_OPTION_INI);
        GConfig->SetBool(Section, TEXT("classopen"), bClassOpen, MATCH_OPTION_INI);
        L2ParamStack Params(10);
        Params.PushBack((void*)bAutoRegister);
        Params.PushBack((void*)bLevelOpen);
        Params.PushBack((void*)bClassOpen);
        Params.PushBack(Memo);
        GConsole->RequestPartyMatchConfig(Params);
    }
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10074cd0, NCMatchOptionWnd, OnOK)

// stock body shared with NCOptionWnd's and two other windows' (0x10059d00)
INT NCMatchOptionWnd::OnCancel()
{
    HideWindow();
    return 1;
}

// 0x10074ec0 - registration defaults to on; a check box the player cannot change keeps its
// state
void NCMatchOptionWnd::LoadOptions()
{
    TCHAR Memo[256];
    if (!GConsole->GetMyUser())
        return;
    const TCHAR* Section = GConsole->GetMyUser()->GetName();
    if (!GConfig->GetBool(Section, TEXT("autoregister"), bAutoRegister, MATCH_OPTION_INI))
        bAutoRegister = 1;
    if (!GConfig->GetBool(Section, TEXT("levelopen"), bLevelOpen, MATCH_OPTION_INI))
        bLevelOpen = 0;
    if (!GConfig->GetBool(Section, TEXT("classopen"), bClassOpen, MATCH_OPTION_INI))
        bClassOpen = 0;
    if (!GConfig->GetString(Section, TEXT("memo"), Memo, 256, MATCH_OPTION_INI))
        Memo[0] = 0;
    if (!AutoRegisterCheck->bDisabled)
        AutoRegisterCheck->bChecked = bAutoRegister;
    if (!LevelOpenCheck->bDisabled)
        LevelOpenCheck->bChecked = bLevelOpen;
    if (!ClassOpenCheck->bDisabled)
        ClassOpenCheck->bChecked = bClassOpen;
    MemoEdit->Clear();
    MemoEdit->SetText(Memo);
}
NW_HOOK_METHOD(0x10074ec0, NCMatchOptionWnd, LoadOptions)

NW_CHECK_VTABLE(NCMatchOptionWnd, 0x100e74a0, 64)
