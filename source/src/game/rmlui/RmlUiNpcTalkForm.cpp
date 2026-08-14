#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiNpcTalkForm.h"

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

extern void RmlNpcTalk_OnClose();
extern void RmlNpcTalk_OnOption(int kind, int index, const char* url);

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

struct CRmlUiNpcTalkForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	bool loadOk = false;
	bool hasPlaced = false;
	std::string viewFingerprint;

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
		Rml::Element* root = Get("ntalk-root");
		if (!root || !context)
			return;
		const Rml::Vector2i dim = context->GetDimensions();
		Rml::Vector2f size = root->GetBox().GetSize(Rml::BoxArea::Border);
		if (size.x < 1.f)
			size.x = 420.f;
		if (size.y < 1.f)
			size.y = 420.f;
		float x = ((float)dim.x - size.x) * 0.5f;
		float y = ((float)dim.y - size.y) * 0.42f;
		if (x < 8.f)
			x = 8.f;
		if (y < 8.f)
			y = 8.f;
		root->SetProperty(Rml::PropertyId::Left, Rml::Property(x, Rml::Unit::PX));
		root->SetProperty(Rml::PropertyId::Top, Rml::Property(y, Rml::Unit::PX));
	}

	static std::string Fingerprint(const CRmlUiNpcTalkForm::TalkView& view) {
		std::string fp = view.title;
		fp += '|';
		fp += view.npcName;
		fp += '|';
		fp += view.body;
		char buf[64];
		for (const auto& opt : view.options) {
			sprintf_s(buf, ";%d:%d:%d:%d:%d:", (int)opt.kind, opt.index, opt.emphasize ? 1 : 0, opt.muted ? 1 : 0,
					  opt.isQuest ? 1 : 0);
			fp += buf;
			fp += opt.label;
			fp += opt.url;
		}
		return fp;
	}

	void RebuildOptions(const CRmlUiNpcTalkForm::TalkView& view) {
		Rml::Element* list = Get("ntalk-options");
		if (!list || !document)
			return;
		ClearChildren(list);

		for (const auto& opt : view.options) {
			Rml::ElementPtr btn = document->CreateElement("button");
			if (!btn)
				break;

			Rml::String classes = "ntalk-option";
			if (opt.isQuest)
				classes += " ntalk-option-quest";
			else if (opt.emphasize)
				classes += " ntalk-option-gold";
			else if (opt.muted)
				classes += " ntalk-option-muted";
			btn->SetClassNames(classes);
			btn->SetAttribute("type", "button");
			btn->SetAttribute("data-kind", std::to_string((int)opt.kind));
			btn->SetAttribute("data-index", std::to_string(opt.index));
			if (!opt.url.empty())
				btn->SetAttribute("data-url", opt.url.c_str());

			btn->SetProperty("display", "block");
			btn->SetProperty("box-sizing", "border-box");
			btn->SetProperty("width", "384dp");
			btn->SetProperty("height", "36dp");
			btn->SetProperty("line-height", "36dp");
			btn->SetProperty("margin", "0dp 0dp 8dp 0dp");
			btn->SetProperty("padding", "0dp 14dp");
			btn->SetProperty("font-size", "13dp");
			btn->SetProperty("font-weight", "bold");
			btn->SetProperty("text-align", "left");

			std::string label = opt.label;
			if (opt.isQuest)
				label = std::string("[Quest] ") + label;
			btn->SetInnerRML(EscapeXml(label.c_str()));

			btn->AddEventListener(Rml::EventId::Click, this);
			list->AppendChild(std::move(btn));
		}

		const int rows = (int)view.options.size();
		const int listH = rows > 0 ? (rows * 44) : 8;
		char hBuf[32];
		sprintf_s(hBuf, "%ddp", listH);
		list->SetProperty("height", hBuf);
		list->SetProperty("min-height", hBuf);
	}

	void ProcessEvent(Rml::Event& event) override {
		if (event.GetId() != Rml::EventId::Click)
			return;
		Rml::Element* target = event.GetTargetElement();
		if (!target)
			return;

		Rml::Element* el = target;
		while (el && el != document) {
			const Rml::String& id = el->GetId();
			if (id == "btnTalkClose" || id == "btnTalkExit") {
				event.StopPropagation();
				RmlNpcTalk_OnClose();
				return;
			}
			if (el->HasAttribute("data-kind") && el->HasAttribute("data-index")) {
				const int kind = atoi(el->GetAttribute("data-kind", Rml::String("0")).c_str());
				const int index = atoi(el->GetAttribute("data-index", Rml::String("0")).c_str());
				const Rml::String url = el->GetAttribute("data-url", Rml::String(""));
				event.StopPropagation();
				RmlNpcTalk_OnOption(kind, index, url.empty() ? nullptr : url.c_str());
				return;
			}
			el = el->GetParentNode();
		}
	}
};

CRmlUiNpcTalkForm::CRmlUiNpcTalkForm() {
	m_impl = new Impl();
}

CRmlUiNpcTalkForm::~CRmlUiNpcTalkForm() {
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiNpcTalkForm& CRmlUiNpcTalkForm::Instance() {
	static CRmlUiNpcTalkForm instance;
	return instance;
}

bool CRmlUiNpcTalkForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;
	m_impl->context = context;
	m_impl->loadOk = false;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->document = context->LoadDocument("npctalk.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load npctalk.rml\n");
		return false;
	}

	static const char* kClickIds[] = {"btnTalkClose", "btnTalkExit"};
	for (const char* id : kClickIds) {
		if (Rml::Element* el = m_impl->Get(id))
			el->AddEventListener(Rml::EventId::Click, m_impl);
	}

	m_impl->document->Hide();
	m_impl->loadOk = true;
	OutputDebugStringA("RmlUi: npctalk.rml loaded\n");
	return true;
}

bool CRmlUiNpcTalkForm::LoadOk() const {
	return m_impl && m_impl->loadOk && m_impl->document;
}

void CRmlUiNpcTalkForm::Unload() {
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

void CRmlUiNpcTalkForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
	if (Rml::Element* root = m_impl->Get("ntalk-root"))
		root->SetClass("ntalk-panel-open", true);
	if (!m_impl->hasPlaced) {
		if (m_impl->context)
			m_impl->context->Update();
		m_impl->PlaceCentered();
		m_impl->hasPlaced = true;
	}
}

void CRmlUiNpcTalkForm::Hide() {
	if (!m_impl)
		return;
	if (Rml::Element* root = m_impl->Get("ntalk-root"))
		root->SetClass("ntalk-panel-open", false);
	if (m_impl->document)
		m_impl->document->Hide();
	m_impl->viewFingerprint.clear();
}

bool CRmlUiNpcTalkForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

void CRmlUiNpcTalkForm::ApplyView(const TalkView& view) {
	if (!m_impl || !m_impl->document)
		return;

	const std::string fp = Impl::Fingerprint(view);
	if (fp == m_impl->viewFingerprint && IsVisible())
		return;
	m_impl->viewFingerprint = fp;

	SetText(m_impl->Get("ntalk-title"), view.title.empty() ? "Dialogue" : view.title);
	SetText(m_impl->Get("ntalk-npc-name"), view.npcName.empty() ? "NPC" : view.npcName);
	SetText(m_impl->Get("ntalk-body"), view.body.empty() ? "..." : view.body);
	m_impl->RebuildOptions(view);

	Show();
	if (m_impl->context)
		m_impl->context->Update();
	m_impl->PlaceCentered();
}
