// Phase 6: the craft windows' protocol v1 parser (src/features/craft/CraftData.cpp) outside the
// game. The messages are built as the server builds them (Lucera C1,
// l2/gameserver/utils/C1Manufacture.java: data() "L2W mfg 1 <view>\n", ints() "key=a,b\n",
// text() "key=value\n" with CR/LF turned to spaces), one per view, plus NWindow_C1's
// tools/craft_data_test.cpp cases. Exit code 0 when every check passes.
//   cmake --build build --config Release --target nwd_craft_test
//   build\craft\Release\nwd_craft_test.exe
#include "features/craft/CraftData.h"

#include <stdio.h>

static INT GFail = 0;
static INT GChecks = 0;

#define CHECK(c)                                                         \
    do                                                                   \
    {                                                                    \
        GChecks++;                                                       \
        if (!(c))                                                        \
        {                                                                \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c);          \
            GFail++;                                                     \
        }                                                                \
    } while (0)

static UBOOL Same(const TCHAR* A, const TCHAR* B)
{
    return wcscmp(A, B) == 0;
}

static NCraftData D;   // 37 KB: not on the stack

int main()
{
    // setupData(): the dwarf's shop setup, a book of two (one row from an older server with six
    // fields), one offer, a message
    CHECK(ParseCraftData(TEXT("L2W mfg 1 setup\n")
                         TEXT("self=268480123\n")
                         TEXT("open=0\n")
                         TEXT("title=Dwarven, Craft = 1\n")
                         TEXT("mp=312,845\n")
                         TEXT("book=1,1889,1,30,100,1\n")
                         TEXT("book=21,1458,10,45,70,2,2921,2,30\n")
                         TEXT("shop=21,15000\n")
                         TEXT("msg=Your store is now OPEN.\n"),
                         &D));
    CHECK(D.View == CV_Setup && D.Version == 1);
    CHECK(D.Self == 268480123 && !D.bOpen && D.MpCur == 312 && D.MpMax == 845);
    CHECK(Same(D.Title, TEXT("Dwarven, Craft = 1")));   // text keys keep commas and '='
    CHECK(D.NumBook == 2 && D.Book[1].RecipeID == 21 && D.Book[1].ProductID == 1458 &&
          D.Book[1].ProductCount == 10 && D.Book[1].MpCost == 45 && D.Book[1].Rate == 70 &&
          D.Book[1].Level == 2 && D.Book[1].ScrollID == 2921 && D.Book[1].Grade == 2 &&
          D.Book[1].Weight == 30);
    CHECK(D.Book[0].ScrollID == 0 && D.Book[0].Grade == 0 && D.Book[0].Weight == 0);
    CHECK(D.NumShop == 1 && D.Shop[0].RecipeID == 21 && D.Shop[0].Fee == 15000);
    CHECK(Same(D.Msg, TEXT("Your store is now OPEN.")));

    // listData(): a customer at the store (CRLF lines, an adena amount past 32 bits)
    CHECK(ParseCraftData(TEXT("L2W mfg 1 list\r\n")
                         TEXT("dwarf=77\r\n")
                         TEXT("name=Gimli\r\n")
                         TEXT("title=Craft\r\n")
                         TEXT("mp=10,20\r\n")
                         TEXT("adena=4000000000\r\n")
                         TEXT("preview=0\r\n")
                         TEXT("item=21,1458,10,15000,45,70\r\n")
                         TEXT("item=1,1889,1,0,30,100\r\n"),
                         &D));
    CHECK(D.View == CV_List && D.Dwarf == 77 && Same(D.Name, TEXT("Gimli")) &&
          Same(D.Title, TEXT("Craft")));
    CHECK(D.Adena == 4000000000LL && !D.bPreview && D.MpCur == 10 && D.MpMax == 20);
    CHECK(D.NumItems == 2 && D.Items[0].Fee == 15000 && D.Items[0].MpCost == 45 &&
          D.Items[0].Rate == 70 && D.Items[0].ProductCount == 10 && D.Items[1].Fee == 0);
    CHECK(D.NumBook == 0 && D.NumShop == 0 && !D.Msg[0]);   // the previous message is gone

    // detailData(): the customer's manufacture, after a failure; a key the parser does not know
    CHECK(ParseCraftData(TEXT("L2W mfg 1 detail\n")
                         TEXT("dwarf=77\n")
                         TEXT("name=Gimli\n")
                         TEXT("recipe=21\n")
                         TEXT("product=1458,10\n")
                         TEXT("grade=3\n")
                         TEXT("owned=12\n")
                         TEXT("fee=15000\n")
                         TEXT("mpcost=45\n")
                         TEXT("rate=70\n")
                         TEXT("mp=10,20\n")
                         TEXT("adena=5\n")
                         TEXT("mat=1864,2,0\n")
                         TEXT("mat=1865,1,3\n")
                         TEXT("result=2\n")
                         TEXT("msg=Failed to create Steel x10 (fee 15000 adena).\n")
                         TEXT("newkey=9\n"),
                         &D));
    CHECK(D.View == CV_Detail && D.RecipeID == 21 && D.ProductID == 1458 &&
          D.ProductCount == 10 && D.Fee == 15000 && D.MpCost == 45 && D.Rate == 70 &&
          D.Result == 2);
    CHECK(D.Grade == 3 && D.Owned == 12 && D.Adena == 5);
    CHECK(D.NumMats == 2 && D.Mats[0].ItemID == 1864 && D.Mats[0].Need == 2 &&
          D.Mats[0].Have == 0 && D.Mats[1].Have == 3);
    CHECK(Same(D.Msg, TEXT("Failed to create Steel x10 (fee 15000 adena).")));

    // closedData(): the store is gone (no final newline)
    CHECK(ParseCraftData(TEXT("L2W mfg 1 closed\ndwarf=77\nmsg=This store is closed."), &D));
    CHECK(D.View == CV_Closed && D.Dwarf == 77 && Same(D.Msg, TEXT("This store is closed.")));

    // sendBook(): the recipe book of the Create Item skill
    CHECK(ParseCraftData(TEXT("L2W mfg 1 book\n")
                         TEXT("dwarven=1\n")
                         TEXT("limit=62\n")
                         TEXT("mp=100,845\n")
                         TEXT("book=1,1889,1,30,100,1,1666,0,30\n")
                         TEXT("book=21,1458,10,45,70,2,2921,2,30\n")
                         TEXT("book=64,2437,1,120,70,5,3024,3,120\n"),
                         &D));
    CHECK(D.View == CV_Book && D.bDwarven && D.Limit == 62 && D.MpCur == 100 && D.NumBook == 3);
    CHECK(D.Book[2].RecipeID == 64 && D.Book[2].ScrollID == 3024 && D.Book[2].Grade == 3 &&
          D.Book[2].Weight == 120 && D.Book[2].Level == 5);

    // sendCraft(): the player's own manufacture, just made
    CHECK(ParseCraftData(TEXT("L2W mfg 1 craft\n")
                         TEXT("recipe=1\n")
                         TEXT("product=1889,1\n")
                         TEXT("grade=0\n")
                         TEXT("owned=4\n")
                         TEXT("mpcost=30\n")
                         TEXT("rate=100\n")
                         TEXT("mp=70,845\n")
                         TEXT("mat=1864,10,40\n")
                         TEXT("result=1\n")
                         TEXT("msg=Coarse Bone Powder has been created.\n"),
                         &D));
    CHECK(D.View == CV_Craft && D.RecipeID == 1 && D.ProductID == 1889 && D.ProductCount == 1 &&
          D.Owned == 4 && D.Result == 1 && D.NumMats == 1 && D.Mats[0].Have == 40);

    // treeData(): the root and depth-first nodes
    CHECK(ParseCraftData(TEXT("L2W mfg 1 tree\n")
                         TEXT("recipe=21\n")
                         TEXT("product=1458,10\n")
                         TEXT("grade=0\n")
                         TEXT("mpcost=45\n")
                         TEXT("rate=70\n")
                         TEXT("level=2\n")
                         TEXT("node=0,1458,0,3,1\n")
                         TEXT("node=1,1869,5,1,1\n")
                         TEXT("node=2,1864,10,40,0\n")
                         TEXT("node=1,1870,2,0,0\n"),
                         &D));
    CHECK(D.View == CV_Tree && D.Level == 2 && D.NumNodes == 4);
    CHECK(D.Nodes[0].Depth == 0 && D.Nodes[0].bChildren && D.Nodes[1].Depth == 1 &&
          D.Nodes[1].Need == 5 && D.Nodes[1].Have == 1 && D.Nodes[2].Depth == 2 &&
          !D.Nodes[2].bChildren && D.Nodes[3].ItemID == 1870);

    // a server path that forces <html> hands the text on unchanged: CraftUi skips it first, the
    // parser does not
    CHECK(!ParseCraftData(TEXT("<html><body>L2W mfg 1 closed\ndwarf=1\n"), &D));
    // not ours
    CHECK(!ParseCraftData(TEXT("<html><body>hello</body></html>"), &D));
    CHECK(!ParseCraftData(TEXT("L2W mfg 1 nosuchview\n"), &D));
    CHECK(!ParseCraftData(TEXT("L2W mfg x setup\n"), &D));
    CHECK(!ParseCraftData(TEXT("L2W shot 1 state\n"), &D));
    CHECK(!ParseCraftData(NULL, &D));

    // records past the capacity are left out, the rest stays right
    static TCHAR Big[CRAFT_MaxBook * 48 + 64];
    INT N = swprintf(Big, TEXT("L2W mfg 1 book\ndwarven=1\n"));
    for (INT i = 0; i < CRAFT_MaxBook + 5; i++)
        N += swprintf(Big + N, TEXT("book=%d,%d,1,1,100,1,0,0,0\n"), i + 1, 1000 + i);
    CHECK(ParseCraftData(Big, &D));
    CHECK(D.NumBook == CRAFT_MaxBook && D.Book[CRAFT_MaxBook - 1].RecipeID == CRAFT_MaxBook);

    // a text longer than its field is cut
    CHECK(ParseCraftData(TEXT("L2W mfg 1 list\nname=0123456789012345678901234567890123456789XYZ\n"),
                         &D));
    CHECK(wcslen(D.Name) == ARRAY_COUNT(D.Name) - 1);

    if (GFail)
        printf("craft_data_test: %d of %d checks FAILED\n", GFail, GChecks);
    else
        printf("craft_data_test: all %d checks passed\n", GChecks);
    return GFail ? 1 : 0;
}
