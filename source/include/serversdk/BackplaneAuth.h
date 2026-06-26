#pragma once

#include "CommRPC.h"
#include "IniFile.h"
#include <cstdint>
#include <vector>

namespace dbc {

constexpr uLong kBackplaneAuthMagic = 0x424B504Bu; // 'BKPK'
constexpr int kBackplaneAuthDisconnectReason = -41;
constexpr uShort kBackplaneAuthNonceSize = 32;
constexpr uShort kBackplaneAuthMacSize = 32;

class RPCMGR;
class TcpCommApp;

struct BackplaneAuthConfig {
	std::vector<uint8_t> psk;
	bool requireAuth = false;
	uLong handshakeTimeoutMs = 10000;

	bool IsEnabled() const { return requireAuth && !psk.empty(); }
};

class BackplaneAuth {
public:
	static BackplaneAuthConfig LoadFromIni(const IniFile& inf);
	static void SetClusterConfig(const BackplaneAuthConfig& cfg);
	static const BackplaneAuthConfig& GetClusterConfig();

	static bool PerformOutboundHandshake(TcpCommApp* app, RPCMGR* rpc, DataSocket* sock);
	static WPacket ServeHello(TcpCommApp* app, DataSocket* sock, RPacket& pk);

	static void OnInboundConnect(DataSocket* sock);
	static void OnSocketClosed(DataSocket* sock);
	static bool AllowProcessData(DataSocket* sock, uShort cmd, TcpCommApp* app);
};

} // namespace dbc
