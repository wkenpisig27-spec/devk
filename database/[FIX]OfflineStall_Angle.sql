-- =============================================
-- Fix: Add angle (facing direction) to offline stalls
--
-- Problem: Offline stall NPCs always face 180 degrees (upward)
-- because the character's facing angle was never saved to the DB.
-- The NPC was created with angle=0 (default) instead of the player's
-- actual facing direction when they set up the stall.
--
-- Fix: Add [angle] SMALLINT column to offline_stalls table.
-- Update OfflineStall_Create to accept and store the angle.
-- Update OfflineStall_LoadAll to return the angle.
--
-- Execute on: GameDB
-- =============================================

USE GameDB;
GO

-- ============================================
-- Step 1: Add column (safe for existing data)
-- ============================================
IF NOT EXISTS (
    SELECT 1 FROM INFORMATION_SCHEMA.COLUMNS 
    WHERE TABLE_NAME = 'offline_stalls' AND COLUMN_NAME = 'angle'
)
BEGIN
    ALTER TABLE [dbo].[offline_stalls]
    ADD [angle] SMALLINT NOT NULL DEFAULT 0;
    PRINT 'Added [angle] column to offline_stalls table.'
END
ELSE
BEGIN
    PRINT '[angle] column already exists - skipping ALTER TABLE.'
END
GO

-- ============================================
-- Step 2: Update OfflineStall_Create SP
-- ============================================
IF OBJECT_ID('dbo.OfflineStall_Create', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_Create
GO

CREATE PROCEDURE [dbo].[OfflineStall_Create]
    @cha_id         INT,
    @cha_name       VARCHAR(50),
    @act_id         INT,
    @stall_title    NVARCHAR(100),
    @look_face      SMALLINT,
    @look_hair      SMALLINT,
    @job            SMALLINT,
    @map_name       VARCHAR(50),
    @pos_x          INT,
    @pos_y          INT,
    @angle          SMALLINT = 0,
    @duration_hours INT,
    @item_count     TINYINT,
    @item_data      VARBINARY(MAX),
    @kitbag_snapshot VARBINARY(MAX),
    @equip_look_data VARBINARY(MAX) = NULL,
    @stall_level    TINYINT = 1
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @expire_time DATETIME = DATEADD(HOUR, @duration_hours, GETDATE());
    DECLARE @existing_id INT;
    
    -- Check if character already has a stall (prevents UNIQUE constraint violation)
    SELECT @existing_id = stall_id FROM offline_stalls 
    WHERE cha_id = @cha_id;
    
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
            angle = @angle,
            expire_time = @expire_time,
            item_count = @item_count,
            item_data = @item_data,
            kitbag_snapshot = @kitbag_snapshot,
            equip_look_data = @equip_look_data,
            is_active = 1,
            pending_gold = 0,
            created_time = GETDATE()
        WHERE stall_id = @existing_id;
        
        SELECT @existing_id AS stall_id;
        RETURN @existing_id;
    END
    ELSE
    BEGIN
        -- Insert new stall
        INSERT INTO offline_stalls (
            cha_id, cha_name, act_id, stall_title,
            look_face, look_hair, job, map_name,
            pos_x, pos_y, angle, expire_time, item_count,
            item_data, kitbag_snapshot, equip_look_data, stall_level,
            created_time, is_active
        ) VALUES (
            @cha_id, @cha_name, @act_id, @stall_title,
            @look_face, @look_hair, @job, @map_name,
            @pos_x, @pos_y, @angle, @expire_time, @item_count,
            @item_data, @kitbag_snapshot, @equip_look_data, @stall_level,
            GETDATE(), 1
        );
        
        DECLARE @new_id INT = SCOPE_IDENTITY();
        SELECT @new_id AS stall_id;
        RETURN @new_id;
    END
END
GO

PRINT 'OfflineStall_Create updated with angle parameter.'
GO

-- ============================================
-- Step 3: Update OfflineStall_LoadAll SP
-- ============================================
IF OBJECT_ID('dbo.OfflineStall_LoadAll', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_LoadAll
GO

CREATE PROCEDURE [dbo].[OfflineStall_LoadAll]
    @map_name VARCHAR(64) = NULL    -- NULL = load all maps
AS
BEGIN
    SET NOCOUNT ON;
    
    -- First, mark expired stalls as inactive
    UPDATE [dbo].[offline_stalls]
    SET [is_active] = 0
    WHERE [expire_time] < GETDATE() AND [is_active] = 1
    
    -- Return active stalls
    -- Column order (indices 0-19):
    -- 0:stall_id, 1:cha_id, 2:cha_name, 3:act_id, 4:stall_title,
    -- 5:look_face, 6:look_hair, 7:job, 8:map_name, 9:pos_x, 10:pos_y,
    -- 11:created_time, 12:expire_time, 13:item_count, 14:item_data,
    -- 15:kitbag_snapshot, 16:pending_gold, 17:stall_level, 18:equip_look_data,
    -- 19:angle
    IF @map_name IS NULL
    BEGIN
        SELECT 
            [stall_id], [cha_id], [cha_name], [act_id],
            [stall_title], [look_face], [look_hair], [job],
            [map_name], [pos_x], [pos_y],
            [created_time], [expire_time],
            [item_count], [item_data], [kitbag_snapshot], [pending_gold],
            [stall_level],
            [equip_look_data],
            [angle]
        FROM [dbo].[offline_stalls]
        WHERE [is_active] = 1 AND [expire_time] > GETDATE()
        ORDER BY [map_name], [stall_id]
    END
    ELSE
    BEGIN
        SELECT 
            [stall_id], [cha_id], [cha_name], [act_id],
            [stall_title], [look_face], [look_hair], [job],
            [map_name], [pos_x], [pos_y],
            [created_time], [expire_time],
            [item_count], [item_data], [kitbag_snapshot], [pending_gold],
            [stall_level],
            [equip_look_data],
            [angle]
        FROM [dbo].[offline_stalls]
        WHERE [is_active] = 1 AND [expire_time] > GETDATE() AND [map_name] = @map_name
        ORDER BY [stall_id]
    END
    
    RETURN @@ROWCOUNT
END
GO

PRINT 'OfflineStall_LoadAll updated with angle column.'
GO

-- ============================================
-- Verification
-- ============================================
PRINT ''
PRINT 'Offline Stall Angle Fix applied successfully.'
PRINT 'Changes:'
PRINT '  - Added [angle] SMALLINT column to offline_stalls'
PRINT '  - Updated OfflineStall_Create with @angle parameter'
PRINT '  - Updated OfflineStall_LoadAll to return angle (column index 19)'
GO
