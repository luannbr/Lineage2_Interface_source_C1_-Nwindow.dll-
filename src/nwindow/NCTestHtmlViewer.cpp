/*=============================================================================
    NCTestHtmlViewer.cpp - the HTML test page. Stock: 0x10026640 .. 0x100266aa,
    0x100277e0 .. 0x1002787f, 0x100298d0 .. 0x10029a3c, 0x1002c660 .. 0x1002c6b4; OnDestroy,
    OnPaint and OnClose are NCNPCHtmlViewer's.
=============================================================================*/
#include "nwindow/NCTestHtmlViewer.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCEditBox.h"

enum { NID_TESTHTML_LOAD = (INT)0xF0F01413 };  // the button's NWM_COMMAND id (name ours)

// 0x100277e0
NCTestHtmlViewer::NCTestHtmlViewer()
{
}
NW_HOOK_CTOR(0x100277e0, NCTestHtmlViewer)

// 0x10027830
NW_HOOK_DTOR(0x10027830, NCTestHtmlViewer)

// 0x10026640 (vslot 24)
INT NCTestHtmlViewer::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCNPCHtmlViewer::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == (WPARAM)NID_TESTHTML_LOAD)
        return OnLoad() != 0;
    return 0;
}
NW_HOOK_METHOD(0x10026640, NCTestHtmlViewer, WindowProc)

// 0x100298d0 (vslot 35) - the edit line and the button, 16 px high, 20 px above the bottom;
// CreateChild for the edit line with the window hidden before its entry is set
INT NCTestHtmlViewer::OnCreate()
{
    NCNPCHtmlViewer::OnCreate();

    FileEdit = new NCEditBox(0, TEXT("NWindow.BlackTexture"), 1, 0);
    FileEdit->SetWindowRect(0, Height - 20, 192, 16);
    NWndEntry* E = AddChildWindow(FileEdit, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    FileEdit->HideWindow();
    FileEdit->Entry = E;
    if (E->Parent)
    {
        NCWnd* Parent = E->Parent->Wnd;
        FileEdit->ScreenX += Parent->ScreenX;
        FileEdit->ScreenY += Parent->ScreenY;
    }
    FileEdit->WindowProc(WM_CREATE, 0, 0);

    LoadButton = new NCButton(TEXT("NWindow.WhiteTexture"), TEXT("NWindow.WhiteTexture"), 0);
    LoadButton->ID = NID_TESTHTML_LOAD;
    LoadButton->SetWindowRect(192, Height - 20, 16, 16);
    CreateChild(LoadButton, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100298d0, NCTestHtmlViewer, OnCreate)

// 0x1002c660 (vslot 63) - the button: ..\test_html\<the name typed>
INT NCTestHtmlViewer::OnLoad()
{
    TCHAR Path[256];
    const TCHAR* Name = FileEdit->CommitText();
    if (Name && *Name)
    {
        Path[0] = 0;
        appSprintf(Path, TEXT("..\\test_html\\%s"), Name);
        LoadFile(Path);
    }
    return 1;
}
NW_HOOK_METHOD(0x1002c660, NCTestHtmlViewer, OnLoad)

NW_CHECK_VTABLE(NCTestHtmlViewer, 0x100de4a8, 64)
