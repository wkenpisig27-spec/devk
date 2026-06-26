#!/usr/bin/env python3
"""
Generate machine-readable opcode inventory from source/include/common/NetCommand.h.

Usage (from repo root):
  python helper/network-tests/scripts/generate_opcode_table.py

Output:
  helper/network-tests/data/opcodes.csv
"""

from __future__ import annotations

import csv
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
NET_COMMAND_H = REPO_ROOT / "source" / "include" / "common" / "NetCommand.h"
OUTPUT_CSV = REPO_ROOT / "helper" / "network-tests" / "data" / "opcodes.csv"

DEFINE_RE = re.compile(
    r"^#define\s+(CMD_[A-Z0-9_]+)\s+(.+?)(?:\s*//.*)?$"
)

DIRECTION_BY_PREFIX = {
    "CM": "Client->GameServer",
    "MC": "GameServer->Client",
    "TM": "GateServer->GameServer",
    "MT": "GameServer->GateServer",
    "TP": "GateServer->GroupServer",
    "PT": "GroupServer->GateServer",
    "PA": "GroupServer->AccountServer",
    "AP": "AccountServer->GroupServer",
    "MM": "GameServer->GameServer",
    "PM": "GroupServer->GameServer",
    "PC": "GroupServer->Client",
    "MP": "GameServer->GroupServer",
    "CP": "Client->GroupServer",
    "OS": "Monitor->Server",
    "SO": "Server->Monitor",
    "TC": "GateServer->Client",
}


def direction_for(name: str) -> str:
    if name == "CMD_INVALID":
        return "Invalid"
    if name.endswith("_BASE") or name.endswith("ROLEBASE") or name.endswith("GULDBASE") or name.endswith("CHARBASE"):
        parts = name.split("_")
        if len(parts) >= 3:
            code = parts[1]
            if code in DIRECTION_BY_PREFIX:
                return DIRECTION_BY_PREFIX[code] + " (base)"
    parts = name.split("_")
    if len(parts) >= 2 and parts[0] == "CMD":
        code = parts[1]
        if code in DIRECTION_BY_PREFIX:
            return DIRECTION_BY_PREFIX[code]
    return "Unknown"


def band_for(value: int) -> str:
    bands = [
        (0, 500, "CM"),
        (500, 1000, "MC"),
        (1000, 1500, "TM"),
        (1500, 2000, "MT"),
        (2000, 2500, "TP"),
        (2500, 3000, "PT"),
        (3000, 3500, "PA"),
        (3500, 4000, "AP"),
        (4000, 4500, "MM"),
        (4500, 5000, "PM"),
        (5000, 5500, "PC"),
        (5500, 6000, "MP"),
        (6000, 6500, "CP"),
        (6500, 7000, "OS"),
        (7000, 7500, "SO"),
        (7500, 8000, "TC"),
    ]
    for lo, hi, label in bands:
        if lo <= value < hi:
            return label
    return "out-of-band"


def parse_defines(text: str) -> dict[str, str]:
    raw: dict[str, str] = {}
    for line in text.splitlines():
        line = line.strip()
        if not line.startswith("#define CMD_"):
            continue
        m = DEFINE_RE.match(line)
        if not m:
            continue
        raw[m.group(1)] = m.group(2).strip()
    return raw


def resolve(name: str, raw: dict[str, str], cache: dict[str, int], stack: set[str]) -> int | None:
    if name in cache:
        return cache[name]
    if name not in raw:
        return None
    if name in stack:
        return None
    expr = raw[name]
    stack.add(name)
    try:
        # Replace CMD_* tokens with resolved integers.
        tokens = re.findall(r"CMD_[A-Z0-9_]+|\d+", expr)
        if not tokens:
            return None
        resolved_parts: list[str] = []
        i = 0
        while i < len(tokens):
            tok = tokens[i]
            if tok.startswith("CMD_"):
                val = resolve(tok, raw, cache, stack)
                if val is None:
                    return None
                resolved_parts.append(str(val))
            else:
                resolved_parts.append(tok)
            i += 1
        # Rebuild expression with operators between tokens as in original.
        # Most defs are "A + N" or plain "N".
        if "+" in expr:
            parts = [p.strip() for p in expr.split("+")]
            total = 0
            for part in parts:
                part = part.strip()
                if part.startswith("CMD_"):
                    v = resolve(part, raw, cache, stack)
                    if v is None:
                        return None
                    total += v
                else:
                    total += int(part, 0)
            cache[name] = total
            return total
        value = int(expr, 0)
        cache[name] = value
        return value
    finally:
        stack.discard(name)


def main() -> int:
    if not NET_COMMAND_H.is_file():
        print(f"ERROR: missing {NET_COMMAND_H}", file=sys.stderr)
        return 1

    text = NET_COMMAND_H.read_text(encoding="utf-8", errors="replace")
    raw = parse_defines(text)
    cache: dict[str, int] = {}
    rows: list[dict[str, str | int]] = []

    for name in sorted(raw.keys()):
        value = resolve(name, raw, cache, set())
        if value is None:
            continue
        rows.append(
            {
                "name": name,
                "value": value,
                "value_hex": f"0x{value:04X}",
                "direction": direction_for(name),
                "band": band_for(value),
                "is_base": "yes" if name.endswith("_BASE") or "BASE" in name and name.count("_") <= 3 else "no",
            }
        )

    rows.sort(key=lambda r: (int(r["value"]), str(r["name"])))

    OUTPUT_CSV.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT_CSV.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=["name", "value", "value_hex", "direction", "band", "is_base"],
        )
        writer.writeheader()
        writer.writerows(rows)

    non_base = [r for r in rows if r["is_base"] == "no" and not str(r["name"]).endswith("ROLEBASE")
                and not str(r["name"]).endswith("GULDBASE") and not str(r["name"]).endswith("CHARBASE")]
    print(f"Wrote {len(rows)} opcodes to {OUTPUT_CSV}")
    print(f"  Total defines: {len(rows)}")
    print(f"  Non-base opcodes: {len(non_base)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
