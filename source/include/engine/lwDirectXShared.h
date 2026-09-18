//
#pragma once

// Shared D3D9 math / viewport / resource typedefs used by both backends.
// D3D9 device-create macros and d3d9/d3dx9 lib pragmas live in lwDirectX.h
// behind MINDPOWER_USE_D3D9_DEVICE. COM wrappers still inherit IDirect3D*9
// until Phase 5 removes the D3D9 backend.

#include "MindPowerRenderConfig.h"

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif

#include <d3d9.h>
#include <d3dx9.h>
#include <d3d9types.h>

typedef IDirect3D9 IDirect3DX;
typedef IDirect3DDevice9 IDirect3DDeviceX;
typedef IDirect3DTexture9 IDirect3DTextureX;
typedef IDirect3DVertexBuffer9 IDirect3DVertexBufferX;
typedef IDirect3DIndexBuffer9 IDirect3DIndexBufferX;
typedef IDirect3DSurface9 IDirect3DSurfaceX;
typedef IDirect3DVolume9 IDirect3DVolumeX;
typedef IDirect3DBaseTexture9 IDirect3DBaseTextureX;
typedef IDirect3DVolumeTexture9 IDirect3DVolumeTextureX;
typedef IDirect3DCubeTexture9 IDirect3DCubeTextureX;

typedef IDirect3DVertexShader9 IDirect3DVertexShaderX;
typedef IDirect3DVertexDeclaration9 IDirect3DVertexDeclarationX;
typedef IDirect3DVertexShaderX* SHADER_TYPE;
typedef IDirect3DPixelShader9 IDirect3DPixelShaderX;

typedef D3DLIGHT9 D3DLIGHTX;
typedef D3DMATERIAL9 D3DMATERIALX;
typedef D3DVERTEXELEMENT9 D3DVERTEXELEMENTX;
typedef D3DVIEWPORT9 D3DVIEWPORTX;
typedef void D3DLOCK_TYPE;

typedef D3DCAPS9 D3DCAPSX;
