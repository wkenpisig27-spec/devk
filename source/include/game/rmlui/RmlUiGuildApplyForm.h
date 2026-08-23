#pragma once

namespace Rml {
class Context;
}

// RmlUi replacement for the guild name / password apply modal (frmName).
class CRmlUiGuildApplyForm {
public:
	static CRmlUiGuildApplyForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();
	bool LoadOk() const;

	void Show(bool resetFields);
	void Hide();
	bool IsVisible() const;
	void ClearName();
	void ClearPasswords();

private:
	CRmlUiGuildApplyForm();
	~CRmlUiGuildApplyForm();
	CRmlUiGuildApplyForm(const CRmlUiGuildApplyForm&) = delete;
	CRmlUiGuildApplyForm& operator=(const CRmlUiGuildApplyForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
