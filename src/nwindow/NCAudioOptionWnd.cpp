/*=============================================================================
    NCAudioOptionWnd.cpp - the audio page of the options. Stock: 0x1007da00 .. 0x1007dd6a,
    0x10080210 .. 0x1008096f (WindowProc is NControl's body).
=============================================================================*/
#include "nwindow/NCAudioOptionWnd.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x10080210 - the two boxes are left as allocated
NCAudioOptionWnd::NCAudioOptionWnd()
{
    Unused9C[0] = NULL;
    Unused9C[1] = NULL;
    Unused9C[2] = NULL;
}
NW_HOOK_CTOR(0x10080210, NCAudioOptionWnd)

// 0x10080270
NW_HOOK_DTOR(0x10080270, NCAudioOptionWnd)

// stock body shared with NControl's (0x10004e90)
INT NCAudioOptionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100802c0 (vslot 35)
INT NCAudioOptionWnd::OnCreate()
{
    NCWnd::OnCreate();
    SoundCombo = new NCComboBox;
    SoundCombo->ID = NID_AUDIO_SOUND;
    SoundCombo->SetWindowRect(83, 3, 112, 18);
    CreateChild(SoundCombo, NWS_VISIBLE);
    MusicCombo = new NCComboBox;
    MusicCombo->ID = NID_AUDIO_MUSIC;
    MusicCombo->SetWindowRect(83, 23, 112, 18);
    CreateChild(MusicCombo, NWS_VISIBLE);
    InitVolumes();
    return 1;
}
NW_HOOK_METHOD(0x100802c0, NCAudioOptionWnd, OnCreate)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x10080580 (vslot 36) - the boxes are enabled again first (no NULL check)
INT NCAudioOptionWnd::OnDestroy()
{
    SoundCombo->EnableWindow(1);
    MusicCombo->EnableWindow(1);
    DestroyChild(this, SoundCombo);
    DestroyChild(this, MusicCombo);
    DestroyChild(this, Unused9C[0]);
    DestroyChild(this, Unused9C[1]);
    DestroyChild(this, Unused9C[2]);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10080580, NCAudioOptionWnd, OnDestroy)

// 0x10080410 (vslot 50) - the labels "SFX Vol." / "Music Vol." (sysstring 216 / 217)
// centered on x 41. The guard names the options window, as in the stock code.
INT NCAudioOptionWnd::OnPaint(NCGDevice* Device)
{
    guard(NCOptionWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[216], 0, 0, 0, 0, 0);
    Device->DrawText(41 - Size.cx / 2, 6, 0xFFFFFFFF, *GL2GameData.SysString[216], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[217], 0, 0, 0, 0, 0);
    Device->DrawText(41 - Size.cx / 2, 26, 0xFFFFFFFF, *GL2GameData.SysString[217], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10080410, NCAudioOptionWnd, OnPaint)

// 0x1007dca0 - kept out of line: OnCreate calls it, as in the stock code
__declspec(noinline) void NCAudioOptionWnd::InitVolumes()
{
    SoundCombo->AddItem(TEXT("5"));
    SoundCombo->AddItem(TEXT("4"));
    SoundCombo->AddItem(TEXT("3"));
    SoundCombo->AddItem(TEXT("2"));
    SoundCombo->AddItem(TEXT("1"));
    SoundCombo->AddItem(0xE4);   // "Off"
    MusicCombo->AddItem(TEXT("5"));
    MusicCombo->AddItem(TEXT("4"));
    MusicCombo->AddItem(TEXT("3"));
    MusicCombo->AddItem(TEXT("2"));
    MusicCombo->AddItem(TEXT("1"));
    MusicCombo->AddItem(0xE4);
    SelectVolumes();
}
NW_HOOK_METHOD(0x1007dca0, NCAudioOptionWnd, InitVolumes)

// the level of a volume: [1, ...) 0, [0.8, 1) 1 ... [0, 0.2) 5 (repeated inline in the
// stock code); a negative volume selects nothing
static FORCEINLINE void SelectLevel(NCComboBox* Combo, FLOAT Volume)
{
    if (0.0f <= Volume && Volume < 0.2f)
        Combo->SetSelected(5);
    else if (0.2f <= Volume && Volume < 0.4f)
        Combo->SetSelected(4);
    else if (0.4f <= Volume && Volume < 0.6f)
        Combo->SetSelected(3);
    else if (0.6f <= Volume && Volume < 0.8f)
        Combo->SetSelected(2);
    else if (0.8f <= Volume && Volume < 1.0f)
        Combo->SetSelected(1);
    else if (1.0f <= Volume)
        Combo->SetSelected(0);
}

// 0x1007da00 - kept out of line: InitVolumes tail-calls it, as in the stock code
__declspec(noinline) void NCAudioOptionWnd::SelectVolumes()
{
    if (GConsole->Engine->Audio)
        SelectLevel(SoundCombo, GConsole->Engine->Audio->GetSoundVolume());
    else
        SoundCombo->EnableWindow(0);
    if (GConsole->Engine->Audio)
        SelectLevel(MusicCombo, GConsole->Engine->Audio->GetMusicVolume());
    else
        MusicCombo->EnableWindow(0);
}
NW_HOOK_METHOD(0x1007da00, NCAudioOptionWnd, SelectVolumes)

// 0x100806a0 - Off also silences what plays (StopSound, StopAllMusic). Music
// back on from Off restarts it: the player controller's ResetMusicControll (reached through
// its pawn), then the lobby music is restored.
void NCAudioOptionWnd::Apply()
{
    TCHAR Buffer[256];
    if (GConsole->Engine->Audio)
    {
        switch (SoundCombo->Selected)
        {
        case 0:
            GConsole->Engine->Audio->SetSoundVolume(1.0f);
            break;
        case 1:
            GConsole->Engine->Audio->SetSoundVolume(0.8f);
            break;
        case 2:
            GConsole->Engine->Audio->SetSoundVolume(0.6f);
            break;
        case 3:
            GConsole->Engine->Audio->SetSoundVolume(0.4f);
            break;
        case 4:
            GConsole->Engine->Audio->SetSoundVolume(0.2f);
            break;
        case 5:
            GConsole->Engine->Audio->SetSoundVolume(0.0f);
            GConsole->Engine->Audio->StopSound(NULL, NULL);
            break;
        }
        appSprintf(Buffer, TEXT("%f"), GConsole->Engine->Audio->GetSoundVolume());
        GConfig->SetString(TEXT("Audio"), TEXT("SoundVolume"), Buffer, TEXT("Option.ini"));
    }
    if (GConsole->Engine->Audio)
    {
        INT Level = MusicCombo->Selected;
        FLOAT OldVolume = GConsole->Engine->Audio->GetMusicVolume();
        switch (Level)
        {
        case 0:
            GConsole->Engine->Audio->SetMusicVolume(1.0f);
            break;
        case 1:
            GConsole->Engine->Audio->SetMusicVolume(0.8f);
            break;
        case 2:
            GConsole->Engine->Audio->SetMusicVolume(0.6f);
            break;
        case 3:
            GConsole->Engine->Audio->SetMusicVolume(0.4f);
            break;
        case 4:
            GConsole->Engine->Audio->SetMusicVolume(0.2f);
            break;
        case 5:
            GConsole->Engine->Audio->SetMusicVolume(0.0f);
            GConsole->Engine->Audio->StopAllMusic(0.0f);
            break;
        }
        if (GConsole->Engine->Audio->GetMusicVolume() > 0.0f && OldVolume == 0.0f)
        {
            APlayerController* Controller = GConsole->Engine->Client->Viewports(0)->Actor;
            if (Controller)
            {
                APawn* Pawn = Controller->Pawn;
                if (Pawn)
                {
                    AController* PawnController = Pawn->Controller;
                    if (PawnController)
                        PawnController->ResetMusicControll();
                }
            }
            GConsole->RestoreLobbyMusic();
        }
        appSprintf(Buffer, TEXT("%f"), GConsole->Engine->Audio->GetMusicVolume());
        GConfig->SetString(TEXT("Audio"), TEXT("MusicVolume"), Buffer, TEXT("Option.ini"));
    }
}
NW_HOOK_METHOD(0x100806a0, NCAudioOptionWnd, Apply)

NW_CHECK_VTABLE(NCAudioOptionWnd, 0x100e98a8, 62)
