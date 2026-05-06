#pragma once
#include "UIGlobalVar.h"
#include "uipage.h"
#include "UICheckBox.h"
#include "UITextButton.h"
#include <string>
#include <vector>

namespace GUI {

class CKnowledgeBase : public CUIInterface {
public:
	void Show(const char* mapName, bool bShow = true);
	void Toggle();
	const char* GetCurrMapName() const { return m_strMapName.c_str(); }
	bool HandleMouseScroll(int nScroll);
	bool IsAutoPath() const { return m_chkID && m_chkID->GetIsChecked(); }
	void OnItemScrollChange();
	void OnMonScrollChange();
	void OnMonInfoClick(int rowIdx);

protected:
	virtual bool Init() override;
	virtual void FrameMove(DWORD dwTime) override;

private:
	CForm*     m_frmRoot    {nullptr};
	CPage*     m_listPage   {nullptr};
	CEdit*     m_edtSearch  {nullptr};
	CCheckBox* m_chkID      {nullptr};
	std::string m_strMapName;

	// NPC list (tab 0 only — monster tab uses virtual scroll)
	CList* m_lstNpcList  {nullptr};
	CList* m_lstCurrList {nullptr};

	// Per-tab search state
	std::string m_lastSearch;
	std::string m_npcSearch;
	std::string m_monsterSearch;
	std::string m_itemSearch;

	// Monster virtual-scroll (tab 1)
	static const int MON_VISIBLE_COUNT = 16;
	CLabelEx*    m_monNames[MON_VISIBLE_COUNT]   {};
	CLabelEx*    m_monLevels[MON_VISIBLE_COUNT]  {};
	CLabelEx*    m_monHPs[MON_VISIBLE_COUNT]     {};
	CTextButton* m_monButtons[MON_VISIBLE_COUNT] {};
	CScroll*     m_monScroll    {nullptr};
	int          m_monScrollPos {0};
	std::vector<int> m_vMonsterData;

	void LoadMonsterDatabase(const char* keyword = "");
	void UpdateMonsterListDisplay();

	// Item database virtual-scroll (tab 2)
	static const int ITEM_VISIBLE_COUNT = 7;
	COneCommand* m_itemSlots[ITEM_VISIBLE_COUNT] {};
	CLabelEx*    m_itemNames[ITEM_VISIBLE_COUNT] {};
	CLabelEx*    m_itemDescs[ITEM_VISIBLE_COUNT] {};
	CScroll*     m_itemScroll {nullptr};
	std::vector<int> m_vItemData;
	int m_itemScrollPos {0};

	void LoadItemDatabase(const char* keyword = "");
	void UpdateItemListDisplay();

	// Static event callbacks
	static void _evtPageChange(CGuiData* pSender);
	static void _evtListChange(CGuiData* pSender);
	static void _evtScrollChange(CGuiData* pSender);
	static void _evtMonScrollChange(CGuiData* pSender);
	static void _evtMonInfoClick(CGuiData* pSender, int x, int y, DWORD key);
	static bool _onMouseScroll(int& nScroll);
	static bool _onMonMouseScroll(int& nScroll);
};

} // namespace GUI

extern GUI::CKnowledgeBase g_stKnowledgeBase;
