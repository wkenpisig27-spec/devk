#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiSelectChaForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>

#include <windows.h>

extern void RmlSelectCha_OnCreate();
extern void RmlSelectCha_OnEnter();
extern void RmlSelectCha_OnDelete();
extern void RmlSelectCha_OnExit();
extern void RmlSelectCha_OnAlter();
extern void RmlSelectCha_OnChangePass();

struct CRmlUiSelectChaForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;

	void ProcessEvent(Rml::Event& event) override;
	void BindControls();
	Rml::Element* Get(const char* id) const;
	bool IsButtonEnabled(Rml::Element* el) const;
};

CRmlUiSelectChaForm::CRmlUiSelectChaForm() : m_impl(new Impl) {}
CRmlUiSelectChaForm::~CRmlUiSelectChaForm() {
	Unload();
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiSelectChaForm& CRmlUiSelectChaForm::Instance() {
	static CRmlUiSelectChaForm instance;
	return instance;
}

bool CRmlUiSelectChaForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;

	m_impl->context = context;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}

	m_impl->document = context->LoadDocument("selectcha.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load selectcha.rml\n");
		return false;
	}

	m_impl->BindControls();
	m_impl->document->Hide();
	OutputDebugStringA("RmlUi: selectcha.rml loaded\n");
	return true;
}

void CRmlUiSelectChaForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
}

void CRmlUiSelectChaForm::Impl::BindControls() {
	if (!document)
		return;

	const char* click_ids[] = {
		"btnCreate", "btnDel", "btnYes", "btnNo", "btnAlter", "btnChangePass"};
	for (const char* id : click_ids) {
		if (Rml::Element* el = document->GetElementById(id))
			el->AddEventListener(Rml::EventId::Click, this);
	}
}

Rml::Element* CRmlUiSelectChaForm::Impl::Get(const char* id) const {
	return document ? document->GetElementById(id) : nullptr;
}

bool CRmlUiSelectChaForm::Impl::IsButtonEnabled(Rml::Element* el) const {
	if (!el)
		return false;
	if (el->HasAttribute("disabled"))
		return false;
	return !el->IsClassSet("notice-btn-disabled");
}

void CRmlUiSelectChaForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
}

void CRmlUiSelectChaForm::Hide() {
	if (m_impl && m_impl->document)
		m_impl->document->Hide();
}

bool CRmlUiSelectChaForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

void CRmlUiSelectChaForm::SetButtonEnabled(const char* id, bool enabled) {
	if (!m_impl || !m_impl->document || !id)
		return;

	Rml::Element* el = m_impl->Get(id);
	if (!el)
		return;

	el->SetClass("notice-btn-disabled", !enabled);
	if (enabled)
		el->RemoveAttribute("disabled");
	else
		el->SetAttribute("disabled", true);
}

void CRmlUiSelectChaForm::Impl::ProcessEvent(Rml::Event& event) {
	Rml::Element* target = event.GetTargetElement();
	if (!target || event != Rml::EventId::Click)
		return;

	Rml::Element* el = target;
	while (el && el != document) {
		const Rml::String& id = el->GetId();
		if (id.empty()) {
			el = el->GetParentNode();
			continue;
		}

		if (!IsButtonEnabled(el))
			return;

		if (id == "btnCreate") {
			RmlSelectCha_OnCreate();
			return;
		}
		if (id == "btnYes") {
			RmlSelectCha_OnEnter();
			return;
		}
		if (id == "btnDel") {
			RmlSelectCha_OnDelete();
			return;
		}
		if (id == "btnNo") {
			RmlSelectCha_OnExit();
			return;
		}
		if (id == "btnAlter") {
			RmlSelectCha_OnAlter();
			return;
		}
		if (id == "btnChangePass") {
			RmlSelectCha_OnChangePass();
			return;
		}

		el = el->GetParentNode();
	}
}
