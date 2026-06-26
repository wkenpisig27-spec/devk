#!/usr/bin/env python3
"""
Minimal network smoke tests for Phase 0 baseline (T0 suite).

Does NOT perform full RSA login — that requires Botan/crypto and is covered
by manual client login or future integration tests.

Usage:
  python helper/network-tests/net-smoke.py
  python helper/network-tests/net-smoke.py --host 127.0.0.1 --port 1973

Requires: GateServer listening on the configured port (default 1973).
"""

from __future__ import annotations

import argparse
import socket
import struct
import sys
import time
from pathlib import Path

# Resolved from opcodes.csv / NetCommand.h
CMD_MC_RSA_HANDSHAKE_1 = 500 + 430 + 13  # 943
CMD_CP_PING = 6000 + 22  # 6022

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 1973
CONNECT_TIMEOUT = 5.0
READ_TIMEOUT = 10.0

# Gate uses RPCMGR: [u16 len BE][u32 sess BE][u16 cmd BE][payload...]
PKT_HEAD = 6  # 2 len + 4 sess
CMD_OFFSET = 6


def read_exact(sock: socket.socket, n: int) -> bytes:
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError(f"connection closed after {len(buf)}/{n} bytes")
        buf += chunk
    return buf


def recv_packet(sock: socket.socket) -> tuple[int, bytes]:
    """Return (cmd, full_packet_bytes)."""
    header = read_exact(sock, 2)
    (pkt_len,) = struct.unpack(">H", header)
    if pkt_len < 2:
        raise ValueError(f"invalid packet length {pkt_len}")
    rest = read_exact(sock, pkt_len - 2)
    data = header + rest
    if len(data) < PKT_HEAD + 2:
        raise ValueError(f"packet too short: {len(data)} bytes")
    (cmd,) = struct.unpack(">H", data[CMD_OFFSET : CMD_OFFSET + 2])
    return cmd, data


def build_packet(cmd: int, payload: bytes = b"", sess: int = 0) -> bytes:
    body = struct.pack(">I", sess) + struct.pack(">H", cmd) + payload
    return struct.pack(">H", len(body) + 2) + body


def test_t0_connect(host: str, port: int) -> tuple[bool, str]:
    """T0-connect: TCP connect + receive CMD_MC_RSA_HANDSHAKE_1."""
    try:
        with socket.create_connection((host, port), timeout=CONNECT_TIMEOUT) as sock:
            sock.settimeout(READ_TIMEOUT)
            cmd, raw = recv_packet(sock)
            if cmd != CMD_MC_RSA_HANDSHAKE_1:
                return False, f"expected cmd {CMD_MC_RSA_HANDSHAKE_1}, got {cmd}"
            if len(raw) < 32:
                return False, f"handshake packet suspiciously short ({len(raw)} bytes)"
            return True, f"OK cmd={cmd} pkt_len={len(raw)}"
    except OSError as e:
        return False, f"connect/read failed: {e}"
    except Exception as e:
        return False, str(e)


def test_t0_ping(host: str, port: int) -> tuple[bool, str]:
    """T0-ping: send CMD_CP_PING after connect; gate should accept without disconnect."""
    try:
        with socket.create_connection((host, port), timeout=CONNECT_TIMEOUT) as sock:
            sock.settimeout(READ_TIMEOUT)
            # Consume server-initiated RSA handshake
            cmd, _ = recv_packet(sock)
            if cmd != CMD_MC_RSA_HANDSHAKE_1:
                return False, f"handshake expected {CMD_MC_RSA_HANDSHAKE_1}, got {cmd}"

            ping = build_packet(CMD_CP_PING)
            sock.sendall(ping)
            time.sleep(0.3)
            # Connection should still be open (recv with MSG_PEEK or short timeout)
            sock.settimeout(0.5)
            try:
                sock.recv(1, socket.MSG_PEEK)
            except socket.timeout:
                pass  # no data yet is fine
            except OSError as e:
                return False, f"connection dropped after ping: {e}"
            return True, "OK ping sent, connection still open"
    except OSError as e:
        return False, f"connect/send failed: {e}"
    except Exception as e:
        return False, str(e)


def test_t0_login_manual() -> tuple[bool, str]:
    """T0-login placeholder — full login needs RSA+AES (client or Botan test)."""
    return False, "SKIP: run manual login via game client (LoginScene -> CS_Login)"


def test_t0_enter_manual() -> tuple[bool, str]:
    return False, "SKIP: run manual char select -> CMD_CM_BGNPLAY after login"


def main() -> int:
    parser = argparse.ArgumentParser(description="Phase 0 network smoke tests")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--skip-manual", action="store_true", default=True)
    args = parser.parse_args()

    tests = [
        ("T0-connect", lambda: test_t0_connect(args.host, args.port)),
        ("T0-ping", lambda: test_t0_ping(args.host, args.port)),
        ("T0-login", test_t0_login_manual),
        ("T0-enter", test_t0_enter_manual),
    ]

    print(f"Network smoke tests -> {args.host}:{args.port}")
    print("-" * 60)

    passed = failed = skipped = 0
    results: list[tuple[str, str, str]] = []

    for name, fn in tests:
        ok, msg = fn()
        if msg.startswith("SKIP"):
            status = "SKIP"
            skipped += 1
        elif ok:
            status = "PASS"
            passed += 1
        else:
            status = "FAIL"
            failed += 1
        results.append((name, status, msg))
        print(f"  [{status:4}] {name}: {msg}")

    print("-" * 60)
    print(f"Summary: {passed} passed, {failed} failed, {skipped} skipped")

    # Write machine-readable results for STATUS.md updates
    out = Path(__file__).resolve().parent / "last-smoke-result.txt"
    lines = [f"{time.strftime('%Y-%m-%d %H:%M:%S')}", f"target={args.host}:{args.port}"]
    for name, status, msg in results:
        lines.append(f"{name}\t{status}\t{msg}")
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Results written to {out}")

    # Phase 0 gate: automated tests must pass; manual tests are skipped not failed
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
