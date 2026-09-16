#include "stdafx.h"
#include "lwRenderBackend.h"
#include "lwD3D11Gaps.h"

#include <string.h>

static lwRenderBackend s_requested = LW_RENDER_BACKEND_DX9;
static lwRenderBackend s_active = LW_RENDER_BACKEND_DX9;
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
	if (!name || !name[0])
		return fallback;
	if (EqualsBackendName(name, "dx9", "d3d9"))
		return LW_RENDER_BACKEND_DX9;
	if (EqualsBackendName(name, "dx11", "d3d11"))
		return LW_RENDER_BACKEND_DX11;
	return fallback;
}

const char* lwRenderBackendName(lwRenderBackend backend)
{
	return (backend == LW_RENDER_BACKEND_DX11) ? "dx11" : "dx9";
}

void lwSetRequestedRenderBackend(lwRenderBackend backend)
{
	s_requested = backend;
	if (!s_resolved)
		s_active = LW_RENDER_BACKEND_DX9;
}

lwRenderBackend lwGetRequestedRenderBackend()
{
	return s_requested;
}

lwRenderBackend lwGetActiveRenderBackend()
{
	return s_active;
}

int lwIsDx11Active()
{
	return (s_active == LW_RENDER_BACKEND_DX11) ? 1 : 0;
}

lwRenderBackend lwResolveRenderBackend()
{
	s_resolved = 1;
	s_active = s_requested;

	if (s_active == LW_RENDER_BACKEND_DX11)
	{
		LG("d3d11gaps", "[SysGraphics] D3D11 backend requested — constructing DeviceObject11 (Slice 1: swapchain + clear/present)\n");
	}

	LG("d3d11gaps", "[SysGraphics] render backend requested=%s active=%s\n",
		lwRenderBackendName(s_requested),
		lwRenderBackendName(s_active));

	return s_active;
}
