# RmlUi (DirectX 9 overlay)

Parallel RmlUi overlay on top of the existing `CFormMgr` UI.

## Layout

| Path | Purpose |
|------|---------|
| `third_party/RmlUi` | RmlUi sources |
| `third_party/freetype` | FreeType (font engine) |
| `source/include/game/rmlui/` | Integration headers |
| `source/src/game/rmlui/` | DX9 renderer + manager |
| `client/ui/rml/` | RML/RCSS documents |
| `source/lib/Release/rmlui*.lib` | Prebuilt Release static libs |
| `source/lib/Debug/rmlui_debugger.lib`, `freetyped.lib` | Smaller Debug libs (main `rmlui.lib` is built locally) |

## Rebuild libraries

From the repo root (requires VS 2022/18 CMake tools):

```powershell
.\tools\build-rmlui.ps1              # Debug + Release
.\tools\build-rmlui.ps1 -Config Debug  # Debug only (needed for Debug game builds)
```

Note: `source/lib/Debug/rmlui.lib` is not committed (over GitHub's 100MB limit). Run the script above after clone.
## Runtime

- Hello document: `client/ui/rml/hello.rml`
- Font: loads `C:/Windows/Fonts/segoeui.ttf` (or arial) if no local TTF is present
- **F8** toggles the RmlUi debugger
- Working directory should be the `client/` folder (same as the game normally)

## Code hooks

- Init: `CGameApp::_Init` after `GetRender().Init()`
- Update/mouse: `CGameApp::_PreMouseRun`
- Render: after `CFormMgr::s_Mgr.Render()` in `CGameApp::_Render`
- Input: `CGameApp::HandleWindowMsg` / `MouseScroll` / mouse button early-outs
- Shutdown: `CGameApp::_End` before `CFormMgr::Clear`
