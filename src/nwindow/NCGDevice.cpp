/*=============================================================================
    NCGDevice.cpp - paint device (UCanvas wrapper with window origin).
    Stock: 0x1000c700 .. 0x1000d25b, 0x1000d6a0 .. 0x1000d806, 0x1000dd30 .. 0x1000dd5b (the
    origins' deque is NStdLib.cpp's).
=============================================================================*/
#include "nwindow/NCGDevice.h"
#include "nwindow/NL2Font.h"
#include "nwindow/NCWnd.h"
#include "nwindow/NConsoleWnd.h"

#include <wchar.h>

/*-----------------------------------------------------------------------------
    Construction / destruction
-----------------------------------------------------------------------------*/

// 0x1000d6a0
NCGDevice::NCGDevice()
    : Canvas(NULL), OriginX(0), OriginY(0)
{
    SIZE Size;
    GetTextExtent(&Size, TEXT("c"), 0, 0, 0, 0, 0);
    LineHeight = Size.cy;
}
NW_HOOK_CTOR(0x1000d6a0, NCGDevice)

// 0x1000d750 - ConditionalDestroy, the origins' Tidy, ~NCObject (NW_DECLARE_CLASS + members)
NW_HOOK_DTOR(0x1000d750, NCGDevice)

// 0x1000d220 (vslot 10) - the text align parser is shared by the whole package
void NCGDevice::Destroy()
{
    Super::Destroy();
    if (GTextAlignParser)
    {
        delete GTextAlignParser;
        GTextAlignParser = NULL;
    }
}
NW_HOOK_METHOD(0x1000d220, NCGDevice, Destroy)

// 0x1000d180
void NCGDevice::Init()
{
    SmallDigits = FindTexture(TEXT("SEK.cbui13"));
    LargeDigits = FindTexture(TEXT("L2UI.NWindow.Number"));
    if (!GTextAlignParser)
        GTextAlignParser = new FL2TextAlignParser;
}
NW_HOOK_METHOD(0x1000d180, NCGDevice, Init)

// 0x1000ca10 - NCVirtualWndMain::SetGDevice, each frame
UBOOL NCGDevice::SetCanvas(UCanvas* InCanvas)
{
    Canvas = InCanvas;
    return 1;
}
NW_HOOK_METHOD(0x1000ca10, NCGDevice, SetCanvas)

// 0x1000ca20 - NCVirtualWndMain::ReleaseGDevice
void NCGDevice::ClearCanvas()
{
    Canvas = NULL;
}
NW_HOOK_METHOD(0x1000ca20, NCGDevice, ClearCanvas)

/*-----------------------------------------------------------------------------
    Text
-----------------------------------------------------------------------------*/

// 0x1000c700 - one character of the default L2 font; Size is left alone for a character the
// font lacks
void NCGDevice::GetCharSize(SIZE* Size, TCHAR C)
{
    FL2FontChar* Char = GCanvas->L2Fonts[L2FONT_Normal]->GetChar(C);
    if (Char)
    {
        Size->cx = Char->Width;
        Size->cy = Char->Height;
    }
}
NW_HOOK_FUNC(0x1000c700, NCGDevice::GetCharSize)

// 0x1000c730 - measures with the viewport canvas, not with a device
DWORD NCGDevice::GetTextExtent(SIZE* Size, const TCHAR* Text, DWORD P3, DWORD P4, UBOOL P5, INT P7, DWORD P8)
{
    return GCanvas->GetTextExtent(Text, Size, P3, P4, P5 ? 1 : 0, L2FONT_Normal, P7, P8);
}
NW_HOOK_FUNC(0x1000c730, NCGDevice::GetTextExtent)

// 0x1000c770 - English clients (GLanguageType 1) use Engine's FL2TextAlignParser; the others
// NWindow's own wrapper over static buffers, which measures each character with the L2 font
// (newlines and control characters are skipped, not line breaks)
TCHAR* NCGDevice::FirstWrappedLine(const TCHAR* Text, DWORD MaxWidth, INT FontType)
{
    if (!Text || !GCanvas)
        return NULL;
    UL2Font* Font = GCanvas->L2Fonts[FontType];
    if (!Font)
        return NULL;
    if (GLanguageType == 1 && GTextAlignParser)
    {
        INT LineStart = 0;   // GetLineBuffer's out value: the offset in Text where the line starts (not a length)
        GTextAlignParser->Clear();
        GTextAlignParser->SetParserParam((TCHAR*)Text, MaxWidth, Font, 0, 0, 1);
        return GTextAlignParser->GetLineBuffer(LineStart);
    }
    wcscpy(GWrapText, Text);
    GWrapLen = (INT)wcslen(GWrapText);
    GWrapPos = 0;
    GWrapLine[0] = 0;
    INT Count = 0;
    DWORD LineWidth = 0;
    if (GWrapText[0])
    {
        const TCHAR* p = GWrapText;
        do
        {
            TCHAR C = *p++;
            if (GWrapPos >= GWrapLen)
                break;
            if (C == 0x0A || C < ' ')
                GWrapPos++;
            else
            {
                FL2FontChar* Char = Font->GetChar(C);
                if (Char)
                {
                    LineWidth = (DWORD)((DOUBLE)Char->Width + (DOUBLE)LineWidth);
                    if (LineWidth > MaxWidth)
                        break;
                }
                GWrapPos++;
                GWrapLine[Count++] = C;
            }
        } while (*p);
    }
    GWrapLine[Count] = 0;
    return Count ? GWrapLine : NULL;
}
NW_HOOK_FUNC(0x1000c770, NCGDevice::FirstWrappedLine)

// 0x1000c8f0 - continues where the previous line stopped
TCHAR* NCGDevice::NextWrappedLine(DWORD MaxWidth, INT FontType)
{
    if (!GCanvas)
        return NULL;
    if (GLanguageType == 1 && GTextAlignParser)
    {
        INT LineStart = 0;   // GetLineBuffer's out value: the offset in Text where the line starts (not a length)
        GTextAlignParser->SetWidthParam(MaxWidth);
        return GTextAlignParser->GetLineBuffer(LineStart);
    }
    if (GWrapPos >= GWrapLen)
        return NULL;
    GWrapLine[0] = 0;
    UL2Font* Font = GCanvas->L2Fonts[FontType];
    INT Count = 0;
    DWORD LineWidth = 0;
    const TCHAR* p = &GWrapText[GWrapPos];
    if (!Font)
        return NULL;
    for (TCHAR C = *p; C; C = *p)
    {
        p++;
        if (GWrapPos >= GWrapLen)
            break;
        if (C == 0x0A || C < ' ')
            GWrapPos++;
        else
        {
            FL2FontChar* Char = Font->GetChar(C);
            if (Char)
            {
                LineWidth = (DWORD)((DOUBLE)Char->Width + (DOUBLE)LineWidth);
                if (LineWidth > MaxWidth)
                    break;
            }
            GWrapPos++;
            GWrapLine[Count++] = C;
        }
    }
    GWrapLine[Count] = 0;
    return Count ? GWrapLine : NULL;
}
NW_HOOK_FUNC(0x1000c8f0, NCGDevice::NextWrappedLine)

// 0x1000cb00
DWORD NCGDevice::DrawText(INT X, INT Y, DWORD Color, const TCHAR* Text, DWORD P5, DWORD P6, DWORD P7,
                          L2FontType FontType, INT P9, INT P10, DWORD P11, INT P12, INT P13, INT P14)
{
    if (!Canvas)
        return 0;
    if (!Text)
        return 0;
    SIZE Size;
    GetTextExtent(&Size, Text, P5, P6, X, 0, 0);  // X really is the 5th argument in the stock code
    LineHeight = Size.cy;
    Touch(OriginY + Y, Size.cy);
    return Canvas->DrawText(OriginX + X, OriginY + Y, Color, Text, 0, P5, P6, 0.f, P7, FontType, P9, P10, P11,
                            P12, P13, P14, NULL);
}
NW_HOOK_METHOD(0x1000cb00, NCGDevice, DrawText)

// 0x1000cf30
void NCGDevice::Draw3DCoordText(FVector Loc, DWORD Color, TCHAR* Text, UTexture* Tex, INT P5, INT P6,
                                L2FontType FontType)
{
    guard(NCGDevice::Draw3DCoordText);
    Canvas->Draw3DCoordText(Loc, Color, Text, Tex, P5, P6, FontType, 0);
    unguard;
}
NW_HOOK_METHOD(0x1000cf30, NCGDevice, Draw3DCoordText)

// 0x1000d120 - digits from a texture strip: 5 px wide small set or 8 px wide large set
DWORD NCGDevice::DrawSpecialDigit(UBOOL bLarge, INT X, INT Y, DWORD Color, TCHAR* Text)
{
    if (!Text || !Canvas)
        return 0;
    if (!bLarge)
        return Canvas->DrawSpecialDigit(5, OriginX + X, OriginY + Y, Color, Text, SmallDigits);
    return Canvas->DrawSpecialDigit(8, OriginX + X, OriginY + Y, Color, Text, LargeDigits);
}
NW_HOOK_METHOD(0x1000d120, NCGDevice, DrawSpecialDigit)

/*-----------------------------------------------------------------------------
    Shapes and textures
-----------------------------------------------------------------------------*/

// 0x1000cbd0
void NCGDevice::DrawRect(INT X, INT Y, INT XL, INT YL, DWORD Color)
{
    UTexture* White = FindTexture(TEXT("NWindow.WhiteTexture"));
    Canvas->DrawRect(OriginX + X, OriginY + Y, XL, YL, White, Color);
    Touch(OriginY + Y, YL);
}
NW_HOOK_METHOD(0x1000cbd0, NCGDevice, DrawRect)

// 0x1000cc40
void NCGDevice::DrawStretchedTex(INT X, INT Y, INT XL, INT YL, UTexture* Tex)
{
    if (Tex && (Tex->USize || Tex->VSize))
    {
        Canvas->DrawStretchedTex(OriginX + X, OriginY + Y, XL, YL, Tex);
        Touch(OriginY + Y, YL);
    }
}
NW_HOOK_METHOD_SIG(0x1000cc40, NCGDevice, DrawStretchedTex, void (NCGDevice::*)(INT, INT, INT, INT, UTexture*))

// 0x1000ccb0
void NCGDevice::DrawStretchedTex(INT X, INT Y, INT XL, INT YL, INT SrcX, INT SrcY, INT SrcXL, INT SrcYL,
                                 UTexture* Tex)
{
    if (Tex && (Tex->USize || Tex->VSize))
    {
        Canvas->DrawStretchedTex(OriginX + X, OriginY + Y, XL, YL, SrcX, SrcY, SrcXL, SrcYL, Tex);
        Touch(OriginY + Y, YL);
    }
}
NW_HOOK_METHOD_SIG(0x1000ccb0, NCGDevice, DrawStretchedTex,
                   void (NCGDevice::*)(INT, INT, INT, INT, INT, INT, INT, INT, UTexture*))

// 0x1000cd30
void NCGDevice::DrawTexture(INT X, INT Y, INT XL, INT YL, INT U, INT V, INT UL, INT VL, UTexture* Tex)
{
    if (Tex && (Tex->USize || Tex->VSize))
    {
        Canvas->DrawTexture(OriginX + X, OriginY + Y, XL, YL, (FLOAT)U, (FLOAT)V, (FLOAT)UL, (FLOAT)VL, Tex);
        Touch(OriginY + Y, YL);
    }
}
NW_HOOK_METHOD(0x1000cd30, NCGDevice, DrawTexture)

// 0x1000cdc0
void NCGDevice::DrawBrightenTexture(INT X, INT Y, INT XL, INT YL, INT U, INT V, INT UL, INT VL, UTexture* Tex,
                                    DWORD Brightness)
{
    if (Tex && (Tex->USize || Tex->VSize))
    {
        Canvas->DrawBrightenTexture(OriginX + X, OriginY + Y, XL, YL, (FLOAT)U, (FLOAT)V, (FLOAT)UL, (FLOAT)VL,
                                    Tex, Brightness);
        Touch(OriginY + Y, YL);
    }
}
NW_HOOK_METHOD(0x1000cdc0, NCGDevice, DrawBrightenTexture)

// 0x1000ceb0
void NCGDevice::DrawLine(INT X1, INT Y1, INT X2, INT Y2, DWORD Color, INT Width)
{
    Canvas->DrawLine(OriginX + X1, OriginY + Y1, OriginX + X2, OriginY + Y2, Color, Width);
    Touch(OriginY + Y1, 0);
    Touch(OriginY + Y2, 0);
}
NW_HOOK_METHOD(0x1000ceb0, NCGDevice, DrawLine)

// 0x1000cfe0 - clan crest, 16x12 taken at (0,4) of the crest texture
UBOOL NCGDevice::DrawPledgeCrest(INT X, INT Y, INT CrestID)
{
    if (CrestID <= 0)
        return 0;
    UTexture* Crest = GConsole->GetPledgeCrestTexture(CrestID);
    if (!Crest || (!Crest->USize && !Crest->VSize))
        return 0;
    Canvas->DrawTexture(OriginX + X, OriginY + Y, 16, 12, 0.f, 4.f, 16.f, 12.f, Crest);
    Touch(OriginY + Y, 12);
    return 1;
}
NW_HOOK_METHOD(0x1000cfe0, NCGDevice, DrawPledgeCrest)

// 0x1000d070 - alliance crest, 8x12
UBOOL NCGDevice::DrawAllyCrest(INT X, INT Y, INT CrestID)
{
    if (CrestID <= 0)
        return 0;
    UTexture* Crest = GConsole->GetAllyCrestTexture(CrestID);
    if (!Crest || (!Crest->USize && !Crest->VSize))
        return 0;
    Canvas->DrawTexture(OriginX + X, OriginY + Y, 8, 12, 0.f, 4.f, 8.f, 12.f, Crest);
    Touch(OriginY + Y, 12);
    return 1;
}
NW_HOOK_METHOD(0x1000d070, NCGDevice, DrawAllyCrest)

/*-----------------------------------------------------------------------------
    Canvas passthroughs
-----------------------------------------------------------------------------*/

// 0x1000ce60
void NCGDevice::DrawMinimapLoc(INT X, INT Y, UTexture* Tex)
{
    Canvas->DrawMinimapLoc(OriginX + X, OriginY + Y, Tex);
}
NW_HOOK_METHOD(0x1000ce60, NCGDevice, DrawMinimapLoc)

// 0x1000ce90
void NCGDevice::DrawRadar()
{
    Canvas->DrawRadar();
}
NW_HOOK_METHOD(0x1000ce90, NCGDevice, DrawRadar)

// 0x1000cea0 - radar coordinates are absolute (no origin)
void NCGDevice::DrawRadarTarget(INT X, INT Y)
{
    Canvas->DrawRadarTarget(X, Y);
}
NW_HOOK_METHOD(0x1000cea0, NCGDevice, DrawRadarTarget)

// 0x1000d100 - clip rectangles are absolute (no origin)
void NCGDevice::SetClip(INT X, INT Y, INT XL, INT YL)
{
    Canvas->SetClip(X, Y, XL, YL);
}
NW_HOOK_METHOD(0x1000d100, NCGDevice, SetClip)

// 0x1000d110
void NCGDevice::ReleaseClip()
{
    Canvas->ReleaseClip();
}
NW_HOOK_METHOD(0x1000d110, NCGDevice, ReleaseClip)

/*-----------------------------------------------------------------------------
    Tool tip box (no stock function: inlined in NCButton::OnDrawToolTip 0x10001dd0
    and NCIcon::OnDrawToolTip 0x10015910).
-----------------------------------------------------------------------------*/

// Clip, background and text are placed relative to the device origin (the window): above it
// (Y = -height) or below it (Y = Wnd->Height), moved left when the box would leave the screen.
void NCGDevice::DrawToolTipBox(NCWnd* Wnd, const TCHAR* Text, UTexture* Back, UBOOL bAbove)
{
    SIZE Size;
    GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    INT ScreenW = GConsole->Desktop->LocalW;
    INT X = 0;
    INT ClipX = Wnd->ScreenX;
    if (Wnd->ScreenX + Size.cx > ScreenW)
    {
        X = ScreenW - Size.cx - Wnd->ScreenX;
        ClipX = ScreenW - Size.cx;
    }
    INT Y = bAbove ? -Size.cy : Wnd->Height;
    SetClip(ClipX, Wnd->ScreenY + Y, Size.cx, Size.cy);
    if (Back)
        DrawStretchedTex(X, Y, Size.cx, Size.cy, Back);
    DrawText(X, Y, 0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    ReleaseClip();
}

/*-----------------------------------------------------------------------------
    Origin stack
-----------------------------------------------------------------------------*/

// 0x1000dd30
void NCGDevice::PushOrigin(INT DX, INT DY)
{
    POINT Move;
    Move.x = DX;
    Move.y = DY;
    Origins.PushBack(Move);
    OriginX += Move.x;
    OriginY += Move.y;
}
NW_HOOK_METHOD(0x1000dd30, NCGDevice, PushOrigin)

// 0x1000d7b0 - the last move undone (nothing checks that there is one)
void NCGDevice::PopOrigin()
{
    POINT& Move = Origins.Back();
    OriginX -= Move.x;
    OriginY -= Move.y;
    Origins.PopBack();
}
NW_HOOK_METHOD(0x1000d7b0, NCGDevice, PopOrigin)

// 0x1000ca30 - DrawText's (see there) with the ranges and the three arguments the canvas's
// colored text takes (1, the origin's move since BeginPaint, 1)
DWORD NCGDevice::DrawColoredLine(INT X, INT Y, DWORD Color, const TCHAR* Text, DWORD P5, DWORD P6,
                                 DWORD P7, L2FontType FontType, INT P9, INT P10, DWORD P11,
                                 TArray<FL2ColorFontInfo>* ColorInfo)
{
    if (!Canvas)
        return 0;
    if (!Text)
        return 0;
    SIZE Size;
    GetTextExtent(&Size, Text, P5, P6, X, 0, 0);  // X really is the 5th argument, as in DrawText
    LineHeight = Size.cy;
    Touch(OriginY + Y, Size.cy);
    return Canvas->DrawText(OriginX + X, OriginY + Y, Color, Text, 0, P5, P6, 0.f, P7, FontType, P9, P10,
                            P11, 1, OriginX - Unknown5C, 1, ColorInfo);
}
NW_HOOK_METHOD(0x1000ca30, NCGDevice, DrawColoredLine)

NW_CHECK_VTABLE(NCGDevice, 0x100d9e40, 24)
