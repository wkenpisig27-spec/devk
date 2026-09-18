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
- [x] CI/build only `Release|x64` with flag; smoke login → world (manual)
- [ ] Log `GetDevice()` inventory at startup (gap report) — optional Phase 2 prep

### Phase 1 — Kill D3D9 device islands (game + engine) — **complete**

All active D3D9 device use on **`MINDPOWER_DX11_ONLY` Release x64** is gated (`#if MINDPOWER_USE_D3D9_DEVICE`) or short-circuited via `MindPowerDx11OnlyBuild()` / `dev_obj` / `lwD3D11CreateTextureFromFile`.

Delivered:

1. Scene / minimap / login / create-char — `MP_LegacyD3D9DeviceOpt()`, `lwDeviceObject11` clears
2. **lwResourceMgr** — stencil clear, file load, color-filter mono textures on D3D11
3. UI — UIRender, BitmapFontAdapter, BitmapFont, MPFont, MPRender init
4. **lwxRenderCtrVS**, **lwShaderMgr** (incl. DX8 mgr stubs on DX11-only)
5. **EffectFile**, **MPResManger**, **MPMap** sky-doom D3D9 shader path (FF sky on DX11)

**Smoke checklist** (Release x64, verified 2026-09-19):

- [x] Login → server list → enter world
- [x] Create character (3D preview) if applicable
- [x] Minimap + large map open/close
- [x] Combat / skill VFX (particles)
- [x] Logout or character select; one window resize or alt-tab

Exit criterion met: no unguarded `GetDevice()` in the DX11-only compile; dual-build retains D3D9 behind `#if MINDPOWER_USE_D3D9_DEVICE`.

### Phase 2 — Replace D3DX9 link dependency — **complete**

1. **Audit** — `docs/DX11_PHASE2_D3DX_INVENTORY.txt` (dual-build D3DX behind `#if MINDPOWER_USE_D3D9_DEVICE` or dead-stripped when `lwIsDx11Active()` is constant)
2. **Textures** — `BitmapFont`, `lwDDSFile`, `lwResourceMgr`, `lwDeviceObject::CreateTextureFromFileInMemory` on DX11 helpers
3. **Link** — `d3dx9.lib` removed from Release x64 `game.vcxproj` (keep `d3d9.lib` until Phase 5)
4. **Math** — defer D3DXMath → DirectXMath unless link audit forces it

**Smoke (2026-09-19):** same Phase 1 checklist passed after Phase 2 builds.

Exit criterion met: Release|x64 links without `d3dx9.lib`; gameplay smoke passes.

### Phase 3 — Shrink FF emulation *(in progress)*

- [x] Pass contract doc — `docs/DX11_PHASE3_FF_AUDIT.md` (RS/TSS consumed by `lwD3D11Mesh`)
- [x] Tag scene-object + transparent passes — `RenderStateMgr` → `lwD3D11MeshSetSceneObject` / `SetTranspObject`
- [ ] Per-subsystem `SetRenderState` inventory (world, UI, VFX)
- [ ] Native PSO bundles per pass; no-op uncached D3DRS on DX11-only

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
