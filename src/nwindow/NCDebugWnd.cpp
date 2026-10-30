/*=============================================================================
    NCDebugWnd.cpp - debug log list. Stock: 0x100576d0 .. 0x10057b0d.
=============================================================================*/
#include "nwindow/NCDebugWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"

// 0x10057a30
NCDebugWnd::NCDebugWnd()
    : NCListBox(50, 10, 16)
{
}
NW_HOOK_CTOR(0x10057a30, NCDebugWnd)

// 0x10057ad0
NCDebugWnd::NCDebugWnd(INT InMaxLines, INT InVisibleLines)
    : NCListBox(InMaxLines, InVisibleLines, 16)
{
}
NW_HOOK_CTOR_ARGS(0x10057ad0, NCDebugWnd, (INT A, INT B), (A, B))

// 0x10057a80
NW_HOOK_DTOR(0x10057a80, NCDebugWnd)

// 0x100576d0 (vslot 24) - also NCChatListBox's (identical body)
INT NCDebugWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCListBox::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x100576d0, NCDebugWnd, WindowProc)

// 0x10057710 (vslot 35)
INT NCDebugWnd::OnCreate()
{
    NCListBox::OnCreate();
    return 1;
}
NW_HOOK_METHOD(0x10057710, NCDebugWnd, OnCreate)

// 0x10057990 (vslot 36) - removes the scroll controls itself, then NCListBox does it again
// (finding them gone)
INT NCDebugWnd::OnDestroy()
{
    if (ScrollUpButton)
    {
        RemoveChildWindow(ScrollUpButton->Entry);
        delete ScrollUpButton;
        ScrollUpButton = NULL;
    }
    if (ScrollDownButton)
    {
        RemoveChildWindow(ScrollDownButton->Entry);
        delete ScrollDownButton;
        ScrollDownButton = NULL;
    }
    if (ScrollBar)
    {
        RemoveChildWindow(ScrollBar->Entry);
        delete ScrollBar;
        ScrollBar = NULL;
    }
    NCListBox::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10057990, NCDebugWnd, OnDestroy)

// 0x10057720 (vslot 50)
INT NCDebugWnd::OnPaint(NCGDevice* Device)
{
    NCListBox::OnPaint(Device);
    return 1;
}
NW_HOOK_METHOD(0x10057720, NCDebugWnd, OnPaint)

// 0x10057740
void NCDebugWnd::SetBackTexture(const TCHAR* Name)
{
    if (Name)
        BackTex = FindTexture(Name);
    else
        BackTex = FindTexture(TEXT("NWindow.BlackTexture"));
}
NW_HOOK_METHOD(0x10057740, NCDebugWnd, SetBackTexture)

// 0x10057770 - "[category]text" in the category color, when the category is enabled
void NCDebugWnd::AddLog(INT Category, const TCHAR* Text, INT Unused)
{
    TCHAR Temp[0x1F74 / sizeof(TCHAR)];
    if (Category < 0 || (GNumDebugCategories > 0 && Category > GNumDebugCategories))
        return;
    NDebugCategory& Cat = GDebugCategories[Category];
    if (Cat.bEnabled)
    {
        appSprintf(Temp, TEXT("[%s]%s"), Cat.Name, Text);
        AddString(Temp, Cat.Color, -1);
    }
}
NW_HOOK_METHOD(0x10057770, NCDebugWnd, AddLog)

// 0x100577f0 - does not use this. The name is copied with the ini value as the format string
// and the color's alpha byte is left unset, as in the stock code.
void NCDebugWnd::LoadCategories()
{
    TCHAR Ini[] = TEXT(".\\L2Debug.ini");
    if (!GConfig->GetInt(TEXT("DebugInfo"), TEXT("count"), GNumDebugCategories, Ini))
        GNumDebugCategories = 0;
    for (INT i = 0; i < GNumDebugCategories; i++)
    {
        TCHAR Section[64], Key[64], Value[1024];
        NDebugCategory& Cat = GDebugCategories[i];
        appSprintf(Section, TEXT("Library%d"), i);
        appSprintf(Key, TEXT("name%d"), i);
        appSprintf(Cat.Name, GConfig->GetStr(Section, Key, Ini));
        appSprintf(Key, TEXT("color%d"), i);
        if (GConfig->GetString(Section, Key, Value, 1024, Ini))
        {
            FColor Color;
            if (!Parse(Value, TEXT("R="), Color.R))
                Color.R = 0xFF;
            if (!Parse(Value, TEXT("G="), Color.G))
                Color.G = 0xFF;
            if (!Parse(Value, TEXT("B="), Color.B))
                Color.B = 0xFF;
            Cat.Color = Color.DWColor();
        }
        appSprintf(Key, TEXT("IsEnable%d"), i);
        if (!GConfig->GetBool(Section, Key, Cat.bEnabled, Ini))
            Cat.bEnabled = 0;
    }
}
NW_HOOK_METHOD(0x100577f0, NCDebugWnd, LoadCategories)

NW_CHECK_VTABLE(NCDebugWnd, 0x100e3028, 70)
