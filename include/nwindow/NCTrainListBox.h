#pragma once
// NCTrainListBox - the skill list of the trainer window (UClass "CTrainListBox", 0x1F4). Rows
// are 32 px high plus LineGap (4): the skill icon, its name and, below it, "Lv <level>" and
// "Required SP : <sp>". The rows are NTrainEntry records kept in Entries (NCListBox::Items is
// not used) and the scroll range follows their count. Clicking a row selects it and reports
// its index to the parent (NWM_COMMAND, like NCListBox).

#include "nwindow/NCListBox.h"

// one skill row (names ours; 0xA9C bytes). The window fills one on the stack and the list keeps
// a copy (AddEntry); GetSelectedEntry copies the selected one back.
struct NTrainEntry
{
    TCHAR Name[128];           // +0x000
    TCHAR IconName[128];       // +0x100 texture looked up by AddEntry
    TCHAR Desc[1024];          // +0x200
    TCHAR UnknownA00[64];      // +0xA00
    UTexture* Icon;            // +0xA80 NWindow.BlackTexture when not found
    INT SkillID;               // +0xA84 sent with Level when a row is picked
    INT Level;                 // +0xA88 <= 0: SysString 371 ("Unacquired state")
    INT UnknownA8C;            // +0xA8C never copied
    INT SP;                    // +0xA90 required SP
    INT UnknownA94;            // +0xA94
    INT UnknownA98;            // +0xA98

    NTrainEntry()
    {
        Name[0] = 0;
        IconName[0] = 0;
        Desc[0] = 0;
        UnknownA00[0] = 0;
        Icon = NULL;
        SkillID = -1;
        Level = -1;
        UnknownA8C = -1;
        SP = 0;
        UnknownA94 = 0;
        UnknownA98 = -1;
    }

    // inlined in AddEntry and GetSelectedEntry: empty strings and a NULL icon are not copied
    // (UnknownA8C never is)
    void Copy(const NTrainEntry* Src)
    {
        if (Src && Src->Name[0])
            wcscpy(Name, Src->Name);
        if (Src->IconName && Src->IconName[0])
            wcscpy(IconName, Src->IconName);
        if (Src->Desc && Src->Desc[0])
            wcscpy(Desc, Src->Desc);
        if (Src->UnknownA00 && Src->UnknownA00[0])
            wcscpy(UnknownA00, Src->UnknownA00);
        if (Src->Icon)
            Icon = Src->Icon;
        SkillID = Src->SkillID;
        Level = Src->Level;
        SP = Src->SP;
        UnknownA94 = Src->UnknownA94;
        UnknownA98 = Src->UnknownA98;
    }
};
NW_ASSERT_OFFSET(NTrainEntry, Icon, 0xA80);
NW_ASSERT_OFFSET(NTrainEntry, SP, 0xA90);
NW_ASSERT_SIZE(NTrainEntry, 0xA9C);

class NCTrainListBox : public NCListBox
{
    NW_DECLARE_CLASS(NCTrainListBox, NCListBox, 0, 0x10151338)

    TArray<NTrainEntry*> Entries;    // +0x1E8

    NCTrainListBox();                                                // 0x100bcb40
    NCTrainListBox(INT InMaxLines, INT InVisibleLines, INT Unused);  // 0x100bcc00
    // ~NCTrainListBox                                               // 0x100bcba0

    // the stock bodies of 35, 36, 56, 63, 65 and UpdateEntryScrollBar are shared with
    // NCVIPShopItemListBox (identical code)
    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100576d0 (NCDebugWnd's)
    virtual INT OnCreate();                                          // 35 0x100ba9a0
    virtual INT OnDestroy();                                         // 36 0x1006dda0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100baed0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x1006bad0
    virtual INT HitTest(INT X, INT Y);                               // 63 0x100ba9d0
    virtual void Clear();                                            // 65 0x100bc570

    // names ours
    void UpdateEntryScrollBar();                                     // 0x100baaa0
    void AddEntry(const NTrainEntry* Src);                           // 0x100bc5f0
    UBOOL GetSelectedEntry(NTrainEntry* Out);                        // 0x100bb3a0
};
NW_ASSERT_OFFSET(NCTrainListBox, Entries, 0x1E8);
NW_ASSERT_SIZE(NCTrainListBox, 0x1F4);
