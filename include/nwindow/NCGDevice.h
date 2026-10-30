#pragma once
// NCGDevice - the paint device handed to every OnPaint: a UCanvas plus the window origin
// and the vertical range touched since the last reset (UClass "CGDevice", sizeof 0x60).
// Each Draw* forwards to the UCanvas method of the same name with the origin applied.

#include "nwindow/NCObject.h"
#include "nwindow/NStdLib.h"

class NCWnd;

// state of the non-English line wrapper (names ours)
typedef TCHAR NWrapBuffer[2048];
#if NW_HOOK_MODE
#define GWrapText NW_GLOBAL(NWrapBuffer, 0x1011c4c0)   // copy of the text being wrapped
#define GWrapLine NW_GLOBAL(NWrapBuffer, 0x1011d4c0)   // the line returned
#define GWrapLen  NW_GLOBAL(INT, 0x1011e4c0)
#define GWrapPos  NW_GLOBAL(INT, 0x1011e4c4)
#else
// references into one object that keeps the stock order (src/standalone/NwGlobals.cpp): an
// overlong text runs from GWrapText into GWrapLine, GWrapLen and GWrapPos (FirstWrappedLine's
// wcscpy), as in the stock image
extern NWrapBuffer& GWrapText;
extern NWrapBuffer& GWrapLine;
extern INT& GWrapLen;
extern INT& GWrapPos;
#endif

class NCGDevice : public NCObject
{
    NW_DECLARE_CLASS(NCGDevice, NCObject, 0, 0x1011e4c8)

    UCanvas* Canvas;            // +0x28
    INT OriginX;                // +0x2C
    INT OriginY;                // +0x30
    NPointDeque Origins;        // +0x34 the moves of PushOrigin, undone by PopOrigin
    INT DirtyTop;               // +0x48 smallest screen Y drawn
    INT DirtyBottom;            // +0x4C largest screen Y drawn
    INT LineHeight;             // +0x50 height of the last text measured by DrawText
    UTexture* SmallDigits;      // +0x54 "SEK.cbui13"
    UTexture* LargeDigits;      // +0x58 "L2UI.NWindow.Number"
    INT Unknown5C;              // +0x5C OriginX again (BeginPaint)

    NCGDevice();                // 0x1000d6a0
    // ~NCGDevice               // 0x1000d750

    virtual void Destroy();     // 0x1000d220 (vslot 10)

    void Init();                                                             // 0x1000d180 (name ours)
    UBOOL SetCanvas(UCanvas* InCanvas);                                      // 0x1000ca10 (name ours)
    void ClearCanvas();                                                      // 0x1000ca20 (name ours)
    static void GetCharSize(SIZE* Size, TCHAR C);                           // 0x1000c700 (name ours)
    static DWORD GetTextExtent(SIZE* Size, const TCHAR* Text, DWORD P3, DWORD P4, UBOOL P5,
                               INT P7, DWORD P8);                           // 0x1000c730 (name ours)
    // line wrapping: the first line of Text that fits MaxWidth pixels, then the next ones
    // (names ours); NULL when there is nothing left
    static TCHAR* FirstWrappedLine(const TCHAR* Text, DWORD MaxWidth, INT FontType); // 0x1000c770
    static TCHAR* NextWrappedLine(DWORD MaxWidth, INT FontType);            // 0x1000c8f0

    DWORD DrawText(INT X, INT Y, DWORD Color, const TCHAR* Text, DWORD P5, DWORD P6, DWORD P7,
                   L2FontType FontType, INT P9, INT P10, DWORD P11, INT P12, INT P13,
                   INT P14);                                                 // 0x1000cb00
    void DrawRect(INT X, INT Y, INT XL, INT YL, DWORD Color);                // 0x1000cbd0
    void DrawStretchedTex(INT X, INT Y, INT XL, INT YL, UTexture* Tex);      // 0x1000cc40
    void DrawStretchedTex(INT X, INT Y, INT XL, INT YL, INT SrcX, INT SrcY, INT SrcXL, INT SrcYL,
                          UTexture* Tex);                                    // 0x1000ccb0
    void DrawTexture(INT X, INT Y, INT XL, INT YL, INT U, INT V, INT UL, INT VL,
                     UTexture* Tex);                                         // 0x1000cd30
    void DrawBrightenTexture(INT X, INT Y, INT XL, INT YL, INT U, INT V, INT UL, INT VL,
                             UTexture* Tex, DWORD Brightness);               // 0x1000cdc0
    void DrawMinimapLoc(INT X, INT Y, UTexture* Tex);                        // 0x1000ce60
    void DrawRadar();                                                        // 0x1000ce90
    void DrawRadarTarget(INT X, INT Y);                                      // 0x1000cea0
    void DrawLine(INT X1, INT Y1, INT X2, INT Y2, DWORD Color, INT Width);   // 0x1000ceb0
    void Draw3DCoordText(FVector Loc, DWORD Color, TCHAR* Text, UTexture* Tex, INT P5, INT P6,
                         L2FontType FontType);                               // 0x1000cf30
    UBOOL DrawPledgeCrest(INT X, INT Y, INT CrestID);                        // 0x1000cfe0 (name ours)
    UBOOL DrawAllyCrest(INT X, INT Y, INT CrestID);                          // 0x1000d070 (name ours)
    void SetClip(INT X, INT Y, INT XL, INT YL);                              // 0x1000d100
    void ReleaseClip();                                                      // 0x1000d110
    DWORD DrawSpecialDigit(UBOOL bLarge, INT X, INT Y, DWORD Color, TCHAR* Text); // 0x1000d120
    // names ours: the origin moved by (DX, DY) (a POINT by value in the stock code) and back
    void PushOrigin(INT DX, INT DY);                                         // 0x1000dd30
    void PopOrigin();                                                        // 0x1000d7b0
    // DrawText with the canvas's color ranges (the HTML viewer's colored words; name ours)
    DWORD DrawColoredLine(INT X, INT Y, DWORD Color, const TCHAR* Text, DWORD P5, DWORD P6,
                          DWORD P7, L2FontType FontType, INT P9, INT P10, DWORD P11,
                          TArray<FL2ColorFontInfo>* ColorInfo);              // 0x1000ca30

    // Every OnPaint / OnDrawToolTip opens and closes with these (inlined in the stock code,
    // names ours): from BeginPaint on, Draw* coordinates are relative to (X, Y).
    void BeginPaint(INT X, INT Y)
    {
        OriginY = Y;
        OriginX = X;
        Unknown5C = X;
        DirtyTop = OriginY;
        DirtyBottom = 0;
    }
    void EndPaint()
    {
        OriginX = 0;
        OriginY = 0;
    }

    // Tool tip box of buttons and icons (inlined in each OnDrawToolTip of the stock code,
    // name ours): Text on Back, above or below Wnd, pushed left to stay on the screen.
    void DrawToolTipBox(NCWnd* Wnd, const TCHAR* Text, UTexture* Back, UBOOL bAbove);

private:
    // the dirty-range update every Draw* repeats inline in the stock code
    void Touch(INT ScreenY, INT Height)
    {
        if (ScreenY + Height > DirtyBottom)
            DirtyBottom = ScreenY + Height;
        if (ScreenY < DirtyTop)
            DirtyTop = ScreenY;
    }
};
NW_ASSERT_OFFSET(NCGDevice, Canvas, 0x28);
NW_ASSERT_OFFSET(NCGDevice, Origins, 0x34);
NW_ASSERT_OFFSET(NCGDevice, DirtyTop, 0x48);
NW_ASSERT_OFFSET(NCGDevice, LineHeight, 0x50);
NW_ASSERT_OFFSET(NCGDevice, LargeDigits, 0x58);
NW_ASSERT_SIZE(NCGDevice, 0x60);
