/*=============================================================================
    NCSystemMenuWnd.cpp - the system menu. Stock: 0x10076bf0 .. 0x10076cf5, 0x10076d20 ..
    0x10076da8, 0x10077420 .. 0x100778ef, 0x100779f0 .. 0x10077aac, plus ShowWindow
    0x10078950 (shared by six windows) and OnOptions 0x10076d00 (NCLoginWnd's slot 63 too);
    OnDestroy is the restart menu's.
=============================================================================*/
#include "nwindow/NCSystemMenuWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x10077420 - Buttons[SYSMENU_Unused] is left as allocated
NCSystemMenuWnd::NCSystemMenuWnd()
{
    BackTex = NULL;
    Buttons[SYSMENU_Help] = NULL;
    Buttons[SYSMENU_Options] = NULL;
    Buttons[SYSMENU_Restart] = NULL;
    Buttons[SYSMENU_Exit] = NULL;
}
NW_HOOK_CTOR(0x10077420, NCSystemMenuWnd)

// 0x10077490
NW_HOOK_DTOR(0x10077490, NCSystemMenuWnd)

// 0x10076bf0 (vslot 24) - the dialog box answers with NWM_DIALOG_RESULT (wParam = dialog id)
INT NCSystemMenuWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_SYSMENU_HELP)
            return OnHelp() != 0;
        if (wParam == NID_SYSMENU_OPTIONS)
            return OnOptions() != 0;
        if (wParam == NID_SYSMENU_RESTART)
            return OnRestart() != 0;
        if (wParam == NID_SYSMENU_EXIT)
            return OnExit() != 0;
    }
    else if (Msg == (UINT)NWM_DIALOG_RESULT)
        return OnDialogResult(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10076bf0, NCSystemMenuWnd, WindowProc)

// one button of the menu (repeated inline in the stock OnCreate)
static FORCEINLINE NCButton* CreateMenuButton(NCWnd* Parent, INT ID, INT X, INT Y, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                    TEXT("L2UI.Control.DefaultButton_click"), 0);
    Button->ID = ID;
    Button->SetWindowRect(X, Y, 67, 19);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->SetTextID(TextID);
    return Button;
}

// 0x100774e0 (vslot 35) - its own decorated title bar
INT NCSystemMenuWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.etcWnd.systemBack"));
    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_SYSMENU_FRAME;
    FrameCtrl->ID = NID_SYSMENU_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    Buttons[SYSMENU_Help] = CreateMenuButton(this, NID_SYSMENU_HELP, 43, 43, 0x91);
    Buttons[SYSMENU_Options] = CreateMenuButton(this, NID_SYSMENU_OPTIONS, 140, 43, 0x92);
    Buttons[SYSMENU_Restart] = CreateMenuButton(this, NID_SYSMENU_RESTART, 43, 80, 0x93);
    Buttons[SYSMENU_Exit] = CreateMenuButton(this, NID_SYSMENU_EXIT, 140, 80, 0x94);
    return 1;
}
NW_HOOK_METHOD(0x100774e0, NCSystemMenuWnd, OnCreate)

// stock body shared with NCRestartMenuWnd's (0x10077900)
INT NCSystemMenuWnd::OnDestroy()
{
    for (INT i = 0; i < SYSMENU_MAX; i++)
    {
        if (Buttons[i])
        {
            RemoveChildWindow(Buttons[i]->Entry);
            delete Buttons[i];
            Buttons[i] = NULL;
        }
    }
    NCWnd::OnDestroy();
    return 1;
}

// 0x10078950 (vslot 42) - body shared with NCBoardWnd, NCMatchWnd and three other windows:
// restored from its icon, the window comes back where the icon was
void NCSystemMenuWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Icon)
    {
        SetWindowRect(Icon->ScreenX, Icon->ScreenY, Width, Height);
        Icon->SetWindowRect(0, 0, 0, 0);
        Icon->HideWindow();
    }
}
NW_HOOK_METHOD(0x10078950, NCSystemMenuWnd, ShowWindow)

// 0x10077870 (vslot 50) - under the title bar; the clip is set before the origin
INT NCSystemMenuWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 101, 0, 0, 240, 101, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10077870, NCSystemMenuWnd, OnPaint)

// 0x10076ce0 (vslot 62)
INT NCSystemMenuWnd::OnHelp()
{
    GDesktop->ToggleHelp();
    return 1;
}
NW_HOOK_METHOD(0x10076ce0, NCSystemMenuWnd, OnHelp)

// 0x10076d00 (vslot 63) - body shared with NCLoginWnd's slot 63
INT NCSystemMenuWnd::OnOptions()
{
    GConsole->ToggleOptionWnd();
    return 1;
}
NW_HOOK_METHOD(0x10076d00, NCSystemMenuWnd, OnOptions)

// the confirmation of Restart / Exit (repeated in both slots)
static FORCEINLINE void AskSystemMenu(NCWnd* Menu, INT DialogID, INT MsgID)
{
    NDialogInfo Info;
    Info.ID = DialogID;
    Info.bMainBox = 1;
    Info.bInput = 0;
    Info.Owner = Menu->Entry;
    Info.SetSystemMessage(MsgID);
    GConsole->ShowDialog(&Info);
}

// 0x100779f0 (vslot 64) - systemmsg 126 "Restart the game. Do you want to continue?"
INT NCSystemMenuWnd::OnRestart()
{
    AskSystemMenu(this, DIALOG_SYSMENU_RESTART, 126);
    return 1;
}
NW_HOOK_METHOD(0x100779f0, NCSystemMenuWnd, OnRestart)

// 0x10077a50 (vslot 65) - systemmsg 125 "Quit game. Do you want to continue?"
INT NCSystemMenuWnd::OnExit()
{
    AskSystemMenu(this, DIALOG_SYSMENU_EXIT, 125);
    return 1;
}
NW_HOOK_METHOD(0x10077a50, NCSystemMenuWnd, OnExit)

// 0x10076d20 (vslot 66) - OK in one of the two dialogs
INT NCSystemMenuWnd::OnDialogResult(WPARAM DialogID, LPARAM Unused)
{
    switch (DialogID)
    {
    case DIALOG_SYSMENU_RESTART:
    {
        L2ParamStack Params(10);
        GConsole->RequestRestart(Params);
        HideWindow();
        break;
    }
    case DIALOG_SYSMENU_EXIT:
        GConsole->RequestLogout();
        break;
    }
    return 1;
}
NW_HOOK_METHOD(0x10076d20, NCSystemMenuWnd, OnDialogResult)

NW_CHECK_VTABLE(NCSystemMenuWnd, 0x100e7c10, 67)
