#pragma once
// NL2Font - L2's bitmap font (UClass "L2Font", 0x40; implements Engine's UL2Font, not an
// NCObject). Load reads the glyph table file: two header ints, the page count, then per page
// its first character, its size and that many 16-byte glyphs; the glyphs index into Texture.

#include "nwindow/NWindow.h"

#include <stdio.h>

// one glyph of the font file (field names tentative: the wrap code uses Width as the advance)
struct FL2FontChar
{
    INT X;
    INT Width;
    INT Y;
    INT Height;
};

// a run of consecutive characters (names ours)
struct FL2FontPage
{
    INT First;
    INT Count;
    FL2FontChar* Chars;

    FL2FontPage();                                          // 0x1005b820
    ~FL2FontPage();                                         // 0x1005b990 (jmp Free)
    FL2FontChar* Find(TCHAR C);                             // 0x1005b770
    UBOOL Load(INT InFirst, INT InCount, FILE* File);       // 0x1005b8e0
    void Free();                                            // 0x1005b960
};
NW_ASSERT_SIZE(FL2FontPage, 0xC);

#if NW_HOOK_MODE
#define GFontBytesRead NW_GLOBAL(INT, 0x1012f190)   // items fread by the font loader (name ours)
#else
extern INT GFontBytesRead;
#endif

class NL2Font : public UL2Font
{
    NW_DECLARE_CLASS(NL2Font, UL2Font, CLASS_Config, 0x1012f198)

    UTexture* Texture;         // +0x2C
    INT Unknown30;             // +0x30 first int of the font file
    INT Unknown34;             // +0x34 second int
    INT NumPages;              // +0x38
    FL2FontPage* Pages;        // +0x3C new[]

    NL2Font();                                                             // 0x1005b7a0
    // ~NL2Font                                                            // 0x1005b880

    virtual UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar) { return 0; }  // FExec 0x100016f0 (folded)
    virtual INT Load(const TCHAR* TextureName, const TCHAR* FontFile);     // 25 0x1005b9a0
    virtual UTexture* GetTexture() { return Texture; }                     // 26 0x1005b840
    virtual FL2FontChar* GetChar(TCHAR C);                                 // 27 0x1005b7e0
    virtual void Clear();                                                  // 28 0x1005bbd0
};
NW_ASSERT_OFFSET(NL2Font, Texture, 0x2C);
NW_ASSERT_OFFSET(NL2Font, Pages, 0x3C);
NW_ASSERT_SIZE(NL2Font, 0x40);
