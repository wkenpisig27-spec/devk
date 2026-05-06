# Centralized Auto-Path Refactor Plan

## Purpose

This document defines a practical, implementation-ready plan to refactor auto-pathing into a centralized system for the PKO client.

Current behavior is spread across UI parsing and form-specific click handlers. The goal is to move to one unified pipeline:

1. Parse clickable intent
2. Resolve destination
3. Execute navigation

All major UI surfaces should rely on one service entry point.

## Current State Summary

### Existing hotspots

- `source/src/game/UIMemo.cpp`
  - Mission text parsing (`CMemoEx::ParseScript`)
  - Hit test and click detection (`CMemoEx::SelMemInfo`, `CMemoEx::MouseRun`)
  - Clickability currently coupled to color (`dwColor == 0xFFFE0000`)

- `source/src/game/UIMisLogForm.cpp`
  - Mission click behavior and direct path execution logic (`CMisLogForm::_ItemClickEvent`)
  - Contains destination parsing/resolution/business rules in form layer

- `source/src/game/UINpcTalkForm.cpp`
  - Separate text interaction path that can later reuse centralized navigation service

- `source/include/game/NPCHelper.h` and `source/src/game/NPCHelper.cpp`
  - NPC lookup source for name/map/position resolution

### Main issues

1. Clickability inferred from render color instead of explicit link metadata
2. Parsing and routing rules duplicated in form code
3. Legacy text formats are brittle (`<j>` tags, mixed free-form text)
4. Difficult to ensure consistent behavior across Quest Log, NPC Talk, and future UI surfaces

## Target Architecture

Introduce a centralized navigation domain module with three layers:

1. **Parser**: Converts text/tag payload to a typed `AutoPathLink`
2. **Resolver**: Produces final map and coordinates with validation
3. **Executor**: Calls pathfinder + navigation bar and returns standardized result

Form classes should not contain route parsing/resolution logic.

## Domain Model (Proposed)

### `AutoPathLinkType`

- `None`
- `CoordinateOnly`
- `MapCoordinate`
- `NpcId`
- `NpcName`
- `ScriptToken`

### `AutoPathLink`

- `AutoPathLinkType type`
- `std::string rawText`
- `std::string displayText`
- `int npcId`
- `std::string npcName`
- `std::string mapName`
- `int x`
- `int y`
- `bool sameMapRequired`
- `bool allowWhileOnBoat`

### `AutoPathContext`

- `CGameScene* scene`
- `CCharacter* mainCharacter`
- `DWORD nowTick`

### `AutoPathError`

- `None`
- `InvalidLink`
- `NpcNotFound`
- `MapNotFound`
- `DifferentMap`
- `InBoatMode`
- `InvalidCoordinates`
- `SceneUnavailable`

### `AutoPathResult`

- `bool success`
- `AutoPathError error`
- `std::string errorText`
- `std::string resolvedMap`
- `int resolvedX`
- `int resolvedY`

## Service Interfaces (Proposed)

### `IAutoPathLinkParser`

- `bool TryParseFromText(const std::string& text, AutoPathLink& outLink) const`
- `bool TryParseFromTagPayload(const std::string& payload, AutoPathLink& outLink) const`

### `IAutoPathResolver`

- `AutoPathResult Resolve(const AutoPathContext& ctx, const AutoPathLink& link) const`

### `IAutoPathExecutor`

- `AutoPathResult Execute(const AutoPathContext& ctx, const AutoPathResult& resolved) const`

### `AutoPathService` (Facade)

- `AutoPathResult Navigate(const AutoPathContext& ctx, const AutoPathLink& link) const`
- `AutoPathResult NavigateFromText(const AutoPathContext& ctx, const std::string& clickedText) const`

## Data Flow

1. UI receives click on text segment
2. UI passes typed link (preferred) or raw text (fallback) to `AutoPathService`
3. Parser creates a normalized `AutoPathLink`
4. Resolver validates and resolves final destination
5. Executor applies find-path state and navigation bar
6. Result returned to UI for message handling

## Backward Compatibility Requirements

The new system must support current content without immediate script rewrite.

### Support legacy inputs

1. Existing `<j>` payload patterns
2. NPC name links currently shown in quest text
3. Mixed sentence text containing coordinates, for example:
   - `Look for <rNewbie Guide - Senna> at (2223, 2785) ...`

### Add new canonical payload formats (for future scripts)

Use a new easy-to-remember tag family: `<nav:...>`.

1. `<nav:npc:5>`
2. `<nav:map:Argent City:2223:2785>`
3. `<nav:coord:2223:2785>`

Optional display-label variant (recommended for content readability):

1. `<nav:npc:5|Newbie Guide - Senna>`
2. `<nav:coord:2223:2785|Go there>`

Parser priority:

1. Typed explicit payload
2. Legacy tagged payload
3. Text fallback extraction

## UI Refactor Strategy

## 1) `CMemoEx` metadata-based clickability

Extend memo segment info to include explicit link metadata:

- `bool isClickable`
- `int linkId`
- `AutoPathLinkType linkType`

Do not rely on render color as click authority.

## 2) Click callback contract

Add explicit callback:

- `evtClickLink(int linkId, const AutoPathLink& link)`

Keep fallback temporarily:

- `evtClickItem(std::string rawText)`

## 3) Form layer simplification

`CMisLogForm::_ItemClickEvent` should delegate to `AutoPathService` and not own map/NPC resolution rules.

## Business Rules (Centralized)

These rules should exist only in resolver/executor:

1. Scene availability checks
2. Main character availability checks
3. Boat restrictions
4. Same-map requirement
5. Coordinate bounds validation
6. NPC/map alias normalization
7. Navigation bar update behavior

## Phased Implementation Plan

### Phase 0: Design baseline

1. Confirm final domain types and error model
2. Freeze accepted payload formats and compatibility policy
3. Freeze UX behavior for errors/cursor states

**Exit criteria**: Interface and behavior contract approved.

### Phase 1: Core module scaffolding

1. Add new navigation headers/sources
2. Implement type definitions and façade signatures

**Exit criteria**: Compiles with no runtime changes yet.

### Phase 2: Resolver and executor

1. Implement lookup via NPCHelper and map normalization
2. Implement path execution wrapper around `g_cFindPathEx` and navigation bar
3. Return standardized `AutoPathResult`

**Exit criteria**: Existing mission click path can call centralized executor for known valid input.

### Phase 3: Parser with adapters

1. Implement typed payload parser
2. Implement legacy `<j>` and mixed-text fallback parser
3. Add parser tests for existing script patterns

**Exit criteria**: Starter quest and common legacy patterns parse correctly.

### Phase 4: `UIMemo` metadata migration

1. Add segment metadata fields
2. Populate link metadata during parse
3. Update hit-test/cursor logic to use metadata

**Exit criteria**: Clickability no longer depends on text color.

### Phase 5: Mission Log integration

1. Replace mission form local routing logic with service call
2. Keep text fallback path enabled temporarily

**Exit criteria**: Quest objective clicks still work with compatibility scripts.

### Phase 6: NPC Talk integration

1. Route NPC dialog link actions via same service
2. Ensure consistent result messaging with Mission Log

**Exit criteria**: Equivalent links behave the same across both UIs.

### Phase 7: Script authoring migration

1. Publish tag authoring guide for explicit typed payloads
2. Keep legacy parser support to avoid forced mass rewrite

**Exit criteria**: New scripts use canonical `<nav:...>` tags; old scripts continue functioning.

### Phase 8: Validation and rollout

1. Add feature flag for centralized navigation path
2. Run compatibility regression suite
3. Roll out gradually by surface

**Exit criteria**: Stable behavior and rollback path available.

## Testing Matrix

### Parser tests

1. NPC name only
2. Coordinate only
3. Map + coordinate
4. Legacy `<j>` coordinate payload
5. New `<nav:...>` payload variants
6. Mixed text with coordinates
7. Invalid payloads and malformed text

### Resolver tests

1. Known NPC name resolves map/x/y
2. Unknown NPC returns `NpcNotFound`
3. Invalid coordinate range returns `InvalidCoordinates`
4. Map mismatch returns `DifferentMap`
5. Boat restriction returns `InBoatMode`

### Integration tests

1. Mission Log clickable objective
2. NPC Talk clickable text
3. Navigation bar update state
4. Route start from current position

## Logging and Diagnostics

Add structured logs at each stage:

1. Parse: link type chosen and payload type
2. Resolve: source used (npc/map/text) and resolved destination
3. Execute: success/failure code and reason

This enables quick production debugging without deep UI tracing.

## Rollback and Safety

1. Add a runtime toggle to route through old behavior if needed
2. Keep old callback path active until migration is complete
3. Remove old path only after full regression pass

## Script Authoring Guide (`<nav:...>`)

This section is for mission/content writers. Use these patterns for clickable navigation text.

### Core rule

Use `<nav:...>` tags for all new clickable routing links.

### Recommended formats

1. NPC by ID (best, stable across localization/name changes)

```
<nav:npc:5|Newbie Guide - Senna>
```

2. Exact coordinate route in current map context

```
<nav:coord:2223:2785|Go to objective>
```

3. Explicit map + coordinate route

```
<nav:map:Argent City:2223:2785|Argent City objective>
```

### Label behavior

`|Label` is optional:

1. Without label, UI can display a default route text.
2. With label, use concise player-facing text.

Examples:

```
<nav:npc:5>
<nav:npc:5|Talk to Senna>
```

### Writing guidelines

1. Prefer `npc` links for NPC objectives.
2. Prefer `map` links for static world points not tied to NPC records.
3. Use `coord` only when map context is guaranteed or intentionally current-map-only.
4. Keep labels short and action-oriented: `Talk to`, `Go to`, `Deliver to`.
5. Do not encode business logic in labels; labels are display text only.

### Migration examples

Legacy text:

```
Look for <rNewbie Guide - Senna> at (2223, 2785)
```

Preferred rewritten text:

```
Look for <nav:npc:5|Newbie Guide - Senna>
```

Legacy explicit tag (if present):

```
<j(2223,2785)>
```

Preferred rewritten tag:

```
<nav:coord:2223:2785|Go there>
```

### Compatibility policy

1. Existing `<j>` and mixed coordinate text remain supported during migration.
2. New content should use `<nav:...>` only.
3. After migration completion, `<j>` support can be deprecated behind a feature flag.

### Validation checklist for content team

1. Click link in Quest Log opens route.
2. Click link in NPC dialog opens route.
3. Route fails gracefully when in boat mode.
4. Cross-map restrictions show the expected message.
5. Link label is readable and localized.

## Estimated Effort

1. Core types + service scaffolding: 0.5 to 1 day
2. Resolver + executor: 1 to 1.5 days
3. Parser + compatibility adapters: 1 to 1.5 days
4. `UIMemo` metadata migration: 1 to 2 days
5. Mission Log + NPC Talk integration: 0.5 to 1 day
6. Testing + rollout hardening: 1 to 1.5 days

Total: approximately 5 to 8 working days.

## Definition of Done

1. Centralized service handles parse/resolve/execute for migrated UI surfaces
2. Mission and NPC talk route behavior is consistent
3. Existing quest scripts continue working
4. Clickability is metadata-driven, not color-driven
5. Feature flag and diagnostics are in place
