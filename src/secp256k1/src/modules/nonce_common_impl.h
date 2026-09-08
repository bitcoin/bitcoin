/***********************************************************************
 * Copyright (c) The libsecp256k1 developers                           *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_NONCE_COMMON_IMPL_H
#define SECP256K1_MODULE_NONCE_COMMON_IMPL_H

#include <string.h>

#include "../../include/secp256k1.h"

#include "nonce_common.h"
#include "../ecmult.h"
#include "../group.h"
#include "../scalar.h"
#include "../util.h"

static void secp256k1_secnonce_save(unsigned char *data, const unsigned char *magic, const secp256k1_scalar *k, const secp256k1_ge *pk) {
    memcpy(&data[0], magic, 4);
    secp256k1_scalar_get_b32(&data[4], &k[0]);
    secp256k1_scalar_get_b32(&data[36], &k[1]);
    secp256k1_ge_to_bytes(&data[68], pk);
}

static int secp256k1_secnonce_load(const secp256k1_context *ctx, secp256k1_scalar *k, secp256k1_ge *pk, const unsigned char *data, const unsigned char *magic) {
    int is_zero;
    ARG_CHECK(secp256k1_memcmp_var(&data[0], magic, 4) == 0);
    /* We make very sure that the nonce isn't invalidated by checking the values
     * in addition to the magic. */
    is_zero = secp256k1_is_zero_array(&data[4], 2 * 32);
    secp256k1_declassify(ctx, &is_zero, sizeof(is_zero));
    ARG_CHECK(!is_zero);

    secp256k1_scalar_set_b32(&k[0], &data[4], NULL);
    secp256k1_scalar_set_b32(&k[1], &data[36], NULL);
    secp256k1_ge_from_bytes(pk, &data[68]);
    return 1;
}

static void secp256k1_secnonce_invalidate(const secp256k1_context *ctx, unsigned char *data, int flag) {
    secp256k1_memczero(data, 132, flag);
    /* The flag argument is usually classified. So, the line above makes the
     * magic and public key classified. However, we need both to be
     * declassified. Note that we don't declassify the entire object, because if
     * flag is 0, then k[0] and k[1] have not been zeroed. */
    secp256k1_declassify(ctx, data, 4);
    secp256k1_declassify(ctx, &data[68], 64);
}

static void secp256k1_nonce_points_save(unsigned char *data, const unsigned char *magic, const secp256k1_ge *ges) {
    int i;
    memcpy(&data[0], magic, 4);
    for (i = 0; i < 2; i++) {
        secp256k1_ge_to_bytes(&data[4 + 64*i], &ges[i]);
    }
}

static int secp256k1_nonce_points_load(const secp256k1_context *ctx, secp256k1_ge *ges, const unsigned char *data, const unsigned char *magic) {
    int i;

    ARG_CHECK(secp256k1_memcmp_var(&data[0], magic, 4) == 0);
    for (i = 0; i < 2; i++) {
        secp256k1_ge_from_bytes(&ges[i], &data[4 + 64*i]);
    }
    return 1;
}

static int secp256k1_nonce_points_parse(secp256k1_ge *ges, const unsigned char *in66) {
    int i;

    for (i = 0; i < 2; i++) {
        if (!secp256k1_ge_parse(&ges[i], &in66[33*i], 33)) {
            return 0;
        }
        if (!secp256k1_ge_is_in_correct_subgroup(&ges[i])) {
            return 0;
        }
    }
    return 1;
}

static void secp256k1_nonce_points_serialize(unsigned char *out66, secp256k1_ge *ges) {
    int i;

    for (i = 0; i < 2; i++) {
        secp256k1_ge_serialize33(&ges[i], &out66[33*i]);
    }
}

static void secp256k1_partial_sig_save(unsigned char *data, const unsigned char *magic, const secp256k1_scalar *s) {
    memcpy(&data[0], magic, 4);
    secp256k1_scalar_get_b32(&data[4], s);
}

static int secp256k1_partial_sig_load(const secp256k1_context *ctx, secp256k1_scalar *s, const unsigned char *data, const unsigned char *magic) {
    int overflow;

    ARG_CHECK(secp256k1_memcmp_var(&data[0], magic, 4) == 0);
    secp256k1_scalar_set_b32(s, &data[4], &overflow);
    /* Parsed signatures can not overflow */
    VERIFY_CHECK(!overflow);
    return 1;
}

static void secp256k1_effective_nonce(secp256k1_gej *out_nonce, const secp256k1_ge *nonce_pts, const secp256k1_scalar *b) {
    secp256k1_gej tmp;

    secp256k1_gej_set_ge(&tmp, &nonce_pts[1]);
    secp256k1_ecmult(out_nonce, &tmp, b, NULL);
    secp256k1_gej_add_ge_var(out_nonce, out_nonce, &nonce_pts[0], NULL);
}

static void secp256k1_partial_sign_clear(secp256k1_scalar *sk, secp256k1_scalar *k) {
    secp256k1_scalar_clear(sk);
    secp256k1_scalar_clear(&k[0]);
    secp256k1_scalar_clear(&k[1]);
}

#endif
