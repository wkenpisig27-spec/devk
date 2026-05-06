-- =============================================
-- Script: PlayerLoginInfo
-- Description: Creates a stored procedure in GameDB that reads
--              player IP and MAC from the AccountServer database.
-- 
-- Prerequisites:
--   - AccountServer database must exist on the same SQL Server instance
--   - The GameDB SQL login must have SELECT permission on
--     [AccountServer].[dbo].[account_login]
--
-- How the data gets there:
--   1. Player launches the client and connects to GateServer
--   2. Client sends its MAC address in the login packet
--   3. GateServer forwards the MAC + client IP to AccountServer
--   4. AccountServer calls [AccountServer].[dbo].[AccountLogin]
--      which writes last_login_ip and last_login_mac into account_login
--   5. This procedure simply READS that existing data
--
-- Usage from Lua (after C++ rebuild):
--   local ip, mac = GetPlayerLoginInfo(role)
-- =============================================

USE [GameDB]
GO

-- =============================================
-- Step 1: Create the stored procedure
-- =============================================
IF OBJECT_ID('dbo.GetPlayerLoginInfo', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetPlayerLoginInfo
GO

CREATE PROCEDURE [dbo].[GetPlayerLoginInfo]
    @act_name VARCHAR(50)
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

PRINT 'Created stored procedure: [GameDB].[dbo].[GetPlayerLoginInfo]'
GO

-- =============================================
-- Step 2: Grant cross-database SELECT permission
-- Uncomment and replace 'sa' with your GameServer SQL login
-- if you get a permission error.
-- =============================================
-- USE [AccountServer]
-- GO
-- CREATE USER [pko_game] FOR LOGIN [pko_game]
-- GO
-- GRANT SELECT ON [dbo].[account_login] TO [pko_game]
-- GO