#pragma once
// NCPetitionListBox - the petition conversation list (UClass "CPetitionListBox", 0x1E8). Adds
// messages like the chat list (wrapped to the width, continuation lines indented 12 px), but
// the scroll controls stay on the right, the text starts at the left edge and the visible line
// count is fixed.

#include "nwindow/NCListBox.h"

class NCPetitionListBox : public NCListBox
{
    NW_DECLARE_CLASS(NCPetitionListBox, NCListBox, 0, 0x10146958)

    NCPetitionListBox();                                             // 0x10094e40
    NCPetitionListBox(INT InMaxLines, INT InVisibleLines, INT Unused); // 0x10094ee0
    // ~NCPetitionListBox                                            // 0x10094e90

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100576d0 (NCDebugWnd's)
    virtual INT OnCreate();                                          // 35 0x100947e0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10094860
    virtual void AddString(const TCHAR* Text, DWORD Color, INT Param); // 64 0x10094be0
};
NW_ASSERT_SIZE(NCPetitionListBox, 0x1E8);
