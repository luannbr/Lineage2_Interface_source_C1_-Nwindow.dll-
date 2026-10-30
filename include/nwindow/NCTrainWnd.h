#pragma once
// NCTrainWnd - the skill trainer window (UClass "CTrainWnd", 0x9C; desktop +0xE8): a title
// bar ("Skill Trainer", sysstring 477) over two pages in the same place, the skill list
// (NCTrainListWnd) and the details of one skill (NCTrainInfoWnd). The console drives it: a new
// list empties both pages and shows the list (vslot 151), each row comes with 153; a row
// picked asks the server for the skill, whose answer (152) fills and shows the info page and
// its required items (154). "Learn" asks the server to learn it, "List" goes back.

#include "nwindow/NCWnd.h"

class NCButton;
class NCTrainListBox;
struct NItemEntry;
struct NItemInfo;
struct NTrainEntry;

// NWM_COMMAND ids of the pages' controls
enum
{
    NID_TRAIN_FRAME = 4,
    NID_TRAIN_LEARN = 2,
    NID_TRAIN_LIST = 3,
    NID_TRAIN_LISTBOX = 0x1A04,
};

// A skill as the info page shows it (names ours; 0x2C0 bytes). The console fills one from the
// server's answer and hands it to SetInfo; the page keeps its own (NCTrainInfoWnd::Info), whose
// Items are its required item entries.
struct NTrainSkillInfo
{
    TCHAR Name[128];           // +0x000
    TCHAR IconName[128];       // +0x100
    FString Desc;              // +0x200
    TCHAR Unknown20C[64];      // +0x20C drawn after the level
    UTexture* Icon;            // +0x28C
    NItemEntry* Items[4];      // +0x290 required items: the page's are made by its OnCreate
                               //   (NItemEntry::UnknownC34 1: shown) and freed here
    INT SkillID;               // +0x2A0 sent with Level by "Learn"
    INT Level;                 // +0x2A4
    INT Unknown2A8;            // +0x2A8
    INT SP;                    // +0x2AC required SP
    INT MpCost;                // +0x2B0
    INT Range;                 // +0x2B4 not shown when negative
    INT Unknown2B8;            // +0x2B8
    INT Unknown2BC;            // +0x2BC

    NTrainSkillInfo();                                               // 0x10047260
    ~NTrainSkillInfo();                                              // 0x1004ddf0 frees Items
};
NW_ASSERT_OFFSET(NTrainSkillInfo, Desc, 0x200);
NW_ASSERT_OFFSET(NTrainSkillInfo, Icon, 0x28C);
NW_ASSERT_OFFSET(NTrainSkillInfo, SkillID, 0x2A0);
NW_ASSERT_OFFSET(NTrainSkillInfo, Unknown2BC, 0x2BC);
NW_ASSERT_SIZE(NTrainSkillInfo, 0x2C0);

// NCTrainListWnd - the list page (UClass "CTrainListWnd", 0x9C): "Skill list" (sysstring 364)
// over the NCTrainListBox, the player's SP at the bottom right.
class NCTrainListWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCTrainListWnd, NCWnd, 0, 0x10150e18)

    UTexture* BackTex;                     // +0x94 "L2UI.SkillTrain.TrainerBack1"
    NCTrainListBox* ListBox;               // +0x98

    NCTrainListWnd();                                                // 0x100bb4a0
    // ~NCTrainListWnd                                               // 0x100bb4f0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100baae0
    virtual INT OnCreate();                                          // 35 0x100bcc50
    virtual INT OnDestroy();                                         // 36 0x100bb540 (NCPetActionWnd's)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100bb580
    // name ours
    virtual INT OnListCommand(LPARAM lParam);                        // 62 0x100bb710 a row picked

    // names ours
    void Clear();                                                    // 0x100bab50
    void AddEntry(const NTrainEntry* Entry);                         // 0x100bc7a0
};
NW_ASSERT_OFFSET(NCTrainListWnd, ListBox, 0x98);
NW_ASSERT_SIZE(NCTrainListWnd, 0x9C);

// NCTrainInfoWnd - the skill page (UClass "CTrainInfoWnd", 0x360): "Skill explanation" (366)
// with the icon, name, level, MP cost, range and description; "Learning condition" (367) with
// the required SP and up to four required items; the player's SP; "Learn" and "List".
class NCTrainInfoWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCTrainInfoWnd, NCWnd, 0, 0x10151d88)

    UTexture* BackTex;                     // +0x94 "L2UI.SkillTrain.TrainerBack3"
    NTrainSkillInfo Info;                  // +0x98 the skill shown
    NCButton* LearnButton;                 // +0x358 "Learn" (sysstring 368)
    NCButton* ListButton;                  // +0x35C "List" (sysstring 369)

    NCTrainInfoWnd();                                                // 0x100bc7b0
    // ~NCTrainInfoWnd                                               // 0x100bc840

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100bab60
    virtual INT OnCreate();                                          // 35 0x100bb810
    virtual INT OnDestroy();                                         // 36 0x100bba20
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100bba90
    // names ours
    virtual INT OnLearn();                                           // 62 0x100bad80
    virtual INT OnList();                                            // 63 0x100bae10

    // names ours
    void SetInfo(NTrainSkillInfo* Src);                              // 0x100babf0
    void Reset();                                                    // 0x100bad10
    void AddRequiredItem(NItemInfo* Item);                           // 0x100bc300 into a free row
};
NW_ASSERT_OFFSET(NCTrainInfoWnd, Info, 0x98);
NW_ASSERT_OFFSET(NCTrainInfoWnd, LearnButton, 0x358);
NW_ASSERT_SIZE(NCTrainInfoWnd, 0x360);

class NCTrainWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCTrainWnd, NCWnd, 0, 0x10151868)

    NCTrainListWnd* ListWnd;               // +0x94
    NCTrainInfoWnd* InfoWnd;               // +0x98

    NCTrainWnd();                                                    // 0x100bc3f0
    // ~NCTrainWnd                                                   // 0x100bc440

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x100bc8a0
    virtual INT OnDestroy();                                         // 36 0x10089310
    virtual void ShowWindow();                                       // 42 0x100a5d70 (folded)
    virtual void HideWindow();                                       // 43 0x1003dab0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100bc490

    // the console's (names ours)
    void Reset();                                                    // 0x100baeb0 both pages
    void ShowList();                                                 // 0x100bae60
    void ShowInfo();                                                 // 0x100bae80
    void SetSkillInfo(NTrainSkillInfo* Info);                        // 0x100baea0
    void AddRequiredItem(NItemInfo* Item);                           // 0x100bc540
    void AddEntry(const NTrainEntry* Entry);                         // 0x100bca90
};
NW_ASSERT_OFFSET(NCTrainWnd, ListWnd, 0x94);
NW_ASSERT_SIZE(NCTrainWnd, 0x9C);
