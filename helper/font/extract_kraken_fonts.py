#!/usr/bin/env python3
"""Extract font-related files from Kraken Online GPK0 packages."""

import os
import struct
import zlib
import sys

ROOT = r"C:\Program Files (x86)\Kraken Online Team\Kraken Online v.2.0.15\data\packages"
OUT = r"C:\Users\Ken\Desktop\Github\devk\helper\font\kraken_extract"

INTEREST = (
    "font", "opensans", "comic", "momstype", "gamedefault",
    ".fnt", ".ttf", ".otf", ".tga", ".png",
)


def decompress(data: bytes) -> bytes:
    for skip in (0, 2):
        try:
            return zlib.decompress(data[skip:])
        except Exception:
            pass
    try:
        return zlib.decompress(data, -zlib.MAX_WBITS)
    except Exception:
        return data


def parse_index(path: str, max_index_bytes: int = 8_000_000):
    """Walk GPK0 index entries from file start (same layout as Scripts.gpk)."""
    entries = []
    size = os.path.getsize(path)
    with open(path, "rb") as f:
        header = f.read(16)
        if header[:4] != b"GPK0":
            print(f"  not GPK0: {path}")
            return entries
        # Read a window of the start for index walking
        f.seek(0)
        blob = f.read(min(size, max_index_bytes))

    i = 16
    n = len(blob)
    while i + 4 < n:
        path_len = struct.unpack_from("<H", blob, i + 2)[0]
        if path_len < 1 or path_len > 260:
            i += 1
            continue
        path_start = i + 4
        path_end = path_start + path_len * 2
        if path_end + 24 > n:
            break
        if blob[path_start:path_start + 2] != b"/\x00":
            i += 1
            continue
        try:
            p = blob[path_start:path_end].decode("utf-16le")
        except Exception:
            i += 1
            continue
        if not p.startswith("/") or any(c in p for c in "\x00\n\r"):
            i += 1
            continue
        # basic path charset check
        if any(ord(c) < 32 or ord(c) > 126 for c in p):
            i += 1
            continue
        off, sz1, sz2 = struct.unpack_from("<QQQ", blob, path_end)
        if off >= size or sz1 == 0 or off + sz1 > size:
            i += 1
            continue
        entries.append((p, off, sz1, sz2))
        i = path_end + 24
    return entries


def extract_interesting(gpk_name: str):
    path = os.path.join(ROOT, gpk_name)
    if not os.path.exists(path):
        print(f"missing {gpk_name}")
        return
    print(f"=== {gpk_name} ({os.path.getsize(path)} bytes) ===")
    entries = parse_index(path)
    print(f"  index entries parsed: {len(entries)}")
    hits = [e for e in entries if any(k in e[0].lower() for k in INTEREST)]
    # Prefer font-ish hits
    hits = sorted(set(hits), key=lambda x: x[0])
    print(f"  interesting hits: {len(hits)}")
    for p, off, sz1, sz2 in hits[:80]:
        print(f"  {p}  off={off} comp={sz1} raw={sz2}")

    os.makedirs(OUT, exist_ok=True)
    with open(path, "rb") as f:
        for p, off, sz1, sz2 in hits:
            if not any(x in p.lower() for x in (
                "/fonts/", "opensans", "comic", "momstype", "gamedefault",
                ".fnt", ".ttf", "ui/font"
            )):
                continue
            f.seek(off)
            comp = f.read(sz1)
            data = decompress(comp)
            if sz2 and abs(len(data) - sz2) > 64 and len(data) == sz1:
                # maybe already raw
                pass
            safe = p.lstrip("/").replace("/", "_")
            out = os.path.join(OUT, safe)
            with open(out, "wb") as o:
                o.write(data)
            print(f"  EXTRACTED {safe} ({len(data)} bytes)")


def main():
    for name in ("Scripts.gpk", "Interface.gpk", "Patch0.gpk", "Textures.gpk"):
        extract_interesting(name)
    print("\n=== OUT DIR ===")
    for fn in sorted(os.listdir(OUT)):
        fp = os.path.join(OUT, fn)
        print(f"  {fn:60} {os.path.getsize(fp):8}")


if __name__ == "__main__":
    main()
