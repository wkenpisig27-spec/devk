# Phase 3 — Fixed-function emulation audit (DX11-only)

`lwDeviceObject11` stores D3D9 **render states**, **texture stage states**, and **sampler states** in CPU caches (`SetRenderState` / `SetTextureStageState` / `SetSamplerState`). Native draws translate a **subset** of that cache into constant buffers, blend/depth/rasterizer state objects, and HLSL in `lwD3D11Mesh` / `lwD3D11Blit` / post chain.

Goal of Phase 3: classify callers by **pass**, then replace cache-and-translate with **explicit pass contracts** and PSOs where the mesh path dominates.

## Pass contracts (game layer)

| Pass | `RenderStateMgr` entry | DX11 mesh hook | Primary draws |
| --- | --- | --- | --- |
| Frame | `BeginScene` / `EndScene` | — | Clear, post, UI backdrop |
| Character | `BeginCharacter` / `EndCharacter` | `lwD3D11MeshSetCharacter(1/0)` | Skinned `.lgo`, cha preview |
| Scene object | `BeginSceneObject` / `EndSceneObject` | `lwD3D11MeshSetSceneObject(1/0)` | Static props, markers |
| Transparent | `BeginTranspObject` / `EndTranspObject` | `lwD3D11MeshSetTranspObject(1/0)` | Alpha foliage, billboards |
| Terrain | `BeginTerrain` / `EndTerrain` (`SceneRender` around `MPMap::Render`) | `lwD3D11MeshSetTerrain(1/0)` | Land tiles, splats |
| Sea | `MPMap::RenderSea` | `lwD3D11MeshSetSea(1/0)` | Ocean mesh |
| Visual tuning | `GameConfig` / `SceneRender` | `lwD3D11MeshSetVisual(...)` | Fog, stylized lit, water |

Legacy RSA atom sets (`_rsa_cha`, `_rsa_sceneobj`, …) still push D3D9 enums into the cache; on DX11 they remain the **compatibility layer** until each pass gets a dedicated PSO.

## D3DRS consumed by `lwD3D11Mesh` (FF mesh shader path)

These are read via `GetCachedRS` when building `MeshCB0` and binding OM/RS state:

- `D3DRS_LIGHTING`, `D3DRS_AMBIENT` — lighting vs unlit; ambient scale
- `D3DRS_TEXTUREFACTOR` — TFACTOR modulate paths
- `D3DRS_ALPHATESTENABLE`, `D3DRS_ALPHAREF`, `D3DRS_ALPHAFUNC` — alpha test in shader
- `D3DRS_CULLMODE`, `D3DRS_MULTISAMPLEANTIALIAS` — rasterizer state objects
- `D3DRS_SRCBLEND`, `D3DRS_DESTBLEND`, `D3DRS_ALPHABLENDENABLE` — blend state objects
- `D3DRS_ZENABLE`, `D3DRS_ZWRITEENABLE` — depth stencil state objects

All other `D3DRS_*` writes from gameplay/engine are **cached only** (no GPU effect on mesh path) unless another DX11 module reads them (`lwD3D11Blit`, particles, UI, etc.).

## D3DTSS consumed by `lwD3D11Mesh`

- Stage 0: `COLOROP`, `COLORARG1`, `COLORARG2`, `ALPHAARG1`, `ALPHAARG2`, `TEXTURETRANSFORMFLAGS`
- Stage 1–2: `COLOROP`, `COLORARG1`, `COLORARG2` — dual/terrain/splat modes (`cb.more[2]`)

Stages 3–7 and most `D3DTSS_*` (bump env, result arg, etc.) are **ignored** on the mesh path today.

## Shadow pass

During character shadow map generation, `lwDeviceObject11` sets `_bShadowPass` and **drops** selected RS/TSS writes (alpha blend, cull, stage 0–1 color ops) so FF cache does not fight the depth-only pass.

## Phase 3 work order

1. **Tag passes** — mesh hooks for scene object + transparent (alongside existing character/sea/visual).
2. **Per-subsystem inventory** — `docs/DX11_PHASE3_RS_INVENTORY.txt` (SceneRender, MPMap, VFX, UI, minimap).
3. **Native PSO bundles** — `ResolveMeshOutputMerger` in `lwD3D11Mesh.cpp`: character/scene-object/transparent passes use fixed depth/blend defaults; cull/MSAA still from cache; VFX overrides (Z off, additive blend) still read D3DRS.
4. **Retire unused cache slots** — `#if MINDPOWER_DX11_ONLY` no-op for RS/TSS never read on native path (after smoke per subsystem).

## Relation to Phase 2

D3DX9 link removal is complete on Release|x64; D3DX **math** types in RSA/light setup remain until DirectXMath migration (optional).
