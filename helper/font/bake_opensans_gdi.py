#!/usr/bin/env python3
"""
Bake Open Sans BMFonts via Windows GDI (antialiased ExtTextOut → DIB).

Produces ascent-based yoffsets so T/h tops align more like Kraken GameDefaultSm,
without importing broken Kraken GFT atlases.
"""

from __future__ import annotations

import ctypes
import struct
import sys
from ctypes import wintypes
from pathlib import Path

from PIL import Image

CHAR_IDS = (
    list(range(32, 127))
    + list(range(160, 256))
    + [8211, 8212, 8216, 8217, 8220, 8221, 8226, 8230, 8364]
)

ATLAS_SIZE = 512
GLYPH_PAD = 1

FONT_JOBS = [
    ("gamedefaultsm", "OpenSans-Regular.ttf", 12, 0),
    ("gamedefaultsmsemibold", "OpenSans-SemiBold.ttf", 12, 0),
    ("gamedefaultsmblack", "OpenSans-Bold.ttf", 12, 1),
    ("gamedefaultmid", "OpenSans-Regular.ttf", 14, 0),
    ("gamedefaultmidsemibold", "OpenSans-SemiBold.ttf", 14, 0),
    ("gamedefaultmidblack", "OpenSans-Bold.ttf", 14, 1),
]

gdi32 = ctypes.WinDLL("gdi32", use_last_error=True)
user32 = ctypes.WinDLL("user32", use_last_error=True)

FR_PRIVATE = 0x10
LF_FACESIZE = 32
ANTIALIASED_QUALITY = 4
CLEARTYPE_QUALITY = 5
DEFAULT_CHARSET = 1
OUT_TT_PRECIS = 4
CLIP_DEFAULT_PRECIS = 0
FW_NORMAL, FW_SEMIBOLD, FW_BOLD = 400, 600, 700
BI_RGB = 0
DIB_RGB_COLORS = 0
TA_LEFT = 0
TA_TOP = 0
TRANSPARENT = 1
SRCCOPY = 0x00CC0020
ETO_OPAQUE = 2


class TEXTMETRICW(ctypes.Structure):
    _fields_ = [
        ("tmHeight", ctypes.c_long),
        ("tmAscent", ctypes.c_long),
        ("tmDescent", ctypes.c_long),
        ("tmInternalLeading", ctypes.c_long),
        ("tmExternalLeading", ctypes.c_long),
        ("tmAveCharWidth", ctypes.c_long),
        ("tmMaxCharWidth", ctypes.c_long),
        ("tmWeight", ctypes.c_long),
        ("tmOverhang", ctypes.c_long),
        ("tmDigitizedAspectX", ctypes.c_long),
        ("tmDigitizedAspectY", ctypes.c_long),
        ("tmFirstChar", wintypes.WCHAR),
        ("tmLastChar", wintypes.WCHAR),
        ("tmDefaultChar", wintypes.WCHAR),
        ("tmBreakChar", wintypes.WCHAR),
        ("tmItalic", ctypes.c_byte),
        ("tmUnderlined", ctypes.c_byte),
        ("tmStruckOut", ctypes.c_byte),
        ("tmPitchAndFamily", ctypes.c_byte),
        ("tmCharSet", ctypes.c_byte),
    ]


class LOGFONTW(ctypes.Structure):
    _fields_ = [
        ("lfHeight", ctypes.c_long),
        ("lfWidth", ctypes.c_long),
        ("lfEscapement", ctypes.c_long),
        ("lfOrientation", ctypes.c_long),
        ("lfWeight", ctypes.c_long),
        ("lfItalic", ctypes.c_byte),
        ("lfUnderline", ctypes.c_byte),
        ("lfStrikeOut", ctypes.c_byte),
        ("lfCharSet", ctypes.c_byte),
        ("lfOutPrecision", ctypes.c_byte),
        ("lfClipPrecision", ctypes.c_byte),
        ("lfQuality", ctypes.c_byte),
        ("lfPitchAndFamily", ctypes.c_byte),
        ("lfFaceName", wintypes.WCHAR * LF_FACESIZE),
    ]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wintypes.DWORD),
        ("biWidth", ctypes.c_long),
        ("biHeight", ctypes.c_long),
        ("biPlanes", wintypes.WORD),
        ("biBitCount", wintypes.WORD),
        ("biCompression", wintypes.DWORD),
        ("biSizeImage", wintypes.DWORD),
        ("biXPelsPerMeter", ctypes.c_long),
        ("biYPelsPerMeter", ctypes.c_long),
        ("biClrUsed", wintypes.DWORD),
        ("biClrImportant", wintypes.DWORD),
    ]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [("bmiHeader", BITMAPINFOHEADER), ("bmiColors", wintypes.DWORD * 3)]


class ABC(ctypes.Structure):
    _fields_ = [("abcA", ctypes.c_int), ("abcB", ctypes.c_uint), ("abcC", ctypes.c_int)]


class SIZE(ctypes.Structure):
    _fields_ = [("cx", ctypes.c_long), ("cy", ctypes.c_long)]


class RECT(ctypes.Structure):
    _fields_ = [
        ("left", ctypes.c_long),
        ("top", ctypes.c_long),
        ("right", ctypes.c_long),
        ("bottom", ctypes.c_long),
    ]


# Prototypes
gdi32.AddFontResourceExW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, ctypes.c_void_p]
gdi32.AddFontResourceExW.restype = ctypes.c_int
gdi32.RemoveFontResourceExW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, ctypes.c_void_p]
gdi32.CreateFontIndirectW.argtypes = [ctypes.POINTER(LOGFONTW)]
gdi32.CreateFontIndirectW.restype = wintypes.HGDIOBJ
gdi32.CreateCompatibleDC.argtypes = [wintypes.HDC]
gdi32.CreateCompatibleDC.restype = wintypes.HDC
gdi32.CreateDIBSection.argtypes = [
    wintypes.HDC,
    ctypes.POINTER(BITMAPINFO),
    wintypes.UINT,
    ctypes.POINTER(ctypes.c_void_p),
    wintypes.HANDLE,
    wintypes.DWORD,
]
gdi32.CreateDIBSection.restype = wintypes.HBITMAP
gdi32.SelectObject.argtypes = [wintypes.HDC, wintypes.HGDIOBJ]
gdi32.SelectObject.restype = wintypes.HGDIOBJ
gdi32.DeleteObject.argtypes = [wintypes.HGDIOBJ]
gdi32.DeleteDC.argtypes = [wintypes.HDC]
gdi32.GetTextMetricsW.argtypes = [wintypes.HDC, ctypes.POINTER(TEXTMETRICW)]
gdi32.SetTextColor.argtypes = [wintypes.HDC, wintypes.COLORREF]
gdi32.SetBkColor.argtypes = [wintypes.HDC, wintypes.COLORREF]
gdi32.SetBkMode.argtypes = [wintypes.HDC, ctypes.c_int]
gdi32.SetTextAlign.argtypes = [wintypes.HDC, wintypes.UINT]
gdi32.ExtTextOutW.argtypes = [
    wintypes.HDC,
    ctypes.c_int,
    ctypes.c_int,
    wintypes.UINT,
    ctypes.POINTER(RECT),
    wintypes.LPCWSTR,
    wintypes.UINT,
    ctypes.POINTER(ctypes.c_int),
]
gdi32.GetCharABCWidthsW.argtypes = [wintypes.HDC, wintypes.UINT, wintypes.UINT, ctypes.POINTER(ABC)]
gdi32.GetTextExtentPoint32W.argtypes = [wintypes.HDC, wintypes.LPCWSTR, ctypes.c_int, ctypes.POINTER(SIZE)]
gdi32.PatBlt.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, wintypes.DWORD]


def ttf_family_name(ttf_path: Path) -> str:
    data = ttf_path.read_bytes()
    num_tables = struct.unpack_from(">H", data, 4)[0]
    best = None
    for i in range(num_tables):
        tag, _c, off, _l = struct.unpack_from(">4sIII", data, 12 + i * 16)
        if tag != b"name":
            continue
        _fmt, count, string_offset = struct.unpack_from(">HHH", data, off)
        for j in range(count):
            platform, encoding, lang, name_id, length_n, offset_n = struct.unpack_from(
                ">HHHHHH", data, off + 6 + j * 12
            )
            if name_id != 1:
                continue
            raw = data[off + string_offset + offset_n : off + string_offset + offset_n + length_n]
            if platform == 3 and encoding in (1, 10):
                s = raw.decode("utf-16-be", errors="ignore")
            elif platform == 1:
                s = raw.decode("latin-1", errors="ignore")
            else:
                continue
            if lang in (0x0409, 0):
                return s
            best = s
    return best or ttf_path.stem.replace("-", " ")


class GdiSession:
    def __init__(self, ttf_path: Path, pixel_size: int, weight: int, use_cleartype: bool = False):
        self.ttf_path = str(ttf_path.resolve())
        self.pixel_size = pixel_size
        self.weight = weight
        self.use_cleartype = use_cleartype
        self.face = ttf_family_name(ttf_path)
        self._added = False
        self.hdc = None
        self.hfont = None
        self.tm = TEXTMETRICW()
        # Scratch DIB large enough for one glyph cell
        self.cell_w = max(pixel_size * 4, 64)
        self.cell_h = max(pixel_size * 3, 48)
        self.hbmp = None
        self.bits = None
        self._old_bmp = None

    def __enter__(self):
        if gdi32.AddFontResourceExW(self.ttf_path, FR_PRIVATE, None) == 0:
            raise RuntimeError(f"AddFontResourceEx failed: {self.ttf_path}")
        self._added = True

        lf = LOGFONTW()
        lf.lfHeight = -self.pixel_size
        lf.lfWeight = self.weight
        lf.lfCharSet = DEFAULT_CHARSET
        lf.lfOutPrecision = OUT_TT_PRECIS
        lf.lfClipPrecision = CLIP_DEFAULT_PRECIS
        lf.lfQuality = CLEARTYPE_QUALITY if self.use_cleartype else ANTIALIASED_QUALITY
        lf.lfFaceName = self.face

        self.hfont = gdi32.CreateFontIndirectW(ctypes.byref(lf))
        if not self.hfont:
            raise RuntimeError(f"CreateFont failed face={self.face!r}")

        self.hdc = gdi32.CreateCompatibleDC(None)
        gdi32.SelectObject(self.hdc, self.hfont)
        if not gdi32.GetTextMetricsW(self.hdc, ctypes.byref(self.tm)):
            raise RuntimeError("GetTextMetrics failed")

        # Top-down 32bpp DIB
        bmi = BITMAPINFO()
        bmi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
        bmi.bmiHeader.biWidth = self.cell_w
        bmi.bmiHeader.biHeight = -self.cell_h  # top-down
        bmi.bmiHeader.biPlanes = 1
        bmi.bmiHeader.biBitCount = 32
        bmi.bmiHeader.biCompression = BI_RGB
        bits_ptr = ctypes.c_void_p()
        self.hbmp = gdi32.CreateDIBSection(
            self.hdc, ctypes.byref(bmi), DIB_RGB_COLORS, ctypes.byref(bits_ptr), None, 0
        )
        if not self.hbmp or not bits_ptr:
            raise RuntimeError("CreateDIBSection failed")
        self.bits = bits_ptr
        self._old_bmp = gdi32.SelectObject(self.hdc, self.hbmp)

        gdi32.SetTextColor(self.hdc, 0x00FFFFFF)  # white BGR
        gdi32.SetBkColor(self.hdc, 0x00000000)
        gdi32.SetBkMode(self.hdc, TRANSPARENT)
        gdi32.SetTextAlign(self.hdc, TA_LEFT | TA_TOP)
        return self

    def __exit__(self, *exc):
        if self.hdc and self._old_bmp:
            gdi32.SelectObject(self.hdc, self._old_bmp)
        if self.hbmp:
            gdi32.DeleteObject(self.hbmp)
        if self.hfont:
            gdi32.DeleteObject(self.hfont)
        if self.hdc:
            gdi32.DeleteDC(self.hdc)
        if self._added:
            gdi32.RemoveFontResourceExW(self.ttf_path, FR_PRIVATE, None)

    def _clear(self):
        # Fill black
        gdi32.SetBkColor(self.hdc, 0x00000000)
        gdi32.SetBkMode(self.hdc, 2)  # OPAQUE briefly for PatBlt alternative
        # memset bits
        nbytes = self.cell_w * self.cell_h * 4
        ctypes.memset(self.bits, 0, nbytes)
        gdi32.SetBkMode(self.hdc, TRANSPARENT)

    def _pixel(self, x, y):
        # BGRA in top-down DIB
        off = (y * self.cell_w + x) * 4
        buf = (ctypes.c_ubyte * 4).from_address(self.bits.value + off)
        # luminance from RGB
        return max(buf[0], buf[1], buf[2])

    def glyph(self, codepoint: int):
        ch = chr(codepoint)
        abc = ABC()
        has_abc = bool(gdi32.GetCharABCWidthsW(self.hdc, codepoint, codepoint, ctypes.byref(abc)))
        size = SIZE()
        gdi32.GetTextExtentPoint32W(self.hdc, ch, 1, ctypes.byref(size))
        xadv = int(abc.abcA + abc.abcB + abc.abcC) if has_abc else int(size.cx)
        if xadv <= 0:
            xadv = max(int(self.tm.tmAveCharWidth), 1)

        # Draw at (pad, 0) so negative A bearings still fit
        pad = max(8, abs(int(abc.abcA)) + 2 if has_abc else 8)
        self._clear()
        # Draw from top of cell; GDI places baseline at tmAscent from top when using TA_TOP
        gdi32.ExtTextOutW(self.hdc, pad, 0, 0, None, ch, 1, None)

        # Find ink bbox
        min_x, min_y = self.cell_w, self.cell_h
        max_x, max_y = -1, -1
        for y in range(self.cell_h):
            for x in range(self.cell_w):
                if self._pixel(x, y) > 8:
                    if x < min_x:
                        min_x = x
                    if y < min_y:
                        min_y = y
                    if x > max_x:
                        max_x = x
                    if y > max_y:
                        max_y = y

        if max_x < 0:
            # space / empty
            return None, 0, 0, xadv

        w = max_x - min_x + 1
        h = max_y - min_y + 1
        # xoffset relative to pen position (pad was draw x)
        xoff = min_x - pad
        yoff = min_y  # already from top of line cell

        pixels = []
        for y in range(min_y, max_y + 1):
            for x in range(min_x, max_x + 1):
                pixels.append(self._pixel(x, y))

        img = Image.new("L", (w, h))
        img.putdata(pixels)
        rgba = Image.new("RGBA", (w, h), (255, 255, 255, 0))
        rgba.putalpha(img)
        return rgba, int(xoff), int(yoff), int(xadv)


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
            print("    WARNING: atlas overflow")
            break
        atlas.paste(img, (cx, cy), img)
        placements[cid] = (cx, cy, w, h)
        cx += w + GLYPH_PAD
        row_h = max(row_h, h)
    return atlas, placements


def snap_shared_yoffset(glyph_imgs: dict, glyph_metrics: dict):
    """
    Pad transparent rows on top so every visible glyph shares the same yoffset
    (Kraken GameDefaultSm style: T/h/a tops align).
    """
    visible = [cid for cid, img in glyph_imgs.items() if img is not None]
    if not visible:
        return
    latin = [cid for cid in visible if (65 <= cid <= 90) or (97 <= cid <= 122)]
    pool = latin if latin else visible
    target = min(glyph_metrics[cid][1] for cid in pool)
    for cid in visible:
        xoff, yoff, xadv = glyph_metrics[cid]
        pad = yoff - target
        if pad > 0:
            img = glyph_imgs[cid]
            w, h = img.size
            padded = Image.new("RGBA", (w, h + pad), (255, 255, 255, 0))
            padded.paste(img, (0, pad), img)
            glyph_imgs[cid] = padded
        glyph_metrics[cid] = (xoff, target, xadv)


def write_fnt(path, face, size, bold, tm, glyphs, placements, png_name):
    line_height = int(tm.tmHeight + max(tm.tmExternalLeading, 0))
    base = int(tm.tmAscent)
    lines = [
        f'info face="{face}" size=-{size} bold={bold} italic=0 charset="" unicode=1 '
        f"stretchH=100 smooth=1 aa=1 padding={GLYPH_PAD},{GLYPH_PAD},{GLYPH_PAD},{GLYPH_PAD} "
        f"spacing=1,1 outline=0",
        f"common lineHeight={line_height} base={base} scaleW={ATLAS_SIZE} scaleH={ATLAS_SIZE} "
        f"pages=1 packed=0 alphaChnl=0 redChnl=4 greenChnl=4 blueChnl=4",
        f'page id=0 file="{png_name}"',
        f"chars count={len(glyphs)}",
    ]
    for cid in sorted(glyphs):
        xoff, yoff, xadv = glyphs[cid]
        if cid in placements:
            ax, ay, aw, ah = placements[cid]
        else:
            ax = ay = 0
            aw = ah = 0
        lines.append(
            f"char id={cid:<6} x={ax:<6} y={ay:<6} width={aw:<6} height={ah:<6} "
            f"xoffset={xoff:<6} yoffset={yoff:<6} xadvance={xadv:<6} page=0  chnl=15"
        )
    lines.append("kernings count=0")
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def bake_one(src_dir: Path, out_dir: Path, name: str, ttf_name: str, size: int, bold: int) -> bool:
    ttf = src_dir / ttf_name
    if not ttf.exists():
        print(f"  MISSING {ttf}")
        return False
    if "SemiBold" in ttf_name:
        weight = FW_SEMIBOLD
    elif "Bold" in ttf_name:
        weight = FW_BOLD
    else:
        weight = FW_NORMAL

    print(f"  [{name}] GDI {ttf_name} {size}px ... ", end="", flush=True)
    with GdiSession(ttf, size, weight, use_cleartype=False) as session:
        glyph_imgs = {}
        glyph_metrics = {}
        for cid in CHAR_IDS:
            img, xoff, yoff, xadv = session.glyph(cid)
            glyph_imgs[cid] = img
            glyph_metrics[cid] = (xoff, yoff, xadv)

        snap_shared_yoffset(glyph_imgs, glyph_metrics)
        atlas, placements = pack_glyphs(glyph_imgs)
        out_dir.mkdir(parents=True, exist_ok=True)
        png_name = f"{name}_0.png"
        atlas.save(out_dir / png_name)
        write_fnt(
            out_dir / f"{name}.fnt",
            session.face,
            size,
            bold,
            session.tm,
            glyph_metrics,
            placements,
            png_name,
        )

        def mh(ch):
            cid = ord(ch)
            yoff = glyph_metrics[cid][1]
            h = placements[cid][3] if cid in placements else 0
            return yoff, h

        ty, th = mh("T")
        hy, hh = mh("h")
        ay, ah = mh("a")
        print(
            f"OK face={session.face!r} lh={session.tm.tmHeight} base={session.tm.tmAscent} "
            f"| T y{ty}/h{th}  h y{hy}/h{hh}  a y{ay}/h{ah}"
        )
    return True


def main():
    script_dir = Path(__file__).resolve().parent
    src_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else script_dir / "opensans_src"
    out_dir = script_dir / "output_gdi"
    client = script_dir.parent.parent / "client" / "font"

    print("=" * 60)
    print("Open Sans GDI BMFont baker (Kraken-like metrics)")
    print("=" * 60)
    print(f"Source : {src_dir}")
    print(f"Output : {out_dir}")
    print()

    ok = 0
    for name, ttf, size, bold in FONT_JOBS:
        if bake_one(src_dir, out_dir, name, ttf, size, bold):
            ok += 1

    print(f"\nBaked {ok}/{len(FONT_JOBS)}")
    if client.is_dir() and ok:
        for name, *_ in FONT_JOBS:
            for fname in (f"{name}.fnt", f"{name}_0.png"):
                src = out_dir / fname
                if src.exists():
                    (client / fname).write_bytes(src.read_bytes())
        print(f"Deployed to {client}")
    return 0 if ok == len(FONT_JOBS) else 1


if __name__ == "__main__":
    raise SystemExit(main())
