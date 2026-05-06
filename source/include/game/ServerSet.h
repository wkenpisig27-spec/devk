#pragma once
#include "TableData.h"

#define MAX_GROUP_GATE 5
#define MAX_REGION_GROUP 20
#define MAX_REGION 20

#include <vector>
#include <map>
typedef std::vector<std::string> ReginList;
typedef std::map<int, ReginList> ReginListMap;

extern std::string g_serverset;

// Server group info - holds Gate IPs/hostnames for a server group
// NOTE: szGateIP increased to 64 chars to support domain names (e.g., play.slimepirates.online)
// Supports hostname:port format (e.g., play.slimepirates.online:25565)
class CServerGroupInfo : public CRawDataInfo {
public:
	CServerGroupInfo() {
		cValidGateCnt = 0;
		memset(szGateIP, 0, sizeof(szGateIP));
		memset(nGatePort, 0, sizeof(nGatePort));
		memset(szRegion, 0, sizeof(szRegion));
	}

	char szGateIP[MAX_GROUP_GATE][64];  // Increased from 16 to 64 for domain names
	unsigned short nGatePort[MAX_GROUP_GATE];   // Port for each gate (default 1973, use 25565 for TCPShield)
	char szRegion[16];
	char cValidGateCnt;
};

class CServerSet : public CRawDataSet {
public:
	static CServerSet* I() { return _Instance; }

	CServerSet(int nIDStart, int nIDCnt)
		: CRawDataSet(nIDStart, nIDCnt) {
		_Instance = this;
		_Init();

		memset(m_nCurGroupList, 0, sizeof(m_nCurGroupList));
		memset(m_nCurGroupCnt, 0, sizeof(m_nCurGroupCnt));

		std::ifstream in;
		in.open(g_serverset.c_str());

		for (int i = 0; i < MAX_REGION; i++)
			m_nCurGroupCnt[i] = 0;

		std::string strLine;
		char szRegion[256] = {0};
		if (in.is_open()) {
			m_nRegionCnt = 0;
			while (!in.eof()) {
				in.getline(szRegion, sizeof(szRegion));
				if (strlen(szRegion) == 0)
					break;

				std::string strList[2];
				Util_ResolveTextLine(szRegion, strList, 2, ',');

				strcpy(m_szRegionName[m_nRegionCnt], strList[0].c_str());
				strcpy(m_szRegionID[m_nRegionCnt++], strList[1].c_str());

				if (m_nRegionCnt >= MAX_REGION)
					break;
			}
			in.close();
		}
	}

public:
	int m_nCurGroupList[MAX_REGION][MAX_REGION_GROUP]; // ?????????????Group
	int m_nCurGroupCnt[MAX_REGION];

	char m_szRegionName[MAX_REGION][32];
	char m_szRegionID[MAX_REGION][32];

	int m_nRegionCnt;

protected:
	static CServerSet* _Instance; // ?????, ?????

	virtual CRawDataInfo* _CreateRawDataArray(int nCnt) {
		return new CServerGroupInfo[nCnt];
	}

	virtual void _DeleteRawDataArray() {
		delete[] (CServerGroupInfo*)_RawDataArray;
	}

	virtual int _GetRawDataInfoSize() {
		return sizeof(CServerGroupInfo);
	}

	virtual void* _CreateNewRawData(CRawDataInfo* pInfo) {
		return nullptr;
	}

	virtual void _DeleteRawData(CRawDataInfo* pInfo) {
		SAFE_DELETE(pInfo->pData);
	}

	virtual BOOL _ReadRawDataInfo(CRawDataInfo* pRawDataInfo, std::vector<std::string>& ParamList) {
		CServerGroupInfo* pInfo = (CServerGroupInfo*)pRawDataInfo;

		strncpy(pInfo->szRegion, ParamList[0].c_str(), sizeof(pInfo->szRegion) - 1);
		pInfo->szRegion[sizeof(pInfo->szRegion) - 1] = '\0';
		
		for (int i = 0; i < MAX_GROUP_GATE; i++) // Read up to 5 gate IPs/hostnames
		{
			const std::string& addr = ParamList[i + 1];
			if (addr == "0" || addr.empty()) {
				pInfo->nGatePort[i] = 1973;  // Default port
				break;
			}
			
			// Parse hostname:port format (e.g., play.slimepirates.online:25565)
			size_t colonPos = addr.rfind(':');
			if (colonPos != std::string::npos && colonPos > 0) {
				// Found port - extract hostname and port separately
				std::string hostname = addr.substr(0, colonPos);
				std::string portStr = addr.substr(colonPos + 1);
				
				strncpy(pInfo->szGateIP[i], hostname.c_str(), sizeof(pInfo->szGateIP[i]) - 1);
				pInfo->szGateIP[i][sizeof(pInfo->szGateIP[i]) - 1] = '\0';
				pInfo->nGatePort[i] = (unsigned short)std::stoi(portStr);
			} else {
				// No port specified - use default 1973
				strncpy(pInfo->szGateIP[i], addr.c_str(), sizeof(pInfo->szGateIP[i]) - 1);
				pInfo->szGateIP[i][sizeof(pInfo->szGateIP[i]) - 1] = '\0';
				pInfo->nGatePort[i] = 1973;
			}
			pInfo->cValidGateCnt++;
		}

		return TRUE;
	}

	virtual void _ProcessRawDataInfo(CRawDataInfo* pRawDataInfo) {
		CServerGroupInfo* pInfo = (CServerGroupInfo*)pRawDataInfo;

		for (size_t i = 0; i < m_nRegionCnt; i++) {
			if (strcmp(m_szRegionName[i], pInfo->szRegion) == 0) // ????
			{
				m_nCurGroupList[i][m_nCurGroupCnt[i]] = pInfo->nID;
				m_nCurGroupCnt[i]++;
				break;
			}
		}
	}
};

// ?????, ????GateIP??
inline CServerGroupInfo* GetServerGroupInfo(int nGroupID) {
	return (CServerGroupInfo*)CServerSet::I()->GetRawDataInfo(nGroupID);
}

// ?????, ????GateIP??
inline CServerGroupInfo* GetServerGroupInfo(const char* pszGroupName) {
	return (CServerGroupInfo*)CServerSet::I()->GetRawDataInfo(pszGroupName);
}

inline int GetCurServerGroupCnt(int nRegionNo) {
	return CServerSet::I()->m_nCurGroupCnt[nRegionNo];
}

inline const char* GetCurServerGroupName(int nRegionNo, int nGroupNo) {
	if (nGroupNo >= GetCurServerGroupCnt(nRegionNo))
		return 0;

	int nNo = CServerSet::I()->m_nCurGroupList[nRegionNo][nGroupNo];
	return GetServerGroupInfo(nNo)->szDataName;
}

// ???????? Michael Chen 2005-06-01
inline int GetRegionCnt() {
	return CServerSet::I()->m_nRegionCnt;
}

inline const char* GetCurRegionName(int nRegionNo) {
	if (nRegionNo < 0 && nRegionNo > GetRegionCnt())
		return 0;

	return CServerSet::I()->m_szRegionName[nRegionNo];
}

// ?????, ????GateIP
inline const char* SelectGroupIP(int nRegionNo, int nGroupNo) {
	LG("connect", "Select Region %d Group %d\n", nRegionNo, nGroupNo);
	if (nGroupNo >= GetCurServerGroupCnt(nRegionNo)) {
		LG("connect", RES_STRING(CL_LANGUAGE_MATCH_387), GetCurServerGroupCnt(nRegionNo), nGroupNo);
		return 0;
	}

	int nNo = CServerSet::I()->m_nCurGroupList[nRegionNo][nGroupNo];
	CServerGroupInfo* pGroup = GetServerGroupInfo(nNo);
	if (!pGroup) {
		LG("connect", "Group Not Found!\n");
		return nullptr;
	}

	if (pGroup->cValidGateCnt == 0) {
		LG("connect", RES_STRING(CL_LANGUAGE_MATCH_388));
		return nullptr;
	}

	srand(GetTickCount());

	int nGateNo = rand() % (int)(pGroup->cValidGateCnt);

	LG("connect", RES_STRING(CL_LANGUAGE_MATCH_389), pGroup->szDataName, nGateNo, pGroup->szGateIP[nGateNo], pGroup->cValidGateCnt);

	return pGroup->szGateIP[nGateNo];
}

// Select Gate IP and Port - returns true if successful
// Supports hostname:port format in ServerSet.txt (e.g., play.slimepirates.online:25565)
inline bool SelectGroupIPAndPort(int nRegionNo, int nGroupNo, const char** outIP, unsigned short* outPort) {
	LG("connect", "Select Region %d Group %d\n", nRegionNo, nGroupNo);
	if (nGroupNo >= GetCurServerGroupCnt(nRegionNo)) {
		LG("connect", RES_STRING(CL_LANGUAGE_MATCH_387), GetCurServerGroupCnt(nRegionNo), nGroupNo);
		return false;
	}

	int nNo = CServerSet::I()->m_nCurGroupList[nRegionNo][nGroupNo];
	CServerGroupInfo* pGroup = GetServerGroupInfo(nNo);
	if (!pGroup) {
		LG("connect", "Group Not Found!\n");
		return false;
	}

	if (pGroup->cValidGateCnt == 0) {
		LG("connect", RES_STRING(CL_LANGUAGE_MATCH_388));
		return false;
	}

	srand(GetTickCount());
	int nGateNo = rand() % (int)(pGroup->cValidGateCnt);

	*outIP = pGroup->szGateIP[nGateNo];
	*outPort = pGroup->nGatePort[nGateNo];

	LG("connect", "Selected gate: %s (idx=%d) port=%d from %s (total gates=%d)\n", 
		*outIP, nGateNo, *outPort, pGroup->szDataName, pGroup->cValidGateCnt);

	return true;
}
