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

struct MeshNativeDrawSnap
{
    int alpha;
    D3D11_BLEND src;
    D3D11_BLEND dest;
    int zenable;
    int zwrite;
    D3D11_CULL_MODE cull;
    int msaa;
    int lighting;
    DWORD ambient;
    DWORD tfactor;
    int atest;
    DWORD aref;
    D3D11_COMPARISON_FUNC afunc;
    MeshColorOp cop[3];
    MeshColorArg ca1[3];
    MeshColorArg ca2[3];
    MeshColorArg aa1[3];
    MeshColorArg aa2[3];
    int uv_xform[3];
    D3D11_TEXTURE_ADDRESS_MODE samp_addr;
    int samp_point;
};

inline D3D11_BLEND lwD3D11MeshMapBlend(DWORD d)
{
    if (!d || d == 0xffffffff)
        return (D3D11_BLEND)0;
    switch (d)
    {
    case D3DBLEND_ZERO: return D3D11_BLEND_ZERO;
    case D3DBLEND_ONE: return D3D11_BLEND_ONE;
    case D3DBLEND_SRCCOLOR: return D3D11_BLEND_SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR: return D3D11_BLEND_INV_SRC_COLOR;
    case D3DBLEND_SRCALPHA: return D3D11_BLEND_SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA: return D3D11_BLEND_INV_SRC_ALPHA;
    case D3DBLEND_DESTALPHA: return D3D11_BLEND_DEST_ALPHA;
    case D3DBLEND_INVDESTALPHA: return D3D11_BLEND_INV_DEST_ALPHA;
    case D3DBLEND_DESTCOLOR: return D3D11_BLEND_DEST_COLOR;
    case D3DBLEND_INVDESTCOLOR: return D3D11_BLEND_INV_DEST_COLOR;
    case D3DBLEND_SRCALPHASAT: return D3D11_BLEND_SRC_ALPHA_SAT;
    default: return D3D11_BLEND_ONE;
    }
}

inline DWORD lwD3D11MeshUnmapBlend(D3D11_BLEND b)
{
    switch (b)
    {
    case D3D11_BLEND_ZERO: return D3DBLEND_ZERO;
    case D3D11_BLEND_ONE: return D3DBLEND_ONE;
    case D3D11_BLEND_SRC_COLOR: return D3DBLEND_SRCCOLOR;
    case D3D11_BLEND_INV_SRC_COLOR: return D3DBLEND_INVSRCCOLOR;
    case D3D11_BLEND_SRC_ALPHA: return D3DBLEND_SRCALPHA;
    case D3D11_BLEND_INV_SRC_ALPHA: return D3DBLEND_INVSRCALPHA;
    case D3D11_BLEND_DEST_ALPHA: return D3DBLEND_DESTALPHA;
    case D3D11_BLEND_INV_DEST_ALPHA: return D3DBLEND_INVDESTALPHA;
    case D3D11_BLEND_DEST_COLOR: return D3DBLEND_DESTCOLOR;
    case D3D11_BLEND_INV_DEST_COLOR: return D3DBLEND_INVDESTCOLOR;
    case D3D11_BLEND_SRC_ALPHA_SAT: return D3DBLEND_SRCALPHASAT;
    default: return 0;
    }
}

inline D3D11_CULL_MODE lwD3D11MeshMapCull(DWORD d)
{
    if (d == D3DCULL_NONE)
        return D3D11_CULL_NONE;
    if (d == D3DCULL_CW)
        return D3D11_CULL_FRONT;
    return D3D11_CULL_BACK;
}

inline DWORD lwD3D11MeshUnmapCull(D3D11_CULL_MODE c)
{
    if (c == D3D11_CULL_NONE)
        return D3DCULL_NONE;
    if (c == D3D11_CULL_FRONT)
        return D3DCULL_CW;
    return D3DCULL_CCW;
}

inline D3D11_COMPARISON_FUNC lwD3D11MeshMapCmp(DWORD d)
{
    switch (d)
    {
    case D3DCMP_NEVER: return D3D11_COMPARISON_NEVER;
    case D3DCMP_LESS: return D3D11_COMPARISON_LESS;
    case D3DCMP_EQUAL: return D3D11_COMPARISON_EQUAL;
    case D3DCMP_LESSEQUAL: return D3D11_COMPARISON_LESS_EQUAL;
    case D3DCMP_GREATER: return D3D11_COMPARISON_GREATER;
    case D3DCMP_NOTEQUAL: return D3D11_COMPARISON_NOT_EQUAL;
    case D3DCMP_GREATEREQUAL: return D3D11_COMPARISON_GREATER_EQUAL;
    default: return D3D11_COMPARISON_ALWAYS;
    }
}

inline DWORD lwD3D11MeshUnmapCmp(D3D11_COMPARISON_FUNC c)
{
    switch (c)
    {
    case D3D11_COMPARISON_NEVER: return D3DCMP_NEVER;
    case D3D11_COMPARISON_LESS: return D3DCMP_LESS;
    case D3D11_COMPARISON_EQUAL: return D3DCMP_EQUAL;
    case D3D11_COMPARISON_LESS_EQUAL: return D3DCMP_LESSEQUAL;
    case D3D11_COMPARISON_GREATER: return D3DCMP_GREATER;
    case D3D11_COMPARISON_NOT_EQUAL: return D3DCMP_NOTEQUAL;
    case D3D11_COMPARISON_GREATER_EQUAL: return D3DCMP_GREATEREQUAL;
    default: return D3DCMP_ALWAYS;
    }
}

inline MeshColorOp lwD3D11MeshMapColorOp(DWORD op)
{
    switch (op)
    {
    case D3DTOP_DISABLE: return MESH_COP_DISABLE;
    case D3DTOP_SELECTARG1: return MESH_COP_SELECTARG1;
    case D3DTOP_SELECTARG2: return MESH_COP_SELECTARG2;
    case D3DTOP_MODULATE: return MESH_COP_MODULATE;
    case D3DTOP_MODULATE2X: return MESH_COP_MODULATE2X;
    case D3DTOP_ADD: return MESH_COP_ADD;
    case D3DTOP_ADDSMOOTH: return MESH_COP_ADDSMOOTH;
    case D3DTOP_ADDSIGNED: return MESH_COP_ADDSIGNED;
    case D3DTOP_ADDSIGNED2X: return MESH_COP_ADDSIGNED2X;
    case D3DTOP_MODULATEALPHA_ADDCOLOR: return MESH_COP_MODULATEALPHA_ADDCOLOR;
    default:
        if (!op || op == 0xffffffff)
            return MESH_COP_DISABLE;
        return MESH_COP_OTHER;
    }
}

inline DWORD lwD3D11MeshUnmapColorOp(MeshColorOp op)
{
    switch (op)
    {
    case MESH_COP_DISABLE: return D3DTOP_DISABLE;
    case MESH_COP_SELECTARG1: return D3DTOP_SELECTARG1;
    case MESH_COP_SELECTARG2: return D3DTOP_SELECTARG2;
    case MESH_COP_MODULATE: return D3DTOP_MODULATE;
    case MESH_COP_MODULATE2X: return D3DTOP_MODULATE2X;
    case MESH_COP_ADD: return D3DTOP_ADD;
    case MESH_COP_ADDSMOOTH: return D3DTOP_ADDSMOOTH;
    case MESH_COP_ADDSIGNED: return D3DTOP_ADDSIGNED;
    case MESH_COP_ADDSIGNED2X: return D3DTOP_ADDSIGNED2X;
    case MESH_COP_MODULATEALPHA_ADDCOLOR: return D3DTOP_MODULATEALPHA_ADDCOLOR;
    default: return D3DTOP_MODULATE;
    }
}

inline MeshColorArg lwD3D11MeshMapColorArg(DWORD a)
{
    switch (a & 0x0f)
    {
    case D3DTA_CURRENT: return MESH_CA_CURRENT;
    case D3DTA_TEXTURE: return MESH_CA_TEXTURE;
    case D3DTA_TFACTOR: return MESH_CA_TFACTOR;
    case D3DTA_DIFFUSE: return MESH_CA_DIFFUSE;
    default: return MESH_CA_OTHER;
    }
}

inline DWORD lwD3D11MeshUnmapColorArg(MeshColorArg a)
{
    switch (a)
    {
    case MESH_CA_CURRENT: return D3DTA_CURRENT;
    case MESH_CA_TEXTURE: return D3DTA_TEXTURE;
    case MESH_CA_TFACTOR: return D3DTA_TFACTOR;
    default: return D3DTA_DIFFUSE;
    }
}

inline D3D11_TEXTURE_ADDRESS_MODE lwD3D11MeshMapAddr(DWORD d)
{
    if (d == D3DTADDRESS_CLAMP)
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    if (d == D3DTADDRESS_MIRROR)
        return D3D11_TEXTURE_ADDRESS_MIRROR;
    if (d == D3DTADDRESS_BORDER)
        return D3D11_TEXTURE_ADDRESS_BORDER;
    return D3D11_TEXTURE_ADDRESS_WRAP;
}

inline DWORD lwD3D11MeshUnmapAddr(D3D11_TEXTURE_ADDRESS_MODE a)
{
    if (a == D3D11_TEXTURE_ADDRESS_CLAMP)
        return D3DTADDRESS_CLAMP;
    if (a == D3D11_TEXTURE_ADDRESS_MIRROR)
        return D3DTADDRESS_MIRROR;
    if (a == D3D11_TEXTURE_ADDRESS_BORDER)
        return D3DTADDRESS_BORDER;
    return D3DTADDRESS_WRAP;
}

enum MeshRsaField
{
    MESH_RSA_NATIVE = 0x8000,
    MESH_RSA_ALPHA = 0x8000,
    MESH_RSA_SRCBLEND,
    MESH_RSA_DESTBLEND,
    MESH_RSA_ZENABLE,
    MESH_RSA_ZWRITE,
    MESH_RSA_CULL,
    MESH_RSA_MSAA,
    MESH_RSA_LIGHTING,
    MESH_RSA_AMBIENT,
    MESH_RSA_TFACTOR,
    MESH_RSA_ATEST,
    MESH_RSA_AREF,
    MESH_RSA_AFUNC,
    MESH_RSA_COP,
    MESH_RSA_CA1,
    MESH_RSA_CA2,
    MESH_RSA_AA1,
    MESH_RSA_AA2,
    MESH_RSA_UVXFORM,
    MESH_RSA_SAMP_ADDR,
    MESH_RSA_SAMP_POINT
};

inline int lwMeshRsaIsNative(DWORD state)
{
    return (state & MESH_RSA_NATIVE) != 0 ? 1 : 0;
}

inline int lwMeshRsaIsTss(DWORD state)
{
    return (state >= MESH_RSA_COP && state <= MESH_RSA_UVXFORM) ? 1 : 0;
}

inline int lwMeshRsaIsSamp(DWORD state)
{
    return (state == MESH_RSA_SAMP_ADDR || state == MESH_RSA_SAMP_POINT) ? 1 : 0;
}

inline DWORD lwMeshRsaFieldFromD3D9(DWORD state)
{
    switch (state)
    {
    case D3DRS_ALPHABLENDENABLE: return MESH_RSA_ALPHA;
    case D3DRS_SRCBLEND: return MESH_RSA_SRCBLEND;
    case D3DRS_DESTBLEND: return MESH_RSA_DESTBLEND;
    case D3DRS_ZENABLE: return MESH_RSA_ZENABLE;
    case D3DRS_ZWRITEENABLE: return MESH_RSA_ZWRITE;
    case D3DRS_CULLMODE: return MESH_RSA_CULL;
    case D3DRS_MULTISAMPLEANTIALIAS: return MESH_RSA_MSAA;
    case D3DRS_LIGHTING: return MESH_RSA_LIGHTING;
    case D3DRS_AMBIENT: return MESH_RSA_AMBIENT;
    case D3DRS_TEXTUREFACTOR: return MESH_RSA_TFACTOR;
    case D3DRS_ALPHATESTENABLE: return MESH_RSA_ATEST;
    case D3DRS_ALPHAREF: return MESH_RSA_AREF;
    case D3DRS_ALPHAFUNC: return MESH_RSA_AFUNC;
    case D3DTSS_COLOROP: return MESH_RSA_COP;
    case D3DTSS_COLORARG1: return MESH_RSA_CA1;
    case D3DTSS_COLORARG2: return MESH_RSA_CA2;
    case D3DTSS_ALPHAARG1: return MESH_RSA_AA1;
    case D3DTSS_ALPHAARG2: return MESH_RSA_AA2;
    default: return 0;
    }
}

inline DWORD lwMeshRsaFieldFromD3D9Samp(DWORD state)
{
    if (state == D3DSAMP_ADDRESSU)
        return MESH_RSA_SAMP_ADDR;
    if (state == D3DSAMP_MAGFILTER)
        return MESH_RSA_SAMP_POINT;
    return 0;
}

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
MINDPOWER_API void lwD3D11MeshSetCombinerColorArg(int stage, int arg, MeshColorArg value);
MINDPOWER_API void lwD3D11MeshSetCombinerAlphaArg(int stage, int arg, MeshColorArg value);
MINDPOWER_API void lwD3D11MeshSetLighting(int enabled, DWORD ambient);
MINDPOWER_API void lwD3D11MeshSetAmbient(DWORD ambient);
MINDPOWER_API void lwD3D11MeshSetZEnable(int enabled);
MINDPOWER_API void lwD3D11MeshSetZWrite(int enabled);
MINDPOWER_API void lwD3D11MeshSetCull(D3D11_CULL_MODE cull);
MINDPOWER_API void lwD3D11MeshSetMsaa(int enabled);
MINDPOWER_API void lwD3D11MeshSetAlphaTest(int enabled);
MINDPOWER_API void lwD3D11MeshSetAlphaRef(DWORD aref);
MINDPOWER_API void lwD3D11MeshSetAlphaFunc(D3D11_COMPARISON_FUNC fn);
MINDPOWER_API void lwD3D11MeshSetTFactor(DWORD tf);
MINDPOWER_API void lwD3D11MeshSetUvXform(int stage, int enabled);
MINDPOWER_API void lwD3D11MeshSetSampAddr(D3D11_TEXTURE_ADDRESS_MODE addr);
MINDPOWER_API void lwD3D11MeshSetSampPoint(int point);
MINDPOWER_API void lwD3D11MeshGetDraw(MeshNativeDrawSnap* out);
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
