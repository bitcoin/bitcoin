/**********************************************************************
 * Copyright (c) 2013-2015 Pieter Wuille, Gregory Maxwell             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_UTIL_H
#define SECP256K1_UTIL_H

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    void (*fn)(const char *text, void* data);
    const void* data;
} secp256k1_callback;

static SECP256K1_INLINE void secp256k1_callback_call(const secp256k1_callback * const cb, const char * const text) {
    cb->fn(text, (void*)cb->data);
}

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

#ifdef HAVE_BUILTIN_EXPECT
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

/* Like assert(), but when VERIFY is defined, and side-effect safe. */
#if defined(COVERAGE)
#define VERIFY_CHECK(check)
#define VERIFY_SETUP(stmt)
#elif defined(VERIFY)
#define VERIFY_CHECK CHECK
#define VERIFY_SETUP(stmt) do { stmt; } while(0)
#else
#define VERIFY_CHECK(cond) do { (void)(cond); } while(0)
#define VERIFY_SETUP(stmt)
#endif

/* Define `VG_UNDEF` and `VG_CHECK` when VALGRIND is defined  */
#if !defined(VG_CHECK)
# if defined(VALGRIND)
#  include <valgrind/memcheck.h>
#  define VG_UNDEF(x,y) VALGRIND_MAKE_MEM_UNDEFINED((x),(y))
#  define VG_CHECK(x,y) VALGRIND_CHECK_MEM_IS_DEFINED((x),(y))
# else
#  define VG_UNDEF(x,y)
#  define VG_CHECK(x,y)
# endif
#endif

/* Like `VG_CHECK` but on VERIFY only */
#if defined(VERIFY)
#define VG_CHECK_VERIFY(x, y) VG_CHECK((x), (y))
#else
#define VG_CHECK_VERIFY(x, y)
#endif

static SECP256K1_INLINE void *checked_malloc(const secp256k1_callback* cb, size_t size) {
    void *ret = malloc(size);
    if (ret == NULL) {
        secp256k1_callback_call(cb, "Out of memory");
    }
    return ret;
}

static SECP256K1_INLINE void *checked_realloc(const secp256k1_callback* cb, void *ptr, size_t size) {
    void *ret = realloc(ptr, size);
    if (ret == NULL) {
        secp256k1_callback_call(cb, "Out of memory");
    }
    return ret;
}

/* Extract the sign of an int64, take the abs and return a uint64, constant time. */
SECP256K1_INLINE static int secp256k1_sign_and_abs64(uint64_t *out, int64_t in) {
    uint64_t mask0, mask1;
    int ret;
    ret = in < 0;
    mask0 = ret + ~((uint64_t)0);
    mask1 = ~mask0;
    *out = (uint64_t)in;
    *out = (*out & mask0) | ((~*out + 1) & mask1);
    return ret;
}

SECP256K1_INLINE static int secp256k1_clz64_var(uint64_t x) {
    int ret;
    if (!x) {
        return 64;
    }
# if defined(HAVE_BUILTIN_CLZLL)
    ret = __builtin_clzll(x);
# else
    /*FIXME: debruijn fallback. */
    for (ret = 0; ((x & (1ULL << 63)) == 0); x <<= 1, ret++);
# endif
    return ret;
}

/* Zero memory if flag == 1. Flag must be 0 or 1. Constant time. */
static SECP256K1_INLINE void memczero(void* s, size_t len, int flag)
{
    unsigned char* p = (unsigned char*)s;
    /* Access flag with a volatile-qualified lvalue.
       This prevents clang from figuring out (after inlining) that flag can
       take only be 0 or 1, which leads to variable time code. */
    volatile int vflag = flag;
    unsigned char mask = -(unsigned char)vflag;
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
static SECP256K1_INLINE int secp256k1_memcmp_var(const void* s1, const void* s2, size_t n)
{
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
static SECP256K1_INLINE void secp256k1_int_cmov(int* r, const int* a, int flag)
{
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

#if defined(HAVE___INT128)
# if defined(__GNUC__)
#  define SECP256K1_GNUC_EXT __extension__
# else
#  define SECP256K1_GNUC_EXT
# endif
SECP256K1_GNUC_EXT typedef unsigned __int128 uint128_t;
#endif

#endif /* SECP256K1_UTIL_H */
