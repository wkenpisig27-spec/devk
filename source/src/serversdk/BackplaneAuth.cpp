#include "BackplaneAuth.h"
#include "Comm.h"
#include "common/NetCommand.h"
#include "util/log.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <botan/auto_rng.h>
#include <botan/mac.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

_DBC_USING

namespace {

BackplaneAuthConfig g_clusterConfig;

enum class BackplaneState : uint8_t {
	Legacy = 0,
	Pending,
	Authenticated,
};

struct BackplaneConnEntry {
	BackplaneState state = BackplaneState::Legacy;
	uLong connectTick = 0;
};

std::mutex g_backplaneMutex;
std::unordered_map<DataSocket*, BackplaneConnEntry> g_backplaneConns;

static std::string Trim(const std::string& str) {
	size_t start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

static uLong NowMs() {
	return static_cast<uLong>(
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch())
			.count());
}

static BackplaneConnEntry& EntryFor(DataSocket* sock) {
	return g_backplaneConns[sock];
}

static void ComputeMac(const std::vector<uint8_t>& psk, const char* label, const uint8_t* a, size_t aLen,
	const uint8_t* b, size_t bLen, uint8_t out[kBackplaneAuthMacSize]) {
	auto mac = Botan::MessageAuthenticationCode::create_or_throw("HMAC(SHA-256)");
	mac->set_key(psk.data(), psk.size());
	mac->update(reinterpret_cast<const uint8_t*>(label), std::strlen(label));
	if (a && aLen)
		mac->update(a, aLen);
	if (b && bLen)
		mac->update(b, bLen);
	Botan::secure_vector<uint8_t> full = mac->final();
	std::memcpy(out, full.data(), kBackplaneAuthMacSize);
}

static bool IsAuthenticated(DataSocket* sock) {
	std::lock_guard<std::mutex> lock(g_backplaneMutex);
	auto it = g_backplaneConns.find(sock);
	if (it == g_backplaneConns.end())
		return !g_clusterConfig.IsEnabled();
	return it->second.state == BackplaneState::Legacy || it->second.state == BackplaneState::Authenticated;
}

static void MarkAuthenticated(DataSocket* sock) {
	std::lock_guard<std::mutex> lock(g_backplaneMutex);
	EntryFor(sock).state = BackplaneState::Authenticated;
}

static void MarkLegacy(DataSocket* sock) {
	std::lock_guard<std::mutex> lock(g_backplaneMutex);
	EntryFor(sock).state = BackplaneState::Legacy;
}

} // namespace

BackplaneAuthConfig BackplaneAuth::LoadFromIni(const IniFile& inf) {
	BackplaneAuthConfig cfg;
	try {
		const IniSection& sec = inf["Backplane"];
		const std::string pskRaw = Trim(sec["PSK"]);
		if (!pskRaw.empty()) {
			cfg.psk.assign(pskRaw.begin(), pskRaw.end());
		}
		const std::string require = Trim(sec["RequireAuth"]);
		cfg.requireAuth = (require == "1" || require == "true" || require == "TRUE");
		const std::string timeout = Trim(sec["HandshakeTimeoutMs"]);
		if (!timeout.empty()) {
			cfg.handshakeTimeoutMs = static_cast<uLong>(std::stoul(timeout));
		}
	} catch (...) {
	}
	return cfg;
}

void BackplaneAuth::SetClusterConfig(const BackplaneAuthConfig& cfg) {
	g_clusterConfig = cfg;
	if (cfg.IsEnabled()) {
		LG("BackplaneAuth", "enabled RequireAuth=1 PSK len=%zu timeout=%u ms\n",
		   cfg.psk.size(), static_cast<unsigned>(cfg.handshakeTimeoutMs));
	} else {
		LG("BackplaneAuth", "disabled (legacy accept)\n");
	}
}

const BackplaneAuthConfig& BackplaneAuth::GetClusterConfig() {
	return g_clusterConfig;
}

void BackplaneAuth::OnInboundConnect(DataSocket* sock) {
	if (!g_clusterConfig.IsEnabled()) {
		MarkLegacy(sock);
		return;
	}
	std::lock_guard<std::mutex> lock(g_backplaneMutex);
	auto& entry = EntryFor(sock);
	entry.state = BackplaneState::Pending;
	entry.connectTick = NowMs();
	LG("BackplaneAuth", "inbound pending auth peer=%s:%u\n", sock->GetPeerIP(), sock->GetPeerPort());
}

void BackplaneAuth::OnSocketClosed(DataSocket* sock) {
	std::lock_guard<std::mutex> lock(g_backplaneMutex);
	g_backplaneConns.erase(sock);
}

bool BackplaneAuth::AllowProcessData(DataSocket* sock, uShort cmd, TcpCommApp* app) {
	(void)cmd;
	if (!g_clusterConfig.IsEnabled())
		return true;
	if (IsAuthenticated(sock))
		return true;

	uLong elapsed = 0;
	{
		std::lock_guard<std::mutex> lock(g_backplaneMutex);
		auto it = g_backplaneConns.find(sock);
		if (it == g_backplaneConns.end() || it->second.state != BackplaneState::Pending)
			return true;
		elapsed = NowMs() - it->second.connectTick;
	}

	if (elapsed > g_clusterConfig.handshakeTimeoutMs) {
		LG("BackplaneAuth", "handshake timeout reject peer=%s:%u elapsed=%u ms\n",
		   sock->GetPeerIP(), sock->GetPeerPort(), static_cast<unsigned>(elapsed));
		if (app)
			app->Disconnect(sock, 0, kBackplaneAuthDisconnectReason);
		return false;
	}

	LG("BackplaneAuth", "reject unauthenticated traffic peer=%s:%u cmd=%u\n",
	   sock->GetPeerIP(), sock->GetPeerPort(), static_cast<unsigned>(cmd));
	if (app)
		app->Disconnect(sock, 0, kBackplaneAuthDisconnectReason);
	return false;
}

bool BackplaneAuth::PerformOutboundHandshake(TcpCommApp* app, RPCMGR* rpc, DataSocket* sock) {
	if (!g_clusterConfig.IsEnabled()) {
		MarkLegacy(sock);
		return true;
	}
	if (!app || !rpc || !sock)
		return false;

	Botan::AutoSeeded_RNG rng;
	uint8_t clientNonce[kBackplaneAuthNonceSize];
	rng.randomize(clientNonce, sizeof(clientNonce));

	uint8_t clientMac[kBackplaneAuthMacSize];
	ComputeMac(g_clusterConfig.psk, "PKO-BP-1", clientNonce, sizeof(clientNonce), nullptr, 0, clientMac);

	WPacket wp = app->GetWPacket();
	wp.WriteCmd(CMD_OS_BACKPLANE_HELLO);
	wp.WriteLong(kBackplaneAuthMagic);
	wp.WriteSequence(reinterpret_cast<cChar*>(clientNonce), sizeof(clientNonce));
	wp.WriteSequence(reinterpret_cast<cChar*>(clientMac), sizeof(clientMac));

	RPacket ret = rpc->SyncCall(sock, wp, g_clusterConfig.handshakeTimeoutMs);
	if (!ret.HasData()) {
		LG("BackplaneAuth", "outbound handshake timeout peer=%s:%u\n", sock->GetPeerIP(), sock->GetPeerPort());
		return false;
	}

	if (ret.ReadCmd() != CMD_SO_BACKPLANE_HELLO) {
		LG("BackplaneAuth", "outbound unexpected reply cmd peer=%s:%u\n", sock->GetPeerIP(), sock->GetPeerPort());
		return false;
	}

	uShort err = ret.ReadShort();
	if (err != 0) {
		LG("BackplaneAuth", "outbound handshake rejected err=%u peer=%s:%u\n",
		   static_cast<unsigned>(err), sock->GetPeerIP(), sock->GetPeerPort());
		return false;
	}

	uShort serverNonceLen = 0;
	uShort serverMacLen = 0;
	cChar* serverNonce = ret.ReadSequence(serverNonceLen);
	cChar* serverMac = ret.ReadSequence(serverMacLen);
	if (!serverNonce || !serverMac || serverNonceLen != kBackplaneAuthNonceSize || serverMacLen != kBackplaneAuthMacSize) {
		LG("BackplaneAuth", "outbound malformed hello reply peer=%s:%u\n", sock->GetPeerIP(), sock->GetPeerPort());
		return false;
	}

	uint8_t expectedMac[kBackplaneAuthMacSize];
	ComputeMac(g_clusterConfig.psk, "PKO-BP-2", clientNonce, sizeof(clientNonce),
		reinterpret_cast<const uint8_t*>(serverNonce), kBackplaneAuthNonceSize, expectedMac);
	if (std::memcmp(expectedMac, serverMac, kBackplaneAuthMacSize) != 0) {
		LG("BackplaneAuth", "outbound server MAC verify failed peer=%s:%u\n", sock->GetPeerIP(), sock->GetPeerPort());
		return false;
	}

	MarkAuthenticated(sock);
	LG("BackplaneAuth", "outbound handshake OK peer=%s:%u\n", sock->GetPeerIP(), sock->GetPeerPort());
	return true;
}

WPacket BackplaneAuth::ServeHello(TcpCommApp* app, DataSocket* sock, RPacket& pk) {
	WPacket ret = app ? app->GetWPacket() : WPacket(0);
	ret.WriteCmd(CMD_SO_BACKPLANE_HELLO);

	if (!g_clusterConfig.IsEnabled()) {
		MarkLegacy(sock);
		ret.WriteShort(0);
		return ret;
	}

	if (pk.ReadLong() != kBackplaneAuthMagic) {
		LG("BackplaneAuth", "inbound bad magic peer=%s:%u\n", sock->GetPeerIP(), sock->GetPeerPort());
		ret.WriteShort(2);
		return ret;
	}

	uShort clientNonceLen = 0;
	uShort clientMacLen = 0;
	cChar* clientNonce = pk.ReadSequence(clientNonceLen);
	cChar* clientMac = pk.ReadSequence(clientMacLen);
	if (!clientNonce || !clientMac || clientNonceLen != kBackplaneAuthNonceSize || clientMacLen != kBackplaneAuthMacSize) {
		LG("BackplaneAuth", "inbound malformed hello peer=%s:%u\n", sock->GetPeerIP(), sock->GetPeerPort());
		ret.WriteShort(3);
		return ret;
	}

	uint8_t expectedMac[kBackplaneAuthMacSize];
	ComputeMac(g_clusterConfig.psk, "PKO-BP-1", reinterpret_cast<const uint8_t*>(clientNonce), kBackplaneAuthNonceSize,
		nullptr, 0, expectedMac);
	if (std::memcmp(expectedMac, clientMac, kBackplaneAuthMacSize) != 0) {
		LG("BackplaneAuth", "inbound client MAC verify failed peer=%s:%u\n", sock->GetPeerIP(), sock->GetPeerPort());
		ret.WriteShort(1);
		return ret;
	}

	Botan::AutoSeeded_RNG rng;
	uint8_t serverNonce[kBackplaneAuthNonceSize];
	rng.randomize(serverNonce, sizeof(serverNonce));

	uint8_t serverMac[kBackplaneAuthMacSize];
	ComputeMac(g_clusterConfig.psk, "PKO-BP-2", reinterpret_cast<const uint8_t*>(clientNonce), kBackplaneAuthNonceSize,
		serverNonce, sizeof(serverNonce), serverMac);

	MarkAuthenticated(sock);
	ret.WriteShort(0);
	ret.WriteSequence(reinterpret_cast<cChar*>(serverNonce), sizeof(serverNonce));
	ret.WriteSequence(reinterpret_cast<cChar*>(serverMac), sizeof(serverMac));
	LG("BackplaneAuth", "inbound handshake OK peer=%s:%u\n", sock->GetPeerIP(), sock->GetPeerPort());
	return ret;
}
