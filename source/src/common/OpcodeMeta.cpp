#include "OpcodeMeta.h"

#include <algorithm>

namespace {

const OpcodeMeta kOpcodeMetaTable[] = {
#include "OpcodeMetaTable.inc"
};

constexpr std::size_t kOpcodeMetaTableSize = sizeof(kOpcodeMetaTable) / sizeof(kOpcodeMetaTable[0]);

const OpcodeMeta* findByValue(uint16_t cmd) {
	const OpcodeMeta* begin = kOpcodeMetaTable;
	const OpcodeMeta* end = begin + kOpcodeMetaTableSize;
	const OpcodeMeta* it = std::lower_bound(
		begin,
		end,
		cmd,
		[](const OpcodeMeta& meta, uint16_t value) { return meta.value < value; });
	if (it != end && it->value == cmd) {
		return it;
	}
	return nullptr;
}

} // namespace

const OpcodeMeta* LookupOpcodeMeta(uint16_t cmd) {
	return findByValue(cmd);
}

const char* OpcodeName(uint16_t cmd) {
	const OpcodeMeta* meta = findByValue(cmd);
	return meta ? meta->name : "CMD_UNKNOWN";
}

OpcodeBand OpcodeBandFor(uint16_t cmd) {
	const OpcodeMeta* meta = findByValue(cmd);
	return meta ? meta->band : OpcodeBand::Unknown;
}

bool IsKnownOpcode(uint16_t cmd) {
	return findByValue(cmd) != nullptr;
}

const OpcodeMeta* OpcodeMetaTable() {
	return kOpcodeMetaTable;
}

std::size_t OpcodeMetaCount() {
	return kOpcodeMetaTableSize;
}

const char* OpcodeBandLabel(OpcodeBand band) {
	switch (band) {
	case OpcodeBand::CM:
		return "CM";
	case OpcodeBand::MC:
		return "MC";
	case OpcodeBand::TM:
		return "TM";
	case OpcodeBand::MT:
		return "MT";
	case OpcodeBand::TP:
		return "TP";
	case OpcodeBand::PT:
		return "PT";
	case OpcodeBand::PA:
		return "PA";
	case OpcodeBand::AP:
		return "AP";
	case OpcodeBand::MM:
		return "MM";
	case OpcodeBand::PM:
		return "PM";
	case OpcodeBand::PC:
		return "PC";
	case OpcodeBand::MP:
		return "MP";
	case OpcodeBand::CP:
		return "CP";
	case OpcodeBand::OS:
		return "OS";
	case OpcodeBand::SO:
		return "SO";
	case OpcodeBand::TC:
		return "TC";
	default:
		return "?";
	}
}
