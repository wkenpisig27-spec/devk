-- ============================================
-- Economy Analytics System - Database Schema
-- PKODev Project
-- Created: January 2026
-- ============================================
-- This script creates tables and procedures for tracking
-- server economy metrics over time (supply, demand, prices).
-- 
-- Run this script AFTER [1]GameDB.sql has been executed.
-- ============================================

USE [GameDB]
GO

-- ============================================
-- DROP EXISTING OBJECTS (for clean install)
-- ============================================

IF OBJECT_ID('dbo.EconomySnapshot_Create', 'P') IS NOT NULL
    DROP PROCEDURE dbo.EconomySnapshot_Create
GO

IF OBJECT_ID('dbo.EconomySnapshot_GetHistory', 'P') IS NOT NULL
    DROP PROCEDURE dbo.EconomySnapshot_GetHistory
GO

IF OBJECT_ID('dbo.economy_snapshots', 'U') IS NOT NULL
    DROP TABLE dbo.economy_snapshots
GO

IF OBJECT_ID('dbo.economy_item_snapshots', 'U') IS NOT NULL
    DROP TABLE dbo.economy_item_snapshots
GO

-- ============================================
-- TABLE: economy_snapshots
-- ============================================
-- Stores periodic snapshots of overall economy health
-- Run collection script every hour or daily
-- ============================================

CREATE TABLE [dbo].[economy_snapshots] (
    [snapshot_id]       INT IDENTITY(1,1) NOT NULL PRIMARY KEY,
    [snapshot_time]     DATETIME NOT NULL DEFAULT GETDATE(),
    
    -- Overall economy metrics
    [total_gold]        BIGINT NOT NULL DEFAULT 0,          -- Total gold in circulation (all characters)
    [total_guild_gold]  BIGINT NOT NULL DEFAULT 0,          -- Total gold in guild treasuries
    [avg_player_gold]   BIGINT NOT NULL DEFAULT 0,          -- Average gold per player
    [median_player_gold] BIGINT NOT NULL DEFAULT 0,         -- Median gold per player
    
    -- Player metrics
    [total_characters]  INT NOT NULL DEFAULT 0,             -- Total active characters
    [active_stalls]     INT NOT NULL DEFAULT 0,             -- Number of active market stalls
    [total_stall_items] INT NOT NULL DEFAULT 0,             -- Total items listed for sale
    
    -- Shop metrics (web shop)
    [shop_purchases_24h] INT NOT NULL DEFAULT 0,            -- Purchases in last 24 hours
    [shop_revenue_24h]  BIGINT NOT NULL DEFAULT 0,          -- Revenue from shop (credits spent)
    
    -- Calculated indices
    [gini_coefficient]  DECIMAL(5,4) NULL,                  -- Wealth inequality (0=equal, 1=unequal)
    [inflation_index]   DECIMAL(10,4) NULL,                 -- Price change vs baseline
    
    INDEX IX_economy_snapshots_time (snapshot_time DESC)
)
GO

-- ============================================
-- TABLE: economy_item_snapshots
-- ============================================
-- Stores per-item economy data for trend analysis
-- Tracks supply, demand, and prices for specific items
-- ============================================

CREATE TABLE [dbo].[economy_item_snapshots] (
    [id]                INT IDENTITY(1,1) NOT NULL PRIMARY KEY,
    [snapshot_id]       INT NOT NULL,                       -- Links to economy_snapshots
    [item_id]           INT NOT NULL,                       -- Item ID from ItemInfo
    
    -- Supply metrics (items in player inventories)
    [total_supply]      INT NOT NULL DEFAULT 0,             -- Total quantity across all players
    [player_count]      INT NOT NULL DEFAULT 0,             -- Number of players who own this item
    [avg_per_player]    DECIMAL(10,2) NULL,                 -- Average quantity per owner
    
    -- Demand metrics (items for sale / wanted)
    [listed_for_sale]   INT NOT NULL DEFAULT 0,             -- Quantity in market stalls
    [stall_count]       INT NOT NULL DEFAULT 0,             -- Number of stalls selling this
    [avg_asking_price]  BIGINT NULL,                        -- Average price being asked
    [min_asking_price]  BIGINT NULL,                        -- Lowest price
    [max_asking_price]  BIGINT NULL,                        -- Highest price
    
    -- Calculated metrics
    [supply_demand_ratio] DECIMAL(10,4) NULL,               -- supply / demand (>1 = oversupply)
    [scarcity_index]    DECIMAL(10,4) NULL,                 -- Rarity score (higher = rarer)
    [velocity_index]    DECIMAL(10,4) NULL,                 -- How fast item moves (trades/day)
    
    INDEX IX_item_snapshots_snapshot (snapshot_id),
    INDEX IX_item_snapshots_item (item_id),
    INDEX IX_item_snapshots_combo (snapshot_id, item_id),
    
    CONSTRAINT FK_item_snapshots_snapshot 
        FOREIGN KEY (snapshot_id) REFERENCES economy_snapshots(snapshot_id)
        ON DELETE CASCADE
)
GO

-- ============================================
-- PROCEDURE: EconomySnapshot_Create
-- ============================================
-- Creates a new economy snapshot with current data
-- Should be called periodically (hourly/daily)
-- ============================================

CREATE PROCEDURE [dbo].[EconomySnapshot_Create]
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @snapshot_id INT;
    DECLARE @total_gold BIGINT;
    DECLARE @total_guild_gold BIGINT;
    DECLARE @avg_gold BIGINT;
    DECLARE @total_chars INT;
    DECLARE @active_stalls INT;
    DECLARE @total_stall_items INT;
    
    -- Calculate overall metrics
    SELECT 
        @total_gold = ISNULL(SUM(CAST(gd AS BIGINT)), 0),
        @avg_gold = ISNULL(AVG(CAST(gd AS BIGINT)), 0),
        @total_chars = COUNT(*)
    FROM character 
    WHERE delflag = 0;
    
    SELECT @total_guild_gold = ISNULL(SUM(CAST(gold AS BIGINT)), 0)
    FROM guild 
    WHERE guild_name NOT LIKE 'Pirate Guild %';
    
    -- Count active stalls (if table exists)
    IF OBJECT_ID('dbo.offline_stalls', 'U') IS NOT NULL
    BEGIN
        SELECT 
            @active_stalls = COUNT(*),
            @total_stall_items = ISNULL(SUM(item_count), 0)
        FROM offline_stalls 
        WHERE is_active = 1 AND expire_time > GETDATE();
    END
    ELSE
    BEGIN
        SET @active_stalls = 0;
        SET @total_stall_items = 0;
    END
    
    -- Insert snapshot
    INSERT INTO economy_snapshots (
        snapshot_time, total_gold, total_guild_gold, avg_player_gold,
        total_characters, active_stalls, total_stall_items
    ) VALUES (
        GETDATE(), @total_gold, @total_guild_gold, @avg_gold,
        @total_chars, @active_stalls, @total_stall_items
    );
    
    SET @snapshot_id = SCOPE_IDENTITY();
    
    -- Return the snapshot ID
    SELECT @snapshot_id AS snapshot_id;
END
GO

-- ============================================
-- PROCEDURE: EconomySnapshot_GetHistory
-- ============================================
-- Gets economy history for charting
-- ============================================

CREATE PROCEDURE [dbo].[EconomySnapshot_GetHistory]
    @days INT = 30
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT 
        snapshot_id,
        snapshot_time,
        total_gold,
        total_guild_gold,
        avg_player_gold,
        total_characters,
        active_stalls,
        total_stall_items,
        gini_coefficient,
        inflation_index
    FROM economy_snapshots
    WHERE snapshot_time >= DATEADD(DAY, -@days, GETDATE())
    ORDER BY snapshot_time ASC;
END
GO

-- ============================================
-- PROCEDURE: EconomyItemSnapshot_GetTrends
-- ============================================
-- Gets item-specific trends for charting
-- ============================================

CREATE PROCEDURE [dbo].[EconomyItemSnapshot_GetTrends]
    @item_id INT,
    @days INT = 30
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT 
        i.id,
        s.snapshot_time,
        i.item_id,
        i.total_supply,
        i.player_count,
        i.listed_for_sale,
        i.stall_count,
        i.avg_asking_price,
        i.min_asking_price,
        i.max_asking_price,
        i.supply_demand_ratio,
        i.scarcity_index
    FROM economy_item_snapshots i
    INNER JOIN economy_snapshots s ON i.snapshot_id = s.snapshot_id
    WHERE i.item_id = @item_id
      AND s.snapshot_time >= DATEADD(DAY, -@days, GETDATE())
    ORDER BY s.snapshot_time ASC;
END
GO

PRINT 'Economy Analytics tables and procedures created successfully.'
GO
