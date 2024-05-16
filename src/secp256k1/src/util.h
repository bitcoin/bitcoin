/***********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_UTIL_H
#define SECP256K1_UTIL_H

#include "../include/secp256k1.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>

#define STR_(x) #x
#define STR(x) STR_(x)
#define DEBUG_CONFIG_MSG(x) "DEBUG_CONFIG: " x
#define DEBUG_CONFIG_DEF(x) DEBUG_CONFIG_MSG(#x "=" STR(x))

/* Debug helper for printing arrays of unsigned char. */
#define PRINT_BUF(buf, len) do { \
    printf("%s[%lu] = ", #buf, (unsigned long)len); \
    print_buf_plain(buf, len); \
} while(0)

static void print_buf_plain(const unsigned char *buf, size_t len) {
    size_t i;
    printf("{");
    for (i = 0; i < len; i++) {
        if (i % 8 == 0) {
            printf("\n    ");
        } else {
            printf(" ");
        }
        printf("0x%02X,", buf[i]);
    }
    printf("\n}\n");
}

# if (!defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L) )
#  if SECP256K1_GNUC_PREREQ(2,7)
#   define SECP256K1_INLINE __inline__
#  elif (defined(_MSC_VER))
#   define SECP256K1_INLINE __inline
#  else
#   define SECP256K1_INLINE
#  endif
# else
#  define SECP256K1_INLINE inline
# endif

/** Assert statically that expr is true.
 *
 * This is a statement-like macro and can only be used inside functions.
 */
#define STATIC_ASSERT(expr) do { \
    switch(0) { \
        case 0: \
        /* If expr evaluates to 0, we have two case labels "0", which is illegal. */ \
        case /* ERROR: static assertion failed */ (expr): \
        ; \
    } \
} while(0)

/** Assert statically that expr is an integer constant expression, and run stmt.
 *
 * Useful for example to enforce that magnitude arguments are constant.
 */
#define ASSERT_INT_CONST_AND_DO(expr, stmt) do { \
    switch(42) { \
        /* C allows only integer constant expressions as case labels. */ \
        case /* ERROR: integer argument is not constant */ (expr): \
            break; \
        default: ; \
    } \
    stmt; \
} while(0)

typedef struct {
    void (*fn)(const char *text, void* data);
    const void* data;
} secp256k1_callback;

static SECP256K1_INLINE void secp256k1_callback_call(const secp256k1_callback * const cb, const char * const text) {
    cb->fn(text, (void*)cb->data);
}

#ifndef USE_EXTERNAL_DEFAULT_CALLBACKS
static void secp256k1_default_illegal_callback_fn(const char* str, void* data) {
    (void)data;
    fprintf(stderr, "[libsecp256k1] illegal argument: %s\n", str);
    abort();
}
static void secp256k1_default_error_callback_fn(const char* str, void* data) {
    (void)data;
    fprintf(stderr, "[libsecp256k1] internal consistency check failed: %s\n", str);
    abort();
}
#else
void secp256k1_default_illegal_callback_fn(const char* str, void* data);
void secp256k1_default_error_callback_fn(const char* str, void* data);
#endif

static const secp256k1_callback default_illegal_callback = {
    secp256k1_default_illegal_callback_fn,
    NULL
};

static const secp256k1_callback default_error_callback = {
    secp256k1_default_error_callback_fn,
    NULL
};


#ifdef DETERMINISTIC
#define TEST_FAILURE(msg) do { \
    fprintf(stderr, "%s\n", msg); \
    abort(); \
} while(0);
#else
#define TEST_FAILURE(msg) do { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, msg); \
    abort(); \
} while(0)
#endif

#if SECP256K1_GNUC_PREREQ(3, 0)
#define EXPECT(x,c) __builtin_expect((x),(c))
#else
#define EXPECT(x,c) (x)
#endif

#ifdef DETERMINISTIC
#define CHECK(cond) do { \
    if (EXPECT(!(cond), 0)) { \
        TEST_FAILURE("test condition failed"); \
    } \
} while(0)
#else
#define CHECK(cond) do { \
    if (EXPECT(!(cond), 0)) { \
        TEST_FAILURE("test condition failed: " #cond); \
    } \
} while(0)
#endif

/* Like assert(), but when VERIFY is defined. */
#if defined(VERIFY)
#define VERIFY_CHECK CHECK
#else
#define VERIFY_CHECK(cond)
#endif

static SECP256K1_INLINE void *checked_malloc(const secp256k1_callback* cb, size_t size) {
    void *ret = malloc(size);
    if (ret == NULL) {
        secp256k1_callback_call(cb, "Out of memory");
    }
    return ret;
}

#if defined(__BIGGEST_ALIGNMENT__)
#define ALIGNMENT __BIGGEST_ALIGNMENT__
#else
/* Using 16 bytes alignment because common architectures never have alignment
 * requirements above 8 for any of the types we care about. In addition we
 * leave some room because currently we don't care about a few bytes. */
#define ALIGNMENT 16
#endif

/* ceil(x/y) for integers x > 0 and y > 0. Here, / denotes rational division. */
#define CEIL_DIV(x, y) (1 + ((x) - 1) / (y))

#define ROUND_TO_ALIGN(size) (CEIL_DIV(size, ALIGNMENT) * ALIGNMENT)

/* Macro for restrict, when available and not in a VERIFY build. */
#if defined(SECP256K1_BUILD) && defined(VERIFY)
# define SECP256K1_RESTRICT
#else
# if (!defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L) )
#  if SECP256K1_GNUC_PREREQ(3,0)
#   define SECP256K1_RESTRICT __restrict__
#  elif (defined(_MSC_VER) && _MSC_VER >= 1400)
#   define SECP256K1_RESTRICT __restrict
#  else
#   define SECP256K1_RESTRICT
#  endif
# else
#  define SECP256K1_RESTRICT restrict
# endif
#endif

#if defined(_WIN32)
# define I64FORMAT "I64d"
# define I64uFORMAT "I64u"
#else
# define I64FORMAT "lld"
# define I64uFORMAT "llu"
#endif

#if defined(__GNUC__)
# define SECP256K1_GNUC_EXT __extension__
#else
# define SECP256K1_GNUC_EXT
#endif

/* Zero memory if flag == 1. Flag must be 0 or 1. Constant time. */
static SECP256K1_INLINE void secp256k1_memczero(void *s, size_t len, int flag) {
    unsigned char *p = (unsigned char *)s;
    /* Access flag with a volatile-qualified lvalue.
       This prevents clang from figuring out (after inlining) that flag can
       take only be 0 or 1, which leads to variable time code. */
    volatile int vflag = flag;
    unsigned char mask = -(unsigned char) vflag;
    while (len) {
        *p &= ~mask;
        p++;
        len--;
    }
}

/** Semantics like memcmp. Variable-time.
 *
 * We use this to avoid possible compiler bugs with memcmp, e.g.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95189
 */
static SECP256K1_INLINE int secp256k1_memcmp_var(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1, *p2 = s2;
    size_t i;

    for (i = 0; i < n; i++) {
        int diff = p1[i] - p2[i];
        if (diff != 0) {
            return diff;
        }
    }
    return 0;
}

/** If flag is true, set *r equal to *a; otherwise leave it. Constant-time.  Both *r and *a must be initialized and non-negative.*/
static SECP256K1_INLINE void secp256k1_int_cmov(int *r, const int *a, int flag) {
    unsigned int mask0, mask1, r_masked, a_masked;
    /* Access flag with a volatile-qualified lvalue.
       This prevents clang from figuring out (after inlining) that flag can
       take only be 0 or 1, which leads to variable time code. */
    volatile int vflag = flag;

    /* Casting a negative int to unsigned and back to int is implementation defined behavior */
    VERIFY_CHECK(*r >= 0 && *a >= 0);

    mask0 = (unsigned int)vflag + ~0u;
    mask1 = ~mask0;
    r_masked = ((unsigned int)*r & mask0);
    a_masked = ((unsigned int)*a & mask1);

    *r = (int)(r_masked | a_masked);
}

#if defined(USE_FORCE_WIDEMUL_INT128_STRUCT)
/* If USE_FORCE_WIDEMUL_INT128_STRUCT is set, use int128_struct. */
# define SECP256K1_WIDEMUL_INT128 1
# define SECP256K1_INT128_STRUCT 1
#elif defined(USE_FORCE_WIDEMUL_INT128)
/* If USE_FORCE_WIDEMUL_INT128 is set, use int128. */
# define SECP256K1_WIDEMUL_INT128 1
# define SECP256K1_INT128_NATIVE 1
#elif defined(USE_FORCE_WIDEMUL_INT64)
/* If USE_FORCE_WIDEMUL_INT64 is set, use int64. */
# define SECP256K1_WIDEMUL_INT64 1
#elif defined(UINT128_MAX) || defined(__SIZEOF_INT128__)
/* If a native 128-bit integer type exists, use int128. */
# define SECP256K1_WIDEMUL_INT128 1
# define SECP256K1_INT128_NATIVE 1
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
/* On 64-bit MSVC targets (x86_64 and arm64), use int128_struct
 * (which has special logic to implement using intrinsics on those systems). */
# define SECP256K1_WIDEMUL_INT128 1
# define SECP256K1_INT128_STRUCT 1
#elif SIZE_MAX > 0xffffffff
/* Systems with 64-bit pointers (and thus registers) very likely benefit from
 * using 64-bit based arithmetic (even if we need to fall back to 32x32->64 based
 * multiplication logic). */
# define SECP256K1_WIDEMUL_INT128 1
# define SECP256K1_INT128_STRUCT 1
#else
/* Lastly, fall back to int64 based arithmetic. */
# define SECP256K1_WIDEMUL_INT64 1
#endif

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

/* Determine the number of trailing zero bits in a (non-zero) 32-bit x.
 * This function is only intended to be used as fallback for
 * secp256k1_ctz32_var, but permits it to be tested separately. */
static SECP256K1_INLINE int secp256k1_ctz32_var_debruijn(uint32_t x) {
    static const uint8_t debruijn[32] = {
        0x00, 0x01, 0x02, 0x18, 0x03, 0x13, 0x06, 0x19, 0x16, 0x04, 0x14, 0x0A,
        0x10, 0x07, 0x0C, 0x1A, 0x1F, 0x17, 0x12, 0x05, 0x15, 0x09, 0x0F, 0x0B,
        0x1E, 0x11, 0x08, 0x0E, 0x1D, 0x0D, 0x1C, 0x1B
    };
    return debruijn[(uint32_t)((x & -x) * 0x04D7651FU) >> 27];
}

/* Determine the number of trailing zero bits in a (non-zero) 64-bit x.
 * This function is only intended to be used as fallback for
 * secp256k1_ctz64_var, but permits it to be tested separately. */
static SECP256K1_INLINE int secp256k1_ctz64_var_debruijn(uint64_t x) {
    static const uint8_t debruijn[64] = {
        0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28,
        62, 5, 39, 46, 44, 42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11,
        63, 52, 6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
        51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12
    };
    return debruijn[(uint64_t)((x & -x) * 0x022FDD63CC95386DU) >> 58];
}

/* Determine the number of trailing zero bits in a (non-zero) 32-bit x. */
static SECP256K1_INLINE int secp256k1_ctz32_var(uint32_t x) {
    VERIFY_CHECK(x != 0);
#if (__has_builtin(__builtin_ctz) || SECP256K1_GNUC_PREREQ(3,4))
    /* If the unsigned type is sufficient to represent the largest uint32_t, consider __builtin_ctz. */
    if (((unsigned)UINT32_MAX) == UINT32_MAX) {
        return __builtin_ctz(x);
    }
#endif
#if (__has_builtin(__builtin_ctzl) || SECP256K1_GNUC_PREREQ(3,4))
    /* Otherwise consider __builtin_ctzl (the unsigned long type is always at least 32 bits). */
    return __builtin_ctzl(x);
#else
    /* If no suitable CTZ builtin is available, use a (variable time) software emulation. */
    return secp256k1_ctz32_var_debruijn(x);
#endif
}

/* Determine the number of trailing zero bits in a (non-zero) 64-bit x. */
static SECP256K1_INLINE int secp256k1_ctz64_var(uint64_t x) {
    VERIFY_CHECK(x != 0);
#if (__has_builtin(__builtin_ctzl) || SECP256K1_GNUC_PREREQ(3,4))
    /* If the unsigned long type is sufficient to represent the largest uint64_t, consider __builtin_ctzl. */
    if (((unsigned long)UINT64_MAX) == UINT64_MAX) {
        return __builtin_ctzl(x);
    }
#endif
#if (__has_builtin(__builtin_ctzll) || SECP256K1_GNUC_PREREQ(3,4))
    /* Otherwise consider __builtin_ctzll (the unsigned long long type is always at least 64 bits). */
    return __builtin_ctzll(x);
#else
    /* If no suitable CTZ builtin is available, use a (variable time) software emulation. */
    return secp256k1_ctz64_var_debruijn(x);
#endif
}

/* Read a uint32_t in big endian */
SECP256K1_INLINE static uint32_t secp256k1_read_be32(const unsigned char* p) {
    return (uint32_t)p[0] << 24 |
           (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8  |
           (uint32_t)p[3];
}

/* Write a uint32_t in big endian */
SECP256K1_INLINE static void secp256k1_write_be32(unsigned char* p, uint32_t x) {
    p[3] = x;
    p[2] = x >>  8;
    p[1] = x >> 16;
    p[0] = x >> 24;
}

/* Read a uint64_t in big endian */
SECP256K1_INLINE static uint64_t secp256k1_read_be64(const unsigned char* p) {
    return (uint64_t)p[0] << 56 |
           (uint64_t)p[1] << 48 |
           (uint64_t)p[2] << 40 |
           (uint64_t)p[3] << 32 |
           (uint64_t)p[4] << 24 |
           (uint64_t)p[5] << 16 |
           (uint64_t)p[6] << 8  |
           (uint64_t)p[7];
}

/* Write a uint64_t in big endian */
SECP256K1_INLINE static void secp256k1_write_be64(unsigned char* p, uint64_t x) {
    p[7] = x;
    p[6] = x >>  8;
    p[5] = x >> 16;
    p[4] = x >> 24;
    p[3] = x >> 32;
    p[2] = x >> 40;
    p[1] = x >> 48;
    p[0] = x >> 56;
}

/* Rotate a uint32_t to the right. */
SECP256K1_INLINE static uint32_t secp256k1_rotr32(const uint32_t x, const unsigned int by) {
#if defined(_MSC_VER)
    return _rotr(x, by);  /* needs <stdlib.h> */
#else
    /* Reduce rotation amount to avoid UB when shifting. */
    const unsigned int mask = CHAR_BIT * sizeof(x) - 1;
    /* Turned into a rot instruction by GCC and clang. */
    return (x >> (by & mask)) | (x << ((-by) & mask));
#endif
}

#endif /* SECP256K1_UTIL_H */
