# Soft-AA rounded inventory slot skins for Notice UI (DX9 TGA).
# Flat face + tight border — no inner-bevel / lift that reads as blur.
# Usage: python tools/gen-inv-slots.py

from __future__ import annotations

import math
import struct
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "client" / "ui" / "rml" / "frames" / "notice"
SIZE = 44
RADIUS = 6.0
# Single-pixel stroke; soft borders look like an inner shadow once AA'd.
BORDER = 1.0
FEATHER = 0.35
SS = 5


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


def coverage(dist: float, feather: float = FEATHER) -> float:
    return max(0.0, min(1.0, 0.5 - dist / feather))


def sample_slot(
    px: float,
    py: float,
    w: float,
    h: float,
    radius: float,
    border: float,
    fill: tuple[int, int, int],
    border_rgb: tuple[int, int, int],
    outer_ring: tuple[int, int, int] | None = None,
    ring: float = 0.0,
) -> tuple[int, int, int, int]:
    ring_cov = 0.0
    if outer_ring and ring > 0.0:
        d_ring_outer = sd_round_box(px, py, w, h, radius + 0.15)
        d_ring_inner = sd_round_box(
            px - ring, py - ring, w - 2 * ring, h - 2 * ring, max(0.0, radius - ring * 0.55)
        )
        ring_cov = coverage(d_ring_outer) * (1.0 - coverage(d_ring_inner))

    inset = ring
    d_outer = sd_round_box(
        px - inset, py - inset, w - 2 * inset, h - 2 * inset, max(0.0, radius - inset * 0.35)
    )
    outer = coverage(d_outer)
    if outer <= 0.001 and ring_cov <= 0.001:
        return (0, 0, 0, 0)

    bi = border
    d_inner = sd_round_box(
        px - inset - bi,
        py - inset - bi,
        w - 2 * inset - 2 * bi,
        h - 2 * inset - 2 * bi,
        max(0.0, radius - inset * 0.35 - bi),
    )
    inner = coverage(d_inner)

    border_a = outer * (1.0 - inner)
    fill_a = outer * inner
    a = border_a + fill_a
    if a < 0.001 and ring_cov <= 0.001:
        return (0, 0, 0, 0)

    # Flat fill — no radial lift (that read as a soft inner shadow).
    r = g = b = 0.0
    if a > 0.001:
        r = (border_rgb[0] * border_a + fill[0] * fill_a) / a
        g = (border_rgb[1] * border_a + fill[1] * fill_a) / a
        b = (border_rgb[2] * border_a + fill[2] * fill_a) / a

    if ring_cov > 0.001 and outer_ring:
        out_a = a + ring_cov * (1.0 - a)
        if out_a > 0.001:
            if a < 0.001:
                r, g, b = float(outer_ring[0]), float(outer_ring[1]), float(outer_ring[2])
            else:
                t = ring_cov / (ring_cov + a)
                r = r * (1 - t) + outer_ring[0] * t
                g = g * (1 - t) + outer_ring[1] * t
                b = b * (1 - t) + outer_ring[2] * t
            a = out_a

    return (clamp(r), clamp(g), clamp(b), clamp(a * 255))


def render_slot(fill, border_rgb, outer_ring=None, ring=0.0):
    out: list[tuple[int, int, int, int]] = []
    # Inset so AA fringe isn't flush against the bitmap edge (clipping made
    # left/top corners look flatter than right/bottom against the panel).
    inset = 0.5
    w = h = float(SIZE) - inset * 2.0
    for y in range(SIZE):
        for x in range(SIZE):
            # Premultiplied SS — averaging straight RGB with (0,0,0,0) darkens
            # fringes and makes corners look unequal on light vs edge backgrounds.
            pr = pg = pb = pa = 0.0
            for sy in range(SS):
                for sx in range(SS):
                    px = (x + (sx + 0.5) / SS) - inset
                    py = (y + (sy + 0.5) / SS) - inset
                    r, g, b, a = sample_slot(px, py, w, h, RADIUS, BORDER, fill, border_rgb, outer_ring, ring)
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


def main() -> None:
    # Match Notice panel body so slots don't look recessed against the chrome.
    body = (245, 249, 255)
    write_tga(
        OUT / "slot_rounded.tga",
        SIZE,
        SIZE,
        render_slot(fill=body, border_rgb=(148, 178, 220)),
    )
    write_tga(
        OUT / "slot_rounded_sel.tga",
        SIZE,
        SIZE,
        render_slot(
            fill=(250, 252, 255),
            border_rgb=(110, 158, 220),
            outer_ring=(236, 172, 56),
            ring=2.0,
        ),
    )
    write_tga(
        OUT / "slot_rounded_locked.tga",
        SIZE,
        SIZE,
        render_slot(fill=(220, 228, 238), border_rgb=(152, 170, 196)),
    )


if __name__ == "__main__":
    main()
