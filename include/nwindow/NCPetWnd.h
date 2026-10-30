#pragma once
// NCPetWnd - the pet window (UClass "CPetWnd", 0x184; opened from the pet's status bar or the
// action window): name and level, HP / MP / food / exp / weight bars, the Name button (a pet
// can be named once) and three pages under tabs: Information (NCPetDetailStatusWnd, the combat
// values), Action (NCPetActionWnd, the pet's actions) and Items (NCPetInventoryWnd).
// Everything is read again from the network handler at each paint.
//
// Desktop (NCConsole) +0x104 is this window.

#include "nwindow/NCWnd.h"

class NCActionItemWnd;
class NCButton;
class NCPetInventoryWnd;
class NCTabControl;
struct NItemInfo;

// NWM_COMMAND ids (names ours; the summon window uses the same tab id)
enum
{
    NID_PET_TABS = 0xFE001B,
    NID_PET_FRAME = 0xFE001C,
    NID_PET_NAME = 0xFE001D,          // the Name button
};

// The Information page (UClass "CPetDetailStatusWnd", 0x14C): P.Atk. ... Casting Spd.
class NCPetDetailStatusWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPetDetailStatusWnd, NCWnd, 0, 0x10144028)

    UTexture* BackTex;               // +0x94 "L2UI.PetWnd.Pet02"
    PetInfo Pet;                     // +0x98 copied by each paint

    NCPetDetailStatusWnd();                                          // 0x10090ee0
    // ~NCPetDetailStatusWnd                                         // 0x10090f40

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x1008ee70
    virtual INT OnDestroy();                                         // 36 0x10083930 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10090f90
};
NW_ASSERT_OFFSET(NCPetDetailStatusWnd, Pet, 0x98);
NW_ASSERT_SIZE(NCPetDetailStatusWnd, 0x14C);

// The Action page (UClass "CPetActionWnd", 0x9C): the pet actions of actionname-*.dat
// (category 4) in an icon grid.
class NCPetActionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPetActionWnd, NCWnd, 0, 0x101459e8)

    UTexture* BackTex;               // +0x94 "L2UI.PetWnd.pet03"
    NCActionItemWnd* Items;          // +0x98

    NCPetActionWnd();                                                // 0x100915e0
    // ~NCPetActionWnd                                               // 0x10091630

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x10093940
    virtual INT OnDestroy();                                         // 36 0x100bb540
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10091680

    void LoadActions();                                              // 0x10091700 (name ours)
};
NW_ASSERT_OFFSET(NCPetActionWnd, Items, 0x98);
NW_ASSERT_SIZE(NCPetActionWnd, 0x9C);

class NCPetWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPetWnd, NCWnd, 0, 0x10145f10)

    UTexture* BackTex;               // +0x94 "L2UI.PetWnd.PetBack"
    UTexture* HPTex;                 // +0x98 "L2UI.StatusWnd.statusHP1"
    UTexture* MPTex;                 // +0x9C "L2UI.StatusWnd.statusMP1"
    UTexture* EXPTex;                // +0xA0 "L2UI.StatusWnd.statusEXP1"
    UTexture* WeightTex[4];          // +0xA4 "L2UI.StatusWnd.statusWt1" .. 4 (as the status
                                     //   window: up to 50%, 66.66%, 80%, above)
    UTexture* FedTex;                // +0xB4 "L2UI.PetWnd.statusEat1"
    PetInfo Pet;                     // +0xB8 copied by each paint
    NCPetDetailStatusWnd* DetailWnd; // +0x16C
    NCPetActionWnd* ActionWnd;       // +0x170
    NCPetInventoryWnd* InventoryWnd; // +0x174
    NCTabControl* TabCtrl;           // +0x178
    NCButton* NameButton;            // +0x17C
    UBOOL bCanName;                  // +0x180 (1) the console clears it (SetCanName)

    NCPetWnd();                                                      // 0x10091fb0
    // ~NCPetWnd                                                     // 0x10092030

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10090e10
    virtual INT OnCreate();                                          // 35 0x10092080
    virtual INT OnDestroy();                                         // 36 0x10092530
    virtual void ShowWindow();                                       // 42 0x10092d10
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10092620
    virtual INT OnName();                                            // 62 0x10092d90 (name ours)

    // names ours (the console)
    void SetCanName(UBOOL bCan);                                     // 0x10090e80
    void ClearInventory();                                           // 0x10090ea0
    UBOOL AddInventoryItem(NItemInfo* Info);                         // 0x100940b0
    UBOOL UpdateInventoryItem(INT Op, NItemInfo* Info);              // 0x100940d0
};
NW_ASSERT_OFFSET(NCPetWnd, WeightTex, 0xA4);
NW_ASSERT_OFFSET(NCPetWnd, Pet, 0xB8);
NW_ASSERT_OFFSET(NCPetWnd, DetailWnd, 0x16C);
NW_ASSERT_OFFSET(NCPetWnd, bCanName, 0x180);
NW_ASSERT_SIZE(NCPetWnd, 0x184);
