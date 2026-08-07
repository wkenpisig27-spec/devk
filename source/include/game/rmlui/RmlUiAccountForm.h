#pragma once

#include <string>

namespace Rml {
class Context;
}

// RmlUi replacement for the old CFormMgr frmAccount login window.
// Header stays free of RmlUi includes so LoginScene can use it safely.
class CRmlUiAccountForm {
public:
	static CRmlUiAccountForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();

	void Show(const std::string& savedAccount, bool rememberAccount);
	void Hide();
	bool IsVisible() const;

private:
	CRmlUiAccountForm();
	~CRmlUiAccountForm();
	CRmlUiAccountForm(const CRmlUiAccountForm&) = delete;
	CRmlUiAccountForm& operator=(const CRmlUiAccountForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
