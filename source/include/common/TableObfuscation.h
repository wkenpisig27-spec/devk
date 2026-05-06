#pragma once

/**
 * TableObfuscation.h
 * 
 * AES-256/GCM encryption for table .bin files to prevent players from
 * decrypting and modifying game data files.
 * 
 * Usage:
 *   - When writing .bin files: Call TableObfuscation::EncryptToFile()
 *   - When reading .bin files: Call TableObfuscation::DecryptFromFile()
 * 
 * The encryption uses AES-256/GCM with 96-bit random nonce and 128-bit authentication tag.
 * 
 * Note: Implementation is in TableObfuscation.cpp to avoid include conflicts with Botan.
 * Link against Common.lib.
 */

#include <cstdint>
#include <cstdio>

namespace TableObfuscation {

/**
 * Encrypt data using AES-256/GCM
 * 
 * @param plainData     Input data to encrypt
 * @param plainSize     Size of input data
 * @param encryptedData Output buffer (caller must free with delete[])
 * @param encryptedSize Output size of encrypted data
 * @return true on success, false on failure
 */
bool Encrypt(const uint8_t* plainData, size_t plainSize,
             uint8_t** encryptedData, size_t* encryptedSize);

/**
 * Decrypt data using AES-256/GCM
 * 
 * @param encryptedData Input encrypted data
 * @param encryptedSize Size of encrypted data
 * @param plainData     Output buffer (caller must free with delete[])
 * @param plainSize     Output size of decrypted data
 * @return true on success, false on failure (including auth failure)
 */
bool Decrypt(const uint8_t* encryptedData, size_t encryptedSize,
             uint8_t** plainData, size_t* plainSize);

/**
 * Convenience wrapper for encrypting and writing to file
 */
bool EncryptToFile(const char* filename, const uint8_t* data, size_t dataSize);

/**
 * Convenience wrapper for reading and decrypting from file
 * Handles both encrypted and unencrypted files (backward compatibility)
 */
bool DecryptFromFile(const char* filename, uint8_t** data, size_t* dataSize);

} // namespace TableObfuscation
