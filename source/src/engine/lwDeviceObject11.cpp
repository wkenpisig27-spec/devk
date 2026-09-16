#include "stdafx.h"
#include "lwDeviceObject11.h"
#include "lwSysGraphics.h"
#include "lwD3D11Gaps.h"
#include "lwD3D11Blit.h"
#include "lwD3D11Texture.h"
#include "lwD3D11Buffer.h"
#include "lwD3D11Mesh.h"
#include "lwShaderMgr11.h"
#include "lwRenderBackend.h"
#include "lwGraphicsUtil.h"
#include "lwStreamObj.h"

#include <d3d11.h>
#include <dxgi.h>
#include <string.h>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

LW_BEGIN

static lwDeviceObject11* s_active11 = 0;

lwDeviceObject11* lwGetActiveDeviceObject11()
{
    return s_active11;
}

LW_STD_IMPLEMENTATION(lwDeviceObject11)

template <typename T>
static void D11Release(T*& p)
{
    if (p)
    {
        p->Release();
        p = 0;
    }
}

#define D11_STUB_OK(id, msg) \
    do { lwD3D11Gap(LW_D3D11_SKIP, id, msg); return LW_RET_OK; } while (0)

#define D11_STUB_FAIL(id, msg) \
    do { lwD3D11Gap(LW_D3D11_GAP, id, msg); return LW_RET_FAILED; } while (0)

lwDeviceObject11::lwDeviceObject11(lwSysGraphics* sys_graphics)
    : _sys_graphics(sys_graphics)
    , _mark_polygon_num(0)
    , _device(0)
    , _context(0)
    , _factory(0)
    , _swapchain(0)
    , _depth_tex(0)
    , _rtv(0)
    , _dsv(0)
    , _saved_rtv(0)
    , _saved_dsv(0)
    , _offscreen_push(0)
    , _bShadowPass(0)
    , _bb_width(0)
    , _bb_height(0)
    , _vsync(0)
    , _bound_vb(0)
    , _bound_vb_off(0)
    , _bound_vb_stride(0)
    , _bound_ib(0)
    , _bound_fvf(0)
    , _bound_vs(0)
    , _bound_decl(0)
    , _up_vb(0)
    , _up_vb_bytes(0)
{
    memset(&_d3d_create_param, 0, sizeof(_d3d_create_param));
    memset(&_display_mode, 0, sizeof(_display_mode));
    memset(&_dev_caps, 0, sizeof(_dev_caps));
    memset(&_viewport, 0, sizeof(_viewport));
    memset(&_bbf_caps, 0, sizeof(_bbf_caps));
    memset(&_rc_window, 0, sizeof(_rc_window));
    memset(&_rc_client, 0, sizeof(_rc_client));
    memset(&_watch_vm_info, 0, sizeof(_watch_vm_info));
    memset(&_material, 0, sizeof(_material));
    memset(_light_enable, 0, sizeof(_light_enable));
    memset(_light_seq, 0, sizeof(_light_seq));
    memset(_tex_seq, 0, sizeof(_tex_seq));
    memset(_vs_c, 0, sizeof(_vs_c));
    _vs_c[0] = 1.0f;
    _vs_c[3] = 765.01f;

    lwMatrix44Identity(&_mat_view);
    lwMatrix44Identity(&_mat_proj);
    lwMatrix44Identity(&_mat_viewproj);
    lwMatrix44Identity(&_mat_world);
    for (int i = 0; i < 8; ++i)
        lwMatrix44Identity(&_mat_tex[i]);

    s_active11 = this;
}

lwDeviceObject11::~lwDeviceObject11()
{
    if (s_active11 == this)
        s_active11 = 0;

    if (_up_vb)
    {
        _up_vb->Release();
        _up_vb = 0;
        _up_vb_bytes = 0;
    }
    lwD3D11ShaderMgrShutdown();
    lwD3D11MeshShutdown();
    lwD3D11BlitShutdown();
    _ReleaseTargets();
    D11Release(_swapchain);
    D11Release(_context);
    D11Release(_device);
    D11Release(_factory);
}

void lwDeviceObject11::_ReleaseTargets()
{
    if (_context)
        _context->OMSetRenderTargets(0, 0, 0);
    D11Release(_rtv);
    D11Release(_dsv);
    D11Release(_depth_tex);
}

void lwDeviceObject11::_BindTargets()
{
    if (_context && _rtv)
        _context->OMSetRenderTargets(1, &_rtv, _dsv);
}

LW_RESULT lwDeviceObject11::_CreateTargets()
{
    if (!_device || !_swapchain)
        return LW_RET_FAILED;

    _ReleaseTargets();

    ID3D11Texture2D* back = 0;
    HRESULT hr = _swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back);
    if (FAILED(hr) || !back)
        return LW_RET_FAILED;

    hr = _device->CreateRenderTargetView(back, 0, &_rtv);
    D3D11_TEXTURE2D_DESC bb = {};
    back->GetDesc(&bb);
    back->Release();
    if (FAILED(hr))
        return LW_RET_FAILED;

    _bb_width = bb.Width;
    _bb_height = bb.Height;

    D3D11_TEXTURE2D_DESC depth = {};
    depth.Width = _bb_width;
    depth.Height = _bb_height;
    depth.MipLevels = 1;
    depth.ArraySize = 1;
    depth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth.SampleDesc.Count = 1;
    depth.Usage = D3D11_USAGE_DEFAULT;
    depth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = _device->CreateTexture2D(&depth, 0, &_depth_tex);
    if (FAILED(hr))
        return LW_RET_FAILED;

    hr = _device->CreateDepthStencilView(_depth_tex, 0, &_dsv);
    if (FAILED(hr))
        return LW_RET_FAILED;

    _viewport.X = 0;
    _viewport.Y = 0;
    _viewport.Width = _bb_width;
    _viewport.Height = _bb_height;
    _viewport.MinZ = 0.0f;
    _viewport.MaxZ = 1.0f;

    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = (float)_bb_width;
    vp.Height = (float)_bb_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &vp);

    _BindTargets();
    lwD3D11BlitSetViewport(_bb_width, _bb_height);
    return LW_RET_OK;
}

void lwDeviceObject11::PushOffscreenTargets(ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv)
{
    if (!rtv)
        return;
    if (_offscreen_push == 0)
    {
        _saved_rtv = _rtv;
        _saved_dsv = _dsv;
    }
    _offscreen_push++;
    _rtv = rtv;
    _dsv = dsv;
    _BindTargets();
}

void lwDeviceObject11::PopOffscreenTargets()
{
    if (_offscreen_push <= 0)
        return;
    _offscreen_push--;
    if (_offscreen_push == 0)
    {
        _rtv = _saved_rtv;
        _dsv = _saved_dsv;
        _saved_rtv = 0;
        _saved_dsv = 0;
        _BindTargets();
    }
}

LW_RESULT lwDeviceObject11::CreateDirect3D()
{
    if (_factory)
        return LW_RET_OK;

    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&_factory);
    if (FAILED(hr) || !_factory)
    {
        LG("d3d11gaps", "[GAP] dxgi-factory — CreateDXGIFactory failed hr=0x%08X\n", (unsigned)hr);
        return LW_RET_FAILED;
    }
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::CreateDevice(lwD3DCreateParam* param)
{
    if (!param)
        return LW_RET_FAILED;
    if (!_factory && LW_FAILED(CreateDirect3D()))
        return LW_RET_FAILED;

    _d3d_create_param = *param;
    if (!_d3d_create_param.hwnd)
        _d3d_create_param.hwnd = param->present_param.hDeviceWindow;
    if (!_d3d_create_param.present_param.hDeviceWindow)
        _d3d_create_param.present_param.hDeviceWindow = _d3d_create_param.hwnd;
    _vsync = (param->present_param.PresentationInterval != D3DPRESENT_INTERVAL_IMMEDIATE) ? 1 : 0;

    UINT w = param->present_param.BackBufferWidth;
    UINT h = param->present_param.BackBufferHeight;
    if (w == 0 || h == 0)
    {
        RECT rc = {};
        GetClientRect(param->hwnd, &rc);
        w = (UINT)(rc.right - rc.left);
        h = (UINT)(rc.bottom - rc.top);
        if (w == 0) w = 1280;
        if (h == 0) h = 720;
        _d3d_create_param.present_param.BackBufferWidth = w;
        _d3d_create_param.present_param.BackBufferHeight = h;
    }

    if (param->present_param.MultiSampleType > D3DMULTISAMPLE_NONE)
    {
        lwD3D11Gap(LW_D3D11_SKIP, "msaa-deferred",
            "Slice 1 swapchain is 1x; MSAA comes after draw parity");
    }

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL got = D3D_FEATURE_LEVEL_10_0;

    HRESULT hr = D3D11CreateDevice(
        0,
        D3D_DRIVER_TYPE_HARDWARE,
        0,
        0,
        levels,
        (UINT)(sizeof(levels) / sizeof(levels[0])),
        D3D11_SDK_VERSION,
        &_device,
        &got,
        &_context);
    if (FAILED(hr) || !_device || !_context)
    {
        LG("d3d11gaps", "[GAP] d3d11-createdevice — D3D11CreateDevice failed hr=0x%08X\n", (unsigned)hr);
        return LW_RET_FAILED;
    }

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = w;
    sd.BufferDesc.Height = h;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = param->hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = 0;
    if (!param->present_param.Windowed)
    {
        lwD3D11Gap(LW_D3D11_FALLBACK, "borderless-fs",
            "DXGI swapchain stays windowed; fullscreen is HWND WS_POPUP + ResizeBuffers");
    }

    hr = _factory->CreateSwapChain(_device, &sd, &_swapchain);
    if (FAILED(hr) || !_swapchain)
    {
        LG("d3d11gaps", "[GAP] dxgi-swapchain — CreateSwapChain failed hr=0x%08X\n", (unsigned)hr);
        return LW_RET_FAILED;
    }

    _factory->MakeWindowAssociation(param->hwnd, DXGI_MWA_NO_ALT_ENTER);

    if (LW_FAILED(_CreateTargets()))
        return LW_RET_FAILED;

    _display_mode.Width = w;
    _display_mode.Height = h;
    _display_mode.RefreshRate = 60;
    _display_mode.Format = D3DFMT_A8R8G8B8;

    UpdateWindowRect();
    _FillSyntheticCaps();

    if (LW_FAILED(lwD3D11BlitInit(_device, _context, _bb_width, _bb_height)))
    {
        LG("d3d11gaps", "[GAP] blit-init — screen-space UI shaders failed to compile\n");
        return LW_RET_FAILED;
    }

    if (LW_FAILED(lwD3D11MeshInit(_device, _context)))
    {
        LG("d3d11gaps", "[GAP] mesh-init — 3D mesh shaders failed to compile\n");
        return LW_RET_FAILED;
    }

    lwD3D11ShaderMgrInit(_device, _context);

    LG("d3d11gaps", "[SysGraphics] DeviceObject11 up: feature=0x%X %ux%u windowed=%d vsync=%d\n",
        (unsigned)got, w, h, (int)param->present_param.Windowed, _vsync);
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::ResetDevice(D3DPRESENT_PARAMETERS* d3dpp)
{
    if (!_swapchain || !_device)
        return LW_RET_FAILED;

    UINT w = d3dpp ? d3dpp->BackBufferWidth : _bb_width;
    UINT h = d3dpp ? d3dpp->BackBufferHeight : _bb_height;
    if (w == 0) w = 1;
    if (h == 0) h = 1;

    if (d3dpp)
        _d3d_create_param.present_param = *d3dpp;

    if (_context)
    {
        ID3D11ShaderResourceView* none[8] = {};
        _context->PSSetShaderResources(0, 8, none);
        _context->VSSetShaderResources(0, 8, none);
        _context->OMSetRenderTargets(0, 0, 0);
    }

    _ReleaseTargets();

    DXGI_SWAP_CHAIN_DESC sd = {};
    if (SUCCEEDED(_swapchain->GetDesc(&sd)) && !sd.Windowed)
        _swapchain->SetFullscreenState(FALSE, 0);

    HRESULT hr = _swapchain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        _swapchain->SetFullscreenState(FALSE, 0);
        hr = _swapchain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
    }
    if (FAILED(hr))
    {
        LG("d3d11gaps", "[GAP] resizebuffers — ResizeBuffers failed hr=0x%08X\n", (unsigned)hr);
        return LW_RET_FAILED;
    }

    if (LW_FAILED(_CreateTargets()))
        return LW_RET_FAILED;

    _display_mode.Width = _bb_width;
    _display_mode.Height = _bb_height;
    UpdateWindowRect();
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::ResetDeviceStateCache()
{
    // DXGI ResizeBuffers does not drop D3D9-style RS/TSS. DX9 reset re-applies the
    // existing cache; calling InitStateCache here zeroed D3DRS_AMBIENT and the next
    // terrain VB refill baked black vertex colors.
    DWORD i, j, v;
    for (i = 0; i < LW_MAX_RENDERSTATE_NUM; ++i)
    {
        if ((v = _rs_value[i]) == LW_INVALID_RS_VALUE)
            continue;
        SetRenderStateForced((D3DRENDERSTATETYPE)i, v);
    }
    for (j = 0; j < LW_MAX_TEXTURESTAGE_NUM; ++j)
    {
        for (i = 0; i < LW_MAX_TEXTURESTAGESTATE_NUM; ++i)
        {
            if ((v = _tss_value[j][i]) == LW_INVALID_TSS_VALUE)
                continue;
            SetTextureStageStateForced(j, (D3DTEXTURESTAGESTATETYPE)i, v);
        }
    }
    for (j = 0; j < LW_MAX_SAMPLESTAGE_NUM; ++j)
    {
        for (i = 0; i < LW_MAX_SAMPLESTATE_NUM; ++i)
        {
            if ((v = _ss_value[j][i]) == LW_INVALID_SS_VALUE)
                continue;
            SetSamplerStateForced(j, (D3DSAMPLERSTATETYPE)i, v);
        }
    }
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::ResetDeviceTransformMatrix()
{
    for (DWORD i = 0; i < LW_MAX_LIGHT_NUM; i++)
    {
        LightEnableForced(i, _light_enable[i]);
        if (_light_enable[i])
            SetLight(i, &_light_seq[i]);
    }

    for (int i = 0; i < 8; ++i)
    {
        lwMatrix44Identity(&_mat_tex[i]);
        SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + i), &_mat_tex[i]);
    }

    SetTransformView(&_mat_view);
    SetTransformProj(&_mat_proj);
    SetTransformWorld(&_mat_world);
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetDirect3D(IDirect3DX*)
{
    lwD3D11Gap(LW_D3D11_SKIP, "set-d3d9", "D3D9 factory is unused on DeviceObject11");
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetDevice(IDirect3DDeviceX*)
{
    lwD3D11Gap(LW_D3D11_SKIP, "set-d3d9-device", "D3D9 device is unused on DeviceObject11");
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetViewPort(const D3DVIEWPORTX* vp)
{
    if (!vp)
        return LW_RET_FAILED;
    _viewport = *vp;
    if (_context)
    {
        D3D11_VIEWPORT dvp = {};
        dvp.TopLeftX = (float)vp->X;
        dvp.TopLeftY = (float)vp->Y;
        dvp.Width = (float)vp->Width;
        dvp.Height = (float)vp->Height;
        dvp.MinDepth = vp->MinZ;
        dvp.MaxDepth = vp->MaxZ;
        _context->RSSetViewports(1, &dvp);
    }
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::GetViewPort(D3DVIEWPORTX* vp)
{
    if (!vp)
        return LW_RET_FAILED;
    *vp = _viewport;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::Clear(DWORD flags, D3DCOLOR color, float z, DWORD stencil)
{
    if (!_context)
        return LW_RET_FAILED;

    if ((flags & D3DCLEAR_TARGET) && _rtv)
    {
        const float c[4] = {
            ((color >> 16) & 0xff) / 255.0f,
            ((color >> 8) & 0xff) / 255.0f,
            (color & 0xff) / 255.0f,
            ((color >> 24) & 0xff) / 255.0f,
        };
        _context->ClearRenderTargetView(_rtv, c);
    }
    if (_dsv && (flags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL)))
    {
        UINT dflags = 0;
        if (flags & D3DCLEAR_ZBUFFER)
            dflags |= D3D11_CLEAR_DEPTH;
        if (flags & D3DCLEAR_STENCIL)
            dflags |= D3D11_CLEAR_STENCIL;
        _context->ClearDepthStencilView(_dsv, dflags, z, (UINT8)stencil);
    }
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::Present()
{
    if (!_swapchain)
        return LW_RET_FAILED;
    HRESULT hr = _swapchain->Present(_vsync ? 1 : 0, 0);
    if (hr == DXGI_STATUS_OCCLUDED)
        return LW_RET_OK;
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        lwD3D11Gap(LW_D3D11_GAP, "present-removed",
            "DXGI device removed/reset hr=0x%08X — restart the client", (unsigned)hr);
        return LW_RET_FAILED;
    }
    if (FAILED(hr))
    {
        LG("d3d11gaps", "[GAP] present — Present failed hr=0x%08X\n", (unsigned)hr);
        return LW_RET_FAILED;
    }
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::BeginScene()
{
    _BindTargets();
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::EndScene()
{
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetTransform(D3DTRANSFORMSTATETYPE state, const lwMatrix44* mat)
{
    if (!mat)
        return LW_RET_FAILED;
    if (state == D3DTS_VIEW)
        return SetTransformView(mat);
    if (state == D3DTS_PROJECTION)
        return SetTransformProj(mat);
    if (state == D3DTS_WORLD)
        return SetTransformWorld(mat);
    if (state >= D3DTS_TEXTURE0 && state <= D3DTS_TEXTURE7)
    {
        _mat_tex[state - D3DTS_TEXTURE0] = *mat;
        return LW_RET_OK;
    }
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetTransformView(const lwMatrix44* mat)
{
    if (!mat)
        return LW_RET_FAILED;
    _mat_view = *mat;
    lwMatrix44Multiply(&_mat_viewproj, &_mat_view, &_mat_proj);
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetTransformProj(const lwMatrix44* mat)
{
    if (!mat)
        return LW_RET_FAILED;
    _mat_proj = *mat;
    lwMatrix44Multiply(&_mat_viewproj, &_mat_view, &_mat_proj);
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetTransformWorld(const lwMatrix44* mat)
{
    if (!mat)
        return LW_RET_FAILED;
    _mat_world = *mat;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetMaterial(lwMaterial* mtl)
{
    if (mtl)
        _material = *mtl;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetTexture(DWORD stage, IDirect3DBaseTextureX* tex)
{
    if (stage < LW_MAX_TEXTURESTAGE_NUM)
        _tex_seq[stage] = tex;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetTextureForced(DWORD stage, IDirect3DTextureX* tex)
{
    return SetTexture(stage, tex);
}

LW_RESULT lwDeviceObject11::SetRenderState(D3DRENDERSTATETYPE state, DWORD value)
{
    if (_bShadowPass)
    {
        if (state == D3DRS_ALPHATESTENABLE || state == D3DRS_ALPHAREF ||
            state == D3DRS_ALPHAFUNC || state == D3DRS_CULLMODE ||
            state == D3DRS_ALPHABLENDENABLE || state == D3DRS_SRCBLEND ||
            state == D3DRS_DESTBLEND || state == D3DRS_TEXTUREFACTOR)
            return LW_RET_OK;
    }
    if ((DWORD)state < LW_MAX_RENDERSTATE_NUM)
        _rs_value[state] = value;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetRenderStateForced(D3DRENDERSTATETYPE state, DWORD value)
{
    if ((DWORD)state < LW_MAX_RENDERSTATE_NUM)
        _rs_value[state] = value;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
    if (_bShadowPass && stage <= 1)
    {
        if (type == D3DTSS_COLOROP || type == D3DTSS_COLORARG1 ||
            type == D3DTSS_ALPHAOP || type == D3DTSS_ALPHAARG1)
            return LW_RET_OK;
    }
    if (stage < LW_MAX_TEXTURESTAGE_NUM && (DWORD)type < LW_MAX_TEXTURESTAGESTATE_NUM)
        _tss_value[stage][type] = value;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetTextureStageStateForced(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
    if (stage < LW_MAX_TEXTURESTAGE_NUM && (DWORD)type < LW_MAX_TEXTURESTAGESTATE_NUM)
        _tss_value[stage][type] = value;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value)
{
    if (sampler < LW_MAX_SAMPLESTAGE_NUM && (DWORD)type < LW_MAX_SAMPLESTATE_NUM)
        _ss_value[sampler][type] = value;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetSamplerStateForced(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value)
{
    return SetSamplerState(sampler, type, value);
}

LW_RESULT lwDeviceObject11::GetRenderState(DWORD state, DWORD* value)
{
    if (!value || state >= LW_MAX_RENDERSTATE_NUM)
        return LW_RET_FAILED;
    *value = _rs_value[state];
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::GetTextureStageState(DWORD stage, DWORD state, DWORD* value)
{
    if (!value || stage >= LW_MAX_TEXTURESTAGE_NUM || state >= LW_MAX_TEXTURESTAGESTATE_NUM)
        return LW_RET_FAILED;
    *value = _tss_value[stage][state];
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::GetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE state, DWORD* value)
{
    if (!value || sampler >= LW_MAX_SAMPLESTAGE_NUM || (DWORD)state >= LW_MAX_SAMPLESTATE_NUM)
        return LW_RET_FAILED;
    *value = _ss_value[sampler][state];
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::GetTexture(DWORD stage, IDirect3DBaseTextureX** tex)
{
    if (!tex || stage >= LW_MAX_TEXTURESTAGE_NUM)
        return LW_RET_FAILED;
    *tex = _tex_seq[stage];
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetLight(DWORD id, const D3DLIGHTX* light)
{
    if (id >= LW_MAX_LIGHT_NUM || !light)
        return LW_RET_FAILED;
    _light_seq[id] = *light;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::GetLight(DWORD id, D3DLIGHTX* light)
{
    if (id >= LW_MAX_LIGHT_NUM || !light)
        return LW_RET_FAILED;
    *light = _light_seq[id];
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::LightEnable(DWORD id, BOOL flag)
{
    if (id >= LW_MAX_LIGHT_NUM)
        return LW_RET_FAILED;
    _light_enable[id] = flag;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::LightEnableForced(DWORD id, BOOL flag)
{
    return LightEnable(id, flag);
}

LW_RESULT lwDeviceObject11::GetLightEnable(DWORD id, BOOL* flag)
{
    if (id >= LW_MAX_LIGHT_NUM || !flag)
        return LW_RET_FAILED;
    *flag = _light_enable[id];
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::UpdateWindowRect()
{
    HWND hwnd = _d3d_create_param.hwnd ? _d3d_create_param.hwnd : _d3d_create_param.present_param.hDeviceWindow;
    if (!hwnd)
        return LW_RET_FAILED;
    ::GetWindowRect(hwnd, &_rc_window);
    ::GetClientRect(hwnd, &_rc_client);
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::GetWindowRect(RECT* rc_wnd, RECT* rc_client)
{
    if (rc_wnd)
        *rc_wnd = _rc_window;
    if (rc_client)
        *rc_client = _rc_client;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::InitStateCache()
{
    int i, j;
    for (i = 0; i < LW_MAX_RENDERSTATE_NUM; ++i)
        _rs_value[i] = LW_INVALID_RS_VALUE;
    for (j = 0; j < LW_MAX_TEXTURESTAGE_NUM; ++j)
    {
        for (i = 0; i < LW_MAX_TEXTURESTAGESTATE_NUM; ++i)
            _tss_value[j][i] = LW_INVALID_TSS_VALUE;
        _tex_seq[j] = 0;
    }
    for (j = 0; j < LW_MAX_SAMPLESTAGE_NUM; ++j)
    {
        for (i = 0; i < LW_MAX_SAMPLESTATE_NUM; ++i)
            _ss_value[j][i] = LW_INVALID_SS_VALUE;
    }

    if (_context)
    {
        ID3D11ShaderResourceView* none[LW_MAX_TEXTURESTAGE_NUM] = {};
        _context->PSSetShaderResources(0, LW_MAX_TEXTURESTAGE_NUM, none);
    }

    for (i = 0; i < 8; ++i)
        lwMatrix44Identity(&_mat_tex[i]);

    SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    SetRenderState(D3DRS_ALPHAREF, 0);
    SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
    SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    SetRenderState(D3DRS_LIGHTING, TRUE);
    SetRenderState(D3DRS_AMBIENT, 0xffffffff);
    SetRenderState(D3DRS_FOGENABLE, FALSE);
    SetRenderState(D3DRS_COLORVERTEX, TRUE);
    SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff);
    SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
    SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
    SetRenderState(D3DRS_COLORWRITEENABLE, 0x0000000F);
    SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);

    for (i = 0; i < LW_MAX_TEXTURESTAGE_NUM; ++i)
    {
        if (i == 0)
        {
            SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
            SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        }
        else
        {
            SetTextureStageState(i, D3DTSS_COLOROP, D3DTOP_DISABLE);
            SetTextureStageState(i, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            SetTextureStageState(i, D3DTSS_COLORARG2, D3DTA_CURRENT);
            SetTextureStageState(i, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            SetTextureStageState(i, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            SetTextureStageState(i, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        }
        SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, i);
        SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    }

    for (i = 0; i < LW_MAX_SAMPLESTAGE_NUM; ++i)
    {
        SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
        SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
        SetSamplerState(i, D3DSAMP_MAXANISOTROPY, 1);
        SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    }
    return LW_RET_OK;
}

void lwDeviceObject11::_FillSyntheticCaps()
{
    memset(&_dev_caps, 0, sizeof(_dev_caps));
    _dev_caps.DeviceType = D3DDEVTYPE_HAL;
    _dev_caps.AdapterOrdinal = 0;
    _dev_caps.Caps = D3DCAPS_READ_SCANLINE;
    _dev_caps.DevCaps = D3DDEVCAPS_HWTRANSFORMANDLIGHT | D3DDEVCAPS_PUREDEVICE | D3DDEVCAPS_TEXTUREVIDEOMEMORY;
    _dev_caps.VertexShaderVersion = D3DVS_VERSION(3, 0);
    _dev_caps.PixelShaderVersion = D3DPS_VERSION(3, 0);
    _dev_caps.MaxVertexShaderConst = 256;
    _dev_caps.MaxTextureWidth = 16384;
    _dev_caps.MaxTextureHeight = 16384;
    _dev_caps.MaxSimultaneousTextures = 8;
    _dev_caps.MaxStreams = 8;
    _dev_caps.MaxStreamStride = 256;
    _dev_caps.MaxUserClipPlanes = 6;
    _dev_caps.MaxPrimitiveCount = 0x00FFFFFE;
    _dev_caps.MaxVertexIndex = 0x00FFFFFF;
    _dev_caps.TextureCaps = D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_MIPMAP | D3DPTEXTURECAPS_CUBEMAP | D3DPTEXTURECAPS_PROJECTED;
    _dev_caps.TextureFilterCaps = D3DPTFILTERCAPS_MINFLINEAR | D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MIPFLINEAR;
    _dev_caps.MaxAnisotropy = 16;
    _dev_caps.PresentationIntervals = D3DPRESENT_INTERVAL_IMMEDIATE | D3DPRESENT_INTERVAL_ONE;

    lwBackBufferFormatsItemInfo* item = &_bbf_caps.fmt_seq[0];
    memset(item, 0, sizeof(*item));
    item->windowed = _d3d_create_param.present_param.Windowed;
    item->format = D3DFMT_A8R8G8B8;

    static const D3DFORMAT kTex[] = {
        D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_R5G6B5, D3DFMT_A4R4G4B4,
        D3DFMT_A1R5G5B5, D3DFMT_X1R5G5B5, D3DFMT_DXT1, D3DFMT_DXT3, D3DFMT_DXT5,
        D3DFMT_L8, D3DFMT_A8L8, D3DFMT_V8U8,
    };
    for (int i = 0; i < (int)(sizeof(kTex) / sizeof(kTex[0])) && i < BBFI_MAX_TEXTURE; ++i)
        item->fmt_texture[i] = kTex[i];

    item->fmt_depthstencil[0] = D3DFMT_D24S8;
    item->fmt_depthstencil[1] = D3DFMT_D24X8;
    item->fmt_depthstencil[2] = D3DFMT_D16;
    item->fmt_rendertarget[0] = D3DFMT_A8R8G8B8;
    item->fmt_rendertarget[1] = D3DFMT_X8R8G8B8;
    item->fmt_cubetexture[0] = D3DFMT_A8R8G8B8;
    item->fmt_volumetexture[0] = D3DFMT_A8R8G8B8;
    item->fmt_multisample[0] = D3DFMT_UNKNOWN;
}

LW_RESULT lwDeviceObject11::InitCapsInfo()
{
    _FillSyntheticCaps();
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::CheckCurrentDeviceFormat(DWORD type, D3DFORMAT check_fmt)
{
    lwBackBufferFormatsItemInfo* item = &_bbf_caps.fmt_seq[0];
    D3DFORMAT* buf = 0;
    int n = 0;
    switch (type)
    {
    case BBFI_TEXTURE:        buf = item->fmt_texture; n = BBFI_MAX_TEXTURE; break;
    case BBFI_VOLUMETEXTURE:  buf = item->fmt_volumetexture; n = BBFI_MAX_VOLUMETEXTURE; break;
    case BBFI_CUBETEXTURE:    buf = item->fmt_cubetexture; n = BBFI_MAX_CUBETEXTURE; break;
    case BBFI_DEPTHSTENCIL:   buf = item->fmt_depthstencil; n = BBFI_MAX_DEPTHSTENCIL; break;
    case BBFI_MULTISAMPLE:    buf = item->fmt_multisample; n = BBFI_MAX_MULTISAMPLE; break;
    default:
        return ERR_INVALID_PARAM;
    }
    for (int i = 0; i < n; ++i)
    {
        if (buf[i] == 0)
            break;
        if (buf[i] == check_fmt)
            return LW_RET_OK;
    }
    return LW_RET_FAILED;
}

LW_RESULT lwDeviceObject11::ScreenToWorld(lwVector3* org, lwVector3* ray, int x, int y)
{
    lwScreenToWorld(org, ray, x, y, _viewport.Width, _viewport.Height, &_mat_proj, &_mat_view);
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::WorldToScreen(int* x, int* y, float* z, const lwVector3* v)
{
    lwWorldToScreen(x, y, z, v, _viewport.Width, _viewport.Height, &_mat_proj, &_mat_view);
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::DumpRenderState(const char* file)
{
    FILE* fp = fopen(file, "wt");
    if (fp)
    {
        fprintf(fp, "DeviceObject11 render-state cache (CPU only)\n");
        fclose(fp);
    }
    return LW_RET_OK;
}

void lwDeviceObject11::BeginBenchMark()
{
    _mark_polygon_num = 0;
}

void lwDeviceObject11::EndBenchMark()
{
}

LW_RESULT lwDeviceObject11::SetFVF(DWORD fvf)
{
    _bound_fvf = fvf;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetVertexShader(IDirect3DVertexShaderX* shader)
{
    _bound_vs = shader;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetVertexShaderForced(IDirect3DVertexShaderX* shader)
{
    return SetVertexShader(shader);
}

LW_RESULT lwDeviceObject11::SetVertexDeclaration(IDirect3DVertexDeclarationX* decl)
{
    _bound_decl = decl;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetVertexDeclarationForced(IDirect3DVertexDeclarationX* decl)
{
    return SetVertexDeclaration(decl);
}

LW_RESULT lwDeviceObject11::SetVertexShaderConstantF(UINT reg_id, const float* data, UINT v_num)
{
    if (!data || v_num == 0)
        return LW_RET_OK;
    if (reg_id >= 256)
        return LW_RET_OK;
    if (reg_id + v_num > 256)
        v_num = 256 - reg_id;
    memcpy(_vs_c + reg_id * 4, data, v_num * 16);
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetStreamSource(UINT, IDirect3DVertexBufferX* stream_data, UINT offset_byte, UINT stride)
{
    _bound_vb = stream_data;
    _bound_vb_off = offset_byte;
    _bound_vb_stride = stride;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::SetIndices(IDirect3DIndexBufferX* index_data, UINT)
{
    _bound_ib = index_data;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::DrawPrimitive(D3DPRIMITIVETYPE pt_type, UINT start_vertex, UINT count)
{
    return lwD3D11MeshDrawPrimitive(this, pt_type, start_vertex, count);
}

LW_RESULT lwDeviceObject11::DrawIndexedPrimitive(D3DPRIMITIVETYPE pt_type, INT base_vert_index, UINT, UINT, UINT start_index, UINT count)
{
    return lwD3D11MeshDrawIndexed(this, pt_type, base_vert_index, start_index, count);
}

LW_RESULT lwDeviceObject11::DrawPrimitiveUP(D3DPRIMITIVETYPE pt_type, UINT count, const void* data, UINT stride)
{
    if (!data || stride == 0 || count == 0)
        return LW_RET_OK;

    UINT verts = 0;
    switch (pt_type)
    {
    case D3DPT_POINTLIST: verts = count; break;
    case D3DPT_LINELIST: verts = count * 2; break;
    case D3DPT_LINESTRIP: verts = count + 1; break;
    case D3DPT_TRIANGLELIST: verts = count * 3; break;
    case D3DPT_TRIANGLESTRIP:
    case D3DPT_TRIANGLEFAN: verts = count + 2; break;
    default: return LW_RET_FAILED;
    }

    const DWORD fvf = _bound_fvf;
    const int rhw = ((fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW) ? 1 : 0;
    const int tex1 = ((fvf & D3DFVF_TEXCOUNT_MASK) == D3DFVF_TEX1) ? 1 : 0;
    if (rhw && tex1 && stride == 28 &&
        (pt_type == D3DPT_TRIANGLELIST || pt_type == D3DPT_TRIANGLEFAN || pt_type == D3DPT_TRIANGLESTRIP))
    {
        const BYTE* src = (const BYTE*)data;
        std::vector<BYTE> list;
        const void* draw_data = data;
        UINT draw_verts = verts;
        if (pt_type == D3DPT_TRIANGLEFAN)
        {
            draw_verts = count * 3;
            list.resize(draw_verts * stride);
            for (UINT i = 0; i < count; ++i)
            {
                memcpy(&list[(i * 3 + 0) * stride], src, stride);
                memcpy(&list[(i * 3 + 1) * stride], src + (i + 1) * stride, stride);
                memcpy(&list[(i * 3 + 2) * stride], src + (i + 2) * stride, stride);
            }
            draw_data = &list[0];
        }
        else if (pt_type == D3DPT_TRIANGLESTRIP)
        {
            draw_verts = count * 3;
            list.resize(draw_verts * stride);
            for (UINT i = 0; i < count; ++i)
            {
                const UINT a = (i & 1) ? (i + 1) : i;
                const UINT b = (i & 1) ? i : (i + 1);
                memcpy(&list[(i * 3 + 0) * stride], src + a * stride, stride);
                memcpy(&list[(i * 3 + 1) * stride], src + b * stride, stride);
                memcpy(&list[(i * 3 + 2) * stride], src + (i + 2) * stride, stride);
            }
            draw_data = &list[0];
        }
        IDirect3DTextureX* tex = (IDirect3DTextureX*)GetBoundTex(0);
        return lwD3D11BlitDrawUP(draw_data, stride, draw_verts, tex, 0);
    }

    const UINT bytes = verts * stride;
    if (!_up_vb || _up_vb_bytes < bytes)
    {
        if (_up_vb)
        {
            _up_vb->Release();
            _up_vb = 0;
            _up_vb_bytes = 0;
        }
        UINT alloc = bytes < 65536u ? 65536u : bytes;
        if (LW_FAILED(lwD3D11CreateVertexBuffer(_device, _context, alloc, fvf, &_up_vb)) || !_up_vb)
            return LW_RET_FAILED;
        _up_vb_bytes = alloc;
    }

    void* locked = 0;
    if (FAILED(_up_vb->Lock(0, bytes, &locked, D3DLOCK_DISCARD)) || !locked)
        return LW_RET_FAILED;
    memcpy(locked, data, bytes);
    _up_vb->Unlock();

    IDirect3DVertexBufferX* prev_vb = _bound_vb;
    UINT prev_off = _bound_vb_off;
    UINT prev_stride = _bound_vb_stride;
    SetStreamSource(0, _up_vb, 0, stride);
    LW_RESULT r = DrawPrimitive(pt_type, 0, count);
    SetStreamSource(0, prev_vb, prev_off, prev_stride);
    return r;
}

LW_RESULT lwDeviceObject11::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE, UINT, UINT, UINT, const void*, D3DFORMAT, const void*, UINT)
{
    D11_STUB_OK("draw-indexed-up", "DrawIndexedPrimitiveUP waits on a dynamic VB");
}

LW_RESULT lwDeviceObject11::CreateVertexBuffer(UINT length, DWORD, DWORD fvf, D3DPOOL, IDirect3DVertexBufferX** vb, HANDLE*)
{
    return lwD3D11CreateVertexBuffer(_device, _context, length, fvf, vb);
}

LW_RESULT lwDeviceObject11::CreateIndexBuffer(UINT length, DWORD, D3DFORMAT fmt, D3DPOOL, IDirect3DIndexBufferX** ib, HANDLE*)
{
    return lwD3D11CreateIndexBuffer(_device, _context, length, fmt, ib);
}

LW_RESULT lwDeviceObject11::CreateTexture(IDirect3DTextureX** out_tex, const lwTexDataInfo* info, DWORD, DWORD, DWORD format, D3DPOOL)
{
    if (out_tex) *out_tex = 0;
    if (!info || !_device)
        return LW_RET_FAILED;
    return lwD3D11CreateEmptyTexture(_device, info->width, info->height, (D3DFORMAT)format, out_tex);
}

LW_RESULT lwDeviceObject11::CreateTexture(IDirect3DTextureX** out_tex, UINT width, UINT height, UINT, DWORD, D3DFORMAT format, D3DPOOL)
{
    if (out_tex) *out_tex = 0;
    if (!_device)
        return LW_RET_FAILED;
    return lwD3D11CreateEmptyTexture(_device, width, height, format, out_tex);
}

LW_RESULT lwDeviceObject11::CreateTextureFromFileInMemory(IDirect3DTextureX** out_tex, void* data, UINT data_size, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, DWORD, DWORD, D3DCOLOR colorkey, D3DXIMAGE_INFO* src_info, PALETTEENTRY*)
{
    if (out_tex) *out_tex = 0;
    if (!_device)
        return LW_RET_FAILED;
    return lwD3D11CreateTextureFromMemory(_device, data, data_size, colorkey, src_info, out_tex);
}

LW_RESULT lwDeviceObject11::CreateVertexBuffer(lwIVertexBuffer** out_obj)
{
    if (out_obj)
        *out_obj = 0;
    lwVertexBuffer* obj = LW_NEW(lwVertexBuffer(this));
    if (!obj)
        return LW_RET_FAILED;
    *out_obj = obj;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::CreateIndexBuffer(lwIIndexBuffer** out_obj)
{
    if (out_obj)
        *out_obj = 0;
    lwIndexBuffer* obj = LW_NEW(lwIndexBuffer(this));
    if (!obj)
        return LW_RET_FAILED;
    *out_obj = obj;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::CreateRenderTarget(IDirect3DSurfaceX** o_surface, UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL, HANDLE*)
{
    if (o_surface) *o_surface = 0;
    D11_STUB_FAIL("create-rt", "offscreen RTs are later slices");
}

LW_RESULT lwDeviceObject11::CreateDepthStencilSurface(IDirect3DSurfaceX** o_surface, UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL, HANDLE*)
{
    if (o_surface) *o_surface = 0;
    D11_STUB_FAIL("create-ds", "extra depth surfaces are later slices");
}

LW_RESULT lwDeviceObject11::CreateCubeTexture(IDirect3DCubeTextureX** o_tex, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, HANDLE*)
{
    if (o_tex) *o_tex = 0;
    D11_STUB_FAIL("create-cube", "cube textures are later slices");
}

LW_RESULT lwDeviceObject11::CreateOffscreenPlainSurface(IDirect3DSurfaceX** surface, UINT, UINT, D3DFORMAT, DWORD, HANDLE*)
{
    if (surface) *surface = 0;
    D11_STUB_FAIL("create-offscreen", "staging surfaces are later slices");
}

LW_RESULT lwDeviceObject11::ReleaseTex(IDirect3DTextureX* tex)
{
    if (tex)
        tex->Release();
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::ReleaseVertexBuffer(IDirect3DVertexBufferX* vb)
{
    if (vb)
        vb->Release();
    if (_bound_vb == vb)
        _bound_vb = 0;
    return LW_RET_OK;
}

LW_RESULT lwDeviceObject11::ReleaseIndexBuffer(IDirect3DIndexBufferX* ib)
{
    if (ib)
        ib->Release();
    if (_bound_ib == ib)
        _bound_ib = 0;
    return LW_RET_OK;
}

LW_END
