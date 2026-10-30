// Phase 6: CraftData - the protocol v1 parser (include/features/craft/CraftData.h).
// NWindow_C1 src/inject/craft/CraftData.cpp, the same reading line by line; the records go to
// the fixed arrays of NCraftData instead of vectors.
#include "features/craft/CraftData.h"

// Key (Len characters) is Name, exactly
static UBOOL KeyIs(const TCHAR* Key, INT Len, const TCHAR* Name)
{
    INT i = 0;
    for (; i < Len; i++)
        if (!Name[i] || Name[i] != Key[i])
            return 0;
    return Name[i] == 0;
}

// S (Len characters) starts with Prefix
static UBOOL StartsWith(const TCHAR* S, INT Len, const TCHAR* Prefix)
{
    INT i = 0;
    for (; Prefix[i]; i++)
        if (i >= Len || S[i] != Prefix[i])
            return 0;
    return 1;
}

static void CopyText(TCHAR* Dst, INT Cap, const TCHAR* Src, INT Len)
{
    if (Len >= Cap)
        Len = Cap - 1;
    for (INT i = 0; i < Len; i++)
        Dst[i] = Src[i];
    Dst[Len] = 0;
}

// Up to Max comma-separated integers (an optional leading '-'); how many were read
static INT ReadInts(const TCHAR* S, INT Len, SQWORD* Out, INT Max)
{
    INT N = 0;
    INT i = 0;
    while (i < Len && N < Max)
    {
        UBOOL bNeg = 0;
        if (S[i] == '-')
        {
            bNeg = 1;
            i++;
        }
        if (i >= Len || S[i] < '0' || S[i] > '9')
            break;
        SQWORD V = 0;
        while (i < Len && S[i] >= '0' && S[i] <= '9')
            V = V * 10 + (S[i++] - '0');
        Out[N++] = bNeg ? -V : V;
        if (i < Len && S[i] == ',')
            i++;
        else
            break;
    }
    return N;
}

static INT ViewOf(const TCHAR* S, INT Len)
{
    if (StartsWith(S, Len, TEXT("setup")))
        return CV_Setup;
    if (StartsWith(S, Len, TEXT("list")))
        return CV_List;
    if (StartsWith(S, Len, TEXT("detail")))
        return CV_Detail;
    if (StartsWith(S, Len, TEXT("closed")))
        return CV_Closed;
    if (StartsWith(S, Len, TEXT("book")))
        return CV_Book;
    if (StartsWith(S, Len, TEXT("craft")))
        return CV_Craft;
    if (StartsWith(S, Len, TEXT("tree")))
        return CV_Tree;
    return CV_None;
}

static void ApplyLine(NCraftData* D, const TCHAR* Key, INT KLen, const TCHAR* Val, INT VLen)
{
    SQWORD V[10] = {0};
    if (KeyIs(Key, KLen, TEXT("title")))
        CopyText(D->Title, ARRAY_COUNT(D->Title), Val, VLen);
    else if (KeyIs(Key, KLen, TEXT("name")))
        CopyText(D->Name, ARRAY_COUNT(D->Name), Val, VLen);
    else if (KeyIs(Key, KLen, TEXT("msg")))
        CopyText(D->Msg, ARRAY_COUNT(D->Msg), Val, VLen);
    else if (KeyIs(Key, KLen, TEXT("book")) || KeyIs(Key, KLen, TEXT("item")))
    {
        INT N = ReadInts(Val, VLen, V, 9);
        NCraftRecipeRow R;
        memset(&R, 0, sizeof(R));
        R.RecipeID = (INT)V[0];
        R.ProductID = (INT)V[1];
        R.ProductCount = V[2];
        if (KeyIs(Key, KLen, TEXT("book")))   // recipe,product,count,mpCost,rate,level,scroll,grade,weight
        {
            R.MpCost = (INT)V[3];
            R.Rate = (INT)V[4];
            R.Level = (INT)V[5];
            R.ScrollID = (INT)V[6];
            R.Grade = (INT)V[7];
            R.Weight = (INT)V[8];
            if (N >= 3 && D->NumBook < CRAFT_MaxBook)
                D->Book[D->NumBook++] = R;
        }
        else   // recipe,product,count,fee,mpCost,rate
        {
            R.Fee = V[3];
            R.MpCost = (INT)V[4];
            R.Rate = (INT)V[5];
            if (N >= 4 && D->NumItems < CRAFT_MaxItems)
                D->Items[D->NumItems++] = R;
        }
    }
    else if (KeyIs(Key, KLen, TEXT("shop")))
    {
        if (ReadInts(Val, VLen, V, 2) == 2 && D->NumShop < CRAFT_MaxShop)
        {
            NCraftRecipeRow& R = D->Shop[D->NumShop++];
            memset(&R, 0, sizeof(R));
            R.RecipeID = (INT)V[0];
            R.Fee = V[1];
        }
    }
    else if (KeyIs(Key, KLen, TEXT("node")))   // depth,item,need,have,hasChildren
    {
        if (ReadInts(Val, VLen, V, 5) == 5 && D->NumNodes < CRAFT_MaxNodes)
        {
            NCraftTreeNode& Node = D->Nodes[D->NumNodes++];
            Node.Depth = (INT)V[0];
            Node.ItemID = (INT)V[1];
            Node.Need = V[2];
            Node.Have = V[3];
            Node.bChildren = V[4] != 0;
        }
    }
    else if (KeyIs(Key, KLen, TEXT("mat")))
    {
        if (ReadInts(Val, VLen, V, 3) == 3 && D->NumMats < CRAFT_MaxMats)
        {
            NCraftMaterial& M = D->Mats[D->NumMats++];
            M.ItemID = (INT)V[0];
            M.Need = V[1];
            M.Have = V[2];
        }
    }
    else
    {
        INT N = ReadInts(Val, VLen, V, 2);
        if (N <= 0)
            return;
        if (KeyIs(Key, KLen, TEXT("self")))
            D->Self = (INT)V[0];
        else if (KeyIs(Key, KLen, TEXT("dwarf")))
            D->Dwarf = (INT)V[0];
        else if (KeyIs(Key, KLen, TEXT("open")))
            D->bOpen = V[0] != 0;
        else if (KeyIs(Key, KLen, TEXT("preview")))
            D->bPreview = V[0] != 0;
        else if (KeyIs(Key, KLen, TEXT("dwarven")))
            D->bDwarven = V[0] != 0;
        else if (KeyIs(Key, KLen, TEXT("limit")))
            D->Limit = (INT)V[0];
        else if (KeyIs(Key, KLen, TEXT("level")))
            D->Level = (INT)V[0];
        else if (KeyIs(Key, KLen, TEXT("adena")))
            D->Adena = V[0];
        else if (KeyIs(Key, KLen, TEXT("recipe")))
            D->RecipeID = (INT)V[0];
        else if (KeyIs(Key, KLen, TEXT("fee")))
            D->Fee = V[0];
        else if (KeyIs(Key, KLen, TEXT("mpcost")))
            D->MpCost = (INT)V[0];
        else if (KeyIs(Key, KLen, TEXT("rate")))
            D->Rate = (INT)V[0];
        else if (KeyIs(Key, KLen, TEXT("result")))
            D->Result = (INT)V[0];
        else if (KeyIs(Key, KLen, TEXT("grade")))
            D->Grade = (INT)V[0];
        else if (KeyIs(Key, KLen, TEXT("owned")))
            D->Owned = V[0];
        else if (KeyIs(Key, KLen, TEXT("mp")))
        {
            D->MpCur = (INT)V[0];
            D->MpMax = N > 1 ? (INT)V[1] : 0;
        }
        else if (KeyIs(Key, KLen, TEXT("product")))
        {
            D->ProductID = (INT)V[0];
            D->ProductCount = N > 1 ? V[1] : 1;
        }
        // unknown keys are ignored: newer servers may add fields
    }
}

UBOOL ParseCraftData(const TCHAR* Text, NCraftData* Out)
{
    if (!Text || !Out)
        return 0;
    memset(Out, 0, sizeof(*Out));
    // header: "L2W mfg <version> <view>"
    const TCHAR* P = Text;
    const TCHAR* Eol = P;
    while (*Eol && *Eol != '\n')
        Eol++;
    INT HLen = (INT)(Eol - P);
    if (HLen > 0 && P[HLen - 1] == '\r')
        HLen--;
    if (!StartsWith(P, HLen, TEXT("L2W mfg ")))
        return 0;
    const TCHAR* H = P + 8;
    INT HL = HLen - 8;
    SQWORD Version = 0;
    if (ReadInts(H, HL, &Version, 1) != 1)
        return 0;
    Out->Version = (INT)Version;
    while (HL > 0 && *H != ' ')
    {
        H++;
        HL--;
    }
    while (HL > 0 && *H == ' ')
    {
        H++;
        HL--;
    }
    Out->View = ViewOf(H, HL);
    if (Out->View == CV_None)
        return 0;

    P = *Eol ? Eol + 1 : Eol;
    while (*P)
    {
        const TCHAR* E = P;
        while (*E && *E != '\n')
            E++;
        INT Len = (INT)(E - P);
        if (Len > 0 && P[Len - 1] == '\r')
            Len--;
        INT Eq = 0;
        while (Eq < Len && P[Eq] != '=')
            Eq++;
        if (Eq > 0 && Eq < Len)
            ApplyLine(Out, P, Eq, P + Eq + 1, Len - Eq - 1);
        P = *E ? E + 1 : E;
    }
    return 1;
}
