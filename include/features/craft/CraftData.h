/*=============================================================================
    CraftData.h - phase 6: protocol v1 of the native craft windows (NWindow_C1
    src/inject/craft/CraftData.h). The server (Lucera C1, C1Manufacture) sends it
    as the text of an NpcHtmlMessage, only to a client that announced the windows
    ("mfg_ui 1"); the windows answer with the mfg_* bypasses (CraftSession.h).

      L2W mfg 1 <view>\n            view = setup | list | detail | closed | book |
                                    craft | tree
      key=value\n ...               text keys (title, name, msg) take the rest of the
                                    line; record keys are comma-separated integers

    setup : self, open, title, mp=cur,max,
            book=recipe,product,count,mpCost,rate,level,scroll,grade,weight (n),
            shop=recipe,fee (n), msg
    list  : dwarf, name, title, mp=cur,max, adena, preview,
            item=recipe,product,count,fee,mpCost,rate (n), msg
    detail: dwarf, name, recipe, product=id,count, grade, owned, fee, mpcost, rate,
            mp=cur,max, adena, mat=item,need,have (n), result (0 view / 1 made /
            2 failed), msg
    closed: dwarf, msg
    book  : dwarven, limit, mp=cur,max, book=... (n, the setup's rows), msg
            (C2 NCRecipeBookWnd)
    craft : recipe, product=id,count, grade, owned, mpcost, rate, mp=cur,max,
            mat=item,need,have (n), result, msg (C2 NCRecipeManufactureWnd)
    tree  : recipe, product=id,count, grade, mpcost, rate, level,
            node=depth,item,need,have,hasChildren (n, depth first) (C2 NCRecipeTreeWnd)
    grade: the product's crystal grade, 0 none, 1 D, 2 C, 3 B, 4 A, 5 S; scroll: the
    recipe item. Unknown keys are ignored (a newer server may add fields).

    Only UE2's types: the parser also builds on its own, outside the game
    (tools/craft/craft_data_test.cpp, target nwd_craft_test).
=============================================================================*/
#pragma once

#include "ue2/UE2Core.h"

// the view a data message fills (names ours)
enum ECraftView
{
    CV_None,
    CV_Setup,     // the dwarf's shop setup (C2 RecipeShopManageList)
    CV_List,      // a customer's view of a store (C2 RecipeShopSellList)
    CV_Detail,    // a customer's manufacture of one recipe (C2 RecipeShopItemInfo)
    CV_Closed,    // the store the customer looked at is gone
    CV_Book,      // the player's recipe book (C2 RecipeBookItemList)
    CV_Craft,     // the player's own manufacture (C2 RecipeItemMakeInfo)
    CV_Tree,      // the ingredient tree of a recipe
};

// How many records a message keeps of each kind. NWindow_C1 grew vectors; these bounds stay
// far above what the server sends (C1Manufacture: 40 offers, a recipe book of 50 plus the
// skill's bonus, a tree 4 levels deep) and keep the C2 grid's 160 for the list the dwarf edits.
enum
{
    CRAFT_MaxBook = 256,
    CRAFT_MaxShop = 160,
    CRAFT_MaxItems = 160,
    CRAFT_MaxMats = 32,
    CRAFT_MaxNodes = 256,
};

// a recipe of the book (setup, book) or of a store (list: with its fee; setup's shop rows:
// only RecipeID and Fee)
struct NCraftRecipeRow
{
    INT RecipeID;
    INT ProductID;
    SQWORD ProductCount;
    SQWORD Fee;
    INT MpCost;
    INT Rate;
    INT Level;       // book rows
    INT ScrollID;    // book rows: the recipe item (tool tip name and description)
    INT Grade;       // book rows: the product's grade
    INT Weight;      // book rows: the recipe item's weight
};

// a line of the ingredient tree: sub-recipes are the children of their product
struct NCraftTreeNode
{
    INT Depth;
    INT ItemID;
    SQWORD Need;
    SQWORD Have;
    UBOOL bChildren;
};

struct NCraftMaterial
{
    INT ItemID;
    SQWORD Need;
    SQWORD Have;
};

struct NCraftData
{
    INT View;               // ECraftView
    INT Version;
    INT Self;
    INT Dwarf;
    UBOOL bOpen;
    UBOOL bPreview;
    UBOOL bDwarven;
    INT Limit;              // book: how many recipes fit
    INT Level;              // tree: the recipe's level
    INT MpCur;
    INT MpMax;
    SQWORD Adena;
    INT RecipeID;
    INT ProductID;
    SQWORD ProductCount;
    SQWORD Fee;
    INT MpCost;
    INT Rate;
    INT Result;             // detail, craft: 0 shown, 1 made, 2 failed
    INT Grade;              // detail, craft, tree: the product's grade
    SQWORD Owned;           // detail, craft: products already in the viewer's inventory
    TCHAR Title[64];
    TCHAR Name[40];
    TCHAR Msg[192];
    INT NumBook;
    NCraftRecipeRow Book[CRAFT_MaxBook];     // setup, book: the recipe book
    INT NumShop;
    NCraftRecipeRow Shop[CRAFT_MaxShop];     // setup: what the store offers (recipe, fee)
    INT NumItems;
    NCraftRecipeRow Items[CRAFT_MaxItems];   // list
    INT NumMats;
    NCraftMaterial Mats[CRAFT_MaxMats];      // detail, craft
    INT NumNodes;
    NCraftTreeNode Nodes[CRAFT_MaxNodes];    // tree
};

// Reads a data message (text starting with "L2W mfg"); 0 when the header is not one. Out is
// cleared first.
UBOOL ParseCraftData(const TCHAR* Text, NCraftData* Out);
