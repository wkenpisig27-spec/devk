#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiAccountForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Input.h>

#include <windows.h>

// Callbacks implemented in LoginScene.cpp (avoids including game headers here).
extern void RmlAccount_OnLogin(const char* account, const char* password, bool remember);
extern void RmlAccount_OnExit();
extern void RmlAccount_OnRegister();

struct CRmlUiAccountForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;

	void ProcessEvent(Rml::Event& event) override;
	void BindControls();
	Rml::Element* Get(const char* id) const;
	std::string GetInputValue(const char* id) const;
	bool IsChecked(const char* id) const;
	void SetInputValue(const char* id, const std::string& value);
	void SetChecked(const char* id, bool checked);
};

CRmlUiAccountForm::CRmlUiAccountForm() : m_impl(new Impl) {}
CRmlUiAccountForm::~CRmlUiAccountForm() {
	Unload();
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiAccountForm& CRmlUiAccountForm::Instance() {
	static CRmlUiAccountForm instance;
	return instance;
}

bool CRmlUiAccountForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;

	m_impl->context = context;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}

	m_impl->document = context->LoadDocument("account.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load account.rml\n");
		return false;
	}

	m_impl->BindControls();
	m_impl->document->Hide();
	OutputDebugStringA("RmlUi: account.rml loaded\n");
	return true;
}

void CRmlUiAccountForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
}

void CRmlUiAccountForm::Impl::BindControls() {
	if (!document)
		return;

	const char* click_ids[] = {"btnLogin", "btnRegister", "btnExit"};
	for (const char* id : click_ids) {
		if (Rml::Element* el = document->GetElementById(id))
			el->AddEventListener(Rml::EventId::Click, this);
	}

	if (Rml::Element* form = document->GetElementById("account-form"))
		form->AddEventListener(Rml::EventId::Submit, this);

	if (Rml::Element* pwd = document->GetElementById("edtPassword"))
		pwd->AddEventListener(Rml::EventId::Keydown, this);
	if (Rml::Element* id = document->GetElementById("edtID"))
		id->AddEventListener(Rml::EventId::Keydown, this);
}

Rml::Element* CRmlUiAccountForm::Impl::Get(const char* id) const {
	return document ? document->GetElementById(id) : nullptr;
}

std::string CRmlUiAccountForm::Impl::GetInputValue(const char* id) const {
	Rml::Element* el = Get(id);
	if (!el)
		return {};
	auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(el);
	if (!input)
		return {};
	return std::string(input->GetValue().c_str());
}

bool CRmlUiAccountForm::Impl::IsChecked(const char* id) const {
	Rml::Element* el = Get(id);
	return el && el->HasAttribute("checked");
}

void CRmlUiAccountForm::Impl::SetInputValue(const char* id, const std::string& value) {
	Rml::Element* el = Get(id);
	if (!el)
		return;
	auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(el);
	if (input)
		input->SetValue(value.c_str());
}

void CRmlUiAccountForm::Impl::SetChecked(const char* id, bool checked) {
	Rml::Element* el = Get(id);
	if (!el)
		return;
	if (checked)
		el->SetAttribute("checked", true);
	else
		el->RemoveAttribute("checked");
}

void CRmlUiAccountForm::Show(const std::string& savedAccount, bool rememberAccount) {
	if (!m_impl || !m_impl->document)
		return;

	m_impl->SetInputValue("edtID", savedAccount);
	m_impl->SetInputValue("edtPassword", "");
	m_impl->SetChecked("chkID", rememberAccount);
	m_impl->document->Show();

	if (Rml::Element* focus = savedAccount.empty() ? m_impl->Get("edtID") : m_impl->Get("edtPassword"))
		focus->Focus();
}

void CRmlUiAccountForm::Hide() {
	if (m_impl && m_impl->document)
		m_impl->document->Hide();
}

bool CRmlUiAccountForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

void CRmlUiAccountForm::Impl::ProcessEvent(Rml::Event& event) {
	Rml::Element* target = event.GetTargetElement();
	if (!target)
		return;

	const Rml::String& id = target->GetId();

	auto do_login = [this]() {
		const std::string account = GetInputValue("edtID");
		const std::string password = GetInputValue("edtPassword");
		const bool remember = IsChecked("chkID");
		RmlAccount_OnLogin(account.c_str(), password.c_str(), remember);
	};

	if (event == Rml::EventId::Submit) {
		event.StopPropagation();
		do_login();
		return;
	}

	if (event == Rml::EventId::Keydown) {
		const auto key = static_cast<Rml::Input::KeyIdentifier>(event.GetParameter<int>("key_identifier", 0));
		if (key == Rml::Input::KI_RETURN || key == Rml::Input::KI_NUMPADENTER) {
			event.StopPropagation();
			do_login();
		}
		return;
	}

	if (event == Rml::EventId::Click) {
		if (id == "btnLogin")
			do_login();
		else if (id == "btnExit")
			RmlAccount_OnExit();
		else if (id == "btnRegister")
			RmlAccount_OnRegister();
	}
}
