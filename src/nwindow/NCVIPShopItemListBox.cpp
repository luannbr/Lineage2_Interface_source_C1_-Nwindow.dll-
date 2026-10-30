/*=============================================================================
    NCVIPShopItemListBox.cpp - VIP shop item list. Stock: 0x1006d380 .. 0x1006d5cf,
    0x1006de10 .. 0x1006df1a, 0x1006e810 .. 0x1006e912; the other bodies are identical to
    NCTrainListBox's and folded with them (hooked in NCTrainListBox.cpp).
=============================================================================*/
#include "nwindow/NCVIPShopItemListBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"

// 0x1006e810
NCVIPShopItemListBox::NCVIPShopItemListBox()
{
    LineGap = 4;
}
NW_HOOK_CTOR(0x1006e810, NCVIPShopItemListBox)

// 0x1006e8d0
NCVIPShopItemListBox::NCVIPShopItemListBox(INT InMaxLines, INT InVisibleLines, INT Unused)
    : NCListBox(InMaxLines, InVisibleLines, Unused)
{
    LineGap = 4;
}
NW_HOOK_CTOR_ARGS(0x1006e8d0, NCVIPShopItemListBox, (INT A, INT B, INT C), (A, B, C))

// 0x1006e870
NW_HOOK_DTOR(0x1006e870, NCVIPShopItemListBox)

// stock body shared with NCDebugWnd's (0x100576d0)
INT NCVIPShopItemListBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCListBox::WindowProc(Msg, wParam, lParam) != 0;
}

// stock body shared with NCTrainListBox's (0x100ba9a0)
INT NCVIPShopItemListBox::OnCreate()
{
    NCListBox::OnCreate();
    BackTex = FindTexture(TEXT("sek.cbui315"));
    return 1;
}

// stock body shared with NCTrainListBox's (0x1006dda0)
INT NCVIPShopItemListBox::OnDestroy()
{
    for (INT i = 0; i < Entries.Num(); i++)
    {
        if (Entries(i))
            appFree(Entries(i));
    }
    Entries.Empty();
    NCListBox::OnDestroy();
    return 1;
}

// 0x1006d380 (vslot 50) - icon, name, description and, for type 5, the count. A NULL entry
// skips only the icon, as in the stock code; the guard has NCTrainListBox's name.
INT NCVIPShopItemListBox::OnPaint(NCGDevice* Device)
{
    guard(NCTrainListBox::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    INT Line = 0;
    for (INT i = TopIndex; i < VisibleLines + TopIndex && i <= NumLines && i < Entries.Num();
         i++, Line++)
    {
        NVIPShopEntry* Entry = Entries(i);
        if (i == Selected && HighlightTex)
            Device->DrawStretchedTex(0, (LineGap + 32) * Line, Width - 18, 32, HighlightTex);
        if (Entry && Entry->Icon)
            Device->DrawTexture(TextX, (LineGap + 32) * Line + TextY, 32, 32, 0, 0, 32, 32,
                                Entry->Icon);
        if (Entry->Name[0])
            Device->DrawText(TextX + 34, (LineGap + 32) * Line + TextY + 2, 0xFFFFFFFF, Entry->Name,
                             0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        if (Entry->Desc[0])
            Device->DrawText(TextX + 34, (LineGap + 32) * Line + TextY + 16, 0xFFB09B79,
                             Entry->Desc, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        if (Entry->Type == 5)
        {
            TCHAR Temp[64];
            appSprintf(Temp, TEXT("X %d"), Entry->Count);
            Device->DrawText(TextX + 34, (LineGap + 32) * Line + TextY + 16, 0xFFFFFFFF, Temp, 0,
                             0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1006d380, NCVIPShopItemListBox, OnPaint)

// stock body shared with NCTrainListBox's (0x1006bad0)
INT NCVIPShopItemListBox::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Entries.Num())
        NotifyParent(Selected);
    return 1;
}

// stock body shared with NCTrainListBox's (0x100ba9d0)
INT NCVIPShopItemListBox::HitTest(INT X, INT Y)
{
    INT RX = X - ScreenX;
    INT RY = Y - ScreenY;
    if (RX < 0 || RX > Width || RY < 0 || RY > Height)
        return -1;
    INT Line = 0;
    for (INT i = TopIndex; i < VisibleLines + TopIndex; i++, Line++)
    {
        if (i >= NumLines)
            break;
        if (RX < Width && RY >= (LineGap + 32) * Line + TextY &&
            RY < (Line + 1) * (LineGap + 32) + TextY)
        {
            Selected = i;
            break;
        }
    }
    return Selected;
}

// stock body shared with NCTrainListBox's (0x100bc570)
void NCVIPShopItemListBox::Clear()
{
    for (INT i = 0; i < Entries.Num(); i++)
    {
        if (Entries(i))
            appFree(Entries(i));
    }
    Entries.Empty();
    ScrollBar->Reset();
    TopIndex = 0;
    NumLines = 0;
    Selected = -1;
}

// stock body shared with NCTrainListBox's (0x100baaa0)
void NCVIPShopItemListBox::UpdateEntryScrollBar()
{
    NumLines = Entries.Num();
    ScrollBar->SetRange(NumLines, VisibleLines);
    ScrollBar->SetValue(TopIndex);
}

// 0x1006de10 - keeps a copy (empty strings are not copied)
void NCVIPShopItemListBox::AddEntry(const NVIPShopEntry* Src)
{
    NVIPShopEntry* Entry = new NVIPShopEntry;
    if (Src->Name[0])
        appStrcpy(Entry->Name, Src->Name);
    if (Src->Desc[0])
        appStrcpy(Entry->Desc, Src->Desc);
    Entry->ProductID = Src->ProductID;
    Entry->Icon = Src->Icon;
    Entry->Count = Src->Count;
    Entry->Type = Src->Type;
    if (NumLines < MaxLines)
    {
        NumLines++;
        if (bAutoScroll && NumLines > VisibleLines)
            TopIndex++;
    }
    INT i = Entries.Add();
    Entries(i) = Entry;
    UpdateEntryScrollBar();
}
NW_HOOK_METHOD(0x1006de10, NCVIPShopItemListBox, AddEntry)

NW_CHECK_VTABLE(NCVIPShopItemListBox, 0x100e6440, 70)
