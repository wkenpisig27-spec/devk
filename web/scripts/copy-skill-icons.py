from pathlib import Path
import re
import shutil

root = Path(__file__).resolve().parents[2]
info = (root / "server/resource/SkillInfo.txt").read_text(encoding="utf-8", errors="replace").splitlines()
header = info[0].lstrip("/").split("\t")
icon_i = header.index("ICON")

by_name: dict[str, str] = {}
for line in info[1:]:
    if not line.strip() or line.startswith("//"):
        continue
    cols = line.split("\t")
    if len(cols) <= icon_i:
        continue
    name, icon = cols[1].strip(), cols[icon_i].strip()
    if not icon.endswith((".png", ".tga")):
        continue
    if name not in by_name:
        by_name[name] = icon
    elif by_name[name] in {"cutting.png", "0"} and icon.startswith("s"):
        by_name[name] = icon

aliases = {
    "Alga Entanglement": "Algae Entanglement",
    "Seamanship": "Sail Mastery",
    "Haul": "Fishing",
    "Deck Repair": "Repair",
    "Lookout": "Traversing",
    "Appraisal": "Analyze",
    "Bargain": "Set Stall",
    "Caravan": "Set Sail",
    "Ledger": "Analyze",
    "Gathering": "Woodcutting",
    "Workshop": "Manufacturing",
    "Mending": "Repair",
    "Blueprints": "Analyze",
    "Flagship": "Reinforce Ship",
    "Order": "Benediction",
    "Broadside": "Cannon Mastery",
    "Rally": "Angel Blessing",
    "Corner": "Set Stall",
    "Silent Partner": "Set Stall",
    "Credit": "Set Stall",
    "Embargo": "Set Stall",
    "Masterwork": "Manufacturing",
    "Shipwright": "Repair",
    "Calibrate": "Cannon Mastery",
}

classes = (root / "web/src/data/classes.ts").read_text(encoding="utf-8")
skill_names = list(dict.fromkeys(re.findall(r'\{ name: "([^"]+)", type:', classes)))

src_dir = root / "client/texture/icon"
out_dir = root / "web/public/skills"
out_dir.mkdir(parents=True, exist_ok=True)

mapping: dict[str, str] = {}
missing: list[str] = []
copied: set[str] = set()
for n in skill_names:
    key = aliases.get(n, n)
    icon = by_name.get(key)
    if not icon:
        missing.append(n)
        continue
    src = src_dir / icon
    if not src.exists():
        missing.append(f"{n} (file missing: {icon})")
        continue
    mapping[n] = icon
    if icon not in copied:
        shutil.copy2(src, out_dir / icon)
        copied.add(icon)

lines = [
    "/** Client skill PNGs from SkillInfo.txt ICON (texture/icon). */",
    "export const skillIcons: Record<string, string> = {",
]
for n, icon in mapping.items():
    lines.append(f'  {n!r}: "/skills/{icon}",')
lines.append("};")
lines.append("")
(root / "web/src/data/skillIcons.ts").write_text("\n".join(lines), encoding="utf-8")

print("mapped", len(mapping), "of", len(skill_names))
print("missing", missing)
print("copied", sorted(copied))
