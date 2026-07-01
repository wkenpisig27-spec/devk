//=============================================================================
// FileName: AttachManage.cpp
// Creater: ZhangXuedong
// Date: 2004.10.19
// Comment: CPassengerMgr class
//=============================================================================
#include "stdafx.h"
#include "AttachManage.h"
#include "SubMap.h"
#include "GameAppNet.h"

_DBC_USING;

CPassengerMgr::CPassengerMgr(uLong) : PreAllocStru(1) {
	m_pCLstHead = 0;
	m_pCLstTail = 0;
	m_lNum = 0;
}

CPassengerMgr::~CPassengerMgr() {
}

void CPassengerMgr::Initially() {
	m_pCLstHead = 0;
	m_pCLstTail = 0;
	m_lNum = 0;
}

void CPassengerMgr::Finally() {
	FreeAll();
}

void CPassengerMgr::Add(CAttachable* pCAttach) {
	T_B if (m_pCLstTail) {
		pCAttach->m_pCPassengerLast = m_pCLstTail;
		pCAttach->m_pCPassengerNext = 0;
		m_pCLstTail->m_pCPassengerNext = pCAttach;
		m_pCLstTail = pCAttach;
	}
	else // 空链
	{
		pCAttach->m_pCPassengerLast = 0;
		pCAttach->m_pCPassengerNext = 0;
		m_pCLstHead = pCAttach;
		m_pCLstTail = pCAttach;
	}
	m_lNum++;
	T_E
}

void CPassengerMgr::Delete(CAttachable* pCAttach) {
	T_B if (pCAttach && m_pCLstHead) {
		if (m_pCCurPess == pCAttach)
			m_pCCurPess = pCAttach->m_pCPassengerNext;
		if (pCAttach->m_pCPassengerLast)
			pCAttach->m_pCPassengerLast->m_pCPassengerNext = pCAttach->m_pCPassengerNext;
		if (pCAttach->m_pCPassengerNext)
			pCAttach->m_pCPassengerNext->m_pCPassengerLast = pCAttach->m_pCPassengerLast;
		if (pCAttach == m_pCLstHead)
			if (m_pCLstHead = m_pCLstHead->m_pCPassengerNext)
				m_pCLstHead->m_pCPassengerLast = 0;
		if (pCAttach == m_pCLstTail)
			if (m_pCLstTail = m_pCLstTail->m_pCPassengerLast)
				m_pCLstTail->m_pCPassengerNext = 0;
		pCAttach->m_pCPassengerLast = pCAttach->m_pCPassengerNext = 0;

		m_lNum--;
	}
	T_E
}

bool CPassengerMgr::SetLeader(CAttachable* pCAttach) {
	T_B bool bRet = false;

	if (!m_pCLstHead) {
		bRet = false;
	} else if (pCAttach == m_pCLstHead) {
		bRet = true;
	} else if (pCAttach == m_pCLstTail) {
		m_pCLstTail = pCAttach->m_pCPassengerLast;
		m_pCLstTail->m_pCPassengerNext = 0;
		pCAttach->m_pCPassengerLast = 0;
		pCAttach->m_pCPassengerNext = m_pCLstHead;
		m_pCLstHead = pCAttach;
		bRet = true;
	} else if (pCAttach->m_pCPassengerLast && pCAttach->m_pCPassengerNext) {
		pCAttach->m_pCPassengerLast->m_pCPassengerNext = pCAttach->m_pCPassengerNext;
		pCAttach->m_pCPassengerNext->m_pCPassengerLast = pCAttach->m_pCPassengerLast;
		pCAttach->m_pCPassengerLast = 0;
		pCAttach->m_pCPassengerNext = m_pCLstHead;
		m_pCLstHead = pCAttach;
		bRet = true;
	} else
		bRet = false;

	return bRet;
	T_E
}

CAttachable* CPassengerMgr::GetLeader() {
	T_B
		CAttachable* pCAttach;

	pCAttach = m_pCLstHead;

	return pCAttach;
	T_E
}

void CPassengerMgr::FreeAll() {
	T_B
		CAttachable* pCTemp;
	SubMap* pSubMap;
	while (pCTemp = m_pCLstHead) {
		pCTemp->m_pCPassengerLast = 0;
		pCTemp->m_pCPassengerNext = 0;
		pSubMap = pCTemp->GetSubMap();
		if (pSubMap)
			pSubMap->GoOut(pCTemp);
		m_pCLstHead = m_pCLstHead->m_pCPassengerNext;
		pCTemp->Free();
	}
	m_pCLstHead = 0;
	m_pCLstTail = 0;
	m_lNum = 0;
	T_E
}

void CPassengerMgr::DeleteAll() {
	T_B
		CAttachable* pCTemp;
	while (pCTemp = m_pCLstHead) {
		m_pCLstHead = m_pCLstHead->m_pCPassengerNext;
		pCTemp->m_pCPassengerLast = 0;
		pCTemp->m_pCPassengerNext = 0;
	}
	m_pCLstHead = 0;
	m_pCLstTail = 0;
	m_lNum = 0;
	T_E
}
