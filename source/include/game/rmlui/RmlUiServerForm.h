#pragma once

#include <string>
#include <vector>

namespace Rml {
class Context;
}

// RmlUi replacement for legacy CFormMgr frmServer (server group pick).
class CRmlUiServerForm {
public:
	static CRmlUiServerForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();

	void SetItems(const std::vector<std::string>& names, int selectedIndex = 0);
	void Show();
	void Hide();
	bool IsVisible() const;

private:
	CRmlUiServerForm();
	~CRmlUiServerForm();
	CRmlUiServerForm(const CRmlUiServerForm&) = delete;
	CRmlUiServerForm& operator=(const CRmlUiServerForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
