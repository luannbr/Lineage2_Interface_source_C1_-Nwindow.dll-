/*=============================================================================
    NCMultiEdit.cpp - the multi-line text box, its line list and string array, and the HTML
    pages' version of it. Stock: 0x1001a440 .. 0x10020c9d, 0x10021f00 .. 0x10022147 (NCMultiEdit),
    0x1002ff20 .. 0x10030154, 0x10030ff0 .. 0x100326cf, 0x10033540 .. 0x100338c2
    (NCHtmlMultiEdit). SetFocus and the HTML box's OnLButtonDown are bodies the linker folded with
    other classes'. The getters of the line count (0x10033ac0, hooked as NActionItem's) and of
    FString::Len (0x1001b380) are folded bodies other classes call too.
=============================================================================*/
#include "nwindow/NCMultiEdit.h"
#include "nwindow/NCIMEManager.h"
#include "nwindow/NCChineseIME.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCHtmlButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCHtmlSliderBar.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCCompositeBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NString.h"

#include <string.h>

enum { TIMER_MultiEditCaret = 0xFA0D05 };

// the code a click sends the viewer with the box's y (as NCHtmlEdit's): MAKELONG(100, LocalY)
enum { HTML_SCROLLTO = 100 };

// the composition window of the Chinese IME and its last composition
static FORCEINLINE NCCompositeBox* ChineseCompositeBox(NCIMEManager* Manager)
{
    return ((NCChineseIME*)Manager->ChineseIME)->CompositeBox;
}
static FORCEINLINE FString& ChineseLastComp(NCIMEManager* Manager)
{
    return ((NCChineseIME*)Manager->ChineseIME)->LastComp;
}

// the IME types whose composition is drawn in its own window (Chinese layouts)
static FORCEINLINE UBOOL IsWindowedIME(INT Type)
{
    return Type == 10 || Type == 9 || Type == 11 || Type == 12;
}

/*-----------------------------------------------------------------------------
    NLineNode, NLineList.
-----------------------------------------------------------------------------*/

// 0x1001b390 (not inlined: the stock callers call it)
__declspec(noinline) NLineNode::NLineNode()
{
    Prev = NULL;
    Next = NULL;
    Text.Empty();
}
NW_HOOK_CTOR(0x1001b390, NLineNode)

// 0x1001b3e0 (not inlined: the stock callers call it)
__declspec(noinline) NLineNode::~NLineNode()
{
    Prev = NULL;
    Next = NULL;
    Text.Empty();
}
NW_HOOK_DTOR(0x1001b3e0, NLineNode)

// 0x1001bc20 - one empty line
NLineList::NLineList()
{
    Count = 1;
    Unknown4 = 0;
    NLineNode* Node = new NLineNode;
    Head = Node;
    Tail = Node;
    Current = Node;
    CurIndex = 0;
}
NW_HOOK_CTOR(0x1001bc20, NLineList)

// 0x10020a20 - Count is cleared before Clear(), which then frees nothing: the nodes are left
// (sic; OnDestroy has emptied the list before)
NLineList::~NLineList()
{
    Count = 0;
    Unknown4 = 0;
    Clear();
    if (Head)
        delete Head;
}
NW_HOOK_DTOR(0x10020a20, NLineList)

// 0x1001bd10 - walks from the nearest of the head, the tail and the cursor. The walks from the
// cursor bound their loop by the cursor as it moves, so they go half the way (sic): one line
// off is right, two or more end on a line in between.
NLineNode* NLineList::GetLine(INT Index)
{
    if (Index >= Count)
        return NULL;
    if (Index == CurIndex)
        return Current;
    NLineNode* Node;
    if (Index < CurIndex)
    {
        if (Index > CurIndex - Index)
        {
            Node = Current;
            for (INT i = 0; i < CurIndex - Index; i++)
                Node = StepBack();
            return Node;
        }
        Current = Head;
        CurIndex = 0;
        Node = Current;
        for (INT i = Index; i > 0; i--)
            Node = StepForward();
        return Node;
    }
    if (Index - CurIndex > Count - Index - 1)
    {
        Current = Tail;
        CurIndex = Count - 1;
        Node = Current;
        for (INT i = 0; i < Count - Index - 1; i++)
            Node = StepBack();
        return Node;
    }
    Node = Current;
    for (INT i = 0; i < Index - CurIndex; i++)
        Node = StepForward();
    return Node;
}
NW_HOOK_METHOD(0x1001bd10, NLineList, GetLine)

// 0x1001bca0
INT NLineList::TotalLength()
{
    INT Total = 0;
    Current = Head;
    CurIndex = 0;
    for (INT i = 0; i < Count; i++)
    {
        if (!Current)
            break;
        Total += Current->Text.Len();
        if (!StepForward())
            break;
    }
    return Total;
}
NW_HOOK_METHOD(0x1001bca0, NLineList, TotalLength)

// 0x1001de60 - -1 appends; an index out of range frees the new node and fails
INT NLineList::Insert(INT Index)
{
    NLineNode* Node = new NLineNode;
    if (Index == -1)
    {
        if (Count)
        {
            if (GetLine(Count - 1) && Node)
            {
                GetLine(Count - 1)->Next = Node;
                Node->Prev = GetLine(Count - 1);
                Count++;
                Tail = Node;
                return 1;
            }
        }
        else if (Node)
        {
            if (Head)
                delete Head;
            Head = Node;
            Current = Node;
            CurIndex = 0;
            Tail = Node;
        }
        Count++;
        return 1;
    }
    if (Index > Count || Index < 0)
    {
        if (Node)
            delete Node;
        return 0;
    }
    NLineNode* At = NULL;
    if (Index < Count && GetLine(Index))
        At = GetLine(Index);
    if (Index > 0 && GetLine(Index - 1))
    {
        NLineNode* Before = GetLine(Index - 1);
        if (Before)
        {
            Before->Next = Node;
            Node->Prev = Before;
        }
    }
    if (Node)
        Node->Next = At;
    if (At)
        At->Prev = Node;
    if (Index == 0)
    {
        Head = Node;
        if (Count == 0)
        {
            Current = Node;
            Tail = Node;
            CurIndex = 0;
        }
    }
    if (Index == Count)
        Tail = Node;
    if (Index <= CurIndex && Count)
        CurIndex++;
    Count++;
    return 1;
}
NW_HOOK_METHOD(0x1001de60, NLineList, Insert)

// 0x1001e000
void NLineList::RemoveAt(INT Index)
{
    if (Index >= Count || Index < 0)
        return;
    if (!GetLine(Index))
        return;
    NLineNode* Node = GetLine(Index);
    GetLine(Index)->Text.Empty();
    if (!Node)
        return;
    if (Index < CurIndex)
    {
        if (CurIndex > 0)
            CurIndex--;
    }
    else if (Index == CurIndex)
    {
        if (Node->Prev)
        {
            Current = Node->Prev;
            CurIndex--;
        }
        else
            Current = Node->Next;
    }
    if (Node->Prev)
        Node->Prev->Next = Node->Next;
    if (Node->Next)
        Node->Next->Prev = Node->Prev;
    if (Index == 0)
        Head = Node->Next;
    if (Index == Count - 1)
        Tail = Node->Prev;
    delete Node;
    Count--;
}
NW_HOOK_METHOD(0x1001e000, NLineList, RemoveAt)

// 0x1001e0c0
void NLineList::Clear()
{
    while (Count)
        RemoveAt(0);
    Head = NULL;
    Current = NULL;
    Tail = NULL;
    CurIndex = 0;
}
NW_HOOK_METHOD(0x1001e0c0, NLineList, Clear)

/*-----------------------------------------------------------------------------
    NStringArray.
-----------------------------------------------------------------------------*/

// 0x10022130
NStringArray::NStringArray()
{
    Num = 0;
    Data = NULL;
    Max = 0;
    InsertAt(-1);
}
NW_HOOK_CTOR(0x10022130, NStringArray)

// 0x10021f70
NStringArray::~NStringArray()
{
    Empty();
}
NW_HOOK_DTOR(0x10021f70, NStringArray)

// 0x10021f00
INT NStringArray::Empty()
{
    for (INT i = 0; i < Num; i++)
        Data[i].Empty();
    Num = 0;
    if (Data)
    {
        delete[] Data;
        Data = NULL;
    }
    Max = 0;
    return 0;
}
NW_HOOK_METHOD(0x10021f00, NStringArray, Empty)

// 0x10021f80 (not inlined: the stock callers call it) - the first string needs no clearing. Growing copies Num *bytes* of the old array
// before destroying it (sic): past 127 strings their text would be lost.
__declspec(noinline) void NStringArray::InsertAt(INT Index)
{
    if (Index == -1)
        Index = Num;
    if (!Data)
    {
        Data = new NString[128];
        Max += 128;
    }
    else
    {
        if (Num >= Max - 1)
        {
            Max += 128;
            NString* Old = Data;
            Data = new NString[Max];
            appMemcpy(Data, Old, Num);
            if (Old)
                delete[] Old;
        }
        if (Index < Num)
            memmove(&Data[Index + 1], &Data[Index], (Num - Index + 1) * sizeof(NString));
        appMemset(&Data[Index], 0, sizeof(NString));
    }
    Num++;
}
NW_HOOK_METHOD(0x10021f80, NStringArray, InsertAt)

/*-----------------------------------------------------------------------------
    NCMultiEdit.
-----------------------------------------------------------------------------*/

// 0x10020b30 - NControl's constructor is inlined
NCMultiEdit::NCMultiEdit()
{
    BackTex = NULL;
    WhiteTex = NULL;
    TextBuf = NULL;
    CaretTimer = 0;
    CaretLine = 0;
    CaretCol = 0;
    bCaretOn = 0;
    VisibleRows = 0;
    TopLine = 0;
    NumLines = 1;
    CaretRow = 0;
    CaretX = 0;
    bCompRow = 0;
    Unknown210 = 0;
    Unknown220 = 0;
    UpButton = NULL;
    DownButton = NULL;
    ScrollBar = NULL;
    bReading = 0;
    Unknown214 = 0;
    Unknown216 = 0;
    Unknown254 = 0;
    Unknown258 = 0;
    IMECaretX = 0;
    IMECaretY = 0;
    TotalRows = 1;
    Unknown250 = 1;
    Unknown24C = 1;
    Unknown25C = 16;
}
NW_HOOK_CTOR(0x10020b30, NCMultiEdit)

// 0x10020a90
NW_HOOK_DTOR(0x10020a90, NCMultiEdit)

// 0x1001a440 (vslot 24) - the scroll controls, then the IME's messages
INT NCMultiEdit::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == (WPARAM)NID_MULTIEDIT_UP)
            return OnScrollUp() != 0;
        if (wParam == (WPARAM)NID_MULTIEDIT_DOWN)
            return OnScrollDown() != 0;
        if (wParam == (WPARAM)NID_MULTIEDIT_BAR)
            return OnScrollBar(lParam) != 0;
        return 0;
    }
    if (Msg == (UINT)NIM_COMPOSITION)
        return OnComposition(wParam, lParam) != 0;
    if (Msg == (UINT)NIM_RESULT)
        return OnResult(wParam, lParam) != 0;
    if (Msg == (UINT)NIM_GETCARET)
        return OnGetCaret(wParam, lParam) != 0;
    if (Msg == (UINT)NIM_READING)
        return OnReading(wParam, lParam) != 0;
    if (Msg == (UINT)NIM_ATTRIBUTE)
        return OnAttribute(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1001a440, NCMultiEdit, WindowProc)

// 0x1001a5a0 - the caret's row stays among the visible ones
void NCMultiEdit::ScrollToCaret()
{
    if (TopLine + VisibleRows <= CaretRow)
        TopLine = CaretRow - VisibleRows + 1;
    if (CaretRow < TopLine)
        TopLine = CaretRow;
}
NW_HOOK_METHOD(0x1001a5a0, NCMultiEdit, ScrollToCaret)

// stock body shared with NCEditBox's (0x10009140)
void NCMultiEdit::SetFocus()
{
    NCWnd::SetFocus();
    IMEManager->Activate();
}

// 0x1001a5e0 (vslot 29)
void NCMultiEdit::KillFocus()
{
    IMEManager->Deactivate();
    NCWnd::KillFocus();
}
NW_HOOK_METHOD(0x1001a5e0, NCMultiEdit, KillFocus)

// 0x1001a600 (vslot 34) - blinks the caret, whatever the timer
INT NCMultiEdit::OnTimer(INT TimerID)
{
    bCaretOn = !bCaretOn;
    return 1;
}
NW_HOOK_METHOD(0x1001a600, NCMultiEdit, OnTimer)

// 0x1001b2c0 (vslot 69) - answers with the caret position the painters left
INT NCMultiEdit::OnGetCaret(WPARAM wParam, LPARAM lParam)
{
    if (wParam)
        SendTo((NWndEntry*)wParam, NIM_CARET, IMECaretX, IMECaretY);
    return 1;
}
NW_HOOK_METHOD(0x1001b2c0, NCMultiEdit, OnGetCaret)

// 0x1001b2f0 (vslot 70)
INT NCMultiEdit::OnReading(WPARAM wParam, LPARAM lParam)
{
    bReading = 1;
    Reading.Empty();
    TCHAR* Str = IMEManager->GetReadingStr();
    if (Str)
    {
        ReadingParam = lParam;
        Reading = Str;
    }
    return 1;
}
NW_HOOK_METHOD(0x1001b2f0, NCMultiEdit, OnReading)

// 0x1001b340 (vslot 71)
INT NCMultiEdit::OnAttribute(WPARAM wParam, LPARAM lParam)
{
    bReading = 0;
    Reading.Empty();
    return 1;
}
NW_HOOK_METHOD(0x1001b340, NCMultiEdit, OnAttribute)

// 0x1001b360 (vslot 66) - the scroll bar was dragged
INT NCMultiEdit::OnScrollBar(INT Value)
{
    TopLine = Value;
    return 1;
}
NW_HOOK_METHOD(0x1001b360, NCMultiEdit, OnScrollBar)

// 0x1001b1a0 - the composing characters, on white while the caret blinks on
void NCMultiEdit::PaintCompositionKR(FString Text, INT X, INT Y, NCGDevice* Device)
{
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *Text, 0, 0, 0, 0, 0);
    if (bCaretOn)
    {
        Device->DrawStretchedTex(LineGap + X, Y, Size.cx, 12, WhiteTex);
        Device->DrawText(LineGap + X, Y, 0xFF000000, *Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                         0);
    }
    else
        Device->DrawText(LineGap + X, Y, 0xFFFFFFFF, *Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                         0);
}
NW_HOOK_METHOD(0x1001b1a0, NCMultiEdit, PaintCompositionKR)

// 0x1001a620 - NCEditBox's Japanese painters (PaintCompositionJP, PaintComposingJP) in one, for
// the row's part of the composition (Text, from CompStart; the characters drawn come from
// Comp): the lines by attribute along the clauses (from the clause the row starts in), then
// the dotted underline of the characters still being typed. The first target character of
// the composition (*Out still 0) gives the IME its caret, below the row (RowNum).
void NCMultiEdit::PaintCompositionJP(FString Text, INT CompStart, INT X, INT Y, INT RowNum,
                                     NCGDevice* Device, INT* Out)
{
    SIZE Size;
    INT LineY = Y + 13;
    NCIME* IME = IMEManager->JapaneseIME;
    INT Offset = 0;
    UBOOL bTarget = 0;
    INT ClauseIdx = 0;
    for (INT c = 0; c < IME->CompClauseLen; c++)
    {
        if ((INT)IME->CompClause[c] > CompStart)
        {
            ClauseIdx = c - 1;
            break;
        }
    }
    DWORD* Clause = &IME->CompClause[ClauseIdx + 1];
    for (INT i = 0; i < Text.Len(); i++)
    {
        BYTE* Attr = &IME->CompAttr[i + CompStart];
        if ((*Attr & 1) && !bTarget)
        {
            bTarget = 1;
            if (!*Out)
            {
                IMECaretX = LineGap + Offset + X;
                IMECaretY = RowNum * 15 + 2;
                *Out = 1;
            }
        }
        INT CharX = LineGap + Offset + X;
        INT CharW = Device->DrawText(CharX, Y, 0xFFFFFFFF, *Comp.Mid(i + CompStart, 1), 0, 0, 0,
                                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Offset += CharW;
        INT EndX = LineGap + Offset + X;
        if (*Clause == (DWORD)(i + CompStart + 1))
        {
            Clause++;
            EndX--;
        }
        if (IsFocused())
        {
            if ((*Attr & 1) && CharW > 0)
            {
                Device->DrawLine(CharX, LineY, EndX, LineY + 1, 0xFFFFFFFF, 0);
                Device->DrawLine(CharX, LineY, EndX, LineY, 0xFFFFFFFF, 0);
            }
            if ((*Attr & 3) && CharW > 0)
                Device->DrawLine(CharX, LineY, EndX, LineY, 0xFFFFFFFF, 0);
            if (*Attr & 2)
                Device->DrawLine(CharX, LineY, EndX, LineY, 0xFFFFFFFF, 0);
            if (*Attr & 4)
                Device->DrawLine(CharX, LineY, EndX, LineY, 0xFFFF0000, 0);
        }
    }
    INT UnderlineW = 0;
    INT StartX = 0;
    for (INT j = 0; j < Text.Len(); j++)
    {
        INT Pos = CompStart + j;
        NCGDevice::GetTextExtent(&Size, *Comp.Mid(Pos, 1), 0, 0, 0, 0, 0);
        NCIME* JP = IMEManager->JapaneseIME;
        if (JP->CompAttr[Pos] == ATTR_INPUT)
        {
            if (j == 0 || JP->CompAttr[Pos - 1] != ATTR_INPUT)
            {
                NCGDevice::GetTextExtent(&Size, *Comp.Mid(CompStart, j), 0, 0, 0, 0, 0);
                StartX = Size.cx;
                NCGDevice::GetTextExtent(&Size, *Comp.Mid(Pos, 1), 0, 0, 0, 0, 0);
                UnderlineW += Size.cx;
            }
            else
            {
                NCGDevice::GetTextExtent(&Size, *Comp.Mid(Pos, 1), 0, 0, 0, 0, 0);
                UnderlineW += Size.cx;
            }
        }
    }
    if (UnderlineW > 0)
    {
        for (INT k = 0; k < UnderlineW; k += 3)
            Device->DrawText(LineGap + k + StartX + X, Y + 3, 0xFFFFFFFF, TEXT("."), 0, 0, 0, L2FONT_Normal,
                             0, 0, 0, 0, 0, 0);
    }
}
NW_HOOK_METHOD(0x1001a620, NCMultiEdit, PaintCompositionJP)

// 0x1001aad0 - NCEditBox's Taiwanese painters in one, for the row's part of the composition
// (Text, from CompStart), with the clause the row starts in: the target clause black on gray
// while focused, bit 2 underlined in red, and for the IME types 5 and 7 the other characters in
// red; a dotted underline for the other types. The first target character gives the IME its
// caret (*Out still 0; the Y is negative, the Taiwanese IME turns it); only when that character
// is 0 px wide does the reading string show there (as NCEditBox's ClauseW): a column of 12 px
// cells below the row for the type 6 (error position on in red on gray, the rest in blue), in
// the line black on gray for the type 4.
void NCMultiEdit::PaintCompositionTW(FString Text, INT CompStart, INT X, INT Y, INT RowNum,
                                     NCGDevice* Device, INT* Out)
{
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *Reading, 0, 0, 0, 0, 0);
    DWORD ReadingW = Size.cx;
    if (ReadingW > 0 && GConsole->GetIMEType() == 6)
        ReadingW = 12;
    NCIME* IME = IMEManager->TaiwanIME;
    INT Offset = 0;
    INT ClauseX = 0;
    UBOOL bTarget = 0;
    UBOOL bClauseX = 0;
    INT ClauseIdx = 0;
    for (INT c = 0; c < IME->CompClauseLen; c++)
    {
        if ((INT)IME->CompClause[c] > CompStart)
        {
            ClauseIdx = c - 1;
            break;
        }
    }
    DWORD* Clause = &IME->CompClause[ClauseIdx + 1];
    for (INT i = 0; i < Text.Len(); i++)
    {
        INT CharX = LineGap + Offset + X;
        INT CharW = Device->DrawText(CharX, Y, 0xFFFFFFFF, *Text.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0,
                                     0, 0, 0, 0);
        BYTE* Attr = &IME->CompAttr[i + CompStart];
        if ((*Attr & 1) && !bTarget)
        {
            bTarget = 1;
            if (!*Out)
            {
                IMECaretX = LineGap + Offset + X;
                IMECaretY = -2 - RowNum * 15;
                *Out = 1;
            }
            if (!CharW)
            {
                ClauseX = Offset;
                bClauseX = 1;
            }
        }
        Offset += CharW;
        INT EndX = LineGap + Offset + X;
        if (*Clause == (DWORD)(i + CompStart + 1))
        {
            Clause++;
            EndX--;
        }
        if (IsFocused())
        {
            INT Marks = 0;
            if ((*Attr & 1) && CharW > 0)
            {
                Device->DrawStretchedTex(CharX, Y, EndX - CharX, 12, FindTexture(TEXT("NWindow.GrayTexture")));
                Device->DrawText(CharX, Y, 0xFF000000, *Text.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                                 0, 0);
                Marks = 1;
            }
            if (*Attr & 4)
            {
                Device->DrawLine(CharX, Y + 13, EndX, Y + 13, 0xFFFF0000, 0);
                Marks++;
            }
            if (!Marks && (GConsole->GetIMEType() == 5 || GConsole->GetIMEType() == 7))
                Device->DrawText(CharX, Y, 0xFFFF0000, *Text.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                                 0, 0);
        }
    }
    if (Text.Len() > 0 && GConsole->GetIMEType() != 5 && GConsole->GetIMEType() != 7)
    {
        NCGDevice::GetTextExtent(&Size, *Text, 0, 0, 0, 0, 0);
        for (INT k = 0; k < Size.cx; k += 3)
            Device->DrawText(X + k, Y + 3, 0xFFFFFFFF, TEXT("."), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    if (bReading && bClauseX)
    {
        INT RX = LineGap + ClauseX + X;
        if (GConsole->GetIMEType() == 6)
        {
            Device->SetClip(ScreenX, ScreenY, Width, Height + 100);
            INT RY = Y + 6;
            Device->DrawStretchedTex(RX, RY, 12, 48, FindTexture(TEXT("NWindow.WhiteTexture")));
            for (INT k = 0; k < Reading.Len(); k++)
            {
                if (k >= ReadingParam)
                {
                    Device->DrawStretchedTex(RX, RY, 12, 12, FindTexture(TEXT("NWindow.GrayTexture")));
                    Device->DrawText(RX, RY, 0xFFFF0000, *Reading.Mid(k, 1), 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                                     0, 0, 0);
                }
                else
                    Device->DrawText(RX, RY, 0xFF0000FF, *Reading.Mid(k, 1), 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                                     0, 0, 0);
                RY += 12;
            }
            Device->ReleaseClip();
        }
        else if (GConsole->GetIMEType() == 4)
        {
            Device->DrawStretchedTex(RX, Y, ReadingW, 12, FindTexture(TEXT("NWindow.GrayTexture")));
            Device->DrawText(RX, Y, 0xFF000000, *Reading, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
    }
}
NW_HOOK_METHOD(0x1001aad0, NCMultiEdit, PaintCompositionTW)

// 0x1001b430 - the Chinese IME's part (nothing composed hides the composition window). The
// IME type 8 draws the row's part of the composition with a dotted underline; the first
// target character (or typed one: that test reads the attribute at the row's index, not the
// composition's, as in the stock code) goes black on white with the IME's caret 5 px past its
// start and 15 px down, where the composition window shows the reading string (hidden without
// one) and the candidate box goes; the caret follows the row's part when the whole composition
// has neither. The types 9 to 12 put the composition window at the row's start 15 px down (9
// and 10 fill it with the row's part), the candidate box 105 px right of the row's start, and
// blink the caret at the row's start. Only the type 10 shows the window's caret.
void NCMultiEdit::PaintCompositionCN(FString Text, INT CompStart, INT X, INT Y, INT RowNum,
                                     NCGDevice* Device, INT* Out)
{
    if (IMEManager->IsNotComposing())
    {
        ChineseCompositeBox(IMEManager)->HideWindow();
        return;
    }
    NCChineseIME* IME = (NCChineseIME*)IMEManager->ChineseIME;
    NCCandidateBox* Cand = IME->CandidateBox;
    INT CandW = 0;
    INT CandH = 0;
    if (Cand)
    {
        CandH = Cand->Height;
        CandW = Cand->Width;
    }
    NCCompositeBox* Box = IME->CompositeBox;
    if (GConsole->GetIMEType() == 10 || GConsole->GetIMEType() == 9)
    {
        IMECaretX = LineGap + X + 105;
        IMECaretY = Y + 15;
        Box->SetText(*Text);
        Box->SetWindowRect(LineGap + X, IMECaretY, 100, 15);
        Box->ShowWindow();
        if (Cand)
            Cand->SetWindowRect(IMECaretX, IMECaretY, CandW, CandH);
        if (IsFocused() && bCaretOn)
            Device->DrawText(LineGap + X - 2, Y, 0xFFFFFFFF, TEXT("|"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                             0, 0);
    }
    else if (GConsole->GetIMEType() == 11 || GConsole->GetIMEType() == 12)
    {
        IMECaretX = LineGap + X + 105;
        IMECaretY = Y + 15;
        Box->SetWindowRect(LineGap + X, Y + 15, 100, 15);
        Box->ShowWindow();
        if (Cand)
            Cand->SetWindowRect(IMECaretX, IMECaretY, CandW, CandH);
        if (IsFocused() && bCaretOn)
            Device->DrawText(LineGap + X - 2, Y, 0xFFFFFFFF, TEXT("|"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                             0, 0);
    }
    else if (GConsole->GetIMEType() == 8)
    {
        SIZE Size;
        Device->DrawText(LineGap + X, Y, 0xFFFFFFFF, *Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        INT Offset = 0;
        NCGDevice::GetTextExtent(&Size, *Text, 0, 0, 0, 0, 0);
        INT TextW = Size.cx;
        for (INT k = 0; k < TextW; k += 3)
            Device->DrawText(LineGap + k + X - 2, Y + 3, 0xFFFFFFFF, TEXT("."), 0, 0, 0, L2FONT_Normal, 0,
                             0, 0, 0, 0, 0);
        INT i;
        for (i = 0; i < Text.Len(); i++)
        {
            NCGDevice::GetTextExtent(&Size, *Text.Mid(i, 1), 0, 0, 0, 0, 0);
            NCIME* CN = IMEManager->ChineseIME;
            if (CN->CompAttr[i + CompStart] == ATTR_TARGET_CONVERTED || CN->CompAttr[i] == ATTR_INPUT)
            {
                IMECaretX = LineGap + Offset + X + 5;
                IMECaretY = Y + 15;
                break;
            }
            Offset += Size.cx;
        }
        NCGDevice::GetTextExtent(&Size, *Text.Mid(i, 1), 0, 0, 0, 0, 0);
        INT CharW = Size.cx;
        if (IsFocused() && bCaretOn && i == Comp.Len() - CompStart)
        {
            INT j;
            for (j = 0; j < Comp.Len(); j++)
            {
                BYTE Attr = IMEManager->ChineseIME->CompAttr[j];
                if (Attr == ATTR_TARGET_CONVERTED || Attr == ATTR_INPUT)
                    break;
            }
            if (j == Comp.Len())
                Device->DrawText(LineGap + TextW + X - 2, Y, 0xFFFFFFFF, TEXT("|"), 0, 0, 0, L2FONT_Normal, 0,
                                 0, 0, 0, 0, 0);
        }
        Device->DrawStretchedTex(LineGap + Offset + X, Y, CharW, 12, FindTexture(TEXT("NWindow.WhiteTexture")));
        Device->DrawText(LineGap + Offset + X, Y, 0xFF000000, *Text.Mid(i, 1), 0, 0, 0, L2FONT_Normal, 0, 0,
                         0, 0, 0, 0);
        Box->SetText(*Reading);
        Box->SetWindowRect(IMECaretX, IMECaretY, 50, 15);
        if (Reading.Len())
            Box->ShowWindow();
        else
            Box->HideWindow();
        if (Cand)
            Cand->SetWindowRect(IMECaretX, IMECaretY, CandW, CandH);
    }
    if (GConsole->GetIMEType() == 10)
        Box->SetShowCaret(1);
    else
        Box->SetShowCaret(0);
}
NW_HOOK_METHOD(0x1001b430, NCMultiEdit, PaintCompositionCN)

// 0x1001ba00 (vslot 61) - rows per notch; scrolling down is bounded by the number of *lines*
// (not rows)
INT NCMultiEdit::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    SWORD Delta = (SWORD)((SWORD)HIWORD(wParam) / 120);
    if (Delta > 0)
    {
        if (TopLine > Delta)
            TopLine -= Delta;
        else
            TopLine = 0;
    }
    else
    {
        INT Visible = VisibleRows;
        INT D = Delta;
        if (Lines.Num() >= Visible)
        {
            if (Lines.Num() + (D - TopLine) > Visible)
                TopLine -= D;
            else
                TopLine = Lines.Num() - Visible;
        }
        else if (Lines.Num() + (D - TopLine) > Visible)
            TopLine -= D;
        else
            TopLine = 0;
    }
    ScrollBar->SetValue(TopLine);
    return 1;
}
NW_HOOK_METHOD(0x1001ba00, NCMultiEdit, OnMouseWheel)

// 0x1001bad0 (vslot 64)
INT NCMultiEdit::OnScrollUp()
{
    if (Lines.Num() > VisibleRows)
    {
        if (--TopLine < 0)
            TopLine = 0;
        ScrollBar->SetValue(TopLine);
    }
    return 1;
}
NW_HOOK_METHOD(0x1001bad0, NCMultiEdit, OnScrollUp)

// 0x1001bb10 (vslot 65)
INT NCMultiEdit::OnScrollDown()
{
    INT Visible = VisibleRows;
    if (Lines.Num() > Visible)
    {
        TopLine++;
        if (TopLine > Lines.Num() - Visible)
            TopLine = Lines.Num() - Visible;
        ScrollBar->SetValue(TopLine);
    }
    return 1;
}
NW_HOOK_METHOD(0x1001bb10, NCMultiEdit, OnScrollDown)

// 0x1001bb70 - repeat period of both scroll buttons
void NCMultiEdit::SetRepeatDelay(INT Delay)
{
    if (UpButton && DownButton)
    {
        UpButton->RepeatDelay = Delay;
        DownButton->RepeatDelay = Delay;
    }
}
NW_HOOK_METHOD(0x1001bb70, NCMultiEdit, SetRepeatDelay)

// 0x1001bba0 - the range is given the scroll position for a total (sic)
void NCMultiEdit::InitScrollBar()
{
    SetRepeatDelay(200);
    ScrollBar->SetRange(TopLine, VisibleRows);
    ScrollBar->SetValue(TopLine);
}
NW_HOOK_METHOD(0x1001bba0, NCMultiEdit, InitScrollBar)

// 0x1001bbe0 (vslot 63) - the bar counts lines, not rows
void NCMultiEdit::UpdateScrollBar()
{
    ScrollBar->SetRange(Lines.Num(), VisibleRows);
    ScrollBar->SetValue(TopLine);
}
NW_HOOK_METHOD(0x1001bbe0, NCMultiEdit, UpdateScrollBar)

// 0x1001be80 - screen rows of a line, the composition counted at the caret
INT NCMultiEdit::CountRows(INT Line)
{
    if (Line >= NumLines)
        return 0;
    INT Rows = 1;
    INT X = 0;
    FString Text(Lines.GetLine(Line)->Text);
    FString Ch;
    SIZE Size;
    INT i;
    for (i = 0; i < Text.Len(); i++)
    {
        if (Line == CaretLine && i == CaretCol && Comp.Len())
        {
            for (INT k = 0; k < Comp.Len(); k++)
            {
                Ch = Comp.Mid(k, 1);
                NCGDevice::GetTextExtent(&Size, *Ch, 0, 0, 0, 0, 0);
                X += Size.cx;
                if (X > Width - 36)
                {
                    X = Size.cx;
                    Rows++;
                }
            }
        }
        Ch = Text.Mid(i, 1);
        NCGDevice::GetTextExtent(&Size, *Ch, 0, 0, 0, 0, 0);
        X += Size.cx;
        if (X > Width - 36)
        {
            X = Size.cx;
            Rows++;
        }
    }
    if (Line == CaretLine && i == CaretCol && Comp.Len())
    {
        for (INT k = 0; k < Comp.Len(); k++)
        {
            Ch = Comp.Mid(k, 1);
            NCGDevice::GetTextExtent(&Size, *Ch, 0, 0, 0, 0, 0);
            X += Size.cx;
            if (X > Width - 36)
            {
                X = Size.cx;
                Rows++;
            }
        }
    }
    return Rows;
}
NW_HOOK_METHOD(0x1001be80, NCMultiEdit, CountRows)

// 0x1001c120 - adds the rows a line gained since OldRows to TotalRows (it never takes any away
// here); the composition counts once: its loop clears bComp
void NCMultiEdit::UpdateRowCount(INT Line, INT OldRows, UBOOL bComp)
{
    if (Line >= NumLines)
        return;
    INT Rows = 1;
    INT X = 0;
    FString Text(Lines.GetLine(Line)->Text);
    FString Ch;
    SIZE Size;
    INT i;
    for (i = 0; i < Text.Len(); i++)
    {
        if (bComp && i == CaretCol && Line == CaretLine)
        {
            for (INT k = 0; k < Comp.Len(); k++)
            {
                bComp = 0;
                Ch = Comp.Mid(k, 1);
                NCGDevice::GetTextExtent(&Size, *Ch, 0, 0, 0, 0, 0);
                X += Size.cx;
                if (X > Width - 36)
                {
                    X = Size.cx;
                    Rows++;
                }
            }
        }
        Ch = Text.Mid(i, 1);
        NCGDevice::GetTextExtent(&Size, *Ch, 0, 0, 0, 0, 0);
        X += Size.cx;
        if (X > Width - 36)
        {
            Rows++;
            X = Size.cx;
        }
    }
    if (bComp && i == CaretCol && Line == CaretLine)
    {
        for (INT k = 0; k < Comp.Len(); k++)
        {
            Ch = Comp.Mid(k, 1);
            NCGDevice::GetTextExtent(&Size, *Ch, 0, 0, 0, 0, 0);
            X += Size.cx;
            if (X > Width - 36)
            {
                X = Size.cx;
                Rows++;
            }
        }
    }
    if (Rows > OldRows)
        TotalRows += Rows - OldRows;
}
NW_HOOK_METHOD(0x1001c120, NCMultiEdit, UpdateRowCount)

// 0x1001c3d0 - counts every screen row again and puts the caret on its row. A character that
// does not fit is measured again on the next row. A line ending in '\n' or '\r' adds a row.
// With Param set the caret is not moved once its line has wrapped before it. The size of a line
// that cannot be fetched is the last one measured (the stock code does not clear it).
void NCMultiEdit::RecalcRows(INT Param)
{
    INT LineLen = 0;
    INT Wraps = 0;
    TotalRows = 1;
    Unknown210 = Param;
    UBOOL bCaretPlaced = 0;
    SIZE Size;
    Size.cx = 0;
    Size.cy = 0;
    for (INT Line = 0; Line < Lines.Num(); Line++)
    {
        INT X = 0;
        if (Line == CaretLine)
        {
            if (Lines.GetLine(Line))
                LineLen = Lines.GetLine(Line)->Text.Len() + 1;
            for (INT Col = 0; Col < LineLen; Col++)
            {
                if (Col != CaretCol)
                {
                    if (Lines.GetLine(Line))
                        NCGDevice::GetTextExtent(&Size, *Lines.GetLine(Line)->Text.Mid(Col, 1), 0, 0,
                                                 0, 0, 0);
                    X += Size.cx;
                    if (X > Width - 36)
                    {
                        TotalRows++;
                        X = 0;
                        Col--;
                    }
                    continue;
                }
                if (!bCaretPlaced && (!Param || !Wraps))
                {
                    CaretX = X;
                    CaretRow = TotalRows - 1;
                }
                if (!IMEManager->IsNotComposing())
                {
                    if (!bCaretPlaced)
                    {
                        for (INT k = 0; k < Comp.Len() + 1; k++)
                        {
                            // the IME's cursor inside the composition gets the caret
                            if (GLanguageType == 2 || GLanguageType == 0 || GLanguageType == 3)
                            {
                                NCIME* IME = GLanguageType == 2   ? IMEManager->JapaneseIME
                                             : GLanguageType == 0 ? IMEManager->KoreanIME
                                                                  : IMEManager->TaiwanIME;
                                if (IME->CursorPos == k)
                                {
                                    CaretX = X;
                                    CaretRow = TotalRows - 1;
                                }
                            }
                            if (GLanguageType == 4 && GConsole->GetIMEType() != 8)
                                continue;
                            NCGDevice::GetTextExtent(&Size, *Comp.Mid(k, 1), 0, 0, 0, 0, 0);
                            X += Size.cx;
                            if (X > Width - 36)
                            {
                                TotalRows++;
                                X = 0;
                                k--;
                            }
                        }
                    }
                    if (Lines.GetLine(Line))
                        NCGDevice::GetTextExtent(&Size, *Lines.GetLine(Line)->Text.Mid(Col, 1), 0, 0,
                                                 0, 0, 0);
                    X += Size.cx;
                    if (X > Width - 36)
                    {
                        bCaretPlaced = 1;
                        TotalRows++;
                        X = 0;
                        Col--;
                    }
                }
                else
                {
                    if (Lines.GetLine(Line))
                        NCGDevice::GetTextExtent(&Size, *Lines.GetLine(Line)->Text.Mid(Col, 1), 0, 0,
                                                 0, 0, 0);
                    X += Size.cx;
                    if (X > Width - 36)
                    {
                        TotalRows++;
                        X = 0;
                        Col--;
                        Wraps++;
                    }
                }
            }
        }
        else
        {
            if (Lines.GetLine(Line))
                LineLen = Lines.GetLine(Line)->Text.Len();
            for (INT Col = 0; Col < LineLen; Col++)
            {
                if (Lines.GetLine(Line))
                    NCGDevice::GetTextExtent(&Size, *Lines.GetLine(Line)->Text.Mid(Col, 1), 0, 0, 0,
                                             0, 0);
                X += Size.cx;
                if (X > Width - 36)
                {
                    TotalRows++;
                    X = 0;
                    Col--;
                }
            }
        }
        if (Lines.GetLine(Line))
        {
            if (Lines.GetLine(Line)->Text.Mid(Lines.GetLine(Line)->Text.Len() - 1, 1) == TEXT("\n") ||
                Lines.GetLine(Line)->Text.Mid(Lines.GetLine(Line)->Text.Len() - 1, 1) == TEXT("\r"))
                TotalRows++;
        }
    }
    ScrollToCaret();
}
NW_HOOK_METHOD(0x1001c3d0, NCMultiEdit, RecalcRows)

// 0x1001c9e0 - a result string goes into the line at the caret (the composition keeps what is
// left of it); a composition string only replaces Comp
void NCMultiEdit::InsertString(TCHAR* Str, UBOOL bResult)
{
    if (bResult)
    {
        if (Str && Lines.GetLine(CaretLine))
        {
            INT OldRows = CountRows(CaretLine);
            FString Line(Lines.GetLine(CaretLine)->Text);
            INT Length = wcslen(Str);
            FString S(Str);
            FString L = Line.Left(CaretCol);
            FString R = Line.Right(Line.Len() - CaretCol);
            Lines.GetLine(CaretLine)->Text = L + S + R;
            CaretCol += Length;
            for (INT i = 0; i < S.Len(); i++)
            {
                SIZE Size;
                NCGDevice::GetTextExtent(&Size, *S.Mid(i, 1), 0, 0, 0, 0, 0);
                CaretX += Size.cx;
                if (CaretX > Width - 36)
                {
                    CaretRow++;
                    CaretX = Size.cx;
                }
            }
            if (Comp.Len() > appStrlen(Str))
                Comp = Comp.Right(Comp.Len() - appStrlen(Str));
            else
                Comp.Empty();
            UpdateRowCount(CaretLine, OldRows, 1);
        }
    }
    else if (Str)
    {
        INT OldRows = CountRows(CaretLine);
        Comp = FString(Str);
        UpdateRowCount(CaretLine, OldRows, 1);
    }
    ScrollToCaret();
}
NW_HOOK_METHOD(0x1001c9e0, NCMultiEdit, InsertString)

// 0x1001cd00 (vslot 35) - NControl::OnCreate is not called
INT NCMultiEdit::OnCreate()
{
    BackTex = FindTexture(TEXT("NWindow.chatback"));
    WhiteTex = FindTexture(TEXT("NWindow.WhiteTexture"));
    IMEManager = new NCIMEManager(0);
    CreateChild(IMEManager, NWS_VISIBLE);
    CaretTimer = SetTimer(TIMER_MultiEditCaret, 500, 0);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, TEXT("\xAC00"), 0, 0, 0, 0, 0);
    CharW = Size.cx;
    CharH = Size.cy;
    CharH2 = Size.cy;
    VisibleRows = (Height - 2) / (Size.cy + 2);
    LineGap = 2;
    NumLines = 1;
    INT W = LocalW;
    INT H = LocalH;

    UpButton = new NCPushButton(TEXT("L2UI.NWindow.WindowScrollUp"),
                                TEXT("L2UI.NWindow.WindowScrollUp"));
    UpButton->ID = NID_MULTIEDIT_UP;
    UpButton->SetWindowRect(W + 2, 0, 12, 12);
    CreateChild(UpButton, NWS_VISIBLE);

    DownButton = new NCPushButton(TEXT("L2UI.NWindow.WindowScrollDown"),
                                  TEXT("L2UI.NWindow.WindowScrollDown"));
    DownButton->ID = NID_MULTIEDIT_DOWN;
    DownButton->SetWindowRect(W + 2, H - 12, 12, 12);
    CreateChild(DownButton, NWS_VISIBLE);

    ScrollBar = new NCSliderBar(TEXT("L2UI.NWindow.WindowScrollBar1"),
                                TEXT("L2UI.NWindow.WindowScrollBar2"),
                                TEXT("L2UI.NWindow.WindowScrollBar3"));
    ScrollBar->ID = NID_MULTIEDIT_BAR;
    ScrollBar->SetWindowRect(W + 2, 12, 12, H >= 24 ? H - 24 : 0);
    CreateChild(ScrollBar, NWS_VISIBLE);
    ScrollBar->DoNothing(0);
    InitScrollBar();
    return 1;
}
NW_HOOK_METHOD(0x1001cd00, NCMultiEdit, OnCreate)

// 0x1001d040 - x of a caret column, the line wrapped as it is drawn; a column past the end
// leaves CaretX alone
void NCMultiEdit::SetCaretX(INT Line, INT Col)
{
    if (Line >= NumLines)
        return;
    FString Text;
    if (Lines.GetLine(Line))
        Text = Lines.GetLine(Line)->Text;
    INT X = 0;
    if (Col > Text.Len())
        return;
    for (INT i = 0; i < Col; i++)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, *Text.Mid(i, 1), 0, 0, 0, 0, 0);
        X += Size.cx;
        if (X > Width - 36)
            X = Size.cx;
    }
    CaretX = X;
}
NW_HOOK_METHOD(0x1001d040, NCMultiEdit, SetCaretX)

// 0x1001d150 - column of its line where a screen row starts (-1 past the text); bCompRow is set
// when the row starts inside the composition, the result is then an index into Comp
INT NCMultiEdit::GetRowStart(INT Row)
{
    INT CurRow = 0;
    if (Row >= TotalRows || Lines.Num() <= 0)
    {
        bCompRow = 0;
        return -1;
    }
    INT Line = 0;
    do
    {
        if (CurRow == Row)
        {
            if (Line == CaretLine && CaretCol == 0 && !IMEManager->IsNotComposing())
            {
                bCompRow = 1;
                return 0;
            }
            bCompRow = 0;
            return 0;
        }
        INT X = 0;
        INT LineLen = 0;
        if (Lines.GetLine(Line))
            LineLen = Lines.GetLine(Line)->Text.Len() + 1;
        for (INT Col = 0; Col < LineLen; Col++)
        {
            SIZE Size;
            if (Line == CaretLine && Col == CaretCol && !IMEManager->IsNotComposing() &&
                GConsole->GetIMEType() != 10 && GConsole->GetIMEType() != 9 &&
                GConsole->GetIMEType() != 11 && GConsole->GetIMEType() != 12)
            {
                for (INT k = 0; k < Comp.Len(); k++)
                {
                    NCGDevice::GetTextExtent(&Size, *Comp.Mid(k, 1), 0, 0, 0, 0, 0);
                    X += Size.cx;
                    if (X > Width - 36)
                    {
                        X = 0;
                        if (++CurRow == Row)
                        {
                            bCompRow = 1;
                            return k;
                        }
                        k--;
                    }
                }
            }
            if (Lines.GetLine(Line))
                NCGDevice::GetTextExtent(&Size, *Lines.GetLine(Line)->Text.Mid(Col, 1), 0, 0, 0, 0,
                                         0);
            else
                Size.cx = 0;
            X += Size.cx;
            if (X > Width - 36)
            {
                X = 0;
                if (++CurRow == Row)
                {
                    bCompRow = 0;
                    return Col;
                }
                Col--;
            }
        }
        CurRow++;
        Line++;
    } while (Line < Lines.Num());
    bCompRow = 0;
    return -1;
}
NW_HOOK_METHOD(0x1001d150, NCMultiEdit, GetRowStart)

// 0x1001d4c0 - column where the row starting at Start of Line ends (0: at the end of the line,
// -1 past the text). A row that starts inside the composition (bCompRow) measures the rest of
// it first; one that breaks inside the composition returns an index into Comp with bCompRow
// set. The line is fetched without a NULL test.
INT NCMultiEdit::GetRowEnd(INT Start, INT Line)
{
    FString Ch;
    Ch.Empty();
    INT X = 0;
    if (Line >= Lines.Num())
    {
        bCompRow = 0;
        return -1;
    }
    SIZE Size;
    if (bCompRow && !IMEManager->IsNotComposing() && Line == CaretLine)
    {
        if (!IsWindowedIME(GConsole->GetIMEType()))
        {
            INT CompLen = Comp.Len();
            for (INT i = Start; i < CompLen; i++)
            {
                Ch.Empty();
                Ch = Comp.Mid(i, 1);
                NCGDevice::GetTextExtent(&Size, *Ch, 0, 0, 0, 0, 0);
                X += Size.cx;
                if (X > Width - 36)
                    return i;
            }
        }
        Start = CaretCol;
    }
    INT LineLen = Lines.GetLine(Line)->Text.Len() + 1;
    for (INT Col = Start; Col < LineLen; Col++)
    {
        if (Line == CaretLine && !bCompRow && Col == CaretCol && !IMEManager->IsNotComposing() &&
            GConsole->GetIMEType() != 10 && GConsole->GetIMEType() != 9 &&
            GConsole->GetIMEType() != 11 && GConsole->GetIMEType() != 12 && CaretCol != Start)
        {
            for (INT k = 0; k < Comp.Len(); k++)
            {
                NCGDevice::GetTextExtent(&Size, *Comp.Mid(k, 1), 0, 0, 0, 0, 0);
                X += Size.cx;
                if (X > Width - 36)
                {
                    bCompRow = 1;
                    return k;
                }
            }
        }
        Ch.Empty();
        if (Lines.GetLine(Line))
            Ch = Lines.GetLine(Line)->Text.Mid(Col, 1);
        NCGDevice::GetTextExtent(&Size, *Ch, 0, 0, 0, 0, 0);
        X += Size.cx;
        if (X > Width - 36)
        {
            bCompRow = 0;
            return Col;
        }
    }
    if (Line + 1 == CaretLine && CaretCol == 0 && !IMEManager->IsNotComposing())
        bCompRow = 1;
    else
        bCompRow = 0;
    return 0;
}
NW_HOOK_METHOD(0x1001d4c0, NCMultiEdit, GetRowEnd)

// 0x1001d8e0 - line of a screen row (-1 past the text); unlike GetRowStart it does not count
// the position after a line's last character
INT NCMultiEdit::GetRowLine(INT Row)
{
    INT CurRow = 0;
    if (Row >= TotalRows || Lines.Num() <= 0)
        return -1;
    INT Line = 0;
    do
    {
        if (CurRow == Row)
            return Line;
        INT X = 0;
        INT LineLen = 0;
        if (Lines.GetLine(Line))
            LineLen = Lines.GetLine(Line)->Text.Len();
        for (INT Col = 0; Col < LineLen; Col++)
        {
            SIZE Size;
            if (Line == CaretLine && Col == CaretCol && !IMEManager->IsNotComposing() &&
                GConsole->GetIMEType() != 10 && GConsole->GetIMEType() != 9 &&
                GConsole->GetIMEType() != 11 && GConsole->GetIMEType() != 12)
            {
                for (INT k = 0; k < Comp.Len(); k++)
                {
                    NCGDevice::GetTextExtent(&Size, *Comp.Mid(k, 1), 0, 0, 0, 0, 0);
                    X += Size.cx;
                    if (X > Width - 36)
                    {
                        X = 0;
                        if (++CurRow == Row)
                            return Line;
                        k--;
                    }
                }
            }
            if (Lines.GetLine(Line))
                NCGDevice::GetTextExtent(&Size, *Lines.GetLine(Line)->Text.Mid(Col, 1), 0, 0, 0, 0,
                                         0);
            else
                Size.cx = 0;
            X += Size.cx;
            if (X > Width - 36)
            {
                X = 0;
                if (++CurRow == Row)
                    return Line;
                Col--;
            }
        }
        CurRow++;
        Line++;
    } while (Line < Lines.Num());
    return -1;
}
NW_HOOK_METHOD(0x1001d8e0, NCMultiEdit, GetRowLine)

// 0x1001dbd0 (vslot 68)
INT NCMultiEdit::OnResult(WPARAM wParam, LPARAM lParam)
{
    bReading = 0;
    Reading.Empty();
    TCHAR* Str = IMEManager->GetResultStr();
    if (Str)
    {
        InsertString(Str, 1);
        return 1;
    }
    Comp.Empty();
    return 1;
}
NW_HOOK_METHOD(0x1001dbd0, NCMultiEdit, OnResult)

// 0x1001dc20 (vslot 67) - the Chinese layouts show the composition in the IME's window, whose
// caret follows the length change
INT NCMultiEdit::OnComposition(WPARAM wParam, LPARAM lParam)
{
    bReading = 0;
    Reading.Empty();
    TCHAR* Str = IMEManager->GetCompStr();
    if (Str)
        InsertString(Str, 0);
    else
        Comp.Empty();
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
NW_HOOK_METHOD(0x1001dc20, NCMultiEdit, OnComposition)

// 0x1001dd60 - the lines joined (their '\n' included) in a buffer the box keeps; NULL when
// empty
TCHAR* NCMultiEdit::GetText()
{
    if (TextBuf)
    {
        delete[] TextBuf;
        TextBuf = NULL;
    }
    UINT Total = Lines.TotalLength();
    if (!Total)
        return NULL;
    TextBuf = new TCHAR[Total + 1];
    INT N = Lines.Num();
    INT Pos = 0;
    for (INT i = 0; i < N; i++)
    {
        INT Length = 0;
        if (Lines.GetLine(i))
        {
            Length = Lines.GetLine(i)->Text.Len();
            appMemcpy(TextBuf + Pos, *Lines.GetLine(i)->Text, Length * sizeof(TCHAR));
        }
        Pos += Length;
    }
    TextBuf[Pos] = 0;
    return TextBuf;
}
NW_HOOK_METHOD(0x1001dd60, NCMultiEdit, GetText)

// 0x1001e0f0 - a character at the caret; '\n' or '\r' ends the line there (and stays at its
// end), the rest of it going to a new line
void NCMultiEdit::InsertChar(TCHAR Ch)
{
    FString Right;
    FString Left;
    TCHAR Buf[260];
    appSprintf(Buf, TEXT("%c"), Ch);
    FString Str(Buf);
    if (Ch == '\n' || Ch == '\r')
    {
        Lines.Insert(CaretLine + 1);
        if (Lines.GetLine(CaretLine + 1) && Lines.GetLine(CaretLine))
        {
            Lines.GetLine(CaretLine + 1)->Text =
                Lines.GetLine(CaretLine)->Text.Right(Lines.GetLine(CaretLine)->Text.Len() - CaretCol);
            Lines.GetLine(CaretLine)->Text = Lines.GetLine(CaretLine)->Text.Left(CaretCol) + Str;
        }
        CaretLine++;
        NumLines = Lines.Num();
        CaretCol = 0;
        CaretX = 0;
        CaretRow++;
        TotalRows++;
    }
    else
    {
        INT OldRows = CountRows(CaretLine);
        if (Lines.GetLine(CaretLine))
        {
            Left = Lines.GetLine(CaretLine)->Text.Left(CaretCol);
            Right = Lines.GetLine(CaretLine)->Text.Right(Lines.GetLine(CaretLine)->Text.Len() - CaretCol);
            Lines.GetLine(CaretLine)->Text = Left + Str + Right;
        }
        CaretCol++;
        // the stock code measures the argument itself (its upper half is 0)
        TCHAR One[2] = { Ch, 0 };
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, One, 0, 0, 0, 0, 0);
        CaretX += Size.cx;
        if (CaretX > Width - 36)
        {
            CaretRow++;
            CaretX = Size.cx;
        }
        UpdateRowCount(CaretLine, OldRows, 0);
    }
    ScrollToCaret();
}
NW_HOOK_METHOD(0x1001e0f0, NCMultiEdit, InsertChar)

// 0x1001e500 - the characters one by one, as typed
void NCMultiEdit::SetText(const TCHAR* Text)
{
    INT Length = wcslen(Text);
    for (INT i = 0; i < Length; i++)
        InsertChar(Text[i]);
    UpdateScrollBar();
}
NW_HOOK_METHOD(0x1001e500, NCMultiEdit, SetText)

// 0x1001e550 (vslot 36)
INT NCMultiEdit::OnDestroy()
{
    CaretLine = 0;
    CaretCol = 0;
    Lines.Clear();
    if (TextBuf)
    {
        delete[] TextBuf;
        TextBuf = NULL;
    }
    if (CaretTimer)
    {
        KillTimer(CaretTimer);
        CaretTimer = 0;
    }
    if (UpButton)
    {
        RemoveChildWindow(UpButton->Entry);
        if (UpButton)
            delete UpButton;
        UpButton = NULL;
    }
    if (DownButton)
    {
        RemoveChildWindow(DownButton->Entry);
        if (DownButton)
            delete DownButton;
        DownButton = NULL;
    }
    if (ScrollBar)
    {
        RemoveChildWindow(ScrollBar->Entry);
        if (ScrollBar)
            delete ScrollBar;
        ScrollBar = NULL;
    }
    if (IMEManager)
    {
        RemoveChildWindow(IMEManager->Entry);
        if (IMEManager)
            delete IMEManager;
        IMEManager = NULL;
    }
    NControl::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1001e550, NCMultiEdit, OnDestroy)

// 0x1001e670 - one empty line again
void NCMultiEdit::Clear()
{
    CaretLine = 0;
    CaretCol = 0;
    Lines.Clear();
    Lines.Insert(-1);
    NumLines = 1;
    TotalRows = 1;
    CaretLine = 0;
    CaretCol = 0;
    TopLine = 0;
    CaretX = 0;
    CaretRow = 0;
    bCompRow = 0;
    bReading = 0;
    if (TextBuf)
    {
        delete[] TextBuf;
        TextBuf = NULL;
    }
}
NW_HOOK_METHOD(0x1001e670, NCMultiEdit, Clear)

// The rows shown, from TopLine: the text of each row, the composition drawn at the caret by the
// painter of the client language, and the caret (Japanese: drawn apart). The width of what is
// drawn is kept across rows (TextW) and used where the stock code uses it.
static FORCEINLINE void PaintMultiEditText(NCMultiEdit* E, NCGDevice* Device)
{
    Device->DrawStretchedTex(0, 0, E->Width, E->Height, E->BackTex);
    INT Y = E->LineGap;
    INT TextW = 0;
    FString Seg;
    INT PainterOut = 0;
    INT Start = E->GetRowStart(E->TopLine);
    INT Line = E->GetRowLine(E->TopLine);
    for (INT Row = E->TopLine; Row < E->TopLine + E->VisibleRows; Row++)
    {
        if (Row >= E->TotalRows)
            break;
        INT RowNum = Row - E->TopLine + 1;
        INT End;
        if (E->bCompRow)
        {
            // the row starts inside the composition
            End = E->GetRowEnd(Start, Line);
            if (E->bCompRow)
            {
                // ... and ends inside it
                Seg = E->Comp.Mid(Start, End - Start);
                if (Seg.Len())
                {
                    if (GLanguageType == 2)
                    {
                        Device->DrawText(E->LineGap, Y, 0xFFFFFFFF, *Seg, 0, 0, 0, L2FONT_Normal, 0, 0,
                                         0, 0, 0, 0);
                        E->PaintCompositionJP(Seg, Start, 0, Y, RowNum, Device, &PainterOut);
                    }
                    else if (GLanguageType == 3)
                    {
                        Device->DrawText(E->LineGap, Y, 0xFFFFFFFF, *Seg, 0, 0, 0, L2FONT_Normal, 0, 0,
                                         0, 0, 0, 0);
                        E->PaintCompositionTW(Seg, Start, 0, Y, RowNum, Device, &PainterOut);
                    }
                    else if (GLanguageType == 4)
                        E->PaintCompositionCN(Seg, Start, 0, Y, RowNum, Device, &PainterOut);
                    else if (GLanguageType == 0)
                        E->PaintCompositionKR(Seg, 0, Y, Device);
                    else
                        Device->DrawText(E->LineGap, Y, 0xFFFFFFFF, *Seg, 0, 0, 0, L2FONT_Normal, 0, 0,
                                         0, 0, 0, 0);
                }
            }
            else
            {
                // ... and the line's text follows the rest of it
                Seg = E->Comp.Mid(Start, E->Comp.Len() - Start);
                TextW = 0;
                if (GLanguageType == 2)
                {
                    TextW = Device->DrawText(E->LineGap, Y, 0xFFFFFFFF, *Seg, 0, 0, 0, L2FONT_Normal, 0,
                                             0, 0, 0, 0, 0);
                    E->PaintCompositionJP(Seg, Start, 0, Y, RowNum, Device, &PainterOut);
                }
                else if (GLanguageType == 3)
                {
                    TextW = Device->DrawText(E->LineGap, Y, 0xFFFFFFFF, *Seg, 0, 0, 0, L2FONT_Normal, 0,
                                             0, 0, 0, 0, 0);
                    E->PaintCompositionTW(Seg, Start, 0, Y, RowNum, Device, &PainterOut);
                }
                else if (GLanguageType == 4)
                    E->PaintCompositionCN(Seg, Start, 0, Y, RowNum, Device, &PainterOut);
                else if (GLanguageType == 0)
                {
                    TextW = Device->DrawText(E->LineGap, Y, 0xFFFFFFFF, *Seg, 0, 0, 0, L2FONT_Normal, 0,
                                             0, 0, 0, 0, 0);
                    E->PaintCompositionKR(Seg, 0, Y, Device);
                }
                else
                    TextW = Device->DrawText(E->LineGap, Y, 0xFFFFFFFF, *Seg, 0, 0, 0, L2FONT_Normal, 0,
                                             0, 0, 0, 0, 0);
                if (E->Lines.GetLine(Line))
                {
                    if (End > 0)
                        Seg = E->Lines.GetLine(Line)->Text.Mid(E->CaretCol, End - E->CaretCol);
                    else
                        Seg = E->Lines.GetLine(Line)->Text.Mid(
                            E->CaretCol, E->Lines.GetLine(Line)->Text.Len() - E->CaretCol);
                    if (Seg.Len())
                        Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *Seg, 0, 0, 0,
                                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                }
            }
            if (Row == E->CaretRow && E->bCaretOn && GLanguageType == 2)
                Device->DrawText(E->CaretX + E->LineGap - 3, Y, 0xFFFFFFFF, TEXT("|"), 0, 0, 0,
                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
        else if (Row == E->CaretRow)
        {
            // the caret's row: the text before the caret, the composition, the rest
            TextW = 0;
            if (E->CaretX && E->Lines.GetLine(Line))
            {
                Seg = E->Lines.GetLine(Line)->Text.Mid(Start, E->CaretCol - Start);
                TextW = Device->DrawText(E->LineGap, Y, 0xFFFFFFFF, *Seg, 0, 0, 0, L2FONT_Normal, 0, 0,
                                         0, 0, 0, 0);
            }
            if (!E->IMEManager->IsNotComposing())
            {
                End = E->GetRowEnd(Start, Line);
                if (E->bCompRow)
                {
                    // the composition goes on in the next row
                    if (GLanguageType == 2)
                    {
                        Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *E->Comp.Mid(0, End), 0, 0,
                                         0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                        E->PaintCompositionJP(E->Comp.Mid(0, End), 0, TextW, Y, RowNum, Device,
                                              &PainterOut);
                    }
                    else if (GLanguageType == 3)
                    {
                        Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *E->Comp.Mid(0, End), 0, 0,
                                         0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                        E->PaintCompositionTW(E->Comp.Mid(0, End), 0, TextW, Y, RowNum, Device,
                                              &PainterOut);
                    }
                    else if (GLanguageType == 4)
                        E->PaintCompositionCN(E->Comp.Mid(0, End), 0, TextW, Y, RowNum, Device,
                                              &PainterOut);
                    else
                        Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *E->Comp.Mid(0, End), 0, 0,
                                         0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                }
                else
                {
                    INT CompW = 0;
                    if (GLanguageType == 2)
                    {
                        CompW = Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *E->Comp, 0, 0, 0,
                                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                        E->PaintCompositionJP(E->Comp, 0, TextW, Y, RowNum, Device, &PainterOut);
                    }
                    else if (GLanguageType == 3)
                    {
                        CompW = Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *E->Comp, 0, 0, 0,
                                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                        E->PaintCompositionTW(E->Comp, 0, TextW, Y, RowNum, Device, &PainterOut);
                    }
                    else if (GLanguageType == 4)
                    {
                        if (GConsole->GetIMEType() == 8)
                            CompW = Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *E->Comp, 0, 0,
                                                     0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                        E->PaintCompositionCN(E->Comp, 0, TextW, Y, RowNum, Device, &PainterOut);
                    }
                    else if (GLanguageType == 0)
                    {
                        CompW = Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *E->Comp, 0, 0, 0,
                                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                        E->PaintCompositionKR(E->Comp, TextW, Y, Device);
                    }
                    else
                        CompW = Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *E->Comp, 0, 0, 0,
                                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                    TextW += CompW;
                    if (E->Lines.GetLine(Line))
                    {
                        if (End > 0)
                            Seg = E->Lines.GetLine(Line)->Text.Mid(E->CaretCol, End - E->CaretCol);
                        else
                            Seg = E->Lines.GetLine(Line)->Text.Mid(
                                E->CaretCol, E->Lines.GetLine(Line)->Text.Len() - E->CaretCol);
                        if (Seg.Len())
                            Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *Seg, 0, 0, 0,
                                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                    }
                }
                if (E->IsFocused() && E->bCaretOn && GLanguageType == 2)
                    Device->DrawText(E->CaretX + E->LineGap, Y, 0xFFFFFFFF, TEXT("|"), 0, 0, 0,
                                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            }
            else
            {
                if (GConsole->GetIMEType() == 10 || GConsole->GetIMEType() == 9 ||
                    GConsole->GetIMEType() == 11 || GConsole->GetIMEType() == 8 ||
                    GConsole->GetIMEType() == 12)
                    ChineseCompositeBox(E->IMEManager)->HideWindow();
                // the caret goes after the text drawn so far on this row (TextW), not at CaretX
                if (E->IsFocused() && E->bCaretOn)
                    Device->DrawText(E->LineGap + TextW - 3, Y, 0xFFFFFFFF, TEXT("|"), 0, 0, 0,
                                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                End = E->GetRowEnd(Start, Line);
                if (E->Lines.GetLine(Line))
                {
                    if (Row + 1 < E->TotalRows && End)
                        Seg = E->Lines.GetLine(Line)->Text.Mid(E->CaretCol, End - E->CaretCol);
                    else
                        Seg = E->Lines.GetLine(Line)->Text.Mid(
                            E->CaretCol, E->Lines.GetLine(Line)->Text.Len() - E->CaretCol);
                    if (Seg.Len())
                        Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *Seg, 0, 0, 0,
                                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                }
            }
        }
        else if (Row + 1 < E->TotalRows)
        {
            End = E->GetRowEnd(Start, Line);
            if (E->bCompRow)
            {
                // the composition starts in this row, after the line's text
                if (E->Lines.GetLine(Line))
                {
                    if (E->CaretCol > 0)
                        Seg = E->Lines.GetLine(Line)->Text.Mid(Start, E->CaretCol - Start);
                    else
                        Seg = E->Lines.GetLine(Line)->Text.Mid(
                            Start, E->Lines.GetLine(Line)->Text.Len() - Start);
                    if (Seg.Len())
                        TextW = Device->DrawText(E->LineGap, Y, 0xFFFFFFFF, *Seg, 0, 0, 0,
                                                 L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                }
                if (GLanguageType == 2)
                {
                    Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *E->Comp.Mid(0, End), 0, 0, 0,
                                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                    E->PaintCompositionJP(E->Comp.Mid(0, End), 0, TextW, Y, RowNum, Device,
                                          &PainterOut);
                }
                else if (GLanguageType == 3)
                {
                    Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *E->Comp.Mid(0, End), 0, 0, 0,
                                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
                    E->PaintCompositionTW(E->Comp.Mid(0, End), 0, TextW, Y, RowNum, Device,
                                          &PainterOut);
                }
                else
                    Device->DrawText(E->LineGap + TextW, Y, 0xFFFFFFFF, *E->Comp.Mid(0, End), 0, 0, 0,
                                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            }
            else if (E->Lines.GetLine(Line))
            {
                if (End)
                    Seg = E->Lines.GetLine(Line)->Text.Mid(Start, End - Start);
                else
                    Seg = E->Lines.GetLine(Line)->Text.Mid(Start,
                                                           E->Lines.GetLine(Line)->Text.Len() - Start);
                if (Seg.Len())
                    TextW = Device->DrawText(E->LineGap, Y, 0xFFFFFFFF, *Seg, 0, 0, 0, L2FONT_Normal,
                                             0, 0, 0, 0, 0, 0);
            }
        }
        else
        {
            // the last row
            End = E->GetRowEnd(Start, Line);
            if (E->Lines.GetLine(Line))
            {
                Seg = E->Lines.GetLine(Line)->Text.Mid(Start, E->Lines.GetLine(Line)->Text.Len() - Start);
                if (Seg.Len())
                    Device->DrawText(E->LineGap, Y, 0xFFFFFFFF, *Seg, 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                                     0, 0, 0);
            }
        }
        Y += E->CharH + E->LineGap;
        Start = End;
        if (End == 0 && (!E->bCompRow || E->CaretCol == 0))
            Line++;
    }
}

// 0x1001e700 (vslot 50)
INT NCMultiEdit::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    PaintMultiEditText(this, Device);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x1001e700, NCMultiEdit, OnPaint)

// 0x1001fb60 (vslot 52) - printable ASCII, and the upper characters with the IME type 9
INT NCMultiEdit::OnChar(WPARAM wParam, LPARAM lParam)
{
    TCHAR Buf[20];
    if ((wParam >= 0x20 && wParam <= 0x7E) || (GConsole->GetIMEType() == 9 && wParam > 0x81))
    {
        appSprintf(Buf, TEXT("%c"), wParam);  // not used
        InsertChar((TCHAR)wParam);
    }
    return 1;
}
NW_HOOK_METHOD(0x1001fb60, NCMultiEdit, OnChar)

// 0x1001fbc0 (vslot 53) - the caret keys; with the IME type 18 (and the Chinese ones for some
// keys) a pending composition is committed first. Delete is Right then Backspace.
INT NCMultiEdit::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    INT IMEType = GConsole->GetIMEType();
    FString Ch;
    Ch.Empty();
    switch (wParam)
    {
    case VK_RETURN:
        if (IMEType == 18)
        {
            if (Comp.Len())
                InsertString((TCHAR*)*Comp, 1);
            SetText(TEXT("\n"));
        }
        else if (IMEManager->IsNotComposing() == 1)
            SetText(TEXT("\n"));
        break;

    case VK_LEFT:
        if (IMEType == 18 || IMEManager->IsNotComposing() == 1 || IMEType == 9 || IMEType == 11 ||
            IMEType == 10 || IMEType == 12)
        {
            INT Step;
            if (Comp.Len() && IMEType == 18)
            {
                Step = Comp.Len();
                InsertString((TCHAR*)*Comp, 1);
            }
            else
                Step = 1;
            if (CaretCol > 0)
            {
                Ch = Lines.GetLine(CaretLine)->Text.Mid(CaretCol - 1, 1);
                SIZE Size;
                NCGDevice::GetTextExtent(&Size, *Ch, 0, 0, 0, 0, 0);
                CaretCol -= Step;
                if (CaretX > 0)
                    CaretX -= Size.cx;
                else
                {
                    CaretRow--;
                    SetCaretX(CaretLine, CaretCol);
                }
            }
            else
            {
                if (CaretLine > 0)
                {
                    CaretLine--;
                    if (Lines.GetLine(CaretLine))
                        CaretCol = Lines.GetLine(CaretLine)->Text.Len() - 1;
                }
                if (CaretRow > 0)
                    CaretRow--;
                SetCaretX(CaretLine, CaretCol);
            }
        }
        ScrollToCaret();
        UpdateScrollBar();
        break;

    case VK_RIGHT:
        if (IMEType == 18 || IMEManager->IsNotComposing() == 1 || IMEType == 9 || IMEType == 11 ||
            IMEType == 10 || IMEType == 12)
        {
            if (Comp.Len() && IMEType == 18)
                InsertString((TCHAR*)*Comp, 1);
            if (Lines.GetLine(CaretLine))
            {
                INT Col = CaretCol;
                UBOOL bAdvance = 0;
                if (CaretLine + 1 < NumLines)
                {
                    if (Col < Lines.GetLine(CaretLine)->Text.Len() - 1)
                    {
                        Ch = Lines.GetLine(CaretLine)->Text.Mid(Col, 1);
                        bAdvance = 1;
                    }
                    else
                    {
                        // past the line's '\n': the start of the next line
                        CaretLine++;
                        CaretCol = 0;
                        CaretRow++;
                        CaretX = 0;
                    }
                }
                else if (Col < Lines.GetLine(CaretLine)->Text.Len())
                {
                    Ch = Lines.GetLine(CaretLine)->Text.Mid(Col, 1);
                    bAdvance = 1;
                }
                if (bAdvance)
                {
                    SIZE Size;
                    NCGDevice::GetTextExtent(&Size, *Ch, 0, 0, 0, 0, 0);
                    CaretX += Size.cx;
                    if (CaretX > Width - 36)
                    {
                        CaretRow++;
                        CaretX = Size.cx;
                    }
                    CaretCol++;
                }
            }
        }
        ScrollToCaret();
        UpdateScrollBar();
        break;

    case VK_UP:
        if (IMEType == 18 || IMEManager->IsNotComposing() == 1)
        {
            if (Comp.Len())
                InsertString((TCHAR*)*Comp, 1);
            if (CaretRow > 0)
            {
                INT RowStart = GetRowStart(CaretRow);
                INT PrevStart = GetRowStart(CaretRow - 1);
                if (RowStart == 0)
                {
                    // the row above ends the previous line
                    CaretLine--;
                    if (Lines.GetLine(CaretLine))
                    {
                        if (Lines.GetLine(CaretLine)->Text.Len() - PrevStart - 1 >= CaretCol)
                            CaretCol += PrevStart;
                        else
                            CaretCol = Lines.GetLine(CaretLine)->Text.Len() - 1;
                    }
                }
                else
                    CaretCol += PrevStart - RowStart;
                CaretRow--;
                if (CaretX > 0)
                    SetCaretX(CaretLine, CaretCol);
            }
        }
        ScrollToCaret();
        UpdateScrollBar();
        break;

    case VK_DOWN:
        if (IMEType == 18 || IMEManager->IsNotComposing() == 1)
        {
            if (Comp.Len())
                InsertString((TCHAR*)*Comp, 1);
            if (CaretRow < TotalRows - 1)
            {
                INT RowStart = GetRowStart(CaretRow);
                INT NextStart = GetRowStart(CaretRow + 1);
                if (NextStart == 0)
                {
                    // the row below starts the next line
                    CaretLine++;
                    if (CaretRow + 2 <= TotalRows - 1)
                    {
                        if (GetRowStart(CaretRow + 2))
                            CaretCol -= RowStart;
                        else if (Lines.GetLine(CaretLine))
                        {
                            if (Lines.GetLine(CaretLine)->Text.Len() - 1 >= CaretCol - RowStart)
                                CaretCol = CaretCol - RowStart;
                            else
                                CaretCol = Lines.GetLine(CaretLine)->Text.Len() - 1;
                        }
                    }
                    else if (Lines.GetLine(CaretLine))
                    {
                        if (CaretCol - RowStart <= Lines.GetLine(CaretLine)->Text.Len())
                            CaretCol = CaretCol - RowStart;
                        else
                            CaretCol = Lines.GetLine(CaretLine)->Text.Len();
                    }
                }
                else if (CaretRow + 2 <= TotalRows - 1)
                {
                    if (GetRowStart(CaretRow + 2))
                        CaretCol += NextStart - RowStart;
                    else if (Lines.GetLine(CaretLine))
                    {
                        if (CaretCol - RowStart <= Lines.GetLine(CaretLine)->Text.Len() - NextStart - 1)
                            CaretCol = CaretCol - RowStart + NextStart;
                        else
                            CaretCol = Lines.GetLine(CaretLine)->Text.Len() - 1;
                    }
                }
                else if (Lines.GetLine(CaretLine))
                {
                    if (CaretCol - RowStart <= Lines.GetLine(CaretLine)->Text.Len() - NextStart)
                        CaretCol = CaretCol - RowStart + NextStart;
                    else
                        CaretCol = Lines.GetLine(CaretLine)->Text.Len();
                }
                CaretRow++;
                if (CaretX > 0)
                    SetCaretX(CaretLine, CaretCol);
            }
        }
        ScrollToCaret();
        UpdateScrollBar();
        break;

    case VK_HOME:
        if (IMEType == 18 || IMEManager->IsNotComposing() == 1 || IMEType == 9 || IMEType == 11 ||
            IMEType == 10 || IMEType == 12)
        {
            if (Comp.Len() && IMEType == 18)
                InsertString((TCHAR*)*Comp, 1);
            CaretCol = GetRowStart(CaretRow);
        }
        CaretX = 0;
        break;

    case VK_END:
        if (IMEType == 18 || IMEManager->IsNotComposing() == 1 || IMEType == 9 || IMEType == 11 ||
            IMEType == 10 || IMEType == 12)
        {
            if (Comp.Len() && IMEType == 18)
                InsertString((TCHAR*)*Comp, 1);
            if (Lines.GetLine(CaretLine))
            {
                if (CaretRow + 1 < TotalRows)
                {
                    if (GetRowStart(CaretRow + 1) == 0)
                        CaretCol = Lines.GetLine(CaretLine)->Text.Len() - 1;
                    else
                        CaretCol = GetRowStart(CaretRow + 1);
                }
                else
                    CaretCol = Lines.GetLine(CaretLine)->Text.Len();
            }
        }
        SetCaretX(CaretLine, CaretCol);
        break;

    case VK_BACK:
        if (IMEType == 18 || IMEManager->IsNotComposing() == 1)
        {
            if (Comp.Len())
                Comp.Delete(0, Comp.Len());
            FString Left;
            FString Right;
            if (CaretCol > 0)
            {
                if (Lines.GetLine(CaretLine))
                {
                    Left = Lines.GetLine(CaretLine)->Text.Left(CaretCol - 1);
                    Right = Lines.GetLine(CaretLine)->Text.Right(Lines.GetLine(CaretLine)->Text.Len() -
                                                                 CaretCol);
                    Lines.GetLine(CaretLine)->Text = Left + Right;
                }
                CaretCol--;
            }
            else if (CaretCol == 0)
            {
                // joins the line to the previous one, whose '\n' goes
                if (CaretLine > 0 && Lines.GetLine(CaretLine) && Lines.GetLine(CaretLine - 1))
                {
                    CaretCol = Lines.GetLine(CaretLine - 1)->Text.Len() - 1;
                    Left = Lines.GetLine(CaretLine - 1)->Text.Left(
                        Lines.GetLine(CaretLine - 1)->Text.Len() - 1);
                    Right = Lines.GetLine(CaretLine)->Text;
                    Lines.GetLine(CaretLine - 1)->Text = Left + Right;
                    Lines.RemoveAt(CaretLine);
                    CaretLine--;
                    NumLines--;
                }
            }
            RecalcRows(0);
        }
        NumLines = Lines.Num();
        ScrollToCaret();
        UpdateScrollBar();
        break;

    case VK_DELETE:
        if (IMEType == 18 || IMEManager->IsNotComposing() == 1)
        {
            if (Lines.GetLine(CaretLine) && Lines.GetLine(CaretLine)->Text.Len() > CaretCol)
            {
                OnKeyDown(VK_RIGHT, 0);
                OnKeyDown(VK_BACK, 0);
            }
        }
        NumLines = Lines.Num();
        break;

    case VK_PROCESSKEY:
        if (IMEType == 10)
        {
            if (lParam == 0x14B0001)
                ChineseCompositeBox(IMEManager)->CaretLeft();
            else if (lParam == 0x14D0001)
                ChineseCompositeBox(IMEManager)->CaretRight();
        }
        break;
    }
    return 1;
}
NW_HOOK_METHOD(0x1001fbc0, NCMultiEdit, OnKeyDown)

NW_CHECK_VTABLE(NCMultiEdit, 0x100dd060, 72)

/*-----------------------------------------------------------------------------
    NCHtmlMultiEdit.
-----------------------------------------------------------------------------*/

// 0x10030ff0
NCHtmlMultiEdit::NCHtmlMultiEdit()
{
    HtmlUpButton = NULL;
    HtmlDownButton = NULL;
    HtmlScrollBar = NULL;
}
NW_HOOK_CTOR(0x10030ff0, NCHtmlMultiEdit)

// 0x10031050
NW_HOOK_DTOR(0x10031050, NCHtmlMultiEdit)

// 0x1002ff20 (vslot 24)
INT NCHtmlMultiEdit::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCMultiEdit::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1002ff20, NCHtmlMultiEdit, WindowProc)

// 0x1002ff60 (vslot 62)
const TCHAR* NCHtmlMultiEdit::GetValue()
{
    return GetText();
}
NW_HOOK_METHOD(0x1002ff60, NCHtmlMultiEdit, GetValue)

// 0x1002ff70 (vslot 61) - NCMultiEdit's, over Strings
INT NCHtmlMultiEdit::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    SWORD Delta = (SWORD)((SWORD)HIWORD(wParam) / 120);
    if (Delta > 0)
    {
        if (TopLine > Delta)
            TopLine -= Delta;
        else
            TopLine = 0;
    }
    else
    {
        INT D = Delta;
        if (Strings.Num >= VisibleRows)
        {
            if (Strings.Num + (D - TopLine) > VisibleRows)
                TopLine -= D;
            else
                TopLine = Strings.Num - VisibleRows;
        }
        else if (Strings.Num + (D - TopLine) > VisibleRows)
            TopLine -= D;
        else
            TopLine = 0;
    }
    HtmlScrollBar->SetValue(TopLine);
    return 1;
}
NW_HOOK_METHOD(0x1002ff70, NCHtmlMultiEdit, OnMouseWheel)

// 0x10030060 (vslot 64)
INT NCHtmlMultiEdit::OnScrollUp()
{
    if (Strings.Num > VisibleRows)
    {
        if (--TopLine < 0)
            TopLine = 0;
        HtmlScrollBar->SetValue(TopLine);
    }
    return 1;
}
NW_HOOK_METHOD(0x10030060, NCHtmlMultiEdit, OnScrollUp)

// 0x100300b0 (vslot 65)
INT NCHtmlMultiEdit::OnScrollDown()
{
    if (Strings.Num > VisibleRows)
    {
        TopLine++;
        if (TopLine > Strings.Num - VisibleRows)
            TopLine = Strings.Num - VisibleRows;
        HtmlScrollBar->SetValue(TopLine);
    }
    return 1;
}
NW_HOOK_METHOD(0x100300b0, NCHtmlMultiEdit, OnScrollDown)

// 0x10030120 (vslot 63)
void NCHtmlMultiEdit::UpdateScrollBar()
{
    HtmlScrollBar->SetRange(Strings.Num, VisibleRows);
    HtmlScrollBar->SetValue(TopLine);
}
NW_HOOK_METHOD(0x10030120, NCHtmlMultiEdit, UpdateScrollBar)

// stock body shared with NCHtmlEdit's (0x100310a0): the viewer scrolls the box into view
INT NCHtmlMultiEdit::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    NotifyParent(((DWORD)(_WORD)LocalY << 16) | HTML_SCROLLTO);
    return 1;
}

// 0x10033540 (vslot 35) - NCMultiEdit's, with the scroll controls inside the box
INT NCHtmlMultiEdit::OnCreate()
{
    IMEManager = new NCIMEManager(0);
    CreateChild(IMEManager, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("NWindow.chatback"));
    WhiteTex = FindTexture(TEXT("NWindow.WhiteTexture"));
    CaretTimer = SetTimer(TIMER_MultiEditCaret, 500, 0);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, TEXT("\xAC00"), 0, 0, 0, 0, 0);
    CharW = Size.cx;
    CharH = Size.cy;
    CharH2 = Size.cy;
    VisibleRows = (Height - 2) / (Size.cy + 2);
    LineGap = 2;
    NumLines = 1;
    INT W = LocalW;
    INT H = LocalH;

    HtmlUpButton = new NCHtmlPushButton(TEXT("L2UI.NWindow.WindowScrollUp"),
                                        TEXT("L2UI.NWindow.WindowScrollUp"));
    HtmlUpButton->ID = NID_MULTIEDIT_UP;
    HtmlUpButton->SetWindowRect(W - 12, 0, 12, 12);
    CreateChild(HtmlUpButton, NWS_VISIBLE);

    HtmlDownButton = new NCHtmlPushButton(TEXT("L2UI.NWindow.WindowScrollDown"),
                                          TEXT("L2UI.NWindow.WindowScrollDown"));
    HtmlDownButton->ID = NID_MULTIEDIT_DOWN;
    HtmlDownButton->SetWindowRect(W - 12, H - 12, 12, 12);
    CreateChild(HtmlDownButton, NWS_VISIBLE);

    HtmlScrollBar = new NCHtmlSliderBar(TEXT("L2UI.NWindow.WindowScrollBar1"),
                                        TEXT("L2UI.NWindow.WindowScrollBar2"),
                                        TEXT("L2UI.NWindow.WindowScrollBar3"));
    HtmlScrollBar->ID = NID_MULTIEDIT_BAR;
    HtmlScrollBar->SetWindowRect(W - 12, 12, 12, H >= 24 ? H - 24 : 0);
    CreateChild(HtmlScrollBar, NWS_VISIBLE);
    HtmlScrollBar->DoNothing(0);
    HtmlUpButton->RepeatDelay = 200;
    HtmlDownButton->RepeatDelay = 200;
    HtmlScrollBar->SetRange(TopLine, VisibleRows);
    HtmlScrollBar->SetValue(TopLine);
    return 1;
}
NW_HOOK_METHOD(0x10033540, NCHtmlMultiEdit, OnCreate)

// 0x100310c0 (vslot 36)
INT NCHtmlMultiEdit::OnDestroy()
{
    Clear();
    if (CaretTimer)
    {
        KillTimer(CaretTimer);
        CaretTimer = 0;
    }
    if (HtmlUpButton)
    {
        RemoveChildWindow(HtmlUpButton->Entry);
        if (HtmlUpButton)
            delete HtmlUpButton;
        HtmlUpButton = NULL;
    }
    if (HtmlDownButton)
    {
        RemoveChildWindow(HtmlDownButton->Entry);
        if (HtmlDownButton)
            delete HtmlDownButton;
        HtmlDownButton = NULL;
    }
    if (HtmlScrollBar)
    {
        RemoveChildWindow(HtmlScrollBar->Entry);
        if (HtmlScrollBar)
            delete HtmlScrollBar;
        HtmlScrollBar = NULL;
    }
    if (IMEManager)
    {
        RemoveChildWindow(IMEManager->Entry);
        if (IMEManager)
            delete IMEManager;
        IMEManager = NULL;
    }
    NCMultiEdit::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100310c0, NCHtmlMultiEdit, OnDestroy)

// 0x100311b0 (vslot 50) - NCMultiEdit's, clipped to its own width and the height of the first
// ancestor that is not a control (as NCHtmlEdit)
INT NCHtmlMultiEdit::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    NCWnd* Owner = FirstNonControlParent(this);
    if (Owner)
        Device->SetClip(ScreenX, Owner->ScreenY, Width, Owner->Height);
    PaintMultiEditText(this, Device);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x100311b0, NCHtmlMultiEdit, OnPaint)

NW_CHECK_VTABLE(NCHtmlMultiEdit, 0x100df240, 72)
