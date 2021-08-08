/***********************************************************************
 * Copyright (c) 2013, 2014, 2015 Pieter Wuille, Gregory Maxwell       *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include "secp256k1.c"
#include "../include/secp256k1.h"
#include "../include/secp256k1_preallocated.h"
#include "testrand_impl.h"
#include "util.h"

#ifdef ENABLE_OPENSSL_TESTS
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
# if OPENSSL_VERSION_NUMBER < 0x10100000L
void ECDSA_SIG_get0(const ECDSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps) {*pr = sig->r; *ps = sig->s;}
# endif
#endif

#include "../contrib/lax_der_parsing.c"
#include "../contrib/lax_der_privatekey_parsing.c"

#include "modinv32_impl.h"
#ifdef SECP256K1_WIDEMUL_INT128
#include "modinv64_impl.h"
#endif

static int count = 64;
static secp256k1_context *ctx = NULL;

static void counting_illegal_callback_fn(const char* str, void* data) {
    /* Dummy callback function that just counts. */
    int32_t *p;
    (void)str;
    p = data;
    (*p)++;
}

static void uncounting_illegal_callback_fn(const char* str, void* data) {
    /* Dummy callback function that just counts (backwards). */
    int32_t *p;
    (void)str;
    p = data;
    (*p)--;
}

void random_field_element_test(secp256k1_fe *fe) {
    do {
        unsigned char b32[32];
        secp256k1_testrand256_test(b32);
        if (secp256k1_fe_set_b32(fe, b32)) {
            break;
        }
    } while(1);
}

void random_field_element_magnitude(secp256k1_fe *fe) {
    secp256k1_fe zero;
    int n = secp256k1_testrand_int(9);
    secp256k1_fe_normalize(fe);
    if (n == 0) {
        return;
    }
    secp256k1_fe_clear(&zero);
    secp256k1_fe_negate(&zero, &zero, 0);
    secp256k1_fe_mul_int(&zero, n - 1);
    secp256k1_fe_add(fe, &zero);
#ifdef VERIFY
    CHECK(fe->magnitude == n);
#endif
}

void random_group_element_test(secp256k1_ge *ge) {
    secp256k1_fe fe;
    do {
        random_field_element_test(&fe);
        if (secp256k1_ge_set_xo_var(ge, &fe, secp256k1_testrand_bits(1))) {
            secp256k1_fe_normalize(&ge->y);
            break;
        }
    } while(1);
    ge->infinity = 0;
}

void random_group_element_jacobian_test(secp256k1_gej *gej, const secp256k1_ge *ge) {
    secp256k1_fe z2, z3;
    do {
        random_field_element_test(&gej->z);
        if (!secp256k1_fe_is_zero(&gej->z)) {
            break;
        }
    } while(1);
    secp256k1_fe_sqr(&z2, &gej->z);
    secp256k1_fe_mul(&z3, &z2, &gej->z);
    secp256k1_fe_mul(&gej->x, &ge->x, &z2);
    secp256k1_fe_mul(&gej->y, &ge->y, &z3);
    gej->infinity = ge->infinity;
}

void random_scalar_order_test(secp256k1_scalar *num) {
    do {
        unsigned char b32[32];
        int overflow = 0;
        secp256k1_testrand256_test(b32);
        secp256k1_scalar_set_b32(num, b32, &overflow);
        if (overflow || secp256k1_scalar_is_zero(num)) {
            continue;
        }
        break;
    } while(1);
}

void random_scalar_order(secp256k1_scalar *num) {
    do {
        unsigned char b32[32];
        int overflow = 0;
        secp256k1_testrand256(b32);
        secp256k1_scalar_set_b32(num, b32, &overflow);
        if (overflow || secp256k1_scalar_is_zero(num)) {
            continue;
        }
        break;
    } while(1);
}

void random_scalar_order_b32(unsigned char *b32) {
    secp256k1_scalar num;
    random_scalar_order(&num);
    secp256k1_scalar_get_b32(b32, &num);
}

void run_context_tests(int use_prealloc) {
    secp256k1_pubkey pubkey;
    secp256k1_pubkey zero_pubkey;
    secp256k1_ecdsa_signature sig;
    unsigned char ctmp[32];
    int32_t ecount;
    int32_t ecount2;
    secp256k1_context *none;
    secp256k1_context *sign;
    secp256k1_context *vrfy;
    secp256k1_context *both;
    void *none_prealloc = NULL;
    void *sign_prealloc = NULL;
    void *vrfy_prealloc = NULL;
    void *both_prealloc = NULL;

    secp256k1_gej pubj;
    secp256k1_ge pub;
    secp256k1_scalar msg, key, nonce;
    secp256k1_scalar sigr, sigs;

    if (use_prealloc) {
        none_prealloc = malloc(secp256k1_context_preallocated_size(SECP256K1_CONTEXT_NONE));
        sign_prealloc = malloc(secp256k1_context_preallocated_size(SECP256K1_CONTEXT_SIGN));
        vrfy_prealloc = malloc(secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY));
        both_prealloc = malloc(secp256k1_context_preallocated_size(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY));
        CHECK(none_prealloc != NULL);
        CHECK(sign_prealloc != NULL);
        CHECK(vrfy_prealloc != NULL);
        CHECK(both_prealloc != NULL);
        none = secp256k1_context_preallocated_create(none_prealloc, SECP256K1_CONTEXT_NONE);
        sign = secp256k1_context_preallocated_create(sign_prealloc, SECP256K1_CONTEXT_SIGN);
        vrfy = secp256k1_context_preallocated_create(vrfy_prealloc, SECP256K1_CONTEXT_VERIFY);
        both = secp256k1_context_preallocated_create(both_prealloc, SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    } else {
        none = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
        sign = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
        vrfy = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
        both = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    }

    memset(&zero_pubkey, 0, sizeof(zero_pubkey));

    ecount = 0;
    ecount2 = 10;
    secp256k1_context_set_illegal_callback(vrfy, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(sign, counting_illegal_callback_fn, &ecount2);
    /* set error callback (to a function that still aborts in case malloc() fails in secp256k1_context_clone() below) */
    secp256k1_context_set_error_callback(sign, secp256k1_default_illegal_callback_fn, NULL);
    CHECK(sign->error_callback.fn != vrfy->error_callback.fn);
    CHECK(sign->error_callback.fn == secp256k1_default_illegal_callback_fn);

    /* check if sizes for cloning are consistent */
    CHECK(secp256k1_context_preallocated_clone_size(none) == secp256k1_context_preallocated_size(SECP256K1_CONTEXT_NONE));
    CHECK(secp256k1_context_preallocated_clone_size(sign) == secp256k1_context_preallocated_size(SECP256K1_CONTEXT_SIGN));
    CHECK(secp256k1_context_preallocated_clone_size(vrfy) == secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY));
    CHECK(secp256k1_context_preallocated_clone_size(both) == secp256k1_context_preallocated_size(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY));

    /*** clone and destroy all of them to make sure cloning was complete ***/
    {
        secp256k1_context *ctx_tmp;

        if (use_prealloc) {
            /* clone into a non-preallocated context and then again into a new preallocated one. */
            ctx_tmp = none; none = secp256k1_context_clone(none); secp256k1_context_preallocated_destroy(ctx_tmp);
            free(none_prealloc); none_prealloc = malloc(secp256k1_context_preallocated_size(SECP256K1_CONTEXT_NONE)); CHECK(none_prealloc != NULL);
            ctx_tmp = none; none = secp256k1_context_preallocated_clone(none, none_prealloc); secp256k1_context_destroy(ctx_tmp);

            ctx_tmp = sign; sign = secp256k1_context_clone(sign); secp256k1_context_preallocated_destroy(ctx_tmp);
            free(sign_prealloc); sign_prealloc = malloc(secp256k1_context_preallocated_size(SECP256K1_CONTEXT_SIGN)); CHECK(sign_prealloc != NULL);
            ctx_tmp = sign; sign = secp256k1_context_preallocated_clone(sign, sign_prealloc); secp256k1_context_destroy(ctx_tmp);

            ctx_tmp = vrfy; vrfy = secp256k1_context_clone(vrfy); secp256k1_context_preallocated_destroy(ctx_tmp);
            free(vrfy_prealloc); vrfy_prealloc = malloc(secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY)); CHECK(vrfy_prealloc != NULL);
            ctx_tmp = vrfy; vrfy = secp256k1_context_preallocated_clone(vrfy, vrfy_prealloc); secp256k1_context_destroy(ctx_tmp);

            ctx_tmp = both; both = secp256k1_context_clone(both); secp256k1_context_preallocated_destroy(ctx_tmp);
            free(both_prealloc); both_prealloc = malloc(secp256k1_context_preallocated_size(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY)); CHECK(both_prealloc != NULL);
            ctx_tmp = both; both = secp256k1_context_preallocated_clone(both, both_prealloc); secp256k1_context_destroy(ctx_tmp);
        } else {
            /* clone into a preallocated context and then again into a new non-preallocated one. */
            void *prealloc_tmp;

            prealloc_tmp = malloc(secp256k1_context_preallocated_size(SECP256K1_CONTEXT_NONE)); CHECK(prealloc_tmp != NULL);
            ctx_tmp = none; none = secp256k1_context_preallocated_clone(none, prealloc_tmp); secp256k1_context_destroy(ctx_tmp);
            ctx_tmp = none; none = secp256k1_context_clone(none); secp256k1_context_preallocated_destroy(ctx_tmp);
            free(prealloc_tmp);

            prealloc_tmp = malloc(secp256k1_context_preallocated_size(SECP256K1_CONTEXT_SIGN)); CHECK(prealloc_tmp != NULL);
            ctx_tmp = sign; sign = secp256k1_context_preallocated_clone(sign, prealloc_tmp); secp256k1_context_destroy(ctx_tmp);
            ctx_tmp = sign; sign = secp256k1_context_clone(sign); secp256k1_context_preallocated_destroy(ctx_tmp);
            free(prealloc_tmp);

            prealloc_tmp = malloc(secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY)); CHECK(prealloc_tmp != NULL);
            ctx_tmp = vrfy; vrfy = secp256k1_context_preallocated_clone(vrfy, prealloc_tmp); secp256k1_context_destroy(ctx_tmp);
            ctx_tmp = vrfy; vrfy = secp256k1_context_clone(vrfy); secp256k1_context_preallocated_destroy(ctx_tmp);
            free(prealloc_tmp);

            prealloc_tmp = malloc(secp256k1_context_preallocated_size(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY)); CHECK(prealloc_tmp != NULL);
            ctx_tmp = both; both = secp256k1_context_preallocated_clone(both, prealloc_tmp); secp256k1_context_destroy(ctx_tmp);
            ctx_tmp = both; both = secp256k1_context_clone(both); secp256k1_context_preallocated_destroy(ctx_tmp);
            free(prealloc_tmp);
        }
    }

    /* Verify that the error callback makes it across the clone. */
    CHECK(sign->error_callback.fn != vrfy->error_callback.fn);
    CHECK(sign->error_callback.fn == secp256k1_default_illegal_callback_fn);
    /* And that it resets back to default. */
    secp256k1_context_set_error_callback(sign, NULL, NULL);
    CHECK(vrfy->error_callback.fn == sign->error_callback.fn);

    /*** attempt to use them ***/
    random_scalar_order_test(&msg);
    random_scalar_order_test(&key);
    secp256k1_ecmult_gen(&both->ecmult_gen_ctx, &pubj, &key);
    secp256k1_ge_set_gej(&pub, &pubj);

    /* Verify context-type checking illegal-argument errors. */
    memset(ctmp, 1, 32);
    CHECK(secp256k1_ec_pubkey_create(vrfy, &pubkey, ctmp) == 0);
    CHECK(ecount == 1);
    VG_UNDEF(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_create(sign, &pubkey, ctmp) == 1);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ecdsa_sign(vrfy, &sig, ctmp, ctmp, NULL, NULL) == 0);
    CHECK(ecount == 2);
    VG_UNDEF(&sig, sizeof(sig));
    CHECK(secp256k1_ecdsa_sign(sign, &sig, ctmp, ctmp, NULL, NULL) == 1);
    VG_CHECK(&sig, sizeof(sig));
    CHECK(ecount2 == 10);
    CHECK(secp256k1_ecdsa_verify(sign, &sig, ctmp, &pubkey) == 0);
    CHECK(ecount2 == 11);
    CHECK(secp256k1_ecdsa_verify(vrfy, &sig, ctmp, &pubkey) == 1);
    CHECK(ecount == 2);
    CHECK(secp256k1_ec_pubkey_tweak_add(sign, &pubkey, ctmp) == 0);
    CHECK(ecount2 == 12);
    CHECK(secp256k1_ec_pubkey_tweak_add(vrfy, &pubkey, ctmp) == 1);
    CHECK(ecount == 2);
    CHECK(secp256k1_ec_pubkey_tweak_mul(sign, &pubkey, ctmp) == 0);
    CHECK(ecount2 == 13);
    CHECK(secp256k1_ec_pubkey_negate(vrfy, &pubkey) == 1);
    CHECK(ecount == 2);
    CHECK(secp256k1_ec_pubkey_negate(sign, &pubkey) == 1);
    CHECK(ecount == 2);
    CHECK(secp256k1_ec_pubkey_negate(sign, NULL) == 0);
    CHECK(ecount2 == 14);
    CHECK(secp256k1_ec_pubkey_negate(vrfy, &zero_pubkey) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_ec_pubkey_tweak_mul(vrfy, &pubkey, ctmp) == 1);
    CHECK(ecount == 3);
    CHECK(secp256k1_context_randomize(vrfy, ctmp) == 1);
    CHECK(ecount == 3);
    CHECK(secp256k1_context_randomize(vrfy, NULL) == 1);
    CHECK(ecount == 3);
    CHECK(secp256k1_context_randomize(sign, ctmp) == 1);
    CHECK(ecount2 == 14);
    CHECK(secp256k1_context_randomize(sign, NULL) == 1);
    CHECK(ecount2 == 14);
    secp256k1_context_set_illegal_callback(vrfy, NULL, NULL);
    secp256k1_context_set_illegal_callback(sign, NULL, NULL);

    /* obtain a working nonce */
    do {
        random_scalar_order_test(&nonce);
    } while(!secp256k1_ecdsa_sig_sign(&both->ecmult_gen_ctx, &sigr, &sigs, &key, &msg, &nonce, NULL));

    /* try signing */
    CHECK(secp256k1_ecdsa_sig_sign(&sign->ecmult_gen_ctx, &sigr, &sigs, &key, &msg, &nonce, NULL));
    CHECK(secp256k1_ecdsa_sig_sign(&both->ecmult_gen_ctx, &sigr, &sigs, &key, &msg, &nonce, NULL));

    /* try verifying */
    CHECK(secp256k1_ecdsa_sig_verify(&vrfy->ecmult_ctx, &sigr, &sigs, &pub, &msg));
    CHECK(secp256k1_ecdsa_sig_verify(&both->ecmult_ctx, &sigr, &sigs, &pub, &msg));

    /* cleanup */
    if (use_prealloc) {
        secp256k1_context_preallocated_destroy(none);
        secp256k1_context_preallocated_destroy(sign);
        secp256k1_context_preallocated_destroy(vrfy);
        secp256k1_context_preallocated_destroy(both);
        free(none_prealloc);
        free(sign_prealloc);
        free(vrfy_prealloc);
        free(both_prealloc);
    } else {
        secp256k1_context_destroy(none);
        secp256k1_context_destroy(sign);
        secp256k1_context_destroy(vrfy);
        secp256k1_context_destroy(both);
    }
    /* Defined as no-op. */
    secp256k1_context_destroy(NULL);
    secp256k1_context_preallocated_destroy(NULL);

}

void run_scratch_tests(void) {
    const size_t adj_alloc = ((500 + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT;

    int32_t ecount = 0;
    size_t checkpoint;
    size_t checkpoint_2;
    secp256k1_context *none = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    secp256k1_scratch_space *scratch;
    secp256k1_scratch_space local_scratch;

    /* Test public API */
    secp256k1_context_set_illegal_callback(none, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(none, counting_illegal_callback_fn, &ecount);

    scratch = secp256k1_scratch_space_create(none, 1000);
    CHECK(scratch != NULL);
    CHECK(ecount == 0);

    /* Test internal API */
    CHECK(secp256k1_scratch_max_allocation(&none->error_callback, scratch, 0) == 1000);
    CHECK(secp256k1_scratch_max_allocation(&none->error_callback, scratch, 1) == 1000 - (ALIGNMENT - 1));
    CHECK(scratch->alloc_size == 0);
    CHECK(scratch->alloc_size % ALIGNMENT == 0);

    /* Allocating 500 bytes succeeds */
    checkpoint = secp256k1_scratch_checkpoint(&none->error_callback, scratch);
    CHECK(secp256k1_scratch_alloc(&none->error_callback, scratch, 500) != NULL);
    CHECK(secp256k1_scratch_max_allocation(&none->error_callback, scratch, 0) == 1000 - adj_alloc);
    CHECK(secp256k1_scratch_max_allocation(&none->error_callback, scratch, 1) == 1000 - adj_alloc - (ALIGNMENT - 1));
    CHECK(scratch->alloc_size != 0);
    CHECK(scratch->alloc_size % ALIGNMENT == 0);

    /* Allocating another 501 bytes fails */
    CHECK(secp256k1_scratch_alloc(&none->error_callback, scratch, 501) == NULL);
    CHECK(secp256k1_scratch_max_allocation(&none->error_callback, scratch, 0) == 1000 - adj_alloc);
    CHECK(secp256k1_scratch_max_allocation(&none->error_callback, scratch, 1) == 1000 - adj_alloc - (ALIGNMENT - 1));
    CHECK(scratch->alloc_size != 0);
    CHECK(scratch->alloc_size % ALIGNMENT == 0);

    /* ...but it succeeds once we apply the checkpoint to undo it */
    secp256k1_scratch_apply_checkpoint(&none->error_callback, scratch, checkpoint);
    CHECK(scratch->alloc_size == 0);
    CHECK(secp256k1_scratch_max_allocation(&none->error_callback, scratch, 0) == 1000);
    CHECK(secp256k1_scratch_alloc(&none->error_callback, scratch, 500) != NULL);
    CHECK(scratch->alloc_size != 0);

    /* try to apply a bad checkpoint */
    checkpoint_2 = secp256k1_scratch_checkpoint(&none->error_callback, scratch);
    secp256k1_scratch_apply_checkpoint(&none->error_callback, scratch, checkpoint);
    CHECK(ecount == 0);
    secp256k1_scratch_apply_checkpoint(&none->error_callback, scratch, checkpoint_2); /* checkpoint_2 is after checkpoint */
    CHECK(ecount == 1);
    secp256k1_scratch_apply_checkpoint(&none->error_callback, scratch, (size_t) -1); /* this is just wildly invalid */
    CHECK(ecount == 2);

    /* try to use badly initialized scratch space */
    secp256k1_scratch_space_destroy(none, scratch);
    memset(&local_scratch, 0, sizeof(local_scratch));
    scratch = &local_scratch;
    CHECK(!secp256k1_scratch_max_allocation(&none->error_callback, scratch, 0));
    CHECK(ecount == 3);
    CHECK(secp256k1_scratch_alloc(&none->error_callback, scratch, 500) == NULL);
    CHECK(ecount == 4);
    secp256k1_scratch_space_destroy(none, scratch);
    CHECK(ecount == 5);

    /* Test that large integers do not wrap around in a bad way */
    scratch = secp256k1_scratch_space_create(none, 1000);
    /* Try max allocation with a large number of objects. Only makes sense if
     * ALIGNMENT is greater than 1 because otherwise the objects take no extra
     * space. */
    CHECK(ALIGNMENT <= 1 || !secp256k1_scratch_max_allocation(&none->error_callback, scratch, (SIZE_MAX / (ALIGNMENT - 1)) + 1));
    /* Try allocating SIZE_MAX to test wrap around which only happens if
     * ALIGNMENT > 1, otherwise it returns NULL anyway because the scratch
     * space is too small. */
    CHECK(secp256k1_scratch_alloc(&none->error_callback, scratch, SIZE_MAX) == NULL);
    secp256k1_scratch_space_destroy(none, scratch);

    /* cleanup */
    secp256k1_scratch_space_destroy(none, NULL); /* no-op */
    secp256k1_context_destroy(none);
}

void run_ctz_tests(void) {
    static const uint32_t b32[] = {1, 0xffffffff, 0x5e56968f, 0xe0d63129};
    static const uint64_t b64[] = {1, 0xffffffffffffffff, 0xbcd02462139b3fc3, 0x98b5f80c769693ef};
    int shift;
    unsigned i;
    for (i = 0; i < sizeof(b32) / sizeof(b32[0]); ++i) {
        for (shift = 0; shift < 32; ++shift) {
            CHECK(secp256k1_ctz32_var_debruijn(b32[i] << shift) == shift);
            CHECK(secp256k1_ctz32_var(b32[i] << shift) == shift);
        }
    }
    for (i = 0; i < sizeof(b64) / sizeof(b64[0]); ++i) {
        for (shift = 0; shift < 64; ++shift) {
            CHECK(secp256k1_ctz64_var_debruijn(b64[i] << shift) == shift);
            CHECK(secp256k1_ctz64_var(b64[i] << shift) == shift);
        }
    }
}

/***** HASH TESTS *****/

void run_sha256_tests(void) {
    static const char *inputs[8] = {
        "", "abc", "message digest", "secure hash algorithm", "SHA256 is considered to be safe",
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
        "For this sample, this 63-byte string will be used as input data",
        "This is exactly 64 bytes long, not counting the terminating byte"
    };
    static const unsigned char outputs[8][32] = {
        {0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55},
        {0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad},
        {0xf7, 0x84, 0x6f, 0x55, 0xcf, 0x23, 0xe1, 0x4e, 0xeb, 0xea, 0xb5, 0xb4, 0xe1, 0x55, 0x0c, 0xad, 0x5b, 0x50, 0x9e, 0x33, 0x48, 0xfb, 0xc4, 0xef, 0xa3, 0xa1, 0x41, 0x3d, 0x39, 0x3c, 0xb6, 0x50},
        {0xf3, 0x0c, 0xeb, 0x2b, 0xb2, 0x82, 0x9e, 0x79, 0xe4, 0xca, 0x97, 0x53, 0xd3, 0x5a, 0x8e, 0xcc, 0x00, 0x26, 0x2d, 0x16, 0x4c, 0xc0, 0x77, 0x08, 0x02, 0x95, 0x38, 0x1c, 0xbd, 0x64, 0x3f, 0x0d},
        {0x68, 0x19, 0xd9, 0x15, 0xc7, 0x3f, 0x4d, 0x1e, 0x77, 0xe4, 0xe1, 0xb5, 0x2d, 0x1f, 0xa0, 0xf9, 0xcf, 0x9b, 0xea, 0xea, 0xd3, 0x93, 0x9f, 0x15, 0x87, 0x4b, 0xd9, 0x88, 0xe2, 0xa2, 0x36, 0x30},
        {0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8, 0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39, 0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67, 0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1},
        {0xf0, 0x8a, 0x78, 0xcb, 0xba, 0xee, 0x08, 0x2b, 0x05, 0x2a, 0xe0, 0x70, 0x8f, 0x32, 0xfa, 0x1e, 0x50, 0xc5, 0xc4, 0x21, 0xaa, 0x77, 0x2b, 0xa5, 0xdb, 0xb4, 0x06, 0xa2, 0xea, 0x6b, 0xe3, 0x42},
        {0xab, 0x64, 0xef, 0xf7, 0xe8, 0x8e, 0x2e, 0x46, 0x16, 0x5e, 0x29, 0xf2, 0xbc, 0xe4, 0x18, 0x26, 0xbd, 0x4c, 0x7b, 0x35, 0x52, 0xf6, 0xb3, 0x82, 0xa9, 0xe7, 0xd3, 0xaf, 0x47, 0xc2, 0x45, 0xf8}
    };
    int i;
    for (i = 0; i < 8; i++) {
        unsigned char out[32];
        secp256k1_sha256 hasher;
        secp256k1_sha256_initialize(&hasher);
        secp256k1_sha256_write(&hasher, (const unsigned char*)(inputs[i]), strlen(inputs[i]));
        secp256k1_sha256_finalize(&hasher, out);
        CHECK(secp256k1_memcmp_var(out, outputs[i], 32) == 0);
        if (strlen(inputs[i]) > 0) {
            int split = secp256k1_testrand_int(strlen(inputs[i]));
            secp256k1_sha256_initialize(&hasher);
            secp256k1_sha256_write(&hasher, (const unsigned char*)(inputs[i]), split);
            secp256k1_sha256_write(&hasher, (const unsigned char*)(inputs[i] + split), strlen(inputs[i]) - split);
            secp256k1_sha256_finalize(&hasher, out);
            CHECK(secp256k1_memcmp_var(out, outputs[i], 32) == 0);
        }
    }
}

void run_hmac_sha256_tests(void) {
    static const char *keys[6] = {
        "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b",
        "\x4a\x65\x66\x65",
        "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa",
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19",
        "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa",
        "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
    };
    static const char *inputs[6] = {
        "\x48\x69\x20\x54\x68\x65\x72\x65",
        "\x77\x68\x61\x74\x20\x64\x6f\x20\x79\x61\x20\x77\x61\x6e\x74\x20\x66\x6f\x72\x20\x6e\x6f\x74\x68\x69\x6e\x67\x3f",
        "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd",
        "\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd",
        "\x54\x65\x73\x74\x20\x55\x73\x69\x6e\x67\x20\x4c\x61\x72\x67\x65\x72\x20\x54\x68\x61\x6e\x20\x42\x6c\x6f\x63\x6b\x2d\x53\x69\x7a\x65\x20\x4b\x65\x79\x20\x2d\x20\x48\x61\x73\x68\x20\x4b\x65\x79\x20\x46\x69\x72\x73\x74",
        "\x54\x68\x69\x73\x20\x69\x73\x20\x61\x20\x74\x65\x73\x74\x20\x75\x73\x69\x6e\x67\x20\x61\x20\x6c\x61\x72\x67\x65\x72\x20\x74\x68\x61\x6e\x20\x62\x6c\x6f\x63\x6b\x2d\x73\x69\x7a\x65\x20\x6b\x65\x79\x20\x61\x6e\x64\x20\x61\x20\x6c\x61\x72\x67\x65\x72\x20\x74\x68\x61\x6e\x20\x62\x6c\x6f\x63\x6b\x2d\x73\x69\x7a\x65\x20\x64\x61\x74\x61\x2e\x20\x54\x68\x65\x20\x6b\x65\x79\x20\x6e\x65\x65\x64\x73\x20\x74\x6f\x20\x62\x65\x20\x68\x61\x73\x68\x65\x64\x20\x62\x65\x66\x6f\x72\x65\x20\x62\x65\x69\x6e\x67\x20\x75\x73\x65\x64\x20\x62\x79\x20\x74\x68\x65\x20\x48\x4d\x41\x43\x20\x61\x6c\x67\x6f\x72\x69\x74\x68\x6d\x2e"
    };
    static const unsigned char outputs[6][32] = {
        {0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53, 0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b, 0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7, 0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7},
        {0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e, 0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7, 0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83, 0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43},
        {0x77, 0x3e, 0xa9, 0x1e, 0x36, 0x80, 0x0e, 0x46, 0x85, 0x4d, 0xb8, 0xeb, 0xd0, 0x91, 0x81, 0xa7, 0x29, 0x59, 0x09, 0x8b, 0x3e, 0xf8, 0xc1, 0x22, 0xd9, 0x63, 0x55, 0x14, 0xce, 0xd5, 0x65, 0xfe},
        {0x82, 0x55, 0x8a, 0x38, 0x9a, 0x44, 0x3c, 0x0e, 0xa4, 0xcc, 0x81, 0x98, 0x99, 0xf2, 0x08, 0x3a, 0x85, 0xf0, 0xfa, 0xa3, 0xe5, 0x78, 0xf8, 0x07, 0x7a, 0x2e, 0x3f, 0xf4, 0x67, 0x29, 0x66, 0x5b},
        {0x60, 0xe4, 0x31, 0x59, 0x1e, 0xe0, 0xb6, 0x7f, 0x0d, 0x8a, 0x26, 0xaa, 0xcb, 0xf5, 0xb7, 0x7f, 0x8e, 0x0b, 0xc6, 0x21, 0x37, 0x28, 0xc5, 0x14, 0x05, 0x46, 0x04, 0x0f, 0x0e, 0xe3, 0x7f, 0x54},
        {0x9b, 0x09, 0xff, 0xa7, 0x1b, 0x94, 0x2f, 0xcb, 0x27, 0x63, 0x5f, 0xbc, 0xd5, 0xb0, 0xe9, 0x44, 0xbf, 0xdc, 0x63, 0x64, 0x4f, 0x07, 0x13, 0x93, 0x8a, 0x7f, 0x51, 0x53, 0x5c, 0x3a, 0x35, 0xe2}
    };
    int i;
    for (i = 0; i < 6; i++) {
        secp256k1_hmac_sha256 hasher;
        unsigned char out[32];
        secp256k1_hmac_sha256_initialize(&hasher, (const unsigned char*)(keys[i]), strlen(keys[i]));
        secp256k1_hmac_sha256_write(&hasher, (const unsigned char*)(inputs[i]), strlen(inputs[i]));
        secp256k1_hmac_sha256_finalize(&hasher, out);
        CHECK(secp256k1_memcmp_var(out, outputs[i], 32) == 0);
        if (strlen(inputs[i]) > 0) {
            int split = secp256k1_testrand_int(strlen(inputs[i]));
            secp256k1_hmac_sha256_initialize(&hasher, (const unsigned char*)(keys[i]), strlen(keys[i]));
            secp256k1_hmac_sha256_write(&hasher, (const unsigned char*)(inputs[i]), split);
            secp256k1_hmac_sha256_write(&hasher, (const unsigned char*)(inputs[i] + split), strlen(inputs[i]) - split);
            secp256k1_hmac_sha256_finalize(&hasher, out);
            CHECK(secp256k1_memcmp_var(out, outputs[i], 32) == 0);
        }
    }
}

void run_rfc6979_hmac_sha256_tests(void) {
    static const unsigned char key1[65] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x00, 0x4b, 0xf5, 0x12, 0x2f, 0x34, 0x45, 0x54, 0xc5, 0x3b, 0xde, 0x2e, 0xbb, 0x8c, 0xd2, 0xb7, 0xe3, 0xd1, 0x60, 0x0a, 0xd6, 0x31, 0xc3, 0x85, 0xa5, 0xd7, 0xcc, 0xe2, 0x3c, 0x77, 0x85, 0x45, 0x9a, 0};
    static const unsigned char out1[3][32] = {
        {0x4f, 0xe2, 0x95, 0x25, 0xb2, 0x08, 0x68, 0x09, 0x15, 0x9a, 0xcd, 0xf0, 0x50, 0x6e, 0xfb, 0x86, 0xb0, 0xec, 0x93, 0x2c, 0x7b, 0xa4, 0x42, 0x56, 0xab, 0x32, 0x1e, 0x42, 0x1e, 0x67, 0xe9, 0xfb},
        {0x2b, 0xf0, 0xff, 0xf1, 0xd3, 0xc3, 0x78, 0xa2, 0x2d, 0xc5, 0xde, 0x1d, 0x85, 0x65, 0x22, 0x32, 0x5c, 0x65, 0xb5, 0x04, 0x49, 0x1a, 0x0c, 0xbd, 0x01, 0xcb, 0x8f, 0x3a, 0xa6, 0x7f, 0xfd, 0x4a},
        {0xf5, 0x28, 0xb4, 0x10, 0xcb, 0x54, 0x1f, 0x77, 0x00, 0x0d, 0x7a, 0xfb, 0x6c, 0x5b, 0x53, 0xc5, 0xc4, 0x71, 0xea, 0xb4, 0x3e, 0x46, 0x6d, 0x9a, 0xc5, 0x19, 0x0c, 0x39, 0xc8, 0x2f, 0xd8, 0x2e}
    };

    static const unsigned char key2[64] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55};
    static const unsigned char out2[3][32] = {
        {0x9c, 0x23, 0x6c, 0x16, 0x5b, 0x82, 0xae, 0x0c, 0xd5, 0x90, 0x65, 0x9e, 0x10, 0x0b, 0x6b, 0xab, 0x30, 0x36, 0xe7, 0xba, 0x8b, 0x06, 0x74, 0x9b, 0xaf, 0x69, 0x81, 0xe1, 0x6f, 0x1a, 0x2b, 0x95},
        {0xdf, 0x47, 0x10, 0x61, 0x62, 0x5b, 0xc0, 0xea, 0x14, 0xb6, 0x82, 0xfe, 0xee, 0x2c, 0x9c, 0x02, 0xf2, 0x35, 0xda, 0x04, 0x20, 0x4c, 0x1d, 0x62, 0xa1, 0x53, 0x6c, 0x6e, 0x17, 0xae, 0xd7, 0xa9},
        {0x75, 0x97, 0x88, 0x7c, 0xbd, 0x76, 0x32, 0x1f, 0x32, 0xe3, 0x04, 0x40, 0x67, 0x9a, 0x22, 0xcf, 0x7f, 0x8d, 0x9d, 0x2e, 0xac, 0x39, 0x0e, 0x58, 0x1f, 0xea, 0x09, 0x1c, 0xe2, 0x02, 0xba, 0x94}
    };

    secp256k1_rfc6979_hmac_sha256 rng;
    unsigned char out[32];
    int i;

    secp256k1_rfc6979_hmac_sha256_initialize(&rng, key1, 64);
    for (i = 0; i < 3; i++) {
        secp256k1_rfc6979_hmac_sha256_generate(&rng, out, 32);
        CHECK(secp256k1_memcmp_var(out, out1[i], 32) == 0);
    }
    secp256k1_rfc6979_hmac_sha256_finalize(&rng);

    secp256k1_rfc6979_hmac_sha256_initialize(&rng, key1, 65);
    for (i = 0; i < 3; i++) {
        secp256k1_rfc6979_hmac_sha256_generate(&rng, out, 32);
        CHECK(secp256k1_memcmp_var(out, out1[i], 32) != 0);
    }
    secp256k1_rfc6979_hmac_sha256_finalize(&rng);

    secp256k1_rfc6979_hmac_sha256_initialize(&rng, key2, 64);
    for (i = 0; i < 3; i++) {
        secp256k1_rfc6979_hmac_sha256_generate(&rng, out, 32);
        CHECK(secp256k1_memcmp_var(out, out2[i], 32) == 0);
    }
    secp256k1_rfc6979_hmac_sha256_finalize(&rng);
}

void run_tagged_sha256_tests(void) {
    int ecount = 0;
    secp256k1_context *none = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    unsigned char tag[32] = { 0 };
    unsigned char msg[32] = { 0 };
    unsigned char hash32[32];
    unsigned char hash_expected[32] = {
        0x04, 0x7A, 0x5E, 0x17, 0xB5, 0x86, 0x47, 0xC1,
        0x3C, 0xC6, 0xEB, 0xC0, 0xAA, 0x58, 0x3B, 0x62,
        0xFB, 0x16, 0x43, 0x32, 0x68, 0x77, 0x40, 0x6C,
        0xE2, 0x76, 0x55, 0x9A, 0x3B, 0xDE, 0x55, 0xB3
    };

    secp256k1_context_set_illegal_callback(none, counting_illegal_callback_fn, &ecount);

    /* API test */
    CHECK(secp256k1_tagged_sha256(none, hash32, tag, sizeof(tag), msg, sizeof(msg)) == 1);
    CHECK(secp256k1_tagged_sha256(none, NULL, tag, sizeof(tag), msg, sizeof(msg)) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_tagged_sha256(none, hash32, NULL, 0, msg, sizeof(msg)) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_tagged_sha256(none, hash32, tag, sizeof(tag), NULL, 0) == 0);
    CHECK(ecount == 3);

    /* Static test vector */
    memcpy(tag, "tag", 3);
    memcpy(msg, "msg", 3);
    CHECK(secp256k1_tagged_sha256(none, hash32, tag, 3, msg, 3) == 1);
    CHECK(secp256k1_memcmp_var(hash32, hash_expected, sizeof(hash32)) == 0);
    secp256k1_context_destroy(none);
}

/***** RANDOM TESTS *****/

void test_rand_bits(int rand32, int bits) {
    /* (1-1/2^B)^rounds[B] < 1/10^9, so rounds is the number of iterations to
     * get a false negative chance below once in a billion */
    static const unsigned int rounds[7] = {1, 30, 73, 156, 322, 653, 1316};
    /* We try multiplying the results with various odd numbers, which shouldn't
     * influence the uniform distribution modulo a power of 2. */
    static const uint32_t mults[6] = {1, 3, 21, 289, 0x9999, 0x80402011};
    /* We only select up to 6 bits from the output to analyse */
    unsigned int usebits = bits > 6 ? 6 : bits;
    unsigned int maxshift = bits - usebits;
    /* For each of the maxshift+1 usebits-bit sequences inside a bits-bit
       number, track all observed outcomes, one per bit in a uint64_t. */
    uint64_t x[6][27] = {{0}};
    unsigned int i, shift, m;
    /* Multiply the output of all rand calls with the odd number m, which
       should not change the uniformity of its distribution. */
    for (i = 0; i < rounds[usebits]; i++) {
        uint32_t r = (rand32 ? secp256k1_testrand32() : secp256k1_testrand_bits(bits));
        CHECK((((uint64_t)r) >> bits) == 0);
        for (m = 0; m < sizeof(mults) / sizeof(mults[0]); m++) {
            uint32_t rm = r * mults[m];
            for (shift = 0; shift <= maxshift; shift++) {
                x[m][shift] |= (((uint64_t)1) << ((rm >> shift) & ((1 << usebits) - 1)));
            }
        }
    }
    for (m = 0; m < sizeof(mults) / sizeof(mults[0]); m++) {
        for (shift = 0; shift <= maxshift; shift++) {
            /* Test that the lower usebits bits of x[shift] are 1 */
            CHECK(((~x[m][shift]) << (64 - (1 << usebits))) == 0);
        }
    }
}

/* Subrange must be a whole divisor of range, and at most 64 */
void test_rand_int(uint32_t range, uint32_t subrange) {
    /* (1-1/subrange)^rounds < 1/10^9 */
    int rounds = (subrange * 2073) / 100;
    int i;
    uint64_t x = 0;
    CHECK((range % subrange) == 0);
    for (i = 0; i < rounds; i++) {
        uint32_t r = secp256k1_testrand_int(range);
        CHECK(r < range);
        r = r % subrange;
        x |= (((uint64_t)1) << r);
    }
    /* Test that the lower subrange bits of x are 1. */
    CHECK(((~x) << (64 - subrange)) == 0);
}

void run_rand_bits(void) {
    size_t b;
    test_rand_bits(1, 32);
    for (b = 1; b <= 32; b++) {
        test_rand_bits(0, b);
    }
}

void run_rand_int(void) {
    static const uint32_t ms[] = {1, 3, 17, 1000, 13771, 999999, 33554432};
    static const uint32_t ss[] = {1, 3, 6, 9, 13, 31, 64};
    unsigned int m, s;
    for (m = 0; m < sizeof(ms) / sizeof(ms[0]); m++) {
        for (s = 0; s < sizeof(ss) / sizeof(ss[0]); s++) {
            test_rand_int(ms[m] * ss[s], ss[s]);
        }
    }
}

/***** MODINV TESTS *****/

/* Compute the modular inverse of (odd) x mod 2^64. */
uint64_t modinv2p64(uint64_t x) {
    /* If w = 1/x mod 2^(2^L), then w*(2 - w*x) = 1/x mod 2^(2^(L+1)). See
     * Hacker's Delight second edition, Henry S. Warren, Jr., pages 245-247 for
     * why. Start with L=0, for which it is true for every odd x that
     * 1/x=1 mod 2. Iterating 6 times gives us 1/x mod 2^64. */
    int l;
    uint64_t w = 1;
    CHECK(x & 1);
    for (l = 0; l < 6; ++l) w *= (2 - w*x);
    return w;
}

/* compute out = (a*b) mod m; if b=NULL, treat b=1.
 *
 * Out is a 512-bit number (represented as 32 uint16_t's in LE order). The other
 * arguments are 256-bit numbers (represented as 16 uint16_t's in LE order). */
void mulmod256(uint16_t* out, const uint16_t* a, const uint16_t* b, const uint16_t* m) {
    uint16_t mul[32];
    uint64_t c = 0;
    int i, j;
    int m_bitlen = 0;
    int mul_bitlen = 0;

    if (b != NULL) {
        /* Compute the product of a and b, and put it in mul. */
        for (i = 0; i < 32; ++i) {
            for (j = i <= 15 ? 0 : i - 15; j <= i && j <= 15; j++) {
                c += (uint64_t)a[j] * b[i - j];
            }
            mul[i] = c & 0xFFFF;
            c >>= 16;
        }
        CHECK(c == 0);

        /* compute the highest set bit in mul */
        for (i = 511; i >= 0; --i) {
            if ((mul[i >> 4] >> (i & 15)) & 1) {
                mul_bitlen = i;
                break;
            }
        }
    } else {
        /* if b==NULL, set mul=a. */
        memcpy(mul, a, 32);
        memset(mul + 16, 0, 32);
        /* compute the highest set bit in mul */
        for (i = 255; i >= 0; --i) {
            if ((mul[i >> 4] >> (i & 15)) & 1) {
                mul_bitlen = i;
                break;
            }
        }
    }

    /* Compute the highest set bit in m. */
    for (i = 255; i >= 0; --i) {
        if ((m[i >> 4] >> (i & 15)) & 1) {
            m_bitlen = i;
            break;
        }
    }

    /* Try do mul -= m<<i, for i going down to 0, whenever the result is not negative */
    for (i = mul_bitlen - m_bitlen; i >= 0; --i) {
        uint16_t mul2[32];
        int64_t cs;

        /* Compute mul2 = mul - m<<i. */
        cs = 0; /* accumulator */
        for (j = 0; j < 32; ++j) { /* j loops over the output limbs in mul2. */
            /* Compute sub: the 16 bits in m that will be subtracted from mul2[j]. */
            uint16_t sub = 0;
            int p;
            for (p = 0; p < 16; ++p) { /* p loops over the bit positions in mul2[j]. */
                int bitpos = j * 16 - i + p; /* bitpos is the correspond bit position in m. */
                if (bitpos >= 0 && bitpos < 256) {
                    sub |= ((m[bitpos >> 4] >> (bitpos & 15)) & 1) << p;
                }
            }
            /* Add mul[j]-sub to accumulator, and shift bottom 16 bits out to mul2[j]. */
            cs += mul[j];
            cs -= sub;
            mul2[j] = (cs & 0xFFFF);
            cs >>= 16;
        }
        /* If remainder of subtraction is 0, set mul = mul2. */
        if (cs == 0) {
            memcpy(mul, mul2, sizeof(mul));
        }
    }
    /* Sanity check: test that all limbs higher than m's highest are zero */
    for (i = (m_bitlen >> 4) + 1; i < 32; ++i) {
        CHECK(mul[i] == 0);
    }
    memcpy(out, mul, 32);
}

/* Convert a 256-bit number represented as 16 uint16_t's to signed30 notation. */
void uint16_to_signed30(secp256k1_modinv32_signed30* out, const uint16_t* in) {
    int i;
    memset(out->v, 0, sizeof(out->v));
    for (i = 0; i < 256; ++i) {
        out->v[i / 30] |= (int32_t)(((in[i >> 4]) >> (i & 15)) & 1) << (i % 30);
    }
}

/* Convert a 256-bit number in signed30 notation to a representation as 16 uint16_t's. */
void signed30_to_uint16(uint16_t* out, const secp256k1_modinv32_signed30* in) {
    int i;
    memset(out, 0, 32);
    for (i = 0; i < 256; ++i) {
        out[i >> 4] |= (((in->v[i / 30]) >> (i % 30)) & 1) << (i & 15);
    }
}

/* Randomly mutate the sign of limbs in signed30 representation, without changing the value. */
void mutate_sign_signed30(secp256k1_modinv32_signed30* x) {
    int i;
    for (i = 0; i < 16; ++i) {
        int pos = secp256k1_testrand_int(8);
        if (x->v[pos] > 0 && x->v[pos + 1] <= 0x3fffffff) {
            x->v[pos] -= 0x40000000;
            x->v[pos + 1] += 1;
        } else if (x->v[pos] < 0 && x->v[pos + 1] >= 0x3fffffff) {
            x->v[pos] += 0x40000000;
            x->v[pos + 1] -= 1;
        }
    }
}

/* Test secp256k1_modinv32{_var}, using inputs in 16-bit limb format, and returning inverse. */
void test_modinv32_uint16(uint16_t* out, const uint16_t* in, const uint16_t* mod) {
    uint16_t tmp[16];
    secp256k1_modinv32_signed30 x;
    secp256k1_modinv32_modinfo m;
    int i, vartime, nonzero;

    uint16_to_signed30(&x, in);
    nonzero = (x.v[0] | x.v[1] | x.v[2] | x.v[3] | x.v[4] | x.v[5] | x.v[6] | x.v[7] | x.v[8]) != 0;
    uint16_to_signed30(&m.modulus, mod);
    mutate_sign_signed30(&m.modulus);

    /* compute 1/modulus mod 2^30 */
    m.modulus_inv30 = modinv2p64(m.modulus.v[0]) & 0x3fffffff;
    CHECK(((m.modulus_inv30 * m.modulus.v[0]) & 0x3fffffff) == 1);

    for (vartime = 0; vartime < 2; ++vartime) {
        /* compute inverse */
        (vartime ? secp256k1_modinv32_var : secp256k1_modinv32)(&x, &m);

        /* produce output */
        signed30_to_uint16(out, &x);

        /* check if the inverse times the input is 1 (mod m), unless x is 0. */
        mulmod256(tmp, out, in, mod);
        CHECK(tmp[0] == nonzero);
        for (i = 1; i < 16; ++i) CHECK(tmp[i] == 0);

        /* invert again */
        (vartime ? secp256k1_modinv32_var : secp256k1_modinv32)(&x, &m);

        /* check if the result is equal to the input */
        signed30_to_uint16(tmp, &x);
        for (i = 0; i < 16; ++i) CHECK(tmp[i] == in[i]);
    }
}

#ifdef SECP256K1_WIDEMUL_INT128
/* Convert a 256-bit number represented as 16 uint16_t's to signed62 notation. */
void uint16_to_signed62(secp256k1_modinv64_signed62* out, const uint16_t* in) {
    int i;
    memset(out->v, 0, sizeof(out->v));
    for (i = 0; i < 256; ++i) {
        out->v[i / 62] |= (int64_t)(((in[i >> 4]) >> (i & 15)) & 1) << (i % 62);
    }
}

/* Convert a 256-bit number in signed62 notation to a representation as 16 uint16_t's. */
void signed62_to_uint16(uint16_t* out, const secp256k1_modinv64_signed62* in) {
    int i;
    memset(out, 0, 32);
    for (i = 0; i < 256; ++i) {
        out[i >> 4] |= (((in->v[i / 62]) >> (i % 62)) & 1) << (i & 15);
    }
}

/* Randomly mutate the sign of limbs in signed62 representation, without changing the value. */
void mutate_sign_signed62(secp256k1_modinv64_signed62* x) {
    static const int64_t M62 = (int64_t)(UINT64_MAX >> 2);
    int i;
    for (i = 0; i < 8; ++i) {
        int pos = secp256k1_testrand_int(4);
        if (x->v[pos] > 0 && x->v[pos + 1] <= M62) {
            x->v[pos] -= (M62 + 1);
            x->v[pos + 1] += 1;
        } else if (x->v[pos] < 0 && x->v[pos + 1] >= -M62) {
            x->v[pos] += (M62 + 1);
            x->v[pos + 1] -= 1;
        }
    }
}

/* Test secp256k1_modinv64{_var}, using inputs in 16-bit limb format, and returning inverse. */
void test_modinv64_uint16(uint16_t* out, const uint16_t* in, const uint16_t* mod) {
    static const int64_t M62 = (int64_t)(UINT64_MAX >> 2);
    uint16_t tmp[16];
    secp256k1_modinv64_signed62 x;
    secp256k1_modinv64_modinfo m;
    int i, vartime, nonzero;

    uint16_to_signed62(&x, in);
    nonzero = (x.v[0] | x.v[1] | x.v[2] | x.v[3] | x.v[4]) != 0;
    uint16_to_signed62(&m.modulus, mod);
    mutate_sign_signed62(&m.modulus);

    /* compute 1/modulus mod 2^62 */
    m.modulus_inv62 = modinv2p64(m.modulus.v[0]) & M62;
    CHECK(((m.modulus_inv62 * m.modulus.v[0]) & M62) == 1);

    for (vartime = 0; vartime < 2; ++vartime) {
        /* compute inverse */
        (vartime ? secp256k1_modinv64_var : secp256k1_modinv64)(&x, &m);

        /* produce output */
        signed62_to_uint16(out, &x);

        /* check if the inverse times the input is 1 (mod m), unless x is 0. */
        mulmod256(tmp, out, in, mod);
        CHECK(tmp[0] == nonzero);
        for (i = 1; i < 16; ++i) CHECK(tmp[i] == 0);

        /* invert again */
        (vartime ? secp256k1_modinv64_var : secp256k1_modinv64)(&x, &m);

        /* check if the result is equal to the input */
        signed62_to_uint16(tmp, &x);
        for (i = 0; i < 16; ++i) CHECK(tmp[i] == in[i]);
    }
}
#endif

/* test if a and b are coprime */
int coprime(const uint16_t* a, const uint16_t* b) {
    uint16_t x[16], y[16], t[16];
    int i;
    int iszero;
    memcpy(x, a, 32);
    memcpy(y, b, 32);

    /* simple gcd loop: while x!=0, (x,y)=(y%x,x) */
    while (1) {
        iszero = 1;
        for (i = 0; i < 16; ++i) {
            if (x[i] != 0) {
                iszero = 0;
                break;
            }
        }
        if (iszero) break;
        mulmod256(t, y, NULL, x);
        memcpy(y, x, 32);
        memcpy(x, t, 32);
    }

    /* return whether y=1 */
    if (y[0] != 1) return 0;
    for (i = 1; i < 16; ++i) {
        if (y[i] != 0) return 0;
    }
    return 1;
}

void run_modinv_tests(void) {
    /* Fixed test cases. Each tuple is (input, modulus, output), each as 16x16 bits in LE order. */
    static const uint16_t CASES[][3][16] = {
        /* Test cases triggering edge cases in divsteps */

        /* Test case known to need 713 divsteps */
        {{0x1513, 0x5389, 0x54e9, 0x2798, 0x1957, 0x66a0, 0x8057, 0x3477,
          0x7784, 0x1052, 0x326a, 0x9331, 0x6506, 0xa95c, 0x91f3, 0xfb5e},
         {0x2bdd, 0x8df4, 0xcc61, 0x481f, 0xdae5, 0x5ca7, 0xf43b, 0x7d54,
          0x13d6, 0x469b, 0x2294, 0x20f4, 0xb2a4, 0xa2d1, 0x3ff1, 0xfd4b},
         {0xffd8, 0xd9a0, 0x456e, 0x81bb, 0xbabd, 0x6cea, 0x6dbd, 0x73ab,
          0xbb94, 0x3d3c, 0xdf08, 0x31c4, 0x3e32, 0xc179, 0x2486, 0xb86b}},
        /* Test case known to need 589 divsteps, reaching delta=-140 and
           delta=141. */
        {{0x3fb1, 0x903b, 0x4eb7, 0x4813, 0xd863, 0x26bf, 0xd89f, 0xa8a9,
          0x02fe, 0x57c6, 0x554a, 0x4eab, 0x165e, 0x3d61, 0xee1e, 0x456c},
         {0x9295, 0x823b, 0x5c1f, 0x5386, 0x48e0, 0x02ff, 0x4c2a, 0xa2da,
          0xe58f, 0x967c, 0xc97e, 0x3f5a, 0x69fb, 0x52d9, 0x0a86, 0xb4a3},
         {0x3d30, 0xb893, 0xa809, 0xa7a8, 0x26f5, 0x5b42, 0x55be, 0xf4d0,
          0x12c2, 0x7e6a, 0xe41a, 0x90c7, 0xebfa, 0xf920, 0x304e, 0x1419}},
        /* Test case known to need 650 divsteps, and doing 65 consecutive (f,g/2) steps. */
        {{0x8583, 0x5058, 0xbeae, 0xeb69, 0x48bc, 0x52bb, 0x6a9d, 0xcc94,
          0x2a21, 0x87d5, 0x5b0d, 0x42f6, 0x5b8a, 0x2214, 0xe9d6, 0xa040},
         {0x7531, 0x27cb, 0x7e53, 0xb739, 0x6a5f, 0x83f5, 0xa45c, 0xcb1d,
          0x8a87, 0x1c9c, 0x51d7, 0x851c, 0xb9d8, 0x1fbe, 0xc241, 0xd4a3},
         {0xcdb4, 0x275c, 0x7d22, 0xa906, 0x0173, 0xc054, 0x7fdf, 0x5005,
          0x7fb8, 0x9059, 0xdf51, 0x99df, 0x2654, 0x8f6e, 0x070f, 0xb347}},
        /* example needing 713 divsteps; delta=-2..3 */
        {{0xe2e9, 0xee91, 0x4345, 0xe5ad, 0xf3ec, 0x8f42, 0x0364, 0xd5c9,
          0xff49, 0xbef5, 0x4544, 0x4c7c, 0xae4b, 0xfd9d, 0xb35b, 0xda9d},
         {0x36e7, 0x8cca, 0x2ed0, 0x47b3, 0xaca4, 0xb374, 0x7d2a, 0x0772,
          0x6bdb, 0xe0a7, 0x900b, 0xfe10, 0x788c, 0x6f22, 0xd909, 0xf298},
         {0xd8c6, 0xba39, 0x13ed, 0x198c, 0x16c8, 0xb837, 0xa5f2, 0x9797,
          0x0113, 0x882a, 0x15b5, 0x324c, 0xabee, 0xe465, 0x8170, 0x85ac}},
        /* example needing 713 divsteps; delta=-2..3 */
        {{0xd5b7, 0x2966, 0x040e, 0xf59a, 0x0387, 0xd96d, 0xbfbc, 0xd850,
          0x2d96, 0x872a, 0xad81, 0xc03c, 0xbb39, 0xb7fa, 0xd904, 0xef78},
         {0x6279, 0x4314, 0xfdd3, 0x1568, 0x0982, 0x4d13, 0x625f, 0x010c,
          0x22b1, 0x0cc3, 0xf22d, 0x5710, 0x1109, 0x5751, 0x7714, 0xfcf2},
         {0xdb13, 0x5817, 0x232e, 0xe456, 0xbbbc, 0x6fbe, 0x4572, 0xa358,
          0xc76d, 0x928e, 0x0162, 0x5314, 0x8325, 0x5683, 0xe21b, 0xda88}},
        /* example needing 713 divsteps; delta=-2..3 */
        {{0xa06f, 0x71ee, 0x3bac, 0x9ebb, 0xdeaa, 0x09ed, 0x1cf7, 0x9ec9,
          0x7158, 0x8b72, 0x5d53, 0x5479, 0x5c75, 0xbb66, 0x9125, 0xeccc},
         {0x2941, 0xd46c, 0x3cd4, 0x4a9d, 0x5c4a, 0x256b, 0xbd6c, 0x9b8e,
          0x8fe0, 0x8a14, 0xffe8, 0x2496, 0x618d, 0xa9d7, 0x5018, 0xfb29},
         {0x437c, 0xbd60, 0x7590, 0x94bb, 0x0095, 0xd35e, 0xd4fe, 0xd6da,
          0x0d4e, 0x5342, 0x4cd2, 0x169b, 0x661c, 0x1380, 0xed2d, 0x85c1}},
        /* example reaching delta=-64..65; 661 divsteps */
        {{0xfde4, 0x68d6, 0x6c48, 0x7f77, 0x1c78, 0x96de, 0x2fd9, 0xa6c2,
          0xbbb5, 0xd319, 0x69cf, 0xd4b3, 0xa321, 0xcda0, 0x172e, 0xe530},
         {0xd9e3, 0x0f60, 0x3d86, 0xeeab, 0x25ee, 0x9582, 0x2d50, 0xfe16,
          0xd4e2, 0xe3ba, 0x94e2, 0x9833, 0x6c5e, 0x8982, 0x13b6, 0xe598},
         {0xe675, 0xf55a, 0x10f6, 0xabde, 0x5113, 0xecaa, 0x61ae, 0xad9f,
          0x0c27, 0xef33, 0x62e5, 0x211d, 0x08fa, 0xa78d, 0xc675, 0x8bae}},
        /* example reaching delta=-64..65; 661 divsteps */
        {{0x21bf, 0x52d5, 0x8fd4, 0xaa18, 0x156a, 0x7247, 0xebb8, 0x5717,
          0x4eb5, 0x1421, 0xb58f, 0x3b0b, 0x5dff, 0xe533, 0xb369, 0xd28a},
         {0x9f6b, 0xe463, 0x2563, 0xc74d, 0x6d81, 0x636a, 0x8fc8, 0x7a94,
          0x9429, 0x1585, 0xf35e, 0x7ff5, 0xb64f, 0x9720, 0xba74, 0xe108},
         {0xa5ab, 0xea7b, 0xfe5e, 0x8a85, 0x13be, 0x7934, 0xe8a0, 0xa187,
          0x86b5, 0xe477, 0xb9a4, 0x75d7, 0x538f, 0xdd70, 0xc781, 0xb67d}},
        /* example reaching delta=-64..65; 661 divsteps */
        {{0xa41a, 0x3e8d, 0xf1f5, 0x9493, 0x868c, 0x5103, 0x2725, 0x3ceb,
          0x6032, 0x3624, 0xdc6b, 0x9120, 0xbf4c, 0x8821, 0x91ad, 0xb31a},
         {0x5c0b, 0xdda5, 0x20f8, 0x32a1, 0xaf73, 0x6ec5, 0x4779, 0x43d6,
          0xd454, 0x9573, 0xbf84, 0x5a58, 0xe04e, 0x307e, 0xd1d5, 0xe230},
         {0xda15, 0xbcd6, 0x7180, 0xabd3, 0x04e6, 0x6986, 0xc0d7, 0x90bb,
          0x3a4d, 0x7c95, 0xaaab, 0x9ab3, 0xda34, 0xa7f6, 0x9636, 0x6273}},
        /* example doing 123 consecutive (f,g/2) steps; 615 divsteps */
        {{0xb4d6, 0xb38f, 0x00aa, 0xebda, 0xd4c2, 0x70b8, 0x9dad, 0x58ee,
          0x68f8, 0x48d3, 0xb5ff, 0xf422, 0x9e46, 0x2437, 0x18d0, 0xd9cc},
         {0x5c83, 0xfed7, 0x97f5, 0x3f07, 0xcaad, 0x95b1, 0xb4a4, 0xb005,
          0x23af, 0xdd27, 0x6c0d, 0x932c, 0xe2b2, 0xe3ae, 0xfb96, 0xdf67},
         {0x3105, 0x0127, 0xfd48, 0x039b, 0x35f1, 0xbc6f, 0x6c0a, 0xb572,
          0xe4df, 0xebad, 0x8edc, 0xb89d, 0x9555, 0x4c26, 0x1fef, 0x997c}},
        /* example doing 123 consecutive (f,g/2) steps; 614 divsteps */
        {{0x5138, 0xd474, 0x385f, 0xc964, 0x00f2, 0x6df7, 0x862d, 0xb185,
          0xb264, 0xe9e1, 0x466c, 0xf39e, 0xafaf, 0x5f41, 0x47e2, 0xc89d},
         {0x8607, 0x9c81, 0x46a2, 0x7dcc, 0xcb0c, 0x9325, 0xe149, 0x2bde,
          0x6632, 0x2869, 0xa261, 0xb163, 0xccee, 0x22ae, 0x91e0, 0xcfd5},
         {0x831c, 0xda22, 0xb080, 0xba7a, 0x26e2, 0x54b0, 0x073b, 0x5ea0,
          0xed4b, 0xcb3d, 0xbba1, 0xbec8, 0xf2ad, 0xae0d, 0x349b, 0x17d1}},
        /* example doing 123 consecutive (f,g/2) steps; 614 divsteps */
        {{0xe9a5, 0xb4ad, 0xd995, 0x9953, 0xcdff, 0x50d7, 0xf715, 0x9dc7,
          0x3e28, 0x15a9, 0x95a3, 0x8554, 0x5b5e, 0xad1d, 0x6d57, 0x3d50},
         {0x3ad9, 0xbd60, 0x5cc7, 0x6b91, 0xadeb, 0x71f6, 0x7cc4, 0xa58a,
          0x2cce, 0xf17c, 0x38c9, 0x97ed, 0x65fb, 0x3fa6, 0xa6bc, 0xeb24},
         {0xf96c, 0x1963, 0x8151, 0xa0cc, 0x299b, 0xf277, 0x001a, 0x16bb,
          0xfd2e, 0x532d, 0x0410, 0xe117, 0x6b00, 0x44ec, 0xca6a, 0x1745}},
        /* example doing 446 (f,g/2) steps; 523 divsteps */
        {{0x3758, 0xa56c, 0xe41e, 0x4e47, 0x0975, 0xa82b, 0x107c, 0x89cf,
          0x2093, 0x5a0c, 0xda37, 0xe007, 0x6074, 0x4f68, 0x2f5a, 0xbb8a},
         {0x4beb, 0xa40f, 0x2c42, 0xd9d6, 0x97e8, 0xca7c, 0xd395, 0x894f,
          0x1f50, 0x8067, 0xa233, 0xb850, 0x1746, 0x1706, 0xbcda, 0xdf32},
         {0x762a, 0xceda, 0x4c45, 0x1ca0, 0x8c37, 0xd8c5, 0xef57, 0x7a2c,
          0x6e98, 0xe38a, 0xc50e, 0x2ca9, 0xcb85, 0x24d5, 0xc29c, 0x61f6}},
        /* example doing 446 (f,g/2) steps; 523 divsteps */
        {{0x6f38, 0x74ad, 0x7332, 0x4073, 0x6521, 0xb876, 0xa370, 0xa6bd,
          0xcea5, 0xbd06, 0x969f, 0x77c6, 0x1e69, 0x7c49, 0x7d51, 0xb6e7},
         {0x3f27, 0x4be4, 0xd81e, 0x1396, 0xb21f, 0x92aa, 0x6dc3, 0x6283,
          0x6ada, 0x3ca2, 0xc1e5, 0x8b9b, 0xd705, 0x5598, 0x8ba1, 0xe087},
         {0x6a22, 0xe834, 0xbc8d, 0xcee9, 0x42fc, 0xfc77, 0x9c45, 0x1ca8,
          0xeb66, 0xed74, 0xaaf9, 0xe75f, 0xfe77, 0x46d2, 0x179b, 0xbf3e}},
        /* example doing 336 (f,(f+g)/2) steps; 693 divsteps */
        {{0x7ea7, 0x444e, 0x84ea, 0xc447, 0x7c1f, 0xab97, 0x3de6, 0x5878,
          0x4e8b, 0xc017, 0x03e0, 0xdc40, 0xbbd0, 0x74ce, 0x0169, 0x7ab5},
         {0x4023, 0x154f, 0xfbe4, 0x8195, 0xfda0, 0xef54, 0x9e9a, 0xc703,
          0x2803, 0xf760, 0x6302, 0xed5b, 0x7157, 0x6456, 0xdd7d, 0xf14b},
         {0xb6fb, 0xe3b3, 0x0733, 0xa77e, 0x44c5, 0x3003, 0xc937, 0xdd4d,
          0x5355, 0x14e9, 0x184e, 0xcefe, 0xe6b5, 0xf2e0, 0x0a28, 0x5b74}},
        /* example doing 336 (f,(f+g)/2) steps; 687 divsteps */
        {{0xa893, 0xb5f4, 0x1ede, 0xa316, 0x242c, 0xbdcc, 0xb017, 0x0836,
          0x3a37, 0x27fb, 0xfb85, 0x251e, 0xa189, 0xb15d, 0xa4b8, 0xc24c},
         {0xb0b7, 0x57ba, 0xbb6d, 0x9177, 0xc896, 0xc7f2, 0x43b4, 0x85a6,
          0xe6c4, 0xe50e, 0x3109, 0x7ca5, 0xd73d, 0x13ff, 0x0c3d, 0xcd62},
         {0x48ca, 0xdb34, 0xe347, 0x2cef, 0x4466, 0x10fb, 0x7ee1, 0x6344,
          0x4308, 0x966d, 0xd4d1, 0xb099, 0x994f, 0xd025, 0x2187, 0x5866}},
        /* example doing 267 (g,(g-f)/2) steps; 678 divsteps */
        {{0x0775, 0x1754, 0x01f6, 0xdf37, 0xc0be, 0x8197, 0x072f, 0x6cf5,
          0x8b36, 0x8069, 0x5590, 0xb92d, 0x6084, 0x47a4, 0x23fe, 0xddd5},
         {0x8e1b, 0xda37, 0x27d9, 0x312e, 0x3a2f, 0xef6d, 0xd9eb, 0x8153,
          0xdcba, 0x9fa3, 0x9f80, 0xead5, 0x134d, 0x2ebb, 0x5ec0, 0xe032},
         {0x1cb6, 0x5a61, 0x1bed, 0x77d6, 0xd5d1, 0x7498, 0xef33, 0x2dd2,
          0x1089, 0xedbd, 0x6958, 0x16ae, 0x336c, 0x45e6, 0x4361, 0xbadc}},
        /* example doing 267 (g,(g-f)/2) steps; 676 divsteps */
        {{0x0207, 0xf948, 0xc430, 0xf36b, 0xf0a7, 0x5d36, 0x751f, 0x132c,
          0x6f25, 0xa630, 0xca1f, 0xc967, 0xaf9c, 0x34e7, 0xa38f, 0xbe9f},
         {0x5fb9, 0x7321, 0x6561, 0x5fed, 0x54ec, 0x9c3a, 0xee0e, 0x6717,
          0x49af, 0xb896, 0xf4f5, 0x451c, 0x722a, 0xf116, 0x64a9, 0xcf0b},
         {0xf4d7, 0xdb47, 0xfef2, 0x4806, 0x4cb8, 0x18c7, 0xd9a7, 0x4951,
          0x14d8, 0x5c3a, 0xd22d, 0xd7b2, 0x750c, 0x3de7, 0x8b4a, 0x19aa}},

        /* Test cases triggering edge cases in divsteps variant starting with delta=1/2 */

        /* example needing 590 divsteps; delta=-5/2..7/2 */
        {{0x9118, 0xb640, 0x53d7, 0x30ab, 0x2a23, 0xd907, 0x9323, 0x5b3a,
          0xb6d4, 0x538a, 0x7637, 0xfe97, 0xfd05, 0x3cc0, 0x453a, 0xfb7e},
         {0x6983, 0x4f75, 0x4ad1, 0x48ad, 0xb2d9, 0x521d, 0x3dbc, 0x9cc0,
          0x4b60, 0x0ac6, 0xd3be, 0x0fb6, 0xd305, 0x3895, 0x2da5, 0xfdf8},
         {0xcec1, 0x33ac, 0xa801, 0x8194, 0xe36c, 0x65ef, 0x103b, 0xca54,
          0xfa9b, 0xb41d, 0x9b52, 0xb6f7, 0xa611, 0x84aa, 0x3493, 0xbf54}},
        /* example needing 590 divsteps; delta=-3/2..5/2 */
        {{0xb5f2, 0x42d0, 0x35e8, 0x8ca0, 0x4b62, 0x6e1d, 0xbdf3, 0x890e,
          0x8c82, 0x23d8, 0xc79a, 0xc8e8, 0x789e, 0x353d, 0x9766, 0xea9d},
         {0x6fa1, 0xacba, 0x4b7a, 0x5de1, 0x95d0, 0xc845, 0xebbf, 0x6f5a,
          0x30cf, 0x52db, 0x69b7, 0xe278, 0x4b15, 0x8411, 0x2ab2, 0xf3e7},
         {0xf12c, 0x9d6d, 0x95fa, 0x1878, 0x9f13, 0x4fb5, 0x3c8b, 0xa451,
          0x7182, 0xc4b6, 0x7e2a, 0x7bb7, 0x6e0e, 0x5b68, 0xde55, 0x9927}},
        /* example needing 590 divsteps; delta=-3/2..5/2 */
        {{0x229c, 0x4ef8, 0x1e93, 0xe5dc, 0xcde5, 0x6d62, 0x263b, 0xad11,
          0xced0, 0x88ff, 0xae8e, 0x3183, 0x11d2, 0xa50b, 0x350d, 0xeb40},
         {0x3157, 0xe2ea, 0x8a02, 0x0aa3, 0x5ae1, 0xb26c, 0xea27, 0x6805,
          0x87e2, 0x9461, 0x37c1, 0x2f8d, 0x85d2, 0x77a8, 0xf805, 0xeec9},
         {0x6f4e, 0x2748, 0xf7e5, 0xd8d3, 0xabe2, 0x7270, 0xc4e0, 0xedc7,
          0xf196, 0x78ca, 0x9139, 0xd8af, 0x72c6, 0xaf2f, 0x85d2, 0x6cd3}},
        /* example needing 590 divsteps; delta=-5/2..7/2 */
        {{0xdce8, 0xf1fe, 0x6708, 0x021e, 0xf1ca, 0xd609, 0x5443, 0x85ce,
          0x7a05, 0x8f9c, 0x90c3, 0x52e7, 0x8e1d, 0x97b8, 0xc0bf, 0xf2a1},
         {0xbd3d, 0xed11, 0x1625, 0xb4c5, 0x844c, 0xa413, 0x2569, 0xb9ba,
          0xcd35, 0xff84, 0xcd6e, 0x7f0b, 0x7d5d, 0x10df, 0x3efe, 0xfbe5},
         {0xa9dd, 0xafef, 0xb1b7, 0x4c8d, 0x50e4, 0xafbf, 0x2d5a, 0xb27c,
          0x0653, 0x66b6, 0x5d36, 0x4694, 0x7e35, 0xc47c, 0x857f, 0x32c5}},
        /* example needing 590 divsteps; delta=-3/2..5/2 */
        {{0x7902, 0xc9f8, 0x926b, 0xaaeb, 0x90f8, 0x1c89, 0xcce3, 0x96b7,
          0x28b2, 0x87a2, 0x136d, 0x695a, 0xa8df, 0x9061, 0x9e31, 0xee82},
         {0xd3a9, 0x3c02, 0x818c, 0x6b81, 0x34b3, 0xebbb, 0xe2c8, 0x7712,
          0xbfd6, 0x8248, 0xa6f4, 0xba6f, 0x03bb, 0xfb54, 0x7575, 0xfe89},
         {0x8246, 0x0d63, 0x478e, 0xf946, 0xf393, 0x0451, 0x08c2, 0x5919,
          0x5fd6, 0x4c61, 0xbeb7, 0x9a15, 0x30e1, 0x55fc, 0x6a01, 0x3724}},
        /* example reaching delta=-127/2..129/2; 571 divsteps */
        {{0x3eff, 0x926a, 0x77f5, 0x1fff, 0x1a5b, 0xf3ef, 0xf64b, 0x8681,
          0xf800, 0xf9bc, 0x761d, 0xe268, 0x62b0, 0xa032, 0xba9c, 0xbe56},
         {0xb8f9, 0x00e7, 0x47b7, 0xdffc, 0xfd9d, 0x5abb, 0xa19b, 0x1868,
          0x31fd, 0x3b29, 0x3674, 0x5449, 0xf54d, 0x1d19, 0x6ac7, 0xff6f},
         {0xf1d7, 0x3551, 0x5682, 0x9adf, 0xe8aa, 0x19a5, 0x8340, 0x71db,
          0xb7ab, 0x4cfd, 0xf661, 0x632c, 0xc27e, 0xd3c6, 0xdf42, 0xd306}},
        /* example reaching delta=-127/2..129/2; 571 divsteps */
        {{0x0000, 0x0000, 0x0000, 0x0000, 0x3aff, 0x2ed7, 0xf2e0, 0xabc7,
          0x8aee, 0x166e, 0x7ed0, 0x9ac7, 0x714a, 0xb9c5, 0x4d58, 0xad6c},
         {0x9cf9, 0x47e2, 0xa421, 0xb277, 0xffc2, 0x2747, 0x6486, 0x94c1,
          0x1d99, 0xd49b, 0x1096, 0x991a, 0xe986, 0xae02, 0xe89b, 0xea36},
         {0x1fb4, 0x98d8, 0x19b7, 0x80e9, 0xcdac, 0xaa5a, 0xf1e6, 0x0074,
          0xe393, 0xed8b, 0x8d5c, 0xe17d, 0x81b3, 0xc16d, 0x54d3, 0x9be3}},
        /* example reaching delta=-127/2..129/2; 571 divsteps */
        {{0xd047, 0x7e36, 0x3157, 0x7ab6, 0xb4d9, 0x8dae, 0x7534, 0x4f5d,
          0x489e, 0xa8ab, 0x8a3d, 0xd52c, 0x62af, 0xa032, 0xba9c, 0xbe56},
         {0xb1f1, 0x737f, 0x5964, 0x5afb, 0x3712, 0x8ef9, 0x19f7, 0x9669,
          0x664d, 0x03ad, 0xc352, 0xf7a5, 0xf545, 0x1d19, 0x6ac7, 0xff6f},
         {0xa834, 0x5256, 0x27bc, 0x33bd, 0xba11, 0x5a7b, 0x791e, 0xe6c0,
          0x9ac4, 0x9370, 0x1130, 0x28b4, 0x2b2e, 0x231b, 0x082a, 0x796e}},
        /* example doing 123 consecutive (f,g/2) steps; 554 divsteps */
        {{0x6ab1, 0x6ea0, 0x1a99, 0xe0c2, 0xdd45, 0x645d, 0x8dbc, 0x466a,
          0xfa64, 0x4289, 0xd3f7, 0xfc8f, 0x2894, 0xe3c5, 0xa008, 0xcc14},
         {0xc75f, 0xc083, 0x4cc2, 0x64f2, 0x2aff, 0x4c12, 0x8461, 0xc4ae,
          0xbbfa, 0xb336, 0xe4b2, 0x3ac5, 0x2c22, 0xf56c, 0x5381, 0xe943},
         {0xcd80, 0x760d, 0x4395, 0xb3a6, 0xd497, 0xf583, 0x82bd, 0x1daa,
          0xbe92, 0x2613, 0xfdfb, 0x869b, 0x0425, 0xa333, 0x7056, 0xc9c5}},
        /* example doing 123 consecutive (f,g/2) steps; 554 divsteps */
        {{0x71d4, 0x64df, 0xec4f, 0x74d8, 0x7e0c, 0x40d3, 0x7073, 0x4cc8,
          0x2a2a, 0xb1ff, 0x8518, 0x6513, 0xb0ea, 0x640a, 0x62d9, 0xd5f4},
         {0xdc75, 0xd937, 0x3b13, 0x1d36, 0xdf83, 0xd034, 0x1c1c, 0x4332,
          0x4cc3, 0xeeec, 0x7d94, 0x6771, 0x3384, 0x74b0, 0x947d, 0xf2c4},
         {0x0a82, 0x37a4, 0x12d5, 0xec97, 0x972c, 0xe6bf, 0xc348, 0xa0a9,
          0xc50c, 0xdc7c, 0xae30, 0x19d1, 0x0fca, 0x35e1, 0xd6f6, 0x81ee}},
        /* example doing 123 consecutive (f,g/2) steps; 554 divsteps */
        {{0xa6b1, 0xabc5, 0x5bbc, 0x7f65, 0xdd32, 0xaa73, 0xf5a3, 0x1982,
          0xced4, 0xe949, 0x0fd6, 0x2bc4, 0x2bd7, 0xe3c5, 0xa008, 0xcc14},
         {0x4b5f, 0x8f96, 0xa375, 0xfbcf, 0x1c7d, 0xf1ec, 0x03f5, 0xb35d,
          0xb999, 0xdb1f, 0xc9a1, 0xb4c7, 0x1dd5, 0xf56c, 0x5381, 0xe943},
         {0xaa3d, 0x38b9, 0xf17d, 0xeed9, 0x9988, 0x69ee, 0xeb88, 0x1495,
          0x203f, 0x18c8, 0x82b7, 0xdcb2, 0x34a7, 0x6b00, 0x6998, 0x589a}},
        /* example doing 453 (f,g/2) steps; 514 divsteps */
        {{0xa478, 0xe60d, 0x3244, 0x60e6, 0xada3, 0xfe50, 0xb6b1, 0x2eae,
          0xd0ef, 0xa7b1, 0xef63, 0x05c0, 0xe213, 0x443e, 0x4427, 0x2448},
         {0x258f, 0xf9ef, 0xe02b, 0x92dd, 0xd7f3, 0x252b, 0xa503, 0x9089,
          0xedff, 0x96c1, 0xfe3a, 0x3a39, 0x198a, 0x981d, 0x0627, 0xedb7},
         {0x595a, 0x45be, 0x8fb0, 0x2265, 0xc210, 0x02b8, 0xdce9, 0xe241,
          0xcab6, 0xbf0d, 0x0049, 0x8d9a, 0x2f51, 0xae54, 0x5785, 0xb411}},
        /* example doing 453 (f,g/2) steps; 514 divsteps */
        {{0x48f0, 0x7db3, 0xdafe, 0x1c92, 0x5912, 0xe11a, 0xab52, 0xede1,
          0x3182, 0x8980, 0x5d2b, 0x9b5b, 0x8718, 0xda27, 0x1683, 0x1de2},
         {0x168f, 0x6f36, 0xce7a, 0xf435, 0x19d4, 0xda5e, 0x2351, 0x9af5,
          0xb003, 0x0ef5, 0x3b4c, 0xecec, 0xa9f0, 0x78e1, 0xdfef, 0xe823},
         {0x5f55, 0xfdcc, 0xb233, 0x2914, 0x84f0, 0x97d1, 0x9cf4, 0x2159,
          0xbf56, 0xb79c, 0x17a3, 0x7cef, 0xd5de, 0x34f0, 0x5311, 0x4c54}},
        /* example doing 510 (f,(f+g)/2) steps; 512 divsteps */
        {{0x2789, 0x2e04, 0x6e0e, 0xb6cd, 0xe4de, 0x4dbf, 0x228d, 0x7877,
          0xc335, 0x806b, 0x38cd, 0x8049, 0xa73b, 0xcfa2, 0x82f7, 0x9e19},
         {0xc08d, 0xb99d, 0xb8f3, 0x663d, 0xbbb3, 0x1284, 0x1485, 0x1d49,
          0xc98f, 0x9e78, 0x1588, 0x11e3, 0xd91a, 0xa2c7, 0xfff1, 0xc7b9},
         {0x1e1f, 0x411d, 0x7c49, 0x0d03, 0xe789, 0x2f8e, 0x5d55, 0xa95e,
          0x826e, 0x8de5, 0x52a0, 0x1abc, 0x4cd7, 0xd13a, 0x4395, 0x63e1}},
        /* example doing 510 (f,(f+g)/2) steps; 512 divsteps */
        {{0xd5a1, 0xf786, 0x555c, 0xb14b, 0x44ae, 0x535f, 0x4a49, 0xffc3,
          0xf497, 0x70d1, 0x57c8, 0xa933, 0xc85a, 0x1910, 0x75bf, 0x960b},
         {0xfe53, 0x5058, 0x496d, 0xfdff, 0x6fb8, 0x4100, 0x92bd, 0xe0c4,
          0xda89, 0xe0a4, 0x841b, 0x43d4, 0xa388, 0x957f, 0x99ca, 0x9abf},
         {0xe530, 0x05bc, 0xfeec, 0xfc7e, 0xbcd3, 0x1239, 0x54cb, 0x7042,
          0xbccb, 0x139e, 0x9076, 0x0203, 0x6068, 0x90c7, 0x1ddf, 0x488d}},
        /* example doing 228 (g,(g-f)/2) steps; 538 divsteps */
        {{0x9488, 0xe54b, 0x0e43, 0x81d2, 0x06e7, 0x4b66, 0x36d0, 0x53d6,
          0x2b68, 0x22ec, 0x3fa9, 0xc1a7, 0x9ad2, 0xa596, 0xb3ac, 0xdf42},
         {0xe31f, 0x0b28, 0x5f3b, 0xc1ff, 0x344c, 0xbf5f, 0xd2ec, 0x2936,
          0x9995, 0xdeb2, 0xae6c, 0x2852, 0xa2c6, 0xb306, 0x8120, 0xe305},
         {0xa56e, 0xfb98, 0x1537, 0x4d85, 0x619e, 0x866c, 0x3cd4, 0x779a,
          0xdd66, 0xa80d, 0xdc2f, 0xcae4, 0xc74c, 0x5175, 0xa65d, 0x605e}},
        /* example doing 228 (g,(g-f)/2) steps; 537 divsteps */
        {{0x8cd5, 0x376d, 0xd01b, 0x7176, 0x19ef, 0xcf09, 0x8403, 0x5e52,
          0x83c1, 0x44de, 0xb91e, 0xb33d, 0xe15c, 0x51e7, 0xbad8, 0x6359},
         {0x3b75, 0xf812, 0x5f9e, 0xa04e, 0x92d3, 0x226e, 0x540e, 0x7c9a,
          0x31c6, 0x46d2, 0x0b7b, 0xdb4a, 0xe662, 0x4950, 0x0265, 0xf76f},
         {0x09ed, 0x692f, 0xe8f1, 0x3482, 0xab54, 0x36b4, 0x8442, 0x6ae9,
          0x4329, 0x6505, 0x183b, 0x1c1d, 0x482d, 0x7d63, 0xb44f, 0xcc09}},

        /* Test cases with the group order as modulus. */

        /* Test case with the group order as modulus, needing 635 divsteps. */
        {{0x95ed, 0x6c01, 0xd113, 0x5ff1, 0xd7d0, 0x29cc, 0x5817, 0x6120,
          0xca8e, 0xaad1, 0x25ae, 0x8e84, 0x9af6, 0x30bf, 0xf0ed, 0x1686},
         {0x4141, 0xd036, 0x5e8c, 0xbfd2, 0xa03b, 0xaf48, 0xdce6, 0xbaae,
          0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x1631, 0xbf4a, 0x286a, 0x2716, 0x469f, 0x2ac8, 0x1312, 0xe9bc,
          0x04f4, 0x304b, 0x9931, 0x113b, 0xd932, 0xc8f4, 0x0d0d, 0x01a1}},
        /* example with group size as modulus needing 631 divsteps */
        {{0x85ed, 0xc284, 0x9608, 0x3c56, 0x19b6, 0xbb5b, 0x2850, 0xdab7,
          0xa7f5, 0xe9ab, 0x06a4, 0x5bbb, 0x1135, 0xa186, 0xc424, 0xc68b},
         {0x4141, 0xd036, 0x5e8c, 0xbfd2, 0xa03b, 0xaf48, 0xdce6, 0xbaae,
          0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x8479, 0x450a, 0x8fa3, 0xde05, 0xb2f5, 0x7793, 0x7269, 0xbabb,
          0xc3b3, 0xd49b, 0x3377, 0x03c6, 0xe694, 0xc760, 0xd3cb, 0x2811}},
        /* example with group size as modulus needing 565 divsteps starting at delta=1/2 */
        {{0x8432, 0x5ceb, 0xa847, 0x6f1e, 0x51dd, 0x535a, 0x6ddc, 0x70ce,
          0x6e70, 0xc1f6, 0x18f2, 0x2a7e, 0xc8e7, 0x39f8, 0x7e96, 0xebbf},
         {0x4141, 0xd036, 0x5e8c, 0xbfd2, 0xa03b, 0xaf48, 0xdce6, 0xbaae,
          0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x257e, 0x449f, 0x689f, 0x89aa, 0x3989, 0xb661, 0x376c, 0x1e32,
          0x654c, 0xee2e, 0xf4e2, 0x33c8, 0x3f2f, 0x9716, 0x6046, 0xcaa3}},
        /* Test case with the group size as modulus, needing 981 divsteps with
           broken eta handling. */
        {{0xfeb9, 0xb877, 0xee41, 0x7fa3, 0x87da, 0x94c4, 0x9d04, 0xc5ae,
          0x5708, 0x0994, 0xfc79, 0x0916, 0xbf32, 0x3ad8, 0xe11c, 0x5ca2},
         {0x4141, 0xd036, 0x5e8c, 0xbfd2, 0xa03b, 0xaf48, 0xdce6, 0xbaae,
          0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x0f12, 0x075e, 0xce1c, 0x6f92, 0xc80f, 0xca92, 0x9a04, 0x6126,
          0x4b6c, 0x57d6, 0xca31, 0x97f3, 0x1f99, 0xf4fd, 0xda4d, 0x42ce}},
        /* Test case with the group size as modulus, input = 0. */
        {{0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
          0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
         {0x4141, 0xd036, 0x5e8c, 0xbfd2, 0xa03b, 0xaf48, 0xdce6, 0xbaae,
          0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
          0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
        /* Test case with the group size as modulus, input = 1. */
        {{0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
          0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
         {0x4141, 0xd036, 0x5e8c, 0xbfd2, 0xa03b, 0xaf48, 0xdce6, 0xbaae,
          0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
          0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
        /* Test case with the group size as modulus, input = 2. */
        {{0x0002, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
          0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
         {0x4141, 0xd036, 0x5e8c, 0xbfd2, 0xa03b, 0xaf48, 0xdce6, 0xbaae,
          0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x20a1, 0x681b, 0x2f46, 0xdfe9, 0x501d, 0x57a4, 0x6e73, 0x5d57,
          0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x7fff}},
        /* Test case with the group size as modulus, input = group - 1. */
        {{0x4140, 0xd036, 0x5e8c, 0xbfd2, 0xa03b, 0xaf48, 0xdce6, 0xbaae,
          0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x4141, 0xd036, 0x5e8c, 0xbfd2, 0xa03b, 0xaf48, 0xdce6, 0xbaae,
          0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x4140, 0xd036, 0x5e8c, 0xbfd2, 0xa03b, 0xaf48, 0xdce6, 0xbaae,
          0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff}},

        /* Test cases with the field size as modulus. */

        /* Test case with the field size as modulus, needing 637 divsteps. */
        {{0x9ec3, 0x1919, 0xca84, 0x7c11, 0xf996, 0x06f3, 0x5408, 0x6688,
          0x1320, 0xdb8a, 0x632a, 0x0dcb, 0x8a84, 0x6bee, 0x9c95, 0xe34e},
         {0xfc2f, 0xffff, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
          0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x18e5, 0x19b6, 0xdf92, 0x1aaa, 0x09fb, 0x8a3f, 0x52b0, 0x8701,
          0xac0c, 0x2582, 0xda44, 0x9bcc, 0x6828, 0x1c53, 0xbd8f, 0xbd2c}},
        /* example with field size as modulus needing 637 divsteps */
        {{0xaec3, 0xa7cf, 0x2f2d, 0x0693, 0x5ad5, 0xa8ff, 0x7ec7, 0x30ff,
          0x0c8b, 0xc242, 0xcab2, 0x063a, 0xf86e, 0x6057, 0x9cbd, 0xf6d8},
         {0xfc2f, 0xffff, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
          0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x0310, 0x579d, 0xcb38, 0x9030, 0x3ded, 0x9bb9, 0x1234, 0x63ce,
          0x0c63, 0x8e3d, 0xacfe, 0x3c20, 0xdc85, 0xf859, 0x919e, 0x1d45}},
        /* example with field size as modulus needing 564 divsteps starting at delta=1/2 */
        {{0x63ae, 0x8d10, 0x0071, 0xdb5c, 0xb454, 0x78d1, 0x744a, 0x5f8e,
          0xe4d8, 0x87b1, 0x8e62, 0x9590, 0xcede, 0xa070, 0x36b4, 0x7f6f},
         {0xfc2f, 0xffff, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
          0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0xfdc8, 0xe8d5, 0xbe15, 0x9f86, 0xa5fe, 0xf18e, 0xa7ff, 0xd291,
          0xf4c2, 0x9c87, 0xf150, 0x073e, 0x69b8, 0xf7c4, 0xee4b, 0xc7e6}},
        /* Test case with the field size as modulus, needing 935 divsteps with
           broken eta handling. */
        {{0x1b37, 0xbdc3, 0x8bcd, 0x25e3, 0x1eae, 0x567d, 0x30b6, 0xf0d8,
          0x9277, 0x0cf8, 0x9c2e, 0xecd7, 0x631d, 0xe38f, 0xd4f8, 0x5c93},
         {0xfc2f, 0xffff, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
          0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x1622, 0xe05b, 0xe880, 0x7de9, 0x3e45, 0xb682, 0xee6c, 0x67ed,
          0xa179, 0x15db, 0x6b0d, 0xa656, 0x7ccb, 0x8ef7, 0xa2ff, 0xe279}},
        /* Test case with the field size as modulus, input = 0. */
        {{0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
          0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
         {0xfc2f, 0xffff, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
          0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
          0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
        /* Test case with the field size as modulus, input = 1. */
        {{0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
          0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
         {0xfc2f, 0xffff, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
          0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
          0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
        /* Test case with the field size as modulus, input = 2. */
        {{0x0002, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
          0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
         {0xfc2f, 0xffff, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
          0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0xfe18, 0x7fff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
          0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x7fff}},
        /* Test case with the field size as modulus, input = field - 1. */
        {{0xfc2e, 0xffff, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
          0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0xfc2f, 0xffff, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
          0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
         {0xfc2e, 0xffff, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
          0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff}},

         /* Selected from a large number of random inputs to reach small/large
          * d/e values in various configurations. */
        {{0x3a08, 0x23e1, 0x4d8c, 0xe606, 0x3263, 0x67af, 0x9bf1, 0x9d70,
          0xf5fd, 0x12e4, 0x03c8, 0xb9ca, 0xe847, 0x8c5d, 0x6322, 0xbd30},
         {0x8359, 0x59dd, 0x1831, 0x7c1a, 0x1e83, 0xaee1, 0x770d, 0xcea8,
          0xfbb1, 0xeed6, 0x10b5, 0xe2c6, 0x36ea, 0xee17, 0xe32c, 0xffff},
         {0x1727, 0x0f36, 0x6f85, 0x5d0c, 0xca6c, 0x3072, 0x9628, 0x5842,
          0xcb44, 0x7c2b, 0xca4f, 0x62e5, 0x29b1, 0x6ffd, 0x9055, 0xc196}},
        {{0x905d, 0x41c8, 0xa2ff, 0x295b, 0x72bb, 0x4679, 0x6d01, 0x2c98,
          0xb3e0, 0xc537, 0xa310, 0xe07e, 0xe72f, 0x4999, 0x1148, 0xf65e},
         {0x5b41, 0x4239, 0x3c37, 0x5130, 0x30e3, 0xff35, 0xc51f, 0x1a43,
          0xdb23, 0x13cf, 0x9f49, 0xf70c, 0x5e70, 0xd411, 0x3005, 0xf8c6},
         {0xc30e, 0x68f0, 0x201a, 0xe10c, 0x864a, 0x6243, 0xe946, 0x43ae,
          0xf3f1, 0x52dc, 0x1f7f, 0x50d4, 0x2797, 0x064c, 0x5ca4, 0x90e3}},
        {{0xf1b5, 0xc6e5, 0xd2c4, 0xff95, 0x27c5, 0x0c92, 0x5d19, 0x7ae5,
          0x4fbe, 0x5438, 0x99e1, 0x880d, 0xd892, 0xa05c, 0x6ffd, 0x7eac},
         {0x2153, 0xcc9d, 0xfc6c, 0x8358, 0x49a1, 0x01e2, 0xcef0, 0x4969,
          0xd69a, 0x8cef, 0xf5b2, 0xfd95, 0xdcc2, 0x71f4, 0x6ae2, 0xceeb},
         {0x9b2e, 0xcdc6, 0x0a5c, 0x7317, 0x9084, 0xe228, 0x56cf, 0xd512,
          0x628a, 0xce21, 0x3473, 0x4e13, 0x8823, 0x1ed0, 0x34d0, 0xbfa3}},
        {{0x5bae, 0x53e5, 0x5f4d, 0x21ca, 0xb875, 0x8ecf, 0x9aa6, 0xbe3c,
          0x9f96, 0x7b82, 0x375d, 0x4d3e, 0x491c, 0xb1eb, 0x04c9, 0xb6c8},
         {0xfcfd, 0x10b7, 0x73b2, 0xd23b, 0xa357, 0x67da, 0x0d9f, 0x8702,
          0xa037, 0xff8e, 0x0e8b, 0x1801, 0x2c5c, 0x4e6e, 0x4558, 0xfff2},
         {0xc50f, 0x5654, 0x6713, 0x5ef5, 0xa7ce, 0xa647, 0xc832, 0x69ce,
          0x1d5c, 0x4310, 0x0746, 0x5a01, 0x96ea, 0xde4b, 0xa88b, 0x5543}},
        {{0xdc7f, 0x5e8c, 0x89d1, 0xb077, 0xd521, 0xcf90, 0x32fa, 0x5737,
          0x839e, 0x1464, 0x007c, 0x09c6, 0x9371, 0xe8ea, 0xc1cb, 0x75c4},
         {0xe3a3, 0x107f, 0xa82a, 0xa375, 0x4578, 0x60f4, 0x75c9, 0x5ee4,
          0x3fd7, 0x2736, 0x2871, 0xd3d2, 0x5f1d, 0x1abb, 0xa764, 0xffff},
         {0x45c6, 0x1f2e, 0xb14c, 0x84d7, 0x7bb7, 0x5a04, 0x0504, 0x3f33,
          0x5cc1, 0xb07a, 0x6a6c, 0x786f, 0x647f, 0xe1d7, 0x78a2, 0x4cf4}},
        {{0xc006, 0x356f, 0x8cd2, 0x967b, 0xb49e, 0x2d4e, 0x14bf, 0x4bcb,
          0xddab, 0xd3f9, 0xa068, 0x2c1c, 0xd242, 0xa56d, 0xf2c7, 0x5f97},
         {0x465b, 0xb745, 0x0e0d, 0x69a9, 0x987d, 0xcb37, 0xf637, 0xb311,
          0xc4d6, 0x2ddb, 0xf68f, 0x2af9, 0x959d, 0x3f53, 0x98f2, 0xf640},
         {0xc0f2, 0x6bfb, 0xf5c3, 0x91c1, 0x6b05, 0x0825, 0x5ca0, 0x7df7,
          0x9d55, 0x6d9e, 0xfe94, 0x2ad9, 0xd9f0, 0xe68b, 0xa72b, 0xd1b2}},
        {{0x2279, 0x61ba, 0x5bc6, 0x136b, 0xf544, 0x717c, 0xafda, 0x02bd,
          0x79af, 0x1fad, 0xea09, 0x81bb, 0x932b, 0x32c9, 0xdf1d, 0xe576},
         {0x8215, 0x7817, 0xca82, 0x43b0, 0x9b06, 0xea65, 0x1291, 0x0621,
          0x0089, 0x46fe, 0xc5a6, 0xddd7, 0x8065, 0xc6a0, 0x214b, 0xfc64},
         {0x04bf, 0x6f2a, 0x86b2, 0x841a, 0x4a95, 0xc632, 0x97b7, 0x5821,
          0x2b18, 0x1bb0, 0x3e97, 0x935e, 0xcc7d, 0x066b, 0xd513, 0xc251}},
        {{0x76e8, 0x5bc2, 0x3eaa, 0x04fc, 0x9974, 0x92c1, 0x7c15, 0xfa89,
          0x1151, 0x36ee, 0x48b2, 0x049c, 0x5f16, 0xcee4, 0x925b, 0xe98e},
         {0x913f, 0x0a2d, 0xa185, 0x9fea, 0xda5a, 0x4025, 0x40d7, 0x7cfa,
          0x88ca, 0xbbe8, 0xb265, 0xb7e4, 0x6cb1, 0xed64, 0xc6f9, 0xffb5},
         {0x6ab1, 0x1a86, 0x5009, 0x152b, 0x1cc4, 0xe2c8, 0x960b, 0x19d0,
          0x3554, 0xc562, 0xd013, 0xcf91, 0x10e1, 0x7933, 0xe195, 0xcf49}},
        {{0x9cb5, 0xd2d7, 0xc6ed, 0xa818, 0xb495, 0x06ee, 0x0f4a, 0x06e3,
          0x4c5a, 0x80ce, 0xd49a, 0x4cd7, 0x7487, 0x92af, 0xe516, 0x676c},
         {0xd6e9, 0x6b85, 0x619a, 0xb52c, 0x20a0, 0x2f79, 0x3545, 0x1edd,
          0x5a6f, 0x8082, 0x9b80, 0xf8f8, 0xc78a, 0xd0a3, 0xadf4, 0xffff},
         {0x01c2, 0x2118, 0xef5e, 0xa877, 0x046a, 0xd2c2, 0x2ad5, 0x951c,
          0x8900, 0xa5c9, 0x8d0f, 0x6b61, 0x55d3, 0xd572, 0x48de, 0x9219}},
        {{0x5114, 0x0644, 0x23dd, 0x01d3, 0xc101, 0xa659, 0xea17, 0x640f,
          0xf767, 0x2644, 0x9cec, 0xd8ba, 0xd6da, 0x9156, 0x8aeb, 0x875a},
         {0xc1bf, 0xdae9, 0xe96b, 0xce77, 0xf7a1, 0x3e99, 0x5c2e, 0x973b,
          0xd048, 0x5bd0, 0x4e8a, 0xcb85, 0xce39, 0x37f5, 0x815d, 0xffff},
         {0x48cc, 0x35b6, 0x26d4, 0x2ea6, 0x50d6, 0xa2f9, 0x64b6, 0x03bf,
          0xd00c, 0xe057, 0x3343, 0xfb79, 0x3ce5, 0xf717, 0xc5af, 0xe185}},
        {{0x13ff, 0x6c76, 0x2077, 0x16e0, 0xd5ca, 0xf2ad, 0x8dba, 0x8f49,
          0x7887, 0x16f9, 0xb646, 0xfc87, 0xfa31, 0x5096, 0xf08c, 0x3fbe},
         {0x8139, 0x6fd7, 0xf6df, 0xa7bf, 0x6699, 0x5361, 0x6f65, 0x13c8,
          0xf4d1, 0xe28f, 0xc545, 0x0a8c, 0x5274, 0xb0a6, 0xffff, 0xffff},
         {0x22ca, 0x0cd6, 0xc1b5, 0xb064, 0x44a7, 0x297b, 0x495f, 0x34ac,
          0xfa95, 0xec62, 0xf08d, 0x621c, 0x66a6, 0xba94, 0x84c6, 0x8ee0}},
        {{0xaa30, 0x312e, 0x439c, 0x4e88, 0x2e2f, 0x32dc, 0xb880, 0xa28e,
          0xf795, 0xc910, 0xb406, 0x8dd7, 0xb187, 0xa5a5, 0x38f1, 0xe49e},
         {0xfb19, 0xf64a, 0xba6a, 0x8ec2, 0x7255, 0xce89, 0x2cf9, 0x9cba,
          0xe1fe, 0x50da, 0x1705, 0xac52, 0xe3d4, 0x4269, 0x0648, 0xfd77},
         {0xb4c8, 0x6e8a, 0x2b5f, 0x4c2d, 0x5a67, 0xa7bb, 0x7d6d, 0x5569,
          0xa0ea, 0x244a, 0xc0f2, 0xf73d, 0x58cf, 0xac7f, 0xd32b, 0x3018}},
        {{0xc953, 0x1ae1, 0xae46, 0x8709, 0x19c2, 0xa986, 0x9abe, 0x1611,
          0x0395, 0xd5ab, 0xf0f6, 0xb5b0, 0x5b2b, 0x0317, 0x80ba, 0x376d},
         {0xfe77, 0xbc03, 0xac2f, 0x9d00, 0xa175, 0x293d, 0x3b56, 0x0e3a,
          0x0a9c, 0xf40c, 0x690e, 0x1508, 0x95d4, 0xddc4, 0xe805, 0xffff},
         {0xb1ce, 0x0929, 0xa5fe, 0x4b50, 0x9d5d, 0x8187, 0x2557, 0x4376,
          0x11ba, 0xdcef, 0xc1f3, 0xd531, 0x1824, 0x93f6, 0xd81f, 0x8f83}},
        {{0xb8d2, 0xb900, 0x4a0c, 0x7188, 0xa5bf, 0x1b0b, 0x2ae5, 0xa35b,
          0x98e0, 0x610c, 0x86db, 0x2487, 0xa267, 0x002c, 0xebb6, 0xc5f4},
         {0x9cdd, 0x1c1b, 0x2f06, 0x43d1, 0xce47, 0xc334, 0x6e60, 0xc016,
          0x989e, 0x0ab2, 0x0cac, 0x1196, 0xe2d9, 0x2e04, 0xc62b, 0xffff},
         {0xdc36, 0x1f05, 0x6aa9, 0x7a20, 0x944f, 0x2fd3, 0xa553, 0xdb4f,
          0xbd5c, 0x3a75, 0x25d4, 0xe20e, 0xa387, 0x1410, 0xdbb1, 0x1b60}},
        {{0x76b3, 0x2207, 0x4930, 0x5dd7, 0x65a0, 0xd55c, 0xb443, 0x53b7,
          0x5c22, 0x818a, 0xb2e7, 0x9de8, 0x9985, 0xed45, 0x33b1, 0x53e8},
         {0x7913, 0x44e1, 0xf15b, 0x5edd, 0x34f3, 0x4eba, 0x0758, 0x7104,
          0x32d9, 0x28f3, 0x4401, 0x85c5, 0xb695, 0xb899, 0xc0f2, 0xffff},
         {0x7f43, 0xd202, 0x24c9, 0x69f3, 0x74dc, 0x1a69, 0xeaee, 0x5405,
          0x1755, 0x4bb8, 0x04e3, 0x2fd2, 0xada8, 0x39eb, 0x5b4d, 0x96ca}},
        {{0x807b, 0x7112, 0xc088, 0xdafd, 0x02fa, 0x9d95, 0x5e42, 0xc033,
          0xde0a, 0xeecf, 0x8e90, 0x8da1, 0xb17e, 0x9a5b, 0x4c6d, 0x1914},
         {0x4871, 0xd1cb, 0x47d7, 0x327f, 0x09ec, 0x97bb, 0x2fae, 0xd346,
          0x6b78, 0x3707, 0xfeb2, 0xa6ab, 0x13df, 0x76b0, 0x8fb9, 0xffb3},
         {0x179e, 0xb63b, 0x4784, 0x231e, 0x9f42, 0x7f1a, 0xa3fb, 0xdd8c,
          0xd1eb, 0xb4c9, 0x8ca7, 0x018c, 0xf691, 0x576c, 0xa7d6, 0xce27}},
        {{0x5f45, 0x7c64, 0x083d, 0xedd5, 0x08a0, 0x0c64, 0x6c6f, 0xec3c,
          0xe2fb, 0x352c, 0x9303, 0x75e4, 0xb4e0, 0x8b09, 0xaca4, 0x7025},
         {0x1025, 0xb482, 0xfed5, 0xa678, 0x8966, 0x9359, 0x5329, 0x98bb,
          0x85b2, 0x73ba, 0x9982, 0x6fdc, 0xf190, 0xbe8c, 0xdc5c, 0xfd93},
         {0x83a2, 0x87a4, 0xa680, 0x52a1, 0x1ba1, 0x8848, 0x5db7, 0x9744,
          0x409c, 0x0745, 0x0e1e, 0x1cfc, 0x00cd, 0xf573, 0x2071, 0xccaa}},
        {{0xf61f, 0x63d4, 0x536c, 0x9eb9, 0x5ddd, 0xbb11, 0x9014, 0xe904,
          0xfe01, 0x6b45, 0x1858, 0xcb5b, 0x4c38, 0x43e1, 0x381d, 0x7f94},
         {0xf61f, 0x63d4, 0xd810, 0x7ca3, 0x8a04, 0x4b83, 0x11fc, 0xdf94,
          0x4169, 0xbd05, 0x608e, 0x7151, 0x4fbf, 0xb31a, 0x38a7, 0xa29b},
         {0xe621, 0xdfa5, 0x3d06, 0x1d03, 0x81e6, 0x00da, 0x53a6, 0x965e,
          0x93e5, 0x2164, 0x5b61, 0x59b8, 0xa629, 0x8d73, 0x699a, 0x6111}},
        {{0x4cc3, 0xd29e, 0xf4a3, 0x3428, 0x2048, 0xeec9, 0x5f50, 0x99a4,
          0x6de9, 0x05f2, 0x5aa9, 0x5fd2, 0x98b4, 0x1adc, 0x225f, 0x777f},
         {0xe649, 0x37da, 0x5ba6, 0x5765, 0x3f4a, 0x8a1c, 0x2e79, 0xf550,
          0x1a54, 0xcd1e, 0x7218, 0x3c3c, 0x6311, 0xfe28, 0x95fb, 0xed97},
         {0xe9b6, 0x0c47, 0x3f0e, 0x849b, 0x11f8, 0xe599, 0x5e4d, 0xd618,
          0xa06d, 0x33a0, 0x9a3e, 0x44db, 0xded8, 0x10f0, 0x94d2, 0x81fb}},
        {{0x2e59, 0x7025, 0xd413, 0x455a, 0x1ce3, 0xbd45, 0x7263, 0x27f7,
          0x23e3, 0x518e, 0xbe06, 0xc8c4, 0xe332, 0x4276, 0x68b4, 0xb166},
         {0x596f, 0x0cf6, 0xc8ec, 0x787b, 0x04c1, 0x473c, 0xd2b8, 0x8d54,
          0x9cdf, 0x77f2, 0xd3f3, 0x6735, 0x0638, 0xf80e, 0x9467, 0xc6aa},
         {0xc7e7, 0x1822, 0xb62a, 0xec0d, 0x89cd, 0x7846, 0xbfa2, 0x35d5,
          0xfa38, 0x870f, 0x494b, 0x1697, 0x8b17, 0xf904, 0x10b6, 0x9822}},
        {{0x6d5b, 0x1d4f, 0x0aaf, 0x807b, 0x35fb, 0x7ee8, 0x00c6, 0x059a,
          0xddf0, 0x1fb1, 0xc38a, 0xd78e, 0x2aa4, 0x79e7, 0xad28, 0xc3f1},
         {0xe3bb, 0x174e, 0xe0a8, 0x74b6, 0xbd5b, 0x35f6, 0x6d23, 0x6328,
          0xc11f, 0x83e1, 0xf928, 0xa918, 0x838e, 0xbf43, 0xe243, 0xfffb},
         {0x9cf2, 0x6b8b, 0x3476, 0x9d06, 0xdcf2, 0xdb8a, 0x89cd, 0x4857,
          0x75c2, 0xabb8, 0x490b, 0xc9bd, 0x890e, 0xe36e, 0xd552, 0xfffa}},
        {{0x2f09, 0x9d62, 0xa9fc, 0xf090, 0xd6d1, 0x9d1d, 0x1828, 0xe413,
          0xc92b, 0x3d5a, 0x1373, 0x368c, 0xbaf2, 0x2158, 0x71eb, 0x08a3},
         {0x2f09, 0x1d62, 0x4630, 0x0de1, 0x06dc, 0xf7f1, 0xc161, 0x1e92,
          0x7495, 0x97e4, 0x94b6, 0xa39e, 0x4f1b, 0x18f8, 0x7bd4, 0x0c4c},
         {0xeb3d, 0x723d, 0x0907, 0x525b, 0x463a, 0x49a8, 0xc6b8, 0xce7f,
          0x740c, 0x0d7d, 0xa83b, 0x457f, 0xae8e, 0xc6af, 0xd331, 0x0475}},
        {{0x6abd, 0xc7af, 0x3e4e, 0x95fd, 0x8fc4, 0xee25, 0x1f9c, 0x0afe,
          0x291d, 0xcde0, 0x48f4, 0xb2e8, 0xf7af, 0x8f8d, 0x0bd6, 0x078d},
         {0x4037, 0xbf0e, 0x2081, 0xf363, 0x13b2, 0x381e, 0xfb6e, 0x818e,
          0x27e4, 0x5662, 0x18b0, 0x0cd2, 0x81f5, 0x9415, 0x0d6c, 0xf9fb},
         {0xd205, 0x0981, 0x0498, 0x1f08, 0xdb93, 0x1732, 0x0579, 0x1424,
          0xad95, 0x642f, 0x050c, 0x1d6d, 0xfc95, 0xfc4a, 0xd41b, 0x3521}},
        {{0xf23a, 0x4633, 0xaef4, 0x1a92, 0x3c8b, 0x1f09, 0x30f3, 0x4c56,
          0x2a2f, 0x4f62, 0xf5e4, 0x8329, 0x63cc, 0xb593, 0xec6a, 0xc428},
         {0x93a7, 0xfcf6, 0x606d, 0xd4b2, 0x2aad, 0x28b4, 0xc65b, 0x8998,
          0x4e08, 0xd178, 0x0900, 0xc82b, 0x7470, 0xa342, 0x7c0f, 0xffff},
         {0x315f, 0xf304, 0xeb7b, 0xe5c3, 0x1451, 0x6311, 0x8f37, 0x93a8,
          0x4a38, 0xa6c6, 0xe393, 0x1087, 0x6301, 0xd673, 0x4ec4, 0xffff}},
        {{0x892e, 0xeed0, 0x1165, 0xcbc1, 0x5545, 0xa280, 0x7243, 0x10c9,
          0x9536, 0x36af, 0xb3fc, 0x2d7c, 0xe8a5, 0x09d6, 0xe1d4, 0xe85d},
         {0xae09, 0xc28a, 0xd777, 0xbd80, 0x23d6, 0xf980, 0xeb7c, 0x4e0e,
          0xf7dc, 0x6475, 0xf10a, 0x2d33, 0x5dfd, 0x797a, 0x7f1c, 0xf71a},
         {0x4064, 0x8717, 0xd091, 0x80b0, 0x4527, 0x8442, 0xac8b, 0x9614,
          0xc633, 0x35f5, 0x7714, 0x2e83, 0x4aaa, 0xd2e4, 0x1acd, 0x0562}},
        {{0xdb64, 0x0937, 0x308b, 0x53b0, 0x00e8, 0xc77f, 0x2f30, 0x37f7,
          0x79ce, 0xeb7f, 0xde81, 0x9286, 0xafda, 0x0e62, 0xae00, 0x0067},
         {0x2cc7, 0xd362, 0xb161, 0x0557, 0x4ff2, 0xb9c8, 0x06fe, 0x5f2b,
          0xde33, 0x0190, 0x28c6, 0xb886, 0xee2b, 0x5a4e, 0x3289, 0x0185},
         {0x4215, 0x923e, 0xf34f, 0xb362, 0x88f8, 0xceec, 0xafdd, 0x7f42,
          0x0c57, 0x56b2, 0xa366, 0x6a08, 0x0826, 0xfb8f, 0x1b03, 0x0163}},
        {{0xa4ba, 0x8408, 0x810a, 0xdeba, 0x47a3, 0x853a, 0xeb64, 0x2f74,
          0x3039, 0x038c, 0x7fbb, 0x498e, 0xd1e9, 0x46fb, 0x5691, 0x32a4},
         {0xd749, 0xb49d, 0x20b7, 0x2af6, 0xd34a, 0xd2da, 0x0a10, 0xf781,
          0x58c9, 0x171f, 0x3cb6, 0x6337, 0x88cd, 0xcf1e, 0xb246, 0x7351},
         {0xf729, 0xcf0a, 0x96ea, 0x032c, 0x4a8f, 0x42fe, 0xbac8, 0xec65,
          0x1510, 0x0d75, 0x4c17, 0x8d29, 0xa03f, 0x8b7e, 0x2c49, 0x0000}},
        {{0x0fa4, 0x8e1c, 0x3788, 0xba3c, 0x8d52, 0xd89d, 0x12c8, 0xeced,
          0x9fe6, 0x9b88, 0xecf3, 0xe3c8, 0xac48, 0x76ed, 0xf23e, 0xda79},
         {0x1103, 0x227c, 0x5b00, 0x3fcf, 0xc5d0, 0x2d28, 0x8020, 0x4d1c,
          0xc6b9, 0x67f9, 0x6f39, 0x989a, 0xda53, 0x3847, 0xd416, 0xe0d0},
         {0xdd8e, 0xcf31, 0x3710, 0x7e44, 0xa511, 0x933c, 0x0cc3, 0x5145,
          0xf632, 0x5e1d, 0x038f, 0x5ce7, 0x7265, 0xda9d, 0xded6, 0x08f8}},
        {{0xe2c8, 0x91d5, 0xa5f5, 0x735f, 0x6b58, 0x56dc, 0xb39d, 0x5c4a,
          0x57d0, 0xa1c2, 0xd92f, 0x9ad4, 0xf7c4, 0x51dd, 0xaf5c, 0x0096},
         {0x1739, 0x7207, 0x7505, 0xbf35, 0x42de, 0x0a29, 0xa962, 0xdedf,
          0x53e8, 0x12bf, 0xcde7, 0xd8e2, 0x8d4d, 0x2c4b, 0xb1b1, 0x0628},
         {0x992d, 0xe3a7, 0xb422, 0xc198, 0x23ab, 0xa6ef, 0xb45d, 0x50da,
          0xa738, 0x014a, 0x2310, 0x85fb, 0x5fe8, 0x1b18, 0x1774, 0x03a7}},
        {{0x1f16, 0x2b09, 0x0236, 0xee90, 0xccf9, 0x9775, 0x8130, 0x4c91,
          0x9091, 0x310b, 0x6dc4, 0x86f6, 0xc2e8, 0xef60, 0xfc0e, 0xf3a4},
         {0x9f49, 0xac15, 0x02af, 0x110f, 0xc59d, 0x5677, 0xa1a9, 0x38d5,
          0x914f, 0xa909, 0x3a3a, 0x4a39, 0x3703, 0xea30, 0x73da, 0xffad},
         {0x15ed, 0xdd16, 0x83c7, 0x270a, 0x862f, 0xd8ad, 0xcaa1, 0x5f41,
          0x99a9, 0x3fc8, 0x7bb2, 0x360a, 0xb06d, 0xfadc, 0x1b36, 0xffa8}},
        {{0xc4e0, 0xb8fd, 0x5106, 0xe169, 0x754c, 0xa58c, 0xc413, 0x8224,
          0x5483, 0x63ec, 0xd477, 0x8473, 0x4778, 0x9281, 0x0000, 0x0000},
         {0x85e1, 0xff54, 0xb200, 0xe413, 0xf4f4, 0x4c0f, 0xfcec, 0xc183,
          0x60d3, 0x1b0c, 0x3834, 0x601c, 0x943c, 0xbe6e, 0x0002, 0x0000},
         {0xf4f8, 0xfd5e, 0x61ef, 0xece8, 0x9199, 0xe5c4, 0x05a6, 0xe6c3,
          0xc4ae, 0x8b28, 0x66b1, 0x8a95, 0x9ece, 0x8f4a, 0x0001, 0x0000}},
        {{0xeae9, 0xa1b4, 0xc6d8, 0x2411, 0x2b5a, 0x1dd0, 0x2dc9, 0xb57b,
          0x5ccd, 0x4957, 0xaf59, 0xa04b, 0x5f42, 0xab7c, 0x2826, 0x526f},
         {0xf407, 0x165a, 0xb724, 0x2f12, 0x2ea1, 0x470b, 0x4464, 0xbd35,
          0x606f, 0xd73e, 0x50d3, 0x8a7f, 0x8029, 0x7ffc, 0xbe31, 0x6cfb},
         {0x8171, 0x1f4c, 0xced2, 0x9c99, 0x6d7e, 0x5a0f, 0xfefb, 0x59e3,
          0xa0c8, 0xabd9, 0xc4c5, 0x57d3, 0xbfa3, 0x4f11, 0x96a2, 0x5a7d}},
        {{0xe068, 0x4cc0, 0x8bcd, 0xc903, 0x9e52, 0xb3e1, 0xd745, 0x0995,
          0xdd8f, 0xf14b, 0xd2ac, 0xd65a, 0xda1d, 0xa742, 0xbac5, 0x474c},
         {0x7481, 0xf2ad, 0x9757, 0x2d82, 0xb683, 0xb16b, 0x0002, 0x7b60,
          0x8f0c, 0x2594, 0x8f64, 0x3b7a, 0x3552, 0x8d9d, 0xb9d7, 0x67eb},
         {0xcaab, 0xb9a1, 0xf966, 0xe311, 0x5b34, 0x0fa0, 0x6abc, 0x8134,
          0xab3d, 0x90f6, 0x1984, 0x9232, 0xec17, 0x74e5, 0x2ceb, 0x434e}},
        {{0x0fb1, 0x7a55, 0x1a5c, 0x53eb, 0xd7b3, 0x7a01, 0xca32, 0x31f6,
          0x3b74, 0x679e, 0x1501, 0x6c57, 0xdb20, 0x8b7c, 0xd7d0, 0x8097},
         {0xb127, 0xb20c, 0xe3a2, 0x96f3, 0xe0d8, 0xd50c, 0x14b4, 0x0b40,
          0x6eeb, 0xa258, 0x99db, 0x3c8c, 0x0f51, 0x4198, 0x3887, 0xffd0},
         {0x0273, 0x9f8c, 0x9669, 0xbbba, 0x1c49, 0x767c, 0xc2af, 0x59f0,
          0x1366, 0xd397, 0x63ac, 0x6fe8, 0x1a9a, 0x1259, 0x01d0, 0x0016}},
        {{0x7876, 0x2a35, 0xa24a, 0x433e, 0x5501, 0x573c, 0xd76d, 0xcb82,
          0x1334, 0xb4a6, 0xf290, 0xc797, 0xeae9, 0x2b83, 0x1e2b, 0x8b14},
         {0x3885, 0x8aef, 0x9dea, 0x2b8c, 0xdd7c, 0xd7cd, 0xb0cc, 0x05ee,
          0x361b, 0x3800, 0xb0d4, 0x4c23, 0xbd3f, 0x5180, 0x9783, 0xff80},
         {0xab36, 0x3104, 0xdae8, 0x0704, 0x4a28, 0x6714, 0x824b, 0x0051,
          0x8134, 0x1f6a, 0x712d, 0x1f03, 0x03b2, 0xecac, 0x377d, 0xfef9}}
    };

    int i, j, ok;

    /* Test known inputs/outputs */
    for (i = 0; (size_t)i < sizeof(CASES) / sizeof(CASES[0]); ++i) {
        uint16_t out[16];
        test_modinv32_uint16(out, CASES[i][0], CASES[i][1]);
        for (j = 0; j < 16; ++j) CHECK(out[j] == CASES[i][2][j]);
#ifdef SECP256K1_WIDEMUL_INT128
        test_modinv64_uint16(out, CASES[i][0], CASES[i][1]);
        for (j = 0; j < 16; ++j) CHECK(out[j] == CASES[i][2][j]);
#endif
    }

    for (i = 0; i < 100 * count; ++i) {
        /* 256-bit numbers in 16-uint16_t's notation */
        static const uint16_t ZERO[16] = {0};
        uint16_t xd[16];  /* the number (in range [0,2^256)) to be inverted */
        uint16_t md[16];  /* the modulus (odd, in range [3,2^256)) */
        uint16_t id[16];  /* the inverse of xd mod md */

        /* generate random xd and md, so that md is odd, md>1, xd<md, and gcd(xd,md)=1 */
        do {
            /* generate random xd and md (with many subsequent 0s and 1s) */
            secp256k1_testrand256_test((unsigned char*)xd);
            secp256k1_testrand256_test((unsigned char*)md);
            md[0] |= 1; /* modulus must be odd */
            /* If modulus is 1, find another one. */
            ok = md[0] != 1;
            for (j = 1; j < 16; ++j) ok |= md[j] != 0;
            mulmod256(xd, xd, NULL, md); /* Make xd = xd mod md */
        } while (!(ok && coprime(xd, md)));

        test_modinv32_uint16(id, xd, md);
#ifdef SECP256K1_WIDEMUL_INT128
        test_modinv64_uint16(id, xd, md);
#endif

        /* In a few cases, also test with input=0 */
        if (i < count) {
            test_modinv32_uint16(id, ZERO, md);
#ifdef SECP256K1_WIDEMUL_INT128
            test_modinv64_uint16(id, ZERO, md);
#endif
        }
    }
}

/***** SCALAR TESTS *****/


void scalar_test(void) {
    secp256k1_scalar s;
    secp256k1_scalar s1;
    secp256k1_scalar s2;
    unsigned char c[32];

    /* Set 's' to a random scalar, with value 'snum'. */
    random_scalar_order_test(&s);

    /* Set 's1' to a random scalar, with value 's1num'. */
    random_scalar_order_test(&s1);

    /* Set 's2' to a random scalar, with value 'snum2', and byte array representation 'c'. */
    random_scalar_order_test(&s2);
    secp256k1_scalar_get_b32(c, &s2);

    {
        int i;
        /* Test that fetching groups of 4 bits from a scalar and recursing n(i)=16*n(i-1)+p(i) reconstructs it. */
        secp256k1_scalar n;
        secp256k1_scalar_set_int(&n, 0);
        for (i = 0; i < 256; i += 4) {
            secp256k1_scalar t;
            int j;
            secp256k1_scalar_set_int(&t, secp256k1_scalar_get_bits(&s, 256 - 4 - i, 4));
            for (j = 0; j < 4; j++) {
                secp256k1_scalar_add(&n, &n, &n);
            }
            secp256k1_scalar_add(&n, &n, &t);
        }
        CHECK(secp256k1_scalar_eq(&n, &s));
    }

    {
        /* Test that fetching groups of randomly-sized bits from a scalar and recursing n(i)=b*n(i-1)+p(i) reconstructs it. */
        secp256k1_scalar n;
        int i = 0;
        secp256k1_scalar_set_int(&n, 0);
        while (i < 256) {
            secp256k1_scalar t;
            int j;
            int now = secp256k1_testrand_int(15) + 1;
            if (now + i > 256) {
                now = 256 - i;
            }
            secp256k1_scalar_set_int(&t, secp256k1_scalar_get_bits_var(&s, 256 - now - i, now));
            for (j = 0; j < now; j++) {
                secp256k1_scalar_add(&n, &n, &n);
            }
            secp256k1_scalar_add(&n, &n, &t);
            i += now;
        }
        CHECK(secp256k1_scalar_eq(&n, &s));
    }

    {
        /* test secp256k1_scalar_shr_int */
        secp256k1_scalar r;
        int i;
        random_scalar_order_test(&r);
        for (i = 0; i < 100; ++i) {
            int low;
            int shift = 1 + secp256k1_testrand_int(15);
            int expected = r.d[0] % (1 << shift);
            low = secp256k1_scalar_shr_int(&r, shift);
            CHECK(expected == low);
        }
    }

    {
        /* Test commutativity of add. */
        secp256k1_scalar r1, r2;
        secp256k1_scalar_add(&r1, &s1, &s2);
        secp256k1_scalar_add(&r2, &s2, &s1);
        CHECK(secp256k1_scalar_eq(&r1, &r2));
    }

    {
        secp256k1_scalar r1, r2;
        secp256k1_scalar b;
        int i;
        /* Test add_bit. */
        int bit = secp256k1_testrand_bits(8);
        secp256k1_scalar_set_int(&b, 1);
        CHECK(secp256k1_scalar_is_one(&b));
        for (i = 0; i < bit; i++) {
            secp256k1_scalar_add(&b, &b, &b);
        }
        r1 = s1;
        r2 = s1;
        if (!secp256k1_scalar_add(&r1, &r1, &b)) {
            /* No overflow happened. */
            secp256k1_scalar_cadd_bit(&r2, bit, 1);
            CHECK(secp256k1_scalar_eq(&r1, &r2));
            /* cadd is a noop when flag is zero */
            secp256k1_scalar_cadd_bit(&r2, bit, 0);
            CHECK(secp256k1_scalar_eq(&r1, &r2));
        }
    }

    {
        /* Test commutativity of mul. */
        secp256k1_scalar r1, r2;
        secp256k1_scalar_mul(&r1, &s1, &s2);
        secp256k1_scalar_mul(&r2, &s2, &s1);
        CHECK(secp256k1_scalar_eq(&r1, &r2));
    }

    {
        /* Test associativity of add. */
        secp256k1_scalar r1, r2;
        secp256k1_scalar_add(&r1, &s1, &s2);
        secp256k1_scalar_add(&r1, &r1, &s);
        secp256k1_scalar_add(&r2, &s2, &s);
        secp256k1_scalar_add(&r2, &s1, &r2);
        CHECK(secp256k1_scalar_eq(&r1, &r2));
    }

    {
        /* Test associativity of mul. */
        secp256k1_scalar r1, r2;
        secp256k1_scalar_mul(&r1, &s1, &s2);
        secp256k1_scalar_mul(&r1, &r1, &s);
        secp256k1_scalar_mul(&r2, &s2, &s);
        secp256k1_scalar_mul(&r2, &s1, &r2);
        CHECK(secp256k1_scalar_eq(&r1, &r2));
    }

    {
        /* Test distributitivity of mul over add. */
        secp256k1_scalar r1, r2, t;
        secp256k1_scalar_add(&r1, &s1, &s2);
        secp256k1_scalar_mul(&r1, &r1, &s);
        secp256k1_scalar_mul(&r2, &s1, &s);
        secp256k1_scalar_mul(&t, &s2, &s);
        secp256k1_scalar_add(&r2, &r2, &t);
        CHECK(secp256k1_scalar_eq(&r1, &r2));
    }

    {
        /* Test multiplicative identity. */
        secp256k1_scalar r1, v1;
        secp256k1_scalar_set_int(&v1,1);
        secp256k1_scalar_mul(&r1, &s1, &v1);
        CHECK(secp256k1_scalar_eq(&r1, &s1));
    }

    {
        /* Test additive identity. */
        secp256k1_scalar r1, v0;
        secp256k1_scalar_set_int(&v0,0);
        secp256k1_scalar_add(&r1, &s1, &v0);
        CHECK(secp256k1_scalar_eq(&r1, &s1));
    }

    {
        /* Test zero product property. */
        secp256k1_scalar r1, v0;
        secp256k1_scalar_set_int(&v0,0);
        secp256k1_scalar_mul(&r1, &s1, &v0);
        CHECK(secp256k1_scalar_eq(&r1, &v0));
    }

}

void run_scalar_set_b32_seckey_tests(void) {
    unsigned char b32[32];
    secp256k1_scalar s1;
    secp256k1_scalar s2;

    /* Usually set_b32 and set_b32_seckey give the same result */
    random_scalar_order_b32(b32);
    secp256k1_scalar_set_b32(&s1, b32, NULL);
    CHECK(secp256k1_scalar_set_b32_seckey(&s2, b32) == 1);
    CHECK(secp256k1_scalar_eq(&s1, &s2) == 1);

    memset(b32, 0, sizeof(b32));
    CHECK(secp256k1_scalar_set_b32_seckey(&s2, b32) == 0);
    memset(b32, 0xFF, sizeof(b32));
    CHECK(secp256k1_scalar_set_b32_seckey(&s2, b32) == 0);
}

void run_scalar_tests(void) {
    int i;
    for (i = 0; i < 128 * count; i++) {
        scalar_test();
    }
    for (i = 0; i < count; i++) {
        run_scalar_set_b32_seckey_tests();
    }

    {
        /* (-1)+1 should be zero. */
        secp256k1_scalar s, o;
        secp256k1_scalar_set_int(&s, 1);
        CHECK(secp256k1_scalar_is_one(&s));
        secp256k1_scalar_negate(&o, &s);
        secp256k1_scalar_add(&o, &o, &s);
        CHECK(secp256k1_scalar_is_zero(&o));
        secp256k1_scalar_negate(&o, &o);
        CHECK(secp256k1_scalar_is_zero(&o));
    }

    {
        /* Does check_overflow check catch all ones? */
        static const secp256k1_scalar overflowed = SECP256K1_SCALAR_CONST(
            0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL,
            0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL
        );
        CHECK(secp256k1_scalar_check_overflow(&overflowed));
    }

    {
        /* Static test vectors.
         * These were reduced from ~10^12 random vectors based on comparison-decision
         *  and edge-case coverage on 32-bit and 64-bit implementations.
         * The responses were generated with Sage 5.9.
         */
        secp256k1_scalar x;
        secp256k1_scalar y;
        secp256k1_scalar z;
        secp256k1_scalar zz;
        secp256k1_scalar one;
        secp256k1_scalar r1;
        secp256k1_scalar r2;
        secp256k1_scalar zzv;
        int overflow;
        unsigned char chal[33][2][32] = {
            {{0xff, 0xff, 0x03, 0x07, 0x00, 0x00, 0x00, 0x00,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x03,
              0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff,
              0xff, 0xff, 0x03, 0x00, 0xc0, 0xff, 0xff, 0xff},
             {0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff}},
            {{0xef, 0xff, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
              0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
             {0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0,
              0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0x7f, 0x00, 0x80, 0xff}},
            {{0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
              0x80, 0x00, 0x00, 0x80, 0xff, 0x3f, 0x00, 0x00,
              0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0xff, 0x00},
             {0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0xff, 0x80,
              0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x00, 0xe0,
              0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff}},
            {{0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
              0x00, 0x1e, 0xf8, 0xff, 0xff, 0xff, 0xfd, 0xff},
             {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f,
              0x00, 0x00, 0x00, 0xf8, 0xff, 0x03, 0x00, 0xe0,
              0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff,
              0xf3, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}},
            {{0x80, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0x00,
              0x00, 0x1c, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xe0, 0xff, 0xff, 0xff, 0x00,
              0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff},
             {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x03, 0x00,
              0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0x1f, 0x00, 0x00, 0x80, 0xff, 0xff, 0x3f,
              0x00, 0xfe, 0xff, 0xff, 0xff, 0xdf, 0xff, 0xff}},
            {{0xff, 0xff, 0xff, 0xff, 0x00, 0x0f, 0xfc, 0x9f,
              0xff, 0xff, 0xff, 0x00, 0x80, 0x00, 0x00, 0x80,
              0xff, 0x0f, 0xfc, 0xff, 0x7f, 0x00, 0x00, 0x00,
              0x00, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00},
             {0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
              0x00, 0x00, 0xf8, 0xff, 0x0f, 0xc0, 0xff, 0xff,
              0xff, 0x1f, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff,
              0xff, 0xff, 0xff, 0x07, 0x80, 0xff, 0xff, 0xff}},
            {{0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x00, 0x00,
              0x80, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff,
              0xf7, 0xff, 0xff, 0xef, 0xff, 0xff, 0xff, 0x00,
              0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xf0},
             {0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
            {{0x00, 0xf8, 0xff, 0x03, 0xff, 0xff, 0xff, 0x00,
              0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
              0x80, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0x03, 0xc0, 0xff, 0x0f, 0xfc, 0xff},
             {0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0xff, 0xff,
              0xff, 0x01, 0x00, 0x00, 0x00, 0x3f, 0x00, 0xc0,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
            {{0x8f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0x7f, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00},
             {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
            {{0x00, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0x03, 0x00, 0x80, 0x00, 0x00, 0x80,
              0xff, 0xff, 0xff, 0x00, 0x00, 0x80, 0xff, 0x7f},
             {0xff, 0xcf, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00,
              0x00, 0xc0, 0xff, 0xcf, 0xff, 0xff, 0xff, 0xff,
              0xbf, 0xff, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x80, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00}},
            {{0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff,
              0xff, 0xff, 0x00, 0xfc, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0x00, 0x80, 0x00, 0x00, 0x80,
              0xff, 0x01, 0xfc, 0xff, 0x01, 0x00, 0xfe, 0xff},
             {0xff, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x03, 0x00}},
            {{0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
              0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0x00, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0x7f, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80},
             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0xf8, 0xff, 0x01, 0x00, 0xf0, 0xff, 0xff,
              0xe0, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
            {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0x00},
             {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
              0xfc, 0xff, 0xff, 0x3f, 0xf0, 0xff, 0xff, 0x3f,
              0x00, 0x00, 0xf8, 0x07, 0x00, 0x00, 0x00, 0xff,
              0xff, 0xff, 0xff, 0xff, 0x0f, 0x7e, 0x00, 0x00}},
            {{0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0x1f, 0x00, 0x00, 0xfe, 0x07, 0x00},
             {0x00, 0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xfb, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60}},
            {{0xff, 0x01, 0x00, 0xff, 0xff, 0xff, 0x0f, 0x00,
              0x80, 0x7f, 0xfe, 0xff, 0xff, 0xff, 0xff, 0x03,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
             {0xff, 0xff, 0x1f, 0x00, 0xf0, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x00, 0x00}},
            {{0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
             {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf1, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x03,
              0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0xff}},
            {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
              0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0xc0, 0xff, 0xff, 0xcf, 0xff, 0x1f, 0x00, 0x00,
              0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80},
             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x00, 0x7e,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
            {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00},
             {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
              0xff, 0xff, 0x7f, 0x00, 0x80, 0x00, 0x00, 0x00,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
              0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff}},
            {{0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00, 0x80,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
              0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00},
             {0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x80,
              0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
              0xff, 0x7f, 0xf8, 0xff, 0xff, 0x1f, 0x00, 0xfe}},
            {{0xff, 0xff, 0xff, 0x3f, 0xf8, 0xff, 0xff, 0xff,
              0xff, 0x03, 0xfe, 0x01, 0x00, 0x00, 0x00, 0x00,
              0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07},
             {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
              0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
              0xff, 0xff, 0xff, 0xff, 0x01, 0x80, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00}},
            {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
             {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
              0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b,
              0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x40}},
            {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
            {{0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
             {0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
            {{0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xc0,
              0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f},
             {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00,
              0xf0, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00,
              0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0x01, 0xff, 0xff, 0xff}},
            {{0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}},
            {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
              0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b,
              0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x40},
             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}},
            {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0x7e, 0x00, 0x00, 0xc0, 0xff, 0xff, 0x07, 0x00,
              0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
              0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
             {0xff, 0x01, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00, 0x80,
              0xff, 0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0x00,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
            {{0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x00,
              0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
              0x00, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01,
              0x80, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff},
             {0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff,
              0xff, 0xff, 0x3f, 0x00, 0xf8, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0x3f, 0x00, 0x00, 0xc0, 0xf1, 0x7f, 0x00}},
            {{0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x80, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0x00},
             {0x00, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff,
              0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x80, 0x1f,
              0x00, 0x00, 0xfc, 0xff, 0xff, 0x01, 0xff, 0xff}},
            {{0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
              0x80, 0x00, 0x00, 0x80, 0xff, 0x03, 0xe0, 0x01,
              0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xfc, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00},
             {0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
              0xfe, 0xff, 0xff, 0xf0, 0x07, 0x00, 0x3c, 0x80,
              0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xff,
              0xff, 0xff, 0x07, 0xe0, 0xff, 0x00, 0x00, 0x00}},
            {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
              0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0xf8,
              0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80},
             {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0x0c, 0x80, 0x00,
              0x00, 0x00, 0x00, 0xc0, 0x7f, 0xfe, 0xff, 0x1f,
              0x00, 0xfe, 0xff, 0x03, 0x00, 0x00, 0xfe, 0xff}},
            {{0xff, 0xff, 0x81, 0xff, 0xff, 0xff, 0xff, 0x00,
              0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x83,
              0xff, 0xff, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
              0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xf0},
             {0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00, 0x00,
              0xf8, 0x07, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xc7, 0xff, 0xff, 0xe0, 0xff, 0xff, 0xff}},
            {{0x82, 0xc9, 0xfa, 0xb0, 0x68, 0x04, 0xa0, 0x00,
              0x82, 0xc9, 0xfa, 0xb0, 0x68, 0x04, 0xa0, 0x00,
              0xff, 0xff, 0xff, 0xff, 0xff, 0x6f, 0x03, 0xfb,
              0xfa, 0x8a, 0x7d, 0xdf, 0x13, 0x86, 0xe2, 0x03},
             {0x82, 0xc9, 0xfa, 0xb0, 0x68, 0x04, 0xa0, 0x00,
              0x82, 0xc9, 0xfa, 0xb0, 0x68, 0x04, 0xa0, 0x00,
              0xff, 0xff, 0xff, 0xff, 0xff, 0x6f, 0x03, 0xfb,
              0xfa, 0x8a, 0x7d, 0xdf, 0x13, 0x86, 0xe2, 0x03}}
        };
        unsigned char res[33][2][32] = {
            {{0x0c, 0x3b, 0x0a, 0xca, 0x8d, 0x1a, 0x2f, 0xb9,
              0x8a, 0x7b, 0x53, 0x5a, 0x1f, 0xc5, 0x22, 0xa1,
              0x07, 0x2a, 0x48, 0xea, 0x02, 0xeb, 0xb3, 0xd6,
              0x20, 0x1e, 0x86, 0xd0, 0x95, 0xf6, 0x92, 0x35},
             {0xdc, 0x90, 0x7a, 0x07, 0x2e, 0x1e, 0x44, 0x6d,
              0xf8, 0x15, 0x24, 0x5b, 0x5a, 0x96, 0x37, 0x9c,
              0x37, 0x7b, 0x0d, 0xac, 0x1b, 0x65, 0x58, 0x49,
              0x43, 0xb7, 0x31, 0xbb, 0xa7, 0xf4, 0x97, 0x15}},
            {{0xf1, 0xf7, 0x3a, 0x50, 0xe6, 0x10, 0xba, 0x22,
              0x43, 0x4d, 0x1f, 0x1f, 0x7c, 0x27, 0xca, 0x9c,
              0xb8, 0xb6, 0xa0, 0xfc, 0xd8, 0xc0, 0x05, 0x2f,
              0xf7, 0x08, 0xe1, 0x76, 0xdd, 0xd0, 0x80, 0xc8},
             {0xe3, 0x80, 0x80, 0xb8, 0xdb, 0xe3, 0xa9, 0x77,
              0x00, 0xb0, 0xf5, 0x2e, 0x27, 0xe2, 0x68, 0xc4,
              0x88, 0xe8, 0x04, 0xc1, 0x12, 0xbf, 0x78, 0x59,
              0xe6, 0xa9, 0x7c, 0xe1, 0x81, 0xdd, 0xb9, 0xd5}},
            {{0x96, 0xe2, 0xee, 0x01, 0xa6, 0x80, 0x31, 0xef,
              0x5c, 0xd0, 0x19, 0xb4, 0x7d, 0x5f, 0x79, 0xab,
              0xa1, 0x97, 0xd3, 0x7e, 0x33, 0xbb, 0x86, 0x55,
              0x60, 0x20, 0x10, 0x0d, 0x94, 0x2d, 0x11, 0x7c},
             {0xcc, 0xab, 0xe0, 0xe8, 0x98, 0x65, 0x12, 0x96,
              0x38, 0x5a, 0x1a, 0xf2, 0x85, 0x23, 0x59, 0x5f,
              0xf9, 0xf3, 0xc2, 0x81, 0x70, 0x92, 0x65, 0x12,
              0x9c, 0x65, 0x1e, 0x96, 0x00, 0xef, 0xe7, 0x63}},
            {{0xac, 0x1e, 0x62, 0xc2, 0x59, 0xfc, 0x4e, 0x5c,
              0x83, 0xb0, 0xd0, 0x6f, 0xce, 0x19, 0xf6, 0xbf,
              0xa4, 0xb0, 0xe0, 0x53, 0x66, 0x1f, 0xbf, 0xc9,
              0x33, 0x47, 0x37, 0xa9, 0x3d, 0x5d, 0xb0, 0x48},
             {0x86, 0xb9, 0x2a, 0x7f, 0x8e, 0xa8, 0x60, 0x42,
              0x26, 0x6d, 0x6e, 0x1c, 0xa2, 0xec, 0xe0, 0xe5,
              0x3e, 0x0a, 0x33, 0xbb, 0x61, 0x4c, 0x9f, 0x3c,
              0xd1, 0xdf, 0x49, 0x33, 0xcd, 0x72, 0x78, 0x18}},
            {{0xf7, 0xd3, 0xcd, 0x49, 0x5c, 0x13, 0x22, 0xfb,
              0x2e, 0xb2, 0x2f, 0x27, 0xf5, 0x8a, 0x5d, 0x74,
              0xc1, 0x58, 0xc5, 0xc2, 0x2d, 0x9f, 0x52, 0xc6,
              0x63, 0x9f, 0xba, 0x05, 0x76, 0x45, 0x7a, 0x63},
             {0x8a, 0xfa, 0x55, 0x4d, 0xdd, 0xa3, 0xb2, 0xc3,
              0x44, 0xfd, 0xec, 0x72, 0xde, 0xef, 0xc0, 0x99,
              0xf5, 0x9f, 0xe2, 0x52, 0xb4, 0x05, 0x32, 0x58,
              0x57, 0xc1, 0x8f, 0xea, 0xc3, 0x24, 0x5b, 0x94}},
            {{0x05, 0x83, 0xee, 0xdd, 0x64, 0xf0, 0x14, 0x3b,
              0xa0, 0x14, 0x4a, 0x3a, 0x41, 0x82, 0x7c, 0xa7,
              0x2c, 0xaa, 0xb1, 0x76, 0xbb, 0x59, 0x64, 0x5f,
              0x52, 0xad, 0x25, 0x29, 0x9d, 0x8f, 0x0b, 0xb0},
             {0x7e, 0xe3, 0x7c, 0xca, 0xcd, 0x4f, 0xb0, 0x6d,
              0x7a, 0xb2, 0x3e, 0xa0, 0x08, 0xb9, 0xa8, 0x2d,
              0xc2, 0xf4, 0x99, 0x66, 0xcc, 0xac, 0xd8, 0xb9,
              0x72, 0x2a, 0x4a, 0x3e, 0x0f, 0x7b, 0xbf, 0xf4}},
            {{0x8c, 0x9c, 0x78, 0x2b, 0x39, 0x61, 0x7e, 0xf7,
              0x65, 0x37, 0x66, 0x09, 0x38, 0xb9, 0x6f, 0x70,
              0x78, 0x87, 0xff, 0xcf, 0x93, 0xca, 0x85, 0x06,
              0x44, 0x84, 0xa7, 0xfe, 0xd3, 0xa4, 0xe3, 0x7e},
             {0xa2, 0x56, 0x49, 0x23, 0x54, 0xa5, 0x50, 0xe9,
              0x5f, 0xf0, 0x4d, 0xe7, 0xdc, 0x38, 0x32, 0x79,
              0x4f, 0x1c, 0xb7, 0xe4, 0xbb, 0xf8, 0xbb, 0x2e,
              0x40, 0x41, 0x4b, 0xcc, 0xe3, 0x1e, 0x16, 0x36}},
            {{0x0c, 0x1e, 0xd7, 0x09, 0x25, 0x40, 0x97, 0xcb,
              0x5c, 0x46, 0xa8, 0xda, 0xef, 0x25, 0xd5, 0xe5,
              0x92, 0x4d, 0xcf, 0xa3, 0xc4, 0x5d, 0x35, 0x4a,
              0xe4, 0x61, 0x92, 0xf3, 0xbf, 0x0e, 0xcd, 0xbe},
             {0xe4, 0xaf, 0x0a, 0xb3, 0x30, 0x8b, 0x9b, 0x48,
              0x49, 0x43, 0xc7, 0x64, 0x60, 0x4a, 0x2b, 0x9e,
              0x95, 0x5f, 0x56, 0xe8, 0x35, 0xdc, 0xeb, 0xdc,
              0xc7, 0xc4, 0xfe, 0x30, 0x40, 0xc7, 0xbf, 0xa4}},
            {{0xd4, 0xa0, 0xf5, 0x81, 0x49, 0x6b, 0xb6, 0x8b,
              0x0a, 0x69, 0xf9, 0xfe, 0xa8, 0x32, 0xe5, 0xe0,
              0xa5, 0xcd, 0x02, 0x53, 0xf9, 0x2c, 0xe3, 0x53,
              0x83, 0x36, 0xc6, 0x02, 0xb5, 0xeb, 0x64, 0xb8},
             {0x1d, 0x42, 0xb9, 0xf9, 0xe9, 0xe3, 0x93, 0x2c,
              0x4c, 0xee, 0x6c, 0x5a, 0x47, 0x9e, 0x62, 0x01,
              0x6b, 0x04, 0xfe, 0xa4, 0x30, 0x2b, 0x0d, 0x4f,
              0x71, 0x10, 0xd3, 0x55, 0xca, 0xf3, 0x5e, 0x80}},
            {{0x77, 0x05, 0xf6, 0x0c, 0x15, 0x9b, 0x45, 0xe7,
              0xb9, 0x11, 0xb8, 0xf5, 0xd6, 0xda, 0x73, 0x0c,
              0xda, 0x92, 0xea, 0xd0, 0x9d, 0xd0, 0x18, 0x92,
              0xce, 0x9a, 0xaa, 0xee, 0x0f, 0xef, 0xde, 0x30},
             {0xf1, 0xf1, 0xd6, 0x9b, 0x51, 0xd7, 0x77, 0x62,
              0x52, 0x10, 0xb8, 0x7a, 0x84, 0x9d, 0x15, 0x4e,
              0x07, 0xdc, 0x1e, 0x75, 0x0d, 0x0c, 0x3b, 0xdb,
              0x74, 0x58, 0x62, 0x02, 0x90, 0x54, 0x8b, 0x43}},
            {{0xa6, 0xfe, 0x0b, 0x87, 0x80, 0x43, 0x67, 0x25,
              0x57, 0x5d, 0xec, 0x40, 0x50, 0x08, 0xd5, 0x5d,
              0x43, 0xd7, 0xe0, 0xaa, 0xe0, 0x13, 0xb6, 0xb0,
              0xc0, 0xd4, 0xe5, 0x0d, 0x45, 0x83, 0xd6, 0x13},
             {0x40, 0x45, 0x0a, 0x92, 0x31, 0xea, 0x8c, 0x60,
              0x8c, 0x1f, 0xd8, 0x76, 0x45, 0xb9, 0x29, 0x00,
              0x26, 0x32, 0xd8, 0xa6, 0x96, 0x88, 0xe2, 0xc4,
              0x8b, 0xdb, 0x7f, 0x17, 0x87, 0xcc, 0xc8, 0xf2}},
            {{0xc2, 0x56, 0xe2, 0xb6, 0x1a, 0x81, 0xe7, 0x31,
              0x63, 0x2e, 0xbb, 0x0d, 0x2f, 0x81, 0x67, 0xd4,
              0x22, 0xe2, 0x38, 0x02, 0x25, 0x97, 0xc7, 0x88,
              0x6e, 0xdf, 0xbe, 0x2a, 0xa5, 0x73, 0x63, 0xaa},
             {0x50, 0x45, 0xe2, 0xc3, 0xbd, 0x89, 0xfc, 0x57,
              0xbd, 0x3c, 0xa3, 0x98, 0x7e, 0x7f, 0x36, 0x38,
              0x92, 0x39, 0x1f, 0x0f, 0x81, 0x1a, 0x06, 0x51,
              0x1f, 0x8d, 0x6a, 0xff, 0x47, 0x16, 0x06, 0x9c}},
            {{0x33, 0x95, 0xa2, 0x6f, 0x27, 0x5f, 0x9c, 0x9c,
              0x64, 0x45, 0xcb, 0xd1, 0x3c, 0xee, 0x5e, 0x5f,
              0x48, 0xa6, 0xaf, 0xe3, 0x79, 0xcf, 0xb1, 0xe2,
              0xbf, 0x55, 0x0e, 0xa2, 0x3b, 0x62, 0xf0, 0xe4},
             {0x14, 0xe8, 0x06, 0xe3, 0xbe, 0x7e, 0x67, 0x01,
              0xc5, 0x21, 0x67, 0xd8, 0x54, 0xb5, 0x7f, 0xa4,
              0xf9, 0x75, 0x70, 0x1c, 0xfd, 0x79, 0xdb, 0x86,
              0xad, 0x37, 0x85, 0x83, 0x56, 0x4e, 0xf0, 0xbf}},
            {{0xbc, 0xa6, 0xe0, 0x56, 0x4e, 0xef, 0xfa, 0xf5,
              0x1d, 0x5d, 0x3f, 0x2a, 0x5b, 0x19, 0xab, 0x51,
              0xc5, 0x8b, 0xdd, 0x98, 0x28, 0x35, 0x2f, 0xc3,
              0x81, 0x4f, 0x5c, 0xe5, 0x70, 0xb9, 0xeb, 0x62},
             {0xc4, 0x6d, 0x26, 0xb0, 0x17, 0x6b, 0xfe, 0x6c,
              0x12, 0xf8, 0xe7, 0xc1, 0xf5, 0x2f, 0xfa, 0x91,
              0x13, 0x27, 0xbd, 0x73, 0xcc, 0x33, 0x31, 0x1c,
              0x39, 0xe3, 0x27, 0x6a, 0x95, 0xcf, 0xc5, 0xfb}},
            {{0x30, 0xb2, 0x99, 0x84, 0xf0, 0x18, 0x2a, 0x6e,
              0x1e, 0x27, 0xed, 0xa2, 0x29, 0x99, 0x41, 0x56,
              0xe8, 0xd4, 0x0d, 0xef, 0x99, 0x9c, 0xf3, 0x58,
              0x29, 0x55, 0x1a, 0xc0, 0x68, 0xd6, 0x74, 0xa4},
             {0x07, 0x9c, 0xe7, 0xec, 0xf5, 0x36, 0x73, 0x41,
              0xa3, 0x1c, 0xe5, 0x93, 0x97, 0x6a, 0xfd, 0xf7,
              0x53, 0x18, 0xab, 0xaf, 0xeb, 0x85, 0xbd, 0x92,
              0x90, 0xab, 0x3c, 0xbf, 0x30, 0x82, 0xad, 0xf6}},
            {{0xc6, 0x87, 0x8a, 0x2a, 0xea, 0xc0, 0xa9, 0xec,
              0x6d, 0xd3, 0xdc, 0x32, 0x23, 0xce, 0x62, 0x19,
              0xa4, 0x7e, 0xa8, 0xdd, 0x1c, 0x33, 0xae, 0xd3,
              0x4f, 0x62, 0x9f, 0x52, 0xe7, 0x65, 0x46, 0xf4},
             {0x97, 0x51, 0x27, 0x67, 0x2d, 0xa2, 0x82, 0x87,
              0x98, 0xd3, 0xb6, 0x14, 0x7f, 0x51, 0xd3, 0x9a,
              0x0b, 0xd0, 0x76, 0x81, 0xb2, 0x4f, 0x58, 0x92,
              0xa4, 0x86, 0xa1, 0xa7, 0x09, 0x1d, 0xef, 0x9b}},
            {{0xb3, 0x0f, 0x2b, 0x69, 0x0d, 0x06, 0x90, 0x64,
              0xbd, 0x43, 0x4c, 0x10, 0xe8, 0x98, 0x1c, 0xa3,
              0xe1, 0x68, 0xe9, 0x79, 0x6c, 0x29, 0x51, 0x3f,
              0x41, 0xdc, 0xdf, 0x1f, 0xf3, 0x60, 0xbe, 0x33},
             {0xa1, 0x5f, 0xf7, 0x1d, 0xb4, 0x3e, 0x9b, 0x3c,
              0xe7, 0xbd, 0xb6, 0x06, 0xd5, 0x60, 0x06, 0x6d,
              0x50, 0xd2, 0xf4, 0x1a, 0x31, 0x08, 0xf2, 0xea,
              0x8e, 0xef, 0x5f, 0x7d, 0xb6, 0xd0, 0xc0, 0x27}},
            {{0x62, 0x9a, 0xd9, 0xbb, 0x38, 0x36, 0xce, 0xf7,
              0x5d, 0x2f, 0x13, 0xec, 0xc8, 0x2d, 0x02, 0x8a,
              0x2e, 0x72, 0xf0, 0xe5, 0x15, 0x9d, 0x72, 0xae,
              0xfc, 0xb3, 0x4f, 0x02, 0xea, 0xe1, 0x09, 0xfe},
             {0x00, 0x00, 0x00, 0x00, 0xfa, 0x0a, 0x3d, 0xbc,
              0xad, 0x16, 0x0c, 0xb6, 0xe7, 0x7c, 0x8b, 0x39,
              0x9a, 0x43, 0xbb, 0xe3, 0xc2, 0x55, 0x15, 0x14,
              0x75, 0xac, 0x90, 0x9b, 0x7f, 0x9a, 0x92, 0x00}},
            {{0x8b, 0xac, 0x70, 0x86, 0x29, 0x8f, 0x00, 0x23,
              0x7b, 0x45, 0x30, 0xaa, 0xb8, 0x4c, 0xc7, 0x8d,
              0x4e, 0x47, 0x85, 0xc6, 0x19, 0xe3, 0x96, 0xc2,
              0x9a, 0xa0, 0x12, 0xed, 0x6f, 0xd7, 0x76, 0x16},
             {0x45, 0xaf, 0x7e, 0x33, 0xc7, 0x7f, 0x10, 0x6c,
              0x7c, 0x9f, 0x29, 0xc1, 0xa8, 0x7e, 0x15, 0x84,
              0xe7, 0x7d, 0xc0, 0x6d, 0xab, 0x71, 0x5d, 0xd0,
              0x6b, 0x9f, 0x97, 0xab, 0xcb, 0x51, 0x0c, 0x9f}},
            {{0x9e, 0xc3, 0x92, 0xb4, 0x04, 0x9f, 0xc8, 0xbb,
              0xdd, 0x9e, 0xc6, 0x05, 0xfd, 0x65, 0xec, 0x94,
              0x7f, 0x2c, 0x16, 0xc4, 0x40, 0xac, 0x63, 0x7b,
              0x7d, 0xb8, 0x0c, 0xe4, 0x5b, 0xe3, 0xa7, 0x0e},
             {0x43, 0xf4, 0x44, 0xe8, 0xcc, 0xc8, 0xd4, 0x54,
              0x33, 0x37, 0x50, 0xf2, 0x87, 0x42, 0x2e, 0x00,
              0x49, 0x60, 0x62, 0x02, 0xfd, 0x1a, 0x7c, 0xdb,
              0x29, 0x6c, 0x6d, 0x54, 0x53, 0x08, 0xd1, 0xc8}},
            {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
            {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}},
            {{0x27, 0x59, 0xc7, 0x35, 0x60, 0x71, 0xa6, 0xf1,
              0x79, 0xa5, 0xfd, 0x79, 0x16, 0xf3, 0x41, 0xf0,
              0x57, 0xb4, 0x02, 0x97, 0x32, 0xe7, 0xde, 0x59,
              0xe2, 0x2d, 0x9b, 0x11, 0xea, 0x2c, 0x35, 0x92},
             {0x27, 0x59, 0xc7, 0x35, 0x60, 0x71, 0xa6, 0xf1,
              0x79, 0xa5, 0xfd, 0x79, 0x16, 0xf3, 0x41, 0xf0,
              0x57, 0xb4, 0x02, 0x97, 0x32, 0xe7, 0xde, 0x59,
              0xe2, 0x2d, 0x9b, 0x11, 0xea, 0x2c, 0x35, 0x92}},
            {{0x28, 0x56, 0xac, 0x0e, 0x4f, 0x98, 0x09, 0xf0,
              0x49, 0xfa, 0x7f, 0x84, 0xac, 0x7e, 0x50, 0x5b,
              0x17, 0x43, 0x14, 0x89, 0x9c, 0x53, 0xa8, 0x94,
              0x30, 0xf2, 0x11, 0x4d, 0x92, 0x14, 0x27, 0xe8},
             {0x39, 0x7a, 0x84, 0x56, 0x79, 0x9d, 0xec, 0x26,
              0x2c, 0x53, 0xc1, 0x94, 0xc9, 0x8d, 0x9e, 0x9d,
              0x32, 0x1f, 0xdd, 0x84, 0x04, 0xe8, 0xe2, 0x0a,
              0x6b, 0xbe, 0xbb, 0x42, 0x40, 0x67, 0x30, 0x6c}},
            {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
              0x45, 0x51, 0x23, 0x19, 0x50, 0xb7, 0x5f, 0xc4,
              0x40, 0x2d, 0xa1, 0x73, 0x2f, 0xc9, 0xbe, 0xbd},
             {0x27, 0x59, 0xc7, 0x35, 0x60, 0x71, 0xa6, 0xf1,
              0x79, 0xa5, 0xfd, 0x79, 0x16, 0xf3, 0x41, 0xf0,
              0x57, 0xb4, 0x02, 0x97, 0x32, 0xe7, 0xde, 0x59,
              0xe2, 0x2d, 0x9b, 0x11, 0xea, 0x2c, 0x35, 0x92}},
            {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
              0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b,
              0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x40},
             {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}},
            {{0x1c, 0xc4, 0xf7, 0xda, 0x0f, 0x65, 0xca, 0x39,
              0x70, 0x52, 0x92, 0x8e, 0xc3, 0xc8, 0x15, 0xea,
              0x7f, 0x10, 0x9e, 0x77, 0x4b, 0x6e, 0x2d, 0xdf,
              0xe8, 0x30, 0x9d, 0xda, 0xe8, 0x9a, 0x65, 0xae},
             {0x02, 0xb0, 0x16, 0xb1, 0x1d, 0xc8, 0x57, 0x7b,
              0xa2, 0x3a, 0xa2, 0xa3, 0x38, 0x5c, 0x8f, 0xeb,
              0x66, 0x37, 0x91, 0xa8, 0x5f, 0xef, 0x04, 0xf6,
              0x59, 0x75, 0xe1, 0xee, 0x92, 0xf6, 0x0e, 0x30}},
            {{0x8d, 0x76, 0x14, 0xa4, 0x14, 0x06, 0x9f, 0x9a,
              0xdf, 0x4a, 0x85, 0xa7, 0x6b, 0xbf, 0x29, 0x6f,
              0xbc, 0x34, 0x87, 0x5d, 0xeb, 0xbb, 0x2e, 0xa9,
              0xc9, 0x1f, 0x58, 0xd6, 0x9a, 0x82, 0xa0, 0x56},
             {0xd4, 0xb9, 0xdb, 0x88, 0x1d, 0x04, 0xe9, 0x93,
              0x8d, 0x3f, 0x20, 0xd5, 0x86, 0xa8, 0x83, 0x07,
              0xdb, 0x09, 0xd8, 0x22, 0x1f, 0x7f, 0xf1, 0x71,
              0xc8, 0xe7, 0x5d, 0x47, 0xaf, 0x8b, 0x72, 0xe9}},
            {{0x83, 0xb9, 0x39, 0xb2, 0xa4, 0xdf, 0x46, 0x87,
              0xc2, 0xb8, 0xf1, 0xe6, 0x4c, 0xd1, 0xe2, 0xa9,
              0xe4, 0x70, 0x30, 0x34, 0xbc, 0x52, 0x7c, 0x55,
              0xa6, 0xec, 0x80, 0xa4, 0xe5, 0xd2, 0xdc, 0x73},
             {0x08, 0xf1, 0x03, 0xcf, 0x16, 0x73, 0xe8, 0x7d,
              0xb6, 0x7e, 0x9b, 0xc0, 0xb4, 0xc2, 0xa5, 0x86,
              0x02, 0x77, 0xd5, 0x27, 0x86, 0xa5, 0x15, 0xfb,
              0xae, 0x9b, 0x8c, 0xa9, 0xf9, 0xf8, 0xa8, 0x4a}},
            {{0x8b, 0x00, 0x49, 0xdb, 0xfa, 0xf0, 0x1b, 0xa2,
              0xed, 0x8a, 0x9a, 0x7a, 0x36, 0x78, 0x4a, 0xc7,
              0xf7, 0xad, 0x39, 0xd0, 0x6c, 0x65, 0x7a, 0x41,
              0xce, 0xd6, 0xd6, 0x4c, 0x20, 0x21, 0x6b, 0xc7},
             {0xc6, 0xca, 0x78, 0x1d, 0x32, 0x6c, 0x6c, 0x06,
              0x91, 0xf2, 0x1a, 0xe8, 0x43, 0x16, 0xea, 0x04,
              0x3c, 0x1f, 0x07, 0x85, 0xf7, 0x09, 0x22, 0x08,
              0xba, 0x13, 0xfd, 0x78, 0x1e, 0x3f, 0x6f, 0x62}},
            {{0x25, 0x9b, 0x7c, 0xb0, 0xac, 0x72, 0x6f, 0xb2,
              0xe3, 0x53, 0x84, 0x7a, 0x1a, 0x9a, 0x98, 0x9b,
              0x44, 0xd3, 0x59, 0xd0, 0x8e, 0x57, 0x41, 0x40,
              0x78, 0xa7, 0x30, 0x2f, 0x4c, 0x9c, 0xb9, 0x68},
             {0xb7, 0x75, 0x03, 0x63, 0x61, 0xc2, 0x48, 0x6e,
              0x12, 0x3d, 0xbf, 0x4b, 0x27, 0xdf, 0xb1, 0x7a,
              0xff, 0x4e, 0x31, 0x07, 0x83, 0xf4, 0x62, 0x5b,
              0x19, 0xa5, 0xac, 0xa0, 0x32, 0x58, 0x0d, 0xa7}},
            {{0x43, 0x4f, 0x10, 0xa4, 0xca, 0xdb, 0x38, 0x67,
              0xfa, 0xae, 0x96, 0xb5, 0x6d, 0x97, 0xff, 0x1f,
              0xb6, 0x83, 0x43, 0xd3, 0xa0, 0x2d, 0x70, 0x7a,
              0x64, 0x05, 0x4c, 0xa7, 0xc1, 0xa5, 0x21, 0x51},
             {0xe4, 0xf1, 0x23, 0x84, 0xe1, 0xb5, 0x9d, 0xf2,
              0xb8, 0x73, 0x8b, 0x45, 0x2b, 0x35, 0x46, 0x38,
              0x10, 0x2b, 0x50, 0xf8, 0x8b, 0x35, 0xcd, 0x34,
              0xc8, 0x0e, 0xf6, 0xdb, 0x09, 0x35, 0xf0, 0xda}},
            {{0xdb, 0x21, 0x5c, 0x8d, 0x83, 0x1d, 0xb3, 0x34,
              0xc7, 0x0e, 0x43, 0xa1, 0x58, 0x79, 0x67, 0x13,
              0x1e, 0x86, 0x5d, 0x89, 0x63, 0xe6, 0x0a, 0x46,
              0x5c, 0x02, 0x97, 0x1b, 0x62, 0x43, 0x86, 0xf5},
             {0xdb, 0x21, 0x5c, 0x8d, 0x83, 0x1d, 0xb3, 0x34,
              0xc7, 0x0e, 0x43, 0xa1, 0x58, 0x79, 0x67, 0x13,
              0x1e, 0x86, 0x5d, 0x89, 0x63, 0xe6, 0x0a, 0x46,
              0x5c, 0x02, 0x97, 0x1b, 0x62, 0x43, 0x86, 0xf5}}
        };
        secp256k1_scalar_set_int(&one, 1);
        for (i = 0; i < 33; i++) {
            secp256k1_scalar_set_b32(&x, chal[i][0], &overflow);
            CHECK(!overflow);
            secp256k1_scalar_set_b32(&y, chal[i][1], &overflow);
            CHECK(!overflow);
            secp256k1_scalar_set_b32(&r1, res[i][0], &overflow);
            CHECK(!overflow);
            secp256k1_scalar_set_b32(&r2, res[i][1], &overflow);
            CHECK(!overflow);
            secp256k1_scalar_mul(&z, &x, &y);
            CHECK(!secp256k1_scalar_check_overflow(&z));
            CHECK(secp256k1_scalar_eq(&r1, &z));
            if (!secp256k1_scalar_is_zero(&y)) {
                secp256k1_scalar_inverse(&zz, &y);
                CHECK(!secp256k1_scalar_check_overflow(&zz));
                secp256k1_scalar_inverse_var(&zzv, &y);
                CHECK(secp256k1_scalar_eq(&zzv, &zz));
                secp256k1_scalar_mul(&z, &z, &zz);
                CHECK(!secp256k1_scalar_check_overflow(&z));
                CHECK(secp256k1_scalar_eq(&x, &z));
                secp256k1_scalar_mul(&zz, &zz, &y);
                CHECK(!secp256k1_scalar_check_overflow(&zz));
                CHECK(secp256k1_scalar_eq(&one, &zz));
            }
        }
    }
}

/***** FIELD TESTS *****/

void random_fe(secp256k1_fe *x) {
    unsigned char bin[32];
    do {
        secp256k1_testrand256(bin);
        if (secp256k1_fe_set_b32(x, bin)) {
            return;
        }
    } while(1);
}

void random_fe_test(secp256k1_fe *x) {
    unsigned char bin[32];
    do {
        secp256k1_testrand256_test(bin);
        if (secp256k1_fe_set_b32(x, bin)) {
            return;
        }
    } while(1);
}

void random_fe_non_zero(secp256k1_fe *nz) {
    int tries = 10;
    while (--tries >= 0) {
        random_fe(nz);
        secp256k1_fe_normalize(nz);
        if (!secp256k1_fe_is_zero(nz)) {
            break;
        }
    }
    /* Infinitesimal probability of spurious failure here */
    CHECK(tries >= 0);
}

void random_fe_non_square(secp256k1_fe *ns) {
    secp256k1_fe r;
    random_fe_non_zero(ns);
    if (secp256k1_fe_sqrt(&r, ns)) {
        secp256k1_fe_negate(ns, ns, 1);
    }
}

int check_fe_equal(const secp256k1_fe *a, const secp256k1_fe *b) {
    secp256k1_fe an = *a;
    secp256k1_fe bn = *b;
    secp256k1_fe_normalize_weak(&an);
    secp256k1_fe_normalize_var(&bn);
    return secp256k1_fe_equal_var(&an, &bn);
}

void run_field_convert(void) {
    static const unsigned char b32[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
        0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40
    };
    static const secp256k1_fe_storage fes = SECP256K1_FE_STORAGE_CONST(
        0x00010203UL, 0x04050607UL, 0x11121314UL, 0x15161718UL,
        0x22232425UL, 0x26272829UL, 0x33343536UL, 0x37383940UL
    );
    static const secp256k1_fe fe = SECP256K1_FE_CONST(
        0x00010203UL, 0x04050607UL, 0x11121314UL, 0x15161718UL,
        0x22232425UL, 0x26272829UL, 0x33343536UL, 0x37383940UL
    );
    secp256k1_fe fe2;
    unsigned char b322[32];
    secp256k1_fe_storage fes2;
    /* Check conversions to fe. */
    CHECK(secp256k1_fe_set_b32(&fe2, b32));
    CHECK(secp256k1_fe_equal_var(&fe, &fe2));
    secp256k1_fe_from_storage(&fe2, &fes);
    CHECK(secp256k1_fe_equal_var(&fe, &fe2));
    /* Check conversion from fe. */
    secp256k1_fe_get_b32(b322, &fe);
    CHECK(secp256k1_memcmp_var(b322, b32, 32) == 0);
    secp256k1_fe_to_storage(&fes2, &fe);
    CHECK(secp256k1_memcmp_var(&fes2, &fes, sizeof(fes)) == 0);
}

int fe_secp256k1_memcmp_var(const secp256k1_fe *a, const secp256k1_fe *b) {
    secp256k1_fe t = *b;
#ifdef VERIFY
    t.magnitude = a->magnitude;
    t.normalized = a->normalized;
#endif
    return secp256k1_memcmp_var(a, &t, sizeof(secp256k1_fe));
}

void run_field_misc(void) {
    secp256k1_fe x;
    secp256k1_fe y;
    secp256k1_fe z;
    secp256k1_fe q;
    secp256k1_fe fe5 = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 5);
    int i, j;
    for (i = 0; i < 5*count; i++) {
        secp256k1_fe_storage xs, ys, zs;
        random_fe(&x);
        random_fe_non_zero(&y);
        /* Test the fe equality and comparison operations. */
        CHECK(secp256k1_fe_cmp_var(&x, &x) == 0);
        CHECK(secp256k1_fe_equal_var(&x, &x));
        z = x;
        secp256k1_fe_add(&z,&y);
        /* Test fe conditional move; z is not normalized here. */
        q = x;
        secp256k1_fe_cmov(&x, &z, 0);
#ifdef VERIFY
        CHECK(x.normalized && x.magnitude == 1);
#endif
        secp256k1_fe_cmov(&x, &x, 1);
        CHECK(fe_secp256k1_memcmp_var(&x, &z) != 0);
        CHECK(fe_secp256k1_memcmp_var(&x, &q) == 0);
        secp256k1_fe_cmov(&q, &z, 1);
#ifdef VERIFY
        CHECK(!q.normalized && q.magnitude == z.magnitude);
#endif
        CHECK(fe_secp256k1_memcmp_var(&q, &z) == 0);
        secp256k1_fe_normalize_var(&x);
        secp256k1_fe_normalize_var(&z);
        CHECK(!secp256k1_fe_equal_var(&x, &z));
        secp256k1_fe_normalize_var(&q);
        secp256k1_fe_cmov(&q, &z, (i&1));
#ifdef VERIFY
        CHECK(q.normalized && q.magnitude == 1);
#endif
        for (j = 0; j < 6; j++) {
            secp256k1_fe_negate(&z, &z, j+1);
            secp256k1_fe_normalize_var(&q);
            secp256k1_fe_cmov(&q, &z, (j&1));
#ifdef VERIFY
            CHECK((q.normalized != (j&1)) && q.magnitude == ((j&1) ? z.magnitude : 1));
#endif
        }
        secp256k1_fe_normalize_var(&z);
        /* Test storage conversion and conditional moves. */
        secp256k1_fe_to_storage(&xs, &x);
        secp256k1_fe_to_storage(&ys, &y);
        secp256k1_fe_to_storage(&zs, &z);
        secp256k1_fe_storage_cmov(&zs, &xs, 0);
        secp256k1_fe_storage_cmov(&zs, &zs, 1);
        CHECK(secp256k1_memcmp_var(&xs, &zs, sizeof(xs)) != 0);
        secp256k1_fe_storage_cmov(&ys, &xs, 1);
        CHECK(secp256k1_memcmp_var(&xs, &ys, sizeof(xs)) == 0);
        secp256k1_fe_from_storage(&x, &xs);
        secp256k1_fe_from_storage(&y, &ys);
        secp256k1_fe_from_storage(&z, &zs);
        /* Test that mul_int, mul, and add agree. */
        secp256k1_fe_add(&y, &x);
        secp256k1_fe_add(&y, &x);
        z = x;
        secp256k1_fe_mul_int(&z, 3);
        CHECK(check_fe_equal(&y, &z));
        secp256k1_fe_add(&y, &x);
        secp256k1_fe_add(&z, &x);
        CHECK(check_fe_equal(&z, &y));
        z = x;
        secp256k1_fe_mul_int(&z, 5);
        secp256k1_fe_mul(&q, &x, &fe5);
        CHECK(check_fe_equal(&z, &q));
        secp256k1_fe_negate(&x, &x, 1);
        secp256k1_fe_add(&z, &x);
        secp256k1_fe_add(&q, &x);
        CHECK(check_fe_equal(&y, &z));
        CHECK(check_fe_equal(&q, &y));
    }
}

void test_fe_mul(const secp256k1_fe* a, const secp256k1_fe* b, int use_sqr)
{
    secp256k1_fe c, an, bn;
    /* Variables in BE 32-byte format. */
    unsigned char a32[32], b32[32], c32[32];
    /* Variables in LE 16x uint16_t format. */
    uint16_t a16[16], b16[16], c16[16];
    /* Field modulus in LE 16x uint16_t format. */
    static const uint16_t m16[16] = {
        0xfc2f, 0xffff, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    };
    uint16_t t16[32];
    int i;

    /* Compute C = A * B in fe format. */
    c = *a;
    if (use_sqr) {
        secp256k1_fe_sqr(&c, &c);
    } else {
        secp256k1_fe_mul(&c, &c, b);
    }

    /* Convert A, B, C into LE 16x uint16_t format. */
    an = *a;
    bn = *b;
    secp256k1_fe_normalize_var(&c);
    secp256k1_fe_normalize_var(&an);
    secp256k1_fe_normalize_var(&bn);
    secp256k1_fe_get_b32(a32, &an);
    secp256k1_fe_get_b32(b32, &bn);
    secp256k1_fe_get_b32(c32, &c);
    for (i = 0; i < 16; ++i) {
        a16[i] = a32[31 - 2*i] + ((uint16_t)a32[30 - 2*i] << 8);
        b16[i] = b32[31 - 2*i] + ((uint16_t)b32[30 - 2*i] << 8);
        c16[i] = c32[31 - 2*i] + ((uint16_t)c32[30 - 2*i] << 8);
    }
    /* Compute T = A * B in LE 16x uint16_t format. */
    mulmod256(t16, a16, b16, m16);
    /* Compare */
    CHECK(secp256k1_memcmp_var(t16, c16, 32) == 0);
}

void run_fe_mul(void) {
    int i;
    for (i = 0; i < 100 * count; ++i) {
        secp256k1_fe a, b, c, d;
        random_fe(&a);
        random_field_element_magnitude(&a);
        random_fe(&b);
        random_field_element_magnitude(&b);
        random_fe_test(&c);
        random_field_element_magnitude(&c);
        random_fe_test(&d);
        random_field_element_magnitude(&d);
        test_fe_mul(&a, &a, 1);
        test_fe_mul(&c, &c, 1);
        test_fe_mul(&a, &b, 0);
        test_fe_mul(&a, &c, 0);
        test_fe_mul(&c, &b, 0);
        test_fe_mul(&c, &d, 0);
    }
}

void run_sqr(void) {
    secp256k1_fe x, s;

    {
        int i;
        secp256k1_fe_set_int(&x, 1);
        secp256k1_fe_negate(&x, &x, 1);

        for (i = 1; i <= 512; ++i) {
            secp256k1_fe_mul_int(&x, 2);
            secp256k1_fe_normalize(&x);
            secp256k1_fe_sqr(&s, &x);
        }
    }
}

void test_sqrt(const secp256k1_fe *a, const secp256k1_fe *k) {
    secp256k1_fe r1, r2;
    int v = secp256k1_fe_sqrt(&r1, a);
    CHECK((v == 0) == (k == NULL));

    if (k != NULL) {
        /* Check that the returned root is +/- the given known answer */
        secp256k1_fe_negate(&r2, &r1, 1);
        secp256k1_fe_add(&r1, k); secp256k1_fe_add(&r2, k);
        secp256k1_fe_normalize(&r1); secp256k1_fe_normalize(&r2);
        CHECK(secp256k1_fe_is_zero(&r1) || secp256k1_fe_is_zero(&r2));
    }
}

void run_sqrt(void) {
    secp256k1_fe ns, x, s, t;
    int i;

    /* Check sqrt(0) is 0 */
    secp256k1_fe_set_int(&x, 0);
    secp256k1_fe_sqr(&s, &x);
    test_sqrt(&s, &x);

    /* Check sqrt of small squares (and their negatives) */
    for (i = 1; i <= 100; i++) {
        secp256k1_fe_set_int(&x, i);
        secp256k1_fe_sqr(&s, &x);
        test_sqrt(&s, &x);
        secp256k1_fe_negate(&t, &s, 1);
        test_sqrt(&t, NULL);
    }

    /* Consistency checks for large random values */
    for (i = 0; i < 10; i++) {
        int j;
        random_fe_non_square(&ns);
        for (j = 0; j < count; j++) {
            random_fe(&x);
            secp256k1_fe_sqr(&s, &x);
            test_sqrt(&s, &x);
            secp256k1_fe_negate(&t, &s, 1);
            test_sqrt(&t, NULL);
            secp256k1_fe_mul(&t, &s, &ns);
            test_sqrt(&t, NULL);
        }
    }
}

/***** FIELD/SCALAR INVERSE TESTS *****/

static const secp256k1_scalar scalar_minus_one = SECP256K1_SCALAR_CONST(
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE,
    0xBAAEDCE6, 0xAF48A03B, 0xBFD25E8C, 0xD0364140
);

static const secp256k1_fe fe_minus_one = SECP256K1_FE_CONST(
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFC2E
);

/* These tests test the following identities:
 *
 * for x==0: 1/x == 0
 * for x!=0: x*(1/x) == 1
 * for x!=0 and x!=1: 1/(1/x - 1) + 1 == -1/(x-1)
 */

void test_inverse_scalar(secp256k1_scalar* out, const secp256k1_scalar* x, int var)
{
    secp256k1_scalar l, r, t;

    (var ? secp256k1_scalar_inverse_var : secp256k1_scalar_inverse)(&l, x);  /* l = 1/x */
    if (out) *out = l;
    if (secp256k1_scalar_is_zero(x)) {
        CHECK(secp256k1_scalar_is_zero(&l));
        return;
    }
    secp256k1_scalar_mul(&t, x, &l);                                             /* t = x*(1/x) */
    CHECK(secp256k1_scalar_is_one(&t));                                          /* x*(1/x) == 1 */
    secp256k1_scalar_add(&r, x, &scalar_minus_one);                              /* r = x-1 */
    if (secp256k1_scalar_is_zero(&r)) return;
    (var ? secp256k1_scalar_inverse_var : secp256k1_scalar_inverse)(&r, &r); /* r = 1/(x-1) */
    secp256k1_scalar_add(&l, &scalar_minus_one, &l);                             /* l = 1/x-1 */
    (var ? secp256k1_scalar_inverse_var : secp256k1_scalar_inverse)(&l, &l); /* l = 1/(1/x-1) */
    secp256k1_scalar_add(&l, &l, &secp256k1_scalar_one);                         /* l = 1/(1/x-1)+1 */
    secp256k1_scalar_add(&l, &r, &l);                                            /* l = 1/(1/x-1)+1 + 1/(x-1) */
    CHECK(secp256k1_scalar_is_zero(&l));                                         /* l == 0 */
}

void test_inverse_field(secp256k1_fe* out, const secp256k1_fe* x, int var)
{
    secp256k1_fe l, r, t;

    (var ? secp256k1_fe_inv_var : secp256k1_fe_inv)(&l, x) ;   /* l = 1/x */
    if (out) *out = l;
    t = *x;                                                    /* t = x */
    if (secp256k1_fe_normalizes_to_zero_var(&t)) {
        CHECK(secp256k1_fe_normalizes_to_zero(&l));
        return;
    }
    secp256k1_fe_mul(&t, x, &l);                               /* t = x*(1/x) */
    secp256k1_fe_add(&t, &fe_minus_one);                       /* t = x*(1/x)-1 */
    CHECK(secp256k1_fe_normalizes_to_zero(&t));                /* x*(1/x)-1 == 0 */
    r = *x;                                                    /* r = x */
    secp256k1_fe_add(&r, &fe_minus_one);                       /* r = x-1 */
    if (secp256k1_fe_normalizes_to_zero_var(&r)) return;
    (var ? secp256k1_fe_inv_var : secp256k1_fe_inv)(&r, &r);   /* r = 1/(x-1) */
    secp256k1_fe_add(&l, &fe_minus_one);                       /* l = 1/x-1 */
    (var ? secp256k1_fe_inv_var : secp256k1_fe_inv)(&l, &l);   /* l = 1/(1/x-1) */
    secp256k1_fe_add(&l, &secp256k1_fe_one);                   /* l = 1/(1/x-1)+1 */
    secp256k1_fe_add(&l, &r);                                  /* l = 1/(1/x-1)+1 + 1/(x-1) */
    CHECK(secp256k1_fe_normalizes_to_zero_var(&l));            /* l == 0 */
}

void run_inverse_tests(void)
{
    /* Fixed test cases for field inverses: pairs of (x, 1/x) mod p. */
    static const secp256k1_fe fe_cases[][2] = {
        /* 0 */
        {SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0),
         SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0)},
        /* 1 */
        {SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 1),
         SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 1)},
        /* -1 */
        {SECP256K1_FE_CONST(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfffffffe, 0xfffffc2e),
         SECP256K1_FE_CONST(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfffffffe, 0xfffffc2e)},
        /* 2 */
        {SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 2),
         SECP256K1_FE_CONST(0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x7ffffe18)},
        /* 2**128 */
        {SECP256K1_FE_CONST(0, 0, 0, 1, 0, 0, 0, 0),
         SECP256K1_FE_CONST(0xbcb223fe, 0xdc24a059, 0xd838091d, 0xd2253530, 0xffffffff, 0xffffffff, 0xffffffff, 0x434dd931)},
        /* Input known to need 637 divsteps */
        {SECP256K1_FE_CONST(0xe34e9c95, 0x6bee8a84, 0x0dcb632a, 0xdb8a1320, 0x66885408, 0x06f3f996, 0x7c11ca84, 0x19199ec3),
         SECP256K1_FE_CONST(0xbd2cbd8f, 0x1c536828, 0x9bccda44, 0x2582ac0c, 0x870152b0, 0x8a3f09fb, 0x1aaadf92, 0x19b618e5)},
        /* Input known to need 567 divsteps starting with delta=1/2. */
        {SECP256K1_FE_CONST(0xf6bc3ba3, 0x636451c4, 0x3e46357d, 0x2c21d619, 0x0988e234, 0x15985661, 0x6672982b, 0xa7549bfc),
         SECP256K1_FE_CONST(0xb024fdc7, 0x5547451e, 0x426c585f, 0xbd481425, 0x73df6b75, 0xeef6d9d0, 0x389d87d4, 0xfbb440ba)},
        /* Input known to need 566 divsteps starting with delta=1/2. */
        {SECP256K1_FE_CONST(0xb595d81b, 0x2e3c1e2f, 0x482dbc65, 0xe4865af7, 0x9a0a50aa, 0x29f9e618, 0x6f87d7a5, 0x8d1063ae),
         SECP256K1_FE_CONST(0xc983337c, 0x5d5c74e1, 0x49918330, 0x0b53afb5, 0xa0428a0b, 0xce6eef86, 0x059bd8ef, 0xe5b908de)},
        /* Set of 10 inputs accessing all 128 entries in the modinv32 divsteps_var table */
        {SECP256K1_FE_CONST(0x00000000, 0x00000000, 0xe0ff1f80, 0x1f000000, 0x00000000, 0x00000000, 0xfeff0100, 0x00000000),
         SECP256K1_FE_CONST(0x9faf9316, 0x77e5049d, 0x0b5e7a1b, 0xef70b893, 0x18c9e30c, 0x045e7fd7, 0x29eddf8c, 0xd62e9e3d)},
        {SECP256K1_FE_CONST(0x621a538d, 0x511b2780, 0x35688252, 0x53f889a4, 0x6317c3ac, 0x32ba0a46, 0x6277c0d1, 0xccd31192),
         SECP256K1_FE_CONST(0x38513b0c, 0x5eba856f, 0xe29e882e, 0x9b394d8c, 0x34bda011, 0xeaa66943, 0x6a841a4c, 0x6ae8bcff)},
        {SECP256K1_FE_CONST(0x00000200, 0xf0ffff1f, 0x00000000, 0x0000e0ff, 0xffffffff, 0xfffcffff, 0xffffffff, 0xffff0100),
         SECP256K1_FE_CONST(0x5da42a52, 0x3640de9e, 0x13e64343, 0x0c7591b7, 0x6c1e3519, 0xf048c5b6, 0x0484217c, 0xedbf8b2f)},
        {SECP256K1_FE_CONST(0xd1343ef9, 0x4b952621, 0x7c52a2ee, 0x4ea1281b, 0x4ab46410, 0x9f26998d, 0xa686a8ff, 0x9f2103e8),
         SECP256K1_FE_CONST(0x84044385, 0x9a4619bf, 0x74e35b6d, 0xa47e0c46, 0x6b7fb47d, 0x9ffab128, 0xb0775aa3, 0xcb318bd1)},
        {SECP256K1_FE_CONST(0xb27235d2, 0xc56a52be, 0x210db37a, 0xd50d23a4, 0xbe621bdd, 0x5df22c6a, 0xe926ba62, 0xd2e4e440),
         SECP256K1_FE_CONST(0x67a26e54, 0x483a9d3c, 0xa568469e, 0xd258ab3d, 0xb9ec9981, 0xdca9b1bd, 0x8d2775fe, 0x53ae429b)},
        {SECP256K1_FE_CONST(0x00000000, 0x00000000, 0x00e0ffff, 0xffffff83, 0xffffffff, 0x3f00f00f, 0x000000e0, 0xffffffff),
         SECP256K1_FE_CONST(0x310e10f8, 0x23bbfab0, 0xac94907d, 0x076c9a45, 0x8d357d7f, 0xc763bcee, 0x00d0e615, 0x5a6acef6)},
        {SECP256K1_FE_CONST(0xfeff0300, 0x001c0000, 0xf80700c0, 0x0ff0ffff, 0xffffffff, 0x0fffffff, 0xffff0100, 0x7f0000fe),
         SECP256K1_FE_CONST(0x28e2fdb4, 0x0709168b, 0x86f598b0, 0x3453a370, 0x530cf21f, 0x32f978d5, 0x1d527a71, 0x59269b0c)},
        {SECP256K1_FE_CONST(0xc2591afa, 0x7bb98ef7, 0x090bb273, 0x85c14f87, 0xbb0b28e0, 0x54d3c453, 0x85c66753, 0xd5574d2f),
         SECP256K1_FE_CONST(0xfdca70a2, 0x70ce627c, 0x95e66fae, 0x848a6dbb, 0x07ffb15c, 0x5f63a058, 0xba4140ed, 0x6113b503)},
        {SECP256K1_FE_CONST(0xf5475db3, 0xedc7b5a3, 0x411c047e, 0xeaeb452f, 0xc625828e, 0x1cf5ad27, 0x8eec1060, 0xc7d3e690),
         SECP256K1_FE_CONST(0x5eb756c0, 0xf963f4b9, 0xdc6a215e, 0xec8cc2d8, 0x2e9dec01, 0xde5eb88d, 0x6aba7164, 0xaecb2c5a)},
        {SECP256K1_FE_CONST(0x00000000, 0x00f8ffff, 0xffffffff, 0x01000000, 0xe0ff1f00, 0x00000000, 0xffffff7f, 0x00000000),
         SECP256K1_FE_CONST(0xe0d2e3d8, 0x49b6157d, 0xe54e88c2, 0x1a7f02ca, 0x7dd28167, 0xf1125d81, 0x7bfa444e, 0xbe110037)},
        /* Selection of randomly generated inputs that reach high/low d/e values in various configurations. */
        {SECP256K1_FE_CONST(0x13cc08a4, 0xd8c41f0f, 0x179c3e67, 0x54c46c67, 0xc4109221, 0x09ab3b13, 0xe24d9be1, 0xffffe950),
         SECP256K1_FE_CONST(0xb80c8006, 0xd16abaa7, 0xcabd71e5, 0xcf6714f4, 0x966dd3d0, 0x64767a2d, 0xe92c4441, 0x51008cd1)},
        {SECP256K1_FE_CONST(0xaa6db990, 0x95efbca1, 0x3cc6ff71, 0x0602e24a, 0xf49ff938, 0x99fffc16, 0x46f40993, 0xc6e72057),
         SECP256K1_FE_CONST(0xd5d3dd69, 0xb0c195e5, 0x285f1d49, 0xe639e48c, 0x9223f8a9, 0xca1d731d, 0x9ca482f9, 0xa5b93e06)},
        {SECP256K1_FE_CONST(0x1c680eac, 0xaeabffd8, 0x9bdc4aee, 0x1781e3de, 0xa3b08108, 0x0015f2e0, 0x94449e1b, 0x2f67a058),
         SECP256K1_FE_CONST(0x7f083f8d, 0x31254f29, 0x6510f475, 0x245c373d, 0xc5622590, 0x4b323393, 0x32ed1719, 0xc127444b)},
        {SECP256K1_FE_CONST(0x147d44b3, 0x012d83f8, 0xc160d386, 0x1a44a870, 0x9ba6be96, 0x8b962707, 0x267cbc1a, 0xb65b2f0a),
         SECP256K1_FE_CONST(0x555554ff, 0x170aef1e, 0x50a43002, 0xe51fbd36, 0xafadb458, 0x7a8aded1, 0x0ca6cd33, 0x6ed9087c)},
        {SECP256K1_FE_CONST(0x12423796, 0x22f0fe61, 0xf9ca017c, 0x5384d107, 0xa1fbf3b2, 0x3b018013, 0x916a3c37, 0x4000b98c),
         SECP256K1_FE_CONST(0x20257700, 0x08668f94, 0x1177e306, 0x136c01f5, 0x8ed1fbd2, 0x95ec4589, 0xae38edb9, 0xfd19b6d7)},
        {SECP256K1_FE_CONST(0xdcf2d030, 0x9ab42cb4, 0x93ffa181, 0xdcd23619, 0x39699b52, 0x08909a20, 0xb5a17695, 0x3a9dcf21),
         SECP256K1_FE_CONST(0x1f701dea, 0xe211fb1f, 0x4f37180d, 0x63a0f51c, 0x29fe1e40, 0xa40b6142, 0x2e7b12eb, 0x982b06b6)},
        {SECP256K1_FE_CONST(0x79a851f6, 0xa6314ed3, 0xb35a55e6, 0xca1c7d7f, 0xe32369ea, 0xf902432e, 0x375308c5, 0xdfd5b600),
         SECP256K1_FE_CONST(0xcaae00c5, 0xe6b43851, 0x9dabb737, 0x38cba42c, 0xa02c8549, 0x7895dcbf, 0xbd183d71, 0xafe4476a)},
        {SECP256K1_FE_CONST(0xede78fdd, 0xcfc92bf1, 0x4fec6c6c, 0xdb8d37e2, 0xfb66bc7b, 0x28701870, 0x7fa27c9a, 0x307196ec),
         SECP256K1_FE_CONST(0x68193a6c, 0x9a8b87a7, 0x2a760c64, 0x13e473f6, 0x23ae7bed, 0x1de05422, 0x88865427, 0xa3418265)},
        {SECP256K1_FE_CONST(0xa40b2079, 0xb8f88e89, 0xa7617997, 0x89baf5ae, 0x174df343, 0x75138eae, 0x2711595d, 0x3fc3e66c),
         SECP256K1_FE_CONST(0x9f99c6a5, 0x6d685267, 0xd4b87c37, 0x9d9c4576, 0x358c692b, 0x6bbae0ed, 0x3389c93d, 0x7fdd2655)},
        {SECP256K1_FE_CONST(0x7c74c6b6, 0xe98d9151, 0x72645cf1, 0x7f06e321, 0xcefee074, 0x15b2113a, 0x10a9be07, 0x08a45696),
         SECP256K1_FE_CONST(0x8c919a88, 0x898bc1e0, 0x77f26f97, 0x12e655b7, 0x9ba0ac40, 0xe15bb19e, 0x8364cc3b, 0xe227a8ee)},
        {SECP256K1_FE_CONST(0x109ba1ce, 0xdafa6d4a, 0xa1cec2b2, 0xeb1069f4, 0xb7a79e5b, 0xec6eb99b, 0xaec5f643, 0xee0e723e),
         SECP256K1_FE_CONST(0x93d13eb8, 0x4bb0bcf9, 0xe64f5a71, 0xdbe9f359, 0x7191401c, 0x6f057a4a, 0xa407fe1b, 0x7ecb65cc)},
        {SECP256K1_FE_CONST(0x3db076cd, 0xec74a5c9, 0xf61dd138, 0x90e23e06, 0xeeedd2d0, 0x74cbc4e0, 0x3dbe1e91, 0xded36a78),
         SECP256K1_FE_CONST(0x3f07f966, 0x8e2a1e09, 0x706c71df, 0x02b5e9d5, 0xcb92ddbf, 0xcdd53010, 0x16545564, 0xe660b107)},
        {SECP256K1_FE_CONST(0xe31c73ed, 0xb4c4b82c, 0x02ae35f7, 0x4cdec153, 0x98b522fd, 0xf7d2460c, 0x6bf7c0f8, 0x4cf67b0d),
         SECP256K1_FE_CONST(0x4b8f1faf, 0x94e8b070, 0x19af0ff6, 0xa319cd31, 0xdf0a7ffb, 0xefaba629, 0x59c50666, 0x1fe5b843)},
        {SECP256K1_FE_CONST(0x4c8b0e6e, 0x83392ab6, 0xc0e3e9f1, 0xbbd85497, 0x16698897, 0xf552d50d, 0x79652ddb, 0x12f99870),
         SECP256K1_FE_CONST(0x56d5101f, 0xd23b7949, 0x17dc38d6, 0xf24022ef, 0xcf18e70a, 0x5cc34424, 0x438544c3, 0x62da4bca)},
        {SECP256K1_FE_CONST(0xb0e040e2, 0x40cc35da, 0x7dd5c611, 0x7fccb178, 0x28888137, 0xbc930358, 0xea2cbc90, 0x775417dc),
         SECP256K1_FE_CONST(0xca37f0d4, 0x016dd7c8, 0xab3ae576, 0x96e08d69, 0x68ed9155, 0xa9b44270, 0x900ae35d, 0x7c7800cd)},
        {SECP256K1_FE_CONST(0x8a32ea49, 0x7fbb0bae, 0x69724a9d, 0x8e2105b2, 0xbdf69178, 0x862577ef, 0x35055590, 0x667ddaef),
         SECP256K1_FE_CONST(0xd02d7ead, 0xc5e190f0, 0x559c9d72, 0xdaef1ffc, 0x64f9f425, 0xf43645ea, 0x7341e08d, 0x11768e96)},
        {SECP256K1_FE_CONST(0xa3592d98, 0x9abe289d, 0x579ebea6, 0xbb0857a8, 0xe242ab73, 0x85f9a2ce, 0xb6998f0f, 0xbfffbfc6),
         SECP256K1_FE_CONST(0x093c1533, 0x32032efa, 0x6aa46070, 0x0039599e, 0x589c35f4, 0xff525430, 0x7fe3777a, 0x44b43ddc)},
        {SECP256K1_FE_CONST(0x647178a3, 0x229e607b, 0xcc98521a, 0xcce3fdd9, 0x1e1bc9c9, 0x97fb7c6a, 0x61b961e0, 0x99b10709),
         SECP256K1_FE_CONST(0x98217c13, 0xd51ddf78, 0x96310e77, 0xdaebd908, 0x602ca683, 0xcb46d07a, 0xa1fcf17e, 0xc8e2feb3)},
        {SECP256K1_FE_CONST(0x7334627c, 0x73f98968, 0x99464b4b, 0xf5964958, 0x1b95870d, 0xc658227e, 0x5e3235d8, 0xdcab5787),
         SECP256K1_FE_CONST(0x000006fd, 0xc7e9dd94, 0x40ae367a, 0xe51d495c, 0x07603b9b, 0x2d088418, 0x6cc5c74c, 0x98514307)},
        {SECP256K1_FE_CONST(0x82e83876, 0x96c28938, 0xa50dd1c5, 0x605c3ad1, 0xc048637d, 0x7a50825f, 0x335ed01a, 0x00005760),
         SECP256K1_FE_CONST(0xb0393f9f, 0x9f2aa55e, 0xf5607e2e, 0x5287d961, 0x60b3e704, 0xf3e16e80, 0xb4f9a3ea, 0xfec7f02d)},
        {SECP256K1_FE_CONST(0xc97b6cec, 0x3ee6b8dc, 0x98d24b58, 0x3c1970a1, 0xfe06297a, 0xae813529, 0xe76bb6bd, 0x771ae51d),
         SECP256K1_FE_CONST(0x0507c702, 0xd407d097, 0x47ddeb06, 0xf6625419, 0x79f48f79, 0x7bf80d0b, 0xfc34b364, 0x253a5db1)},
        {SECP256K1_FE_CONST(0xd559af63, 0x77ea9bc4, 0x3cf1ad14, 0x5c7a4bbb, 0x10e7d18b, 0x7ce0dfac, 0x380bb19d, 0x0bb99bd3),
         SECP256K1_FE_CONST(0x00196119, 0xb9b00d92, 0x34edfdb5, 0xbbdc42fc, 0xd2daa33a, 0x163356ca, 0xaa8754c8, 0xb0ec8b0b)},
        {SECP256K1_FE_CONST(0x8ddfa3dc, 0x52918da0, 0x640519dc, 0x0af8512a, 0xca2d33b2, 0xbde52514, 0xda9c0afc, 0xcb29fce4),
         SECP256K1_FE_CONST(0xb3e4878d, 0x5cb69148, 0xcd54388b, 0xc23acce0, 0x62518ba8, 0xf09def92, 0x7b31e6aa, 0x6ba35b02)},
        {SECP256K1_FE_CONST(0xf8207492, 0xe3049f0a, 0x65285f2b, 0x0bfff996, 0x00ca112e, 0xc05da837, 0x546d41f9, 0x5194fb91),
         SECP256K1_FE_CONST(0x7b7ee50b, 0xa8ed4bbd, 0xf6469930, 0x81419a5c, 0x071441c7, 0x290d046e, 0x3b82ea41, 0x611c5f95)},
        {SECP256K1_FE_CONST(0x050f7c80, 0x5bcd3c6b, 0x823cb724, 0x5ce74db7, 0xa4e39f5c, 0xbd8828d7, 0xfd4d3e07, 0x3ec2926a),
         SECP256K1_FE_CONST(0x000d6730, 0xb0171314, 0x4764053d, 0xee157117, 0x48fd61da, 0xdea0b9db, 0x1d5e91c6, 0xbdc3f59e)},
        {SECP256K1_FE_CONST(0x3e3ea8eb, 0x05d760cf, 0x23009263, 0xb3cb3ac9, 0x088f6f0d, 0x3fc182a3, 0xbd57087c, 0xe67c62f9),
         SECP256K1_FE_CONST(0xbe988716, 0xa29c1bf6, 0x4456aed6, 0xab1e4720, 0x49929305, 0x51043bf4, 0xebd833dd, 0xdd511e8b)},
        {SECP256K1_FE_CONST(0x6964d2a9, 0xa7fa6501, 0xa5959249, 0x142f4029, 0xea0c1b5f, 0x2f487ef6, 0x301ac80a, 0x768be5cd),
         SECP256K1_FE_CONST(0x3918ffe4, 0x07492543, 0xed24d0b7, 0x3df95f8f, 0xaffd7cb4, 0x0de2191c, 0x9ec2f2ad, 0x2c0cb3c6)},
        {SECP256K1_FE_CONST(0x37c93520, 0xf6ddca57, 0x2b42fd5e, 0xb5c7e4de, 0x11b5b81c, 0xb95e91f3, 0x95c4d156, 0x39877ccb),
         SECP256K1_FE_CONST(0x9a94b9b5, 0x57eb71ee, 0x4c975b8b, 0xac5262a8, 0x077b0595, 0xe12a6b1f, 0xd728edef, 0x1a6bf956)}
    };
    /* Fixed test cases for scalar inverses: pairs of (x, 1/x) mod n. */
    static const secp256k1_scalar scalar_cases[][2] = {
        /* 0 */
        {SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0),
         SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0)},
        /* 1 */
        {SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 1),
         SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 1)},
        /* -1 */
        {SECP256K1_SCALAR_CONST(0xffffffff, 0xffffffff, 0xffffffff, 0xfffffffe, 0xbaaedce6, 0xaf48a03b, 0xbfd25e8c, 0xd0364140),
         SECP256K1_SCALAR_CONST(0xffffffff, 0xffffffff, 0xffffffff, 0xfffffffe, 0xbaaedce6, 0xaf48a03b, 0xbfd25e8c, 0xd0364140)},
        /* 2 */
        {SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 2),
         SECP256K1_SCALAR_CONST(0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x5d576e73, 0x57a4501d, 0xdfe92f46, 0x681b20a1)},
        /* 2**128 */
        {SECP256K1_SCALAR_CONST(0, 0, 0, 1, 0, 0, 0, 0),
         SECP256K1_SCALAR_CONST(0x50a51ac8, 0x34b9ec24, 0x4b0dff66, 0x5588b13e, 0x9984d5b3, 0xcf80ef0f, 0xd6a23766, 0xa3ee9f22)},
        /* Input known to need 635 divsteps */
        {SECP256K1_SCALAR_CONST(0xcb9f1d35, 0xdd4416c2, 0xcd71bf3f, 0x6365da66, 0x3c9b3376, 0x8feb7ae9, 0x32a5ef60, 0x19199ec3),
         SECP256K1_SCALAR_CONST(0x1d7c7bba, 0xf1893d53, 0xb834bd09, 0x36b411dc, 0x42c2e42f, 0xec72c428, 0x5e189791, 0x8e9bc708)},
        /* Input known to need 566 divsteps starting with delta=1/2. */
        {SECP256K1_SCALAR_CONST(0x7e3c993d, 0xa4272488, 0xbc015b49, 0x2db54174, 0xd382083a, 0xebe6db35, 0x80f82eff, 0xcd132c72),
         SECP256K1_SCALAR_CONST(0x086f34a0, 0x3e631f76, 0x77418f28, 0xcc84ac95, 0x6304439d, 0x365db268, 0x312c6ded, 0xd0b934f8)},
        /* Input known to need 565 divsteps starting with delta=1/2. */
        {SECP256K1_SCALAR_CONST(0xbad7e587, 0x3f307859, 0x60d93147, 0x8a18491e, 0xb38a9fd5, 0x254350d3, 0x4b1f0e4b, 0x7dd6edc4),
         SECP256K1_SCALAR_CONST(0x89f2df26, 0x39e2b041, 0xf19bd876, 0xd039c8ac, 0xc2223add, 0x29c4943e, 0x6632d908, 0x515f467b)},
        /* Selection of randomly generated inputs that reach low/high d/e values in various configurations. */
        {SECP256K1_SCALAR_CONST(0x1950d757, 0xb37a5809, 0x435059bb, 0x0bb8997e, 0x07e1e3c8, 0x5e5d7d2c, 0x6a0ed8e3, 0xdbde180e),
         SECP256K1_SCALAR_CONST(0xbf72af9b, 0x750309e2, 0x8dda230b, 0xfe432b93, 0x7e25e475, 0x4388251e, 0x633d894b, 0x3bcb6f8c)},
        {SECP256K1_SCALAR_CONST(0x9bccf4e7, 0xc5a515e3, 0x50637aa9, 0xbb65a13f, 0x391749a1, 0x62de7d4e, 0xf6d7eabb, 0x3cd10ce0),
         SECP256K1_SCALAR_CONST(0xaf2d5623, 0xb6385a33, 0xcd0365be, 0x5e92a70d, 0x7f09179c, 0x3baaf30f, 0x8f9cc83b, 0x20092f67)},
        {SECP256K1_SCALAR_CONST(0x73a57111, 0xb242952a, 0x5c5dee59, 0xf3be2ace, 0xa30a7659, 0xa46e5f47, 0xd21267b1, 0x39e642c9),
         SECP256K1_SCALAR_CONST(0xa711df07, 0xcbcf13ef, 0xd61cc6be, 0xbcd058ce, 0xb02cf157, 0x272d4a18, 0x86d0feb3, 0xcd5fa004)},
        {SECP256K1_SCALAR_CONST(0x04884963, 0xce0580b1, 0xba547030, 0x3c691db3, 0x9cd2c84f, 0x24c7cebd, 0x97ebfdba, 0x3e785ec2),
         SECP256K1_SCALAR_CONST(0xaaaaaf14, 0xd7c99ba7, 0x517ce2c1, 0x78a28b4c, 0x3769a851, 0xe5c5a03d, 0x4cc28f33, 0x0ec4dc5d)},
        {SECP256K1_SCALAR_CONST(0x1679ed49, 0x21f537b1, 0x815cb8ae, 0x9efc511c, 0x5b9fa037, 0x0b0f275e, 0x6c985281, 0x6c4a9905),
         SECP256K1_SCALAR_CONST(0xb14ac3d5, 0x62b52999, 0xef34ead1, 0xffca4998, 0x0294341a, 0x1f8172aa, 0xea1624f9, 0x302eea62)},
        {SECP256K1_SCALAR_CONST(0x626b37c0, 0xf0057c35, 0xee982f83, 0x452a1fd3, 0xea826506, 0x48b08a9d, 0x1d2c4799, 0x4ad5f6ec),
         SECP256K1_SCALAR_CONST(0xe38643b7, 0x567bfc2f, 0x5d2f1c15, 0xe327239c, 0x07112443, 0x69509283, 0xfd98e77a, 0xdb71c1e8)},
        {SECP256K1_SCALAR_CONST(0x1850a3a7, 0x759efc56, 0x54f287b2, 0x14d1234b, 0xe263bbc9, 0xcf4d8927, 0xd5f85f27, 0x965bd816),
         SECP256K1_SCALAR_CONST(0x3b071831, 0xcac9619a, 0xcceb0596, 0xf614d63b, 0x95d0db2f, 0xc6a00901, 0x8eaa2621, 0xabfa0009)},
        {SECP256K1_SCALAR_CONST(0x94ae5d06, 0xa27dc400, 0x487d72be, 0xaa51ebed, 0xe475b5c0, 0xea675ffc, 0xf4df627a, 0xdca4222f),
         SECP256K1_SCALAR_CONST(0x01b412ed, 0xd7830956, 0x1532537e, 0xe5e3dc99, 0x8fd3930a, 0x54f8d067, 0x32ef5760, 0x594438a5)},
        {SECP256K1_SCALAR_CONST(0x1f24278a, 0xb5bfe374, 0xa328dbbc, 0xebe35f48, 0x6620e009, 0xd58bb1b4, 0xb5a6bf84, 0x8815f63a),
         SECP256K1_SCALAR_CONST(0xfe928416, 0xca5ba2d3, 0xfde513da, 0x903a60c7, 0x9e58ad8a, 0x8783bee4, 0x083a3843, 0xa608c914)},
        {SECP256K1_SCALAR_CONST(0xdc107d58, 0x274f6330, 0x67dba8bc, 0x26093111, 0x5201dfb8, 0x968ce3f5, 0xf34d1bd4, 0xf2146504),
         SECP256K1_SCALAR_CONST(0x660cfa90, 0x13c3d93e, 0x7023b1e5, 0xedd09e71, 0x6d9c9d10, 0x7a3d2cdb, 0xdd08edc3, 0xaa78fcfb)},
        {SECP256K1_SCALAR_CONST(0x7cd1e905, 0xc6f02776, 0x2f551cc7, 0x5da61cff, 0x7da05389, 0x1119d5a4, 0x631c7442, 0x894fd4f7),
         SECP256K1_SCALAR_CONST(0xff20862a, 0x9d3b1a37, 0x1628803b, 0x3004ccae, 0xaa23282a, 0xa89a1109, 0xd94ece5e, 0x181bdc46)},
        {SECP256K1_SCALAR_CONST(0x5b9dade8, 0x23d26c58, 0xcd12d818, 0x25b8ae97, 0x3dea04af, 0xf482c96b, 0xa062f254, 0x9e453640),
         SECP256K1_SCALAR_CONST(0x50c38800, 0x15fa53f4, 0xbe1e5392, 0x5c9b120a, 0x262c22c7, 0x18fa0816, 0x5f2baab4, 0x8cb5db46)},
        {SECP256K1_SCALAR_CONST(0x11cdaeda, 0x969c464b, 0xef1f4ab0, 0x5b01d22e, 0x656fd098, 0x882bea84, 0x65cdbe7a, 0x0c19ff03),
         SECP256K1_SCALAR_CONST(0x1968d0fa, 0xac46f103, 0xb55f1f72, 0xb3820bed, 0xec6b359a, 0x4b1ae0ad, 0x7e38e1fb, 0x295ccdfb)},
        {SECP256K1_SCALAR_CONST(0x2c351aa1, 0x26e91589, 0x194f8a1e, 0x06561f66, 0x0cb97b7f, 0x10914454, 0x134d1c03, 0x157266b4),
         SECP256K1_SCALAR_CONST(0xbe49ada6, 0x92bd8711, 0x41b176c4, 0xa478ba95, 0x14883434, 0x9d1cd6f3, 0xcc4b847d, 0x22af80f5)},
        {SECP256K1_SCALAR_CONST(0x6ba07c6e, 0x13a60edb, 0x6247f5c3, 0x84b5fa56, 0x76fe3ec5, 0x80426395, 0xf65ec2ae, 0x623ba730),
         SECP256K1_SCALAR_CONST(0x25ac23f7, 0x418cd747, 0x98376f9d, 0x4a11c7bf, 0x24c8ebfe, 0x4c8a8655, 0x345f4f52, 0x1c515595)},
        {SECP256K1_SCALAR_CONST(0x9397a712, 0x8abb6951, 0x2d4a3d54, 0x703b1c2a, 0x0661dca8, 0xd75c9b31, 0xaed4d24b, 0xd2ab2948),
         SECP256K1_SCALAR_CONST(0xc52e8bef, 0xd55ce3eb, 0x1c897739, 0xeb9fb606, 0x36b9cd57, 0x18c51cc2, 0x6a87489e, 0xffd0dcf3)},
        {SECP256K1_SCALAR_CONST(0xe6a808cc, 0xeb437888, 0xe97798df, 0x4e224e44, 0x7e3b380a, 0x207c1653, 0x889f3212, 0xc6738b6f),
         SECP256K1_SCALAR_CONST(0x31f9ae13, 0xd1e08b20, 0x757a2e5e, 0x5243a0eb, 0x8ae35f73, 0x19bb6122, 0xb910f26b, 0xda70aa55)},
        {SECP256K1_SCALAR_CONST(0xd0320548, 0xab0effe7, 0xa70779e0, 0x61a347a6, 0xb8c1e010, 0x9d5281f8, 0x2ee588a6, 0x80000000),
         SECP256K1_SCALAR_CONST(0x1541897e, 0x78195c90, 0x7583dd9e, 0x728b6100, 0xbce8bc6d, 0x7a53b471, 0x5dcd9e45, 0x4425fcaf)},
        {SECP256K1_SCALAR_CONST(0x93d623f1, 0xd45b50b0, 0x796e9186, 0x9eac9407, 0xd30edc20, 0xef6304cf, 0x250494e7, 0xba503de9),
         SECP256K1_SCALAR_CONST(0x7026d638, 0x1178b548, 0x92043952, 0x3c7fb47c, 0xcd3ea236, 0x31d82b01, 0x612fc387, 0x80b9b957)},
        {SECP256K1_SCALAR_CONST(0xf860ab39, 0x55f5d412, 0xa4d73bcc, 0x3b48bd90, 0xc248ffd3, 0x13ca10be, 0x8fba84cc, 0xdd28d6a3),
         SECP256K1_SCALAR_CONST(0x5c32fc70, 0xe0b15d67, 0x76694700, 0xfe62be4d, 0xeacdb229, 0x7a4433d9, 0x52155cd0, 0x7649ab59)},
        {SECP256K1_SCALAR_CONST(0x4e41311c, 0x0800af58, 0x7a690a8e, 0xe175c9ba, 0x6981ab73, 0xac532ea8, 0x5c1f5e63, 0x6ac1f189),
         SECP256K1_SCALAR_CONST(0xfffffff9, 0xd075982c, 0x7fbd3825, 0xc05038a2, 0x4533b91f, 0x94ec5f45, 0xb280b28f, 0x842324dc)},
        {SECP256K1_SCALAR_CONST(0x48e473bf, 0x3555eade, 0xad5d7089, 0x2424c4e4, 0x0a99397c, 0x2dc796d8, 0xb7a43a69, 0xd0364141),
         SECP256K1_SCALAR_CONST(0x634976b2, 0xa0e47895, 0x1ec38593, 0x266d6fd0, 0x6f602644, 0x9bb762f1, 0x7180c704, 0xe23a4daa)},
        {SECP256K1_SCALAR_CONST(0xbe83878d, 0x3292fc54, 0x26e71c62, 0x556ccedc, 0x7cbb8810, 0x4032a720, 0x34ead589, 0xe4d6bd13),
         SECP256K1_SCALAR_CONST(0x6cd150ad, 0x25e59d0f, 0x74cbae3d, 0x6377534a, 0x1e6562e8, 0xb71b9d18, 0xe1e5d712, 0x8480abb3)},
        {SECP256K1_SCALAR_CONST(0xcdddf2e5, 0xefc15f88, 0xc9ee06de, 0x8a846ca9, 0x28561581, 0x68daa5fb, 0xd1cf3451, 0xeb1782d0),
         SECP256K1_SCALAR_CONST(0xffffffd9, 0xed8d2af4, 0x993c865a, 0x23e9681a, 0x3ca3a3dc, 0xe6d5a46e, 0xbd86bd87, 0x61b55c70)},
        {SECP256K1_SCALAR_CONST(0xb6a18f1f, 0x04872df9, 0x08165ec4, 0x319ca19c, 0x6c0359ab, 0x1f7118fb, 0xc2ef8082, 0xca8b7785),
         SECP256K1_SCALAR_CONST(0xff55b19b, 0x0f1ac78c, 0x0f0c88c2, 0x2358d5ad, 0x5f455e4e, 0x3330b72f, 0x274dc153, 0xffbf272b)},
        {SECP256K1_SCALAR_CONST(0xea4898e5, 0x30eba3e8, 0xcf0e5c3d, 0x06ec6844, 0x01e26fb6, 0x75636225, 0xc5d08f4c, 0x1decafa0),
         SECP256K1_SCALAR_CONST(0xe5a014a8, 0xe3c4ec1e, 0xea4f9b32, 0xcfc7b386, 0x00630806, 0x12c08d02, 0x6407ccc2, 0xb067d90e)},
        {SECP256K1_SCALAR_CONST(0x70e9aea9, 0x7e933af0, 0x8a23bfab, 0x23e4b772, 0xff951863, 0x5ffcf47d, 0x6bebc918, 0x2ca58265),
         SECP256K1_SCALAR_CONST(0xf4e00006, 0x81bc6441, 0x4eb6ec02, 0xc194a859, 0x80ad7c48, 0xba4e9afb, 0x8b6bdbe0, 0x989d8f77)},
        {SECP256K1_SCALAR_CONST(0x3c56c774, 0x46efe6f0, 0xe93618b8, 0xf9b5a846, 0xd247df61, 0x83b1e215, 0x06dc8bcc, 0xeefc1bf5),
         SECP256K1_SCALAR_CONST(0xfff8937a, 0x2cd9586b, 0x43c25e57, 0xd1cefa7a, 0x9fb91ed3, 0x95b6533d, 0x8ad0de5b, 0xafb93f00)},
        {SECP256K1_SCALAR_CONST(0xfb5c2772, 0x5cb30e83, 0xe38264df, 0xe4e3ebf3, 0x392aa92e, 0xa68756a1, 0x51279ac5, 0xb50711a8),
         SECP256K1_SCALAR_CONST(0x000013af, 0x1105bfe7, 0xa6bbd7fb, 0x3d638f99, 0x3b266b02, 0x072fb8bc, 0x39251130, 0x2e0fd0ea)}
    };
    int i, var, testrand;
    unsigned char b32[32];
    secp256k1_fe x_fe;
    secp256k1_scalar x_scalar;
    memset(b32, 0, sizeof(b32));
    /* Test fixed test cases through test_inverse_{scalar,field}, both ways. */
    for (i = 0; (size_t)i < sizeof(fe_cases)/sizeof(fe_cases[0]); ++i) {
        for (var = 0; var <= 1; ++var) {
            test_inverse_field(&x_fe, &fe_cases[i][0], var);
            check_fe_equal(&x_fe, &fe_cases[i][1]);
            test_inverse_field(&x_fe, &fe_cases[i][1], var);
            check_fe_equal(&x_fe, &fe_cases[i][0]);
        }
    }
    for (i = 0; (size_t)i < sizeof(scalar_cases)/sizeof(scalar_cases[0]); ++i) {
        for (var = 0; var <= 1; ++var) {
            test_inverse_scalar(&x_scalar, &scalar_cases[i][0], var);
            CHECK(secp256k1_scalar_eq(&x_scalar, &scalar_cases[i][1]));
            test_inverse_scalar(&x_scalar, &scalar_cases[i][1], var);
            CHECK(secp256k1_scalar_eq(&x_scalar, &scalar_cases[i][0]));
        }
    }
    /* Test inputs 0..999 and their respective negations. */
    for (i = 0; i < 1000; ++i) {
        b32[31] = i & 0xff;
        b32[30] = (i >> 8) & 0xff;
        secp256k1_scalar_set_b32(&x_scalar, b32, NULL);
        secp256k1_fe_set_b32(&x_fe, b32);
        for (var = 0; var <= 1; ++var) {
            test_inverse_scalar(NULL, &x_scalar, var);
            test_inverse_field(NULL, &x_fe, var);
        }
        secp256k1_scalar_negate(&x_scalar, &x_scalar);
        secp256k1_fe_negate(&x_fe, &x_fe, 1);
        for (var = 0; var <= 1; ++var) {
            test_inverse_scalar(NULL, &x_scalar, var);
            test_inverse_field(NULL, &x_fe, var);
        }
    }
    /* test 128*count random inputs; half with testrand256_test, half with testrand256 */
    for (testrand = 0; testrand <= 1; ++testrand) {
        for (i = 0; i < 64 * count; ++i) {
            (testrand ? secp256k1_testrand256_test : secp256k1_testrand256)(b32);
            secp256k1_scalar_set_b32(&x_scalar, b32, NULL);
            secp256k1_fe_set_b32(&x_fe, b32);
            for (var = 0; var <= 1; ++var) {
                test_inverse_scalar(NULL, &x_scalar, var);
                test_inverse_field(NULL, &x_fe, var);
            }
        }
    }
}

/***** GROUP TESTS *****/

void ge_equals_ge(const secp256k1_ge *a, const secp256k1_ge *b) {
    CHECK(a->infinity == b->infinity);
    if (a->infinity) {
        return;
    }
    CHECK(secp256k1_fe_equal_var(&a->x, &b->x));
    CHECK(secp256k1_fe_equal_var(&a->y, &b->y));
}

/* This compares jacobian points including their Z, not just their geometric meaning. */
int gej_xyz_equals_gej(const secp256k1_gej *a, const secp256k1_gej *b) {
    secp256k1_gej a2;
    secp256k1_gej b2;
    int ret = 1;
    ret &= a->infinity == b->infinity;
    if (ret && !a->infinity) {
        a2 = *a;
        b2 = *b;
        secp256k1_fe_normalize(&a2.x);
        secp256k1_fe_normalize(&a2.y);
        secp256k1_fe_normalize(&a2.z);
        secp256k1_fe_normalize(&b2.x);
        secp256k1_fe_normalize(&b2.y);
        secp256k1_fe_normalize(&b2.z);
        ret &= secp256k1_fe_cmp_var(&a2.x, &b2.x) == 0;
        ret &= secp256k1_fe_cmp_var(&a2.y, &b2.y) == 0;
        ret &= secp256k1_fe_cmp_var(&a2.z, &b2.z) == 0;
    }
    return ret;
}

void ge_equals_gej(const secp256k1_ge *a, const secp256k1_gej *b) {
    secp256k1_fe z2s;
    secp256k1_fe u1, u2, s1, s2;
    CHECK(a->infinity == b->infinity);
    if (a->infinity) {
        return;
    }
    /* Check a.x * b.z^2 == b.x && a.y * b.z^3 == b.y, to avoid inverses. */
    secp256k1_fe_sqr(&z2s, &b->z);
    secp256k1_fe_mul(&u1, &a->x, &z2s);
    u2 = b->x; secp256k1_fe_normalize_weak(&u2);
    secp256k1_fe_mul(&s1, &a->y, &z2s); secp256k1_fe_mul(&s1, &s1, &b->z);
    s2 = b->y; secp256k1_fe_normalize_weak(&s2);
    CHECK(secp256k1_fe_equal_var(&u1, &u2));
    CHECK(secp256k1_fe_equal_var(&s1, &s2));
}

void test_ge(void) {
    int i, i1;
    int runs = 6;
    /* 25 points are used:
     * - infinity
     * - for each of four random points p1 p2 p3 p4, we add the point, its
     *   negation, and then those two again but with randomized Z coordinate.
     * - The same is then done for lambda*p1 and lambda^2*p1.
     */
    secp256k1_ge *ge = (secp256k1_ge *)checked_malloc(&ctx->error_callback, sizeof(secp256k1_ge) * (1 + 4 * runs));
    secp256k1_gej *gej = (secp256k1_gej *)checked_malloc(&ctx->error_callback, sizeof(secp256k1_gej) * (1 + 4 * runs));
    secp256k1_fe zf;
    secp256k1_fe zfi2, zfi3;

    secp256k1_gej_set_infinity(&gej[0]);
    secp256k1_ge_clear(&ge[0]);
    secp256k1_ge_set_gej_var(&ge[0], &gej[0]);
    for (i = 0; i < runs; i++) {
        int j;
        secp256k1_ge g;
        random_group_element_test(&g);
        if (i >= runs - 2) {
            secp256k1_ge_mul_lambda(&g, &ge[1]);
        }
        if (i >= runs - 1) {
            secp256k1_ge_mul_lambda(&g, &g);
        }
        ge[1 + 4 * i] = g;
        ge[2 + 4 * i] = g;
        secp256k1_ge_neg(&ge[3 + 4 * i], &g);
        secp256k1_ge_neg(&ge[4 + 4 * i], &g);
        secp256k1_gej_set_ge(&gej[1 + 4 * i], &ge[1 + 4 * i]);
        random_group_element_jacobian_test(&gej[2 + 4 * i], &ge[2 + 4 * i]);
        secp256k1_gej_set_ge(&gej[3 + 4 * i], &ge[3 + 4 * i]);
        random_group_element_jacobian_test(&gej[4 + 4 * i], &ge[4 + 4 * i]);
        for (j = 0; j < 4; j++) {
            random_field_element_magnitude(&ge[1 + j + 4 * i].x);
            random_field_element_magnitude(&ge[1 + j + 4 * i].y);
            random_field_element_magnitude(&gej[1 + j + 4 * i].x);
            random_field_element_magnitude(&gej[1 + j + 4 * i].y);
            random_field_element_magnitude(&gej[1 + j + 4 * i].z);
        }
    }

    /* Generate random zf, and zfi2 = 1/zf^2, zfi3 = 1/zf^3 */
    do {
        random_field_element_test(&zf);
    } while(secp256k1_fe_is_zero(&zf));
    random_field_element_magnitude(&zf);
    secp256k1_fe_inv_var(&zfi3, &zf);
    secp256k1_fe_sqr(&zfi2, &zfi3);
    secp256k1_fe_mul(&zfi3, &zfi3, &zfi2);

    for (i1 = 0; i1 < 1 + 4 * runs; i1++) {
        int i2;
        for (i2 = 0; i2 < 1 + 4 * runs; i2++) {
            /* Compute reference result using gej + gej (var). */
            secp256k1_gej refj, resj;
            secp256k1_ge ref;
            secp256k1_fe zr;
            secp256k1_gej_add_var(&refj, &gej[i1], &gej[i2], secp256k1_gej_is_infinity(&gej[i1]) ? NULL : &zr);
            /* Check Z ratio. */
            if (!secp256k1_gej_is_infinity(&gej[i1]) && !secp256k1_gej_is_infinity(&refj)) {
                secp256k1_fe zrz; secp256k1_fe_mul(&zrz, &zr, &gej[i1].z);
                CHECK(secp256k1_fe_equal_var(&zrz, &refj.z));
            }
            secp256k1_ge_set_gej_var(&ref, &refj);

            /* Test gej + ge with Z ratio result (var). */
            secp256k1_gej_add_ge_var(&resj, &gej[i1], &ge[i2], secp256k1_gej_is_infinity(&gej[i1]) ? NULL : &zr);
            ge_equals_gej(&ref, &resj);
            if (!secp256k1_gej_is_infinity(&gej[i1]) && !secp256k1_gej_is_infinity(&resj)) {
                secp256k1_fe zrz; secp256k1_fe_mul(&zrz, &zr, &gej[i1].z);
                CHECK(secp256k1_fe_equal_var(&zrz, &resj.z));
            }

            /* Test gej + ge (var, with additional Z factor). */
            {
                secp256k1_ge ge2_zfi = ge[i2]; /* the second term with x and y rescaled for z = 1/zf */
                secp256k1_fe_mul(&ge2_zfi.x, &ge2_zfi.x, &zfi2);
                secp256k1_fe_mul(&ge2_zfi.y, &ge2_zfi.y, &zfi3);
                random_field_element_magnitude(&ge2_zfi.x);
                random_field_element_magnitude(&ge2_zfi.y);
                secp256k1_gej_add_zinv_var(&resj, &gej[i1], &ge2_zfi, &zf);
                ge_equals_gej(&ref, &resj);
            }

            /* Test gej + ge (const). */
            if (i2 != 0) {
                /* secp256k1_gej_add_ge does not support its second argument being infinity. */
                secp256k1_gej_add_ge(&resj, &gej[i1], &ge[i2]);
                ge_equals_gej(&ref, &resj);
            }

            /* Test doubling (var). */
            if ((i1 == 0 && i2 == 0) || ((i1 + 3)/4 == (i2 + 3)/4 && ((i1 + 3)%4)/2 == ((i2 + 3)%4)/2)) {
                secp256k1_fe zr2;
                /* Normal doubling with Z ratio result. */
                secp256k1_gej_double_var(&resj, &gej[i1], &zr2);
                ge_equals_gej(&ref, &resj);
                /* Check Z ratio. */
                secp256k1_fe_mul(&zr2, &zr2, &gej[i1].z);
                CHECK(secp256k1_fe_equal_var(&zr2, &resj.z));
                /* Normal doubling. */
                secp256k1_gej_double_var(&resj, &gej[i2], NULL);
                ge_equals_gej(&ref, &resj);
                /* Constant-time doubling. */
                secp256k1_gej_double(&resj, &gej[i2]);
                ge_equals_gej(&ref, &resj);
            }

            /* Test adding opposites. */
            if ((i1 == 0 && i2 == 0) || ((i1 + 3)/4 == (i2 + 3)/4 && ((i1 + 3)%4)/2 != ((i2 + 3)%4)/2)) {
                CHECK(secp256k1_ge_is_infinity(&ref));
            }

            /* Test adding infinity. */
            if (i1 == 0) {
                CHECK(secp256k1_ge_is_infinity(&ge[i1]));
                CHECK(secp256k1_gej_is_infinity(&gej[i1]));
                ge_equals_gej(&ref, &gej[i2]);
            }
            if (i2 == 0) {
                CHECK(secp256k1_ge_is_infinity(&ge[i2]));
                CHECK(secp256k1_gej_is_infinity(&gej[i2]));
                ge_equals_gej(&ref, &gej[i1]);
            }
        }
    }

    /* Test adding all points together in random order equals infinity. */
    {
        secp256k1_gej sum = SECP256K1_GEJ_CONST_INFINITY;
        secp256k1_gej *gej_shuffled = (secp256k1_gej *)checked_malloc(&ctx->error_callback, (4 * runs + 1) * sizeof(secp256k1_gej));
        for (i = 0; i < 4 * runs + 1; i++) {
            gej_shuffled[i] = gej[i];
        }
        for (i = 0; i < 4 * runs + 1; i++) {
            int swap = i + secp256k1_testrand_int(4 * runs + 1 - i);
            if (swap != i) {
                secp256k1_gej t = gej_shuffled[i];
                gej_shuffled[i] = gej_shuffled[swap];
                gej_shuffled[swap] = t;
            }
        }
        for (i = 0; i < 4 * runs + 1; i++) {
            secp256k1_gej_add_var(&sum, &sum, &gej_shuffled[i], NULL);
        }
        CHECK(secp256k1_gej_is_infinity(&sum));
        free(gej_shuffled);
    }

    /* Test batch gej -> ge conversion without known z ratios. */
    {
        secp256k1_ge *ge_set_all = (secp256k1_ge *)checked_malloc(&ctx->error_callback, (4 * runs + 1) * sizeof(secp256k1_ge));
        secp256k1_ge_set_all_gej_var(ge_set_all, gej, 4 * runs + 1);
        for (i = 0; i < 4 * runs + 1; i++) {
            secp256k1_fe s;
            random_fe_non_zero(&s);
            secp256k1_gej_rescale(&gej[i], &s);
            ge_equals_gej(&ge_set_all[i], &gej[i]);
        }
        free(ge_set_all);
    }

    /* Test batch gej -> ge conversion with many infinities. */
    for (i = 0; i < 4 * runs + 1; i++) {
        int odd;
        random_group_element_test(&ge[i]);
        odd = secp256k1_fe_is_odd(&ge[i].x);
        CHECK(odd == 0 || odd == 1);
        /* randomly set half the points to infinity */
        if (odd == i % 2) {
            secp256k1_ge_set_infinity(&ge[i]);
        }
        secp256k1_gej_set_ge(&gej[i], &ge[i]);
    }
    /* batch convert */
    secp256k1_ge_set_all_gej_var(ge, gej, 4 * runs + 1);
    /* check result */
    for (i = 0; i < 4 * runs + 1; i++) {
        ge_equals_gej(&ge[i], &gej[i]);
    }

    /* Test batch gej -> ge conversion with all infinities. */
    for (i = 0; i < 4 * runs + 1; i++) {
        secp256k1_gej_set_infinity(&gej[i]);
    }
    /* batch convert */
    secp256k1_ge_set_all_gej_var(ge, gej, 4 * runs + 1);
    /* check result */
    for (i = 0; i < 4 * runs + 1; i++) {
        CHECK(secp256k1_ge_is_infinity(&ge[i]));
    }

    free(ge);
    free(gej);
}


void test_intialized_inf(void) {
    secp256k1_ge p;
    secp256k1_gej pj, npj, infj1, infj2, infj3;
    secp256k1_fe zinv;

    /* Test that adding P+(-P) results in a fully initialized infinity*/
    random_group_element_test(&p);
    secp256k1_gej_set_ge(&pj, &p);
    secp256k1_gej_neg(&npj, &pj);

    secp256k1_gej_add_var(&infj1, &pj, &npj, NULL);
    CHECK(secp256k1_gej_is_infinity(&infj1));
    CHECK(secp256k1_fe_is_zero(&infj1.x));
    CHECK(secp256k1_fe_is_zero(&infj1.y));
    CHECK(secp256k1_fe_is_zero(&infj1.z));

    secp256k1_gej_add_ge_var(&infj2, &npj, &p, NULL);
    CHECK(secp256k1_gej_is_infinity(&infj2));
    CHECK(secp256k1_fe_is_zero(&infj2.x));
    CHECK(secp256k1_fe_is_zero(&infj2.y));
    CHECK(secp256k1_fe_is_zero(&infj2.z));

    secp256k1_fe_set_int(&zinv, 1);
    secp256k1_gej_add_zinv_var(&infj3, &npj, &p, &zinv);
    CHECK(secp256k1_gej_is_infinity(&infj3));
    CHECK(secp256k1_fe_is_zero(&infj3.x));
    CHECK(secp256k1_fe_is_zero(&infj3.y));
    CHECK(secp256k1_fe_is_zero(&infj3.z));


}

void test_add_neg_y_diff_x(void) {
    /* The point of this test is to check that we can add two points
     * whose y-coordinates are negatives of each other but whose x
     * coordinates differ. If the x-coordinates were the same, these
     * points would be negatives of each other and their sum is
     * infinity. This is cool because it "covers up" any degeneracy
     * in the addition algorithm that would cause the xy coordinates
     * of the sum to be wrong (since infinity has no xy coordinates).
     * HOWEVER, if the x-coordinates are different, infinity is the
     * wrong answer, and such degeneracies are exposed. This is the
     * root of https://github.com/bitcoin-core/secp256k1/issues/257
     * which this test is a regression test for.
     *
     * These points were generated in sage as
     * # secp256k1 params
     * F = FiniteField (0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F)
     * C = EllipticCurve ([F (0), F (7)])
     * G = C.lift_x(0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798)
     * N = FiniteField(G.order())
     *
     * # endomorphism values (lambda is 1^{1/3} in N, beta is 1^{1/3} in F)
     * x = polygen(N)
     * lam  = (1 - x^3).roots()[1][0]
     *
     * # random "bad pair"
     * P = C.random_element()
     * Q = -int(lam) * P
     * print "    P: %x %x" % P.xy()
     * print "    Q: %x %x" % Q.xy()
     * print "P + Q: %x %x" % (P + Q).xy()
     */
    secp256k1_gej aj = SECP256K1_GEJ_CONST(
        0x8d24cd95, 0x0a355af1, 0x3c543505, 0x44238d30,
        0x0643d79f, 0x05a59614, 0x2f8ec030, 0xd58977cb,
        0x001e337a, 0x38093dcd, 0x6c0f386d, 0x0b1293a8,
        0x4d72c879, 0xd7681924, 0x44e6d2f3, 0x9190117d
    );
    secp256k1_gej bj = SECP256K1_GEJ_CONST(
        0xc7b74206, 0x1f788cd9, 0xabd0937d, 0x164a0d86,
        0x95f6ff75, 0xf19a4ce9, 0xd013bd7b, 0xbf92d2a7,
        0xffe1cc85, 0xc7f6c232, 0x93f0c792, 0xf4ed6c57,
        0xb28d3786, 0x2897e6db, 0xbb192d0b, 0x6e6feab2
    );
    secp256k1_gej sumj = SECP256K1_GEJ_CONST(
        0x671a63c0, 0x3efdad4c, 0x389a7798, 0x24356027,
        0xb3d69010, 0x278625c3, 0x5c86d390, 0x184a8f7a,
        0x5f6409c2, 0x2ce01f2b, 0x511fd375, 0x25071d08,
        0xda651801, 0x70e95caf, 0x8f0d893c, 0xbed8fbbe
    );
    secp256k1_ge b;
    secp256k1_gej resj;
    secp256k1_ge res;
    secp256k1_ge_set_gej(&b, &bj);

    secp256k1_gej_add_var(&resj, &aj, &bj, NULL);
    secp256k1_ge_set_gej(&res, &resj);
    ge_equals_gej(&res, &sumj);

    secp256k1_gej_add_ge(&resj, &aj, &b);
    secp256k1_ge_set_gej(&res, &resj);
    ge_equals_gej(&res, &sumj);

    secp256k1_gej_add_ge_var(&resj, &aj, &b, NULL);
    secp256k1_ge_set_gej(&res, &resj);
    ge_equals_gej(&res, &sumj);
}

void run_ge(void) {
    int i;
    for (i = 0; i < count * 32; i++) {
        test_ge();
    }
    test_add_neg_y_diff_x();
    test_intialized_inf();
}

void test_ec_combine(void) {
    secp256k1_scalar sum = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0);
    secp256k1_pubkey data[6];
    const secp256k1_pubkey* d[6];
    secp256k1_pubkey sd;
    secp256k1_pubkey sd2;
    secp256k1_gej Qj;
    secp256k1_ge Q;
    int i;
    for (i = 1; i <= 6; i++) {
        secp256k1_scalar s;
        random_scalar_order_test(&s);
        secp256k1_scalar_add(&sum, &sum, &s);
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &Qj, &s);
        secp256k1_ge_set_gej(&Q, &Qj);
        secp256k1_pubkey_save(&data[i - 1], &Q);
        d[i - 1] = &data[i - 1];
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &Qj, &sum);
        secp256k1_ge_set_gej(&Q, &Qj);
        secp256k1_pubkey_save(&sd, &Q);
        CHECK(secp256k1_ec_pubkey_combine(ctx, &sd2, d, i) == 1);
        CHECK(secp256k1_memcmp_var(&sd, &sd2, sizeof(sd)) == 0);
    }
}

void run_ec_combine(void) {
    int i;
    for (i = 0; i < count * 8; i++) {
         test_ec_combine();
    }
}

void test_group_decompress(const secp256k1_fe* x) {
    /* The input itself, normalized. */
    secp256k1_fe fex = *x;
    /* Results of set_xo_var(..., 0), set_xo_var(..., 1). */
    secp256k1_ge ge_even, ge_odd;
    /* Return values of the above calls. */
    int res_even, res_odd;

    secp256k1_fe_normalize_var(&fex);

    res_even = secp256k1_ge_set_xo_var(&ge_even, &fex, 0);
    res_odd = secp256k1_ge_set_xo_var(&ge_odd, &fex, 1);

    CHECK(res_even == res_odd);

    if (res_even) {
        secp256k1_fe_normalize_var(&ge_odd.x);
        secp256k1_fe_normalize_var(&ge_even.x);
        secp256k1_fe_normalize_var(&ge_odd.y);
        secp256k1_fe_normalize_var(&ge_even.y);

        /* No infinity allowed. */
        CHECK(!ge_even.infinity);
        CHECK(!ge_odd.infinity);

        /* Check that the x coordinates check out. */
        CHECK(secp256k1_fe_equal_var(&ge_even.x, x));
        CHECK(secp256k1_fe_equal_var(&ge_odd.x, x));

        /* Check odd/even Y in ge_odd, ge_even. */
        CHECK(secp256k1_fe_is_odd(&ge_odd.y));
        CHECK(!secp256k1_fe_is_odd(&ge_even.y));
    }
}

void run_group_decompress(void) {
    int i;
    for (i = 0; i < count * 4; i++) {
        secp256k1_fe fe;
        random_fe_test(&fe);
        test_group_decompress(&fe);
    }
}

/***** ECMULT TESTS *****/

void run_ecmult_chain(void) {
    /* random starting point A (on the curve) */
    secp256k1_gej a = SECP256K1_GEJ_CONST(
        0x8b30bbe9, 0xae2a9906, 0x96b22f67, 0x0709dff3,
        0x727fd8bc, 0x04d3362c, 0x6c7bf458, 0xe2846004,
        0xa357ae91, 0x5c4a6528, 0x1309edf2, 0x0504740f,
        0x0eb33439, 0x90216b4f, 0x81063cb6, 0x5f2f7e0f
    );
    /* two random initial factors xn and gn */
    secp256k1_scalar xn = SECP256K1_SCALAR_CONST(
        0x84cc5452, 0xf7fde1ed, 0xb4d38a8c, 0xe9b1b84c,
        0xcef31f14, 0x6e569be9, 0x705d357a, 0x42985407
    );
    secp256k1_scalar gn = SECP256K1_SCALAR_CONST(
        0xa1e58d22, 0x553dcd42, 0xb2398062, 0x5d4c57a9,
        0x6e9323d4, 0x2b3152e5, 0xca2c3990, 0xedc7c9de
    );
    /* two small multipliers to be applied to xn and gn in every iteration: */
    static const secp256k1_scalar xf = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0x1337);
    static const secp256k1_scalar gf = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0x7113);
    /* accumulators with the resulting coefficients to A and G */
    secp256k1_scalar ae = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 1);
    secp256k1_scalar ge = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0);
    /* actual points */
    secp256k1_gej x;
    secp256k1_gej x2;
    int i;

    /* the point being computed */
    x = a;
    for (i = 0; i < 200*count; i++) {
        /* in each iteration, compute X = xn*X + gn*G; */
        secp256k1_ecmult(&ctx->ecmult_ctx, &x, &x, &xn, &gn);
        /* also compute ae and ge: the actual accumulated factors for A and G */
        /* if X was (ae*A+ge*G), xn*X + gn*G results in (xn*ae*A + (xn*ge+gn)*G) */
        secp256k1_scalar_mul(&ae, &ae, &xn);
        secp256k1_scalar_mul(&ge, &ge, &xn);
        secp256k1_scalar_add(&ge, &ge, &gn);
        /* modify xn and gn */
        secp256k1_scalar_mul(&xn, &xn, &xf);
        secp256k1_scalar_mul(&gn, &gn, &gf);

        /* verify */
        if (i == 19999) {
            /* expected result after 19999 iterations */
            secp256k1_gej rp = SECP256K1_GEJ_CONST(
                0xD6E96687, 0xF9B10D09, 0x2A6F3543, 0x9D86CEBE,
                0xA4535D0D, 0x409F5358, 0x6440BD74, 0xB933E830,
                0xB95CBCA2, 0xC77DA786, 0x539BE8FD, 0x53354D2D,
                0x3B4F566A, 0xE6580454, 0x07ED6015, 0xEE1B2A88
            );

            secp256k1_gej_neg(&rp, &rp);
            secp256k1_gej_add_var(&rp, &rp, &x, NULL);
            CHECK(secp256k1_gej_is_infinity(&rp));
        }
    }
    /* redo the computation, but directly with the resulting ae and ge coefficients: */
    secp256k1_ecmult(&ctx->ecmult_ctx, &x2, &a, &ae, &ge);
    secp256k1_gej_neg(&x2, &x2);
    secp256k1_gej_add_var(&x2, &x2, &x, NULL);
    CHECK(secp256k1_gej_is_infinity(&x2));
}

void test_point_times_order(const secp256k1_gej *point) {
    /* X * (point + G) + (order-X) * (pointer + G) = 0 */
    secp256k1_scalar x;
    secp256k1_scalar nx;
    secp256k1_scalar zero = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0);
    secp256k1_scalar one = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 1);
    secp256k1_gej res1, res2;
    secp256k1_ge res3;
    unsigned char pub[65];
    size_t psize = 65;
    random_scalar_order_test(&x);
    secp256k1_scalar_negate(&nx, &x);
    secp256k1_ecmult(&ctx->ecmult_ctx, &res1, point, &x, &x); /* calc res1 = x * point + x * G; */
    secp256k1_ecmult(&ctx->ecmult_ctx, &res2, point, &nx, &nx); /* calc res2 = (order - x) * point + (order - x) * G; */
    secp256k1_gej_add_var(&res1, &res1, &res2, NULL);
    CHECK(secp256k1_gej_is_infinity(&res1));
    secp256k1_ge_set_gej(&res3, &res1);
    CHECK(secp256k1_ge_is_infinity(&res3));
    CHECK(secp256k1_ge_is_valid_var(&res3) == 0);
    CHECK(secp256k1_eckey_pubkey_serialize(&res3, pub, &psize, 0) == 0);
    psize = 65;
    CHECK(secp256k1_eckey_pubkey_serialize(&res3, pub, &psize, 1) == 0);
    /* check zero/one edge cases */
    secp256k1_ecmult(&ctx->ecmult_ctx, &res1, point, &zero, &zero);
    secp256k1_ge_set_gej(&res3, &res1);
    CHECK(secp256k1_ge_is_infinity(&res3));
    secp256k1_ecmult(&ctx->ecmult_ctx, &res1, point, &one, &zero);
    secp256k1_ge_set_gej(&res3, &res1);
    ge_equals_gej(&res3, point);
    secp256k1_ecmult(&ctx->ecmult_ctx, &res1, point, &zero, &one);
    secp256k1_ge_set_gej(&res3, &res1);
    ge_equals_ge(&res3, &secp256k1_ge_const_g);
}

/* These scalars reach large (in absolute value) outputs when fed to secp256k1_scalar_split_lambda.
 *
 * They are computed as:
 * - For a in [-2, -1, 0, 1, 2]:
 *   - For b in [-3, -1, 1, 3]:
 *     - Output (a*LAMBDA + (ORDER+b)/2) % ORDER
 */
static const secp256k1_scalar scalars_near_split_bounds[20] = {
    SECP256K1_SCALAR_CONST(0xd938a566, 0x7f479e3e, 0xb5b3c7fa, 0xefdb3749, 0x3aa0585c, 0xc5ea2367, 0xe1b660db, 0x0209e6fc),
    SECP256K1_SCALAR_CONST(0xd938a566, 0x7f479e3e, 0xb5b3c7fa, 0xefdb3749, 0x3aa0585c, 0xc5ea2367, 0xe1b660db, 0x0209e6fd),
    SECP256K1_SCALAR_CONST(0xd938a566, 0x7f479e3e, 0xb5b3c7fa, 0xefdb3749, 0x3aa0585c, 0xc5ea2367, 0xe1b660db, 0x0209e6fe),
    SECP256K1_SCALAR_CONST(0xd938a566, 0x7f479e3e, 0xb5b3c7fa, 0xefdb3749, 0x3aa0585c, 0xc5ea2367, 0xe1b660db, 0x0209e6ff),
    SECP256K1_SCALAR_CONST(0x2c9c52b3, 0x3fa3cf1f, 0x5ad9e3fd, 0x77ed9ba5, 0xb294b893, 0x3722e9a5, 0x00e698ca, 0x4cf7632d),
    SECP256K1_SCALAR_CONST(0x2c9c52b3, 0x3fa3cf1f, 0x5ad9e3fd, 0x77ed9ba5, 0xb294b893, 0x3722e9a5, 0x00e698ca, 0x4cf7632e),
    SECP256K1_SCALAR_CONST(0x2c9c52b3, 0x3fa3cf1f, 0x5ad9e3fd, 0x77ed9ba5, 0xb294b893, 0x3722e9a5, 0x00e698ca, 0x4cf7632f),
    SECP256K1_SCALAR_CONST(0x2c9c52b3, 0x3fa3cf1f, 0x5ad9e3fd, 0x77ed9ba5, 0xb294b893, 0x3722e9a5, 0x00e698ca, 0x4cf76330),
    SECP256K1_SCALAR_CONST(0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xd576e735, 0x57a4501d, 0xdfe92f46, 0x681b209f),
    SECP256K1_SCALAR_CONST(0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xd576e735, 0x57a4501d, 0xdfe92f46, 0x681b20a0),
    SECP256K1_SCALAR_CONST(0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xd576e735, 0x57a4501d, 0xdfe92f46, 0x681b20a1),
    SECP256K1_SCALAR_CONST(0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xd576e735, 0x57a4501d, 0xdfe92f46, 0x681b20a2),
    SECP256K1_SCALAR_CONST(0xd363ad4c, 0xc05c30e0, 0xa5261c02, 0x88126459, 0xf85915d7, 0x7825b696, 0xbeebc5c2, 0x833ede11),
    SECP256K1_SCALAR_CONST(0xd363ad4c, 0xc05c30e0, 0xa5261c02, 0x88126459, 0xf85915d7, 0x7825b696, 0xbeebc5c2, 0x833ede12),
    SECP256K1_SCALAR_CONST(0xd363ad4c, 0xc05c30e0, 0xa5261c02, 0x88126459, 0xf85915d7, 0x7825b696, 0xbeebc5c2, 0x833ede13),
    SECP256K1_SCALAR_CONST(0xd363ad4c, 0xc05c30e0, 0xa5261c02, 0x88126459, 0xf85915d7, 0x7825b696, 0xbeebc5c2, 0x833ede14),
    SECP256K1_SCALAR_CONST(0x26c75a99, 0x80b861c1, 0x4a4c3805, 0x1024c8b4, 0x704d760e, 0xe95e7cd3, 0xde1bfdb1, 0xce2c5a42),
    SECP256K1_SCALAR_CONST(0x26c75a99, 0x80b861c1, 0x4a4c3805, 0x1024c8b4, 0x704d760e, 0xe95e7cd3, 0xde1bfdb1, 0xce2c5a43),
    SECP256K1_SCALAR_CONST(0x26c75a99, 0x80b861c1, 0x4a4c3805, 0x1024c8b4, 0x704d760e, 0xe95e7cd3, 0xde1bfdb1, 0xce2c5a44),
    SECP256K1_SCALAR_CONST(0x26c75a99, 0x80b861c1, 0x4a4c3805, 0x1024c8b4, 0x704d760e, 0xe95e7cd3, 0xde1bfdb1, 0xce2c5a45)
};

void test_ecmult_target(const secp256k1_scalar* target, int mode) {
    /* Mode: 0=ecmult_gen, 1=ecmult, 2=ecmult_const */
    secp256k1_scalar n1, n2;
    secp256k1_ge p;
    secp256k1_gej pj, p1j, p2j, ptj;
    static const secp256k1_scalar zero = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0);

    /* Generate random n1,n2 such that n1+n2 = -target. */
    random_scalar_order_test(&n1);
    secp256k1_scalar_add(&n2, &n1, target);
    secp256k1_scalar_negate(&n2, &n2);

    /* Generate a random input point. */
    if (mode != 0) {
        random_group_element_test(&p);
        secp256k1_gej_set_ge(&pj, &p);
    }

    /* EC multiplications */
    if (mode == 0) {
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &p1j, &n1);
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &p2j, &n2);
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &ptj, target);
    } else if (mode == 1) {
        secp256k1_ecmult(&ctx->ecmult_ctx, &p1j, &pj, &n1, &zero);
        secp256k1_ecmult(&ctx->ecmult_ctx, &p2j, &pj, &n2, &zero);
        secp256k1_ecmult(&ctx->ecmult_ctx, &ptj, &pj, target, &zero);
    } else {
        secp256k1_ecmult_const(&p1j, &p, &n1, 256);
        secp256k1_ecmult_const(&p2j, &p, &n2, 256);
        secp256k1_ecmult_const(&ptj, &p, target, 256);
    }

    /* Add them all up: n1*P + n2*P + target*P = (n1+n2+target)*P = (n1+n1-n1-n2)*P = 0. */
    secp256k1_gej_add_var(&ptj, &ptj, &p1j, NULL);
    secp256k1_gej_add_var(&ptj, &ptj, &p2j, NULL);
    CHECK(secp256k1_gej_is_infinity(&ptj));
}

void run_ecmult_near_split_bound(void) {
    int i;
    unsigned j;
    for (i = 0; i < 4*count; ++i) {
        for (j = 0; j < sizeof(scalars_near_split_bounds) / sizeof(scalars_near_split_bounds[0]); ++j) {
            test_ecmult_target(&scalars_near_split_bounds[j], 0);
            test_ecmult_target(&scalars_near_split_bounds[j], 1);
            test_ecmult_target(&scalars_near_split_bounds[j], 2);
        }
    }
}

void run_point_times_order(void) {
    int i;
    secp256k1_fe x = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 2);
    static const secp256k1_fe xr = SECP256K1_FE_CONST(
        0x7603CB59, 0xB0EF6C63, 0xFE608479, 0x2A0C378C,
        0xDB3233A8, 0x0F8A9A09, 0xA877DEAD, 0x31B38C45
    );
    for (i = 0; i < 500; i++) {
        secp256k1_ge p;
        if (secp256k1_ge_set_xo_var(&p, &x, 1)) {
            secp256k1_gej j;
            CHECK(secp256k1_ge_is_valid_var(&p));
            secp256k1_gej_set_ge(&j, &p);
            test_point_times_order(&j);
        }
        secp256k1_fe_sqr(&x, &x);
    }
    secp256k1_fe_normalize_var(&x);
    CHECK(secp256k1_fe_equal_var(&x, &xr));
}

void ecmult_const_random_mult(void) {
    /* random starting point A (on the curve) */
    secp256k1_ge a = SECP256K1_GE_CONST(
        0x6d986544, 0x57ff52b8, 0xcf1b8126, 0x5b802a5b,
        0xa97f9263, 0xb1e88044, 0x93351325, 0x91bc450a,
        0x535c59f7, 0x325e5d2b, 0xc391fbe8, 0x3c12787c,
        0x337e4a98, 0xe82a9011, 0x0123ba37, 0xdd769c7d
    );
    /* random initial factor xn */
    secp256k1_scalar xn = SECP256K1_SCALAR_CONST(
        0x649d4f77, 0xc4242df7, 0x7f2079c9, 0x14530327,
        0xa31b876a, 0xd2d8ce2a, 0x2236d5c6, 0xd7b2029b
    );
    /* expected xn * A (from sage) */
    secp256k1_ge expected_b = SECP256K1_GE_CONST(
        0x23773684, 0x4d209dc7, 0x098a786f, 0x20d06fcd,
        0x070a38bf, 0xc11ac651, 0x03004319, 0x1e2a8786,
        0xed8c3b8e, 0xc06dd57b, 0xd06ea66e, 0x45492b0f,
        0xb84e4e1b, 0xfb77e21f, 0x96baae2a, 0x63dec956
    );
    secp256k1_gej b;
    secp256k1_ecmult_const(&b, &a, &xn, 256);

    CHECK(secp256k1_ge_is_valid_var(&a));
    ge_equals_gej(&expected_b, &b);
}

void ecmult_const_commutativity(void) {
    secp256k1_scalar a;
    secp256k1_scalar b;
    secp256k1_gej res1;
    secp256k1_gej res2;
    secp256k1_ge mid1;
    secp256k1_ge mid2;
    random_scalar_order_test(&a);
    random_scalar_order_test(&b);

    secp256k1_ecmult_const(&res1, &secp256k1_ge_const_g, &a, 256);
    secp256k1_ecmult_const(&res2, &secp256k1_ge_const_g, &b, 256);
    secp256k1_ge_set_gej(&mid1, &res1);
    secp256k1_ge_set_gej(&mid2, &res2);
    secp256k1_ecmult_const(&res1, &mid1, &b, 256);
    secp256k1_ecmult_const(&res2, &mid2, &a, 256);
    secp256k1_ge_set_gej(&mid1, &res1);
    secp256k1_ge_set_gej(&mid2, &res2);
    ge_equals_ge(&mid1, &mid2);
}

void ecmult_const_mult_zero_one(void) {
    secp256k1_scalar zero = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0);
    secp256k1_scalar one = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 1);
    secp256k1_scalar negone;
    secp256k1_gej res1;
    secp256k1_ge res2;
    secp256k1_ge point;
    secp256k1_scalar_negate(&negone, &one);

    random_group_element_test(&point);
    secp256k1_ecmult_const(&res1, &point, &zero, 3);
    secp256k1_ge_set_gej(&res2, &res1);
    CHECK(secp256k1_ge_is_infinity(&res2));
    secp256k1_ecmult_const(&res1, &point, &one, 2);
    secp256k1_ge_set_gej(&res2, &res1);
    ge_equals_ge(&res2, &point);
    secp256k1_ecmult_const(&res1, &point, &negone, 256);
    secp256k1_gej_neg(&res1, &res1);
    secp256k1_ge_set_gej(&res2, &res1);
    ge_equals_ge(&res2, &point);
}

void ecmult_const_chain_multiply(void) {
    /* Check known result (randomly generated test problem from sage) */
    const secp256k1_scalar scalar = SECP256K1_SCALAR_CONST(
        0x4968d524, 0x2abf9b7a, 0x466abbcf, 0x34b11b6d,
        0xcd83d307, 0x827bed62, 0x05fad0ce, 0x18fae63b
    );
    const secp256k1_gej expected_point = SECP256K1_GEJ_CONST(
        0x5494c15d, 0x32099706, 0xc2395f94, 0x348745fd,
        0x757ce30e, 0x4e8c90fb, 0xa2bad184, 0xf883c69f,
        0x5d195d20, 0xe191bf7f, 0x1be3e55f, 0x56a80196,
        0x6071ad01, 0xf1462f66, 0xc997fa94, 0xdb858435
    );
    secp256k1_gej point;
    secp256k1_ge res;
    int i;

    secp256k1_gej_set_ge(&point, &secp256k1_ge_const_g);
    for (i = 0; i < 100; ++i) {
        secp256k1_ge tmp;
        secp256k1_ge_set_gej(&tmp, &point);
        secp256k1_ecmult_const(&point, &tmp, &scalar, 256);
    }
    secp256k1_ge_set_gej(&res, &point);
    ge_equals_gej(&res, &expected_point);
}

void run_ecmult_const_tests(void) {
    ecmult_const_mult_zero_one();
    ecmult_const_random_mult();
    ecmult_const_commutativity();
    ecmult_const_chain_multiply();
}

typedef struct {
    secp256k1_scalar *sc;
    secp256k1_ge *pt;
} ecmult_multi_data;

static int ecmult_multi_callback(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *cbdata) {
    ecmult_multi_data *data = (ecmult_multi_data*) cbdata;
    *sc = data->sc[idx];
    *pt = data->pt[idx];
    return 1;
}

static int ecmult_multi_false_callback(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *cbdata) {
    (void)sc;
    (void)pt;
    (void)idx;
    (void)cbdata;
    return 0;
}

void test_ecmult_multi(secp256k1_scratch *scratch, secp256k1_ecmult_multi_func ecmult_multi) {
    int ncount;
    secp256k1_scalar szero;
    secp256k1_scalar sc[32];
    secp256k1_ge pt[32];
    secp256k1_gej r;
    secp256k1_gej r2;
    ecmult_multi_data data;

    data.sc = sc;
    data.pt = pt;
    secp256k1_scalar_set_int(&szero, 0);

    /* No points to multiply */
    CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, NULL, ecmult_multi_callback, &data, 0));

    /* Check 1- and 2-point multiplies against ecmult */
    for (ncount = 0; ncount < count; ncount++) {
        secp256k1_ge ptg;
        secp256k1_gej ptgj;
        random_scalar_order(&sc[0]);
        random_scalar_order(&sc[1]);

        random_group_element_test(&ptg);
        secp256k1_gej_set_ge(&ptgj, &ptg);
        pt[0] = ptg;
        pt[1] = secp256k1_ge_const_g;

        /* only G scalar */
        secp256k1_ecmult(&ctx->ecmult_ctx, &r2, &ptgj, &szero, &sc[0]);
        CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &sc[0], ecmult_multi_callback, &data, 0));
        secp256k1_gej_neg(&r2, &r2);
        secp256k1_gej_add_var(&r, &r, &r2, NULL);
        CHECK(secp256k1_gej_is_infinity(&r));

        /* 1-point */
        secp256k1_ecmult(&ctx->ecmult_ctx, &r2, &ptgj, &sc[0], &szero);
        CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_callback, &data, 1));
        secp256k1_gej_neg(&r2, &r2);
        secp256k1_gej_add_var(&r, &r, &r2, NULL);
        CHECK(secp256k1_gej_is_infinity(&r));

        /* Try to multiply 1 point, but callback returns false */
        CHECK(!ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_false_callback, &data, 1));

        /* 2-point */
        secp256k1_ecmult(&ctx->ecmult_ctx, &r2, &ptgj, &sc[0], &sc[1]);
        CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_callback, &data, 2));
        secp256k1_gej_neg(&r2, &r2);
        secp256k1_gej_add_var(&r, &r, &r2, NULL);
        CHECK(secp256k1_gej_is_infinity(&r));

        /* 2-point with G scalar */
        secp256k1_ecmult(&ctx->ecmult_ctx, &r2, &ptgj, &sc[0], &sc[1]);
        CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &sc[1], ecmult_multi_callback, &data, 1));
        secp256k1_gej_neg(&r2, &r2);
        secp256k1_gej_add_var(&r, &r, &r2, NULL);
        CHECK(secp256k1_gej_is_infinity(&r));
    }

    /* Check infinite outputs of various forms */
    for (ncount = 0; ncount < count; ncount++) {
        secp256k1_ge ptg;
        size_t i, j;
        size_t sizes[] = { 2, 10, 32 };

        for (j = 0; j < 3; j++) {
            for (i = 0; i < 32; i++) {
                random_scalar_order(&sc[i]);
                secp256k1_ge_set_infinity(&pt[i]);
            }
            CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_callback, &data, sizes[j]));
            CHECK(secp256k1_gej_is_infinity(&r));
        }

        for (j = 0; j < 3; j++) {
            for (i = 0; i < 32; i++) {
                random_group_element_test(&ptg);
                pt[i] = ptg;
                secp256k1_scalar_set_int(&sc[i], 0);
            }
            CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_callback, &data, sizes[j]));
            CHECK(secp256k1_gej_is_infinity(&r));
        }

        for (j = 0; j < 3; j++) {
            random_group_element_test(&ptg);
            for (i = 0; i < 16; i++) {
                random_scalar_order(&sc[2*i]);
                secp256k1_scalar_negate(&sc[2*i + 1], &sc[2*i]);
                pt[2 * i] = ptg;
                pt[2 * i + 1] = ptg;
            }

            CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_callback, &data, sizes[j]));
            CHECK(secp256k1_gej_is_infinity(&r));

            random_scalar_order(&sc[0]);
            for (i = 0; i < 16; i++) {
                random_group_element_test(&ptg);

                sc[2*i] = sc[0];
                sc[2*i+1] = sc[0];
                pt[2 * i] = ptg;
                secp256k1_ge_neg(&pt[2*i+1], &pt[2*i]);
            }

            CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_callback, &data, sizes[j]));
            CHECK(secp256k1_gej_is_infinity(&r));
        }

        random_group_element_test(&ptg);
        secp256k1_scalar_set_int(&sc[0], 0);
        pt[0] = ptg;
        for (i = 1; i < 32; i++) {
            pt[i] = ptg;

            random_scalar_order(&sc[i]);
            secp256k1_scalar_add(&sc[0], &sc[0], &sc[i]);
            secp256k1_scalar_negate(&sc[i], &sc[i]);
        }

        CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_callback, &data, 32));
        CHECK(secp256k1_gej_is_infinity(&r));
    }

    /* Check random points, constant scalar */
    for (ncount = 0; ncount < count; ncount++) {
        size_t i;
        secp256k1_gej_set_infinity(&r);

        random_scalar_order(&sc[0]);
        for (i = 0; i < 20; i++) {
            secp256k1_ge ptg;
            sc[i] = sc[0];
            random_group_element_test(&ptg);
            pt[i] = ptg;
            secp256k1_gej_add_ge_var(&r, &r, &pt[i], NULL);
        }

        secp256k1_ecmult(&ctx->ecmult_ctx, &r2, &r, &sc[0], &szero);
        CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_callback, &data, 20));
        secp256k1_gej_neg(&r2, &r2);
        secp256k1_gej_add_var(&r, &r, &r2, NULL);
        CHECK(secp256k1_gej_is_infinity(&r));
    }

    /* Check random scalars, constant point */
    for (ncount = 0; ncount < count; ncount++) {
        size_t i;
        secp256k1_ge ptg;
        secp256k1_gej p0j;
        secp256k1_scalar rs;
        secp256k1_scalar_set_int(&rs, 0);

        random_group_element_test(&ptg);
        for (i = 0; i < 20; i++) {
            random_scalar_order(&sc[i]);
            pt[i] = ptg;
            secp256k1_scalar_add(&rs, &rs, &sc[i]);
        }

        secp256k1_gej_set_ge(&p0j, &pt[0]);
        secp256k1_ecmult(&ctx->ecmult_ctx, &r2, &p0j, &rs, &szero);
        CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_callback, &data, 20));
        secp256k1_gej_neg(&r2, &r2);
        secp256k1_gej_add_var(&r, &r, &r2, NULL);
        CHECK(secp256k1_gej_is_infinity(&r));
    }

    /* Sanity check that zero scalars don't cause problems */
    for (ncount = 0; ncount < 20; ncount++) {
        random_scalar_order(&sc[ncount]);
        random_group_element_test(&pt[ncount]);
    }

    secp256k1_scalar_clear(&sc[0]);
    CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_callback, &data, 20));
    secp256k1_scalar_clear(&sc[1]);
    secp256k1_scalar_clear(&sc[2]);
    secp256k1_scalar_clear(&sc[3]);
    secp256k1_scalar_clear(&sc[4]);
    CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_callback, &data, 6));
    CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &szero, ecmult_multi_callback, &data, 5));
    CHECK(secp256k1_gej_is_infinity(&r));

    /* Run through s0*(t0*P) + s1*(t1*P) exhaustively for many small values of s0, s1, t0, t1 */
    {
        const size_t TOP = 8;
        size_t s0i, s1i;
        size_t t0i, t1i;
        secp256k1_ge ptg;
        secp256k1_gej ptgj;

        random_group_element_test(&ptg);
        secp256k1_gej_set_ge(&ptgj, &ptg);

        for(t0i = 0; t0i < TOP; t0i++) {
            for(t1i = 0; t1i < TOP; t1i++) {
                secp256k1_gej t0p, t1p;
                secp256k1_scalar t0, t1;

                secp256k1_scalar_set_int(&t0, (t0i + 1) / 2);
                secp256k1_scalar_cond_negate(&t0, t0i & 1);
                secp256k1_scalar_set_int(&t1, (t1i + 1) / 2);
                secp256k1_scalar_cond_negate(&t1, t1i & 1);

                secp256k1_ecmult(&ctx->ecmult_ctx, &t0p, &ptgj, &t0, &szero);
                secp256k1_ecmult(&ctx->ecmult_ctx, &t1p, &ptgj, &t1, &szero);

                for(s0i = 0; s0i < TOP; s0i++) {
                    for(s1i = 0; s1i < TOP; s1i++) {
                        secp256k1_scalar tmp1, tmp2;
                        secp256k1_gej expected, actual;

                        secp256k1_ge_set_gej(&pt[0], &t0p);
                        secp256k1_ge_set_gej(&pt[1], &t1p);

                        secp256k1_scalar_set_int(&sc[0], (s0i + 1) / 2);
                        secp256k1_scalar_cond_negate(&sc[0], s0i & 1);
                        secp256k1_scalar_set_int(&sc[1], (s1i + 1) / 2);
                        secp256k1_scalar_cond_negate(&sc[1], s1i & 1);

                        secp256k1_scalar_mul(&tmp1, &t0, &sc[0]);
                        secp256k1_scalar_mul(&tmp2, &t1, &sc[1]);
                        secp256k1_scalar_add(&tmp1, &tmp1, &tmp2);

                        secp256k1_ecmult(&ctx->ecmult_ctx, &expected, &ptgj, &tmp1, &szero);
                        CHECK(ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &actual, &szero, ecmult_multi_callback, &data, 2));
                        secp256k1_gej_neg(&expected, &expected);
                        secp256k1_gej_add_var(&actual, &actual, &expected, NULL);
                        CHECK(secp256k1_gej_is_infinity(&actual));
                    }
                }
            }
        }
    }
}

void test_ecmult_multi_batch_single(secp256k1_ecmult_multi_func ecmult_multi) {
    secp256k1_scalar szero;
    secp256k1_scalar sc;
    secp256k1_ge pt;
    secp256k1_gej r;
    ecmult_multi_data data;
    secp256k1_scratch *scratch_empty;

    random_group_element_test(&pt);
    random_scalar_order(&sc);
    data.sc = &sc;
    data.pt = &pt;
    secp256k1_scalar_set_int(&szero, 0);

    /* Try to multiply 1 point, but scratch space is empty.*/
    scratch_empty = secp256k1_scratch_create(&ctx->error_callback, 0);
    CHECK(!ecmult_multi(&ctx->error_callback, &ctx->ecmult_ctx, scratch_empty, &r, &szero, ecmult_multi_callback, &data, 1));
    secp256k1_scratch_destroy(&ctx->error_callback, scratch_empty);
}

void test_secp256k1_pippenger_bucket_window_inv(void) {
    int i;

    CHECK(secp256k1_pippenger_bucket_window_inv(0) == 0);
    for(i = 1; i <= PIPPENGER_MAX_BUCKET_WINDOW; i++) {
        /* Bucket_window of 8 is not used with endo */
        if (i == 8) {
            continue;
        }
        CHECK(secp256k1_pippenger_bucket_window(secp256k1_pippenger_bucket_window_inv(i)) == i);
        if (i != PIPPENGER_MAX_BUCKET_WINDOW) {
            CHECK(secp256k1_pippenger_bucket_window(secp256k1_pippenger_bucket_window_inv(i)+1) > i);
        }
    }
}

/**
 * Probabilistically test the function returning the maximum number of possible points
 * for a given scratch space.
 */
void test_ecmult_multi_pippenger_max_points(void) {
    size_t scratch_size = secp256k1_testrand_int(256);
    size_t max_size = secp256k1_pippenger_scratch_size(secp256k1_pippenger_bucket_window_inv(PIPPENGER_MAX_BUCKET_WINDOW-1)+512, 12);
    secp256k1_scratch *scratch;
    size_t n_points_supported;
    int bucket_window = 0;

    for(; scratch_size < max_size; scratch_size+=256) {
        size_t i;
        size_t total_alloc;
        size_t checkpoint;
        scratch = secp256k1_scratch_create(&ctx->error_callback, scratch_size);
        CHECK(scratch != NULL);
        checkpoint = secp256k1_scratch_checkpoint(&ctx->error_callback, scratch);
        n_points_supported = secp256k1_pippenger_max_points(&ctx->error_callback, scratch);
        if (n_points_supported == 0) {
            secp256k1_scratch_destroy(&ctx->error_callback, scratch);
            continue;
        }
        bucket_window = secp256k1_pippenger_bucket_window(n_points_supported);
        /* allocate `total_alloc` bytes over `PIPPENGER_SCRATCH_OBJECTS` many allocations */
        total_alloc = secp256k1_pippenger_scratch_size(n_points_supported, bucket_window);
        for (i = 0; i < PIPPENGER_SCRATCH_OBJECTS - 1; i++) {
            CHECK(secp256k1_scratch_alloc(&ctx->error_callback, scratch, 1));
            total_alloc--;
        }
        CHECK(secp256k1_scratch_alloc(&ctx->error_callback, scratch, total_alloc));
        secp256k1_scratch_apply_checkpoint(&ctx->error_callback, scratch, checkpoint);
        secp256k1_scratch_destroy(&ctx->error_callback, scratch);
    }
    CHECK(bucket_window == PIPPENGER_MAX_BUCKET_WINDOW);
}

void test_ecmult_multi_batch_size_helper(void) {
    size_t n_batches, n_batch_points, max_n_batch_points, n;

    max_n_batch_points = 0;
    n = 1;
    CHECK(secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, max_n_batch_points, n) == 0);

    max_n_batch_points = 1;
    n = 0;
    CHECK(secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, max_n_batch_points, n) == 1);
    CHECK(n_batches == 0);
    CHECK(n_batch_points == 0);

    max_n_batch_points = 2;
    n = 5;
    CHECK(secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, max_n_batch_points, n) == 1);
    CHECK(n_batches == 3);
    CHECK(n_batch_points == 2);

    max_n_batch_points = ECMULT_MAX_POINTS_PER_BATCH;
    n = ECMULT_MAX_POINTS_PER_BATCH;
    CHECK(secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, max_n_batch_points, n) == 1);
    CHECK(n_batches == 1);
    CHECK(n_batch_points == ECMULT_MAX_POINTS_PER_BATCH);

    max_n_batch_points = ECMULT_MAX_POINTS_PER_BATCH + 1;
    n = ECMULT_MAX_POINTS_PER_BATCH + 1;
    CHECK(secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, max_n_batch_points, n) == 1);
    CHECK(n_batches == 2);
    CHECK(n_batch_points == ECMULT_MAX_POINTS_PER_BATCH/2 + 1);

    max_n_batch_points = 1;
    n = SIZE_MAX;
    CHECK(secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, max_n_batch_points, n) == 1);
    CHECK(n_batches == SIZE_MAX);
    CHECK(n_batch_points == 1);

    max_n_batch_points = 2;
    n = SIZE_MAX;
    CHECK(secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, max_n_batch_points, n) == 1);
    CHECK(n_batches == SIZE_MAX/2 + 1);
    CHECK(n_batch_points == 2);
}

/**
 * Run secp256k1_ecmult_multi_var with num points and a scratch space restricted to
 * 1 <= i <= num points.
 */
void test_ecmult_multi_batching(void) {
    static const int n_points = 2*ECMULT_PIPPENGER_THRESHOLD;
    secp256k1_scalar scG;
    secp256k1_scalar szero;
    secp256k1_scalar *sc = (secp256k1_scalar *)checked_malloc(&ctx->error_callback, sizeof(secp256k1_scalar) * n_points);
    secp256k1_ge *pt = (secp256k1_ge *)checked_malloc(&ctx->error_callback, sizeof(secp256k1_ge) * n_points);
    secp256k1_gej r;
    secp256k1_gej r2;
    ecmult_multi_data data;
    int i;
    secp256k1_scratch *scratch;

    secp256k1_gej_set_infinity(&r2);
    secp256k1_scalar_set_int(&szero, 0);

    /* Get random scalars and group elements and compute result */
    random_scalar_order(&scG);
    secp256k1_ecmult(&ctx->ecmult_ctx, &r2, &r2, &szero, &scG);
    for(i = 0; i < n_points; i++) {
        secp256k1_ge ptg;
        secp256k1_gej ptgj;
        random_group_element_test(&ptg);
        secp256k1_gej_set_ge(&ptgj, &ptg);
        pt[i] = ptg;
        random_scalar_order(&sc[i]);
        secp256k1_ecmult(&ctx->ecmult_ctx, &ptgj, &ptgj, &sc[i], NULL);
        secp256k1_gej_add_var(&r2, &r2, &ptgj, NULL);
    }
    data.sc = sc;
    data.pt = pt;
    secp256k1_gej_neg(&r2, &r2);

    /* Test with empty scratch space. It should compute the correct result using
     * ecmult_mult_simple algorithm which doesn't require a scratch space. */
    scratch = secp256k1_scratch_create(&ctx->error_callback, 0);
    CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &scG, ecmult_multi_callback, &data, n_points));
    secp256k1_gej_add_var(&r, &r, &r2, NULL);
    CHECK(secp256k1_gej_is_infinity(&r));
    secp256k1_scratch_destroy(&ctx->error_callback, scratch);

    /* Test with space for 1 point in pippenger. That's not enough because
     * ecmult_multi selects strauss which requires more memory. It should
     * therefore select the simple algorithm. */
    scratch = secp256k1_scratch_create(&ctx->error_callback, secp256k1_pippenger_scratch_size(1, 1) + PIPPENGER_SCRATCH_OBJECTS*ALIGNMENT);
    CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &scG, ecmult_multi_callback, &data, n_points));
    secp256k1_gej_add_var(&r, &r, &r2, NULL);
    CHECK(secp256k1_gej_is_infinity(&r));
    secp256k1_scratch_destroy(&ctx->error_callback, scratch);

    for(i = 1; i <= n_points; i++) {
        if (i > ECMULT_PIPPENGER_THRESHOLD) {
            int bucket_window = secp256k1_pippenger_bucket_window(i);
            size_t scratch_size = secp256k1_pippenger_scratch_size(i, bucket_window);
            scratch = secp256k1_scratch_create(&ctx->error_callback, scratch_size + PIPPENGER_SCRATCH_OBJECTS*ALIGNMENT);
        } else {
            size_t scratch_size = secp256k1_strauss_scratch_size(i);
            scratch = secp256k1_scratch_create(&ctx->error_callback, scratch_size + STRAUSS_SCRATCH_OBJECTS*ALIGNMENT);
        }
        CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, &ctx->ecmult_ctx, scratch, &r, &scG, ecmult_multi_callback, &data, n_points));
        secp256k1_gej_add_var(&r, &r, &r2, NULL);
        CHECK(secp256k1_gej_is_infinity(&r));
        secp256k1_scratch_destroy(&ctx->error_callback, scratch);
    }
    free(sc);
    free(pt);
}

void run_ecmult_multi_tests(void) {
    secp256k1_scratch *scratch;

    test_secp256k1_pippenger_bucket_window_inv();
    test_ecmult_multi_pippenger_max_points();
    scratch = secp256k1_scratch_create(&ctx->error_callback, 819200);
    test_ecmult_multi(scratch, secp256k1_ecmult_multi_var);
    test_ecmult_multi(NULL, secp256k1_ecmult_multi_var);
    test_ecmult_multi(scratch, secp256k1_ecmult_pippenger_batch_single);
    test_ecmult_multi_batch_single(secp256k1_ecmult_pippenger_batch_single);
    test_ecmult_multi(scratch, secp256k1_ecmult_strauss_batch_single);
    test_ecmult_multi_batch_single(secp256k1_ecmult_strauss_batch_single);
    secp256k1_scratch_destroy(&ctx->error_callback, scratch);

    /* Run test_ecmult_multi with space for exactly one point */
    scratch = secp256k1_scratch_create(&ctx->error_callback, secp256k1_strauss_scratch_size(1) + STRAUSS_SCRATCH_OBJECTS*ALIGNMENT);
    test_ecmult_multi(scratch, secp256k1_ecmult_multi_var);
    secp256k1_scratch_destroy(&ctx->error_callback, scratch);

    test_ecmult_multi_batch_size_helper();
    test_ecmult_multi_batching();
}

void test_wnaf(const secp256k1_scalar *number, int w) {
    secp256k1_scalar x, two, t;
    int wnaf[256];
    int zeroes = -1;
    int i;
    int bits;
    secp256k1_scalar_set_int(&x, 0);
    secp256k1_scalar_set_int(&two, 2);
    bits = secp256k1_ecmult_wnaf(wnaf, 256, number, w);
    CHECK(bits <= 256);
    for (i = bits-1; i >= 0; i--) {
        int v = wnaf[i];
        secp256k1_scalar_mul(&x, &x, &two);
        if (v) {
            CHECK(zeroes == -1 || zeroes >= w-1); /* check that distance between non-zero elements is at least w-1 */
            zeroes=0;
            CHECK((v & 1) == 1); /* check non-zero elements are odd */
            CHECK(v <= (1 << (w-1)) - 1); /* check range below */
            CHECK(v >= -(1 << (w-1)) - 1); /* check range above */
        } else {
            CHECK(zeroes != -1); /* check that no unnecessary zero padding exists */
            zeroes++;
        }
        if (v >= 0) {
            secp256k1_scalar_set_int(&t, v);
        } else {
            secp256k1_scalar_set_int(&t, -v);
            secp256k1_scalar_negate(&t, &t);
        }
        secp256k1_scalar_add(&x, &x, &t);
    }
    CHECK(secp256k1_scalar_eq(&x, number)); /* check that wnaf represents number */
}

void test_constant_wnaf_negate(const secp256k1_scalar *number) {
    secp256k1_scalar neg1 = *number;
    secp256k1_scalar neg2 = *number;
    int sign1 = 1;
    int sign2 = 1;

    if (!secp256k1_scalar_get_bits(&neg1, 0, 1)) {
        secp256k1_scalar_negate(&neg1, &neg1);
        sign1 = -1;
    }
    sign2 = secp256k1_scalar_cond_negate(&neg2, secp256k1_scalar_is_even(&neg2));
    CHECK(sign1 == sign2);
    CHECK(secp256k1_scalar_eq(&neg1, &neg2));
}

void test_constant_wnaf(const secp256k1_scalar *number, int w) {
    secp256k1_scalar x, shift;
    int wnaf[256] = {0};
    int i;
    int skew;
    int bits = 256;
    secp256k1_scalar num = *number;
    secp256k1_scalar scalar_skew;

    secp256k1_scalar_set_int(&x, 0);
    secp256k1_scalar_set_int(&shift, 1 << w);
    for (i = 0; i < 16; ++i) {
        secp256k1_scalar_shr_int(&num, 8);
    }
    bits = 128;
    skew = secp256k1_wnaf_const(wnaf, &num, w, bits);

    for (i = WNAF_SIZE_BITS(bits, w); i >= 0; --i) {
        secp256k1_scalar t;
        int v = wnaf[i];
        CHECK(v != 0); /* check nonzero */
        CHECK(v & 1);  /* check parity */
        CHECK(v > -(1 << w)); /* check range above */
        CHECK(v < (1 << w));  /* check range below */

        secp256k1_scalar_mul(&x, &x, &shift);
        if (v >= 0) {
            secp256k1_scalar_set_int(&t, v);
        } else {
            secp256k1_scalar_set_int(&t, -v);
            secp256k1_scalar_negate(&t, &t);
        }
        secp256k1_scalar_add(&x, &x, &t);
    }
    /* Skew num because when encoding numbers as odd we use an offset */
    secp256k1_scalar_set_int(&scalar_skew, 1 << (skew == 2));
    secp256k1_scalar_add(&num, &num, &scalar_skew);
    CHECK(secp256k1_scalar_eq(&x, &num));
}

void test_fixed_wnaf(const secp256k1_scalar *number, int w) {
    secp256k1_scalar x, shift;
    int wnaf[256] = {0};
    int i;
    int skew;
    secp256k1_scalar num = *number;

    secp256k1_scalar_set_int(&x, 0);
    secp256k1_scalar_set_int(&shift, 1 << w);
    for (i = 0; i < 16; ++i) {
        secp256k1_scalar_shr_int(&num, 8);
    }
    skew = secp256k1_wnaf_fixed(wnaf, &num, w);

    for (i = WNAF_SIZE(w)-1; i >= 0; --i) {
        secp256k1_scalar t;
        int v = wnaf[i];
        CHECK(v == 0 || v & 1);  /* check parity */
        CHECK(v > -(1 << w)); /* check range above */
        CHECK(v < (1 << w));  /* check range below */

        secp256k1_scalar_mul(&x, &x, &shift);
        if (v >= 0) {
            secp256k1_scalar_set_int(&t, v);
        } else {
            secp256k1_scalar_set_int(&t, -v);
            secp256k1_scalar_negate(&t, &t);
        }
        secp256k1_scalar_add(&x, &x, &t);
    }
    /* If skew is 1 then add 1 to num */
    secp256k1_scalar_cadd_bit(&num, 0, skew == 1);
    CHECK(secp256k1_scalar_eq(&x, &num));
}

/* Checks that the first 8 elements of wnaf are equal to wnaf_expected and the
 * rest is 0.*/
void test_fixed_wnaf_small_helper(int *wnaf, int *wnaf_expected, int w) {
    int i;
    for (i = WNAF_SIZE(w)-1; i >= 8; --i) {
        CHECK(wnaf[i] == 0);
    }
    for (i = 7; i >= 0; --i) {
        CHECK(wnaf[i] == wnaf_expected[i]);
    }
}

void test_fixed_wnaf_small(void) {
    int w = 4;
    int wnaf[256] = {0};
    int i;
    int skew;
    secp256k1_scalar num;

    secp256k1_scalar_set_int(&num, 0);
    skew = secp256k1_wnaf_fixed(wnaf, &num, w);
    for (i = WNAF_SIZE(w)-1; i >= 0; --i) {
        int v = wnaf[i];
        CHECK(v == 0);
    }
    CHECK(skew == 0);

    secp256k1_scalar_set_int(&num, 1);
    skew = secp256k1_wnaf_fixed(wnaf, &num, w);
    for (i = WNAF_SIZE(w)-1; i >= 1; --i) {
        int v = wnaf[i];
        CHECK(v == 0);
    }
    CHECK(wnaf[0] == 1);
    CHECK(skew == 0);

    {
        int wnaf_expected[8] = { 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf };
        secp256k1_scalar_set_int(&num, 0xffffffff);
        skew = secp256k1_wnaf_fixed(wnaf, &num, w);
        test_fixed_wnaf_small_helper(wnaf, wnaf_expected, w);
        CHECK(skew == 0);
    }
    {
        int wnaf_expected[8] = { -1, -1, -1, -1, -1, -1, -1, 0xf };
        secp256k1_scalar_set_int(&num, 0xeeeeeeee);
        skew = secp256k1_wnaf_fixed(wnaf, &num, w);
        test_fixed_wnaf_small_helper(wnaf, wnaf_expected, w);
        CHECK(skew == 1);
    }
    {
        int wnaf_expected[8] = { 1, 0, 1, 0, 1, 0, 1, 0 };
        secp256k1_scalar_set_int(&num, 0x01010101);
        skew = secp256k1_wnaf_fixed(wnaf, &num, w);
        test_fixed_wnaf_small_helper(wnaf, wnaf_expected, w);
        CHECK(skew == 0);
    }
    {
        int wnaf_expected[8] = { -0xf, 0, 0xf, -0xf, 0, 0xf, 1, 0 };
        secp256k1_scalar_set_int(&num, 0x01ef1ef1);
        skew = secp256k1_wnaf_fixed(wnaf, &num, w);
        test_fixed_wnaf_small_helper(wnaf, wnaf_expected, w);
        CHECK(skew == 0);
    }
}

void run_wnaf(void) {
    int i;
    secp256k1_scalar n = {{0}};

    test_constant_wnaf(&n, 4);
    /* Sanity check: 1 and 2 are the smallest odd and even numbers and should
     *               have easier-to-diagnose failure modes  */
    n.d[0] = 1;
    test_constant_wnaf(&n, 4);
    n.d[0] = 2;
    test_constant_wnaf(&n, 4);
    /* Test -1, because it's a special case in wnaf_const */
    n = secp256k1_scalar_one;
    secp256k1_scalar_negate(&n, &n);
    test_constant_wnaf(&n, 4);

    /* Test -2, which may not lead to overflows in wnaf_const */
    secp256k1_scalar_add(&n, &secp256k1_scalar_one, &secp256k1_scalar_one);
    secp256k1_scalar_negate(&n, &n);
    test_constant_wnaf(&n, 4);

    /* Test (1/2) - 1 = 1/-2 and 1/2 = (1/-2) + 1
       as corner cases of negation handling in wnaf_const */
    secp256k1_scalar_inverse(&n, &n);
    test_constant_wnaf(&n, 4);

    secp256k1_scalar_add(&n, &n, &secp256k1_scalar_one);
    test_constant_wnaf(&n, 4);

    /* Test 0 for fixed wnaf */
    test_fixed_wnaf_small();
    /* Random tests */
    for (i = 0; i < count; i++) {
        random_scalar_order(&n);
        test_wnaf(&n, 4+(i%10));
        test_constant_wnaf_negate(&n);
        test_constant_wnaf(&n, 4 + (i % 10));
        test_fixed_wnaf(&n, 4 + (i % 10));
    }
    secp256k1_scalar_set_int(&n, 0);
    CHECK(secp256k1_scalar_cond_negate(&n, 1) == -1);
    CHECK(secp256k1_scalar_is_zero(&n));
    CHECK(secp256k1_scalar_cond_negate(&n, 0) == 1);
    CHECK(secp256k1_scalar_is_zero(&n));
}

void test_ecmult_constants(void) {
    /* Test ecmult_gen() for [0..36) and [order-36..0). */
    secp256k1_scalar x;
    secp256k1_gej r;
    secp256k1_ge ng;
    int i;
    int j;
    secp256k1_ge_neg(&ng, &secp256k1_ge_const_g);
    for (i = 0; i < 36; i++ ) {
        secp256k1_scalar_set_int(&x, i);
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &r, &x);
        for (j = 0; j < i; j++) {
            if (j == i - 1) {
                ge_equals_gej(&secp256k1_ge_const_g, &r);
            }
            secp256k1_gej_add_ge(&r, &r, &ng);
        }
        CHECK(secp256k1_gej_is_infinity(&r));
    }
    for (i = 1; i <= 36; i++ ) {
        secp256k1_scalar_set_int(&x, i);
        secp256k1_scalar_negate(&x, &x);
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &r, &x);
        for (j = 0; j < i; j++) {
            if (j == i - 1) {
                ge_equals_gej(&ng, &r);
            }
            secp256k1_gej_add_ge(&r, &r, &secp256k1_ge_const_g);
        }
        CHECK(secp256k1_gej_is_infinity(&r));
    }
}

void run_ecmult_constants(void) {
    test_ecmult_constants();
}

void test_ecmult_gen_blind(void) {
    /* Test ecmult_gen() blinding and confirm that the blinding changes, the affine points match, and the z's don't match. */
    secp256k1_scalar key;
    secp256k1_scalar b;
    unsigned char seed32[32];
    secp256k1_gej pgej;
    secp256k1_gej pgej2;
    secp256k1_gej i;
    secp256k1_ge pge;
    random_scalar_order_test(&key);
    secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &pgej, &key);
    secp256k1_testrand256(seed32);
    b = ctx->ecmult_gen_ctx.blind;
    i = ctx->ecmult_gen_ctx.initial;
    secp256k1_ecmult_gen_blind(&ctx->ecmult_gen_ctx, seed32);
    CHECK(!secp256k1_scalar_eq(&b, &ctx->ecmult_gen_ctx.blind));
    secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &pgej2, &key);
    CHECK(!gej_xyz_equals_gej(&pgej, &pgej2));
    CHECK(!gej_xyz_equals_gej(&i, &ctx->ecmult_gen_ctx.initial));
    secp256k1_ge_set_gej(&pge, &pgej);
    ge_equals_gej(&pge, &pgej2);
}

void test_ecmult_gen_blind_reset(void) {
    /* Test ecmult_gen() blinding reset and confirm that the blinding is consistent. */
    secp256k1_scalar b;
    secp256k1_gej initial;
    secp256k1_ecmult_gen_blind(&ctx->ecmult_gen_ctx, 0);
    b = ctx->ecmult_gen_ctx.blind;
    initial = ctx->ecmult_gen_ctx.initial;
    secp256k1_ecmult_gen_blind(&ctx->ecmult_gen_ctx, 0);
    CHECK(secp256k1_scalar_eq(&b, &ctx->ecmult_gen_ctx.blind));
    CHECK(gej_xyz_equals_gej(&initial, &ctx->ecmult_gen_ctx.initial));
}

void run_ecmult_gen_blind(void) {
    int i;
    test_ecmult_gen_blind_reset();
    for (i = 0; i < 10; i++) {
        test_ecmult_gen_blind();
    }
}

/***** ENDOMORPHISH TESTS *****/
void test_scalar_split(const secp256k1_scalar* full) {
    secp256k1_scalar s, s1, slam;
    const unsigned char zero[32] = {0};
    unsigned char tmp[32];

    secp256k1_scalar_split_lambda(&s1, &slam, full);

    /* check slam*lambda + s1 == full */
    secp256k1_scalar_mul(&s, &secp256k1_const_lambda, &slam);
    secp256k1_scalar_add(&s, &s, &s1);
    CHECK(secp256k1_scalar_eq(&s, full));

    /* check that both are <= 128 bits in size */
    if (secp256k1_scalar_is_high(&s1)) {
        secp256k1_scalar_negate(&s1, &s1);
    }
    if (secp256k1_scalar_is_high(&slam)) {
        secp256k1_scalar_negate(&slam, &slam);
    }

    secp256k1_scalar_get_b32(tmp, &s1);
    CHECK(secp256k1_memcmp_var(zero, tmp, 16) == 0);
    secp256k1_scalar_get_b32(tmp, &slam);
    CHECK(secp256k1_memcmp_var(zero, tmp, 16) == 0);
}


void run_endomorphism_tests(void) {
    unsigned i;
    static secp256k1_scalar s;
    test_scalar_split(&secp256k1_scalar_zero);
    test_scalar_split(&secp256k1_scalar_one);
    secp256k1_scalar_negate(&s,&secp256k1_scalar_one);
    test_scalar_split(&s);
    test_scalar_split(&secp256k1_const_lambda);
    secp256k1_scalar_add(&s, &secp256k1_const_lambda, &secp256k1_scalar_one);
    test_scalar_split(&s);

    for (i = 0; i < 100U * count; ++i) {
        secp256k1_scalar full;
        random_scalar_order_test(&full);
        test_scalar_split(&full);
    }
    for (i = 0; i < sizeof(scalars_near_split_bounds) / sizeof(scalars_near_split_bounds[0]); ++i) {
        test_scalar_split(&scalars_near_split_bounds[i]);
    }
}

void ec_pubkey_parse_pointtest(const unsigned char *input, int xvalid, int yvalid) {
    unsigned char pubkeyc[65];
    secp256k1_pubkey pubkey;
    secp256k1_ge ge;
    size_t pubkeyclen;
    int32_t ecount;
    ecount = 0;
    secp256k1_context_set_illegal_callback(ctx, counting_illegal_callback_fn, &ecount);
    for (pubkeyclen = 3; pubkeyclen <= 65; pubkeyclen++) {
        /* Smaller sizes are tested exhaustively elsewhere. */
        int32_t i;
        memcpy(&pubkeyc[1], input, 64);
        VG_UNDEF(&pubkeyc[pubkeyclen], 65 - pubkeyclen);
        for (i = 0; i < 256; i++) {
            /* Try all type bytes. */
            int xpass;
            int ypass;
            int ysign;
            pubkeyc[0] = i;
            /* What sign does this point have? */
            ysign = (input[63] & 1) + 2;
            /* For the current type (i) do we expect parsing to work? Handled all of compressed/uncompressed/hybrid. */
            xpass = xvalid && (pubkeyclen == 33) && ((i & 254) == 2);
            /* Do we expect a parse and re-serialize as uncompressed to give a matching y? */
            ypass = xvalid && yvalid && ((i & 4) == ((pubkeyclen == 65) << 2)) &&
                ((i == 4) || ((i & 251) == ysign)) && ((pubkeyclen == 33) || (pubkeyclen == 65));
            if (xpass || ypass) {
                /* These cases must parse. */
                unsigned char pubkeyo[65];
                size_t outl;
                memset(&pubkey, 0, sizeof(pubkey));
                VG_UNDEF(&pubkey, sizeof(pubkey));
                ecount = 0;
                CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, pubkeyc, pubkeyclen) == 1);
                VG_CHECK(&pubkey, sizeof(pubkey));
                outl = 65;
                VG_UNDEF(pubkeyo, 65);
                CHECK(secp256k1_ec_pubkey_serialize(ctx, pubkeyo, &outl, &pubkey, SECP256K1_EC_COMPRESSED) == 1);
                VG_CHECK(pubkeyo, outl);
                CHECK(outl == 33);
                CHECK(secp256k1_memcmp_var(&pubkeyo[1], &pubkeyc[1], 32) == 0);
                CHECK((pubkeyclen != 33) || (pubkeyo[0] == pubkeyc[0]));
                if (ypass) {
                    /* This test isn't always done because we decode with alternative signs, so the y won't match. */
                    CHECK(pubkeyo[0] == ysign);
                    CHECK(secp256k1_pubkey_load(ctx, &ge, &pubkey) == 1);
                    memset(&pubkey, 0, sizeof(pubkey));
                    VG_UNDEF(&pubkey, sizeof(pubkey));
                    secp256k1_pubkey_save(&pubkey, &ge);
                    VG_CHECK(&pubkey, sizeof(pubkey));
                    outl = 65;
                    VG_UNDEF(pubkeyo, 65);
                    CHECK(secp256k1_ec_pubkey_serialize(ctx, pubkeyo, &outl, &pubkey, SECP256K1_EC_UNCOMPRESSED) == 1);
                    VG_CHECK(pubkeyo, outl);
                    CHECK(outl == 65);
                    CHECK(pubkeyo[0] == 4);
                    CHECK(secp256k1_memcmp_var(&pubkeyo[1], input, 64) == 0);
                }
                CHECK(ecount == 0);
            } else {
                /* These cases must fail to parse. */
                memset(&pubkey, 0xfe, sizeof(pubkey));
                ecount = 0;
                VG_UNDEF(&pubkey, sizeof(pubkey));
                CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, pubkeyc, pubkeyclen) == 0);
                VG_CHECK(&pubkey, sizeof(pubkey));
                CHECK(ecount == 0);
                CHECK(secp256k1_pubkey_load(ctx, &ge, &pubkey) == 0);
                CHECK(ecount == 1);
            }
        }
    }
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

void run_ec_pubkey_parse_test(void) {
#define SECP256K1_EC_PARSE_TEST_NVALID (12)
    const unsigned char valid[SECP256K1_EC_PARSE_TEST_NVALID][64] = {
        {
            /* Point with leading and trailing zeros in x and y serialization. */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x52,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x64, 0xef, 0xa1, 0x7b, 0x77, 0x61, 0xe1, 0xe4, 0x27, 0x06, 0x98, 0x9f, 0xb4, 0x83,
            0xb8, 0xd2, 0xd4, 0x9b, 0xf7, 0x8f, 0xae, 0x98, 0x03, 0xf0, 0x99, 0xb8, 0x34, 0xed, 0xeb, 0x00
        },
        {
            /* Point with x equal to a 3rd root of unity.*/
            0x7a, 0xe9, 0x6a, 0x2b, 0x65, 0x7c, 0x07, 0x10, 0x6e, 0x64, 0x47, 0x9e, 0xac, 0x34, 0x34, 0xe9,
            0x9c, 0xf0, 0x49, 0x75, 0x12, 0xf5, 0x89, 0x95, 0xc1, 0x39, 0x6c, 0x28, 0x71, 0x95, 0x01, 0xee,
            0x42, 0x18, 0xf2, 0x0a, 0xe6, 0xc6, 0x46, 0xb3, 0x63, 0xdb, 0x68, 0x60, 0x58, 0x22, 0xfb, 0x14,
            0x26, 0x4c, 0xa8, 0xd2, 0x58, 0x7f, 0xdd, 0x6f, 0xbc, 0x75, 0x0d, 0x58, 0x7e, 0x76, 0xa7, 0xee,
        },
        {
            /* Point with largest x. (1/2) */
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x2c,
            0x0e, 0x99, 0x4b, 0x14, 0xea, 0x72, 0xf8, 0xc3, 0xeb, 0x95, 0xc7, 0x1e, 0xf6, 0x92, 0x57, 0x5e,
            0x77, 0x50, 0x58, 0x33, 0x2d, 0x7e, 0x52, 0xd0, 0x99, 0x5c, 0xf8, 0x03, 0x88, 0x71, 0xb6, 0x7d,
        },
        {
            /* Point with largest x. (2/2) */
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x2c,
            0xf1, 0x66, 0xb4, 0xeb, 0x15, 0x8d, 0x07, 0x3c, 0x14, 0x6a, 0x38, 0xe1, 0x09, 0x6d, 0xa8, 0xa1,
            0x88, 0xaf, 0xa7, 0xcc, 0xd2, 0x81, 0xad, 0x2f, 0x66, 0xa3, 0x07, 0xfb, 0x77, 0x8e, 0x45, 0xb2,
        },
        {
            /* Point with smallest x. (1/2) */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
            0x42, 0x18, 0xf2, 0x0a, 0xe6, 0xc6, 0x46, 0xb3, 0x63, 0xdb, 0x68, 0x60, 0x58, 0x22, 0xfb, 0x14,
            0x26, 0x4c, 0xa8, 0xd2, 0x58, 0x7f, 0xdd, 0x6f, 0xbc, 0x75, 0x0d, 0x58, 0x7e, 0x76, 0xa7, 0xee,
        },
        {
            /* Point with smallest x. (2/2) */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
            0xbd, 0xe7, 0x0d, 0xf5, 0x19, 0x39, 0xb9, 0x4c, 0x9c, 0x24, 0x97, 0x9f, 0xa7, 0xdd, 0x04, 0xeb,
            0xd9, 0xb3, 0x57, 0x2d, 0xa7, 0x80, 0x22, 0x90, 0x43, 0x8a, 0xf2, 0xa6, 0x81, 0x89, 0x54, 0x41,
        },
        {
            /* Point with largest y. (1/3) */
            0x1f, 0xe1, 0xe5, 0xef, 0x3f, 0xce, 0xb5, 0xc1, 0x35, 0xab, 0x77, 0x41, 0x33, 0x3c, 0xe5, 0xa6,
            0xe8, 0x0d, 0x68, 0x16, 0x76, 0x53, 0xf6, 0xb2, 0xb2, 0x4b, 0xcb, 0xcf, 0xaa, 0xaf, 0xf5, 0x07,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x2e,
        },
        {
            /* Point with largest y. (2/3) */
            0xcb, 0xb0, 0xde, 0xab, 0x12, 0x57, 0x54, 0xf1, 0xfd, 0xb2, 0x03, 0x8b, 0x04, 0x34, 0xed, 0x9c,
            0xb3, 0xfb, 0x53, 0xab, 0x73, 0x53, 0x91, 0x12, 0x99, 0x94, 0xa5, 0x35, 0xd9, 0x25, 0xf6, 0x73,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x2e,
        },
        {
            /* Point with largest y. (3/3) */
            0x14, 0x6d, 0x3b, 0x65, 0xad, 0xd9, 0xf5, 0x4c, 0xcc, 0xa2, 0x85, 0x33, 0xc8, 0x8e, 0x2c, 0xbc,
            0x63, 0xf7, 0x44, 0x3e, 0x16, 0x58, 0x78, 0x3a, 0xb4, 0x1f, 0x8e, 0xf9, 0x7c, 0x2a, 0x10, 0xb5,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x2e,
        },
        {
            /* Point with smallest y. (1/3) */
            0x1f, 0xe1, 0xe5, 0xef, 0x3f, 0xce, 0xb5, 0xc1, 0x35, 0xab, 0x77, 0x41, 0x33, 0x3c, 0xe5, 0xa6,
            0xe8, 0x0d, 0x68, 0x16, 0x76, 0x53, 0xf6, 0xb2, 0xb2, 0x4b, 0xcb, 0xcf, 0xaa, 0xaf, 0xf5, 0x07,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        },
        {
            /* Point with smallest y. (2/3) */
            0xcb, 0xb0, 0xde, 0xab, 0x12, 0x57, 0x54, 0xf1, 0xfd, 0xb2, 0x03, 0x8b, 0x04, 0x34, 0xed, 0x9c,
            0xb3, 0xfb, 0x53, 0xab, 0x73, 0x53, 0x91, 0x12, 0x99, 0x94, 0xa5, 0x35, 0xd9, 0x25, 0xf6, 0x73,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        },
        {
            /* Point with smallest y. (3/3) */
            0x14, 0x6d, 0x3b, 0x65, 0xad, 0xd9, 0xf5, 0x4c, 0xcc, 0xa2, 0x85, 0x33, 0xc8, 0x8e, 0x2c, 0xbc,
            0x63, 0xf7, 0x44, 0x3e, 0x16, 0x58, 0x78, 0x3a, 0xb4, 0x1f, 0x8e, 0xf9, 0x7c, 0x2a, 0x10, 0xb5,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
        }
    };
#define SECP256K1_EC_PARSE_TEST_NXVALID (4)
    const unsigned char onlyxvalid[SECP256K1_EC_PARSE_TEST_NXVALID][64] = {
        {
            /* Valid if y overflow ignored (y = 1 mod p). (1/3) */
            0x1f, 0xe1, 0xe5, 0xef, 0x3f, 0xce, 0xb5, 0xc1, 0x35, 0xab, 0x77, 0x41, 0x33, 0x3c, 0xe5, 0xa6,
            0xe8, 0x0d, 0x68, 0x16, 0x76, 0x53, 0xf6, 0xb2, 0xb2, 0x4b, 0xcb, 0xcf, 0xaa, 0xaf, 0xf5, 0x07,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x30,
        },
        {
            /* Valid if y overflow ignored (y = 1 mod p). (2/3) */
            0xcb, 0xb0, 0xde, 0xab, 0x12, 0x57, 0x54, 0xf1, 0xfd, 0xb2, 0x03, 0x8b, 0x04, 0x34, 0xed, 0x9c,
            0xb3, 0xfb, 0x53, 0xab, 0x73, 0x53, 0x91, 0x12, 0x99, 0x94, 0xa5, 0x35, 0xd9, 0x25, 0xf6, 0x73,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x30,
        },
        {
            /* Valid if y overflow ignored (y = 1 mod p). (3/3)*/
            0x14, 0x6d, 0x3b, 0x65, 0xad, 0xd9, 0xf5, 0x4c, 0xcc, 0xa2, 0x85, 0x33, 0xc8, 0x8e, 0x2c, 0xbc,
            0x63, 0xf7, 0x44, 0x3e, 0x16, 0x58, 0x78, 0x3a, 0xb4, 0x1f, 0x8e, 0xf9, 0x7c, 0x2a, 0x10, 0xb5,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x30,
        },
        {
            /* x on curve, y is from y^2 = x^3 + 8. */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03
        }
    };
#define SECP256K1_EC_PARSE_TEST_NINVALID (7)
    const unsigned char invalid[SECP256K1_EC_PARSE_TEST_NINVALID][64] = {
        {
            /* x is third root of -8, y is -1 * (x^3+7); also on the curve for y^2 = x^3 + 9. */
            0x0a, 0x2d, 0x2b, 0xa9, 0x35, 0x07, 0xf1, 0xdf, 0x23, 0x37, 0x70, 0xc2, 0xa7, 0x97, 0x96, 0x2c,
            0xc6, 0x1f, 0x6d, 0x15, 0xda, 0x14, 0xec, 0xd4, 0x7d, 0x8d, 0x27, 0xae, 0x1c, 0xd5, 0xf8, 0x53,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        },
        {
            /* Valid if x overflow ignored (x = 1 mod p). */
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x30,
            0x42, 0x18, 0xf2, 0x0a, 0xe6, 0xc6, 0x46, 0xb3, 0x63, 0xdb, 0x68, 0x60, 0x58, 0x22, 0xfb, 0x14,
            0x26, 0x4c, 0xa8, 0xd2, 0x58, 0x7f, 0xdd, 0x6f, 0xbc, 0x75, 0x0d, 0x58, 0x7e, 0x76, 0xa7, 0xee,
        },
        {
            /* Valid if x overflow ignored (x = 1 mod p). */
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x30,
            0xbd, 0xe7, 0x0d, 0xf5, 0x19, 0x39, 0xb9, 0x4c, 0x9c, 0x24, 0x97, 0x9f, 0xa7, 0xdd, 0x04, 0xeb,
            0xd9, 0xb3, 0x57, 0x2d, 0xa7, 0x80, 0x22, 0x90, 0x43, 0x8a, 0xf2, 0xa6, 0x81, 0x89, 0x54, 0x41,
        },
        {
            /* x is -1, y is the result of the sqrt ladder; also on the curve for y^2 = x^3 - 5. */
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x2e,
            0xf4, 0x84, 0x14, 0x5c, 0xb0, 0x14, 0x9b, 0x82, 0x5d, 0xff, 0x41, 0x2f, 0xa0, 0x52, 0xa8, 0x3f,
            0xcb, 0x72, 0xdb, 0x61, 0xd5, 0x6f, 0x37, 0x70, 0xce, 0x06, 0x6b, 0x73, 0x49, 0xa2, 0xaa, 0x28,
        },
        {
            /* x is -1, y is the result of the sqrt ladder; also on the curve for y^2 = x^3 - 5. */
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x2e,
            0x0b, 0x7b, 0xeb, 0xa3, 0x4f, 0xeb, 0x64, 0x7d, 0xa2, 0x00, 0xbe, 0xd0, 0x5f, 0xad, 0x57, 0xc0,
            0x34, 0x8d, 0x24, 0x9e, 0x2a, 0x90, 0xc8, 0x8f, 0x31, 0xf9, 0x94, 0x8b, 0xb6, 0x5d, 0x52, 0x07,
        },
        {
            /* x is zero, y is the result of the sqrt ladder; also on the curve for y^2 = x^3 - 7. */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x8f, 0x53, 0x7e, 0xef, 0xdf, 0xc1, 0x60, 0x6a, 0x07, 0x27, 0xcd, 0x69, 0xb4, 0xa7, 0x33, 0x3d,
            0x38, 0xed, 0x44, 0xe3, 0x93, 0x2a, 0x71, 0x79, 0xee, 0xcb, 0x4b, 0x6f, 0xba, 0x93, 0x60, 0xdc,
        },
        {
            /* x is zero, y is the result of the sqrt ladder; also on the curve for y^2 = x^3 - 7. */
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x70, 0xac, 0x81, 0x10, 0x20, 0x3e, 0x9f, 0x95, 0xf8, 0xd8, 0x32, 0x96, 0x4b, 0x58, 0xcc, 0xc2,
            0xc7, 0x12, 0xbb, 0x1c, 0x6c, 0xd5, 0x8e, 0x86, 0x11, 0x34, 0xb4, 0x8f, 0x45, 0x6c, 0x9b, 0x53
        }
    };
    const unsigned char pubkeyc[66] = {
        /* Serialization of G. */
        0x04, 0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB, 0xAC, 0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B,
        0x07, 0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28, 0xD9, 0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17,
        0x98, 0x48, 0x3A, 0xDA, 0x77, 0x26, 0xA3, 0xC4, 0x65, 0x5D, 0xA4, 0xFB, 0xFC, 0x0E, 0x11, 0x08,
        0xA8, 0xFD, 0x17, 0xB4, 0x48, 0xA6, 0x85, 0x54, 0x19, 0x9C, 0x47, 0xD0, 0x8F, 0xFB, 0x10, 0xD4,
        0xB8, 0x00
    };
    unsigned char sout[65];
    unsigned char shortkey[2];
    secp256k1_ge ge;
    secp256k1_pubkey pubkey;
    size_t len;
    int32_t i;
    int32_t ecount;
    int32_t ecount2;
    ecount = 0;
    /* Nothing should be reading this far into pubkeyc. */
    VG_UNDEF(&pubkeyc[65], 1);
    secp256k1_context_set_illegal_callback(ctx, counting_illegal_callback_fn, &ecount);
    /* Zero length claimed, fail, zeroize, no illegal arg error. */
    memset(&pubkey, 0xfe, sizeof(pubkey));
    ecount = 0;
    VG_UNDEF(shortkey, 2);
    VG_UNDEF(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, shortkey, 0) == 0);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(ecount == 0);
    CHECK(secp256k1_pubkey_load(ctx, &ge, &pubkey) == 0);
    CHECK(ecount == 1);
    /* Length one claimed, fail, zeroize, no illegal arg error. */
    for (i = 0; i < 256 ; i++) {
        memset(&pubkey, 0xfe, sizeof(pubkey));
        ecount = 0;
        shortkey[0] = i;
        VG_UNDEF(&shortkey[1], 1);
        VG_UNDEF(&pubkey, sizeof(pubkey));
        CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, shortkey, 1) == 0);
        VG_CHECK(&pubkey, sizeof(pubkey));
        CHECK(ecount == 0);
        CHECK(secp256k1_pubkey_load(ctx, &ge, &pubkey) == 0);
        CHECK(ecount == 1);
    }
    /* Length two claimed, fail, zeroize, no illegal arg error. */
    for (i = 0; i < 65536 ; i++) {
        memset(&pubkey, 0xfe, sizeof(pubkey));
        ecount = 0;
        shortkey[0] = i & 255;
        shortkey[1] = i >> 8;
        VG_UNDEF(&pubkey, sizeof(pubkey));
        CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, shortkey, 2) == 0);
        VG_CHECK(&pubkey, sizeof(pubkey));
        CHECK(ecount == 0);
        CHECK(secp256k1_pubkey_load(ctx, &ge, &pubkey) == 0);
        CHECK(ecount == 1);
    }
    memset(&pubkey, 0xfe, sizeof(pubkey));
    ecount = 0;
    VG_UNDEF(&pubkey, sizeof(pubkey));
    /* 33 bytes claimed on otherwise valid input starting with 0x04, fail, zeroize output, no illegal arg error. */
    CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, pubkeyc, 33) == 0);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(ecount == 0);
    CHECK(secp256k1_pubkey_load(ctx, &ge, &pubkey) == 0);
    CHECK(ecount == 1);
    /* NULL pubkey, illegal arg error. Pubkey isn't rewritten before this step, since it's NULL into the parser. */
    CHECK(secp256k1_ec_pubkey_parse(ctx, NULL, pubkeyc, 65) == 0);
    CHECK(ecount == 2);
    /* NULL input string. Illegal arg and zeroize output. */
    memset(&pubkey, 0xfe, sizeof(pubkey));
    ecount = 0;
    VG_UNDEF(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, NULL, 65) == 0);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(ecount == 1);
    CHECK(secp256k1_pubkey_load(ctx, &ge, &pubkey) == 0);
    CHECK(ecount == 2);
    /* 64 bytes claimed on input starting with 0x04, fail, zeroize output, no illegal arg error. */
    memset(&pubkey, 0xfe, sizeof(pubkey));
    ecount = 0;
    VG_UNDEF(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, pubkeyc, 64) == 0);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(ecount == 0);
    CHECK(secp256k1_pubkey_load(ctx, &ge, &pubkey) == 0);
    CHECK(ecount == 1);
    /* 66 bytes claimed, fail, zeroize output, no illegal arg error. */
    memset(&pubkey, 0xfe, sizeof(pubkey));
    ecount = 0;
    VG_UNDEF(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, pubkeyc, 66) == 0);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(ecount == 0);
    CHECK(secp256k1_pubkey_load(ctx, &ge, &pubkey) == 0);
    CHECK(ecount == 1);
    /* Valid parse. */
    memset(&pubkey, 0, sizeof(pubkey));
    ecount = 0;
    VG_UNDEF(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, pubkeyc, 65) == 1);
    CHECK(secp256k1_ec_pubkey_parse(secp256k1_context_no_precomp, &pubkey, pubkeyc, 65) == 1);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(ecount == 0);
    VG_UNDEF(&ge, sizeof(ge));
    CHECK(secp256k1_pubkey_load(ctx, &ge, &pubkey) == 1);
    VG_CHECK(&ge.x, sizeof(ge.x));
    VG_CHECK(&ge.y, sizeof(ge.y));
    VG_CHECK(&ge.infinity, sizeof(ge.infinity));
    ge_equals_ge(&secp256k1_ge_const_g, &ge);
    CHECK(ecount == 0);
    /* secp256k1_ec_pubkey_serialize illegal args. */
    ecount = 0;
    len = 65;
    CHECK(secp256k1_ec_pubkey_serialize(ctx, NULL, &len, &pubkey, SECP256K1_EC_UNCOMPRESSED) == 0);
    CHECK(ecount == 1);
    CHECK(len == 0);
    CHECK(secp256k1_ec_pubkey_serialize(ctx, sout, NULL, &pubkey, SECP256K1_EC_UNCOMPRESSED) == 0);
    CHECK(ecount == 2);
    len = 65;
    VG_UNDEF(sout, 65);
    CHECK(secp256k1_ec_pubkey_serialize(ctx, sout, &len, NULL, SECP256K1_EC_UNCOMPRESSED) == 0);
    VG_CHECK(sout, 65);
    CHECK(ecount == 3);
    CHECK(len == 0);
    len = 65;
    CHECK(secp256k1_ec_pubkey_serialize(ctx, sout, &len, &pubkey, ~0) == 0);
    CHECK(ecount == 4);
    CHECK(len == 0);
    len = 65;
    VG_UNDEF(sout, 65);
    CHECK(secp256k1_ec_pubkey_serialize(ctx, sout, &len, &pubkey, SECP256K1_EC_UNCOMPRESSED) == 1);
    VG_CHECK(sout, 65);
    CHECK(ecount == 4);
    CHECK(len == 65);
    /* Multiple illegal args. Should still set arg error only once. */
    ecount = 0;
    ecount2 = 11;
    CHECK(secp256k1_ec_pubkey_parse(ctx, NULL, NULL, 65) == 0);
    CHECK(ecount == 1);
    /* Does the illegal arg callback actually change the behavior? */
    secp256k1_context_set_illegal_callback(ctx, uncounting_illegal_callback_fn, &ecount2);
    CHECK(secp256k1_ec_pubkey_parse(ctx, NULL, NULL, 65) == 0);
    CHECK(ecount == 1);
    CHECK(ecount2 == 10);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
    /* Try a bunch of prefabbed points with all possible encodings. */
    for (i = 0; i < SECP256K1_EC_PARSE_TEST_NVALID; i++) {
        ec_pubkey_parse_pointtest(valid[i], 1, 1);
    }
    for (i = 0; i < SECP256K1_EC_PARSE_TEST_NXVALID; i++) {
        ec_pubkey_parse_pointtest(onlyxvalid[i], 1, 0);
    }
    for (i = 0; i < SECP256K1_EC_PARSE_TEST_NINVALID; i++) {
        ec_pubkey_parse_pointtest(invalid[i], 0, 0);
    }
}

void run_eckey_edge_case_test(void) {
    const unsigned char orderc[32] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
        0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b,
        0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41
    };
    const unsigned char zeros[sizeof(secp256k1_pubkey)] = {0x00};
    unsigned char ctmp[33];
    unsigned char ctmp2[33];
    secp256k1_pubkey pubkey;
    secp256k1_pubkey pubkey2;
    secp256k1_pubkey pubkey_one;
    secp256k1_pubkey pubkey_negone;
    const secp256k1_pubkey *pubkeys[3];
    size_t len;
    int32_t ecount;
    /* Group order is too large, reject. */
    CHECK(secp256k1_ec_seckey_verify(ctx, orderc) == 0);
    VG_UNDEF(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, orderc) == 0);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) == 0);
    /* Maximum value is too large, reject. */
    memset(ctmp, 255, 32);
    CHECK(secp256k1_ec_seckey_verify(ctx, ctmp) == 0);
    memset(&pubkey, 1, sizeof(pubkey));
    VG_UNDEF(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, ctmp) == 0);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) == 0);
    /* Zero is too small, reject. */
    memset(ctmp, 0, 32);
    CHECK(secp256k1_ec_seckey_verify(ctx, ctmp) == 0);
    memset(&pubkey, 1, sizeof(pubkey));
    VG_UNDEF(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, ctmp) == 0);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) == 0);
    /* One must be accepted. */
    ctmp[31] = 0x01;
    CHECK(secp256k1_ec_seckey_verify(ctx, ctmp) == 1);
    memset(&pubkey, 0, sizeof(pubkey));
    VG_UNDEF(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, ctmp) == 1);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) > 0);
    pubkey_one = pubkey;
    /* Group order + 1 is too large, reject. */
    memcpy(ctmp, orderc, 32);
    ctmp[31] = 0x42;
    CHECK(secp256k1_ec_seckey_verify(ctx, ctmp) == 0);
    memset(&pubkey, 1, sizeof(pubkey));
    VG_UNDEF(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, ctmp) == 0);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) == 0);
    /* -1 must be accepted. */
    ctmp[31] = 0x40;
    CHECK(secp256k1_ec_seckey_verify(ctx, ctmp) == 1);
    memset(&pubkey, 0, sizeof(pubkey));
    VG_UNDEF(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, ctmp) == 1);
    VG_CHECK(&pubkey, sizeof(pubkey));
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) > 0);
    pubkey_negone = pubkey;
    /* Tweak of zero leaves the value unchanged. */
    memset(ctmp2, 0, 32);
    CHECK(secp256k1_ec_seckey_tweak_add(ctx, ctmp, ctmp2) == 1);
    CHECK(secp256k1_memcmp_var(orderc, ctmp, 31) == 0 && ctmp[31] == 0x40);
    memcpy(&pubkey2, &pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_tweak_add(ctx, &pubkey, ctmp2) == 1);
    CHECK(secp256k1_memcmp_var(&pubkey, &pubkey2, sizeof(pubkey)) == 0);
    /* Multiply tweak of zero zeroizes the output. */
    CHECK(secp256k1_ec_seckey_tweak_mul(ctx, ctmp, ctmp2) == 0);
    CHECK(secp256k1_memcmp_var(zeros, ctmp, 32) == 0);
    CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &pubkey, ctmp2) == 0);
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(pubkey)) == 0);
    memcpy(&pubkey, &pubkey2, sizeof(pubkey));
    /* If seckey_tweak_add or seckey_tweak_mul are called with an overflowing
    seckey, the seckey is zeroized. */
    memcpy(ctmp, orderc, 32);
    memset(ctmp2, 0, 32);
    ctmp2[31] = 0x01;
    CHECK(secp256k1_ec_seckey_verify(ctx, ctmp2) == 1);
    CHECK(secp256k1_ec_seckey_verify(ctx, ctmp) == 0);
    CHECK(secp256k1_ec_seckey_tweak_add(ctx, ctmp, ctmp2) == 0);
    CHECK(secp256k1_memcmp_var(zeros, ctmp, 32) == 0);
    memcpy(ctmp, orderc, 32);
    CHECK(secp256k1_ec_seckey_tweak_mul(ctx, ctmp, ctmp2) == 0);
    CHECK(secp256k1_memcmp_var(zeros, ctmp, 32) == 0);
    /* If seckey_tweak_add or seckey_tweak_mul are called with an overflowing
    tweak, the seckey is zeroized. */
    memcpy(ctmp, orderc, 32);
    ctmp[31] = 0x40;
    CHECK(secp256k1_ec_seckey_tweak_add(ctx, ctmp, orderc) == 0);
    CHECK(secp256k1_memcmp_var(zeros, ctmp, 32) == 0);
    memcpy(ctmp, orderc, 32);
    ctmp[31] = 0x40;
    CHECK(secp256k1_ec_seckey_tweak_mul(ctx, ctmp, orderc) == 0);
    CHECK(secp256k1_memcmp_var(zeros, ctmp, 32) == 0);
    memcpy(ctmp, orderc, 32);
    ctmp[31] = 0x40;
    /* If pubkey_tweak_add or pubkey_tweak_mul are called with an overflowing
    tweak, the pubkey is zeroized. */
    CHECK(secp256k1_ec_pubkey_tweak_add(ctx, &pubkey, orderc) == 0);
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(pubkey)) == 0);
    memcpy(&pubkey, &pubkey2, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &pubkey, orderc) == 0);
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(pubkey)) == 0);
    memcpy(&pubkey, &pubkey2, sizeof(pubkey));
    /* If the resulting key in secp256k1_ec_seckey_tweak_add and
     * secp256k1_ec_pubkey_tweak_add is 0 the functions fail and in the latter
     * case the pubkey is zeroized. */
    memcpy(ctmp, orderc, 32);
    ctmp[31] = 0x40;
    memset(ctmp2, 0, 32);
    ctmp2[31] = 1;
    CHECK(secp256k1_ec_seckey_tweak_add(ctx, ctmp2, ctmp) == 0);
    CHECK(secp256k1_memcmp_var(zeros, ctmp2, 32) == 0);
    ctmp2[31] = 1;
    CHECK(secp256k1_ec_pubkey_tweak_add(ctx, &pubkey, ctmp2) == 0);
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(pubkey)) == 0);
    memcpy(&pubkey, &pubkey2, sizeof(pubkey));
    /* Tweak computation wraps and results in a key of 1. */
    ctmp2[31] = 2;
    CHECK(secp256k1_ec_seckey_tweak_add(ctx, ctmp2, ctmp) == 1);
    CHECK(secp256k1_memcmp_var(ctmp2, zeros, 31) == 0 && ctmp2[31] == 1);
    ctmp2[31] = 2;
    CHECK(secp256k1_ec_pubkey_tweak_add(ctx, &pubkey, ctmp2) == 1);
    ctmp2[31] = 1;
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey2, ctmp2) == 1);
    CHECK(secp256k1_memcmp_var(&pubkey, &pubkey2, sizeof(pubkey)) == 0);
    /* Tweak mul * 2 = 1+1. */
    CHECK(secp256k1_ec_pubkey_tweak_add(ctx, &pubkey, ctmp2) == 1);
    ctmp2[31] = 2;
    CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &pubkey2, ctmp2) == 1);
    CHECK(secp256k1_memcmp_var(&pubkey, &pubkey2, sizeof(pubkey)) == 0);
    /* Test argument errors. */
    ecount = 0;
    secp256k1_context_set_illegal_callback(ctx, counting_illegal_callback_fn, &ecount);
    CHECK(ecount == 0);
    /* Zeroize pubkey on parse error. */
    memset(&pubkey, 0, 32);
    CHECK(secp256k1_ec_pubkey_tweak_add(ctx, &pubkey, ctmp2) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(pubkey)) == 0);
    memcpy(&pubkey, &pubkey2, sizeof(pubkey));
    memset(&pubkey2, 0, 32);
    CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &pubkey2, ctmp2) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_memcmp_var(&pubkey2, zeros, sizeof(pubkey2)) == 0);
    /* Plain argument errors. */
    ecount = 0;
    CHECK(secp256k1_ec_seckey_verify(ctx, ctmp) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_ec_seckey_verify(ctx, NULL) == 0);
    CHECK(ecount == 1);
    ecount = 0;
    memset(ctmp2, 0, 32);
    ctmp2[31] = 4;
    CHECK(secp256k1_ec_pubkey_tweak_add(ctx, NULL, ctmp2) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_ec_pubkey_tweak_add(ctx, &pubkey, NULL) == 0);
    CHECK(ecount == 2);
    ecount = 0;
    memset(ctmp2, 0, 32);
    ctmp2[31] = 4;
    CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, NULL, ctmp2) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &pubkey, NULL) == 0);
    CHECK(ecount == 2);
    ecount = 0;
    memset(ctmp2, 0, 32);
    CHECK(secp256k1_ec_seckey_tweak_add(ctx, NULL, ctmp2) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_ec_seckey_tweak_add(ctx, ctmp, NULL) == 0);
    CHECK(ecount == 2);
    ecount = 0;
    memset(ctmp2, 0, 32);
    ctmp2[31] = 1;
    CHECK(secp256k1_ec_seckey_tweak_mul(ctx, NULL, ctmp2) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_ec_seckey_tweak_mul(ctx, ctmp, NULL) == 0);
    CHECK(ecount == 2);
    ecount = 0;
    CHECK(secp256k1_ec_pubkey_create(ctx, NULL, ctmp) == 0);
    CHECK(ecount == 1);
    memset(&pubkey, 1, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, NULL) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) == 0);
    /* secp256k1_ec_pubkey_combine tests. */
    ecount = 0;
    pubkeys[0] = &pubkey_one;
    VG_UNDEF(&pubkeys[0], sizeof(secp256k1_pubkey *));
    VG_UNDEF(&pubkeys[1], sizeof(secp256k1_pubkey *));
    VG_UNDEF(&pubkeys[2], sizeof(secp256k1_pubkey *));
    memset(&pubkey, 255, sizeof(secp256k1_pubkey));
    VG_UNDEF(&pubkey, sizeof(secp256k1_pubkey));
    CHECK(secp256k1_ec_pubkey_combine(ctx, &pubkey, pubkeys, 0) == 0);
    VG_CHECK(&pubkey, sizeof(secp256k1_pubkey));
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_ec_pubkey_combine(ctx, NULL, pubkeys, 1) == 0);
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) == 0);
    CHECK(ecount == 2);
    memset(&pubkey, 255, sizeof(secp256k1_pubkey));
    VG_UNDEF(&pubkey, sizeof(secp256k1_pubkey));
    CHECK(secp256k1_ec_pubkey_combine(ctx, &pubkey, NULL, 1) == 0);
    VG_CHECK(&pubkey, sizeof(secp256k1_pubkey));
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) == 0);
    CHECK(ecount == 3);
    pubkeys[0] = &pubkey_negone;
    memset(&pubkey, 255, sizeof(secp256k1_pubkey));
    VG_UNDEF(&pubkey, sizeof(secp256k1_pubkey));
    CHECK(secp256k1_ec_pubkey_combine(ctx, &pubkey, pubkeys, 1) == 1);
    VG_CHECK(&pubkey, sizeof(secp256k1_pubkey));
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) > 0);
    CHECK(ecount == 3);
    len = 33;
    CHECK(secp256k1_ec_pubkey_serialize(ctx, ctmp, &len, &pubkey, SECP256K1_EC_COMPRESSED) == 1);
    CHECK(secp256k1_ec_pubkey_serialize(ctx, ctmp2, &len, &pubkey_negone, SECP256K1_EC_COMPRESSED) == 1);
    CHECK(secp256k1_memcmp_var(ctmp, ctmp2, 33) == 0);
    /* Result is infinity. */
    pubkeys[0] = &pubkey_one;
    pubkeys[1] = &pubkey_negone;
    memset(&pubkey, 255, sizeof(secp256k1_pubkey));
    VG_UNDEF(&pubkey, sizeof(secp256k1_pubkey));
    CHECK(secp256k1_ec_pubkey_combine(ctx, &pubkey, pubkeys, 2) == 0);
    VG_CHECK(&pubkey, sizeof(secp256k1_pubkey));
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) == 0);
    CHECK(ecount == 3);
    /* Passes through infinity but comes out one. */
    pubkeys[2] = &pubkey_one;
    memset(&pubkey, 255, sizeof(secp256k1_pubkey));
    VG_UNDEF(&pubkey, sizeof(secp256k1_pubkey));
    CHECK(secp256k1_ec_pubkey_combine(ctx, &pubkey, pubkeys, 3) == 1);
    VG_CHECK(&pubkey, sizeof(secp256k1_pubkey));
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) > 0);
    CHECK(ecount == 3);
    len = 33;
    CHECK(secp256k1_ec_pubkey_serialize(ctx, ctmp, &len, &pubkey, SECP256K1_EC_COMPRESSED) == 1);
    CHECK(secp256k1_ec_pubkey_serialize(ctx, ctmp2, &len, &pubkey_one, SECP256K1_EC_COMPRESSED) == 1);
    CHECK(secp256k1_memcmp_var(ctmp, ctmp2, 33) == 0);
    /* Adds to two. */
    pubkeys[1] = &pubkey_one;
    memset(&pubkey, 255, sizeof(secp256k1_pubkey));
    VG_UNDEF(&pubkey, sizeof(secp256k1_pubkey));
    CHECK(secp256k1_ec_pubkey_combine(ctx, &pubkey, pubkeys, 2) == 1);
    VG_CHECK(&pubkey, sizeof(secp256k1_pubkey));
    CHECK(secp256k1_memcmp_var(&pubkey, zeros, sizeof(secp256k1_pubkey)) > 0);
    CHECK(ecount == 3);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

void run_eckey_negate_test(void) {
    unsigned char seckey[32];
    unsigned char seckey_tmp[32];

    random_scalar_order_b32(seckey);
    memcpy(seckey_tmp, seckey, 32);

    /* Verify negation changes the key and changes it back */
    CHECK(secp256k1_ec_seckey_negate(ctx, seckey) == 1);
    CHECK(secp256k1_memcmp_var(seckey, seckey_tmp, 32) != 0);
    CHECK(secp256k1_ec_seckey_negate(ctx, seckey) == 1);
    CHECK(secp256k1_memcmp_var(seckey, seckey_tmp, 32) == 0);

    /* Check that privkey alias gives same result */
    CHECK(secp256k1_ec_seckey_negate(ctx, seckey) == 1);
    CHECK(secp256k1_ec_privkey_negate(ctx, seckey_tmp) == 1);
    CHECK(secp256k1_memcmp_var(seckey, seckey_tmp, 32) == 0);

    /* Negating all 0s fails */
    memset(seckey, 0, 32);
    memset(seckey_tmp, 0, 32);
    CHECK(secp256k1_ec_seckey_negate(ctx, seckey) == 0);
    /* Check that seckey is not modified */
    CHECK(secp256k1_memcmp_var(seckey, seckey_tmp, 32) == 0);

    /* Negating an overflowing seckey fails and the seckey is zeroed. In this
     * test, the seckey has 16 random bytes to ensure that ec_seckey_negate
     * doesn't just set seckey to a constant value in case of failure. */
    random_scalar_order_b32(seckey);
    memset(seckey, 0xFF, 16);
    memset(seckey_tmp, 0, 32);
    CHECK(secp256k1_ec_seckey_negate(ctx, seckey) == 0);
    CHECK(secp256k1_memcmp_var(seckey, seckey_tmp, 32) == 0);
}

void random_sign(secp256k1_scalar *sigr, secp256k1_scalar *sigs, const secp256k1_scalar *key, const secp256k1_scalar *msg, int *recid) {
    secp256k1_scalar nonce;
    do {
        random_scalar_order_test(&nonce);
    } while(!secp256k1_ecdsa_sig_sign(&ctx->ecmult_gen_ctx, sigr, sigs, key, msg, &nonce, recid));
}

void test_ecdsa_sign_verify(void) {
    secp256k1_gej pubj;
    secp256k1_ge pub;
    secp256k1_scalar one;
    secp256k1_scalar msg, key;
    secp256k1_scalar sigr, sigs;
    int getrec;
    /* Initialize recid to suppress a false positive -Wconditional-uninitialized in clang.
       VG_UNDEF ensures that valgrind will still treat the variable as uninitialized. */
    int recid = -1; VG_UNDEF(&recid, sizeof(recid));
    random_scalar_order_test(&msg);
    random_scalar_order_test(&key);
    secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &pubj, &key);
    secp256k1_ge_set_gej(&pub, &pubj);
    getrec = secp256k1_testrand_bits(1);
    random_sign(&sigr, &sigs, &key, &msg, getrec?&recid:NULL);
    if (getrec) {
        CHECK(recid >= 0 && recid < 4);
    }
    CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sigr, &sigs, &pub, &msg));
    secp256k1_scalar_set_int(&one, 1);
    secp256k1_scalar_add(&msg, &msg, &one);
    CHECK(!secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sigr, &sigs, &pub, &msg));
}

void run_ecdsa_sign_verify(void) {
    int i;
    for (i = 0; i < 10*count; i++) {
        test_ecdsa_sign_verify();
    }
}

/** Dummy nonce generation function that just uses a precomputed nonce, and fails if it is not accepted. Use only for testing. */
static int precomputed_nonce_function(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int counter) {
    (void)msg32;
    (void)key32;
    (void)algo16;
    memcpy(nonce32, data, 32);
    return (counter == 0);
}

static int nonce_function_test_fail(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int counter) {
   /* Dummy nonce generator that has a fatal error on the first counter value. */
   if (counter == 0) {
       return 0;
   }
   return nonce_function_rfc6979(nonce32, msg32, key32, algo16, data, counter - 1);
}

static int nonce_function_test_retry(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int counter) {
   /* Dummy nonce generator that produces unacceptable nonces for the first several counter values. */
   if (counter < 3) {
       memset(nonce32, counter==0 ? 0 : 255, 32);
       if (counter == 2) {
           nonce32[31]--;
       }
       return 1;
   }
   if (counter < 5) {
       static const unsigned char order[] = {
           0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
           0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
           0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,
           0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x41
       };
       memcpy(nonce32, order, 32);
       if (counter == 4) {
           nonce32[31]++;
       }
       return 1;
   }
   /* Retry rate of 6979 is negligible esp. as we only call this in deterministic tests. */
   /* If someone does fine a case where it retries for secp256k1, we'd like to know. */
   if (counter > 5) {
       return 0;
   }
   return nonce_function_rfc6979(nonce32, msg32, key32, algo16, data, counter - 5);
}

int is_empty_signature(const secp256k1_ecdsa_signature *sig) {
    static const unsigned char res[sizeof(secp256k1_ecdsa_signature)] = {0};
    return secp256k1_memcmp_var(sig, res, sizeof(secp256k1_ecdsa_signature)) == 0;
}

void test_ecdsa_end_to_end(void) {
    unsigned char extra[32] = {0x00};
    unsigned char privkey[32];
    unsigned char message[32];
    unsigned char privkey2[32];
    secp256k1_ecdsa_signature signature[6];
    secp256k1_scalar r, s;
    unsigned char sig[74];
    size_t siglen = 74;
    unsigned char pubkeyc[65];
    size_t pubkeyclen = 65;
    secp256k1_pubkey pubkey;
    secp256k1_pubkey pubkey_tmp;
    unsigned char seckey[300];
    size_t seckeylen = 300;

    /* Generate a random key and message. */
    {
        secp256k1_scalar msg, key;
        random_scalar_order_test(&msg);
        random_scalar_order_test(&key);
        secp256k1_scalar_get_b32(privkey, &key);
        secp256k1_scalar_get_b32(message, &msg);
    }

    /* Construct and verify corresponding public key. */
    CHECK(secp256k1_ec_seckey_verify(ctx, privkey) == 1);
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, privkey) == 1);

    /* Verify exporting and importing public key. */
    CHECK(secp256k1_ec_pubkey_serialize(ctx, pubkeyc, &pubkeyclen, &pubkey, secp256k1_testrand_bits(1) == 1 ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED));
    memset(&pubkey, 0, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, pubkeyc, pubkeyclen) == 1);

    /* Verify negation changes the key and changes it back */
    memcpy(&pubkey_tmp, &pubkey, sizeof(pubkey));
    CHECK(secp256k1_ec_pubkey_negate(ctx, &pubkey_tmp) == 1);
    CHECK(secp256k1_memcmp_var(&pubkey_tmp, &pubkey, sizeof(pubkey)) != 0);
    CHECK(secp256k1_ec_pubkey_negate(ctx, &pubkey_tmp) == 1);
    CHECK(secp256k1_memcmp_var(&pubkey_tmp, &pubkey, sizeof(pubkey)) == 0);

    /* Verify private key import and export. */
    CHECK(ec_privkey_export_der(ctx, seckey, &seckeylen, privkey, secp256k1_testrand_bits(1) == 1));
    CHECK(ec_privkey_import_der(ctx, privkey2, seckey, seckeylen) == 1);
    CHECK(secp256k1_memcmp_var(privkey, privkey2, 32) == 0);

    /* Optionally tweak the keys using addition. */
    if (secp256k1_testrand_int(3) == 0) {
        int ret1;
        int ret2;
        int ret3;
        unsigned char rnd[32];
        unsigned char privkey_tmp[32];
        secp256k1_pubkey pubkey2;
        secp256k1_testrand256_test(rnd);
        memcpy(privkey_tmp, privkey, 32);
        ret1 = secp256k1_ec_seckey_tweak_add(ctx, privkey, rnd);
        ret2 = secp256k1_ec_pubkey_tweak_add(ctx, &pubkey, rnd);
        /* Check that privkey alias gives same result */
        ret3 = secp256k1_ec_privkey_tweak_add(ctx, privkey_tmp, rnd);
        CHECK(ret1 == ret2);
        CHECK(ret2 == ret3);
        if (ret1 == 0) {
            return;
        }
        CHECK(secp256k1_memcmp_var(privkey, privkey_tmp, 32) == 0);
        CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey2, privkey) == 1);
        CHECK(secp256k1_memcmp_var(&pubkey, &pubkey2, sizeof(pubkey)) == 0);
    }

    /* Optionally tweak the keys using multiplication. */
    if (secp256k1_testrand_int(3) == 0) {
        int ret1;
        int ret2;
        int ret3;
        unsigned char rnd[32];
        unsigned char privkey_tmp[32];
        secp256k1_pubkey pubkey2;
        secp256k1_testrand256_test(rnd);
        memcpy(privkey_tmp, privkey, 32);
        ret1 = secp256k1_ec_seckey_tweak_mul(ctx, privkey, rnd);
        ret2 = secp256k1_ec_pubkey_tweak_mul(ctx, &pubkey, rnd);
        /* Check that privkey alias gives same result */
        ret3 = secp256k1_ec_privkey_tweak_mul(ctx, privkey_tmp, rnd);
        CHECK(ret1 == ret2);
        CHECK(ret2 == ret3);
        if (ret1 == 0) {
            return;
        }
        CHECK(secp256k1_memcmp_var(privkey, privkey_tmp, 32) == 0);
        CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey2, privkey) == 1);
        CHECK(secp256k1_memcmp_var(&pubkey, &pubkey2, sizeof(pubkey)) == 0);
    }

    /* Sign. */
    CHECK(secp256k1_ecdsa_sign(ctx, &signature[0], message, privkey, NULL, NULL) == 1);
    CHECK(secp256k1_ecdsa_sign(ctx, &signature[4], message, privkey, NULL, NULL) == 1);
    CHECK(secp256k1_ecdsa_sign(ctx, &signature[1], message, privkey, NULL, extra) == 1);
    extra[31] = 1;
    CHECK(secp256k1_ecdsa_sign(ctx, &signature[2], message, privkey, NULL, extra) == 1);
    extra[31] = 0;
    extra[0] = 1;
    CHECK(secp256k1_ecdsa_sign(ctx, &signature[3], message, privkey, NULL, extra) == 1);
    CHECK(secp256k1_memcmp_var(&signature[0], &signature[4], sizeof(signature[0])) == 0);
    CHECK(secp256k1_memcmp_var(&signature[0], &signature[1], sizeof(signature[0])) != 0);
    CHECK(secp256k1_memcmp_var(&signature[0], &signature[2], sizeof(signature[0])) != 0);
    CHECK(secp256k1_memcmp_var(&signature[0], &signature[3], sizeof(signature[0])) != 0);
    CHECK(secp256k1_memcmp_var(&signature[1], &signature[2], sizeof(signature[0])) != 0);
    CHECK(secp256k1_memcmp_var(&signature[1], &signature[3], sizeof(signature[0])) != 0);
    CHECK(secp256k1_memcmp_var(&signature[2], &signature[3], sizeof(signature[0])) != 0);
    /* Verify. */
    CHECK(secp256k1_ecdsa_verify(ctx, &signature[0], message, &pubkey) == 1);
    CHECK(secp256k1_ecdsa_verify(ctx, &signature[1], message, &pubkey) == 1);
    CHECK(secp256k1_ecdsa_verify(ctx, &signature[2], message, &pubkey) == 1);
    CHECK(secp256k1_ecdsa_verify(ctx, &signature[3], message, &pubkey) == 1);
    /* Test lower-S form, malleate, verify and fail, test again, malleate again */
    CHECK(!secp256k1_ecdsa_signature_normalize(ctx, NULL, &signature[0]));
    secp256k1_ecdsa_signature_load(ctx, &r, &s, &signature[0]);
    secp256k1_scalar_negate(&s, &s);
    secp256k1_ecdsa_signature_save(&signature[5], &r, &s);
    CHECK(secp256k1_ecdsa_verify(ctx, &signature[5], message, &pubkey) == 0);
    CHECK(secp256k1_ecdsa_signature_normalize(ctx, NULL, &signature[5]));
    CHECK(secp256k1_ecdsa_signature_normalize(ctx, &signature[5], &signature[5]));
    CHECK(!secp256k1_ecdsa_signature_normalize(ctx, NULL, &signature[5]));
    CHECK(!secp256k1_ecdsa_signature_normalize(ctx, &signature[5], &signature[5]));
    CHECK(secp256k1_ecdsa_verify(ctx, &signature[5], message, &pubkey) == 1);
    secp256k1_scalar_negate(&s, &s);
    secp256k1_ecdsa_signature_save(&signature[5], &r, &s);
    CHECK(!secp256k1_ecdsa_signature_normalize(ctx, NULL, &signature[5]));
    CHECK(secp256k1_ecdsa_verify(ctx, &signature[5], message, &pubkey) == 1);
    CHECK(secp256k1_memcmp_var(&signature[5], &signature[0], 64) == 0);

    /* Serialize/parse DER and verify again */
    CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, sig, &siglen, &signature[0]) == 1);
    memset(&signature[0], 0, sizeof(signature[0]));
    CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &signature[0], sig, siglen) == 1);
    CHECK(secp256k1_ecdsa_verify(ctx, &signature[0], message, &pubkey) == 1);
    /* Serialize/destroy/parse DER and verify again. */
    siglen = 74;
    CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, sig, &siglen, &signature[0]) == 1);
    sig[secp256k1_testrand_int(siglen)] += 1 + secp256k1_testrand_int(255);
    CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &signature[0], sig, siglen) == 0 ||
          secp256k1_ecdsa_verify(ctx, &signature[0], message, &pubkey) == 0);
}

void test_random_pubkeys(void) {
    secp256k1_ge elem;
    secp256k1_ge elem2;
    unsigned char in[65];
    /* Generate some randomly sized pubkeys. */
    size_t len = secp256k1_testrand_bits(2) == 0 ? 65 : 33;
    if (secp256k1_testrand_bits(2) == 0) {
        len = secp256k1_testrand_bits(6);
    }
    if (len == 65) {
      in[0] = secp256k1_testrand_bits(1) ? 4 : (secp256k1_testrand_bits(1) ? 6 : 7);
    } else {
      in[0] = secp256k1_testrand_bits(1) ? 2 : 3;
    }
    if (secp256k1_testrand_bits(3) == 0) {
        in[0] = secp256k1_testrand_bits(8);
    }
    if (len > 1) {
        secp256k1_testrand256(&in[1]);
    }
    if (len > 33) {
        secp256k1_testrand256(&in[33]);
    }
    if (secp256k1_eckey_pubkey_parse(&elem, in, len)) {
        unsigned char out[65];
        unsigned char firstb;
        int res;
        size_t size = len;
        firstb = in[0];
        /* If the pubkey can be parsed, it should round-trip... */
        CHECK(secp256k1_eckey_pubkey_serialize(&elem, out, &size, len == 33));
        CHECK(size == len);
        CHECK(secp256k1_memcmp_var(&in[1], &out[1], len-1) == 0);
        /* ... except for the type of hybrid inputs. */
        if ((in[0] != 6) && (in[0] != 7)) {
            CHECK(in[0] == out[0]);
        }
        size = 65;
        CHECK(secp256k1_eckey_pubkey_serialize(&elem, in, &size, 0));
        CHECK(size == 65);
        CHECK(secp256k1_eckey_pubkey_parse(&elem2, in, size));
        ge_equals_ge(&elem,&elem2);
        /* Check that the X9.62 hybrid type is checked. */
        in[0] = secp256k1_testrand_bits(1) ? 6 : 7;
        res = secp256k1_eckey_pubkey_parse(&elem2, in, size);
        if (firstb == 2 || firstb == 3) {
            if (in[0] == firstb + 4) {
              CHECK(res);
            } else {
              CHECK(!res);
            }
        }
        if (res) {
            ge_equals_ge(&elem,&elem2);
            CHECK(secp256k1_eckey_pubkey_serialize(&elem, out, &size, 0));
            CHECK(secp256k1_memcmp_var(&in[1], &out[1], 64) == 0);
        }
    }
}

void run_pubkey_comparison(void) {
    unsigned char pk1_ser[33] = {
        0x02,
        0x58, 0x84, 0xb3, 0xa2, 0x4b, 0x97, 0x37, 0x88, 0x92, 0x38, 0xa6, 0x26, 0x62, 0x52, 0x35, 0x11,
        0xd0, 0x9a, 0xa1, 0x1b, 0x80, 0x0b, 0x5e, 0x93, 0x80, 0x26, 0x11, 0xef, 0x67, 0x4b, 0xd9, 0x23
    };
    const unsigned char pk2_ser[33] = {
        0x02,
        0xde, 0x36, 0x0e, 0x87, 0x59, 0x8f, 0x3c, 0x01, 0x36, 0x2a, 0x2a, 0xb8, 0xc6, 0xf4, 0x5e, 0x4d,
        0xb2, 0xc2, 0xd5, 0x03, 0xa7, 0xf9, 0xf1, 0x4f, 0xa8, 0xfa, 0x95, 0xa8, 0xe9, 0x69, 0x76, 0x1c
    };
    secp256k1_pubkey pk1;
    secp256k1_pubkey pk2;
    int32_t ecount = 0;

    CHECK(secp256k1_ec_pubkey_parse(ctx, &pk1, pk1_ser, sizeof(pk1_ser)) == 1);
    CHECK(secp256k1_ec_pubkey_parse(ctx, &pk2, pk2_ser, sizeof(pk2_ser)) == 1);

    secp256k1_context_set_illegal_callback(ctx, counting_illegal_callback_fn, &ecount);
    CHECK(secp256k1_ec_pubkey_cmp(ctx, NULL, &pk2) < 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_ec_pubkey_cmp(ctx, &pk1, NULL) > 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_ec_pubkey_cmp(ctx, &pk1, &pk2) < 0);
    CHECK(secp256k1_ec_pubkey_cmp(ctx, &pk2, &pk1) > 0);
    CHECK(secp256k1_ec_pubkey_cmp(ctx, &pk1, &pk1) == 0);
    CHECK(secp256k1_ec_pubkey_cmp(ctx, &pk2, &pk2) == 0);
    CHECK(ecount == 2);
    {
        secp256k1_pubkey pk_tmp;
        memset(&pk_tmp, 0, sizeof(pk_tmp)); /* illegal pubkey */
        CHECK(secp256k1_ec_pubkey_cmp(ctx, &pk_tmp, &pk2) < 0);
        CHECK(ecount == 3);
        CHECK(secp256k1_ec_pubkey_cmp(ctx, &pk_tmp, &pk_tmp) == 0);
        CHECK(ecount == 5);
        CHECK(secp256k1_ec_pubkey_cmp(ctx, &pk2, &pk_tmp) > 0);
        CHECK(ecount == 6);
    }

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);

    /* Make pk2 the same as pk1 but with 3 rather than 2. Note that in
     * an uncompressed encoding, these would have the opposite ordering */
    pk1_ser[0] = 3;
    CHECK(secp256k1_ec_pubkey_parse(ctx, &pk2, pk1_ser, sizeof(pk1_ser)) == 1);
    CHECK(secp256k1_ec_pubkey_cmp(ctx, &pk1, &pk2) < 0);
    CHECK(secp256k1_ec_pubkey_cmp(ctx, &pk2, &pk1) > 0);
}

void run_random_pubkeys(void) {
    int i;
    for (i = 0; i < 10*count; i++) {
        test_random_pubkeys();
    }
}

void run_ecdsa_end_to_end(void) {
    int i;
    for (i = 0; i < 64*count; i++) {
        test_ecdsa_end_to_end();
    }
}

int test_ecdsa_der_parse(const unsigned char *sig, size_t siglen, int certainly_der, int certainly_not_der) {
    static const unsigned char zeroes[32] = {0};
#ifdef ENABLE_OPENSSL_TESTS
    static const unsigned char max_scalar[32] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
        0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b,
        0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x40
    };
#endif

    int ret = 0;

    secp256k1_ecdsa_signature sig_der;
    unsigned char roundtrip_der[2048];
    unsigned char compact_der[64];
    size_t len_der = 2048;
    int parsed_der = 0, valid_der = 0, roundtrips_der = 0;

    secp256k1_ecdsa_signature sig_der_lax;
    unsigned char roundtrip_der_lax[2048];
    unsigned char compact_der_lax[64];
    size_t len_der_lax = 2048;
    int parsed_der_lax = 0, valid_der_lax = 0, roundtrips_der_lax = 0;

#ifdef ENABLE_OPENSSL_TESTS
    ECDSA_SIG *sig_openssl;
    const BIGNUM *r = NULL, *s = NULL;
    const unsigned char *sigptr;
    unsigned char roundtrip_openssl[2048];
    int len_openssl = 2048;
    int parsed_openssl, valid_openssl = 0, roundtrips_openssl = 0;
#endif

    parsed_der = secp256k1_ecdsa_signature_parse_der(ctx, &sig_der, sig, siglen);
    if (parsed_der) {
        ret |= (!secp256k1_ecdsa_signature_serialize_compact(ctx, compact_der, &sig_der)) << 0;
        valid_der = (secp256k1_memcmp_var(compact_der, zeroes, 32) != 0) && (secp256k1_memcmp_var(compact_der + 32, zeroes, 32) != 0);
    }
    if (valid_der) {
        ret |= (!secp256k1_ecdsa_signature_serialize_der(ctx, roundtrip_der, &len_der, &sig_der)) << 1;
        roundtrips_der = (len_der == siglen) && secp256k1_memcmp_var(roundtrip_der, sig, siglen) == 0;
    }

    parsed_der_lax = ecdsa_signature_parse_der_lax(ctx, &sig_der_lax, sig, siglen);
    if (parsed_der_lax) {
        ret |= (!secp256k1_ecdsa_signature_serialize_compact(ctx, compact_der_lax, &sig_der_lax)) << 10;
        valid_der_lax = (secp256k1_memcmp_var(compact_der_lax, zeroes, 32) != 0) && (secp256k1_memcmp_var(compact_der_lax + 32, zeroes, 32) != 0);
    }
    if (valid_der_lax) {
        ret |= (!secp256k1_ecdsa_signature_serialize_der(ctx, roundtrip_der_lax, &len_der_lax, &sig_der_lax)) << 11;
        roundtrips_der_lax = (len_der_lax == siglen) && secp256k1_memcmp_var(roundtrip_der_lax, sig, siglen) == 0;
    }

    if (certainly_der) {
        ret |= (!parsed_der) << 2;
    }
    if (certainly_not_der) {
        ret |= (parsed_der) << 17;
    }
    if (valid_der) {
        ret |= (!roundtrips_der) << 3;
    }

    if (valid_der) {
        ret |= (!roundtrips_der_lax) << 12;
        ret |= (len_der != len_der_lax) << 13;
        ret |= ((len_der != len_der_lax) || (secp256k1_memcmp_var(roundtrip_der_lax, roundtrip_der, len_der) != 0)) << 14;
    }
    ret |= (roundtrips_der != roundtrips_der_lax) << 15;
    if (parsed_der) {
        ret |= (!parsed_der_lax) << 16;
    }

#ifdef ENABLE_OPENSSL_TESTS
    sig_openssl = ECDSA_SIG_new();
    sigptr = sig;
    parsed_openssl = (d2i_ECDSA_SIG(&sig_openssl, &sigptr, siglen) != NULL);
    if (parsed_openssl) {
        ECDSA_SIG_get0(sig_openssl, &r, &s);
        valid_openssl = !BN_is_negative(r) && !BN_is_negative(s) && BN_num_bits(r) > 0 && BN_num_bits(r) <= 256 && BN_num_bits(s) > 0 && BN_num_bits(s) <= 256;
        if (valid_openssl) {
            unsigned char tmp[32] = {0};
            BN_bn2bin(r, tmp + 32 - BN_num_bytes(r));
            valid_openssl = secp256k1_memcmp_var(tmp, max_scalar, 32) < 0;
        }
        if (valid_openssl) {
            unsigned char tmp[32] = {0};
            BN_bn2bin(s, tmp + 32 - BN_num_bytes(s));
            valid_openssl = secp256k1_memcmp_var(tmp, max_scalar, 32) < 0;
        }
    }
    len_openssl = i2d_ECDSA_SIG(sig_openssl, NULL);
    if (len_openssl <= 2048) {
        unsigned char *ptr = roundtrip_openssl;
        CHECK(i2d_ECDSA_SIG(sig_openssl, &ptr) == len_openssl);
        roundtrips_openssl = valid_openssl && ((size_t)len_openssl == siglen) && (secp256k1_memcmp_var(roundtrip_openssl, sig, siglen) == 0);
    } else {
        len_openssl = 0;
    }
    ECDSA_SIG_free(sig_openssl);

    ret |= (parsed_der && !parsed_openssl) << 4;
    ret |= (valid_der && !valid_openssl) << 5;
    ret |= (roundtrips_openssl && !parsed_der) << 6;
    ret |= (roundtrips_der != roundtrips_openssl) << 7;
    if (roundtrips_openssl) {
        ret |= (len_der != (size_t)len_openssl) << 8;
        ret |= ((len_der != (size_t)len_openssl) || (secp256k1_memcmp_var(roundtrip_der, roundtrip_openssl, len_der) != 0)) << 9;
    }
#endif
    return ret;
}

static void assign_big_endian(unsigned char *ptr, size_t ptrlen, uint32_t val) {
    size_t i;
    for (i = 0; i < ptrlen; i++) {
        int shift = ptrlen - 1 - i;
        if (shift >= 4) {
            ptr[i] = 0;
        } else {
            ptr[i] = (val >> shift) & 0xFF;
        }
    }
}

static void damage_array(unsigned char *sig, size_t *len) {
    int pos;
    int action = secp256k1_testrand_bits(3);
    if (action < 1 && *len > 3) {
        /* Delete a byte. */
        pos = secp256k1_testrand_int(*len);
        memmove(sig + pos, sig + pos + 1, *len - pos - 1);
        (*len)--;
        return;
    } else if (action < 2 && *len < 2048) {
        /* Insert a byte. */
        pos = secp256k1_testrand_int(1 + *len);
        memmove(sig + pos + 1, sig + pos, *len - pos);
        sig[pos] = secp256k1_testrand_bits(8);
        (*len)++;
        return;
    } else if (action < 4) {
        /* Modify a byte. */
        sig[secp256k1_testrand_int(*len)] += 1 + secp256k1_testrand_int(255);
        return;
    } else { /* action < 8 */
        /* Modify a bit. */
        sig[secp256k1_testrand_int(*len)] ^= 1 << secp256k1_testrand_bits(3);
        return;
    }
}

static void random_ber_signature(unsigned char *sig, size_t *len, int* certainly_der, int* certainly_not_der) {
    int der;
    int nlow[2], nlen[2], nlenlen[2], nhbit[2], nhbyte[2], nzlen[2];
    size_t tlen, elen, glen;
    int indet;
    int n;

    *len = 0;
    der = secp256k1_testrand_bits(2) == 0;
    *certainly_der = der;
    *certainly_not_der = 0;
    indet = der ? 0 : secp256k1_testrand_int(10) == 0;

    for (n = 0; n < 2; n++) {
        /* We generate two classes of numbers: nlow==1 "low" ones (up to 32 bytes), nlow==0 "high" ones (32 bytes with 129 top bits set, or larger than 32 bytes) */
        nlow[n] = der ? 1 : (secp256k1_testrand_bits(3) != 0);
        /* The length of the number in bytes (the first byte of which will always be nonzero) */
        nlen[n] = nlow[n] ? secp256k1_testrand_int(33) : 32 + secp256k1_testrand_int(200) * secp256k1_testrand_int(8) / 8;
        CHECK(nlen[n] <= 232);
        /* The top bit of the number. */
        nhbit[n] = (nlow[n] == 0 && nlen[n] == 32) ? 1 : (nlen[n] == 0 ? 0 : secp256k1_testrand_bits(1));
        /* The top byte of the number (after the potential hardcoded 16 0xFF characters for "high" 32 bytes numbers) */
        nhbyte[n] = nlen[n] == 0 ? 0 : (nhbit[n] ? 128 + secp256k1_testrand_bits(7) : 1 + secp256k1_testrand_int(127));
        /* The number of zero bytes in front of the number (which is 0 or 1 in case of DER, otherwise we extend up to 300 bytes) */
        nzlen[n] = der ? ((nlen[n] == 0 || nhbit[n]) ? 1 : 0) : (nlow[n] ? secp256k1_testrand_int(3) : secp256k1_testrand_int(300 - nlen[n]) * secp256k1_testrand_int(8) / 8);
        if (nzlen[n] > ((nlen[n] == 0 || nhbit[n]) ? 1 : 0)) {
            *certainly_not_der = 1;
        }
        CHECK(nlen[n] + nzlen[n] <= 300);
        /* The length of the length descriptor for the number. 0 means short encoding, anything else is long encoding. */
        nlenlen[n] = nlen[n] + nzlen[n] < 128 ? 0 : (nlen[n] + nzlen[n] < 256 ? 1 : 2);
        if (!der) {
            /* nlenlen[n] max 127 bytes */
            int add = secp256k1_testrand_int(127 - nlenlen[n]) * secp256k1_testrand_int(16) * secp256k1_testrand_int(16) / 256;
            nlenlen[n] += add;
            if (add != 0) {
                *certainly_not_der = 1;
            }
        }
        CHECK(nlen[n] + nzlen[n] + nlenlen[n] <= 427);
    }

    /* The total length of the data to go, so far */
    tlen = 2 + nlenlen[0] + nlen[0] + nzlen[0] + 2 + nlenlen[1] + nlen[1] + nzlen[1];
    CHECK(tlen <= 856);

    /* The length of the garbage inside the tuple. */
    elen = (der || indet) ? 0 : secp256k1_testrand_int(980 - tlen) * secp256k1_testrand_int(8) / 8;
    if (elen != 0) {
        *certainly_not_der = 1;
    }
    tlen += elen;
    CHECK(tlen <= 980);

    /* The length of the garbage after the end of the tuple. */
    glen = der ? 0 : secp256k1_testrand_int(990 - tlen) * secp256k1_testrand_int(8) / 8;
    if (glen != 0) {
        *certainly_not_der = 1;
    }
    CHECK(tlen + glen <= 990);

    /* Write the tuple header. */
    sig[(*len)++] = 0x30;
    if (indet) {
        /* Indeterminate length */
        sig[(*len)++] = 0x80;
        *certainly_not_der = 1;
    } else {
        int tlenlen = tlen < 128 ? 0 : (tlen < 256 ? 1 : 2);
        if (!der) {
            int add = secp256k1_testrand_int(127 - tlenlen) * secp256k1_testrand_int(16) * secp256k1_testrand_int(16) / 256;
            tlenlen += add;
            if (add != 0) {
                *certainly_not_der = 1;
            }
        }
        if (tlenlen == 0) {
            /* Short length notation */
            sig[(*len)++] = tlen;
        } else {
            /* Long length notation */
            sig[(*len)++] = 128 + tlenlen;
            assign_big_endian(sig + *len, tlenlen, tlen);
            *len += tlenlen;
        }
        tlen += tlenlen;
    }
    tlen += 2;
    CHECK(tlen + glen <= 1119);

    for (n = 0; n < 2; n++) {
        /* Write the integer header. */
        sig[(*len)++] = 0x02;
        if (nlenlen[n] == 0) {
            /* Short length notation */
            sig[(*len)++] = nlen[n] + nzlen[n];
        } else {
            /* Long length notation. */
            sig[(*len)++] = 128 + nlenlen[n];
            assign_big_endian(sig + *len, nlenlen[n], nlen[n] + nzlen[n]);
            *len += nlenlen[n];
        }
        /* Write zero padding */
        while (nzlen[n] > 0) {
            sig[(*len)++] = 0x00;
            nzlen[n]--;
        }
        if (nlen[n] == 32 && !nlow[n]) {
            /* Special extra 16 0xFF bytes in "high" 32-byte numbers */
            int i;
            for (i = 0; i < 16; i++) {
                sig[(*len)++] = 0xFF;
            }
            nlen[n] -= 16;
        }
        /* Write first byte of number */
        if (nlen[n] > 0) {
            sig[(*len)++] = nhbyte[n];
            nlen[n]--;
        }
        /* Generate remaining random bytes of number */
        secp256k1_testrand_bytes_test(sig + *len, nlen[n]);
        *len += nlen[n];
        nlen[n] = 0;
    }

    /* Generate random garbage inside tuple. */
    secp256k1_testrand_bytes_test(sig + *len, elen);
    *len += elen;

    /* Generate end-of-contents bytes. */
    if (indet) {
        sig[(*len)++] = 0;
        sig[(*len)++] = 0;
        tlen += 2;
    }
    CHECK(tlen + glen <= 1121);

    /* Generate random garbage outside tuple. */
    secp256k1_testrand_bytes_test(sig + *len, glen);
    *len += glen;
    tlen += glen;
    CHECK(tlen <= 1121);
    CHECK(tlen == *len);
}

void run_ecdsa_der_parse(void) {
    int i,j;
    for (i = 0; i < 200 * count; i++) {
        unsigned char buffer[2048];
        size_t buflen = 0;
        int certainly_der = 0;
        int certainly_not_der = 0;
        random_ber_signature(buffer, &buflen, &certainly_der, &certainly_not_der);
        CHECK(buflen <= 2048);
        for (j = 0; j < 16; j++) {
            int ret = 0;
            if (j > 0) {
                damage_array(buffer, &buflen);
                /* We don't know anything anymore about the DERness of the result */
                certainly_der = 0;
                certainly_not_der = 0;
            }
            ret = test_ecdsa_der_parse(buffer, buflen, certainly_der, certainly_not_der);
            if (ret != 0) {
                size_t k;
                fprintf(stderr, "Failure %x on ", ret);
                for (k = 0; k < buflen; k++) {
                    fprintf(stderr, "%02x ", buffer[k]);
                }
                fprintf(stderr, "\n");
            }
            CHECK(ret == 0);
        }
    }
}

/* Tests several edge cases. */
void test_ecdsa_edge_cases(void) {
    int t;
    secp256k1_ecdsa_signature sig;

    /* Test the case where ECDSA recomputes a point that is infinity. */
    {
        secp256k1_gej keyj;
        secp256k1_ge key;
        secp256k1_scalar msg;
        secp256k1_scalar sr, ss;
        secp256k1_scalar_set_int(&ss, 1);
        secp256k1_scalar_negate(&ss, &ss);
        secp256k1_scalar_inverse(&ss, &ss);
        secp256k1_scalar_set_int(&sr, 1);
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &keyj, &sr);
        secp256k1_ge_set_gej(&key, &keyj);
        msg = ss;
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key, &msg) == 0);
    }

    /* Verify signature with r of zero fails. */
    {
        const unsigned char pubkey_mods_zero[33] = {
            0x02, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xfe, 0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0,
            0x3b, 0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41,
            0x41
        };
        secp256k1_ge key;
        secp256k1_scalar msg;
        secp256k1_scalar sr, ss;
        secp256k1_scalar_set_int(&ss, 1);
        secp256k1_scalar_set_int(&msg, 0);
        secp256k1_scalar_set_int(&sr, 0);
        CHECK(secp256k1_eckey_pubkey_parse(&key, pubkey_mods_zero, 33));
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key, &msg) == 0);
    }

    /* Verify signature with s of zero fails. */
    {
        const unsigned char pubkey[33] = {
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x01
        };
        secp256k1_ge key;
        secp256k1_scalar msg;
        secp256k1_scalar sr, ss;
        secp256k1_scalar_set_int(&ss, 0);
        secp256k1_scalar_set_int(&msg, 0);
        secp256k1_scalar_set_int(&sr, 1);
        CHECK(secp256k1_eckey_pubkey_parse(&key, pubkey, 33));
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key, &msg) == 0);
    }

    /* Verify signature with message 0 passes. */
    {
        const unsigned char pubkey[33] = {
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x02
        };
        const unsigned char pubkey2[33] = {
            0x02, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xfe, 0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0,
            0x3b, 0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41,
            0x43
        };
        secp256k1_ge key;
        secp256k1_ge key2;
        secp256k1_scalar msg;
        secp256k1_scalar sr, ss;
        secp256k1_scalar_set_int(&ss, 2);
        secp256k1_scalar_set_int(&msg, 0);
        secp256k1_scalar_set_int(&sr, 2);
        CHECK(secp256k1_eckey_pubkey_parse(&key, pubkey, 33));
        CHECK(secp256k1_eckey_pubkey_parse(&key2, pubkey2, 33));
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key, &msg) == 1);
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key2, &msg) == 1);
        secp256k1_scalar_negate(&ss, &ss);
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key, &msg) == 1);
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key2, &msg) == 1);
        secp256k1_scalar_set_int(&ss, 1);
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key, &msg) == 0);
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key2, &msg) == 0);
    }

    /* Verify signature with message 1 passes. */
    {
        const unsigned char pubkey[33] = {
            0x02, 0x14, 0x4e, 0x5a, 0x58, 0xef, 0x5b, 0x22,
            0x6f, 0xd2, 0xe2, 0x07, 0x6a, 0x77, 0xcf, 0x05,
            0xb4, 0x1d, 0xe7, 0x4a, 0x30, 0x98, 0x27, 0x8c,
            0x93, 0xe6, 0xe6, 0x3c, 0x0b, 0xc4, 0x73, 0x76,
            0x25
        };
        const unsigned char pubkey2[33] = {
            0x02, 0x8a, 0xd5, 0x37, 0xed, 0x73, 0xd9, 0x40,
            0x1d, 0xa0, 0x33, 0xd2, 0xdc, 0xf0, 0xaf, 0xae,
            0x34, 0xcf, 0x5f, 0x96, 0x4c, 0x73, 0x28, 0x0f,
            0x92, 0xc0, 0xf6, 0x9d, 0xd9, 0xb2, 0x09, 0x10,
            0x62
        };
        const unsigned char csr[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
            0x45, 0x51, 0x23, 0x19, 0x50, 0xb7, 0x5f, 0xc4,
            0x40, 0x2d, 0xa1, 0x72, 0x2f, 0xc9, 0xba, 0xeb
        };
        secp256k1_ge key;
        secp256k1_ge key2;
        secp256k1_scalar msg;
        secp256k1_scalar sr, ss;
        secp256k1_scalar_set_int(&ss, 1);
        secp256k1_scalar_set_int(&msg, 1);
        secp256k1_scalar_set_b32(&sr, csr, NULL);
        CHECK(secp256k1_eckey_pubkey_parse(&key, pubkey, 33));
        CHECK(secp256k1_eckey_pubkey_parse(&key2, pubkey2, 33));
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key, &msg) == 1);
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key2, &msg) == 1);
        secp256k1_scalar_negate(&ss, &ss);
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key, &msg) == 1);
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key2, &msg) == 1);
        secp256k1_scalar_set_int(&ss, 2);
        secp256k1_scalar_inverse_var(&ss, &ss);
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key, &msg) == 0);
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key2, &msg) == 0);
    }

    /* Verify signature with message -1 passes. */
    {
        const unsigned char pubkey[33] = {
            0x03, 0xaf, 0x97, 0xff, 0x7d, 0x3a, 0xf6, 0xa0,
            0x02, 0x94, 0xbd, 0x9f, 0x4b, 0x2e, 0xd7, 0x52,
            0x28, 0xdb, 0x49, 0x2a, 0x65, 0xcb, 0x1e, 0x27,
            0x57, 0x9c, 0xba, 0x74, 0x20, 0xd5, 0x1d, 0x20,
            0xf1
        };
        const unsigned char csr[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
            0x45, 0x51, 0x23, 0x19, 0x50, 0xb7, 0x5f, 0xc4,
            0x40, 0x2d, 0xa1, 0x72, 0x2f, 0xc9, 0xba, 0xee
        };
        secp256k1_ge key;
        secp256k1_scalar msg;
        secp256k1_scalar sr, ss;
        secp256k1_scalar_set_int(&ss, 1);
        secp256k1_scalar_set_int(&msg, 1);
        secp256k1_scalar_negate(&msg, &msg);
        secp256k1_scalar_set_b32(&sr, csr, NULL);
        CHECK(secp256k1_eckey_pubkey_parse(&key, pubkey, 33));
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key, &msg) == 1);
        secp256k1_scalar_negate(&ss, &ss);
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key, &msg) == 1);
        secp256k1_scalar_set_int(&ss, 3);
        secp256k1_scalar_inverse_var(&ss, &ss);
        CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sr, &ss, &key, &msg) == 0);
    }

    /* Signature where s would be zero. */
    {
        secp256k1_pubkey pubkey;
        size_t siglen;
        int32_t ecount;
        unsigned char signature[72];
        static const unsigned char nonce[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        };
        static const unsigned char nonce2[32] = {
            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
            0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,
            0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x40
        };
        const unsigned char key[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        };
        unsigned char msg[32] = {
            0x86, 0x41, 0x99, 0x81, 0x06, 0x23, 0x44, 0x53,
            0xaa, 0x5f, 0x9d, 0x6a, 0x31, 0x78, 0xf4, 0xf7,
            0xb8, 0x12, 0xe0, 0x0b, 0x81, 0x7a, 0x77, 0x62,
            0x65, 0xdf, 0xdd, 0x31, 0xb9, 0x3e, 0x29, 0xa9,
        };
        ecount = 0;
        secp256k1_context_set_illegal_callback(ctx, counting_illegal_callback_fn, &ecount);
        CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg, key, precomputed_nonce_function, nonce) == 0);
        CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg, key, precomputed_nonce_function, nonce2) == 0);
        msg[31] = 0xaa;
        CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg, key, precomputed_nonce_function, nonce) == 1);
        CHECK(ecount == 0);
        CHECK(secp256k1_ecdsa_sign(ctx, NULL, msg, key, precomputed_nonce_function, nonce2) == 0);
        CHECK(ecount == 1);
        CHECK(secp256k1_ecdsa_sign(ctx, &sig, NULL, key, precomputed_nonce_function, nonce2) == 0);
        CHECK(ecount == 2);
        CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg, NULL, precomputed_nonce_function, nonce2) == 0);
        CHECK(ecount == 3);
        CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg, key, precomputed_nonce_function, nonce2) == 1);
        CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, key) == 1);
        CHECK(secp256k1_ecdsa_verify(ctx, NULL, msg, &pubkey) == 0);
        CHECK(ecount == 4);
        CHECK(secp256k1_ecdsa_verify(ctx, &sig, NULL, &pubkey) == 0);
        CHECK(ecount == 5);
        CHECK(secp256k1_ecdsa_verify(ctx, &sig, msg, NULL) == 0);
        CHECK(ecount == 6);
        CHECK(secp256k1_ecdsa_verify(ctx, &sig, msg, &pubkey) == 1);
        CHECK(ecount == 6);
        CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, NULL) == 0);
        CHECK(ecount == 7);
        /* That pubkeyload fails via an ARGCHECK is a little odd but makes sense because pubkeys are an opaque data type. */
        CHECK(secp256k1_ecdsa_verify(ctx, &sig, msg, &pubkey) == 0);
        CHECK(ecount == 8);
        siglen = 72;
        CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, NULL, &siglen, &sig) == 0);
        CHECK(ecount == 9);
        CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, signature, NULL, &sig) == 0);
        CHECK(ecount == 10);
        CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, signature, &siglen, NULL) == 0);
        CHECK(ecount == 11);
        CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, signature, &siglen, &sig) == 1);
        CHECK(ecount == 11);
        CHECK(secp256k1_ecdsa_signature_parse_der(ctx, NULL, signature, siglen) == 0);
        CHECK(ecount == 12);
        CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &sig, NULL, siglen) == 0);
        CHECK(ecount == 13);
        CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &sig, signature, siglen) == 1);
        CHECK(ecount == 13);
        siglen = 10;
        /* Too little room for a signature does not fail via ARGCHECK. */
        CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, signature, &siglen, &sig) == 0);
        CHECK(ecount == 13);
        ecount = 0;
        CHECK(secp256k1_ecdsa_signature_normalize(ctx, NULL, NULL) == 0);
        CHECK(ecount == 1);
        CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, NULL, &sig) == 0);
        CHECK(ecount == 2);
        CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, signature, NULL) == 0);
        CHECK(ecount == 3);
        CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, signature, &sig) == 1);
        CHECK(ecount == 3);
        CHECK(secp256k1_ecdsa_signature_parse_compact(ctx, NULL, signature) == 0);
        CHECK(ecount == 4);
        CHECK(secp256k1_ecdsa_signature_parse_compact(ctx, &sig, NULL) == 0);
        CHECK(ecount == 5);
        CHECK(secp256k1_ecdsa_signature_parse_compact(ctx, &sig, signature) == 1);
        CHECK(ecount == 5);
        memset(signature, 255, 64);
        CHECK(secp256k1_ecdsa_signature_parse_compact(ctx, &sig, signature) == 0);
        CHECK(ecount == 5);
        secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
    }

    /* Nonce function corner cases. */
    for (t = 0; t < 2; t++) {
        static const unsigned char zero[32] = {0x00};
        int i;
        unsigned char key[32];
        unsigned char msg[32];
        secp256k1_ecdsa_signature sig2;
        secp256k1_scalar sr[512], ss;
        const unsigned char *extra;
        extra = t == 0 ? NULL : zero;
        memset(msg, 0, 32);
        msg[31] = 1;
        /* High key results in signature failure. */
        memset(key, 0xFF, 32);
        CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg, key, NULL, extra) == 0);
        CHECK(is_empty_signature(&sig));
        /* Zero key results in signature failure. */
        memset(key, 0, 32);
        CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg, key, NULL, extra) == 0);
        CHECK(is_empty_signature(&sig));
        /* Nonce function failure results in signature failure. */
        key[31] = 1;
        CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg, key, nonce_function_test_fail, extra) == 0);
        CHECK(is_empty_signature(&sig));
        /* The retry loop successfully makes its way to the first good value. */
        CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg, key, nonce_function_test_retry, extra) == 1);
        CHECK(!is_empty_signature(&sig));
        CHECK(secp256k1_ecdsa_sign(ctx, &sig2, msg, key, nonce_function_rfc6979, extra) == 1);
        CHECK(!is_empty_signature(&sig2));
        CHECK(secp256k1_memcmp_var(&sig, &sig2, sizeof(sig)) == 0);
        /* The default nonce function is deterministic. */
        CHECK(secp256k1_ecdsa_sign(ctx, &sig2, msg, key, NULL, extra) == 1);
        CHECK(!is_empty_signature(&sig2));
        CHECK(secp256k1_memcmp_var(&sig, &sig2, sizeof(sig)) == 0);
        /* The default nonce function changes output with different messages. */
        for(i = 0; i < 256; i++) {
            int j;
            msg[0] = i;
            CHECK(secp256k1_ecdsa_sign(ctx, &sig2, msg, key, NULL, extra) == 1);
            CHECK(!is_empty_signature(&sig2));
            secp256k1_ecdsa_signature_load(ctx, &sr[i], &ss, &sig2);
            for (j = 0; j < i; j++) {
                CHECK(!secp256k1_scalar_eq(&sr[i], &sr[j]));
            }
        }
        msg[0] = 0;
        msg[31] = 2;
        /* The default nonce function changes output with different keys. */
        for(i = 256; i < 512; i++) {
            int j;
            key[0] = i - 256;
            CHECK(secp256k1_ecdsa_sign(ctx, &sig2, msg, key, NULL, extra) == 1);
            CHECK(!is_empty_signature(&sig2));
            secp256k1_ecdsa_signature_load(ctx, &sr[i], &ss, &sig2);
            for (j = 0; j < i; j++) {
                CHECK(!secp256k1_scalar_eq(&sr[i], &sr[j]));
            }
        }
        key[0] = 0;
    }

    {
        /* Check that optional nonce arguments do not have equivalent effect. */
        const unsigned char zeros[32] = {0};
        unsigned char nonce[32];
        unsigned char nonce2[32];
        unsigned char nonce3[32];
        unsigned char nonce4[32];
        VG_UNDEF(nonce,32);
        VG_UNDEF(nonce2,32);
        VG_UNDEF(nonce3,32);
        VG_UNDEF(nonce4,32);
        CHECK(nonce_function_rfc6979(nonce, zeros, zeros, NULL, NULL, 0) == 1);
        VG_CHECK(nonce,32);
        CHECK(nonce_function_rfc6979(nonce2, zeros, zeros, zeros, NULL, 0) == 1);
        VG_CHECK(nonce2,32);
        CHECK(nonce_function_rfc6979(nonce3, zeros, zeros, NULL, (void *)zeros, 0) == 1);
        VG_CHECK(nonce3,32);
        CHECK(nonce_function_rfc6979(nonce4, zeros, zeros, zeros, (void *)zeros, 0) == 1);
        VG_CHECK(nonce4,32);
        CHECK(secp256k1_memcmp_var(nonce, nonce2, 32) != 0);
        CHECK(secp256k1_memcmp_var(nonce, nonce3, 32) != 0);
        CHECK(secp256k1_memcmp_var(nonce, nonce4, 32) != 0);
        CHECK(secp256k1_memcmp_var(nonce2, nonce3, 32) != 0);
        CHECK(secp256k1_memcmp_var(nonce2, nonce4, 32) != 0);
        CHECK(secp256k1_memcmp_var(nonce3, nonce4, 32) != 0);
    }


    /* Privkey export where pubkey is the point at infinity. */
    {
        unsigned char privkey[300];
        unsigned char seckey[32] = {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
            0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b,
            0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41,
        };
        size_t outlen = 300;
        CHECK(!ec_privkey_export_der(ctx, privkey, &outlen, seckey, 0));
        outlen = 300;
        CHECK(!ec_privkey_export_der(ctx, privkey, &outlen, seckey, 1));
    }
}

void run_ecdsa_edge_cases(void) {
    test_ecdsa_edge_cases();
}

#ifdef ENABLE_OPENSSL_TESTS
EC_KEY *get_openssl_key(const unsigned char *key32) {
    unsigned char privkey[300];
    size_t privkeylen;
    const unsigned char* pbegin = privkey;
    int compr = secp256k1_testrand_bits(1);
    EC_KEY *ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    CHECK(ec_privkey_export_der(ctx, privkey, &privkeylen, key32, compr));
    CHECK(d2i_ECPrivateKey(&ec_key, &pbegin, privkeylen));
    CHECK(EC_KEY_check_key(ec_key));
    return ec_key;
}

void test_ecdsa_openssl(void) {
    secp256k1_gej qj;
    secp256k1_ge q;
    secp256k1_scalar sigr, sigs;
    secp256k1_scalar one;
    secp256k1_scalar msg2;
    secp256k1_scalar key, msg;
    EC_KEY *ec_key;
    unsigned int sigsize = 80;
    size_t secp_sigsize = 80;
    unsigned char message[32];
    unsigned char signature[80];
    unsigned char key32[32];
    secp256k1_testrand256_test(message);
    secp256k1_scalar_set_b32(&msg, message, NULL);
    random_scalar_order_test(&key);
    secp256k1_scalar_get_b32(key32, &key);
    secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &qj, &key);
    secp256k1_ge_set_gej(&q, &qj);
    ec_key = get_openssl_key(key32);
    CHECK(ec_key != NULL);
    CHECK(ECDSA_sign(0, message, sizeof(message), signature, &sigsize, ec_key));
    CHECK(secp256k1_ecdsa_sig_parse(&sigr, &sigs, signature, sigsize));
    CHECK(secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sigr, &sigs, &q, &msg));
    secp256k1_scalar_set_int(&one, 1);
    secp256k1_scalar_add(&msg2, &msg, &one);
    CHECK(!secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &sigr, &sigs, &q, &msg2));

    random_sign(&sigr, &sigs, &key, &msg, NULL);
    CHECK(secp256k1_ecdsa_sig_serialize(signature, &secp_sigsize, &sigr, &sigs));
    CHECK(ECDSA_verify(0, message, sizeof(message), signature, secp_sigsize, ec_key) == 1);

    EC_KEY_free(ec_key);
}

void run_ecdsa_openssl(void) {
    int i;
    for (i = 0; i < 10*count; i++) {
        test_ecdsa_openssl();
    }
}
#endif

#ifdef ENABLE_MODULE_ECDH
# include "modules/ecdh/tests_impl.h"
#endif

#ifdef ENABLE_MODULE_RECOVERY
# include "modules/recovery/tests_impl.h"
#endif

#ifdef ENABLE_MODULE_EXTRAKEYS
# include "modules/extrakeys/tests_impl.h"
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
# include "modules/schnorrsig/tests_impl.h"
#endif

void run_secp256k1_memczero_test(void) {
    unsigned char buf1[6] = {1, 2, 3, 4, 5, 6};
    unsigned char buf2[sizeof(buf1)];

    /* secp256k1_memczero(..., ..., 0) is a noop. */
    memcpy(buf2, buf1, sizeof(buf1));
    secp256k1_memczero(buf1, sizeof(buf1), 0);
    CHECK(secp256k1_memcmp_var(buf1, buf2, sizeof(buf1)) == 0);

    /* secp256k1_memczero(..., ..., 1) zeros the buffer. */
    memset(buf2, 0, sizeof(buf2));
    secp256k1_memczero(buf1, sizeof(buf1) , 1);
    CHECK(secp256k1_memcmp_var(buf1, buf2, sizeof(buf1)) == 0);
}

void int_cmov_test(void) {
    int r = INT_MAX;
    int a = 0;

    secp256k1_int_cmov(&r, &a, 0);
    CHECK(r == INT_MAX);

    r = 0; a = INT_MAX;
    secp256k1_int_cmov(&r, &a, 1);
    CHECK(r == INT_MAX);

    a = 0;
    secp256k1_int_cmov(&r, &a, 1);
    CHECK(r == 0);

    a = 1;
    secp256k1_int_cmov(&r, &a, 1);
    CHECK(r == 1);

    r = 1; a = 0;
    secp256k1_int_cmov(&r, &a, 0);
    CHECK(r == 1);

}

void fe_cmov_test(void) {
    static const secp256k1_fe zero = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0);
    static const secp256k1_fe one = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 1);
    static const secp256k1_fe max = SECP256K1_FE_CONST(
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL,
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL
    );
    secp256k1_fe r = max;
    secp256k1_fe a = zero;

    secp256k1_fe_cmov(&r, &a, 0);
    CHECK(secp256k1_memcmp_var(&r, &max, sizeof(r)) == 0);

    r = zero; a = max;
    secp256k1_fe_cmov(&r, &a, 1);
    CHECK(secp256k1_memcmp_var(&r, &max, sizeof(r)) == 0);

    a = zero;
    secp256k1_fe_cmov(&r, &a, 1);
    CHECK(secp256k1_memcmp_var(&r, &zero, sizeof(r)) == 0);

    a = one;
    secp256k1_fe_cmov(&r, &a, 1);
    CHECK(secp256k1_memcmp_var(&r, &one, sizeof(r)) == 0);

    r = one; a = zero;
    secp256k1_fe_cmov(&r, &a, 0);
    CHECK(secp256k1_memcmp_var(&r, &one, sizeof(r)) == 0);
}

void fe_storage_cmov_test(void) {
    static const secp256k1_fe_storage zero = SECP256K1_FE_STORAGE_CONST(0, 0, 0, 0, 0, 0, 0, 0);
    static const secp256k1_fe_storage one = SECP256K1_FE_STORAGE_CONST(0, 0, 0, 0, 0, 0, 0, 1);
    static const secp256k1_fe_storage max = SECP256K1_FE_STORAGE_CONST(
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL,
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL
    );
    secp256k1_fe_storage r = max;
    secp256k1_fe_storage a = zero;

    secp256k1_fe_storage_cmov(&r, &a, 0);
    CHECK(secp256k1_memcmp_var(&r, &max, sizeof(r)) == 0);

    r = zero; a = max;
    secp256k1_fe_storage_cmov(&r, &a, 1);
    CHECK(secp256k1_memcmp_var(&r, &max, sizeof(r)) == 0);

    a = zero;
    secp256k1_fe_storage_cmov(&r, &a, 1);
    CHECK(secp256k1_memcmp_var(&r, &zero, sizeof(r)) == 0);

    a = one;
    secp256k1_fe_storage_cmov(&r, &a, 1);
    CHECK(secp256k1_memcmp_var(&r, &one, sizeof(r)) == 0);

    r = one; a = zero;
    secp256k1_fe_storage_cmov(&r, &a, 0);
    CHECK(secp256k1_memcmp_var(&r, &one, sizeof(r)) == 0);
}

void scalar_cmov_test(void) {
    static const secp256k1_scalar zero = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0);
    static const secp256k1_scalar one = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 1);
    static const secp256k1_scalar max = SECP256K1_SCALAR_CONST(
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL,
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL
    );
    secp256k1_scalar r = max;
    secp256k1_scalar a = zero;

    secp256k1_scalar_cmov(&r, &a, 0);
    CHECK(secp256k1_memcmp_var(&r, &max, sizeof(r)) == 0);

    r = zero; a = max;
    secp256k1_scalar_cmov(&r, &a, 1);
    CHECK(secp256k1_memcmp_var(&r, &max, sizeof(r)) == 0);

    a = zero;
    secp256k1_scalar_cmov(&r, &a, 1);
    CHECK(secp256k1_memcmp_var(&r, &zero, sizeof(r)) == 0);

    a = one;
    secp256k1_scalar_cmov(&r, &a, 1);
    CHECK(secp256k1_memcmp_var(&r, &one, sizeof(r)) == 0);

    r = one; a = zero;
    secp256k1_scalar_cmov(&r, &a, 0);
    CHECK(secp256k1_memcmp_var(&r, &one, sizeof(r)) == 0);
}

void ge_storage_cmov_test(void) {
    static const secp256k1_ge_storage zero = SECP256K1_GE_STORAGE_CONST(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    static const secp256k1_ge_storage one = SECP256K1_GE_STORAGE_CONST(0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1);
    static const secp256k1_ge_storage max = SECP256K1_GE_STORAGE_CONST(
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL,
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL,
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL,
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL
    );
    secp256k1_ge_storage r = max;
    secp256k1_ge_storage a = zero;

    secp256k1_ge_storage_cmov(&r, &a, 0);
    CHECK(secp256k1_memcmp_var(&r, &max, sizeof(r)) == 0);

    r = zero; a = max;
    secp256k1_ge_storage_cmov(&r, &a, 1);
    CHECK(secp256k1_memcmp_var(&r, &max, sizeof(r)) == 0);

    a = zero;
    secp256k1_ge_storage_cmov(&r, &a, 1);
    CHECK(secp256k1_memcmp_var(&r, &zero, sizeof(r)) == 0);

    a = one;
    secp256k1_ge_storage_cmov(&r, &a, 1);
    CHECK(secp256k1_memcmp_var(&r, &one, sizeof(r)) == 0);

    r = one; a = zero;
    secp256k1_ge_storage_cmov(&r, &a, 0);
    CHECK(secp256k1_memcmp_var(&r, &one, sizeof(r)) == 0);
}

void run_cmov_tests(void) {
    int_cmov_test();
    fe_cmov_test();
    fe_storage_cmov_test();
    scalar_cmov_test();
    ge_storage_cmov_test();
}

int main(int argc, char **argv) {
    /* Disable buffering for stdout to improve reliability of getting
     * diagnostic information. Happens right at the start of main because
     * setbuf must be used before any other operation on the stream. */
    setbuf(stdout, NULL);
    /* Also disable buffering for stderr because it's not guaranteed that it's
     * unbuffered on all systems. */
    setbuf(stderr, NULL);

    /* find iteration count */
    if (argc > 1) {
        count = strtol(argv[1], NULL, 0);
    } else {
        const char* env = getenv("SECP256K1_TEST_ITERS");
        if (env && strlen(env) > 0) {
            count = strtol(env, NULL, 0);
        }
    }
    if (count <= 0) {
        fputs("An iteration count of 0 or less is not allowed.\n", stderr);
        return EXIT_FAILURE;
    }
    printf("test count = %i\n", count);

    /* find random seed */
    secp256k1_testrand_init(argc > 2 ? argv[2] : NULL);

    /* initialize */
    run_context_tests(0);
    run_context_tests(1);
    run_scratch_tests();
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    if (secp256k1_testrand_bits(1)) {
        unsigned char rand32[32];
        secp256k1_testrand256(rand32);
        CHECK(secp256k1_context_randomize(ctx, secp256k1_testrand_bits(1) ? rand32 : NULL));
    }

    run_rand_bits();
    run_rand_int();

    run_ctz_tests();
    run_modinv_tests();
    run_inverse_tests();

    run_sha256_tests();
    run_hmac_sha256_tests();
    run_rfc6979_hmac_sha256_tests();
    run_tagged_sha256_tests();

    /* scalar tests */
    run_scalar_tests();

    /* field tests */
    run_field_misc();
    run_field_convert();
    run_fe_mul();
    run_sqr();
    run_sqrt();

    /* group tests */
    run_ge();
    run_group_decompress();

    /* ecmult tests */
    run_wnaf();
    run_point_times_order();
    run_ecmult_near_split_bound();
    run_ecmult_chain();
    run_ecmult_constants();
    run_ecmult_gen_blind();
    run_ecmult_const_tests();
    run_ecmult_multi_tests();
    run_ec_combine();

    /* endomorphism tests */
    run_endomorphism_tests();

    /* EC point parser test */
    run_ec_pubkey_parse_test();

    /* EC key edge cases */
    run_eckey_edge_case_test();

    /* EC key arithmetic test */
    run_eckey_negate_test();

#ifdef ENABLE_MODULE_ECDH
    /* ecdh tests */
    run_ecdh_tests();
#endif

    /* ecdsa tests */
    run_pubkey_comparison();
    run_random_pubkeys();
    run_ecdsa_der_parse();
    run_ecdsa_sign_verify();
    run_ecdsa_end_to_end();
    run_ecdsa_edge_cases();
#ifdef ENABLE_OPENSSL_TESTS
    run_ecdsa_openssl();
#endif

#ifdef ENABLE_MODULE_RECOVERY
    /* ECDSA pubkey recovery tests */
    run_recovery_tests();
#endif

#ifdef ENABLE_MODULE_EXTRAKEYS
    run_extrakeys_tests();
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
    run_schnorrsig_tests();
#endif

    /* util tests */
    run_secp256k1_memczero_test();

    run_cmov_tests();

    secp256k1_testrand_finish();

    /* shutdown */
    secp256k1_context_destroy(ctx);

    printf("no problems found\n");
    return 0;
}
