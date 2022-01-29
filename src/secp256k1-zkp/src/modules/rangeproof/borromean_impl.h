/**********************************************************************
 * Copyright (c) 2014, 2015 Gregory Maxwell                          *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/


#ifndef _SECP256K1_BORROMEAN_IMPL_H_
#define _SECP256K1_BORROMEAN_IMPL_H_

#include "scalar.h"
#include "field.h"
#include "group.h"
#include "hash.h"
#include "eckey.h"
#include "ecmult.h"
#include "ecmult_gen.h"
#include "borromean.h"

#include <limits.h>
#include <string.h>

#ifdef WORDS_BIGENDIAN
#define BE32(x) (x)
#else
#define BE32(p) ((((p) & 0xFF) << 24) | (((p) & 0xFF00) << 8) | (((p) & 0xFF0000) >> 8) | (((p) & 0xFF000000) >> 24))
#endif

SECP256K1_INLINE static void secp256k1_borromean_hash(unsigned char *hash, const unsigned char *m, size_t mlen, const unsigned char *e, size_t elen,
 size_t ridx, size_t eidx) {
    uint32_t ring;
    uint32_t epos;
    secp256k1_sha256 sha256_en;
    secp256k1_sha256_initialize(&sha256_en);
    ring = BE32((uint32_t)ridx);
    epos = BE32((uint32_t)eidx);
    secp256k1_sha256_write(&sha256_en, e, elen);
    secp256k1_sha256_write(&sha256_en, m, mlen);
    secp256k1_sha256_write(&sha256_en, (unsigned char*)&ring, 4);
    secp256k1_sha256_write(&sha256_en, (unsigned char*)&epos, 4);
    secp256k1_sha256_finalize(&sha256_en, hash);
}

/**  "Borromean" ring signature.
 *   Verifies nrings concurrent ring signatures all sharing a challenge value.
 *   Signature is one s value per pubkey and a hash.
 *   Verification equation:
 *   | m = H(P_{0..}||message) (Message must contain pubkeys or a pubkey commitment)
 *   | For each ring i:
 *   | | en = to_scalar(H(e0||m||i||0))
 *   | | For each pubkey j:
 *   | | | r = s_i_j G + en * P_i_j
 *   | | | e = H(r||m||i||j)
 *   | | | en = to_scalar(e)
 *   | | r_i = r
 *   | return e_0 ==== H(r_{0..i}||m)
 */
int secp256k1_borromean_verify(const secp256k1_ecmult_context* ecmult_ctx, secp256k1_scalar *evalues, const unsigned char *e0,
 const secp256k1_scalar *s, const secp256k1_gej *pubs, const size_t *rsizes, size_t nrings, const unsigned char *m, size_t mlen) {
    secp256k1_gej rgej;
    secp256k1_ge rge;
    secp256k1_scalar ens;
    secp256k1_sha256 sha256_e0;
    unsigned char tmp[33];
    size_t i;
    size_t j;
    size_t count;
    size_t size;
    int overflow;
    VERIFY_CHECK(ecmult_ctx != NULL);
    VERIFY_CHECK(e0 != NULL);
    VERIFY_CHECK(s != NULL);
    VERIFY_CHECK(pubs != NULL);
    VERIFY_CHECK(rsizes != NULL);
    VERIFY_CHECK(nrings > 0);
    VERIFY_CHECK(m != NULL);
    count = 0;
    secp256k1_sha256_initialize(&sha256_e0);
    for (i = 0; i < nrings; i++) {
        VERIFY_CHECK(INT_MAX - count > rsizes[i]);
        secp256k1_borromean_hash(tmp, m, mlen, e0, 32, i, 0);
        secp256k1_scalar_set_b32(&ens, tmp, &overflow);
        for (j = 0; j < rsizes[i]; j++) {
            if (overflow || secp256k1_scalar_is_zero(&s[count]) || secp256k1_scalar_is_zero(&ens) || secp256k1_gej_is_infinity(&pubs[count])) {
                return 0;
            }
            if (evalues) {
                /*If requested, save the challenges for proof rewind.*/
                evalues[count] = ens;
            }
            secp256k1_ecmult(ecmult_ctx, &rgej, &pubs[count], &ens, &s[count]);
            if (secp256k1_gej_is_infinity(&rgej)) {
                return 0;
            }
            /* OPT: loop can be hoisted and split to use batch inversion across all the rings; this would make it much faster. */
            secp256k1_ge_set_gej_var(&rge, &rgej);
            secp256k1_eckey_pubkey_serialize(&rge, tmp, &size, 1);
            if (j != rsizes[i] - 1) {
                secp256k1_borromean_hash(tmp, m, mlen, tmp, 33, i, j + 1);
                secp256k1_scalar_set_b32(&ens, tmp, &overflow);
            } else {
                secp256k1_sha256_write(&sha256_e0, tmp, size);
            }
            count++;
        }
    }
    secp256k1_sha256_write(&sha256_e0, m, mlen);
    secp256k1_sha256_finalize(&sha256_e0, tmp);
    return memcmp(e0, tmp, 32) == 0;
}

int secp256k1_borromean_sign(const secp256k1_ecmult_context* ecmult_ctx, const secp256k1_ecmult_gen_context *ecmult_gen_ctx,
 unsigned char *e0, secp256k1_scalar *s, const secp256k1_gej *pubs, const secp256k1_scalar *k, const secp256k1_scalar *sec,
 const size_t *rsizes, const size_t *secidx, size_t nrings, const unsigned char *m, size_t mlen) {
    secp256k1_gej rgej;
    secp256k1_ge rge;
    secp256k1_scalar ens;
    secp256k1_sha256 sha256_e0;
    unsigned char tmp[33];
    size_t i;
    size_t j;
    size_t count;
    size_t size;
    int overflow;
    VERIFY_CHECK(ecmult_ctx != NULL);
    VERIFY_CHECK(ecmult_gen_ctx != NULL);
    VERIFY_CHECK(e0 != NULL);
    VERIFY_CHECK(s != NULL);
    VERIFY_CHECK(pubs != NULL);
    VERIFY_CHECK(k != NULL);
    VERIFY_CHECK(sec != NULL);
    VERIFY_CHECK(rsizes != NULL);
    VERIFY_CHECK(secidx != NULL);
    VERIFY_CHECK(nrings > 0);
    VERIFY_CHECK(m != NULL);
    secp256k1_sha256_initialize(&sha256_e0);
    count = 0;
    for (i = 0; i < nrings; i++) {
        VERIFY_CHECK(INT_MAX - count > rsizes[i]);
        secp256k1_ecmult_gen(ecmult_gen_ctx, &rgej, &k[i]);
        secp256k1_ge_set_gej(&rge, &rgej);
        if (secp256k1_gej_is_infinity(&rgej)) {
            return 0;
        }
        secp256k1_eckey_pubkey_serialize(&rge, tmp, &size, 1);
        for (j = secidx[i] + 1; j < rsizes[i]; j++) {
            secp256k1_borromean_hash(tmp, m, mlen, tmp, 33, i, j);
            secp256k1_scalar_set_b32(&ens, tmp, &overflow);
            if (overflow || secp256k1_scalar_is_zero(&ens)) {
                return 0;
            }
            /** The signing algorithm as a whole is not memory uniform so there is likely a cache sidechannel that
             *  leaks which members are non-forgeries. That the forgeries themselves are variable time may leave
             *  an additional privacy impacting timing side-channel, but not a key loss one.
             */
            secp256k1_ecmult(ecmult_ctx, &rgej, &pubs[count + j], &ens, &s[count + j]);
            if (secp256k1_gej_is_infinity(&rgej)) {
                return 0;
            }
            secp256k1_ge_set_gej_var(&rge, &rgej);
            secp256k1_eckey_pubkey_serialize(&rge, tmp, &size, 1);
        }
        secp256k1_sha256_write(&sha256_e0, tmp, size);
        count += rsizes[i];
    }
    secp256k1_sha256_write(&sha256_e0, m, mlen);
    secp256k1_sha256_finalize(&sha256_e0, e0);
    count = 0;
    for (i = 0; i < nrings; i++) {
        VERIFY_CHECK(INT_MAX - count > rsizes[i]);
        secp256k1_borromean_hash(tmp, m, mlen, e0, 32, i, 0);
        secp256k1_scalar_set_b32(&ens, tmp, &overflow);
        if (overflow || secp256k1_scalar_is_zero(&ens)) {
            return 0;
        }
        for (j = 0; j < secidx[i]; j++) {
            secp256k1_ecmult(ecmult_ctx, &rgej, &pubs[count + j], &ens, &s[count + j]);
            if (secp256k1_gej_is_infinity(&rgej)) {
                return 0;
            }
            secp256k1_ge_set_gej_var(&rge, &rgej);
            secp256k1_eckey_pubkey_serialize(&rge, tmp, &size, 1);
            secp256k1_borromean_hash(tmp, m, mlen, tmp, 33, i, j + 1);
            secp256k1_scalar_set_b32(&ens, tmp, &overflow);
            if (overflow || secp256k1_scalar_is_zero(&ens)) {
                return 0;
            }
        }
        secp256k1_scalar_mul(&s[count + j], &ens, &sec[i]);
        secp256k1_scalar_negate(&s[count + j], &s[count + j]);
        secp256k1_scalar_add(&s[count + j], &s[count + j], &k[i]);
        if (secp256k1_scalar_is_zero(&s[count + j])) {
            return 0;
        }
        count += rsizes[i];
    }
    secp256k1_scalar_clear(&ens);
    secp256k1_ge_clear(&rge);
    secp256k1_gej_clear(&rgej);
    memset(tmp, 0, 33);
    return 1;
}

#endif
