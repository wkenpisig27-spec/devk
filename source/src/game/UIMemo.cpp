#include "StdAfx.h"
#include "uimemo.h"
#include "uigraph.h"
#include "uitextparse.h"
#include "MapSet.h"
#include "MonsterSet.h"
#include <characterrecord.h>
#include "gameapp.h"
#include "Character.h"
#include "scene.h"
#include "NPCHelper.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <limits>

using namespace GUI;
using namespace std;

namespace {
std::string TrimWhitespaceCopy(const std::string& value) {
	std::string out = value;
	out.erase(out.begin(), std::find_if(out.begin(), out.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
	out.erase(std::find_if(out.rbegin(), out.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), out.end());
	return out;
}

std::string BuildMapMatchKeyForMemo(const std::string& value) {
	std::string key;
	key.reserve(value.size());

	for (size_t i = 0; i < value.size(); ++i) {
		const unsigned char ch = static_cast<unsigned char>(value[i]);
		if (std::isalnum(ch)) {
			key.push_back(static_cast<char>(std::tolower(ch)));
		}
	}

	if (key == "garner" || key == "ascaron") {
		return "garner";
	}

	if (key == "magicsea" || key == "magicalocean") {
		return "magicsea";
	}

	if (key == "darkblue" || key == "deepblue") {
		return "darkblue";
	}

	return key;
}

bool IsMapMatchForMemo(const std::string& left, const std::string& right) {
	const std::string leftKey = BuildMapMatchKeyForMemo(TrimWhitespaceCopy(left));
	const std::string rightKey = BuildMapMatchKeyForMemo(TrimWhitespaceCopy(right));
	if (leftKey.empty() || rightKey.empty()) {
		return false;
	}
	return leftKey == rightKey;
}

bool IsCountNeedComplete(const NET_MISNEED& need) {
	if ((need.byType != mission::MIS_NEED_ITEM && need.byType != mission::MIS_NEED_KILL) || need.wParam2 == 0) {
		return false;
	}

	return need.wParam3 >= need.wParam2;
}

bool TryResolveMonsterHint(int monsterId, std::string& outMapName, int& outX, int& outY) {
	if (monsterId <= 0 || !CMonsterSet::I()) {
		return false;
	}

	const int nLastId = CMonsterSet::I()->GetLastID();
	for (int mapId = 1; mapId <= nLastId; ++mapId) {
		CMonsterInfo* pInfo = GetMonsterInfo(mapId);
		if (!pInfo || !pInfo->bExist) {
			continue;
		}

		for (int i = 0; i < MONSTER_LIST_MAX; ++i) {
			if (pInfo->nMonsterList[i] != monsterId) {
				continue;
			}

			outMapName = TrimWhitespaceCopy(pInfo->szArea);
			outX = (pInfo->ptStart.x + pInfo->ptEnd.x) / 2;
			outY = (pInfo->ptStart.y + pInfo->ptEnd.y) / 2;
			return true;
		}
	}

	return false;
}

bool TryBuildNeedPayload(const NET_MISNEED& need, std::string& outPayload, std::string& outMapName, int& outX, int& outY, bool& outHasCoords) {
	outPayload.clear();
	outMapName.clear();
	outX = 0;
	outY = 0;
	outHasCoords = false;

	switch (need.byType) {
	case mission::MIS_NEED_SEND: {
		if (need.wParam1 == 0) {
			return false;
		}

		outPayload = "npc:" + std::to_string(need.wParam1);
		NPCData* pNpc = GetNPCDataInfo(need.wParam1);
		if (pNpc) {
			outMapName = TrimWhitespaceCopy(pNpc->szMapName);
			if (outMapName.empty()) {
				outMapName = TrimWhitespaceCopy(pNpc->szArea);
			}
			outX = static_cast<int>(pNpc->dwxPos0);
			outY = static_cast<int>(pNpc->dwyPos0);
			outHasCoords = true;
		}
		return true;
	}
	case mission::MIS_NEED_KILL: {
		if (need.wParam1 == 0) {
			return false;
		}

		outPayload = "monster:" + std::to_string(need.wParam1);
		outHasCoords = TryResolveMonsterHint(need.wParam1, outMapName, outX, outY);
		return true;
	}
	case mission::MIS_NEED_CONVOY:
	case mission::MIS_NEED_EXPLORE: {
		if (need.wParam2 < 0 || need.wParam3 < 0) {
			return false;
		}

		CMapInfo* pMap = GetMapInfo(need.wParam1);
		outMapName = pMap ? std::string(pMap->szName) : "";
		if (!outMapName.empty()) {
			outPayload = "map:" + outMapName + ":" + std::to_string(need.wParam2) + ":" + std::to_string(need.wParam3);
		} else {
			outPayload = "coord:" + std::to_string(need.wParam2) + ":" + std::to_string(need.wParam3);
		}
		outX = static_cast<int>(need.wParam2);
		outY = static_cast<int>(need.wParam3);
		outHasCoords = true;
		return true;
	}
	default:
		return false;
	}
}

int ComputeNeedScore(const NET_MISNEED& need,
	int needIndex,
	const std::string& needMap,
	bool hasCoords,
	int tx,
	int ty,
	bool hasPlayerPos,
	int cx,
	int cy,
	const std::string& currentMap,
	const std::string& currentTerrain) {
	int score = 10000 - (needIndex * 250);

	switch (need.byType) {
	case mission::MIS_NEED_SEND:
		score += 200;
		break;
	case mission::MIS_NEED_KILL:
		score += 160;
		break;
	case mission::MIS_NEED_CONVOY:
	case mission::MIS_NEED_EXPLORE:
		score += 140;
		break;
	default:
		break;
	}

	if (!needMap.empty()) {
		if (IsMapMatchForMemo(needMap, currentMap) || IsMapMatchForMemo(needMap, currentTerrain)) {
			score += 180;
		} else {
			score -= 140;
		}
	}

	if (hasCoords && hasPlayerPos) {
		const int dist = std::abs(tx - cx) + std::abs(ty - cy);
		score += std::max(0, 420 - (dist / 6));
	}

	return score;
}

bool TryParseLegacyCoordPayload(const std::string& raw, int& outX, int& outY) {
	std::string inner = TrimWhitespaceCopy(raw);
	if (inner.empty()) {
		return false;
	}

	if (inner.front() == '(' && inner.back() == ')') {
		inner = TrimWhitespaceCopy(inner.substr(1, inner.size() - 2));
	}

	const size_t commaPos = inner.find(',');
	if (commaPos == std::string::npos) {
		return false;
	}

	const std::string xPart = TrimWhitespaceCopy(inner.substr(0, commaPos));
	const std::string yPart = TrimWhitespaceCopy(inner.substr(commaPos + 1));
	if (xPart.empty() || yPart.empty()) {
		return false;
	}

	try {
		outX = std::stoi(xPart);
		outY = std::stoi(yPart);
		return true;
	} catch (...) {
		return false;
	}
}
} // namespace

CMemo::CMemo(CForm& frmOwn)
	: CCompent(frmOwn), _IsTextCenter(true), _color(0xff000000), _nItemRowNum(0), _nMisRowNum(0), evtSelectChange(nullptr), _nMaxNum(40), _nRowNum(0), _nFirstRow(0), _nLastRow(0), _nPageShowNum(4), _bIsHaveItem(false), _bIsHaveMis(false), _nLeftMargin(0), _nTopMargin(4), _nRightMargin(4), _nBottomMargin(4), _nRowHeight(20), _nSelectItem(-1), _nSelectMission(-1), _nMemoNum(0) {
	int i = 0;
	for (; i < MEMO_MAX_LINE; i++)
		_str[i] = "";

	for (i = 0; i < MEMO_MAX_ITEM; i++)
		_strItem[i] = "";
	for (i = 0; i < MEMO_MAX_MISSION; i++)
		_strMis[i] = "";

	for (i = 0; i < 3; i++)
		for (int j = 0; j < 2; j++)
			_nRowInfo[i][j] = 0;

	_strCaption = "";
	_pScroll = new CScroll(frmOwn);
	_SetSelf();
}

CMemo::CMemo(const CMemo& rhs)
	: CCompent(rhs), _color(rhs._color), _strCaption(rhs._strCaption), _IsTextCenter(true) {
	_pScroll = new CScroll(*rhs._pScroll);
	_nLeftMargin = rhs._nLeftMargin;
	_nTopMargin = rhs._nTopMargin;
	_nRightMargin = rhs._nRightMargin;
	_nBottomMargin = rhs._nBottomMargin;
	_bIsHaveItem = rhs._bIsHaveItem;
	_bIsHaveMis = rhs._bIsHaveMis;
	_nItemRowNum = rhs._nItemRowNum;
	_nMisRowNum = rhs._nMisRowNum;

	_nMemoNum = rhs._nMemoNum;
	evtSelectChange = rhs.evtSelectChange;

	_Copy(rhs);
}

void CMemo::reset() {
	int i = 0;
	for (; i < MEMO_MAX_LINE; i++)
		_str[i] = "";

	for (i = 0; i < MEMO_MAX_ITEM; i++)
		_strItem[i] = "";

	for (i = 0; i < MEMO_MAX_MISSION; i++)
		_strMis[i] = "";

	for (i = 0; i < 3; i++)
		for (int j = 0; j < 2; j++)
			_nRowInfo[i][j] = 0;

	_strCaption = "";
	_nSelectItem = -1;
	_nSelectMission = -1;
	_nMemoNum = 0;
	_bIsHaveItem = false;
	_bIsHaveMis = false;
	_nItemRowNum = 0;
	_nMisRowNum = 0;

	while (!_files.empty()) {
		_files.pop_back();
	}
}

CMemo& CMemo::operator=(const CMemo& rhs) {
	CCompent::operator=(rhs);
	*_pScroll = *rhs._pScroll;

	_nLeftMargin = rhs._nLeftMargin;
	_nTopMargin = rhs._nTopMargin;
	_nRightMargin = rhs._nRightMargin;
	_nBottomMargin = rhs._nBottomMargin;
	_bIsHaveItem = rhs._bIsHaveItem;
	_bIsHaveMis = rhs._bIsHaveMis;
	_nItemRowNum = rhs._nItemRowNum;
	_nMisRowNum = rhs._nMisRowNum;

	_nMemoNum = rhs._nMemoNum;

	evtSelectChange = rhs.evtSelectChange;

	_Copy(rhs);
	return *this;
}

CMemo::~CMemo(void) {
}

void CMemo::_Copy(const CMemo& rhs) {
	_nRowHeight = rhs._nRowHeight;
	_nMaxNum = rhs._nMaxNum;
	_nRowNum = rhs._nRowNum;
	_nItemRowNum = rhs._nItemRowNum;
	_nMisRowNum = rhs._nMisRowNum;
	_nMemoNum = rhs._nMemoNum;
	_nSelectItem = rhs._nSelectItem;
	_nSelectMission = rhs._nSelectMission;
	_strCaption = rhs._strCaption;
	_nFirstRow = rhs._nFirstRow;
	_nLastRow = rhs._nLastRow;
	_nPageShowNum = rhs._nPageShowNum;

	_SetSelf();
}

void CMemo::_SetSelf() {
	_pScroll->SetParent(this);
	_pScroll->SetIsShow(true);
	_pScroll->evtChange = _OnScrollChange;
}

void CMemo::_CheckTextAlign() {
}

void CMemo::_CheckScroll() {
}
void CMemo::AddItemRowContent(int row, const char* szFunc, const char* szItemEx) {
	if (!szFunc) {
		LG("errormemo", "msgCMemo::AddItemRowContent szFunc is null!\n");
		return;
	}

	if (row >= MEMO_MAX_ITEM || row < 0) {
		LG("errormemo", "msgCMemo::AddItemRowContent( %d, %s ) out of range!\n", row, szFunc);
		return;
	}
	// ??????
	_strItem[row] = szFunc;
	_strItemEx[row] = szItemEx;
}

void CMemo::AddMisRowContent(int row, const char* szFunc) {
	if (!szFunc) {
		LG("errormemo", "msgCMemo::AddMisRowContent szFunc is null!\n");
		return;
	}

	if (row >= MEMO_MAX_ITEM || row < 0) {
		LG("errormemo", "msgCMemo::AddMisRowContent( %d, %s ) out of range!\n", row, szFunc);
		return;
	}

	_strMis[row] = szFunc;
}

void CMemo::ProcessCaption() {
	// Clear existing rows
	for (int nRow = 0; nRow <= _nRowNum; nRow++)
		_str[nRow] = "";
	_nRowNum = 0;

	// Calculate available pixel width for text (component width minus margins and scrollbar)
	int nAvailableWidth = GetWidth() - _nLeftMargin - _nRightMargin - _pScroll->GetWidth() - 4;
	if (nAvailableWidth <= 0)
		nAvailableWidth = 200; // Fallback minimum width

	// Process text with '_' as line break markers
	const char* s = _strCaption.c_str();
	string strTemp;
	int length = (int)strlen(s);
	int begin = 0;
	int end = 0;
	int i;
	bool bNotFlag = false;
	for (i = 0; i < length; i++) {
		if (s[i] & 0x80) {
			i++;
			continue;
		} else if (s[i] == '_') {
			bNotFlag = true;
			end = i;
			strTemp = _strCaption.substr(begin, end - begin);

			// Use pixel-based wrapping with remainder output
			while (!strTemp.empty() && CGuiFont::s_Font.GetWidth(strTemp.c_str()) > nAvailableWidth) {
				std::string remainder;
				_str[_nRowNum] = GetStringByPixelWidth(strTemp, nAvailableWidth, &remainder);
				if (_str[_nRowNum].empty()) break; // Safety check
				strTemp = remainder;
				_nRowNum += 1;
				if (_nRowNum >= MEMO_MAX_LINE - 1) break;
			}
			if (!strTemp.empty() && _nRowNum < MEMO_MAX_LINE - 1) {
				_str[_nRowNum] = strTemp;
				_nRowNum += 1;
			}
			begin = i + 1;
		}
	}
	if (bNotFlag) {
		_str[_nRowNum] = _strCaption.substr(begin, length - begin);
		_nRowNum++;
	}

	else if (!bNotFlag) {
		strTemp = _strCaption;
		// Use pixel-based wrapping with remainder output
		while (!strTemp.empty() && CGuiFont::s_Font.GetWidth(strTemp.c_str()) > nAvailableWidth) {
			std::string remainder;
			_str[_nRowNum] = GetStringByPixelWidth(strTemp, nAvailableWidth, &remainder);
			if (_str[_nRowNum].empty()) break; // Safety check
			strTemp = remainder;
			_nRowNum += 1;
			if (_nRowNum >= MEMO_MAX_LINE - 1) break;
		}
		if (!strTemp.empty() && _nRowNum < MEMO_MAX_LINE - 1) {
			_str[_nRowNum] = strTemp;
			_nRowNum += 1;
		}
	}

	_nRowInfo[0][1] = _nRowNum; // Number of memo rows

	if (_strCaption.length() == 0) {
		_nRowInfo[0][0] = _nRowInfo[0][1] = 0;
		_nRowNum = 0;
	}

	if (_bIsHaveMis) {
		_str[_nRowNum] = "";
		_nRowNum++;
		_nRowInfo[1][0] = _nRowNum;

		for (int i = 0; i < _nMisRowNum; i++) {
			_str[_nRowNum] = _strMis[i];
			_nRowNum++;
		}
		_nRowInfo[1][1] = _nRowNum;
	} else {
		//_str[_nRowNum] = "" ;
		//_nRowNum++;
		//_nRowInfo[1][0]= _nRowNum;
		_nRowInfo[1][0] = _nRowNum;
	}

	if (_bIsHaveItem) {
		_str[_nRowNum] = ""; // ??????????????????????
		_nRowNum++;
		_nRowInfo[2][0] = _nRowNum;

		for (i = 0; i < _nItemRowNum; i++) // ???lst??????
		{
			_str[_nRowNum] = _strItem[i];
			_nRowNum++;
		}
		_nRowInfo[2][1] = _nRowNum;
	} else {
		_str[_nRowNum] = "";
		_nRowNum++;
		_nRowInfo[2][0] = _nRowNum;
		_nRowInfo[2][1] = _nRowNum;
	}
	Refresh();
}

void CMemo::Init() {
	_pScroll->Reset();
	_pScroll->SetSize(_pScroll->GetWidth(), GetHeight());
	_pScroll->SetPos(GetWidth() - _pScroll->GetWidth(), 0);
	_pScroll->Init();
	_OnScrollChange();

	_strCaption = "";
	int i = 0;
	for (; i < MEMO_MAX_LINE; i++) {
		_str[i] = "";
	}

	_IsTextCenter = false;
	for (i = 0; i < MEMO_MAX_ITEM; i++) {
		_strItem[i] = "";
	}
	_nSelectItem = 0;

	for (i = 0; i < MEMO_MAX_ITEM; i++) {
		_strMis[i] = "";
	}
	_nSelectMission = 0;
	_bIsHaveItem = false;
	_bIsHaveMis = false;
}

void CMemo::AddIcon(int index, CGraph* pGraph) {
	stIconIndex f;
	f.nIndex = index;
	f.pGraph = pGraph;
	_files.push_back(f);
}

inline void CMemo::RenderRowAsMessage(int row) {
	const int nPosX = GetX() + _nLeftMargin;
	const int nPosY = GetY() + _nTopMargin + _nRowHeight * (row - _nFirstRow);
	CGuiFont::s_Font.Render((char*)_str[row].c_str(), nPosX, nPosY, 0xFF000000); // 0xFFFFFFFF );
}

inline void CMemo::RenderRowAsSelectedQuest(int row) {
	const int nPosX = GetX() + _nLeftMargin + 12;
	const int nPosY = GetY() + _nTopMargin + _nRowHeight * (row - _nFirstRow);
	if (_files[row - _nRowInfo[1][0]].pGraph && _str[row].length() > 0) {
		_files[row - _nRowInfo[1][0]].pGraph->Render(nPosX, nPosY - 3);
	}
	CGuiFont::s_Font.Render((char*)_str[row].c_str(), nPosX + 24, nPosY, 0xFFFF00FF);
}

inline void CMemo::RenderRowAsQuest(int row) {
	const int nPosX = GetX() + _nLeftMargin + 12;
	const int nPosY = GetY() + _nTopMargin + _nRowHeight * (row - _nFirstRow);
	if (_files[row - _nRowInfo[1][0]].pGraph && _str[row].length() > 0) {
		_files[row - _nRowInfo[1][0]].pGraph->Render(nPosX, nPosY - 2);
	}
	CGuiFont::s_Font.Render((char*)_str[row].c_str(), nPosX + 24, nPosY + 1, 0xFFcF00cF);
}

inline void CMemo::RenderRowAsFunction(int row) {
	const int nPosX = GetX() + _nLeftMargin;
	const int nPosY = GetY() + _nTopMargin + _nRowHeight * (row - _nFirstRow);
	if (row == _nSelectItem + _nRowInfo[2][0]) {
		CGuiFont::s_Font.Render((char*)_str[row].c_str(), nPosX + 24, nPosY, 0xFF0000FF);
	} else {
		CGuiFont::s_Font.Render((char*)_str[row].c_str(), nPosX + 24, nPosY + 1, 0xFF0000aF);
	}
}

void CMemo::Render() {
	for (int row = _nFirstRow; row < _nLastRow; row++) {
		if (RowIsMessage(row)) {
			RenderRowAsMessage(row);
		} else if (RowIsQuest(row)) {
			if (row == _nSelectMission + _nRowInfo[1][0]) {
				RenderRowAsSelectedQuest(row);
			} else {
				RenderRowAsQuest(row);
			}
		} else if (RowIsFunction(row)) {
			RenderRowAsFunction(row);
		}
	}

	if (_pScroll->GetIsShow()) {
		_pScroll->Render();
	}
}

void CMemo::Refresh() {
	CCompent::Refresh();
	_pScroll->SetRange((float)0.0f, (float)_nRowNum - 4);
	_pScroll->Refresh();
	_OnScrollChange();
}

void CMemo::SetMisRowNum(int num) {
	if (num < 0 || num >= MEMO_MAX_ITEM) {
		LG("errormemo", "msgCMemo::SetMisRowNum( %d ) out of range!\n", num);
		if (num < 0)
			num = 0;
		if (num >= MEMO_MAX_ITEM)
			num = MEMO_MAX_ITEM - 1;
	}
	_nMisRowNum = num;
}

bool CMemo::MouseRun(int x, int y, DWORD key) {
	if (!IsNormal())
		return false;
	_nSelectMission = -1;
	_nSelectItem = -1;
	if (InRect(x, y)) {
		if (_pScroll->MouseRun(x, y, key))
			return true;

		int nPosY = y - GetTop() - GetForm()->GetTop();
		int nRow = _nFirstRow + (nPosY - _nTopMargin) / _nRowHeight - _nMemoNum;
		if (_bIsHaveMis && nRow >= _nRowInfo[1][0] && nRow <= _nRowInfo[1][1]) {
			_nSelectMission = nRow - _nRowInfo[1][0];
			if (_nSelectMission >= _nMisRowNum) {
				_nSelectMission = -1;
			} else if (_strMis[_nSelectMission].length() > 0) {
				SetCursor(CCursor::stButtonClick);
			}
		} else if (_bIsHaveItem && nRow >= _nRowInfo[2][0] && nRow <= _nRowInfo[2][1]) {
			_nSelectItem = nRow - _nRowInfo[2][0];
			if (_nSelectItem >= _nItemRowNum) {
				_nSelectItem = -1;
			} else if (_strItem[_nSelectItem].length() > 0) {
				SetCursor(CCursor::stButtonClick);
			}
		}

		if ((key & Mouse_LDown)) {
			if (evtSelectChange)
				evtSelectChange(this);
		}
		return true;
	}

	return _IsMouseIn;
}

bool CMemo::GetSelectItemText(string& item, string& itemex) {
	if (_nSelectItem >= 0 && _nSelectItem < MEMO_MAX_ITEM) {
		item = _strItem[_nSelectItem];
		itemex = _strItemEx[_nSelectItem];
		return true;
	}
	return false;
}

void CMemo::_OnScrollChange() {
	_nFirstRow = (int)_pScroll->GetStep().GetPosition();
	_nLastRow = _nFirstRow + _nPageShowNum;
	if (_nLastRow > MEMO_MAX_LINE)
		_nLastRow = MEMO_MAX_LINE;
}

bool CMemo::MouseScroll(int nScroll) {
	if (!IsNormal())
		return false;

	if (_IsMouseIn)
		_pScroll->MouseScroll(nScroll);
	return _IsMouseIn;
}

void CMemo::SetMargin(int left, int top, int right, int bottom) {
	_nLeftMargin = left;
	_nTopMargin = top;
	_nRightMargin = right;
	_nBottomMargin = bottom;
}

void CMemo::_SetScrollRange() {
	_pScroll->Refresh();
}

////////////////////////////////////////////////////
// ???????????????????????????????
////////////////////////////////////////////////////

CMemoEx::CMemoEx(CForm& frmOwn)
	: CCompent(frmOwn), _color(0xff000000), evtSelectChange(nullptr), _nMaxNum(64), _nRowNum(0), _nFirst(0), _nLast(0), _nPageShowNum(4), _nLeftMargin(0), _nTopMargin(4), _nRightMargin(4), _nBottomMargin(4),
	  _nRowHeight(14), _nTitleHeight(24), _nIconHeight(18), _nPhraseNum(0), evtClickItem(nullptr) {
	_pScroll = new CScroll(frmOwn);
	_SetSelf();

	// ??'?????????????
	for (int i = 0; i < MEMO_MAX_LINE; i++) {
		m_MemoInfo[i].strDesp = "";
		m_MemoInfo[i].strLinkPayload = "";
		m_MemoInfo[i].byData = 0;
		m_MemoInfo[i].sData = 0;
		m_MemoInfo[i].byType = 0;
		m_MemoInfo[i].bClickable = false;
		m_MemoInfo[i].dwColor = 0;
		m_MemoInfo[i].pIcon = nullptr;
		m_MemoInfo[i].sxPos = 0;
		m_MemoInfo[i].syPos = 0;
	}

	// Add by lark.li 20080721 begin
	m_SelMem = -1;
	// End

	memset(&m_PageInfo, 0, sizeof(NET_MISPAGE));
	m_sNumInfo = 0;
	m_bUpdate = FALSE;
	m_pPrizePic = nullptr;
	m_pPrizeSelPic = nullptr;
	m_bIsSelect = FALSE;
}

CMemoEx::CMemoEx(const CMemoEx& rhs)
	: CCompent(rhs), _color(rhs._color) {
	_pScroll = new CScroll(*rhs._pScroll);
	_nLeftMargin = rhs._nLeftMargin;
	_nTopMargin = rhs._nTopMargin;
	_nRightMargin = rhs._nRightMargin;
	_nBottomMargin = rhs._nBottomMargin;

	_nPhraseNum = rhs._nPhraseNum;

	evtSelectChange = rhs.evtSelectChange;
	evtClickItem = rhs.evtClickItem;
	m_pPrizePic = nullptr;
	m_pPrizeSelPic = nullptr;
	m_bIsSelect = FALSE;
	_Copy(rhs);
}

void CMemoEx::Clear() {
	// ??'?????????????
	for (int i = 0; i < MEMO_MAX_LINE; i++) {
		m_MemoInfo[i].strDesp = "";
		m_MemoInfo[i].strLinkPayload = "";
		m_MemoInfo[i].byData = 0;
		m_MemoInfo[i].sData = 0;
		m_MemoInfo[i].byType = 0;
		m_MemoInfo[i].bClickable = false;
		m_MemoInfo[i].dwColor = 0;
		SAFE_DELETE(m_MemoInfo[i].pIcon);
		m_MemoInfo[i].sxPos = 0;
		m_MemoInfo[i].syPos = 0;
	}

	// Add by lark.li 20080721 begin
	m_SelMem = -1;
	// End

	memset(&m_PageInfo, 0, sizeof(NET_MISPAGE));
	m_sNumInfo = 0;
	m_bUpdate = FALSE;
	m_bySelPrize = -1;
	m_bIsSelect = FALSE;
}

CMemoEx& CMemoEx::operator=(const CMemoEx& rhs) {
	CCompent::operator=(rhs);
	*_pScroll = *rhs._pScroll;

	_nLeftMargin = rhs._nLeftMargin;
	_nTopMargin = rhs._nTopMargin;
	_nRightMargin = rhs._nRightMargin;
	_nBottomMargin = rhs._nBottomMargin;

	evtSelectChange = rhs.evtSelectChange;
	evtClickItem = rhs.evtClickItem;
	m_pPrizePic = nullptr;
	m_pPrizeSelPic = nullptr;
	m_bIsSelect = FALSE;

	_Copy(rhs);
	return *this;
}

CMemoEx::~CMemoEx(void) {
	Clear();
	SAFE_DELETE(m_pPrizePic);
	SAFE_DELETE(m_pPrizeSelPic);
}

void CMemoEx::_Copy(const CMemoEx& rhs) {
	_SetSelf();
}

void CMemoEx::_SetSelf() {
	_pScroll->SetParent(this);
	_pScroll->SetIsShow(true);
	_pScroll->evtChange = _OnScrollChange;
}

void CMemoEx::_CheckScroll() {
}

void CMemoEx::Init() {
	_pScroll->Reset();
	_pScroll->SetRange(0, 16);
	_pScroll->SetSize(_pScroll->GetWidth(), GetHeight());
	_pScroll->SetPos(GetWidth() - _pScroll->GetWidth(), 0);
	_pScroll->Init();
	_OnScrollChange();

	// ??????????????
	if (m_pPrizePic == nullptr && m_pPrizeSelPic == nullptr) {
		SAFE_DELETE(m_pPrizePic);
		SAFE_DELETE(m_pPrizeSelPic);

		m_pPrizePic = new CGuiPic;
		m_pPrizePic->LoadImage("./texture/ui/QQ2.tga", 156, 38, 0, 81, 0);
		m_pPrizeSelPic = new CGuiPic;
		m_pPrizeSelPic->LoadImage("./texture/ui/QQ2.tga", 156, 38, 0, 81, 39);
	}

	// ??'?????????????
	Clear();
}

BOOL CMemoEx::SelPrizeItem(int nxPos, int nyPos, DWORD dwKey) {
	if (!GetIsShow()) {
		return FALSE;
	}

	int nPosY = GetY() + _nTopMargin;
	int nPosX = GetX() + _nLeftMargin;

	// Row dimensions
	int nHeight = _nRowHeight;
	int x, y;
	int nInterval = 0;
	BYTE bySelID = 0;

	// Track cumulative pixel width per row for proportional font support
	int nCurrentRow = -1;
	int nRowPixelOffset = 0;

	for (int i = 0; i < m_sNumInfo; i++) {
		if (m_MemoInfo[i].byType != MEMO_LINE_ICON) {
			if (m_MemoInfo[i].syPos < _nFirst || m_MemoInfo[i].syPos > _nLast) {
				continue;
			}
		}

		// Check if we're on a new row - reset pixel offset
		if (m_MemoInfo[i].syPos != nCurrentRow) {
			nCurrentRow = m_MemoInfo[i].syPos;
			nRowPixelOffset = 0;
		}

		// Calculate x position based on cumulative pixel widths
		x = nRowPixelOffset + nPosX;
		y = (m_MemoInfo[i].syPos - _nFirst) * _nRowHeight + nPosY + nInterval * 2;
		
		int nSegmentWidth = 0;
		if (m_MemoInfo[i].byType == MEMO_LINE_TITLE) {
			nInterval++;
			nSegmentWidth = CGuiFont::s_Font.GetWidth(m_MemoInfo[i].strDesp.c_str());
		} else if (m_MemoInfo[i].byType == MEMO_LINE_DESP) {
			nSegmentWidth = CGuiFont::s_Font.GetWidth(m_MemoInfo[i].strDesp.c_str());
		} else if (m_MemoInfo[i].byType == MEMO_LINE_INTERVAL) {
			nInterval += 2;
		} else if (m_MemoInfo[i].byType == MEMO_LINE_ICON) {
			if (m_MemoInfo[i].pIcon) {
				nSegmentWidth = m_MemoInfo[i].pIcon->GetWidth();
			}
			if (nxPos >= x - 2 && nxPos <= x + nSegmentWidth + 4 && nyPos >= y && nyPos <= y + 32) {
				if (dwKey & Mouse_LDown) {
					m_bySelPrize = bySelID;
				}
				return TRUE;
			}
			bySelID++;
		} else {
			// Unknown type
		}
		nRowPixelOffset += nSegmentWidth;
	}
	return FALSE;
}

// Add by lark.li 20080721 begin
int CMemoEx::SelMemInfo(int x, int y, DWORD key) {
	if (!GetIsShow())
		return -1;

	int nPosY = GetY() + _nTopMargin;
	int nPosX = GetX() + _nLeftMargin;

	// Row dimensions
	int nHeight = _nRowHeight;
	int startX, startY;

	int nInterval = 0;

	// Track cumulative pixel width per row for proportional font support
	int nCurrentRow = -1;
	int nRowPixelOffset = 0;

	for (int i = 0; i < m_sNumInfo; i++) {
		if (m_MemoInfo[i].byType != MEMO_LINE_ICON) {
			if (m_MemoInfo[i].syPos < _nFirst || m_MemoInfo[i].syPos > _nLast) {
				continue;
			}
		}

		// Check if we're on a new row - reset pixel offset
		if (m_MemoInfo[i].syPos != nCurrentRow) {
			nCurrentRow = m_MemoInfo[i].syPos;
			nRowPixelOffset = 0;
		}

		// Calculate position based on cumulative pixel widths
		startX = nRowPixelOffset + nPosX;
		startY = (m_MemoInfo[i].syPos - _nFirst) * _nRowHeight + nPosY + nInterval * 2;

		int nSegmentWidth = 0;
		if (m_MemoInfo[i].byType == MEMO_LINE_TITLE) {
			nInterval++;
			nSegmentWidth = CGuiFont::s_Font.GetWidth(m_MemoInfo[i].strDesp.c_str());
		} else if (m_MemoInfo[i].byType == MEMO_LINE_DESP) {
			nSegmentWidth = CGuiFont::s_Font.GetWidth(m_MemoInfo[i].strDesp.c_str());
			if (startX < x && (x < startX + nSegmentWidth) && startY < y && (y < startY + nHeight)) {
				return i;
			}
		} else if (m_MemoInfo[i].byType == MEMO_LINE_INTERVAL) {
			nInterval += 2;
		} else if (m_MemoInfo[i].byType == MEMO_LINE_ICON) {
			if (m_MemoInfo[i].pIcon) {
				nSegmentWidth = m_MemoInfo[i].pIcon->GetWidth();
			}
		} else {
			// Unknown type
		}
		nRowPixelOffset += nSegmentWidth;
	}

	return -1;
}
// End

void CMemoEx::Render() {
	if (!GetIsShow())
		return;

	int nPosY = GetY() + _nTopMargin;
	int nPosX = GetX() + _nLeftMargin;

	// Row dimensions
	int nHeight = _nRowHeight;
	int nWidth = CGuiFont::s_Font.GetWidth("a");
	int x, y;
	int nInterval = 0;
	BYTE bySelID = 0;

	// Clip rect
	GetRender().SetClipRect(nPosX, nPosY, 32 * nWidth, (_nPageShowNum + 1) * nHeight);

	// Track cumulative pixel width per row for proportional font support
	int nCurrentRow = -1;
	int nRowPixelOffset = 0;

	for (int i = 0; i < m_sNumInfo; i++) {
		if (m_MemoInfo[i].byType != MEMO_LINE_ICON) {
			if (m_MemoInfo[i].syPos < _nFirst || m_MemoInfo[i].syPos > _nLast) {
				continue;
			}
		}

		// Check if we're on a new row - reset pixel offset
		if (m_MemoInfo[i].syPos != nCurrentRow) {
			nCurrentRow = m_MemoInfo[i].syPos;
			nRowPixelOffset = 0;
		}

		// Calculate x position based on cumulative pixel widths
		x = nRowPixelOffset + nPosX;
		y = (m_MemoInfo[i].syPos - _nFirst) * _nRowHeight + nPosY + nInterval * 2;
		
		if (m_MemoInfo[i].byType == MEMO_LINE_TITLE) {
			CGuiFont::s_Font.Render(2, m_MemoInfo[i].strDesp.c_str(), x, y, m_MemoInfo[i].dwColor);
			nInterval++;
			// Update cumulative width
			nRowPixelOffset += CGuiFont::s_Font.GetWidth(m_MemoInfo[i].strDesp.c_str());
		} else if (m_MemoInfo[i].byType == MEMO_LINE_DESP) {
			CGuiFont::s_Font.Render(m_MemoInfo[i].strDesp.c_str(), x, y, m_MemoInfo[i].dwColor);
			// Update cumulative width
			nRowPixelOffset += CGuiFont::s_Font.GetWidth(m_MemoInfo[i].strDesp.c_str());
		} else if (m_MemoInfo[i].byType == MEMO_LINE_INTERVAL) {
			nInterval += 2;
		} else if (m_MemoInfo[i].byType == MEMO_LINE_ICON) {
			if (m_bySelPrize == bySelID) {
				m_pPrizeSelPic->Render(x - 4, y - 3);
			} else {
				m_pPrizePic->Render(x - 4, y - 3);
			}
			m_MemoInfo[i].pIcon->Render(x, y);
			bySelID++;
			// Update cumulative width for icons
			if (m_MemoInfo[i].pIcon) {
				nRowPixelOffset += m_MemoInfo[i].pIcon->GetWidth();
			}
		} else {
			// Unknown type
		}
	}

	GetRender().Reset();
	if (_pScroll->GetIsShow())
		_pScroll->Render();
}

std::vector<std::string> stringQuebrada(std::string original) {
	std::string temp = "";
	std::vector<std::string> stringQuebrada;
	for (int i = 0; i < original.length(); i++) {
		;
		if (original[i] != ' ') {
			temp = temp + original[i];
		} else if (original[i] == ' ') {
			stringQuebrada.push_back(temp);
			temp = "";
		}
	}
	return stringQuebrada;
}

std::string justificarNew(string texto) {
	// std::string texto = "Seja Bem vindo ao 'Soul of Pirates Online'. \n Voc? acaba de embarcar em um Mundo conhecido como Orisord, um lugar repleto de criaturas m?sticas, tesouros e segredos que o aguardam para grandes aventuras. \n Sua jornada se inicia em uma cidade conhecida como Argent City, localizada na Regi?o de Ascaron. Sinta-se livre para explorar-la ao m?ximo. \n  Ao longo de sua jornada voc? poder? escolher entre fazer parte das For?as da Marinha ou de Grandes Organiza??es Piratas. Tal escolha ser? ?nica e muito importante para o seu futuro. \n  Por enquanto, o seu primeiro objetivo ? encontrar a <bInstrutora Senna> em <rArgent City> nas coordenadas (2223, 2785). Ela ir? o auxiliar no come?o de sua mais nova aventura. ";
	texto.append(" ");
	const int nMax = 32;
	for (int i = 0; i < stringQuebrada(texto).size(); i++) {
	}
	std::string linhaTemp = "";
	std::string stringFinal = "";
	for (int i = 0; i < stringQuebrada(texto).size(); i++) {

		linhaTemp += stringQuebrada(texto)[i] + " ";
		if (linhaTemp.find("<n>") != -1) {
			linhaTemp.erase(linhaTemp.find("<n>") + 2);
			stringFinal += linhaTemp;
			linhaTemp = "";
			continue;
		} else if (i + 1 == stringQuebrada(texto).size()) {
			stringFinal += linhaTemp;
			break;
		} else if (linhaTemp.length() + stringQuebrada(texto)[i + 1].length() > nMax) {
			linhaTemp.erase(linhaTemp.length() - 1);
			int k = 0;

			while (linhaTemp.length() < nMax) {
				int pos[nMax];

				if (linhaTemp[k] == ' ') {
					linhaTemp.insert(k, " ");
					k = linhaTemp.find_first_not_of(" ", k) + 1;
					continue;
				}
				k++;
				if (k > linhaTemp.length())
					k = 0;
			}
			stringFinal += linhaTemp + "<n>";
			linhaTemp = "";
		}
	}

	return stringFinal;
}

void CMemoEx::SetMisPage(const NET_MISPAGE& page) {
	m_bUpdate = TRUE;
	memcpy(&m_PageInfo, &page, sizeof(NET_MISPAGE));
	ParseMisPage();
}

void CMemoEx::ParseMisPage() {
	USHORT sMaxCom = 32;
	USHORT sRow = 0, sCom = 0;
	DWORD dwColor = 0xFF000000; // ???

	// ???????????
	m_MemoInfo[m_sNumInfo].strDesp = m_PageInfo.szName;
	m_MemoInfo[m_sNumInfo].dwColor = 0xFF000000;
	m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_TITLE;
	m_MemoInfo[m_sNumInfo].sxPos = sCom;
	m_MemoInfo[m_sNumInfo].syPos = sRow;

	// ??'??h??
	m_sNumInfo++;
	sRow++;
	m_MemoInfo[m_sNumInfo].strDesp = "";
	m_MemoInfo[m_sNumInfo].dwColor = dwColor;
	m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_DESP;
	m_MemoInfo[m_sNumInfo].sxPos = sCom;
	m_MemoInfo[m_sNumInfo].syPos = sRow;

	// ????????????
	char* pszData = m_PageInfo.szDesp;
	char* pszTemp = pszData;
	ParseScript(pszData, m_sNumInfo, sRow, sCom, 0, sMaxCom, 0xFF000000, m_MemoInfo, MEMO_MAX_LINE);

	// ??'??h??
	m_sNumInfo++;
	// sRow++;
	m_MemoInfo[m_sNumInfo].strDesp = "";
	m_MemoInfo[m_sNumInfo].dwColor = dwColor;
	m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_INTERVAL;
	m_MemoInfo[m_sNumInfo].sxPos = sCom;
	m_MemoInfo[m_sNumInfo].syPos = sRow;

	char szNumber[128] = {0};
	// ???????
	if (m_PageInfo.byNeedNum > 0) {
		sRow++;
		sCom = 0;
		dwColor = 0xFF000000;
		m_sNumInfo++;
		m_MemoInfo[m_sNumInfo].strDesp = RES_STRING(CL_LANGUAGE_MATCH_704);
		m_MemoInfo[m_sNumInfo].dwColor = dwColor;
		m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_TITLE;
		m_MemoInfo[m_sNumInfo].sxPos = sCom;
		m_MemoInfo[m_sNumInfo].syPos = sRow;

		for (int i = 0; i < m_PageInfo.byNeedNum; i++) {
			string strTemp = "  ";
			if (m_PageInfo.MisNeed[i].byType == mission::MIS_NEED_ITEM) {
				strTemp += RES_STRING(CL_LANGUAGE_MATCH_705);
				CItemRecord* pItem = GetItemRecordInfo(m_PageInfo.MisNeed[i].wParam1);
				if (!pItem) {
					strTemp += RES_STRING(CL_LANGUAGE_MATCH_706);
					sprintf(szNumber, "0x%X", m_PageInfo.MisNeed[i].wParam1);
					strTemp += szNumber;
					strTemp += "]>";
				} else {
					strTemp += RES_STRING(CL_LANGUAGE_MATCH_707);
					strTemp += pItem->szName;
					strTemp += RES_STRING(CL_LANGUAGE_MATCH_708);
				}
				szNumber[0] = 0;
				sprintf(szNumber, "<r%d/%d>", m_PageInfo.MisNeed[i].wParam3, m_PageInfo.MisNeed[i].wParam2);
				size_t nLen = strTemp.length() + strlen(szNumber) - 6;
				if (nLen > sMaxCom) {
					strTemp += szNumber;
				} else {
					for (int n = 0; n < int(sMaxCom - nLen); n++) {
						strTemp += " ";
					}
					strTemp += szNumber;
				}
			} else if (m_PageInfo.MisNeed[i].byType == mission::MIS_NEED_KILL) {
				strTemp += RES_STRING(CL_LANGUAGE_MATCH_709);
				CChaRecord* pInfo = GetChaRecordInfo(m_PageInfo.MisNeed[i].wParam1);
				if (!pInfo) {
					strTemp += RES_STRING(CL_LANGUAGE_MATCH_706);
					sprintf(szNumber, "0x%X", m_PageInfo.MisNeed[i].wParam1);
					strTemp += szNumber;
					strTemp += "]>";
				} else {
					strTemp += RES_STRING(CL_LANGUAGE_MATCH_707);
					strTemp += "<nav:monster:" + std::to_string(m_PageInfo.MisNeed[i].wParam1) + "|" + std::string(pInfo->szName) + ">";
					strTemp += RES_STRING(CL_LANGUAGE_MATCH_708);
				}
				szNumber[0] = 0;
				sprintf(szNumber, "<r%d/%d>", m_PageInfo.MisNeed[i].wParam3, m_PageInfo.MisNeed[i].wParam2);
				size_t nLen = strTemp.length() + strlen(szNumber) - 6;
				if (nLen > sMaxCom) {
					strTemp += szNumber;
				} else {
					for (int n = 0; n < int(sMaxCom - nLen); n++) {
						strTemp += " ";
					}
					strTemp += szNumber;
				}
			} else if (m_PageInfo.MisNeed[i].byType == mission::MIS_NEED_DESP) {
				strTemp += m_PageInfo.MisNeed[i].szNeed;
			} else {
				// ?????????
				strTemp += RES_STRING(CL_LANGUAGE_MATCH_710);
			}

			sRow++;
			sCom = 0;
			dwColor = 0xFF000000;
			m_sNumInfo++;
			m_MemoInfo[m_sNumInfo].strDesp = "";
			m_MemoInfo[m_sNumInfo].dwColor = dwColor;
			m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_DESP;
			m_MemoInfo[m_sNumInfo].sxPos = sCom;
			m_MemoInfo[m_sNumInfo].syPos = sRow;
			char szNeed[256] = {0};
			strncpy(szNeed, strTemp.c_str(), 255);
			ParseScript(szNeed, m_sNumInfo, sRow, sCom, 0, sMaxCom, 0xFF000000, m_MemoInfo, MEMO_MAX_LINE);
			if (m_MemoInfo[m_sNumInfo].strDesp.length() <= 0) {
				m_sNumInfo--;
			}
		}

		// ??'??h??
		m_sNumInfo++;
		// sRow++;
		m_MemoInfo[m_sNumInfo].strDesp = "";
		m_MemoInfo[m_sNumInfo].dwColor = dwColor;
		m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_INTERVAL;
		m_MemoInfo[m_sNumInfo].sxPos = sCom;
		m_MemoInfo[m_sNumInfo].syPos = sRow;
	}

	if (m_PageInfo.byPrizeNum > 0) {
		// ??????
		sRow++;
		sCom = 0;
		dwColor = 0xFF000000;
		m_sNumInfo++;
		m_MemoInfo[m_sNumInfo].strDesp = RES_STRING(CL_LANGUAGE_MATCH_711);
		if (m_PageInfo.byPrizeSelType == mission::PRZIE_SELALL) {
			// m_MemoInfo[m_sNumInfo].strDesp += "(???)";
		} else {
			m_MemoInfo[m_sNumInfo].strDesp += RES_STRING(CL_LANGUAGE_MATCH_712);
		}
		m_MemoInfo[m_sNumInfo].dwColor = dwColor;
		m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_TITLE;
		m_MemoInfo[m_sNumInfo].sxPos = sCom;
		m_MemoInfo[m_sNumInfo].syPos = sRow;

		sRow++;
		sCom = 1;
		char szItem[64];
		for (int n = 0; n < m_PageInfo.byPrizeNum; n++) {
			strcpy(szItem, "./texture/ui/system/error.tga");
			if (m_PageInfo.MisPrize[n].byType == mission::MIS_PRIZE_ITEM) {
				m_sNumInfo++;
				m_MemoInfo[m_sNumInfo].sxPos = sCom;
				m_MemoInfo[m_sNumInfo].syPos = sRow;
				m_MemoInfo[m_sNumInfo].byData = mission::MIS_PRIZE_ITEM;
				m_MemoInfo[m_sNumInfo].sData = m_PageInfo.MisPrize[n].wParam1;
				CItemRecord* pItem = GetItemRecordInfo(m_PageInfo.MisPrize[n].wParam1);
				if (pItem) {
					strcpy(szItem, pItem->GetIconFile());
				}

				m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_ICON;
				m_MemoInfo[m_sNumInfo].pIcon = new CGraph(szItem, 32, 32, 0, 0, 1);

				// ??'??h??
				m_sNumInfo++;
				// sRow++;
				m_MemoInfo[m_sNumInfo].strDesp = "";
				m_MemoInfo[m_sNumInfo].dwColor = dwColor;
				m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_INTERVAL;
				m_MemoInfo[m_sNumInfo].sxPos = sCom;
				m_MemoInfo[m_sNumInfo].syPos = sRow;

				sCom += 8;

				m_sNumInfo++;
				if (pItem) {
					strcpy(szItem, pItem->szName);
				} else {
					sprintf(szItem, RES_STRING(CL_LANGUAGE_MATCH_713), m_PageInfo.MisPrize[n].wParam1);
				}

				sprintf(szNumber, RES_STRING(CL_LANGUAGE_MATCH_714), m_PageInfo.MisPrize[n].wParam2, szItem);
				m_MemoInfo[m_sNumInfo].strDesp = "";
				m_MemoInfo[m_sNumInfo].dwColor = 0xFF808080;
				m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_DESP;
				m_MemoInfo[m_sNumInfo].sxPos = sCom;
				m_MemoInfo[m_sNumInfo].syPos = sRow;
				ParseScript(szNumber, m_sNumInfo, sRow, sCom, 6, sMaxCom, 0xFF808080, m_MemoInfo, MEMO_MAX_LINE);
				sRow += 2;
				sCom = 1;
			} else if (m_PageInfo.MisPrize[n].byType == mission::MIS_PRIZE_MONEY) {
				m_sNumInfo++;
				m_MemoInfo[m_sNumInfo].sxPos = sCom;
				m_MemoInfo[m_sNumInfo].syPos = sRow;
				m_MemoInfo[m_sNumInfo].byData = mission::MIS_PRIZE_ITEM;
				m_MemoInfo[m_sNumInfo].sData = m_PageInfo.MisPrize[n].wParam1;
				CItemRecord* pItem = GetItemRecordInfo(194);
				if (pItem) {
					strcpy(szItem, pItem->GetIconFile());
				}

				m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_ICON;
				m_MemoInfo[m_sNumInfo].pIcon = new CGraph(szItem, 32, 32, 0, 0, 1);

				// ??'??h??
				m_sNumInfo++;
				// sRow++;
				m_MemoInfo[m_sNumInfo].strDesp = "";
				m_MemoInfo[m_sNumInfo].dwColor = dwColor;
				m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_INTERVAL;
				m_MemoInfo[m_sNumInfo].sxPos = sCom;
				m_MemoInfo[m_sNumInfo].syPos = sRow;

				sCom += 8;

				m_sNumInfo++;
				sprintf(szNumber, RES_STRING(CL_LANGUAGE_MATCH_715), m_PageInfo.MisPrize[n].wParam1);
				m_MemoInfo[m_sNumInfo].strDesp = "";
				m_MemoInfo[m_sNumInfo].dwColor = 0xFF808080;
				m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_DESP;
				m_MemoInfo[m_sNumInfo].sxPos = sCom;
				m_MemoInfo[m_sNumInfo].syPos = sRow;
				ParseScript(szNumber, m_sNumInfo, sRow, sCom, 6, sMaxCom, 0xFF808080, m_MemoInfo, MEMO_MAX_LINE);
				sRow += 2;
				sCom = 1;
			} else if (m_PageInfo.MisPrize[n].byType == mission::MIS_PRIZE_FAME) {
				m_sNumInfo++;
				m_MemoInfo[m_sNumInfo].sxPos = sCom;
				m_MemoInfo[m_sNumInfo].syPos = sRow;
				m_MemoInfo[m_sNumInfo].byData = mission::MIS_PRIZE_ITEM;
				m_MemoInfo[m_sNumInfo].sData = m_PageInfo.MisPrize[n].wParam1;
				CItemRecord* pItem = GetItemRecordInfo(195);
				if (pItem) {
					strcpy(szItem, pItem->GetIconFile());
				}

				m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_ICON;
				m_MemoInfo[m_sNumInfo].pIcon = new CGraph(szItem, 32, 32, 0, 0, 1);

				// ??'??h??
				m_sNumInfo++;
				// sRow++;
				m_MemoInfo[m_sNumInfo].strDesp = "";
				m_MemoInfo[m_sNumInfo].dwColor = dwColor;
				m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_INTERVAL;
				m_MemoInfo[m_sNumInfo].sxPos = sCom;
				m_MemoInfo[m_sNumInfo].syPos = sRow;

				sCom += 8;

				m_sNumInfo++;
				sprintf(szNumber, RES_STRING(CL_LANGUAGE_MATCH_716), m_PageInfo.MisPrize[n].wParam1);
				m_MemoInfo[m_sNumInfo].strDesp = "";
				m_MemoInfo[m_sNumInfo].dwColor = 0xFF808080;
				m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_DESP;
				m_MemoInfo[m_sNumInfo].sxPos = sCom;
				m_MemoInfo[m_sNumInfo].syPos = sRow;
				ParseScript(szNumber, m_sNumInfo, sRow, sCom, 6, sMaxCom, 0xFF808080, m_MemoInfo, MEMO_MAX_LINE);
				sRow += 2;
				sCom = 1;
			} else if (m_PageInfo.MisPrize[n].byType == mission::MIS_PRIZE_CESS) {
				m_sNumInfo++;
				m_MemoInfo[m_sNumInfo].sxPos = sCom;
				m_MemoInfo[m_sNumInfo].syPos = sRow;
				m_MemoInfo[m_sNumInfo].byData = mission::MIS_PRIZE_ITEM;
				m_MemoInfo[m_sNumInfo].sData = m_PageInfo.MisPrize[n].wParam1;
				CItemRecord* pItem = GetItemRecordInfo(195);
				if (pItem) {
					strcpy(szItem, pItem->GetIconFile());
				}

				m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_ICON;
				m_MemoInfo[m_sNumInfo].pIcon = new CGraph(szItem, 32, 32, 0, 0, 1);

				// ??'??h??
				m_sNumInfo++;
				// sRow++;
				m_MemoInfo[m_sNumInfo].strDesp = "";
				m_MemoInfo[m_sNumInfo].dwColor = dwColor;
				m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_INTERVAL;
				m_MemoInfo[m_sNumInfo].sxPos = sCom;
				m_MemoInfo[m_sNumInfo].syPos = sRow;

				sCom += 8;

				m_sNumInfo++;
				sprintf(szNumber, RES_STRING(CL_LANGUAGE_MATCH_717), m_PageInfo.MisPrize[n].wParam1);
				m_MemoInfo[m_sNumInfo].strDesp = "";
				m_MemoInfo[m_sNumInfo].dwColor = 0xFF808080;
				m_MemoInfo[m_sNumInfo].byType = MEMO_LINE_DESP;
				m_MemoInfo[m_sNumInfo].sxPos = sCom;
				m_MemoInfo[m_sNumInfo].syPos = sRow;
				ParseScript(szNumber, m_sNumInfo, sRow, sCom, 6, sMaxCom, 0xFF808080, m_MemoInfo, MEMO_MAX_LINE);
				sRow += 2;
				sCom = 1;
			} else {
				g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_718), m_PageInfo.MisPrize[n].byType, m_PageInfo.MisPrize[n].wParam1, m_PageInfo.MisPrize[n].wParam2);
			}
		}
	}
	m_sNumInfo++;
}

bool CMemoEx::TryGetFirstClickableNavPayload(std::string& outPayload) const {
	for (USHORT i = 0; i < m_sNumInfo; ++i) {
		const MEMO_INFO& info = m_MemoInfo[i];
		if (info.byType == MEMO_LINE_DESP && info.bClickable && !info.strLinkPayload.empty()) {
			outPayload = info.strLinkPayload;
			return true;
		}
	}
	return false;
}

bool CMemoEx::TryGetMissionAutoRoutePayload(std::string& outPayload) const {
	outPayload.clear();

	CGameScene* pScene = dynamic_cast<CGameScene*>(CGameApp::GetCurScene());
	const bool hasPlayerPos = (pScene && pScene->GetMainCha());
	const int cx = hasPlayerPos ? static_cast<int>(pScene->GetMainCha()->GetPos().x) : 0;
	const int cy = hasPlayerPos ? static_cast<int>(pScene->GetMainCha()->GetPos().y) : 0;
	const std::string currentMap = (pScene && pScene->GetCurMapInfo()) ? std::string(pScene->GetCurMapInfo()->szName) : "";
	const std::string currentTerrain = pScene ? std::string(pScene->GetTerrainName()) : "";

	int bestScore = std::numeric_limits<int>::min();
	std::string bestPayload;

	for (int i = 0; i < m_PageInfo.byNeedNum; ++i) {
		const NET_MISNEED& need = m_PageInfo.MisNeed[i];
		if (IsCountNeedComplete(need)) {
			continue;
		}

		std::string payload;
		std::string mapName;
		int tx = 0;
		int ty = 0;
		bool hasCoords = false;
		if (!TryBuildNeedPayload(need, payload, mapName, tx, ty, hasCoords)) {
			continue;
		}

		const int score = ComputeNeedScore(need, i, mapName, hasCoords, tx, ty, hasPlayerPos, cx, cy, currentMap, currentTerrain);
		if (score > bestScore) {
			bestScore = score;
			bestPayload = payload;
		}
	}

	if (!bestPayload.empty()) {
		outPayload = bestPayload;
		return true;
	}

	for (int i = 0; i < m_PageInfo.byNeedNum; ++i) {
		std::string payload;
		std::string mapName;
		int tx = 0;
		int ty = 0;
		bool hasCoords = false;
		if (TryBuildNeedPayload(m_PageInfo.MisNeed[i], payload, mapName, tx, ty, hasCoords)) {
			outPayload = payload;
			return true;
		}
	}

	return false;
}

BOOL CMemoEx::ParseScript(char* pszTemp, USHORT& sNumLine, USHORT& sRow, USHORT& sCom, USHORT sStartCom, USHORT sMaxCom, DWORD dwDefColor, MEMO_INFO* pInfo, USHORT sMaxInfo) {
	if (pszTemp == nullptr || pInfo == nullptr) {
		return FALSE;
	}

	auto InitMemoSegment = [&](USHORT index, DWORD color, USHORT com, USHORT row, bool clickable, const std::string& payload) {
		if (index >= sMaxInfo) {
			return;
		}
		pInfo[index].strDesp = "";
		pInfo[index].strLinkPayload = payload;
		pInfo[index].dwColor = color;
		pInfo[index].byType = MEMO_LINE_DESP;
		pInfo[index].bClickable = clickable;
		pInfo[index].sxPos = com;
		pInfo[index].syPos = row;
	};

	auto TryParseNavTag = [&](char* source, std::string& outPayload, std::string& outLabel, int& outConsumed) -> bool {
		if (!source || source[0] != '<' || _strnicmp(source, "<nav:", 5) != 0) {
			return false;
		}

		char* end = strchr(source, '>');
		if (!end) {
			return false;
		}

		std::string tagContent(source + 1, end);
		std::string payload = tagContent;
		std::string label;

		size_t pipePos = tagContent.find('|');
		if (pipePos != std::string::npos) {
			payload = tagContent.substr(0, pipePos);
			label = tagContent.substr(pipePos + 1);
		}

		if (label.empty()) {
			label = payload;
			if (_strnicmp(label.c_str(), "nav:", 4) == 0) {
				label = label.substr(4);
			}

			if (label.empty()) {
				label = "Route";
			}
		}

		outPayload = payload;
		outLabel = label;
		outConsumed = static_cast<int>((end - source) + 1);
		return true;
	};

	auto TryParseLegacyJTag = [&](char* source, std::string& outPayload, std::string& outLabel, int& outConsumed) -> bool {
		if (!source || source[0] != '<' || source[1] != 'j') {
			return false;
		}

		char* end = strchr(source, '>');
		if (!end) {
			return false;
		}

		std::string rawPayload(source + 2, end);
		rawPayload = TrimWhitespaceCopy(rawPayload);
		if (rawPayload.empty()) {
			return false;
		}

		int tx = 0;
		int ty = 0;
		if (TryParseLegacyCoordPayload(rawPayload, tx, ty)) {
			outPayload = "coord:" + std::to_string(tx) + ":" + std::to_string(ty);
			outLabel = "(" + std::to_string(tx) + ", " + std::to_string(ty) + ")";
		} else {
			outPayload = rawPayload;
			outLabel = rawPayload;
		}

		outConsumed = static_cast<int>((end - source) + 1);
		return true;
	};

	DWORD dwColor = dwDefColor;
	char szData[3];
	memset(szData, 0, 3);
	
	// Calculate pixel-based max width (sMaxCom was character count, convert to approximate pixel width)
	int nCharWidth = CGuiFont::s_Font.GetWidth("a");
	int nMaxPixelWidth = sMaxCom * nCharWidth;
	int nCurrentPixelWidth = sCom * nCharWidth; // Current position in pixels

	while (pszTemp[0]) {
		szData[0] = 0;
		szData[1] = 0;

		std::string navPayload;
		std::string navLabel;
		int navConsumed = 0;
		if (TryParseNavTag(pszTemp, navPayload, navLabel, navConsumed) || TryParseLegacyJTag(pszTemp, navPayload, navLabel, navConsumed)) {
			dwColor = 0xFF0000FF; // blue
			sNumLine++;
			InitMemoSegment(sNumLine, dwColor, sCom, sRow, true, navPayload);

			for (size_t i = 0; i < navLabel.size(); ++i) {
				char ch = navLabel[i];

				BOOL bFlag1 = 0x81 <= (BYTE)ch && (BYTE)ch <= 0xFE;
				BOOL bFlag2 = (i + 1 < navLabel.size()) && (((0x40 <= (BYTE)navLabel[i + 1] && (BYTE)navLabel[i + 1] <= 0x7E) || (0x7E <= (BYTE)navLabel[i + 1] && (BYTE)navLabel[i + 1] <= 0xFE)));

				if (bFlag1 && bFlag2) {
					szData[0] = navLabel[i];
					szData[1] = navLabel[i + 1];
					int nCharPixelWidth = CGuiFont::s_Font.GetWidth(szData);

					if (nCurrentPixelWidth + nCharPixelWidth > nMaxPixelWidth) {
						sRow++;
						sCom = sStartCom;
						nCurrentPixelWidth = sStartCom * nCharWidth;
						sNumLine++;
						InitMemoSegment(sNumLine, dwColor, sCom, sRow, true, navPayload);
					}

					pInfo[sNumLine].strDesp += szData;
					nCurrentPixelWidth += nCharPixelWidth;
					sCom += 2;
					i++;
				} else {
					szData[0] = ch;
					int nCharPixelWidth = CGuiFont::s_Font.GetWidth(szData);

					if (nCurrentPixelWidth + nCharPixelWidth > nMaxPixelWidth) {
						sRow++;
						sCom = sStartCom;
						nCurrentPixelWidth = sStartCom * nCharWidth;
						sNumLine++;
						InitMemoSegment(sNumLine, dwColor, sCom, sRow, true, navPayload);
					}

					pInfo[sNumLine].strDesp += szData;
					nCurrentPixelWidth += nCharPixelWidth;
					sCom++;
				}
			}

			pszTemp += navConsumed;
			dwColor = dwDefColor;
			sNumLine++;
			InitMemoSegment(sNumLine, dwColor, sCom, sRow, false, "");
			continue;
		}

		// Handle markup tags
		if (pszTemp[0] == '<') {
			pszTemp++;
			if (pszTemp[0] == 'p') {
				dwColor = 0xFFFF00FF; // Purple
				if (pszTemp[1] == '>') {
					pszTemp++;
					continue;
				}
			} else if (pszTemp[0] == 'j') {
				dwColor = 0xFF0000FF; // Blue (j/nav link)
				if (pszTemp[1] == '>') {
					pszTemp += 2;
					continue;
				}
			} else if (pszTemp[0] == 'r') {
				dwColor = 0xFFFF0000;
				if (pszTemp[1] == '>') {
					pszTemp++;
					continue;
				}
			} else if (pszTemp[0] == 'y') {
				dwColor = 0xFFFF8A00;
				if (pszTemp[1] == '>') {
					pszTemp++;
					continue;
				}
			} else if (pszTemp[0] == 'b') {
				dwColor = 0xFF0000FF;
				if (pszTemp[1] == '>') {
					pszTemp++;
					continue;
				}
			} else if (pszTemp[0] == 'g') {
				dwColor = 0xFF808080;
				if (pszTemp[1] == '>') {
					pszTemp++;
					continue;
				}
			} else if (pszTemp[0] == 'n') {
				sRow++;
				sCom = sStartCom;
				nCurrentPixelWidth = sStartCom * nCharWidth;
				if (pszTemp[1] == '>') {
					pszTemp++;
					continue;
				}
			} else if (pszTemp[0] == 't') {
				int nTabWidth = 4 * nCharWidth;
				nCurrentPixelWidth += nTabWidth;
				sCom += 4;
				if (nCurrentPixelWidth >= nMaxPixelWidth) {
					sRow++;
					sCom = sStartCom;
					nCurrentPixelWidth = sStartCom * nCharWidth;
				}
				if (pszTemp[1] == '>') {
					pszTemp++;
					continue;
				}
			} else {
				continue;
			}

			pszTemp++;
			sNumLine++;
			pInfo[sNumLine].strDesp = "";
			pInfo[sNumLine].strLinkPayload.clear();
			pInfo[sNumLine].dwColor = dwColor;
			pInfo[sNumLine].byType = MEMO_LINE_DESP;
			pInfo[sNumLine].bClickable = false;
			pInfo[sNumLine].sxPos = sCom;
			pInfo[sNumLine].syPos = sRow;
		} else if (pszTemp[0] == '>') {
			dwColor = dwDefColor;
			pszTemp++;

			sNumLine++;
			pInfo[sNumLine].strDesp = "";
			pInfo[sNumLine].strLinkPayload.clear();
			pInfo[sNumLine].dwColor = dwColor;
			pInfo[sNumLine].byType = MEMO_LINE_DESP;
			pInfo[sNumLine].bClickable = false;
			pInfo[sNumLine].sxPos = sCom;
			pInfo[sNumLine].syPos = sRow;
			continue;
		}

		// Check if this is a GBK character (Chinese)
		BOOL bFlag1 = 0x81 <= (BYTE)pszTemp[0] && (BYTE)pszTemp[0] <= 0xFE;
		BOOL bFlag2 = (0x40 <= (BYTE)pszTemp[1] && (BYTE)pszTemp[1] <= 0x7E) || (0x7E <= (BYTE)pszTemp[1] && (BYTE)pszTemp[1] <= 0xFE);
		
		if (bFlag1 && bFlag2) {
			// GBK character - can break anywhere for Chinese
			szData[0] = pszTemp[0];
			szData[1] = pszTemp[1];
			int nCharPixelWidth = CGuiFont::s_Font.GetWidth(szData);
			
			if (nCurrentPixelWidth + nCharPixelWidth <= nMaxPixelWidth) {
				pInfo[sNumLine].strDesp += szData;
				nCurrentPixelWidth += nCharPixelWidth;
				sCom += 2;
				pszTemp += 2;
			} else {
				// Wrap to next line
				sRow++;
				sCom = sStartCom;
				nCurrentPixelWidth = sStartCom * nCharWidth;

				sNumLine++;
				pInfo[sNumLine].strDesp = "";
				pInfo[sNumLine].strLinkPayload.clear();
				pInfo[sNumLine].dwColor = dwColor;
				pInfo[sNumLine].byType = MEMO_LINE_DESP;
				pInfo[sNumLine].bClickable = false;
				pInfo[sNumLine].sxPos = sCom;
				pInfo[sNumLine].syPos = sRow;
			}
		} else {
			// ASCII character - implement word wrapping
			// If this is start of a word (not a space), look ahead to find word length
			if (pszTemp[0] != ' ' && pszTemp[0] != '\t' && pszTemp[0] != '\n' && pszTemp[0] != '\r') {
				// Find the end of this word
				const char* pWordEnd = pszTemp;
				std::string word = "";
				while (*pWordEnd && *pWordEnd != ' ' && *pWordEnd != '\t' && *pWordEnd != '\n' && *pWordEnd != '\r' 
				       && *pWordEnd != '<' && *pWordEnd != '>') {
					// Check for GBK in word - stop at GBK boundary
					BOOL bGBK1 = 0x81 <= (BYTE)pWordEnd[0] && (BYTE)pWordEnd[0] <= 0xFE;
					BOOL bGBK2 = (0x40 <= (BYTE)pWordEnd[1] && (BYTE)pWordEnd[1] <= 0x7E) || (0x7E <= (BYTE)pWordEnd[1] && (BYTE)pWordEnd[1] <= 0xFE);
					if (bGBK1 && bGBK2) {
						break; // Stop at GBK character
					}
					word += *pWordEnd;
					pWordEnd++;
				}
				
				int nWordPixelWidth = CGuiFont::s_Font.GetWidth(word.c_str());
				
				// Check if word fits on current line
				if (nCurrentPixelWidth + nWordPixelWidth <= nMaxPixelWidth) {
					// Word fits - add it character by character (to maintain segment structure)
					szData[0] = pszTemp[0];
					int nCharPixelWidth = CGuiFont::s_Font.GetWidth(szData);
					pInfo[sNumLine].strDesp += szData;
					nCurrentPixelWidth += nCharPixelWidth;
					sCom++;
					pszTemp++;
				} else if (nWordPixelWidth <= nMaxPixelWidth - (sStartCom * nCharWidth)) {
					// Word doesn't fit on current line but will fit on a new line - wrap first
					sRow++;
					sCom = sStartCom;
					nCurrentPixelWidth = sStartCom * nCharWidth;

					sNumLine++;
					pInfo[sNumLine].strDesp = "";
					pInfo[sNumLine].strLinkPayload.clear();
					pInfo[sNumLine].dwColor = dwColor;
					pInfo[sNumLine].byType = MEMO_LINE_DESP;
					pInfo[sNumLine].bClickable = false;
					pInfo[sNumLine].sxPos = sCom;
					pInfo[sNumLine].syPos = sRow;
					// Don't advance pszTemp - will process this character on next iteration
				} else {
					// Word is too long to fit on any line - break it character by character
					szData[0] = pszTemp[0];
					int nCharPixelWidth = CGuiFont::s_Font.GetWidth(szData);
					
					if (nCurrentPixelWidth + nCharPixelWidth <= nMaxPixelWidth) {
						pInfo[sNumLine].strDesp += szData;
						nCurrentPixelWidth += nCharPixelWidth;
						sCom++;
						pszTemp++;
					} else {
						sRow++;
						sCom = sStartCom;
						nCurrentPixelWidth = sStartCom * nCharWidth;

						sNumLine++;
						pInfo[sNumLine].strDesp = "";
						pInfo[sNumLine].strLinkPayload.clear();
						pInfo[sNumLine].dwColor = dwColor;
						pInfo[sNumLine].byType = MEMO_LINE_DESP;
						pInfo[sNumLine].bClickable = false;
						pInfo[sNumLine].sxPos = sCom;
						pInfo[sNumLine].syPos = sRow;
					}
				}
			} else {
				// Space or whitespace character
				szData[0] = pszTemp[0];
				int nCharPixelWidth = CGuiFont::s_Font.GetWidth(szData);
				
				if (nCurrentPixelWidth + nCharPixelWidth <= nMaxPixelWidth) {
					pInfo[sNumLine].strDesp += szData;
					nCurrentPixelWidth += nCharPixelWidth;
					sCom++;
					pszTemp++;
				} else {
					// Wrap to next line (skip the space at line break)
					sRow++;
					sCom = sStartCom;
					nCurrentPixelWidth = sStartCom * nCharWidth;

					sNumLine++;
					pInfo[sNumLine].strDesp = "";
					pInfo[sNumLine].strLinkPayload.clear();
					pInfo[sNumLine].dwColor = dwColor;
					pInfo[sNumLine].byType = MEMO_LINE_DESP;
					pInfo[sNumLine].bClickable = false;
					pInfo[sNumLine].sxPos = sCom;
					pInfo[sNumLine].syPos = sRow;
					pszTemp++; // Skip the space
				}
			}
		}
	}
	return TRUE;
}

void CMemoEx::Refresh() {
	CCompent::Refresh();
	//_pScroll->SetRange( (float)0.0f, (float )_nRowNum-4 );  // _nRowNum???????
	_pScroll->Refresh();
	_OnScrollChange();
}

BOOL CMemoEx::IsSelPrize() {
	return m_PageInfo.byPrizeNum > 0 && m_PageInfo.byPrizeSelType == mission::PRIZE_SELONE;
}

bool CMemoEx::MouseRun(int x, int y, DWORD key) {
	if (!IsNormal())
		return false;

	if (InRect(x, y)) {
		if (_pScroll->MouseRun(x, y, key)) {
			return true;
		}

		// Add by lark.li 20080721 begin
		m_SelMem = SelMemInfo(x, y, key);
		// End

		if (m_SelMem > 0 && evtClickItem) {
			const bool bIsClickable = m_MemoInfo[m_SelMem].bClickable && !m_MemoInfo[m_SelMem].strLinkPayload.empty();
			if (bIsClickable) {
				GUI::CGuiData::SetCursor(GUI::CCursor::stActive);
			} else {
				GUI::CGuiData::SetCursor(GUI::CCursor::stNormal);
			}
		} else {
			GUI::CGuiData::SetCursor(GUI::CCursor::stNormal);
		}

		if (m_SelMem > 0 && (key & Mouse_LDown)) {
			if (evtClickItem) {
				const bool bIsClickable = m_MemoInfo[m_SelMem].bClickable && !m_MemoInfo[m_SelMem].strLinkPayload.empty();
				if (!bIsClickable) {
					return true;
				}

				evtClickItem(m_MemoInfo[m_SelMem].strLinkPayload);
			}
		}

		if (m_bIsSelect) {
			// ?????????????????
			if (IsSelPrize()) {
				if (SelPrizeItem(x, y, key)) {
					SetCursor(CCursor::stButtonClick);
				}
			}
		}
		return true;
	}

	return _IsMouseIn;
}

void CMemoEx::_OnScrollChange() {
	_nFirst = (int)_pScroll->GetStep().GetPosition();
	_nLast = _nFirst + _nPageShowNum;
	if (_nLast > MEMO_MAX_LINE)
		_nLast = MEMO_MAX_LINE;

	return;
}

bool CMemoEx::MouseScroll(int nScroll) {
	if (!IsNormal())
		return false;

	if (_IsMouseIn)
		_pScroll->MouseScroll(nScroll);
	return _IsMouseIn;
}

void CMemoEx::SetMargin(int left, int top, int right, int bottom) {
	_nLeftMargin = left;
	_nTopMargin = top;
	_nRightMargin = right;
	_nBottomMargin = bottom;
}

void CMemoEx::_SetScrollRange() {
	_pScroll->Refresh();
}

// CRichMemo implement
CRichMemo::CRichMemo(CForm& frmOwn)
	: CCompent(frmOwn), m_pScroll(nullptr) {
	m_ShowRect.left = 0;
	m_ShowRect.right = 0;
	m_ShowRect.top = 0;
	m_ShowRect.bottom = 0;

	m_pScroll = new CScroll(frmOwn);
	m_pScroll->SetParent(this);
	m_pScroll->SetIsShow(true);
	m_pScroll->evtChange = OnScrollChange;
	m_pScroll->SetAutoHide(false);

	m_pImageList = new CFaceImage;
	m_sMaxLine = 40;
	m_dwTitleColor = 0xFF0000FF;
	m_byTitleFont = 0;
	m_byDist = 2;
	m_bAutoScroll = TRUE;
}

CRichMemo::CRichMemo(const CRichMemo& rhs)
	: CCompent(rhs) {
	m_pScroll = new CScroll(*rhs.m_pScroll);
	m_pScroll->SetParent(this);
	m_pScroll->SetIsShow(true);
	m_pScroll->evtChange = OnScrollChange;

	memcpy(&m_ShowRect, &rhs.m_ShowRect, sizeof(RECT));
	m_sMaxLine = rhs.m_sMaxLine;
	m_dwTitleColor = rhs.m_dwTitleColor;
	m_byTitleFont = rhs.m_byTitleFont;
	m_byDist = rhs.m_byDist;
	m_pImageList = new CFaceImage;
	m_bAutoScroll = rhs.m_bAutoScroll;
}

CRichMemo& CRichMemo::operator=(const CRichMemo& rhs) {
	CCompent::operator=(rhs);
	*m_pScroll = *rhs.m_pScroll;
	m_pScroll->SetParent(this);
	m_pScroll->SetIsShow(true);
	m_pScroll->evtChange = OnScrollChange;

	memcpy(&m_ShowRect, &rhs.m_ShowRect, sizeof(RECT));
	m_sMaxLine = rhs.m_sMaxLine;
	m_dwTitleColor = rhs.m_dwTitleColor;
	m_byTitleFont = rhs.m_byTitleFont;
	m_byDist = rhs.m_byDist;
	m_pImageList = new CFaceImage;
	m_bAutoScroll = rhs.m_bAutoScroll;

	return *this;
}

CRichMemo::~CRichMemo(void) {
	MEMO_LINEINFO_LIST::iterator linePos;
	for (linePos = m_LineList.begin(); linePos != m_LineList.end(); linePos++) {
		SAFE_DELETE((*linePos));
	}
	m_LineList.clear();
	SAFE_DELETE(m_pImageList);
}

void CRichMemo::Render() {
	int nStart = (int)m_pScroll->GetStep().GetPosition() + m_ShowRect.top;
	int nEnd = nStart + _nHeight - m_ShowRect.bottom;

	int nPosY = GetY();
	int nPosX = GetX();
	DWORD dwyPos = 0;

	// ?�??????
	GetRender().SetClipRect(nPosX + m_ShowRect.left, nPosY + m_ShowRect.top,
							GetWidth() - m_ShowRect.right + 2, GetHeight() - m_ShowRect.bottom);

	nPosY += m_ShowRect.top;
	MEMO_LINEINFO_LIST::iterator pos;
	for (const auto& pInfo : m_LineList) {
		dwyPos += pInfo->sHeight + m_byDist;
		// ??????????????
		MEMO_COLINFO_ARRAY::iterator colpos;
		for (const auto& colpos : pInfo->ColInfoArray) {
			int nxPos = colpos.sxPos + nPosX;
			switch (colpos.byType) {
			case COL_TEXT: {
				int nyPos = dwyPos - pInfo->sFontHeight - nStart;
				if (nyPos > _nHeight || nyPos + pInfo->sHeight < 0) {
					continue;
				}
				nyPos += nPosY;
				CGuiFont::s_Font.Render(pInfo->byFontType, colpos.strDesp.c_str(), nxPos, nyPos, colpos.dwColor);
			} break;
			case COL_ICON: {
				int nyPos = dwyPos - colpos.pIcon->GetHeight() - nStart;
				if (nyPos > _nHeight || nyPos + pInfo->sHeight < 0) {
					continue;
				}
				nyPos += nPosY;
				colpos.pIcon->Render(nxPos, nyPos);
			} break;
			default:
				break;
			}
		}
	}

	GetRender().Reset();
	if (m_pScroll->GetIsShow())
		m_pScroll->Render();
}

void CRichMemo::AutoScroll() {
	if (m_bAutoScroll) {
		DWORD dwyPos = 0;
		MEMO_LINEINFO_LIST::iterator pos;
		for (pos = m_LineList.begin(); pos != m_LineList.end(); pos++) {
			MEMO_LINE_INFO* pInfo = *pos;
			dwyPos += pInfo->sHeight + m_byDist;
		}

		int nHeight = _nHeight - (m_ShowRect.top + m_ShowRect.bottom);
		float fData = (dwyPos > (DWORD)nHeight) ? float(dwyPos - nHeight) : 0;

		m_pScroll->SetRange(0, float(fData));
		m_pScroll->GetStep().SetPosition(fData);
		m_pScroll->Refresh();
	}
}

void CRichMemo::Refresh() {
	CCompent::Refresh();
	m_pScroll->Refresh();
	OnScrollChange();
}

bool CRichMemo::MouseRun(int x, int y, DWORD key) {
	if (!IsNormal()) {
		return false;
	}

	if (InRect(x, y)) {
		return m_pScroll->MouseRun(x, y, key);
	}

	return _IsMouseIn;
}

bool CRichMemo::MouseScroll(int nScroll) {
	if (!IsNormal()) {
		return false;
	}

	if (_IsMouseIn) {
		m_pScroll->MouseScroll(nScroll);
	}

	return _IsMouseIn;
}

void CRichMemo::OnScrollChange() {
}

void CRichMemo::Init() {
	Clear();
}

void CRichMemo::Clear() {
	m_pScroll->Reset();
	m_pScroll->SetSize(m_pScroll->GetWidth(), GetHeight());
	m_pScroll->SetPos(GetWidth() - m_pScroll->GetWidth() + 5, 0);
	m_pScroll->Init();
	OnScrollChange();

	MEMO_LINEINFO_LIST::iterator linePos;
	for (linePos = m_LineList.begin(); linePos != m_LineList.end(); linePos++) {
		SAFE_DELETE((*linePos));
	}
	m_LineList.clear();

	// ???????
	AutoScroll();
}

void CRichMemo::AddText(const char szTitle[], const char szText[], BYTE byColorType, BYTE byFontType) {
	// ??????????????
	if (m_LineList.size() >= m_sMaxLine) {
		// ?????h????????????
		MEMO_LINEINFO_LIST::iterator pos = m_LineList.begin();
		if (pos != m_LineList.end()) {
			SAFE_DELETE((*pos));
			m_LineList.erase(pos);
		}
	}

	DWORD dwColor = GetColor(byColorType);
	ParseScript(szTitle, m_dwTitleColor, m_byTitleFont, (USHORT)m_ShowRect.left,
				(USHORT)(_nWidth - m_ShowRect.right - m_ShowRect.left));
	ParseScript(szText, dwColor, byFontType, (USHORT)m_ShowRect.left + 2,
				(USHORT)(_nWidth - m_ShowRect.right - m_ShowRect.left));

	// ???????
	AutoScroll();
}

DWORD CRichMemo::GetColor(BYTE byType) {
	switch (byType) {
	case TEXT_COLOR_WIGHT: // ???
		return 0xFFFFFFFF;
		break;
	case TEXT_COLOR_BLACK: // ???
		return 0xFF000000;
		break;
	case TEXT_COLOR_RED: // ???
		return 0xFFFF0000;
		break;
	case TEXT_COLOR_GREEN: // ???
		return 0xFF00FF00;
		break;
	case TEXT_COLOR_BLUE: // ???
		return 0xFF0000FF;
		break;
	case TEXT_COLOR_GRAY: // ???
		return 0xFF808080;
		break;
	case TEXT_COLOR_PURPLE: // ???
		return 0xFFFF00FF;
		break;
	default:
		return 0xFF233413; // ????
		break;
	}
}

CGraph* CRichMemo::GetImage(const char szImage[]) {
	static CGraph* g_pErrorImage = nullptr;
	if (!g_pErrorImage) {
		g_pErrorImage = new CGraph();
		g_pErrorImage->GetImage()->LoadImage("./texture/ui/system/error.tga");
	}
	return g_pErrorImage;
}

void CRichMemo::ParseScript(const char szInfo[], DWORD dwDefColor, BYTE byFont, USHORT sStartCom, USHORT sMaxCom) {
	char szData[3];
	memset(szData, 0, 3);

	// ?????????
	int nWidth, nHeight;
	CGuiFont::s_Font.GetSize(byFont, "a", nWidth, nHeight);

	string strDesp = "";
	USHORT sCurHeight = (USHORT)nHeight;
	USHORT sCom = sStartCom;
	USHORT sxPos = sCom;
	DWORD dwColor = dwDefColor;
	const char* pszTemp = szInfo;

	MEMO_LINE_INFO* pLineInfo = new MEMO_LINE_INFO;
	if (pLineInfo == nullptr)
		return;

	while (pszTemp[0]) {
		szData[0] = 0;
		szData[1] = 0;

		if (pszTemp[0] == '<') {
			if (strDesp.length() > 0) {
				// ????????
				MEMO_COLINFO Info;
				Info.strDesp = strDesp;
				Info.dwColor = dwColor;
				Info.sxPos = sxPos;
				Info.byType = COL_TEXT;
				Info.pIcon = nullptr;
				pLineInfo->ColInfoArray.push_back(Info);

				sxPos = sCom;
				strDesp = "";
			}

			pszTemp++;
			if (pszTemp[0] == 'p') {
				dwColor = 0xFFFF00FF; // ???
				if (pszTemp[1] == '>') {
					pszTemp += 2;
					continue;
				}
			} else if (pszTemp[0] == 'j') {
				dwColor = 0xFFFE0000;
				if (pszTemp[1] == '>') {
					pszTemp += 2;
					continue;
				}
			} else if (pszTemp[0] == 'r') {
				dwColor = 0xFFFF0000;
				if (pszTemp[1] == '>') {
					pszTemp += 2;
					continue;
				}
			} else if (pszTemp[0] == 'y') {
				dwColor = 0xFFFFFF00;
				if (pszTemp[1] == '>') {
					pszTemp += 2;
					continue;
				}
			} else if (pszTemp[0] == 'b') {
				dwColor = 0xFF0000FF;
				if (pszTemp[1] == '>') {
					pszTemp += 2;
					continue;
				}
			} else if (pszTemp[0] == 'g') {
				dwColor = 0xFF808080;
				if (pszTemp[1] == '>') {
					pszTemp += 2;
					continue;
				}
			} else if (pszTemp[0] == 'n') {
				pLineInfo->byFontType = byFont;
				pLineInfo->sFontHeight = (USHORT)nHeight;
				pLineInfo->sFontWidth = (USHORT)nWidth;
				pLineInfo->sHeight = sCurHeight;
				m_LineList.push_back(pLineInfo);

				pLineInfo = new MEMO_LINE_INFO;
				if (pLineInfo == nullptr)
					return;

				// ????
				sCom = sStartCom;
				sxPos = sCom;
				sCurHeight = (USHORT)nHeight;
				strDesp = "";

				if (pszTemp[1] == '>') {
					pszTemp += 2;
					continue;
				}
			} else if (pszTemp[0] == 't') {
				sCom += 4 * (USHORT)nWidth;
				sxPos = sCom;
				if (sCom >= sMaxCom) {
					pLineInfo->byFontType = byFont;
					pLineInfo->sFontHeight = (USHORT)nHeight;
					pLineInfo->sFontWidth = (USHORT)nWidth;
					pLineInfo->sHeight = sCurHeight;
					m_LineList.push_back(pLineInfo);
					pLineInfo = new MEMO_LINE_INFO;
					if (pLineInfo == nullptr)
						return;

					// ????
					sCom = sStartCom;
					sxPos = sCom;
					sCurHeight = (USHORT)nHeight;
					strDesp = "";
				}
				if (pszTemp[1] == '>') {
					pszTemp += 2;
					continue;
				}
			} else if (pszTemp[0] == '/') {
				if (pszTemp[1] == '>') {
					pszTemp++;
					continue;
				}

				char* pszEnd = (char*)strstr(pszTemp, ">");
				if (pszEnd == nullptr) {
					continue;
				}
				char szIcon[32] = "";
				strncpy(szIcon, pszTemp + 1, pszEnd - pszTemp - 1);
				pszTemp = pszEnd;
				// ??????
				CGraph* pIcon = m_pImageList->GetImage(szIcon);
				if (!pIcon) {
					continue;
				} else {
					// ??????????????????�?????????????
					if (sCom + pIcon->GetWidth() > sMaxCom) {
						pLineInfo->byFontType = byFont;
						pLineInfo->sFontHeight = (USHORT)nHeight;
						pLineInfo->sFontWidth = (USHORT)nWidth;
						pLineInfo->sHeight = sCurHeight;
						m_LineList.push_back(pLineInfo);
						pLineInfo = new MEMO_LINE_INFO;
						if (pLineInfo == nullptr)
							return;

						sCom = sStartCom;
						sxPos = sCom;
						sCurHeight = (USHORT)nHeight;
						strDesp = "";
					}

					// ??????????????
					if (sCurHeight < pIcon->GetHeight() + 4) {
						sCurHeight = pIcon->GetHeight() + 4;
					}

					MEMO_COLINFO Info;
					memset(&Info, 0, sizeof(MEMO_COLINFO));
					Info.byType = COL_ICON;
					Info.strDesp = "";
					Info.pIcon = pIcon;
					Info.sxPos = sxPos;
					pLineInfo->ColInfoArray.push_back(Info);

					sCom += pIcon->GetWidth();
					sxPos = sCom;

					// ?????????????�??????????????
					if (sCom >= sMaxCom) {
						pLineInfo->byFontType = byFont;
						pLineInfo->sFontHeight = (USHORT)nHeight;
						pLineInfo->sFontWidth = (USHORT)nWidth;
						pLineInfo->sHeight = sCurHeight;
						m_LineList.push_back(pLineInfo);
						pLineInfo = new MEMO_LINE_INFO;
						if (pLineInfo == nullptr)
							return;

						sCom = sStartCom;
						sxPos = sCom;
						sCurHeight = (USHORT)nHeight;
						strDesp = "";
					}
				}
			} else {
				// d??????????
				continue;
			}

			// '???�??????�
			pszTemp++;
			continue;
		} else if (pszTemp[0] == '>') {
			// '???�??????�
			// ????????
			MEMO_COLINFO Info;
			Info.strDesp = strDesp;
			Info.dwColor = dwColor;
			Info.sxPos = sxPos;
			Info.byType = COL_TEXT;
			Info.pIcon = nullptr;
			pLineInfo->ColInfoArray.push_back(Info);

			dwColor = dwDefColor; // ???????????
			pszTemp++;
			sxPos = sCom;
			strDesp = "";
			continue;
		}

		// ??????h??GBK
		BOOL bFlag1 = 0x81 <= (BYTE)pszTemp[0] && (BYTE)pszTemp[0] <= 0xFE;
		BOOL bFlag2 = (0x40 <= (BYTE)pszTemp[1] && (BYTE)pszTemp[1] <= 0x7E) || (0x7E <= (BYTE)pszTemp[1] && (BYTE)pszTemp[1] <= 0xFE);
		if (bFlag1 && bFlag2) {
			if (sMaxCom - sCom >= 2) {
				szData[0] = pszTemp[0];
				szData[1] = pszTemp[1];
				strDesp += szData;

				// ?z?????????h?????
				sCom += (USHORT)nWidth * 2;
				pszTemp += 2;
			} else {
				MEMO_COLINFO Info;
				memset(&Info, 0, sizeof(MEMO_COLINFO));
				Info.byType = COL_TEXT;
				Info.strDesp = strDesp;
				Info.pIcon = nullptr;
				Info.sxPos = sxPos;
				Info.dwColor = dwColor;
				pLineInfo->ColInfoArray.push_back(Info);

				pLineInfo->byFontType = byFont;
				pLineInfo->sFontHeight = (USHORT)nHeight;
				pLineInfo->sFontWidth = (USHORT)nWidth;
				pLineInfo->sHeight = sCurHeight;
				m_LineList.push_back(pLineInfo);
				pLineInfo = new MEMO_LINE_INFO;
				if (pLineInfo == nullptr)
					return;

				// ????
				sCom = sStartCom;
				sxPos = sCom;
				sCurHeight = (USHORT)nHeight;
				strDesp = "";
			}
		} else {
			if (sMaxCom - sCom >= 1) {
				szData[0] = pszTemp[0];
				strDesp += szData;

				// ?z?????????h?????
				sCom += (USHORT)nWidth;
				pszTemp++;
			} else {
				MEMO_COLINFO Info;
				memset(&Info, 0, sizeof(MEMO_COLINFO));
				Info.byType = COL_TEXT;
				Info.strDesp = strDesp;
				Info.pIcon = nullptr;
				Info.sxPos = sxPos;
				Info.dwColor = dwColor;
				pLineInfo->ColInfoArray.push_back(Info);

				pLineInfo->byFontType = byFont;
				pLineInfo->sFontHeight = (USHORT)nHeight;
				pLineInfo->sFontWidth = (USHORT)nWidth;
				pLineInfo->sHeight = sCurHeight;
				m_LineList.push_back(pLineInfo);
				pLineInfo = new MEMO_LINE_INFO;
				if (pLineInfo == nullptr)
					return;

				// ????
				sCom = sStartCom;
				sxPos = sCom;
				sCurHeight = (USHORT)nHeight;
				strDesp = "";
			}
		}
	}

	if (strDesp.length() > 0) {
		MEMO_COLINFO Info{};
		// memset( &Info, 0, sizeof(MEMO_COLINFO) );
		Info.byType = COL_TEXT;
		Info.strDesp = strDesp;
		Info.pIcon = nullptr;
		Info.sxPos = sxPos;
		Info.dwColor = dwColor;
		pLineInfo->ColInfoArray.push_back(Info);

		pLineInfo->byFontType = byFont;
		pLineInfo->sFontHeight = (USHORT)nHeight;
		pLineInfo->sFontWidth = (USHORT)nWidth;
		pLineInfo->sHeight = sCurHeight;
		m_LineList.push_back(pLineInfo);

	} else if (pLineInfo) {
		pLineInfo->byFontType = byFont;
		pLineInfo->sFontHeight = (USHORT)nHeight;
		pLineInfo->sFontWidth = (USHORT)nWidth;
		pLineInfo->sHeight = sCurHeight;
		m_LineList.push_back(pLineInfo);

	} else if (pLineInfo && pLineInfo->ColInfoArray.size() <= 0) {
		// delete pLineInfo;
		SAFE_DELETE(pLineInfo); // UI????????
	}
}

//------------------------------------------------------------------------
//	Class CImageData
//------------------------------------------------------------------------
vector<CFaceImage::sImage*> CFaceImage::m_pData;
CFaceImage::CFaceImage() {
}

CFaceImage::~CFaceImage() {
	sImage* pNode;
	vector<sImage*>::iterator Iter;
	while (m_pData.size() > 0) {
		Iter = m_pData.begin();
		pNode = *Iter;

		// delete pNode;
		SAFE_DELETE(pNode); // UI????????

		m_pData.erase(Iter);
	}
}

CGraph* CFaceImage::GetImage(const char* szImage) {
	int nIndex = atoi(szImage);
	if (nIndex >= 0 && nIndex < g_TextParse.GetFaceCount()) {
		return g_TextParse.GetFace(nIndex);
	}
	vector<sImage*>::iterator Iter;
	for (Iter = m_pData.begin(); Iter != m_pData.end(); Iter++) {
		if ((*Iter)->name == szImage) {
			return (*Iter)->pImage;
		}
	}
	return nullptr;
}

bool CFaceImage::AddImage(const char* szImage, CGraph* pImage) {
	if (!szImage || !pImage)
		return false;
	sImage* pNewImage = new sImage;
	pNewImage->name = szImage;
	pNewImage->pImage = pImage;
	m_pData.push_back(pNewImage);
	return true;
}
