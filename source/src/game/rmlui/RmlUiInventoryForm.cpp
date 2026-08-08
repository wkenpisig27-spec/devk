#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiInventoryForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Property.h>

#include <cstdio>
#include <cstring>
#include <windows.h>

extern void RmlInv_OnClose();
extern void RmlInv_OnBagDblClick(int index);
extern void RmlInv_OnBagRightClick(int index);
extern void RmlInv_OnEquipDblClick(int link);
extern void RmlInv_OnDrop(int srcBag, int srcEquip, int dstBag, int dstEquip);
extern void RmlInv_OnItemDragEnd();
extern void RmlInv_OnFilterChanged();
extern void RmlInv_OnToggleApparel(bool apparel);
extern void RmlInv_OnToggleLock();
extern void RmlInv_OnExpandBag();
extern void RmlInv_OnTempBag();
extern void RmlInv_ApplyItemHint(int bagIndex, int equipLink, int mouseX, int mouseY);
extern void RmlInv_RenderItemHint();
extern void RmlInv_RenderChaPreview(int centerX, int centerY);
extern void RmlInv_RotatePreviewLeft();
extern void RmlInv_RotatePreviewRight();

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

bool ResolveInvSlot(Rml::Element* el, Rml::Element* document, int& bagIndex, int& equipLink) {
	bagIndex = -1;
	equipLink = -1;
	while (el && el != document) {
		if (el->HasAttribute("data-bag")) {
			bagIndex = el->GetAttribute("data-bag", -1);
			return bagIndex >= 0;
		}
		if (el->HasAttribute("data-equip")) {
			equipLink = el->GetAttribute("data-equip", -1);
			return equipLink >= 0;
		}
		el = el->GetParentNode();
	}
	return false;
}

void AppendSlotFingerprint(std::string& out, const RmlInvSlotView& view) {
	char buf[64];
	sprintf_s(buf, "%d|%d|%d|", view.id, view.qty, view.locked ? 1 : 0);
	out += buf;
	out += view.iconPath;
	out += '|';
	out += view.placeholderPath;
	out += ';';
}

} // namespace

struct CRmlUiInventoryForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	std::string filter = "all";
	bool apparelMode = false;
	std::string lastGold;
	std::string lastImp;
	std::string lastCapacity;
	std::string bagFingerprint;
	std::string equipFingerprint;
	int lastBagColumns = -1;
	int lastLockState = -1;
	bool hasPlacedRoot = false;
	bool itemDragging = false;
	int hoverBag = -1;
	int hoverEquip = -1;

	void ProcessEvent(Rml::Event& event) override;
	void BindStaticControls();
	void CenterRoot();
	Rml::Element* Get(const char* id) const;
	void SetActiveTab(const char* id);
	void SetModeButtons(bool apparel);
	void ClearChildren(Rml::Element* parent);
	Rml::ElementPtr MakeSlot(const char* id, const RmlInvSlotView& view, const char* dataKey);
	void FillSlotColumn(Rml::Element* parent, const std::vector<RmlInvSlotView>& slots, const char* idPrefix, const char* dataKey);
	void FillEquipBottom(Rml::Element* parent, const std::vector<RmlInvSlotView>& slots);
	static std::string FingerprintSlots(const std::vector<RmlInvSlotView>& slots);
};

CRmlUiInventoryForm::CRmlUiInventoryForm() : m_impl(new Impl) {}
CRmlUiInventoryForm::~CRmlUiInventoryForm() {
	Unload();
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiInventoryForm& CRmlUiInventoryForm::Instance() {
	static CRmlUiInventoryForm instance;
	return instance;
}

bool CRmlUiInventoryForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;

	m_impl->context = context;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}

	m_impl->document = context->LoadDocument("inventory.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load inventory.rml\n");
		return false;
	}

	m_impl->BindStaticControls();
	m_impl->document->Hide();
	OutputDebugStringA("RmlUi: inventory.rml loaded\n");
	return true;
}

void CRmlUiInventoryForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
	m_impl->hasPlacedRoot = false;
	m_impl->itemDragging = false;
	m_impl->bagFingerprint.clear();
	m_impl->equipFingerprint.clear();
	m_impl->lastBagColumns = -1;
}

void CRmlUiInventoryForm::Impl::CenterRoot() {
	Rml::Element* root = Get("inv-root");
	if (!root || !context)
		return;

	const Rml::Vector2i dim = context->GetDimensions();
	Rml::Vector2f size = root->GetBox().GetSize(Rml::BoxArea::Border);
	if (size.x < 1.f)
		size.x = 828.f;
	if (size.y < 1.f)
		size.y = 520.f;

	const float left = (static_cast<float>(dim.x) - size.x) * 0.5f;
	const float top = (static_cast<float>(dim.y) - size.y) * 0.5f;
	root->SetProperty(Rml::PropertyId::Left, Rml::Property(left, Rml::Unit::PX));
	root->SetProperty(Rml::PropertyId::Top, Rml::Property(top, Rml::Unit::PX));
}

void CRmlUiInventoryForm::Impl::BindStaticControls() {
	if (!document)
		return;

	const char* clickIds[] = {
		"btnClose", "btnLock", "btnExpand", "btnTempBag",
		"btnModeEquip", "btnModeApparel",
		"btnPreviewLeft", "btnPreviewRight",
		"tabItems", "tabEquipment", "tabConsumable", "tabMaterial", "tabOther"};
	for (const char* id : clickIds) {
		if (Rml::Element* el = document->GetElementById(id))
			el->AddEventListener(Rml::EventId::Click, this);
	}
	SetModeButtons(apparelMode);
}

Rml::Element* CRmlUiInventoryForm::Impl::Get(const char* id) const {
	return document ? document->GetElementById(id) : nullptr;
}

void CRmlUiInventoryForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
	if (!m_impl->hasPlacedRoot) {
		// Layout must run once so GetBox() returns the real root size.
		if (m_impl->context)
			m_impl->context->Update();
		m_impl->CenterRoot();
		m_impl->hasPlacedRoot = true;
	}
}

void CRmlUiInventoryForm::Hide() {
	if (!m_impl)
		return;
	m_impl->hoverBag = -1;
	m_impl->hoverEquip = -1;
	if (m_impl->document)
		m_impl->document->Hide();
}

void CRmlUiInventoryForm::UpdateItemHint(int mouseX, int mouseY) {
	if (!m_impl || !IsVisible() || m_impl->itemDragging)
		return;
	if (m_impl->hoverBag < 0 && m_impl->hoverEquip < 0)
		return;
	RmlInv_ApplyItemHint(m_impl->hoverBag, m_impl->hoverEquip, mouseX, mouseY);
}

void CRmlUiInventoryForm::RenderItemHint() {
	if (!m_impl || !IsVisible() || m_impl->itemDragging)
		return;
	if (m_impl->hoverBag < 0 && m_impl->hoverEquip < 0)
		return;
	RmlInv_RenderItemHint();
}

void CRmlUiInventoryForm::RenderChaPreview() {
	if (!m_impl || !IsVisible() || !m_impl->document)
		return;
	Rml::Element* well = m_impl->Get("inv-preview");
	if (!well)
		return;

	const Rml::Vector2f off = well->GetAbsoluteOffset(Rml::BoxArea::Content);
	const Rml::Vector2f size = well->GetBox().GetSize(Rml::BoxArea::Content);
	if (size.x < 1.f || size.y < 1.f)
		return;

	const int cx = (int)(off.x + size.x * 0.5f);
	// Bias downward so the model sits in the taller preview well.
	const int cy = (int)(off.y + size.y * 0.78f);
	RmlInv_RenderChaPreview(cx, cy);
}

bool CRmlUiInventoryForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

bool CRmlUiInventoryForm::IsItemDragging() const {
	return m_impl && m_impl->itemDragging;
}

std::string CRmlUiInventoryForm::Impl::FingerprintSlots(const std::vector<RmlInvSlotView>& slots) {
	std::string out;
	out.reserve(slots.size() * 48);
	for (const RmlInvSlotView& view : slots)
		AppendSlotFingerprint(out, view);
	return out;
}

const char* CRmlUiInventoryForm::GetFilter() const {
	return m_impl ? m_impl->filter.c_str() : "all";
}

bool CRmlUiInventoryForm::IsEquipModeApparel() const {
	return m_impl && m_impl->apparelMode;
}

void CRmlUiInventoryForm::SetEquipModeApparel(bool apparel) {
	if (!m_impl)
		return;
	m_impl->apparelMode = apparel;
	m_impl->SetModeButtons(apparel);
}

void CRmlUiInventoryForm::Impl::SetModeButtons(bool apparel) {
	if (Rml::Element* eq = Get("btnModeEquip"))
		eq->SetClass("inv-mode-active", !apparel);
	if (Rml::Element* ap = Get("btnModeApparel"))
		ap->SetClass("inv-mode-active", apparel);
}

void CRmlUiInventoryForm::SetPackageLocked(bool locked) {
	if (!m_impl || !m_impl->document)
		return;
	const int state = locked ? 1 : 0;
	if (m_impl->lastLockState == state)
		return;
	m_impl->lastLockState = state;
	if (Rml::Element* img = m_impl->Get("imgLock"))
		img->SetAttribute("src", locked ? "ui/rml/frames/notice/ico_unlock.tga"
										: "ui/rml/frames/notice/ico_lock.tga");
	if (Rml::Element* btn = m_impl->Get("btnLock"))
		btn->SetAttribute("title", locked ? "Unlock" : "Lock");
}

void CRmlUiInventoryForm::SetCurrency(const char* gold, const char* imp) {
	if (!m_impl || !m_impl->document)
		return;
	const char* g = gold ? gold : "0";
	const char* i = imp ? imp : "0";
	if (m_impl->lastGold == g && m_impl->lastImp == i)
		return;
	m_impl->lastGold = g;
	m_impl->lastImp = i;
	if (Rml::Element* el = m_impl->Get("labGold"))
		el->SetInnerRML(EscapeXml(g));
	if (Rml::Element* el = m_impl->Get("labIMP"))
		el->SetInnerRML(EscapeXml(i));
}

void CRmlUiInventoryForm::SetCapacity(int used, int unlocked) {
	if (!m_impl || !m_impl->document)
		return;
	char buf[64];
	sprintf_s(buf, "Capacity: %d/%d", used, unlocked);
	if (m_impl->lastCapacity == buf)
		return;
	m_impl->lastCapacity = buf;
	if (Rml::Element* el = m_impl->Get("labCapacity"))
		el->SetInnerRML(EscapeXml(buf));
}

void CRmlUiInventoryForm::Impl::ClearChildren(Rml::Element* parent) {
	if (!parent)
		return;
	while (Rml::Element* c = parent->GetFirstChild())
		parent->RemoveChild(c);
}

Rml::ElementPtr CRmlUiInventoryForm::Impl::MakeSlot(const char* id, const RmlInvSlotView& view, const char* dataKey) {
	// div slots: RmlUi button/flex defaults collapse empty cells into thin bars
	Rml::ElementPtr slot = document->CreateElement("div");
	if (!slot)
		return slot;

	Rml::String classes = "inv-slot";
	if (view.locked)
		classes += " inv-slot-locked";
	else if (view.iconPath.empty())
		classes += " inv-slot-empty";
	slot->SetClassNames(classes);
	if (id && id[0])
		slot->SetId(id);
	slot->SetAttribute(dataKey, view.id);
	slot->AddEventListener(Rml::EventId::Dblclick, this);
	slot->AddEventListener(Rml::EventId::Mouseup, this);
	slot->AddEventListener(Rml::EventId::Mouseover, this);
	slot->AddEventListener(Rml::EventId::Mouseout, this);
	slot->AddEventListener(Rml::EventId::Dragstart, this);
	slot->AddEventListener(Rml::EventId::Dragend, this);
	slot->AddEventListener(Rml::EventId::Dragdrop, this);

	// Inline sizes beat stylesheet/cascade surprises for dynamically created nodes
	slot->SetProperty("display", "block");
	slot->SetProperty("box-sizing", "border-box");
	slot->SetProperty("flex", "none");
	slot->SetProperty("width", "44dp");
	slot->SetProperty("height", "44dp");
	slot->SetProperty("min-width", "44dp");
	slot->SetProperty("min-height", "44dp");
	slot->SetProperty("max-width", "44dp");
	slot->SetProperty("max-height", "44dp");
	slot->SetProperty("padding", "2dp");
	slot->SetProperty("overflow", "hidden");
	slot->SetProperty("background-color", view.locked ? "#c8d4e4" : "#e0ecf8");
	slot->SetProperty("border-top-width", "1dp");
	slot->SetProperty("border-right-width", "1dp");
	slot->SetProperty("border-bottom-width", "1dp");
	slot->SetProperty("border-left-width", "1dp");
	slot->SetProperty("border-color", "#9eb8e8");

	const bool filled = !view.iconPath.empty() && !view.locked;
	if (filled)
		slot->SetProperty("drag", "clone");
	else
		slot->SetProperty("drag", "none");

	if (filled) {
		Rml::ElementPtr img = document->CreateElement("img");
		if (img) {
			img->SetClassNames("inv-slot-icon");
			img->SetAttribute("src", view.iconPath.c_str());
			img->SetProperty("display", "block");
			img->SetProperty("width", "38dp");
			img->SetProperty("height", "38dp");
			img->SetProperty("background-color", "transparent");
			img->SetProperty("pointer-events", "none");
			slot->AppendChild(std::move(img));
		}
		if (view.qty > 1) {
			Rml::ElementPtr qtyEl = document->CreateElement("div");
			if (qtyEl) {
				qtyEl->SetClassNames("inv-slot-qty");
				char buf[16];
				sprintf_s(buf, "%d", view.qty);
				qtyEl->SetInnerRML(EscapeXml(buf));
				qtyEl->SetProperty("pointer-events", "none");
				slot->AppendChild(std::move(qtyEl));
			}
		}
	} else if (!view.placeholderPath.empty() && !view.locked) {
		Rml::ElementPtr img = document->CreateElement("img");
		if (img) {
			img->SetClassNames("inv-slot-icon inv-slot-placeholder");
			img->SetAttribute("src", view.placeholderPath.c_str());
			img->SetProperty("display", "block");
			img->SetProperty("width", "38dp");
			img->SetProperty("height", "38dp");
			img->SetProperty("background-color", "transparent");
			img->SetProperty("pointer-events", "none");
			slot->AppendChild(std::move(img));
		}
	} else {
		Rml::ElementPtr filler = document->CreateElement("div");
		if (filler) {
			filler->SetClassNames("inv-slot-icon");
			filler->SetProperty("display", "block");
			filler->SetProperty("width", "38dp");
			filler->SetProperty("height", "38dp");
			filler->SetProperty("background-color", "transparent");
			filler->SetProperty("pointer-events", "none");
			slot->AppendChild(std::move(filler));
		}
	}

	return slot;
}

void CRmlUiInventoryForm::Impl::FillSlotColumn(Rml::Element* parent, const std::vector<RmlInvSlotView>& slots, const char* idPrefix, const char* dataKey) {
	ClearChildren(parent);
	if (!parent)
		return;
	for (const RmlInvSlotView& view : slots) {
		char idBuf[48];
		sprintf_s(idBuf, "%s-%d", idPrefix, view.id);
		Rml::ElementPtr slot = MakeSlot(idBuf, view, dataKey);
		if (slot) {
			slot->SetProperty("margin", "0dp 0dp 6dp 0dp");
			parent->AppendChild(std::move(slot));
		}
	}
}

void CRmlUiInventoryForm::Impl::FillEquipBottom(Rml::Element* parent, const std::vector<RmlInvSlotView>& slots) {
	ClearChildren(parent);
	if (!parent)
		return;
	parent->SetProperty("display", "flex");
	parent->SetProperty("flex-direction", "row");
	parent->SetProperty("flex-wrap", "wrap");
	parent->SetProperty("justify-content", "center");
	parent->SetProperty("align-content", "flex-start");
	for (const RmlInvSlotView& view : slots) {
		char idBuf[48];
		sprintf_s(idBuf, "equip-%d", view.id);
		Rml::ElementPtr slot = MakeSlot(idBuf, view, "data-equip");
		if (slot) {
			slot->SetProperty("margin", "0dp 4dp 4dp 0dp");
			parent->AppendChild(std::move(slot));
		}
	}
}

void CRmlUiInventoryForm::SetEquipSlots(const std::vector<RmlInvSlotView>& left,
										const std::vector<RmlInvSlotView>& right,
										const std::vector<RmlInvSlotView>& bottom) {
	if (!m_impl || !m_impl->document)
		return;
	if (m_impl->itemDragging)
		return;

	std::string fp = Impl::FingerprintSlots(left);
	fp += '#';
	fp += Impl::FingerprintSlots(right);
	fp += '#';
	fp += Impl::FingerprintSlots(bottom);
	if (fp == m_impl->equipFingerprint)
		return;
	m_impl->equipFingerprint = std::move(fp);

	m_impl->FillSlotColumn(m_impl->Get("equip-left"), left, "equip", "data-equip");
	m_impl->FillSlotColumn(m_impl->Get("equip-right"), right, "equip", "data-equip");
	m_impl->FillEquipBottom(m_impl->Get("equip-bottom"), bottom);
}

void CRmlUiInventoryForm::SetBagSlots(const std::vector<RmlInvSlotView>& slots, int columns) {
	if (!m_impl || !m_impl->document)
		return;
	if (m_impl->itemDragging)
		return;

	const int cols = columns > 0 ? columns : 6;
	std::string fp = Impl::FingerprintSlots(slots);
	if (cols == m_impl->lastBagColumns && fp == m_impl->bagFingerprint)
		return;
	m_impl->lastBagColumns = cols;
	m_impl->bagFingerprint = std::move(fp);

	Rml::Element* grid = m_impl->Get("bag-grid");
	m_impl->ClearChildren(grid);
	if (!grid)
		return;

	Rml::Element* row = nullptr;
	int col = 0;
	for (const RmlInvSlotView& view : slots) {
		if (col == 0) {
			Rml::ElementPtr rowPtr = m_impl->document->CreateElement("div");
			if (!rowPtr)
				break;
			rowPtr->SetClassNames("inv-bag-row");
			rowPtr->SetProperty("display", "flex");
			rowPtr->SetProperty("flex-direction", "row");
			rowPtr->SetProperty("flex-wrap", "nowrap");
			rowPtr->SetProperty("align-items", "flex-start");
			rowPtr->SetProperty("width", "324dp");
			rowPtr->SetProperty("height", "48dp");
			rowPtr->SetProperty("margin-bottom", "4dp");
			row = grid->AppendChild(std::move(rowPtr));
		}

		char idBuf[32];
		sprintf_s(idBuf, "bag-%d", view.id);
		Rml::ElementPtr slot = m_impl->MakeSlot(idBuf, view, "data-bag");
		if (slot && row) {
			slot->SetProperty("margin", "0dp 6dp 0dp 0dp");
			slot->SetProperty("background-color", view.locked ? "#c8d4e4" : "#e0ecf8");
			row->AppendChild(std::move(slot));
		}
		col++;
		if (col >= cols)
			col = 0;
	}
}

void CRmlUiInventoryForm::Impl::SetActiveTab(const char* id) {
	const char* tabs[] = {"tabItems", "tabEquipment", "tabConsumable", "tabMaterial", "tabOther"};
	for (const char* tabId : tabs) {
		if (Rml::Element* el = Get(tabId))
			el->SetClass("inv-tab-active", id && strcmp(tabId, id) == 0);
	}
}

void CRmlUiInventoryForm::Impl::ProcessEvent(Rml::Event& event) {
	Rml::Element* target = event.GetTargetElement();
	if (!target)
		return;

	const bool isRightUp = (event == Rml::EventId::Mouseup && event.GetParameter<int>("button", -1) == 1);
	const bool isClick = (event == Rml::EventId::Click);
	const bool isDblClick = (event == Rml::EventId::Dblclick);
	const bool isDragDrop = (event == Rml::EventId::Dragdrop);
	const bool isDragStart = (event == Rml::EventId::Dragstart);
	const bool isDragEnd = (event == Rml::EventId::Dragend);
	const bool isMouseOver = (event == Rml::EventId::Mouseover);
	const bool isMouseOut = (event == Rml::EventId::Mouseout);
	if (!isClick && !isRightUp && !isDblClick && !isDragDrop && !isDragStart && !isDragEnd && !isMouseOver && !isMouseOut)
		return;

	if (isMouseOver) {
		int bag = -1, equip = -1;
		if (ResolveInvSlot(target, document, bag, equip)) {
			hoverBag = bag;
			hoverEquip = equip;
		}
		return;
	}
	if (isMouseOut) {
		int bag = -1, equip = -1;
		if (ResolveInvSlot(target, document, bag, equip)) {
			if (hoverBag == bag && hoverEquip == equip) {
				hoverBag = -1;
				hoverEquip = -1;
			}
		}
		return;
	}

	if (isDragStart) {
		int bag = -1, equip = -1;
		if (ResolveInvSlot(target, document, bag, equip))
			itemDragging = true;
		hoverBag = -1;
		hoverEquip = -1;
		return;
	}
	if (isDragEnd) {
		itemDragging = false;
		bagFingerprint.clear();
		equipFingerprint.clear();
		RmlInv_OnItemDragEnd();
		return;
	}

	if (isDragDrop) {
		Rml::Element* dragEl = static_cast<Rml::Element*>(event.GetParameter<void*>("drag_element", nullptr));
		if (!dragEl)
			return;
		int srcBag = -1, srcEquip = -1, dstBag = -1, dstEquip = -1;
		if (!ResolveInvSlot(dragEl, document, srcBag, srcEquip))
			return;
		if (!ResolveInvSlot(target, document, dstBag, dstEquip))
			return;
		if (srcBag == dstBag && srcEquip == dstEquip)
			return;
		RmlInv_OnDrop(srcBag, srcEquip, dstBag, dstEquip);
		return;
	}

	Rml::Element* el = target;
	while (el && el != document) {
		const Rml::String& id = el->GetId();

		if (isClick) {
			if (id == "btnClose") {
				RmlInv_OnClose();
				return;
			}
			if (id == "btnLock") {
				RmlInv_OnToggleLock();
				return;
			}
			if (id == "btnExpand") {
				RmlInv_OnExpandBag();
				return;
			}
			if (id == "btnTempBag") {
				RmlInv_OnTempBag();
				return;
			}
			if (id == "btnPreviewLeft") {
				RmlInv_RotatePreviewLeft();
				return;
			}
			if (id == "btnPreviewRight") {
				RmlInv_RotatePreviewRight();
				return;
			}
			if (id == "btnModeEquip") {
				apparelMode = false;
				SetModeButtons(false);
				RmlInv_OnToggleApparel(false);
				return;
			}
			if (id == "btnModeApparel") {
				apparelMode = true;
				SetModeButtons(true);
				RmlInv_OnToggleApparel(true);
				return;
			}

			if (el->HasAttribute("data-filter")) {
				const Rml::String filterAttr = el->GetAttribute("data-filter", Rml::String("all"));
				filter = filterAttr.c_str();
				SetActiveTab(id.c_str());
				RmlInv_OnFilterChanged();
				return;
			}
		}

		if (isDblClick) {
			if (el->HasAttribute("data-bag")) {
				const int index = el->GetAttribute("data-bag", -1);
				if (index >= 0)
					RmlInv_OnBagDblClick(index);
				return;
			}
			if (el->HasAttribute("data-equip")) {
				const int link = el->GetAttribute("data-equip", -1);
				if (link >= 0)
					RmlInv_OnEquipDblClick(link);
				return;
			}
		}

		if (isRightUp && el->HasAttribute("data-bag")) {
			const int index = el->GetAttribute("data-bag", -1);
			if (index >= 0)
				RmlInv_OnBagRightClick(index);
			return;
		}

		el = el->GetParentNode();
	}
}
