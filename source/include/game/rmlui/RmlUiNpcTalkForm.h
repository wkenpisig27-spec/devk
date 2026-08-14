#pragma once

#include <string>
#include <vector>

namespace Rml {
class Context;
}

// Notice NPC dialogue overlay. Legacy frmNPCchat stays as protocol host.
class CRmlUiNpcTalkForm {
public:
	enum class OptionKind {
		Func = 0,
		Mission = 1,
		Url = 2,
	};

	struct OptionView {
		OptionKind kind = OptionKind::Func;
		int index = 0;
		std::string label;
		std::string url; // only for Url
		bool emphasize = false; // gold (Trade / primary)
		bool muted = false;	 // soft exit-style (Nothing...)
		bool isQuest = false;
	};

	struct TalkView {
		std::string title;	 // header ("Dialogue")
		std::string npcName; // hero name
		std::string body;	 // speech text (newlines allowed)
		std::vector<OptionView> options;
	};

	static CRmlUiNpcTalkForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();
	bool LoadOk() const;

	void Show();
	void Hide();
	bool IsVisible() const;

	void ApplyView(const TalkView& view);

private:
	CRmlUiNpcTalkForm();
	~CRmlUiNpcTalkForm();
	CRmlUiNpcTalkForm(const CRmlUiNpcTalkForm&) = delete;
	CRmlUiNpcTalkForm& operator=(const CRmlUiNpcTalkForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
