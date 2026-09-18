# Native D3D11 migration (branch `feature/dx11-native`)

Goal: **true D3D11** end-to-end — no runtime D3D9 device, no long-term reliance on D3DRS/TSS emulation, D3DX9 replaced incrementally. This branch is experimental; `main` keeps the dual backend until native milestones land.

## Current vs target

| Layer | Today (DX11 play path) | Target (native) |
| --- | --- | --- |
| Device | `lwDeviceObject11`, `GetDevice()` NULL | `ID3D11Device` / `ID3D11DeviceContext` as primary API |
| State | D3D9 enums → 11 state objects in `lwDeviceObject11` | PSO + root signature / explicit CBs per pass |
| Shaders | SM4 HLSL + FF mesh shader + ShaderMgr11 VS | All draws through compiled HLSL; retire `.vsh` / asm |
| Loaders | D3DX9 texture/mesh in places | DirectXTex / WIC / existing `.dds` path |
| Effects | `eff.fx` / D3DX Effect on DX9 | DeviceObject11 path + HLSL (extend stage-8 work) |
| Math | D3DX9 vectors/matrices | DirectXMath (gradual) |

## Build flag

`MINDPOWER_DX11_ONLY=1` (Release x64 on this branch):

- Forces active backend to DX11 regardless of `renderer=` in ini.
- Never constructs `lwDeviceObject` (D3D9).
- UI should not offer DX9 (when wired).

Native API work uses **`lwD3D11NativeContext`** as the long-term home for device/context/swapchain access instead of `GetDevice()`.

## Phases

### Phase 0 — DX11-only product (this PR start)

- [x] Branch + this doc
- [x] `MINDPOWER_DX11_ONLY` compile gate
- [x] `lwD3D11NativeContext` skeleton + init from `lwDeviceObject11`
- [ ] CI/build only `Release|x64` with flag; smoke login → world
- [ ] Log `GetDevice()` inventory at startup (gap report)

### Phase 1 — Kill D3D9 device islands (game + engine)

Port or guard every `GetDevice()` site (~75 references in `source/src`). Priority order:

1. **Crash / map enter** — `Scene.cpp` (EffBox, PathBox, SmallMap), `SMallMap.cpp` *(in progress: `MP_LegacyD3D9DeviceOpt()`, PathBox/EffBox null dev_obj)*
2. **Resources** — `lwResourceMgr.cpp` (`D3DXCreateTextureFromFileEx` → 11 or DDS) *(stencil clear + file-load `#else` branch)*
3. **UI** — `UIRender.cpp`, `BitmapFont*.cpp`, `GameAppInit.cpp`
4. **Legacy VS** — `lwxRenderCtrVS.cpp`, `lwShaderMgr.cpp` (DX9 only paths `#if !MINDPOWER_DX11_ONLY`)
5. **Effects / sky** — `EffectFile.cpp`, `MPMap` sky dome, `MPResManger.cpp` *(LoadTotalVShader no-op on DX11; D3D9 backbuffer paths `#if`-gated)*

Exit: `MINDPOWER_DX11_ONLY` build with zero unguarded `GetDevice()` calls.

### Phase 2 — Replace D3DX9 link dependency

- Texture creation: route all loads through `lwD3D11Texture` / DirectXTex
- Remove `d3dx9.lib` from `game.vcxproj` link line
- Keep `D3DXMath` or migrate to `DirectXMath` in hot paths

### Phase 3 — Shrink FF emulation

- Audit `lwDeviceObject11::SetRenderState` / TSS cache — classify by draw pass
- Introduce native mesh/terrain PSOs; stop translating unused D3D9 states
- Document state contract per subsystem (world, UI, VFX)

### Phase 4 — Rename and re-home types

- Split `lwDirectX.h`: shared math/viewport vs D3D9 legacy (behind `#if !MINDPOWER_DX11_ONLY`)
- Public engine headers expose `d3d11.h` types where needed
- Rename lib output `MindPower3D_D11R.lib` (optional, cosmetic)

### Phase 5 — Delete DX9 backend

- Remove `lwDeviceObject.cpp`, dual-backend switches, `lwIsDx11Active()` branches
- Single init path in `lwIFunc.cpp`
- Update LINUX/DXVK notes (separate VM or abandon DX9-on-Linux for this fork)

## `GetDevice()` inventory (auto-tracked)

Regenerate:

```powershell
rg "GetDevice\(\)" source/src --glob "*.cpp" -c | Sort-Object
```

Snapshot at branch start — see commit message / `docs/DX11_NATIVE_INVENTORY.txt`.

## Rules for new code on this branch

1. Do **not** call `IDirect3DDevice9` or `g_Render.GetDevice()` without an DX11 alternative.
2. Prefer `lwGetActiveDeviceObject11()` or `lwD3D11NativeGetDevice()`.
3. New rendering features use HLSL + `ID3D11*` only.
4. Keep gameplay, network, and asset formats unchanged.

## Relation to stylized / day-night work

Visual features (day/night, fog, post) should target **native CBs / post chain** on this branch, not new D3D9 state.
