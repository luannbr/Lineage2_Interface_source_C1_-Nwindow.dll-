#pragma once
// The data tables of FL2GameData that the windows walk or search: the hash maps (ItemTable,
// NpcTable, SkillTable) and the entries of the items and the NPCs (the skills' is the
// engine's FL2MagicSkillData). Names ours.

#include "nwindow/NWindow.h"

// A table's hash map (UE2's TMap layout: the pairs, then the hash), by id. The viewers walk
// the pairs with TMap's iterator; nothing is removed, so its destructor's Relax
// (TMapBase::Rehash) never runs.
template <class T> struct NDataMap
{
    struct NPair
    {
        INT HashNext;
        INT Key;               // the id
        T* Value;
    };
    TArray<NPair> Pairs;
    INT* Hash;
    INT HashCount;

    void Rehash();                   // 0x1007a760 (NTemplates.cpp)
    T* FindRef(const INT& Key);      // 0x1004dda0 (NTemplates.cpp)
};

// TMapBase::Rehash (UnTemplate.h): the hash chains rebuilt in a new table of HashCount
// entries (HashCount is a power of 2), the old one freed
template <class T> void NDataMap<T>::Rehash()
{
    INT* NewHash = new (TEXT("HashMapHash")) INT[HashCount];
    for (INT i = 0; i < HashCount; i++)
        NewHash[i] = -1;
    for (INT i = 0; i < Pairs.Num(); i++)
    {
        NPair& Pair = Pairs(i);
        INT iHash = Pair.Key & (HashCount - 1);
        Pair.HashNext = NewHash[iHash];
        NewHash[iHash] = i;
    }
    if (Hash)
        appFree(Hash);   // delete[] in UE2's source (GMalloc->Free inline)
    Hash = NewHash;
}

// TMapBase::FindRef (UnTemplate.h): the value stored under Key, NULL when there is none (the
// hash is always there: no check)
template <class T> T* NDataMap<T>::FindRef(const INT& Key)
{
    for (INT i = Hash[Key & (HashCount - 1)]; i != -1; i = Pairs(i).HashNext)
        if (Pairs(i).Key == Key)
            return Pairs(i).Value;
    return NULL;
}

// An item of the item table (found by its id: NItemTable::Find), one layout for the three
// classes of items; names after what the windows make of the values (NItemInfo, NItemEntry).
enum { ITEMCLASS_Weapon = 0, ITEMCLASS_Armor = 1, ITEMCLASS_Etc = 2 };
struct NViewItemData
{
    INT Unknown00;             // +0x00
    INT Class;                 // +0x04 ITEMCLASS_* (the viewers list weapons and etc items)
    BYTE _Unknown08[0x2C - 0x08];
    L2FName IconName;          // +0x2C (NConsoleWnd::GetIconName)
    L2FName IconName2;         // +0x30 the second and third icons of an item drawn over two
    L2FName IconName3;         // +0x34   slots (the inventory's paperdoll)
    L2FName Name;              // +0x38 what the NPC viewer's item search looks in
    L2FName Description;       // +0x3C (FL2GameData::GetItemDescription)
    INT Unknown40;             // +0x40 (NItemInfo +0xC4C)
    INT Weight;                // +0x44
    INT Unknown48;             // +0x48 (NItemInfo +0xC2C)
    INT CrystalCount;          // +0x4C the crystals it breaks into (0: none, not crystallized)
    INT BodyPart;              // +0x50 the slot index (NItemEntry::BodyPart)
    INT PawnWeaponType;        // +0x54 weapons: the pawn's weapon type, 0..7 (APawn::WeaponType;
                               //   the pawn viewer's weapon lines)
    BYTE _Unknown58[0x68 - 0x58];
    UBOOL bStackable;          // +0x68 etc items
    INT EtcType;               // +0x6C etc items (8: placed where it is dropped)
    BYTE _Unknown70[0x90 - 0x70];
    INT Unknown90;             // +0x90 weapons (NItemInfo +0xC54)
    INT PAtk;                  // +0x94
    INT MAtk;                  // +0x98
    INT WeaponType;            // +0x9C (NConsoleWnd::GetWeaponTypeName)
    INT CrystalGrade;          // +0xA0
    INT UnknownA4;             // +0xA4 (NItemInfo +0xC58)
    INT UnknownA8;             // +0xA8 (NItemInfo +0xC5C)
    INT Evasion;               // +0xAC a shield's
    INT ShieldPDef;            // +0xB0
    INT ShieldRate;            // +0xB4
    INT AttackSpeed;           // +0xB8 (NConsoleWnd::GetAttackSpeedText)
    INT UnknownBC;             // +0xBC (NItemInfo +0xC64)
    INT SoulShots;             // +0xC0 used per attack
    INT SpiritShots;           // +0xC4
    BYTE _UnknownC8[0x1F4 - 0xC8];
    INT ArmorType;             // +0x1F4 armors (3: magic, the robes)
    INT ArmorCrystalGrade;     // +0x1F8
    INT ArmorEvasion;          // +0x1FC
    INT PDef;                  // +0x200
    INT MDef;                  // +0x204
    INT MPBonus;               // +0x208
};
NW_ASSERT_OFFSET(NViewItemData, IconName, 0x2C);
NW_ASSERT_OFFSET(NViewItemData, Name, 0x38);
NW_ASSERT_OFFSET(NViewItemData, Description, 0x3C);
NW_ASSERT_OFFSET(NViewItemData, CrystalCount, 0x4C);
NW_ASSERT_OFFSET(NViewItemData, EtcType, 0x6C);
NW_ASSERT_OFFSET(NViewItemData, SpiritShots, 0xC4);
NW_ASSERT_OFFSET(NViewItemData, ArmorType, 0x1F4);
NW_ASSERT_OFFSET(NViewItemData, MPBonus, 0x208);

// an NPC of the NPC table
struct NViewNpcData
{
    INT ID;                    // +0x00 (User::NpcID, APawn::NpcID)
    L2FName Name;              // +0x04 0: no NPC here
    BYTE _Unknown8[0x10 - 0x8];
    L2FName ClassName;         // +0x10 the pawn's class, loaded by name
    L2FName MeshName;          // +0x14 "LineageNPCs..." or "LineageMonsters..."

    UBOOL HasName() const { return *(const INT*)&Name != 0; }
};
