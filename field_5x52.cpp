#include <assert.h>
#include <string.h>
#include "num.h"
#include "field.h"

#ifdef INLINE_ASM
#include "lin64.h"
#endif

/** Implements arithmetic modulo FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE FFFFFC2F,
 *  represented as 5 uint64_t's in base 2^52. The values are allowed to contain >52 each. In particular,
 *  each FieldElem has a 'magnitude' associated with it. Internally, a magnitude M means each element
 *  is at most M*(2^53-1), except the most significant one, which is limited to M*(2^49-1). All operations
 *  accept any input with magnitude at most M, and have different rules for propagating magnitude to their
 *  output.
 */

void static secp256k1_fe_normalize(secp256k1_fe_t *r) {
    uint64_t c;
    c = r->n[0];
    uint64_t t0 = c & 0xFFFFFFFFFFFFFULL;
    c = (c >> 52) + r->n[1];
    uint64_t t1 = c & 0xFFFFFFFFFFFFFULL;
    c = (c >> 52) + r->n[2];
    uint64_t t2 = c & 0xFFFFFFFFFFFFFULL;
    c = (c >> 52) + r->n[3];
    uint64_t t3 = c & 0xFFFFFFFFFFFFFULL;
    c = (c >> 52) + r->n[4];
    uint64_t t4 = c & 0x0FFFFFFFFFFFFULL;
    c >>= 48;

    // The following code will not modify the t's if c is initially 0.
    c = c * 0x1000003D1ULL + t0;
    t0 = c & 0xFFFFFFFFFFFFFULL;
    c = (c >> 52) + t1;
    t1 = c & 0xFFFFFFFFFFFFFULL;
    c = (c >> 52) + t2;
    t2 = c & 0xFFFFFFFFFFFFFULL;
    c = (c >> 52) + t3;
    t3 = c & 0xFFFFFFFFFFFFFULL;
    c = (c >> 52) + t4;
    t4 = c & 0x0FFFFFFFFFFFFULL;

    // Subtract p if result >= p
    uint64_t mask = -(int64_t)((t4 < 0xFFFFFFFFFFFFULL) | (t3 < 0xFFFFFFFFFFFFFULL) | (t2 < 0xFFFFFFFFFFFFFULL) | (t1 < 0xFFFFFFFFFFFFFULL) | (t0 < 0xFFFFEFFFFFC2FULL));
    t4 &= mask;
    t3 &= mask;
    t2 &= mask;
    t1 &= mask;
    t0 -= (~mask & 0xFFFFEFFFFFC2FULL);

    // push internal variables back
    r->n[0] = t0; r->n[1] = t1; r->n[2] = t2; r->n[3] = t3; r->n[4] = t4;

#ifdef VERIFY
    r->magnitude = 1;
    r->normalized = 1;
#endif
}

void static secp256k1_fe_set_int(secp256k1_fe_t *r, int a) {
    r->n[0] = a;
    r->n[1] = r->n[2] = r->n[3] = r->n[4] = 0;
#ifdef VERIFY
    r->magnitude = 1;
    r->normalized = 1;
#endif
}

// TODO: not constant time!
int static secp256k1_fe_is_zero(const secp256k1_fe_t *a) {
#ifdef VERIFY
    assert(a->normalized);
#endif
    return (a->n[0] == 0 && a->n[1] == 0 && a->n[2] == 0 && a->n[3] == 0 && a->n[4] == 0);
}

int static secp256k1_fe_is_odd(const secp256k1_fe_t *a) {
#ifdef VERIFY
    assert(a->normalized);
#endif
    return a->n[0] & 1;
}

// TODO: not constant time!
int static secp256k1_fe_equal(const secp256k1_fe_t *a, const secp256k1_fe_t *b) {
#ifdef VERIFY
    assert(a->normalized);
    assert(b->normalized);
#endif
    return (a->n[0] == b->n[0] && a->n[1] == b->n[1] && a->n[2] == b->n[2] && a->n[3] == b->n[3] && a->n[4] == b->n[4]);
}

void static secp256k1_fe_set_b32(secp256k1_fe_t *r, const unsigned char *a) {
    r->n[0] = r->n[1] = r->n[2] = r->n[3] = r->n[4] = 0;
    for (int i=0; i<32; i++) {
        for (int j=0; j<2; j++) {
            int limb = (8*i+4*j)/52;
            int shift = (8*i+4*j)%52;
            r->n[limb] |= (uint64_t)((a[31-i] >> (4*j)) & 0xF) << shift;
        }
    }
#ifdef VERIFY
    r->magnitude = 1;
    r->normalized = true;
#endif
}

/** Convert a field element to a 32-byte big endian value. Requires the input to be normalized */
void static secp256k1_fe_get_b32(unsigned char *r, const secp256k1_fe_t *a) {
#ifdef VERIFY
    assert(a->normalized);
#endif
    for (int i=0; i<32; i++) {
        int c = 0;
        for (int j=0; j<2; j++) {
            int limb = (8*i+4*j)/52;
            int shift = (8*i+4*j)%52;
            c |= ((a->n[limb] >> shift) & 0xF) << (4 * j);
        }
        r[31-i] = c;
    }
}

void static secp256k1_fe_negate(secp256k1_fe_t *r, const secp256k1_fe_t *a, int m) {
#ifdef VERIFY
    assert(a->magnitude <= m);
    r->magnitude = m + 1;
    r->normalized = 0;
#endif
    r->n[0] = 0xFFFFEFFFFFC2FULL * (m + 1) - a->n[0];
    r->n[1] = 0xFFFFFFFFFFFFFULL * (m + 1) - a->n[1];
    r->n[2] = 0xFFFFFFFFFFFFFULL * (m + 1) - a->n[2];
    r->n[3] = 0xFFFFFFFFFFFFFULL * (m + 1) - a->n[3];
    r->n[4] = 0x0FFFFFFFFFFFFULL * (m + 1) - a->n[4];
}

void static secp256k1_fe_mul_int(secp256k1_fe_t *r, int a) {
#ifdef VERIFY
    r->magnitude *= a;
    r->normalized = false;
#endif
    r->n[0] *= a;
    r->n[1] *= a;
    r->n[2] *= a;
    r->n[3] *= a;
    r->n[4] *= a;
}

void static secp256k1_fe_add(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
#ifdef VERIFY
    r->magnitude += a->magnitude;
    r->normalized = 0;
#endif
    r->n[0] += a->n[0];
    r->n[1] += a->n[1];
    r->n[2] += a->n[2];
    r->n[3] += a->n[3];
    r->n[4] += a->n[4];
}

void static secp256k1_fe_mul(secp256k1_fe_t *r, const secp256k1_fe_t *a, const secp256k1_fe_t *b) {
#ifdef VERIFY
    assert(a->magnitude <= 8);
    assert(b->magnitude <= 8);
#endif

#ifdef INLINE_ASM
    ExSetMult((uint64_t*)a->n, (uint64_t*)b->n, (uint64_t*)r->n);
#else
    unsigned __int128 c = (__int128)a->n[0] * b->n[0];
    uint64_t t0 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0FFFFFFFFFFFFFE0
    c = c + (__int128)a->n[0] * b->n[1] +
            (__int128)a->n[1] * b->n[0];
    uint64_t t1 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 20000000000000BF
    c = c + (__int128)a->n[0] * b->n[2] +
            (__int128)a->n[1] * b->n[1] +
            (__int128)a->n[2] * b->n[0];
    uint64_t t2 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 30000000000001A0
    c = c + (__int128)a->n[0] * b->n[3] +
            (__int128)a->n[1] * b->n[2] +
            (__int128)a->n[2] * b->n[1] +
            (__int128)a->n[3] * b->n[0];
    uint64_t t3 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 4000000000000280
    c = c + (__int128)a->n[0] * b->n[4] +
            (__int128)a->n[1] * b->n[3] +
            (__int128)a->n[2] * b->n[2] +
            (__int128)a->n[3] * b->n[1] +
            (__int128)a->n[4] * b->n[0];
    uint64_t t4 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 320000000000037E
    c = c + (__int128)a->n[1] * b->n[4] +
            (__int128)a->n[2] * b->n[3] +
            (__int128)a->n[3] * b->n[2] +
            (__int128)a->n[4] * b->n[1];
    uint64_t t5 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 22000000000002BE
    c = c + (__int128)a->n[2] * b->n[4] +
            (__int128)a->n[3] * b->n[3] +
            (__int128)a->n[4] * b->n[2];
    uint64_t t6 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 12000000000001DE
    c = c + (__int128)a->n[3] * b->n[4] +
            (__int128)a->n[4] * b->n[3];
    uint64_t t7 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 02000000000000FE
    c = c + (__int128)a->n[4] * b->n[4];
    uint64_t t8 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 001000000000001E
    uint64_t t9 = c;

    c = t0 + (__int128)t5 * 0x1000003D10ULL;
    t0 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
    c = c + t1 + (__int128)t6 * 0x1000003D10ULL;
    t1 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
    c = c + t2 + (__int128)t7 * 0x1000003D10ULL;
    r->n[2] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
    c = c + t3 + (__int128)t8 * 0x1000003D10ULL;
    r->n[3] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
    c = c + t4 + (__int128)t9 * 0x1000003D10ULL;
    r->n[4] = c & 0x0FFFFFFFFFFFFULL; c = c >> 48; // c max 000001000003D110
    c = t0 + (__int128)c * 0x1000003D1ULL;
    r->n[0] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 1000008
    r->n[1] = t1 + c;
#endif
#ifdef VERIFY
    r->magnitude = 1;
    r->normalized = 0;
#endif
}

void static secp256k1_fe_sqr(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
#ifdef VERIFY
    assert(a->magnitude <= 8);
#endif

#ifdef INLINE_ASM
    ExSetSquare((uint64_t*)&a->n, (uint64_t*)&r->n);
#else
    __int128 c = (__int128)a->n[0] * a->n[0];
    uint64_t t0 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0FFFFFFFFFFFFFE0
    c = c + (__int128)(a->n[0]*2) * a->n[1];
    uint64_t t1 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 20000000000000BF
    c = c + (__int128)(a->n[0]*2) * a->n[2] +
            (__int128)a->n[1] * a->n[1];
    uint64_t t2 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 30000000000001A0
    c = c + (__int128)(a->n[0]*2) * a->n[3] +
            (__int128)(a->n[1]*2) * a->n[2];
    uint64_t t3 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 4000000000000280
    c = c + (__int128)(a->n[0]*2) * a->n[4] +
            (__int128)(a->n[1]*2) * a->n[3] +
            (__int128)a->n[2] * a->n[2];
    uint64_t t4 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 320000000000037E
    c = c + (__int128)(a->n[1]*2) * a->n[4] +
            (__int128)(a->n[2]*2) * a->n[3];
    uint64_t t5 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 22000000000002BE
    c = c + (__int128)(a->n[2]*2) * a->n[4] +
            (__int128)a->n[3] * a->n[3];
    uint64_t t6 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 12000000000001DE
    c = c + (__int128)(a->n[3]*2) * a->n[4];
    uint64_t t7 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 02000000000000FE
    c = c + (__int128)a->n[4] * a->n[4];
    uint64_t t8 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 001000000000001E
    uint64_t t9 = c;
    c = t0 + (__int128)t5 * 0x1000003D10ULL;
    t0 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
    c = c + t1 + (__int128)t6 * 0x1000003D10ULL;
    t1 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
    c = c + t2 + (__int128)t7 * 0x1000003D10ULL;
    r->n[2] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
    c = c + t3 + (__int128)t8 * 0x1000003D10ULL;
    r->n[3] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
    c = c + t4 + (__int128)t9 * 0x1000003D10ULL;
    r->n[4] = c & 0x0FFFFFFFFFFFFULL; c = c >> 48; // c max 000001000003D110
    c = t0 + (__int128)c * 0x1000003D1ULL;
    r->n[0] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 1000008
    r->n[1] = t1 + c;
#endif

#ifdef VERIFY
    assert(a->magnitude <= 8);
    a->normalized = 0;
#endif
}
