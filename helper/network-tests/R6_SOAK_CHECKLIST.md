# R6.1 — 30-minute Phase 3 exit soak checklist

**Purpose:** Manual sign-off that the full server stack behaves cleanly under normal play after audit remediation R1–R5.  
**Prerequisite:** Gate + Group + Game (+ Account) running from the **same Release|x64 build**; deploy to `server/` and restart via `Connect server.bat`.  
**Duration:** ~30 minutes wall clock with 1–2 clients.

---

## Pre-soak automated gate (T0)

Run before starting the manual soak (or in CI/agent session):

```powershell
cd C:\Users\Ken\Desktop\Github\devk
python helper/network-tests/net-smoke.py
```

**Expect:** `T0-connect PASS`, `T0-ping PASS`. Results append to `helper/network-tests/last-smoke-result.txt`.

---

## Manual soak script (~30 min)

Use a test account with at least two characters if possible. Check off each step; note time and any client/server anomalies.

| # | Step | Time | Pass | Notes |
|---|------|------|------|-------|
| 1 | Start stack (Gate, Group, Game, Account) — matched binaries | | ☐ | |
| 2 | **Login** — RSA handshake → char list | | ☐ | |
| 3 | **Enter world** — BGNPLAY → map load | | ☐ | |
| 4 | **Move** — walk/run ~2 min (`CM_BEGINACTION` MOVE) | | ☐ | |
| 5 | **Skill** — cast or combat action | | ☐ | |
| 6 | **Chat** — say in map | | ☐ | |
| 7 | **Party or friend PM** — invite/accept or guild chat if available | | ☐ | |
| 8 | **ENDPLAY** — return to char select | | ☐ | |
| 9 | **Enter alt char** (if available) | | ☐ | |
| 10 | **Logout** — account logout | | ☐ | |
| 11 | **Re-login** — fresh session slot/gen | | ☐ | |
| 12 | *(Optional)* **Group restart** — restart GroupServer only; verify TP_SYNC_PLYLST session restore or clean re-login | | ☐ | See R5.3 |

**Soak idle:** Leave one client in-world for the remainder of the 30 minutes (movement/chat occasionally).

**Sign-off:** When all steps pass and log grep (below) is clean for the soak window, mark R6.1 **done** in `AUDIT_REMEDIATION_PLAN.md` and Phase 3 exit gate **done** in `STATUS.md`.

---

## Log grep — must NOT appear during normal play

Run after the soak (or on today's log folder). Replace the date if needed.

### PowerShell (recommended)

```powershell
$logRoot = "C:\Users\Ken\Desktop\Github\devk\server\LOG"
$date = "2026-07-02"   # soak date

$patterns = @(
    "Invalid player trailer",
    "[OpcodeIngress] [reject]",
    "SessionManager REJECT",
    "PM broadcast reject",
    "EnterMap MP_ENTERMAP REJECT",
    "TM_ENTERMAP trailer too short",
    "Group REJECT",
    "Game REJECT"
)

Get-ChildItem -Path $logRoot -Recurse -Filter *.log |
    Where-Object { $_.FullName -match [regex]::Escape($date) } |
    ForEach-Object {
        foreach ($p in $patterns) {
            Select-String -Path $_.FullName -Pattern $p -SimpleMatch -ErrorAction SilentlyContinue
        }
    }
```

**Expect:** No output (or only pre-soak / attack traffic clearly outside the soak window).

### findstr (cmd)

```cmd
cd /d C:\Users\Ken\Desktop\Github\devk\server\LOG
findstr /s /i /c:"Invalid player trailer" /c:"OpcodeIngress" /c:"SessionManager REJECT" /c:"PM broadcast reject" /c:"MP_ENTERMAP REJECT" /c:"trailer too short" *\2026-07-02\*.log
```

### ripgrep (if installed)

```powershell
rg -i "Invalid player trailer|\[OpcodeIngress\] \[reject\]|SessionManager REJECT|PM broadcast reject|MP_ENTERMAP REJECT|TM_ENTERMAP trailer too short|Group REJECT|Game REJECT" server/LOG --glob "*2026-07-02*"
```

---

## Expected healthy session lines (sanity)

During login/enter/move you **should** see (examples):

- `SessionManager TP_USER_LOGIN bound session slot=… gen=…`
- `EnterMap TM_ENTERMAP session slot=… gen=…`
- `EnterMap MP_ENTERMAP session slot=… gen=…`
- `ReRoute Game cmd=6 session slot=…` (movement)
- `SessionManager CM session cmd=1` (chat) on Game

---

## Automated-only status (agent / CI)

If only `net-smoke.py` was run (no 30-min manual soak):

- Mark R6.1 as **pending user soak** (not fully done).
- Record automated T0 result in `STATUS.md` and `last-smoke-result.txt`.
