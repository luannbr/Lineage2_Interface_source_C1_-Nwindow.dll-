/*=============================================================================
    NCBoardWnd.cpp - the community board and its page viewer. Stock: 0x10026420 .. 0x10026488,
    0x10026490 .. 0x100264b0, 0x10026510 .. 0x10026630, 0x100266b0 .. 0x1002699d,
    0x10027460 .. 0x100276d3, 0x10027880 .. 0x10027ac6, 0x10027ad0 .. 0x10027b71,
    0x100296b0 .. 0x1002976f, 0x10029a40 .. 0x10029fbc, 0x1002a890 .. 0x1002a89a (the viewer's
    slot 72 and the board's ShowWindow are bodies the linker folded with other windows').
=============================================================================*/
#include "nwindow/NCBoardWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NHtmlLayout.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <string.h>

enum
{
    NID_BOARD_FRAME = (INT)0xF0F01420,
    NID_BOARD_BUTTON1 = (INT)0xF0F01422,  // .. 0xF0F01427
    NID_BBS_HIDE = (INT)0xF0F01411,
};

enum { STR_BoardButton1 = 377 };  // .. 382
enum { SOUND_Click = 0 };

enum
{
    MSG_IncorrectName = 204,           // "Incorrect name. Please try again. "
    MSG_TitleTooLong = 328,            // "Please input title using less than 128 characters."
    MSG_TooLong3000 = 329,             // "Please input contents using less than 3000 characters."
};

/*-----------------------------------------------------------------------------
    NCBBSHtmlViewer.
-----------------------------------------------------------------------------*/

// 0x100296b0
NCBBSHtmlViewer::NCBBSHtmlViewer()
{
    Unknown110 = 0;
    NumPostPieces = 0;
    NumFormPieces = 0;
    NumPageParts = 0;
    bLocked = 0;
}
NW_HOOK_CTOR(0x100296b0, NCBBSHtmlViewer)

// 0x10029720
NW_HOOK_DTOR(0x10029720, NCBBSHtmlViewer)

// 0x10026420 (vslot 24)
INT NCBBSHtmlViewer::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCHtmlViewer::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == (WPARAM)NID_BBS_HIDE)
        return OnHideCommand() != 0;
    return 0;
}
NW_HOOK_METHOD(0x10026420, NCBBSHtmlViewer, WindowProc)

// 0x10026490 (vslot 35) - NCScrollWnd's (the viewer's own OnCreate is skipped)
INT NCBBSHtmlViewer::OnCreate()
{
    NCScrollWnd::OnCreate();
    BoardBackTex = FindTexture(TEXT("Sek.cbui353"));
    return 1;
}
NW_HOOK_METHOD(0x10026490, NCBBSHtmlViewer, OnCreate)

// 0x1002a890 (vslot 36) - also NCTutorialHtmlViewer's
INT NCBBSHtmlViewer::OnDestroy()
{
    NCHtmlViewer::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1002a890, NCBBSHtmlViewer, OnDestroy)

// 0x10027560 (vslot 42)
void NCBBSHtmlViewer::ShowWindow()
{
    NCHtmlViewer::ShowWindow();
}
NW_HOOK_METHOD(0x10027560, NCBBSHtmlViewer, ShowWindow)

// 0x10027460 (vslot 50) - NCHtmlViewer's, on the board's frame and clipped inside it
INT NCBBSHtmlViewer::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex ? BackTex : BoardBackTex);
    Device->SetClip(ScreenX + 3, ScreenY + 10, Width - 6, Height - 20);
    Device->PushOrigin(ScrollX, ScrollY);
    if (Doc && bLoaded)
    {
        Doc->Draw(Device);
        if (bLoaded)
            SetContentHeight(Doc->GetContentHeight());
    }
    Device->PopOrigin();
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10027460, NCBBSHtmlViewer, OnPaint)

// 0x10027570 (vslot 65) - a missing value says the title is too long (sic); the one-line boxes
// may not hold the characters the console rejects; any value is 2..3000 bytes. The bad words of
// the value are replaced in place.
TCHAR* NCBBSHtmlViewer::GetControlValue(const TCHAR* Name)
{
    ANSICHAR Ansi[512];
    if (!Name)
        return NULL;
    INT N = Controls.Num();
    for (INT i = 0; i < N; i++)
    {
        NControl* Control = Controls(i);
        const TCHAR* Caption = Control->GetCaption();
        if (!Caption || wcscmp(Caption, Name))
            continue;

        if (!Control->GetValue())
        {
            GConsole->ShowSystemMessage(MSG_TitleTooLong, 0xFFB09B79);
            return NULL;
        }
        if (!Controls(i)->IsA(NClass::NCHtmlMultiEdit()) &&
            GConsole->HasInvalidChars((TCHAR*)Control->GetValue()))
        {
            GConsole->ShowSystemMessage(MSG_IncorrectName, 0xFFB09B79);
            return NULL;
        }
        WideCharToMultiByte(CP_ACP, 0, Control->GetValue(), -1, Ansi, 512, NULL, NULL);
        INT Length = (INT)strlen(Ansi);
        if (Length <= 1 || Length > 3000)
        {
            GConsole->ShowSystemMessage(MSG_TooLong3000, 0xFFB09B79);
            return NULL;
        }
        GConsole->HasBadWord((TCHAR*)Control->GetValue());
        return (TCHAR*)Control->GetValue();
    }
    return NULL;
}
NW_HOOK_METHOD(0x10027570, NCBBSHtmlViewer, GetControlValue)

// 0x10026510 (vslot 67) - locked once the console accepts a command; the sound plays even for
// an empty link
INT NCBBSHtmlViewer::OnActionLink(TCHAR* Action)
{
    if (bLocked)
        return 1;
    wcscpy(GHtmlLinkTarget, Action);
    if (GHtmlLinkTarget[0])
    {
        INT Result = GConsole->RunHtmlCommand(GHtmlLinkTarget, this);
        if (Result == 2)
        {
            if (Owner)
                Owner->Wnd->WindowProc(NWM_HTML_CLOSE, 0, 0);
            else
                HideWindow();
        }
        else if (Result == 1)
            bLocked = 1;
    }
    GConsole->Engine->OnInterfacePlaySound(SOUND_Click);
    return 1;
}
NW_HOOK_METHOD(0x10026510, NCBBSHtmlViewer, OnActionLink)

// 0x100265b0 (vslot 68)
INT NCBBSHtmlViewer::OnButton(TCHAR* Action)
{
    if (bLocked)
        return 1;
    wcscpy(GHtmlLinkTarget, Action);
    INT Result = GConsole->RunHtmlCommand(GHtmlLinkTarget, this);
    if (Result == 2)
    {
        if (Owner)
        {
            Owner->Wnd->WindowProc(NWM_HTML_CLOSE, 0, 0);
            return 1;
        }
        HideWindow();
        return 1;
    }
    if (Result == 1)
        bLocked = 1;
    return 1;
}
NW_HOOK_METHOD(0x100265b0, NCBBSHtmlViewer, OnButton)

// stock body shared with NCOptionWnd::OnCancel (0x10059d00)
INT NCBBSHtmlViewer::OnHideCommand()
{
    HideWindow();
    return 1;
}

NW_CHECK_VTABLE(NCBBSHtmlViewer, 0x100deab0, 73)

/*-----------------------------------------------------------------------------
    NCBoardWnd.
-----------------------------------------------------------------------------*/

// 0x10027880
NCBoardWnd::NCBoardWnd()
{
    bLocked = 0;
}
NW_HOOK_CTOR(0x10027880, NCBoardWnd)

// 0x100278d0
NW_HOOK_DTOR(0x100278d0, NCBoardWnd)

// 0x100266b0 (vslot 24) - the six buttons
INT NCBoardWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == (WPARAM)NID_BOARD_BUTTON1)
            return OnButton1() != 0;
        if (wParam == (WPARAM)(NID_BOARD_BUTTON1 + 1))
            return OnButton2() != 0;
        if (wParam == (WPARAM)(NID_BOARD_BUTTON1 + 2))
            return OnButton3() != 0;
        if (wParam == (WPARAM)(NID_BOARD_BUTTON1 + 3))
            return OnButton4() != 0;
        if (wParam == (WPARAM)(NID_BOARD_BUTTON1 + 4))
            return OnButton5() != 0;
        if (wParam == (WPARAM)(NID_BOARD_BUTTON1 + 5))
            return OnButton6() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100266b0, NCBoardWnd, WindowProc)

// the buttons' command, sent while the board is not locked; an accepted one locks it
static FORCEINLINE INT RunBoardCommand(NCBoardWnd* Board, TCHAR* Command)
{
    if (Command[0] && !Board->bLocked)
    {
        if (GConsole->RunHtmlCommand(Command, Board->HtmlViewer) == 1)
            Board->bLocked = 1;
    }
    return 1;
}

// 0x100267c0 (vslot 62)
INT NCBoardWnd::OnButton1()
{
    return RunBoardCommand(this, Commands[0]);
}
NW_HOOK_METHOD(0x100267c0, NCBoardWnd, OnButton1)

// 0x10026810 (vslot 63)
INT NCBoardWnd::OnButton2()
{
    return RunBoardCommand(this, Commands[1]);
}
NW_HOOK_METHOD(0x10026810, NCBoardWnd, OnButton2)

// 0x10026860 (vslot 64)
INT NCBoardWnd::OnButton3()
{
    return RunBoardCommand(this, Commands[2]);
}
NW_HOOK_METHOD(0x10026860, NCBoardWnd, OnButton3)

// 0x100268b0 (vslot 65) - not stopped by the lock; an accepted command disables this button
INT NCBoardWnd::OnButton4()
{
    if (Commands[3][0])
    {
        if (GConsole->RunHtmlCommand(Commands[3], HtmlViewer) == 1)
            Buttons[3]->EnableWindow(0);
    }
    return 1;
}
NW_HOOK_METHOD(0x100268b0, NCBoardWnd, OnButton4)

// 0x10026900 (vslot 66)
INT NCBoardWnd::OnButton5()
{
    return RunBoardCommand(this, Commands[4]);
}
NW_HOOK_METHOD(0x10026900, NCBoardWnd, OnButton5)

// 0x10026950 (vslot 67)
INT NCBoardWnd::OnButton6()
{
    return RunBoardCommand(this, Commands[5]);
}
NW_HOOK_METHOD(0x10026950, NCBoardWnd, OnButton6)

// 0x10029a40 (vslot 35) - title bar, page, then the buttons above it
INT NCBoardWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_BOARD_FRAME;
    FrameCtrl->ID = NID_BOARD_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 646, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    BackTex = FindTexture(TEXT("sek.cbui354"));

    HtmlViewer = new NCBBSHtmlViewer;
    HtmlViewer->SetWindowRect(0, 39, 646, 486);
    CreateChild(HtmlViewer, NWS_VISIBLE);
    HtmlViewer->SetOwner(Entry);

    for (INT i = 0; i < 6; i++)
    {
        Buttons[i] = new NCButton(TEXT("sek.cbui91"), TEXT("sek.cbui93"), 0);
        Buttons[i]->ID = NID_BOARD_BUTTON1 + i;
        Buttons[i]->SetWindowRect(10 + i * 60, 24, 50, 16);
        CreateChild(Buttons[i], NWS_VISIBLE);
    }
    for (INT i = 0; i < 6; i++)
        Buttons[i]->SetTextID(STR_BoardButton1 + i);
    return 1;
}
NW_HOOK_METHOD(0x10029a40, NCBoardWnd, OnCreate)

// 0x10027920 (vslot 36) - the buttons are enabled again before they go
INT NCBoardWnd::OnDestroy()
{
    if (HtmlViewer)
    {
        RemoveChildWindow(HtmlViewer->Entry);
        if (HtmlViewer)
            delete HtmlViewer;
        HtmlViewer = NULL;
    }
    for (INT i = 0; i < 6; i++)
        Buttons[i]->EnableWindow(1);
    for (INT i = 0; i < 6; i++)
    {
        if (Buttons[i])
        {
            RemoveChildWindow(Buttons[i]->Entry);
            if (Buttons[i])
                delete Buttons[i];
            Buttons[i] = NULL;
        }
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10027920, NCBoardWnd, OnDestroy)

// stock body shared with NCSystemMenuWnd's (0x10078950): shown again from its minimized icon
void NCBoardWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Icon)
    {
        SetWindowRect(Icon->ScreenX, Icon->ScreenY, Width, Height);
        Icon->SetWindowRect(0, 0, 0, 0);
        Icon->HideWindow();
    }
}

// 0x10027ad0 (vslot 50) - the strip under the title bar; the guard names the NPC dialog, as
// in the stock code
INT NCBoardWnd::OnPaint(NCGDevice* Device)
{
    guard(NCNPCHtmlViewer::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 21, 646, 18, 0, 0, 646, 18, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10027ad0, NCBoardWnd, OnPaint)

NW_CHECK_VTABLE(NCBoardWnd, 0x100de5a8, 68)
