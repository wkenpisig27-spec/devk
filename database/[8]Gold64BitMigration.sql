-- ============================================
-- Gold 64-Bit Migration Script
-- PKODev Project
-- Created: January 2026
-- ============================================
-- This script updates all gold-related columns and stored procedures
-- from INT (32-bit, max 2.1 billion) to BIGINT (64-bit, max 100 billion).
-- 
-- Run this script AFTER all other database scripts have been executed.
-- Make sure to BACKUP your database before running this script!
-- ============================================

USE [GameDB]
GO

PRINT '============================================'
PRINT 'Starting Gold 64-Bit Migration...'
PRINT '============================================'

-- ============================================
-- STEP 1: Verify Current Column Types
-- ============================================
PRINT ''
PRINT 'STEP 1: Checking current column types...'

SELECT 
    t.name AS TableName,
    c.name AS ColumnName,
    ty.name AS DataType,
    CASE WHEN ty.name = 'bigint' THEN 'OK' ELSE 'NEEDS UPDATE' END AS Status
FROM sys.tables t
INNER JOIN sys.columns c ON t.object_id = c.object_id
INNER JOIN sys.types ty ON c.user_type_id = ty.user_type_id
WHERE 
    (c.name IN ('gd', 'gold', 'challmoney', 'pending_gold') 
     OR c.name LIKE '%money%' 
     OR c.name LIKE '%gold%')
    AND t.name NOT LIKE 'sys%'
ORDER BY t.name, c.name;

-- ============================================
-- STEP 2: Alter Character Table (if needed)
-- ============================================
PRINT ''
PRINT 'STEP 2: Checking character.gd column...'

IF EXISTS (
    SELECT 1 FROM sys.columns c 
    INNER JOIN sys.types t ON c.user_type_id = t.user_type_id
    WHERE c.object_id = OBJECT_ID('character') 
    AND c.name = 'gd' 
    AND t.name != 'bigint'
)
BEGIN
    PRINT '  -> Dropping default constraint on character.gd...'
    IF EXISTS (SELECT 1 FROM sys.default_constraints WHERE name = 'DF_character_gd')
        ALTER TABLE [dbo].[character] DROP CONSTRAINT [DF_character_gd];
    
    PRINT '  -> Altering character.gd to BIGINT...'
    ALTER TABLE [dbo].[character] ALTER COLUMN [gd] BIGINT NOT NULL;
    
    PRINT '  -> Re-adding default constraint...'
    ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_gd] DEFAULT ((10000)) FOR [gd];
    
    PRINT '  -> character.gd updated to BIGINT'
END
ELSE
BEGIN
    PRINT '  -> character.gd is already BIGINT'
END
GO

-- ============================================
-- STEP 3: Alter Character Log Table (if needed)
-- ============================================
PRINT ''
PRINT 'STEP 3: Checking character_log.gd column...'

IF EXISTS (
    SELECT 1 FROM sys.columns c 
    INNER JOIN sys.types t ON c.user_type_id = t.user_type_id
    WHERE c.object_id = OBJECT_ID('character_log') 
    AND c.name = 'gd' 
    AND t.name != 'bigint'
)
BEGIN
    PRINT '  -> Dropping default constraint on character_log.gd...'
    IF EXISTS (SELECT 1 FROM sys.default_constraints WHERE name = 'DF_character_log_gd')
        ALTER TABLE [dbo].[character_log] DROP CONSTRAINT [DF_character_log_gd];
    
    PRINT '  -> Altering character_log.gd to BIGINT...'
    ALTER TABLE [dbo].[character_log] ALTER COLUMN [gd] BIGINT NOT NULL;
    
    PRINT '  -> Re-adding default constraint...'
    ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_gd] DEFAULT ((0)) FOR [gd];
    
    PRINT '  -> character_log.gd updated to BIGINT'
END
ELSE
BEGIN
    PRINT '  -> character_log.gd is already BIGINT'
END
GO

-- ============================================
-- STEP 4: Alter Guild Table (if needed)
-- ============================================
PRINT ''
PRINT 'STEP 4: Checking guild gold columns...'

IF EXISTS (
    SELECT 1 FROM sys.columns c 
    INNER JOIN sys.types t ON c.user_type_id = t.user_type_id
    WHERE c.object_id = OBJECT_ID('guild') 
    AND c.name = 'gold' 
    AND t.name != 'bigint'
)
BEGIN
    PRINT '  -> Altering guild.gold to BIGINT...'
    ALTER TABLE [dbo].[guild] ALTER COLUMN [gold] BIGINT NOT NULL;
    PRINT '  -> guild.gold updated to BIGINT'
END
ELSE
BEGIN
    PRINT '  -> guild.gold is already BIGINT'
END

IF EXISTS (
    SELECT 1 FROM sys.columns c 
    INNER JOIN sys.types t ON c.user_type_id = t.user_type_id
    WHERE c.object_id = OBJECT_ID('guild') 
    AND c.name = 'challmoney' 
    AND t.name != 'bigint'
)
BEGIN
    PRINT '  -> Altering guild.challmoney to BIGINT...'
    ALTER TABLE [dbo].[guild] ALTER COLUMN [challmoney] BIGINT NOT NULL;
    PRINT '  -> guild.challmoney updated to BIGINT'
END
ELSE
BEGIN
    PRINT '  -> guild.challmoney is already BIGINT'
END
GO

-- ============================================
-- STEP 5: Alter Offline Stalls Table (if exists)
-- ============================================
PRINT ''
PRINT 'STEP 5: Checking offline_stalls.pending_gold column...'

IF OBJECT_ID('dbo.offline_stalls', 'U') IS NOT NULL
BEGIN
    IF EXISTS (
        SELECT 1 FROM sys.columns c 
        INNER JOIN sys.types t ON c.user_type_id = t.user_type_id
        WHERE c.object_id = OBJECT_ID('offline_stalls') 
        AND c.name = 'pending_gold' 
        AND t.name != 'bigint'
    )
    BEGIN
        PRINT '  -> Altering offline_stalls.pending_gold to BIGINT...'
        ALTER TABLE [dbo].[offline_stalls] ALTER COLUMN [pending_gold] BIGINT NOT NULL;
        PRINT '  -> offline_stalls.pending_gold updated to BIGINT'
    END
    ELSE
    BEGIN
        PRINT '  -> offline_stalls.pending_gold is already BIGINT'
    END
END
ELSE
BEGIN
    PRINT '  -> offline_stalls table does not exist (skipping)'
END
GO

-- ============================================
-- STEP 6: Update SaveCharacterData Stored Procedure
-- ============================================
PRINT ''
PRINT 'STEP 6: Updating SaveCharacterData procedure...'

IF OBJECT_ID('dbo.SaveCharacterData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveCharacterData
GO

CREATE PROCEDURE [dbo].[SaveCharacterData]
    @chaId INT,
    @hp INT,
    @sp INT,
    @exp BIGINT,
    @map VARCHAR(50),
    @mainMap VARCHAR(50),
    @mapX INT,
    @mapY INT,
    @radius INT,
    @angle SMALLINT,
    @pkCtrl TINYINT,
    @degree SMALLINT,
    @job VARCHAR(20),
    @gd BIGINT,                    -- Changed from INT to BIGINT
    @ap INT,
    @tp INT,
    @str INT,
    @dex INT,
    @agi INT,
    @con INT,
    @sta INT,
    @luk INT,
    @look VARCHAR(500),
    @skillbag VARCHAR(1500),
    @shortcut VARCHAR(1500),
    @mission VARCHAR(MAX),
    @misrecord VARCHAR(MAX),
    @mistrigger VARCHAR(MAX),
    @miscount VARCHAR(MAX),
    @birth VARCHAR(50),
    @loginCha VARCHAR(50),
    @sailLv INT,
    @sailExp INT,
    @sailLeftExp INT,
    @liveLv INT,
    @liveExp INT,
    @liveTp INT,
    @kbLocked INT,
    @credit INT,
    @storeItem INT,
    @skillState VARCHAR(1024),
    @extend VARCHAR(MAX),
    @imp INT,
    @savePos BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    
    IF @savePos = 1
    BEGIN
        UPDATE character SET 
            hp = @hp, sp = @sp, exp = @exp, map = @map, main_map = @mainMap, 
            map_x = @mapX, map_y = @mapY, radius = @radius, angle = @angle, 
            pk_ctrl = @pkCtrl, degree = @degree, job = @job, gd = @gd, ap = @ap, tp = @tp, 
            str = @str, dex = @dex, agi = @agi, con = @con, sta = @sta, luk = @luk, 
            look = @look, skillbag = @skillbag, shortcut = @shortcut, 
            mission = @mission, misrecord = @misrecord, mistrigger = @mistrigger, miscount = @miscount, 
            birth = @birth, login_cha = @loginCha, 
            sail_lv = @sailLv, sail_exp = @sailExp, sail_left_exp = @sailLeftExp, 
            live_lv = @liveLv, live_exp = @liveExp, live_tp = @liveTp, 
            kb_locked = @kbLocked, credit = @credit, store_item = @storeItem, 
            skill_state = @skillState, extend = @extend, IMP = @imp
        WHERE cha_id = @chaId;
    END
    ELSE
    BEGIN
        UPDATE character SET 
            hp = @hp, sp = @sp, exp = @exp, radius = @radius, 
            pk_ctrl = @pkCtrl, degree = @degree, job = @job, gd = @gd, ap = @ap, tp = @tp, 
            str = @str, dex = @dex, agi = @agi, con = @con, sta = @sta, luk = @luk, 
            look = @look, skillbag = @skillbag, shortcut = @shortcut, 
            mission = @mission, misrecord = @misrecord, mistrigger = @mistrigger, miscount = @miscount, 
            birth = @birth, login_cha = @loginCha, 
            sail_lv = @sailLv, sail_exp = @sailExp, sail_left_exp = @sailLeftExp, 
            live_lv = @liveLv, live_exp = @liveExp, live_tp = @liveTp, 
            kb_locked = @kbLocked, credit = @credit, store_item = @storeItem, 
            skill_state = @skillState, extend = @extend, IMP = @imp
        WHERE cha_id = @chaId;
    END
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

PRINT '  -> SaveCharacterData updated with BIGINT @gd parameter'

-- ============================================
-- STEP 7: Update SaveCharacterMoney Stored Procedure
-- ============================================
PRINT ''
PRINT 'STEP 7: Updating SaveCharacterMoney procedure...'

IF OBJECT_ID('dbo.SaveCharacterMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveCharacterMoney
GO

CREATE PROCEDURE [dbo].[SaveCharacterMoney]
    @chaId INT,
    @gd BIGINT                     -- Changed from INT to BIGINT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET gd = @gd WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

PRINT '  -> SaveCharacterMoney updated with BIGINT @gd parameter'

-- ============================================
-- STEP 8: Update AddCharacterMoney Stored Procedure
-- ============================================
PRINT ''
PRINT 'STEP 8: Updating AddCharacterMoney procedure...'

IF OBJECT_ID('dbo.AddCharacterMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AddCharacterMoney
GO

CREATE PROCEDURE [dbo].[AddCharacterMoney]
    @chaId INT,
    @money BIGINT
AS
BEGIN
    SET NOCOUNT ON;
    
    -- Cap gold at 100 billion (100,000,000,000)
    DECLARE @currentGold BIGINT;
    DECLARE @newGold BIGINT;
    DECLARE @maxGold BIGINT;
    SET @maxGold = 100000000000;
    
    SELECT @currentGold = ISNULL(gd, 0) FROM character WHERE cha_id = @chaId;
    
    SET @newGold = @currentGold + @money;
    
    -- Ensure gold doesn't go negative
    IF @newGold < 0
        SET @newGold = 0;
    
    -- Cap at maximum
    IF @newGold > @maxGold
        SET @newGold = @maxGold;
    
    UPDATE character SET gd = @newGold WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

PRINT '  -> AddCharacterMoney updated with BIGINT @money parameter and 100B cap'

-- ============================================
-- STEP 9: Update CharacterAddMoney (if exists)
-- ============================================
PRINT ''
PRINT 'STEP 9: Checking for CharacterAddMoney procedure...'

IF OBJECT_ID('dbo.CharacterAddMoney', 'P') IS NOT NULL
BEGIN
    DROP PROCEDURE dbo.CharacterAddMoney
    PRINT '  -> Dropped existing CharacterAddMoney procedure'
END
GO

-- Only create if it existed before (check by looking for related procedures)
IF OBJECT_ID('dbo.AddCharacterMoney', 'P') IS NOT NULL
BEGIN
    -- Create CharacterAddMoney as an alias pattern used by GroupServer
    EXEC('
    CREATE PROCEDURE [dbo].[CharacterAddMoney]
        @money BIGINT,
        @cha_id INT
    AS
    BEGIN
        SET NOCOUNT ON;
        
        DECLARE @currentGold BIGINT;
        DECLARE @newGold BIGINT;
        DECLARE @maxGold BIGINT;
        SET @maxGold = 100000000000;
        
        SELECT @currentGold = ISNULL(gd, 0) FROM character WHERE cha_id = @cha_id;
        
        SET @newGold = @currentGold + @money;
        
        IF @newGold < 0
            SET @newGold = 0;
        
        IF @newGold > @maxGold
            SET @newGold = @maxGold;
        
        UPDATE character SET gd = @newGold WHERE cha_id = @cha_id;
        
        RETURN @@ROWCOUNT;
    END
    ')
    PRINT '  -> CharacterAddMoney created with BIGINT @money parameter'
END
GO

-- ============================================
-- STEP 10: Update Guild Gold Procedures
-- ============================================
PRINT ''
PRINT 'STEP 10: Updating GuildAddMoney procedure...'

IF OBJECT_ID('dbo.GuildAddMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildAddMoney
GO

CREATE PROCEDURE [dbo].[GuildAddMoney]
    @guild_id INT,
    @money BIGINT
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @currentGold BIGINT;
    DECLARE @newGold BIGINT;
    DECLARE @maxGold BIGINT;
    SET @maxGold = 100000000000;
    
    SELECT @currentGold = ISNULL(gold, 0) FROM guild WHERE guild_id = @guild_id;
    
    SET @newGold = @currentGold + @money;
    
    IF @newGold < 0
        SET @newGold = 0;
    
    IF @newGold > @maxGold
        SET @newGold = @maxGold;
    
    UPDATE guild SET gold = @newGold WHERE guild_id = @guild_id;
    
    RETURN @@ROWCOUNT;
END
GO

PRINT '  -> GuildAddMoney created/updated with BIGINT @money parameter'

-- ============================================
-- STEP 11: Update any remaining procedures with INT money params
-- ============================================
PRINT ''
PRINT 'STEP 11: Checking for other money-related procedures...'

-- Check and update GuildUpdateMoney if exists
IF OBJECT_ID('dbo.GuildUpdateMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildUpdateMoney
GO

-- Create GuildUpdateMoney if it was dropped
IF NOT EXISTS (SELECT 1 FROM sys.procedures WHERE name = 'GuildUpdateMoney')
BEGIN
    EXEC('
    CREATE PROCEDURE [dbo].[GuildUpdateMoney]
        @guild_id INT,
        @money BIGINT
    AS
    BEGIN
        SET NOCOUNT ON;
        UPDATE guild SET gold = gold + @money WHERE guild_id = @guild_id;
        RETURN @@ROWCOUNT;
    END
    ')
    PRINT '  -> GuildUpdateMoney created'
END
GO

-- ============================================
-- STEP 12: Final Verification
-- ============================================
PRINT ''
PRINT 'STEP 12: Final verification of column types...'

SELECT 
    t.name AS TableName,
    c.name AS ColumnName,
    ty.name AS DataType,
    CASE WHEN ty.name = 'bigint' THEN 'OK (64-bit)' ELSE 'WARNING: Still ' + ty.name END AS Status
FROM sys.tables t
INNER JOIN sys.columns c ON t.object_id = c.object_id
INNER JOIN sys.types ty ON c.user_type_id = ty.user_type_id
WHERE 
    (c.name IN ('gd', 'gold', 'challmoney', 'pending_gold') 
     OR c.name LIKE '%money%' 
     OR c.name LIKE '%gold%')
    AND t.name NOT LIKE 'sys%'
ORDER BY t.name, c.name;

-- ============================================
-- STEP 13: Verify Stored Procedure Parameters
-- ============================================
PRINT ''
PRINT 'STEP 13: Checking stored procedure parameter types...'

SELECT 
    OBJECT_NAME(p.object_id) AS ProcedureName,
    p.name AS ParameterName,
    t.name AS DataType,
    CASE 
        WHEN p.name LIKE '%money%' OR p.name LIKE '%gold%' OR p.name = '@gd'
        THEN CASE WHEN t.name = 'bigint' THEN 'OK (64-bit)' ELSE 'WARNING: Should be BIGINT' END
        ELSE 'N/A'
    END AS Status
FROM sys.parameters p
INNER JOIN sys.types t ON p.user_type_id = t.user_type_id
WHERE (p.name LIKE '%money%' OR p.name LIKE '%gold%' OR p.name = '@gd')
ORDER BY OBJECT_NAME(p.object_id), p.parameter_id;

PRINT ''
PRINT '============================================'
PRINT 'Gold 64-Bit Migration Complete!'
PRINT '============================================'
PRINT ''
PRINT 'Summary of changes:'
PRINT '  - character.gd: BIGINT (supports up to 100 billion)'
PRINT '  - character_log.gd: BIGINT'
PRINT '  - guild.gold: BIGINT'
PRINT '  - guild.challmoney: BIGINT'
PRINT '  - offline_stalls.pending_gold: BIGINT (if table exists)'
PRINT '  - All stored procedures updated with BIGINT parameters'
PRINT '  - Gold cap of 100,000,000,000 (100 billion) enforced'
PRINT ''
PRINT 'IMPORTANT: Rebuild GameServer and GroupServer after running this script!'
PRINT '============================================'
GO
