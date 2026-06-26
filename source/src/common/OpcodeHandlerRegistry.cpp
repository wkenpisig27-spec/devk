#include "OpcodeHandlerRegistry.h"

#include "serversdk/Packet.h"

#include <algorithm>
#include <array>
#include <vector>

namespace {

using HandlerTable = std::vector<OpcodeHandlerEntry>;

std::array<HandlerTable, static_cast<std::size_t>(OpcodeDispatchDomain::Count)> g_handlers;

HandlerTable& table(OpcodeDispatchDomain domain) {
	return g_handlers[static_cast<std::size_t>(domain)];
}

const OpcodeHandlerEntry* findHandler(const HandlerTable& handlers, uint16_t opcode) {
	auto it = std::lower_bound(
		handlers.begin(),
		handlers.end(),
		opcode,
		[](const OpcodeHandlerEntry& entry, uint16_t value) { return entry.opcode < value; });
	if (it != handlers.end() && it->opcode == opcode) {
		return &(*it);
	}
	return nullptr;
}

bool mergeHandlers(HandlerTable& handlers, const OpcodeHandlerEntry* entries, std::size_t count) {
	if (!entries || count == 0) {
		return false;
	}

	HandlerTable merged = handlers;
	merged.insert(merged.end(), entries, entries + count);
	std::sort(merged.begin(), merged.end(), [](const OpcodeHandlerEntry& a, const OpcodeHandlerEntry& b) {
		return a.opcode < b.opcode;
	});

	for (std::size_t i = 1; i < merged.size(); ++i) {
		if (merged[i].opcode == merged[i - 1].opcode) {
			return false;
		}
	}

	handlers.swap(merged);
	return true;
}

} // namespace

const OpcodeHandlerEntry* LookupOpcodeHandler(OpcodeDispatchDomain domain, uint16_t opcode) {
	if (domain >= OpcodeDispatchDomain::Count) {
		return nullptr;
	}
	return findHandler(table(domain), opcode);
}

bool RegisterOpcodeHandlers(OpcodeDispatchDomain domain, const OpcodeHandlerEntry* entries, std::size_t count) {
	if (domain >= OpcodeDispatchDomain::Count) {
		return false;
	}
	return mergeHandlers(table(domain), entries, count);
}

bool DispatchOpcodeHandler(OpcodeDispatchDomain domain, uint16_t opcode, void* ctx, dbc::DataSocket* sock, dbc::RPacket& recv) {
	if (domain >= OpcodeDispatchDomain::Count) {
		return false;
	}
	const OpcodeHandlerEntry* entry = findHandler(table(domain), opcode);
	if (!entry || !entry->handler) {
		return false;
	}
	if (entry->minPayloadBytes > 0 && recv.RemainData() < entry->minPayloadBytes) {
		return false;
	}
	return entry->handler(ctx, sock, recv);
}

void ClearOpcodeHandlers(OpcodeDispatchDomain domain) {
	if (domain >= OpcodeDispatchDomain::Count) {
		return;
	}
	table(domain).clear();
}

std::size_t OpcodeHandlerCount(OpcodeDispatchDomain domain) {
	if (domain >= OpcodeDispatchDomain::Count) {
		return 0;
	}
	return table(domain).size();
}
