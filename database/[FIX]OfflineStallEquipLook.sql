-- =============================================
-- Migration: Add equip_look_data and stall_level columns to offline_stalls
-- Purpose: Stores equipment appearance data so offline stall NPCs
--          render with the player's equipped gear instead of appearing naked.
--          Also stores stall skill level for correct booth appearance.
-- Run this AFTER [6]OfflineStalls.sql if the table already exists.
-- Safe to run multiple times (checks before altering).
-- =============================================

USE [GameDB]
GO

-- Add equip_look_data column if it doesn't exist
IF NOT EXISTS (
    SELECT 1 FROM INFORMATION_SCHEMA.COLUMNS 
    WHERE TABLE_NAME = 'offline_stalls' AND COLUMN_NAME = 'equip_look_data'
)
BEGIN
    ALTER TABLE [dbo].[offline_stalls]
    ADD [equip_look_data] VARBINARY(MAX) NULL;
    
    PRINT 'Added equip_look_data column to offline_stalls table.'
END
ELSE
BEGIN
    PRINT 'equip_look_data column already exists - no changes needed.'
END
GO

-- Add stall_level column if it doesn't exist
IF NOT EXISTS (
    SELECT 1 FROM INFORMATION_SCHEMA.COLUMNS 
    WHERE TABLE_NAME = 'offline_stalls' AND COLUMN_NAME = 'stall_level'
)
BEGIN
    ALTER TABLE [dbo].[offline_stalls]
    ADD [stall_level] TINYINT NOT NULL DEFAULT 1;
    
    PRINT 'Added stall_level column to offline_stalls table.'
END
ELSE
BEGIN
    PRINT 'stall_level column already exists - no changes needed.'
END
GO

-- Re-create OfflineStall_Create with new parameter
IF OBJECT_ID('dbo.OfflineStall_Create', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_Create
GO

CREATE PROCEDURE [dbo].[OfflineStall_Create]
    @cha_id INT,
    @cha_name VARCHAR(50),
    @act_id INT,
    @stall_title NVARCHAR(100),
    @look_face SMALLINT,
    @look_hair SMALLINT,
    @job SMALLINT,
    @map_name VARCHAR(50),
    @pos_x INT,
    @pos_y INT,
    @duration_hours INT,
    @item_count TINYINT,
    @item_data VARBINARY(MAX),
    @kitbag_snapshot VARBINARY(MAX),
    @equip_look_data VARBINARY(MAX) = NULL,
    @stall_level TINYINT = 1
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @expire_time DATETIME = DATEADD(HOUR, @duration_hours, GETDATE());
    DECLARE @existing_id INT;
    
    -- Check for existing stall by this character
    SELECT @existing_id = stall_id FROM offline_stalls 
    WHERE cha_id = @cha_id AND is_active = 1;
    
    IF @existing_id IS NOT NULL
    BEGIN
        -- Update existing stall
        UPDATE offline_stalls SET
            stall_title = @stall_title,
            look_face = @look_face,
            look_hair = @look_hair,
            job = @job,
            stall_level = @stall_level,
            map_name = @map_name,
            pos_x = @pos_x,
            pos_y = @pos_y,
            expire_time = @expire_time,
            item_count = @item_count,
            item_data = @item_data,
            kitbag_snapshot = @kitbag_snapshot,
            [equip_look_data] = @equip_look_data
        WHERE stall_id = @existing_id;
        
        SELECT @existing_id AS stall_id;
        RETURN @existing_id;
    END
    ELSE
    BEGIN
        -- Insert new stall
        INSERT INTO offline_stalls (
            cha_id, cha_name, act_id, stall_title,
            look_face, look_hair, job, stall_level, map_name,
            pos_x, pos_y, expire_time, item_count,
            item_data, kitbag_snapshot,
            [equip_look_data],
            created_time, is_active
        ) VALUES (
            @cha_id, @cha_name, @act_id, @stall_title,
            @look_face, @look_hair, @job, @stall_level, @map_name,
            @pos_x, @pos_y, @expire_time, @item_count,
            @item_data, @kitbag_snapshot,
            @equip_look_data,
            GETDATE(), 1
        );
        
        DECLARE @new_id INT = SCOPE_IDENTITY();
        SELECT @new_id AS stall_id;
        RETURN @new_id;
    END
END
GO

-- Re-create OfflineStall_LoadAll to include equip_look_data
IF OBJECT_ID('dbo.OfflineStall_LoadAll', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_LoadAll
GO

CREATE PROCEDURE [dbo].[OfflineStall_LoadAll]
    @map_name VARCHAR(50) = NULL
AS
BEGIN
    SET NOCOUNT ON;
    
    IF @map_name IS NULL
    BEGIN
        SELECT 
            stall_id, cha_id, cha_name, act_id,
            stall_title, look_face, look_hair, job,
            map_name, pos_x, pos_y,
            created_time, expire_time,
            item_count, item_data,
            kitbag_snapshot,
            ISNULL(pending_gold, 0) as pending_gold,
            [stall_level],
            [equip_look_data]
        FROM offline_stalls
        WHERE is_active = 1 AND expire_time > GETDATE();
    END
    ELSE
    BEGIN
        SELECT 
            stall_id, cha_id, cha_name, act_id,
            stall_title, look_face, look_hair, job,
            map_name, pos_x, pos_y,
            created_time, expire_time,
            item_count, item_data,
            kitbag_snapshot,
            ISNULL(pending_gold, 0) as pending_gold,
            [stall_level],
            [equip_look_data]
        FROM offline_stalls
        WHERE is_active = 1 AND expire_time > GETDATE()
            AND map_name = @map_name;
    END
END
GO

PRINT 'Migration complete: offline_stalls now supports equipment appearance and stall level.'
GO
