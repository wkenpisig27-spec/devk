#include "stdafx.h"
#include "lwRenderBackend.h"
#include "MindPowerRenderConfig.h"
#include "lwD3D11Gaps.h"

#include <string.h>

#if MINDPOWER_USE_D3D9_DEVICE
static lwRenderBackend s_requested = LW_RENDER_BACKEND_DX9;
static lwRenderBackend s_active = LW_RENDER_BACKEND_DX9;
#else
static lwRenderBackend s_requested = LW_RENDER_BACKEND_DX11;
static lwRenderBackend s_active = LW_RENDER_BACKEND_DX11;
#endif
static int s_resolved = 0;

static int EqualsBackendName(const char* name, const char* a, const char* b)
{
	if (!name || !name[0])
		return 0;
	if (_stricmp(name, a) == 0)
		return 1;
	if (b && _stricmp(name, b) == 0)
		return 1;
	return 0;
}

lwRenderBackend lwParseRenderBackend(const char* name, lwRenderBackend fallback)
{
#if !MINDPOWER_USE_D3D9_DEVICE
	(void)name;
	(void)fallback;
	return LW_RENDER_BACKEND_DX11;
#else
	if (!name || !name[0])
		return fallback;
	if (EqualsBackendName(name, "dx9", "d3d9"))
		return LW_RENDER_BACKEND_DX9;
	if (EqualsBackendName(name, "dx11", "d3d11"))
		return LW_RENDER_BACKEND_DX11;
	return fallback;
#endif
}

const char* lwRenderBackendName(lwRenderBackend backend)
{
	return (backend == LW_RENDER_BACKEND_DX11) ? "dx11" : "dx9";
}

void lwSetRequestedRenderBackend(lwRenderBackend backend)
{
#if !MINDPOWER_USE_D3D9_DEVICE
	(void)backend;
	s_requested = LW_RENDER_BACKEND_DX11;
	if (!s_resolved)
		s_active = LW_RENDER_BACKEND_DX11;
#else
	s_requested = backend;
	if (!s_resolved)
		s_active = LW_RENDER_BACKEND_DX9;
#endif
}

lwRenderBackend lwGetRequestedRenderBackend()
{
	return s_requested;
}

lwRenderBackend lwGetActiveRenderBackend()
{
	return s_active;
}

#if MINDPOWER_USE_D3D9_DEVICE
int lwIsDx11Active()
{
	return (s_active == LW_RENDER_BACKEND_DX11) ? 1 : 0;
}
#endif

lwRenderBackend lwResolveRenderBackend()
{
	s_resolved = 1;
#if !MINDPOWER_USE_D3D9_DEVICE
	s_requested = LW_RENDER_BACKEND_DX11;
	s_active = LW_RENDER_BACKEND_DX11;
	lwD3D11Gap(LW_D3D11_INVENTORY, "dx11-only-build",
		"MINDPOWER_DX11_ONLY — D3D9 backend disabled at compile time");
#else
	s_active = s_requested;
#endif

	if (s_active == LW_RENDER_BACKEND_DX11)
	{
		LG("d3d11gaps", "[SysGraphics] D3D11 backend requested — constructing DeviceObject11 (Slice 1: swapchain + clear/present)\n");
	}

	LG("d3d11gaps", "[SysGraphics] render backend requested=%s active=%s\n",
		lwRenderBackendName(s_requested),
		lwRenderBackendName(s_active));

	return s_active;
}
