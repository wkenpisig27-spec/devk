#pragma once

// Wire and receive-buffer limits (I2). recvbuf_cap should match pkt_maxlen per link.
namespace NetLimits {
constexpr unsigned kClientMaxPacket = 32768;       // Gate <-> game client (login rsp with chars can exceed 16KB)
constexpr unsigned kClientGameMaxPacket = 32768;     // Game client <-> gate
constexpr unsigned kInterServerMaxPacket = 32768;    // Gate <-> Game / Group
constexpr unsigned kGameServerMaxPacket = 32768;     // Game <-> Gate
constexpr unsigned kGroupServerMaxPacket = 16384;    // Group server links
constexpr unsigned kAccountServerMaxPacket = 4096;   // Account server links

// Gate ToClient rate limits (I1) — override via GateServer.cfg [AntiDDoS]
constexpr unsigned kGateConnMinIntervalMs = 100;
constexpr unsigned kGateMaxConnPerSecondPerIp = 5;
constexpr unsigned kGateMaxRecvBytesPerSec = 12 * 1024;
constexpr unsigned kGateMaxRecvPktsPerSec = 500;
constexpr unsigned kGateConnBlockMinutes = 1;
constexpr size_t kGateMaxTrackedConnIps = 50000;

// Gate→Game/Group session trailer: WriteLong(slot) + WriteLong(gen) + WriteLongLong(addr)
constexpr unsigned kGateSessionTrailerBytes = 16;
} // namespace NetLimits
