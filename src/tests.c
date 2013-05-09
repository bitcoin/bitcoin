// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>

#include "impl/num.h"
#include "impl/field.h"
#include "impl/group.h"
#include "impl/ecmult.h"
#include "impl/ecdsa.h"
#include "impl/util.h"

#ifdef ENABLE_OPENSSL_TESTS
#include "openssl/bn.h"
#include "openssl/ec.h"
#include "openssl/ecdsa.h"
#include "openssl/obj_mac.h"
#endif

static int count = 100;

/***** NUM TESTS *****/

void random_num_negate(secp256k1_num_t *num) {
    if (secp256k1_rand32() & 1)
        secp256k1_num_negate(num);
}

void random_num_order_test(secp256k1_num_t *num) {
    do {
        unsigned char b32[32];
        secp256k1_rand256_test(b32);
        secp256k1_num_set_bin(num, b32, 32);
        if (secp256k1_num_is_zero(num))
            continue;
        if (secp256k1_num_cmp(num, &secp256k1_ge_consts->order) >= 0)
            continue;
        break;
    } while(1);
}

void random_num_order(secp256k1_num_t *num) {
    do {
        unsigned char b32[32];
        secp256k1_rand256(b32);
        secp256k1_num_set_bin(num, b32, 32);
        if (secp256k1_num_is_zero(num))
            continue;
        if (secp256k1_num_cmp(num, &secp256k1_ge_consts->order) >= 0)
            continue;
        break;
    } while(1);
}

void test_num_copy_inc_cmp() {
    secp256k1_num_t n1,n2;
    secp256k1_num_init(&n1);
    secp256k1_num_init(&n2);
    random_num_order(&n1);
    secp256k1_num_copy(&n2, &n1);
    assert(secp256k1_num_cmp(&n1, &n2) == 0);
    assert(secp256k1_num_cmp(&n2, &n1) == 0);
    secp256k1_num_inc(&n2);
    assert(secp256k1_num_cmp(&n1, &n2) != 0);
    assert(secp256k1_num_cmp(&n2, &n1) != 0);
    secp256k1_num_free(&n1);
    secp256k1_num_free(&n2);
}


void test_num_get_set_hex() {
    secp256k1_num_t n1,n2;
    secp256k1_num_init(&n1);
    secp256k1_num_init(&n2);
    random_num_order_test(&n1);
    char c[64];
    secp256k1_num_get_hex(c, 64, &n1);
    secp256k1_num_set_hex(&n2, c, 64);
    assert(secp256k1_num_cmp(&n1, &n2) == 0);
    for (int i=0; i<64; i++) {
        // check whether the lower 4 bits correspond to the last hex character
        int low1 = secp256k1_num_shift(&n1, 4);
        int lowh = c[63];
        int low2 = (lowh>>6)*9+(lowh-'0')&15;
        assert(low1 == low2);
        // shift bits off the hex representation, and compare
        memmove(c+1, c, 63);
        c[0] = '0';
        secp256k1_num_set_hex(&n2, c, 64);
        assert(secp256k1_num_cmp(&n1, &n2) == 0);
    }
    secp256k1_num_free(&n2);
    secp256k1_num_free(&n1);
}

void test_num_get_set_bin() {
    secp256k1_num_t n1,n2;
    secp256k1_num_init(&n1);
    secp256k1_num_init(&n2);
    random_num_order_test(&n1);
    unsigned char c[32];
    secp256k1_num_get_bin(c, 32, &n1);
    secp256k1_num_set_bin(&n2, c, 32);
    assert(secp256k1_num_cmp(&n1, &n2) == 0);
    for (int i=0; i<32; i++) {
        // check whether the lower 8 bits correspond to the last byte
        int low1 = secp256k1_num_shift(&n1, 8);
        int low2 = c[31];
        assert(low1 == low2);
        // shift bits off the byte representation, and compare
        memmove(c+1, c, 31);
        c[0] = 0;
        secp256k1_num_set_bin(&n2, c, 32);
        assert(secp256k1_num_cmp(&n1, &n2) == 0);
    }
    secp256k1_num_free(&n2);
    secp256k1_num_free(&n1);
}

void run_num_int() {
    secp256k1_num_t n1;
    secp256k1_num_init(&n1);
    for (int i=-255; i<256; i++) {
        unsigned char c1[3] = {};
        c1[2] = abs(i);
        unsigned char c2[3] = {0x11,0x22,0x33};
        secp256k1_num_set_int(&n1, i);
        secp256k1_num_get_bin(c2, 3, &n1);
        assert(memcmp(c1, c2, 3) == 0);
    }
    secp256k1_num_free(&n1);
}

void test_num_negate() {
    secp256k1_num_t n1;
    secp256k1_num_t n2;
    secp256k1_num_init(&n1);
    secp256k1_num_init(&n2);
    random_num_order_test(&n1); // n1 = R
    random_num_negate(&n1);
    secp256k1_num_copy(&n2, &n1); // n2 = R
    secp256k1_num_sub(&n1, &n2, &n1); // n1 = n2-n1 = 0
    assert(secp256k1_num_is_zero(&n1));
    secp256k1_num_copy(&n1, &n2); // n1 = R
    secp256k1_num_negate(&n1); // n1 = -R
    assert(!secp256k1_num_is_zero(&n1));
    secp256k1_num_add(&n1, &n2, &n1); // n1 = n2+n1 = 0
    assert(secp256k1_num_is_zero(&n1));
    secp256k1_num_copy(&n1, &n2); // n1 = R
    secp256k1_num_negate(&n1); // n1 = -R
    assert(secp256k1_num_is_neg(&n1) != secp256k1_num_is_neg(&n2));
    secp256k1_num_negate(&n1); // n1 = R
    assert(secp256k1_num_cmp(&n1, &n2) == 0);
    assert(secp256k1_num_is_neg(&n1) == secp256k1_num_is_neg(&n2));
    secp256k1_num_free(&n2);
    secp256k1_num_free(&n1);
}

void test_num_add_sub() {
    secp256k1_num_t n1;
    secp256k1_num_t n2;
    secp256k1_num_init(&n1);
    secp256k1_num_init(&n2);
    random_num_order_test(&n1); // n1 = R1
    random_num_negate(&n1);
    random_num_order_test(&n2); // n2 = R2
    random_num_negate(&n2);
    secp256k1_num_t n1p2, n2p1, n1m2, n2m1;
    secp256k1_num_init(&n1p2);
    secp256k1_num_init(&n2p1);
    secp256k1_num_init(&n1m2);
    secp256k1_num_init(&n2m1);
    secp256k1_num_add(&n1p2, &n1, &n2); // n1p2 = R1 + R2
    secp256k1_num_add(&n2p1, &n2, &n1); // n2p1 = R2 + R1
    secp256k1_num_sub(&n1m2, &n1, &n2); // n1m2 = R1 - R2
    secp256k1_num_sub(&n2m1, &n2, &n1); // n2m1 = R2 - R1
    assert(secp256k1_num_cmp(&n1p2, &n2p1) == 0);
    assert(secp256k1_num_cmp(&n1p2, &n1m2) != 0);
    secp256k1_num_negate(&n2m1); // n2m1 = -R2 + R1
    assert(secp256k1_num_cmp(&n2m1, &n1m2) == 0);
    assert(secp256k1_num_cmp(&n2m1, &n1) != 0);
    secp256k1_num_add(&n2m1, &n2m1, &n2); // n2m1 = -R2 + R1 + R2 = R1
    assert(secp256k1_num_cmp(&n2m1, &n1) == 0);
    assert(secp256k1_num_cmp(&n2p1, &n1) != 0);
    secp256k1_num_sub(&n2p1, &n2p1, &n2); // n2p1 = R2 + R1 - R2 = R1
    assert(secp256k1_num_cmp(&n2p1, &n1) == 0);
    secp256k1_num_free(&n2m1);
    secp256k1_num_free(&n1m2);
    secp256k1_num_free(&n2p1);
    secp256k1_num_free(&n1p2);
    secp256k1_num_free(&n2);
    secp256k1_num_free(&n1);
}

void run_num_smalltests() {
    for (int i=0; i<100*count; i++) {
        test_num_copy_inc_cmp();
        test_num_get_set_hex();
        test_num_get_set_bin();
        test_num_negate();
        test_num_add_sub();
    }
    run_num_int();
}

void run_ecmult_chain() {
    // random starting point A (on the curve)
    secp256k1_fe_t ax; secp256k1_fe_set_hex(&ax, "8b30bbe9ae2a990696b22f670709dff3727fd8bc04d3362c6c7bf458e2846004", 64);
    secp256k1_fe_t ay; secp256k1_fe_set_hex(&ay, "a357ae915c4a65281309edf20504740f0eb3343990216b4f81063cb65f2f7e0f", 64);
    secp256k1_gej_t a; secp256k1_gej_set_xy(&a, &ax, &ay);
    // two random initial factors xn and gn
    secp256k1_num_t xn;
    secp256k1_num_init(&xn);
    secp256k1_num_set_hex(&xn, "84cc5452f7fde1edb4d38a8ce9b1b84ccef31f146e569be9705d357a42985407", 64);
    secp256k1_num_t gn;
    secp256k1_num_init(&gn);
    secp256k1_num_set_hex(&gn, "a1e58d22553dcd42b23980625d4c57a96e9323d42b3152e5ca2c3990edc7c9de", 64);
    // two small multipliers to be applied to xn and gn in every iteration:
    secp256k1_num_t xf;
    secp256k1_num_init(&xf);
    secp256k1_num_set_hex(&xf, "1337", 4);
    secp256k1_num_t gf;
    secp256k1_num_init(&gf);
    secp256k1_num_set_hex(&gf, "7113", 4);
    // accumulators with the resulting coefficients to A and G
    secp256k1_num_t ae;
    secp256k1_num_init(&ae);
    secp256k1_num_set_int(&ae, 1);
    secp256k1_num_t ge;
    secp256k1_num_init(&ge);
    secp256k1_num_set_int(&ge, 0);
    // the point being computed
    secp256k1_gej_t x = a;
    const secp256k1_num_t *order = &secp256k1_ge_consts->order;
    for (int i=0; i<200*count; i++) {
        // in each iteration, compute X = xn*X + gn*G;
        secp256k1_ecmult(&x, &x, &xn, &gn);
        // also compute ae and ge: the actual accumulated factors for A and G
        // if X was (ae*A+ge*G), xn*X + gn*G results in (xn*ae*A + (xn*ge+gn)*G)
        secp256k1_num_mod_mul(&ae, &ae, &xn, order);
        secp256k1_num_mod_mul(&ge, &ge, &xn, order);
        secp256k1_num_add(&ge, &ge, &gn);
        secp256k1_num_mod(&ge, order);
        // modify xn and gn
        secp256k1_num_mod_mul(&xn, &xn, &xf, order);
        secp256k1_num_mod_mul(&gn, &gn, &gf, order);

        // verify
        if (i == 19999) {
            char res[132]; int resl = 132;
            secp256k1_gej_get_hex(res, &resl, &x);
            assert(strcmp(res, "(D6E96687F9B10D092A6F35439D86CEBEA4535D0D409F53586440BD74B933E830,B95CBCA2C77DA786539BE8FD53354D2D3B4F566AE658045407ED6015EE1B2A88)") == 0);
        }
    }
    // redo the computation, but directly with the resulting ae and ge coefficients:
    secp256k1_gej_t x2; secp256k1_ecmult(&x2, &a, &ae, &ge);
    char res[132]; int resl = 132;
    char res2[132]; int resl2 = 132;
    secp256k1_gej_get_hex(res, &resl, &x);
    secp256k1_gej_get_hex(res2, &resl2, &x2);
    assert(strcmp(res, res2) == 0);
    assert(strlen(res) == 131);
    secp256k1_num_free(&xn);
    secp256k1_num_free(&gn);
    secp256k1_num_free(&xf);
    secp256k1_num_free(&gf);
    secp256k1_num_free(&ae);
    secp256k1_num_free(&ge);
}

void test_point_times_order(const secp256k1_gej_t *point) {
    // either the point is not on the curve, or multiplying it by the order results in O
    if (!secp256k1_gej_is_valid(point))
        return;

    const secp256k1_num_t *order = &secp256k1_ge_consts->order;
    secp256k1_num_t zero;
    secp256k1_num_init(&zero);
    secp256k1_num_set_int(&zero, 0);
    secp256k1_gej_t res;
    secp256k1_ecmult(&res, point, order, order); // calc res = order * point + order * G;
    assert(secp256k1_gej_is_infinity(&res));
    secp256k1_num_free(&zero);
}

void run_point_times_order() {
    secp256k1_fe_t x; secp256k1_fe_set_hex(&x, "02", 2);
    for (int i=0; i<500; i++) {
        secp256k1_ge_t p; secp256k1_ge_set_xo(&p, &x, 1);
        secp256k1_gej_t j; secp256k1_gej_set_ge(&j, &p);
        test_point_times_order(&j);
        secp256k1_fe_sqr(&x, &x);
    }
    char c[65]; int cl=65;
    secp256k1_fe_get_hex(c, &cl, &x);
    assert(strcmp(c, "7603CB59B0EF6C63FE6084792A0C378CDB3233A80F8A9A09A877DEAD31B38C45") == 0);
}

void test_wnaf(const secp256k1_num_t *number, int w) {
    secp256k1_num_t x, two, t;
    secp256k1_num_init(&x);
    secp256k1_num_init(&two);
    secp256k1_num_init(&t);
    secp256k1_num_set_int(&x, 0);
    secp256k1_num_set_int(&two, 2);
    int wnaf[257];
    int bits = secp256k1_ecmult_wnaf(wnaf, number, w);
    int zeroes = -1;
    for (int i=bits-1; i>=0; i--) {
        secp256k1_num_mul(&x, &x, &two);
        int v = wnaf[i];
        if (v) {
            assert(zeroes == -1 || zeroes >= w-1); // check that distance between non-zero elements is at least w-1
            zeroes=0;
            assert((v & 1) == 1); // check non-zero elements are odd
            assert(v <= (1 << (w-1)) - 1); // check range below
            assert(v >= -(1 << (w-1)) - 1); // check range above
        } else {
            assert(zeroes != -1); // check that no unnecessary zero padding exists
            zeroes++;
        }
        secp256k1_num_set_int(&t, v);
        secp256k1_num_add(&x, &x, &t);
    }
    assert(secp256k1_num_cmp(&x, number) == 0); // check that wnaf represents number
    secp256k1_num_free(&x);
    secp256k1_num_free(&two);
    secp256k1_num_free(&t);
}

void run_wnaf() {
    secp256k1_num_t n;
    secp256k1_num_init(&n);
    for (int i=0; i<count; i++) {
        random_num_order(&n);
        if (i % 1)
            secp256k1_num_negate(&n);
        test_wnaf(&n, 4+(i%10));
    }
    secp256k1_num_free(&n);
}

void random_sign(secp256k1_ecdsa_sig_t *sig, const secp256k1_num_t *key, const secp256k1_num_t *msg, int *recid) {
    secp256k1_num_t nonce;
    secp256k1_num_init(&nonce);
    do {
        random_num_order_test(&nonce);
    } while(!secp256k1_ecdsa_sig_sign(sig, key, msg, &nonce, recid));
    secp256k1_num_free(&nonce);
}

void test_ecdsa_sign_verify() {
    const secp256k1_ge_consts_t *c = secp256k1_ge_consts;
    secp256k1_num_t msg, key;
    secp256k1_num_init(&msg);
    random_num_order_test(&msg);
    secp256k1_num_init(&key);
    random_num_order_test(&key);
    secp256k1_gej_t pubj; secp256k1_ecmult_gen(&pubj, &key);
    secp256k1_ge_t pub; secp256k1_ge_set_gej(&pub, &pubj);
    secp256k1_ecdsa_sig_t sig;
    secp256k1_ecdsa_sig_init(&sig);
    random_sign(&sig, &key, &msg, NULL);
    assert(secp256k1_ecdsa_sig_verify(&sig, &pub, &msg));
    secp256k1_num_inc(&msg);
    assert(!secp256k1_ecdsa_sig_verify(&sig, &pub, &msg));
    secp256k1_ecdsa_sig_free(&sig);
    secp256k1_num_free(&msg);
    secp256k1_num_free(&key);
}

void run_ecdsa_sign_verify() {
    for (int i=0; i<10*count; i++) {
        test_ecdsa_sign_verify();
    }
}

#ifdef ENABLE_OPENSSL_TESTS
EC_KEY *get_openssl_key(const secp256k1_num_t *key) {
    unsigned char privkey[300];
    int privkeylen;
    int compr = secp256k1_rand32() & 1;
    const unsigned char* pbegin = privkey;
    EC_KEY *ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    assert(secp256k1_ecdsa_privkey_serialize(privkey, &privkeylen, key, compr));
    assert(d2i_ECPrivateKey(&ec_key, &pbegin, privkeylen));
    assert(EC_KEY_check_key(ec_key));
    return ec_key;
}

void test_ecdsa_openssl() {
    const secp256k1_ge_consts_t *c = secp256k1_ge_consts;
    secp256k1_num_t key, msg;
    secp256k1_num_init(&msg);
    unsigned char message[32];
    secp256k1_rand256_test(message);
    secp256k1_num_set_bin(&msg, message, 32);
    secp256k1_num_init(&key);
    random_num_order_test(&key);
    secp256k1_gej_t qj;
    secp256k1_ecmult_gen(&qj, &key);
    secp256k1_ge_t q;
    secp256k1_ge_set_gej(&q, &qj);
    EC_KEY *ec_key = get_openssl_key(&key);
    assert(ec_key);
    unsigned char signature[80];
    int sigsize = 80;
    assert(ECDSA_sign(0, message, sizeof(message), signature, &sigsize, ec_key));
    secp256k1_ecdsa_sig_t sig;
    secp256k1_ecdsa_sig_init(&sig);
    assert(secp256k1_ecdsa_sig_parse(&sig, signature, sigsize));
    assert(secp256k1_ecdsa_sig_verify(&sig, &q, &msg));
    secp256k1_num_inc(&sig.r);
    assert(!secp256k1_ecdsa_sig_verify(&sig, &q, &msg));

    random_sign(&sig, &key, &msg, NULL);
    sigsize = 80;
    assert(secp256k1_ecdsa_sig_serialize(signature, &sigsize, &sig));
    assert(ECDSA_verify(0, message, sizeof(message), signature, sigsize, ec_key) == 1);

    secp256k1_ecdsa_sig_free(&sig);
    EC_KEY_free(ec_key);
    secp256k1_num_free(&key);
    secp256k1_num_free(&msg);
}

void run_ecdsa_openssl() {
    for (int i=0; i<10*count; i++) {
        test_ecdsa_openssl();
    }
}
#endif

int main(int argc, char **argv) {
    if (argc > 1)
        count = strtol(argv[1], NULL, 0)*47;

    printf("test count = %i\n", count);

    // initialize
    secp256k1_fe_start();
    secp256k1_ge_start();
    secp256k1_ecmult_start();

    // num tests
    run_num_smalltests();

    // ecmult tests
    run_wnaf();
    run_point_times_order();
    run_ecmult_chain();

    // ecdsa tests
    run_ecdsa_sign_verify();
#ifdef ENABLE_OPENSSL_TESTS
    run_ecdsa_openssl();
#endif

    // shutdown
    secp256k1_ecmult_stop();
    secp256k1_ge_stop();
    secp256k1_fe_stop();
    return 0;
}
