#include "StdAfx.h"
#include "uiheadsay.h"
#include "GameApp.h"
#include "Character.h"
#include "uiitem.h"
#include "UILabel.h"
#include "UIProgressBar.h"
#include "SkillStateRecord.h"
#include "SkillStateType.h"
#include "chastate.h"
#include "uiboatform.h"
#include "uiMiniMapForm.h"
#include "uistartForm.h"
#include "globalvar.h"
#include "uiequipform.h"

using namespace std;

// Helper for rendering name text - uses Render() if font has pre-baked outline, else ORender()
inline void RenderNameText(const char* text, int x, int y, DWORD textColor, DWORD outlineColor = COLOR_BLACK) {
	if (CGuiFont::s_NameFontHasPrebakedOutline) {
		CGuiFont::s_NameFont.Render(text, x, y, textColor);
	} else {
		CGuiFont::s_NameFont.ORender(text, x, y, textColor, outlineColor);
	}
}
using namespace GUI;

bool g_IsShowModel = true;
bool g_IsShowShop = true;
bool g_IsShowHp = false;
bool g_IsShowNames = false;
bool g_IsShowStates = false;
bool CHeadSay::_ShowEnemyNames = true;
bool CHeadSay::_ShowBars = false;
bool CHeadSay::_ShowPercentages = false;
bool CHeadSay::_ShowInfo = false;

//---------------------------------------------------------------------------
// calss CHeadSay
//---------------------------------------------------------------------------
int CHeadSay::_nMaxShowTime = 100;
CGuiPic* CHeadSay::_pImgLife = new CGuiPic(nullptr, 2);
CGuiPic* CHeadSay::_pImgMana = new CGuiPic(nullptr, 2);

DWORD CHeadSay::_dwBkgColor = 0x90000000;

unsigned int CHeadSay::_nFaceFrequency = 6;
CGuiPic* CHeadSay::_pImgFace = new CGuiPic[FACE_MAX];
CGuiPic* CHeadSay::_pImgEvil = new CGuiPic[EVIL_MAX];

CGuiPic* CHeadSay::_pImgTeamLeaderFlag = new CGuiPic;
CGuiPic* CHeadSay::_pImgGuildLeaderFlag = new CGuiPic;

CGuiPic* CHeadSay::_pImgShopHidden = new CGuiPic;

CGuiPic CHeadSay::_ImgShop[3];
CGuiPic CHeadSay::_ImgShop2[3];
int CHeadSay::_nShopFrameWidth = 0;
int CHeadSay::_nShopFontYOff = 0;

// State icon cache to avoid reloading textures every frame
std::map<std::string, CGuiPic*> CHeadSay::_stateIconCache;
static const int ITEM_BUFF_STATE_IDS[] = {
    17,19,20,41,42,44,47,48,49,102,103,104,105,106,107,108,109,120,127,128,
    129,130,133,134,147,165,166,167,194,195,199,-1
};

static const int DEBUFF_STATE_IDS[] = {
    1,3,4,7,12,13,14,22,26,39,45,46,65,67,68,81,86,87,89,91,
    95,96,98,114,115,116,117,118,125,126,131,138,143,159,160,
    162,163,176,177,178,179,183,188,189,-1
};

static const int BUFF_STATE_IDS[] = {
    2,5,6,24,29,33,37,40,43,50,51,70,83,88,90,92,
    132,168,169,170,171,172,173,174,-1
};

// Helper function to check if stateId is in an array
static bool IsStateInArray(int stateId, const int* arr) {
	for (int i = 0; arr[i] != -1; i++) {
		if (arr[i] == stateId) return true;
	}
	return false;
}

//===========================================================================

// ?????????? (j?) ?? ????? + (??????) ?? [????]
char CHeadSay::s_sNamePart[NAME_PART_NUM][64] = {0};

char CHeadSay::s_szName[1024] = {0};

DWORD CHeadSay::s_dwNamePartsColors[NAME_PART_NUM][2] = //	jh????j???,??h??????????
	{
		COLOR_WHITE, COLOR_BLACK,	  //	(
		0, COLOR_BLACK,				  //	j???
		COLOR_WHITE, COLOR_BLACK,	  //	)
		0, COLOR_BLACK,				  //	?????
		COLOR_BLACK, 0,				  //	(
		COLOR_BLACK, 0,				  //	??????
		COLOR_BLACK, 0,				  //	)
		COLOR_WHITE, COLOR_BLACK,	  //	[
		COLOR_SHIP_NAME, COLOR_BLACK, //	????
		COLOR_WHITE, COLOR_BLACK,	  //	]
};

char CHeadSay::s_szConsortiaNamePart[4][64] = {0};

char CHeadSay::s_szConsortiaName[256] = {0};

// Member Methods---------------------------------------------------------

bool CHeadSay::Init() {
	_pImgTeamLeaderFlag->LoadImage("texture/ui/flag.tga", 12, 12, 0, 0, 0, 1.0, 1.0);
	_pImgGuildLeaderFlag->LoadImage("texture/ui/guildflag.tga", 16, 16, 0, 0, 0, 0.8, 0.8);
	_pImgShopHidden->LoadImage("texture/ui/hidestall.tga", 25, 32, 0, 0, 0, 1.0, 1.0);

	_nShopFrameWidth = _ImgShop[0].GetWidth() - 1;
	_nShopFontYOff = (_ImgShop[0].GetHeight() - CGuiFont::s_Font.GetHeight(RES_STRING(CL_LANGUAGE_MATCH_489))) / 2;

	for (int i = 0; i < EVIL_MAX; i++) {
		char buffer[64] = {0};
		sprintf(buffer, "texture/icon/evil_lv%d.tga", i + 1);

		if (!_pImgEvil)
			break;

		_pImgEvil[i].LoadImage(buffer, 16, 16, 0, 0, 0, 0.0, 0.0);
	}
	return true;
}

bool CHeadSay::Clear() {
	if (_pImgLife) {
		delete _pImgLife;
		_pImgLife = nullptr;
	}

	if (_pImgMana) {
		delete _pImgMana;
		_pImgMana = nullptr;
	}

	if (_pImgFace) {
		delete[] _pImgFace;
		_pImgFace = nullptr;
	}

	if (_pImgTeamLeaderFlag) {
		delete _pImgTeamLeaderFlag;
		_pImgTeamLeaderFlag = nullptr;
	}

	if (_pImgGuildLeaderFlag) {
		delete _pImgGuildLeaderFlag;
		_pImgGuildLeaderFlag = nullptr;
	}

	if (_pImgShopHidden) {
		delete _pImgShopHidden;
		_pImgShopHidden = nullptr;
	}

	if (_pImgEvil) {
		delete[] _pImgEvil;
	}

	// Clear state icon cache
	for (auto& pair : _stateIconCache) {
		delete pair.second;
	}
	_stateIconCache.clear();

	return true;
}

CGuiPic* CHeadSay::GetCachedStateIcon(const char* path, float scale) {
	std::string key(path);
	
	auto it = _stateIconCache.find(key);
	if (it != _stateIconCache.end()) {
		return it->second;
	}
	
	// Load and cache the icon
	CGuiPic* icon = new CGuiPic(nullptr, 1);
	icon->LoadImage(path, 32, 32, 0, 0, 0, scale, scale);
	_stateIconCache[key] = icon;
	return icon;
}

CHeadSay::CHeadSay(CCharacter* p)
	: _pOwn(p), _nShowTime(0), _pObj(nullptr), _fLifeW(1.0f), _dwFaceTime(0), _pCurFace(nullptr), _IsShowLife(false), _IsShowMana(false), _IsShowName(false), _nChaNameOffX(0), _fScale(0.1f), _dwNameColor(COLOR_WHITE), _sEvilLevel(0), _IsShowEvil(false), isGuildMember(false), isTeamMember(false), RenderDebuff(false) {
}

void CHeadSay::AddItem(CItemEx* obj) {
	if (_pObj)
		delete _pObj;

	_pObj = obj;

	_nShowTimeWhenCreated = _nMaxShowTime * (g_Render.GetFPS() / 30.f);
	_nShowTime = _nShowTimeWhenCreated;

	_fScale = 0.1f;
}

void CHeadSay::SetName(const char* name) {
	_nChaNameOffX = 0 - CGuiFont::s_NameFont.GetWidth(name) / 2;
}

// State categories for row-based display
enum StateCategory {
	STATE_CAT_BUFF,      // Skill buffs (row 1)
	STATE_CAT_ITEM_BUFF, // Item/potion buffs (row 2)
	STATE_CAT_DEBUFF     // Debuffs (row 3)
};

// State info for sorting and rendering
struct StateRenderInfo {
	int stateId;
	unsigned long timeRemaining;
	CSkillStateRecord* pState;
	CChaStateMgr::stChaState stateData;
	StateCategory category;
};

// Categorize state based on configured arrays and restriction flags
StateCategory GetStateCategory(int stateId, CSkillStateRecord* pState) {
	// Check manually configured arrays first (highest priority)
	if (IsStateInArray(stateId, BUFF_STATE_IDS))      return STATE_CAT_BUFF;
	if (IsStateInArray(stateId, ITEM_BUFF_STATE_IDS)) return STATE_CAT_ITEM_BUFF;
	if (IsStateInArray(stateId, DEBUFF_STATE_IDS))    return STATE_CAT_DEBUFF;
	
	// Auto-detect using restriction flags
	if (pState && (!pState->bCanMove || !pState->bCanMSkill || !pState->bCanGSkill || 
	               !pState->bCanTrade || !pState->bCanItem || pState->IsDizzy)) {
		return STATE_CAT_DEBUFF;
	}
	
	// Default: treat as buff
	return STATE_CAT_BUFF;
}

// Sort by time remaining (expiring soonest first), unlimited states last
bool CompareStates(const StateRenderInfo& a, const StateRenderInfo& b) {
	bool aUnlimited = (a.timeRemaining / 60) >= 60;
	bool bUnlimited = (b.timeRemaining / 60) >= 60;
	
	if (aUnlimited != bUnlimited) return bUnlimited;
	if (aUnlimited) return a.stateId < b.stateId;
	return a.timeRemaining < b.timeRemaining;
}

void CHeadSay::RenderStateIcons(CCharacter* cha, int x, int y, float scale, float spacing, int rowSize, bool Rendertimer) {
	if (!cha->IsMainCha())
		Rendertimer = false;

	int nTotalState = CSkillStateRecordSet::I()->GetLastID() + 1;
	
	// Category vectors
	std::vector<StateRenderInfo> buffStates, itemBuffStates, debuffStates;
	
	// Collect and categorize active states
	for (int i = 0; i < nTotalState; i++) {
		if (!cha->GetStateMgr()->HasSkillState(i)) continue;
		
		CSkillStateRecord* pState = GetCSkillStateRecordInfo(i);
		if (!pState || stricmp(pState->szIcon[0], "0") == 0) continue;
		
		StateRenderInfo info = { i, 0, pState, cha->GetStateMgr()->GetStateData(i), STATE_CAT_BUFF };
		info.timeRemaining = info.stateData.lTimeRemaining;
		info.category = GetStateCategory(i, pState);
		
		switch (info.category) {
			case STATE_CAT_BUFF:      buffStates.push_back(info); break;
			case STATE_CAT_ITEM_BUFF: itemBuffStates.push_back(info); break;
			case STATE_CAT_DEBUFF:    debuffStates.push_back(info); break;
		}
	}
	
	// Sort each category
	std::sort(buffStates.begin(), buffStates.end(), CompareStates);
	std::sort(itemBuffStates.begin(), itemBuffStates.end(), CompareStates);
	std::sort(debuffStates.begin(), debuffStates.end(), CompareStates);
	
	// Rendering setup
	bool blinkOn = ((GetTickCount() / 250) % 2) == 0;
	int iconSize = (int)(32 * scale);
	int rowHeight = (int)(spacing + (Rendertimer ? 12 : 0));
	int currentRow = 0;
	int stateCount = 0;
	int mouseX = g_pGameApp->GetMouseX();
	int mouseY = g_pGameApp->GetMouseY();
	
	// Render a single state icon
	auto renderIcon = [&](const StateRenderInfo& info, int col, int row) {
		// Get icon name (check for level-specific icon)
		int stateLv = cha->GetStateMgr()->GetStateLv(info.stateId);
		const char* iconName = info.pState->szIcon[0];
		if (stateLv > 0 && stricmp(info.pState->szIcon[stateLv - 1], "0") != 0)
			iconName = info.pState->szIcon[stateLv - 1];

		char buf[64];
		sprintf(buf, "texture/icon/%s.png", iconName);
		CGuiPic* icon = GetCachedStateIcon(buf, scale);

		int xi = x + col * (int)spacing;
		int yi = y + row * rowHeight;
		int minutes = info.timeRemaining / 60;
		int seconds = info.timeRemaining % 60;
		bool isExpiring = (info.timeRemaining > 0 && info.timeRemaining <= 10);
		
		// Render icon (blink if expiring)
		icon->Render(xi, yi, (isExpiring && !blinkOn) ? (BYTE)100 : (BYTE)255);

		// Render timer text
		if (info.timeRemaining > 0 && Rendertimer) {
			char szTime[16];
			if (minutes >= 60)
				sprintf(szTime, "--");
			else if (minutes >= 1)
				sprintf(szTime, "%dm", minutes);
			else
				sprintf(szTime, "%ds", seconds);
			
			int textX = xi + iconSize / 2 - (minutes >= 10 ? 8 : 5);
			GetRender().FillFrame(xi, yi + iconSize, xi + iconSize + 1, yi + iconSize + CGuiFont::s_Font.GetHeight("0"));
			CGuiFont::s_Font.Render(0, szTime, textX, yi + iconSize, COLOR_WHITE, 1.0f);
		}

		stateCount++;

		// Mouse hover tooltip
		if (mouseX >= xi && mouseX <= xi + iconSize && mouseY >= yi && mouseY <= yi + iconSize)
			g_pGameApp->ShowStateHint(mouseX, mouseY, info.stateData);
	};
	
	// Render category helper
	auto renderCategory = [&](std::vector<StateRenderInfo>& states) {
		if (states.empty()) return;
		for (size_t i = 0; i < states.size(); i++)
			renderIcon(states[i], i % rowSize, currentRow + (int)(i / rowSize));
		currentRow += (int)((states.size() + rowSize - 1) / rowSize);
	};
	
	// Render all categories in order
	renderCategory(buffStates);
	renderCategory(itemBuffStates);
	renderCategory(debuffStates);

	// Update state form size
	if (stateCount > 0 && g_IsShowStates) {
		int w = min(250, (int)(iconSize * rowSize + spacing * (rowSize - 1)));
		int h = max(45, currentRow * rowHeight);
		g_stUIEquip.stateDrags->SetSize(w, h);
		g_stUIEquip.stateDrags->Refresh();
		g_stUIEquip.stateDrags->SetIsShow(true);
	} else {
		g_stUIEquip.stateDrags->SetIsShow(false);
	}
}

void CHeadSay::Render(D3DXVECTOR3& pos) {
	static int x = 0, y = 0;
	static int nSayTotalWidth = 32 * CGuiFont::s_Font.GetWidth("a"); // 32??????????
	g_Render.WorldToScreen(pos.x, pos.y, pos.z + _pOwn->GetDefaultChaInfo()->fHeight, &x, &y);

	/*
	if(_pOwn->getHumanID() == g_stUIStart.targetInfoID && g_stUIStart.frmTargetInfo->GetIsShow()){
		float scale = 0.45;
		float spacing = scale * 32 + 2;
		RenderStateIcons(_pOwn,200,40, scale,spacing,10);
	}
	*/

	if (_ShowBars && (_IsShowLife || (_pOwn->IsPlayer() && _ShowInfo)) && !_fLifeW <= 0) // ???
	{
		static int x1 = 0, y1 = 0;
		g_Render.WorldToScreen(pos.x, pos.y, pos.z, &x1, &y1);

		int nLifeWidth = _pImgLife->GetWidth();
		_pImgLife->SetScaleW(1, _fLifeW);
		int nOffset = (int)((x - g_Render.GetScrWidth() / 2) * 0.02f);

		DWORD hpcolour = 0xA0FF0000;
		int red, green;
		if (_fLifeW > 0.5f) {
			red = 510 - (_fLifeW * 2 * 255);
			green = 255;
			hpcolour = 0xB3000000 + (256 * green) + (65536 * red);
		} else if (_fLifeW <= 0.5f && _fLifeW > 0) {
			red = 255;
			green = _fLifeW * 2 * 255;
			hpcolour = 0xB3000000 + (256 * green) + (65536 * red); // B3
		}

		_pImgLife->RenderAll(x - nLifeWidth / 2 - nOffset, y1 + 20, hpcolour);

		char hpInfo[32];

		if (_ShowPercentages || (_pOwn->GetIsPK() && !isTeamMember)) {
			sprintf(hpInfo, "%.0f%%", _fLifeW * 100);
		} else {
			sprintf(hpInfo, "%d/%d", _fCurHp, _fMxHp);
		}

		if (_pOwn->GetStateMgr()->HasSkillState(83)) {
			if (isTeamMember || isGuildMember || (_ShowInfo && _pOwn->IsPlayer()) || _IsShowLife) {

				if (_pOwn->IsPlayer()) {
					int nManaWidth = _pImgMana->GetWidth();
					_pImgMana->SetScaleW(1, _fManaW);

					DWORD spcolour = 0xA0FF0000;
					if (_fManaW > 0) {
						int red = 255 - (_fManaW * 255);
						int blue = _fManaW * 255;
						spcolour = 0xB3000000 + blue + (65536 * red);
					}

					_pImgMana->RenderAll(x - nManaWidth / 2 - nOffset, y1 + 28, spcolour);

					char spInfo[32];

					if (_ShowPercentages || (_pOwn->GetIsPK() && !isTeamMember)) {
						sprintf(spInfo, "%.0f%%", _fManaW * 100);
					} else {
						sprintf(spInfo, "%d/%d", _fCurSp, _fMxSp);
					}
					CGuiFont::s_Font.BRender(spInfo, x - nOffset - (CGuiFont::s_Font.GetWidth(spInfo) / 2), y1 + 32, spcolour, COLOR_BLACK);
				}
			}
		}

		CGuiFont::s_Font.BRender(hpInfo, x - nOffset - (CGuiFont::s_Font.GetWidth(hpInfo) / 2), y1 + 8, hpcolour, COLOR_BLACK);

		if (g_IsShowStates) {
			float scale = 0.60;
			float spacing = scale * 32 + 2;
			int picX = 0;
			int picY = 0;
			int count = 9;
			if (_pOwn == CGameScene::GetMainCha()) {
				picX = g_stUIEquip.stateDrags->GetLeft();
				picY = g_stUIEquip.stateDrags->GetTop();
			}
			RenderStateIcons(_pOwn, picX, picY, scale, spacing, count, true);
		}
	}

	if (g_stUIMap.IsPKSilver()) {
		// ??????????????????????????????????????h??????
		return;
	}

	if (_pOwn->GetIsFly())
		y -= 30;

	if (_pOwn->IsTeamLeader()) {
		y -= 20;
		GetLeaderPic()->Render(x - 6, y);
	}

	if (g_IsShowShop && _pOwn->IsShop()) {
		if (_pOwn->GetStateMgr()->GetStateLv(99) == 3) {
			_RenderShop2(_pOwn->getShopName(), x, y);
		} else {
			_RenderShop(_pOwn->getShopName(), x, y);
		}

		static int nImageHeight = _ImgShop[0].GetHeight();
		y -= nImageHeight;
	}

	if (!g_IsShowShop && _pOwn->IsShop()) {
		_pImgShopHidden->Render(x - (_pImgShopHidden->GetWidth() / 2), y - 30);
	}

	if (_nShowTime <= 0) {
#ifdef _LOG_NAME_								// ???????
		const int LINE_HEIGHT_STEP = 14;		// h?????
		int iNameHeightStep = LINE_HEIGHT_STEP; // ?????????????

		// ?????????
		if (CCharacter::IsShowLogName) {
			int nNameLength = 0 - CGuiFont::s_NameFont.GetWidth(_pOwn->getLogName()) / 2;
			RenderNameText(_pOwn->getLogName(), x + nNameLength, y - iNameHeightStep, _dwNameColor, COLOR_BLACK);
			//_dwNameColor

		} else if (_IsShowName || g_IsShowNames || (_pOwn->GetIsPK() && _ShowEnemyNames) || (_ShowInfo && _pOwn->IsPlayer())) {

			// ??????????
			if (*(_pOwn->getGuildName())) {
				iNameHeightStep += LINE_HEIGHT_STEP; // ????????h??
				int iGuildNameHeightStep = LINE_HEIGHT_STEP;

				// ??????
				if (*(_pOwn->getGuildName())) {
					strncpy(s_szConsortiaNamePart[0], _pOwn->getGuildName(), NAME_LENGTH);
				} else {
					strncpy(s_szConsortiaNamePart[0], "", NAME_LENGTH);
				}
				// ??????????
				if (*(_pOwn->getGuildMotto())) {
					strncpy(s_szConsortiaNamePart[1], "(", strlen("("));
					strncpy(s_szConsortiaNamePart[2], _pOwn->getGuildMotto(), NAME_LENGTH);
					strncpy(s_szConsortiaNamePart[3], ")", strlen(")"));
				} else {
					strncpy(s_szConsortiaNamePart[1], "", NAME_LENGTH);
					strncpy(s_szConsortiaNamePart[2], "", NAME_LENGTH);
					strncpy(s_szConsortiaNamePart[3], "", NAME_LENGTH);
				}

				// ZeroMemory(s_szConsortiaName, sizeof(s_szConsortiaName));
				s_szConsortiaName[0] = '\0';
				for (int i(0); i < 4; i++) {
					strncat(s_szConsortiaName, s_szConsortiaNamePart[i], NAME_LENGTH);
				}
				int nNameLength = 0 - CGuiFont::s_NameFont.GetWidth(s_szConsortiaName) / 2;
				int iStartPosX = 0 - CGuiFont::s_NameFont.GetWidth(s_szConsortiaName) / 2;

				// renderÿh??????
				int perm = (_pOwn->getGuildPermission() & emGldPermLeader);
				if (perm == emGldPermLeader) {
					// GetGuildLeaderPic()->TintColour( 255, 0, 0 );
					GetGuildLeaderPic()->Render(x + iStartPosX - 13, y - iGuildNameHeightStep - 2);
				}

				for (int i(0); i < 4; i++) {
					RenderNameText(s_szConsortiaNamePart[i], x + iStartPosX, y - iGuildNameHeightStep, _pOwn->getGuildColor(), COLOR_BLACK);
					iStartPosX += CGuiFont::s_NameFont.GetWidth(s_szConsortiaNamePart[i]);
				}
			}

			// j???
			if (*(_pOwn->GetPreName())) {
				strncpy(s_sNamePart[PRENAME_SEP1_INDEX], "(", strlen("("));
				strncpy(s_sNamePart[PRENAME_INDEX], _pOwn->GetPreName(), NAME_LENGTH);
				strncpy(s_sNamePart[PRENAME_SEP2_INDEX], ")", strlen(")"));
				s_dwNamePartsColors[PRENAME_INDEX][0] = _pOwn->GetPreColor();
			} else {
				strncpy(s_sNamePart[PRENAME_SEP1_INDEX], "", NAME_LENGTH);
				strncpy(s_sNamePart[PRENAME_INDEX], "", NAME_LENGTH);
				strncpy(s_sNamePart[PRENAME_SEP2_INDEX], "", NAME_LENGTH);
				s_dwNamePartsColors[PRENAME_INDEX][0] = 0;
			}

			// ??????
			if (*(_pOwn->getSecondName())) {
				strncpy(s_sNamePart[MOTTO_NAME_SEP1_INDEX], "(", strlen("("));
				strncpy(s_sNamePart[MOTTO_NAME_INDEX], _pOwn->getSecondName(), NAME_LENGTH);
				strncpy(s_sNamePart[MOTTO_NAME_SEP2_INDEX], ")", strlen(")"));
			} else {
				strncpy(s_sNamePart[MOTTO_NAME_SEP1_INDEX], "", NAME_LENGTH);
				strncpy(s_sNamePart[MOTTO_NAME_INDEX], "", NAME_LENGTH);
				strncpy(s_sNamePart[MOTTO_NAME_SEP2_INDEX], "", NAME_LENGTH);
			}

			// ?????????

			// Added by NINJA?#5684 June 2021 credits to Igor Silva#2195
			if (_ShowEnemyNames == true && _pOwn->IsPlayer() && CGameScene::GetMainCha()) {
				if (!_pOwn->IsMainCha() && _pOwn->GetIsPK()) {
					if (_pOwn->GetTeamLeaderID() && _pOwn->GetTeamLeaderID() == CGameApp::GetCurScene()->GetMainCha()->GetTeamLeaderID())
						SetNameColor(COLOR_GREEN);
					else if (_pOwn->getGuildID() && _pOwn->getGuildID() == CGameApp::GetCurScene()->GetMainCha()->getGuildID())
						SetNameColor(COLOR_GREEN);
					else
						SetNameColor(COLOR_RED);
				} else {
					SetNameColor(COLOR_WHITE);
				}
			}

			s_dwNamePartsColors[NAME_INDEX][0] = _dwNameColor;

			// Added by Mdr.st May 2020 - FPO alpha
			/* if (_ShowEnemyNames == true && _pOwn->GetIsPK() && !_pOwn->IsMainCha() && !_pOwn->IsNPC() ) {
				s_dwNamePartsColors[NAME_INDEX][0] = COLOR_RED;
			}
			if (_ShowEnemyNames == true && _pOwn->GetHeadSay()->isTeamMember && _pOwn->GetHeadSay()->isGuildMember && !_pOwn->IsMainCha() && _pOwn->GetIsPK()){
				s_dwNamePartsColors[NAME_INDEX][0] = COLOR_GREEN;
			} */

			// End

			// _dwNameColor;
			if (_pOwn->IsBoat()) {
				strncpy(s_sNamePart[NAME_INDEX], _pOwn->getHumanName(), NAME_LENGTH);
				strncpy(s_sNamePart[BOAT_NAME_SEP1_INDEX], "[", strlen("["));
				strncpy(s_sNamePart[BOAT_NAME_INDEX], _pOwn->getName(), NAME_LENGTH);
				strncpy(s_sNamePart[BOAT_NAME_SEP2_INDEX], "]", strlen("]"));
			} else {
				strncpy(s_sNamePart[BOAT_NAME_SEP1_INDEX], "", NAME_LENGTH);
				strncpy(s_sNamePart[BOAT_NAME_INDEX], "", NAME_LENGTH);
				strncpy(s_sNamePart[BOAT_NAME_SEP2_INDEX], "", NAME_LENGTH);
				strncpy(s_sNamePart[NAME_INDEX], _pOwn->getName(), NAME_LENGTH);

				if (_pOwn->IsMonster()) { // ????????????????????10?????????????
					if (_ShowInfo && !_IsShowName) {
						return;
					}

					// ???????
					static int nMainLevel(0);
					if (g_stUIBoat.GetHuman()) {
						nMainLevel = g_stUIBoat.GetHuman()->getGameAttr()->get(ATTR_LV);
					}

					// ??????
					static int nMonsterLevel(0);
					nMonsterLevel = _pOwn->getGameAttr()->get(ATTR_LV);

					static char szBuf[NAME_LENGTH] = {0};
					if (nMonsterLevel - nMainLevel <= 10) { // ??????
						sprintf(szBuf, "Lv%d %s", nMonsterLevel, _pOwn->getName());
					} else {
						sprintf(szBuf, "??? %s", _pOwn->getName());
					}
					strncpy(s_sNamePart[NAME_INDEX], szBuf, NAME_LENGTH);
				}
			}

			/*if (_pOwn->IsNPC())
			{
				s_dwNamePartsColors[NAME_INDEX][0] = 0xFF9BA1E9; // D3DXCOLOR(109,87,218,1);
			}*/

			// ?õ?????????
			s_szName[0] = '\0';
			// ZeroMemory( s_szName, sizeof(s_szName) );
			for (int i(0); i < NAME_PART_NUM; i++) {
				strncat(s_szName, s_sNamePart[i], NAME_LENGTH);
			}
			int nNameLength = 0 - CGuiFont::s_NameFont.GetWidth(s_szName) / 2;
			int iStartPosX = 0 - CGuiFont::s_NameFont.GetWidth(s_szName) / 2;

			if (_IsShowEvil) {
				if (_pImgEvil) {
					if (_sEvilLevel != EVIL_MAX) {
						_pImgEvil[_sEvilLevel].Render(x + iStartPosX - 16, y - iNameHeightStep);
					}
				}
			}

			// renderÿh??????

			for (int i(0); i < NAME_PART_NUM; i++) {
				if (s_dwNamePartsColors[i][1]) {
					RenderNameText(s_sNamePart[i], x + iStartPosX, y - iNameHeightStep, s_dwNamePartsColors[i][0], s_dwNamePartsColors[i][1]);
				} else {
					RenderNameText(s_sNamePart[i], x + iStartPosX, y - iNameHeightStep, s_dwNamePartsColors[i][0], COLOR_BLACK);
				}
				iStartPosX += CGuiFont::s_NameFont.GetWidth(s_sNamePart[i]);
			}
		}
#else
		if (_IsShowName) {
			if (_pOwn->IsBoat()) // ????????+????
			{
				static string sNameBuf;
				sNameBuf = _pOwn->getHumanName();
				sNameBuf += "--";
				sNameBuf += _pOwn->getName();

				int nNameLength = 0 - CGuiFont::s_NameFont.GetWidth(sNameBuf.c_str()) / 2;
				RenderNameText(sNameBuf.c_str(), x + nNameLength, y - LINE_HEIGHT_STEP, _dwNameColor, COLOR_BLACK);
				if (_pOwn->IsShowSecondName()) {
					if (strlen(_pOwn->getSecondName()) > 0) {
						string strSec = "(";
						strSec += _pOwn->getSecondName();
						strSec += ")";
						CGuiFont::s_NameFont.Render(strSec.c_str(), x - nNameLength, y - LINE_HEIGHT_STEP, COLOR_BLACK);
					}
				}
			} else // ????????
			{
				int nNameLength = 0 - CGuiFont::s_NameFont.GetWidth(_pOwn->getName()) / 2;
				RenderNameText(_pOwn->getName(), x + nNameLength, y - LINE_HEIGHT_STEP, _dwNameColor, COLOR_BLACK);
				if (_pOwn->IsShowSecondName()) {
					if (strlen(_pOwn->getSecondName()) > 0) {
						string strSec = "(";
						strSec += _pOwn->getSecondName();
						strSec += ")";
						CGuiFont::s_NameFont.Render(strSec.c_str(), x - nNameLength, y - LINE_HEIGHT_STEP, COLOR_BLACK);
					}
				}

			} // end of if (_pOwn->IsBoat())
		}
#endif
	} else if (_nShowTime > 0) // ????
	{
		_nShowTime--;

		if (_pObj) {
			int left = x - (int)(_fScale * _pObj->GetWidth() / 2);
			int nLine = _pObj->GetLineNum();

			string str = _pObj->GetString();

			if (str.find("#") != std::string::npos) {
				if (nLine == 1) {
					GetRender().FillFrame(left - 2, y - 20 - 32, left + (int)(_fScale * _pObj->GetWidth()) + 4, y - 14, _dwBkgColor);
					_pObj->RenderEx(left, y - 18 - 32, 32, _fScale);
				} else if (nLine == 2) {
					GetRender().FillFrame(left - 2, y - 20 - 44, left + (int)(_fScale * _pObj->GetWidth()) + 4, y - 14, _dwBkgColor);
					_pObj->RenderEx(left, y - 18 - 44, 24, _fScale);

				} else if (nLine == 3) {
					GetRender().FillFrame(left - 2, y - 20 - 76, left + (int)(_fScale * _pObj->GetWidth()) + 4, y - 14, _dwBkgColor);
					_pObj->RenderEx(left, y - 18 - 76, 24, _fScale);
				}
			} else {
				if (nLine == 1) {
					GetRender().FillFrame(left - 2, y - 20 - 18, left + (int)(_fScale * _pObj->GetWidth()) + 4, y - 14, _dwBkgColor);
					_pObj->RenderEx(left, y - 18 - 18, 12, _fScale);
				} else if (nLine == 2) {
					GetRender().FillFrame(left - 2, y - 20 - 36, left + (int)(_fScale * _pObj->GetWidth()) + 4, y - 14, _dwBkgColor);
					_pObj->RenderEx(left, y - 18 - 36, 12, _fScale);
				} else if (nLine == 3) {
					GetRender().FillFrame(left - 2, y - 20 - 54, left + (int)(_fScale * _pObj->GetWidth()) + 4, y - 14, _dwBkgColor);
					_pObj->RenderEx(left, y - 18 - 54, 12, _fScale);
				}
			}
			_fScale += 0.2f;
			if (_fScale > 1.0f)
				_fScale = 1.0f;

			if (_nShowTime == 4)
				_fScale = 0.8f;
			else if (_nShowTime == 3)
				_fScale = 0.6f;
			else if (_nShowTime == 2)
				_fScale = 0.4f;
			else if (_nShowTime == 1)
				_fScale = 0.2f;
		}
	}

#define FFQ 60
	if (_pCurFace && _dwFaceTime) // ??
	{
		if (_dwFaceTime == 1)
			_dwFaceTime = g_dwCurFrameTick;

		DWORD dwElaped = g_dwCurFrameTick - _dwFaceTime;
		y -= 20;
		if (dwElaped > _nMaxShowTime * FFQ / 2) {
			_dwFaceTime = 0;
		} else if (_pCurFace->GetMax()) {
			DWORD dwCurFrame = (dwElaped / (_nFaceFrequency * FFQ)) % _pCurFace->GetMax();
			_pCurFace->SetFrame(dwCurFrame);
			_pCurFace->Render(x - _pCurFace->GetWidth() / 2, y - 20);
		}
	}

#ifdef _LOG_NAME_ // ???????
	if (CCharacter::IsShowLogName) {
		CChaStateMgr* pState = _pOwn->GetStateMgr();
		int nCount = pState->GetSkillStateNum();
		for (int i = 0; i < nCount; i++) {
			y -= 20;
			CGuiFont::s_Font.Render(pState->GetSkillState(i)->szName, x, y, COLOR_WHITE);
		}
	}
#endif
}

bool CHeadSay::SetFaceID(unsigned int faceid) {
	if (faceid >= FACE_MAX)
		return false;

	_nCurFaceFrame = 0;
	_nCurFaceCycle = 0;
	_dwFaceTime = 1;
	_pCurFace = &_pImgFace[faceid];
	return true;
}

void CHeadSay::SetLifeNum(int num, int max) {
	if (max == 0)
		return;

	if (num < 0)
		num = 0;
	if (num > max)
		num = max;
	if (_pImgLife) {
		_fLifeW = (float)num / (float)max;
		_fCurHp = num;
		_fMxHp = max;
	}
}

void CHeadSay::SetManaNum(int num, int max) {
	if (max == 0)
		return;

	if (num < 0)
		num = 0;
	if (num > max)
		num = max;
	if (_pImgMana) {
		_fManaW = (float)num / (float)max;
		_fCurSp = num;
		_fMxSp = max;
	}
}

void CHeadSay::_RenderShop(const char* szShopName, int x, int y) {
	static int nImageHeight = _ImgShop[0].GetHeight();
	static int nImageWidth = _ImgShop[0].GetWidth();
	y -= nImageHeight;

	static int nFontWidth = 0;
	nFontWidth = CGuiFont::s_Font.GetWidth(szShopName);
	const DWORD dwImgColor = 0xffffffff;

	if (nFontWidth < 40) {
		static int nFontTrueWidth = 0;
		nFontTrueWidth = nFontWidth;
		nFontWidth = 40;

		x -= _nShopFrameWidth + nFontWidth / 2;

		_nShopX0 = x;
		_nShopY0 = y;
		_nShopX1 = x + nFontWidth + nImageWidth;
		_nShopY1 = y + nImageHeight;

		_ImgShop[0].Render(x, y, dwImgColor);
		x += _nShopFrameWidth;

		_ImgShop[2].Render(x + nFontWidth, y, dwImgColor);

		_ImgShop[1].SetScaleW(nFontWidth);
		_ImgShop[1].Render(x, y, dwImgColor);
		// CGuiFont::s_Font.BRender( szShopName, x + (nFontWidth - nFontTrueWidth) / 2, y+_nShopFontYOff, COLOR_BLACK, COLOR_WHITE );
		CGuiFont::s_Font.Render(szShopName, x + (nFontWidth - nFontTrueWidth) / 2, y + _nShopFontYOff, COLOR_BLACK);
	} else {
		x -= _nShopFrameWidth + nFontWidth / 2;

		_nShopX0 = x;
		_nShopY0 = y;
		_nShopX1 = x + nFontWidth + nImageWidth;
		_nShopY1 = y + nImageHeight;

		_ImgShop[0].Render(x, y, dwImgColor);
		x += _nShopFrameWidth;

		_ImgShop[2].Render(x + nFontWidth, y, dwImgColor);

		_ImgShop[1].SetScaleW(nFontWidth);
		_ImgShop[1].Render(x, y, dwImgColor);
		// CGuiFont::s_Font.BRender( szShopName, x, y+_nShopFontYOff, COLOR_BLACK, COLOR_WHITE );
		CGuiFont::s_Font.Render(szShopName, x, y + _nShopFontYOff, COLOR_BLACK);
	}
}

void CHeadSay::_RenderShop2(const char* szShopName, int x, int y) {
	static int nImageHeight = _ImgShop2[0].GetHeight();
	static int nImageWidth = _ImgShop2[0].GetWidth();
	y -= nImageHeight;

	static int nFontWidth = 0;
	nFontWidth = CGuiFont::s_Font.GetWidth(szShopName);
	const DWORD dwImgColor = 0xffffffff;
	if (nFontWidth < 40) {
		static int nFontTrueWidth = 0;
		nFontTrueWidth = nFontWidth;
		nFontWidth = 40;

		x -= _nShopFrameWidth + nFontWidth / 2;

		_nShopX0 = x;
		_nShopY0 = y;
		_nShopX1 = x + nFontWidth + nImageWidth;
		_nShopY1 = y + nImageHeight;

		_ImgShop2[0].Render(x, y, dwImgColor);
		x += _nShopFrameWidth;

		_ImgShop2[2].Render(x + nFontWidth, y, dwImgColor);

		_ImgShop2[1].SetScaleW(nFontWidth);
		_ImgShop2[1].Render(x, y, dwImgColor);
		// CGuiFont::s_Font.BRender( szShopName, x + (nFontWidth - nFontTrueWidth) / 2, y+_nShopFontYOff, COLOR_BLACK, COLOR_WHITE );
		CGuiFont::s_Font.Render(szShopName, x + (nFontWidth - nFontTrueWidth) / 2, y + _nShopFontYOff, COLOR_BLACK);
	} else {
		x -= _nShopFrameWidth + nFontWidth / 2;

		_nShopX0 = x;
		_nShopY0 = y;
		_nShopX1 = x + nFontWidth + nImageWidth;
		_nShopY1 = y + nImageHeight;

		_ImgShop2[0].Render(x, y, dwImgColor);
		x += _nShopFrameWidth;

		_ImgShop2[2].Render(x + nFontWidth, y, dwImgColor);

		_ImgShop2[1].SetScaleW(nFontWidth);
		_ImgShop2[1].Render(x, y, dwImgColor);
		// CGuiFont::s_Font.BRender( szShopName, x, y+_nShopFontYOff, COLOR_BLACK, COLOR_WHITE );
		CGuiFont::s_Font.Render(szShopName, x, y + _nShopFontYOff, COLOR_BLACK);
	}
}

void CHeadSay::RenderText(const char* szShopName, int x, int y) {
	static int nImageHeight = _ImgShop[0].GetHeight();
	static int nImageWidth = _ImgShop[0].GetWidth();
	// y -= nImageHeight;
	y -= 100;

	static int nFontWidth = 0;
	nFontWidth = CGuiFont::s_Font.GetWidth(szShopName);
	const DWORD dwImgColor = 0xffffffff;

	if (nFontWidth < 40) {
		static int nFontTrueWidth = 0;
		nFontTrueWidth = nFontWidth;
		nFontWidth = 40;

		x -= _nShopFrameWidth + nFontWidth / 2;

		_ImgShop[0].Render(x, y, dwImgColor);
		x += _nShopFrameWidth;

		_ImgShop[2].Render(x + nFontWidth, y, dwImgColor);

		_ImgShop[1].SetScaleW(nFontWidth);
		_ImgShop[1].Render(x, y, dwImgColor);
		CGuiFont::s_Font.BRender(szShopName, x + (nFontWidth - nFontTrueWidth) / 2, y + _nShopFontYOff, COLOR_BLACK, COLOR_WHITE);
	} else {
		x -= _nShopFrameWidth + nFontWidth / 2;

		_ImgShop[0].Render(x, y, dwImgColor);
		x += _nShopFrameWidth;

		_ImgShop[2].Render(x + nFontWidth, y, dwImgColor);

		_ImgShop[1].SetScaleW(nFontWidth);
		_ImgShop[1].Render(x, y, dwImgColor);
		CGuiFont::s_Font.BRender(szShopName, x, y + _nShopFontYOff, COLOR_BLACK, COLOR_WHITE);
	}
}

void CHeadSay::SetEvilLevel(short sMaxEnergy) {
	if (sMaxEnergy < 4) {
		_sEvilLevel = 0;
	} else if (sMaxEnergy == 4 || sMaxEnergy == 5) {
		_sEvilLevel = 1;
	} else if (sMaxEnergy == 6) {
		_sEvilLevel = 2;
	} else if (sMaxEnergy == 7) {
		_sEvilLevel = 3;
	} else if (sMaxEnergy == 8) {
		_sEvilLevel = 4;
	} else {
		_sEvilLevel = EVIL_MAX;
	}
}

void CHeadSay::SetIsShowEvil(bool bShow) {
	_IsShowEvil = bShow;
}
