// mbstring.h - Linux compatibility shim for MSVC <mbstring.h>
// Provides MBCS (Multi-Byte Character Set) function stubs
#pragma once
#ifndef _MBSTRING_H_LINUX_COMPAT
#define _MBSTRING_H_LINUX_COMPAT

#include <cstring>
#include <cctype>
#include <cstddef>

// _ismbslead: returns non-zero if byte at pos is a lead byte of a multi-byte char
// Simplified: treats bytes >= 0x80 as potential lead bytes (works for DBCS/UTF-8)
inline int _ismbslead(const unsigned char* /*str*/, const unsigned char* pos) {
    return (*pos >= 0x80) ? 1 : 0;
}

// _ismbblead: same but for a single byte value
inline int _ismbblead(unsigned int c) {
    return (c >= 0x80) ? 1 : 0;
}

// _mbslen: get length of MBCS string (simplified - counts bytes like strlen)
inline size_t _mbslen(const unsigned char* str) {
    return strlen((const char*)str);
}

inline unsigned char* _mbschr(const unsigned char* str, unsigned int c) {
    return (unsigned char*)strchr((const char*)str, (int)c);
}

inline unsigned char* _mbsrchr(const unsigned char* str, unsigned int c) {
    return (unsigned char*)strrchr((const char*)str, (int)c);
}

inline int _mbscmp(const unsigned char* str1, const unsigned char* str2) {
    return strcmp((const char*)str1, (const char*)str2);
}

inline int _mbsicmp(const unsigned char* str1, const unsigned char* str2) {
    return strcasecmp((const char*)str1, (const char*)str2);
}

inline unsigned char* _mbslwr(unsigned char* str) {
    for (unsigned char* p = str; *p; ++p) *p = (unsigned char)tolower(*p);
    return str;
}

#endif // _MBSTRING_H_LINUX_COMPAT
