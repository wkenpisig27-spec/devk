#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiNpcTradeForm.h"
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
#include <string>
#include <windows.h>

extern void RmlNpcTrade_OnClose();
extern void RmlNpcTrade_OnTab(int page);
extern void RmlNpcTrade_OnBuy(int page, int shopIndex, int bagSlot);
extern void RmlNpcTrade_OnDrop(int srcPage, int srcShop, int srcBag, int dstBag);
extern void RmlNpcTrade_OnDragEnd(int srcPage, int srcShop, int mouseX, int mouseY);
extern void RmlNpcTrade_OnBagSell(int bagIndex);
extern void RmlNpcTrade_OnConfirmYes();
extern void RmlNpcTrade_OnConfirmNo();
extern void RmlNpcTrade_OnQtyYes();
extern void RmlNpcTrade_OnQtyNo();
extern void RmlNpcTrade_OnQtyChanged(int qty);
extern void RmlNpcTrade_ApplyItemHint(int page, int shopIndex, int mouseX, int mouseY);
extern void RmlNpcTrade_RenderItemHint();

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

bool ResolveTradeSlot(Rml::Element* el, int& page, int& shopIndex) {
	page = -1;
	shopIndex = -1;
	while (el) {
		if (el->HasAttribute("data-trade")) {
			shopIndex = el->GetAttribute("data-trade", -1);
			page = el->GetAttribute("data-trade-page", 0);
			return shopIndex >= 0;
		}
		if (el->HasAttribute("data-bag") || el->HasAttribute("data-equip") || el->HasAttribute("data-bank"))
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
		if (el->HasAttribute("data-trade") || el->HasAttribute("data-bank") || el->HasAttribute("data-equip"))
			return false;
		el = el->GetParentNode();
	}
	return false;
}

std::string FingerprintSlots(const std::vector<RmlNpcSlotView>& slots) {
	std::string out;
	out.reserve(slots.size() * 48);
	for (const RmlNpcSlotView& view : slots) {
		char buf[64];
		sprintf_s(buf, "%d|%d|", view.id, view.page);
		out += buf;
		out += view.iconPath;
		out += view.priceText;
		out += ';';
	}
	return out;
}

} // namespace

struct CRmlUiNpcTradeForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	bool loadOk = false;
	bool hasPlacedRoot = false;
	bool itemDragging = false;
	bool thumbDragging = false;
	bool confirmModalOpen = false;
	bool qtyModalOpen = false;
	float thumbDragGrabY = 0.f;
	float thumbDragStartScroll = 0.f;
	std::string slotFingerprint;
	std::string goldCache;
	int lastColumns = -1;
	int activePage = 0;
	int qtyValue = 1;
	int qtyMax = 1;
	__int64 qtyUnitPrice = 0;
	int hoverPage = -1;
	int hoverShop = -1;
	double lastClickTime = 0.0;
	int lastClickShop = -1;

	Rml::Element* Get(const char* id) const {
		return document ? document->GetElementById(id) : nullptr;
	}

	void SetModalOpen(const char* id, bool open) {
		if (Rml::Element* el = Get(id))
			el->SetClass("npc-modal-open", open);
	}

	void SyncQtyUi() {
		char buf[32];
		sprintf_s(buf, "%d", qtyValue);
		if (Rml::Element* el = Get("npc-qty-value"))
			el->SetInnerRML(EscapeXml(buf));
		const __int64 total = qtyUnitPrice * (__int64)qtyValue;
		char totalBuf[64];
		sprintf_s(totalBuf, "Total: %lld", (long long)total);
		if (Rml::Element* el = Get("npc-qty-total"))
			el->SetInnerRML(EscapeXml(totalBuf));
	}

	void ClearChildren(Rml::Element* parent) {
		if (!parent)
			return;
		while (Rml::Element* c = parent->GetFirstChild())
			parent->RemoveChild(c);
	}

	void SetActiveTabUi(int page) {
		activePage = page;
		if (Rml::Element* el = Get("tabWeapon"))
			el->SetClass("npc-tab-active", page == 0);
		if (Rml::Element* el = Get("tabEquip"))
			el->SetClass("npc-tab-active", page == 1);
		if (Rml::Element* el = Get("tabOther"))
			el->SetClass("npc-tab-active", page == 2);
	}

	void SetScrollTop(float scrollTop) {
		Rml::Element* grid = Get("npc-grid");
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
		Rml::Element* grid = Get("npc-grid");
		if (!grid)
			return;
		SetScrollTop(grid->GetScrollTop() + deltaPx);
	}

	void UpdateScrollbar() {
		Rml::Element* grid = Get("npc-grid");
		Rml::Element* bar = Get("npc-scrollbar");
		Rml::Element* thumb = Get("npc-scroll-thumb");
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
		const float maxTop = trackH - thumbH;
		float top = 0.f;
		if (maxScroll > 0.f)
			top = (grid->GetScrollTop() / maxScroll) * maxTop;
		char hBuf[32];
		char tBuf[32];
		sprintf_s(hBuf, "%.0fdp", thumbH);
		sprintf_s(tBuf, "%.0fdp", top);
		thumb->SetProperty("height", hBuf);
		thumb->SetProperty("top", tBuf);
	}

	Rml::ElementPtr MakeSlot(const char* id, const RmlNpcSlotView& view) {
		Rml::ElementPtr cell = document->CreateElement("div");
		if (!cell)
			return cell;

		Rml::String classes = "npc-cell";
		if (view.iconPath.empty())
			classes += " npc-cell-empty";
		cell->SetClassNames(classes);
		if (id && id[0])
			cell->SetId(id);
		cell->SetAttribute("data-trade", view.id);
		cell->SetAttribute("data-trade-page", view.page);
		cell->AddEventListener(Rml::EventId::Dblclick, this);
		cell->AddEventListener(Rml::EventId::Dragstart, this);
		cell->AddEventListener(Rml::EventId::Dragend, this);
		cell->AddEventListener(Rml::EventId::Dragdrop, this);
		cell->AddEventListener(Rml::EventId::Mouseover, this);
		cell->AddEventListener(Rml::EventId::Mouseout, this);

		cell->SetProperty("display", "flex");
		cell->SetProperty("flex-direction", "column");
		cell->SetProperty("align-items", "center");
		cell->SetProperty("box-sizing", "border-box");
		cell->SetProperty("flex", "none");
		cell->SetProperty("width", "52dp");
		cell->SetProperty("height", "68dp");
		cell->SetProperty("min-width", "52dp");
		cell->SetProperty("min-height", "68dp");
		cell->SetProperty("padding", "0dp");
		cell->SetProperty("overflow", "visible");
		cell->SetProperty("background-color", "transparent");
		cell->SetProperty(Rml::PropertyId::Focus, Rml::Property(Rml::Style::Focus::None));
		cell->SetProperty("pointer-events", "auto");

		const bool filled = !view.iconPath.empty();
		cell->SetProperty(Rml::PropertyId::Drag,
						  Rml::Property(filled ? Rml::Style::Drag::Clone : Rml::Style::Drag::None));

		Rml::ElementPtr slot = document->CreateElement("div");
		if (slot) {
			slot->SetClassNames("npc-slot");
			slot->SetProperty("display", "block");
			slot->SetProperty("box-sizing", "border-box");
			slot->SetProperty("flex", "none");
			slot->SetProperty("width", "44dp");
			slot->SetProperty("height", "44dp");
			slot->SetProperty("padding", "4dp");
			slot->SetProperty("overflow", "visible");
			slot->SetProperty("background-color", "transparent");
			slot->SetProperty("decorator", "image(ui/rml/frames/notice/slot_rounded.tga)");
			slot->SetProperty("pointer-events", "none");

			if (filled) {
				Rml::ElementPtr img = document->CreateElement("img");
				if (img) {
					img->SetClassNames("npc-slot-icon");
					img->SetAttribute("src", view.iconPath.c_str());
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
					filler->SetClassNames("npc-slot-icon");
					filler->SetProperty("display", "block");
					filler->SetProperty("width", "36dp");
					filler->SetProperty("height", "36dp");
					filler->SetProperty("background-color", "transparent");
					slot->AppendChild(std::move(filler));
				}
			}
			cell->AppendChild(std::move(slot));
		}

		if (filled && !view.priceText.empty()) {
			Rml::ElementPtr priceEl = document->CreateElement("div");
			if (priceEl) {
				priceEl->SetClassNames("npc-slot-price");
				std::string label = view.priceText;
				label += " G";
				priceEl->SetInnerRML(EscapeXml(label.c_str()));
				priceEl->SetProperty("display", "block");
				priceEl->SetProperty("flex", "none");
				priceEl->SetProperty("width", "52dp");
				priceEl->SetProperty("height", "18dp");
				priceEl->SetProperty("line-height", "18dp");
				priceEl->SetProperty("margin-top", "4dp");
				priceEl->SetProperty("padding", "0dp 2dp");
				priceEl->SetProperty("font-size", "11dp");
				priceEl->SetProperty("font-weight", "bold");
				priceEl->SetProperty("text-align", "center");
				priceEl->SetProperty("color", "#8a5a10");
				priceEl->SetProperty("decorator",
									"tiled-horizontal(ui/rml/frames/notice/cap_pill_l.tga, ui/rml/frames/notice/cap_pill_c.tga, ui/rml/frames/notice/cap_pill_r.tga)");
				priceEl->SetProperty("image-color", "#f3e2b0");
				priceEl->SetProperty("pointer-events", "none");
				cell->AppendChild(std::move(priceEl));
			}
		}
		return cell;
	}

	void ProcessEvent(Rml::Event& event) override {
		const Rml::EventId idEv = event.GetId();
		Rml::Element* target = event.GetTargetElement();
		if (!target)
			return;

		if (idEv == Rml::EventId::Mouseover) {
			int page = -1, shop = -1;
			if (ResolveTradeSlot(target, page, shop)) {
				hoverPage = page;
				hoverShop = shop;
			}
			return;
		}
		if (idEv == Rml::EventId::Mouseout) {
			int page = -1, shop = -1;
			if (ResolveTradeSlot(target, page, shop)) {
				if (hoverPage == page && hoverShop == shop) {
					hoverPage = -1;
					hoverShop = -1;
				}
			}
			return;
		}

		if (idEv == Rml::EventId::Mousescroll) {
			const float wheelY = event.GetParameter<float>("wheel_delta_y", 0.f);
			if (wheelY != 0.f)
				ScrollBy(wheelY * 48.f);
			return;
		}

		if (idEv == Rml::EventId::Mousemove || idEv == Rml::EventId::Mouseup) {
			if (!thumbDragging)
				return;
			if (idEv == Rml::EventId::Mouseup) {
				thumbDragging = false;
				return;
			}
			Rml::Element* grid = Get("npc-grid");
			Rml::Element* bar = Get("npc-scrollbar");
			Rml::Element* thumb = Get("npc-scroll-thumb");
			if (!grid || !bar || !thumb)
				return;
			const float clientH = grid->GetClientHeight();
			const float scrollH = grid->GetScrollHeight();
			const float maxScroll = scrollH - clientH;
			if (maxScroll <= 1.f)
				return;
			const float trackH = bar->GetClientHeight();
			float thumbH = thumb->GetClientHeight();
			if (thumbH < 1.f)
				thumbH = 28.f;
			const float mouseY = event.GetParameter<float>("mouse_y", 0.f);
			const float delta = mouseY - thumbDragGrabY;
			const float travel = (std::max)(1.f, trackH - thumbH);
			SetScrollTop(thumbDragStartScroll + (delta / travel) * maxScroll);
			return;
		}

		if (idEv == Rml::EventId::Dragstart) {
			int page = -1, shop = -1;
			if (ResolveTradeSlot(target, page, shop))
				itemDragging = true;
			hoverPage = -1;
			hoverShop = -1;
			return;
		}
		if (idEv == Rml::EventId::Dragend) {
			itemDragging = false;
			int page = -1, shop = -1;
			ResolveTradeSlot(target, page, shop);
			const int mx = (int)event.GetParameter<float>("mouse_x", 0.f);
			const int my = (int)event.GetParameter<float>("mouse_y", 0.f);
			slotFingerprint.clear();
			RmlNpcTrade_OnDragEnd(page, shop, mx, my);
			return;
		}
		if (idEv == Rml::EventId::Dragdrop) {
			Rml::Element* dragEl = static_cast<Rml::Element*>(event.GetParameter<void*>("drag_element", nullptr));
			if (!dragEl)
				return;
			int srcBag = -1;
			int dstPage = -1, dstShop = -1;
			int srcPage = -1, srcShop = -1;
			ResolveTradeSlot(dragEl, srcPage, srcShop);
			ResolveBagIndex(dragEl, srcBag);
			ResolveTradeSlot(target, dstPage, dstShop);
			if (srcBag >= 0) {
				// Bag item dropped onto shop → sell.
				RmlNpcTrade_OnBagSell(srcBag);
				return;
			}
			if (srcPage >= 0 && srcShop >= 0) {
				// Shop rearrange ignored; buy goes via inventory drop.
				(void)dstShop;
				return;
			}
			return;
		}

		if (idEv == Rml::EventId::Dblclick) {
			int page = -1, shop = -1;
			if (ResolveTradeSlot(target, page, shop))
				RmlNpcTrade_OnBuy(page, shop, -1);
			return;
		}

		if (idEv != Rml::EventId::Click)
			return;

		Rml::Element* el = target;
		while (el && el != document) {
			const Rml::String& id = el->GetId();
			if (id == "btnNpcClose") {
				RmlNpcTrade_OnClose();
				return;
			}
			if (id == "btnNpcConfirmYes") {
				confirmModalOpen = false;
				SetModalOpen("npc-confirm-modal", false);
				RmlNpcTrade_OnConfirmYes();
				return;
			}
			if (id == "btnNpcConfirmNo" || id == "npc-confirm-scrim") {
				confirmModalOpen = false;
				SetModalOpen("npc-confirm-modal", false);
				RmlNpcTrade_OnConfirmNo();
				return;
			}
			if (id == "btnNpcQtyYes") {
				qtyModalOpen = false;
				SetModalOpen("npc-qty-modal", false);
				RmlNpcTrade_OnQtyYes();
				return;
			}
			if (id == "btnNpcQtyNo" || id == "npc-qty-scrim") {
				qtyModalOpen = false;
				SetModalOpen("npc-qty-modal", false);
				RmlNpcTrade_OnQtyNo();
				return;
			}
			if (id == "btnNpcQtyMinus") {
				if (qtyValue > 1) {
					qtyValue--;
					SyncQtyUi();
					RmlNpcTrade_OnQtyChanged(qtyValue);
				}
				return;
			}
			if (id == "btnNpcQtyPlus") {
				if (qtyValue < qtyMax) {
					qtyValue++;
					SyncQtyUi();
					RmlNpcTrade_OnQtyChanged(qtyValue);
				}
				return;
			}
			if (id == "npc-scroll-thumb") {
				thumbDragging = true;
				thumbDragGrabY = event.GetParameter<float>("mouse_y", 0.f);
				if (Rml::Element* grid = Get("npc-grid"))
					thumbDragStartScroll = grid->GetScrollTop();
				return;
			}
			if (el->HasAttribute("data-page")) {
				const int page = el->GetAttribute("data-page", 0);
				SetActiveTabUi(page);
				RmlNpcTrade_OnTab(page);
				return;
			}
			el = el->GetParentNode();
		}
	}
};

CRmlUiNpcTradeForm::CRmlUiNpcTradeForm() {
	m_impl = new Impl();
}

CRmlUiNpcTradeForm::~CRmlUiNpcTradeForm() {
	Unload();
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiNpcTradeForm& CRmlUiNpcTradeForm::Instance() {
	static CRmlUiNpcTradeForm instance;
	return instance;
}

bool CRmlUiNpcTradeForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;
	m_impl->context = context;
	m_impl->loadOk = false;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->document = context->LoadDocument("npctrade.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load npctrade.rml\n");
		return false;
	}

	static const char* kIds[] = {
		"btnNpcClose", "tabWeapon", "tabEquip", "tabOther", "npc-scroll-thumb", "npc-scrollbar",
		"btnNpcConfirmYes", "btnNpcConfirmNo", "npc-confirm-scrim",
		"btnNpcQtyYes", "btnNpcQtyNo", "npc-qty-scrim", "btnNpcQtyMinus", "btnNpcQtyPlus"};
	for (const char* id : kIds) {
		if (Rml::Element* el = m_impl->Get(id))
			el->AddEventListener(Rml::EventId::Click, m_impl);
	}
	if (Rml::Element* grid = m_impl->Get("npc-grid")) {
		grid->AddEventListener(Rml::EventId::Mousescroll, m_impl);
		grid->AddEventListener(Rml::EventId::Dragdrop, m_impl);
	}
	if (Rml::Element* root = m_impl->Get("npc-root")) {
		root->AddEventListener(Rml::EventId::Mousescroll, m_impl);
		root->AddEventListener(Rml::EventId::Dragdrop, m_impl);
	}
	m_impl->document->AddEventListener(Rml::EventId::Mousemove, m_impl);
	m_impl->document->AddEventListener(Rml::EventId::Mouseup, m_impl);

	m_impl->document->Hide();
	m_impl->SetModalOpen("npc-confirm-modal", false);
	m_impl->SetModalOpen("npc-qty-modal", false);
	m_impl->loadOk = true;
	OutputDebugStringA("RmlUi: npctrade.rml loaded\n");
	return true;
}

bool CRmlUiNpcTradeForm::LoadOk() const {
	return m_impl && m_impl->loadOk && m_impl->document;
}

void CRmlUiNpcTradeForm::Unload() {
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
	m_impl->goldCache.clear();
	m_impl->lastColumns = -1;
	m_impl->activePage = 0;
}

void CRmlUiNpcTradeForm::Show() {
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

void CRmlUiNpcTradeForm::Hide() {
	if (!m_impl)
		return;
	HideModals();
	if (m_impl->document)
		m_impl->document->Hide();
	m_impl->slotFingerprint.clear();
	m_impl->itemDragging = false;
	m_impl->thumbDragging = false;
	m_impl->hoverPage = -1;
	m_impl->hoverShop = -1;
}

bool CRmlUiNpcTradeForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

void CRmlUiNpcTradeForm::SetActivePage(int page) {
	if (!m_impl)
		return;
	if (page < 0)
		page = 0;
	if (page > 2)
		page = 2;
	m_impl->SetActiveTabUi(page);
}

int CRmlUiNpcTradeForm::GetActivePage() const {
	return m_impl ? m_impl->activePage : 0;
}

void CRmlUiNpcTradeForm::SetGold(const char* gold) {
	if (!m_impl)
		return;
	const std::string text = gold ? gold : "0";
	if (text == m_impl->goldCache)
		return;
	m_impl->goldCache = text;
	if (Rml::Element* el = m_impl->Get("labNpcGold"))
		el->SetInnerRML(EscapeXml(text.c_str()));
}

void CRmlUiNpcTradeForm::ShowBuyConfirm(const char* itemName, const char* priceText, const char* iconPath) {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->SetModalOpen("npc-qty-modal", false);
	m_impl->qtyModalOpen = false;
	if (Rml::Element* el = m_impl->Get("npc-confirm-title"))
		el->SetInnerRML("Buy Item");
	if (Rml::Element* el = m_impl->Get("npc-confirm-msg"))
		el->SetInnerRML("Do you wish to buy this item?");
	if (Rml::Element* el = m_impl->Get("npc-confirm-name"))
		el->SetInnerRML(EscapeXml(itemName ? itemName : ""));
	if (Rml::Element* el = m_impl->Get("npc-confirm-price")) {
		std::string price = "Price: ";
		price += priceText ? priceText : "0";
		el->SetInnerRML(EscapeXml(price.c_str()));
	}
	if (Rml::Element* el = m_impl->Get("npc-confirm-icon")) {
		if (iconPath && iconPath[0])
			el->SetAttribute("src", iconPath);
		else
			el->SetAttribute("src", "");
	}
	m_impl->SetModalOpen("npc-confirm-modal", true);
	m_impl->confirmModalOpen = true;
}

void CRmlUiNpcTradeForm::ShowSellConfirm(const char* itemName, const char* priceText, const char* iconPath) {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->SetModalOpen("npc-qty-modal", false);
	m_impl->qtyModalOpen = false;
	if (Rml::Element* el = m_impl->Get("npc-confirm-title"))
		el->SetInnerRML("Sell Item");
	if (Rml::Element* el = m_impl->Get("npc-confirm-msg"))
		el->SetInnerRML("Do you wish to sell this item?");
	if (Rml::Element* el = m_impl->Get("npc-confirm-name"))
		el->SetInnerRML(EscapeXml(itemName ? itemName : ""));
	if (Rml::Element* el = m_impl->Get("npc-confirm-price")) {
		std::string price = "You receive: ";
		price += priceText ? priceText : "0";
		el->SetInnerRML(EscapeXml(price.c_str()));
	}
	if (Rml::Element* el = m_impl->Get("npc-confirm-icon")) {
		if (iconPath && iconPath[0])
			el->SetAttribute("src", iconPath);
		else
			el->SetAttribute("src", "");
	}
	m_impl->SetModalOpen("npc-confirm-modal", true);
	m_impl->confirmModalOpen = true;
}

void CRmlUiNpcTradeForm::ShowQtyPrompt(bool isBuy, const char* itemName, const char* unitPriceText,
										 const char* iconPath, __int64 unitPrice, int maxQty) {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->SetModalOpen("npc-confirm-modal", false);
	m_impl->confirmModalOpen = false;
	m_impl->qtyUnitPrice = unitPrice > 0 ? unitPrice : 0;
	m_impl->qtyMax = maxQty > 0 ? maxQty : 1;
	m_impl->qtyValue = 1;
	if (Rml::Element* el = m_impl->Get("npc-qty-title"))
		el->SetInnerRML(isBuy ? "Buy Quantity" : "Sell Quantity");
	if (Rml::Element* el = m_impl->Get("npc-qty-name"))
		el->SetInnerRML(EscapeXml(itemName ? itemName : ""));
	if (Rml::Element* el = m_impl->Get("npc-qty-unit")) {
		std::string unit = "Unit: ";
		unit += unitPriceText ? unitPriceText : "0";
		el->SetInnerRML(EscapeXml(unit.c_str()));
	}
	if (Rml::Element* el = m_impl->Get("npc-qty-icon")) {
		if (iconPath && iconPath[0])
			el->SetAttribute("src", iconPath);
		else
			el->SetAttribute("src", "");
	}
	m_impl->SyncQtyUi();
	m_impl->SetModalOpen("npc-qty-modal", true);
	m_impl->qtyModalOpen = true;
}

void CRmlUiNpcTradeForm::HideModals() {
	if (!m_impl)
		return;
	m_impl->SetModalOpen("npc-confirm-modal", false);
	m_impl->SetModalOpen("npc-qty-modal", false);
	m_impl->confirmModalOpen = false;
	m_impl->qtyModalOpen = false;
}

bool CRmlUiNpcTradeForm::IsModalOpen() const {
	return m_impl && (m_impl->confirmModalOpen || m_impl->qtyModalOpen);
}

int CRmlUiNpcTradeForm::GetQtyValue() const {
	return m_impl ? m_impl->qtyValue : 1;
}

void CRmlUiNpcTradeForm::UpdateItemHint(int mouseX, int mouseY) {
	if (!m_impl || !IsVisible() || m_impl->itemDragging || m_impl->confirmModalOpen || m_impl->qtyModalOpen)
		return;
	if (m_impl->hoverPage < 0 || m_impl->hoverShop < 0)
		return;
	RmlNpcTrade_ApplyItemHint(m_impl->hoverPage, m_impl->hoverShop, mouseX, mouseY);
}

void CRmlUiNpcTradeForm::RenderItemHint() {
	if (!m_impl || !IsVisible() || m_impl->itemDragging || m_impl->confirmModalOpen || m_impl->qtyModalOpen)
		return;
	if (m_impl->hoverPage < 0 || m_impl->hoverShop < 0)
		return;
	RmlNpcTrade_RenderItemHint();
}

void CRmlUiNpcTradeForm::SetSlots(const std::vector<RmlNpcSlotView>& slots, int columns) {
	if (!m_impl || !m_impl->document)
		return;
	if (m_impl->itemDragging)
		return;

	const int cols = columns > 0 ? columns : 4;
	std::string fp = FingerprintSlots(slots);
	fp += '|';
	fp += std::to_string(m_impl->activePage);
	if (cols == m_impl->lastColumns && fp == m_impl->slotFingerprint)
		return;
	m_impl->lastColumns = cols;
	m_impl->slotFingerprint = std::move(fp);

	Rml::Element* grid = m_impl->Get("npc-grid");
	if (!grid)
		return;

	const float savedScroll = grid->GetScrollTop();
	m_impl->ClearChildren(grid);

	const int rowW = cols * 52 + (cols > 0 ? (cols - 1) * 6 : 0);
	char rowWBuf[32];
	sprintf_s(rowWBuf, "%ddp", rowW);

	Rml::Element* row = nullptr;
	int col = 0;
	for (const RmlNpcSlotView& view : slots) {
		if (col == 0) {
			Rml::ElementPtr rowPtr = m_impl->document->CreateElement("div");
			if (!rowPtr)
				break;
			rowPtr->SetClassNames("npc-row");
			rowPtr->SetProperty("display", "flex");
			rowPtr->SetProperty("flex-direction", "row");
			rowPtr->SetProperty("align-items", "flex-start");
			rowPtr->SetProperty("width", rowWBuf);
			rowPtr->SetProperty("height", "68dp");
			rowPtr->SetProperty("margin-bottom", "8dp");
			row = grid->AppendChild(std::move(rowPtr));
		}
		char idBuf[48];
		sprintf_s(idBuf, "npc-%d-%d", view.page, view.id);
		Rml::ElementPtr slot = m_impl->MakeSlot(idBuf, view);
		if (slot && row) {
			if (col + 1 >= cols)
				slot->SetProperty("margin", "0dp");
			else
				slot->SetProperty("margin", "0dp 6dp 0dp 0dp");
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

bool CRmlUiNpcTradeForm::ContainsScreenPoint(int x, int y) const {
	if (!IsVisible() || !m_impl)
		return false;
	Rml::Element* root = m_impl->Get("npc-root");
	if (!root)
		return false;
	const Rml::Vector2f off = root->GetAbsoluteOffset(Rml::BoxArea::Border);
	const Rml::Vector2f size = root->GetBox().GetSize(Rml::BoxArea::Border);
	return x >= off.x && y >= off.y && x <= off.x + size.x && y <= off.y + size.y;
}

void CRmlUiNpcTradeForm::PlaceBesideInventory() {
	if (!m_impl || !m_impl->document || !m_impl->context)
		return;
	Rml::Element* root = m_impl->Get("npc-root");
	if (!root)
		return;

	const Rml::Vector2i dim = m_impl->context->GetDimensions();
	Rml::Vector2f size = root->GetBox().GetSize(Rml::BoxArea::Border);
	if (size.x < 1.f)
		size.x = 280.f;
	if (size.y < 1.f)
		size.y = 560.f;

	float x = 40.f;
	float y = ((float)dim.y - size.y) * 0.5f;

	float invX = 0.f, invY = 0.f, invW = 0.f, invH = 0.f;
	if (CRmlUiInventoryForm::Instance().GetRootScreenRect(invX, invY, invW, invH) && invW > 0.f) {
		x = invX - size.x - 8.f;
		y = invY;
	}

	if (x < 8.f)
		x = 8.f;
	if (y < 8.f)
		y = 8.f;
	if (x + size.x > (float)dim.x - 8.f)
		x = (float)dim.x - size.x - 8.f;

	root->SetProperty(Rml::PropertyId::Left, Rml::Property(x, Rml::Unit::PX));
	root->SetProperty(Rml::PropertyId::Top, Rml::Property(y, Rml::Unit::PX));
}
