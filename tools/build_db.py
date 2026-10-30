#!/usr/bin/env python3
"""Master function table: db/functions.csv (+ docs/PROGRESSO.md).

Merges every source of knowledge about the stock NWindow.dll:
  analysis/ghidra/functions.tsv  every function Ghidra knows (after tools/gen_seeds.py pass)
  analysis/classes.json          IMPLEMENT_CLASS table: ctors, vtables (tools/extract_classes.py)
  analysis/eh.json               EH funclets and guard() names (tools/eh_scan.py)
  db/names.csv                   names given by hand (va,name[,note]) - highest priority
  db/morto.csv                   stock functions no path of the game reaches in hook mode
  db/vslots.csv                  virtual slot names (class,slot,name): named once, inherited by
                                 every override in the subclasses
  src/**                         NW_HOOK_* macros: which functions are already reimplemented

kind:   game | init (dynamic initializers / atexit) | eh (catch/unwind funclet or the handler
        stub, goes with its parent) | crt (static MSVC 7.0 runtime, replaced by ours) | thunk
        (import jump)
status: todo | done (reimplemented + hooked) | verified (done + tested in game) | conv (never
        hooked by convention) | fase5 (static initialization: the standalone build writes it) |
        morto (db/morto.csv: every caller is ours) | lib | part; the reason for conv, fase5 and
        morto goes to notes
"""
from __future__ import annotations

import csv
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

THUNK_LO, THUNK_HI = 0x100BD360, 0x100BD4D3  # jmp [IAT] thunks placed by the linker
CRT_LO = 0x100BD4D4                         # libcmt 7.0 objects follow the user objects

UOBJECT_SLOTS = ["QueryInterface", "AddRef", "Release", "`scalar deleting destructor'",
                 "ProcessEvent", "ProcessDelegate", "ProcessState", "ProcessRemoteFunction",
                 "Modify", "PostLoad", "Destroy", "Serialize", "IsPendingKill", "GotoState",
                 "GotoLabel", "InitExecution", "ShutdownAfterError", "PostEditChange",
                 "CallFunction", "ScriptConsoleExec", "Register", "LanguageChange", "Rename",
                 "NetDirty"]
HOOK_RE = re.compile(r"NW_HOOK_(\w+)\(\s*(0x[0-9A-Fa-f]{8})\s*,\s*([^\n]*)")
# the stock globals our headers name: #define GName NW_GLOBAL(Type, 0x1012xxxx)
GLOBAL_RE = re.compile(r"#define\s+(G\w+)\s+NW_GLOBAL\(.*?,\s*(0x[0-9A-Fa-f]{8})\s*\)")
# the stock vtables the source checks ours against: NW_CHECK_VTABLE(Class, 0x100dxxxx, N)
VTCHECK_RE = re.compile(r"NW_CHECK_VTABLE(?:_FOR)?\(\s*(\w+)\s*,(?:\s*\w+\s*,)?\s*(0x[0-9A-Fa-f]{8})")


def hook_name(kind: str, args: str) -> str:
    """Name a NW_HOOK_* registration gives the function it replaces."""
    args = args.split("//")[0]
    parts = [p.strip().rstrip(")").strip() for p in args.split(",")]
    if kind in ("CTOR", "CTOR_ARGS"):
        return f"{parts[0]}::{parts[0]}"
    if kind == "DTOR":
        return f"{parts[0]}::~{parts[0]}"
    if kind == "FUNC":
        return parts[0]
    return f"{parts[0]}::{parts[1]}"  # METHOD, METHOD_SIG, VARARGS


def read_csv(path: Path) -> list[dict]:
    if not path.exists():
        return []
    return list(csv.DictReader((l for l in path.open(encoding="utf-8") if not l.startswith("#"))))


def main() -> int:
    funcs = list(csv.DictReader((ROOT / "analysis/ghidra/functions.tsv").open(encoding="utf-8"),
                                delimiter="\t"))
    classes = json.loads((ROOT / "analysis/classes.json").read_text(encoding="utf-8"))
    eh = json.loads((ROOT / "analysis/eh.json").read_text(encoding="utf-8"))
    cpp = {c["cpp"]: c for c in classes}

    by_va = {int(f["va"], 16): f for f in funcs}
    # entries Ghidra missed (db/extra_functions.csv)
    for r in read_csv(ROOT / "db/extra_functions.csv"):
        va = int(r["va"], 16)
        if va not in by_va:
            by_va[va] = {"va": r["va"], "size": r["size"], "name": f"FUN_{r['va']}",
                         "ranges": f"{va:x}-{va + int(r['size']) - 1:x}"}
    rows = {}
    for va, f in by_va.items():
        rows[va] = {"va": va, "size": int(f["size"]), "kind": "game", "name": "", "class": "",
                    "status": "todo", "src": "", "notes": ""}

    # ---- kinds -----------------------------------------------------------------------------
    funclet_parent = {}
    stub_parent = {}   # `mov eax, FuncInfo; jmp __CxxFrameHandler`, a few filed as functions
    for pva, e in eh.items():
        for c in e["catches"]:
            funclet_parent[c["va"]] = int(pva, 16)
        for u in e["unwinds"]:
            funclet_parent[u] = int(pva, 16)
        if e.get("stub"):
            stub_parent[e["stub"]] = int(pva, 16)
    eh_region = min(va for va, f in by_va.items()
                    if va > CRT_LO and f["name"].startswith(("Unwind@", "Catch@")))
    init_lo = min(c["initializer"] for c in classes)
    for va, r in rows.items():
        name = by_va[va]["name"]
        if name.startswith(("Catch@", "Unwind@")):
            r["kind"], r["status"] = "eh", "part"
            p = funclet_parent.get(va)
            r["notes"] = f"parent {p:08x}" if p else ""
        elif va in stub_parent:
            r["kind"], r["status"] = "eh", "part"
            r["notes"] = f"parent {stub_parent[va]:08x} (handler stub)"
        elif THUNK_LO <= va <= THUNK_HI:
            r["kind"], r["status"], r["name"] = "thunk", "lib", name.split("::", 1)[-1]
        elif CRT_LO <= va < eh_region:
            r["kind"], r["status"] = "crt", "lib"
            if not name.startswith("FUN_"):
                r["name"] = name
        elif va >= init_lo:
            r["kind"] = "init"
    for r in read_csv(ROOT / "db/not_functions.csv"):
        va = int(r["va"], 16)
        if va in rows:
            rows[va]["kind"], rows[va]["status"] = "fragment", "part"
            rows[va]["notes"] = f"part of {r['part_of']}: {r['reason']}"
    names: dict[int, tuple[str, str, str]] = {}  # va -> (name, class, source)

    def give(va, name, cls, src, force=False):
        if va not in rows:
            return
        if force or va not in names:
            names[va] = (name, cls, src)

    # ---- IMPLEMENT_CLASS: initializers, ctors, InternalConstructor -------------------------
    sys.path.insert(0, str(ROOT / "tools"))
    from nwd.image import Image

    img = Image()
    text_lo, text_hi = img.sect_range(".text")
    text = img.read(text_lo, text_hi - text_lo)

    def body_at(pattern: bytes):
        """Address of the only place in .text holding these bytes (give() keeps function starts)."""
        i = text.find(pattern)
        return text_lo + i if i >= 0 and text.find(pattern, i + 1) < 0 else None

    def le(v: int) -> bytes:
        return v.to_bytes(4, "little")

    uclass_dtor = le(img.import_slot("??1UClass@@UAE@XZ"))
    ctor_users: dict[int, list[str]] = {}
    for c in classes:
        give(c["initializer"], f"{c['cpp']}::PrivateStaticClass`dynamic initializer'", c["cpp"], "class")
        # the export's: mov [autoclassX], &X::PrivateStaticClass; ret
        va = body_at(b"\xc7\x05" + le(c["autoclass_va"]) + le(c["static_class"]) + b"\xc3")
        if va:
            give(va, f"autoclass{c['cpp']}`dynamic initializer'", c["cpp"], "class")
        # what the initializer registers with atexit: mov ecx, &X::PrivateStaticClass; jmp [~UClass]
        va = body_at(b"\xb9" + le(c["static_class"]) + b"\xff\x25" + uclass_dtor)
        if va:
            give(va, f"{c['cpp']}::PrivateStaticClass`dynamic atexit destructor'", c["cpp"], "class")
        give(c["internal_ctor"], f"{c['cpp']}::InternalConstructor", c["cpp"], "class")
        if c["ctor"]:
            ctor_users.setdefault(c["ctor"], []).append(c["cpp"])

    # the other static objects (TArray globals): the initializer constructs the object and
    # registers its destructor with atexit (or only registers it), the atexit destructor is
    # `mov ecx, &G; jmp ~T`; named after the global our headers declare at that address
    gname = {}
    for path in sorted((ROOT / "include").rglob("*.h")):
        for m in GLOBAL_RE.finditer(path.read_text(encoding="utf-8", errors="replace")):
            gname[int(m.group(2), 16)] = m.group(1)

    def this_global(va):
        b = img.read(va, 5)
        return gname.get(int.from_bytes(b[1:], "little")) if b[0] == 0xB9 else None

    for va, r in rows.items():
        if r["kind"] != "init" or va in names:
            continue
        ins = list(img.disasm(va, r["size"], stop_at_ret=True))
        g = this_global(va)
        if g and ins[-1].mnemonic == "jmp":
            give(va, f"{g}`dynamic atexit destructor'", "", "global")
            continue
        for i in ins:
            if not g and i.mnemonic == "push" and i.op_str.startswith("0x") and img.in_text(int(i.op_str, 16)):
                g = this_global(int(i.op_str, 16))
        if g:
            give(va, f"{g}`dynamic initializer'", "", "global")

    # code the image exports by name: the package's DllMain (_DllMain@12)
    for exp, va in img.exports.items():
        if va in rows:
            give(va, exp.lstrip("_").split("@")[0], "", "export")
    for va, users in ctor_users.items():
        users.sort(key=lambda n: cpp[n]["depth"])
        give(va, f"{users[0]}::{users[0]}", users[0], "class")
        if len(users) > 1:
            rows[va]["notes"] = "ICF: tambem " + ", ".join(f"{u}::{u}" for u in users[1:])

    # ---- virtual slots ---------------------------------------------------------------------
    def slots(c):
        return [s["va"] for s in c.get("vtable_slots", [])]

    def slot_owner(c, k):
        """Highest ancestor of c that already has slot k (the class that introduced it)."""
        owner = c["cpp"]
        s = c.get("super")
        while s in cpp and k < len(slots(cpp[s])):
            owner = s
            s = cpp[s].get("super")
        return owner

    vslot_names = {}
    for r in read_csv(ROOT / "db/vslots.csv"):
        vslot_names[(r["class"], int(r["slot"]))] = r["name"]

    guard_name = {}
    for pva, e in eh.items():
        g = e["guard"]
        if g:
            base = [n for n in g if all(o.startswith(n) for o in g)]
            guard_name[int(pva, 16)] = base[0] if base else g[-1]

    # a guard name on an override names the whole slot
    for c in classes:
        for k, t in enumerate(slots(c)):
            if t in guard_name and "::" in guard_name[t]:
                cls, meth = guard_name[t].split("::", 1)
                if cls == c["cpp"] and "::" not in meth:
                    vslot_names.setdefault((slot_owner(c, k), k), meth)

    for c in sorted(classes, key=lambda c: c["depth"]):
        sup = cpp.get(c.get("super"))
        inherited = slots(sup) if sup else []
        for k, t in enumerate(slots(c)):
            if k < len(inherited) and inherited[k] == t:
                continue  # not overridden here
            if rows.get(t, {}).get("kind") == "thunk":
                continue
            if k < len(UOBJECT_SLOTS):
                meth = UOBJECT_SLOTS[k]
            else:
                meth = vslot_names.get((slot_owner(c, k), k), f"vfn{k:03d}")
            give(t, f"{c['cpp']}::{meth}", c["cpp"], f"vslot {k}")
            note = f"vslot {k} (+0x{4 * k:x})"
            if note not in rows[t]["notes"]:
                rows[t]["notes"] = (rows[t]["notes"] + "; " if rows[t]["notes"] else "") + note

    # the real destructor: first direct call of the `scalar deleting destructor' (vslot 3)
    for c in sorted(classes, key=lambda c: c["depth"]):
        s = slots(c)
        if len(s) > 3:
            for ins in img.disasm(s[3], 0x40, stop_at_ret=True):
                if ins.mnemonic == "call" and ins.op_str.startswith("0x"):
                    give(int(ins.op_str, 16), f"{c['cpp']}::~{c['cpp']}", c["cpp"], "dtor")
                    break

    for va, g in guard_name.items():
        give(va, g, g.split("::")[0] if "::" in g else "", "guard", force=True)

    # names carried by the reimplementation itself (NW_HOOK_* in src/)
    hooks = []
    for path in sorted((ROOT / "src").rglob("*.cpp")):
        for m in HOOK_RE.finditer(path.read_text(encoding="utf-8", errors="replace")):
            hooks.append((int(m.group(2), 16), m.group(1), m.group(3), path))
    for va, kind, args, path in hooks:
        n = hook_name(kind, args)
        # a stock guard copied from another function is reproduced on purpose: no warning when
        # the source file uses that guard name
        if (va in guard_name and guard_name[va] != n
                and f"guard({guard_name[va]})" not in path.read_text(encoding="utf-8", errors="replace")):
            print(f"WARN {va:08x}: source says {n}, guard() string says {guard_name[va]}")
        give(va, n, n.split("::")[0] if "::" in n else "", "source", force=True)

    for r in read_csv(ROOT / "db/names.csv"):
        va = int(r["va"], 16)
        give(va, r["name"], r["name"].split("::")[0] if "::" in r["name"] else "", "manual", force=True)
        if r.get("note"):
            rows[va]["notes"] = r["note"]

    for va, (n, cls, src) in names.items():
        rows[va]["name"], rows[va]["class"] = n, cls
    for va, r in rows.items():
        if not r["name"]:
            r["name"] = f"sub_{va:08x}"
        if r["kind"] == "eh" and r["notes"].startswith("parent "):
            p = int(r["notes"][7:15], 16)
            r["class"] = rows.get(p, {}).get("class", "")

    # ---- status from the source tree ---------------------------------------------------------
    verified = {int(r["va"], 16) for r in read_csv(ROOT / "db/verified.csv")}
    for va, kind, args, path in hooks:
        if va not in rows:
            print(f"WARN {path.name}: hook {va:#x} is not a known function start")
            continue
        rows[va]["status"] = "verified" if va in verified else "done"
        rows[va]["src"] = path.relative_to(ROOT).as_posix()

    set_left_status(rows, by_va, classes, img)

    # ---- write -------------------------------------------------------------------------------
    out = ROOT / "db" / "functions.csv"
    cols = ["va", "size", "kind", "status", "name", "class", "src", "notes"]
    with out.open("w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=cols)
        w.writeheader()
        for va in sorted(rows):
            r = dict(rows[va])
            r["va"] = f"{va:08x}"
            w.writerow(r)

    write_progress(rows, classes)
    return 0


FINISHED = ("done", "verified")
LEFT = ("conv", "fase5", "morto")


def set_left_status(rows, by_va, classes, img):
    """Status for what has no hook on purpose, with the reason in notes (docs/notas/Restante.md):
    conv   never hooked by convention: the deleting destructors the compiler writes for every
           class with a virtual destructor and IMPLEMENT_CLASS's InternalConstructor; the stock
           one only calls a destructor / constructor that has a hook
    fase5  the image's static initialization (IMPLEMENT_CLASS, the autoclass exports, the
           atexit destructors, the TArray globals) and DllMain: the standalone build writes them
    morto  db/morto.csv: every caller is ours; checked here against the stock code
    """
    import bisect

    uobject_delete = f"[{img.import_slot('??3UObject@@SAXPAXI@Z'):#x}]"
    gmalloc = f"[{img.import_slot('?GMalloc@@3PAVFMalloc@@A'):#x}]"
    ictors = {c["internal_ctor"] for c in classes}
    exported = set(img.exports.values())
    morto = {int(r["va"], 16): r["reason"] for r in read_csv(ROOT / "db/morto.csv")}
    vt_class = {}  # stock vtable -> class, from the NW_CHECK_VTABLEs of the source
    for path in sorted((ROOT / "src").rglob("*.cpp")):
        for m in VTCHECK_RE.finditer(path.read_text(encoding="utf-8", errors="replace")):
            vt_class[int(m.group(2), 16)] = m.group(1)

    def mark(r, status, why):
        r["status"] = status
        r["notes"] = f"{r['notes']}; {why}" if r["notes"] else why

    def deleting_dtor(va, size):
        """For `~T(); if (flags & 1) delete this; return this;`: (True, the ~T it calls or None
        when it is inline); (False, None) for anything else."""
        ins = list(img.disasm(va, size, stop_at_ret=True))
        if not ins or ins[-1].mnemonic != "ret" or ins[-1].op_str != "4":
            return False, None
        body = " ; ".join(f"{i.mnemonic} {i.op_str}" for i in ins)
        if not re.search(r"test byte ptr \[esp \+ [48]\], 1", body):
            return False, None
        if uobject_delete not in body and not (gmalloc in body and "+ 8]" in body):
            return False, None
        calls = [int(i.op_str, 16) for i in ins if i.mnemonic == "call" and i.op_str.startswith("0x")]
        return True, calls[0] if calls else None

    # who calls what (rel32 call/jmp), and which function holds an address (Ghidra's ranges)
    lo, hi = img.sect_range(".text")
    text = img.read(lo, hi - lo)
    sites: dict[int, list[int]] = {}
    for m in re.finditer(rb"[\xe8\xe9]", text):
        p = m.start()
        if p + 5 <= len(text):
            t = lo + p + 5 + int.from_bytes(text[p + 1:p + 5], "little", signed=True)
            sites.setdefault(t, []).append(lo + p)
    spans = sorted((int(a, 16), int(b, 16), va) for va, f in by_va.items()
                   for a, b in (rg.split("-") for rg in f["ranges"].split(",")))
    starts = [s[0] for s in spans]

    def ours(site):
        i = bisect.bisect_right(starts, site) - 1
        while i >= 0 and spans[i][0] <= site:
            if site <= spans[i][1]:
                r = rows[spans[i][2]]
                if r["kind"] == "eh" and r["notes"].startswith("parent "):
                    r = rows.get(int(r["notes"][7:15], 16), r)
                return r["status"] in FINISHED
            i -= 1
        return False

    for va in sorted(rows):
        r = rows[va]
        if r["kind"] not in ("game", "init") or r["status"] != "todo":
            continue
        n = r["name"]
        if r["kind"] == "init" or va in exported:
            if "::PrivateStaticClass`dynamic initializer'" in n:
                why = "IMPLEMENT_CLASS: construtor nativo da UClass + atexit"
            elif n.startswith("autoclass"):
                why = "inicializador do export " + n.split("`")[0]
            elif "::PrivateStaticClass`dynamic atexit destructor'" in n:
                why = "~UClass registrado no atexit"
            elif n.startswith("G") and "`dynamic initializer'" in n:
                calls = sum(1 for i in img.disasm(va, r["size"]) if i.mnemonic == "call")
                why = "global " + n.split("`")[0] + (" (TArray): construtor e registro no atexit" if calls > 1
                                                     else " (TArray zerado): só o registro no atexit")
            elif n.startswith("G") and "`dynamic atexit destructor'" in n:
                why = "global " + n.split("`")[0] + " (TArray): destrutor registrado no atexit"
            elif n == "DllMain":
                why = "DllMain do IMPLEMENT_PACKAGE; sem gancho: o regcheck acusa o chamador do CRT"
            else:
                why = "inicialização estática"
            mark(r, "fase5", why + " (fase 5)")
            continue
        if va in ictors:
            t = next((int(i.op_str, 16) for i in img.disasm(va, r["size"], stop_at_ret=True)
                      if i.mnemonic in ("call", "jmp") and i.op_str.startswith("0x")), None)
            if t in rows and rows[t]["status"] in FINISHED:
                mark(r, "conv", f"InternalConstructor do IMPLEMENT_CLASS: chama {rows[t]['name']}, que tem gancho")
            continue
        is_sdd, dtor = deleting_dtor(va, r["size"])
        if is_sdd:
            if dtor is None:
                vt = next((int(i.op_str.split(", ")[1], 16) for i in img.disasm(va, r["size"], stop_at_ret=True)
                           if i.mnemonic == "mov" and re.fullmatch(r"dword ptr \[e\w\w\], 0x[0-9a-f]+", i.op_str)), None)
                cls, why = vt_class.get(vt, ""), "o destrutor é inline"
            elif dtor in rows and rows[dtor]["status"] in FINISHED:
                cls, why = rows[dtor]["class"], f"chama {rows[dtor]['name']}, que tem gancho"
            else:
                continue
            if n.startswith("sub_") and cls:
                r["name"], r["class"] = f"{cls}::`scalar deleting destructor'", cls
            mark(r, "conv", "deleting destructor do compilador: " + why)
            continue
        if va in morto:
            stock = [s for s in sites.get(va, []) if not ours(s)]
            if stock:
                print(f"WARN morto {va:08x}: still called by the original at "
                      + ", ".join(f"{s:08x}" for s in stock))
            else:
                mark(r, "morto", morto[va])


def write_progress(rows, classes):
    from collections import Counter

    work = [r for r in rows.values() if r["kind"] in ("game", "init")]
    done = [r for r in work if r["status"] in FINISHED]
    ver = [r for r in work if r["status"] == "verified"]
    left = {s: [r for r in work if r["status"] == s] for s in LEFT + ("todo",)}
    tot_b = sum(r["size"] for r in work)
    done_b = sum(r["size"] for r in done)
    kinds = Counter(r["kind"] for r in rows.values())

    def count(rs):
        return f"**{len(rs)}** ({sum(r['size'] for r in rs):,} bytes)"

    lines = [
        "# Progresso da decompilação", "",
        "Gerado por `tools/build_db.py` — não editar à mão.", "",
        f"- Funções a decompilar (game + init): **{len(work)}** ({tot_b:,} bytes)",
        f"- Reimplementadas (hook ativo): **{len(done)}** ({done_b:,} bytes, "
        f"{100.0 * done_b / tot_b:.2f}% do código)",
        f"- Verificadas no jogo: **{len(ver)}**",
        f"- Sem gancho, com motivo na coluna notes do banco (docs/notas/Restante.md):",
        f"  - convenção (deleting destructors, InternalConstructor): {count(left['conv'])}",
        f"  - só na fase 5 (inicialização estática, DllMain): {count(left['fase5'])}",
        f"  - mortas no modo gancho (db/morto.csv): {count(left['morto'])}",
        f"- Sem gancho e sem motivo (todo): {count(left['todo'])}", "",
        "| kind | funções |", "|---|---:|",
    ]
    for k, n in sorted(kinds.items()):
        lines.append(f"| {k} | {n} |")
    lines += ["", "## Por classe", "", "| classe | funções | feitas | sem gancho | todo | bytes | % bytes |",
              "|---|---:|---:|---:|---:|---:|---:|"]
    by_cls: dict[str, list] = {}
    for r in work:
        by_cls.setdefault(r["class"] or "(sem classe)", []).append(r)
    for cls in sorted(by_cls, key=lambda c: (c == "(sem classe)", c.lower())):
        rs = by_cls[cls]
        b = sum(r["size"] for r in rs)
        db = sum(r["size"] for r in rs if r["status"] in FINISHED)
        nd = sum(1 for r in rs if r["status"] in FINISHED)
        nl = sum(1 for r in rs if r["status"] in LEFT)
        nt = sum(1 for r in rs if r["status"] == "todo")
        lines.append(f"| {cls} | {len(rs)} | {nd} | {nl} | {nt} | {b:,} | {100.0 * db / b if b else 0:.1f}% |")
    (ROOT / "docs" / "PROGRESSO.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"db: {len(rows)} functions; work {len(work)} ({tot_b:,} B): done {len(done)} "
          f"({100.0 * done_b / tot_b:.2f}%), " + ", ".join(f"{s} {len(left[s])}" for s in left)
          + f"; kinds {dict(kinds)}")


if __name__ == "__main__":
    sys.exit(main())
