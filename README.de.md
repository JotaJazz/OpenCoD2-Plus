# OpenCoD2-Plus

<p align="center"><img src="docs/banner.svg" alt="OpenCoD2-Plus" width="100%"></p>

![Status: in development](https://img.shields.io/badge/status-in%20development-orange)
![License: none yet](https://img.shields.io/badge/license-none%20yet-lightgrey)
![Platforms: Linux | Windows](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![CoD2 1.3](https://img.shields.io/badge/CoD2-1.3-c9b37e)
[![Fork of opencod2/opencod2](https://img.shields.io/badge/fork%20of-opencod2%2Fopencod2-555)](https://github.com/opencod2/opencod2)
[![Last commit](https://img.shields.io/github/last-commit/JotaJazz/opencod2-plus)](https://github.com/JotaJazz/opencod2-plus/commits/main)

[Español](README.es.md) · [English](README.md) · [Português](README.pt.md) · [Français](README.fr.md) · **Deutsch** · [Русский](README.ru.md)

**Call of Duty 2 lebt noch.** OpenCoD2-Plus ist die Multiplayer-Engine von Call of Duty 2, aus ihrem Quellcode neu aufgebaut: der Client und der dedizierte Server, für Windows und Linux, gemacht von Leuten, die das Spiel noch spielen und es weiter verbessern.

<table>
  <tr>
    <td><img src="docs/images/carentan.jpg" alt="Carentan: das Klavier vor dem Café Jeanne d'Arc"></td>
    <td><img src="docs/images/toujane.jpg" alt="Toujane: die Flak und der Turm der Moschee"></td>
  </tr>
  <tr>
    <td colspan="2"><img src="docs/images/scoreboard.jpg" alt="Die Punktetafel eines lokalen Spiels mit Bots"></td>
  </tr>
</table>
<p align="center"><sub>Carentan, Toujane und ein lokales Spiel mit Bots, gezeichnet vom Linux-Client mit den Shadern des Spiels selbst.</sub></p>

## Was heute funktioniert

- **Online spielen auf öffentlichen 1.3-Servern** mit dem Windows-Client, mit der originalen Beleuchtung: Direct3D 9 und die Shader des Spiels selbst. Die Mods eines Servers werden per HTTP geladen, wenn er sie anbietet (`sv_wwwBaseURL`), auch unter Windows.
- **Ein Spiel aus dem Client heraus erstellen**, dem Freunde beitreten können, auf Wunsch mit Bots (libcod).
- **Ein dedizierter Server für 1.3 und 1.0**, gebaut auf [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus): Unter Linux besteht er den ganzen Smoke-Test von CoD2rev-Plus.
- **Ein Linux-Client**, und einer auf Vulkan (DXVK), der ebenfalls mit den Shadern des Spiels selbst zeichnet.
- **Mit den originalen 1.3-Binärdateien abgeglichen**: Bewegungsvorhersage, Effekte, Sounds, Rückstoß, Menüs und viele Korrekturen ([was sich geändert hat](docs/changes.md)).

> [!NOTE]
> **Noch in Entwicklung.** Rechnen Sie mit Fehlern und fehlenden Teilen, und melden Sie sie bitte in den [Issues](https://github.com/JotaJazz/opencod2-plus/issues).

> [!IMPORTANT]
> **Es sind keine Spielinhalte enthalten – bringen Sie Ihre eigenen Daten mit.** Dieses Repository enthält nur Quellcode: keine ausführbaren Dateien, Archive, Karten, Modelle, Texturen, Sounds oder Skripte. Um irgendetwas auszuführen, müssen Sie Datendateien aus einer Kopie des Spiels bereitstellen, die **Sie rechtmäßig besitzen**.

## Wie man spielt

Fertige Builds werden auf der Seite **Releases** veröffentlicht. Bis dahin bauen Sie den Client (siehe [Kompilieren](#kompilieren)) und richten ihn auf Ihren Call-of-Duty-2-Ordner, den mit `main/`:

```bat
cod2_win32.exe +set fs_basepath "C:\path\to\CoD2" +connect IP:port
```

```sh
./cod2_linux +set fs_basepath /path/to/CoD2 +connect IP:port
./cod2_linux_vk +set fs_basepath /path/to/CoD2 +connect IP:port   # with the game's own shaders (Vulkan)
```

Ohne `+connect` öffnet der Client das Hauptmenü, wo Sie auch Ihr eigenes Spiel starten können. Ein dedizierter Server für Ihr lokales Netz:

```sh
./cod2_lnxded +set fs_basepath /path/to/CoD2 +set dedicated 1 +map mp_toujane
```

## Für Mitwirkende

Jede Art von Hilfe zählt: spielen und Fehler melden, auf anderen Systemen testen, oder Code. Die technische Dokumentation, auf Englisch, beginnt bei [docs/README.md](docs/README.md); [AGENTS.md](AGENTS.md) ist der Einstiegspunkt für KI-Programmieragenten.

## Kompilieren

Alle Ziele werden mit **CMake (≥ 3.16)** gebaut, jedes als Out-of-Source-Build in einem eigenen Verzeichnis.

### Linux

Die Engine ist eine 32-Bit-x86-Binärdatei. Auf einem 64-Bit-System mit Debian oder Ubuntu werden die 32-Bit-Architektur und -Bibliotheken benötigt:

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

Zum Bauen beider werden die Bibliotheken des Clients gebraucht, aber `cod2_lnxded` läuft nur mit den 32-Bit-Versionen von libc, libm, libz und libstdc++.

`cod2_linux` zeichnet mit Ersatz-Shadern (keine Sonne, weißer Himmel). Für die Shader des Spiels selbst baue `cod2_linux_vk`: `scripts/build_linux_vk_deps.sh` baut DXVK native und vkd3d-shader (32 Bit, aus festgelegten Versionen; es braucht außerdem `meson`, `ninja` und `libvulkan1:i386`), danach mit `-DCOD2_LINUX_DXVK=ON` in einem eigenen Build-Verzeichnis konfigurieren. Es braucht ein Vulkan-Gerät.

### Windows (Cross-Kompilierung unter Linux mit MinGW)

Benötigt die 32-Bit-Compiler von MinGW-w64 (`g++-mingw-w64-i686`) und die statische zlib für MinGW (`libz-mingw-w64-dev`).

```sh
cmake -S . -B build-win32 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake
cmake --build build-win32 -j
# -> build-win32/cod2_win32_ded.exe   (dedicated server)
```

Fügen Sie `-DCOD2_WIN32_CLIENT=ON` für den Client, der mit Direct3D 9 und den Shadern des Spiels zeichnet (`-DCOD2_WIN32_REAL_D3D9=OFF`: das GL-Gerät) hinzu (die statischen SDL2-Bibliotheken müssen Sie unter `src/win32/sdl2/lib/` bereitstellen; der dedizierte Server braucht keine). Wie der Linux-Client lässt er seine lokalen Spiele auf Server, Game und Bgame von CoD2rev-Plus laufen und verfolgt mit dessen Kollision. Ohne Windows richtet `scripts/portable_wine.sh DIR` ein 32-Bit-Wine ohne Root ein (Debian oder Ubuntu mit aktiviertem i386) und schreibt `DIR/wine.sh`, um es zu starten; `COD2_CONSOLE_STDOUT=1` kopiert die Konsole nach stdout, und mit `scripts/listen_test.py --host-binary cod2_win32.exe --host-wine DIR/wine.sh` erstellt er ein Spiel, dem der Linux-Client beitritt.

<details>
<summary><b>Windows (MSVC, nativ)</b></summary>

Ein nativer Build des vollständigen SDL2/GL-Clients mit Visual Studio 2022 oder neuer und dessen **x86**-Toolset. Konfigurieren und bauen Sie aus einer Eingabeaufforderung „x86 Native Tools for VS“:

```bat
cmake --preset msvc-client
cmake --build build/msvc --target cod2_win32
:: -> build/msvc/cod2_win32.exe

cmake --preset msvc-client-release
cmake --build build/msvc-release --target cod2_win32
:: -> build/msvc-release/cod2_win32.exe   (/O2)
```

Dieser Build kompiliert noch C und verwendet die rekonstruierte Skript-VM (`src/PC/script`). SDL2 stellen Sie selbst bereit: Legen Sie das 32-Bit-MSVC-Entwicklungspaket von SDL2 unter `third_party/SDL2-<version>/` ab und kopieren Sie `SDL2.dll` neben die ausführbare Datei. Mit `-DCOD2_SDL2_STATIC=ON` linkt er ein selbst gebautes statisches SDL2 (`SDL2-static.lib`, gleiche Konfiguration wie die Engine) und die statische C-Laufzeit zu einer einzigen ausführbaren Datei, die keine DLL braucht; bauen Sie SDL2 mit `-DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_RENDER=OFF -DSDL_FORCE_STATIC_VCRT=ON`. Release wendet `/O2` auf decompilergetreuen Code an, der weniger getestet ist als der Debug-Build: Wenn sich etwas nur im Release falsch verhält, verdächtigen Sie den Optimierer.

</details>

<details>
<summary><b>CMake-Optionen</b></summary>

| Option | Standard | Beschreibung |
| --- | --- | --- |
| `COD2_CXX_ALL` | `ON` | Kompiliert die Engine unter Linux und MinGW als C++. `OFF` baut C. |
| `COD2_CXX_DIRS` | `""` | Mit `COD2_CXX_ALL=OFF` die Module oder Dateien in `src/`, die weiterhin als C++ kompiliert werden. |
| `COD2_PLUS_SCRIPT` | `ON` | Die Skript-VM von CoD2rev-Plus (`src/script`) unter Linux und MinGW. `OFF` verwendet die rekonstruierte (`src/PC/script`), wie es MSVC, der Web-Build und x64 tun. |
| `COD2_PLUS_SERVER` | `ON` | Server, Game, Bgame und Kern von CoD2rev-Plus (`src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`, `src/xanim`, `src/unix` oder `src/win32`) im dedizierten Server, unter Linux und MinGW; braucht `COD2_PLUS_SCRIPT`. `OFF` verwendet die rekonstruierten (`src/PC/server_mp`, `src/PC/game_mp`, `src/PC/bgame`). |
| `COD2_PLUS_CLIENT_SERVER` | `ON` | Server, Game, Bgame und Kollision von CoD2rev-Plus für die lokalen Spiele des Clients (Linux, und MinGW mit `COD2_WIN32_CLIENT`), auf dem Kern der Engine; braucht `COD2_PLUS_SERVER`. `OFF` behält `src/PC/server_mp`, wie es MSVC, der Web-Build und x64 noch tun. |
| `COD2_PLUS_LIBCOD` | `ON` | Die libcod von CoD2rev-Plus (`src/libcod`) im dedizierten Server und, mit `COD2_PLUS_CLIENT_SERVER`, in den lokalen Spielen des Clients, mit ihren Modulen außer MySQL; braucht `COD2_PLUS_SERVER`. |
| `COD2_FEATURE_CFLAGS` | `""` | Feature-Schalter; `-DCOD2_PATCH_10` baut die Version 1.0. |
| `COD2_WIN32_CLIENT` | `OFF` | MinGW: der Client statt des dedizierten Servers. |
| `COD2_WIN32_REAL_D3D9` | `ON` | MinGW-Client: echtes Direct3D 9 und die Shader des Spiels (d3dx9 des Systems); `OFF` behält das GL-Gerät. |
| `COD2_WIN_MSVC` | `OFF` | Nativer MSVC-Build (von den Presets `msvc-client` gesetzt). |
| `COD2_SDL2_STATIC` | `OFF` | MSVC: eine eigenständige ausführbare Datei mit statischem SDL2 und statischer C-Laufzeit. |
| `COD2_DEBUG` | `OFF` | `-O0` und keine Härtung, zum Debuggen. |
| `COD2_X64` | `OFF` | x86_64-Ziel; eine Portierung in Arbeit, die kompiliert, aber noch nicht läuft. |

Es gibt außerdem einen WebAssembly-Build (`emcmake cmake -S . -B build/web`, mit dem Emscripten-SDK), der am Anfang von `CMakeLists.txt` beschrieben ist.

</details>

## Tests

<details>
<summary><b>Die Testskripte</b></summary>

Die Prüfungen brauchen den Build und, für diejenigen, die den Server starten, einen Spielordner mit den originalen `.iwd`-Dateien. Sie starten den Server im LAN-Modus mit einem temporären Benutzerordner, der Spielordner wird also nur gelesen.

```bash
python3 scripts/build_check.py all                      # every variant (linux, linux-10, mingw, mingw-client...) into build/check-*/
python3 scripts/check_c_linkage.py build/check-linux    # no unexpected C++ names in the engine
python3 scripts/layout_parity.py                        # types shared with CoD2rev-Plus's VM lay out alike
python3 scripts/check_c_decls.py build/check-linux     # jeder Engine-Aufruf passt zu den Typen seiner Definition
python3 scripts/check_var_sizes.py build/check-linux    # keine globale Variable wird größer verwendet als definiert
python3 scripts/opencod2_smoke.py /path/to/CoD2 --protocol 118
python3 scripts/two_players_test.py /path/to/CoD2 --protocol 118
python3 scripts/listen_test.py /path/to/CoD2 --protocol 118
python3 scripts/map_reload_test.py /path/to/CoD2 --protocol 118
python3 scripts/rcon_drop_test.py /path/to/CoD2 --protocol 118
python3 scripts/anim_builtins_test.py /path/to/CoD2 --protocol 118
python3 scripts/prediction_test.py /path/to/CoD2 --protocol 118
python3 scripts/fx_test.py /path/to/CoD2 --protocol 118
python3 scripts/hud_align_test.py /path/to/CoD2 --protocol 118
python3 scripts/www_download_test.py /path/to/CoD2
python3 scripts/bots_test.py /path/to/CoD2 --binary build/check-linux/cod2_lnxded
python3 scripts/http_test.py /path/to/CoD2 --binary build/check-linux/cod2_lnxded
python3 scripts/vendor_check.py /path/to/CoD2rev-Plus   # files copied from CoD2rev-Plus are unchanged
python3 -m unittest discover -s test
```

- `opencod2_smoke.py` führt `scripts/smoke_test.py` von CoD2rev-Plus unverändert aus und vergleicht seine Fehlschläge mit `test/smoke_expected_failures_<protocol>.txt`: den Prüfungen, die dieser Server noch nicht besteht. Er schlägt bei einem neuen Fehlschlag fehl und bei einem gelisteten, der jetzt besteht, sodass die Listen nur kürzer werden. `--protocol 115` testet den 1.0-Build. `--protocol 118 --binary build/check-mingw/cod2_win32_ded.exe --wine DIR/wine.sh` führt ihn gegen den dedizierten Windows-Server unter Wine aus (`scripts/portable_wine.sh`), mit eigener Liste, `test/smoke_expected_failures_wine.txt`: Unter Wine schreibt der Server nur in sein Fenster, und ein Signal erreicht ihn nicht als solches (ein mit `kill` gesendetes SIGSEGV wird ignoriert, SIGTERM beendet ihn sofort), daher werden dort der Absturzbericht, die SIGTERM-Prüfungen und die Prüfungen seiner Ausgabe nicht ausgeführt (das Skript prüft `net_strictPort` in seinem Konsolenlog). Die Linux-Listen sind leer: Der dedizierte Server besteht den ganzen Smoke-Test.
- `two_players_test.py` lässt zwei Clients auf tdm beitreten, lässt sie Befehle senden, trennt einen und holt ihn zurück und kickt den anderen; der Server, mit `developer 1`, damit ein Skriptfehler die Karte anhält, muss nach jedem Schritt antworten.
- `listen_test.py` startet einen Client, der ein Spiel erstellt, und einen zweiten, der beitritt, und prüft beide nach einem `map_restart` und einem Kartenwechsel; es braucht einen CD-Key in `~/.cod2/preferences`.
- `map_reload_test.py` lädt Karten über rcon, abwechselnd mit `map_restart` und `map` zwischen zwei Karten, und prüft jedes Laden; `rcon_drop_test.py` löst über rcon einen Drop-Fehler aus (`error drop`) und prüft, dass der Server weiterläuft.
- `anim_builtins_test.py` führt `getAnimLength`, `animHasNotetrack`, `getMoveDelta` und `getAngleDelta` aus dem Level-Skript einer Test-Mod aus.
- `prediction_test.py` zählt die Vorhersagefehler eines echten Clients (mit CD-Key), der läuft, sich duckt, sich hinlegt, springt und schießt; `--max-misses` legt fest, wie viele erlaubt sind.
- `fx_test.py` lässt einen echten Client (mit CD-Key) ein Sprite, einen Kugeleinschlag und zwei Explosionen vor sich abspielen und prüft, dass jedes den Screenshot verändert.
- `hud_align_test.py` gibt einem echten Client (mit CD-Key) drei farbige Kästen mit verschiedenen `horzAlign`/`vertAlign`-Paaren, wie den Titel einer Killcam, und prüft auf einem Screenshot, dass jeder dort landet, wo 1.3 ihn hinsetzt.
- `www_download_test.py` lässt einen Client einem Server beitreten, dessen Mod ihm fehlt, und ihn von einem lokalen HTTP-Server laden; `--wine DIR/wine.sh --binary build/check-mingw-client/cod2_win32.exe` nimmt den Windows-Client, `--disconnect` lässt den Server ihn bitten, zu gehen, zu laden und zurückzukommen.
- `bots_test.py` und `http_test.py` stammen unverändert von CoD2rev-Plus und prüfen libcod: Die Bots aus `mods/bots` müssen ihre Wege finden, ihren Graphen speichern und wieder laden und auf einer echten Karte kämpfen; `httpFetch` und `webSocket*` müssen mit lokalen HTTP-, HTTPS-, WS- und WSS-Servern funktionieren (sie brauchen `openssl`). Übergeben Sie `--binary build/check-linux/cod2_lnxded`.
- `scripts/cxx_errors.py` zählt die C++-Fehler eines Moduls (`--return-type` findet Nicht-void-Funktionen, die ohne return enden können, was in C++ undefiniert ist); `scripts/gen_script_bridge.py --module script|server|client_server` erzeugt die Brücke zur Skript-VM oder zum Server neu, und `scripts/engine_abi.py --module server` zeichnet mit gdb die ABI der Engine auf, gegen die die Brücke prüft (`test/server_abi.txt`).

</details>

## Aufbau des Repositorys

<details>
<summary><b>Was wo liegt</b></summary>

| Pfad | Inhalt |
| --- | --- |
| `src/PC` | Die rekonstruierte Engine: Client (`client_mp`, `cgame_mp`, `ui_mp`), Server (`server_mp`), Spiel (`game_mp`, `bgame`), Renderer (`gfx_d3d`), gemeinsamer Code (`qcommon`, `universal`), Animationen (`xanim`) und die rekonstruierte Skript-VM (`script`). Unter Linux und im dedizierten MinGW-Server bleiben `server_mp`, `game_mp`, `game` und der Kern, den CoD2rev-Plus ersetzt, außen vor (`test/plus_replaces.txt`); der MinGW-Client lässt `server_mp`, `game_mp`, `game` und die Kollision weg. |
| `src/Mac` | Die Plattformschicht der Mac-Portierung, aus der die Rekonstruktion stammt: OpenGL hinter einer Direct3D-Schnittstelle, Eingabe und Sound. |
| `src/script` | Die Skript-VM von CoD2rev-Plus, unverändert kopiert, mit den Headern, die sie braucht, in `src/qcommon`, `src/universal`, `src/game`, `src/bgame`, `src/server`, `src/xanim`, `src/zlib`, `src/misc` und `src/libcod`. |
| `src/server` | Der Server von CoD2rev-Plus, unverändert kopiert; der dedizierte Server verwendet ihn. |
| `src/game`, `src/bgame` | Game und Bgame von CoD2rev-Plus, unverändert kopiert; der dedizierte Server verwendet sie. |
| `src/universal`, `src/qcommon`, `src/xanim`, `src/unix`, `src/win32` | Kern und Plattformcode von CoD2rev-Plus, unverändert kopiert; der dedizierte Server läuft darauf. `test/plus_replaces.txt` listet die Quellen der Engine, die sie dort ersetzen. |
| `src/libcod`, `mods/bots` | Die libcod von CoD2rev-Plus und ihre Bibliotheken (yyjson, SQLite, Mbed TLS; ihr Speex wird nicht gebaut, das der Engine wird verwendet) sowie ihr Bot-Mod, unverändert kopiert; der dedizierte Server und die lokalen Spiele des Clients verwenden sie. |
| `src/compat` | Die Brücken zwischen den C-Namen der Engine und der C++-VM, dem Server und dem Kern von CoD2rev-Plus; größtenteils generiert. |
| `src/headers`, `src/imports` | Gemeinsame Typen, gegen die Originalbinärdatei geprüfte Offsets und Deklarationen importierter Funktionen. |
| `src/blobs`, `build/native_gen`, `build/win_gen` | Aus der Originalbinärdatei übernommene Daten, die die Engine noch verwendet. |
| `src/unix`, `src/win32`, `src/stubs`, `src/web` | Plattformcode und Stubs. |
| `scripts` | Build- und Testskripte sowie der Testclient von CoD2rev-Plus. |
| `test` | Unit-Tests und die Listen, die die Prüfungen lesen. |
| `docs` | Die [Dokumentation](docs/README.md) und das Banner. |

</details>

## Transparenz

- **Wer das hier pflegt:** Ich bin JotaJazz. Ich mag alte Spiele, und deshalb gibt es diesen Fork: Ich möchte weiterhin mit meinen Freunden CoD2 spielen und das Spiel für die Community, die es noch spielt, mit einem offenen Projekt am Leben halten.
- **Wie hier gearbeitet wird:** Ich arbeite mit [Claude Code](https://claude.com/claude-code), dem Programmierassistenten von Anthropic. Die Änderungen in diesem Fork, einschließlich dieses README und seiner Übersetzungen, sind mit seiner Hilfe geschrieben. So entstandene Commits tragen die Zeile `Co-Authored-By: Claude` in der Commit-Nachricht und lassen sich daher mit `git log` im Verlauf erkennen.
- **Verhältnis zum Original:** Die Rekonstruktion ist das Werk der Autoren von opencod2 (riicchhaarrd) und von Sipex. Dieser Fork übernimmt keine Änderungen mehr von ihnen; stattdessen übernimmt er Code aus CoD2rev-Plus, unverändert kopiert, damit dessen Commits auch hier anwendbar sind.
- **Lizenz:** noch keine, wie beim Original. Der Code von CoD2rev-Plus hier (Skript-VM, Server, Game, Kern, libcod und ihr Bot-Mod) steht unter AGPL-3.0, und die Lizenz des Ganzen wird mit den ursprünglichen Autoren geklärt. Drittbibliotheken behalten ihre eigenen: SQLite (gemeinfrei), Mbed TLS (Apache-2.0, `src/libcod/mbedtls/LICENSE`), yyjson (MIT) und Speex (BSD, nicht gebaut).
- **Was es nicht ist:** Es ist kein offizielles Projekt und steht in keiner Verbindung zu Activision, Infinity Ward oder den Autoren des Original-Repositorys. Es enthält keine Spieldateien.

## Hinweis

Dies ist eine unabhängige Rekonstruktion der Engine von Call of Duty 2 auf Quellcodeebene. Sie steht in keiner Verbindung zu Activision Publishing, Inc., Infinity Ward oder einem ihrer verbundenen Unternehmen und wird von diesen weder autorisiert, gesponsert noch unterstützt.

„Call of Duty“ und „Call of Duty 2“ sind Marken von Activision Publishing, Inc. Sie werden in diesem Repository nur zur Identifikation und Interoperabilität verwendet, um zu beschreiben, was der Code rekonstruiert. Es werden keine Ansprüche an diesen Marken erhoben.

Dieses Projekt stellt keine urheberrechtlich geschützten Spieldaten bereit und hilft nicht dabei, sie zu beschaffen. Um es mit echten Spielinhalten zu verwenden, müssen Sie Datendateien aus einer Kopie von Call of Duty 2 bereitstellen, die Sie rechtmäßig besitzen.

Der rekonstruierte Quellcode der Engine ist ein abgeleitetes Werk, das zu Zwecken der Bewahrung, der Interoperabilität, der Forschung und der Bildung erstellt wurde. Er wird ohne Mängelgewähr („as is“) bereitgestellt, ohne jegliche ausdrückliche oder stillschweigende Gewährleistung.

Wenn Sie Rechteinhaber sind und der Meinung sind, dass etwas hier nicht verbreitet werden sollte, eröffnen Sie bitte ein Issue oder kontaktieren Sie den Maintainer, und die Angelegenheit wird umgehend bearbeitet.

Die Screenshots zeigen Inhalte von Call of Duty 2, Eigentum von Activision Publishing, Inc., nur zur Identifikation.

## Danksagungen

- [opencod2](https://github.com/opencod2/opencod2) (riicchhaarrd), die Rekonstruktion der Engine
- [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web) (Sipex), Härtung und Netzwerkkorrekturen
- [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) und [voron00/CoD2rev_Server](https://github.com/voron00/CoD2rev_Server), der dedizierte Server und die Skript-VM
- id Software
- Activision / Infinity Ward
