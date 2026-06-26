#pragma once

#include "serversdk/Packet.h"
#include "common/PacketReadUtils.h"

// M5-prep: typed read helpers over RPacket (I4 contract). Does not consume cmd.

namespace net {

class PacketReader {
public:
	explicit PacketReader(dbc::RPacket& pk)
		: pk_(pk) {}

	uLong Remain() const { return pk_.RemainData(); }

	bool Char(uChar& out) {
		if (pk_.RemainData() < sizeof(uChar)) {
			return false;
		}
		out = pk_.ReadChar();
		return true;
	}

	bool Short(uShort& out) {
		if (pk_.RemainData() < sizeof(uShort)) {
			return false;
		}
		out = pk_.ReadShort();
		return true;
	}

	bool Long(uLong& out) {
		if (pk_.RemainData() < sizeof(uLong)) {
			return false;
		}
		out = pk_.ReadLong();
		return true;
	}

	bool LongLong(unsigned long long& out) {
		if (pk_.RemainData() < sizeof(unsigned long long)) {
			return false;
		}
		out = pk_.ReadLongLong();
		return true;
	}

	bool Float(float& out) {
		if (pk_.RemainData() < sizeof(float)) {
			return false;
		}
		out = pk_.ReadFloat();
		return true;
	}

	bool String(cChar*& out) { return PacketRead::String(pk_, out); }

	bool String(cChar*& out, uShort& outLen) { return PacketRead::String(pk_, out, outLen); }

	dbc::RPacket& Raw() { return pk_; }
	const dbc::RPacket& Raw() const { return pk_; }

private:
	dbc::RPacket& pk_;
};

} // namespace net
