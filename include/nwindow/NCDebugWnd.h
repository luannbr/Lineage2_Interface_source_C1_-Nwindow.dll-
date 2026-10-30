#pragma once
// NCDebugWnd - the debug log list (UClass "CDebugWnd", 0x1E8), fed by NCObject::Debugf.
// Lines are prefixed with their category; the categories come from .\L2Debug.ini
// ([DebugInfo] count, then Library%d: name%d, color%d = R=,G=,B=, IsEnable%d).

#include "nwindow/NCListBox.h"

// one debug category (names ours)
struct NDebugCategory
{
    TCHAR Name[32];            // +0x00 (entry 0: "NWindow")
    UBOOL bEnabled;            // +0x40
    DWORD Color;               // +0x44
};
NW_ASSERT_SIZE(NDebugCategory, 0x48);

#if NW_HOOK_MODE
#define GDebugCategories (reinterpret_cast<NDebugCategory*>(::nw::Addr(0x1010f4f8)))
#define GNumDebugCategories NW_GLOBAL(INT, 0x1012dcf0)
#else
extern NDebugCategory GDebugCategories[];   // 20 in the stock image
extern INT GNumDebugCategories;
#endif

class NCDebugWnd : public NCListBox
{
    NW_DECLARE_CLASS(NCDebugWnd, NCListBox, 0, 0x1012dcf8)

    NCDebugWnd();                                                    // 0x10057a30 (50, 10)
    NCDebugWnd(INT InMaxLines, INT InVisibleLines);                  // 0x10057ad0
    // ~NCDebugWnd                                                   // 0x10057a80

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100576d0
    virtual INT OnCreate();                                          // 35 0x10057710
    virtual INT OnDestroy();                                         // 36 0x10057990
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10057720

    // names ours
    void SetBackTexture(const TCHAR* Name);                          // 0x10057740
    void AddLog(INT Category, const TCHAR* Text, INT Unused);        // 0x10057770
    void LoadCategories();                                           // 0x100577f0
};
NW_ASSERT_SIZE(NCDebugWnd, 0x1E8);
