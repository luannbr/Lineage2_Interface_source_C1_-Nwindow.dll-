# Guide for AI coding agents (LLMs)

Read this before you touch the code. It condenses `README.en.md`, `docs/PLANO.md`, `docs/CONVENCOES.md`,
`docs/notas/Standalone.md` and `docs/notas/Fase6.md` (those notes are in Portuguese; code comments are in
English). When this file and the code disagree, the code and the judges win.

## 1. What the code base is

A C++ rebuild of the Lineage II C1 `NWindow.dll` (the UI DLL; stock build TimeDateStamp `0x40DBD452`,
VC++ 7.0, md5 `2dacb6ebcbb3bbe22cec2b296231dc08`, ImageBase `0x10000000`). One source, three CMake targets:

| Target | Define | Role |
|---|---|---|
| `NWindowDecomp` | `NW_HOOK_MODE=1` | hook DLL loaded by the stock `NWindow.dll`; each `NW_HOOK_*` puts a `jmp ours` at a stock entry |
| `NWindowStandalone` | `NW_HOOK_MODE=0` | the faithful standalone `NWindow.dll`; the reference for every stock comparison |
| `NWindowL2` | `NW_HOOK_MODE=0`, `NW_FEATURES=1` | standalone + `src/features/` (phase 6, what players run) |

Key places:

| Path | Content |
|---|---|
| `include/nwindow/`, `src/nwindow/` | decompiled game code, **one class per file**, file = class name |
| `include/ue2/` | Core/Engine declarations (`UE2Core.h`, `UE2Engine.h`, generated vtables in `gen/*.vt.inc`) |
| `src/re/` | hook runtime: `NwHook.h` (macros), installer, log, ini, `NwCrtCompat.cpp` |
| `src/standalone/` | `NwStandalone.h` (empty hook macros), `Classes.gen.cpp` (178 `NW_IMPLEMENT_CLASS`), `NwGlobals.cpp`, `NWindow.def`, `NWindowPackage.cpp` (DllMain) |
| `include/features/`, `src/features/` | phase-6 features |
| `db/` | `functions.csv` (generated, never edit) + hand-written `names.csv`, `vslots.csv`, `verify_notes.csv`, `verified.csv`, `morto.csv` |
| `analysis/` | facts from the stock binary (`classes.json`/`classes.md`: size, parent, vtable of the 178 classes; `eh.json`); `analysis/ghidra/` (not in git) = Ghidra export, `decomp/<va>.c` and `asm/<va>.asm` per function |
| `tests/golden/` | what the stock code wrote for each difftest case |
| `orig/` | the user's stock binaries (not in git) |

Object model: everything is a UE2 `UObject`. `NCObject : UObject` → `NCCmdTarget` → `NCWnd` (0x94 bytes, the
window tree) → `NControl` (0x1A0, controls) and the ~110 `NC*Wnd` windows. `NConsoleWnd : UL2ConsoleWnd`
(0x3740 bytes, 285 vtable slots: the engine's network/UI callbacks) owns the window manager
(`NCVirtualWndMain`) and the in-game desktop `NCConsole` (`GetDesktop()`), whose `OnCreate` builds every
in-game window.

## 2. How a function is reimplemented (the loop)

1. Pick `todo` functions in `db/functions.csv` (today none are left without a reason; new work is mostly
   phase 6). Prefer whole classes.
2. Read `analysis/ghidra/decomp/<va>.c`, then **check the assembly** (`analysis/ghidra/asm/<va>.asm`,
   `python tools/disasm.py <va|Class::Method>`): Ghidra gets parameter counts wrong (check `ret N`), mixes
   parameters with the return address and does not know vtable slots. Helpers: `tools/xrefs.py` (who
   references an address), `tools/vtable.py <Class>` (slots vs parent), `tools/engine_vtables.py <Class>`
   (engine virtual order).
3. Write it in `src/nwindow/<Class>.cpp`, UE2 style, address in a comment, hook macro right after:

   ```cpp
   // 0x100774e0 (vslot 35) - its own decorated title bar
   INT NCSystemMenuWnd::OnCreate()
   {
       NCWnd::OnCreate();
       ...
       return 1;
   }
   NW_HOOK_METHOD(0x100774e0, NCSystemMenuWnd, OnCreate)
   ```

   Declarations carry `// 0x...` too; invented names carry `(name ours)` (and `(nome nosso)` in
   `db/names.csv`). Layouts: `NW_ASSERT_OFFSET(Class, Field, 0x..)` for every known field and
   `NW_ASSERT_SIZE(Class, 0x..)` = the `IMPLEMENT_CLASS` size from `analysis/classes.md`; unknown fields are
   `UnknownXX` with the offset in the name. Each class has `NW_DECLARE_CLASS(Class, Super, Flags, StaticClassVA)`
   and `NW_CHECK_VTABLE(Class, StockVtableVA, Slots)`.
4. Run `python tools/cycle.py --system <test System> --no-install` until green (section 4).
5. Only after an in-game test: a line in `db/verified.csv`.

Hook macros (`src/re/NwHook.h`; empty in standalone):

| Macro | Use |
|---|---|
| `NW_HOOK_METHOD(va, Class, Method)` | virtual or non-virtual method (installs a non-virtual forwarder, so a virtual never loops through the stock vtable) |
| `NW_HOOK_METHOD_SIG(va, Class, Method, Sig)` | overloaded method, `Sig = R (Class::*)(A...)` |
| `NW_HOOK_VARARGS(va, Class, Method, Sig)` | `...` method (cdecl, `this` on the stack); must not be virtual |
| `NW_HOOK_FUNC(va, Func)` | free/static function, same convention as stock |
| `NW_HOOK_CTOR(va, Class)`, `NW_HOOK_CTOR_ARGS(va, Class, (Params), (Args))`, `NW_HOOK_DTOR(va, Class)` | constructors / destructor (a `__fastcall` wrapper has the thiscall layout) |
| `NW_NAKED ... NW_STOCK(va)` | declared but not decompiled: jumps into stock code (hook mode only) |
| `nw::StockCall<R>(va, obj, args...)`, `nw::Field<T>(obj, off)`, `nw::VirtualCall<R>(obj, slot, ...)` | scaffolding for classes not declared yet |
| `NW_GLOBAL(Type, va)` | a stock global (hook mode); standalone defines it in `src/standalone/NwGlobals.cpp` (`tools/gen_globals.py`) |

Globals are always declared in two branches: `#if NW_HOOK_MODE #define GX NW_GLOBAL(T, va) #else extern T GX; #endif`.
`NW_STOCK`, `nw::StockCall` and `NW_GLOBAL` have **no definition** in standalone: if one escapes
`#if NW_HOOK_MODE`, the standalone link fails and names it (`python tools/standalone_report.py` turns the link
errors into a work list).

## 3. Traps (each one has bitten this project)

- **Never hook a scalar deleting destructor**: the stock one calls the (already hooked) destructor.
- **Virtual overloads are laid out in reverse declaration order** by MSVC: declare the higher slot first
  (see `UCanvas` in `include/ue2/UE2Engine.h`). `NW_CHECK_VTABLE` + `verify_standalone --checks source` catch it.
- **Calling conventions**: thiscall is called through `__fastcall` with a dummy `edx`; varargs members are cdecl;
  `ret N` in the stock assembly is the truth for the stack bytes. `verify_hooks.py` compares it.
- **Register assumptions (`/GL`)**: a stock caller may keep using `ecx`/`edx`/`eax` across a direct call it
  "knows" does not touch them. Our code follows the standard ABI. `tools/regcheck.py` lists such callers: hook
  them together with the callee. `--all` lists pairs that are safe only because both sides are ours.
- **ICF folding**: identical functions share one address in the stock image (`NCObject::NCObject` =
  `NCCmdTarget::NCCmdTarget`...). One hook serves all; the `notes` column shows the cases. In our own build
  `/OPT:ICF` folds too (the link map then lists one name per fold).
- **Engine imports** must produce exactly the exported decorated name (`CORE_API`/`ENGINE_API`); otherwise the
  link fails. `TCHAR` is `unsigned short` (`/Zc:wchar_t-`), which makes `PBG`/`PAG` manglings match.
- **Compiler flags are part of the ABI**: `/Zc:wchar_t- /GR- /EHa /arch:IA32 /Oy-`, static CRT. `/EHa` keeps
  UE2's `guard/unguard` semantics; `/arch:IA32` keeps x87 maths.
- **x87 arithmetic**: integer × float constant / integer must be written in `DOUBLE`
  (`(DOUBLE)(Exp - Base) * 189.0 / (Next - Base)`), or results above 2^24 differ.
- **CRT differences**: keep `legacy_stdio_float_rounding.obj` (`NWD_CRT_COMPAT_OBJS`) and
  `src/re/NwCrtCompat.cpp` (printf half-rounding, `1.#INF`); never use UCRT `_wtol/_wtoi/_ttoi/atol/atoi`
  (use `NWtol`/`NAtol`, `include/nwindow/NCrt.h`) nor `iswctype`/`isw*` (use `NIswctype`).
- **Plain C++ in game code**: no STL, lambdas, `auto` or new templates in `src/nwindow`; UE2 types (`INT`,
  `DWORD`, `TCHAR`, `UBOOL`, `TEXT()`, `FString`, `TArray`, `appMalloc`), `guard/unguard` where the original has
  them, with the original string.
- **Faithful before pretty**: reproduce odd stock behaviour exactly (e.g. `NCGDevice::DrawText` passes `X` as the
  5th argument of `GetTextExtent`). Fixes are separate, annotated changes.
- **Layout is frozen**: never add a field or a new virtual slot to a stock class (overriding a slot the class
  already inherits is fine: AutoShot overrides `NCShortCutWnd::OnRButtonDown` under `#if NW_FEATURES`). Its size is registered with Core (`UClass` size from
  `classes.json`) and stock code / the engine index into it. A feature that needs state uses globals in its own
  `.cpp`, or a new L2-only class (section 5).
- **Inline helpers that the stock compiler inlined** are `FORCEINLINE` (e.g. `NCWnd::CreateChild`, UE2
  `operator new`): inside a guard our compiler would otherwise keep them out of line, where the stock code has
  them inline.
- **Proof marks**: code that exists only to prove hooks are live uses `NW_DEBUG_MARKER()` and the comment
  "proof mode, not stock" (`[Debug] Marker=1`).

## 4. The judges and what "green" means

`python tools/cycle.py --system <dir> [--no-install]` runs, in order, and stops at the first failure:
build → smoke (`nwd_smoke.exe`, every hook `ok` in `NWindowDecomp.log`) → `verify_hooks.py` (ret bytes, vtable
slots, imports per function vs stock; accepted differences only with a reason in `db/verify_notes.csv`) →
`regcheck.py` → `difftest.py --golden` (results equal to `tests/golden/`) → `verify_standalone.py --checks source` →
`NWindowStandalone` build → `verify_standalone.py --dll` (PE, resources, exports, classes, vtables) →
`difftest.py --standalone` → `smoke_standalone.py` (loaded like the engine loads it, output identical to stock) →
`NWindowL2` build → `nwd_craft_test.exe` → `presence/presence_test.py` → `verify_standalone.py --l2` →
`smoke_standalone.py --l2` → install
(unless `--no-install`) → `build_db.py` (regenerates `db/functions.csv`, `docs/PROGRESSO.md`).

**Green = exit code 0 of the whole cycle**, with no new compiler warning. A difference is never "fixed" by
editing a golden file or `verify_notes.csv` without understanding it; `--update-golden` rewrites goldens from
the **stock** code only. Prerequisites: `orig/NWindow.dll`, `orig/Core.dll`, `orig/Engine.dll`; the Ghidra
export (`tools/ghidra/run_export.sh`); a C1 `System` folder for `--system`. The cycle does **not** draw the UI:
visual changes need an in-game test by a human.

## 5. Adding a feature safely (phase 6)

Rules from `docs/notas/Fase6.md`:

1. Code goes in `src/features/<Feature>.cpp` + `include/features/<Feature>.h`, inside `namespace nwf` and
   `#if NW_FEATURES` (the header includes `features/NwFeatures.h`).
2. In decompiled code only the call site, in `#if NW_FEATURES ... #endif` with `// phase 6: <Feature> ...`;
   the faithful code must read the same without it. Hook mode and `NWindowStandalone` never compile feature code.
3. Wiring points that already exist:
   - `nwf::OnDllMain` (`src/features/NwFeatureStartup.cpp`): called by our `DllMain` at attach/detach, under the
     loader lock (install patches of other modules here; undo them at detach);
   - `nwf::OnConsoleTick` (`src/features/NwFeatureTick.cpp`): the first thing `NConsoleWnd::Tick` does, every
     frame, under one exception handler (a fault loses that frame's feature work, the stock tick still runs);
   - a specific stock method (`NCConsole::OnCreate`/`OnDestroy` to create/destroy windows, a window's
     `WindowProc`/`OnPaint`/`OnDrawToolTip`/`OnLButtonUp`, `NConsoleWnd::Say2` for chat...).
4. Settings: `nwf::IniInt` / `IniString` / `IniWriteInt` / `IniWriteString` (section/key; registry
   `HKCU\Software\Lineage2C1\NWindow\<Section>`, then the compiled-in `GBuiltIn` table in
   `NwFeatureConfig.cpp`, then the default). Logging: `nwf::Log(tag, fmt, ...)`, a no-op unless
   `NW_FEATURE_LOG=1`. No file next to the DLL.
5. Talking to the server: `nwf::SendBypassToServer` (queued, one per 120 ms), `SendShortCutReg`,
   `SendShortCutDel` (`include/features/ManufactureCmd.h`). Same protocol as the stock client; the server does
   not change.
6. A window with fields of its own is a new class: `NW_DECLARE_L2_CLASS` / `NW_IMPLEMENT_L2_CLASS`
   (`include/features/NwL2Class.h`) and a line in `tests/standalone/l2_extras.csv` (the L2 judges accept exactly
   those extra classes). Example: the craft windows (`include/features/craft/CraftWnd.h`).
7. Patching another module (WinDrv, Network, Engine) at run time: check the bytes at install and switch the
   feature off with a log line if they are not the expected ones (`WindowFix.cpp`, `ReturnToLogin.cpp`,
   `CraftBalloon.cpp`).
8. Prove it: the full cycle green, then an in-game test by the user in a test `System` folder.

## 6. Working rules

- **Stay faithful to the original first.** Reproduce, then improve in a separate, annotated change.
- **Prove every change with the judges**; never weaken a judge or a golden file to make a change pass.
- **Small commits**, one subject each, message saying what was checked.
- Do not install into the folder the user plays from; in-game tests only with the user's consent.
- Never commit game binaries (`orig/`, `*.dll`, `*.exe`; see `.gitignore`).
- Keep the address comment and the `NW_HOOK_*` together; keep `(name ours)` until something confirms a name.
- When unsure about a stock behaviour, read the assembly again rather than guessing; write down what was
  verified and what was inferred.
