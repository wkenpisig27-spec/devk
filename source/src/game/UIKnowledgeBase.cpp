#include "StdAfx.h"
#include "UIKnowledgeBase.h"
#include "UIFormMgr.h"
#include "uipage.h"
#include "uiEdit.h"
#include "UIScroll.h"
#include "uilist.h"
#include "UIFastCommand.h"
#include "uiitemcommand.h"
#include "UIMinimapForm.h"
#include "UIStartForm.h"
#include "NPCHelper.h"
#include "CharacterRecord.h"
#include "ItemRecord.h"
#include "gameapp.h"
#include "worldscene.h"
#include "StringLib.h"
#include "Tools.h"

#include <algorithm>
#include <set>

using namespace std;
using namespace GUI;

GUI::CKnowledgeBase g_stKnowledgeBase;

// ---------------------------------------------------------------------------
// Module-level callbacks
// ---------------------------------------------------------------------------

static void _evtKBScrollChange(CGuiData* pSender) {
	g_stKnowledgeBase.OnItemScrollChange();
}

static bool _onKBMouseScroll(int& nScroll) {
	return g_stKnowledgeBase.HandleMouseScroll(nScroll);
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

bool CKnowledgeBase::Init() {
	m_frmRoot = CFormMgr::s_Mgr.Find("frmKnowledgeBase");
	if (!m_frmRoot)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), "frmKnowledgeBase", "frmKnowledgeBase");

	m_lstNpcList = dynamic_cast<CList*>(m_frmRoot->Find("lstNpcList"));
	assert(m_lstNpcList != nullptr);
	m_lstNpcList->evtSelectChange = _evtListChange;
	m_lstCurrList = m_lstNpcList;

	m_chkID = dynamic_cast<CCheckBox*>(m_frmRoot->Find("chkID"));
	if (m_chkID) m_chkID->SetIsChecked(true);

	m_edtSearch = dynamic_cast<CEdit*>(m_frmRoot->Find("edtNpcSearch"));
	m_lastSearch    = "";
	m_npcSearch     = "";
	m_monsterSearch = "";
	m_itemSearch    = "";

	// Monster virtual-scroll slots
	m_monScrollPos = 0;
	for (int i = 0; i < MON_VISIBLE_COUNT; i++) {
		char buf[32];
		sprintf(buf, "labMonName%d", i);
		m_monNames[i] = dynamic_cast<CLabelEx*>(m_frmRoot->Find(buf));
		sprintf(buf, "labMonLevel%d", i);
		m_monLevels[i] = dynamic_cast<CLabelEx*>(m_frmRoot->Find(buf));
		sprintf(buf, "labMonHP%d", i);
		m_monHPs[i] = dynamic_cast<CLabelEx*>(m_frmRoot->Find(buf));
		sprintf(buf, "btnMonInfo%d", i);
		m_monButtons[i] = dynamic_cast<CTextButton*>(m_frmRoot->Find(buf));
		if (m_monButtons[i]) m_monButtons[i]->evtMouseClick = _evtMonInfoClick;
	}
	m_monScroll = dynamic_cast<CScroll*>(m_frmRoot->Find("scrMonsterList"));
	if (m_monScroll) {
		m_monScroll->evtChange = _evtMonScrollChange;
		m_monScroll->SetIsShow(false);
	}

	// Item database virtual-scroll slots
	m_itemScrollPos = 0;
	for (int i = 0; i < ITEM_VISIBLE_COUNT; i++) {
		char buf[32];
		sprintf(buf, "listItemSlots%d", i);
		m_itemSlots[i] = dynamic_cast<COneCommand*>(m_frmRoot->Find(buf));
		sprintf(buf, "labItemName%d", i);
		m_itemNames[i] = dynamic_cast<CLabelEx*>(m_frmRoot->Find(buf));
		sprintf(buf, "labItemDesc%d", i);
		m_itemDescs[i] = dynamic_cast<CLabelEx*>(m_frmRoot->Find(buf));
	}
	m_itemScroll = dynamic_cast<CScroll*>(m_frmRoot->Find("scrItemList"));
	if (m_itemScroll) {
		m_itemScroll->evtChange = _evtKBScrollChange;
		m_itemScroll->SetIsShow(false);
	}

	CFormMgr::s_Mgr.AddMouseScrollEvent(_onKBMouseScroll);

	m_listPage = dynamic_cast<CPage*>(m_frmRoot->Find("pgeSkill"));
	assert(m_listPage != nullptr);
	m_listPage->evtSelectPage = _evtPageChange;

	return true;
}

// ---------------------------------------------------------------------------
// FrameMove — search debounce
// ---------------------------------------------------------------------------

void CKnowledgeBase::FrameMove(DWORD dwTime) {
	if (!m_frmRoot || !m_frmRoot->GetIsShow() || !m_edtSearch) return;

	static CTimeWork searchTime(200);
	const char* cur = m_edtSearch->GetCaption();
	if (!cur) cur = "";

	if (m_lastSearch != cur && searchTime.IsTimeOut(dwTime)) {
		m_lastSearch = cur;
		int tabIdx = (m_listPage ? m_listPage->GetIndex() : 0);
		if (tabIdx == 1)
			m_monsterSearch = cur;
		else if (tabIdx == 2)
			m_itemSearch = cur;
		else
			m_npcSearch = cur;

		if (tabIdx == 1)
			LoadMonsterDatabase(cur);
		else if (tabIdx == 2)
			LoadItemDatabase(cur);
		else
			Show(m_strMapName.c_str(), true);
	}
}

// ---------------------------------------------------------------------------
// Show / Toggle
// ---------------------------------------------------------------------------

void CKnowledgeBase::Toggle() {
	if (!m_frmRoot) return;
	Show(m_strMapName.c_str(), !m_frmRoot->GetIsShow());
}

void CKnowledgeBase::Show(const char* /*mapName*/, bool bShow) {
	if (!m_frmRoot) return;

	// Items tab — skip list population
	if (m_listPage && m_listPage->GetIndex() == 2) {
		m_frmRoot->SetIsShow(bShow);
		return;
	}

	// Resolve display name for the current map
	string strCurMap = g_pGameApp->GetCurScene()->GetTerrainName();
	if      (strCurMap == "garner")     strCurMap = RES_STRING(CL_LANGUAGE_MATCH_56);
	else if (strCurMap == "magicsea")   strCurMap = RES_STRING(CL_LANGUAGE_MATCH_57);
	else if (strCurMap == "darkblue")   strCurMap = RES_STRING(CL_LANGUAGE_MATCH_58);
	else if (strCurMap == "winterland") strCurMap = RES_STRING(CL_LANGUAGE_MATCH_59);
	else if (strCurMap == "jialebi")    strCurMap = "Pirate's Base";
	m_strMapName = strCurMap;

	const char* keyword = (m_edtSearch ? m_edtSearch->GetCaption() : "");
	if (!keyword) keyword = "";
	std::string kwLower(keyword);
	std::transform(kwLower.begin(), kwLower.end(), kwLower.begin(), ::tolower);

	auto matchKeyword = [&kwLower](const char* name) -> bool {
		if (kwLower.empty()) return true;
		if (!name) return false;
		std::string n(name);
		std::transform(n.begin(), n.end(), n.begin(), ::tolower);
		return n.find(kwLower) != std::string::npos;
	};

	int tabIdx = (m_listPage ? m_listPage->GetIndex() : 0);

	// Monster tab: use CharacterInfo virtual scroll
	if (tabIdx == 1) {
		LoadMonsterDatabase(keyword);
		m_frmRoot->SetIsShow(bShow);
		return;
	}

	if (!m_lstCurrList) { m_frmRoot->SetIsShow(bShow); return; }

	CListItems* items = m_lstCurrList->GetItems();
	items->Clear();
	m_lstCurrList->Refresh();

	// NPC tab: use NPCHelper / NPCList
	if (!NPCHelper::I()) {
		NPCHelper* h = new NPCHelper(0, 1000);
		h->LoadRawDataInfo("scripts/table/NPCList", FALSE);
	}

	auto trimRight = [](const char* s) -> std::string {
		std::string str(s);
		while (!str.empty() && str.back() == ' ') str.pop_back();
		return str;
	};

	bool bSearchAll = (keyword[0] != '\0');
	int nTotalIndex = NPCHelper::I()->GetLastID() + 1;
	for (int i = 1; i < nTotalIndex; ++i) {
		NPCData* p = GetNPCDataInfo(i);
		if (!p) continue;
		if (!bSearchAll && trimRight(p->szMapName) != m_strMapName) continue;
		if (!matchKeyword(p->szName)) continue;

		std::string strName = p->szName;
		while (strName.length() < 32) strName += " ";
		std::string strArea = std::string(p->szArea);
		while (strArea.length() < 16) strArea += " ";

		char buff[1024];
		sprintf(buff, "%s%s(%d,%d)", strName.c_str(), strArea.c_str(), p->dwxPos0, p->dwyPos0);
		m_lstCurrList->Add(buff);
		m_lstCurrList->Refresh();
	}

	m_frmRoot->SetIsShow(bShow);
}

// ---------------------------------------------------------------------------
// Monster database
// ---------------------------------------------------------------------------

void CKnowledgeBase::LoadMonsterDatabase(const char* keyword) {
	m_vMonsterData.clear();
	m_monScrollPos = 0;

	if (!keyword) keyword = "";
	std::string kwLower(keyword);
	std::transform(kwLower.begin(), kwLower.end(), kwLower.begin(), ::tolower);

	CChaRecordSet* pSet = CChaRecordSet::I();
	if (!pSet) { UpdateMonsterListDisplay(); return; }

	std::set<std::string> seen;
	int lastID = pSet->GetLastID();
	for (int i = 1; i <= lastID; i++) {
		CChaRecord* pCha = GetChaRecordInfo(i);
		if (!pCha) continue;
		if (pCha->chCtrlType != enumCHACTRL_MONS) continue;
		if (!kwLower.empty()) {
			std::string n(pCha->szName);
			std::transform(n.begin(), n.end(), n.begin(), ::tolower);
			if (n.find(kwLower) == std::string::npos) continue;
		}
		char key[48];
		sprintf(key, "%s|%d", pCha->szName, pCha->lLv);
		if (seen.count(key)) continue;
		seen.insert(key);
		m_vMonsterData.push_back(i);
	}

	int total = (int)m_vMonsterData.size();
	if (m_monScroll) {
		float fMax = (float)std::max(0, total - MON_VISIBLE_COUNT);
		m_monScroll->SetRange(0.0f, fMax);
		m_monScroll->Reset();
		m_monScroll->SetIsShow(total > MON_VISIBLE_COUNT);
	}

	UpdateMonsterListDisplay();
}

void CKnowledgeBase::UpdateMonsterListDisplay() {
	for (int i = 0; i < MON_VISIBLE_COUNT; i++) {
		if (m_monNames[i])   m_monNames[i]->SetCaption("");
		if (m_monLevels[i])  m_monLevels[i]->SetCaption("");
		if (m_monHPs[i])     m_monHPs[i]->SetCaption("");
		if (m_monButtons[i]) m_monButtons[i]->SetIsShow(false);
	}

	int total = (int)m_vMonsterData.size();
	for (int i = 0; i < MON_VISIBLE_COUNT; i++) {
		int dataIdx = m_monScrollPos + i;
		if (dataIdx >= total) break;

		CChaRecord* pCha = GetChaRecordInfo(m_vMonsterData[dataIdx]);
		if (!pCha) continue;

		if (m_monNames[i])   m_monNames[i]->SetCaption(pCha->szName);
		if (m_monLevels[i])  m_monLevels[i]->SetCaption(std::to_string(pCha->lLv).c_str());
		if (m_monHPs[i])     m_monHPs[i]->SetCaption(std::to_string(pCha->lMxHp).c_str());
		if (m_monButtons[i]) m_monButtons[i]->SetIsShow(true);
	}
}

void CKnowledgeBase::OnMonScrollChange() {
	if (m_monScroll) {
		m_monScrollPos = (int)m_monScroll->GetStep().GetPosition();
		UpdateMonsterListDisplay();
	}
}

void CKnowledgeBase::OnMonInfoClick(int rowIdx) {
	int dataIdx = m_monScrollPos + rowIdx;
	if (dataIdx < 0 || dataIdx >= (int)m_vMonsterData.size()) return;
	g_stUIStart.ShowMonsterInfoByCharId(m_vMonsterData[dataIdx]);
}

// ---------------------------------------------------------------------------
// Mouse wheel
// ---------------------------------------------------------------------------

bool CKnowledgeBase::HandleMouseScroll(int nScroll) {
	if (!m_frmRoot || !m_frmRoot->GetIsShow()) return false;
	if (!m_frmRoot->InRect(CGuiData::GetMouseX(), CGuiData::GetMouseY())) return false;
	int tabIdx = (m_listPage ? m_listPage->GetIndex() : 0);
	if (tabIdx == 1) {
		if ((int)m_vMonsterData.size() <= MON_VISIBLE_COUNT) return false;
		if (m_monScroll) return m_monScroll->MouseScroll(nScroll);
		return false;
	}
	if (tabIdx == 2) {
		if ((int)m_vItemData.size() <= ITEM_VISIBLE_COUNT) return false;
		if (m_itemScroll) return m_itemScroll->MouseScroll(nScroll);
		return false;
	}
	return false;
}

// ---------------------------------------------------------------------------
// Item database
// ---------------------------------------------------------------------------

void CKnowledgeBase::LoadItemDatabase(const char* keyword) {
	m_vItemData.clear();
	m_itemScrollPos = 0;

	if (!keyword) keyword = "";
	std::string kwLower(keyword);
	std::transform(kwLower.begin(), kwLower.end(), kwLower.begin(), ::tolower);

	CItemRecordSet* pSet = CItemRecordSet::I();
	if (!pSet) return;

	int lastID = pSet->GetLastID();
	for (int id = 0; id <= lastID; id++) {
		CItemRecord* pItem = GetItemRecordInfo(id);
		if (!pItem) continue;
		if (!kwLower.empty()) {
			std::string nameL(pItem->szName);
			std::transform(nameL.begin(), nameL.end(), nameL.begin(), ::tolower);
			if (nameL.find(kwLower) == std::string::npos) continue;
		}
		m_vItemData.push_back(id);
	}

	int total = (int)m_vItemData.size();
	if (m_itemScroll) {
		float fMax = (float)std::max(0, total - ITEM_VISIBLE_COUNT);
		m_itemScroll->SetRange(0.0f, fMax);
		m_itemScroll->Reset();
		m_itemScroll->SetIsShow(total > ITEM_VISIBLE_COUNT);
	}

	UpdateItemListDisplay();
}

void CKnowledgeBase::UpdateItemListDisplay() {
	for (int i = 0; i < ITEM_VISIBLE_COUNT; i++) {
		if (m_itemSlots[i]) m_itemSlots[i]->DelCommand();
		if (m_itemNames[i]) m_itemNames[i]->SetCaption("");
		if (m_itemDescs[i]) m_itemDescs[i]->SetCaption("");
	}

	int total = (int)m_vItemData.size();
	for (int i = 0; i < ITEM_VISIBLE_COUNT; i++) {
		int dataIdx = m_itemScrollPos + i;
		if (dataIdx >= total) break;

		CItemRecord* pItem = GetItemRecordInfo(m_vItemData[dataIdx]);
		if (!pItem) continue;

		if (m_itemSlots[i]) {
			CItemCommand* cmd = new CItemCommand(pItem);
			m_itemSlots[i]->AddCommand(cmd);
			m_itemSlots[i]->SetIsEnabled(false);
		}
		if (m_itemNames[i]) m_itemNames[i]->SetCaption(pItem->szName);
		if (m_itemDescs[i]) {
			char desc[61] = {0};
			strncpy(desc, pItem->szDescriptor, 60);
			m_itemDescs[i]->SetCaption(desc);
		}
	}
}

void CKnowledgeBase::OnItemScrollChange() {
	if (m_itemScroll) {
		m_itemScrollPos = (int)m_itemScroll->GetStep().GetPosition();
		UpdateItemListDisplay();
	}
}

// ---------------------------------------------------------------------------
// Static callbacks
// ---------------------------------------------------------------------------

void CKnowledgeBase::_evtScrollChange(CGuiData* pSender) {
	g_stKnowledgeBase.OnItemScrollChange();
}

void CKnowledgeBase::_evtMonScrollChange(CGuiData* pSender) {
	g_stKnowledgeBase.OnMonScrollChange();
}

void CKnowledgeBase::_evtMonInfoClick(CGuiData* pSender, int /*x*/, int /*y*/, DWORD /*key*/) {
	for (int i = 0; i < MON_VISIBLE_COUNT; i++) {
		if (pSender == g_stKnowledgeBase.m_monButtons[i]) {
			g_stKnowledgeBase.OnMonInfoClick(i);
			return;
		}
	}
}

void CKnowledgeBase::_evtPageChange(CGuiData* pSender) {
	int index = g_stKnowledgeBase.m_listPage->GetIndex();

	if (index == 0) {
		NPCHelper* h = NPCHelper::I(); SAFE_DELETE(h);
		h = new NPCHelper(0, 1000);
		h->LoadRawDataInfo("scripts/table/NPCList", FALSE);
		g_stKnowledgeBase.m_lstCurrList = g_stKnowledgeBase.m_lstNpcList;
		if (g_stKnowledgeBase.m_edtSearch) {
			g_stKnowledgeBase.m_edtSearch->SetCaption(g_stKnowledgeBase.m_npcSearch.c_str());
			g_stKnowledgeBase.m_lastSearch = g_stKnowledgeBase.m_npcSearch;
		}
		if (g_stKnowledgeBase.m_itemScroll) g_stKnowledgeBase.m_itemScroll->SetIsShow(false);
		if (g_stKnowledgeBase.m_monScroll)  g_stKnowledgeBase.m_monScroll->SetIsShow(false);
		g_stKnowledgeBase.Show(g_stKnowledgeBase.GetCurrMapName(), true);

	} else if (index == 1) {
		NPCHelper* h = NPCHelper::I(); SAFE_DELETE(h);
		g_stKnowledgeBase.m_lstCurrList = nullptr;
		if (g_stKnowledgeBase.m_edtSearch) {
			g_stKnowledgeBase.m_edtSearch->SetCaption(g_stKnowledgeBase.m_monsterSearch.c_str());
			g_stKnowledgeBase.m_lastSearch = g_stKnowledgeBase.m_monsterSearch;
		}
		if (g_stKnowledgeBase.m_itemScroll) g_stKnowledgeBase.m_itemScroll->SetIsShow(false);
		g_stKnowledgeBase.LoadMonsterDatabase(g_stKnowledgeBase.m_monsterSearch.c_str());
		g_stKnowledgeBase.m_frmRoot->SetIsShow(true);

	} else if (index == 2) {
		g_stKnowledgeBase.m_lstCurrList = nullptr;
		if (g_stKnowledgeBase.m_edtSearch) {
			g_stKnowledgeBase.m_edtSearch->SetCaption(g_stKnowledgeBase.m_itemSearch.c_str());
			g_stKnowledgeBase.m_lastSearch = g_stKnowledgeBase.m_itemSearch;
		}
		if (g_stKnowledgeBase.m_monScroll) g_stKnowledgeBase.m_monScroll->SetIsShow(false);
		g_stKnowledgeBase.LoadItemDatabase(g_stKnowledgeBase.m_itemSearch.c_str());
	}
}

void CKnowledgeBase::_evtListChange(CGuiData* pSender) {
	if (!g_stKnowledgeBase.m_lstCurrList) return;

	CItemRow* itemrow = g_stKnowledgeBase.m_lstCurrList->GetSelectItem();
	if (!itemrow) return;
	CItemObj* itemobj = itemrow->GetBegin();
	if (!itemobj) return;

	std::string itemstring(itemobj->GetString());

	int pos  = (int)itemstring.find("(") - 1;
	int pos1 = (int)itemstring.find("(", pos);
	int pos2 = (int)itemstring.find(",", pos);
	int pos3 = (int)itemstring.find(")", pos);

	if (pos1 >= 0 && pos2 > pos1 && pos3 > pos2 && pos3 <= (int)itemstring.length()) {
		string xStr = itemstring.substr(pos1 + 1, pos2 - pos1 - 1);
		string yStr = itemstring.substr(pos2 + 1, pos3 - pos2 - 1);
		g_stUIMap.ShowRadar(xStr.c_str(), yStr.c_str());
	}

	g_stKnowledgeBase.Show(g_stKnowledgeBase.GetCurrMapName(), false);
}
