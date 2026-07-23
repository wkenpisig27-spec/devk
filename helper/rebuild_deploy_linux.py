#!/usr/bin/env python3
"""Create exact-case header links from all #includes, rebuild GameServer, deploy."""
import paramiko
import sys
import time

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
c = paramiko.SSHClient()
c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
c.connect("161.49.194.196", username="spcf-admin", password="@dmin123", timeout=30)


def run(cmd, timeout=1800):
    print("\n>>>>", cmd[:160].replace("\n", " "))
    i, o, e = c.exec_command(cmd, timeout=timeout)
    out = (o.read() + e.read()).decode(errors="replace")
    print(out[-14000:] if len(out) > 14000 else out)
    return out


SCRIPT = r'''
set -e
cd ~/pkodev/source
python3 <<'PY'
import os, re
from pathlib import Path
from collections import defaultdict

inc_re = re.compile(r'#\s*include\s*"([^"]+)"')

# Index real (non-symlink) headers by lowercase basename and by lowercase relpath
index_base = defaultdict(list)  # lower basename -> [Path]
index_rel = {}  # lower relpath from include/ -> Path
include_root = Path("include")
for p in include_root.rglob("*"):
    if not p.is_file() or p.is_symlink():
        continue
    if p.suffix.lower() not in {".h", ".hpp", ".inl"}:
        continue
    rel = p.relative_to(include_root).as_posix()
    index_rel[rel.lower()] = p
    index_base[p.name.lower()].append(p)

# Also index src tree headers (rare)
for p in Path("src").rglob("*"):
    if not p.is_file() or p.is_symlink():
        continue
    if p.suffix.lower() not in {".h", ".hpp", ".inl"}:
        continue
    index_base[p.name.lower()].append(p)

needed = set()
for root in [Path("src"), Path("include")]:
    for p in root.rglob("*"):
        if p.suffix.lower() not in {".c", ".cpp", ".h", ".hpp", ".inl"}:
            continue
        try:
            text = p.read_text(errors="ignore")
        except Exception:
            continue
        for m in inc_re.finditer(text):
            needed.add(m.group(1).replace("\\", "/"))

created = 0
unresolved = []
for inc in sorted(needed):
    name = Path(inc).name
    low_name = name.lower()
    low_inc = inc.lower()

    # Prefer exact relative match under include/
    real = index_rel.get(low_inc)
    if real is None:
        cands = index_base.get(low_name, [])
        if not cands:
            unresolved.append(inc)
            continue
        # Prefer GameServer/ for gameserver-looking includes
        prefer = [x for x in cands if "GameServer" in x.as_posix()]
        real = prefer[0] if prefer else cands[0]

    # Create sibling symlink with EXACT requested basename casing
    link = real.parent / name
    if link.exists() or link.is_symlink():
        continue
    if link.resolve() == real.resolve():
        continue
    try:
        link.symlink_to(real.name)
        created += 1
        print(f"LINK {link} -> {real.name}")
    except Exception as ex:
        print(f"FAIL {link}: {ex}")

print(f"created={created}")
# show unresolved bare headers (likely client/windows only)
bare = [u for u in unresolved if "/" not in u and u.lower().endswith(".h")]
print(f"unresolved_bare={len(bare)}")
for u in bare[:40]:
    print("UNRESOLVED", u)
PY

# Extra: all stdafx casings next to every stdafx.h
find include -iname stdafx.h ! -type l | while read -r f; do
  d=$(dirname "$f"); b=$(basename "$f")
  for alt in StdAfx.h Stdafx.h STDAFX.h stdAfx.h; do
    [ -e "$d/$alt" ] || ln -sf "$b" "$d/$alt"
  done
done
ls -la include/GameServer/stdafx.h include/GameServer/StdAfx.h include/GameServer/Stdafx.h
'''

print(run(SCRIPT))

print(
    run(
        """
cd ~/pkodev/source/out/linux
# continue compiling; loop until success or no progress
make -j$(nproc) GameServer > /tmp/make_gs3.log 2>&1
echo MAKE_EXIT:$?
ls -la bin/
echo ---FATALS---
grep 'fatal error:' /tmp/make_gs3.log | sed 's/.*fatal error: //' | sort | uniq -c | sort -rn | head -40 || echo NONE
echo ---UNDEF---
grep 'undefined reference' /tmp/make_gs3.log | head -20 || echo NONE
tail -15 /tmp/make_gs3.log
""",
        timeout=1800,
    )
)

# If still failing on fatals, print them and stop before deploy
out = run(
    """
BIN=~/pkodev/source/out/linux/bin
if [ -x "$BIN/GameServer" ] && [ -x "$BIN/AccountServer" ]; then
  cp -f "$BIN"/{AccountServer,GateServer,GroupServer,GameServer} ~/pkodev-server/
  chmod +x ~/pkodev-server/{AccountServer,GateServer,GroupServer,GameServer}
  echo DEPLOY_OK
  ls -la --time-style=long-iso ~/pkodev-server/{AccountServer,GateServer,GroupServer,GameServer}
else
  echo DEPLOY_SKIP
  ls -la "$BIN" || true
fi
"""
)

if "DEPLOY_OK" in out:
    print(
        run(
            """
for pane in AccountServer GroupServer GateServer GameServer; do
  tmux send-keys -t "pko:$pane" C-c 2>/dev/null || true
done
sleep 3
pkill -x AccountServer 2>/dev/null || true
pkill -x GroupServer 2>/dev/null || true
pkill -x GateServer 2>/dev/null || true
pkill -x GameServer 2>/dev/null || true
sleep 2
tmux send-keys -t 'pko:AccountServer' './AccountServer' Enter
sleep 2
tmux send-keys -t 'pko:GroupServer' './GroupServer' Enter
sleep 2
tmux send-keys -t 'pko:GateServer' './GateServer' Enter
sleep 2
tmux send-keys -t 'pko:GameServer' './GameServer' Enter
"""
        )
    )
    for _ in range(100):
        time.sleep(2)
        cap = run("tmux capture-pane -t 'pko:GameServer' -p -S -12")
        if "Character opcode registry" in cap:
            print("READY")
            break
    print(run("pgrep -a 'AccountServer|GateServer|GroupServer|GameServer' || true"))
    print(
        run(
            "ls -la --time-style=long-iso ~/pkodev-server/{AccountServer,GateServer,GroupServer,GameServer}; "
            "cd ~/pkodev && git log -1 --oneline"
        )
    )

c.close()
print("ALL_DONE")
