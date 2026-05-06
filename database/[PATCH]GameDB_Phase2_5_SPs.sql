-- =============================================================================
-- PATCH: GameDB Phase 2.5 - New Stored Procedures
-- Apply to:  GameDB
-- Date:      April 2026
-- Purpose:   Adds 6 stored procedures added during the security/refactoring
--            work that are not yet present on a server created from the
--            original database scripts.
--
-- Safe to run on a live server - each procedure uses IF OBJECT_ID + DROP
-- so it is idempotent (re-runnable without side effects).
-- =============================================================================

USE [GameDB]
GO

-- ---------------------------------------------------------------------------
-- 1. GetMasterDBID
--    Returns the apprentice ID (cha_id2) for a given master ID (cha_id1)
-- ---------------------------------------------------------------------------
IF OBJECT_ID('dbo.GetMasterDBID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetMasterDBID
GO

CREATE PROCEDURE [dbo].[GetMasterDBID]
    @cha_id1 INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT cha_id2 FROM master (NOLOCK) WHERE cha_id1 = @cha_id1;
END
GO

PRINT 'Created stored procedure: GetMasterDBID'
GO

-- ---------------------------------------------------------------------------
-- 2. IsMasterRelation
--    Returns 1 if a master/apprentice relationship exists between two chars
-- ---------------------------------------------------------------------------
IF OBJECT_ID('dbo.IsMasterRelation', 'P') IS NOT NULL
    DROP PROCEDURE dbo.IsMasterRelation
GO

CREATE PROCEDURE [dbo].[IsMasterRelation]
    @cha_id1 INT,
    @cha_id2 INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT count(*) FROM master (NOLOCK) WHERE (cha_id1 = @cha_id1) AND (cha_id2 = @cha_id2);
END
GO

PRINT 'Created stored procedure: IsMasterRelation'
GO

-- ---------------------------------------------------------------------------
-- 3. CalWinTicket
--    Calculates lottery winning ticket numbers for a given issue
-- ---------------------------------------------------------------------------
IF OBJECT_ID('dbo.CalWinTicket', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CalWinTicket
GO

CREATE PROCEDURE [dbo].[CalWinTicket]
    @issue INT,
    @max   INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT TOP 10 itemno, num FROM (
        SELECT itemno, COUNT(*) AS num
        FROM Ticket
        WHERE (issue = @issue) AND real = 0
        GROUP BY itemno
    ) AS A
    WHERE num <= @max
    ORDER BY num;
END
GO

PRINT 'Created stored procedure: CalWinTicket'
GO

-- ---------------------------------------------------------------------------
-- 4. ReadKitbagTmpDataByID
--    Reads kitbag temporary content from the resource table by ID
-- ---------------------------------------------------------------------------
IF OBJECT_ID('dbo.ReadKitbagTmpDataByID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ReadKitbagTmpDataByID
GO

CREATE PROCEDURE [dbo].[ReadKitbagTmpDataByID]
    @id INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT content FROM resource WHERE id = @id;
END
GO

PRINT 'Created stored procedure: ReadKitbagTmpDataByID'
GO

-- ---------------------------------------------------------------------------
-- 5. ShowExpRank
--    Returns top @count characters ranked by experience (handles overflow)
-- ---------------------------------------------------------------------------
IF OBJECT_ID('dbo.ShowExpRank', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ShowExpRank
GO

CREATE PROCEDURE [dbo].[ShowExpRank]
    @count INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT TOP (@count) cha_name, job, degree
    FROM character
    WHERE delflag = 0
    ORDER BY CASE WHEN (exp < 0) THEN (exp + 4294967296) ELSE exp END DESC;
END
GO

PRINT 'Created stored procedure: ShowExpRank'
GO

-- ---------------------------------------------------------------------------
-- 6. IsAmphitheaterTeamLogin
--    Checks whether a given actor is already part of an amphitheater team
-- ---------------------------------------------------------------------------
IF OBJECT_ID('dbo.IsAmphitheaterTeamLogin', 'P') IS NOT NULL
    DROP PROCEDURE dbo.IsAmphitheaterTeamLogin
GO

CREATE PROCEDURE [dbo].[IsAmphitheaterTeamLogin]
    @actor_id INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT count(*) FROM AmphitheaterTeam (NOLOCK)
    WHERE captain = @actor_id
       OR member LIKE CAST(@actor_id AS VARCHAR(20)) + ',%'
       OR member LIKE '%,' + CAST(@actor_id AS VARCHAR(20));
END
GO

PRINT 'Created stored procedure: IsAmphitheaterTeamLogin'
GO

-- ---------------------------------------------------------------------------
-- Done
-- ---------------------------------------------------------------------------
PRINT ''
PRINT 'PATCH complete: 6 stored procedures created/updated in GameDB.'
GO
