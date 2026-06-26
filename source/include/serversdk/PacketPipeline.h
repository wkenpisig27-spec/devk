#pragma once

#include "Comm.h"
#include "DataSocket.h"
#include "Packet.h"
#include "util/log.h"

_DBC_BEGIN

// Disconnect reasons (see TcpCommApp::OnDisconnect / GetDisconnectErrText):
//  DS_DECRYPT_FAIL     (-11) payload decrypt threw
//  DS_HANDLER_EXCP     (-12) OnProcessData / handler threw
//  DS_PACKET_PIPELINE  (-13) receive/dispatch pipeline threw

inline void PacketPipelineFailDisconnect(TcpCommApp* tca, DataSocket* datasock, RPacket& rpk, int reason, cChar* stage) {
	if (!tca || !datasock || datasock->IsDisconnectPending()) {
		return;
	}
	uShort cmd = 0;
	uLong dataLen = 0;
	if (rpk) {
		cmd = rpk.ReadCmd();
		dataLen = rpk.GetDataLen();
	}
	LG("Security",
	   "[PacketPipeline] %s peer=%s:%u cmd=%u data_len=%u reason=%d\n",
	   stage ? stage : "unknown",
	   datasock->GetPeerIP(),
	   datasock->GetPeerPort(),
	   static_cast<unsigned int>(cmd),
	   dataLen,
	   reason);
	tca->Disconnect(datasock, 0, reason);
}

_DBC_END
