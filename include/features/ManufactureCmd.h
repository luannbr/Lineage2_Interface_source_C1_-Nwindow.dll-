/*=============================================================================
    ManufactureCmd.h - phase 6: /dwarvenmanufacture, the action ids C1's dispatch leaves out
    (37 Dwarven Manufacture, 52 servitor Return), and the requests the features send to the
    server (NWindow_C1 src/inject/ManufactureCmd.cpp; its docs MANUFACTURE_37_*.md,
    SERVITOR_RETURN_ACTION_52.md, REQUEST_ACTION_USE_37_SERVER.md). Notes:
    docs/notas/F6_ManufactureCmd.md.

    The race check of action 37 is the server's (MANUFACTURE_37_REMOVE_CLIENT_DWARF_GATE.md):
    the client always sends it.
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

#include "nwindow/NWindow.h"

namespace nwf {

// actions the stock dispatch never sends (it stops at id 31; the server's ActionName has them)
enum { ACTION_DwarvenManufacture = 37, ACTION_ServitorReturn = 52, ACTION_MaxExtended = 52 };

// RequestActionUse(ActionID, Ctrl 0, Shift 0): what the manufacture and Return buttons send
UBOOL SendRequestActionUse(INT ActionID);

// RequestBypassToServer(Cmd), the packet an HTML link sends ("mfg_*", "ss_*"). The server drops
// a bypass that comes less than 100 ms after another (silently), so ours go out 120 ms apart:
// one at once when it can, the rest queued (8 at most) for BypassFlush.
UBOOL SendBypassToServer(const TCHAR* Cmd);
// once a tick (NConsoleWnd::Tick): sends the next queued bypass when its time has come
void BypassFlush();

// RequestShortCutReg(Type, Slot, ID, Extra) / RequestShortCutDel(Slot), what the stock bar sends
// when something is dropped on it or dragged off it. Type 1 item (ID: the object), 2 skill,
// 3 action; Slot absolute (page * 10 + index).
UBOOL SendShortCutReg(INT Type, INT Slot, INT ID, INT Extra);
UBOOL SendShortCutDel(INT Slot);

// NCommandParser::GetCommand: the first token of a "/..." line. /dwarvenmanufacture is not in
// C1's command table: 1 when it was that one (RequestActionUse(37) sent, no command then).
UBOOL ManufactureCmd_OnCommand(const TCHAR* Command);

// The UseAction switches (the shortcut bar's click and key, the action window): ids 32 .. 52,
// past the stock dispatch. 37 and 52 go as RequestActionUse(id, 0, 0), the others as the stock
// RequestActionUse group (Ctrl, Shift). 1: taken; 0: the stock switch goes on.
UBOOL ManufactureCmd_UseAction(INT ActionID, UBOOL bActionWindow);

}  // namespace nwf

#endif  // NW_FEATURES
