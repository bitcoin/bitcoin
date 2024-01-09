#ifndef SECP256K1_INT128_H
#define SECP256K1_INT128_H

#include "util.h"

#if defined(SECP256K1_WIDEMUL_INT128)
#  if defined(SECP256K1_INT128_NATIVE)
#    include "int128_native.h"
#  elif defined(SECP256K1_INT128_STRUCT)
#    include "int128_struct.h"
#  else
#    error "Please select int128 implementation"
#  endif

/* Construct an unsigned 128-bit value from a high and a low 64-bit value. */
static SECP256K1_INLINE void secp256k1_u128_load(secp256k1_uint128 *r, uint64_t hi, uint64_t lo);

/* Multiply two unsigned 64-bit values a and b and write the result to r. */
static SECP256K1_INLINE void secp256k1_u128_mul(secp256k1_uint128 *r, uint64_t a, uint64_t b);

/* Multiply two unsigned 64-bit values a and b and add the result to r.
 * The final result is taken modulo 2^128.
 */
static SECP256K1_INLINE void secp256k1_u128_accum_mul(secp256k1_uint128 *r, uint64_t a, uint64_t b);

/* Add an unsigned 64-bit value a to r.
 * The final result is taken modulo 2^128.
 */
static SECP256K1_INLINE void secp256k1_u128_accum_u64(secp256k1_uint128 *r, uint64_t a);

/* Unsigned (logical) right shift.
 * Non-constant time in n.
 */
static SECP256K1_INLINE void secp256k1_u128_rshift(secp256k1_uint128 *r, unsigned int n);

/* Return the low 64-bits of a 128-bit value as an unsigned 64-bit value. */
static SECP256K1_INLINE uint64_t secp256k1_u128_to_u64(const secp256k1_uint128 *a);

/* Return the high 64-bits of a 128-bit value as an unsigned 64-bit value. */
static SECP256K1_INLINE uint64_t secp256k1_u128_hi_u64(const secp256k1_uint128 *a);

/* Write an unsigned 64-bit value to r. */
static SECP256K1_INLINE void secp256k1_u128_from_u64(secp256k1_uint128 *r, uint64_t a);

/* Tests if r is strictly less than to 2^n.
 * n must be strictly less than 128.
 */
static SECP256K1_INLINE int secp256k1_u128_check_bits(const secp256k1_uint128 *r, unsigned int n);

/* Construct an signed 128-bit value from a high and a low 64-bit value. */
static SECP256K1_INLINE void secp256k1_i128_load(secp256k1_int128 *r, int64_t hi, uint64_t lo);

/* Multiply two signed 64-bit values a and b and write the result to r. */
static SECP256K1_INLINE void secp256k1_i128_mul(secp256k1_int128 *r, int64_t a, int64_t b);

/* Multiply two signed 64-bit values a and b and add the result to r.
 * Overflow or underflow from the addition is undefined behaviour.
 */
static SECP256K1_INLINE void secp256k1_i128_accum_mul(secp256k1_int128 *r, int64_t a, int64_t b);

/* Compute a*d - b*c from signed 64-bit values and write the result to r. */
static SECP256K1_INLINE void secp256k1_i128_det(secp256k1_int128 *r, int64_t a, int64_t b, int64_t c, int64_t d);

/* Signed (arithmetic) right shift.
 * Non-constant time in b.
 */
static SECP256K1_INLINE void secp256k1_i128_rshift(secp256k1_int128 *r, unsigned int b);

/* Return the input value modulo 2^64. */
static SECP256K1_INLINE uint64_t secp256k1_i128_to_u64(const secp256k1_int128 *a);

/* Return the value as a signed 64-bit value.
 * Requires the input to be between INT64_MIN and INT64_MAX.
 */
static SECP256K1_INLINE int64_t secp256k1_i128_to_i64(const secp256k1_int128 *a);

/* Write a signed 64-bit value to r. */
static SECP256K1_INLINE void secp256k1_i128_from_i64(secp256k1_int128 *r, int64_t a);

/* Compare two 128-bit values for equality. */
static SECP256K1_INLINE int secp256k1_i128_eq_var(const secp256k1_int128 *a, const secp256k1_int128 *b);

/* Tests if r is equal to sign*2^n (sign must be 1 or -1).
 * n must be strictly less than 127.
 */
static SECP256K1_INLINE int secp256k1_i128_check_pow2(const secp256k1_int128 *r, unsigned int n, int sign);

#endif

#endif
