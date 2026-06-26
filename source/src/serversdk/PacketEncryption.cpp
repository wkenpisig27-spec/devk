#include "PacketEncryption.h"
#include "Packet.h"

namespace dbc {

void WritePacketSequenceEncrypted(WPacket& wpk, const AES_KEY& aes_key, uint8_t seq[], size_t seq_len) {
	std::unique_ptr<Botan::Cipher_Mode> aes_encryptor = Botan::Cipher_Mode::create(
		"AES-128/CBC/PKCS7", Botan::ENCRYPTION);
	Botan::AutoSeeded_RNG rng;
	Botan::InitializationVector iv(rng, AES_IV_LENGTH);

	aes_encryptor->set_key(aes_key, AES_KEY_LENGTH);
	aes_encryptor->start(iv.bits_of());
	Botan::secure_vector<uint8_t> cipher(seq, seq + seq_len);
	aes_encryptor->finish(cipher);

	// Send AES-encrypted password.
	wpk.WriteSequence((cChar*)cipher.data(), cipher.size());
	// Send IV.
	wpk.WriteSequence((cChar*)iv.bits_of().data(), iv.bits_of().size());
}

Botan::secure_vector<uint8_t> ReadPacketSequenceEncrypted(RPacket& rpk, const AES_KEY& aes_key) {
	std::unique_ptr<Botan::Cipher_Mode> aes_decryptor = Botan::Cipher_Mode::create(
		"AES-128/CBC/PKCS7", Botan::DECRYPTION);
	Botan::AutoSeeded_RNG rng;

	AES_IV iv;
	uShort ivLen;
	uShort cipherLen;
	cChar* cipherptr = rpk.ReadSequence(cipherLen);
	Botan::secure_vector<uint8_t> plaintext(cipherptr, cipherptr + cipherLen);
	memcpy(iv, rpk.ReadSequence(ivLen), AES_IV_LENGTH);

	aes_decryptor->set_key(aes_key, AES_KEY_LENGTH);
	aes_decryptor->start(iv, AES_IV_LENGTH);
	aes_decryptor->finish(plaintext);
	return plaintext;
}

void WireBuildGcmNonce(uint8_t nonce[WIRE_GCM_NONCE_SIZE], const AES_IV& ivBase, uint64_t seq) {
	memcpy(nonce, ivBase, WIRE_GCM_NONCE_SIZE);
	for (int i = 0; i < 8; ++i) {
		nonce[WIRE_GCM_NONCE_SIZE - 1 - i] ^= static_cast<uint8_t>(seq >> (8 * i));
	}
}

void WireGcmEncryptInPlace(const AES_KEY& key, const AES_IV& ivBase, uint64_t seq, uint8_t* data, uint32_t& len, uint32_t capacity) {
	if (len + WIRE_GCM_TAG_SIZE > capacity) {
		throw Botan::Invalid_Argument("wire GCM encrypt buffer too small");
	}
	std::unique_ptr<Botan::AEAD_Mode> aead = Botan::AEAD_Mode::create("AES-128/GCM", Botan::ENCRYPTION);
	aead->set_key(key, AES_KEY_LENGTH);
	uint8_t nonce[WIRE_GCM_NONCE_SIZE];
	WireBuildGcmNonce(nonce, ivBase, seq);
	aead->start(nonce, sizeof(nonce));
	Botan::secure_vector<uint8_t> buf(data, data + len);
	aead->finish(buf);
	memcpy(data, buf.data(), buf.size());
	len = static_cast<uint32_t>(buf.size());
}

void WireGcmDecryptInPlace(const AES_KEY& key, const AES_IV& ivBase, uint64_t seq, uint8_t* data, uint32_t& len) {
	if (len < WIRE_GCM_TAG_SIZE) {
		throw Botan::Invalid_Authentication_Tag("wire GCM packet truncated");
	}
	std::unique_ptr<Botan::AEAD_Mode> aead = Botan::AEAD_Mode::create("AES-128/GCM", Botan::DECRYPTION);
	aead->set_key(key, AES_KEY_LENGTH);
	uint8_t nonce[WIRE_GCM_NONCE_SIZE];
	WireBuildGcmNonce(nonce, ivBase, seq);
	aead->start(nonce, sizeof(nonce));
	Botan::secure_vector<uint8_t> buf(data, data + len);
	aead->finish(buf);
	memcpy(data, buf.data(), buf.size());
	len = static_cast<uint32_t>(buf.size());
}

} // namespace dbc