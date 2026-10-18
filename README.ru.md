# opencod2-plus

<p align="center"><img src="docs/banner.svg" alt="opencod2-plus" width="100%"></p>

![Status: work in progress](https://img.shields.io/badge/status-work%20in%20progress-orange)
![License: none yet](https://img.shields.io/badge/license-none%20yet-lightgrey)
![Platforms: Linux | Windows](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![CoD2 1.3](https://img.shields.io/badge/CoD2-1.3-c9b37e)
[![Fork of opencod2/opencod2](https://img.shields.io/badge/fork%20of-opencod2%2Fopencod2-555)](https://github.com/opencod2/opencod2)
[![Last commit](https://img.shields.io/github/last-commit/JotaJazz/opencod2-plus)](https://github.com/JotaJazz/opencod2-plus/commits/main)

[Español](README.es.md) · [English](README.md) · [Português](README.pt.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · **Русский**

Исходный код движка **Call of Duty 2**, полученный методом обратной разработки: многопользовательский клиент и выделенный сервер для Linux и Windows.

Этот репозиторий — форк [opencod2/opencod2](https://github.com/opencod2/opencod2), где и была проделана реконструкция движка. В нём также есть изменения из [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web). Здесь добавлены только изменения, описанные в разделе [Изменения в этом форке](#изменения-в-этом-форке). Его цель — стать одним репозиторием с клиентом и выделенным сервером, перенося сюда сервер [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) модуль за модулем.

> [!WARNING]
> **Работа в процессе — это пока не работает полностью.** Клиент загружается до главного меню, а выделенный сервер загружает карту и держит игроков, но ожидайте падений, недостающих функций и неработающих возможностей. Проект выложен для любопытных и для совместной работы, а не как законченный или пригодный для игры порт.

> [!IMPORTANT]
> **Игровой контент не включён — нужны собственные данные.** Этот репозиторий содержит только исходный код: никаких исполняемых файлов, архивов, карт, моделей, текстур, звуков или скриптов. Чтобы что-либо запустить, нужно предоставить файлы данных из копии игры, которой **вы законно владеете**.

## Прозрачность

- **Кто это поддерживает:** я JotaJazz. Мне нравятся старые игры, и именно поэтому существует этот форк: я хочу и дальше играть в CoD2 с друзьями и поддерживать игру живой для сообщества, которое всё ещё в неё играет, с помощью открытого проекта.
- **Как здесь ведётся работа:** я работаю с [Claude Code](https://claude.com/claude-code), помощником по программированию от Anthropic. Изменения этого форка, включая этот README и его переводы, написаны с его помощью. Сделанные так коммиты содержат в сообщении строку `Co-Authored-By: Claude`, поэтому их можно найти в истории через `git log`.
- **Отношения с оригиналом:** реконструкция — работа авторов opencod2 (riicchhaarrd) и Sipex. Этот форк больше не забирает у них изменения; вместо этого он переносит код из CoD2rev-Plus, скопированный без изменений, чтобы его коммиты применялись и здесь.
- **Лицензия:** у оригинального репозитория нет лицензии, и у этого пока тоже. CoD2rev-Plus, чья виртуальная машина скриптов уже здесь (`src/script`), распространяется под AGPL-3.0. Лицензия всего проекта согласовывается с авторами оригинала, прежде чем здесь будет опубликовано больше этого кода.
- **Чем это не является:** это не официальный проект, и он не связан с Activision, Infinity Ward или авторами оригинального репозитория. В нём нет файлов игры.

## Изменения в этом форке

- **Сборки снова работают:** сборки для Linux и MinGW, сломанные с тех пор, как оригинал собирался только MSVC, снова собираются; `scripts/build_check.py` собирает Linux 1.3, Linux 1.0 и MinGW за один раз.
- **Компиляция как C++:** в Linux и MinGW весь движок компилируется как C++ (gnu++17), общие заголовки сделаны корректным C++, без изменения поведения; проверки охраняют C-имена, на которые опираются блоки данных, и раскладку структур оригинального бинарного файла.
- **Виртуальная машина скриптов CoD2rev-Plus:** виртуальная машина GSC взята из CoD2rev-Plus, скопирована без изменений в `src/script`, со сгенерированным мостом C/C++. Сервер теперь держит двух игроков, отключения, переподключения и `clientkick`, тогда как реконструированная виртуальная машина падала, как только подключался второй игрок.
- **Исправления:** сервер отвечал не на том порту, не видел `.iwd` на дисках Windows под WSL и падал при любом кике; в сборке 1.0 он сообщал протокол 118; сборка C++ для Linux остаётся на glibc 2.34; `getAnimLength` и три другие встроенные функции анимации больше не роняют сервер.
- **Тесты:** smoke-тест CoD2rev-Plus, без изменений, против этого сервера, со списком проверок, которые он пока не проходит; тест с двумя игроками, тест встроенных функций анимации и проверки раскладки структур, C-имён и файлов, скопированных из CoD2rev-Plus.

Каждое изменение подробно описано в [docs/CHANGES.md](docs/CHANGES.md) (на английском).

## Версии игры

- **Версия выбирается при сборке.** По умолчанию движок собирается как 1.3 (протокол 118); с `-DCOD2_FEATURE_CFLAGS=-DCOD2_PATCH_10` он собирается как 1.0 (протокол 115).
- **Выделенный сервер 1.3 работает для тестов:** он загружает карту, отвечает на `getinfo`, `getstatus` и rcon и держит двух игроков, которые входят, выходят, возвращаются и получают кик. Он всё ещё падает при четвёртой загрузке карты.
- **Выделенный сервер 1.0 пока не работает:** он загружает карту и принимает подключения, но падает при отправке gamestate. Сервер CoD2rev-Plus, который поддерживает 1.0, 1.3 и версию из Game Pass, — следующая часть, которая будет перенесена.

## Сборка

Все цели собираются с помощью **CMake (≥ 3.16)**, каждая вне дерева исходников, в собственную папку.

### Linux

Движок — это 32-битный бинарный файл x86. В 64-битной системе Debian или Ubuntu нужны 32-битная архитектура и библиотеки:

```sh
sudo dpkg --add-architecture i386 && sudo apt update
sudo apt install build-essential gcc-multilib g++-multilib cmake \
     libsdl2-dev:i386 libgl1-mesa-dev:i386 libx11-dev:i386 \
     libcurl4-openssl-dev:i386 zlib1g-dev:i386 libstdc++6:i386
```

```sh
cmake -S . -B build-native
cmake --build build-native -j
# -> build-native/cod2_linux    (client)
# -> build-native/cod2_lnxded   (dedicated server)
```

### Windows (кросс-компиляция из Linux с помощью MinGW)

Нужны 32-битные компиляторы MinGW-w64 (`g++-mingw-w64-i686`) и статическая zlib для MinGW (`libz-mingw-w64-dev`).

```sh
cmake -S . -B build-win32 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake
cmake --build build-win32 -j
# -> build-win32/cod2_win32_ded.exe   (dedicated server)
```

Добавьте `-DCOD2_WIN32_CLIENT=ON` для клиента SDL2/GL (статические библиотеки SDL2 нужно положить в `src/win32/sdl2/lib/`; выделенному серверу они не нужны).

#### Сменная DLL рендерера (необязательно, экспериментально)

Рендерер можно собрать как отдельную сменную DLL: она экспортирует `GetRefAPI` и общается с движком только через таблицы `ri`/`re`. С настроенным клиентом запустите цель `gfxdll`:

```sh
cmake --build build-win32 --target gfxdll
# -> build-win32/gfx_d3d_mp_x86_s.dll   (renderer DLL)
# -> build-win32/cod2_win32_gfxdll.exe  (engine that loads it at runtime)
```

Мост между рендерером и движком сгенерирован заранее и лежит в репозитории в `build/gfxdll/`; сборка — это только компиляция и компоновка. Экспериментально и не проверено исчерпывающе.

### Windows (MSVC, нативно)

Нативная сборка полного клиента SDL2/GL с Visual Studio 2022 или новее и её набором инструментов **x86**. Настройте и соберите из командной строки "x86 Native Tools for VS":

```bat
cmake --preset msvc-client
cmake --build build/msvc --target cod2_win32
:: -> build/msvc/cod2_win32.exe

cmake --preset msvc-client-release
cmake --build build/msvc-release --target cod2_win32
:: -> build/msvc-release/cod2_win32.exe   (/O2)
```

Эта сборка по-прежнему компилирует C и использует реконструированную виртуальную машину скриптов (`src/PC/script`). SDL2 предоставляете вы: положите 32-битный пакет разработки SDL2 для MSVC в `third_party/SDL2-<version>/` и скопируйте `SDL2.dll` рядом с исполняемым файлом. С `-DCOD2_SDL2_STATIC=ON` она компонует статическую SDL2, собранную вами (`SDL2-static.lib`, в той же конфигурации, что и движок), и статическую среду выполнения C в один исполняемый файл, которому не нужны DLL; собирайте SDL2 с `-DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_RENDER=OFF -DSDL_FORCE_STATIC_VCRT=ON`. Release применяет `/O2` к коду, верному декомпилятору, который проверен хуже, чем сборка Debug: если что-то ведёт себя неправильно только в Release, подозревайте оптимизатор.

### Параметры CMake

| Параметр | По умолчанию | Описание |
| --- | --- | --- |
| `COD2_CXX_ALL` | `ON` | Компилирует движок как C++ в Linux и MinGW. `OFF` собирает C. |
| `COD2_CXX_DIRS` | `""` | С `COD2_CXX_ALL=OFF` — модули или файлы `src/`, которые всё равно компилируются как C++. |
| `COD2_PLUS_SCRIPT` | `ON` | Виртуальная машина скриптов CoD2rev-Plus (`src/script`) в Linux и MinGW. `OFF` использует реконструированную (`src/PC/script`), как это делают MSVC, веб-сборка и x64. |
| `COD2_FEATURE_CFLAGS` | `""` | Переключатели возможностей; `-DCOD2_PATCH_10` собирает версию 1.0. |
| `COD2_WIN32_CLIENT` | `OFF` | MinGW: клиент SDL2/GL вместо выделенного сервера. |
| `COD2_WIN_MSVC` | `OFF` | Нативная сборка MSVC (задаётся пресетами `msvc-client`). |
| `COD2_SDL2_STATIC` | `OFF` | MSVC: один автономный исполняемый файл со статическими SDL2 и средой выполнения C. |
| `COD2_DEBUG` | `OFF` | `-O0` и без усиления защиты, для отладки. |
| `COD2_X64` | `OFF` | Цель x86_64; порт в процессе, который компилируется, но пока не запускается. |

Есть также сборка WebAssembly (`emcmake cmake -S . -B build/web`, с Emscripten SDK), описанная в начале `CMakeLists.txt`.

## Запуск

Движку нужны файлы оригинальной игры (папка `main` с файлами `.iwd`), которых в этом репозитории нет. Нужна собственная копия Call of Duty 2.

```sh
./build-native/cod2_linux +set fs_basepath /path/to/CoD2
./build-native/cod2_lnxded +set fs_basepath /path/to/CoD2 +set dedicated 1 +map mp_toujane
```

В Windows:

```bat
build\msvc\cod2_win32.exe +set fs_basepath "C:\path\to\CoD2"
build-win32\cod2_win32_ded.exe +set fs_basepath "C:\path\to\CoD2" +set dedicated 1 +map mp_toujane
```

`dedicated 1` оставляет сервер в локальной сети; это тестовый сервер, а не сервер для публичного списка — пока.

## Тесты

Проверкам нужна сборка, а тем, которые запускают сервер, — папка игры с оригинальными файлами `.iwd`. Они запускают сервер в режиме LAN с временной домашней папкой, поэтому папка игры только читается.

```bash
python3 scripts/build_check.py all                      # linux (1.3), linux-10 (1.0) and mingw into build/check-*/
python3 scripts/check_c_linkage.py build/check-linux    # no unexpected C++ names in the engine
python3 scripts/layout_parity.py                        # types shared with CoD2rev-Plus's VM lay out alike
python3 scripts/opencod2_smoke.py /path/to/CoD2 --protocol 118
python3 scripts/two_players_test.py /path/to/CoD2 --protocol 118
python3 scripts/anim_builtins_test.py /path/to/CoD2 --protocol 118
python3 scripts/vendor_check.py /path/to/CoD2rev-Plus   # files copied from CoD2rev-Plus are unchanged
python3 -m unittest discover -s test
```

- `opencod2_smoke.py` запускает `scripts/smoke_test.py` из CoD2rev-Plus, без изменений, и сравнивает его ошибки с `test/smoke_expected_failures_<protocol>.txt`: проверками, которые этот сервер пока не проходит. Он завершается с ошибкой при новой ошибке и при проверке из списка, которая теперь проходит, поэтому списки только сокращаются. `--protocol 115` тестирует сборку 1.0.
- `two_players_test.py` подключает двух клиентов на tdm, заставляет их отправлять команды, отключает одного и возвращает его, а другого кикает; сервер должен отвечать после каждого шага.
- `anim_builtins_test.py` запускает `getAnimLength`, `animHasNotetrack`, `getMoveDelta` и `getAngleDelta` из скрипта уровня тестового мода.
- `scripts/cxx_errors.py` подсчитывает ошибки C++ модуля (`--return-type` находит функции, возвращающие значение, которые могут завершиться без return, что в C++ является неопределённым поведением); `scripts/gen_script_bridge.py` заново генерирует мост к виртуальной машине скриптов.

## Структура репозитория

| Путь | Содержимое |
| --- | --- |
| `src/PC` | Реконструированный движок: клиент (`client_mp`, `cgame_mp`, `ui_mp`), сервер (`server_mp`), игра (`game_mp`, `bgame`), рендерер (`gfx_d3d`), общий код (`qcommon`, `universal`), анимации (`xanim`) и реконструированная виртуальная машина скриптов (`script`). |
| `src/Mac` | Платформенный слой порта для Mac, из которого получена реконструкция: OpenGL за интерфейсом Direct3D, ввод и звук. |
| `src/script` | Виртуальная машина скриптов CoD2rev-Plus, скопированная без изменений, с нужными ей заголовками в `src/qcommon`, `src/universal`, `src/game`, `src/bgame`, `src/server`, `src/xanim`, `src/zlib`, `src/misc` и `src/libcod`. |
| `src/compat` | Мост между C-именами движка и виртуальной машиной C++ из CoD2rev-Plus; в основном сгенерирован. |
| `src/headers`, `src/imports` | Общие типы, смещения, проверенные по оригинальному бинарному файлу, и объявления импортируемых функций. |
| `src/blobs`, `build/native_gen`, `build/win_gen`, `build/gfxdll` | Данные, взятые из оригинального бинарного файла, которые движок всё ещё использует. |
| `src/unix`, `src/win32`, `src/stubs`, `src/web` | Код, зависящий от платформы, и заглушки. |
| `scripts` | Скрипты сборки и тестов, а также тестовый клиент из CoD2rev-Plus. |
| `test` | Юнит-тесты и списки, которые читают проверки. |
| `docs` | [CHANGES.md](docs/CHANGES.md) и баннер. |

## Уведомление

Это независимая реконструкция движка Call of Duty 2 на уровне исходного кода. Она не связана с Activision Publishing, Inc., Infinity Ward или какими-либо их аффилированными лицами, не авторизована, не спонсируется и не одобрена ими.

«Call of Duty» и «Call of Duty 2» являются товарными знаками Activision Publishing, Inc. В этом репозитории они используются только для идентификации и обеспечения совместимости, чтобы описать, что реконструирует код. Никаких прав на эти знаки не заявляется.

Этот проект не предоставляет защищённые авторским правом игровые данные и не помогает их получить. Чтобы использовать его с настоящим игровым контентом, вы должны предоставить файлы данных из копии Call of Duty 2, которой вы законно владеете.

Исходный код реконструированного движка является производным произведением, созданным в целях сохранения, обеспечения совместимости, исследований и образования. Он предоставляется «как есть», без каких-либо гарантий, явных или подразумеваемых.

Если вы являетесь правообладателем и считаете, что что-то здесь не должно распространяться, пожалуйста, откройте issue или свяжитесь с сопровождающим, и вопрос будет оперативно рассмотрен.

## Благодарности

- [opencod2](https://github.com/opencod2/opencod2) (riicchhaarrd), реконструкция движка
- [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web) (Sipex), усиление защиты и сетевые исправления
- [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) и [voron00/CoD2rev_Server](https://github.com/voron00/CoD2rev_Server), выделенный сервер и виртуальная машина скриптов
- id Software
- Activision / Infinity Ward
