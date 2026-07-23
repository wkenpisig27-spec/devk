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
    ("nameoutline",   "OpenSans-Bold.ttf",       14, 1, 1),
    ("namesmoutline", "OpenSans-Bold.ttf",       12, 1, 1),
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


def render_glyph(char, pil_font, outline_thickness=0):
    """
    Returns (img_rgba, xoffset, yoffset, xadvance).

    Keep native advance for natural spacing. Only add outline_thickness so
    baked strokes don't collide with the next glyph.
    """
    bbox = pil_font.getbbox(char)
    native_advance = int(math.ceil(pil_font.getlength(char)))

    if bbox is None or (bbox[2] - bbox[0]) <= 0 or (bbox[3] - bbox[1]) <= 0:
        return None, 0, 0, max(native_advance, 1)

    glyph_w = bbox[2] - bbox[0]
    glyph_h = bbox[3] - bbox[1]
    extra = outline_thickness
    canvas_w = glyph_w + extra * 2
    canvas_h = glyph_h + extra * 2
    draw_x = extra - bbox[0]
    draw_y = extra - bbox[1]

    if outline_thickness > 0:
        outline_layer = Image.new("L", (canvas_w, canvas_h), 0)
        ol_draw = ImageDraw.Draw(outline_layer)
        for dx in range(-outline_thickness, outline_thickness + 1):
            for dy in range(-outline_thickness, outline_thickness + 1):
                if dx == 0 and dy == 0:
                    continue
                ol_draw.text((draw_x + dx, draw_y + dy), char, font=pil_font, fill=255)

        fill_layer = Image.new("L", (canvas_w, canvas_h), 0)
        ImageDraw.Draw(fill_layer).text((draw_x, draw_y), char, font=pil_font, fill=255)

        black = Image.new("RGBA", (canvas_w, canvas_h), (0, 0, 0, 0))
        white = Image.new("RGBA", (canvas_w, canvas_h), (255, 255, 255, 0))
        black.putalpha(outline_layer)
        white.putalpha(fill_layer)
        img_rgba = Image.alpha_composite(black, white)
    else:
        canvas = Image.new("L", (canvas_w, canvas_h), 0)
        ImageDraw.Draw(canvas).text((draw_x, draw_y), char, font=pil_font, fill=255)
        img_rgba = Image.new("RGBA", (canvas_w, canvas_h), (255, 255, 255, 0))
        img_rgba.putalpha(canvas)

    xoffset = bbox[0] - extra
    yoffset = bbox[1] - extra
    # Keep native metrics for authentic spacing; only reserve room for the outline.
    xadvance = max(1, native_advance + outline_thickness)
    return img_rgba, xoffset, yoffset, xadvance


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
        atlas.paste(img, (cx, cy), img)
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
        f.write(
            f'common lineHeight={line_height} base={ascent + outline_thickness}'
            f' scaleW={ATLAS_SIZE} scaleH={ATLAS_SIZE}'
            f' pages=1 packed=0 alphaChnl=0 redChnl=4 greenChnl=4 blueChnl=4\n'
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
