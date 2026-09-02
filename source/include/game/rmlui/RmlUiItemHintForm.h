#pragma once

#include <string>

namespace Rml {
class Context;
}

namespace GUI {
class CCommandObj;
class CItemCommand;
}

// Notice item-hint card. Replaces the dark CTextHint overlay for items.
class CRmlUiItemHintForm {
public:
	static CRmlUiItemHintForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();
	bool LoadOk() const;

	void Show();
	void Hide();
	bool IsVisible() const;

	bool TryShow(GUI::CCommandObj* cmd, int mouseX, int mouseY);
	void PlaceNearCursor();
	bool ConsumeShownThisFrame();

private:
	CRmlUiItemHintForm();
	~CRmlUiItemHintForm();
	CRmlUiItemHintForm(const CRmlUiItemHintForm&) = delete;
	CRmlUiItemHintForm& operator=(const CRmlUiItemHintForm&) = delete;

	struct Impl;
	Impl* m_impl;
};

bool RmlItemHint_TryShow(GUI::CCommandObj* cmd, int x, int y);
void RmlItemHint_Hide();
bool RmlItemHint_IsShowing();
