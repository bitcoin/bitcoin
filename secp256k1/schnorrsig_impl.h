/***********************************************************************
 * Copyright (c) 2018-2020 Andrew Poelstra, Jonas Nick                 *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_SCHNORRSIG_IMPL_H
#define SECP256K1_SCHNORRSIG_IMPL_H

#include "secp256k1.h"
#include "schnorrsig.h"
#include "../sha256.h"

/* Initializes SHA256 with fixed midstate. This midstate was computed by applying
 * SHA256 to SHA256("BIP0340/challenge")||SHA256("BIP0340/challenge"). */
static sha256_context secp256k1_schnorrsig_sha256_tagged(uint32_t* output) {
    output[0] = 0x9cecba11ul;
    output[1] = 0x23925381ul;
    output[2] = 0x11679112ul;
    output[3] = 0xd1627e0ful;
    output[4] = 0x97c87550ul;
    output[5] = 0x003cc765ul;
    output[6] = 0x90f61164ul;
    output[7] = 0x33e9b66aul;
    return (sha256_context){ .output = output, .counter = 64 };
}

static void secp256k1_schnorrsig_challenge(secp256k1_scalar* e, const unsigned char *r32, const unsigned char *msg, size_t msglen, const unsigned char *pubkey32)
{
    unsigned char buf[32];
   sha256_midstate sha_buf;

    /* tagged hash(r.x, pk.x, msg32) */
    sha256_context sha = secp256k1_schnorrsig_sha256_tagged(sha_buf.s);
    sha256_uchars(&sha, r32, 32);
    sha256_uchars(&sha, pubkey32, 32);
    sha256_uchars(&sha, msg, msglen);
    sha256_finalize(&sha);
    sha256_fromMidstate(buf, sha_buf.s);

    /* Set scalar e to the challenge hash modulo the curve order as per
     * BIP340. */
    secp256k1_scalar_set_b32(e, buf, NULL);
}

static int secp256k1_schnorrsig_verify(const unsigned char *sig64, const unsigned char *msg, size_t msglen, const secp256k1_xonly_pubkey *pubkey) {
    secp256k1_scalar s;
    secp256k1_scalar e;
    secp256k1_gej rj;
    secp256k1_ge pk;
    secp256k1_gej pkj;
    secp256k1_fe rx;
    secp256k1_ge r;
    unsigned char buf[32];
    int overflow;

    ARG_CHECK(sig64 != NULL);
    ARG_CHECK(msg != NULL || msglen == 0);
    ARG_CHECK(pubkey != NULL);

    if (!secp256k1_fe_set_b32(&rx, &sig64[0])) {
        return 0;
    }

    secp256k1_scalar_set_b32(&s, &sig64[32], &overflow);
    if (overflow) {
        return 0;
    }

    if (!secp256k1_xonly_pubkey_load(&pk, pubkey)) {
        return 0;
    }

    /* Compute e. */
    secp256k1_fe_get_b32(buf, &pk.x);
    secp256k1_schnorrsig_challenge(&e, &sig64[0], msg, msglen, buf);

    /* Compute rj =  s*G + (-e)*pkj */
    secp256k1_scalar_negate(&e, &e);
    secp256k1_gej_set_ge(&pkj, &pk);
    secp256k1_ecmult(&rj, &pkj, &e, &s);

    secp256k1_ge_set_gej_var(&r, &rj);
    if (secp256k1_ge_is_infinity(&r)) {
        return 0;
    }

    secp256k1_fe_normalize_var(&r.y);
    return !secp256k1_fe_is_odd(&r.y) &&
           secp256k1_fe_equal_var(&rx, &r.x);
}

#endif
