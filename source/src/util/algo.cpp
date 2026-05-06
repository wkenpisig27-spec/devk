
#include "util.h"
#include "algo.h"
#include "md5.h"

#ifdef _MSC_VER
#pragma warning(disable : 4018 4267)
#define NKD __declspec(naked)
#else
#define NKD
#endif

#define INL inline

// base64 encode and decode
#define OK (0)
#define FAIL (-1)
#define BUFOVER (-2)
#define CHAR64(c) (((c) < 0 || (c) > 127) ? -1 : index_64[(c)])
static char const basis_64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
//"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";

static char const basis_128[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789~!@#$%^&*()_+|\0";

static char const index_64[128] =
	{
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
		-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
		-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1};

int base64(char const* src, unsigned int src_len, char* dst,
		   unsigned int dst_max_len, unsigned int* dst_len) {
	unsigned char const* in = (unsigned char const*)src;
	unsigned char* out = (unsigned char*)dst;
	unsigned char oval;
	unsigned int olen;
	char* blah;

	olen = (src_len + 2) / 3 * 4;
	if (dst_len != nullptr)
		*dst_len = olen;
	if (dst_max_len < olen)
		return BUFOVER;

	blah = (char*)out;
	while (src_len >= 3) {
		*out++ = basis_64[in[0] >> 2];
		*out++ = basis_64[((in[0] << 4) & 0x30) | (in[1] >> 4)];
		*out++ = basis_64[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
		*out++ = basis_64[in[2] & 0x3f];
		in += 3;
		src_len -= 3;
	}

	if (src_len > 0) {
		*out++ = basis_64[in[0] >> 2];
		oval = (in[0] << 4) & 0x30;
		if (src_len > 1)
			oval |= in[1] >> 4;
		*out++ = basis_64[oval];
		*out++ = (src_len < 2) ? '=' : basis_64[(in[1] << 2) & 0x3c];
		*out++ = '=';
	}

	if (olen < dst_max_len)
		*out = '\0';
	return OK;
}

int ibase64(char const* src, unsigned int src_len, char* dst, unsigned int* dst_len) {
	unsigned len = 0, lup;
	int c1, c2, c3, c4;

	if (dst == nullptr)
		return FAIL;

	if (src[0] == '+' && src[1] == ' ')
		src += 2;
	if (*src == '\r')
		return FAIL;

	for (lup = 0; lup < src_len / 4; ++lup) {
		c1 = src[0];
		if (CHAR64(c1) == -1)
			return FAIL;
		c2 = src[1];
		if (CHAR64(c2) == -1)
			return FAIL;
		c3 = src[2];
		if (c3 != '=' && CHAR64(c3) == -1)
			return FAIL;
		c4 = src[3];
		if (c4 != '=' && CHAR64(c4) == -1)
			return FAIL;
		src += 4;
		*dst++ = (CHAR64(c1) << 2) | (CHAR64(c2) >> 4);
		++len;
		if (c3 != '=') {
			*dst++ = ((CHAR64(c2) << 4) & 0xf0) | (CHAR64(c3) >> 2);
			++len;
			if (c4 != '=') {
				*dst++ = ((CHAR64(c3) << 6) & 0xc0) | CHAR64(c4);
				++len;
			}
		}
	} // end for

	//*dst = 0;
	if (dst_len != nullptr)
		*dst_len = len;
	return OK;
}

// SHA-1
unsigned int func_S(char n, unsigned int num) { return (num << n) | (num >> (32 - n)); }

unsigned int func_F(unsigned int t, unsigned int B, unsigned int C, unsigned int D) {
	if (t < 20) {
		return ((B & C) | ((~B) & D));
	} else if (t < 40) {
		return (B ^ C ^ D);
	} else if (t < 60) {
		return ((B & C) | (B & D) | (C & D));
	} else if (t < 80) {
		return (B ^ C ^ D);
	} else
		return 0;
}

unsigned char const* sha1(unsigned char const* msg, unsigned int len, unsigned char* md) {
	unsigned int message_length;
	unsigned char i, ctemp, index;
	unsigned char hasAdd1 = 0;
	unsigned char hasAddLen = 0;
	unsigned int W[16];
	unsigned int H[5];
	unsigned int A, B, C, D, E, s;
	unsigned int MASK = 0x0000000F;
	unsigned int K[4] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6};
	unsigned int TEMP;
	unsigned int t;

	message_length = len;
	H[0] = 0x67452301;
	H[1] = 0xEFCDAB89;
	H[2] = 0x98BADCFE;
	H[3] = 0x10325476;
	H[4] = 0xC3D2E1F0;
	index = 0;

	while (!hasAddLen) {
		for (t = 0; t < 16; ++t) {
			W[t] = 0;
			for (i = 0; i < 4; ++i) {
				if (!hasAdd1) {
					if (index >= len) {
						ctemp = 0x80;
						hasAdd1 = 1;
					} else
						ctemp = msg[index++];
				} else
					ctemp = 0x00;

				W[t] = W[t] * 256 + ctemp;
			}
		} // end for

		if (hasAdd1)
			if ((W[14] == 0x00) && (W[15] == 0x00)) {
				W[15] = message_length;
				for (t = 0; t < 3; ++t) {
					if (W[15] & 0x80000000)
						W[14] = W[14] * 2 + 1;
					W[15] = W[15] * 2;
				}

				hasAddLen = 1;
			}

		A = H[0];
		B = H[1];
		C = H[2];
		D = H[3];
		E = H[4];
		for (t = 0; t < 80; ++t) {
			s = t & MASK;
			if (t >= 16) {
				W[s] = W[(s + 13) & MASK] ^ W[(s + 8) & MASK] ^ W[(s + 2) & MASK] ^ W[s];
				W[s] = func_S(1, W[s]);
			}
			TEMP = func_S(5, A) + func_F(t, B, C, D) + E + W[s] + K[t / 20];
			E = D;
			D = C;
			C = func_S(30, B);
			B = A;
			A = TEMP;
		}

		H[0] += A;
		H[1] += B;
		H[2] += C;
		H[3] += D;
		H[4] += E;
	} // end while

	md[0] = (unsigned char)((H[0] >> 24) & 0xFF);
	md[1] = (unsigned char)((H[0] >> 16) & 0xFF);
	md[2] = (unsigned char)((H[0] >> 8) & 0xFF);
	md[3] = (unsigned char)((H[0]) & 0xFF);
	md[4 + 0] = (unsigned char)((H[1] >> 24) & 0xFF);
	md[4 + 1] = (unsigned char)((H[1] >> 16) & 0xFF);
	md[4 + 2] = (unsigned char)((H[1] >> 8) & 0xFF);
	md[4 + 3] = (unsigned char)((H[1]) & 0xFF);
	md[8 + 0] = (unsigned char)((H[2] >> 24) & 0xFF);
	md[8 + 1] = (unsigned char)((H[2] >> 16) & 0xFF);
	md[8 + 2] = (unsigned char)((H[2] >> 8) & 0xFF);
	md[8 + 3] = (unsigned char)((H[2]) & 0xFF);
	md[12 + 0] = (unsigned char)((H[3] >> 24) & 0xFF);
	md[12 + 1] = (unsigned char)((H[3] >> 16) & 0xFF);
	md[12 + 2] = (unsigned char)((H[3] >> 8) & 0xFF);
	md[12 + 3] = (unsigned char)((H[3]) & 0xFF);
	md[16 + 0] = (unsigned char)((H[4] >> 24) & 0xFF);
	md[16 + 1] = (unsigned char)((H[4] >> 16) & 0xFF);
	md[16 + 2] = (unsigned char)((H[4] >> 8) & 0xFF);
	md[16 + 3] = (unsigned char)((H[4]) & 0xFF);

	return md;
}

// ===========================================================================
// DEPRECATED CODE REMOVED (Phase 1 Cleanup - 2026-01-18)
// The following dead code was removed:
// - encrypt_B() and helpers (rol_byte_off, ror_byte_off, xor_byte_off)
// - Standalone DES functions (encrypt_A, algo_A, base_A, etc.)
// - Old #if 0 dbpswd versions
// These were replaced by C3DES/CDES classes and Botan AES-256.
// ===========================================================================

// ...
// ===========================================================================
// DEPRECATED CODE REMOVED (Phase 2 Cleanup - 2026-01-18)
// Removed:
// - dbpswd_out / dbpswd_in (only used in test UI)
// - KCHAPc / KCHAPs authentication classes (unused)
// - CSHA1, C3DES, CDES classes (unused)
// ===========================================================================

//////////////////////////////////////////////////////////////////////////
//
// Message Digest 5
//
void md5(char const* msg, unsigned char dig[16]) {
	MD5_CTX context;
	unsigned int len = (unsigned int)strlen(msg);

	MD5Init(&context);
	MD5Update(&context, (unsigned char*)msg, len);
	MD5Final(dig, &context);
}

void md5string(char const* msg, char str[33]) {
	unsigned char dig[16];
	md5(msg, dig);

	char tmp[3];
	str[0] = 0, str[32] = 0;
	for (int i = 0; i < 16; ++i) {
		sprintf(tmp, "%02X", dig[i]);
		strcat(str, tmp);
	}
}

void init_Noise(int nNoise, char szKey[4]) {
	/*szKey[0] = char((nNoise&0xFF)^1);
	szKey[1] = char(((nNoise&0xFF00)>>8)^2);
	szKey[2] = char(((nNoise&0xFF0000)>>16)^3);
	szKey[3] = char(((nNoise&0xFF000000)>>24)^4);*/

	szKey[0] = (char)(nNoise & 0x01);
	szKey[1] = (char)(nNoise & 0x02);
	szKey[2] = (char)(nNoise & 0x04);
	szKey[3] = (char)(nNoise & 0x08);
}

bool encrypt_Noise(char szKey[4], char* src, unsigned int src_len) {
	int nLen = src_len >> 2;
	if (nLen > 8) {
		nLen = 8;
	}
	int nCount = 0;
	for (int i = 0; i < nLen; i++) {
		src[nCount++] ^= szKey[3];
		src[nCount++] ^= szKey[2];
		src[nCount++] ^= szKey[1];
		src[nCount++] ^= szKey[0];
	}

	if (src_len >= 8) {
		szKey[0] = src[7] ^ (src[3] ^ 1);
		szKey[1] = src[6] ^ (src[2] ^ 2);
		szKey[2] = src[5] ^ (src[1] ^ 3);
		szKey[3] = src[4] ^ (src[0] ^ 4);
	}

	return true;
}

bool decrypt_Noise(char szKey[4], char* src, unsigned int src_len) {
	char szTemp[4];
	if (src_len >= 8) {
		szTemp[0] = src[7] ^ (src[3] ^ 1);
		szTemp[1] = src[6] ^ (src[2] ^ 2);
		szTemp[2] = src[5] ^ (src[1] ^ 3);
		szTemp[3] = src[4] ^ (src[0] ^ 4);
	}

	int nLen = src_len >> 2;
	if (nLen > 8) {
		nLen = 8;
	}
	int nCount = 0;
	for (int i = 0; i < nLen; i++) {
		src[nCount++] ^= szKey[3];
		src[nCount++] ^= szKey[2];
		src[nCount++] ^= szKey[1];
		src[nCount++] ^= szKey[0];
	}

	if (src_len >= 8) {
		szKey[0] = szTemp[0];
		szKey[1] = szTemp[1];
		szKey[2] = szTemp[2];
		szKey[3] = szTemp[3];
	}

	return true;
}
