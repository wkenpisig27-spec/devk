#include "stdafx.h"
#include "lwD3D11Blit.h"
#include "lwD3D11Texture.h"
#include "lwD3D11Gaps.h"
#include "lwDeviceObject11.h"

#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

LW_BEGIN

struct BlitVert
{
    float x, y, z, rhw;
    DWORD color;
    float u, v;
};

static const char* kBlitHLSL =
    "cbuffer CB : register(b0) { float2 invSize; float2 pad; };\n"
    "Texture2D tex0 : register(t0);\n"
    "SamplerState samp0 : register(s0);\n"
    "struct VSIn { float4 pos : POSITION; float4 col : COLOR; float2 uv : TEXCOORD; };\n"
    "struct PSIn { float4 pos : SV_POSITION; float4 col : COLOR; float2 uv : TEXCOORD; };\n"
    "PSIn VSMain(VSIn i) {\n"
    "  PSIn o;\n"
    "  o.pos = float4(i.pos.x * invSize.x * 2.0f - 1.0f, 1.0f - i.pos.y * invSize.y * 2.0f, 0.0f, 1.0f);\n"
    "  o.col = i.col;\n"
    "  o.uv = i.uv;\n"
    "  return o;\n"
    "}\n"
    "float4 PSMain(PSIn i) : SV_TARGET { float4 c = tex0.Sample(samp0, i.uv) * i.col; clip(c.a - 0.004f); return c; }\n";

struct lwD3D11BlitState
{
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11VertexShader* vs;
    ID3D11PixelShader* ps;
    ID3D11InputLayout* layout;
    ID3D11Buffer* vb;
    ID3D11Buffer* cb;
    ID3D11BlendState* blend;
    ID3D11DepthStencilState* depth;
    ID3D11RasterizerState* raster;
    ID3D11SamplerState* samp_linear;
    ID3D11SamplerState* samp_point;
    ID3D11Texture2D* white_tex;
    ID3D11ShaderResourceView* white_srv;
    UINT vb_bytes;
    UINT width;
    UINT height;
    int ready;
};

static lwD3D11BlitState s_blit = {};

static void ReleaseBlit()
{
    if (s_blit.white_srv) s_blit.white_srv->Release();
    if (s_blit.white_tex) s_blit.white_tex->Release();
    if (s_blit.samp_point) s_blit.samp_point->Release();
    if (s_blit.samp_linear) s_blit.samp_linear->Release();
    if (s_blit.raster) s_blit.raster->Release();
    if (s_blit.depth) s_blit.depth->Release();
    if (s_blit.blend) s_blit.blend->Release();
    if (s_blit.cb) s_blit.cb->Release();
    if (s_blit.vb) s_blit.vb->Release();
    if (s_blit.layout) s_blit.layout->Release();
    if (s_blit.ps) s_blit.ps->Release();
    if (s_blit.vs) s_blit.vs->Release();
    memset(&s_blit, 0, sizeof(s_blit));
}

static ID3DBlob* Compile(const char* entry, const char* target)
{
    ID3DBlob* blob = 0;
    ID3DBlob* err = 0;
    HRESULT hr = D3DCompile(kBlitHLSL, strlen(kBlitHLSL), "lwD3D11Blit", 0, 0, entry, target, D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, &blob, &err);
    if (FAILED(hr))
    {
        if (err)
        {
            lwD3D11Gap(LW_D3D11_GAP, "blit-hlsl", "%s", (const char*)err->GetBufferPointer());
            err->Release();
        }
        return 0;
    }
    if (err)
        err->Release();
    return blob;
}

LW_RESULT lwD3D11BlitInit(ID3D11Device* device, ID3D11DeviceContext* context, UINT width, UINT height)
{
    lwD3D11BlitShutdown();
    if (!device || !context)
        return LW_RET_FAILED;

    s_blit.device = device;
    s_blit.context = context;
    s_blit.width = width ? width : 1;
    s_blit.height = height ? height : 1;

    ID3DBlob* vsb = Compile("VSMain", "vs_4_0");
    ID3DBlob* psb = Compile("PSMain", "ps_4_0");
    if (!vsb || !psb)
    {
        if (vsb) vsb->Release();
        if (psb) psb->Release();
        ReleaseBlit();
        return LW_RET_FAILED;
    }

    HRESULT hr = device->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), 0, &s_blit.vs);
    if (SUCCEEDED(hr))
        hr = device->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), 0, &s_blit.ps);

    D3D11_INPUT_ELEMENT_DESC elems[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    if (SUCCEEDED(hr))
        hr = device->CreateInputLayout(elems, 3, vsb->GetBufferPointer(), vsb->GetBufferSize(), &s_blit.layout);
    vsb->Release();
    psb->Release();
    if (FAILED(hr))
    {
        ReleaseBlit();
        return LW_RET_FAILED;
    }

    s_blit.vb_bytes = sizeof(BlitVert) * 4096;
    D3D11_BUFFER_DESC vbd = {};
    vbd.ByteWidth = s_blit.vb_bytes;
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&vbd, 0, &s_blit.vb)))
    {
        ReleaseBlit();
        return LW_RET_FAILED;
    }

    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth = 16;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&cbd, 0, &s_blit.cb)))
    {
        ReleaseBlit();
        return LW_RET_FAILED;
    }

    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&bd, &s_blit.blend);

    D3D11_DEPTH_STENCIL_DESC dd = {};
    dd.DepthEnable = FALSE;
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dd.DepthFunc = D3D11_COMPARISON_ALWAYS;
    device->CreateDepthStencilState(&dd, &s_blit.depth);

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    device->CreateRasterizerState(&rd, &s_blit.raster);

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    device->CreateSamplerState(&sd, &s_blit.samp_linear);
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    device->CreateSamplerState(&sd, &s_blit.samp_point);

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = 1;
    td.Height = 1;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    const DWORD white = 0xFFFFFFFFu;
    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = &white;
    init.SysMemPitch = 4;
    if (SUCCEEDED(device->CreateTexture2D(&td, &init, &s_blit.white_tex)))
        device->CreateShaderResourceView(s_blit.white_tex, 0, &s_blit.white_srv);

    s_blit.ready = 1;
    return LW_RET_OK;
}

void lwD3D11BlitShutdown()
{
    ReleaseBlit();
}

void lwD3D11BlitSetViewport(UINT width, UINT height)
{
    if (width)
        s_blit.width = width;
    if (height)
        s_blit.height = height;
}

static LW_RESULT DrawVerts(const BlitVert* verts, UINT count, ID3D11ShaderResourceView* srv, int point_filter)
{
    if (!s_blit.ready || !verts || count < 3)
        return LW_RET_FAILED;

    UINT bytes = count * sizeof(BlitVert);
    if (bytes > s_blit.vb_bytes)
        count = s_blit.vb_bytes / sizeof(BlitVert);
    bytes = count * sizeof(BlitVert);

    D3D11_MAPPED_SUBRESOURCE map = {};
    if (FAILED(s_blit.context->Map(s_blit.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
        return LW_RET_FAILED;
    memcpy(map.pData, verts, bytes);
    s_blit.context->Unmap(s_blit.vb, 0);

    if (FAILED(s_blit.context->Map(s_blit.cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
        return LW_RET_FAILED;
    float* cb = (float*)map.pData;
    cb[0] = 1.0f / (float)s_blit.width;
    cb[1] = 1.0f / (float)s_blit.height;
    cb[2] = 0.0f;
    cb[3] = 0.0f;
    s_blit.context->Unmap(s_blit.cb, 0);

    UINT stride = sizeof(BlitVert);
    UINT offset = 0;
    s_blit.context->IASetInputLayout(s_blit.layout);
    s_blit.context->IASetVertexBuffers(0, 1, &s_blit.vb, &stride, &offset);
    s_blit.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    s_blit.context->VSSetShader(s_blit.vs, 0, 0);
    s_blit.context->PSSetShader(s_blit.ps, 0, 0);
    s_blit.context->VSSetConstantBuffers(0, 1, &s_blit.cb);
    s_blit.context->OMSetBlendState(s_blit.blend, 0, 0xFFFFFFFF);
    s_blit.context->OMSetDepthStencilState(s_blit.depth, 0);
    s_blit.context->RSSetState(s_blit.raster);
    ID3D11ShaderResourceView* use = srv ? srv : s_blit.white_srv;
    s_blit.context->PSSetShaderResources(0, 1, &use);
    ID3D11SamplerState* samp = point_filter ? s_blit.samp_point : s_blit.samp_linear;
    s_blit.context->PSSetSamplers(0, 1, &samp);
    s_blit.context->Draw(count, 0);
    return LW_RET_OK;
}

static void MakeQuad(BlitVert* v, float x, float y, float w, float h, float u1, float v1, float u2, float v2, DWORD color)
{
    const BlitVert q[6] = {
        { x,     y,     0, 1, color, u1, v1 },
        { x + w, y,     0, 1, color, u2, v1 },
        { x,     y + h, 0, 1, color, u1, v2 },
        { x + w, y,     0, 1, color, u2, v1 },
        { x + w, y + h, 0, 1, color, u2, v2 },
        { x,     y + h, 0, 1, color, u1, v2 },
    };
    memcpy(v, q, sizeof(q));
}

LW_RESULT lwD3D11BlitSprite(
    IDirect3DTextureX* tex,
    const RECT* src,
    const D3DXVECTOR2* scale,
    const D3DXVECTOR2* dest,
    DWORD color)
{
    lwD3D11Texture* t11 = lwAsD3D11Texture(tex);
    if (!t11)
        return LW_RET_FAILED;

    UINT tw = t11->GetWidth();
    UINT th = t11->GetHeight();
    if (tw == 0 || th == 0)
        return LW_RET_FAILED;

    RECT rc;
    if (src)
        rc = *src;
    else
    {
        rc.left = 0;
        rc.top = 0;
        rc.right = (LONG)tw;
        rc.bottom = (LONG)th;
    }
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return LW_RET_OK;

    float sx = scale ? scale->x : 1.0f;
    float sy = scale ? scale->y : 1.0f;
    float dx = dest ? dest->x : 0.0f;
    float dy = dest ? dest->y : 0.0f;
    float w = (float)(rc.right - rc.left) * sx;
    float h = (float)(rc.bottom - rc.top) * sy;
    float u1 = (float)rc.left / (float)tw;
    float v1 = (float)rc.top / (float)th;
    float u2 = (float)rc.right / (float)tw;
    float v2 = (float)rc.bottom / (float)th;

    BlitVert verts[6];
    MakeQuad(verts, dx, dy, w, h, u1, v1, u2, v2, color ? color : 0xFFFFFFFF);
    return DrawVerts(verts, 6, t11->GetSRV(), 0);
}

LW_RESULT lwD3D11BlitDrawUP(
    const void* verts,
    UINT stride,
    UINT vert_count,
    IDirect3DTextureX* tex,
    int point_filter)
{
    if (!verts || stride != sizeof(BlitVert) || vert_count < 3)
        return LW_RET_FAILED;
    lwD3D11Texture* t11 = lwAsD3D11Texture(tex);
    return DrawVerts((const BlitVert*)verts, vert_count, t11 ? t11->GetSRV() : 0, point_filter);
}

LW_END
