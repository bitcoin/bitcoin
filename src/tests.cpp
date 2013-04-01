#include <assert.h>

#include "num.cpp"
#include "field.cpp"
#include "group.cpp"
#include "ecmult.cpp"
#include "ecdsa.cpp"

// #define COUNT 2
#define COUNT 100

void test_run_ecmult_chain() {
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
    for (int i=0; i<200*COUNT; i++) {
        // in each iteration, compute X = xn*X + gn*G;
        secp256k1_ecmult(&x, &x, &xn, &gn);
        // also compute ae and ge: the actual accumulated factors for A and G
        // if X was (ae*A+ge*G), xn*X + gn*G results in (xn*ae*A + (xn*ge+gn)*G)
        secp256k1_num_mod_mul(&ae, &ae, &xn, order);
        secp256k1_num_mod_mul(&ge, &ge, &xn, order);
        secp256k1_num_add(&ge, &ge, &gn);
        secp256k1_num_mod(&ge, &ge, order);
        // modify xn and gn
        secp256k1_num_mod_mul(&xn, &xn, &xf, order);
        secp256k1_num_mod_mul(&gn, &gn, &gf, order);
    }
    char res[132]; int resl = 132;
    secp256k1_gej_get_hex(res, &resl, &x);
    if (COUNT == 100) {
      assert(strcmp(res, "(D6E96687F9B10D092A6F35439D86CEBEA4535D0D409F53586440BD74B933E830,B95CBCA2C77DA786539BE8FD53354D2D3B4F566AE658045407ED6015EE1B2A88)") == 0);
    }
    // redo the computation, but directly with the resulting ae and ge coefficients:
    secp256k1_gej_t x2; secp256k1_ecmult(&x2, &a, &ae, &ge);
    char res2[132]; int resl2 = 132;
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

void test_point_times_order(const secp256k1_gej_t &point) {
    // either the point is not on the curve, or multiplying it by the order results in O
    if (!secp256k1_gej_is_valid(&point))
        return;

    const secp256k1_num_t *order = &secp256k1_ge_consts->order;
    secp256k1_num_t zero;
    secp256k1_num_init(&zero);
    secp256k1_num_set_int(&zero, 0);
    secp256k1_gej_t res;
    secp256k1_ecmult(&res, &point, order, order); // calc res = order * point + order * G;
    assert(secp256k1_gej_is_infinity(&res));
    secp256k1_num_free(&zero);
}

void test_run_point_times_order() {
    secp256k1_fe_t x; secp256k1_fe_set_hex(&x, "02", 2);
    for (int i=0; i<500; i++) {
        secp256k1_gej_t j; secp256k1_gej_set_xo(&j, &x, true);
        test_point_times_order(j);
        secp256k1_fe_sqr(&x, &x);
    }
    char c[65]; int cl=65;
    secp256k1_fe_get_hex(c, &cl, &x);
    assert(strcmp(c, "7603CB59B0EF6C63FE6084792A0C378CDB3233A80F8A9A09A877DEAD31B38C45") == 0);
}

void test_wnaf(const secp256k1_num_t &number, int w) {
    secp256k1_num_t x, two, t;
    secp256k1_num_init(&x);
    secp256k1_num_init(&two);
    secp256k1_num_init(&t);
    secp256k1_num_set_int(&x, 0);
    secp256k1_num_set_int(&two, 2);
    int wnaf[1024];
    int bits = secp256k1_ecmult_wnaf(wnaf, &number, w);
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
    assert(secp256k1_num_cmp(&x, &number) == 0); // check that wnaf represents number
    secp256k1_num_free(&x);
    secp256k1_num_free(&two);
    secp256k1_num_free(&t);
}

void test_run_wnaf() {
    secp256k1_num_t range, min, n;
    secp256k1_num_init(&range);
    secp256k1_num_init(&min);
    secp256k1_num_init(&n);
    secp256k1_num_set_hex(&range, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 256);
    secp256k1_num_copy(&min, &range);
    secp256k1_num_shift(&min, 1);
    secp256k1_num_negate(&min);
    for (int i=0; i<COUNT; i++) {
        secp256k1_num_set_rand(&n, &range);
        secp256k1_num_add(&n, &n, &min);
        test_wnaf(n, 4+(i%10));
    }
    secp256k1_num_free(&range);
    secp256k1_num_free(&min);
    secp256k1_num_free(&n);
}

void test_ecdsa_sign_verify() {
    const secp256k1_ge_consts_t &c = *secp256k1_ge_consts;
    secp256k1_num_t msg, key, nonce;
    secp256k1_num_init(&msg);
    secp256k1_num_set_rand(&msg, &c.order);
    secp256k1_num_init(&key);
    secp256k1_num_set_rand(&key, &c.order);
    secp256k1_num_init(&nonce);
    secp256k1_gej_t pub; secp256k1_ecmult_gen(&pub, &key);
    secp256k1_ecdsa_sig_t sig;
    secp256k1_ecdsa_sig_init(&sig);
    do {
        secp256k1_num_set_rand(&nonce, &c.order);
    } while(!secp256k1_ecdsa_sig_sign(&sig, &key, &msg, &nonce));
    assert(secp256k1_ecdsa_sig_verify(&sig, &pub, &msg));
    secp256k1_num_inc(&msg);
    assert(!secp256k1_ecdsa_sig_verify(&sig, &pub, &msg));
    secp256k1_ecdsa_sig_free(&sig);
    secp256k1_num_free(&msg);
    secp256k1_num_free(&key);
    secp256k1_num_free(&nonce);
}

void test_run_ecdsa_sign_verify() {
    for (int i=0; i<10*COUNT; i++) {
        test_ecdsa_sign_verify();
    }
}

int main(void) {
    secp256k1_num_start();
    secp256k1_fe_start();
    secp256k1_ge_start();
    secp256k1_ecmult_start();

    test_run_wnaf();
    test_run_point_times_order();
    test_run_ecmult_chain();
    test_run_ecdsa_sign_verify();

    secp256k1_ecmult_stop();
    secp256k1_ge_stop();
    secp256k1_fe_stop();
    secp256k1_num_stop();
    return 0;
}
