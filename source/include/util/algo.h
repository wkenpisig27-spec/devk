
#ifndef _ALGO_H_
#define _ALGO_H_

#include <string>

// Standard Utilities
int base64(char const* src, unsigned int src_len, char* dst,
		   unsigned int dst_max_len, unsigned int* dst_len);
int ibase64(char const* src, unsigned int src_len, char* dst, unsigned int* dst_len);
unsigned char const* sha1(unsigned char const* msg, unsigned int len, unsigned char* md);

// Packet Obfuscation
void init_Noise(int nNoise, char szKey[4]);
bool encrypt_Noise(char szKey[4], char* src, unsigned int src_len);
bool decrypt_Noise(char szkey[4], char* src, unsigned int src_len);

// MD5 Utilities
void md5(char const* msg, unsigned char dig[16]);
void md5string(char const* msg, char str[33]);

// ===========================================================================
// DEPRECATED CODE REMOVED (Phase 1 & 2 Cleanup - 2026-01-18)
// Removed:
// - encrypt_A, encrypt_B
// - dbpswd_out, dbpswd_in
// - KCHAPc, KCHAPs, CSHA1, C3DES, CDES classes
// ===========================================================================

#endif
