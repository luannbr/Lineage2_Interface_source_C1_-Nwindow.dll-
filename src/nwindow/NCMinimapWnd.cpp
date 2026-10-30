/*=============================================================================
    NCMinimapWnd.cpp - the map window. Stock: 0x100781e0 .. 0x100784ae,
    0x100784b0 .. 0x1007890e, 0x100789a0 .. 0x10078bab, 0x10078c30 .. 0x100792da,
    0x10079370 .. 0x100793d1, 0x10079410 .. 0x1007946f (0x10078950 is the ShowWindow body
    NCSystemMenuWnd keeps; 0x10078bb0 and 0x100792e0 are TArray<FMinimapTarget> templates).
=============================================================================*/
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <stdlib.h>

// title bar id
enum { NID_MINIMAP_FRAME = 0xB000F0 };

// 0x10079370 - the target mark texture and the button are left as allocated
NCMinimapWnd::NCMinimapWnd()
{
    BackTex = NULL;
    AdenMapTex = NULL;
    ElmoreMapTex = NULL;
    MyPosTex = NULL;
    MyX = 0;
    MyY = 0;
    ScrollX = 0;
    ScrollY = 0;
    MapID = 0;
    bDragging = 0;
}
NW_HOOK_CTOR(0x10079370, NCMinimapWnd)

// 0x10079410
NW_HOOK_DTOR(0x10079410, NCMinimapWnd)

// 0x100781e0 (vslot 24)
INT NCMinimapWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == NID_MINIMAP_INITPOS)
        return OnInitPos() != 0;
    return 0;
}
NW_HOOK_METHOD(0x100781e0, NCMinimapWnd, WindowProc)

// 0x100784a0 (vslot 29) - a drag in progress lets the cursor go
void NCMinimapWnd::KillFocus()
{
    UnlockCursor();
    NCWnd::KillFocus();
}
NW_HOOK_METHOD(0x100784a0, NCMinimapWnd, KillFocus)

// 0x10078660 (vslot 35) - a decorated title bar; the map textures FontInfo.ini names for the
// client's country
INT NCMinimapWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_MINIMAP_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 334, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    BackTex = FindTexture(TEXT("L2UI.MinimapWnd.MinimapBack"));
    MyPosTex = FindTexture(TEXT("L2UI.MinimapWnd.MinimapPos"));
    TargetPosTex = FindTexture(TEXT("L2UI.MinimapWnd.TargetPos"));

    FString MapName;
    TCHAR Key[256];
    appSprintf(Key, TEXT("%s_AdenMinimap"), GCountryName);
    if (!GConfig->GetString(GCountryName, Key, MapName, TEXT("FontInfo.ini")))
        MapName = TEXT("L2font.aden_map-k");
    AdenMapTex = FindTexture(*MapName);
    appSprintf(Key, TEXT("%s_ElmoreMinimap"), GCountryName);
    if (!GConfig->GetString(GCountryName, Key, MapName, TEXT("FontInfo.ini")))
        MapName = TEXT("L2font.elmore_map-k");
    ElmoreMapTex = FindTexture(*MapName);

    InitPosButton = new NCButton(TEXT("L2UI.MinimapWnd.InitPosButton"),
                                 TEXT("L2UI.MinimapWnd.InitPosButton"), 0);
    InitPosButton->ID = NID_MINIMAP_INITPOS;
    InitPosButton->SetWindowRect(4, 22, 26, 26);
    CreateChild(InitPosButton, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10078660, NCMinimapWnd, OnCreate)

// 0x10078910 (vslot 36)
INT NCMinimapWnd::OnDestroy()
{
    if (InitPosButton)
    {
        RemoveChildWindow(InitPosButton->Entry);
        delete InitPosButton;
        InitPosButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10078910, NCMinimapWnd, OnDestroy)

// stock body shared with NCSystemMenuWnd's (0x10078950): shown again from its minimized icon,
// the window takes the icon's place and the icon goes away
void NCMinimapWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    if (Icon)
    {
        SetWindowRect(Icon->ScreenX, Icon->ScreenY, Width, Height);
        Icon->SetWindowRect(0, 0, 0, 0);
        Icon->HideWindow();
    }
}

// 0x10078c30 (vslot 50) - the map, then the grid (numbered from the map's origin), the frame,
// the player and the targets. The stock code sets the grid size only for the two known maps
// (for any other id it divides by whatever the stack held); 61 here.
INT NCMinimapWnd::OnPaint(NCGDevice* Device)
{
    TCHAR Buf[256];
    SIZE Size;
    Device->SetClip(ScreenX, ScreenY, Width, Height - 3);
    Device->BeginPaint(ScreenX, ScreenY);
    INT OffX = 0;
    INT OffY = 0;
    if (MapID == MINIMAP_Aden && AdenMapTex)
    {
        OffX = 0x30000;
        OffY = 0x38000;
        Device->DrawTexture(3, 21, 328, 329, ScrollX, ScrollY, 328, 329, AdenMapTex);
    }
    else if (MapID == MINIMAP_Elmore && ElmoreMapTex)
    {
        OffX = 0x40000;
        OffY = 0x8000;
        Device->DrawTexture(3, 21, 328, 329, ScrollX, ScrollY, 328, 329, ElmoreMapTex);
    }
    // both offsets take the map's horizontal scale
    if (MapID == MINIMAP_Aden)
        OffX = OffX * 1642 / 327680;
    else if (MapID == MINIMAP_Elmore)
        OffX = OffX * 1312 / 262144;
    if (MapID == MINIMAP_Aden)
        OffY = OffY * 1642 / 327680;
    else if (MapID == MINIMAP_Elmore)
        OffY = OffY * 1312 / 262144;
    INT GridW = 61;
    INT GridH = 61;

    INT Label = (ScrollX + OffX) / GridW + 20;
    INT X = GridW - (ScrollX + OffX) % GridW;
    INT Row = (OffY + ScrollY) / GridH;
    INT Y = GridH - (OffY + ScrollY) % GridH;
    for (INT k = 0; k <= 328 / GridW; k++, X += GridW, Label++)
    {
        if (Label >= 0)
        {
            appSprintf(Buf, TEXT("%d"), Label);
            NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
            if (X > 15)
                Device->DrawText(X - Size.cx / 2 + 4, 24, 0xFFFFFFFF, Buf, 0, 0, 0, (L2FontType)1, 0,
                                 0, 0, 0, 0, 0);
        }
        Device->DrawLine(X + 4, Size.cy + 22, X + 4, 351, 0x40000000, 1);
    }
    Label = Row + 21;
    for (INT k = 0; k <= 329 / GridH; k++, Y += GridH, Label++)
    {
        if (Label >= 1)
        {
            appSprintf(Buf, TEXT("%d"), Label);
            NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
            if (Y > 20)
                Device->DrawText(4, Y - Size.cy / 2 + 22, 0xFFFFFFFF, Buf, 0, 0, 0, (L2FontType)1, 0,
                                 0, 0, 0, 0, 0);
        }
        Device->DrawLine(Size.cx + 4, Y + 22, 332, Y + 22, 0x40000000, 1);
    }

    if (BackTex)
        Device->DrawTexture(0, 21, 334, 333, 0, 0, 334, 333, BackTex);
    GetMyMapPos(&MyX, &MyY, MapID);
    MyX -= ScrollX;
    MyY -= ScrollY;
    if (MyPosTex && MyX >= 12 && MyX <= Width - 20 && MyY >= 0 && MyY < Height - 37)
        Device->DrawMinimapLoc(MyX + 3, MyY + 21, MyPosTex);
    for (INT i = 0; i < Targets.Num(); i++)
    {
        FMinimapTarget T = Targets(i);
        INT TX, TY;
        WorldToMap(T.X, T.Y, &TX, &TY, MapID);
        TX -= ScrollX;
        TY -= ScrollY;
        if (TargetPosTex && TX >= 0 && TX <= Width - 20 && TY >= 0 && TY < Height - 37)
            Device->DrawMinimapLoc(TX + 3, TY + 21, TargetPosTex);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10078c30, NCMinimapWnd, OnPaint)

// 0x100782d0 (vslot 55) - dragging scrolls the map within its limits
INT NCMinimapWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    if (IsFocused())
    {
        INT X = (SWORD)LOWORD(lParam) - ScreenX - 3;
        INT Y = (SWORD)HIWORD(lParam) - ScreenY - 21;
        if (X <= 3)
            X = 3;
        if (Y <= 21)
            Y = 21;
        if (bDragging && X >= 4 && X <= Width && Y >= 22 && Y <= Height)
        {
            INT DX = DragX - X;
            INT DY = DragY - Y;
            if (MapID == MINIMAP_Aden)
            {
                if (DX > 0)
                {
                    ScrollX += DX;
                    if (ScrollX > 1314)
                        ScrollX = 1314;
                }
                else
                {
                    ScrollX -= abs(DX);
                    if (ScrollX < 0)
                        ScrollX = 0;
                }
                if (DY > 0)
                {
                    ScrollY += DY;
                    if (ScrollY > 1147)
                        ScrollY = 1147;
                }
                else
                {
                    ScrollY -= abs(DY);
                    if (ScrollY < 0)
                        ScrollY = 0;
                }
            }
            else if (MapID == MINIMAP_Elmore)
            {
                if (DX > 0)
                {
                    ScrollX += DX;
                    if (ScrollX > 984)
                        ScrollX = 984;
                }
                else
                {
                    ScrollX -= abs(DX);
                    if (ScrollX < 0)
                        ScrollX = 0;
                }
                if (DY > 0)
                {
                    ScrollY += DY;
                    if (ScrollY > 655)
                        ScrollY = 655;
                }
                else
                {
                    ScrollY -= abs(DY);
                    if (ScrollY < 0)
                        ScrollY = 0;
                }
            }
        }
        DragX = X;
        DragY = Y;
    }
    return 1;
}
NW_HOOK_METHOD(0x100782d0, NCMinimapWnd, OnMouseMove)

// 0x10078250 (vslot 56) - a drag starts on the map area
INT NCMinimapWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT X = (SWORD)LOWORD(lParam) - ScreenX - 3;
    INT Y = (SWORD)HIWORD(lParam) - ScreenY - 21;
    if (X <= 3)
        X = 3;
    if (Y <= 21)
        Y = 21;
    if (X >= 4 && X <= Width && Y >= 22 && Y <= Height)
    {
        DragX = X;
        DragY = Y;
        bDragging = 1;
        SetCursor(3, NULL);
        GConsole->LockCursor();
    }
    return 1;
}
NW_HOOK_METHOD(0x10078250, NCMinimapWnd, OnLButtonDown)

// 0x10078470 (vslot 57)
INT NCMinimapWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    GConsole->UnlockCursor();
    SetCursor(0, NULL);
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10078470, NCMinimapWnd, OnLButtonUp)

// 0x100792d0 (vslot 62) - the button in the corner
INT NCMinimapWnd::OnInitPos()
{
    CenterOnPlayer();
    return 1;
}
NW_HOOK_METHOD(0x100792d0, NCMinimapWnd, OnInitPos)

// 0x100784b0 - world coordinates to map pixels, per map (the result goes through a float)
void NCMinimapWnd::WorldToMap(INT WX, INT WY, INT* OutX, INT* OutY, INT InMapID)
{
    if (MapID == MINIMAP_Aden)
    {
        if (WX < 0)
            *OutX = (INT)(FLOAT)((0x20000 - abs(WX)) * 1642 / 327680);
        else
            *OutX = (INT)(FLOAT)((WX + 0x20000) * 1642 / 327680);
        if (WY > 0)
            *OutY = (INT)(FLOAT)((WY + 0x8000) * 1476 / 294912);
        else
            *OutY = (INT)(FLOAT)((0x8000 - abs(WY)) * 1476 / 294912);
    }
    else if (MapID == MINIMAP_Elmore)
    {
        if (WX < 0)
            *OutX = (INT)(FLOAT)((0x10000 - abs(WX)) * 1312 / 262144);
        else
            *OutX = (INT)(FLOAT)((WX + 0x10000) * 1312 / 262144);
        *OutY = (INT)(FLOAT)((0x40000 - abs(WY)) * 984 / 196608);
    }
}
NW_HOOK_METHOD(0x100784b0, NCMinimapWnd, WorldToMap)

// 0x10078650 - from the console (vslot 217), which then centers the map on the player
void NCMinimapWnd::SetMapID(INT InMapID)
{
    MapID = InMapID;
}
NW_HOOK_METHOD(0x10078650, NCMinimapWnd, SetMapID)

// 0x100789a0 - the player's pawn on the map. For Aden the horizontal scale is not the one
// WorldToMap uses for the targets (1312 / 262144 against 1642 / 327680), as in the stock code.
void NCMinimapWnd::GetMyMapPos(INT* OutX, INT* OutY, INT InMapID)
{
    FVector Loc = GConsole->Engine->Client->Viewports(0)->Actor->Pawn->Location;
    INT WX = (INT)Loc.X;
    INT WY = (INT)Loc.Y;
    if (MapID == MINIMAP_Aden)
    {
        if (WX < 0)
            *OutX = (INT)(FLOAT)((0x20000 - abs(WX)) * 1312 / 262144);
        else
            *OutX = (INT)(FLOAT)((WX + 0x20000) * 1312 / 262144);
        if (WY > 0)
            *OutY = (INT)(FLOAT)((WY + 0x8000) * 1476 / 294912);
        else
            *OutY = (INT)(FLOAT)((0x8000 - abs(WY)) * 1476 / 294912);
    }
    else if (MapID == MINIMAP_Elmore)
    {
        if (WX < 0)
            *OutX = (INT)(FLOAT)((0x10000 - abs(WX)) * 1312 / 262144);
        else
            *OutX = (INT)(FLOAT)((WX + 0x10000) * 1312 / 262144);
        *OutY = (INT)(FLOAT)((0x40000 - abs(WY)) * 984 / 196608);
    }
}
NW_HOOK_METHOD(0x100789a0, NCMinimapWnd, GetMyMapPos)

// 0x10079130 - the player 164 pixels from the corner, within the map (the vertical limits
// are one pixel short of the matching positions, as in the stock code)
void NCMinimapWnd::CenterOnPlayer()
{
    INT CurMap = MapID;
    INT X, Y;
    GetMyMapPos(&X, &Y, CurMap);
    if (CurMap == MINIMAP_Aden)
    {
        if (X < 164)
            ScrollX = 0;
        else if (X > 1478)
            ScrollX = 1314;
        else
            ScrollX = X - 164;
        if (Y < 164)
            ScrollY = 0;
        else if (Y > 1312)
            ScrollY = 1147;
        else
            ScrollY = Y - 164;
    }
    else if (CurMap == MINIMAP_Elmore)
    {
        if (X < 164)
            ScrollX = 0;
        else if (X > 1148)
            ScrollX = 984;
        else
            ScrollX = X - 164;
        if (Y < 164)
            ScrollY = 0;
        else if (Y > 820)
            ScrollY = 655;
        else
            ScrollY = Y - 164;
    }
}
NW_HOOK_METHOD(0x10079130, NCMinimapWnd, CenterOnPlayer)

// 0x10079230 - from the console (vslot 72), on this window and on the radar
void NCMinimapWnd::AddTarget(INT X, INT Y, INT Z)
{
    INT i = Targets.Add();
    Targets(i).X = X;
    Targets(i).Y = Y;
    Targets(i).Z = Z;
}
NW_HOOK_METHOD(0x10079230, NCMinimapWnd, AddTarget)

// 0x10079270 - from the console (vslot 73); the entry after a removed one moves into its place
// and is not compared, as in the stock code
void NCMinimapWnd::RemoveTarget(INT X, INT Y, INT Z)
{
    for (INT i = 0; i < Targets.Num(); i++)
    {
        FMinimapTarget T = Targets(i);
        if (T.X == X && T.Y == Y && T.Z == Z)
            Targets.Remove(i, 1);
    }
}
NW_HOOK_METHOD(0x10079270, NCMinimapWnd, RemoveTarget)

NW_CHECK_VTABLE(NCMinimapWnd, 0x100e8328, 63)
