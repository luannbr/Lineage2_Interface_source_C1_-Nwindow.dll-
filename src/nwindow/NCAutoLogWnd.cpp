/*=============================================================================
    NCAutoLogWnd.cpp - the self-login screen. Stock: 0x10037360 .. 0x1003762d; WindowProc
    and OnDestroy are bodies shared with other classes.
=============================================================================*/
#include "nwindow/NCAutoLogWnd.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x10037440
NCAutoLogWnd::NCAutoLogWnd()
{
    BackTex = NULL;
    bLoginSent = 0;
    Unknown9C = 0;
    Step = 0;
}
NW_HOOK_CTOR(0x10037440, NCAutoLogWnd)

// 0x100374a0
NW_HOOK_DTOR(0x100374a0, NCAutoLogWnd)

// stock body shared with NControl's (0x10004e90)
INT NCAutoLogWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10037360 (vslot 35) - the text's place (computed unsigned) and AutoLog.txt; NCWnd's
// OnCreate is not called
INT NCAutoLogWnd::OnCreate()
{
    BackTex = FindTexture(TEXT("NWindow.BlackTexture"));
    TextX = (DWORD)Width * 4 / 10;
    TextY = (DWORD)Height / 4;
    if (!GConfig->GetString(TEXT("AutoLogOn"), TEXT("L2ID"), GAutoLogID, 64, TEXT("AutoLog.txt")))
        appSprintf(GAutoLogID, TEXT("zodiac"));
    if (!GConfig->GetString(TEXT("AutoLogOn"), TEXT("L2Passwd"), GAutoLogPassword, 64,
                            TEXT("AutoLog.txt")))
        appSprintf(GAutoLogPassword, TEXT("zodiac"));
    if (!GConfig->GetInt(TEXT("AutoLogOn"), TEXT("L2Slot"), GAutoLogSlot, TEXT("AutoLog.txt")))
        GAutoLogSlot = 0;
    return 1;
}
NW_HOOK_METHOD(0x10037360, NCAutoLogWnd, OnCreate)

// stock body shared with NCCompositeBox's (0x10083930)
INT NCAutoLogWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    return 1;
}

// 0x100374f0 (vslot 50) - the progress lines 12 px apart, then the login, once it went out.
// The clip goes on before the origin.
INT NCAutoLogWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawStretchedTex(0, 0, Width, Height, BackTex);
    if (Step < 3)
    {
        for (INT i = 0; i < Step + 1; i++)
            Device->DrawText(TextX, TextY + 12 * i, 0xFFFFFFFF, GAutoLogMessages[i], 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else if (Step == 3)
    {
        Device->DrawText(TextX, TextY, 0xFFFFFFFF, GAutoLogMessages[3], 0, 0, 0, L2FONT_Normal,
                         0, 0, 0, 0, 0, 0);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    if (!bLoginSent && GConsole->RequestAuthLogin(GAutoLogID, GAutoLogPassword))
        bLoginSent = 1;
    return 1;
}
NW_HOOK_METHOD(0x100374f0, NCAutoLogWnd, OnPaint)

NW_CHECK_VTABLE(NCAutoLogWnd, 0x100dffe0, 62)
