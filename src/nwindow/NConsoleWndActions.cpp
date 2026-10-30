/*=============================================================================
    NConsoleWndActions.cpp - what the console does for the keyboard and the pages: the
    shortcut slots and the actions of the action window used from the keys, the console
    commands of the chat, the HTML pages' "bypass" commands, and the actors under the mouse
    (an item on the ground, a pawn).
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCActionItemWnd.h"
#include "nwindow/NCChatListBox.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCShortCutWnd.h"

#include <wchar.h>

// the map items: used from the shortcut bar they open the minimap (ShowMapItem)
enum { ITEM_Map1 = 1665, ITEM_Map2 = 1863 };

// the dialog boxes a shortcut item closes (NDialogInfo::ID, the inventory's)
enum { DIALOG_DROP_ITEM = 4, DIALOG_DESTROY_ITEM = 5, DIALOG_CRYSTALLIZE_ITEM = 6 };

// the actor under the mouse
static FORCEINLINE AActor* HoveredActor(UEngine* Engine)
{
    return static_cast<UGameEngine*>(Engine)->MouseHitActor;
}

// a system message line in the chat window's first list
static FORCEINLINE void ChatSystemMessage(NConsoleWnd* Console, INT MsgID)
{
    Console->GetDesktop()->ChatWnd->Lists[CHATLIST_All]->AddString(
        *GL2GameData.SystemMsg[MsgID].Message, 0xFFB09B79, -1);
}

// 0x1004a0b0 - a console command (the chat's "/" commands the client does not know) run by
// the first viewport, its output to the log
void NConsoleWnd::ExecCommand(TCHAR* Command)
{
    UClient* Client = Engine->Client;
    if (Client && Client->Viewports.Num())
        static_cast<FExec*>(Client->Viewports(0))->Exec(Command, *GLog);
}
NW_HOOK_METHOD(0x1004a0b0, NConsoleWnd, ExecCommand)

// 0x100465e0 - the item on the ground under the mouse, NULL for anything else
AActor* NConsoleWnd::GetPickupUnderMouse()
{
    if (!Engine)
        return NULL;
    AActor* Actor = HoveredActor(Engine);
    if (Actor && Actor->IsA(AL2Pickup::StaticClass()))
        return Actor;
    return NULL;
}
NW_HOOK_METHOD(0x100465e0, NConsoleWnd, GetPickupUnderMouse)

// 0x10047230
APawn* CastToPawn(UObject* Object)
{
    if (Object && Object->IsA(APawn::StaticClass()))
        return (APawn*)Object;
    return NULL;
}
NW_HOOK_FUNC(0x10047230, CastToPawn)

// 0x100551c0 - an action used from a shortcut key (UseShortCut): the requests of the action
// window's own (NCActionItemWnd::UseAction; the expulsion reads the target through
// GConsole), the social motions by the action table's parameter; the others go to the server
// with Ctrl (force) and Shift (don't move)
void NConsoleWnd::UseAction(INT ActionID)
{
    switch (ActionID)
    {
    case ACTION_Attack:
    {
        User* Target = GetSelectedActor();
        User* Me = GetMyUser();
        if (Me && Target)
            RequestAttack(Target->ObjectID, Me->Pawn->Location);
        break;
    }
    case ACTION_Exchange:
    {
        User* Target = GetSelectedActor();
        if (Target && Target->Pawn)
            RequestTrade(Target->Pawn->CreatureID);
        break;
    }
    case ACTION_NextTarget:
    {
        User* Target = GetSelectedActor();
        if (Target && Target->Pawn)
            RequestTargetNext(200, Target->Pawn->CreatureID);
        else
            RequestTargetNext(200, -1);
        break;
    }
    case ACTION_PickUp:
        RequestPickUp(200, -1);
        break;
    case ACTION_PetPickUp:
        PetPickUp(200, -1);
        break;
    case ACTION_Assist:
        RequestAssist();
        break;
    case ACTION_Invite:
    {
        User* Target = GetSelectedActor();
        if (Target && !Target->Unknown8)
        {
            L2ParamStack Params(10);
            Params.PushBack((void*)(INT_PTR)Target->Pawn->CreatureID);
            Params.PushBack((void*)(INT_PTR)PartyItemDistribution);
            RequestPartyInvite(Params);
        }
        else
            ChatSystemMessage(this, 185);   // "Select user to invite to your party."
        break;
    }
    case ACTION_LeaveParty:
    {
        L2ParamStack Params(10);
        RequestPartyLeave(Params);
        break;
    }
    case ACTION_Dismiss:
    {
        User* Target = GConsole->GetSelectedActor();
        if (Target && !Target->Unknown8)
        {
            L2ParamStack Params(10);
            Params.PushBack(Target->GetName());
            RequestPartyDismiss(Params);
        }
        else
            ChatSystemMessage(this, 187);   // "Select user to expel."
        break;
    }
    case ACTION_PrivateStoreSell:
        RequestVendor();
        break;
    case ACTION_PrivateStoreBuy:
        RequestBuy();
        break;
    case ACTION_PartyMatching:
        GetDesktop()->TogglePartyMatch();
        break;
    case ACTION_Greeting:
    case ACTION_Victory:
    case ACTION_Advance:
    case ACTION_SocialYes:
    case ACTION_SocialNo:
    case ACTION_SocialBow:
    case ACTION_SocialUnaware:
    case ACTION_SocialWaiting:
    case ACTION_SocialLaugh:
        if (ActionID > 0 && ActionID < 1000)
        {
            INT Social = GL2GameData.Actions[ActionID].Param;
            if (Social > 0)
                RequestSocialAction(Social);
        }
        break;
    default:
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)ActionID);
        Params.PushBack((void*)(INT_PTR)IsCtrlDown());
        Params.PushBack((void*)(INT_PTR)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
        RequestActionUse(Params);
        break;
    }
    }
}
NW_HOOK_METHOD(0x100551c0, NConsoleWnd, UseAction)

// 0x100561f0 - a slot of the shortcut bar used from a key (HookShortCutKey, UseShortCutSlot):
// an item from the player's inventory (the map items open the minimap; an open dialog box of
// dropping, destroying or crystallizing an item is closed), a skill with Ctrl (force) and
// Shift (don't move), or an action
void NConsoleWnd::UseShortCut(INT Slot)
{
    NShortCutItem& Item = GetDesktop()->ShortCutWnd->Items[Slot];
    if (Item.Type == SHORTCUT_Item)
    {
        if (Item.ID == ITEM_Map1 || Item.ID == ITEM_Map2)
        {
            ShowMapItem(Item.ID);
            return;
        }
        RequestUseItem(Item.ObjectID);
        if (GDesktop->MainDialogBox->IsShowWindow())
        {
            INT DialogID = GDesktop->MainDialogBox->DialogID;
            if (DialogID == DIALOG_DESTROY_ITEM || DialogID == DIALOG_DROP_ITEM ||
                DialogID == DIALOG_CRYSTALLIZE_ITEM)
            {
                GDesktop->MainDialogBox->Reset();
                GDesktop->MainDialogBox->HideWindow();
            }
        }
    }
    else if (Item.Type == SHORTCUT_Skill)
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Item.ID);
        Params.PushBack((void*)(INT_PTR)IsCtrlDown());
        Params.PushBack((void*)(INT_PTR)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
        RequestMagicSkillUse(Params);
    }
    else if (Item.Type == SHORTCUT_Action)
        UseAction(Item.ID);
}
NW_HOOK_METHOD(0x100561f0, NConsoleWnd, UseShortCut)

// 0x10049750 (vslot 284) - an HTML page's command: "bypass [-h] ..." sends its tokens
// ("$name": the value of the page's control of that name) joined by spaces to the server
// (the tutorial's pages their own way); the result is 2 when an "-h" came first (the page
// closes), else 1, and 0 for a bypass without tokens (nothing sent). Any other command is
// the desktop's.
INT NConsoleWnd::RunHtmlCommand(TCHAR* Command, NCHtmlViewer* Viewer)
{
    INT Result = 0;
    const TCHAR* Str = Command;
    if (!ParseCommand(&Str, TEXT("BYPASS")))
        return GetDesktop()->OnHtmlCommand(Command, Viewer);
    TCHAR Bypass[1024] = {0};
    L2ParamStack Params(10);
    TCHAR Token[256];
    if (ParseToken(Str, Token, 256, 1))
    {
        do
        {
            if (Result != 2 && !wcscmp(Token, TEXT("-h")))
                Result = 2;
            else if (Token[0] == '$')
            {
                if (Viewer)
                {
                    TCHAR* Value = Viewer->GetControlValue(&Token[1]);
                    if (Value)
                    {
                        if (!Bypass[0])
                            appSprintf(Bypass, TEXT("%s"), Value);
                        else
                            appSprintf(Bypass, TEXT("%s %s"), Bypass, Value);
                    }
                }
            }
            else if (!Bypass[0])
                appSprintf(Bypass, TEXT("%s"), Token);
            else
                appSprintf(Bypass, TEXT("%s %s"), Bypass, Token);
        } while (ParseToken(Str, Token, 256, 1));
        Params.PushBack(Bypass);
        if (Viewer->IsA(NClass::NCTutorialHtmlViewer()))
            RequestTutorialPassCmdToServer(Params);
        else
            RequestBypassToServer(Params);
        if (!Result)
            Result = 1;
    }
    return Result;
}
NW_HOOK_METHOD(0x10049750, NConsoleWnd, RunHtmlCommand)
