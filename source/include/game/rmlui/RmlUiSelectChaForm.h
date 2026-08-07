#pragma once

namespace Rml {
class Context;
}

// RmlUi replacement for legacy CFormMgr frmSelect (character-select bottom bar).
class CRmlUiSelectChaForm {
public:
	static CRmlUiSelectChaForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();

	void Show();
	void Hide();
	bool IsVisible() const;

	void SetButtonEnabled(const char* id, bool enabled);

private:
	CRmlUiSelectChaForm();
	~CRmlUiSelectChaForm();
	CRmlUiSelectChaForm(const CRmlUiSelectChaForm&) = delete;
	CRmlUiSelectChaForm& operator=(const CRmlUiSelectChaForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
