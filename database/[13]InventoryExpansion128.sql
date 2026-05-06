-- =============================================
-- Migration: Expand inventory slots from 48 to 128
-- Date: 2026-03-17
-- Description: Increases kitbag/bag varchar columns
--   from 7000 to MAX to support 128 inventory slots.
--   Recreates affected stored procedures.
-- =============================================

USE GameDB
GO

PRINT '=== Starting Inventory Expansion Migration ==='
PRINT ''

-- =============================================
-- Step 1: Drop default constraints that block ALTER COLUMN
-- =============================================

-- Drop default constraint on character.kitbag (if exists)
PRINT 'Dropping default constraints on character.kitbag...'
DECLARE @ConstraintName1 NVARCHAR(256)
SELECT @ConstraintName1 = dc.name
FROM sys.default_constraints dc
JOIN sys.columns c ON dc.parent_object_id = c.object_id AND dc.parent_column_id = c.column_id
WHERE dc.parent_object_id = OBJECT_ID('dbo.character') AND c.name = 'kitbag'

IF @ConstraintName1 IS NOT NULL
BEGIN
    EXEC('ALTER TABLE [dbo].[character] DROP CONSTRAINT [' + @ConstraintName1 + ']')
    PRINT '  Dropped constraint: ' + @ConstraintName1
END
ELSE
    PRINT '  No default constraint found.'
GO

-- Drop default constraint on boat.boat_bag (if exists)
PRINT 'Dropping default constraints on boat.boat_bag...'
DECLARE @ConstraintName2 NVARCHAR(256)
SELECT @ConstraintName2 = dc.name
FROM sys.default_constraints dc
JOIN sys.columns c ON dc.parent_object_id = c.object_id AND dc.parent_column_id = c.column_id
WHERE dc.parent_object_id = OBJECT_ID('dbo.boat') AND c.name = 'boat_bag'

IF @ConstraintName2 IS NOT NULL
BEGIN
    EXEC('ALTER TABLE [dbo].[boat] DROP CONSTRAINT [' + @ConstraintName2 + ']')
    PRINT '  Dropped constraint: ' + @ConstraintName2
END
ELSE
    PRINT '  No default constraint found.'
GO

-- =============================================
-- Step 2: ALTER TABLE columns
-- =============================================
PRINT 'Altering character.kitbag column...'
ALTER TABLE [dbo].[character] ALTER COLUMN [kitbag] VARCHAR(MAX) NOT NULL;
GO

PRINT 'Altering boat.boat_bag column...'
ALTER TABLE [dbo].[boat] ALTER COLUMN [boat_bag] VARCHAR(MAX) NOT NULL;
GO

-- =============================================
-- Step 3: Re-add default constraints
-- =============================================
PRINT 'Re-adding default constraint on character.kitbag...'
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_kitbag] DEFAULT ('') FOR [kitbag];
GO

PRINT 'Re-adding default constraint on boat.boat_bag...'
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_bag] DEFAULT ('') FOR [boat_bag];
GO

-- =============================================
-- Step 2: Recreate stored procedures with updated parameter types
-- =============================================

-- SaveBoatData
PRINT 'Recreating SaveBoatData...'
IF OBJECT_ID('dbo.SaveBoatData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBoatData
GO

CREATE PROCEDURE [dbo].[SaveBoatData]
    @boatId INT,
    @boatBerth SMALLINT,
    @boatName VARCHAR(17),
    @header INT,
    @body INT,
    @engine INT,
    @cannon INT,
    @equipment INT,
    @bagSize SMALLINT,
    @bag VARCHAR(MAX),
    @curEndure INT,
    @mxEndure INT,
    @curSupply INT,
    @mxSupply INT,
    @skillState VARCHAR(400),
    @isDead CHAR(1)
AS
BEGIN
    SET NOCOUNT ON;

    UPDATE boat SET
        boat_berth = @boatBerth,
        boat_name = @boatName,
        boat_header = @header,
        boat_body = @body,
        boat_engine = @engine,
        boat_cannon = @cannon,
        boat_equipment = @equipment,
        boat_bagsize = @bagSize,
        boat_bag = @bag,
        cur_endure = @curEndure,
        mx_endure = @mxEndure,
        cur_supply = @curSupply,
        mx_supply = @mxSupply,
        skill_state = @skillState,
        boat_isdead = @isDead
    WHERE boat_id = @boatId;
END
GO

-- SaveBoatExWithPos
PRINT 'Recreating SaveBoatExWithPos...'
IF OBJECT_ID('dbo.SaveBoatExWithPos', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBoatExWithPos
GO

CREATE PROCEDURE [dbo].[SaveBoatExWithPos]
    @_BOAT_BERTH SMALLINT,
    @_BOAT_OWNERID INT,
    @_CUR_ENDURE INT,
    @_MX_ENDURE INT,
    @_CUR_SUPPLY INT,
    @_MX_SUPPLY INT,
    @_SKILL_STATE VARCHAR(400),
    @_MAP VARCHAR(50),
    @_MAP_X INT,
    @_MAP_Y INT,
    @_ANGLE INT,
    @_DEGREE SMALLINT,
    @_EXP INT,
    @_BOAT_BAG VARCHAR(MAX),
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;

    UPDATE boat SET
        boat_berth = @_BOAT_BERTH, boat_ownerid = @_BOAT_OWNERID,
        cur_endure = @_CUR_ENDURE, mx_endure = @_MX_ENDURE,
        cur_supply = @_CUR_SUPPLY, mx_supply = @_MX_SUPPLY,
        skill_state = @_SKILL_STATE, map = @_MAP, map_x = @_MAP_X, map_y = @_MAP_Y,
        angle = @_ANGLE, degree = @_DEGREE, [exp] = @_EXP, boat_bag = @_BOAT_BAG
    WHERE boat_id = @_BOAT_ID;
END
GO

-- SaveBoatEx
PRINT 'Recreating SaveBoatEx...'
IF OBJECT_ID('dbo.SaveBoatEx', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBoatEx
GO

CREATE PROCEDURE [dbo].[SaveBoatEx]
    @_BOAT_BERTH SMALLINT,
    @_BOAT_OWNERID INT,
    @_CUR_ENDURE INT,
    @_MX_ENDURE INT,
    @_CUR_SUPPLY INT,
    @_MX_SUPPLY INT,
    @_SKILL_STATE VARCHAR(400),
    @_DEGREE SMALLINT,
    @_EXP INT,
    @_BOAT_BAG VARCHAR(MAX),
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;

    UPDATE boat SET
        boat_berth = @_BOAT_BERTH, boat_ownerid = @_BOAT_OWNERID,
        cur_endure = @_CUR_ENDURE, mx_endure = @_MX_ENDURE,
        cur_supply = @_CUR_SUPPLY, mx_supply = @_MX_SUPPLY,
        skill_state = @_SKILL_STATE, degree = @_DEGREE, [exp] = @_EXP, boat_bag = @_BOAT_BAG
    WHERE boat_id = @_BOAT_ID;
END
GO

-- SaveCabin
PRINT 'Recreating SaveCabin...'
IF OBJECT_ID('dbo.SaveCabin', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveCabin
GO

CREATE PROCEDURE [dbo].[SaveCabin]
    @_KITBAG VARCHAR(MAX),
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;

    UPDATE boat SET boat_bag = @_KITBAG WHERE boat_id = @_BOAT_ID;
END
GO

PRINT ''
PRINT '=== Inventory Expansion Migration Complete ==='
PRINT 'character.kitbag -> VARCHAR(MAX)'
PRINT 'boat.boat_bag -> VARCHAR(MAX)'
PRINT 'Stored procedures updated: SaveBoatData, SaveBoatExWithPos, SaveBoatEx, SaveCabin'
PRINT ''
PRINT 'NOTE: Rebuild both GameServer and Client with updated source code.'
PRINT 'NOTE: Add new inventory expansion items in ItemEffect.lua for slots beyond 48.'
GO
