# Как изменять интерфейс

[Português](MODIFICAR_INTERFACE.md) · [English](MODIFY_INTERFACE.md) · **Русский**

Практическое руководство по изменению интерфейса C1 в этих исходниках: как устроены окна, как подключены функции
фазы 6, и пошаговые рецепты (новая панель ярлыков, кнопка, окно, цвет чата, строка подсказки). Любые изменения
интерфейса делаются в сборке **`NWindowL2`** (`NW_FEATURES=1`); режим хуков и `NWindowStandalone` остаются
точной копией оригинала, и именно на них «судьи» сравнивают код с оригиналом.

## 1. Как устроены окна

### Иерархия

| Класс | Файл | Роль |
|---|---|---|
| `NCObject : UObject` | `include/nwindow/NCObject.h` | база всего; `FindTexture(TEXT("Пакет.Группа.Имя"))` загружает текстуру |
| `NCWnd` (0x94 байт) | `include/nwindow/NCWnd.h` | любое окно: позиция (`LocalX/Y/W/H` относительно родителя, `ScreenX/Y`, `Width`, `Height` абсолютные), стиль, дочерние окна, фокус, заголовок (`FrameCtrl`), иконка в свёрнутом виде |
| `NControl : NCWnd` (0x1A0) | `include/nwindow/NControl.h` | любой контрол; у него есть `ID`, и он сообщает родителю через `NWM_COMMAND` (wParam = `ID`) |
| `NCButton`, `NCPushButton`, `NCExButton`, `NCRadioButton` | `include/nwindow/NCButton.h` | кнопки с текстурами обычного/нажатого состояния (`SetNormalTex`, `SetPushedTex`, `SetText`, `SetTextID`) |
| `NCEditBox`, `NCListBox`, `NCScrollWnd`, `NCComboBox`, `NCTabControl`, `NCSliderBar`, `NCProgressCtrl`, `NCListCtrl`, `NCMultiEdit`, `NCHtmlViewer`... | `include/nwindow/` | остальные контролы |
| `NC*Wnd` | `include/nwindow/` | игровые окна (инвентарь, чат, панель ярлыков, статус...) |
| `NCVirtualWndMain` | `include/nwindow/NCVirtualWndMain.h` | владелец всех окон: регистрация, Z-порядок, проверка попадания, фокус, рассылка сообщений, курсоры |
| `NConsoleWnd : UL2ConsoleWnd` | `include/nwindow/NConsoleWnd.h` | консоль движка (285 слотов: сетевые и UI-события); `GConsole` |
| `NCConsole` | `include/nwindow/NCConsole.h` | игровой **рабочий стол** (`GConsole->GetDesktop()`): родитель всех игровых окон, с указателем на каждое (`MainWnd`, `InventoryWnd`, `ShortCutWnd`, `ChatWnd`...) |

### Создание окна или контрола

Шаблон из оригинального кода (например, `NCConsole::OnCreate` в `src/nwindow/NCConsole.cpp` или
`NCSystemMenuWnd::OnCreate` в `src/nwindow/NCSystemMenuWnd.cpp`):

```cpp
Button = new NCButton(TEXT("L2UI.Control.DefaultButton"), TEXT("L2UI.Control.DefaultButton_click"), 0);
Button->ID = NID_SOMETHING;             // то, что придёт в NWM_COMMAND родителя
Button->SetWindowRect(43, 43, 67, 19);  // относительно родителя
CreateChild(Button, NWS_VISIBLE);       // регистрирует, делает позицию абсолютной, шлёт WM_CREATE -> OnCreate
Button->SetTextID(0x91);                // текст из таблицы SysString клиента
```

- `new` для класса пакета — это `operator new` из UE2 (объект — `UObject`, выделяется с размером
  зарегистрированного `UClass`).
- `CreateChild(child, style)` (`NCWnd`, inline) = `AddChildWindow` + `Entry` + экранная позиция + `WM_CREATE`.
  Рабочий стол использует ещё и скрытый вариант `CreateHiddenChild` (`AddChildWindow`, `HideWindow`, `AttachEntry`).
- Стили (`NWS_*` в `NCWnd.h`): `NWS_VISIBLE`, `NWS_TOPMOST` (отдельный список, поверх остальных), `NWS_TITLEBAR`
  (`OnCreate` у `NCWnd` создаёт заголовок `NCFrameCtrl`), `NWS_MINIMIZABLE` (иконка при сворачивании), `NWS_NOCLOSE`.
- Уничтожение: в `OnDestroy` владельца `RemoveChildWindow(Child->Entry); delete Child;` (см.
  `NCSystemMenuWnd::OnDestroy`). `NCWnd::OnDestroy` уничтожает только заголовок и иконку.

### Сообщения, отрисовка, подсказки

- Всё приходит через `WindowProc(Msg, wParam, lParam)` (vslot 24) с номерами сообщений Win32; `NCWnd` раздаёт их
  виртуальным методам: `OnCreate` (35), `OnDestroy` (36), `OnPaint(NCGDevice*)` (50), `OnDrawToolTip` (51),
  `OnKeyDown`, `OnMouseMove`, `OnLButtonDown/Up`, `OnRButtonDown/Up`, `OnMove` (`WM_MOVE`), `OnMouseWheel`.
- Контрол сообщает родителю через `NWM_COMMAND` (`0x501`), `wParam` = `ID` контрола. Родитель обрабатывает это в
  своём `WindowProc` (пример: `NCSystemMenuWnd::WindowProc`).
- Отрисовка (`OnPaint`) идёт через `NCGDevice` (`include/nwindow/NCGDevice.h`): `SetClip`, `BeginPaint(ScreenX,
  ScreenY)`, `DrawTexture(x, y, w, h, u, v, ul, vl, Tex)`, `DrawStretchedTex`, `DrawRect`, `DrawLine`,
  `DrawText(...)`, `EndPaint`, `ReleaseClip`. Текстуры загружаются один раз в `OnCreate` через `FindTexture`
  (пакеты клиента `L2UI.*`).
- Подсказка: `OnDrawToolTip(NCGDevice*)` окна под курсором, вызывается после отрисовки.

### Консоль и рабочий стол

`NConsoleWnd` (объект, который движок создаёт по `L2.ini`) получает события сервера и игры (свои 285 слотов) и
владеет рабочим столом `NCConsole`. `NCConsole::OnCreate` создаёт все игровые окна (позиции от размера экрана), а
`NCConsole::OnDestroy` уничтожает их. `NConsoleWnd::Tick` выполняется каждый кадр.

## 2. Как подключены функции фазы 6

| Часть | Файл | Что делает |
|---|---|---|
| `NW_FEATURES` | `include/features/NwFeatures.h` | равен 1 только в `NWindowL2`; весь код функций находится в `#if NW_FEATURES` |
| `nwf::OnDllMain` | `src/features/NwFeatureStartup.cpp` | вызывается из нашего `DllMain` (`src/standalone/NWindowPackage.cpp`) при attach/detach: при attach ставит то, что меняет другие модули (WindowFix, ReturnToLogin, Discord) |
| `nwf::OnConsoleTick` | `src/features/NwFeatureTick.cpp` | первое действие `NConsoleWnd::Tick`, каждый кадр, под одним обработчиком исключений (combat log, крафт, AutoShot, ReturnToLogin, очередь bypass, CacheCleaner) |
| `nwf::IniInt/IniString/IniWriteInt/IniWriteString` | `src/features/NwFeatureConfig.cpp` | настройки: реестр `HKCU\Software\Lineage2C1\NWindow\<Раздел>`, затем вкомпилированная таблица `GBuiltIn`, затем значение по умолчанию |
| `nwf::Log(tag, ...)` | `src/features/NwFeatureConfig.cpp` | `LineageWindow.log`, только с `-DNW_FEATURE_LOG=ON` |
| `nwf::SendBypassToServer`, `SendShortCutReg`, `SendShortCutDel` | `include/features/ManufactureCmd.h` | запросы к серверу по стандартному протоколу (bypass через очередь, один раз в 120 мс) |
| `NW_DECLARE_L2_CLASS` / `NW_IMPLEMENT_L2_CLASS` | `include/features/NwL2Class.h` | `UClass`, существующие только в L2, для окон со своими полями; каждый — строка в `tests/standalone/l2_extras.csv` |

Точки вызова в декомпилированном коде (всегда `#if NW_FEATURES` + `// phase 6: ...`):

| Функция | Где подключена |
|---|---|
| AutoShot | `NCShortCutWnd.cpp` (`OnRButtonDown`, `OnLButtonUp`, `OnPaint`), `NConsoleWndEvents.cpp` (сообщения с данными в `NpcHtmlMessage`) |
| BuffTimer | `NCAbnormalStatusWnd.cpp` (отсчёт на иконках, строка в подсказке) |
| ChatColors | `NConsoleWndEvents.cpp`, `NConsoleWnd::Say2` (последний `else`) |
| CombatLog | `NCChatWnd.cpp` (уводит боевые строки) + тик |
| ItemHoverTip | `OnDrawToolTip` в `NCItemWnd.cpp`, `NCEquipItemWnd.cpp`, `NCShortCutWnd.cpp` |
| ManufactureCmd | `NCommandParser.cpp` (`/dwarvenmanufacture`), `NCActionItemWnd.cpp` и `NCShortCutWnd.cpp` (действия 32..52) |
| MinimapHotkey | `NConsoleWndInput.cpp` (Alt+M / M) |
| ReturnToLogin | `NCDialogBox.cpp`, `NConsoleWndLogin.cpp` + тик + `DllMain` |
| WindowFix | `NCVirtualWndMain.cpp`, `NCWnd.cpp`, `NConsoleWndInput.cpp` (позиция мыши), `NCVideoOptionWnd.cpp`, `NConsoleWndWindows.cpp` (полноэкранный режим) + `DllMain` |
| Окна крафта | создаются по требованию на рабочем столе (`src/features/craft/CraftUi.cpp`: `new NCRecipe...Wnd`, `Create(GConsole->Desktop)`) + тик |

Заметки по каждой функции — в `docs/notas/F6_*.md` (на португальском).

## 3. Рецепт: новая панель ярлыков (как в C5 / Interlude)

Этот рецепт описывает подход, который уже был реализован и **проверен в игре** в отдельном эксперименте (ветка
`experiment/extra-shortcut-bars` репозитория разработки; в эти исходники она не входит). Исследование — в
`docs/notas/BarrasExtras_Atalhos.md`.

**Идея (как в C5):** у дополнительной панели **нет своих слотов**. Это ещё один экземпляр `NCShortCutWnd`, который
показывает другую страницу той же таблицы из 100 ярлыков (10 страниц по 10). Слот ярлыка — это
`страница × 10 + позиция`, плоское число, которое сервер хранит и возвращает; панель, которая лишь показывает
другую страницу, ничего не меняет ни в протоколе, ни на сервере, ни в текстурах. Размер класса не меняется, новый
`UClass` не нужен.

**Шаги:**

1. **Файлы функции:** `include/features/ShortCutBars.h` и `src/features/ShortCutBars.cpp`, в `namespace nwf` и
   `#if NW_FEATURES`, с функциями вроде `ShortCutBars_Create(NCWnd* Desktop, NCShortCutWnd* Main)`,
   `ShortCutBars_Destroy`, `ShortCutBars_OnTick`, `ShortCutBars_OnCommand`, `ShortCutBars_OnMove`,
   `ShortCutBars_OnDropOnOtherBar`, `ShortCutBars_OnPaint`. Состояние (панели, кнопка) хранится в глобальных
   переменных `.cpp`, а не в новых полях `NCShortCutWnd`.
2. **Создание** в `NCConsole::OnCreate`, сразу после создания основной панели (`ShortCutWnd = new
   NCShortCutWnd(10, 10, 32)` ... `CreateHiddenChild(...)`): для каждой дополнительной панели
   `new NCShortCutWnd(Main->Cols, Main->Pages, Main->IconSize)`, `SetWindowRect` и та же регистрация, что делает
   рабочий стол (`AddChildWindow` с `NWS_VISIBLE | 0x20 | NWS_TOPMOST`, `HideWindow`, `AttachEntry`). Стандартный
   `OnCreate` создал собственные таблицы: освободите их (`delete[] Bar->Items`, `delete[] Bar->Keys`) и укажите на
   таблицы основной панели (`Bar->Items = Main->Items`, `Bar->Keys = Main->Keys`). `Bar->Page` — начальная
   страница (в C5 панель 2 начинается со страницы 2, панель 3 — со страницы 3). Скройте `RotateButton` у
   дополнительных панелей (поворачивает их все основная).
3. **Уничтожение** в `NCConsole::OnDestroy`, **до** `DestroyChild(this, ShortCutWnd)`: у дополнительных панелей
   обнулите `Items` и `Keys` (они принадлежат основной; иначе стандартный `OnDestroy` освободит их дважды), затем
   `RemoveChildWindow(Bar->Entry)` и `delete Bar`.
4. **Следование за основной панелью** в конце `NCShortCutWnd::Tick` (тик есть только у основной): у каждой
   дополнительной та же ориентация (`bHorizontal`, и `UpdateLayout()` при смене), позиция (над горизонтальной,
   слева от вертикальной, с другой стороны, если не хватает экрана; 44 × 417 каждая) через `SetWindowRect`, и
   видимость только вместе с основной.
5. **Кнопка раскрытия** (по желанию): `NCPushButton` — дочерний элемент основной панели (`CreateChild`), с новым
   `ID` после стандартных (у панели они заканчиваются на `0xFF000008`, `NID_SHORTCUT_ROTATE`), обрабатывается в
   ветке `NWM_COMMAND` метода `NCShortCutWnd::WindowProc`. Своей графики для неё в C1 нет: эксперимент использовал
   стрелки страниц (`L2UI.ShortcutWnd.UpBtn1/2`, `LeftBtn1/2`...). Количество видимых панелей сохраняйте через
   `IniWriteInt`.
6. **Перетаскивание между панелями:** в `NCShortCutWnd::OnLButtonUp` стандартная ветка, которая **удаляет** ярлык,
   брошенный вне панели (`RequestShortCutDel`), сначала проверяет, не над другой ли панелью точка; если да —
   переносит ярлык в слот там (`SlotAt`) через `SendShortCutReg`/`SendShortCutDel` (обмен, если слот занят), а
   брошенный на панель, но не на слот — оставляет как есть.
7. **Перетаскивание за ручку дополнительной панели двигает всю группу:** в начале `NCShortCutWnd::WindowProc`
   `WM_MOVE` дополнительной панели передаётся основной. Перед этим запишите в `FrameCtrl->GrabX/GrabY` основной
   панели точку захвата дополнительной плюс её смещение относительно основной (по рассчитанной раскладке, а не по
   текущим позициям, которые отстают на кадр); без этого `NCWnd::Snap` тянет группу, и мышь «прыгает».
8. **Графика:** фоны `L2UI.ShortcutWnd.Back` (вертикальный) и `.Back1` (горизонтальный) уже содержат нарисованную
   в текстуре иконку поворота. У дополнительных панелей кнопки поверх неё нет: в конце `NCShortCutWnd::OnPaint`
   закройте эту ячейку копией 1:1 соседней пустой ячейки той же текстуры (`DrawTexture` между
   `BeginPaint`/`EndPaint`).

**Полезно знать:** подсказка, использование по клику, AutoShot и перетаскивание из инвентаря/умений работают без
изменений, потому что это тот же класс и всё вычисляется от `Page`. Клавиши (F1..F10, страницы, джойпад) остаются
только у основной панели, как в C5. Фильтр пакетов сервера может отбрасывать слишком частые регистрации ярлыков.
Проблемы позиции и графики видны только в игре: после первого теста эксперименту понадобились три исправления
(иконка в графике, ручка перетаскивания, точка захвата).

## 4. Рецепт: новая кнопка в существующем окне

Пример: ещё одна кнопка в системном меню (`NCSystemMenuWnd`).

1. `include/features/MyButton.h` / `src/features/MyButton.cpp` (`namespace nwf`, `#if NW_FEATURES`):
   `MyButton_Create(NCWnd* Owner)`, `MyButton_OnCommand(NCWnd* Owner, WPARAM ID)`, `MyButton_Destroy(NCWnd* Owner)`;
   указатель на кнопку хранится в глобальной переменной `.cpp`.
2. Создайте её в конце `NCSystemMenuWnd::OnCreate`:
   `#if NW_FEATURES` / `// phase 6: MyButton ...` / `nwf::MyButton_Create(this);` / `#endif`. Внутри:
   `new NCButton(normal, pushed, 0)`, `ID`, который не использует ни один контрол этого окна (см. его enum `NID_*`),
   `SetWindowRect` в пределах окна, `Owner->CreateChild(Button, NWS_VISIBLE)`, `SetText`.
3. Обработайте клик в ветке `NWM_COMMAND` метода `NCSystemMenuWnd::WindowProc`:
   `if (nwf::MyButton_OnCommand(this, wParam)) return 1;` (в `#if NW_FEATURES`).
4. Уничтожьте её в `NCSystemMenuWnd::OnDestroy`, до `NCWnd::OnDestroy()`: `RemoveChildWindow(Button->Entry);
   delete Button;`.
5. Если окно нужно увеличить, измените его `SetWindowRect` в `NCConsole::OnCreate` и отрисовку фона в его
   `OnPaint` — всегда в `#if NW_FEATURES`, не трогая стандартную ветку.

## 5. Рецепт: новое окно

1. Окно со своими полями или виртуальными методами — это новый класс, зарегистрированный только в L2:
   `class NCMyWnd : public NCWnd { NW_DECLARE_L2_CLASS(NCMyWnd, NCWnd, 0) ... };` в заголовке и
   `NW_IMPLEMENT_L2_CLASS(NCMyWnd, NCWnd, 0)` в `.cpp` (`include/features/NwL2Class.h`).
2. Добавьте строку в `tests/standalone/l2_extras.csv` (`class,super,class_flags,feature`): `verify_standalone.py
   --l2` и `smoke_standalone.py --l2` принимают **ровно** эти дополнительные классы.
3. Переопределите нужные виртуальные методы: `OnCreate` (сначала `NCWnd::OnCreate()`, затем текстуры через
   `FindTexture`, заголовок и контролы), `OnDestroy` (уничтожить дочерние окна, затем `NCWnd::OnDestroy()`),
   `WindowProc` (`NWM_COMMAND`), `OnPaint`.
4. Создавайте окно на рабочем столе, когда оно нужно (окна крафта делают это по требованию:
   `SetWindowRect(0, 0, W, H)`, `GConsole->Desktop` как родитель в `CreateChild(this, NWS_VISIBLE)`,
   `HideWindow()`), а показывайте через `ShowWindow()` + `Desktop->BringChildToTop(Entry)`.
5. Полный образец: `include/features/craft/CraftWnd.h`, `src/features/craft/CraftWnd.cpp` (`NCRecipeWnd`:
   оформленный заголовок `NCFrameCtrl(1)`, кнопки, прокручиваемые списки, подсказка, позиция в реестре).

## 6. Рецепт: новый цвет чата

Сервер не присылает цвет: он присылает **тип** сообщения (`Say2`), а цвет выбирает клиент в `NConsoleWnd::Say2`
(`src/nwindow/NConsoleWndEvents.cpp`). Типы, которых C1 не знает, попадают в последний `else` (белый), где фаза 6
спрашивает `nwf::ChatColors_Find(Type, &Color)`.

1. Добавьте тип и цвет (ARGB, например `0xFF408CFF`) в `switch` файла `src/features/ChatColors.cpp`.
2. Строка по-прежнему попадает в чат как обычная (вкладка All, фильтр Normal). Не используйте типы с побочными
   эффектами (TELL добавляет автора в список личных сообщений).
3. Изменить цвет типа, который C1 **уже** раскрашивает, — значит изменить точную ветку: делайте это под
   `#if NW_FEATURES` внутри `Say2`, сохранив оригинальный цвет в ветке `#else`.
4. Подробности и цвета Interlude/Essence: `docs/notas/F6_ChatColors.md`, `docs/notas/ChatColors_Essence.md`.

## 7. Рецепт: новая строка в подсказке предмета

Полную подсказку предмета рисуют функции из `include/nwindow/NItemToolTip.h`, которые подключают сетки предметов:
`SetTexts`/`SetStoreTexts` заполняют `ToolTipName` и `ToolTipText` (текст, по которому **измеряется** рамка), а
`DrawLines` рисует строки с шагом 13 пикселей (`DrawTypeLines`, затем `DrawTailLines`, который заканчивается
описанием).

1. Добавьте строку в оба места, в `#if NW_FEATURES`: в измеряемый текст (чтобы рамка выросла) и в отрисовку
   (например, в `DrawTailLines` перед описанием, через `DrawLabel`/`DrawValue` и `Y += 13`).
2. Эти функции — шаблоны, которые каждая сетка инстанцирует со своими параметрами (инвентарь, paperdoll, обмен,
   склад, личные магазины): проверьте каждую копию, которую меняете.
3. Для подсказок других окон образец — BuffTimer (`nwf::BuffTimer_ToolTipExtraHeight` и
   `BuffTimer_DrawToolTipTime`, вызываются из `NCAbnormalStatusWnd.cpp`): один хук даёт дополнительную высоту,
   другой рисует строку. Короткая подсказка при наведении — это `ItemHoverTip` (`F6_ItemHoverTip.md`).

## 8. Цикл сборки и проверки

```bat
cmake --build build --config Release --target NWindowL2
python tools\cycle.py --system "C:\...\Lineage II\System - Test" --no-install
```

1. Собирайте `NWindowL2`, пока не исчезнут ошибки и новые предупреждения.
2. Запустите полный цикл: он доказывает, что режим хуков и точный standalone не изменились (судьи сравнивают их с
   оригиналом), а `NWindowL2` проходит `verify_standalone --l2` и `smoke_standalone --l2`.
3. Проверка в игре: закройте игру, сохраните `NWindow.dll` **копии** папки `System` и скопируйте поверх него
   `build\l2\Release\NWindow.dll`. Цикл не рисует интерфейс: позиция, графика и перетаскивание видны только в игре.
4. Для диагностики соберите с `-DNW_FEATURE_LOG=ON` и читайте `LineageWindow.log` рядом с DLL.

## 9. Правила безопасности

- Никогда не добавляйте поля и новые виртуальные слоты в оригинальный класс: его размер зарегистрирован в движке,
  и оригинальный код обращается к нему и к его vtable по смещениям. Переопределять уже унаследованный слот можно
  (AutoShot переопределяет `OnRButtonDown` в `NCShortCutWnd` под `#if NW_FEATURES`). Состояние функции хранится в
  глобальных переменных `.cpp` или в новом L2-классе.
- В декомпилированном коде — только точка вызова, в `#if NW_FEATURES` с `// phase 6: ...`; стандартная ветка
  остаётся как есть.
- Любой патч другого модуля во время работы сначала проверяет байты и, если они не совпадают, отключает функцию с
  записью в лог.
- Настройки — через `nwf::Ini*` (реестр), никаких файлов рядом с DLL.
- Протокол — как у оригинального клиента: сервер не меняется; учитывайте лимиты его фильтра пакетов.
- Проверяйте на копии клиента, а не в папке, из которой играете; никакие бинарники игры не попадают в репозиторий.
- Небольшие коммиты, по одной теме, с «зелёным» циклом.
