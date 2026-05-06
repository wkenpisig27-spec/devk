#include "stdafx.h"
#include "netguild.h"
#include "UIGlobalVar.h"
#include "GuildMemberData.h"
#include "GuildMembersMgr.h"
#include "RecruitMemberData.h"
#include "RecruitMembersMgr.h"
#include "GuildListMgr.h"
#include "guildlistdata.h"
#include "GuildData.h"
#include "UIGuildApply.h"
#include "uiguildlist.h"
#include "UIGuildMgr.h"
#include "UIGraph.h"
#include "UIChat.h"
#include "UITreeView.h"
#include "ChatIconSet.h"
#include "CompCommand.h"
#include "GameApp.h"
#include "Character.h"
#include "StringLib.h"
#include "uiboatform.h"
#include "UIGuildChallengeForm.h"

using namespace std;

void NetMC_GUILD_GETNAME() {
	CUIGuildApply::ShowForm();
}
void NetMC_LISTGUILD_BEGIN() {
	CGuildListMgr::ResetAll();
}

void NetMC_LISTGUILD_END() {
	CUIGuildList::ResetOrder();
	CGuildListMgr::SortGuildsByName();
	CUIGuildList::ShowGuildList();
}

void NetMC_LISTGUILD(uLong id, cChar* name, cChar* motto, cChar* leadername, uShort memtotal, LLong exp) {
	CGuildListMgr::AddGuild(new CGuildListData(id, name, motto, leadername, memtotal, exp));
}

void NetMC_GUILD_TRYFORCFM(cChar* oldgldname) {
	CUIGuildList::OnMsgReplaceApply(oldgldname);
}

void NetMC_LISTTRYPLAYER_BEGIN(uLong gldid, cChar* gldname, cChar* motto, char stat, cChar* ldrname, uShort memnum, uShort maxmem, LLong exp, uLong remain) {
	CRecruitMembersMgr::ResetAll();
	CGuildData::sGuildData data;
	data.guild_id = gldid;
	data.guild_name = gldname;
	data.guild_motto_name = motto;
	data.guild_master_name = ldrname;
	data.guild_member_count = memnum;
	data.guild_member_max = maxmem;
	data.guild_experience = exp;
	data.guild_remain_time = remain;
	data.guild_state = CGuildData::eState(stat);
	CGuildData::LoadData(data);
}

void NetMC_LISTTRYPLAYER_END() {
	CUIGuildMgr::ShowForm();
}

void NetMC_LISTTRYPLAYER(uLong chaid, cChar* chaname, cChar* job, uShort degree) {
	// Validate string parameters to prevent crash from null pointers
	if (!chaname) chaname = "";
	if (!job) job = "";
	
	CRecruitMemberData* pMemberData = new CRecruitMemberData;
	pMemberData->SetID(chaid);
	pMemberData->SetName(chaname);
	pMemberData->SetJob(job);
	pMemberData->SetLevel(degree);
	CRecruitMembersMgr::AddRecruitMember(pMemberData);
}

void NetPC_GUILD_ONLINE(uLong chaid) {
	CGuildMemberData* pMemberData = CGuildMembersMgr::FindGuildMemberByID(chaid);
	if (!pMemberData)
		return;
	pMemberData->SetOnline(true);
	CTextGraph* pItem = static_cast<CTextGraph*>(pMemberData->GetPointer());
	if (pItem) {
		CChatIconInfo* pIconInfo = GetChatIconInfo(pMemberData->GetIcon());
		if (pIconInfo) {
			CGuiPic* pPic = pItem->GetImage();
			if (pPic) {
				string strPath = "texture/ui/HEAD/";
				pPic->LoadImage((strPath + pIconInfo->szSmall).c_str(), SMALL_ICON_SIZE, SMALL_ICON_SIZE, 0, pIconInfo->nSmallX, pIconInfo->nSmallY);
				pItem->SetColor(0xfffc2f20);
				pPic->SetFrame(0);
			}
			if (g_stUIChat.GetTeamView()) {
				g_stUIChat.GetTeamView()->Refresh();
			}
		}
	}
}

void NetPC_GUILD_UPDATEPERM(uLong chaid, uLong perm) {
	CGuildMemberData* pMemberData = CGuildMembersMgr::FindGuildMemberByID(chaid);
	if (!pMemberData)
		return;
	pMemberData->SetPerm(perm);
}

void NetPC_GUILD_OFFLINE(uLong chaid) {
	CGuildMemberData* pMemberData = CGuildMembersMgr::FindGuildMemberByID(chaid);
	if (!pMemberData)
		return;
	pMemberData->SetOnline(false);
	CTextGraph* pItem = static_cast<CTextGraph*>(pMemberData->GetPointer());
	if (pItem) {
		CChatIconInfo* pIconInfo = GetChatIconInfo(pMemberData->GetIcon());
		if (pIconInfo) {
			CGuiPic* pPic = pItem->GetImage();
			if (pPic) {
				std::string strPath = "texture/ui/HEAD/";
				pPic->LoadImage((strPath + pIconInfo->szSmallOff).c_str(), SMALL_ICON_SIZE, SMALL_ICON_SIZE, 0, pIconInfo->nSmallX, pIconInfo->nSmallY);
				pItem->SetColor(0xff000000);
				pPic->SetFrame(0);
			}
			if (g_stUIChat.GetTeamView()) {
				g_stUIChat.GetTeamView()->Refresh();
			}
		}
	}
}

void NetPC_GUILD_START_BEGIN(uLong guildid, cChar* guildname, uLong leaderid) {
	CGuildMembersMgr::ResetAll();
	if (g_stUIChat.GetGuildNode()) {
		g_stUIChat.GetGuildNode()->Clear();
	}
	CGuildData::SetGuildID(guildid);
	if (guildid) {
		CGuildData::SetGuildMasterID(leaderid);
		// CGuildData::SetGuildMasterName(guildname);
		CGuildData::SetGuildName(guildname);
	} else {
		CGuildData::SetGuildMasterID(0);
		// CGuildData::SetGuildMasterName("");
		CGuildData::SetGuildName("");
	}
}

void NetPC_GUILD_START_END() {
	if (CGuildData::GetGuildID()) {
	}
	if (g_stUIChat.GetTeamView()) {
		g_stUIChat.GetTeamView()->Refresh();
	}
}

void NetPC_GUILD_START(bool online, uLong chaid, cChar* chaname, cChar* motto, cChar* job, uShort degree, uShort icon, uLong permission) {
	// Validate string parameters to prevent crash from null pointers
	if (!chaname) chaname = "";
	if (!motto) motto = "";
	if (!job) job = "";
	
	CGuildMemberData* pMemberData = new CGuildMemberData;
	pMemberData->SetOnline(online);
	pMemberData->SetID(chaid);
	pMemberData->SetName(chaname);
	pMemberData->SetJob(job);
	pMemberData->SetLevel(degree);
	pMemberData->SetMottoName(motto);
	pMemberData->SetIcon(icon);
	pMemberData->SetManager(permission & emGldPermMgr);
	pMemberData->SetPerm(permission);
	CGuildMembersMgr::AddGuildMember(pMemberData);
	
	// Safely check if this is the main character
	if (g_pGameApp && g_pGameApp->GetCurScene() && g_pGameApp->GetCurScene()->GetMainCha()) {
		if (g_pGameApp->GetCurScene()->GetMainCha()->getHumanID() == chaid)
			return;
	}
	
	// Validate UI elements before accessing
	if (!g_stUIChat.GetGuildNode()) {
		return;
	}
	
	CTextGraph* pItem = new CTextGraph(2);
	pMemberData->SetPointer(pItem);
	string str = chaname;
	if (strlen(motto) > 0) {
		str += "(" + string(motto) + ")";
	}
	pItem->SetHint(str.c_str());
	str = StringLimit(str, 14);
	pItem->SetString(str.c_str());
	pItem->SetPointer(pMemberData);
	CChatIconInfo* pIconInfo = GetChatIconInfo(icon);
	if (pIconInfo) {
		CGuiPic* pPic = pItem->GetImage();
		string strPath = "texture/ui/HEAD/";
		if (online) {
			pPic->LoadImage((strPath + pIconInfo->szSmall).c_str(), SMALL_ICON_SIZE, SMALL_ICON_SIZE, 0, pIconInfo->nSmallX, pIconInfo->nSmallY);
			pItem->SetColor(0xfffc2f20);
		} else {
			pPic->LoadImage((strPath + pIconInfo->szSmallOff).c_str(), SMALL_ICON_SIZE, SMALL_ICON_SIZE, 0, pIconInfo->nSmallX, pIconInfo->nSmallY);
			pItem->SetColor(0xff000000);
		}
		pPic->SetFrame(0);
	}
	g_stUIChat.GetGuildNode()->AddItem(pItem);
}

void NetPC_GUILD_ADD(bool online, uLong chaid, cChar* chaname, cChar* motto, cChar* job, uShort degree, uShort icon, uLong permission) {
	if (CGuildMembersMgr::FindGuildMemberByID(chaid))
		return;
	
	// Validate string parameters to prevent crash from null pointers
	if (!chaname) chaname = "";
	if (!motto) motto = "";
	if (!job) job = "";
	
	CGuildMemberData* pMemberData = new CGuildMemberData;
	pMemberData->SetOnline(online);
	pMemberData->SetID(chaid);
	pMemberData->SetName(chaname);
	pMemberData->SetJob(job);
	pMemberData->SetLevel(degree);
	pMemberData->SetMottoName(motto);
	pMemberData->SetIcon(icon);
	pMemberData->SetManager(permission & emGldPermMgr);
	pMemberData->SetPerm(permission);
	CGuildMembersMgr::AddGuildMember(pMemberData);
	
	// Safely check if this is the main character
	if (g_pGameApp && g_pGameApp->GetCurScene() && g_pGameApp->GetCurScene()->GetMainCha()) {
		if (g_pGameApp->GetCurScene()->GetMainCha()->getHumanID() == chaid)
			return;
	}
	
	// Validate UI elements before accessing
	if (!g_stUIChat.GetGuildNode()) {
		return;
	}
	
	CTextGraph* pItem = new CTextGraph(2);
	pMemberData->SetPointer(pItem);
	string str = chaname;
	if (strlen(motto) > 0) {
		str += "(" + string(motto) + ")";
	}
	pItem->SetHint(str.c_str());
	str = StringLimit(str, 14);
	pItem->SetString(str.c_str());
	pItem->SetPointer(pMemberData);
	CChatIconInfo* pIconInfo = GetChatIconInfo(icon);
	if (pIconInfo) {
		CGuiPic* pPic = pItem->GetImage();
		string strPath = "texture/ui/HEAD/";
		if (online) {
			pPic->LoadImage((strPath + pIconInfo->szSmall).c_str(), SMALL_ICON_SIZE, SMALL_ICON_SIZE, 0, pIconInfo->nSmallX, pIconInfo->nSmallY);
			pItem->SetColor(0xfffc2f20);
		} else {
			pPic->LoadImage((strPath + pIconInfo->szSmallOff).c_str(), SMALL_ICON_SIZE, SMALL_ICON_SIZE, 0, pIconInfo->nSmallX, pIconInfo->nSmallY);
			pItem->SetColor(0xff000000);
		}
		pPic->SetFrame(0);
	}
	g_stUIChat.GetGuildNode()->AddItem(pItem);
	if (g_stUIChat.GetTeamView()) {
		g_stUIChat.GetTeamView()->Refresh();
	}
	CGuildData::SetMemberCount(CGuildData::GetMemberCount() + 1);
	CUIGuildMgr::RefreshForm();
}

void NetPC_GUILD_DEL(uLong chaid) {
	CGuildMemberData* pMemberData = CGuildMembersMgr::FindGuildMemberByID(chaid);
	if (!pMemberData) {
		return;
	}
	
	if (g_stUIChat.GetGuildNode() && pMemberData->GetPointer()) {
		g_stUIChat.GetGuildNode()->DelItem((CItemObj*)pMemberData->GetPointer());
	}
	CGuildMembersMgr::DelGuildMemberByID(chaid);
	
	if (g_stUIChat.GetTeamView()) {
		g_stUIChat.GetTeamView()->Refresh();
	}
	CGuildData::SetMemberCount(CGuildData::GetMemberCount() - 1);
	CUIGuildMgr::RefreshForm();
}

void NetPC_GUILD_STOP() {
	CGuildListMgr::ResetAll();
	CGuildMembersMgr::ResetAll();
	if (g_stUIChat.GetGuildNode()) {
		g_stUIChat.GetGuildNode()->Clear();
	}
	CUIGuildMgr::RemoveForm();
	CGuildData::Reset();
}

void NetMC_GUILD_MOTTO(cChar* motto) {
	// ?????????(???)-Arcol 2005.10.9
	CGuildData::SetGuildMottoName(motto);
	CUIGuildMgr::RefreshAttribute();
}

void NetMC_GUILD_INFO(DWORD dwCharID, DWORD dwGuildID, const char szGuildName[], const char szGuildMotto[], uLong chGuildPermission) {
	const char* pszLogName = g_LogName.GetLogName(dwCharID);
	LG(pszLogName, "Guild Info:%u, Name:%s, Motto:%s\n", dwGuildID, szGuildName, szGuildMotto);

	if (!CGameApp::GetCurScene()) {
		LG("error", RES_STRING(CL_LANGUAGE_MATCH_244));
		return;
	}

	if (!CGameScene::GetMainCha()) {
		LG("error", RES_STRING(CL_LANGUAGE_MATCH_245));
		return;
	}

	CCharacter* pCha = CGameScene::GetMainCha();
	if (pCha->getHumanID() == dwCharID) {
		CGuildData::SetGuildMottoName(szGuildMotto);
		CUIGuildMgr::RefreshAttribute();

		LG(pszLogName, "Guild - Main Cha:%u, %s\n", pCha->getAttachID(), pCha->getLogName());

		CCharacter* pAll[defMaxBoat + 1] = {0};
		for (int i = 0; i < defMaxBoat; i++)
			pAll[i] = g_stUIBoat.GetBoat(i)->GetCha();

		pAll[defMaxBoat] = g_stUIBoat.GetHuman();
		for (int i = 0; i < defMaxBoat + 1; i++) {
			if (pAll[i]) {
				LG(pszLogName, "Guild:%u, %s\n", pAll[i]->getAttachID(), pAll[i]->getLogName());
				pAll[i]->setGuildID(dwGuildID);
				pAll[i]->setGuildName(szGuildName);
				pAll[i]->setGuildMotto(szGuildMotto);
				pAll[i]->setGuildPermission(chGuildPermission);
			}
		}
	} else {
		pCha = CGameApp::GetCurScene()->SearchByHumanID(dwCharID);

		if (!pCha) {
			LG("error", RES_STRING(CL_LANGUAGE_MATCH_246), dwCharID);
			return;
		}

		pCha->setGuildID(dwGuildID);
		pCha->setGuildName(szGuildName);
		pCha->setGuildMotto(szGuildMotto);
		pCha->setGuildPermission(chGuildPermission);
	}
}

void NetMC_GUILD_CHALLINFO(const NET_GUILD_CHALLINFO& Info) {
	g_stGuildChallenge.SetContent(Info);
	g_stGuildChallenge.Show(true);
}
