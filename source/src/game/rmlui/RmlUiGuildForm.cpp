#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiGuildForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/Property.h>
#include <RmlUi/Core/StyleTypes.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <windows.h>

extern void RmlGuild_OnClose();
extern void RmlGuild_OnTab(int tab);
extern void RmlGuild_OnRecruit();
extern void RmlGuild_OnRemove();
extern void RmlGuild_OnReject();
extern void RmlGuild_OnExit();
extern void RmlGuild_OnMotto();
extern void RmlGuild_OnMottoConfirm();
extern void RmlGuild_OnMottoCancel();
extern void RmlGuild_OnPermissions();
extern void RmlGuild_OnPermConfirm();
extern void RmlGuild_OnPermCancel();
extern void RmlGuild_OnPermPreset(int preset);
extern void RmlGuild_OnSelectMember(unsigned int id);
extern void RmlGuild_OnSelectApply(unsigned int id);
extern void RmlGuild_OnSort(int col);
extern void RmlGuild_OnLogPrev();
extern void RmlGuild_OnLogNext();
extern void RmlGuild_OnGoldTake();
extern void RmlGuild_OnGoldPut();
extern void RmlGuild_OnVaultDblClick(int index);
extern void RmlGuild_OnVaultDrop(int srcVault, int srcBag, int dstVault, int dstBag);
extern void RmlGuild_OnVaultDragEnd(int srcVault, int mouseX, int mouseY);
extern void RmlGuild_ApplyItemHint(int vaultIndex, int mouseX, int mouseY);
extern void RmlInv_RenderItemHint();

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

void SetText(Rml::Element* el, const std::string& text) {
	if (!el)
		return;
	el->SetInnerRML(EscapeXml(text.c_str()));
}

void SetBtnEnabled(Rml::Element* el, bool enabled) {
	if (!el)
		return;
	el->SetClass("notice-btn-disabled", !enabled);
	if (enabled)
		el->RemoveAttribute("disabled");
	else
		el->SetAttribute("disabled", true);
}

bool ResolveVaultIndex(Rml::Element* el, int& vaultIndex) {
	vaultIndex = -1;
	while (el) {
		if (el->HasAttribute("data-gvault")) {
			vaultIndex = el->GetAttribute("data-gvault", -1);
			return vaultIndex >= 0;
		}
		if (el->HasAttribute("data-bag") || el->HasAttribute("data-bank") || el->HasAttribute("data-equip"))
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
		if (el->HasAttribute("data-gvault") || el->HasAttribute("data-bank") || el->HasAttribute("data-equip"))
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

std::string FingerprintView(const CRmlUiGuildForm::GuildView& view) {
	std::string fp = view.name;
	fp += '|';
	fp += view.founder;
	fp += '|';
	fp += view.members;
	fp += '|';
	fp += view.exp;
	fp += '|';
	fp += view.gold;
	fp += '|';
	fp += view.level;
	fp += '|';
	fp += view.motto;
	char buf[96];
	sprintf_s(buf, ";%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:",
			  view.isLeader ? 1 : 0, view.recruitEnabled ? 1 : 0, view.removeEnabled ? 1 : 0,
			  view.rejectEnabled ? 1 : 0, view.footerVisible ? 1 : 0, view.tab, view.sortCol,
			  view.sortAsc ? 1 : 0, view.logPage, view.vaultLocked ? 1 : 0);
	fp += buf;
	fp += view.vaultGold;
	for (const auto& row : view.membersList) {
		sprintf_s(buf, ";m%u:%d:%d:", row.id, row.selected ? 1 : 0, row.online ? 1 : 0);
		fp += buf;
		fp += row.name;
		fp += row.job;
		fp += row.level;
	}
	for (const auto& row : view.applyList) {
		sprintf_s(buf, ";a%u:%d:", row.id, row.selected ? 1 : 0);
		fp += buf;
		fp += row.name;
		fp += row.job;
		fp += row.level;
	}
	for (const auto& log : view.logs)
		fp += log;
	return fp;
}

} // namespace

struct CRmlUiGuildForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	bool loadOk = false;
	bool hasPlaced = false;
	bool permOpen = false;
	bool mottoOpen = false;
	bool itemDragging = false;
	bool thumbDragging = false;
	float thumbDragGrabY = 0.f;
	int activeTab = 0;
	unsigned int selectedMemberId = 0;
	unsigned int selectedApplyId = 0;
	unsigned int permMask = 0;
	int hoverVault = -1;
	std::string viewFingerprint;
	std::string slotFingerprint;
	int lastColumns = -1;
	double lastClickTime = 0.0;
	int lastClickVault = -1;
	float lastClickX = 0.f;
	float lastClickY = 0.f;

	Rml::Element* Get(const char* id) const {
		return document ? document->GetElementById(id) : nullptr;
	}

	void ClearChildren(Rml::Element* parent) {
		if (!parent)
			return;
		while (Rml::Element* child = parent->GetFirstChild())
			parent->RemoveChild(child);
	}

	void PlaceDefault() {
		Rml::Element* root = Get("guild-root");
		if (!root || !context)
			return;
		const Rml::Vector2i dim = context->GetDimensions();
		float x = ((float)dim.x - 455.f) * 0.5f;
		float y = ((float)dim.y - 373.f) * 0.22f;
		if (x < 24.f)
			x = 24.f;
		if (y < 32.f)
			y = 32.f;
		root->SetProperty(Rml::PropertyId::Left, Rml::Property(x, Rml::Unit::PX));
		root->SetProperty(Rml::PropertyId::Top, Rml::Property(y, Rml::Unit::PX));
	}

	void SyncScrim() {
		if (Rml::Element* scrim = Get("guild-perm-scrim"))
			scrim->SetClass("guild-perm-scrim-open", permOpen || mottoOpen);
	}

	void SetPermOpen(bool open) {
		permOpen = open;
		if (Rml::Element* modal = Get("guild-perm-modal"))
			modal->SetClass("guild-perm-modal-open", open);
		SyncScrim();
	}

	void SetMottoOpen(bool open) {
		mottoOpen = open;
		if (Rml::Element* modal = Get("guild-motto-modal"))
			modal->SetClass("guild-motto-modal-open", open);
		SyncScrim();
	}

	void SetModalOpen(bool open) {
		SetPermOpen(open);
		if (!open)
			SetMottoOpen(false);
	}

	void ApplyPermButtons(unsigned int mask) {
		permMask = mask;
		for (int i = 0; i < 12; ++i) {
			char id[16];
			sprintf_s(id, "perm%d", i);
			if (Rml::Element* el = Get(id))
				el->SetClass("guild-perm-on", (mask & (1u << i)) != 0);
		}
	}

	void RebuildRows(Rml::Element* list, const std::vector<CRmlUiGuildForm::MemberRow>& rows, const char* kind) {
		if (!list || !document)
			return;
		ClearChildren(list);
		for (const auto& row : rows) {
			Rml::ElementPtr btn = document->CreateElement("button");
			if (!btn)
				break;
			Rml::String classes = "guild-row";
			if (row.selected)
				classes += " guild-row-sel";
			if (!row.online)
				classes += " guild-row-offline";
			btn->SetClassNames(classes);
			btn->SetAttribute("type", "button");
			btn->SetAttribute("data-kind", kind);
			btn->SetAttribute("data-id", (int)row.id);
			btn->AddEventListener(Rml::EventId::Click, this);

			auto addCell = [&](const char* cls, const std::string& text) {
				Rml::ElementPtr cell = document->CreateElement("div");
				if (!cell)
					return;
				cell->SetClassNames(cls);
				cell->SetInnerRML(EscapeXml(text.c_str()));
				btn->AppendChild(std::move(cell));
			};
			addCell("guild-cell guild-cell-name", row.name);
			addCell("guild-cell guild-cell-class", row.job);
			addCell("guild-cell guild-cell-level", row.level);
			list->AppendChild(std::move(btn));
		}
	}

	void RebuildLogs(const std::vector<std::string>& logs) {
		Rml::Element* list = Get("logRows");
		if (!list || !document)
			return;
		ClearChildren(list);
		for (const auto& line : logs) {
			Rml::ElementPtr el = document->CreateElement("div");
			if (!el)
				break;
			el->SetClassNames("guild-log");
			el->SetInnerRML(EscapeXml(line.c_str()));
			list->AppendChild(std::move(el));
		}
	}

	void SetSortHeader(const char* id, int col, int sortCol, bool sortAsc) {
		Rml::Element* el = Get(id);
		if (!el)
			return;
		const char* base = "Name";
		if (col == 1)
			base = "Class";
		else if (col == 2)
			base = "Level";
		std::string label = base;
		if (sortCol == col)
			label += sortAsc ? " ^" : " v";
		SetText(el, label);
	}

	void SetScrollTop(float scrollTop) {
		Rml::Element* grid = Get("vault-grid");
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
		Rml::Element* grid = Get("vault-grid");
		if (!grid)
			return;
		SetScrollTop(grid->GetScrollTop() + deltaPx);
	}

	void UpdateScrollbar() {
		Rml::Element* grid = Get("vault-grid");
		Rml::Element* bar = Get("vault-scrollbar");
		Rml::Element* thumb = Get("vault-scroll-thumb");
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

		Rml::String classes = "guild-slot";
		if (view.locked)
			classes += " guild-slot-locked";
		slot->SetClassNames(classes);
		if (id && id[0])
			slot->SetId(id);
		slot->SetAttribute("data-gvault", view.id);
		slot->AddEventListener(Rml::EventId::Dblclick, this);
		slot->AddEventListener(Rml::EventId::Mousedown, this);
		slot->AddEventListener(Rml::EventId::Mouseup, this);
		slot->AddEventListener(Rml::EventId::Mousemove, this);
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
				img->SetClassNames("guild-slot-icon");
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
					qtyEl->SetClassNames("guild-slot-qty");
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
				filler->SetClassNames("guild-slot-icon");
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

	bool TryManualDoubleClick(int vault, float mouseX, float mouseY) {
		const double now = Rml::GetSystemInterface() ? Rml::GetSystemInterface()->GetElapsedTime() : 0.0;
		const float dx = mouseX - lastClickX;
		const float dy = mouseY - lastClickY;
		const bool same = (vault >= 0 && vault == lastClickVault);
		const bool quick = (now - lastClickTime) < 0.40;
		const bool closeEnough = (dx * dx + dy * dy) < (10.f * 10.f);
		lastClickVault = vault;
		lastClickTime = now;
		lastClickX = mouseX;
		lastClickY = mouseY;
		if (same && quick && closeEnough && vault >= 0) {
			RmlGuild_OnVaultDblClick(vault);
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
		const bool isKeyDown = (event == Rml::EventId::Keydown);

		if (isKeyDown && mottoOpen) {
			const auto key = static_cast<Rml::Input::KeyIdentifier>(event.GetParameter<int>("key_identifier", 0));
			if (key == Rml::Input::KI_RETURN || key == Rml::Input::KI_NUMPADENTER) {
				event.StopPropagation();
				RmlGuild_OnMottoConfirm();
			}
			return;
		}

		if (isScroll) {
			const float wheel = event.GetParameter<float>("wheel_delta_y", 0.f);
			if (wheel != 0.f && activeTab == 2) {
				ScrollBy(-wheel * 48.f);
				event.StopPropagation();
			}
			return;
		}

		if (isMouseDown) {
			if (target->GetId() == "vault-scroll-thumb" || target->GetId() == "vault-scrollbar") {
				Rml::Element* thumb = Get("vault-scroll-thumb");
				Rml::Element* bar = Get("vault-scrollbar");
				if (thumb && bar) {
					thumbDragging = true;
					const float mouseY = event.GetParameter<float>("mouse_y", 0.f);
					const float thumbTop = thumb->GetAbsoluteOffset(Rml::BoxArea::Border).y;
					thumbDragGrabY = mouseY - thumbTop;
					event.StopPropagation();
				}
				return;
			}
			int vault = -1;
			if (ResolveVaultIndex(target, vault) && vault >= 0) {
				hoverVault = vault;
				const float mx = event.GetParameter<float>("mouse_x", 0.f);
				const float my = event.GetParameter<float>("mouse_y", 0.f);
				if (TryManualDoubleClick(vault, mx, my)) {
					event.StopPropagation();
					return;
				}
			}
		}

		if (isMouseMove) {
			if (thumbDragging) {
				Rml::Element* bar = Get("vault-scrollbar");
				Rml::Element* grid = Get("vault-grid");
				Rml::Element* thumb = Get("vault-scroll-thumb");
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
			int vault = -1;
			hoverVault = ResolveVaultIndex(target, vault) ? vault : -1;
			return;
		}

		if (isMouseUp) {
			thumbDragging = false;
			return;
		}

		if (isDragStart) {
			int vault = -1;
			if (ResolveVaultIndex(target, vault))
				itemDragging = true;
			return;
		}

		if (isDragEnd) {
			itemDragging = false;
			int vault = -1;
			ResolveVaultIndex(target, vault);
			const int mx = (int)event.GetParameter<float>("mouse_x", 0.f);
			const int my = (int)event.GetParameter<float>("mouse_y", 0.f);
			slotFingerprint.clear();
			RmlGuild_OnVaultDragEnd(vault, mx, my);
			return;
		}

		if (isDragDrop) {
			Rml::Element* dragEl = static_cast<Rml::Element*>(event.GetParameter<void*>("drag_element", nullptr));
			if (!dragEl)
				return;
			int srcVault = -1, srcBag = -1, dstVault = -1;
			ResolveVaultIndex(dragEl, srcVault);
			ResolveBagIndex(dragEl, srcBag);
			if (!ResolveVaultIndex(target, dstVault))
				return;
			if (srcVault < 0 && srcBag < 0)
				return;
			if (srcVault >= 0 && srcVault == dstVault)
				return;
			RmlGuild_OnVaultDrop(srcVault, srcBag, dstVault, -1);
			return;
		}

		if (isDblClick) {
			int vault = -1;
			if (ResolveVaultIndex(target, vault) && vault >= 0) {
				RmlGuild_OnVaultDblClick(vault);
				return;
			}
		}

		if (!isClick)
			return;

		Rml::Element* el = target;
		while (el && el != document) {
			const Rml::String& id = el->GetId();
			if (id == "btnGuildClose") {
				event.StopPropagation();
				RmlGuild_OnClose();
				return;
			}
			if (id == "btnRecruit") {
				RmlGuild_OnRecruit();
				return;
			}
			if (id == "btnRemove") {
				RmlGuild_OnRemove();
				return;
			}
			if (id == "btnReject") {
				RmlGuild_OnReject();
				return;
			}
			if (id == "btnExit") {
				RmlGuild_OnExit();
				return;
			}
			if (id == "btnMotto") {
				RmlGuild_OnMotto();
				return;
			}
			if (id == "btnMottoConfirm") {
				RmlGuild_OnMottoConfirm();
				return;
			}
			if (id == "btnMottoCancel") {
				RmlGuild_OnMottoCancel();
				return;
			}
			if (id == "btnPerms") {
				RmlGuild_OnPermissions();
				return;
			}
			if (id == "btnPermConfirm") {
				RmlGuild_OnPermConfirm();
				return;
			}
			if (id == "btnPermCancel" || id == "guild-perm-scrim") {
				if (mottoOpen)
					RmlGuild_OnMottoCancel();
				else
					RmlGuild_OnPermCancel();
				return;
			}
			if (id == "tabMembers") {
				RmlGuild_OnTab(0);
				return;
			}
			if (id == "tabApply") {
				RmlGuild_OnTab(1);
				return;
			}
			if (id == "tabVault") {
				RmlGuild_OnTab(2);
				return;
			}
			if (id == "tabLogs") {
				RmlGuild_OnTab(3);
				return;
			}
			if (id == "sortName" || id == "sortApplyName") {
				RmlGuild_OnSort(0);
				return;
			}
			if (id == "sortClass" || id == "sortApplyClass") {
				RmlGuild_OnSort(1);
				return;
			}
			if (id == "sortLevel" || id == "sortApplyLevel") {
				RmlGuild_OnSort(2);
				return;
			}
			if (id == "btnLogPrev") {
				RmlGuild_OnLogPrev();
				return;
			}
			if (id == "btnLogNext") {
				RmlGuild_OnLogNext();
				return;
			}
			if (id == "btnGoldTake") {
				RmlGuild_OnGoldTake();
				return;
			}
			if (id == "btnGoldPut") {
				RmlGuild_OnGoldPut();
				return;
			}
			if (id.size() == 11 && id.substr(0, 10) == "permPredef") {
				const int preset = id[10] - '0';
				if (preset >= 1 && preset <= 6)
					RmlGuild_OnPermPreset(preset);
				return;
			}
			if (el->HasAttribute("data-perm")) {
				const int bit = el->GetAttribute("data-perm", -1);
				if (bit >= 0 && bit < 12) {
					permMask ^= (1u << bit);
					ApplyPermButtons(permMask);
				}
				return;
			}
			if (el->HasAttribute("data-id") && el->HasAttribute("data-kind")) {
				const unsigned int rowId = (unsigned int)el->GetAttribute("data-id", 0);
				const Rml::String kind = el->GetAttribute("data-kind", Rml::String());
				if (kind == "member")
					RmlGuild_OnSelectMember(rowId);
				else if (kind == "apply")
					RmlGuild_OnSelectApply(rowId);
				return;
			}
			el = el->GetParentNode();
		}
	}
};

CRmlUiGuildForm::CRmlUiGuildForm() : m_impl(new Impl) {}
CRmlUiGuildForm::~CRmlUiGuildForm() {
	Unload();
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiGuildForm& CRmlUiGuildForm::Instance() {
	static CRmlUiGuildForm instance;
	return instance;
}

bool CRmlUiGuildForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;

	m_impl->context = context;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}

	m_impl->document = context->LoadDocument("guild.rml");
	if (!m_impl->document) {
		m_impl->loadOk = false;
		OutputDebugStringA("RmlUi: failed to load guild.rml\n");
		return false;
	}

	static const char* kClick[] = {
		"btnGuildClose", "btnRecruit", "btnRemove", "btnReject", "btnExit",
		"btnMotto", "btnMottoConfirm", "btnMottoCancel",
		"btnPerms", "btnPermConfirm", "btnPermCancel", "guild-perm-scrim",
		"tabMembers", "tabApply", "tabVault", "tabLogs",
		"sortName", "sortClass", "sortLevel",
		"sortApplyName", "sortApplyClass", "sortApplyLevel",
		"btnLogPrev", "btnLogNext", "btnGoldTake", "btnGoldPut",
		"permPredef1", "permPredef2", "permPredef3", "permPredef4", "permPredef5", "permPredef6",
		"perm0", "perm1", "perm2", "perm3", "perm4", "perm5",
		"perm6", "perm7", "perm8", "perm9", "perm10", "perm11",
	};
	for (const char* id : kClick) {
		if (Rml::Element* el = m_impl->Get(id))
			el->AddEventListener(Rml::EventId::Click, m_impl);
	}
	if (Rml::Element* grid = m_impl->Get("vault-grid"))
		grid->AddEventListener(Rml::EventId::Mousescroll, m_impl);
	if (Rml::Element* bar = m_impl->Get("vault-scrollbar")) {
		bar->AddEventListener(Rml::EventId::Mousedown, m_impl);
		bar->AddEventListener(Rml::EventId::Mousemove, m_impl);
		bar->AddEventListener(Rml::EventId::Mouseup, m_impl);
	}
	if (Rml::Element* thumb = m_impl->Get("vault-scroll-thumb")) {
		thumb->AddEventListener(Rml::EventId::Mousedown, m_impl);
		thumb->AddEventListener(Rml::EventId::Mousemove, m_impl);
		thumb->AddEventListener(Rml::EventId::Mouseup, m_impl);
	}
	if (Rml::Element* mottoEdit = m_impl->Get("edtMottoEdit"))
		mottoEdit->AddEventListener(Rml::EventId::Keydown, m_impl);

	m_impl->document->Hide();
	m_impl->loadOk = true;
	OutputDebugStringA("RmlUi: guild.rml loaded\n");
	return true;
}

void CRmlUiGuildForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
	m_impl->loadOk = false;
	m_impl->hasPlaced = false;
	m_impl->viewFingerprint.clear();
	m_impl->slotFingerprint.clear();
}

bool CRmlUiGuildForm::LoadOk() const {
	return m_impl && m_impl->loadOk && m_impl->document;
}

void CRmlUiGuildForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
	if (Rml::Element* root = m_impl->Get("guild-root"))
		root->SetClass("guild-root-open", true);
	if (!m_impl->hasPlaced) {
		if (m_impl->context)
			m_impl->context->Update();
		m_impl->PlaceDefault();
		m_impl->hasPlaced = true;
	}
}

void CRmlUiGuildForm::Hide() {
	if (!m_impl)
		return;
	HideModal();
	if (Rml::Element* root = m_impl->Get("guild-root"))
		root->SetClass("guild-root-open", false);
	if (m_impl->document)
		m_impl->document->Hide();
	m_impl->viewFingerprint.clear();
	m_impl->hoverVault = -1;
}

bool CRmlUiGuildForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

bool CRmlUiGuildForm::IsModalOpen() const {
	return m_impl && (m_impl->permOpen || m_impl->mottoOpen);
}

void CRmlUiGuildForm::HideModal() {
	if (m_impl)
		m_impl->SetModalOpen(false);
}

void CRmlUiGuildForm::ApplyView(const GuildView& view) {
	if (!m_impl || !m_impl->document)
		return;
	const std::string fp = FingerprintView(view);
	if (fp == m_impl->viewFingerprint && IsVisible())
		return;
	m_impl->viewFingerprint = fp;
	m_impl->activeTab = view.tab;
	m_impl->selectedMemberId = 0;
	m_impl->selectedApplyId = 0;
	for (const auto& row : view.membersList) {
		if (row.selected)
			m_impl->selectedMemberId = row.id;
	}
	for (const auto& row : view.applyList) {
		if (row.selected)
			m_impl->selectedApplyId = row.id;
	}

	SetText(m_impl->Get("labName"), view.name);
	SetText(m_impl->Get("labFounder"), view.founder);
	SetText(m_impl->Get("labMembers"), view.members);
	SetText(m_impl->Get("labExp"), view.exp);
	SetText(m_impl->Get("labGold"), view.gold);
	SetText(m_impl->Get("labLevel"), view.level);
	SetText(m_impl->Get("labVaultGold"), view.vaultGold.empty() ? view.gold : view.vaultGold);
	if (Rml::Element* motto = m_impl->Get("edtMotto")) {
		if (auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(motto)) {
			Rml::Element* focus = m_impl->context ? m_impl->context->GetFocusElement() : nullptr;
			if (focus != motto)
				input->SetValue(view.motto.c_str());
		}
	}

	SetBtnEnabled(m_impl->Get("btnRecruit"), view.recruitEnabled);
	SetBtnEnabled(m_impl->Get("btnRemove"), view.removeEnabled);
	SetBtnEnabled(m_impl->Get("btnReject"), view.rejectEnabled);
	if (Rml::Element* exitBtn = m_impl->Get("btnExit"))
		SetText(exitBtn, view.isLeader ? "Disband" : "Exit");

	static const char* kTabs[] = {"tabMembers", "tabApply", "tabVault", "tabLogs"};
	static const char* kPages[] = {"pageMembers", "pageApply", "pageVault", "pageLogs"};
	for (int i = 0; i < 4; ++i) {
		if (Rml::Element* tab = m_impl->Get(kTabs[i]))
			tab->SetClass("guild-tab-active", view.tab == i);
		if (Rml::Element* page = m_impl->Get(kPages[i]))
			page->SetClass("guild-page-hidden", view.tab != i);
	}

	if (Rml::Element* footer = m_impl->Get("guild-footer"))
		footer->SetClass("guild-footer-hidden", !view.footerVisible);

	m_impl->SetSortHeader("sortName", 0, view.sortCol, view.sortAsc);
	m_impl->SetSortHeader("sortClass", 1, view.sortCol, view.sortAsc);
	m_impl->SetSortHeader("sortLevel", 2, view.sortCol, view.sortAsc);
	m_impl->SetSortHeader("sortApplyName", 0, view.sortCol, view.sortAsc);
	m_impl->SetSortHeader("sortApplyClass", 1, view.sortCol, view.sortAsc);
	m_impl->SetSortHeader("sortApplyLevel", 2, view.sortCol, view.sortAsc);

	m_impl->RebuildRows(m_impl->Get("memberRows"), view.membersList, "member");
	m_impl->RebuildRows(m_impl->Get("applyRows"), view.applyList, "apply");
	m_impl->RebuildLogs(view.logs);

	char pageBuf[16];
	sprintf_s(pageBuf, "%d", view.logPage);
	SetText(m_impl->Get("labLogPage"), pageBuf);
	SetBtnEnabled(m_impl->Get("btnLogPrev"), view.logPrevEnabled);
	SetBtnEnabled(m_impl->Get("btnLogNext"), view.logNextEnabled);

	if (Rml::Element* locked = m_impl->Get("vaultLocked"))
		locked->SetClass("guild-vault-locked-on", view.vaultLocked);
	SetBtnEnabled(m_impl->Get("btnGoldTake"), !view.vaultLocked);
	SetBtnEnabled(m_impl->Get("btnGoldPut"), !view.vaultLocked);

	Show();
	if (m_impl->context)
		m_impl->context->Update();
	if (!m_impl->hasPlaced)
		m_impl->PlaceDefault();
}

void CRmlUiGuildForm::SetVaultSlots(const std::vector<RmlInvSlotView>& slots, int columns) {
	if (!m_impl || !m_impl->document)
		return;
	if (m_impl->itemDragging)
		return;

	const int cols = columns > 0 ? columns : 5;
	std::string fp = FingerprintSlots(slots);
	if (cols == m_impl->lastColumns && fp == m_impl->slotFingerprint)
		return;
	m_impl->lastColumns = cols;
	m_impl->slotFingerprint = std::move(fp);

	Rml::Element* grid = m_impl->Get("vault-grid");
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
			rowPtr->SetClassNames("guild-vault-row");
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
		sprintf_s(idBuf, "gvault-%d", view.id);
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

void CRmlUiGuildForm::SetGold(const char* gold) {
	if (!m_impl)
		return;
	const char* text = gold ? gold : "";
	SetText(m_impl->Get("labGold"), text);
	SetText(m_impl->Get("labVaultGold"), text);
}

int CRmlUiGuildForm::GetActiveTab() const {
	return m_impl ? m_impl->activeTab : 0;
}

unsigned int CRmlUiGuildForm::GetSelectedMemberId() const {
	return m_impl ? m_impl->selectedMemberId : 0;
}

unsigned int CRmlUiGuildForm::GetSelectedApplyId() const {
	return m_impl ? m_impl->selectedApplyId : 0;
}

std::string CRmlUiGuildForm::GetMottoInput() const {
	if (!m_impl)
		return {};
	Rml::Element* el = m_impl->Get("edtMotto");
	if (!el)
		return {};
	auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(el);
	if (!input)
		return {};
	return std::string(input->GetValue().c_str());
}

std::string CRmlUiGuildForm::GetMottoEditInput() const {
	if (!m_impl)
		return {};
	Rml::Element* el = m_impl->Get("edtMottoEdit");
	if (!el)
		return {};
	auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(el);
	if (!input)
		return {};
	return std::string(input->GetValue().c_str());
}

unsigned int CRmlUiGuildForm::GetPermissionMask() const {
	return m_impl ? m_impl->permMask : 0;
}

void CRmlUiGuildForm::SetSelectedMember(unsigned int id) {
	if (m_impl)
		m_impl->selectedMemberId = id;
}

void CRmlUiGuildForm::SetSelectedApply(unsigned int id) {
	if (m_impl)
		m_impl->selectedApplyId = id;
}

void CRmlUiGuildForm::SetPermissionMask(unsigned int mask, const char* memberName) {
	if (!m_impl)
		return;
	m_impl->ApplyPermButtons(mask);
	SetText(m_impl->Get("permMember"), memberName ? memberName : "Select a member");
}

void CRmlUiGuildForm::ShowPermissions(bool show) {
	if (!m_impl)
		return;
	if (show)
		m_impl->SetMottoOpen(false);
	m_impl->SetPermOpen(show);
}

void CRmlUiGuildForm::ShowMottoEdit(const char* motto) {
	if (!m_impl)
		return;
	m_impl->SetPermOpen(false);
	if (Rml::Element* el = m_impl->Get("edtMottoEdit")) {
		if (auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(el))
			input->SetValue(motto ? motto : "");
		el->Focus();
	}
	m_impl->SetMottoOpen(true);
}

bool CRmlUiGuildForm::ContainsScreenPoint(int x, int y) const {
	if (!IsVisible() || !m_impl)
		return false;
	Rml::Element* root = m_impl->Get("guild-root");
	if (!root)
		return false;
	const Rml::Vector2f off = root->GetAbsoluteOffset(Rml::BoxArea::Border);
	const Rml::Vector2f size = root->GetBox().GetSize(Rml::BoxArea::Border);
	return x >= off.x && y >= off.y && x <= off.x + size.x && y <= off.y + size.y;
}

bool CRmlUiGuildForm::ContainsVaultPoint(int x, int y) const {
	if (!IsVisible() || !m_impl || m_impl->activeTab != 2)
		return false;
	return ContainsScreenPoint(x, y);
}

void CRmlUiGuildForm::UpdateItemHint(int mouseX, int mouseY) {
	if (!m_impl || !IsVisible() || m_impl->itemDragging || m_impl->permOpen)
		return;
	if (m_impl->hoverVault < 0)
		return;
	RmlGuild_ApplyItemHint(m_impl->hoverVault, mouseX, mouseY);
}

void CRmlUiGuildForm::RenderItemHint() {
	if (!m_impl || !IsVisible() || m_impl->itemDragging || m_impl->permOpen)
		return;
	if (m_impl->hoverVault < 0)
		return;
	RmlInv_RenderItemHint();
}
