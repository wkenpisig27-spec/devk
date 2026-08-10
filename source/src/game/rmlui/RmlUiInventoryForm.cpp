#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiInventoryForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Property.h>
#include <RmlUi/Core/StyleTypes.h>
#include <RmlUi/Core/SystemInterface.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <windows.h>

extern void RmlInv_OnClose();
extern void RmlInv_OnBagDblClick(int index);
extern void RmlInv_OnBagRightClick(int index);
// Returns true if the mousedown was consumed (Ctrl multi-select — suppress drag).
extern bool RmlInv_OnBagMouseDown(int index, bool ctrl);
extern void RmlInv_OnEquipDblClick(int link);
extern void RmlInv_OnDrop(int srcBag, int srcEquip, int dstBag, int dstEquip);
extern void RmlInv_OnItemDragEnd(int srcBag, int srcEquip, int mouseX, int mouseY);
extern void RmlBank_OnDrop(int srcBank, int srcBag, int dstBank, int dstBag);
extern void RmlInv_OnFilterChanged();
extern void RmlInv_OnToggleApparel(bool apparel);
extern void RmlInv_OnToggleLock();
extern void RmlInv_OnExpandBag();
extern void RmlInv_OnTempBag();
extern void RmlInv_OnLockConfirm();
extern void RmlInv_OnExpandConfirm();
extern void RmlInv_OnItemConfirmYes();
extern void RmlInv_OnItemConfirmNo();
extern void RmlInv_OnContextAction(const char* action);
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

Rml::String EscapeXmlWithBreaks(const char* text) {
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
		case '\r': break;
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
	sprintf_s(buf, "%d|%d|%d|%d|%d|", view.id, view.qty, view.locked ? 1 : 0, view.iconDimmed ? 1 : 0, view.selected ? 1 : 0);
	out += buf;
	out += view.iconPath;
	out += '|';
	out += view.placeholderPath;
	out += ';';
}

bool IsCtrlKeyDown() {
	return (GetKeyState(VK_CONTROL) & 0x8000) != 0;
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
	bool lockModalOpen = false;
	bool expandModalOpen = false;
	bool confirmModalOpen = false;
	bool ctxMenuOpen = false;
	bool thumbDragging = false;
	float thumbDragGrabY = 0.f;
	int hoverBag = -1;
	int hoverEquip = -1;
	// Manual double-click (RmlUi dblclick depends on focus; slots use focus:none).
	int lastClickBag = -1;
	int lastClickEquip = -1;
	double lastClickTime = 0.0;
	float lastClickX = 0.f;
	float lastClickY = 0.f;

	void ProcessEvent(Rml::Event& event) override;
	void BindStaticControls();
	void CenterRoot();
	Rml::Element* Get(const char* id) const;
	void SetModalOpen(const char* modalId, bool open);
	void SetActiveTab(const char* id);
	void SetModeButtons(bool apparel);
	void ClearChildren(Rml::Element* parent);
	Rml::ElementPtr MakeSlot(const char* id, const RmlInvSlotView& view, const char* dataKey);
	void FillSlotColumn(Rml::Element* parent, const std::vector<RmlInvSlotView>& slots, const char* idPrefix, const char* dataKey);
	void FillEquipBottom(Rml::Element* parent, const std::vector<RmlInvSlotView>& slots);
	void ScrollBagBy(float deltaPx);
	void SetBagScrollTop(float scrollTop);
	void UpdateBagScrollbar();
	bool IsUnderBagScroll(Rml::Element* el) const;
	bool TryManualDoubleClick(int bag, int equip, float mouseX, float mouseY);
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
	m_impl->lockModalOpen = false;
	m_impl->expandModalOpen = false;
	m_impl->thumbDragging = false;
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
		"btnLockConfirm", "btnLockCancel", "inv-lock-scrim",
		"btnExpandConfirm", "btnExpandCancel", "inv-expand-scrim",
		"btnConfirmYes", "btnConfirmNo", "inv-confirm-scrim",
		"inv-ctx-scrim",
		"ctx-throw", "ctx-delete", "ctx-lock", "ctx-unlock", "ctx-sell", "ctx-deposit", "ctx-boxrates", "ctx-chat",
		"tabItems", "tabEquipment", "tabConsumable", "tabMaterial", "tabOther"};
	for (const char* id : clickIds) {
		if (Rml::Element* el = document->GetElementById(id))
			el->AddEventListener(Rml::EventId::Click, this);
	}

	if (Rml::Element* grid = document->GetElementById("bag-grid"))
		grid->AddEventListener(Rml::EventId::Mousescroll, this);
	if (Rml::Element* backpack = document->GetElementById("inv-backpack"))
		backpack->AddEventListener(Rml::EventId::Mousescroll, this);

	if (Rml::Element* bar = document->GetElementById("bag-scrollbar"))
		bar->AddEventListener(Rml::EventId::Mousedown, this);
	if (Rml::Element* thumb = document->GetElementById("bag-scroll-thumb"))
		thumb->AddEventListener(Rml::EventId::Mousedown, this);
	// Thumb-drag tracking on the document (does not StopPropagation — item drag still works).
	if (document) {
		document->AddEventListener(Rml::EventId::Mousemove, this);
		document->AddEventListener(Rml::EventId::Mouseup, this);
	}

	SetModeButtons(apparelMode);
	SetModalOpen("inv-lock-modal", false);
	SetModalOpen("inv-expand-modal", false);
	SetModalOpen("inv-confirm-modal", false);
	if (Rml::Element* ctx = Get("inv-ctx"))
		ctx->SetClass("inv-ctx-open", false);
	ctxMenuOpen = false;
}

Rml::Element* CRmlUiInventoryForm::Impl::Get(const char* id) const {
	return document ? document->GetElementById(id) : nullptr;
}

void CRmlUiInventoryForm::Impl::SetModalOpen(const char* modalId, bool open) {
	Rml::Element* modal = Get(modalId);
	if (!modal)
		return;
	if (open)
		modal->SetClass("inv-modal-open", true);
	else
		modal->SetClass("inv-modal-open", false);
}

bool CRmlUiInventoryForm::Impl::IsUnderBagScroll(Rml::Element* el) const {
	while (el && el != document) {
		const Rml::String& id = el->GetId();
		if (id == "bag-grid" || id == "bag-scrollbar" || id == "bag-scroll-thumb" || id == "inv-backpack")
			return true;
		el = el->GetParentNode();
	}
	return false;
}

void CRmlUiInventoryForm::Impl::SetBagScrollTop(float scrollTop) {
	Rml::Element* grid = Get("bag-grid");
	if (!grid)
		return;
	const float maxScroll = (std::max)(0.f, grid->GetScrollHeight() - grid->GetClientHeight());
	if (scrollTop < 0.f)
		scrollTop = 0.f;
	if (scrollTop > maxScroll)
		scrollTop = maxScroll;
	grid->SetScrollTop(scrollTop);
	UpdateBagScrollbar();
}

void CRmlUiInventoryForm::Impl::ScrollBagBy(float deltaPx) {
	Rml::Element* grid = Get("bag-grid");
	if (!grid)
		return;
	SetBagScrollTop(grid->GetScrollTop() + deltaPx);
}

void CRmlUiInventoryForm::Impl::UpdateBagScrollbar() {
	Rml::Element* grid = Get("bag-grid");
	Rml::Element* bar = Get("bag-scrollbar");
	Rml::Element* thumb = Get("bag-scroll-thumb");
	if (!grid || !bar || !thumb)
		return;

	const float clientH = grid->GetClientHeight();
	const float scrollH = grid->GetScrollHeight();
	const float maxScroll = scrollH - clientH;
	const bool needScroll = maxScroll > 1.f;

	bar->SetClass("inv-bag-scrollbar-visible", needScroll);
	if (!needScroll) {
		thumb->SetProperty(Rml::PropertyId::Top, Rml::Property(0.f, Rml::Unit::PX));
		thumb->SetProperty(Rml::PropertyId::Height, Rml::Property(clientH > 1.f ? clientH : 48.f, Rml::Unit::PX));
		return;
	}

	const float trackH = bar->GetClientHeight() > 1.f ? bar->GetClientHeight() : clientH;
	float thumbH = trackH * (clientH / scrollH);
	if (thumbH < 28.f)
		thumbH = 28.f;
	if (thumbH > trackH)
		thumbH = trackH;

	const float scrollRatio = grid->GetScrollTop() / maxScroll;
	const float thumbTop = scrollRatio * (trackH - thumbH);

	thumb->SetProperty(Rml::PropertyId::Height, Rml::Property(thumbH, Rml::Unit::PX));
	thumb->SetProperty(Rml::PropertyId::Top, Rml::Property(thumbTop, Rml::Unit::PX));
}

bool CRmlUiInventoryForm::Impl::TryManualDoubleClick(int bag, int equip, float mouseX, float mouseY) {
	const double now = Rml::GetSystemInterface() ? Rml::GetSystemInterface()->GetElapsedTime() : 0.0;
	const float dx = mouseX - lastClickX;
	const float dy = mouseY - lastClickY;
	const bool sameSlot = (bag >= 0 && bag == lastClickBag) || (equip >= 0 && equip == lastClickEquip);
	const bool quick = (now - lastClickTime) < 0.40;
	const bool closeEnough = (dx * dx + dy * dy) < (10.f * 10.f);

	if (sameSlot && quick && closeEnough && (bag >= 0 || equip >= 0)) {
		lastClickBag = -1;
		lastClickEquip = -1;
		lastClickTime = 0.0;
		if (bag >= 0)
			RmlInv_OnBagDblClick(bag);
		else
			RmlInv_OnEquipDblClick(equip);
		return true;
	}

	lastClickBag = bag;
	lastClickEquip = equip;
	lastClickTime = now;
	lastClickX = mouseX;
	lastClickY = mouseY;
	return false;
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
	HideModals();
	if (m_impl->document)
		m_impl->document->Hide();
}

void CRmlUiInventoryForm::ShowLockConfirmModal() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->SetModalOpen("inv-expand-modal", false);
	m_impl->expandModalOpen = false;
	m_impl->SetModalOpen("inv-confirm-modal", false);
	m_impl->confirmModalOpen = false;
	m_impl->SetModalOpen("inv-lock-modal", true);
	m_impl->lockModalOpen = true;
}

void CRmlUiInventoryForm::ShowExpandBagModal(int currentCapacity, int addSlots, int costImp) {
	if (!m_impl || !m_impl->document)
		return;

	char capBuf[64];
	sprintf_s(capBuf, "Capacity: %d → %d", currentCapacity, currentCapacity + addSlots);
	if (Rml::Element* el = m_impl->Get("inv-expand-cap"))
		el->SetInnerRML(EscapeXml(capBuf));

	char costBuf[64];
	sprintf_s(costBuf, "Cost: %d IMP", costImp);
	if (Rml::Element* el = m_impl->Get("inv-expand-cost"))
		el->SetInnerRML(EscapeXml(costBuf));

	m_impl->SetModalOpen("inv-lock-modal", false);
	m_impl->lockModalOpen = false;
	m_impl->SetModalOpen("inv-confirm-modal", false);
	m_impl->confirmModalOpen = false;
	m_impl->SetModalOpen("inv-expand-modal", true);
	m_impl->expandModalOpen = true;
}

void CRmlUiInventoryForm::ShowConfirmModal(const char* title, const char* message, const char* hint) {
	if (!m_impl || !m_impl->document)
		return;

	if (Rml::Element* el = m_impl->Get("inv-confirm-title"))
		el->SetInnerRML(EscapeXml(title ? title : "Confirm"));
	if (Rml::Element* el = m_impl->Get("inv-confirm-msg"))
		el->SetInnerRML(EscapeXmlWithBreaks(message ? message : ""));
	if (Rml::Element* el = m_impl->Get("inv-confirm-hint")) {
		const bool hasHint = hint && hint[0];
		el->SetInnerRML(hasHint ? EscapeXmlWithBreaks(hint) : "");
		el->SetProperty("display", hasHint ? "block" : "none");
	}

	m_impl->SetModalOpen("inv-lock-modal", false);
	m_impl->lockModalOpen = false;
	m_impl->SetModalOpen("inv-expand-modal", false);
	m_impl->expandModalOpen = false;
	m_impl->SetModalOpen("inv-confirm-modal", true);
	m_impl->confirmModalOpen = true;
}

void CRmlUiInventoryForm::HideModals() {
	if (!m_impl)
		return;
	m_impl->SetModalOpen("inv-lock-modal", false);
	m_impl->SetModalOpen("inv-expand-modal", false);
	m_impl->SetModalOpen("inv-confirm-modal", false);
	m_impl->lockModalOpen = false;
	m_impl->expandModalOpen = false;
	m_impl->confirmModalOpen = false;
	HideContextMenu();
}

bool CRmlUiInventoryForm::IsModalOpen() const {
	return m_impl && (m_impl->lockModalOpen || m_impl->expandModalOpen || m_impl->confirmModalOpen || m_impl->ctxMenuOpen);
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

bool CRmlUiInventoryForm::GetRootScreenRect(float& x, float& y, float& w, float& h) const {
	x = y = w = h = 0.f;
	if (!IsVisible() || !m_impl)
		return false;
	Rml::Element* root = m_impl->Get("inv-root");
	if (!root)
		return false;
	const Rml::Vector2f off = root->GetAbsoluteOffset(Rml::BoxArea::Border);
	const Rml::Vector2f size = root->GetBox().GetSize(Rml::BoxArea::Border);
	x = off.x;
	y = off.y;
	w = size.x;
	h = size.y;
	return w > 0.f && h > 0.f;
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
	sprintf_s(buf, "%d/%d", used, unlocked);
	if (m_impl->lastCapacity == buf)
		return;
	m_impl->lastCapacity = buf;

	char usedBuf[16];
	char maxBuf[16];
	sprintf_s(usedBuf, "%d", used);
	sprintf_s(maxBuf, "%d", unlocked);
	if (Rml::Element* el = m_impl->Get("labCapacityUsed"))
		el->SetInnerRML(EscapeXml(usedBuf));
	if (Rml::Element* el = m_impl->Get("labCapacityMax"))
		el->SetInnerRML(EscapeXml(maxBuf));
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
	if (view.selected)
		classes += " inv-slot-selected";
	slot->SetClassNames(classes);
	if (id && id[0])
		slot->SetId(id);
	slot->SetAttribute(dataKey, view.id);
	slot->AddEventListener(Rml::EventId::Dblclick, this);
	slot->AddEventListener(Rml::EventId::Mousedown, this);
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
	slot->SetProperty("padding", "4dp");
	// visible so soft-AA corners aren't clipped unevenly on flush grid edges
	slot->SetProperty("overflow", "visible");
	slot->SetProperty("background-color", "transparent");
	slot->SetProperty("border-top-width", "0dp");
	slot->SetProperty("border-right-width", "0dp");
	slot->SetProperty("border-bottom-width", "0dp");
	slot->SetProperty("border-left-width", "0dp");
	if (view.locked)
		slot->SetProperty("decorator", "image(ui/rml/frames/notice/slot_rounded_locked.tga)");
	else if (view.selected)
		slot->SetProperty("decorator", "image(ui/rml/frames/notice/slot_rounded_sel.tga)");
	else
		slot->SetProperty("decorator", "image(ui/rml/frames/notice/slot_rounded.tga)");
	slot->SetProperty(Rml::PropertyId::Focus, Rml::Property(Rml::Style::Focus::None));
	slot->SetProperty("pointer-events", "auto");

	const bool filled = !view.iconPath.empty() && !view.locked;
	// Use typed enum — string SetProperty("drag") has been unreliable with ancestor styles.
	if (filled)
		slot->SetProperty(Rml::PropertyId::Drag, Rml::Property(Rml::Style::Drag::Clone));
	else
		slot->SetProperty(Rml::PropertyId::Drag, Rml::Property(Rml::Style::Drag::None));

	if (filled) {
		Rml::ElementPtr img = document->CreateElement("img");
		if (img) {
			img->SetClassNames(view.iconDimmed ? "inv-slot-icon inv-slot-icon-dim" : "inv-slot-icon");
			img->SetAttribute("src", view.iconPath.c_str());
			img->SetProperty("display", "block");
			img->SetProperty("width", "36dp");
			img->SetProperty("height", "36dp");
			img->SetProperty("background-color", "transparent");
			img->SetProperty("pointer-events", "none");
			if (view.iconDimmed)
				img->SetProperty("image-color", "#8a96a8");
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
			img->SetProperty("width", "36dp");
			img->SetProperty("height", "36dp");
			img->SetProperty("background-color", "transparent");
			img->SetProperty("pointer-events", "none");
			slot->AppendChild(std::move(img));
		}
	} else {
		Rml::ElementPtr filler = document->CreateElement("div");
		if (filler) {
			filler->SetClassNames("inv-slot-icon");
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

void CRmlUiInventoryForm::Impl::FillSlotColumn(Rml::Element* parent, const std::vector<RmlInvSlotView>& slots, const char* idPrefix, const char* dataKey) {
	ClearChildren(parent);
	if (!parent)
		return;
	const size_t n = slots.size();
	for (size_t i = 0; i < n; ++i) {
		const RmlInvSlotView& view = slots[i];
		char idBuf[48];
		sprintf_s(idBuf, "%s-%d", idPrefix, view.id);
		Rml::ElementPtr slot = MakeSlot(idBuf, view, dataKey);
		if (slot) {
			// Flush last slot — no trailing gutter under the column.
			if (i + 1 >= n)
				slot->SetProperty("margin", "0dp 0dp 0dp 0dp");
			else
				slot->SetProperty("margin", "0dp 0dp 4dp 0dp");
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
	if (!grid)
		return;

	// Keep the user's scroll position across live capacity / item refreshes.
	const float savedScroll = grid->GetScrollTop();
	m_impl->ClearChildren(grid);

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
			rowPtr->SetProperty("width", "284dp");
			rowPtr->SetProperty("height", "44dp");
			rowPtr->SetProperty("margin-bottom", "4dp");
			row = grid->AppendChild(std::move(rowPtr));
		}

		char idBuf[32];
		sprintf_s(idBuf, "bag-%d", view.id);
		Rml::ElementPtr slot = m_impl->MakeSlot(idBuf, view, "data-bag");
		if (slot && row) {
			// 6*44 + 5*4 = 284 — no trailing gutter on the last column.
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
	m_impl->SetBagScrollTop(savedScroll);
}

void CRmlUiInventoryForm::ApplyBagSelection(const std::vector<char>& selectedByIndex) {
	if (!m_impl || !m_impl->document)
		return;

	for (size_t i = 0; i < selectedByIndex.size(); ++i) {
		char idBuf[48];
		sprintf_s(idBuf, "bag-%d", (int)i);
		Rml::Element* el = m_impl->Get(idBuf);
		if (!el)
			continue;
		const bool selected = selectedByIndex[i] != 0;
		el->SetClass("inv-slot-selected", selected);
		if (el->IsClassSet("inv-slot-locked"))
			continue;
		el->SetProperty("decorator", selected
										 ? "image(ui/rml/frames/notice/slot_rounded_sel.tga)"
										 : "image(ui/rml/frames/notice/slot_rounded.tga)");
	}
	// Force next full SetBagSlots to rebuild if selection changed.
	m_impl->bagFingerprint.clear();
}

void CRmlUiInventoryForm::ShowContextMenu(int screenX, int screenY, const CtxMenuFlags& flags) {
	if (!m_impl || !m_impl->document)
		return;

	auto setItem = [&](const char* id, bool show) {
		if (Rml::Element* el = m_impl->Get(id))
			el->SetProperty("display", show ? "block" : "none");
	};
	setItem("ctx-throw", flags.throwItem);
	setItem("ctx-delete", flags.deleteItem);
	setItem("ctx-lock", flags.lockItem);
	setItem("ctx-unlock", flags.unlockItem);
	setItem("ctx-sell", flags.sellItem);
	setItem("ctx-deposit", flags.depositItem);
	setItem("ctx-boxrates", flags.boxRates);
	setItem("ctx-chat", flags.sendToChat);

	Rml::Element* root = m_impl->Get("inv-ctx");
	Rml::Element* panel = m_impl->Get("inv-ctx-panel");
	if (!root || !panel)
		return;

	// Convert screen coords into document-local space.
	const Rml::Vector2f docOff = m_impl->document->GetAbsoluteOffset(Rml::BoxArea::Border);
	float localX = (float)screenX - docOff.x + 4.f;
	float localY = (float)screenY - docOff.y + 4.f;

	root->SetClass("inv-ctx-open", true);
	m_impl->ctxMenuOpen = true;

	// Place first, then clamp after layout if needed.
	panel->SetProperty(Rml::PropertyId::Left, Rml::Property(localX, Rml::Unit::PX));
	panel->SetProperty(Rml::PropertyId::Top, Rml::Property(localY, Rml::Unit::PX));
	if (m_impl->context)
		m_impl->context->Update();

	const Rml::Vector2f panelSize = panel->GetBox().GetSize(Rml::BoxArea::Border);
	const Rml::Vector2f docSize = m_impl->document->GetBox().GetSize(Rml::BoxArea::Border);
	if (panelSize.x > 0.f && localX + panelSize.x > docSize.x)
		localX = (std::max)(4.f, docSize.x - panelSize.x - 4.f);
	if (panelSize.y > 0.f && localY + panelSize.y > docSize.y)
		localY = (std::max)(4.f, docSize.y - panelSize.y - 4.f);
	panel->SetProperty(Rml::PropertyId::Left, Rml::Property(localX, Rml::Unit::PX));
	panel->SetProperty(Rml::PropertyId::Top, Rml::Property(localY, Rml::Unit::PX));
}

void CRmlUiInventoryForm::HideContextMenu() {
	if (!m_impl)
		return;
	if (Rml::Element* root = m_impl->Get("inv-ctx"))
		root->SetClass("inv-ctx-open", false);
	m_impl->ctxMenuOpen = false;
}

bool CRmlUiInventoryForm::IsContextMenuOpen() const {
	return m_impl && m_impl->ctxMenuOpen;
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
	const bool isMouseScroll = (event == Rml::EventId::Mousescroll);
	const bool isMouseDown = (event == Rml::EventId::Mousedown);
	const bool isMouseMove = (event == Rml::EventId::Mousemove);
	const bool isMouseUp = (event == Rml::EventId::Mouseup);

	if (isMouseScroll) {
		if (!IsUnderBagScroll(target))
			return;
		const float wheelY = event.GetParameter<float>("wheel_delta_y", 0.f);
		if (wheelY == 0.f)
			return;
		// One notch ≈ one bag row.
		ScrollBagBy(wheelY * 52.f);
		event.StopPropagation();
		return;
	}

	if (isMouseDown) {
		const int button = event.GetParameter<int>("button", -1);
		Rml::Element* el = target;
		while (el && el != document) {
			const Rml::String& id = el->GetId();
			if (id == "bag-scroll-thumb") {
				thumbDragging = true;
				const float mouseY = event.GetParameter<float>("mouse_y", 0.f);
				const float thumbTop = el->GetAbsoluteOffset(Rml::BoxArea::Border).y;
				thumbDragGrabY = mouseY - thumbTop;
				event.StopPropagation();
				return;
			}
			if (id == "bag-scrollbar") {
				Rml::Element* bar = el;
				Rml::Element* grid = Get("bag-grid");
				Rml::Element* thumb = Get("bag-scroll-thumb");
				if (grid && thumb) {
					const float mouseY = event.GetParameter<float>("mouse_y", 0.f);
					const float barTop = bar->GetAbsoluteOffset(Rml::BoxArea::Border).y;
					const float trackH = bar->GetClientHeight();
					const float thumbH = thumb->GetBox().GetSize(Rml::BoxArea::Border).y;
					const float maxScroll = (std::max)(0.f, grid->GetScrollHeight() - grid->GetClientHeight());
					float ratio = 0.f;
					if (trackH > thumbH)
						ratio = (mouseY - barTop - thumbH * 0.5f) / (trackH - thumbH);
					if (ratio < 0.f)
						ratio = 0.f;
					if (ratio > 1.f)
						ratio = 1.f;
					SetBagScrollTop(ratio * maxScroll);
					thumbDragging = true;
					thumbDragGrabY = thumbH * 0.5f;
				}
				event.StopPropagation();
				return;
			}
			el = el->GetParentNode();
		}

		// Manual double-click for bag/equip slots (independent of RmlUi focus).
		if (button == 0) {
			int bag = -1, equip = -1;
			if (ResolveInvSlot(target, document, bag, equip)) {
				const float mx = event.GetParameter<float>("mouse_x", 0.f);
				const float my = event.GetParameter<float>("mouse_y", 0.f);
				if (TryManualDoubleClick(bag, equip, mx, my)) {
					// Consume so RmlUi won't start a drag on the second click.
					event.StopPropagation();
					return;
				}
				// Ctrl+click toggles multi-select (legacy goods-grid); plain click clears it.
				if (bag >= 0 && RmlInv_OnBagMouseDown(bag, IsCtrlKeyDown())) {
					event.StopPropagation();
					return;
				}
			}
		}
	}

	if (isMouseMove && thumbDragging) {
		Rml::Element* bar = Get("bag-scrollbar");
		Rml::Element* grid = Get("bag-grid");
		Rml::Element* thumb = Get("bag-scroll-thumb");
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
			SetBagScrollTop(ratio * maxScroll);
		}
		return;
	}

	if (isMouseUp) {
		const bool wasThumb = thumbDragging;
		thumbDragging = false;
		if (wasThumb) {
			event.StopPropagation();
			return;
		}
		// Left mouseup on slots is only for right-click path below; ignore otherwise.
		if (!isRightUp && event.GetParameter<int>("button", -1) == 0)
			return;
	}

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
		int bag = -1, equip = -1;
		ResolveInvSlot(target, document, bag, equip);
		const int mx = (int)event.GetParameter<float>("mouse_x", 0.f);
		const int my = (int)event.GetParameter<float>("mouse_y", 0.f);
		bagFingerprint.clear();
		equipFingerprint.clear();
		RmlInv_OnItemDragEnd(bag, equip, mx, my);
		return;
	}

	if (isDragDrop) {
		Rml::Element* dragEl = static_cast<Rml::Element*>(event.GetParameter<void*>("drag_element", nullptr));
		if (!dragEl)
			return;
		int srcBag = -1, srcEquip = -1, dstBag = -1, dstEquip = -1;
		int srcBank = -1;
		// Cross-document: bank → bag (drag_element may live outside this document).
		{
			Rml::Element* el = dragEl;
			while (el) {
				if (el->HasAttribute("data-bank")) {
					srcBank = el->GetAttribute("data-bank", -1);
					break;
				}
				if (el->HasAttribute("data-bag") || el->HasAttribute("data-equip"))
					break;
				el = el->GetParentNode();
			}
		}
		if (srcBank >= 0) {
			if (ResolveInvSlot(target, document, dstBag, dstEquip) && dstBag >= 0)
				RmlBank_OnDrop(srcBank, -1, -1, dstBag);
			return;
		}
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
			if (id == "btnLockConfirm") {
				lockModalOpen = false;
				SetModalOpen("inv-lock-modal", false);
				RmlInv_OnLockConfirm();
				return;
			}
			if (id == "btnLockCancel" || id == "inv-lock-scrim") {
				lockModalOpen = false;
				SetModalOpen("inv-lock-modal", false);
				return;
			}
			if (id == "btnExpandConfirm") {
				expandModalOpen = false;
				SetModalOpen("inv-expand-modal", false);
				RmlInv_OnExpandConfirm();
				return;
			}
			if (id == "btnExpandCancel" || id == "inv-expand-scrim") {
				expandModalOpen = false;
				SetModalOpen("inv-expand-modal", false);
				return;
			}
			if (id == "btnConfirmYes") {
				confirmModalOpen = false;
				SetModalOpen("inv-confirm-modal", false);
				RmlInv_OnItemConfirmYes();
				return;
			}
			if (id == "btnConfirmNo" || id == "inv-confirm-scrim") {
				confirmModalOpen = false;
				SetModalOpen("inv-confirm-modal", false);
				RmlInv_OnItemConfirmNo();
				return;
			}
			if (id == "inv-ctx-scrim") {
				ctxMenuOpen = false;
				if (Rml::Element* root = Get("inv-ctx"))
					root->SetClass("inv-ctx-open", false);
				return;
			}
			if (el->HasAttribute("data-ctx")) {
				const Rml::String action = el->GetAttribute("data-ctx", Rml::String());
				ctxMenuOpen = false;
				if (Rml::Element* root = Get("inv-ctx"))
					root->SetClass("inv-ctx-open", false);
				if (!action.empty())
					RmlInv_OnContextAction(action.c_str());
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
