-- =============================================
-- Badge System Migration
-- Adds badge_level column to character table
-- and stored procedures for badge management
-- =============================================

USE [GameDB]
GO

-- Step 1: Add badge column to character table
IF NOT EXISTS (
    SELECT 1 FROM INFORMATION_SCHEMA.COLUMNS
    WHERE TABLE_NAME = 'character' AND COLUMN_NAME = 'badge'
)
BEGIN
    ALTER TABLE [dbo].[character]
    ADD [badge] [int] NOT NULL DEFAULT 0;
    PRINT 'Added badge column to character table.';
END
ELSE
BEGIN
    PRINT 'badge column already exists.';
END
GO

-- Step 2: GetBadgeLevelByName stored procedure
IF OBJECT_ID('dbo.GetBadgeLevelByName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetBadgeLevelByName
GO

CREATE PROCEDURE [dbo].[GetBadgeLevelByName]
    @cha_name VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    SELECT badge FROM character WHERE cha_name = @cha_name AND delflag = 0;
END
GO

-- Step 3: SetBadgeLevelByName stored procedure
IF OBJECT_ID('dbo.SetBadgeLevelByName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SetBadgeLevelByName
GO

CREATE PROCEDURE [dbo].[SetBadgeLevelByName]
    @cha_name VARCHAR(50),
    @badge INT
AS
BEGIN
    SET NOCOUNT ON;
    UPDATE character SET badge = @badge WHERE cha_name = @cha_name AND delflag = 0;
END
GO

PRINT 'Badge system migration complete.';
GO
