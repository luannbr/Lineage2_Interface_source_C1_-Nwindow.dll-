#pragma once
// NHtmlAnalyzer - an empty scroll window (UClass "HtmlAnalyzer", 0x140): four point deques
// (like NCGDevice's origin stack) and a few fields no code of NWindow reads. Its paint only
// runs the base one; nothing in NWindow creates it.

#include "nwindow/NCScrollWnd.h"
#include "nwindow/NStdLib.h"

class NHtmlAnalyzer : public NCScrollWnd
{
    NW_DECLARE_CLASS(NHtmlAnalyzer, NCScrollWnd, 0, 0x10124288)

    INT UnknownCC;                   // +0xCC (0)
    INT UnknownD0;                   // +0xD0 (0)
    INT UnknownD4;                   // +0xD4 (0)
    NPointDeque Points1;             // +0xD8
    NPointDeque Points2;             // +0xEC
    INT Unknown100;                  // +0x100 (0)
    INT Unknown104;                  // +0x104 (0)
    INT Unknown108;                  // +0x108 (5)
    NPointDeque Points3;             // +0x10C
    NPointDeque Points4;             // +0x120
    INT Unknown134[3];               // +0x134 never set

    NHtmlAnalyzer();                                                 // 0x10024120
    // ~NHtmlAnalyzer                                                // 0x100241f0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1006bb90 (shared)
    virtual INT OnCreate();                                          // 35 0x10079480 (shared)
    virtual INT OnDestroy();                                         // 36 0x10079490 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100240a0
};
NW_ASSERT_OFFSET(NHtmlAnalyzer, Points1, 0xD8);
NW_ASSERT_OFFSET(NHtmlAnalyzer, Unknown108, 0x108);
NW_ASSERT_OFFSET(NHtmlAnalyzer, Points4, 0x120);
NW_ASSERT_SIZE(NHtmlAnalyzer, 0x140);
