#pragma once

#include <string>
#include <vector>

namespace Rml {
class Context;
}

struct RmlHotbarSlotView {
	int index = -1;
	std::string iconPath;
	std::string keyLabel;
	int qty = 0;
	bool dimmed = false;
};

// Dark HUD F-key / item shortcut bar. CFastCommand stays as data host.
class CRmlUiHotbarForm {
public:
	static CRmlUiHotbarForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();
	bool LoadOk() const;

	void Show();
	void Hide();
	bool IsVisible() const;

	void SetSlots(const std::vector<RmlHotbarSlotView>& items,
				  const std::vector<RmlHotbarSlotView>& skills);
	void Layout();
	bool ContainsScreenPoint(int x, int y) const;
	int SlotIndexAtScreenPoint(int x, int y) const;

	void UpdateItemHint(int mouseX, int mouseY);
	int GetHoverSlot() const;

private:
	CRmlUiHotbarForm();
	~CRmlUiHotbarForm();
	CRmlUiHotbarForm(const CRmlUiHotbarForm&) = delete;
	CRmlUiHotbarForm& operator=(const CRmlUiHotbarForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
