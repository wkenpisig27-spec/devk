#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "string"
#include "Player.h"

CPlayer::CPlayer(void) {
}

CPlayer::~CPlayer(void) {
}

void CPlayer::Initial() {
	m_mapPlayers.clear();
}

void CPlayer::AddPlayer(int nTask, std::string strPlayerName) {
	m_mapPlayers[nTask] = strPlayerName;
}

std::string CPlayer::GetPlayer(int nTask) {
	StringMap::const_iterator iter = m_mapPlayers.find(nTask);
	if (iter == m_mapPlayers.end())
		return "";
	return iter->second;
}

void CPlayer::RemovePlayer(int nTask) {
	m_mapPlayers.erase(nTask);
}