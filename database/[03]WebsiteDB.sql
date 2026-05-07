USE [master]
GO
CREATE DATABASE [WebsiteDB]
GO
USE [WebsiteDB]
GO
SET
ANSI_NULLS ON
GO
SET
QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[LarryEdit](
	[TuoNimi] [char](50) NOT NULL,
	[TavaraHinta] [int] NOT NULL,
	[TavaraTeksti] [text] NOT NULL,
	[Quota] [int] NOT NULL,
	[Icon] [char](10) NOT NULL,
	[MyyniTyyppi] [char](10) NOT NULL,
	[TavaraID] [int] NOT NULL,
	[MontaTavaraa] [int] NOT NULL,
	[category] [char](15) NOT NULL,
	[TavaraListaID] [int] IDENTITY(1,1) NOT NULL,
	[bought] [int] NOT NULL
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[LarryLaatikko](
	[Tavara_id] [int] IDENTITY(1,1) NOT NULL,
	[act_id] [int] NOT NULL,
	[item_id] [char](20) NOT NULL,
	[assigned] [int] NOT NULL,
	[assigned_char] [char](30) NULL,
	[assigned_date] [datetime] NULL,
	[Icon] [char](10) NOT NULL,
	[TuoNimi] [char](30) NULL,
	[MontaTavaraa] [int] NOT NULL
) ON [PRIMARY]
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[vote](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[name] [char](20) NOT NULL,
	[prize] [int] NOT NULL,
	[link] [text] NOT NULL,
	[image] [text] NOT NULL
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[vote_log](
	[ip] [char](30) NOT NULL,
	[mac] [varchar](50) NULL,
	[act_id] [int] NOT NULL,
	[account] [char](10) NOT NULL,
	[id] [char](10) NOT NULL,
	[date] [datetime] NOT NULL
) ON [PRIMARY]
GO
USE [master]
GO
ALTER DATABASE [WebsiteDB] SET  READ_WRITE 
GO