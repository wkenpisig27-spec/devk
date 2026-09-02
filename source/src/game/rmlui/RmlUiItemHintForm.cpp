#include "StdAfx.h"
#include "rmlui/RmlUi_Win32_Undef.h"

#include "rmlui/RmlUiItemHintForm.h"

#include "UICommand.h"
#include "UIItemCommand.h"
#include "UIEquipForm.h"
#include "UIGlobalVar.h"
#include "UIFont.h"
#include "ItemRecord.h"
#include "StoneSet.h"
#include "StringLib.h"
#include "ItemAttrType.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Property.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <windows.h>

using namespace GUI;

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

void SetText(Rml::Element* el, const std::string& text) {
	if (!el)
		return;
	el->SetInnerRML(EscapeXml(text.c_str()));
}

std::string TrimCopy(const std::string& in) {
	size_t a = 0;
	size_t b = in.size();
	while (a < b && (unsigned char)in[a] <= ' ')
		++a;
	while (b > a && (unsigned char)in[b - 1] <= ' ')
		--b;
	return in.substr(a, b - a);
}

const char* ItemTypeLabel(short sType) {
	using E = EItemType;
	switch (sType) {
	case E::enumItemTypeSword: return "Weapon · Sword";
	case E::enumItemTypeGlave: return "Weapon · Greatsword";
	case E::enumItemTypeBow: return "Weapon · Bow";
	case E::enumItemTypeHarquebus: return "Weapon · Gun";
	case E::enumItemTypeFalchion: return "Weapon · Blade";
	case E::enumItemTypeMitten: return "Weapon · Fist";
	case E::enumItemTypeStylet: return "Weapon · Dagger";
	case E::enumItemTypeMoneybag: return "Weapon · Pouch";
	case E::enumItemTypeCosh: return "Weapon · Club";
	case E::enumItemTypeSinker: return "Weapon · Hammer";
	case E::enumItemTypeShield: return "Armor · Shield";
	case E::enumItemTypeArrow: return "Ammo · Arrow";
	case E::enumItemTypeAmmo: return "Ammo";
	case E::enumItemTypeAxe: return "Weapon · Axe";
	case E::enumItemTypeHair: return "Armor · Helm";
	case E::enumItemTypeFace: return "Apparel · Face";
	case E::enumItemTypeClothing: return "Armor · Body";
	case E::enumItemTypeGlove: return "Armor · Gloves";
	case E::enumItemTypeBoot: return "Armor · Boots";
	case E::enumItemTypeNecklace: return "Accessory · Necklace";
	case E::enumItemTypeRing: return "Accessory · Ring";
	case E::enumItemTypeTattoo: return "Armor · Tattoo";
	case E::enumItemTypeHairdo: return "Apparel · Hair";
	case E::enumItemTypeConch: return "Fairy · Conch";
	case E::enumItemTypeMedicine: return "Consumable";
	case E::enumItemTypeOvum: return "Consumable";
	case E::enumItemTypeMission: return "Quest Item";
	case E::enumItemTypeBoat: return "Captain License";
	case E::enumItemTypeWing: return "Apparel · Wings";
	case E::enumItemTypeGem: return "Enchant Stone";
	case E::enumItemTypeRefineGem: return "Refine Stone";
	case E::enumItemTypePet: return "Fairy";
	case E::enumItemTypeStatEgg: return "Stat Egg";
	case E::enumItemTypeBracelet: return "Accessory · Bracelet";
	case E::enumItemTypeBelt: return "Accessory · Belt";
	case E::enumItemTypeHandguard: return "Accessory · Bracer";
	case E::enumItemCloak: return "Apparel · Cloak";
	case E::enumItemMount: return "Mount";
	default:
		if (sType >= 1 && sType <= 10)
			return "Weapon";
		return "Item";
	}
}

std::string CssFromHintColor(DWORD color, bool onHeader) {
	const unsigned r = (color >> 16) & 0xff;
	const unsigned g = (color >> 8) & 0xff;
	const unsigned b = color & 0xff;
	const int lum = (int)(0.299f * r + 0.587f * g + 0.114f * b);
	if (onHeader) {
		if (lum > 230)
			return "#ffffff";
	} else {
		if (lum > 200)
			return "#2a4a80";
		if (r > 180 && g < 60 && b < 60)
			return "#c44040";
		if (g > 160 && r < 90 && b < 90)
			return "#1a8a38";
		if (r < 180 && g > 180 && b > 220)
			return "#3d7ec8";
	}
	char buf[16];
	sprintf_s(buf, "#%02x%02x%02x", r, g, b);
	return buf;
}

const char* StatClassForColor(DWORD color) {
	const unsigned r = (color >> 16) & 0xff;
	const unsigned g = (color >> 8) & 0xff;
	const unsigned b = color & 0xff;
	if (r > 180 && g < 60 && b < 60)
		return "ih-stat ih-stat-bad";
	if (g > 160 && r < 90 && b < 90)
		return "ih-stat ih-stat-ok";
	if (r < 200 && g > 160 && b > 200)
		return "ih-stat ih-stat-adv";
	return "ih-stat";
}

bool LineStartsWith(const std::string& line, const char* prefix) {
	const size_t n = strlen(prefix);
	return line.size() >= n && strncmp(line.c_str(), prefix, n) == 0;
}

bool IsEquippedItem(CItemCommand* cmd) {
	if (!cmd)
		return false;
	for (unsigned i = 0; i < (unsigned)enumEQUIP_NUM; ++i) {
		if (g_stUIEquip.GetEquipItem(i) == cmd)
			return true;
	}
	return false;
}

} // namespace

struct CRmlUiItemHintForm::Impl {
	Rml::Context* context = nullptr;
	Rml::ElementDocument* document = nullptr;
	bool loadOk = false;
	std::string viewFingerprint;
	std::string iconCache;
	int mouseX = 0;
	int mouseY = 0;
	bool open = false;
	bool shownThisFrame = false;

	Rml::Element* Get(const char* id) const {
		return document ? document->GetElementById(id) : nullptr;
	}

	void ClearChildren(Rml::Element* parent) {
		if (!parent)
			return;
		while (Rml::Element* child = parent->GetFirstChild())
			parent->RemoveChild(child);
	}

	void SetHidden(Rml::Element* el, bool hidden) {
		if (!el)
			return;
		el->SetClass("ih-badge-hidden", hidden);
		el->SetClass("ih-section-hidden", hidden);
	}

	void Place() {
		Rml::Element* root = Get("ih-root");
		if (!root || !context)
			return;
		const Rml::Vector2i dim = context->GetDimensions();
		Rml::Vector2f size = root->GetBox().GetSize(Rml::BoxArea::Border);
		if (size.x < 8.f)
			size.x = 280.f;
		if (size.y < 8.f)
			size.y = 180.f;

		float x = (float)mouseX + 18.f;
		float y = (float)mouseY - 12.f;
		if (x + size.x > (float)dim.x - 8.f)
			x = (float)mouseX - size.x - 12.f;
		if (x < 8.f)
			x = 8.f;
		if (y + size.y > (float)dim.y - 8.f)
			y = (float)dim.y - size.y - 8.f;
		if (y < 8.f)
			y = 8.f;

		root->SetProperty(Rml::PropertyId::Left, Rml::Property(x, Rml::Unit::PX));
		root->SetProperty(Rml::PropertyId::Top, Rml::Property(y, Rml::Unit::PX));
	}

	static std::string Fingerprint(CItemCommand* cmd) {
		if (!cmd)
			return {};
		std::string fp;
		char buf[64];
		CItemRecord* info = cmd->GetItemInfo();
		sprintf_s(buf, "%p:%d:", (void*)cmd, info ? info->lID : 0);
		fp += buf;
		SItemGrid& data = cmd->GetData();
		sprintf_s(buf, "%d:%d:%d:%u:%d:", data.sNum, data.chForgeLv, (int)data.dwDBID, (unsigned)data.lDBParam[0],
				  data.sEndure[0]);
		fp += buf;
		fp += cmd->GetName() ? cmd->GetName() : "";
		CTextHint& hints = CCommandObj::GetHints();
		sprintf_s(buf, ":%d", hints.GetCount());
		fp += buf;
		for (int i = 0; i < hints.GetCount(); ++i) {
			CTextHint::stHint* h = hints.GetHint(i);
			if (!h)
				continue;
			fp += '|';
			fp += h->hint;
		}
		return fp;
	}

	void Rebuild(CItemCommand* cmd) {
		if (!cmd || !document)
			return;
		CItemRecord* info = cmd->GetItemInfo();
		if (!info)
			return;

		CTextHint& hints = CCommandObj::GetHints();
		std::string name = cmd->GetName() ? cmd->GetName() : info->szName;
		DWORD nameColor = 0xffffffff;
		if (hints.GetCount() > 0 && hints.GetHint(0) && !hints.GetHint(0)->hint.empty()) {
			name = TrimCopy(hints.GetHint(0)->hint);
			nameColor = hints.GetHint(0)->color;
		}

		std::string iconPath;
		if (info->szICON[0])
			iconPath = info->GetIconFile();

		if (Rml::Element* icon = Get("ih-icon")) {
			if (iconPath != iconCache) {
				iconCache = iconPath;
				if (!iconPath.empty())
					icon->SetAttribute("src", iconPath.c_str());
			}
		}

		if (Rml::Element* nameEl = Get("ih-name")) {
			SetText(nameEl, name);
			nameEl->SetProperty("color", CssFromHintColor(nameColor, true));
		}
		SetText(Get("ih-type"), ItemTypeLabel(info->sType));

		const bool equipped = IsEquippedItem(cmd);
		if (Rml::Element* eq = Get("ih-equipped")) {
			eq->SetClass("ih-badge-hidden", !equipped);
		}

		SItemGrid& data = cmd->GetData();
		if (Rml::Element* refine = Get("ih-refine")) {
			if (data.chForgeLv > 0) {
				char lv[32];
				sprintf_s(lv, "Refine Lv.%d", (int)data.chForgeLv);
				SetText(refine, lv);
				refine->SetClass("ih-badge-hidden", false);
			} else {
				refine->SetClass("ih-badge-hidden", true);
			}
		}

		const bool locked = cmd->IsLocked();
		if (Rml::Element* lock = Get("ih-lock"))
			lock->SetClass("ih-badge-hidden", !locked);

		SItemForge& forge = cmd->GetForgeInfo();
		Rml::Element* dots = Get("ih-gem-dots");
		if (dots) {
			ClearChildren(dots);
			const int holes = forge.IsForge ? forge.nHoleNum : 0;
			for (int i = 0; i < holes && i < 3; ++i) {
				Rml::ElementPtr dot = document->CreateElement("div");
				if (!dot)
					break;
				const bool filled = i < forge.nStoneNum;
				dot->SetClassNames(filled ? "ih-gem-dot" : "ih-gem-dot ih-gem-dot-empty");
				dots->AppendChild(std::move(dot));
			}
		}

		std::vector<std::string> skipExact;
		skipExact.push_back(name);
		if (forge.IsForge) {
			for (int i = 0; i < forge.nStoneNum && i < 3; ++i) {
				if (forge.szStoneHint[i][0])
					skipExact.push_back(TrimCopy(forge.szStoneHint[i]));
			}
		}

		auto shouldSkipLine = [&](const std::string& line) {
			if (line.empty())
				return true;
			if (line == "Trade" || line == "Throw" || line == "Delete")
				return true;
			if (LineStartsWith(line, "Stack:") || LineStartsWith(line, "Trade Value:"))
				return true;
			if (line.find("Socket") != std::string::npos)
				return true;
			if (line == "Locked")
				return true;
			for (const std::string& s : skipExact) {
				if (line == s)
					return true;
			}
			if (forge.IsForge) {
				for (int i = 0; i < forge.nStoneNum && i < 3; ++i) {
					if (forge.pStoneInfo[i] && forge.pStoneInfo[i]->szDataName[0] &&
						line.find(forge.pStoneInfo[i]->szDataName) != std::string::npos)
						return true;
				}
			}
			return false;
		};

		std::string valueText;
		std::string metaText;
		Rml::Element* stats = Get("ih-stats");
		if (stats)
			ClearChildren(stats);

		for (int i = 0; i < hints.GetCount(); ++i) {
			CTextHint::stHint* h = hints.GetHint(i);
			if (!h)
				continue;
			const std::string line = TrimCopy(h->hint);
			if (LineStartsWith(line, "Trade Value:")) {
				valueText = line;
				continue;
			}
			if (shouldSkipLine(line))
				continue;
			if (LineStartsWith(line, "Lv.:") || LineStartsWith(line, "Level Requirement:") ||
				LineStartsWith(line, "Effectiveness")) {
				if (!metaText.empty())
					metaText += "\n";
				metaText += line;
				continue;
			}
			if (!stats)
				continue;
			Rml::ElementPtr row = document->CreateElement("div");
			if (!row)
				break;
			row->SetClassNames(StatClassForColor(h->color));
			Rml::ElementPtr star = document->CreateElement("div");
			if (star) {
				star->SetClassNames("ih-stat-star");
				row->AppendChild(std::move(star));
			}
			Rml::ElementPtr txt = document->CreateElement("div");
			if (txt) {
				txt->SetClassNames("ih-stat-text");
				txt->SetInnerRML(EscapeXml(line.c_str()));
				row->AppendChild(std::move(txt));
			}
			stats->AppendChild(std::move(row));
		}
		SetText(Get("ih-meta"), metaText);

		Rml::Element* gemsWrap = Get("ih-gems-wrap");
		Rml::Element* gems = Get("ih-gems");
		const bool showGems = forge.IsForge && forge.nHoleNum > 0;
		if (gemsWrap)
			gemsWrap->SetClass("ih-section-hidden", !showGems);
		if (gems) {
			ClearChildren(gems);
			if (showGems) {
				for (int i = 0; i < forge.nHoleNum && i < 3; ++i) {
					Rml::ElementPtr row = document->CreateElement("div");
					if (!row)
						break;
					row->SetClassNames("ih-gem");

					Rml::ElementPtr well = document->CreateElement("div");
					well->SetClassNames("ih-gem-icon-well");
					if (i < forge.nStoneNum && forge.pStoneInfo[i]) {
						CItemRecord* stoneItem = GetItemRecordInfo(forge.pStoneInfo[i]->nItemID);
						if (stoneItem && stoneItem->szICON[0]) {
							const std::string gemIcon = stoneItem->GetIconFile();
							Rml::ElementPtr img = document->CreateElement("img");
							img->SetClassNames("ih-gem-icon");
							img->SetAttribute("src", gemIcon.c_str());
							well->AppendChild(std::move(img));
						}
					}
					row->AppendChild(std::move(well));

					Rml::ElementPtr text = document->CreateElement("div");
					text->SetClassNames("ih-gem-text");
					Rml::ElementPtr nm = document->CreateElement("div");
					Rml::ElementPtr bonus = document->CreateElement("div");
					if (i < forge.nStoneNum && forge.pStoneInfo[i]) {
						char title[160];
						sprintf_s(title, "%s  Lv.%d", forge.pStoneInfo[i]->szDataName, forge.nStoneLevel[i]);
						nm->SetClassNames("ih-gem-name");
						nm->SetInnerRML(EscapeXml(title));
						bonus->SetClassNames("ih-gem-bonus");
						bonus->SetInnerRML(EscapeXml(forge.szStoneHint[i]));
					} else {
						nm->SetClassNames("ih-gem-name ih-gem-empty");
						nm->SetInnerRML("Empty socket");
						bonus->SetClassNames("ih-gem-bonus ih-gem-empty");
						bonus->SetInnerRML("No stone socketed");
					}
					text->AppendChild(std::move(nm));
					text->AppendChild(std::move(bonus));
					row->AppendChild(std::move(text));
					gems->AppendChild(std::move(row));
				}
			}
		}

		const bool isGemItem = info->sType == enumItemTypeGem;
		Rml::Element* specialWrap = Get("ih-special-wrap");
		Rml::Element* special = Get("ih-special");
		std::string gemEffect;
		if (isGemItem)
			gemEffect = TrimCopy(cmd->GetStoneHint());
		if (specialWrap)
			specialWrap->SetClass("ih-section-hidden", gemEffect.empty() || gemEffect == "error");
		if (special && !gemEffect.empty() && gemEffect != "error")
			SetText(special, gemEffect);

		Rml::Element* flags = Get("ih-flags");
		Rml::Element* flagsRule = Get("ih-flags-rule");
		if (flags) {
			ClearChildren(flags);
			if (data.sNum != 0) {
				if (flagsRule)
					flagsRule->SetClass("ih-section-hidden", false);
				const bool tradable = data.GetInstAttr(ITEMATTR_TRADABLE) != 0 && info->chIsTrade != 0;
				const bool throwable = data.GetInstAttr(ITEMATTR_TRADABLE) != 0 && info->chIsThrow != 0;
				const bool deletable = info->chIsDel != 0;
				struct Flag {
					const char* label;
					bool on;
				} list[] = {{"Trade", tradable}, {"Throw", throwable}, {"Delete", deletable}};
				for (const Flag& f : list) {
					Rml::ElementPtr el = document->CreateElement("div");
					if (!el)
						break;
					el->SetClassNames(f.on ? "ih-flag ih-flag-on" : "ih-flag ih-flag-off");
					el->SetInnerRML(EscapeXml(f.label));
					flags->AppendChild(std::move(el));
				}

				char stack[48];
				sprintf_s(stack, "%d/%d", data.sNum, info->nPileMax > 0 ? info->nPileMax : 1);
				Rml::ElementPtr st = document->CreateElement("div");
				if (st) {
					st->SetClassNames("ih-flag ih-flag-stack");
					st->SetInnerRML(EscapeXml(stack));
					flags->AppendChild(std::move(st));
				}
			} else if (flagsRule) {
				flagsRule->SetClass("ih-section-hidden", true);
			}
		}

		if (valueText.empty() && cmd->GetPrice() != 0) {
			char buf[96];
			sprintf_s(buf, "Trade Value: %s", StringSplitNum(cmd->GetPrice()));
			valueText = buf;
		}
		SetText(Get("ih-value"), valueText);
	}
};

CRmlUiItemHintForm& CRmlUiItemHintForm::Instance() {
	static CRmlUiItemHintForm inst;
	return inst;
}

CRmlUiItemHintForm::CRmlUiItemHintForm()
	: m_impl(new Impl) {}

CRmlUiItemHintForm::~CRmlUiItemHintForm() {
	delete m_impl;
}

bool CRmlUiItemHintForm::Load(Rml::Context* context) {
	if (!m_impl || !context)
		return false;
	m_impl->context = context;
	m_impl->loadOk = false;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->document = context->LoadDocument("itemhint.rml");
	if (!m_impl->document) {
		OutputDebugStringA("RmlUi: failed to load itemhint.rml\n");
		return false;
	}
	m_impl->document->Hide();
	m_impl->loadOk = true;
	OutputDebugStringA("RmlUi: itemhint.rml loaded\n");
	return true;
}

bool CRmlUiItemHintForm::LoadOk() const {
	return m_impl && m_impl->loadOk && m_impl->document;
}

void CRmlUiItemHintForm::Unload() {
	if (!m_impl)
		return;
	if (m_impl->document) {
		m_impl->document->Close();
		m_impl->document = nullptr;
	}
	m_impl->context = nullptr;
	m_impl->loadOk = false;
	m_impl->open = false;
	m_impl->shownThisFrame = false;
	m_impl->viewFingerprint.clear();
	m_impl->iconCache.clear();
}

void CRmlUiItemHintForm::Show() {
	if (!m_impl || !m_impl->document)
		return;
	m_impl->document->Show();
	if (Rml::Element* root = m_impl->Get("ih-root"))
		root->SetClass("ih-card-open", true);
	m_impl->open = true;
}

void CRmlUiItemHintForm::Hide() {
	if (!m_impl)
		return;
	if (Rml::Element* root = m_impl->Get("ih-root"))
		root->SetClass("ih-card-open", false);
	if (m_impl->document)
		m_impl->document->Hide();
	m_impl->open = false;
	m_impl->shownThisFrame = false;
	m_impl->viewFingerprint.clear();
}

bool CRmlUiItemHintForm::IsVisible() const {
	return m_impl && m_impl->open && m_impl->document && m_impl->document->IsVisible();
}

bool CRmlUiItemHintForm::TryShow(GUI::CCommandObj* cmd, int mouseX, int mouseY) {
	if (!LoadOk() || !cmd)
		return false;
	CItemCommand* item = dynamic_cast<CItemCommand*>(cmd);
	if (!item) {
		Hide();
		return false;
	}

	m_impl->mouseX = mouseX;
	m_impl->mouseY = mouseY;
	const std::string fp = Impl::Fingerprint(item);
	if (fp != m_impl->viewFingerprint || !m_impl->open) {
		m_impl->viewFingerprint = fp;
		m_impl->Rebuild(item);
	}
	m_impl->shownThisFrame = true;
	Show();
	m_impl->Place();
	return true;
}

void CRmlUiItemHintForm::PlaceNearCursor() {
	if (m_impl && m_impl->open)
		m_impl->Place();
}

bool CRmlUiItemHintForm::ConsumeShownThisFrame() {
	if (!m_impl)
		return false;
	const bool shown = m_impl->shownThisFrame;
	m_impl->shownThisFrame = false;
	return shown;
}

bool RmlItemHint_TryShow(GUI::CCommandObj* cmd, int x, int y) {
	return CRmlUiItemHintForm::Instance().TryShow(cmd, x, y);
}

void RmlItemHint_Hide() {
	CRmlUiItemHintForm::Instance().Hide();
}

bool RmlItemHint_IsShowing() {
	return CRmlUiItemHintForm::Instance().IsVisible();
}
