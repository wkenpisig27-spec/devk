#pragma once

#include <string>
#include <vector>

namespace Rml {
class Context;
}

struct RmlInvSlotView {
	int id = -1;				 // bag index or equip link
	std::string iconPath;		 // empty = empty slot
	std::string placeholderPath; // greyed silhouette when empty (equip only)
	int qty = 0;
	bool locked = false; // capacity lock placeholder
};

// Notice-inspired inventory overlay (Character + Backpack + category tabs).
// Data is pushed from CEquipMgr; clicks/right-clicks call back into CEquipMgr.
class CRmlUiInventoryForm {
public:
	static CRmlUiInventoryForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();

	void Show();
	void Hide();
	bool IsVisible() const;
	bool IsItemDragging() const;

	// Rebuild legacy CTextHint after FormMgr clears it; render after RmlUi.
	void UpdateItemHint(int mouseX, int mouseY);
	void RenderItemHint();
	void RenderChaPreview();

	void SetCurrency(const char* gold, const char* imp);
	void SetCapacity(int used, int unlocked);
	void SetBagSlots(const std::vector<RmlInvSlotView>& slots, int columns);
	void SetEquipSlots(const std::vector<RmlInvSlotView>& left,
					   const std::vector<RmlInvSlotView>& right,
					   const std::vector<RmlInvSlotView>& bottom);

	void SetPackageLocked(bool locked);
	void SetEquipModeApparel(bool apparel);
	bool IsEquipModeApparel() const;

	// Current category filter requested by UI (all/equipment/consumable/material/other).
	const char* GetFilter() const;

private:
	CRmlUiInventoryForm();
	~CRmlUiInventoryForm();
	CRmlUiInventoryForm(const CRmlUiInventoryForm&) = delete;
	CRmlUiInventoryForm& operator=(const CRmlUiInventoryForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
