#pragma once
// The page model of NCHtmlViewer (all names ours: no class of this module has a UClass or a
// guard string). NHtmlParser turns the text into a list of tags (NHtmlTag); the viewer builds
// a tree from them: NHtmlBlock stacks its children one under the other (the page, a table
// cell), NHtmlTable lays cells out in rows and columns, NHtmlItem is a leaf (a run of text, an
// image, a line break, or a control the viewer creates: link, button, edit, combo box). Each
// object sizes itself (Layout), places its children (Arrange) and draws them (Draw) relative to
// its parent.
//
// The parser and the item's strings use the VC 7.1 STL of the stock build (std::vector,
// std::wstring), mirrored without the STL in NStl.h.

#include "nwindow/NWindow.h"
#include "nwindow/NStl.h"

class NCHtmlViewer;
class NCGDevice;

// tag ids of NHtmlTag::Tag (the parser maps its alphabetical tag table to these)
enum
{
    HTMLTAG_Text = 1,          // text between tags (NHtmlTag::Text)
    HTMLTAG_Unknown = 2,
    HTMLTAG_Comment = 3,
    HTMLTAG_Extend = 4,
    HTMLTAG_Img = 5,
    HTMLTAG_Input = 6,
    HTMLTAG_Select = 7,
    HTMLTAG_Option = 8,
    HTMLTAG_TextArea = 9,
    HTMLTAG_Spin = 10,
    HTMLTAG_Bar = 11,
    HTMLTAG_Volumn = 12,
    HTMLTAG_Var = 13,
    HTMLTAG_A = 14,
    HTMLTAG_Br = 15,
    HTMLTAG_Left = 16,
    HTMLTAG_Right = 17,
    HTMLTAG_Center = 18,
    HTMLTAG_Font = 19,
    HTMLTAG_B = 20,
    HTMLTAG_I = 21,
    HTMLTAG_U = 22,
    HTMLTAG_TT = 23,
    HTMLTAG_Strike = 24,
    HTMLTAG_Sub = 25,
    HTMLTAG_Sup = 26,
    HTMLTAG_P = 27,
    HTMLTAG_H1 = 28,           // .. 34 H7
    HTMLTAG_Pre = 35,
    HTMLTAG_Address = 36,
    HTMLTAG_UL = 37,
    HTMLTAG_OL = 38,
    HTMLTAG_LI = 39,
    HTMLTAG_Table = 40,
    HTMLTAG_TR = 41,
    HTMLTAG_TD = 42,
    HTMLTAG_Html = 43,
    HTMLTAG_Body = 44,
    HTMLTAG_Head = 45,
    HTMLTAG_Title = 46,
    HTMLTAG_Button = 49,
    HTMLTAG_MultiEdit = 50,
    HTMLTAG_Edit = 51,
    HTMLTAG_ComboBox = 52,
};

// attribute ids: 1-based index in the tag's sorted attribute list
enum { ATTR_A_Action = 1, ATTR_A_Href = 2, ATTR_A_Link = 3, ATTR_A_Msg = 4 };
enum { ATTR_Html_ImgSrc = 2 };
enum { ATTR_Table_BgColor = 3, ATTR_Table_Border = 4, ATTR_Table_Width = 9 };
enum { ATTR_TD_Align = 1, ATTR_TD_FixWidth = 5, ATTR_TD_Height = 6, ATTR_TD_VAlign = 7, ATTR_TD_Width = 8 };
enum { ATTR_Img_Height = 1, ATTR_Img_Src = 2, ATTR_Img_Width = 4 };
enum { ATTR_Button_Action = 1, ATTR_Button_Back = 2, ATTR_Button_Fore = 3, ATTR_Button_Height = 4,
       ATTR_Button_Value = 5, ATTR_Button_Width = 6 };
enum { ATTR_MultiEdit_Height = 1, ATTR_MultiEdit_Var = 2, ATTR_MultiEdit_Width = 3 };
enum { ATTR_Edit_Action = 1, ATTR_Edit_Height = 2, ATTR_Edit_Type = 3, ATTR_Edit_Var = 4,
       ATTR_Edit_Width = 5 };
enum { ATTR_Combo_List = 1, ATTR_Combo_Sel = 2, ATTR_Combo_Var = 3, ATTR_Combo_Width = 4 };

// NHtmlObject::HAlign / VAlign
enum { HTMLALIGN_None = 0, HTMLALIGN_Center = 1, HTMLALIGN_Left = 2, HTMLALIGN_Right = 3 };
enum { HTMLVALIGN_None = 0, HTMLVALIGN_Center = 1, HTMLVALIGN_Top = 2, HTMLVALIGN_Bottom = 3 };

// NHtmlItem::Mode (NHtmlBlock::Mode holds it for the next item)
enum
{
    HTMLITEM_Text = 0,
    HTMLITEM_FileLink = 1,     // <a link=...>: loads another page (NCFileLinker)
    HTMLITEM_ActionLink = 2,   // <a action=... msg=...> (NCActionLinker)
    HTMLITEM_Button = 3,
    HTMLITEM_MultiEdit = 4,
    HTMLITEM_Edit = 5,
    HTMLITEM_RedText = 6,      // drawn red; nothing in this module sets it
    HTMLITEM_Image = 7,
    HTMLITEM_ComboBox = 8,
};

// Parser state shared with the tree while it is built (stock globals, names ours): the
// horizontal alignment stack (<center>, <td align>) and the target / message of the <a> being
// read (256 characters each; the viewer's command handlers reuse the first as their buffer).
#if NW_HOOK_MODE
#define GHtmlAlignStack  NW_GLOBAL(TArray<INT>, 0x10126a9c)
#define GHtmlVAlignStack NW_GLOBAL(TArray<INT>, 0x101255f8)   // only ever holds its 0
#define GHtmlLinkTarget  (reinterpret_cast<TCHAR*>(::nw::Addr(0x101251f8)))
#define GHtmlLinkMsg     (reinterpret_cast<TCHAR*>(::nw::Addr(0x101253f8)))
#else
extern TArray<INT> GHtmlAlignStack;
// references into one object that keeps the stock order (src/standalone/NwGlobals.cpp): an
// overlong link target runs into the message and the message into GHtmlVAlignStack (the wcscpy
// of BuildTree and of the viewers' handlers), as in the stock image
extern TCHAR (&GHtmlLinkTarget)[256];
extern TCHAR (&GHtmlLinkMsg)[256];
extern TArray<INT>& GHtmlVAlignStack;
#endif

/*-----------------------------------------------------------------------------
    The parser (src/nwindow/NHtmlParser.cpp).
-----------------------------------------------------------------------------*/

// One tag or text run from NHtmlParser (0x60 bytes). The attributes are two parallel vectors:
// ids (1-based index in the tag's attribute list) and values.
struct NHtmlTag
{
    INT Tag;                   // +0x00 HTMLTAG_* (not set by the constructor)
    BYTE bOpen;                // +0x04 1 = <tag>, 0 = </tag> (not set by the constructor)
    NStlWString Text;          // +0x08 HTMLTAG_Text: the text; comments, unknown tags and
                               //   <extend>: their source
    NStlWordVector AttrIds;    // +0x24
    NStlWStringVector AttrValues; // +0x34
    NStlWString Colors;        // +0x44 HTMLTAG_Text: the <font color> runs, "color\x1Eword\x1E...";
                               //   other tags: the last attribute not in their list, "name=value"

    NHtmlTag() {}                                                 // 0x1002f260
    ~NHtmlTag() {}                                                // 0x1002f170
    const TCHAR* GetAttr(INT Attr, UBOOL bUnused);                // 0x10025d70 NULL = absent
    bool HasAttr(INT Attr);                                       // 0x10025dd0 (al only)
};
NW_ASSERT_OFFSET(NHtmlTag, Text, 0x08);
NW_ASSERT_OFFSET(NHtmlTag, AttrIds, 0x24);
NW_ASSERT_OFFSET(NHtmlTag, AttrValues, 0x34);
NW_ASSERT_OFFSET(NHtmlTag, Colors, 0x44);
NW_ASSERT_SIZE(NHtmlTag, 0x60);

// std::vector<NHtmlTag*> made by NHtmlParser::Parse
struct NHtmlTagList : NStlVector<NHtmlTag*>
{
};
NW_ASSERT_SIZE(NHtmlTagList, 0x10);

// The tag names the parser knows, in alphabetical order, each with its sorted attribute names
// (stock table, 50 searched), and the HTMLTAG_* id of each (names ours).
struct NHtmlTagInfo
{
    const TCHAR* Name;
    const TCHAR* const* Attrs;
    INT NumAttrs;
};
#if NW_HOOK_MODE
#define GHtmlTagInfo (reinterpret_cast<const NHtmlTagInfo*>(::nw::Addr(0x1010ba90)))
#define GHtmlTagIds  (reinterpret_cast<const INT*>(::nw::Addr(0x1010c1c8)))
#else
extern const NHtmlTagInfo GHtmlTagInfo[];
extern const INT GHtmlTagIds[];
#endif

// Name lookups in GHtmlTagInfo (4 bytes: a vtable only)
class NHtmlTagTable
{
public:
    NHtmlTagTable() {}                                            // 0x10025c40
    virtual ~NHtmlTagTable() {}                                   // 0x10025c50
    // names ours
    INT FindTag(const TCHAR* Name);                               // 0x10025c60 index, -1
    INT FindAttr(INT Tag, const TCHAR* Name);                     // 0x10025cc0 1-based, -1
};
NW_ASSERT_SIZE(NHtmlTagTable, 4);

// base of the parser (its destructor is inlined; out-of-line copy 0x1002c750)
class NHtmlParserBase
{
public:
    virtual ~NHtmlParserBase() {}
};

// The tokenizer (0x84 bytes): turns the page into a list of tags and text runs.
class NHtmlParser : public NHtmlParserBase
{
public:
    NHtmlTagTable Table;       // +0x04
    INT Unknown08;             // +0x08 never set
    UBOOL bOutOfMemory;        // +0x0C a tag could not be allocated (never read)
    NStlWString Name;          // +0x10 name of the tag or attribute being read, upper case
    NStlWString Value;         // +0x2C value of the attribute being read
    NStlWString Extra;         // +0x48 an attribute the tag does not have, "name=value"
    NStlWString Unknown64;     // +0x64 emptied by ReadText, never read
    TCHAR* File;               // +0x80 the text LoadFile read

    NHtmlParser();                                                // 0x1002ca80
    virtual ~NHtmlParser();                                       // 0x1002cb00
    TCHAR* LoadFile(TCHAR* FileName);                             // 0x1002c890 its text, NULL
                                                                  //   when it cannot be read
    NHtmlTagList* Parse(TCHAR* Text);                             // 0x1002fae0
    void FreeTags(NHtmlTagList* Tags);                            // 0x1002f670
    void FreeFile();                                              // 0x1002c860
    // names ours
    bool IsAlnum(TCHAR C);                                        // 0x1002c6e0
    void ResetError();                                            // 0x1002c710
    TCHAR* SkipSpaces(TCHAR* P);                                  // 0x1002c790
    TCHAR* SkipWord(TCHAR* P);                                    // 0x1002c7c0 to a space, < or >
    TCHAR* SkipName(TCHAR* P);                                    // 0x1002c810 ... or =
    TCHAR* ReadTagName(TCHAR* P, NHtmlTag* Tag, INT* OutTag);     // 0x1002d530
    TCHAR* ReadSource(TCHAR* P, NHtmlTag* Tag);                   // 0x1002d770
    void ReplaceEntities(NStlWString* Str);                       // 0x1002d840
    TCHAR* ReadText(TCHAR* P, NHtmlTag* Tag);                     // 0x1002e1b0
    TCHAR* ReadExtend(TCHAR* P, NHtmlTag* Tag, INT TagIndex);     // 0x1002f2c0
    NHtmlTag* NewTag();                                           // 0x1002f550
    void DeleteTag(NHtmlTag* Tag);                                // 0x1002f640
    TCHAR* ReadAttrs(TCHAR* P, NHtmlTag* Tag, INT TagIndex);      // 0x1002f6e0
};
NW_ASSERT_OFFSET(NHtmlParser, Table, 0x04);
NW_ASSERT_OFFSET(NHtmlParser, Name, 0x10);
NW_ASSERT_OFFSET(NHtmlParser, Extra, 0x48);
NW_ASSERT_OFFSET(NHtmlParser, File, 0x80);
NW_ASSERT_SIZE(NHtmlParser, 0x84);

/*-----------------------------------------------------------------------------
    The tree.
-----------------------------------------------------------------------------*/

// Base of the tree (0x2C bytes, abstract: every slot is _purecall in the stock vtable). Objects
// are always deleted through this class, whose destructor does nothing: the derived parts are
// only freed by Release.
class NHtmlObject
{
public:
    INT Type;                  // +0x04 HTMLTAG_* of the tag that made it
    INT HAlign;                // +0x08 HTMLALIGN_*, top of GHtmlAlignStack when parsed
    INT VAlign;                // +0x0C HTMLVALIGN_*
    INT X;                     // +0x10 in the parent
    INT Y;                     // +0x14
    NHtmlObject* Parent;       // +0x18
    NCHtmlViewer* Viewer;      // +0x1C
    INT Width;                 // +0x20
    INT Height;                // +0x24
    INT MinWidth;              // +0x28 narrowest width it can be laid out at

    // not hooked: the class is abstract and only our constructors / deletes use them
    NHtmlObject();                                                // 0x10028aa0
    ~NHtmlObject() {}                                             // 0x10025e00

    virtual void Layout(INT MaxWidth, INT MaxHeight) = 0;         // 0 sizes itself
    virtual void Arrange(INT AreaWidth, INT AreaHeight) = 0;      // 1 aligns and places
    virtual void Draw(NCGDevice* Device) = 0;                     // 2
    virtual UBOOL IsPercentWidth() = 0;                           // 3
    virtual INT GetPercentWidth(INT Total) = 0;                   // 4 WIDTH=n% of Total, else 0
    virtual void Release() = 0;                                   // 5 frees what it owns

    void AlignX(INT AreaWidth);                                   // 0x10025e10
    INT GetAlignY(INT AreaHeight);                                // 0x10025e50
    POINT* GetAbsPos(POINT* Out);                                 // 0x10026bb0 in the page
};
NW_ASSERT_OFFSET(NHtmlObject, Type, 0x04);
NW_ASSERT_OFFSET(NHtmlObject, Viewer, 0x1C);
NW_ASSERT_SIZE(NHtmlObject, 0x2C);

// Children stacked top to bottom (0x50 bytes): the page (<body>) and every table cell.
class NHtmlBlock : public NHtmlObject
{
public:
    INT Unknown2C;             // +0x2C
    INT FixWidth;              // +0x30 <td fixwidth>, -1
    INT WidthValue;            // +0x34 <td width> / <table width>
    UBOOL bPercent;            // +0x38 the width ends with '%'
    TArray<NHtmlObject*> Children; // +0x3C
    UBOOL bBorder;             // +0x48 a cell of a table with a border: draws its frame
    INT Mode;                  // +0x4C HTMLITEM_* of the next item (set by <a>, <button>...)

    NHtmlBlock();                                                 // 0x10028b10
    NHtmlBlock(NHtmlObject* InParent, NCHtmlViewer* InViewer);    // 0x10028b50
    ~NHtmlBlock();                                                // 0x10028b90

    virtual void Layout(INT MaxWidth, INT MaxHeight);             // 0 0x10026d40
    virtual void Arrange(INT AreaWidth, INT AreaHeight);          // 1 0x10026dc0
    virtual void Draw(NCGDevice* Device);                         // 2 0x10026e30
    virtual UBOOL IsPercentWidth();                               // 3 0x10028b40
    virtual INT GetPercentWidth(INT Total);                       // 4 0x10025f20
    virtual void Release();                                       // 5 0x10027e00

    void ParseCellAttrs(NHtmlTag* Tag);                           // 0x10028c30 <td ...>
    INT GetContentHeight();                                       // 0x10027440
    NHtmlObject* NewItem(NHtmlTag*** It);                         // 0x1002b750
    NHtmlObject* NewTable(NHtmlTag*** It);                        // 0x1002a760
    void Parse(NHtmlTag*** It, UBOOL bInBorder);                  // 0x1002b7e0 to the closing tag
};
NW_ASSERT_OFFSET(NHtmlBlock, FixWidth, 0x30);
NW_ASSERT_OFFSET(NHtmlBlock, Children, 0x3C);
NW_ASSERT_OFFSET(NHtmlBlock, Mode, 0x4C);
NW_ASSERT_SIZE(NHtmlBlock, 0x50);

// <table> (0xA0 bytes): rows of cells (NHtmlBlock), columns as wide as their widest cell.
class NHtmlTable : public NHtmlBlock
{
public:
    INT Border;                // +0x50 <table border>, px
    INT NumRows;               // +0x54 <tr> read
    INT CurCol;                // +0x58 column of the next cell of the row
    TArray<TArray<NHtmlBlock*>*> Rows; // +0x5C
    TArray<INT> ColMinWidths;  // +0x68 widest MinWidth of each column
    TArray<INT> ColLayoutWidths; // +0x74 width of each column after Layout
    TArray<INT> ColWidths;     // +0x80 widest cell of each column
    TArray<INT> RowHeights;    // +0x8C after Layout
    DWORD BgColor;             // +0x98 <table bgcolor>
    UBOOL bBgColor;            // +0x9C

    NHtmlTable(NHtmlObject* InParent, NCHtmlViewer* InViewer);    // 0x10028e00
    // (deleted as NHtmlObject)

    virtual void Layout(INT MaxWidth, INT MaxHeight);             // 0 0x10027e60
    virtual void Arrange(INT AreaWidth, INT AreaHeight);          // 1 0x10026f00
    virtual void Draw(NCGDevice* Device);                         // 2 0x10026fa0
    virtual void Release();                                       // 5 0x100291d0

    NHtmlBlock* NewCell(NHtmlTag*** It);                          // 0x10028e80
    void Parse(NHtmlTag*** It);                                   // 0x10028f10
};
NW_ASSERT_OFFSET(NHtmlTable, Rows, 0x5C);
NW_ASSERT_OFFSET(NHtmlTable, RowHeights, 0x8C);
NW_ASSERT_OFFSET(NHtmlTable, bBgColor, 0x9C);
NW_ASSERT_SIZE(NHtmlTable, 0xA0);

// A leaf (0xD0 bytes): text, image, line break or a control. The strings hold the tag's
// attributes until Arrange hands them to the control it creates.
class NHtmlItem : public NHtmlObject
{
public:
    INT WrapWidth;             // +0x2C text: width it wraps at
    INT Mode;                  // +0x30 HTMLITEM_*
    NStlWString Action;        // +0x34 link target, button action, image, edit action,
                               //   combo box list
    NStlWString ForeTex;       // +0x50 <button fore>
    NStlWString BackTex;       // +0x6C <button back>
    NStlWString Var;           // +0x88 control variable, button label, link message
    NStlWString Sel;           // +0xA4 <combobox sel>
    TCHAR* Colors;             // +0xC0 text: copy of NHtmlTag::Colors
    UTexture* Tex;             // +0xC4 image
    INT EditType;              // +0xC8 <edit type>
    TCHAR* Text;               // +0xCC text: copy of NHtmlTag::Text

    NHtmlItem(NHtmlObject* InParent, NCHtmlViewer* InViewer, INT InMode); // 0x1002ad60
    // (deleted as NHtmlObject)

    virtual void Layout(INT MaxWidth, INT MaxHeight);             // 0 0x10026c90
    virtual void Arrange(INT AreaWidth, INT AreaHeight);          // 1 0x1002a4c0
    virtual void Draw(NCGDevice* Device);                         // 2 0x1002b5f0
    virtual UBOOL IsPercentWidth() { return 0; }                  // 3 0x100016e0 (shared)
    virtual INT GetPercentWidth(INT Total) { return 0; }          // 4 0x10001710 (shared)
    virtual void Release();                                       // 5 0x10026cf0

    INT ParseEditType(const TCHAR* Str);                          // 0x10025e90 (this unused)
    void SetText(NHtmlTag* Tag);                                  // 0x10027ce0
    void Parse(NHtmlTag*** It);                                   // 0x1002ae60
    void DrawColoredText(NCGDevice* Device, UBOOL bLink);         // 0x1002b2b0
};
NW_ASSERT_OFFSET(NHtmlItem, Mode, 0x30);
NW_ASSERT_OFFSET(NHtmlItem, Var, 0x88);
NW_ASSERT_OFFSET(NHtmlItem, Colors, 0xC0);
NW_ASSERT_OFFSET(NHtmlItem, Text, 0xCC);
NW_ASSERT_SIZE(NHtmlItem, 0xD0);

// "RRGGBB" as 0xFFRRGGBB; "LEVEL" is 0xFFFFCC00 (name ours)
DWORD HtmlParseColor(const TCHAR* Str);                           // 0x10026c00
