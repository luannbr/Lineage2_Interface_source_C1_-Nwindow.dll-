# NWindow.dll для Lineage II C1 — исходный код, восстановленный на C++

[Português](README.md) · [English](README.en.md) · **Русский**

Пофункциональное восстановление `NWindow.dll` клиента Lineage II **Chronicle 1 (Harbingers of War)** —
нативной DLL интерфейса (окна, контролы, HTML диалогов NPC, IME, чат, консоль). Цель — оригинальная сборка
от 25.06.2004 (TimeDateStamp `0x40DBD452`, Visual C++ .NET 2002, md5 `2dacb6eb…`). Один и тот же исходный
код собирается тремя способами:

| Цель CMake | Результат | Что это |
|---|---|---|
| `NWindowDecomp` | `build/bin/Release/NWindowDecomp.dll` | **режим хуков**: работает рядом с оригинальным `NWindow.dll` и заменяет каждую уже восстановленную функцию переходом `jmp` на нашу |
| `NWindowStandalone` | `build/standalone/Release/NWindow.dll` | **точный standalone**: весь `NWindow.dll` только из нашего кода; эталон, с которым все тесты сравнивают оригинал |
| `NWindowL2` | `build/l2/Release/NWindow.dll` | **standalone + функции фазы 6** (DLL, которую получают игроки): тот же код с `NW_FEATURES=1` и `src/features/` |

## Как это работает

**Режим хуков.** Установщик (`tools/install_test.py`) добавляет в оригинальный `NWindow.dll` импорт
`NWindowDecomp.dll`. Наша DLL загружается раньше, чем выполняется любой код оригинала, и ставит `jmp` на вход
каждой восстановленной функции (макрос `NW_HOOK_*` рядом с каждым определением, `src/re/NwHook.h`). Всё
остальное остаётся оригинальным кодом. Так каждая новая функция проверяется в настоящей игре, а любую из них
можно отключить в `NWindowDecomp.ini` (`[Hooks] Disable=`), чтобы локализовать ошибку.

**Standalone (фаза 5).** При `NW_HOOK_MODE=0` макросы хуков пусты, и исходники сами собирают `NWindow.dll`:
178 объектов `UClass` (`IMPLEMENT_CLASS`), глобальные переменные, экспорты с оригинальными ординалами
(`src/standalone/NWindow.def`) и 7 цветных курсоров. Движок (`Core.dll`/`Engine.dll`) загружает нашу DLL так
же, как загружал оригинал.

**Фаза 6 (функции).** Возможности, которые раньше вносились патчами снаружи (предыдущий проект `NWindow_C1`,
внедряемая DLL), здесь стали исходным кодом в `src/features/`. В декомпилированном коде остаётся только точка
вызова — блок `#if NW_FEATURES` с комментарием `// phase 6: ...`; режим хуков и точный standalone этого кода
не видят. Функции:

- **Combat log** (панель системных/боевых сообщений над чатом), **BuffTimer** (оставшееся время на иконках баффов);
- **AutoShot** (автоматические soulshot/spiritshot, как в C3), **ManufactureCmd** (`/dwarvenmanufacture`,
  действия 37 и 52), **MinimapHotkey** (Alt+M / M), **CacheCleaner** (периодические `FLUSH` + `OBJ GARBAGE`);
- **окна крафта** в стиле C2 (магазин, книга рецептов, дерево) и **облачко мастерской**;
- **WindowFix** (оконный режим: подвижная рамка, мышь интерфейса, фильтр кликов), **ReturnToLogin** (возврат на
  экран входа вместо «мёртвого» списка серверов);
- **Discord Rich Presence** (общается напрямую с локальным pipe Discord, без SDK);
- **ChatColors** (цвета Interlude для command channel и hero voice), **ItemHoverTip** (название предмета под
  курсором в тёмной рамке со значком грейда).

Настройки не используют файлов: то, что было в `LineageWindow.ini`, вкомпилировано
(`src/features/NwFeatureConfig.cpp`), а то, что игрок меняет в игре, хранится в реестре,
`HKCU\Software\Lineage2C1\NWindow\<Раздел>`. Клиентские защиты, которые использует сервер автора, — закрытый
модуль, и **в этот исходный код они не входят**.

## Состояние

- Режим хуков: готово **2 744 из 3 653** функций (**95,42 %** байтов кода), ни одной без объяснения; остальное
  компилятор генерирует сам в standalone-сборке (scalar deleting destructors, `InternalConstructor`, статическая
  инициализация). **1 360** функций проверены в игре. Актуальные цифры: [docs/PROGRESSO.md](docs/PROGRESSO.md)
  (генерирует `tools/build_db.py`).
- Standalone: линкуется, проходит статические проверки и smoke-тест (вывод совпадает с оригиналом строка в
  строку) и проверен в игре.
- Фаза 6: все функции выше есть в исходниках и проходят цикл; состояние каждой (проверена в игре или нет) — в
  [docs/notas/Fase6.md](docs/notas/Fase6.md).
- Технические заметки (`docs/`) написаны на португальском; комментарии в коде — на английском.

## Требования

- Windows, **Visual Studio 2022** с C++ для x86 (MSVC v143), **CMake ≥ 3.20**;
- **Python 3** с `pefile` и `capstone` (`pip install pefile capstone`): нужен standalone-сборке (извлечение
  курсоров) и всем инструментам;
- **ваш собственный клиент Lineage II C1**. В репозитории нет ни одного файла игры: скопируйте в `orig/` файлы
  из [orig/README.md](orig/README.md) (у `NWindow.dll` должен быть указанный там md5);
- для «судей» полного цикла (`verify_hooks.py`, `regcheck.py`, `verify_standalone.py`, `build_db.py`): экспорт
  Ghidra в `analysis/ghidra/`, который создаёт `tools/ghidra/run_export.sh` (Ghidra 11.3.2 и JDK 21; пути в
  переменных `GHIDRA` и `GHIDRA_JAVA_HOME`).

## Сборка

В x86 *Developer Command Prompt* VS 2022 (или после `vcvars32.bat`), в корне репозитория:

```bat
cmake -S . -B build -A Win32
cmake --build build --config Release
cmake --build build --config Release --target NWindowStandalone
cmake --build build --config Release --target NWindowL2
```

Первый `--build` собирает DLL режима хуков и тестовые инструменты (`nwd_smoke`, `nwd_difftest`,
`nwd_craft_test`) и не требует `orig/`. Для `NWindowStandalone` и `NWindowL2` нужен `orig/NWindow.dll`. Опция
`-DNW_FEATURE_LOG=ON` заставляет `NWindowL2` писать `LineageWindow.log` (только для диагностики; обычная сборка
не пишет никаких файлов).

## Установка и проверка

Используйте **копию** папки `System` клиента, а не ту, в которой вы играете.

**Режим хуков:**

```bat
python tools\install_test.py --system "C:\...\Lineage II\System - Test"
python tools\install_test.py --system "C:\...\Lineage II\System - Test" --restore
```

Первая команда записывает `NWindow.dll`, пересобранный из `orig/NWindow.dll` с дополнительным импортом,
`NWindowDecomp.dll` и `NWindowDecomp.ini` (прежний `NWindow.dll` сохраняется как `NWindow.dll.pre-decomp-bak`);
вторая возвращает то, что было. `NWindowDecomp.log` в этой папке перечисляет все установленные хуки. В `.ini`:
`[Hooks] Enabled=0` оставляет весь оригинальный код; `Disable=` принимает адрес (`1000cc40`), метод
(`NCGDevice::DrawText`) или класс (`NCGDevice::*`); `[Debug] Marker=1` рисует метки, которые рисует только наш
код (жёлтая линия вверху, `#` в поле пароля), чтобы убедиться, что хуки активны.

**Standalone / фаза 6:** закройте игру, сохраните `NWindow.dll` тестовой папки и скопируйте поверх него
`build\standalone\Release\NWindow.dll` (точный) или `build\l2\Release\NWindow.dll` (с функциями). Только этот
файл: никаких `.ini` и `.log` рядом. Чтобы откатиться, верните оригинал.

**Полный цикл** (сборка, smoke, судьи, standalone, L2, установка, база; останавливается на первом упавшем шаге):

```bat
python tools\cycle.py --system "C:\...\Lineage II\System - Test"
python tools\cycle.py --system "C:\...\Lineage II\System - Test" --no-install
```

Указывайте `--system` или задайте `NWD_CLIENT_SYSTEM` (ниже). Цикл
использует `build/` и копирует из этой папки DLL, нужные загрузчику (`Core.dll`, `Engine.dll` и их
зависимости). На шаге установки игра из этой папки не должна быть запущена.

**Пути машины.** Ни в одном инструменте нет путей машины автора: папка вне репозитория задаётся опцией командной
строки или одной из этих переменных (опция важнее; инструмент, которому нужна папка, а нет ни того ни другого,
останавливается и называет переменную):

| Переменная | Указывает на | Где используется |
|---|---|---|
| `NWD_CLIENT_SYSTEM` | папку `System` тестового клиента (копию: установка пишет в неё) | `--system` по умолчанию в `install_test.py`, `cycle.py`, `smoke_standalone.py`, `difftest.py`; `verify_hooks.py --log` |
| `NWD_C1_SERVER` | папку развёртывания сервера (страницы в `gameserver/data/html-en`) | `difftest.py`, набор страниц `server` (без неё пропускается) |
| `NWD_GHIDRA_HOME` | установку Ghidra (с `support/analyzeHeadless.bat`; `GHIDRA` по-прежнему важнее) | `tools/ghidra/*.sh`, `tools/c2/ghidra_census.sh` |

## Судьи (проверки)

Цикл (`tools/cycle.py`) запускает их все в этом порядке; «зелёный» — значит цикл завершился с кодом 0.

| Судья | Команда (цикл добавляет ваш `--system`, где инструмент его принимает) | Что доказывает |
|---|---|---|
| smoke | `build\smoke\Release\nwd_smoke.exe build\smoke_system` | пропатченный `NWindow.dll` (установленный в `build/smoke_system`) загружается вне игры, и каждый хук отвечает `ok` (`NWindowDecomp.log`) |
| verify | `python tools/verify_hooks.py --log build/smoke_system/NWindowDecomp.log` | каждая наша функция против оригинала по дизассемблеру: байты стека, снимаемые `ret`, вызываемые слоты vtable и импорты; расхождение допускается только с объяснением в `db/verify_notes.csv` |
| regcheck | `python tools/regcheck.py` | ни один вызывающий код оригинала не полагается на регистр, который наша функция может изменить (оригинал собран с `/GL`) |
| difftest | `python tools/difftest.py --golden` | куски DLL выполняются вне игры и пишут то же, что оригинал (`tests/golden/`) |
| standalone: объявления | `python tools/verify_standalone.py --checks source` | наши `NW_DECLARE_CLASS` / `NW_CHECK_VTABLE` против таблицы классов оригинала |
| standalone: verify | `python tools/verify_standalone.py --dll build/standalone/Release/NWindow.dll` | PE, ресурсы, экспорты, классы и vtable совпадают с оригиналом, без запуска |
| standalone: difftest | `python tools/difftest.py --standalone build/standalone/Release/NWindow.dll` | те же случаи difftest на standalone-DLL |
| standalone: smoke | `python tools/smoke_standalone.py --dll build/standalone/Release/NWindow.dll` | загрузка так, как это делает движок: 178 классов зарегистрированы, `ConsoleWnd`/`L2Font` созданы, вывод совпадает с оригиналом |
| l2: craft data | `build\craft\Release\nwd_craft_test.exe` | парсер протокола окон крафта |
| l2: discord presence | `python tools/presence/presence_test.py` | Rich Presence против поддельного Discord |
| l2: verify / smoke | `python tools/verify_standalone.py --dll build/l2/Release/NWindow.dll --l2` и `python tools/smoke_standalone.py --dll build/l2/Release/NWindow.dll --l2` | DLL фазы 6 проходит те же тесты, ровно с дополнительными классами из `tests/standalone/l2_extras.csv` |
| db | `python tools/build_db.py` | перегенерирует `db/functions.csv` и `docs/PROGRESSO.md` |

## Структура

```
orig/          бинарники вашего клиента (не в git; md5 в orig/README.md)
analysis/      факты, извлечённые из оригинала (классы, EH, vtable; экспортируемые vtable Core/Engine); analysis/ghidra/ = экспорт Ghidra (генерируется, не в git)
db/            база функций (functions.csv генерируется) + имена, заметки и проверки, внесённые вручную
include/ue2/   объявления Core.dll/Engine.dll из C1, сверенные с экспортами
include/nwindow/, src/nwindow/    декомпилированный код, один класс на файл
src/re/        среда режима хуков (установка хуков, лог, ini, совместимость CRT)
src/standalone/                   IMPLEMENT_CLASS / IMPLEMENT_PACKAGE, глобальные переменные, NWindow.def
include/features/, src/features/  фаза 6: функции
tests/         эталонный вывод оригинала (golden) и списки для standalone
third_party/ue_import/            .def экспортов Core/Engine (сборка превращает их в .lib)
tools/         анализ, генераторы, установщик, судьи, цикл
docs/          план, соглашения, прогресс и технические заметки (на португальском)
```

## Руководства

- [docs/PLANO.md](docs/PLANO.md) — план, метод и фазы; [docs/CONVENCOES.md](docs/CONVENCOES.md) — правила кода и ловушки (на португальском)
- [docs/MODIFY_INTERFACE.ru.md](docs/MODIFY_INTERFACE.ru.md) — как изменять интерфейс (панель ярлыков, кнопка, окно, цвет чата, строка подсказки)
  · [Português](docs/MODIFICAR_INTERFACE.md) · [English](docs/MODIFY_INTERFACE.md)
- [docs/LLM.md](docs/LLM.md) — руководство для ИИ-агентов, работающих с этим кодом (на английском)
- [docs/notas/Standalone.md](docs/notas/Standalone.md) — фаза 5; [docs/notas/Fase6.md](docs/notas/Fase6.md) — фаза 6 и каждая функция (`docs/notas/F6_*.md`)

## Правовая оговорка

Независимый проект обратной разработки ради совместимости и сохранения. Он не связан с NCSoft, не
авторизован и не одобрен ею. *Lineage II* — товарный знак NCSoft Corporation. **Оригинальные файлы игры не
включены** (ни DLL, ни исполняемые файлы, ни пакеты данных): каждый пользователь использует свой клиент.

## Лицензия

[GNU General Public License v3.0](LICENSE).
