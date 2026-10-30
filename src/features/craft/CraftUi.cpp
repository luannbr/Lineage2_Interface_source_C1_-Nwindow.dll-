// Phase 6: CraftUi (include/features/craft/CraftUi.h) - NWindow_C1 src/inject/craft/CraftUi.cpp
// (install, transport, texture test), the tick of CraftWindows.cpp (CraftWindows_OnData /
// CraftWindows_Tick) and nwui::Tick of NwUi.cpp (outside the world, Esc).
//
// NWindow_C1 caught the Engine's notifications by retargeting ILT thunks of Engine.dll
// (IltHook.h). Here they are our code or read from it:
//   * OnNpcHtmlMessage only hands the text to the console's slot 66: NConsoleWnd::NpcHtmlMessage
//     calls CraftUi_OnNpcHtml before anything else (no sound, no page for a data message);
//   * OnUserInfo reaches no console slot, but it is what spawns the player's pawn: a local User
//     that NConsoleWnd::GetMyUser returns and did not before is the "first UserInfo for a new
//     local User*" NWindow_C1 announced on, read from the tick;
//   * OnRestartResponse / OnAcceptLogOut only cleared the announced User: leaving the world
//     (console state not 6) clears it here, as AutoShot does for its "ss_ui 1".
#include "features/craft/CraftUi.h"
#include "features/craft/CraftWindows.h"
#include "features/ManufactureCmd.h"

#include "nwindow/NCConsole.h"

// the windows, made the first time one is needed (NWindow_C1's globals, in their order: Esc
// closes the first one open that has the focus or is in front)
enum
{
    CW_Shop,               // SetupWnd
    CW_BuyList,            // ListWnd
    CW_BuyManufacture,     // MakeWnd
    CW_Manufacture,        // CraftWnd
    CW_Book,               // BookWnd
    CW_Tree,               // TreeWnd
    CW_Max
};

// UObject::StaticLoadObject's flags (UE2 UnObjBas.h)
enum { LOAD_NoWarn = 0x0002, LOAD_Quiet = 0x2000 };

static UBOOL GSettingsRead;
static UBOOL GNativeUI;              // [Craft] NativeUI
static UBOOL GTextureTest;           // [Craft] TextureTest
static UBOOL GTextureTestDone;
// "mfg_ui 1" went out for the player with this server id. Not the User pointer: GetMyUser()
// returns the record in the game data's user table, which moves when other users come into
// view (the test showed a new pointer every few seconds, and a new announcement each time)
static INT GAnnouncedID;
static NConsoleWnd* GWndConsole;     // the console the windows were made under
static NCRecipeWnd* GWnds[CW_Max];
static UBOOL GEscDown;
// what the data received asks of the windows, done on the tick (CraftWindows_OnData)
static UBOOL GDirtySetup;
static UBOOL GDirtyList;
static UBOOL GDirtyDetail;
static UBOOL GDirtyBook;
static UBOOL GDirtyCraft;
static UBOOL GDirtyTree;
static UBOOL GClosedStore;
static TCHAR GPendingMsg[192];
static UBOOL GHasMsg;
static NCraftData GParsed;           // the message being read (37 KB: not on the stack)

static const TCHAR* const GTextures[] = {
    TEXT("c1recipe.recipeBookBack"),        TEXT("c1recipe.recipeMakingBack"),
    TEXT("c1recipe.recipePersonalListBack"), TEXT("c1recipe.recipePersonalMakingBack"),
    TEXT("c1recipe.recipeTreeBack"),        TEXT("c1recipe.RecipeTreeBtn"),
    TEXT("c1recipe.RecipeTreeBtn_click"),   TEXT("c1recipe.RecipeTreeIconBack"),
    TEXT("c1recipe.RecipeTreeIconBack_click"), TEXT("c1recipe.RecipeTreeIconDisableBack"),
    TEXT("c1recipe.treePlus"),              TEXT("c1recipe.treeMinus"),
    TEXT("c1recipe.manufacture"),           TEXT("c1recipe.manufacture_click"),
};

// the [Craft] switches, read once (NWindow_C1 InstallCraftUi at DLL attach)
static void ReadSettings()
{
    GSettingsRead = 1;
    GTextureTest = nwf::IniInt("Craft", "TextureTest", 0) != 0;
    GNativeUI = nwf::IniInt("Craft", "NativeUI", 0) != 0;
    if (GTextureTest)
        nwf::Log("craftui", "install: texture test armed");
    if (GNativeUI)
        nwf::Log("craftui", "install: native UI on (console callbacks)");
}

static void Narrow(const TCHAR* Text, char* Out, INT Size)
{
    WideCharToMultiByte(CP_UTF8, 0, Text ? Text : TEXT(""), -1, Out, Size, NULL, NULL);
    Out[Size - 1] = 0;
}

/*-----------------------------------------------------------------------------
    The windows.
-----------------------------------------------------------------------------*/

static NCRecipeWnd* Wnd(INT Kind)
{
    if (!GWnds[Kind])
    {
        NCRecipeWnd* W = NULL;
        switch (Kind)
        {
        case CW_Shop:
            W = new NCRecipeShopWnd;
            break;
        case CW_BuyList:
            W = new NCRecipeBuyListWnd;
            break;
        case CW_BuyManufacture:
            W = new NCRecipeBuyManufactureWnd;
            break;
        case CW_Manufacture:
            W = new NCRecipeManufactureWnd;
            break;
        case CW_Book:
            W = new NCRecipeBookWnd;
            break;
        default:
            W = new NCRecipeTreeWnd;
            break;
        }
        W->Create(GConsole->Desktop);
        GWnds[Kind] = W;
    }
    return GWnds[Kind];
}

static void CloseWnd(INT Kind)
{
    if (GWnds[Kind])
        GWnds[Kind]->Close();
}

// the stored view's work for the tick; a server notice ("This store is closed.", errors) goes to
// the chat like a stock system message, a manufacture result shows in the window instead (C2's
// result line)
static void QueueView(INT View, INT Result, const TCHAR* Msg)
{
    switch (View)
    {
    case CV_Setup:
        GDirtySetup = 1;
        break;
    case CV_List:
        GDirtyList = 1;
        break;
    case CV_Detail:
        GDirtyDetail = 1;
        break;
    case CV_Book:
        GDirtyBook = 1;
        break;
    case CV_Craft:
        GDirtyCraft = 1;
        break;
    case CV_Tree:
        GDirtyTree = 1;
        break;
    case CV_Closed:
        GClosedStore = 1;
        break;
    default:
        return;
    }
    UBOOL bResultLine = (View == CV_Detail || View == CV_Craft) && (Result == 1 || Result == 2);
    if (Msg && *Msg && !bResultLine)
    {
        lstrcpynW(GPendingMsg, Msg, ARRAY_COUNT(GPendingMsg));
        GHasMsg = 1;
    }
}

// the data received, in the windows: the store's list and manufacture take each other's
// place, the book and the own manufacture too (NWindow_C1 CraftWindows_Tick)
static void ApplyData()
{
    if (GDirtySetup)
    {
        GDirtySetup = 0;
        NCRecipeShopWnd* Shop = static_cast<NCRecipeShopWnd*>(Wnd(CW_Shop));
        Shop->Load();
        Shop->Open();
    }
    if (GDirtyList)
    {
        GDirtyList = 0;
        CloseWnd(CW_BuyManufacture);
        NCRecipeBuyListWnd* List = static_cast<NCRecipeBuyListWnd*>(Wnd(CW_BuyList));
        List->Refresh();
        List->Open();
    }
    if (GDirtyDetail)
    {
        GDirtyDetail = 0;
        CloseWnd(CW_BuyList);
        NCRecipeBuyManufactureWnd* Make =
            static_cast<NCRecipeBuyManufactureWnd*>(Wnd(CW_BuyManufacture));
        Make->Refresh();
        Make->Open();
    }
    if (GDirtyBook)
    {
        GDirtyBook = 0;
        CloseWnd(CW_Manufacture);
        NCRecipeBookWnd* Book = static_cast<NCRecipeBookWnd*>(Wnd(CW_Book));
        Book->Refresh();
        Book->Open();
    }
    if (GDirtyCraft)
    {
        GDirtyCraft = 0;
        CloseWnd(CW_Book);
        NCRecipeManufactureWnd* Craft = static_cast<NCRecipeManufactureWnd*>(Wnd(CW_Manufacture));
        Craft->Refresh();
        Craft->Open();
    }
    if (GDirtyTree)
    {
        GDirtyTree = 0;
        NCRecipeTreeWnd* Tree = static_cast<NCRecipeTreeWnd*>(Wnd(CW_Tree));
        Tree->Refresh();
        Tree->Open();
    }
    if (GClosedStore)
    {
        GClosedStore = 0;
        CloseWnd(CW_BuyList);
        CloseWnd(CW_BuyManufacture);
    }
    if (GHasMsg)
    {
        GHasMsg = 0;
        NCRecipeWnd::SystemMessage(GPendingMsg);
    }
}

// outside the world: every window closed, what was pending dropped
static void CloseAll()
{
    GDirtySetup = GDirtyList = GDirtyDetail = GDirtyBook = GDirtyCraft = GDirtyTree = 0;
    GClosedStore = GHasMsg = 0;
    for (INT i = 0; i < CW_Max; i++)
        if (GWnds[i] && GWnds[i]->IsOpen())
            GWnds[i]->Close();
}

// Esc: the stock hot keys take it before any window sees it (NConsoleWnd::HookShortCutKey); a
// press closes the first open window that has the focus or is in front (NWindow_C1 polled it)
static void CheckEscape()
{
    UBOOL bFocus = !GViewportHWnd || GetForegroundWindow() == GViewportHWnd;
    UBOOL bEsc = bFocus && (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    if (bEsc && !GEscDown)
    {
        for (INT i = 0; i < CW_Max; i++)
        {
            NCRecipeWnd* W = GWnds[i];
            if (W && W->IsOpen() && (W->FocusInside() || W->Frontmost()))
            {
                W->Close();
                break;
            }
        }
    }
    GEscDown = bEsc;
}

void CraftUi_ToggleTree(INT RecipeID)
{
    NCRecipeWnd* Tree = GWnds[CW_Tree];
    if (Tree && Tree->IsOpen())
        Tree->Close();   // the C2 tree button is a toggle too
    else if (RecipeID)
        GetCraftSession().ViewTree(RecipeID);   // the answer opens it
}

/*-----------------------------------------------------------------------------
    Transport.
-----------------------------------------------------------------------------*/

// the local User, NULL before the pawn is there (no controller yet: none either)
static User* LocalUser(NConsoleWnd* Console)
{
    UEngine* Engine = Console->Engine;
    if (!Engine || !Engine->Client || Engine->Client->Viewports.Num() < 1 ||
        !Engine->Client->Viewports(0) || !Engine->Client->Viewports(0)->Actor)
        return NULL;
    return Console->GetMyUser();
}

// entering the world: "mfg_ui 1" once per player (again if it could not be sent); before
// AutoShot's "ss_ui 1" of the same tick, as NWindow_C1 sent them
static void Announce(NConsoleWnd* Console)
{
    User* Me = LocalUser(Console);
    if (!Me || !Me->ObjectID || Me->ObjectID == GAnnouncedID)
        return;
    if (nwf::SendBypassToServer(TEXT("mfg_ui 1")))
    {
        GAnnouncedID = Me->ObjectID;
        nwf::Log("craftui", "announced native craft UI (player %d)", Me->ObjectID);
    }
}

// "L2W mfg" data, also behind "<html><body>" (a server path that forces <html>)
static const TCHAR* CraftDataIn(const TCHAR* Html)
{
    if (!Html)
        return NULL;
    const TCHAR* P = Html;
    static const TCHAR Wrap[] = TEXT("<html><body>");
    INT i = 0;
    for (; Wrap[i]; i++)
    {
        TCHAR C = P[i];
        if (C >= 'A' && C <= 'Z')
            C = (TCHAR)(C - 'A' + 'a');
        if (C != Wrap[i])
            break;
    }
    if (!Wrap[i])
        P += i;
    static const TCHAR Marker[] = TEXT("L2W mfg");
    for (i = 0; Marker[i]; i++)
        if (P[i] != Marker[i])
            return NULL;
    return P;
}

static void LogData(const NCraftData& D)
{
    if (!nwf::LogEnabled())
        return;
    char Title[96], Name[64], Msg[200];
    Narrow(D.Title, Title, sizeof(Title));
    Narrow(D.Name, Name, sizeof(Name));
    Narrow(D.Msg, Msg, sizeof(Msg));
    switch (D.View)
    {
    case CV_Setup:
        nwf::Log("craftui", "data v%d setup self=%d open=%d title='%s' mp=%d/%d book=%d shop=%d msg='%s'",
                 D.Version, D.Self, D.bOpen, Title, D.MpCur, D.MpMax, D.NumBook, D.NumShop, Msg);
        break;
    case CV_List:
        nwf::Log("craftui", "data v%d list dwarf=%d '%s' title='%s' mp=%d/%d items=%d preview=%d msg='%s'",
                 D.Version, D.Dwarf, Name, Title, D.MpCur, D.MpMax, D.NumItems, D.bPreview, Msg);
        break;
    case CV_Detail:
        nwf::Log("craftui", "data v%d detail dwarf=%d recipe=%d product=%d x%d fee=%d mp=%d/%d cost=%d "
                 "rate=%d mats=%d result=%d msg='%s'",
                 D.Version, D.Dwarf, D.RecipeID, D.ProductID, (INT)D.ProductCount, (INT)D.Fee, D.MpCur,
                 D.MpMax, D.MpCost, D.Rate, D.NumMats, D.Result, Msg);
        break;
    default:
        nwf::Log("craftui", "data v%d closed dwarf=%d msg='%s'", D.Version, D.Dwarf, Msg);
        break;
    }
}

UBOOL CraftUi_OnNpcHtml(const TCHAR* Html)
{
    if (!GSettingsRead)
        ReadSettings();
    if (!GNativeUI)
        return 0;
    const TCHAR* Data = CraftDataIn(Html);
    if (!Data)
        return 0;
    if (!ParseCraftData(Data, &GParsed))
    {
        if (nwf::LogEnabled())
        {
            char Head[160];
            Narrow(Data, Head, sizeof(Head));
            nwf::Log("craftui", "data: unparsed (%d chars) %s", (INT)wcslen(Data), Head);
        }
        return 1;
    }
    GetCraftSession().OnData(GParsed);
    QueueView(GParsed.View, GParsed.Result, GParsed.Msg);
    LogData(GParsed);
    return 1;
}

/*-----------------------------------------------------------------------------
    The texture test ([Craft] TextureTest=1): the 14 c1recipe textures, once, logged.
-----------------------------------------------------------------------------*/

typedef const TCHAR*(__fastcall* FGetFullName)(UObject* Object, void* Edx, TCHAR* Buffer);

static void TextureTest()
{
    // UObject::GetFullName is not in our Core declarations: taken from Core.dll as NWindow_C1 did
    HMODULE Core = GetModuleHandleW(TEXT("Core.dll"));
    FGetFullName GetFullName =
        Core ? (FGetFullName)GetProcAddress(Core, "?GetFullName@UObject@@QBEPBGPAG@Z") : NULL;
    INT Loaded = 0;
    INT Total = ARRAY_COUNT(GTextures);
    for (INT i = 0; i < Total; i++)
    {
        UObject* Tex = NULL;
        UBOOL bThrew = 0;
        try
        {
            Tex = UObject::StaticLoadObject(UTexture::StaticClass(), NULL, GTextures[i], NULL,
                                            LOAD_NoWarn | LOAD_Quiet, NULL);
        }
        catch (...)
        {
            bThrew = 1;
        }
        char Name[96];
        Narrow(GTextures[i], Name, sizeof(Name));
        if (!Tex)
        {
            nwf::Log("craftui", "texture %s -> %s", Name, bThrew ? "EXCEPTION" : "NULL");
            continue;
        }
        Loaded++;
        char Full[160] = "?";
        if (GetFullName)
        {
            TCHAR Buffer[256];
            Buffer[0] = 0;
            GetFullName(Tex, NULL, Buffer);
            Narrow(Buffer, Full, sizeof(Full));
        }
        nwf::Log("craftui", "texture %s -> %p (%s)", Name, Tex, Full);
    }
    nwf::Log("craftui", "texture test: %d/%d loaded", Loaded, Total);
}

/*-----------------------------------------------------------------------------
    The tick.
-----------------------------------------------------------------------------*/

void CraftUi_OnTick(NConsoleWnd* Console)
{
    if (!GSettingsRead)
        ReadSettings();
    if (!Console || !Console->bInitialized)
        return;
    if (GNativeUI && Console->Desktop)
    {
        if (Console != GWndConsole)   // another console: the old tree is gone
        {
            for (INT i = 0; i < CW_Max; i++)
                GWnds[i] = NULL;
            NCRecipeWnd::ForgetCaches();
            GAnnouncedID = 0;
            GWndConsole = Console;
        }
        if (Console->State == L2CS_InGame)
        {
            Announce(Console);
            ApplyData();
            CheckEscape();
        }
        else
        {
            // leaving the world (the lobby, the login screen) forgets the announcement; the
            // loading screen comes and goes without a new player (AutoShot's rule). The windows
            // close whenever the state is not in-world, as NWindow_C1's NwUi
            if (Console->State != L2CS_Loading)
                GAnnouncedID = 0;
            CloseAll();
        }
    }
    if (GTextureTest && !GTextureTestDone)
    {
        GTextureTestDone = 1;
        TextureTest();
    }
}
