# Agent notes (devk)

## UI

Player-facing **dialogs / forms / login / character-select overlays** use the **Notice** light theme.

- Rule (always on): [`.cursor/rules/notice-ui-theme.mdc`](.cursor/rules/notice-ui-theme.mdc)
- Design system: [`docs/UI_DESIGN_SYSTEM.md`](docs/UI_DESIGN_SYSTEM.md)
- RmlUi overview: [`docs/RMLUI.md`](docs/RMLUI.md)
- Templates: `client/ui/rml/account.rml` (modal), `client/ui/rml/selectcha.rml` (bottom strip), shared `notice.rcss`

In-game HUD chrome uses **Aether** (`client/ui/rml/system/aether.rcss`) — do not mix Notice and Aether on the same screen.
