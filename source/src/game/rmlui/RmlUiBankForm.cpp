#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiBankForm.h"
#include "rmlui/RmlUiInventoryForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Property.h>
#include <RmlUi/Core/StyleTypes.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <windows.h>

extern void RmlBank_OnClose();
extern void RmlBank_OnDblClick(int index);
extern void RmlBank_OnDrop(int srcBank, int srcBag, int dstBank, int dstBag);
extern void RmlBank_OnDragEnd(int srcBank, int mouseX, int mouseY);

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

bool ResolveBankIndex(Rml::Element* el, int& bankIndex) {
	bankIndex = -1;
	while (el) {
		if (el->HasAttribute("data-bank")) {
			bankIndex = el->GetAttribute("data-bank", -1);
			return bankIndex >= 0;
		}
		if (el->HasAttribute("data-bag") || el->HasAttribute("data-equip"))
			return false;
		el = el->GetParentNode();
	}
	return false;
}

bool ResolveBagIndex(Rml::Element* el, int& bagIndex) {
	bagIndex = -1;
	while (el) {
		if (el->HasAttribute("data-bag")) {
			bagIndex = el->GetAttribute("data-bag", -1);
			return bagIndex >= 0;
		}
		if (el->HasAttribute("data-bank") || el->HasAttribute("data-equip"))
			return false;
		el = el->GetParentNode();
	}
	return false;
}

std::string FingerprintSlots(const std::vector<RmlInvSlotView>& slots) {
	std::string out;
	out.reserve(slots.size() * 48);
	for (const RmlInvSlotView& view : slots) {
		char buf[64];
		sprintf_s(buf, "%d|%d|%d|", view.id, view.qty, view.locked ? 1 : 0);
		out += buf;
		out += view.iconPath;
		out += ';';
	}
	return out;
}

} // namespace

struct CRmlUiBankForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	bool loadOk = false;
	bool hasPlacedRoot = false;
	bool itemDragging = false;
	bool thumbDragging = false;
	float thumbDragGrabY = 0.f;
	std::string slotFingerprint;
	int lastColumns = -1;
	double lastClickTime = 0.0;
	int lastClickBank = -1;
	float lastClickX = 0.f;
	float lastClickY = 0.f;

	Rml::Element* Get(const char* id) const {
		return document ? document->GetElementById(id) : nullptr;
	}

	void ClearChildren(Rml::Element* parent) {
		if (!parent)
			return;
		while (Rml::Element* c = parent->GetFirstChild())
			parent->RemoveChild(c);
	}

	void SetScrollTop(float scrollTop) {
		Rml::Element* grid = Get("bank-grid");
		if (!grid)
			return;
		const float maxScroll = (std::max)(0.f, grid->GetScrollHeight() - grid->GetClientHeight());
		if (scrollTop < 0.f)
			scrollTop = 0.f;
		if (scrollTop > maxScroll)
			scrollTop = maxScroll;
		grid->SetScrollTop(scrollTop);
		UpdateScrollbar();
	}

	void ScrollBy(float deltaPx) {
		Rml::Element* grid = Get("bank-grid");
		if (!grid)
			return;
		SetScrollTop(grid->GetScrollTop() + deltaPx);
	}

	void UpdateScrollbar() {
		Rml::Element* grid = Get("bank-grid");
		Rml::Element* bar = Get("bank-scrollbar");
		Rml::Element* thumb = Get("bank-scroll-thumb");
		if (!grid || !bar || !thumb)
			return;

		const float viewH = grid->GetClientHeight();
		const float contentH = grid->GetScrollHeight();
		const float trackH = bar->GetClientHeight();
		if (viewH <= 0.f || contentH <= viewH + 0.5f || trackH <= 0.f) {
			thumb->SetProperty("display", "none");
			return;
		}

		thumb->SetProperty("display", "block");
		const float ratio = viewH / contentH;
		float thumbH = trackH * ratio;
		if (thumbH < 24.f)
			thumbH = 24.f;
		if (thumbH > trackH)
			thumbH = trackH;
		const float maxScroll = contentH - viewH;
		const float maxThumb = trackH - thumbH;
		const float top = (maxScroll > 0.f) ? (grid->GetScrollTop() / maxScroll) * maxThumb : 0.f;
		char hBuf[32];
		char tBuf[32];
		sprintf_s(hBuf, "%fdp", thumbH);
		sprintf_s(tBuf, "%fdp", top);
		thumb->SetProperty("height", hBuf);
		thumb->SetProperty("top", tBuf);
	}

	Rml::ElementPtr MakeSlot(const char* id, const RmlInvSlotView& view) {
		Rml::ElementPtr slot = document->CreateElement("div");
		if (!slot)
			return slot;

		Rml::String classes = "bank-slot";
		if (view.locked)
			classes += " bank-slot-locked";
		else if (view.iconPath.empty())
			classes += " bank-slot-empty";
		slot->SetClassNames(classes);
		if (id && id[0])
			slot->SetId(id);
		slot->SetAttribute("data-bank", view.id);
		slot->AddEventListener(Rml::EventId::Dblclick, this);
		slot->AddEventListener(Rml::EventId::Mousedown, this);
		slot->AddEventListener(Rml::EventId::Mouseup, this);
		slot->AddEventListener(Rml::EventId::Dragstart, this);
		slot->AddEventListener(Rml::EventId::Dragend, this);
		slot->AddEventListener(Rml::EventId::Dragdrop, this);

		slot->SetProperty("display", "block");
		slot->SetProperty("box-sizing", "border-box");
		slot->SetProperty("flex", "none");
		slot->SetProperty("width", "44dp");
		slot->SetProperty("height", "44dp");
		slot->SetProperty("min-width", "44dp");
		slot->SetProperty("min-height", "44dp");
		slot->SetProperty("max-width", "44dp");
		slot->SetProperty("max-height", "44dp");
		slot->SetProperty("padding", "4dp");
		slot->SetProperty("overflow", "visible");
		slot->SetProperty("background-color", "transparent");
		slot->SetProperty("border-top-width", "0dp");
		slot->SetProperty("border-right-width", "0dp");
		slot->SetProperty("border-bottom-width", "0dp");
		slot->SetProperty("border-left-width", "0dp");
		slot->SetProperty("decorator", view.locked
										   ? "image(ui/rml/frames/notice/slot_rounded_locked.tga)"
										   : "image(ui/rml/frames/notice/slot_rounded.tga)");
		slot->SetProperty(Rml::PropertyId::Focus, Rml::Property(Rml::Style::Focus::None));
		slot->SetProperty("pointer-events", "auto");

		const bool filled = !view.iconPath.empty() && !view.locked;
		slot->SetProperty(Rml::PropertyId::Drag,
						  Rml::Property(filled ? Rml::Style::Drag::Clone : Rml::Style::Drag::None));

		if (filled) {
			Rml::ElementPtr img = document->CreateElement("img");
			if (img) {
				img->SetClassNames("bank-slot-icon");
				img->SetAttribute("src", view.iconPath.c_str());
				img->SetProperty("display", "block");
				img->SetProperty("width", "36dp");
				img->SetProperty("height", "36dp");
				img->SetProperty("background-color", "transparent");
				img->SetProperty("pointer-events", "none");
				slot->AppendChild(std::move(img));
			}
			if (view.qty > 1) {
				Rml::ElementPtr qtyEl = document->CreateElement("div");
				if (qtyEl) {
					qtyEl->SetClassNames("bank-slot-qty");
					char buf[16];
					sprintf_s(buf, "%d", view.qty);
					qtyEl->SetInnerRML(EscapeXml(buf));
					qtyEl->SetProperty("pointer-events", "none");
					slot->AppendChild(std::move(qtyEl));
				}
			}
		} else {
			Rml::ElementPtr filler = document->CreateElement("div");
			if (filler) {
				filler->SetClassNames("bank-slot-icon");
				filler->SetProperty("display", "block");
				filler->SetProperty("width", "36dp");
				filler->SetProperty("height", "36dp");
				filler->SetProperty("background-color", "transparent");
				filler->SetProperty("pointer-events", "none");
				slot->AppendChild(std::move(filler));
			}
		}
		return slot;
	}

	bool TryManualDoubleClick(int bank, float mouseX, float mouseY) {
		const double now = Rml::GetSystemInterface() ? Rml::GetSystemInterface()->GetElapsedTime() : 0.0;
		const float dx = mouseX - lastClickX;
		const float dy = mouseY - lastClickY;
		const bool same = (bank >= 0 && bank == lastClickBank);
		const bool quick = (now - lastClickTime) < 0.40;
		const bool closeEnough = (dx * dx + dy * dy) < (10.f * 10.f);
		lastClickBank = bank;
		lastClickTime = now;
		lastClickX = mouseX;
		lastClickY = mouseY;
		if (same && quick && closeEnough && bank >= 0) {
			RmlBank_OnDblClick(bank);
			return true;
		}
		return false;
	}

	void ProcessEvent(Rml::Event& event) override {
		Rml::Element* target = event.GetTargetElement();
		if (!target || !document)
			return;

		const bool isClick = (event == Rml::EventId::Click);
		const bool isDblClick = (event == Rml::EventId::Dblclick);
		const bool isDragDrop = (event == Rml::EventId::Dragdrop);
		const bool isDragStart = (event == Rml::EventId::Dragstart);
		const bool isDragEnd = (event == Rml::EventId::Dragend);
		const bool isMouseDown = (event == Rml::EventId::Mousedown);
		const bool isMouseUp = (event == Rml::EventId::Mouseup);
		const bool isMouseMove = (event == Rml::EventId::Mousemove);
		const bool isScroll = (event == Rml::EventId::Mousescroll);

		if (isScroll) {
			const float wheel = event.GetParameter<float>("wheel_delta_y", 0.f);
			if (wheel != 0.f) {
				ScrollBy(-wheel * 48.f);
				event.StopPropagation();
			}
			return;
		}

		if (isMouseDown) {
			if (target->GetId() == "bank-scroll-thumb" || target->GetId() == "bank-scrollbar") {
				Rml::Element* thumb = Get("bank-scroll-thumb");
				Rml::Element* bar = Get("bank-scrollbar");
				if (thumb && bar) {
					thumbDragging = true;
					const float mouseY = event.GetParameter<float>("mouse_y", 0.f);
					const float thumbTop = thumb->GetAbsoluteOffset(Rml::BoxArea::Border).y;
					thumbDragGrabY = mouseY - thumbTop;
					event.StopPropagation();
				}
				return;
			}
			int bank = -1;
			if (ResolveBankIndex(target, bank) && bank >= 0) {
				const float mx = event.GetParameter<float>("mouse_x", 0.f);
				const float my = event.GetParameter<float>("mouse_y", 0.f);
				if (TryManualDoubleClick(bank, mx, my)) {
					event.StopPropagation();
					return;
				}
			}
		}

		if (isMouseMove && thumbDragging) {
			Rml::Element* bar = Get("bank-scrollbar");
			Rml::Element* grid = Get("bank-grid");
			Rml::Element* thumb = Get("bank-scroll-thumb");
			if (bar && grid && thumb) {
				const float mouseY = event.GetParameter<float>("mouse_y", 0.f);
				const float barTop = bar->GetAbsoluteOffset(Rml::BoxArea::Border).y;
				const float trackH = bar->GetClientHeight();
				const float thumbH = thumb->GetBox().GetSize(Rml::BoxArea::Border).y;
				const float maxScroll = (std::max)(0.f, grid->GetScrollHeight() - grid->GetClientHeight());
				float ratio = 0.f;
				if (trackH > thumbH)
					ratio = (mouseY - barTop - thumbDragGrabY) / (trackH - thumbH);
				if (ratio < 0.f)
					ratio = 0.f;
				if (ratio > 1.f)
					ratio = 1.f;
				SetScrollTop(ratio * maxScroll);
			}
			return;
		}

		if (isMouseUp) {
			thumbDragging = false;
			return;
		}

		if (isDragStart) {
			int bank = -1;
			if (ResolveBankIndex(target, bank))
				itemDragging = true;
			return;
		}

		if (isDragEnd) {
			itemDragging = false;
			int bank = -1;
			ResolveBankIndex(target, bank);
			const int mx = (int)event.GetParameter<float>("mouse_x", 0.f);
			const int my = (int)event.GetParameter<float>("mouse_y", 0.f);
			slotFingerprint.clear();
			RmlBank_OnDragEnd(bank, mx, my);
			return;
		}

		if (isDragDrop) {
			Rml::Element* dragEl = static_cast<Rml::Element*>(event.GetParameter<void*>("drag_element", nullptr));
			if (!dragEl)
				return;
			int srcBank = -1, srcBag = -1, dstBank = -1;
			ResolveBankIndex(dragEl, srcBank);
			ResolveBagIndex(dragEl, srcBag);
			if (!ResolveBankIndex(target, dstBank))
				return;
			if (srcBank < 0 && srcBag < 0)
				return;
			if (srcBank >= 0 && srcBank == dstBank)
				return;
			RmlBank_OnDrop(srcBank, srcBag, dstBank, -1);
			return;
		}

		if (isDblClick) {
			int bank = -1;
			if (ResolveBankIndex(target, bank) && bank >= 0) {
				RmlBank_OnDblClick(bank);
				return;
			}
		}

		if (isClick) {
			Rml::Element* el = target;
			while (el && el != document) {
				if (el->GetId() == "btnBankClose") {
					RmlBank_OnClose();
					return;
				}
				el = el->GetParentNode();
			}
		}
	}
};

CRmlUiBankForm::CRmlUiBankForm() : m_impl(new Impl) {}
CRmlUiBankForm::~CRmlUiBankForm() {
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiBankForm& CRmlUiBankForm::Instance() {
	static CRmlUiBankForm instance;
	return instance;
}

bool CRmlUiBankForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;
	m_impl->context = context;
	m_impl->loadOk = false;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->document = context->LoadDocument("bank.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load bank.rml\n");
		return false;
	}

	if (Rml::Element* el = m_impl->Get("btnBankClose"))
		el->AddEventListener(Rml::EventId::Click, m_impl);
	if (Rml::Element* grid = m_impl->Get("bank-grid"))
		grid->AddEventListener(Rml::EventId::Mousescroll, m_impl);
	if (Rml::Element* root = m_impl->Get("bank-root"))
		root->AddEventListener(Rml::EventId::Mousescroll, m_impl);
	if (Rml::Element* bar = m_impl->Get("bank-scrollbar"))
		bar->AddEventListener(Rml::EventId::Mousedown, m_impl);
	if (Rml::Element* thumb = m_impl->Get("bank-scroll-thumb"))
		thumb->AddEventListener(Rml::EventId::Mousedown, m_impl);
	if (m_impl->document) {
		m_impl->document->AddEventListener(Rml::EventId::Mousemove, m_impl);
		m_impl->document->AddEventListener(Rml::EventId::Mouseup, m_impl);
	}

	m_impl->document->Hide();
	m_impl->loadOk = true;
	OutputDebugStringA("RmlUi: bank.rml loaded\n");
	return true;
}

bool CRmlUiBankForm::LoadOk() const {
	return m_impl && m_impl->loadOk && m_impl->document;
}

void CRmlUiBankForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
	m_impl->loadOk = false;
	m_impl->hasPlacedRoot = false;
	m_impl->itemDragging = false;
	m_impl->thumbDragging = false;
	m_impl->slotFingerprint.clear();
	m_impl->lastColumns = -1;
}

void CRmlUiBankForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
	if (!m_impl->hasPlacedRoot) {
		if (m_impl->context)
			m_impl->context->Update();
		PlaceBesideInventory();
		m_impl->hasPlacedRoot = true;
	}
}

void CRmlUiBankForm::Hide() {
	if (!m_impl)
		return;
	if (m_impl->document)
		m_impl->document->Hide();
}

bool CRmlUiBankForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

void CRmlUiBankForm::SetOwnerName(const char* name) {
	if (!m_impl)
		return;
	if (Rml::Element* el = m_impl->Get("labBankOwner"))
		el->SetInnerRML(EscapeXml(name ? name : ""));
}

void CRmlUiBankForm::SetCapacity(int used, int unlocked) {
	if (!m_impl)
		return;
	char usedBuf[16];
	char maxBuf[16];
	sprintf_s(usedBuf, "%d", used);
	sprintf_s(maxBuf, "%d", unlocked);
	if (Rml::Element* el = m_impl->Get("labBankUsed"))
		el->SetInnerRML(EscapeXml(usedBuf));
	if (Rml::Element* el = m_impl->Get("labBankMax"))
		el->SetInnerRML(EscapeXml(maxBuf));
}

void CRmlUiBankForm::SetSlots(const std::vector<RmlInvSlotView>& slots, int columns) {
	if (!m_impl || !m_impl->document)
		return;
	if (m_impl->itemDragging)
		return;

	const int cols = columns > 0 ? columns : 4;
	std::string fp = FingerprintSlots(slots);
	if (cols == m_impl->lastColumns && fp == m_impl->slotFingerprint)
		return;
	m_impl->lastColumns = cols;
	m_impl->slotFingerprint = std::move(fp);

	Rml::Element* grid = m_impl->Get("bank-grid");
	if (!grid)
		return;

	const float savedScroll = grid->GetScrollTop();
	m_impl->ClearChildren(grid);

	const int rowW = cols * 44 + (cols > 0 ? (cols - 1) * 4 : 0);
	char rowWBuf[32];
	sprintf_s(rowWBuf, "%ddp", rowW);

	Rml::Element* row = nullptr;
	int col = 0;
	for (const RmlInvSlotView& view : slots) {
		if (col == 0) {
			Rml::ElementPtr rowPtr = m_impl->document->CreateElement("div");
			if (!rowPtr)
				break;
			rowPtr->SetClassNames("bank-row");
			rowPtr->SetProperty("display", "flex");
			rowPtr->SetProperty("flex-direction", "row");
			rowPtr->SetProperty("flex-wrap", "nowrap");
			rowPtr->SetProperty("align-items", "flex-start");
			rowPtr->SetProperty("width", rowWBuf);
			rowPtr->SetProperty("height", "44dp");
			rowPtr->SetProperty("margin-bottom", "4dp");
			row = grid->AppendChild(std::move(rowPtr));
		}

		char idBuf[32];
		sprintf_s(idBuf, "bank-%d", view.id);
		Rml::ElementPtr slot = m_impl->MakeSlot(idBuf, view);
		if (slot && row) {
			if (col + 1 >= cols)
				slot->SetProperty("margin", "0dp 0dp 0dp 0dp");
			else
				slot->SetProperty("margin", "0dp 4dp 0dp 0dp");
			row->AppendChild(std::move(slot));
		}
		col++;
		if (col >= cols)
			col = 0;
	}

	if (m_impl->context)
		m_impl->context->Update();
	m_impl->SetScrollTop(savedScroll);
}

bool CRmlUiBankForm::ContainsScreenPoint(int x, int y) const {
	if (!IsVisible() || !m_impl)
		return false;
	Rml::Element* root = m_impl->Get("bank-root");
	if (!root)
		return false;
	const Rml::Vector2f off = root->GetAbsoluteOffset(Rml::BoxArea::Border);
	const Rml::Vector2f size = root->GetBox().GetSize(Rml::BoxArea::Border);
	return x >= off.x && y >= off.y && x <= off.x + size.x && y <= off.y + size.y;
}

void CRmlUiBankForm::PlaceBesideInventory() {
	if (!m_impl || !m_impl->document || !m_impl->context)
		return;
	Rml::Element* root = m_impl->Get("bank-root");
	if (!root)
		return;

	const Rml::Vector2i dim = m_impl->context->GetDimensions();
	Rml::Vector2f size = root->GetBox().GetSize(Rml::BoxArea::Border);
	if (size.x < 1.f)
		size.x = 248.f;
	if (size.y < 1.f)
		size.y = 560.f;

	float x = (float)dim.x * 0.5f + 380.f;
	float y = ((float)dim.y - size.y) * 0.5f;

	float invX = 0.f, invY = 0.f, invW = 0.f, invH = 0.f;
	if (CRmlUiInventoryForm::Instance().GetRootScreenRect(invX, invY, invW, invH) && invW > 0.f) {
		x = invX + invW + 8.f;
		y = invY;
	}

	if (x + size.x > (float)dim.x - 8.f)
		x = (float)dim.x - size.x - 8.f;
	if (x < 8.f)
		x = 8.f;
	if (y < 8.f)
		y = 8.f;

	root->SetProperty(Rml::PropertyId::Left, Rml::Property(x, Rml::Unit::PX));
	root->SetProperty(Rml::PropertyId::Top, Rml::Property(y, Rml::Unit::PX));
}
