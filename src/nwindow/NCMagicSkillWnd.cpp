/*=============================================================================
    NCMagicSkillWnd.cpp - the skill window and its GM copy. Stock: 0x100729a0 .. 0x100729aa,
    0x100732f0 .. 0x1007374e (WindowProc is NControl's body, OnDestroy NCPetActionWnd's),
    NCGMMagicSkillWnd's 0x1005be40 .. 0x1005be4a, 0x1005cb10 .. 0x1005cdaf and 0x100bd0e0 ..
    0x100bd11e.
=============================================================================*/
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCSkillWnd.h"
#include "nwindow/NCSkillItemWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <wchar.h>

/*-----------------------------------------------------------------------------
    NCMagicSkillWnd
-----------------------------------------------------------------------------*/

// 0x100732f0 - the fields are left as allocated
NCMagicSkillWnd::NCMagicSkillWnd()
{
}
NW_HOOK_CTOR(0x100732f0, NCMagicSkillWnd)

// 0x10073340
NW_HOOK_DTOR(0x10073340, NCMagicSkillWnd)

// stock body shared with NControl's (0x10004e90)
INT NCMagicSkillWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10073390 (vslot 35)
INT NCMagicSkillWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.SkillWnd.skillBack"));
    SkillWnd = new NCSkillWnd;
    SkillWnd->ID = NID_MAGICSKILL_LIST;
    SkillWnd->SetWindowRect(0, 0, 240, 300);
    CreateChild(SkillWnd, NWS_VISIBLE | 0x20);
    return 1;
}
NW_HOOK_METHOD(0x10073390, NCMagicSkillWnd, OnCreate)

// stock body shared with NCPetActionWnd's (0x100bb540, hooked there; also NCGMActionWnd's,
// NCPetInventoryWnd's, NCSummonedActionWnd's and NCTrainListWnd's); BackTex is left
INT NCMagicSkillWnd::OnDestroy()
{
    if (SkillWnd)
    {
        RemoveChildWindow(SkillWnd->Entry);
        delete SkillWnd;
        SkillWnd = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}

// 0x100734f0 (vslot 42) - only when hidden: the lists empty, the server is asked for them
void NCMagicSkillWnd::ShowWindow()
{
    if (!IsShowWindow())
    {
        ClearSkills();
        L2ParamStack Params(10);
        GConsole->RequestSkillList(Params);
        NCWnd::ShowWindow();
    }
}
NW_HOOK_METHOD(0x100734f0, NCMagicSkillWnd, ShowWindow)

// 0x10073570 (vslot 43)
void NCMagicSkillWnd::HideWindow()
{
    ClearSkills();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x10073570, NCMagicSkillWnd, HideWindow)

// 0x10073460 (vslot 50) - the clip one pixel taller than the window
INT NCMagicSkillWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height + 1);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, 240, 301, 0, 0, 240, 301, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10073460, NCMagicSkillWnd, OnPaint)

// 0x100729a0
void NCMagicSkillWnd::ClearSkills()
{
    SkillWnd->ClearLists();
}
NW_HOOK_METHOD(0x100729a0, NCMagicSkillWnd, ClearSkills)

// a list's skills take the new name and description and make their tool tip again (repeated
// inline in the stock ReloadSkillTexts); the level is NItemEntry +0xC18
static FORCEINLINE void ReloadListTexts(NCSkillItemWnd* List)
{
    for (INT i = 0; i < List->Items.Num(); i++)
    {
        NItemEntry* Item = List->Items(i);
        if (!Item)
            continue;
        if (GConsole->GetSkillName(Item->ID, Item->UnknownC18))
            wcscpy(Item->Name, GConsole->GetSkillName(Item->ID, Item->UnknownC18));
        if (GConsole->GetSkillDesc(Item->ID, Item->UnknownC18))
            wcscpy(Item->Description, GConsole->GetSkillDesc(Item->ID, Item->UnknownC18));
        List->SetItemToolTip(Item);
    }
}

// 0x10073580 - after a language change (NConsoleWnd::ReloadGameTexts)
void NCMagicSkillWnd::ReloadSkillTexts()
{
    ReloadListTexts(SkillWnd->ActiveList);
    ReloadListTexts(SkillWnd->PassiveList);
}
NW_HOOK_METHOD(0x10073580, NCMagicSkillWnd, ReloadSkillTexts)

NW_CHECK_VTABLE(NCMagicSkillWnd, 0x100e72d8, 62)

/*-----------------------------------------------------------------------------
    NCGMMagicSkillWnd
-----------------------------------------------------------------------------*/

// 0x1005cb10 - the fields are left as allocated
NCGMMagicSkillWnd::NCGMMagicSkillWnd()
{
}
NW_HOOK_CTOR(0x1005cb10, NCGMMagicSkillWnd)

// 0x1005cb60
NW_HOOK_DTOR(0x1005cb60, NCGMMagicSkillWnd)

// stock body shared with NControl's (0x10004e90)
INT NCGMMagicSkillWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x1005cbb0 (vslot 35) - the title bar (no text) over the skill list
INT NCGMMagicSkillWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_GMMAGICSKILL_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    SkillWnd = new NCGMSkillWnd;
    SkillWnd->ID = NID_MAGICSKILL_LIST;
    SkillWnd->SetWindowRect(0, 21, 240, 300);
    CreateChild(SkillWnd, NWS_VISIBLE | 0x20);
    BackTex = FindTexture(TEXT("L2UI.SkillWnd.skillBack"));
    return 1;
}
NW_HOOK_METHOD(0x1005cbb0, NCGMMagicSkillWnd, OnCreate)

// 0x100bd0e0 (vslot 36) - also NCNPCHtmlViewer's, NCTestHtmlViewer's and
// NCTutorialViewerWnd's (their child at +0x94); BackTex is left
INT NCGMMagicSkillWnd::OnDestroy()
{
    if (SkillWnd)
    {
        RemoveChildWindow(SkillWnd->Entry);
        delete SkillWnd;
        SkillWnd = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100bd0e0, NCGMMagicSkillWnd, OnDestroy)

// stock body shared with NCEditBox's (0x100a5d70)
void NCGMMagicSkillWnd::ShowWindow()
{
    NCWnd::ShowWindow();
}

// 0x1005cda0 (vslot 43)
void NCGMMagicSkillWnd::HideWindow()
{
    ClearSkills();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x1005cda0, NCGMMagicSkillWnd, HideWindow)

// 0x1005cd10 (vslot 50) - the clip 8 pixels taller than the window, the background under the
// title bar
INT NCGMMagicSkillWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height + 8);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 300, 0, 0, 240, 300, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x1005cd10, NCGMMagicSkillWnd, OnPaint)

// 0x1005be40
void NCGMMagicSkillWnd::ClearSkills()
{
    SkillWnd->ClearLists();
}
NW_HOOK_METHOD(0x1005be40, NCGMMagicSkillWnd, ClearSkills)

NW_CHECK_VTABLE(NCGMMagicSkillWnd, 0x100e40d8, 62)
