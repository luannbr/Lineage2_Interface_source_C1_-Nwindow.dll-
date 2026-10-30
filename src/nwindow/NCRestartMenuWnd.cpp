/*=============================================================================
    NCRestartMenuWnd.cpp - the restart menu of a dead character. Stock: 0x100767c0 ..
    0x10076beb, 0x10076f00 .. 0x1007741f, 0x10077900 .. 0x100779ee (the system menu shares
    OnDestroy).
=============================================================================*/
#include "nwindow/NCRestartMenuWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x10076f00 - Buttons[RESTART_Fixed] is left as allocated
NCRestartMenuWnd::NCRestartMenuWnd()
{
    BackTex = NULL;
    Buttons[RESTART_Village] = NULL;
    Buttons[RESTART_Hideaway] = NULL;
    Buttons[RESTART_Castle] = NULL;
    Buttons[RESTART_SiegeHQ] = NULL;
    for (INT i = 0; i < RESTART_MAX; i++)
        bAvailable[i] = 1;
}
NW_HOOK_CTOR(0x10076f00, NCRestartMenuWnd)

// 0x10076f90
NW_HOOK_DTOR(0x10076f90, NCRestartMenuWnd)

// 0x100767c0 (vslot 24)
INT NCRestartMenuWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_RESTART_VILLAGE)
            return OnVillage() != 0;
        if (wParam == NID_RESTART_HIDEAWAY)
            return OnHideaway() != 0;
        if (wParam == NID_RESTART_CASTLE)
            return OnCastle() != 0;
        if (wParam == NID_RESTART_SIEGEHQ)
            return OnSiegeHQ() != 0;
        if (wParam == NID_RESTART_FIXED)
            return OnFixed() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100767c0, NCRestartMenuWnd, WindowProc)

// one button of the menu (repeated inline in the stock OnCreate)
static FORCEINLINE NCButton* CreateRestartButton(NCWnd* Parent, INT ID, INT Y, INT TextID)
{
    NCButton* Button = new NCButton(TEXT("L2UI.ActionWnd.PledgeButten1"),
                                    TEXT("L2UI.ActionWnd.PledgeButten2"), 0);
    Button->ID = ID;
    Button->SetWindowRect(22, Y, 99, 19);
    Parent->CreateChild(Button, NWS_VISIBLE);
    Button->SetTextID(TextID);
    return Button;
}

// 0x10076fe0 (vslot 35)
INT NCRestartMenuWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.EtcWnd.RestartBack"));
    Buttons[RESTART_Village] = CreateRestartButton(this, NID_RESTART_VILLAGE, 6, 0x174);
    Buttons[RESTART_Hideaway] = CreateRestartButton(this, NID_RESTART_HIDEAWAY, 30, 0x176);
    Buttons[RESTART_Castle] = CreateRestartButton(this, NID_RESTART_CASTLE, 54, 0x177);
    Buttons[RESTART_SiegeHQ] = CreateRestartButton(this, NID_RESTART_SIEGEHQ, 78, 0x175);
    Buttons[RESTART_Fixed] = CreateRestartButton(this, NID_RESTART_FIXED, 102, 0x1D1);
    return 1;
}
NW_HOOK_METHOD(0x10076fe0, NCRestartMenuWnd, OnCreate)

// 0x10077900 (vslot 36) - also the system menu's (its Buttons[4] is never created)
INT NCRestartMenuWnd::OnDestroy()
{
    for (INT i = 0; i < RESTART_MAX; i++)
    {
        if (Buttons[i])
        {
            RemoveChildWindow(Buttons[i]->Entry);
            delete Buttons[i];
            Buttons[i] = NULL;
        }
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10077900, NCRestartMenuWnd, OnDestroy)

// 0x10077410 (vslot 42)
void NCRestartMenuWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    HideUnavailable();
}
NW_HOOK_METHOD(0x10077410, NCRestartMenuWnd, ShowWindow)

// 0x10077380 (vslot 50) - the clip is set before the origin, as in the stock code
INT NCRestartMenuWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, 141, 131, 0, 0, 141, 131, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10077380, NCRestartMenuWnd, OnPaint)

// the choice goes to the server and the menu closes (repeated in each button's slot)
static FORCEINLINE void SendRestartPoint(NCWnd* Menu, INT Point)
{
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)Point);
    GConsole->RequestRestartPoint(Params);
    Menu->HideWindow();
}

// 0x10076970 (vslot 62)
INT NCRestartMenuWnd::OnVillage()
{
    SendRestartPoint(this, RESTART_Village);
    return 1;
}
NW_HOOK_METHOD(0x10076970, NCRestartMenuWnd, OnVillage)

// 0x100769f0 (vslot 63)
INT NCRestartMenuWnd::OnHideaway()
{
    SendRestartPoint(this, RESTART_Hideaway);
    return 1;
}
NW_HOOK_METHOD(0x100769f0, NCRestartMenuWnd, OnHideaway)

// 0x10076a70 (vslot 64)
INT NCRestartMenuWnd::OnCastle()
{
    SendRestartPoint(this, RESTART_Castle);
    return 1;
}
NW_HOOK_METHOD(0x10076a70, NCRestartMenuWnd, OnCastle)

// 0x10076af0 (vslot 65)
INT NCRestartMenuWnd::OnSiegeHQ()
{
    SendRestartPoint(this, RESTART_SiegeHQ);
    return 1;
}
NW_HOOK_METHOD(0x10076af0, NCRestartMenuWnd, OnSiegeHQ)

// 0x10076b70 (vslot 66)
INT NCRestartMenuWnd::OnFixed()
{
    SendRestartPoint(this, RESTART_Fixed);
    return 1;
}
NW_HOOK_METHOD(0x10076b70, NCRestartMenuWnd, OnFixed)

// 0x100768b0 - kept out of line: ShowWindow calls it, as in the stock code
__declspec(noinline) void NCRestartMenuWnd::HideUnavailable()
{
    for (INT i = 0; i < RESTART_MAX; i++)
    {
        if (!bAvailable[i])
            Buttons[i]->HideWindow();
    }
}
NW_HOOK_METHOD(0x100768b0, NCRestartMenuWnd, HideUnavailable)

// 0x10076930
void NCRestartMenuWnd::SetRestartPoints(UBOOL bVillage, UBOOL bHideaway, UBOOL bCastle,
                                        UBOOL bSiegeHQ, UBOOL bFixed)
{
    bAvailable[RESTART_Village] = bVillage;
    bAvailable[RESTART_Hideaway] = bHideaway;
    bAvailable[RESTART_Castle] = bCastle;
    bAvailable[RESTART_SiegeHQ] = bSiegeHQ;
    bAvailable[RESTART_Fixed] = bFixed;
}
NW_HOOK_METHOD(0x10076930, NCRestartMenuWnd, SetRestartPoints)

NW_CHECK_VTABLE(NCRestartMenuWnd, 0x100e7ad0, 67)
