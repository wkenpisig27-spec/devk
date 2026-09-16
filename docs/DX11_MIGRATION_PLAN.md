# DirectX 11 Migration Plan

> **Target:** MindPower3D dual backend (`renderer=dx9|dx11`)  
> **Constraint:** DX9 stays production / playable. DX11 must stay selectable.  
> **Paused:** 16 Sep 2026 — city world holds together on DX11; character bodies and D3DX effects still incomplete.  
> **Do not:** copy Crimson HLSL, silently force DX9, or treat DX11 as shippable yet.

This is the checkpoint for the next session. The original slice plan is unchanged; what changed is how far we got and which shortcuts we took.

---

## Where we are right now

Last deployed client: `client/system/Game.exe` (Release x64, copied 16 Sep 2026 ~10:31). Config: `[Video] renderer=dx11`.

| Scene | DX11 status | Notes |
| --- | --- | --- |
| Login 2D UI | Working | Fonts / blit / sprites via DeviceObject11 + DrawPrimitiveUP |
| Select / create character 3D | Partial | Dock, ship, lighthouse beam render. Sky/depth-clip was blue; DepthClipEnable FALSE. Character models still unreliable. |
| World map | Partial | Terrain, buildings, lamps, HUD, nameplates work. Character / NPC **meshes** were missing (names at the right spots). |
| Particles / skill FX / `eff.fx` | Not at parity | `ID3DXEffect` is skipped on DX11 (`m_pEffect == NULL`). Soft / FF fallback only. |
| Shadows / post / resize | Not started | Slice 7 and 9. |

### Last untested fixes (built, not visually confirmed)

These landed just before pause. Next session should **run the client first** before changing more:

1. **Character world matrix** — DX8 `VertexBlend` (`ctrl_id=2`) now has the same DX11 `BeginSet` path as `VERTEXBLEND_DX9` (`ctrl_id=3`): `SetTransformWorld` + bone palette. Missing bodies were likely identity world (characters stacked at origin / culled). File: `source/src/engine/lwxRenderCtrVS.cpp`.
2. **Select-cha apparel crash** — `CCharacter::UpdataItem` null-checked `GetItemRecordInfo` before `pInfoMain->sType` (BugTrap ACCESS_VIOLATION ~line 1754). File: `source/src/game/Character.cpp`.
3. **Input layouts** always include COLOR; dummy BLENDWEIGHT when unsinned. Dual-tex, unlit FF, additive Z-write, LASTBETA leftover weight, TEXUV `D3DTS_TEXTURE0`.

### How DX11 actually draws (important — not the original shader plan)

We did **not** dual-compile `helper/shaders/hlsl` or stand up ShaderMgr11. Runtime path:

- `lwDeviceObject11` + DXGI swapchain implements `lwIDeviceObject`. `GetDevice()` is NULL.
- Meshes: `lwD3D11Mesh` with a custom `vs_4_0` / `ps_4_0` (rigid + skin, LASTBETA, dual tex, unlit, additive).
- Resources: `lwD3D11Buffer` / `lwD3D11Texture` / `lwD3D11Blit`; lockable DYNAMIC VBs; `DrawPrimitiveUP` via a dynamic VB (XYZRHW → blit).
- D3DX `.fx` techniques are **no-ops**. `MPResManger` forces `m_bUseSoft=true` on DX11 so `CMPModelEff` will not call `SetTechnique` on a null effect (that was the map-enter crash).
- Caps report VS 3.0 so other code does not fall into unused DX8 shader objects.

So: scene meshes work because of the custom mesh HLSL + FF-state emulation, **not** because old `.vsh` / `eff.fx` run on 11. Slice 3 is still open.

---

## Original slices vs now

| Slice | Original exit | Status | What we did / what is left |
| --- | --- | --- | --- |
| 0 Inventory + policy | Leak list + `renderer` key | **Done** | `GameConfig` `renderer=dx9\|dx11`, `lwD3D11Gaps` logger, DX9 default. |
| 1 Device + swapchain | Empty window Present | **Done** | `lwDeviceObject11`, DXGI factory/swapchain, Clear/Present, `lwRenderBackend`. |
| 2 Resources | One triangle through stream mgr | **Mostly done** | Real VB/IB/tex/blit used by the game. Not a dedicated triangle test. Finish leftover `GetDevice()` islands. |
| 3 Shader pipeline | Dual-compile HLSL, cbuffer packoffset | **Not done (shortcut)** | Custom mesh shader only. Still need ShaderMgr11 + `helper/shaders/hlsl` SM4 compile + `SetVertexShaderConstantF` → packoffset. |
| 4 FF replacement | Textured unskinned mesh | **Mostly done** | Buildings/props textured. Combiner coverage is incomplete vs full TSS matrix. |
| 5 Characters | Login char matches DX9 | **In progress** | Bone palette + outline pass exist. Bodies missing until last world-matrix fix — **verify next**. Cel-shade parity not checked. |
| 6 World | City/sea holds together | **In progress** | City terrain+buildings yes. Sea/particles/`eff.fx` no. Shade maps re-enabled after an earlier skip. |
| 7 Post + shadows | Receive-pass matches DX9 | **Not started** | Shadow maps, bloom/FXAA, 1 MB stream cap lift. |
| 8 UI / fonts / maps | HUD + login readable | **Mostly done** | Fonts, UI, minimap work. Captcha / D3DX sprite leftovers. |
| 9 Resize + ship | Screenshot-diff vs DX9 | **Not started** | `ResizeBuffers`, MSAA, visual diff suite. DX11 stays experimental. |

---

## Architecture we are keeping

- Runtime select behind `lwIDeviceObject`. **No** `LW_USE_DX11` compile fork.
- `GetDevice()` returns NULL on DX11 — callers must use `lwIDeviceObject` / `g_Render` / `lwGetActiveDeviceObject11()`.
- DX9 remains default and the Linux/DXVK path.
- Cel-shade inverted hull stays a second VS pass.
- Do not copy Crimson shader trees.

Key new files:

- `source/include/engine/lwDeviceObject11.h` + `source/src/engine/lwDeviceObject11.cpp`
- `source/include/engine/lwD3D11Mesh.h` + `source/src/engine/lwD3D11Mesh.cpp`
- `source/include/engine/lwD3D11Buffer.h` / `lwD3D11Texture.h` / `lwD3D11Blit.h` / `lwD3D11Gaps.h`
- `source/include/engine/lwRenderBackend.h` + `source/src/engine/lwRenderBackend.cpp`

---

## Crashes and visual bugs already fixed (do not re-diagnose from scratch)

| Symptom | Cause | Fix |
| --- | --- | --- |
| Login MessageBox | Shade `FillVertex` lock on DX11 | LoadVideoMemory + lockable VB; shade no longer skipped |
| Physique AV | Raw `GetDevice()` RS | Skip / use device object |
| Navy / missing select-cha meshes | Cull + no mesh draw | Skip `Cull()` on DX11; `MPCharacter::Render` |
| Solid red blob | LASTBETA leftover weight + lighting | Blend leftover onto next bone; unlit path |
| B&W lighthouse | Additive + TEXUV cookie | `DESTBLEND_ONE`, UV via `D3DTS_TEXTURE0`, additive no Z-write |
| Blue sky | Depth clip | `DepthClipEnable FALSE` |
| Map crash `MPMap::Render` CreateVertexBuffer | `GetDevice()` NULL | `MapDev()` / `dev_obj->CreateVertexBuffer` |
| Map crash `MPTile::RenderTerrain` | `GetDevice` + null neighbors | `GetRenderState`; skip null TileList |
| Map crash `CMPModelEff::RenderVS` SetTechnique | Null `ID3DXEffect` | Null-guard `CMPEffectFile`; force `m_bUseSoft` on DX11 |
| Terrain only, no buildings | VERTEXBLEND_DX9 `Initialize` D3DX constant table on empty bytecode; layout missing COLOR | Skip D3DX on DX11; layouts always COLOR |
| Buildings yes, bodies no | DX8 VertexBlend never `SetTransformWorld` | DX11 early path on ctrl_id 2 (**untested**) |
| Select-cha BugTrap in `UpdataItem` | Null item record + apparel | Null-check `pInfoMain` (**untested**) |

MSBuild: `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe`, Release \| x64. Kill `Game.exe` on LNK1104 / C1041. If CaLua is locked, `/p:BuildProjectReferences=false`. Copy `source/bin/Release/game/Game.exe` → `client/system/Game.exe`.

---

## What to do next session (in order)

1. **Verify the last build** with `renderer=dx11`:
   - Select-cha: no apparel crash; character models visible.
   - World: character / NPC bodies (not just names).
   - If bodies still missing, dump DX11 gaps log + confirm `ctrl_id` and `SetTransformWorld` on the physique primitive.
2. **Effects parity** — real replacement for `ID3DXEffect` / `eff.fx` (or a documented FF subset). Today techniques are skipped.
3. **Sweep remaining `GetDevice()`** — any leftover raw D3D9 call is a crash.
4. **Slice 3 properly** — ShaderMgr11, dual-compile existing HLSL, packoffset cbuffers. The custom mesh shader is a bridge, not the destination.
5. Slice 7 shadows / post; lift 1 MB stream cap.
6. Slice 9 resize + screenshot-diff vs DX9. Only then consider DX11 selectable as more than a debug option.

DX9 must keep working after every change. If DX11 is blank/wrong, fix DX11 — do not hide it by forcing DX9.

---

## Related

- Original canvas: `canvases/dx11-engine-plan.canvas.tsx` (slice todos updated to this checkpoint).
- DX9 history: `docs/DX9_MIGRATION_PLAN.md`.
- BugTrap is wired in `ErrorHandler` for DX11 crash dumps during this port.
