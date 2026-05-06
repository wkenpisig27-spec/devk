#pragma once

#include "map"
#include <string>

struct sPlayerData;

class CPlayerMgr {
public:
	CPlayerMgr(void);
	~CPlayerMgr(void);

	void Initial();
	void PlayerLogin(std::string strPlayerName);
	void PlayerLogout(std::string strPlayerName);

	// void AddPlayer(int nTask,std::string strPlayerName);
	// std::string GetPlayer(int nTask);
	// void RemovePlayer(int nTask);

private:
	typedef std::map<std::string, sPlayerData> StringMap;
	StringMap m_mapPlayers;
};
