USE [AccountServer]
GO

-- Password reset tokens table
-- Stores hashed one-time tokens for the forgot-password flow.
-- The plain token is only ever sent to the user via email; only its SHA-256
-- hash is persisted here so a database breach cannot be used to reset accounts.

IF OBJECT_ID('dbo.password_reset_tokens', 'U') IS NULL
BEGIN
    CREATE TABLE [dbo].[password_reset_tokens] (
        [id]         INT           IDENTITY(1,1) NOT NULL,
        [username]   VARCHAR(32)   NOT NULL,
        [token_hash] CHAR(64)      NOT NULL,        -- SHA-256 hex of the plain token
        [expires_at] DATETIME      NOT NULL,
        [used]       BIT           NOT NULL CONSTRAINT [DF_prt_used]       DEFAULT 0,
        [created_at] DATETIME      NOT NULL CONSTRAINT [DF_prt_created_at] DEFAULT GETDATE(),

        CONSTRAINT [PK_password_reset_tokens] PRIMARY KEY CLUSTERED ([id]),
        CONSTRAINT [UQ_password_reset_tokens_hash] UNIQUE ([token_hash])
    );

    -- Index to speed up lookup by username (invalidating previous tokens)
    CREATE INDEX [IX_prt_username] ON [dbo].[password_reset_tokens] ([username]);

    PRINT 'password_reset_tokens table created successfully.';
END
ELSE
BEGIN
    PRINT 'password_reset_tokens table already exists, skipping.';
END
GO

-- Grant required permissions to the web user
GRANT SELECT, INSERT, UPDATE ON dbo.password_reset_tokens TO pko_web;
PRINT 'Permissions granted to pko_web.';
GO
