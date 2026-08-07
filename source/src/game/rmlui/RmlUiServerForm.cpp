#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiServerForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>

#include <cstdio>
#include <windows.h>

extern void RmlServer_OnSelect(int index);
extern void RmlServer_OnBack();

static Rml::String EscapeXml(const char* text) {
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

struct CRmlUiServerForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	int selectedIndex = 0;

	void ProcessEvent(Rml::Event& event) override;
	void BindStaticControls();
	Rml::Element* Get(const char* id) const;
	void RebuildList(const std::vector<std::string>& names);
};

CRmlUiServerForm::CRmlUiServerForm() : m_impl(new Impl) {}
CRmlUiServerForm::~CRmlUiServerForm() {
	Unload();
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiServerForm& CRmlUiServerForm::Instance() {
	static CRmlUiServerForm instance;
	return instance;
}

bool CRmlUiServerForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;

	m_impl->context = context;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}

	m_impl->document = context->LoadDocument("server.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load server.rml\n");
		return false;
	}

	m_impl->BindStaticControls();
	m_impl->document->Hide();
	OutputDebugStringA("RmlUi: server.rml loaded\n");
	return true;
}

void CRmlUiServerForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
}

void CRmlUiServerForm::Impl::BindStaticControls() {
	if (!document)
		return;
	if (Rml::Element* backBtn = document->GetElementById("btnBack"))
		backBtn->AddEventListener(Rml::EventId::Click, this);
}

Rml::Element* CRmlUiServerForm::Impl::Get(const char* id) const {
	return document ? document->GetElementById(id) : nullptr;
}

void CRmlUiServerForm::Impl::RebuildList(const std::vector<std::string>& names) {
	if (!document)
		return;

	Rml::Element* list = Get("server-list");
	if (!list)
		return;

	while (Rml::Element* child = list->GetFirstChild())
		list->RemoveChild(child);

	for (int i = 0; i < (int)names.size(); ++i) {
		if (names[i].empty())
			continue;

		Rml::ElementPtr item = document->CreateElement("div");
		if (!item)
			continue;

		item->SetClassNames("notice-list-item");
		if (i == selectedIndex)
			item->SetClass("notice-list-item-active", true);

		char idBuf[32];
		sprintf_s(idBuf, "server-item-%d", i);
		item->SetId(idBuf);
		item->SetAttribute("data-index", i);
		item->SetInnerRML(EscapeXml(names[i].c_str()));
		item->AddEventListener(Rml::EventId::Click, this);

		list->AppendChild(std::move(item));
	}
}

void CRmlUiServerForm::SetItems(const std::vector<std::string>& names, int selectedIndex) {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->selectedIndex = selectedIndex;
	m_impl->RebuildList(names);
}

void CRmlUiServerForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
}

void CRmlUiServerForm::Hide() {
	if (m_impl && m_impl->document)
		m_impl->document->Hide();
}

bool CRmlUiServerForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

void CRmlUiServerForm::Impl::ProcessEvent(Rml::Event& event) {
	Rml::Element* target = event.GetTargetElement();
	if (!target || event != Rml::EventId::Click)
		return;

	Rml::Element* el = target;
	while (el && el != document) {
		const Rml::String& id = el->GetId();
		if (id == "btnBack") {
			RmlServer_OnBack();
			return;
		}
		if (el->HasAttribute("data-index")) {
			const int index = el->GetAttribute("data-index", -1);
			if (index < 0)
				return;

			selectedIndex = index;
			if (Rml::Element* list = Get("server-list")) {
				for (int i = 0; i < list->GetNumChildren(); ++i) {
					Rml::Element* child = list->GetChild(i);
					if (!child)
						continue;
					const bool active = child->GetAttribute("data-index", -1) == index;
					child->SetClass("notice-list-item-active", active);
				}
			}
			RmlServer_OnSelect(index);
			return;
		}
		el = el->GetParentNode();
	}
}
