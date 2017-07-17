/**********************************************************************
 * Copyright (c) 2016 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_SURJECTION_IMPL_H_
#define _SECP256K1_SURJECTION_IMPL_H_

#include <assert.h>
#include <string.h>

#include "eckey.h"
#include "group.h"
#include "scalar.h"
#include "hash.h"

SECP256K1_INLINE static void secp256k1_surjection_genmessage(unsigned char *msg32, secp256k1_ge *ephemeral_input_tags, size_t n_input_tags, secp256k1_ge *ephemeral_output_tag) {
    /* compute message */
    size_t i;
    unsigned char pk_ser[33];
    size_t pk_len = sizeof(pk_ser);
    secp256k1_sha256_t sha256_en;

    secp256k1_sha256_initialize(&sha256_en);
    for (i = 0; i < n_input_tags; i++) {
        secp256k1_eckey_pubkey_serialize(&ephemeral_input_tags[i], pk_ser, &pk_len, 1);
        assert(pk_len == sizeof(pk_ser));
        secp256k1_sha256_write(&sha256_en, pk_ser, pk_len);
    }
    secp256k1_eckey_pubkey_serialize(ephemeral_output_tag, pk_ser, &pk_len, 1);
    assert(pk_len == sizeof(pk_ser));
    secp256k1_sha256_write(&sha256_en, pk_ser, pk_len);
    secp256k1_sha256_finalize(&sha256_en, msg32);
}

SECP256K1_INLINE static int secp256k1_surjection_genrand(secp256k1_scalar *s, size_t ns, const secp256k1_scalar *blinding_key) {
    size_t i;
    unsigned char sec_input[36];
    secp256k1_sha256_t sha256_en;

    /* compute s values */
    secp256k1_scalar_get_b32(&sec_input[4], blinding_key);
    for (i = 0; i < ns; i++) {
        int overflow = 0;
        sec_input[0] = i;
        sec_input[1] = i >> 8;
        sec_input[2] = i >> 16;
        sec_input[3] = i >> 24;

        secp256k1_sha256_initialize(&sha256_en);
        secp256k1_sha256_write(&sha256_en, sec_input, 36);
        secp256k1_sha256_finalize(&sha256_en, sec_input);
        secp256k1_scalar_set_b32(&s[i], sec_input, &overflow);
        if (overflow == 1) {
            memset(sec_input, 0, 32);
            return 0;
        }
    }
    memset(sec_input, 0, 32);
    return 1;
}

SECP256K1_INLINE static int secp256k1_surjection_compute_public_keys(secp256k1_gej *pubkeys, size_t n_pubkeys, const secp256k1_ge *input_tags, size_t n_input_tags, const unsigned char *used_tags, const secp256k1_ge *output_tag, size_t input_index, size_t *ring_input_index) {
    size_t i;
    size_t j = 0;
    for (i = 0; i < n_input_tags; i++) {
        if (used_tags[i / 8] & (1 << (i % 8))) {
            secp256k1_ge tmpge;
            secp256k1_ge_neg(&tmpge, &input_tags[i]);
            secp256k1_gej_set_ge(&pubkeys[j], &tmpge);
            secp256k1_gej_add_ge_var(&pubkeys[j], &pubkeys[j], output_tag, NULL);
            if (ring_input_index != NULL && input_index == i) {
                *ring_input_index = j;
            }
            j++;
            if (j > n_pubkeys) {
                return 0;
            }
        }
    }
    return 1;
}


#endif
