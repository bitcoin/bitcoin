#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "num.h"
#include "field.h"

void static secp256k1_fe_normalize(secp256k1_fe_t *r) {
#if (GMP_NUMB_BITS >= 40)
    if (r->n[FIELD_LIMBS] == 0)
        return;
    mp_limb_t carry = mpn_add_1(r->n, r->n, FIELD_LIMBS, 0x1000003D1ULL * r->n[FIELD_LIMBS]);
    mpn_add_1(r->n, r->n, FIELD_LIMBS, 0x1000003D1ULL * carry);
    r->n[FIELD_LIMBS] = 0;
    int overflow = 1;
    for (int i=FIELD_LIMBS-1; i>0; i--)
        overflow &= (r->n[i] == GMP_NUMB_MASK);
    overflow &= (r->n[0] >= GMP_NUMB_MASK - 0x1000003D0ULL);
    if (overflow) {
        for (int i=FIELD_LIMBS-1; i>0; i--)
            r->n[i] = 0;
        r->n[0] -= (GMP_NUMB_MASK - 0x1000003D0ULL);
    }
#else
#    error "GMP_NUMB_BITS too low"
#endif
}

void static inline secp256k1_fe_set_int(secp256k1_fe_t *r, int a) {
    r->n[0] = a;
    for (int i=1; i<FIELD_LIMBS+1; i++)
        r->n[i] = 0;
}

// TODO: not constant time!
int static inline secp256k1_fe_is_zero(const secp256k1_fe_t *a) {
    int ret = 1;
    for (int i=0; i<FIELD_LIMBS+1; i++)
        ret &= (a->n[i] == 0);
    return ret;
}

int static inline secp256k1_fe_is_odd(const secp256k1_fe_t *a) {
    return a->n[0] & 1;
}

// TODO: not constant time!
int static inline secp256k1_fe_equal(const secp256k1_fe_t *a, const secp256k1_fe_t *b) {
    int ret = 1;
    for (int i=0; i<FIELD_LIMBS+1; i++)
        ret &= (a->n[i] == b->n[i]);
    return ret;
}

void static secp256k1_fe_set_b32(secp256k1_fe_t *r, const unsigned char *a) {
    for (int i=0; i<FIELD_LIMBS+1; i++)
        r->n[i] = 0;
    for (int i=0; i<256; i++) {
        int limb = i/GMP_NUMB_BITS;
        int shift = i%GMP_NUMB_BITS;
        r->n[limb] |= (mp_limb_t)((a[31-i/8] >> (i%8)) & 0x1) << shift;
    }
}

/** Convert a field element to a 32-byte big endian value. Requires the input to be normalized */
void static secp256k1_fe_get_b32(unsigned char *r, const secp256k1_fe_t *a) {
    for (int i=0; i<32; i++) {
        int c = 0;
        for (int j=0; j<8; j++) {
            int limb = (8*i+j)/GMP_NUMB_BITS;
            int shift = (8*i+j)%GMP_NUMB_BITS;
            c |= ((a->n[limb] >> shift) & 0x1) << j;
        }
        r[31-i] = c;
    }
}

void static inline secp256k1_fe_negate(secp256k1_fe_t *r, const secp256k1_fe_t *a, int m) {
    *r = *a;
    secp256k1_fe_normalize(r);
    for (int i=0; i<FIELD_LIMBS; i++)
        r->n[i] = ~(r->n[i]);
#if (GMP_NUMB_BITS >= 33)
    mpn_sub_1(r->n, r->n, FIELD_LIMBS, 0x1000003D0ULL);
#else
    mpn_sub_1(r->n, r->n, FIELD_LIMBS, 0x3D0UL);
    mpn_sub_1(r->n+1, r->n+1, FIELD_LIMBS-1, 0x1);
#endif
}

void static inline secp256k1_fe_mul_int(secp256k1_fe_t *r, int a) {
    mpn_mul_1(r->n, r->n, FIELD_LIMBS+1, a);
}

void static inline secp256k1_fe_add(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
    mpn_add(r->n, r->n, FIELD_LIMBS+1, a->n, FIELD_LIMBS+1);
}

void static secp256k1_fe_reduce(secp256k1_fe_t *r, mp_limb_t *tmp) {
    mp_limb_t o = mpn_addmul_1(tmp, tmp+FIELD_LIMBS, FIELD_LIMBS, 0x1000003D1);
    mp_limb_t q[2];
    q[1] = mpn_mul_1(q, &o, 1, 0x1000003D1);
    r->n[FIELD_LIMBS] = mpn_add(r->n, tmp, FIELD_LIMBS, q, 2);
}

void static secp256k1_fe_mul(secp256k1_fe_t *r, const secp256k1_fe_t *a, const secp256k1_fe_t *b) {
    secp256k1_fe_t ac = *a;
    secp256k1_fe_t bc = *b;
    secp256k1_fe_normalize(&ac);
    secp256k1_fe_normalize(&bc);
    mp_limb_t tmp[2*FIELD_LIMBS];
    mpn_mul_n(tmp, ac.n, bc.n, FIELD_LIMBS);
    secp256k1_fe_reduce(r, tmp);
}

void static secp256k1_fe_sqr(secp256k1_fe_t *r, const secp256k1_fe_t *a) {
    secp256k1_fe_t ac = *a;
    secp256k1_fe_normalize(&ac);
    mp_limb_t tmp[2*FIELD_LIMBS];
    mpn_sqr(tmp, ac.n, FIELD_LIMBS);
    secp256k1_fe_reduce(r, tmp);
}
