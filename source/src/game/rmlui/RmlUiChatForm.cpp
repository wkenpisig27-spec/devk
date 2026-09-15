#include "StdAfx.h"
#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiChatForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Input.h>

#include <cstdio>
#include <string>
#include <windows.h>

extern void RmlChat_OnSend();
extern void RmlChat_OnHistory(int direction);
extern void RmlChat_OnChannelPicked(const char* id);
extern void RmlChat_OnToggleChannelMenu();
extern void RmlChat_OnFace();
extern void RmlChat_OnAction();
extern void RmlChat_OnFilter();

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

void ColorToCss(unsigned color, char* buf, size_t bufSize) {
	sprintf_s(buf, bufSize, "#%02X%02X%02X",
		(color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
}

} // namespace

struct CRmlUiChatForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	int lineCount = 0;
	bool resizing = false;
	float resizeStartY = 0.f;
	float resizeStartH = 0.f;
	static const int kMaxLines = 200;
	static const int kMinLogHeight = 56;
	static const int kMaxLogHeight = 220;

	void ProcessEvent(Rml::Event& event) override;
	void BindControls();
	Rml::Element* Get(const char* id) const;
	void StickLogToBottom();
	void BeginResize(Rml::Event& event);
	void UpdateResize(Rml::Event& event);
	void EndResize();
	float MaxLogHeight() const;
};

CRmlUiChatForm::CRmlUiChatForm() : m_impl(new Impl) {}
CRmlUiChatForm::~CRmlUiChatForm() {
	Unload();
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiChatForm& CRmlUiChatForm::Instance() {
	static CRmlUiChatForm instance;
	return instance;
}

bool CRmlUiChatForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;

	m_impl->context = context;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}

	m_impl->document = context->LoadDocument("chat.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load chat.rml\n");
		return false;
	}

	m_impl->BindControls();
	m_impl->document->Hide();
	OutputDebugStringA("RmlUi: chat.rml loaded\n");
	return true;
}

void CRmlUiChatForm::Unload() {
	if (!m_impl)
		return;
	m_impl->EndResize();
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
}

bool CRmlUiChatForm::LoadOk() const {
	return m_impl && m_impl->document;
}

void CRmlUiChatForm::Impl::BindControls() {
	if (!document)
		return;

	const char* click_ids[] = {
		"chat-channel", "chat-btn-face", "chat-btn-action", "chat-btn-filter",
		"chat-ch-sight", "chat-ch-team", "chat-ch-guild",
		"chat-ch-world", "chat-ch-trade", "chat-ch-side"};
	for (const char* id : click_ids) {
		if (Rml::Element* el = document->GetElementById(id))
			el->AddEventListener(Rml::EventId::Click, this);
	}
	if (Rml::Element* input = document->GetElementById("chat-input"))
		input->AddEventListener(Rml::EventId::Keydown, this);
	if (Rml::Element* grip = document->GetElementById("chat-resize"))
		grip->AddEventListener(Rml::EventId::Mousedown, this);
	document->AddEventListener(Rml::EventId::Mousemove, this);
	document->AddEventListener(Rml::EventId::Mouseup, this);
}

Rml::Element* CRmlUiChatForm::Impl::Get(const char* id) const {
	return document ? document->GetElementById(id) : nullptr;
}

void CRmlUiChatForm::Impl::StickLogToBottom() {
	Rml::Element* log = Get("chat-log");
	if (log)
		log->SetScrollTop(log->GetScrollHeight());
}

float CRmlUiChatForm::Impl::MaxLogHeight() const {
	float cap = (float)kMaxLogHeight;
	if (context) {
		const float room = (float)context->GetDimensions().y - 48.f;
		if (room > (float)kMinLogHeight && room < cap)
			cap = room;
	}
	return cap;
}

void CRmlUiChatForm::Impl::BeginResize(Rml::Event& event) {
	Rml::Element* log = Get("chat-log");
	if (!log)
		return;
	resizing = true;
	resizeStartY = event.GetParameter<float>("mouse_y", 0.f);
	resizeStartH = log->GetBox().GetSize(Rml::BoxArea::Border).y;
	if (resizeStartH < 1.f)
		resizeStartH = (float)kMinLogHeight;
	if (document)
		document->SetProperty("pointer-events", "auto");
}

void CRmlUiChatForm::Impl::UpdateResize(Rml::Event& event) {
	Rml::Element* log = Get("chat-log");
	if (!log)
		return;
	const float mouseY = event.GetParameter<float>("mouse_y", resizeStartY);
	float height = resizeStartH + (resizeStartY - mouseY);
	if (height < (float)kMinLogHeight)
		height = (float)kMinLogHeight;
	const float cap = MaxLogHeight();
	if (height > cap)
		height = cap;
	char buf[32];
	sprintf_s(buf, "%.0fpx", height);
	log->SetProperty("height", buf);
	StickLogToBottom();
}

void CRmlUiChatForm::Impl::EndResize() {
	if (!resizing)
		return;
	resizing = false;
	if (document)
		document->SetProperty("pointer-events", "none");
}

void CRmlUiChatForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
	m_impl->StickLogToBottom();
}

void CRmlUiChatForm::Hide() {
	if (!m_impl)
		return;
	m_impl->EndResize();
	if (m_impl->document)
		m_impl->document->Hide();
}

bool CRmlUiChatForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

void CRmlUiChatForm::Clear() {
	if (!m_impl)
		return;
	Rml::Element* log = m_impl->Get("chat-log");
	if (!log)
		return;
	while (Rml::Element* child = log->GetLastChild())
		log->RemoveChild(child);
	m_impl->lineCount = 0;
}

void CRmlUiChatForm::AppendLine(const std::string& display, unsigned channelColor, unsigned nameColor) {
	if (!m_impl || !m_impl->document || display.empty())
		return;
	Rml::Element* log = m_impl->Get("chat-log");
	if (!log)
		return;

	char tagCss[16];
	char nameCss[16];
	char bodyCss[16];
	const unsigned bodyColor = (channelColor == 0) ? 0xFFE8EEF4 : channelColor;
	const unsigned nameColorCss = (nameColor == 0) ? 0xFFFFFFFF : nameColor;
	ColorToCss(0xFFF0C050, tagCss, sizeof(tagCss));
	ColorToCss(nameColorCss, nameCss, sizeof(nameCss));
	ColorToCss(bodyColor, bodyCss, sizeof(bodyCss));

	std::string tag;
	std::string rest = display;
	if (!display.empty() && display[0] == '[') {
		const size_t close = display.find(']');
		if (close != std::string::npos) {
			tag = display.substr(0, close + 1);
			rest = display.substr(close + 1);
			if (!rest.empty() && rest[0] == ' ')
				rest.erase(0, 1);
		}
	}

	std::string name;
	std::string body = rest;
	const size_t colon = rest.find(": ");
	if (colon != std::string::npos && colon < 48) {
		name = rest.substr(0, colon + 1);
		body = rest.substr(colon + 2);
	}

	Rml::String rml = "<span class=\"chat-tag\" style=\"color:";
	rml += tagCss;
	rml += ";\">";
	rml += EscapeXml(tag.c_str());
	rml += "</span> ";
	if (!name.empty()) {
		rml += "<span class=\"chat-name\" style=\"color:";
		rml += nameCss;
		rml += ";\">";
		rml += EscapeXml(name.c_str());
		rml += "</span> ";
	}
	rml += "<span class=\"chat-text\" style=\"color:";
	rml += bodyCss;
	rml += ";\">";
	rml += EscapeXml(body.c_str());
	rml += "</span>";

	Rml::ElementPtr line = m_impl->document->CreateElement("div");
	if (!line)
		return;
	line->SetClassNames("chat-line");
	line->SetInnerRML(rml);
	log->AppendChild(std::move(line));
	m_impl->lineCount++;

	while (m_impl->lineCount > Impl::kMaxLines) {
		if (Rml::Element* first = log->GetFirstChild()) {
			log->RemoveChild(first);
			m_impl->lineCount--;
		} else {
			break;
		}
	}
	m_impl->StickLogToBottom();
}

void CRmlUiChatForm::SetChannelLabel(const std::string& name) {
	if (!m_impl)
		return;
	if (Rml::Element* el = m_impl->Get("chat-channel"))
		el->SetInnerRML(EscapeXml(name.c_str()));
}

void CRmlUiChatForm::SetInput(const std::string& text) {
	if (!m_impl)
		return;
	Rml::Element* el = m_impl->Get("chat-input");
	if (!el)
		return;
	auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(el);
	if (input)
		input->SetValue(text.c_str());
}

std::string CRmlUiChatForm::GetInput() const {
	if (!m_impl)
		return {};
	Rml::Element* el = m_impl->Get("chat-input");
	if (!el)
		return {};
	auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(el);
	if (!input)
		return {};
	return std::string(input->GetValue().c_str());
}

void CRmlUiChatForm::FocusInput() {
	if (!m_impl)
		return;
	if (Rml::Element* el = m_impl->Get("chat-input"))
		el->Focus();
}

bool CRmlUiChatForm::IsInputFocused() const {
	if (!m_impl || !m_impl->context || !m_impl->document || !m_impl->document->IsVisible())
		return false;
	Rml::Element* focus = m_impl->context->GetFocusElement();
	Rml::Element* input = m_impl->Get("chat-input");
	if (!focus || !input)
		return false;
	for (Rml::Element* el = focus; el; el = el->GetParentNode()) {
		if (el == input)
			return true;
	}
	return false;
}

void CRmlUiChatForm::SetChannelMenuOpen(bool open) {
	if (!m_impl)
		return;
	if (Rml::Element* menu = m_impl->Get("chat-channel-menu"))
		menu->SetClass("chat-hidden", !open);
}

bool CRmlUiChatForm::IsChannelMenuOpen() const {
	if (!m_impl)
		return false;
	Rml::Element* menu = m_impl->Get("chat-channel-menu");
	return menu && !menu->IsClassSet("chat-hidden");
}

void CRmlUiChatForm::Impl::ProcessEvent(Rml::Event& event) {
	Rml::Element* target = event.GetTargetElement();
	if (!target)
		return;
	const Rml::String& id = target->GetId();

	if (event == Rml::EventId::Keydown) {
		const auto key = static_cast<Rml::Input::KeyIdentifier>(
			event.GetParameter<int>("key_identifier", 0));
		if (key == Rml::Input::KI_RETURN || key == Rml::Input::KI_NUMPADENTER) {
			event.StopPropagation();
			RmlChat_OnSend();
		} else if (key == Rml::Input::KI_UP) {
			event.StopPropagation();
			RmlChat_OnHistory(-1);
		} else if (key == Rml::Input::KI_DOWN) {
			event.StopPropagation();
			RmlChat_OnHistory(1);
		}
		return;
	}

	if (event == Rml::EventId::Mousedown) {
		if (event.GetParameter<int>("button", -1) != 0)
			return;
		for (Rml::Element* el = target; el; el = el->GetParentNode()) {
			if (el->GetId() == "chat-resize") {
				BeginResize(event);
				event.StopPropagation();
				return;
			}
		}
		return;
	}

	if (event == Rml::EventId::Mousemove) {
		if (resizing) {
			UpdateResize(event);
			event.StopPropagation();
		}
		return;
	}

	if (event == Rml::EventId::Mouseup) {
		if (resizing) {
			UpdateResize(event);
			EndResize();
			event.StopPropagation();
		}
		return;
	}

	if (event != Rml::EventId::Click)
		return;

	if (id == "chat-channel") {
		RmlChat_OnToggleChannelMenu();
	} else if (id == "chat-btn-face") {
		RmlChat_OnFace();
	} else if (id == "chat-btn-action") {
		RmlChat_OnAction();
	} else if (id == "chat-btn-filter") {
		RmlChat_OnFilter();
	} else if (id == "chat-ch-sight" || id == "chat-ch-team" || id == "chat-ch-guild" ||
		id == "chat-ch-world" || id == "chat-ch-trade" || id == "chat-ch-side") {
		RmlChat_OnChannelPicked(id.c_str());
	}
}
