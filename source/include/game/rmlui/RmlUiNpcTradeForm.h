#pragma once

#include "rmlui/RmlUiInventoryForm.h"

#include <string>
#include <vector>

namespace Rml {
class Context;
}

struct RmlNpcSlotView {
	int id = -1; // shop slot index within page
	int page = 0;
	std::string iconPath;
	std::string priceText; // shown on slot corner
};

// Notice NPC Item Trade overlay. Legacy frmNPCtrade grids stay as data hosts.
class CRmlUiNpcTradeForm {
public:
	static CRmlUiNpcTradeForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();
	bool LoadOk() const;

	void Show();
	void Hide();
	bool IsVisible() const;

	void SetActivePage(int page);
	int GetActivePage() const;
	void SetGold(const char* gold);
	void SetSlots(const std::vector<RmlNpcSlotView>& slots, int columns);

	void ShowBuyConfirm(const char* itemName, const char* priceText, const char* iconPath);
	void ShowSellConfirm(const char* itemName, const char* priceText, const char* iconPath);
	void ShowQtyPrompt(bool isBuy, const char* itemName, const char* unitPriceText, const char* iconPath,
					   __int64 unitPrice, int maxQty);
	void HideModals();
	bool IsModalOpen() const;
	int GetQtyValue() const;

	void UpdateItemHint(int mouseX, int mouseY);
	void RenderItemHint();

	bool ContainsScreenPoint(int x, int y) const;
	void PlaceBesideInventory();

private:
	CRmlUiNpcTradeForm();
	~CRmlUiNpcTradeForm();
	CRmlUiNpcTradeForm(const CRmlUiNpcTradeForm&) = delete;
	CRmlUiNpcTradeForm& operator=(const CRmlUiNpcTradeForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
