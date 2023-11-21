/***********************************************************************
 * Copyright (c) 2020 Gregory Maxwell                                  *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include <stdio.h>

#include "../include/secp256k1.h"
#include "assumptions.h"
#include "checkmem.h"

#if !SECP256K1_CHECKMEM_ENABLED
#  error "This tool cannot be compiled without memory-checking interface (valgrind or msan)"
#endif

#ifdef ENABLE_MODULE_ECDH
# include "../include/secp256k1_ecdh.h"
#endif

#ifdef ENABLE_MODULE_RECOVERY
# include "../include/secp256k1_recovery.h"
#endif

#ifdef ENABLE_MODULE_EXTRAKEYS
# include "../include/secp256k1_extrakeys.h"
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
#include "../include/secp256k1_schnorrsig.h"
#endif

#ifdef ENABLE_MODULE_ELLSWIFT
#include "../include/secp256k1_ellswift.h"
#endif

static void run_tests(secp256k1_context *ctx, unsigned char *key);

int main(void) {
    secp256k1_context* ctx;
    unsigned char key[32];
    int ret, i;

    if (!SECP256K1_CHECKMEM_RUNNING()) {
        fprintf(stderr, "This test can only usefully be run inside valgrind because it was not compiled under msan.\n");
        fprintf(stderr, "Usage: libtool --mode=execute valgrind ./ctime_tests\n");
        return 1;
    }
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_DECLASSIFY);
    /** In theory, testing with a single secret input should be sufficient:
     *  If control flow depended on secrets the tool would generate an error.
     */
    for (i = 0; i < 32; i++) {
        key[i] = i + 65;
    }

    run_tests(ctx, key);

    /* Test context randomisation. Do this last because it leaves the context
     * tainted. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_context_randomize(ctx, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);

    secp256k1_context_destroy(ctx);
    return 0;
}

static void run_tests(secp256k1_context *ctx, unsigned char *key) {
    secp256k1_ecdsa_signature signature;
    secp256k1_pubkey pubkey;
    size_t siglen = 74;
    size_t outputlen = 33;
    int i;
    int ret;
    unsigned char msg[32];
    unsigned char sig[74];
    unsigned char spubkey[33];
#ifdef ENABLE_MODULE_RECOVERY
    secp256k1_ecdsa_recoverable_signature recoverable_signature;
    int recid;
#endif
#ifdef ENABLE_MODULE_EXTRAKEYS
    secp256k1_keypair keypair;
#endif
#ifdef ENABLE_MODULE_ELLSWIFT
    unsigned char ellswift[64];
    static const unsigned char prefix[64] = {'t', 'e', 's', 't'};
#endif

    for (i = 0; i < 32; i++) {
        msg[i] = i + 1;
    }

    /* Test keygen. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ec_pubkey_create(ctx, &pubkey, key);
    SECP256K1_CHECKMEM_DEFINE(&pubkey, sizeof(secp256k1_pubkey));
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(secp256k1_ec_pubkey_serialize(ctx, spubkey, &outputlen, &pubkey, SECP256K1_EC_COMPRESSED) == 1);

    /* Test signing. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ecdsa_sign(ctx, &signature, msg, key, NULL, NULL);
    SECP256K1_CHECKMEM_DEFINE(&signature, sizeof(secp256k1_ecdsa_signature));
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, sig, &siglen, &signature));

#ifdef ENABLE_MODULE_ECDH
    /* Test ECDH. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ecdh(ctx, msg, &pubkey, key, NULL, NULL);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif

#ifdef ENABLE_MODULE_RECOVERY
    /* Test signing a recoverable signature. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ecdsa_sign_recoverable(ctx, &recoverable_signature, msg, key, NULL, NULL);
    SECP256K1_CHECKMEM_DEFINE(&recoverable_signature, sizeof(recoverable_signature));
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, sig, &recid, &recoverable_signature));
    CHECK(recid >= 0 && recid <= 3);
#endif

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ec_seckey_verify(ctx, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ec_seckey_negate(ctx, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    SECP256K1_CHECKMEM_UNDEFINE(msg, 32);
    ret = secp256k1_ec_seckey_tweak_add(ctx, key, msg);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    SECP256K1_CHECKMEM_UNDEFINE(msg, 32);
    ret = secp256k1_ec_seckey_tweak_mul(ctx, key, msg);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    /* Test keypair_create and keypair_xonly_tweak_add. */
#ifdef ENABLE_MODULE_EXTRAKEYS
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_keypair_create(ctx, &keypair, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    /* The tweak is not treated as a secret in keypair_tweak_add */
    SECP256K1_CHECKMEM_DEFINE(msg, 32);
    ret = secp256k1_keypair_xonly_tweak_add(ctx, &keypair, msg);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    SECP256K1_CHECKMEM_UNDEFINE(&keypair, sizeof(keypair));
    ret = secp256k1_keypair_sec(ctx, key, &keypair);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_keypair_create(ctx, &keypair, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
    ret = secp256k1_schnorrsig_sign32(ctx, sig, msg, &keypair, NULL);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif

#ifdef ENABLE_MODULE_ELLSWIFT
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = secp256k1_ellswift_create(ctx, ellswift, key, NULL);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);

    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = secp256k1_ellswift_create(ctx, ellswift, key, ellswift);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);

    for (i = 0; i < 2; i++) {
        VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
        VALGRIND_MAKE_MEM_DEFINED(&ellswift, sizeof(ellswift));
        ret = secp256k1_ellswift_xdh(ctx, msg, ellswift, ellswift, key, i, secp256k1_ellswift_xdh_hash_function_bip324, NULL);
        VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
        CHECK(ret == 1);

        VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
        VALGRIND_MAKE_MEM_DEFINED(&ellswift, sizeof(ellswift));
        ret = secp256k1_ellswift_xdh(ctx, msg, ellswift, ellswift, key, i, secp256k1_ellswift_xdh_hash_function_prefix, (void *)prefix);
        VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
        CHECK(ret == 1);
    }

#endif
}
