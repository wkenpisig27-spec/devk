# Network Tests — Phase 0 Baseline

Smoke tests and inventory for the MMORPG packet infrastructure refactor (Option B).

## Quick start

```bat
REM 1. Generate opcode inventory
python helper\network-tests\scripts\generate_opcode_table.py

REM 2. Build Release (requires VS developer shell)
powershell -File helper\network-tests\scripts\build-release.ps1

REM 3. Start GateServer (and dependent servers), then:
python helper\network-tests\net-smoke.py
python helper\network-tests\test_packet_read_contract.py
```

## Files

| Path | Purpose |
|------|---------|
| `BASELINE.md` | Frozen build/config/protocol inventory |
| `STATUS.md` | Task checklist — update after each work session |
| `net-smoke.py` | Automated T0-connect / T0-ping |
| `data/opcodes.csv` | Machine-readable opcode list |
| `scripts/generate_opcode_table.py` | Regenerate opcodes.csv |
| `scripts/build-release.ps1` | MSBuild Release\|x64 verification |
| `last-smoke-result.txt` | Written by net-smoke.py |

## Manual tests (Phase 0)

1. **T0-login:** Start full stack, launch client, log in with a test account.
2. **T0-enter:** Select character, enter world.
3. **T0-inter-server:** Confirm `server/LOG/GameServer/*/GameLogin.log` shows gate connected.

Mark these in `STATUS.md` when done.
