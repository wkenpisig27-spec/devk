# Soft-AA white icons for inventory footer (Lock / Expand / Temp) — DX9 TGA.
# Usage: python tools/gen-inv-footer-icons.py

from __future__ import annotations

import math
import struct
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "client" / "ui" / "rml" / "frames" / "notice"
SIZE = 24
FEATHER = 0.85
RGB = (255, 255, 255)
ALPHA = 0.95


def clamp(v: float) -> int:
    return int(0 if v < 0 else 255 if v > 255 else round(v))


def write_tga(path: Path, w: int, h: int, rgba: list[tuple[int, int, int, int]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, w, h, 32, 0x20)
    raw = bytearray()
    for r, g, b, a in rgba:
        raw += bytes((b, g, r, a))
    path.write_bytes(header + raw)
    print(f"wrote {path.name}")


def coverage(dist: float, feather: float = FEATHER) -> float:
    return max(0.0, min(1.0, 0.5 - dist / feather))


def sd_circle(px: float, py: float, cx: float, cy: float, r: float) -> float:
    return math.hypot(px - cx, py - cy) - r


def sd_box(px: float, py: float, cx: float, cy: float, hw: float, hh: float) -> float:
    dx = abs(px - cx) - hw
    dy = abs(py - cy) - hh
    ox, oy = max(dx, 0.0), max(dy, 0.0)
    return math.hypot(ox, oy) + min(max(dx, dy), 0.0)


def sd_round_box(px: float, py: float, cx: float, cy: float, hw: float, hh: float, r: float) -> float:
    return sd_box(px, py, cx, cy, hw - r, hh - r) - r


def sd_segment(px: float, py: float, ax: float, ay: float, bx: float, by: float, r: float) -> float:
    dx, dy = bx - ax, by - ay
    len2 = dx * dx + dy * dy
    if len2 < 1e-6:
        return math.hypot(px - ax, py - ay) - r
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / len2))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy)) - r


def op_union(a: float, b: float) -> float:
    return min(a, b)


def op_sub(a: float, b: float) -> float:
    return max(a, -b)


def paint(name: str, sdf) -> None:
    pixels: list[tuple[int, int, int, int]] = []
    for y in range(SIZE):
        for x in range(SIZE):
            d = sdf(x + 0.5, y + 0.5)
            a = coverage(d) * ALPHA
            if a < 0.004:
                pixels.append((0, 0, 0, 0))
            else:
                pixels.append((RGB[0], RGB[1], RGB[2], clamp(a * 255)))
    write_tga(OUT / f"{name}.tga", SIZE, SIZE, pixels)


def shape_lock(px: float, py: float) -> float:
    # Shackle
    outer = sd_circle(px, py, 12, 8.5, 5.2)
    inner = sd_circle(px, py, 12, 8.5, 3.2)
    shackle = op_sub(outer, inner)
    shackle = op_sub(shackle, sd_box(px, py, 12, 12.5, 6.5, 4.5))  # open bottom of ring
    # Body
    body = sd_round_box(px, py, 12, 16.5, 6.2, 5.0, 1.6)
    # Keyhole
    key = op_union(sd_circle(px, py, 12, 15.2, 1.35), sd_box(px, py, 12, 17.6, 0.85, 1.8))
    return op_sub(op_union(shackle, body), key)


def shape_unlock(px: float, py: float) -> float:
    # Open shackle shifted left
    outer = sd_circle(px, py, 9.5, 7.5, 5.0)
    inner = sd_circle(px, py, 9.5, 7.5, 3.1)
    shackle = op_sub(outer, inner)
    shackle = op_sub(shackle, sd_box(px, py, 12.5, 10.5, 7.0, 5.0))
    body = sd_round_box(px, py, 13.5, 16.5, 6.2, 5.0, 1.6)
    key = op_union(sd_circle(px, py, 13.5, 15.2, 1.35), sd_box(px, py, 13.5, 17.6, 0.85, 1.8))
    return op_sub(op_union(shackle, body), key)


def shape_expand(px: float, py: float) -> float:
    # Bag / pack outline + plus
    bag = sd_round_box(px, py, 12, 14.5, 7.5, 6.2, 2.0)
    bag = op_sub(bag, sd_round_box(px, py, 12, 14.5, 5.2, 4.0, 1.2))
    strap = sd_segment(px, py, 7.5, 9.0, 16.5, 9.0, 1.15)
    strap = op_union(strap, sd_segment(px, py, 7.5, 9.0, 7.5, 11.2, 1.1))
    strap = op_union(strap, sd_segment(px, py, 16.5, 9.0, 16.5, 11.2, 1.1))
    plus = op_union(sd_box(px, py, 12, 14.5, 3.4, 1.05), sd_box(px, py, 12, 14.5, 1.05, 3.4))
    return op_union(op_union(bag, strap), plus)


def shape_temp(px: float, py: float) -> float:
    # Clock / hourglass-lite: circle + hands + small bag notch
    ring = op_sub(sd_circle(px, py, 12, 12, 8.2), sd_circle(px, py, 12, 12, 6.3))
    hand = op_union(sd_segment(px, py, 12, 12, 12, 7.2, 1.05), sd_segment(px, py, 12, 12, 16.2, 13.5, 1.05))
    hub = sd_circle(px, py, 12, 12, 1.35)
    return op_union(op_union(ring, hand), hub)


def main() -> None:
    paint("ico_lock", shape_lock)
    paint("ico_unlock", shape_unlock)
    paint("ico_expand", shape_expand)
    paint("ico_temp", shape_temp)


if __name__ == "__main__":
    main()
