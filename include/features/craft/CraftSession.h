/*=============================================================================
    CraftSession.h - phase 6: what the craft windows show and the actions they send
    (NWindow_C1 src/inject/craft/CraftSession.h).

    Holds the latest data message of each view and turns the windows' actions into
    the mfg_* bypasses C1Manufacture answers. Like the C2 packets they stand for:
      * setup (C2 RecipeShopManageList): the dwarf edits the offer list in the
        window; Start sends the whole list (RecipeShopListSet), Abort and the close
        button send the quit (RecipeShopManageQuit);
      * customer (RecipeShopSellList / RecipeShopItemInfo) and the recipe book: every
        action is answered with a fresh view, so those windows show the last one.
=============================================================================*/
#pragma once

#include "features/craft/CraftData.h"

// an entry of the offer list the dwarf edits
struct NCraftOffer
{
    INT RecipeID;
    SQWORD Fee;
};

class NCraftSession
{
public:
    // stores a data message in its view (a closed store clears the customer's views)
    INT OnData(const NCraftData& D);
    void Reset();

    const NCraftData& Setup() const { return SetupView; }
    const NCraftData& List() const { return ListView; }
    const NCraftData& Detail() const { return DetailView; }
    const NCraftData& Book() const { return BookView; }
    const NCraftData& Craft() const { return CraftView; }
    const NCraftData& Tree() const { return TreeView; }

    // the dwarf's shop setup (C2 NCRecipeShopWnd)
    UBOOL StartStore(const NCraftOffer* Offers, INT Count);   // mfg_start id:fee,...
    UBOOL QuitSetup();                                        // mfg_quit
    UBOOL SetTitle(const TCHAR* Title);                       // mfg_title <text> (the store
                                                              //   message)
    // the recipe book and the player's own manufacture, the "Create Item" skill (C2
    // NCRecipeBookWnd / NCRecipeManufactureWnd; the server keeps them in C1Craft)
    UBOOL OpenBook();                     // mfg_book <dwarven>
    UBOOL ViewRecipe(INT RecipeID);       // mfg_recipe?id=R
    UBOOL CraftItem(INT RecipeID);        // mfg_craft?id=R
    UBOOL DeleteRecipe(INT RecipeID);     // mfg_rmrecipe?id=R
    UBOOL ViewTree(INT RecipeID);         // mfg_tree?id=R (C2 NCRecipeTreeWnd)
    // the customer (C2 NCRecipeBuyListWnd / NCRecipeBuyManufactureWnd)
    UBOOL ViewStore(INT DwarfID);                 // mfg_store?d=D
    UBOOL ViewDetail(INT DwarfID, INT RecipeID);  // mfg_detail?d=D&id=R
    UBOOL Make(INT DwarfID, INT RecipeID);        // mfg_make?d=D&id=R

private:
    NCraftData SetupView;
    NCraftData ListView;
    NCraftData DetailView;
    NCraftData BookView;
    NCraftData CraftView;
    NCraftData TreeView;
};

NCraftSession& GetCraftSession();
