#pragma once

#include "MindPowerAPI.h"
#include "MindPowerRenderConfig.h"
#include "lwDirectX.h"

// Long-term native D3D11 access point (Phase 0+). Prefer this over
// IDirect3DDevice9 / GetDevice() in new code on feature/dx11-native.

#ifdef __cplusplus
extern "C" {
#endif

MINDPOWER_API void lwD3D11NativeBindDevice(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapchain);
MINDPOWER_API ID3D11Device* lwD3D11NativeGetDevice();
MINDPOWER_API ID3D11DeviceContext* lwD3D11NativeGetContext();
MINDPOWER_API IDXGISwapChain* lwD3D11NativeGetSwapChain();

#ifdef __cplusplus
}
#endif
