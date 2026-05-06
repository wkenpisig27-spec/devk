-- =============================================
-- Gold 64-Bit Data Type Verification & Fix Script
-- PKODev - January 29, 2026
-- =============================================
-- This script verifies and fixes all gold-related columns
-- to support 100 billion gold cap (requires BIGINT)
-- =============================================

USE GameDB;
GO

PRINT '============================================='
PRINT 'GOLD 64-BIT DATA TYPE VERIFICATION SCRIPT'
PRINT '============================================='
PRINT ''

-- =============================================
-- STEP 1: Show current column data types
-- =============================================
PRINT 'STEP 1: Current Gold-Related Column Data Types'
PRINT '-----------------------------------------------'

SELECT 
    t.name AS TableName,
    c.name AS ColumnName,
    ty.name AS DataType,
    c.max_length AS MaxLength,
    CASE 
        WHEN ty.name = 'bigint' THEN 'OK (64-bit)'
        WHEN ty.name = 'int' THEN 'NEEDS FIX (32-bit)'
        WHEN ty.name = 'money' THEN 'NEEDS FIX (money type)'
        ELSE 'CHECK MANUALLY'
    END AS Status
FROM sys.tables t
INNER JOIN sys.columns c ON t.object_id = c.object_id
INNER JOIN sys.types ty ON c.user_type_id = ty.user_type_id
WHERE c.name IN ('gd', 'gold', 'money', 'pending_gold', 'challmoney')
ORDER BY t.name, c.name;

PRINT ''

-- =============================================
-- STEP 2: Show current stored procedure parameter types
-- =============================================
PRINT 'STEP 2: Stored Procedures with Gold Parameters'
PRINT '-----------------------------------------------'

SELECT 
    OBJECT_NAME(p.object_id) AS ProcedureName,
    p.name AS ParameterName,
    t.name AS DataType,
    CASE 
        WHEN t.name = 'bigint' THEN 'OK (64-bit)'
        WHEN t.name = 'int' THEN 'NEEDS FIX (32-bit)'
        WHEN t.name = 'varchar' THEN 'OK (string, converted in SP)'
        ELSE 'CHECK MANUALLY'
    END AS Status
FROM sys.parameters p
INNER JOIN sys.types t ON p.user_type_id = t.user_type_id
WHERE p.name LIKE '%gd%' 
   OR p.name LIKE '%gold%' 
   OR p.name LIKE '%money%'
   OR p.name IN ('@_GD', '@_GOLD')
ORDER BY OBJECT_NAME(p.object_id), p.parameter_id;

PRINT ''

-- =============================================
-- STEP 3: Fix character.gd column if needed
-- =============================================
PRINT 'STEP 3: Checking character.gd column...'

IF EXISTS (
    SELECT 1 FROM sys.columns c
    INNER JOIN sys.types t ON c.user_type_id = t.user_type_id
    WHERE OBJECT_NAME(c.object_id) = 'character' 
    AND c.name = 'gd' 
    AND t.name != 'bigint'
)
BEGIN
    PRINT '  -> character.gd is NOT BIGINT, fixing...'
    
    -- Drop default constraint if exists
    DECLARE @constraintName NVARCHAR(128)
    SELECT @constraintName = dc.name
    FROM sys.default_constraints dc
    INNER JOIN sys.columns c ON dc.parent_object_id = c.object_id AND dc.parent_column_id = c.column_id
    WHERE OBJECT_NAME(dc.parent_object_id) = 'character' AND c.name = 'gd'
    
    IF @constraintName IS NOT NULL
    BEGIN
        EXEC('ALTER TABLE [character] DROP CONSTRAINT ' + @constraintName)
        PRINT '  -> Dropped constraint: ' + @constraintName
    END
    
    ALTER TABLE [character] ALTER COLUMN [gd] BIGINT NOT NULL;
    ALTER TABLE [character] ADD CONSTRAINT DF_character_gd DEFAULT (0) FOR [gd];
    PRINT '  -> character.gd converted to BIGINT'
END
ELSE
BEGIN
    PRINT '  -> character.gd is already BIGINT (OK)'
END

-- =============================================
-- STEP 4: Fix guild.gold column if needed
-- =============================================
PRINT 'STEP 4: Checking guild.gold column...'

IF EXISTS (
    SELECT 1 FROM sys.columns c
    INNER JOIN sys.types t ON c.user_type_id = t.user_type_id
    WHERE OBJECT_NAME(c.object_id) = 'guild' 
    AND c.name = 'gold' 
    AND t.name != 'bigint'
)
BEGIN
    PRINT '  -> guild.gold is NOT BIGINT, fixing...'
    ALTER TABLE [guild] ALTER COLUMN [gold] BIGINT NOT NULL;
    PRINT '  -> guild.gold converted to BIGINT'
END
ELSE
BEGIN
    PRINT '  -> guild.gold is already BIGINT (OK)'
END

-- =============================================
-- STEP 5: Fix guild.challmoney column if needed
-- =============================================
PRINT 'STEP 5: Checking guild.challmoney column...'

IF EXISTS (
    SELECT 1 FROM sys.columns c
    INNER JOIN sys.types t ON c.user_type_id = t.user_type_id
    WHERE OBJECT_NAME(c.object_id) = 'guild' 
    AND c.name = 'challmoney' 
    AND t.name != 'bigint'
)
BEGIN
    PRINT '  -> guild.challmoney is NOT BIGINT, fixing...'
    ALTER TABLE [guild] ALTER COLUMN [challmoney] BIGINT NOT NULL;
    PRINT '  -> guild.challmoney converted to BIGINT'
END
ELSE
BEGIN
    PRINT '  -> guild.challmoney is already BIGINT (OK)'
END

-- =============================================
-- STEP 6: Fix offline_stalls.pending_gold if exists
-- =============================================
PRINT 'STEP 6: Checking offline_stalls.pending_gold column...'

IF EXISTS (SELECT 1 FROM sys.tables WHERE name = 'offline_stalls')
BEGIN
    IF EXISTS (
        SELECT 1 FROM sys.columns c
        INNER JOIN sys.types t ON c.user_type_id = t.user_type_id
        WHERE OBJECT_NAME(c.object_id) = 'offline_stalls' 
        AND c.name = 'pending_gold' 
        AND t.name != 'bigint'
    )
    BEGIN
        PRINT '  -> offline_stalls.pending_gold is NOT BIGINT, fixing...'
        ALTER TABLE [offline_stalls] ALTER COLUMN [pending_gold] BIGINT NOT NULL;
        PRINT '  -> offline_stalls.pending_gold converted to BIGINT'
    END
    ELSE
    BEGIN
        PRINT '  -> offline_stalls.pending_gold is already BIGINT (OK)'
    END
END
ELSE
BEGIN
    PRINT '  -> offline_stalls table does not exist (skipping)'
END

-- =============================================
-- STEP 7: Fix character_log.gd if exists
-- =============================================
PRINT 'STEP 7: Checking character_log.gd column...'

IF EXISTS (SELECT 1 FROM sys.tables WHERE name = 'character_log')
BEGIN
    IF EXISTS (
        SELECT 1 FROM sys.columns c
        INNER JOIN sys.types t ON c.user_type_id = t.user_type_id
        WHERE OBJECT_NAME(c.object_id) = 'character_log' 
        AND c.name = 'gd' 
        AND t.name != 'bigint'
    )
    BEGIN
        PRINT '  -> character_log.gd is NOT BIGINT, fixing...'
        
        -- Drop default constraint if exists
        DECLARE @constraintName2 NVARCHAR(128)
        SELECT @constraintName2 = dc.name
        FROM sys.default_constraints dc
        INNER JOIN sys.columns c ON dc.parent_object_id = c.object_id AND dc.parent_column_id = c.column_id
        WHERE OBJECT_NAME(dc.parent_object_id) = 'character_log' AND c.name = 'gd'
        
        IF @constraintName2 IS NOT NULL
        BEGIN
            EXEC('ALTER TABLE [character_log] DROP CONSTRAINT ' + @constraintName2)
            PRINT '  -> Dropped constraint: ' + @constraintName2
        END
        
        ALTER TABLE [character_log] ALTER COLUMN [gd] BIGINT NULL;
        PRINT '  -> character_log.gd converted to BIGINT'
    END
    ELSE
    BEGIN
        PRINT '  -> character_log.gd is already BIGINT (OK)'
    END
END
ELSE
BEGIN
    PRINT '  -> character_log table does not exist (skipping)'
END

GO

-- =============================================
-- STEP 8: Update SaveMoney stored procedure
-- =============================================
PRINT ''
PRINT 'STEP 8: Updating SaveMoney stored procedure...'

IF OBJECT_ID('dbo.SaveMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveMoney
GO

CREATE PROCEDURE [dbo].[SaveMoney]
    @_GD BIGINT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    UPDATE character SET gd = @_GD WHERE cha_id = @_CHA_ID;
END
GO
PRINT '  -> SaveMoney updated with BIGINT parameter'

-- =============================================
-- STEP 9: Update AddMoney stored procedure
-- =============================================
PRINT 'STEP 9: Updating AddMoney stored procedure...'

IF OBJECT_ID('dbo.AddMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AddMoney
GO

CREATE PROCEDURE [dbo].[AddMoney]
    @_GOLD BIGINT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @currentGold BIGINT;
    DECLARE @newGold BIGINT;
    DECLARE @maxGold BIGINT;
    SET @maxGold = 100000000000; -- 100 billion cap
    
    SELECT @currentGold = gd FROM character WHERE cha_id = @_CHA_ID;
    SET @newGold = @currentGold + @_GOLD;
    
    -- Cap at max gold
    IF @newGold > @maxGold
        SET @newGold = @maxGold;
    IF @newGold < 0
        SET @newGold = 0;
    
    UPDATE character SET gd = @newGold WHERE cha_id = @_CHA_ID;
END
GO
PRINT '  -> AddMoney updated with BIGINT parameter and 100B cap'

-- =============================================
-- STEP 10: Update CharacterAddMoney stored procedure
-- =============================================
PRINT 'STEP 10: Updating CharacterAddMoney stored procedure...'

IF OBJECT_ID('dbo.CharacterAddMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CharacterAddMoney
GO

CREATE PROCEDURE [dbo].[CharacterAddMoney]
    @money BIGINT,
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @currentGold BIGINT;
    DECLARE @newGold BIGINT;
    DECLARE @maxGold BIGINT;
    SET @maxGold = 100000000000; -- 100 billion cap
    
    SELECT @currentGold = gd FROM character WHERE cha_id = @cha_id;
    SET @newGold = @currentGold + @money;
    
    -- Cap at max gold
    IF @newGold > @maxGold
        SET @newGold = @maxGold;
    IF @newGold < 0
        SET @newGold = 0;
    
    UPDATE character SET gd = @newGold WHERE cha_id = @cha_id;
    RETURN @@ROWCOUNT;
END
GO
PRINT '  -> CharacterAddMoney updated with BIGINT parameter and 100B cap'

-- =============================================
-- STEP 11: Final verification
-- =============================================
PRINT ''
PRINT '============================================='
PRINT 'FINAL VERIFICATION'
PRINT '============================================='
PRINT ''

PRINT 'Gold Columns After Fix:'
PRINT '-----------------------'

SELECT 
    t.name AS TableName,
    c.name AS ColumnName,
    ty.name AS DataType,
    CASE 
        WHEN ty.name = 'bigint' THEN 'OK'
        ELSE 'FAILED'
    END AS Status
FROM sys.tables t
INNER JOIN sys.columns c ON t.object_id = c.object_id
INNER JOIN sys.types ty ON c.user_type_id = ty.user_type_id
WHERE c.name IN ('gd', 'gold', 'money', 'pending_gold', 'challmoney')
ORDER BY t.name, c.name;

PRINT ''
PRINT 'Stored Procedures After Fix:'
PRINT '----------------------------'

SELECT 
    OBJECT_NAME(p.object_id) AS ProcedureName,
    p.name AS ParameterName,
    t.name AS DataType,
    CASE 
        WHEN t.name = 'bigint' THEN 'OK'
        WHEN t.name = 'varchar' THEN 'OK (string)'
        ELSE 'CHECK'
    END AS Status
FROM sys.parameters p
INNER JOIN sys.types t ON p.user_type_id = t.user_type_id
WHERE OBJECT_NAME(p.object_id) IN ('SaveMoney', 'AddMoney', 'CharacterAddMoney', 'SaveAllData', 'SaveAllDataWithoutPos')
  AND (p.name LIKE '%gd%' OR p.name LIKE '%gold%' OR p.name LIKE '%money%' OR p.name = '@_GD' OR p.name = '@_GOLD')
ORDER BY OBJECT_NAME(p.object_id), p.parameter_id;

PRINT ''
PRINT '============================================='
PRINT 'CURRENT CHARACTER GOLD VALUES (Top 10)'
PRINT '============================================='

SELECT TOP 10 
    cha_id,
    cha_name,
    gd AS Gold,
    CASE 
        WHEN gd > 2147483647 THEN 'Over 32-bit limit'
        WHEN gd < 0 THEN 'NEGATIVE (corrupted!)'
        WHEN gd > 100000000000 THEN 'Over 100B cap'
        ELSE 'Normal'
    END AS Status
FROM character
ORDER BY gd DESC;

PRINT ''
PRINT '============================================='
PRINT 'CHARACTERS WITH SUSPICIOUS GOLD VALUES'
PRINT '============================================='

SELECT 
    cha_id,
    cha_name,
    gd AS Gold,
    'CORRUPTED - Needs Reset' AS Status
FROM character
WHERE gd < 0 OR gd > 100000000000;

PRINT ''
PRINT 'Script completed. Review results above.'
PRINT 'To fix corrupted gold: UPDATE character SET gd = 0 WHERE gd < 0 OR gd > 100000000000;'
GO
