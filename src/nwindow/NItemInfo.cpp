/*=============================================================================
    NItemInfo.cpp - an item / skill / action icon as the item windows get it.
    Stock: 0x100342c0 .. 0x100343d0.
=============================================================================*/
#include "nwindow/NItemInfo.h"

// 0x100342c0 - the stores in the stock order
NItemInfo::NItemInfo()
{
    Name[0] = 0;
    IconName[0] = 0;
    Unknown200[0] = 0;
    Unknown300[0] = 0;
    Description[0] = 0;
    Icon = NULL;
    UnknownC04 = 0;
    UnknownC08 = 0;
    Param = -1;
    ObjectID = 0;
    ID = 0;
    Count = 0;
    UnknownC1C = 0;
    UnknownC20 = -1;
    BodyPart = 0;
    UnknownC28 = 0;
    UnknownC2C = -1;
    UnknownC30 = -1;
    for (INT i = 0; i < 13; i++)
        UnknownC34[i] = 0;
    SubType = -1;
    UnknownC6C = 0;
    UnknownC70 = 0;
    Enchant = 0;
    UnknownC84 = 0;
    bStackable = 0;
    UnknownC8C = 0;
    UnknownC90 = -1;
    UnknownC94 = 0;
    UnknownC78 = 0;
    UnknownC7C = 0;
    UnknownC80 = 0;
    UnknownC98 = 0;
}
NW_HOOK_CTOR(0x100342c0, NItemInfo)
