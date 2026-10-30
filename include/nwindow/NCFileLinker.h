#pragma once
// NCFileLinker - an <a link> of the HTML viewer (UClass "CFileLinker", 0x1A4): an invisible
// control over the text run whose click hands the page to load to the viewer (NWM_COMMAND,
// lParam = File).

#include "nwindow/NControl.h"

class NCFileLinker : public NControl
{
    NW_DECLARE_CLASS(NCFileLinker, NControl, 0, 0x101274f0)

    TCHAR* File;                     // +0x1A0 (name ours) the page, an item's string

    NCFileLinker(TCHAR* InFile = NULL);                                     // 0x10030620
    // ~NCFileLinker                                                 // 0x10030690

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100051f0 (shared)
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x1002fe50 (shared)
};
NW_ASSERT_OFFSET(NCFileLinker, File, 0x1A0);
NW_ASSERT_SIZE(NCFileLinker, 0x1A4);
