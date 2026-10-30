#!/usr/bin/env python3
"""One decompilation cycle, after editing src/:

  1. build (Release, Win32)
  2. smoke: install into build/smoke_system (a minimal copy of the engine DLLs) and load
     NWindow.dll there outside the game - every hook must report "ok"
  3. verify: compare every hook with the stock function (tools/verify_hooks.py), then check
     the register assumptions of the stock callers (tools/regcheck.py)
  4. difftest: run pieces of the DLL outside the game with our code and compare what they
     write with what the stock code wrote (tests/golden/, tools/difftest.py --golden)
  4b. standalone (phase 5): our NW_DECLARE_CLASS/NW_CHECK_VTABLE against the stock class table
     (tools/verify_standalone.py --checks source); then the standalone NWindow.dll is built
     (target NWindowStandalone, build/standalone/Release), checked without running it
     (tools/verify_standalone.py) and run on the same cases against tests/golden/
     (tools/difftest.py --standalone: a case that needs a function the standalone inlined away
     is skipped with its name, anything else must be the same); last, it is loaded the engine's
     way outside the game next to the stock DLL (tools/smoke_standalone.py: bound, registered,
     ConsoleWnd/L2Font loaded and constructed by Core); each of these stops the cycle
  4c. l2 (phase 6, docs/notas/Fase6.md): the NWindow.dll players get, the standalone + the
     LineageWindow features (target NWindowL2, build/l2/Release), must build; the craft
     windows' protocol parser runs on the server's messages (build/craft/Release/
     nwd_craft_test.exe) and the Discord presence talks to a fake Discord
     (tools/presence/presence_test.py); the L2 DLL is
     checked without running it (tools/verify_standalone.py
     --l2) and smoke-loaded (tools/smoke_standalone.py --l2: in a System of its own, with the
     runtime patches of WinDrv/Network/Engine read back). Both accept exactly the classes of
     tests/standalone/l2_extras.csv on top of the stock ones; each of these stops the cycle.
  5. install into the test System (tools/install_test.py); with the game running from it the
     new build takes effect at the next start; --no-install skips it
  6. regenerate db/functions.csv and docs/PROGRESSO.md (tools/build_db.py)

  python tools/cycle.py [--system DIR] [--no-install]
Stops at the first failing step.
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from install_test import DEFAULT_SYSTEM, NO_SYSTEM, writable  # noqa: E402

SMOKE = ROOT / "build" / "smoke_system"
# what the loader needs to map NWindow.dll (Engine/Core and their dependencies)
ENGINE_DEPS = ["Core.dll", "Engine.dll", "IFC23.DLL", "MSVCR70.dll", "ogg.dll", "vorbis.dll", "vorbisfile.dll"]


def no_log_flush(ini: Path) -> None:
    """[Log] Flush=0: the smoke log is read after the process ends, and a flush per line (about
    14,500 lines) takes from 5 s to over 2 minutes depending on how busy the disk is."""
    text = ini.read_text(encoding="latin-1") if ini.exists() else ""
    if "[Log]" not in text:
        with ini.open("a", encoding="latin-1", newline="") as f:
            f.write("\r\n[Log]\r\nFlush=0\r\n")


def run(title: str, cmd: list[str]) -> bool:
    print(f"\n=== {title}", flush=True)
    env = dict(os.environ, MSYS_NO_PATHCONV="1")
    return subprocess.run(cmd, cwd=ROOT, env=env).returncode == 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--system", type=Path, default=DEFAULT_SYSTEM)
    ap.add_argument("--no-install", action="store_true")
    args = ap.parse_args()
    if args.system is None:
        ap.error(NO_SYSTEM)
    py = sys.executable
    if not (ROOT / "build" / "CMakeCache.txt").exists():
        if not run("configure", ["cmake", "-S", ".", "-B", "build", "-G", "Visual Studio 17 2022", "-A", "Win32"]):
            return 1
    if not run("build", ["cmake", "--build", "build", "--config", "Release"]):
        return 1

    SMOKE.mkdir(parents=True, exist_ok=True)
    for dep in ENGINE_DEPS:
        if not (SMOKE / dep).exists():
            shutil.copy2(args.system / dep, SMOKE / dep)
    if not run("smoke install", [py, "tools/install_test.py", "--system", str(SMOKE)]):
        return 1
    no_log_flush(SMOKE / "NWindowDecomp.ini")
    smoke_exe = ROOT / "build" / "smoke" / "Release" / "nwd_smoke.exe"
    if not run("smoke", [str(smoke_exe), str(SMOKE)]):
        return 1
    log = SMOKE / "NWindowDecomp.log"
    text = log.read_text(encoding="latin-1") if log.exists() else ""
    if "ERROR" in text or "hooks:" not in text:
        print("smoke: hook errors, see", log)
        return 1
    if not run("verify", [py, "tools/verify_hooks.py", "--log", str(log)]):
        return 1
    # stock callers that keep a volatile register across a call to a hook (whole-program
    # optimization in the stock build): must be none before anything reaches the game
    if not run("regcheck", [py, "tools/regcheck.py"]):
        return 1
    # results, not only calls: the fixed cases and the server page sets (about 3 s)
    if not run("difftest", [py, "tools/difftest.py", "--golden", "--system", str(args.system)]):
        return 1
    if not run("standalone: declarations", [py, "tools/verify_standalone.py", "--checks", "source"]):
        return 1
    if not run("standalone: build", ["cmake", "--build", "build", "--config", "Release", "--target",
                                     "NWindowStandalone"]):
        return 1
    standalone = ROOT / "build" / "standalone" / "Release" / "NWindow.dll"
    if not run("standalone: verify", [py, "tools/verify_standalone.py", "--dll", str(standalone)]):
        return 1
    if not run("standalone: difftest", [py, "tools/difftest.py", "--standalone", str(standalone),
                                        "--system", str(args.system)]):
        return 1
    # loaded, registered and constructed through Core outside the game, the output compared with
    # the stock NWindow.dll's (phase 5 step 6)
    if not run("standalone: smoke", [py, "tools/smoke_standalone.py", "--dll", str(standalone),
                                     "--system", str(args.system)]):
        return 1
    # phase 6: the NWindow.dll players get (the standalone + the LineageWindow features,
    # docs/notas/Fase6.md): built, its craft data parser run, checked and smoke-loaded
    if not run("l2: build", ["cmake", "--build", "build", "--config", "Release", "--target", "NWindowL2"]):
        return 1
    # the craft windows' protocol v1 parser, outside the game (built with ALL)
    if not run("l2: craft data", [str(ROOT / "build" / "craft" / "Release" / "nwd_craft_test.exe")]):
        return 1
    # the Discord Rich Presence frames and exchange, outside the game, against a fake Discord
    if not run("l2: discord presence", [py, "tools/presence/presence_test.py"]):
        return 1
    l2 = ROOT / "build" / "l2" / "Release" / "NWindow.dll"
    # the stock checks, plus exactly the classes only NWindowL2 registers (the L2 extras)
    if not run("l2: verify", [py, "tools/verify_standalone.py", "--dll", str(l2), "--l2"]):
        return 1
    # loaded like the faithful build, in its own System; the runtime patches read back
    if not run("l2: smoke", [py, "tools/smoke_standalone.py", "--dll", str(l2), "--system", str(args.system),
                             "--l2"]):
        return 1

    if not args.no_install:
        if not run("install", [py, "tools/install_test.py", "--system", str(args.system)]):
            return 1
        if not writable(args.system / "NWindow.dll"):
            print(f"    the game is running from {args.system}: restart it to load this build")
    return 0 if run("db", [py, "tools/build_db.py"]) else 1


if __name__ == "__main__":
    sys.exit(main())
