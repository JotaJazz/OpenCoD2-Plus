# 0008. Windows client on real Direct3D 9

- Status: Accepted
- Date: 2026-10-21

## Context

The Mac port's GL layer stands in for the game's shaders with a few ARB programs: no sun, no specular, a white world and sky.

## Decision

The Windows client creates a real `IDirect3DDevice9` and compiles the game's HLSL with the system's d3dx9 (`COD2_WIN32_REAL_D3D9`, ON by default); `-DCOD2_WIN32_REAL_D3D9=OFF` keeps the GL device (`build_check.py mingw-client-gl`).

## Consequences

- Lighting matches the original; under Wine both draw nearly the same picture.
- The Linux client keeps the GL device.
- Shaders Wine's compiler cannot handle (`particle_cloud_outdoor`) fail there.
