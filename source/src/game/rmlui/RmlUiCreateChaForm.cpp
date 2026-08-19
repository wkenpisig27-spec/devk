#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiCreateChaForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>

#include <windows.h>

extern void RmlCreateCha_OnAccept();
extern void RmlCreateCha_OnBack();
extern void RmlCreateCha_OnLeftHair();
extern void RmlCreateCha_OnRightHair();
extern void RmlCreateCha_OnLeftFace();
extern void RmlCreateCha_OnRightFace();
extern void RmlCreateCha_OnLeftRotate();
extern void RmlCreateCha_OnRightRotate();
extern void RmlCreateCha_OnSelectRace(int index);
extern void RmlCreateCha_RenderPreview(int stageX, int stageY, int stageW, int stageH);

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

std::string NormalizeDescriptionText(const char* text) {
	std::string out;
	if (!text)
		return out;
	bool last_was_space = true;
	for (const char* p = text; *p; ++p) {
		char c = *p;
		if (c == '\r' || c == '\n' || c == '\t')
			c = ' ';
		if (c == ' ') {
			if (!last_was_space && !out.empty()) {
				out += ' ';
				last_was_space = true;
			}
		} else {
			out += c;
			last_was_space = false;
		}
	}
	while (!out.empty() && out.back() == ' ')
		out.pop_back();
	return out;
}

} // namespace

struct CRmlUiCreateChaForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;

	void ProcessEvent(Rml::Event& event) override;
	void BindControls();
	Rml::Element* Get(const char* id) const;
	std::string GetInputValue(const char* id) const;
	void SetInputValue(const char* id, const std::string& value);
	void SetDescriptionText(const char* text);
	void SetLabelText(const char* id, const char* text);
	bool IsButtonEnabled(Rml::Element* el) const;
};

CRmlUiCreateChaForm::CRmlUiCreateChaForm() : m_impl(new Impl) {}
CRmlUiCreateChaForm::~CRmlUiCreateChaForm() {
	Unload();
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiCreateChaForm& CRmlUiCreateChaForm::Instance() {
	static CRmlUiCreateChaForm instance;
	return instance;
}

bool CRmlUiCreateChaForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;

	m_impl->context = context;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}

	m_impl->document = context->LoadDocument("createcha.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load createcha.rml\n");
		return false;
	}

	m_impl->BindControls();
	m_impl->document->Hide();
	OutputDebugStringA("RmlUi: createcha.rml loaded\n");
	return true;
}

void CRmlUiCreateChaForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
}

void CRmlUiCreateChaForm::Impl::BindControls() {
	if (!document)
		return;

	const char* click_ids[] = {
		"btnYes", "btnNo",
		"btnLeftHair", "btnRightHair",
		"btnLeftFace", "btnRightFace",
		"btnLeft3d", "btnRight3d",
		"btnRace0", "btnRace1", "btnRace2", "btnRace3"};
	for (const char* id : click_ids) {
		if (Rml::Element* el = document->GetElementById(id))
			el->AddEventListener(Rml::EventId::Click, this);
	}
}

Rml::Element* CRmlUiCreateChaForm::Impl::Get(const char* id) const {
	return document ? document->GetElementById(id) : nullptr;
}

std::string CRmlUiCreateChaForm::Impl::GetInputValue(const char* id) const {
	Rml::Element* el = Get(id);
	if (!el)
		return {};
	auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(el);
	if (!input)
		return {};
	return std::string(input->GetValue().c_str());
}

void CRmlUiCreateChaForm::Impl::SetInputValue(const char* id, const std::string& value) {
	Rml::Element* el = Get(id);
	if (!el)
		return;
	auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(el);
	if (input)
		input->SetValue(value.c_str());
}

void CRmlUiCreateChaForm::Impl::SetDescriptionText(const char* text) {
	Rml::Element* el = Get("memChaDescribe");
	if (!el)
		return;
	const std::string normalized = NormalizeDescriptionText(text);
	el->SetInnerRML("<p class=\"createcha-desc-text\">" + EscapeXml(normalized.c_str()) + "</p>");
}

void CRmlUiCreateChaForm::Impl::SetLabelText(const char* id, const char* text) {
	Rml::Element* el = Get(id);
	if (!el)
		return;
	el->SetInnerRML(EscapeXml(text ? text : ""));
}

bool CRmlUiCreateChaForm::Impl::IsButtonEnabled(Rml::Element* el) const {
	if (!el)
		return false;
	if (el->HasAttribute("disabled"))
		return false;
	return !el->IsClassSet("notice-btn-disabled");
}

void CRmlUiCreateChaForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
	if (Rml::Element* name = m_impl->Get("edtName"))
		name->Focus();
}

void CRmlUiCreateChaForm::Hide() {
	if (m_impl && m_impl->document)
		m_impl->document->Hide();
}

bool CRmlUiCreateChaForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

void CRmlUiCreateChaForm::SetDescription(const char* text) {
	if (m_impl)
		m_impl->SetDescriptionText(text);
}

void CRmlUiCreateChaForm::SetName(const char* name) {
	if (m_impl)
		m_impl->SetInputValue("edtName", name ? name : "");
}

void CRmlUiCreateChaForm::SetHairLabel(const char* text) {
	if (m_impl)
		m_impl->SetLabelText("labHairShow", text);
}

void CRmlUiCreateChaForm::SetFaceLabel(const char* text) {
	if (m_impl)
		m_impl->SetLabelText("labFaceShow", text);
}

void CRmlUiCreateChaForm::SetActiveRace(int index) {
	if (!m_impl || !m_impl->document || index < 0 || index > 3)
		return;
	const char* ids[] = {"btnRace0", "btnRace1", "btnRace2", "btnRace3"};
	for (int i = 0; i < 4; ++i) {
		if (Rml::Element* tab = m_impl->Get(ids[i]))
			tab->SetClass("createcha-race-tab-active", i == index);
	}
}

void CRmlUiCreateChaForm::SetRaceLabels(const char* name0, const char* name1, const char* name2, const char* name3) {
	if (!m_impl)
		return;
	auto trim = [](const char* text, const char* fallback) {
		std::string out = text ? text : fallback;
		while (!out.empty() && out.back() == ' ')
			out.pop_back();
		return out;
	};
	if (Rml::Element* el = m_impl->Get("btnRace0"))
		el->SetInnerRML(EscapeXml(trim(name0, "Lance").c_str()));
	if (Rml::Element* el = m_impl->Get("btnRace1"))
		el->SetInnerRML(EscapeXml(trim(name1, "Carsise").c_str()));
	if (Rml::Element* el = m_impl->Get("btnRace2"))
		el->SetInnerRML(EscapeXml(trim(name2, "Phyllis").c_str()));
	if (Rml::Element* el = m_impl->Get("btnRace3"))
		el->SetInnerRML(EscapeXml(trim(name3, "Ami").c_str()));
}

std::string CRmlUiCreateChaForm::GetName() const {
	return m_impl ? m_impl->GetInputValue("edtName") : std::string();
}

void CRmlUiCreateChaForm::RenderChaPreview() {
	if (!m_impl || !IsVisible() || !m_impl->document)
		return;

	Rml::Element* stage = m_impl->Get("createcha-preview-stage");
	if (!stage)
		return;

	const Rml::Vector2f off = stage->GetAbsoluteOffset(Rml::BoxArea::Content);
	const Rml::Vector2f size = stage->GetBox().GetSize(Rml::BoxArea::Content);
	if (size.x < 1.f || size.y < 1.f)
		return;

	const int sx = (int)off.x;
	const int sy = (int)off.y;
	const int sw = (int)size.x;
	const int sh = (int)size.y;
	RmlCreateCha_RenderPreview(sx, sy, sw, sh);
}

void CRmlUiCreateChaForm::Impl::ProcessEvent(Rml::Event& event) {
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

		if (id == "btnYes") {
			RmlCreateCha_OnAccept();
			return;
		}
		if (id == "btnNo") {
			RmlCreateCha_OnBack();
			return;
		}
		if (id == "btnLeftHair") {
			RmlCreateCha_OnLeftHair();
			return;
		}
		if (id == "btnRightHair") {
			RmlCreateCha_OnRightHair();
			return;
		}
		if (id == "btnLeftFace") {
			RmlCreateCha_OnLeftFace();
			return;
		}
		if (id == "btnRightFace") {
			RmlCreateCha_OnRightFace();
			return;
		}
		if (id == "btnLeft3d") {
			RmlCreateCha_OnLeftRotate();
			return;
		}
		if (id == "btnRight3d") {
			RmlCreateCha_OnRightRotate();
			return;
		}
		if (id == "btnRace0") {
			RmlCreateCha_OnSelectRace(0);
			return;
		}
		if (id == "btnRace1") {
			RmlCreateCha_OnSelectRace(1);
			return;
		}
		if (id == "btnRace2") {
			RmlCreateCha_OnSelectRace(2);
			return;
		}
		if (id == "btnRace3") {
			RmlCreateCha_OnSelectRace(3);
			return;
		}

		el = el->GetParentNode();
	}
}
