#include "stdafx.h"
#include "lwD3D11Gaps.h"
#include "lwRenderBackend.h"

#include <set>
#include <string>
#include <stdarg.h>

static CRITICAL_SECTION s_cs;
static int s_cs_ready = 0;
static std::set<std::string> s_seen;

static void EnsureLock()
{
	if (!s_cs_ready)
	{
		InitializeCriticalSection(&s_cs);
		s_cs_ready = 1;
	}
}

static const char* KindTag(lwD3D11GapKind kind)
{
	switch (kind)
	{
	case LW_D3D11_SKIP:      return "SKIP";
	case LW_D3D11_FALLBACK:  return "FALLBACK";
	case LW_D3D11_INVENTORY: return "INVENTORY";
	case LW_D3D11_GAP:
	default:                 return "GAP";
	}
}

void lwD3D11Gap(lwD3D11GapKind kind, const char* id, const char* fmt, ...)
{
	if (!id || !id[0])
		return;

	EnsureLock();
	EnterCriticalSection(&s_cs);
	std::pair<std::set<std::string>::iterator, bool> inserted = s_seen.insert(id);
	const int first = inserted.second ? 1 : 0;
	LeaveCriticalSection(&s_cs);

	if (!first)
		return;

	char body[512];
	body[0] = 0;
	if (fmt && fmt[0])
	{
		va_list args;
		va_start(args, fmt);
		_vsnprintf(body, sizeof(body) - 1, fmt, args);
		va_end(args);
		body[sizeof(body) - 1] = 0;
	}

	if (body[0])
		LG("d3d11gaps", "[%s] %s — %s (further hits for this id suppressed)\n", KindTag(kind), id, body);
	else
		LG("d3d11gaps", "[%s] %s (further hits for this id suppressed)\n", KindTag(kind), id);
}

void lwD3D11GapReportInventory()
{
	// Quiet on the production DX9 path. List the islands when someone asked
	// for DX11 so the leak catalog shows up in log/game/d3d11gaps_*.log.
	if (lwGetRequestedRenderBackend() != LW_RENDER_BACKEND_DX11)
	{
		LG("d3d11gaps", "[SysGraphics] 18 D3D9-island groups on file; set [video] renderer=dx11 to list them\n");
		return;
	}

	struct Entry
	{
		const char* id;
		const char* detail;
	};

	static const Entry kIslands[] = {
		{ "inv-getdevice-raw",
		  "lwIDeviceObject::GetDevice / MPRender::GetDevice return IDirect3DDevice9* (40+ call sites; NULL on DX11)" },
		{ "inv-fvf",
		  "SetFVF still used in lwResourceMgr, fonts, particles, minimap — D3D11 has no FVF" },
		{ "inv-texture-stage",
		  "Mesh PS maps stage-1/2 combiners (modulate/add/mod2x/lit-A); remaining FF stages still DX9-only" },
		{ "inv-d3dx-effect",
		  "eff.hlsl is compiled for t0-t6; CMPModelEff uses the VS-path on DX11 with TFACTOR + vertex UVs" },
		{ "inv-d3dx-sprite-font-tex",
		  "D3DXCreateSprite is skipped; UI/console sprites blit. CMPFont GPU atlas locks empty DX11 textures" },
		{ "inv-device-lost",
		  "DX11 resize is DXGI ResizeBuffers + borderless HWND; D3D9 lose/reset fan-out stays skipped" },
		{ "inv-stream-1mb",
		  "lwStreamObj 1 MB static-stream cap is a DX9 leftover; lift on D3D11" },
		{ "inv-shader-mgr9",
		  "lwShaderMgr9 / ShaderLoad register .hlsl keys; ShaderMgr11 compiles SM4 from shader\\hlsl" },
		{ "inv-fixed-function",
		  "lwRenderCtrlVSFixedFunction + lwRenderCtrlEmb still use the D3D9 FF pipeline" },
		{ "inv-pixel-shader-raw",
		  "lwxRenderCtrVS calls IDirect3DDevice9::SetPixelShader directly" },
		{ "inv-mpfont",
		  "MPFont.cpp / BitmapFont.cpp talk to the raw device" },
		{ "inv-particles",
		  "MPParticleCtrl.cpp / MPParticleSys.cpp — DrawPrimitiveUP + GetDevice" },
		{ "inv-terrain",
		  "MPMap.cpp (~86 D3D9 calls) — terrain is a world-slice, not Slice 1" },
		{ "inv-shadow-shade",
		  "MPShadowMap.cpp / MPShadeMap.cpp — render-target + device-lost chain" },
		{ "inv-physique",
		  "lwPhysique.cpp still binds VS constants through the D3D9 device object" },
		{ "inv-minimap",
		  "SMallMap.cpp (~100 D3D9 calls) — UI/map slice" },
		{ "inv-ui-captcha",
		  "UINumAnswer captcha texture is DeviceObject CreateTexture + CPU lock; login captcha is GDI text" },
		{ "inv-scene-misc",
		  "CEffectBox/CPathBox and color-filter textures use DeviceObject; CharacterModel Lit uses synthetic blend-stage caps" },
	};

	const int n = (int)(sizeof(kIslands) / sizeof(kIslands[0]));
	LG("d3d11gaps", "[SysGraphics] D3D9-island inventory (%d groups) — Slice 0 catalog, not live hits\n", n);
	for (int i = 0; i < n; ++i)
		lwD3D11Gap(LW_D3D11_INVENTORY, kIslands[i].id, "%s", kIslands[i].detail);
}
