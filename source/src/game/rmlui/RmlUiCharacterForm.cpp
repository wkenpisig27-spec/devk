#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiCharacterForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Property.h>

#include <cstdio>
#include <cstring>
#include <cmath>
#include <windows.h>

extern void RmlCharacter_OnClose();
extern void RmlCharacter_OnAllocate(const char* btnId);

namespace {

Rml::String EscapeXml(const char* text) {
	Rml::String out;
	if (!text)
		return out;
	for (const char* p = text; *p; ++p) {
		switch (*p) {
		case '&': out += "&amp;"; break;
		case '<': out += "&lt;"; break;
		case '>': out += "&gt;"; break;
		case '"': out += "&quot;"; break;
		default: out += *p; break;
		}
	}
	return out;
}

void SetTextCached(Rml::Element* el, std::string& cache, const std::string& text) {
	if (!el || cache == text)
		return;
	cache = text;
	el->SetInnerRML(EscapeXml(text.c_str()));
}

void SetBarPct(Rml::Element* el, float& cache, float pct) {
	if (!el)
		return;
	if (pct < 0.f)
		pct = 0.f;
	if (pct > 100.f)
		pct = 100.f;
	// Quantize to 0.5% to avoid tiny float flicker.
	const float q = floorf(pct * 2.f + 0.5f) * 0.5f;
	if (cache == q)
		return;
	cache = q;
	el->SetProperty(Rml::PropertyId::Width, Rml::Property(q, Rml::Unit::PERCENT));
}

void SetPlusVisible(Rml::Element* el, bool visible) {
	if (!el)
		return;
	el->SetClass("char-plus-visible", visible);
}

} // namespace

struct CRmlUiCharacterForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	bool loadOk = false;
	bool hasPlacedRoot = false;
	CRmlUiCharacterForm::StateView lastState;
	bool hasLastState = false;

	Rml::Element* Get(const char* id) const {
		return document ? document->GetElementById(id) : nullptr;
	}

	void ProcessEvent(Rml::Event& event) override {
		if (event.GetId() != Rml::EventId::Click)
			return;
		Rml::Element* target = event.GetTargetElement();
		while (target) {
			const Rml::String& id = target->GetId();
			if (!id.empty()) {
				if (id == "btnClose") {
					RmlCharacter_OnClose();
					return;
				}
				if (id == "btnStr" || id == "btnAgi" || id == "btnCon" || id == "btnSta" || id == "btnDex") {
					RmlCharacter_OnAllocate(id.c_str());
					return;
				}
			}
			target = target->GetParentNode();
		}
	}
};

CRmlUiCharacterForm::CRmlUiCharacterForm() {
	m_impl = new Impl();
}

CRmlUiCharacterForm::~CRmlUiCharacterForm() {
	Unload();
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiCharacterForm& CRmlUiCharacterForm::Instance() {
	static CRmlUiCharacterForm instance;
	return instance;
}

bool CRmlUiCharacterForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;
	m_impl->context = context;
	m_impl->loadOk = false;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->document = context->LoadDocument("character.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load character.rml\n");
		return false;
	}

	static const char* kClickIds[] = {
		"btnClose", "btnStr", "btnAgi", "btnCon", "btnSta", "btnDex"};
	for (const char* id : kClickIds) {
		if (Rml::Element* el = m_impl->Get(id))
			el->AddEventListener(Rml::EventId::Click, m_impl);
	}

	m_impl->document->Hide();
	m_impl->loadOk = true;
	OutputDebugStringA("RmlUi: character.rml loaded\n");
	return true;
}

bool CRmlUiCharacterForm::LoadOk() const {
	return m_impl && m_impl->loadOk && m_impl->document;
}

void CRmlUiCharacterForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
	m_impl->loadOk = false;
	m_impl->hasPlacedRoot = false;
	m_impl->hasLastState = false;
	m_impl->lastState = StateView();
}

void CRmlUiCharacterForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
	if (!m_impl->hasPlacedRoot) {
		if (Rml::Element* root = m_impl->Get("char-root")) {
			root->SetProperty(Rml::PropertyId::Left, Rml::Property(40.f, Rml::Unit::PX));
			root->SetProperty(Rml::PropertyId::Top, Rml::Property(80.f, Rml::Unit::PX));
		}
		m_impl->hasPlacedRoot = true;
	}
}

void CRmlUiCharacterForm::Hide() {
	if (!m_impl)
		return;
	if (m_impl->document)
		m_impl->document->Hide();
	// Force a full apply next show so cached strings cannot go stale.
	m_impl->hasLastState = false;
}

bool CRmlUiCharacterForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

void CRmlUiCharacterForm::ApplyState(const StateView& state) {
	if (!m_impl || !m_impl->document)
		return;

	StateView& last = m_impl->lastState;
	const bool first = !m_impl->hasLastState;
	const std::string guildText = state.guild.empty() ? "—" : state.guild;

	SetTextCached(m_impl->Get("labName"), last.name, state.name);
	SetTextCached(m_impl->Get("labJob"), last.job, state.job);
	SetTextCached(m_impl->Get("labGuild"), last.guild, guildText);
	SetTextCached(m_impl->Get("labLevel"), last.level, state.level);
	SetTextCached(m_impl->Get("labExp"), last.exp, state.exp);
	SetTextCached(m_impl->Get("labHP"), last.hp, state.hp);
	SetTextCached(m_impl->Get("labSP"), last.sp, state.sp);
	SetTextCached(m_impl->Get("labSkillPts"), last.skillPts, state.skillPts);
	SetTextCached(m_impl->Get("labStatPts"), last.statPts, state.statPts);
	SetTextCached(m_impl->Get("labStr"), last.str, state.str);
	SetTextCached(m_impl->Get("labAgi"), last.agi, state.agi);
	SetTextCached(m_impl->Get("labCon"), last.con, state.con);
	SetTextCached(m_impl->Get("labSta"), last.sta, state.sta);
	SetTextCached(m_impl->Get("labDex"), last.dex, state.dex);
	SetTextCached(m_impl->Get("labMinAtk"), last.minAtk, state.minAtk);
	SetTextCached(m_impl->Get("labMaxAtk"), last.maxAtk, state.maxAtk);
	SetTextCached(m_impl->Get("labHit"), last.hit, state.hit);
	SetTextCached(m_impl->Get("labFlee"), last.flee, state.flee);
	SetTextCached(m_impl->Get("labDef"), last.def, state.def);
	SetTextCached(m_impl->Get("labAspeed"), last.aspeed, state.aspeed);
	SetTextCached(m_impl->Get("labPdef"), last.pdef, state.pdef);
	SetTextCached(m_impl->Get("labMspeed"), last.mspeed, state.mspeed);
	SetTextCached(m_impl->Get("labFame"), last.fame, state.fame);
	SetTextCached(m_impl->Get("labBattle"), last.battle, state.battle);

	SetBarPct(m_impl->Get("barExp"), last.expPct, state.expPct);
	SetBarPct(m_impl->Get("barHP"), last.hpPct, state.hpPct);
	SetBarPct(m_impl->Get("barSP"), last.spPct, state.spPct);

	if (first || last.showAllocate != state.showAllocate) {
		SetPlusVisible(m_impl->Get("btnStr"), state.showAllocate);
		SetPlusVisible(m_impl->Get("btnAgi"), state.showAllocate);
		SetPlusVisible(m_impl->Get("btnCon"), state.showAllocate);
		SetPlusVisible(m_impl->Get("btnSta"), state.showAllocate);
		SetPlusVisible(m_impl->Get("btnDex"), state.showAllocate);
		last.showAllocate = state.showAllocate;
	}

	m_impl->hasLastState = true;
}

void CRmlUiCharacterForm::SetBattlePoints(const char* value) {
	if (!m_impl)
		return;
	const std::string text = value ? value : "0";
	SetTextCached(m_impl->Get("labBattle"), m_impl->lastState.battle, text);
}
