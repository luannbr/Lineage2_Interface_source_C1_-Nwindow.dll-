#pragma once
// NItemInfo - what the console and the item windows pass around about one icon of an item,
// skill or action (name ours; 0xC9C bytes, built on the stack). The inventory fills most of
// it (NConsoleWnd::AddInventoryItem...); the action window only the names, the icon and two
// numbers. The item windows keep a copy of it in each NItemEntry (NCItemWnd.h), where the
// fields' use shows. Field meanings beyond those are not known yet.

#include "nwindow/NWindow.h"

struct NItemInfo
{
    TCHAR Name[128];           // +0x000
    TCHAR IconName[128];       // +0x100 texture of the icon (the windows look it up)
    TCHAR Unknown200[128];     // +0x200
    TCHAR Unknown300[128];     // +0x300
    TCHAR Description[1024];   // +0x400
    UTexture* Icon;            // +0xC00
    INT UnknownC04;            // +0xC04
    INT UnknownC08;            // +0xC08
    INT Param;                 // +0xC0C (-1) actions: FL2ActionData::Param; items: the type
                               //   (0..3: the enchant shows, NCQuestItemWnd)
    INT ObjectID;              // +0xC10 items: the server's id of this one
    INT ID;                    // +0xC14 actions: FL2ActionData::ActionID
    INT Count;                 // +0xC18
    INT UnknownC1C;            // +0xC1C
    INT UnknownC20;            // +0xC20 (-1)
    INT BodyPart;              // +0xC24 items: slot mask (NConsoleWnd::GetItemSlotText)
    INT UnknownC28;            // +0xC28
    INT UnknownC2C;            // +0xC2C (-1)
    INT UnknownC30;            // +0xC30 (-1)
    INT UnknownC34[13];        // +0xC34
    INT SubType;               // +0xC68 (-1) items: (NConsoleWnd::GetItemSlotText)
    INT UnknownC6C;            // +0xC6C
    INT UnknownC70;            // +0xC70
    INT Enchant;               // +0xC74
    INT UnknownC78;            // +0xC78
    INT UnknownC7C;            // +0xC7C
    INT UnknownC80;            // +0xC80
    INT UnknownC84;            // +0xC84
    UBOOL bStackable;          // +0xC88 the count shows next to the name
    INT UnknownC8C;            // +0xC8C
    INT UnknownC90;            // +0xC90 (-1)
    INT UnknownC94;            // +0xC94
    INT UnknownC98;            // +0xC98

    NItemInfo();                                                     // 0x100342c0
};
NW_ASSERT_OFFSET(NItemInfo, Description, 0x400);
NW_ASSERT_OFFSET(NItemInfo, Icon, 0xC00);
NW_ASSERT_OFFSET(NItemInfo, ID, 0xC14);
NW_ASSERT_OFFSET(NItemInfo, SubType, 0xC68);
NW_ASSERT_OFFSET(NItemInfo, Enchant, 0xC74);
NW_ASSERT_OFFSET(NItemInfo, bStackable, 0xC88);
NW_ASSERT_OFFSET(NItemInfo, UnknownC90, 0xC90);
NW_ASSERT_SIZE(NItemInfo, 0xC9C);
