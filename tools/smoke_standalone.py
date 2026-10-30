#!/usr/bin/env python3
"""Load the standalone NWindow.dll outside the game the way the engine does (phase 5,
docs/notas/Standalone.md, section 10), and do the same with the stock NWindow.dll as a control.

Two smoke Systems under build/, each with the engine DLLs of the test client and a NWindow.dll
installed by tools/install_test.py --standalone (no NWindowDecomp.dll):
  build/smoke_standalone        our standalone build
  build/smoke_standalone_stock  orig/NWindow.dll, unpatched
nwd_smoke --standalone runs the engine's path in stages (tools/smoke/nwd_smoke_standalone.cpp):
bind (Core's appGetDllHandle), dllmain, exports (appGetDllExport), queue (GAutoRegister), init
(FName/UObject::StaticInit, i.e. ProcessRegistrants, with stand-ins for what appInit sets up),
registered, load (StaticLoadClass of "ini:Engine.Engine.L2Console" / "...L2Font" as Engine.dll
calls it) and construct (StaticConstructObject of both, as Engine.dll does next).

The output has no addresses: ours must print exactly what the stock DLL prints (Core's log
included), and every registered class must match analysis/classes.json (name, package, parent,
size). What the UClass objects hold before registration is difftest's case "classes".

  python tools/smoke_standalone.py [--dll DLL] [--system DIR] [--show] [--l2]
    --dll     the standalone NWindow.dll (default: the first of verify_standalone's candidates)
    --system  where the engine DLLs are copied from (default: the test client)
    --show    print the whole output of our run
    --l2      phase 6, the NWindowL2 build (the standalone + the LineageWindow features,
              docs/notas/Fase6.md, build/l2/Release/NWindow.dll):
              - it is loaded in a smoke System of its own (build/smoke_l2): the LineageWindow.ini
                and .log it writes stay out of the faithful build's;
              - it registers the stock 178 classes plus exactly the L2 extras of
                tests/standalone/l2_extras.csv (the list verify_standalone.py --l2 reads too).
                Its export table must be the stock one (the 180 names at their ordinals) plus the
                extras' autoclass<Class> right after it (181..), read with pefile; nwd_smoke
                --extras leaves the extras out of every stock count and check (exports, ordinals,
                GAutoRegister, registered classes) and reports them on "  l2 extra" lines, which
                are checked against the list (package, name, parent) and left out of the
                comparison with the stock output;
              - its runtime patches of other modules are read back (nwd_smoke --l2-patches,
                build/smoke_l2_patches): Core, then the client's WinDrv.dll (with Window.dll,
                Network.dll, Engine.dll), then our NWindow.dll, as in the game. The four WinDrv
                ILT thunks (WindowFix), the two Network.dll CClientSocket slots and the Engine.dll
                OnServerClose thunk (ReturnToLogin) must lead into our DLL. The six Engine.dll
                sites of the craft balloon (CraftBalloon) are patched on the first console tick,
                which never comes outside the game: they must hold the bytes its install checks.
                Its label pair holds an absolute address that only matches with Engine.dll at its
                preferred base; relocated (as always in the game: L2.exe's image sits in that
                range) the install leaves the pair stock, and so does this check.
              The faithful build never gets --l2 and stays as strict.
Exit code 0 when every stage passes and the output is the stock one.
"""
from __future__ import annotations

import argparse
import csv
import difflib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pefile

sys.path.insert(0, str(Path(__file__).resolve().parent))
from install_test import DEFAULT_SYSTEM, NO_SYSTEM  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "build" / "smoke" / "Release" / "nwd_smoke.exe"
OURS = ROOT / "build" / "smoke_standalone"
OURS_L2 = ROOT / "build" / "smoke_l2"                  # --l2: its settings and logs stay apart
OURS_L2_PATCHES = ROOT / "build" / "smoke_l2_patches"  # --l2: loaded after WinDrv/Network/Engine
STOCK = ROOT / "build" / "smoke_standalone_stock"
ORIG = ROOT / "orig" / "NWindow.dll"
ENGINE_DEPS = ["Core.dll", "Engine.dll", "IFC23.DLL", "MSVCR70.dll", "ogg.dll", "vorbis.dll", "vorbisfile.dll"]
# what WinDrv.dll brings (the L2 build patches WinDrv, Network and Engine)
PATCH_DEPS = ["WinDrv.dll", "Window.dll", "Network.dll"]
PATCHES = 7          # at load: 4 WinDrv thunks (WindowFix), 2 Network slots + 1 Engine thunk (ReturnToLogin)
BALLOON_SITES = 6    # at the first console tick: the craft balloon's Engine sites (CraftBalloon)
BALLOON_READY = ("stock bytes, as CraftBalloon checks them", "hooked (NWindow.dll)")
# the label pair's immediate is an absolute address: with Engine.dll relocated (always so in the
# game, L2.exe sits in its range) CraftBalloon's check leaves that pair stock, as NWindow_C1's did
BALLOON_RELOCATED = "stock, relocated immediate: CraftBalloon leaves this group stock"
CANDIDATES = [ROOT / "build" / "standalone" / "Release" / "NWindow.dll",
              ROOT / "build" / "bin" / "Release" / "NWindow.dll",
              ROOT / "build" / "Release" / "NWindow.dll"]
ENGINE_PARENT = {"UObject": "Core.Object", "UL2ConsoleWnd": "Engine.L2ConsoleWnd", "UL2Font": "Engine.L2Font"}
L2_EXTRAS = ROOT / "tests" / "standalone" / "l2_extras.csv"


def l2_extras() -> dict[str, dict]:
    """tests/standalone/l2_extras.csv: the classes only NWindowL2 registers (phase 6)."""
    rows = csv.DictReader(l for l in L2_EXTRAS.open(encoding="utf-8") if not l.startswith("#"))
    return {r["class"]: r for r in rows}


def check_l2_exports(dll: Path, extras: dict[str, dict]) -> list[str]:
    """The L2 DLL's export table: the stock (ordinal, name) pairs exactly, then autoclass<Extra>
    for every extra and nothing else, at the ordinals right after the stock ones."""
    def table(path: Path) -> dict[str, int]:
        pe = pefile.PE(str(path), fast_load=True)
        pe.parse_data_directories([pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXPORT"]])
        return {(s.name or b"").decode(): s.ordinal for s in pe.DIRECTORY_ENTRY_EXPORT.symbols}
    ours, stock = table(dll), table(ORIG)
    problems = [f"{n}: ordinal {ours.get(n)}, stock {o}" for n, o in sorted(stock.items()) if ours.get(n) != o]
    want = {f"autoclass{c}" for c in extras}
    extra = {n: o for n, o in ours.items() if n not in stock}
    if set(extra) != want:
        problems.append(f"exports beyond the stock: {sorted(extra)}, the L2 extras want {sorted(want)}")
    last = max(stock.values())
    if sorted(extra.values()) != list(range(last + 1, last + 1 + len(extra))):
        problems.append(f"L2 extras' ordinals {sorted(extra.values())}, wanted {last + 1}..{last + len(extra)}")
    return problems


def prepare(sysdir: Path, dll: Path, engine: Path, extra: list[str] | None = None) -> bool:
    sysdir.mkdir(parents=True, exist_ok=True)
    for dep in ENGINE_DEPS + (extra or []):
        if not (sysdir / dep).exists():
            if not (engine / dep).exists():
                print(f"smoke_standalone: {engine / dep} missing (the engine DLLs come from the test client)")
                return False
            shutil.copy2(engine / dep, sysdir / dep)
    r = subprocess.run([sys.executable, str(ROOT / "tools" / "install_test.py"), "--system", str(sysdir),
                        "--standalone", str(dll)], capture_output=True, text=True)
    if r.returncode:
        print(r.stdout + r.stderr)
    return r.returncode == 0


def instance_rva_ours(dll: Path) -> int | None:
    """GNWindowInstance (defined by NW_IMPLEMENT_PACKAGE) in our map."""
    mp = dll.with_suffix(".map")
    if not mp.exists():
        return None
    text = mp.read_text(encoding="latin-1")
    base = int(re.search(r"Preferred load address is ([0-9a-f]+)", text).group(1), 16)
    m = re.search(r"\?GNWindowInstance@@3PAUHINSTANCE__@@A\s+([0-9a-f]{8})", text)
    return int(m.group(1), 16) - base if m else None


def instance_rva_stock() -> int:
    """The stock GNWindowInstance: the hook-mode line of NWindow.h."""
    text = (ROOT / "include" / "nwindow" / "NWindow.h").read_text(encoding="utf-8")
    return int(re.search(r"#define GNWindowInstance\s+NW_GLOBAL\(HINSTANCE, (0x[0-9a-fA-F]+)\)", text).group(1), 16) - 0x10000000


def smoke(sysdir: Path, rva: int | None, extras: list[str] | None = None) -> tuple[int, list[str]]:
    cmd = [str(EXE), "--standalone", str(sysdir)]
    if rva is not None:
        cmd += ["--instance-rva", f"{rva:x}"]
    if extras:
        cmd += ["--extras", ",".join(extras)]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, errors="replace", timeout=300)
    except subprocess.TimeoutExpired:
        return -1, ["TIMEOUT after 300 s"]
    return r.returncode, (r.stdout + r.stderr).splitlines()


def normalize(lines: list[str]) -> list[str]:
    """What must be the same for the stock DLL and ours. With --l2, nwd_smoke --extras already
    leaves the L2 extras out of the stock counts and checks; their own lines go."""
    out = []
    for line in lines:
        if line.startswith(("System: ", "  l2 extra")):
            continue
        out.append(re.sub(r"image: (stock C1 build|not the stock build)", "image: -", line))
    return out


def check_registrations(lines: list[str]) -> list[str]:
    """The 'registered' lines of the stock classes against analysis/classes.json (an L2 extra's
    line starts with "l2 extra": check_extras)."""
    classes = {c["cpp"]: c for c in json.loads((ROOT / "analysis" / "classes.json").read_text(encoding="utf-8"))}
    seen, problems = set(), []
    for line in lines:
        m = re.match(r"\s+(N\w+)\s+(\w+)\.(\w+) super=(\S+) size=0x([0-9a-f]+) flags=([0-9a-f]+)", line)
        if not m:
            continue
        cpp, pkg, name, sup, size = m.group(1), m.group(2), m.group(3), m.group(4), int(m.group(5), 16)
        c = classes.get(cpp)
        if not c:
            problems.append(f"{cpp}: not in classes.json")
            continue
        seen.add(cpp)
        want_sup = ENGINE_PARENT.get(c["super"]) or f"NWindow.{c['super'][1:]}"
        if (pkg, name, sup, size) != ("NWindow", cpp[1:], want_sup, c["size"]):
            problems.append(f"{cpp}: {pkg}.{name} super={sup} size={size:#x}, expected NWindow.{cpp[1:]} "
                            f"super={want_sup} size={c['size']:#x}")
    missing = sorted(set(classes) - seen)
    if missing:
        problems.append(f"{len(missing)} classes not reported as registered: {', '.join(missing[:8])}")
    return problems


def check_extras(lines: list[str], extras: dict[str, dict]) -> list[str]:
    """The "l2 extra" registration lines of our run against tests/standalone/l2_extras.csv:
    package, name and parent (sizes and class flags: verify_standalone.py --l2)."""
    stock = {c["cpp"] for c in json.loads((ROOT / "analysis" / "classes.json").read_text(encoding="utf-8"))}
    seen, problems = {}, []
    for line in lines:
        m = re.match(r"\s+l2 extra (N\w+)\s+(\w+)\.(\w+) super=(\S+) size=0x([0-9a-f]+) flags=([0-9a-f]+)", line)
        if m:
            seen[m.group(1)] = (m.group(2), m.group(3), m.group(4), "WRONG" in line)
    for cpp, e in sorted(extras.items()):
        if cpp in stock:
            problems.append(f"{cpp}: a stock class, not an L2 extra")
            continue
        if cpp not in seen:
            problems.append(f"{cpp}: listed, but the DLL did not register it")
            continue
        pkg, name, sup, wrong = seen[cpp]
        want_sup = ENGINE_PARENT.get(e["super"]) or f"NWindow.{e['super'][1:]}"
        if wrong or (pkg, name, sup) != ("NWindow", cpp[1:], want_sup):
            problems.append(f"{cpp} (L2 extra): {pkg}.{name} super={sup}, expected NWindow.{cpp[1:]} "
                            f"super={want_sup}")
    return problems


def l2_patches(dll: Path, engine: Path, show: bool) -> bool:
    """nwd_smoke --l2-patches: the NWindowL2 build's runtime patches of other modules, read back
    after its load."""
    sysdir = OURS_L2_PATCHES
    if not prepare(sysdir, dll, engine, PATCH_DEPS):
        return False
    (sysdir / "LineageWindow.ini").write_text("[Window]\r\nLog=1\r\n", encoding="latin-1", newline="")
    (sysdir / "LineageWindow.log").unlink(missing_ok=True)
    try:
        r = subprocess.run([str(EXE), "--l2-patches", str(sysdir)], capture_output=True, text=True,
                           errors="replace", timeout=120)
        lines = (r.stdout + r.stderr).splitlines()
    except subprocess.TimeoutExpired:
        r, lines = None, ["TIMEOUT after 120 s"]
    patched = [l.strip() for l in lines if re.match(r"\s+(WinDrv|Network|Engine) .*: \S", l)]
    hooked = [l for l in patched if l.endswith("hooked (NWindow.dll)")]
    balloon = [l.strip() for l in lines if re.match(r"\s+craft balloon .*: \S", l)]
    ready = [l for l in balloon if l.endswith(BALLOON_READY)]
    relocated = [l for l in balloon if " buy label " in l and l.endswith(BALLOON_RELOCATED)]
    engine_at = [l.strip() for l in lines if l.startswith("  Engine.dll at ")]
    log = [l.strip()[2:] for l in lines if l.startswith("  | ")]
    print(f"l2 runtime patches in {sysdir.relative_to(ROOT)} (WinDrv, Network, Engine loaded first, as in the game):")
    for line in (lines if show else patched + engine_at + balloon):
        print("  " + line.strip())
    print("  LineageWindow.log: " + (" / ".join(log) if log else "(none)"))
    ok_load = r is not None and r.returncode == 0 and "DONE" in lines and len(patched) == PATCHES and \
        len(hooked) == PATCHES
    # the type and colour pairs carry the feature (the colour caves put the text too); the label
    # pair is either as checked or, with Engine.dll relocated, left stock by the install
    ok_balloon = (len(balloon) == BALLOON_SITES and len(ready) + len(relocated) == BALLOON_SITES
                  and all(l.endswith(BALLOON_READY) for l in balloon if " buy label " not in l))
    print(f"  at load: {len(hooked)} of {PATCHES} in place" + ("" if ok_load else "  <- FAILED"))
    print(f"  craft balloon (patched on the first console tick): {len(ready)} of {BALLOON_SITES} sites as its "
          "install checks them" + (", the label pair left stock (Engine.dll relocated)" if relocated else "")
          + ("" if ok_balloon else "  <- FAILED"))
    return ok_load and ok_balloon


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dll", type=Path)
    ap.add_argument("--system", type=Path, default=DEFAULT_SYSTEM)
    ap.add_argument("--show", action="store_true")
    ap.add_argument("--l2", action="store_true",
                    help="the NWindowL2 build: own smoke System, the L2 extras of "
                         "tests/standalone/l2_extras.csv, the runtime patches read back")
    args = ap.parse_args()
    if args.system is None:
        ap.error(NO_SYSTEM)
    extras = l2_extras() if args.l2 else None
    ours_dir = OURS if extras is None else OURS_L2
    dll = args.dll or next((c for c in CANDIDATES if c.exists()), None)
    if not dll or not dll.exists():
        print("smoke_standalone: no standalone NWindow.dll (cmake --build build --config Release --target NWindowStandalone)")
        return 1
    if not EXE.exists():
        print(f"smoke_standalone: {EXE} missing (cmake --build build --config Release --target nwd_smoke)")
        return 1
    if not prepare(ours_dir, dll, args.system) or not prepare(STOCK, ORIG, args.system):
        return 1

    code, ours = smoke(ours_dir, instance_rva_ours(dll), sorted(extras or {}))
    scode, stock = smoke(STOCK, instance_rva_stock())
    if args.show:
        print("\n".join(ours))

    stages = [l for l in ours if l.startswith(("STAGE ", "STOPPED", "ALL STAGES"))]
    print(f"standalone {dll.relative_to(ROOT) if ROOT in dll.parents else dll} in {ours_dir.relative_to(ROOT)}:")
    for line in stages:
        print("  " + line)
    for line in ours:
        if line.startswith("  last log line") or "WRONG" in line or " NO" in line:
            print("  " + line.strip())
    reg = [l for l in ours if re.match(r"\s+N\w+\s+\w+\.\w+ super=", l)]
    problems = check_registrations(ours)
    print(f"  registered classes checked against classes.json: {len(reg)}, "
          + ("all match" if not problems else f"{len(problems)} problems"))
    for p in problems[:20]:
        print("    " + p)
    if extras is not None:
        export_problems = check_l2_exports(dll, extras)
        print(f"  L2 extras ({len(extras)}, {L2_EXTRAS.relative_to(ROOT).as_posix()}): export table "
              + ("the stock one + their autoclass right after it" if not export_problems else "WRONG"))
        for p in export_problems[:20]:
            print("    " + p)
        for line in ours:
            if line.startswith(("  l2 extra exports", "  l2 extra classes queued")):
                print("  " + line.strip())
        xproblems = check_extras(ours, extras)
        print(f"  L2 extras registered as listed: "
              + ("all" if not xproblems else f"{len(xproblems)} problems"))
        for p in xproblems[:20]:
            print("    " + p)
        problems += export_problems + xproblems
        if not l2_patches(dll, args.system, args.show):
            problems.append("l2 runtime patches")

    a, b = normalize(stock), normalize(ours)
    same = a == b
    stock_stages = [l for l in stock if l.startswith(("STOPPED", "ALL STAGES"))]
    print(f"control: orig/NWindow.dll in {STOCK.relative_to(ROOT)}: {stock_stages[-1] if stock_stages else f'exit {scode}'}")
    print(f"  same output as the stock NWindow.dll ({len(b)} lines, Core's log included"
          + (", the L2 extras left out" if extras else "") + f"): {'yes' if same else 'NO'}")
    if not same:
        diff = list(difflib.unified_diff(a, b, "stock", "ours", n=1, lineterm=""))
        for line in diff[:40]:
            print("    " + line)
        if len(diff) > 40:
            print(f"    ... {len(diff) - 40} more diff lines")
    ok = code == 0 and any(l.startswith("ALL STAGES OK") for l in ours) and not problems and same
    print(f"smoke_standalone: {'OK' if ok else 'FAILED'}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
