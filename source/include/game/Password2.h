#pragma once

#include "serversdk/Packet.h"
#include "serversdk/PacketEncryption.h"

// Unified client-side secondary password (MD5 + AES-CBC) helpers.

namespace Password2 {

// Hash plain numeric PIN to uppercase MD5 hex string (32 chars + NUL).
void HashPassword2(const char* plain, char md5Out[33]);

// Append AES-encrypted MD5 of plain password to an outbound packet.
void AppendEncryptedPassword2(dbc::WPacket& pk, const dbc::AES_KEY& aesKey, const char* plain);

// Append AES-encrypted precomputed MD5 (e.g. delete-character flow).
void AppendEncryptedPassword2Md5(dbc::WPacket& pk, const dbc::AES_KEY& aesKey, const char* md5);

} // namespace Password2
