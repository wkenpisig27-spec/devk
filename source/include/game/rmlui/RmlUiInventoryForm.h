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
	bool iconDimmed = false; // exhausted fairy / unusable tint via image-color
	bool selected = false;	 // multi-select highlight (Ctrl-click)
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
	bool GetRootScreenRect(float& x, float& y, float& w, float& h) const;

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

	// In-place multi-select chrome (no slot rebuild — safe during mousedown/drag).
	void ApplyBagSelection(const std::vector<char>& selectedByIndex);

	void SetPackageLocked(bool locked);
	void SetEquipModeApparel(bool apparel);
	bool IsEquipModeApparel() const;

	// Notice confirm overlays (lock inventory / expand bag slots / item actions).
	void ShowLockConfirmModal();
	void ShowExpandBagModal(int currentCapacity, int addSlots, int costImp);
	void ShowConfirmModal(const char* title, const char* message, const char* hint = "");
	void HideModals();
	bool IsModalOpen() const;

	// Notice right-click context menu (replaces legacy CMenu while inventory is open).
	struct CtxMenuFlags {
		bool throwItem = false;
		bool deleteItem = false;
		bool lockItem = false;
		bool unlockItem = false;
		bool sellItem = false;
		bool depositItem = false; // personal bank open
		bool boxRates = false;
		bool sendToChat = false;
	};
	void ShowContextMenu(int screenX, int screenY, const CtxMenuFlags& flags);
	void HideContextMenu();
	bool IsContextMenuOpen() const;

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
