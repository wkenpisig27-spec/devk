USE [master]
GO
CREATE DATABASE [GameDB]
GO
USE [GameDB]
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[account] (
  [act_id] [int] NOT NULL,
  [act_name] [varchar](50) NOT NULL,
  [gm] [tinyint] NOT NULL,
  [cha_ids] [varchar](80) NOT NULL,
  [last_ip] [varchar](16) NOT NULL,
  [disc_reason] [varchar](128) NOT NULL,
  [last_leave] [datetime] NOT NULL,
  [password] [varchar](50) NULL,
  [merge_state] [int] NOT NULL,
  [total_votes] [int] NOT NULL,
  [credit] [int] NOT NULL,
  CONSTRAINT [PK_account] PRIMARY KEY CLUSTERED
  (
  [act_id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[AmphitheaterSetting] (
  [section] [smallint] NOT NULL,
  [season] [smallint] NOT NULL,
  [round] [smallint] NOT NULL,
  [state] [smallint] NOT NULL,
  [createdate] [datetime] NOT NULL,
  [updatetime] [datetime] NULL,
  [winner] [varchar](100) NULL
) ON [PRIMARY]

GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[AmphitheaterTeam] (
  [id] [int] NOT NULL,
  [captain] [int] NULL,
  [member] [varchar](50) NULL,
  [matchno] [int] NOT NULL,
  [state] [smallint] NOT NULL,
  [map] [smallint] NULL,
  [mapflag] [int] NULL,
  [winnum] [smallint] NOT NULL,
  [losenum] [smallint] NOT NULL,
  [relivenum] [smallint] NOT NULL,
  [createdate] [datetime] NULL,
  [updatetime] [datetime] NOT NULL,
  CONSTRAINT [PK_AmphitheaterTeam] PRIMARY KEY CLUSTERED
  (
  [id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[boat] (
  [boat_id] [int] IDENTITY (1, 1) NOT NULL,
  [boat_berth] [smallint] NOT NULL,
  [boat_name] [char](17) NOT NULL,
  [boat_boatid] [int] NOT NULL,
  [boat_header] [int] NOT NULL,
  [boat_body] [int] NOT NULL,
  [boat_engine] [int] NOT NULL,
  [boat_cannon] [int] NOT NULL,
  [boat_equipment] [int] NOT NULL,
  [boat_bagsize] [smallint] NOT NULL,
  [boat_bag] [char](7000) NOT NULL,
  [boat_diecount] [smallint] NOT NULL,
  [boat_isdead] [char](1) NOT NULL,
  [cur_endure] [int] NOT NULL,
  [mx_endure] [int] NOT NULL,
  [cur_supply] [int] NOT NULL,
  [mx_supply] [int] NOT NULL,
  [skill_state] [char](400) NOT NULL,
  [boat_ownerid] [int] NOT NULL,
  [boat_createtime] [char](50) NOT NULL,
  [boat_isdeleted] [char](1) NOT NULL,
  [map] [char](50) NOT NULL,
  [map_x] [int] NOT NULL,
  [map_y] [int] NOT NULL,
  [angle] [int] NOT NULL,
  [degree] [smallint] NOT NULL,
  [exp] [int] NOT NULL,
  [version] [smallint] NOT NULL,
  CONSTRAINT [PK_boat] PRIMARY KEY CLUSTERED
  (
  [boat_id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[character] (
  [cha_id] [int] IDENTITY (1, 1) NOT NULL,
  [cha_name] [varchar](50) NOT NULL,
  [motto] [varchar](50) NOT NULL,
  [icon] [smallint] NOT NULL,
  [version] [smallint] NOT NULL,
  [pk_ctrl] [tinyint] NOT NULL,
  [mem_addr] [bigint] NOT NULL,
  [act_id] [int] NOT NULL,
  [guild_id] [int] NOT NULL,
  [guild_stat] [tinyint] NOT NULL,
  [guild_permission] [bigint] NOT NULL,
  [job] [varchar](50) NOT NULL,
  [degree] [smallint] NOT NULL,
  [exp] [bigint] NOT NULL,
  [hp] [int] NOT NULL,
  [sp] [int] NOT NULL,
  [ap] [int] NOT NULL,
  [tp] [int] NOT NULL,
  [gd] [bigint] NOT NULL,
  [str] [int] NOT NULL,
  [dex] [int] NOT NULL,
  [agi] [int] NOT NULL,
  [con] [int] NOT NULL,
  [sta] [int] NOT NULL,
  [luk] [int] NOT NULL,
  [sail_lv] [int] NOT NULL,
  [sail_exp] [int] NOT NULL,
  [sail_left_exp] [int] NOT NULL,
  [live_lv] [int] NOT NULL,
  [live_exp] [int] NOT NULL,
  [map] [varchar](50) NOT NULL,
  [map_x] [int] NOT NULL,
  [map_y] [int] NOT NULL,
  [radius] [int] NOT NULL,
  [angle] [int] NOT NULL,
  [look] [varchar](2000) NOT NULL,
  [kb_capacity] [int] NOT NULL,
  [kitbag] [varchar](7000) NOT NULL,
  [skillbag] [varchar](1200) NOT NULL,
  [shortcut] [varchar](1200) NOT NULL,
  [mission] [varchar](2048) NOT NULL,
  [misrecord] [varchar](2048) NOT NULL,
  [mistrigger] [varchar](2048) NOT NULL,
  [miscount] [varchar](512) NOT NULL,
  [birth] [varchar](50) NOT NULL,
  [login_cha] [varchar](50) NOT NULL,
  [live_tp] [int] NOT NULL,
  [map_mask] [varchar](8000) NOT NULL,
  [delflag] [tinyint] NOT NULL,
  [operdate] [datetime] NOT NULL,
  [deldate] [datetime] NULL,
  [main_map] [varchar](50) NOT NULL,
  [skill_state] [varchar](1024) NOT NULL,
  [bank] [varchar](50) NOT NULL,
  [estop] [datetime] NOT NULL,
  [estoptime] [int] NOT NULL,
  [kb_locked] [int] NOT NULL,
  [kitbag_tmp] [int] NOT NULL,
  [credit] [int] NOT NULL,
  [store_item] [int] NOT NULL,
  [extend] [varchar](2048) NULL,
  [chatColour] [int] NOT NULL,
  [IMP] [int] NOT NULL,
  [battle_power] [int] NOT NULL,
  CONSTRAINT [PK_character] PRIMARY KEY CLUSTERED
  (
  [cha_id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY],
  CONSTRAINT [IX_character] UNIQUE NONCLUSTERED
  (
  [cha_name] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[character_log] (
  [cha_id] [int] NOT NULL,
  [cha_name] [varchar](50) NOT NULL,
  [act_id] [int] NOT NULL,
  [guild_id] [int] NOT NULL,
  [job] [varchar](50) NOT NULL,
  [degree] [smallint] NOT NULL,
  [exp] [int] NOT NULL,
  [hp] [int] NOT NULL,
  [sp] [int] NOT NULL,
  [ap] [int] NOT NULL,
  [tp] [int] NOT NULL,
  [gd] [bigint] NOT NULL,
  [str] [int] NOT NULL,
  [dex] [int] NOT NULL,
  [agi] [int] NOT NULL,
  [con] [int] NOT NULL,
  [sta] [int] NOT NULL,
  [luk] [int] NOT NULL,
  [map] [varchar](50) NOT NULL,
  [map_x] [int] NOT NULL,
  [map_y] [int] NOT NULL,
  [radius] [int] NOT NULL,
  [look] [varchar](80) NOT NULL,
  [del_date] [datetime] NOT NULL
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[friends] (
  [cha_id1] [int] NOT NULL,
  [cha_id2] [int] NOT NULL,
  [relation] [varchar](50) NOT NULL,
  [createtime] [datetime] NULL
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[guild] (
  [guild_id] [int] NOT NULL,
  [guild_name] [varchar](16) NOT NULL,
  [motto] [varchar](50) NOT NULL,
  [passwd] [varchar](20) NOT NULL,
  [leader_id] [int] NOT NULL,
  [exp] [bigint] NOT NULL,
  [gold] [bigint] NOT NULL,
  [bank] [varchar](8000) NOT NULL,
  [banklog] [nvarchar](MAX) NULL,
  [level] [int] NOT NULL,
  [member_total] [smallint] NOT NULL,
  [try_total] [smallint] NOT NULL,
  [disband_date] [datetime] NOT NULL,
  [challlevel] [smallint] NOT NULL,
  [challid] [int] NOT NULL,
  [challmoney] [bigint] NOT NULL,
  [challstart] [smallint] NOT NULL,
  CONSTRAINT [PK_guild] PRIMARY KEY CLUSTERED
  (
  [guild_id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY],
  CONSTRAINT [IX_guild_name] UNIQUE NONCLUSTERED
  (
  [guild_name] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[LotterySetting] (
  [section] [smallint] NOT NULL,
  [issue] [smallint] NOT NULL,
  [state] [smallint] NOT NULL,
  [createdate] [datetime] NOT NULL,
  [updatetime] [datetime] NULL,
  [itemno] [varchar](50) NULL,
  CONSTRAINT [PK_LotterySetting] PRIMARY KEY CLUSTERED
  (
  [section] ASC,
  [issue] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[map_mask] (
  [id] [int] IDENTITY (1, 1) NOT NULL,
  [cha_id] [int] NOT NULL,
  [content1] [char](600) NOT NULL,
  [content2] [char](600) NOT NULL,
  [content3] [char](600) NOT NULL,
  [content4] [char](600) NOT NULL,
  [content5] [char](600) NOT NULL,
  CONSTRAINT [PK_map_mask] PRIMARY KEY CLUSTERED
  (
  [id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY],
  CONSTRAINT [IX_map_mask] UNIQUE NONCLUSTERED
  (
  [id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[master] (
  [cha_id1] [int] NOT NULL,
  [cha_id2] [int] NOT NULL,
  [finish] [int] NOT NULL,
  [relation] [varchar](50) NOT NULL
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[param] (
  [id] [int] IDENTITY (1, 1) NOT NULL,
  [param1] [int] NULL,
  [param2] [int] NULL,
  [param3] [int] NULL,
  [param4] [int] NULL,
  [param5] [int] NULL,
  [param6] [int] NULL,
  [param7] [int] NULL,
  [param8] [int] NULL,
  [param9] [int] NULL,
  [param10] [int] NULL,
  CONSTRAINT [PK_param] PRIMARY KEY CLUSTERED
  (
  [id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[personavatar] (
  [cha_id] [int] NOT NULL,
  [avatar] [image] NULL,
  CONSTRAINT [PK_psersonavatar] PRIMARY KEY CLUSTERED
  (
  [cha_id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]

GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[personinfo] (
  [cha_id] [int] NOT NULL,
  [motto] [varchar](40) NULL,
  [showmotto] [bit] NULL,
  [sex] [varchar](50) NULL,
  [age] [int] NULL,
  [name] [varchar](50) NULL,
  [animal_zodiac] [varchar](50) NULL,
  [blood_type] [varchar](50) NULL,
  [birthday] [int] NULL,
  [state] [varchar](50) NULL,
  [city] [varchar](50) NULL,
  [constellation] [varchar](50) NULL,
  [career] [varchar](50) NULL,
  [avatarsize] [int] NULL,
  [prevent] [bit] NULL,
  [support] [int] NULL,
  [oppose] [int] NULL,
  CONSTRAINT [PK_personinfo] PRIMARY KEY CLUSTERED
  (
  [cha_id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[property] (
  [id] [bigint] NOT NULL,
  [cha_id] [bigint] NOT NULL,
  [context] [varchar](255) NOT NULL,
  [sum] [bigint] NOT NULL,
  [time] [datetime] NOT NULL,
  CONSTRAINT [PK_property] PRIMARY KEY CLUSTERED
  (
  [id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[Resource] (
  [id] [int] IDENTITY (1, 1) NOT NULL,
  [cha_id] [int] NOT NULL,
  [type_id] [smallint] NOT NULL,
  [content] [varchar](MAX) NOT NULL,
  CONSTRAINT [PK_Resource] PRIMARY KEY CLUSTERED
  (
  [id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[stat_log] (
  [track_date] [datetime] NOT NULL,
  [login_num] [int] NOT NULL,
  [play_num] [int] NOT NULL,
  [wgplay_num] [int] NULL,
  CONSTRAINT [PK_stat_log] PRIMARY KEY CLUSTERED
  (
  [track_date] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[StatDegree] (
  [statDate] [datetime] NOT NULL,
  [degree] [smallint] NOT NULL,
  [characterCount] [bigint] NULL,
  CONSTRAINT [PK_StatDegree] PRIMARY KEY CLUSTERED
  (
  [statDate] ASC,
  [degree] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[StatGender] (
  [statDate] [datetime] NOT NULL,
  [gender] [varchar](8) NOT NULL,
  [genderCount] [bigint] NULL,
  CONSTRAINT [PK_StatGender] PRIMARY KEY CLUSTERED
  (
  [statDate] ASC,
  [gender] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[StatJob] (
  [statDate] [datetime] NOT NULL,
  [job] [varchar](50) NOT NULL,
  [characterCount] [bigint] NULL,
  CONSTRAINT [PK_StatJob] PRIMARY KEY CLUSTERED
  (
  [statDate] ASC,
  [job] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[StatLogin] (
  [statDate] [datetime] NOT NULL,
  [loginCount] [bigint] NULL,
  CONSTRAINT [PK_StatLogin] PRIMARY KEY CLUSTERED
  (
  [statDate] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[StatMap] (
  [statDate] [datetime] NOT NULL,
  [map] [varchar](50) NOT NULL,
  [playCount] [bigint] NULL,
  CONSTRAINT [PK_StatMap] PRIMARY KEY CLUSTERED
  (
  [statDate] ASC,
  [map] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Ticket] (
  [id] [int] IDENTITY (1, 1) NOT NULL,
  [cha_id] [int] NOT NULL,
  [issue] [int] NOT NULL,
  [itemno] [varchar](50) NOT NULL,
  [real] [bit] NOT NULL,
  [buydate] [datetime] NOT NULL,
  CONSTRAINT [PK_Ticket] PRIMARY KEY CLUSTERED
  (
  [id] ASC
  ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Trade_Log] (
  [ID] [int] IDENTITY (1, 1) NOT NULL,
  [ExecuteTime] [datetime] NOT NULL,
  [GameServer] [varchar](50) NOT NULL,
  [Action] [varchar](50) NOT NULL,
  [From] [varchar](50) NOT NULL,
  [To] [varchar](50) NULL,
  [Memo] [varchar](1000) NULL
) ON [PRIMARY]

GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[weekreport] (
  [act_name] [varchar](50) NULL,
  [cha_name] [varchar](50) NULL,
  [degree] [int] NULL,
  [ip] [varchar](20) NULL,
  [createdate] [datetime] NULL,
  [logouttime] [datetime] NULL,
  [playtime] [int] NULL,
  [Guild_Name] [varchar](16) NULL
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[WinTicket] (
  [issue] [smallint] NOT NULL,
  [itemno] [varchar](10) NOT NULL,
  [grade] [smallint] NOT NULL,
  [createdate] [datetime] NOT NULL,
  [num] [smallint] NOT NULL
) ON [PRIMARY]

GO
ALTER TABLE [dbo].[account] ADD CONSTRAINT [DF_account_act_name] DEFAULT ('') FOR [act_name]
GO
ALTER TABLE [dbo].[account] ADD CONSTRAINT [DF_account_gm] DEFAULT ((0)) FOR [gm]
GO
ALTER TABLE [dbo].[account] ADD CONSTRAINT [DF_account_cha_ids] DEFAULT ('') FOR [cha_ids]
GO
ALTER TABLE [dbo].[account] ADD CONSTRAINT [DF_account_last_ip] DEFAULT ('') FOR [last_ip]
GO
ALTER TABLE [dbo].[account] ADD CONSTRAINT [DF_account_disc_reson] DEFAULT ('') FOR [disc_reason]
GO
ALTER TABLE [dbo].[account] ADD CONSTRAINT [DF_account_last_leave] DEFAULT ('2001-01-01') FOR [last_leave]
GO
ALTER TABLE [dbo].[account] ADD CONSTRAINT [DF_account_merge_state] DEFAULT ((0)) FOR [merge_state]
GO
ALTER TABLE [dbo].[account] ADD CONSTRAINT [DF_account_total_votes] DEFAULT ((0)) FOR [total_votes]
GO
ALTER TABLE [dbo].[account] ADD CONSTRAINT [DF_account_credit] DEFAULT ((0)) FOR [credit]
GO
ALTER TABLE [dbo].[AmphitheaterTeam] ADD CONSTRAINT [DF_AmphitheaterTeam_matchno] DEFAULT ((0)) FOR [matchno]
GO
ALTER TABLE [dbo].[AmphitheaterTeam] ADD CONSTRAINT [DF_AmphitheaterTeam_state] DEFAULT ((0)) FOR [state]
GO
ALTER TABLE [dbo].[AmphitheaterTeam] ADD CONSTRAINT [DF_AmphitheaterTeam_winnum] DEFAULT ((0)) FOR [winnum]
GO
ALTER TABLE [dbo].[AmphitheaterTeam] ADD CONSTRAINT [DF_AmphitheaterTeam_losenum] DEFAULT ((0)) FOR [losenum]
GO
ALTER TABLE [dbo].[AmphitheaterTeam] ADD CONSTRAINT [DF_AmphitheaterTeam_relivenum] DEFAULT ((0)) FOR [relivenum]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_berth] DEFAULT ((0)) FOR [boat_berth]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_name] DEFAULT ('') FOR [boat_name]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_boat] DEFAULT ((0)) FOR [boat_boatid]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_header] DEFAULT ((0)) FOR [boat_header]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_body] DEFAULT ((0)) FOR [boat_body]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_engine] DEFAULT ((0)) FOR [boat_engine]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_cannon] DEFAULT ((0)) FOR [boat_cannon]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_part] DEFAULT ((0)) FOR [boat_equipment]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_bagsize] DEFAULT ((0)) FOR [boat_bagsize]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_bag] DEFAULT ('') FOR [boat_bag]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_diecount] DEFAULT ((0)) FOR [boat_diecount]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_isdie] DEFAULT ((0)) FOR [boat_isdead]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_cur_endure] DEFAULT ((0)) FOR [cur_endure]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_mx_endure] DEFAULT ((0)) FOR [mx_endure]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_cur_supply] DEFAULT ((0)) FOR [cur_supply]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_mx_supply] DEFAULT ((0)) FOR [mx_supply]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_skill_state] DEFAULT ('') FOR [skill_state]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_ownerid] DEFAULT ((0)) FOR [boat_ownerid]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_createtime] DEFAULT ('') FOR [boat_createtime]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_boat_isdeleted] DEFAULT ((0)) FOR [boat_isdeleted]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_map] DEFAULT ('') FOR [map]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_map_x] DEFAULT ((-1)) FOR [map_x]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_map_y] DEFAULT ((-1)) FOR [map_y]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_angle] DEFAULT ((0)) FOR [angle]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_degree] DEFAULT ((1)) FOR [degree]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_exp] DEFAULT ((0)) FOR [exp]
GO
ALTER TABLE [dbo].[boat] ADD CONSTRAINT [DF_boat_version] DEFAULT ((1)) FOR [version]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_motto] DEFAULT ('') FOR [motto]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_icon] DEFAULT ((1)) FOR [icon]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_version] DEFAULT ((1)) FOR [version]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_pk_ctrl_1] DEFAULT ((0)) FOR [pk_ctrl]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_mem_addr] DEFAULT ((0)) FOR [mem_addr]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_guild_id] DEFAULT ((0)) FOR [guild_id]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_guild_stat] DEFAULT ((0)) FOR [guild_stat]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_guild_permission] DEFAULT ((0)) FOR [guild_permission]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_job] DEFAULT ('??') FOR [job]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_degree] DEFAULT ((0)) FOR [degree]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_exp] DEFAULT ((0)) FOR [exp]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_hp] DEFAULT ((-1)) FOR [hp]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_sp] DEFAULT ((-1)) FOR [sp]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_ap] DEFAULT ((0)) FOR [ap]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_tp] DEFAULT ((0)) FOR [tp]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_gd] DEFAULT ((10000)) FOR [gd]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_str] DEFAULT ((0)) FOR [str]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_dex] DEFAULT ((0)) FOR [dex]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_agi] DEFAULT ((0)) FOR [agi]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_con] DEFAULT ((0)) FOR [con]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_sta] DEFAULT ((0)) FOR [sta]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_luk] DEFAULT ((0)) FOR [luk]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_sail_lv] DEFAULT ((1)) FOR [sail_lv]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_sail_exp] DEFAULT ((0)) FOR [sail_exp]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_sail_left_exp] DEFAULT ((0)) FOR [sail_left_exp]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_live_lv] DEFAULT ((1)) FOR [live_lv]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_live_exp] DEFAULT ((0)) FOR [live_exp]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_map] DEFAULT ('') FOR [map]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_map_x] DEFAULT ((-1)) FOR [map_x]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_map_y] DEFAULT ((-1)) FOR [map_y]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_radius] DEFAULT ((0)) FOR [radius]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_angle] DEFAULT ((0)) FOR [angle]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_look] DEFAULT ('') FOR [look]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_kb_capacity] DEFAULT ((24)) FOR [kb_capacity]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_kitbag] DEFAULT ('') FOR [kitbag]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_skillbag] DEFAULT ('') FOR [skillbag]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_shortcut_1] DEFAULT ('') FOR [shortcut]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_mission] DEFAULT ('') FOR [mission]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_misrecord] DEFAULT ('') FOR [misrecord]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_trigger] DEFAULT ('') FOR [mistrigger]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_miscount] DEFAULT ('') FOR [miscount]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_birth] DEFAULT ('???') FOR [birth]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_login_cha] DEFAULT ((0)) FOR [login_cha]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_live_tp] DEFAULT ((0)) FOR [live_tp]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_map_mask] DEFAULT ((0)) FOR [map_mask]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_delflag] DEFAULT ((0)) FOR [delflag]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_operdate] DEFAULT (GETDATE()) FOR [operdate]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_main_map] DEFAULT ('') FOR [main_map]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_skill_state] DEFAULT ('') FOR [skill_state]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_bank] DEFAULT ('') FOR [bank]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_estop] DEFAULT (GETDATE()) FOR [estop]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_estoptime] DEFAULT ((0)) FOR [estoptime]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_kb_locked] DEFAULT ((0)) FOR [kb_locked]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_kitbag_tmp] DEFAULT ((0)) FOR [kitbag_tmp]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_credit] DEFAULT ((0)) FOR [credit]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_store_item] DEFAULT ((0)) FOR [store_item]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_chatColour] DEFAULT ((-1)) FOR [chatColour]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_IMP] DEFAULT ((0)) FOR [IMP]
GO
ALTER TABLE [dbo].[character] ADD CONSTRAINT [DF_character_battle_power] DEFAULT ((0)) FOR [battle_power]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_guild_id] DEFAULT ((0)) FOR [guild_id]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_job] DEFAULT ((0)) FOR [job]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_degree] DEFAULT ((1)) FOR [degree]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_exp] DEFAULT ((0)) FOR [exp]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_hp] DEFAULT ((-1)) FOR [hp]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_sp] DEFAULT ((-1)) FOR [sp]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_ap] DEFAULT ((0)) FOR [ap]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_tp] DEFAULT ((0)) FOR [tp]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_gd] DEFAULT ((0)) FOR [gd]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_str] DEFAULT ((0)) FOR [str]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_dex] DEFAULT ((0)) FOR [dex]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_agi] DEFAULT ((0)) FOR [agi]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_con] DEFAULT ((0)) FOR [con]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_sta] DEFAULT ((0)) FOR [sta]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_luk] DEFAULT ((0)) FOR [luk]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_map] DEFAULT ('') FOR [map]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_map_x] DEFAULT ((-1)) FOR [map_x]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_map_y] DEFAULT ((-1)) FOR [map_y]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_radius] DEFAULT ((0)) FOR [radius]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_look] DEFAULT ('') FOR [look]
GO
ALTER TABLE [dbo].[character_log] ADD CONSTRAINT [DF_character_log_del_date] DEFAULT (GETDATE()) FOR [del_date]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_guild_name] DEFAULT ('') FOR [guild_name]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_motto] DEFAULT ('') FOR [motto]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_passwd] DEFAULT ('') FOR [passwd]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_leader_id] DEFAULT ((0)) FOR [leader_id]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_exp] DEFAULT ((0)) FOR [exp]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_gold] DEFAULT ((0)) FOR [gold]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_bank] DEFAULT ('') FOR [bank]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_level] DEFAULT ((0)) FOR [level]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_total] DEFAULT ((0)) FOR [member_total]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_pending_total] DEFAULT ((0)) FOR [try_total]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_disband_date] DEFAULT (GETDATE()) FOR [disband_date]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_challlevel] DEFAULT ((0)) FOR [challlevel]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_challid] DEFAULT ((0)) FOR [challid]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_challmoney] DEFAULT ((0)) FOR [challmoney]
GO
ALTER TABLE [dbo].[guild] ADD CONSTRAINT [DF_guild_challstart] DEFAULT ((0)) FOR [challstart]

GO
ALTER TABLE [dbo].[map_mask] ADD CONSTRAINT [DF_map_mask_content] DEFAULT ((0)) FOR [content1]
GO
ALTER TABLE [dbo].[map_mask] ADD CONSTRAINT [DF_map_mask_content4] DEFAULT ((0)) FOR [content2]
GO
ALTER TABLE [dbo].[map_mask] ADD CONSTRAINT [DF_map_mask_content3] DEFAULT ((0)) FOR [content3]
GO
ALTER TABLE [dbo].[map_mask] ADD CONSTRAINT [DF_map_mask_content2] DEFAULT ((0)) FOR [content4]
GO
ALTER TABLE [dbo].[map_mask] ADD CONSTRAINT [DF_map_mask_content1] DEFAULT ((0)) FOR [content5]
GO
ALTER TABLE [dbo].[master] ADD CONSTRAINT [DF_master_cha_id1] DEFAULT ((0)) FOR [cha_id1]
GO
ALTER TABLE [dbo].[master] ADD CONSTRAINT [DF_master_cha_id2] DEFAULT ((0)) FOR [cha_id2]
GO
ALTER TABLE [dbo].[master] ADD CONSTRAINT [DF_master_finish] DEFAULT ((0)) FOR [finish]
GO
ALTER TABLE [dbo].[master] ADD CONSTRAINT [DF_master_relation] DEFAULT ('teacher') FOR [relation]
GO
ALTER TABLE [dbo].[param] ADD CONSTRAINT [DF_param_param1] DEFAULT ((0)) FOR [param1]
GO
ALTER TABLE [dbo].[param] ADD CONSTRAINT [DF_param_param2] DEFAULT ((0)) FOR [param2]
GO
ALTER TABLE [dbo].[param] ADD CONSTRAINT [DF_param_param3] DEFAULT ((0)) FOR [param3]
GO
ALTER TABLE [dbo].[param] ADD CONSTRAINT [DF_param_param4] DEFAULT ((0)) FOR [param4]
GO
ALTER TABLE [dbo].[param] ADD CONSTRAINT [DF_param_param5] DEFAULT ((0)) FOR [param5]
GO
ALTER TABLE [dbo].[param] ADD CONSTRAINT [DF_param_param6] DEFAULT ((0)) FOR [param6]
GO
ALTER TABLE [dbo].[param] ADD CONSTRAINT [DF_param_param7] DEFAULT ((0)) FOR [param7]
GO
ALTER TABLE [dbo].[param] ADD CONSTRAINT [DF_param_param8] DEFAULT ((0)) FOR [param8]
GO
ALTER TABLE [dbo].[param] ADD CONSTRAINT [DF_param_param9] DEFAULT ((0)) FOR [param9]
GO
ALTER TABLE [dbo].[param] ADD CONSTRAINT [DF_param_param10] DEFAULT ((0)) FOR [param10]
GO
ALTER TABLE [dbo].[Resource] ADD CONSTRAINT [DF_Recource_type_id] DEFAULT ((1)) FOR [type_id]
GO
ALTER TABLE [dbo].[Resource] ADD CONSTRAINT [DF_Recource_content] DEFAULT ('') FOR [content]
GO
ALTER TABLE [dbo].[stat_log] ADD CONSTRAINT [DF_stat_log_track_date] DEFAULT (GETDATE()) FOR [track_date]
GO
ALTER TABLE [dbo].[stat_log] ADD CONSTRAINT [DF_stat_log_login_num] DEFAULT ((0)) FOR [login_num]
GO
ALTER TABLE [dbo].[stat_log] ADD CONSTRAINT [DF_stat_log_play_num] DEFAULT ((0)) FOR [play_num]
GO
ALTER TABLE [dbo].[WinTicket] ADD CONSTRAINT [DF_Ticket_num] DEFAULT ((0)) FOR [num]
GO
ALTER TABLE [dbo].[character] WITH NOCHECK ADD CONSTRAINT [FK_character_account] FOREIGN KEY ([act_id])
REFERENCES [dbo].[account] ([act_id])
ON UPDATE CASCADE
ON DELETE CASCADE
GO
ALTER TABLE [dbo].[character] CHECK CONSTRAINT [FK_character_account]
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'?????',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'LotterySetting',
                                @level2type = N'COLUMN',
                                @level2name = N'section'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'??',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'LotterySetting',
                                @level2type = N'COLUMN',
                                @level2name = N'issue'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'??:0 ??? 1 ?? 2  ??? ',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'LotterySetting',
                                @level2type = N'COLUMN',
                                @level2name = N'state'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'????',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'LotterySetting',
                                @level2type = N'COLUMN',
                                @level2name = N'createdate'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'?????',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'LotterySetting',
                                @level2type = N'COLUMN',
                                @level2name = N'updatetime'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'??',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'personinfo',
                                @level2type = N'COLUMN',
                                @level2name = N'sex'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'??',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'personinfo',
                                @level2type = N'COLUMN',
                                @level2name = N'age'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'??',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'personinfo',
                                @level2type = N'COLUMN',
                                @level2name = N'name'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'??',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'personinfo',
                                @level2type = N'COLUMN',
                                @level2name = N'animal_zodiac'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'??',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'personinfo',
                                @level2type = N'COLUMN',
                                @level2name = N'blood_type'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'??',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'personinfo',
                                @level2type = N'COLUMN',
                                @level2name = N'birthday'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'??',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'personinfo',
                                @level2type = N'COLUMN',
                                @level2name = N'state'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'??',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'personinfo',
                                @level2type = N'COLUMN',
                                @level2name = N'city'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'??',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'personinfo',
                                @level2type = N'COLUMN',
                                @level2name = N'constellation'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'??',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'personinfo',
                                @level2type = N'COLUMN',
                                @level2name = N'career'
GO
EXEC sys.sp_addextendedproperty @name = N'MS_Description',
                                @value = N'???',
                                @level0type = N'SCHEMA',
                                @level0name = N'dbo',
                                @level1type = N'TABLE',
                                @level1name = N'personinfo',
                                @level2type = N'COLUMN',
                                @level2name = N'prevent'
GO
-- =============================================
-- STORED PROCEDURES
-- Combined from [6]StoredProcedures.sql
-- =============================================

-- PART 2: GameDB Database Stored Procedures
-- =============================================
USE [GameDB]
GO

-- =============================================
-- Procedure: AddStatLog
-- Description: Adds server statistics log entry
-- Parameters:
--   @login - Login count
--   @play - Play count
--   @wgplay - Wargame play count
-- =============================================
IF OBJECT_ID('dbo.AddStatLog', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AddStatLog
GO

CREATE PROCEDURE [dbo].[AddStatLog]
    @login INT,
    @play INT,
    @wgplay INT
AS
BEGIN
    SET NOCOUNT ON;

    -- Insert statistics entry (stat_log table should already exist)
    INSERT INTO stat_log (track_date, login_num, play_num, wgplay_num)
    VALUES (GETDATE(), @login, @play, @wgplay);

    RETURN 0;
END
GO

-- =============================================
-- Procedure: SetDiscInfo
-- Description: Sets disconnect information for an account
-- Parameters:
--   @cli_ip - Client IP address
--   @reason - Disconnect reason
--   @actid - Account ID
-- =============================================
IF OBJECT_ID('dbo.SetDiscInfo', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SetDiscInfo
GO

CREATE PROCEDURE [dbo].[SetDiscInfo]
    @cli_ip VARCHAR(16),
    @reason VARCHAR(128),
    @actid INT
AS
BEGIN
    SET NOCOUNT ON;
    
    -- Update account disconnect info
    UPDATE account
    SET last_ip = @cli_ip,
        disc_reason = @reason,
        last_leave = GETDATE()
    WHERE act_id = @actid;
    
    -- If no rows affected, insert new record (for first-time users)
    IF @@ROWCOUNT = 0
    BEGIN
        -- The account table requires act_name which we don't have here
        -- Log the disconnect info to a separate table instead
        IF NOT EXISTS (SELECT 1 FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = 'disconnect_log')
        BEGIN
            CREATE TABLE [dbo].[disconnect_log] (
                [log_id] INT IDENTITY(1,1) NOT NULL,
                [act_id] INT NOT NULL,
                [client_ip] VARCHAR(16) NOT NULL,
                [reason] VARCHAR(128) NOT NULL,
                [log_time] DATETIME NOT NULL DEFAULT GETDATE(),
                CONSTRAINT [PK_disconnect_log] PRIMARY KEY CLUSTERED ([log_id] ASC)
            );
        END
        
        INSERT INTO disconnect_log (act_id, client_ip, reason, log_time)
        VALUES (@actid, @cli_ip, @reason, GETDATE());
    END
    
    RETURN 0;
END
GO

-- =============================================
-- PART 3: GameServer Character Stored Procedures
-- =============================================

-- =============================================
-- Procedure: VerifyCharacterName
-- Description: Checks if a character name already exists
-- Parameters:
--   @chaName - Character name to check
-- Returns: 1 if exists, 0 if not
-- =============================================
IF OBJECT_ID('dbo.VerifyCharacterName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.VerifyCharacterName
GO

CREATE PROCEDURE [dbo].[VerifyCharacterName]
    @chaName VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    IF EXISTS (SELECT 1 FROM character WHERE cha_name = @chaName)
        SELECT 1 AS [NameExists];
    ELSE
        SELECT 0 AS [NameExists];
    
    RETURN 0;
END
GO

-- =============================================
-- Procedure: GetCharacterName
-- Description: Gets character name by ID
-- Parameters:
--   @chaId - Character ID
-- Returns: Character name
-- =============================================
IF OBJECT_ID('dbo.GetCharacterName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetCharacterName
GO

CREATE PROCEDURE [dbo].[GetCharacterName]
    @chaId INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT cha_name FROM character WHERE cha_id = @chaId;
    
    RETURN 0;
END
GO

-- =============================================
-- Procedure: ReadCharacterData
-- Description: Reads all character data for login
-- Parameters:
--   @chaId - Character ID
-- Returns: Full character data
-- =============================================
IF OBJECT_ID('dbo.ReadCharacterData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ReadCharacterData
GO

CREATE PROCEDURE [dbo].[ReadCharacterData]
    @chaId INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT 
        act_id, guild_stat, guild_id, hp, sp, exp, radius, angle, cha_name, motto, icon, 
        version, pk_ctrl, degree, job, gd, ap, tp, str, dex, agi, con, sta, luk, 
        sail_lv, sail_exp, sail_left_exp, live_lv, live_exp, live_tp, main_map, map_x, map_y, 
        birth, look, skillbag, shortcut, mission, misrecord, mistrigger, miscount, login_cha, 
        kitbag, kitbag_tmp, map_mask, skill_state, bank, kb_locked, credit, store_item, 
        extend, guild_permission, chatColour, IMP
    FROM character
    WHERE cha_id = @chaId;
    
    RETURN 0;
END
GO

-- =============================================
-- Procedure: SaveCharacterData
-- Description: Saves all character data (with position)
-- Parameters: All character attributes
-- =============================================
IF OBJECT_ID('dbo.SaveCharacterData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveCharacterData
GO

CREATE PROCEDURE [dbo].[SaveCharacterData]
    @chaId INT,
    @hp INT,
    @sp INT,
    @exp BIGINT,
    @map VARCHAR(50),
    @mainMap VARCHAR(50),
    @mapX INT,
    @mapY INT,
    @radius INT,
    @angle SMALLINT,
    @pkCtrl TINYINT,
    @degree SMALLINT,
    @job VARCHAR(20),
    @gd INT,
    @ap INT,
    @tp INT,
    @str INT,
    @dex INT,
    @agi INT,
    @con INT,
    @sta INT,
    @luk INT,
    @look VARCHAR(500),
    @skillbag VARCHAR(1500),
    @shortcut VARCHAR(1500),
    @mission VARCHAR(MAX),
    @misrecord VARCHAR(MAX),
    @mistrigger VARCHAR(MAX),
    @miscount VARCHAR(MAX),
    @birth VARCHAR(50),
    @loginCha VARCHAR(50),
    @sailLv INT,
    @sailExp INT,
    @sailLeftExp INT,
    @liveLv INT,
    @liveExp INT,
    @liveTp INT,
    @kbLocked INT,
    @credit INT,
    @storeItem INT,
    @skillState VARCHAR(1024),
    @extend VARCHAR(MAX),
    @imp INT,
    @savePos BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    
    IF @savePos = 1
    BEGIN
        UPDATE character SET 
            hp = @hp, sp = @sp, exp = @exp, map = @map, main_map = @mainMap, 
            map_x = @mapX, map_y = @mapY, radius = @radius, angle = @angle, 
            pk_ctrl = @pkCtrl, degree = @degree, job = @job, gd = @gd, ap = @ap, tp = @tp, 
            str = @str, dex = @dex, agi = @agi, con = @con, sta = @sta, luk = @luk, 
            look = @look, skillbag = @skillbag, shortcut = @shortcut, 
            mission = @mission, misrecord = @misrecord, mistrigger = @mistrigger, miscount = @miscount, 
            birth = @birth, login_cha = @loginCha, 
            sail_lv = @sailLv, sail_exp = @sailExp, sail_left_exp = @sailLeftExp, 
            live_lv = @liveLv, live_exp = @liveExp, live_tp = @liveTp, 
            kb_locked = @kbLocked, credit = @credit, store_item = @storeItem, 
            skill_state = @skillState, extend = @extend, IMP = @imp
        WHERE cha_id = @chaId;
    END
    ELSE
    BEGIN
        UPDATE character SET 
            hp = @hp, sp = @sp, exp = @exp, radius = @radius, 
            pk_ctrl = @pkCtrl, degree = @degree, job = @job, gd = @gd, ap = @ap, tp = @tp, 
            str = @str, dex = @dex, agi = @agi, con = @con, sta = @sta, luk = @luk, 
            look = @look, skillbag = @skillbag, shortcut = @shortcut, 
            mission = @mission, misrecord = @misrecord, mistrigger = @mistrigger, miscount = @miscount, 
            birth = @birth, login_cha = @loginCha, 
            sail_lv = @sailLv, sail_exp = @sailExp, sail_left_exp = @sailLeftExp, 
            live_lv = @liveLv, live_exp = @liveExp, live_tp = @liveTp, 
            kb_locked = @kbLocked, credit = @credit, store_item = @storeItem, 
            skill_state = @skillState, extend = @extend, IMP = @imp
        WHERE cha_id = @chaId;
    END
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: SaveCharacterPosition
-- Description: Saves only character position
-- Parameters:
--   @chaId - Character ID
--   @map - Current map
--   @mainMap - Main map
--   @mapX - X coordinate
--   @mapY - Y coordinate
--   @angle - Character angle
-- =============================================
IF OBJECT_ID('dbo.SaveCharacterPosition', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveCharacterPosition
GO

CREATE PROCEDURE [dbo].[SaveCharacterPosition]
    @chaId INT,
    @map VARCHAR(50),
    @mainMap VARCHAR(50),
    @mapX INT,
    @mapY INT,
    @angle SMALLINT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET 
        map = @map, 
        main_map = @mainMap, 
        map_x = @mapX, 
        map_y = @mapY, 
        angle = @angle
    WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: SaveCharacterMoney
-- Description: Saves character money (gold)
-- Parameters:
--   @chaId - Character ID
--   @gd - Gold amount
-- =============================================
IF OBJECT_ID('dbo.SaveCharacterMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveCharacterMoney
GO

CREATE PROCEDURE [dbo].[SaveCharacterMoney]
    @chaId INT,
    @gd INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET gd = @gd WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: AddCharacterMoney
-- Description: Adds money to character
-- Parameters:
--   @chaId - Character ID
--   @money - Amount to add (can be negative) - BIGINT for 100B cap
-- =============================================
IF OBJECT_ID('dbo.AddCharacterMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AddCharacterMoney
GO

CREATE PROCEDURE [dbo].[AddCharacterMoney]
    @chaId INT,
    @money BIGINT
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @newGold BIGINT;
    DECLARE @currentGold BIGINT;
    
    SELECT @currentGold = gd FROM character WHERE cha_id = @chaId;
    SET @newGold = @currentGold + @money;
    
    -- Cap at 100 billion, floor at 0
    IF @newGold > 100000000000
        SET @newGold = 100000000000;
    IF @newGold < 0
        SET @newGold = 0;
    
    UPDATE character SET gd = @newGold WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: AddCharacterCredit
-- Description: Adds credit to character
-- Parameters:
--   @chaId - Character ID
--   @credit - Amount to add
-- =============================================
IF OBJECT_ID('dbo.AddCharacterCredit', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AddCharacterCredit
GO

CREATE PROCEDURE [dbo].[AddCharacterCredit]
    @chaId INT,
    @credit INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET credit = credit + @credit WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: SaveKitbagDBID
-- Description: Saves kitbag resource ID
-- Parameters:
--   @chaId - Character ID
--   @kitbagId - Kitbag resource ID
-- =============================================
IF OBJECT_ID('dbo.SaveKitbagDBID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveKitbagDBID
GO

CREATE PROCEDURE [dbo].[SaveKitbagDBID]
    @chaId INT,
    @kitbagId INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET kitbag = @kitbagId WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: SaveKitbagTmpDBID
-- Description: Saves temporary kitbag resource ID
-- Parameters:
--   @chaId - Character ID
--   @kitbagTmpId - Temp kitbag resource ID
-- =============================================
IF OBJECT_ID('dbo.SaveKitbagTmpDBID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveKitbagTmpDBID
GO

CREATE PROCEDURE [dbo].[SaveKitbagTmpDBID]
    @chaId INT,
    @kitbagTmpId INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET kitbag_tmp = @kitbagTmpId WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: SaveKitbagLockState
-- Description: Saves kitbag lock state
-- Parameters:
--   @chaId - Character ID
--   @locked - Lock state (0/1)
-- =============================================
IF OBJECT_ID('dbo.SaveKitbagLockState', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveKitbagLockState
GO

CREATE PROCEDURE [dbo].[SaveKitbagLockState]
    @chaId INT,
    @locked INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET kb_locked = @locked WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: SaveStoreItemID
-- Description: Saves store item ID
-- Parameters:
--   @chaId - Character ID
--   @storeItemId - Store item ID
-- =============================================
IF OBJECT_ID('dbo.SaveStoreItemID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveStoreItemID
GO

CREATE PROCEDURE [dbo].[SaveStoreItemID]
    @chaId INT,
    @storeItemId INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET store_item = @storeItemId WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: IsCharacterOnline
-- Description: Checks if character is online
-- Parameters:
--   @chaId - Character ID
-- Returns: mem_addr value (>0 if online)
-- =============================================
IF OBJECT_ID('dbo.IsCharacterOnline', 'P') IS NOT NULL
    DROP PROCEDURE dbo.IsCharacterOnline
GO

CREATE PROCEDURE [dbo].[IsCharacterOnline]
    @chaId INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT ISNULL(mem_addr, 0) AS mem_addr FROM character WHERE cha_id = @chaId;
    
    RETURN 0;
END
GO

-- =============================================
-- Procedure: SaveMapMaskDBID
-- Description: Saves map mask database ID
-- Parameters:
--   @chaId - Character ID
--   @mapMaskId - Map mask ID
-- =============================================
IF OBJECT_ID('dbo.SaveMapMaskDBID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveMapMaskDBID
GO

CREATE PROCEDURE [dbo].[SaveMapMaskDBID]
    @chaId INT,
    @mapMaskId INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET map_mask = @mapMaskId WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: SaveBankDBID
-- Description: Saves bank database IDs
-- Parameters:
--   @chaId - Character ID
--   @bankIds - Bank IDs string
-- =============================================
IF OBJECT_ID('dbo.SaveBankDBID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBankDBID
GO

CREATE PROCEDURE [dbo].[SaveBankDBID]
    @chaId INT,
    @bankIds VARCHAR(200)
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET bank = @bankIds WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: SaveMissionData
-- Description: Saves mission/quest data
-- Parameters:
--   @chaId - Character ID
--   @mission - Mission data
--   @misrecord - Mission record
--   @mistrigger - Mission triggers
--   @miscount - Mission counts
-- =============================================
IF OBJECT_ID('dbo.SaveMissionData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveMissionData
GO

CREATE PROCEDURE [dbo].[SaveMissionData]
    @chaId INT,
    @mission VARCHAR(MAX),
    @misrecord VARCHAR(MAX),
    @mistrigger VARCHAR(MAX),
    @miscount VARCHAR(MAX)
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET 
        mission = @mission, 
        misrecord = @misrecord, 
        mistrigger = @mistrigger, 
        miscount = @miscount
    WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: UpdateCharacterGuild
-- Description: Updates character guild info
-- Parameters:
--   @chaId - Character ID
--   @guildId - Guild ID
--   @guildStat - Guild status
--   @guildPermission - Guild permission flags
-- =============================================
IF OBJECT_ID('dbo.UpdateCharacterGuild', 'P') IS NOT NULL
    DROP PROCEDURE dbo.UpdateCharacterGuild
GO

CREATE PROCEDURE [dbo].[UpdateCharacterGuild]
    @chaId INT,
    @guildId INT,
    @guildStat INT,
    @guildPermission BIGINT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET 
        guild_id = @guildId, 
        guild_stat = @guildStat, 
        guild_permission = @guildPermission
    WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: CreateNewCharacter
-- Description: Creates a new character
-- Parameters: Character creation data
-- Returns: New character ID
-- =============================================
IF OBJECT_ID('dbo.CreateNewCharacter', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CreateNewCharacter
GO

CREATE PROCEDURE [dbo].[CreateNewCharacter]
    @actId INT,
    @chaName VARCHAR(50),
    @job VARCHAR(20),
    @map VARCHAR(50),
    @mapX INT,
    @mapY INT,
    @look VARCHAR(500),
    @icon SMALLINT,
    @str INT,
    @dex INT,
    @agi INT,
    @con INT,
    @sta INT,
    @luk INT,
    @hp INT,
    @sp INT,
    @birth VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    -- Check if name already exists
    IF EXISTS (SELECT 1 FROM character WHERE cha_name = @chaName)
    BEGIN
        SELECT -1 AS cha_id;
        RETURN -1;
    END
    
    INSERT INTO character (
        act_id, cha_name, job, map, main_map, map_x, map_y, look, icon, 
        str, dex, agi, con, sta, luk, hp, sp, birth, 
        degree, gd, ap, tp, exp, radius, angle, version, pk_ctrl,
        guild_id, guild_stat, guild_permission, 
        sail_lv, sail_exp, sail_left_exp, live_lv, live_exp, live_tp,
        kb_locked, credit, store_item, delflag, operdate
    ) VALUES (
        @actId, @chaName, @job, @map, @map, @mapX, @mapY, @look, @icon,
        @str, @dex, @agi, @con, @sta, @luk, @hp, @sp, @birth,
        1, 0, 0, 0, 0, 50, 0, 1, 0,
        0, 0, 0,
        1, 0, 0, 1, 0, 0,
        0, 0, 0, 0, GETDATE()
    );
    
    SELECT SCOPE_IDENTITY() AS cha_id;
    
    RETURN 0;
END
GO

-- =============================================
-- Procedure: DeleteCharacter
-- Description: Marks a character as deleted
-- Parameters:
--   @chaId - Character ID
--   @actId - Account ID (for verification)
-- =============================================
IF OBJECT_ID('dbo.DeleteCharacter', 'P') IS NOT NULL
    DROP PROCEDURE dbo.DeleteCharacter
GO

CREATE PROCEDURE [dbo].[DeleteCharacter]
    @chaId INT,
    @actId INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET 
        delflag = 1, 
        operdate = GETDATE()
    WHERE cha_id = @chaId AND act_id = @actId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- Procedure: GetExpRank
-- Description: Gets top characters by experience
-- Parameters:
--   @count - Number of results to return
-- Returns: Top characters (name, job, level)
-- =============================================
IF OBJECT_ID('dbo.GetExpRank', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetExpRank
GO

CREATE PROCEDURE [dbo].[GetExpRank]
    @count INT = 10
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT TOP (@count) cha_name, job, degree
    FROM character 
    WHERE delflag = 0 
    ORDER BY CASE WHEN (exp < 0) THEN (exp + 4294967296) ELSE exp END DESC;
    
    RETURN 0;
END
GO

-- =============================================
-- Procedure: SetCharacterOnlineStatus
-- Description: Sets character online/offline status
-- Parameters:
--   @chaId - Character ID
--   @memAddr - Memory address (0 = offline)
-- =============================================
IF OBJECT_ID('dbo.SetCharacterOnlineStatus', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SetCharacterOnlineStatus
GO

CREATE PROCEDURE [dbo].[SetCharacterOnlineStatus]
    @chaId INT,
    @memAddr INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET mem_addr = @memAddr WHERE cha_id = @chaId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- PART 4: Boat/Ship Stored Procedures
-- =============================================

-- =============================================
-- Procedure: GetBoatData
-- Description: Gets boat data by ID
-- Parameters:
--   @boatId - Boat ID
-- =============================================
IF OBJECT_ID('dbo.GetBoatData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetBoatData
GO

CREATE PROCEDURE [dbo].[GetBoatData]
    @boatId INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT * FROM boat WHERE boat_id = @boatId;
    
    RETURN 0;
END
GO

-- =============================================
-- Procedure: SaveBoatData
-- Description: Saves boat data
-- Parameters: Boat attributes
-- =============================================
IF OBJECT_ID('dbo.SaveBoatData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBoatData
GO

CREATE PROCEDURE [dbo].[SaveBoatData]
    @boatId INT,
    @boatBerth SMALLINT,
    @boatName VARCHAR(17),
    @header INT,
    @body INT,
    @engine INT,
    @cannon INT,
    @equipment INT,
    @bagSize SMALLINT,
    @bag VARCHAR(7000),
    @curEndure INT,
    @mxEndure INT,
    @curSupply INT,
    @mxSupply INT,
    @skillState VARCHAR(400),
    @isDead CHAR(1)
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE boat SET 
        boat_berth = @boatBerth,
        boat_name = @boatName,
        boat_header = @header,
        boat_body = @body,
        boat_engine = @engine,
        boat_cannon = @cannon,
        boat_equipment = @equipment,
        boat_bagsize = @bagSize,
        boat_bag = @bag,
        cur_endure = @curEndure,
        mx_endure = @mxEndure,
        cur_supply = @curSupply,
        mx_supply = @mxSupply,
        skill_state = @skillState,
        boat_isdead = @isDead
    WHERE boat_id = @boatId;
    
    IF @@ROWCOUNT > 0
        RETURN 0;
    ELSE
        RETURN -1;
END
GO

-- =============================================
-- PART 5: Resource/Item Stored Procedures  
-- =============================================

-- =============================================
-- Procedure: GetResourceData
-- Description: Gets resource data by ID
-- Parameters:
--   @resId - Resource ID
-- =============================================
IF OBJECT_ID('dbo.GetResourceData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetResourceData
GO

CREATE PROCEDURE [dbo].[GetResourceData]
    @resId INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT * FROM resource WHERE id = @resId;
    
    RETURN 0;
END
GO

-- =============================================
-- Procedure: SaveResourceData
-- Description: Saves resource data
-- Parameters:
--   @resId - Resource ID
--   @type - Resource type
--   @content - Resource content
-- =============================================
IF OBJECT_ID('dbo.SaveResourceData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveResourceData
GO

CREATE PROCEDURE [dbo].[SaveResourceData]
    @resId INT,
    @chaId INT,
    @type INT,
    @content VARCHAR(MAX)
AS
BEGIN
    SET NOCOUNT ON;

    IF EXISTS (SELECT 1 FROM Resource WHERE id = @resId)
    BEGIN
        UPDATE Resource SET type_id = @type, content = @content WHERE id = @resId;
    END
    ELSE
    BEGIN
        INSERT INTO Resource (cha_id, type_id, content) VALUES (@chaId, @type, @content);
    END

    RETURN 0;
END
GO

-- =============================================
-- Procedure: InsertResourceData
-- Description: Inserts new resource data
-- Parameters:
--   @type - Resource type
--   @content - Resource content
-- Returns: New resource ID
-- =============================================
IF OBJECT_ID('dbo.InsertResourceData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.InsertResourceData
GO

CREATE PROCEDURE [dbo].[InsertResourceData]
    @chaId INT,
    @type INT,
    @content VARCHAR(MAX)
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO Resource (cha_id, type_id, content) VALUES (@chaId, @type, @content);

    SELECT SCOPE_IDENTITY() AS NewResourceId;

    RETURN 0;
END
GO

-- =============================================
-- Additional Utility Procedures
-- =============================================

-- =============================================
-- Procedure: ValidateUserLogin
-- Description: Validates user credentials for login
-- Parameters:
--   @username - Account username
--   @password - Account password (hashed)
-- Returns: User info if valid, empty if invalid
-- =============================================

USE [GameDB]
GO
-- GRANT EXECUTE ON dbo.AddStatLog TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SetDiscInfo TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.VerifyCharacterName TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.GetCharacterName TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.ReadCharacterData TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SaveCharacterData TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SaveCharacterPosition TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SaveCharacterMoney TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.AddCharacterMoney TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.AddCharacterCredit TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SaveKitbagDBID TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SaveKitbagTmpDBID TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SaveKitbagLockState TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SaveStoreItemID TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.IsCharacterOnline TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SaveMapMaskDBID TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SaveBankDBID TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SaveMissionData TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.UpdateCharacterGuild TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.CreateNewCharacter TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.DeleteCharacter TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.GetExpRank TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SetCharacterOnlineStatus TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.GetBoatData TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SaveBoatData TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.GetResourceData TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.SaveResourceData TO [YourSQLLogin];
-- GRANT EXECUTE ON dbo.InsertResourceData TO [YourSQLLogin];

-- =============================================
-- PART 5: TOP-Master Compatible Stored Procedures
-- These procedures match TOP-master naming conventions
-- for use with stored_procedure() calls in C++ code
-- =============================================

USE [GameDB]
GO

-- =============================================
-- Procedure: ReadAllData
-- Description: Reads all character data for login (TOP-master compatible)
-- Parameters:
--   @charID - Character ID
-- =============================================
IF OBJECT_ID('dbo.ReadAllData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ReadAllData
GO

CREATE PROCEDURE [dbo].[ReadAllData]
    @charID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT 
        act_id, guild_stat, guild_id, hp, sp, exp, radius, angle, cha_name, motto, icon, 
        version, pk_ctrl, degree, job, gd, ap, tp, str, dex, agi, con, sta, luk, 
        sail_lv, sail_exp, sail_left_exp, live_lv, live_exp, live_tp, main_map, map_x, map_y, 
        birth, look, skillbag, shortcut, mission, misrecord, mistrigger, miscount, login_cha, 
        kitbag, kitbag_tmp, map_mask, skill_state, bank, kb_locked, credit, store_item, 
        extend, guild_permission, chatColour, IMP
    FROM character
    WHERE cha_id = @charID;
END
GO

-- =============================================
-- Procedure: SaveAllData
-- Description: Saves all character data with position (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.SaveAllData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveAllData
GO

CREATE PROCEDURE [dbo].[SaveAllData]
    @_HP INT,
    @_SP INT,
    @_EXP VARCHAR(32),
    @_MAP VARCHAR(50),
    @_MAIN_MAP VARCHAR(50),
    @_MAP_X INT,
    @_MAP_Y INT,
    @_RADIUS INT,
    @_ANGLE SMALLINT,
    @_PK_CTRL TINYINT,
    @_DEGREE SMALLINT,
    @_JOB VARCHAR(50),
    @_GD BIGINT,
    @_AP INT,
    @_TP INT,
    @_STR INT,
    @_DEX INT,
    @_AGI INT,
    @_CON INT,
    @_STA INT,
    @_LUK INT,
    @_LOOK VARCHAR(2000),
    @_SKILLBAG VARCHAR(1500),
    @_SHORTCUT VARCHAR(1500),
    @_MISSION VARCHAR(MAX),
    @_MISRECORD VARCHAR(MAX),
    @_MISTRIGGER VARCHAR(MAX),
    @_MISCOUNT VARCHAR(512),
    @_BIRTH VARCHAR(50),
    @_LOGIN_CHA VARCHAR(50),
    @_SAIL_LV INT,
    @_SAIL_EXP INT,
    @_SAIL_LEFT_EXP INT,
    @_LIVE_LV INT,
    @_LIVE_EXP INT,
    @_LIVE_TP INT,
    @_KB_LOCKED INT,
    @_CREDIT INT,
    @_STORE_ITEM INT,
    @_SKILL_STATE VARCHAR(1024),
    @_EXTEND VARCHAR(MAX),
    @_IMP INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET
        hp = @_HP, sp = @_SP, exp = CAST(@_EXP AS BIGINT), 
        map = @_MAP, main_map = @_MAIN_MAP, map_x = @_MAP_X, map_y = @_MAP_Y, 
        radius = @_RADIUS, angle = @_ANGLE, pk_ctrl = @_PK_CTRL, degree = @_DEGREE,
        job = @_JOB, gd = @_GD, ap = @_AP, tp = @_TP, 
        str = @_STR, dex = @_DEX, agi = @_AGI, con = @_CON, sta = @_STA, luk = @_LUK, 
        look = @_LOOK, skillbag = @_SKILLBAG, shortcut = @_SHORTCUT, 
        mission = @_MISSION, misrecord = @_MISRECORD, mistrigger = @_MISTRIGGER, miscount = @_MISCOUNT, 
        birth = @_BIRTH, login_cha = @_LOGIN_CHA,
        sail_lv = @_SAIL_LV, sail_exp = @_SAIL_EXP, sail_left_exp = @_SAIL_LEFT_EXP, 
        live_lv = @_LIVE_LV, live_exp = @_LIVE_EXP, live_tp = @_LIVE_TP, 
        kb_locked = @_KB_LOCKED, credit = @_CREDIT, store_item = @_STORE_ITEM, 
        skill_state = @_SKILL_STATE, extend = @_EXTEND, IMP = @_IMP
    WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: SaveAllDataWithoutPos
-- Description: Saves all character data without position (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.SaveAllDataWithoutPos', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveAllDataWithoutPos
GO

CREATE PROCEDURE [dbo].[SaveAllDataWithoutPos]
    @_HP INT,
    @_SP INT,
    @_EXP VARCHAR(32),
    @_RADIUS INT,
    @_PK_CTRL TINYINT,
    @_DEGREE SMALLINT,
    @_JOB VARCHAR(50),
    @_GD BIGINT,
    @_AP INT,
    @_TP INT,
    @_STR INT,
    @_DEX INT,
    @_AGI INT,
    @_CON INT,
    @_STA INT,
    @_LUK INT,
    @_LOOK VARCHAR(2000),
    @_SKILLBAG VARCHAR(1500),
    @_SHORTCUT VARCHAR(1500),
    @_MISSION VARCHAR(MAX),
    @_MISRECORD VARCHAR(MAX),
    @_MISTRIGGER VARCHAR(MAX),
    @_MISCOUNT VARCHAR(512),
    @_BIRTH VARCHAR(50),
    @_LOGIN_CHA VARCHAR(50),
    @_SAIL_LV INT,
    @_SAIL_EXP INT,
    @_SAIL_LEFT_EXP INT,
    @_LIVE_LV INT,
    @_LIVE_EXP INT,
    @_LIVE_TP INT,
    @_KB_LOCKED INT,
    @_CREDIT INT,
    @_STORE_ITEM INT,
    @_SKILL_STATE VARCHAR(1024),
    @_EXTEND VARCHAR(MAX),
    @_IMP INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET
        hp = @_HP, sp = @_SP, exp = CAST(@_EXP AS BIGINT), 
        radius = @_RADIUS, pk_ctrl = @_PK_CTRL, degree = @_DEGREE,
        job = @_JOB, gd = @_GD, ap = @_AP, tp = @_TP, 
        str = @_STR, dex = @_DEX, agi = @_AGI, con = @_CON, sta = @_STA, luk = @_LUK, 
        look = @_LOOK, skillbag = @_SKILLBAG, shortcut = @_SHORTCUT, 
        mission = @_MISSION, misrecord = @_MISRECORD, mistrigger = @_MISTRIGGER, miscount = @_MISCOUNT, 
        birth = @_BIRTH, login_cha = @_LOGIN_CHA,
        sail_lv = @_SAIL_LV, sail_exp = @_SAIL_EXP, sail_left_exp = @_SAIL_LEFT_EXP, 
        live_lv = @_LIVE_LV, live_exp = @_LIVE_EXP, live_tp = @_LIVE_TP, 
        kb_locked = @_KB_LOCKED, credit = @_CREDIT, store_item = @_STORE_ITEM, 
        skill_state = @_SKILL_STATE, extend = @_EXTEND, IMP = @_IMP
    WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: SavePos
-- Description: Saves character position only (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.SavePos', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SavePos
GO

CREATE PROCEDURE [dbo].[SavePos]
    @_MAP VARCHAR(50),
    @_MAIN_MAP VARCHAR(50),
    @_MAP_X INT,
    @_MAP_Y INT,
    @_ANGLE INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET
        map = @_MAP, main_map = @_MAIN_MAP, map_x = @_MAP_X, map_y = @_MAP_Y, angle = @_ANGLE
    WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: SaveMoney
-- Description: Saves character gold amount (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.SaveMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveMoney
GO

CREATE PROCEDURE [dbo].[SaveMoney]
    @_GD BIGINT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET gd = @_GD WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: AddMoney
-- Description: Adds gold to character (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.AddMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AddMoney
GO

CREATE PROCEDURE [dbo].[AddMoney]
    @_GOLD BIGINT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET gd = gd + @_GOLD WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: SaveKBagDBIDEx
-- Description: Saves kitbag resource ID (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.SaveKBagDBIDEx', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveKBagDBIDEx
GO

CREATE PROCEDURE [dbo].[SaveKBagDBIDEx]
    @_ID INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET kitbag = @_ID WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: SaveKBagTmpDBID
-- Description: Saves temporary kitbag resource ID (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.SaveKBagTmpDBID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveKBagTmpDBID
GO

CREATE PROCEDURE [dbo].[SaveKBagTmpDBID]
    @_ID INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET kitbag_tmp = @_ID WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: SaveKBState
-- Description: Saves kitbag lock state (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.SaveKBState', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveKBState
GO

CREATE PROCEDURE [dbo].[SaveKBState]
    @_Lock INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET kb_locked = @_Lock WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: SaveStoreItemID (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.SaveStoreItemID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveStoreItemID
GO

CREATE PROCEDURE [dbo].[SaveStoreItemID]
    @_ID INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET store_item = @_ID WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: AddCreditByDBID
-- Description: Sets character credit (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.AddCreditByDBID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AddCreditByDBID
GO

CREATE PROCEDURE [dbo].[AddCreditByDBID]
    @_CREDIT INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET credit = @_CREDIT WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: IsChaOnline
-- Description: Checks if character is online (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.IsChaOnline', 'P') IS NOT NULL
    DROP PROCEDURE dbo.IsChaOnline
GO

CREATE PROCEDURE [dbo].[IsChaOnline]
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT mem_addr FROM character WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: SaveBankDBID
-- Description: Saves bank resource ID (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.SaveBankDBID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBankDBID
GO

CREATE PROCEDURE [dbo].[SaveBankDBID]
    @_BANK VARCHAR(50),
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET bank = @_BANK WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: SaveMissionData
-- Description: Saves mission data (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.SaveMissionData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveMissionData
GO

CREATE PROCEDURE [dbo].[SaveMissionData]
    @_MIS_INFO VARCHAR(MAX),
    @_MIS_RECORD VARCHAR(MAX),
    @_MIS_TRIGGER VARCHAR(MAX),
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET
        mission = @_MIS_INFO, misrecord = @_MIS_RECORD, mistrigger = @_MIS_TRIGGER
    WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: SetAddr
-- Description: Sets character memory address (online status) (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.SetAddr', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SetAddr
GO

CREATE PROCEDURE [dbo].[SetAddr]
    @_MEM_ADDR BIGINT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET mem_addr = @_MEM_ADDR WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Procedure: ZeroAddr
-- Description: Resets all character memory addresses (TOP-master compatible)
-- =============================================
IF OBJECT_ID('dbo.ZeroAddr', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ZeroAddr
GO

CREATE PROCEDURE [dbo].[ZeroAddr]
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET mem_addr = 0 WHERE mem_addr != 0;
END
GO

-- =============================================
-- Boat Procedures (TOP-master compatible)
-- =============================================

-- GetBoat
IF OBJECT_ID('dbo.GetBoat', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetBoat
GO

CREATE PROCEDURE [dbo].[GetBoat]
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT boat_name, boat_boatid, boat_berth, boat_header, boat_body,
           boat_engine, boat_cannon, boat_equipment, boat_diecount, boat_isdead,
           boat_ownerid, boat_isdeleted, cur_endure, mx_endure, cur_supply, mx_supply,
           skill_state, map, map_x, map_y, angle, degree, [exp]
    FROM boat
    WHERE boat_id = @_BOAT_ID;
END
GO

-- CreateBoat
IF OBJECT_ID('dbo.CreateBoat', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CreateBoat
GO

CREATE PROCEDURE [dbo].[CreateBoat]
    @_BOAT_NAME VARCHAR(17),
    @_BOAT_BERTH SMALLINT,
    @_BOAT_BOATID INT,
    @_BOAT_HEADER INT,
    @_BOAT_BODY INT,
    @_BOAT_ENGINE INT,
    @_BOAT_CANNON INT,
    @_BOAT_EQUIP INT,
    @_BOAT_BAG INT,
    @_BOAT_DIECOUNT INT,
    @_BOAT_ISDEAD INT,
    @_BOAT_OWNERID INT,
    @_BOAT_CREATETIME VARCHAR(50),
    @_BOAT_ISDEL INT
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO boat (boat_name, boat_berth, boat_boatid, boat_header, boat_body,
                      boat_engine, boat_cannon, boat_equipment, boat_bag, boat_diecount,
                      boat_isdead, boat_ownerid, boat_createtime, boat_isdeleted)
    VALUES (@_BOAT_NAME, @_BOAT_BERTH, @_BOAT_BOATID, @_BOAT_HEADER, @_BOAT_BODY,
            @_BOAT_ENGINE, @_BOAT_CANNON, @_BOAT_EQUIP, @_BOAT_BAG, @_BOAT_DIECOUNT,
            @_BOAT_ISDEAD, @_BOAT_OWNERID, @_BOAT_CREATETIME, @_BOAT_ISDEL);
    
    SELECT SCOPE_IDENTITY() AS boat_id;
END
GO

-- SaveBoatExWithPos
IF OBJECT_ID('dbo.SaveBoatExWithPos', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBoatExWithPos
GO

CREATE PROCEDURE [dbo].[SaveBoatExWithPos]
    @_BOAT_BERTH SMALLINT,
    @_BOAT_OWNERID INT,
    @_CUR_ENDURE INT,
    @_MX_ENDURE INT,
    @_CUR_SUPPLY INT,
    @_MX_SUPPLY INT,
    @_SKILL_STATE VARCHAR(400),
    @_MAP VARCHAR(50),
    @_MAP_X INT,
    @_MAP_Y INT,
    @_ANGLE INT,
    @_DEGREE SMALLINT,
    @_EXP INT,
    @_BOAT_BAG VARCHAR(7000),
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE boat SET
        boat_berth = @_BOAT_BERTH, boat_ownerid = @_BOAT_OWNERID,
        cur_endure = @_CUR_ENDURE, mx_endure = @_MX_ENDURE,
        cur_supply = @_CUR_SUPPLY, mx_supply = @_MX_SUPPLY,
        skill_state = @_SKILL_STATE, map = @_MAP, map_x = @_MAP_X, map_y = @_MAP_Y,
        angle = @_ANGLE, degree = @_DEGREE, [exp] = @_EXP, boat_bag = @_BOAT_BAG
    WHERE boat_id = @_BOAT_ID;
END
GO

-- SaveBoatEx
IF OBJECT_ID('dbo.SaveBoatEx', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBoatEx
GO

CREATE PROCEDURE [dbo].[SaveBoatEx]
    @_BOAT_BERTH SMALLINT,
    @_BOAT_OWNERID INT,
    @_CUR_ENDURE INT,
    @_MX_ENDURE INT,
    @_CUR_SUPPLY INT,
    @_MX_SUPPLY INT,
    @_SKILL_STATE VARCHAR(400),
    @_DEGREE SMALLINT,
    @_EXP INT,
    @_BOAT_BAG VARCHAR(7000),
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE boat SET
        boat_berth = @_BOAT_BERTH, boat_ownerid = @_BOAT_OWNERID,
        cur_endure = @_CUR_ENDURE, mx_endure = @_MX_ENDURE,
        cur_supply = @_CUR_SUPPLY, mx_supply = @_MX_SUPPLY,
        skill_state = @_SKILL_STATE, degree = @_DEGREE, [exp] = @_EXP, boat_bag = @_BOAT_BAG
    WHERE boat_id = @_BOAT_ID;
END
GO

-- SaveBoatDelTag
IF OBJECT_ID('dbo.SaveBoatDelTag', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBoatDelTag
GO

CREATE PROCEDURE [dbo].[SaveBoatDelTag]
    @_BOAT_ISDEL INT,
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE boat SET boat_isdeleted = @_BOAT_ISDEL WHERE boat_id = @_BOAT_ID;
END
GO

-- ReadCabin
IF OBJECT_ID('dbo.ReadCabin', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ReadCabin
GO

CREATE PROCEDURE [dbo].[ReadCabin]
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT boat_bag FROM boat WHERE boat_id = @_BOAT_ID;
END
GO

-- SaveCabin
IF OBJECT_ID('dbo.SaveCabin', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveCabin
GO

CREATE PROCEDURE [dbo].[SaveCabin]
    @_KITBAG VARCHAR(7000),
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE boat SET boat_bag = @_KITBAG WHERE boat_id = @_BOAT_ID;
END
GO

-- =============================================
-- Resource Procedures (TOP-master compatible)
-- =============================================

-- ReadBankDataEx
IF OBJECT_ID('dbo.ReadBankDataEx', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ReadBankDataEx
GO

CREATE PROCEDURE [dbo].[ReadBankDataEx]
    @_BANK_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT cha_id, type_id, content FROM Resource WHERE id = @_BANK_ID;
END
GO

-- SaveBankDataEx
IF OBJECT_ID('dbo.SaveBankDataEx', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBankDataEx
GO

CREATE PROCEDURE [dbo].[SaveBankDataEx]
    @_CONTENT VARCHAR(8000),
    @_BANK_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE Resource SET content = @_CONTENT WHERE id = @_BANK_ID;
END
GO

-- ReadKitbagTmpData
IF OBJECT_ID('dbo.ReadKitbagTmpData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ReadKitbagTmpData
GO

CREATE PROCEDURE [dbo].[ReadKitbagTmpData]
    @_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT content FROM Resource WHERE id = @_ID;
END
GO

-- ReadKitbagTmpDataEx
IF OBJECT_ID('dbo.ReadKitbagTmpDataEx', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ReadKitbagTmpDataEx
GO

CREATE PROCEDURE [dbo].[ReadKitbagTmpDataEx]
    @_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT cha_id, type_id, content FROM Resource WHERE id = @_ID;
END
GO

-- SaveKitbagTmpDataEx
IF OBJECT_ID('dbo.SaveKitbagTmpDataEx', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveKitbagTmpDataEx
GO

CREATE PROCEDURE [dbo].[SaveKitbagTmpDataEx]
    @_CONTENT VARCHAR(8000),
    @_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE Resource SET content = @_CONTENT WHERE id = @_ID;
END
GO

-- ResourceCreate
IF OBJECT_ID('dbo.ResourceCreate', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ResourceCreate
GO

CREATE PROCEDURE [dbo].[ResourceCreate]
    @_ID INT,
    @_TYPE_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO Resource (cha_id, type_id) VALUES (@_ID, @_TYPE_ID);
    
    SELECT SCOPE_IDENTITY() AS resource_id;
END
GO

-- =============================================
-- Guild Procedures (TOP-master compatible)
-- =============================================

-- GetGuildName
IF OBJECT_ID('dbo.GetGuildName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetGuildName
GO

CREATE PROCEDURE [dbo].[GetGuildName]
    @_GUILD_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT guild_name FROM guild WHERE guild_id = @_GUILD_ID;
END
GO

-- GetGuildMemberNum
IF OBJECT_ID('dbo.GetGuildMemberNum', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetGuildMemberNum
GO

CREATE PROCEDURE [dbo].[GetGuildMemberNum]
    @_GUILD_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT member_total FROM guild WHERE guild_id = @_GUILD_ID;
END
GO

-- CancelTryFor
IF OBJECT_ID('dbo.CancelTryFor', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CancelTryFor
GO

CREATE PROCEDURE [dbo].[CancelTryFor]
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET guild_id = 0, guild_stat = 0, guild_permission = 0
    WHERE cha_id = @_CHA_ID;
END
GO

-- Leave1
IF OBJECT_ID('dbo.Leave1', 'P') IS NOT NULL
    DROP PROCEDURE dbo.Leave1
GO

CREATE PROCEDURE [dbo].[Leave1]
    @_CHA_ID INT,
    @_GUILD_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET guild_id = 0, guild_stat = 0, guild_permission = 0
    WHERE cha_id = @_CHA_ID AND guild_id = @_GUILD_ID AND guild_stat = 0 
          AND cha_id NOT IN (SELECT leader_id FROM guild WHERE guild_id = @_GUILD_ID);
END
GO

-- Leave2
IF OBJECT_ID('dbo.Leave2', 'P') IS NOT NULL
    DROP PROCEDURE dbo.Leave2
GO

CREATE PROCEDURE [dbo].[Leave2]
    @_GUILD_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET member_total = member_total - 1 WHERE guild_id = @_GUILD_ID;
END
GO

-- Kick2
IF OBJECT_ID('dbo.Kick2', 'P') IS NOT NULL
    DROP PROCEDURE dbo.Kick2
GO

CREATE PROCEDURE [dbo].[Kick2]
    @_CHA_ID INT,
    @_GUILD_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET guild_id = 0, guild_stat = 0, guild_permission = 0
    WHERE cha_id = @_CHA_ID AND guild_id = @_GUILD_ID AND guild_stat = 0 
          AND cha_id NOT IN (SELECT leader_id FROM guild WHERE guild_id = @_GUILD_ID);
END
GO

-- Disband1
IF OBJECT_ID('dbo.Disband1', 'P') IS NOT NULL
    DROP PROCEDURE dbo.Disband1
GO

CREATE PROCEDURE [dbo].[Disband1]
    @_GUILD_ID INT
AS
BEGIN
    SET NOCOUNT ON;

    UPDATE guild SET motto = '', passwd = '', leader_id = 0, level = 0,
                     gold = 0, [exp] = 0, member_total = 0, try_total = 0
    WHERE guild_id = @_GUILD_ID;
END
GO

-- Disband2
IF OBJECT_ID('dbo.Disband2', 'P') IS NOT NULL
    DROP PROCEDURE dbo.Disband2
GO

CREATE PROCEDURE [dbo].[Disband2]
    @_GUILD_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET guild_id = 0, guild_stat = 0, guild_permission = 0
    WHERE guild_id = @_GUILD_ID;
END
GO

-- =============================================
-- Friend/Master Procedures (TOP-master compatible)
-- =============================================

-- AddFriend
IF OBJECT_ID('dbo.AddFriend', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AddFriend
GO

CREATE PROCEDURE [dbo].[AddFriend]
    @_CHA_ID1 INT,
    @_CHA_ID2 INT,
    @_RELATION VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO friends (cha_id1, cha_id2, relation) VALUES (@_CHA_ID1, @_CHA_ID2, @_RELATION);
END
GO

-- DelFriend
IF OBJECT_ID('dbo.DelFriend', 'P') IS NOT NULL
    DROP PROCEDURE dbo.DelFriend
GO

CREATE PROCEDURE [dbo].[DelFriend]
    @_CHA_ID1 INT,
    @_CHA_ID2 INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DELETE FROM friends 
    WHERE (cha_id1 = @_CHA_ID1 AND cha_id2 = @_CHA_ID2) 
       OR (cha_id1 = @_CHA_ID2 AND cha_id2 = @_CHA_ID1);
END
GO

-- AddMaster
IF OBJECT_ID('dbo.AddMaster', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AddMaster
GO

CREATE PROCEDURE [dbo].[AddMaster]
    @_CHA_ID INT,
    @_CHA_ID2 INT
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO [master] (cha_id1, cha_id2, finish) VALUES (@_CHA_ID, @_CHA_ID2, 0);
END
GO

-- DelMaster
IF OBJECT_ID('dbo.DelMaster', 'P') IS NOT NULL
    DROP PROCEDURE dbo.DelMaster
GO

CREATE PROCEDURE [dbo].[DelMaster]
    @_CHA_ID INT,
    @_CHA_ID2 INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DELETE FROM [master] WHERE (cha_id1 = @_CHA_ID AND cha_id2 = @_CHA_ID2);
END
GO

-- HasMaster
IF OBJECT_ID('dbo.HasMaster', 'P') IS NOT NULL
    DROP PROCEDURE dbo.HasMaster
GO

CREATE PROCEDURE [dbo].[HasMaster]
    @_CHA_ID INT,
    @_CHA_ID2 INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT COUNT(*) AS num FROM [master] WHERE (cha_id1 = @_CHA_ID AND cha_id2 = @_CHA_ID2);
END
GO

-- FinishMaster
IF OBJECT_ID('dbo.FinishMaster', 'P') IS NOT NULL
    DROP PROCEDURE dbo.FinishMaster
GO

CREATE PROCEDURE [dbo].[FinishMaster]
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE [master] SET finish = 1 WHERE cha_id1 = @_CHA_ID;
END
GO

-- =============================================
-- Character Info Procedures (TOP-master compatible)
-- =============================================

-- UpdateInfo
IF OBJECT_ID('dbo.UpdateInfo', 'P') IS NOT NULL
    DROP PROCEDURE dbo.UpdateInfo
GO

CREATE PROCEDURE [dbo].[UpdateInfo]
    @_ICON SMALLINT,
    @_MOTTO VARCHAR(50),
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET icon = @_ICON, motto = @_MOTTO WHERE cha_id = @_CHA_ID;
END
GO

-- FetchRowByChaName
IF OBJECT_ID('dbo.FetchRowByChaName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.FetchRowByChaName
GO

CREATE PROCEDURE [dbo].[FetchRowByChaName]
    @_CHA_NAME VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT cha_id, motto, icon FROM character WHERE cha_name = @_CHA_NAME;
END
GO

-- FetchAccidByChaName
IF OBJECT_ID('dbo.FetchAccidByChaName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.FetchAccidByChaName
GO

CREATE PROCEDURE [dbo].[FetchAccidByChaName]
    @_CHA_NAME VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT cha_id FROM character WHERE cha_name = @_CHA_NAME;
END
GO

-- BackupRow
IF OBJECT_ID('dbo.BackupRow', 'P') IS NOT NULL
    DROP PROCEDURE dbo.BackupRow
GO

CREATE PROCEDURE [dbo].[BackupRow]
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT guild_id, guild_stat FROM character WHERE cha_id = @_CHA_ID;
END
GO

-- BackupRow1
IF OBJECT_ID('dbo.BackupRow1', 'P') IS NOT NULL
    DROP PROCEDURE dbo.BackupRow1
GO

CREATE PROCEDURE [dbo].[BackupRow1]
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET delflag = 1, deldate = GETDATE() WHERE cha_id = @_CHA_ID;
END
GO

-- InsertRow3 (Create Character)
IF OBJECT_ID('dbo.InsertRow3', 'P') IS NOT NULL
    DROP PROCEDURE dbo.InsertRow3
GO

CREATE PROCEDURE [dbo].[InsertRow3]
    @_CHA_NAME VARCHAR(50),
    @_ACT_ID INT,
    @_BIRTH VARCHAR(50),
    @_MAP VARCHAR(50),
    @_LOOK VARCHAR(2000)
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO character (cha_name, act_id, birth, map, look) 
    VALUES (@_CHA_NAME, @_ACT_ID, @_BIRTH, @_MAP, @_LOOK);
    
    SELECT SCOPE_IDENTITY() AS cha_id;
END
GO

-- =============================================
-- Additional Stored Procedures (PKODev specific)
-- =============================================

-- SaveMMaskDBID (alias for SaveMapMaskDBID with param order matching code)
IF OBJECT_ID('dbo.SaveMMaskDBID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveMMaskDBID
GO

CREATE PROCEDURE [dbo].[SaveMMaskDBID]
    @_MAP_MASK INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET map_mask = @_MAP_MASK WHERE cha_id = @_CHA_ID;
END
GO

-- SaveTableVer - Updates character table version
IF OBJECT_ID('dbo.SaveTableVer', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveTableVer
GO

CREATE PROCEDURE [dbo].[SaveTableVer]
    @_VERSION INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET version = @_VERSION WHERE cha_id = @_CHA_ID;
END
GO

-- SetGuildPermission - Updates guild permission for character
IF OBJECT_ID('dbo.SetGuildPermission', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SetGuildPermission
GO

CREATE PROCEDURE [dbo].[SetGuildPermission]
    @_PERMISSION INT,
    @_CHA_ID INT,
    @_GUILD_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET guild_permission = @_PERMISSION 
    WHERE cha_id = @_CHA_ID AND guild_id = @_GUILD_ID;
END
GO

-- SetChaAddr - Sets character memory address (online status)
IF OBJECT_ID('dbo.SetChaAddr', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SetChaAddr
GO

CREATE PROCEDURE [dbo].[SetChaAddr]
    @_MEM_ADDR INT,
    @_CHA_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET mem_addr = @_MEM_ADDR WHERE cha_id = @_CHA_ID;
END
GO

-- =============================================
-- Boat/Ship Stored Procedures (Additional)
-- =============================================

-- SaveBoatTempData - Updates boat temp data (owner and delete flag)
IF OBJECT_ID('dbo.SaveBoatTempData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBoatTempData
GO

CREATE PROCEDURE [dbo].[SaveBoatTempData]
    @_OWNER_ID INT,
    @_IS_DELETED INT,
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE boat SET boat_ownerid = @_OWNER_ID, boat_isdeleted = @_IS_DELETED 
    WHERE boat_id = @_BOAT_ID;
END
GO

-- SaveBoatDelTag - Updates boat delete flag only
IF OBJECT_ID('dbo.SaveBoatDelTag', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBoatDelTag
GO

CREATE PROCEDURE [dbo].[SaveBoatDelTag]
    @_IS_DELETED INT,
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE boat SET boat_isdeleted = @_IS_DELETED WHERE boat_id = @_BOAT_ID;
END
GO

-- SaveBoatTempDataEx - Updates boat temp data with die count and dead flag
IF OBJECT_ID('dbo.SaveBoatTempDataEx', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBoatTempDataEx
GO

CREATE PROCEDURE [dbo].[SaveBoatTempDataEx]
    @_DIE_COUNT INT,
    @_IS_DEAD INT,
    @_OWNER_ID INT,
    @_IS_DELETED INT,
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE boat SET boat_diecount = @_DIE_COUNT, boat_isdead = @_IS_DEAD, 
                    boat_ownerid = @_OWNER_ID, boat_isdeleted = @_IS_DELETED 
    WHERE boat_id = @_BOAT_ID;
END
GO

-- SaveCabin - Updates boat cabin (bag) data
IF OBJECT_ID('dbo.SaveCabin', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveCabin
GO

CREATE PROCEDURE [dbo].[SaveCabin]
    @_BOAT_BAG VARCHAR(MAX),
    @_BOAT_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE boat SET boat_bag = @_BOAT_BAG WHERE boat_id = @_BOAT_ID;
END
GO

-- =============================================
-- Resource Table Stored Procedures
-- =============================================

-- SaveResourceContent - Updates resource content by ID
IF OBJECT_ID('dbo.SaveResourceContent', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveResourceContent
GO

CREATE PROCEDURE [dbo].[SaveResourceContent]
    @_CONTENT VARCHAR(MAX),
    @_ID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE resource SET content = @_CONTENT WHERE id = @_ID;
END
GO

-- =============================================
-- PART 9: Lottery System Stored Procedures
-- =============================================

-- =============================================
-- Procedure: LotteryAddIssue
-- Description: Adds a new lottery issue
-- =============================================
IF OBJECT_ID('dbo.LotteryAddIssue', 'P') IS NOT NULL
    DROP PROCEDURE dbo.LotteryAddIssue
GO

CREATE PROCEDURE [dbo].[LotteryAddIssue]
    @section INT,
    @issue INT,
    @state INT
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO LotterySetting (section, issue, state, createdate, updatetime) 
    VALUES (@section, @issue, @state, GETDATE(), GETDATE());
END
GO

-- =============================================
-- Procedure: LotteryDisuseIssue
-- Description: Updates lottery issue state
-- =============================================
IF OBJECT_ID('dbo.LotteryDisuseIssue', 'P') IS NOT NULL
    DROP PROCEDURE dbo.LotteryDisuseIssue
GO

CREATE PROCEDURE [dbo].[LotteryDisuseIssue]
    @state INT,
    @issue INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE LotterySetting SET state = @state, updatetime = GETDATE() WHERE issue = @issue;
END
GO

-- =============================================
-- Procedure: LotterySetWinItemNo
-- Description: Sets winning item number for lottery issue
-- =============================================
IF OBJECT_ID('dbo.LotterySetWinItemNo', 'P') IS NOT NULL
    DROP PROCEDURE dbo.LotterySetWinItemNo
GO

CREATE PROCEDURE [dbo].[LotterySetWinItemNo]
    @itemno VARCHAR(20),
    @issue INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE LotterySetting SET itemno = @itemno, updatetime = GETDATE() WHERE issue = @issue;
END
GO

-- =============================================
-- Procedure: TicketAddTicket
-- Description: Adds a lottery ticket
-- =============================================
IF OBJECT_ID('dbo.TicketAddTicket', 'P') IS NOT NULL
    DROP PROCEDURE dbo.TicketAddTicket
GO

CREATE PROCEDURE [dbo].[TicketAddTicket]
    @cha_id INT,
    @issue VARCHAR(20),
    @itemno VARCHAR(10),
    @real INT
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO Ticket (cha_id, issue, itemno, real, buydate) 
    VALUES (@cha_id, @issue, @itemno, @real, GETDATE());
END
GO

-- =============================================
-- Procedure: WinTicketAddTicket
-- Description: Adds a winning ticket record
-- =============================================
IF OBJECT_ID('dbo.WinTicketAddTicket', 'P') IS NOT NULL
    DROP PROCEDURE dbo.WinTicketAddTicket
GO

CREATE PROCEDURE [dbo].[WinTicketAddTicket]
    @issue INT,
    @itemno VARCHAR(20),
    @grade INT
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO WinTicket (issue, itemno, grade, createdate) 
    VALUES (@issue, @itemno, @grade, GETDATE());
END
GO

-- =============================================
-- Procedure: WinTicketExchange
-- Description: Exchange a winning ticket (increment count)
-- =============================================
IF OBJECT_ID('dbo.WinTicketExchange', 'P') IS NOT NULL
    DROP PROCEDURE dbo.WinTicketExchange
GO

CREATE PROCEDURE [dbo].[WinTicketExchange]
    @issue INT,
    @itemno VARCHAR(20)
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE WinTicket SET num = num + 1 WHERE issue = @issue AND itemno = @itemno;
END
GO

-- =============================================
-- PART 10: Amphitheater System Stored Procedures
-- =============================================

-- =============================================
-- Procedure: AmphitheaterAddSeason
-- Description: Adds a new amphitheater season
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterAddSeason', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterAddSeason
GO

CREATE PROCEDURE [dbo].[AmphitheaterAddSeason]
    @section INT,
    @season INT,
    @round INT,
    @state INT
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO AmphitheaterSetting (section, season, [round], state, createdate, updatetime, winner) 
    VALUES (@section, @season, @round, @state, GETDATE(), GETDATE(), NULL);
END
GO

-- =============================================
-- Procedure: AmphitheaterDisuseSeason
-- Description: Updates amphitheater season state with winner
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterDisuseSeason', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterDisuseSeason
GO

CREATE PROCEDURE [dbo].[AmphitheaterDisuseSeason]
    @state INT,
    @winner VARCHAR(100),
    @season INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterSetting SET state = @state, updatetime = GETDATE(), winner = @winner WHERE season = @season;
END
GO

-- =============================================
-- Procedure: AmphitheaterUpdateRound
-- Description: Updates amphitheater round
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterUpdateRound', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterUpdateRound
GO

CREATE PROCEDURE [dbo].[AmphitheaterUpdateRound]
    @round INT,
    @season INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterSetting SET [round] = @round, updatetime = GETDATE() WHERE season = @season;
END
GO

-- =============================================
-- Procedure: AmphitheaterTeamSignUP
-- Description: Team registration for amphitheater
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterTeamSignUP', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterTeamSignUP
GO

CREATE PROCEDURE [dbo].[AmphitheaterTeamSignUP]
    @captain INT,
    @member VARCHAR(100),
    @state INT,
    @teamID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterTeam SET captain = @captain, member = @member, state = @state, updatetime = GETDATE() WHERE id = @teamID;
END
GO

-- =============================================
-- Procedure: AmphitheaterTeamCancel
-- Description: Cancel team registration
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterTeamCancel', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterTeamCancel
GO

CREATE PROCEDURE [dbo].[AmphitheaterTeamCancel]
    @state INT,
    @teamID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterTeam SET captain = NULL, member = NULL, matchno = 0, state = @state, updatetime = GETDATE() WHERE id = @teamID;
END
GO

-- =============================================
-- Procedure: AmphitheaterUpdateMapNum
-- Description: Update map flag for team
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterUpdateMapNum', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterUpdateMapNum
GO

CREATE PROCEDURE [dbo].[AmphitheaterUpdateMapNum]
    @mapflag INT,
    @teamID INT,
    @mapID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterTeam SET mapflag = @mapflag WHERE id = @teamID AND map = @mapID;
END
GO

-- =============================================
-- Procedure: AddMoney
-- Description: Add gold to character - BIGINT for 100B cap
-- =============================================
IF OBJECT_ID('dbo.AddMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AddMoney
GO

CREATE PROCEDURE [dbo].[AddMoney]
    @money BIGINT,
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @newGold BIGINT;
    DECLARE @currentGold BIGINT;
    
    SELECT @currentGold = gd FROM character WHERE cha_id = @cha_id;
    SET @newGold = @currentGold + @money;
    
    -- Cap at 100 billion, floor at 0
    IF @newGold > 100000000000
        SET @newGold = 100000000000;
    IF @newGold < 0
        SET @newGold = 0;
    
    UPDATE character SET gd = @newGold WHERE cha_id = @cha_id;
END
GO

-- =============================================
-- Procedure: AmphitheaterUpdateMap
-- Description: Clear map for amphitheater team
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterUpdateMap', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterUpdateMap
GO

CREATE PROCEDURE [dbo].[AmphitheaterUpdateMap]
    @mapID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterTeam SET map = NULL WHERE map = @mapID;
END
GO

-- =============================================
-- Procedure: AmphitheaterUpdateAbsentTeamRelive
-- Description: Update absent team state to relive
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterUpdateAbsentTeamRelive', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterUpdateAbsentTeamRelive
GO

CREATE PROCEDURE [dbo].[AmphitheaterUpdateAbsentTeamRelive]
    @reliveState INT,
    @useState INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterTeam SET state = @reliveState WHERE state = @useState;
END
GO

-- =============================================
-- Procedure: AmphitheaterUpdateMapAfterEnter
-- Description: Update team map after entering
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterUpdateMapAfterEnter', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterUpdateMapAfterEnter
GO

CREATE PROCEDURE [dbo].[AmphitheaterUpdateMapAfterEnter]
    @mapID INT,
    @captainID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterTeam SET map = @mapID WHERE captain = @captainID;
END
GO

-- =============================================
-- Procedure: AmphitheaterUpdateWinnum
-- Description: Increment team win count
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterUpdateWinnum', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterUpdateWinnum
GO

CREATE PROCEDURE [dbo].[AmphitheaterUpdateWinnum]
    @teamID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterTeam SET winnum = winnum + 1 WHERE id = @teamID;
END
GO

-- =============================================
-- Procedure: AmphitheaterSetMatchnoState
-- Description: Set match number state
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterSetMatchnoState', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterSetMatchnoState
GO

CREATE PROCEDURE [dbo].[AmphitheaterSetMatchnoState]
    @teamID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterTeam SET matchno = 1 WHERE id = @teamID;
END
GO

-- =============================================
-- Procedure: AmphitheaterUpdateState
-- Description: Update team state from promotion to use
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterUpdateState', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterUpdateState
GO

CREATE PROCEDURE [dbo].[AmphitheaterUpdateState]
    @useState INT,
    @promotionState INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterTeam SET state = @useState WHERE state = @promotionState;
END
GO

-- =============================================
-- Procedure: AmphitheaterSetTeamState
-- Description: Set team state for match result
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterSetTeamState', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterSetTeamState
GO

CREATE PROCEDURE [dbo].[AmphitheaterSetTeamState]
    @state INT,
    @teamID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterTeam SET state = @state WHERE id = @teamID;
END
GO

-- =============================================
-- Procedure: AmphitheaterUpdateReliveNum
-- Description: Update relive number for team
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterUpdateReliveNum', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterUpdateReliveNum
GO

CREATE PROCEDURE [dbo].[AmphitheaterUpdateReliveNum]
    @relivenum INT,
    @teamID INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterTeam SET relivenum = @relivenum WHERE id = @teamID;
END
GO

-- =============================================
-- Procedure: AmphitheaterCleanMapFlag
-- Description: Clear map flag for two teams
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterCleanMapFlag', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterCleanMapFlag
GO

CREATE PROCEDURE [dbo].[AmphitheaterCleanMapFlag]
    @teamID1 INT,
    @teamID2 INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE AmphitheaterTeam SET mapflag = NULL WHERE id = @teamID1 OR id = @teamID2;
END
GO

-- =============================================
-- Procedure: SaveIMP
-- Description: Save character IMP value
-- =============================================
IF OBJECT_ID('dbo.SaveIMP', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveIMP
GO

CREATE PROCEDURE [dbo].[SaveIMP]
    @IMP INT,
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET IMP = @IMP WHERE cha_id = @cha_id;
END
GO

-- =============================================
-- Procedure: SaveGmLv
-- Description: Save account GM level
-- =============================================
IF OBJECT_ID('dbo.SaveGmLv', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveGmLv
GO

CREATE PROCEDURE [dbo].[SaveGmLv]
    @gmLv INT,
    @act_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE account SET gm = @gmLv WHERE act_id = @act_id;
END
GO

-- =============================================
-- Procedure: ResourceCreate
-- Description: Create a new resource record and return the new ID
-- =============================================
IF OBJECT_ID('dbo.ResourceCreate', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ResourceCreate
GO

CREATE PROCEDURE [dbo].[ResourceCreate]
    @cha_id INT,
    @type_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO resource (cha_id, type_id) VALUES (@cha_id, @type_id);
    SELECT SCOPE_IDENTITY() AS NewID;
END
GO

-- =============================================
-- Procedure: MapMaskCreate
-- Description: Create a new map mask record and return the new ID
-- =============================================
IF OBJECT_ID('dbo.MapMaskCreate', 'P') IS NOT NULL
    DROP PROCEDURE dbo.MapMaskCreate
GO

CREATE PROCEDURE [dbo].[MapMaskCreate]
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO map_mask (cha_id) VALUES (@cha_id);
    SELECT SCOPE_IDENTITY() AS NewID;
END
GO

-- =============================================
-- Procedure: BankCreate
-- Description: Create a new bank record and return the new ID
-- =============================================
IF OBJECT_ID('dbo.BankCreate', 'P') IS NOT NULL
    DROP PROCEDURE dbo.BankCreate
GO

CREATE PROCEDURE [dbo].[BankCreate]
    @cha_id INT,
    @type_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO resource (cha_id, type_id) VALUES (@cha_id, @type_id);
    SELECT SCOPE_IDENTITY() AS NewID;
END
GO

-- =============================================
-- Procedure: SaveBankData
-- Description: Save bank data content
-- =============================================
IF OBJECT_ID('dbo.SaveBankData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.SaveBankData
GO

CREATE PROCEDURE [dbo].[SaveBankData]
    @content NVARCHAR(MAX),
    @id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE resource SET content = @content WHERE id = @id;
END
GO

-- =============================================
-- Procedure: BoatCreate
-- Description: Create a new boat record and return the new ID
-- =============================================
IF OBJECT_ID('dbo.BoatCreate', 'P') IS NOT NULL
    DROP PROCEDURE dbo.BoatCreate
GO

CREATE PROCEDURE [dbo].[BoatCreate]
    @boat_name NVARCHAR(50),
    @boat_berth INT,
    @boat_boatid INT,
    @boat_header INT,
    @boat_body INT,
    @boat_engine INT,
    @boat_cannon INT,
    @boat_equipment INT,
    @boat_bagsize INT,
    @boat_bag NVARCHAR(MAX),
    @boat_diecount INT,
    @boat_isdead INT,
    @boat_ownerid INT,
    @boat_createtime NVARCHAR(50),
    @boat_isdeleted INT
AS
BEGIN
    SET NOCOUNT ON;

    -- Determine initial map position based on berth ID
    -- This prevents the bluescreen bug when teleporting before boat position is saved
    DECLARE @init_map NVARCHAR(50);
    DECLARE @init_map_x INT;
    DECLARE @init_map_y INT;

    -- Set default harbor position based on berth
    -- Berth IDs: 1=Argent, 2=Shaitan, 3=Icicle ... (Custom mapping can be adjusted)
    SELECT @init_map = CASE @boat_berth
        WHEN 1 THEN 'garner'      -- Argent Harbor
        WHEN 2 THEN 'magicsea'    -- Shaitan Harbor
        WHEN 3 THEN 'winterland'  -- Icicle Harbor
        WHEN 4 THEN 'darkblue'    -- Thundoria Harbor
        ELSE 'garner'             -- Default to Argent Harbor
    END;

    -- Default harbor coordinates (Argent Harbor area - safe location)
    -- This is a fallback to ensure we never have -1, -1 (Void/Blue Map)
    SET @init_map_x = 219275;
    SET @init_map_y = 277825;

    -- Insert with initialized map position to prevent corruption
    INSERT INTO boat (boat_name, boat_berth, boat_boatid, boat_header, boat_body, 
                     boat_engine, boat_cannon, boat_equipment, boat_bagsize, boat_bag, boat_diecount,
                     boat_isdead, boat_ownerid, boat_createtime, boat_isdeleted,
                     map, map_x, map_y)
    VALUES (@boat_name, @boat_berth, @boat_boatid, @boat_header, @boat_body,
            @boat_engine, @boat_cannon, @boat_equipment, @boat_bagsize, @boat_bag, @boat_diecount,
            @boat_isdead, @boat_ownerid, @boat_createtime, @boat_isdeleted,
            @init_map, @init_map_x, @init_map_y);
            
    SELECT SCOPE_IDENTITY() AS NewID;
END
GO

-- =============================================
-- GUILD SYSTEM STORED PROCEDURES
-- =============================================

-- =============================================
-- Procedure: GuildCreate
-- Description: Create a new guild by claiming an empty guild slot
-- =============================================
IF OBJECT_ID('dbo.GuildCreate', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildCreate
GO

CREATE PROCEDURE [dbo].[GuildCreate]
    @leader_id INT,
    @passwd NVARCHAR(50),
    @guild_name NVARCHAR(50),
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET leader_id = @leader_id, passwd = @passwd, guild_name = @guild_name, 
                     exp = 0, member_total = 1, try_total = 0
    WHERE leader_id = 0 AND guild_id = @guild_id;
END
GO

-- =============================================
-- Procedure: GuildSetCharacterGuild
-- Description: Set character's guild membership after guild creation
-- =============================================
IF OBJECT_ID('dbo.GuildSetCharacterGuild', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildSetCharacterGuild
GO

CREATE PROCEDURE [dbo].[GuildSetCharacterGuild]
    @guild_id INT,
    @guild_permission INT,
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET guild_id = @guild_id, guild_stat = 0, guild_permission = @guild_permission
    WHERE cha_id = @cha_id;
END
GO

-- =============================================
-- Procedure: GuildTryForApply
-- Description: Apply to join a guild (try for membership)
-- =============================================
IF OBJECT_ID('dbo.GuildTryForApply', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildTryForApply
GO

CREATE PROCEDURE [dbo].[GuildTryForApply]
    @guild_id INT,
    @cha_id INT,
    @max_try_members INT,
    @max_members INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET guild_id = @guild_id, guild_stat = 1, guild_permission = 0
    WHERE cha_id = @cha_id AND 
          @guild_id IN (SELECT guild_id FROM guild WHERE leader_id > 0 AND guild_id = @guild_id 
                       AND try_total < @max_try_members AND member_total < @max_members);
END
GO

-- =============================================
-- Procedure: GuildIncrementTryTotal
-- Description: Increment try_total count for guild
-- =============================================
IF OBJECT_ID('dbo.GuildIncrementTryTotal', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildIncrementTryTotal
GO

CREATE PROCEDURE [dbo].[GuildIncrementTryTotal]
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET try_total = try_total + 1 WHERE guild_id = @guild_id;
END
GO

-- =============================================
-- Procedure: GuildDecrementTryTotal
-- Description: Decrement try_total count for guild
-- =============================================
IF OBJECT_ID('dbo.GuildDecrementTryTotal', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildDecrementTryTotal
GO

CREATE PROCEDURE [dbo].[GuildDecrementTryTotal]
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET try_total = try_total - 1 WHERE guild_id = @guild_id AND try_total > 0;
END
GO

-- =============================================
-- Procedure: GuildSetBankLog
-- Description: Update guild bank log
-- =============================================
IF OBJECT_ID('dbo.GuildSetBankLog', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildSetBankLog
GO

CREATE PROCEDURE [dbo].[GuildSetBankLog]
    @banklog NVARCHAR(MAX),
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild WITH (ROWLOCK) SET banklog = @banklog WHERE guild_id = @guild_id;
END
GO

-- =============================================
-- Procedure: GuildApprove
-- Description: Approve a player's guild application (part 1 - update guild)
-- =============================================
IF OBJECT_ID('dbo.GuildApproveUpdateGuild', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildApproveUpdateGuild
GO

CREATE PROCEDURE [dbo].[GuildApproveUpdateGuild]
    @guild_id INT,
    @max_members INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET try_total = try_total - 1, member_total = member_total + 1
    WHERE guild_id = @guild_id AND member_total < @max_members AND try_total > 0;
END
GO

-- =============================================
-- Procedure: GuildApproveUpdateCharacter
-- Description: Approve a player's guild application (part 2 - update character)
-- =============================================
IF OBJECT_ID('dbo.GuildApproveUpdateCharacter', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildApproveUpdateCharacter
GO

CREATE PROCEDURE [dbo].[GuildApproveUpdateCharacter]
    @guild_permission INT,
    @cha_id INT,
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET guild_stat = 0, guild_permission = @guild_permission
    WHERE cha_id = @cha_id AND guild_id = @guild_id AND guild_stat = 1 AND delflag = 0;
END
GO

-- =============================================
-- Procedure: GuildRejectUpdateCharacter
-- Description: Reject a player's guild application (update character)
-- =============================================
IF OBJECT_ID('dbo.GuildRejectUpdateCharacter', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildRejectUpdateCharacter
GO

CREATE PROCEDURE [dbo].[GuildRejectUpdateCharacter]
    @cha_id INT,
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET guild_id = 0, guild_stat = 0, guild_permission = 0
    WHERE cha_id = @cha_id AND guild_id = @guild_id AND guild_stat = 1;
END
GO

-- =============================================
-- Procedure: GuildKickUpdateCharacter
-- Description: Kick a member from guild (update character)
-- =============================================
IF OBJECT_ID('dbo.GuildKickUpdateCharacter', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildKickUpdateCharacter
GO

CREATE PROCEDURE [dbo].[GuildKickUpdateCharacter]
    @cha_id INT,
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET guild_id = 0, guild_stat = 0, guild_permission = 0
    WHERE cha_id = @cha_id AND guild_id = @guild_id AND guild_stat = 0 
          AND cha_id NOT IN (SELECT leader_id FROM guild WHERE guild_id = @guild_id);
END
GO

-- =============================================
-- Procedure: GuildDecrementMemberTotal
-- Description: Decrement member_total count for guild
-- =============================================
IF OBJECT_ID('dbo.GuildDecrementMemberTotal', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildDecrementMemberTotal
GO

CREATE PROCEDURE [dbo].[GuildDecrementMemberTotal]
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET member_total = member_total - 1 WHERE guild_id = @guild_id;
END
GO

-- =============================================
-- Procedure: GuildLeaveUpdateCharacter
-- Description: Leave guild (update character)
-- =============================================
IF OBJECT_ID('dbo.GuildLeaveUpdateCharacter', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildLeaveUpdateCharacter
GO

CREATE PROCEDURE [dbo].[GuildLeaveUpdateCharacter]
    @cha_id INT,
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET guild_id = 0, guild_stat = 0, guild_permission = 0
    WHERE cha_id = @cha_id AND guild_id = @guild_id AND guild_stat = 0
          AND cha_id NOT IN (SELECT leader_id FROM guild WHERE guild_id = @guild_id);
END
GO

-- =============================================
-- Procedure: GuildDisbandUpdateGuild
-- Description: Disband guild (update guild table)
-- =============================================
IF OBJECT_ID('dbo.GuildDisbandUpdateGuild', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildDisbandUpdateGuild
GO

CREATE PROCEDURE [dbo].[GuildDisbandUpdateGuild]
    @guild_id INT,
    @passwd NVARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET level = 0, gold = 0, bank = '', motto = '', passwd = '', 
                     leader_id = 0, exp = 0, member_total = 0, try_total = 0
    WHERE guild_id = @guild_id AND passwd = @passwd;
END
GO

-- =============================================
-- Procedure: GuildDisbandUpdateCharacters
-- Description: Disband guild (update all member characters)
-- =============================================
IF OBJECT_ID('dbo.GuildDisbandUpdateCharacters', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildDisbandUpdateCharacters
GO

CREATE PROCEDURE [dbo].[GuildDisbandUpdateCharacters]
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET guild_id = 0, guild_stat = 0, guild_permission = 0
    WHERE guild_id = @guild_id;
END
GO

-- =============================================
-- Procedure: GuildUpdateMotto
-- Description: Update guild motto
-- =============================================
IF OBJECT_ID('dbo.GuildUpdateMotto', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildUpdateMotto
GO

CREATE PROCEDURE [dbo].[GuildUpdateMotto]
    @motto NVARCHAR(200),
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET motto = @motto WHERE guild_id = @guild_id;
END
GO

-- =============================================
-- Procedure: GuildLeizhuSetLevel
-- Description: Set guild level for Leizhu (claim title)
-- =============================================
IF OBJECT_ID('dbo.GuildLeizhuSetLevel', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildLeizhuSetLevel
GO

CREATE PROCEDURE [dbo].[GuildLeizhuSetLevel]
    @chall_level TINYINT,
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET challid = 0, challstart = 0, challmoney = 0, challlevel = @chall_level
    WHERE guild_id = @guild_id;
END
GO

-- =============================================
-- Procedure: GuildChallengeUpdate
-- Description: Update challenger for guild battle - BIGINT for 100B cap
-- =============================================
IF OBJECT_ID('dbo.GuildChallengeUpdate', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildChallengeUpdate
GO

CREATE PROCEDURE [dbo].[GuildChallengeUpdate]
    @chall_id INT,
    @chall_money BIGINT,
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    -- Cap challenge money at 100 billion
    DECLARE @safeMoney BIGINT;
    SET @safeMoney = @chall_money;
    IF @safeMoney > 100000000000
        SET @safeMoney = 100000000000;
    IF @safeMoney < 0
        SET @safeMoney = 0;
    
    UPDATE guild SET challid = @chall_id, challmoney = @safeMoney 
    WHERE guild_id = @guild_id AND challmoney < @safeMoney AND challstart = 0;
END
GO

-- =============================================
-- Procedure: GuildStartChallenge
-- Description: Start guild challenge battle
-- =============================================
IF OBJECT_ID('dbo.GuildStartChallenge', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildStartChallenge
GO

CREATE PROCEDURE [dbo].[GuildStartChallenge]
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET challstart = 1 WHERE guild_id = @guild_id AND challstart = 0;
END
GO

-- =============================================
-- Procedure: GuildChallWinUpdateLoser
-- Description: Update loser guild after challenge (with new level)
-- =============================================
IF OBJECT_ID('dbo.GuildChallWinUpdateLoser', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildChallWinUpdateLoser
GO

CREATE PROCEDURE [dbo].[GuildChallWinUpdateLoser]
    @chall_level TINYINT,
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET challid = 0, challstart = 0, challmoney = 0, challlevel = @chall_level
    WHERE guild_id = @guild_id;
END
GO

-- =============================================
-- Procedure: GuildChallWinUpdateLoserNoLevel
-- Description: Update loser guild after challenge (reset level to 0)
-- =============================================
IF OBJECT_ID('dbo.GuildChallWinUpdateLoserNoLevel', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildChallWinUpdateLoserNoLevel
GO

CREATE PROCEDURE [dbo].[GuildChallWinUpdateLoserNoLevel]
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET challid = 0, challstart = 0, challmoney = 0, challlevel = 0
    WHERE guild_id = @guild_id;
END
GO

-- =============================================
-- Procedure: GuildChallWinUpdateWinner
-- Description: Update winner guild after challenge
-- =============================================
IF OBJECT_ID('dbo.GuildChallWinUpdateWinner', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildChallWinUpdateWinner
GO

CREATE PROCEDURE [dbo].[GuildChallWinUpdateWinner]
    @chall_level TINYINT,
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild SET challid = 0, challstart = 0, challmoney = 0, challlevel = @chall_level
    WHERE guild_id = @guild_id;
END
GO

-- =============================================
-- Procedure: GuildUpdateBank
-- Description: Update guild bank contents
-- =============================================
IF OBJECT_ID('dbo.GuildUpdateBank', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildUpdateBank
GO

CREATE PROCEDURE [dbo].[GuildUpdateBank]
    @bank_data NVARCHAR(MAX),
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE guild WITH (ROWLOCK) SET bank = @bank_data WHERE guild_id = @guild_id;
END
GO

-- =============================================
-- Procedure: GuildUpdateBankGold
-- Description: Update guild bank gold (add/subtract)
-- =============================================
IF OBJECT_ID('dbo.GuildUpdateBankGold', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildUpdateBankGold
GO

CREATE PROCEDURE [dbo].[GuildUpdateBankGold]
    @money BIGINT,
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @currentGold BIGINT;
    DECLARE @newGold BIGINT;
    
    SELECT @currentGold = gold FROM guild WITH (ROWLOCK) WHERE guild_id = @guild_id;
    SET @newGold = @currentGold + @money;
    
    -- Cap at 100 billion, floor at 0
    IF @newGold > 100000000000
        SET @newGold = 100000000000;
    IF @newGold < 0
        SET @newGold = 0;
    
    UPDATE guild SET gold = @newGold WHERE guild_id = @guild_id;
END
GO

-- =============================================
-- Procedure: AmphitheaterSetMaxBallotRelive
-- Description: Set max ballot teams to relive state
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterSetMaxBallotRelive', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterSetMaxBallotRelive
GO

CREATE PROCEDURE [dbo].[AmphitheaterSetMaxBallotRelive]
    @odd_or_even INT,
    @promotion_state INT,
    @relive_state INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE amphitheater_team SET state = @promotion_state, relivenum = 0 
    WHERE id IN (SELECT TOP (@odd_or_even) id FROM amphitheater_team WHERE state = @relive_state ORDER BY relivenum DESC);
END
GO

-- =============================================
-- Procedure: AmphitheaterSetOutState
-- Description: Set teams to out state
-- =============================================
IF OBJECT_ID('dbo.AmphitheaterSetOutState', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AmphitheaterSetOutState
GO

CREATE PROCEDURE [dbo].[AmphitheaterSetOutState]
    @out_state INT,
    @relive_state INT,
    @use_state INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE amphitheater_team SET state = @out_state, relivenum = 0 
    WHERE state = @relive_state OR state = @use_state;
END
GO

-- =============================================
-- Procedure: PropertyInsert
-- Description: Insert item property record
-- =============================================
IF OBJECT_ID('dbo.PropertyInsert', 'P') IS NOT NULL
    DROP PROCEDURE dbo.PropertyInsert
GO

CREATE PROCEDURE [dbo].[PropertyInsert]
    @id INT,
    @cha_id INT,
    @context NVARCHAR(MAX),
    @sum INT
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO property (id, cha_id, context, sum, time) VALUES (@id, @cha_id, @context, @sum, GETDATE());
END
GO

-- =============================================
-- Procedure: PropertyDelete
-- Description: Delete item property record
-- =============================================
IF OBJECT_ID('dbo.PropertyDelete', 'P') IS NOT NULL
    DROP PROCEDURE dbo.PropertyDelete
GO

CREATE PROCEDURE [dbo].[PropertyDelete]
    @id INT,
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DELETE FROM property WHERE id = @id AND cha_id = @cha_id;
END
GO

-- =============================================
-- Procedure: BoatSaveWithPos
-- Description: Save boat data including position
-- =============================================
IF OBJECT_ID('dbo.BoatSaveWithPos', 'P') IS NOT NULL
    DROP PROCEDURE dbo.BoatSaveWithPos
GO

CREATE PROCEDURE [dbo].[BoatSaveWithPos]
    @boat_berth INT,
    @boat_ownerid INT,
    @cur_endure INT,
    @mx_endure INT,
    @cur_supply INT,
    @mx_supply INT,
    @skill_state NVARCHAR(MAX),
    @map NVARCHAR(50),
    @map_x INT,
    @map_y INT,
    @angle INT,
    @degree INT,
    @exp INT,
    @boat_bag NVARCHAR(MAX),
    @boat_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE boat SET boat_berth = @boat_berth, boat_ownerid = @boat_ownerid,
        cur_endure = @cur_endure, mx_endure = @mx_endure,
        cur_supply = @cur_supply, mx_supply = @mx_supply,
        skill_state = @skill_state, map = @map, map_x = @map_x, map_y = @map_y,
        angle = @angle, degree = @degree, exp = @exp, boat_bag = @boat_bag
    WHERE boat_id = @boat_id;
END
GO

-- =============================================
-- Procedure: BoatSaveNoPos
-- Description: Save boat data without position
-- =============================================
IF OBJECT_ID('dbo.BoatSaveNoPos', 'P') IS NOT NULL
    DROP PROCEDURE dbo.BoatSaveNoPos
GO

CREATE PROCEDURE [dbo].[BoatSaveNoPos]
    @boat_berth INT,
    @boat_ownerid INT,
    @cur_endure INT,
    @mx_endure INT,
    @cur_supply INT,
    @mx_supply INT,
    @skill_state NVARCHAR(MAX),
    @degree INT,
    @exp INT,
    @boat_bag NVARCHAR(MAX),
    @boat_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE boat SET boat_berth = @boat_berth, boat_ownerid = @boat_ownerid,
        cur_endure = @cur_endure, mx_endure = @mx_endure,
        cur_supply = @cur_supply, mx_supply = @mx_supply,
        skill_state = @skill_state, degree = @degree, exp = @exp, boat_bag = @boat_bag
    WHERE boat_id = @boat_id;
END
GO

PRINT ''
PRINT 'All stored procedures created successfully!'
PRINT ''
PRINT '=== AccountServer Database Procedures ==='
PRINT 'InsertNewUser, UpdateUserPassword, InsertUserLogin, UpdateUserLogout,'
PRINT 'GetAccountInfo, OperAccountBan, ValidateUserLogin, UpdateLastLoginMac,'
PRINT 'GetServerList, CheckUsernameExists'
PRINT ''
PRINT '=== GameDB Database Procedures ==='
PRINT 'AddStatLog, SetDiscInfo'
PRINT ''
PRINT '=== GameDB Character Procedures (Legacy) ==='
PRINT 'VerifyCharacterName, GetCharacterName, ReadCharacterData, SaveCharacterData,'
PRINT 'SaveCharacterPosition, SaveCharacterMoney, AddCharacterMoney, AddCharacterCredit,'
PRINT 'SaveKitbagDBID, SaveKitbagTmpDBID, SaveKitbagLockState, SaveStoreItemID,'
PRINT 'IsCharacterOnline, SaveMapMaskDBID, SaveBankDBID, SaveMissionData,'
PRINT 'UpdateCharacterGuild, CreateNewCharacter, DeleteCharacter, GetExpRank,'
PRINT 'SetCharacterOnlineStatus'
PRINT ''
PRINT '=== GameDB Boat/Ship Procedures ==='
PRINT 'GetBoatData, SaveBoatData'
PRINT ''
PRINT '=== GameDB Resource Procedures ==='
PRINT 'GetResourceData, SaveResourceData, InsertResourceData'
PRINT ''
PRINT '=== Account/Character Procedures ==='
PRINT 'SaveIMP, SaveGmLv'
PRINT ''
PRINT '=== TOP-Master Compatible Procedures ==='
PRINT 'ReadAllData, SaveAllData, SaveAllDataWithoutPos, SavePos, SaveMoney, AddMoney,'
PRINT 'SaveKBagDBIDEx, SaveKBagTmpDBID, SaveKBState, SaveStoreItemID, AddCreditByDBID,'
PRINT 'IsChaOnline, SaveBankDBID, SaveMissionData, SetAddr, ZeroAddr,'
PRINT 'GetBoat, CreateBoat, SaveBoatExWithPos, SaveBoatEx, SaveBoatDelTag,'
PRINT 'ReadCabin, SaveCabin, ReadBankDataEx, SaveBankDataEx, ReadKitbagTmpData,'
PRINT 'ReadKitbagTmpDataEx, SaveKitbagTmpDataEx, ResourceCreate,'
PRINT 'GetGuildName, GetGuildMemberNum, CancelTryFor, Leave1, Leave2, Kick2,'
PRINT 'Disband1, Disband2, AddFriend, DelFriend, AddMaster, DelMaster, HasMaster,'
PRINT 'FinishMaster, UpdateInfo, FetchRowByChaName, FetchAccidByChaName,'
PRINT 'BackupRow, BackupRow1, InsertRow3'
PRINT ''
PRINT '=== Lottery System Procedures ==='
PRINT 'LotteryAddIssue, LotteryDisuseIssue, LotterySetWinItemNo,'
PRINT 'TicketAddTicket, WinTicketAddTicket, WinTicketExchange'
PRINT ''
PRINT '=== Amphitheater System Procedures ==='
PRINT 'AmphitheaterAddSeason, AmphitheaterDisuseSeason, AmphitheaterUpdateRound,'
PRINT 'AmphitheaterTeamSignUP, AmphitheaterTeamCancel, AmphitheaterUpdateMapNum'
GO

-- =============================================

-- PART 6: GroupServer Database Stored Procedures
-- =============================================
USE [GameDB]
GO

-- =============================================
-- Procedure: AccountSaveInsert
-- Description: Inserts a new account_save record with auto-generated ID
-- Parameters:
--   @act_name - Account name
--   @cha_ids - Character IDs string
-- =============================================
IF OBJECT_ID('dbo.AccountSaveInsert', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AccountSaveInsert
GO

CREATE PROCEDURE [dbo].[AccountSaveInsert]
    @act_name VARCHAR(50),
    @cha_ids VARCHAR(255)
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @new_act_id INT;
    SELECT @new_act_id = ISNULL(MAX(act_id), 0) + 1 FROM account;
    
    INSERT INTO account (act_id, act_name, cha_ids) 
    VALUES (@new_act_id, @act_name, @cha_ids);
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: AccountSaveUpdateChaIds
-- Description: Updates character IDs for an account
-- Parameters:
--   @cha_ids - New character IDs string
--   @act_id - Account ID
-- =============================================
IF OBJECT_ID('dbo.AccountSaveUpdateChaIds', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AccountSaveUpdateChaIds
GO

CREATE PROCEDURE [dbo].[AccountSaveUpdateChaIds]
    @cha_ids VARCHAR(255),
    @act_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE account SET cha_ids = @cha_ids WHERE act_id = @act_id;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: AccountSaveUpdatePassword
-- Description: Updates password for an account
-- Parameters:
--   @password - New password
--   @act_id - Account ID
-- =============================================
IF OBJECT_ID('dbo.AccountSaveUpdatePassword', 'P') IS NOT NULL
    DROP PROCEDURE dbo.AccountSaveUpdatePassword
GO

CREATE PROCEDURE [dbo].[AccountSaveUpdatePassword]
    @password VARCHAR(255),
    @act_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE account SET password = @password WHERE act_id = @act_id;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: CharacterZeroAddr
-- Description: Resets all character memory addresses on server startup
-- =============================================
IF OBJECT_ID('dbo.CharacterZeroAddr', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CharacterZeroAddr
GO

CREATE PROCEDURE [dbo].[CharacterZeroAddr]
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET mem_addr = 0 WHERE mem_addr != 0;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: CharacterSetAddr
-- Description: Sets memory address for a character
-- Parameters:
--   @addr - Memory address
--   @cha_id - Character ID
-- =============================================
IF OBJECT_ID('dbo.CharacterSetAddr', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CharacterSetAddr
GO

CREATE PROCEDURE [dbo].[CharacterSetAddr]
    @addr BIGINT,
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET mem_addr = @addr WHERE cha_id = @cha_id;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: CharacterInsert
-- Description: Inserts a new character record
-- Parameters:
--   @cha_name - Character name
--   @act_id - Account ID
--   @birth - Birth date
--   @map - Starting map
--   @look - Character appearance
-- =============================================
IF OBJECT_ID('dbo.CharacterInsert', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CharacterInsert
GO

CREATE PROCEDURE [dbo].[CharacterInsert]
    @cha_name VARCHAR(50),
    @act_id INT,
    @birth VARCHAR(20),
    @map VARCHAR(50),
    @look VARCHAR(2000)
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO character (cha_name, act_id, birth, map, look) 
    VALUES (@cha_name, @act_id, @birth, @map, @look);
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: CharacterUpdateInfo
-- Description: Updates character icon and motto
-- Parameters:
--   @icon - Character icon
--   @motto - Character motto
--   @cha_id - Character ID
-- =============================================
IF OBJECT_ID('dbo.CharacterUpdateInfo', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CharacterUpdateInfo
GO

CREATE PROCEDURE [dbo].[CharacterUpdateInfo]
    @icon SMALLINT,
    @motto VARCHAR(255),
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET icon = @icon, motto = @motto WHERE cha_id = @cha_id;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: CharacterStartEstop
-- Description: Starts estop time for a character
-- Parameters:
--   @cha_id - Character ID
-- =============================================
IF OBJECT_ID('dbo.CharacterStartEstop', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CharacterStartEstop
GO

CREATE PROCEDURE [dbo].[CharacterStartEstop]
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET estop = GETDATE() WHERE cha_id = @cha_id;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: CharacterEndEstop
-- Description: Ends estop time for a character (calculates remaining time)
-- Parameters:
--   @cha_id - Character ID
-- =============================================
IF OBJECT_ID('dbo.CharacterEndEstop', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CharacterEndEstop
GO

CREATE PROCEDURE [dbo].[CharacterEndEstop]
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character 
    SET estoptime = estoptime - DATEDIFF(MINUTE, estop, GETDATE()) 
    WHERE cha_id = @cha_id;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: CharacterEstop
-- Description: Sets estop time for a character by name
-- Parameters:
--   @estoptime - Estop duration in minutes
--   @cha_name - Character name
-- =============================================
IF OBJECT_ID('dbo.CharacterEstop', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CharacterEstop
GO

CREATE PROCEDURE [dbo].[CharacterEstop]
    @estoptime INT,
    @cha_name VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET estop = GETDATE(), estoptime = @estoptime WHERE cha_name = @cha_name;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: CharacterAddMoney
-- Description: Adds money to a character
-- Parameters:
--   @money - Amount to add (BIGINT for 100B gold cap)
--   @cha_id - Character ID
-- =============================================
IF OBJECT_ID('dbo.CharacterAddMoney', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CharacterAddMoney
GO

CREATE PROCEDURE [dbo].[CharacterAddMoney]
    @money BIGINT,
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET gd = gd + @money WHERE cha_id = @cha_id;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: CharacterDelEstop
-- Description: Removes estop from a character by name
-- Parameters:
--   @cha_name - Character name
-- =============================================
IF OBJECT_ID('dbo.CharacterDelEstop', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CharacterDelEstop
GO

CREATE PROCEDURE [dbo].[CharacterDelEstop]
    @cha_name VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE character SET estoptime = 0 WHERE cha_name = @cha_name;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: CharacterBackupRow
-- Description: Marks a character as deleted and updates guild counts
-- Parameters:
--   @cha_id - Character ID
-- =============================================
IF OBJECT_ID('dbo.CharacterBackupRow', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CharacterBackupRow
GO

CREATE PROCEDURE [dbo].[CharacterBackupRow]
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @guild_id INT, @guild_stat TINYINT;
    
    -- Get guild info for the character
    SELECT @guild_id = guild_id, @guild_stat = guild_stat 
    FROM character WHERE cha_id = @cha_id;
    
    IF @guild_id IS NULL
    BEGIN
        RETURN -1; -- Character not found
    END
    
    -- Update guild counts if character is in a guild
    IF @guild_id > 0
    BEGIN
        IF @guild_stat = 0 -- Normal member (emGldMembStatNormal)
        BEGIN
            UPDATE guild SET member_total = member_total - 1 
            WHERE guild_id = @guild_id AND member_total > 0;
        END
        ELSE -- Trying/applying status
        BEGIN
            UPDATE guild SET try_total = try_total - 1 
            WHERE guild_id = @guild_id AND try_total > 0;
        END
    END
    
    -- Mark character as deleted
    UPDATE character SET delflag = 1, deldate = GETDATE() WHERE cha_id = @cha_id;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: FriendsUpdateGroupById
-- Description: Updates friend relation by character IDs
-- Parameters:
--   @newgroup - New group/relation name
--   @cha_id1 - First character ID
--   @cha_id2 - Second character ID
-- =============================================
IF OBJECT_ID('dbo.FriendsUpdateGroupById', 'P') IS NOT NULL
    DROP PROCEDURE dbo.FriendsUpdateGroupById
GO

CREATE PROCEDURE [dbo].[FriendsUpdateGroupById]
    @newgroup VARCHAR(50),
    @cha_id1 INT,
    @cha_id2 INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE friends SET relation = @newgroup WHERE cha_id1 = @cha_id1 AND cha_id2 = @cha_id2;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: FriendsUpdateGroupByName
-- Description: Updates friend relation by group name
-- Parameters:
--   @newgroup - New group/relation name
--   @cha_id1 - Character ID
--   @oldgroup - Old group/relation name
-- =============================================
IF OBJECT_ID('dbo.FriendsUpdateGroupByName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.FriendsUpdateGroupByName
GO

CREATE PROCEDURE [dbo].[FriendsUpdateGroupByName]
    @newgroup VARCHAR(50),
    @cha_id1 INT,
    @oldgroup VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE friends SET relation = @newgroup WHERE cha_id1 = @cha_id1 AND relation = @oldgroup;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: FriendsAdd
-- Description: Adds bidirectional friendship
-- Parameters:
--   @cha_id1 - First character ID
--   @cha_id2 - Second character ID
-- =============================================
IF OBJECT_ID('dbo.FriendsAdd', 'P') IS NOT NULL
    DROP PROCEDURE dbo.FriendsAdd
GO

CREATE PROCEDURE [dbo].[FriendsAdd]
    @cha_id1 INT,
    @cha_id2 INT
AS
BEGIN
    SET NOCOUNT ON;
    
    -- Add both directions of friendship with default group
    INSERT INTO friends (cha_id1, cha_id2, relation) VALUES (@cha_id1, @cha_id2, 'Friend');
    INSERT INTO friends (cha_id1, cha_id2, relation) VALUES (@cha_id2, @cha_id1, 'Friend');
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: FriendsDelete
-- Description: Deletes bidirectional friendship
-- Parameters:
--   @cha_id1 - First character ID
--   @cha_id2 - Second character ID
-- =============================================
IF OBJECT_ID('dbo.FriendsDelete', 'P') IS NOT NULL
    DROP PROCEDURE dbo.FriendsDelete
GO

CREATE PROCEDURE [dbo].[FriendsDelete]
    @cha_id1 INT,
    @cha_id2 INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DELETE FROM friends WHERE (cha_id1 = @cha_id1 AND cha_id2 = @cha_id2) OR (cha_id1 = @cha_id2 AND cha_id2 = @cha_id1);
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: MasterAdd
-- Description: Adds master-apprentice relationship
-- Parameters:
--   @cha_id1 - Apprentice character ID
--   @cha_id2 - Master character ID
-- =============================================
IF OBJECT_ID('dbo.MasterAdd', 'P') IS NOT NULL
    DROP PROCEDURE dbo.MasterAdd
GO

CREATE PROCEDURE [dbo].[MasterAdd]
    @cha_id1 INT,
    @cha_id2 INT
AS
BEGIN
    SET NOCOUNT ON;
    
    INSERT INTO master (cha_id1, cha_id2, finish) VALUES (@cha_id1, @cha_id2, 0);
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: MasterDelete
-- Description: Deletes master-apprentice relationship
-- Parameters:
--   @cha_id1 - Apprentice character ID
--   @cha_id2 - Master character ID
-- =============================================
IF OBJECT_ID('dbo.MasterDelete', 'P') IS NOT NULL
    DROP PROCEDURE dbo.MasterDelete
GO

CREATE PROCEDURE [dbo].[MasterDelete]
    @cha_id1 INT,
    @cha_id2 INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DELETE FROM master WHERE cha_id1 = @cha_id1 AND cha_id2 = @cha_id2;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: MasterFinish
-- Description: Marks master relationship as finished
-- Parameters:
--   @cha_id - Apprentice character ID
-- =============================================
IF OBJECT_ID('dbo.MasterFinish', 'P') IS NOT NULL
    DROP PROCEDURE dbo.MasterFinish
GO

CREATE PROCEDURE [dbo].[MasterFinish]
    @cha_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE master SET finish = 1 WHERE cha_id1 = @cha_id;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: GuildDisband
-- Description: Disbands a guild and clears member associations
-- Parameters:
--   @guild_id - Guild ID
-- =============================================
IF OBJECT_ID('dbo.GuildDisband', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GuildDisband
GO

CREATE PROCEDURE [dbo].[GuildDisband]
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;

    -- Reset guild data
    UPDATE guild
    SET motto = '', passwd = '', leader_id = 0, gold = 0, exp = 0, member_total = 0, try_total = 0
    WHERE guild_id = @guild_id;

    -- Clear member guild associations
    UPDATE character
    SET guild_id = 0, guild_stat = 0, guild_permission = 0
    WHERE guild_id = @guild_id;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: ParamSave
-- Description: Saves ranking parameters
-- Parameters:
--   @param1-5 - Character IDs for top 5
--   @param6-10 - Fight points for top 5
-- =============================================
IF OBJECT_ID('dbo.ParamSave', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ParamSave
GO

CREATE PROCEDURE [dbo].[ParamSave]
    @param1 INT,
    @param2 INT,
    @param3 INT,
    @param4 INT,
    @param5 INT,
    @param6 INT,
    @param7 INT,
    @param8 INT,
    @param9 INT,
    @param10 INT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE param 
    SET param1 = @param1, param2 = @param2, param3 = @param3, param4 = @param4, param5 = @param5,
        param6 = @param6, param7 = @param7, param8 = @param8, param9 = @param9, param10 = @param10
    WHERE id = 1;
    
    RETURN @@ROWCOUNT;
END
GO

PRINT ''
PRINT '=== GroupServer Stored Procedures ==='
PRINT 'AccountSaveInsert, AccountSaveUpdateChaIds, AccountSaveUpdatePassword'
PRINT 'CharacterZeroAddr, CharacterSetAddr, CharacterInsert, CharacterUpdateInfo'
PRINT 'CharacterStartEstop, CharacterEndEstop, CharacterEstop, CharacterAddMoney, CharacterDelEstop'
PRINT 'CharacterBackupRow'
PRINT 'FriendsUpdateGroupById, FriendsUpdateGroupByName, FriendsAdd, FriendsDelete'
PRINT 'MasterAdd, MasterDelete, MasterFinish'
PRINT 'GuildDisband, ParamSave'
PRINT 'GetAccountByName, GetCharacterByChaName, GetAccountIdByChaName, GetGuildByName'
GO

-- =============================================
-- SECURITY FIX: SQL Injection Prevention Procedures
-- These procedures replace vulnerable _get_row() calls
-- that used string concatenation with user input
-- =============================================

-- =============================================
-- Procedure: GetAccountByName
-- Description: Safely fetches account by name (prevents SQL injection)
-- Parameters:
--   @actName - Account name to look up
-- Returns: act_id, gm, cha_ids, password, last_ip, disc_reason, last_leave
-- =============================================
IF OBJECT_ID('dbo.GetAccountByName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetAccountByName
GO

CREATE PROCEDURE [dbo].[GetAccountByName]
    @actName VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT act_id, gm, cha_ids, password, last_ip, disc_reason, 
           CONVERT(VARCHAR(20), last_leave, 120) AS last_leave
    FROM account 
    WHERE act_name = @actName;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: GetCharacterByChaName
-- Description: Safely fetches character by name (prevents SQL injection)
-- Parameters:
--   @chaName - Character name to look up
-- Returns: cha_id, motto, icon
-- =============================================
IF OBJECT_ID('dbo.GetCharacterByChaName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetCharacterByChaName
GO

CREATE PROCEDURE [dbo].[GetCharacterByChaName]
    @chaName VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT cha_id, motto, icon
    FROM character 
    WHERE cha_name = @chaName;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: GetAccountIdByChaName
-- Description: Safely fetches account ID by character name (prevents SQL injection)
-- Parameters:
--   @chaName - Character name to look up
-- Returns: act_id
-- =============================================
IF OBJECT_ID('dbo.GetAccountIdByChaName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetAccountIdByChaName
GO

CREATE PROCEDURE [dbo].[GetAccountIdByChaName]
    @chaName VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT act_id
    FROM character 
    WHERE cha_name = @chaName;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: GetGuildByName
-- Description: Safely fetches guild by name (prevents SQL injection)
-- Parameters:
--   @guildName - Guild name to look up
-- Returns: guild_id
-- =============================================
IF OBJECT_ID('dbo.GetGuildByName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetGuildByName
GO

CREATE PROCEDURE [dbo].[GetGuildByName]
    @guildName VARCHAR(16)
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT guild_id
    FROM guild 
    WHERE guild_name = @guildName;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: GetChaIdByChaName
-- Description: Safely fetches character ID by name (prevents SQL injection)
-- Parameters:
--   @chaName - Character name to look up
-- Returns: cha_id
-- =============================================
IF OBJECT_ID('dbo.GetChaIdByChaName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetChaIdByChaName
GO

CREATE PROCEDURE [dbo].[GetChaIdByChaName]
    @chaName VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT cha_id
    FROM character 
    WHERE cha_name = @chaName;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: GetActIdByChaName
-- Description: Safely fetches account ID by character name (prevents SQL injection)
-- Parameters:
--   @chaName - Character name to look up
-- Returns: act_id
-- =============================================
IF OBJECT_ID('dbo.GetActIdByChaName', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetActIdByChaName
GO

CREATE PROCEDURE [dbo].[GetActIdByChaName]
    @chaName VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT act_id
    FROM character 
    WHERE cha_name = @chaName;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: CheckTicketExists
-- Description: Safely checks if ticket exists (prevents SQL injection)
-- Parameters:
--   @issue - Issue number
--   @itemno - Item number
-- Returns: count of matching tickets
-- =============================================
IF OBJECT_ID('dbo.CheckTicketExists', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CheckTicketExists
GO

CREATE PROCEDURE [dbo].[CheckTicketExists]
    @issue INT,
    @itemno VARCHAR(50)
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT COUNT(*) AS [TicketCount]
    FROM ticket 
    WHERE issue = @issue AND itemno = @itemno;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- PHASE 2: Additional SQL Injection Prevention Procedures
-- Added: January 2026
-- =============================================

-- =============================================
-- Procedure: MapMaskSaveData
-- Description: Saves map mask data for a player
-- Used by: CTableMapMask::SaveData()
-- Parameters:
--   @colIndex - Column index (1=garner, 2=magicsea, 3=darkblue, 4=winterland)
--   @maskData - Base64 encoded mask data
--   @dbId - Database ID of the map mask record
-- =============================================
IF OBJECT_ID('dbo.MapMaskSaveData', 'P') IS NOT NULL
    DROP PROCEDURE dbo.MapMaskSaveData
GO

CREATE PROCEDURE [dbo].[MapMaskSaveData]
    @colIndex INT,
    @maskData VARCHAR(MAX),
    @dbId INT
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @sql NVARCHAR(MAX);
    DECLARE @colName VARCHAR(20);
    
    SET @colName = CASE @colIndex
        WHEN 1 THEN 'content1'
        WHEN 2 THEN 'content2'
        WHEN 3 THEN 'content3'
        WHEN 4 THEN 'content4'
        ELSE NULL
    END;
    
    IF @colName IS NULL
    BEGIN
        RAISERROR('Invalid column index', 16, 1);
        RETURN -1;
    END
    
    SET @sql = N'UPDATE map_mask SET ' + QUOTENAME(@colName) + N' = @maskDataParam WHERE id = @dbIdParam';
    
    EXEC sp_executesql @sql, 
        N'@maskDataParam VARCHAR(MAX), @dbIdParam INT',
        @maskDataParam = @maskData,
        @dbIdParam = @dbId;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: OfflineStall_Create
-- Description: Creates a new offline stall
-- Used by: CGameDB::CreateOfflineStall()
-- =============================================
IF OBJECT_ID('dbo.OfflineStall_Create', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_Create
GO

CREATE PROCEDURE [dbo].[OfflineStall_Create]
    @cha_id INT,
    @cha_name VARCHAR(50),
    @act_id INT,
    @stall_title NVARCHAR(100),
    @look_face SMALLINT,
    @look_hair SMALLINT,
    @job SMALLINT,
    @map_name VARCHAR(50),
    @pos_x INT,
    @pos_y INT,
    @duration_hours INT,
    @item_count TINYINT,
    @item_data VARBINARY(MAX),
    @kitbag_snapshot VARBINARY(MAX),
    @equip_look_data VARBINARY(MAX) = NULL,
    @stall_level TINYINT = 1
AS
BEGIN
    SET NOCOUNT ON;
    
    DECLARE @expire_time DATETIME = DATEADD(HOUR, @duration_hours, GETDATE());
    DECLARE @existing_id INT;
    
    -- Check if character already has a stall (IF EXISTS prevents UNIQUE constraint violation)
    SELECT @existing_id = stall_id FROM offline_stalls 
    WHERE cha_id = @cha_id;
    
    IF @existing_id IS NOT NULL
    BEGIN
        -- Update existing stall
        UPDATE offline_stalls SET
            stall_title = @stall_title,
            look_face = @look_face,
            look_hair = @look_hair,
            job = @job,
            stall_level = @stall_level,
            map_name = @map_name,
            pos_x = @pos_x,
            pos_y = @pos_y,
            expire_time = @expire_time,
            item_count = @item_count,
            item_data = @item_data,
            kitbag_snapshot = @kitbag_snapshot,
            equip_look_data = @equip_look_data,
            is_active = 1,
            pending_gold = 0,
            created_time = GETDATE()
        WHERE stall_id = @existing_id;
        
        SELECT @existing_id AS stall_id;
        RETURN @existing_id;
    END
    ELSE
    BEGIN
        -- Insert new stall
        INSERT INTO offline_stalls (
            cha_id, cha_name, act_id, stall_title,
            look_face, look_hair, job, map_name,
            pos_x, pos_y, expire_time, item_count,
            item_data, kitbag_snapshot, equip_look_data, stall_level, created_time, is_active
        ) VALUES (
            @cha_id, @cha_name, @act_id, @stall_title,
            @look_face, @look_hair, @job, @map_name,
            @pos_x, @pos_y, @expire_time, @item_count,
            @item_data, @kitbag_snapshot, @equip_look_data, @stall_level, GETDATE(), 1
        );
        
        DECLARE @new_id INT = SCOPE_IDENTITY();
        SELECT @new_id AS stall_id;
        RETURN @new_id;
    END
END
GO

-- =============================================
-- Procedure: OfflineStall_UpdateItems
-- Description: Updates item data for an offline stall
-- Used by: CGameDB::UpdateOfflineStallItems()
-- =============================================
IF OBJECT_ID('dbo.OfflineStall_UpdateItems', 'P') IS NOT NULL
    DROP PROCEDURE dbo.OfflineStall_UpdateItems
GO

CREATE PROCEDURE [dbo].[OfflineStall_UpdateItems]
    @stall_id INT,
    @item_count TINYINT,
    @item_data VARBINARY(MAX),
    @gold_earned BIGINT
AS
BEGIN
    SET NOCOUNT ON;
    
    UPDATE offline_stalls
    SET item_count = @item_count,
        item_data = @item_data,
        pending_gold = ISNULL(pending_gold, 0) + @gold_earned
    WHERE stall_id = @stall_id AND is_active = 1;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: InitAllGuilds
-- Description: Retrieves all active guilds for server initialization
-- Used by: TBLGuilds::InitAllGuilds()
-- =============================================
IF OBJECT_ID('dbo.InitAllGuilds', 'P') IS NOT NULL
    DROP PROCEDURE dbo.InitAllGuilds
GO

CREATE PROCEDURE [dbo].[InitAllGuilds]
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT 
        g.guild_id, g.guild_name, g.motto, g.leader_id,
        g.exp, g.member_total, g.try_total, g.disband_date, g.level
    FROM guild AS g
    WHERE g.guild_id > 0;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: GetGuildMembers
-- Description: Retrieves all members of a specific guild
-- Used by: TBLGuilds::InitGuildMember()
-- =============================================
IF OBJECT_ID('dbo.GetGuildMembers', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetGuildMembers
GO

CREATE PROCEDURE [dbo].[GetGuildMembers]
    @guild_id INT
AS
BEGIN
    SET NOCOUNT ON;
    
    SELECT c.mem_addr, c.cha_id, c.cha_name, c.motto, 
           c.job, c.degree, c.icon, c.guild_permission
    FROM character AS c
    WHERE c.guild_stat = 0 AND c.guild_id = @guild_id AND c.delflag = 0;
    
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: GetAmphitheaterCaptainByMap
-- Description: Gets captains for a specific amphitheater map
-- =============================================
IF OBJECT_ID('dbo.GetAmphitheaterCaptainByMap', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetAmphitheaterCaptainByMap
GO

CREATE PROCEDURE [dbo].[GetAmphitheaterCaptainByMap]
    @map_id INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT captain FROM AmphitheaterTeam WHERE map = @map_id;
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: GetAmphitheaterPromotionTeams
-- Description: Gets promotion leaderboard for amphitheater
-- =============================================
IF OBJECT_ID('dbo.GetAmphitheaterPromotionTeams', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetAmphitheaterPromotionTeams
GO

CREATE PROCEDURE [dbo].[GetAmphitheaterPromotionTeams]
    @state INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT B.cha_name, A.id, A.winnum 
    FROM AmphitheaterTeam A
    INNER JOIN character B ON B.cha_id = A.captain 
    WHERE A.state = @state ORDER BY A.winnum DESC;
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: GetAmphitheaterReliveTeams
-- Description: Gets relive leaderboard for amphitheater
-- =============================================
IF OBJECT_ID('dbo.GetAmphitheaterReliveTeams', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetAmphitheaterReliveTeams
GO

CREATE PROCEDURE [dbo].[GetAmphitheaterReliveTeams]
    @state INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT B.cha_name, A.relivenum, A.id 
    FROM AmphitheaterTeam A
    INNER JOIN character B ON B.cha_id = A.captain 
    WHERE A.state = @state ORDER BY A.relivenum DESC;
    RETURN @@ROWCOUNT;
END
GO

-- =============================================
-- Procedure: InsertGameLog
-- Description: Inserts a game log entry
-- =============================================
IF OBJECT_ID('dbo.InsertGameLog', 'P') IS NOT NULL
    DROP PROCEDURE dbo.InsertGameLog
GO

CREATE PROCEDURE [dbo].[InsertGameLog]
    @action VARCHAR(50),
    @c1 VARCHAR(100),
    @c2 VARCHAR(100),
    @c3 VARCHAR(100),
    @c4 VARCHAR(100),
    @content VARCHAR(8000)
AS
BEGIN
    SET NOCOUNT ON;
    INSERT INTO gamelog (action, c1, c2, c3, c4, content, log_time)
    VALUES (@action, @c1, @c2, @c3, @c4, @content, GETDATE());
    RETURN @@ROWCOUNT;
END
GO

PRINT ''
PRINT '=== Phase 2 SQL Injection Prevention Procedures ==='
PRINT 'MapMaskSaveData, OfflineStall_Create, OfflineStall_UpdateItems'
PRINT 'InitAllGuilds, GetGuildMembers, GetAmphitheaterCaptainByMap'
PRINT 'GetAmphitheaterPromotionTeams, GetAmphitheaterReliveTeams, InsertGameLog'
GO

-- ============================================
-- Get Player Login IP and MAC from AccountServer
-- ============================================
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

PRINT 'Created stored procedure: GetPlayerLoginInfo'
GO

-- ============================================================
-- Phase 2.5: Parameterized stored procedures (replaces raw SQL)
-- ============================================================

IF OBJECT_ID('dbo.GetMasterDBID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.GetMasterDBID
GO

CREATE PROCEDURE [dbo].[GetMasterDBID]
    @cha_id1 INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT cha_id2 FROM master (NOLOCK) WHERE cha_id1 = @cha_id1;
END
GO

PRINT 'Created stored procedure: GetMasterDBID'
GO

IF OBJECT_ID('dbo.IsMasterRelation', 'P') IS NOT NULL
    DROP PROCEDURE dbo.IsMasterRelation
GO

CREATE PROCEDURE [dbo].[IsMasterRelation]
    @cha_id1 INT,
    @cha_id2 INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT count(*) FROM master (NOLOCK) WHERE (cha_id1 = @cha_id1) AND (cha_id2 = @cha_id2);
END
GO

PRINT 'Created stored procedure: IsMasterRelation'
GO

IF OBJECT_ID('dbo.CalWinTicket', 'P') IS NOT NULL
    DROP PROCEDURE dbo.CalWinTicket
GO

CREATE PROCEDURE [dbo].[CalWinTicket]
    @issue INT,
    @max   INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT TOP 10 itemno, num FROM (
        SELECT itemno, COUNT(*) AS num
        FROM Ticket
        WHERE (issue = @issue) AND real = 0
        GROUP BY itemno
    ) AS A
    WHERE num <= @max
    ORDER BY num;
END
GO

PRINT 'Created stored procedure: CalWinTicket'
GO

IF OBJECT_ID('dbo.ReadKitbagTmpDataByID', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ReadKitbagTmpDataByID
GO

CREATE PROCEDURE [dbo].[ReadKitbagTmpDataByID]
    @id INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT content FROM resource WHERE id = @id;
END
GO

PRINT 'Created stored procedure: ReadKitbagTmpDataByID'
GO

IF OBJECT_ID('dbo.ShowExpRank', 'P') IS NOT NULL
    DROP PROCEDURE dbo.ShowExpRank
GO

CREATE PROCEDURE [dbo].[ShowExpRank]
    @count INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT TOP (@count) cha_name, job, degree
    FROM character
    WHERE delflag = 0
    ORDER BY CASE WHEN (exp < 0) THEN (exp + 4294967296) ELSE exp END DESC;
END
GO

PRINT 'Created stored procedure: ShowExpRank'
GO

IF OBJECT_ID('dbo.IsAmphitheaterTeamLogin', 'P') IS NOT NULL
    DROP PROCEDURE dbo.IsAmphitheaterTeamLogin
GO

CREATE PROCEDURE [dbo].[IsAmphitheaterTeamLogin]
    @actor_id INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT count(*) FROM AmphitheaterTeam (NOLOCK)
    WHERE captain = @actor_id
       OR member LIKE CAST(@actor_id AS VARCHAR(20)) + ',%'
       OR member LIKE '%,' + CAST(@actor_id AS VARCHAR(20));
END
GO

PRINT 'Created stored procedure: IsAmphitheaterTeamLogin'
GO


USE [master]
GO
ALTER DATABASE [GameDB]
SET
READ_WRITE
GO
