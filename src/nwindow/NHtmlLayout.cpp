/*=============================================================================
    NHtmlLayout.cpp - the page tree of NCHtmlViewer: NHtmlBlock, NHtmlTable, NHtmlItem and
    the tag accessors. Stock: 0x10025d70 .. 0x1002b7e0, interleaved with the viewer. The
    parser is in NHtmlParser.cpp, the std::wstring members in NStl.cpp.
=============================================================================*/
#include "nwindow/NHtmlLayout.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCrt.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// "CENTER", "RIGHT", "LEFT", "TOP", "BOTTOM" (stock table, name ours)
#if NW_HOOK_MODE
#define GHtmlAlignNames (reinterpret_cast<const TCHAR* const*>(::nw::Addr(0x1010bfd0)))
#else
extern const TCHAR* const GHtmlAlignNames[];
#endif

// separator of NHtmlItem::Colors
enum { HTMLCOLOR_Sep = 0x1E };

// a colored stretch of text for NCGDevice's colored DrawText (12 bytes, name ours; the device
// takes them as the engine's FL2ColorFontInfo)
struct NHtmlColorRange
{
    INT Start;                 // first character
    INT End;                   // last character
    DWORD Color;
};

// top of an alignment stack, 0 when empty (inlined everywhere in the stock code)
static FORCEINLINE INT TopOf(TArray<INT>& Stack)
{
    return Stack.Num() > 0 ? Stack(Stack.Num() - 1) : 0;
}

// TArray<INT>::Pop, inline as in the stock build (with its check, UnTemplate.h line 398)
static FORCEINLINE INT PopAlign()
{
    return GHtmlAlignStack.Pop();
}

/*-----------------------------------------------------------------------------
    NHtmlTag.
-----------------------------------------------------------------------------*/

// 0x10025d70 - the value of attribute Attr (the second argument is read but not used)
const TCHAR* NHtmlTag::GetAttr(INT Attr, UBOOL bUnused)
{
    _WORD* Id = AttrIds.First;
    NStlWString* Value = AttrValues.First;
    for (; Id != AttrIds.Last && Value != AttrValues.Last; Id++, Value++)
    {
        if (*Id == Attr)
            return Value->CStr();
    }
    return NULL;
}
NW_HOOK_METHOD(0x10025d70, NHtmlTag, GetAttr)

// 0x10025dd0 - a bool: the stock callers test al (the rest of eax is what the loop left)
bool NHtmlTag::HasAttr(INT Attr)
{
    for (_WORD* Id = AttrIds.First; Id != AttrIds.Last; Id++)
    {
        if (*Id == Attr)
            return true;
    }
    return false;
}
NW_HOOK_METHOD(0x10025dd0, NHtmlTag, HasAttr)

// 0x10026c00 - "RRGGBB" read as 0xFFRRGGBB (a buffer from the global new)
DWORD HtmlParseColor(const TCHAR* Str)
{
    if (!Str)
        return 0;
    if (!wcscmp(Str, TEXT("LEVEL")))
        return 0xFFFFCC00;
    TCHAR* Buf = new TCHAR[wcslen(Str) + 5];
    wcscpy(Buf, TEXT("0xff"));
    wcscat(Buf, Str);
    Buf[wcslen(Str) + 4] = 0;
    DWORD Color = wcstoul(Buf, NULL, 16);
    delete[] Buf;
    return Color;
}
NW_HOOK_FUNC(0x10026c00, HtmlParseColor)

/*-----------------------------------------------------------------------------
    NHtmlObject.
-----------------------------------------------------------------------------*/

// 0x10028aa0 - makes sure both alignment stacks have their bottom 0
NHtmlObject::NHtmlObject()
{
    Y = 0;
    X = 0;
    MinWidth = 0;
    Height = 0;
    Width = 0;
    Type = 0;
    HAlign = 0;
    VAlign = 0;
    Parent = NULL;
    Viewer = NULL;
    if (GHtmlAlignStack.Num() <= 0)
        GHtmlAlignStack.AddItem(0);
    if (GHtmlVAlignStack.Num() <= 0)
        GHtmlVAlignStack.AddItem(0);
}

// 0x10025e00 (out-of-line copy for the stock destructors; the constructor stays stock: the class
// is abstract)
NW_HOOK_DTOR(0x10025e00, NHtmlObject)

// 0x10025e10 - centered or right-aligned in an area AreaWidth wide
void NHtmlObject::AlignX(INT AreaWidth)
{
    if (HAlign == HTMLALIGN_Center)
        X = AreaWidth / 2 - Width / 2;
    else if (HAlign == HTMLALIGN_Right)
        X = AreaWidth - Width;
}
NW_HOOK_METHOD(0x10025e10, NHtmlObject, AlignX)

// 0x10025e50 - offset from the top for the vertical alignment in AreaHeight
INT NHtmlObject::GetAlignY(INT AreaHeight)
{
    if (VAlign == HTMLVALIGN_Center)
        return AreaHeight / 2 - Height / 2;
    if (VAlign == HTMLVALIGN_Bottom)
        return AreaHeight - Height;
    return 0;
}
NW_HOOK_METHOD(0x10025e50, NHtmlObject, GetAlignY)

// 0x10026bb0 - position in the page (the parents' positions added up)
POINT* NHtmlObject::GetAbsPos(POINT* Out)
{
    if (Parent)
    {
        POINT P;
        POINT* PP = Parent->GetAbsPos(&P);
        Out->x = X + PP->x;
        Out->y = Y + PP->y;
        return Out;
    }
    Out->x = X;
    Out->y = Y;
    return Out;
}
NW_HOOK_METHOD(0x10026bb0, NHtmlObject, GetAbsPos)

/*-----------------------------------------------------------------------------
    NHtmlBlock.
-----------------------------------------------------------------------------*/

// 0x10028b10
NHtmlBlock::NHtmlBlock()
{
    WidthValue = 0;
    bPercent = 0;
    bBorder = 0;
    Mode = HTMLITEM_Text;
    FixWidth = -1;
}
NW_HOOK_CTOR(0x10028b10, NHtmlBlock)

// 0x10028b50
NHtmlBlock::NHtmlBlock(NHtmlObject* InParent, NCHtmlViewer* InViewer)
{
    WidthValue = 0;
    bPercent = 0;
    bBorder = 0;
    Mode = HTMLITEM_Text;
    Parent = InParent;
    Viewer = InViewer;
    FixWidth = -1;
}
NW_HOOK_CTOR_ARGS(0x10028b50, NHtmlBlock, (NHtmlObject* A, NCHtmlViewer* B), (A, B))

// 0x10028b90 - the children go through NHtmlObject's destructor (Release has normally freed
// and removed them already)
NHtmlBlock::~NHtmlBlock()
{
    for (INT i = 0; i < Children.Num(); i++)
    {
        NHtmlObject* Child = Children(i);
        if (Child)
            delete Child;
    }
    Children.Empty();
}
NW_HOOK_DTOR(0x10028b90, NHtmlBlock)

// 0x10026d40 (vslot 0) - the children one under the other, at least MinWidth wide
void NHtmlBlock::Layout(INT MaxWidth, INT MaxHeight)
{
    INT N = Children.Num();
    INT ChildY = 0;
    if (MinWidth > MaxWidth)
        MaxWidth = MinWidth;
    Width = MaxWidth;
    for (INT i = 0; i < N; i++)
    {
        Children(i)->Layout(MaxWidth, MaxHeight);
        Children(i)->Y = ChildY;
        Children(i)->X = 0;
        ChildY += Children(i)->Height;
        if (Children(i)->Width > Width)
            Width = Children(i)->Width;
    }
    if (Height < ChildY)
        Height = ChildY;
}
NW_HOOK_METHOD(0x10026d40, NHtmlBlock, Layout)

// 0x10026dc0 (vslot 1) - fills the area, the children stacked from the vertical alignment
void NHtmlBlock::Arrange(INT AreaWidth, INT AreaHeight)
{
    INT N = Children.Num();
    INT ChildY = GetAlignY(AreaHeight);
    if (AreaHeight > Height)
        Height = AreaHeight;
    if (AreaWidth > Width)
        Width = AreaWidth;
    for (INT i = 0; i < N; i++)
    {
        Children(i)->Arrange(Width, Height);
        Children(i)->Y = ChildY;
        ChildY += Children(i)->Height;
    }
}
NW_HOOK_METHOD(0x10026dc0, NHtmlBlock, Arrange)

// 0x10026e30 (vslot 2) - a cell of a bordered table gets a frame, darker on the top and left
void NHtmlBlock::Draw(NCGDevice* Device)
{
    if (bBorder && Type == HTMLTAG_TD)
    {
        Device->DrawLine(X, Y, X + Width, Y, 0xFF404040, 0);
        Device->DrawLine(X, Y, X, Y + Height, 0xFF404040, 0);
        Device->DrawLine(X, Y + Height, X + Width, Y + Height, 0xFF646464, 0);
        Device->DrawLine(X + Width, Y, X + Width, Y + Height, 0xFF646464, 0);
    }
    Device->PushOrigin(X, Y);
    INT N = Children.Num();
    for (INT i = 0; i < N; i++)
        Children(i)->Draw(Device);
    Device->PopOrigin();
}
NW_HOOK_METHOD(0x10026e30, NHtmlBlock, Draw)

// 0x10028b40 (vslot 3)
UBOOL NHtmlBlock::IsPercentWidth()
{
    return bPercent;
}
NW_HOOK_METHOD(0x10028b40, NHtmlBlock, IsPercentWidth)

// 0x10025f20 (vslot 4)
INT NHtmlBlock::GetPercentWidth(INT Total)
{
    if (bPercent)
        return WidthValue * Total / 100;
    return 0;
}
NW_HOOK_METHOD(0x10025f20, NHtmlBlock, GetPercentWidth)

// 0x10027e00 (vslot 5)
void NHtmlBlock::Release()
{
    INT N = Children.Num();
    for (INT i = 0; i < N; i++)
    {
        Children(i)->Release();
        NHtmlObject* Child = Children(i);
        if (Child)
            delete Child;
    }
    Children.Empty();
}
NW_HOOK_METHOD(0x10027e00, NHtmlBlock, Release)

// 0x10028c30 - <td align valign width height fixwidth>: the alignment is pushed for the
// cell's contents (popped at </td>), a cell is vertically centered unless told otherwise
void NHtmlBlock::ParseCellAttrs(NHtmlTag* Tag)
{
    if (Tag->HasAttr(ATTR_TD_Align))
    {
        const TCHAR* Value = Tag->GetAttr(ATTR_TD_Align, 0);
        INT Align;
        if (!_wcsicmp(GHtmlAlignNames[0], Value))
            Align = HTMLALIGN_Center;
        else if (!_wcsicmp(GHtmlAlignNames[1], Value))
            Align = HTMLALIGN_Right;
        else
            Align = _wcsicmp(GHtmlAlignNames[2], Value) ? HTMLALIGN_None : HTMLALIGN_Left;
        GHtmlAlignStack.AddItem(Align);
    }
    else
        GHtmlAlignStack.AddItem(HTMLALIGN_None);

    if (Tag->HasAttr(ATTR_TD_VAlign))
    {
        const TCHAR* Value = Tag->GetAttr(ATTR_TD_VAlign, 0);
        if (!_wcsicmp(GHtmlAlignNames[3], Value))
            VAlign = HTMLVALIGN_Top;
        else if (!_wcsicmp(GHtmlAlignNames[4], Value))
            VAlign = HTMLVALIGN_Bottom;
        else
            VAlign = _wcsicmp(GHtmlAlignNames[0], Value) ? HTMLVALIGN_None : HTMLVALIGN_Center;
    }
    else
        VAlign = HTMLVALIGN_Center;

    if (Tag->HasAttr(ATTR_TD_Width))
    {
        const TCHAR* Value = Tag->GetAttr(ATTR_TD_Width, 0);
        WidthValue = NWtol(Value);
        if (Value[wcslen(Value) - 1] == '%')
            bPercent = 1;
    }
    if (Tag->HasAttr(ATTR_TD_Height))
        Height = NWtol(Tag->GetAttr(ATTR_TD_Height, 0));
    if (Tag->HasAttr(ATTR_TD_FixWidth))
        FixWidth = NWtol(Tag->GetAttr(ATTR_TD_FixWidth, 0));
    else
        FixWidth = -1;
    if (!bPercent && WidthValue > 0)
        Width = WidthValue;
}
NW_HOOK_METHOD(0x10028c30, NHtmlBlock, ParseCellAttrs)

// 0x10027440
INT NHtmlBlock::GetContentHeight()
{
    INT Total = 0;
    for (INT i = 0; i < Children.Num(); i++)
        Total += Children(i)->Height;
    return Total;
}
NW_HOOK_METHOD(0x10027440, NHtmlBlock, GetContentHeight)

// 0x1002b750 - the item takes the pending mode (link, control), which then goes back to text
NHtmlObject* NHtmlBlock::NewItem(NHtmlTag*** It)
{
    NHtmlItem* Item = new NHtmlItem(this, Viewer, Mode);
    Item->Parse(It);
    Mode = HTMLITEM_Text;
    return Item;
}
NW_HOOK_METHOD(0x1002b750, NHtmlBlock, NewItem)

// 0x1002a760
NHtmlObject* NHtmlBlock::NewTable(NHtmlTag*** It)
{
    NHtmlTable* Table = new NHtmlTable(this, Viewer);
    Table->Parse(It);
    return Table;
}
NW_HOOK_METHOD(0x1002a760, NHtmlBlock, NewTable)

// 0x1002b7e0 - the children up to the closing tag of the one *It points at. The loop tests
// the tag it handled last, so the closing tag is consumed before it stops. Only the first
// item after an <a> is a link.
void NHtmlBlock::Parse(NHtmlTag*** It, UBOOL bInBorder)
{
    HAlign = TopOf(GHtmlAlignStack);
    VAlign = TopOf(GHtmlVAlignStack);
    bBorder = bInBorder;
    NHtmlTag* Tag = **It;
    Type = Tag->Tag;
    if (Tag->Tag == HTMLTAG_TD)
        ParseCellAttrs(Tag);

    while (Tag->Tag != Type || Tag->bOpen)
    {
        Tag = **It;
        NHtmlObject* Child;
        switch (Tag->Tag)
        {
        case HTMLTAG_Text:
            if (Tag->Text.Size == 0)
            {
                (*It)++;
                continue;
            }
            Child = NewItem(It);
            break;
        case HTMLTAG_Br:
            Child = NewItem(It);
            break;
        case HTMLTAG_Img:
            Mode = HTMLITEM_Image;
            Child = NewItem(It);
            break;
        case HTMLTAG_Button:
            Mode = HTMLITEM_Button;
            Child = NewItem(It);
            break;
        case HTMLTAG_MultiEdit:
            Mode = HTMLITEM_MultiEdit;
            Child = NewItem(It);
            break;
        case HTMLTAG_Edit:
            Mode = HTMLITEM_Edit;
            Child = NewItem(It);
            break;
        case HTMLTAG_ComboBox:
            Mode = HTMLITEM_ComboBox;
            Child = NewItem(It);
            break;
        case HTMLTAG_Table:
            Child = NewTable(It);
            break;
        case HTMLTAG_Center:
            if (Tag->bOpen)
                GHtmlAlignStack.AddItem(HTMLALIGN_Center);
            else
                PopAlign();
            (*It)++;
            continue;
        case HTMLTAG_A:
            if (Tag->bOpen)
            {
                if (Tag->HasAttr(ATTR_A_Link))
                {
                    Mode = HTMLITEM_FileLink;
                    wcscpy(GHtmlLinkTarget, Tag->GetAttr(ATTR_A_Link, 0));
                    (*It)++;
                    continue;
                }
                if (Tag->HasAttr(ATTR_A_Action))
                {
                    Mode = HTMLITEM_ActionLink;
                    wcscpy(GHtmlLinkTarget, Tag->GetAttr(ATTR_A_Action, 0));
                    if (Tag->HasAttr(ATTR_A_Msg))
                        wcscpy(GHtmlLinkMsg, Tag->GetAttr(ATTR_A_Msg, 0));
                    else
                        GHtmlLinkMsg[0] = 0;
                    (*It)++;
                    continue;
                }
            }
            Mode = HTMLITEM_Text;
            GHtmlLinkTarget[0] = 0;
            (*It)++;
            continue;
        default:
            (*It)++;
            continue;
        }
        Children.AddItem(Child);
        if (Child->Width > Width)
            Width = Child->Width;
        if (Child->MinWidth > MinWidth)
            MinWidth = Child->MinWidth;
    }

    if (Type == HTMLTAG_TD)
    {
        if (FixWidth > 0)
            Width = FixWidth;
        PopAlign();
    }
}
NW_HOOK_METHOD(0x1002b7e0, NHtmlBlock, Parse)

/*-----------------------------------------------------------------------------
    NHtmlTable.
-----------------------------------------------------------------------------*/

// 0x10028e00
NHtmlTable::NHtmlTable(NHtmlObject* InParent, NCHtmlViewer* InViewer)
{
    Border = 0;
    CurCol = 0;
    NumRows = 0;
    bBgColor = 0;
    Parent = InParent;
    Viewer = InViewer;
    BgColor = 0xFFFFFFFF;
}
NW_HOOK_CTOR_ARGS(0x10028e00, NHtmlTable, (NHtmlObject* A, NCHtmlViewer* B), (A, B))

// 0x10027e60 (vslot 0) - each column as wide as its widest cell (scaled to a set width), each
// row as high as its highest cell, 2 px between cells and Border around
void NHtmlTable::Layout(INT MaxWidth, INT MaxHeight)
{
    RowHeights.Empty();
    INT N = Rows.Num();
    INT CellY = Border + 2;
    INT RowHeight = 0;
    INT TableWidth = 0;
    if (bPercent)
        WidthValue = WidthValue * MaxWidth / 100;
    if (Width >= MaxWidth && WidthValue == 0)
        WidthValue = MaxWidth;
    for (INT Row = 0; Row < N; Row++)
    {
        TArray<NHtmlBlock*>* Cells = Rows(Row);
        INT NumCells = Cells->Num();
        INT CellX = Border + 2;
        for (INT Col = 0; Col < NumCells; Col++)
        {
            NHtmlBlock* Cell = (*Cells)(Col);
            if (MaxWidth > Width && WidthValue == 0)
                Cell->Layout(ColWidths(Col), 0);
            else
                Cell->Layout(ColWidths(Col) * WidthValue / Width, 0);
            if (Cell->Height > RowHeight)
                RowHeight = Cell->Height;
            Cell->X = CellX;
            Cell->Y = CellY;
            INT CellW = Cell->Width;
            CellX += CellW + 2;
            if (Col < ColLayoutWidths.Num())
            {
                if (CellW > ColLayoutWidths(Col))
                    ColLayoutWidths(Col) = CellW;
            }
            else
                ColLayoutWidths.AddItem(CellW);
        }
        RowHeights.AddItem(RowHeight);
        CellY += RowHeight + 2;
        CellX += Border;
        if (CellX > TableWidth)
            TableWidth = CellX;
        RowHeight = 0;
    }
    Width = TableWidth;
    Height = Border + CellY;
}
NW_HOOK_METHOD(0x10027e60, NHtmlTable, Layout)

// 0x10026f00 (vslot 1) - every cell of a column gets the column's width, every cell of a row
// the row's height
void NHtmlTable::Arrange(INT AreaWidth, INT AreaHeight)
{
    AlignX(AreaWidth);
    INT N = Rows.Num();
    for (INT Row = 0; Row < N; Row++)
    {
        TArray<NHtmlBlock*>* Cells = Rows(Row);
        INT NumCells = Cells->Num();
        INT CellX = Border + 2;
        for (INT Col = 0; Col < NumCells; Col++)
        {
            NHtmlBlock* Cell = (*Cells)(Col);
            Cell->X = CellX;
            INT ColW = ColLayoutWidths(Col);
            Cell->Arrange(ColW, RowHeights(Row));
            CellX += ColW + 2;
        }
    }
}
NW_HOOK_METHOD(0x10026f00, NHtmlTable, Arrange)

// 0x10026fa0 (vslot 2) - background, a Border px frame lighter on the top and left, cells
void NHtmlTable::Draw(NCGDevice* Device)
{
    if (bBgColor)
        Device->DrawRect(X, Y, Width, Height, BgColor);
    if (Border > 0)
    {
        for (INT i = 0; i < Border; i++)
        {
            Device->DrawLine(X + i, Y + i, X + Width - i, Y + i, 0xFF646464, 0);
            Device->DrawLine(X + i, Y + i, X + i, Y + Height - i, 0xFF646464, 0);
            Device->DrawLine(X + i, Y + Height - i, X + Width - i, Y + Height - i, 0xFF404040, 0);
            Device->DrawLine(X + Width - i, Y + i, X + Width - i, Y + Height - i, 0xFF404040, 0);
        }
    }
    Device->PushOrigin(X, Y);
    INT N = Rows.Num();
    for (INT Row = 0; Row < N; Row++)
    {
        INT NumCells = Rows(Row)->Num();
        for (INT Col = 0; Col < NumCells; Col++)
            (*Rows(Row))(Col)->Draw(Device);
    }
    Device->PopOrigin();
}
NW_HOOK_METHOD(0x10026fa0, NHtmlTable, Draw)

// 0x100291d0 (vslot 5)
void NHtmlTable::Release()
{
    INT N = Rows.Num();
    for (INT Row = 0; Row < N; Row++)
    {
        INT NumCells = Rows(Row)->Num();
        for (INT Col = 0; Col < NumCells; Col++)
        {
            (*Rows(Row))(Col)->Release();
            NHtmlObject* Cell = (*Rows(Row))(Col);
            if (Cell)
                delete Cell;
        }
        Rows(Row)->Empty();
        TArray<NHtmlBlock*>* Cells = Rows(Row);
        if (Cells)
            delete Cells;
    }
    Rows.Empty();
    ColMinWidths.Empty();
    ColLayoutWidths.Empty();
    ColWidths.Empty();
    RowHeights.Empty();
}
NW_HOOK_METHOD(0x100291d0, NHtmlTable, Release)

// 0x10028e80 - a <td>: the cells draw their frame when the table has a border
NHtmlBlock* NHtmlTable::NewCell(NHtmlTag*** It)
{
    NHtmlBlock* Cell = new NHtmlBlock(this, Viewer);
    if (Border > 0)
        Cell->Parse(It, 1);
    else
        Cell->Parse(It, 0);
    return Cell;
}
NW_HOOK_METHOD(0x10028e80, NHtmlTable, NewCell)

// 0x10028f10 - the rows and cells up to </table>, which is consumed (the loop tests the tag it
// handled last). A <td> before the first <tr> is never consumed: the stock loop spins on it.
void NHtmlTable::Parse(NHtmlTag*** It)
{
    HAlign = TopOf(GHtmlAlignStack);
    VAlign = TopOf(GHtmlVAlignStack);
    TArray<NHtmlBlock*>* Row = NULL;
    NHtmlTag* Tag = **It;
    Type = Tag->Tag;
    ColWidths.Empty();
    ColLayoutWidths.Empty();
    ColMinWidths.Empty();
    if (Tag->HasAttr(ATTR_Table_Border))
        Border = NWtol(Tag->GetAttr(ATTR_Table_Border, 0));
    if (Tag->HasAttr(ATTR_Table_Width))
    {
        const TCHAR* Value = Tag->GetAttr(ATTR_Table_Width, 0);
        WidthValue = NWtol(Value);
        if (Value[wcslen(Value) - 1] == '%')
            bPercent = 1;
    }
    if (Tag->HasAttr(ATTR_Table_BgColor))
    {
        BgColor = HtmlParseColor(Tag->GetAttr(ATTR_Table_BgColor, 0));
        bBgColor = 1;
    }
    else
        bBgColor = 0;

    while (Tag->Tag != Type || Tag->bOpen)
    {
        Tag = **It;
        if (Tag->Tag == HTMLTAG_TR)
        {
            if (Tag->bOpen)
            {
                NumRows++;
                Row = new TArray<NHtmlBlock*>;
                Rows.AddItem(Row);
                (*It)++;
            }
            else
            {
                CurCol = 0;
                (*It)++;
            }
        }
        else if (Tag->Tag == HTMLTAG_TD)
        {
            if (!Row)
                continue;
            NHtmlBlock* Cell = NewCell(It);
            Row->AddItem(Cell);
            if (CurCol < ColWidths.Num())
            {
                if (Cell->Width > ColWidths(CurCol))
                    ColWidths(CurCol) = Cell->Width;
                if (Cell->MinWidth > ColMinWidths(CurCol))
                    ColMinWidths(CurCol) = Cell->MinWidth;
                CurCol++;
            }
            else
            {
                ColWidths.AddItem(Cell->Width);
                ColMinWidths.AddItem(Cell->MinWidth);
                CurCol++;
            }
        }
        else
            (*It)++;
    }

    Width = 0;
    for (INT i = 0; i < ColWidths.Num(); i++)
    {
        Width += ColWidths(i) + 2;
        MinWidth += ColMinWidths(i) + 2;
    }
    Width += Border * 2 + 2;
    MinWidth += Border * 2 + 2;
}
NW_HOOK_METHOD(0x10028f10, NHtmlTable, Parse)

/*-----------------------------------------------------------------------------
    NHtmlItem.
-----------------------------------------------------------------------------*/

// 0x1002ad60 - a link item takes the target and message of the <a> being read
NHtmlItem::NHtmlItem(NHtmlObject* InParent, NCHtmlViewer* InViewer, INT InMode)
{
    Parent = InParent;
    Colors = NULL;
    Tex = NULL;
    Viewer = InViewer;
    Text = NULL;
    Mode = InMode;
    if (Mode)
    {
        Action.Assign(GHtmlLinkTarget, wcslen(GHtmlLinkTarget));
        Var.Assign(GHtmlLinkMsg, wcslen(GHtmlLinkMsg));
    }
}
NW_HOOK_CTOR_ARGS(0x1002ad60, NHtmlItem, (NHtmlObject* A, NCHtmlViewer* B, INT C), (A, B, C))

// 0x10026c90 (vslot 0) - text wraps at the width given (at least one character)
void NHtmlItem::Layout(INT MaxWidth, INT MaxHeight)
{
    if (Type == HTMLTAG_Text)
    {
        INT W = MaxWidth;
        if (MinWidth > W)
            W = MinWidth;
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Text, W, 0, 1, 0, 0);
        WrapWidth = W;
        Width = Size.cx;
        Height = Size.cy + 3;
    }
}
NW_HOOK_METHOD(0x10026c90, NHtmlItem, Layout)

// 0x1002a4c0 (vslot 1) - aligned; a control item creates its control at its page position,
// an image finds its texture
void NHtmlItem::Arrange(INT AreaWidth, INT AreaHeight)
{
    AlignX(AreaWidth);
    POINT P;
    if (Mode == HTMLITEM_FileLink)
    {
        GetAbsPos(&P);
        Viewer->AddFileLink(P.x, P.y, Width, Height, Action.CStr());
    }
    else if (Mode == HTMLITEM_ActionLink)
    {
        GetAbsPos(&P);
        Viewer->AddActionLink(P.x, P.y, Width, Height, Action.CStr(), Var.CStr());
    }
    else if (Mode == HTMLITEM_Button)
    {
        GetAbsPos(&P);
        Viewer->AddButton(P.x, P.y, Width, Height, ForeTex.CStr(), BackTex.CStr(), Action.CStr(),
                          Var.CStr());
    }
    else if (Mode == HTMLITEM_MultiEdit)
    {
        GetAbsPos(&P);
        Viewer->AddMultiEdit(P.x, P.y, Width, Height, Var.CStr());
    }
    else if (Mode == HTMLITEM_Image)
        Tex = Viewer->FindPageTexture(Action.CStr());
    else if (Mode == HTMLITEM_Edit)
    {
        GetAbsPos(&P);
        Viewer->AddEdit(P.x, P.y, Width, Height, Var.CStr(), EditType, Action.CStr());
    }
    else if (Mode == HTMLITEM_ComboBox)
    {
        GetAbsPos(&P);
        Viewer->AddComboBox(P.x, P.y, Width, Height, Var.CStr(), Action.CStr(), Sel.CStr());
    }
}
NW_HOOK_METHOD(0x1002a4c0, NHtmlItem, Arrange)

// 0x1002b5f0 (vslot 2) - text white (links blue and underlined, mode 6 red, <font color> runs
// in their colors), images stretched
void NHtmlItem::Draw(NCGDevice* Device)
{
    if (Type == HTMLTAG_Text)
    {
        if (Mode == HTMLITEM_Text)
        {
            if (wcscmp(Colors, TEXT("")))
            {
                DrawColoredText(Device, 0);
                return;
            }
            Device->DrawText(X, Y, 0xFFFFFFFF, Text, WrapWidth, 0, 1, L2FONT_Normal, 0, 0, 0, 0, 0,
                             1);
        }
        else if (Mode == HTMLITEM_ActionLink || Mode == HTMLITEM_FileLink)
        {
            if (wcscmp(Colors, TEXT("")))
            {
                DrawColoredText(Device, 1);
                return;
            }
            Device->DrawText(X, Y, 0xFF6464FF, Text, WrapWidth, 0, 1, L2FONT_Normal, 0, 1,
                             0xFF000080, 0, 0, 1);
        }
        else if (Mode == HTMLITEM_RedText)
            Device->DrawText(X, Y, 0xFFFF0000, Text, WrapWidth, 0, 1, L2FONT_Normal, 0, 0, 0, 0, 0,
                             1);
        else
            Device->DrawText(X, Y, 0xFFFFFFFF, Text, WrapWidth, 0, 1, L2FONT_Normal, 0, 0, 0, 0, 0,
                             1);
    }
    else if (Type == HTMLTAG_Img)
    {
        if (Tex)
            Device->DrawTexture(X, Y, Width, Height, 0, 0, Width, Height, Tex);
    }
}
NW_HOOK_METHOD(0x1002b5f0, NHtmlItem, Draw)

// 0x10026cf0 (vslot 5)
void NHtmlItem::Release()
{
    if (Type == HTMLTAG_Text)
    {
        if (Colors)
        {
            delete[] Colors;
            Colors = NULL;
        }
        if (Text)
        {
            delete[] Text;
            Text = NULL;
        }
    }
}
NW_HOOK_METHOD(0x10026cf0, NHtmlItem, Release)

// 0x10025e90 - <edit type>: normal 0, password 2, number 3, date 4, time 5 (NCEditBox::Type)
INT NHtmlItem::ParseEditType(const TCHAR* Str)
{
    if (!wcscmp(Str, TEXT("normal")))
        return 0;
    if (!wcscmp(Str, TEXT("password")))
        return 2;
    if (!wcscmp(Str, TEXT("date")))
        return 4;
    if (!wcscmp(Str, TEXT("time")))
        return 5;
    return wcscmp(Str, TEXT("number")) ? 0 : 3;
}
NW_HOOK_METHOD(0x10025e90, NHtmlItem, ParseEditType)

// 0x10027ce0 - a text run: its own copies of the text and of the color runs, measured on one
// line; one character is the narrowest it gets
void NHtmlItem::SetText(NHtmlTag* Tag)
{
    Type = Tag->Tag;
    Text = new TCHAR[wcslen(Tag->Text.CStr()) + 1];
    wcscpy(Text, Tag->Text.CStr());
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    Width = Size.cx;
    Height = Size.cy + 3;
    NCGDevice::GetTextExtent(&Size, TEXT("\xAC00"), 0, 0, 0, 0, 0);
    MinWidth = Size.cx;
    Colors = new TCHAR[wcslen(Tag->Colors.CStr()) + 1];
    wcscpy(Colors, Tag->Colors.CStr());
}
NW_HOOK_METHOD(0x10027ce0, NHtmlItem, SetText)

// 0x1002ae60 - the size and strings of the item from its tag, which is consumed. Only images
// and text record their tag in Type. Controls get a default size; edits and multi-line
// edits are 3 px taller than asked.
void NHtmlItem::Parse(NHtmlTag*** It)
{
    HAlign = TopOf(GHtmlAlignStack);
    VAlign = TopOf(GHtmlVAlignStack);
    NHtmlTag* Tag = **It;
    INT W, H;
    switch (Tag->Tag)
    {
    case HTMLTAG_Img:
        Type = Tag->Tag;
        W = 0;
        H = 0;
        if (Tag->HasAttr(ATTR_Img_Src))
            Action.Assign(Tag->GetAttr(ATTR_Img_Src, 0));
        if (Tag->HasAttr(ATTR_Img_Width))
            W = NWtol(Tag->GetAttr(ATTR_Img_Width, 0));
        if (Tag->HasAttr(ATTR_Img_Height))
            H = NWtol(Tag->GetAttr(ATTR_Img_Height, 0));
        Height = H;
        break;
    case HTMLTAG_Br:
        Width = 1;
        MinWidth = 1;
        Height = 9;
        (*It)++;
        return;
    case HTMLTAG_Text:
        SetText(Tag);
        (*It)++;
        return;
    case HTMLTAG_Button:
        W = 10;
        H = 10;
        if (Tag->HasAttr(ATTR_Button_Action))
            Action.Assign(Tag->GetAttr(ATTR_Button_Action, 0));
        if (Tag->HasAttr(ATTR_Button_Width))
            W = NWtol(Tag->GetAttr(ATTR_Button_Width, 0));
        if (Tag->HasAttr(ATTR_Button_Height))
            H = NWtol(Tag->GetAttr(ATTR_Button_Height, 0));
        ForeTex.Assign(Tag->HasAttr(ATTR_Button_Fore) ? Tag->GetAttr(ATTR_Button_Fore, 0)
                                                      : TEXT("NWindow.BlackTexture"));
        BackTex.Assign(Tag->HasAttr(ATTR_Button_Back) ? Tag->GetAttr(ATTR_Button_Back, 0)
                                                      : TEXT("NWindow.WhiteTexture"));
        if (Tag->HasAttr(ATTR_Button_Value))
            Var.Assign(Tag->GetAttr(ATTR_Button_Value, 0));
        Height = H;
        break;
    case HTMLTAG_MultiEdit:
        W = 50;
        H = 40;
        if (Tag->HasAttr(ATTR_MultiEdit_Width))
            W = NWtol(Tag->GetAttr(ATTR_MultiEdit_Width, 0));
        if (Tag->HasAttr(ATTR_MultiEdit_Height))
            H = NWtol(Tag->GetAttr(ATTR_MultiEdit_Height, 0));
        if (Tag->HasAttr(ATTR_MultiEdit_Var))
            Var.Assign(Tag->GetAttr(ATTR_MultiEdit_Var, 0));
        Height = H + 3;
        break;
    case HTMLTAG_Edit:
        W = 50;
        H = 15;
        if (Tag->HasAttr(ATTR_Edit_Width))
            W = NWtol(Tag->GetAttr(ATTR_Edit_Width, 0));
        if (Tag->HasAttr(ATTR_Edit_Height))
            H = NWtol(Tag->GetAttr(ATTR_Edit_Height, 0));
        if (Tag->HasAttr(ATTR_Edit_Var))
            Var.Assign(Tag->GetAttr(ATTR_Edit_Var, 0));
        if (Tag->HasAttr(ATTR_Edit_Action))
            Action.Assign(Tag->GetAttr(ATTR_Edit_Action, 0));
        EditType = 0;
        if (Tag->HasAttr(ATTR_Edit_Type))
            EditType = ParseEditType(Tag->GetAttr(ATTR_Edit_Type, 0));
        Height = H + 3;
        break;
    case HTMLTAG_ComboBox:
        W = 30;
        if (Tag->HasAttr(ATTR_Combo_Width))
            W = NWtol(Tag->GetAttr(ATTR_Combo_Width, 0));
        if (Tag->HasAttr(ATTR_Combo_Var))
            Var.Assign(Tag->GetAttr(ATTR_Combo_Var, 0));
        if (Tag->HasAttr(ATTR_Combo_List))
            Action.Assign(Tag->GetAttr(ATTR_Combo_List, 0));
        if (Tag->HasAttr(ATTR_Combo_Sel))
            Sel.Assign(Tag->GetAttr(ATTR_Combo_Sel, 0));
        Height = 18;
        break;
    default:
        (*It)++;
        return;
    }
    Width = W;
    MinWidth = W;
    (*It)++;
}
NW_HOOK_METHOD(0x1002ae60, NHtmlItem, Parse)

// 0x1002b2b0 - text with <font color> runs. Colors reads "color\x1Eword\x1Ecolor\x1E..." from
// its second character on; each word is looked for after the previous one and gets the color
// before it, the rest of the text stays white. bLink is not used (links lose their color).
void NHtmlItem::DrawColoredText(NCGDevice* Device, UBOOL bLink)
{
    TCHAR* Spec = new TCHAR[wcslen(Colors) + 2];
    wcscpy(Spec, Colors);
    Spec[wcslen(Colors)] = HTMLCOLOR_Sep;
    Spec[wcslen(Colors) + 1] = 0;
    INT SpecLen = wcslen(Spec);
    TCHAR* ColorBuf = new TCHAR[256];
    TCHAR* WordBuf = new TCHAR[256];
    INT ColorLen = 0;
    INT WordLen = 0;
    UBOOL bReadColor = 1;
    NStlWString Rest;
    Rest.Assign(Text, wcslen(Text));
    POINT Pos;
    Pos.x = X;
    Pos.y = Y;
    TArray<NHtmlColorRange> Ranges;
    DWORD Color;
    for (INT i = 1; i < SpecLen; i++)
    {
        TCHAR C = Spec[i];
        if (bReadColor)
        {
            if (C != HTMLCOLOR_Sep)
                ColorBuf[ColorLen++] = C;
            else
            {
                ColorBuf[ColorLen] = 0;
                Color = HtmlParseColor(ColorBuf);
                bReadColor = 0;
                ColorLen = 0;
            }
        }
        else
        {
            if (C != HTMLCOLOR_Sep)
                WordBuf[WordLen++] = C;
            else
            {
                WordBuf[WordLen] = 0;
                bReadColor = 1;
                WordLen = 0;
                INT Found = Rest.Find(WordBuf, 0, wcslen(WordBuf));
                if (Color)
                {
                    INT End = appStrlen(WordBuf) + Found - 1;
                    NHtmlColorRange* Range = &Ranges(Ranges.Add());
                    Range->Start = Found;
                    Range->End = End;
                    Range->Color = Color;
                }
                Rest.Erase(0, wcslen(WordBuf) + Found);
            }
        }
    }
    Device->DrawColoredLine(Pos.x, Pos.y, 0xFFFFFFFF, Text, WrapWidth, 0, 1, (L2FontType)0, 0, 0,
                            0, (TArray<FL2ColorFontInfo>*)&Ranges);
    Ranges.Empty();
    Rest.Erase(0, Rest.Size);
    delete[] Spec;
    delete[] ColorBuf;
    delete[] WordBuf;
}
NW_HOOK_METHOD(0x1002b2b0, NHtmlItem, DrawColoredText)

// 0x10028790 - the ranges' array destructor (a TArray instance of the stock code)
typedef TArray<NHtmlColorRange> NHtmlColorRangeArray;
NW_HOOK_DTOR(0x10028790, NHtmlColorRangeArray)

NW_CHECK_VTABLE(NHtmlBlock, 0x100de960, 6)
NW_CHECK_VTABLE(NHtmlTable, 0x100de978, 6)
NW_CHECK_VTABLE(NHtmlItem, 0x100de948, 6)
