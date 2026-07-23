#!/usr/bin/env python3
import paramiko
import sys

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
c = paramiko.SSHClient()
c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
c.connect("161.49.194.196", username="spcf-admin", password="@dmin123", timeout=30)


def run(cmd, t=120):
    i, o, e = c.exec_command(cmd, timeout=t)
    return (o.read() + e.read()).decode(errors="replace")


print("=== find stdafx ===")
print(run('find ~/pkodev/source -iname "stdafx.h" 2>/dev/null | head -30'))
print(run("ls ~/pkodev/source/include/GameServer/ | grep -i stda; ls ~/pkodev/source/src/gameserver/ | grep -i stda"))
print(run("ls -la ~/pkodev/source/include/util/util.h ~/pkodev/source/include/serversdk/UdpSocket.h 2>&1"))
print("=== fatals ===")
print(
    run(
        "grep 'fatal error:' /tmp/make3.log | sed 's/.*fatal error: //' | sort | uniq -c | sort -rn | head -40"
    )
)
print("=== include dirs for GameServer ===")
print(
    run(
        "grep -n include_directories ~/pkodev/source/CMakeLists.txt | head -40; "
        "grep -A30 'add_executable(GameServer' ~/pkodev/source/CMakeLists.txt | head -40"
    )
)
c.close()
