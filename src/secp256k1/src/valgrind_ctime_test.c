/**********************************************************************
 * Copyright (c) 2020 Gregory Maxwell                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <valgrind/memcheck.h>
#include "include/secp256k1.h"
#include "util.h"

#if ENABLE_MODULE_ECDH
# include "include/secp256k1_ecdh.h"
#endif

#if ENABLE_MODULE_RECOVERY
# include "include/secp256k1_recovery.h"
#endif

int main(void) {
    secp256k1_context* ctx;
    secp256k1_ecdsa_signature signature;
    secp256k1_pubkey pubkey;
    size_t siglen = 74;
    size_t outputlen = 33;
    int i;
    int ret;
    unsigned char msg[32];
    unsigned char key[32];
    unsigned char sig[74];
    unsigned char spubkey[33];
#if ENABLE_MODULE_RECOVERY
    secp256k1_ecdsa_recoverable_signature recoverable_signature;
    int recid;
#endif

    if (!RUNNING_ON_VALGRIND) {
        fprintf(stderr, "This test can only usefully be run inside valgrind.\n");
        fprintf(stderr, "Usage: libtool --mode=execute valgrind ./valgrind_ctime_test\n");
        exit(1);
    }

    /** In theory, testing with a single secret input should be sufficient:
     *  If control flow depended on secrets the tool would generate an error.
     */
    for (i = 0; i < 32; i++) {
        key[i] = i + 65;
    }
    for (i = 0; i < 32; i++) {
        msg[i] = i + 1;
    }

    ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_DECLASSIFY);

    /* Test keygen. */
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = secp256k1_ec_pubkey_create(ctx, &pubkey, key);
    VALGRIND_MAKE_MEM_DEFINED(&pubkey, sizeof(secp256k1_pubkey));
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(secp256k1_ec_pubkey_serialize(ctx, spubkey, &outputlen, &pubkey, SECP256K1_EC_COMPRESSED) == 1);

    /* Test signing. */
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = secp256k1_ecdsa_sign(ctx, &signature, msg, key, NULL, NULL);
    VALGRIND_MAKE_MEM_DEFINED(&signature, sizeof(secp256k1_ecdsa_signature));
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, sig, &siglen, &signature));

#if ENABLE_MODULE_ECDH
    /* Test ECDH. */
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = secp256k1_ecdh(ctx, msg, &pubkey, key, NULL, NULL);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif

#if ENABLE_MODULE_RECOVERY
    /* Test signing a recoverable signature. */
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = secp256k1_ecdsa_sign_recoverable(ctx, &recoverable_signature, msg, key, NULL, NULL);
    VALGRIND_MAKE_MEM_DEFINED(&recoverable_signature, sizeof(recoverable_signature));
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, sig, &recid, &recoverable_signature));
    CHECK(recid >= 0 && recid <= 3);
#endif

    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = secp256k1_ec_seckey_verify(ctx, key);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);

    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = secp256k1_ec_seckey_negate(ctx, key);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);

    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    VALGRIND_MAKE_MEM_UNDEFINED(msg, 32);
    ret = secp256k1_ec_seckey_tweak_add(ctx, key, msg);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);

    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    VALGRIND_MAKE_MEM_UNDEFINED(msg, 32);
    ret = secp256k1_ec_seckey_tweak_mul(ctx, key, msg);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);

    /* Test context randomisation. Do this last because it leaves the context tainted. */
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = secp256k1_context_randomize(ctx, key);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret);

    secp256k1_context_destroy(ctx);
    return 0;
}
