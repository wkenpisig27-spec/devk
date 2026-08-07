# Agent notes (devk)

## UI

Player-facing **dialogs / forms / login / character-select overlays** use the **Notice** light theme.

- Rule (always on): [`.cursor/rules/notice-ui-theme.mdc`](.cursor/rules/notice-ui-theme.mdc)
- Design system: [`docs/UI_DESIGN_SYSTEM.md`](docs/UI_DESIGN_SYSTEM.md)
- RmlUi overview: [`docs/RMLUI.md`](docs/RMLUI.md)
- Templates: `client/ui/rml/account.rml` (modal), `client/ui/rml/selectcha.rml` (bottom strip), `client/ui/rml/inventory.rml` (in-game inventory), shared `notice.rcss`

**Inventory** uses Notice-inspired light chrome (`inventory.rml`). Other in-game HUD chrome uses **Aether** (`client/ui/rml/system/aether.rcss`) — do not mix themes on the same screen.
