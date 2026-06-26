#pragma once

#include <cstdint>
#include <cstddef>

// Opcode metadata (M1) — table generated from NetCommand.h via
// helper/network-tests/scripts/generate_opcode_table.py

enum class OpcodeBand : uint8_t {
	CM = 0,
	MC,
	TM,
	MT,
	TP,
	PT,
	PA,
	AP,
	MM,
	PM,
	PC,
	MP,
	CP,
	OS,
	SO,
	TC,
	Unknown,
};

struct OpcodeMeta {
	uint16_t value;
	OpcodeBand band;
	bool isBase;
	const char* name;
};

const OpcodeMeta* LookupOpcodeMeta(uint16_t cmd);
const char* OpcodeName(uint16_t cmd);
OpcodeBand OpcodeBandFor(uint16_t cmd);
bool IsKnownOpcode(uint16_t cmd);
const OpcodeMeta* OpcodeMetaTable();
std::size_t OpcodeMetaCount();

const char* OpcodeBandLabel(OpcodeBand band);
