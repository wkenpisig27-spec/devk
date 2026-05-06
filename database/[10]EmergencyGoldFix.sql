-- =============================================
-- EMERGENCY Gold Corruption Fix Script
-- PKODev - January 29, 2026
-- =============================================
-- This script fixes:
-- 1. Remaining stored procedures with INT instead of BIGINT
-- 2. Corrupted gold values in character table
-- =============================================

USE GameDB;
GO

PRINT '============================================='
PRINT 'EMERGENCY GOLD CORRUPTION FIX'
PRINT '============================================='
PRINT ''

-- =============================================
-- STEP 1: Show corrupted characters BEFORE fix
-- =============================================
PRINT 'STEP 1: Characters with corrupted gold values:'
PRINT '-----------------------------------------------'

SELECT 
    cha_id,
    cha_name,
    gd AS CorruptedGold,
    CASE 
        WHEN gd < 0 THEN 'NEGATIVE (will reset to 0)'
        WHEN gd > 100000000000 THEN 'OVER 100B (will cap at 100B)'
        ELSE 'Normal'
    END AS Issue
FROM character
WHERE gd < 0 OR gd > 100000000000
ORDER BY gd DESC;

PRINT ''

-- =============================================
-- STEP 2: Fix SaveAllData stored procedure
-- =============================================
PRINT 'STEP 2: Fixing SaveAllData stored procedure...'

IF OBJECT_ID('dbo.SaveAllData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveAllData
GO

CREATE PROCEDURE [dbo].[SaveAllData]
    @_HP INT,
    @_SP INT,
    @_EXP VARCHAR(32),
    @_MAP VARCHAR(50),
    @_MAIN_MAP VARCHAR(50),
    @_MAP_X INT,
    @_MAP_Y INT,
    @_RADIUS INT,
    @_ANGLE SMALLINT,
    @_PK_CTRL TINYINT,
    @_DEGREE SMALLINT,
    @_JOB VARCHAR(50),
    @_GD BIGINT,              -- FIXED: Changed from INT to BIGINT
    @_AP INT,
    @_TP INT,
    @_STR INT,
    @_DEX INT,
    @_AGI INT,
    @_CON INT,
    @_STA INT,
    @_LUK INT,
    @_LOOK VARCHAR(2000),
    @_SKILLBAG VARCHAR(1500),
    @_SHORTCUT VARCHAR(1500),
    @_MISSION VARCHAR(MAX),
    @_MISRECORD VARCHAR(MAX),
    @_MISTRIGGER VARCHAR(MAX),
    @_MISCOUNT VARCHAR(512),
    @_BIRTH VARCHAR(50),
    @_LOGIN_CHA VARCHAR(50),
    @_SAIL_LV INT,
    @_SAIL_EXP INT,
    @_SAIL_LEFT_EXP INT,
    @_LIVE_LV INT,
    @_LIVE_EXP INT,
    @_LIVE_TP INT,
    @_KB_LOCKED INT,
    @_CREDIT INT,
    @_STORE_ITEM INT,
    @_SKILL_STATE VARCHAR(1024),
    @_EXTEND VARCHAR(MAX),
    @_IMP INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    -- Cap gold at 100 billion
    DECLARE @safeGold BIGINT;
    SET @safeGold = @_GD;
    IF @safeGold > 100000000000
        SET @safeGold = 100000000000;
    IF @safeGold < 0
        SET @safeGold = 0;
    
    UPDATE character SET
        hp = @_HP, sp = @_SP, exp = CAST(@_EXP AS BIGINT), 
        map = @_MAP, main_map = @_MAIN_MAP, map_x = @_MAP_X, map_y = @_MAP_Y, 
        radius = @_RADIUS, angle = @_ANGLE, pk_ctrl = @_PK_CTRL, degree = @_DEGREE,
        job = @_JOB, gd = @safeGold, ap = @_AP, tp = @_TP, 
        str = @_STR, dex = @_DEX, agi = @_AGI, con = @_CON, sta = @_STA, luk = @_LUK, 
        look = @_LOOK, skillbag = @_SKILLBAG, shortcut = @_SHORTCUT, 
        mission = @_MISSION, misrecord = @_MISRECORD, mistrigger = @_MISTRIGGER, miscount = @_MISCOUNT, 
        birth = @_BIRTH, login_cha = @_LOGIN_CHA,
        sail_lv = @_SAIL_LV, sail_exp = @_SAIL_EXP, sail_left_exp = @_SAIL_LEFT_EXP, 
        live_lv = @_LIVE_LV, live_exp = @_LIVE_EXP, live_tp = @_LIVE_TP, 
        kb_locked = @_KB_LOCKED, credit = @_CREDIT, store_item = @_STORE_ITEM, 
        skill_state = @_SKILL_STATE, extend = @_EXTEND, IMP = @_IMP
    WHERE cha_id = @_CHA_ID;
END
GO

PRINT '  -> SaveAllData FIXED with BIGINT @_GD parameter'

-- =============================================
-- STEP 3: Fix SaveAllDataWithoutPos stored procedure
-- =============================================
PRINT 'STEP 3: Fixing SaveAllDataWithoutPos stored procedure...'

IF OBJECT_ID('dbo.SaveAllDataWithoutPos', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveAllDataWithoutPos
GO

CREATE PROCEDURE [dbo].[SaveAllDataWithoutPos]
    @_HP INT,
    @_SP INT,
    @_EXP VARCHAR(32),
    @_RADIUS INT,
    @_PK_CTRL TINYINT,
    @_DEGREE SMALLINT,
    @_JOB VARCHAR(50),
    @_GD BIGINT,              -- FIXED: Changed from INT to BIGINT
    @_AP INT,
    @_TP INT,
    @_STR INT,
    @_DEX INT,
    @_AGI INT,
    @_CON INT,
    @_STA INT,
    @_LUK INT,
    @_LOOK VARCHAR(2000),
    @_SKILLBAG VARCHAR(1500),
    @_SHORTCUT VARCHAR(1500),
    @_MISSION VARCHAR(MAX),
    @_MISRECORD VARCHAR(MAX),
    @_MISTRIGGER VARCHAR(MAX),
    @_MISCOUNT VARCHAR(512),
    @_BIRTH VARCHAR(50),
    @_LOGIN_CHA VARCHAR(50),
    @_SAIL_LV INT,
    @_SAIL_EXP INT,
    @_SAIL_LEFT_EXP INT,
    @_LIVE_LV INT,
    @_LIVE_EXP INT,
    @_LIVE_TP INT,
    @_KB_LOCKED INT,
    @_CREDIT INT,
    @_STORE_ITEM INT,
    @_SKILL_STATE VARCHAR(1024),
    @_EXTEND VARCHAR(MAX),
    @_IMP INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    -- Cap gold at 100 billion
    DECLARE @safeGold BIGINT;
    SET @safeGold = @_GD;
    IF @safeGold > 100000000000
        SET @safeGold = 100000000000;
    IF @safeGold < 0
        SET @safeGold = 0;
    
    UPDATE character SET
        hp = @_HP, sp = @_SP, exp = CAST(@_EXP AS BIGINT), 
        radius = @_RADIUS, pk_ctrl = @_PK_CTRL, degree = @_DEGREE,
        job = @_JOB, gd = @safeGold, ap = @_AP, tp = @_TP, 
        str = @_STR, dex = @_DEX, agi = @_AGI, con = @_CON, sta = @_STA, luk = @_LUK, 
        look = @_LOOK, skillbag = @_SKILLBAG, shortcut = @_SHORTCUT, 
        mission = @_MISSION, misrecord = @_MISRECORD, mistrigger = @_MISTRIGGER, miscount = @_MISCOUNT, 
        birth = @_BIRTH, login_cha = @_LOGIN_CHA,
        sail_lv = @_SAIL_LV, sail_exp = @_SAIL_EXP, sail_left_exp = @_SAIL_LEFT_EXP, 
        live_lv = @_LIVE_LV, live_exp = @_LIVE_EXP, live_tp = @_LIVE_TP, 
        kb_locked = @_KB_LOCKED, credit = @_CREDIT, store_item = @_STORE_ITEM, 
        skill_state = @_SKILL_STATE, extend = @_EXTEND, IMP = @_IMP
    WHERE cha_id = @_CHA_ID;
END
GO

PRINT '  -> SaveAllDataWithoutPos FIXED with BIGINT @_GD parameter'

-- =============================================
-- STEP 4: Fix GuildUpdateBankGold stored procedure
-- =============================================
PRINT 'STEP 4: Fixing GuildUpdateBankGold stored procedure...'

IF OBJECT_ID('dbo.GuildUpdateBankGold', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildUpdateBankGold
GO

CREATE PROCEDURE [dbo].[GuildUpdateBankGold]
    @money BIGINT,            -- FIXED: Changed from INT to BIGINT
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @currentGold BIGINT;
    DECLARE @newGold BIGINT;
    
    SELECT @currentGold = gold FROM guild WITH (ROWLOCK) WHERE guild_id = @guild_id;
    SET @newGold = @currentGold + @money;
    
    -- Cap at 100 billion, floor at 0
    IF @newGold > 100000000000
        SET @newGold = 100000000000;
    IF @newGold < 0
        SET @newGold = 0;
    
    UPDATE guild SET gold = @newGold WHERE guild_id = @guild_id;
END
GO

PRINT '  -> GuildUpdateBankGold FIXED with BIGINT @money parameter'

-- =============================================
-- STEP 4B: Fix AddMoney stored procedure
-- =============================================
PRINT 'STEP 4B: Fixing AddMoney stored procedure...'

IF OBJECT_ID('dbo.AddMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AddMoney
GO

CREATE PROCEDURE [dbo].[AddMoney]
    @money BIGINT,            -- FIXED: Changed from INT to BIGINT
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @newGold BIGINT;
    DECLARE @currentGold BIGINT;
    
    SELECT @currentGold = gd FROM character WHERE cha_id = @cha_id;
    SET @newGold = @currentGold + @money;
    
    -- Cap at 100 billion, floor at 0
    IF @newGold > 100000000000
        SET @newGold = 100000000000;
    IF @newGold < 0
        SET @newGold = 0;
    
    UPDATE character SET gd = @newGold WHERE cha_id = @cha_id;
END
GO

PRINT '  -> AddMoney FIXED with BIGINT @money parameter'

-- =============================================
-- STEP 4C: Fix AddCharacterMoney stored procedure
-- =============================================
PRINT 'STEP 4C: Fixing AddCharacterMoney stored procedure...'

IF OBJECT_ID('dbo.AddCharacterMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AddCharacterMoney
GO

CREATE PROCEDURE [dbo].[AddCharacterMoney]
    @chaId INT,
    @money BIGINT            -- FIXED: Changed from INT to BIGINT
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @newGold BIGINT;
    DECLARE @currentGold BIGINT;
    
    SELECT @currentGold = gd FROM character WHERE cha_id = @chaId;
    SET @newGold = @currentGold + @money;
    
    -- Cap at 100 billion, floor at 0
    IF @newGold > 100000000000
        SET @newGold = 100000000000;
    IF @newGold < 0
        SET @newGold = 0;
    
    UPDATE character SET gd = @newGold WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

PRINT '  -> AddCharacterMoney FIXED with BIGINT @money parameter'

-- =============================================
-- STEP 4D: Fix GuildChallengeUpdate stored procedure
-- =============================================
PRINT 'STEP 4D: Fixing GuildChallengeUpdate stored procedure...'

IF OBJECT_ID('dbo.GuildChallengeUpdate', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildChallengeUpdate
GO

CREATE PROCEDURE [dbo].[GuildChallengeUpdate]
    @chall_id INT,
    @chall_money BIGINT,      -- FIXED: Changed from INT to BIGINT
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    -- Cap challenge money at 100 billion
    DECLARE @safeMoney BIGINT;
    SET @safeMoney = @chall_money;
    IF @safeMoney > 100000000000
        SET @safeMoney = 100000000000;
    IF @safeMoney < 0
        SET @safeMoney = 0;
    
    UPDATE guild SET challid = @chall_id, challmoney = @safeMoney 
    WHERE guild_id = @guild_id AND challmoney < @safeMoney AND challstart = 0;
END
GO

PRINT '  -> GuildChallengeUpdate FIXED with BIGINT @chall_money parameter'

-- =============================================
-- STEP 5: Fix corrupted character gold values
-- =============================================
PRINT ''
PRINT 'STEP 5: Fixing corrupted character gold values...'

-- Count affected characters
DECLARE @negativeCount INT;
DECLARE @overCapCount INT;

SELECT @negativeCount = COUNT(*) FROM character WHERE gd < 0;
SELECT @overCapCount = COUNT(*) FROM character WHERE gd > 100000000000;

PRINT '  -> Characters with negative gold: ' + CAST(@negativeCount AS VARCHAR(10));
PRINT '  -> Characters over 100B cap: ' + CAST(@overCapCount AS VARCHAR(10));

-- Fix negative gold (set to 0)
UPDATE character SET gd = 0 WHERE gd < 0;
PRINT '  -> Reset ' + CAST(@@ROWCOUNT AS VARCHAR(10)) + ' characters with negative gold to 0';

-- Cap gold at 100 billion
UPDATE character SET gd = 100000000000 WHERE gd > 100000000000;
PRINT '  -> Capped ' + CAST(@@ROWCOUNT AS VARCHAR(10)) + ' characters to 100 billion';

-- =============================================
-- STEP 6: Fix corrupted guild gold values
-- =============================================
PRINT ''
PRINT 'STEP 6: Fixing corrupted guild gold values...'

UPDATE guild SET gold = 0 WHERE gold < 0;
PRINT '  -> Reset ' + CAST(@@ROWCOUNT AS VARCHAR(10)) + ' guilds with negative gold to 0';

UPDATE guild SET gold = 100000000000 WHERE gold > 100000000000;
PRINT '  -> Capped ' + CAST(@@ROWCOUNT AS VARCHAR(10)) + ' guilds to 100 billion';

-- =============================================
-- STEP 7: Final verification
-- =============================================
PRINT ''
PRINT '============================================='
PRINT 'VERIFICATION COMPLETE'
PRINT '============================================='
PRINT ''

PRINT 'Stored Procedure Parameters (should all be bigint):'
SELECT 
    OBJECT_NAME(p.object_id) AS ProcedureName,
    p.name AS ParameterName,
    t.name AS DataType,
    CASE WHEN t.name = 'bigint' THEN 'OK' ELSE '*** FAILED ***' END AS Status
FROM sys.parameters p
INNER JOIN sys.types t ON p.user_type_id = t.user_type_id
WHERE OBJECT_NAME(p.object_id) IN ('SaveAllData', 'SaveAllDataWithoutPos', 'SaveMoney', 'AddMoney', 'CharacterAddMoney', 'GuildUpdateBankGold', 'AddCharacterMoney', 'GuildChallengeUpdate')
  AND (p.name LIKE '%gd%' OR p.name LIKE '%gold%' OR p.name LIKE '%money%' OR p.name = '@_GD' OR p.name = '@_GOLD')
ORDER BY OBJECT_NAME(p.object_id);

PRINT ''
PRINT 'Characters with abnormal gold (should be empty):'
SELECT cha_id, cha_name, gd FROM character WHERE gd < 0 OR gd > 100000000000;

PRINT ''
PRINT 'Top 10 richest characters:'
SELECT TOP 10 cha_id, cha_name, gd AS Gold FROM character ORDER BY gd DESC;

PRINT ''
PRINT '============================================='
PRINT 'FIX COMPLETE!'
PRINT '============================================='
PRINT 'All stored procedures now use BIGINT for gold.'
PRINT 'All corrupted gold values have been fixed.'
PRINT 'Players should now see correct gold values.'
GO
