-- =============================================
-- FIX: MapMaskSaveData stored procedure - table name typo
-- Date: January 30, 2026
-- Issue: The stored procedure referenced 'mapmask' but the table is 'map_mask'
-- This caused "Invalid object name 'mapmask'" errors leading to server crash
-- =============================================

USE [GameDB]
GO

-- Recreate the stored procedure with correct table name
IF OBJECT_ID('dbo.MapMaskSaveData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.MapMaskSaveData
GO

CREATE PROCEDURE [dbo].[MapMaskSaveData]
    @colIndex INT,
    @maskData VARCHAR(MAX),
    @dbId INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @sql NVARCHAR(MAX);
    DECLARE @colName VARCHAR(20);
    
    SET @colName = CASE @colIndex
        WHEN 1 THEN 'content1'
        WHEN 2 THEN 'content2'
        WHEN 3 THEN 'content3'
        WHEN 4 THEN 'content4'
        ELSE NULL
    END;
    
    IF @colName IS NULL
    BEGIN
        RAISERROR('Invalid column index', 16, 1);
        RETURN -1;
    END
    
    -- FIXED: Changed 'mapmask' to 'map_mask' (correct table name)
    SET @sql = N'UPDATE map_mask SET ' + QUOTENAME(@colName) + N' = @maskDataParam WHERE id = @dbIdParam';
    
    EXEC sp_executesql @sql, 
        N'@maskDataParam VARCHAR(MAX), @dbIdParam INT',
        @maskDataParam = @maskData,
        @dbIdParam = @dbId;
    
    RETURN @@ROWCOUNT;
END
GO

PRINT 'MapMaskSaveData stored procedure fixed successfully!'
GO
