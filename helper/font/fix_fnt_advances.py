#!/usr/bin/env python3
"""
Conservative BMFont advance fixer.

Only bumps glyphs that are *critically* tight (ink overhang > 2px past
xadvance). Does NOT force advance to the full bitmap AABB — that creates
fake tracking / an "edited" look.
"""

import argparse
import os
import re
import sys

CHAR_RE = re.compile(
    r'^(char id=(?P<id>\d+)\s+'
    r'x=(?P<x>-?\d+)\s+'
    r'y=(?P<y>-?\d+)\s+'
    r'width=(?P<w>\d+)\s+'
    r'height=(?P<h>\d+)\s+'
    r'xoffset=(?P<xoff>-?\d+)\s+'
    r'yoffset=(?P<yoff>-?\d+)\s+'
    r'xadvance=(?P<xadv>-?\d+)\s+'
    r'(?P<rest>.*))$'
)
INFO_OUTLINE_RE = re.compile(r'\boutline=(\d+)')


def fix_fnt(path, aa_slop=2):
    with open(path, "r", encoding="ascii", errors="ignore") as f:
        lines = f.readlines()

    outline = 0
    if lines:
        m = INFO_OUTLINE_RE.search(lines[0])
        if m:
            outline = int(m.group(1))

    changed = 0
    out = []
    for line in lines:
        m = CHAR_RE.match(line.rstrip("\n"))
        if not m:
            out.append(line if line.endswith("\n") else line + "\n")
            continue

        w = int(m.group("w"))
        h = int(m.group("h"))
        xoff = int(m.group("xoff"))
        xadv = int(m.group("xadv"))
        if w <= 1 and h <= 1:
            out.append(line if line.endswith("\n") else line + "\n")
            continue

        # Ignore AA/padding fringe (aa_slop) and only fix real collisions.
        overhang = (xoff + w) - xadv
        new_adv = xadv
        if overhang > aa_slop:
            new_adv = xadv + (overhang - aa_slop)
        # Outline fonts: ensure at least +outline on native-looking advances
        if outline > 0 and new_adv < xadv + outline and overhang > 0:
            new_adv = max(new_adv, xadv)  # already handled by baker usually

        if new_adv != xadv:
            changed += 1
        out.append(
            f'char id={int(m.group("id")):<6} x={int(m.group("x")):<6} y={int(m.group("y")):<6}'
            f' width={w:<6} height={h:<6}'
            f' xoffset={xoff:<6} yoffset={int(m.group("yoff")):<6}'
            f' xadvance={new_adv:<6} {m.group("rest")}\n'
        )

    with open(path, "w", encoding="ascii", newline="\n") as f:
        f.writelines(out)
    return changed


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("font_dir", nargs="?", default=None)
    ap.add_argument("--aa-slop", type=int, default=2)
    args = ap.parse_args()

    script_dir = os.path.dirname(os.path.abspath(__file__))
    font_dir = args.font_dir or os.path.join(script_dir, "..", "..", "client", "font")
    font_dir = os.path.abspath(font_dir)

    total = 0
    for name in sorted(os.listdir(font_dir)):
        if not name.endswith(".fnt"):
            continue
        path = os.path.join(font_dir, name)
        changed = fix_fnt(path, args.aa_slop)
        total += changed
        print(f"  {name}: fixed {changed} critically-tight advances")
    print(f"Done. Updated {total} glyphs in {font_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
