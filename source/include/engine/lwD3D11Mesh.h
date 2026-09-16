#pragma once

#include "MindPowerAPI.h"
#include "lwHeader.h"
#include "lwDirectX.h"
#include "lwMath.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

LW_BEGIN

class lwDeviceObject11;

MINDPOWER_API LW_RESULT lwD3D11MeshInit(ID3D11Device* device, ID3D11DeviceContext* context);
MINDPOWER_API void lwD3D11MeshShutdown();

MINDPOWER_API void lwD3D11MeshSetBonePalette(const lwMatrix44* mats, DWORD count);
MINDPOWER_API void lwD3D11MeshSetOutline(int enabled, float width, float r, float g, float b);

MINDPOWER_API LW_RESULT lwD3D11MeshDrawPrimitive(
    lwDeviceObject11* dev,
    D3DPRIMITIVETYPE pt_type,
    UINT start_vertex,
    UINT prim_count);

MINDPOWER_API LW_RESULT lwD3D11MeshDrawIndexed(
    lwDeviceObject11* dev,
    D3DPRIMITIVETYPE pt_type,
    INT base_vert_index,
    UINT start_index,
    UINT prim_count);

LW_END
