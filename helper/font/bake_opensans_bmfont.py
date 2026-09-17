#!/usr/bin/env python3
"""Bake Open Sans BMFonts via AngelCode BMFont with native advances (no remetrics)."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SRC = ROOT / "opensans_src"
OUT = ROOT / "output"
CLIENT = ROOT.parent.parent / "client" / "font"
BMFONT = ROOT / "bmfont64.exe"

JOBS = [
    ("gamedefaultsm", "OpenSans-Regular.ttf", 12, "Open Sans", 0),
    ("gamedefaultsmsemibold", "OpenSans-SemiBold.ttf", 12, "Open Sans SemiBold", 0),
    ("gamedefaultsmblack", "OpenSans-Bold.ttf", 12, "Open Sans", 1),
    ("gamedefaultmid", "OpenSans-Regular.ttf", 14, "Open Sans", 0),
    ("gamedefaultmidsemibold", "OpenSans-SemiBold.ttf", 14, "Open Sans SemiBold", 0),
    ("gamedefaultmidblack", "OpenSans-Bold.ttf", 14, "Open Sans", 1),
    ("gamedefaultbig", "OpenSans-Regular.ttf", 20, "Open Sans", 0),
    ("gamedefaulthuge", "OpenSans-Regular.ttf", 28, "Open Sans", 0),
    ("titleblack", "OpenSans-ExtraBold.ttf", 28, "Open Sans ExtraBold", 0),
    ("splashblack", "OpenSans-ExtraBold.ttf", 40, "Open Sans ExtraBold", 0),
]

BMFC = """fileVersion=1
fontName={face}
fontFile={ttf}
charSet=0
fontSize=-{size}
aa=1
scaleH=100
useSmoothing=1
isBold={bold}
isItalic=0
useUnicode=1
disableBoxChars=1
outputInvalidCharGlyph=0
dontIncludeKerningPairs=0
useHinting=1
renderFromOutline=1
useClearType=0
paddingDown=1
paddingUp=1
paddingRight=1
paddingLeft=1
spacingHoriz=1
spacingVert=1
useFixedHeight=0
forceZero=0
widthPaddingFactor=0.00
outWidth=512
outHeight=512
outBitDepth=32
fontDescFormat=0
fourChnlPacked=0
textureFormat=png
textureCompression=0
alphaChnl=0
redChnl=4
greenChnl=4
blueChnl=4
invA=0
invR=0
invG=0
invB=0
outlineThickness=0
chars=32-126,160-255,8211-8212,8216-8217,8220-8221,8226,8230,8364
iconImages=
iconImages=
"""


def main() -> int:
    if not BMFONT.exists():
        print(f"ERROR: missing {BMFONT}")
        return 1
    OUT.mkdir(parents=True, exist_ok=True)
    ok = 0
    for name, ttf, size, face, bold in JOBS:
        ttf_path = SRC / ttf
        if not ttf_path.exists():
            print(f"MISSING {ttf}")
            continue
        cfg = ROOT / f"temp_{name}.bmfc"
        cfg.write_text(
            BMFC.format(face=face, ttf=ttf_path.as_posix(), size=size, bold=bold),
            encoding="ascii",
        )
        fnt = OUT / f"{name}.fnt"
        print(f"[{name}] ...", end=" ", flush=True)
        subprocess.run([str(BMFONT), "-c", str(cfg), "-o", str(fnt)], check=False)
        if not fnt.exists():
            print("FAIL")
            continue
        print("OK")
        ok += 1
        if CLIENT.is_dir():
            for fname in (f"{name}.fnt", f"{name}_0.png"):
                (CLIENT / fname).write_bytes((OUT / fname).read_bytes())
    print(f"Done {ok}/{len(JOBS)} (native advances, deployed to {CLIENT})")
    return 0 if ok == len(JOBS) else 1


if __name__ == "__main__":
    raise SystemExit(main())
