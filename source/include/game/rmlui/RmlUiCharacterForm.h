#pragma once

#include <string>

namespace Rml {
class Context;
}

// Notice character / attributes overlay. Legacy frmState stays as data host only.
class CRmlUiCharacterForm {
public:
	struct StateView {
		std::string name;
		std::string job;
		std::string guild;
		std::string level;
		std::string exp;
		std::string hp;
		std::string sp;
		std::string skillPts;
		std::string statPts;
		std::string str;
		std::string agi;
		std::string con;
		std::string sta;
		std::string dex;
		std::string minAtk;
		std::string maxAtk;
		std::string hit;
		std::string flee;
		std::string def;
		std::string aspeed;
		std::string pdef;
		std::string mspeed;
		std::string fame;
		std::string battle;
		float expPct = 0.f;
		float hpPct = 0.f;
		float spPct = 0.f;
		bool showAllocate = false;
	};

	static CRmlUiCharacterForm& Instance();

	bool Load(Rml::Context* context);
	void Unload();
	bool LoadOk() const;

	void Show();
	void Hide();
	bool IsVisible() const;

	void ApplyState(const StateView& state);
	void SetBattlePoints(const char* value);

private:
	CRmlUiCharacterForm();
	~CRmlUiCharacterForm();
	CRmlUiCharacterForm(const CRmlUiCharacterForm&) = delete;
	CRmlUiCharacterForm& operator=(const CRmlUiCharacterForm&) = delete;

	struct Impl;
	Impl* m_impl;
};
