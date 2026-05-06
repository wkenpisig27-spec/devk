#include "gateserver.h"
#include "log.h"
#include <stdexcept>
#include <condition_variable>
using namespace dbc;
using namespace std;
#include <sstream>
#include <algorithm>

// External function to log security events to terminal and log file
extern void LogSecurityEvent(const char* ip, const char* action, const char* reason);

ToClient::ToClient(char const* fname, ThreadPool* proc, ThreadPool* comm)
	: TcpServerApp(this, proc, comm, false), RPCMGR(this), m_maxcon(500), m_atexit(0), m_calltotal(0) {
	IniFile inf(fname);
	
	// Set log directory from config
	IniSection& resSection = inf["Res"];
	const std::string logDir = resSection["log_dir"];
	if (!logDir.empty()) {
		LG_SetDirWithTimestamp(logDir.c_str());
	}
	
	m_maxcon = std::stoi(inf["ToClient"]["MaxConnection"]); // Modify by lark.li

	m_version = std::stoi(inf["Main"]["Version"]);
	IniSection& is = inf["ToClient"];
	const std::string ip = is["IP"];
	uShort port = std::stoi(is["Port"]);
	_comm_enc = std::stoi(is["CommEncrypt"]);
	
	// Proxy Protocol support (CRITICAL for SmartProxy integration)
	IniSection& ddos = inf["AntiDDoS"];
	std::string proxyProtocolStr = ddos["ProxyProtocol"];
	m_proxyProtocolEnabled = !proxyProtocolStr.empty() && std::stoi(proxyProtocolStr) != 0;
	
	// Optional whitelist (for direct testing without SmartProxy)
	std::string wlEnabledStr = ddos["WhitelistEnabled"];
	m_whitelistEnabled = !wlEnabledStr.empty() && std::stoi(wlEnabledStr) != 0;
	
	if (m_whitelistEnabled) {
		std::string list = ddos["Whitelist"];
		std::stringstream ss(list);
		std::string item;
		while (std::getline(ss, item, ',')) {
			// Trim whitespace
			item.erase(0, item.find_first_not_of(" \t\n\r"));
			item.erase(item.find_last_not_of(" \t\n\r") + 1);
			if (!item.empty()) {
				m_whitelist.push_back(item);
			}
		}
		printf("Whitelist: %zu IPs loaded\n", m_whitelist.size());
	}
	
	if (m_proxyProtocolEnabled) {
		printf("Proxy Protocol v1: ENABLED (SmartProxy integration)\n");
		printf("  Real client IPs will be extracted from PROXY headers\n");
	}
	
	printf("Current client version is %d\n", m_version);
	SetPKParse(0, 2, 16 * 1024, 100);
	BeginWork(std::stoi(is["EnablePing"]), 1);
	if (OpenListenSocket(port, ip.c_str()) != 0)
		throw std::runtime_error("ToClient listen failed\n");

	printf("Generating RSA key pair and AES key...\n");
	Botan::AutoSeeded_RNG rng;
	m_serverPrivateKey = new Botan::RSA_PrivateKey(rng, 3072);
	printf("Key generated!\n");
}

ToClient::~ToClient() {
	m_atexit = 1;
	while (m_calltotal) {
		Sleep(1);
	}
	
	delete m_serverPrivateKey;
	ShutDown(12 * 1000);
}

// Parse Proxy Protocol v1 header to extract real client IP
// Formats supported:
//   "PROXY TCP4 <src-ip> <dst-ip> <src-port> <dst-port>\r\n"
//   "PROXY TCP6 <src-ip> <dst-ip> <src-port> <dst-port>\r\n"
//   "PROXY UNKNOWN\r\n" (health checks - keep socket peer IP)
// Returns: true = successfully parsed (or UNKNOWN), false = failed or disabled
bool ToClient::ParseProxyProtocol(DataSocket* datasock) {
	if (!m_proxyProtocolEnabled) {
		return false;
	}
	
	SOCKET sock = datasock->GetSocket();
	char buffer[108];  // Max proxy protocol v1 header size
	int totalRead = 0;
	
	// Set socket to blocking temporarily for this read
	u_long nonBlocking = 0;
	ioctlsocket(sock, FIONBIO, &nonBlocking);
	
	// Set a short timeout for this read (2 seconds)
	DWORD timeout = 2000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	
	// First, peek to check if we have "PROXY " prefix (6 bytes)
	int peekResult = recv(sock, buffer, 6, MSG_PEEK);
	if (peekResult < 6 || strncmp(buffer, "PROXY ", 6) != 0) {
		// Not a proxy protocol header - restore socket and fail
		nonBlocking = 1;
		ioctlsocket(sock, FIONBIO, &nonBlocking);
		timeout = 0;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
		LG("GateServer", "[ProxyProtocol] No PROXY header from %s\n", datasock->GetPeerIP());
		return false;
	}
	
	// Read up to 107 bytes or until \r\n
	// Use larger chunks for efficiency
	bool foundEnd = false;
	while (totalRead < 107 && !foundEnd) {
		int toRead = (107 - totalRead) < 32 ? (107 - totalRead) : 32;
		int result = recv(sock, buffer + totalRead, toRead, 0);
		if (result <= 0) {
			break;
		}
		
		// Scan newly received bytes for \r\n
		for (int i = totalRead; i < totalRead + result - 1; i++) {
			if (buffer[i] == '\r' && buffer[i+1] == '\n') {
				totalRead = i + 2;
				foundEnd = true;
				break;
			}
		}
		if (!foundEnd) {
			totalRead += result;
		}
	}
	buffer[totalRead] = '\0';
	
	// Restore non-blocking mode
	nonBlocking = 1;
	ioctlsocket(sock, FIONBIO, &nonBlocking);
	
	// Remove timeout
	timeout = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	
	if (!foundEnd) {
		LG("GateServer", "[ProxyProtocol] Incomplete header from %s (len=%d)\n", datasock->GetPeerIP(), totalRead);
		return false;
	}
	
	// Check for "PROXY UNKNOWN\r\n" (health checks from TCPShield)
	if (strncmp(buffer, "PROXY UNKNOWN", 13) == 0) {
		LG("GateServer", "[ProxyProtocol] UNKNOWN (health check) from %s\n", datasock->GetPeerIP());
		return true;  // Accept but keep original IP
	}
	
	// Parse: "PROXY TCP4 192.168.1.1 10.0.0.1 12345 1973\r\n"
	//    or: "PROXY TCP6 ::1 ::1 12345 1973\r\n"
	char protocol[10], family[10], srcIP[46], dstIP[46];
	int srcPort = 0, dstPort = 0;
	
	int parsed = sscanf(buffer, "%9s %9s %45s %45s %d %d", 
		protocol, family, srcIP, dstIP, &srcPort, &dstPort);
	
	if (parsed < 2 || strcmp(protocol, "PROXY") != 0) {
		LG("GateServer", "[ProxyProtocol] Parse failed from %s: %s\n", datasock->GetPeerIP(), buffer);
		return false;
	}
	
	// Validate family is TCP4 or TCP6
	if (strcmp(family, "TCP4") != 0 && strcmp(family, "TCP6") != 0) {
		// Unknown family - accept but log
		LG("GateServer", "[ProxyProtocol] Unknown family '%s' from %s\n", family, datasock->GetPeerIP());
		return true;
	}
	
	// Need full parse for TCP4/TCP6
	if (parsed != 6) {
		LG("GateServer", "[ProxyProtocol] Incomplete %s parse from %s: %s\n", family, datasock->GetPeerIP(), buffer);
		return false;
	}
	
	// Validate IP format (basic check)
	size_t ipLen = strlen(srcIP);
	if (ipLen < 1 || ipLen > 45) {
		LG("GateServer", "[ProxyProtocol] Invalid source IP length: %s\n", srcIP);
		return false;
	}
	
	// Update the socket's peer IP with the real client IP
	const char* oldIP = datasock->GetPeerIP();
	datasock->SetPeerIP(srcIP);
	datasock->SetPeerPort((uShort)srcPort);
	
	LG("GateServer", "[ProxyProtocol] Real IP: %s:%d (proxy: %s, family: %s)\n", srcIP, srcPort, oldIP, family);
	
	return true;
}

bool ToClient::DoCommand(DataSocket* datasock, cChar* cmdline) {
	return false;
}

void ToClient::TaskDispatcher(Task* task) {
	extern std::mutex global_gate_ready_mutex;
	extern std::condition_variable global_gate_ready_cv;
	extern bool is_global_gate_ready;
	std::unique_lock<std::mutex> lk(global_gate_ready_mutex);
	global_gate_ready_cv.wait(lk, [] { return is_global_gate_ready; });
	lk.unlock();

	TcpServerApp::TaskDispatcher(task);
}

bool ToClient::OnConnect(DataSocket* datasock) {
	// === PROXY PROTOCOL PARSING ===
	// SmartProxy sends real client IP in PROXY PROTOCOL v1 header
	// This must happen first so all logs and checks use the real IP
	if (m_proxyProtocolEnabled) {
		if (!ParseProxyProtocol(datasock)) {
			LG("GateServer", "client: %s - Proxy Protocol parsing failed, disconnecting\n", datasock->GetPeerIP());
			return false;
		}
	}
	
	// Check GroupServer ready
	if (!g_gtsvr->gp_conn->IsReady()) {
		LG("GateServer", "client: %s\tcome, groupserver isn't ready, force disconnect...\n", datasock->GetPeerIP());
		return false;
	}
	
	// Check max connections
	if (GetSockTotal() >= m_maxcon) {
		LG("GateServer", "client: %s\tcome, greater than %u player, force disconnect...\n", datasock->GetPeerIP(), m_maxcon);
		return false;
	}
	
	// Set socket buffers
	datasock->SetRecvBuf(64 * 1024);
	datasock->SetSendBuf(64 * 1024);
	
	LG("GateServer", "client: %s\tcome...Socket num: %d\n", datasock->GetPeerIP(), GetSockTotal() + 1);
	return true;
}



void ToClient::OnConnected(DataSocket* datasock) {
	Player* l_ply = g_gtsvr->player_heap.Get();
	if (!l_ply) {
		LG("GateServer", "error: poor mem %s!\n", datasock->GetPeerIP());
		Disconnect(datasock);
		return;
	}

	if (!l_ply->InitReference(datasock)) {
		LG("GateServer", "warning: forbid %s repeat connect!\n", datasock->GetPeerIP());
		l_ply->Free();
		Disconnect(datasock);
		return;
	}

	// Add player to the monitored player list
	l_ply->BeginRun();

	WPacket l_wpk = GetWPacket();
	l_wpk.WriteCmd(CMD_MC_RSA_HANDSHAKE_1);
	// Send server's public key to client.
	std::string PEM_encoded = Botan::X509::PEM_encode(*m_serverPrivateKey); // No copy
	l_wpk.WriteString(PEM_encoded.c_str());

	PEM_encoded.clear();
	PEM_encoded.shrink_to_fit();
	SendData(datasock, l_wpk);
}

void ToClient::OnDisconnect(DataSocket* datasock, int reason) {
	LG("GateServer", "client: %s gone...Socket num: %d ,reason=%s\n", datasock->GetPeerIP(), GetSockTotal(), GetDisconnectErrText(reason).c_str());

	RPacket l_rpk = 0;
	CM_LOGOUT(datasock, l_rpk);
}

void ToClient::CM_RSA_HANDSHAKE1(DataSocket* datasock, RPacket& recvbuf) {
	uLong l_ulMilliseconds = m_handshakeTimeout * 1000;
	uLong l_tick = GetTickCount() - recvbuf.GetTickCount();
	if (l_ulMilliseconds > l_tick) {
		l_ulMilliseconds = l_ulMilliseconds - l_tick;

		Player* l_ply = (Player*)datasock->GetPointer();
		if (l_ply) {
			MutexArmor l_lockStat(l_ply->m_mtxstat);
			uShort len;
			const char* pemKey = recvbuf.ReadString(&len);
			
			// === RSA KEY VALIDATION (prevent CPU exhaustion attack) ===
			// Validate PEM format before attempting expensive crypto operations
			if (!pemKey || len < 100 || len > 4096) {
				// Invalid key length - legitimate RSA-3072 keys are ~1700-2000 bytes in PEM
				LG("GateServer", "[Security] %s: Invalid RSA key length: %d\n", datasock->GetPeerIP(), len);
				l_lockStat.unlock();
				Disconnect(datasock, 100, -31);
				return;
			}
			
			// Quick PEM header check before expensive parsing
			if (strncmp(pemKey, "-----BEGIN", 10) != 0) {
				LG("GateServer", "[Security] %s: Invalid RSA key format (no PEM header)\n", datasock->GetPeerIP());
				l_lockStat.unlock();
				Disconnect(datasock, 100, -31);
				return;
			}
			
			try {
				Botan::DataSource_Memory public_key_data((uint8_t*)pemKey, len);
				l_ply->m_clientPublicKey = Botan::X509::load_key(public_key_data);
				
				// Verify it's actually an RSA key with reasonable size
				if (!l_ply->m_clientPublicKey || l_ply->m_clientPublicKey->algo_name() != "RSA") {
					LG("GateServer", "[Security] %s: Not an RSA key\n", datasock->GetPeerIP());
					l_lockStat.unlock();
					Disconnect(datasock, 100, -31);
					return;
				}
				
				// Now send AES key to client.
				Botan::AutoSeeded_RNG rng;
				Botan::PK_Encryptor_EME enc(*l_ply->m_clientPublicKey, rng, "OAEP(SHA-256)");
				std::vector<uint8_t> aes_encrypted = enc.encrypt((uint8_t*)l_ply->m_AESKey, AES_KEY_LENGTH, rng);
				std::vector<uint8_t> iv_encrypted = enc.encrypt((uint8_t*)l_ply->m_IV, AES_IV_LENGTH, rng);
				dbc::WPacket l_wpk = GetWPacket();
				l_wpk.WriteCmd(CMD_MC_RSA_HANDSHAKE_2);
				l_wpk.WriteSequence((cChar*)aes_encrypted.data(), aes_encrypted.size());
				l_wpk.WriteSequence((cChar*)iv_encrypted.data(), iv_encrypted.size());
				l_wpk.WriteChar((uChar)_comm_enc); // Tell client if encryption is enabled
				SendData(datasock, l_wpk);

				aes_encrypted.clear();
				aes_encrypted.shrink_to_fit();
				iv_encrypted.clear();
				iv_encrypted.shrink_to_fit();
			} catch (const std::exception& e) {
				LG("GateServer", "[Security] %s: RSA key parse error: %s\n", datasock->GetPeerIP(), e.what());
				l_lockStat.unlock();
				Disconnect(datasock, 100, -31);
				return;
			}

			l_lockStat.unlock();
		}
	} else {
		Disconnect(datasock, 100, -25);
	}
}

std::string ToClient::GetDisconnectErrText(int reason) const {
	switch (reason) {
	case -21:
		return RES_STRING(GS_TOCLIENT_CPP_00011);
	case -23:
		return RES_STRING(GS_TOCLIENT_CPP_00012);
	case -24:
		return RES_STRING(GS_TOCLIENT_CPP_00013);
	case -25:
		return RES_STRING(GS_TOCLIENT_CPP_00014);
	case -27:
		return RES_STRING(GS_TOCLIENT_CPP_00015);
	case -29:
		return RES_STRING(GS_TOCLIENT_CPP_00016);
	case -31:
		return RES_STRING(GS_TOCLIENT_CPP_00017);
	case -32:
		return RES_STRING(GS_TOCLIENT_CPP_00019);
	case -33:
		return RES_STRING(GS_TOCLIENT_CPP_00020);
	default:
		return TcpServerApp::GetDisconnectErrText(reason).c_str();
	}
}

WPacket ToClient::OnServeCall(DataSocket* datasock, RPacket& in_para) {
	uShort l_cmd = in_para.ReadCmd();

	switch (l_cmd) {
	case CMD_CM_LOGOUT: {
		CM_LOGOUT(datasock, in_para);
		Disconnect(datasock, 65, -27);
		return 0;
	}
	default: {
		break;
	}
	}
	return 0;
}

void ToClient::ReRouteToGameServer(dbc::DataSocket* datasock, dbc::RPacket& recvbuf) {
	Player* l_ply = (Player*)datasock->GetPointer();
	if (l_ply) {
		const long long l_gpaddr = l_ply->gp_addr;
		const long long l_gmaddr = l_ply->gm_addr;
		GameServer* l_game = l_ply->game;

		if (l_gpaddr && l_gmaddr && l_game) {
			WPacket l_wpk = WPacket(recvbuf).Duplicate();
			l_wpk.WriteLongLong(MakeULong(l_ply));
			l_wpk.WriteLongLong(l_gmaddr);
			g_gtsvr->gm_conn->SendData(l_ply->game->m_datasock, l_wpk);
		}
	}
}

void ToClient::ReRouteToGroupServer(dbc::DataSocket* datasock, dbc::RPacket& recvbuf) {
	Player* l_ply = (Player*)datasock->GetPointer();
	if (l_ply) {
		if (!g_gtsvr->gp_conn->IsReady()) {
			LG("ToGroupServerError", "l_cmd = %d IsReady \n", recvbuf.ReadCmd());
			dbc::WPacket l_wpk = GetWPacket();
			l_wpk.WriteCmd(CMD_MC_LOGIN);
			l_wpk.WriteShort(ERR_MC_NETEXCP); // ������
			SendData(datasock, l_wpk);		  // �����ͻ���
			this->Disconnect(datasock, 100, -31);
		}

		const long long l_gpaddr = l_ply->gp_addr;
		const long long l_gmaddr = l_ply->gm_addr;
		if (l_gpaddr && l_gmaddr) {
			WPacket l_wpk = WPacket(recvbuf).Duplicate();
			l_wpk.WriteLongLong(MakeULong(l_ply));
			l_wpk.WriteLongLong(l_gpaddr);
			g_gtsvr->gp_conn->SendData(g_gtsvr->gp_conn->get_datasock(), l_wpk);
		}
	}
}

void ToClient::OnProcessData(DataSocket* datasock, RPacket& recvbuf) {
	uShort l_cmd = recvbuf.ReadCmd();
	try {
		// Validate CMD is within a valid range for GateServer (client-facing)
		// Valid ranges: CMD_CM (0-499) client→game, CMD_CP (6000-6499) client→group
		bool validCmd =
			(l_cmd >= CMD_CM_BASE && l_cmd < CMD_MC_BASE) ||
			(l_cmd >= CMD_CP_BASE && l_cmd < CMD_OS_BASE);

		if (!validCmd) {
			LG("Security", "[PacketValidation] Invalid CMD %u from %s - rejected\n",
			   static_cast<unsigned int>(l_cmd), datasock->GetPeerIP());
			Disconnect(datasock, 50, -32);
			return;
		}

		// Anti-Exploit: Check for oversized packets (max 8KB for client packets)
		const int MAX_CLIENT_PACKET_SIZE = 8192;
		int packetSize = recvbuf.GetDataLen();
		if (packetSize > MAX_CLIENT_PACKET_SIZE) {
			const char* ip = datasock->GetPeerIP();
			LG("AntiExploit", "[BIG PACKET] IP: %s Size: %d bytes CMD: %d", 
				ip ? ip : "Unknown", packetSize, l_cmd);
			LogSecurityEvent(ip ? ip : "Unknown", "BIG PACKET", "Oversized packet");
			Disconnect(datasock, 100, -25);
			return;
		}

		Player* l_ply = (Player*)datasock->GetPointer();

		switch (l_cmd) {
		case CMD_CM_LOGIN:	 // �����û���/����Խ�����֤,�����û����µ����з��������ϵ���Ч��ɫ�б�.
		case CMD_CM_LOGOUT:	 // ͬ������
		case CMD_CM_BGNPLAY: // ����ѡ��Ľ�ɫ������GroupServer��֤��Ȼ��֪ͨGameServerʹ��ɫ������ͼ�ռ�.
		case CMD_CM_ENDPLAY:
		case CMD_CM_NEWCHA:
		case CMD_CM_DELCHA:
		case CMD_CM_CREATE_PASSWORD2:
		case CMD_CM_UPDATE_PASSWORD2:
		case CMD_CM_REGISTER:
		case CMD_CP_CHANGEPASS:
		case CMD_CM_RSA_HANDSHAKE_1: {
			if (!g_gtsvr->gp_conn->IsReady()) {
				LG("ToGroupServerError", "l_cmd = %d Login \n", l_cmd);
				dbc::WPacket l_wpk = GetWPacket();
				l_wpk.WriteCmd(CMD_MC_LOGIN);
				l_wpk.WriteShort(ERR_MC_NETEXCP);
				SendData(datasock, l_wpk);
				this->Disconnect(datasock, 100, -31);
				break;
			}

			// Anti-exhaustion: Cap concurrent SyncCall tasks to reserve threads for active players
			// Increment first (atomic), then check — prevents race where two threads both read 27 and pass
			++m_calltotal;
			if (m_calltotal > 28) {
				--m_calltotal;
				LG("Security", "[SyncCall] Thread cap reached, rejecting CMD %d from %s\n",
					l_cmd, datasock->GetPeerIP());
				dbc::WPacket l_wpk = GetWPacket();
				l_wpk.WriteCmd(CMD_MC_LOGIN);
				l_wpk.WriteShort(ERR_MC_NETEXCP);
				SendData(datasock, l_wpk);
				this->Disconnect(datasock, 100, -31);
				break;
			}
			if (m_atexit) {
				--m_calltotal;
				return;
			}

			TransmitCall* l_tc = g_gtsvr->m_tch.Get();
			l_tc->Init(datasock, recvbuf);
			GetProcessor()->AddTask(l_tc);
		} break;
		case CMD_CP_PING: {
			// Client keepalive ping — always accept to reset m_recvtime.
			// Only forward to GroupServer if the player is fully in-game.
			// Never disconnect on ping — a keepalive should not kill the connection.
			Player* l_ply = (Player*)datasock->GetPointer();
			if (l_ply && l_ply->gp_addr && l_ply->gm_addr && g_gtsvr->gp_conn->IsReady()) {
				WPacket l_wpk = GetWPacket();
				l_wpk.WriteCmd(CMD_CP_PING);
				l_wpk.WriteLong(GetTickCount() - l_ply->m_pingtime);
				l_ply->m_pingtime = 0;

				l_wpk.WriteLongLong(MakeULong(l_ply));
				l_wpk.WriteLongLong(l_ply->gp_addr);
				g_gtsvr->gp_conn->SendData(g_gtsvr->gp_conn->get_datasock(), l_wpk);
			}
			// If not in-game or GroupServer is down, silently ignore.
			// The packet arrival already reset m_recvtime (keepalive purpose served).
		} break;
		case CMD_CM_SAY: {
			Player* l_ply = (Player*)datasock->GetPointer();
			if (!l_ply) {
				// The packet can't be coming from an actual player,
				// lets not process it further
				LG("ErrServer", "CMD_CM_SAY: invalid player.\n");
				break;
			}

			cChar* l_str = recvbuf.ReadString();
			if (!l_str) {
				break;
			}
			if (*l_str == '&' && DoCommand(datasock, l_str + 1)) {
				break;
			}
			if (strstr(l_str, "#21")) {
				break;
			}

			if (l_ply->m_estop) {
				if (GetTickCount() - l_ply->m_lestoptick >= 1000 * 60 * 2) {
					WPacket l_wpk = GetWPacket();
					l_wpk.WriteCmd(CMD_TP_ESTOPUSER_CHECK);
					l_wpk.WriteLong(l_ply->m_actid);

					g_gtsvr->gp_conn->SendData(g_gtsvr->gp_conn->get_datasock(), l_wpk);
				}
				WPacket l_wpk = GetWPacket();
				l_wpk.WriteCmd(CMD_MC_SYSINFO);
				// l_wpk.WriteString("���Ѿ���ϵͳ���ԣ�");
				l_wpk.WriteString(RES_STRING(GS_TOCLIENT_CPP_00018));
				g_gtsvr->gp_conn->SendData(l_ply->m_datasock, l_wpk);
				break;
			}

			ReRouteToGameServer(datasock, recvbuf);
		} break;
		case CMD_CM_KITBAG_UNLOCK: {
			auto ply = (Player*)datasock->GetPointer();
			if (!ply)
				break;

			auto seq = ReadPacketSequenceEncrypted(recvbuf, ply->m_AESKey);

			// Reset the buffer (in size only, capacity remains the same)
			recvbuf.DiscardLast(recvbuf.HasData());
			WPacket wpk(recvbuf);

			// and overwrite with decrypted sequence
			wpk.WriteSequence((cChar*)seq.data(), seq.size());

			// Update recvbuf with metadata from wpk
			recvbuf = wpk;

			ReRouteToGameServer(datasock, recvbuf);
		} break;
		case CMD_CM_ITEM_UNLOCK_ASK: {
			auto ply = (Player*)datasock->GetPointer();
			if (!ply)
				break;

			const auto seq = ReadPacketSequenceEncrypted(recvbuf, ply->m_AESKey);

			WPacket wpk = GetWPacket();
			wpk.WriteCmd(CMD_CM_ITEM_UNLOCK_ASK);
			wpk.WriteSequence((cChar*)seq.data(), seq.size());
			wpk.WriteChar(recvbuf.ReadChar()); // slot position

			recvbuf = wpk;
			ReRouteToGameServer(datasock, recvbuf);
		} break;
		case CMD_CM_OFFLINE_MODE: {
			MutexArmor lock(g_gtsvr->_mtxother);
			Player* player = (Player*)datasock->GetPointer();
			if (!player) {
				break;
			}

			if (!player->game) {
				break;
			}

			MutexArmor lock_stat(player->m_mtxstat);
			if (player->m_status != 2) {
				break;
			}

			auto wpk = GetWPacket();
			wpk.WriteCmd(CMD_TM_OFFLINE_MODE);
			wpk.WriteLongLong(MakeULong(player));
			wpk.WriteLongLong(player->gm_addr);

			auto rpk = SyncCall(player->game->m_datasock, wpk, 10 * 1000);
			if (!rpk.HasData()) {
				break;
			}

			switch (const auto return_code = static_cast<ReturnCode::OfflineMode>(rpk.ReadChar())) {
			case ReturnCode::OfflineMode::Success: {
				try {
					// Notify GroupServer about disconnect (like normal logout)
					// This is important so the account status gets set to OFFLINE
					{
						WPacket l_wpk = GetWPacket();
						l_wpk.WriteCmd(CMD_TP_DISC);
						l_wpk.WriteLong(player->m_actid);
						l_wpk.WriteLong(inet_addr(datasock->GetPeerIP()));
						l_wpk.WriteString("Offline stall mode activated");
						g_gtsvr->gp_conn->SendData(g_gtsvr->gp_conn->get_datasock(), l_wpk);
					}

					// Notify GameServer to clean up player (but keep the offline stall NPC)
					GameServer* l_game = player->game;
					if (player->gm_addr && l_game && l_game->m_datasock) {
						WPacket l_wpk = l_game->m_datasock->GetWPacket();
						l_wpk.WriteCmd(CMD_TM_GOOUTMAP);
						l_wpk.WriteChar(1); // 1 = offline stall mode, preserve stall
						l_wpk.WriteLongLong(MakeULong(player));
						l_wpk.WriteLongLong(player->gm_addr);
						SendData(l_game->m_datasock, l_wpk);
						player->game = nullptr;
						player->gm_addr = 0;
					}

					// Notify GroupServer about logout (this sets account to OFFLINE)
					// MUST use SyncCall (not SendData) because CMD_TP_USER_LOGOUT is only
					// handled in OnServeCall. SendData routes to OnProcessData which has
					// no handler for it, causing the logout to be silently dropped.
					{
						DataSocket* gp_ds = g_gtsvr->gp_conn->get_datasock();
						if (gp_ds) {
							WPacket l_wpk = g_gtsvr->gp_conn->GetWPacket();
							l_wpk.WriteCmd(CMD_TP_USER_LOGOUT);
							l_wpk.WriteLongLong(MakeULong(player));
							l_wpk.WriteLongLong(player->gp_addr);
							player->gp_addr = 0;
							g_gtsvr->gp_conn->SyncCall(gp_ds, l_wpk, 10 * 1000);
						} else {
							player->gp_addr = 0;
						}
					}

					player->EndRun();
					TC_DISCONNECT(player->m_datasock, -33);
				} catch (...) {
					LG("GateServer", "Error offline mode!\n");
				}

				player->m_datasock = nullptr;
				datasock->SetPointer(nullptr);
				player->Free();
			} break;
			case ReturnCode::OfflineMode::Disabled: {
				player->SendSysInfo("Offline stall mode is disabled.");
			} break;
			case ReturnCode::OfflineMode::NotSafeZone: {
				player->SendSysInfo("Offline stall is only available in safe zones.");
			} break;
			case ReturnCode::OfflineMode::NotStalling: {
				player->SendSysInfo("You must have an active stall to use offline mode.");
			} break;
			default: {
				player->SendSysInfo("Something went wrong trying to use offline mode.");
				LG("ErrServer", "Offline stall failed for a player, return code: %d\n", static_cast<int>(return_code));
			} break;
			}
		} break;

		default:									// ȱʡ��ת����GroupServer����GameServer
			if (l_cmd / 500 == CMD_CM_BASE / 500) { // ת����GameServer
				ReRouteToGameServer(datasock, recvbuf);
			} else if (l_cmd / 500 == CMD_CP_BASE / 500) { // ת����GroupServer
				auto l_ply = static_cast<Player*>(datasock->GetPointer());
				if (l_ply) {
					if (l_cmd == CMD_CP_SAY2TRADE ||
						l_cmd == CMD_CP_SAY2ALL ||
						l_cmd == CMD_CP_SAY2YOU ||
						l_cmd == CMD_CP_SAY2GUD ||
						l_cmd == CMD_CP_SESS_SAY) {
						IniFile inf("GateServer.cfg");
						if (std::stoi(inf["Chaos"]["IsActive"])) {
							const char* chamap = l_ply->GetMapName();
							if (strcmp(chamap, inf["Chaos"]["Map"].c_str()) == 0) {
								WPacket b_wpk = datasock->GetWPacket();
								b_wpk.WriteCmd(CMD_MC_SYSINFO);
								const char* msg = "Unable to chat in this map!";
								b_wpk.WriteSequence(msg, uShort(strlen(msg)) + 1);
								g_gtsvr->cli_conn->SendData(l_ply->m_datasock, b_wpk);
								return;
							}
						}
					}
					ReRouteToGroupServer(datasock, recvbuf);
				}
			}
			break;
		}
		if (datasock->m_recvbyteps > 1024 * 12) {
			LG("AttackMonitor", "[%s] flooding (>12K/s)\n", datasock->GetPeerIP());
			std::cout << "[" << datasock->GetPeerIP() << "] flooding (>12K/s)\n";
		}
	} catch (...) {
		LG("ToClientError", "l_cmd = %d\n", l_cmd);
	}
	LG("ToClient", "<--l_cmd = %d\n", l_cmd);
	return;
}
int TransmitCall::Process() {
	if (!g_gtsvr->gp_conn->IsReady()) {
		g_gtsvr->cli_conn->Disconnect(m_datasock, 50, -27);
		LG("GateServer", "IsReady = false\n");
		--(g_gtsvr->cli_conn->m_calltotal);
		return 0;
	}

	uShort l_cmd = m_recvbuf.ReadCmd();

	// l_line<<newln<<"st:"<<l_cmd;

	try {
		switch (l_cmd) {
		case CMD_CM_LOGIN: // �����û���/����Խ�����֤,�����û����µ����з��������ϵ���Ч��ɫ�б�.
			g_gtsvr->cli_conn->CM_LOGIN(m_datasock, m_recvbuf);
			break;
		case CMD_CM_LOGOUT: // ͬ������
			g_gtsvr->cli_conn->CM_LOGOUT(m_datasock, m_recvbuf);
			g_gtsvr->cli_conn->Disconnect(m_datasock, 50, -27);
			break;
		case CMD_CM_BGNPLAY: // ����ѡ��Ľ�ɫ������GroupServer��֤��Ȼ��֪ͨGameServerʹ��ɫ������ͼ�ռ�.
			g_gtsvr->cli_conn->CM_BGNPLAY(m_datasock, m_recvbuf);
			break;
		case CMD_CM_ENDPLAY:
			g_gtsvr->cli_conn->CM_ENDPLAY(m_datasock, m_recvbuf);
			break;
		case CMD_CM_NEWCHA:
			g_gtsvr->cli_conn->CM_NEWCHA(m_datasock, m_recvbuf);
			break;
		case CMD_CM_DELCHA:
			g_gtsvr->cli_conn->CM_DELCHA(m_datasock, m_recvbuf);
			break;
		case CMD_CM_CREATE_PASSWORD2:
			g_gtsvr->cli_conn->CM_CREATE_PASSWORD2(m_datasock, m_recvbuf);
			break;
		case CMD_CM_UPDATE_PASSWORD2:
			g_gtsvr->cli_conn->CM_UPDATE_PASSWORD2(m_datasock, m_recvbuf);
			break;
		case CMD_CM_REGISTER:
			g_gtsvr->cli_conn->CM_REGISTER(m_datasock, m_recvbuf);
			break;
		case CMD_CP_CHANGEPASS:
			g_gtsvr->cli_conn->CP_CHANGEPASS(m_datasock, m_recvbuf);
			break;
		case CMD_CM_RSA_HANDSHAKE_1:
			g_gtsvr->cli_conn->CM_RSA_HANDSHAKE1(m_datasock, m_recvbuf);
		}
	} catch (std::exception& e) {
		LG("GateServer", "exception: %s\n", e.what());
		LG("GateServer", "IsReady = false exception:%d\n", l_cmd);
	} catch (...) {
		try {
			g_gtsvr->cli_conn->Disconnect(m_datasock, 50, -27);
		} catch (...) {
			LG("gate_error", "Exception during emergency disconnect in ToClient\n");
		}
		LG("GateServer", "IsReady = false exception:%d\n", l_cmd);
	}

	--(g_gtsvr->cli_conn->m_calltotal);
	// l_line<<newln<<"st:"<<l_cmd;
	return 0;
}

void ToClient::CM_LOGIN(DataSocket* datasock, RPacket& recvbuf) {
	// Reduced timeout from 30s to 10s to prevent thread exhaustion
	uLong l_ulMilliseconds = 10 * 1000;
	uLong l_tick = GetTickCount() - recvbuf.GetTickCount();

	if (l_ulMilliseconds > l_tick) {
		l_ulMilliseconds = l_ulMilliseconds - l_tick;

		if (m_version != recvbuf.ReverseReadShort()) {
			WPacket l_wpk = GetWPacket();
			l_wpk.WriteCmd(CMD_MC_LOGIN);
			l_wpk.WriteShort(ERR_MC_VER_ERROR); // 错误码
			SendData(datasock, l_wpk);			// 发给客户端
			// l_line<<newln<<"客户端: "<<datasock->GetPeerIP()<<"	登陆错误：客户端的版本号与服务器不匹配!";
			LG("GateServer", "client: %s\tlogin error: client and server can't match!\n", datasock->GetPeerIP());
			Disconnect(datasock, 100, -31);
			return;
		}
		
		ToGroupServer* l_gps = g_gtsvr->gp_conn;
		Player* l_ply = reinterpret_cast<Player*>(datasock->GetPointer());

		recvbuf.DiscardLast(static_cast<uLong>(sizeof(uShort)));

		if (!l_ply) // 组织重复进入
		{
			return;
		}

		auto plaintext = ReadPacketSequenceEncrypted(recvbuf, l_ply->m_AESKey);
		WPacket l_wpk = WPacket(recvbuf).Duplicate();
		l_wpk.WriteCmd(CMD_TP_USER_LOGIN);
		l_wpk.WriteSequence((cChar*)plaintext.data(), plaintext.size());
		l_wpk.WriteLong(inet_addr(datasock->GetPeerIP()));
		l_wpk.WriteLongLong(MakeULong(l_ply)); // 附加上在GateServer上的内存地址

		RPacket l_rpk = l_gps->SyncCall(l_gps->get_datasock(), l_wpk, l_ulMilliseconds);
		
		uShort l_errno = 0;
		if (l_rpk.HasData() == 0) {
			l_wpk = GetWPacket();
			l_wpk.WriteCmd(CMD_MC_LOGIN);
			l_wpk.WriteShort(ERR_MC_NETEXCP); // 错误码
			SendData(datasock, l_wpk);		  // 发给客户端
			LG("GateServer", "client: %s\tlogin error: GroupServer is disresponse!\n", datasock->GetPeerIP());
			Disconnect(datasock, 100, -31);
		} else if (l_errno = l_rpk.ReadShort()) {
			l_wpk = l_rpk;
			l_wpk.WriteCmd(CMD_MC_LOGIN);
			SendData(datasock, l_wpk);
			// l_line<<newln<<"客户端: "<<datasock->GetPeerIP()<<"	登陆失败，错误码："<<l_errno<<endln;
			LG("GateServer", "client: %s\tlogin error, error:%d\n", datasock->GetPeerIP(), l_errno);
			Disconnect(datasock, 100, -31);
		} else {
			l_ply->m_status = 1; // 置于选/建/删角色状态

			l_ply->gp_addr = l_rpk.ReverseReadLongLong();	// ���������GroupServer�ϵ��ڴ��ַ
			l_ply->m_loginID = l_rpk.ReverseReadLong(); //  Account DB id
			l_ply->m_actid = l_rpk.ReverseReadLong();
			
			BYTE byPassword = l_rpk.ReverseReadChar();
			l_rpk.DiscardLast(sizeof(LLong) + 2 * sizeof(uLong) + sizeof(char));

			l_wpk = WPacket(l_rpk).Duplicate();
			l_wpk.WriteCmd(CMD_MC_LOGIN);
			l_wpk.WriteChar(byPassword);
			l_wpk.WriteLong(0x3214);
			SendData(datasock, l_wpk);
			// l_line<<newln<<"客户端: "<<datasock->GetPeerIP()<<"	登陆成功。"<<endln;
			LG("GateServer", "client: %s\tlogin ok.\n", datasock->GetPeerIP());

			// 开始加密
			l_ply->enc = true;
		}
	} else {
		WPacket l_wpk = GetWPacket();
		l_wpk.WriteCmd(CMD_MC_LOGIN);
		l_wpk.WriteShort(ERR_MC_NETEXCP); // 错误码
		SendData(datasock, l_wpk);		  // 发给客户端
		// l_line<<newln<<"客户端: "<<datasock->GetPeerIP()<<"	登陆错误：包在队列中已经超时!"<<endln;
		LG("GateServer", "client: %s\tlogin error: packet time out!\n", datasock->GetPeerIP());
		Disconnect(datasock, 100, -31);
	}
}

WPacket ToClient::CM_LOGOUT(DataSocket* datasock, RPacket& recvbuf) {
	uLong l_ulMilliseconds = 10 * 1000;
	uLong l_tick = GetTickCount() - recvbuf.GetTickCount();
	l_ulMilliseconds = (l_ulMilliseconds > l_tick) ? l_ulMilliseconds - l_tick : 1;

	WPacket l_retpk = 0;
	Player* l_ply = 0;
	MutexArmor lock(g_gtsvr->_mtxother);
	l_ply = (Player*)datasock->GetPointer();

	if (l_ply)
		l_ply->m_datasock = nullptr;

	datasock->SetPointer(0);
	lock.unlock();

	if (l_ply) {
		// Add by lark.li 20081119 begin
		l_ply->EndRun();
		// End

		MutexArmor l_lockStat(l_ply->m_mtxstat);

		try {
			if (l_ply->m_status == 0) {
				WPacket l_wpk = datasock->GetWPacket();
				l_retpk.WriteShort(ERR_MC_NOTLOGIN);
				// Should probably free() our player...
				l_ply->Free();
				return l_retpk;
			}

			{ // GateServer -> GroupServer
				WPacket l_wpk = GetWPacket();
				l_wpk.WriteCmd(CMD_TP_DISC);
				l_wpk.WriteLong(l_ply->m_actid);
				l_wpk.WriteLong(inet_addr(datasock->GetPeerIP()));
				l_wpk.WriteString(GetDisconnectErrText(datasock->GetDisconnectReason() ? datasock->GetDisconnectReason() : -27).c_str());
				g_gtsvr->gp_conn->SendData(g_gtsvr->gp_conn->get_datasock(), l_wpk);
			}

			GameServer* l_game = l_ply->game;
			if ((l_ply->m_status == 2) && l_ply->gm_addr && l_game && l_game->m_datasock) {
				LG("GateServer", "client: %s:%d\tGoOut map,Gate to [%s]send GoOutMap command,dbid:%u,Gate address:%llX,Game address:%lld\n",
					datasock->GetPeerIP(), datasock->GetPeerPort(),
					l_game->m_datasock->GetPeerIP(), l_ply->m_dbid, MakeULong(l_ply), l_ply->gm_addr);

				{ // GateServer -> GameServer
					WPacket l_wpk = l_game->m_datasock->GetWPacket();
					l_wpk.WriteCmd(CMD_TM_GOOUTMAP);
					l_wpk.WriteChar(0);
					l_wpk.WriteLongLong(MakeULong(l_ply));
					l_wpk.WriteLongLong(l_ply->gm_addr); // ��������GameServer�ϵĵ�ַ
					SendData(l_game->m_datasock, l_wpk);
				}

				l_ply->game = 0;	// ��ֹ����ĵ�GameServer������
				l_ply->gm_addr = 0; // ��ֹ����ĵ�GameServer������
			}

			{ // ֪ͨGroupServer Logout
				WPacket l_wpk = g_gtsvr->gp_conn->GetWPacket();
				l_wpk.WriteCmd(CMD_TP_USER_LOGOUT);
				l_wpk.WriteLongLong(MakeULong(l_ply));
				l_wpk.WriteLongLong(l_ply->gp_addr);
				l_ply->gp_addr = 0;
				DataSocket* gp_ds = g_gtsvr->gp_conn->get_datasock();
				if (gp_ds) {
					l_retpk = g_gtsvr->gp_conn->SyncCall(gp_ds, l_wpk, l_ulMilliseconds);
				}
			}
		} catch (...) {
			LG("GateServer", "Error exit!\n");
		}
		l_lockStat.unlock();
		l_ply->Free();
	}
	return l_retpk;
}

void ToClient::CM_BGNPLAY(DataSocket* datasock, RPacket& recvbuf) {
	uLong l_ulMilliseconds = 10 * 1000;
	uLong l_tick = GetTickCount() - recvbuf.GetTickCount();
	if (l_ulMilliseconds > l_tick) {
		l_ulMilliseconds = l_ulMilliseconds - l_tick;

		Player* l_ply = (Player*)datasock->GetPointer();
		if (l_ply) {
			MutexArmor l_lockStat(l_ply->m_mtxstat);
			
			if (l_ply->m_status != 1 || !l_ply->gp_addr) // 发出这个命令时机非法，因为当前玩家不处于选角色状态，不能选择另外一个角色
			{
				WPacket l_wpk = datasock->GetWPacket();
				l_wpk.WriteCmd(CMD_MC_BGNPLAY);
				l_wpk.WriteShort(ERR_MC_NOTSELCHA);
				SendData(datasock, l_wpk);
			} else {
				// 验证所玩角色合法性
				WPacket l_wpk = WPacket(recvbuf).Duplicate();
				l_wpk.WriteCmd(CMD_TP_BGNPLAY);
				l_wpk.WriteLongLong(MakeULong(l_ply));
				l_wpk.WriteLongLong(l_ply->gp_addr);
				RPacket l_rpk = g_gtsvr->gp_conn->SyncCall(g_gtsvr->gp_conn->get_datasock(), l_wpk, l_ulMilliseconds);
				uShort l_errno;
				if (!l_rpk.HasData()) // 网络错误
				{
					l_wpk = datasock->GetWPacket();
					l_wpk.WriteCmd(CMD_MC_BGNPLAY);
					l_wpk.WriteShort(ERR_MC_NETEXCP);
					SendData(datasock, l_wpk);
				} else if (l_errno = l_rpk.ReadShort()) // 所玩角色不合法
				{
					l_wpk = l_rpk;
					l_wpk.WriteCmd(CMD_MC_BGNPLAY);
					SendData(datasock, l_wpk);
					if (l_errno == ERR_PT_KICKUSER) {
						Disconnect(datasock, 100, -25);
					}
				} else // 选择角色成功，返回成功消息，并且把地图名和成功的角色ID发给GameServer。
				{
					// Modify by lark.li 20080317
					memset(l_ply->m_password, 0, sizeof(l_ply->m_password));
					strncpy(l_ply->m_password, l_rpk.ReadString(), ROLE_MAXSIZE_PASSWORD2); // 角色二次密码
					// End

					l_ply->m_dbid = l_rpk.ReadLong();	 // 角色ID;
					l_ply->m_worldid = l_rpk.ReadLong(); // GroupServer分配的唯一ID
					cChar* l_map = l_rpk.ReadString();
					l_ply->m_sGarnerWiner = l_rpk.ReadShort();
					GameServer* l_game = g_gtsvr->gm_conn->find(l_map);
					if (!l_game) // 目标地图不可达
					{
						l_wpk = datasock->GetWPacket();
						l_wpk.WriteCmd(CMD_MC_BGNPLAY);
						l_wpk.WriteShort(ERR_MC_NOTARRIVE);
						SendData(datasock, l_wpk);
					} else if (l_game->m_plynum > 15000) // 人数过多
					{
						l_wpk = datasock->GetWPacket();
						l_wpk.WriteCmd(CMD_MC_BGNPLAY);
						l_wpk.WriteShort(ERR_MC_TOOMANYPLY);
						SendData(datasock, l_wpk);
					} else {
						// clean codes later
						datasock->m_gsCheck = 0;
						short totalgs = 0;

						// Count GameServers first to budget timeout per server
						GameServer* _game_list = g_gtsvr->gm_conn->GetGameList();
						for (GameServer* t_game = _game_list; t_game; t_game = t_game->next) {
							totalgs++;
						}

						// Budget remaining time across all GameServers (min 500ms each, capped to total budget)
						uLong kickBudget = l_ulMilliseconds;
						uLong kickTimeoutPerGS = (totalgs > 0) ? (kickBudget / totalgs) : kickBudget;
						if (kickTimeoutPerGS < 500) kickTimeoutPerGS = 500;

						WPacket wpk = datasock->GetWPacket();
						wpk.WriteCmd(CMD_TM_KICKCHA);
						wpk.WriteLong(l_ply->m_dbid);
						uLong kickLoopStart = GetTickCount();
						for (GameServer* t_game = _game_list; t_game; t_game = t_game->next) {
							uLong kickElapsed = GetTickCount() - kickLoopStart;
							if (kickElapsed >= kickBudget) break; // Budget exhausted
							uLong kickRemaining = kickBudget - kickElapsed;
							uLong thisKickTimeout = (kickTimeoutPerGS < kickRemaining) ? kickTimeoutPerGS : kickRemaining;
							if (thisKickTimeout < 500) thisKickTimeout = 500;
							RPacket l_rpk = g_gtsvr->gm_conn->SyncCall(t_game->m_datasock, wpk, thisKickTimeout);
							if (l_rpk.HasData()) {
								char isFound = l_rpk.ReadChar();
								datasock->m_gsCheck++;
							}
						}
						if (totalgs == datasock->m_gsCheck) {
							l_ply->m_status = 2; // 选择角色成功，置于玩游戏状态
							l_game->m_plynum++;	 // 不用同步，只是简单参考

							// 通知GameServer进入地图
							// l_line<<newln<<"客户端: "<<datasock->GetPeerIP()<<":"<<datasock->GetPeerPort()<<"	BeginPlay入地图,Gate向["
							//<<l_game->m_datasock->GetPeerIP()<<"]发送了EnterMap命令,dbid:"<<l_ply->m_dbid
							//<<uppercase<<hex<<",附带Gate地址:"<<MakeULong(l_ply)<<dec<<nouppercase<<endln;
							LG("GateServer", "client: %s:%d\tBeginPlay entry map,Gate to[%s]send EnterMap command,dbid:%u,Gate address:%llX\n",
								datasock->GetPeerIP(), datasock->GetPeerPort(),
								l_game->m_datasock->GetPeerIP(), l_ply->m_dbid, MakeULong(l_ply));
							l_game->EnterMap(l_ply, l_ply->m_loginID, l_ply->m_dbid, l_ply->m_worldid, l_map, -1, 0, 0, 0, l_ply->m_sGarnerWiner); // 根据地图查找GameServer，然后请求GameServer以进入这个地图。
						}
					}
				}
			}
			l_lockStat.unlock();
		}
	} else {
		WPacket l_wpk = datasock->GetWPacket();
		l_wpk.WriteCmd(CMD_MC_BGNPLAY);
		l_wpk.WriteShort(ERR_MC_NETEXCP);
		SendData(datasock, l_wpk);
	}
}

void ToClient::CM_ENDPLAY(DataSocket* datasock, RPacket& recvbuf) {
	uLong l_ulMilliseconds = 10 * 1000;
	uLong l_tick = GetTickCount() - recvbuf.GetTickCount();
	l_ulMilliseconds = (l_ulMilliseconds > l_tick) ? l_ulMilliseconds - l_tick : 1;

	Player* l_ply = (Player*)datasock->GetPointer();
	if (l_ply) {
		MutexArmor l_lockStat(l_ply->m_mtxstat);

		if (l_ply->m_status != 2 || !l_ply->gm_addr)
		{
			// Player is not in-game (duplicate EndPlay or race condition) — send error but don't disconnect.
			// The client will handle the error gracefully and fall back to the login scene.
			LG("GateServer", "CM_ENDPLAY: player %s status=%d gm_addr=%lld, not in play state\n",
				datasock->GetPeerIP(), l_ply->m_status, (long long)l_ply->gm_addr);
			WPacket l_wpk = datasock->GetWPacket();
			l_wpk.WriteCmd(CMD_MC_ENDPLAY);
			l_wpk.WriteShort(ERR_MC_NOTPLAY);
			SendData(datasock, l_wpk);
		} else {
			GameServer* l_game = l_ply->game;
			if (l_game && l_game->m_datasock) {
				l_ply->m_status = 1; // ����ѡ��ɫ����״̬

				l_game->m_plynum--;
				// ֪ͨGameServer����ͼ
				LG("GateServer", "client: %s:%d\tGoOut map,Gate to[%s] send GoOutMap command,dbid:%u,Gate address:%llX,Game address:%lld\n",
					datasock->GetPeerIP(), datasock->GetPeerPort(),
					l_game->m_datasock->GetPeerIP(), l_ply->m_dbid, MakeULong(l_ply), l_ply->gm_addr);
				WPacket l_wpk = WPacket(recvbuf).Duplicate();
				l_wpk.WriteCmd(CMD_TM_GOOUTMAP);

				l_wpk.WriteChar(0);

				l_wpk.WriteLongLong(MakeULong(l_ply));
				l_wpk.WriteLongLong(l_ply->gm_addr); // ����GameServer�ϵĵ�ַ

				l_ply->game = 0;	// ��ֹ����ĵ�GameServer������
				l_ply->gm_addr = 0; // ��ֹ����ĵ�GameServer������

				g_gtsvr->gm_conn->SendData(l_game->m_datasock, l_wpk);
				// g_gtsvr->gm_conn->SyncCall(l_game->m_datasock,l_wpk);
				// ֪ͨGroupServer��������Ϸ

				l_wpk = WPacket(recvbuf).Duplicate();
				l_wpk.WriteCmd(CMD_TP_ENDPLAY);
				l_wpk.WriteLongLong(MakeULong(l_ply));
				l_wpk.WriteLongLong(l_ply->gp_addr);
				LG("GateServer", "CMD_TP_ENDPLAY: l_ply=%p gp_addr=0x%llx\n", l_ply, (unsigned long long)l_ply->gp_addr);
				l_wpk = g_gtsvr->gp_conn->SyncCall(g_gtsvr->gp_conn->get_datasock(), l_wpk, l_ulMilliseconds);
				if (!l_wpk.HasData()) {
					LG("GateServer", "CMD_TP_ENDPLAY: SyncCall TIMEOUT - no data!\n");
					l_wpk = datasock->GetWPacket();
					l_wpk.WriteCmd(CMD_MC_ENDPLAY);
					l_wpk.WriteShort(ERR_MC_NETEXCP);
					SendData(datasock, l_wpk);
				} else {
					uShort err = RPacket(l_wpk).ReadShort();
					LG("GateServer", "CMD_TP_ENDPLAY: SyncCall returned err=%d\n", err);
					if (err == ERR_PT_INERR || err == ERR_PT_KICKUSER) {
						LG("GateServer", "CMD_TP_ENDPLAY: GroupServer error=%d\n", err);
						WPacket errpk = datasock->GetWPacket();
						errpk.WriteCmd(CMD_MC_ENDPLAY);
						errpk.WriteShort(err);
						SendData(datasock, errpk);
					} else {
						// ���ظ�Client
						l_wpk.WriteCmd(CMD_MC_ENDPLAY);
						SendData(datasock, l_wpk);
						l_ply->m_dbid = 0;
						l_ply->m_worldid = 0;
					}
				}
			}
		}
		l_lockStat.unlock();
	}
}

void ToClient::CP_CHANGEPASS(DataSocket* datasock, RPacket& recvbuf) {
	uLong l_ulMilliseconds = 10 * 1000;
	uLong l_tick = GetTickCount() - recvbuf.GetTickCount();
	if (l_ulMilliseconds > l_tick) {
		l_ulMilliseconds = l_ulMilliseconds - l_tick;

		Player* l_ply = (Player*)datasock->GetPointer();
		if (l_ply) {
			MutexArmor l_lockStat(l_ply->m_mtxstat);
			WPacket l_wpk = WPacket(recvbuf).Duplicate();
			l_wpk.WriteCmd(CMD_TP_CHANGEPASS);
			l_wpk.WriteLongLong(MakeULong(l_ply));
			l_wpk.WriteLongLong(l_ply->gp_addr);
			l_wpk = g_gtsvr->gp_conn->SyncCall(g_gtsvr->gp_conn->get_datasock(), l_wpk, l_ulMilliseconds);
			if (l_wpk.HasData()) {
				SendData(datasock, l_wpk);
			} else {
				// SyncCall timed out — notify client so the password change dialog doesn't hang
				l_wpk = datasock->GetWPacket();
				l_wpk.WriteCmd(CMD_MC_LOGIN);
				l_wpk.WriteShort(ERR_MC_NETEXCP);
				SendData(datasock, l_wpk);
			}
			l_lockStat.unlock();
		}
	} else {
		Disconnect(datasock, 100, -25);
	}
}

void ToClient::CM_REGISTER(DataSocket* datasock, RPacket& recvbuf) {
	uLong l_ulMilliseconds = 10 * 1000;
	uLong l_tick = GetTickCount() - recvbuf.GetTickCount();
	if (l_ulMilliseconds > l_tick) {
		l_ulMilliseconds = l_ulMilliseconds - l_tick;

		Player* l_ply = (Player*)datasock->GetPointer();
		if (l_ply) {
			MutexArmor l_lockStat(l_ply->m_mtxstat);
			WPacket l_wpk = WPacket(recvbuf).Duplicate();
			l_wpk.WriteCmd(CMD_TP_REGISTER);
			l_wpk.WriteLongLong(MakeULong(l_ply));
			l_wpk.WriteLongLong(l_ply->gp_addr);
			l_wpk = g_gtsvr->gp_conn->SyncCall(g_gtsvr->gp_conn->get_datasock(), l_wpk, l_ulMilliseconds);
			if (l_wpk.HasData()) {
				l_wpk.WriteCmd(CMD_PC_REGISTER);
				SendData(datasock, l_wpk);
			} else {
				// SyncCall timed out — send error so client doesn't hang
				l_wpk = datasock->GetWPacket();
				l_wpk.WriteCmd(CMD_MC_LOGIN);
				l_wpk.WriteShort(ERR_MC_NETEXCP);
				SendData(datasock, l_wpk);
				Disconnect(datasock, 100, -25);
			}
		}
	} else {
		WPacket l_wpk = datasock->GetWPacket();
		l_wpk.WriteCmd(CMD_MC_LOGIN);
		l_wpk.WriteShort(ERR_MC_NETEXCP);
		SendData(datasock, l_wpk);
		Disconnect(datasock, 100, -25);
	}
}

void ToClient::CM_NEWCHA(DataSocket* datasock, RPacket& recvbuf) {
	uLong l_ulMilliseconds = 10 * 1000;
	uLong l_tick = GetTickCount() - recvbuf.GetTickCount();
	if (l_ulMilliseconds > l_tick) {
		l_ulMilliseconds = l_ulMilliseconds - l_tick;

		Player* l_ply = (Player*)datasock->GetPointer();
		if (l_ply) {
			MutexArmor l_lockStat(l_ply->m_mtxstat);
			if (l_ply->m_status != 1 || !l_ply->gp_addr) {
				WPacket l_wpk = datasock->GetWPacket();
				l_wpk.WriteCmd(CMD_MC_NEWCHA);
				l_wpk.WriteShort(ERR_MC_NOTSELCHA);
				SendData(datasock, l_wpk);
			} else {
				// 调用GroupServer
				WPacket l_wpk = WPacket(recvbuf).Duplicate();
				l_wpk.WriteCmd(CMD_TP_NEWCHA);
				l_wpk.WriteLongLong(MakeULong(l_ply));
				l_wpk.WriteLongLong(l_ply->gp_addr); // 附带地址
				l_wpk = g_gtsvr->gp_conn->SyncCall(g_gtsvr->gp_conn->get_datasock(), l_wpk, l_ulMilliseconds);
				if (!l_wpk.HasData()) {
					l_wpk = datasock->GetWPacket();
					l_wpk.WriteCmd(CMD_MC_NEWCHA);
					l_wpk.WriteShort(ERR_MC_NETEXCP);
					SendData(datasock, l_wpk);
					Disconnect(datasock, 100, -25);
				} else {
					// 返回Client
					l_wpk.WriteCmd(CMD_MC_NEWCHA);
					SendData(datasock, l_wpk);
					if (RPacket(l_wpk).ReadShort() == ERR_PT_KICKUSER) {
						Disconnect(datasock, 100, -25);
					}
				}
			}
			l_lockStat.unlock();
		}
	} else {
		WPacket l_wpk = datasock->GetWPacket();
		l_wpk.WriteCmd(CMD_MC_NEWCHA);
		l_wpk.WriteShort(ERR_MC_NETEXCP);
		SendData(datasock, l_wpk);
		Disconnect(datasock, 100, -25);
	}
}
void ToClient::CM_DELCHA(DataSocket* datasock, RPacket& recvbuf) {
	uLong l_ulMilliseconds = 10 * 1000;
	uLong l_tick = GetTickCount() - recvbuf.GetTickCount();
	if (l_ulMilliseconds > l_tick) {
		l_ulMilliseconds = l_ulMilliseconds - l_tick;

		Player* l_ply = (Player*)datasock->GetPointer();
		if (l_ply) {
			MutexArmor l_lockStat(l_ply->m_mtxstat);
			if (l_ply->m_status != 1 || !l_ply->gp_addr) {
				WPacket l_wpk = datasock->GetWPacket();
				l_wpk.WriteCmd(CMD_MC_DELCHA);
				l_wpk.WriteShort(ERR_MC_NOTSELCHA);
				SendData(datasock, l_wpk);
			} else {
				// ����GroupServer
				dbc::uShort cha_name_len;
				const auto cha_name = recvbuf.ReadSequence(cha_name_len);
				const auto password = ReadPacketSequenceEncrypted(recvbuf, l_ply->m_AESKey);

				auto l_wpk = datasock->GetWPacket();
				l_wpk.WriteCmd(CMD_TP_DELCHA);
				l_wpk.WriteSequence(cha_name, cha_name_len);
				l_wpk.WriteSequence((cChar*)password.data(), password.size());
				l_wpk.WriteLongLong(MakeULong(l_ply));
				l_wpk.WriteLongLong(l_ply->gp_addr); // ������ַ
				l_wpk = g_gtsvr->gp_conn->SyncCall(g_gtsvr->gp_conn->get_datasock(), l_wpk, l_ulMilliseconds);
				if (!l_wpk.HasData()) {
					l_wpk = datasock->GetWPacket();
					l_wpk.WriteCmd(CMD_MC_DELCHA);
					l_wpk.WriteShort(ERR_MC_NETEXCP);
					SendData(datasock, l_wpk);
					Disconnect(datasock, 100, -25);
				} else {
					// ����Client
					l_wpk.WriteCmd(CMD_MC_DELCHA);
					SendData(datasock, l_wpk);
					if (RPacket(l_wpk).ReadShort() == ERR_PT_KICKUSER) {
						Disconnect(datasock, 100, -25);
					}
				}
			}
			l_lockStat.unlock();
		}
	} else {
		WPacket l_wpk = datasock->GetWPacket();
		l_wpk.WriteCmd(CMD_MC_DELCHA);
		l_wpk.WriteShort(ERR_MC_NETEXCP);
		SendData(datasock, l_wpk);
		Disconnect(datasock, 100, -25);
	}
}

void ToClient::CM_CREATE_PASSWORD2(DataSocket* datasock, RPacket& recvbuf) {
	uLong l_ulMilliseconds = 10 * 1000;
	uLong l_tick = GetTickCount() - recvbuf.GetTickCount();
	if (l_ulMilliseconds > l_tick) {
		l_ulMilliseconds = l_ulMilliseconds - l_tick;

		Player* l_ply = (Player*)datasock->GetPointer();
		if (l_ply) {
			MutexArmor l_lockStat(l_ply->m_mtxstat);
			if (l_ply->m_status != 1 || !l_ply->gp_addr) {
				WPacket l_wpk = datasock->GetWPacket();
				l_wpk.WriteCmd(CMD_MC_DELCHA);
				l_wpk.WriteShort(ERR_MC_NOTSELCHA);
				SendData(datasock, l_wpk);
			} else {
				// ����GroupServer
				auto password = ReadPacketSequenceEncrypted(recvbuf, l_ply->m_AESKey);
				auto l_wpk = datasock->GetWPacket();
				l_wpk.WriteCmd(CMD_TP_CREATE_PASSWORD2);
				l_wpk.WriteSequence((cChar*)password.data(), password.size());
				l_wpk.WriteLongLong(MakeULong(l_ply));
				l_wpk.WriteLongLong(l_ply->gp_addr); // ������ַ
				l_wpk = g_gtsvr->gp_conn->SyncCall(g_gtsvr->gp_conn->get_datasock(), l_wpk, l_ulMilliseconds);
				if (!l_wpk.HasData()) {
					l_wpk = datasock->GetWPacket();
					l_wpk.WriteCmd(CMD_MC_CREATE_PASSWORD2);
					l_wpk.WriteShort(ERR_MC_NETEXCP);
					SendData(datasock, l_wpk);
					Disconnect(datasock, 100, -25);
				} else {
					// ����Client
					l_wpk.WriteCmd(CMD_MC_CREATE_PASSWORD2);
					SendData(datasock, l_wpk);
					if (RPacket(l_wpk).ReadShort() == ERR_PT_KICKUSER) {
						Disconnect(datasock, 100, -25);
					}
				}
			}
			l_lockStat.unlock();
		}
	} else {
		WPacket l_wpk = datasock->GetWPacket();
		l_wpk.WriteCmd(CMD_MC_CREATE_PASSWORD2);
		l_wpk.WriteShort(ERR_MC_NETEXCP);
		SendData(datasock, l_wpk);
		Disconnect(datasock, 100, -25);
	}
}

void ToClient::CM_UPDATE_PASSWORD2(DataSocket* datasock, RPacket& recvbuf) {
	uLong l_ulMilliseconds = 10 * 1000;
	uLong l_tick = GetTickCount() - recvbuf.GetTickCount();
	if (l_ulMilliseconds > l_tick) {
		l_ulMilliseconds = l_ulMilliseconds - l_tick;

		Player* l_ply = (Player*)datasock->GetPointer();
		if (l_ply) {
			MutexArmor l_lockStat(l_ply->m_mtxstat);
			if (l_ply->m_status != 1 || !l_ply->gp_addr) {
				WPacket l_wpk = datasock->GetWPacket();
				l_wpk.WriteCmd(CMD_MC_DELCHA);
				l_wpk.WriteShort(ERR_MC_NOTSELCHA);
				SendData(datasock, l_wpk);
			} else {
				// ����GroupServer

				const auto old_password = ReadPacketSequenceEncrypted(recvbuf, l_ply->m_AESKey);
				const auto new_password = ReadPacketSequenceEncrypted(recvbuf, l_ply->m_AESKey);
				auto l_wpk = datasock->GetWPacket();
				l_wpk.WriteCmd(CMD_TP_UPDATE_PASSWORD2);
				l_wpk.WriteSequence((cChar*)old_password.data(), old_password.size());
				l_wpk.WriteSequence((cChar*)new_password.data(), new_password.size());
				l_wpk.WriteLongLong(MakeULong(l_ply));
				l_wpk.WriteLongLong(l_ply->gp_addr); // ������ַ
				l_wpk = g_gtsvr->gp_conn->SyncCall(g_gtsvr->gp_conn->get_datasock(), l_wpk, l_ulMilliseconds);
				if (!l_wpk.HasData()) {
					l_wpk = datasock->GetWPacket();
					l_wpk.WriteCmd(CMD_MC_UPDATE_PASSWORD2);
					l_wpk.WriteShort(ERR_MC_NETEXCP);
					SendData(datasock, l_wpk);
					Disconnect(datasock, 100, -25);
				} else {
					// ����Client
					l_wpk.WriteCmd(CMD_MC_UPDATE_PASSWORD2);
					SendData(datasock, l_wpk);
					if (RPacket(l_wpk).ReadShort() == ERR_PT_KICKUSER) {
						Disconnect(datasock, 100, -25);
					}
				}
			}
			l_lockStat.unlock();
		}
	} else {
		WPacket l_wpk = datasock->GetWPacket();
		l_wpk.WriteCmd(CMD_MC_UPDATE_PASSWORD2);
		l_wpk.WriteShort(ERR_MC_NETEXCP);
		SendData(datasock, l_wpk);
		Disconnect(datasock, 100, -25);
	}
}

// Remain should be > 0, to give server a chance to send packet
void ToClient::TC_DISCONNECT(dbc::DataSocket* datasock, int reason, int remain) {
	if (!datasock) {
		return;
	}

	Disconnect(datasock, remain, reason);

	auto wpk = GetWPacket();
	wpk.WriteCmd(CMD_MC_DISCONNECT);
	wpk.WriteLong(reason);
	g_gtsvr->cli_conn->SendData(datasock, wpk);
}

void ToClient::OnEncrypt(DataSocket* datasock, char* ciphertext, const char* text, uLong& len) {
	TcpCommApp::OnEncrypt(datasock, ciphertext, text, len);

	if (_comm_enc > 0) {
		auto ply = static_cast<Player*>(datasock->GetPointer());

		if (ply && ply->enc) {
			try {
				// Use AES-128/CTR for stream encryption (no padding needed)
				auto aes_enc = Botan::Cipher_Mode::create("AES-128/CTR", Botan::ENCRYPTION);
				if (!aes_enc) {
					LG("ErrServer", "[%s] OnEncrypt: Failed to create cipher\n", datasock->GetPeerIP());
					return;
				}
				
				aes_enc->set_key(ply->m_AESKey, AES_KEY_LENGTH);
				aes_enc->start(ply->m_IV, AES_IV_LENGTH);
				
				// CTR mode doesn't change length, encrypt in place
				Botan::secure_vector<uint8_t> buffer((uint8_t*)text, (uint8_t*)text + len);
				aes_enc->finish(buffer);
				
				memcpy(ciphertext, buffer.data(), len);
			} catch (const Botan::Exception& e) {
				LG("ErrServer", "[%s] OnEncrypt Botan Error: %s\n", datasock->GetPeerIP(), e.what());
			} catch (...) {
				LG("ErrServer", "[%s] OnEncrypt Unknown Error!\n", datasock->GetPeerIP());
			}
		}
	}

	return;
}

void ToClient::OnDecrypt(DataSocket* datasock, char* ciphertext, uLong& len) {
	TcpCommApp::OnDecrypt(datasock, ciphertext, len);

	if (_comm_enc > 0) {
		auto ply = static_cast<Player*>(datasock->GetPointer());
		if (ply && ply->enc) {
			try {
				// Use AES-128/CTR for stream decryption (same as encryption in CTR mode)
				auto aes_dec = Botan::Cipher_Mode::create("AES-128/CTR", Botan::DECRYPTION);
				if (!aes_dec) {
					LG("ErrServer", "[%s] OnDecrypt: Failed to create cipher\n", datasock->GetPeerIP());
					Disconnect(datasock, 0, -32); // Graceful disconnect
					return;
				}
				
				aes_dec->set_key(ply->m_AESKey, AES_KEY_LENGTH);
				aes_dec->start(ply->m_IV, AES_IV_LENGTH);
				
				// CTR mode doesn't change length, decrypt in place
				Botan::secure_vector<uint8_t> buffer((uint8_t*)ciphertext, (uint8_t*)ciphertext + len);
				aes_dec->finish(buffer);
				
				memcpy(ciphertext, buffer.data(), len);
			} catch (const Botan::Exception& e) {
				LG("ErrServer", "[%s] OnDecrypt Botan Error: %s\n", datasock->GetPeerIP(), e.what());
				Disconnect(datasock, 0, -32); // Graceful disconnect on decryption failure
			} catch (...) {
				LG("ErrServer", "[%s] OnDecrypt Unknown Error!\n", datasock->GetPeerIP());
				Disconnect(datasock, 0, -32); // Graceful disconnect on error
			}
		}
	}
	return;
}

void ToClient::post_mapcrash_msg(Player* ply) {
	if (ply->m_datasock == nullptr)
		return;
	WPacket pk = ply->m_datasock->GetWPacket();
	pk.WriteCmd(CMD_MC_MAPCRASH);
	// pk.WriteString("��ͼ���������ϣ����Ժ�����...");
	pk.WriteString(RES_STRING(GS_TOCLIENT_CPP_00031));
	SendData(ply->m_datasock, pk);
}
