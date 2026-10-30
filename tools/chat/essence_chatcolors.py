#!/usr/bin/env python3
"""
essence_chatcolors.py - prints the chat channel colors of an Essence / Interlude-era interface
(the ChatChannelDefinition data the client's NCChatChannelManager loads), read from its Interface.xdat
or InterfaceClassic.xdat. Reference for src/features/ChatColors.cpp (docs/notas/F6_ChatColors.md).

  python tools/chat/essence_chatcolors.py "<essence-client>/system.il542/InterfaceClassic.xdat"

Where they are: NWindow.dll's UIScript::GetChatColorByType / GetChatSubColorByType read a map that
XMLChatChannelDefinition fills; in the .xdat that map is a block near the end: a count (30), then one
record per SayPacketType id: int32 type, then the color and the sub color as 4 bytes B,G,R,A each
(the dword is 0xAARRGGBB, as C1's own colors). The color is the speaker's / the prefix's, the sub
color the message's (ChatWnd.uc HandleChatmessage passes both to AddStringToChatWindow).
"""
import struct
import sys

# SayPacketType (NWindow/Classes/UIEventManager.uc of L2Emu-IFC-542), with the Lucera ChatType name
NAMES = ["NORMAL/ALL", "SHOUT", "TELL", "PARTY", "PLEDGE/CLAN", "SYSTEM", "USER_PET", "GM_PET", "MARKET/TRADE",
         "ALLIANCE", "ANNOUNCE", "CUSTOM (system msg)", "L2_FRIEND", "MSN_CHAT", "PARTY_ROOM_CHAT",
         "COMMANDER_CHAT (CC_ALL)", "INTER_PARTYMASTER (CC_COMMANDER)", "HERO", "CRITICAL_ANNOUNCE",
         "SCREEN_ANNOUNCE", "DOMINIONWAR (BATTLEFIELD)", "MPCC_ROOM_CHAT", "NPC_NORMAL", "NPC_SHOUT",
         "FRIEND_ANNOUNCE", "WORLD", "PLEDGE_INRAIDSERVER", "ALLIANCE_INRAIDSERVER",
         "CASTLEWAR_PLEDGE_COMMAND_MSG", "WORLD_INRAIDSERVER"]


def read_table(path):
    raw = open(path, "rb").read()
    key = struct.pack("<II", len(NAMES), 0) + b"\xdc\xdc\xdc\xff"   # 30 records, the first is NORMAL: DCDCDC
    at = raw.find(key)
    if at < 0:
        raise SystemExit("chat channel color block not found in " + path)
    pos = at + 4
    table = []
    for _ in range(len(NAMES)):
        table.append(struct.unpack_from("<III", raw, pos))
        pos += 12
    return table


def rgb(c):
    return f"{(c >> 16) & 255:3d},{(c >> 8) & 255:3d},{c & 255:3d}"


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    for t, color, sub in read_table(sys.argv[1]):
        print(f"{t:2d} {NAMES[t]:34} color {color:08X} ({rgb(color)})  sub {sub:08X} ({rgb(sub)})")
