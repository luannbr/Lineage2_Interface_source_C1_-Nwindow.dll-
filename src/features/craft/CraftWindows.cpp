// Phase 6: the six craft windows (include/features/craft/CraftWindows.h).
// NWindow_C1 src/inject/craft/CraftWindows.cpp: the same layout, texts, colours and actions
// (SetupWnd = NCRecipeShopWnd, ListWnd = NCRecipeBuyListWnd, MakeWnd =
// NCRecipeBuyManufactureWnd, CraftWnd = NCRecipeManufactureWnd, BookWnd = NCRecipeBookWnd,
// TreeWnd = NCRecipeTreeWnd); their lists are fixed arrays instead of vectors.
#include "features/craft/CraftWindows.h"
#include "features/craft/CraftUi.h"

#include "nwindow/NCGDevice.h"

NW_IMPLEMENT_L2_CLASS(NCRecipeShopWnd, NCRecipeWnd, 0)
NW_IMPLEMENT_L2_CLASS(NCRecipeBuyListWnd, NCRecipeWnd, 0)
NW_IMPLEMENT_L2_CLASS(NCRecipeManufactureWnd, NCRecipeWnd, 0)
NW_IMPLEMENT_L2_CLASS(NCRecipeBuyManufactureWnd, NCRecipeManufactureWnd, 0)
NW_IMPLEMENT_L2_CLASS(NCRecipeBookWnd, NCRecipeWnd, 0)
NW_IMPLEMENT_L2_CLASS(NCRecipeTreeWnd, NCRecipeWnd, 0)

enum
{
    CELL = 35,        // grid pitch: a 32 px icon + 3
    COLS = 6,
    GRID_W = 223,
    SHOP_MAX = 40,    // the server's cap (C1Manufacture MAX_OFFERS: it keeps the first 40 and drops
                      // the rest without a word); NWindow_C1 had the C2 grid's 160
};

static const DWORD White = 0xFFFFFFFF;
static const DWORD Yellow = 0xFFFFFF00;
static const DWORD Grey = 0xFFA3A3A3;
static const DWORD Tan = 0xFFB09B79;
static const DWORD ResultBlue = 0xFFB2BECF;
static const DWORD Missing = 0xFFFF6464;   // not enough of an ingredient

static const TCHAR ItemCreation[] = TEXT("Item Creation");   // SysString 663
static const TCHAR RecipeSpot[] = TEXT("Recipe");
static const TCHAR Sep[] = TEXT(" : ");

enum
{
    MSG_SalesPrice = 322,     // "Please decide on the sales price."
    MSG_StoreMessage = 334,   // "Please enter store message."
};

// the controls' NWM_COMMAND ids and the lists' (NWindow_C1's)
enum
{
    ID_Book = 0x7A10,
    ID_Shop,
    ID_Up,
    ID_Down,
    ID_Start,
    ID_Message,
    ID_Abort,
    ID_Items = 0x7A20,
    ID_Mats = 0x7A30,
    ID_Create,
    ID_Prev,
    ID_Close,
    ID_Tree,
    ID_Recipes = 0x7A40,
    ID_Trash,
    ID_Nodes,
};

static NCraftSession& S()
{
    return GetCraftSession();
}

// "1,234,567" (C2 FUN_10057150)
static void Commas(SQWORD V, TCHAR* Out)
{
    TCHAR Tmp[32];
    INT N = 0;
    INT K = 0;
    UBOOL bNeg = V < 0;
    QWORD U = bNeg ? (QWORD)(-V) : (QWORD)V;
    do
    {
        if (K && K % 3 == 0)
            Tmp[N++] = ',';
        Tmp[N++] = (TCHAR)('0' + U % 10);
        U /= 10;
        K++;
    } while (U);
    INT O = 0;
    if (bNeg)
        Out[O++] = '-';
    while (N)
        Out[O++] = Tmp[--N];
    Out[O] = 0;
}

// the colour of an adena amount by its digit count (C2 FUN_1002BB70)
static DWORD AdenaColor(const TCHAR* Text)
{
    INT N = 0;
    for (; *Text; Text++)
        if (*Text >= '0' && *Text <= '9')
            N++;
    if (N < 5)
        return 0xFFFFFFFF;
    switch ((N - 2) % 4)
    {
    case 0:
        return 0xFFFF80FF;
    case 1:
        return 0xFFFFFF00;
    case 2:
        return 0xFF00FF00;
    default:
        return 0xFF00FFFF;
    }
}

static INT TextW(const TCHAR* T)
{
    return NCRecipeWnd::Measure(T).cx;
}

// T shortened with "..." to fit MaxW px (the C1 font is wider than C2's)
static const TCHAR* FitText(const TCHAR* T, INT MaxW, TCHAR* Buf, INT Cap)
{
    if (!T || TextW(T) <= MaxW)
        return T;
    INT N = (INT)wcslen(T);
    if (N > Cap - 4)
        N = Cap - 4;
    for (; N > 0; N--)
    {
        for (INT i = 0; i < N; i++)
            Buf[i] = T[i];
        Buf[N] = Buf[N + 1] = Buf[N + 2] = '.';
        Buf[N + 3] = 0;
        if (TextW(Buf) <= MaxW)
            return Buf;
    }
    return TEXT("...");
}

// "label : value": the label's colour, a white " : ", the value's colour (C2 detail lines)
static void LabelValue(NCGDevice* D, INT X, INT Y, const TCHAR* Label, DWORD LabelColor,
                       const TCHAR* Value, DWORD ValueColor, INT Gap = 0)
{
    NCRecipeWnd::PaintText(D, X, Y, LabelColor, Label);
    X += TextW(Label) + Gap;
    NCRecipeWnd::PaintText(D, X, Y, White, Sep);
    X += TextW(Sep);
    NCRecipeWnd::PaintText(D, X, Y, ValueColor, Value);
}

static void TipLabelValue(NCraftToolTip& T, const TCHAR* Label, const TCHAR* Value, DWORD Color)
{
    T.Line(Label, Grey);
    T.Seg(Sep, White);
    T.Seg(Value, Color);
}

static const TCHAR* ItemNameOr(INT ItemID, TCHAR* Fallback)
{
    const TCHAR* N = NCRecipeWnd::ItemName(ItemID);
    if (N && *N)
        return N;
    wsprintfW(Fallback, TEXT("Item %d"), ItemID);
    return Fallback;
}

// C2's tool tip of a recipe (type 1): the recipe item's name, [cost], weight, description
static void RecipeTip(NCraftToolTip& T, const NCraftRecipeRow& R, const SQWORD* Fee)
{
    TCHAR Fb[24], V[32];
    T.Line(ItemNameOr(R.ScrollID ? R.ScrollID : R.ProductID, Fb), White);
    if (Fee)
    {
        Commas(*Fee, V);
        TipLabelValue(T, TEXT("Cost"), V, AdenaColor(V));
        T.Seg(TEXT(" Adena"), Grey);
    }
    if (R.Weight > 0)
    {
        wsprintfW(V, TEXT("%d"), R.Weight);
        TipLabelValue(T, TEXT("Weight"), V, Tan);
    }
    if (R.ScrollID)
        T.Wrapped(NCRecipeWnd::ItemDescription(R.ScrollID), Tan);
}

/*-----------------------------------------------------------------------------
    NCRecipeShopWnd: the dwarf's shop setup.
-----------------------------------------------------------------------------*/

NCRecipeShopWnd::NCRecipeShopWnd()
    : NCRecipeWnd(TEXT("c1recipe.recipePersonalMakingBack"), ItemCreation, RecipeSpot, 290, 205)
{
    BookGrid = NULL;
    ShopGrid = NULL;
    NumOffers = 0;
    PendingRecipe = 0;
}

void NCRecipeShopWnd::Load()
{
    const NCraftData& D = S().Setup();
    NumOffers = 0;
    for (INT i = 0; i < D.NumShop; i++)
        if (BookRow(D.Shop[i].RecipeID) && NumOffers < SHOP_MAX)
        {
            Offers[NumOffers].RecipeID = D.Shop[i].RecipeID;
            Offers[NumOffers].Fee = D.Shop[i].Fee;
            NumOffers++;
        }
    RefreshGrids();
}

void NCRecipeShopWnd::OnCreateChildren()
{
    LoadCommon();
    ShopGrid = AddList(ID_Shop, 16, 225, GRID_W, 104, CELL, COLS, CELL);
    BookGrid = AddList(ID_Book, 16, 50, GRID_W, 139, CELL, COLS, CELL);
    AddButton(ID_Up, 194, 200, 14, 14, NULL, TEXT("L2UI.StoreWnd.StoreUpBtn1"),
              TEXT("L2UI.StoreWnd.StoreUpBtn1"));
    AddButton(ID_Down, 210, 200, 14, 14, NULL, TEXT("L2UI.StoreWnd.StoreDownBtn1"),
              TEXT("L2UI.StoreWnd.StoreDownBtn2"));
    AddButton(ID_Start, 164, 335, 67, 19, TEXT("Start"));
    AddButton(ID_Message, 87, 335, 67, 19, TEXT("Message"));
    AddButton(ID_Abort, 10, 335, 67, 19, TEXT("Abort"));
    RefreshGrids();
}

void NCRecipeShopWnd::PaintWindow(NCGDevice* Device)
{
    PaintBack(Device);
    PaintText(Device, 15, 25, White, TEXT("Recipe List"));
    PaintText(Device, 15, 200, White, TEXT("Production List"));
}

void NCRecipeShopWnd::PaintListItem(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X,
                                    INT Y, UBOOL bSelected)
{
    const NCraftRecipeRow* R = RowOf(List, Index);
    if (!R)
        return;
    if (bSelected)
        PaintSelection(Device, X, Y);
    PaintIcon(Device, R->ProductID, X, Y);   // a recipe shows its PRODUCT's icon
}

void NCRecipeShopWnd::OnListClick(NCRecipeListWnd* List, INT Index, UBOOL bDouble)
{
    if (!bDouble)
    {
        Sound(3);   // itemsound.pickup, as the stock grid
        return;
    }
    if (List == BookGrid)
        AskPrice(Index);
    else if (List == ShopGrid)
        RemoveOffer(Index);
}

void NCRecipeShopWnd::PaintListToolTip(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X,
                                       INT Y)
{
    const NCraftRecipeRow* R = RowOf(List, Index);
    if (!R)
        return;
    NCraftToolTip T;
    SQWORD Fee = List == ShopGrid ? Offers[Index].Fee : 0;
    RecipeTip(T, *R, List == ShopGrid ? &Fee : NULL);
    T.Draw(Device, X, Y, 32, 32);
}

void NCRecipeShopWnd::OnCommand(INT ID, INT Value)
{
    switch (ID)
    {
    case ID_Up:     // the selected offer goes
        if (ShopGrid)
            RemoveOffer(ShopGrid->Selected);
        break;
    case ID_Down:   // the selected recipe gets a price, then an offer
        if (BookGrid)
            AskPrice(BookGrid->Selected);
        break;
    case ID_Start:
        S().StartStore(Offers, NumOffers);
        Clear();
        Close();
        break;
    case ID_Message:
        AskText(MSG_StoreMessage, S().Setup().Title);
        break;
    case ID_Abort:
        S().QuitSetup();
        Clear();
        Close();
        break;
    default:
        break;
    }
}

void NCRecipeShopWnd::OnDialogNumber(SQWORD Price)
{
    if (!BookRow(PendingRecipe) || NumOffers >= SHOP_MAX)
        return;
    for (INT i = 0; i < NumOffers; i++)
        if (Offers[i].RecipeID == PendingRecipe)
            return;   // C2 ignores a recipe offered twice
    Offers[NumOffers].RecipeID = PendingRecipe;
    Offers[NumOffers].Fee = Price;
    NumOffers++;
    if (BookGrid)
        BookGrid->Select(-1);
    RefreshGrids();
}

void NCRecipeShopWnd::OnDialogText(const TCHAR* Text)
{
    if (Text && *Text)
        S().SetTitle(Text);
}

void NCRecipeShopWnd::OnUserClose()
{
    S().QuitSetup();
    Clear();
}

const NCraftRecipeRow* NCRecipeShopWnd::BookRow(INT RecipeID)
{
    const NCraftData& D = S().Setup();
    for (INT i = 0; i < D.NumBook; i++)
        if (D.Book[i].RecipeID == RecipeID)
            return &D.Book[i];
    return NULL;
}

const NCraftRecipeRow* NCRecipeShopWnd::RowOf(NCRecipeListWnd* List, INT Index)
{
    const NCraftData& D = S().Setup();
    if (List == BookGrid && Index >= 0 && Index < D.NumBook)
        return &D.Book[Index];
    if (List == ShopGrid && Index >= 0 && Index < NumOffers)
        return BookRow(Offers[Index].RecipeID);
    return NULL;
}

void NCRecipeShopWnd::AskPrice(INT BookIndex)
{
    const NCraftData& D = S().Setup();
    if (BookIndex < 0 || BookIndex >= D.NumBook)
        return;
    PendingRecipe = D.Book[BookIndex].RecipeID;
    AskNumber(MSG_SalesPrice);
}

void NCRecipeShopWnd::RemoveOffer(INT Index)
{
    if (Index < 0 || Index >= NumOffers)
        return;
    for (INT i = Index; i < NumOffers - 1; i++)
        Offers[i] = Offers[i + 1];
    NumOffers--;
    if (ShopGrid)
        ShopGrid->Select(-1);
    RefreshGrids();
}

void NCRecipeShopWnd::Clear()
{
    NumOffers = 0;
    if (BookGrid)
    {
        BookGrid->Select(-1);
        BookGrid->SetItems(0);
    }
    if (ShopGrid)
    {
        ShopGrid->Select(-1);
        ShopGrid->SetItems(0);
    }
}

void NCRecipeShopWnd::RefreshGrids()
{
    if (BookGrid)
        BookGrid->SetItems(S().Setup().NumBook);
    if (ShopGrid)
        ShopGrid->SetItems(NumOffers);
}

/*-----------------------------------------------------------------------------
    NCRecipeBuyListWnd: a customer's view of a manufacture store.
-----------------------------------------------------------------------------*/

NCRecipeBuyListWnd::NCRecipeBuyListWnd()
    : NCRecipeWnd(TEXT("c1recipe.recipePersonalListBack"), ItemCreation, RecipeSpot, 290, 205)
{
    AdenaTex = NULL;
    HighlightTex = NULL;
    Items = NULL;
}

void NCRecipeBuyListWnd::Refresh()
{
    if (Items)
    {
        Items->Select(-1);
        Items->SetItems(S().List().NumItems);
    }
}

void NCRecipeBuyListWnd::OnCreateChildren()
{
    LoadCommon();
    AdenaTex = LoadTex(TEXT("L2UI.WindowIcon.WindowIconAdena"));
    HighlightTex = LoadTex(TEXT("NWindow.MenuHighlight"));
    // C2 NCRecipeListBox(100 items, 5 rows) at (1, 39, 236, 252): x0 15, y0 9, rows 43 + 5 apart
    Items = AddList(ID_Items, 1, 39, 236, 252, 48, 1, 0, 9);
    Refresh();
}

void NCRecipeBuyListWnd::PaintWindow(NCGDevice* Device)
{
    const NCraftData& D = S().List();
    PaintBack(Device);
    PaintText(Device, 15, 25, White, TEXT("Recipe List"));
    PaintTextRight(Device, 70, 299, White, TEXT("Creator MP"));
    PaintMpBar(Device, 72, 298, D.MpCur, D.MpMax);
    PaintTex(Device, AdenaTex, 52, 313, 16, 12, 0, 0, 16, 12);
    TCHAR A[32];
    Commas(D.Adena, A);
    PaintTextRight(Device, 222, 315, White, A);
}

void NCRecipeBuyListWnd::PaintListItem(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X,
                                       INT Y, UBOOL bSelected)
{
    const NCraftData& D = S().List();
    if (Index < 0 || Index >= D.NumItems)
        return;
    const NCraftRecipeRow& R = D.Items[Index];
    const INT X0 = 15;
    const INT TX = X0 + 42;
    if (bSelected)
        PaintStretch(Device, HighlightTex, 2, Y, 236 - 18, 43);
    PaintIcon(Device, R.ProductID, X0, Y + 4);
    TCHAR Fb[24], Fit[96], C[32], Rate[16];
    PaintText(Device, TX, Y + 2, White,
              FitText(ItemNameOr(R.ProductID, Fb), List->DrawWidth() - TX - 4, Fit, 96));
    // Cost : 1,000 Adena (C2: "Manufacturing Cost", too wide for the C1 font with big fees)
    const TCHAR* Label = TEXT("Cost");
    INT CX = TX;
    PaintText(Device, CX, Y + 17, Yellow, Label);
    CX += TextW(Label);
    PaintText(Device, CX, Y + 17, White, Sep);
    CX += TextW(Sep);
    Commas(R.Fee, C);
    PaintText(Device, CX, Y + 17, AdenaColor(C), C);
    CX += TextW(C) + 5;
    PaintText(Device, CX, Y + 17, Yellow, TEXT("Adena"));
    // Success Rate : 70%
    wsprintfW(Rate, TEXT("%d%%"), R.Rate);
    LabelValue(Device, TX, Y + 32, TEXT("Success Rate"), Grey, Rate, Tan);
}

void NCRecipeBuyListWnd::OnListClick(NCRecipeListWnd* List, INT Index, UBOOL bDouble)
{
    if (bDouble)
        return;   // the click already asked for the recipe (RecipeShopMakeInfo)
    const NCraftData& D = S().List();
    if (Index >= 0 && Index < D.NumItems)
        S().ViewDetail(D.Dwarf, D.Items[Index].RecipeID);
}

/*-----------------------------------------------------------------------------
    NCRecipeManufactureWnd / NCRecipeBuyManufactureWnd: the manufacture window.
-----------------------------------------------------------------------------*/

NCRecipeManufactureWnd::NCRecipeManufactureWnd()
    : NCRecipeWnd(TEXT("c1recipe.recipeMakingBack"), ItemCreation, RecipeSpot, 290, 205)
{
    ChatBackTex = NULL;
    Mats = NULL;
}

void NCRecipeManufactureWnd::Refresh()
{
    if (Mats)
    {
        Mats->Select(-1);
        Mats->SetItems(Data().NumMats);
    }
}

const NCraftData& NCRecipeManufactureWnd::Data()
{
    return S().Craft();
}

void NCRecipeManufactureWnd::DoCreate()
{
    S().CraftItem(Data().RecipeID);
}

void NCRecipeManufactureWnd::DoPrev()
{
    S().OpenBook();
}

void NCRecipeManufactureWnd::OnCreateChildren()
{
    LoadCommon();
    ChatBackTex = LoadTex(TEXT("NWindow.ChatBack"));
    Mats = AddList(ID_Mats, 16, 128, GRID_W, 104, CELL, COLS, CELL);
    AddButton(ID_Tree, 200, 43, 32, 32, NULL, TEXT("c1recipe.RecipeTreeBtn"),
              TEXT("c1recipe.RecipeTreeBtn_click"));
    AddButton(ID_Create, 10, 335, 67, 19, TEXT("Create"));
    AddButton(ID_Prev, 87, 335, 67, 19, TEXT("<PREV"));
    AddButton(ID_Close, 164, 335, 67, 19, TEXT("Close"));
    Refresh();
}

void NCRecipeManufactureWnd::PaintWindow(NCGDevice* Device)
{
    const NCraftData& D = Data();
    PaintBack(Device);
    TCHAR Fb[24], Fit[96], V[48];
    PaintIcon(Device, D.ProductID, 16, 43);
    const TCHAR* Name = ItemNameOr(D.ProductID, Fb);
    const TCHAR* Shown =
        FitText(Name, RECIPEWND_Width - 60 - 44 - (D.Grade > 0 ? 18 : 0), Fit, 96);
    PaintText(Device, 60, 32, White, Shown);
    PaintGrade(Device, 62 + TextW(Shown), 30, D.Grade);
    wsprintfW(V, TEXT("%d"), D.MpCost);
    LabelValue(Device, 60, 46, TEXT("MP cost"), Grey, V, Tan);
    wsprintfW(V, TEXT("%d%%"), D.Rate);
    LabelValue(Device, 60, 60, TEXT("Success Rate"), Grey, V, Tan);
    Commas(D.ProductCount, V);
    LabelValue(Device, 60, 74, TEXT("Number of Resulting Items"), Grey, V, Tan);
    PaintText(Device, 15, 103, White, TEXT("Ingredient(s)"));
    PaintText(Device, 15, 241, White, TEXT("Creator MP"));
    PaintMpBar(Device, 72, 242, D.MpCur, D.MpMax);
    PaintText(Device, 15, 268, White, TEXT("Result(s)"));
    Commas(D.Owned, V);
    LabelValue(Device, 15, 292, TEXT("Number of Retained Items"), Grey, V, Tan, 2);
    if (D.Result == 1 || D.Result == 2)   // SystemMsg 959 / 960
    {
        TCHAR Line[200], N[32];
        Commas(D.ProductCount, N);
        if (D.Result == 1)
            wsprintfW(Line, TEXT("%s (*%s) manufacturing success."), Name, N);
        else
            wsprintfW(Line, TEXT("%s manufacturing failure."), Name);
        PaintText(Device, 15, 310, ResultBlue, Line, RECIPEWND_Width - 15 - 6);   // wraps above
                                                                                   //   the buttons
    }
}

void NCRecipeManufactureWnd::PaintListItem(NCRecipeListWnd* List, NCGDevice* Device, INT Index,
                                           INT X, INT Y, UBOOL bSelected)
{
    // this grid draws no selection frame
    const NCraftData& D = Data();
    if (Index < 0 || Index >= D.NumMats)
        return;
    const NCraftMaterial& M = D.Mats[Index];
    PaintIcon(Device, M.ItemID, X, Y);
    if (M.Have < M.Need)   // not enough: darkened
        PaintTex(Device, ChatBackTex, X, Y, 32, 32, 0, 0, 32, 32);
}

void NCRecipeManufactureWnd::OnListClick(NCRecipeListWnd* List, INT Index, UBOOL bDouble)
{
    if (!bDouble)
        Sound(3);
}

void NCRecipeManufactureWnd::PaintListToolTip(NCRecipeListWnd* List, NCGDevice* Device,
                                              INT Index, INT X, INT Y)
{
    const NCraftData& D = Data();
    if (Index < 0 || Index >= D.NumMats)
        return;
    const NCraftMaterial& M = D.Mats[Index];
    NCraftToolTip T;
    TCHAR Fb[24], V[32];
    T.Line(ItemNameOr(M.ItemID, Fb), White);
    T.Wrapped(ItemDescription(M.ItemID), Tan);
    Commas(M.Need, V);
    TipLabelValue(T, TEXT("Number of Needed Items"), V, Tan);
    Commas(M.Have, V);
    TipLabelValue(T, TEXT("Number of Kept Items"), V, M.Have < M.Need ? Missing : Tan);
    T.Draw(Device, X, Y, 32, 32);
}

void NCRecipeManufactureWnd::OnCommand(INT ID, INT Value)
{
    switch (ID)
    {
    case ID_Create:
        DoCreate();
        break;
    case ID_Prev:   // the window before reopens at this spot
        Close();
        DoPrev();
        break;
    case ID_Close:
        Close();
        break;
    case ID_Tree:
        CraftUi_ToggleTree(Data().RecipeID);
        break;
    default:
        break;
    }
}

NCRecipeBuyManufactureWnd::NCRecipeBuyManufactureWnd()
{
}

const NCraftData& NCRecipeBuyManufactureWnd::Data()
{
    return S().Detail();
}

void NCRecipeBuyManufactureWnd::DoCreate()
{
    S().Make(Data().Dwarf, Data().RecipeID);
}

void NCRecipeBuyManufactureWnd::DoPrev()
{
    S().ViewStore(Data().Dwarf);
}

/*-----------------------------------------------------------------------------
    NCRecipeBookWnd: the recipe book of the "Create Item" skill.
-----------------------------------------------------------------------------*/

NCRecipeBookWnd::NCRecipeBookWnd()
    : NCRecipeWnd(TEXT("c1recipe.recipeBookBack"), ItemCreation, RecipeSpot, 290, 205)
{
    Grid = NULL;
    PendingDelete = 0;
}

void NCRecipeBookWnd::Refresh()
{
    if (Grid)
    {
        Grid->Select(-1);
        Grid->SetItems(S().Book().NumBook);
    }
}

void NCRecipeBookWnd::OnCreateChildren()
{
    LoadCommon();
    Grid = AddList(ID_Recipes, 16, 50, GRID_W, 244, CELL, COLS, CELL);
    AddButton(ID_Trash, 190, 315, 32, 32, NULL, TEXT("L2UI.WindowIcon.DestroyItem"),
              TEXT("L2UI.WindowIcon.DestroyItem_active"));
    Refresh();
}

void NCRecipeBookWnd::PaintWindow(NCGDevice* Device)
{
    const NCraftData& D = S().Book();
    PaintBack(Device);
    PaintText(Device, 12, 25, White, TEXT("Recipe Book"));
    TCHAR T[32];
    wsprintfW(T, TEXT("(%d/%d)"), D.NumBook, D.Limit > 0 ? D.Limit : 50);
    PaintTextRight(Device, RECIPEWND_Width - 10, 25, Tan, T);
}

void NCRecipeBookWnd::PaintListItem(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X,
                                    INT Y, UBOOL bSelected)
{
    const NCraftData& D = S().Book();
    if (Index < 0 || Index >= D.NumBook)
        return;
    if (bSelected)
        PaintSelection(Device, X, Y);
    PaintIcon(Device, D.Book[Index].ProductID, X, Y);
}

void NCRecipeBookWnd::OnListClick(NCRecipeListWnd* List, INT Index, UBOOL bDouble)
{
    const NCraftData& D = S().Book();
    if (Index < 0 || Index >= D.NumBook)
        return;
    if (bDouble)
        S().ViewRecipe(D.Book[Index].RecipeID);   // RequestRecipeItemMakeInfo
    else
        Sound(3);
}

void NCRecipeBookWnd::PaintListToolTip(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X,
                                       INT Y)
{
    const NCraftData& D = S().Book();
    if (Index < 0 || Index >= D.NumBook)
        return;
    NCraftToolTip T;
    RecipeTip(T, D.Book[Index], NULL);
    T.Draw(Device, X, Y, 32, 32);
}

void NCRecipeBookWnd::OnCommand(INT ID, INT Value)
{
    if (ID != ID_Trash)
        return;
    const NCraftData& D = S().Book();
    INT Sel = Grid ? Grid->Selected : -1;
    if (Sel < 0 || Sel >= D.NumBook)
    {
        Sound(2);
        return;
    }
    PendingDelete = D.Book[Sel].RecipeID;   // SystemMsg 74 "Destroy $s1. Do you want to
                                            //   continue?"
    TCHAR Fb[24], Line[160];
    wsprintfW(Line, TEXT("Destroy %s. Do you want to continue?"),
              ItemNameOr(D.Book[Sel].ScrollID ? D.Book[Sel].ScrollID : D.Book[Sel].ProductID, Fb));
    AskConfirm(Line);
}

void NCRecipeBookWnd::OnDialogConfirm()
{
    if (PendingDelete)
        S().DeleteRecipe(PendingDelete);
    PendingDelete = 0;
}

/*-----------------------------------------------------------------------------
    NCRecipeTreeWnd: "Recipe Information", the ingredient tree of a recipe.
-----------------------------------------------------------------------------*/

NCRecipeTreeWnd::NCRecipeTreeWnd()
    : NCRecipeWnd(TEXT("c1recipe.recipeTreeBack"), TEXT("Recipe Information"), TEXT("RecipeTree"),
                  260, 150)
{
    ChatBackTex = NULL;
    FrameTex = NULL;
    FrameOpenTex = NULL;
    FrameLeafTex = NULL;
    PlusTex = NULL;
    MinusTex = NULL;
    Tree = NULL;
    NumVisible = 0;
    NumExpanded = 0;
}

void NCRecipeTreeWnd::Refresh()
{
    NumExpanded = S().Tree().NumNodes;
    for (INT i = 0; i < NumExpanded; i++)
        Expanded[i] = 0;
    if (NumExpanded)
        Expanded[0] = 1;
    Rebuild();
}

void NCRecipeTreeWnd::OnCreateChildren()
{
    LoadCommon();
    ChatBackTex = LoadTex(TEXT("NWindow.ChatBack"));
    FrameTex = LoadTex(TEXT("c1recipe.RecipeTreeIconBack"));
    FrameOpenTex = LoadTex(TEXT("c1recipe.RecipeTreeIconBack_click"));
    FrameLeafTex = LoadTex(TEXT("c1recipe.RecipeTreeIconDisableBack"));
    PlusTex = LoadTex(TEXT("c1recipe.treePlus"));
    MinusTex = LoadTex(TEXT("c1recipe.treeMinus"));
    Tree = AddList(ID_Nodes, 2, 82, 236, 246, 37, 1, 0, 5);
    AddButton(ID_Close, 87, 331, 67, 19, TEXT("Close"));
    Rebuild();
}

void NCRecipeTreeWnd::PaintWindow(NCGDevice* Device)
{
    const NCraftData& D = S().Tree();
    PaintBack(Device);
    TCHAR Fb[24], Fit[96], V[32];
    PaintIcon(Device, D.ProductID, 16, 35);
    const TCHAR* Shown = FitText(ItemNameOr(D.ProductID, Fb),
                                 RECIPEWND_Width - 60 - 46 - (D.Grade > 0 ? 18 : 0), Fit, 96);
    PaintText(Device, 60, 31, White, Shown);
    PaintGrade(Device, 62 + TextW(Shown), 29, D.Grade);
    wsprintfW(V, TEXT("%d"), D.MpCost);
    LabelValue(Device, 60, 45, TEXT("MP cost"), Grey, V, Tan);
    wsprintfW(V, TEXT("%d%%"), D.Rate);
    LabelValue(Device, 60, 59, TEXT("Success Rate"), Grey, V, Tan);
    wsprintfW(V, TEXT("Lv.%d"), D.Level);
    PaintText(Device, 207, 59, White, V);
}

void NCRecipeTreeWnd::PaintListItem(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X,
                                    INT Y, UBOOL bSelected)
{
    const NCraftData& D = S().Tree();
    if (Index < 0 || Index >= NumVisible)
        return;
    INT Node = Visible[Index];
    const NCraftTreeNode& N = D.Nodes[Node];
    INT NX = 15 + N.Depth * 16;
    PaintIcon(Device, N.ItemID, NX, Y);
    PaintTex(Device, N.bChildren ? (Expanded[Node] ? FrameOpenTex : FrameTex) : FrameLeafTex, NX,
             Y, 32, 32, 0, 0, 32, 32);
    if (N.bChildren)
        PaintTex(Device, Expanded[Node] ? MinusTex : PlusTex, NX - 14, Y + 10, 12, 12, 0, 0, 12,
                 12);
    TCHAR Fb[24], Fit[96], V[56];
    PaintText(Device, NX + 37, Y + 4, White,
              FitText(ItemNameOr(N.ItemID, Fb), List->DrawWidth() - NX - 41, Fit, 96));
    if (N.Depth > 0)
    {
        TCHAR Have[24], Need[24];
        Commas(N.Have, Have);
        Commas(N.Need, Need);
        wsprintfW(V, TEXT("(%s/%s)"), Have, Need);
        PaintText(Device, NX + 37, Y + 18, N.Have < N.Need ? Missing : White, V);
        if (N.Have < N.Need)
            PaintTex(Device, ChatBackTex, NX, Y, 32, 32, 0, 0, 32, 32);
    }
}

void NCRecipeTreeWnd::OnListClick(NCRecipeListWnd* List, INT Index, UBOOL bDouble)
{
    if (Index < 0 || Index >= NumVisible)
        return;
    INT Node = Visible[Index];
    if (!S().Tree().Nodes[Node].bChildren)
        return;
    Expanded[Node] = !Expanded[Node];
    Sound(0);
    Rebuild();
}

void NCRecipeTreeWnd::OnCommand(INT ID, INT Value)
{
    if (ID == ID_Close)
        Close();
}

// the depth-first nodes, without the children of the closed ones
void NCRecipeTreeWnd::Rebuild()
{
    const NCraftData& D = S().Tree();
    if (NumExpanded != D.NumNodes)
    {
        NumExpanded = D.NumNodes;
        for (INT i = 0; i < NumExpanded; i++)
            Expanded[i] = 0;
    }
    NumVisible = 0;
    INT HiddenBelow = -1;   // the depth of the closed node whose subtree is skipped
    for (INT i = 0; i < D.NumNodes; i++)
    {
        const NCraftTreeNode& N = D.Nodes[i];
        if (HiddenBelow >= 0)
        {
            if (N.Depth > HiddenBelow)
                continue;
            HiddenBelow = -1;
        }
        Visible[NumVisible++] = i;
        if (N.bChildren && !Expanded[i])
            HiddenBelow = N.Depth;
    }
    if (Tree)
    {
        Tree->Select(-1);
        Tree->SetItems(NumVisible);
    }
}
