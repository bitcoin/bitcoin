/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                               *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/


#ifndef _SECP256K1_ECDSA_IMPL_H_
#define _SECP256K1_ECDSA_IMPL_H_

#include "num.h"
#include "field.h"
#include "group.h"
#include "ecmult.h"
#include "ecmult_gen.h"
#include "ecdsa.h"

static int secp256k1_ecdsa_sig_parse(secp256k1_ecdsa_sig_t *r, const unsigned char *sig, int size) {
    if (sig[0] != 0x30) return 0;
    int lenr = sig[3];
    if (5+lenr >= size) return 0;
    int lens = sig[lenr+5];
    if (sig[1] != lenr+lens+4) return 0;
    if (lenr+lens+6 > size) return 0;
    if (sig[2] != 0x02) return 0;
    if (lenr == 0) return 0;
    if (sig[lenr+4] != 0x02) return 0;
    if (lens == 0) return 0;
    secp256k1_num_set_bin(&r->r, sig+4, lenr);
    secp256k1_num_set_bin(&r->s, sig+6+lenr, lens);
    return 1;
}

static int secp256k1_ecdsa_sig_serialize(unsigned char *sig, int *size, const secp256k1_ecdsa_sig_t *a) {
    int lenR = (secp256k1_num_bits(&a->r) + 7)/8;
    if (lenR == 0 || secp256k1_num_get_bit(&a->r, lenR*8-1))
        lenR++;
    int lenS = (secp256k1_num_bits(&a->s) + 7)/8;
    if (lenS == 0 || secp256k1_num_get_bit(&a->s, lenS*8-1))
        lenS++;
    if (*size < 6+lenS+lenR)
        return 0;
    *size = 6 + lenS + lenR;
    sig[0] = 0x30;
    sig[1] = 4 + lenS + lenR;
    sig[2] = 0x02;
    sig[3] = lenR;
    secp256k1_num_get_bin(sig+4, lenR, &a->r);
    sig[4+lenR] = 0x02;
    sig[5+lenR] = lenS;
    secp256k1_num_get_bin(sig+lenR+6, lenS, &a->s);
    return 1;
}

static int secp256k1_ecdsa_sig_recompute(secp256k1_num_t *r2, const secp256k1_ecdsa_sig_t *sig, const secp256k1_ge_t *pubkey, const secp256k1_num_t *message) {
    const secp256k1_ge_consts_t *c = secp256k1_ge_consts;

    if (secp256k1_num_is_neg(&sig->r) || secp256k1_num_is_neg(&sig->s))
        return 0;
    if (secp256k1_num_is_zero(&sig->r) || secp256k1_num_is_zero(&sig->s))
        return 0;
    if (secp256k1_num_cmp(&sig->r, &c->order) >= 0 || secp256k1_num_cmp(&sig->s, &c->order) >= 0)
        return 0;

    int ret = 0;
    secp256k1_num_t sn, u1, u2;
    secp256k1_num_init(&sn);
    secp256k1_num_init(&u1);
    secp256k1_num_init(&u2);
    secp256k1_num_mod_inverse(&sn, &sig->s, &c->order);
    secp256k1_num_mod_mul(&u1, &sn, message, &c->order);
    secp256k1_num_mod_mul(&u2, &sn, &sig->r, &c->order);
    secp256k1_gej_t pubkeyj; secp256k1_gej_set_ge(&pubkeyj, pubkey);
    secp256k1_gej_t pr; secp256k1_ecmult(&pr, &pubkeyj, &u2, &u1);
    if (!secp256k1_gej_is_infinity(&pr)) {
        secp256k1_fe_t xr; secp256k1_gej_get_x_var(&xr, &pr);
        secp256k1_fe_normalize(&xr);
        unsigned char xrb[32]; secp256k1_fe_get_b32(xrb, &xr);
        secp256k1_num_set_bin(r2, xrb, 32);
        secp256k1_num_mod(r2, &c->order);
        ret = 1;
    }
    secp256k1_num_free(&sn);
    secp256k1_num_free(&u1);
    secp256k1_num_free(&u2);
    return ret;
}

static int secp256k1_ecdsa_sig_recover(const secp256k1_ecdsa_sig_t *sig, secp256k1_ge_t *pubkey, const secp256k1_num_t *message, int recid) {
    const secp256k1_ge_consts_t *c = secp256k1_ge_consts;

    if (secp256k1_num_is_neg(&sig->r) || secp256k1_num_is_neg(&sig->s))
        return 0;
    if (secp256k1_num_is_zero(&sig->r) || secp256k1_num_is_zero(&sig->s))
        return 0;
    if (secp256k1_num_cmp(&sig->r, &c->order) >= 0 || secp256k1_num_cmp(&sig->s, &c->order) >= 0)
        return 0;

    secp256k1_num_t rx;
    secp256k1_num_init(&rx);
    secp256k1_num_copy(&rx, &sig->r);
    if (recid & 2) {
        secp256k1_num_add(&rx, &rx, &c->order);
        if (secp256k1_num_cmp(&rx, &secp256k1_fe_consts->p) >= 0)
            return 0;
    }
    unsigned char brx[32];
    secp256k1_num_get_bin(brx, 32, &rx);
    secp256k1_num_free(&rx);
    secp256k1_fe_t fx;
    secp256k1_fe_set_b32(&fx, brx);
    secp256k1_ge_t x;
    if (!secp256k1_ge_set_xo(&x, &fx, recid & 1))
        return 0;
    secp256k1_gej_t xj;
    secp256k1_gej_set_ge(&xj, &x);
    secp256k1_num_t rn, u1, u2;
    secp256k1_num_init(&rn);
    secp256k1_num_init(&u1);
    secp256k1_num_init(&u2);
    secp256k1_num_mod_inverse(&rn, &sig->r, &c->order);
    secp256k1_num_mod_mul(&u1, &rn, message, &c->order);
    secp256k1_num_sub(&u1, &c->order, &u1);
    secp256k1_num_mod_mul(&u2, &rn, &sig->s, &c->order);
    secp256k1_gej_t qj;
    secp256k1_ecmult(&qj, &xj, &u2, &u1);
    secp256k1_ge_set_gej_var(pubkey, &qj);
    secp256k1_num_free(&rn);
    secp256k1_num_free(&u1);
    secp256k1_num_free(&u2);
    return !secp256k1_gej_is_infinity(&qj);
}

static int secp256k1_ecdsa_sig_verify(const secp256k1_ecdsa_sig_t *sig, const secp256k1_ge_t *pubkey, const secp256k1_num_t *message) {
    secp256k1_num_t r2;
    secp256k1_num_init(&r2);
    int ret = 0;
    ret = secp256k1_ecdsa_sig_recompute(&r2, sig, pubkey, message) && secp256k1_num_cmp(&sig->r, &r2) == 0;
    secp256k1_num_free(&r2);
    return ret;
}

static int secp256k1_ecdsa_sig_sign(secp256k1_ecdsa_sig_t *sig, const secp256k1_scalar_t *seckey, const secp256k1_scalar_t *message, const secp256k1_scalar_t *nonce, int *recid) {
    secp256k1_gej_t rp;
    secp256k1_ecmult_gen(&rp, nonce);
    secp256k1_ge_t r;
    secp256k1_ge_set_gej(&r, &rp);
    unsigned char b[32];
    secp256k1_fe_normalize(&r.x);
    secp256k1_fe_normalize(&r.y);
    secp256k1_fe_get_b32(b, &r.x);
    int overflow = 0;
    secp256k1_scalar_t sigr;
    secp256k1_scalar_set_b32(&sigr, b, &overflow);
    if (recid)
        *recid = (overflow ? 2 : 0) | (secp256k1_fe_is_odd(&r.y) ? 1 : 0);
    secp256k1_scalar_t n;
    secp256k1_scalar_mul(&n, &sigr, seckey);
    secp256k1_scalar_add(&n, &n, message);
    secp256k1_scalar_t sigs;
    secp256k1_scalar_inverse(&sigs, nonce);
    secp256k1_scalar_mul(&sigs, &sigs, &n);
    secp256k1_scalar_clear(&n);
    secp256k1_gej_clear(&rp);
    secp256k1_ge_clear(&r);
    if (secp256k1_scalar_is_zero(&sigs))
        return 0;
    if (secp256k1_scalar_is_high(&sigs)) {
        secp256k1_scalar_negate(&sigs, &sigs);
        if (recid)
            *recid ^= 1;
    }
    secp256k1_scalar_get_num(&sig->s, &sigs);
    secp256k1_scalar_get_num(&sig->r, &sigr);
    return 1;
}

static void secp256k1_ecdsa_sig_set_rs(secp256k1_ecdsa_sig_t *sig, const secp256k1_num_t *r, const secp256k1_num_t *s) {
    secp256k1_num_copy(&sig->r, r);
    secp256k1_num_copy(&sig->s, s);
}

#endif
