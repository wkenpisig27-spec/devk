# How to Export/Import Windows Firewall Rules

It is good practice to backup your firewall rules before making major changes.

## Exporting Rules

Run this command in PowerShell (Admin):

```powershell
New-Item -ItemType Directory -Path "C:\Backups" -Force
Export-NetFirewallRule -PolicyStore ActiveStore -FilePath "C:\Backups\FirewallRules-$(Get-Date -Format 'yyyy-MM-dd').wfw"
```

## Importing Rules

**WARNING:** This will overwrite your current firewall configuration!

```powershell
Import-NetFirewallRule -PolicyStore ActiveStore -FilePath "C:\Backups\FirewallRules-2023-10-25.wfw"
```

## resetting Firewall to Defaults

If everything goes wrong and you need a clean slate:

```powershell
netsh advfirewall reset
```
*Note: This will delete ALL custom rules. You will need to re-run your security scripts.*
