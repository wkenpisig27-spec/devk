#pragma once
#include <botan/auto_rng.h>
#include <botan/pkcs8.h>
#include <botan/x509_key.h>
#include <botan/rsa.h>
#include <botan/pubkey.h>
#include <botan/botan.h>
#include <botan/aes.h>
#include <botan/cipher_mode.h>
#include <botan/aead.h>
#include <botan/data_src.h>

namespace dbc {
class WPacket;
class RPacket;

constexpr auto AES_KEY_LENGTH{128 / 8};
constexpr auto AES_IV_LENGTH{128 / 8};
using AES_KEY = uint8_t[AES_KEY_LENGTH];
using AES_IV = uint8_t[AES_IV_LENGTH];

constexpr uint8_t WIRE_CRYPTO_OFF = 0;
constexpr uint8_t WIRE_CRYPTO_CTR = 1;
constexpr uint8_t WIRE_CRYPTO_GCM = 2;
constexpr size_t WIRE_GCM_TAG_SIZE = 16;
constexpr size_t WIRE_GCM_NONCE_SIZE = 12;

void WritePacketSequenceEncrypted(WPacket& wpk, const AES_KEY& aes_key, uint8_t seq[], size_t seq_len);

Botan::secure_vector<uint8_t> ReadPacketSequenceEncrypted(RPacket& rpk, const AES_KEY& aes_key);

void WireBuildGcmNonce(uint8_t nonce[WIRE_GCM_NONCE_SIZE], const AES_IV& ivBase, uint64_t seq);
void WireGcmEncryptInPlace(const AES_KEY& key, const AES_IV& ivBase, uint64_t seq, uint8_t* data, uLong& len, uLong capacity);
void WireGcmDecryptInPlace(const AES_KEY& key, const AES_IV& ivBase, uint64_t seq, uint8_t* data, uLong& len);

} // namespace dbc