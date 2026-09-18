//
#pragma once

// Shared D3D9 math / viewport / resource typedefs used by both backends.
// DX11-only: d3d9.h for enums/device pointer type; DirectXMath via lwD3DXCompat.h;
// resource wrappers inherit lwDx11I* instead of IDirect3D*9.

#include "MindPowerRenderConfig.h"

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif

#include <d3d9.h>
#include <d3d9types.h>

#if MINDPOWER_USE_D3D9_DEVICE
#include <d3dx9.h>
#else
#include "lwD3DXCompat.h"
#include "lwD3D11ResourceIface.h"
#endif

#if MINDPOWER_USE_D3D9_DEVICE
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
typedef IDirect3DPixelShader9 IDirect3DPixelShaderX;
#else
typedef IDirect3D9 IDirect3DX;
typedef IDirect3DDevice9 IDirect3DDeviceX;
typedef lwDx11ITexture IDirect3DTextureX;
typedef lwDx11IVertexBuffer IDirect3DVertexBufferX;
typedef lwDx11IIndexBuffer IDirect3DIndexBufferX;
typedef lwDx11ISurface IDirect3DSurfaceX;
typedef IDirect3DVolume9 IDirect3DVolumeX;
typedef lwDx11IBaseTexture IDirect3DBaseTextureX;
typedef IDirect3DVolumeTexture9 IDirect3DVolumeTextureX;
typedef IDirect3DCubeTexture9 IDirect3DCubeTextureX;
typedef lwDx11IVertexShader IDirect3DVertexShaderX;
typedef lwDx11IVertexDecl IDirect3DVertexDeclarationX;
typedef IDirect3DPixelShader9 IDirect3DPixelShaderX;
#endif

typedef IDirect3DVertexShaderX* SHADER_TYPE;

typedef D3DLIGHT9 D3DLIGHTX;
typedef D3DMATERIAL9 D3DMATERIALX;
typedef D3DVERTEXELEMENT9 D3DVERTEXELEMENTX;
typedef D3DVIEWPORT9 D3DVIEWPORTX;
typedef void D3DLOCK_TYPE;

typedef D3DCAPS9 D3DCAPSX;
