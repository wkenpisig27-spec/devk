-- =============================================
-- FIX: GetPlayerLoginInfo cross-DB access permissions
--
-- Run on live servers where GetPlayerLoginInfo returns empty rows or
-- Msg 33009 SID mismatch after a database restore.
--
-- RUN ON: master + GameDB + AccountServer
-- =============================================

USE [master]
GO

ALTER AUTHORIZATION ON DATABASE::[GameDB] TO [sa];
PRINT 'Reset GameDB owner to sa (fixes Msg 33009 SID mismatch)'
GO

IF (SELECT is_trustworthy_on FROM sys.databases WHERE name = 'GameDB') = 0
BEGIN
    ALTER DATABASE [GameDB] SET TRUSTWORTHY ON;
    PRINT 'Set GameDB TRUSTWORTHY ON'
END
ELSE
    PRINT 'GameDB already TRUSTWORTHY ON'
GO

USE [GameDB]
GO

IF OBJECT_ID('dbo.GetPlayerLoginInfo', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetPlayerLoginInfo
GO

CREATE PROCEDURE [dbo].[GetPlayerLoginInfo]
    @act_name VARCHAR(50)
WITH EXECUTE AS OWNER
AS
BEGIN
    SET NOCOUNT ON;
    SELECT TOP 1
        ISNULL(al.last_login_ip, '') AS last_login_ip,
        ISNULL(al.last_login_mac, '') AS last_login_mac
    FROM [AccountServer].[dbo].[account_login] al
    WHERE al.[name] = @act_name;
END
GO

GRANT EXECUTE ON [dbo].[GetPlayerLoginInfo] TO [pko_game];
GO

-- Belt-and-suspenders: direct SELECT grant on AccountServer
USE [AccountServer]
GO

IF EXISTS (SELECT 1 FROM sys.database_principals WHERE name = 'pko_game')
BEGIN
    GRANT SELECT ON [dbo].[account_login] TO [pko_game];
    PRINT 'Granted SELECT on account_login to pko_game'
END
GO

USE [GameDB]
GO

IF EXISTS (SELECT 1 FROM sys.database_principals WHERE name = 'pko_game')
BEGIN
    GRANT VIEW DEFINITION TO [pko_game];
    PRINT 'Granted VIEW DEFINITION to pko_game (ODBC SP parameter caching)'
END
GO

PRINT '=== GetPlayerLoginInfo fix complete ==='
GO
