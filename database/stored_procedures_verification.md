# Stored Procedures Verification

## Code vs Database Parameter Verification

Based on analysis of your codebase stored procedure calls vs database definitions:

### ✅ PHASE 1 - VERIFIED MATCHES (January 2026)

1. **CharacterInsert** - ✅ FIXED
   - Code: `(cha_name, &act_id, birth, map, look)`
   - DB: `(@cha_name VARCHAR(50), @act_id INT, @birth VARCHAR(20), @map VARCHAR(50), @look VARCHAR(2000))`
   - Status: ✅ Fixed - VARCHAR(2000) for look parameter

2. **CharacterZeroAddr** - ✅ OK
   - Code: No parameters
   - DB: No parameters

3. **CharacterSetAddr** - ✅ OK  
   - Code: `(&addr, &cha_id)` 
   - DB: `(@addr BIGINT, @cha_id INT)`

4. **CharacterUpdateInfo** - ✅ OK
   - Code: `(&l_icon, motto, &l_cha_id)`
   - DB: `(@icon SMALLINT, @motto VARCHAR(255), @cha_id INT)`

### ✅ PHASE 2 - SQL INJECTION FIXES (January 2026)

The following raw SQL queries have been converted to parameterized stored procedures:

| File | Function | Old (Vulnerable) | New (Secure) |
|------|----------|------------------|--------------|
| DataBaseCtrl.cpp | InsertUser | Raw INSERT with string concat | `{CALL dbo.InsertNewUser(?,?,?)}` |
| PlayerMgr.cpp | PlayerExit | Raw UPDATE (disabled) | Commented with SP guidance |
| GameDB.cpp | MapMask::SaveData | Raw UPDATE with dynamic column | `{CALL dbo.MapMaskSaveData(?,?,?)}` |
| GameDB.cpp | CreateOfflineStall | EXEC with string concat | `{CALL dbo.OfflineStall_Create(...)}` |
| GameDB.cpp | UpdateOfflineStallItems | EXEC with string concat | `{CALL dbo.OfflineStall_UpdateItems(?,?,?,?)}` |

### NEW STORED PROCEDURES ADDED

| Procedure Name | Database | Purpose |
|----------------|----------|---------|
| InsertNewUser | AccountServer | Create new user account securely |
| MapMaskSaveData | GameDB | Save map mask data with dynamic column |
| OfflineStall_Create | GameDB | Create offline stall with all parameters |
| OfflineStall_UpdateItems | GameDB | Update stall items securely |
| InitAllGuilds | GameDB | Get all guilds (no user input) |
| GetGuildMembers | GameDB | Get guild members by ID |
| GetAmphitheaterCaptainByMap | GameDB | Get amphitheater captains |
| GetAmphitheaterPromotionTeams | GameDB | Get promotion leaderboard |
| GetAmphitheaterReliveTeams | GameDB | Get relive leaderboard |
| InsertGameLog | GameDB | Insert game log entry |

### ⚠️ NEEDS VERIFICATION

Run this SQL query in your database to verify all procedures exist with correct parameters:

```sql
SELECT 
    p.name AS ProcedureName,
    p.type_desc,
    COUNT(pr.parameter_id) AS ParamCount,
    STRING_AGG(pr.name + ' ' + t.name + 
        CASE WHEN t.name IN ('varchar','char','nvarchar','nchar') 
             THEN '(' + CAST(pr.max_length as varchar) + ')' 
             ELSE '' END, ', ') WITHIN GROUP (ORDER BY pr.parameter_id) AS Parameters
FROM sys.procedures p
LEFT JOIN sys.parameters pr ON p.object_id = pr.object_id
LEFT JOIN sys.types t ON pr.system_type_id = t.system_type_id
WHERE p.name IN (
    'AddStatLog', 'SetDiscInfo', 'AccountSaveInsert', 'AccountSaveUpdateChaIds',
    'CharacterZeroAddr', 'CharacterSetAddr', 'CharacterInsert', 'CharacterUpdateInfo',
    'CharacterStartEstop', 'CharacterEndEstop', 'CharacterEstop', 'CharacterAddMoney',
    'CharacterDelEstop', 'CharacterBackupRow', 'FriendsAdd', 'FriendsDelete',
    'MasterAdd', 'MasterDelete', 'MasterFinish', 'GuildDisband', 'ParamSave'
)
GROUP BY p.name, p.type_desc, p.create_date, p.modify_date
ORDER BY p.name;
```

### CRITICAL STORED PROCEDURES TO CHECK:

**GroupServer calls these procedures:**
1. AddStatLog - 3 params (login, play, wgplay)
2. SetDiscInfo - 3 params (cli_ip, reason, actid)  
3. AccountSaveInsert - 2 params (act_name, cha_ids)
4. AccountSaveUpdateChaIds - 2 params (cha_ids, act_id)
5. CharacterInsert - 5 params (cha_name, act_id, birth, map, look)
6. CharacterZeroAddr - 0 params
7. CharacterSetAddr - 2 params (addr, cha_id)
8. CharacterUpdateInfo - 3 params (icon, motto, cha_id)
9. CharacterStartEstop - 1 param (cha_id)
10. CharacterEndEstop - 1 param (cha_id)
11. CharacterEstop - 2 params (times, cha_name)
12. CharacterAddMoney - 2 params (money, cha_id)
13. CharacterDelEstop - 1 param (cha_name)
14. CharacterBackupRow - 1 param (cha_id)
15. FriendsAdd - 2 params (cha_id1, cha_id2)
16. FriendsDelete - 2 params (cha_id1, cha_id2)
17. MasterAdd - 2 params (cha_id1, cha_id2)
18. MasterDelete - 2 params (cha_id1, cha_id2)
19. MasterFinish - 1 param (cha_id)
20. GuildDisband - 1 param (guild_id)
21. ParamSave - 10 params

If any of these procedures are missing or have wrong parameter counts/types, you'll get SQL errors.

### RECOMMENDATIONS:

1. Run the SQL query above to check all procedures exist
2. If any are missing, run `[1]GameDB.sql` to create them
3. Pay special attention to parameter order - C++ passes them positionally
4. VARCHAR lengths must be sufficient (especially look parameter = 2000 chars)

### STATUS: 
- ✅ CharacterInsert fixed (VARCHAR(2000) for look)
- ⚠️ Other procedures need verification against database