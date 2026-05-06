# PKO Server — Linux Setup Guide

Tested and verified setup guide for running the PKO server on Ubuntu Linux.
Last verified on **Ubuntu 24.04.4 LTS** with GCC 13.3.0, CMake 3.28.3.

---

## Architecture Overview

```
┌──────────────────────────────────────────────────────────┐
│  Your Windows PC (development)                           │
│  ├── source/source.sln  → Visual Studio builds           │
│  ├── client/             → Game client (Windows only)     │
│  └── server/             → Configs, Lua scripts, data     │
└──────────────────────────────────────────────────────────┘
         ▼  git push → git pull on Linux
┌──────────────────────────────────────────────────────────┐
│  Linux Server (production)                               │
│  ├── ~/pkodev/              → Git repo (source + data)    │
│  │   ├── source/            → CMake + GCC build           │
│  │   ├── server/            → Configs, resource, addons   │
│  │   └── database/          → SQL setup scripts           │
│  │                                                        │
│  └── ~/pkodev-server/       → Deploy directory            │
│      ├── AccountServer      ← compiled binary             │
│      ├── GateServer         ← compiled binary             │
│      ├── GroupServer        ← compiled binary             │
│      ├── GameServer         ← compiled binary             │
│      ├── *.cfg              ← server configs              │
│      ├── resource/          ← maps, scripts, data tables  │
│      ├── addons/            ← Lua server addons           │
│      └── LOG/               ← runtime logs                │
└──────────────────────────────────────────────────────────┘
```

**Ports used:**

| Service | Port | Exposed? | Purpose |
|---------|------|----------|---------|
| AccountServer | 1978 | Internal | GroupServer ↔ AccountServer |
| GroupServer | 1975 | Internal | GateServer ↔ GroupServer |
| GateServer | 1973 | **External** | Client connections |
| GateServer | 1971 | Internal | GameServer ↔ GateServer |
| GameServer | 1985 | Internal | Info channel to GateServer |
| SQL Server | 1433 | Internal | Database connections |

Only port **1973** needs to be exposed to the internet.

---

## Step 1: Provision the Server

### Minimum requirements
- **OS:** Ubuntu 24.04 LTS (tested) or Ubuntu 22.04 LTS
- **CPU:** 4 vCPUs recommended (2 minimum)
- **RAM:** 8 GB recommended (4 GB minimum)
- **Disk:** 20 GB

### Initial system setup

```bash
apt update && apt upgrade -y
```

---

## Step 2: Install Build Dependencies

```bash
apt install -y \
    build-essential \
    cmake \
    git \
    tmux \
    rsync \
    curl \
    wget

apt install -y \
    libunixodbc-dev \
    libssl-dev \
    libicu-dev \
    libluajit-5.1-dev \
    libbotan-2-dev
```

Verify versions:
```bash
gcc --version | head -1     # GCC 13.3.0 on Ubuntu 24.04
cmake --version | head -1   # cmake 3.28.3 on Ubuntu 24.04
```

---

## Step 3: Install SQL Server 2022

### Add Microsoft repos and install

```bash
# Import Microsoft GPG key
curl -fsSL https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor -o /usr/share/keyrings/microsoft-prod.gpg

# Add SQL Server 2022 repo
curl -fsSL https://packages.microsoft.com/config/ubuntu/$(lsb_release -rs)/mssql-server-2022.list | tee /etc/apt/sources.list.d/mssql-server-2022.list

# Add ODBC driver repo
curl -fsSL https://packages.microsoft.com/config/ubuntu/$(lsb_release -rs)/prod.list | tee /etc/apt/sources.list.d/mssql-release.list

apt update

# Install SQL Server
apt install -y mssql-server

# Install ODBC Driver 18 + command-line tools
ACCEPT_EULA=Y apt install -y msodbcsql18 mssql-tools18 unixodbc-dev
```

### Configure SQL Server

```bash
# Run interactive setup — choose option 2 (Developer edition)
/opt/mssql/bin/mssql-conf setup
```

When prompted:
1. Select **2) Developer** (free, full-featured)
2. Set the SA password (e.g. `YourSAPassword`)
3. Accept the license terms

### Add sqlcmd to PATH

```bash
echo 'export PATH="$PATH:/opt/mssql-tools18/bin"' >> ~/.bashrc
source ~/.bashrc
```

### Verify SQL Server is running

```bash
systemctl status mssql-server
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -Q "SELECT @@VERSION"
```

> **Note:** The `-C` flag trusts the self-signed certificate. Required for ODBC Driver 18.

### Verify ODBC driver

```bash
odbcinst -q -d
# Should output: [ODBC Driver 18 for SQL Server]
```

The driver config should be at `/etc/odbcinst.ini`:
```ini
[ODBC Driver 18 for SQL Server]
Description=Microsoft ODBC Driver 18 for SQL Server
Driver=/opt/microsoft/msodbcsql18/lib64/libmsodbcsql-18.6.so.1.1
UsageCount=1
```

---

## Step 4: Clone the Repository

```bash
cd ~
git clone https://github.com/YourOrg/pkodev.git
cd pkodev
git checkout devr-linux
```

---

## Step 5: Fix ICU Header Version Mismatch

The source tree bundles ICU 76 headers in `source/include/unicode/`, but Ubuntu 24.04 ships system ICU 74. The compiled `.so` is ICU 74, so the headers must match.

```bash
cd ~/pkodev/source/include

# Backup bundled ICU 76 headers
mv unicode unicode_bundled76

# Symlink to system ICU 74 headers
ln -s /usr/include/unicode unicode
```

Verify:
```bash
ls -la ~/pkodev/source/include/unicode
# Should show: unicode -> /usr/include/unicode
```

> **Why not just install ICU 76?** Ubuntu 24.04 repos only have ICU 74. Building ICU 76 from source would work but adds unnecessary complexity. The system ICU 74 is fully compatible at runtime.

---

## Step 6: Build the Server

### Using the build script

```bash
cd ~/pkodev/source/scripts
chmod +x *.sh
./build-linux.sh
```

### Or build manually

```bash
cd ~/pkodev/source
mkdir -p out/linux && cd out/linux
cmake ../.. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Verify build output

```bash
ls -lh ~/pkodev/source/out/linux/bin/
```

You should see 4 binaries:
```
AccountServer    (~3 MB)
GateServer       (~4 MB)
GroupServer       (~5 MB)
GameServer       (~18 MB)
```

---

## Step 7: Set Up the Database

### Run SQL scripts in order

The `database/` folder contains numbered SQL scripts. Run them **in order**:

```bash
cd ~/pkodev/database

# Core databases (required)
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[0]AccountServer.sql'
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[1]GameDB.sql'
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[2]Guild.sql'
```

### Create SQL logins

Edit `[3]Create SQL Login.sql` to set the passwords you want for the two service accounts, then run it:

```bash
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[3]Create SQL Login.sql'
```

This creates two SQL logins used by the server binaries:
- `pko_account` — Used by AccountServer (connects to `AccountServer` DB)
- `pko_game` — Used by GroupServer (connects to `GameDB`)

### Create first game account

```bash
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[4]Create Account.sql'
```

### Run remaining scripts

```bash
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[5]WebsiteDB.sql'
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[6]OfflineStalls.sql'
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[7]EconomyAnalytics.sql'
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[8]Gold64BitMigration.sql'
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[9]VerifyGold64Bit.sql'
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[10]EmergencyGoldFix.sql'
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[11]NewsSystem.sql'
sqlcmd -S localhost -U SA -P 'YourSAPassword' -C -i '[12]PlayerLoginInfo.sql'
```

---

## Step 8: Deploy the Server

### Using the deploy script

```bash
cd ~/pkodev/source/scripts
PKO_DEPLOY_DIR=~/pkodev-server ./deploy-linux.sh
```

The deploy script copies:
- 4 server binaries from `source/out/linux/bin/`
- `.cfg` files from `server/` (skips if already exist to preserve local edits)
- `resource/` directory from `server/`
- `.txt` data files from `server/`

### Copy files NOT handled by the deploy script

The deploy script doesn't copy addons, `.lic`, `.res`, or `cmd.cfg`. Copy them manually:

```bash
cd ~/pkodev

# Addons (Lua server scripts)
cp -r server/addons ~/pkodev-server/

# License, language, and config files
cp server/license.lic ~/pkodev-server/
cp server/en_US.res ~/pkodev-server/
cp server/cmd.cfg ~/pkodev-server/

# Create LOG and data directories
mkdir -p ~/pkodev-server/LOG/{AccountServer,GameServer,GateServer,GroupServer}
mkdir -p ~/pkodev-server/PlayerData
```

### Verify deploy directory

```bash
ls ~/pkodev-server/
```

Expected contents:
```
AccountServer         GameServer.cfg       TrustedIPs.txt
AccountServer.cfg     GateServer           addons/
BossLastHit.txt       GateServer.cfg       cmd.cfg
BossTimers.txt        GroupServer           en_US.res
BossTracked.txt       GroupServer.cfg      license.lic
ChaNameFilter.txt     LOG/                 resource/
GameServer            PlayerData/
```

---

## Step 9: Configure Server Files

The `.cfg` files shipped in the repo are set up for Windows. On Linux, you need to update database connection info and log paths.

### AccountServer.cfg

```ini
[Res]
log_dir = LOG/AccountServer

[db]
dbserver = localhost
db = AccountServer
userid = pko_account
passwd = YourDBPassword
```

### GroupServer.cfg

```ini
[Res]
log_dir = LOG/GroupServer

[Database]
IP = localhost
DB = GameDB
Login = pko_game
Password = YourDBPassword
```

### GateServer.cfg

```ini
[Res]
log_dir = LOG/GateServer

[GroupServer]
IP = 127.0.0.1
Port = 1975

[ToClient]
IP = 0.0.0.0
Port = 1973
```

### GameServer.cfg

```ini
[Gate]
gate = 127.0.0.1, 1971
info = 127.0.0.1, 1985, YourDBPassword, 1
```

> **Important:** All log paths must use **forward slashes** (`LOG/GameServer` not `LOG\GameServer`).

---

## Step 10: Open Firewall

Only port **1973** (GateServer — client connections) needs to be exposed:

```bash
ufw allow 22/tcp comment 'SSH'
ufw allow 1973/tcp comment 'PKO GateServer - Client'
ufw enable
ufw status
```

Expected output:
```
Status: active

To                         Action      From
--                         ------      ----
1973/tcp                   ALLOW       Anywhere       # PKO GateServer - Client
22/tcp                     ALLOW       Anywhere       # SSH
```

> **Do NOT expose** ports 1978, 1975, 1971, 1985, or 1433 to the internet.

---

## Step 11: Start the Servers

### Using tmux (recommended)

The servers must be started in this order: **AccountServer → GroupServer → GateServer → GameServer**.

```bash
cd ~/pkodev-server

# Create a tmux session
tmux new-session -d -s pko -n AccountServer

# Window 0: AccountServer
tmux send-keys -t pko:0 'cd ~/pkodev-server && ./AccountServer' Enter

# Wait for AccountServer to initialize
sleep 3

# Window 1: GroupServer
tmux new-window -t pko -n GroupServer
tmux send-keys -t pko:1 'cd ~/pkodev-server && ./GroupServer' Enter

# Wait for GroupServer to connect to AccountServer
sleep 3

# Window 2: GateServer
tmux new-window -t pko -n GateServer
tmux send-keys -t pko:2 'cd ~/pkodev-server && ./GateServer' Enter

# Wait for GateServer to connect to GroupServer
sleep 3

# Window 3: GameServer
tmux new-window -t pko -n GameServer
tmux send-keys -t pko:3 'cd ~/pkodev-server && ./GameServer' Enter
```

### tmux controls

```
tmux attach -t pko        # Attach to server session
Ctrl+B, 0                 # Switch to AccountServer window
Ctrl+B, 1                 # Switch to GroupServer window
Ctrl+B, 2                 # Switch to GateServer window
Ctrl+B, 3                 # Switch to GameServer window
Ctrl+B, D                 # Detach (servers keep running)
```

### Verify all servers are running

```bash
ss -tlnp | grep -E '1978|1975|1973|1971'
```

Expected output (4 listening ports):
```
LISTEN  0  128  0.0.0.0:1978  0.0.0.0:*  users:(("AccountServer",pid=...,fd=...))
LISTEN  0  128  0.0.0.0:1975  0.0.0.0:*  users:(("GroupServer",pid=...,fd=...))
LISTEN  0  128  0.0.0.0:1973  0.0.0.0:*  users:(("GateServer",pid=...,fd=...))
LISTEN  0  128  0.0.0.0:1971  0.0.0.0:*  users:(("GateServer",pid=...,fd=...))
```

---

## Step 12: Connect the Game Client

The game client runs on **Windows only**.

1. On your Windows PC, edit `client/system/Server.ini`
2. Set the server IP to your Linux server's public IP
3. Set the port to **1973**
4. Launch `Game.exe`

---

## Day-to-Day Operations

### Rebuild after code changes

```bash
# On Windows: commit and push
git add -A && git commit -m "fix: whatever" && git push

# On Linux: pull, rebuild, redeploy
cd ~/pkodev
git pull
cd source/out/linux
make -j$(nproc)
cd ~/pkodev/source/scripts
PKO_DEPLOY_DIR=~/pkodev-server ./deploy-linux.sh
```

Then restart the servers (kill and relaunch in tmux).

### Update scripts/data only (no recompile)

```bash
cd ~/pkodev
git pull

# Sync resource and addons to deploy dir
rsync -a server/resource/ ~/pkodev-server/resource/
rsync -a server/addons/ ~/pkodev-server/addons/
```

Then restart GameServer (it loads Lua scripts on startup).

### View logs

```bash
# Live tail GameServer log
tail -f ~/pkodev-server/LOG/GameServer/*.log

# Search all logs for errors
grep -ri "error\|fail" ~/pkodev-server/LOG/
```

### Stop all servers

```bash
pkill -f AccountServer
pkill -f GroupServer
pkill -f GateServer
pkill -f GameServer

# Or kill the tmux session
tmux kill-session -t pko
```

---

## Troubleshooting

### ODBC connection fails
```
[unixODBC][Driver Manager]Can't open lib 'ODBC Driver 18 for SQL Server'
```
**Fix:** Install or reinstall the ODBC driver:
```bash
ACCEPT_EULA=Y apt install -y msodbcsql18
odbcinst -q -d   # verify: [ODBC Driver 18 for SQL Server]
```

### SQL Server won't start
```bash
systemctl status mssql-server
journalctl -u mssql-server --no-pager -n 50
```
Common cause: not enough RAM (SQL Server needs at least 2 GB).

### "Address already in use" on server start
```bash
# Find what's using the port
ss -tlnp | grep -E '1973|1975|1978|1971'

# Kill stale server processes
pkill -f AccountServer; pkill -f GroupServer; pkill -f GateServer; pkill -f GameServer
```

### Build fails: ICU version mismatch
If you see errors about ICU symbols or `unicode/` headers:
```bash
# Ensure the symlink points to system ICU
ls -la ~/pkodev/source/include/unicode
# Should be: unicode -> /usr/include/unicode

# If not, fix it:
cd ~/pkodev/source/include
mv unicode unicode_bundled76 2>/dev/null
ln -s /usr/include/unicode unicode
```

### Build fails: missing dependencies
```bash
# Check what's installed
dpkg -l | grep -E 'luajit|botan|libicu|unixodbc|libssl'

# Install anything missing
apt install -y libunixodbc-dev libssl-dev libicu-dev libluajit-5.1-dev libbotan-2-dev
```

### GameServer: "Load map jump point info error!"
This means a map's `SwhMap.txt` file was not found. Check:
```bash
# Verify the map resource files exist
ls ~/pkodev-server/resource/map/garner/
# Should contain: garnerSwhMap.txt, garnerNPC.txt, etc.
```
If maps are listed in `GameServer.cfg` but have no resource directory, comment them out.

### Permission denied on binaries
```bash
chmod +x ~/pkodev-server/*Server
```

### GroupServer crashes (SIGSEGV)
This was caused by uninitialized database pool pointers. The fix is already committed in the `devr-linux` branch:
- `GroupServerAppServ.cpp`: Use `db->m_tblaccounts` and `db->m_tblcharaters` instead of bare `m_tblaccounts`/`m_tblcharaters`
- `GroupServerAppFrnd.cpp`: Use `db->m_tblfriends` instead of bare `m_tblfriends`

Make sure you're on the latest `devr-linux` branch.

---

## Quick Reference

```bash
# Build
cd ~/pkodev/source/out/linux && make -j$(nproc)

# Deploy
cd ~/pkodev/source/scripts && PKO_DEPLOY_DIR=~/pkodev-server ./deploy-linux.sh

# Start (in tmux)
tmux attach -t pko   # or create new session per Step 11

# Stop
pkill -f AccountServer; pkill -f GroupServer; pkill -f GateServer; pkill -f GameServer

# Check status
ss -tlnp | grep -E '1978|1975|1973|1971'

# Firewall
ufw status
```
