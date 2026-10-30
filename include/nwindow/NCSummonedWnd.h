#pragma once
// NCSummonedWnd - the summon window (UClass "CSummonedWnd", 0x160): the pet window without
// food, exp, weight, name button and items: name and level, HP / MP bars, and the pages
// Information (NCSummonedDetailStatusWnd) and Action (NCSummonedActionWnd, the summon actions
// of actionname-*.dat, category 5). Both pages share their code with the pet's.
//
// Desktop (NCConsole) +0x108 is this window.

#include "nwindow/NCWnd.h"

class NCActionItemWnd;
class NCTabControl;

enum { NID_SUMMON_FRAME = 0xFE001E };  // the tabs use NID_PET_TABS (0xFE001B)

// The Information page (UClass "CSummonedDetailStatusWnd", 0x14C)
class NCSummonedDetailStatusWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSummonedDetailStatusWnd, NCWnd, 0, 0x10144558)

    UTexture* BackTex;               // +0x94 "L2UI.PetWnd.Summon02"
    PetInfo Summon;                  // +0x98 copied by each paint

    NCSummonedDetailStatusWnd();                                     // 0x10092e00
    // ~NCSummonedDetailStatusWnd                                    // 0x10092e60

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x10090eb0
    virtual INT OnDestroy();                                         // 36 0x10083930 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10090f90 (folded)
};
NW_ASSERT_OFFSET(NCSummonedDetailStatusWnd, Summon, 0x98);
NW_ASSERT_SIZE(NCSummonedDetailStatusWnd, 0x14C);

// The Action page (UClass "CSummonedActionWnd", 0x9C)
class NCSummonedActionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSummonedActionWnd, NCWnd, 0, 0x10143b08)

    UTexture* BackTex;               // +0x94 "L2UI.PetWnd.Summon03"
    NCActionItemWnd* Items;          // +0x98

    NCSummonedActionWnd();                                           // 0x10092eb0
    // ~NCSummonedActionWnd                                          // 0x10092f00

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x100940f0
    virtual INT OnDestroy();                                         // 36 0x100bb540 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10091680 (folded)

    void LoadActions();                                              // 0x10092f50 (name ours)
};
NW_ASSERT_OFFSET(NCSummonedActionWnd, Items, 0x98);
NW_ASSERT_SIZE(NCSummonedActionWnd, 0x9C);

class NCSummonedWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSummonedWnd, NCWnd, 0, 0x10144fa0)

    UTexture* BackTex;               // +0x94 "L2UI.PetWnd.Summon01"
    UTexture* HPTex;                 // +0x98 "L2UI.StatusWnd.statusHP1"
    UTexture* MPTex;                 // +0x9C "L2UI.StatusWnd.statusMP1"
    NCSummonedDetailStatusWnd* DetailWnd; // +0xA0
    NCSummonedActionWnd* ActionWnd;  // +0xA4
    NCTabControl* TabCtrl;           // +0xA8
    PetInfo Summon;                  // +0xAC copied by each paint

    NCSummonedWnd();                                                 // 0x10093080
    // ~NCSummonedWnd                                                // 0x100930f0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x10093140
    virtual INT OnDestroy();                                         // 36 0x10093440
    virtual void ShowWindow();                                       // 42 0x10093870
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100934e0
};
NW_ASSERT_OFFSET(NCSummonedWnd, DetailWnd, 0xA0);
NW_ASSERT_OFFSET(NCSummonedWnd, Summon, 0xAC);
NW_ASSERT_SIZE(NCSummonedWnd, 0x160);
