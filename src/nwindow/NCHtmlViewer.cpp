/*=============================================================================
    NCHtmlViewer.cpp - the HTML viewer and NCTutorialHtmlViewer. Stock: 0x10025f50 ..
    0x1002be63, interleaved with the page tree (NHtmlLayout.cpp) and the board viewers.
=============================================================================*/
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NHtmlLayout.h"
#include "nwindow/NCActionLinker.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCFileLinker.h"
#include "nwindow/NCHtmlButton.h"
#include "nwindow/NCHtmlComboBox.h"
#include "nwindow/NCHtmlEdit.h"
#include "nwindow/NCMultiEdit.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCrt.h"

#include <string.h>
#include <wchar.h>

// NCHtmlViewer::FileLinkID .. ComboBoxID
enum { NID_HTML_FIRST = (INT)0xFF00FF00 };

// the code a click in an edit box sends: MAKELONG(100, its y) (NCHtmlEdit::OnLButtonDown)
enum { HTML_SCROLLTO = 100 };

// style bit 0x200 of the page controls (meaning unknown)
enum { NWS_HTMLCONTROL = 0x200 };

// dialog of NCActionLinker's question (NCDialogBox::DialogID)
enum { DIALOG_ActionLink = 0x38 };

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_Click = 0 };

// systemmsg lines (names ours)
enum
{
    MSG_IncorrectName = 204,           // "Incorrect name. Please try again. "
    MSG_TooLong3000 = 329,             // "Please input contents using less than 3000 characters."
};

// <html imgsrc> absent: no background. The stock code tests this pointer, not the text.
static const TCHAR HtmlTransparent[] = TEXT("Transparent");

// a new page control is shown only when its top is on the page's visible part (repeated in
// every Add* of the stock code)
static FORCEINLINE void ShowIfVisible(NCHtmlViewer* Viewer, NControl* Control)
{
    if (Control->LocalY <= Viewer->Height && Control->LocalY >= 0)
        Control->ShowWindow();
    else
        Control->HideWindow();
}

/*-----------------------------------------------------------------------------
    NCHtmlViewer.
-----------------------------------------------------------------------------*/

// 0x100292e0 - the alignment stacks of the parser get their bottom 0
NCHtmlViewer::NCHtmlViewer()
{
    BackTex = NULL;
    Unknown100 = 0;
    UnknownFC = 0;
    CreateWidth = 0;
    CreateHeight = 0;
    if (GHtmlAlignStack.Num() <= 0)
        GHtmlAlignStack.AddItem(0);
    if (GHtmlVAlignStack.Num() <= 0)
        GHtmlVAlignStack.AddItem(0);
    bLoaded = 0;
    Unknown100 = 0;
    UnknownFC = 0;
    Owner = NULL;
    Doc = NULL;
    FileLinkID = NID_HTML_FIRST;
    ActionLinkID = NID_HTML_FIRST + 1;
    ButtonID = NID_HTML_FIRST + 2;
    EditID = NID_HTML_FIRST + 3;
    MultiEditID = NID_HTML_FIRST + 4;
    ComboBoxID = NID_HTML_FIRST + 5;
    Unknown0C8 = 16;
}
NW_HOOK_CTOR(0x100292e0, NCHtmlViewer)

// 0x10028a00
NW_HOOK_DTOR(0x10028a00, NCHtmlViewer)

// 0x10025f50 (vslot 24) - each kind of page control has its id
INT NCHtmlViewer::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCScrollWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (FileLinkID == (INT)wParam)
            return OnFileLink((TCHAR*)lParam) != 0;
        if (ActionLinkID == (INT)wParam)
            return OnActionLink((TCHAR*)lParam) != 0;
        if (ButtonID == (INT)wParam)
            return OnButton((TCHAR*)lParam) != 0;
        if (EditID == (INT)wParam)
            return OnEdit(lParam) != 0;
        if (MultiEditID == (INT)wParam)
            return OnMultiEdit(lParam) != 0;
        if (ComboBoxID == (INT)wParam)
            return OnComboBox(lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10025f50, NCHtmlViewer, WindowProc)

// 0x10026070 (vslot 35)
INT NCHtmlViewer::OnCreate()
{
    NCScrollWnd::OnCreate();
    CreateHeight = Height;
    CreateWidth = Width;
    return 1;
}
NW_HOOK_METHOD(0x10026070, NCHtmlViewer, OnCreate)

// 0x1002a7e0 (vslot 36) - the page and its controls go
INT NCHtmlViewer::OnDestroy()
{
    if (bLoaded && Doc)
    {
        Doc->Release();
        if (Doc)
            delete Doc;
        Doc = NULL;
    }
    INT N = Controls.Num();
    for (INT i = 0; i < N; i++)
    {
        NControl* Control = Controls(i);
        if (Control->Entry && Control)
        {
            RemoveChildWindow(Control->Entry);
            delete Control;
        }
    }
    Controls.Empty();
    NCScrollWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1002a7e0, NCHtmlViewer, OnDestroy)

// 0x100273d0 (vslot 42) - the controls below the visible part stay hidden
void NCHtmlViewer::ShowWindow()
{
    NCWnd::ShowWindow();
    INT N = Controls.Num();
    for (INT i = 0; i < N; i++)
    {
        NControl* Control = Controls(i);
        if (Height + ScrollY < Control->LocalY)
            Control->HideWindow();
        else
            Control->ShowWindow();
    }
}
NW_HOOK_METHOD(0x100273d0, NCHtmlViewer, ShowWindow)

// 0x10028040 (vslot 50) - background (the scroll bar's 16 px of the texture left out), the
// page shifted by the scroll position, whose height sets the scroll range. The clip goes on
// before BeginPaint.
INT NCHtmlViewer::OnPaint(NCGDevice* Device)
{
    guard(NCHtmlViewer::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width - 16, Height, BackTex);
    Device->PushOrigin(ScrollX, ScrollY);
    if (bLoaded && Doc)
        Doc->Draw(Device);
    if (bLoaded && Doc)
        SetContentHeight(Doc->GetContentHeight());
    Device->PopOrigin();
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10028040, NCHtmlViewer, OnPaint)

// 0x100271f0 (vslot 61) - the page's controls follow every scroll
INT NCHtmlViewer::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    NCScrollWnd::OnMouseWheel(wParam, lParam);
    ScrollControls(0, DeltaY);
    return 1;
}
NW_HOOK_METHOD(0x100271f0, NCHtmlViewer, OnMouseWheel)

// 0x10027180 (vslot 62)
INT NCHtmlViewer::ScrollUp()
{
    NCScrollWnd::ScrollUp();
    ScrollControls(0, DeltaY);
    return 1;
}
NW_HOOK_METHOD(0x10027180, NCHtmlViewer, ScrollUp)

// 0x100271a0 (vslot 63)
INT NCHtmlViewer::ScrollDown()
{
    NCScrollWnd::ScrollDown();
    ScrollControls(0, DeltaY);
    return 1;
}
NW_HOOK_METHOD(0x100271a0, NCHtmlViewer, ScrollDown)

// 0x100271c0 (vslot 64)
INT NCHtmlViewer::OnScrollBar(INT Value)
{
    NCScrollWnd::OnScrollBar(Value);
    ScrollControls(0, DeltaY);
    return 1;
}
NW_HOOK_METHOD(0x100271c0, NCHtmlViewer, OnScrollBar)

// 0x10027220 (vslot 65) - the value of the control whose caption (variable) is Name, for a
// $Name in a command. A one-line box must hold a name: 1 to 16 bytes in the ANSI code page,
// characters the font has and no '&'; a multi-line box 2 to 3000 bytes. Anything else
// shows a system message and gives NULL.
TCHAR* NCHtmlViewer::GetControlValue(const TCHAR* Name)
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
            return NULL;
        if (!Controls(i)->IsA(NClass::NCHtmlMultiEdit()))
        {
            if (GConsole->HasInvalidChars((TCHAR*)Control->GetValue()) ||
                !GConsole->HasNoAmpersand((TCHAR*)Control->GetValue()))
            {
                GConsole->ShowSystemMessage(MSG_IncorrectName, 0xFFB09B79);
                return NULL;
            }
        }
        WideCharToMultiByte(CP_ACP, 0, Control->GetValue(), -1, Ansi, 512, NULL, NULL);
        INT Length = (INT)strlen(Ansi);
        if (Controls(i)->IsA(NClass::NCHtmlMultiEdit()))
        {
            if (Length <= 1 || Length > 3000)
            {
                GConsole->ShowSystemMessage(MSG_TooLong3000, 0xFFB09B79);
                return NULL;
            }
        }
        if (!Controls(i)->IsA(NClass::NCHtmlMultiEdit()) && (Length <= 0 || Length > 16))
        {
            GConsole->ShowSystemMessage(MSG_IncorrectName, 0xFFB09B79);
            return NULL;
        }
        return (TCHAR*)Control->GetValue();
    }
    return NULL;
}
NW_HOOK_METHOD(0x10027220, NCHtmlViewer, GetControlValue)

// 0x1002be30 (vslot 66) - <a link>: another page from a file
INT NCHtmlViewer::OnFileLink(TCHAR* File)
{
    wcscpy(GHtmlLinkTarget, File);
    if (GHtmlLinkTarget[0])
        LoadHtmlFile(GHtmlLinkTarget);
    return 1;
}
NW_HOOK_METHOD(0x1002be30, NCHtmlViewer, OnFileLink)

// 0x10026110 (vslot 67) - <a action>: the command, a click sound
INT NCHtmlViewer::OnActionLink(TCHAR* Action)
{
    wcscpy(GHtmlLinkTarget, Action);
    if (GHtmlLinkTarget[0])
    {
        if (GConsole->RunHtmlCommand(GHtmlLinkTarget, this) == 2)
        {
            if (Owner)
                Owner->Wnd->WindowProc(NWM_HTML_CLOSE, 0, 0);
            else
                HideWindow();
        }
        GConsole->Engine->OnInterfacePlaySound(SOUND_Click);
    }
    return 1;
}
NW_HOOK_METHOD(0x10026110, NCHtmlViewer, OnActionLink)

// 0x10026190 (vslot 68) - <button action> (no emptiness test, no sound)
INT NCHtmlViewer::OnButton(TCHAR* Action)
{
    wcscpy(GHtmlLinkTarget, Action);
    if (GConsole->RunHtmlCommand(GHtmlLinkTarget, this) == 2)
    {
        if (Owner)
        {
            Owner->Wnd->WindowProc(NWM_HTML_CLOSE, 0, 0);
            return 1;
        }
        HideWindow();
    }
    return 1;
}
NW_HOOK_METHOD(0x10026190, NCHtmlViewer, OnButton)

// 0x10026200 (vslot 69) - from an edit box: a click scrolls it into view when it is above
// the visible part; Enter runs its action
INT NCHtmlViewer::OnEdit(LPARAM Param)
{
    if ((SWORD)LOWORD(Param) == HTML_SCROLLTO)
    {
        INT EditY = (SWORD)HIWORD(Param);
        if (EditY < 0)
        {
            OnScrollBar(EditY - ScrollY);
            ScrollBar->SetScrollPos(-ScrollY);
        }
        return 1;
    }
    TCHAR* Action = (TCHAR*)Param;
    if (Action && *Action)
    {
        wcscpy(GHtmlLinkTarget, Action);
        if (GConsole->RunHtmlCommand(GHtmlLinkTarget, this) == 2)
        {
            if (Owner)
            {
                Owner->Wnd->WindowProc(NWM_HTML_CLOSE, 0, 0);
                return 1;
            }
            HideWindow();
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10026200, NCHtmlViewer, OnEdit)

// 0x100262c0 (vslot 70) - the same for a multi-line box (an empty action runs too)
INT NCHtmlViewer::OnMultiEdit(LPARAM Param)
{
    if ((SWORD)LOWORD(Param) == HTML_SCROLLTO)
    {
        INT EditY = (SWORD)HIWORD(Param);
        if (EditY < 0)
        {
            OnScrollBar(EditY - ScrollY);
            ScrollBar->SetScrollPos(-ScrollY);
        }
        return 1;
    }
    TCHAR* Action = (TCHAR*)Param;
    if (Action)
    {
        wcscpy(GHtmlLinkTarget, Action);
        if (GConsole->RunHtmlCommand(GHtmlLinkTarget, this) == 2)
        {
            if (Owner)
            {
                Owner->Wnd->WindowProc(NWM_HTML_CLOSE, 0, 0);
                return 1;
            }
            HideWindow();
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100262c0, NCHtmlViewer, OnMultiEdit)

// 0x10026380 (vslot 71) - from a combo box: only the scroll into view
INT NCHtmlViewer::OnComboBox(LPARAM Param)
{
    INT BoxY = (SWORD)HIWORD(Param);
    if (LOWORD(Param) == HTML_SCROLLTO && BoxY < 0)
    {
        OnScrollBar(BoxY - ScrollY);
        ScrollBar->SetScrollPos(-ScrollY);
    }
    return 1;
}
NW_HOOK_METHOD(0x10026380, NCHtmlViewer, OnComboBox)

// 0x100260a0 - NULL leaves the old one
void NCHtmlViewer::SetBackTexture(const TCHAR* Name)
{
    if (Name)
    {
        if (Name == HtmlTransparent)
            BackTex = NULL;
        else
            BackTex = FindTexture(Name);
    }
}
NW_HOOK_METHOD(0x100260a0, NCHtmlViewer, SetBackTexture)

// 0x100260e0
void NCHtmlViewer::SetOwner(NWndEntry* InOwner)
{
    Owner = InOwner;
}
NW_HOOK_METHOD(0x100260e0, NCHtmlViewer, SetOwner)

// 0x100260f0 - <img src>; no name is the black texture
UTexture* NCHtmlViewer::FindPageTexture(const TCHAR* Name)
{
    if (!Name)
        Name = TEXT("NWindow.BlackTexture");
    return FindTexture(Name);
}
NW_HOOK_METHOD(0x100260f0, NCHtmlViewer, FindPageTexture)

// 0x10027100 - the controls moved with the page, shown while any of them is visible
void NCHtmlViewer::ScrollControls(INT DX, INT DY)
{
    INT N = Controls.Num();
    for (INT i = 0; i < N; i++)
    {
        Controls(i)->MoveWindow(DX, -DY);
        NControl* Control = Controls(i);
        if (Control->LocalY <= Height && Control->LocalY + Control->LocalH >= 0)
            Control->ShowWindow();
        else
            Control->HideWindow();
    }
}
NW_HOOK_METHOD(0x10027100, NCHtmlViewer, ScrollControls)

// 0x10028170 - <a link> over a text run
INT NCHtmlViewer::AddFileLink(INT X, INT Y, INT W, INT H, TCHAR* File)
{
    NCFileLinker* Link = new NCFileLinker(File);
    Link->ID = FileLinkID;
    Link->SetWindowRect(X, Y, W, H);
    CreateChild(Link, NWS_HTMLCONTROL | NWS_VISIBLE);
    ShowIfVisible(this, Link);
    Controls.AddItem(Link);
    return 1;
}
NW_HOOK_METHOD(0x10028170, NCHtmlViewer, AddFileLink)

// 0x10028260 - <a action msg>: msg is a systemmsg line asked first
INT NCHtmlViewer::AddActionLink(INT X, INT Y, INT W, INT H, TCHAR* Action, TCHAR* Msg)
{
    NCActionLinker* Link = new NCActionLinker((INT)Action);
    Link->ID = ActionLinkID;
    Link->SetWindowRect(X, Y, W, H);
    CreateChild(Link, NWS_HTMLCONTROL | NWS_VISIBLE);
    if (Msg)
    {
        INT MsgID = NWtol(Msg);
        if (MsgID > 0)
            appStrcpy(Link->Message, *GL2GameData.SystemMsg[MsgID].Message);
    }
    ShowIfVisible(this, Link);
    Controls.AddItem(Link);
    return 1;
}
NW_HOOK_METHOD(0x10028260, NCHtmlViewer, AddActionLink)

// 0x10028380 - <button>: a label "&$n;" is sysstring n
INT NCHtmlViewer::AddButton(INT X, INT Y, INT W, INT H, TCHAR* NormalTexName,
                            TCHAR* PushedTexName, TCHAR* Action, TCHAR* Label)
{
    TCHAR Head[8];
    NCHtmlButton* Button = new NCHtmlButton(NormalTexName, PushedTexName, Action, 0);
    Button->ID = ButtonID;
    Button->SetWindowRect(X, Y, W, H);
    CreateChild(Button, NWS_HTMLCONTROL | NWS_VISIBLE);
    if (Label)
    {
        INT Length = appStrlen(Label);
        appStrncpy(Head, Label, 3);
        if (!appStrcmp(Head, TEXT("&$")) && Label[Length - 1] == ';')
            Button->SetTextID(appAtoi(Label + 2));
        else
            Button->SetText(Label);
    }
    ShowIfVisible(this, Button);
    Controls.AddItem(Button);
    return 1;
}
NW_HOOK_METHOD(0x10028380, NCHtmlViewer, AddButton)

// 0x100284e0 - <multiedit var>
INT NCHtmlViewer::AddMultiEdit(INT X, INT Y, INT W, INT H, TCHAR* Var)
{
    NCHtmlMultiEdit* Edit = new NCHtmlMultiEdit;
    Edit->ID = MultiEditID;
    Edit->SetWindowRect(X, Y, W, H);
    CreateChild(Edit, NWS_HTMLCONTROL | NWS_VISIBLE);
    Edit->SetCaption(Var);
    ShowIfVisible(this, Edit);
    Controls.AddItem(Edit);
    return 1;
}
NW_HOOK_METHOD(0x100284e0, NCHtmlViewer, AddMultiEdit)

// 0x100285d0 - <edit var type action>: Enter sends the action
INT NCHtmlViewer::AddEdit(INT X, INT Y, INT W, INT H, TCHAR* Var, INT Type, TCHAR* Action)
{
    NCHtmlEdit* Edit = new NCHtmlEdit(100, Type);
    Edit->SetWindowRect(X, Y, W, H);
    CreateChild(Edit, NWS_HTMLCONTROL | NWS_VISIBLE);
    Edit->ID = EditID;
    Edit->SetCaption(Var);
    Edit->EnterCommand = Action;
    ShowIfVisible(this, Edit);
    Controls.AddItem(Edit);
    return 1;
}
NW_HOOK_METHOD(0x100285d0, NCHtmlViewer, AddEdit)

// 0x10029400 - <combobox var list sel>: always left hidden (ShowWindow shows it)
INT NCHtmlViewer::AddComboBox(INT X, INT Y, INT W, INT H, TCHAR* Var, TCHAR* List, TCHAR* Sel)
{
    NCHtmlComboBox* Combo = new NCHtmlComboBox;
    Combo->SetWindowRect(X, Y, W, H);
    CreateChild(Combo, NWS_HTMLCONTROL | NWS_VISIBLE);
    Combo->SetCaption(Var);
    Combo->ID = ComboBoxID;
    Combo->SetList(List, Sel);
    ShowIfVisible(this, Combo);
    Combo->HideWindow();
    Controls.AddItem(Combo);
    return 1;
}
NW_HOOK_METHOD(0x10029400, NCHtmlViewer, AddComboBox)

// 0x10029510 - back to an empty page. The controls are taken off the window but not deleted;
// an open question of an action link is closed.
void NCHtmlViewer::Clear()
{
    if (bLoaded && Doc)
    {
        Doc->Release();
        if (Doc)
            delete Doc;
        Doc = NULL;
    }
    INT N = Controls.Num();
    for (INT i = 0; i < N; i++)
    {
        if (Controls(i)->IsA(NClass::NCActionLinker()))
        {
            NCDialogBox* Box = GDesktop->MainDialogBox;
            if (Box->DialogID == DIALOG_ActionLink)
            {
                Box->HideWindow();
                GDesktop->MainDialogBox->Reset();
            }
        }
        RemoveChildWindow(Controls(i)->Entry);
    }
    Controls.Empty();
    bLoaded = 0;
    ScrollY = 0;
    if (ScrollBar)
        ScrollBar->SetScrollPos(0);
    DeltaY = 0;
}
NW_HOOK_METHOD(0x10029510, NCHtmlViewer, Clear)

// 0x1002bac0 - the tree from the tag list: <html> (whose imgsrc is the background) or <body>
// starts the page, which runs to its closing tag; laid out at the width without the scroll
// bar. TAB then goes through the controls in page order and back to the viewer.
void NCHtmlViewer::BuildDocument(NHtmlTagList* Tags)
{
    NHtmlTag** It = Tags->First;
    while (It != Tags->Last)
    {
        NHtmlTag* Tag = *It;
        if (Tag->Tag == HTMLTAG_Html)
        {
            SetBackTexture(Tag->HasAttr(ATTR_Html_ImgSrc) ? Tag->GetAttr(ATTR_Html_ImgSrc, 0)
                                                          : HtmlTransparent);
        }
        else if (Tag->Tag != HTMLTAG_Body)
        {
            It++;
            continue;
        }
        Doc = new NHtmlBlock(NULL, this);
        Doc->Parse(&It, 0);
    }
    if (!Doc)
        return;

    Doc->Layout(Width - 16, Height);
    Doc->Arrange(Width - 16, Height);
    INT N = Controls.Num();
    if (N > 1)
    {
        SetTabOrder(Controls(0)->Entry, Controls(N - 1)->Entry);
        Controls(0)->SetTabOrder(Controls(1)->Entry, Entry);
        Controls(N - 1)->SetTabOrder(Entry, Controls(N - 2)->Entry);
        for (INT i = 1; i < N - 1; i++)
            Controls(i)->SetTabOrder(Controls(i + 1)->Entry, Controls(i - 1)->Entry);
    }
}
NW_HOOK_METHOD(0x1002bac0, NCHtmlViewer, BuildDocument)

// 0x1002bc70 - a page from a file (links, the tutorial)
INT NCHtmlViewer::LoadHtmlFile(TCHAR* Source)
{
    NHtmlParser Parser;
    TCHAR* Text = Parser.LoadFile(Source);
    if (!Text)
    {
        Debugf(TEXT("Can not open Html file : %s"), Source);
        return 0;
    }
    if (bLoaded)
        Clear();
    NHtmlTagList* Tags = Parser.Parse(Text);
    BuildDocument(Tags);
    Parser.FreeTags(Tags);
    Parser.FreeFile();
    bLoaded = 1;
    return 1;
}
NW_HOOK_METHOD(0x1002bc70, NCHtmlViewer, LoadHtmlFile)

// 0x1002bd60 - a page from text (the server's NPC dialogs)
INT NCHtmlViewer::LoadHtml(TCHAR* Text)
{
    NHtmlParser Parser;
    if (!Text)
        return 0;
    if (bLoaded)
        Clear();
    NHtmlTagList* Tags = Parser.Parse(Text);
    BuildDocument(Tags);
    Parser.FreeTags(Tags);
    bLoaded = 1;
    return 1;
}
NW_HOOK_METHOD(0x1002bd60, NCHtmlViewer, LoadHtml)

NW_CHECK_VTABLE(NCHtmlViewer, 0x100de828, 72)

/*-----------------------------------------------------------------------------
    NCTutorialHtmlViewer.
-----------------------------------------------------------------------------*/

// 0x10029610
NCTutorialHtmlViewer::NCTutorialHtmlViewer()
{
}
NW_HOOK_CTOR(0x10029610, NCTutorialHtmlViewer)

// 0x10029660
NW_HOOK_DTOR(0x10029660, NCTutorialHtmlViewer)

// 0x100263d0 (vslot 24)
INT NCTutorialHtmlViewer::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCHtmlViewer::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x100263d0, NCTutorialHtmlViewer, WindowProc)

// 0x10026410 (vslot 35)
INT NCTutorialHtmlViewer::OnCreate()
{
    NCHtmlViewer::OnCreate();
    return 1;
}
NW_HOOK_METHOD(0x10026410, NCTutorialHtmlViewer, OnCreate)

// stock body shared with NCBBSHtmlViewer's (0x1002a890)
INT NCTutorialHtmlViewer::OnDestroy()
{
    NCHtmlViewer::OnDestroy();
    return 1;
}

NW_CHECK_VTABLE(NCTutorialHtmlViewer, 0x100de990, 72)
