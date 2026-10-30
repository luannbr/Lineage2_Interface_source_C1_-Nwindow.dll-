#pragma once
// NCSkillItemWnd - a skill grid of the skill window (UClass "CSkillItemWnd", 0x134; the Active
// and Passive lists of NCSkillWnd, 6 x 7 icons of 32 pixels). Each skill has a cool time icon
// drawn over it (CoolTimes, one per item, same index); the active ones get the icon frame and,
// while pressed, the click frame, and a toggle skill whose effect is on the player the toggle
// animation. Released over the same icon a skill is used (Ctrl forces, Shift stays in place);
// dropped on a shortcut bar slot it is registered there. The tool tip is always the box: name
// and type, "MP cost" and "Range" when the skill has them, description.
// NCGMSkillItemWnd, the GM view of a player's skills, uses nothing on release.

#include "nwindow/NCItemWnd.h"

class NCCoolTimeIcon;

class NCSkillItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCSkillItemWnd, NCItemWnd, 0, 0x101386d0)

    UTexture* IconBackTex;             // +0x118 "l2ui.nwindow.icon_back" frame of an active skill
    UTexture* IconClickTex;            // +0x11C "l2ui.nwindow.icon_click" while pressed
    UTexture* ToggleTex;               // +0x120 "L2_SkillTime.ToggleEffect.ToggleEffect001"
    UBOOL bPressed;                    // +0x124 the left button is down on the grid
    TArray<NCCoolTimeIcon*> CoolTimes; // +0x128 one per item (the console starts them)

    NCSkillItemWnd();                                                // 0x100739a0
    NCSkillItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x10073a40
    // ~NCSkillItemWnd                                               // 0x1005fe80

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10071fc0
    virtual INT OnDestroy();                                         // 36 0x10073770
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100729b0
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 51 0x10072cf0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x10072000
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x10072f60
    virtual void Clear();                                            // 65 0x100737e0
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 67 0x10072360

    // names ours
    INT AddSkill(NItemInfo* Info);                                   // 0x10073890 no limit
    void SetItemToolTip(NItemEntry* Item);                           // 0x10072030 (the box's
                                                                     //   size comes from it)
};
NW_ASSERT_OFFSET(NCSkillItemWnd, IconBackTex, 0x118);
NW_ASSERT_OFFSET(NCSkillItemWnd, bPressed, 0x124);
NW_ASSERT_OFFSET(NCSkillItemWnd, CoolTimes, 0x128);
NW_ASSERT_SIZE(NCSkillItemWnd, 0x134);

// NCGMSkillItemWnd - the grids of NCGMSkillWnd (UClass "CGMSkillItemWnd", 0x134): a click
// only selects.
class NCGMSkillItemWnd : public NCSkillItemWnd
{
    NW_DECLARE_CLASS(NCGMSkillItemWnd, NCSkillItemWnd, 0, 0x10130618)

    NCGMSkillItemWnd();                                              // 0x10060400
    NCGMSkillItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x100604a0
    // ~NCGMSkillItemWnd                                             // 0x10060450

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1005bd80
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x1005bdc0 (folded)
};
NW_ASSERT_SIZE(NCGMSkillItemWnd, 0x134);
