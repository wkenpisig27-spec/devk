-- Item audit trail for economy dup investigation (Batch 7)
-- Run against GameDB_devk after deployment

USE [GameDB_devk]
GO

IF OBJECT_ID(N'dbo.item_audit_log', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.item_audit_log (
        id BIGINT IDENTITY(1,1) NOT NULL PRIMARY KEY,
        ts DATETIME2 NOT NULL CONSTRAINT DF_item_audit_log_ts DEFAULT (GETDATE()),
        cha_id INT NOT NULL,
        action VARCHAR(32) NOT NULL,
        item_dbid INT NOT NULL,
        item_id INT NOT NULL,
        qty INT NOT NULL,
        counterparty INT NULL,
        gold_delta BIGINT NULL,
        detail NVARCHAR(256) NULL
    );
    CREATE INDEX IX_item_audit_log_item_dbid ON dbo.item_audit_log (item_dbid);
    CREATE INDEX IX_item_audit_log_cha_id ON dbo.item_audit_log (cha_id, ts DESC);
END
GO

IF OBJECT_ID(N'dbo.ItemAuditInsert', N'P') IS NOT NULL
    DROP PROCEDURE dbo.ItemAuditInsert;
GO

CREATE PROCEDURE dbo.ItemAuditInsert
    @cha_id INT,
    @action VARCHAR(32),
    @item_dbid INT,
    @item_id INT,
    @qty INT,
    @counterparty INT = NULL,
    @gold_delta BIGINT = NULL,
    @detail NVARCHAR(256) = NULL
AS
BEGIN
    SET NOCOUNT ON;
    IF @counterparty IS NOT NULL AND @counterparty < 0
        SET @counterparty = NULL;

    INSERT INTO dbo.item_audit_log (cha_id, action, item_dbid, item_id, qty, counterparty, gold_delta, detail)
    VALUES (@cha_id, @action, @item_dbid, @item_id, @qty,
            @counterparty, @gold_delta, @detail);
END
GO
