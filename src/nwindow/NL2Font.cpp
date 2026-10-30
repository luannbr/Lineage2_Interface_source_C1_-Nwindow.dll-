/*=============================================================================
    NL2Font.cpp - L2 bitmap font. Stock: 0x1005b770 .. 0x1005bc37.
=============================================================================*/
#include "nwindow/NL2Font.h"

/*-----------------------------------------------------------------------------
    FL2FontPage.
-----------------------------------------------------------------------------*/

// 0x1005b820
FL2FontPage::FL2FontPage()
{
    First = 0;
    Count = 0;
    Chars = NULL;
}
NW_HOOK_CTOR(0x1005b820, FL2FontPage)

// 0x1005b990
FL2FontPage::~FL2FontPage()
{
    Free();
}
NW_HOOK_DTOR(0x1005b990, FL2FontPage)

// 0x1005b770 - the stock NL2Font::GetChar keeps ecx across this call (see tools/regcheck.py):
// disable both in NWindowDecomp.ini or neither
FL2FontChar* FL2FontPage::Find(TCHAR C)
{
    INT Index = (_WORD)C;
    if (Index >= First && Index < First + Count)
        return &Chars[Index - First];
    return NULL;
}
NW_HOOK_METHOD(0x1005b770, FL2FontPage, Find)

// 0x1005b8e0 - an empty page fails the whole font (Load returns -1)
UBOOL FL2FontPage::Load(INT InFirst, INT InCount, FILE* File)
{
    if (!InCount)
        return 0;
    First = InFirst;
    Count = InCount;
    Chars = new FL2FontChar[Count];
    for (INT i = 0; i < Count; i++)
        GFontBytesRead += (INT)fread(&Chars[i], sizeof(FL2FontChar), 1, File);
    return 1;
}
NW_HOOK_METHOD(0x1005b8e0, FL2FontPage, Load)

// 0x1005b960
void FL2FontPage::Free()
{
    if (Chars)
    {
        delete[] Chars;
        Chars = NULL;
    }
}
NW_HOOK_METHOD(0x1005b960, FL2FontPage, Free)

/*-----------------------------------------------------------------------------
    NL2Font.
-----------------------------------------------------------------------------*/

// 0x1005b7a0
NL2Font::NL2Font()
{
    Texture = NULL;
    Unknown30 = 0;
    Unknown34 = 0;
    NumPages = 0;
    Pages = NULL;
}
NW_HOOK_CTOR(0x1005b7a0, NL2Font)

// 0x1005b880 - ConditionalDestroy, then Engine's ~UL2Font
NW_HOOK_DTOR(0x1005b880, NL2Font)

// 0x1005b9a0 (vslot 25) - 0 = loaded, -1 = no file or an empty page (the file stays open then,
// as in the stock code)
INT NL2Font::Load(const TCHAR* TextureName, const TCHAR* FontFile)
{
    Texture = (UTexture*)StaticFindObject(UTexture::StaticClass(), ANY_PACKAGE, TextureName, 0);
    if (!Texture)
        Texture = (UTexture*)StaticLoadObject(UTexture::StaticClass(), NULL, TextureName, NULL, 1,
                                              NULL);
    if (Texture)
        Texture->SetFlags(RF_Native);

    FILE* File;
    if (GUnicodeOS)
        File = _wfopen(FontFile, L"rb");
    else
        File = fopen(TCHAR_TO_ANSI(FontFile), "rb");
    if (!File)
        return -1;

    GFontBytesRead += (INT)fread(&Unknown30, 4, 1, File);
    GFontBytesRead += (INT)fread(&Unknown34, 4, 1, File);
    GFontBytesRead += (INT)fread(&NumPages, 4, 1, File);
    if (NumPages)
    {
        Pages = new FL2FontPage[NumPages];
        for (INT i = 0; i < NumPages; i++)
        {
            INT PageFirst, PageCount;
            GFontBytesRead += (INT)fread(&PageFirst, 4, 1, File);
            GFontBytesRead += (INT)fread(&PageCount, 4, 1, File);
            if (!Pages[i].Load(PageFirst, PageCount, File))
                return -1;
        }
    }
    fclose(File);
    return 0;
}
NW_HOOK_METHOD(0x1005b9a0, NL2Font, Load)

// 0x1005b840 (vslot 26)
NW_HOOK_METHOD(0x1005b840, NL2Font, GetTexture)

// 0x1005b7e0 (vslot 27) - the first page holding a glyph with a size
FL2FontChar* NL2Font::GetChar(TCHAR C)
{
    for (INT i = 0; i < NumPages; i++)
    {
        FL2FontChar* Char = Pages[i].Find(C);
        if (Char && Char->Width > 0 && Char->Height > 0)
            return Char;
    }
    return NULL;
}
NW_HOOK_METHOD(0x1005b7e0, NL2Font, GetChar)

// 0x1005bbd0 (vslot 28)
void NL2Font::Clear()
{
    for (INT i = 0; i < NumPages; i++)
        Pages[i].Free();
    if (Pages)
    {
        delete[] Pages;
        Pages = NULL;
    }
    if (Texture)
        Texture->ClearFlags(RF_Native);
}
NW_HOOK_METHOD(0x1005bbd0, NL2Font, Clear)

NW_CHECK_VTABLE_FOR(NL2Font, UObject, 0x100e39b8, 29)
