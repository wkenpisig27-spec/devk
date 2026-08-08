# Soft-AA equipment slot placeholder silhouettes for Notice inventory (DX9 TGA).
# Usage: python tools/gen-inv-placeholders.py

from __future__ import annotations

import math
import struct
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "client" / "ui" / "rml" / "frames" / "notice" / "slots"
SIZE = 40
FEATHER = 0.4
# Muted Notice blue-violet (matches legacy "ghost" readability on light panels)
RGB = (150, 170, 210)
ALPHA = 0.42


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


def shape_helm(px: float, py: float) -> float:
    d = sd_round_box(px, py, 20, 18, 11, 10, 4)
    d = op_union(d, sd_box(px, py, 20, 28, 12, 3))
    d = op_union(d, sd_segment(px, py, 10, 14, 30, 14, 1.2))
    return d


def shape_face(px: float, py: float) -> float:
    d = sd_circle(px, py, 20, 18, 10)
    d = op_union(d, sd_round_box(px, py, 20, 30, 7, 4, 2))
    return d


def shape_body(px: float, py: float) -> float:
    d = sd_round_box(px, py, 20, 22, 10, 12, 3)
    d = op_union(d, sd_segment(px, py, 10, 12, 30, 12, 2.0))
    return d


def shape_glove(px: float, py: float) -> float:
    d = sd_round_box(px, py, 18, 20, 7, 9, 3)
    d = op_union(d, sd_circle(px, py, 28, 14, 3.5))
    d = op_union(d, sd_circle(px, py, 29, 20, 3.2))
    d = op_union(d, sd_circle(px, py, 28, 26, 3.0))
    return d


def shape_shoes(px: float, py: float) -> float:
    d = sd_round_box(px, py, 18, 22, 8, 6, 3)
    d = op_union(d, sd_round_box(px, py, 26, 26, 7, 4, 2))
    return d


def shape_neck(px: float, py: float) -> float:
    d = sd_circle(px, py, 20, 12, 7)
    # ring hole
    hole = sd_circle(px, py, 20, 12, 4.5)
    d = max(d, -hole)
    d = op_union(d, sd_circle(px, py, 20, 26, 4))
    d = op_union(d, sd_segment(px, py, 20, 18, 20, 23, 1.2))
    return d


def shape_sword(px: float, py: float) -> float:
    d = sd_segment(px, py, 12, 28, 28, 12, 2.2)
    d = op_union(d, sd_segment(px, py, 14, 18, 22, 26, 1.6))
    d = op_union(d, sd_circle(px, py, 11, 29, 2.2))
    return d


def shape_shield(px: float, py: float) -> float:
    d = sd_round_box(px, py, 20, 18, 10, 11, 4)
    d = op_union(d, sd_segment(px, py, 20, 10, 20, 28, 1.3))
    d = op_union(d, sd_segment(px, py, 12, 18, 28, 18, 1.3))
    return d


def shape_ring(px: float, py: float) -> float:
    outer = sd_circle(px, py, 20, 20, 9)
    inner = sd_circle(px, py, 20, 20, 5.5)
    d = max(outer, -inner)
    d = op_union(d, sd_circle(px, py, 20, 10, 2.8))
    return d


def shape_wing(px: float, py: float) -> float:
    d = sd_segment(px, py, 20, 28, 10, 10, 2.0)
    d = op_union(d, sd_segment(px, py, 20, 28, 30, 10, 2.0))
    d = op_union(d, sd_segment(px, py, 12, 16, 20, 22, 1.4))
    d = op_union(d, sd_segment(px, py, 28, 16, 20, 22, 1.4))
    return d


def shape_cloak(px: float, py: float) -> float:
    d = sd_segment(px, py, 12, 10, 28, 10, 2.0)
    d = op_union(d, sd_segment(px, py, 12, 10, 16, 30, 2.2))
    d = op_union(d, sd_segment(px, py, 28, 10, 24, 30, 2.2))
    d = op_union(d, sd_segment(px, py, 16, 30, 24, 30, 1.8))
    return d


def shape_fairy(px: float, py: float) -> float:
    d = sd_circle(px, py, 20, 14, 5)
    d = op_union(d, sd_round_box(px, py, 20, 24, 5, 6, 2))
    d = op_union(d, sd_circle(px, py, 12, 18, 3))
    d = op_union(d, sd_circle(px, py, 28, 18, 3))
    return d


def shape_rear(px: float, py: float) -> float:
    d = sd_circle(px, py, 20, 18, 8)
    d = op_union(d, sd_segment(px, py, 20, 26, 20, 32, 2.0))
    d = op_union(d, sd_segment(px, py, 20, 32, 14, 36, 1.5))
    d = op_union(d, sd_segment(px, py, 20, 32, 26, 36, 1.5))
    return d


def shape_mount(px: float, py: float) -> float:
    d = sd_round_box(px, py, 20, 22, 11, 7, 3)
    d = op_union(d, sd_circle(px, py, 28, 14, 5))
    d = op_union(d, sd_circle(px, py, 12, 28, 2.5))
    d = op_union(d, sd_circle(px, py, 28, 28, 2.5))
    return d


def shape_glow(px: float, py: float) -> float:
    d = 1e9
    for ang in range(0, 360, 45):
        rad = math.radians(ang)
        d = op_union(d, sd_segment(px, py, 20, 20, 20 + math.cos(rad) * 12, 20 + math.sin(rad) * 12, 1.3))
    d = op_union(d, sd_circle(px, py, 20, 20, 3.5))
    return d


def shape_dagger(px: float, py: float) -> float:
    d = sd_segment(px, py, 14, 26, 26, 12, 1.8)
    d = op_union(d, sd_segment(px, py, 16, 18, 22, 24, 1.4))
    return d


def shape_gun(px: float, py: float) -> float:
    d = sd_round_box(px, py, 22, 18, 10, 3, 1.5)
    d = op_union(d, sd_round_box(px, py, 14, 24, 3, 6, 1.2))
    return d


def shape_greatsword(px: float, py: float) -> float:
    d = sd_segment(px, py, 10, 30, 30, 10, 3.0)
    d = op_union(d, sd_segment(px, py, 12, 18, 24, 30, 2.0))
    return d


def shape_staff(px: float, py: float) -> float:
    d = sd_segment(px, py, 14, 30, 26, 10, 1.8)
    d = op_union(d, sd_circle(px, py, 27, 9, 3.5))
    return d


def shape_bow(px: float, py: float) -> float:
    # Approximate arc with segments
    d = sd_segment(px, py, 12, 10, 12, 30, 1.5)
    d = op_union(d, sd_segment(px, py, 12, 10, 26, 20, 1.5))
    d = op_union(d, sd_segment(px, py, 12, 30, 26, 20, 1.5))
    d = op_union(d, sd_segment(px, py, 14, 20, 30, 20, 1.2))
    return d


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    shapes = {
        "helm": shape_helm,
        "face": shape_face,
        "body": shape_body,
        "glove": shape_glove,
        "shoes": shape_shoes,
        "neck": shape_neck,
        "sword": shape_sword,
        "shield": shape_shield,
        "ring": shape_ring,
        "wing": shape_wing,
        "cloak": shape_cloak,
        "fairy": shape_fairy,
        "rear": shape_rear,
        "mount": shape_mount,
        "glow": shape_glow,
        "dagger": shape_dagger,
        "gun": shape_gun,
        "greatsword": shape_greatsword,
        "staff": shape_staff,
        "bow": shape_bow,
    }
    for name, fn in shapes.items():
        paint(name, fn)
    print("done ->", OUT)


if __name__ == "__main__":
    main()
