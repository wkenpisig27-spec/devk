# Soft-AA white category icons + blue rounded-square tab face for inventory header.
# Usage: python tools/gen-inv-tab-icons.py

from __future__ import annotations

import math
import struct
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "client" / "ui" / "rml" / "frames" / "notice"
ICON = 22
FACE = 40
FEATHER = 0.35
EDGE = 0.75


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


def paint_icon(name: str, sdf) -> None:
    pixels: list[tuple[int, int, int, int]] = []
    for y in range(ICON):
        for x in range(ICON):
            d = sdf(x + 0.5, y + 0.5)
            a = coverage(d) * 0.95
            if a < 0.004:
                pixels.append((0, 0, 0, 0))
            else:
                pixels.append((255, 255, 255, clamp(a * 255)))
    write_tga(OUT / f"{name}.tga", ICON, ICON, pixels)


def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def paint_tab_face() -> None:
    """Blue rounded-square face (tintable via image-color)."""
    top = (140, 190, 245)
    bot = (74, 138, 208)
    bhi = (175, 210, 250)
    blo = (55, 115, 188)
    radius = 10.0
    border = 1.15
    pixels: list[tuple[int, int, int, int]] = []
    for y in range(FACE):
        ty = y / max(1, FACE - 1)
        fr = lerp(top[0], bot[0], ty)
        fg = lerp(top[1], bot[1], ty)
        fb = lerp(top[2], bot[2], ty)
        for x in range(FACE):
            px, py = x + 0.5, y + 0.5
            d_outer = sd_round_box_xy(px, py, EDGE, EDGE, FACE - EDGE, FACE - EDGE, radius)
            outer_a = coverage(d_outer)
            if outer_a <= 0.001:
                pixels.append((0, 0, 0, 0))
                continue
            d_inner = sd_round_box_xy(
                px, py, EDGE + border, EDGE + border, FACE - EDGE - border, FACE - EDGE - border, max(0.0, radius - border)
            )
            inner_a = coverage(d_inner)
            border_a = outer_a * (1.0 - inner_a)
            fill_a = outer_a * inner_a
            br = lerp(bhi[0], blo[0], ty)
            bg = lerp(bhi[1], blo[1], ty)
            bb = lerp(bhi[2], blo[2], ty)
            r = fr * fill_a + br * border_a
            g = fg * fill_a + bg * border_a
            b = fb * fill_a + bb * border_a
            a = max(fill_a, border_a)
            pixels.append((clamp(r), clamp(g), clamp(b), clamp(a * 255)))
    write_tga(OUT / "tab_square.tga", FACE, FACE, pixels)


def sd_round_box_xy(px: float, py: float, x0: float, y0: float, x1: float, y1: float, radius: float) -> float:
    w = x1 - x0
    h = y1 - y0
    cx = x0 + w * 0.5
    cy = y0 + h * 0.5
    return sd_round_box(px, py, cx, cy, w * 0.5, h * 0.5, radius)


# Icons centered in 22x22
C = ICON * 0.5


def shape_items(px: float, py: float) -> float:
    # 2x2 grid of small rounded cells
    s = 3.2
    g = 1.4
    cells = [
        sd_round_box(px, py, C - s - g * 0.5, C - s - g * 0.5, s, s, 1.1),
        sd_round_box(px, py, C + s + g * 0.5, C - s - g * 0.5, s, s, 1.1),
        sd_round_box(px, py, C - s - g * 0.5, C + s + g * 0.5, s, s, 1.1),
        sd_round_box(px, py, C + s + g * 0.5, C + s + g * 0.5, s, s, 1.1),
    ]
    d = cells[0]
    for c in cells[1:]:
        d = op_union(d, c)
    return d


def shape_equip(px: float, py: float) -> float:
    # Shield (clearer at 22px than a thin sword)
    body = sd_round_box(px, py, C, C - 0.5, 6.2, 6.8, 2.4)
    point = sd_segment(px, py, C - 5.8, C + 4.0, C, C + 9.8, 1.55)
    point = op_union(point, sd_segment(px, py, C + 5.8, C + 4.0, C, C + 9.8, 1.55))
    boss = sd_circle(px, py, C, C - 1.0, 1.8)
    return op_union(op_union(body, point), boss)


def shape_consumable(px: float, py: float) -> float:
    # Potion flask
    body = sd_round_box(px, py, C, 13.5, 5.2, 5.5, 2.2)
    neck = sd_round_box(px, py, C, 7.2, 2.0, 2.4, 0.8)
    lip = sd_round_box(px, py, C, 5.0, 3.2, 1.1, 0.7)
    bubble = sd_circle(px, py, C - 1.5, 13.0, 1.3)
    return op_union(op_union(op_union(body, neck), lip), bubble)


def shape_material(px: float, py: float) -> float:
    # Crystal shard (outline diamond + core)
    top_l = sd_segment(px, py, C, 4.2, C - 6.0, 11.0, 1.5)
    top_r = sd_segment(px, py, C, 4.2, C + 6.0, 11.0, 1.5)
    bot_l = sd_segment(px, py, C - 6.0, 11.0, C, 18.8, 1.5)
    bot_r = sd_segment(px, py, C + 6.0, 11.0, C, 18.8, 1.5)
    mid = sd_segment(px, py, C - 6.0, 11.0, C + 6.0, 11.0, 1.35)
    core = sd_circle(px, py, C, 11.0, 2.4)
    return op_union(op_union(op_union(op_union(op_union(top_l, top_r), bot_l), bot_r), mid), core)


def shape_other(px: float, py: float) -> float:
    # Three dots
    return op_union(
        op_union(sd_circle(px, py, C - 5.5, C, 2.1), sd_circle(px, py, C, C, 2.1)),
        sd_circle(px, py, C + 5.5, C, 2.1),
    )


def main() -> None:
    paint_tab_face()
    paint_icon("ico_tab_items", shape_items)
    paint_icon("ico_tab_equip", shape_equip)
    paint_icon("ico_tab_consumable", shape_consumable)
    paint_icon("ico_tab_material", shape_material)
    paint_icon("ico_tab_other", shape_other)


if __name__ == "__main__":
    main()
