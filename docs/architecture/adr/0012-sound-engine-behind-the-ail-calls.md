# 0012. Sound engine behind the AIL calls

- Status: Accepted
- Date: 2026-11-01

## Context

`snd_driver.c` is the Windows binary's driver, written against Miles (`AIL_*`). On Windows the calls reach the game's `mss32.dll`; on Linux they reached the Mac port's stubs, so the Linux client had no sound. `mss32.dll` also crashes at 2x rate with `snd_khz 22` ([debt](../../roadmap/debt.md)), and its source is not available.

## Decision

`src/snd_sdl/` is our own engine, in C, on SDL2 audio: a mixer, WAV and IMA ADPCM, the distance law of `msssoft.m3d`, streams with `minimp3`, and `snd_ail.c`, which defines every `AIL_*` function `snd_driver.c` calls. The boundary stays there: `snd_driver.c` keeps talking to Miles' interface. What it gained is small: `sndWavFileLen`, as `AIL_WAV_info` has no length and the file is untrusted; closing a stream it does not start; and the corrected reverb conditions. The two clients link `src/snd_sdl/*.c` and the dedicated server links `src/Mac/Tools/MacMSS*.c`. `src/stubs/audio_stubs.c` stays in both, as the voice code links to it.

The audio callback runs under SDL's device lock, and the game writes under `SDL_LockAudioDevice`. It never touches the game's files or waits: a stream's reader thread fills a ring, and an empty ring is silence. The lock is held only to detach a stream, because closing one waits for the reader. A failed device never aborts: `AIL_open_digital_driver` returns NULL and `snd_driver.c` takes its failure path. A device that goes away keeps the voices running on the clock, so that the game's channels free up.

Everything Miles decides comes from `mss32.dll`, `miles/msssoft.m3d` and 1.3, with addresses in the code. Declared exceptions:

| Where | Miles | Here | Why |
|---|---|---|---|
| Reverb | its own room filter and table | our own Freeverb-style bus, from the public EAX presets | the only intended difference in sound; no data of Miles' but generic's zero level |
| MP3 | `mssmp3.asi` | `minimp3` | a decoder is a decoder; the length is estimated from the bitrate in both |
| `AIL_sample_buffer_ready` | counts the flips of the current buffer | looks at which buffer is played out | two flips between calls would stall `SND_RawSamples` forever |
| `mss_q3fs 0` | opens files itself | the stream fails with an error | iwds would stay out of reach anyway |

## Consequences

- Both clients run on this engine: Miles was retired from the Windows client on 2026-11-01, after the user compared the two in real games, so `mss32.dll` is no longer read. Reverb, the one intended difference, was compared by numbers only.
- The Linux client has sound, with no new dependency: `minimp3` is one vendored header, and SDL2 was already linked.
- Since the provider refuses them, a stereo or ADPCM sound on a 3D channel stays silent, as under Miles.
- An ADPCM sound loaded under `snd_khz 22` or 11, or `snd_stereo 0`, keeps its data while the driver halves its rate: wrong pitch. The stock game has no ADPCM.
