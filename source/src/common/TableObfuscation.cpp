#include "TableObfuscation.h"
#include <cstdio>
#include <cstring>
#include <string>

// Define NOMINMAX to prevent conflicts with Botan
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Save and undefine Windows min/max macros to prevent conflicts with Botan
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

// Botan includes
#include <botan/cipher_mode.h>
#include <botan/auto_rng.h>
#include <botan/aead.h>
#include <botan/hex.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h> // For OutputDebugStringA
#else
#include "serversdk/platform_compat.h"
#endif

namespace TableObfuscation {

// Magic header to identify encrypted files
constexpr uint32_t MAGIC_HEADER = 0x504B4F42; // "PKOB" in little endian
constexpr uint8_t  FILE_VERSION = 1;

// AES-256/GCM parameters
constexpr size_t AES_KEY_SIZE = 32;   // 256 bits
constexpr size_t NONCE_SIZE   = 12;   // 96 bits (recommended for GCM)
constexpr size_t TAG_SIZE     = 16;   // 128 bits

// Header structure for encrypted files
#pragma pack(push, 1)
struct EncryptedFileHeader {
    uint32_t magic;           // 4 bytes: MAGIC_HEADER
    uint8_t  version;         // 1 byte: FILE_VERSION
    uint8_t  nonce[NONCE_SIZE];  // 12 bytes: random nonce
    uint8_t  tag[TAG_SIZE];      // 16 bytes: GCM auth tag
    uint32_t originalSize;    // 4 bytes: size of original data
};
#pragma pack(pop)

static_assert(sizeof(EncryptedFileHeader) == 37, "Header size mismatch");

/**
 * Get the encryption key.
 * This key is intentionally obfuscated to make it harder to extract.
 * In production, consider additional obfuscation techniques.
 */
static void GetEncryptionKey(uint8_t key[AES_KEY_SIZE]) {
    // Key is split and XOR'd to make static analysis harder
    // Base key parts (will be XOR'd together)
    static const uint8_t keyPart1[AES_KEY_SIZE] = {
        0x53, 0x6C, 0x69, 0x6D, 0x65, 0x50, 0x69, 0x72,  // "SlimePir"
        0x61, 0x74, 0x65, 0x73, 0x4F, 0x6E, 0x6C, 0x69,  // "atesOnli"
        0x6E, 0x65, 0x32, 0x30, 0x32, 0x36, 0x4B, 0x65,  // "ne2026Ke"
        0x79, 0x53, 0x65, 0x63, 0x72, 0x65, 0x74, 0x21   // "ySecret!"
    };
    
    // XOR mask to obfuscate the actual key
    static const uint8_t keyMask[AES_KEY_SIZE] = {
        0xA7, 0x3B, 0xF2, 0x8C, 0x15, 0xD4, 0x69, 0xE1,
        0x2F, 0x93, 0x7A, 0xBC, 0x48, 0x5D, 0xC6, 0x31,
        0x82, 0xEE, 0x17, 0xAD, 0x64, 0x9F, 0x3C, 0x75,
        0xB8, 0x21, 0xD9, 0x4E, 0xA3, 0x6F, 0x88, 0xC2
    };
    
    // Derive the actual key
    for (size_t i = 0; i < AES_KEY_SIZE; ++i) {
        key[i] = keyPart1[i] ^ keyMask[i];
    }
}

/**
 * Check if a buffer contains an encrypted file (by checking magic header)
 */
static bool IsEncrypted(const uint8_t* data, size_t dataSize) {
    if (dataSize < sizeof(EncryptedFileHeader)) {
        return false;
    }
    const auto* header = reinterpret_cast<const EncryptedFileHeader*>(data);
    return header->magic == MAGIC_HEADER && header->version == FILE_VERSION;
}

bool Encrypt(const uint8_t* plainData, size_t plainSize,
                    uint8_t** encryptedData, size_t* encryptedSize) {
    if (!plainData || plainSize == 0 || !encryptedData || !encryptedSize) {
        return false;
    }

    try {
        // Get encryption key
        uint8_t key[AES_KEY_SIZE];
        GetEncryptionKey(key);

        // Create AEAD cipher (AES-256/GCM)
        auto aead = Botan::AEAD_Mode::create("AES-256/GCM", Botan::ENCRYPTION);
        if (!aead) {
            return false;
        }

        // Set the key
        aead->set_key(key, AES_KEY_SIZE);

        // Generate random nonce
        Botan::AutoSeeded_RNG rng;
        uint8_t nonce[NONCE_SIZE];
        rng.randomize(nonce, NONCE_SIZE);

        // Start encryption with nonce
        aead->start(nonce, NONCE_SIZE);

        // Prepare buffer for encryption (GCM adds tag at the end)
        Botan::secure_vector<uint8_t> buffer(plainData, plainData + plainSize);
        
        // Finish encryption (this adds the authentication tag)
        aead->finish(buffer);

        // Calculate output size: header + encrypted data (which includes tag)
        *encryptedSize = sizeof(EncryptedFileHeader) + buffer.size() - TAG_SIZE;
        *encryptedData = new uint8_t[*encryptedSize + TAG_SIZE];

        // Build the header
        auto* header = reinterpret_cast<EncryptedFileHeader*>(*encryptedData);
        header->magic = MAGIC_HEADER;
        header->version = FILE_VERSION;
        memcpy(header->nonce, nonce, NONCE_SIZE);
        
        // The GCM tag is at the end of the buffer
        memcpy(header->tag, buffer.data() + plainSize, TAG_SIZE);
        header->originalSize = static_cast<uint32_t>(plainSize);

        // Copy encrypted data (without tag, which is in header)
        memcpy(*encryptedData + sizeof(EncryptedFileHeader), buffer.data(), plainSize);
        *encryptedSize = sizeof(EncryptedFileHeader) + plainSize;

        // Clear sensitive data
        memset(key, 0, AES_KEY_SIZE);

        return true;
    }
    catch (const Botan::Exception& e) {
        // Log error in debug builds
        #ifdef _DEBUG
        OutputDebugStringA("TableObfuscation::Encrypt error: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
        #endif
        return false;
    }
    catch (...) {
        return false;
    }
}

bool Decrypt(const uint8_t* encryptedData, size_t encryptedSize,
                    uint8_t** plainData, size_t* plainSize) {
    if (!encryptedData || !plainData || !plainSize) {
        return false;
    }

    // Check minimum size
    if (encryptedSize < sizeof(EncryptedFileHeader)) {
        return false;
    }

    // Parse header
    const auto* header = reinterpret_cast<const EncryptedFileHeader*>(encryptedData);

    // Verify magic and version
    if (header->magic != MAGIC_HEADER || header->version != FILE_VERSION) {
        return false;
    }

    // Calculate encrypted data size
    size_t cipherSize = encryptedSize - sizeof(EncryptedFileHeader);
    if (cipherSize != header->originalSize) {
        return false;
    }

    try {
        // Get encryption key
        uint8_t key[AES_KEY_SIZE];
        GetEncryptionKey(key);

        // Create AEAD cipher (AES-256/GCM)
        auto aead = Botan::AEAD_Mode::create("AES-256/GCM", Botan::DECRYPTION);
        if (!aead) {
            return false;
        }

        // Set the key
        aead->set_key(key, AES_KEY_SIZE);

        // Start decryption with nonce from header
        aead->start(header->nonce, NONCE_SIZE);

        // Prepare buffer: ciphertext + tag (tag must be appended for verification)
        Botan::secure_vector<uint8_t> buffer;
        buffer.reserve(cipherSize + TAG_SIZE);
        
        // Copy ciphertext
        const uint8_t* cipherStart = encryptedData + sizeof(EncryptedFileHeader);
        buffer.insert(buffer.end(), cipherStart, cipherStart + cipherSize);
        
        // Append the authentication tag
        buffer.insert(buffer.end(), header->tag, header->tag + TAG_SIZE);

        // Finish decryption (this verifies the tag)
        aead->finish(buffer);

        // Allocate output
        *plainSize = buffer.size();
        *plainData = new uint8_t[*plainSize];
        memcpy(*plainData, buffer.data(), *plainSize);

        // Clear sensitive data
        memset(key, 0, AES_KEY_SIZE);

        return true;
    }
    catch (const Botan::Exception& e) {
        // Authentication failure or other error
        #ifdef _DEBUG
        OutputDebugStringA("TableObfuscation::Decrypt error: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
        #endif
        return false;
    }
    catch (...) {
        return false;
    }
}

bool EncryptToFile(const char* filename, const uint8_t* data, size_t dataSize) {
    uint8_t* encryptedData = nullptr;
    size_t encryptedSize = 0;

    if (!Encrypt(data, dataSize, &encryptedData, &encryptedSize)) {
        return false;
    }

    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        delete[] encryptedData;
        return false;
    }

    size_t written = fwrite(encryptedData, 1, encryptedSize, fp);
    fclose(fp);
    delete[] encryptedData;

    return written == encryptedSize;
}

bool DecryptFromFile(const char* filename, uint8_t** data, size_t* dataSize) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return false;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    int fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fileSize <= 0) {
        fclose(fp);
        return false;
    }

    // Read file content
    uint8_t* fileContent = new uint8_t[fileSize];
    size_t readBytes = fread(fileContent, 1, fileSize, fp);
    fclose(fp);

    if (readBytes != static_cast<size_t>(fileSize)) {
        delete[] fileContent;
        return false;
    }

    // Check if file is encrypted
    if (!IsEncrypted(fileContent, fileSize)) {
        // File is not encrypted, return as-is (for backward compatibility)
        *data = fileContent;
        *dataSize = fileSize;
        return true;
    }

    // Decrypt
    bool success = Decrypt(fileContent, fileSize, data, dataSize);
    delete[] fileContent;

    return success;
}

} // namespace TableObfuscation
