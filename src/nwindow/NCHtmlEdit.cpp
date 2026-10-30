/*=============================================================================
    NCHtmlEdit.cpp - NCHtmlEdit and NCJHtmlEdit, the edit boxes of the HTML pages.
    Stock: 0x10030160 .. 0x100301e7, 0x100310a0 .. 0x100310b7, 0x100326e0 .. 0x10032f9f
    (WindowProc, OnKeyDown and GetValue are one body for both classes).
=============================================================================*/
#include "nwindow/NCHtmlEdit.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCIMEManager.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// the code a click sends the viewer with the box's y: MAKELONG(100, LocalY)
enum { HTML_SCROLLTO = 100 };

/*-----------------------------------------------------------------------------
    NCHtmlEdit.
-----------------------------------------------------------------------------*/

// 0x100326e0 - NCEditBox(0, NULL, 1, 0), then the type, the back texture and the extra width
NCHtmlEdit::NCHtmlEdit(INT InExtraWidth, INT InType)
    : NCEditBox(0, NULL, 1, 0)
{
    Type = InType;
    EnterCommand = NULL;
    if (!BackTex)
        BackTex = FindTexture(TEXT("NWindow.chatback"));
    if (InExtraWidth > 0)
        ExtraWidth = InExtraWidth;
}
NW_HOOK_CTOR_ARGS(0x100326e0, NCHtmlEdit, (INT A, INT B), (A, B))

// 0x100327a0
NW_HOOK_DTOR(0x100327a0, NCHtmlEdit)

// 0x100301a0 (vslot 24) - also NCJHtmlEdit's
INT NCHtmlEdit::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCEditBox::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x100301a0, NCHtmlEdit, WindowProc)

// 0x100327f0 (vslot 50) - NCEditBox::OnPaint with the back texture stretched instead of the
// frame, clipped to the viewer's height (its own width); the origin goes on before the clip.
// The IME painters get the (still 0) clause width where NCEditBox passes 0.
INT NCHtmlEdit::OnPaint(NCGDevice* Device)
{
    TCHAR Stars[258];
    SIZE Size;
    INT TWClauseW = 0;
    Device->BeginPaint(ScreenX, ScreenY);
    NCWnd* Owner = FirstNonControlParent(this);
    if (Owner)
        Device->SetClip(ScreenX, Owner->ScreenY, Width, Owner->Height);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);

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

    if (Type == 2)
    {
        INT i;
        for (i = 0; i < Left.Len(); i++)
            Stars[i] = '*';
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
            PaintCompositionJP(2, 2, TextW, TWClauseW, Device);
        else if (GLanguageType == 0)
            PaintCompositionKR(2, 2, TextW, TWClauseW, Device);
        else if (GLanguageType == 3)
            PaintCompositionTW(2, 2, TextW, &TWClauseW, Device);
        else if (GLanguageType == 4)
            PaintCompositionCN(2, 2, TextW, Device);
    }
    if (Type == 2)
    {
        INT i;
        for (i = 0; i < Right.Len(); i++)
            Stars[i] = '*';
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
            Device->DrawText(Type == 2 ? CaretX : CaretX - 1, 2, 0xFFFFFFFF, TEXT("|"), 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x100327f0, NCHtmlEdit, OnPaint)

// 0x10030160 (vslot 53) - Enter with a command sends it; anything else is NCEditBox's
INT NCHtmlEdit::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN && EnterCommand)
    {
        NotifyParent((LPARAM)EnterCommand);
        return 1;
    }
    NCEditBox::OnKeyDown(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x10030160, NCHtmlEdit, OnKeyDown)

// 0x100310a0 (vslot 56) - the viewer scrolls the box into view
INT NCHtmlEdit::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    NotifyParent(((DWORD)(_WORD)LocalY << 16) | HTML_SCROLLTO);
    return 1;
}
NW_HOOK_METHOD(0x100310a0, NCHtmlEdit, OnLButtonDown)

// 0x100301e0 (vslot 62)
const TCHAR* NCHtmlEdit::GetValue()
{
    return CommitText();
}
NW_HOOK_METHOD(0x100301e0, NCHtmlEdit, GetValue)

NW_CHECK_VTABLE(NCHtmlEdit, 0x100df360, 73)

/*-----------------------------------------------------------------------------
    NCJHtmlEdit.
-----------------------------------------------------------------------------*/

// 0x10032f10 - the three arguments are not used
NCJHtmlEdit::NCJHtmlEdit(INT Unused1, INT Unused2, INT Unused3)
    : NCEditBox(0, NULL, 1, 0)
{
    EnterCommand = NULL;
}
NW_HOOK_CTOR_ARGS(0x10032f10, NCJHtmlEdit, (INT A, INT B, INT C), (A, B, C))

// 0x10032f70
NW_HOOK_DTOR(0x10032f70, NCJHtmlEdit)

// stock body shared with NCHtmlEdit's (0x100301a0)
INT NCJHtmlEdit::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCEditBox::WindowProc(Msg, wParam, lParam) != 0;
}

// stock body shared with NCHtmlEdit's (0x10030160)
INT NCJHtmlEdit::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN && EnterCommand)
    {
        NotifyParent((LPARAM)EnterCommand);
        return 1;
    }
    NCEditBox::OnKeyDown(wParam, lParam);
    return 1;
}

// stock body shared with NCHtmlEdit's (0x100301e0)
const TCHAR* NCJHtmlEdit::GetValue()
{
    return CommitText();
}

NW_CHECK_VTABLE(NCJHtmlEdit, 0x100df488, 73)
