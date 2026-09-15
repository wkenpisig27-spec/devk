#include "StdAfx.h"
#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiHotbarForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Property.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <windows.h>

extern void RmlHotbar_OnClick(int slotIndex, bool rightClick);
extern void RmlHotbar_OnPage(int delta);
extern void RmlHotbar_OnDropBag(int slotIndex, int bagIndex);
extern void RmlHotbar_OnDropSkill(int slotIndex, int skillId);
extern void RmlHotbar_OnDropSlot(int dstSlot, int srcSlot);
extern void RmlHotbar_OnHint(int slotIndex, int mouseX, int mouseY);

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

std::string Fingerprint(const std::vector<RmlHotbarSlotView>& slots) {
	std::string fp;
	char buf[96];
	for (const auto& s : slots) {
		sprintf_s(buf, ";%d:%d:%d:%s:", s.index, s.qty, s.dimmed ? 1 : 0, s.keyLabel.c_str());
		fp += buf;
		fp += s.iconPath;
	}
	return fp;
}

bool ResolveHotSlot(Rml::Element* el, int& slot) {
	slot = -1;
	while (el) {
		if (el->HasAttribute("data-fast")) {
			slot = el->GetAttribute("data-fast", -1);
			return slot >= 0;
		}
		el = el->GetParentNode();
	}
	return false;
}

} // namespace

struct CRmlUiHotbarForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	std::string itemFp;
	std::string skillFp;
	int hoverSlot = -1;

	void ProcessEvent(Rml::Event& event) override;
	void BindControls();
	Rml::Element* Get(const char* id) const;
	void FillRow(Rml::Element* row, const std::vector<RmlHotbarSlotView>& slots);
};

CRmlUiHotbarForm::CRmlUiHotbarForm() : m_impl(new Impl) {}
CRmlUiHotbarForm::~CRmlUiHotbarForm() {
	Unload();
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiHotbarForm& CRmlUiHotbarForm::Instance() {
	static CRmlUiHotbarForm instance;
	return instance;
}

bool CRmlUiHotbarForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;
	m_impl->context = context;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->document = context->LoadDocument("hotbar.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load hotbar.rml\n");
		return false;
	}
	m_impl->BindControls();
	m_impl->document->Hide();
	OutputDebugStringA("RmlUi: hotbar.rml loaded\n");
	return true;
}

void CRmlUiHotbarForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
	m_impl->itemFp.clear();
	m_impl->skillFp.clear();
	m_impl->hoverSlot = -1;
}

bool CRmlUiHotbarForm::LoadOk() const {
	return m_impl && m_impl->document;
}

void CRmlUiHotbarForm::Impl::BindControls() {
	if (!document)
		return;
	if (Rml::Element* up = document->GetElementById("hotbar-page-up"))
		up->AddEventListener(Rml::EventId::Click, this);
	if (Rml::Element* down = document->GetElementById("hotbar-page-down"))
		down->AddEventListener(Rml::EventId::Click, this);
	document->AddEventListener(Rml::EventId::Dragdrop, this);
}

Rml::Element* CRmlUiHotbarForm::Impl::Get(const char* id) const {
	return document ? document->GetElementById(id) : nullptr;
}

void CRmlUiHotbarForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
	Layout();
}

void CRmlUiHotbarForm::Hide() {
	if (m_impl && m_impl->document)
		m_impl->document->Hide();
	if (m_impl)
		m_impl->hoverSlot = -1;
}

bool CRmlUiHotbarForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

void CRmlUiHotbarForm::Impl::FillRow(Rml::Element* row, const std::vector<RmlHotbarSlotView>& slots) {
	if (!row || !document)
		return;
	while (Rml::Element* child = row->GetLastChild())
		row->RemoveChild(child);
	for (size_t i = 0; i < slots.size(); ++i) {
		const bool last = (i + 1 == slots.size());
		Rml::ElementPtr cell = document->CreateElement("div");
		if (!cell)
			break;
		cell->SetClassNames(last ? "hotbar-cell hotbar-cell-last" : "hotbar-cell");
		cell->SetProperty("width", "30dp");

		Rml::ElementPtr slot = document->CreateElement("div");
		if (!slot)
			break;
		const RmlHotbarSlotView& view = slots[i];
		slot->SetClassNames("hotbar-slot");
		slot->SetAttribute("data-fast", view.index);
		slot->AddEventListener(Rml::EventId::Click, this);
		slot->AddEventListener(Rml::EventId::Mousedown, this);
		slot->AddEventListener(Rml::EventId::Mouseover, this);
		slot->AddEventListener(Rml::EventId::Mouseout, this);
		slot->AddEventListener(Rml::EventId::Dragstart, this);
		slot->AddEventListener(Rml::EventId::Dragdrop, this);
		slot->SetProperty("display", "block");
		slot->SetProperty("position", "relative");
		slot->SetProperty("box-sizing", "border-box");
		slot->SetProperty("width", "30dp");
		slot->SetProperty("height", "30dp");
		slot->SetProperty("min-width", "30dp");
		slot->SetProperty("min-height", "30dp");
		slot->SetProperty("padding", "3dp");
		slot->SetProperty("overflow", "visible");
		slot->SetProperty("background-color", "transparent");
		slot->SetProperty("border-top-width", "0dp");
		slot->SetProperty("border-right-width", "0dp");
		slot->SetProperty("border-bottom-width", "0dp");
		slot->SetProperty("border-left-width", "0dp");
		slot->SetProperty("decorator", "image(ui/rml/frames/hud/slot_dark.tga)");
		slot->SetProperty(Rml::PropertyId::Focus, Rml::Property(Rml::Style::Focus::None));
		slot->SetProperty("pointer-events", "auto");
		const bool filled = !view.iconPath.empty();
		slot->SetProperty(Rml::PropertyId::Drag,
						  Rml::Property(filled ? Rml::Style::Drag::Clone : Rml::Style::Drag::None));
		if (filled) {
			Rml::ElementPtr img = document->CreateElement("img");
			if (img) {
				img->SetClassNames("hotbar-slot-icon");
				img->SetAttribute("src", view.iconPath.c_str());
				img->SetProperty("display", "block");
				img->SetProperty("width", "24dp");
				img->SetProperty("height", "24dp");
				img->SetProperty("pointer-events", "none");
				if (view.dimmed)
					img->SetProperty("image-color", "#8a96a8");
				slot->AppendChild(std::move(img));
			}
			if (view.qty > 0) {
				Rml::ElementPtr qty = document->CreateElement("div");
				if (qty) {
					qty->SetClassNames("hotbar-slot-qty");
					char buf[16];
					sprintf_s(buf, "%d", view.qty);
					qty->SetInnerRML(EscapeXml(buf));
					qty->SetProperty("pointer-events", "none");
					slot->AppendChild(std::move(qty));
				}
			}
		}
		if (!view.keyLabel.empty()) {
			Rml::ElementPtr key = document->CreateElement("div");
			if (key) {
				key->SetClassNames("hotbar-key");
				key->SetInnerRML(EscapeXml(view.keyLabel.c_str()));
				key->SetProperty("pointer-events", "none");
				slot->AppendChild(std::move(key));
			}
		}
		cell->AppendChild(std::move(slot));
		row->AppendChild(std::move(cell));
	}
}

void CRmlUiHotbarForm::SetSlots(const std::vector<RmlHotbarSlotView>& items,
							   const std::vector<RmlHotbarSlotView>& skills) {
	if (!m_impl || !m_impl->document)
		return;
	const std::string itemFp = Fingerprint(items);
	const std::string skillFp = Fingerprint(skills);
	if (itemFp != m_impl->itemFp) {
		m_impl->FillRow(m_impl->Get("hotbar-items"), items);
		m_impl->itemFp = itemFp;
	}
	if (skillFp != m_impl->skillFp) {
		m_impl->FillRow(m_impl->Get("hotbar-skills"), skills);
		m_impl->skillFp = skillFp;
	}
	Layout();
}

void CRmlUiHotbarForm::Layout() {
	if (!m_impl || !m_impl->document || !m_impl->context)
		return;
	Rml::Element* root = m_impl->Get("hotbar-root");
	if (!root)
		return;
	m_impl->context->Update();
	const Rml::Vector2i dim = m_impl->context->GetDimensions();
	Rml::Vector2f size = root->GetBox().GetSize(Rml::BoxArea::Border);
	if (size.x < 1.f)
		size.x = 640.f;
	if (size.y < 1.f)
		size.y = 80.f;
	const float x = (float)dim.x * 0.5f - size.x * 0.5f;
	const float y = (float)dim.y - size.y - 8.f;
	root->SetProperty(Rml::PropertyId::Left, Rml::Property(x, Rml::Unit::PX));
	root->SetProperty(Rml::PropertyId::Top, Rml::Property(y, Rml::Unit::PX));
}

bool CRmlUiHotbarForm::ContainsScreenPoint(int x, int y) const {
	if (!IsVisible() || !m_impl)
		return false;
	Rml::Element* root = m_impl->Get("hotbar-root");
	if (!root)
		return false;
	const Rml::Vector2f off = root->GetAbsoluteOffset(Rml::BoxArea::Border);
	const Rml::Vector2f size = root->GetBox().GetSize(Rml::BoxArea::Border);
	return x >= off.x && y >= off.y && x < off.x + size.x && y < off.y + size.y;
}

int CRmlUiHotbarForm::SlotIndexAtScreenPoint(int x, int y) const {
	if (!IsVisible() || !m_impl || !m_impl->document)
		return -1;
	Rml::Element* el = m_impl->document->GetElementById("hotbar-root");
	if (!el)
		return -1;
	// Walk both rows' children.
	const char* rows[] = {"hotbar-items", "hotbar-skills"};
	for (const char* rowId : rows) {
		Rml::Element* row = m_impl->Get(rowId);
		if (!row)
			continue;
		for (int i = 0; i < row->GetNumChildren(); ++i) {
			Rml::Element* cell = row->GetChild(i);
			if (!cell)
				continue;
			for (int j = 0; j < cell->GetNumChildren(); ++j) {
				Rml::Element* slot = cell->GetChild(j);
				if (!slot || !slot->HasAttribute("data-fast"))
					continue;
				const Rml::Vector2f off = slot->GetAbsoluteOffset(Rml::BoxArea::Border);
				const Rml::Vector2f size = slot->GetBox().GetSize(Rml::BoxArea::Border);
				if (x >= off.x && y >= off.y && x < off.x + size.x && y < off.y + size.y)
					return slot->GetAttribute("data-fast", -1);
			}
		}
	}
	(void)el;
	return -1;
}

void CRmlUiHotbarForm::UpdateItemHint(int mouseX, int mouseY) {
	if (!m_impl || !IsVisible() || m_impl->hoverSlot < 0)
		return;
	RmlHotbar_OnHint(m_impl->hoverSlot, mouseX, mouseY);
}

int CRmlUiHotbarForm::GetHoverSlot() const {
	return m_impl ? m_impl->hoverSlot : -1;
}

void CRmlUiHotbarForm::Impl::ProcessEvent(Rml::Event& event) {
	Rml::Element* target = event.GetTargetElement();
	if (!target)
		return;
	const Rml::String& id = target->GetId();

	if (event == Rml::EventId::Click) {
		if (id == "hotbar-page-up") {
			RmlHotbar_OnPage(-1);
			return;
		}
		if (id == "hotbar-page-down") {
			RmlHotbar_OnPage(1);
			return;
		}
		int slot = -1;
		if (ResolveHotSlot(target, slot))
			RmlHotbar_OnClick(slot, false);
		return;
	}

	if (event == Rml::EventId::Mousedown) {
		const int button = event.GetParameter<int>("button", -1);
		if (button == 1) {
			int slot = -1;
			if (ResolveHotSlot(target, slot)) {
				RmlHotbar_OnClick(slot, true);
				event.StopPropagation();
			}
		}
		return;
	}

	if (event == Rml::EventId::Mouseover) {
		int slot = -1;
		if (ResolveHotSlot(target, slot))
			hoverSlot = slot;
		return;
	}
	if (event == Rml::EventId::Mouseout) {
		int slot = -1;
		if (ResolveHotSlot(target, slot) && hoverSlot == slot)
			hoverSlot = -1;
		return;
	}

	if (event == Rml::EventId::Dragdrop) {
		Rml::Element* dragEl = static_cast<Rml::Element*>(event.GetParameter<void*>("drag_element", nullptr));
		int dst = -1;
		if (!ResolveHotSlot(target, dst))
			return;
		int srcFast = -1;
		int srcBag = -1;
		int srcSkill = -1;
		for (Rml::Element* el = dragEl; el; el = el->GetParentNode()) {
			if (el->HasAttribute("data-fast")) {
				srcFast = el->GetAttribute("data-fast", -1);
				break;
			}
			if (el->HasAttribute("data-bag")) {
				srcBag = el->GetAttribute("data-bag", -1);
				break;
			}
			if (el->HasAttribute("data-skill")) {
				srcSkill = atoi(el->GetAttribute("data-skill", Rml::String("0")).c_str());
				break;
			}
		}
		if (srcFast >= 0 && srcFast != dst)
			RmlHotbar_OnDropSlot(dst, srcFast);
		else if (srcBag >= 0)
			RmlHotbar_OnDropBag(dst, srcBag);
		else if (srcSkill > 0)
			RmlHotbar_OnDropSkill(dst, srcSkill);
		event.StopPropagation();
	}
}
