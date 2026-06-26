#pragma once

#include "serversdk/Packet.h"

// M5-prep: thin write facade over WPacket for handler code.

namespace net {

class PacketWriter {
public:
	explicit PacketWriter(dbc::WPacket& pk)
		: pk_(pk) {}

	bool Cmd(uShort cmd) { return pk_.WriteCmd(cmd); }
	bool Char(uChar ch) { return pk_.WriteChar(ch); }
	bool Short(uShort sh) { return pk_.WriteShort(sh); }
	bool Long(uLong lo) { return pk_.WriteLong(lo); }
	bool LongLong(unsigned long long ll) { return pk_.WriteLongLong(ll); }
	bool Float(float fVal) { return pk_.WriteFloat(fVal); }
	bool Sequence(cChar* seq, uShort len) { return pk_.WriteSequence(seq, len); }
	bool String(cChar* str) { return pk_.WriteString(str); }

	dbc::WPacket& Raw() { return pk_; }
	const dbc::WPacket& Raw() const { return pk_; }

private:
	dbc::WPacket& pk_;
};

} // namespace net
