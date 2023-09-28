#ifndef SECP256K1_INT128_STRUCT_IMPL_H
#define SECP256K1_INT128_STRUCT_IMPL_H

#include "int128.h"

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64)) /* MSVC */
#    include <intrin.h>
#    if defined(_M_ARM64) || defined(SECP256K1_MSVC_MULH_TEST_OVERRIDE)
/* On ARM64 MSVC, use __(u)mulh for the upper half of 64x64 multiplications.
   (Define SECP256K1_MSVC_MULH_TEST_OVERRIDE to test this code path on X64,
   which supports both __(u)mulh and _umul128.) */
#        if defined(SECP256K1_MSVC_MULH_TEST_OVERRIDE)
#            pragma message(__FILE__ ": SECP256K1_MSVC_MULH_TEST_OVERRIDE is defined, forcing use of __(u)mulh.")
#        endif
static SECP256K1_INLINE uint64_t secp256k1_umul128(uint64_t a, uint64_t b, uint64_t* hi) {
    *hi = __umulh(a, b);
    return a * b;
}

static SECP256K1_INLINE int64_t secp256k1_mul128(int64_t a, int64_t b, int64_t* hi) {
    *hi = __mulh(a, b);
    return (uint64_t)a * (uint64_t)b;
}
#    else
/* On x84_64 MSVC, use native _(u)mul128 for 64x64->128 multiplications. */
#        define secp256k1_umul128 _umul128
#        define secp256k1_mul128 _mul128
#    endif
#else
/* On other systems, emulate 64x64->128 multiplications using 32x32->64 multiplications. */
static SECP256K1_INLINE uint64_t secp256k1_umul128(uint64_t a, uint64_t b, uint64_t* hi) {
    uint64_t ll = (uint64_t)(uint32_t)a * (uint32_t)b;
    uint64_t lh = (uint32_t)a * (b >> 32);
    uint64_t hl = (a >> 32) * (uint32_t)b;
    uint64_t hh = (a >> 32) * (b >> 32);
    uint64_t mid34 = (ll >> 32) + (uint32_t)lh + (uint32_t)hl;
    *hi = hh + (lh >> 32) + (hl >> 32) + (mid34 >> 32);
    return (mid34 << 32) + (uint32_t)ll;
}

static SECP256K1_INLINE int64_t secp256k1_mul128(int64_t a, int64_t b, int64_t* hi) {
    uint64_t ll = (uint64_t)(uint32_t)a * (uint32_t)b;
    int64_t lh = (uint32_t)a * (b >> 32);
    int64_t hl = (a >> 32) * (uint32_t)b;
    int64_t hh = (a >> 32) * (b >> 32);
    uint64_t mid34 = (ll >> 32) + (uint32_t)lh + (uint32_t)hl;
    *hi = hh + (lh >> 32) + (hl >> 32) + (mid34 >> 32);
    return (mid34 << 32) + (uint32_t)ll;
}
#endif

static SECP256K1_INLINE void secp256k1_u128_load(secp256k1_uint128 *r, uint64_t hi, uint64_t lo) {
    r->hi = hi;
    r->lo = lo;
}

static SECP256K1_INLINE void secp256k1_u128_mul(secp256k1_uint128 *r, uint64_t a, uint64_t b) {
   r->lo = secp256k1_umul128(a, b, &r->hi);
}

static SECP256K1_INLINE void secp256k1_u128_accum_mul(secp256k1_uint128 *r, uint64_t a, uint64_t b) {
   uint64_t lo, hi;
   lo = secp256k1_umul128(a, b, &hi);
   r->lo += lo;
   r->hi += hi + (r->lo < lo);
}

static SECP256K1_INLINE void secp256k1_u128_accum_u64(secp256k1_uint128 *r, uint64_t a) {
   r->lo += a;
   r->hi += r->lo < a;
}

/* Unsigned (logical) right shift.
 * Non-constant time in n.
 */
static SECP256K1_INLINE void secp256k1_u128_rshift(secp256k1_uint128 *r, unsigned int n) {
   VERIFY_CHECK(n < 128);
   if (n >= 64) {
     r->lo = r->hi >> (n-64);
     r->hi = 0;
   } else if (n > 0) {
     r->lo = ((1U * r->hi) << (64-n)) | r->lo >> n;
     r->hi >>= n;
   }
}

static SECP256K1_INLINE uint64_t secp256k1_u128_to_u64(const secp256k1_uint128 *a) {
   return a->lo;
}

static SECP256K1_INLINE uint64_t secp256k1_u128_hi_u64(const secp256k1_uint128 *a) {
   return a->hi;
}

static SECP256K1_INLINE void secp256k1_u128_from_u64(secp256k1_uint128 *r, uint64_t a) {
   r->hi = 0;
   r->lo = a;
}

static SECP256K1_INLINE int secp256k1_u128_check_bits(const secp256k1_uint128 *r, unsigned int n) {
   VERIFY_CHECK(n < 128);
   return n >= 64 ? r->hi >> (n - 64) == 0
                  : r->hi == 0 && r->lo >> n == 0;
}

static SECP256K1_INLINE void secp256k1_i128_load(secp256k1_int128 *r, int64_t hi, uint64_t lo) {
    r->hi = hi;
    r->lo = lo;
}

static SECP256K1_INLINE void secp256k1_i128_mul(secp256k1_int128 *r, int64_t a, int64_t b) {
   int64_t hi;
   r->lo = (uint64_t)secp256k1_mul128(a, b, &hi);
   r->hi = (uint64_t)hi;
}

static SECP256K1_INLINE void secp256k1_i128_accum_mul(secp256k1_int128 *r, int64_t a, int64_t b) {
   int64_t hi;
   uint64_t lo = (uint64_t)secp256k1_mul128(a, b, &hi);
   r->lo += lo;
   hi += r->lo < lo;
   /* Verify no overflow.
    * If r represents a positive value (the sign bit is not set) and the value we are adding is a positive value (the sign bit is not set),
    * then we require that the resulting value also be positive (the sign bit is not set).
    * Note that (X <= Y) means (X implies Y) when X and Y are boolean values (i.e. 0 or 1).
    */
   VERIFY_CHECK((r->hi <= 0x7fffffffffffffffu && (uint64_t)hi <= 0x7fffffffffffffffu) <= (r->hi + (uint64_t)hi <= 0x7fffffffffffffffu));
   /* Verify no underflow.
    * If r represents a negative value (the sign bit is set) and the value we are adding is a negative value (the sign bit is set),
    * then we require that the resulting value also be negative (the sign bit is set).
    */
   VERIFY_CHECK((r->hi > 0x7fffffffffffffffu && (uint64_t)hi > 0x7fffffffffffffffu) <= (r->hi + (uint64_t)hi > 0x7fffffffffffffffu));
   r->hi += hi;
}

static SECP256K1_INLINE void secp256k1_i128_dissip_mul(secp256k1_int128 *r, int64_t a, int64_t b) {
   int64_t hi;
   uint64_t lo = (uint64_t)secp256k1_mul128(a, b, &hi);
   hi += r->lo < lo;
   /* Verify no overflow.
    * If r represents a positive value (the sign bit is not set) and the value we are subtracting is a negative value (the sign bit is set),
    * then we require that the resulting value also be positive (the sign bit is not set).
    */
   VERIFY_CHECK((r->hi <= 0x7fffffffffffffffu && (uint64_t)hi > 0x7fffffffffffffffu) <= (r->hi - (uint64_t)hi <= 0x7fffffffffffffffu));
   /* Verify no underflow.
    * If r represents a negative value (the sign bit is set) and the value we are subtracting is a positive value (the sign sign bit is not set),
    * then we require that the resulting value also be negative (the sign bit is set).
    */
   VERIFY_CHECK((r->hi > 0x7fffffffffffffffu && (uint64_t)hi <= 0x7fffffffffffffffu) <= (r->hi - (uint64_t)hi > 0x7fffffffffffffffu));
   r->hi -= hi;
   r->lo -= lo;
}

static SECP256K1_INLINE void secp256k1_i128_det(secp256k1_int128 *r, int64_t a, int64_t b, int64_t c, int64_t d) {
   secp256k1_i128_mul(r, a, d);
   secp256k1_i128_dissip_mul(r, b, c);
}

/* Signed (arithmetic) right shift.
 * Non-constant time in n.
 */
static SECP256K1_INLINE void secp256k1_i128_rshift(secp256k1_int128 *r, unsigned int n) {
   VERIFY_CHECK(n < 128);
   if (n >= 64) {
     r->lo = (uint64_t)((int64_t)(r->hi) >> (n-64));
     r->hi = (uint64_t)((int64_t)(r->hi) >> 63);
   } else if (n > 0) {
     r->lo = ((1U * r->hi) << (64-n)) | r->lo >> n;
     r->hi = (uint64_t)((int64_t)(r->hi) >> n);
   }
}

static SECP256K1_INLINE uint64_t secp256k1_i128_to_u64(const secp256k1_int128 *a) {
   return a->lo;
}

static SECP256K1_INLINE int64_t secp256k1_i128_to_i64(const secp256k1_int128 *a) {
   /* Verify that a represents a 64 bit signed value by checking that the high bits are a sign extension of the low bits. */
   VERIFY_CHECK(a->hi == -(a->lo >> 63));
   return (int64_t)secp256k1_i128_to_u64(a);
}

static SECP256K1_INLINE void secp256k1_i128_from_i64(secp256k1_int128 *r, int64_t a) {
   r->hi = (uint64_t)(a >> 63);
   r->lo = (uint64_t)a;
}

static SECP256K1_INLINE int secp256k1_i128_eq_var(const secp256k1_int128 *a, const secp256k1_int128 *b) {
   return a->hi == b->hi && a->lo == b->lo;
}

static SECP256K1_INLINE int secp256k1_i128_check_pow2(const secp256k1_int128 *r, unsigned int n, int sign) {
    VERIFY_CHECK(n < 127);
    VERIFY_CHECK(sign == 1 || sign == -1);
    return n >= 64 ? r->hi == (uint64_t)sign << (n - 64) && r->lo == 0
                   : r->hi == (uint64_t)(sign >> 1) && r->lo == (uint64_t)sign << n;
}

#endif
