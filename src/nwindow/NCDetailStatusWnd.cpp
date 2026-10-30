/*=============================================================================
    NCDetailStatusWnd.cpp - the character tab of the status window (Alt+T) and the GM's view
    of another player's status.
    Stock: 0x1005be50 .. 0x1005be67, 0x1005c5b0 .. 0x1005c5ff, 0x1005cdb0 .. 0x1005cdfd,
    0x1005ce30 .. 0x1005e30b, 0x100aea90 .. 0x100aeb64, 0x100b1be0 .. 0x100b32fa.
=============================================================================*/
#include "nwindow/NCDetailStatusWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <stdio.h>
#include <wchar.h>

// exp at the start of each level (index = level), a table of the stock image, and the GM
// window's own copy of it, from level 1 (index = level - 1); same values
#if NW_HOOK_MODE
#define GExpTable (reinterpret_cast<const INT*>(::nw::Addr(0x100eed2c)))
#define GGMExpTable (reinterpret_cast<const INT*>(::nw::Addr(0x100e3a48)))
#else
extern const INT GExpTable[];
extern const INT GGMExpTable[];
#endif

// every text of the window goes out with the same trailing arguments
static FORCEINLINE void DrawString(NCGDevice* Device, INT X, INT Y, DWORD Color, const TCHAR* Text)
{
    Device->DrawText(X, Y, Color, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
}

// a number centered on CenterX in the values' color (spelled out at each use in the stock code)
static FORCEINLINE void DrawNumber(NCGDevice* Device, TCHAR* Buf, INT CenterX, INT Y, INT Value)
{
    SIZE Size;
    appSprintf(Buf, TEXT("%d"), Value);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    DrawString(Device, CenterX - Size.cx / 2, Y, 0xFFB09B79, Buf);
}

// 0x100b1be0
NCDetailStatusWnd::NCDetailStatusWnd()
{
    BackTex = NULL;
}
NW_HOOK_CTOR(0x100b1be0, NCDetailStatusWnd)

// 0x1005c5b0
NW_HOOK_DTOR(0x1005c5b0, NCDetailStatusWnd)

// stock body shared with NControl's (0x10004e90)
INT NCDetailStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100aea90 (vslot 35)
INT NCDetailStatusWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.StatusWnd.statusback"));
    HPTex = FindTexture(TEXT("L2UI.StatusWnd.statusHP1"));
    MPTex = FindTexture(TEXT("L2UI.StatusWnd.statusMP1"));
    EXPTex = FindTexture(TEXT("L2UI.StatusWnd.statusEXP1"));
    WeightTex[0] = FindTexture(TEXT("L2UI.StatusWnd.statusWt1"));
    WeightTex[1] = FindTexture(TEXT("L2UI.StatusWnd.statusWt2"));
    WeightTex[2] = FindTexture(TEXT("L2UI.StatusWnd.statusWt3"));
    WeightTex[3] = FindTexture(TEXT("L2UI.StatusWnd.statusWt4"));
    SPTex = FindTexture(TEXT("L2UI.StatusWnd.statusSP"));
    NameLeftTex = FindTexture(TEXT("L2UI.StatusWnd.statusName"));
    NameRightTex = FindTexture(TEXT("L2UI.StatusWnd.statusName-1"));
    return 1;
}
NW_HOOK_METHOD(0x100aea90, NCDetailStatusWnd, OnCreate)

// stock body shared with NCCompositeBox's (0x10083930)
INT NCDetailStatusWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    return 1;
}

// stock body shared with NCEditBox's (0x100a5d70)
void NCDetailStatusWnd::ShowWindow()
{
    NCWnd::ShowWindow();
}

// 0x100b1c00 (vslot 50) - the labels, then the player's values: bars 84 (HP, MP, weight) and
// 189 (exp) pixels wide, the weight bar not clamped and textured by how loaded the player is;
// title and name centered over the window (each cut to 9 characters and ".." when both are
// wider than 220 pixels together), the name reddened by the karma. Pawn and controller are
// read for the speed without a check.
INT NCDetailStatusWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, 240, 300, 0, 0, 240, 300, BackTex);

    DrawString(Device, 17, 31, 0xFFA3A3A3, *GL2GameData.SysString[430]);   // "Clan"
    DrawString(Device, 17, 46, 0xFFA3A3A3, *GL2GameData.SysString[88]);    // "Lv"
    DrawString(Device, 17, 112, 0xFFFFFFFF, *GL2GameData.SysString[93]);   // "Combat"
    DrawString(Device, 17, 128, 0xFFA3A3A3, *GL2GameData.SysString[94]);   // "P.Atk."
    DrawString(Device, 17, 144, 0xFFA3A3A3, *GL2GameData.SysString[95]);   // "P.Def."
    DrawString(Device, 17, 159, 0xFFA3A3A3, *GL2GameData.SysString[96]);   // "Accuracy"
    DrawString(Device, 17, 174, 0xFFA3A3A3, *GL2GameData.SysString[113]);  // "Critical"
    DrawString(Device, 17, 189, 0xFFA3A3A3, *GL2GameData.SysString[111]);  // "Atk. Spd."
    DrawString(Device, 17, 206, 0xFFFFFFFF, *GL2GameData.SysString[103]);  // "Basic"
    DrawString(Device, 122, 128, 0xFFA3A3A3, *GL2GameData.SysString[98]);  // "M.Atk."
    DrawString(Device, 122, 144, 0xFFA3A3A3, *GL2GameData.SysString[99]);  // "M.Def."
    DrawString(Device, 122, 159, 0xFFA3A3A3, *GL2GameData.SysString[97]);  // "Evasion"
    DrawString(Device, 122, 174, 0xFFA3A3A3, *GL2GameData.SysString[432]); // "Speed"
    DrawString(Device, 122, 189, 0xFFA3A3A3, *GL2GameData.SysString[112]); // "Casting Spd."
    DrawString(Device, 17, 224, 0xFFA3A3A3, *GL2GameData.SysString[104]);  // "STR"
    DrawString(Device, 93, 224, 0xFFA3A3A3, *GL2GameData.SysString[105]);  // "DEX"
    DrawString(Device, 163, 224, 0xFFA3A3A3, *GL2GameData.SysString[106]); // "CON"
    DrawString(Device, 17, 241, 0xFFA3A3A3, *GL2GameData.SysString[107]);  // "INT"
    DrawString(Device, 93, 241, 0xFFA3A3A3, *GL2GameData.SysString[108]);  // "WIT"
    DrawString(Device, 163, 241, 0xFFA3A3A3, *GL2GameData.SysString[109]); // "MEN"
    DrawString(Device, 17, 259, 0xFFFFFFFF, *GL2GameData.SysString[100]);  // "Social"
    DrawString(Device, 17, 278, 0xFFFFFFFF, *GL2GameData.SysString[101]);  // "Karma"
    DrawString(Device, 122, 278, 0xFFA3A3A3, *GL2GameData.SysString[102]); // "PvP"

    Player = GConsole->GetMyUser();
    if (Player)
    {
        INT HPW = 0;
        INT MPW = 0;
        INT WeightW = 0;
        INT EXPW = 0;
        if (Player->MaxHP)
            HPW = (INT)(Player->HP * 84.0 / Player->MaxHP);
        if (Player->MaxMP)
            MPW = (INT)(Player->MP * 84.0 / Player->MaxMP);
        if (Player->MaxLoad)
            WeightW = (INT)(Player->Load * 84.0 / Player->MaxLoad);
        if (GExpTable[Player->Level + 1])
            EXPW = (INT)((DOUBLE)(Player->Exp - GExpTable[Player->Level]) * 189.0 /
                         (GExpTable[Player->Level + 1] - GExpTable[Player->Level]));

        if (HPW > 0 && HPW <= 84)
            Device->DrawTexture(34, 79, HPW, 10, 0, 0, HPW, 10, HPTex);
        else if (HPW > 84)
            Device->DrawTexture(34, 79, 84, 10, 0, 0, 84, 10, HPTex);
        if (MPW > 0 && MPW <= 84)
            Device->DrawTexture(139, 79, MPW, 10, 0, 0, MPW, 10, MPTex);
        else if (MPW > 84)
            Device->DrawTexture(139, 79, 84, 10, 0, 0, 84, 10, MPTex);
        if (EXPW > 0 && EXPW <= 189)
            Device->DrawTexture(34, 65, EXPW, 10, 0, 0, EXPW, 10, EXPTex);
        else if (EXPW > 189)
            Device->DrawTexture(34, 65, 189, 10, 0, 0, 189, 10, EXPTex);

        FLOAT WeightPct = 0.f;
        if (WeightW > 0)
        {
            // the first test uses the unrounded quotient (fst, then fcomp); the others reload the
            // stored float, as the stock code does
            DOUBLE Pct = (DOUBLE)Player->Load / Player->MaxLoad * 100.0;
            WeightPct = (FLOAT)Pct;
            if (Pct <= 50.0)
                Device->DrawTexture(139, 93, WeightW, 10, 0, 0, WeightW, 10, WeightTex[0]);
            else if (WeightPct <= 66.66)
                Device->DrawTexture(139, 93, WeightW, 10, 0, 0, WeightW, 10, WeightTex[1]);
            else if (WeightPct > 66.66 && WeightPct <= 80.f)
                Device->DrawTexture(139, 93, WeightW, 10, 0, 0, WeightW, 10, WeightTex[2]);
            else if (WeightPct > 80.f)
                Device->DrawTexture(139, 93, WeightW, 10, 0, 0, WeightW, 10, WeightTex[3]);
        }

        TCHAR Buf[64];
        SIZE Size;
        INT LevelExp = GExpTable[Player->Level];
        swprintf(Buf, ARRAY_COUNT(Buf), TEXT("%.2f%s"),
                 (DOUBLE)(Player->Exp - LevelExp) * 100.0 / (GExpTable[Player->Level + 1] - LevelExp),
                 TEXT("%"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 128 - Size.cx / 2, 66, 0xFFB4B4B4, Buf);

        // "now / max" around a slash centered at 76 (HP) and 181 (MP)
        appSprintf(Buf, TEXT("%s"), TEXT("/"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 76 - Size.cx / 2, 80, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%d"), Player->HP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 66 - Size.cx, 80, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%d"), Player->MaxHP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 82, 80, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%s"), TEXT("/"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 181 - Size.cx / 2, 80, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%d"), Player->MP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 171 - Size.cx, 80, 0xFFFFFFFF, Buf);
        appSprintf(Buf, TEXT("%d"), Player->MaxMP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 187, 80, 0xFFFFFFFF, Buf);

        DrawNumber(Device, Buf, 76, 93, Player->SP);
        appSprintf(Buf, TEXT("%.2f%s"), WeightPct, TEXT("%"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        Device->DrawSpecialDigit(1, 181 - Size.cx / 2, 94, 0xFFFFFFFF, Buf);

        // the name is white without karma; with it, green and blue go down by the karma (at
        // least by 100 + karma / 16, at most by 255: red)
        INT Red = Player->Karma;
        if (Red > 255)
            Red = 255;
        if (Red > 0)
        {
            INT MinRed = Red / 16 + 100;
            if (Red < MinRed)
                Red = MinRed;
            else if (Red >= 255)
                Red = 255;
        }
        DWORD NameColor = 0xFFFF0000 | ((255 - Red) << 8) | (255 - Red);

        // the title first, when there is one; the name between two end pieces
        TCHAR Name[64];
        TCHAR Title[64];
        SIZE NameSize;
        SIZE TitleSize;
        if (Player->GetNickName() && *Player->GetNickName())
        {
            appSprintf(Title, TEXT("%s"), Player->GetNickName());
            NCGDevice::GetTextExtent(&TitleSize, Title, 0, 0, 0, 0, 0);
            appSprintf(Name, TEXT("%s"), Player->GetName());
            NCGDevice::GetTextExtent(&NameSize, Name, 0, 0, 0, 0, 0);
            if (TitleSize.cx + NameSize.cx > 220)
            {
                if (TitleSize.cx > 109)
                {
                    Title[9] = '.';
                    Title[10] = '.';
                    Title[11] = 0;
                    NCGDevice::GetTextExtent(&TitleSize, Title, 0, 0, 0, 0, 0);
                }
                if (NameSize.cx > 109)
                {
                    Name[9] = '.';
                    Name[10] = '.';
                    Name[11] = 0;
                    NCGDevice::GetTextExtent(&NameSize, Name, 0, 0, 0, 0, 0);
                }
            }
            INT X = 120 - (TitleSize.cx + NameSize.cx) / 2;
            DrawString(Device, X, 7, 0xFFA2F9EC, Title);
            DrawString(Device, X + TitleSize.cx + 6, 7, NameColor, Name);
            Device->DrawTexture(X - 6, 5, 6, 16, 0, 0, 6, 16, NameLeftTex);
            Device->DrawTexture(X + NameSize.cx + TitleSize.cx + 6, 5, 6, 16, 0, 0, 6, 16,
                                NameRightTex);
        }
        else
        {
            appSprintf(Name, TEXT("%s"), Player->GetName());
            NCGDevice::GetTextExtent(&NameSize, Name, 0, 0, 0, 0, 0);
            INT X = 120 - NameSize.cx / 2;
            DrawString(Device, X, 7, NameColor, Name);
            Device->DrawTexture(X - 6, 5, 6, 16, 0, 0, 6, 16, NameLeftTex);
            Device->DrawTexture(X + NameSize.cx, 5, 6, 16, 0, 0, 6, 16, NameRightTex);
        }

        // the clan: crest and name, else SysString 431 ("No Clan"). The crest is the texture
        // the network handler keeps with the clan (the GM window looks it up by name instead)
        PledgeInfo* Pledge = GConsole->GetPledgeInfo(Player->PledgeID);
        if (Pledge)
        {
            if (Pledge->CrestTex)
                Device->DrawTexture(73, 31, 16, 12, 0, 4, 16, 12, Pledge->CrestTex);
            const TCHAR* PledgeName = Pledge->Name;
            if (PledgeName && *PledgeName)
            {
                NCGDevice::GetTextExtent(&Size, PledgeName, 0, 0, 0, 0, 0);
                DrawString(Device, 93, 31, 0xFFB09B79, PledgeName);
            }
        }
        else
            DrawString(Device, 73, 31, 0xFFFFFFFF, *GL2GameData.SysString[431]);

        appSprintf(Buf, TEXT("%d %s"), Player->Level,
                   GConsole->GetClassNameString(Player->ClassID));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        DrawString(Device, 35, 46, 0xFFB09B79, Buf);

        DrawNumber(Device, Buf, 89, 128, Player->PAtk);
        DrawNumber(Device, Buf, 89, 144, Player->PDef);
        DrawNumber(Device, Buf, 89, 159, Player->Accuracy);
        DrawNumber(Device, Buf, 89, 174, Player->Critical);
        DrawNumber(Device, Buf, 89, 189, Player->PAtkSpeed);
        DrawNumber(Device, Buf, 206, 128, Player->MAtk);
        DrawNumber(Device, Buf, 206, 144, Player->MDef);
        DrawNumber(Device, Buf, 206, 159, Player->Evasion);
        // the speed as the player moves: running or walking (anything else: no speed), times
        // the pawn's speed factor
        APawn* Pawn = Player->Pawn;
        BYTE MoveMode = Pawn->Controller->MoveMode;
        if (MoveMode == 1)
            DrawNumber(Device, Buf, 206, 174, (INT)(Player->RunSpeed * Pawn->SpeedRate));
        else if (MoveMode == 0)
            DrawNumber(Device, Buf, 206, 174, (INT)(Player->WalkSpeed * Pawn->SpeedRate));
        DrawNumber(Device, Buf, 206, 189, Player->CastSpeed);
        DrawNumber(Device, Buf, 65, 224, Player->Str);
        DrawNumber(Device, Buf, 138, 224, Player->Dex);
        DrawNumber(Device, Buf, 212, 224, Player->Con);
        DrawNumber(Device, Buf, 65, 241, Player->Int);
        DrawNumber(Device, Buf, 138, 241, Player->Wit);
        DrawNumber(Device, Buf, 212, 241, Player->Men);

        // from 999999 on the karma gets a "+" (the value itself is still printed)
        if (Player->Karma >= 999999)
            appSprintf(Buf, TEXT("%d+"), Player->Karma);
        else
            appSprintf(Buf, TEXT("%d"), Player->Karma);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        DrawString(Device, 91 - Size.cx / 2, 278, 0xFFB09B79, Buf);
        appSprintf(Buf, TEXT("%d / %d"), Player->PvPCount, Player->PKCount);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        DrawString(Device, 187 - Size.cx / 2, 278, 0xFFB09B79, Buf);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100b1c00, NCDetailStatusWnd, OnPaint)

NW_CHECK_VTABLE(NCDetailStatusWnd, 0x100e3b90, 62)

/*-----------------------------------------------------------------------------
    NCGMDetailStatusWnd.
-----------------------------------------------------------------------------*/

// a number of the GM window: black one pixel lower right, then white
static FORCEINLINE void DrawShadowText(NCGDevice* Device, INT X, INT Y, const TCHAR* Text)
{
    DrawString(Device, X + 1, Y + 1, 0xFF000000, Text);
    DrawString(Device, X, Y, 0xFFFFFFFF, Text);
}

// the slash between two numbers of the GM window, in the large digits, with the same shadow
static FORCEINLINE void DrawShadowSlash(NCGDevice* Device, TCHAR* Buf, INT CenterX, INT Y)
{
    SIZE Size;
    appSprintf(Buf, TEXT("%s"), TEXT("/"));
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawSpecialDigit(1, CenterX + 1 - Size.cx / 2, Y + 1, 0xFF000000, Buf);
    Device->DrawSpecialDigit(1, CenterX - Size.cx / 2, Y, 0xFFFFFFFF, Buf);
}

// 0x1005cdb0 - the User is built by Engine's constructor
NCGMDetailStatusWnd::NCGMDetailStatusWnd()
{
}
NW_HOOK_CTOR(0x1005cdb0, NCGMDetailStatusWnd)

// 0x1005ce30
NW_HOOK_DTOR(0x1005ce30, NCGMDetailStatusWnd)

// stock body shared with NCButton's (0x100051f0)
INT NCGMDetailStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCDetailStatusWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x1005ce80 (vslot 35) - the character tab's textures, and a title bar
INT NCGMDetailStatusWnd::OnCreate()
{
    NCDetailStatusWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_GMDETAIL_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x1005ce80, NCGMDetailStatusWnd, OnCreate)

// 0x1005cf30 (vslot 50) - the character tab's contents from Target, rearranged: HP, MP and
// weight bars 84 pixels wide at most but not clamped, the exp bar 119 (of the 189 the texture
// has room for) and its percentage right-aligned in the common font, HP / MP with a shadow.
// The speed is always the running one, times the User's own factor, a pixel below its label.
INT NCGMDetailStatusWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 300, 0, 0, 240, 300, BackTex);

    DrawString(Device, 17, 55, 0xFFA3A3A3, *GL2GameData.SysString[430]);   // "Clan"
    DrawString(Device, 17, 70, 0xFFA3A3A3, *GL2GameData.SysString[88]);    // "Lv"
    DrawString(Device, 17, 134, 0xFFFFFFFF, *GL2GameData.SysString[93]);   // "Combat"
    DrawString(Device, 17, 149, 0xFFA3A3A3, *GL2GameData.SysString[94]);   // "P.Atk."
    DrawString(Device, 17, 163, 0xFFA3A3A3, *GL2GameData.SysString[95]);   // "P.Def."
    DrawString(Device, 17, 176, 0xFFA3A3A3, *GL2GameData.SysString[96]);   // "Accuracy"
    DrawString(Device, 17, 189, 0xFFA3A3A3, *GL2GameData.SysString[113]);  // "Critical"
    DrawString(Device, 17, 202, 0xFFA3A3A3, *GL2GameData.SysString[111]);  // "Atk. Spd."
    DrawString(Device, 17, 227, 0xFFFFFFFF, *GL2GameData.SysString[103]);  // "Basic"
    DrawString(Device, 122, 149, 0xFFA3A3A3, *GL2GameData.SysString[98]);  // "M.Atk."
    DrawString(Device, 122, 163, 0xFFA3A3A3, *GL2GameData.SysString[99]);  // "M.Def."
    DrawString(Device, 122, 176, 0xFFA3A3A3, *GL2GameData.SysString[97]);  // "Evasion"
    DrawString(Device, 122, 189, 0xFFA3A3A3, *GL2GameData.SysString[432]); // "Speed"
    DrawString(Device, 122, 202, 0xFFA3A3A3, *GL2GameData.SysString[112]); // "Casting Spd."
    DrawString(Device, 17, 246, 0xFFA3A3A3, *GL2GameData.SysString[104]);  // "STR"
    DrawString(Device, 93, 246, 0xFFA3A3A3, *GL2GameData.SysString[105]);  // "DEX"
    DrawString(Device, 163, 246, 0xFFA3A3A3, *GL2GameData.SysString[106]); // "CON"
    DrawString(Device, 17, 259, 0xFFA3A3A3, *GL2GameData.SysString[107]);  // "INT"
    DrawString(Device, 93, 259, 0xFFA3A3A3, *GL2GameData.SysString[108]);  // "WIT"
    DrawString(Device, 163, 259, 0xFFA3A3A3, *GL2GameData.SysString[109]); // "MEN"
    DrawString(Device, 17, 279, 0xFFFFFFFF, *GL2GameData.SysString[100]);  // "Social"
    DrawString(Device, 17, 298, 0xFFFFFFFF, *GL2GameData.SysString[101]);  // "Karma"
    DrawString(Device, 122, 298, 0xFFA3A3A3, *GL2GameData.SysString[102]); // "PvP"

    INT HPW = 0;
    INT MPW = 0;
    INT WeightW = 0;
    INT EXPW = 0;
    if (Target.MaxHP)
        HPW = Target.HP * 84 / Target.MaxHP;
    if (Target.MaxMP)
        MPW = Target.MP * 84 / Target.MaxMP;
    if (Target.MaxLoad)
        WeightW = Target.Load * 84 / Target.MaxLoad;
    if (GGMExpTable[Target.Level])
        EXPW = (Target.Exp - GGMExpTable[Target.Level - 1]) * 119 /
               (GGMExpTable[Target.Level] - GGMExpTable[Target.Level - 1]);
    if (HPW > 0)
        Device->DrawTexture(34, 100, HPW, 10, 0, 0, HPW, 10, HPTex);
    if (MPW > 0)
        Device->DrawTexture(139, 100, MPW, 10, 0, 0, MPW, 10, MPTex);
    if (EXPW > 0)
        Device->DrawTexture(34, 86, EXPW, 10, 0, 0, EXPW, 10, EXPTex);

    FLOAT WeightPct = 0.f;
    if (WeightW > 0)
    {
        // the first test uses the unrounded quotient (fst, then fcomp); the others reload the
        // stored float, as the stock code does
        DOUBLE Pct = (DOUBLE)Target.Load / Target.MaxLoad * 100.0;
        WeightPct = (FLOAT)Pct;
        if (Pct <= 50.0)
            Device->DrawTexture(139, 114, WeightW, 10, 0, 0, WeightW, 10, WeightTex[0]);
        else if (WeightPct <= 66.66)
            Device->DrawTexture(139, 114, WeightW, 10, 0, 0, WeightW, 10, WeightTex[1]);
        else if (WeightPct > 66.66 && WeightPct <= 80.f)
            Device->DrawTexture(139, 114, WeightW, 10, 0, 0, WeightW, 10, WeightTex[2]);
        else if (WeightPct > 80.f)
            Device->DrawTexture(139, 114, WeightW, 10, 0, 0, WeightW, 10, WeightTex[3]);
    }

    TCHAR Buf[64];
    TCHAR CrestName[128];
    SIZE Size;
    INT LevelExp = GGMExpTable[Target.Level - 1];
    swprintf(Buf, ARRAY_COUNT(Buf), TEXT("%.2f%s"),
             (DOUBLE)(Target.Exp - LevelExp) * 100.0 / (GGMExpTable[Target.Level] - LevelExp),
             TEXT("%"));
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    DrawString(Device, 220 - Size.cx, 74, 0xFFFFFFFF, Buf);

    // "now / max" around a slash centered at 74 (HP) and 179 (MP)
    DrawShadowSlash(Device, Buf, 74, 100);
    appSprintf(Buf, TEXT("%d"), Target.HP);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    DrawShadowText(Device, 71 - Size.cx, 100, Buf);
    appSprintf(Buf, TEXT("%d"), Target.MaxHP);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    DrawShadowText(Device, 80, 100, Buf);
    DrawShadowSlash(Device, Buf, 179, 100);
    appSprintf(Buf, TEXT("%d"), Target.MP);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    DrawShadowText(Device, 176 - Size.cx, 100, Buf);
    appSprintf(Buf, TEXT("%d"), Target.MaxMP);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    DrawShadowText(Device, 185, 100, Buf);

    DrawNumber(Device, Buf, 76, 114, Target.SP);
    appSprintf(Buf, TEXT("%.2f%s"), WeightPct, TEXT("%"));
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawSpecialDigit(1, 181 - Size.cx / 2, 115, 0xFFFFFFFF, Buf);

    // "title name" (or the name) in white between the two end pieces
    User* U = &Target;
    if (U->GetNickName() && *U->GetNickName())
        appSprintf(Buf, TEXT("%s %s"), U->GetNickName(), U->GetName());
    else
        appSprintf(Buf, TEXT("%s"), U->GetName());
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    DrawString(Device, 120 - Size.cx / 2, 31, 0xFFFFFFFF, Buf);
    Device->DrawTexture(114 - Size.cx / 2, 29, 6, 16, 0, 0, 6, 16, NameLeftTex);
    Device->DrawTexture(Size.cx - Size.cx / 2 + 120, 29, 6, 16, 0, 0, 6, 16, NameRightTex);

    // the clan: the crest by its texture name when the network handler has imported it,
    // kept in CrestTex; else SysString 431 ("No Clan")
    PledgeInfo* Pledge = GConsole->GetPledgeInfo(Target.PledgeID);
    if (Pledge)
    {
        if (GConsole->Engine->NetHandler->IsImportedPledgeCrestID(Pledge->CrestID))
        {
            appSprintf(CrestName, TEXT("crest.%d_%d"), GConsole->GetServerID(), Pledge->CrestID);
            CrestTex = FindTexture(CrestName);
            if (CrestTex)
                Device->DrawTexture(73, 55, 16, 12, 0, 4, 16, 12, CrestTex);
        }
        const TCHAR* PledgeName = Pledge->Name;
        if (PledgeName && *PledgeName)
        {
            NCGDevice::GetTextExtent(&Size, PledgeName, 0, 0, 0, 0, 0);
            DrawString(Device, 93, 55, 0xFFB09B79, PledgeName);
        }
    }
    else
        DrawString(Device, 73, 55, 0xFFFFFFFF, *GL2GameData.SysString[431]);

    appSprintf(Buf, TEXT("%d %s"), Target.Level, GConsole->GetClassNameString(Target.ClassID));
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    DrawString(Device, 35, 70, 0xFFB09B79, Buf);

    DrawNumber(Device, Buf, 89, 149, Target.PAtk);
    DrawNumber(Device, Buf, 89, 163, Target.PDef);
    DrawNumber(Device, Buf, 89, 176, Target.Accuracy);
    DrawNumber(Device, Buf, 89, 189, Target.Critical);
    DrawNumber(Device, Buf, 89, 202, Target.PAtkSpeed);
    DrawNumber(Device, Buf, 206, 149, Target.MAtk);
    DrawNumber(Device, Buf, 206, 163, Target.MDef);
    DrawNumber(Device, Buf, 206, 176, Target.Evasion);
    DrawNumber(Device, Buf, 206, 190, (INT)(Target.RunSpeed * Target.MoveSpeedRate));
    DrawNumber(Device, Buf, 206, 202, Target.CastSpeed);
    DrawNumber(Device, Buf, 65, 246, Target.Str);
    DrawNumber(Device, Buf, 138, 246, Target.Dex);
    DrawNumber(Device, Buf, 212, 246, Target.Con);
    DrawNumber(Device, Buf, 65, 259, Target.Int);
    DrawNumber(Device, Buf, 138, 259, Target.Wit);
    DrawNumber(Device, Buf, 212, 259, Target.Men);

    if (Target.Karma >= 999999)
        appSprintf(Buf, TEXT("%d+"), Target.Karma);
    else
        appSprintf(Buf, TEXT("%d"), Target.Karma);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    DrawString(Device, 91 - Size.cx / 2, 298, 0xFFB09B79, Buf);
    appSprintf(Buf, TEXT("%d / %d"), Target.PvPCount, Target.PKCount);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    DrawString(Device, 186 - Size.cx / 2, 298, 0xFFB09B79, Buf);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x1005cf30, NCGMDetailStatusWnd, OnPaint)

// 0x1005be50 - the server's answer about the player (NConsoleWnd's virtual 202)
void NCGMDetailStatusWnd::SetTarget(User InTarget)
{
    Target = InTarget;
}
NW_HOOK_METHOD(0x1005be50, NCGMDetailStatusWnd, SetTarget)

NW_CHECK_VTABLE(NCGMDetailStatusWnd, 0x100e4200, 62)
