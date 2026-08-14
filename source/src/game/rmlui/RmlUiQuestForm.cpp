#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiQuestForm.h"

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

extern void RmlQuest_OnClose();
extern void RmlQuest_OnSelect(int misId);
extern void RmlQuest_OnAbandon();

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

} // namespace

struct CRmlUiQuestForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	bool loadOk = false;
	bool hasPlaced = false;
	std::string viewFingerprint;
	bool canAbandon = false;

	Rml::Element* Get(const char* id) const {
		return document ? document->GetElementById(id) : nullptr;
	}

	void ClearChildren(Rml::Element* parent) {
		if (!parent)
			return;
		while (Rml::Element* child = parent->GetFirstChild())
			parent->RemoveChild(child);
	}

	void PlaceDefault() {
		Rml::Element* root = Get("quest-root");
		if (!root || !context)
			return;
		const Rml::Vector2i dim = context->GetDimensions();
		float x = 48.f;
		float y = ((float)dim.y - 480.f) * 0.28f;
		if (y < 48.f)
			y = 48.f;
		root->SetProperty(Rml::PropertyId::Left, Rml::Property(x, Rml::Unit::PX));
		root->SetProperty(Rml::PropertyId::Top, Rml::Property(y, Rml::Unit::PX));
	}

	static std::string Fingerprint(const CRmlUiQuestForm::QuestView& view) {
		std::string fp = view.title;
		fp += '|';
		fp += view.body;
		char buf[64];
		sprintf_s(buf, ";%d:%d:%d:", view.selectedId, view.canAbandon ? 1 : 0, view.hasDetail ? 1 : 0);
		fp += buf;
		for (const auto& row : view.rows) {
			sprintf_s(buf, ";%d:%d:%d:", row.misId, row.statusKind, row.header ? 1 : 0);
			fp += buf;
			fp += row.name;
			fp += row.status;
		}
		for (const auto& n : view.needs)
			fp += n.text;
		for (const auto& p : view.prizes)
			fp += p.text;
		return fp;
	}

	void RebuildList(const CRmlUiQuestForm::QuestView& view) {
		Rml::Element* list = Get("quest-list");
		if (!list || !document)
			return;
		ClearChildren(list);

		int h = 8;
		for (const auto& row : view.rows) {
			if (row.header) {
				Rml::ElementPtr el = document->CreateElement("div");
				if (!el)
					break;
				el->SetClassNames("quest-cat");
				el->SetInnerRML(EscapeXml(row.name.c_str()));
				list->AppendChild(std::move(el));
				h += 30;
				continue;
			}

			Rml::ElementPtr btn = document->CreateElement("button");
			if (!btn)
				break;
			Rml::String classes = "quest-row";
			if (row.selected)
				classes += " quest-row-sel";
			if (row.statusKind == 1)
				classes += " quest-row-done";
			btn->SetClassNames(classes);
			btn->SetAttribute("type", "button");
			btn->SetAttribute("data-mis", std::to_string(row.misId));
			btn->SetProperty("display", "block");
			btn->SetProperty("width", "256dp");
			btn->SetProperty("height", "36dp");
			btn->SetProperty("line-height", "36dp");
			btn->SetProperty("margin", "0dp 0dp 6dp 0dp");
			btn->SetProperty("padding", "0dp 10dp");
			btn->SetProperty("font-size", "12dp");
			btn->SetProperty("font-weight", "bold");
			btn->SetProperty("text-align", "left");

			std::string label = row.name;
			if (!row.status.empty())
				label += "  " + row.status;
			btn->SetInnerRML(EscapeXml(label.c_str()));
			btn->AddEventListener(Rml::EventId::Click, this);
			list->AppendChild(std::move(btn));
			h += 42;
		}

		char hBuf[32];
		sprintf_s(hBuf, "%ddp", h < 40 ? 40 : h);
		list->SetProperty("height", hBuf);
		list->SetProperty("min-height", hBuf);
	}

	void RebuildNeeds(const CRmlUiQuestForm::QuestView& view) {
		Rml::Element* list = Get("qd-needs");
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

	void RebuildPrizes(const CRmlUiQuestForm::QuestView& view) {
		Rml::Element* list = Get("qd-prizes");
		if (!list || !document)
			return;
		ClearChildren(list);
		for (const auto& p : view.prizes) {
			Rml::ElementPtr el = document->CreateElement("div");
			if (!el)
				break;
			el->SetClassNames("qd-prize");
			el->SetInnerRML(EscapeXml(p.text.c_str()));
			list->AppendChild(std::move(el));
		}
	}

	void ProcessEvent(Rml::Event& event) override {
		if (event.GetId() != Rml::EventId::Click)
			return;
		Rml::Element* el = event.GetTargetElement();
		while (el && el != document) {
			const Rml::String& id = el->GetId();
			if (id == "btnQuestClose") {
				event.StopPropagation();
				RmlQuest_OnClose();
				return;
			}
			if (id == "btnQuestAbandon") {
				event.StopPropagation();
				if (canAbandon)
					RmlQuest_OnAbandon();
				return;
			}
			if (el->HasAttribute("data-mis")) {
				const int misId = atoi(el->GetAttribute("data-mis", Rml::String("0")).c_str());
				event.StopPropagation();
				RmlQuest_OnSelect(misId);
				return;
			}
			el = el->GetParentNode();
		}
	}
};

CRmlUiQuestForm::CRmlUiQuestForm() {
	m_impl = new Impl();
}

CRmlUiQuestForm::~CRmlUiQuestForm() {
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiQuestForm& CRmlUiQuestForm::Instance() {
	static CRmlUiQuestForm instance;
	return instance;
}

bool CRmlUiQuestForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;
	m_impl->context = context;
	m_impl->loadOk = false;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->document = context->LoadDocument("quest.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load quest.rml\n");
		return false;
	}
	static const char* kIds[] = {"btnQuestClose", "btnQuestAbandon"};
	for (const char* id : kIds) {
		if (Rml::Element* el = m_impl->Get(id))
			el->AddEventListener(Rml::EventId::Click, m_impl);
	}
	m_impl->document->Hide();
	m_impl->loadOk = true;
	OutputDebugStringA("RmlUi: quest.rml loaded\n");
	return true;
}

bool CRmlUiQuestForm::LoadOk() const {
	return m_impl && m_impl->loadOk && m_impl->document;
}

void CRmlUiQuestForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
	m_impl->loadOk = false;
	m_impl->hasPlaced = false;
	m_impl->viewFingerprint.clear();
}

void CRmlUiQuestForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
	if (Rml::Element* root = m_impl->Get("quest-root"))
		root->SetClass("quest-root-open", true);
	if (!m_impl->hasPlaced) {
		if (m_impl->context)
			m_impl->context->Update();
		m_impl->PlaceDefault();
		m_impl->hasPlaced = true;
	}
}

void CRmlUiQuestForm::Hide() {
	if (!m_impl)
		return;
	if (Rml::Element* root = m_impl->Get("quest-root"))
		root->SetClass("quest-root-open", false);
	if (m_impl->document)
		m_impl->document->Hide();
	m_impl->viewFingerprint.clear();
}

bool CRmlUiQuestForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

void CRmlUiQuestForm::ApplyView(const QuestView& view) {
	if (!m_impl || !m_impl->document)
		return;
	const std::string fp = Impl::Fingerprint(view);
	if (fp == m_impl->viewFingerprint && IsVisible())
		return;
	m_impl->viewFingerprint = fp;
	m_impl->canAbandon = view.canAbandon;

	SetText(m_impl->Get("qd-name"), view.hasDetail ? view.title : "Select a quest");
	SetText(m_impl->Get("qd-status"), view.status);
	SetText(m_impl->Get("qd-body"), view.body);
	m_impl->RebuildList(view);
	m_impl->RebuildNeeds(view);
	m_impl->RebuildPrizes(view);

	if (Rml::Element* needTitle = m_impl->Get("qd-need-title"))
		needTitle->SetProperty("display", view.needs.empty() ? "none" : "block");
	if (Rml::Element* prizeTitle = m_impl->Get("qd-prize-title"))
		prizeTitle->SetProperty("display", view.prizes.empty() ? "none" : "block");

	if (Rml::Element* abandon = m_impl->Get("btnQuestAbandon")) {
		abandon->SetClass("quest-btn-disabled", !view.canAbandon);
		if (view.canAbandon)
			abandon->RemoveAttribute("disabled");
		else
			abandon->SetAttribute("disabled", true);
	}

	Show();
	if (m_impl->context)
		m_impl->context->Update();
	if (!m_impl->hasPlaced)
		m_impl->PlaceDefault();
}
