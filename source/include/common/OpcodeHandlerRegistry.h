#pragma once

#include <cstdint>
#include <cstddef>

namespace dbc {
class DataSocket;
class RPacket;
} // namespace dbc

// M2-prep: opcode → handler dispatch table (sorted, binary search).
// Handlers return true when the packet was consumed.

using OpcodeHandlerFn = bool (*)(void* ctx, dbc::DataSocket* sock, dbc::RPacket& recv);

struct OpcodeHandlerEntry {
	uint16_t opcode;
	OpcodeHandlerFn handler;
	const char* name;
};

bool RegisterOpcodeHandlers(const OpcodeHandlerEntry* entries, std::size_t count);
bool DispatchOpcodeHandler(uint16_t opcode, void* ctx, dbc::DataSocket* sock, dbc::RPacket& recv);
void ClearOpcodeHandlers();
std::size_t OpcodeHandlerCount();
