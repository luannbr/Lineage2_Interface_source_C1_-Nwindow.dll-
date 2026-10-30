#pragma once
// NCHtmlComboBox - the <combobox> of the HTML pages (UClass "CHtmlComboBox", 0x200): an
// NCComboBox whose drop button is an NCHtmlButton (kept in its own field) and that paints
// clipped to the viewer while closed. Its items come from a ';'-separated list; its value is
// the chosen item.

#include "nwindow/NCComboBox.h"

class NCHtmlButton;

class NCHtmlComboBox : public NCComboBox
{
    NW_DECLARE_CLASS(NCHtmlComboBox, NCComboBox, 0, 0x10127f30)

    INT Unknown1F8;                  // +0x1F8
    NCHtmlButton* HtmlDropButton;    // +0x1FC (NCComboBox::DropButton stays NULL)

    NCHtmlComboBox();                                                // 0x10028960
    // ~NCHtmlComboBox                                               // 0x100289b0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1002fcf0
    virtual INT OnCreate();                                          // 35 0x10033300
    virtual INT OnDestroy();                                         // 36 0x100334a0
    virtual void ShowWindow();                                       // 42 0x100a5d70 (shared)
    virtual void HideWindow();                                       // 43 0x1003dab0 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10030240
    virtual const TCHAR* GetValue();                                 // 62 0x10030230

    INT SetList(TCHAR* List, TCHAR* Sel);                            // 0x1002fd80 (name ours)
};
NW_ASSERT_OFFSET(NCHtmlComboBox, HtmlDropButton, 0x1FC);
NW_ASSERT_SIZE(NCHtmlComboBox, 0x200);
