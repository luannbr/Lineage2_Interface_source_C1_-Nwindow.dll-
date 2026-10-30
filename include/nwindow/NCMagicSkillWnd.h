#pragma once
// NCMagicSkillWnd - the skill window (UClass "CMagicSkillWnd", 0x9C; a page of the character
// window, NCMainWnd): the NCSkillWnd lists on "L2UI.SkillWnd.skillBack". Shown, it empties
// the lists and asks the server for the skill list; hidden, it empties them.

#include "nwindow/NCWnd.h"

class NCSkillWnd;
class NCGMSkillWnd;

// NWM_COMMAND ids: the skill list (both windows), the GM window's title bar
enum { NID_MAGICSKILL_LIST = 0x1F01, NID_GMMAGICSKILL_FRAME = 0xF0F117 };

class NCMagicSkillWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMagicSkillWnd, NCWnd, 0, 0x10139120)

    UTexture* BackTex;                     // +0x94 "L2UI.SkillWnd.skillBack"
    NCSkillWnd* SkillWnd;                  // +0x98

    NCMagicSkillWnd();                                               // 0x100732f0
    // ~NCMagicSkillWnd                                              // 0x10073340

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x10073390
    virtual INT OnDestroy();                                         // 36 0x100bb540 (NCPetActionWnd's)
    virtual void ShowWindow();                                       // 42 0x100734f0
    virtual void HideWindow();                                       // 43 0x10073570
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10073460

    // names ours
    void ClearSkills();                                              // 0x100729a0
    void ReloadSkillTexts();                                         // 0x10073580 new language
};
NW_ASSERT_OFFSET(NCMagicSkillWnd, BackTex, 0x94);
NW_ASSERT_OFFSET(NCMagicSkillWnd, SkillWnd, 0x98);
NW_ASSERT_SIZE(NCMagicSkillWnd, 0x9C);

// NCGMMagicSkillWnd - a GM's view of another player's skills (UClass "CGMMagicSkillWnd", 0x9C;
// desktop window): an NCGMSkillWnd under a title bar, on the same background. Hidden, it
// empties the lists; the console fills them.
class NCGMMagicSkillWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGMMagicSkillWnd, NCWnd, 0, 0x10130b38)

    NCGMSkillWnd* SkillWnd;                // +0x94
    UTexture* BackTex;                     // +0x98 "L2UI.SkillWnd.skillBack"

    NCGMMagicSkillWnd();                                             // 0x1005cb10
    // ~NCGMMagicSkillWnd                                            // 0x1005cb60

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x1005cbb0
    virtual INT OnDestroy();                                         // 36 0x100bd0e0
    virtual void ShowWindow();                                       // 42 0x100a5d70 (folded)
    virtual void HideWindow();                                       // 43 0x1005cda0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1005cd10

    // names ours
    void ClearSkills();                                              // 0x1005be40
};
NW_ASSERT_OFFSET(NCGMMagicSkillWnd, SkillWnd, 0x94);
NW_ASSERT_OFFSET(NCGMMagicSkillWnd, BackTex, 0x98);
NW_ASSERT_SIZE(NCGMMagicSkillWnd, 0x9C);
