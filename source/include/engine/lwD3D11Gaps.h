#pragma once

#include "MindPowerAPI.h"

// First-hit logger for leftover D3D9 islands. Each `id` is emitted once.
// Channel: LG("d3d11gaps", ...). Call from DX9-only code as those sites are
// migrated; Slice 0 dumps the known inventory at init when dx11 is requested.

enum lwD3D11GapKind
{
	LW_D3D11_GAP = 0,       // unported call that will null-deref / skip on DX11
	LW_D3D11_SKIP = 1,      // D3D9-only path deliberately not taken on DX11
	LW_D3D11_FALLBACK = 2,  // running a substitute instead of native D3D11
	LW_D3D11_INVENTORY = 3, // catalog entry dumped at init
};

#ifdef __cplusplus
extern "C" {
#endif

MINDPOWER_API void lwD3D11Gap(lwD3D11GapKind kind, const char* id, const char* fmt, ...);
MINDPOWER_API void lwD3D11GapReportInventory();

#ifdef __cplusplus
}
#endif
