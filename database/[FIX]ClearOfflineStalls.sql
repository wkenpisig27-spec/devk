-- =============================================================================
-- Fix Script: Clear Offline Stalls After Structure Change
-- Date: January 12, 2026
-- =============================================================================
-- This script clears all offline stalls from the database after changing the
-- SOfflineStallItem structure to include full SItemGrid data.
-- 
-- Old structure was ~40 bytes per item, new structure is ~200+ bytes per item.
-- Existing data is incompatible and must be cleared.
--
-- NOTE: This will delete all offline stalls! Players will need to recreate them.
-- =============================================================================

USE GameDB
GO

-- Clear all offline stalls
DELETE FROM [dbo].[offline_stalls]
GO

PRINT 'All offline stalls have been cleared due to structure change.'
PRINT 'Players will need to create new offline stalls.'
GO
