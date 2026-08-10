#pragma once

#include "rmlui/RmlUiInventoryForm.h"

#include <string>
#include <vector>

namespace Rml {
class Context;
}

// Notice personal bank overlay. Data stays on legacy grdNPCstorage (data host).
class CRmlUiBankForm {
public:
	static CRmlUiBankForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();
	bool LoadOk() const;

	void Show();
	void Hide();
	bool IsVisible() const;

	void SetOwnerName(const char* name);
	void SetCapacity(int used, int unlocked);
	void SetSlots(const std::vector<RmlInvSlotView>& slots, int columns);

	bool ContainsScreenPoint(int x, int y) const;
	void PlaceBesideInventory();

private:
	CRmlUiBankForm();
	~CRmlUiBankForm();
	CRmlUiBankForm(const CRmlUiBankForm&) = delete;
	CRmlUiBankForm& operator=(const CRmlUiBankForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
