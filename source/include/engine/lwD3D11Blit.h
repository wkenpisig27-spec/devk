#pragma once

#include "MindPowerAPI.h"
#include "lwHeader.h"
#include "lwDirectX.h"

LW_BEGIN

MINDPOWER_API LW_RESULT lwD3D11BlitInit(ID3D11Device* device, ID3D11DeviceContext* context, UINT width, UINT height);
MINDPOWER_API void lwD3D11BlitShutdown();
MINDPOWER_API void lwD3D11BlitSetViewport(UINT width, UINT height);

// D3DXSprite-compatible: src rect in texels, dest in pixels, scale multiplies src size.
MINDPOWER_API LW_RESULT lwD3D11BlitSprite(
    IDirect3DTextureX* tex,
    const RECT* src,
    const D3DXVECTOR2* scale,
    const D3DXVECTOR2* dest,
    DWORD color);

// XYZRHW + DIFFUSE + TEX1 triangle list (bitmap fonts).
MINDPOWER_API LW_RESULT lwD3D11BlitDrawUP(
    const void* verts,
    UINT stride,
    UINT vert_count,
    IDirect3DTextureX* tex,
    int point_filter);

LW_END
