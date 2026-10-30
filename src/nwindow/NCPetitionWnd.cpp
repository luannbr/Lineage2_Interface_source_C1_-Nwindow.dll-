/*=============================================================================
    NCPetitionWnd.cpp - the petition windows, the player's and the GM's. Stock: 0x100472e0 ..
    0x1004737f, 0x10094380 .. 0x100947d3, 0x100949c0 .. 0x10094bcd, 0x10094f10 ..
    0x10095587 and NCGMPetitionWnd's 0x1003e810 .. 0x1003e8af (ShowWindow is
    NCSystemMenuWnd's body, HideWindow NCChatWnd's).
=============================================================================*/
#include "nwindow/NCPetitionWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCPetitionListBox.h"
#include "nwindow/NCommandParser.h"
#include "nwindow/NConsoleWnd.h"

// chat types of the petition conversation (names ours)
enum { CHAT_Petition = 6, CHAT_GMPetition = 7 };

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

// the conversation list of both windows (repeated in the stock OnCreates): 16 lines seen,
// 200 kept, a bare list (NWS_NOHITTEST, not NWS_VISIBLE) that follows the last line
static FORCEINLINE NCPetitionListBox* CreatePetitionList(NCWnd* Parent, INT ID)
{
    NCPetitionListBox* List = new NCPetitionListBox(200, 16, 12);
    List->ID = ID;
    List->SetWindowRect(15, 38, 282, 195);
    Parent->CreateChild(List, NWS_NOHITTEST);
    List->bAutoScroll = 1;
    List->LineGap = 0;
    return List;
}

// the line to write in (repeated in the stock OnCreates)
static FORCEINLINE NCEditBox* CreatePetitionEdit(NCWnd* Parent, INT ID)
{
    NCEditBox* Edit = new NCEditBox(1, NULL, 1, 2600);
    Edit->ID = ID;
    Edit->SetWindowRect(15, 239, 269, 16);
    Parent->CreateChild(Edit, NWS_VISIBLE);
    return Edit;
}

// a button under the list (repeated in the stock OnCreates)
static FORCEINLINE NCButton* CreatePetitionButton(NCWnd* Parent, INT ID, INT X, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("L2UI.control.DefaultButton"),
                                    TEXT("L2UI.control.DefaultButton_click"), 0);
    Button->ID = ID;
    Button->SetWindowRect(X, 270, 64, 19);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->SetText(*GL2GameData.SysString[TextID]);
    return Button;
}

// the title bar of both windows (repeated in the stock OnCreates)
static FORCEINLINE void CreatePetitionFrame(NCWnd* Wnd, INT ID)
{
    Wnd->FrameCtrl = new NCFrameCtrl(1);
    Wnd->FrameCtrl->ID = ID;
    Wnd->FrameCtrl->SetWindowRect(0, 0, 300, 21);
    Wnd->CreateChild(Wnd->FrameCtrl, NWS_VISIBLE);
}

// the background under the title bar (one stock body for both windows, 0x10094a60); the
// texture is not checked
static FORCEINLINE INT PaintPetitionWnd(NCWnd* Wnd, UTexture* BackTex, NCGDevice* Device)
{
    Device->BeginPaint(Wnd->ScreenX, Wnd->ScreenY);
    Device->SetClip(Wnd->ScreenX, Wnd->ScreenY, Wnd->Width, Wnd->Height);
    Device->DrawTexture(0, 21, 300, 280, 0, 0, 300, 280, BackTex);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}

// restored from its icon, the window comes back where the icon was (NCSystemMenuWnd's
// ShowWindow, 0x10078950, one body for six windows)
static FORCEINLINE void ShowFromIcon(NCWnd* Wnd)
{
    Wnd->NCWnd::ShowWindow();
    if (Wnd->Icon)
    {
        Wnd->SetWindowRect(Wnd->Icon->ScreenX, Wnd->Icon->ScreenY, Wnd->Width, Wnd->Height);
        Wnd->Icon->SetWindowRect(0, 0, 0, 0);
        Wnd->Icon->HideWindow();
    }
}

// a line of the conversation: the player's own petition lines (type 6) in white
static FORCEINLINE void AddPetitionMessage(NCPetitionListBox* List, TCHAR* Text, DWORD Color,
                                           INT Type)
{
    if (!List)
        return;
    if (Type == CHAT_Petition)
        List->AddString(Text, 0xFFFFFFFF, -1);
    else
        List->AddString(Text, Color, -1);
}

/*-----------------------------------------------------------------------------
    NCPetitionWnd
-----------------------------------------------------------------------------*/

// 0x100472e0 - the fields are left as allocated
NCPetitionWnd::NCPetitionWnd()
{
}
NW_HOOK_CTOR(0x100472e0, NCPetitionWnd)

// 0x10047330
NW_HOOK_DTOR(0x10047330, NCPetitionWnd)

// 0x10094380 (vslot 24)
INT NCPetitionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_PETITION_SEND)
            return OnSend() != 0;
        if (wParam == NID_PETITION_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_PETITION_EDIT)
            return OnEdit(lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10094380, NCPetitionWnd, WindowProc)

// 0x10094f10 (vslot 35)
INT NCPetitionWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.etcwnd.Petitionback"));
    CreatePetitionFrame(this, NID_PETITION_FRAME);
    ListBox = CreatePetitionList(this, NID_PETITION_LIST);
    EditBox = CreatePetitionEdit(this, NID_PETITION_EDIT);
    CancelButton = CreatePetitionButton(this, NID_PETITION_CANCEL, 121, 473);
    return 1;
}
NW_HOOK_METHOD(0x10094f10, NCPetitionWnd, OnCreate)

// 0x100949c0 (vslot 36) - UnusedA0 is not looked at
INT NCPetitionWnd::OnDestroy()
{
    DestroyChild(this, ListBox);
    DestroyChild(this, EditBox);
    DestroyChild(this, CancelButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100949c0, NCPetitionWnd, OnDestroy)

// stock body shared with NCSystemMenuWnd's (0x10078950)
void NCPetitionWnd::ShowWindow()
{
    ShowFromIcon(this);
}

// stock body shared with NCChatWnd's (0x1003dab0)
void NCPetitionWnd::HideWindow()
{
    NCWnd::HideWindow();
}

// 0x10094a60 (vslot 50) - also NCGMPetitionWnd's
INT NCPetitionWnd::OnPaint(NCGDevice* Device)
{
    return PaintPetitionWnd(this, BackTex, Device);
}
NW_HOOK_METHOD(0x10094a60, NCPetitionWnd, OnPaint)

// 0x10094430 (vslot 62) - Enter: the line to the GM (chat type 6), then emptied
INT NCPetitionWnd::OnEdit(LPARAM lParam)
{
    if (lParam == 1)
    {
        const TCHAR* Text = EditBox->CommitText();
        if (Text)
            GConsole->SendChatMsgToServer(CHAT_Petition, (TCHAR*)Text, NULL);
        EditBox->ClearText();
    }
    return 1;
}
NW_HOOK_METHOD(0x10094430, NCPetitionWnd, OnEdit)

// 0x100944e0 (vslot 64)
INT NCPetitionWnd::OnCancel()
{
    GConsole->RequestPetitionCancel();
    return 1;
}
NW_HOOK_METHOD(0x100944e0, NCPetitionWnd, OnCancel)

// 0x10094480 - Param is not used; also the desktop's calls on NCGMPetitionWnd
void NCPetitionWnd::AddMessage(TCHAR* Text, DWORD Color, INT Type, INT Param)
{
    AddPetitionMessage(ListBox, Text, Color, Type);
}
NW_HOOK_METHOD(0x10094480, NCPetitionWnd, AddMessage)

// 0x100944c0 - the line and the conversation empty; also NCGMPetitionWnd's
void NCPetitionWnd::Clear()
{
    EditBox->ClearText();
    ListBox->Clear();
}
NW_HOOK_METHOD(0x100944c0, NCPetitionWnd, Clear)

NW_CHECK_VTABLE(NCPetitionWnd, 0x100e1e88, 65)

/*-----------------------------------------------------------------------------
    NCGMPetitionWnd
-----------------------------------------------------------------------------*/

// 0x1003e810 - the fields are left as allocated
NCGMPetitionWnd::NCGMPetitionWnd()
{
}
NW_HOOK_CTOR(0x1003e810, NCGMPetitionWnd)

// 0x1003e860
NW_HOOK_DTOR(0x1003e860, NCGMPetitionWnd)

// 0x10094500 (vslot 24)
INT NCGMPetitionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_GMPETITION_START)
            return OnStart() != 0;
        if (wParam == NID_GMPETITION_END)
            return OnEnd() != 0;
        if (wParam == NID_GMPETITION_EDIT)
            return OnEdit(lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10094500, NCGMPetitionWnd, WindowProc)

// 0x100951f0 (vslot 35)
INT NCGMPetitionWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.etcwnd.Petitionback"));
    CreatePetitionFrame(this, NID_GMPETITION_FRAME);
    ListBox = CreatePetitionList(this, NID_GMPETITION_LIST);
    EditBox = CreatePetitionEdit(this, NID_GMPETITION_EDIT);
    StartButton = CreatePetitionButton(this, NID_GMPETITION_START, 70, 474);
    EndButton = CreatePetitionButton(this, NID_GMPETITION_END, 170, 475);
    return 1;
}
NW_HOOK_METHOD(0x100951f0, NCGMPetitionWnd, OnCreate)

// 0x10094af0 (vslot 36)
INT NCGMPetitionWnd::OnDestroy()
{
    DestroyChild(this, ListBox);
    DestroyChild(this, EditBox);
    DestroyChild(this, StartButton);
    DestroyChild(this, EndButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10094af0, NCGMPetitionWnd, OnDestroy)

// stock body shared with NCSystemMenuWnd's (0x10078950)
void NCGMPetitionWnd::ShowWindow()
{
    ShowFromIcon(this);
}

// stock body shared with NCChatWnd's (0x1003dab0)
void NCGMPetitionWnd::HideWindow()
{
    NCWnd::HideWindow();
}

// stock body shared with NCPetitionWnd's (0x10094a60)
INT NCGMPetitionWnd::OnPaint(NCGDevice* Device)
{
    return PaintPetitionWnd(this, BackTex, Device);
}

// 0x100945b0 (vslot 62) - Enter: a "//" line goes as a builder command, a plain one as the
// GM's petition chat (type 7), another /command nowhere; the line is emptied unless it was
// empty
INT NCGMPetitionWnd::OnEdit(LPARAM lParam)
{
    if (lParam != 1)
        return 1;
    const TCHAR* Text = EditBox->CommitText();
    if (!Text)
        return 1;
    TCHAR Line[1024];
    appStrcpy(Line, Text);
    NCommandParser Parser;
    INT Command = Parser.GetCommand(Line);
    if (Command == NCMD_Builder)
    {
        L2ParamStack Params(10);
        Params.PushBack(Line + 2);
        GConsole->BuilderCommand(Params);
    }
    else if (Command == NCMD_Chat)
        GConsole->SendChatMsgToServer(CHAT_GMPetition, (TCHAR*)Text, NULL);
    EditBox->ClearText();
    return 1;
}
NW_HOOK_METHOD(0x100945b0, NCGMPetitionWnd, OnEdit)

// 0x100946e0 (vslot 63)
INT NCGMPetitionWnd::OnStart()
{
    L2ParamStack Params(10);
    Params.PushBack((void*)TEXT("startpet"));
    GConsole->BuilderCommand(Params);
    return 1;
}
NW_HOOK_METHOD(0x100946e0, NCGMPetitionWnd, OnStart)

// 0x10094760 (vslot 64)
INT NCGMPetitionWnd::OnEnd()
{
    L2ParamStack Params(10);
    Params.PushBack((void*)TEXT("endpet"));
    GConsole->BuilderCommand(Params);
    return 1;
}
NW_HOOK_METHOD(0x10094760, NCGMPetitionWnd, OnEnd)

// stock body shared with NCPetitionWnd's (0x10094480)
void NCGMPetitionWnd::AddMessage(TCHAR* Text, DWORD Color, INT Type, INT Param)
{
    AddPetitionMessage(ListBox, Text, Color, Type);
}

// stock body shared with NCPetitionWnd's (0x100944c0)
void NCGMPetitionWnd::Clear()
{
    EditBox->ClearText();
    ListBox->Clear();
}

NW_CHECK_VTABLE(NCGMPetitionWnd, 0x100e0af8, 65)
