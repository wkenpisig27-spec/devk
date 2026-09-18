-- =============================================
-- FIX: Add missing indexes to the friends table
--
-- The friends table may be a heap with no indexes. GroupServer runs
-- friend-list queries on every login; without indexes these become full
-- table scans that cause lock storms and HYT00 timeouts under load.
--
-- RUN ON: GameDB_devk (once, idempotent)
-- =============================================

USE [GameDB_devk]
GO

PRINT '=== Adding indexes to friends table ==='
PRINT ''

IF NOT EXISTS (
    SELECT 1 FROM sys.indexes
    WHERE object_id = OBJECT_ID('dbo.friends')
      AND name = 'IX_friends_cha_id1'
)
BEGIN
    CREATE NONCLUSTERED INDEX [IX_friends_cha_id1]
        ON [dbo].[friends] ([cha_id1] ASC)
        INCLUDE ([cha_id2], [relation]);
    PRINT 'Created index: IX_friends_cha_id1'
END
ELSE
    PRINT 'Index already exists: IX_friends_cha_id1'
GO

IF NOT EXISTS (
    SELECT 1 FROM sys.indexes
    WHERE object_id = OBJECT_ID('dbo.friends')
      AND name = 'IX_friends_cha_id1_id2'
)
BEGIN
    CREATE NONCLUSTERED INDEX [IX_friends_cha_id1_id2]
        ON [dbo].[friends] ([cha_id1] ASC, [cha_id2] ASC)
        INCLUDE ([relation]);
    PRINT 'Created index: IX_friends_cha_id1_id2'
END
ELSE
    PRINT 'Index already exists: IX_friends_cha_id1_id2'
GO

IF NOT EXISTS (
    SELECT 1 FROM sys.indexes
    WHERE object_id = OBJECT_ID('dbo.friends')
      AND name = 'IX_friends_cha_id2'
)
BEGIN
    CREATE NONCLUSTERED INDEX [IX_friends_cha_id2]
        ON [dbo].[friends] ([cha_id2] ASC)
        INCLUDE ([cha_id1], [relation]);
    PRINT 'Created index: IX_friends_cha_id2'
END
ELSE
    PRINT 'Index already exists: IX_friends_cha_id2'
GO

PRINT ''
PRINT '=== Done ==='
GO
