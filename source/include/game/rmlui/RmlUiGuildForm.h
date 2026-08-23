#pragma once

#include "rmlui/RmlUiInventoryForm.h"

#include <string>
#include <vector>

namespace Rml {
class Context;
}

// Notice guild manager (Members / Apply / Vault / Logs).
class CRmlUiGuildForm {
public:
	struct MemberRow {
		unsigned int id = 0;
		std::string name;
		std::string job;
		std::string level;
		bool selected = false;
		bool online = true;
	};

	struct GuildView {
		std::string name;
		std::string founder;
		std::string members;
		std::string exp;
		std::string gold;
		std::string level;
		std::string motto;
		bool isLeader = false;
		bool recruitEnabled = false;
		bool removeEnabled = true;
		bool rejectEnabled = false;
		bool footerVisible = true;
		int tab = 0;
		int sortCol = 0;
		bool sortAsc = true;
		std::vector<MemberRow> membersList;
		std::vector<MemberRow> applyList;
		std::vector<std::string> logs;
		int logPage = 1;
		bool logPrevEnabled = false;
		bool logNextEnabled = true;
		bool vaultLocked = true;
		std::string vaultGold;
	};

	static CRmlUiGuildForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();
	bool LoadOk() const;

	void Show();
	void Hide();
	bool IsVisible() const;
	bool IsModalOpen() const;
	void HideModal();

	void ApplyView(const GuildView& view);
	void SetVaultSlots(const std::vector<RmlInvSlotView>& slots, int columns);
	void SetGold(const char* gold);

	int GetActiveTab() const;
	unsigned int GetSelectedMemberId() const;
	unsigned int GetSelectedApplyId() const;
	std::string GetMottoInput() const;
	std::string GetMottoEditInput() const;
	unsigned int GetPermissionMask() const;

	void SetSelectedMember(unsigned int id);
	void SetSelectedApply(unsigned int id);
	void SetPermissionMask(unsigned int mask, const char* memberName);
	void ShowPermissions(bool show);
	void ShowMottoEdit(const char* motto);

	bool ContainsScreenPoint(int x, int y) const;
	bool ContainsVaultPoint(int x, int y) const;

	void UpdateItemHint(int mouseX, int mouseY);
	void RenderItemHint();

private:
	CRmlUiGuildForm();
	~CRmlUiGuildForm();
	CRmlUiGuildForm(const CRmlUiGuildForm&) = delete;
	CRmlUiGuildForm& operator=(const CRmlUiGuildForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
