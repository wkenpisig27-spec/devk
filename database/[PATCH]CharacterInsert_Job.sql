-- Patch: add @job parameter to CharacterInsert (matches [02]GameDB.sql update)
USE [GameDB]
GO

IF OBJECT_ID('dbo.CharacterInsert', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CharacterInsert
GO

CREATE PROCEDURE [dbo].[CharacterInsert]
    @cha_name VARCHAR(50),
    @act_id INT,
    @birth VARCHAR(20),
    @map VARCHAR(50),
    @look VARCHAR(2000),
    @job VARCHAR(50) = 'Newbie'
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO character (cha_name, act_id, birth, map, look, job)
    VALUES (@cha_name, @act_id, @birth, @map, @look, @job);

    RETURN @@ROWCOUNT;
END
GO

PRINT 'Patched stored procedure: CharacterInsert (@job parameter)'
GO
