#pragma once

#include "map"
#include <string>
// #include "atltime.h"

class CPlayer {
public:
	CPlayer(void);
	~CPlayer(void);
	void Initial();
	void AddPlayer(int nTask, std::string strPlayerName);
	std::string GetPlayer(int nTask);
	void RemovePlayer(int nTask);

private:
	typedef std::map<int, std::string> StringMap;
	StringMap m_mapPlayers;
};
