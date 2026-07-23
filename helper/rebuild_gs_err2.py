#!/usr/bin/env python3
import sys, paramiko
sys.stdout.reconfigure(encoding="utf-8", errors="replace")
c = paramiko.SSHClient()
c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
c.connect("161.49.194.196", username="spcf-admin", password="@dmin123", timeout=30)
def run(cmd, timeout=300):
    stdin, stdout, stderr = c.exec_command(cmd, timeout=timeout, get_pty=True)
    return (stdout.read() + stderr.read()).decode(errors="replace")

print("=== Util/Common errors ===")
print(run("grep -E 'error:|fatal error:' /tmp/pko_rebuild.log | head -50"))
print("=== rebuild Util alone ===")
print(run("cd ~/pkodev/source/out/linux && make Util 2>&1 | grep -E 'error:|Built target|Error' | head -40", timeout=180))
print("=== check util headers ===")
print(run("ls ~/pkodev/source/include/util/ | head -40; ls ~/pkodev/source/src/util/ | head -20"))
print("=== unicode symlink ===")
print(run("ls -la ~/pkodev/source/include/unicode 2>&1 | head -5"))
c.close()
