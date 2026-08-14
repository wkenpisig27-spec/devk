#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiSkillForm.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Property.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <windows.h>

extern void RmlSkill_OnClose();
extern void RmlSkill_OnTab(const char* filter);
extern void RmlSkill_OnSelect(int skillId);
extern void RmlSkill_OnUse(int skillId);
extern void RmlSkill_OnUpgrade(int skillId);

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
		case '\n': out += "<br/>"; break;
		default: out += *p; break;
		}
	}
	return out;
}

void SetTextCached(Rml::Element* el, std::string& cache, const std::string& text) {
	if (!el || cache == text)
		return;
	cache = text;
	el->SetInnerRML(EscapeXml(text.c_str()));
}

std::string DetailFingerprint(const CRmlUiSkillForm::DetailView& d) {
	std::string fp;
	char buf[64];
	sprintf_s(buf, "%d:%d:%d|", d.visible ? 1 : 0, d.skillId, d.canUpgrade ? 1 : 0);
	fp += buf;
	fp += d.name;
	fp += '|';
	fp += d.levelText;
	fp += '|';
	fp += d.description;
	fp += '|';
	fp += d.effect;
	fp += '|';
	fp += d.cost;
	fp += '|';
	fp += d.nextTitle;
	fp += '|';
	fp += d.nextBody;
	fp += '|';
	fp += d.nextReq;
	fp += '|';
	fp += d.iconPath;
	for (size_t i = 0; i < d.tags.size(); ++i) {
		fp += ';';
		fp += d.tags[i];
		fp += (i < d.tagGold.size() && d.tagGold[i]) ? '1' : '0';
	}
	for (const auto& s : d.stats) {
		fp += ';';
		fp += s.label;
		fp += '=';
		fp += s.value;
		fp += s.highlightNext ? '!' : '.';
	}
	return fp;
}

} // namespace

struct CRmlUiSkillForm::Impl : public Rml::EventListener {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	bool loadOk = false;
	bool hasPlacedRoot = false;
	std::string activeTab = "fight";
	int selectedSkillId = -1;
	std::string fightPtsCache;
	std::string lifePtsCache;
	std::string listFingerprint;
	std::string detailFingerprint;
	std::string sdNameCache;
	std::string sdLevelCache;
	std::string sdDescCache;
	std::string sdEffectCache;
	std::string sdCostCache;
	std::string sdNextTitleCache;
	std::string sdNextBodyCache;
	std::string sdNextReqCache;
	std::string sdIconCache;
	bool thumbDragging = false;
	float thumbDragGrabY = 0.f;
	float thumbDragStartScroll = 0.f;
	bool detailVisible = false;

	Rml::Element* Get(const char* id) const {
		return document ? document->GetElementById(id) : nullptr;
	}

	void ClearChildren(Rml::Element* parent) {
		if (!parent)
			return;
		while (Rml::Element* child = parent->GetFirstChild())
			parent->RemoveChild(child);
	}

	void SetListScrollTop(float scrollTop) {
		Rml::Element* list = Get("skill-list");
		if (!list)
			return;
		const float maxScroll = (std::max)(0.f, list->GetScrollHeight() - list->GetClientHeight());
		if (scrollTop < 0.f)
			scrollTop = 0.f;
		if (scrollTop > maxScroll)
			scrollTop = maxScroll;
		list->SetScrollTop(scrollTop);
		UpdateScrollbar();
	}

	void ScrollListBy(float deltaPx) {
		Rml::Element* list = Get("skill-list");
		if (!list)
			return;
		SetListScrollTop(list->GetScrollTop() + deltaPx);
	}

	void UpdateScrollbar() {
		Rml::Element* list = Get("skill-list");
		Rml::Element* bar = Get("skill-scrollbar");
		Rml::Element* thumb = Get("skill-scroll-thumb");
		if (!list || !bar || !thumb)
			return;

		const float clientH = list->GetClientHeight();
		const float scrollH = list->GetScrollHeight();
		const float maxScroll = scrollH - clientH;
		const bool needScroll = maxScroll > 1.f;

		bar->SetProperty("opacity", needScroll ? "1" : "0.35");
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

		const float scrollRatio = list->GetScrollTop() / maxScroll;
		const float thumbTop = scrollRatio * (trackH - thumbH);
		thumb->SetProperty(Rml::PropertyId::Height, Rml::Property(thumbH, Rml::Unit::PX));
		thumb->SetProperty(Rml::PropertyId::Top, Rml::Property(thumbTop, Rml::Unit::PX));
	}

	void SetActiveTab(const char* id) {
		static const char* kTabs[] = {"tabFight", "tabLife", "tabSail"};
		for (const char* tabId : kTabs) {
			if (Rml::Element* el = Get(tabId))
				el->SetClass("skill-tab-active", id && strcmp(tabId, id) == 0);
		}
	}

	void ApplyDetail(const CRmlUiSkillForm::DetailView& detail) {
		Rml::Element* panel = Get("skill-detail");
		if (!panel)
			return;

		const std::string fp = DetailFingerprint(detail);
		const bool wasVisible = detailVisible;
		detailVisible = detail.visible;
		panel->SetClass("skill-detail-hidden", !detail.visible);

		if (!detail.visible) {
			detailFingerprint = fp;
			return;
		}

		if (fp == detailFingerprint && wasVisible)
			return;
		detailFingerprint = fp;

		if (Rml::Element* icon = Get("sd-icon")) {
			if (detail.iconPath != sdIconCache) {
				sdIconCache = detail.iconPath;
				if (!detail.iconPath.empty())
					icon->SetAttribute("src", detail.iconPath.c_str());
			}
		}

		SetTextCached(Get("sd-name"), sdNameCache, detail.name);
		SetTextCached(Get("sd-level"), sdLevelCache, detail.levelText);
		SetTextCached(Get("sd-desc"), sdDescCache, detail.description.empty() ? "-" : detail.description);
		SetTextCached(Get("sd-effect"), sdEffectCache, detail.effect.empty() ? "-" : detail.effect);
		SetTextCached(Get("sd-cost"), sdCostCache, detail.cost.empty() ? "-" : detail.cost);
		SetTextCached(Get("sd-next-title"), sdNextTitleCache, detail.nextTitle);
		SetTextCached(Get("sd-next-body"), sdNextBodyCache, detail.nextBody.empty() ? "-" : detail.nextBody);
		SetTextCached(Get("sd-next-req"), sdNextReqCache, detail.nextReq);

		if (Rml::Element* costSection = Get("sd-cost-section")) {
			const bool showCost = !detail.cost.empty() && detail.cost != "-" && detail.cost != "Passive";
			costSection->SetClass("skill-detail-section-hidden", !showCost);
		}

		if (Rml::Element* tags = Get("sd-tags")) {
			ClearChildren(tags);
			for (size_t i = 0; i < detail.tags.size(); ++i) {
				Rml::ElementPtr tag = document->CreateElement("div");
				if (!tag)
					break;
				const bool gold = i < detail.tagGold.size() && detail.tagGold[i];
				tag->SetClassNames(gold ? "skill-detail-tag skill-detail-tag-gold" : "skill-detail-tag");
				tag->SetInnerRML(EscapeXml(detail.tags[i].c_str()));
				tags->AppendChild(std::move(tag));
			}
		}

		if (Rml::Element* stats = Get("sd-stats")) {
			ClearChildren(stats);
			for (const auto& st : detail.stats) {
				Rml::ElementPtr row = document->CreateElement("div");
				if (!row)
					break;
				row->SetClassNames("skill-detail-stat");
				row->SetProperty("display", "flex");
				row->SetProperty("flex-direction", "row");
				row->SetProperty("width", "308dp");
				Rml::ElementPtr lab = document->CreateElement("div");
				if (lab) {
					lab->SetClassNames("skill-detail-stat-label");
					lab->SetProperty("display", "block");
					lab->SetProperty("width", "110dp");
					lab->SetInnerRML(EscapeXml(st.label.c_str()));
					row->AppendChild(std::move(lab));
				}
				Rml::ElementPtr val = document->CreateElement("div");
				if (val) {
					val->SetClassNames(st.highlightNext ? "skill-detail-stat-value skill-detail-stat-next"
														: "skill-detail-stat-value");
					val->SetProperty("display", "block");
					val->SetProperty("width", "190dp");
					val->SetProperty("text-align", "right");
					val->SetInnerRML(EscapeXml(st.value.c_str()));
					row->AppendChild(std::move(val));
				}
				stats->AppendChild(std::move(row));
			}
		}

		if (Rml::Element* up = Get("btnDetailUpgrade")) {
			up->SetClass("skill-detail-upgrade-visible", detail.canUpgrade);
			if (detail.canUpgrade)
				up->RemoveAttribute("disabled");
			else
				up->SetAttribute("disabled", "true");
		}

		if (context)
			context->Update();
	}

	static std::string Fingerprint(const CRmlUiSkillForm::SkillView& view) {
		std::string fp = view.activeTab;
		fp += '|';
		fp += view.fightPts;
		fp += '|';
		fp += view.lifePts;
		char buf[64];
		for (const auto& row : view.rows) {
			sprintf_s(buf, ";%d:%d:%d:%s:%s", row.skillId, row.canUpgrade ? 1 : 0, row.selected ? 1 : 0,
					  row.name.c_str(), row.meta.c_str());
			fp += buf;
			fp += row.iconPath;
		}
		return fp;
	}

	void RebuildList(const CRmlUiSkillForm::SkillView& view) {
		Rml::Element* list = Get("skill-list");
		if (!list || !document)
			return;

		const float savedScroll = list->GetScrollTop();
		ClearChildren(list);

		for (const auto& row : view.rows) {
			Rml::ElementPtr rowPtr = document->CreateElement("div");
			if (!rowPtr)
				break;
			rowPtr->SetClassNames(row.selected ? "skill-row skill-row-selected" : "skill-row");
			char idBuf[48];
			sprintf_s(idBuf, "skill-row-%d", row.skillId);
			rowPtr->SetId(idBuf);
			rowPtr->SetAttribute("data-skill", std::to_string(row.skillId));

			Rml::ElementPtr well = document->CreateElement("div");
			if (well) {
				well->SetClassNames("skill-row-icon-well");
				if (!row.iconPath.empty()) {
					Rml::ElementPtr img = document->CreateElement("img");
					if (img) {
						img->SetClassNames("skill-row-icon");
						img->SetAttribute("src", row.iconPath.c_str());
						well->AppendChild(std::move(img));
					}
				}
				rowPtr->AppendChild(std::move(well));
			}

			Rml::ElementPtr text = document->CreateElement("div");
			if (text) {
				text->SetClassNames("skill-row-text");
				Rml::ElementPtr nameEl = document->CreateElement("div");
				if (nameEl) {
					nameEl->SetClassNames("skill-row-name");
					nameEl->SetInnerRML(EscapeXml(row.name.c_str()));
					text->AppendChild(std::move(nameEl));
				}
				Rml::ElementPtr metaEl = document->CreateElement("div");
				if (metaEl) {
					metaEl->SetClassNames("skill-row-meta");
					metaEl->SetInnerRML(EscapeXml(row.meta.c_str()));
					text->AppendChild(std::move(metaEl));
				}
				rowPtr->AppendChild(std::move(text));
			}

			Rml::ElementPtr up = document->CreateElement("button");
			if (up) {
				up->SetClassNames(row.canUpgrade ? "skill-row-upgrade skill-row-upgrade-visible" : "skill-row-upgrade");
				sprintf_s(idBuf, "skill-up-%d", row.skillId);
				up->SetId(idBuf);
				up->SetAttribute("type", "button");
				up->SetAttribute("data-upgrade", std::to_string(row.skillId));
				up->SetAttribute("title", "Upgrade");
				Rml::ElementPtr ico = document->CreateElement("img");
				if (ico) {
					ico->SetClassNames("skill-row-upgrade-icon");
					ico->SetAttribute("src", "ui/rml/frames/notice/ico_plus.tga");
					up->AppendChild(std::move(ico));
				}
				// Click is handled by the row listener (do not dual-register or upgrade fires twice).
				rowPtr->AppendChild(std::move(up));
			}

			rowPtr->AddEventListener(Rml::EventId::Click, this);
			rowPtr->AddEventListener(Rml::EventId::Dblclick, this);
			list->AppendChild(std::move(rowPtr));
		}

		if (context)
			context->Update();
		SetListScrollTop(savedScroll);
	}

	void ProcessEvent(Rml::Event& event) override {
		const Rml::EventId idEv = event.GetId();
		Rml::Element* target = event.GetTargetElement();
		if (!target)
			return;

		if (idEv == Rml::EventId::Mousescroll) {
			const float wheelY = event.GetParameter<float>("wheel_delta_y", 0.f);
			if (wheelY != 0.f)
				ScrollListBy(wheelY * 52.f);
			return;
		}

		if (idEv == Rml::EventId::Mousemove || idEv == Rml::EventId::Mouseup) {
			if (!thumbDragging)
				return;
			if (idEv == Rml::EventId::Mouseup) {
				thumbDragging = false;
				return;
			}
			Rml::Element* list = Get("skill-list");
			Rml::Element* bar = Get("skill-scrollbar");
			Rml::Element* thumb = Get("skill-scroll-thumb");
			if (!list || !bar || !thumb)
				return;
			const float clientH = list->GetClientHeight();
			const float scrollH = list->GetScrollHeight();
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
			SetListScrollTop(thumbDragStartScroll + (delta / travel) * maxScroll);
			return;
		}

		if (idEv != Rml::EventId::Click && idEv != Rml::EventId::Dblclick)
			return;

		Rml::Element* el = target;
		while (el && el != document) {
			const Rml::String& id = el->GetId();

			if (idEv == Rml::EventId::Click) {
				if (id == "btnClose") {
					RmlSkill_OnClose();
					return;
				}
				if (id == "btnDetailClose") {
					selectedSkillId = -1;
					RmlSkill_OnSelect(-1);
					return;
				}
				if (id == "btnDetailUpgrade") {
					if (selectedSkillId > 0)
						RmlSkill_OnUpgrade(selectedSkillId);
					return;
				}
				if (id == "skill-scroll-thumb") {
					thumbDragging = true;
					thumbDragGrabY = event.GetParameter<float>("mouse_y", 0.f);
					if (Rml::Element* list = Get("skill-list"))
						thumbDragStartScroll = list->GetScrollTop();
					return;
				}
				if (id == "skill-scrollbar") {
					Rml::Element* list = Get("skill-list");
					Rml::Element* thumb = Get("skill-scroll-thumb");
					if (list && thumb) {
						const float mouseY = event.GetParameter<float>("mouse_y", 0.f);
						const Rml::Vector2f barOff = el->GetAbsoluteOffset(Rml::BoxArea::Border);
						const float localY = mouseY - barOff.y;
						const float clientH = list->GetClientHeight();
						const float scrollH = list->GetScrollHeight();
						const float maxScroll = scrollH - clientH;
						if (maxScroll > 1.f) {
							const float trackH = el->GetClientHeight();
							float thumbH = thumb->GetClientHeight();
							if (thumbH < 1.f)
								thumbH = 28.f;
							const float travel = (std::max)(1.f, trackH - thumbH);
							const float ratio = (localY - thumbH * 0.5f) / travel;
							SetListScrollTop(ratio * maxScroll);
						}
					}
					return;
				}
				if (el->HasAttribute("data-filter")) {
					const Rml::String filter = el->GetAttribute("data-filter", Rml::String("fight"));
					SetActiveTab(id.c_str());
					activeTab = filter.c_str();
					selectedSkillId = -1;
					RmlSkill_OnTab(filter.c_str());
					return;
				}
				if (el->HasAttribute("data-upgrade")) {
					const int skillId = atoi(el->GetAttribute("data-upgrade", Rml::String("0")).c_str());
					if (skillId > 0) {
						event.StopPropagation();
						RmlSkill_OnUpgrade(skillId);
					}
					return;
				}
				if (el->HasAttribute("data-skill")) {
					const int skillId = atoi(el->GetAttribute("data-skill", Rml::String("0")).c_str());
					if (skillId > 0) {
						selectedSkillId = skillId;
						RmlSkill_OnSelect(skillId);
					}
					return;
				}
			}

			if (idEv == Rml::EventId::Dblclick && el->HasAttribute("data-skill")) {
				const int skillId = atoi(el->GetAttribute("data-skill", Rml::String("0")).c_str());
				if (skillId > 0)
					RmlSkill_OnUse(skillId);
				return;
			}

			el = el->GetParentNode();
		}
	}
};

CRmlUiSkillForm::CRmlUiSkillForm() {
	m_impl = new Impl();
}

CRmlUiSkillForm::~CRmlUiSkillForm() {
	delete m_impl;
	m_impl = nullptr;
}

CRmlUiSkillForm& CRmlUiSkillForm::Instance() {
	static CRmlUiSkillForm instance;
	return instance;
}

bool CRmlUiSkillForm::Load(Rml::Context* context) {
	if (!context || !m_impl)
		return false;
	m_impl->context = context;
	m_impl->loadOk = false;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->document = context->LoadDocument("skill.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load skill.rml\n");
		return false;
	}

	static const char* kClickIds[] = {
		"btnClose", "btnDetailClose", "btnDetailUpgrade",
		"tabFight", "tabLife", "tabSail",
		"skill-scrollbar", "skill-scroll-thumb"};
	for (const char* id : kClickIds) {
		if (Rml::Element* el = m_impl->Get(id))
			el->AddEventListener(Rml::EventId::Click, m_impl);
	}
	if (Rml::Element* list = m_impl->Get("skill-list"))
		list->AddEventListener(Rml::EventId::Mousescroll, m_impl);
	if (Rml::Element* wrap = m_impl->document->GetElementById("skill-root"))
		wrap->AddEventListener(Rml::EventId::Mousescroll, m_impl);
	if (Rml::Element* doc = m_impl->document) {
		doc->AddEventListener(Rml::EventId::Mousemove, m_impl);
		doc->AddEventListener(Rml::EventId::Mouseup, m_impl);
	}

	m_impl->document->Hide();
	m_impl->loadOk = true;
	OutputDebugStringA("RmlUi: skill.rml loaded\n");
	return true;
}

bool CRmlUiSkillForm::LoadOk() const {
	return m_impl && m_impl->loadOk && m_impl->document;
}

void CRmlUiSkillForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
	m_impl->loadOk = false;
	m_impl->hasPlacedRoot = false;
	m_impl->listFingerprint.clear();
	m_impl->detailFingerprint.clear();
	m_impl->fightPtsCache.clear();
	m_impl->lifePtsCache.clear();
	m_impl->sdNameCache.clear();
	m_impl->sdLevelCache.clear();
	m_impl->sdDescCache.clear();
	m_impl->sdEffectCache.clear();
	m_impl->sdCostCache.clear();
	m_impl->sdNextTitleCache.clear();
	m_impl->sdNextBodyCache.clear();
	m_impl->sdNextReqCache.clear();
	m_impl->sdIconCache.clear();
	m_impl->activeTab = "fight";
	m_impl->selectedSkillId = -1;
	m_impl->detailVisible = false;
	m_impl->thumbDragging = false;
}

void CRmlUiSkillForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
	if (!m_impl->hasPlacedRoot) {
		if (Rml::Element* root = m_impl->Get("skill-root")) {
			root->SetProperty(Rml::PropertyId::Left, Rml::Property(720.f, Rml::Unit::PX));
			root->SetProperty(Rml::PropertyId::Top, Rml::Property(80.f, Rml::Unit::PX));
		}
		m_impl->hasPlacedRoot = true;
	}
}

void CRmlUiSkillForm::Hide() {
	if (!m_impl)
		return;
	if (m_impl->document)
		m_impl->document->Hide();
	m_impl->listFingerprint.clear();
	m_impl->detailFingerprint.clear();
	m_impl->selectedSkillId = -1;
	m_impl->detailVisible = false;
	m_impl->thumbDragging = false;
	if (Rml::Element* panel = m_impl->Get("skill-detail"))
		panel->SetClass("skill-detail-hidden", true);
}

bool CRmlUiSkillForm::IsVisible() const {
	return m_impl && m_impl->document && m_impl->document->IsVisible();
}

const char* CRmlUiSkillForm::GetActiveTab() const {
	return m_impl ? m_impl->activeTab.c_str() : "fight";
}

int CRmlUiSkillForm::GetSelectedSkillId() const {
	return m_impl ? m_impl->selectedSkillId : -1;
}

int CRmlUiSkillForm::GetScrollOffset() const {
	if (!m_impl)
		return 0;
	if (Rml::Element* list = m_impl->Get("skill-list"))
		return (int)list->GetScrollTop();
	return 0;
}

bool CRmlUiSkillForm::IsDetailVisible() const {
	return m_impl && m_impl->detailVisible && m_impl->selectedSkillId > 0;
}

void CRmlUiSkillForm::ClearSelection() {
	if (!m_impl)
		return;
	m_impl->selectedSkillId = -1;
	RmlSkill_OnSelect(-1);
}

void CRmlUiSkillForm::ApplyView(const SkillView& view) {
	if (!m_impl || !m_impl->document)
		return;

	m_impl->activeTab = view.activeTab.empty() ? "fight" : view.activeTab;
	if (m_impl->activeTab == "life")
		m_impl->SetActiveTab("tabLife");
	else if (m_impl->activeTab == "sail")
		m_impl->SetActiveTab("tabSail");
	else
		m_impl->SetActiveTab("tabFight");

	SetTextCached(m_impl->Get("labSkillPts"), m_impl->fightPtsCache, view.fightPts);
	SetTextCached(m_impl->Get("labLifePts"), m_impl->lifePtsCache, view.lifePts);

	const std::string fp = Impl::Fingerprint(view);
	if (fp != m_impl->listFingerprint) {
		m_impl->listFingerprint = fp;
		m_impl->RebuildList(view);
	}

	m_impl->ApplyDetail(view.detail);
}
