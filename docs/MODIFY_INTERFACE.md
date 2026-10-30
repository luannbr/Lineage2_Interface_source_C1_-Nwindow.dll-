# How to modify the interface

[Português](MODIFICAR_INTERFACE.md) · **English** · [Русский](MODIFY_INTERFACE.ru.md)

A practical guide to changing the C1 interface in this source: how the windows are built, how the phase-6
features are wired, and step-by-step recipes (a new shortcut bar, a button, a window, a chat colour, a tool-tip
line). Every interface change goes into the **`NWindowL2`** build (`NW_FEATURES=1`); hook mode and
`NWindowStandalone` stay faithful to the original and are where the judges compare with it.

## 1. How the windows are built

### The hierarchy

| Class | File | Role |
|---|---|---|
| `NCObject : UObject` | `include/nwindow/NCObject.h` | base of everything; `FindTexture(TEXT("Package.Group.Name"))` loads a texture |
| `NCWnd` (0x94 bytes) | `include/nwindow/NCWnd.h` | every window: position (`LocalX/Y/W/H` relative to the parent, `ScreenX/Y`, `Width`, `Height` absolute), style, children, focus, title bar (`FrameCtrl`), minimised icon |
| `NControl : NCWnd` (0x1A0) | `include/nwindow/NControl.h` | every control; has an `ID` and tells its parent with `NWM_COMMAND` (wParam = `ID`) |
| `NCButton`, `NCPushButton`, `NCExButton`, `NCRadioButton` | `include/nwindow/NCButton.h` | buttons with normal/pushed textures (`SetNormalTex`, `SetPushedTex`, `SetText`, `SetTextID`) |
| `NCEditBox`, `NCListBox`, `NCScrollWnd`, `NCComboBox`, `NCTabControl`, `NCSliderBar`, `NCProgressCtrl`, `NCListCtrl`, `NCMultiEdit`, `NCHtmlViewer`... | `include/nwindow/` | the other controls |
| `NC*Wnd` | `include/nwindow/` | the game windows (inventory, chat, shortcut bar, status...) |
| `NCVirtualWndMain` | `include/nwindow/NCVirtualWndMain.h` | owner of every window: registration, z-order, hit tests, focus, message dispatch, cursors |
| `NConsoleWnd : UL2ConsoleWnd` | `include/nwindow/NConsoleWnd.h` | the engine's console (285 slots: network and UI events); `GConsole` |
| `NCConsole` | `include/nwindow/NCConsole.h` | the in-game **desktop** (`GConsole->GetDesktop()`): parent of every in-game window, with a pointer to each (`MainWnd`, `InventoryWnd`, `ShortCutWnd`, `ChatWnd`...) |

### Creating a window or a control

The pattern of the original code (e.g. `NCConsole::OnCreate` in `src/nwindow/NCConsole.cpp`, or
`NCSystemMenuWnd::OnCreate` in `src/nwindow/NCSystemMenuWnd.cpp`):

```cpp
Button = new NCButton(TEXT("L2UI.Control.DefaultButton"), TEXT("L2UI.Control.DefaultButton_click"), 0);
Button->ID = NID_SOMETHING;             // what arrives in the parent's NWM_COMMAND
Button->SetWindowRect(43, 43, 67, 19);  // relative to the parent
CreateChild(Button, NWS_VISIBLE);       // registers it, makes the position absolute, sends WM_CREATE -> OnCreate
Button->SetTextID(0x91);                // text from the client's SysString table
```

- `new` of a package class is UE2's `operator new` (the object is a `UObject`, allocated with the size of the
  registered `UClass`).
- `CreateChild(child, style)` (`NCWnd`, inline) = `AddChildWindow` + `Entry` + screen position + `WM_CREATE`. The
  desktop also uses the hidden variant `CreateHiddenChild` (`AddChildWindow`, `HideWindow`, `AttachEntry`).
- Styles (`NWS_*` in `NCWnd.h`): `NWS_VISIBLE`, `NWS_TOPMOST` (separate list, on top), `NWS_TITLEBAR` (`NCWnd`'s
  `OnCreate` makes an `NCFrameCtrl` title bar), `NWS_MINIMIZABLE` (icon when minimised), `NWS_NOCLOSE`.
- Destroying: in the owner's `OnDestroy`, `RemoveChildWindow(Child->Entry); delete Child;` (see
  `NCSystemMenuWnd::OnDestroy`). `NCWnd::OnDestroy` only destroys the title bar and the icon.

### Messages, painting, tool tips

- Everything arrives through `WindowProc(Msg, wParam, lParam)` (vslot 24) with Win32 message numbers; `NCWnd`
  dispatches to the virtuals: `OnCreate` (35), `OnDestroy` (36), `OnPaint(NCGDevice*)` (50), `OnDrawToolTip`
  (51), `OnKeyDown`, `OnMouseMove`, `OnLButtonDown/Up`, `OnRButtonDown/Up`, `OnMove` (`WM_MOVE`), `OnMouseWheel`.
- A control tells its parent with `NWM_COMMAND` (`0x501`), `wParam` = the control's `ID`. The parent handles it in
  its `WindowProc` (example: `NCSystemMenuWnd::WindowProc`).
- Painting (`OnPaint`) uses the `NCGDevice` (`include/nwindow/NCGDevice.h`): `SetClip`, `BeginPaint(ScreenX,
  ScreenY)`, `DrawTexture(x, y, w, h, u, v, ul, vl, Tex)`, `DrawStretchedTex`, `DrawRect`, `DrawLine`,
  `DrawText(...)`, `EndPaint`, `ReleaseClip`. Textures are loaded once in `OnCreate` with `FindTexture` (the
  client's `L2UI.*` packages).
- Tool tip: `OnDrawToolTip(NCGDevice*)` of the window under the mouse, called after painting.

### Console and desktop

`NConsoleWnd` (the object the engine creates from `L2.ini`) receives the server and game events (its 285 slots)
and owns the `NCConsole` desktop. `NCConsole::OnCreate` creates every game window (positions from the screen
size) and `NCConsole::OnDestroy` destroys them. `NConsoleWnd::Tick` runs every frame.

## 2. How the phase-6 features are wired

| Piece | File | What it does |
|---|---|---|
| `NW_FEATURES` | `include/features/NwFeatures.h` | only `NWindowL2` defines it as 1; all feature code sits in `#if NW_FEATURES` |
| `nwf::OnDllMain` | `src/features/NwFeatureStartup.cpp` | called by our `DllMain` (`src/standalone/NWindowPackage.cpp`) at attach/detach: at attach, installs what touches other modules (WindowFix, ReturnToLogin, Discord) |
| `nwf::OnConsoleTick` | `src/features/NwFeatureTick.cpp` | first thing in `NConsoleWnd::Tick`, every frame, under a single exception handler (combat log, craft, AutoShot, ReturnToLogin, bypass queue, CacheCleaner) |
| `nwf::IniInt/IniString/IniWriteInt/IniWriteString` | `src/features/NwFeatureConfig.cpp` | settings: registry `HKCU\Software\Lineage2C1\NWindow\<Section>`, then the compiled-in `GBuiltIn` table, then the default |
| `nwf::Log(tag, ...)` | `src/features/NwFeatureConfig.cpp` | `LineageWindow.log`, only with `-DNW_FEATURE_LOG=ON` |
| `nwf::SendBypassToServer`, `SendShortCutReg`, `SendShortCutDel` | `include/features/ManufactureCmd.h` | requests to the server through the stock protocol (bypass in a queue, one every 120 ms) |
| `NW_DECLARE_L2_CLASS` / `NW_IMPLEMENT_L2_CLASS` | `include/features/NwL2Class.h` | L2-only `UClass`es, for windows with fields of their own; each one is a line of `tests/standalone/l2_extras.csv` |

Call sites in the decompiled code (always `#if NW_FEATURES` + `// phase 6: ...`):

| Feature | Where it hooks in |
|---|---|
| AutoShot | `NCShortCutWnd.cpp` (`OnRButtonDown`, `OnLButtonUp`, `OnPaint`), `NConsoleWndEvents.cpp` (data messages in `NpcHtmlMessage`) |
| BuffTimer | `NCAbnormalStatusWnd.cpp` (countdown on the icons, tool-tip line) |
| ChatColors | `NConsoleWndEvents.cpp`, `NConsoleWnd::Say2` (the last `else`) |
| CombatLog | `NCChatWnd.cpp` (diverts the combat lines) + the tick |
| ItemHoverTip | `OnDrawToolTip` of `NCItemWnd.cpp`, `NCEquipItemWnd.cpp`, `NCShortCutWnd.cpp` |
| ManufactureCmd | `NCommandParser.cpp` (`/dwarvenmanufacture`), `NCActionItemWnd.cpp` and `NCShortCutWnd.cpp` (actions 32..52) |
| MinimapHotkey | `NConsoleWndInput.cpp` (Alt+M / M) |
| ReturnToLogin | `NCDialogBox.cpp`, `NConsoleWndLogin.cpp` + the tick + `DllMain` |
| WindowFix | `NCVirtualWndMain.cpp`, `NCWnd.cpp`, `NConsoleWndInput.cpp` (mouse position), `NCVideoOptionWnd.cpp`, `NConsoleWndWindows.cpp` (full screen) + `DllMain` |
| Craft windows | created on demand on the desktop (`src/features/craft/CraftUi.cpp`: `new NCRecipe...Wnd`, `Create(GConsole->Desktop)`) + the tick |

Each feature's notes are in `docs/notas/F6_*.md` (Portuguese).

## 3. Recipe: a new shortcut bar (as in C5 / Interlude)

This recipe describes the approach that was already implemented and **tested in the game** in a separate
experiment (branch `experiment/extra-shortcut-bars` of the development repository; it is not in this source). The
study is `docs/notas/BarrasExtras_Atalhos.md`.

**The idea (C5's):** the extra bar **owns no slots**. It is one more `NCShortCutWnd` instance showing another page
of the same table of 100 shortcuts (10 pages of 10). A shortcut's slot is `page × 10 + position`, a flat number the
server stores and sends back; a bar that only shows another page changes nothing in the protocol, the server or
the textures. The class keeps its size and no new `UClass` is needed.

**Steps:**

1. **Feature files:** `include/features/ShortCutBars.h` and `src/features/ShortCutBars.cpp`, in `namespace nwf`
   and `#if NW_FEATURES`, with functions such as `ShortCutBars_Create(NCWnd* Desktop, NCShortCutWnd* Main)`,
   `ShortCutBars_Destroy`, `ShortCutBars_OnTick`, `ShortCutBars_OnCommand`, `ShortCutBars_OnMove`,
   `ShortCutBars_OnDropOnOtherBar`, `ShortCutBars_OnPaint`. The state (the bars, the button) lives in globals of
   the `.cpp`, never in new fields of `NCShortCutWnd`.
2. **Create** in `NCConsole::OnCreate`, right after the main bar is made (`ShortCutWnd = new NCShortCutWnd(10, 10,
   32)` ... `CreateHiddenChild(...)`): for each extra bar, `new NCShortCutWnd(Main->Cols, Main->Pages,
   Main->IconSize)`, `SetWindowRect`, and the same registration the desktop does (`AddChildWindow` with
   `NWS_VISIBLE | 0x20 | NWS_TOPMOST`, `HideWindow`, `AttachEntry`). The stock `OnCreate` made tables of its own:
   free them (`delete[] Bar->Items`, `delete[] Bar->Keys`) and point to the main bar's (`Bar->Items =
   Main->Items`, `Bar->Keys = Main->Keys`). `Bar->Page` = the first page shown (in C5 bar 2 starts on page 2 and
   bar 3 on page 3). Hide the extra bars' `RotateButton` (the main bar turns them all).
3. **Destroy** in `NCConsole::OnDestroy`, **before** `DestroyChild(this, ShortCutWnd)`: on the extra bars set
   `Items` and `Keys` to `NULL` (they belong to the main bar; otherwise the stock `OnDestroy` frees them twice),
   then `RemoveChildWindow(Bar->Entry)` and `delete Bar`.
4. **Follow the main bar** at the end of `NCShortCutWnd::Tick` (only the main bar ticks): for each extra bar, the
   same orientation (`bHorizontal`, and `UpdateLayout()` when it changes), the position (above a horizontal bar,
   left of a vertical one, the other side when the screen has no room; 44 × 417 each) through `SetWindowRect`, and
   shown only while the main bar is.
5. **Expand button** (optional): an `NCPushButton` child of the main bar (`CreateChild`), with a new `ID` after the
   stock ones (the bar's go up to `0xFF000008`, `NID_SHORTCUT_ROTATE`), handled in the `NWM_COMMAND` branch of
   `NCShortCutWnd::WindowProc`. C1 has no art for it: the experiment reused the page arrows
   (`L2UI.ShortcutWnd.UpBtn1/2`, `LeftBtn1/2`...). Keep how many bars are shown with `IniWriteInt`.
6. **Dragging between bars:** in `NCShortCutWnd::OnLButtonUp`, the stock branch that **deletes** a shortcut
   dropped off the bar (`RequestShortCutDel`) first checks whether the point is over another bar; if so it moves
   the shortcut to the slot there (`SlotAt`) with `SendShortCutReg`/`SendShortCutDel` (a swap when the slot is
   taken), and a drop on a bar but not on a slot leaves it alone.
7. **Dragging an extra bar's handle moves the group:** at the start of `NCShortCutWnd::WindowProc`, a `WM_MOVE` of
   an extra bar is passed on to the main bar. Before that, give the main bar's `FrameCtrl->GrabX/GrabY` the extra
   bar's grab point plus its offset from the main bar (from the computed layout, not the current positions, which
   lag a frame); without it `NCWnd::Snap` pulls the group and the mouse "jumps".
8. **Art:** the backgrounds `L2UI.ShortcutWnd.Back` (vertical) and `.Back1` (horizontal) have the turn button's
   icon painted into the texture. The extra bars have no button over it: at the end of `NCShortCutWnd::OnPaint`,
   cover that cell with a 1:1 copy of the empty cell next to it in the same texture (`DrawTexture` between
   `BeginPaint`/`EndPaint`).

**Good to know:** tool tip, click to use, AutoShot and drag from the inventory/skills work unchanged, because it is
the same class and everything is computed from `Page`. The keys (F1..F10, pages, game pad) stay on the main bar,
as in C5. The server's packet filter may drop shortcut registrations sent too fast. Only the game shows position
and art problems: the experiment needed three fixes after the first test (the icon in the art, the drag handle,
the grab point).

## 4. Recipe: a new button in an existing window

Example: one more button in the system menu (`NCSystemMenuWnd`).

1. `include/features/MyButton.h` / `src/features/MyButton.cpp` (`namespace nwf`, `#if NW_FEATURES`):
   `MyButton_Create(NCWnd* Owner)`, `MyButton_OnCommand(NCWnd* Owner, WPARAM ID)`, `MyButton_Destroy(NCWnd* Owner)`;
   the button pointer lives in a global of the `.cpp`.
2. Create it at the end of `NCSystemMenuWnd::OnCreate`:
   `#if NW_FEATURES` / `// phase 6: MyButton ...` / `nwf::MyButton_Create(this);` / `#endif`. Inside:
   `new NCButton(normal, pushed, 0)`, an `ID` no control of that window uses (see its `NID_*` enum),
   `SetWindowRect` inside the window's area, `Owner->CreateChild(Button, NWS_VISIBLE)`, `SetText`.
3. Handle the click in the `NWM_COMMAND` branch of `NCSystemMenuWnd::WindowProc`:
   `if (nwf::MyButton_OnCommand(this, wParam)) return 1;` (in `#if NW_FEATURES`).
4. Destroy it in `NCSystemMenuWnd::OnDestroy`, before `NCWnd::OnDestroy()`: `RemoveChildWindow(Button->Entry);
   delete Button;`.
5. If the window has to grow, change its `SetWindowRect` in `NCConsole::OnCreate` and the background painting in
   its `OnPaint` — always in `#if NW_FEATURES`, leaving the stock branch intact.

## 5. Recipe: a new window

1. A window with fields or virtuals of its own is a new class, registered only in the L2 build:
   `class NCMyWnd : public NCWnd { NW_DECLARE_L2_CLASS(NCMyWnd, NCWnd, 0) ... };` in the header and
   `NW_IMPLEMENT_L2_CLASS(NCMyWnd, NCWnd, 0)` in the `.cpp` (`include/features/NwL2Class.h`).
2. Add its line to `tests/standalone/l2_extras.csv` (`class,super,class_flags,feature`): `verify_standalone.py
   --l2` and `smoke_standalone.py --l2` accept **exactly** those extra classes.
3. Override the virtuals you need: `OnCreate` (call `NCWnd::OnCreate()` first, load textures with `FindTexture`,
   create the title bar and the controls), `OnDestroy` (destroy the children, then `NCWnd::OnDestroy()`),
   `WindowProc` (`NWM_COMMAND`), `OnPaint`.
4. Create it on the desktop when needed (the craft windows do it on demand: `SetWindowRect(0, 0, W, H)`,
   `GConsole->Desktop` as the parent in `CreateChild(this, NWS_VISIBLE)`, `HideWindow()`), and show it with
   `ShowWindow()` + `Desktop->BringChildToTop(Entry)`.
5. Full model: `include/features/craft/CraftWnd.h`, `src/features/craft/CraftWnd.cpp` (`NCRecipeWnd`: decorated
   title bar `NCFrameCtrl(1)`, buttons, scrolling lists, tool tip, position kept in the registry).

## 6. Recipe: a new chat colour

The server sends no colour: it sends the chat **type** (`Say2`), and the client picks the colour in
`NConsoleWnd::Say2` (`src/nwindow/NConsoleWndEvents.cpp`). Types C1 does not know fall into the last `else`
(white), where phase 6 asks `nwf::ChatColors_Find(Type, &Color)`.

1. Add the type and the colour (ARGB, e.g. `0xFF408CFF`) to the `switch` in `src/features/ChatColors.cpp`.
2. The line still goes to the chat as a normal one (All tab, Normal filter). Do not reuse types with side effects
   (TELL adds the speaker to the tell list).
3. Changing the colour of a type C1 **already** colours means changing a faithful branch: do it under
   `#if NW_FEATURES` inside `Say2`, keeping the original colour in the `#else` branch.
4. Details and the Interlude/Essence colours: `docs/notas/F6_ChatColors.md`, `docs/notas/ChatColors_Essence.md`.

## 7. Recipe: a new line in the item tool tip

The full item tool tip is drawn by the routines of `include/nwindow/NItemToolTip.h`, which the item grids include:
`SetTexts`/`SetStoreTexts` fill `ToolTipName` and `ToolTipText` (the text that **sizes** the box) and `DrawLines`
draws the lines, 13 pixels apart (`DrawTypeLines`, then `DrawTailLines`, which ends with the description).

1. Add the line in both places, in `#if NW_FEATURES`: in the sizing text (so the box grows) and in the drawing (for
   example in `DrawTailLines`, before the description, with `DrawLabel`/`DrawValue` and `Y += 13`).
2. The routines are templates instantiated by each grid with different parameters (inventory, paperdoll, trade,
   warehouse, private stores): check every copy you change.
3. For other windows' tool tips the model is BuffTimer (`nwf::BuffTimer_ToolTipExtraHeight` and
   `BuffTimer_DrawToolTipTime`, called from `NCAbnormalStatusWnd.cpp`): one hook gives the extra height, the other
   draws the line. The short hover tip is `ItemHoverTip` (`F6_ItemHoverTip.md`).

## 8. The build and test loop

```bat
cmake --build build --config Release --target NWindowL2
python tools\cycle.py --system "C:\...\Lineage II\System - Test" --no-install
```

1. Build `NWindowL2` until there is no error and no new warning.
2. Run the full cycle: it proves hook mode and the faithful standalone did not change (the judges compare them with
   the original) and that `NWindowL2` passes `verify_standalone --l2` and `smoke_standalone --l2`.
3. Test in the game: close the game, keep the `NWindow.dll` of a **copy** of the `System` folder aside and copy
   `build\l2\Release\NWindow.dll` over it. The cycle does not draw the interface: position, art and dragging only
   show in the game.
4. To diagnose, build with `-DNW_FEATURE_LOG=ON` and read `LineageWindow.log` next to the DLL.

## 9. Safety rules

- Never add fields or new virtual slots to an original class: its size is registered with the engine and the
  original code indexes into it and its vtable. Overriding a slot the class already inherits is fine (AutoShot
  overrides `OnRButtonDown` in `NCShortCutWnd`, under `#if NW_FEATURES`). Feature state goes into globals of the
  `.cpp` or into a new L2 class.
- In decompiled code, only the call site, in `#if NW_FEATURES` with `// phase 6: ...`; the stock branch stays as
  it is.
- Any run-time patch of another module checks the bytes first and switches the feature off with a log line when
  they do not match.
- Settings through `nwf::Ini*` (registry), never files next to the DLL.
- Same protocol as the original client: the server does not change; respect its packet filter limits.
- Test on a copy of the client, never in the folder you play from; no game binary goes into the repository.
- Small commits, one subject each, with the cycle green.
