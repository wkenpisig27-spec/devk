#pragma once

#include "MindPowerAPI.h"
#include "MindPowerRenderConfig.h"

// Runtime renderer selection. Compile-time LW_USE_DX9 stays; DX11 is an
// optional backend chosen from [video] renderer= in user/system.ini.
// feature/dx11-native: MINDPOWER_DX11_ONLY (Debug+Release) forces DX11.

enum lwRenderBackend
{
	LW_RENDER_BACKEND_DX9 = 0,
	LW_RENDER_BACKEND_DX11 = 1,
};

#ifdef __cplusplus
extern "C" {
#endif

MINDPOWER_API lwRenderBackend lwParseRenderBackend(const char* name, lwRenderBackend fallback);
MINDPOWER_API const char* lwRenderBackendName(lwRenderBackend backend);

MINDPOWER_API void lwSetRequestedRenderBackend(lwRenderBackend backend);
MINDPOWER_API lwRenderBackend lwGetRequestedRenderBackend();
MINDPOWER_API lwRenderBackend lwGetActiveRenderBackend();

#if !MINDPOWER_USE_D3D9_DEVICE
#ifdef __cplusplus
inline int lwIsDx11Active() { return 1; }
#else
#define lwIsDx11Active() 1
#endif
#else
MINDPOWER_API int lwIsDx11Active();
#endif

// Map requested -> active. Call once from MPRender::Init before device create.
MINDPOWER_API lwRenderBackend lwResolveRenderBackend();

#ifdef __cplusplus
}
#endif
