#pragma once

#include "MindPowerAPI.h"
#include "lwHeader.h"
#include "lwDirectX.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct IDXGISwapChain;

LW_BEGIN

// DX11 scene post: HDR MSAA color, highlight-only tone, bloom, unsharp.
// Params are live; hdr on/off is sampled when CreateTargets runs.

MINDPOWER_API void lwD3D11PostSetParams(
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
    float fill);

MINDPOWER_API int lwD3D11PostWantsHdr();
MINDPOWER_API int lwD3D11PostIsActive();

MINDPOWER_API LW_RESULT lwD3D11PostInit(ID3D11Device* device, ID3D11DeviceContext* context);
MINDPOWER_API void lwD3D11PostShutdown();
MINDPOWER_API void lwD3D11PostReleaseTargets();
MINDPOWER_API LW_RESULT lwD3D11PostCreateTargets(UINT width, UINT height, UINT msaa);

MINDPOWER_API ID3D11RenderTargetView* lwD3D11PostSceneRTV();
MINDPOWER_API ID3D11DepthStencilView* lwD3D11PostSceneDSV();

MINDPOWER_API LW_RESULT lwD3D11PostResolve(
    IDXGISwapChain* swapchain,
    ID3D11RenderTargetView* backbuffer_rtv,
    UINT width,
    UINT height);

LW_END
