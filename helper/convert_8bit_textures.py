#!/usr/bin/env python3
"""
Unpack paletted 8-bit BMPs in client/texture to 24-bit or 32-bit BMP.

Keeps the original filename and pixel size so model references still work.
Packed/encrypted files (no BM header) are left alone.

32-bit files are uncompressed BI_RGB BMPs with opaque alpha, matching the
existing 32-bit textures in this client (BITMAPINFOHEADER, compression 0).

Original 8-bit files are copied to helper/texture-8bit-backup/ before overwrite.

Usage:
    python helper/convert_8bit_textures.py
    python helper/convert_8bit_textures.py --bits 32
    python helper/convert_8bit_textures.py --dry-run
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
from io import BytesIO
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SRC = REPO_ROOT / "client" / "texture"
DEFAULT_BACKUP = REPO_ROOT / "helper" / "texture-8bit-backup"
DEFAULT_REPORT = REPO_ROOT / "helper" / "texture-8bit-convert-report.txt"


def read_bmp_header(path: Path) -> tuple[bool, int, int] | None:
    """Return (is_bmp, bpp, compression) or None if unreadable."""
    try:
        with path.open("rb") as fh:
            hdr = fh.read(54)
    except OSError:
        return None
    if len(hdr) < 54 or hdr[:2] != b"BM":
        return (False, 0, 0)
    bpp = struct.unpack_from("<H", hdr, 28)[0]
    compression = struct.unpack_from("<I", hdr, 30)[0]
    return (True, bpp, compression)


def image_to_rgb_bytes(data: bytes) -> tuple[int, int, bytes]:
    """Decode BMP bytes to opaque RGB. Ignore palette alpha / transparency."""
    with Image.open(BytesIO(data)) as im:
        im.load()
        im.info.pop("transparency", None)
        if im.mode == "P":
            pal = list(im.getpalette() or [])
            pal = pal + [0] * max(0, 768 - len(pal))
            indexes = im.tobytes()
            width, height = im.size
            rgb = bytearray(len(indexes) * 3)
            for i, idx in enumerate(indexes):
                p = idx * 3
                o = i * 3
                rgb[o : o + 3] = pal[p : p + 3]
            return width, height, bytes(rgb)
        rgb = im.convert("RGB")
        return rgb.size[0], rgb.size[1], rgb.tobytes()


def save_bmp24(path: Path, width: int, height: int, rgb: bytes) -> None:
    image = Image.frombytes("RGB", (width, height), rgb)
    image.save(path, format="BMP")
    image.close()


def save_bmp32(path: Path, width: int, height: int, rgb: bytes) -> None:
    """Write BI_RGB 32-bit BMP (BGRA, alpha=255), same layout as existing client files."""
    stride = width * 4
    pixels = bytearray(stride * height)
    for y in range(height):
        src_y = height - 1 - y
        src_row = src_y * width * 3
        dst_row = y * stride
        for x in range(width):
            s = src_row + x * 3
            d = dst_row + x * 4
            pixels[d] = rgb[s + 2]
            pixels[d + 1] = rgb[s + 1]
            pixels[d + 2] = rgb[s]
            pixels[d + 3] = 255
    header_size = 54
    file_size = header_size + len(pixels)
    header = struct.pack(
        "<2sIHHIIiiHHIIiiII",
        b"BM",
        file_size,
        0,
        0,
        header_size,
        40,
        width,
        height,
        1,
        32,
        0,
        len(pixels),
        0,
        0,
        0,
        0,
    )
    path.write_bytes(header + pixels)


def convert_file(src: Path, dest: Path, bits: int) -> tuple[int, int]:
    # Read into memory first. Opening the on-disk BMP with Pillow on Windows
    # keeps a lock, so in-place overwrite then fails with Access Denied.
    width, height, rgb = image_to_rgb_bytes(src.read_bytes())
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_name(dest.name + ".converting.tmp")
    try:
        if bits == 32:
            save_bmp32(tmp, width, height, rgb)
        else:
            save_bmp24(tmp, width, height, rgb)
        os.replace(tmp, dest)
    except Exception:
        if tmp.exists():
            tmp.unlink()
        raise
    return width, height


def iter_bmps(root: Path):
    for dirpath, _, files in os.walk(root):
        for name in files:
            if name.lower().endswith(".bmp"):
                yield Path(dirpath) / name


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert paletted 8-bit BMPs to 24-bit or 32-bit BMP")
    parser.add_argument("--src", type=Path, default=DEFAULT_SRC)
    parser.add_argument("--backup", type=Path, default=DEFAULT_BACKUP)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--bits", type=int, choices=(24, 32), default=24)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    src_root = args.src.resolve()
    backup_root = args.backup.resolve()
    if not src_root.is_dir():
        print(f"Texture folder not found: {src_root}", file=sys.stderr)
        return 1

    converted: list[str] = []
    skipped_packed: list[str] = []
    skipped_other: list[str] = []
    failed: list[str] = []

    if args.bits == 32:
        targets = [backup_root / rel.relative_to(backup_root) for rel in iter_bmps(backup_root)]
        walk = []
        for backup_path in targets:
            rel = backup_path.relative_to(backup_root)
            walk.append((src_root / rel, backup_path, rel.as_posix()))
    else:
        walk = [(path, backup_root / path.relative_to(src_root), path.relative_to(src_root).as_posix()) for path in iter_bmps(src_root)]

    for path, backup_path, rel in walk:
        header = read_bmp_header(path) if path.exists() else None
        if header is None:
            failed.append(f"{rel}  (unreadable)")
            continue
        is_bmp, bpp, compression = header
        if not is_bmp:
            skipped_packed.append(rel)
            continue
        if args.bits == 32:
            if bpp == 32:
                skipped_other.append(f"{rel}  (already 32-bit)")
                continue
            if bpp not in (8, 24):
                skipped_other.append(f"{rel}  ({bpp}-bit)")
                continue
        elif bpp != 8:
            skipped_other.append(f"{rel}  ({bpp}-bit)")
            continue

        if args.dry_run:
            converted.append(f"{rel}  ({bpp}-bit -> {args.bits}-bit, compression={compression})")
            continue

        original = path.read_bytes()
        try:
            if args.bits == 24 and not backup_path.exists():
                backup_path.parent.mkdir(parents=True, exist_ok=True)
                backup_path.write_bytes(original)
            width, height = convert_file(path, path, args.bits)
            after = read_bmp_header(path)
            if after is None or not after[0] or after[1] != args.bits or after[2] != 0:
                raise RuntimeError(f"expected {args.bits}-bit BI_RGB BMP after convert, got {after}")
            converted.append(f"{rel}  {width}x{height}")
        except Exception as exc:
            failed.append(f"{rel}  ({exc})")
            try:
                path.write_bytes(original)
            except OSError:
                pass

    lines = [
        f"{args.bits}-bit BMP unpack report",
        f"source:  {src_root}",
        f"backup:  {backup_root}",
        f"bits:    {args.bits}",
        f"dry-run: {args.dry_run}",
        "",
        f"converted: {len(converted)}",
        f"skipped packed/encrypted: {len(skipped_packed)}",
        f"skipped other bit depths: {len(skipped_other)}",
        f"failed: {len(failed)}",
        "",
        "== converted ==",
        *converted,
        "",
        "== skipped packed/encrypted ==",
        *skipped_packed,
        "",
        "== skipped other bit depths ==",
        *skipped_other,
        "",
        "== failed ==",
        *failed,
        "",
    ]
    report_text = "\n".join(lines)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(report_text, encoding="utf-8")

    print(f"converted: {len(converted)}")
    print(f"skipped packed/encrypted: {len(skipped_packed)}")
    print(f"skipped other bit depths: {len(skipped_other)}")
    print(f"failed: {len(failed)}")
    print(f"report: {args.report}")
    if failed:
        print("failures:", file=sys.stderr)
        for item in failed[:20]:
            print(f"  {item}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
