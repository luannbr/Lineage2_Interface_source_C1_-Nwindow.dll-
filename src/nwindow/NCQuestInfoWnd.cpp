/*=============================================================================
    NCQuestInfoWnd.cpp - the quest window's description page. Stock: 0x1009fde0 .. 0x1009fe69,
    0x100a04f0 .. 0x100a08f2, 0x100a0960 .. 0x100a0b31, 0x100a13d0 .. 0x100a1484 and
    0x100a1660 .. 0x100a17c0 (the quest window and its item page sit in between).
=============================================================================*/
#include "nwindow/NCQuestInfoWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NConsoleWnd.h"

// ids of the arrows (names ours)
enum
{
    NID_QUESTINFO_NEXT = 0xF8,
    NID_QUESTINFO_PREV = 0xF9,
};

// 0x100a16f0 - the arrows are left as allocated
NCQuestInfoWnd::NCQuestInfoWnd()
{
    Text[0] = 0;
    Current = -1;
}
NW_HOOK_CTOR(0x100a16f0, NCQuestInfoWnd)

// 0x100a1760 - Quests' destructor is TArray<NQuestEntry>'s (0x100a1660)
NW_HOOK_DTOR(0x100a1760, NCQuestInfoWnd)

// 0x1009fde0 (vslot 24)
INT NCQuestInfoWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCScrollWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_QUESTINFO_NEXT)
            return NextStep() != 0;
        if (wParam == NID_QUESTINFO_PREV)
            return PrevStep() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1009fde0, NCQuestInfoWnd, WindowProc)

// 0x100a04f0 (vslot 35) - the arrows sit below the page (y 130 of a 122-pixel window) and
// repeat every 180 ms while held
INT NCQuestInfoWnd::OnCreate()
{
    NCScrollWnd::OnCreate();
    NextButton = new NCPushButton(TEXT("L2UI.ShortcutWnd.RightBtn1"), TEXT("L2UI.ShortcutWnd.RightBtn2"));
    NextButton->ID = NID_QUESTINFO_NEXT;
    NextButton->SetWindowRect(163, 130, 13, 13);
    CreateChild(NextButton, NWS_VISIBLE);
    NextButton->RepeatDelay = 180;
    PrevButton = new NCPushButton(TEXT("L2UI.ShortcutWnd.LeftBtn1"), TEXT("L2UI.ShortcutWnd.LeftBtn2"));
    PrevButton->ID = NID_QUESTINFO_PREV;
    PrevButton->SetWindowRect(30, 130, 13, 13);
    CreateChild(PrevButton, NWS_VISIBLE);
    PrevButton->RepeatDelay = 180;
    return 1;
}
NW_HOOK_METHOD(0x100a04f0, NCQuestInfoWnd, OnCreate)

// 0x100a0680 (vslot 50) - the text is drawn with the origin moved by the scroll position and
// measured after it, which sets the scroll range; the clip opens before BeginPaint
INT NCQuestInfoWnd::OnPaint(NCGDevice* Device)
{
    guard(NCQuestInfoWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (Current >= 0 && Current < Quests.Num() && Text[0])
    {
        Device->PushOrigin(ScrollX, ScrollY);
        Device->DrawText(2, 1, 0xFFFFFFFF, Text, Width - 20, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Text, Width - 20, 0, 0, 0, 0);
        SetContentHeight(Size.cy);
        Device->PopOrigin();
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100a0680, NCQuestInfoWnd, OnPaint)

// 0x100a07d0 (vslot 62) - NCScrollWnd's, but DeltaY is kept when already at the top
INT NCQuestInfoWnd::ScrollUp()
{
    if (ScrollY < 0)
    {
        DeltaX = ScrollX;
        DeltaY = ScrollY;
        ScrollY += LineStep;
        if (ScrollY > 0)
            ScrollY = 0;
        ScrollBar->SetScrollPos(-ScrollY);
        DeltaX -= ScrollX;
        DeltaY -= ScrollY;
    }
    return 1;
}
NW_HOOK_METHOD(0x100a07d0, NCQuestInfoWnd, ScrollUp)

// 0x100a0860 (vslot 63) - NCScrollWnd's, but DeltaY is kept when already at the bottom
INT NCQuestInfoWnd::ScrollDown()
{
    if (ScrollY > MinScrollY)
    {
        DeltaX = ScrollX;
        DeltaY = ScrollY;
        ScrollY -= LineStep;
        if (ScrollY < MinScrollY)
            ScrollY = MinScrollY;
        ScrollBar->SetScrollPos(-ScrollY);
        DeltaX -= ScrollX;
        DeltaY -= ScrollY;
    }
    return 1;
}
NW_HOOK_METHOD(0x100a0860, NCQuestInfoWnd, ScrollDown)

// 0x100a0960 - at the step the player is at; the text empties when the quest data has none
void NCQuestInfoWnd::ShowQuest(INT Index)
{
    Current = Index;
    ScrollBar->SetScrollPos(0);
    ScrollY = 0;
    if (Current >= 0 && Current < Quests.Num())
    {
        Quests(Current).ShownStep = Quests(Current).Step;
        const TCHAR* S = GConsole->GetQuestText(Quests(Current).QuestID, Quests(Current).Step);
        if (S)
        {
            appStrcpy(Text, S);
            return;
        }
        Text[0] = 0;
    }
}
NW_HOOK_METHOD(0x100a0960, NCQuestInfoWnd, ShowQuest)

// 0x100a0a00 (vslot 65) - up to the step the player is at; the text stays when the quest
// data has none for the new step
INT NCQuestInfoWnd::NextStep()
{
    if (Current >= 0 && Current < Quests.Num() && Quests(Current).ShownStep < Quests(Current).Step)
    {
        ScrollBar->SetScrollPos(0);
        ScrollY = 0;
        Quests(Current).ShownStep++;
        const TCHAR* S = GConsole->GetQuestText(Quests(Current).QuestID, Quests(Current).ShownStep);
        if (S)
            appStrcpy(Text, S);
    }
    return 1;
}
NW_HOOK_METHOD(0x100a0a00, NCQuestInfoWnd, NextStep)

// 0x100a0aa0 (vslot 66) - down to step 1
INT NCQuestInfoWnd::PrevStep()
{
    if (Current >= 0 && Current < Quests.Num() && Quests(Current).ShownStep > 1)
    {
        ScrollBar->SetScrollPos(0);
        ScrollY = 0;
        Quests(Current).ShownStep--;
        const TCHAR* S = GConsole->GetQuestText(Quests(Current).QuestID, Quests(Current).ShownStep);
        if (S)
            appStrcpy(Text, S);
    }
    return 1;
}
NW_HOOK_METHOD(0x100a0aa0, NCQuestInfoWnd, PrevStep)

// 0x100a13d0 (vslot 36) - the arrows go with the child windows
INT NCQuestInfoWnd::OnDestroy()
{
    Quests.Empty();
    NCScrollWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100a13d0, NCQuestInfoWnd, OnDestroy)

// 0x100a1400 - shown at the step the player is at
void NCQuestInfoWnd::AddQuest(INT QuestID, INT Step)
{
    INT i = Quests.Add();
    Quests(i).QuestID = QuestID;
    Quests(i).Step = Step;
    Quests(i).ShownStep = Step;
    ScrollBar->SetScrollPos(0);
}
NW_HOOK_METHOD(0x100a1400, NCQuestInfoWnd, AddQuest)

// 0x100a1450
void NCQuestInfoWnd::ClearQuests()
{
    Quests.Empty();
    Current = -1;
    Text[0] = 0;
}
NW_HOOK_METHOD(0x100a1450, NCQuestInfoWnd, ClearQuests)

NW_CHECK_VTABLE(NCQuestInfoWnd, 0x100ed618, 67)
