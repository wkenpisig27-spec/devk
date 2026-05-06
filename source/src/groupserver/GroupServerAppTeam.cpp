#include "stdafx.h"
#include <iostream>
#include <conformity.h>
#include "GroupServerApp.h"
#include "log.h"
#include "GameCommon.h"

#include "Parser.h"

void GroupServerApp::CP_TEAM_INVITE(Player* ply, DataSocket* datasock, RPacket& pk) {
	//// Add by lark.li 20080715 begin
	// if(!CheckFunction("garner", "Team_Invite"))
	//{
	//	ply->SendSysInfo("You can't invite!");
	//	return;
	// }
	//// End

	if (ply->GetTeam() && ply->GetLeader() != ply) {
		// ply->SendSysInfo("只有队长才能发出组队邀请。");
		ply->SendSysInfo(RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00001));
	} else if (ply->GetTeam() && ply->GetTeam()->GetTotal() >= const_team.MemberMax) {
		// ply->SendSysInfo("你的队伍成员数已经达到了系统允许的上限了。");
		ply->SendSysInfo(RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00002));
	} else {
		Invited* l_invited = 0;
		uShort l_len;
		cChar* l_invited_name = pk.ReadString(&l_len);
		if (!l_invited_name || l_len > common::conformity::character::name::max_length) {
			return;
		}
		Player* l_invited_ply = FindPlayerByChaName(l_invited_name);
		if (!l_invited_ply || l_invited_ply->m_currcha < 0 || l_invited_ply == ply) {
			char l_buf[256];
			// sprintf(l_buf,"你所邀请的玩家【%s】当前不在线上。",l_invited_name);
			sprintf(l_buf, RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00003), l_invited_name);
			ply->SendSysInfo(l_buf);
		} else if (l_invited_ply->GetTeam()) {
			char l_buf[256];
			// sprintf(l_buf,"你所邀请的玩家【%s】当前处于组队状态。",l_invited_name);
			sprintf(l_buf, RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00004), l_invited_name);
			ply->SendSysInfo(l_buf);
		} else if (l_invited = l_invited_ply->TeamFindInvitedByInviterChaID(ply->m_chaid[ply->m_currcha])) {
			// ply->SendSysInfo(dstring("您当前对【")<<l_invited_name<<"】已经有一个未决的组队邀请，请稍安毋躁。");
			char l_buf[256];
			sprintf(l_buf, RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00005), l_invited_name);
			ply->SendSysInfo(l_buf);

		} else if (!l_invited_ply->CanReceiveRequests()) {
			char l_buf[256];
			sprintf(l_buf, "%s is currently offline. Unable to send request!", l_invited_name);
			ply->SendSysInfo(l_buf);
		} else {
			PtInviter l_ptinviter = l_invited_ply->TeamBeginInvited(ply);
			if (l_ptinviter) {
				char l_buf[256];
				// sprintf(l_buf,"你所邀请的组队玩家【%s】由于被邀请过多而处于繁忙状态,你对其的邀请已被系统取消。",l_invited_name);
				sprintf(l_buf, RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00007), l_invited_name);
				l_ptinviter->SendSysInfo(l_buf);

				WPacket l_wpk = GetWPacket();
				l_wpk.WriteCmd(CMD_PC_TEAM_CANCEL);
				l_wpk.WriteChar(MSG_TEAM_CANCLE_BUSY);
				l_wpk.WriteLong(l_ptinviter.m_chaid);
				SendToClient(l_invited_ply, l_wpk);
			}
			WPacket l_wpk = GetWPacket();
			l_wpk.WriteCmd(CMD_PC_TEAM_INVITE);
			l_wpk.WriteString(ply->m_chaname[ply->m_currcha].c_str());
			l_wpk.WriteLong(ply->m_chaid[ply->m_currcha]);
			l_wpk.WriteShort(ply->m_icon[ply->m_currcha]);
			SendToClient(l_invited_ply, l_wpk);
		}
	}
}
void GroupServerApp::CP_TEAM_REFUSE(Player* ply, DataSocket* datasock, RPacket& pk) {
	uLong l_inviter_chaid = pk.ReadLong();
	PtInviter l_inviter = ply->TeamEndInvited(l_inviter_chaid);
	if (l_inviter && l_inviter->m_currcha >= 0 && l_inviter.m_chaid == l_inviter->m_chaid[l_inviter->m_currcha]) {
		char l_buf[256];
		// sprintf(l_buf,"玩家【%s】拒绝了您的组队邀请。",ply->m_chaname[ply->m_currcha].c_str());
		sprintf(l_buf, RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00008), ply->m_chaname[ply->m_currcha].c_str());
		l_inviter->SendSysInfo(l_buf);
	}
}
void GroupServerApp::MP_TEAM_CREATE(Player* ply, DataSocket* datasock, RPacket& pk) {
	bool bInvited = false;
	cChar* szName1 = pk.ReadString(); // 队员
	cChar* szName2 = pk.ReadString(); // 队长

	Player* pPly = FindPlayerByChaName(szName2);
	Player* pPly2 = FindPlayerByChaName(szName1);

	if (!pPly || !pPly2) {
		// l_line<<newln<<"MP_TEAM_CREATE()成员不在线!";
		LG("Master", "MP_TEAM_CREATE() member is offline!\n");
		return;
	}

	// 邀请
	if (pPly->GetTeam() && pPly->GetLeader() != pPly) {
		// pPly->SendSysInfo("您不是队长!");
		pPly->SendSysInfo(RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00010));
		// pPly2->SendSysInfo("对方不是队长!");
		pPly2->SendSysInfo(RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00011));
	} else if (pPly->GetTeam() && pPly->GetTeam()->GetTotal() >= const_team.MemberMax) {
		// pPly->SendSysInfo("你的队伍成员数已经达到了系统允许的上限了!");
		pPly->SendSysInfo(RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00002));
		pPly2->SendSysInfo(RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00012));
	} else {
		Invited* l_invited = 0;
		uShort l_len = (uShort)strlen(szName1);
		cChar* l_invited_name = szName1;
		if (!l_invited_name || l_len > common::conformity::character::name::max_length) {
			// l_line<<newln<<"MP_TEAM_CREATE()姓名长度非法!";
			LG("Master", "MP_TEAM_CREATE() name length is invalid!\n");
			return;
		}
		Player* l_invited_ply = pPly2;
		if (!l_invited_ply || l_invited_ply->m_currcha < 0 || l_invited_ply == pPly) {
			char l_buf[256];
			// sprintf(l_buf,"玩家【%s】当前不在线上!",l_invited_name);
			sprintf(l_buf, RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00014), l_invited_name);
			pPly->SendSysInfo(l_buf);
		} else if (l_invited_ply->GetTeam()) {
			char l_buf[256];
			// sprintf(l_buf,"玩家【%s】当前处于组队状态。",l_invited_name);
			sprintf(l_buf, RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00015), l_invited_name);
			pPly->SendSysInfo(l_buf);
		} else if (l_invited = l_invited_ply->TeamFindInvitedByInviterChaID(pPly->m_chaid[pPly->m_currcha])) {
			// pPly->SendSysInfo(dstring("您当前对【")<<l_invited_name<<"】已经有一个未决的组队邀请，请稍安毋躁。");
			char l_buf[256];
			sprintf(l_buf, RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00005), l_invited_name);
			pPly->SendSysInfo(l_buf);
		} else {
			PtInviter l_ptinviter = l_invited_ply->TeamBeginInvited(pPly);
			if (l_ptinviter) {
				char l_buf[256];
				// sprintf(l_buf,"玩家【%s】处于繁忙状态,你对其的邀请已被系统取消。",l_invited_name);
				sprintf(l_buf, RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00016), l_invited_name);
				l_ptinviter->SendSysInfo(l_buf);
			}

			bInvited = true;
		}
	}

	// 接受邀请
	if (bInvited) {
		cChar* szInviterName = szName2;
		int l_count = pPly2->JoinTeam(szInviterName);
		if (l_count && (l_count > const_team.MemberMax)) {
			pPly2->LeaveTeam();
			// pPly2->SendSysInfo("你要加入的组队成员数已达到允许的上限。");
			pPly2->SendSysInfo(RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00017));
		} else if (l_count) {
			// l_line<<newln<<"玩家"<<pPly2->m_chaname[pPly2->m_currcha]<<"加入了组队"<<pPly2->GetLeader()->m_chaname[pPly2->GetLeader()->m_currcha]<<endln;
			LG("Master", "player %s add team %s\n", pPly2->m_chaname[pPly2->m_currcha].c_str(), pPly2->GetLeader()->m_chaname[pPly2->GetLeader()->m_currcha].c_str());
			// 通知Client组成员变化
			{
				Team* l_team = pPly2->GetTeam();

				WPacket l_wpk = GetWPacket();
				l_wpk.WriteCmd(CMD_PC_TEAM_REFRESH);
				l_wpk.WriteChar(TEAM_MSG_ADD);
				l_wpk.WriteChar(uChar(l_count));

				Player* l_plylst[10240];
				short l_plynum = 0;

				Player* l_ply1;
				char l_currcha;
				RunChainGetArmor<TeamMember> l(*l_team);
				for (int i = 0; i < l_count && (l_ply1 = static_cast<Player*>(l_team->GetNextItem())); i++) {
					if ((l_currcha = l_ply1->m_currcha) >= 0) {
						l_wpk.WriteLong(l_ply1->m_chaid[l_currcha]);
						l_wpk.WriteString(l_ply1->m_chaname[l_currcha].c_str());
						l_wpk.WriteString(l_ply1->m_motto[l_currcha].c_str());
						l_wpk.WriteShort(l_ply1->m_icon[l_currcha]);

						l_plylst[l_plynum] = l_ply1;
						l_plynum++;
					}
				}
				l.unlock();

				SendToClient(l_plylst, l_plynum, l_wpk);
			}
			// 通知GameServer组成员变化
			{
				Team* l_team = pPly2->GetTeam();
				Player* l_plyr;
				GateServer* l_gate[30];
				for (int i = 0; i < 30; i++)
					l_gate[i] = 0;
				char l_gtnum = 0;

				WPacket l_wpk = GetWPacket();
				l_wpk.WriteCmd(CMD_PM_TEAM);
				l_wpk.WriteChar(TEAM_MSG_ADD);
				l_wpk.WriteChar(uChar(l_count));
				RunChainGetArmor<TeamMember> l(*l_team);
				for (int i = 0; i < l_count && (l_plyr = static_cast<Player*>(l_team->GetNextItem())); i++) {
					l_wpk.WriteString(l_plyr->m_gate->m_name.c_str());
					l_wpk.WriteLongLong(l_plyr->m_gtAddr);
					l_wpk.WriteLong(l_plyr->m_chaid[l_plyr->m_currcha]);
					for (int j = 0; j < 30; j++) {
						if (l_gate[j] == l_plyr->m_gate) {
							break;
						}
						if (!l_gate[j]) {
							l_gate[j] = l_plyr->m_gate;
							l_gtnum++;
							break;
						}
					}
				}
				l.unlock();
#if 1
				l_gtnum = l_gtnum ? 1 : 0;
#endif
				for (int j = 0; j < l_gtnum; j++) {
					if (auto* sock = l_gate[j]->GetDataSock(); sock) {
						sock->SendData(l_wpk);
					}
					// l_line<<newln<<"MP_TEAM_CREATE()确认ToGameServer的加入组队操作发送了数据到GateServer";
					LG("Master", "MP_TEAM_CREATE() send ToGameServer data to GateServer\n");
				}
			}
		}
	} else {
		// l_line<<newln<<"MP_TEAM_CREATE()邀请失败";
		LG("Master", "MP_TEAM_CREATE() invite failed\n");
	}
}
void GroupServerApp::CP_TEAM_ACCEPT(Player* ply, DataSocket* datasock, RPacket& pk) {
	uLong l_inviter_chaid = pk.ReadLong();
	int l_count = ply->JoinTeam(l_inviter_chaid);
	if (l_count && (l_count > const_team.MemberMax)) {
		ply->LeaveTeam();
		// ply->SendSysInfo("你要加入的组队成员数已达到允许的上限。");
		ply->SendSysInfo(RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00017));
	} else if (l_count) {
		// l_line<<newln<<"玩家"<<ply->m_chaname[ply->m_currcha]<<"加入了组队"<<ply->GetLeader()->m_chaname[ply->GetLeader()->m_currcha]<<endln;
		LG("Team", "player %s add team %s\n", ply->m_chaname[ply->m_currcha].c_str(), ply->GetLeader()->m_chaname[ply->GetLeader()->m_currcha].c_str());
		// 通知Client组成员变化
		{
			Team* l_team = ply->GetTeam();

			WPacket l_wpk = GetWPacket();
			l_wpk.WriteCmd(CMD_PC_TEAM_REFRESH);
			l_wpk.WriteChar(TEAM_MSG_ADD);
			l_wpk.WriteChar(uChar(l_count));

			Player* l_plylst[10240];
			short l_plynum = 0;

			Player* l_ply1;
			char l_currcha;
			RunChainGetArmor<TeamMember> l(*l_team);
			for (int i = 0; i < l_count && (l_ply1 = static_cast<Player*>(l_team->GetNextItem())); i++) {
				if ((l_currcha = l_ply1->m_currcha) >= 0) {
					l_wpk.WriteLong(l_ply1->m_chaid[l_currcha]);
					l_wpk.WriteString(l_ply1->m_chaname[l_currcha].c_str());
					l_wpk.WriteString(l_ply1->m_motto[l_currcha].c_str());
					l_wpk.WriteShort(l_ply1->m_icon[l_currcha]);

					l_plylst[l_plynum] = l_ply1;
					l_plynum++;
				}
			}
			l.unlock();

			SendToClient(l_plylst, l_plynum, l_wpk);
		}
		// 通知GameServer组成员变化
		{
			Team* l_team = ply->GetTeam();
			Player* l_plyr;
			GateServer* l_gate[30];
			for (int i = 0; i < 30; i++)
				l_gate[i] = 0;
			char l_gtnum = 0;

			WPacket l_wpk = GetWPacket();
			l_wpk.WriteCmd(CMD_PM_TEAM);
			l_wpk.WriteChar(TEAM_MSG_ADD);
			l_wpk.WriteChar(uChar(l_count));
			RunChainGetArmor<TeamMember> l(*l_team);
			for (int i = 0; i < l_count && (l_plyr = static_cast<Player*>(l_team->GetNextItem())); i++) {
				l_wpk.WriteString(l_plyr->m_gate->m_name.c_str());
				l_wpk.WriteLongLong(l_plyr->m_gtAddr);
				l_wpk.WriteLong(l_plyr->m_chaid[l_plyr->m_currcha]);
				for (int j = 0; j < 30; j++) {
					if (l_gate[j] == l_plyr->m_gate) {
						break;
					}
					if (!l_gate[j]) {
						l_gate[j] = l_plyr->m_gate;
						l_gtnum++;
						break;
					}
				}
			}
			l.unlock();
#if 1
			l_gtnum = l_gtnum ? 1 : 0;
#endif
			for (int j = 0; j < l_gtnum; j++) {
				if (auto* sock = l_gate[j]->GetDataSock(); sock) {
					sock->SendData(l_wpk);
				}
				// l_line<<newln<<"确认ToGameServer的加入组队操作发送了数据到GateServer";
				LG("Team", "MP_TEAM_CREATE() send ToGameServer data to GateServer\n");
			}
		}
	}
}
void GroupServerApp::CP_TEAM_LEAVE(Player* ply, DataSocket* datasock, RPacket& pk) {
	Team* l_team = ply->GetTeam();
	if (!l_team)
		return;
	Player* l_leader = l_team->GetLeader();
	int l_count = ply->LeaveTeam();
	if (l_count) {
		// 通知Client组成员变化
		{
			WPacket l_wpk = GetWPacket();
			l_wpk.WriteCmd(CMD_PC_TEAM_REFRESH);
			l_wpk.WriteChar(TEAM_MSG_LEAVE);
			l_wpk.WriteChar(uChar(l_count));

			Player* l_plylst[10240];
			short l_plynum = 0;

			Player* l_ply1;
			char l_currcha;
			RunChainGetArmor<TeamMember> l(*l_team);
			for (int i = 0; i < l_count && ((l_ply1 = static_cast<Player*>(l_team->GetNextItem())) || (l_ply1 = ply)); i++) {
				if ((l_currcha = l_ply1->m_currcha) >= 0) {
					l_wpk.WriteLong(l_ply1->m_chaid[l_currcha]);
					l_wpk.WriteString(l_ply1->m_chaname[l_currcha].c_str());
					l_wpk.WriteString(l_ply1->m_motto[l_currcha].c_str());
					l_wpk.WriteShort(l_ply1->m_icon[l_currcha]);

					l_plylst[l_plynum] = l_ply1;
					l_plynum++;
				}
			}
			l.unlock();
			SendToClient(l_plylst, l_plynum, l_wpk);
		}
		// 通知GameServer组成员变化
		{
			Player* l_plyr;
			GateServer* l_gate[30];
			for (int i = 0; i < 30; i++)
				l_gate[i] = 0;
			char l_gtnum = 0;

			WPacket l_wpk = GetWPacket();
			l_wpk.WriteCmd(CMD_PM_TEAM);
			l_wpk.WriteChar(TEAM_MSG_LEAVE);
			l_wpk.WriteChar(uChar(l_count));
			RunChainGetArmor<TeamMember> l(*l_team);
			for (int i = 0; i < l_count && ((l_plyr = static_cast<Player*>(l_team->GetNextItem())) || (l_plyr = ply)); i++) {
				l_wpk.WriteString(l_plyr->m_gate->m_name.c_str());
				l_wpk.WriteLongLong(l_plyr->m_gtAddr);
				l_wpk.WriteLong(l_plyr->m_chaid[l_plyr->m_currcha]);
				for (int j = 0; j < 30; j++) {
					if (l_gate[j] == l_plyr->m_gate) {
						break;
					}
					if (!l_gate[j]) {
						l_gate[j] = l_plyr->m_gate;
						l_gtnum++;
						break;
					}
				}
			}
			l.unlock();
#if 1
			l_gtnum = l_gtnum ? 1 : 0;
#endif
			for (int j = 0; j < l_gtnum; j++) {
				if (auto* sock = l_gate[j]->GetDataSock(); sock) {
					sock->SendData(l_wpk);
				}
			}

			if (l_count == 2) {
				l_team->GetLeader()->LeaveTeam();
			}
			/*l_line<<newln<<"玩家"<<ply->m_chaname[ply->m_currcha]<<"离开了组队"
				<<l_leader->m_chaname[l_leader->m_currcha]<<(l_count ==2?",队伍解散。":"。")
				<<endln;
			*/
			LG("Team", "player %s leave team %s%s\n", ply->m_chaname[ply->m_currcha].c_str(), l_leader->m_chaname[l_leader->m_currcha].c_str(), (l_count == 2 ? ",free team." : "."));
		}
	}
}
void GroupServerApp::CP_TEAM_KICK(Player* ply, DataSocket* datasock, RPacket& pk) {
	Team* l_team = ply->GetTeam();
	if (!l_team)
		return;
	Player* l_leader = l_team->GetLeader();
	if (ply != l_leader)
		return;
	DWORD dwKickedID = pk.ReadLong();
	if (dwKickedID == (ply->m_currcha >= 0 ? ply->m_chaid[ply->m_currcha] : 0)) {
		return;
	}
	Player* pKicker = l_team->GetMember(dwKickedID);
	if (!pKicker) {
		// l_line<<newln<<"队长"<<ply->m_chaname[ply->m_currcha]<<"踢出队员不存在！ID["
		//	<<dwKickedID<<"]"<<endln;
		LG("Team", "captain %s killed member not exsit! ID[%u]\n", ply->m_chaname[ply->m_currcha].c_str(), dwKickedID);
		return;
	}
	int l_count = pKicker->LeaveTeam();
	if (l_count) {
		// 通知Client组成员变化
		{
			WPacket l_wpk = GetWPacket();
			l_wpk.WriteCmd(CMD_PC_TEAM_REFRESH);
			l_wpk.WriteChar(TEAM_MSG_KICK);
			l_wpk.WriteChar(uChar(l_count));

			Player* l_plylst[10240];
			short l_plynum = 0;

			Player* l_ply1;
			char l_currcha;
			RunChainGetArmor<TeamMember> l(*l_team);
			for (int i = 0; i < l_count && ((l_ply1 = static_cast<Player*>(l_team->GetNextItem())) || (l_ply1 = pKicker)); i++) {
				if ((l_currcha = l_ply1->m_currcha) >= 0) {
					l_wpk.WriteLong(l_ply1->m_chaid[l_currcha]);
					l_wpk.WriteString(l_ply1->m_chaname[l_currcha].c_str());
					l_wpk.WriteString(l_ply1->m_motto[l_currcha].c_str());
					l_wpk.WriteShort(l_ply1->m_icon[l_currcha]);

					l_plylst[l_plynum] = l_ply1;
					l_plynum++;
				}
			}
			l.unlock();
			SendToClient(l_plylst, l_plynum, l_wpk);
		}
		// 通知GameServer组成员变化
		{
			Player* l_plyr;
			GateServer* l_gate[30];
			for (int i = 0; i < 30; i++)
				l_gate[i] = 0;
			char l_gtnum = 0;

			WPacket l_wpk = GetWPacket();
			l_wpk.WriteCmd(CMD_PM_TEAM);
			l_wpk.WriteChar(TEAM_MSG_LEAVE);
			l_wpk.WriteChar(uChar(l_count));
			RunChainGetArmor<TeamMember> l(*l_team);
			for (int i = 0; i < l_count && ((l_plyr = static_cast<Player*>(l_team->GetNextItem())) || (l_plyr = pKicker)); i++) {
				l_wpk.WriteString(l_plyr->m_gate->m_name.c_str());
				l_wpk.WriteLongLong(l_plyr->m_gtAddr);
				l_wpk.WriteLong(l_plyr->m_chaid[l_plyr->m_currcha]);
				for (int j = 0; j < 30; j++) {
					if (l_gate[j] == l_plyr->m_gate) {
						break;
					}
					if (!l_gate[j]) {
						l_gate[j] = l_plyr->m_gate;
						l_gtnum++;
						break;
					}
				}
			}
			l.unlock();
#if 1
			l_gtnum = l_gtnum ? 1 : 0;
#endif
			for (int j = 0; j < l_gtnum; j++) {
				if (auto* sock = l_gate[j]->GetDataSock(); sock) {
					sock->SendData(l_wpk);
				}
			}

			if (l_count == 2) {
				l_team->GetLeader()->LeaveTeam();
			}
			/*l_line<<newln<<"玩家"<<pKicker->m_chaname[ply->m_currcha]<<"被队长踢出了组队"
				<<l_leader->m_chaname[l_leader->m_currcha]<<(l_count ==2?",队伍解散。":"。")
				<<endln;
			*/
			LG("Team", "player %s killed by captain %s%s\n", pKicker->m_chaname[ply->m_currcha].c_str(), l_leader->m_chaname[l_leader->m_currcha].c_str(), (l_count == 2 ? ",free team." : "."));
		}
	}
}
void Player::TeamInvitedCheck(Invited* invited) {
	Player* l_inviter = invited->m_ptinviter.m_ply;
	if (m_currcha < 0) {
		TeamEndInvited(l_inviter);
	} else if (l_inviter->m_currcha < 0 || l_inviter->m_chaid[l_inviter->m_currcha] != invited->m_ptinviter.m_chaid) {
		WPacket l_wpk = g_gpsvr->GetWPacket();
		l_wpk.WriteCmd(CMD_PC_TEAM_CANCEL);
		l_wpk.WriteChar(MSG_TEAM_CANCLE_OFFLINE);
		l_wpk.WriteLong(invited->m_ptinviter.m_chaid);
		g_gpsvr->SendToClient(this, l_wpk);
		TeamEndInvited(l_inviter);
	} else if (l_inviter->GetTeam() && l_inviter->GetTeam()->GetTotal() >= g_gpsvr->const_team.MemberMax) {
		WPacket l_wpk = g_gpsvr->GetWPacket();
		l_wpk.WriteCmd(CMD_PC_TEAM_CANCEL);
		l_wpk.WriteChar(MSG_TEAM_CANCLE_ISFULL);
		l_wpk.WriteLong(invited->m_ptinviter.m_chaid);
		g_gpsvr->SendToClient(this, l_wpk);
		TeamEndInvited(l_inviter);
	} else if (l_inviter->GetTeam() && l_inviter->GetLeader() != l_inviter) {
		WPacket l_wpk = g_gpsvr->GetWPacket();
		l_wpk.WriteCmd(CMD_PC_TEAM_CANCEL);
		l_wpk.WriteChar(MSG_TEAM_CANCLE_CANCEL);
		l_wpk.WriteLong(invited->m_ptinviter.m_chaid);
		g_gpsvr->SendToClient(this, l_wpk);
		TeamEndInvited(l_inviter);
	} else if (GetTeam() && GetTeam()->GetTotal() > 1) {
		char l_buf[256];
		// sprintf(l_buf,"你邀请的【%s】已经加入了别的组队，你的邀请被取消。",m_chaname[m_currcha].c_str());
		sprintf(l_buf, RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00028), m_chaname[m_currcha].c_str());
		l_inviter->SendSysInfo(l_buf);

		WPacket l_wpk = g_gpsvr->GetWPacket();
		l_wpk.WriteCmd(CMD_PC_TEAM_CANCEL);
		l_wpk.WriteChar(MSG_TEAM_CANCLE_CANCEL);
		l_wpk.WriteLong(invited->m_ptinviter.m_chaid);
		g_gpsvr->SendToClient(this, l_wpk);
		TeamEndInvited(l_inviter);
	} else if (g_gpsvr->GetCurrentTick() - invited->m_tick >= g_gpsvr->const_team.PendTimeOut) {
		char l_buf[256];
		// sprintf(l_buf,"你对【%s】的组队邀请已超过%d秒钟没有回应，系统自动取消了你的邀请。",m_chaname[m_currcha].c_str(),g_gpsvr->const_team.PendTimeOut/1000);
		sprintf(l_buf, RES_STRING(GP_GROUPSERVERAPPTEAM_CPP_00029), m_chaname[m_currcha].c_str(), g_gpsvr->const_team.PendTimeOut / 1000);
		l_inviter->SendSysInfo(l_buf);

		WPacket l_wpk = g_gpsvr->GetWPacket();
		l_wpk.WriteCmd(CMD_PC_TEAM_CANCEL);
		l_wpk.WriteChar(MSG_TEAM_CANCLE_TIMEOUT);
		l_wpk.WriteLong(invited->m_ptinviter.m_chaid);
		g_gpsvr->SendToClient(this, l_wpk);
		TeamEndInvited(l_inviter);
	}
}
void GroupServerApp::MP_SWITCH(Player* ply) {
	// Safety check: validate player pointer before accessing members
	if (!ply) {
		LG("Security", "MP_SWITCH: Received null player pointer\n");
		return;
	}

	// Validate m_currcha is within valid bounds
	if (ply->m_currcha < 0 || ply->m_currcha >= Player::emMaxCharacters) {
		LG("Security", "MP_SWITCH: Invalid m_currcha=%d for player %s\n", 
		   ply->m_currcha, ply->m_acctname.c_str());
		return;
	}

	Team* l_team = ply->GetTeam();
	if (!l_team)
		return;
	Player* l_leader = l_team->GetLeader();
	int l_count = l_team->GetTotal();
	{
		Player* l_plyr;
		GateServer* l_gate[3];
		for (int i = 0; i < 3; i++)
			l_gate[i] = 0;
		char l_gtnum = 0;

		WPacket l_wpk = GetWPacket();
		l_wpk.WriteCmd(CMD_PM_TEAM);
		l_wpk.WriteChar(TEAM_MSG_UPDATE);
		l_wpk.WriteChar(uChar(l_count));
		RunChainGetArmor<TeamMember> l(*l_team);
		for (int i = 0; i < l_count && ((l_plyr = static_cast<Player*>(l_team->GetNextItem())) || (l_plyr = ply)); i++) {
			// Skip players with invalid character index
			if (!l_plyr || l_plyr->m_currcha < 0 || l_plyr->m_currcha >= Player::emMaxCharacters || !l_plyr->m_gate) {
				continue;
			}
			l_wpk.WriteString(l_plyr->m_gate->m_name.c_str());
			l_wpk.WriteLongLong(l_plyr->m_gtAddr);
			l_wpk.WriteLong(l_plyr->m_chaid[l_plyr->m_currcha]);
			for (int j = 0; j < 3; j++) {
				if (l_gate[j] == l_plyr->m_gate) {
					break;
				}
				if (!l_gate[j]) {
					l_gate[j] = l_plyr->m_gate;
					l_gtnum++;
					break;
				}
			}
		}
		l.unlock();
#if 1
		l_gtnum = l_gtnum ? 1 : 0;
#endif
		for (int j = 0; j < l_gtnum; j++) {
			if (auto* sock = l_gate[j]->GetDataSock(); sock) {
				sock->SendData(l_wpk);
			}
		}

		// l_line<<newln<<"玩家"<<ply->m_chaname[ply->m_currcha]<<"因为跳转地图刷新了组队"<<l_leader->m_chaname[l_leader->m_currcha];
		if (l_leader && l_leader->m_currcha >= 0 && l_leader->m_currcha < Player::emMaxCharacters) {
			LG("Team", "player %s refresh team by switch map %s\n", ply->m_chaname[ply->m_currcha].c_str(), l_leader->m_chaname[l_leader->m_currcha].c_str());
		} else {
			LG("Team", "player %s refresh team by switch map (leader unavailable)\n", ply->m_chaname[ply->m_currcha].c_str());
		}
	}
}
