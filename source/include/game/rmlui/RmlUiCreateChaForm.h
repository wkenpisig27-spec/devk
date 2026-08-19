#pragma once

#include <string>

namespace Rml {
class Context;
}

// RmlUi replacement for legacy frmFound (character creation panel).
class CRmlUiCreateChaForm {
public:
	static CRmlUiCreateChaForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();

	void Show();
	void Hide();
	bool IsVisible() const;

	void SetDescription(const char* text);
	void SetName(const char* name);
	void SetHairLabel(const char* text);
	void SetFaceLabel(const char* text);
	void SetActiveRace(int index);
	void SetRaceLabels(const char* name0, const char* name1, const char* name2, const char* name3);
	std::string GetName() const;

	void RenderChaPreview();

private:
	CRmlUiCreateChaForm();
	~CRmlUiCreateChaForm();
	CRmlUiCreateChaForm(const CRmlUiCreateChaForm&) = delete;
	CRmlUiCreateChaForm& operator=(const CRmlUiCreateChaForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
