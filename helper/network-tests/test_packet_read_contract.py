#!/usr/bin/env python3
"""Unit-style checks for RPacket read contract (I4). No server required."""

from __future__ import annotations

import struct
import sys
from pathlib import Path

# Minimal simulation of length-prefixed string read logic (mirrors Packet.cpp contract)


def read_sequence(data: bytes, pos: int) -> tuple[int | None, bytes | None, int]:
    if pos + 2 > len(data):
        return pos, None, 0
    (slen,) = struct.unpack_from(">H", data, pos)
    pos += 2
    if slen == 0 or pos + slen > len(data):
        return pos, None, 0
    seq = data[pos : pos + slen]
    return pos + slen, seq, slen


def read_string(data: bytes, pos: int) -> tuple[int | None, str | None]:
    pos, seq, slen = read_sequence(data, pos)
    if seq is None or slen == 0:
        return None, None
    if seq[-1] != 0:
        return None, None
    return pos, seq[:-1].decode("utf-8", errors="replace")


def build_string(s: str) -> bytes:
    raw = s.encode("utf-8") + b"\x00"
    return struct.pack(">H", len(raw)) + raw


def test_valid_string() -> None:
    payload = build_string("hello")
    pos, val = read_string(payload, 0)
    assert val == "hello", val


def test_truncated_length() -> None:
    payload = b"\x00\x10"  # claims 16 bytes, none follow
    pos, val = read_string(payload, 0)
    assert val is None


def test_missing_nul() -> None:
    raw = b"hello"  # no trailing nul
    payload = struct.pack(">H", len(raw)) + raw
    pos, val = read_string(payload, 0)
    assert val is None


def main() -> int:
    test_valid_string()
    test_truncated_length()
    test_missing_nul()
    print("I4 packet read contract tests: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
