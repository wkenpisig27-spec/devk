#pragma once

#include <string>
#include <vector>

namespace Rml {
class Context;
}

// Notice NPC quest page (accept / complete). Legacy frmNPCMission stays as host.
class CRmlUiNpcMissionForm {
public:
	struct NeedView {
		std::string text;
		bool done = false;
	};

	struct PrizeView {
		std::string text;
		std::string iconPath;
		bool selectable = false;
		bool selected = false;
		int index = 0;
	};

	struct MissionView {
		std::string title;
		std::string body;
		std::vector<NeedView> needs;
		std::vector<PrizeView> prizes;
		bool prizePick = false;
		bool showAccept = false;
		bool showComplete = false;
		bool completeEnabled = false;
	};

	static CRmlUiNpcMissionForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();
	bool LoadOk() const;

	void Show();
	void Hide();
	bool IsVisible() const;

	void ApplyView(const MissionView& view);
	int GetSelectedPrize() const;

private:
	CRmlUiNpcMissionForm();
	~CRmlUiNpcMissionForm();
	CRmlUiNpcMissionForm(const CRmlUiNpcMissionForm&) = delete;
	CRmlUiNpcMissionForm& operator=(const CRmlUiNpcMissionForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
