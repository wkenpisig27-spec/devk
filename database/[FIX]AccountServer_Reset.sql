-- =============================================
-- FIX: AccountServer account_login table reset
-- This fixes the "Unknown exception raised from AuthThread::ResetAccount()" error
-- =============================================

USE AccountServer;
GO

-- Verify account_login table exists
IF OBJECT_ID('dbo.account_login', 'U') IS NOT NULL
BEGIN
    PRINT 'Clearing stale login sessions...';
    
    -- Reset all accounts to offline state (same as what ResetAccount() tries to do)
    UPDATE account_login 
    SET login_status = 0,  -- ACCOUNT_OFFLINE
        sid = -1,          -- INVALID_SID
        enable_login_time = GETDATE(),
        login_group = '';
    
    PRINT 'All login sessions cleared successfully';
END
ELSE
BEGIN
    PRINT 'Warning: account_login table does not exist';
    PRINT 'Please run [0]AccountServer.sql to create the database schema';
END
GO

PRINT 'AccountServer database reset complete!';
GO
