#include <iostream>
#include <signal.h>
#include <condition_variable>
#include "timer.h"
#include "gateserver.h"
#include "udpmanage.h"
#include "log.h"

using namespace std;

// #pragma comment( lib, "../../../status/lib/Status.lib" )

dbc::uLong NetBuffer[] = {100, 10, 0};
bool g_logautobak = true;
// LogStream declarations removed - using LG() instead

dbc::InterLockedLong g_exit = 0;
dbc::InterLockedLong g_ref = 0;

// NOTE(Ogge): To prevent nullptr referencing of g_gtsvr by ToClient && ToGameServer && ToGroupServer
std::condition_variable global_gate_ready_cv;
std::mutex global_gate_ready_mutex;
bool is_global_gate_ready{false};

dbc::TimerMgr g_timermgr;
//=========Timer==============
#ifdef PKO_PLATFORM_WINDOWS
extern "C" {
WINBASEAPI HWND APIENTRY GetConsoleWindow(VOID);
}
class DisableCloseButton : public dbc::Timer {
public:
	DisableCloseButton(dbc::uLong interval) : dbc::Timer(interval), m_hMenu(0) {
		HWND hWnd = ::GetConsoleWindow();
		m_hMenu = GetSystemMenu(hWnd, FALSE);
	}

private:
	~DisableCloseButton() {
	}
	void Process() {
		dbc::RefArmor l(g_ref);
		if (!g_exit && m_hMenu) {
			EnableMenuItem(m_hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		}
	}
	HMENU m_hMenu;
};
#else
// On Linux, no console close button to disable — no-op timer
class DisableCloseButton : public dbc::Timer {
public:
	DisableCloseButton(dbc::uLong interval) : dbc::Timer(interval) {}
private:
	~DisableCloseButton() {}
	void Process() {}
};
#endif
class DelayLogout : public dbc::Timer, public dbc::RunBiDirectChain<Player> {
public:
	DelayLogout(dbc::uLong interval) : dbc::Timer(interval) {}
	void AddPlayer(Player* ply) {
		ply->_BeginRun(this);
	}
	void DelPlayer(Player* ply) {
		ply->_EndRun();
	}

private:
	void Process() {
		Player* l_ply{};
		dbc::RunChainGetArmor<Player> l_lock(*this);
		while (l_ply = GetNextItem()) {
		}
		l_lock.unlock();
	}
};
class HandshakeTimeoutTimer : public dbc::Timer {
public:
	HandshakeTimeoutTimer(dbc::uLong interval) : dbc::Timer(interval) {}

private:
	void Process() {
		if (!g_gtsvr || !g_gtsvr->cli_conn)
			return;

		dbc::uLong handshakeTimeout = g_gtsvr->cli_conn->GetHandshakeTimeout() * 1000;
		dbc::uLong loginTimeout = 60000; // 60 seconds to login
		dbc::uLong now = GetTickCount();

		// Iterate all connected players
		dbc::RunChainGetArmor<Player> l_lock(g_gtsvr->m_plylst);
		Player* ply = nullptr;
		while ((ply = g_gtsvr->m_plylst.GetNextItem())) {
			if (ply->m_datasock) {
				// Handle tick wraparound carefully
				dbc::uLong elapsed = (now >= ply->m_connectTime) ? (now - ply->m_connectTime) : (ULONG_MAX - ply->m_connectTime + now);

				// 1. Handshake Timeout: Must exchange public keys within X seconds
				if (!ply->m_clientPublicKey) {
					if (elapsed > handshakeTimeout) {
						LG("GateServer", "Handshake timeout for %s (Elapsed: %d ms)\n", ply->m_datasock->GetPeerIP(), elapsed);
						g_gtsvr->cli_conn->Disconnect(ply->m_datasock, 0, -25);
					}
				}
				// 2. Login Timeout: Must successfully login (get AccountID) within 60 seconds
				else if (ply->m_actid == 0) {
					if (elapsed > loginTimeout) {
						LG("GateServer", "Login timeout for %s (Elapsed: %d ms)\n", ply->m_datasock->GetPeerIP(), elapsed);
						g_gtsvr->cli_conn->Disconnect(ply->m_datasock, 0, -25);
					}
				}
			}
		}
		l_lock.unlock();
	}
};

void __cdecl ctrlc_dispatch(int sig) {
	if (sig == SIGINT) {
		g_exit = 1;
		signal(SIGINT, ctrlc_dispatch);
	}
}

//---------------------------------------------------------------------------
// class GateServer
//---------------------------------------------------------------------------
GateServer::GateServer(char const* fname)
	: player_heap(1, 2000), m_tch(1, 200), gm_conn(nullptr), gp_conn(nullptr), cli_conn(nullptr), m_clcomm(nullptr), m_gpcomm(nullptr), m_gmcomm(nullptr), m_clproc(nullptr) {
	dbc::TcpCommApp::WSAStartup();
	srand((unsigned int)time(nullptr)); // ��ʼ�����������

	m_tch.Init();
	player_heap.Init();

	m_clproc = dbc::ThreadPool::CreatePool(24, 32, 4096);
	m_clcomm = dbc::ThreadPool::CreatePool(6, 12, 4096, THREAD_PRIORITY_ABOVE_NORMAL);
	m_gpproc = dbc::ThreadPool::CreatePool(4, 8, 1024, THREAD_PRIORITY_ABOVE_NORMAL);
	m_gpcomm = dbc::ThreadPool::CreatePool(12, 24, 2048, THREAD_PRIORITY_ABOVE_NORMAL);
	m_gmcomm = dbc::ThreadPool::CreatePool(4, 4, 2048, THREAD_PRIORITY_ABOVE_NORMAL);

	try {
		gm_conn = new ToGameServer(fname, 0, m_gmcomm);
		gp_conn = new ToGroupServer(fname, m_gpproc, m_gpcomm);
		cli_conn = new ToClient(fname, m_clproc, m_clcomm);
		m_gpproc->AddTask(new ConnectGroupServer(gp_conn));
		m_clproc->AddTask(&g_timermgr);
		g_timermgr.AddTimer(new DisableCloseButton(200));
		g_timermgr.AddTimer(new HandshakeTimeoutTimer(1000)); // Check every second
		signal(SIGINT, ctrlc_dispatch);
	} catch (...) {
		if (gp_conn) {
			delete gp_conn;
			gp_conn = 0;
		}
		if (gm_conn) {
			delete gm_conn;
			gm_conn = nullptr;
		}
		if (cli_conn) {
			delete cli_conn;
			cli_conn = nullptr;
		}
		m_gmcomm->DestroyPool();
		m_gpcomm->DestroyPool();
		m_clcomm->DestroyPool();
		m_clproc->DestroyPool();
		dbc::TcpCommApp::WSACleanup();
		throw;
	}
}

GateServer::~GateServer() {
	g_exit = 1;
	while (g_ref) {
		Sleep(1);
	}
	delete cli_conn;
	delete gp_conn;
	delete gm_conn;
	m_gmcomm->DestroyPool();
	m_gpcomm->DestroyPool();
	m_clcomm->DestroyPool();
	m_clproc->DestroyPool();
	dbc::TcpCommApp::WSACleanup();
}

void GateServer::RunLoop() {
	BandwidthStat l_band;
	LLong recvpkps_max = 0, recvbandps_max = 0, sendpkps_max = 0, sendbandps_max = 0;

	dstring l_str;
	l_str.SetSize(256);
	while (!g_exit) {
		std::cout << "Please enter a command (exit or ctrl+c to exit):\n";
		std::cin.getline(l_str.GetBuffer(), 256);

		if (l_str == "exit" || g_exit) {
			std::cout << RES_STRING(GS_GATESERVER_CPP_00002) << std::endl;
			break;
		} else if (l_str == "help" || l_str == "?") {
			std::cout << "exit" << std::endl;
			std::cout << "getinfo" << std::endl;
			std::cout << "clmax" << std::endl;
			std::cout << "getmaxcon" << std::endl;
			std::cout << "setmaxcon" << std::endl;
			std::cout << "reconnect" << std::endl;
			std::cout << "calltotal" << std::endl;
			std::cout << " " << std::endl;

			std::cout << "help" << std::endl;

		} else if (l_str == "getinfo") {
			std::cout << "getinfo..." << std::endl;

			l_band = cli_conn->GetBandwidthStat();
			std::cout << "getinfo: GetBandwidthStat..." << std::endl;

			std::cout << RES_STRING(GS_GATESERVER_CPP_00003) << cli_conn->GetSockTotal() << std::endl;
			std::cout << RES_STRING(GS_GATESERVER_CPP_00004) << l_band.m_sendpktps << "}{pkt:" << l_band.m_sendpkts << "}{KB/s:" << l_band.m_sendbyteps / 1024 << "}{KB:" << l_band.m_sendbytes / 1024 << "}" << std::endl;
			std::cout << RES_STRING(GS_GATESERVER_CPP_00005) << l_band.m_recvpktps << "}{pkt:" << l_band.m_recvpkts << "}{KB/s:" << l_band.m_recvbyteps / 1024 << "}{KB:" << l_band.m_recvbytes / 1024 << "}" << std::endl;

			if (l_band.m_sendpktps > sendpkps_max)
				sendpkps_max = l_band.m_sendpktps;
			if (l_band.m_sendbyteps / 1024 > sendbandps_max)
				sendbandps_max = l_band.m_sendbyteps / 1024;
			if (l_band.m_recvpktps > recvpkps_max)
				recvpkps_max = l_band.m_recvpktps;
			if (l_band.m_recvbyteps / 1024 > recvbandps_max)
				recvbandps_max = l_band.m_recvbyteps / 1024;
			std::cout << RES_STRING(GS_GATESERVER_CPP_00006) << sendpkps_max << "}{KB/s:" << sendbandps_max << "}" << std::endl;
			std::cout << RES_STRING(GS_GATESERVER_CPP_00007) << recvpkps_max << "}{KB/s:" << recvbandps_max << "}" << std::endl;
		} else if (l_str == "clmax") {
			recvpkps_max = recvbandps_max = sendpkps_max = sendbandps_max = 0;
		} else if (l_str == "getmaxcon") {
			std::cout << RES_STRING(GS_GATESERVER_CPP_00008) << g_gtsvr->cli_conn->GetMaxCon() << std::endl;
		} else if (!strncmp(l_str.c_str(), "setmaxcon", 9)) {
			uShort l_maxcon = atoi(l_str.c_str() + 9);
			if (l_maxcon > 1500) {
				std::cout << RES_STRING(GS_GATESERVER_CPP_00009) << std::endl;
				l_maxcon = 1500;
			} else {
				std::cout << RES_STRING(GS_GATESERVER_CPP_00010) << l_maxcon << std::endl;
			}
			g_gtsvr->cli_conn->SetMaxCon(l_maxcon);
		}
		else if (l_str == "getqueparm") {
			std::cout << "ToClient Process Queue:" << m_clproc->GetTaskCount() << "\tToClint Comm Queue:" << m_clcomm->GetTaskCount() << std::endl;
			std::cout << "ToGroup Comm Queue:" << m_gpcomm->GetTaskCount() << "\tToGame Comm Queue:" << m_gmcomm->GetTaskCount() << std::endl;
		} else if (l_str == "reconnect") {
			if (g_gtsvr->gp_conn) {
				g_gtsvr->gp_conn->Disconnect(g_gtsvr->gp_conn->get_datasock(), -9);
				std::cout << "reconnect success!" << std::endl;
			} else {
				std::cout << "reconnect failed! null pointer!" << std::endl;
			}
		} else if (l_str == "calltotal") {
			std::cout << "clinet::calltotal:[" << g_gtsvr->cli_conn->GetCallTotal() << "]" << std::endl;
			std::cout << "group::calltotal:[" << g_gtsvr->gp_conn->GetCallTotal() << "]" << std::endl;
		}
		else {
			std::cout << RES_STRING(GS_GATESERVER_CPP_00012) << std::endl;
		}
	}
}

//---------------------------------------------------------------------------
// class Player
//---------------------------------------------------------------------------
bool Player::InitReference(dbc::DataSocket* datasock) {
	MutexArmor lock(g_gtsvr->_mtxother); // 组织重复进入
	if (datasock && !datasock->GetPointer()) {
		datasock->SetPointer(this);
		m_datasock = datasock;
		return true;
	} else {
		if (datasock) {
			try {
				LG("GateServer", RES_STRING(GS_GATESERVER_CPP_00013), datasock->GetPeerIP());
				auto l_ply = static_cast<Player*>(datasock->GetPointer());
				if (l_ply) {
					l_ply->m_datasock = nullptr;
					datasock->SetPointer(nullptr);
				}
			} catch (...) {
				LG("GateServer", RES_STRING(GS_GATESERVER_CPP_00014), datasock->GetPeerIP());
			}
		}
		return false;
	}
	return false;
}

void Player::Initially() {
	m_worldid = m_actid = m_dbid = gm_addr = gp_addr = m_status = 0;
	m_password[0] = 0;
	m_acctname[0] = 0;
	m_connectTime = GetTickCount();
	m_datasock = nullptr;
	game = nullptr;

	enc = false;
	m_pingtime = 0;
	m_lestoptick = GetTickCount();
	m_estop = false;
	m_sGarnerWiner = 0;

	// RSA-AES BOTAN
	m_clientPrivateKey = nullptr;
	m_clientPublicKey = nullptr;
	Botan::AutoSeeded_RNG rng;
	Botan::SymmetricKey aes_key(rng, AES_KEY_LENGTH); // Clears on destruction.
	Botan::InitializationVector iv(rng, aes_key.length());
	memcpy(m_AESKey, aes_key.bits_of().data(), aes_key.bits_of().size());
	memcpy(m_IV, iv.bits_of().data(), iv.bits_of().size());

	// Initialize stateful ciphers. Two separate IVs ensure S→C and C→S
	// keystreams are never identical (same IV both ways allows cross-stream XOR attacks).
	//   Server enc (S→C) / Client dec: base IV as-is
	//   Server dec (C→S) / Client enc: IV with first byte flipped (^= 0x01)
	AES_IV cs_iv;
	memcpy(cs_iv, m_IV, AES_IV_LENGTH);
	cs_iv[0] ^= 0x01;

	m_enc_cipher = Botan::Cipher_Mode::create("AES-128/CTR", Botan::ENCRYPTION);
	m_enc_cipher->set_key(m_AESKey, AES_KEY_LENGTH);
	m_enc_cipher->start(m_IV, AES_IV_LENGTH);   // S→C: base IV
	m_mtx_enc.Create(false);

	m_dec_cipher = Botan::Cipher_Mode::create("AES-128/CTR", Botan::DECRYPTION);
	m_dec_cipher->set_key(m_AESKey, AES_KEY_LENGTH);
	m_dec_cipher->start(cs_iv, AES_IV_LENGTH);  // C→S: modified IV

	// Register in player registry with new generation
	if (g_gtsvr) {
		g_gtsvr->RegisterPlayer(this);
	}
}

void Player::Finally() {
	// Unregister from player registry before resetting state
	if (g_gtsvr) {
		g_gtsvr->UnregisterPlayer(this);
	}
	
	m_worldid = m_actid = m_dbid = gm_addr = gp_addr = m_status = 0;
	game = nullptr;
	enc = false;
	if (m_datasock != nullptr) {
		m_datasock->SetPointer(nullptr);
		m_datasock = nullptr;
	}

	if (m_clientPrivateKey) {
		delete m_clientPrivateKey;
		m_clientPrivateKey = nullptr;
	}
	if (m_clientPublicKey) {
		delete m_clientPublicKey;
		m_clientPublicKey = nullptr;
	}
	memset(m_AESKey, 0, sizeof(m_AESKey));
	memset(m_IV, 0, sizeof(m_IV));
	m_enc_cipher.reset();
	m_dec_cipher.reset();
}

// Add by lark.li 20081119 begin
bool Player::BeginRun() {
	return RunBiDirectItem<Player>::_BeginRun(&(g_gtsvr->m_plylst)) ? true : false;
}
bool Player::EndRun() {
	return RunBiDirectItem<Player>::_EndRun() ? true : false;
}

void Player::SendSysInfo(std::string_view message) const {
	auto wpk = g_gtsvr->cli_conn->GetWPacket();
	wpk.WriteCmd(CMD_MC_SYSINFO);
	wpk.WriteString(message.data());
	g_gtsvr->cli_conn->SendData(m_datasock, wpk);
}
// End

// Static generation counter initialization
std::atomic<uint32_t> Player::s_globalGeneration{1};

// Player::Free implementation - handles registry cleanup
void Player::Free() {
	if (g_gtsvr) {
		g_gtsvr->UnregisterPlayer(this);
	}
	PreAllocStru::Free();
}

//---------------------------------------------------------------------------
// Player Pointer Validation System
//---------------------------------------------------------------------------

void GateServer::RegisterPlayer(Player* ply) {
	if (!ply) return;
	
	// Assign new generation
	ply->m_generation = Player::s_globalGeneration.fetch_add(1, std::memory_order_relaxed);
	
	std::unique_lock<std::shared_mutex> lock(m_playerRegistryMutex);
	uintptr_t addr = reinterpret_cast<uintptr_t>(ply);
	m_playerRegistry[addr] = ply->m_generation;
	
	LG("PlayerRegistry", "Registered player %p with generation %u\n", ply, ply->m_generation);
}

void GateServer::UnregisterPlayer(Player* ply) {
	if (!ply) return;
	
	std::unique_lock<std::shared_mutex> lock(m_playerRegistryMutex);
	uintptr_t addr = reinterpret_cast<uintptr_t>(ply);
	auto it = m_playerRegistry.find(addr);
	if (it != m_playerRegistry.end()) {
		LG("PlayerRegistry", "Unregistered player %p (generation %u)\n", ply, it->second);
		m_playerRegistry.erase(it);
	}
}

bool GateServer::ValidatePlayerPointer(Player* ply, long long expectedGpAddr) {
	if (!ply) {
		return false;
	}
	
	uintptr_t addr = reinterpret_cast<uintptr_t>(ply);
	uint32_t expectedGeneration = 0;
	
	// Check registry
	{
		std::shared_lock<std::shared_mutex> lock(m_playerRegistryMutex);
		auto it = m_playerRegistry.find(addr);
		if (it == m_playerRegistry.end()) {
			LG("PlayerRegistry", "INVALID: Player %p not in registry\n", ply);
			return false;
		}
		expectedGeneration = it->second;
	}
	
	// Validate generation matches
	if (ply->m_generation != expectedGeneration) {
		LG("PlayerRegistry", "INVALID: Player %p generation mismatch (expected %u, got %u)\n", 
		   ply, expectedGeneration, ply->m_generation);
		return false;
	}
	
	// Validate gp_addr if provided (for packets from GroupServer)
	if (expectedGpAddr != 0 && ply->gp_addr != expectedGpAddr) {
		LG("PlayerRegistry", "INVALID: Player %p gp_addr mismatch (expected %lld, got %lld)\n",
		   ply, expectedGpAddr, (long long)ply->gp_addr);
		return false;
	}
	
	return true;
}

bool GateServer::IsPlayerRegistered(Player* ply) const {
	if (!ply) return false;
	
	std::shared_lock<std::shared_mutex> lock(m_playerRegistryMutex);
	uintptr_t addr = reinterpret_cast<uintptr_t>(ply);
	return m_playerRegistry.find(addr) != m_playerRegistry.end();
}

//---------------------------------------------------------------------------
// class GateServerApp
//---------------------------------------------------------------------------
GateServerApp* g_app = nullptr;

GateServerApp::GateServerApp() {
	g_app = this;
}

void GateServerApp::ServiceStart() {
	// 启动服务器
	try {
		const char* file_cfg = "GateServer.cfg";
		g_gtsvr = new GateServer(file_cfg);

		std::unique_lock<std::mutex> lk(global_gate_ready_mutex);
		is_global_gate_ready = true;
		lk.unlock();
		global_gate_ready_cv.notify_all();
	} catch (std::exception const& e) {
		cout << e.what() << endl;
		Sleep(10 * 1000);
		exit(-1);
	} catch (...) {
		// cout << "GateServer 初始化期间发生未知错误，请通知开发者!" << endl;
		cout << "GateSerever had an unknown error during initialization, please notify the developer!" << endl;
		Sleep(10 * 1000);
		exit(-2);
	}

	// 服务器启动成功，进入主循环
	// cout << "GateServer 启动成功!" << endl;
	cout << "Launched GateServer successfully!" << endl;
}
void GateServerApp::ServiceStop() {
	// 服务器退出
	delete g_gtsvr;
	g_app = nullptr;

	// cout << "GateServer 成功退出!" << endl;
	cout << RES_STRING(GS_GATESERVER_CPP_00018) << endl;
	Sleep(2000);
}

// ȫ�� GateServer ����
GateServer* g_gtsvr;
bool volatile g_appexit = false;