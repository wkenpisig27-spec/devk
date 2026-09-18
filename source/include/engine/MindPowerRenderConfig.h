#pragma once

// Compile-time renderer policy for the native D3D11 migration branch.
//
// MINDPOWER_DX11_ONLY — set in MindPower3D + game Debug|x64 and Release|x64.
//   * No lwDeviceObject (D3D9) construction
//   * Backend forced to DX11 at runtime
//   * Play-path GPU handle is lwD3D11NativeContext (not IDirect3DDevice9*)
//   * lwDeviceObject.cpp is removed; play path is lwDeviceObject11
//
// MINDPOWER_NATIVE_DX11_API — Phase 4+ gate for subsystems that include d3d11.h
//   via lwDirectX.h. Reserved; do not require it for existing DeviceObject11 paths.

#if defined(MINDPOWER_DX11_ONLY)
#define MINDPOWER_USE_D3D9_DEVICE 0
#else
#define MINDPOWER_USE_D3D9_DEVICE 1
#endif

#if defined(MINDPOWER_NATIVE_DX11_API) && !defined(MINDPOWER_DX11_ONLY)
#error MINDPOWER_NATIVE_DX11_API requires MINDPOWER_DX11_ONLY
#endif

#ifdef __cplusplus
inline int MindPowerDx11OnlyBuild()
{
#if MINDPOWER_USE_D3D9_DEVICE
	return 0;
#else
	return 1;
#endif
}
#endif
