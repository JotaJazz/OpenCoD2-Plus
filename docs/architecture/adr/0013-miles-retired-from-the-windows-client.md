# 0013. Miles retired from the Windows client

- Status: Accepted
- Date: 2026-11-01

## Context

[ADR 0012](0012-sound-engine-behind-the-ail-calls.md) put our own engine behind the `AIL_*` calls and kept the game's `mss32.dll` as the Windows default, selected by `snd_backend`, until the two had been compared. The user compared them in real games on native Windows and heard no difference. Miles is closed source, does not load under Wine, so its path was never tested here, and it crashes at 2x rate with `snd_khz 22`.

## Decision

On the `plus` branch the MinGW client has only `src/snd_sdl/`. The wrapper around the DLL, `snd_backend` and the `__stdcall` table are gone, and the `AIL_*` functions are plain ones, as on Linux. `main` keeps Miles.

## Consequences

- The Windows client no longer reads `mss32.dll` or `miles/`, and its sound is the one `sound_test.py --wine` tests.
- The `snd_khz 22` crash is gone, along with the need for the launchers' `snd_khz 44`.
- A config that names another `mss_3d_provider` falls back to the only one left, as `snd_driver.c` already does.
- A 64-bit Windows client no longer needs a new sound layer, which corrects [ADR 0010](0010-64-bit-dedicated-server-without-the-engine.md).
- Going back means reverting the commit. Reverb still resembles Miles without matching it, and can now only be compared against a build of `main`.
- The MSVC client never used Miles: it links the Mac port's stubs and stays silent.
