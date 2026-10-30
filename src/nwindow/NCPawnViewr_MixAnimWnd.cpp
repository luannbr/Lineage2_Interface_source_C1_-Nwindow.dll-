/*=============================================================================
    NCPawnViewr_MixAnimWnd.cpp - the pawn viewer's page of combined animations. Stock:
    0x10088260 .. 0x1008844a, 0x10089620 .. 0x10089f06.
=============================================================================*/
#include "nwindow/NCPawnViewrWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <stdlib.h>

// NWM_COMMAND ids (names ours); the edit lines are 0xFF..0x101
enum
{
    NID_MIXANIM_TAKE1 = 0xFB,
    NID_MIXANIM_TAKE2 = 0xFC,
    NID_MIXANIM_TAKE3 = 0xFD,
    NID_MIXANIM_START = 0xFE,
    NID_MIXANIM_EDIT = 0xFF,
};

// sysstring lines
enum { STR_Start = 428, STR_Use = 443 };

// a child window goes (repeated inline in the stock OnDestroys)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// the animation selected in the character window's list into an edit line (inline in each
// "Use" button's handler, name ours)
static FORCEINLINE void TakeSelectedAnim(NCEditBox* Edit)
{
    NCPawnView_CharacterSelect_List* List = NPawnView::Viewer()->CharSelectWnd->AnimList;
    if (List && List->Selected >= 0)
    {
        Edit->ClearText();
        Edit->InsertText((const TCHAR*)List->GetItemParam(List->Selected), 0);
    }
}

// 0x10089620
NCPawnViewr_MixAnimWnd::NCPawnViewr_MixAnimWnd()
{
    TakeButtons[0] = NULL;
    TakeButtons[1] = NULL;
    TakeButtons[2] = NULL;
    StartButton = NULL;
}
NW_HOOK_CTOR(0x10089620, NCPawnViewr_MixAnimWnd)

// 0x10089680
NW_HOOK_DTOR(0x10089680, NCPawnViewr_MixAnimWnd)

// 0x10088260 (vslot 24)
INT NCPawnViewr_MixAnimWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_MIXANIM_TAKE1)
            return OnTake1() != 0;
        if (wParam == NID_MIXANIM_TAKE2)
            return OnTake2() != 0;
        if (wParam == NID_MIXANIM_TAKE3)
            return OnTake3() != 0;
        if (wParam == NID_MIXANIM_START)
            return OnStart() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10088260, NCPawnViewr_MixAnimWnd, WindowProc)

// 0x100896d0 (vslot 35) - "Use" beside each animation line (disabled: only the buttons fill
// them), the speed ("1.9") and "Start"
INT NCPawnViewr_MixAnimWnd::OnCreate()
{
    NCWnd::OnCreate();
    for (INT i = 0; i < 3; i++)
    {
        TakeButtons[i] = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                      TEXT("L2UI.Control.DefaultButton_click"), 0);
        TakeButtons[i]->ID = NID_MIXANIM_TAKE1 + i;
        TakeButtons[i]->SetWindowRect(20, 25 + 30 * i, 28, 16);
        CreateChild(TakeButtons[i], NWS_VISIBLE);
        TakeButtons[i]->SetTextID(STR_Use);
    }
    StartButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                               TEXT("L2UI.Control.DefaultButton_click"), 0);
    StartButton->ID = NID_MIXANIM_START;
    StartButton->SetWindowRect(120, 140, 38, 16);
    CreateChild(StartButton, NWS_VISIBLE);
    StartButton->SetTextID(STR_Start);
    for (INT i = 0; i < 3; i++)
    {
        AnimEdits[i] = new NCEditBox(1, NULL, 1, 64);
        AnimEdits[i]->ID = NID_MIXANIM_EDIT + i;
        AnimEdits[i]->SetWindowRect(75, 25 + 30 * i, 140, 16);
        CreateChild(AnimEdits[i], NWS_VISIBLE);
        AnimEdits[i]->EnableWindow(0);
    }
    SpeedEdit = new NCEditBox(1, NULL, 1, 64);
    SpeedEdit->SetWindowRect(65, 140, 35, 16);
    CreateChild(SpeedEdit, NWS_VISIBLE);
    SpeedEdit->InsertText(TEXT("1.9"), 1);
    return 1;
}
NW_HOOK_METHOD(0x100896d0, NCPawnViewr_MixAnimWnd, OnCreate)

// 0x10089dc0 (vslot 36) - the speed line is left alone
INT NCPawnViewr_MixAnimWnd::OnDestroy()
{
    DestroyChild(this, TakeButtons[0]);
    DestroyChild(this, TakeButtons[1]);
    DestroyChild(this, TakeButtons[2]);
    DestroyChild(this, StartButton);
    DestroyChild(this, AnimEdits[0]);
    DestroyChild(this, AnimEdits[1]);
    DestroyChild(this, AnimEdits[2]);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10089dc0, NCPawnViewr_MixAnimWnd, OnDestroy)

// 0x10089cb0 (vslot 50) - "HITTIME" centered in a 50 x 16 box left of the speed; the origin
// goes on before the clip
INT NCPawnViewr_MixAnimWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPawnViewr_MixAnimWnd::OnPaint);
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    NCGDevice::GetTextExtent(&Size, TEXT("HITTIME"), 0, 0, 0, 0, 0);
    Device->DrawText((50 - Size.cx) / 2 + 10, (16 - Size.cy) / 2 + 140, 0xFFFFFFFF,
                     TEXT("HITTIME"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10089cb0, NCPawnViewr_MixAnimWnd, OnPaint)

// 0x10088330 (vslot 62)
INT NCPawnViewr_MixAnimWnd::OnTake1()
{
    TakeSelectedAnim(AnimEdits[0]);
    return 1;
}
NW_HOOK_METHOD(0x10088330, NCPawnViewr_MixAnimWnd, OnTake1)

// 0x10088390 (vslot 63)
INT NCPawnViewr_MixAnimWnd::OnTake2()
{
    TakeSelectedAnim(AnimEdits[1]);
    return 1;
}
NW_HOOK_METHOD(0x10088390, NCPawnViewr_MixAnimWnd, OnTake2)

// 0x100883f0 (vslot 64)
INT NCPawnViewr_MixAnimWnd::OnTake3()
{
    TakeSelectedAnim(AnimEdits[2]);
    return 1;
}
NW_HOOK_METHOD(0x100883f0, NCPawnViewr_MixAnimWnd, OnTake3)

// 0x10089b10 (vslot 65) - the three animations as the steps of a skill of the player's pawn
// on the player's target (the last line plays first), at the speed typed (read with the C
// runtime's _wtof, unlike the simulator's appAtof)
INT NCPawnViewr_MixAnimWnd::OnStart()
{
    if (!SpeedEdit->CommitText())
        return 1;
    FLOAT Speed = (FLOAT)_wtof(SpeedEdit->CommitText());
    if (!AnimEdits[0]->CommitText() || !AnimEdits[1]->CommitText() ||
        !AnimEdits[2]->CommitText() || !(Speed > 0.0f))
        return 1;
    APlayerController* Controller = ViewActor();
    APawn* Pawn = Controller->Pawn;
    AActor* Target = Controller->Target;
    Pawn->Controller->StopMove();
    Pawn->SetMagicInfo(-1, 1, Speed, Speed, Target, 1.0f);
    Pawn->Unknown578 = 9;
    Pawn->Unknown580 = 3;
    TArray<FName>& Steps = Pawn->MixAnimSteps;
    Steps.Empty();
    Steps.AddZeroed(3);
    Steps(2) = FName(AnimEdits[0]->CommitText(), FNAME_Add);
    Steps(1) = FName(AnimEdits[1]->CommitText(), FNAME_Add);
    Steps(0) = FName(AnimEdits[2]->CommitText(), FNAME_Add);
    if (Pawn->InitSkillProcess())
    {
        Pawn->ActionType = 5;
        Pawn->ActionTarget = Target;
    }
    return 1;
}
NW_HOOK_METHOD(0x10089b10, NCPawnViewr_MixAnimWnd, OnStart)

NW_CHECK_VTABLE(NCPawnViewr_MixAnimWnd, 0x100eaf20, 66)
