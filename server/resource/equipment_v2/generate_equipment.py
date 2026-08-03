#!/usr/bin/env python3
"""Generate family-tier equipment rows into ItemInfo (IDs 10000+)."""
from __future__ import annotations

import copy
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # server/resource
ITEMINFO = ROOT / "ItemInfo.txt"
OUT_FRAGMENT = Path(__file__).resolve().parent / "ItemInfo_equipment_v2.txt"

# id = 10000 + tier_idx*100 + (family-1)*12 + slot
ID_BASE = 10000
TIERS = [10, 20, 30, 40, 50, 60, 70]

FAMILIES = [
    (1, "Guardian"),
    (2, "Royal"),
    (3, "Wind"),
    (4, "Hunter"),
    (5, "Shadow"),
    (6, "Sanctum"),
]

# slot definitions: name_suffix, type, able, need, jobs, template_ Prefer_ids, kind
# kind: weapon_sword/gs/bow/gun/staff/dagger, head, body, glove, boot, neck, ring
SLOTS = [
    ("Greatsword", 2, "9", "-1", "1,8", [15, 1372, 1381], "weapon_gs"),
    ("Sword", 1, "9,6", "-1", "1,9,10", [4, 1391, 2], "weapon_sword"),
    ("Bow", 3, "6", "-1", "2,11,12", [29, 28, 27], "weapon_bow"),
    ("Gun", 4, "9", "-1", "2,11,12", [39, 1408, 38], "weapon_gun"),
    ("Staff", 9, "9", "-1", "5,13,14", [100, 1430, 101], "weapon_staff"),
    ("Dagger", 7, "9", "-1", "4,16", [76, 1418, 1446], "weapon_dagger"),
    ("Cap", 20, "0", "-1", "1,2,3,4,5,8,9,10,12,13,14,16", [828, 1115, 2187], "head"),
    ("Body", 22, "2", "-1", "1,2,3,4,5,8,9,10,12,13,14,16", [295, 300, 310], "body"),
    ("Gloves", 23, "3", "-1", "1,2,3,4,5,8,9,10,12,13,14,16", [471, 476, 486], "glove"),
    ("Boots", 24, "4", "-1", "1,2,3,4,5,8,9,10,12,13,14,16", [647, 652, 662], "boot"),
    ("Necklace", 25, "5", "-1", "-1", [461, 739, 4691], "neck"),
    ("Ring", 26, "7,8", "-1", "-1", [324, 327, 328], "ring"),
]

# Max ATK targets by weapon family (median-ish), MnATK = ~0.82 * Mx
WEAPON_MX = {
    "weapon_sword":  {10: 55, 20: 85, 30: 105, 40: 120, 50: 140, 60: 155, 70: 170},
    "weapon_gs":     {10: 70, 20: 110, 30: 150, 40: 250, 50: 295, 60: 340, 70: 375},
    "weapon_bow":    {10: 60, 20: 100, 30: 135, 40: 275, 50: 300, 60: 340, 70: 350},
    "weapon_gun":    {10: 60, 20: 100, 30: 135, 40: 275, 50: 300, 60: 340, 70: 350},
    "weapon_staff":  {10: 70, 20: 125, 30: 170, 40: 210, 50: 245, 60: 280, 70: 305},
    "weapon_dagger": {10: 70, 20: 125, 30: 170, 40: 210, 50: 245, 60: 280, 70: 310},
}

# Total DEF budget for armor set pieces (head+body+glove+boot)
DEF_BUDGET = {10: 22, 20: 30, 30: 50, 40: 64, 50: 80, 60: 90, 70: 110}
DEF_SPLIT = {"head": 0.18, "body": 0.46, "glove": 0.18, "boot": 0.18}


def pair(v: int) -> str:
    return f"{v},{v}"


def pair_range(mid: int, spread: float = 0.06) -> str:
    lo = max(1, int(round(mid * (1 - spread))))
    hi = max(lo, int(round(mid * (1 + spread))))
    return f"{lo},{hi}"


def zero_pair() -> str:
    return "0,0"


def make_item_id(tier_idx: int, family_id: int, slot_idx: int) -> int:
    return ID_BASE + tier_idx * 100 + (family_id - 1) * 12 + slot_idx


def load_iteminfo(path: Path) -> tuple[str, list[str], dict[int, list[str]]]:
    header = ""
    rows: list[str] = []
    by_id: dict[int, list[str]] = {}
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.startswith("//") and "Item Name" in line:
                header = line.rstrip("\n")
                continue
            if line.startswith("//") or not line.strip():
                continue
            parts = line.rstrip("\n").split("\t")
            try:
                iid = int(parts[0])
            except ValueError:
                continue
            by_id[iid] = parts
            rows.append(line.rstrip("\n"))
    if not header:
        raise RuntimeError("ItemInfo header not found")
    return header, rows, by_id


def pick_template(by_id: dict[int, list[str]], prefer: list[int], item_type: int) -> list[str]:
    for iid in prefer:
        if iid in by_id and int(by_id[iid][10]) == item_type:
            return copy.deepcopy(by_id[iid])
    # fallback: any of type
    for parts in by_id.values():
        if int(parts[10]) == item_type:
            return copy.deepcopy(parts)
    raise RuntimeError(f"No template for type {item_type}")


def clear_combat_stats(p: list[str]) -> None:
    # Coefs are Str2Int (single); value stats are min,max pairs.
    for i in range(31, 52):
        p[i] = "0"
    for i in range(52, 74):
        p[i] = "0,0"
    p[74] = "0"  # left hand


def apply_weapon_stats(p: list[str], kind: str, tier: int) -> None:
    mx = WEAPON_MX[kind][tier]
    mn = int(round(mx * 0.82))
    p[60] = pair_range(mn)
    p[61] = pair_range(mx)
    p[74] = "1" if kind in ("weapon_sword", "weapon_dagger") else "0"


def apply_armor_stats(p: list[str], kind: str, tier: int) -> None:
    if kind in DEF_SPLIT:
        def_val = max(1, int(round(DEF_BUDGET[tier] * DEF_SPLIT[kind])))
        p[62] = pair(def_val)
    # jewelry: tiny shared HP so bases aren't empty
    if kind in ("neck", "ring"):
        hp = {10: 20, 20: 35, 30: 55, 40: 75, 50: 100, 60: 125, 70: 160}[tier]
        p[63] = pair(hp // (2 if kind == "ring" else 1))


def build_row(
    template: list[str],
    item_id: int,
    name: str,
    family_id: int,
    tier: int,
    slot: tuple,
    slot_idx: int,
) -> list[str]:
    suffix, typ, able, need, jobs, _prefer, kind = slot
    p = copy.deepcopy(template)
    # ensure width
    while len(p) < 95:
        p.append("0")
    p[0] = f"{item_id:04d}" if item_id < 10000 else str(item_id)
    p[1] = name
    p[10] = str(typ)
    p[11] = "10"  # prefix rate
    p[12] = str(family_id)  # set ID = family
    p[13] = "0"  # forge lv req
    p[14] = "7"  # stable
    p[15] = "1"
    p[16] = p[17] = p[18] = p[19] = "1"
    p[20] = "1"  # pile
    p[21] = "1"  # instance
    p[22] = str(1000 + tier * 250 + family_id * 10)
    p[23] = "1,2,3,4"
    p[24] = str(tier)
    p[25] = jobs
    p[26] = "0"
    p[27] = "0"
    p[28] = able
    p[29] = need
    p[30] = "0"
    clear_combat_stats(p)
    if kind.startswith("weapon_"):
        apply_weapon_stats(p, kind, tier)
        p[76] = "10000,10000"
        p[77] = "3"
        p[75] = "0,1000"
    else:
        apply_armor_stats(p, kind, tier)
        p[76] = "8000,8000"
        p[77] = "2" if kind in ("body", "head", "glove", "boot") else "1"
        p[75] = "0,1000"
    # description / remark
    p[92] = f"T{tier} {name} (Family set)"
    p[93] = "equipment_v2"
    # zero ship fields noise if needed - leave from template for non-ship
    return p


def main() -> None:
    header, rows, by_id = load_iteminfo(ITEMINFO)
    col_count = len(header.lstrip("/").split("\t"))

    generated: list[list[str]] = []
    for tier_idx, tier in enumerate(TIERS):
        for family_id, family_name in FAMILIES:
            for slot_idx, slot in enumerate(SLOTS):
                item_id = make_item_id(tier_idx, family_id, slot_idx)
                template = pick_template(by_id, slot[5], slot[1])
                name = f"{family_name} {slot[0]} T{tier}"
                generated.append(
                    build_row(template, item_id, name, family_id, tier, slot, slot_idx)
                )

    # write fragment
    with OUT_FRAGMENT.open("w", encoding="utf-8", newline="\n") as f:
        f.write("// equipment_v2 generated rows — DO NOT HAND EDIT; regenerate via generate_equipment.py\n")
        for p in generated:
            # pad/trim to header width
            if len(p) < col_count:
                p.extend(["0"] * (col_count - len(p)))
            f.write("\t".join(p[:col_count]) + "\n")

    # merge into ItemInfo: drop prior 10000-10699, append new
    new_rows = []
    removed = 0
    for line in rows:
        parts = line.split("\t")
        try:
            iid = int(parts[0])
        except ValueError:
            new_rows.append(line)
            continue
        if 10000 <= iid <= 10699:
            removed += 1
            continue
        new_rows.append(line)

    with ITEMINFO.open("w", encoding="utf-8", newline="\n") as f:
        f.write(header + "\n")
        for line in new_rows:
            f.write(line + "\n")
        f.write("// --- equipment_v2 family gear (10000+) ---\n")
        for p in generated:
            if len(p) < col_count:
                p.extend(["0"] * (col_count - len(p)))
            f.write("\t".join(p[:col_count]) + "\n")

    print(f"Generated {len(generated)} items")
    print(f"ID range {generated[0][0]} .. {generated[-1][0]}")
    print(f"Removed previous v2 rows: {removed}")
    print(f"Wrote {OUT_FRAGMENT}")
    print(f"Updated {ITEMINFO}")


if __name__ == "__main__":
    main()
