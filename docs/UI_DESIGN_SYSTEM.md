# UI Design System

Two RmlUi visual languages share fonts/renderer constraints:

| Theme | Use for | Reference |
|-------|---------|-----------|
| **Notice** (primary for dialogs) | Login, register, confirm, modal forms, select-cha dock, **inventory** | `account.rml` / `selectcha.rml` / `inventory.rml` + `notice.rcss` |
| **Aether** (dark HUD) | Other in-game windows (not inventory) | `system/aether.rcss` |

**Hard rule for forms/dialogs:** match the **Notice** look. Do not restyle login-class screens as dark Aether glass.

---

## Notice — light dialog theme (target)

Soft mobile-MMO / RO Origin style: pale panel, blue header, gold + blue pills.

### Tokens
- Body fill `#f5f9ff` · Border `#9eb8e8`
- Header `#7eb4f0` → `#5a96dc` · Title white bold ~20dp
- Label / muted `#5a7ab0` · Input text `#2a4a80`
- Gold action `#f5d782` → `#e0b040` · Blue action `#7eb4f0` → `#4a8ad0`

### Structure (`account.rml` — modal)
```
scrim → form.notice
  .notice-header → .notice-title
  .notice-body → fields / checks
  .notice-footer → gold btn + blue btn
  .notice-exit-row → Exit pill
```

### Structure (`selectcha.rml` — slim bottom strip)
```
body (no scrim) → .selectcha-bar (input_*.tga shell, absolute bottom)
  .selectcha-actions → compact Notice pills
```
Do not use `.notice` panel chrome for slim bars (baked header band is too tall).

### Soft-AA skins (required on DX9)
- Panel 9-slice with **header baked into top tiles**: `frames/notice/panel_*.tga`
- Inputs: `frames/notice/input_*.tga`
- Pills: `frames/notice/btn_gold_*.tga`, `btn_blue_*.tga`
- Regen: `python tools/gen-notice-skin.py`
- Keep feather tight (~0.85) — avoid blurry halos
- Never stack a second soft-AA header layer over the panel (corners punch through)

### Text centering
Set `line-height` equal to control `height` on inputs and buttons.

### File map
```
client/ui/rml/
  notice.rcss                  ← shared Notice chrome + list styles
  account.rml / account.rcss   ← login
  region.rml / server.rml      ← region + server pick
  selectcha.rml / selectcha.rcss ← character-select bottom dock (no scrim)
  inventory.rml / inventory.rcss ← in-game inventory (Character + Backpack + tabs)
  frames/notice/               ← soft-AA TGA
  fonts/LatoLatin-*.ttf
  system/aether.rcss           ← dark HUD (separate)
```

---

## Aether — dark HUD theme

Reusable dark glass language for in-game windows (not for login/Notice dialogs).
Inspired by Lost Ark / Blue Protocol / BDO / Throne & Liberty — not web dashboards.

### Principles

| Principle | Implementation |
|-----------|----------------|
| Frosted dark glass | Nested panel layers (`ui-window-glass`) |
| Metallic frames | `ui-window` cyan glow + `ui-window-metal` chrome |
| Magical cyan accents | `#3ec9d8` / `#7aebf5` borders, fills, focus rings |
| Corner ornaments | TGA overlays `frames/corner_*.tga` |
| Depth without box-shadow | Layered borders (DX9 has no shadow/filter layers) |
| Hierarchy | Titlebar → section → body → actions |
| Motion | `transition` 0.12–0.18s on hover/focus/active |

**Hard rule:** new *in-game* screens import `system/aether.rcss` and compose existing classes.

### File map

```
client/ui/rml/
  system/
    aether.rcss      ← single <link> include (RmlUi has no @import)
    tokens.rcss …    ← modular sources (edit + mirror into aether.rcss)
  frames/            ← corner + accent TGA (regen via tools/gen-ui-frames.py)
  templates/         ← copy-paste RML shells
  account.rml        ← Notice reference screen (not Aether)
```

When editing modular `system/*.rcss` files, also update the matching section in `aether.rcss`.

### Tokens

**Surfaces:** Void `#05080c` · Panel `#0b141c` · Raised `#121e28` · Sunken `#070d12` · Header `#0f1a24` · Input `#060b10`

**Accent:** `#3ec9d8` · High `#7aebf5` · Low `#1a7f8c` · Dim `#1e4a54` · Text `#b6f4fc`

**Metal:** Hi `#9eb4c2` · Mid `#6a8292` · Lo `#3a4e5a` · Frame `#2a4450`

**Semantic:** Danger `#d45a5a` · Warn `#d4a84a` · Success `#4ecf8a` · Rare `#4a8dff` · Epic `#b07aef` · Legend `#efb04a`

**Type:** `LatoLatin` · sizes 11 / 12 / 14 / 16 / 18 / 20 / 22 dp

**Space:** grid 4–32 dp · prefer soft-AA TGA over large CSS `border-radius` on DX9

### Components (Aether)

- Window: `ui-window` (+ size/metal/glass), ornaments
- Titlebar: `ui-titlebar*`
- Buttons: `ui-btn` primary/secondary/danger/ghost
- Forms: `ui-field`, `ui-label`, `ui-input`, checks/radios, `ui-select`
- Tabs, progress, scroll, tooltip, toast, slots, lists — see `system/*.rcss`

For player-facing confirm/login-style modals, prefer **Notice** chrome instead of Aether.

## Building a new screen

### Notice dialog / form
1. Modal: clone `account.rml` / `region.rml`. Slim bottom bar: clone `selectcha.rml`.
2. Keep element IDs stable for C++ bindings.
3. Reuse `frames/notice/*` skins; regen only via `tools/gen-notice-skin.py`.

### Aether in-game window
1. Copy `templates/window.rml` structure.
2. `<link href="system/aether.rcss"/>` then a thin screen RCSS for layout only.
3. Use component classes only for chrome/controls.

Working directory = `client/` so `frames/` and `fonts/` resolve.

## Renderer constraints (important)

DX9 RmlUi backend does **not** implement layer/filter/box-shadow paths.  
Do **not** use `box-shadow`, filter blur, or advanced shaders — they produce white/broken panels.  
Depth = nested frames + **soft-AA TGA skins** + border color hierarchy.

**Rounded corners:** do not rely on CSS `border-radius` for large shells — use Notice TGA 9-slices / tiled-horizontal.

Decorator paths: `ui/rml/frames/notice/...` from the client working directory.

## Regenerating ornaments

```powershell
python tools/gen-notice-skin.py   # Notice dialog skins
python tools/gen-ui-frames.py     # Aether corner ornaments
```

## Reference implementation

- Modal: `account.rml` (canonical Notice dialog)
- Bottom strip: `selectcha.rml` (compact dock without panel header)
- Inventory: `inventory.rml` (Notice-inspired in-game Character + Backpack)
