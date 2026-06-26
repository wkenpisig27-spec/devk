#include "OpcodeHandlerRegistry.h"

#include <algorithm>
#include <vector>

namespace {

std::vector<OpcodeHandlerEntry> g_handlers;

const OpcodeHandlerEntry* findHandler(uint16_t opcode) {
	auto it = std::lower_bound(
		g_handlers.begin(),
		g_handlers.end(),
		opcode,
		[](const OpcodeHandlerEntry& entry, uint16_t value) { return entry.opcode < value; });
	if (it != g_handlers.end() && it->opcode == opcode) {
		return &(*it);
	}
	return nullptr;
}

} // namespace

bool RegisterOpcodeHandlers(const OpcodeHandlerEntry* entries, std::size_t count) {
	if (!entries || count == 0) {
		return false;
	}

	std::vector<OpcodeHandlerEntry> merged = g_handlers;
	merged.insert(merged.end(), entries, entries + count);
	std::sort(merged.begin(), merged.end(), [](const OpcodeHandlerEntry& a, const OpcodeHandlerEntry& b) {
		return a.opcode < b.opcode;
	});

	for (std::size_t i = 1; i < merged.size(); ++i) {
		if (merged[i].opcode == merged[i - 1].opcode) {
			return false;
		}
	}

	g_handlers.swap(merged);
	return true;
}

bool DispatchOpcodeHandler(uint16_t opcode, void* ctx, dbc::DataSocket* sock, dbc::RPacket& recv) {
	const OpcodeHandlerEntry* entry = findHandler(opcode);
	if (!entry || !entry->handler) {
		return false;
	}
	return entry->handler(ctx, sock, recv);
}

void ClearOpcodeHandlers() {
	g_handlers.clear();
}

std::size_t OpcodeHandlerCount() {
	return g_handlers.size();
}
