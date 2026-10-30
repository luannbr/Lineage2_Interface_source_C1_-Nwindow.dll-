/*=============================================================================
    NCLoadingWnd.cpp - the loading screen. Stock: 0x10068910 .. 0x10068b5d (WindowProc
    and OnCreate are bodies the linker folded with other classes).
=============================================================================*/
#include "nwindow/NCLoadingWnd.h"
#include "nwindow/NCGDevice.h"

// 0x10068a20
NCLoadingWnd::NCLoadingWnd()
{
    LoadingTex = NULL;
}
NW_HOOK_CTOR(0x10068a20, NCLoadingWnd)

// 0x10068a70
NW_HOOK_DTOR(0x10068a70, NCLoadingWnd)

// stock body shared with NControl's (0x10004e90)
INT NCLoadingWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10068ac0 (vslot 50) - the clip goes on before BeginPaint and comes off after EndPaint
INT NCLoadingWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (LoadingTex)
        Device->DrawStretchedTex(0, 0, Width, Height, 0, 0, 1024, 768, LoadingTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10068ac0, NCLoadingWnd, OnPaint)

// 0x10068910 - ScreenID is NConsoleWnd +0x3728, set when a server is chosen from its age
// rating (NCServerSelectWnd::OnOK): 10001 all ages, 10002 15+, 10003 18+. Only the Korean
// client uses it; the others read "<country>_LoadingTexture" from FontInfo.ini.
void NCLoadingWnd::SetLoadingTexture(INT ScreenID)
{
    FString Name;
    if (GLanguageType == 0)
    {
        switch (ScreenID)
        {
        case 10001:
            LoadingTex = FindTexture(TEXT("L2Font.loading02-k"));
            break;
        case 10002:
            LoadingTex = FindTexture(TEXT("L2Font.loading03-k"));
            break;
        case 10003:
            LoadingTex = FindTexture(TEXT("L2Font.loading04-k"));
            break;
        default:
            LoadingTex = FindTexture(TEXT("L2Font.loading02-k"));
            break;
        }
    }
    else
    {
        TCHAR Key[256];
        appSprintf(Key, TEXT("%s_LoadingTexture"), GCountryName);
        if (!GConfig->GetString(GCountryName, Key, Name, TEXT("FontInfo.ini")))
            Name = TEXT("L2Font.loading02-k");
        LoadingTex = FindTexture(*Name);
    }
}
NW_HOOK_METHOD(0x10068910, NCLoadingWnd, SetLoadingTexture)

NW_CHECK_VTABLE(NCLoadingWnd, 0x100e5828, 62)
