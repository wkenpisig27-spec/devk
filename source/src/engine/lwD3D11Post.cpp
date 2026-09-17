#include "stdafx.h"
#include "lwD3D11Post.h"
#include "lwD3D11Gaps.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <string.h>

#pragma comment(lib, "d3dcompiler.lib")

LW_BEGIN

static const char* kPostHLSL =
    "cbuffer CB : register(b0) {\n"
    "  float2 texel;\n"
    "  float2 dir;\n"
    "  float4 pack;\n"
    "  float4 look;\n"
    "};\n"
    "Texture2D tex0 : register(t0);\n"
    "Texture2D tex1 : register(t1);\n"
    "SamplerState samp : register(s0);\n"
    "struct PSIn { float4 pos : SV_POSITION; float2 uv : TEXCOORD; };\n"
    "PSIn VSMain(uint id : SV_VertexID) {\n"
    "  PSIn o;\n"
    "  o.uv = float2((id << 1) & 2, id & 2);\n"
    "  o.pos = float4(o.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);\n"
    "  return o;\n"
    "}\n"
    "float4 PSBright(PSIn i) : SV_TARGET {\n"
    "  float3 c = tex0.Sample(samp, i.uv).rgb;\n"
    "  float l = dot(c, float3(0.2126f, 0.7152f, 0.0722f));\n"
    "  float t = pack.y;\n"
    "  float k = saturate((l - t) / max(1e-3f, t + 0.35f));\n"
    "  return float4(c * k, 1.0f);\n"
    "}\n"
    "float4 PSBlur(PSIn i) : SV_TARGET {\n"
    "  float2 stepv = dir * texel;\n"
    "  float3 a = tex0.Sample(samp, i.uv).rgb * 0.227027f;\n"
    "  a += tex0.Sample(samp, i.uv + stepv).rgb * 0.1945946f;\n"
    "  a += tex0.Sample(samp, i.uv - stepv).rgb * 0.1945946f;\n"
    "  a += tex0.Sample(samp, i.uv + stepv * 2.0f).rgb * 0.1216216f;\n"
    "  a += tex0.Sample(samp, i.uv - stepv * 2.0f).rgb * 0.1216216f;\n"
    "  a += tex0.Sample(samp, i.uv + stepv * 3.0f).rgb * 0.054054f;\n"
    "  a += tex0.Sample(samp, i.uv - stepv * 3.0f).rgb * 0.054054f;\n"
    "  a += tex0.Sample(samp, i.uv + stepv * 4.0f).rgb * 0.016216f;\n"
    "  a += tex0.Sample(samp, i.uv - stepv * 4.0f).rgb * 0.016216f;\n"
    "  return float4(a, 1.0f);\n"
    "}\n"
    "float4 PSTonemap(PSIn i) : SV_TARGET {\n"
    "  float3 hdr = tex0.Sample(samp, i.uv).rgb;\n"
    "  float3 bloom = tex1.Sample(samp, i.uv).rgb;\n"
    "  float3 c = (hdr + bloom * pack.z) * pack.x;\n"
    "  float3 over = max(c - 1.0f, 0.0f);\n"
    "  c = min(c, 1.0f) + over / (1.0f + over);\n"
    "  float3 lwgt = float3(0.2126f, 0.7152f, 0.0722f);\n"
    "  float l = dot(c, lwgt);\n"
    "  float3 chroma = c - l;\n"
    "  float lFlat = lerp(0.50f, l, look.x);\n"
    "  lFlat += look.w * saturate(0.45f - lFlat);\n"
    "  float haze = look.z;\n"
    "  lFlat = saturate((lFlat - haze) / max(1e-3f, 1.0f - haze));\n"
    "  float span = max(c.r, max(c.g, c.b)) - min(c.r, min(c.g, c.b));\n"
    "  float chromaGain = look.y * (1.0f + 0.18f * (1.0f - saturate(span * 1.7f)));\n"
    "  c = saturate(lFlat + chroma * chromaGain);\n"
    "  return float4(c, 1.0f);\n"
    "}\n"
    "float4 PSSharpen(PSIn i) : SV_TARGET {\n"
    "  float3 c = tex0.Sample(samp, i.uv).rgb;\n"
    "  float3 n = tex0.Sample(samp, i.uv + float2(0.0f, -texel.y)).rgb;\n"
    "  float3 s = tex0.Sample(samp, i.uv + float2(0.0f,  texel.y)).rgb;\n"
    "  float3 e = tex0.Sample(samp, i.uv + float2( texel.x, 0.0f)).rgb;\n"
    "  float3 w = tex0.Sample(samp, i.uv + float2(-texel.x, 0.0f)).rgb;\n"
    "  float3 blur = (n + s + e + w) * 0.25f;\n"
    "  return float4(saturate(c + (c - blur) * pack.w), 1.0f);\n"
    "}\n";

struct PostCB
{
    float texel_x, texel_y;
    float dir_x, dir_y;
    float exposure, thresh, intensity, sharpen;
    float contrast, saturation, dehaze, fill;
};

struct PostParams
{
    int hdr;
    int bloom;
    int sharpen;
    float exposure;
    float bloom_th;
    float bloom_int;
    float sharpen_str;
    float contrast;
    float saturation;
    float dehaze;
    float fill;
};

struct PostState
{
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11VertexShader* vs;
    ID3D11PixelShader* ps_bright;
    ID3D11PixelShader* ps_blur;
    ID3D11PixelShader* ps_tonemap;
    ID3D11PixelShader* ps_sharpen;
    ID3D11Buffer* cb;
    ID3D11BlendState* blend_off;
    ID3D11DepthStencilState* depth_off;
    ID3D11RasterizerState* rast;
    ID3D11SamplerState* samp_linear;
    ID3D11SamplerState* samp_point;

    ID3D11Texture2D* hdr_msaa;
    ID3D11RenderTargetView* hdr_msaa_rtv;
    ID3D11Texture2D* hdr_depth;
    ID3D11DepthStencilView* hdr_dsv;
    ID3D11Texture2D* hdr_resolved;
    ID3D11ShaderResourceView* hdr_srv;
    ID3D11RenderTargetView* hdr_resolved_rtv;

    ID3D11Texture2D* bloom0;
    ID3D11Texture2D* bloom1;
    ID3D11RenderTargetView* bloom0_rtv;
    ID3D11RenderTargetView* bloom1_rtv;
    ID3D11ShaderResourceView* bloom0_srv;
    ID3D11ShaderResourceView* bloom1_srv;

    ID3D11Texture2D* ldr_copy;
    ID3D11ShaderResourceView* ldr_copy_srv;
    ID3D11Texture2D* black_tex;
    ID3D11ShaderResourceView* black_srv;

    UINT w;
    UINT h;
    UINT bloom_w;
    UINT bloom_h;
    UINT msaa;
    int ready;
    int targets;
};

static PostParams s_params = { 0, 0, 0, 0.97f, 1.00f, 0.28f, 0.10f, 0.96f, 1.12f, 0.045f, 0.015f };
static PostState s_post = {};

template <typename T>
static void Rel(T*& p)
{
    if (p)
    {
        p->Release();
        p = 0;
    }
}

static void UnbindPS()
{
    if (!s_post.context)
        return;
    ID3D11ShaderResourceView* none[4] = {};
    s_post.context->PSSetShaderResources(0, 4, none);
    s_post.context->OMSetRenderTargets(0, 0, 0);
}

static ID3DBlob* Compile(const char* entry, const char* target)
{
    ID3DBlob* blob = 0;
    ID3DBlob* err = 0;
    HRESULT hr = D3DCompile(kPostHLSL, strlen(kPostHLSL), "lwD3D11Post", 0, 0,
        entry, target, D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, &blob, &err);
    if (FAILED(hr))
    {
        if (err)
        {
            lwD3D11Gap(LW_D3D11_GAP, "post-hlsl", "%s", (const char*)err->GetBufferPointer());
            err->Release();
        }
        return 0;
    }
    if (err)
        err->Release();
    return blob;
}

static float Clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void lwD3D11PostSetParams(
    int hdr,
    int bloom,
    int sharpen,
    float exposure,
    float bloom_threshold,
    float bloom_intensity,
    float sharpen_strength,
    float contrast,
    float saturation,
    float dehaze,
    float fill)
{
    s_params.hdr = hdr ? 1 : 0;
    s_params.bloom = bloom ? 1 : 0;
    s_params.sharpen = sharpen ? 1 : 0;
    s_params.exposure = Clampf(exposure, 0.20f, 2.00f);
    s_params.bloom_th = Clampf(bloom_threshold, 0.10f, 2.00f);
    s_params.bloom_int = Clampf(bloom_intensity, 0.0f, 2.00f);
    s_params.sharpen_str = Clampf(sharpen_strength, 0.0f, 1.50f);
    s_params.contrast = Clampf(contrast, 0.70f, 1.60f);
    s_params.saturation = Clampf(saturation, 0.40f, 1.80f);
    s_params.dehaze = Clampf(dehaze, 0.0f, 0.12f);
    s_params.fill = Clampf(fill, 0.0f, 0.20f);
}

int lwD3D11PostWantsHdr()
{
    return s_params.hdr;
}

int lwD3D11PostIsActive()
{
    return (s_params.hdr && s_post.ready && s_post.targets) ? 1 : 0;
}

static void ReleaseTargets()
{
    UnbindPS();
    Rel(s_post.hdr_msaa_rtv);
    Rel(s_post.hdr_msaa);
    Rel(s_post.hdr_dsv);
    Rel(s_post.hdr_depth);
    Rel(s_post.hdr_srv);
    Rel(s_post.hdr_resolved_rtv);
    Rel(s_post.hdr_resolved);
    Rel(s_post.bloom0_srv);
    Rel(s_post.bloom0_rtv);
    Rel(s_post.bloom0);
    Rel(s_post.bloom1_srv);
    Rel(s_post.bloom1_rtv);
    Rel(s_post.bloom1);
    Rel(s_post.ldr_copy_srv);
    Rel(s_post.ldr_copy);
    s_post.w = 0;
    s_post.h = 0;
    s_post.bloom_w = 0;
    s_post.bloom_h = 0;
    s_post.msaa = 1;
    s_post.targets = 0;
}

void lwD3D11PostReleaseTargets()
{
    ReleaseTargets();
}

static void ReleaseAll()
{
    ReleaseTargets();
    Rel(s_post.black_srv);
    Rel(s_post.black_tex);
    Rel(s_post.samp_point);
    Rel(s_post.samp_linear);
    Rel(s_post.rast);
    Rel(s_post.depth_off);
    Rel(s_post.blend_off);
    Rel(s_post.cb);
    Rel(s_post.ps_sharpen);
    Rel(s_post.ps_tonemap);
    Rel(s_post.ps_blur);
    Rel(s_post.ps_bright);
    Rel(s_post.vs);
    memset(&s_post, 0, sizeof(s_post));
}

void lwD3D11PostShutdown()
{
    ReleaseAll();
}

static HRESULT MakeTex2D(
    ID3D11Device* device,
    UINT w, UINT h, UINT msaa,
    DXGI_FORMAT fmt,
    UINT bind,
    ID3D11Texture2D** tex)
{
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w;
    td.Height = h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = fmt;
    td.SampleDesc.Count = msaa ? msaa : 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = bind;
    return device->CreateTexture2D(&td, 0, tex);
}

static void SetViewport(UINT w, UINT h)
{
    D3D11_VIEWPORT vp = {};
    vp.Width = (float)(w ? w : 1);
    vp.Height = (float)(h ? h : 1);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    s_post.context->RSSetViewports(1, &vp);
}

static void UpdateCB(float texel_x, float texel_y, float dir_x, float dir_y)
{
    D3D11_MAPPED_SUBRESOURCE map = {};
    if (FAILED(s_post.context->Map(s_post.cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
        return;
    PostCB cb;
    cb.texel_x = texel_x;
    cb.texel_y = texel_y;
    cb.dir_x = dir_x;
    cb.dir_y = dir_y;
    cb.exposure = s_params.exposure;
    cb.thresh = s_params.bloom_th;
    cb.intensity = s_params.bloom_int;
    cb.sharpen = s_params.sharpen_str;
    cb.contrast = s_params.contrast;
    cb.saturation = s_params.saturation;
    cb.dehaze = s_params.dehaze;
    cb.fill = s_params.fill;
    memcpy(map.pData, &cb, sizeof(cb));
    s_post.context->Unmap(s_post.cb, 0);
}

static void DrawPS(
    ID3D11PixelShader* ps,
    ID3D11RenderTargetView* rtv,
    ID3D11ShaderResourceView* t0,
    ID3D11ShaderResourceView* t1,
    ID3D11SamplerState* samp,
    UINT vp_w, UINT vp_h,
    float texel_x, float texel_y,
    float dir_x, float dir_y)
{
    UnbindPS();
    UpdateCB(texel_x, texel_y, dir_x, dir_y);
    s_post.context->OMSetRenderTargets(1, &rtv, 0);
    SetViewport(vp_w, vp_h);
    s_post.context->IASetInputLayout(0);
    s_post.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = 0, offset = 0;
    ID3D11Buffer* none = 0;
    s_post.context->IASetVertexBuffers(0, 1, &none, &stride, &offset);
    s_post.context->VSSetShader(s_post.vs, 0, 0);
    s_post.context->PSSetShader(ps, 0, 0);
    s_post.context->PSSetConstantBuffers(0, 1, &s_post.cb);
    ID3D11ShaderResourceView* srvs[2] = { t0, t1 };
    s_post.context->PSSetShaderResources(0, 2, srvs);
    s_post.context->PSSetSamplers(0, 1, &samp);
    const float blend[4] = { 0, 0, 0, 0 };
    s_post.context->OMSetBlendState(s_post.blend_off, blend, 0xffffffff);
    s_post.context->OMSetDepthStencilState(s_post.depth_off, 0);
    s_post.context->RSSetState(s_post.rast);
    s_post.context->Draw(3, 0);
}

LW_RESULT lwD3D11PostInit(ID3D11Device* device, ID3D11DeviceContext* context)
{
    lwD3D11PostShutdown();
    if (!device || !context)
        return LW_RET_FAILED;

    s_post.device = device;
    s_post.context = context;

    ID3DBlob* vsb = Compile("VSMain", "vs_4_0");
    ID3DBlob* bright = Compile("PSBright", "ps_4_0");
    ID3DBlob* blur = Compile("PSBlur", "ps_4_0");
    ID3DBlob* tone = Compile("PSTonemap", "ps_4_0");
    ID3DBlob* sharp = Compile("PSSharpen", "ps_4_0");
    if (!vsb || !bright || !blur || !tone || !sharp)
    {
        if (vsb) vsb->Release();
        if (bright) bright->Release();
        if (blur) blur->Release();
        if (tone) tone->Release();
        if (sharp) sharp->Release();
        ReleaseAll();
        return LW_RET_FAILED;
    }

    HRESULT hr = device->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), 0, &s_post.vs);
    if (SUCCEEDED(hr))
        hr = device->CreatePixelShader(bright->GetBufferPointer(), bright->GetBufferSize(), 0, &s_post.ps_bright);
    if (SUCCEEDED(hr))
        hr = device->CreatePixelShader(blur->GetBufferPointer(), blur->GetBufferSize(), 0, &s_post.ps_blur);
    if (SUCCEEDED(hr))
        hr = device->CreatePixelShader(tone->GetBufferPointer(), tone->GetBufferSize(), 0, &s_post.ps_tonemap);
    if (SUCCEEDED(hr))
        hr = device->CreatePixelShader(sharp->GetBufferPointer(), sharp->GetBufferSize(), 0, &s_post.ps_sharpen);
    vsb->Release();
    bright->Release();
    blur->Release();
    tone->Release();
    sharp->Release();
    if (FAILED(hr))
    {
        ReleaseAll();
        return LW_RET_FAILED;
    }

    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth = 48;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&cbd, 0, &s_post.cb)))
    {
        ReleaseAll();
        return LW_RET_FAILED;
    }

    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&bd, &s_post.blend_off);

    D3D11_DEPTH_STENCIL_DESC dd = {};
    dd.DepthEnable = FALSE;
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dd.DepthFunc = D3D11_COMPARISON_ALWAYS;
    device->CreateDepthStencilState(&dd, &s_post.depth_off);

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    device->CreateRasterizerState(&rd, &s_post.rast);

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    device->CreateSamplerState(&sd, &s_post.samp_linear);
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    device->CreateSamplerState(&sd, &s_post.samp_point);

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = 1;
    td.Height = 1;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    const DWORD black = 0;
    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = &black;
    init.SysMemPitch = 4;
    if (FAILED(device->CreateTexture2D(&td, &init, &s_post.black_tex)))
    {
        ReleaseAll();
        return LW_RET_FAILED;
    }
    if (FAILED(device->CreateShaderResourceView(s_post.black_tex, 0, &s_post.black_srv)))
    {
        ReleaseAll();
        return LW_RET_FAILED;
    }

    s_post.ready = 1;
    return LW_RET_OK;
}

LW_RESULT lwD3D11PostCreateTargets(UINT width, UINT height, UINT msaa)
{
    ReleaseTargets();
    if (!s_post.ready || !s_post.device || !s_params.hdr)
        return LW_RET_FAILED;

    UINT w = width ? width : 1;
    UINT h = height ? height : 1;
    UINT samples = msaa ? msaa : 1;
    if (samples > 8)
        samples = 8;

    UINT q = 0;
    if (samples > 1)
    {
        if (FAILED(s_post.device->CheckMultisampleQualityLevels(DXGI_FORMAT_R16G16B16A16_FLOAT, samples, &q)) || q == 0)
        {
            lwD3D11Gap(LW_D3D11_FALLBACK, "hdr-msaa",
                "R16G16B16A16_FLOAT %ux MSAA unsupported, using 1x HDR", samples);
            samples = 1;
        }
    }

    const DXGI_FORMAT hdr_fmt = DXGI_FORMAT_R16G16B16A16_FLOAT;
    const DXGI_FORMAT depth_fmt = DXGI_FORMAT_D24_UNORM_S8_UINT;

    if (samples > 1)
    {
        if (FAILED(MakeTex2D(s_post.device, w, h, samples, hdr_fmt, D3D11_BIND_RENDER_TARGET, &s_post.hdr_msaa)))
            goto fail;
        D3D11_RENDER_TARGET_VIEW_DESC rvd = {};
        rvd.Format = hdr_fmt;
        rvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        if (FAILED(s_post.device->CreateRenderTargetView(s_post.hdr_msaa, &rvd, &s_post.hdr_msaa_rtv)))
            goto fail;
        if (FAILED(MakeTex2D(s_post.device, w, h, samples, depth_fmt, D3D11_BIND_DEPTH_STENCIL, &s_post.hdr_depth)))
            goto fail;
        D3D11_DEPTH_STENCIL_VIEW_DESC dvd = {};
        dvd.Format = depth_fmt;
        dvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        if (FAILED(s_post.device->CreateDepthStencilView(s_post.hdr_depth, &dvd, &s_post.hdr_dsv)))
            goto fail;
        if (FAILED(MakeTex2D(s_post.device, w, h, 1, hdr_fmt,
            D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, &s_post.hdr_resolved)))
            goto fail;
        if (FAILED(s_post.device->CreateShaderResourceView(s_post.hdr_resolved, 0, &s_post.hdr_srv)))
            goto fail;
    }
    else
    {
        if (FAILED(MakeTex2D(s_post.device, w, h, 1, hdr_fmt,
            D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, &s_post.hdr_resolved)))
            goto fail;
        if (FAILED(s_post.device->CreateRenderTargetView(s_post.hdr_resolved, 0, &s_post.hdr_resolved_rtv)))
            goto fail;
        if (FAILED(s_post.device->CreateShaderResourceView(s_post.hdr_resolved, 0, &s_post.hdr_srv)))
            goto fail;
        if (FAILED(MakeTex2D(s_post.device, w, h, 1, depth_fmt, D3D11_BIND_DEPTH_STENCIL, &s_post.hdr_depth)))
            goto fail;
        if (FAILED(s_post.device->CreateDepthStencilView(s_post.hdr_depth, 0, &s_post.hdr_dsv)))
            goto fail;
    }

    s_post.bloom_w = w / 4;
    s_post.bloom_h = h / 4;
    if (s_post.bloom_w < 1) s_post.bloom_w = 1;
    if (s_post.bloom_h < 1) s_post.bloom_h = 1;

    if (FAILED(MakeTex2D(s_post.device, s_post.bloom_w, s_post.bloom_h, 1, hdr_fmt,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, &s_post.bloom0)))
        goto fail;
    if (FAILED(s_post.device->CreateRenderTargetView(s_post.bloom0, 0, &s_post.bloom0_rtv)))
        goto fail;
    if (FAILED(s_post.device->CreateShaderResourceView(s_post.bloom0, 0, &s_post.bloom0_srv)))
        goto fail;
    if (FAILED(MakeTex2D(s_post.device, s_post.bloom_w, s_post.bloom_h, 1, hdr_fmt,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, &s_post.bloom1)))
        goto fail;
    if (FAILED(s_post.device->CreateRenderTargetView(s_post.bloom1, 0, &s_post.bloom1_rtv)))
        goto fail;
    if (FAILED(s_post.device->CreateShaderResourceView(s_post.bloom1, 0, &s_post.bloom1_srv)))
        goto fail;

    if (FAILED(MakeTex2D(s_post.device, w, h, 1, DXGI_FORMAT_B8G8R8A8_UNORM,
        D3D11_BIND_SHADER_RESOURCE, &s_post.ldr_copy)))
        goto fail;
    if (FAILED(s_post.device->CreateShaderResourceView(s_post.ldr_copy, 0, &s_post.ldr_copy_srv)))
        goto fail;

    s_post.w = w;
    s_post.h = h;
    s_post.msaa = samples;
    s_post.targets = 1;
    LG("d3d11gaps", "[SysGraphics] post-fx hdr=1 bloom=%d sharpen=%d msaa=%u %ux%u\n",
        s_params.bloom, s_params.sharpen, samples, w, h);
    return LW_RET_OK;

fail:
    lwD3D11Gap(LW_D3D11_FALLBACK, "hdr-targets", "HDR scene RT create failed, post-fx off");
    ReleaseTargets();
    s_params.hdr = 0;
    return LW_RET_FAILED;
}

ID3D11RenderTargetView* lwD3D11PostSceneRTV()
{
    if (!s_post.targets)
        return 0;
    if (s_post.msaa > 1)
        return s_post.hdr_msaa_rtv;
    return s_post.hdr_resolved_rtv;
}

ID3D11DepthStencilView* lwD3D11PostSceneDSV()
{
    return s_post.targets ? s_post.hdr_dsv : 0;
}

LW_RESULT lwD3D11PostResolve(IDXGISwapChain* swapchain, ID3D11RenderTargetView* backbuffer_rtv, UINT width, UINT height)
{
    if (!s_post.targets || !s_post.context || !backbuffer_rtv)
        return LW_RET_FAILED;

    UnbindPS();

    if (s_post.msaa > 1 && s_post.hdr_msaa && s_post.hdr_resolved)
        s_post.context->ResolveSubresource(s_post.hdr_resolved, 0, s_post.hdr_msaa, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);

    ID3D11ShaderResourceView* bloom_srv = s_post.black_srv;
    if (s_params.bloom && s_post.bloom0_rtv)
    {
        DrawPS(s_post.ps_bright, s_post.bloom0_rtv, s_post.hdr_srv, 0, s_post.samp_linear,
            s_post.bloom_w, s_post.bloom_h,
            1.0f / (float)s_post.w, 1.0f / (float)s_post.h, 0.0f, 0.0f);
        DrawPS(s_post.ps_blur, s_post.bloom1_rtv, s_post.bloom0_srv, 0, s_post.samp_linear,
            s_post.bloom_w, s_post.bloom_h,
            1.0f / (float)s_post.bloom_w, 1.0f / (float)s_post.bloom_h, 1.0f, 0.0f);
        DrawPS(s_post.ps_blur, s_post.bloom0_rtv, s_post.bloom1_srv, 0, s_post.samp_linear,
            s_post.bloom_w, s_post.bloom_h,
            1.0f / (float)s_post.bloom_w, 1.0f / (float)s_post.bloom_h, 0.0f, 1.0f);
        bloom_srv = s_post.bloom0_srv;
    }

    DrawPS(s_post.ps_tonemap, backbuffer_rtv, s_post.hdr_srv, bloom_srv, s_post.samp_linear,
        width, height,
        1.0f / (float)s_post.w, 1.0f / (float)s_post.h, 0.0f, 0.0f);

    if (s_params.sharpen && s_post.ldr_copy && swapchain)
    {
        UnbindPS();
        ID3D11Texture2D* back = 0;
        if (SUCCEEDED(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back)) && back)
        {
            s_post.context->CopyResource(s_post.ldr_copy, back);
            back->Release();
            DrawPS(s_post.ps_sharpen, backbuffer_rtv, s_post.ldr_copy_srv, 0, s_post.samp_point,
                width, height,
                1.0f / (float)width, 1.0f / (float)height, 0.0f, 0.0f);
        }
    }

    UnbindPS();
    return LW_RET_OK;
}

LW_END
