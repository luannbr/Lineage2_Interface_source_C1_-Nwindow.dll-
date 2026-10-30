/*=============================================================================
    NCPartyWnd.cpp - the party window. Stock: 0x100ae9b0 .. 0x100aea8b,
    0x100b1720 .. 0x100b1bdf, 0x100b34f0 .. 0x100b36d3.
=============================================================================*/
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"

// NWM_COMMAND id that shows / hides the member list (no sender found in NWindow)
enum { NID_PARTY_TOGGLE = 0xF0 };

// title bar id
enum { NID_PARTY_FRAME = 0xFFA0B0 };

// 0x100b1720
NCPartyWnd::NCPartyWnd()
{
    bShown = 0;
    NumMembers = 0;
}
NW_HOOK_CTOR(0x100b1720, NCPartyWnd)

// 0x100b1770
NW_HOOK_DTOR(0x100b1770, NCPartyWnd)

// 0x100ae9b0 (vslot 24)
INT NCPartyWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == NID_PARTY_TOGGLE)
        return OnToggle() != 0;
    return 0;
}
NW_HOOK_METHOD(0x100ae9b0, NCPartyWnd, WindowProc)

// 0x100b17c0 (vslot 35) - the eight member windows exist from the start, hidden until used
INT NCPartyWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameTopTex = FindTexture(TEXT("L2UI.TargetWnd.TargetFrameTop"));
    FrameCenterTex = FindTexture(TEXT("L2UI.TargetWnd.TargetFrameCenter"));
    FrameBottomTex = FindTexture(TEXT("L2UI.TargetWnd.TargetFrameBottom"));

    FrameCtrl = new NCFrameCtrl(0);
    FrameCtrl->SetWindowRect(0, 0, 12, Height);
    FrameCtrl->ID = NID_PARTY_FRAME;
    CreateChild(FrameCtrl, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown

    for (INT i = 0; i < NUM_PARTY_SLOTS; i++)
    {
        Members[i] = new NCPartyStatusWnd(144);
        Members[i]->SetWindowRect(12, 0, 140, 36);
        CreateChild(Members[i], NWS_VISIBLE);
    }
    return 1;
}
NW_HOOK_METHOD(0x100b17c0, NCPartyWnd, OnCreate)

// 0x100b1940 (vslot 36) - NCWnd's first, then the members
INT NCPartyWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    for (INT i = 0; i < NUM_PARTY_SLOTS; i++)
    {
        if (Members[i])
        {
            RemoveChildWindow(Members[i]->Entry);
            delete Members[i];
            Members[i] = NULL;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x100b1940, NCPartyWnd, OnDestroy)

// 0x100b1bd0 (vslot 42)
void NCPartyWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    UpdateLayout();
}
NW_HOOK_METHOD(0x100b1bd0, NCPartyWnd, ShowWindow)

// 0x100b1990 (vslot 50) - the left edge, while there is a party
INT NCPartyWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (NumMembers > 0)
    {
        if (FrameTopTex)
            Device->DrawTexture(0, 0, 12, 8, 0, 0, 12, 8, FrameTopTex);
        if (FrameCenterTex)
            Device->DrawTexture(0, 7, 12, Height - 13, 0, 0, 11, 16, FrameCenterTex);
        if (FrameBottomTex)
            Device->DrawTexture(0, Height - 7, 12, 7, 0, 0, 12, 7, FrameBottomTex);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100b1990, NCPartyWnd, OnPaint)

// 0x100b36a0 (vslot 62)
INT NCPartyWnd::OnToggle()
{
    if (bShown)
    {
        bShown = 0;
        UpdateLayout();
        return 1;
    }
    bShown = 1;
    UpdateLayout();
    return 1;
}
NW_HOOK_METHOD(0x100b36a0, NCPartyWnd, OnToggle)

// 0x100aea20 - new HP / MP of a member, found by name among the first NumMembers
void NCPartyWnd::UpdateMember(INT MemberID, const TCHAR* Name, INT HP, INT MaxHP, INT MP,
                              INT MaxMP)
{
    if (!Name)
        return;
    for (INT i = 0; i < NumMembers; i++)
    {
        if (Members[i]->IsName(Name))
        {
            Members[i]->SetInfo(MemberID, Name, HP, MaxHP, MP, MaxMP);
            return;
        }
    }
}
NW_HOOK_METHOD(0x100aea20, NCPartyWnd, UpdateMember)

// 0x100b1a70 - the members shown stacked under each other, 34 pixels apart (36 for the
// first); the window takes their height. Kept out of line like the stock code.
__declspec(noinline) void NCPartyWnd::UpdateLayout()
{
    if (bShown)
    {
        SetSize(152, 0);
        // the stock code fills { LocalX, LocalY, LocalW, LocalH }; SetScreenPos reads x and y
        POINT Origin;
        Origin.x = LocalX;
        Origin.y = LocalY;
        for (INT i = 0; i < NUM_PARTY_SLOTS; i++)
            Members[i]->SetScreenPos(12, 0, &Origin);
        UBOOL bAny = 0;
        for (INT i = 0; i < NUM_PARTY_SLOTS; i++)
        {
            NCPartyStatusWnd* Member = Members[i];
            if (!Member->IsEmpty())
            {
                Member->ShowWindow();
                if (!bAny)
                {
                    SetSize(152, LocalH + 36);
                    bAny = 1;
                }
                else
                    SetSize(152, LocalH + 34);
                for (INT j = i + 1; j < NUM_PARTY_SLOTS; j++)
                    Members[j]->OffsetWindow(0, 34);
            }
            else
                Member->HideWindow();
        }
    }
    else
    {
        for (INT i = 0; i < NUM_PARTY_SLOTS; i++)
            Members[i]->HideWindow();
    }
    FrameCtrl->SetWindowRect(0, 0, 12, Height);
}
NW_HOOK_METHOD(0x100b1a70, NCPartyWnd, UpdateLayout)

// 0x100b34f0 - into the first free slot; the list shows itself
void NCPartyWnd::AddMember(INT MemberID, const TCHAR* Name, INT HP, INT MaxHP, INT MP,
                           INT MaxMP)
{
    if (!Name)
        return;
    for (INT i = 0; i < NUM_PARTY_SLOTS; i++)
    {
        if (Members[i]->IsEmpty())
        {
            Members[i]->SetInfo(MemberID, Name, HP, MaxHP, MP, MaxMP);
            NumMembers++;
            bShown = 1;
            UpdateLayout();
            return;
        }
    }
}
NW_HOOK_METHOD(0x100b34f0, NCPartyWnd, AddMember)

// 0x100b3570 - the members below move up a slot
void NCPartyWnd::DeleteMember(INT MemberID, const TCHAR* Name)
{
    if (!Name)
        return;
    for (INT i = 0; i < NUM_PARTY_SLOTS; i++)
    {
        if (Members[i]->IsName(Name))
        {
            Members[i]->Clear();
            for (INT j = i + 1; j < NUM_PARTY_SLOTS; j++)
            {
                NCPartyStatusWnd* Member = Members[j];
                if (!Member->IsEmpty())
                {
                    Members[j - 1]->SetInfo(Member->MemberID, Member->Name, Member->HP,
                                            Member->MaxHP, Member->MP, Member->MaxMP);
                    Member->Clear();
                }
            }
            if (--NumMembers == 0)
                bShown = 0;
            UpdateLayout();
            return;
        }
    }
}
NW_HOOK_METHOD(0x100b3570, NCPartyWnd, DeleteMember)

// 0x100b3650
void NCPartyWnd::DeleteAll()
{
    for (INT i = 0; i < NumMembers; i++)
        Members[i]->Clear();
    NumMembers = 0;
    bShown = 0;
    UpdateLayout();
}
NW_HOOK_METHOD(0x100b3650, NCPartyWnd, DeleteAll)

NW_CHECK_VTABLE(NCPartyWnd, 0x100efde8, 63)
