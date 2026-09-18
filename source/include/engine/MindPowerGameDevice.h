#pragma once

#include "MindPowerRenderConfig.h"
#include "MPRender.h"
#include "lwRenderBackend.h"

// D3D9 leftover. DX11-only play path uses lwD3D11NativeGetDevice() / GetContext().
inline IDirect3DDeviceX* MP_LegacyD3D9DeviceOpt()
{
#if MINDPOWER_USE_D3D9_DEVICE
	if (lwIsDx11Active())
		return nullptr;
	return g_Render.GetDevice();
#else
	return nullptr;
#endif
}
