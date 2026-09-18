#include "stdafx.h"
#include "lwD3D11NativeContext.h"
#include "lwD3D11Gaps.h"

#include <d3d11.h>
#include <dxgi.h>

static ID3D11Device* s_native_device = 0;
static ID3D11DeviceContext* s_native_context = 0;
static IDXGISwapChain* s_native_swap = 0;

void lwD3D11NativeBindDevice(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapchain)
{
	s_native_device = device;
	s_native_context = context;
	s_native_swap = swapchain;
	if (device && context)
	{
		lwD3D11Gap(LW_D3D11_INVENTORY, "native-context-bound",
			"lwD3D11NativeContext bound (DX11-only / native migration path)");
	}
}

ID3D11Device* lwD3D11NativeGetDevice()
{
	return s_native_device;
}

ID3D11DeviceContext* lwD3D11NativeGetContext()
{
	return s_native_context;
}

IDXGISwapChain* lwD3D11NativeGetSwapChain()
{
	return s_native_swap;
}
