// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_ECDSA_IMPL_H_
#define _SECP256K1_ECDSA_IMPL_H_

#include "../num.h"
#include "../field.h"
#include "../group.h"
#include "../ecmult.h"
#include "../ecdsa.h"

void static secp256k1_ecdsa_sig_init(secp256k1_ecdsa_sig_t *r) {
    secp256k1_num_init(&r->r);
    secp256k1_num_init(&r->s);
}

void static secp256k1_ecdsa_sig_free(secp256k1_ecdsa_sig_t *r) {
    secp256k1_num_free(&r->r);
    secp256k1_num_free(&r->s);
}

int static secp256k1_ecdsa_pubkey_parse(secp256k1_ge_t *elem, const unsigned char *pub, int size) {
    if (size == 33 && (pub[0] == 0x02 || pub[0] == 0x03)) {
        secp256k1_fe_t x;
        secp256k1_fe_set_b32(&x, pub+1);
        secp256k1_ge_set_xo(elem, &x, pub[0] == 0x03);
    } else if (size == 65 && (pub[0] == 0x04 || pub[0] == 0x06 || pub[0] == 0x07)) {
        secp256k1_fe_t x, y;
        secp256k1_fe_set_b32(&x, pub+1);
        secp256k1_fe_set_b32(&y, pub+33);
        secp256k1_ge_set_xy(elem, &x, &y);
        if ((pub[0] == 0x06 || pub[0] == 0x07) && secp256k1_fe_is_odd(&y) != (pub[0] == 0x07))
            return 0;
    } else {
        return 0;
    }
    return secp256k1_ge_is_valid(elem);
}

int static secp256k1_ecdsa_sig_parse(secp256k1_ecdsa_sig_t *r, const unsigned char *sig, int size) {
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

int static secp256k1_ecdsa_sig_serialize(unsigned char *sig, int *size, const secp256k1_ecdsa_sig_t *a) {
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

int static secp256k1_ecdsa_sig_recompute(secp256k1_num_t *r2, const secp256k1_ecdsa_sig_t *sig, const secp256k1_ge_t *pubkey, const secp256k1_num_t *message) {
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
        secp256k1_fe_t xr; secp256k1_gej_get_x(&xr, &pr);
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

int static secp256k1_ecdsa_sig_recover(const secp256k1_ecdsa_sig_t *sig, secp256k1_ge_t *pubkey, const secp256k1_num_t *message, int recid) {
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
    if (recid & 2)
        secp256k1_num_add(&rx, &rx, &c->order);
    unsigned char brx[32];
    secp256k1_num_get_bin(brx, 32, &rx);
    secp256k1_num_free(&rx);
    secp256k1_fe_t fx;
    secp256k1_fe_set_b32(&fx, brx);
    secp256k1_ge_t x;
    secp256k1_ge_set_xo(&x, &fx, recid & 1);
    if (!secp256k1_ge_is_valid(&x))
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
    secp256k1_ge_set_gej(pubkey, &qj);
    secp256k1_num_free(&rn);
    secp256k1_num_free(&u1);
    secp256k1_num_free(&u2);
    return 1;
}

int static secp256k1_ecdsa_sig_verify(const secp256k1_ecdsa_sig_t *sig, const secp256k1_ge_t *pubkey, const secp256k1_num_t *message) {
    secp256k1_num_t r2;
    secp256k1_num_init(&r2);
    int ret = 0;
    ret = secp256k1_ecdsa_sig_recompute(&r2, sig, pubkey, message) && secp256k1_num_cmp(&sig->r, &r2) == 0;
    secp256k1_num_free(&r2);
    return ret;
}

int static secp256k1_ecdsa_sig_sign(secp256k1_ecdsa_sig_t *sig, const secp256k1_num_t *seckey, const secp256k1_num_t *message, const secp256k1_num_t *nonce, int *recid) {
    const secp256k1_ge_consts_t *c = secp256k1_ge_consts;

    secp256k1_gej_t rp;
    secp256k1_ecmult_gen(&rp, nonce);
    secp256k1_ge_t r;
    secp256k1_ge_set_gej(&r, &rp);
    unsigned char b[32];
    secp256k1_fe_normalize(&r.x);
    secp256k1_fe_normalize(&r.y);
    secp256k1_fe_get_b32(b, &r.x);
    secp256k1_num_set_bin(&sig->r, b, 32);
    if (recid)
        *recid = (secp256k1_num_cmp(&sig->r, &c->order) >= 0 ? 2 : 0) | (secp256k1_fe_is_odd(&r.y) ? 1 : 0);
    secp256k1_num_mod(&sig->r, &c->order);
    secp256k1_num_t n;
    secp256k1_num_init(&n);
    secp256k1_num_mod_mul(&n, &sig->r, seckey, &c->order);
    secp256k1_num_add(&n, &n, message);
    secp256k1_num_mod(&n, &c->order);
    secp256k1_num_mod_inverse(&sig->s, nonce, &c->order);
    secp256k1_num_mod_mul(&sig->s, &sig->s, &n, &c->order);
    secp256k1_num_free(&n);
    if (secp256k1_num_is_zero(&sig->s))
        return 0;
    if (secp256k1_num_is_odd(&sig->s)) {
        secp256k1_num_sub(&sig->s, &c->order, &sig->s);
        if (recid)
            *recid ^= 1;
    }
    return 1;
}

void static secp256k1_ecdsa_sig_set_rs(secp256k1_ecdsa_sig_t *sig, const secp256k1_num_t *r, const secp256k1_num_t *s) {
    secp256k1_num_copy(&sig->r, r);
    secp256k1_num_copy(&sig->s, s);
}

void static secp256k1_ecdsa_pubkey_serialize(secp256k1_ge_t *elem, unsigned char *pub, int *size, int compressed) {
    secp256k1_fe_normalize(&elem->x);
    secp256k1_fe_normalize(&elem->y);
    secp256k1_fe_get_b32(&pub[1], &elem->x);
    if (compressed) {
        *size = 33;
        pub[0] = 0x02 | (secp256k1_fe_is_odd(&elem->y) ? 0x01 : 0x00);
    } else {
        *size = 65;
        pub[0] = 0x04;
        secp256k1_fe_get_b32(&pub[33], &elem->y);
    }
}

int static secp256k1_ecdsa_privkey_parse(secp256k1_num_t *key, const unsigned char *privkey, int privkeylen) {
    const unsigned char *end = privkey + privkeylen;
    // sequence header
    if (end < privkey+1 || *privkey != 0x30)
        return 0;
    privkey++;
    // sequence length constructor
    int lenb = 0;
    if (end < privkey+1 || !(*privkey & 0x80))
        return 0;
    lenb = *privkey & ~0x80; privkey++;
    if (lenb < 1 || lenb > 2)
        return 0;
    if (end < privkey+lenb)
        return 0;
    // sequence length
    int len = 0;
    len = privkey[lenb-1] | (lenb > 1 ? privkey[lenb-2] << 8 : 0);
    privkey += lenb;
    if (end < privkey+len)
        return 0;
    // sequence element 0: version number (=1)
    if (end < privkey+3 || privkey[0] != 0x02 || privkey[1] != 0x01 || privkey[2] != 0x01)
        return 0;
    privkey += 3;
    // sequence element 1: octet string, 32 bytes
    if (end < privkey+34 || privkey[0] != 0x04 || privkey[1] != 0x20)
        return 0;
    secp256k1_num_set_bin(key, privkey+2, 32);
    return 1;
}

int static secp256k1_ecdsa_privkey_serialize(unsigned char *privkey, int *privkeylen, const secp256k1_num_t *key, int compressed) {
    secp256k1_gej_t rp;
    secp256k1_ecmult_gen(&rp, key);
    secp256k1_ge_t r;
    secp256k1_ge_set_gej(&r, &rp);
    if (compressed) {
        static const unsigned char begin[] = {
            0x30,0x81,0xD3,0x02,0x01,0x01,0x04,0x20
        };
        static const unsigned char middle[] = {
            0xA0,0x81,0x85,0x30,0x81,0x82,0x02,0x01,0x01,0x30,0x2C,0x06,0x07,0x2A,0x86,0x48,
            0xCE,0x3D,0x01,0x01,0x02,0x21,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFE,0xFF,0xFF,0xFC,0x2F,0x30,0x06,0x04,0x01,0x00,0x04,0x01,0x07,0x04,
            0x21
        };
        static const unsigned char end[] = {
            0x02,0x21,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFE,0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,0xBF,0xD2,0x5E,0x8C,0xD0,
            0x36,0x41,0x41,0x02,0x01,0x01,0xA1,0x24,0x03,0x22,0x00,0x03,0x18,0x39,0x05,0xAE,
            0x25,0xE8,0x15,0x63,0x4C,0xE7,0xF5,0xD9,0xBE,0xDB,0xAA,0x2C,0x39,0x03,0x2A,0xB9,
            0x8C,0x75,0xB5,0xE8,0x8F,0xE4,0x3F,0x8D,0xD8,0x24,0x6F,0x3C
        };
        unsigned char *ptr = privkey;
        memcpy(ptr, begin, sizeof(begin)); ptr += sizeof(begin);
        secp256k1_num_get_bin(ptr, 32, key); ptr += 32;
        memcpy(ptr, middle, sizeof(middle)); ptr += sizeof(middle);
        int pubkeylen = 0;
        secp256k1_ecdsa_pubkey_serialize(&r, ptr, &pubkeylen, 1); ptr += pubkeylen;
        memcpy(ptr, end, sizeof(end)); ptr += sizeof(end);
        *privkeylen = ptr - privkey;
    } else {
        static const unsigned char begin[] = {
            0x30,0x82,0x01,0x13,0x02,0x01,0x01,0x04,0x20
        };
        static const unsigned char middle[] = {
            0xA0,0x81,0xA5,0x30,0x81,0xA2,0x02,0x01,0x01,0x30,0x2C,0x06,0x07,0x2A,0x86,0x48,
            0xCE,0x3D,0x01,0x01,0x02,0x21,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFE,0xFF,0xFF,0xFC,0x2F,0x30,0x06,0x04,0x01,0x00,0x04,0x01,0x07,0x04,
            0x41
        };
        static const unsigned char end[] = {
            0x02,0x21,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFE,0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,0xBF,0xD2,0x5E,0x8C,0xD0,
            0x36,0x41,0x41,0x02,0x01,0x01,0xA1,0x44,0x03,0x42,0x00,0x04,0x18,0x39,0x05,0xAE,
            0x25,0xE8,0x15,0x63,0x4C,0xE7,0xF5,0xD9,0xBE,0xDB,0xAA,0x2C,0x39,0x03,0x2A,0xB9,
            0x8C,0x75,0xB5,0xE8,0x8F,0xE4,0x3F,0x8D,0xD8,0x24,0x6F,0x3C,0x54,0x73,0xCC,0xD4,
            0xAB,0x47,0x5E,0x6A,0x9E,0x66,0x20,0xB5,0x2F,0x5C,0xE2,0xFD,0x15,0xA2,0xDE,0x32,
            0xCB,0xE9,0x05,0x15,0x4B,0x3A,0x05,0x84,0x4A,0xF7,0x07,0x85
        };
        unsigned char *ptr = privkey;
        memcpy(ptr, begin, sizeof(begin)); ptr += sizeof(begin);
        secp256k1_num_get_bin(ptr, 32, key); ptr += 32;
        memcpy(ptr, middle, sizeof(middle)); ptr += sizeof(middle);
        int pubkeylen = 0;
        secp256k1_ecdsa_pubkey_serialize(&r, ptr, &pubkeylen, 0); ptr += pubkeylen;
        memcpy(ptr, end, sizeof(end)); ptr += sizeof(end);
        *privkeylen = ptr - privkey;
    }
    return 1;
}

#endif
