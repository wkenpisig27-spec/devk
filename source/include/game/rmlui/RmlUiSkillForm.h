#pragma once

#include <string>
#include <vector>

namespace Rml {
class Context;
}

// Notice skill overlay. Legacy frmSkill stays as data host (CSkillList sync).
class CRmlUiSkillForm {
public:
	struct RowView {
		int skillId = 0;
		std::string iconPath;
		std::string name;
		std::string meta;
		bool canUpgrade = false;
		bool selected = false;
	};

	struct DetailStat {
		std::string label;
		std::string value;
		bool highlightNext = false;
	};

	struct DetailView {
		bool visible = false;
		int skillId = 0;
		std::string iconPath;
		std::string name;
		std::string levelText;
		std::vector<std::string> tags;
		std::vector<bool> tagGold; // parallel to tags; gold capsule vs blue
		std::string description;
		std::string effect;
		std::string cost;
		std::vector<DetailStat> stats;
		std::string nextTitle;
		std::string nextBody;
		std::string nextReq;
		bool canUpgrade = false;
	};

	struct SkillView {
		std::string fightPts;
		std::string lifePts;
		std::string activeTab; // fight | life | sail
		std::vector<RowView> rows;
		DetailView detail;
		int scrollOffset = 0;
	};

	static CRmlUiSkillForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();
	bool LoadOk() const;

	void Show();
	void Hide();
	bool IsVisible() const;

	void ApplyView(const SkillView& view);
	const char* GetActiveTab() const;
	int GetSelectedSkillId() const;
	int GetScrollOffset() const;

	bool IsDetailVisible() const;
	void ClearSelection();

private:
	CRmlUiSkillForm();
	~CRmlUiSkillForm();
	CRmlUiSkillForm(const CRmlUiSkillForm&) = delete;
	CRmlUiSkillForm& operator=(const CRmlUiSkillForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
