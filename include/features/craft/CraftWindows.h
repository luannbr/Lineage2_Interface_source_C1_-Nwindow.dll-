/*=============================================================================
    CraftWindows.h - phase 6: the C2 manufacture windows (NWindow_C1
    src/inject/craft/CraftWindows.cpp), laid out from C2's NWindow.dll:
      NCRecipeShopWnd            OnCreate 100C3C50 / OnPaint 100C14B0, grids
                                 NCRecipeShopItemWnd: the dwarf's shop setup
      NCRecipeBuyListWnd         OnCreate 100C42F0 / OnPaint 100C20D0, rows
                                 NCRecipeListBox 100C1A00: a customer's view of a store
      NCRecipeBuyManufactureWnd  OnCreate 100C23C0 / OnPaint 100C28D0, grid
                                 NCRecipeManufactureItemWnd: a customer's manufacture
      NCRecipeBookWnd            OnCreate 100C7010 / OnPaint 100C4E10: the recipe book
                                 of the "Create Item" skill
      NCRecipeManufactureWnd     OnCreate 100C7390 / OnPaint 100C5240: the player's own
                                 manufacture (C2 paints it like the store's, so the
                                 store's is this window with the store's data and
                                 buttons)
      NCRecipeTreeWnd            "Recipe Information", the ingredient tree
    (the addresses are C2's). All are 240 x 365 with the title "Item Creation"
    (SysString 663 of C2), open at (screen width - 290, 205) and share one saved
    spot (C2: WindowsInfo.ini [0]); the store's list and manufacture, and the book
    and the own manufacture, take each other's place there. The tree has a spot of
    its own. Grids are C2's NCItemWnd: 6 columns of 32 px icons, 35 px apart; every
    coordinate below is C2's (window or list relative).
    What each shows comes from the session (CraftSession.h); CraftUi.cpp opens,
    refreshes and closes them as the server's data arrives.
=============================================================================*/
#pragma once

#include "features/craft/CraftWnd.h"

#if NW_FEATURES

#include "features/craft/CraftSession.h"

// NCRecipeShopWnd: the dwarf's shop setup (C2 NCRecipeShopWnd)
class NCRecipeShopWnd : public NCRecipeWnd
{
    NW_DECLARE_L2_CLASS(NCRecipeShopWnd, NCRecipeWnd, 0)

    NCRecipeListWnd* BookGrid;               // "Recipe List": the dwarven recipe book
    NCRecipeListWnd* ShopGrid;               // "Production List": the offers
    NCraftOffer Offers[CRAFT_MaxShop];       // edited here, sent whole by Start
    INT NumOffers;
    INT PendingRecipe;                       // the recipe whose price is being asked

    NCRecipeShopWnd();

    // RecipeShopManageList: both grids again, the offers from the server's list
    void Load();

    virtual void OnCreateChildren();
    virtual void PaintWindow(NCGDevice* Device);
    virtual void PaintListItem(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X, INT Y,
                               UBOOL bSelected);
    virtual void OnListClick(NCRecipeListWnd* List, INT Index, UBOOL bDouble);
    virtual void PaintListToolTip(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X,
                                  INT Y);
    virtual void OnCommand(INT ID, INT Value);
    virtual void OnDialogNumber(SQWORD Price);
    virtual void OnDialogText(const TCHAR* Text);
    virtual void OnUserClose();               // the close button: RequestRecipeShopManageQuit

private:
    const NCraftRecipeRow* BookRow(INT RecipeID);
    const NCraftRecipeRow* RowOf(NCRecipeListWnd* List, INT Index);
    void AskPrice(INT BookIndex);
    void RemoveOffer(INT Index);
    void Clear();
    void RefreshGrids();
};

// NCRecipeBuyListWnd: a customer's view of a manufacture store (C2 NCRecipeBuyListWnd)
class NCRecipeBuyListWnd : public NCRecipeWnd
{
    NW_DECLARE_L2_CLASS(NCRecipeBuyListWnd, NCRecipeWnd, 0)

    UTexture* AdenaTex;                      // "L2UI.WindowIcon.WindowIconAdena"
    UTexture* HighlightTex;                  // "NWindow.MenuHighlight", the selected row
    NCRecipeListWnd* Items;

    NCRecipeBuyListWnd();

    // RecipeShopSellList: the rows again, no selection
    void Refresh();

    virtual void OnCreateChildren();
    virtual void PaintWindow(NCGDevice* Device);
    virtual void PaintListItem(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X, INT Y,
                               UBOOL bSelected);
    virtual void OnListClick(NCRecipeListWnd* List, INT Index, UBOOL bDouble);
};

// NCRecipeManufactureWnd: the "Create Item" skill, the player makes the item himself (C2
// NCRecipeManufactureWnd); with the store's data and buttons it is NCRecipeBuyManufactureWnd
class NCRecipeManufactureWnd : public NCRecipeWnd
{
    NW_DECLARE_L2_CLASS(NCRecipeManufactureWnd, NCRecipeWnd, 0)

    UTexture* ChatBackTex;                   // "NWindow.ChatBack": a missing ingredient
    NCRecipeListWnd* Mats;                   // "Ingredient(s)"

    NCRecipeManufactureWnd();

    // RecipeItemMakeInfo / RecipeShopItemInfo: the ingredients again
    void Refresh();

    virtual const NCraftData& Data();        // the craft view
    virtual void DoCreate();                 // Create: mfg_craft
    virtual void DoPrev();                   // <PREV: back to the book (mfg_book)

    virtual void OnCreateChildren();
    virtual void PaintWindow(NCGDevice* Device);
    virtual void PaintListItem(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X, INT Y,
                               UBOOL bSelected);
    virtual void OnListClick(NCRecipeListWnd* List, INT Index, UBOOL bDouble);
    virtual void PaintListToolTip(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X,
                                  INT Y);
    virtual void OnCommand(INT ID, INT Value);
};

// NCRecipeBuyManufactureWnd: a customer's manufacture - the dwarf makes it, the customer pays
// the fee (C2 NCRecipeBuyManufactureWnd)
class NCRecipeBuyManufactureWnd : public NCRecipeManufactureWnd
{
    NW_DECLARE_L2_CLASS(NCRecipeBuyManufactureWnd, NCRecipeManufactureWnd, 0)

    NCRecipeBuyManufactureWnd();

    virtual const NCraftData& Data();        // the detail view
    virtual void DoCreate();                 // Create: mfg_make
    virtual void DoPrev();                   // <PREV: back to the store (mfg_store)
};

// NCRecipeBookWnd: the recipe book of the "Create Item" skill (C2 NCRecipeBookWnd)
class NCRecipeBookWnd : public NCRecipeWnd
{
    NW_DECLARE_L2_CLASS(NCRecipeBookWnd, NCRecipeWnd, 0)

    NCRecipeListWnd* Grid;
    INT PendingDelete;                       // the recipe the trash asks about

    NCRecipeBookWnd();

    void Refresh();

    virtual void OnCreateChildren();
    virtual void PaintWindow(NCGDevice* Device);
    virtual void PaintListItem(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X, INT Y,
                               UBOOL bSelected);
    virtual void OnListClick(NCRecipeListWnd* List, INT Index, UBOOL bDouble);
    virtual void PaintListToolTip(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X,
                                  INT Y);
    virtual void OnCommand(INT ID, INT Value);
    virtual void OnDialogConfirm();
};

// NCRecipeTreeWnd: "Recipe Information", the ingredient tree of a recipe (C2 NCRecipeTreeWnd)
class NCRecipeTreeWnd : public NCRecipeWnd
{
    NW_DECLARE_L2_CLASS(NCRecipeTreeWnd, NCRecipeWnd, 0)

    UTexture* ChatBackTex;                   // a missing ingredient
    UTexture* FrameTex;                      // "c1recipe.RecipeTreeIconBack": a closed branch
    UTexture* FrameOpenTex;                  // "..._click": an open branch
    UTexture* FrameLeafTex;                  // "c1recipe.RecipeTreeIconDisableBack": a leaf
    UTexture* PlusTex;                       // "c1recipe.treePlus"
    UTexture* MinusTex;                      // "c1recipe.treeMinus"
    NCRecipeListWnd* Tree;
    INT Visible[CRAFT_MaxNodes];             // the nodes shown, in order
    INT NumVisible;
    BYTE Expanded[CRAFT_MaxNodes];           // per node of the tree data
    INT NumExpanded;                         // the node count they were made for

    NCRecipeTreeWnd();

    // new tree data: only the root open (C2 starts with every node closed)
    void Refresh();

    virtual void OnCreateChildren();
    virtual void PaintWindow(NCGDevice* Device);
    virtual void PaintListItem(NCRecipeListWnd* List, NCGDevice* Device, INT Index, INT X, INT Y,
                               UBOOL bSelected);
    virtual void OnListClick(NCRecipeListWnd* List, INT Index, UBOOL bDouble);
    virtual void OnCommand(INT ID, INT Value);

private:
    void Rebuild();
};

#endif  // NW_FEATURES
