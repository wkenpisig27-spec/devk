# Dark HUD hotbar slots + circular page buttons (DX9 TGA).
# Usage: python tools/gen-hotbar-skin.py

from __future__ import annotations

import math
import struct
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "client" / "ui" / "rml" / "frames" / "hud"
SLOT = 48
BTN = 22
RADIUS = 7.0
BORDER = 1.15
FEATHER = 0.35
SS = 5
FILL_OPACITY = 0.90


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


def sd_round_box(px: float, py: float, w: float, h: float, radius: float) -> float:
    r = max(0.0, min(radius, min(w, h) * 0.5))
    bx = w * 0.5 - r
    by = h * 0.5 - r
    dx = abs(px - w * 0.5) - bx
    dy = abs(py - h * 0.5) - by
    ox = max(dx, 0.0)
    oy = max(dy, 0.0)
    return math.hypot(ox, oy) + min(max(dx, dy), 0.0) - r


def sd_circle(px: float, py: float, cx: float, cy: float, radius: float) -> float:
    return math.hypot(px - cx, py - cy) - radius


def dist_seg(px: float, py: float, ax: float, ay: float, bx: float, by: float) -> float:
    vx, vy = bx - ax, by - ay
    wx, wy = px - ax, py - ay
    denom = vx * vx + vy * vy
    t = 0.0 if denom < 1e-6 else max(0.0, min(1.0, (wx * vx + wy * vy) / denom))
    return math.hypot(px - (ax + t * vx), py - (ay + t * vy))


def coverage(dist: float, feather: float = FEATHER) -> float:
    return max(0.0, min(1.0, 0.5 - dist / feather))


def premul_ss(w: int, h: int, sample) -> list[tuple[int, int, int, int]]:
    out: list[tuple[int, int, int, int]] = []
    for y in range(h):
        for x in range(w):
            pr = pg = pb = pa = 0.0
            for sy in range(SS):
                for sx in range(SS):
                    px = x + (sx + 0.5) / SS
                    py = y + (sy + 0.5) / SS
                    r, g, b, a = sample(px, py)
                    af = a / 255.0
                    pr += r * af
                    pg += g * af
                    pb += b * af
                    pa += af
            n = float(SS * SS)
            pa /= n
            if pa < 0.004:
                out.append((0, 0, 0, 0))
            else:
                out.append((
                    clamp(pr / n / pa),
                    clamp(pg / n / pa),
                    clamp(pb / n / pa),
                    clamp(pa * 255),
                ))
    return out


def sample_slot(px: float, py: float) -> tuple[int, int, int, int]:
    inset = 0.6
    w = h = float(SLOT) - inset * 2.0
    px -= inset
    py -= inset
    fill = (36, 30, 24)
    border_rgb = (92, 78, 62)
    d_outer = sd_round_box(px, py, w, h, RADIUS)
    outer = coverage(d_outer)
    if outer <= 0.001:
        return (0, 0, 0, 0)
    d_inner = sd_round_box(px - BORDER, py - BORDER, w - 2 * BORDER, h - 2 * BORDER, max(0.0, RADIUS - BORDER))
    inner = coverage(d_inner)
    border_a = outer * (1.0 - inner)
    fill_a = outer * inner * FILL_OPACITY
    a = border_a + fill_a
    if a < 0.001:
        return (0, 0, 0, 0)
    r = (border_rgb[0] * border_a + fill[0] * fill_a) / a
    g = (border_rgb[1] * border_a + fill[1] * fill_a) / a
    b = (border_rgb[2] * border_a + fill[2] * fill_a) / a
    return (clamp(r), clamp(g), clamp(b), clamp(a * 255))


def sample_page_btn(px: float, py: float, up: bool) -> tuple[int, int, int, int]:
    cx = cy = BTN * 0.5
    radius = BTN * 0.5 - 0.7
    fill = (28, 24, 20)
    border_rgb = (86, 74, 58)
    chevron = (244, 240, 232)
    d_outer = sd_circle(px, py, cx, cy, radius)
    outer = coverage(d_outer)
    if outer <= 0.001:
        return (0, 0, 0, 0)
    d_inner = sd_circle(px, py, cx, cy, radius - 1.15)
    inner = coverage(d_inner)
    border_a = outer * (1.0 - inner)
    fill_a = outer * inner * 0.94
    a = border_a + fill_a
    r = (border_rgb[0] * border_a + fill[0] * fill_a) / a if a > 0.001 else 0.0
    g = (border_rgb[1] * border_a + fill[1] * fill_a) / a if a > 0.001 else 0.0
    b = (border_rgb[2] * border_a + fill[2] * fill_a) / a if a > 0.001 else 0.0

    # Two-stroke chevron.
    if up:
        ax, ay = cx - 5.2, cy + 1.6
        bx, by = cx, cy - 3.4
        cx2, cy2 = cx + 5.2, cy + 1.6
    else:
        ax, ay = cx - 5.2, cy - 1.6
        bx, by = cx, cy + 3.4
        cx2, cy2 = cx + 5.2, cy - 1.6
    d_ch = min(dist_seg(px, py, ax, ay, bx, by), dist_seg(px, py, cx2, cy2, bx, by))
    ch = coverage(d_ch - 0.85)
    if ch > 0.001:
        t = ch
        r = r * (1.0 - t) + chevron[0] * t
        g = g * (1.0 - t) + chevron[1] * t
        b = b * (1.0 - t) + chevron[2] * t
        a = a + ch * (1.0 - a)
    return (clamp(r), clamp(g), clamp(b), clamp(a * 255))


def main() -> None:
    write_tga(OUT / "slot_dark.tga", SLOT, SLOT, premul_ss(SLOT, SLOT, sample_slot))
    write_tga(OUT / "btn_page_up.tga", BTN, BTN, premul_ss(BTN, BTN, lambda x, y: sample_page_btn(x, y, True)))
    write_tga(OUT / "btn_page_down.tga", BTN, BTN, premul_ss(BTN, BTN, lambda x, y: sample_page_btn(x, y, False)))


if __name__ == "__main__":
    main()
