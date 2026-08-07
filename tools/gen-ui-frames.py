# Generate UI frame / ornament TGAs for RmlUi (32-bit, top-left origin).
# Usage: python tools/gen-ui-frames.py

from __future__ import annotations

import math
import struct
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "client" / "ui" / "rml" / "frames"


def clamp(v: int) -> int:
    return 0 if v < 0 else 255 if v > 255 else v


def write_tga(path: Path, w: int, h: int, rgba: list[tuple[int, int, int, int]]) -> None:
    header = struct.pack(
        "<BBBHHBHHHHBB",
        0,
        0,
        2,
        0,
        0,
        0,
        0,
        0,
        w,
        h,
        32,
        0x20,
    )
    raw = bytearray()
    for r, g, b, a in rgba:
        raw += bytes((b, g, r, a))
    path.write_bytes(header + raw)
    print("wrote", path, f"({w}x{h})")


def ring_alpha(dist: float, radius: float, width: float = 1.4) -> float:
    d = abs(dist - radius)
    if d > width:
        return 0.0
    return max(0.0, 1.0 - d / width)


def header_flourish_pixels(w: int = 96, h: int = 96) -> list[tuple[int, int, int, int]]:
    """Soft white concentric swirls for light-blue Notice headers."""
    cx, cy = w * 0.28, h * 0.42
    pixels: list[tuple[int, int, int, int]] = []
    for y in range(h):
        for x in range(w):
            dx = x - cx
            dy = y - cy
            dist = math.sqrt(dx * dx + dy * dy)
            ang = math.atan2(dy, dx)

            a = 0.0
            for radius in (12.0, 22.0, 34.0, 46.0):
                a = max(a, ring_alpha(dist, radius, 1.25) * 0.55)

            # Soft spiral accent
            spiral_r = 18.0 + (ang + math.pi) * 4.5
            a = max(a, ring_alpha(dist, spiral_r, 1.1) * 0.35)

            # Fade toward right so title stays readable
            fade = max(0.0, 1.0 - (x / w) * 1.15)
            alpha = clamp(int(255 * a * fade))
            pixels.append((255, 255, 255, alpha))
    return pixels


def corner_pixels(size: int, mirror_x: bool, mirror_y: bool) -> list[tuple[int, int, int, int]]:
    pixels: list[tuple[int, int, int, int]] = []
    for y in range(size):
        for x in range(size):
            lx = (size - 1 - x) if mirror_x else x
            ly = (size - 1 - y) if mirror_y else y
            on_frame = (lx < 6 and ly < 28) or (ly < 6 and lx < 28)
            on_inner = (lx < 3 and ly < 22) or (ly < 3 and lx < 22)
            on_glow = (lx < 9 and ly < 32) or (ly < 9 and lx < 32)
            r = g = b = a = 0
            if on_glow and not on_frame:
                t = 1.0 - min(lx, ly) / 9.0
                a = clamp(int(90 * t))
                r, g, b = 70, 210, 230
            if on_frame:
                shade = 140 + (lx * 3 + ly * 2) % 40
                a = 235
                r = clamp(shade - 10)
                g = clamp(shade + 8)
                b = clamp(shade + 20)
            if on_inner:
                r, g, b, a = 40, 190, 205, 255
            pixels.append((r, g, b, a))
    return pixels


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)

    write_tga(OUT / "header_flourish.tga", 96, 96, header_flourish_pixels())

    # Mirror for right side of header
    left = header_flourish_pixels()
    w, h = 96, 96
    right: list[tuple[int, int, int, int]] = []
    for y in range(h):
        row = left[y * w : (y + 1) * w]
        right.extend(reversed(row))
    write_tga(OUT / "header_flourish_r.tga", w, h, right)

    size = 48
    for name, mx, my in (
        ("corner_tl.tga", False, False),
        ("corner_tr.tga", True, False),
        ("corner_bl.tga", False, True),
        ("corner_br.tga", True, True),
    ):
        write_tga(OUT / name, size, size, corner_pixels(size, mx, my))


if __name__ == "__main__":
    main()
