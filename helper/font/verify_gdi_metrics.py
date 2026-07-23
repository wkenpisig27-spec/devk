#!/usr/bin/env python3
from pathlib import Path

client = Path(r"C:\Users\Ken\Desktop\Github\devk\client\font")


def dump(name):
    info, g = {}, {}
    for line in open(client / f"{name}.fnt", encoding="ascii", errors="ignore"):
        if line.startswith("common "):
            for p in line.split():
                if "=" in p:
                    k, v = p.split("=", 1)
                    info[k] = v
        if line.startswith("info "):
            for p in line.split():
                if p.startswith("face="):
                    info["face"] = p.split("=", 1)[1].strip('"')
        if line.startswith("char id="):
            p = dict(x.split("=", 1) for x in line.split() if "=" in x)
            g[int(p["id"])] = p
    print(f"=== {name} face={info.get('face')} lh={info.get('lineHeight')} base={info.get('base')}")
    for ch in "Thaesd":
        c = g[ord(ch)]
        y = int(c["yoffset"])
        h = int(c["height"])
        print(f"  '{ch}' yoff={y} h={h} bot={y+h} xadv={c['xadvance']}")
    # Success check: T and h same yoff
    ty, hy = int(g[ord("T")]["yoffset"]), int(g[ord("h")]["yoffset"])
    th, hh = int(g[ord("T")]["height"]), int(g[ord("h")]["height"])
    ok = ty == hy and abs(th - hh) <= 1
    print(f"  T/h align: {'PASS' if ok else 'FAIL'} (yoff {ty}/{hy}, h {th}/{hh})")


for n in ["gamedefaultsmsemibold", "gamedefaultsm", "gamedefaultmid", "gamedefaultmidsemibold"]:
    dump(n)
