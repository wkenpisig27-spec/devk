#include "StdAfx.h"
#include "PacketCmd.h"

#include "GameApp.h"
#include "Character.h"
#include "actor.h"
#include "procirculate.h"

#include "UIStoreForm.h"
_DBC_USING
//------------------------
// Э��C->S : ����
//------------------------
bool CS_Connect(dbc::cChar* hostname, dbc::uShort port, dbc::uLong timeout) {
	// LG("connect", RES_STRING(CL_LANGUAGE_MATCH_294), hostname);
	{
		MutexArmor lock(g_NetIF->m_mutProCir);
		if (g_NetIF->m_pCProCir) {
			delete g_NetIF->m_pCProCir;
		}
		g_NetIF->m_pCProCir = new CProCirculateCS(g_NetIF);
	}

	discordInit();
	updateDiscordPresence("Connecting...", "");

	MutexArmor lock(g_NetIF->m_mutProCir);
	return g_NetIF->m_pCProCir->Connect(hostname, port, timeout);
}

//------------------------
// Э��C->S : �Ͽ�����
//------------------------
void CS_Disconnect(int reason) {
	MutexArmor lock(g_NetIF->m_mutProCir);
	if (g_NetIF->m_pCProCir)
		g_NetIF->m_pCProCir->Disconnect(reason);
}

//------------------------
// Э��C->S : ��½
//------------------------
void CS_Login(const char* accounts, const char* password, const char* passport) {
	MutexArmor lock(g_NetIF->m_mutProCir);
	if (g_NetIF->m_pCProCir)
		g_NetIF->m_pCProCir->Login(accounts, password, passport);
}

//------------------------
// Э��C->S : ���͵ǳ�����
//------------------------
void CS_Logout() {
	{
		MutexArmor lock(g_NetIF->m_mutProCir);
		if (g_NetIF->m_pCProCir)
			g_NetIF->m_pCProCir->Logout();
	}
	return;

	// ��Ϊ�������˳���ʱ,����������µ�¼ʱ�ֻ�����������Կ,����˳�������������Կ, lh by 2006-02
	// ������Կ
	memset(g_NetIF->_key, 0, sizeof g_NetIF->_key);
	g_NetIF->_key_len = 0;
	g_NetIF->_comm_enc = 0;
	g_NetIF->_enc = false;
}

void CS_OfflineMode() {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_OFFLINE_MODE);
	g_NetIF->SendPacketMessage(pk);
}

void CS_CancelExit() {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_CANCELEXIT); // �����ж�
	g_NetIF->SendPacketMessage(pk);
}

//------------------------
// Э��C->S : ����ѡ���ɫ����
//------------------------
void CS_BeginPlay(const char* cha) {
	MutexArmor lock(g_NetIF->m_mutProCir);
	if (g_NetIF->m_pCProCir)
		g_NetIF->m_pCProCir->BeginPlay(cha);
}

//------------------------
// Э��C->S : ��������ѡ���ɫ����
//------------------------
void CS_EndPlay() {
	MutexArmor lock(g_NetIF->m_mutProCir);
	if (g_NetIF->m_pCProCir)
		g_NetIF->m_pCProCir->EndPlay();
}

//------------------------
// Э��C->S : �����½���ɫ����
//------------------------

void CS_NewCha(const char* chaname, const char* birth, LOOK& look) {
	MutexArmor lock(g_NetIF->m_mutProCir);
	if (g_NetIF->m_pCProCir)
		g_NetIF->m_pCProCir->NewCha(chaname, birth, look);
}

void CS_NewCha2(const char* chaname, const char* birth, int type, int hair, int face) {
	MutexArmor lock(g_NetIF->m_mutProCir);
	if (g_NetIF->m_pCProCir)
		g_NetIF->m_pCProCir->NewCha2(chaname, birth, type, hair, face);
}

//------------------------
// Э��C->S : ����ɾ����ɫ����
//------------------------
void CS_DelCha(const char* cha, const char szPassword2[]) {
	MutexArmor lock(g_NetIF->m_mutProCir);
	if (g_NetIF->m_pCProCir)
		g_NetIF->m_pCProCir->DelCha(cha, szPassword2);
}

//------------------------
// Э��C->S : ����������Ϣ
//------------------------
void CS_Say(const char* content) {
	MutexArmor lock(g_NetIF->m_mutProCir);
	if (g_NetIF->m_pCProCir)
		g_NetIF->m_pCProCir->Say(content);
}

void CS_GuildBankOper(stNetBank* pNetBank) {
	if (g_pGameApp->GetCurScene()->GetMainCha()->IsBoat()) {
		g_pGameApp->MsgBox("Not available at sea");
		return;
	}

	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CP_GUILDBANK);
	pk.WriteChar(0);
	pk.WriteChar(pNetBank->chSrcType);
	pk.WriteShort(pNetBank->sSrcID);
	pk.WriteShort(pNetBank->sSrcNum);
	pk.WriteChar(pNetBank->chTarType);
	pk.WriteShort(pNetBank->sTarID);
	g_NetIF->SendPacketMessage(pk);
}

void CS_GuildBankTakeGold(long long gold) {
	if (g_pGameApp->GetCurScene()->GetMainCha()->IsBoat()) {
		g_pGameApp->MsgBox("Not available at sea");
		return;
	}
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CP_GUILDBANK);
	pk.WriteChar(1);
	pk.WriteChar(0);
	pk.WriteLongLong(gold);
	g_NetIF->SendPacketMessage(pk);
}

void CS_GuildBankGiveGold(long long gold) {
	if (g_pGameApp->GetCurScene()->GetMainCha()->IsBoat()) {
		g_pGameApp->MsgBox("Not available at sea");
		return;
	}
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CP_GUILDBANK);
	pk.WriteChar(1);
	pk.WriteChar(1);
	pk.WriteLongLong(gold);
	g_NetIF->SendPacketMessage(pk);
}

void CS_ChangePass(const char* pass, const char* pin) {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CP_CHANGEPASS);
	// TODO(Ogge): Might need to hash it using blake2s?
	pk.WriteString(pass);
	pk.WriteString(pin);
	g_NetIF->SendPacketMessage(pk);
}

void CS_Register(const char* user, const char* pass, const char* email) {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_REGISTER);
	pk.WriteString(user);
	// Send plaintext password - server will hash with bcrypt before storing
	pk.WriteString(pass);
	pk.WriteString(email);
	g_NetIF->SendPacketMessage(pk);
}

void CS_StallSearch(int ItemID) {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_STALLSEARCH); // �����ж�
	pk.WriteLong(ItemID);
	g_NetIF->SendPacketMessage(pk);
}

// ������������
void CS_CreatePassword2(const char szPassword[]) {
	char szMD5[33];
	md5string(szPassword, szMD5);

	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_CREATE_PASSWORD2); // 结束行动
	WritePacketSequenceEncrypted(pk, g_NetIF->m_AESKey, (uint8_t*)szMD5, strlen(szMD5) + 1);
	g_NetIF->SendPacketMessage(pk);
}

void CS_UpdatePassword2(const char szOld[], const char szPassword[]) {
	char szOldMD5[33] = {0};
	char szNewMD5[33] = {0};
	md5string(szOld, szOldMD5);
	md5string(szPassword, szNewMD5);

	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_UPDATE_PASSWORD2); // 结束行动
	WritePacketSequenceEncrypted(pk, g_NetIF->m_AESKey, (uint8_t*)szOldMD5, strlen(szOldMD5) + 1);
	WritePacketSequenceEncrypted(pk, g_NetIF->m_AESKey, (uint8_t*)szNewMD5, strlen(szNewMD5) + 1);
	g_NetIF->SendPacketMessage(pk);
}

// ��������
void CS_LockKitbag() {
	WPacket pk = g_NetIF->GetWPacket();

	pk.WriteCmd(CMD_CM_KITBAG_LOCK);
	g_NetIF->SendPacketMessage(pk);
}

// ��������
void CS_UnlockKitbag(const char szPassword[]) {
	char szMD5[33] = {0};
	md5string(szPassword, szMD5);
	WPacket pk = g_NetIF->GetWPacket();

	pk.WriteCmd(CMD_CM_KITBAG_UNLOCK);
	WritePacketSequenceEncrypted(pk, g_NetIF->m_AESKey, (uint8_t*)szMD5, strlen(szMD5) + 1);
	g_NetIF->SendPacketMessage(pk);
}

// ��鱳������״̬
void CS_KitbagCheck() {
	WPacket pk = g_NetIF->GetWPacket();

	pk.WriteCmd(CMD_CM_KITBAG_CHECK);
	g_NetIF->SendPacketMessage(pk);
}

// �����Զ�����״̬
void CS_AutoKitbagLock(bool bAutoLock) {
	WPacket pk = g_NetIF->GetWPacket();

	pk.WriteCmd(CMD_CM_KITBAG_AUTOLOCK);
	pk.WriteChar(bAutoLock ? 1 : 0);
	g_NetIF->SendPacketMessage(pk);
}

void CS_KitbagExpand() {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_KITBAG_EXPAND);
	g_NetIF->SendPacketMessage(pk);
}

// Э��C->S : �����ж���Ϣ
void CS_BeginAction(CCharacter* pCha, DWORD type, void* param, CActionState* pState) {
	// Lock m_pCProCir: OnDisconnect on the network thread can delete/replace it at any time.
	// Without this lock, the main thread can dereference a freed pointer (use-after-free crash).
	MutexArmor lock(g_NetIF->m_mutProCir);
	if (g_NetIF->m_pCProCir)
		g_NetIF->m_pCProCir->BeginAction(pCha, type, param, pState);
}

// Э��C->S : ����ֹͣ�ж���Ϣ
void CS_EndAction(CActionState* pState) {
	MutexArmor lock(g_NetIF->m_mutProCir);
	if (g_NetIF->m_pCProCir)
		g_NetIF->m_pCProCir->EndAction(pState);
}

// Э��C->S : ����ֹͣ�ж���Ϣ
void CS_DieReturn(char chReliveType) {
	WPacket pk = g_NetIF->GetWPacket();

	pk.WriteCmd(CMD_CM_DIE_RETURN); // �����ж�
	pk.WriteChar(chReliveType);
	g_NetIF->SendPacketMessage(pk);

	// log
	LG(g_LogName.GetMainLogName(), "###Send(DieReturn):\tTick:[%d]\n", GetTickCount());
	//
}

//----------------------------
// Э��C->S	: Ping��Ϸ������
//----------------------------
void CS_SendPing() {
	// Lightweight keepalive: send CMD_CP_PING to GateServer
	// This resets m_recvtime on the server, preventing idle disconnection (reason -9).
	// CMD_CP_PING is handled entirely by GateServer — no GameServer round-trip needed.
	// For in-game players, GateServer forwards the ping to GroupServer for latency tracking.
	// For login/character select, the packet still resets the keepalive timer.
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CP_PING);
	g_NetIF->SendPacketMessage(pk);
}

void CS_MapMask(const char* szMapName) {
	WPacket pk = g_NetIF->GetWPacket();

	pk.WriteCmd(CMD_CM_MAP_MASK); // �����ж�
	pk.WriteString(szMapName);
	g_NetIF->SendPacketMessage(pk);

	// log
	LG(g_LogName.GetMainLogName(), "###Send(MapMask):\tTick:[%d]\n", GetTickCount());
	//
}

void CS_RequestTalk(DWORD dwNpcID, BYTE byCmd) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTALK);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_TALKPAGE);
	packet.WriteChar(byCmd);

	g_NetIF->SendPacketMessage(packet);
}

void CS_SelFunction(DWORD dwNpcID, BYTE byPageID, BYTE byIndex) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTALK);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_FUNCITEM);
	packet.WriteChar(byPageID);
	packet.WriteChar(byIndex);

	g_NetIF->SendPacketMessage(packet);
}

void CS_Sale(DWORD dwNpcID, BYTE byIndex, BYTE byCount) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTRADE);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_TRADEITEM);
	packet.WriteChar(ROLE_TRADE_SALE);
	packet.WriteShort(1);
	packet.WriteChar(byIndex);
	packet.WriteChar(byCount);

	g_NetIF->SendPacketMessage(packet);
}

void CS_MultipleSale(DWORD dwNpcID, std::vector<stNetSaleItem> vItems) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTRADE);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_TRADEITEM);
	packet.WriteChar(ROLE_TRADE_SALE);
	packet.WriteShort(vItems.size());
	std::vector<stNetSaleItem>::iterator it = vItems.begin();
	for (it; it != vItems.end(); ++it) {
		packet.WriteChar(it->byIndex);
		packet.WriteChar(it->byCount);
	}
	g_NetIF->SendPacketMessage(packet);
}

void CS_Buy(DWORD dwNpcID, BYTE byItemType, BYTE byIndex1, BYTE byIndex2, BYTE byCount) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTRADE);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_TRADEITEM);
	packet.WriteChar(ROLE_TRADE_BUY);
	packet.WriteChar(byItemType);
	packet.WriteChar(byIndex1);
	packet.WriteChar(byIndex2);
	packet.WriteChar(byCount);

	g_NetIF->SendPacketMessage(packet);
}

void CS_SelectTradeBoat(DWORD dwNpcID, BYTE byIndex) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTRADE);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_TRADEITEM);
	packet.WriteChar(ROLE_TRADE_SELECT_BOAT);
	packet.WriteChar(byIndex);

	g_NetIF->SendPacketMessage(packet);
}

void CS_SaleGoods(DWORD dwNpcID, DWORD dwBoatID, BYTE byIndex, BYTE byCount) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTRADE);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_TRADEITEM);
	packet.WriteChar(ROLE_TRADE_SALE_GOODS);
	packet.WriteLong(dwBoatID);
	packet.WriteChar(byIndex);
	packet.WriteChar(byCount);

	g_NetIF->SendPacketMessage(packet);
}

void CS_BuyGoods(DWORD dwNpcID, DWORD dwBoatID, BYTE byItemType, BYTE byIndex1, BYTE byIndex2, BYTE byCount) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTRADE);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_TRADEITEM);
	packet.WriteChar(ROLE_TRADE_BUY_GOODS);
	packet.WriteLong(dwBoatID);
	packet.WriteChar(byItemType);
	packet.WriteChar(byIndex1);
	packet.WriteChar(byIndex2);
	packet.WriteChar(byCount);

	g_NetIF->SendPacketMessage(packet);
}

void CS_BlackMarketExchangeReq(DWORD dwNpcID, short sSrcID, short sSrcNum, short sTarID, short sTarNum, short sTimeNum, BYTE byIndex) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTRADE);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_BLACKMARKET_EXCHANGE_REQ);
	packet.WriteShort(sTimeNum);
	packet.WriteShort(sSrcID);
	packet.WriteShort(sSrcNum);
	packet.WriteShort(sTarID);
	packet.WriteShort(sTarNum);
	packet.WriteShort(byIndex);

	g_NetIF->SendPacketMessage(packet);
}

void CS_RequestTrade(BYTE byType, DWORD dwCharID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_CHARTRADE_REQUEST);
	packet.WriteChar(byType);
	packet.WriteLong(dwCharID);

	g_NetIF->SendPacketMessage(packet);
}

void CS_AcceptTrade(BYTE byType, DWORD dwCharID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_CHARTRADE_ACCEPT);
	packet.WriteChar(byType);
	packet.WriteLong(dwCharID);

	g_NetIF->SendPacketMessage(packet);
}

void CS_AddItem(BYTE byType, DWORD dwCharID, BYTE byOpType, BYTE byIndex, BYTE byItemIndex, BYTE byCount) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_CHARTRADE_ITEM);
	packet.WriteChar(byType);
	packet.WriteLong(dwCharID);
	packet.WriteChar(byOpType);
	packet.WriteChar(byIndex);
	packet.WriteChar(byItemIndex);
	packet.WriteChar(byCount);

	g_NetIF->SendPacketMessage(packet);
}

void CS_AddMoney(BYTE byType, DWORD dwCharID, BYTE byOpType, long long llMoney) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_CHARTRADE_MONEY);
	packet.WriteChar(byType);
	packet.WriteLong(dwCharID);
	packet.WriteChar(byOpType);
	packet.WriteChar(0);
	packet.WriteLongLong(llMoney);

	g_NetIF->SendPacketMessage(packet);
}

void CS_AddIMP(BYTE byType, DWORD dwCharID, BYTE byOpType, DWORD dwMoney) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_CHARTRADE_MONEY);
	packet.WriteChar(byType);
	packet.WriteLong(dwCharID);
	packet.WriteChar(byOpType);
	packet.WriteChar(1);
	packet.WriteLong(dwMoney);

	g_NetIF->SendPacketMessage(packet);
}

void CS_CancelTrade(BYTE byType, DWORD dwCharID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_CHARTRADE_CANCEL);
	packet.WriteChar(byType);
	packet.WriteLong(dwCharID);

	g_NetIF->SendPacketMessage(packet);
}

void CS_ValidateTradeData(BYTE byType, DWORD dwCharID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_CHARTRADE_VALIDATEDATA);
	packet.WriteChar(byType);
	packet.WriteLong(dwCharID);

	g_NetIF->SendPacketMessage(packet);
}

void CS_ValidateTrade(BYTE byType, DWORD dwCharID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_CHARTRADE_VALIDATE);
	packet.WriteChar(byType);
	packet.WriteLong(dwCharID);

	g_NetIF->SendPacketMessage(packet);
}

void CS_MissionPage(DWORD dwNpcID, BYTE byCmd, BYTE bySelItem, BYTE byParam) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTALK);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_MISSION);
	packet.WriteChar(byCmd);
	packet.WriteChar(bySelItem);
	packet.WriteChar(byParam);

	g_NetIF->SendPacketMessage(packet);
}

void CS_SelMission(DWORD dwNpcID, BYTE byIndex) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTALK);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_MISSION);
	packet.WriteChar(ROLE_MIS_SEL);
	packet.WriteChar(byIndex);

	g_NetIF->SendPacketMessage(packet);
}

void CS_MissionTalk(DWORD dwNpcID, BYTE byCmd) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTALK);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_MISSION);
	packet.WriteChar(ROLE_MIS_TALK);
	packet.WriteShort(CMD_CM_TALKPAGE);
	packet.WriteChar(byCmd);

	g_NetIF->SendPacketMessage(packet);
}

void CS_SelMissionFunc(DWORD dwNpcID, BYTE byPageID, BYTE byIndex) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_REQUESTTALK);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(CMD_CM_MISSION);
	packet.WriteChar(ROLE_MIS_TALK);
	packet.WriteShort(CMD_CM_FUNCITEM);
	packet.WriteChar(byPageID);
	packet.WriteChar(byIndex);

	g_NetIF->SendPacketMessage(packet);
}

void CS_MisLog() {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_MISLOG);

	g_NetIF->SendPacketMessage(packet);
}

void CS_MisLogInfo(WORD wID) {
	if (wID == -1)
		return;
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_MISLOGINFO);
	packet.WriteShort(wID);
	g_NetIF->SendPacketMessage(packet);
}

void CS_MisClear(WORD wID) {
	if (wID == -1)
		return;
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_MISLOG_CLEAR);
	packet.WriteShort(wID);
	g_NetIF->SendPacketMessage(packet);
}

void CS_ForgeItem(BYTE byIndex) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_FORGE);
	packet.WriteChar(byIndex);

	g_NetIF->SendPacketMessage(packet);
}

void CS_CreateBoat(const char szBoat[], char szHeader, char szEngine, char szCannon, char szEquipment) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_CREATE_BOAT);
	packet.WriteString(szBoat);
	packet.WriteChar(szHeader);
	packet.WriteChar(szEngine);
	packet.WriteChar(szCannon);
	packet.WriteChar(szEquipment);

	g_NetIF->SendPacketMessage(packet);
}

void CS_SelectBoatList(DWORD dwNpcID, BYTE byType, BYTE byIndex) {

	if (byType == mission::BERTH_TRADE_LIST) {
		CS_SelectTradeBoat(dwNpcID, byIndex);
	} else {
		WPacket packet = g_NetIF->GetWPacket();
		packet.WriteCmd(CMD_CM_BOAT_SELECT);
		packet.WriteLong(dwNpcID);
		packet.WriteChar(byType);
		packet.WriteChar(byIndex);
		g_NetIF->SendPacketMessage(packet);
	}
}

void CS_SelectBoat(DWORD dwNpcID, BYTE byIndex) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_BOAT_LUANCH);
	packet.WriteLong(dwNpcID);
	packet.WriteChar(byIndex);

	g_NetIF->SendPacketMessage(packet);
}

void CS_SelectBoatBag(DWORD dwNpcID, BYTE byIndex) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_BOAT_BAGSEL);
	packet.WriteLong(dwNpcID);
	packet.WriteChar(byIndex);

	g_NetIF->SendPacketMessage(packet);
}

void CS_UpdateBoat(char szHeader, char szEngine, char szCannon, char szEquipment) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_UPDATEBOAT_PART);
	packet.WriteChar(szHeader);
	packet.WriteChar(szEngine);
	packet.WriteChar(szCannon);
	packet.WriteChar(szEquipment);

	g_NetIF->SendPacketMessage(packet);
}

void CS_CancelBoat() {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_BOAT_CANCEL);

	g_NetIF->SendPacketMessage(packet);
}

void CS_GetBoatInfo() {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_BOAT_GETINFO);

	g_NetIF->SendPacketMessage(packet);
}

void CS_EntityEvent(DWORD dwEntityID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_ENTITY_EVENT);
	packet.WriteLong(dwEntityID);

	g_NetIF->SendPacketMessage(packet);

	const char* szLogName = g_LogName.GetMainLogName();
	LG(szLogName, "###Send(Event-Entyty):\tTick:[%d]\n", GetTickCount());
	LG(szLogName, "\n");
}

void CS_StallInfo(const char szName[], mission::NET_STALL_ALLDATA& Data) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_STALL_ALLDATA);
	packet.WriteString(szName);
	packet.WriteChar(Data.byNum);
	for (BYTE i = 0; i < Data.byNum; ++i) {
		packet.WriteChar(Data.Info[i].byGrid);
		packet.WriteLongLong(Data.Info[i].llMoney);  // Changed from WriteLong to WriteLongLong for 64-bit
		packet.WriteShort(Data.Info[i].byCount);  // Changed from WriteChar to WriteShort to match server
		packet.WriteChar(Data.Info[i].byIndex);
	}
	g_NetIF->SendPacketMessage(packet);
}

void CS_StallOpen(DWORD dwCharID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_STALL_OPEN);
	packet.WriteLong(dwCharID);
	g_NetIF->SendPacketMessage(packet);
}

void CS_StallBuy(DWORD dwCharID, BYTE byIndex, USHORT byCount, int gridID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_STALL_BUY);
	packet.WriteLong(dwCharID);
	packet.WriteChar(byIndex);
	packet.WriteShort(byCount);  // Changed from WriteChar to WriteShort to match server
	packet.WriteChar(gridID);
	g_NetIF->SendPacketMessage(packet);
}

void CS_StallClose() {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_STALL_CLOSE);
	g_NetIF->SendPacketMessage(packet);
}

// add by jilinlee 2007/4/20/////////////////////
void CS_ReadBookStart() {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_READBOOK_START);
	g_NetIF->SendPacketMessage(packet);
}

void CS_ReadBookClose() {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_READBOOK_CLOSE);
	g_NetIF->SendPacketMessage(packet);
}
///////////////////////////////////////////////

void CS_UpdateHair(stNetUpdateHair& stData) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_UPDATEHAIR);
	packet.WriteShort(stData.sScriptID);
	for (short i = 0; i < 4; i++) {
		packet.WriteShort(stData.sGridLoc[i]);
	}
	g_NetIF->SendPacketMessage(packet);
}

void CS_TeamFightAsk(unsigned long ulWorldID, long lHandle, char chType) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_TEAM_FIGHT_ASK);
	packet.WriteChar(chType);
	packet.WriteLong(ulWorldID);
	packet.WriteLong(lHandle);
	g_NetIF->SendPacketMessage(packet);
}

void CS_TeamFightAnswer(bool bAccess) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_TEAM_FIGHT_ASR);
	packet.WriteChar(bAccess ? 1 : 0);
	g_NetIF->SendPacketMessage(packet);
}

// lRepairmanID,lRepairmanHandle ����Ա
// chPosType��1��װ������2��������
// chPosID����Ӧλ�õı��
void CS_ItemRepairAsk(long lRepairmanID, long lRepairmanHandle, char chPosType, char chPosID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_ITEM_REPAIR_ASK);
	packet.WriteLong(lRepairmanID);
	packet.WriteLong(lRepairmanHandle);
	packet.WriteChar(chPosType);
	packet.WriteChar(chPosID);
	g_NetIF->SendPacketMessage(packet);
}

void CS_ItemRepairAnswer(bool bAccess) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_ITEM_REPAIR_ASR);
	packet.WriteChar(bAccess ? 1 : 0);
	g_NetIF->SendPacketMessage(packet);
}

void CS_ItemForgeAsk(bool bSure, stNetItemForgeAsk* pSForge) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_ITEM_FORGE_ASK);
	if (!bSure) {
		packet.WriteChar(0);
	} else {
		packet.WriteChar(1);
		packet.WriteChar(pSForge->chType);
		for (int i = 0; i < defMAX_FORGE_GROUP_NUM; i++) {
			packet.WriteShort(pSForge->SGroup[i].sCellNum);
			for (short j = 0; j < pSForge->SGroup[i].sCellNum; j++) {
				packet.WriteShort(pSForge->SGroup[i].pCell->sPosID);
				packet.WriteShort(pSForge->SGroup[i].pCell->sNum);
			}
		}
	}

	g_NetIF->SendPacketMessage(packet);
}

void CS_ItemForgeAnswer(bool bAccess) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_ITEM_FORGE_ASR);
	packet.WriteChar(bAccess ? 1 : 0);
	g_NetIF->SendPacketMessage(packet);
}

// Request server validation for placing item in forge/crafting slot
void CS_ValidateSlotItem(char chFormType, char chSlotIndex, short sGridID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_VALIDATE_SLOT_ITEM);
	packet.WriteChar(chFormType);
	packet.WriteChar(chSlotIndex);
	packet.WriteShort(sGridID);
	g_NetIF->SendPacketMessage(packet);
}

// Add by lark.li 20080514 begin
void CS_ItemLotteryAnswer(bool bAccess) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_ITEM_LOTTERY_ASR);
	packet.WriteChar(bAccess ? 1 : 0);
	g_NetIF->SendPacketMessage(packet);
}

void CS_ItemLotteryAsk(bool bSure, stNetItemLotteryAsk* pSLottery) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_ITEM_LOTTERY_ASK);
	if (!bSure) {
		packet.WriteChar(0);
	} else {
		packet.WriteChar(1);

		for (int i = 0; i < defMAX_LOTTERY_GROUP_NUM; i++) {
			packet.WriteShort(pSLottery->SGroup[i].sCellNum);
			for (short j = 0; j < pSLottery->SGroup[i].sCellNum; j++) {
				packet.WriteShort(pSLottery->SGroup[i].pCell->sPosID);
				packet.WriteShort(pSLottery->SGroup[i].pCell->sNum);
			}
		}
	}

	g_NetIF->SendPacketMessage(packet);
}
// End
// Add by sunny.sun 20080726
void CS_ItemAmphitheaterAsk(bool bSure, int ReID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_ITEM_AMPHITHEATER_ASK);
	if (!bSure) {
		packet.WriteChar(0);
	} else {
		packet.WriteChar(1);
		packet.WriteShort(ReID);
	}
	g_NetIF->SendPacketMessage(packet);
}

void CS_ItemForgeAsk(bool bSure, int nType, int arPacketPos[], int nPosCount) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_ITEM_FORGE_ASK);

	if (!bSure) {
		packet.WriteChar(0);
	} else {
		packet.WriteChar(1);
		packet.WriteChar((char)(nType)); // ����������

		for (int i = 0; i < defMAX_FORGE_GROUP_NUM; ++i) {
			if (i < nPosCount) {
				packet.WriteShort((short)1);	   // cellnum
				packet.WriteShort(arPacketPos[i]); // ����λ��
				packet.WriteShort((short)1);	   // ����
			} else {
				packet.WriteShort((short)0); // cellnum
				packet.WriteShort((short)0); // ����λ��
				packet.WriteShort((short)0); // ����
			}
		}
	}

	g_NetIF->SendPacketMessage(packet);
}

// ���̳�
void CS_StoreOpenAsk(const char szPassword[]) {
	char szMD5[33] = {0};
	md5string(szPassword, szMD5);

	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_STORE_OPEN_ASK);
	WritePacketSequenceEncrypted(packet, g_NetIF->m_AESKey, (uint8_t*)szMD5, strlen(szMD5) + 1);
	g_NetIF->SendPacketMessage(packet);
}

void CS_StoreClose() {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_STORE_CLOSE);
	g_NetIF->SendPacketMessage(packet);
}

void CS_StoreListAsk(long lClsID, short sPage, short sNum) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_STORE_LIST_ASK);
	packet.WriteLong(lClsID);
	packet.WriteShort(sPage);
	packet.WriteShort(sNum);
	g_NetIF->SendPacketMessage(packet);
}

void CS_StoreBuyAsk(long lComID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_STORE_BUY_ASK);
	packet.WriteLong(lComID);
	g_NetIF->SendPacketMessage(packet);
}

void CS_StoreChangeAsk(long lNum) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_STORE_CHANGE_ASK);
	packet.WriteLong(lNum);
	g_NetIF->SendPacketMessage(packet);
}

void CS_StoreQuery(long lNum) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_STORE_QUERY);
	packet.WriteLong(lNum);
	g_NetIF->SendPacketMessage(packet);
}

// void CS_StoreVIP(short sVipID, short sMonth)
//{
//	WPacket packet = g_NetIF->GetWPacket();
//	packet.WriteCmd( CMD_CM_STORE_VIP );
//	packet.WriteShort(sVipID);
//	packet.WriteShort(sMonth);
//	g_NetIF->SendPacketMessage( packet );
// }

void CS_ReportWG(const char szInfo[]) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CP_REPORT_WG);
	packet.WriteString(szInfo);
	g_NetIF->SendPacketMessage(packet);
}

void CS_TigerStart(DWORD dwNpcID, short sSel1, short sSel2, short sSel3) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_TIGER_START);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(sSel1);
	packet.WriteShort(sSel2);
	packet.WriteShort(sSel3);
	g_NetIF->SendPacketMessage(packet);
}

void CS_TigerStop(DWORD dwNpcID, short sNum) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_TIGER_STOP);
	packet.WriteLong(dwNpcID);
	packet.WriteShort(sNum);
	g_NetIF->SendPacketMessage(packet);
}

void CS_VolunteerList(short sPage, short sNum) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_VOLUNTER_LIST);
	packet.WriteShort(sPage);
	packet.WriteShort(sNum);
	g_NetIF->SendPacketMessage(packet);
}

void CS_VolunteerAdd() {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_VOLUNTER_ADD);
	g_NetIF->SendPacketMessage(packet);
}

void CS_VolunteerDel() {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_VOLUNTER_DEL);
	g_NetIF->SendPacketMessage(packet);
}

void CS_VolunteerSel(const char* szName) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_VOLUNTER_SEL);
	packet.WriteString(szName);
	g_NetIF->SendPacketMessage(packet);
}

void CS_VolunteerOpen(short sNum) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_VOLUNTER_OPEN);
	packet.WriteShort(sNum);
	g_NetIF->SendPacketMessage(packet);
}

void CS_VolunteerAsr(BOOL bRet, const char* szName) {
	short sRet = bRet ? 1 : 0;
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_VOLUNTER_ASR);
	packet.WriteShort(sRet);
	packet.WriteString(szName);
	g_NetIF->SendPacketMessage(packet);
}

void CS_SyncKitbagTemp() {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_KITBAGTEMP_SYNC);
	g_NetIF->SendPacketMessage(packet);
}

// Add by lark.li 20080707 begin
void CS_CaptainConfirmAsr(short sRet, DWORD dwTeamID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_CAPTAIN_CONFIRM_ASR);
	packet.WriteShort(sRet);
	packet.WriteLong(dwTeamID);
	g_NetIF->SendPacketMessage(packet);
}
// End

void CS_MasterInvite(const char* szName, DWORD dwCharID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_MASTER_INVITE);
	packet.WriteString(szName);
	packet.WriteLong(dwCharID);
	g_NetIF->SendPacketMessage(packet);
}

void CS_MasterAsr(short sRet, const char* szName, DWORD dwCharID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_MASTER_ASR);
	packet.WriteShort(sRet);
	packet.WriteString(szName);
	packet.WriteLong(dwCharID);
	g_NetIF->SendPacketMessage(packet);
}

void CS_PrenticeInvite(const char* szName, DWORD dwCharID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_PRENTICE_INVITE);
	packet.WriteString(szName);
	packet.WriteLong(dwCharID);
	g_NetIF->SendPacketMessage(packet);
}

void CS_PrenticeAsr(short sRet, const char* szName, DWORD dwCharID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_PRENTICE_ASR);
	packet.WriteShort(sRet);
	packet.WriteString(szName);
	packet.WriteLong(dwCharID);
	g_NetIF->SendPacketMessage(packet);
}

void CS_MasterDel(const char* szName, uLong ulChaID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_MASTER_DEL);
	packet.WriteString(szName);
	packet.WriteLong(ulChaID);
	g_NetIF->SendPacketMessage(packet);
}

void CS_PrenticeDel(const char* szName, uLong ulChaID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_PRENTICE_DEL);
	packet.WriteString(szName);
	packet.WriteLong(ulChaID);
	g_NetIF->SendPacketMessage(packet);
}

void CP_Master_Refresh_Info(unsigned long chaid) {
	WPacket l_wpk = g_NetIF->GetWPacket();
	l_wpk.WriteCmd(CMD_CP_MASTER_REFRESH_INFO);
	l_wpk.WriteLong(chaid);
	g_NetIF->SendPacketMessage(l_wpk);
}

void CP_Prentice_Refresh_Info(unsigned long chaid) {
	WPacket l_wpk = g_NetIF->GetWPacket();
	l_wpk.WriteCmd(CMD_CP_PRENTICE_REFRESH_INFO);
	l_wpk.WriteLong(chaid);
	g_NetIF->SendPacketMessage(l_wpk);
}

void CS_Say2Camp(const char* szContent) {
	WPacket l_wpk = g_NetIF->GetWPacket();
	l_wpk.WriteCmd(CMD_CM_SAY2CAMP);
	l_wpk.WriteString(szContent);
	g_NetIF->SendPacketMessage(l_wpk);
}

void CS_GMSend(DWORD dwNPCID, const char* szTitle, const char* szContent) {
	WPacket l_wpk = g_NetIF->GetWPacket();
	l_wpk.WriteCmd(CMD_CM_GM_SEND);
	l_wpk.WriteLong(dwNPCID);
	l_wpk.WriteString(szTitle);
	l_wpk.WriteString(szContent);
	g_NetIF->SendPacketMessage(l_wpk);
}

void CS_GMRecv(DWORD dwNPCID) {
	WPacket l_wpk = g_NetIF->GetWPacket();
	l_wpk.WriteCmd(CMD_CM_GM_RECV);
	l_wpk.WriteLong(dwNPCID);
	g_NetIF->SendPacketMessage(l_wpk);
}

// void CS_PKCtrl(bool bCanPK)
//{
//	WPacket l_wpk	=g_NetIF->GetWPacket();
//	l_wpk.WriteCmd(CMD_CM_PK_CTRL);
//	l_wpk.WriteChar(bCanPK? 1 : 0);
//	g_NetIF->SendPacketMessage(l_wpk);
// }

void CS_CheatCheck(cChar* answer) {
	WPacket l_wpk = g_NetIF->GetWPacket();
	l_wpk.WriteCmd(CMD_CM_CHEAT_CHECK);
	l_wpk.WriteString(answer);
	g_NetIF->SendPacketMessage(l_wpk);
}

//  ���������ǣ�ˢ������
// void CS_PKSilverSort(DWORD dwNPCID, short sItemPos)
//{
//    WPacket packet = g_NetIF->GetWPacket();
//    packet.WriteCmd(CMD_CM_GARNER2_REORDER);
//    packet.WriteLong(dwNPCID);
//    packet.WriteShort(sItemPos);
//    g_NetIF->SendPacketMessage(packet);
//}

void CS_LifeSkill(long type, DWORD dwNPCID) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_LIFESKILL_ASR);
	packet.WriteLong(type);
	packet.WriteLong(dwNPCID);
	g_NetIF->SendPacketMessage(packet);
}

void CS_Compose(DWORD dwNPCID, int* iPos, int iCount, bool asr /* = false */) {
	WPacket packet = g_NetIF->GetWPacket();
	if (asr) {
		packet.WriteCmd(CMD_CM_LIFESKILL_ASR);
	} else {
		packet.WriteCmd(CMD_CM_LIFESKILL_ASK);
	}
	packet.WriteLong(0); //  ��֧
	packet.WriteLong(dwNPCID);
	for (int i = 0; i < iCount; i++) {
		packet.WriteShort((short)iPos[i]);
	}
	g_NetIF->SendPacketMessage(packet);
}

void CS_Break(DWORD dwNPCID, int* iPos, int iCount, bool asr /* = false */) {
	WPacket packet = g_NetIF->GetWPacket();
	if (asr) {
		packet.WriteCmd(CMD_CM_LIFESKILL_ASR);
	} else {
		packet.WriteCmd(CMD_CM_LIFESKILL_ASK);
	}
	packet.WriteLong(1); //  ��֧
	packet.WriteLong(dwNPCID);
	for (int i = 0; i < iCount; i++) {
		packet.WriteShort((short)iPos[i]);
	}
	g_NetIF->SendPacketMessage(packet);
}

void CS_Found(DWORD dwNPCID, int* iPos, int iCount, short big, bool asr /* = false */) {
	WPacket packet = g_NetIF->GetWPacket();
	if (asr) {
		packet.WriteCmd(CMD_CM_LIFESKILL_ASR);
	} else {
		packet.WriteCmd(CMD_CM_LIFESKILL_ASK);
	}
	packet.WriteLong(2); //  ��֧
	packet.WriteLong(dwNPCID);
	for (int i = 0; i < iCount; i++) {
		packet.WriteShort((short)iPos[i]);
	}
	packet.WriteShort(big);
	g_NetIF->SendPacketMessage(packet);
}

void CS_Cooking(DWORD dwNPCID, int* iPos, int iCount, short percent, bool asr /* = false */) {
	WPacket packet = g_NetIF->GetWPacket();
	if (asr) {
		packet.WriteCmd(CMD_CM_LIFESKILL_ASR);
	} else {
		packet.WriteCmd(CMD_CM_LIFESKILL_ASK);
	}
	packet.WriteLong(3); //  ��֧
	packet.WriteLong(dwNPCID);
	for (int i = 0; i < iCount; i++) {
		packet.WriteShort((short)iPos[i]);
	}
	packet.WriteShort(percent);
	g_NetIF->SendPacketMessage(packet);
}

void CS_UnlockCharacter() {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_ITEM_FORGE_CANACTION);
	packet.WriteShort(0);
	g_NetIF->SendPacketMessage(packet);
}
// add by ALLEN 2007-10-19
void CS_AutionBidup(DWORD dwNPCID, short sItemID, uLong price) {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_BIDUP);
	packet.WriteLong(dwNPCID);
	packet.WriteShort(sItemID);
	packet.WriteLong(price);
	g_NetIF->SendPacketMessage(packet);
}

void CS_AntiIndulgence_Close() {
	WPacket packet = g_NetIF->GetWPacket();
	packet.WriteCmd(CMD_CM_ANTIINDULGENCE);
	g_NetIF->SendPacketMessage(packet);
}

void CS_DropLock(int slot) {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_ITEM_LOCK_ASK);
	pk.WriteChar(slot);
	g_NetIF->SendPacketMessage(pk);
}

void CS_UnlockItem(const char szPassword[], int slot) {
	char szMD5[33] = {0};
	md5string(szPassword, szMD5);

	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_ITEM_UNLOCK_ASK);
	WritePacketSequenceEncrypted(pk, g_NetIF->m_AESKey, (uint8_t*)szMD5, strlen(szMD5) + 1);
	pk.WriteChar(slot);
	g_NetIF->SendPacketMessage(pk);
}

void CS_SendGameRequest(const char szPassword[]) {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_GAME_REQUEST_PIN);
	pk.WriteString(szPassword);
	g_NetIF->SendPacketMessage(pk);
}

void CS_SetGuildPerms(DWORD ID, uLong Perms) {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_GUILD_PERM);
	pk.WriteLong(ID);
	pk.WriteLong(Perms);
	g_NetIF->SendPacketMessage(pk);
}

void CS_RequestDropRate() {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_REQUEST_DROP_RATE);
	g_NetIF->SendPacketMessage(pk);
}

void CS_RequestExpRate() {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_REQUEST_EXP_RATE);
	g_NetIF->SendPacketMessage(pk);
}

void CS_RequestBattlePoint() {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_GET_PLAYER_BATTLE_POINT);
	g_NetIF->SendPacketMessage(pk);
}

void CS_RequestChestPreview(int chestItemID) {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_REQUEST_CHEST_PREVIEW);
	pk.WriteLong(chestItemID);
	g_NetIF->SendPacketMessage(pk);
}

void CS_BossTimerRequest() {
	WPacket pk = g_NetIF->GetWPacket();
	pk.WriteCmd(CMD_CM_BOSSTIMER_REQUEST);
	g_NetIF->SendPacketMessage(pk);
}
