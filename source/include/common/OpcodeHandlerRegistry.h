#pragma once

#include <cstdint>
#include <cstddef>

namespace dbc {
class DataSocket;
class RPacket;
} // namespace dbc

// M2-prep: opcode → handler dispatch table (sorted, binary search).
// Handlers return true when the packet was consumed.
//
// Each process may host multiple domains (e.g. GameServer: GameCharacter + GameApp).
// Domains are isolated so GameApp dispatch cannot steal CM handlers from Character.

using OpcodeHandlerFn = bool (*)(void* ctx, dbc::DataSocket* sock, dbc::RPacket& recv);

enum class OpcodeDispatchDomain : uint8_t {
	Gate = 0,
	GameCharacter,
	GameApp,
	Count
};

struct OpcodeHandlerEntry {
	uint16_t opcode;
	OpcodeHandlerFn handler;
	const char* name;
	uint16_t minPayloadBytes = 0;
};

const OpcodeHandlerEntry* LookupOpcodeHandler(OpcodeDispatchDomain domain, uint16_t opcode);
bool RegisterOpcodeHandlers(OpcodeDispatchDomain domain, const OpcodeHandlerEntry* entries, std::size_t count);
bool DispatchOpcodeHandler(OpcodeDispatchDomain domain, uint16_t opcode, void* ctx, dbc::DataSocket* sock, dbc::RPacket& recv);
void ClearOpcodeHandlers(OpcodeDispatchDomain domain);
std::size_t OpcodeHandlerCount(OpcodeDispatchDomain domain);
