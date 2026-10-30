/*=============================================================================
    NCLobbyWnd.cpp - the character selection screen. Stock: 0x10068eb0 .. 0x10069193,
    0x10069e20 .. 0x1006a701 (NCPawnSelectWnd's functions sit in between).
=============================================================================*/
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCPawnSelectWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// NWM_COMMAND ids of the buttons (names ours; 0x10001 is handled but nothing sends it)
enum
{
    NID_LOBBY_10001 = 0x10001,
    NID_LOBBY_START = 0x1009,
    NID_LOBBY_PREVIOUS = 0x100A,
    NID_LOBBY_CREATE = 0x100B,
    NID_LOBBY_DELETE = 0x100C,
};

// systemmsg lines (names ours)
enum
{
    MSG_TooManyCharacters = 77,    // "You cannot create another character. ..."
    MSG_DeleteCharacter = 78,      // "Do you want to delete $s1?"
    MSG_BadCharacterName = 205,    // "Incorrect character name. Please ask the GM."
};

// dialog id of the delete question: only its OK comes back (NWM_DIALOG_RESULT)
enum { DIALOG_DeleteCharacter = 0 };

// the characters of the stage as the console keeps them (NConsoleWnd +0x25F8, eight User
// records; not placed in its declaration yet, name ours)
static User* ConsoleCharacters() { return GConsole->SelectCharacterInfo; }

// 0x10069e20 - the panel and the box are left as allocated
NCLobbyWnd::NCLobbyWnd()
{
    StartButton = NULL;
    PreviousButton = NULL;
    CreateButton = NULL;
    DeleteButton = NULL;
    NumCharacters = 0;
}
NW_HOOK_CTOR(0x10069e20, NCLobbyWnd)

// 0x10069e90
NW_HOOK_DTOR(0x10069e90, NCLobbyWnd)

// 0x10068eb0 (vslot 24) - the delete question's answer arrives as NWM_DIALOG_RESULT
INT NCLobbyWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_LOBBY_10001)
            return OnCommand10001() != 0;
        if (wParam == NID_LOBBY_START)
            return OnStart() != 0;
        if (wParam == NID_LOBBY_PREVIOUS)
            return OnPrevious() != 0;
        if (wParam == NID_LOBBY_CREATE)
            return OnCreateCharacter() != 0;
        if (wParam == NID_LOBBY_DELETE)
            return OnDelete() != 0;
    }
    else if (Msg == NWM_DIALOG_RESULT)
        return OnDeleteAnswer((INT)wParam, (INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10068eb0, NCLobbyWnd, WindowProc)

// 0x1006a050 (vslot 35) - the panel in the upper left corner, the box centered, the buttons
// stacked in the lower right corner (Width / Height halved unsigned, as in the stock code).
// Create is off on test servers; Delete and Start wait for a choice.
INT NCLobbyWnd::OnCreate()
{
    NCWnd::OnCreate();

    PawnSelectWnd = new NCPawnSelectWnd;
    PawnSelectWnd->SetWindowRect(10, 10, 240, 133);
    CreateChild(PawnSelectWnd, NWS_NOCLOSE | NWS_VISIBLE);
    PawnSelectWnd->SetTitle(*GL2GameData.SysString[157]);      // "Select Character"

    DialogBox = new NCDialogBox;
    DialogBox->SetWindowRect(((DWORD)Width >> 1) - 101, ((DWORD)Height >> 1) - 44, 203, 87);
    CreateChild(DialogBox, 0x20 | NWS_VISIBLE);
    DialogBox->SetButtonTexts(184, 185);                       // "Yes", "No"
    DialogBox->HideWindow();

    CreateButton = new NCButton(TEXT("L2UI.ActionWnd.PledgeButten1"),
                                TEXT("L2UI.ActionWnd.PledgeButten2"), 0);
    CreateButton->ID = NID_LOBBY_CREATE;
    CreateButton->SetWindowRect(Width - 131, Height - 172, 99, 19);
    CreateChild(CreateButton, NWS_VISIBLE);
    CreateButton->SetTextID(158);                              // "Create"
    if (GL2TestServer)
        CreateButton->EnableWindow(0);

    DeleteButton = new NCButton(TEXT("L2UI.ActionWnd.PledgeButten1"),
                                TEXT("L2UI.ActionWnd.PledgeButten2"), 0);
    DeleteButton->ID = NID_LOBBY_DELETE;
    DeleteButton->SetWindowRect(Width - 131, Height - 148, 99, 19);
    CreateChild(DeleteButton, NWS_VISIBLE);
    DeleteButton->SetTextID(159);                              // "Delete"
    DeleteButton->EnableWindow(0);

    StartButton = new NCButton(TEXT("L2UI.ActionWnd.PledgeButten1"),
                               TEXT("L2UI.ActionWnd.PledgeButten2"), 0);
    StartButton->ID = NID_LOBBY_START;
    StartButton->SetWindowRect(Width - 131, Height - 124, 99, 19);
    CreateChild(StartButton, NWS_VISIBLE);
    StartButton->SetTextID(160);                               // "Start"
    StartButton->EnableWindow(0);

    PreviousButton = new NCButton(TEXT("L2UI.ActionWnd.PledgeButten1"),
                                  TEXT("L2UI.ActionWnd.PledgeButten2"), 0);
    PreviousButton->ID = NID_LOBBY_PREVIOUS;
    PreviousButton->SetWindowRect(Width - 131, Height - 100, 99, 19);
    CreateChild(PreviousButton, NWS_VISIBLE);
    PreviousButton->SetTextID(161);                            // "Previous"
    return 1;
}
NW_HOOK_METHOD(0x1006a050, NCLobbyWnd, OnCreate)

// 0x1006a510 (vslot 36)
INT NCLobbyWnd::OnDestroy()
{
    if (PawnSelectWnd)
    {
        RemoveChildWindow(PawnSelectWnd->Entry);
        delete PawnSelectWnd;
        PawnSelectWnd = NULL;
    }
    if (DialogBox)
    {
        RemoveChildWindow(DialogBox->Entry);
        delete DialogBox;
        DialogBox = NULL;
    }
    if (StartButton)
    {
        RemoveChildWindow(StartButton->Entry);
        delete StartButton;
        StartButton = NULL;
    }
    if (PreviousButton)
    {
        RemoveChildWindow(PreviousButton->Entry);
        delete PreviousButton;
        PreviousButton = NULL;
    }
    if (CreateButton)
    {
        RemoveChildWindow(CreateButton->Entry);
        delete CreateButton;
        CreateButton = NULL;
    }
    if (DeleteButton)
    {
        RemoveChildWindow(DeleteButton->Entry);
        delete DeleteButton;
        DeleteButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1006a510, NCLobbyWnd, OnDestroy)

// 0x10069160 (vslot 42) - nothing chosen yet
void NCLobbyWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    DeleteButton->EnableWindow(0);
    StartButton->EnableWindow(0);
    DialogBox->HideWindow();
}
NW_HOOK_METHOD(0x10069160, NCLobbyWnd, ShowWindow)

// 0x10069ee0 (vslot 50) - each character's name over its head: half its collision height
// above the location, plus 7 (4 more for mesh type 0). Every record is copied whole first, as
// in the stock code. The clip goes on before BeginPaint.
INT NCLobbyWnd::OnPaint(NCGDevice* Device)
{
    guard(NCLobbyWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    for (INT i = 0; i < 8; i++)
    {
        User Info = ConsoleCharacters()[i];
        if (Info.Pawn)
        {
            FVector Loc = Info.Pawn->Location;
            FLOAT Z = Info.Pawn->CollisionHeight * 0.5f + Loc.Z + 7.f;
            if (Info.GetMeshType() == 0)
                Z += 4.f;
            Device->Draw3DCoordText(FVector(Loc.X, Loc.Y, Z), 0xFFFFFFFF, Info.GetName(), NULL, 2,
                                    0, (L2FontType)1);
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10069ee0, NCLobbyWnd, OnPaint)

// 0x10069030 (vslot 63) - enter the world with the chosen character, unless its name has
// characters the font cannot draw
INT NCLobbyWnd::OnStart()
{
    INT Index = PawnSelectWnd->GetSelected();
    if (NumCharacters > 0 && Index >= 0 && Index < NumCharacters)
    {
        const TCHAR* Name = PawnSelectWnd->NameCombo->GetItemText(Index);
        if (Name && *Name && GConsole->HasInvalidChars((TCHAR*)Name))
        {
            GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[MSG_BadCharacterName].Message);
            return 1;
        }
        GConsole->RequestGameStart(Index);
        GConsole->StopIntroMusic();
    }
    return 1;
}
NW_HOOK_METHOD(0x10069030, NCLobbyWnd, OnStart)

// 0x100690c0 (vslot 64)
INT NCLobbyWnd::OnPrevious()
{
    GConsole->ReturnToLogin();
    return 1;
}
NW_HOOK_METHOD(0x100690c0, NCLobbyWnd, OnPrevious)

// 0x100690e0 (vslot 65) - seven characters at most
INT NCLobbyWnd::OnCreateCharacter()
{
    if (NumCharacters == 7)
    {
        GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[MSG_TooManyCharacters].Message);
        return 1;
    }
    if (NumCharacters < 7)
        GConsole->RequestNewCharacter();
    return 1;
}
NW_HOOK_METHOD(0x100690e0, NCLobbyWnd, OnCreateCharacter)

// 0x1006a630 (vslot 66) - "Do you want to delete ...?" (the selection is not checked, as in the
// stock code)
INT NCLobbyWnd::OnDelete()
{
    TCHAR Name[32];
    TCHAR Text[128];
    appSprintf(Name, TEXT("%s"), PawnSelectWnd->GetCharacterName(PawnSelectWnd->GetSelected()));
    GConsole->FormatSystemMessage(Text, *GL2GameData.SystemMsg[MSG_DeleteCharacter].Message, Name,
                                  NULL);
    NDialogInfo Info;
    Info.ID = DIALOG_DeleteCharacter;
    Info.bMainBox = 0;
    Info.bInput = 0;
    Info.Owner = Entry;
    Info.SetText(Text);
    DialogBox->SetInfo(&Info);
    DialogBox->ShowWindow();
    return 1;
}
NW_HOOK_METHOD(0x1006a630, NCLobbyWnd, OnDelete)

// 0x10069130 (vslot 67) - the delete question's OK (Cancel sends nothing for this dialog)
INT NCLobbyWnd::OnDeleteAnswer(INT bYes, INT Unused)
{
    INT Index = PawnSelectWnd->GetSelected();
    if (Index >= 0)
        GConsole->RequestCharacterDelete(Index);
    return 1;
}
NW_HOOK_METHOD(0x10069130, NCLobbyWnd, OnDeleteAnswer)

// 0x10068fc0 - one character of the server's list (NConsoleWnd)
INT NCLobbyWnd::AddCharacter(User Info)
{
    if (NumCharacters >= 7)
        return 0;
    PawnSelectWnd->AddCharacter(Info);
    NumCharacters++;
    return 1;
}
NW_HOOK_METHOD(0x10068fc0, NCLobbyWnd, AddCharacter)

// 0x10069010 - NConsoleWnd::ReturnToLogin and before a new list
void NCLobbyWnd::ClearCharacters()
{
    PawnSelectWnd->ClearCharacters();
    NumCharacters = 0;
}
NW_HOOK_METHOD(0x10069010, NCLobbyWnd, ClearCharacters)

NW_CHECK_VTABLE(NCLobbyWnd, 0x100e5c98, 68)
