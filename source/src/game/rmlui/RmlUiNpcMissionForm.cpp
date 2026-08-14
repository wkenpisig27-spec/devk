#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiNpcMissionForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Property.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <windows.h>

extern void RmlNpcMis_OnClose();
extern void RmlNpcMis_OnAccept();
extern void RmlNpcMis_OnComplete();
extern void RmlNpcMis_OnPrize(int index);

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
		case '\n': out += "<br/>"; break;
		default: out += *p; break;
		}
	}
	return out;
}

void SetText(Rml::Element* el, const std::string& text) {
	if (!el)
		return;
	el->SetInnerRML(EscapeXml(text.c_str()));
}

void SetHidden(Rml::Element* el, bool hidden) {
	if (!el)
		return;
	el->SetClass("nmis-hidden", hidden);
	el->SetProperty("display", hidden ? "none" : "block");
}

} // namespace

struct CRmlUiNpcMissionForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	bool loadOk = false;
	bool hasPlaced = false;
	std::string viewFingerprint;
	int selectedPrize = -1;
	bool prizePick = false;

	Rml::Element* Get(const char* id) const {
		return document ? document->GetElementById(id) : nullptr;
	}

	void ClearChildren(Rml::Element* parent) {
		if (!parent)
			return;
		while (Rml::Element* child = parent->GetFirstChild())
			parent->RemoveChild(child);
	}

	void PlaceCentered() {
		Rml::Element* root = Get("nmis-root");
		if (!root || !context)
			return;
		const Rml::Vector2i dim = context->GetDimensions();
		Rml::Vector2f size = root->GetBox().GetSize(Rml::BoxArea::Border);
		if (size.x < 1.f)
			size.x = 420.f;
		if (size.y < 1.f)
			size.y = 420.f;
		float x = ((float)dim.x - size.x) * 0.5f;
		float y = ((float)dim.y - size.y) * 0.38f;
		if (x < 8.f)
			x = 8.f;
		if (y < 8.f)
			y = 8.f;
		root->SetProperty(Rml::PropertyId::Left, Rml::Property(x, Rml::Unit::PX));
		root->SetProperty(Rml::PropertyId::Top, Rml::Property(y, Rml::Unit::PX));
	}

	static std::string Fingerprint(const CRmlUiNpcMissionForm::MissionView& view) {
		std::string fp = view.title;
		fp += '|';
		fp += view.body;
		char buf[32];
		sprintf_s(buf, ";%d%d%d:", view.showAccept ? 1 : 0, view.showComplete ? 1 : 0, view.completeEnabled ? 1 : 0);
		fp += buf;
		for (const auto& n : view.needs)
			fp += n.text;
		for (const auto& p : view.prizes) {
			fp += p.text;
			fp += p.selected ? "1" : "0";
		}
		return fp;
	}

	void RebuildNeeds(const CRmlUiNpcMissionForm::MissionView& view) {
		Rml::Element* list = Get("nmis-needs");
		if (!list || !document)
			return;
		ClearChildren(list);
		for (const auto& n : view.needs) {
			Rml::ElementPtr el = document->CreateElement("div");
			if (!el)
				break;
			el->SetClassNames(n.done ? "qd-need qd-need-done" : "qd-need");
			el->SetInnerRML(EscapeXml(n.text.c_str()));
			list->AppendChild(std::move(el));
		}
	}

	void RebuildPrizes(const CRmlUiNpcMissionForm::MissionView& view) {
		Rml::Element* list = Get("nmis-prizes");
		if (!list || !document)
			return;
		ClearChildren(list);
		for (const auto& p : view.prizes) {
			Rml::ElementPtr el = document->CreateElement("button");
			if (!el)
				break;
			Rml::String classes = "qd-prize";
			if (p.selected)
				classes += " qd-prize-sel";
			el->SetClassNames(classes);
			el->SetAttribute("type", "button");
			el->SetAttribute("data-prize", std::to_string(p.index));
			el->SetProperty("display", "block");
			el->SetProperty("width", "384dp");
			el->SetProperty("height", "32dp");
			el->SetProperty("line-height", "32dp");
			el->SetInnerRML(EscapeXml(p.text.c_str()));
			if (p.selectable)
				el->AddEventListener(Rml::EventId::Click, this);
			list->AppendChild(std::move(el));
		}
	}

	void ProcessEvent(Rml::Event& event) override {
		if (event.GetId() != Rml::EventId::Click)
			return;
		Rml::Element* el = event.GetTargetElement();
		while (el && el != document) {
			const Rml::String& id = el->GetId();
			if (id == "btnNmisClose" || id == "btnNmisExit") {
				event.StopPropagation();
				RmlNpcMis_OnClose();
				return;
			}
			if (id == "btnNmisAccept") {
				event.StopPropagation();
				RmlNpcMis_OnAccept();
				return;
			}
			if (id == "btnNmisComplete") {
				event.StopPropagation();
				RmlNpcMis_OnComplete();
				return;
			}
			if (el->HasAttribute("data-prize")) {
				const int index = atoi(el->GetAttribute("data-prize", Rml::String("0")).c_str());
				event.StopPropagation();
				selectedPrize = index;
				RmlNpcMis_OnPrize(index);
				return;
			}
			el = el->GetParentNode();
		}
	}
};

CRmlUiNpcMissionForm::CRmlUiNpcMissionForm() {
	m_impl = new Impl();
}

CRmlUiNpcMissionForm::~CRmlUiNpcMissionForm() {
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiNpcMissionForm& CRmlUiNpcMissionForm::Instance() {
	static CRmlUiNpcMissionForm instance;
	return instance;
}

bool CRmlUiNpcMissionForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;
	m_impl->context = context;
	m_impl->loadOk = false;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->document = context->LoadDocument("npcmission.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load npcmission.rml\n");
		return false;
	}
	static const char* kIds[] = {"btnNmisClose", "btnNmisExit", "btnNmisAccept", "btnNmisComplete"};
	for (const char* id : kIds) {
		if (Rml::Element* el = m_impl->Get(id))
			el->AddEventListener(Rml::EventId::Click, m_impl);
	}
	m_impl->document->Hide();
	m_impl->loadOk = true;
	OutputDebugStringA("RmlUi: npcmission.rml loaded\n");
	return true;
}

bool CRmlUiNpcMissionForm::LoadOk() const {
	return m_impl && m_impl->loadOk && m_impl->document;
}

void CRmlUiNpcMissionForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
	m_impl->loadOk = false;
	m_impl->hasPlaced = false;
	m_impl->selectedPrize = -1;
	m_impl->viewFingerprint.clear();
}

void CRmlUiNpcMissionForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
	if (Rml::Element* root = m_impl->Get("nmis-root"))
		root->SetClass("nmis-panel-open", true);
	if (!m_impl->hasPlaced) {
		if (m_impl->context)
			m_impl->context->Update();
		m_impl->PlaceCentered();
		m_impl->hasPlaced = true;
	}
}

void CRmlUiNpcMissionForm::Hide() {
	if (!m_impl)
		return;
	if (Rml::Element* root = m_impl->Get("nmis-root"))
		root->SetClass("nmis-panel-open", false);
	if (m_impl->document)
		m_impl->document->Hide();
	m_impl->viewFingerprint.clear();
	m_impl->selectedPrize = -1;
}

bool CRmlUiNpcMissionForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

int CRmlUiNpcMissionForm::GetSelectedPrize() const {
	return m_impl ? m_impl->selectedPrize : -1;
}

void CRmlUiNpcMissionForm::ApplyView(const MissionView& view) {
	if (!m_impl || !m_impl->document)
		return;
	const std::string fp = Impl::Fingerprint(view);
	if (fp == m_impl->viewFingerprint && IsVisible())
		return;
	m_impl->viewFingerprint = fp;
	m_impl->prizePick = view.prizePick;
	if (!view.prizePick)
		m_impl->selectedPrize = 0;
	else {
		m_impl->selectedPrize = -1;
		for (const auto& p : view.prizes) {
			if (p.selected)
				m_impl->selectedPrize = p.index;
		}
	}

	SetText(m_impl->Get("nmis-title"), "Quest");
	SetText(m_impl->Get("nmis-name"), view.title.empty() ? "Quest" : view.title);
	SetText(m_impl->Get("nmis-body"), view.body);
	m_impl->RebuildNeeds(view);
	m_impl->RebuildPrizes(view);

	if (Rml::Element* needTitle = m_impl->Get("nmis-need-title"))
		needTitle->SetProperty("display", view.needs.empty() ? "none" : "block");
	if (Rml::Element* prizeTitle = m_impl->Get("nmis-prize-title"))
		prizeTitle->SetProperty("display", view.prizes.empty() ? "none" : "block");

	SetHidden(m_impl->Get("btnNmisAccept"), !view.showAccept);
	SetHidden(m_impl->Get("btnNmisComplete"), !view.showComplete);
	if (Rml::Element* complete = m_impl->Get("btnNmisComplete")) {
		complete->SetClass("quest-btn-disabled", !view.completeEnabled);
		if (view.completeEnabled)
			complete->RemoveAttribute("disabled");
		else
			complete->SetAttribute("disabled", true);
	}

	Show();
	if (m_impl->context)
		m_impl->context->Update();
	m_impl->PlaceCentered();
	m_impl->hasPlaced = true;
}
