-- =============================================
-- PKO Website - News System Database Schema
-- Run this on WebsiteDB
-- =============================================

USE [WebsiteDB]
GO

-- =============================================
-- Create News Table
-- =============================================
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[news]') AND type in (N'U'))
BEGIN
    CREATE TABLE [dbo].[news] (
        [id] INT IDENTITY(1,1) PRIMARY KEY,
        [title] NVARCHAR(255) NOT NULL,
        [slug] NVARCHAR(255) NOT NULL UNIQUE,
        [summary] NVARCHAR(500) NULL,
        [content] NVARCHAR(MAX) NOT NULL,
        [image] NVARCHAR(255) NULL,
        [category] NVARCHAR(50) DEFAULT 'update',
        [author] NVARCHAR(100) NOT NULL,
        [author_id] INT NOT NULL,
        [is_published] BIT DEFAULT 1,
        [is_pinned] BIT DEFAULT 0,
        [views] INT DEFAULT 0,
        [created_at] DATETIME DEFAULT GETDATE(),
        [updated_at] DATETIME DEFAULT GETDATE()
    )
    
    PRINT 'News table created successfully'
END
ELSE
BEGIN
    PRINT 'News table already exists'
END
GO

-- Create index on slug for fast lookups
IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name = 'IX_news_slug')
BEGIN
    CREATE UNIQUE INDEX IX_news_slug ON [dbo].[news] ([slug])
END
GO

-- Create index on created_at for sorting
IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name = 'IX_news_created_at')
BEGIN
    CREATE INDEX IX_news_created_at ON [dbo].[news] ([created_at] DESC)
END
GO

-- Create index on is_published for filtering
IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name = 'IX_news_published')
BEGIN
    CREATE INDEX IX_news_published ON [dbo].[news] ([is_published], [created_at] DESC)
END
GO

-- =============================================
-- Stored Procedures
-- =============================================

-- Get all news (with pagination)
IF OBJECT_ID('dbo.News_GetAll', 'P') IS NOT NULL
    DROP PROCEDURE dbo.News_GetAll
GO

CREATE PROCEDURE [dbo].[News_GetAll]
    @PageNumber INT = 1,
    @PageSize INT = 10,
    @PublishedOnly BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @Offset INT = (@PageNumber - 1) * @PageSize;
    
    SELECT 
        id, title, slug, summary, image, category, author, 
        is_published, is_pinned, views, created_at, updated_at
    FROM [dbo].[news]
    WHERE (@PublishedOnly = 0 OR is_published = 1)
    ORDER BY is_pinned DESC, created_at DESC
    OFFSET @Offset ROWS FETCH NEXT @PageSize ROWS ONLY;
    
    -- Also return total count
    SELECT COUNT(*) AS total_count 
    FROM [dbo].[news] 
    WHERE (@PublishedOnly = 0 OR is_published = 1);
END
GO

-- Get news by ID
IF OBJECT_ID('dbo.News_GetById', 'P') IS NOT NULL
    DROP PROCEDURE dbo.News_GetById
GO

CREATE PROCEDURE [dbo].[News_GetById]
    @Id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT * FROM [dbo].[news] WHERE id = @Id;
END
GO

-- Get news by slug
IF OBJECT_ID('dbo.News_GetBySlug', 'P') IS NOT NULL
    DROP PROCEDURE dbo.News_GetBySlug
GO

CREATE PROCEDURE [dbo].[News_GetBySlug]
    @Slug NVARCHAR(255)
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT * FROM [dbo].[news] WHERE slug = @Slug;
END
GO

-- Create news
IF OBJECT_ID('dbo.News_Create', 'P') IS NOT NULL
    DROP PROCEDURE dbo.News_Create
GO

CREATE PROCEDURE [dbo].[News_Create]
    @Title NVARCHAR(255),
    @Slug NVARCHAR(255),
    @Summary NVARCHAR(500),
    @Content NVARCHAR(MAX),
    @Image NVARCHAR(255),
    @Category NVARCHAR(50),
    @Author NVARCHAR(100),
    @AuthorId INT,
    @IsPublished BIT = 1,
    @IsPinned BIT = 0
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO [dbo].[news] (title, slug, summary, content, image, category, author, author_id, is_published, is_pinned)
    VALUES (@Title, @Slug, @Summary, @Content, @Image, @Category, @Author, @AuthorId, @IsPublished, @IsPinned);
    
    SELECT SCOPE_IDENTITY() AS new_id;
END
GO

-- Update news
IF OBJECT_ID('dbo.News_Update', 'P') IS NOT NULL
    DROP PROCEDURE dbo.News_Update
GO

CREATE PROCEDURE [dbo].[News_Update]
    @Id INT,
    @Title NVARCHAR(255),
    @Slug NVARCHAR(255),
    @Summary NVARCHAR(500),
    @Content NVARCHAR(MAX),
    @Image NVARCHAR(255),
    @Category NVARCHAR(50),
    @IsPublished BIT,
    @IsPinned BIT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE [dbo].[news]
    SET title = @Title,
        slug = @Slug,
        summary = @Summary,
        content = @Content,
        image = COALESCE(@Image, image),
        category = @Category,
        is_published = @IsPublished,
        is_pinned = @IsPinned,
        updated_at = GETDATE()
    WHERE id = @Id;
    
    SELECT @@ROWCOUNT AS affected_rows;
END
GO

-- Delete news
IF OBJECT_ID('dbo.News_Delete', 'P') IS NOT NULL
    DROP PROCEDURE dbo.News_Delete
GO

CREATE PROCEDURE [dbo].[News_Delete]
    @Id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DELETE FROM [dbo].[news] WHERE id = @Id;
    
    SELECT @@ROWCOUNT AS affected_rows;
END
GO

-- Increment view count
IF OBJECT_ID('dbo.News_IncrementViews', 'P') IS NOT NULL
    DROP PROCEDURE dbo.News_IncrementViews
GO

CREATE PROCEDURE [dbo].[News_IncrementViews]
    @Id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE [dbo].[news] SET views = views + 1 WHERE id = @Id;
END
GO

-- =============================================
-- Insert sample news articles
-- =============================================
IF NOT EXISTS (SELECT TOP 1 1 FROM [dbo].[news])
BEGIN
    INSERT INTO [dbo].[news] (title, slug, summary, content, category, author, author_id, is_pinned)
    VALUES 
    (
        'Welcome to Slime Pirates Online!',
        'welcome-to-slime-pirates',
        'We are excited to announce the launch of our server. Join us on this epic adventure!',
        '<p>Welcome, adventurers!</p>
<p>We are thrilled to announce the official launch of <strong>Slime Pirates Online</strong>! After months of hard work and preparation, our server is now open for all players.</p>
<h3>What awaits you:</h3>
<ul>
<li>Classic PKO gameplay with modern improvements</li>
<li>Active community and helpful staff</li>
<li>Regular events and updates</li>
<li>Fair economy and balanced gameplay</li>
</ul>
<p>Join our Discord community to stay updated and meet fellow pirates!</p>',
        'announcement',
        'Admin',
        1,
        1
    ),
    (
        'Server Maintenance - February 2026',
        'server-maintenance-february-2026',
        'Scheduled maintenance for server improvements and bug fixes.',
        '<p>Dear players,</p>
<p>We will be performing scheduled maintenance on <strong>February 10, 2026</strong> from 2:00 AM to 4:00 AM UTC.</p>
<h3>What''s being updated:</h3>
<ul>
<li>Server stability improvements</li>
<li>Bug fixes for quest system</li>
<li>Performance optimizations</li>
</ul>
<p>Thank you for your patience and understanding!</p>',
        'maintenance',
        'Admin',
        1,
        0
    ),
    (
        'Double EXP Weekend Event!',
        'double-exp-weekend-event',
        'Enjoy double experience points this weekend! Level up faster than ever.',
        '<p>Get ready for an exciting weekend!</p>
<p>From <strong>Friday 6 PM</strong> to <strong>Sunday 11:59 PM UTC</strong>, all players will receive <strong>2x EXP</strong> from all sources!</p>
<h3>Event Details:</h3>
<ul>
<li>2x Character EXP</li>
<li>2x Skill EXP</li>
<li>Applies to all maps and monsters</li>
</ul>
<p>Don''t miss this opportunity to level up your characters!</p>',
        'event',
        'Admin',
        1,
        0
    )
    
    PRINT 'Sample news articles inserted'
END
GO

PRINT 'News system setup complete!'
GO
