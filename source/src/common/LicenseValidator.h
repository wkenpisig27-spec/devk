#pragma once
/**
 * LicenseValidator.h
 * 
 * OFFLINE License validation module for PKO binaries.
 * Validates license keys locally without network connection.
 * 
 * Features:
 * - Hardware ID (HWID) fingerprinting
 * - Cryptographic license key validation
 * - Machine binding (one license = one machine)
 * - Expiry date support
 * - No internet required
 */

#ifndef LICENSE_VALIDATOR_H
#define LICENSE_VALIDATOR_H

#include <string>
#include <ctime>

namespace License {

    // Validation result codes
    enum class ValidationResult {
        SUCCESS,                    // License is valid
        INVALID_LICENSE,            // License key invalid or corrupt
        LICENSE_EXPIRED,            // License has expired
        HWID_MISMATCH,              // License bound to different machine
        FILE_NOT_FOUND,             // License file not found
        INVALID_FORMAT,             // License file format invalid
        INTERNAL_ERROR              // Unexpected error
    };

    // License info returned from validation
    struct LicenseInfo {
        bool valid;
        std::string owner;
        std::string productType;
        time_t expiresAt;           // 0 = never expires
        time_t createdAt;
        std::string boundHWID;
        std::string message;
        ValidationResult result;
    };

    /**
     * Main validator class
     * 
     * Usage:
     *   License::Validator validator;
     *   auto result = validator.validateFile("license.lic", "GameServer");
     *   if (!result.valid) {
     *       // Show error and exit
     *   }
     */
    class Validator {
    public:
        Validator();

        /**
         * Validate a license file
         * @param filepath Path to the .lic license file
         * @param productName Name of the product (e.g., "GameServer", "Client")
         * @return LicenseInfo with validation result
         */
        LicenseInfo validateFile(const std::string& filepath, const std::string& productName);

        /**
         * Validate a license string directly
         * @param licenseData The license data string
         * @param productName Name of the product
         * @return LicenseInfo with validation result
         */
        LicenseInfo validateString(const std::string& licenseData, const std::string& productName);

        /**
         * Get the hardware ID for this machine
         * Used for machine binding
         */
        static std::string getHardwareId();

        /**
         * Check if a license file exists
         */
        static bool licenseFileExists(const std::string& filepath);

    private:
        std::string m_hardwareId;

        // Decode and verify license
        bool decodeLicense(const std::string& encoded, LicenseInfo& info);
        
        // Verify checksum
        bool verifyChecksum(const std::string& data, const std::string& checksum);
    };

    /**
     * Simple validation function for quick integration
     * Validates and shows error dialog if failed
     * 
     * @param licenseFilePath Path to license.lic file
     * @param productName Product name (e.g., "GameServer")
     * @return true if valid, false if should exit
     */
    bool ValidateOrExit(const std::string& licenseFilePath, const std::string& productName);

    /**
     * Convert result code to string
     */
    const char* ResultToString(ValidationResult result);

    /**
     * Get the currently valid license info
     * Populated after ValidateOrExit() is called
     */
    LicenseInfo GetCurrentLicenseInfo();

} // namespace License

#endif // LICENSE_VALIDATOR_H
