# DirectX 9 Migration Plan

> **Reference Source:** `new_generations_src`  
> **Target Project:** `pkodev`  
> **Created:** January 6, 2026

---

## Overview

This document lists all files from `new_generations_src` that have been modified to support DirectX 9 migration. Use this as a reference when migrating `pkodev` from DirectX 8 to DirectX 9.

---

## Migration Architecture

### Abstraction Layer Approach

The DX9 migration uses a **compile-time abstraction layer** defined in `lwDirectX.h`:

```cpp
// When LW_USE_DX9 is defined:
typedef IDirect3D9 IDirect3DX;
typedef IDirect3DDevice9 IDirect3DDeviceX;
typedef IDirect3DTexture9 IDirect3DTextureX;
// etc.

// When LW_USE_DX8 is defined (default):
typedef IDirect3D8 IDirect3DX;
typedef IDirect3DDevice8 IDirect3DDeviceX;
typedef IDirect3DTexture8 IDirect3DTextureX;
// etc.
```

### Preprocessor Definitions Required

For DX9 builds, add these to project preprocessor definitions:
```
USE_DX_VERSION
LW_USE_DX9
DIRECT3D_VERSION=0x0900
```

### Library Paths

| Build | DirectX Include Path | DirectX Library Path |
|-------|---------------------|---------------------|
| DX8 (Debug) | `Libraries\DirectX\Include` | `Libraries\DirectX\lib` |
| DX9 (Release) | `Libraries\DirectX\DX9\Include` | `Libraries\DirectX\DX9\Lib` |

---

## Files Modified for DX9 (68 total)

### Priority 1: Core Abstraction Layer (Must Migrate First)

| File | Description | Status |
|------|-------------|--------|
| `Engine\include\lwDirectX.h` | Core DX8/DX9 type abstraction layer | ☐ |

### Priority 2: Engine Core Files (High Impact)

#### Engine Headers (18 files)

| File | Description | Status |
|------|-------------|--------|
| `Engine\include\d3dutil.h` | D3D utility functions | ☐ |
| `Engine\include\EffectFile.h` | Effect file handling | ☐ |
| `Engine\include\I_Effect.h` | Effect interface | ☐ |
| `Engine\include\lwIFunc.h` | Interface functions | ☐ |
| `Engine\include\lwInterface.h` | Main engine interface | ☐ |
| `Engine\include\lwITypes.h` | Interface types | ☐ |
| `Engine\include\lwITypes2.h` | Interface types (extended) | ☐ |
| `Engine\include\MPCharacter.h` | Character system | ☐ |
| `Engine\include\MPMap.h` | Map system | ☐ |
| `Engine\include\MPModelEff.h` | Model effects | ☐ |
| `Engine\include\MPParticleSys.h` | Particle system | ☐ |
| `Engine\include\MPRender.h` | Main render interface | ☐ |
| `Engine\include\MPResManger.h` | Resource manager | ☐ |
| `Engine\include\MPSceneItem.h` | Scene items | ☐ |
| `Engine\include\MPSceneObject.h` | Scene objects | ☐ |
| `Engine\include\MPTextureSet.h` | Texture sets | ☐ |
| `Engine\include\mygraph.h` | Graphics utilities | ☐ |

#### Engine Source (30 files)

| File | Description | DX9 Conditionals | Status |
|------|-------------|------------------|--------|
| `Engine\src\lwDeviceObject.cpp` | Device object management | 11 | ☐ |
| `Engine\src\lwDeviceObject.h` | Device object definitions | 4 | ☐ |
| `Engine\src\MPResManger.cpp` | Resource manager impl | 7 | ☐ |
| `Engine\src\lwxRenderCtrVS.cpp` | Vertex shader render control | 6 | ☐ |
| `Engine\src\ShaderLoad.cpp` | Shader loading | 3 | ☐ |
| `Engine\src\lwSystemInfo.cpp` | System info detection | 3 | ☐ |
| `Engine\src\lwIFunc.cpp` | Interface functions impl | 3 | ☐ |
| `Engine\src\lwShaderMgr.cpp` | Shader manager | 1 | ☐ |
| `Engine\src\lwShaderMgr.h` | Shader manager definitions | 1 | ☐ |
| `Engine\src\d3dutil.cpp` | D3D utilities impl | - | ☐ |
| `Engine\src\lwD3DSettings.cpp` | D3D settings | - | ☐ |
| `Engine\src\lwDDSFile.cpp` | DDS texture loading | - | ☐ |
| `Engine\src\lwDDSFile.h` | DDS file definitions | - | ☐ |
| `Engine\src\lwExpObj.cpp` | Export objects | 1 | ☐ |
| `Engine\src\lwgraphicsutil.cpp` | Graphics utilities | 1 | ☐ |
| `Engine\src\lwgraphicsutil.h` | Graphics utility definitions | - | ☐ |
| `Engine\src\lwPhysique.cpp` | Physics system | - | ☐ |
| `Engine\src\lwResourceMgr.cpp` | Resource manager | - | ☐ |
| `Engine\src\lwResourceMgr.h` | Resource manager definitions | - | ☐ |
| `Engine\src\lwStreamObj.cpp` | Stream objects | 3 | ☐ |
| `Engine\src\lwStreamObj.h` | Stream object definitions | - | ☐ |
| `Engine\src\lwxRenderCtrlVS.h` | Vertex shader render control | 1 | ☐ |
| `Engine\src\MPCharacter.cpp` | Character impl | - | ☐ |
| `Engine\src\MPMap.cpp` | Map impl | - | ☐ |
| `Engine\src\MPModelEff.cpp` | Model effects impl | - | ☐ |
| `Engine\src\MPParticleCtrl.cpp` | Particle controller | - | ☐ |
| `Engine\src\MPRender.cpp` | Main renderer | - | ☐ |
| `Engine\src\MPSceneItem.cpp` | Scene items impl | - | ☐ |
| `Engine\src\MPSceneObject.cpp` | Scene objects impl | - | ☐ |
| `Engine\src\mygraph.cpp` | Graphics impl | - | ☐ |

### Priority 3: Client Files (20 files)

| File | Description | Status |
|------|-------------|--------|
| `Client\src\stdafx.h` | Precompiled header | ☐ |
| `Client\src\Main.cpp` | Entry point | ☐ |
| `Client\src\Scene.h` | Scene definitions | ☐ |
| `Client\src\Scene.cpp` | Scene logic | ☐ |
| `Client\src\SceneRender.cpp` | Scene rendering | ☐ |
| `Client\src\SceneLight.h` | Lighting definitions | ☐ |
| `Client\src\SceneObj.h` | Scene object definitions | ☐ |
| `Client\src\SceneObj.cpp` | Scene objects | ☐ |
| `Client\src\RenderStateMgr.h` | Render state definitions | ☐ |
| `Client\src\RenderStateMgr.cpp` | Render state management | ☐ |
| `Client\src\UIRender.h` | UI render definitions | ☐ |
| `Client\src\UIRender.cpp` | UI rendering | ☐ |
| `Client\src\CharacterModel.h` | Character model definitions | ☐ |
| `Client\src\CharacterModel.cpp` | Character rendering | ☐ |
| `Client\src\EffectObj.h` | Effect object definitions | ☐ |
| `Client\src\SMallMap.h` | Minimap definitions | ☐ |
| `Client\src\SMallMap.cpp` | Minimap | ☐ |
| `Client\src\SelectChaScene.cpp` | Character select scene | ☐ |
| `Client\src\GameAppInterface.cpp` | App interface | ☐ |
| `Client\src\GameAppMsg.cpp` | Message handling | ☐ |

---

## Files with Hard-coded DX8 Types (MUST FIX)

These files bypass the abstraction layer and use DX8 types directly. They will **NOT compile** in DX9 mode without modification:

| File | DX8 References | Required Changes |
|------|----------------|------------------|
| `Engine\include\I_Effect.h` | 13 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Client\src\SMallMap.h` | 10 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Engine\include\MPFont.h` | 8 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Engine\src\I_Effect.cpp` | 5 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Client\src\SMallMap.cpp` | 5 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Engine\include\d3dfont.h` | 4 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Engine\include\EffectFile.h` | 4 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Engine\src\EffectFile.cpp` | 3 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Engine\include\MPModelEff.h` | 3 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Engine\include\MPShadeMap.h` | 2 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Engine\src\MPResManger.cpp` | 2 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Engine\include\MPResManger.h` | 2 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Engine\include\MPParticleCtrl.h` | 1 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Engine\src\d3dfont.cpp` | 1 | Replace `D3DCAPS8`, `D3DVIEWPORT8` with `D3DCAPSX`, `D3DVIEWPORTX` |
| `Engine\src\MPFont.cpp` | 1 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |
| `Client\src\WorldScene.cpp` | 1 | Replace `LPDIRECT3D*8` with `IDirect3D*X*` |

---

## Type Mapping Reference

### Interface Pointers

| DX8 Type | Abstracted Type | DX9 Equivalent |
|----------|-----------------|----------------|
| `LPDIRECT3D8` | `IDirect3DX*` | `LPDIRECT3D9` |
| `LPDIRECT3DDEVICE8` | `IDirect3DDeviceX*` | `LPDIRECT3DDEVICE9` |
| `LPDIRECT3DTEXTURE8` | `IDirect3DTextureX*` | `LPDIRECT3DTEXTURE9` |
| `LPDIRECT3DVERTEXBUFFER8` | `IDirect3DVertexBufferX*` | `LPDIRECT3DVERTEXBUFFER9` |
| `LPDIRECT3DINDEXBUFFER8` | `IDirect3DIndexBufferX*` | `LPDIRECT3DINDEXBUFFER9` |
| `LPDIRECT3DSURFACE8` | `IDirect3DSurfaceX*` | `LPDIRECT3DSURFACE9` |
| `LPDIRECT3DVOLUME8` | `IDirect3DVolumeX*` | `LPDIRECT3DVOLUME9` |
| `LPDIRECT3DBASETEXTURE8` | `IDirect3DBaseTextureX*` | `LPDIRECT3DBASETEXTURE9` |
| `LPDIRECT3DVOLUMETEXTURE8` | `IDirect3DVolumeTextureX*` | `LPDIRECT3DVOLUMETEXTURE9` |
| `LPDIRECT3DCUBETEXTURE8` | `IDirect3DCubeTextureX*` | `LPDIRECT3DCUBETEXTURE9` |

### Struct Types

| DX8 Type | Abstracted Type | DX9 Equivalent |
|----------|-----------------|----------------|
| `D3DCAPS8` | `D3DCAPSX` | `D3DCAPS9` |
| `D3DLIGHT8` | `D3DLIGHTX` | `D3DLIGHT9` |
| `D3DMATERIAL8` | `D3DMATERIALX` | `D3DMATERIAL9` |
| `D3DVIEWPORT8` | `D3DVIEWPORTX` | `D3DVIEWPORT9` |

### Shader Types

| DX8 Type | Abstracted Type | DX9 Equivalent |
|----------|-----------------|----------------|
| `DWORD` (vertex shader handle) | `IDirect3DVertexShaderX` | `IDirect3DVertexShader9*` |
| `DWORD` (pixel shader handle) | `IDirect3DPixelShaderX` | `IDirect3DPixelShader9*` |

---

## API Differences (DX8 vs DX9)

### Functions with Different Signatures

```cpp
// CreateVertexBuffer
// DX8: CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer)
// DX9: CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer, pHandle)

// CreateIndexBuffer
// DX8: CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer)
// DX9: CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer, pHandle)

// SetStreamSource
// DX8: SetStreamSource(StreamNum, StreamData, Stride)
// DX9: SetStreamSource(StreamNum, StreamData, Offset, Stride)

// SetIndices
// DX8: SetIndices(pIndexData, BaseVertexIndex)
// DX9: SetIndices(pIndexData)

// DrawIndexedPrimitive
// DX8: DrawIndexedPrimitive(Type, MinIndex, NumVertices, StartIndex, PrimitiveCount)
// DX9: DrawIndexedPrimitive(Type, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount)

// GetDisplayMode
// DX8: GetDisplayMode(pMode)
// DX9: GetDisplayMode(SwapChain, pMode)

// CreateRenderTarget
// DX8: CreateRenderTarget(Width, Height, Format, MultiSample, Lockable, ppSurface)
// DX9: CreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pHandle)

// CreateDepthStencilSurface
// DX8: CreateDepthStencilSurface(Width, Height, Format, MultiSample, ppSurface)
// DX9: CreateDepthStencilSurface(Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pHandle)
```

### DX9-Only Features

```cpp
// SetFVF (DX9 only, DX8 uses SetVertexShader for FVF)
// DX8: SetVertexShader(fvf)
// DX9: SetFVF(fvf)

// Vertex Declarations (DX9)
typedef D3DVERTEXELEMENT9 D3DVERTEXELEMENTX;
IDirect3DVertexDeclaration9* // DX9 only
```

---

## Migration Steps

### Phase 1: Setup
1. [ ] Copy DX9 SDK headers to `Libraries\DirectX\DX9\Include`
2. [ ] Copy DX9 SDK libraries to `Libraries\DirectX\DX9\Lib`
3. [ ] Create `lwDirectX.h` abstraction layer

### Phase 2: Engine Migration
1. [ ] Update `Engine.vcxproj` with DX9 preprocessor definitions (Release config)
2. [ ] Update all engine headers to use abstracted types
3. [ ] Update all engine source files to use abstracted types
4. [ ] Fix all files with hard-coded DX8 types

### Phase 3: Client Migration
1. [ ] Update `Client.vcxproj` with DX9 preprocessor definitions (Release config)
2. [ ] Update all client files to use abstracted types

### Phase 4: Testing
1. [ ] Build in Debug mode (DX8) - verify no regressions
2. [ ] Build in Release mode (DX9) - verify DX9 works
3. [ ] Test all rendering features
4. [ ] Test all effects and particles
5. [ ] Test UI rendering

---

## Notes

- The `new_generations_src` project still uses DX8 for Debug builds
- Release builds use DX9
- Some files have incomplete migration (hard-coded DX8 types)
- `d3dfont.h/cpp` files are from Microsoft SDK and may need replacement or heavy modification

---

## References

- [new_generations_src Engine](../new_generations_src/Engine/)
- [new_generations_src Client](../new_generations_src/Client/)
- [DirectX 9 Documentation](https://docs.microsoft.com/en-us/windows/win32/direct3d9/dx9-graphics)
