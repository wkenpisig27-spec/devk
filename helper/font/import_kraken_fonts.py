#!/usr/bin/env python3
"""Convert Kraken .gft+.tga GameDefault fonts into BMFont .fnt+.png for our client."""
import struct
from pathlib import Path
from PIL import Image

KRAKEN = Path(r"C:\Users\Ken\Desktop\Github\devk\helper\font\kraken_extract")
OUT = Path(r"C:\Users\Ken\Desktop\Github\devk\helper\font\output")
CLIENT = Path(r"C:\Users\Ken\Desktop\Github\devk\client\font")


def parse_glyphs(data: bytes, pos: int, count: int, tex_h: int = 512):
    """Parse glyph records. GFT Y is bottom-up; convert to BMFont top-left with floor/ceil."""
    import math

    glyphs = {}
    for _ in range(count):
        if pos + 40 > len(data):
            break
        cid = struct.unpack_from("<I", data, pos)[0]
        fs = struct.unpack_from("<9f", data, pos + 4)
        # fs[1]=x1, fs[2]=yMax, fs[3]=yMin, fs[4]=x2  (Y origin at bottom of atlas)
        x1, y_max, y_min, x2 = fs[1], fs[2], fs[3], fs[4]
        if y_min > y_max:
            y_min, y_max = y_max, y_min
        if x1 > x2:
            x1, x2 = x2, x1

        x = int(math.floor(x1))
        w = max(int(math.ceil(x2) - math.floor(x1)), 0)
        # Inclusive-ish float UVs: use ceil on max so the top row (e.g. T crossbar) is kept
        y = int(tex_h - math.ceil(y_max))
        h = max(int(math.ceil(y_max) - math.floor(y_min)), 0)
        if y < 0:
            h += y
            y = 0
        glyphs[cid] = {
            "x": x,
            "y": y,
            "width": w,
            "height": h,
            "xoffset": int(round(fs[5])),
            "yoffset": int(round(fs[7])),
            "xadvance": max(int(round(fs[8])), 0),
            "bottom": fs[6],
        }
        pos += 40
    return glyphs


def parse_gft(path: Path):
    data = path.read_bytes()
    # Detect format: Sm has version=256 and embedded name; Mid+ has size at +4 and count at +40
    ver = struct.unpack_from("<I", data, 0)[0]
    if ver == 256 or data.find(b"GameDefault") == 48:
        font_size = struct.unpack_from("<f", data, 8)[0]
        tex_w = struct.unpack_from("<I", data, 12)[0]
        tex_h = struct.unpack_from("<I", data, 16)[0]
        line_height = struct.unpack_from("<f", data, 24)[0]
        name_len = struct.unpack_from("<I", data, 44)[0]
        name = data[48 : 48 + name_len].decode("ascii", errors="ignore")
        start = 48 + name_len
        count = struct.unpack_from("<I", data, start)[0]
        glyphs = parse_glyphs(data, start + 4, count, tex_h or 512)
    else:
        font_size = struct.unpack_from("<f", data, 4)[0]
        tex_w = struct.unpack_from("<I", data, 8)[0]
        tex_h = struct.unpack_from("<I", data, 12)[0]
        line_height = struct.unpack_from("<f", data, 20)[0]
        count = struct.unpack_from("<I", data, 40)[0]
        name = path.stem.replace("texture_ui_font_", "")
        glyphs = parse_glyphs(data, 44, count, tex_h or 512)

    base = int(round(line_height)) - 2
    if glyphs:
        base = int(round(max(g["bottom"] for g in glyphs.values() if g["bottom"] > 0)))
    return {
        "name": name,
        "size": int(round(font_size)),
        "lineHeight": int(round(line_height)),
        "base": base,
        "scaleW": tex_w or 512,
        "scaleH": tex_h or 512,
        "glyphs": glyphs,
    }


def tga_to_bmfont_png(tga_path: Path, png_path: Path):
    """Kraken TGA: glyph coverage in alpha. Emit RGB+A coverage for BMFont."""
    im = Image.open(tga_path).convert("RGBA")
    a = im.split()[-1]
    Image.merge("RGBA", (a, a, a, a)).save(png_path)


def write_fnt(meta, png_name: str, out_fnt: Path):
    glyphs = meta["glyphs"]
    lines = [
        f'info face="{meta["name"]}" size=-{meta["size"]} bold=0 italic=0 '
        f'charset="" unicode=1 stretchH=100 smooth=1 aa=1 '
        f'padding=0,0,0,0 spacing=1,1 outline=0',
        f'common lineHeight={meta["lineHeight"]} base={meta["base"]} '
        f'scaleW={meta["scaleW"]} scaleH={meta["scaleH"]} '
        f'pages=1 packed=0 alphaChnl=0 redChnl=4 greenChnl=4 blueChnl=4',
        f'page id=0 file="{png_name}"',
        f'chars count={len(glyphs)}',
    ]
    for cid in sorted(glyphs):
        g = glyphs[cid]
        lines.append(
            f'char id={cid:<6} x={g["x"]:<6} y={g["y"]:<6} '
            f'width={g["width"]:<6} height={g["height"]:<6} '
            f'xoffset={g["xoffset"]:<6} yoffset={g["yoffset"]:<6} '
            f'xadvance={g["xadvance"]:<6} page=0  chnl=15'
        )
    lines.append("kernings count=0")
    out_fnt.write_text("\n".join(lines) + "\n", encoding="ascii")


def convert_one(gft_name: str, tga_name: str, out_name: str):
    gft = KRAKEN / gft_name
    tga = KRAKEN / tga_name
    if not gft.exists() or not tga.exists():
        print("MISSING", gft_name if not gft.exists() else tga_name)
        return False
    meta = parse_gft(gft)
    if len(meta["glyphs"]) < 50:
        print(f"FAIL {out_name}: only {len(meta['glyphs'])} glyphs")
        return False
    OUT.mkdir(parents=True, exist_ok=True)
    png_name = f"{out_name}_0.png"
    tga_to_bmfont_png(tga, OUT / png_name)
    write_fnt(meta, png_name, OUT / f"{out_name}.fnt")
    (CLIENT / f"{out_name}.fnt").write_bytes((OUT / f"{out_name}.fnt").read_bytes())
    (CLIENT / png_name).write_bytes((OUT / png_name).read_bytes())
    print(
        f"OK {out_name:28} face={meta['name']!r:20} size={meta['size']:2} "
        f"lh={meta['lineHeight']:2} glyphs={len(meta['glyphs']):4}"
    )
    for ch in "aTh":
        g = meta["glyphs"].get(ord(ch))
        if g:
            print(
                f"   '{ch}' {g['width']}x{g['height']} "
                f"xoff={g['xoffset']} yoff={g['yoffset']} xadv={g['xadvance']}"
            )
    return True


def main():
    jobs = [
        # Chat/default uses semibold slot -> Kraken CHAT_FONT (Sm2 metrics on Sm atlas)
        ("texture_ui_font_gamedefaultsm2.gft", "texture_ui_font_gamedefaultsm.tga", "gamedefaultsmsemibold"),
        ("texture_ui_font_gamedefaultsm.gft", "texture_ui_font_gamedefaultsm.tga", "gamedefaultsm"),
        ("texture_ui_font_gamedefaultmid.gft", "texture_ui_font_gamedefaultmid.tga", "gamedefaultmid"),
        ("texture_ui_font_gamedefaultmid2.gft", "texture_ui_font_gamedefaultmid2.tga", "gamedefaultmidsemibold"),
        ("texture_ui_font_gamedefaultbig.gft", "texture_ui_font_gamedefaultbig.tga", "gamedefaultbig"),
        ("texture_ui_font_gamedefaulthuge.gft", "texture_ui_font_gamedefaulthuge.tga", "gamedefaulthuge"),
        ("texture_ui_font_gamedefaultsm2.gft", "texture_ui_font_gamedefaultsm.tga", "gamedefaultsmblack"),
        ("texture_ui_font_gamedefaultmid2.gft", "texture_ui_font_gamedefaultmid2.tga", "gamedefaultmidblack"),
    ]
    ok = sum(1 for j in jobs if convert_one(*j))
    print(f"\nConverted {ok}/{len(jobs)}")
    print("Fully quit + restart the client to reload client/font/.")


if __name__ == "__main__":
    main()
