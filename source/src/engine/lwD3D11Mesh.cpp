#include "stdafx.h"
#include "lwD3D11Mesh.h"
#include "lwD3D11Buffer.h"
#include "lwD3D11Texture.h"
#include "lwD3D11Gaps.h"
#include "lwDeviceObject11.h"
#include "lwShaderMgr11.h"
#include "ShaderLoad.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <map>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

LW_BEGIN

static const char* kMeshHLSL =
    "cbuffer CB0 : register(b0) {\n"
    "  row_major float4x4 world;\n"
    "  row_major float4x4 viewProj;\n"
    "  float4 lightDir;\n"
    "  float4 ambient;\n"
    "  float4 diffuse;\n"
    "  float4 flags; /* x=skin, y=hasNrm, z=hasUv, w=blend_floats */\n"
    "  float4 extra; /* x=boneCount, y=outline, z=outlineWidth, w=alphaRef */\n"
    "  float4 outlineColor;\n"
    "  float4 more; /* x=hasColor, y=unlit, z=dualTex, w=tfactor mix */\n"
    "  float4 tfactor;\n"
    "  row_major float4x4 uvMat;\n"
    "  row_major float4x4 uvMat1;\n"
    "  float4 look; /* xyz=eye, w=stylized */\n"
    "  float4 hemiSky;\n"
    "  float4 hemiGnd;\n"
    "  float4 fog; /* rgb + density */\n"
    "  float4 fogMore; /* x=fog, y=heightFog, z=sea, w=flatChar */\n"
    "};\n"
    "cbuffer CB1 : register(b1) {\n"
    "  row_major float4x4 bones[64];\n"
    "};\n"
    "Texture2D tex0 : register(t0);\n"
    "Texture2D tex1 : register(t1);\n"
    "Texture2D tex2 : register(t2);\n"
    "SamplerState samp0 : register(s0);\n"
    "struct VSInRigid { float3 pos : POSITION; float3 nrm : NORMAL; float2 uv : TEXCOORD0; float2 uv1 : TEXCOORD1; float4 col : COLOR; };\n"
    "struct VSInSkin {\n"
    "  float3 pos : POSITION;\n"
    "  float4 blend : BLENDWEIGHT;\n"
    "  uint4 idx : BLENDINDICES;\n"
    "  float3 nrm : NORMAL;\n"
    "  float2 uv : TEXCOORD0;\n"
    "  float2 uv1 : TEXCOORD1;\n"
    "  float4 col : COLOR;\n"
    "};\n"
    "struct PSIn { float4 pos : SV_POSITION; float3 nrm : NORMAL; float2 uv : TEXCOORD0; float2 uv1 : TEXCOORD1; float4 col : COLOR; float3 wpos : TEXCOORD2; };\n"
    "uint4 ClampBones(uint4 i) {\n"
    "  uint n = (uint)extra.x;\n"
    "  if (n == 0) return uint4(0,0,0,0);\n"
    "  uint last = n - 1;\n"
    "  if (last > 63) last = 63;\n"
    "  return uint4(min(i.x,last), min(i.y,last), min(i.z,last), min(i.w,last));\n"
    "}\n"
    "float4 FixWeights(float4 w) {\n"
    "  int bf = (int)(flags.w + 0.5);\n"
    "  if (bf <= 0) return float4(1,0,0,0);\n"
    "  if (bf == 1) return float4(w.x, saturate(1.0 - w.x), 0, 0);\n"
    "  if (bf == 2) return float4(w.x, w.y, saturate(1.0 - w.x - w.y), 0);\n"
    "  if (bf == 3) {\n"
    "    w.w = saturate(1.0 - w.x - w.y - w.z);\n"
    "    return w;\n"
    "  }\n"
    "  float s = w.x + w.y + w.z + w.w;\n"
    "  if (s < 0.0001) return float4(1,0,0,0);\n"
    "  return w / s;\n"
    "}\n"
    "float3 SkinP(float3 p, float4 w, uint4 i) {\n"
    "  float4 hp = float4(p, 1);\n"
    "  float4 r = mul(hp, bones[i.x]) * w.x + mul(hp, bones[i.y]) * w.y\n"
    "           + mul(hp, bones[i.z]) * w.z + mul(hp, bones[i.w]) * w.w;\n"
    "  return r.xyz;\n"
    "}\n"
    "float3 SkinN(float3 n, float4 w, uint4 i) {\n"
    "  float3 r = mul(n, (float3x3)bones[i.x]) * w.x + mul(n, (float3x3)bones[i.y]) * w.y\n"
    "           + mul(n, (float3x3)bones[i.z]) * w.z + mul(n, (float3x3)bones[i.w]) * w.w;\n"
    "  return r;\n"
    "}\n"
    "void FinishVS(inout PSIn o, float3 p, float3 n, float2 uv, float2 uv1, float4 col) {\n"
    "  float4 wp = mul(float4(p, 1), world);\n"
    "  o.nrm = flags.y > 0.5 ? mul(n, (float3x3)world) : float3(0,0,1);\n"
    "  o.pos = mul(wp, viewProj);\n"
    "  if (extra.y > 0.5) {\n"
    "    float2 nxy = mul(float4(normalize(o.nrm), 0), viewProj).xy;\n"
    "    float nlen = length(nxy);\n"
    "    if (nlen > 1e-5) nxy /= nlen;\n"
    "    o.pos.xy += nxy * float2(extra.z, more.w) * o.pos.w;\n"
    "    o.pos.z += 0.0002 * o.pos.w;\n"
    "  }\n"
    "  float2 t = flags.z > 0.5 ? uv : float2(0,0);\n"
    "  o.uv = mul(float3(t, 1), (float3x3)uvMat).xy;\n"
    "  float2 t1 = hemiSky.w > 0.5 ? uv1 : t;\n"
    "  o.uv1 = mul(float3(t1, 1), (float3x3)uvMat1).xy;\n"
    "  o.col = more.x > 0.5 ? col : float4(1,1,1,1);\n"
    "  o.wpos = wp.xyz;\n"
    "}\n"
    "PSIn VSRigid(VSInRigid i) {\n"
    "  PSIn o;\n"
    "  FinishVS(o, i.pos, i.nrm, i.uv, i.uv1, i.col);\n"
    "  return o;\n"
    "}\n"
    "PSIn VSSkin(VSInSkin i) {\n"
    "  PSIn o;\n"
    "  float4 w = FixWeights(i.blend);\n"
    "  uint4 idx = ClampBones(i.idx);\n"
    "  float3 p = flags.x > 0.5 ? SkinP(i.pos, w, idx) : i.pos;\n"
    "  float3 n = flags.x > 0.5 ? SkinN(i.nrm, w, idx) : i.nrm;\n"
    "  FinishVS(o, p, n, i.uv, i.uv1, i.col);\n"
    "  return o;\n"
    "}\n"
    "float4 PSMain(PSIn i) : SV_TARGET {\n"
    "  float4 tex = tex0.Sample(samp0, i.uv);\n"
    "  float z = more.z;\n"
    "  if (z > 0.5) {\n"
    "    float4 t1 = tex1.Sample(samp0, i.uv1);\n"
    "    if (z < 1.5) tex = float4(t1.rgb, tex.a);\n"
    "    else if (z < 2.5) tex.rgb *= t1.rgb;\n"
    "    else if (z < 3.5) tex.rgb = tex.rgb + tex.a * t1.rgb;\n"
    "    else if (z < 4.5) tex.rgb += t1.rgb;\n"
    "    else if (z < 5.5) tex.rgb = tex.rgb + t1.rgb - 0.5;\n"
    "    else if (z < 6.5) tex.rgb *= t1.rgb * 2.0;\n"
    "    else {\n"
    "      float4 t2 = tex2.Sample(samp0, i.uv1);\n"
    "      tex.a *= t1.a;\n"
    "      tex.rgb = tex.rgb + tex.a * t2.rgb;\n"
    "    }\n"
    "  }\n"
    "  if (extra.w >= 0.0) clip(tex.a - extra.w);\n"
    "  if (extra.y > 0.5) return float4(outlineColor.rgb, outlineColor.a * tex.a);\n"
    "  int mix = (int)(more.w + 0.5);\n"
    "  if (more.y > 1.5) {\n"
    "    float sa = (mix & 2) ? tfactor.a : tex.a;\n"
    "    return float4(tfactor.rgb, sa);\n"
    "  }\n"
    "  float3 tint;\n"
    "  if (mix & 1) tint = tfactor.rgb;\n"
    "  else if (more.y > 0.5 || flags.x > 0.5 || fogMore.w > 0.5) tint = i.col.rgb;\n"
    "  else {\n"
    "    float3 n = normalize(i.nrm);\n"
    "    float3 L = -normalize(lightDir.xyz);\n"
    "    float ndl = saturate(dot(n, L));\n"
    "    if (look.w > 0.5) {\n"
    "      float b0 = smoothstep(0.26, 0.44, ndl);\n"
    "      float b1 = smoothstep(0.60, 0.80, ndl);\n"
    "      float band = 0.62 + b0 * 0.22 + b1 * 0.16;\n"
    "      float3 lit = ambient.rgb + diffuse.rgb * band;\n"
    "      lit *= lerp(float3(0.96, 0.97, 1.02), float3(1.02, 1.00, 0.97), band);\n"
    "      float hz = saturate(n.z * 0.5 + 0.5);\n"
    "      lit += lerp(hemiGnd.rgb, hemiSky.rgb, hz);\n"
    "      float3 V = normalize(look.xyz - i.wpos);\n"
    "      float fres = 1.0 - saturate(dot(n, V));\n"
    "      float rim = smoothstep(0.52, 0.82, fres) * 0.10;\n"
    "      lit += float3(1.00, 0.94, 0.88) * rim;\n"
    "      tint = saturate(lit) * i.col.rgb;\n"
    "    } else {\n"
    "      tint = saturate(ambient.rgb + diffuse.rgb * ndl) * i.col.rgb;\n"
    "    }\n"
    "  }\n"
    "  float a = tex.a * ((mix & 2) ? tfactor.a : i.col.a);\n"
    "  float3 rgb = tex.rgb * tint;\n"
    "  if (fogMore.z > 0.5) {\n"
    "    float3 V = normalize(look.xyz - i.wpos);\n"
    "    float fresnel = 1.0 - abs(V.z);\n"
    "    fresnel = fresnel * fresnel;\n"
    "    rgb += float3(70.0, 85.0, 40.0) / 255.0 * fresnel;\n"
    "    a = saturate(a + fresnel * 25.0 / 255.0);\n"
    "  } else if (fogMore.x > 0.5 && extra.y < 0.5 && more.y < 0.5) {\n"
    "    float2 xy = i.wpos.xy - look.xy;\n"
    "    float d = max(length(xy) - 40.0, 0.0);\n"
    "    float dens = fog.w;\n"
    "    if (dens > 0.0 && dens < 0.002) dens = 0.002;\n"
    "    float ed = dens * d;\n"
    "    float f = 1.0 - exp(-ed * ed);\n"
    "    if (fogMore.y > 0.5) {\n"
    "      float hf = saturate((12.0 - i.wpos.z) * 0.04);\n"
    "      f = saturate(f + hf * 0.08);\n"
    "    }\n"
    "    f = min(saturate(f), 0.22);\n"
    "    rgb = lerp(rgb, fog.rgb, f);\n"
    "  }\n"
    "  return float4(rgb, a);\n"
    "}\n";

struct MeshCB0
{
    float world[16];
    float viewProj[16];
    float lightDir[4];
    float ambient[4];
    float diffuse[4];
    float flags[4];
    float extra[4];
    float outlineColor[4];
    float more[4];
    float tfactor[4];
    float uvMat[16];
    float uvMat1[16];
    float look[4];
    float hemiSky[4];
    float hemiGnd[4];
    float fog[4];
    float fogMore[4];
};

struct MeshState
{
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11VertexShader* vs_rigid;
    ID3D11VertexShader* vs_skin;
    ID3D11PixelShader* ps;
    ID3DBlob* vs_rigid_blob;
    ID3DBlob* vs_skin_blob;
    ID3D11Buffer* cb0;
    ID3D11Buffer* cb1;
    ID3D11SamplerState* samp;
    ID3D11SamplerState* samp_clamp;
    ID3D11SamplerState* samp_point;
    ID3D11SamplerState* samp_point_clamp;
    ID3D11RasterizerState* rast_ccw;
    ID3D11RasterizerState* rast_cw;
    ID3D11RasterizerState* rast_none;
    ID3D11RasterizerState* rast_ccw_noaa;
    ID3D11RasterizerState* rast_cw_noaa;
    ID3D11RasterizerState* rast_none_noaa;
    ID3D11DepthStencilState* depth_on;
    ID3D11DepthStencilState* depth_read;
    ID3D11DepthStencilState* depth_off;
    int outline;
    float outline_width;
    float outline_color[4];
    ID3D11BlendState* blend_alpha;
    ID3D11BlendState* blend_opaque;
    ID3D11Texture2D* white_tex;
    ID3D11ShaderResourceView* white_srv;
    lwMatrix44 bones[64];
    DWORD bone_count;
    ID3D11Buffer* fan_ib;
    UINT fan_ib_prims;
    int stylized;
    int character;
    int scene_object;
    int transp_object;
    int terrain;
    int vfx;
    int fog_on;
    int height_fog;
    int sea;
    int water_enhance;
    float fog_color[4];
    float fog_density;
    int ready;
};

static MeshState s_mesh = {};
static std::map<DWORD, ID3D11InputLayout*> s_layouts;
static std::map<DWORD, ID3D11BlendState*> s_blends;

static void CopyMat(float* dst, const lwMatrix44* m)
{
    memcpy(dst, m, sizeof(float) * 16);
}

static int TexUvTransformOn(DWORD ttff)
{
    return (ttff && ttff != D3DTTFF_DISABLE && ttff != 0xffffffff && ttff != D3DTSS_FORCE_DWORD) ? 1 : 0;
}

static void EyeFromView(const lwMatrix44* v, float* eye)
{
    if (!v)
    {
        eye[0] = eye[1] = eye[2] = 0.0f;
        return;
    }
    eye[0] = -(v->_41 * v->_11 + v->_42 * v->_21 + v->_43 * v->_31);
    eye[1] = -(v->_41 * v->_12 + v->_42 * v->_22 + v->_43 * v->_32);
    eye[2] = -(v->_41 * v->_13 + v->_42 * v->_23 + v->_43 * v->_33);
}

static void ReleaseMesh()
{
    for (std::map<DWORD, ID3D11InputLayout*>::iterator it = s_layouts.begin(); it != s_layouts.end(); ++it)
    {
        if (it->second)
            it->second->Release();
    }
    s_layouts.clear();

    for (std::map<DWORD, ID3D11BlendState*>::iterator it = s_blends.begin(); it != s_blends.end(); ++it)
    {
        if (it->second)
            it->second->Release();
    }
    s_blends.clear();

    if (s_mesh.fan_ib) s_mesh.fan_ib->Release();
    if (s_mesh.white_srv) s_mesh.white_srv->Release();
    if (s_mesh.white_tex) s_mesh.white_tex->Release();
    if (s_mesh.blend_opaque) s_mesh.blend_opaque->Release();
    if (s_mesh.blend_alpha) s_mesh.blend_alpha->Release();
    if (s_mesh.depth_off) s_mesh.depth_off->Release();
    if (s_mesh.depth_read) s_mesh.depth_read->Release();
    if (s_mesh.depth_on) s_mesh.depth_on->Release();
    if (s_mesh.rast_none_noaa) s_mesh.rast_none_noaa->Release();
    if (s_mesh.rast_cw_noaa) s_mesh.rast_cw_noaa->Release();
    if (s_mesh.rast_ccw_noaa) s_mesh.rast_ccw_noaa->Release();
    if (s_mesh.rast_none) s_mesh.rast_none->Release();
    if (s_mesh.rast_cw) s_mesh.rast_cw->Release();
    if (s_mesh.rast_ccw) s_mesh.rast_ccw->Release();
    if (s_mesh.samp_point_clamp) s_mesh.samp_point_clamp->Release();
    if (s_mesh.samp_point) s_mesh.samp_point->Release();
    if (s_mesh.samp_clamp) s_mesh.samp_clamp->Release();
    if (s_mesh.samp) s_mesh.samp->Release();
    if (s_mesh.cb1) s_mesh.cb1->Release();
    if (s_mesh.cb0) s_mesh.cb0->Release();
    if (s_mesh.ps) s_mesh.ps->Release();
    if (s_mesh.vs_skin) s_mesh.vs_skin->Release();
    if (s_mesh.vs_rigid) s_mesh.vs_rigid->Release();
    if (s_mesh.vs_skin_blob) s_mesh.vs_skin_blob->Release();
    if (s_mesh.vs_rigid_blob) s_mesh.vs_rigid_blob->Release();
    memset(&s_mesh, 0, sizeof(s_mesh));
}

static ID3DBlob* Compile(const char* entry, const char* target)
{
    ID3DBlob* blob = 0;
    ID3DBlob* err = 0;
    HRESULT hr = D3DCompile(kMeshHLSL, strlen(kMeshHLSL), "lwD3D11Mesh", 0, 0, entry, target,
        D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, &blob, &err);
    if (FAILED(hr))
    {
        if (err)
        {
            lwD3D11Gap(LW_D3D11_GAP, "mesh-hlsl", "%s", (const char*)err->GetBufferPointer());
            err->Release();
        }
        return 0;
    }
    if (err)
        err->Release();
    return blob;
}

static ID3D11RasterizerState* MakeRast(ID3D11Device* device, D3D11_CULL_MODE cull, BOOL msaa)
{
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = cull;
    rd.FrontCounterClockwise = FALSE;
    rd.DepthClipEnable = FALSE;
    rd.MultisampleEnable = msaa ? TRUE : FALSE;
    rd.AntialiasedLineEnable = msaa ? TRUE : FALSE;
    ID3D11RasterizerState* rs = 0;
    device->CreateRasterizerState(&rd, &rs);
    return rs;
}

LW_RESULT lwD3D11MeshInit(ID3D11Device* device, ID3D11DeviceContext* context)
{
    lwD3D11MeshShutdown();
    if (!device || !context)
        return LW_RET_FAILED;

    s_mesh.device = device;
    s_mesh.context = context;

    s_mesh.vs_rigid_blob = Compile("VSRigid", "vs_4_0");
    s_mesh.vs_skin_blob = Compile("VSSkin", "vs_4_0");
    ID3DBlob* psb = Compile("PSMain", "ps_4_0");
    if (!s_mesh.vs_rigid_blob || !s_mesh.vs_skin_blob || !psb)
    {
        if (psb) psb->Release();
        ReleaseMesh();
        return LW_RET_FAILED;
    }

    HRESULT hr = device->CreateVertexShader(s_mesh.vs_rigid_blob->GetBufferPointer(), s_mesh.vs_rigid_blob->GetBufferSize(), 0, &s_mesh.vs_rigid);
    if (SUCCEEDED(hr))
        hr = device->CreateVertexShader(s_mesh.vs_skin_blob->GetBufferPointer(), s_mesh.vs_skin_blob->GetBufferSize(), 0, &s_mesh.vs_skin);
    if (SUCCEEDED(hr))
        hr = device->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), 0, &s_mesh.ps);
    psb->Release();
    if (FAILED(hr))
    {
        ReleaseMesh();
        return LW_RET_FAILED;
    }

    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth = sizeof(MeshCB0);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&cbd, 0, &s_mesh.cb0)))
    {
        ReleaseMesh();
        return LW_RET_FAILED;
    }
    cbd.ByteWidth = sizeof(lwMatrix44) * 64;
    if (FAILED(device->CreateBuffer(&cbd, 0, &s_mesh.cb1)))
    {
        ReleaseMesh();
        return LW_RET_FAILED;
    }

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    device->CreateSamplerState(&sd, &s_mesh.samp);
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device->CreateSamplerState(&sd, &s_mesh.samp_clamp);
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    device->CreateSamplerState(&sd, &s_mesh.samp_point);
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device->CreateSamplerState(&sd, &s_mesh.samp_point_clamp);

    s_mesh.rast_ccw = MakeRast(device, D3D11_CULL_BACK, TRUE);
    s_mesh.rast_cw = MakeRast(device, D3D11_CULL_FRONT, TRUE);
    s_mesh.rast_none = MakeRast(device, D3D11_CULL_NONE, TRUE);
    s_mesh.rast_ccw_noaa = MakeRast(device, D3D11_CULL_BACK, FALSE);
    s_mesh.rast_cw_noaa = MakeRast(device, D3D11_CULL_FRONT, FALSE);
    s_mesh.rast_none_noaa = MakeRast(device, D3D11_CULL_NONE, FALSE);

    D3D11_DEPTH_STENCIL_DESC dd = {};
    dd.DepthEnable = TRUE;
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    device->CreateDepthStencilState(&dd, &s_mesh.depth_on);
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    device->CreateDepthStencilState(&dd, &s_mesh.depth_read);
    dd.DepthEnable = FALSE;
    device->CreateDepthStencilState(&dd, &s_mesh.depth_off);

    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&bd, &s_mesh.blend_opaque);
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    device->CreateBlendState(&bd, &s_mesh.blend_alpha);

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = 1;
    td.Height = 1;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    UINT white = 0xffffffff;
    D3D11_SUBRESOURCE_DATA srd = {};
    srd.pSysMem = &white;
    srd.SysMemPitch = 4;
    if (SUCCEEDED(device->CreateTexture2D(&td, &srd, &s_mesh.white_tex)))
        device->CreateShaderResourceView(s_mesh.white_tex, 0, &s_mesh.white_srv);

    for (int i = 0; i < 64; ++i)
        lwMatrix44Identity(&s_mesh.bones[i]);
    s_mesh.bone_count = 0;
    s_mesh.outline = 0;
    s_mesh.outline_width = 0.014f;
    s_mesh.outline_color[0] = 0.33f;
    s_mesh.outline_color[1] = 0.25f;
    s_mesh.outline_color[2] = 0.20f;
    s_mesh.outline_color[3] = 0.70f;
    s_mesh.stylized = 1;
    s_mesh.character = 0;
    s_mesh.scene_object = 0;
    s_mesh.transp_object = 0;
    s_mesh.terrain = 0;
    s_mesh.fog_on = 1;
    s_mesh.height_fog = 0;
    s_mesh.sea = 0;
    s_mesh.water_enhance = 1;
    s_mesh.fog_color[0] = 185.0f / 255.0f;
    s_mesh.fog_color[1] = 195.0f / 255.0f;
    s_mesh.fog_color[2] = 208.0f / 255.0f;
    s_mesh.fog_color[3] = 1.0f;
    s_mesh.fog_density = 0.00035f;
    s_mesh.ready = 1;
    return LW_RET_OK;
}

void lwD3D11MeshShutdown()
{
    ReleaseMesh();
}

void lwD3D11MeshSetBonePalette(const lwMatrix44* mats, DWORD count)
{
    if (!mats || count == 0)
    {
        s_mesh.bone_count = 0;
        for (int i = 0; i < 64; ++i)
            lwMatrix44Identity(&s_mesh.bones[i]);
        return;
    }
    if (count > 64)
        count = 64;
    memcpy(s_mesh.bones, mats, sizeof(lwMatrix44) * count);
    for (DWORD i = count; i < 64; ++i)
        lwMatrix44Identity(&s_mesh.bones[i]);
    s_mesh.bone_count = count;
}

void lwD3D11MeshSetOutline(int enabled, float width, float r, float g, float b)
{
    s_mesh.outline = enabled ? 1 : 0;
    if (width > 0.0f)
        s_mesh.outline_width = width;
    s_mesh.outline_color[0] = r;
    s_mesh.outline_color[1] = g;
    s_mesh.outline_color[2] = b;
    s_mesh.outline_color[3] = 0.70f;
}

void lwD3D11MeshSetVisual(
    int stylized,
    int fog,
    int height_fog,
    float fog_r,
    float fog_g,
    float fog_b,
    float fog_density,
    int water_enhance)
{
    s_mesh.stylized = stylized ? 1 : 0;
    s_mesh.fog_on = fog ? 1 : 0;
    s_mesh.height_fog = height_fog ? 1 : 0;
    s_mesh.fog_color[0] = fog_r;
    s_mesh.fog_color[1] = fog_g;
    s_mesh.fog_color[2] = fog_b;
    s_mesh.fog_color[3] = 1.0f;
    s_mesh.fog_density = fog_density;
    s_mesh.water_enhance = water_enhance ? 1 : 0;
}

void lwD3D11MeshSetSea(int enabled)
{
    s_mesh.sea = enabled ? 1 : 0;
}

void lwD3D11MeshSetCharacter(int enabled)
{
    s_mesh.character = enabled ? 1 : 0;
}

void lwD3D11MeshSetSceneObject(int enabled)
{
    s_mesh.scene_object = enabled ? 1 : 0;
}

void lwD3D11MeshSetTranspObject(int enabled)
{
    s_mesh.transp_object = enabled ? 1 : 0;
}

void lwD3D11MeshSetTerrain(int enabled)
{
    s_mesh.terrain = enabled ? 1 : 0;
}

void lwD3D11MeshSetVfx(int enabled)
{
    s_mesh.vfx = enabled ? 1 : 0;
}

int lwD3D11MeshWaterEnhance()
{
    return s_mesh.water_enhance ? 1 : 0;
}

int lwD3D11MeshIsOutline()
{
    return s_mesh.outline ? 1 : 0;
}

struct FvfInfo
{
    UINT stride;
    UINT nrm_off;
    UINT uv_off;
    UINT blend_off;
    UINT idx_off;
    UINT diff_off;
    UINT uv1_off;
    int has_nrm;
    int has_uv;
    int has_blend;
    int has_diff;
    int blend_floats;
    int ntex;
};

static FvfInfo ParseFvf(DWORD fvf)
{
    FvfInfo i = {};
    UINT o = 0;
    DWORD pos = fvf & D3DFVF_POSITION_MASK;
    int betas = 0;
    switch (pos)
    {
    case D3DFVF_XYZ: o = 12; break;
    case D3DFVF_XYZRHW:
    case D3DFVF_XYZW: o = 16; break;
    case D3DFVF_XYZB1: o = 16; betas = 1; break;
    case D3DFVF_XYZB2: o = 20; betas = 2; break;
    case D3DFVF_XYZB3: o = 24; betas = 3; break;
    case D3DFVF_XYZB4: o = 28; betas = 4; break;
    case D3DFVF_XYZB5: o = 32; betas = 5; break;
    default: o = 12; break;
    }

    const int last_ubyte = (fvf & (D3DFVF_LASTBETA_UBYTE4 | D3DFVF_LASTBETA_D3DCOLOR)) ? 1 : 0;
    if (last_ubyte && betas > 0)
    {
        i.has_blend = 1;
        i.blend_off = 12;
        i.blend_floats = betas - 1;
        i.idx_off = 12 + i.blend_floats * 4;
    }
    else if (last_ubyte)
    {
        // XYZ|LASTBETA_UBYTE4|... : DWORD bone index after position, weight 1.
        i.has_blend = 1;
        i.blend_off = 12;
        i.blend_floats = 0;
        i.idx_off = 12;
        o += 4;
    }

    if (fvf & D3DFVF_NORMAL)
    {
        i.has_nrm = 1;
        i.nrm_off = o;
        o += 12;
    }
    if (fvf & D3DFVF_PSIZE)
        o += 4;
    if (fvf & D3DFVF_DIFFUSE)
    {
        i.has_diff = 1;
        i.diff_off = o;
        o += 4;
    }
    if (fvf & D3DFVF_SPECULAR)
        o += 4;

    int ntex = (int)((fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT);
    i.ntex = ntex;
    if (ntex > 0)
    {
        i.has_uv = 1;
        i.uv_off = o;
        i.uv1_off = o + 8;
        o += 8 * ntex;
    }
    i.stride = o;
    return i;
}

static DXGI_FORMAT BlendWeightFormat(int floats)
{
    switch (floats)
    {
    case 1: return DXGI_FORMAT_R32_FLOAT;
    case 2: return DXGI_FORMAT_R32G32_FLOAT;
    case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
    default: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    }
}

static ID3D11InputLayout* LayoutFor(DWORD fvf, const FvfInfo& info, int skin)
{
    DWORD key = fvf ^ (skin ? 0x80000000u : 0);
    std::map<DWORD, ID3D11InputLayout*>::iterator it = s_layouts.find(key);
    if (it != s_layouts.end())
        return it->second;

    ID3DBlob* blob = skin ? s_mesh.vs_skin_blob : s_mesh.vs_rigid_blob;
    if (!blob)
        return 0;

    D3D11_INPUT_ELEMENT_DESC elems[8] = {};
    UINT n = 0;
    elems[n].SemanticName = "POSITION";
    elems[n].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    elems[n].AlignedByteOffset = 0;
    elems[n].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    ++n;

    if (skin)
    {
        elems[n].SemanticName = "BLENDWEIGHT";
        if (info.blend_floats > 0)
        {
            elems[n].Format = BlendWeightFormat(info.blend_floats);
            elems[n].AlignedByteOffset = info.blend_off;
        }
        else
        {
            elems[n].Format = DXGI_FORMAT_R32_FLOAT;
            elems[n].AlignedByteOffset = 0;
        }
        elems[n].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        ++n;

        elems[n].SemanticName = "BLENDINDICES";
        elems[n].Format = DXGI_FORMAT_R8G8B8A8_UINT;
        elems[n].AlignedByteOffset = info.idx_off;
        elems[n].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        ++n;
    }

    elems[n].SemanticName = "NORMAL";
    elems[n].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    elems[n].AlignedByteOffset = info.has_nrm ? info.nrm_off : 0;
    elems[n].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    ++n;

    elems[n].SemanticName = "TEXCOORD";
    elems[n].Format = DXGI_FORMAT_R32G32_FLOAT;
    elems[n].AlignedByteOffset = info.has_uv ? info.uv_off : 0;
    elems[n].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    ++n;

    elems[n].SemanticName = "TEXCOORD";
    elems[n].SemanticIndex = 1;
    elems[n].Format = DXGI_FORMAT_R32G32_FLOAT;
    elems[n].AlignedByteOffset = (info.ntex >= 2) ? info.uv1_off : (info.has_uv ? info.uv_off : 0);
    elems[n].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    ++n;

    elems[n].SemanticName = "COLOR";
    elems[n].Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    elems[n].AlignedByteOffset = info.has_diff ? info.diff_off : 0;
    elems[n].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    ++n;

    ID3D11InputLayout* layout = 0;
    if (FAILED(s_mesh.device->CreateInputLayout(elems, n, blob->GetBufferPointer(), blob->GetBufferSize(), &layout)))
    {
        lwD3D11Gap(LW_D3D11_GAP, "mesh-layout", "CreateInputLayout failed fvf=0x%08X skin=%d", (unsigned)fvf, skin);
        return 0;
    }
    s_layouts[key] = layout;
    return layout;
}

static D3D11_BLEND MapBlend(DWORD d)
{
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

static ID3D11BlendState* BlendFor(DWORD src, DWORD dest)
{
    DWORD key = (src << 8) | (dest & 0xff);
    std::map<DWORD, ID3D11BlendState*>::iterator it = s_blends.find(key);
    if (it != s_blends.end())
        return it->second;

    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = MapBlend(src);
    bd.RenderTarget[0].DestBlend = MapBlend(dest);
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ID3D11BlendState* bs = 0;
    if (FAILED(s_mesh.device->CreateBlendState(&bd, &bs)) || !bs)
        return s_mesh.blend_alpha;
    s_blends[key] = bs;
    return bs;
}

struct MeshOmResolved
{
    ID3D11RasterizerState* rast;
    ID3D11DepthStencilState* depth;
    ID3D11BlendState* blend;
};

// RenderStateMgr pass tags select default OM; VFX still overrides via D3DRS cache.
static void ResolveMeshOutputMerger(lwDeviceObject11* dev, const FvfInfo& info, MeshOmResolved* om)
{
    DWORD cull = dev->GetCachedRS(D3DRS_CULLMODE);
    DWORD msaa_aa = dev->GetCachedRS(D3DRS_MULTISAMPLEANTIALIAS);
    const int no_aa = (msaa_aa == 0);
    ID3D11RasterizerState* rast = no_aa ? s_mesh.rast_ccw_noaa : s_mesh.rast_ccw;
    if (s_mesh.outline)
        rast = s_mesh.rast_cw;
    else if (cull == D3DCULL_NONE)
        rast = no_aa ? s_mesh.rast_none_noaa : s_mesh.rast_none;
    else if (cull == D3DCULL_CW)
        rast = no_aa ? s_mesh.rast_cw_noaa : s_mesh.rast_cw;

    DWORD srcblend = dev->GetCachedRS(D3DRS_SRCBLEND);
    DWORD destblend = dev->GetCachedRS(D3DRS_DESTBLEND);
    if (srcblend == 0xffffffff || srcblend == 0)
        srcblend = D3DBLEND_SRCALPHA;
    if (destblend == 0xffffffff || destblend == 0)
        destblend = D3DBLEND_INVSRCALPHA;
    const int additive = (destblend == D3DBLEND_ONE || destblend == D3DBLEND_INVSRCCOLOR ||
        destblend == D3DBLEND_SRCCOLOR);

    DWORD alpha = dev->GetCachedRS(D3DRS_ALPHABLENDENABLE);
    if (alpha == 0xffffffff)
        alpha = 0;

    DWORD zenable = dev->GetCachedRS(D3DRS_ZENABLE);
    DWORD zwrite = dev->GetCachedRS(D3DRS_ZWRITEENABLE);

    const int pass = s_mesh.vfx ? 5
        : (s_mesh.transp_object ? 3
        : (s_mesh.character ? 1 : (s_mesh.scene_object ? 2 : (s_mesh.terrain ? 4 : 0))));

    ID3D11DepthStencilState* depth = s_mesh.depth_on;
    ID3D11BlendState* blend = s_mesh.blend_opaque;

    if (pass != 0)
    {
        depth = (pass == 3 || pass == 5) ? s_mesh.depth_read : s_mesh.depth_on;
        blend = s_mesh.blend_alpha;
        if (zenable == 0)
            depth = s_mesh.depth_off;
        else if (s_mesh.outline || zwrite == 0 || additive)
            depth = s_mesh.depth_read;
        if (!alpha && !s_mesh.outline)
            blend = s_mesh.blend_opaque;
        else if (additive || s_mesh.outline ||
            srcblend != D3DBLEND_SRCALPHA || destblend != D3DBLEND_INVSRCALPHA)
            blend = BlendFor(srcblend, destblend);
    }
    else
    {
        if (zenable == 0)
            depth = s_mesh.depth_off;
        else if (s_mesh.outline || zwrite == 0 || additive)
            depth = s_mesh.depth_read;
        else if (alpha && !info.has_nrm && zwrite != TRUE)
            depth = s_mesh.depth_read;

        if (alpha || s_mesh.outline)
            blend = BlendFor(srcblend, destblend);
    }

    om->rast = rast;
    om->depth = depth;
    om->blend = blend;
}

static UINT PrimIndexCount(D3DPRIMITIVETYPE pt, UINT prim_count)
{
    switch (pt)
    {
    case D3DPT_TRIANGLELIST: return prim_count * 3;
    case D3DPT_TRIANGLESTRIP: return prim_count + 2;
    case D3DPT_LINELIST: return prim_count * 2;
    case D3DPT_LINESTRIP: return prim_count + 1;
    case D3DPT_POINTLIST: return prim_count;
    default: return prim_count * 3;
    }
}

static D3D11_PRIMITIVE_TOPOLOGY Topology(D3DPRIMITIVETYPE pt)
{
    switch (pt)
    {
    case D3DPT_TRIANGLELIST: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case D3DPT_TRIANGLESTRIP: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case D3DPT_LINELIST: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    case D3DPT_LINESTRIP: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case D3DPT_POINTLIST: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    default: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

static int EnsureFanIB(UINT prims)
{
    if (s_mesh.fan_ib && s_mesh.fan_ib_prims >= prims)
        return 1;
    if (!s_mesh.device || prims == 0)
        return 0;
    UINT cap = prims < 64 ? 64 : prims;
    if (cap > 4096)
        cap = 4096;
    if (prims > cap)
        return 0;
    std::vector<USHORT> idx(cap * 3);
    for (UINT i = 0; i < cap; ++i)
    {
        idx[i * 3 + 0] = 0;
        idx[i * 3 + 1] = (USHORT)(i + 1);
        idx[i * 3 + 2] = (USHORT)(i + 2);
    }
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = (UINT)(idx.size() * sizeof(USHORT));
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA srd = {};
    srd.pSysMem = &idx[0];
    ID3D11Buffer* nb = 0;
    if (FAILED(s_mesh.device->CreateBuffer(&bd, &srd, &nb)) || !nb)
        return 0;
    if (s_mesh.fan_ib)
        s_mesh.fan_ib->Release();
    s_mesh.fan_ib = nb;
    s_mesh.fan_ib_prims = cap;
    return 1;
}

static void ArgbToFloat(DWORD c, float* out)
{
    out[0] = ((c >> 16) & 0xff) / 255.0f;
    out[1] = ((c >> 8) & 0xff) / 255.0f;
    out[2] = (c & 0xff) / 255.0f;
    out[3] = ((c >> 24) & 0xff) / 255.0f;
}

struct MeshOmResolved;

static void ResolveMeshOutputMerger(lwDeviceObject11* dev, const FvfInfo& info, MeshOmResolved* om);

static LW_RESULT DrawCommon(lwDeviceObject11* dev, D3DPRIMITIVETYPE pt, int indexed, INT base_vert, UINT start, UINT prim_count)
{
    if (!s_mesh.ready || !dev || prim_count == 0)
        return LW_RET_OK;

    lwD3D11VertexBuffer* vb = lwAsD3D11VertexBuffer(dev->GetBoundVB());
    if (!vb)
        return LW_RET_OK;

    DWORD fvf = dev->GetBoundFVF();
    if (fvf == 0)
        fvf = vb->GetFVF();
    FvfInfo info = ParseFvf(fvf);
    UINT stride = dev->GetBoundVBStride();
    if (stride == 0)
        stride = info.stride;
    if (stride == 0)
        return LW_RET_OK;

    ID3D11Buffer* fan_ib = 0;
    if (pt == D3DPT_TRIANGLEFAN)
    {
        if (!EnsureFanIB(prim_count))
            return LW_RET_OK;
        fan_ib = s_mesh.fan_ib;
        if (!indexed)
            base_vert = (INT)start;
        start = 0;
        indexed = 1;
        pt = D3DPT_TRIANGLELIST;
    }

    int skin = info.has_blend ? 1 : 0;
    ID3D11InputLayout* layout = LayoutFor(fvf, info, skin);
    if (!layout && skin)
    {
        skin = 0;
        layout = LayoutFor(fvf, info, 0);
    }
    if (!layout)
        return LW_RET_OK;

    int use_sm4 = 0;
    {
        DWORD cop1_early = dev->GetCachedTSS(1, D3DTSS_COLOROP);
        DWORD cop2_early = dev->GetCachedTSS(2, D3DTSS_COLOROP);
        lwD3D11Texture* tex1_early = lwAsD3D11Texture(dev->GetBoundTex(1));
        lwD3D11Texture* tex2_early = lwAsD3D11Texture(dev->GetBoundTex(2));
        const int dual_early = ((cop1_early && cop1_early != D3DTOP_DISABLE &&
            cop1_early != 0xffffffff && cop1_early != D3DTSS_FORCE_DWORD &&
            tex1_early && tex1_early->GetSRV()) ||
            (cop2_early && cop2_early != D3DTOP_DISABLE &&
            cop2_early != 0xffffffff && cop2_early != D3DTSS_FORCE_DWORD &&
            tex2_early && tex2_early->GetSRV())) ? 1 : 0;
        const int rhw = ((fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW) ? 1 : 0;
        const int eff_xyzb1 = ((fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZB1) &&
            !(fvf & (D3DFVF_LASTBETA_UBYTE4 | D3DFVF_LASTBETA_D3DCOLOR));
        // FVF-only effect/shade/particle verts have no normal and no LASTBETA.
        // ShaderMgr11 skin VS + leftover character decl must not consume them.
        const int fvf_only_fx = (!info.has_nrm && !info.has_blend) || eff_xyzb1;
        // Rigid items / lit weapon overlays must use the FF mesh path (uvMat).
        // Leftover character VS after physique draw must not hijack them.
        if (!dual_early && !rhw && !fvf_only_fx && info.has_blend)
        {
            ID3D11InputLayout* sm_layout = 0;
            if (lwD3D11ShaderMgrPrepareDraw(dev, &sm_layout) && sm_layout)
            {
                layout = sm_layout;
                use_sm4 = 1;
            }
        }
    }

    ID3D11Buffer* vbb = vb->GetBuffer();
    UINT offset = dev->GetBoundVBOffset();
    s_mesh.context->IASetVertexBuffers(0, 1, &vbb, &stride, &offset);
    s_mesh.context->IASetInputLayout(layout);
    s_mesh.context->IASetPrimitiveTopology(Topology(pt));

    if (fan_ib)
        s_mesh.context->IASetIndexBuffer(fan_ib, DXGI_FORMAT_R16_UINT, 0);
    else if (indexed)
    {
        lwD3D11IndexBuffer* ib = lwAsD3D11IndexBuffer(dev->GetBoundIB());
        if (!ib)
            return LW_RET_OK;
        DXGI_FORMAT fmt = (ib->GetFormat() == D3DFMT_INDEX32) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        s_mesh.context->IASetIndexBuffer(ib->GetBuffer(), fmt, 0);
    }

    MeshCB0 cb = {};
    if (!use_sm4)
    {
    CopyMat(cb.world, dev->GetMatWorld());
    CopyMat(cb.viewProj, dev->GetMatViewProj());

    DWORD lighting = dev->GetCachedRS(D3DRS_LIGHTING);
    if (lighting == 0xffffffff)
        lighting = 1;

    float rs_amb[4] = { 1, 1, 1, 1 };
    DWORD amb_rs = dev->GetCachedRS(D3DRS_AMBIENT);
    if (amb_rs && amb_rs != 0xffffffff && amb_rs != D3DRS_FORCE_DWORD)
        ArgbToFloat(amb_rs, rs_amb);

    const D3DLIGHTX* lgt = dev->GetBoundLight(0);
    const int use_lgt = (lighting != 0) && dev->GetBoundLightEnable(0) && lgt && lgt->Type == D3DLIGHT_DIRECTIONAL;
    if (use_lgt)
    {
        cb.lightDir[0] = lgt->Direction.x;
        cb.lightDir[1] = lgt->Direction.y;
        cb.lightDir[2] = lgt->Direction.z;
        cb.lightDir[3] = 0;
        cb.ambient[0] = lgt->Ambient.r + rs_amb[0];
        cb.ambient[1] = lgt->Ambient.g + rs_amb[1];
        cb.ambient[2] = lgt->Ambient.b + rs_amb[2];
        cb.ambient[3] = 1.0f;
        cb.diffuse[0] = lgt->Diffuse.r;
        cb.diffuse[1] = lgt->Diffuse.g;
        cb.diffuse[2] = lgt->Diffuse.b;
        cb.diffuse[3] = lgt->Diffuse.a;
    }
    else
    {
        cb.ambient[0] = rs_amb[0];
        cb.ambient[1] = rs_amb[1];
        cb.ambient[2] = rs_amb[2];
        cb.ambient[3] = 1.0f;
        cb.lightDir[2] = -1.0f;
        cb.diffuse[0] = cb.diffuse[1] = cb.diffuse[2] = 0.0f;
        cb.diffuse[3] = 1.0f;
    }

    const lwMaterial* mtl = dev->GetBoundMaterial();
    if (mtl)
    {
        cb.ambient[0] *= mtl->amb.r;
        cb.ambient[1] *= mtl->amb.g;
        cb.ambient[2] *= mtl->amb.b;
        cb.diffuse[0] *= mtl->dif.r;
        cb.diffuse[1] *= mtl->dif.g;
        cb.diffuse[2] *= mtl->dif.b;
        if (mtl->amb.a > 0.0f)
            cb.ambient[3] *= mtl->amb.a;
    }

    if (cb.ambient[0] > 1.0f) cb.ambient[0] = 1.0f;
    if (cb.ambient[1] > 1.0f) cb.ambient[1] = 1.0f;
    if (cb.ambient[2] > 1.0f) cb.ambient[2] = 1.0f;

    cb.flags[0] = (skin && s_mesh.bone_count > 0) ? 1.0f : 0.0f;
    cb.flags[1] = info.has_nrm ? 1.0f : 0.0f;
    cb.flags[2] = info.has_uv ? 1.0f : 0.0f;
    cb.flags[3] = (float)info.blend_floats;
    cb.extra[0] = (float)s_mesh.bone_count;
    cb.extra[1] = (s_mesh.outline && info.has_nrm) ? 1.0f : 0.0f;
    cb.extra[2] = s_mesh.outline_width;
    cb.extra[3] = -1.0f;
    cb.outlineColor[0] = s_mesh.outline_color[0];
    cb.outlineColor[1] = s_mesh.outline_color[1];
    cb.outlineColor[2] = s_mesh.outline_color[2];
    cb.outlineColor[3] = s_mesh.outline_color[3];
    cb.more[0] = info.has_diff ? 1.0f : 0.0f;
    cb.more[1] = (lighting == 0 || !info.has_nrm) ? 1.0f : 0.0f;
    if (s_mesh.character || info.has_blend || s_mesh.transp_object || s_mesh.vfx)
        cb.more[1] = 1.0f;
    DWORD cop0 = dev->GetCachedTSS(0, D3DTSS_COLOROP);
    DWORD ca1 = dev->GetCachedTSS(0, D3DTSS_COLORARG1);
    DWORD carg2 = dev->GetCachedTSS(0, D3DTSS_COLORARG2);
    DWORD aarg2 = dev->GetCachedTSS(0, D3DTSS_ALPHAARG2);
    const int color_tf = (carg2 == D3DTA_TFACTOR);
    const int alpha_tf = (aarg2 == D3DTA_TFACTOR);
    cb.more[3] = (float)(color_tf + alpha_tf * 2);
    if (cop0 == D3DTOP_SELECTARG1 && ca1 == D3DTA_TFACTOR)
    {
        cb.more[1] = 2.0f;
        DWORD aa1 = dev->GetCachedTSS(0, D3DTSS_ALPHAARG1);
        cb.more[3] = (aa1 == D3DTA_TEXTURE) ? 0.0f : 2.0f;
    }
    DWORD tf = dev->GetCachedRS(D3DRS_TEXTUREFACTOR);
    if (tf == 0xffffffff || tf == D3DRS_FORCE_DWORD)
        tf = 0xffffffff;
    ArgbToFloat(tf, cb.tfactor);
    DWORD cop1 = dev->GetCachedTSS(1, D3DTSS_COLOROP);
    DWORD s1_ca1 = dev->GetCachedTSS(1, D3DTSS_COLORARG1);
    DWORD s1_ca2 = dev->GetCachedTSS(1, D3DTSS_COLORARG2);
    lwD3D11Texture* tex1_check = lwAsD3D11Texture(dev->GetBoundTex(1));
    lwD3D11Texture* tex2_check = lwAsD3D11Texture(dev->GetBoundTex(2));
    float dual = 0.0f;
    if (cop1 && cop1 != D3DTOP_DISABLE && cop1 != 0xffffffff && cop1 != D3DTSS_FORCE_DWORD && tex1_check && tex1_check->GetSRV())
    {
        if (cop1 == D3DTOP_SELECTARG1 && s1_ca1 == D3DTA_TEXTURE)
            dual = 1.0f;
        else if (cop1 == D3DTOP_SELECTARG2 && s1_ca2 == D3DTA_TEXTURE)
            dual = 1.0f;
        else if (cop1 == D3DTOP_SELECTARG1 && s1_ca1 == D3DTA_CURRENT)
        {
            DWORD cop2 = dev->GetCachedTSS(2, D3DTSS_COLOROP);
            if (cop2 == D3DTOP_MODULATEALPHA_ADDCOLOR && tex2_check && tex2_check->GetSRV())
                dual = 7.0f;
        }
        else if (cop1 == D3DTOP_MODULATEALPHA_ADDCOLOR)
            dual = 3.0f;
        else if (cop1 == D3DTOP_ADD || cop1 == D3DTOP_ADDSMOOTH)
            dual = 4.0f;
        else if (cop1 == D3DTOP_ADDSIGNED)
            dual = 5.0f;
        else if (cop1 == D3DTOP_MODULATE2X || cop1 == D3DTOP_ADDSIGNED2X)
            dual = (cop1 == D3DTOP_ADDSIGNED2X) ? 5.0f : 6.0f;
        else if (cop1 == D3DTOP_MODULATE)
        {
            // Terrain splats: stage0 = alpha atlas, stage1 = tile, ARG2 = DIFFUSE.
            // RGB must come from tex1; mask RGB is near-black and must not modulate.
            // Character dual-tex uses CURRENT and still wants tex0 * tex1.
            const int uses_current = (s1_ca1 == D3DTA_CURRENT || s1_ca2 == D3DTA_CURRENT);
            dual = uses_current ? 2.0f : 1.0f;
        }
        else if (cop1 != D3DTOP_SELECTARG2)
            dual = 2.0f;
    }
    cb.more[2] = dual;
    {
        lwMatrix44 id;
        lwMatrix44Identity(&id);
        const lwMatrix44* uv = dev->GetMatTex(0);
        DWORD ttff = dev->GetCachedTSS(0, D3DTSS_TEXTURETRANSFORMFLAGS);
        CopyMat(cb.uvMat, (uv && TexUvTransformOn(ttff)) ? uv : &id);
        const lwMatrix44* uv1m = dev->GetMatTex(1);
        DWORD ttff1 = dev->GetCachedTSS(1, D3DTSS_TEXTURETRANSFORMFLAGS);
        CopyMat(cb.uvMat1, (uv1m && TexUvTransformOn(ttff1)) ? uv1m : &id);
    }

    if (s_mesh.outline)
    {
        float sx = 0.0f, sy = 0.0f;
        lwGetOutlineScreenScale(dev, &sx, &sy);
        cb.extra[2] = sx;
        cb.more[3] = sy;
    }

    DWORD atest = dev->GetCachedRS(D3DRS_ALPHATESTENABLE);
    if (atest && atest != 0xffffffff && atest != D3DRS_FORCE_DWORD)
    {
        DWORD aref = dev->GetCachedRS(D3DRS_ALPHAREF);
        if (aref == 0xffffffff)
            aref = 0;
        DWORD afunc = dev->GetCachedRS(D3DRS_ALPHAFUNC);
        const float ref = (float)(aref & 0xff) / 255.0f;
        if (afunc == D3DCMP_NOTEQUAL)
            cb.extra[3] = ref + 0.5f / 255.0f;
        else if (afunc == D3DCMP_GREATER || afunc == D3DCMP_GREATEREQUAL || afunc == 0xffffffff)
            cb.extra[3] = ref;
    }

    EyeFromView(dev->GetMatView(), cb.look);
    cb.look[3] = (s_mesh.stylized && !s_mesh.character && !info.has_blend && !s_mesh.terrain && !s_mesh.vfx) ? 1.0f : 0.0f;
    cb.hemiSky[0] = cb.ambient[0] * 0.12f + 0.02f;
    cb.hemiSky[1] = cb.ambient[1] * 0.12f + 0.03f;
    cb.hemiSky[2] = cb.ambient[2] * 0.12f + 0.05f;
    cb.hemiSky[3] = (info.ntex >= 2) ? 1.0f : 0.0f;
    cb.hemiGnd[0] = cb.ambient[0] * 0.08f + 0.03f;
    cb.hemiGnd[1] = cb.ambient[1] * 0.08f + 0.02f;
    cb.hemiGnd[2] = cb.ambient[2] * 0.08f + 0.01f;
    cb.hemiGnd[3] = 1.0f;
    cb.fog[0] = s_mesh.fog_color[0];
    cb.fog[1] = s_mesh.fog_color[1];
    cb.fog[2] = s_mesh.fog_color[2];
    cb.fog[3] = s_mesh.fog_density;
    cb.fogMore[0] = s_mesh.fog_on ? 1.0f : 0.0f;
    cb.fogMore[1] = s_mesh.height_fog ? 1.0f : 0.0f;
    cb.fogMore[2] = s_mesh.sea ? 1.0f : 0.0f;
    cb.fogMore[3] = s_mesh.character ? 1.0f : 0.0f;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(s_mesh.context->Map(s_mesh.cb0, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, &cb, sizeof(cb));
        s_mesh.context->Unmap(s_mesh.cb0, 0);
    }
    if (SUCCEEDED(s_mesh.context->Map(s_mesh.cb1, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, s_mesh.bones, sizeof(s_mesh.bones));
        s_mesh.context->Unmap(s_mesh.cb1, 0);
    }
    }

    ID3D11ShaderResourceView* srv = s_mesh.white_srv;
    lwD3D11Texture* tex = lwAsD3D11Texture(dev->GetBoundTex(0));
    if (tex && tex->GetSRV())
        srv = tex->GetSRV();
    else if (!info.has_nrm && !info.has_blend)
        return LW_RET_OK;

    MeshOmResolved om = {};
    ResolveMeshOutputMerger(dev, info, &om);

    if (info.has_blend)
    {
        const lwMatrix44* w = dev->GetMatWorld();
        lwD3D11Gap(LW_D3D11_INVENTORY, use_sm4 ? "sm4-skin-draw" : "mesh-skin-draw",
            "fvf=0x%08X bones=%u skin=%d sm4=%d world=(%.2f,%.2f,%.2f)",
            (unsigned)fvf, s_mesh.bone_count, (skin && s_mesh.bone_count > 0) ? 1 : 0, use_sm4,
            w ? w->_41 : 0.f, w ? w->_42 : 0.f, w ? w->_43 : 0.f);
    }

    if (!use_sm4)
    {
        s_mesh.context->VSSetShader(skin ? s_mesh.vs_skin : s_mesh.vs_rigid, 0, 0);
        s_mesh.context->PSSetShader(s_mesh.ps, 0, 0);
        s_mesh.context->VSSetConstantBuffers(0, 1, &s_mesh.cb0);
        s_mesh.context->VSSetConstantBuffers(1, 1, &s_mesh.cb1);
        s_mesh.context->PSSetConstantBuffers(0, 1, &s_mesh.cb0);
    }
    else
    {
        static int sm4_logged = 0;
        if (!sm4_logged)
        {
            sm4_logged = 1;
            lwD3D11Gap(LW_D3D11_INVENTORY, "sm4-draw", "ShaderMgr11 VS bound fvf=0x%08X", (unsigned)fvf);
        }
    }
    s_mesh.context->PSSetShaderResources(0, 1, &srv);
    ID3D11ShaderResourceView* srv1 = s_mesh.white_srv;
    lwD3D11Texture* tex1 = lwAsD3D11Texture(dev->GetBoundTex(1));
    if (tex1 && tex1->GetSRV())
        srv1 = tex1->GetSRV();
    s_mesh.context->PSSetShaderResources(1, 1, &srv1);
    ID3D11ShaderResourceView* srv2 = s_mesh.white_srv;
    lwD3D11Texture* tex2 = lwAsD3D11Texture(dev->GetBoundTex(2));
    if (tex2 && tex2->GetSRV())
        srv2 = tex2->GetSRV();
    s_mesh.context->PSSetShaderResources(2, 1, &srv2);
    ID3D11SamplerState* samp = s_mesh.samp;
    DWORD addr = dev->GetCachedSS(0, D3DSAMP_ADDRESSU);
    DWORD mag = dev->GetCachedSS(0, D3DSAMP_MAGFILTER);
    const int point = (mag == D3DTEXF_POINT);
    const int clamp = (addr == D3DTADDRESS_CLAMP);
    if (point && clamp && s_mesh.samp_point_clamp)
        samp = s_mesh.samp_point_clamp;
    else if (point && s_mesh.samp_point)
        samp = s_mesh.samp_point;
    else if (clamp && s_mesh.samp_clamp)
        samp = s_mesh.samp_clamp;
    s_mesh.context->PSSetSamplers(0, 1, &samp);
    s_mesh.context->PSSetSamplers(1, 1, &samp);
    s_mesh.context->PSSetSamplers(2, 1, &samp);
    s_mesh.context->RSSetState(om.rast);
    s_mesh.context->OMSetDepthStencilState(om.depth, 0);
    float bf[4] = { 0, 0, 0, 0 };
    s_mesh.context->OMSetBlendState(om.blend, bf, 0xffffffff);

    UINT idx_count = PrimIndexCount(pt, prim_count);
    if (indexed)
        s_mesh.context->DrawIndexed(idx_count, start, base_vert);
    else
        s_mesh.context->Draw(idx_count, start);

    return LW_RET_OK;
}

LW_RESULT lwD3D11MeshDrawPrimitive(lwDeviceObject11* dev, D3DPRIMITIVETYPE pt_type, UINT start_vertex, UINT prim_count)
{
    return DrawCommon(dev, pt_type, 0, 0, start_vertex, prim_count);
}

LW_RESULT lwD3D11MeshDrawIndexed(lwDeviceObject11* dev, D3DPRIMITIVETYPE pt_type, INT base_vert_index, UINT start_index, UINT prim_count)
{
    return DrawCommon(dev, pt_type, 1, base_vert_index, start_index, prim_count);
}

LW_END
