#!/usr/bin/env python3
import argparse
import re
from pathlib import Path

J_TAG_PATTERN = re.compile(r"<j([^>]*)>")
R_TAG_PATTERN = re.compile(r"<r([^>]*)>")

COORD_PATTERN = re.compile(r"^\s*\(?\s*([0-9]{1,4})\s*,\s*([0-9]{1,4})\s*\)?\s*$")
BOLD_AT_COORD_PATTERN = re.compile(r"<b([^>]+)>\s*at\s*<nav:coord:[0-9]{1,4}:[0-9]{1,4}>", re.IGNORECASE)
MONSTER_CONTEXT_PATTERN = re.compile(
    r"\b(kill|hunt|slay|defeat|destroy|capture|from|drop|drops|dropped|found|appears?|resides?|nearby|near|around|where)\b",
    re.IGNORECASE,
)
PROGRESS_PATTERN = re.compile(r"^\s*\d+\s*/\s*\d+\s*$")
MONSTER_COORD_TRAIL_PATTERN = re.compile(
    r"(<nav:monstername:[^>]+>)\s*"
    r"(?:"
    r"(?:can\s+(?:only\s+)?be\s+found|can\s+be\s+found|is\s+found|are\s+found|"
    r"found|appears?|resides?|located|seen|gather(?:ed|ing)?)\s*"
    r")?"
    r"(?:near|nearby|around|at|in)?\s*"
    r"<nav:coord:[0-9]{1,4}:[0-9]{1,4}>",
    re.IGNORECASE,
)
MONSTER_COORD_CLAUSE_PATTERN = re.compile(
    r"(?:,?\s*(?:they|these(?:\s+[a-z]+)?)?\s*"
    r"(?:can\s+(?:only\s+)?be\s+found|can\s+be\s+found|are\s+found|is\s+found|"
    r"found|appears?|resides?|located|seen)\s*(?:at|in|near|around)?\s*"
    r"<nav:coord:[0-9]{1,4}:[0-9]{1,4}>\.?)",
    re.IGNORECASE,
)
MONSTER_AT_COORD_SIMPLE_PATTERN = re.compile(
    r"\s+at\s+<nav:coord:[0-9]{1,4}:[0-9]{1,4}>",
    re.IGNORECASE,
)
R_MONSTER_AT_COORD_PATTERN = re.compile(
    r"<r([^>]+)>\s*"
    r"(?:"
    r"(?:can\s+(?:only\s+)?be\s+found|can\s+be\s+found|is\s+found|are\s+found|"
    r"found|appears?|resides?|located|seen|gather(?:ed|ing)?|from|at|near|nearby|around)\s*"
    r")?"
    r"(?:at|in|near|around)?\s*"
    r"<nav:coord:[0-9]{1,4}:[0-9]{1,4}>",
    re.IGNORECASE,
)


def convert_j_tag(match):
    inner = match.group(1)
    if not inner:
        return match.group(0)
    # coordinates form
    m = COORD_PATTERN.match(inner)
    if m:
        x, y = m.group(1), m.group(2)
        return f"<nav:coord:{x}:{y}>"
    # name form
    name = inner.strip()
    return f"<nav:{name}>"


def convert_monster_r_tags_line(line):
    direct_count = 0

    def direct_repl(match):
        nonlocal direct_count
        inner = match.group(1).strip()
        if not inner:
            return match.group(0)
        if PROGRESS_PATTERN.match(inner):
            return match.group(0)
        if not re.search(r"[A-Za-z]", inner):
            return match.group(0)
        direct_count += 1
        return f"<nav:monstername:{inner}|{inner}>"

    line = R_MONSTER_AT_COORD_PATTERN.sub(direct_repl, line)

    if not MONSTER_CONTEXT_PATTERN.search(line):
        return line, direct_count

    converted_count = 0

    def repl(match):
        nonlocal converted_count
        inner = match.group(1).strip()
        if not inner:
            return match.group(0)
        if inner.lower().startswith("nav:"):
            return match.group(0)
        if PROGRESS_PATTERN.match(inner):
            return match.group(0)
        if not re.search(r"[A-Za-z]", inner):
            return match.group(0)

        converted_count += 1
        return f"<nav:monstername:{inner}|{inner}>"

    converted_line = R_TAG_PATTERN.sub(repl, line)
    converted_line = MONSTER_COORD_TRAIL_PATTERN.sub(r"\1", converted_line)

    if "<nav:monstername:" in converted_line:
        converted_line = MONSTER_COORD_CLAUSE_PATTERN.sub("", converted_line)
        converted_line = MONSTER_AT_COORD_SIMPLE_PATTERN.sub("", converted_line)
        converted_line = re.sub(r"\s{2,}", " ", converted_line)
        converted_line = re.sub(r"\s+([,.;!?])", r"\1", converted_line)

    return converted_line, converted_count + direct_count


def process_file(path: Path, dry_run=False):
    data = path.read_text(encoding='utf-8', errors='replace')
    converted = J_TAG_PATTERN.sub(convert_j_tag, data)
    # Convert <bNPC> at <nav:coord:X:Y> into <nav:NPC> when present
    converted = BOLD_AT_COORD_PATTERN.sub(lambda m: f"<nav:{m.group(1).strip()}>", converted)
    monster_converted_count = 0
    new_lines = []
    for line in converted.splitlines(keepends=True):
        updated_line, count = convert_monster_r_tags_line(line)
        new_lines.append(updated_line)
        monster_converted_count += count
    converted = ''.join(new_lines)
    if converted != data:
        coord_only = []
        for m in J_TAG_PATTERN.finditer(data):
            inner = m.group(1).strip()
            if COORD_PATTERN.match(inner):
                coord_only.append(inner)
        if dry_run:
            print(f"{path}: would convert {len([*J_TAG_PATTERN.finditer(data)])} <j> tags")
            if monster_converted_count:
                print(f"  monster name tags converted: {monster_converted_count}")
            if coord_only:
                print(f"  coordinate-only tags found: {coord_only}")
        else:
            path.write_text(converted, encoding='utf-8')
            print(f"{path}: converted {len([*J_TAG_PATTERN.finditer(data)])} <j> tags")
            if monster_converted_count:
                print(f"  monster name tags converted: {monster_converted_count}")
            if coord_only:
                print(f"  coordinate-only tags found: {coord_only}")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert <j> quest tags to <nav> style in lua scripts.')
    parser.add_argument('target', nargs='?', default='server/resource/script', help='File or directory to process')
    parser.add_argument('--dry-run', action='store_true', help='Do not write changes')
    args = parser.parse_args()

    target_path = Path(args.target)
    files = []
    if target_path.is_dir():
        files = list(target_path.rglob('*.lua'))
    elif target_path.is_file():
        files = [target_path]
    else:
        raise FileNotFoundError(target_path)

    for f in files:
        process_file(f, dry_run=args.dry_run)
