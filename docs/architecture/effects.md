# Effects (EffectsCore)

`src/PC/EffectsCore`: the effects system (Raven's FX, as in `CoD2MP_s.exe`
1.3). `.efx` files define primitives; `FxScheduler_PlayEffect` schedules them;
the renderer draws them through `FX_DrawScheduledEffects`, which ends with
`FX_DrawAll` (`0x4a2950`).

## Data the parser reads

| Item | Source of truth |
|---|---|
| `flags` (24 names) and `spawnFlags` (13) | the binary's tables at `0x59d098` and `0x59d1b8`, in `src/stubs/link_stubs.c` |
| channel `scale` | applied to every size, velocity and length curve |
| color curves | one RGB triple per key |
| old style `velocity`/`acceleration` | random curves in the base curves' units (`0x49c630`, `0x49c6b0`) |

## Primitives

| Primitive | Update (1.3 address) | What it does |
|---|---|---|
| Particle | `0x495ed0`, `0x495f70` | second radius is `size2` when the scale is not uniform, else the first (`0x495c80`); turns by its rotation delta; plays its material's frames |
| OrientedParticle | `0x496970` | faces its normal (the scene entity's forward axis) |
| Tail | `0x497350` | trails back toward where it was, its length long |
| Line | `0x4970e0` | stays between its two points |
| Cylinder | `0x497790` | does not move; reaches its length along its axis |
| Cloud | `0x496d70` | turns about its random direction; scale = size, radii = size2 and length; drawn as a particle cloud (1024 quads in a unit cube scaled by the size) |
| Emitter | `0x497970`, `0x497b50` | model turns and is scaled by its size; leaves effects every step it travels (density ± variance) |

Every primitive whose bolt is gone dies (`0x494020`); a time before the start
counts as the start (`0x4940d0`). A primitive's origin is the effect's
(`0x4a0bf3`), taken into the bolt's space when bolted.

## Playing along a direction

`FxScheduler_PlayEffect` takes a 3×3 axis. `FxScheduler_PlayEffectWithDir`
builds it from a direction with `MakeNormalVectors`; map effects with an up
vector use `FX_PlayOrientedEffect`. `FX_PlaySimpleEffect` plays along forward
up (`0x499500`).

## fxRestart

Keeps the templates on the hunk and parses each file again into its old
template (`FX_KeepTemplates`, `FX_ReloadTemplates`), since cgame, weapon defs
and entities hold pointers to them. Each restart takes hunk memory again until
the next map load.

## Tests

`scripts/fx_test.py`: plays a sprite, an impact, explosions, a water splash
and dirt plumes and checks each changes the picture
([../development/testing.md](../development/testing.md)).
