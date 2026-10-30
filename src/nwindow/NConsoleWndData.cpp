/*=============================================================================
    NConsoleWndData.cpp - the console's getters of game data: names and texts of the skills,
    actions, items, classes and castles, the tool tip lines, the enchant bonus. They read the
    engine's tables (GL2GameData) and never the console itself, except the equipment slots
    (the player) and the buffs (the desktop's abnormal status window).
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCInventoryWnd.h"

// texture names of the crystal grades: "none", "graded", "gradec", "gradeb", "gradea", "grades"
#if NW_HOOK_MODE
#define GGradeNames (&NW_GLOBAL(const TCHAR*, 0x1010f1f4))
#else
extern const TCHAR* GGradeNames[];
#endif

// attack / defense added per enchant step, by crystal grade (none, D, C, B, A, S)
static const DOUBLE GWeaponEnchantBonus[6] = { 0.0, 2.0, 3.0, 3.0, 4.0, 5.0 };
static const DOUBLE GArmorEnchantBonus[6] = { 0.0, 1.0, 1.0, 1.0, 1.0, 1.0 };

/*-----------------------------------------------------------------------------
    Quests.
-----------------------------------------------------------------------------*/

// 0x10044600 - the quest's text at that step (NCQuestInfoWnd); the stock copied the guard of the
// virtual AddQuestID (slot 124)
const TCHAR* NConsoleWnd::GetQuestText(INT QuestID, INT Step)
{
    guard(NConsoleWnd::AddQuestID);
    for (INT i = 0; i < 1000; i++)
    {
        FL2QuestNameData& Data = GL2GameData.QuestNames[i];
        if (Data.bValid && Data.QuestID == QuestID && Data.Step == Step)
            return *Data.Text;
    }
    return NULL;
    unguard;
}
NW_HOOK_METHOD(0x10044600, NConsoleWnd, GetQuestText)

/*-----------------------------------------------------------------------------
    Skills and actions.
-----------------------------------------------------------------------------*/

// 0x10046650
const TCHAR* NConsoleWnd::GetSkillIconName(INT SkillID, INT Level)
{
    FL2MagicSkillData* Data = GL2GameData.GetMSData(SkillID, Level);
    if (!Data)
        return NULL;
    return *Data->IconName;
}
NW_HOOK_METHOD(0x10046650, NConsoleWnd, GetSkillIconName)

// 0x10046680
TCHAR* NConsoleWnd::GetSkillName(INT SkillID, INT Level)
{
    FL2MagicSkillData* Data = GL2GameData.GetMSData(SkillID, Level);
    if (!Data)
        return NULL;
    return (TCHAR*)*Data->Name;
}
NW_HOOK_METHOD(0x10046680, NConsoleWnd, GetSkillName)

// 0x100466b0
TCHAR* NConsoleWnd::GetSkillDesc(INT SkillID, INT Level)
{
    FL2MagicSkillData* Data = GL2GameData.GetMSData(SkillID, Level);
    if (!Data)
        return NULL;
    return (TCHAR*)*Data->Description;
}
NW_HOOK_METHOD(0x100466b0, NConsoleWnd, GetSkillDesc)

// 0x100466e0
UBOOL NConsoleWnd::IsToggleSkill(INT SkillID, INT Level)
{
    FL2MagicSkillData* Data = GL2GameData.GetMSData(SkillID, Level);
    if (Data && Data->OperateType == 3)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x100466e0, NConsoleWnd, IsToggleSkill)

// 0x10046710 - sysstring 313 "Magic", 312 "Passive Skill" or 311 "Active Skill"
const TCHAR* NConsoleWnd::GetSkillTypeText(INT SkillID, INT Level)
{
    FL2MagicSkillData* Data = GL2GameData.GetMSData(SkillID, Level);
    if (!Data)
        return NULL;
    if (Data->bMagic)
        return *GL2GameData.SysString[313];
    if (Data->OperateType == 2)
        return *GL2GameData.SysString[312];
    return *GL2GameData.SysString[311];
}
NW_HOOK_METHOD(0x10046710, NConsoleWnd, GetSkillTypeText)

// 0x10046780
INT NConsoleWnd::GetSkillMpCost(INT SkillID, INT Level)
{
    FL2MagicSkillData* Data = GL2GameData.GetMSData(SkillID, Level);
    if (!Data)
        return 0;
    return Data->MpCost;
}
NW_HOOK_METHOD(0x10046780, NConsoleWnd, GetSkillMpCost)

// 0x100467b0
INT NConsoleWnd::GetSkillRange(INT SkillID, INT Level)
{
    FL2MagicSkillData* Data = GL2GameData.GetMSData(SkillID, Level);
    if (!Data)
        return 0;
    return Data->Range;
}
NW_HOOK_METHOD(0x100467b0, NConsoleWnd, GetSkillRange)

// 0x100467e0
const TCHAR* NConsoleWnd::GetActionIconName(INT ActionID)
{
    for (INT i = 0; i < 1000; i++)
    {
        FL2ActionData& Data = GL2GameData.Actions[i];
        if (Data.bValid && Data.ActionID == ActionID)
            return *Data.IconName;
    }
    return NULL;
}
NW_HOOK_METHOD(0x100467e0, NConsoleWnd, GetActionIconName)

// 0x10046830
const TCHAR* NConsoleWnd::GetActionName(INT ActionID)
{
    for (INT i = 0; i < 1000; i++)
    {
        FL2ActionData& Data = GL2GameData.Actions[i];
        if (Data.bValid && Data.ActionID == ActionID)
            return *Data.Name;
    }
    return NULL;
}
NW_HOOK_METHOD(0x10046830, NConsoleWnd, GetActionName)

/*-----------------------------------------------------------------------------
    Items.
-----------------------------------------------------------------------------*/

// 0x10046880 - the pet collars: 2375, 3500..3502
UBOOL NConsoleWnd::IsPetCollar(INT ItemID)
{
    if (ItemID == 2375 || (ItemID > 3499 && ItemID <= 3502))
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x10046880, NConsoleWnd, IsPetCollar)

// 0x100468b0 - sysstring 43..49, 504 by weapon type
const TCHAR* NConsoleWnd::GetWeaponTypeName(INT Type)
{
    switch (Type)
    {
    case 1: return *GL2GameData.SysString[43];
    case 2: return *GL2GameData.SysString[44];
    case 3: return *GL2GameData.SysString[45];
    case 4: return *GL2GameData.SysString[46];
    case 6: return *GL2GameData.SysString[48];
    case 5: return *GL2GameData.SysString[47];
    case 7: return *GL2GameData.SysString[49];
    case 8: return *GL2GameData.SysString[504];
    }
    return NULL;
}
NW_HOOK_METHOD(0x100468b0, NConsoleWnd, GetWeaponTypeName)

// 0x100469a0 - the slot line of an item's tool tip, by item type (0 weapon, 1 armor,
// 2 accessory, 3 and 4) and body part bits; the armors' SubType adds the material. Result is left
// alone for the combinations without a text.
void NConsoleWnd::GetItemSlotText(INT Type, INT BodyPart, FString& Result, INT SubType)
{
    TCHAR Separator[10];
    if (Type == 0)
    {
        if (BodyPart == 0x80 || BodyPart == 0x100)
            Result = *GL2GameData.SysString[242];
        else if (BodyPart == 0x4000)
            Result = *GL2GameData.SysString[243];
    }
    else if (Type == 1)
    {
        switch (BodyPart)
        {
        case 0x1:
            Result = *GL2GameData.SysString[229];
            return;
        case 0x40:
            Result = *GL2GameData.SysString[230];
            return;
        case 0x80:
        case 0x100:
            Result = *GL2GameData.SysString[231];
            return;
        case 0x200:
            Result = *GL2GameData.SysString[232];
            return;
        case 0x1000:
            Result = *GL2GameData.SysString[233];
            return;
        case 0x2000:
            Result = *GL2GameData.SysString[234];
            return;
        case 0x400:
            appStrcpy(Separator, TEXT(" / "));
            Result = *GL2GameData.SysString[235];
            break;
        case 0x800:
            appStrcpy(Separator, TEXT(" / "));
            Result = *GL2GameData.SysString[236];
            break;
        case 0x8000:
            appStrcpy(Separator, TEXT(" / "));
            Result = *GL2GameData.SysString[542];
            break;
        default:
            return;
        }
        Result += Separator;
        switch (SubType)
        {
        case 1:
            Result += *GL2GameData.SysString[245];
            break;
        case 2:
            Result += *GL2GameData.SysString[246];
            break;
        case 3:
            Result += *GL2GameData.SysString[244];
            break;
        }
    }
    else if (Type == 2)
    {
        if ((BodyPart & 4) || (BodyPart & 2))
            Result = *GL2GameData.SysString[237];
        else if (BodyPart & 8)
            Result = *GL2GameData.SysString[238];
        else if ((BodyPart & 0x20) || (BodyPart & 0x10))
            Result = *GL2GameData.SysString[239];
    }
    else if (Type == 3)
        Result = *GL2GameData.SysString[240];
    else if (Type == 4)
        Result = *GL2GameData.SysString[241];
}
NW_HOOK_METHOD(0x100469a0, NConsoleWnd, GetItemSlotText)

// 0x10046c70 - sysstring 310 (fastest) .. 306 by the attack speed
const TCHAR* NConsoleWnd::GetAttackSpeedText(INT Speed)
{
    if (Speed < 245)
        return *GL2GameData.SysString[310];
    if (Speed < 280)
        return *GL2GameData.SysString[309];
    if (Speed < 330)
        return *GL2GameData.SysString[308];
    if (Speed < 380)
        return *GL2GameData.SysString[307];
    return *GL2GameData.SysString[306];
}
NW_HOOK_METHOD(0x10046c70, NConsoleWnd, GetAttackSpeedText)

/*-----------------------------------------------------------------------------
    Castles and classes.
-----------------------------------------------------------------------------*/

// 0x10046cf0
const TCHAR* NConsoleWnd::GetCastleName(INT CastleID)
{
    for (INT i = 0; i < 100; i++)
    {
        FL2CastleNameData& Data = GL2GameData.CastleNames[i];
        if (Data.bValid && Data.CastleID == CastleID)
            return *Data.Name;
    }
    return NULL;
}
NW_HOOK_METHOD(0x10046cf0, NConsoleWnd, GetCastleName)

// 0x10046d40
const TCHAR* NConsoleWnd::GetCastleLocation(INT CastleID)
{
    for (INT i = 0; i < 100; i++)
    {
        FL2CastleNameData& Data = GL2GameData.CastleNames[i];
        if (Data.bValid && Data.CastleID == CastleID)
            return *Data.Location;
    }
    return NULL;
}
NW_HOOK_METHOD(0x10046d40, NConsoleWnd, GetCastleLocation)

// 0x10046d90
const TCHAR* NConsoleWnd::GetCastleDescription(INT CastleID)
{
    for (INT i = 0; i < 100; i++)
    {
        FL2CastleNameData& Data = GL2GameData.CastleNames[i];
        if (Data.bValid && Data.CastleID == CastleID)
            return *Data.Description;
    }
    return NULL;
}
NW_HOOK_METHOD(0x10046d90, NConsoleWnd, GetCastleDescription)

// 0x10046f00
const TCHAR* NConsoleWnd::GetClassNameString(INT ClassID)
{
    return *GL2GameData.SysString[247 + ClassID];
}
NW_HOOK_METHOD(0x10046f00, NConsoleWnd, GetClassNameString)

// 0x10046f20 - no upper bound (grades are 0..5)
const TCHAR* NConsoleWnd::GetGradeText(INT Grade)
{
    if (Grade < 0)
        return NULL;
    return GGradeNames[Grade];
}
NW_HOOK_METHOD(0x10046f20, NConsoleWnd, GetGradeText)

// 0x10047040 - enchant levels past +3 count twice on a weapon
INT NConsoleWnd::GetWeaponEnchantSteps(INT Enchant)
{
    if (Enchant > 3)
        return 2 * Enchant - 3;
    return Enchant;
}
NW_HOOK_METHOD(0x10047040, NConsoleWnd, GetWeaponEnchantSteps)

// 0x10047050 - and three times on an armor
INT NConsoleWnd::GetArmorEnchantSteps(INT Enchant)
{
    if (Enchant > 3)
        return 3 * Enchant - 6;
    return Enchant;
}
NW_HOOK_METHOD(0x10047050, NConsoleWnd, GetArmorEnchantSteps)

// 0x10047070 - castle guards, teleporters and mercenaries (the target window shows no level)
BYTE NConsoleWnd::IsSiegeNpc(INT NpcID)
{
    if (NpcID == 12024
        || (NpcID >= 12009 && NpcID <= 12037)
        || (NpcID >= 7488 && NpcID <= 7491)
        || (NpcID >= 12093 && NpcID <= 12120)
        || (NpcID >= 12155 && NpcID <= 12158)
        || (NpcID >= 12212 && NpcID <= 12239)
        || (NpcID >= 12243 && NpcID <= 12246)
        || (NpcID >= 12265 && NpcID <= 12290)
        || (NpcID >= 12292 && NpcID <= 12297)
        || (NpcID >= 12301 && NpcID <= 12310)
        || (NpcID >= 12325 && NpcID <= 12341))
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x10047070, NConsoleWnd, IsSiegeNpc)

// 0x10047120 - soulshots (1463..1467, 1835) and spiritshots (2509..2514, 3947..3952)
UBOOL NConsoleWnd::IsShotItem(INT ItemID)
{
    if ((ItemID >= 2509 && ItemID <= 2514) || (ItemID >= 1463 && ItemID <= 1467) || ItemID == 1835
        || (ItemID >= 3947 && ItemID <= 3952))
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x10047120, NConsoleWnd, IsShotItem)

// 0x1004cfd0
INT NConsoleWnd::GetEnchantedAttack(INT Grade, INT Enchant, INT Base)
{
    return (INT)(GetWeaponEnchantSteps(Enchant) * GWeaponEnchantBonus[Grade] + Base);
}
NW_HOOK_METHOD(0x1004cfd0, NConsoleWnd, GetEnchantedAttack)

// 0x1004d000
INT NConsoleWnd::GetEnchantedDefense(INT Grade, INT Enchant, INT Base)
{
    return (INT)(GetArmorEnchantSteps(Enchant) * GArmorEnchantBonus[Grade] + Base);
}
NW_HOOK_METHOD(0x1004d000, NConsoleWnd, GetEnchantedDefense)

// 0x1004d030 - the paperdoll slot of an item by its body part bits; the two-slot ones (ears,
// fingers) by the object id the player wears in each
INT NConsoleWnd::GetEquipSlot(INT BodyPart, INT ObjectID)
{
    if (BodyPart & 6)
    {
        User* Me = GetMyUser();
        if (Me->Equipped[1] == ObjectID)
            return 3;
        if (Me->Equipped[2] == ObjectID)
            return 4;
    }
    if (BodyPart & 0x30)
    {
        User* Me = GetMyUser();
        if (Me->Equipped[4] == ObjectID)
            return 12;
        if (Me->Equipped[5] == ObjectID)
            return 13;
    }
    switch (BodyPart)
    {
    case 0x1: return 0;
    case 0x8: return 8;
    case 0x40: return 1;
    case 0x80: return 5;
    case 0x100: return 7;
    case 0x200: return 9;
    case 0x400: return 6;
    case 0x800: return 10;
    case 0x1000: return 11;
    case 0x2000: return 2;
    case 0x4000: return 15;
    case 0x8000: return 14;
    }
    return -1;
}
NW_HOOK_METHOD(0x1004d030, NConsoleWnd, GetEquipSlot)

// 0x1004d2e0 - the skill's effect is among the player's buffs
UBOOL NConsoleWnd::HasAbnormal(INT SkillID)
{
    NCAbnormalStatusWnd* Wnd = static_cast<NCConsole*>(Desktop)->AbnormalStatusWnd;
    for (INT i = 0; i < Wnd->Statuses.Num(); i++)
        if (Wnd->Statuses(i).SkillID == SkillID)
            return 1;
    return 0;
}
NW_HOOK_METHOD(0x1004d2e0, NConsoleWnd, HasAbnormal)

// 0x10054c90
const TCHAR* NConsoleWnd::GetIconName(INT ItemID)
{
    guard(NConsoleWnd::GetIconName);
    NViewItemData* Data = NItemTable::Find(&ItemID);
    if (ItemID >= 0 && Data)
        return *Data->IconName;
    return NULL;
    unguard;
}
NW_HOOK_METHOD(0x10054c90, NConsoleWnd, GetIconName)

// 0x10054d40
const TCHAR* NConsoleWnd::GetItemName(INT ItemID)
{
    guard(NConsoleWnd::GetItemName);
    NViewItemData* Data = NItemTable::Find(&ItemID);
    if (ItemID >= 0 && Data)
        return *Data->Name;
    return NULL;
    unguard;
}
NW_HOOK_METHOD(0x10054d40, NConsoleWnd, GetItemName)

// 0x10054df0 - armor type 3 (the MP robes)
UBOOL NConsoleWnd::IsMagicArmor(INT ItemID)
{
    NViewItemData* Data = NItemTable::Find(&ItemID);
    if (Data && Data->ArmorType == 3)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x10054df0, NConsoleWnd, IsMagicArmor)

/*-----------------------------------------------------------------------------
    System messages.
-----------------------------------------------------------------------------*/

// 0x100575c0 - "yyyy/mm/dd hh:mm" of a server time: systemmsg 713 with five numbers (the
// parameter stack is made for 11 values and gets 12)
void NConsoleWnd::FormatDateTime(INT Time, TCHAR* Out)
{
    INT Year, Month, Day, Hour, Min;
    appConvertIntToTime(Time, Year, Month, Day, Hour, Min);
    L2ParamStack Params(11);
    Params.PushBack((void*)713);
    Params.PushBack((void*)5);
    Params.PushBack((void*)1);
    Params.PushBack((void*)(INT_PTR)Year);
    Params.PushBack((void*)1);
    Params.PushBack((void*)(INT_PTR)Month);
    Params.PushBack((void*)1);
    Params.PushBack((void*)(INT_PTR)Day);
    Params.PushBack((void*)1);
    Params.PushBack((void*)(INT_PTR)Hour);
    Params.PushBack((void*)1);
    Params.PushBack((void*)(INT_PTR)Min);
    MakeSystemMessage(Params, Out);
}
NW_HOOK_METHOD(0x100575c0, NConsoleWnd, FormatDateTime)
