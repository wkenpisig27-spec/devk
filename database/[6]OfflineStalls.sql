-- ============================================
-- Offline Stall System - Database Schema
-- PKODev Project
-- Created: January 2026
-- ============================================
-- This script creates the offline_stalls table and stored procedures
-- for the database-persisted offline stall system.
-- 
-- Run this script AFTER [1]GameDB.sql has been executed.
-- ============================================

USE [GameDB]
GO

-- ============================================
-- DROP EXISTING OBJECTS (for clean install)
-- ============================================

IF OBJECT_ID('dbo.OfflineStall_Create', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_Create
GO

IF OBJECT_ID('dbo.OfflineStall_Delete', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_Delete
GO

IF OBJECT_ID('dbo.OfflineStall_DeleteByCharId', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_DeleteByCharId
GO

IF OBJECT_ID('dbo.OfflineStall_LoadAll', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_LoadAll
GO

IF OBJECT_ID('dbo.OfflineStall_UpdateItems', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_UpdateItems
GO

IF OBJECT_ID('dbo.OfflineStall_GetByCharId', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_GetByCharId
GO

IF OBJECT_ID('dbo.OfflineStall_ExtendTime', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_ExtendTime
GO

IF OBJECT_ID('dbo.OfflineStall_CleanupExpired', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_CleanupExpired
GO

IF OBJECT_ID('dbo.offline_stalls', 'U') IS NOT NULL
    DROP TABLE dbo.offline_stalls
GO

-- ============================================
-- CREATE TABLE: offline_stalls
-- ============================================
-- Stores offline stall data for characters who disconnect while stalling.
-- Virtual NPCs are created from this data on server startup.
-- ============================================

CREATE TABLE [dbo].[offline_stalls] (
    -- Primary key
    [stall_id]      INT IDENTITY(1,1) NOT NULL,
    
    -- Character reference
    [cha_id]        INT NOT NULL,                           -- Character database ID
    [cha_name]      VARCHAR(50) NOT NULL,                   -- Character name (for display)
    [act_id]        INT NOT NULL,                           -- Account ID (for validation)
    
    -- Stall display info
    [stall_title]   NVARCHAR(64) NOT NULL,                  -- Shop title/name
    [look_face]     INT NOT NULL DEFAULT 0,                 -- Character face ID for NPC appearance
    [look_hair]     INT NOT NULL DEFAULT 0,                 -- Character hair ID
    [job]           SMALLINT NOT NULL DEFAULT 0,            -- Character job/class
    [stall_level]   TINYINT NOT NULL DEFAULT 1,             -- Stall skill level (booth appearance)
    
    -- Location
    [map_name]      VARCHAR(64) NOT NULL,                   -- Map name where stall is located
    [pos_x]         INT NOT NULL,                           -- X coordinate
    [pos_y]         INT NOT NULL,                           -- Y coordinate
    [angle]         SMALLINT NOT NULL DEFAULT 0,            -- Facing direction (0-360)
    
    -- Timing
    [created_time]  DATETIME NOT NULL DEFAULT GETDATE(),    -- When stall was created
    [expire_time]   DATETIME NOT NULL,                      -- When stall expires
    
    -- Stall item data (serialized)
    -- Format: Each item contains full SItemGrid (~200 bytes) plus metadata
    -- Max 20 items = ~4000 bytes, using 8000 for safety (max VARBINARY without MAX)
    [item_count]    TINYINT NOT NULL DEFAULT 0,             -- Number of items in stall
    [item_data]     VARBINARY(8000) NULL,                   -- Serialized stall goods with full item data
    
    -- Character inventory snapshot (for item references)
    -- Kitbag grid IDs that correspond to stall items
    [kitbag_snapshot] VARBINARY(256) NULL,                  -- Serialized kitbag item references
    
    -- Equipment appearance data (for rendering NPC with player's gear)
    -- Contains serialized SOfflineStallEquipLook[34] with item IDs, forge levels, etc.
    [equip_look_data] VARBINARY(MAX) NULL,                  -- Serialized equipment appearance
    
    -- Stall gold (accumulated from sales while offline)
    [pending_gold]  BIGINT NOT NULL DEFAULT 0,              -- Gold to give character on login
    
    -- Status
    [is_active]     BIT NOT NULL DEFAULT 1,                 -- Whether stall is still active
    
    CONSTRAINT [PK_offline_stalls] PRIMARY KEY CLUSTERED ([stall_id]),
    CONSTRAINT [UQ_offline_stalls_cha_id] UNIQUE ([cha_id])  -- One stall per character
)
GO

-- Index for fast lookups
CREATE NONCLUSTERED INDEX [IX_offline_stalls_expire] 
    ON [dbo].[offline_stalls] ([expire_time], [is_active])
GO

CREATE NONCLUSTERED INDEX [IX_offline_stalls_map] 
    ON [dbo].[offline_stalls] ([map_name], [is_active])
GO

-- ============================================
-- STORED PROCEDURE: OfflineStall_Create
-- ============================================
-- Creates a new offline stall when a player disconnects while stalling.
-- If character already has a stall, it updates the existing one.
-- ============================================

CREATE PROCEDURE [dbo].[OfflineStall_Create]
    @cha_id         INT,
    @cha_name       VARCHAR(50),
    @act_id         INT,
    @stall_title    NVARCHAR(64),
    @look_face      INT,
    @look_hair      INT,
    @job            SMALLINT,
    @map_name       VARCHAR(64),
    @pos_x          INT,
    @pos_y          INT,
    @angle          SMALLINT = 0,
    @duration_hours INT,            -- How many hours until stall expires
    @item_count     TINYINT,
    @item_data      VARBINARY(8000),
    @kitbag_snapshot VARBINARY(256),
    @equip_look_data VARBINARY(MAX) = NULL,
    @stall_level    TINYINT = 1
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @expire_time DATETIME = DATEADD(HOUR, @duration_hours, GETDATE())
    
    -- Check if character already has a stall
    IF EXISTS (SELECT 1 FROM [dbo].[offline_stalls] WHERE [cha_id] = @cha_id)
    BEGIN
        -- Update existing stall
        UPDATE [dbo].[offline_stalls]
        SET [stall_title] = @stall_title,
            [look_face] = @look_face,
            [look_hair] = @look_hair,
            [job] = @job,
            [stall_level] = @stall_level,
            [map_name] = @map_name,
            [pos_x] = @pos_x,
            [pos_y] = @pos_y,
            [angle] = @angle,
            [expire_time] = @expire_time,
            [item_count] = @item_count,
            [item_data] = @item_data,
            [kitbag_snapshot] = @kitbag_snapshot,
            [equip_look_data] = @equip_look_data,
            [is_active] = 1,
            [created_time] = GETDATE()
        WHERE [cha_id] = @cha_id
        
        SELECT [stall_id] FROM [dbo].[offline_stalls] WHERE [cha_id] = @cha_id
    END
    ELSE
    BEGIN
        -- Insert new stall
        INSERT INTO [dbo].[offline_stalls] (
            [cha_id], [cha_name], [act_id], [stall_title],
            [look_face], [look_hair], [job], [stall_level],
            [map_name], [pos_x], [pos_y], [angle],
            [expire_time], [item_count], [item_data], [kitbag_snapshot],
            [equip_look_data]
        )
        VALUES (
            @cha_id, @cha_name, @act_id, @stall_title,
            @look_face, @look_hair, @job, @stall_level,
            @map_name, @pos_x, @pos_y, @angle,
            @expire_time, @item_count, @item_data, @kitbag_snapshot,
            @equip_look_data
        )
        
        SELECT SCOPE_IDENTITY() AS [stall_id]
    END
    
    RETURN @@ROWCOUNT
END
GO

-- ============================================
-- STORED PROCEDURE: OfflineStall_Delete
-- ============================================
-- Removes an offline stall by stall_id.
-- ============================================

CREATE PROCEDURE [dbo].[OfflineStall_Delete]
    @stall_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DELETE FROM [dbo].[offline_stalls]
    WHERE [stall_id] = @stall_id
    
    RETURN @@ROWCOUNT
END
GO

-- ============================================
-- STORED PROCEDURE: OfflineStall_DeleteByCharId
-- ============================================
-- Removes an offline stall by character ID.
-- Used when player logs back in.
-- Returns pending_gold that should be given to the character.
-- ============================================

CREATE PROCEDURE [dbo].[OfflineStall_DeleteByCharId]
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    -- Return the pending gold before deleting
    SELECT [pending_gold], [stall_id]
    FROM [dbo].[offline_stalls]
    WHERE [cha_id] = @cha_id AND [is_active] = 1
    
    -- Delete the stall
    DELETE FROM [dbo].[offline_stalls]
    WHERE [cha_id] = @cha_id
    
    RETURN @@ROWCOUNT
END
GO

-- ============================================
-- STORED PROCEDURE: OfflineStall_LoadAll
-- ============================================
-- Loads all active, non-expired stalls for a specific map.
-- Called on GameServer startup to create virtual stall NPCs.
-- ============================================

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

-- ============================================
-- STORED PROCEDURE: OfflineStall_UpdateItems
-- ============================================
-- Updates stall items after a purchase.
-- Also adds gold from the sale to pending_gold.
-- ============================================

CREATE PROCEDURE [dbo].[OfflineStall_UpdateItems]
    @stall_id       INT,
    @item_count     TINYINT,
    @item_data      VARBINARY(8000),
    @gold_earned    BIGINT              -- Gold from this sale
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE [dbo].[offline_stalls]
    SET [item_count] = @item_count,
        [item_data] = @item_data,
        [pending_gold] = [pending_gold] + @gold_earned
    WHERE [stall_id] = @stall_id AND [is_active] = 1
    
    -- If no more items, mark as inactive
    IF @item_count = 0
    BEGIN
        UPDATE [dbo].[offline_stalls]
        SET [is_active] = 0
        WHERE [stall_id] = @stall_id
    END
    
    RETURN @@ROWCOUNT
END
GO

-- ============================================
-- STORED PROCEDURE: OfflineStall_GetByCharId
-- ============================================
-- Checks if a character has an active offline stall.
-- Used during login to restore stall or give pending gold.
-- ============================================

CREATE PROCEDURE [dbo].[OfflineStall_GetByCharId]
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT 
        [stall_id], [cha_name], [stall_title],
        [map_name], [pos_x], [pos_y],
        [created_time], [expire_time],
        [item_count], [item_data], [kitbag_snapshot], [pending_gold],
        [is_active]
    FROM [dbo].[offline_stalls]
    WHERE [cha_id] = @cha_id
    
    RETURN @@ROWCOUNT
END
GO

-- ============================================
-- STORED PROCEDURE: OfflineStall_ExtendTime
-- ============================================
-- Extends the expiration time of a stall (e.g., after a purchase).
-- ============================================

CREATE PROCEDURE [dbo].[OfflineStall_ExtendTime]
    @stall_id       INT,
    @extend_hours   INT             -- Additional hours to add
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE [dbo].[offline_stalls]
    SET [expire_time] = DATEADD(HOUR, @extend_hours, [expire_time])
    WHERE [stall_id] = @stall_id AND [is_active] = 1
    
    RETURN @@ROWCOUNT
END
GO

-- ============================================
-- STORED PROCEDURE: OfflineStall_CleanupExpired
-- ============================================
-- Cleans up expired stalls from the database.
-- Should be run periodically (e.g., daily maintenance).
-- Expired stalls older than @days_old are permanently deleted.
-- ============================================

CREATE PROCEDURE [dbo].[OfflineStall_CleanupExpired]
    @days_old INT = 7               -- Delete expired stalls older than this many days
AS
BEGIN
    SET NOCOUNT ON;
    
    -- First, mark recently expired stalls as inactive (keep for a while for pending gold)
    UPDATE [dbo].[offline_stalls]
    SET [is_active] = 0
    WHERE [expire_time] < GETDATE() AND [is_active] = 1
    
    -- Delete very old expired stalls
    DELETE FROM [dbo].[offline_stalls]
    WHERE [is_active] = 0 
      AND [expire_time] < DATEADD(DAY, -@days_old, GETDATE())
      AND [pending_gold] = 0        -- Only delete if no pending gold
    
    RETURN @@ROWCOUNT
END
GO

-- ============================================
-- VERIFICATION
-- ============================================

PRINT 'Offline Stall System - Database Schema Created Successfully'
PRINT ''
PRINT 'Tables created:'
PRINT '  - dbo.offline_stalls'
PRINT ''
PRINT 'Stored procedures created:'
PRINT '  - dbo.OfflineStall_Create'
PRINT '  - dbo.OfflineStall_Delete'
PRINT '  - dbo.OfflineStall_DeleteByCharId'
PRINT '  - dbo.OfflineStall_LoadAll'
PRINT '  - dbo.OfflineStall_UpdateItems'
PRINT '  - dbo.OfflineStall_GetByCharId'
PRINT '  - dbo.OfflineStall_ExtendTime'
PRINT '  - dbo.OfflineStall_CleanupExpired'
GO
