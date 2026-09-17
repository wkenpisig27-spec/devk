#!/usr/bin/env python3
"""
Generate pre-baked BLACK-outline BMFonts from Open Sans.

Bakes black outline + white fill into RGBA so a single Render() keeps a
true black stroke under colored text (MODULATE: black stays black).
"""

import os
import math
import sys

from PIL import Image, ImageDraw, ImageFont

try:
    from fontTools import ttLib
    HAS_FONTTOOLS = True
except ImportError:
    HAS_FONTTOOLS = False

CHAR_IDS = (
    list(range(32, 127))
    + list(range(160, 256))
    + [8211, 8212, 8216, 8217, 8220, 8221, 8226, 8230, 8364]
)

ATLAS_SIZE = 512
GLYPH_PAD = 1

# (output_name, ttf_file, size_pt, outline_px, bold_flag)
FONT_MAP = [
    ("nameoutline",   "OpenSans-SemiBold.ttf",   14, 1, 0),
    ("namesmoutline", "OpenSans-SemiBold.ttf",   12, 1, 0),
    ("hintoutline",   "OpenSans-Regular.ttf",    12, 1, 0),
    ("titleoutline",  "OpenSans-ExtraBold.ttf",  28, 2, 1),
    ("splashoutline", "OpenSans-ExtraBold.ttf",  40, 2, 1),
]


def get_kerning_pairs(font_path):
    if not HAS_FONTTOOLS:
        return {}
    try:
        tt = ttLib.TTFont(font_path)
        if "kern" not in tt:
            return {}
        pairs = {}
        for subtable in tt["kern"].kernTables:
            if subtable.format == 0:
                for (first, second), value in subtable.kernTable.items():
                    pairs[(ord(first), ord(second))] = value
        return pairs
    except Exception as e:
        print(f"    Warning: kerning read failed: {e}")
        return {}


def _render_alpha(char, pil_font, canvas_w, canvas_h, draw_x, draw_y):
    layer = Image.new("L", (canvas_w, canvas_h), 0)
    ImageDraw.Draw(layer).text((draw_x, draw_y), char, font=pil_font, fill=255)
    return layer


def render_glyph(char, pil_font, outline_thickness=0):
    """
    Returns (img_rgba, xoffset, yoffset, xadvance).

    Critical for in-engine MODULATE (tex.rgb * vertex.rgb):
      - fill pixels MUST be pure white RGB
      - outline pixels MUST be pure black RGB
      - never emit gray RGB (alpha_composite of soft AA creates mud)
    Soft coverage lives only in alpha.
    """
    bbox = pil_font.getbbox(char)
    native_advance = int(math.ceil(pil_font.getlength(char)))

    if bbox is None or (bbox[2] - bbox[0]) <= 0 or (bbox[3] - bbox[1]) <= 0:
        return None, 0, 0, max(native_advance, 1)

    glyph_w = bbox[2] - bbox[0]
    glyph_h = bbox[3] - bbox[1]
    extra = outline_thickness
    # +1px safety so AA fringe isn't clipped
    pad = extra + 1
    canvas_w = glyph_w + pad * 2
    canvas_h = glyph_h + pad * 2
    draw_x = pad - bbox[0]
    draw_y = pad - bbox[1]

    fill_layer = _render_alpha(char, pil_font, canvas_w, canvas_h, draw_x, draw_y)

    if outline_thickness > 0:
        outline_layer = Image.new("L", (canvas_w, canvas_h), 0)
        ol_draw = ImageDraw.Draw(outline_layer)
        for dx in range(-outline_thickness, outline_thickness + 1):
            for dy in range(-outline_thickness, outline_thickness + 1):
                if dx == 0 and dy == 0:
                    continue
                ol_draw.text((draw_x + dx, draw_y + dy), char, font=pil_font, fill=255)
    else:
        outline_layer = None

    # Build RGBA pixel-by-pixel: white fill wins over black outline; no gray RGB.
    img_rgba = Image.new("RGBA", (canvas_w, canvas_h), (0, 0, 0, 0))
    fp = fill_layer.load()
    op = outline_layer.load() if outline_layer is not None else None
    out = img_rgba.load()
    for y in range(canvas_h):
        for x in range(canvas_w):
            fa = fp[x, y]
            oa = op[x, y] if op is not None else 0
            if fa >= 32:
                # Pure white fill — alpha from fill coverage
                out[x, y] = (255, 255, 255, fa)
            elif oa >= 32:
                # Pure black outline — alpha from outline coverage
                out[x, y] = (0, 0, 0, oa)
            # else leave transparent

    # Tight crop to ink (keeps UV boxes honest)
    bbox2 = img_rgba.getbbox()
    if bbox2 is None:
        return None, 0, 0, max(native_advance, 1)
    left, top, right, bottom = bbox2
    img_rgba = img_rgba.crop((left, top, right, bottom))

    # Origin was (0,0)=text origin; we drew at (draw_x, draw_y) relative to canvas
    # Canvas (0,0) corresponds to text origin shifted by (-pad+bbox[0]?):
    # text origin maps to canvas pixel (draw_x + bbox[0], draw_y + bbox[1]) = (pad, pad)
    # So canvas (0,0) is text origin + (-pad, -pad) relative to... 
    # Actually: draw at (draw_x, draw_y) where draw_x = pad - bbox[0].
    # Pillow draws glyph so ink's bbox[0] lands at draw_x + bbox[0] = pad.
    # Text origin (0,0) is at canvas position where glyph's local 0 is.
    # Local glyph coord (0,0) is at canvas (draw_x, draw_y) = (pad - bbox[0], pad - bbox[1]).
    # So text origin is at canvas (pad - bbox[0], pad - bbox[1]).
    origin_x = pad - bbox[0]
    origin_y = pad - bbox[1]
    xoffset = left - origin_x
    yoffset = top - origin_y
    xadvance = max(1, native_advance + outline_thickness)
    return img_rgba, int(xoffset), int(yoffset), int(xadvance)


def paste_opaque(dst: Image.Image, src: Image.Image, xy: tuple[int, int]) -> None:
    """Copy src onto dst without alpha-blending (keeps pure black/white RGB)."""
    x0, y0 = xy
    sp = src.load()
    dp = dst.load()
    sw, sh = src.size
    for y in range(sh):
        for x in range(sw):
            r, g, b, a = sp[x, y]
            if a:
                dp[x0 + x, y0 + y] = (r, g, b, a)


def pack_glyphs(glyph_map):
    atlas = Image.new("RGBA", (ATLAS_SIZE, ATLAS_SIZE), (0, 0, 0, 0))
    placements = {}
    items = [(cid, img) for cid, img in glyph_map.items() if img is not None]
    items.sort(key=lambda x: -x[1].size[1])

    cx, cy, row_h = GLYPH_PAD, GLYPH_PAD, 0
    for cid, img in items:
        w, h = img.size
        if cx + w + GLYPH_PAD > ATLAS_SIZE:
            cy += row_h + GLYPH_PAD
            cx = GLYPH_PAD
            row_h = 0
        if cy + h + GLYPH_PAD > ATLAS_SIZE:
            print("    WARNING: Atlas overflow!")
            break
        paste_opaque(atlas, img, (cx, cy))
        placements[cid] = (cx, cy, w, h)
        cx += w + GLYPH_PAD
        row_h = max(row_h, h)
    return atlas, placements


def generate_bmfont(font_path, font_name, font_size, output_dir, outline_thickness=0, is_bold=0):
    label = f"[{font_name}] {font_size}pt +{outline_thickness}px outline"
    print(f"  {label} ... ", end="", flush=True)

    try:
        pil_font = ImageFont.truetype(font_path, font_size)
    except Exception as e:
        print(f"FAILED - {e}")
        return False

    ascent, descent = pil_font.getmetrics()
    line_height = ascent + descent + outline_thickness * 2

    glyph_images = {}
    glyph_metrics = {}
    for cid in CHAR_IDS:
        try:
            char = chr(cid)
        except (ValueError, OverflowError):
            continue
        img, xoff, yoff, xadv = render_glyph(char, pil_font, outline_thickness)
        glyph_images[cid] = img
        glyph_metrics[cid] = (xoff, yoff, xadv)

    atlas, placements = pack_glyphs(glyph_images)
    stub = (ATLAS_SIZE - 2, ATLAS_SIZE - 2, 1, 1)
    for cid in CHAR_IDS:
        if cid not in placements:
            placements[cid] = stub

    os.makedirs(output_dir, exist_ok=True)
    atlas.save(os.path.join(output_dir, f"{font_name}_0.png"))

    kerning = get_kerning_pairs(font_path)
    valid_kerning = {
        (f, s): amt
        for (f, s), amt in kerning.items()
        if f in placements and s in placements and amt != 0
    }

    with open(os.path.join(output_dir, f"{font_name}.fnt"), "w", encoding="ascii") as f:
        f.write(
            f'info face="{font_name}" size=-{font_size} bold={is_bold} italic=0'
            f' charset="" unicode=1 stretchH=100 smooth=1 aa=1'
            f' padding={GLYPH_PAD},{GLYPH_PAD},{GLYPH_PAD},{GLYPH_PAD}'
            f' spacing=1,1 outline={outline_thickness}\n'
        )
        # Outline fonts store black/white in RGB (MODULATE keeps outline black).
        # Plain glyphs (outline=0) are white RGB + alpha coverage.
        if outline_thickness > 0:
            chnl = "alphaChnl=1 redChnl=0 greenChnl=0 blueChnl=0"
        else:
            chnl = "alphaChnl=0 redChnl=4 greenChnl=4 blueChnl=4"
        f.write(
            f'common lineHeight={line_height} base={ascent + outline_thickness}'
            f' scaleW={ATLAS_SIZE} scaleH={ATLAS_SIZE}'
            f' pages=1 packed=0 {chnl}\n'
        )
        f.write(f'page id=0 file="{font_name}_0.png"\n')
        f.write(f'chars count={len(placements)}\n')
        for cid in sorted(placements):
            ax, ay, aw, ah = placements[cid]
            xoff, yoff, xadv = glyph_metrics.get(cid, (0, 0, 0))
            f.write(
                f'char id={cid:<6} x={ax:<6} y={ay:<6}'
                f' width={aw:<6} height={ah:<6}'
                f' xoffset={xoff:<6} yoffset={yoff:<6}'
                f' xadvance={xadv:<6} page=0  chnl=15\n'
            )
        f.write(f'kernings count={len(valid_kerning)}\n')
        for (first, second), amount in sorted(valid_kerning.items()):
            f.write(f'kerning first={first}  second={second}  amount={amount}\n')

    visible = sum(1 for img in glyph_images.values() if img is not None)
    print(f"OK  ({visible} glyphs)")
    return True


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(script_dir, "output")

    default_src = os.path.join(script_dir, "opensans_src")
    src = sys.argv[1] if len(sys.argv) > 1 else default_src
    if not os.path.isdir(src):
        print(f"ERROR: Open Sans folder not found: {src}")
        return 1

    print("=" * 60)
    print("Open Sans pre-baked BLACK outline fonts")
    print("=" * 60)
    print(f"Source : {src}")
    print(f"Output : {output_dir}")
    print()

    success = 0
    for font_name, ttf_file, size, outline, bold in FONT_MAP:
        font_path = os.path.join(src, ttf_file)
        if not os.path.exists(font_path):
            print(f"  MISSING: {ttf_file}")
            continue
        if generate_bmfont(font_path, font_name, size, output_dir, outline, bold):
            success += 1

    print()
    print(f"Done: {success}/{len(FONT_MAP)} outline fonts")
    return 0 if success == len(FONT_MAP) else 1


if __name__ == "__main__":
    raise SystemExit(main())
