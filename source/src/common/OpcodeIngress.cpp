#include "common/OpcodeIngress.h"

#include "common/NetCommand.h"
#include "common/OpcodeHandlerRegistry.h"
#include "serversdk/Packet.h"
#include "util/log.h"

#include <atomic>
#include <chrono>

namespace {

std::atomic<uint32_t> g_rejectCount{0};
std::chrono::steady_clock::time_point g_lastRejectSummary = std::chrono::steady_clock::now();

void MaybeLogRejectSummary() {
	const auto now = std::chrono::steady_clock::now();
	if (g_rejectCount.load() == 0) {
		return;
	}
	if (now - g_lastRejectSummary < std::chrono::seconds(60)) {
		return;
	}
	LG("OpcodeIngress", "[summary] rejects=%u in last 60s\n", g_rejectCount.exchange(0));
	g_lastRejectSummary = now;
}

void LogReject(uint16_t cmd, const char* reason, const char* peer) {
	++g_rejectCount;
	LG("OpcodeIngress", "[reject] cmd=%u name=%s reason=%s peer=%s\n",
		static_cast<unsigned>(cmd),
		OpcodeName(cmd),
		reason ? reason : "?",
		peer ? peer : "?");
	MaybeLogRejectSummary();
}

bool IsClientToGateBand(OpcodeBand band) {
	return band == OpcodeBand::CM || band == OpcodeBand::CP;
}

bool IsGameAppBand(OpcodeBand band) {
	return band == OpcodeBand::TM || band == OpcodeBand::PM || band == OpcodeBand::MM;
}

bool IsGroupServeCallBand(OpcodeBand band) {
	return band == OpcodeBand::TP || band == OpcodeBand::PA || band == OpcodeBand::OS;
}

bool IsGroupProcessDataBand(OpcodeBand band) {
	return band == OpcodeBand::CP || band == OpcodeBand::MP;
}

bool IsGateSyncClientOpcode(uint16_t cmd) {
	switch (cmd) {
	case CMD_CM_LOGIN:
	case CMD_CM_RSA_HANDSHAKE_1:
	case CMD_CM_LOGOUT:
	case CMD_CM_BGNPLAY:
	case CMD_CM_ENDPLAY:
	case CMD_CM_NEWCHA:
	case CMD_CM_DELCHA:
	case CMD_CM_CREATE_PASSWORD2:
	case CMD_CM_UPDATE_PASSWORD2:
	case CMD_CM_REGISTER:
	case CMD_CP_CHANGEPASS:
		return true;
	default:
		return false;
	}
}

uint16_t LookupGroupProcessDataMinPayload(uint16_t cmd) {
	switch (cmd) {
	case CMD_CP_PING:
	case CMD_CP_TEAM_ACCEPT:
	case CMD_CP_TEAM_REFUSE:
	case CMD_CP_FRND_ACCEPT:
	case CMD_CP_SESS_SAY:
	case CMD_CP_SESS_ADD:
	case CMD_CP_SESS_LEAVE:
		return 4;
	case CMD_CP_SESS_CREATE:
		return 1;
	default:
		return 0;
	}
}

uint16_t LookupGateSyncMinPayload(uint16_t cmd) {
	switch (cmd) {
	case CMD_CM_BGNPLAY:
	case CMD_CM_NEWCHA:
	case CMD_CM_DELCHA:
		return 1;
	default:
		return 0;
	}
}

} // namespace

bool ValidateKnownOpcode(uint16_t cmd) {
	const OpcodeMeta* meta = LookupOpcodeMeta(cmd);
	return meta && !meta->isBase;
}

bool ValidateOpcodeBand(uint16_t cmd, OpcodeBand expected) {
	return OpcodeBandFor(cmd) == expected;
}

bool ValidateMinPayload(dbc::RPacket& pk, uint16_t minBytes) {
	if (minBytes == 0) {
		return true;
	}
	return pk.RemainData() >= minBytes;
}

bool ValidateClientToGateOpcode(uint16_t cmd, dbc::RPacket& pk, const char* peer) {
	const OpcodeBand band = OpcodeBandFor(cmd);
	if (!IsClientToGateBand(band)) {
		LogReject(cmd, "band not CM/CP", peer);
		return false;
	}

	const OpcodeMeta* meta = LookupOpcodeMeta(cmd);
	if (meta && meta->isBase) {
		LogReject(cmd, "base opcode", peer);
		return false;
	}

	const OpcodeHandlerEntry* entry = LookupOpcodeHandler(OpcodeDispatchDomain::Gate, cmd);
	if (!ValidateKnownOpcode(cmd) && !entry) {
		LogReject(cmd, "unknown opcode", peer);
		return false;
	}

	if (entry && entry->minPayloadBytes > 0 && !ValidateMinPayload(pk, entry->minPayloadBytes)) {
		LogReject(cmd, "payload too short", peer);
		return false;
	}

	return true;
}

bool ValidateGateSyncClientOpcode(uint16_t cmd, dbc::RPacket& pk, const char* peer) {
	if (!IsGateSyncClientOpcode(cmd)) {
		return true;
	}

	const OpcodeBand band = OpcodeBandFor(cmd);
	if (!IsClientToGateBand(band)) {
		LogReject(cmd, "band not CM/CP", peer);
		return false;
	}

	if (!ValidateKnownOpcode(cmd)) {
		LogReject(cmd, "unknown opcode", peer);
		return false;
	}

	const uint16_t minBytes = LookupGateSyncMinPayload(cmd);
	if (minBytes > 0 && !ValidateMinPayload(pk, minBytes)) {
		LogReject(cmd, "payload too short", peer);
		return false;
	}

	return true;
}

bool ValidateGameCharacterOpcode(uint16_t cmd, dbc::RPacket& pk, const char* peer) {
	if (!ValidateKnownOpcode(cmd)) {
		LogReject(cmd, "unknown opcode", peer);
		return false;
	}
	if (!ValidateOpcodeBand(cmd, OpcodeBand::CM)) {
		LogReject(cmd, "band not CM", peer);
		return false;
	}

	const OpcodeHandlerEntry* entry = LookupOpcodeHandler(OpcodeDispatchDomain::GameCharacter, cmd);
	if (entry && entry->minPayloadBytes > 0 && !ValidateMinPayload(pk, entry->minPayloadBytes)) {
		LogReject(cmd, "payload too short", peer);
		return false;
	}

	return true;
}

bool ValidateGameAppOpcode(uint16_t cmd, dbc::RPacket& pk, const char* peer) {
	const OpcodeHandlerEntry* entry = LookupOpcodeHandler(OpcodeDispatchDomain::GameApp, cmd);
	if (!entry) {
		return true;
	}

	if (!ValidateKnownOpcode(cmd)) {
		LogReject(cmd, "unknown opcode", peer);
		return false;
	}

	const OpcodeBand band = OpcodeBandFor(cmd);
	if (!IsGameAppBand(band)) {
		LogReject(cmd, "band not TM/PM/MM", peer);
		return false;
	}

	if (entry->minPayloadBytes > 0 && !ValidateMinPayload(pk, entry->minPayloadBytes)) {
		LogReject(cmd, "payload too short", peer);
		return false;
	}

	return true;
}

bool ValidateGroupIngressOpcode(uint16_t cmd, dbc::RPacket& pk, const char* peer, GroupIngressPath path) {
	const OpcodeBand band = OpcodeBandFor(cmd);

	if (path == GroupIngressPath::ServeCall) {
		if (!IsGroupServeCallBand(band)) {
			LogReject(cmd, "band not TP/PA/OS", peer);
			return false;
		}
	} else if (!IsGroupProcessDataBand(band)) {
		return true;
	}

	if (!ValidateKnownOpcode(cmd)) {
		LogReject(cmd, "unknown opcode", peer);
		return false;
	}

	if (path == GroupIngressPath::ProcessData) {
		const uint16_t minBytes = LookupGroupProcessDataMinPayload(cmd);
		if (minBytes > 0 && !ValidateMinPayload(pk, minBytes)) {
			LogReject(cmd, "payload too short", peer);
			return false;
		}
	}

	return true;
}

bool ValidateAccountIngressOpcode(uint16_t cmd, dbc::RPacket& pk, const char* peer) {
	if (cmd == CMD_OS_BACKPLANE_HELLO) {
		return true;
	}

	if (!ValidateOpcodeBand(cmd, OpcodeBand::PA)) {
		LogReject(cmd, "band not PA", peer);
		return false;
	}

	if (!ValidateKnownOpcode(cmd)) {
		LogReject(cmd, "unknown opcode", peer);
		return false;
	}

	(void)pk;
	return true;
}
