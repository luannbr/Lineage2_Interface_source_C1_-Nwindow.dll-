/*=============================================================================
    NTemplates.cpp - the template code the stock module keeps out of line: TArray methods
    for the element types of the windows (UnTemplate.h, with its check()s), UE2's Cast and
    TMap's Rehash, FString::Len. Each stock instance is hooked to ours through a typedef;
    instances that differ only in their exception tables (one per class that uses them)
    all come here. Stock: 0x10003a10, 0x10003bb0, 0x100039f0, 0x10004ce0, 0x10017620,
    0x10017eb0, 0x10018460, 0x10019880, 0x1001a010, 0x1001b380, 0x10041b20, 0x100424e0,
    0x1005fce0, 0x1006e700, 0x10070b40, 0x100745e0, 0x10075bc0, 0x10078bb0, 0x100792e0,
    0x1007a760, 0x100887e0, 0x10088800, 0x100a1660, 0x100ad570, 0x100b36e0, 0x100bcaa0,
    0x10022400, 0x10022980, 0x100259f0, 0x10028700, 0x10028a90, 0x10029fc0, 0x10036e70,
    0x1003cb40, and the console's 0x1004dd20, 0x1004dda0, 0x10055660, 0x100556f0, 0x10055780,
    0x10045e60.
=============================================================================*/
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCCandidateBox.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCListBox.h"
#include "nwindow/NCListCtrl.h"
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NCQuestInfoWnd.h"
#include "nwindow/NCSiegeInfoMainWnd.h"
#include "nwindow/NCSkillItemWnd.h"
#include "nwindow/NCTrainListBox.h"
#include "nwindow/NCVIPShopWnd.h"
#include "nwindow/NGameData.h"
#include "nwindow/NCActionItemWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NControl.h"

/*-----------------------------------------------------------------------------
    TArray of pointers: one out-of-line Remove, copy constructor and new(Array) for all of
    them; destructors per class.
-----------------------------------------------------------------------------*/

typedef TArray<void*> TPtrArray;
typedef TArray<NListRow*> TListRowArray;
typedef TArray<NItemEntry*> TItemEntryArray;
typedef TArray<NString*> TStringPtrArray;
typedef TArray<NCCoolTimeIcon*> TCoolTimeArray;
typedef TArray<NVIPNeededItem*> TVIPNeededItemArray;
typedef TArray<NSiegeClanInfo*> TSiegeClanArray;
typedef TArray<NTrainEntry*> TTrainEntryArray;
typedef TArray<AActor*> TActorArray;

// 0x10019880
NW_HOOK_METHOD(0x10019880, TPtrArray, Remove)
// 0x10075bc0 - the copy (NCMatchWnd's rows)
NW_HOOK_CTOR_ARGS(0x10075bc0, TPtrArray, (const TPtrArray& A), (A))
// 0x100745e0 - new(Array): room for a new last element (the operator's instance for pointers)
static void* NewPtrArrayElement(size_t Size, TPtrArray& Array)
{
    return operator new(Size, Array);
}
NW_HOOK_FUNC(0x100745e0, NewPtrArrayElement)

// 0x1001a010 - NCListCtrl's rows and columns, NCMatchWnd's copy of them
NW_HOOK_DTOR(0x1001a010, TListRowArray)
// 0x10017620 - NCItemWnd's, NCEquipItemWnd's and NCShortCutWnd's items
NW_HOOK_DTOR(0x10017620, TItemEntryArray)
// 0x10004ce0 - NCComboBox's items, NCChatWnd's tell names, NCPawnCreateWnd's (NCSelectCtrl)
NW_HOOK_DTOR(0x10004ce0, TStringPtrArray)
// 0x1005fce0 - NCSkillItemWnd's cool times
NW_HOOK_DTOR(0x1005fce0, TCoolTimeArray)
// 0x1006e700 - the VIP shop's lists
NW_HOOK_DTOR(0x1006e700, TVIPNeededItemArray)
// 0x100ad570 - NCSiegePartInfoWnd's clans
NW_HOOK_DTOR(0x100ad570, TSiegeClanArray)
// 0x100bcaa0 - NCTrainListBox's entries
NW_HOOK_DTOR(0x100bcaa0, TTrainEntryArray)

// 0x10070b40 - the inventories' and the pawn viewer's actor lists
NW_HOOK_METHOD(0x10070b40, TActorArray, AddItem)

typedef TArray<NCRadioButton*> TRadioButtonArray;
typedef TArray<NWndEntry*> TWndEntryArray;
typedef TArray<NControl*> TControlArray;
typedef TArray<NActionItem*> TActionItemArray;

// 0x10022400, 0x10022980 - NCTabControl's tabs (the copy GetTabs returns)
NW_HOOK_CTOR_ARGS(0x10022400, TRadioButtonArray, (const TRadioButtonArray& A), (A))
NW_HOOK_DTOR(0x10022980, TRadioButtonArray)
// 0x100259f0 - NCVirtualWndMain's window lists
NW_HOOK_DTOR(0x100259f0, TWndEntryArray)
// 0x10028700 - NCHtmlViewer's controls and the page tree's children
NW_HOOK_DTOR(0x10028700, TControlArray)
// 0x10036e70 - NCActionItemWnd's icons
NW_HOOK_DTOR(0x10036e70, TActionItemArray)
// 0x1003cb40 - a jump to 0x10004ce0 (NCChatWnd's unwind code)
NW_HOOK_DTOR(0x1003cb40, TStringPtrArray)

/*-----------------------------------------------------------------------------
    TArray of structures.
-----------------------------------------------------------------------------*/

typedef TArray<NListItem> TListItemArray;
typedef TArray<FMinimapTarget> TMinimapTargetArray;
typedef TArray<NQuestEntry> TQuestEntryArray;
typedef TArray<FAbnormalStatus> TAbnormalStatusArray;
typedef TArray<NImeText> TImeTextArray;
typedef TArray<NChatBalloon> TChatBalloonArray;
typedef TArray<FName> TNameArray;

// 0x10017eb0, 0x10018460 - NCListBox's lines (Remove: the chat and petition lists' AddString)
NW_HOOK_METHOD(0x10017eb0, TListItemArray, Remove)
NW_HOOK_DTOR(0x10018460, TListItemArray)
// 0x10078bb0, 0x100792e0 - the minimap's and the radar's targets
NW_HOOK_METHOD(0x10078bb0, TMinimapTargetArray, Remove)
NW_HOOK_DTOR(0x100792e0, TMinimapTargetArray)
// 0x100a1660 - NCQuestInfoWnd's quests
NW_HOOK_DTOR(0x100a1660, TQuestEntryArray)
// 0x100b36e0 - NCAbnormalStatusWnd's statuses
NW_HOOK_DTOR(0x100b36e0, TAbnormalStatusArray)
// 0x100039f0, 0x10003a10, 0x10003bb0 - the IMEs' clauses and the candidate box's items
NW_HOOK_METHOD(0x100039f0, TImeTextArray, Empty)
NW_HOOK_METHOD(0x10003a10, TImeTextArray, AddItem)
NW_HOOK_DTOR(0x10003bb0, TImeTextArray)
// 0x10041b20, 0x100424e0 - NCConsole's chat balloons (each one's text pointer cleared)
NW_HOOK_METHOD(0x10041b20, TChatBalloonArray, Remove)
NW_HOOK_DTOR(0x100424e0, TChatBalloonArray)
// 0x100887e0 - the pawn viewer's animation steps
NW_HOOK_METHOD(0x100887e0, TNameArray, Empty)

/*-----------------------------------------------------------------------------
    TArray<INT>: the HTML alignment stacks.
-----------------------------------------------------------------------------*/

typedef TArray<INT> TIntArray;

// 0x10028a90 - a jump to the pointer arrays' destructor 0x10028700 (the stacks' atexit code)
NW_HOOK_DTOR(0x10028a90, TIntArray)
// 0x10029fc0 - Pop (the stock one calls its out-of-line Remove)
NW_HOOK_METHOD(0x10029fc0, TIntArray, Pop)

/*-----------------------------------------------------------------------------
    Cast, TMap, FString.
-----------------------------------------------------------------------------*/

// 0x10088800 - the hair change window's
NW_HOOK_FUNC(0x10088800, Cast<AActor>)

// 0x1007a760 - the data tables' maps (one instance for them all)
typedef NDataMap<NViewItemData> TViewItemMap;
NW_HOOK_METHOD(0x1007a760, TViewItemMap, Rehash)
// 0x1004dda0 - the item table's search by id (NItemTable::Find)
NW_HOOK_METHOD(0x1004dda0, TViewItemMap, FindRef)

// 0x1001b380 - the multi-line edit box's line lengths
NW_HOOK_METHOD(0x1001b380, FString, Len)

/*-----------------------------------------------------------------------------
    The console's.
-----------------------------------------------------------------------------*/

typedef TArray<NTimer> TTimerArray;
typedef TArray<NSysMsgParam> TSysMsgParamArray;

// 0x1004dd20 - Tick drops the killed timers
NW_HOOK_METHOD(0x1004dd20, TTimerArray, Remove)
// 0x100556f0, 0x10055660 - the console's timers and system message parameters (for
// ~NConsoleWnd and the unwinding)
NW_HOOK_DTOR(0x100556f0, TTimerArray)
NW_HOOK_DTOR(0x10055660, TSysMsgParamArray)
// 0x10045e60 - a systemmsg line's implicit destructor (its Message), for the unwinding of
// EndSystemMessageParam and MakeSystemMessage
NW_HOOK_DTOR(0x10045e60, FL2SystemMsgData)

// TMap's iterator (UnTemplate.h): the map, its pairs, the index, the count removed while
// walking; with any removed, its destructor relaxes the hash (TMapBase::Relax, inline)
template <class T> struct NDataMapIterator
{
    NDataMap<T>& Map;
    TArray<typename NDataMap<T>::NPair>& Pairs;
    INT Index;
    INT Removed;

    NDataMapIterator(NDataMap<T>& InMap) : Map(InMap), Pairs(InMap.Pairs), Index(0), Removed(0) {}
    ~NDataMapIterator()
    {
        if (Removed)
        {
            while (Map.HashCount > Map.Pairs.Num() * 2 + 8)
                Map.HashCount /= 2;
            Map.Rehash();
        }
    }
};
typedef NDataMapIterator<NViewItemData> TViewItemMapIterator;

// 0x10055780 - one instance for every map's iterator: NConsoleWnd::IsValidName calls it, the
// unwinding of the others jumps to it
NW_HOOK_DTOR(0x10055780, TViewItemMapIterator)
