// Phase 6: CraftSession (include/features/craft/CraftSession.h).
// NWindow_C1 src/inject/craft/CraftSession.cpp, the same commands.
#include "features/craft/CraftSession.h"
#include "features/ManufactureCmd.h"

#include <stdarg.h>

enum
{
    CRAFT_TitleMax = 28,   // the server's clean(): <>"\& stripped, 28 characters
    CRAFT_MaxOffers = 40,  // the server's cap (C1Manufacture MAX_OFFERS); NWindow_C1 had the C2 grid's 160
};

// decimal text of a 64-bit value, negative as 0 (wsprintf has no 64-bit specifier)
static INT U64Text(SQWORD V, TCHAR* Out)
{
    TCHAR Tmp[24];
    INT N = 0;
    QWORD U = V < 0 ? 0 : (QWORD)V;
    do
    {
        Tmp[N++] = (TCHAR)('0' + U % 10);
        U /= 10;
    } while (U);
    for (INT i = 0; i < N; i++)
        Out[i] = Tmp[N - 1 - i];
    Out[N] = 0;
    return N;
}

static UBOOL Send(const TCHAR* Fmt, ...)
{
    TCHAR Buf[160];
    va_list Args;
    va_start(Args, Fmt);
    wvsprintfW(Buf, Fmt, Args);
    va_end(Args);
    return nwf::SendBypassToServer(Buf);
}

INT NCraftSession::OnData(const NCraftData& D)
{
    switch (D.View)
    {
    case CV_Setup:
        SetupView = D;
        break;
    case CV_List:
        ListView = D;
        break;
    case CV_Detail:
        DetailView = D;
        break;
    case CV_Book:
        BookView = D;
        break;
    case CV_Craft:
        CraftView = D;
        break;
    case CV_Tree:
        TreeView = D;
        break;
    case CV_Closed:
        // the store the customer was looking at is gone
        if (ListView.Dwarf == D.Dwarf)
            memset(&ListView, 0, sizeof(ListView));
        if (DetailView.Dwarf == D.Dwarf)
            memset(&DetailView, 0, sizeof(DetailView));
        break;
    default:
        break;
    }
    return D.View;
}

void NCraftSession::Reset()
{
    memset(&SetupView, 0, sizeof(SetupView));
    memset(&ListView, 0, sizeof(ListView));
    memset(&DetailView, 0, sizeof(DetailView));
    memset(&BookView, 0, sizeof(BookView));
    memset(&CraftView, 0, sizeof(CraftView));
    memset(&TreeView, 0, sizeof(TreeView));
}

UBOOL NCraftSession::OpenBook()
{
    return Send(TEXT("mfg_book %d"), BookView.bDwarven ? 1 : 0);
}

UBOOL NCraftSession::ViewRecipe(INT RecipeID)
{
    return Send(TEXT("mfg_recipe?id=%d"), RecipeID);
}

UBOOL NCraftSession::CraftItem(INT RecipeID)
{
    return Send(TEXT("mfg_craft?id=%d"), RecipeID);
}

UBOOL NCraftSession::DeleteRecipe(INT RecipeID)
{
    return Send(TEXT("mfg_rmrecipe?id=%d"), RecipeID);
}

UBOOL NCraftSession::ViewTree(INT RecipeID)
{
    return Send(TEXT("mfg_tree?id=%d"), RecipeID);
}

// "mfg_start 12:1000,15:250"
UBOOL NCraftSession::StartStore(const NCraftOffer* Offers, INT Count)
{
    // an offer takes at most 1 + 11 + 1 + 20 characters
    static TCHAR Buf[16 + CRAFT_MaxOffers * 34];
    INT N = wsprintfW(Buf, TEXT("mfg_start"));
    for (INT i = 0; i < Count && i < CRAFT_MaxOffers; i++)
    {
        Buf[N++] = (TCHAR)(i ? ',' : ' ');
        N += wsprintfW(Buf + N, TEXT("%d:"), Offers[i].RecipeID);
        N += U64Text(Offers[i].Fee, Buf + N);
    }
    Buf[N] = 0;
    if (nwf::SendBypassToServer(Buf))
        return 1;
    // the queue takes 1024 characters (NWindow_C1's took 200, which refused 15 to 20 offers);
    // the 40 offers the server keeps need at most 689
    nwf::Log("craftui", "mfg_start not sent (%d offers, %d characters)", Count, N);
    return 0;
}

UBOOL NCraftSession::QuitSetup()
{
    return Send(TEXT("mfg_quit"));
}

UBOOL NCraftSession::SetTitle(const TCHAR* Title)
{
    TCHAR Clean[CRAFT_TitleMax + 1];
    INT N = 0;
    for (const TCHAR* P = Title; P && *P && N < CRAFT_TitleMax; P++)
    {
        TCHAR C = *P;
        if (C == '<' || C == '>' || C == '"' || C == '\\' || C == '&' || C < ' ')
            continue;
        Clean[N++] = C;
    }
    Clean[N] = 0;
    while (N > 0 && Clean[N - 1] == ' ')
        Clean[--N] = 0;
    if (!N)
        return 0;
    if (!Send(TEXT("mfg_title %s"), Clean))
        return 0;
    lstrcpynW(SetupView.Title, Clean, ARRAY_COUNT(SetupView.Title));   // the next Message
                                                                       //   prompt starts from it
    return 1;
}

UBOOL NCraftSession::ViewStore(INT DwarfID)
{
    return Send(TEXT("mfg_store?d=%d"), DwarfID);
}

UBOOL NCraftSession::ViewDetail(INT DwarfID, INT RecipeID)
{
    return Send(TEXT("mfg_detail?d=%d&id=%d"), DwarfID, RecipeID);
}

UBOOL NCraftSession::Make(INT DwarfID, INT RecipeID)
{
    return Send(TEXT("mfg_make?d=%d&id=%d"), DwarfID, RecipeID);
}

// zero-initialized: every view empty
static NCraftSession GCraftSession;

NCraftSession& GetCraftSession()
{
    return GCraftSession;
}
