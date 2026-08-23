#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiGuildApplyForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Input.h>

#include <string>
#include <windows.h>

extern void RmlGuildApply_OnConfirm(const char* name, const char* password, const char* confirm);
extern void RmlGuildApply_OnCancel();

struct CRmlUiGuildApplyForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	bool loadOk = false;

	void ProcessEvent(Rml::Event& event) override;
	void BindControls();
	Rml::Element* Get(const char* id) const;
	std::string GetInputValue(const char* id) const;
	void SetInputValue(const char* id, const std::string& value);
	void Submit();
	void FocusFirstEmpty();
};

CRmlUiGuildApplyForm::CRmlUiGuildApplyForm() : m_impl(new Impl) {}
CRmlUiGuildApplyForm::~CRmlUiGuildApplyForm() {
	Unload();
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiGuildApplyForm& CRmlUiGuildApplyForm::Instance() {
	static CRmlUiGuildApplyForm instance;
	return instance;
}

bool CRmlUiGuildApplyForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;

	m_impl->context = context;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}

	m_impl->document = context->LoadDocument("guildapply.rml");
	if (!m_impl->document) {
		m_impl->loadOk = false;
		OutputDebugStringA("RmlUi: failed to load guildapply.rml\n");
		return false;
	}

	m_impl->BindControls();
	m_impl->document->Hide();
	m_impl->loadOk = true;
	OutputDebugStringA("RmlUi: guildapply.rml loaded\n");
	return true;
}

void CRmlUiGuildApplyForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
	m_impl->loadOk = false;
}

bool CRmlUiGuildApplyForm::LoadOk() const {
	return m_impl && m_impl->loadOk && m_impl->document;
}

void CRmlUiGuildApplyForm::Impl::BindControls() {
	if (!document)
		return;

	const char* click_ids[] = {"btnConfirm", "btnCancel"};
	for (const char* id : click_ids) {
		if (Rml::Element* el = document->GetElementById(id))
			el->AddEventListener(Rml::EventId::Click, this);
	}

	if (Rml::Element* form = document->GetElementById("guildapply-form"))
		form->AddEventListener(Rml::EventId::Submit, this);

	const char* input_ids[] = {"edtName", "edtPCode", "edtPCode2"};
	for (const char* id : input_ids) {
		if (Rml::Element* el = document->GetElementById(id))
			el->AddEventListener(Rml::EventId::Keydown, this);
	}

	document->AddEventListener(Rml::EventId::Keydown, this);
}

Rml::Element* CRmlUiGuildApplyForm::Impl::Get(const char* id) const {
	return document ? document->GetElementById(id) : nullptr;
}

std::string CRmlUiGuildApplyForm::Impl::GetInputValue(const char* id) const {
	Rml::Element* el = Get(id);
	if (!el)
		return {};
	auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(el);
	if (!input)
		return {};
	return std::string(input->GetValue().c_str());
}

void CRmlUiGuildApplyForm::Impl::SetInputValue(const char* id, const std::string& value) {
	Rml::Element* el = Get(id);
	if (!el)
		return;
	auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(el);
	if (input)
		input->SetValue(value.c_str());
}

void CRmlUiGuildApplyForm::Impl::Submit() {
	const std::string name = GetInputValue("edtName");
	const std::string password = GetInputValue("edtPCode");
	const std::string confirm = GetInputValue("edtPCode2");
	RmlGuildApply_OnConfirm(name.c_str(), password.c_str(), confirm.c_str());
}

void CRmlUiGuildApplyForm::Impl::FocusFirstEmpty() {
	const char* focusId = "edtName";
	if (!GetInputValue("edtName").empty())
		focusId = GetInputValue("edtPCode").empty() ? "edtPCode" : "edtPCode2";
	if (Rml::Element* focus = Get(focusId))
		focus->Focus();
}

void CRmlUiGuildApplyForm::Show(bool resetFields) {
	if (!m_impl || !m_impl->document)
		return;

	if (resetFields) {
		m_impl->SetInputValue("edtName", "");
		m_impl->SetInputValue("edtPCode", "");
		m_impl->SetInputValue("edtPCode2", "");
	}

	m_impl->document->Show();
	m_impl->FocusFirstEmpty();
}

void CRmlUiGuildApplyForm::Hide() {
	if (m_impl && m_impl->document)
		m_impl->document->Hide();
}

void CRmlUiGuildApplyForm::ClearName() {
	if (m_impl)
		m_impl->SetInputValue("edtName", "");
}

void CRmlUiGuildApplyForm::ClearPasswords() {
	if (m_impl) {
		m_impl->SetInputValue("edtPCode", "");
		m_impl->SetInputValue("edtPCode2", "");
	}
}

bool CRmlUiGuildApplyForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

void CRmlUiGuildApplyForm::Impl::ProcessEvent(Rml::Event& event) {
	Rml::Element* target = event.GetTargetElement();
	if (!target)
		return;

	const Rml::String& id = target->GetId();

	if (event == Rml::EventId::Submit) {
		event.StopPropagation();
		Submit();
		return;
	}

	if (event == Rml::EventId::Keydown) {
		const auto key = static_cast<Rml::Input::KeyIdentifier>(event.GetParameter<int>("key_identifier", 0));
		if (key == Rml::Input::KI_ESCAPE) {
			event.StopPropagation();
			RmlGuildApply_OnCancel();
			return;
		}
		if (key == Rml::Input::KI_RETURN || key == Rml::Input::KI_NUMPADENTER) {
			event.StopPropagation();
			Submit();
		}
		return;
	}

	if (event == Rml::EventId::Click) {
		if (id == "btnConfirm")
			Submit();
		else if (id == "btnCancel")
			RmlGuildApply_OnCancel();
	}
}
