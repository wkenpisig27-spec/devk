//----------------------------------------------------------------------
// ����:����
// ����:lh 2004-07-19
// ����޸�����:2004-10-09
//----------------------------------------------------------------------
#pragma once
#include "UICommand.h"
#include "CompCommand.h"
#include "SkillRecord.h"
#include "ItemRecord.h"
#include <array>
#include <utility>  // for std::pair, std::make_pair
class CItemRecord;
struct SGameAttr;
struct xShipInfo;
struct NET_CHARTRADE_BOATDATA;
class CStoneInfo;
class CItemRefineInfo;
class CItemRefineEffectInfo;

class CAniClock;
class CSkillRecord;

namespace GUI {

struct SItemForge {
	bool IsForge; // �Ƿ���
	int nHoleNum; // �м�����
	int nLevel;	  // �����ȼ�

	CStoneInfo* pStoneInfo[3]; // ���ű�ʯ,Ϊ���ޱ�ʯ
	int nStoneLevel[3];		   // ��Ӧ�ı�ʯ�ȼ�
	char szStoneHint[3][256];  // ��ʯ˵��
	int nStoneNum;			   // ��ʯ����

	static SItemForge& Convert(DWORD v, int nItemID = -1);

	static float GetAlpha(int nTotalLevel); // �����ܵȼ�,�õ���Чalpha

public:				   // Ϊ������
	int nStoneType[3]; // ��ʯ����,û��Ϊ-1
	CItemRefineInfo* pRefineInfo;
	CItemRefineEffectInfo* pEffectInfo;
	int nEffectLevel;

private:
	void Refresh(int nItemID);
};

// ��Դ��SItemGrid,��ʵ�������Բ�һ����ʵ�������� = SItemGrid��ʵ�������� + ��ȡ����
struct SItemHint {
	short sID;
	short sNum;
	std::array<short, 2> sEndure;
	std::array<short, 2> sEnergy;
	char chForgeLv;
	std::array<int, enumITEMDBP_MAXNUM> lDBParam;
	std::array<short, ITEMATTR_CLIENT_MAX> sInstAttr;
	bool bItemTradable;
	long expiration;

	void Convert(SItemGrid& ItemGrid, CItemRecord* pInfo);
};

class CItemCommand : public CCommandObj {
	enum {
		SOLID_ALPHA = 0xa0000000,
		INVALID_COLOR = 0x00ff0000,
	};

public:
	CItemCommand(CItemRecord* pItem);
	CItemCommand(const CItemCommand& rhs);
	CItemCommand& operator=(const CItemCommand& rhs);
	~CItemCommand();
	ITEM_CLONE(CItemCommand)

	virtual void RenderEnergy(int x, int y);
	virtual void SaleRender(int x, int y, int nWidth, int nHeight);
	virtual void Render(int x, int y);
	virtual void OwnDefRender(int x, int y, int nWidth, int nHeight);

	virtual bool UseCommand(bool isRightClick = false);
	virtual bool StartCommand();
	virtual bool IsAllowUse();

	virtual bool IsDragFast();
	virtual int GetTotalNum();
	virtual bool IsAllowThrow();
	virtual bool MouseDown();
	virtual void SetTotalNum(int num);
	virtual const char* GetName();

	virtual bool GetCanDrag() { return _canDrag; };
	virtual void SetCanDrag(bool drag) { _canDrag = drag; };
	CGuiPic* GetIcon() { return _pImage; }
	void SetData(const SItemGrid& item);
	SItemGrid& GetData() { return _ItemData; }

	void SetBoatHint(const NET_CHARTRADE_BOATDATA* const pBoat);

	virtual void SetIsValid(bool v);
	virtual bool GetIsValid();

	virtual bool GetIsPile();
	virtual __int64 GetPrice();

	void SetIsSolid(bool v);
	bool GetIsSolid();

	bool IsLocked() const { return static_cast<bool>(_ItemData.dwDBID); }

	CItemRecord* GetItemInfo() { return _pItem; }

	int GetThrowLink(); // �ɹ�>=0,ʧ�ܷ���-1

	void SetPrice(__int64 n) { _nPrice = n; }

	SItemForge& GetForgeInfo();
	std::string GetStoneHint(int nLevel = -1); // �õ���ʯ��hint������Ϊ��1�ǵ��߱�����hint������Ϊ����ȼ���hint

	// Unified cooldown system
	static void ClearCoolDown() { _mapCooldown.clear(); }
	static void SetCooldown(int nCooldownGroup, DWORD dwStartTime, int nDurationMs) { 
		// Memory safeguard: cleanup if map is getting large
		if (_mapCooldown.size() >= MAX_COOLDOWN_ENTRIES) {
			_CleanupExpiredCooldowns();
		}
		_mapCooldown[nCooldownGroup] = std::make_pair(dwStartTime, nDurationMs); 
	}
	bool IsOnCooldown() const;
	float GetCooldownRemaining() const;
	void StartCooldown();

	void SetColor(DWORD c) { _dwColor = c; }
	DWORD GetColor() { return _dwColor; }

protected:
	virtual bool IsAtOnce();
	virtual bool ReadyUse();
	virtual void Error();
	virtual void AddHint(int x, int y);

	void PUSH_HINT(const char* str, int value, DWORD color = COLOR_WHITE);

	void _Copy(const CItemCommand& rhs);
	// int     _GetValue( int nItemAttrType, SItemGrid& item );
	int _GetValue(int nItemAttrType, SItemHint& item);
	void _ShowWork(CItemRecord* pItem, SGameAttr* pAttr);									   // ������ʾ���ߵ�ְҵ����
	void _ShowFusionWork(CItemRecord* pAppearItem, CItemRecord* pEquipItem, SGameAttr* pAttr); // ������ʾ�ۺϺ���ߵ�ְҵ����
	void _AddDescriptor();
	void _ShowWork(xShipInfo* pInfo, SGameAttr* pAttr); // ������ʾ����ְҵ����
	void _ShowBody(CItemRecord* _pItem2 = nullptr);		// ������ʾ��ɫ����
	void _ShowFusionBody(CItemRecord* pEquipItem);		// ������ʾ�ۺϺ��ɫ����

protected:
	// void	_PushItemAttr( int attr, SItemGrid& item, DWORD color=COLOR_WHITE );
	void _PushItemAttr(int attr, SItemHint& item, DWORD color = COLOR_WHITE);
	void _PushValue(const char* szFormat, int attr, SItemHint& item, DWORD color = COLOR_WHITE);

	int _GetCooldownGroup() const;    // Returns grouping key for cooldown map
	int _GetCooldownDuration() const; // Returns cooldown duration in milliseconds

	// Cleanup expired cooldown entries to prevent memory growth
	static void _CleanupExpiredCooldowns();
	static constexpr size_t MAX_COOLDOWN_ENTRIES = 256; // Safeguard against unbounded growth

private:
	CGuiPic* _pImage;
	CItemRecord* _pItem;

	//
	CAniClock* _pAniClock;
	CSkillRecord* _pSkill;

	DWORD _dwCooldownEnd;  // Tick when cooldown expires (for visual animation)
	//

	SItemGrid _ItemData;
	__int64 _nPrice;
	bool _canDrag;

private:
	DWORD _dwColor; // ��Ч��ɫ��ʾ, ��ʵ���͸��
	NET_CHARTRADE_BOATDATA* _pBoatHint;

	// Cooldown map: group ID -> (start tick, duration)
	// Storing both allows cleanup without knowing item info
	static std::map<int, std::pair<DWORD, int>> _mapCooldown;
};

// ��������
inline void CItemCommand::SetIsValid(bool v) {
	_dwColor = (_dwColor & 0xff000000) | (v ? 0x00ffffff : INVALID_COLOR);
}

inline bool CItemCommand::GetIsValid() {
	return (_dwColor & 0x00ffffff) == 0x00ffffff || (_dwColor & 0x00aaaaaa) == 0x00aaaaaa;
}

inline void CItemCommand::SetIsSolid(bool v) {
	_dwColor = (_dwColor & 0x00ffffff) | (v ? 0xff000000 : SOLID_ALPHA);
}

inline bool CItemCommand::GetIsSolid() {
	return (_dwColor & 0xff000000) == 0xff000000;
}

inline int CItemCommand::_GetCooldownGroup() const {
	if (_pItem->sType == 71 && _pSkill) {
		// Skill items share cooldown by skill ID
		return _pSkill->nID;
	}
	// Regular items: group by item ID
	return _pItem->lID;
}

inline int CItemCommand::_GetCooldownDuration() const {
	if (_pSkill) {
		return atoi(_pSkill->szFireSpeed);
	}
	return _pItem->nCooldown * 1000;
}

} // namespace GUI
// add by ning.yan 2008-11-11 begin
extern char _lock_pos_;
extern long _lock_fusion_item_id_;
// end