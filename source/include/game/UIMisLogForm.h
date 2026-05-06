#pragma once
#include "UIGlobalVar.h"
#include "NetProtocol.h"
#include "UIMissionForm.h"

namespace GUI {
#define MISLOG_REFRESH_TIME 2000 // 锟斤拷锟斤拷锟斤拷志刷锟斤拷时锟斤拷

class CTreeNodeObj;
class CMisLogForm : public CUIInterface {
public:
	CMisLogForm();
	~CMisLogForm();

	void MisLogList(const NET_MISLOG_LIST& List);
	void MissionLog(WORD wMisID, const NET_MISPAGE& page);
	void MisLogState(WORD wMisID, BYTE byState);
	void MisClear(WORD wMisID);
	void MisAddLog(WORD wMisID, BYTE byState);
	void MisRefresh();

protected:
	bool Init();
	void End();

	void GetMisData(WORD wMisID, BYTE& byType, char szBuf[], USHORT sBufLen);
	BOOL AddNode(WORD wMisID, BYTE byState, BYTE& byType);
	void ClearAllNode();

private:
	// 锟斤拷锟节达拷锟斤拷锟斤拷息锟斤拷锟斤拷
	static void _MouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey);
	static void _ItemClickEvent(std::string strItem);
	static void _MouseDown(CGuiData* pSender, int x, int y, DWORD key);
	static void _Show(CGuiData* pSender);
	// 确锟斤拷锟角凤拷锟叫讹拷锟斤拷锟斤拷
	static void _evtBreakYesNoEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey);

	CForm* m_pForm;

	// 锟斤拷锟斤拷锟斤拷志锟截硷拷
	CTreeView* m_pMisTree;
	CMemoEx* m_pMisInfo;

	// 锟斤拷锟斤拷锟斤拷锟酵节碉拷
	CTreeNodeObj* m_pNormal;
	CTreeNodeObj* m_pHistory;
	CTreeNodeObj* m_pGuild;
	CTreeNodeObj* m_pInvalid;

	// 锟斤拷锟斤拷锟斤拷志锟斤拷息
	NET_MISLOG_LIST m_LogList;

	// 锟斤拷前锟斤拷锟斤拷说锟斤拷
	WORD m_wMisID;
	DWORD m_dwUpdateTick;
};

} // namespace GUI
