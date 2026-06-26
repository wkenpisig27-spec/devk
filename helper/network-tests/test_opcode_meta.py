#!/usr/bin/env python3
"""Verify OpcodeMetaTable.inc matches opcodes.csv (M1). No build required."""

from __future__ import annotations

import csv
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
CSV_PATH = REPO / "helper" / "network-tests" / "data" / "opcodes.csv"
INC_PATH = REPO / "source" / "include" / "common" / "OpcodeMetaTable.inc"

ROW_RE = re.compile(
    r'^\s*\{\s*(\d+),\s*OpcodeBand::(\w+),\s*(true|false),\s*"(CMD_[A-Z0-9_]+)"\s*\},?\s*$'
)


def load_csv() -> list[dict[str, str]]:
    with CSV_PATH.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def load_inc() -> list[tuple[int, str, bool, str]]:
    rows: list[tuple[int, str, bool, str]] = []
    for line in INC_PATH.read_text(encoding="utf-8").splitlines():
        m = ROW_RE.match(line)
        if not m:
            continue
        value, band, is_base, name = m.groups()
        rows.append((int(value), band, is_base == "true", name))
    return rows


def main() -> int:
    if not CSV_PATH.is_file():
        print(f"ERROR: missing {CSV_PATH}", file=sys.stderr)
        return 1
    if not INC_PATH.is_file():
        print(f"ERROR: missing {INC_PATH} — run generate_opcode_table.py", file=sys.stderr)
        return 1

    csv_rows = load_csv()
    inc_rows = load_inc()

    assert len(csv_rows) == len(inc_rows), (
        f"row count mismatch csv={len(csv_rows)} inc={len(inc_rows)}"
    )

    prev_value = -1
    for csv_row, (value, band, is_base, name) in zip(csv_rows, inc_rows):
        assert int(csv_row["value"]) == value
        assert csv_row["name"] == name
        assert csv_row["band"] == band or (csv_row["band"] == "out-of-band" and band == "Unknown")
        csv_base = csv_row["is_base"] == "yes"
        assert csv_base == is_base, f"{name} is_base mismatch"
        assert value >= prev_value, f"table not sorted at {name}"
        prev_value = value

    ping = next((r for r in inc_rows if r[3] == "CMD_CP_PING"), None)
    assert ping is not None, "CMD_CP_PING missing"
    assert ping[0] == 6022, f"CMD_CP_PING value expected 6022 got {ping[0]}"

    print(f"M1 opcode meta tests: PASS ({len(inc_rows)} opcodes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
