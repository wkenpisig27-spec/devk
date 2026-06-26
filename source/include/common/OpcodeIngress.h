#pragma once

#include "common/OpcodeMeta.h"

namespace dbc {
class RPacket;
} // namespace dbc

// M5 Track E: OpcodeMeta-driven ingress validation (fail-closed at call sites).

bool ValidateKnownOpcode(uint16_t cmd);
bool ValidateOpcodeBand(uint16_t cmd, OpcodeBand expected);
bool ValidateMinPayload(dbc::RPacket& pk, uint16_t minBytes);

enum class GroupIngressPath : uint8_t {
	ServeCall,
	ProcessData,
};

bool ValidateClientToGateOpcode(uint16_t cmd, dbc::RPacket& pk, const char* peer);
bool ValidateGateSyncClientOpcode(uint16_t cmd, dbc::RPacket& pk, const char* peer);
bool ValidateGameCharacterOpcode(uint16_t cmd, dbc::RPacket& pk, const char* peer);
bool ValidateGameAppOpcode(uint16_t cmd, dbc::RPacket& pk, const char* peer);
bool ValidateGroupIngressOpcode(uint16_t cmd, dbc::RPacket& pk, const char* peer, GroupIngressPath path);
bool ValidateAccountIngressOpcode(uint16_t cmd, dbc::RPacket& pk, const char* peer);
