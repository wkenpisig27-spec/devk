#pragma once

#include "MindPowerRenderConfig.h"
#include "MPRender.h"
#include "lwRenderBackend.h"

// Legacy IDirect3DDevice9* for subsystems that accept nullptr and use dev_obj (DX11).
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
