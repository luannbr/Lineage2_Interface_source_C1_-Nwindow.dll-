/*=============================================================================
    NCPawnSelectWnd.cpp - the character panel of the character selection.
    Stock: 0x10068b60 .. 0x10068ea2 (NCLobbyWnd's functions follow), 0x100691a0 ..
    0x10069e0e (ShowWindow is a body the linker folded with other classes).
=============================================================================*/
#include "nwindow/NCPawnSelectWnd.h"
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <wchar.h>

// NWM_COMMAND id of the name combo box, title bar id (names ours)
enum
{
    NID_PAWNSELECT_NAME = 0x1F04,
    NID_PAWNSELECT_FRAME = (INT)0xAFF12011,
};

// systemmsg line (name ours): "You have $s1 day(s) left until deletion. Do you want to cancel
// deletion?"
enum { MSG_RestoreCharacter = 180 };

// dialog id of the restore question: its answer comes back as NWM_DIALOG_RESULT
enum { DIALOG_RestoreCharacter = 1 };

// exp at the start of each level (index = level): the character selection's own copy
#if NW_HOOK_MODE
#define GLobbyExpTable (reinterpret_cast<const INT*>(::nw::Addr(0x100e593c)))
#else
extern const INT GLobbyExpTable[];
#endif

// the console's character selection
static NCLobbyWnd* ConsoleLobbyWnd() { return GConsole->LobbyWnd; }

// 0x100691a0 - the seven User records are built by Engine's constructor
NCPawnSelectWnd::NCPawnSelectWnd()
{
    BackTex = NULL;
    HPTex = NULL;
    MPTex = NULL;
    EXPTex = NULL;
    UnknownA4 = 0;
    NumCharacters = 0;
    Selected = -1;
}
NW_HOOK_CTOR(0x100691a0, NCPawnSelectWnd)

// 0x10069270
NW_HOOK_DTOR(0x10069270, NCPawnSelectWnd)

// 0x10068b60 (vslot 24) - the combo box reports the chosen name, the restore question its
// answer
INT NCPawnSelectWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_PAWNSELECT_NAME)
            return OnSelect((INT)lParam) != 0;
    }
    else if (Msg == NWM_DIALOG_RESULT)
        return OnRestoreAnswer((INT)wParam, (INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10068b60, NCPawnSelectWnd, WindowProc)

// 0x100692c0 (vslot 35)
INT NCPawnSelectWnd::OnCreate()
{
    NCWnd::OnCreate();

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_PAWNSELECT_FRAME;
    FrameCtrl->ID = NID_PAWNSELECT_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    BackTex = FindTexture(TEXT("L2UI.statusWnd.selectback"));
    HPTex = FindTexture(TEXT("L2UI.StatusWnd.statusHP1"));
    MPTex = FindTexture(TEXT("L2UI.StatusWnd.statusMP1"));
    EXPTex = FindTexture(TEXT("L2UI.StatusWnd.statusEXP1"));
    for (INT i = 0; i < 7; i++)
        Characters[i].Pawn = NULL;

    NameCombo = new NCComboBox;
    NameCombo->ID = NID_PAWNSELECT_NAME;
    NameCombo->SetWindowRect(57, 27, 126, 15);
    CreateChild(NameCombo, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x100692c0, NCPawnSelectWnd, OnCreate)

// 0x10069bd0 (vslot 36)
INT NCPawnSelectWnd::OnDestroy()
{
    if (NameCombo)
    {
        RemoveChildWindow(NameCombo->Entry);
        delete NameCombo;
        NameCombo = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10069bd0, NCPawnSelectWnd, OnDestroy)

// stock body shared with NCEditBox's (0x100a5d70)
void NCPawnSelectWnd::ShowWindow()
{
    NCWnd::ShowWindow();
}

// 0x10069470 (vslot 50) - labels, then the chosen character: bars 84 pixels wide (exp 189),
// "current / max" in the digit font, exp in per cent, level and class, karma. The exp per cent
// is worked out even when the next level's exp equals this one's. The clip goes on before
// BeginPaint.
INT NCPawnSelectWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPawnSelectWnd::OnPaint);
    TCHAR Buf[64];
    SIZE Size;
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 110, 0, 0, 240, 110, BackTex);
    Device->DrawText(23, 8, 0xFFFFFFFF, *GL2GameData.SysString[157], 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);                                // "Select Character"
    Device->DrawText(10, 29, 0xFFA3A3A3, *GL2GameData.SysString[50], 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);                                // "Name"
    Device->DrawText(16, 70, 0xFFA3A3A3, *GL2GameData.SysString[88], 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);                                // "Lv"
    Device->DrawText(115, 114, 0xFFFFFFFF, *GL2GameData.SysString[101], 0, 0, 0, L2FONT_Normal,
                     0, 0, 0, 0, 0, 0);                             // "Karma"
    if (Selected >= 0 && NumCharacters > 0)
    {
        User& Info = Characters[Selected];
        INT HPW = 0;
        INT MPW = 0;
        INT EXPW = 0;
        if (Info.MaxHP)
            HPW = Info.HP * 84 / Info.MaxHP;
        if (Info.MaxMP)
            MPW = Info.MP * 84 / Info.MaxMP;
        if (GLobbyExpTable[Info.Level + 1])
            EXPW = (INT)((DOUBLE)(Info.Exp - GLobbyExpTable[Info.Level]) * 189.0 /
                         (GLobbyExpTable[Info.Level + 1] - GLobbyExpTable[Info.Level]));

        if (HPW > 0 && HPW <= 84)
            Device->DrawTexture(34, 100, HPW, 10, 0, 0, HPW, 10, HPTex);
        else if (HPW > 84)
            Device->DrawTexture(34, 100, 84, 10, 0, 0, 84, 10, HPTex);
        if (MPW > 0 && MPW <= 84)
            Device->DrawTexture(139, 100, MPW, 10, 0, 0, MPW, 10, MPTex);
        else if (MPW > 84)
            Device->DrawTexture(139, 100, 84, 10, 0, 0, 84, 10, MPTex);
        if (EXPW > 0 && EXPW <= 189)
            Device->DrawTexture(34, 87, EXPW, 10, 0, 0, EXPW, 10, EXPTex);
        else if (EXPW > 189)
            Device->DrawTexture(34, 87, 189, 10, 0, 0, 189, 10, EXPTex);

        INT LevelExp = GLobbyExpTable[Info.Level];
        swprintf(Buf, ARRAY_COUNT(Buf), TEXT("%.2f%s"),
                 (DOUBLE)(Info.Exp - LevelExp) * 100.0 / (GLobbyExpTable[Info.Level + 1] - LevelExp),
                 TEXT("%"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 128 - Size.cx / 2, 88, 0xFFFFFFFF, Buf);

        appSprintf(Buf, TEXT("%s"), TEXT("/"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 76 - Size.cx / 2, 101, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%d"), Info.HP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 66 - Size.cx, 101, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%d"), Info.MaxHP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 82, 101, 0xFFFFFFFF, Buf);

        appSprintf(Buf, TEXT("%s"), TEXT("/"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 181 - Size.cx / 2, 101, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%d"), Info.MP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 171 - Size.cx, 101, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%d"), Info.MaxMP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 187, 101, 0xFFFFFFFF, Buf);

        appSprintf(Buf, TEXT("%d"), Info.SP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawText(76 - Size.cx / 2, 114, 0xFFB09B79, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                         0, 0, 0);

        appSprintf(Buf, TEXT("%d %s"), Info.Level, GConsole->GetClassNameString(Info.ClassID));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawText(35, 70, 0xFFB09B79, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);

        if (Info.Karma < 999999)
            appSprintf(Buf, TEXT("%d"), Info.Karma);
        else
            appSprintf(Buf, TEXT("%d+"), Info.Karma);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawText(181 - Size.cx / 2, 114, 0xFFB09B79, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                         0, 0, 0);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10069470, NCPawnSelectWnd, OnPaint)

// 0x10068d20 (vslot 62)
void NCPawnSelectWnd::SelectCharacter(INT Index)
{
    OnSelect(Index);
    if (NameCombo)
        NameCombo->SetSelected(Index);
}
NW_HOOK_METHOD(0x10068d20, NCPawnSelectWnd, SelectCharacter)

// 0x10069c10 (vslot 63) - a character being deleted asks first whether to restore it (and
// the lobby buttons wait for the answer); otherwise the stage turns to it. An index one past
// the last is accepted, and -1 reads the record before the first, as in the stock code.
INT NCPawnSelectWnd::OnSelect(INT Index)
{
    TCHAR Days[32];
    TCHAR Text[128];
    if (NumCharacters < Index)
        Selected = -1;
    else
        Selected = Index;
    if (Characters[Selected].DeleteDays > 0)
    {
        GConsole->SelectedCharacterNum(-1);
        appSprintf(Days, TEXT("%d"), Characters[Selected].DeleteDays);
        GConsole->FormatSystemMessage(Text, *GL2GameData.SystemMsg[MSG_RestoreCharacter].Message,
                                      Days, NULL);
        NDialogInfo Info;
        Info.ID = DIALOG_RestoreCharacter;
        Info.bMainBox = 1;
        Info.bInput = 0;
        Info.Owner = Entry;
        Info.SetText(Text);
        ConsoleLobbyWnd()->DialogBox->SetInfo(&Info);
        ConsoleLobbyWnd()->DialogBox->ShowWindow();
        ConsoleLobbyWnd()->CreateButton->EnableWindow(0);
        ConsoleLobbyWnd()->DeleteButton->EnableWindow(0);
        ConsoleLobbyWnd()->StartButton->EnableWindow(0);
    }
    else if (Characters[Selected].DeleteDays == 0)
    {
        GConsole->SelectedCharacterNum(Selected);
        ConsoleLobbyWnd()->CreateButton->EnableWindow(1);
        ConsoleLobbyWnd()->DeleteButton->EnableWindow(1);
        ConsoleLobbyWnd()->StartButton->EnableWindow(1);
    }
    return 1;
}
NW_HOOK_METHOD(0x10069c10, NCPawnSelectWnd, OnSelect)

// 0x10068e00 (vslot 64) - the restore question answered: nothing stays selected
INT NCPawnSelectWnd::OnRestoreAnswer(INT bYes, INT Unused)
{
    if (bYes && Selected >= 0)
        GConsole->RequestCharacterRestore(Selected);
    GConsole->SelectedCharacterNum(-1);
    NameCombo->SetSelected(-1);
    Selected = -1;
    ConsoleLobbyWnd()->CreateButton->EnableWindow(1);
    ConsoleLobbyWnd()->DeleteButton->EnableWindow(1);
    ConsoleLobbyWnd()->StartButton->EnableWindow(1);
    return 1;
}
NW_HOOK_METHOD(0x10068e00, NCPawnSelectWnd, OnRestoreAnswer)

// 0x10068c00 - one character from the server (NCLobbyWnd::AddCharacter): its actor is spawned
// facing the camera and dressed, and the engine's stage gets the record. The karma is copied
// again after SetPawnResource, as in the stock code.
INT NCPawnSelectWnd::AddCharacter(User Info)
{
    if (NumCharacters >= 7)
        return 0;
    Characters[NumCharacters] = Info;
    Characters[NumCharacters].Pawn =
        (APawn*)SpawnActor(Characters[NumCharacters].GetClassNamePointer());
    Characters[NumCharacters].Pawn->Rotation.Yaw = 0x7FFF;
    Characters[NumCharacters].SetPawnResource();
    NameCombo->AddItem(Info.GetName());
    Characters[NumCharacters].Karma = Info.Karma;
    GConsole->InsertSelectCharacterInfo(NumCharacters, Characters[NumCharacters]);
    NumCharacters++;
    return 1;
}
NW_HOOK_METHOD(0x10068c00, NCPawnSelectWnd, AddCharacter)

// 0x10068d50
INT NCPawnSelectWnd::GetSelected()
{
    return Selected;
}
NW_HOOK_METHOD(0x10068d50, NCPawnSelectWnd, GetSelected)

// 0x10068d60
TCHAR* NCPawnSelectWnd::GetCharacterName(INT Index)
{
    return Characters[Index].GetName();
}
NW_HOOK_METHOD(0x10068d60, NCPawnSelectWnd, GetCharacterName)

// 0x10068d80 - the actors go, the stage forgets the characters
void NCPawnSelectWnd::ClearCharacters()
{
    for (INT i = 0; i < NumCharacters; i++)
    {
        if (Characters[i].Pawn)
        {
            Characters[i].Pawn->Destroy();
            Characters[i].Pawn = NULL;
        }
    }
    NumCharacters = 0;
    Selected = -1;
    NameCombo->ClearItems();
    GConsole->SelectedCharacterNum(-1);
    GConsole->ResetSelectCharacterInfo();
}
NW_HOOK_METHOD(0x10068d80, NCPawnSelectWnd, ClearCharacters)

NW_CHECK_VTABLE(NCPawnSelectWnd, 0x100e5a88, 65)
