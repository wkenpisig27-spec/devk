//=============================================================================
// FileName: EntitySpawn.h
// Creater: ZhangXuedong
// Date: 2004.09.10
// Comment: CChaSpawn class
//=============================================================================

#ifndef ENTITYSPAWN_H
#define ENTITYSPAWN_H

#include "GameAppNet.h"
#include "MonRefRecord.h"
#include "SwitchMapRecord.h"
#include "EventRecord.h"
#include <NpcRecord.h>
#include "Npc.h"

class SubMap;

class CChaSpawn {
public:
	struct SMonInfo {
		int lRegionID;
		int lMonsterNum[defMAX_REGION_MONSTER_TYPE];
	};

	CChaSpawn();
	virtual ~CChaSpawn();

	bool Init(char* szSpawnTable, int lRegionNum);
	int Load(SubMap* pCMap);
	int Reload();

	int GetChaCount(void) { return m_lCount; }

protected:
private:
	int m_lRegionNum;
	SMonInfo* m_pSMonInfo;

	char m_szSpawnTable[_MAX_FNAME];
	int m_lRecordNum;
	CMonRefRecordSet* m_pCMonRefRecordSet;

	SubMap* m_pCMap;

	int m_lCount;
};

class CMapSwitchEntitySpawn {
public:
	CMapSwitchEntitySpawn();
	virtual ~CMapSwitchEntitySpawn();

	bool Init(char* szSpawnTable, int lRecordNum);
	int Load(SubMap* pCMap);
	int Reload();

protected:
private:
	char m_szSpawnTable[_MAX_FNAME];
	int m_lRecordNum;
	SubMap* m_pCMap;
	CSwitchMapRecordSet* m_pCSwitchMapRecSet;
};

class CNpcSpawn {
public:
	CNpcSpawn();
	~CNpcSpawn();

	bool Init(char* szSpawnTable, int lRecordNum);
	int Load(SubMap& submap);
	int Reload();
	void Clear();
	CNpcRecord* GetNpcInfo(USHORT sNpcID);

	// 召唤NPC出现
	BOOL SummonNpc(const char szNpc[], USHORT sAreaID, USHORT sTime);
	mission::CNpc* FindNpc(const char szName[]);

protected:
	char m_szSpawnTable[_MAX_FNAME];
	int m_lRecordNum;
	CNpcRecordSet* m_pNpcRecordSet;

	// 招唤的NPC列表
	mission::CNpc* m_NpcList[ROLE_MAXNUM_MAPNPC];
	USHORT m_sNumNpc;
};

#endif // ENTITYSPAWN_H