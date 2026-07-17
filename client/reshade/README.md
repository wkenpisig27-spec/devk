# ReShade FXAA (optional alpha-edge AA)

Engine MSAA (`video.msaa` in `user\system.ini`: `0` / `2` / `4` / `8`) cleans geometric edges.
ReShade FXAA is complementary: it softens alpha-tested foliage, hair, and particle edges that MSAA misses.

This is optional. Leave it uninstalled if you prefer maximum sharpness or FPS.

## Setup

1. Download ReShade from https://reshade.me/
2. Run the installer, select `Game.exe` (this client), API **Direct3D 9**
3. When asked for effect packages, include at least **FXAA** (or SMAA)
4. Copy `PKO_FXAA.ini` from this folder next to `Game.exe`, or load it from ReShade’s Home overlay → *Presets*
5. In-game: Home → select preset **PKO_FXAA** → enable technique **FXAA**

## Notes

- Keep engine `video.msaa` at `4` (default) or `8` for hard building edges; use `2` or `0` on low-end GPUs
- FXAA + MSAA together is intentional; turn FXAA off if UI text looks too soft
- Do not install ReShade’s depth effects unless you know you need them — they can break this client’s UI
