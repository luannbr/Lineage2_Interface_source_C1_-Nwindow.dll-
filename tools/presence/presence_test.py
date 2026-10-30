#!/usr/bin/env python3
"""
presence_test.py - builds tools/presence/presence_test.cpp (MSVC x86) and checks the frames that the
Discord Rich Presence feature (src/features/DiscordPresence.cpp) writes: the handshake and the
SET_ACTIVITY, as Discord's IPC reads them (opcode DWORD, length DWORD, JSON), and the whole exchange
(handshake, READY, SET_ACTIVITY, PING -> PONG, close) against a fake Discord on a pipe of its own name.
"""
import json
import os
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build" / "presence_test"
VCVARS = r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"

EXPECT = {
    "client_id": "1154460536088567829",
    "details": "Play L2GIRAN.COM chronicle 1",
    "buttons": [{"label": "Site", "url": "https://l2giran.com"},
                {"label": "Discord", "url": "https://discord.gg/uzWj8M4gcm"}],
}


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    exe = OUT / "presence_test.exe"
    src = ROOT / "tools" / "presence" / "presence_test.cpp"
    bat = OUT / "build.bat"
    bat.write_text(f'@call "{VCVARS}" >nul\r\n'
                   f'@cl /nologo /W3 /O2 /EHsc /I "{ROOT / "include"}" /I "{ROOT / "src"}" "{src}" '
                   f'/Fe"{exe}" /Fo"{OUT}\\\\" user32.lib\r\n', encoding="ascii")
    r = subprocess.run(["cmd", "/d", "/c", str(bat)], capture_output=True, text=True, errors="replace")
    if r.returncode != 0:
        print(r.stdout[-3000:], r.stderr[-2000:])
        return 1
    frames_file = OUT / "frames.bin"
    r = subprocess.run([str(exe), str(frames_file)], capture_output=True, text=True)
    print(r.stdout.strip())
    if r.returncode != 0:
        print("presence_test.exe failed", r.returncode)
        return 1
    data = frames_file.read_bytes()
    frames, o = [], 0
    while o < len(data):
        op, ln = struct.unpack_from("<II", data, o)
        frames.append((op, json.loads(data[o + 8:o + 8 + ln].decode("utf-8"))))
        o += 8 + ln
    assert o == len(data), "trailing bytes"
    (op0, hs), (op1, act) = frames
    assert op0 == 0 and hs == {"v": 1, "client_id": EXPECT["client_id"]}, hs
    assert op1 == 1 and act["cmd"] == "SET_ACTIVITY" and act["nonce"], act
    a = act["args"]["activity"]
    assert act["args"]["pid"] > 0
    assert a["details"] == EXPECT["details"], a
    assert "state" not in a and "assets" not in a, a
    assert a["buttons"] == EXPECT["buttons"], a
    assert abs(a["timestamps"]["start"] - time.time()) < 120, a["timestamps"]
    esc = r.stdout.split("escape ", 1)[1].splitlines()[0]
    assert json.loads(esc) == 'a"b\\c\nd\x01', esc
    assert "ingame 0" in r.stdout            # the test exe is not L2.exe: the feature would stay off
    assert "session wait 5000 fake 1" in r.stdout, "handshake / READY / SET_ACTIVITY / PING-PONG exchange"
    print("handshake:", json.dumps(hs))
    print("activity :", json.dumps(act, ensure_ascii=False))
    print("presence_test: all checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
