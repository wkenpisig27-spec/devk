/**
 * LicenseGenerator.cpp
 * 
 * PKO License Key Generator Tool
 * 
 * This tool generates valid license files (.lic) for PKO binaries.
 * Keep this tool PRIVATE - never distribute it!
 * 
 * Compile with:
 *   cl /EHsc LicenseGenerator.cpp /link crypt32.lib bcrypt.lib
 * 
 * Or with g++ (MinGW):
 *   g++ -o LicenseGenerator.exe LicenseGenerator.cpp -lcrypt32 -lbcrypt
 */

#include <Windows.h>
#include <wincrypt.h>
#include <bcrypt.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <ctime>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "bcrypt.lib")

// ============================================
// SECRET KEY - KEEP THIS PRIVATE!
// Must match the key in LicenseValidator.cpp
// 512-bit cryptographically random key
// ============================================
static const char* LICENSE_SECRET_KEY = "fc39b94bb83b5f61ac11c4bc1a36774270fa55e84adf4f1ef4d118a34559910b5ae2095a9734b4f0fa2909793764a20da14c7a524f4b8c6b7ab75c335a5c190c";

// License file magic bytes
static const char* LICENSE_MAGIC = "PKOLIC1";

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

// Base64 encode
static std::string Base64Encode(const std::string& data) {
    DWORD encodedLen = 0;
    CryptBinaryToStringA((BYTE*)data.c_str(), (DWORD)data.length(),
                         CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                         nullptr, &encodedLen);
    
    std::string encoded(encodedLen, 0);
    CryptBinaryToStringA((BYTE*)data.c_str(), (DWORD)data.length(),
                         CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                         &encoded[0], &encodedLen);
    
    while (!encoded.empty() && encoded.back() == 0) {
        encoded.pop_back();
    }
    return encoded;
}

// HMAC-SHA256 for signing
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

// XOR obfuscation
static std::string XorObfuscate(const std::string& data, const std::string& key) {
    std::string result = data;
    for (size_t i = 0; i < result.length(); i++) {
        result[i] ^= key[i % key.length()];
    }
    return result;
}

// Parse date string (YYYY-MM-DD) to time_t
static time_t ParseDate(const std::string& dateStr) {
    if (dateStr.empty() || dateStr == "0" || dateStr == "never") {
        return 0; // Never expires
    }
    
    struct tm tm = {};
    int year, month, day;
    
    if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) != 3) {
        return 0;
    }
    
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 59;
    
    return mktime(&tm);
}

// ============================================
// LICENSE GENERATION
// ============================================

struct LicenseData {
    std::string owner;
    std::string productType;
    std::string hwid;          // Hardware ID or "*" for any machine
    time_t createdAt;
    time_t expiresAt;          // 0 = never expires
};

std::string GenerateLicense(const LicenseData& data) {
    // Build license string: magic|owner|product|hwid|created|expires
    std::stringstream ss;
    ss << LICENSE_MAGIC << "|";
    ss << data.owner << "|";
    ss << data.productType << "|";
    ss << data.hwid << "|";
    ss << data.createdAt << "|";
    ss << data.expiresAt;
    
    std::string dataToSign = ss.str();
    
    // Generate HMAC signature
    std::string signature = HmacSha256(LICENSE_SECRET_KEY, dataToSign);
    
    // Add signature
    ss << "|" << signature;
    std::string fullData = ss.str();
    
    // Obfuscate
    std::string obfuscated = XorObfuscate(fullData, LICENSE_SECRET_KEY);
    
    // Base64 encode
    return Base64Encode(obfuscated);
}

// ============================================
// INTERACTIVE MODE
// ============================================

void PrintBanner() {
    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << "     PKO License Generator v1.0\n";
    std::cout << "============================================\n";
    std::cout << "\n";
    std::cout << "WARNING: Keep this tool private!\n";
    std::cout << "         Never distribute the generator.\n";
    std::cout << "\n";
}

void InteractiveMode() {
    LicenseData data;
    std::string input;
    
    // Owner name
    std::cout << "Enter owner name: ";
    std::getline(std::cin, data.owner);
    if (data.owner.empty()) {
        data.owner = "Licensed User";
    }
    
    // Product type
    std::cout << "Enter product type (full/server/client) [full]: ";
    std::getline(std::cin, data.productType);
    if (data.productType.empty()) {
        data.productType = "full";
    }
    
    // HWID
    std::cout << "Enter Hardware ID (or * for any machine) [*]: ";
    std::getline(std::cin, data.hwid);
    if (data.hwid.empty()) {
        data.hwid = "*";
    }
    
    // Expiry date
    std::cout << "Enter expiry date (YYYY-MM-DD or 'never') [never]: ";
    std::getline(std::cin, input);
    data.expiresAt = ParseDate(input);
    
    // Created at
    data.createdAt = time(nullptr);
    
    // Generate
    std::string license = GenerateLicense(data);
    
    // Output filename
    std::cout << "Enter output filename [license.lic]: ";
    std::getline(std::cin, input);
    if (input.empty()) {
        input = "license.lic";
    }
    
    // Save to file
    std::ofstream file(input);
    if (file.is_open()) {
        file << license;
        file.close();
        std::cout << "\n";
        std::cout << "License generated successfully!\n";
        std::cout << "Saved to: " << input << "\n";
        std::cout << "\n";
        std::cout << "License Details:\n";
        std::cout << "  Owner: " << data.owner << "\n";
        std::cout << "  Type: " << data.productType << "\n";
        std::cout << "  HWID: " << data.hwid << "\n";
        std::cout << "  Created: " << ctime(&data.createdAt);
        if (data.expiresAt == 0) {
            std::cout << "  Expires: Never\n";
        } else {
            std::cout << "  Expires: " << ctime(&data.expiresAt);
        }
        std::cout << "\n";
    } else {
        std::cerr << "Error: Could not write to file: " << input << "\n";
    }
}

// ============================================
// COMMAND LINE MODE
// ============================================

void PrintUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "  -o, --owner <name>     Owner name (required)\n";
    std::cout << "  -t, --type <type>      Product type: full, server, client (default: full)\n";
    std::cout << "  -h, --hwid <hwid>      Hardware ID or * for any machine (default: *)\n";
    std::cout << "  -e, --expires <date>   Expiry date YYYY-MM-DD or 'never' (default: never)\n";
    std::cout << "  -f, --file <path>      Output file path (default: license.lic)\n";
    std::cout << "  --help                 Show this help\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " -o \"John Doe\" -f john_license.lic\n";
    std::cout << "  " << programName << " -o \"Company\" -e 2025-12-31 -h \"abc123def456\"\n";
    std::cout << "\n";
    std::cout << "If no arguments provided, runs in interactive mode.\n";
}

int main(int argc, char* argv[]) {
    PrintBanner();
    
    if (argc == 1) {
        // Interactive mode
        InteractiveMode();
        return 0;
    }
    
    // Parse command line arguments
    LicenseData data;
    data.owner = "";
    data.productType = "full";
    data.hwid = "*";
    data.expiresAt = 0;
    data.createdAt = time(nullptr);
    std::string outputFile = "license.lic";
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        } else if ((arg == "-o" || arg == "--owner") && i + 1 < argc) {
            data.owner = argv[++i];
        } else if ((arg == "-t" || arg == "--type") && i + 1 < argc) {
            data.productType = argv[++i];
        } else if ((arg == "-h" || arg == "--hwid") && i + 1 < argc) {
            data.hwid = argv[++i];
        } else if ((arg == "-e" || arg == "--expires") && i + 1 < argc) {
            data.expiresAt = ParseDate(argv[++i]);
        } else if ((arg == "-f" || arg == "--file") && i + 1 < argc) {
            outputFile = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }
    
    // Validate required fields
    if (data.owner.empty()) {
        std::cerr << "Error: Owner name is required.\n";
        PrintUsage(argv[0]);
        return 1;
    }
    
    // Generate license
    std::string license = GenerateLicense(data);
    
    // Save to file
    std::ofstream file(outputFile);
    if (file.is_open()) {
        file << license;
        file.close();
        std::cout << "License generated successfully!\n";
        std::cout << "Saved to: " << outputFile << "\n";
        std::cout << "\n";
        std::cout << "License Details:\n";
        std::cout << "  Owner: " << data.owner << "\n";
        std::cout << "  Type: " << data.productType << "\n";
        std::cout << "  HWID: " << data.hwid << "\n";
        if (data.expiresAt == 0) {
            std::cout << "  Expires: Never\n";
        } else {
            std::cout << "  Expires: " << ctime(&data.expiresAt);
        }
    } else {
        std::cerr << "Error: Could not write to file: " << outputFile << "\n";
        return 1;
    }
    
    return 0;
}
