# Option B / Phase-6 — Execution Status

Track progress for autonomous refactoring. Update after each completed task.

**Current phase:** Phase 1 — Immediate fixes (Sprint 1)  
**Last updated:** 2026-06-26

---

## Phase 0 — Baseline & safety net

| Task | Status | Date | Notes |
|------|--------|------|-------|
| 0.1 Opcode CSV from NetCommand.h | done | 2026-06-26 | `scripts/generate_opcode_table.py` → `data/opcodes.csv` |
| 0.1 Build target inventory | done | 2026-06-26 | `BASELINE.md` |
| 0.1 Network config documentation | done | 2026-06-26 | Gate/Game cfg documented |
| 0.2 T0 smoke test script | done | 2026-06-26 | `net-smoke.py` (connect + ping automated) |
| 0.3 NET_AUDIT_DIAG flag | done | 2026-06-26 | `NetAuditDiag.h` + `Receiver.cpp` |
| 0.4 Release build verify | done | 2026-06-26 | User confirmed Release\|x64 build, no errors |
| 0.4 T0-connect automated | done | 2026-06-26 | PASS cmd=943 (RSA handshake) |
| 0.4 T0-ping automated | done | 2026-06-26 | PASS |
| 0.4 T0-login manual | done | 2026-06-26 | User confirmed build+login path verified |
| **Phase 0 exit gate** | **done** | 2026-06-26 | Baseline locked; proceed to Phase 1 |

---

## Phase 1 — Immediate fixes (Sprint 1)

| Task | Status | Date | Notes |
|------|--------|------|-------|
| I4 Read contract + caller audit | done | 2026-06-26 | Packet.cpp/h; server critical paths; `PacketReadUtils.h` |
| I5 Fail closed on parse exceptions | done | 2026-06-26 | `PacketPipeline.h`; Receiver, CommRPC, PacketQueue |
| I2 Alloc caps / inter-server limits | done | 2026-06-26 | `NetLimits.h`, `__recvbuf_cap`, Receiver alloc cap |
| I1 Connection rate limiting | done | 2026-06-26 | OnConnect IP limits; traffic flood disconnect |
| I3 Steady clock keepalive | done | 2026-06-26 | `GetSteadyMs()`; recv/send/del timers use steady_clock |
| I7 volatile → atomic | done | 2026-06-26 | DataSocket flags/stats use `std::atomic` |
| I6 AES-GCM wire integrity | done | 2026-06-26 | CommEncrypt=2, WireGcm* in PacketEncryption |
| **Phase 1 exit gate** | **done** | 2026-06-26 | Sprint 1 complete |

---

## Phase 2 — Foundation (not started)

| Task | Status | Date | Notes |
|------|--------|------|-------|
| M1 Opcode table (OpcodeMeta) | pending | | |
| M5-prep PacketReader/Writer | pending | | |
| M2-prep Handler registry + 5 pilots | pending | | |
| **Phase 2 exit gate** | pending | | |

---

## Phase 3 — Medium-term core (not started)

See master plan for Tracks A–E (M2 bulk, M3 sessions, M4 backplane, M5 validation, M6 zero-copy).

---

## Known regressions

- (none recorded)

---

## Last smoke test run

```
2026-06-26 15:48:13 — T0-connect PASS, T0-ping PASS (127.0.0.1:1973)
See last-smoke-result.txt for full output.
```

---

## Resume prompt for next session

> Phase 1 complete. Start **Phase 2** (M1 opcode table) or run full-stack test with `CommEncrypt=2`. Read `helper/network-tests/STATUS.md`.
