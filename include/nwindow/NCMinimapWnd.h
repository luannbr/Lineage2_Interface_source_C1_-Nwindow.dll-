#pragma once
// NCMinimapWnd - the map window (UClass "CMinimapWnd", 0xE0): a 328 x 329 piece of the map
// texture (Aden or Elmore, named in FontInfo.ini) under a numbered 61-pixel grid, the player's
// position and the targets the console marks. Dragging the map scrolls it; the button in the
// corner scrolls back to the player.
//
// Desktop (NCConsole) +0xF4 is the map window. The console marks the same targets on the radar
// (NCRadarWnd) by calling AddTarget / RemoveTarget on it too (same array at +0x94).

#include "nwindow/NCWnd.h"

class NCButton;

// A position the console marks on the map and the radar (name ours; world coordinates).
struct FMinimapTarget
{
    INT X;
    INT Y;
    INT Z;
};
NW_ASSERT_SIZE(FMinimapTarget, 0xC);

// map ids the console sets (SetMapID); names ours, after the textures
enum
{
    MINIMAP_Aden = 0x681,
    MINIMAP_Elmore = 0x747,
};

// NWM_COMMAND id of the button that scrolls back to the player
enum { NID_MINIMAP_INITPOS = 0xB000F1 };

class NCMinimapWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMinimapWnd, NCWnd, 0, 0x1013bf78)

    TArray<FMinimapTarget> Targets;  // +0x94
    UTexture* BackTex;               // +0xA0 "L2UI.MinimapWnd.MinimapBack" (frame over the map)
    UTexture* AdenMapTex;            // +0xA4 FontInfo.ini "<country>_AdenMinimap"
    UTexture* ElmoreMapTex;          // +0xA8 FontInfo.ini "<country>_ElmoreMinimap"
    UTexture* MyPosTex;              // +0xAC "L2UI.MinimapWnd.MinimapPos"
    UTexture* TargetPosTex;          // +0xB0 "L2UI.MinimapWnd.TargetPos"
    NCButton* InitPosButton;         // +0xB4
    INT MapID;                       // +0xB8 MINIMAP_*
    UBOOL bDragging;                 // +0xBC
    INT MyX;                         // +0xC0 the player, relative to the map area (each paint)
    INT MyY;                         // +0xC4
    INT ScrollX;                     // +0xC8 map pixel at the map area's corner
    INT ScrollY;                     // +0xCC
    INT DragX;                       // +0xD0 last mouse position, relative to the map area
    INT DragY;                       // +0xD4
    INT UnknownD8[2];                // +0xD8

    NCMinimapWnd();                                                  // 0x10079370
    // ~NCMinimapWnd                                                 // 0x10079410

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100781e0
    virtual void KillFocus();                                        // 29 0x100784a0
    virtual INT OnCreate();                                          // 35 0x10078660
    virtual INT OnDestroy();                                         // 36 0x10078910
    virtual void ShowWindow();                                       // 42 0x10078950 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10078c30
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x100782d0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x10078250
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x10078470
    virtual INT OnInitPos();                                         // 62 0x100792d0 (name ours)

    // names ours; the map id arguments are not read (MapID is)
    void WorldToMap(INT WX, INT WY, INT* OutX, INT* OutY, INT InMapID); // 0x100784b0
    void SetMapID(INT InMapID);                                      // 0x10078650
    void GetMyMapPos(INT* OutX, INT* OutY, INT InMapID);             // 0x100789a0
    void CenterOnPlayer();                                           // 0x10079130
    void AddTarget(INT X, INT Y, INT Z);                             // 0x10079230
    void RemoveTarget(INT X, INT Y, INT Z);                          // 0x10079270
};
NW_ASSERT_OFFSET(NCMinimapWnd, Targets, 0x94);
NW_ASSERT_OFFSET(NCMinimapWnd, InitPosButton, 0xB4);
NW_ASSERT_OFFSET(NCMinimapWnd, MapID, 0xB8);
NW_ASSERT_OFFSET(NCMinimapWnd, ScrollX, 0xC8);
NW_ASSERT_OFFSET(NCMinimapWnd, DragY, 0xD4);
NW_ASSERT_SIZE(NCMinimapWnd, 0xE0);
