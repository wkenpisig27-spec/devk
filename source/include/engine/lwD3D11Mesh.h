#pragma once

#include "MindPowerAPI.h"
#include "lwHeader.h"
#include "lwDirectX.h"
#include "lwMath.h"

LW_BEGIN

class lwDeviceObject11;

enum MeshColorOp
{
    MESH_COP_DISABLE = 0,
    MESH_COP_SELECTARG1,
    MESH_COP_SELECTARG2,
    MESH_COP_MODULATE,
    MESH_COP_MODULATE2X,
    MESH_COP_ADD,
    MESH_COP_ADDSMOOTH,
    MESH_COP_ADDSIGNED,
    MESH_COP_ADDSIGNED2X,
    MESH_COP_MODULATEALPHA_ADDCOLOR,
    MESH_COP_OTHER
};

enum MeshColorArg
{
    MESH_CA_TEXTURE = 0,
    MESH_CA_DIFFUSE,
    MESH_CA_CURRENT,
    MESH_CA_TFACTOR,
    MESH_CA_OTHER
};

MINDPOWER_API D3D11_BLEND lwD3D11MeshMapBlend(DWORD d3d9);

MINDPOWER_API LW_RESULT lwD3D11MeshInit(ID3D11Device* device, ID3D11DeviceContext* context);
MINDPOWER_API void lwD3D11MeshShutdown();

MINDPOWER_API void lwD3D11MeshSetBonePalette(const lwMatrix44* mats, DWORD count);
MINDPOWER_API void lwD3D11MeshSetOutline(int enabled, float width, float r, float g, float b);
MINDPOWER_API int lwD3D11MeshIsOutline();
MINDPOWER_API void lwD3D11MeshSetVisual(
    int stylized,
    int fog,
    int height_fog,
    float fog_r,
    float fog_g,
    float fog_b,
    float fog_density,
    int water_enhance);
MINDPOWER_API void lwD3D11MeshSetSea(int enabled);
MINDPOWER_API void lwD3D11MeshSetCharacter(int enabled);
MINDPOWER_API void lwD3D11MeshSetSceneObject(int enabled);
MINDPOWER_API void lwD3D11MeshSetTranspObject(int enabled);
MINDPOWER_API void lwD3D11MeshSetTerrain(int enabled);
MINDPOWER_API void lwD3D11MeshSetVfx(int enabled);
MINDPOWER_API void lwD3D11MeshHintAdditive(int enabled);
MINDPOWER_API void lwD3D11MeshSetAlpha(int enabled);
MINDPOWER_API void lwD3D11MeshSetBlend(D3D11_BLEND src, D3D11_BLEND dest);
MINDPOWER_API void lwD3D11MeshSetCombiner(int stage, MeshColorOp op);
MINDPOWER_API void lwD3D11MeshSetCombinerArgs(int stage, MeshColorOp op, MeshColorArg arg1, MeshColorArg arg2);
MINDPOWER_API void lwD3D11MeshSetLighting(int enabled, DWORD ambient);
MINDPOWER_API void lwD3D11MeshSetAlphaTest(int enabled);
MINDPOWER_API void lwD3D11MeshSetTFactor(DWORD tf);
MINDPOWER_API void lwD3D11MeshSetUvXform(int stage, int enabled);
MINDPOWER_API void lwD3D11MeshNoteRs(DWORD state, DWORD value);
MINDPOWER_API void lwD3D11MeshNoteTss(DWORD stage, DWORD type, DWORD value);
MINDPOWER_API void lwD3D11MeshNoteSamp(DWORD type, DWORD value);
MINDPOWER_API int lwD3D11MeshReadRs(DWORD state, DWORD* value);
MINDPOWER_API int lwD3D11MeshReadTss(DWORD stage, DWORD type, DWORD* value);
MINDPOWER_API int lwD3D11MeshReadSamp(DWORD type, DWORD* value);
MINDPOWER_API int lwD3D11MeshCompileEff(const char* path);
MINDPOWER_API void lwD3D11MeshSetEffTech(int tech);
MINDPOWER_API int lwD3D11MeshWaterEnhance();

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
