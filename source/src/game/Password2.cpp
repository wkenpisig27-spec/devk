#include "StdAfx.h"
#include "Password2.h"
#include "algo.h"

namespace Password2 {

void HashPassword2(const char* plain, char md5Out[33]) {
	md5string(plain, md5Out);
}

void AppendEncryptedPassword2(dbc::WPacket& pk, const dbc::AES_KEY& aesKey, const char* plain) {
	char md5[33] = {0};
	HashPassword2(plain, md5);
	AppendEncryptedPassword2Md5(pk, aesKey, md5);
}

void AppendEncryptedPassword2Md5(dbc::WPacket& pk, const dbc::AES_KEY& aesKey, const char* md5) {
	if (!md5) {
		return;
	}
	WritePacketSequenceEncrypted(pk, aesKey, reinterpret_cast<uint8_t*>(const_cast<char*>(md5)), strlen(md5) + 1);
}

} // namespace Password2
