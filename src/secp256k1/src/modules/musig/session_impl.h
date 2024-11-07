/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_MUSIG_SESSION_IMPL_H
#define SECP256K1_MODULE_MUSIG_SESSION_IMPL_H

#include <string.h>

#include "../../../include/secp256k1.h"
#include "../../../include/secp256k1_extrakeys.h"
#include "../../../include/secp256k1_musig.h"

#include "keyagg.h"
#include "session.h"
#include "../../eckey.h"
#include "../../hash.h"
#include "../../scalar.h"
#include "../../util.h"

/* Outputs 33 zero bytes if the given group element is the point at infinity and
 * otherwise outputs the compressed serialization */
static void secp256k1_musig_ge_serialize_ext(unsigned char *out33, secp256k1_ge* ge) {
    if (secp256k1_ge_is_infinity(ge)) {
        memset(out33, 0, 33);
    } else {
        int ret;
        size_t size = 33;
        ret = secp256k1_eckey_pubkey_serialize(ge, out33, &size, 1);
#ifdef VERIFY
        /* Serialize must succeed because the point is not at infinity */
        VERIFY_CHECK(ret && size == 33);
#else
        (void) ret;
#endif
    }
}

/* Outputs the point at infinity if the given byte array is all zero, otherwise
 * attempts to parse compressed point serialization. */
static int secp256k1_musig_ge_parse_ext(secp256k1_ge* ge, const unsigned char *in33) {
    unsigned char zeros[33] = { 0 };

    if (secp256k1_memcmp_var(in33, zeros, sizeof(zeros)) == 0) {
        secp256k1_ge_set_infinity(ge);
        return 1;
    }
    if (!secp256k1_eckey_pubkey_parse(ge, in33, 33)) {
        return 0;
    }
    return secp256k1_ge_is_in_correct_subgroup(ge);
}

static const unsigned char secp256k1_musig_secnonce_magic[4] = { 0x22, 0x0e, 0xdc, 0xf1 };

static void secp256k1_musig_secnonce_save(secp256k1_musig_secnonce *secnonce, const secp256k1_scalar *k, const secp256k1_ge *pk) {
    memcpy(&secnonce->data[0], secp256k1_musig_secnonce_magic, 4);
    secp256k1_scalar_get_b32(&secnonce->data[4], &k[0]);
    secp256k1_scalar_get_b32(&secnonce->data[36], &k[1]);
    secp256k1_ge_to_bytes(&secnonce->data[68], pk);
}

static int secp256k1_musig_secnonce_load(const secp256k1_context* ctx, secp256k1_scalar *k, secp256k1_ge *pk, const secp256k1_musig_secnonce *secnonce) {
    int is_zero;
    ARG_CHECK(secp256k1_memcmp_var(&secnonce->data[0], secp256k1_musig_secnonce_magic, 4) == 0);
    /* We make very sure that the nonce isn't invalidated by checking the values
     * in addition to the magic. */
    is_zero = secp256k1_is_zero_array(&secnonce->data[4], 2 * 32);
    secp256k1_declassify(ctx, &is_zero, sizeof(is_zero));
    ARG_CHECK(!is_zero);

    secp256k1_scalar_set_b32(&k[0], &secnonce->data[4], NULL);
    secp256k1_scalar_set_b32(&k[1], &secnonce->data[36], NULL);
    secp256k1_ge_from_bytes(pk, &secnonce->data[68]);
    return 1;
}

/* If flag is true, invalidate the secnonce; otherwise leave it. Constant-time. */
static void secp256k1_musig_secnonce_invalidate(const secp256k1_context* ctx, secp256k1_musig_secnonce *secnonce, int flag) {
    secp256k1_memczero(secnonce->data, sizeof(secnonce->data), flag);
    /* The flag argument is usually classified. So, the line above makes the
     * magic and public key classified. However, we need both to be
     * declassified. Note that we don't declassify the entire object, because if
     * flag is 0, then k[0] and k[1] have not been zeroed. */
    secp256k1_declassify(ctx, secnonce->data, sizeof(secp256k1_musig_secnonce_magic));
    secp256k1_declassify(ctx, &secnonce->data[68], 64);
}

static const unsigned char secp256k1_musig_pubnonce_magic[4] = { 0xf5, 0x7a, 0x3d, 0xa0 };

/* Saves two group elements into a pubnonce. Requires that none of the provided
 * group elements is infinity. */
static void secp256k1_musig_pubnonce_save(secp256k1_musig_pubnonce* nonce, const secp256k1_ge* ges) {
    int i;
    memcpy(&nonce->data[0], secp256k1_musig_pubnonce_magic, 4);
    for (i = 0; i < 2; i++) {
        secp256k1_ge_to_bytes(nonce->data + 4+64*i, &ges[i]);
    }
}

/* Loads two group elements from a pubnonce. Returns 1 unless the nonce wasn't
 * properly initialized */
static int secp256k1_musig_pubnonce_load(const secp256k1_context* ctx, secp256k1_ge* ges, const secp256k1_musig_pubnonce* nonce) {
    int i;

    ARG_CHECK(secp256k1_memcmp_var(&nonce->data[0], secp256k1_musig_pubnonce_magic, 4) == 0);
    for (i = 0; i < 2; i++) {
        secp256k1_ge_from_bytes(&ges[i], nonce->data + 4 + 64*i);
    }
    return 1;
}

static const unsigned char secp256k1_musig_aggnonce_magic[4] = { 0xa8, 0xb7, 0xe4, 0x67 };

static void secp256k1_musig_aggnonce_save(secp256k1_musig_aggnonce* nonce, const secp256k1_ge* ges) {
    int i;
    memcpy(&nonce->data[0], secp256k1_musig_aggnonce_magic, 4);
    for (i = 0; i < 2; i++) {
        secp256k1_ge_to_bytes_ext(&nonce->data[4 + 64*i], &ges[i]);
    }
}

static int secp256k1_musig_aggnonce_load(const secp256k1_context* ctx, secp256k1_ge* ges, const secp256k1_musig_aggnonce* nonce) {
    int i;

    ARG_CHECK(secp256k1_memcmp_var(&nonce->data[0], secp256k1_musig_aggnonce_magic, 4) == 0);
    for (i = 0; i < 2; i++) {
        secp256k1_ge_from_bytes_ext(&ges[i], &nonce->data[4 + 64*i]);
    }
    return 1;
}

static const unsigned char secp256k1_musig_session_cache_magic[4] = { 0x9d, 0xed, 0xe9, 0x17 };

/* A session consists of
 * - 4 byte session cache magic
 * - 1 byte the parity of the final nonce
 * - 32 byte serialized x-only final nonce
 * - 32 byte nonce coefficient b
 * - 32 byte signature challenge hash e
 * - 32 byte scalar s that is added to the partial signatures of the signers
 */
static void secp256k1_musig_session_save(secp256k1_musig_session *session, const secp256k1_musig_session_internal *session_i) {
    unsigned char *ptr = session->data;

    memcpy(ptr, secp256k1_musig_session_cache_magic, 4);
    ptr += 4;
    *ptr = session_i->fin_nonce_parity;
    ptr += 1;
    memcpy(ptr, session_i->fin_nonce, 32);
    ptr += 32;
    secp256k1_scalar_get_b32(ptr, &session_i->noncecoef);
    ptr += 32;
    secp256k1_scalar_get_b32(ptr, &session_i->challenge);
    ptr += 32;
    secp256k1_scalar_get_b32(ptr, &session_i->s_part);
}

static int secp256k1_musig_session_load(const secp256k1_context* ctx, secp256k1_musig_session_internal *session_i, const secp256k1_musig_session *session) {
    const unsigned char *ptr = session->data;

    ARG_CHECK(secp256k1_memcmp_var(ptr, secp256k1_musig_session_cache_magic, 4) == 0);
    ptr += 4;
    session_i->fin_nonce_parity = *ptr;
    ptr += 1;
    memcpy(session_i->fin_nonce, ptr, 32);
    ptr += 32;
    secp256k1_scalar_set_b32(&session_i->noncecoef, ptr, NULL);
    ptr += 32;
    secp256k1_scalar_set_b32(&session_i->challenge, ptr, NULL);
    ptr += 32;
    secp256k1_scalar_set_b32(&session_i->s_part, ptr, NULL);
    return 1;
}

static const unsigned char secp256k1_musig_partial_sig_magic[4] = { 0xeb, 0xfb, 0x1a, 0x32 };

static void secp256k1_musig_partial_sig_save(secp256k1_musig_partial_sig* sig, secp256k1_scalar *s) {
    memcpy(&sig->data[0], secp256k1_musig_partial_sig_magic, 4);
    secp256k1_scalar_get_b32(&sig->data[4], s);
}

static int secp256k1_musig_partial_sig_load(const secp256k1_context* ctx, secp256k1_scalar *s, const secp256k1_musig_partial_sig* sig) {
    int overflow;

    ARG_CHECK(secp256k1_memcmp_var(&sig->data[0], secp256k1_musig_partial_sig_magic, 4) == 0);
    secp256k1_scalar_set_b32(s, &sig->data[4], &overflow);
    /* Parsed signatures can not overflow */
    VERIFY_CHECK(!overflow);
    return 1;
}

int secp256k1_musig_pubnonce_parse(const secp256k1_context* ctx, secp256k1_musig_pubnonce* nonce, const unsigned char *in66) {
    secp256k1_ge ges[2];
    int i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(nonce != NULL);
    ARG_CHECK(in66 != NULL);

    for (i = 0; i < 2; i++) {
        if (!secp256k1_eckey_pubkey_parse(&ges[i], &in66[33*i], 33)) {
            return 0;
        }
        if (!secp256k1_ge_is_in_correct_subgroup(&ges[i])) {
            return 0;
        }
    }
    secp256k1_musig_pubnonce_save(nonce, ges);
    return 1;
}

int secp256k1_musig_pubnonce_serialize(const secp256k1_context* ctx, unsigned char *out66, const secp256k1_musig_pubnonce* nonce) {
    secp256k1_ge ges[2];
    int i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(out66 != NULL);
    memset(out66, 0, 66);
    ARG_CHECK(nonce != NULL);

    if (!secp256k1_musig_pubnonce_load(ctx, ges, nonce)) {
        return 0;
    }
    for (i = 0; i < 2; i++) {
        int ret;
        size_t size = 33;
        ret = secp256k1_eckey_pubkey_serialize(&ges[i], &out66[33*i], &size, 1);
#ifdef VERIFY
        /* serialize must succeed because the point was just loaded */
        VERIFY_CHECK(ret && size == 33);
#else
        (void) ret;
#endif
    }
    return 1;
}

int secp256k1_musig_aggnonce_parse(const secp256k1_context* ctx, secp256k1_musig_aggnonce* nonce, const unsigned char *in66) {
    secp256k1_ge ges[2];
    int i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(nonce != NULL);
    ARG_CHECK(in66 != NULL);

    for (i = 0; i < 2; i++) {
        if (!secp256k1_musig_ge_parse_ext(&ges[i], &in66[33*i])) {
            return 0;
        }
    }
    secp256k1_musig_aggnonce_save(nonce, ges);
    return 1;
}

int secp256k1_musig_aggnonce_serialize(const secp256k1_context* ctx, unsigned char *out66, const secp256k1_musig_aggnonce* nonce) {
    secp256k1_ge ges[2];
    int i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(out66 != NULL);
    memset(out66, 0, 66);
    ARG_CHECK(nonce != NULL);

    if (!secp256k1_musig_aggnonce_load(ctx, ges, nonce)) {
        return 0;
    }
    for (i = 0; i < 2; i++) {
        secp256k1_musig_ge_serialize_ext(&out66[33*i], &ges[i]);
    }
    return 1;
}

int secp256k1_musig_partial_sig_parse(const secp256k1_context* ctx, secp256k1_musig_partial_sig* sig, const unsigned char *in32) {
    secp256k1_scalar tmp;
    int overflow;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(sig != NULL);
    ARG_CHECK(in32 != NULL);

    /* Ensure that using the signature will fail if parsing fails (and the user
     * doesn't check the return value). */
    memset(sig, 0, sizeof(*sig));

    secp256k1_scalar_set_b32(&tmp, in32, &overflow);
    if (overflow) {
        return 0;
    }
    secp256k1_musig_partial_sig_save(sig, &tmp);
    return 1;
}

int secp256k1_musig_partial_sig_serialize(const secp256k1_context* ctx, unsigned char *out32, const secp256k1_musig_partial_sig* sig) {
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(out32 != NULL);
    ARG_CHECK(sig != NULL);
    ARG_CHECK(secp256k1_memcmp_var(&sig->data[0], secp256k1_musig_partial_sig_magic, 4) == 0);

    memcpy(out32, &sig->data[4], 32);
    return 1;
}

/* Write optional inputs into the hash */
static void secp256k1_nonce_function_musig_helper(secp256k1_sha256 *sha, unsigned int prefix_size, const unsigned char *data, unsigned char len) {
    unsigned char zero[7] = { 0 };
    /* The spec requires length prefixes to be between 1 and 8 bytes
     * (inclusive) */
    VERIFY_CHECK(prefix_size >= 1 && prefix_size <= 8);
    /* Since the length of all input data fits in a byte, we can always pad the
     * length prefix with prefix_size - 1 zero bytes. */
    secp256k1_sha256_write(sha, zero, prefix_size - 1);
    if (data != NULL) {
        secp256k1_sha256_write(sha, &len, 1);
        secp256k1_sha256_write(sha, data, len);
    } else {
        len = 0;
        secp256k1_sha256_write(sha, &len, 1);
    }
}

/* Initializes SHA256 with fixed midstate. This midstate was computed by applying
 * SHA256 to SHA256("MuSig/aux")||SHA256("MuSig/aux"). */
static void secp256k1_nonce_function_musig_sha256_tagged_aux(secp256k1_sha256 *sha) {
    secp256k1_sha256_initialize(sha);
    sha->s[0] = 0xa19e884bul;
    sha->s[1] = 0xf463fe7eul;
    sha->s[2] = 0x2f18f9a2ul;
    sha->s[3] = 0xbeb0f9fful;
    sha->s[4] = 0x0f37e8b0ul;
    sha->s[5] = 0x06ebd26ful;
    sha->s[6] = 0xe3b243d2ul;
    sha->s[7] = 0x522fb150ul;
    sha->bytes = 64;
}

/* Initializes SHA256 with fixed midstate. This midstate was computed by applying
 * SHA256 to SHA256("MuSig/nonce")||SHA256("MuSig/nonce"). */
static void secp256k1_nonce_function_musig_sha256_tagged(secp256k1_sha256 *sha) {
    secp256k1_sha256_initialize(sha);
    sha->s[0] = 0x07101b64ul;
    sha->s[1] = 0x18003414ul;
    sha->s[2] = 0x0391bc43ul;
    sha->s[3] = 0x0e6258eeul;
    sha->s[4] = 0x29d26b72ul;
    sha->s[5] = 0x8343937eul;
    sha->s[6] = 0xb7a0a4fbul;
    sha->s[7] = 0xff568a30ul;
    sha->bytes = 64;
}

static void secp256k1_nonce_function_musig(secp256k1_scalar *k, const unsigned char *session_secrand, const unsigned char *msg32, const unsigned char *seckey32, const unsigned char *pk33, const unsigned char *agg_pk32, const unsigned char *extra_input32) {
    secp256k1_sha256 sha;
    unsigned char rand[32];
    unsigned char i;
    unsigned char msg_present;

    if (seckey32 != NULL) {
        secp256k1_nonce_function_musig_sha256_tagged_aux(&sha);
        secp256k1_sha256_write(&sha, session_secrand, 32);
        secp256k1_sha256_finalize(&sha, rand);
        for (i = 0; i < 32; i++) {
            rand[i] ^= seckey32[i];
        }
    } else {
        memcpy(rand, session_secrand, sizeof(rand));
    }

    secp256k1_nonce_function_musig_sha256_tagged(&sha);
    secp256k1_sha256_write(&sha, rand, sizeof(rand));
    secp256k1_nonce_function_musig_helper(&sha, 1, pk33, 33);
    secp256k1_nonce_function_musig_helper(&sha, 1, agg_pk32, 32);
    msg_present = msg32 != NULL;
    secp256k1_sha256_write(&sha, &msg_present, 1);
    if (msg_present) {
        secp256k1_nonce_function_musig_helper(&sha, 8, msg32, 32);
    }
    secp256k1_nonce_function_musig_helper(&sha, 4, extra_input32, 32);

    for (i = 0; i < 2; i++) {
        unsigned char buf[32];
        secp256k1_sha256 sha_tmp = sha;
        secp256k1_sha256_write(&sha_tmp, &i, 1);
        secp256k1_sha256_finalize(&sha_tmp, buf);
        secp256k1_scalar_set_b32(&k[i], buf, NULL);

        /* Attempt to erase secret data */
        secp256k1_memclear(buf, sizeof(buf));
        secp256k1_sha256_clear(&sha_tmp);
    }
    secp256k1_memclear(rand, sizeof(rand));
    secp256k1_sha256_clear(&sha);
}

static int secp256k1_musig_nonce_gen_internal(const secp256k1_context* ctx, secp256k1_musig_secnonce *secnonce, secp256k1_musig_pubnonce *pubnonce, const unsigned char *input_nonce, const unsigned char *seckey, const secp256k1_pubkey *pubkey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32) {
    secp256k1_scalar k[2];
    secp256k1_ge nonce_pts[2];
    int i;
    unsigned char pk_ser[33];
    size_t pk_ser_len = sizeof(pk_ser);
    unsigned char aggpk_ser[32];
    unsigned char *aggpk_ser_ptr = NULL;
    secp256k1_ge pk;
    int pk_serialize_success;
    int ret = 1;

    ARG_CHECK(pubnonce != NULL);
    memset(pubnonce, 0, sizeof(*pubnonce));
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));

    /* Check that the seckey is valid to be able to sign for it later. */
    if (seckey != NULL) {
        secp256k1_scalar sk;
        ret &= secp256k1_scalar_set_b32_seckey(&sk, seckey);
        secp256k1_scalar_clear(&sk);
    }

    if (keyagg_cache != NULL) {
        secp256k1_keyagg_cache_internal cache_i;
        if (!secp256k1_keyagg_cache_load(ctx, &cache_i, keyagg_cache)) {
            return 0;
        }
        /* The loaded point cache_i.pk can not be the point at infinity. */
        secp256k1_fe_get_b32(aggpk_ser, &cache_i.pk.x);
        aggpk_ser_ptr = aggpk_ser;
    }
    if (!secp256k1_pubkey_load(ctx, &pk, pubkey)) {
        return 0;
    }
    pk_serialize_success = secp256k1_eckey_pubkey_serialize(&pk, pk_ser, &pk_ser_len, 1);

#ifdef VERIFY
    /* A pubkey cannot be the point at infinity */
    VERIFY_CHECK(pk_serialize_success);
    VERIFY_CHECK(pk_ser_len == sizeof(pk_ser));
#else
    (void) pk_serialize_success;
#endif

    secp256k1_nonce_function_musig(k, input_nonce, msg32, seckey, pk_ser, aggpk_ser_ptr, extra_input32);
    VERIFY_CHECK(!secp256k1_scalar_is_zero(&k[0]));
    VERIFY_CHECK(!secp256k1_scalar_is_zero(&k[1]));
    secp256k1_musig_secnonce_save(secnonce, k, &pk);
    secp256k1_musig_secnonce_invalidate(ctx, secnonce, !ret);

    for (i = 0; i < 2; i++) {
        secp256k1_gej nonce_ptj;
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &nonce_ptj, &k[i]);
        secp256k1_ge_set_gej(&nonce_pts[i], &nonce_ptj);
        secp256k1_declassify(ctx, &nonce_pts[i], sizeof(nonce_pts[i]));
        secp256k1_scalar_clear(&k[i]);
        secp256k1_gej_clear(&nonce_ptj);
    }
    /* None of the nonce_pts will be infinity because k != 0 with overwhelming
     * probability */
    secp256k1_musig_pubnonce_save(pubnonce, nonce_pts);
    return ret;
}

int secp256k1_musig_nonce_gen(const secp256k1_context* ctx, secp256k1_musig_secnonce *secnonce, secp256k1_musig_pubnonce *pubnonce, unsigned char *session_secrand32, const unsigned char *seckey, const secp256k1_pubkey *pubkey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32) {
    int ret = 1;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secnonce != NULL);
    memset(secnonce, 0, sizeof(*secnonce));
    ARG_CHECK(session_secrand32 != NULL);

    /* Check in constant time that the session_secrand32 is not 0 as a
     * defense-in-depth measure that may protect against a faulty RNG. */
    ret &= !secp256k1_is_zero_array(session_secrand32, 32);

    /* We can declassify because branching on ret is only relevant when this
     * function called with an invalid session_secrand32 argument */
    secp256k1_declassify(ctx, &ret, sizeof(ret));
    if (ret == 0) {
        secp256k1_musig_secnonce_invalidate(ctx, secnonce, 1);
        return 0;
    }

    ret &= secp256k1_musig_nonce_gen_internal(ctx, secnonce, pubnonce, session_secrand32, seckey, pubkey, msg32, keyagg_cache, extra_input32);

    /* Set the session_secrand32 buffer to zero to prevent the caller from using
     * nonce_gen multiple times with the same buffer. */
    secp256k1_memczero(session_secrand32, 32, ret);
    return ret;
}

int secp256k1_musig_nonce_gen_counter(const secp256k1_context* ctx, secp256k1_musig_secnonce *secnonce, secp256k1_musig_pubnonce *pubnonce, uint64_t nonrepeating_cnt, const secp256k1_keypair *keypair, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32) {
    unsigned char buf[32] = { 0 };
    unsigned char seckey[32];
    secp256k1_pubkey pubkey;
    int ret;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secnonce != NULL);
    memset(secnonce, 0, sizeof(*secnonce));
    ARG_CHECK(keypair != NULL);

    secp256k1_write_be64(buf, nonrepeating_cnt);
    /* keypair_sec and keypair_pub do not fail if the arguments are not NULL */
    ret = secp256k1_keypair_sec(ctx, seckey, keypair);
    VERIFY_CHECK(ret);
    ret = secp256k1_keypair_pub(ctx, &pubkey, keypair);
    VERIFY_CHECK(ret);
#ifndef VERIFY
    (void) ret;
#endif

    if (!secp256k1_musig_nonce_gen_internal(ctx, secnonce, pubnonce, buf, seckey, &pubkey, msg32, keyagg_cache, extra_input32)) {
        return 0;
    }
    secp256k1_memclear(seckey, sizeof(seckey));
    return 1;
}

static int secp256k1_musig_sum_pubnonces(const secp256k1_context* ctx, secp256k1_gej *summed_pubnonces, const secp256k1_musig_pubnonce * const* pubnonces, size_t n_pubnonces) {
    size_t i;
    int j;

    secp256k1_gej_set_infinity(&summed_pubnonces[0]);
    secp256k1_gej_set_infinity(&summed_pubnonces[1]);

    for (i = 0; i < n_pubnonces; i++) {
        secp256k1_ge nonce_pts[2];
        if (!secp256k1_musig_pubnonce_load(ctx, nonce_pts, pubnonces[i])) {
            return 0;
        }
        for (j = 0; j < 2; j++) {
            secp256k1_gej_add_ge_var(&summed_pubnonces[j], &summed_pubnonces[j], &nonce_pts[j], NULL);
        }
    }
    return 1;
}

int secp256k1_musig_nonce_agg(const secp256k1_context* ctx, secp256k1_musig_aggnonce  *aggnonce, const secp256k1_musig_pubnonce * const* pubnonces, size_t n_pubnonces) {
    secp256k1_gej aggnonce_ptsj[2];
    secp256k1_ge aggnonce_pts[2];
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(aggnonce != NULL);
    ARG_CHECK(pubnonces != NULL);
    ARG_CHECK(n_pubnonces > 0);

    if (!secp256k1_musig_sum_pubnonces(ctx, aggnonce_ptsj, pubnonces, n_pubnonces)) {
        return 0;
    }
    secp256k1_ge_set_all_gej_var(aggnonce_pts, aggnonce_ptsj, 2);
    secp256k1_musig_aggnonce_save(aggnonce, aggnonce_pts);
    return 1;
}

/* Initializes SHA256 with fixed midstate. This midstate was computed by applying
 * SHA256 to SHA256("MuSig/noncecoef")||SHA256("MuSig/noncecoef"). */
static void secp256k1_musig_compute_noncehash_sha256_tagged(secp256k1_sha256 *sha) {
    secp256k1_sha256_initialize(sha);
    sha->s[0] = 0x2c7d5a45ul;
    sha->s[1] = 0x06bf7e53ul;
    sha->s[2] = 0x89be68a6ul;
    sha->s[3] = 0x971254c0ul;
    sha->s[4] = 0x60ac12d2ul;
    sha->s[5] = 0x72846dcdul;
    sha->s[6] = 0x6c81212ful;
    sha->s[7] = 0xde7a2500ul;
    sha->bytes = 64;
}

/* tagged_hash(aggnonce[0], aggnonce[1], agg_pk, msg) */
static void secp256k1_musig_compute_noncehash(unsigned char *noncehash, secp256k1_ge *aggnonce, const unsigned char *agg_pk32, const unsigned char *msg) {
    unsigned char buf[33];
    secp256k1_sha256 sha;
    int i;

    secp256k1_musig_compute_noncehash_sha256_tagged(&sha);
    for (i = 0; i < 2; i++) {
        secp256k1_musig_ge_serialize_ext(buf, &aggnonce[i]);
        secp256k1_sha256_write(&sha, buf, sizeof(buf));
    }
    secp256k1_sha256_write(&sha, agg_pk32, 32);
    secp256k1_sha256_write(&sha, msg, 32);
    secp256k1_sha256_finalize(&sha, noncehash);
}

/* out_nonce = nonce_pts[0] + b*nonce_pts[1] */
static void secp256k1_effective_nonce(secp256k1_gej *out_nonce, const secp256k1_ge *nonce_pts, const secp256k1_scalar *b) {
    secp256k1_gej tmp;

    secp256k1_gej_set_ge(&tmp, &nonce_pts[1]);
    secp256k1_ecmult(out_nonce, &tmp, b, NULL);
    secp256k1_gej_add_ge_var(out_nonce, out_nonce, &nonce_pts[0], NULL);
}

static void secp256k1_musig_nonce_process_internal(int *fin_nonce_parity, unsigned char *fin_nonce, secp256k1_scalar *b, secp256k1_ge *aggnonce_pts, const unsigned char *agg_pk32, const unsigned char *msg) {
    unsigned char noncehash[32];
    secp256k1_ge fin_nonce_pt;
    secp256k1_gej fin_nonce_ptj;

    secp256k1_musig_compute_noncehash(noncehash, aggnonce_pts, agg_pk32, msg);
    secp256k1_scalar_set_b32(b, noncehash, NULL);
    /* fin_nonce = aggnonce_pts[0] + b*aggnonce_pts[1] */
    secp256k1_effective_nonce(&fin_nonce_ptj, aggnonce_pts, b);
    secp256k1_ge_set_gej(&fin_nonce_pt, &fin_nonce_ptj);
    if (secp256k1_ge_is_infinity(&fin_nonce_pt)) {
        fin_nonce_pt = secp256k1_ge_const_g;
    }
    /* fin_nonce_pt is not the point at infinity */
    secp256k1_fe_normalize_var(&fin_nonce_pt.x);
    secp256k1_fe_get_b32(fin_nonce, &fin_nonce_pt.x);
    secp256k1_fe_normalize_var(&fin_nonce_pt.y);
    *fin_nonce_parity = secp256k1_fe_is_odd(&fin_nonce_pt.y);
}

int secp256k1_musig_nonce_process(const secp256k1_context* ctx, secp256k1_musig_session *session, const secp256k1_musig_aggnonce  *aggnonce, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache) {
    secp256k1_keyagg_cache_internal cache_i;
    secp256k1_ge aggnonce_pts[2];
    unsigned char fin_nonce[32];
    secp256k1_musig_session_internal session_i;
    unsigned char agg_pk32[32];

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(session != NULL);
    ARG_CHECK(aggnonce != NULL);
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(keyagg_cache != NULL);

    if (!secp256k1_keyagg_cache_load(ctx, &cache_i, keyagg_cache)) {
        return 0;
    }
    secp256k1_fe_get_b32(agg_pk32, &cache_i.pk.x);

    if (!secp256k1_musig_aggnonce_load(ctx, aggnonce_pts, aggnonce)) {
        return 0;
    }

    secp256k1_musig_nonce_process_internal(&session_i.fin_nonce_parity, fin_nonce, &session_i.noncecoef, aggnonce_pts, agg_pk32, msg32);
    secp256k1_schnorrsig_challenge(&session_i.challenge, fin_nonce, msg32, 32, agg_pk32);

    /* If there is a tweak then set `challenge` times `tweak` to the `s`-part.*/
    secp256k1_scalar_set_int(&session_i.s_part, 0);
    if (!secp256k1_scalar_is_zero(&cache_i.tweak)) {
        secp256k1_scalar e_tmp;
        secp256k1_scalar_mul(&e_tmp, &session_i.challenge, &cache_i.tweak);
        if (secp256k1_fe_is_odd(&cache_i.pk.y)) {
            secp256k1_scalar_negate(&e_tmp, &e_tmp);
        }
        session_i.s_part = e_tmp;
    }
    memcpy(session_i.fin_nonce, fin_nonce, sizeof(session_i.fin_nonce));
    secp256k1_musig_session_save(session, &session_i);
    return 1;
}

static void secp256k1_musig_partial_sign_clear(secp256k1_scalar *sk, secp256k1_scalar *k) {
    secp256k1_scalar_clear(sk);
    secp256k1_scalar_clear(&k[0]);
    secp256k1_scalar_clear(&k[1]);
}

int secp256k1_musig_partial_sign(const secp256k1_context* ctx, secp256k1_musig_partial_sig *partial_sig, secp256k1_musig_secnonce *secnonce, const secp256k1_keypair *keypair, const secp256k1_musig_keyagg_cache *keyagg_cache, const secp256k1_musig_session *session) {
    secp256k1_scalar sk;
    secp256k1_ge pk, keypair_pk;
    secp256k1_scalar k[2];
    secp256k1_scalar mu, s;
    secp256k1_keyagg_cache_internal cache_i;
    secp256k1_musig_session_internal session_i;
    int ret;

    VERIFY_CHECK(ctx != NULL);

    ARG_CHECK(secnonce != NULL);
    /* Fails if the magic doesn't match */
    ret = secp256k1_musig_secnonce_load(ctx, k, &pk, secnonce);
    /* Set nonce to zero to avoid nonce reuse. This will cause subsequent calls
     * of this function to fail */
    memset(secnonce, 0, sizeof(*secnonce));
    if (!ret) {
        secp256k1_musig_partial_sign_clear(&sk, k);
        return 0;
    }

    ARG_CHECK(partial_sig != NULL);
    ARG_CHECK(keypair != NULL);
    ARG_CHECK(keyagg_cache != NULL);
    ARG_CHECK(session != NULL);

    if (!secp256k1_keypair_load(ctx, &sk, &keypair_pk, keypair)) {
        secp256k1_musig_partial_sign_clear(&sk, k);
        return 0;
    }
    ARG_CHECK(secp256k1_fe_equal(&pk.x, &keypair_pk.x)
              && secp256k1_fe_equal(&pk.y, &keypair_pk.y));
    if (!secp256k1_keyagg_cache_load(ctx, &cache_i, keyagg_cache)) {
        secp256k1_musig_partial_sign_clear(&sk, k);
        return 0;
    }

    /* Negate sk if secp256k1_fe_is_odd(&cache_i.pk.y)) XOR cache_i.parity_acc.
     * This corresponds to the line "Let d = g⋅gacc⋅d' mod n" in the
     * specification. */
    if ((secp256k1_fe_is_odd(&cache_i.pk.y)
         != cache_i.parity_acc)) {
        secp256k1_scalar_negate(&sk, &sk);
    }

    /* Multiply KeyAgg coefficient */
    secp256k1_musig_keyaggcoef(&mu, &cache_i, &pk);
    secp256k1_scalar_mul(&sk, &sk, &mu);

    if (!secp256k1_musig_session_load(ctx, &session_i, session)) {
        secp256k1_musig_partial_sign_clear(&sk, k);
        return 0;
    }

    if (session_i.fin_nonce_parity) {
        secp256k1_scalar_negate(&k[0], &k[0]);
        secp256k1_scalar_negate(&k[1], &k[1]);
    }

    /* Sign */
    secp256k1_scalar_mul(&s, &session_i.challenge, &sk);
    secp256k1_scalar_mul(&k[1], &session_i.noncecoef, &k[1]);
    secp256k1_scalar_add(&k[0], &k[0], &k[1]);
    secp256k1_scalar_add(&s, &s, &k[0]);
    secp256k1_musig_partial_sig_save(partial_sig, &s);
    secp256k1_musig_partial_sign_clear(&sk, k);
    return 1;
}

int secp256k1_musig_partial_sig_verify(const secp256k1_context* ctx, const secp256k1_musig_partial_sig *partial_sig, const secp256k1_musig_pubnonce *pubnonce, const secp256k1_pubkey *pubkey, const secp256k1_musig_keyagg_cache *keyagg_cache, const secp256k1_musig_session *session) {
    secp256k1_keyagg_cache_internal cache_i;
    secp256k1_musig_session_internal session_i;
    secp256k1_scalar mu, e, s;
    secp256k1_gej pkj;
    secp256k1_ge nonce_pts[2];
    secp256k1_gej rj;
    secp256k1_gej tmp;
    secp256k1_ge pkp;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(partial_sig != NULL);
    ARG_CHECK(pubnonce != NULL);
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(keyagg_cache != NULL);
    ARG_CHECK(session != NULL);

    if (!secp256k1_musig_session_load(ctx, &session_i, session)) {
        return 0;
    }

    if (!secp256k1_musig_pubnonce_load(ctx, nonce_pts, pubnonce)) {
        return 0;
    }
    /* Compute "effective" nonce rj = nonce_pts[0] + b*nonce_pts[1] */
    /* TODO: use multiexp to compute -s*G + e*mu*pubkey + nonce_pts[0] + b*nonce_pts[1] */
    secp256k1_effective_nonce(&rj, nonce_pts, &session_i.noncecoef);

    if (!secp256k1_pubkey_load(ctx, &pkp, pubkey)) {
        return 0;
    }
    if (!secp256k1_keyagg_cache_load(ctx, &cache_i, keyagg_cache)) {
        return 0;
    }
    /* Multiplying the challenge by the KeyAgg coefficient is equivalent
     * to multiplying the signer's public key by the coefficient, except
     * much easier to do. */
    secp256k1_musig_keyaggcoef(&mu, &cache_i, &pkp);
    secp256k1_scalar_mul(&e, &session_i.challenge, &mu);

    /* Negate e if secp256k1_fe_is_odd(&cache_i.pk.y)) XOR cache_i.parity_acc.
     * This corresponds to the line "Let g' = g⋅gacc mod n" and the multiplication "g'⋅e"
     * in the specification. */
    if (secp256k1_fe_is_odd(&cache_i.pk.y)
            != cache_i.parity_acc) {
        secp256k1_scalar_negate(&e, &e);
    }

    if (!secp256k1_musig_partial_sig_load(ctx, &s, partial_sig)) {
        return 0;
    }
    /* Compute -s*G + e*pkj + rj (e already includes the keyagg coefficient mu) */
    secp256k1_scalar_negate(&s, &s);
    secp256k1_gej_set_ge(&pkj, &pkp);
    secp256k1_ecmult(&tmp, &pkj, &e, &s);
    if (session_i.fin_nonce_parity) {
        secp256k1_gej_neg(&rj, &rj);
    }
    secp256k1_gej_add_var(&tmp, &tmp, &rj, NULL);

    return secp256k1_gej_is_infinity(&tmp);
}

int secp256k1_musig_partial_sig_agg(const secp256k1_context* ctx, unsigned char *sig64, const secp256k1_musig_session *session, const secp256k1_musig_partial_sig * const* partial_sigs, size_t n_sigs) {
    size_t i;
    secp256k1_musig_session_internal session_i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(sig64 != NULL);
    ARG_CHECK(session != NULL);
    ARG_CHECK(partial_sigs != NULL);
    ARG_CHECK(n_sigs > 0);

    if (!secp256k1_musig_session_load(ctx, &session_i, session)) {
        return 0;
    }
    for (i = 0; i < n_sigs; i++) {
        secp256k1_scalar term;
        if (!secp256k1_musig_partial_sig_load(ctx, &term, partial_sigs[i])) {
            return 0;
        }
        secp256k1_scalar_add(&session_i.s_part, &session_i.s_part, &term);
    }
    secp256k1_scalar_get_b32(&sig64[32], &session_i.s_part);
    memcpy(&sig64[0], session_i.fin_nonce, 32);
    return 1;
}

#endif
