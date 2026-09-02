#pragma once

#include <string>

namespace Rml {
class Context;
}

// In-world chat HUD. Legacy CCozeForm stays as send/receive host.
class CRmlUiChatForm {
public:
	static CRmlUiChatForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();
	bool LoadOk() const;

	void Show();
	void Hide();
	bool IsVisible() const;

	void Clear();
	void AppendLine(const std::string& display, unsigned channelColor, unsigned nameColor);
	void SetChannelLabel(const std::string& name);
	void SetInput(const std::string& text);
	std::string GetInput() const;
	void FocusInput();
	bool IsInputFocused() const;
	void SetChannelMenuOpen(bool open);
	bool IsChannelMenuOpen() const;

private:
	CRmlUiChatForm();
	~CRmlUiChatForm();
	CRmlUiChatForm(const CRmlUiChatForm&) = delete;
	CRmlUiChatForm& operator=(const CRmlUiChatForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
