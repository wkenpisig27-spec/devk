/**
 * GetHWID.cpp
 * 
 * Hardware ID Extractor Tool
 * 
 * This tool displays the Hardware ID (HWID) of the current machine.
 * Give this to your customers so they can send you their HWID
 * for machine-bound license generation.
 * 
 * Compile with:
 *   cl /EHsc GetHWID.cpp /link crypt32.lib iphlpapi.lib
 * 
 * Or with g++ (MinGW):
 *   g++ -o GetHWID.exe GetHWID.cpp -lcrypt32 -liphlpapi
 */

#include <Windows.h>
#include <wincrypt.h>
#include <iphlpapi.h>
#include <intrin.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <string>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "iphlpapi.lib")

// Hex encode
static std::string ToHex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; i++) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
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

// Get Hardware ID (must match LicenseValidator.cpp exactly!)
std::string GetHardwareId() {
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

int main() {
    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << "     PKO Hardware ID Extractor\n";
    std::cout << "============================================\n";
    std::cout << "\n";
    
    std::string hwid = GetHardwareId();
    
    std::cout << "Your Hardware ID is:\n";
    std::cout << "\n";
    std::cout << "  " << hwid << "\n";
    std::cout << "\n";
    std::cout << "Please send this ID to your license provider\n";
    std::cout << "to get a machine-bound license.\n";
    std::cout << "\n";
    
    // Also copy to clipboard
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, hwid.size() + 1);
        if (hMem) {
            memcpy(GlobalLock(hMem), hwid.c_str(), hwid.size() + 1);
            GlobalUnlock(hMem);
            SetClipboardData(CF_TEXT, hMem);
            std::cout << "(Copied to clipboard!)\n";
        }
        CloseClipboard();
    }
    
    std::cout << "\n";
    std::cout << "Press Enter to exit...";
    std::cin.get();
    
    return 0;
}
