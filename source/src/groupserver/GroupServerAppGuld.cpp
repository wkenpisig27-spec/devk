#include "stdafx.h"
#include <iostream>
#include "GroupServerApp.h"
#include "GameCommon.h"
#include "log.h"

void GroupServerApp::PC_GULD_INIT(Player* ply) {
	if (ply->m_guild[ply->m_currcha] > 0) {
		Guild* l_guild = FindGuildByGldID(ply->m_guild[ply->m_currcha]);
		if (l_guild) {
			ply->JoinGuild(l_guild);
			if (l_guild->m_leaderID == ply->m_chaid[ply->m_currcha]) {
				l_guild->m_leader = ply;
			}
		} else {
			LG("Guild", "player [%s] can't get guild struct ,guild ID:%d\n", ply->m_chaname[ply->m_currcha].c_str(), ply->m_guild[ply->m_currcha]);
		}
	}
	{
		auto db = GetDB();
		if (!db) return;
		db->tblguilds->InitGuildMember(ply, ply->m_chaid[ply->m_currcha], ply->m_guild[ply->m_currcha], 0);
	}
}
void GroupServerApp::MP_GUILD_CREATE(Player* ply, DataSocket* datasock, RPacket& pk) {
	ply->m_guildPermission[ply->m_currcha] = emGldPermMax;
	ply->m_guild[ply->m_currcha] = pk.ReadLong();
	Guild* l_gld = FindGuildByGldID(ply->m_guild[ply->m_currcha]);
	l_gld->m_id = ply->m_guild[ply->m_currcha];		  // ����ID
	strncpy_s(l_gld->m_name, sizeof(l_gld->m_name), pk.ReadString(), _TRUNCATE);			  // ������
	l_gld->m_motto[0] = 0;						  // ����������
	l_gld->m_leaderID = ply->m_chaid[ply->m_currcha]; // �᳤ID
	l_gld->m_stat = 0;								  // ����״̬
	l_gld->m_remain_minute = 0;						  // �����ɢʣ�������
	l_gld->m_tick = GetTickCount();

	ply->JoinGuild(l_gld);
	WPacket l_wpk = g_gpsvr->GetWPacket();
	l_wpk.WriteCmd(CMD_PC_GUILD);
	l_wpk.WriteChar(MSG_GUILD_START);
	l_wpk.WriteLong(ply->m_guild[ply->m_currcha]); // ����ID
	l_wpk.WriteString(ply->GetGuild()->m_name);	   // ����name
	l_wpk.WriteLong(ply->GetGuild()->m_leaderID);  // �᳤ID

	l_wpk.WriteChar(1);										   // online
	l_wpk.WriteLong(ply->m_chaid[ply->m_currcha]);			   // chaid
	l_wpk.WriteString(ply->m_chaname[ply->m_currcha].c_str()); // chaname
	l_wpk.WriteString(ply->m_motto[ply->m_currcha].c_str());   // motto
	l_wpk.WriteString(pk.ReadString());						   // job
	l_wpk.WriteShort(pk.ReadShort());						   // degree
	l_wpk.WriteShort(ply->m_icon[ply->m_currcha]);			   // icon
	l_wpk.WriteLong(emGldPermMax);							   // permission

	l_wpk.WriteLong(0);
	l_wpk.WriteChar(1);
	g_gpsvr->SendToClient(ply, l_wpk);
}
void GroupServerApp::MP_GUILD_APPROVE(Player* ply, DataSocket* datasock, RPacket& pk) {
	uLong l_chaid = pk.ReadLong();
	Player* l_ply = FindPlayerByChaID(l_chaid);
	if (!ply->GetGuild()) {
		LG("Guild", "GroupServer guild data exception, please contact developer...\n");
		return;
	}
	if (l_ply) {
		l_ply->m_guild[l_ply->m_currcha] = ply->GetGuild()->m_id;
		l_ply->m_guildPermission[l_ply->m_currcha] = emGldPermDefault;
		l_ply->JoinGuild(ply->GetGuild());
	}
	{
		auto db = GetDB();
		if (!db) return;
		db->tblguilds->InitGuildMember(l_ply, l_chaid, ply->GetGuild()->m_id, 1);
	}
}
void GroupServerApp::MP_GUILD_KICK(Player* ply, DataSocket* datasock, RPacket& pk) {
	uLong l_chaid = pk.ReadLong();
	Guild* l_guild = ply->GetGuild();
	if (!l_guild) {
		LG("Guild", "GroupServer guild data exception, please contact developer...\n");
		return;
	}
	Player* l_ply = l_guild->FindGuildMemByChaID(l_chaid);

	if (l_ply && l_ply->m_currcha >= 0) {
		l_ply->m_guild[l_ply->m_currcha] = 0;
		ply->m_guildPermission[ply->m_currcha] = 0;
		l_ply->LeaveGuild();

		WPacket l_wpk = GetWPacket();
		l_wpk.WriteCmd(CMD_PC_GUILD);
		l_wpk.WriteChar(MSG_GUILD_STOP);
		SendToClient(l_ply, l_wpk);
	}
	Player* l_plylst[10240];
	short l_plynum = 0;

	WPacket l_wpk = GetWPacket();
	l_wpk.WriteCmd(CMD_PC_GUILD);
	l_wpk.WriteChar(MSG_GUILD_DEL);
	l_wpk.WriteLong(l_chaid);
	RunChainGetArmor<GuildMember> l(*l_guild);
	while (l_ply = static_cast<Player*>(l_guild->GetNextItem())) {
		l_plylst[l_plynum] = l_ply;
		l_plynum++;
	}
	l.unlock();

	SendToClient(l_plylst, l_plynum, l_wpk);
}
void GroupServerApp::MP_GUILD_LEAVE(Player* ply, DataSocket* datasock, RPacket& pk) {
	uLong l_chaid = ply->m_chaid[ply->m_currcha];
	Guild* l_guild = ply->GetGuild();
	if (!l_guild) {
		LG("Guild", "GroupServer guild data exception, please contact developer...\n");
		return;
	}
	{
		ply->m_guildPermission[ply->m_currcha] = 0;
		ply->m_guild[ply->m_currcha] = 0;
		ply->LeaveGuild();

		WPacket l_wpk = GetWPacket();
		l_wpk.WriteCmd(CMD_PC_GUILD);
		l_wpk.WriteChar(MSG_GUILD_STOP);
		SendToClient(ply, l_wpk);
	}
	Player* l_plylst[10240];
	short l_plynum = 0;

	WPacket l_wpk = GetWPacket();
	l_wpk.WriteCmd(CMD_PC_GUILD);
	l_wpk.WriteChar(MSG_GUILD_DEL);
	l_wpk.WriteLong(l_chaid);
	RunChainGetArmor<GuildMember> l(*l_guild);
	while (ply = static_cast<Player*>(l_guild->GetNextItem())) {
		l_plylst[l_plynum] = ply;
		l_plynum++;
	}
	l.unlock();

	SendToClient(l_plylst, l_plynum, l_wpk);
}
void GroupServerApp::MP_GUILD_DISBAND(Player* ply, DataSocket* datasock, RPacket& pk) {
	Guild* l_guild = ply->GetGuild();
	if (!l_guild) {
		LG("Guild", "GroupServer guild data exception, please contact developer...\n");
		return;
	}
	l_guild->m_leader = nullptr;
	l_guild->m_leaderID = 0;

	Player* l_plylst[10240];
	short l_plynum = 0;

	WPacket l_wpk = GetWPacket();
	l_wpk.WriteCmd(CMD_PC_GUILD);
	l_wpk.WriteChar(MSG_GUILD_STOP);
	RunChainGetArmor<GuildMember> l(*l_guild);
	while (ply = static_cast<Player*>(l_guild->GetFirstItem())) {
		ply->m_guildPermission[ply->m_currcha] = 0;
		ply->m_guild[ply->m_currcha] = 0;
		ply->LeaveGuild();

		l_plylst[l_plynum] = ply;
		l_plynum++;
	}
	l.unlock();

	SendToClient(l_plylst, l_plynum, l_wpk);
}
void GroupServerApp::MP_GUILD_MOTTO(Player* ply, DataSocket* datasock, RPacket& pk) {
	Guild* l_guild = ply->GetGuild();
	if (!l_guild) {
		LG("Guild", "GroupServer guild data exception, please contact developer...\n");
		return;
	}
	strncpy_s(l_guild->m_motto, sizeof(l_guild->m_motto), pk.ReadString(), _TRUNCATE);
}

void GroupServerApp::MP_GUILD_CHALLMONEY(Player* ply, DataSocket* datasock, RPacket& pk) {
	DWORD dwChallID = pk.ReadLong();
	long long dwMoney = pk.ReadLongLong();
	Guild* pGuild = FindGuildByGldID(dwChallID);
	if (!pGuild || pGuild->m_leaderID == 0) {
		LG("Guild", "GroupServer guild data exception, find guild nothing, or guild has no leader! withdrawal challenging money! guildid = %lumoney = %lld\n", dwChallID, dwMoney);
		return;
	}

	const char* pszGuild1 = pk.ReadString();
	const char* pszGuild2 = pk.ReadString();

	Player* l_ply = pGuild->m_leader;
	if (!l_ply || l_ply->m_currcha == -1 || pGuild->m_leaderID != l_ply->m_chaid[l_ply->m_currcha]) {
		LG("Guild", "player is offline, withdrawal challenging [%s] money!chaid = %lumoney = %lld\n", pszGuild1, pGuild->m_leaderID, dwMoney);

		auto db = g_gpsvr->GetDB();
		if (db && !db->tblcharaters->AddMoney(pGuild->m_leaderID, dwMoney)) {
			LG("Guild", "challenge guild, withdrawal challenging [%s] money failed!chaid = %lumoney = %lld\n", pszGuild1, pGuild->m_leaderID, dwMoney);
		}
	} else {
		LG("Guild", "online guild, withdrawal challenging [%s] money!chaid = %lumoney = %lld\n", pszGuild1, pGuild->m_leaderID, dwMoney);

		WPacket l_wpk = GetWPacket();
		l_wpk.WriteCmd(CMD_PM_GUILD_CHALLMONEY);
		l_wpk.WriteLong(pGuild->m_leaderID);
		l_wpk.WriteLongLong(dwMoney);
		l_wpk.WriteString(pszGuild1);
		l_wpk.WriteString(pszGuild2);
		l_wpk.WriteShort(0);
		SendToClient(l_ply, l_wpk);
	}
}

void GroupServerApp::MP_GUILD_CHALL_PRIZEMONEY(Player* ply, DataSocket* datasock, RPacket& pk) {
	DWORD dwChallID = pk.ReadLong();
	long long dwMoney = pk.ReadLongLong();
	Guild* pGuild = FindGuildByGldID(dwChallID);
	if (!pGuild || pGuild->m_leaderID == 0) {
		LG("Guild", "GroupServer guild data exception, can't find leader, or has no leader! withdrawal challenging money failed!guildid = %lumoney = %lld\n", dwChallID, dwMoney);
		return;
	}

	Player* l_ply = pGuild->m_leader;
	if (!l_ply || l_ply->m_currcha == -1 || pGuild->m_leaderID != l_ply->m_chaid[l_ply->m_currcha]) {
		LG("Guild", "player is offline, withdrawal challenging guild [%s] money! chaid = %lumoney = %lld\n", pGuild->m_name, pGuild->m_leaderID, dwMoney);

		auto db = g_gpsvr->GetDB();
		if (db && !db->tblcharaters->AddMoney(pGuild->m_leaderID, dwMoney)) {
			LG("Guild", "challenging guild, withdrawal challenging guild [%s] money failed! chaid = %lumoney = %lld\n", pGuild->m_name, pGuild->m_leaderID, dwMoney);
		}
	} else {
		LG("Guild", "online challenging guild, withdrawal challenging guild [%s] moeny!chaid = %lumoney = %lld\n", pGuild->m_name, pGuild->m_leaderID, dwMoney);

		// ������֪ͨ���ڷ�����
		WPacket l_wpk = GetWPacket();
		l_wpk.WriteCmd(CMD_PM_GUILD_CHALL_PRIZEMONEY);
		l_wpk.WriteLong(pGuild->m_leaderID);
		l_wpk.WriteLongLong(dwMoney);
		SendToClient(l_ply, l_wpk);
	}
}
