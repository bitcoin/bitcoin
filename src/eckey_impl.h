/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_ECKEY_IMPL_H_
#define _SECP256K1_ECKEY_IMPL_H_

#include "eckey.h"

#include "scalar.h"
#include "field.h"
#include "group.h"
#include "ecmult_gen.h"

static int secp256k1_eckey_pubkey_parse(secp256k1_ge *elem, const unsigned char *pub, size_t size) {
    if (size == 33 && (pub[0] == 0x02 || pub[0] == 0x03)) {
        secp256k1_fe x;
        return secp256k1_fe_set_b32(&x, pub+1) && secp256k1_ge_set_xo_var(elem, &x, pub[0] == 0x03);
    } else if (size == 65 && (pub[0] == 0x04 || pub[0] == 0x06 || pub[0] == 0x07)) {
        secp256k1_fe x, y;
        if (!secp256k1_fe_set_b32(&x, pub+1) || !secp256k1_fe_set_b32(&y, pub+33)) {
            return 0;
        }
        secp256k1_ge_set_xy(elem, &x, &y);
        if ((pub[0] == 0x06 || pub[0] == 0x07) && secp256k1_fe_is_odd(&y) != (pub[0] == 0x07)) {
            return 0;
        }
        return secp256k1_ge_is_valid_var(elem);
    } else {
        return 0;
    }
}

static int secp256k1_eckey_pubkey_serialize(secp256k1_ge *elem, unsigned char *pub, size_t *size, unsigned int flags) {
    if (secp256k1_ge_is_infinity(elem)) {
        return 0;
    }
    secp256k1_fe_normalize_var(&elem->x);
    secp256k1_fe_normalize_var(&elem->y);
    secp256k1_fe_get_b32(&pub[1], &elem->x);
    if (flags & SECP256K1_EC_COMPRESSED) {
        *size = 33;
        pub[0] = 0x02 | (secp256k1_fe_is_odd(&elem->y) ? 0x01 : 0x00);
    } else {
        *size = 65;
        pub[0] = 0x04;
        secp256k1_fe_get_b32(&pub[33], &elem->y);
    }
    return 1;
}

static int secp256k1_eckey_privkey_parse(secp256k1_scalar *key, const unsigned char *privkey, size_t privkeylen) {
    unsigned char c[32] = {0};
    const unsigned char *end = privkey + privkeylen;
    int lenb = 0;
    int len = 0;
    int overflow = 0;
    /* sequence header */
    if (end < privkey+1 || *privkey != 0x30) {
        return 0;
    }
    privkey++;
    /* sequence length constructor */
    if (end < privkey+1 || !(*privkey & 0x80)) {
        return 0;
    }
    lenb = *privkey & ~0x80; privkey++;
    if (lenb < 1 || lenb > 2) {
        return 0;
    }
    if (end < privkey+lenb) {
        return 0;
    }
    /* sequence length */
    len = privkey[lenb-1] | (lenb > 1 ? privkey[lenb-2] << 8 : 0);
    privkey += lenb;
    if (end < privkey+len) {
        return 0;
    }
    /* sequence element 0: version number (=1) */
    if (end < privkey+3 || privkey[0] != 0x02 || privkey[1] != 0x01 || privkey[2] != 0x01) {
        return 0;
    }
    privkey += 3;
    /* sequence element 1: octet string, up to 32 bytes */
    if (end < privkey+2 || privkey[0] != 0x04 || privkey[1] > 0x20 || end < privkey+2+privkey[1]) {
        return 0;
    }
    memcpy(c + 32 - privkey[1], privkey + 2, privkey[1]);
    secp256k1_scalar_set_b32(key, c, &overflow);
    memset(c, 0, 32);
    return !overflow;
}

static int secp256k1_eckey_privkey_serialize(const secp256k1_ecmult_gen_context *ctx, unsigned char *privkey, size_t *privkeylen, const secp256k1_scalar *key, unsigned int flags) {
    secp256k1_gej rp;
    secp256k1_ge r;
    size_t pubkeylen = 0;
    secp256k1_ecmult_gen(ctx, &rp, key);
    secp256k1_ge_set_gej(&r, &rp);
    if (flags & SECP256K1_EC_COMPRESSED) {
        static const unsigned char begin[] = {
            0x30,0x81,0xD3,0x02,0x01,0x01,0x04,0x20
        };
        static const unsigned char middle[] = {
            0xA0,0x81,0x85,0x30,0x81,0x82,0x02,0x01,0x01,0x30,0x2C,0x06,0x07,0x2A,0x86,0x48,
            0xCE,0x3D,0x01,0x01,0x02,0x21,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFE,0xFF,0xFF,0xFC,0x2F,0x30,0x06,0x04,0x01,0x00,0x04,0x01,0x07,0x04,
            0x21,0x02,0x79,0xBE,0x66,0x7E,0xF9,0xDC,0xBB,0xAC,0x55,0xA0,0x62,0x95,0xCE,0x87,
            0x0B,0x07,0x02,0x9B,0xFC,0xDB,0x2D,0xCE,0x28,0xD9,0x59,0xF2,0x81,0x5B,0x16,0xF8,
            0x17,0x98,0x02,0x21,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFF,0xFF,0xFE,0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,0xBF,0xD2,0x5E,
            0x8C,0xD0,0x36,0x41,0x41,0x02,0x01,0x01,0xA1,0x24,0x03,0x22,0x00
        };
        unsigned char *ptr = privkey;
        memcpy(ptr, begin, sizeof(begin)); ptr += sizeof(begin);
        secp256k1_scalar_get_b32(ptr, key); ptr += 32;
        memcpy(ptr, middle, sizeof(middle)); ptr += sizeof(middle);
        if (!secp256k1_eckey_pubkey_serialize(&r, ptr, &pubkeylen, 1)) {
            return 0;
        }
        ptr += pubkeylen;
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
            0x41,0x04,0x79,0xBE,0x66,0x7E,0xF9,0xDC,0xBB,0xAC,0x55,0xA0,0x62,0x95,0xCE,0x87,
            0x0B,0x07,0x02,0x9B,0xFC,0xDB,0x2D,0xCE,0x28,0xD9,0x59,0xF2,0x81,0x5B,0x16,0xF8,
            0x17,0x98,0x48,0x3A,0xDA,0x77,0x26,0xA3,0xC4,0x65,0x5D,0xA4,0xFB,0xFC,0x0E,0x11,
            0x08,0xA8,0xFD,0x17,0xB4,0x48,0xA6,0x85,0x54,0x19,0x9C,0x47,0xD0,0x8F,0xFB,0x10,
            0xD4,0xB8,0x02,0x21,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFF,0xFF,0xFE,0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,0xBF,0xD2,0x5E,
            0x8C,0xD0,0x36,0x41,0x41,0x02,0x01,0x01,0xA1,0x44,0x03,0x42,0x00
        };
        unsigned char *ptr = privkey;
        memcpy(ptr, begin, sizeof(begin)); ptr += sizeof(begin);
        secp256k1_scalar_get_b32(ptr, key); ptr += 32;
        memcpy(ptr, middle, sizeof(middle)); ptr += sizeof(middle);
        if (!secp256k1_eckey_pubkey_serialize(&r, ptr, &pubkeylen, 0)) {
            return 0;
        }
        ptr += pubkeylen;
        *privkeylen = ptr - privkey;
    }
    return 1;
}

static int secp256k1_eckey_privkey_tweak_add(secp256k1_scalar *key, const secp256k1_scalar *tweak) {
    secp256k1_scalar_add(key, key, tweak);
    if (secp256k1_scalar_is_zero(key)) {
        return 0;
    }
    return 1;
}

static int secp256k1_eckey_pubkey_tweak_add(const secp256k1_ecmult_context *ctx, secp256k1_ge *key, const secp256k1_scalar *tweak) {
    secp256k1_gej pt;
    secp256k1_scalar one;
    secp256k1_gej_set_ge(&pt, key);
    secp256k1_scalar_set_int(&one, 1);
    secp256k1_ecmult(ctx, &pt, &pt, &one, tweak);

    if (secp256k1_gej_is_infinity(&pt)) {
        return 0;
    }
    secp256k1_ge_set_gej(key, &pt);
    return 1;
}

static int secp256k1_eckey_privkey_tweak_mul(secp256k1_scalar *key, const secp256k1_scalar *tweak) {
    if (secp256k1_scalar_is_zero(tweak)) {
        return 0;
    }

    secp256k1_scalar_mul(key, key, tweak);
    return 1;
}

static int secp256k1_eckey_pubkey_tweak_mul(const secp256k1_ecmult_context *ctx, secp256k1_ge *key, const secp256k1_scalar *tweak) {
    secp256k1_scalar zero;
    secp256k1_gej pt;
    if (secp256k1_scalar_is_zero(tweak)) {
        return 0;
    }

    secp256k1_scalar_set_int(&zero, 0);
    secp256k1_gej_set_ge(&pt, key);
    secp256k1_ecmult(ctx, &pt, &pt, tweak, &zero);
    secp256k1_ge_set_gej(key, &pt);
    return 1;
}

#endif
