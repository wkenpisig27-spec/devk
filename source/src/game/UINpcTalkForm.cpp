#include "StdAfx.h"
#include "uinpctalkform.h"
#include "uiequipform.h"
#include "uigoodsgrid.h"
#include "uiitemcommand.h"
#include "uipage.h"
#include "packetcmd.h"
#include "gameapp.h"
#include "scene.h"
#include "character.h"
#include "uiboxform.h"
#include "uiedit.h"
#include "STNpcTalk.h"
#include "uiformmgr.h"
#include "uimemo.h"
#include "worldscene.h"
#include <shellapi.h>

#include "NetChat.h"
#include "UIsystemform.h"
#include "GlobalVar.h"
#include "UILabel.h"
#include "AreaRecord.h"

#include "rmlui/RmlUiNpcTalkForm.h"

#include <algorithm>
#include <cctype>
#include <string>

using namespace std;
using namespace GUI;

static BYTE _byIndex = -1;
static BYTE _byPage = -1;
static DWORD _npcID = -1;
static BYTE _byCmd = 0;

BYTE CNpcTalkMgr::_byTalkStyle = 0;

namespace {

std::string NormalizeTalkBody(const char* text) {
	std::string out;
	if (!text)
		return out;
	for (const char* p = text; *p; ++p) {
		if (*p == '_')
			out += '\n';
		else
			out += *p;
	}
	return out;
}

bool LooksLikeTrade(const std::string& label) {
	std::string lower = label;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
		return (char)std::tolower(c);
	});
	return lower.find("trade") != std::string::npos || lower.find("shop") != std::string::npos ||
		   lower.find("buy") != std::string::npos || lower.find("sell") != std::string::npos;
}

bool LooksLikeExit(const std::string& label) {
	std::string lower = label;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
		return (char)std::tolower(c);
	});
	return lower.find("nothing") != std::string::npos || lower.find("goodbye") != std::string::npos ||
		   lower.find("leave") != std::string::npos || lower == "exit" || lower == "cancel";
}

std::string ResolveNpcName(DWORD npcId, const std::string& body) {
	if (CGameScene* scene = CGameApp::GetCurScene()) {
		if (CCharacter* cha = scene->SearchByID(npcId)) {
			if (const char* name = cha->getName()) {
				if (name[0])
					return name;
			}
		}
	}
	// Fallback: "Name: message" baked into talk scripts.
	const size_t colon = body.find(':');
	if (colon != std::string::npos && colon > 0 && colon < 48) {
		std::string name = body.substr(0, colon);
		while (!name.empty() && (name.back() == ' ' || name.back() == '\n'))
			name.pop_back();
		if (!name.empty() && name.find('\n') == std::string::npos)
			return name;
	}
	return "NPC";
}

void PushRmlTalkView(const char* bodyRaw, const NET_FUNCPAGE* funcs, BYTE byCount, BYTE byMisNum) {
	CRmlUiNpcTalkForm::TalkView view;
	view.body = NormalizeTalkBody(bodyRaw);
	view.npcName = ResolveNpcName(_npcID, view.body);
	view.title = "Dialogue";

	// Avoid repeating "Jimberry: ..." under the hero name.
	{
		std::string prefix = view.npcName + ":";
		if (view.body.size() > prefix.size()) {
			std::string head = view.body.substr(0, prefix.size());
			std::string headLower = head;
			std::string prefixLower = prefix;
			std::transform(headLower.begin(), headLower.end(), headLower.begin(), [](unsigned char c) {
				return (char)std::tolower(c);
			});
			std::transform(prefixLower.begin(), prefixLower.end(), prefixLower.begin(), [](unsigned char c) {
				return (char)std::tolower(c);
			});
			if (headLower == prefixLower) {
				size_t i = prefix.size();
				while (i < view.body.size() && (view.body[i] == ' ' || view.body[i] == '\n'))
					++i;
				view.body = view.body.substr(i);
			}
		}
	}

	if (funcs && byMisNum > 0) {
		for (int i = 0; i < byMisNum; ++i) {
			if (!funcs->MisItem[i].szMis[0])
				continue;
			CRmlUiNpcTalkForm::OptionView opt;
			opt.kind = CRmlUiNpcTalkForm::OptionKind::Mission;
			opt.index = i;
			opt.label = funcs->MisItem[i].szMis;
			opt.isQuest = true;
			view.options.push_back(std::move(opt));
		}
	}

	if (funcs && byCount > 0) {
		for (int i = 0; i < byCount; ++i) {
			CRmlUiNpcTalkForm::OptionView opt;
			opt.index = i;
			const char* raw = funcs->FuncItem[i].szFunc;
			if (!raw || !raw[0])
				continue;
			if (raw[0] == '@') {
				string item = &raw[1];
				size_t pos = item.find("http");
				if (pos == string::npos) {
					opt.kind = CRmlUiNpcTalkForm::OptionKind::Func;
					opt.label = item;
				} else {
					opt.kind = CRmlUiNpcTalkForm::OptionKind::Url;
					opt.url = item.substr(pos, item.length());
					opt.label = item.substr(0, pos);
				}
			} else {
				opt.kind = CRmlUiNpcTalkForm::OptionKind::Func;
				opt.label = raw;
			}
			opt.emphasize = LooksLikeTrade(opt.label);
			opt.muted = LooksLikeExit(opt.label) && !opt.emphasize;
			view.options.push_back(std::move(opt));
		}
	}

	CRmlUiNpcTalkForm::Instance().ApplyView(view);
}

} // namespace

bool CNpcTalkMgr::Init() {
	m_bIsNpcTalk = false;
	CFormMgr& mgr = CFormMgr::s_Mgr;
	(void)mgr;

	frmNPCchat = _FindForm("frmNPCchat");
	if (!frmNPCchat)
		return false;
	frmNPCchat->evtEntrustMouseEvent = _MainMouseNPCEvent;
	frmNPCchat->SetIsEscClose(false);
	frmNPCchat->SetIsShow(false);

	memCtrl = dynamic_cast<CMemo*>(frmNPCchat->Find("memCtrl"));
	if (!memCtrl)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmNPCchat->GetName(), "memCtrl");
	memCtrl->evtSelectChange = _evtMemSelectChange;
	return true;
}

void CNpcTalkMgr::End() {
}

DWORD CNpcTalkMgr::GetNpcId() {
	return _npcID;
}

void CNpcTalkMgr::ShowFuncPage(BYTE byFuncPage, BYTE byCount, BYTE byMisNum, const NET_FUNCPAGE& FuncArray, DWORD dwNpcID) {
	m_bIsNpcTalk = true;
	_byPage = byFuncPage;
	_npcID = dwNpcID;
	_byIndex = -1;

	// Keep legacy memo filled as a silent data host (optional fallback).
	if (memCtrl) {
		memCtrl->Init();
		memCtrl->reset();
		memCtrl->SetCaption(FuncArray.szTalk);
		if (byCount > 0) {
			memCtrl->SetIsHaveItem(true);
			memCtrl->SetItemRowNum(byCount);
			for (int i = 0; i < byCount; i++) {
				if (FuncArray.FuncItem[i].szFunc[0] == '@') {
					string item = &FuncArray.FuncItem[i].szFunc[1];
					size_t pos = item.find("http");
					if (pos == string::npos) {
						memCtrl->AddItemRowContent(i, item.c_str());
					} else {
						string http = item.substr(pos, item.length());
						item = item.substr(0, pos);
						memCtrl->AddItemRowContent(i, item.c_str(), http.c_str());
					}
				} else {
					memCtrl->AddItemRowContent(i, FuncArray.FuncItem[i].szFunc);
				}
			}
		}
		if (byMisNum > 0) {
			memCtrl->SetIsHaveMis(true);
			memCtrl->SetMisRowNum(byMisNum);
			for (int i = 0; i < byMisNum; i++) {
				int index = FuncArray.MisItem[i].byState;
				int y = (index / 16) * 16;
				int x = (index % 16) * 16;
				CGraph* p = new CGraph("texture/ui/corsairs/missionIcon.png", 16, 16, x, y, 10);
				memCtrl->AddIcon(i, p);
				memCtrl->AddMisRowContent(i, FuncArray.MisItem[i].szMis);
			}
		}
		memCtrl->ProcessCaption();
	}

	if (frmNPCchat && frmNPCchat->GetIsShow())
		frmNPCchat->SetIsShow(false);

	PushRmlTalkView(FuncArray.szTalk, &FuncArray, byCount, byMisNum);
}

void CNpcTalkMgr::CloseForm() {
	if (CRmlUiNpcTalkForm::Instance().IsVisible())
		CRmlUiNpcTalkForm::Instance().Hide();
	if (m_bIsNpcTalk && frmNPCchat && frmNPCchat->GetIsShow())
		frmNPCchat->Close();
}

void CNpcTalkMgr::_MainMouseNPCEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string name = pSender->GetName();

	if (name == "btnNo" || name == "btnClose") {
		pSender->GetForm()->Close();
		pSender->GetForm()->Find("memCtrl")->SetCaption("");
		CRmlUiNpcTalkForm::Instance().Hide();
		return;
	}
	return;
}

void CNpcTalkMgr::_evtMemSelectChange(CGuiData* pSender) {
	CMemo* memo = dynamic_cast<CMemo*>(pSender);
	if (!memo)
		return;

	if (!memo->GetIsHaveItem() && !memo->GetIsHaveMis())
		return;

	int nMis = -1;
	int nItem = -1;

	nMis = memo->GetSelectMis();
	nItem = memo->GetSelectItem();

	if (nMis == -1 && nItem == -1)
		return;

	if (nMis >= 0) {
		_byTalkStyle = 1;
		_byIndex = nMis;
	}

	if (nItem >= 0) {
		_byTalkStyle = 0;
		_byIndex = nItem;
	}

	switch (_byTalkStyle) {
	case 0: {
		string item, itemex;
		memo->GetSelectItemText(item, itemex);
		if (itemex.empty()) {
			CS_SelFunction(_npcID, _byPage, _byIndex);
		} else {
			::ShellExecute(nullptr, "open",
						   itemex.c_str(), nullptr, nullptr, SW_SHOW);
		}
	} break;
	case 1:
		CS_SelMission(_npcID, _byIndex);

		break;
	case 2:
		CS_SelMissionFunc(_npcID, _byPage, _byIndex);
		break;
	}
	pSender->GetForm()->Close();
	CRmlUiNpcTalkForm::Instance().Hide();
}

void CNpcTalkMgr::FrameMove(DWORD dwTime) {
	return;
	static CTimeWork time(100);
	if (!time.IsTimeOut(dwTime))
		return;

	if (frmNPCchat && !frmNPCchat->GetIsShow() && !CRmlUiNpcTalkForm::Instance().IsVisible()) {
		if (m_HelpInfoList.size() > 0) {
			HELP_LIST::iterator pos = m_HelpInfoList.begin();
			if (pos == m_HelpInfoList.end())
				return;

			ShowHelpInfo(*pos);
			m_HelpInfoList.erase(pos);
		}
	}
}

void CNpcTalkMgr::LoadingCall() {
}

void CNpcTalkMgr::SwitchMap() {
	if (!(dynamic_cast<CWorldScene*>(CGameApp::GetCurScene())))
		return;

	static bool IsFirstWorldScene = true;
	if (IsFirstWorldScene) {
		IsFirstWorldScene = false;
		FILE* fp = fopen("./scripts/txt/HelpInfo.tx", "rt");
		if (fp) {
			if (fseek(fp, 0, SEEK_END) == 0) {
				long size = ftell(fp);
				fseek(fp, 0, SEEK_SET);

				NET_HELPINFO Info;
				memset(&Info, 0, sizeof(NET_HELPINFO));
				fread(Info.szDesp, size > HELPINFO_DESPSIZE - 1 ? HELPINFO_DESPSIZE - 1 : size, 1, fp);
				Info.byType = mission::MIS_HELP_DESP;
				AddHelpInfo(Info);
			}
			fclose(fp);
		}
		return;
	}
}

void CNpcTalkMgr::ShowHelpInfo(const NET_HELPINFO& Info) {
	if (Info.byType == mission::MIS_HELP_DESP || Info.byType == mission::MIS_HELP_IMAGE) {
		m_bIsNpcTalk = false;
		_npcID = 0;
		if (memCtrl) {
			memCtrl->Init();
			memCtrl->SetCaption(Info.szDesp);
			memCtrl->ProcessCaption();
		}
		if (frmNPCchat && frmNPCchat->GetIsShow())
			frmNPCchat->SetIsShow(false);
		PushRmlTalkView(Info.szDesp, nullptr, 0, 0);
	} else if (Info.byType == mission::MIS_HELP_SOUND) {
		// Info.sID;
	} else {
		return;
	}
	return;
}

void CNpcTalkMgr::AddHelpInfo(const NET_HELPINFO& Info) {
	m_HelpInfoList.push_back(Info);
}

void CNpcTalkMgr::ShowTalkPage(const char* content, BYTE command, DWORD npcID) {
	m_bIsNpcTalk = true;
	_byCmd = command;
	_npcID = npcID;
	_byPage = 0;
	_byIndex = -1;

	if (memCtrl) {
		memCtrl->Init();
		memCtrl->SetCaption(content);
		memCtrl->ProcessCaption();
	}
	if (frmNPCchat && frmNPCchat->GetIsShow())
		frmNPCchat->SetIsShow(false);

	PushRmlTalkView(content, nullptr, 0, 0);
}

void CNpcTalkMgr::CloseTalk(DWORD dwNpcID) {
	(void)dwNpcID;
	CRmlUiNpcTalkForm::Instance().Hide();
	if (frmNPCchat) {
		frmNPCchat->Close();
		if (memCtrl)
			memCtrl->SetCaption("");
	}
}

void CNpcTalkMgr::HideTalkUi() {
	CRmlUiNpcTalkForm::Instance().Hide();
	if (frmNPCchat && frmNPCchat->GetIsShow())
		frmNPCchat->Close();
	m_bIsNpcTalk = false;
}

bool CNpcTalkMgr::IsTalkUiVisible() const {
	return CRmlUiNpcTalkForm::Instance().IsVisible() || (frmNPCchat && frmNPCchat->GetIsShow());
}

void RmlNpcTalk_OnClose() {
	g_stUINpcTalk.HideTalkUi();
}

void RmlNpcTalk_OnOption(int kind, int index, const char* url) {
	if (kind == (int)CRmlUiNpcTalkForm::OptionKind::Url) {
		if (url && url[0])
			::ShellExecute(nullptr, "open", url, nullptr, nullptr, SW_SHOW);
		g_stUINpcTalk.HideTalkUi();
		return;
	}
	if (kind == (int)CRmlUiNpcTalkForm::OptionKind::Mission) {
		CS_SelMission(g_stUINpcTalk.GetNpcId(), (BYTE)index);
		g_stUINpcTalk.HideTalkUi();
		return;
	}
	CS_SelFunction(g_stUINpcTalk.GetNpcId(), _byPage, (BYTE)index);
	g_stUINpcTalk.HideTalkUi();
}

