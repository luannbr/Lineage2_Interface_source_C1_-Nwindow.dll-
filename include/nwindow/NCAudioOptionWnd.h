#pragma once
// NCAudioOptionWnd - the audio page of the options window (UClass "CAudioOptionWnd", 0xA8):
// sound effect and music volume, each a combo box of six levels ("5" .. "1", Off = sysstring
// 228). Without an audio subsystem both boxes are disabled. Apply sets the volumes and saves
// them (Option.ini [Audio] SoundVolume / MusicVolume); music turned back on from Off restarts.

#include "nwindow/NCWnd.h"

class NCComboBox;

// NWM_COMMAND ids of the boxes
enum
{
    NID_AUDIO_SOUND = 0x1F12,
    NID_AUDIO_MUSIC = 0x1F16,
};

class NCAudioOptionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCAudioOptionWnd, NCWnd, 0, 0x1013f820)

    NCComboBox* SoundCombo;                // +0x94 level 0 (1.0) .. 5 (Off)
    NCComboBox* MusicCombo;                // +0x98
    NCWnd* Unused9C[3];                    // +0x9C destroyed, never created

    NCAudioOptionWnd();                                              // 0x10080210
    // ~NCAudioOptionWnd                                             // 0x10080270

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x100802c0
    virtual INT OnDestroy();                                         // 36 0x10080580
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10080410

    // names ours
    void InitVolumes();                                              // 0x1007dca0
    void SelectVolumes();                                            // 0x1007da00 current levels
    void Apply();                                                    // 0x100806a0
};
NW_ASSERT_OFFSET(NCAudioOptionWnd, SoundCombo, 0x94);
NW_ASSERT_OFFSET(NCAudioOptionWnd, Unused9C, 0x9C);
NW_ASSERT_SIZE(NCAudioOptionWnd, 0xA8);
