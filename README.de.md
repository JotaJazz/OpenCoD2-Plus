# opencod2-plus

<p align="center"><img src="docs/banner.svg" alt="opencod2-plus" width="100%"></p>

![Status: work in progress](https://img.shields.io/badge/status-work%20in%20progress-orange)
![License: none yet](https://img.shields.io/badge/license-none%20yet-lightgrey)
![Platforms: Linux | Windows](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![CoD2 1.3](https://img.shields.io/badge/CoD2-1.3-c9b37e)
[![Fork of opencod2/opencod2](https://img.shields.io/badge/fork%20of-opencod2%2Fopencod2-555)](https://github.com/opencod2/opencod2)
[![Last commit](https://img.shields.io/github/last-commit/JotaJazz/opencod2-plus)](https://github.com/JotaJazz/opencod2-plus/commits/main)

[Español](README.es.md) · [English](README.md) · [Português](README.pt.md) · [Français](README.fr.md) · **Deutsch** · [Русский](README.ru.md)

Durch Reverse Engineering gewonnener Quellcode der Engine von **Call of Duty 2**: der Multiplayer-Client und der dedizierte Server, für Linux und Windows.

Dieses Repository ist ein Fork von [opencod2/opencod2](https://github.com/opencod2/opencod2), wo die Rekonstruktion der Engine geleistet wurde. Es enthält außerdem Änderungen aus [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web). Dieser Fork ergänzt nur die unter [Änderungen in diesem Fork](#änderungen-in-diesem-fork) beschriebenen Änderungen. Sein Ziel ist ein einziges Repository mit dem Client und dem dedizierten Server, indem der Server von [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) Modul für Modul übernommen wird. Unter Linux ist das erreicht: Der dedizierte Server läuft vollständig auf dem Code von CoD2rev-Plus, und die lokalen Spiele des Clients laufen auf seinem Server.

> [!WARNING]
> **In Arbeit – das funktioniert noch nicht vollständig.** Der Client startet bis ins Hauptmenü, erstellt lokale Spiele, denen andere Clients beitreten, und der dedizierte Server, jetzt der von CoD2rev-Plus, lädt Karten und hält Spieler in 1.0 und 1.3, aber rechnen Sie mit Abstürzen, fehlenden Funktionen und kaputten Features. Es wird für Neugierige und zur Zusammenarbeit geteilt, nicht als fertige oder spielbare Portierung.

> [!IMPORTANT]
> **Es sind keine Spielinhalte enthalten – bringen Sie Ihre eigenen Daten mit.** Dieses Repository enthält nur Quellcode: keine ausführbaren Dateien, Archive, Karten, Modelle, Texturen, Sounds oder Skripte. Um irgendetwas auszuführen, müssen Sie Datendateien aus einer Kopie des Spiels bereitstellen, die **Sie rechtmäßig besitzen**.

## Transparenz

- **Wer das hier pflegt:** Ich bin JotaJazz. Ich mag alte Spiele, und deshalb gibt es diesen Fork: Ich möchte weiterhin mit meinen Freunden CoD2 spielen und das Spiel für die Community, die es noch spielt, mit einem offenen Projekt am Leben halten.
- **Wie hier gearbeitet wird:** Ich arbeite mit [Claude Code](https://claude.com/claude-code), dem Programmierassistenten von Anthropic. Die Änderungen in diesem Fork, einschließlich dieses README und seiner Übersetzungen, sind mit seiner Hilfe geschrieben. So entstandene Commits tragen die Zeile `Co-Authored-By: Claude` in der Commit-Nachricht und lassen sich daher mit `git log` im Verlauf erkennen.
- **Verhältnis zum Original:** Die Rekonstruktion ist das Werk der Autoren von opencod2 (riicchhaarrd) und von Sipex. Dieser Fork übernimmt keine Änderungen mehr von ihnen; stattdessen übernimmt er Code aus CoD2rev-Plus, unverändert kopiert, damit dessen Commits auch hier anwendbar sind.
- **Lizenz:** Das Original-Repository hat keine Lizenz, und dieses hat noch keine. CoD2rev-Plus, dessen Skript-VM, Server, Game und Bgame bereits hier sind (`src/script`, `src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`, `src/xanim`), steht unter AGPL-3.0. Die Lizenz des Ganzen wird mit den ursprünglichen Autoren geklärt, bevor mehr von diesem Code hier veröffentlicht wird. libcod (`src/libcod`) und ihr Bot-Mod (`mods/bots`) stammen ebenfalls von CoD2rev-Plus, mit Drittbibliotheken unter ihren eigenen Lizenzen: SQLite (gemeinfrei), Mbed TLS (Apache-2.0, `src/libcod/mbedtls/LICENSE`), yyjson (MIT, in `yyjson.h`) und Speex (BSD, `src/libcod/speex/COPYING`, nicht gebaut).
- **Was es nicht ist:** Es ist kein offizielles Projekt und steht in keiner Verbindung zu Activision, Infinity Ward oder den Autoren des Original-Repositorys. Es enthält keine Spieldateien.

## Änderungen in diesem Fork

- **Builds, die wieder funktionieren:** Die Linux- und MinGW-Builds, die kaputt waren, seit das Original nur mit MSVC gebaut wurde, lassen sich wieder bauen; `scripts/build_check.py` baut Linux und MinGW (den dedizierten Server und, mit SDL2, den Client), als 1.3 und als 1.0, in einem Durchgang.
- **Als C++ kompiliert:** Unter Linux und MinGW wird die gesamte Engine als C++ (gnu++17) kompiliert, mit gültigen C++-Versionen der gemeinsamen Header, ohne an ihrem Verhalten etwas zu ändern; Prüfungen sichern die C-Namen, auf die sich die Daten-Blobs verlassen, und die Strukturlayouts der Originalbinärdatei ab.
- **Die Skript-VM von CoD2rev-Plus:** Die virtuelle Maschine für GSC ist die von CoD2rev-Plus, unverändert nach `src/script` kopiert, mit einer generierten C/C++-Brücke. Der Server hält jetzt zwei Spieler und übersteht Trennungen, Wiederverbindungen und `clientkick`, wo die rekonstruierte VM abstürzte, sobald der zweite Spieler beitrat.
- **Der Server von CoD2rev-Plus:** Der dedizierte Server wird mit dem Server von CoD2rev-Plus gebaut, unverändert nach `src/server` kopiert, mit einer generierten Brücke, in der jeder Aufruf zwischen beiden gegen die ABI der Engine geprüft wird. Auch die lokalen Spiele des Linux- und des Windows-Clients (MinGW) laufen darauf, und der ganze Client verfolgt Treffer und Bewegungen mit seiner Kollision: Ein Client erstellt ein Spiel, und andere treten bei. Mit ihm verweigert der Server das Senden der Standard-`.iwd`-Dateien und der nur für den Server bestimmten. Mit ihm kommen sein Game und sein Bgame (`src/game`, `src/bgame`) und damit `addip` und die Rotation von `games_mp.log`; im Client behält cgame das Bgame der Engine (`src/PC/bgame`), dessen Globale das Präfix `eng_` tragen und das die Bewegungen, das Klettern und die Waffe des Spielers jetzt mit dem Code von CoD2rev-Plus vorhersagt: bei denselben Bewegungen liegt die Vorhersage jetzt etwa achtmal seltener daneben (Zurückspringen). Seitdem läuft der dedizierte Server auch auf seinem Kern: `universal`, `qcommon` und dem Plattformcode (`src/universal`, `src/qcommon`, `src/unix`, `src/win32`), die Rotation und Zeitstempel des Konsolenlogs, `net_strictPort` und seinen Absturzbericht mitbringen. Mit seinem xanim (`src/xanim`) ist alles, was der dedizierte Server ausführt, Code von CoD2rev-Plus.
- **Die libcod von CoD2rev-Plus:** Der dedizierte Server wird mit libcod gebaut, unverändert nach `src/libcod` kopiert, mit den Modulen, die CoD2rev-Plus aktiviert (alle außer MySQL): ihre Skriptfunktionen, die Bots, `sv_master1` bis `sv_master5`, HTTP und WebSocket (Mbed TLS), SQLite und JSON. Er besteht die Smoke-, Bot- und HTTP-Tests von CoD2rev-Plus in 1.3 und 1.0; auch die lokalen Spiele des Clients haben sie, auf seinem lokalen Server (Linux und Windows): Die Bots und der HTTP-Test bestehen dort ebenfalls.
- **Ein dedizierter Server ohne die Bibliotheken des Clients:** Unter Linux braucht `cod2_lnxded` nur libc, libm, libz und libstdc++, nicht GL, X11, SDL2 oder curl.
- **Korrekturen:** Der Server antwortete auf dem falschen Port, sah keine `.iwd` auf Windows-Laufwerken unter WSL und stürzte bei jedem Kick ab; im 1.0-Build meldete er Protokoll 118; der C++-Linux-Build bleibt bei glibc 2.34; `getAnimLength` und drei weitere Animations-Builtins lassen den Server nicht mehr abstürzen. Das vierte oder fünfte Laden einer Karte ließ den Server abstürzen: Das Animationsskript wuchs mit jeder Karte, und der Fehler, den es über rcon auslöste, beschädigte den Stack. Der 1.0-Server überschrieb seine eigenen Configstrings, und kein 1.0-Client konnte beitreten. Fragmentierte Nachrichten verwendeten in beiden Versionen die falschen Offsets, und der Port einer Netzwerkadresse lag dort, wo Quake III ihn ablegt, nicht dort, wo CoD2 es tut. Die `.iwd`-Dateien im eigenen Ordner des Servers wurden nicht durchsucht, und die Builtins, die einen Spieler erwarten, sahen mit der VM von CoD2rev-Plus keinen. Entitäten wurden unter der falschen Nummer in die Sektoren der Welt eingetragen, sodass das Austragen mancher von ihnen abstürzte und Traces die vorherige Entität prüften. Im Client wurde jede als Box geclippte Entität aus den Sektoren der Welt ausgetragen, und der Box-Test verfehlte die Strecken, die sie kreuzten, sodass Kugeln in einem lokalen Spiel durch die Spieler gingen; und seit einer Upstream-Änderung für x64 lief auf 32 Bit keine Animation mit mehr als einem Frame, was die Spieler und die Waffe in der Ego-Ansicht in einer kaputten Pose ließ. Der Client stürzte ab, wenn er einen Spieler sterben sah, und seine Bewegungsvorhersage konnte an Wänden den Stack überschreiben; er schickte einen Sprung, einen Haltungs- oder Waffenwechsel einen Befehl zu spät und schnitt die vorhergesagte Geschwindigkeit ab; die Animationen des MG42, das Füllen und die Textbreiten der UI und ein Dutzend weitere Engine-Aufrufe übergaben Argumente oder lasen Ergebnisse eines anderen Typs als dem der Funktion, und Effekte mit Richtung bekamen eine kaputte Achse. Es wurde überhaupt kein Effekt gezeichnet (Einschläge, Explosionen, der Rauch der Karten): Der Effektcode las seine Flags, Kurvenskalen, Farben und Geschwindigkeiten anders als die Originalbinärdatei, plante verzögerte Teile für einen Zeitpunkt, der nie kam, ließ jedes Partikel ohne zweiten Radius im Ursprung der Welt, und nichts zeichnete sie.
- **Tests:** der Smoke-Test von CoD2rev-Plus, unverändert, gegen diesen Server, mit der Liste der Prüfungen, die er noch nicht besteht; Tests für zwei Spieler, das Laden von Karten, einen Drop-Fehler über rcon und die Animations-Builtins; und Prüfungen der Strukturlayouts, der C-Namen, der ABI der Brücke und der aus CoD2rev-Plus kopierten Dateien. Tests schießen waagerecht und nach unten auf einen Spieler und prüfen, dass jeder Schuss den Oberkörper oder die Beine trifft und dass ein Messer und eine Explosion einen Spieler erreichen, auf dem dedizierten Server und in einem lokalen Spiel.

Jede Änderung im Detail steht in [docs/CHANGES.md](docs/CHANGES.md) (auf Englisch).

## Spielversionen

- **Die Version wird beim Kompilieren gewählt.** Standardmäßig wird die Engine als 1.3 gebaut (Protokoll 118); mit `-DCOD2_FEATURE_CFLAGS=-DCOD2_PATCH_10` wird sie als 1.0 gebaut (Protokoll 115).
- **Beide dedizierten Server funktionieren zum Testen:** In 1.3 und in 1.0 lädt der Server Karten, wechselt und startet sie neu, antwortet auf `getinfo`, `getstatus` und rcon und hält zwei Spieler, die beitreten, gehen, zurückkommen und gekickt werden.
- **Lokale Spiele funktionieren zum Testen:** In 1.3 und in 1.0 erstellt ein Client ein Spiel auf dem Server von CoD2rev-Plus, ein anderer Client tritt bei, und beide bleiben nach `map_restart` und einem Kartenwechsel (`scripts/listen_test.py`, mit Ihrem CD-Key). Ein Schuss trifft den Körperteil, auf den er zielt (`scripts/hit_location_test.py`).
- **Was CoD2rev-Plus in libcod hat, ist noch nicht hier:** seine Skriptfunktionen, die Bots und `sv_master1`…`sv_master5`, sodass der Server seinen Heartbeat nur an den Master-Server von Activision sendet.

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

### Windows (Cross-Kompilierung unter Linux mit MinGW)

Benötigt die 32-Bit-Compiler von MinGW-w64 (`g++-mingw-w64-i686`) und die statische zlib für MinGW (`libz-mingw-w64-dev`).

```sh
cmake -S . -B build-win32 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake
cmake --build build-win32 -j
# -> build-win32/cod2_win32_ded.exe   (dedicated server)
```

Fügen Sie `-DCOD2_WIN32_CLIENT=ON` für den SDL2/GL-Client hinzu (die statischen SDL2-Bibliotheken müssen Sie unter `src/win32/sdl2/lib/` bereitstellen; der dedizierte Server braucht keine). Wie der Linux-Client lässt er seine lokalen Spiele auf Server, Game und Bgame von CoD2rev-Plus laufen und verfolgt mit dessen Kollision. Ohne Windows richtet `scripts/portable_wine.sh DIR` ein 32-Bit-Wine ohne Root ein (Debian oder Ubuntu mit aktiviertem i386) und schreibt `DIR/wine.sh`, um es zu starten; `COD2_CONSOLE_STDOUT=1` kopiert die Konsole nach stdout, und mit `scripts/listen_test.py --host-binary cod2_win32.exe --host-wine DIR/wine.sh` erstellt er ein Spiel, dem der Linux-Client beitritt.

### Windows (MSVC, nativ)

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

### CMake-Optionen

| Option | Standard | Beschreibung |
| --- | --- | --- |
| `COD2_CXX_ALL` | `ON` | Kompiliert die Engine unter Linux und MinGW als C++. `OFF` baut C. |
| `COD2_CXX_DIRS` | `""` | Mit `COD2_CXX_ALL=OFF` die Module oder Dateien in `src/`, die weiterhin als C++ kompiliert werden. |
| `COD2_PLUS_SCRIPT` | `ON` | Die Skript-VM von CoD2rev-Plus (`src/script`) unter Linux und MinGW. `OFF` verwendet die rekonstruierte (`src/PC/script`), wie es MSVC, der Web-Build und x64 tun. |
| `COD2_PLUS_SERVER` | `ON` | Server, Game, Bgame und Kern von CoD2rev-Plus (`src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`, `src/xanim`, `src/unix` oder `src/win32`) im dedizierten Server, unter Linux und MinGW; braucht `COD2_PLUS_SCRIPT`. `OFF` verwendet die rekonstruierten (`src/PC/server_mp`, `src/PC/game_mp`, `src/PC/bgame`). |
| `COD2_PLUS_CLIENT_SERVER` | `ON` | Server, Game, Bgame und Kollision von CoD2rev-Plus für die lokalen Spiele des Clients (Linux, und MinGW mit `COD2_WIN32_CLIENT`), auf dem Kern der Engine; braucht `COD2_PLUS_SERVER`. `OFF` behält `src/PC/server_mp`, wie es MSVC, der Web-Build und x64 noch tun. |
| `COD2_PLUS_LIBCOD` | `ON` | Die libcod von CoD2rev-Plus (`src/libcod`) im dedizierten Server und, mit `COD2_PLUS_CLIENT_SERVER`, in den lokalen Spielen des Clients, mit ihren Modulen außer MySQL; braucht `COD2_PLUS_SERVER`. |
| `COD2_FEATURE_CFLAGS` | `""` | Feature-Schalter; `-DCOD2_PATCH_10` baut die Version 1.0. |
| `COD2_WIN32_CLIENT` | `OFF` | MinGW: der SDL2/GL-Client statt des dedizierten Servers. |
| `COD2_WIN_MSVC` | `OFF` | Nativer MSVC-Build (von den Presets `msvc-client` gesetzt). |
| `COD2_SDL2_STATIC` | `OFF` | MSVC: eine eigenständige ausführbare Datei mit statischem SDL2 und statischer C-Laufzeit. |
| `COD2_DEBUG` | `OFF` | `-O0` und keine Härtung, zum Debuggen. |
| `COD2_X64` | `OFF` | x86_64-Ziel; eine Portierung in Arbeit, die kompiliert, aber noch nicht läuft. |

Es gibt außerdem einen WebAssembly-Build (`emcmake cmake -S . -B build/web`, mit dem Emscripten-SDK), der am Anfang von `CMakeLists.txt` beschrieben ist.

## Ausführen

Die Engine braucht die Dateien des Originalspiels (den Ordner `main` mit seinen `.iwd`-Dateien), die nicht in diesem Repository liegen. Man braucht eine eigene Kopie von Call of Duty 2.

```sh
./build-native/cod2_linux +set fs_basepath /path/to/CoD2
./build-native/cod2_lnxded +set fs_basepath /path/to/CoD2 +set dedicated 1 +map mp_toujane
```

Unter Windows:

```bat
build\msvc\cod2_win32.exe +set fs_basepath "C:\path\to\CoD2"
build-win32\cod2_win32_ded.exe +set fs_basepath "C:\path\to\CoD2" +set dedicated 1 +map mp_toujane
```

`dedicated 1` hält den Server im lokalen Netz; es ist ein Testserver, noch keiner für die öffentliche Liste.

## Tests

Die Prüfungen brauchen den Build und, für diejenigen, die den Server starten, einen Spielordner mit den originalen `.iwd`-Dateien. Sie starten den Server im LAN-Modus mit einem temporären Benutzerordner, der Spielordner wird also nur gelesen.

```bash
python3 scripts/build_check.py all                      # linux (1.3), linux-10 (1.0) and mingw into build/check-*/
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
- `bots_test.py` und `http_test.py` stammen unverändert von CoD2rev-Plus und prüfen libcod: Die Bots aus `mods/bots` müssen ihre Wege finden, ihren Graphen speichern und wieder laden und auf einer echten Karte kämpfen; `httpFetch` und `webSocket*` müssen mit lokalen HTTP-, HTTPS-, WS- und WSS-Servern funktionieren (sie brauchen `openssl`). Übergeben Sie `--binary build/check-linux/cod2_lnxded`.
- `scripts/cxx_errors.py` zählt die C++-Fehler eines Moduls (`--return-type` findet Nicht-void-Funktionen, die ohne return enden können, was in C++ undefiniert ist); `scripts/gen_script_bridge.py --module script|server` erzeugt die Brücke zur Skript-VM oder zum Server neu, und `scripts/engine_abi.py --module server` zeichnet mit gdb die ABI der Engine auf, gegen die die Brücke prüft (`test/server_abi.txt`).

## Aufbau des Repositorys

| Pfad | Inhalt |
| --- | --- |
| `src/PC` | Die rekonstruierte Engine: Client (`client_mp`, `cgame_mp`, `ui_mp`), Server (`server_mp`), Spiel (`game_mp`, `bgame`), Renderer (`gfx_d3d`), gemeinsamer Code (`qcommon`, `universal`), Animationen (`xanim`) und die rekonstruierte Skript-VM (`script`). Unter Linux und im dedizierten MinGW-Server bleiben `server_mp`, `game_mp`, `game` und der Kern, den CoD2rev-Plus ersetzt, außen vor (`test/plus_replaces.txt`); der MinGW-Client lässt `server_mp`, `game_mp`, `game` und die Kollision weg. |
| `src/Mac` | Die Plattformschicht der Mac-Portierung, aus der die Rekonstruktion stammt: OpenGL hinter einer Direct3D-Schnittstelle, Eingabe und Sound. |
| `src/script` | Die Skript-VM von CoD2rev-Plus, unverändert kopiert, mit den Headern, die sie braucht, in `src/qcommon`, `src/universal`, `src/game`, `src/bgame`, `src/server`, `src/xanim`, `src/zlib`, `src/misc` und `src/libcod`. |
| `src/server` | Der Server von CoD2rev-Plus, unverändert kopiert; der dedizierte Server verwendet ihn. |
| `src/game`, `src/bgame` | Game und Bgame von CoD2rev-Plus, unverändert kopiert; der dedizierte Server verwendet sie. |
| `src/universal`, `src/qcommon`, `src/xanim`, `src/unix`, `src/win32` | Kern und Plattformcode von CoD2rev-Plus, unverändert kopiert; der dedizierte Server läuft darauf. `test/plus_replaces.txt` listet die Quellen der Engine, die sie dort ersetzen. |
| `src/libcod`, `mods/bots` | Die libcod von CoD2rev-Plus und ihre Bibliotheken (yyjson, SQLite, Mbed TLS; ihr Speex wird nicht gebaut, das der Engine wird verwendet) sowie ihr Bot-Mod, unverändert kopiert; der dedizierte Server verwendet sie. |
| `src/compat` | Die Brücken zwischen den C-Namen der Engine und der C++-VM, dem Server und dem Kern von CoD2rev-Plus; größtenteils generiert. |
| `src/headers`, `src/imports` | Gemeinsame Typen, gegen die Originalbinärdatei geprüfte Offsets und Deklarationen importierter Funktionen. |
| `src/blobs`, `build/native_gen`, `build/win_gen` | Aus der Originalbinärdatei übernommene Daten, die die Engine noch verwendet. |
| `src/unix`, `src/win32`, `src/stubs`, `src/web` | Plattformcode und Stubs. |
| `scripts` | Build- und Testskripte sowie der Testclient von CoD2rev-Plus. |
| `test` | Unit-Tests und die Listen, die die Prüfungen lesen. |
| `docs` | [CHANGES.md](docs/CHANGES.md) und das Banner. |

## Hinweis

Dies ist eine unabhängige Rekonstruktion der Engine von Call of Duty 2 auf Quellcodeebene. Sie steht in keiner Verbindung zu Activision Publishing, Inc., Infinity Ward oder einem ihrer verbundenen Unternehmen und wird von diesen weder autorisiert, gesponsert noch unterstützt.

„Call of Duty“ und „Call of Duty 2“ sind Marken von Activision Publishing, Inc. Sie werden in diesem Repository nur zur Identifikation und Interoperabilität verwendet, um zu beschreiben, was der Code rekonstruiert. Es werden keine Ansprüche an diesen Marken erhoben.

Dieses Projekt stellt keine urheberrechtlich geschützten Spieldaten bereit und hilft nicht dabei, sie zu beschaffen. Um es mit echten Spielinhalten zu verwenden, müssen Sie Datendateien aus einer Kopie von Call of Duty 2 bereitstellen, die Sie rechtmäßig besitzen.

Der rekonstruierte Quellcode der Engine ist ein abgeleitetes Werk, das zu Zwecken der Bewahrung, der Interoperabilität, der Forschung und der Bildung erstellt wurde. Er wird ohne Mängelgewähr („as is“) bereitgestellt, ohne jegliche ausdrückliche oder stillschweigende Gewährleistung.

Wenn Sie Rechteinhaber sind und der Meinung sind, dass etwas hier nicht verbreitet werden sollte, eröffnen Sie bitte ein Issue oder kontaktieren Sie den Maintainer, und die Angelegenheit wird umgehend bearbeitet.

## Danksagungen

- [opencod2](https://github.com/opencod2/opencod2) (riicchhaarrd), die Rekonstruktion der Engine
- [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web) (Sipex), Härtung und Netzwerkkorrekturen
- [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) und [voron00/CoD2rev_Server](https://github.com/voron00/CoD2rev_Server), der dedizierte Server und die Skript-VM
- id Software
- Activision / Infinity Ward
