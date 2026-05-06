/**
 * LicenseValidator.cpp
 * 
 * OFFLINE License validation implementation for PKO binaries.
 * 
 * Windows: Uses CryptoAPI/BCrypt for crypto, Win32 for HWID
 * Linux:   Uses OpenSSL for crypto, POSIX for HWID
 * 
 * License Format:
 * The license file contains a base64-encoded, signed data block with:
 * - Owner name
 * - Product type
 * - HWID (machine binding)
 * - Creation date
 * - Expiry date (optional)
 * - HMAC signature for tamper detection
 */

#include "LicenseValidator.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#include <wincrypt.h>
#include <iphlpapi.h>
#include <intrin.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "bcrypt.lib")
#else
// Linux
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <cpuid.h>
#include <climits>
#endif

// ============================================
// SECRET KEY - KEEP THIS PRIVATE!
// Must match the key in LicenseGenerator*.cpp
// 512-bit cryptographically random key
// ============================================
static const char* LICENSE_SECRET_KEY = "fc39b94bb83b5f61ac11c4bc1a36774270fa55e84adf4f1ef4d118a34559910b5ae2095a9734b4f0fa2909793764a20da14c7a524f4b8c6b7ab75c335a5c190c";

// License file magic bytes (to identify valid files)
static const char* LICENSE_MAGIC = "PKOLIC1";

namespace License {

// ============================================
// UTILITY FUNCTIONS
// ============================================

// Hex encode
static std::string ToHex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; i++) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

// Hex decode
static std::vector<unsigned char> FromHex(const std::string& hex) {
    std::vector<unsigned char> result;
    for (size_t i = 0; i < hex.length(); i += 2) {
        unsigned int byte;
        std::stringstream ss;
        ss << std::hex << hex.substr(i, 2);
        ss >> byte;
        result.push_back(static_cast<unsigned char>(byte));
    }
    return result;
}

// Base64 encode
#ifdef _WIN32
static std::string Base64Encode(const std::string& data) {
    DWORD encodedLen = 0;
    CryptBinaryToStringA((BYTE*)data.c_str(), (DWORD)data.length(),
                         CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                         nullptr, &encodedLen);
    
    std::string encoded(encodedLen, 0);
    CryptBinaryToStringA((BYTE*)data.c_str(), (DWORD)data.length(),
                         CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                         &encoded[0], &encodedLen);
    
    // Remove null terminator if present
    while (!encoded.empty() && encoded.back() == 0) {
        encoded.pop_back();
    }
    return encoded;
}

// Base64 decode
static std::string Base64Decode(const std::string& encoded) {
    DWORD decodedLen = 0;
    CryptStringToBinaryA(encoded.c_str(), (DWORD)encoded.length(),
                         CRYPT_STRING_BASE64,
                         nullptr, &decodedLen, nullptr, nullptr);
    
    std::string decoded(decodedLen, 0);
    CryptStringToBinaryA(encoded.c_str(), (DWORD)encoded.length(),
                         CRYPT_STRING_BASE64,
                         (BYTE*)&decoded[0], &decodedLen, nullptr, nullptr);
    
    decoded.resize(decodedLen);
    return decoded;
}

// SHA256 hash
static std::string Sha256(const std::string& data) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE hash[32];
    DWORD hashLen = 32;
    
    if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return "";
    }
    
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    if (!CryptHashData(hHash, (BYTE*)data.c_str(), (DWORD)data.length(), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    
    return ToHex(hash, 32);
}

// HMAC-SHA256 for signature verification
static std::string HmacSha256(const std::string& key, const std::string& data) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    DWORD cbHashObject = 0;
    DWORD cbData = 0;
    DWORD cbHash = 32;
    std::vector<BYTE> pbHashObject;
    BYTE pbHash[32];

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0) {
        return "";
    }

    if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0) != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    pbHashObject.resize(cbHashObject);

    if (BCryptCreateHash(hAlg, &hHash, pbHashObject.data(), cbHashObject, 
                         (PBYTE)key.c_str(), (ULONG)key.length(), 0) != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    if (BCryptHashData(hHash, (PBYTE)data.c_str(), (ULONG)data.length(), 0) != 0) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    if (BCryptFinishHash(hHash, pbHash, cbHash, 0) != 0) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    return ToHex(pbHash, 32);
}

#else // Linux implementations

static std::string Base64Encode(const std::string& data) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, data.c_str(), (int)data.length());
    BIO_flush(b64);
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);
    return result;
}

static std::string Base64Decode(const std::string& encoded) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new_mem_buf(encoded.c_str(), (int)encoded.length());
    bmem = BIO_push(b64, bmem);
    BIO_set_flags(bmem, BIO_FLAGS_BASE64_NO_NL);
    
    std::vector<char> buffer(encoded.length());
    int decodedLen = BIO_read(bmem, buffer.data(), (int)buffer.size());
    BIO_free_all(bmem);
    
    if (decodedLen <= 0) return "";
    return std::string(buffer.data(), decodedLen);
}

static std::string Sha256(const std::string& data) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";
    
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(ctx, data.c_str(), data.length()) != 1 ||
        EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    
    EVP_MD_CTX_free(ctx);
    return ToHex(hash, hashLen);
}

static std::string HmacSha256(const std::string& key, const std::string& data) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    
    HMAC(EVP_sha256(), key.c_str(), (int)key.length(),
         (const unsigned char*)data.c_str(), data.length(),
         hash, &hashLen);
    
    if (hashLen == 0) return "";
    return ToHex(hash, hashLen);
}

#endif // _WIN32

// Simple XOR obfuscation for the license data
static std::string XorObfuscate(const std::string& data, const std::string& key) {
    std::string result = data;
    for (size_t i = 0; i < result.length(); i++) {
        result[i] ^= key[i % key.length()];
    }
    return result;
}

// ============================================
// HARDWARE ID GENERATION
// ============================================

#ifdef _WIN32

std::string Validator::getHardwareId() {
    std::stringstream hwid;
    
    // 1. Get CPU ID
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 0);
    hwid << std::hex << cpuInfo[1] << cpuInfo[3] << cpuInfo[2];
    
    __cpuid(cpuInfo, 1);
    hwid << std::hex << cpuInfo[0] << cpuInfo[3];
    
    // 2. Get Computer Name
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    if (GetComputerNameA(computerName, &size)) {
        hwid << "_" << computerName;
    }
    
    // 3. Get Volume Serial Number (C: drive)
    DWORD volumeSerial = 0;
    if (GetVolumeInformationA("C:\\", nullptr, 0, &volumeSerial, nullptr, nullptr, nullptr, 0)) {
        hwid << "_" << std::hex << volumeSerial;
    }
    
    // 4. Get first MAC address
    ULONG bufLen = sizeof(IP_ADAPTER_INFO);
    std::vector<BYTE> buffer(bufLen);
    PIP_ADAPTER_INFO pAdapterInfo = (PIP_ADAPTER_INFO)buffer.data();
    
    if (GetAdaptersInfo(pAdapterInfo, &bufLen) == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(bufLen);
        pAdapterInfo = (PIP_ADAPTER_INFO)buffer.data();
    }
    
    if (GetAdaptersInfo(pAdapterInfo, &bufLen) == NO_ERROR) {
        hwid << "_";
        for (UINT i = 0; i < pAdapterInfo->AddressLength; i++) {
            hwid << std::hex << std::setw(2) << std::setfill('0') 
                 << (int)pAdapterInfo->Address[i];
        }
    }
    
    // Hash the combined data for a fixed-length, consistent ID
    return Sha256(hwid.str());
}

#else // Linux HWID

// Note: Cannot use #include <cpuid.h> because Botan's cpuid.h shadows the GCC
// system header due to -I include path ordering. Use __cpuid_count intrinsic directly.
static inline void _pko_cpuid(unsigned int leaf, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(0)
    );
}

std::string Validator::getHardwareId() {
    std::stringstream hwid;
    
    // 1. Get CPU ID via inline asm cpuid
    unsigned int eax, ebx, ecx, edx;
    _pko_cpuid(0, &eax, &ebx, &ecx, &edx);
    hwid << std::hex << ebx << edx << ecx;
    _pko_cpuid(1, &eax, &ebx, &ecx, &edx);
    hwid << std::hex << eax << edx;
    
    // 2. Get hostname
    char hostname[256] = {0};
    if (gethostname(hostname, sizeof(hostname) - 1) == 0) {
        hwid << "_" << hostname;
    }
    
    // 3. Machine ID (replaces Windows volume serial)
    std::ifstream machineId("/etc/machine-id");
    if (machineId.is_open()) {
        std::string mid;
        std::getline(machineId, mid);
        if (!mid.empty()) {
            hwid << "_" << mid;
        }
    }
    
    // 4. Get first MAC address via getifaddrs
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == 0) {
        for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr) continue;
            if (ifa->ifa_addr->sa_family != AF_PACKET) continue;
            // Skip loopback
            if (ifa->ifa_flags & IFF_LOOPBACK) continue;
            
            struct sockaddr_ll* sll = (struct sockaddr_ll*)ifa->ifa_addr;
            if (sll->sll_halen == 6) {
                hwid << "_";
                for (int i = 0; i < 6; i++) {
                    hwid << std::hex << std::setw(2) << std::setfill('0')
                         << (int)sll->sll_addr[i];
                }
                break; // Use first non-loopback interface
            }
        }
        freeifaddrs(ifaddr);
    }
    
    // Hash the combined data for a fixed-length, consistent ID
    return Sha256(hwid.str());
}

#endif // _WIN32

// ============================================
// VALIDATOR CLASS IMPLEMENTATION
// ============================================

Validator::Validator() {
    m_hardwareId = getHardwareId();
}

bool Validator::licenseFileExists(const std::string& filepath) {
    std::ifstream file(filepath);
    return file.good();
}

bool Validator::verifyChecksum(const std::string& data, const std::string& checksum) {
    std::string computed = HmacSha256(LICENSE_SECRET_KEY, data);
    return computed == checksum;
}

bool Validator::decodeLicense(const std::string& encoded, LicenseInfo& info) {
    // Decode base64
    std::string decoded = Base64Decode(encoded);
    if (decoded.empty()) {
        return false;
    }
    
    // De-obfuscate
    std::string data = XorObfuscate(decoded, LICENSE_SECRET_KEY);
    
    // Parse format: magic|owner|product|hwid|created|expires|signature
    std::vector<std::string> parts;
    std::stringstream ss(data);
    std::string part;
    while (std::getline(ss, part, '|')) {
        parts.push_back(part);
    }
    
    if (parts.size() < 7) {
        return false;
    }
    
    // Verify magic
    if (parts[0] != LICENSE_MAGIC) {
        return false;
    }
    
    // Extract fields
    info.owner = parts[1];
    info.productType = parts[2];
    info.boundHWID = parts[3];
    
    try {
        info.createdAt = std::stoll(parts[4]);
        info.expiresAt = std::stoll(parts[5]);
    } catch (...) {
        return false;
    }
    
    std::string signature = parts[6];
    
    // Verify signature (HMAC of data without signature)
    std::string dataToSign = parts[0] + "|" + parts[1] + "|" + parts[2] + "|" + 
                             parts[3] + "|" + parts[4] + "|" + parts[5];
    
    if (!verifyChecksum(dataToSign, signature)) {
        return false;
    }
    
    return true;
}

LicenseInfo Validator::validateFile(const std::string& filepath, const std::string& productName) {
    LicenseInfo result = {};
    result.valid = false;
    
    // Check if file exists
    std::ifstream file(filepath);
    if (!file.is_open()) {
        result.result = ValidationResult::FILE_NOT_FOUND;
        result.message = "License file not found: " + filepath;
        return result;
    }
    
    // Read file content
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Trim whitespace
    content.erase(0, content.find_first_not_of(" \t\n\r"));
    content.erase(content.find_last_not_of(" \t\n\r") + 1);
    
    return validateString(content, productName);
}

LicenseInfo Validator::validateString(const std::string& licenseData, const std::string& productName) {
    LicenseInfo result = {};
    result.valid = false;
    
    if (licenseData.empty()) {
        result.result = ValidationResult::INVALID_FORMAT;
        result.message = "License data is empty";
        return result;
    }
    
    // Decode and parse license
    if (!decodeLicense(licenseData, result)) {
        result.result = ValidationResult::INVALID_LICENSE;
        result.message = "Invalid or corrupted license";
        return result;
    }
    
    // Check expiry
    if (result.expiresAt != 0) {
        time_t now = time(nullptr);
        if (now > result.expiresAt) {
            result.result = ValidationResult::LICENSE_EXPIRED;
            result.message = "License has expired";
            return result;
        }
    }
    
    // Check HWID
    if (result.boundHWID != "*" && result.boundHWID != m_hardwareId) {
        result.result = ValidationResult::HWID_MISMATCH;
        result.message = "License is bound to a different machine";
        return result;
    }
    
    // License is valid!
    result.valid = true;
    result.result = ValidationResult::SUCCESS;
    result.message = "License validated successfully";
    
    return result;
}

// ============================================
// HELPER FUNCTIONS
// ============================================

const char* ResultToString(ValidationResult result) {
    switch (result) {
        case ValidationResult::SUCCESS: return "Success";
        case ValidationResult::INVALID_LICENSE: return "Invalid License";
        case ValidationResult::LICENSE_EXPIRED: return "License Expired";
        case ValidationResult::HWID_MISMATCH: return "Machine Mismatch";
        case ValidationResult::FILE_NOT_FOUND: return "License File Not Found";
        case ValidationResult::INVALID_FORMAT: return "Invalid Format";
        case ValidationResult::INTERNAL_ERROR: return "Internal Error";
        default: return "Unknown Error";
    }
}

// Global instance to store valid license info (file scope)
static LicenseInfo g_CurrentLicenseInfo = { false };

LicenseInfo GetCurrentLicenseInfo() {
    return g_CurrentLicenseInfo;
}

bool ValidateOrExit(const std::string& licenseFileName, const std::string& productName) {
#ifndef _WIN32
    // License validation disabled on Linux builds
    printf("[License] Skipped on Linux build\n");
    return true;
#else
    // Resolve absolute path to license file (relative to executable)
#ifdef _WIN32
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
        strcpy_s(exePath, ".");
    }
#else
    char exePath[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len <= 0) {
        strcpy(exePath, ".");
        len = 1;
    }
    exePath[len] = '\0';
#endif
    std::string fullPath = exePath;
    size_t lastSlash = fullPath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        fullPath = fullPath.substr(0, lastSlash + 1);
    } else {
        fullPath = "./";
    }
    fullPath += licenseFileName;

    Validator validator;
    
    // Check if license file exists
    if (!Validator::licenseFileExists(fullPath)) {
        std::string msg = "License file not found: " + fullPath + 
                          "\nPlease place your license.lic file in the application directory.";
#ifdef _WIN32
        MessageBoxA(nullptr, msg.c_str(), "License Error", MB_OK | MB_ICONERROR);
#else
        fprintf(stderr, "[License Error] %s\n", msg.c_str());
#endif
        return false;
    }

    // Validate
    LicenseInfo info = validator.validateFile(fullPath, productName);
    
    // Store in global
    g_CurrentLicenseInfo = info;
    
    if (!info.valid) {
        std::stringstream msg;
        msg << "License validation failed!\n\n";
        msg << "Error: " << ResultToString(info.result) << "\n";
        msg << "Details: " << info.message << "\n\n";
        
        if (info.result == ValidationResult::HWID_MISMATCH) {
            msg << "Your Hardware ID: " << Validator::getHardwareId().substr(0, 16) << "...\n";
            msg << "\nContact your license provider to get a license for this machine.";
        }
        
#ifdef _WIN32
        MessageBoxA(nullptr, msg.str().c_str(), "License Error", MB_OK | MB_ICONERROR);
#else
        fprintf(stderr, "[License Error] %s\n", msg.str().c_str());
#endif
        return false;
    }
    
    // Print license info to console
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD savedAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    
    // Save current console attributes
    if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo)) {
        savedAttributes = consoleInfo.wAttributes;
    }
    
    const WORD COLOR_GREEN = FOREGROUND_GREEN | FOREGROUND_INTENSITY; 
    
    auto PrintLine = [&](const char* label, const std::string& value) {
        SetConsoleTextAttribute(hConsole, COLOR_GREEN);
        printf("%-15s: %s\n", label, value.c_str());
    };
#else
    // Linux: use ANSI escape codes for green text
    auto PrintLine = [](const char* label, const std::string& value) {
        printf("\033[1;32m%-15s: %s\033[0m\n", label, value.c_str());
    };
#endif

    // 1. License Status
    PrintLine("License Status", "VALID");

    // 2. License Type
    std::string typeUpper = info.productType;
    std::transform(typeUpper.begin(), typeUpper.end(), typeUpper.begin(), ::toupper);
    PrintLine("License Type", typeUpper);

    // 3. Server
    PrintLine("Server", info.owner);

    // 4. Machine ID (Masked)
    std::string maskedHWID;
    if (info.boundHWID == "*") {
        maskedHWID = "ANY MACHINE";
    } else if (info.boundHWID.length() > 10) {
        maskedHWID = info.boundHWID.substr(0, 6) + "****" + info.boundHWID.substr(info.boundHWID.length() - 4);
    } else {
        maskedHWID = info.boundHWID;
    }
    PrintLine("Machine ID", maskedHWID);

    // 5. Expiry Date
    std::string dateStr = "NEVER";
    if (info.expiresAt != 0) {
        char buf[64];
        time_t expiry = static_cast<time_t>(info.expiresAt);
        struct tm* tm = localtime(&expiry);
        strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
        dateStr = buf;
    }
    PrintLine("Expiry Date", dateStr);

    printf("\n");
    
#ifdef _WIN32
    // Restore original console attributes
    SetConsoleTextAttribute(hConsole, savedAttributes);
#endif
    
    return true;
#endif // !_WIN32 (license disabled on Linux)
}


} // namespace License
