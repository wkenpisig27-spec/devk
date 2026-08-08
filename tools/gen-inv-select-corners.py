# Soft-AA gold corner brackets for Notice inventory multi-select (DX9 TGA).
# Usage: python tools/gen-inv-select-corners.py

from __future__ import annotations

import math
import struct
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "client" / "ui" / "rml" / "frames" / "notice"
SIZE = 14
THICK = 2.4
FEATHER = 0.35
# Warm gold matching the reference corner brackets
RGB = (240, 168, 72)


def clamp(v: float) -> int:
    return int(0 if v < 0 else 255 if v > 255 else round(v))


def write_tga(path: Path, w: int, h: int, rgba: list[tuple[int, int, int, int]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, w, h, 32, 0x20)
    raw = bytearray()
    for r, g, b, a in rgba:
        raw += bytes((b, g, r, a))
    path.write_bytes(header + raw)
    print(f"wrote {path.name} ({w}x{h})")


def coverage(dist: float) -> float:
    return max(0.0, min(1.0, 0.5 - dist / FEATHER))


def sd_box(px: float, py: float, x0: float, y0: float, x1: float, y1: float) -> float:
    cx = (x0 + x1) * 0.5
    cy = (y0 + y1) * 0.5
    hx = (x1 - x0) * 0.5
    hy = (y1 - y0) * 0.5
    dx = abs(px - cx) - hx
    dy = abs(py - cy) - hy
    ox = max(dx, 0.0)
    oy = max(dy, 0.0)
    return math.hypot(ox, oy) + min(max(dx, dy), 0.0)


def make_corner(kind: str) -> None:
    # L-bracket in top-left orientation, then flip for other corners.
    pixels: list[tuple[int, int, int, int]] = []
    arm = SIZE - 2.0
    for y in range(SIZE):
        for x in range(SIZE):
            px = x + 0.5
            py = y + 0.5
            # Horizontal bar + vertical bar of the L
            d_h = sd_box(px, py, 1.0, 1.0, 1.0 + arm, 1.0 + THICK)
            d_v = sd_box(px, py, 1.0, 1.0, 1.0 + THICK, 1.0 + arm)
            d = min(d_h, d_v)
            a = coverage(d)
            pixels.append((RGB[0], RGB[1], RGB[2], clamp(a * 255)))

    # Flip to target corner
    def idx(xx: int, yy: int) -> int:
        return yy * SIZE + xx

    out = [(0, 0, 0, 0)] * (SIZE * SIZE)
    for y in range(SIZE):
        for x in range(SIZE):
            sx, sy = x, y
            if kind in ("tr", "br"):
                sx = SIZE - 1 - x
            if kind in ("bl", "br"):
                sy = SIZE - 1 - y
            out[idx(x, y)] = pixels[idx(sx, sy)]

    write_tga(OUT / f"sel_corner_{kind}.tga", SIZE, SIZE, out)


def main() -> None:
    for kind in ("tl", "tr", "bl", "br"):
        make_corner(kind)


if __name__ == "__main__":
    main()
