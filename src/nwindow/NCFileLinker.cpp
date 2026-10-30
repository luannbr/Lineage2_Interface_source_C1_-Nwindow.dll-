/*=============================================================================
    NCFileLinker.cpp - the file link of the HTML viewer. Stock: 0x10030620 .. 0x100306df
    (WindowProc and OnLButtonDown are bodies the linker folded with other classes).
=============================================================================*/
#include "nwindow/NCFileLinker.h"

// 0x10030620 - NControl's constructor is inlined
NCFileLinker::NCFileLinker(TCHAR* InFile)
{
    File = InFile;
}
NW_HOOK_CTOR_ARGS(0x10030620, NCFileLinker, (TCHAR* A), (A))

// 0x10030690
NW_HOOK_DTOR(0x10030690, NCFileLinker)

// stock body shared with NCButton's (0x100051f0)
INT NCFileLinker::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}

// stock body shared with NCActionLinker::OnConfirm (0x1002fe50)
INT NCFileLinker::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    NotifyParent((LPARAM)File);
    return 1;
}

NW_CHECK_VTABLE(NCFileLinker, 0x100dede8, 63)
