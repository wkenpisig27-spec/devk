#pragma once

#include "serversdk/Packet.h"
#include <string>

// Helpers for I4 read contract: RPacket::ReadString() returns nullptr on underflow.

namespace PacketRead {

inline bool String(dbc::RPacket& pk, cChar*& out) {
	out = pk.ReadString();
	return out != nullptr;
}

inline bool String(dbc::RPacket& pk, cChar*& out, uShort& outLen) {
	out = pk.ReadString(&outLen);
	return out != nullptr;
}

inline std::string StringCopy(dbc::RPacket& pk) {
	cChar* s = pk.ReadString();
	return s ? std::string(s) : std::string();
}

} // namespace PacketRead
