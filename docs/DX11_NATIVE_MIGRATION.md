# Native D3D11 migration (branch `feature/dx11-native`)

Goal: **true D3D11** end-to-end — no runtime D3D9 device, no long-term reliance on D3DRS/TSS emulation, D3DX9 replaced incrementally. This branch is experimental; `main` keeps the dual backend until native milestones land.

## Current vs target

| Layer | Today (DX11 play path) | Target (native) |
| --- | --- | --- |
| Device | `lwD3D11NativeContext` (`ID3D11Device` / context / swapchain); `GetDevice()` is a D3D9 leftover that returns NULL | same (PSO later) |
| State | D3D9 enums → 11 state objects in `lwDeviceObject11` | PSO + root signature / explicit CBs per pass |
| Shaders | SM4 HLSL + FF mesh shader + ShaderMgr11 VS | All draws through compiled HLSL; retire `.vsh` / asm |
| Loaders | DDS/BMP/TGA + GDI+ (`lwD3D11CreateTextureFromMemory`) | same (DirectXTex optional) |
| Effects | Compiled `shader\\eff.hlsl` (tex * diffuse/TFACTOR); OM still from `Pass()` | same |
| Math | DirectXMath via `lwD3DXCompat.h`; public names are XM* (`lwXMMath.h`) | same (storage types, not SIMD registers) |

## Build flag

`MINDPOWER_DX11_ONLY=1` (Debug|x64 and Release|x64 on this branch):

- Forces active backend to DX11 regardless of `renderer=` in ini.
- Never constructs `lwDeviceObject` (D3D9). `lwDeviceObject.cpp` is excluded from both configs.
- Play-path GPU handle is **`lwD3D11NativeContext`** (`lwD3D11NativeGetDevice/GetContext/GetSwapChain`). `MPRender` does not store `IDirect3DDevice9*`.
- UI should not offer DX9 (when wired).

Lib names: Debug `MindPower3D_D11D.lib`, Release `MindPower3D_D11R.lib`. Neither links `d3d9.lib` / `d3dx9`.

## Phases

### Phase 0 — DX11-only product (this PR start)

- [x] Branch + this doc
- [x] `MINDPOWER_DX11_ONLY` compile gate
- [x] `lwD3D11NativeContext` skeleton + init from `lwDeviceObject11`
- [x] CI/build only `Release|x64` with flag; smoke login → world (manual)
- [ ] Log `GetDevice()` inventory at startup (gap report) — optional Phase 2 prep

### Phase 1 — Kill D3D9 device islands (game + engine) — **complete**

All active D3D9 device use on **`MINDPOWER_DX11_ONLY` Debug and Release x64** is gated (`#if MINDPOWER_USE_D3D9_DEVICE`) or short-circuited via `MindPowerDx11OnlyBuild()` / `dev_obj` / `lwD3D11CreateTextureFromFile`.

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
3. **Link** — `d3dx9.lib` and `d3d9.lib` dropped from Release x64 (`lwD3DXCompat.h` + DirectXMath).
4. **Math** — DirectXMath in `lwD3DXCompat.h`; play-path names are XM* (`lwXMMath.h`). Storage layout still matches D3DX (12-byte vec3 / 64-byte matrix + operators). Not `DirectX::XMVECTOR` / `DirectX::XMMATRIX`.

**Smoke (2026-09-19):** same Phase 1 checklist passed after Phase 2 builds.

Exit criterion met: Release|x64 links without `d3dx9.lib`; gameplay smoke passes.

### Phase 3 — Shrink FF emulation — **complete**

- [x] Pass contract doc — `docs/DX11_PHASE3_FF_AUDIT.md` (RS/TSS consumed by `lwD3D11Mesh`)
- [x] Tag scene-object + transparent passes — `RenderStateMgr` → `lwD3D11MeshSetSceneObject` / `SetTranspObject`
- [x] Per-subsystem `SetRenderState` inventory — `docs/DX11_PHASE3_RS_INVENTORY.txt`
- [x] Native OM bundles (character / scene-object / transparent) — `ResolveMeshOutputMerger` in `lwD3D11Mesh.cpp`
- [x] Terrain pass — `SceneRender` → `BeginTerrain` / `EndTerrain`, `lwD3D11MeshSetTerrain`, OM bundle + no stylized cel on land
- [x] DX11-only no-op cache for legacy RS (shade/dither/fog table/clip/fill) in `lwDeviceObject11::SetRenderState`
- [x] VFX pass — `BeginVfx` / `EndVfx` around shade maps, particles, skill effects; unlit + depth-read OM
- [x] Unused TSS no-ops (stages ≥3, bump, texcoord index) — forced TSS still cached

**Smoke (2026-09-19):** Phase 3 pass bundles + additive/transparent/character focus passed. VFX re-smoke (combat skills, death particles, ground shade, weapon lit, quest) passed.

Stage 0–2 combiners still exist in compiled HLSL (`CombineTss`). Character/scene/transp/default decode TSS for dual-tex; VFX/terrain/sea bake combiner from the pass. Math is DirectXMath behind D3DX names (`lwD3DXCompat.h`).

### Phase 4 — Rename and re-home types — **complete**

- [x] Split `lwDirectX.h`: shared math/viewport/COM typedefs in `lwDirectXShared.h`; public headers include `d3d11.h`
- [x] D3D9 device lib pragma (`d3d9.lib`) only when `MINDPOWER_USE_D3D9_DEVICE`; `d3dx9.lib` stays for math until DirectXMath
- [x] Lib output `MindPower3D_D11R.lib` (Release) / `MindPower3D_D11D.lib` (Debug)

Create*X device macros stay so remaining D3D9 `.cpp` still compiles; they are unused on the DX11-only play path.

**Smoke (2026-09-19):** login → world after header/lib rename passed.

### Phase 5 — Delete DX9 backend — **complete** (optional source strip remains)

- [x] Exclude `lwDeviceObject.cpp` from Debug|x64 and Release|x64 (`MINDPOWER_DX11_ONLY`)
- [x] Gate remaining `lwDeviceObject*` casts (shadow, stream list, VS pixel-shader)
- [x] Skip D3D8.1 runtime version probe on DX11-only init
- [x] `lwIsDx11Active()` is a compile-time `1` on DX11-only (header inline)
- [x] LINUX/DXVK — native client is Windows-only; see below
- [x] Drop `d3d9.lib` from game Release (wrappers inherit `lwDx11I*`, not `IDirect3D*9`)

**Smoke (2026-09-19):** login → world passed after excluding `lwDeviceObject.cpp`, inlining `lwIsDx11Active()`, and dropping `d3d9.lib`.

### Post-5 — Native types, math, combiners, loaders — **complete**

- [x] Texture/buffer/shader wrappers inherit `lwDx11I*` (`lwD3D11ResourceIface.h`), not Microsoft `IDirect3D*9` COM vtables. `IDirect3DTextureX` etc. typedef to those types on DX11-only (names avoid `MindPower::lwIVertexBuffer`).
- [x] DirectXMath via `lwD3DXCompat.h`; Release no longer pragma-links `d3dx9.lib`. D3DXVECTOR/MATRIX names kept for asset layout.
- [x] Stage 0–2 combiners live in compiled FF mesh HLSL (`CombineTss`). Remaining `.vsh` files used by ShaderMgr map to SM4 HLSL (incl. alt/outline).
- [x] Texture create on DX11-only is `lwD3D11CreateTextureFromMemory` / FromFile (DDS/BMP/TGA/GDI+). Remaining D3DX texture/effect/asm APIs are `#if MINDPOWER_USE_D3D9_DEVICE`.

**Smoke (2026-09-19):** login → world passed after Post-5.

Debug and Release are both DX11-only (`MindPower3D_D11D.lib` / `MindPower3D_D11R.lib`). D3D9 sources remain in the tree but are not compiled or linked on the play path.

### Pass state — **code complete** (await smoke)

`kMeshPass` in `lwD3D11Mesh.cpp` supplies OM defaults per tagged pass (character / scene / transp / terrain / vfx / sea). Cache is only consulted for cull/MSAA and documented intra-pass variation:

- character/scene: leftover `DESTBLEND_ONE` ignored; hair/cape still enable alpha via `ALPHABLENDENABLE`
- VFX: leftover `ALPHABLENDENABLE=0` still blends; each effect may still set src/dest. Combiner still decodes TSS (weapon/skill dual-tex). `BeginVfx` / `ResetWeaponGlowStageState` clear leftover tex1/tex2, UV, additive blend, TFACTOR, and alpha-test. Sword trail strips fade via vertex diffuse alpha.
- terrain: leftover particle dest-blend ignored, but splat `EnableAlpha` still turns on standard alpha (`CACHE_BLEND_ALPHA`). Splat is tex1 SRV + stage1 COLOROP → `dual=1` (not leftover COLOROP alone, and not `dual=2` mask multiply). `BeginTerrain` clears leftover tex1/tex2.

UV mats, TFACTOR, alpha-test, lights/materials, bones stay per-draw. Character dual-tex still decodes TSS (`dual=2` vs `3` vs `7`).

**Smoke:** character, terrain, transparent props, combat VFX, weapon lit — not only login → world.

## Remaining (after native device handle)

Highest-value leftover vs the native target table:

1. Next: delete unused `.vsh` assets. Do not delete D3D9 sources unless asked — they are already out of the play-path compile.

### eff.fx as HLSL — **complete**

- `client/shader/eff.hlsl` is compiled at mesh init / `CMPEffectFile::LoadEffectFromFile`.
- `Pass()` selects `lwD3D11MeshSetEffTech(0..6)` and binds the compiled PS. Pixel formula is `tex * (TFACTOR or vertex color)` — not TSS ColorOp.
- OM (z/blend/cull) and sampler still come from the t0–t6 table plus caller overrides after `Pass()` (additive dest-blend, model TFACTOR).
- `End()` clears the tech so later character/scene draws use the mesh combiner PS again.

### XM* math rename — **complete**

- `lwXMMath.h` exposes XM* storage types (`XMVECTOR3`, `XMMATRIX`, `XMCOLORF`, …) and same-signature wrappers (`XMMatrixLookAtLH`, `XMVector3Normalize`, …).
- Implementation stays in `lwD3DXCompat.h` (DirectXMath load/store). D3DX names remain as aliases so dual-build / missed call sites still compile.
- Engine + game call sites renamed. Texture/effect `D3DXCreate*` / `ID3DX*` APIs unchanged.
- Do not `using namespace DirectX` in TUs that use the global `XMMATRIX` storage type.

### Device handle — **complete**

- `lwD3D11NativeBindDevice` from `lwDeviceObject11::CreateDevice`; `lwD3D11NativeUnbindDevice` on destroy.
- `MPRender` no longer stores `IDirect3DDeviceX*` on DX11-only. `GetDevice()` is a leftover that returns nullptr. Use `g_Render.GetD3D11Device()` / `GetD3D11Context()` / `GetSwapChain()`, or `lwD3D11NativeGet*`.
- Debug|x64 uses the same `MINDPOWER_DX11_ONLY` play path as Release.

## Linux / DXVK

The native D3D11 client on this branch (`MINDPOWER_DX11_ONLY`, Debug and Release x64) is **Windows-only**. It does not use a D3D9 device, so DXVK’s D3D9 translation path does not apply. A Linux/DXVK play client would need either:

- the D3D9 backend still on `main`, or
- a separate Vulkan/DXVK-native port (out of scope here)

Server binaries still build on Linux (`source/scripts/build-linux.sh`). Do not treat DX9-on-Linux as a requirement for this fork’s DX11 play path.

## `GetDevice()` inventory (auto-tracked)

Regenerate:

```powershell
rg "GetDevice\(\)" source/src --glob "*.cpp" -c | Sort-Object
```

Snapshot at branch start — see commit message / `docs/DX11_NATIVE_INVENTORY.txt`.

## Rules for new code on this branch

1. Do **not** call `IDirect3DDevice9` or `g_Render.GetDevice()` without an DX11 alternative.
2. Prefer `lwD3D11NativeGetDevice()` / `GetContext()` / `GetSwapChain()`, or `lwGetActiveDeviceObject11()`.
3. New rendering features use HLSL + `ID3D11*` only.
4. Keep gameplay, network, and asset formats unchanged.

## Relation to stylized / day-night work

Visual features (day/night, fog, post) should target **native CBs / post chain** on this branch, not new D3D9 state.
