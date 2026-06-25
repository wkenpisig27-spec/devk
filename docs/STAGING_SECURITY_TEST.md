# Staging Test Checklist — Security Remediation

**Environment:** staging only · **Duration:** ~45–60 min · **Required:** 2 test characters (Char A & Char B), GM access optional

**Build under test:** GameServer, GateServer, GroupServer, AccountServer (Release x64) + updated Lua scripts in `server/resource/script/calculate/`

**Before testing:** deploy all four servers, restart after deploy. Run `database/[12]ItemAuditLog.sql` on GameDB if audit DB tests are included.

| Config (GameServer.cfg) | Staging value | Production target |
|-------------------------|---------------|-------------------|
| `supercmd` | `0` (or note if left `1` — expect startup warning) | `0` |
| `enforce_speed_hack` | `1` (default if omitted) | `1` |
| `log_dir` | e.g. `LOG\GameServer` | as deployed |

**Log locations:** `server/LOG/GameServer/` · `server/LOG/AccountServer/` · look for files named `Security_YYYY-MM-DD.log`

**Tester:** _________________ **Date:** __________ **Build/branch:** _________________

---

## Pass / Fail legend

| Result | Meaning |
|--------|---------|
| **PASS** | Matches expected behavior |
| **FAIL** | Wrong behavior — note steps & log snippet in “Notes” column |
| **SKIP** | Not testable on this staging setup |
| **N/A** | Requires GM/DB access not available |

---

## A. Smoke test (15 min) — must all PASS before sign-off

| # | Test | Steps | Expected | P/F | Notes |
|---|------|-------|----------|-----|-------|
| A1 | Login | Log in Char A and Char B | Both enter world normally | | |
| A2 | Trade | A trades 1 item + gold to B; both relog | Inventories & gold match on both sides after relog | | |
| A3 | Stall | B buys 1 item from A’s stall; both relog | Buyer has item, seller gold/items correct | | |
| A4 | Bank | A deposits then withdraws 1 item at bank NPC; relog | Item count unchanged after relog | | |
| A5 | Normal play | 5 min move, fight, use skills | No false “can’t move” / random disconnects | | |

---

## B. Economy integrity

| # | Test | Steps | Expected | P/F | Notes |
|---|------|-------|----------|-----|-------|
| B1 | Trade fail-safe | *(Optional, needs DB pause)* Confirm trade while SQL is briefly unavailable | Both chars get failure notice; **no** success UI; items/gold unchanged after relog | | |
| B2 | Stall fail-safe | Same as B1 on stall purchase | Transaction reversed or never completes; no duped items | | |
| B3 | Bank fail-safe | Same as B1 on bank transfer | Bags match pre-operation after relog | | |
| B4 | IMP cap | Trade IMP near 2B cap between two chars | Overflow trade rejected with system notice | | |
| B5 | Item audit | After A2/A3/A4, run SQL below | Rows appear (or `Security` log fallback if migration not run) | | |

```sql
SELECT TOP 20 * FROM dbo.item_audit_log ORDER BY ts DESC;
```

---

## C. Anti-cheat & rate limits

| # | Test | Steps | Expected | P/F | Notes |
|---|------|-------|----------|-----|-------|
| C1 | Normal movement | Run, jump, lag simulation (if possible) | No move rejection under normal play | | |
| C2 | Skill range | Cast targeted skill; move out of range before hit lands | No damage / effect; optional `Security` log | | |
| C3 | Packet spam | Rapid move or trade clicks for ~10 sec | Server stable; extra packets dropped (may be silent) | | |
| C4 | Speed enforcement | *(Test env / known tool only)* Repeat speed violations | Move cancelled after repeated violations; `Security` log | | |

---

## D. Recovery & ops

| # | Test | Steps | Expected | P/F | Notes |
|---|------|-------|----------|-----|-------|
| D1 | SuperCmd warning | Start GameServer with `supercmd = 1` | `WARNING: SuperCmd enabled` in Security log at startup | | |
| D2 | SuperCmd prod | Start with `supercmd = 0` | No SuperCmd warning | | |
| D3 | Log rotation | Confirm log files use daily suffix | Files like `Security_2026-06-25.log`, append not wipe | | |
| D4 | DB degraded | *(Optional)* Stop DB; trigger 3+ saves (trade/bank) | System notice: economy disabled; move/chat still work | | |
| D5 | Stale login | *(Optional, DB)* Set test account ONLINE with old timestamp; wait ≥30 min | Account reset OFFLINE; AccountServer Security log | | |
| D6 | Corrupt kitbag | *(Optional, DB)* Break kitbag checksum on test char | Login succeeds, empty bag, `KITBAG_CHECKSUM_FAIL` in Security log | | |

---

## E. RNG / EXP (GM or script access)

| # | Test | Steps | Expected | P/F | Notes |
|---|------|-------|----------|-----|-------|
| E1 | Drop rate clamp | High drop-rate buff + farming | No “everything always drops” behavior | | |
| E2 | Forge rate | Refine with high success modifiers | Not 100% success every attempt | | |
| E3 | High EXP | GM addexp on high-level char; check DB EXP | No negative / wrapped EXP value | | |

---

## Sign-off

| Area | PASS | FAIL | SKIP |
|------|------|------|------|
| A Smoke | /5 | | |
| B Economy | /5 | | |
| C Anti-cheat | /4 | | |
| D Ops | /6 | | |
| E RNG/EXP | /3 | | |

**Overall result:** ☐ **APPROVED for production** · ☐ **BLOCKED** — attach failed test #s and log excerpts

**Blocking issues:**

```
(list test IDs and log paths)
```

**Approved by:** _________________ **Date:** __________

---

*Reference: security remediation batches 1–7. Out of scope: client-side RNG, full inventory rewrite. Item audit DB requires `database/[12]ItemAuditLog.sql`.*
