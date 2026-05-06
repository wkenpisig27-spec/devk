# 🛡️ OVH Network Firewall Configuration Guide

Configuring the OVH Network Firewall is critical because it blocks attack traffic **before** it reaches your VPS. This saves your server's CPU and bandwidth.

## 1. Accessing the Firewall
1. Log in to the [OVH Control Panel](https://www.ovh.com/manager/).
2. Go to **Bare Metal Cloud** -> **IP**.
3. Find your server's IP address in the list.
4. Click the `...` menu next to the IP and select **Create Firewall** (or **Configure Firewall** if it exists).
5. Enable the firewall (status should be "Enabled").

## 2. Default Rule (Action on other rules)
*   **Rule:** Deny (Refuse) or Authorize?
*   **Recommendation:** **Refuse** (Deny all traffic by default).
    *   *Note: If you choose Refuse, you must explicitly allow everything you need (RDP, Game, Web).*
    *   *If you are unsure, start with "Authorize" and just block port 3389 specifically, but "Refuse" is much more secure.*

## 3. Required Rules (If Default is "Refuse")

Add these rules in this specific order (Priority 0-19):

| Priority | Action | Protocol | Source IP | Source Port | Destination Port | Description |
|----------|--------|----------|-----------|-------------|------------------|-------------|
| 0 | Authorize | TCP | Your Home IP | Any | 41592 | **Admin RDP Access** (Use your custom port) |
| 1 | Authorize | TCP | Any | Any | 7515 | **GateServer** (Game Login) |
| 2 | Authorize | TCP | Any | Any | 80 | Web Server (If installed) |
| 3 | Authorize | TCP | Any | Any | 443 | SSL Web Server (If installed) |
| 4 | Authorize | ICMP | Any | - | - | Allow Ping (Optional, good for monitoring) |
| 19 | Refuse | TCP | Any | Any | 3389 | **Block Default RDP** (Extra safety) |

## 4. Game Server Ports
Ensure you allow all ports your game uses. Typical PKO ports:
*   **GateServer:** 7515 (TCP) - *Critical for players*
*   **GroupServer:** 1973 (TCP) - *Usually internal only, don't expose unless needed*
*   **GameServer:** 1976 (TCP) - *Usually internal only*
*   **AccountServer:** 1971 (TCP) - *Internal only*

**Restricting Internal Ports:**
Use the "Refuse" default policy to effectively hide internal ports (Group/Game/Account) from the public internet. Only the GateServer needs to be public.

## 5. Anti-DDoS Game Protection & Armor
OVH offers specialized game protection (UDP/TCP).
1. Go to **IP** -> `...` -> **Configure Game Firewall** (if available for your plan).
2. Add a rule for your game ports (7515 TCP).
3. Select "Other" or "Custom" profile if "PKO" isn't listed.
4. This adds advanced filtering for game protocol attacks.
