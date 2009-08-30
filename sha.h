// This file is public domain
// SHA routines extracted as a standalone file from:
// Crypto++: a C++ Class Library of Cryptographic Schemes
// Version 5.5.2 (9/24/2007)
// http://www.cryptopp.com
#ifndef CRYPTOPP_SHA_H
#define CRYPTOPP_SHA_H
#include <stdlib.h>

namespace CryptoPP
{

//
// Dependencies
//

typedef unsigned char byte;
typedef unsigned short word16;
typedef unsigned int word32;
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 word64;
#else
typedef unsigned long long word64;
#endif

template <class T> inline T rotlFixed(T x, unsigned int y)
{
    assert(y < sizeof(T)*8);
    return T((x<<y) | (x>>(sizeof(T)*8-y)));
}

template <class T> inline T rotrFixed(T x, unsigned int y)
{
    assert(y < sizeof(T)*8);
    return T((x>>y) | (x<<(sizeof(T)*8-y)));
}

// ************** endian reversal ***************

#ifdef _MSC_VER
    #if _MSC_VER >= 1400
        #define CRYPTOPP_FAST_ROTATE(x) 1
    #elif _MSC_VER >= 1300
        #define CRYPTOPP_FAST_ROTATE(x) ((x) == 32 | (x) == 64)
    #else
        #define CRYPTOPP_FAST_ROTATE(x) ((x) == 32)
    #endif
#elif (defined(__MWERKS__) && TARGET_CPU_PPC) || \
    (defined(__GNUC__) && (defined(_ARCH_PWR2) || defined(_ARCH_PWR) || defined(_ARCH_PPC) || defined(_ARCH_PPC64) || defined(_ARCH_COM)))
    #define CRYPTOPP_FAST_ROTATE(x) ((x) == 32)
#elif defined(__GNUC__) && (CRYPTOPP_BOOL_X64 || CRYPTOPP_BOOL_X86) // depend on GCC's peephole optimization to generate rotate instructions
    #define CRYPTOPP_FAST_ROTATE(x) 1
#else
    #define CRYPTOPP_FAST_ROTATE(x) 0
#endif

inline byte ByteReverse(byte value)
{
    return value;
}

inline word16 ByteReverse(word16 value)
{
#ifdef CRYPTOPP_BYTESWAP_AVAILABLE
    return bswap_16(value);
#elif defined(_MSC_VER) && _MSC_VER >= 1300
    return _byteswap_ushort(value);
#else
    return rotlFixed(value, 8U);
#endif
}

inline word32 ByteReverse(word32 value)
{
#if defined(__GNUC__)
    __asm__ ("bswap %0" : "=r" (value) : "0" (value));
    return value;
#elif defined(CRYPTOPP_BYTESWAP_AVAILABLE)
    return bswap_32(value);
#elif defined(__MWERKS__) && TARGET_CPU_PPC
    return (word32)__lwbrx(&value,0);
#elif _MSC_VER >= 1400 || (_MSC_VER >= 1300 && !defined(_DLL))
    return _byteswap_ulong(value);
#elif CRYPTOPP_FAST_ROTATE(32)
    // 5 instructions with rotate instruction, 9 without
    return (rotrFixed(value, 8U) & 0xff00ff00) | (rotlFixed(value, 8U) & 0x00ff00ff);
#else
    // 6 instructions with rotate instruction, 8 without
    value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
    return rotlFixed(value, 16U);
#endif
}

#ifdef WORD64_AVAILABLE
inline word64 ByteReverse(word64 value)
{
#if defined(__GNUC__) && defined(__x86_64__)
    __asm__ ("bswap %0" : "=r" (value) : "0" (value));
    return value;
#elif defined(CRYPTOPP_BYTESWAP_AVAILABLE)
    return bswap_64(value);
#elif defined(_MSC_VER) && _MSC_VER >= 1300
    return _byteswap_uint64(value);
#elif defined(CRYPTOPP_SLOW_WORD64)
    return (word64(ByteReverse(word32(value))) << 32) | ByteReverse(word32(value>>32));
#else
    value = ((value & W64LIT(0xFF00FF00FF00FF00)) >> 8) | ((value & W64LIT(0x00FF00FF00FF00FF)) << 8);
    value = ((value & W64LIT(0xFFFF0000FFFF0000)) >> 16) | ((value & W64LIT(0x0000FFFF0000FFFF)) << 16);
    return rotlFixed(value, 32U);
#endif
}
#endif


//
// SHA
//

// http://www.weidai.com/scan-mirror/md.html#SHA-1
class SHA1
{
public:
    typedef word32 HashWordType;
    static void InitState(word32 *state);
    static void Transform(word32 *digest, const word32 *data);
    static const char * StaticAlgorithmName() {return "SHA-1";}
};

typedef SHA1 SHA;   // for backwards compatibility

// implements the SHA-256 standard
class SHA256
{
public:
    typedef word32 HashWordType;
    static void InitState(word32 *state);
    static void Transform(word32 *digest, const word32 *data);
    static const char * StaticAlgorithmName() {return "SHA-256";}
};

// implements the SHA-224 standard
class SHA224
{
public:
    typedef word32 HashWordType;
    static void InitState(word32 *state);
    static void Transform(word32 *digest, const word32 *data) {SHA256::Transform(digest, data);}
    static const char * StaticAlgorithmName() {return "SHA-224";}
};

#ifdef WORD64_AVAILABLE

// implements the SHA-512 standard
class SHA512
{
public:
    typedef word64 HashWordType;
    static void InitState(word64 *state);
    static void Transform(word64 *digest, const word64 *data);
    static const char * StaticAlgorithmName() {return "SHA-512";}
};

// implements the SHA-384 standard
class SHA384
{
public:
    typedef word64 HashWordType;
    static void InitState(word64 *state);
    static void Transform(word64 *digest, const word64 *data) {SHA512::Transform(digest, data);}
    static const char * StaticAlgorithmName() {return "SHA-384";}
};

#endif

}

#endif
