#pragma once

#include <string>
#include <vector>

namespace Rml {
class Context;
}

// Notice quest log. Legacy frmMission stays as protocol host.
class CRmlUiQuestForm {
public:
	struct RowView {
		int misId = 0;
		std::string name;
		std::string status;
		int statusKind = 0; // 0 pending, 1 complete, 2 failed, 3 other
		bool selected = false;
		bool header = false;
	};

	struct NeedView {
		std::string text;
		bool done = false;
	};

	struct PrizeView {
		std::string text;
		std::string iconPath;
	};

	struct QuestView {
		std::vector<RowView> rows;
		int selectedId = 0;
		std::string title;
		std::string status;
		int statusKind = 0;
		std::string body;
		std::vector<NeedView> needs;
		std::vector<PrizeView> prizes;
		bool canAbandon = false;
		bool hasDetail = false;
	};

	static CRmlUiQuestForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();
	bool LoadOk() const;

	void Show();
	void Hide();
	bool IsVisible() const;

	void ApplyView(const QuestView& view);

private:
	CRmlUiQuestForm();
	~CRmlUiQuestForm();
	CRmlUiQuestForm(const CRmlUiQuestForm&) = delete;
	CRmlUiQuestForm& operator=(const CRmlUiQuestForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
