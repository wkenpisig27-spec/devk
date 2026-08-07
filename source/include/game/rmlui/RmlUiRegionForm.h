#pragma once

#include <string>
#include <vector>

namespace Rml {
class Context;
}

// RmlUi replacement for legacy CFormMgr frmArea (region pick).
class CRmlUiRegionForm {
public:
	static CRmlUiRegionForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();

	void SetItems(const std::vector<std::string>& names, int selectedIndex = 0);
	void Show();
	void Hide();
	bool IsVisible() const;

private:
	CRmlUiRegionForm();
	~CRmlUiRegionForm();
	CRmlUiRegionForm(const CRmlUiRegionForm&) = delete;
	CRmlUiRegionForm& operator=(const CRmlUiRegionForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
