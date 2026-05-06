-- =============================================
-- Fix: OfflineStall_Create - Add upsert logic
-- From commit: e066aa51
-- 
-- Problem: If a character already had a stall row in offline_stalls
-- (e.g., from a previous session that wasn't cleaned up), the INSERT
-- would fail with a UNIQUE constraint violation on cha_id, preventing
-- the stall from being created.
--
-- Fix: Check if a stall already exists for the character. If yes,
-- UPDATE the existing row. If no, INSERT a new row.
--
-- Execute on: GameDB
-- =============================================

USE GameDB;
GO

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
            pos_x, pos_y, expire_time, item_count,
            item_data, kitbag_snapshot, equip_look_data, stall_level, created_time, is_active
        ) VALUES (
            @cha_id, @cha_name, @act_id, @stall_title,
            @look_face, @look_hair, @job, @map_name,
            @pos_x, @pos_y, @expire_time, @item_count,
            @item_data, @kitbag_snapshot, @equip_look_data, @stall_level, GETDATE(), 1
        );
        
        DECLARE @new_id INT = SCOPE_IDENTITY();
        SELECT @new_id AS stall_id;
        RETURN @new_id;
    END
END
GO

PRINT 'OfflineStall_Create updated with upsert logic.'
GO
