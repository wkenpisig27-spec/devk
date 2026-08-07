# Soft anti-aliased Notice UI skin for DX9 RmlUi.
# Panel top tiles include the blue header band so corners never punch through.
# Usage: python tools/gen-notice-skin.py

from __future__ import annotations

import math
import struct
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "client" / "ui" / "rml" / "frames" / "notice"
SS = 5
# ~1px coverage ramp — enough to kill DX9 stair-steps, not a blur halo
FEATHER = 0.85


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


def mix_rgb(a: tuple[int, int, int], b: tuple[int, int, int], t: float) -> tuple[float, float, float]:
    t = max(0.0, min(1.0, t))
    return (a[0] * (1 - t) + b[0] * t, a[1] * (1 - t) + b[1] * t, a[2] * (1 - t) + b[2] * t)


def flourish_boost(px: float, py: float, cx: float, cy: float) -> float:
    """Subtle concentric rings (0..~0.22) baked into header corners."""
    d = math.hypot(px - cx, py - cy)
    rings = 0.0
    for r0, w in ((18.0, 2.2), (30.0, 2.0), (42.0, 1.8), (54.0, 1.6)):
        rings += math.exp(-((d - r0) / w) ** 2) * 0.18
    return min(0.28, rings)


def sample_notice_panel(px: float, py: float, w: float, h: float,
                        radius: float, border: float, header_h: float) -> tuple[int, int, int, int]:
    d_outer = sd_round_box(px, py, w, h, radius)
    outer = coverage(d_outer)
    if outer <= 0.001:
        return (0, 0, 0, 0)

    inset = border
    iw, ih = w - 2 * inset, h - 2 * inset
    ir = max(0.0, radius - inset)
    d_inner = sd_round_box(px - inset, py - inset, iw, ih, ir)
    inner = coverage(d_inner)

    border_a = outer * (1.0 - inner)
    fill_a = outer * inner
    a = border_a + fill_a
    if a < 0.001:
        return (0, 0, 0, 0)

    # Content coords relative to inner rect
    ix = px - inset
    iy = py - inset

    # Header vs body fill
    header_top = (126, 180, 240)
    header_bot = (90, 150, 220)
    body = (245, 249, 255)
    border_rgb = (158, 184, 232)

    in_header = iy < header_h
    if in_header:
        t = max(0.0, min(1.0, iy / max(1.0, header_h - 1)))
        fill = mix_rgb(header_top, header_bot, t)
        # Soft rings near top corners (clipped by alpha naturally)
        fl = flourish_boost(ix, iy, 8.0, header_h * 0.35)
        fr = flourish_boost(ix, iy, iw - 8.0, header_h * 0.35)
        boost = max(fl, fr)
        fill = (
            min(255.0, fill[0] + boost * 55),
            min(255.0, fill[1] + boost * 45),
            min(255.0, fill[2] + boost * 35),
        )
        # Hairline under header
        if abs(iy - (header_h - 0.5)) < 0.9:
            fill = mix_rgb((70, 120, 190), fill, 0.35)
    else:
        fill = body

    # Keep header outer rim the same blue family (no pale halo line on top)
    if in_header:
        border_c = mix_rgb((110, 160, 220), (70, 120, 190), max(0.0, min(1.0, iy / max(1.0, header_h - 1))))
    else:
        border_c = border_rgb

    r = (border_c[0] * border_a + fill[0] * fill_a) / a
    g = (border_c[1] * border_a + fill[1] * fill_a) / a
    b = (border_c[2] * border_a + fill[2] * fill_a) / a
    return (clamp(r), clamp(g), clamp(b), clamp(a * 255))


def sample_framed(px: float, py: float, w: float, h: float, radius: float, border: float,
                  fill: tuple[int, int, int], border_rgb: tuple[int, int, int]) -> tuple[int, int, int, int]:
    d_outer = sd_round_box(px, py, w, h, radius)
    outer = coverage(d_outer)
    if outer <= 0.001:
        return (0, 0, 0, 0)

    inset = border
    iw, ih = w - 2 * inset, h - 2 * inset
    ir = max(0.0, radius - inset)
    d_inner = sd_round_box(px - inset, py - inset, iw, ih, ir)
    inner = coverage(d_inner)

    border_a = outer * (1.0 - inner)
    fill_a = outer * inner
    a = border_a + fill_a
    if a < 0.001:
        return (0, 0, 0, 0)

    r = (border_rgb[0] * border_a + fill[0] * fill_a) / a
    g = (border_rgb[1] * border_a + fill[1] * fill_a) / a
    b = (border_rgb[2] * border_a + fill[2] * fill_a) / a
    return (clamp(r), clamp(g), clamp(b), clamp(a * 255))


def sample_gradient_framed(px: float, py: float, w: float, h: float, radius: float, border: float,
                           fill_top: tuple[int, int, int], fill_bot: tuple[int, int, int],
                           border_hi: tuple[int, int, int], border_lo: tuple[int, int, int]
                           ) -> tuple[int, int, int, int]:
    t = max(0.0, min(1.0, py / max(1.0, h - 1)))
    fill = mix_rgb(fill_top, fill_bot, t)
    bord = mix_rgb(border_hi, border_lo, t)
    return sample_framed(px, py, w, h, radius, border,
                         (int(fill[0]), int(fill[1]), int(fill[2])),
                         (int(bord[0]), int(bord[1]), int(bord[2])))


def render(w: int, h: int, fn) -> list[tuple[int, int, int, int]]:
    out: list[tuple[int, int, int, int]] = []
    for y in range(h):
        for x in range(w):
            ar = ag = ab = aa = 0.0
            for sy in range(SS):
                for sx in range(SS):
                    px = x + (sx + 0.5) / SS
                    py = y + (sy + 0.5) / SS
                    r, g, b, a = fn(px, py)
                    ar += r
                    ag += g
                    ab += b
                    aa += a
            n = float(SS * SS)
            out.append((clamp(ar / n), clamp(ag / n), clamp(ab / n), clamp(aa / n)))
    return out


def crop(rgba: list[tuple[int, int, int, int]], src_w: int, x0: int, y0: int, cw: int, ch: int):
    return [rgba[(y0 + y) * src_w + (x0 + x)] for y in range(ch) for x in range(cw)]


def gen_panel() -> None:
    """
    9-slice where top corners/edge are TALL = full header height.
    Soft outer AA is only on the panel silhouette — no separate header layer.
    """
    size_w = 128
    size_h = 200
    radius = 18.0
    border = 2.0
    header_h = 52.0  # inner content header band

    master = render(
        size_w, size_h,
        lambda px, py: sample_notice_panel(px, py, float(size_w), float(size_h), radius, border, header_h),
    )

    # Top slice height must cover rounded corner + header band
    top_h = int(border + header_h) + 2  # ~56
    side = 24
    bot = 24
    edge = 8

    write_tga(OUT / "panel_tl.tga", side, top_h, crop(master, size_w, 0, 0, side, top_h))
    write_tga(OUT / "panel_tr.tga", side, top_h, crop(master, size_w, size_w - side, 0, side, top_h))
    write_tga(OUT / "panel_bl.tga", side, bot, crop(master, size_w, 0, size_h - bot, side, bot))
    write_tga(OUT / "panel_br.tga", side, bot, crop(master, size_w, size_w - side, size_h - bot, side, bot))

    mx = size_w // 2 - edge // 2
    # Mid Y for side edges: below header
    my = int(border + header_h) + (size_h - int(border + header_h) - bot) // 2 - edge // 2

    write_tga(OUT / "panel_t.tga", edge, top_h, crop(master, size_w, mx, 0, edge, top_h))
    write_tga(OUT / "panel_b.tga", edge, bot, crop(master, size_w, mx, size_h - bot, edge, bot))
    write_tga(OUT / "panel_l.tga", side, edge, crop(master, size_w, 0, my, side, edge))
    write_tga(OUT / "panel_r.tga", side, edge, crop(master, size_w, size_w - side, my, side, edge))
    write_tga(OUT / "panel_c.tga", edge, edge, crop(master, size_w, mx, my, edge, edge))


def gen_input() -> None:
    size = 64
    radius = 10.0
    border = 1.5
    fill = (255, 255, 255)
    border_rgb = (160, 188, 224)
    master = render(size, size, lambda px, py: sample_framed(
        px, py, float(size), float(size), radius, border, fill, border_rgb))
    c = 16
    e = 8
    write_tga(OUT / "input_tl.tga", c, c, crop(master, size, 0, 0, c, c))
    write_tga(OUT / "input_tr.tga", c, c, crop(master, size, size - c, 0, c, c))
    write_tga(OUT / "input_bl.tga", c, c, crop(master, size, 0, size - c, c, c))
    write_tga(OUT / "input_br.tga", c, c, crop(master, size, size - c, size - c, c, c))
    mx = size // 2 - e // 2
    my = size // 2 - e // 2
    write_tga(OUT / "input_t.tga", e, c, crop(master, size, mx, 0, e, c))
    write_tga(OUT / "input_b.tga", e, c, crop(master, size, mx, size - c, e, c))
    write_tga(OUT / "input_l.tga", c, e, crop(master, size, 0, my, c, e))
    write_tga(OUT / "input_r.tga", c, e, crop(master, size, size - c, my, c, e))
    write_tga(OUT / "input_c.tga", e, e, crop(master, size, mx, my, e, e))


def gen_pill(name: str, top: tuple[int, int, int], bot: tuple[int, int, int],
             bhi: tuple[int, int, int], blo: tuple[int, int, int]) -> None:
    # Match on-screen button height (~46dp) so edges aren't blurred by downscale
    h = 46
    cap = h
    mid = 12
    w = cap * 2 + mid
    radius = h * 0.5
    master = render(w, h, lambda px, py: sample_gradient_framed(
        px, py, float(w), float(h), radius, 1.35, top, bot, bhi, blo))
    write_tga(OUT / f"{name}_l.tga", cap, h, crop(master, w, 0, 0, cap, h))
    write_tga(OUT / f"{name}_c.tga", mid, h, crop(master, w, cap, 0, mid, h))
    write_tga(OUT / f"{name}_r.tga", cap, h, crop(master, w, cap + mid, 0, cap, h))


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    gen_panel()
    gen_input()
    gen_pill("btn_blue", (126, 180, 240), (74, 138, 208), (170, 205, 250), (58, 120, 192))
    gen_pill("btn_gold", (245, 215, 130), (224, 176, 64), (255, 235, 170), (200, 152, 48))
    # Remove obsolete separate header tiles if present
    for name in ("header_l.tga", "header_c.tga", "header_r.tga"):
        p = OUT / name
        if p.exists():
            p.unlink()
            print(f"removed {name}")
    print("done ->", OUT)


if __name__ == "__main__":
    main()
