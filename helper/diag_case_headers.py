#!/usr/bin/env python3
import sys, paramiko
sys.stdout.reconfigure(encoding="utf-8", errors="replace")
c = paramiko.SSHClient()
c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
c.connect("161.49.194.196", username="spcf-admin", password="@dmin123", timeout=30)
def run(cmd, timeout=120):
    stdin, stdout, stderr = c.exec_command(cmd, timeout=timeout, get_pty=True)
    return (stdout.read() + stderr.read()).decode(errors="replace")

print(run("cd ~/pkodev && git ls-files 'source/include/util/*' | head -60"))
print("---")
print(run("cd ~/pkodev && git ls-files | grep -iE 'util\\.h|point\\.h|Util\\.h|Point\\.h' | head -30"))
print("---")
print(run("ls -la ~/pkodev/source/include/util/ | grep -i util; ls -la ~/pkodev/source/include/serversdk/ | grep -i point; find ~/pkodev/source/include -iname 'util.h' -o -iname 'point.h' 2>/dev/null"))
print("--- SectionData include ---")
print(run("head -20 ~/pkodev/source/include/util/SectionData.h; head -20 ~/pkodev/source/include/common/EventRecord.h"))
c.close()
