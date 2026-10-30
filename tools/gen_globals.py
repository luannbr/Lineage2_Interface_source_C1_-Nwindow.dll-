#!/usr/bin/env python3
"""src/standalone/NwGlobals.cpp: the stock globals, defined for the standalone build.

In hook mode our code reaches the globals of the stock image by address; each one is declared
in two branches (docs/notas/Standalone.md, section 5):

    #if NW_HOOK_MODE
    #define GFocusEntry NW_GLOBAL(NWndEntry*, 0x10123d54)                        state
    #define GBoardMarks (reinterpret_cast<const TCHAR* const*>(::nw::Addr(0x1010bfe8)))  table
    #else
    extern NWndEntry* GFocusEntry;
    extern const TCHAR* const GBoardMarks[];
    #endif

The standalone build (NW_HOOK_MODE=0) links against the definitions this script writes. The
source gives the address (the hook-mode line) and the type (the extern); the stock image
(orig/NWindow.dll) gives the initial value: .data values, .bss zeros, and the tables rebuilt
from .data/.rdata by following their string pointers. The relocations of the image tell the
pointers from the integers, and every value is checked against them.

SPEC below says how to read each global (and how many entries a table has). The script fails
when a hook-mode global of the source has no SPEC entry or no extern, and when a SPEC entry is
no longer in the source. The stock image never changes, so the output is committed; run it
again after adding or retyping a global. Globals the stock code overflows into their
neighbour are kept together in the stock layout (CHAINS).

    python tools/gen_globals.py                     write src/standalone/NwGlobals.cpp
    python tools/gen_globals.py --check DLL [MAP]   read every global back from a standalone
                                                    build (the probe links too) through its map,
                                                    compare it with the stock object, and
                                                    compare the offsets inside each chain
"""
from __future__ import annotations

import re
import struct
import sys
import textwrap
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from nwd.image import ROOT, Image  # noqa: E402

OUT = ROOT / "src" / "standalone" / "NwGlobals.cpp"

# hook-mode only: the stock std::length_error that the stock code catches (NStdLib.h); the
# standalone build throws its own
HOOK_ONLY = {"GStdLogicErrorVtbl", "GStdLengthErrorVtbl", "GStdLengthErrorThrowInfo"}

# How to read each global from the image. Kinds:
#   int              4-byte integer                    bool       1-byte bool
#   null             pointer, NULL in the image        zero N     N zero bytes, no initializer
#                                                                 (zeroed or constructed)
#   package          defined by NW_IMPLEMENT_PACKAGE (NWindow.h), only checked here
#   wstr             pointer to a string
#   tchars           the string stored in place (TCHAR array)
#   wstrs N          N string pointers                 wstr_pairs N   N pairs of them
#   ints N           N integers
#   records N (f..)  N structs of integer fields f..   debugcats N    NDebugCategory[N]
#   htmltags N       NHtmlTagInfo[N] and the attribute name arrays they point to
# A table read past its end by the stock code keeps, as its last entries, what follows it in
# the image ("past": how many, "past_what": what they are; the note says why). The globals of a
# chain (CHAINS) are members of one object instead.
SPEC: dict[str, dict] = {
    # NWindow.h
    "GConsole": dict(kind="null"),
    "GCanvas": dict(kind="null"),
    "GTextAlignParser": dict(kind="null"),
    "GNWindowDebug": dict(kind="int"),
    "GViewportHWnd": dict(kind="null"),
    "GNWindowInstance": dict(kind="package"),
    # NCWnd.h, NCConsole.cpp, NCScrollWnd.h, NCTabControl.h
    "GFocusEntry": dict(kind="null"),
    "GShiftDown": dict(kind="int"),
    "GNextFrameID": dict(kind="int"),
    "GNextWndNumber": dict(kind="int"),
    "GNextScrollCtrlID": dict(kind="int"),
    "GNextTabGroupID": dict(kind="int"),
    # NCGDevice.h
    "GWrapText": dict(kind="zero", size=0x1000),
    "GWrapLine": dict(kind="zero", size=0x1000),
    "GWrapLen": dict(kind="int"),
    "GWrapPos": dict(kind="int"),
    # NCIME.h, NCChineseIME.h, NCTaiwanIME.h
    "GImmLockIMC": dict(kind="null"),
    "GImmUnlockIMC": dict(kind="null"),
    "GImmLockIMCC": dict(kind="null"),
    "GImmUnlockIMCC": dict(kind="null"),
    "GIMEOSChecked": dict(kind="int"),
    "GIMEUnicodeOS": dict(kind="int"),
    "GChineseGetReadingString": dict(kind="null"),
    "GChineseOSVersion": dict(kind="zero", size=0x94),
    "GChineseOSVersionRead": dict(kind="bool"),
    "GChineseNT": dict(kind="bool"),
    "GChineseNTChecked": dict(kind="bool"),
    "GTaiwanGetReadingString": dict(kind="null"),
    # NCCoolTimeIcon.h, NHtmlLayout.h: TArrays (the stock initializers construct them and
    # register their destructors, as the compiler does for ours)
    "GCoolTimeTextures": dict(kind="zero", size=0xC),
    "GCoolTimeEndTextures": dict(kind="zero", size=0xC),
    "GHtmlAlignStack": dict(kind="zero", size=0xC),
    "GHtmlVAlignStack": dict(kind="zero", size=0xC),
    "GHtmlLinkTarget": dict(kind="zero", size=0x200),
    "GHtmlLinkMsg": dict(kind="zero", size=0x200),
    # NCAutoLogWnd.h, NCDebugWnd.h, NConsoleWnd*.cpp, NL2Font.h
    "GAutoLogID": dict(kind="zero", size=0x80),
    "GAutoLogPassword": dict(kind="zero", size=0x80),
    "GAutoLogSlot": dict(kind="int"),
    "GNumDebugCategories": dict(kind="int"),
    "GCursorLocked": dict(kind="int"),
    "GL2DebugLog": dict(kind="int"),
    "GIsL2AutoLogOn": dict(kind="int"),
    "GFontBytesRead": dict(kind="int"),
    # tables
    "GKeypadTextures": dict(kind="wstr_pairs", count=11, note="the keypad's buttons 1..9, 0, backspace"),
    "GChatPrefix": dict(kind="wstr_pairs", count=10, note="Say2 types 0..9"),
    "GTellPrefix": dict(kind="wstr", note="NCMatchWnd's copy of the chat prefixes, the tell's (Say2 type 2)"),
    "GAutoLogMessages": dict(kind="wstrs", count=4),
    "GGradeNames": dict(kind="wstrs", count=6, note="crystal grades none, D, C, B, A, S"),
    "GServerNames": dict(kind="wstrs", count=15, note="server ids 0..14 (NCServerSelectWnd tests <= 14)"),
    "GRaceEventNames": dict(kind="wstrs", count=5),
    "GClassTypeEventNames": dict(kind="wstrs", count=2),
    "GSexEventNames": dict(kind="wstrs", count=2),
    "GCommandSeparators": dict(kind="tchars"),
    "GBoardMarks": dict(kind="wstrs", count=19, note="17 post fields, <!--COMMENT-->, <!--THREAD-->"),
    "GHtmlAlignNames": dict(kind="wstrs", count=5),
    "GHtmlEntityNames": dict(kind="wstrs", count=8),
    "GHtmlEntityValues": dict(kind="wstrs", count=8, past=1, past_what="GHtmlSysStringMark",
                              note="7 in the stock table, ReplaceEntities reads 8"),
    "GHtmlSysStringMark": dict(kind="wstr"),
    "GHtmlCastleNameMark": dict(kind="wstr"),
    "GHtmlCastleLocMark": dict(kind="wstr"),
    "GHtmlCastleDescMark": dict(kind="wstr"),
    "GHtmlTagInfo": dict(kind="htmltags", count=50),
    "GHtmlTagIds": dict(kind="ints", count=50),
    "GDebugCategories": dict(kind="debugcats", count=20),
    "GClassMagicType": dict(kind="ints", count=58, note="class ids 0..57: 0 fighter, 1 mystic"),
    "GMSViewCharacters": dict(kind="records", count=19, past=1, fields=("Race", "Sex", "ClassID"),
                              past_what="the next table's pointers, as numbers",
                              note="18 in the stock table, AddAllCharacter reads 19"),
    "GDefaultCharacters": dict(kind="records", count=19, past=1, fields=("Race", "Sex", "ClassID"),
                               past_what="the next table's pointers, as numbers",
                               note="18 in the stock table, NCPawnViewrWnd::AddAllCharacter reads 19"),
    "GExpTable": dict(kind="ints", count=83, sentinel=True, note="levels 0..81, then the 0 that ends it"),
    "GGMExpTable": dict(kind="ints", count=82, sentinel=True, note="levels 1..81, then 0"),
    "GLobbyExpTable": dict(kind="ints", count=83, sentinel=True, note="levels 0..81, then 0"),
}

# Neighbours the stock code overflows into: an unbounded wcscpy runs from one global into the
# next (NCGDevice::FirstWrappedLine; NHtmlLayout's <a> and the HTML viewers' handlers), and what
# happens then depends on which global comes next. The standalone build keeps each chain as one
# object with the stock layout (a struct here, its offsets asserted), and the names are
# references to its members (extern T& GName in the source); --check reads the references back
# and compares the offsets with the stock ones.
CHAINS: dict[str, tuple[str, ...]] = {
    "Wrap": ("GWrapText", "GWrapLine", "GWrapLen", "GWrapPos"),
    "HtmlLink": ("GHtmlLinkTarget", "GHtmlLinkMsg", "GHtmlVAlignStack"),
}
CHAIN_OF = {m: c for c, ms in CHAINS.items() for m in ms}

DEFINE_RE = re.compile(r"^#define\s+(G\w+)\s+(.*(?:NW_GLOBAL|nw::Addr)\(.*)$", re.M)
EXTERN_RE = re.compile(r"^extern\s+([^;]+?)\s*;", re.M)
VA_RE = re.compile(r"0x[0-9A-Fa-f]{8}")


class Global:
    def __init__(self, name: str):
        self.name = name
        self.va: int | None = None
        self.decl: str | None = None       # the standalone extern, without "extern" and ";"
        self.files: list[Path] = []


def scan() -> dict[str, Global]:
    found: dict[str, Global] = {}
    paths = sorted((ROOT / "include" / "nwindow").glob("*.h")) + sorted((ROOT / "src" / "nwindow").glob("*.cpp"))
    for path in paths:
        text = path.read_text(encoding="utf-8")
        for m in DEFINE_RE.finditer(text):
            name = m.group(1)
            if name in HOOK_ONLY:
                continue
            vas = VA_RE.findall(m.group(2))
            if len(vas) != 1:
                sys.exit(f"{path.name}: {name}: expected one address in the hook-mode line")
            g = found.setdefault(name, Global(name))
            va = int(vas[0], 16)
            if g.va is not None and g.va != va:
                sys.exit(f"{name}: two addresses ({g.va:#x} and {va:#x})")
            g.va = va
            g.files.append(path)
        for m in EXTERN_RE.finditer(text):
            decl = " ".join(m.group(1).split())
            names = [n for n in re.findall(r"\bG\w+", decl)]
            if len(names) != 1 or re.search(names[0] + r"\s*\(", decl):
                continue                # not one object (a function, a list)
            g = found.setdefault(names[0], Global(names[0]))
            if g.decl is not None and g.decl != decl:
                sys.exit(f"{names[0]}: two standalone declarations ({g.decl!r} and {decl!r})")
            g.decl = decl
    for name, g in sorted(found.items()):
        if g.va is None:
            del found[name]            # an extern of something else (not a stock global)
            continue
        if g.decl is None:
            sys.exit(f"{name} ({g.va:#x}): no standalone extern next to its hook-mode line")
        if name not in SPEC:
            sys.exit(f"{name} ({g.va:#x}): not in tools/gen_globals.py's SPEC")
        if (name in CHAIN_OF) != is_reference(g.decl, name):
            sys.exit(f"{name}: a chain member is declared as a reference (extern T& {name}), any "
                     f"other global is not; got {g.decl!r}")
    for name in SPEC:
        if name not in found:
            sys.exit(f"SPEC has {name}, which the source no longer declares")
    return found


def is_reference(decl: str, name: str) -> bool:
    return bool(re.search(r"&\s*" + name + r"\b", decl))


def object_decl(decl: str, name: str) -> str:
    """The object a reference declares: 'T& N' -> 'T N', 'T (&N)[256]' -> 'T N[256]'."""
    d = re.sub(r"\(\s*&\s*" + name + r"\s*\)", name, decl)
    return re.sub(r"&\s*" + name + r"\b", " " + name, d).replace("  ", " ")


def member_size(spec: dict) -> int:
    return {"int": 4, "null": 4, "bool": 1}.get(spec["kind"]) or spec["size"]


def bsearch_misses(names: list[str]) -> list[str]:
    """The names a binary search with wcscmp (NHtmlTagTable::FindTag / FindAttr) does not find."""
    missed = []
    for want in names:
        lo, hi, found = 0, len(names), False
        while hi > lo:
            mid = lo + (hi - lo) // 2
            if names[mid] == want:
                found = True
                break
            if names[mid] < want:
                lo = mid + 1
            else:
                hi = mid
        if not found:
            missed.append(want)
    return missed


# ---- C++ output helpers --------------------------------------------------------------------
def wlit(s: str) -> str:
    """Wide string literal; hex escapes end their literal so a following hex digit stays out."""
    out, hexed = [], False
    for ch in s:
        o = ord(ch)
        if 0x20 <= o < 0x7F:
            if hexed and ch in "0123456789abcdefABCDEF":
                out.append('" L"')
            out.append({"\\": "\\\\", '"': '\\"'}.get(ch, ch))
            hexed = False
        elif ch in "\t\n\r":
            out.append({"\t": "\\t", "\n": "\\n", "\r": "\\r"}[ch])
            hexed = False
        else:
            out.append(f"\\x{o:04x}")
            hexed = True
    return 'L"' + "".join(out) + '"'


def cint(v: int) -> str:
    return str(v) if -256 < v < 256 else (f"0x{v:X}" if v >= 0 else f"-0x{-v:X}")


class Reader:
    def __init__(self, img: Image):
        self.img = img
        self.relocs = img.reloc_set
        lo, _ = img.sect_range(".data")
        s = next(s for s in img.pe.sections if s.Name.rstrip(b"\0") == b".data")
        self.data_raw_end = lo + s.SizeOfRawData

    def where(self, va: int) -> str:
        sect = self.img.section(va)
        if sect == ".data" and va >= self.data_raw_end:
            return ".bss"
        return sect

    def plain(self, va: int, n: int, what: str) -> bytes:
        """n bytes that must not hold a pointer."""
        for a in range(va - 3, va + n):
            if a in self.relocs:
                sys.exit(f"{what}: {a:#x} is a relocated pointer, expected plain data")
        return self.img.read(va, n)

    def i32(self, va: int, what: str) -> int:
        return struct.unpack("<i", self.plain(va, 4, what))[0]

    def ptr(self, va: int, what: str) -> int:
        p = self.img.u32(va)
        if p and va not in self.relocs:
            sys.exit(f"{what}: {va:#x} holds {p:#x} without a relocation, expected a pointer")
        return p

    def wstr_at(self, va: int, what: str) -> str:
        if self.img.section(va) not in (".rdata", ".data"):
            sys.exit(f"{what}: {va:#x} is not a string of the image")
        s = self.img.wstr(va, 1024)
        if s is None:
            sys.exit(f"{what}: no terminator at {va:#x}")
        return s

    def wstr_ptr(self, va: int, what: str) -> str:
        p = self.ptr(va, what)
        return "NULL" if not p else wlit(self.wstr_at(p, what))


def definition(g: Global, spec: dict, rd: Reader) -> list[str]:
    """The lines defining g (comment + definition)."""
    kind, va, name = spec["kind"], g.va, g.name
    decl = g.decl
    decl_type = decl.split(name)[0].strip()
    # a const object has internal linkage unless declared extern: say it
    const_obj = decl_type.endswith("const") or ("*" not in decl_type and decl_type.startswith("const "))
    lead = "extern " if const_obj else ""
    src = ", ".join(sorted({p.relative_to(ROOT).as_posix() for p in g.files}))
    head = [f"// {va:#010x} {rd.where(va)}; {src}" + (f"; {spec['note']}" if "note" in spec else "")]

    def sized(n: int) -> str:
        """decl with the table's size in its first empty bound."""
        if "[]" not in decl:
            sys.exit(f"{name}: a table needs an unsized array extern, got {decl!r}")
        return decl.replace("[]", f"[{n}]", 1)

    if kind == "int":
        return head + [f"{lead}{decl} = {cint(rd.i32(va, name))};"]
    if kind == "bool":
        b = rd.plain(va, 1, name)[0]
        if b not in (0, 1):
            sys.exit(f"{name}: bool {b}")
        return head + [f"{lead}{decl} = {'true' if b else 'false'};"]
    if kind == "null":
        if rd.ptr(va, name):
            sys.exit(f"{name}: not NULL in the image")
        return head + [f"{lead}{decl} = NULL;"]
    if kind == "zero":
        if any(rd.plain(va, spec["size"], name)):
            sys.exit(f"{name}: not zero in the image")
        return head + [f"{decl};"]
    if kind == "package":
        if any(rd.plain(va, 4, name)):
            sys.exit(f"{name}: not zero in the image")
        return head + [f"// {name}: defined by NW_IMPLEMENT_PACKAGE (NWindow.h), set by DllMain"]
    if kind == "wstr":
        return head + [f"{lead}{decl} = {rd.wstr_ptr(va, name)};"]
    if kind == "tchars":
        return head + [f"{lead}{decl} = {wlit(rd.wstr_at(va, name))};"]

    count, past = spec["count"], spec.get("past", 0)
    rows: list[str] = []
    if kind == "wstrs":
        rows = [rd.wstr_ptr(va + 4 * i, f"{name}[{i}]") for i in range(count)]
    elif kind == "wstr_pairs":
        rows = [f"{{ {rd.wstr_ptr(va + 8 * i, name)}, {rd.wstr_ptr(va + 8 * i + 4, name)} }}"
                for i in range(count)]
    elif kind == "ints":
        vals = [rd.i32(va + 4 * i, f"{name}[{i}]") for i in range(count)]
        if spec.get("sentinel") and (vals[-1] != 0 or 0 in vals[2:-1]):
            sys.exit(f"{name}: the table does not end at its first 0 after level 1")
        per = 8
        rows = [", ".join(str(v) for v in vals[i:i + per]) for i in range(0, count, per)]
        return head + [f"{lead}{sized(count)} =", "{"] + [f"    {r}," for r in rows] + ["};"]
    elif kind == "records":
        fields = spec["fields"]
        size = 4 * len(fields)
        for i in range(count):
            a = va + size * i
            if i < count - past:
                vals = [str(rd.i32(a + 4 * k, name)) for k in range(len(fields))]
            else:
                # past the stock table the image goes on with pointers (the next table's), which
                # the stock code reads as the fields of one more record: their stock values
                vals = [f"0x{rd.ptr(a + 4 * k, name):08X}" for k in range(len(fields))]
            rows.append("{ " + ", ".join(vals) + " }")
    elif kind == "debugcats":
        for i in range(count):
            a = va + 0x48 * i
            raw = rd.plain(a, 0x48, f"{name}[{i}]")
            s = rd.img.wstr(a, 32)
            if s is None or any(raw[2 * (len(s) + 1):0x40]):
                sys.exit(f"{name}[{i}]: the name is not a clean TCHAR[32]")
            enabled, color = struct.unpack_from("<iI", raw, 0x40)
            rows.append(f"{{ {wlit(s)}, {enabled}, 0x{color:08X} }}")
    elif kind == "htmltags":
        pre: list[str] = []
        tags: list[str] = []
        unsorted: list[str] = []
        for i in range(count):
            a = va + 12 * i
            tag = rd.wstr_at(rd.ptr(a, f"{name}[{i}].Name"), name)
            tags.append(tag)
            attrs = rd.ptr(a + 4, f"{name}[{i}].Attrs")
            n = rd.i32(a + 8, f"{name}[{i}].NumAttrs")
            if attrs:
                arr = f"GHtmlTagAttrs_{tag}"
                names = [rd.wstr_at(rd.ptr(attrs + 4 * k, arr), arr) for k in range(n)]
                missed = bsearch_misses(names)
                if missed:
                    unsorted.append(f"{tag} (never finds {', '.join(missed)})")
                pre.append(f"static const TCHAR* const {arr}[{n}] = {{ {', '.join(wlit(x) for x in names)} }};")
                rows.append(f"{{ {wlit(tag)}, {arr}, {n} }}")
            else:
                rows.append(f"{{ {wlit(tag)}, NULL, {n} }}")
        if bsearch_misses(tags):
            sys.exit(f"{name}: FindTag would miss {bsearch_misses(tags)}")
        about = ("The attribute names of each tag, which NHtmlTagTable::FindAttr searches by halves "
                 "with wcscmp. The stock arrays are sorted except " + "; ".join(unsorted) + ": faithful "
                 "data. A tag without attributes has Attrs NULL and NumAttrs 1, and FindAttr reads "
                 "through the NULL when such a tag has an attribute, as the stock code does.")
        head = ["// " + line for line in textwrap.wrap(about, 93)] + pre + [""] + head
    else:
        sys.exit(f"{name}: unknown kind {kind}")
    body = []
    for i, r in enumerate(rows):
        c = f"   // past the stock table: {spec['past_what']}" if i >= count - past else ""
        body.append(f"    {r},{c}")
    return head + [f"{lead}{sized(count)} =", "{"] + body + ["};"]


def chain_definition(chain: str, gs: list[Global], rd: Reader) -> list[str]:
    """One object holding the globals of a chain at their stock offsets, and the references the
    source names them by."""
    gs = sorted(gs, key=lambda g: g.va)
    base = gs[0].va
    stype, obj = f"N{chain}Chain", f"G{chain}Chain"
    files = sorted({p.relative_to(ROOT).as_posix() for g in gs for p in g.files})
    body, asserts, off = [], [], 0
    for g in gs:
        spec, delta = SPEC[g.name], g.va - base
        if spec["kind"] not in ("int", "null", "bool", "zero"):
            sys.exit(f"{g.name}: a chain member must be state (int, null, bool, zero)")
        if delta < off:
            sys.exit(f"{g.name}: overlaps the member before it in the stock image")
        if delta > off:
            body.append(f"    BYTE Gap{off:X}[0x{delta - off:X}];   // +0x{off:04X}, stock padding")
        size = member_size(spec)
        if any(rd.plain(g.va, size, g.name)):
            sys.exit(f"{g.name}: not zero in the stock image (the chain has no initializer)")
        body.append(f"    {object_decl(g.decl, g.name)};   // +0x{delta:04X} (stock {g.va:#010x})")
        if delta:
            asserts.append(f"NW_ASSERT_OFFSET({stype}, {g.name}, 0x{delta:X});")
        off = delta + size
    return ([f"// {base:#010x}..{base + off:#010x} {rd.where(base)}; {', '.join(files)}",
             f"// {' -> '.join(g.name for g in gs)}: the stock code overflows each one into the",
             "// next, so they stay one object in the stock order; the names are references to its",
             "// members. Zero in the stock image" + (" (the TArray constructs itself)." if any(
                 "TArray" in g.decl for g in gs) else "."),
             f"struct {stype}", "{"] + body + ["};"] + asserts
            + [f"NW_ASSERT_SIZE({stype}, 0x{off:X});", f"static {stype} {obj};"]
            + [f"{g.decl} = {obj}.{g.name};" for g in gs])


def local_structs(found: dict[str, Global]) -> list[str]:
    """Structs a .cpp declares for its own table: copied here (identical, as the ODR allows)."""
    out: dict[str, str] = {}
    for g in found.values():
        for path in g.files:
            if path.suffix != ".cpp":
                continue
            text = path.read_text(encoding="utf-8").replace("\r\n", "\n")
            for tname in re.findall(r"\b(N\w+)\b", g.decl):
                m = re.search(r"^struct " + tname + r"\n\{.*?^\};\n", text, re.M | re.S)
                if not m:
                    continue
                if tname in out and out[tname] != m.group(0):
                    sys.exit(f"struct {tname}: the copies in the source differ")
                out[tname] = m.group(0)
    lines: list[str] = []
    for tname in sorted(out):
        lines += out[tname].rstrip("\n").split("\n") + [""]
    return lines


def check(dll: Path, mapfile: Path) -> int:
    """Every global of a linked standalone NWindow.dll equals the stock one: the same bytes, or
    for pointers to strings the same strings; and the members of each chain sit at the stock
    distances from one another."""
    found = scan()
    stock, ours = Image(), Image(dll)
    addr: dict[str, int] = {}
    refs: set[str] = set()
    for m in re.finditer(r"^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+\?(G\w+)@@3(\S*)\s+([0-9a-f]{8})",
                         mapfile.read_text(encoding="latin-1"), re.M):
        if m.group(1) not in addr:
            addr[m.group(1)] = int(m.group(3), 16)
            if m.group(2).startswith("AA"):     # "T& G": a reference
                refs.add(m.group(1))
    bad = 0
    # a chain member's symbol is its reference: follow it (it must be bound at link time)
    for name in CHAIN_OF:
        ref = addr.get(name)
        if ref is None or name not in refs:
            continue
        if ref not in ours.reloc_set:
            print(f"DIFF {name}: the reference at {ref:#x} is not bound statically (no relocation)")
            bad += 1
        addr[name] = ours.u32(ref)
    for chain, names in CHAINS.items():
        if any(n not in addr for n in names):
            continue
        base = min(names, key=lambda n: found[n].va)
        got = [addr[n] - addr[base] for n in names]
        want = [found[n].va - found[base].va for n in names]
        where = ", ".join(f"{n} {d:+#x}" for n, d in zip(names, got))
        if got != want:
            print(f"DIFF chain {chain}: {where}; stock " + ", ".join(f"{d:+#x}" for d in want))
            bad += 1
        else:
            print(f"chain {chain} at {addr[base]:#x} as in the stock image: {where}")

    def text(img: Image, p: int) -> str | None:
        return None if p == 0 else img.wstr(p, 1024)

    def same_strs(a: int, va: int, n: int) -> bool:
        return all(text(ours, ours.u32(a + 4 * i)) == text(stock, stock.u32(va + 4 * i)) for i in range(n))

    for g in sorted(found.values(), key=lambda g: g.va):
        spec, a, va = SPEC[g.name], addr.get(g.name), g.va
        kind = spec["kind"]
        if a is None:
            print(f"MISSING {g.name}")
            bad += 1
            continue
        size = {"int": 4, "null": 4, "package": 4, "bool": 1, "zero": spec.get("size", 0),
                "ints": 4 * spec.get("count", 0), "debugcats": 0x48 * spec.get("count", 0),
                "records": 4 * len(spec.get("fields", ())) * spec.get("count", 0)}.get(kind)
        if size is not None:
            ok = ours.read(a, size) == stock.read(va, size)
        elif kind == "wstr":
            ok = same_strs(a, va, 1)
        elif kind == "tchars":
            ok = ours.wstr(a) == stock.wstr(va)
        elif kind in ("wstrs", "wstr_pairs"):
            ok = same_strs(a, va, spec["count"] * (2 if kind == "wstr_pairs" else 1))
        else:  # htmltags
            ok = True
            for i in range(spec["count"]):
                on, oa, oc = struct.unpack("<IIi", ours.read(a + 12 * i, 12))
                sn, sa, sc = struct.unpack("<IIi", stock.read(va + 12 * i, 12))
                ok &= text(ours, on) == text(stock, sn) and oc == sc and (oa == 0) == (sa == 0)
                ok &= not oa or same_strs(oa, sa, sc)
        if not ok:
            print(f"DIFF {g.name} (stock {va:#x}, ours {a:#x})")
            bad += 1
    print(f"{len(found)} globals checked against the stock image, {bad} different or missing")
    return 1 if bad else 0


def main() -> int:
    if len(sys.argv) > 2 and sys.argv[1] == "--check":
        dll = Path(sys.argv[2])
        return check(dll, Path(sys.argv[3]) if len(sys.argv) > 3 else dll.with_suffix(".map"))
    found = scan()
    rd = Reader(Image())
    headers = sorted({p.relative_to(ROOT / "include").as_posix() for g in found.values() for p in g.files
                      if p.suffix == ".h"} - {"nwindow/NWindow.h"})
    lines = [
        "/*=============================================================================",
        "    NwGlobals.cpp - the stock globals, for the standalone build.",
        "    Generated by tools/gen_globals.py from orig/NWindow.dll - do not edit.",
        "",
        "    Hook mode reaches these objects in the stock image (NW_GLOBAL / nw::Addr);",
        "    the standalone build defines them here with the stock initial values:",
        "    .data values, .bss zeros, the tables rebuilt from .data/.rdata. Ordered",
        "    by stock address; each comment says where the stock object is and which",
        "    file declares it.",
        "=============================================================================*/",
        '#include "nwindow/NWindow.h"',
    ] + [f'#include "{h}"' for h in headers] + [""]
    structs = local_structs(found)
    if structs:
        lines += ["// the record types of the tables that a .cpp declares for itself (copies)"] + structs
    n_state = n_table = 0
    for g in sorted(found.values(), key=lambda g: g.va):
        spec = SPEC[g.name]
        chain = CHAIN_OF.get(g.name)
        if chain is None:
            lines += definition(g, spec, rd) + [""]
        elif g.name == min(CHAINS[chain], key=lambda n: found[n].va):
            lines += chain_definition(chain, [found[n] for n in CHAINS[chain]], rd) + [""]
        if spec["kind"] in ("int", "bool", "null", "zero", "package"):
            n_state += 1
        else:
            n_table += 1
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(lines).rstrip("\n") + "\n", encoding="utf-8", newline="\r\n")
    print(f"{len(found)} globals ({n_state} state, {n_table} tables and strings) -> "
          f"{OUT.relative_to(ROOT).as_posix()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
