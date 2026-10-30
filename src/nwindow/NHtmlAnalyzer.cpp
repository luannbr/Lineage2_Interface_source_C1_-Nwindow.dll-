/*=============================================================================
    NHtmlAnalyzer.cpp - an empty scroll window. Stock: 0x100240a0 .. 0x1002428d; WindowProc,
    OnCreate and OnDestroy are bodies shared with other classes.
=============================================================================*/
#include "nwindow/NHtmlAnalyzer.h"

// 0x10024120 - the deques' constructors are inlined
NHtmlAnalyzer::NHtmlAnalyzer()
{
    UnknownCC = 0;
    UnknownD0 = 0;
    UnknownD4 = 0;
    Unknown100 = 0;
    Unknown104 = 0;
    Unknown108 = 5;
}
NW_HOOK_CTOR(0x10024120, NHtmlAnalyzer)

// 0x100241f0
NW_HOOK_DTOR(0x100241f0, NHtmlAnalyzer)

// stock body shared with NCItemWnd's (0x1006bb90)
INT NHtmlAnalyzer::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCScrollWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// stock body shared with NCNoticeInfoWnd's (0x10079480)
INT NHtmlAnalyzer::OnCreate()
{
    NCScrollWnd::OnCreate();
    return 1;
}

// stock body shared with NCNoticeInfoWnd's (0x10079490)
INT NHtmlAnalyzer::OnDestroy()
{
    NCScrollWnd::OnDestroy();
    return 1;
}

// 0x100240a0 (vslot 50)
INT NHtmlAnalyzer::OnPaint(NCGDevice* Device)
{
    guard(NHtmlAnalyzer::OnPaint);
    NCScrollWnd::OnPaint(Device);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100240a0, NHtmlAnalyzer, OnPaint)

NW_CHECK_VTABLE(NHtmlAnalyzer, 0x100dd698, 65)
