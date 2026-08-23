# RmlUi (DirectX 9 overlay)

Parallel RmlUi overlay on top of the existing `CFormMgr` UI. The account login window is served from RML/RCSS instead of `frmAccount`.

## Layout

| Path | Purpose |
|------|---------|
| `third_party/RmlUi` | Trimmed RmlUi sources (Core + Debugger + Win32 platform) |
| `third_party/freetype` | FreeType (font engine) |
| `source/include/game/rmlui/` | Integration headers |
| `source/src/game/rmlui/` | DX9 renderer + manager + account form |
| `client/ui/rml/` | RML/RCSS documents (`account.rml`) + shipped `fonts/` |
| `source/lib/Release/rmlui*.lib` | Prebuilt Release static libs |
| `source/lib/Debug/rmlui_debugger.lib`, `freetyped.lib` | Smaller Debug libs (main `rmlui.lib` is built locally) |

Samples, Tests, unused backends (GL/VK/DX12/SDL/…), and optional Lua/Lottie/SVG plugins are **not** vendored.

## Rebuild libraries

From the repo root (requires VS 2022/18 CMake tools):

```powershell
.\tools\build-rmlui.ps1              # Debug + Release
.\tools\build-rmlui.ps1 -Config Debug  # Debug only (needed for Debug game builds)
```

Note: `source/lib/Debug/rmlui.lib` is not committed (over GitHub's 100MB limit). Run the script above after clone.

## Runtime

- Account document: `client/ui/rml/account.rml` (**Notice** light dialog theme — see `docs/UI_DESIGN_SYSTEM.md`)
- Region / server: `region.rml`, `server.rml` (same Notice chrome via `notice.rcss`)
- Character select: `selectcha.rml` bottom dock (no scrim; replaces `frmSelect`)
- Inventory: `inventory.rml` Notice-inspired Character + Backpack (replaces `frmInv` chrome)
- Guild: `guild.rml` Notice guild manager (replaces `frmManage` chrome)
- Design system: Notice skins in `frames/notice/`; dark HUD classes in `system/aether.rcss`
- Font: shipped `client/ui/rml/fonts/LatoLatin-*.ttf` registered as family `LatoLatin` (Windows Arial/Segoe only as fallback if those files are missing)
- **F8** toggles the RmlUi debugger (optional; UI text no longer depends on it)
- Working directory should be the `client/` folder (same as the game normally)

## Code hooks

- Init: `CGameApp::_Init` after `GetRender().Init()`
- Update/mouse: `CGameApp::_PreMouseRun`
- Render: after `CFormMgr::s_Mgr.Render()` in `CGameApp::_Render`
- Input: `CGameApp::HandleWindowMsg` / `MouseScroll` / mouse button early-outs
- Shutdown: `CGameApp::_End` before `CFormMgr::Clear`
