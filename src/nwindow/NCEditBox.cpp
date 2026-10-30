/*=============================================================================
    NCEditBox.cpp - single line text input. Stock: 0x100080d0 .. 0x10008bd6,
    0x10008be0 .. 0x10009a5a, 0x10009a60 .. 0x1000bb16; WindowProc shared with NCIMEManager,
    ShowWindow folded.
=============================================================================*/
#include "nwindow/NCEditBox.h"
#include <stdio.h>
#include "nwindow/NCIMEManager.h"
#include "nwindow/NCChineseIME.h"
#include "nwindow/NCCompositeBox.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCGDevice.h"
#include "re/NwDebug.h"

enum { NTIMER_EDIT_CARET = 0xFA0D05 };

// Chinese IME fields the edit box reads
static NCCompositeBox* ChineseCompositeBox(NCIMEManager* Manager)
{
    return ((NCChineseIME*)Manager->ChineseIME)->CompositeBox;
}
static const FString& ChineseLastComp(NCIMEManager* Manager)
{
    return ((NCChineseIME*)Manager->ChineseIME)->LastComp;
}

// the three pieces are measured one after the other, as the stock code does everywhere
#define TEXT_W(Str) GetTextWidth(Str)

// 0x10009a60 - NControl's constructor is inlined in the stock code
NCEditBox::NCEditBox(INT InType, const TCHAR* BackTexName, UBOOL bInIMEPosFixed, INT InExtraWidth)
    : CaretTimer(0), BackTex(NULL), WhiteTex(NULL)
{
    Type = InType;
    bIMEPosFixed = bInIMEPosFixed;
    if (BackTexName && BackTexName[0])
        BackTex = FindTexture(BackTexName);
    IMECaretX = 0;
    IMECaretY = 0;
    CaretX = 0;
    Unknown224 = 0;
    CursorPos = 0;
    bReading = 0;
    Unknown1DC = 0;
    ViewStart = 0;
    ViewStart2 = 0;
    ReadingParam = -1;
    bCaretOn = 1;
    bCanInsert = 1;
    ExtraWidth = InExtraWidth;
}
NW_HOOK_CTOR_ARGS(0x10009a60, NCEditBox, (INT A, const TCHAR* B, UBOOL C, INT D), (A, B, C, D))

// 0x10009be0
NW_HOOK_DTOR(0x10009be0, NCEditBox)

// stock body shared with NCIMEManager's (0x10007f80)
INT NCEditBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NIM_COMPOSITION)
        return OnComposition(wParam, lParam) != 0;
    if (Msg == NIM_RESULT)
        return OnResult(wParam, lParam) != 0;
    if (Msg == NIM_ATTRIBUTE)
        return OnAttribute(wParam, lParam) != 0;
    if (Msg == NIM_READING)
        return OnReading(wParam, lParam) != 0;
    if (Msg == NIM_14)
        return On14(wParam, lParam) != 0;
    if (Msg == NIM_GETCARET)
        return OnGetCaret(wParam, lParam) != 0;
    if (Msg == NIM_ENDCOMPOSITION)
        return OnEndComposition(wParam, lParam) != 0;
    if (Msg == NIM_CURSORPOS)
        return OnCursorPos(wParam, lParam) != 0;
    return 0;
}

// 0x10009140 (vslot 28)
void NCEditBox::SetFocus()
{
    NCWnd::SetFocus();
    IMEManager->Activate();
}
NW_HOOK_METHOD(0x10009140, NCEditBox, SetFocus)

// 0x10009160 (vslot 29) - the pieces are joined when any of them is empty (all three set: kept
// apart), as in the stock code
void NCEditBox::KillFocus()
{
    if (Left.Len() == 0 || Comp.Len() == 0 || Right.Len() == 0)
    {
        Left += Comp;
        Left += Right;
        Comp.Empty();
        Right.Empty();
    }
    IMEManager->Deactivate();
    NCWnd::KillFocus();
}
NW_HOOK_METHOD(0x10009160, NCEditBox, KillFocus)

// 0x10009100 (vslot 34) - caret blink
INT NCEditBox::OnTimer(INT TimerID)
{
    if (TimerID == NTIMER_EDIT_CARET)
        bCaretOn = !bCaretOn;
    return 1;
}
NW_HOOK_METHOD(0x10009100, NCEditBox, OnTimer)

// 0x10009c80 (vslot 35) - NControl::OnCreate is not called. IMECaretY: where the IME puts its
// windows (Japanese clients above the box), 137 px or 17 px (bIMEPosFixed not set) away, 152
// for some Chinese layouts.
INT NCEditBox::OnCreate()
{
    Edit1 = FindTexture(TEXT("L2UI.Control.Edit1"));
    Edit2 = FindTexture(TEXT("L2UI.Control.Edit2"));
    Edit3 = FindTexture(TEXT("L2UI.Control.Edit3"));
    EditDisable1 = FindTexture(TEXT("L2UI.Control.Edit_disable1"));
    EditDisable2 = FindTexture(TEXT("L2UI.Control.Edit_disable2"));
    EditDisable3 = FindTexture(TEXT("L2UI.Control.Edit_disable3"));
    CaretTimer = SetTimer(NTIMER_EDIT_CARET, 500, NULL);
    Left.Empty();
    Right.Empty();
    Comp.Empty();
    if (!WhiteTex)
        WhiteTex = FindTexture(TEXT("NWindow.WhiteTexture"));
    WhiteTex = FindTexture(TEXT("NWindow.WhiteTexture"));
    if (bIMEPosFixed)
    {
        if (GConsole->GetIMEType() == 5 || GConsole->GetIMEType() == 7 ||
            GConsole->GetIMEType() == 9 || GConsole->GetIMEType() == 12)
            IMECaretY = 152;
        else
            IMECaretY = GLanguageType == 2 ? -137 : 137;
    }
    else
        IMECaretY = GLanguageType == 3 ? -17 : 17;
    IMEManager = new NCIMEManager(0);
    CreateChild(IMEManager, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10009c80, NCEditBox, OnCreate)

// 0x10009e70 (vslot 36)
INT NCEditBox::OnDestroy()
{
    if (CaretTimer)
    {
        KillTimer(CaretTimer);
        CaretTimer = 0;
    }
    if (IMEManager)
    {
        RemoveChildWindow(IMEManager->Entry);
        delete IMEManager;
        IMEManager = NULL;
    }
    NControl::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10009e70, NCEditBox, OnDestroy)

// 0x100a5d70 (vslot 42) - one body for every class that only calls NCWnd's
void NCEditBox::ShowWindow()
{
    NCWnd::ShowWindow();
}
NW_HOOK_METHOD(0x100a5d70, NCEditBox, ShowWindow)

// 0x1000b370 (vslot 50) - frame, text from ViewStart, IME composition at the caret, the rest,
// then the blinking caret (or the composing character). The widths measured before the text
// is drawn are thrown away, as in the stock code.
INT NCEditBox::OnPaint(NCGDevice* Device)
{
    TCHAR Stars[258];
    SIZE Size;
    INT TWClauseW = 0;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);

    UTexture* RightTex;
    if (IsFocused())
    {
        if (Edit1)
            Device->DrawTexture(0, 0, 8, 16, 0, 0, 8, 16, Edit1);
        if (Edit2)
            Device->DrawTexture(8, 0, Width - 16, 16, 0, 0, 8, 16, Edit2);
        RightTex = Edit3;
    }
    else
    {
        if (EditDisable1)
            Device->DrawTexture(0, 0, 8, 16, 0, 0, 8, Height, EditDisable1);
        if (EditDisable2)
            Device->DrawTexture(8, 0, Width - 16, 16, 0, 0, 8, 16, EditDisable2);
        RightTex = EditDisable3;
    }
    if (RightTex)
        Device->DrawTexture(Width - 8, 0, 8, 16, 0, 0, 8, 16, RightTex);

    if (ExtraWidth > 0)
        MaxWidth = Width + ExtraWidth;
    else
        MaxWidth = Width;

    NCGDevice::GetTextExtent(&Size, *Left, 0, 0, 0, 0, 0);
    INT TextW = Size.cx;
    NCGDevice::GetTextExtent(&Size, *Comp, 0, 0, 0, 0, 0);
    TextW += Size.cx;
    NCGDevice::GetTextExtent(&Size, *Right, 0, 0, 0, 0, 0);
    TextW += Size.cx;
    bCanInsert = TextW < MaxWidth - 12;

    // proof mode, not stock (re/NwDebug.h): '#' for the password and a red caret
    const TCHAR Star = NW_DEBUG_MARKER() ? '#' : '*';
    const DWORD CaretColor = NW_DEBUG_MARKER() ? 0xFFFF0000 : 0xFFFFFFFF;

    if (Type == 2)
    {
        INT i;
        for (i = 0; i < Left.Len(); i++)
            Stars[i] = Star;
        Stars[i] = 0;
        TextW = Device->DrawText(2, 2, 0xFFFFFFFF, Stars, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else
    {
        TextW = 0;
        UBOOL bFull = 0;
        if (ViewStart < Left.Len())
        {
            for (INT i = ViewStart; i < Left.Len(); i++)
            {
                NCGDevice::GetTextExtent(&Size, *Left.Mid(i, 1), 0, 0, 0, 0, 0);
                TextW += Size.cx;
                if (TextW >= Width)
                {
                    bFull = 1;
                    break;
                }
            }
        }
        if (!bFull)
        {
            NCGDevice::GetTextExtent(&Size, *Comp, 0, 0, 0, 0, 0);
            TextW += Size.cx;
            if (TextW < Width)
            {
                for (INT i = 0; i < Right.Len(); i++)
                {
                    NCGDevice::GetTextExtent(&Size, *Right.Mid(i, 1), 0, 0, 0, 0, 0);
                    TextW += Size.cx;
                    if (TextW >= Width)
                        break;
                }
            }
        }
        if (ViewStart < Left.Len())
        {
            TextW = Device->DrawText(2, 2, 0xFFFFFFFF, *Left + ViewStart, 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                                     0, 0, 0);
            if (Left.Len() > 0)
                TextW = GetTextWidth(FString(*Left + ViewStart));
        }
        else
            TextW = 0;
    }

    NCGDevice::GetTextExtent(&Size, *Comp, 0, 0, 0, 0, 0);
    INT CompW = Size.cx;
    NCGDevice::GetTextExtent(&Size, *Reading, 0, 0, 0, 0, 0);
    DWORD ReadingW = Size.cx;
    if (ReadingW > 0 && GConsole->GetIMEType() == 6)
        ReadingW = 12;

    if (Type != 2)
    {
        if (GLanguageType == 2)
            PaintCompositionJP(2, 2, TextW, 0, Device);
        else if (GLanguageType == 0)
            PaintCompositionKR(2, 2, TextW, 0, Device);
        else if (GLanguageType == 3)
            PaintCompositionTW(2, 2, TextW, &TWClauseW, Device);
        else if (GLanguageType == 4)
            PaintCompositionCN(2, 2, TextW, Device);
    }
    if (Type == 2)
    {
        INT i;
        for (i = 0; i < Right.Len(); i++)
            Stars[i] = Star;
        Stars[i] = 0;
        NCGDevice::GetTextExtent(&Size, *Comp, 0, 0, 0, 0, 0);
        Device->DrawText(ReadingW + TextW + Size.cx + 2, 2, 0xFFFFFFFF, Stars, 0, 0, 0, L2FONT_Normal, 0, 0,
                         0, 0, 0, 0);
    }
    else
    {
        // Chinese layouts draw the composition in their own window
        if (GConsole->GetIMEType() == 10 || GConsole->GetIMEType() == 9)
            Size.cx = 0;
        else
            NCGDevice::GetTextExtent(&Size, *Comp, 0, 0, 0, 0, 0);
        Device->DrawText(ReadingW + TextW + Size.cx + 2, 2, 0xFFFFFFFF, *Right, 0, 0, 0, L2FONT_Normal, 0,
                         0, 0, 0, 0, 0);
    }

    if (IMEManager->IsNotComposing() == 0)
    {
        if (Type != 2)
        {
            if (IsFocused() && GLanguageType == 0)
                PaintComposingKR(2, 2, TextW, CompW, Device);
            else if (IsFocused() && GLanguageType == 2)
                PaintComposingJP(2, 2, TextW, CompW, Device);
            else if (IsFocused() && GLanguageType == 3)
                PaintComposingTW(2, 2, TextW, CompW, TWClauseW, ReadingW, Device);
            else if (IsFocused() && GLanguageType == 4)
                PaintComposingCN(2, 2, TextW, CompW, Device);
            if ((DWORD)(CompW + TextW) > (DWORD)Width)
                ViewStart++;
        }
    }
    else
    {
        // the caret stays inside the box: past the right edge the view scrolls one character
        CaretX = TextW + CompW;
        if (CaretX == (DWORD)Width)
            CaretX--;
        else if (CaretX > (DWORD)Width)
        {
            ViewStart++;
            ViewStart2 = ViewStart;
            CaretX = Width - 1;
        }
        if (IsFocused() && bCaretOn)
            Device->DrawText(Type == 2 ? CaretX : CaretX - 1, 2, CaretColor, TEXT("|"), 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x1000b370, NCEditBox, OnPaint)

// 0x10008f50 (vslot 52) - printable ASCII (to 0x81) only, other characters only with IME type
// 9; digits only for Type 3. A '"' typed first in the chat line is reported (3).
INT NCEditBox::OnChar(WPARAM wParam, LPARAM lParam)
{
    TCHAR Temp[20];
    if (!bCanInsert)
        return 1;
    if (Type == 3 && (wParam < '0' || wParam > '9'))
        return 1;
    if (wParam < 0x20 || wParam > 0x81)
    {
        if (GConsole->GetIMEType() != 9)
            return 1;
        if (wParam <= 0x81)
            return 1;
    }
    swprintf(Temp, 20, TEXT("%c"), wParam);
    if (Left.Len() > 0)
        Left += Temp;
    else
    {
        Left = Temp;
        if (wParam == '"' && Type == 1)
            NotifyParent(3);
    }
    if (GetTextWidth(FString(*Left + ViewStart)) > Width)
    {
        SIZE Shown, Typed;
        NCGDevice::GetTextExtent(&Shown, *Left.Mid(ViewStart, 2), 0, 0, 0, 0, 0);
        NCGDevice::GetTextExtent(&Typed, Temp, 0, 0, 0, 0, 0);
        if (Typed.cx < Shown.cx)
            ViewStart += 1;
        else
            ViewStart += 2;
        ViewStart2 = ViewStart;
    }
    return 1;
}
NW_HOOK_METHOD(0x10008f50, NCEditBox, OnChar)

// 0x1000abe0 (vslot 53) - editing keys; the arrows, Home and End wait for the IME to finish
// composing (except Chinese layouts). Delete is Right arrow + Backspace.
INT NCEditBox::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    INT IMEType = GConsole->GetIMEType();
    switch (wParam)
    {
    case VK_LEFT:
        if (IMEManager->IsNotComposing() == 1 || IMEType == 9 || IMEType == 11 || IMEType == 10 ||
            IMEType == 12)
        {
            if (Left.Len() > 0)
            {
                if (CaretX < 13)
                    ViewStart--;
                if (ViewStart < 0)
                    ViewStart = 0;
                FString Old(Right);
                Right.Empty();
                Right = Left.Right(1);
                Right += Old;
                Left.Delete(Left.Len() - 1, 1);
            }
        }
        break;
    case VK_RIGHT:
        if (IMEManager->IsNotComposing() == 1 || IMEType == 9 || IMEType == 11 || IMEType == 10 ||
            IMEType == 12)
        {
            if ((DWORD)(Width - 12) <= CaretX)
                ViewStart++;
            if (Right.Len() > 0)
            {
                Left += Right.Left(1);
                Right.Delete(0, 1);
            }
        }
        break;
    case VK_UP:
        if (Type == 1)
            NotifyParent(4);
        break;
    case VK_DOWN:
        if (Type == 1)
            NotifyParent(5);
        break;
    case VK_HOME:
        if (IMEManager->IsNotComposing() == 1 || IMEType == 10 || IMEType == 9 || IMEType == 11 ||
            IMEType == 12)
        {
            Left += Right;
            Right = Left;
            Left.Empty();
            ViewStart = 0;
        }
        break;
    case VK_END:
        if (IMEManager->IsNotComposing() == 1 || IMEType == 10 || IMEType == 9 || IMEType == 11 ||
            IMEType == 12)
        {
            Left += Right;
            Right.Empty();
            ViewStart = ViewStart2;
        }
        break;
    case VK_BACK:
        // the view scrolls back one character, or two when a wide character was removed and
        // the two before the view are narrow
        if (IMEManager->IsNotComposing() == 1 && Left.Len() > 0)
        {
            SIZE Removed, Before1, Before2;
            NCGDevice::GetTextExtent(&Removed, *Left.Mid(Left.Len() - 1, 1), 0, 0, 0, 0, 0);
            if (ViewStart >= 1)
                NCGDevice::GetTextExtent(&Before1, *Left.Mid(ViewStart - 1, 1), 0, 0, 0, 0, 0);
            else
                Before1.cx = 0;
            if (ViewStart >= 2)
                NCGDevice::GetTextExtent(&Before2, *Left.Mid(ViewStart - 2, 2), 0, 0, 0, 0, 0);
            else
                Before2.cx = 0;
            Left.Delete(Left.Len() - 1, 1);
            if (ViewStart > 0)
            {
                if (Removed.cx >= 12 && Before2.cx > 6 && Before2.cx <= 12)
                {
                    ViewStart -= 2;
                    ViewStart2 -= 2;
                }
                else if (!(Removed.cx < 12 && Before1.cx >= 12 &&
                           GetTextWidth(FString(*Left + ViewStart)) >= Width - 12) &&
                         !(Removed.cx < Before1.cx &&
                           GetTextWidth(FString(*Left + ViewStart)) >= Width - 12))
                {
                    ViewStart--;
                    ViewStart2--;
                }
            }
            if (TEXT_W(Left) + TEXT_W(Comp) + TEXT_W(Right) <= Width)
                ViewStart = 0;
        }
        break;
    case VK_DELETE:
        if (Right.Len() > 0)
        {
            OnKeyDown(VK_RIGHT, 0);
            OnKeyDown(VK_BACK, 0);
        }
        break;
    case VK_RETURN:
        ViewStart = 0;
        ViewStart2 = 0;
        if (Left.Len() == 0 && Comp.Len() == 0 && Right.Len() == 0)
            NotifyParent(0);
        else if (Comp.Len() == 0)
            NotifyParent(1);
        break;
    case VK_PROCESSKEY:
        // Chinese composition window: the arrows move its caret (scan codes in lParam)
        if (IMEType == 10)
        {
            if (lParam == 0x14B0001)
                ChineseCompositeBox(IMEManager)->CaretLeft();
            else if (lParam == 0x14D0001)
                ChineseCompositeBox(IMEManager)->CaretRight();
        }
        break;
    }
    NCWnd::OnKeyDown(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x1000abe0, NCEditBox, OnKeyDown)

// 0x10008bf0 (vslot 63) - Left + Right joined in Left; NULL when there is no text
const TCHAR* NCEditBox::CommitText()
{
    if (Left.Len() == 0 && Right.Len() == 0)
        return NULL;
    Left += Right;
    Right.Empty();
    return *Left;
}
NW_HOOK_METHOD(0x10008bf0, NCEditBox, CommitText)

// 0x10008f10 (vslot 64)
void NCEditBox::ClearText()
{
    Left.Empty();
    Right.Empty();
    Comp.Empty();
    ViewStart = 0;
}
NW_HOOK_METHOD(0x10008f10, NCEditBox, ClearText)

// 0x10009440 (vslot 65) - takes the IME's composition when it still fits (shortening it from the
// end otherwise); Chinese layouts also move the caret of their composition window
INT NCEditBox::OnComposition(WPARAM wParam, LPARAM lParam)
{
    bReading = 0;
    Reading.Empty();
    if (Type == 3 && (wParam < '0' || wParam > '9'))
        return 1;
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *Left, 0, 0, 0, 0, 0);
    INT TextW = Size.cx;
    NCGDevice::GetTextExtent(&Size, *Right, 0, 0, 0, 0, 0);
    TextW += Size.cx;
    if (TextW < MaxWidth - 12)
        bCanInsert = 1;
    else
        bCanInsert = 0;
    if (!bCanInsert)
        return 1;
    Comp.Empty();
    const TCHAR* Str = IMEManager->GetCompStr();
    if (Str)
        Comp = Str;
    NCGDevice::GetTextExtent(&Size, *Comp, 0, 0, 0, 0, 0);
    if (Size.cx + TextW < MaxWidth - 12)
        bCanInsert = 1;
    else
    {
        INT i;
        for (i = 0; i < Comp.Len(); i++)
        {
            Comp = Comp.Left(Comp.Len() - 1);
            NCGDevice::GetTextExtent(&Size, *Comp, 0, 0, 0, 0, 0);
            if (Size.cx + TextW < MaxWidth - 12)
                break;
        }
        if (i == Comp.Len())
            Comp.Empty();
        bCanInsert = 0;
    }
    if (GConsole->GetIMEType() == 10 || GConsole->GetIMEType() == 9)
    {
        NCCompositeBox* Box = ChineseCompositeBox(IMEManager);
        FString Old(ChineseLastComp(IMEManager));
        Box->SetText(*Comp);
        if (Old.Len() > Comp.Len())
            Box->CaretLeft();
        else if (Old.Len() < Comp.Len())
            Box->CaretRight();
        Box->SetText(*Comp);
    }
    return 1;
}
NW_HOOK_METHOD(0x10009440, NCEditBox, OnComposition)

// 0x100091e0 (vslot 66) - the IME's result joins Left (digits only for Type 3); a line past the
// box scrolls by two characters when the first one shown is narrow (under 12 pixels), else by one
INT NCEditBox::OnResult(WPARAM wParam, LPARAM lParam)
{
    bReading = 0;
    Reading.Empty();
    if (!bCanInsert)
        return 1;
    const TCHAR* Str;
    if (Type == 3)
    {
        for (INT i = 0; i < Comp.Len(); i++)
            if ((*Comp.Mid(i, 1))[0] < '0' || (*Comp.Mid(i, 1))[0] > '9')
                return 1;
        Str = *Comp;
    }
    else
        Str = IMEManager->GetResultStr();
    if (Str)
    {
        if (Left.Len() == 0)
            Left = Str;
        else
            Left += Str;
        if (TEXT_W(Left) + TEXT_W(Comp) + TEXT_W(Right) < MaxWidth &&
            GetTextWidth(FString(*Left + ViewStart)) + 12 > Width)
        {
            SIZE Size;
            NCGDevice::GetTextExtent(&Size, *Left.Mid(ViewStart, 1), 0, 0, 0, 0, 0);
            if (Size.cx < 12)
                ViewStart += 2;
            else
                ViewStart += 1;
            ViewStart2 = ViewStart;
        }
        Comp.Empty();
    }
    return 1;
}
NW_HOOK_METHOD(0x100091e0, NCEditBox, OnResult)

// 0x10009700 (vslot 67) - wParam 0/lParam 0 and 1/0 (when it fits): the composition shown;
// 1/1 cleared
INT NCEditBox::OnAttribute(WPARAM wParam, LPARAM lParam)
{
    bReading = 0;
    Reading.Empty();
    const TCHAR* Str = IMEManager->GetCompStr();
    if (!Str)
        return 1;
    if (wParam == 0)
    {
        if (lParam == 0)
            Comp = Str;
    }
    else if (wParam == 1)
    {
        if (lParam == 0)
        {
            if (TEXT_W(Left) + TEXT_W(Comp) + TEXT_W(Right) < MaxWidth - 12)
                Comp = Str;
        }
        else if (lParam == 1)
            Comp.Empty();
    }
    return 1;
}
NW_HOOK_METHOD(0x10009700, NCEditBox, OnAttribute)

// 0x10009820 (vslot 68) - the reading string, when the line has room
INT NCEditBox::OnReading(WPARAM wParam, LPARAM lParam)
{
    bReading = 1;
    Reading.Empty();
    const TCHAR* Str = IMEManager->GetReadingStr();
    if (Str)
    {
        ReadingParam = lParam;
        if (TEXT_W(Left) + TEXT_W(Comp) + TEXT_W(Right) < MaxWidth - 12)
            Reading = Str;
    }
    return 1;
}
NW_HOOK_METHOD(0x10009820, NCEditBox, OnReading)

// 0x10009900 (vslot 70) - wParam: the entry of the window asking
INT NCEditBox::OnGetCaret(WPARAM wParam, LPARAM lParam)
{
    if (wParam)
        SendTo((NWndEntry*)wParam, NIM_CARET, IMECaretX, IMECaretY);
    return 1;
}
NW_HOOK_METHOD(0x10009900, NCEditBox, OnGetCaret)

// 0x10009930 (vslot 71)
INT NCEditBox::OnEndComposition(WPARAM wParam, LPARAM lParam)
{
    if (IMEManager->IsNotComposing() == 1)
        Comp.Empty();
    return 1;
}
NW_HOOK_METHOD(0x10009930, NCEditBox, OnEndComposition)

// 0x10009960 (vslot 72)
INT NCEditBox::OnCursorPos(WPARAM wParam, LPARAM lParam)
{
    CursorPos = wParam;
    return 1;
}
NW_HOOK_METHOD(0x10009960, NCEditBox, OnCursorPos)

// 0x10008be0
void NCEditBox::SetText(const TCHAR* InText)
{
    Left = InText;
}
NW_HOOK_METHOD(0x10008be0, NCEditBox, SetText)

// 0x10008c40 - appended to or put before Left. The scroll test compares "Left is not empty"
// with the width, as in the stock code, so it only fires in boxes up to 12 px wide.
void NCEditBox::InsertText(const TCHAR* InText, UBOOL bAppend)
{
    if (bAppend)
    {
        Left += InText;
        if ((INT)(Left.Num() != 0) > Width - 12)
            ViewStart++;
    }
    else
    {
        FString Old;
        Old = Left;
        Left = InText;
        Left += Old;
        if ((INT)(Left.Num() != 0) > Width - 12)
            ViewStart++;
    }
}
NW_HOOK_METHOD(0x10008c40, NCEditBox, InsertText)

// 0x10008d30 - Index counts through Left, Comp and Right; the positions in Comp and Right are
// taken from their ends, as in the stock code (callers only delete from Left)
void NCEditBox::DeleteChar(INT Index)
{
    if (Index < Left.Len())
    {
        Left.Delete(Index, 1);
        if ((INT)(Left.Num() != 0) > Width - 12)
            ViewStart--;
    }
    else if (Index >= Left.Len() && Index < Comp.Len() + Left.Len())
        Comp.Delete(Comp.Len() - Index + Left.Len(), 1);
    else if (Index >= Comp.Len() + Left.Len() && Index < Right.Len() + Comp.Len() + Left.Len())
        Right.Delete(Right.Len() - Index + Comp.Len() + Left.Len(), 1);
}
NW_HOOK_METHOD(0x10008d30, NCEditBox, DeleteChar)

// 0x10008ea0
INT NCEditBox::GetTextWidth(FString Text)
{
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *Text, 0, 0, 0, 0, 0);
    return Size.cx;
}
NW_HOOK_METHOD(0x10008ea0, NCEditBox, GetTextWidth)

// 0x10009130
void NCEditBox::SetType(INT InType)
{
    Type = InType;
}
NW_HOOK_METHOD(0x10009130, NCEditBox, SetType)

// 0x100080d0 - the character being composed: black on white while the caret blinks on
void NCEditBox::PaintComposingKR(INT X, INT Y, INT TextW, INT CompW, NCGDevice* Device)
{
    if (bCaretOn)
    {
        Device->DrawStretchedTex(X + TextW, Y, CompW, 12, WhiteTex);
        Device->DrawText(X + TextW, Y, 0xFF000000, *Comp, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        return;
    }
    Device->DrawText(X + TextW, Y, 0xFFFFFFFF, *Comp, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
}
NW_HOOK_METHOD(0x100080d0, NCEditBox, PaintComposingKR)

// 0x100087d0 - the composition character by character; IMECaretX goes to the first target
// clause and characters with attribute bit 2 get a red underline
void NCEditBox::PaintCompositionKR(INT X, INT Y, INT TextW, INT Offset, NCGDevice* Device)
{
    IMECaretX = TextW;
    NCIME* IME = IMEManager->KoreanIME;
    INT LineY = bIMEPosFixed ? Y - 1 : Y + 13;
    DWORD* Clause = &IME->CompClause[1];
    UBOOL bTarget = 0;
    for (INT i = 0; i < Comp.Len(); i++)
    {
        if ((IME->CompAttr[i] & 1) && !bTarget)
        {
            bTarget = 1;
            IMECaretX = X + TextW + Offset;
        }
        INT CharX = X + TextW + Offset;
        Offset += Device->DrawText(CharX, Y, 0xFFFFFFFF, *Comp.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0,
                                   0, 0, 0, 0);
        INT EndX = X + TextW + Offset;
        if (*Clause == (DWORD)(i + 1))
        {
            Clause++;
            EndX--;
        }
        if (IsFocused() && (IME->CompAttr[i] & 4))
            Device->DrawLine(CharX, LineY, EndX, LineY, 0xFFFF0000, 0);
    }
}
NW_HOOK_METHOD(0x100087d0, NCEditBox, PaintCompositionKR)

// 0x10008180 - the composition gets a dotted underline (not for the IME types 5 and 7; while
// there is none, the type 7 blinks a caret at TextW - 1, not past X). With a reading string at
// the target clause (ClauseW, see PaintCompositionTW): the type 6 draws it in a column of
// 12 px cells, over the box when the IME position is fixed (the characters from the error
// position on in red on gray, the others in blue) or below it (red and blue only); the type 4
// in the line, black on gray.
void NCEditBox::PaintComposingTW(INT X, INT Y, INT TextW, INT CompW, INT ClauseW, INT ReadingW,
                                 NCGDevice* Device)
{
    if (CompW != 0)
    {
        if (GConsole->GetIMEType() != 5 && GConsole->GetIMEType() != 7)
        {
            for (INT i = 0; i < CompW; i += 3)
                Device->DrawText(X + TextW + i - 2, Y + 3, 0xFFFFFFFF, TEXT("."), 0, 0, 0, L2FONT_Normal, 0,
                                 0, 0, 0, 0, 0);
        }
    }
    else if (GConsole->GetIMEType() == 7 && IsFocused() && bCaretOn)
        Device->DrawText(TextW - 1, Y, 0xFFFFFFFF, TEXT("|"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);

    if (!bReading)
        return;
    if (GConsole->GetIMEType() == 6)
    {
        INT RX = X + TextW + ClauseW;
        if (bIMEPosFixed)
        {
            Device->SetClip(ScreenX, ScreenY - 100, Width, Height + 100);
            INT RY = Y - 42;
            Device->DrawStretchedTex(RX, RY, 12, 48, FindTexture(TEXT("NWindow.WhiteTexture")));
            for (INT i = 0; i < Reading.Len(); i++)
            {
                if (i >= ReadingParam)
                {
                    Device->DrawStretchedTex(RX, RY, 12, 12, FindTexture(TEXT("NWindow.GrayTexture")));
                    Device->DrawText(RX, RY, 0xFFFF0000, *Reading.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0,
                                     0, 0, 0, 0);
                }
                else
                    Device->DrawText(RX, RY, 0xFF0000FF, *Reading.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0,
                                     0, 0, 0, 0);
                RY += 12;
            }
        }
        else
        {
            Device->SetClip(ScreenX, ScreenY, Width, Height + 100);
            INT RY = Y + 6;
            Device->DrawStretchedTex(RX, RY, 12, 48, FindTexture(TEXT("NWindow.WhiteTexture")));
            for (INT i = 0; i < Reading.Len(); i++)
            {
                if (i >= ReadingParam)
                    Device->DrawText(RX, RY, 0xFFFF0000, *Reading.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0,
                                     0, 0, 0, 0);
                else
                    Device->DrawText(RX, RY, 0xFF0000FF, *Reading.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0,
                                     0, 0, 0, 0);
                RY += 12;
            }
        }
        Device->ReleaseClip();
    }
    else if (GConsole->GetIMEType() == 4)
    {
        INT RX = X + TextW + ClauseW;
        Device->DrawStretchedTex(RX, Y, ReadingW, 12, FindTexture(TEXT("NWindow.GrayTexture")));
        Device->DrawText(RX, Y, 0xFF000000, *Reading, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
}
NW_HOOK_METHOD(0x10008180, NCEditBox, PaintComposingTW)

// 0x100085b0 - the Korean one's (see there) with the lines under the text by attribute while
// focused, at Y + 13 whatever bIMEPosFixed says: the target clause (bit 0) twice, one line for
// bits 0 or 1 (both only under characters with a width), one more for bit 1, a red one for
// bit 2
void NCEditBox::PaintCompositionJP(INT X, INT Y, INT TextW, INT Offset, NCGDevice* Device)
{
    IMECaretX = TextW;
    NCIME* IME = IMEManager->JapaneseIME;
    INT LineY = Y + 13;
    DWORD* Clause = &IME->CompClause[1];
    UBOOL bTarget = 0;
    for (INT i = 0; i < Comp.Len(); i++)
    {
        if ((IME->CompAttr[i] & 1) && !bTarget)
        {
            bTarget = 1;
            IMECaretX = X + TextW + Offset;
        }
        INT CharX = X + TextW + Offset;
        INT CharW = Device->DrawText(CharX, Y, 0xFFFFFFFF, *Comp.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0,
                                     0, 0, 0, 0);
        Offset += CharW;
        INT EndX = X + TextW + Offset;
        if (*Clause == (DWORD)(i + 1))
        {
            Clause++;
            EndX--;
        }
        if (IsFocused())
        {
            if ((IME->CompAttr[i] & 1) && CharW > 0)
            {
                Device->DrawLine(CharX, LineY, EndX, LineY + 1, 0xFFFFFFFF, 0);
                Device->DrawLine(CharX, LineY, EndX, LineY, 0xFFFFFFFF, 0);
            }
            if ((IME->CompAttr[i] & 3) && CharW > 0)
                Device->DrawLine(CharX, LineY, EndX, LineY, 0xFFFFFFFF, 0);
            if (IME->CompAttr[i] & 2)
                Device->DrawLine(CharX, LineY, EndX, LineY, 0xFFFFFFFF, 0);
            if (IME->CompAttr[i] & 4)
                Device->DrawLine(CharX, LineY, EndX, LineY, 0xFFFF0000, 0);
        }
    }
}
NW_HOOK_METHOD(0x100085b0, NCEditBox, PaintCompositionJP)

// 0x10008980 - the Korean one's (see there) with the target clause (attribute bit 0) drawn
// black on gray while focused. *ClauseW gets the offset of the first target character, but
// only when that character is drawn 0 px wide (as in the stock code).
void NCEditBox::PaintCompositionTW(INT X, INT Y, INT TextW, INT* ClauseW, NCGDevice* Device)
{
    IMECaretX = TextW;
    NCIME* IME = IMEManager->TaiwanIME;
    INT Offset = 0;
    INT LineY = bIMEPosFixed ? Y - 1 : Y + 13;
    DWORD* Clause = &IME->CompClause[1];
    UBOOL bTarget = 0;
    for (INT i = 0; i < Comp.Len(); i++)
    {
        INT CharX = X + TextW + Offset;
        INT CharW = Device->DrawText(CharX, Y, 0xFFFFFFFF, *Comp.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0,
                                     0, 0, 0, 0);
        if ((IME->CompAttr[i] & 1) && !bTarget)
        {
            bTarget = 1;
            IMECaretX = CharX;
            if (!CharW)
                *ClauseW = Offset;
        }
        Offset += CharW;
        INT EndX = X + TextW + Offset;
        if (*Clause == (DWORD)(i + 1))
        {
            Clause++;
            EndX--;
        }
        if (IsFocused())
        {
            if ((IME->CompAttr[i] & 1) && CharW > 0)
            {
                Device->DrawStretchedTex(CharX, Y, EndX - CharX, 12, FindTexture(TEXT("NWindow.GrayTexture")));
                Device->DrawText(CharX, Y, 0xFF000000, *Comp.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                                 0, 0);
            }
            if (IME->CompAttr[i] & 4)
                Device->DrawLine(CharX, LineY, EndX, LineY, 0xFFFF0000, 0);
        }
    }
}
NW_HOOK_METHOD(0x10008980, NCEditBox, PaintCompositionTW)

// 0x10009980 - the caret of the composition: Left's width plus the composition's up to
// CursorPos
INT NCEditBox::GetCompCaretX()
{
    SIZE Size;
    FString Str;
    NCGDevice::GetTextExtent(&Size, *Left, 0, 0, 0, 0, 0);
    INT LeftW = Size.cx;
    Str = Comp.Left(CursorPos);
    NCGDevice::GetTextExtent(&Size, *Str, 0, 0, 0, 0, 0);
    return Size.cx + LeftW;
}
NW_HOOK_METHOD(0x10009980, NCEditBox, GetCompCaretX)

// 0x10009ed0 - the characters still being typed (attribute 0) get a dotted underline from the
// first of the last run of them, and the caret blinks as a double bar at CursorPos (see
// GetCompCaretX, which also gives CaretX); CompW is not used
void NCEditBox::PaintComposingJP(INT X, INT Y, INT TextW, INT CompW, NCGDevice* Device)
{
    SIZE Size;
    CaretX = GetCompCaretX();
    INT UnderlineW = 0;
    INT StartX = 0;
    for (INT i = 0; i < IMEManager->JapaneseIME->CompAttrLen; i++)
    {
        NCGDevice::GetTextExtent(&Size, *Comp.Mid(i, 1), 0, 0, 0, 0, 0);
        NCIME* IME = IMEManager->JapaneseIME;
        if (IME->CompAttr[i] == ATTR_INPUT)
        {
            if (i == 0 || IME->CompAttr[i - 1] != ATTR_INPUT)
            {
                NCGDevice::GetTextExtent(&Size, *Comp.Mid(0, i), 0, 0, 0, 0, 0);
                StartX = Size.cx;
                NCGDevice::GetTextExtent(&Size, *Comp.Mid(i, 1), 0, 0, 0, 0, 0);
                UnderlineW += Size.cx;
            }
            else
            {
                NCGDevice::GetTextExtent(&Size, *Comp.Mid(i, 1), 0, 0, 0, 0, 0);
                UnderlineW += Size.cx;
            }
        }
    }
    if (UnderlineW > 0)
    {
        for (INT j = 0; j < UnderlineW; j += 3)
            Device->DrawText(X + TextW + StartX + j - 2, Y + 3, 0xFFFFFFFF, TEXT("."), 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    if (bCaretOn)
    {
        Device->DrawText(CaretX + X - 1, Y, 0xFFFFFFFF, TEXT("|"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Device->DrawText(CaretX + X, Y, 0xFFFFFFFF, TEXT("|"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
}
NW_HOOK_METHOD(0x10009ed0, NCEditBox, PaintComposingJP)

// 0x1000a180 - the IME type 8 draws the composition itself: the part in view (from the view
// start past Left, as far as the box goes) with a dotted underline, the target characters
// (attribute 1) black on white with the composition window over each and the candidate box
// beside it (IMECaretY: 152 px - 137 but for the types 5, 9 and 12 - above the box when the
// IME position is fixed, else 17 px below), the caret after the composition when nothing is
// the target, and the view scrolled to keep the target in it. The types 9 to 12 only blink
// the caret. CompW is not used.
void NCEditBox::PaintComposingCN(INT X, INT Y, INT TextW, INT CompW, NCGDevice* Device)
{
    if (IMEManager->IsNotComposing())
        return;
    if (GConsole->GetIMEType() == 8)
    {
        SIZE Size;
        INT Start = 0;
        INT VisibleW = 0;
        if (bIMEPosFixed)
        {
            if (GConsole->GetIMEType() == 5 || GConsole->GetIMEType() == 9 || GConsole->GetIMEType() == 12)
                IMECaretY = 152;
            else
                IMECaretY = 137;
        }
        else
            IMECaretY = 17;
        if (ViewStart >= Left.Len())
            Start = ViewStart - Left.Len();
        INT i;
        for (i = Start; i < Comp.Len(); i++)
        {
            NCGDevice::GetTextExtent(&Size, *Comp.Mid(i, 1), 0, 0, 0, 0, 0);
            if ((DWORD)(VisibleW + Size.cx + TextW) > (DWORD)Width)
                break;
            VisibleW += Size.cx;
        }
        INT CompX = X + TextW;
        Device->DrawText(CompX, Y, 0xFFFFFFFF, *Comp.Mid(Start, i - Start), 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                         0, 0, 0);
        for (INT j = 0; j < VisibleW; j += 3)
            Device->DrawText(CompX + j - 2, Y + 3, 0xFFFFFFFF, TEXT("."), 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                             0, 0, 0);

        IMECaretX = 0;
        INT OffX = 0;
        INT TargetW = 0;
        NCCandidateBox* Cand = ((NCChineseIME*)IMEManager->ChineseIME)->CandidateBox;
        for (i = Start; i < IMEManager->ChineseIME->CompAttrLen; i++)
        {
            NCGDevice::GetTextExtent(&Size, *Comp.Mid(i, 1), 0, 0, 0, 0, 0);
            if (IMEManager->ChineseIME->CompAttr[i] == ATTR_TARGET_CONVERTED)
            {
                INT CharX = CompX + OffX;
                TargetW = Size.cx;
                Device->DrawStretchedTex(CharX, Y, TargetW, 12, FindTexture(TEXT("NWindow.WhiteTexture")));
                Device->DrawText(CharX, Y, 0xFF000000, *Comp.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                                 0, 0);
                IMECaretX = CompX + OffX + 5;
                ChineseCompositeBox(IMEManager)->SetWindowRect(IMECaretX, Y - 10, 50, 15);
                Cand->SetWindowRect(IMECaretX, bIMEPosFixed ? -IMECaretY : IMECaretY, Cand->Width,
                                    Cand->Height);
            }
            OffX += Size.cx;
        }
        if (!TargetW && IsFocused() && bCaretOn)
            Device->DrawText(X + OffX + TextW - 2, Y, 0xFFFFFFFF, TEXT("|"), 0, 0, 0, L2FONT_Normal, 0, 0,
                             0, 0, 0, 0);

        // the view follows the target: back while it is before the view start, on while the
        // text up to it is wider than the box
        INT ViewW = 0;
        if (ViewStart < Left.Len())
        {
            NCGDevice::GetTextExtent(&Size, *Comp.Mid(ViewStart, Comp.Len() - ViewStart), 0, 0, 0, 0, 0);
            ViewW = Size.cx;
        }
        INT k;
        for (k = 0; k < IMEManager->ChineseIME->CompAttrLen; k++)
        {
            NCGDevice::GetTextExtent(&Size, *Comp.Mid(k, 1), 0, 0, 0, 0, 0);
            if (k >= ViewStart - Left.Len())
                ViewW += Size.cx;
            if (IMEManager->ChineseIME->CompAttr[k] == ATTR_TARGET_CONVERTED)
                break;
        }
        if (k < ViewStart - Left.Len())
            ViewStart--;
        else if ((DWORD)ViewW > (DWORD)Width)
        {
            ViewStart++;
            ViewStart2 = ViewStart;
        }
    }
    else if (GConsole->GetIMEType() == 10 || GConsole->GetIMEType() == 9 || GConsole->GetIMEType() == 11 ||
             GConsole->GetIMEType() == 12)
    {
        if (IsFocused() && bCaretOn)
            Device->DrawText(X + TextW - 2, Y, 0xFFFFFFFF, TEXT("|"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                             0);
    }
}
NW_HOOK_METHOD(0x1000a180, NCEditBox, PaintComposingCN)

// 0x1000a7f0 - the Chinese IME's windows; nothing composed hides the composition window. The
// IME type 8 shows the reading string in it, 5 px past the composition's first character
// being typed or converted (attribute 0 or 1, counted from the view start) and 10 px up
// (hidden without one). The types 9 to 12 put it at the end of the box's text (not moved while
// Unknown1DC is set), 152 px - 137 but for the types 5, 9 and 12 - above the box when the IME
// position is fixed, else 17 px below, and the candidate box 100 px right of it. Only the type
// 10 shows the window's caret.
void NCEditBox::PaintCompositionCN(INT X, INT Y, INT TextW, NCGDevice* Device)
{
    if (IMEManager->IsNotComposing())
    {
        ChineseCompositeBox(IMEManager)->HideWindow();
        return;
    }
    SIZE Size;
    INT OffX = 0;
    if (ViewStart < Left.Len())
    {
        for (INT i = 0; i < Comp.Len(); i++)
        {
            NCGDevice::GetTextExtent(&Size, *Comp.Mid(i, 1), 0, 0, 0, 0, 0);
            BYTE Attr = IMEManager->ChineseIME->CompAttr[i];
            if (Attr == ATTR_TARGET_CONVERTED || Attr == ATTR_INPUT)
                break;
            OffX += Size.cx;
        }
    }
    else
    {
        for (INT i = ViewStart - Left.Len(); i < Comp.Len(); i++)
        {
            NCGDevice::GetTextExtent(&Size, *Comp.Mid(i, 1), 0, 0, 0, 0, 0);
            BYTE Attr = IMEManager->ChineseIME->CompAttr[i];
            if (Attr == ATTR_TARGET_CONVERTED || Attr == ATTR_INPUT)
                break;
            OffX += Size.cx;
        }
    }
    NCCompositeBox* Box = ChineseCompositeBox(IMEManager);
    if (GConsole->GetIMEType() == 10 || GConsole->GetIMEType() == 9 || GConsole->GetIMEType() == 11 ||
        GConsole->GetIMEType() == 12)
    {
        if (bIMEPosFixed)
        {
            if (GConsole->GetIMEType() == 5 || GConsole->GetIMEType() == 9 || GConsole->GetIMEType() == 12)
                IMECaretY = 152;
            else
                IMECaretY = 137;
        }
        else
            IMECaretY = 17;
        if (!Unknown1DC)
        {
            if (bIMEPosFixed)
                Box->SetWindowRect(X + TextW, -IMECaretY, 100, 15);
            else
                Box->SetWindowRect(X + TextW, IMECaretY, 100, 15);
        }
        Box->ShowWindow();
        if (Unknown1DC)
            IMECaretX = X + TextW + 5;
        else
            IMECaretX = X + TextW + 105;
        NCCandidateBox* Cand = ((NCChineseIME*)IMEManager->ChineseIME)->CandidateBox;
        if (Cand)
            Cand->SetWindowRect(IMECaretX, bIMEPosFixed ? -IMECaretY : IMECaretY, Cand->Width,
                                Cand->Height);
    }
    else if (GConsole->GetIMEType() == 8)
    {
        Box->SetText(*Reading);
        Box->SetWindowRect(X + OffX + TextW + 5, Y - 10, 50, 15);
        if (Reading.Len())
            Box->ShowWindow();
        else
            Box->HideWindow();
    }
    if (GConsole->GetIMEType() == 10)
        Box->SetShowCaret(1);
    else
        Box->SetShowCaret(0);
}
NW_HOOK_METHOD(0x1000a7f0, NCEditBox, PaintCompositionCN)

NW_CHECK_VTABLE(NCEditBox, 0x100d9758, 73)
