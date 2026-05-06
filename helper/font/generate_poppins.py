#!/usr/bin/env python3
"""
PKODev Poppins BMFont Generator
Generates .fnt + .png atlas files compatible with the PKODev BMFont renderer.
No bmfont64.exe required - uses fonttools + Pillow.

Usage:
    python generate_poppins.py
"""

import os
import math
from PIL import Image, ImageDraw, ImageFont

try:
    from fonttools import ttLib
    HAS_FONTTOOLS = True
except ImportError:
    HAS_FONTTOOLS = False
    print("Warning: fonttools not found. Kerning skipped. pip install fonttools")

# ---------------------------------------------------------------------------
# Character set - mirrors the existing PKODev bmfc config
# ---------------------------------------------------------------------------
CHAR_IDS = (
    list(range(32, 127))     # Basic ASCII (printable)
    + list(range(160, 256))  # Extended Latin
    + [8211, 8212,           # En dash, Em dash
       8216, 8217,           # Left/Right single quotation marks
       8220, 8221,           # Left/Right double quotation marks
       8226,                 # Bullet
       8230,                 # Horizontal ellipsis
       8364]                 # Euro sign
)

ATLAS_SIZE = 512
GLYPH_PAD  = 1   # pixels between glyphs in atlas

# ---------------------------------------------------------------------------
# Font map: (output_name, poppins_file, size_pt, outline_px, bold, italic)
# ---------------------------------------------------------------------------
POPPINS_DIR = r"C:\Users\pisig\Downloads\Poppins"

FONT_MAP = [
    ("gamedefaultsm",          "Poppins-Regular.ttf",  12, 0, 0, 0),
    ("gamedefaultmid",         "Poppins-Regular.ttf",  14, 0, 0, 0),
    ("gamedefaultbig",         "Poppins-Regular.ttf",  20, 0, 0, 0),
    ("gamedefaulthuge",        "Poppins-Regular.ttf",  28, 0, 0, 0),
    ("gamedefaultsmblack",     "Poppins-Bold.ttf",     12, 0, 1, 0),
    ("gamedefaultmidblack",    "Poppins-Bold.ttf",     14, 0, 1, 0),
    ("gamedefaultsmsemibold",  "Poppins-SemiBold.ttf", 12, 0, 0, 0),
    ("gamedefaultmidsemibold", "Poppins-SemiBold.ttf", 14, 0, 0, 0),
    ("titleblack",             "Poppins-Bold.ttf",     40, 0, 1, 0),
    ("splashblack",            "Poppins-Bold.ttf",     48, 0, 1, 0),
    # nameoutline: pre-baked 1px black outline (9x faster than runtime ORender)
    ("nameoutline",            "Poppins-Bold.ttf",     14, 1, 1, 0),
]

# ---------------------------------------------------------------------------
# Kerning extraction
# ---------------------------------------------------------------------------
def get_kerning_pairs(font_path):
    if not HAS_FONTTOOLS:
        return {}
    try:
        tt = ttLib.TTFont(font_path)
        if "kern" not in tt:
            return {}
        pairs = {}
        for subtable in tt["kern"].kernTables:
            if subtable.Format == 0:
                for (first, second), value in subtable.kernTable.items():
                    pairs[(ord(first), ord(second))] = value
        return pairs
    except Exception as e:
        print(f"    Warning: kerning read failed: {e}")
        return {}

# ---------------------------------------------------------------------------
# Glyph rendering
# ---------------------------------------------------------------------------
def render_glyph(char, pil_font, outline_thickness=0):
    """
    Render one glyph and return (img_rgba, xoffset, yoffset, xadvance).
    img_rgba is None when the char has no visible pixels (e.g. space).
    xoffset/yoffset are from the draw origin (top-of-line, left-of-cursor).
    """
    bbox    = pil_font.getbbox(char)
    xadvance = int(math.ceil(pil_font.getlength(char)))

    if bbox is None or (bbox[2] - bbox[0]) <= 0 or (bbox[3] - bbox[1]) <= 0:
        return None, 0, 0, xadvance

    glyph_w = bbox[2] - bbox[0]
    glyph_h = bbox[3] - bbox[1]

    # Canvas is tight around the visible pixels plus outline bleed
    extra    = outline_thickness
    canvas_w = glyph_w + extra * 2
    canvas_h = glyph_h + extra * 2

    # Draw position inside canvas so the glyph top-left lands at (extra, extra)
    draw_x = extra - bbox[0]
    draw_y = extra - bbox[1]

    if outline_thickness > 0:
        # Accumulate outline passes into a separate layer
        outline_layer = Image.new("L", (canvas_w, canvas_h), 0)
        ol_draw = ImageDraw.Draw(outline_layer)
        for dx in range(-outline_thickness, outline_thickness + 1):
            for dy in range(-outline_thickness, outline_thickness + 1):
                if dx == 0 and dy == 0:
                    continue
                ol_draw.text((draw_x + dx, draw_y + dy), char, font=pil_font, fill=255)

        fill_layer = Image.new("L", (canvas_w, canvas_h), 0)
        ImageDraw.Draw(fill_layer).text((draw_x, draw_y), char, font=pil_font, fill=255)

        # Black RGBA for outline region, white RGBA for fill region
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
    return img_rgba, xoffset, yoffset, xadvance

# ---------------------------------------------------------------------------
# Shelf packer
# ---------------------------------------------------------------------------
def pack_glyphs(glyph_map):
    """Pack glyphs into a 512x512 atlas using shelf packing (tallest first)."""
    atlas = Image.new("RGBA", (ATLAS_SIZE, ATLAS_SIZE), (0, 0, 0, 0))
    placements = {}

    items = [(cid, img) for cid, img in glyph_map.items() if img is not None]
    items.sort(key=lambda x: -x[1].size[1])

    cx, cy, row_h = GLYPH_PAD, GLYPH_PAD, 0

    for cid, img in items:
        w, h = img.size
        if cx + w + GLYPH_PAD > ATLAS_SIZE:
            cy += row_h + GLYPH_PAD
            cx  = GLYPH_PAD
            row_h = 0
        if cy + h + GLYPH_PAD > ATLAS_SIZE:
            print("    WARNING: Atlas overflow - increase ATLAS_SIZE!")
            break
        atlas.paste(img, (cx, cy))
        placements[cid] = (cx, cy, w, h)
        cx    += w + GLYPH_PAD
        row_h  = max(row_h, h)

    return atlas, placements

# ---------------------------------------------------------------------------
# Main generator
# ---------------------------------------------------------------------------
def generate_bmfont(font_path, font_name, font_size, output_dir,
                    outline_thickness=0, is_bold=0, is_italic=0):
    label = f"[{font_name}] {font_size}pt"
    if outline_thickness:
        label += f" +{outline_thickness}px outline"
    print(f"  {label} ... ", end="", flush=True)

    try:
        pil_font = ImageFont.truetype(font_path, font_size)
    except Exception as e:
        print(f"FAILED - {e}")
        return False

    ascent, descent = pil_font.getmetrics()
    line_height = ascent + descent

    # Render all glyphs
    glyph_images  = {}
    glyph_metrics = {}
    for cid in CHAR_IDS:
        try:
            char = chr(cid)
        except (ValueError, OverflowError):
            continue
        img, xoff, yoff, xadv = render_glyph(char, pil_font, outline_thickness)
        glyph_images[cid]  = img
        glyph_metrics[cid] = (xoff, yoff, xadv)

    # Pack into atlas
    atlas, placements = pack_glyphs(glyph_images)

    # Stub slot for invisible chars (space, etc.)
    stub = (ATLAS_SIZE - 2, ATLAS_SIZE - 2, 1, 1)
    for cid in CHAR_IDS:
        if cid not in placements:
            placements[cid] = stub

    # Save atlas
    os.makedirs(output_dir, exist_ok=True)
    atlas.save(os.path.join(output_dir, f"{font_name}_0.png"))

    # Kerning
    kerning = get_kerning_pairs(font_path)
    valid_kerning = {
        (f, s): amt
        for (f, s), amt in kerning.items()
        if f in placements and s in placements and amt != 0
    }

    # Write .fnt
    with open(os.path.join(output_dir, f"{font_name}.fnt"), "w") as f:
        f.write(
            f'info face="{font_name}" size=-{font_size} bold={is_bold} italic={is_italic}'
            f' charset="" unicode=1 stretchH=100 smooth=1 aa=1'
            f' padding={GLYPH_PAD},{GLYPH_PAD},{GLYPH_PAD},{GLYPH_PAD}'
            f' spacing=1,1 outline={outline_thickness}\n'
        )
        f.write(
            f'common lineHeight={line_height} base={ascent}'
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
    print(f"OK  ({visible} glyphs, {len(valid_kerning)} kern pairs)")
    return True

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(script_dir, "output")

    print("=" * 60)
    print("PKODev Poppins Font Generator")
    print("=" * 60)
    print(f"Poppins : {POPPINS_DIR}")
    print(f"Output  : {output_dir}")
    print()

    success = 0
    for font_name, poppins_file, size, outline, bold, italic in FONT_MAP:
        font_path = os.path.join(POPPINS_DIR, poppins_file)
        if not os.path.exists(font_path):
            print(f"  MISSING: {poppins_file}")
            continue
        if generate_bmfont(font_path, font_name, size, output_dir, outline, bold, italic):
            success += 1

    total = len(FONT_MAP)
    print()
    print(f"Done: {success}/{total} fonts generated")
    print(f"Output: {output_dir}")
    print()
    print("Next: run  copy_to_client.bat  to deploy to client/font/")

if __name__ == "__main__":
    main()
