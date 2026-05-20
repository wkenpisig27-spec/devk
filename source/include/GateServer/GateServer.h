#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>
#include <dstring.h>
#include <datasocket.h>
#include <threadpool.h>
#include <commrpc.h>
#include <point.h>
#include <inifile.h>
#include <gamecommon.h>
#include <prealloc.h>
#include <algo.h>
#include <PacketEncryption.h>
#include <memory>
#include <stdlib.h>
#include <unordered_map>
#include <unordered_set>

#include "i18n.h" //Add by lark.li 20080130
#include "pi_Memory.h"
#include "pi_Alloc.h"
#include "NetRetCode.h"

#include <iostream>
#include <map>
#include <vector>
#include <atomic>
#include <shared_mutex>
#include <fstream>

using namespace std;
using namespace dbc;

// Ensure uLong64 is defined
typedef unsigned long long uLong64;

// ?? Client ????==========
class Player;
class ToClient : public TcpServerApp, public RPCMGR {
	friend class TransmitCall;

public:
	ToClient(char const* fname, ThreadPool* proc, ThreadPool* comm);
	~ToClient();

	void post_mapcrash_msg(Player* ply);
	std::string GetDisconnectErrText(int reason) const;
	void SetMaxCon(uShort maxcon) { m_maxcon = maxcon; }
	uShort GetMaxCon() { return m_maxcon; }
	void CM_LOGIN(DataSocket* datasock, RPacket& recvbuf);
	WPacket CM_LOGOUT(DataSocket* datasock, RPacket& recvbuf);
	void CM_BGNPLAY(DataSocket* datasock, RPacket& recvbuf);
	void CM_ENDPLAY(DataSocket* datasock, RPacket& recvbuf);
	void CM_NEWCHA(DataSocket* datasock, RPacket& recvbuf);
	void CM_REGISTER(DataSocket* datasock, RPacket& recvbuf);
	void CP_CHANGEPASS(DataSocket* datasock, RPacket& recvbuf);
	void CM_DELCHA(DataSocket* datasock, RPacket& recvbuf);
	void CM_CREATE_PASSWORD2(DataSocket* datasock, RPacket& recvbuf);
	void CM_UPDATE_PASSWORD2(DataSocket* datasock, RPacket& recvbuf);
	void CM_RSA_HANDSHAKE1(DataSocket* datasock, RPacket& recvbuf);
	void CM_OPERGUILDBANK(DataSocket* datasock, RPacket& recvbuf);
	void TC_DISCONNECT(DataSocket* datasock, int reason = DS_DISCONN, int remain = 4000);

	uShort GetVersion() { return m_version; }
	int GetCallTotal() { return m_calltotal; }
	uShort GetHandshakeTimeout() const { return m_handshakeTimeout; }



private:
	bool DoCommand(DataSocket* datasock, cChar* cmdline);
	virtual void TaskDispatcher(Task* task) final;
	virtual bool OnConnect(DataSocket* datasock); // ???:true-????,false-?????
	virtual void OnConnected(DataSocket* datasock);
	virtual void OnDisconnect(DataSocket* datasock, int reason);
	virtual void OnProcessData(DataSocket* datasock, RPacket& recvbuf);
	virtual WPacket OnServeCall(DataSocket* datasock, RPacket& in_para);
	virtual bool OnSendBlock(DataSocket* datasock) { return false; }

	// communication encryption
	virtual void OnEncrypt(DataSocket* datasock, char* ciphertext, const char* text, uLong& len);
	virtual void OnDecrypt(DataSocket* datasock, char* ciphertext, uLong& len);

	void ReRouteToGameServer(DataSocket* datasock, RPacket& recvbuf);
	void ReRouteToGroupServer(DataSocket* datasock, RPacket& recvbuf);

	InterLockedLong m_atexit, m_calltotal;
	volatile uShort m_maxcon;
	uShort m_version;
	int _comm_enc; // communication encryption
	uShort m_handshakeTimeout{10};           // RSA handshake timeout in seconds
	
	// Proxy Protocol support - CRITICAL for SmartProxy integration
	bool m_proxyProtocolEnabled{false};      // Enable Proxy Protocol v1 parsing
	bool ParseProxyProtocol(DataSocket* datasock); // Parse and extract real IP from SmartProxy
	
	// Optional whitelist for direct testing without SmartProxy
	bool m_whitelistEnabled{false};
	std::vector<std::string> m_whitelist;
	
	// Server has only 1 private+public key pair.
	Botan::RSA_PrivateKey* m_serverPrivateKey;
	IMPLEMENT_CDELETE(ToClient)
};

// ?? GameServer ????===============
class ToGameServer;

#define MAX_MAP 100
#define MAX_GAM 100
struct GameServer : public PreAllocStru {
	friend class PreAllocHeap<GameServer>;

private:
	GameServer(uLong Size) : PreAllocStru(Size),
							 m_datasock(nullptr), next(nullptr) {}
	~GameServer() {
		if (m_datasock != nullptr) {
			m_datasock->SetPointer(nullptr);
			m_datasock = nullptr;
		}
	}
	GameServer(GameServer const&) : PreAllocStru(1) {}
	GameServer& operator=(GameServer const&) {}
	void Initially();
	void Finally();

public:
	void EnterMap(Player* ply, uLong actid, uLong dbid, uLong worldid, cChar* map, Long lMapCpyNO, uLong x, uLong y, char entertype, short swiner); // ??chaid?????????map????(x,y) winer????????????
public:
	InterLockedLong m_plynum{};
	std::string gamename;
	std::string ip;
	uShort port{};
	DataSocket* m_datasock{};
	GameServer* next{};
	std::string maplist[MAX_MAP];
	uShort mapcnt{};
};

class ToGameServer : public TcpServerApp, public RPCMGR {
	friend class ToGroupServer;

public:
	ToGameServer(char const* fname, ThreadPool* proc, ThreadPool* comm);
	~ToGameServer();

	GameServer* find(cChar* mapname);
	GameServer* GetGameList() { return _game_list; }

private:
	virtual void TaskDispatcher(Task* task) final;
	virtual bool OnConnect(DataSocket* datasock); // ???:true-????,false-?????
	virtual void OnDisconnect(DataSocket* datasock, int reason);
	virtual WPacket OnServeCall(DataSocket* datasock, RPacket& in_para);
	virtual void OnProcessData(DataSocket* datasock, RPacket& recvbuf);

	PreAllocHeap<GameServer> _game_heap;				   // GameServer ?????
	void MT_LOGIN(DataSocket* datasock, RPacket& recvbuf); // GameServer ?? GateServer
	void MT_SWITCHMAP(DataSocket* datasock, RPacket& recvbuf);
	void MT_MAPENTRY(DataSocket* datasock, RPacket& recvbuf);
	void MC_ENTERMAP(DataSocket* datasock, RPacket& recvbuf);
	void MT_KICKUSER(DataSocket* datasock, RPacket& recvbuf);

	GameServer* _game_list; // ?? GameServer ???????
	short _game_num;
	void _add_game(GameServer* game);
	bool _exist_game(char const* game);
	void _del_game(GameServer* game);
	std::map<std::string, GameServer*> _map_game; // �ӵ�ͼ����Ӧ GameServer ��������
	Mutex _mut_game;

	IMPLEMENT_CDELETE(ToGameServer)
};

// ?? GroupServer ????==========
class ToGroupServer;
class ConnectGroupServer : public Task {
public:
	ConnectGroupServer(ToGroupServer* tgts) {
		_tgps = tgts;
		_timeout = 3000;
	}

private:
	virtual int Process();
	virtual Task* Lastly();

	ToGroupServer* _tgps;
	DWORD _timeout;
};

struct GroupServer {
	GroupServer() : datasock(nullptr), next(nullptr) {}
	std::string ip;
	uShort port;
	DataSocket* datasock;
	GroupServer* next;
};

class ToGroupServer : public TcpClientApp, public RPCMGR {
	friend class ConnectGroupServer;
	friend void ToGameServer::MT_LOGIN(DataSocket* datasock, RPacket& rpk);

public:
	ToGroupServer(char const* fname, ThreadPool* proc, ThreadPool* comm);
	~ToGroupServer();

	DataSocket* get_datasock() const { return _gs.datasock; }
	RPacket get_playerlist();

	int GetCallTotal() { return m_calltotal; }

	// Add by lark.li 20081119 begin
	bool IsSync() { return m_bSync; }
	void SetSync(bool sync = true) { m_bSync = sync; }
	// End

	// ???
	bool IsReady() { return (!m_bSync && _connected); }

private:
	void TaskDispatcher(Task* task) final;
	virtual bool OnConnect(DataSocket* datasock); // ???:true-????,false-?????
	virtual void OnDisconnect(DataSocket* datasock, int reason);
	virtual void OnProcessData(DataSocket* datasock, RPacket& recvbuf);
	virtual WPacket OnServeCall(DataSocket* datasock, RPacket& in_para);

	InterLockedLong m_atexit, m_calltotal;

	std::string _myself; // GateServer?????
	GroupServer _gs;
	bool volatile _connected{false};

	// Add by lark.li 20081119 begin
	bool volatile m_bSync{false};
	// End

	IMPLEMENT_CDELETE(ToGroupServer)
};

// GateServer ????=====================
struct Player : public PreAllocStru, public RunBiDirectItem<Player> {
	friend class PreAllocHeap<Player>;
	friend class DelayLogout;

	// Generation counter for pointer validation
	static std::atomic<uint32_t> s_globalGeneration;
	uint32_t m_generation{0};

public:
	uint32_t GetGeneration() const { return m_generation; }
	bool InitReference(DataSocket* datasock);
	void Free();

	// Add by lark.li 20081119 begin
	bool BeginRun();
	bool EndRun();
	// End

	char m_password[ROLE_MAXSIZE_PASSWORD2 + 1];

	void SendSysInfo(std::string_view message) const;

	Botan::RSA_PrivateKey* m_clientPrivateKey;
	Botan::Public_Key* m_clientPublicKey{};
	char m_nonce[12];
	AES_KEY m_AESKey;
	AES_IV m_IV;

	// Stateful AES-128/CTR cipher objects — one per traffic direction.
	// Initialized once in Initially() so keystream advances continuously
	// across all packets. Never call start() again after init; calling it
	// would reset the counter to 0, producing the same keystream as packet 1.
	// m_mtx_enc guards m_enc_cipher: multiple threads can call SendData for
	// the same player concurrently (GameServer relay + GateServer heartbeat),
	// and process() on a shared cipher is not thread-safe.
	std::unique_ptr<Botan::Cipher_Mode> m_enc_cipher; // server → client
	std::unique_ptr<Botan::Cipher_Mode> m_dec_cipher; // client → server
	Mutex m_mtx_enc; // protects m_enc_cipher only (dec is single-threaded)

	uLong volatile m_actid{};
	uLong volatile m_loginID{};
	uLong volatile m_dbid{};	// ????????ID
	uLong volatile m_worldid{}; // ?????????ID
	uLong volatile m_pingtime{};
	uLong volatile m_connectTime{}; // Time when player connected (for online time tracking)
	char m_acctname[129]{}; // Account name for GUI display
	uInt volatile comm_key_len{};	   // ??????
	char comm_textkey[12];			   // GateServer ? Client ?????????
	InterLockedLongLong gm_addr{};	   // GameServer ? Player ?????
	InterLockedLongLong gp_addr{};	   // GroupServer ? Player ?????
	DataSocket* volatile m_datasock{}; // ? Player ? GateServer <-> Client ??
	GameServer* volatile game{};	   // ? Player ????? GameServer ????
	volatile bool enc{};			   // ????????

	// ????
	uLong volatile m_lestoptick{};
	bool volatile m_estop{};
	short volatile m_sGarnerWiner{};

	// Status fields (was anonymous struct, but GCC disallows members with ctor/dtor in anonymous aggregates)
	Mutex m_mtxstat;		// 0:??m_status;
	volatile char m_status; // 0:??;1.?????;2.????.
	volatile char m_exit;	// 0:??;1.???????????,????????;2.???????????,????????

	dstring m_chamap;
	void SetMapName(dbc::cChar* cszName) { m_chamap = cszName; }
	const char* GetMapName(void) { return m_chamap.c_str(); }

private:
	Player(uLong Size) : PreAllocStru(Size),
						 m_datasock(nullptr), game(nullptr), gm_addr(0), gp_addr(0), m_dbid(0), m_worldid(0), m_status(0), m_sGarnerWiner(0) {
		m_mtxstat.Create(false);
	}
	~Player() {
		if (m_datasock != nullptr) {
			m_datasock->SetPointer(nullptr);
			m_datasock = nullptr;
		}
	}
	Player(Player const&) : PreAllocStru(1) {}
	Player& operator=(Player const&) {}
	void Initially();
	void Finally();
};
class TransmitCall : public PreAllocTask {
public:
	TransmitCall(uLong size) : PreAllocTask(size){};
	void Init(DataSocket* datasock, RPacket& recvbuf) {
		m_datasock = datasock;
		m_recvbuf = recvbuf;
	}
	int Process();

	virtual void Finally() {
		m_recvbuf = nullptr;
	}

	DataSocket* m_datasock;
	RPacket m_recvbuf;
};
class GateServer {
public:
	GateServer(char const* fname);
	~GateServer();
	void RunLoop(); // ???
	ThreadPool *m_clproc, *m_clcomm, *m_gpcomm, *m_gpproc, *m_gmcomm;
	ToGroupServer* gp_conn; // ?GroupServer?????(??????)
	ToGameServer* gm_conn;	// ?GameServer?????(??)
	ToClient* cli_conn;		// ?Client?????(??)
	Mutex _mtxother;

	PreAllocHeap<Player> player_heap; // ?????

	// Add by lark.li 20081119 begin
	RunBiDirectChain<Player> m_plylst; // ????
	// End

	PreAllocHeap<TransmitCall> m_tch;

	// Player pointer validation system
	mutable std::shared_mutex m_playerRegistryMutex;
	std::unordered_map<uintptr_t, uint32_t> m_playerRegistry;
	
	void RegisterPlayer(Player* ply);
	void UnregisterPlayer(Player* ply);
	bool ValidatePlayerPointer(Player* ply, long long expectedGpAddr = 0);
	bool IsPlayerRegistered(Player* ply) const;

	IMPLEMENT_CDELETE(GateServer)
};
extern GateServer* g_gtsvr;
extern bool volatile g_appexit;

class CUdpManage;
class CUdpServer;
class GateServerApp {
public:
	GateServerApp();

	void ServiceStart();
	void ServiceStop();
	virtual bool CanPaused() const { return true; }; // ???????????
};

// LogStream extern declarations removed - using LG() instead
extern GateServerApp* g_app;

extern HANDLE hConsole;
extern InterLockedLong g_exit;

#ifdef PKO_PLATFORM_WINDOWS
#define C_PRINT(s, ...)                    \
	SetConsoleTextAttribute(hConsole, 14); \
	printf(s, __VA_ARGS__);                \
	SetConsoleTextAttribute(hConsole, 10);

#define C_TITLE(s)                                                             \
	char szPID[32];                                                            \
	_snprintf_s(szPID, sizeof(szPID), _TRUNCATE, "%d", GetCurrentProcessId()); \
	std::string strConsoleT;                                                   \
	strConsoleT += "[PID:";                                                    \
	strConsoleT += szPID;                                                      \
	strConsoleT += "]";                                                        \
	strConsoleT += s;                                                          \
	SetConsoleTitle(strConsoleT.c_str());
#else
#define C_PRINT(s, ...) printf(s, ##__VA_ARGS__)
#define C_TITLE(s) do { printf("[PID:%d] %s\n", (int)getpid(), s); } while(0)
#endif

extern CResourceBundleManage g_ResourceBundleManage; // Add by lark.li 20080130
