#pragma once

#include "MindPowerAPI.h"
#include "MindPowerRenderConfig.h"
#include "lwDirectX.h"

// Play-path D3D11 handle on MINDPOWER_DX11_ONLY (Debug and Release).
// Bound from lwDeviceObject11::CreateDevice. Do not store or call
// IDirect3DDevice9* / MPRender::GetDevice() for live draws.

#ifdef __cplusplus
extern "C" {
#endif

MINDPOWER_API void lwD3D11NativeBindDevice(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapchain);
MINDPOWER_API void lwD3D11NativeUnbindDevice();
MINDPOWER_API int lwD3D11NativeIsBound();
MINDPOWER_API ID3D11Device* lwD3D11NativeGetDevice();
MINDPOWER_API ID3D11DeviceContext* lwD3D11NativeGetContext();
MINDPOWER_API IDXGISwapChain* lwD3D11NativeGetSwapChain();

#ifdef __cplusplus
}
#endif
