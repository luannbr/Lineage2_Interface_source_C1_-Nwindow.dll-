/*=============================================================================
    NCActionLinker.cpp - an HTML link that runs an action. Stock: 0x1002fe50 .. 0x1002fed8,
    0x100306e0 .. 0x10030835 (OnConfirm's body is also NCFileLinker's OnLButtonDown).
=============================================================================*/
#include "nwindow/NCActionLinker.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NConsoleWnd.h"

// 0x100306e0 - NControl's constructor is inlined; Message is left as allocated
NCActionLinker::NCActionLinker(INT InAction)
{
    Action = InAction;
}
NW_HOOK_CTOR_ARGS(0x100306e0, NCActionLinker, (INT A), (A))

// 0x10030750
NW_HOOK_DTOR(0x10030750, NCActionLinker)

// 0x1002fe70 (vslot 24)
INT NCActionLinker::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == (UINT)NWM_ACTIONLINK_CONFIRM)
        return OnConfirm(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1002fe70, NCActionLinker, WindowProc)

// 0x100307a0 (vslot 56)
INT NCActionLinker::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (Message[0])
    {
        NDialogInfo Info;
        Info.ID = 0x38;
        Info.bMainBox = 1;
        Info.bInput = 0;
        Info.Owner = Entry;
        Info.SetText(Message);
        Info.bLargeAmount = 1;   // set by the stock code, meaningless for a yes / no box
        GConsole->ShowDialog(&Info);
        return 1;
    }
    NotifyParent(Action);
    return 1;
}
NW_HOOK_METHOD(0x100307a0, NCActionLinker, OnLButtonDown)

// 0x1002fe50 (vslot 63) - also NCFileLinker's slot 56 (identical body)
INT NCActionLinker::OnConfirm(WPARAM wParam, LPARAM lParam)
{
    NotifyParent(Action);
    return 1;
}
NW_HOOK_METHOD(0x1002fe50, NCActionLinker, OnConfirm)

NW_CHECK_VTABLE(NCActionLinker, 0x100deee8, 64)
