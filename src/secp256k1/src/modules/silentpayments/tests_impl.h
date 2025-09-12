/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_SILENTPAYMENTS_TESTS_H
#define SECP256K1_MODULE_SILENTPAYMENTS_TESTS_H

#include "../../../include/secp256k1_silentpayments.h"
#include "../../../src/modules/silentpayments/vectors.h"

/** Constants
 *
 *             orderc: a scalar which overflows the secp256k1 group order
 *   Malformed Seckey: a seckey that is all zeros
 *          Addresses: scan and spend public keys for Bob and Carol
 *            Outputs: generated outputs from Alice's secret key and Bob/Carol's
 *                     scan public keys
 *  Smallest Outpoint: smallest outpoint lexicographically from the transaction
 *             Seckey: secret key for Alice
 *
 *  The values themselves are not important.
 */
static unsigned char ORDERC[32] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
    0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b,
    0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41
};
static unsigned char MALFORMED_SECKEY[32] = { 0x00 };
static unsigned char BOB_ADDRESS[2][33] = {
    {
        0x02, 0x15, 0x40, 0xae, 0xa8, 0x97, 0x54, 0x7a,
        0xd4, 0x39, 0xb4, 0xe0, 0xf6, 0x09, 0xe5, 0xf0,
        0xfa, 0x63, 0xde, 0x89, 0xab, 0x11, 0xed, 0xe3,
        0x1e, 0x8c, 0xde, 0x4b, 0xe2, 0x19, 0x42, 0x5f,
        0x23
    },
    {
        0x02, 0x3e, 0xff, 0xf8, 0x18, 0x51, 0x65, 0xea,
        0x63, 0xa9, 0x92, 0xb3, 0x9f, 0x31, 0xd8, 0xfd,
        0x8e, 0x0e, 0x64, 0xae, 0xf9, 0xd3, 0x88, 0x07,
        0x34, 0x97, 0x37, 0x14, 0xa5, 0x3d, 0x83, 0x11,
        0x8d
    }
};
static unsigned char CAROL_ADDRESS[2][33] = {
    {
        0x03, 0xbb, 0xc6, 0x3f, 0x12, 0x74, 0x5d, 0x3b,
        0x9e, 0x9d, 0x24, 0xc6, 0xcd, 0x7a, 0x1e, 0xfe,
        0xba, 0xd0, 0xa7, 0xf4, 0x69, 0x23, 0x2f, 0xbe,
        0xcf, 0x31, 0xfb, 0xa7, 0xb4, 0xf7, 0xdd, 0xed,
        0xa8
    },
    {
        0x03, 0x81, 0xeb, 0x9a, 0x9a, 0x9e, 0xc7, 0x39,
        0xd5, 0x27, 0xc1, 0x63, 0x1b, 0x31, 0xb4, 0x21,
        0x56, 0x6f, 0x5c, 0x2a, 0x47, 0xb4, 0xab, 0x5b,
        0x1f, 0x6a, 0x68, 0x6d, 0xfb, 0x68, 0xea, 0xb7,
        0x16
    }
};
static unsigned char BOB_OUTPUT[32] = {
    0x46, 0x0d, 0x68, 0x08, 0x65, 0x64, 0x45, 0xee,
    0x4d, 0x4e, 0xc0, 0x8e, 0xba, 0x8a, 0x66, 0xea,
    0x66, 0x8e, 0x4e, 0x12, 0x98, 0x9a, 0x0e, 0x60,
    0x4b, 0x5c, 0x36, 0x0e, 0x43, 0xf5, 0x5a, 0xfa
};
static unsigned char CAROL_OUTPUT_ONE[32] = {
    0xb7, 0xf3, 0xc6, 0x79, 0x30, 0x4a, 0xef, 0x8c,
    0xc0, 0xc7, 0x61, 0xf1, 0x00, 0x99, 0xdd, 0x7b,
    0x20, 0x65, 0x20, 0xd7, 0x11, 0x6f, 0xb7, 0x91,
    0xee, 0x74, 0x54, 0xa2, 0xfc, 0x22, 0x79, 0xf4
};
static unsigned char CAROL_OUTPUT_TWO[32] = {
    0x4b, 0x81, 0x34, 0x5d, 0x53, 0x89, 0xba, 0xa3,
    0xd8, 0x93, 0xe2, 0xfb, 0xe7, 0x08, 0xdd, 0x6d,
    0x82, 0xdc, 0xd8, 0x49, 0xab, 0x03, 0xc1, 0xdb,
    0x68, 0xbe, 0xc7, 0xe9, 0x2a, 0x45, 0xfa, 0xc5
};
static unsigned char SMALLEST_OUTPOINT[36] = {
    0x16, 0x9e, 0x1e, 0x83, 0xe9, 0x30, 0x85, 0x33, 0x91,
    0xbc, 0x6f, 0x35, 0xf6, 0x05, 0xc6, 0x75, 0x4c, 0xfe,
    0xad, 0x57, 0xcf, 0x83, 0x87, 0x63, 0x9d, 0x3b, 0x40,
    0x96, 0xc5, 0x4f, 0x18, 0xf4, 0x00, 0x00, 0x00, 0x00
};
static unsigned char ALICE_SECKEY[32] = {
    0xea, 0xdc, 0x78, 0x16, 0x5f, 0xf1, 0xf8, 0xea,
    0x94, 0xad, 0x7c, 0xfd, 0xc5, 0x49, 0x90, 0x73,
    0x8a, 0x4c, 0x53, 0xf6, 0xe0, 0x50, 0x7b, 0x42,
    0x15, 0x42, 0x01, 0xb8, 0xe5, 0xdf, 0xf3, 0xb1
};
/* sha256("message") */
static unsigned char MSG32[32] = {
    0xab,0x53,0x0a,0x13,0xe4,0x59,0x14,0x98,
    0x2b,0x79,0xf9,0xb7,0xe3,0xfb,0xa9,0x94,
    0xcf,0xd1,0xf3,0xfb,0x22,0xf7,0x1c,0xea,
    0x1a,0xfb,0xf0,0x2b,0x46,0x0c,0x6d,0x1d
};
/* sha256("random auxiliary data") */
static unsigned char AUX32[32] = {
    0x0b,0x3f,0xdd,0xfd,0x67,0xbf,0x76,0xae,
    0x76,0x39,0xee,0x73,0x5b,0x70,0xff,0x15,
    0x83,0xfd,0x92,0x48,0xc0,0x57,0xd2,0x86,
    0x07,0xa2,0x15,0xf4,0x0b,0x0a,0x3e,0xcc
};

struct label_cache_entry {
    unsigned char label[33];
    unsigned char label_tweak[32];
};
struct labels_cache {
    size_t entries_used;
    struct label_cache_entry entries[10];
};
struct labels_cache labels_cache;
const unsigned char* label_lookup(const unsigned char* key, const void* cache_ptr) {
    const struct labels_cache* cache;
    size_t i;

    if (cache_ptr == NULL) {
        return NULL;
    }
    cache = (const struct labels_cache*)cache_ptr;
    for (i = 0; i < cache->entries_used; i++) {
        if (secp256k1_memcmp_var(cache->entries[i].label, key, 33) == 0) {
            return cache->entries[i].label_tweak;
        }
    }
    return NULL;
}

static void test_recipient_sort_helper(unsigned char (*sp_addresses[3])[2][33], unsigned char (*sp_outputs[3])[32]) {
    unsigned char const *seckey_ptrs[1];
    secp256k1_silentpayments_recipient recipients[3];
    const secp256k1_silentpayments_recipient *recipient_ptrs[3];
    secp256k1_xonly_pubkey generated_outputs[3];
    secp256k1_xonly_pubkey *generated_output_ptrs[3];
    unsigned char xonly_ser[32];
    size_t i;
    int ret;

    seckey_ptrs[0] = ALICE_SECKEY;
    for (i = 0; i < 3; i++) {
        CHECK(secp256k1_ec_pubkey_parse(CTX, &recipients[i].scan_pubkey, (*sp_addresses[i])[0], 33));
        CHECK(secp256k1_ec_pubkey_parse(CTX, &recipients[i].spend_pubkey,(*sp_addresses[i])[1], 33));
        recipients[i].index = i;
        recipient_ptrs[i] = &recipients[i];
        generated_output_ptrs[i] = &generated_outputs[i];
    }
    ret = secp256k1_silentpayments_sender_create_outputs(CTX,
        generated_output_ptrs,
        recipient_ptrs, 3,
        SMALLEST_OUTPOINT,
        NULL, 0,
        seckey_ptrs, 1
    );
    CHECK(ret == 1);
    for (i = 0; i < 3; i++) {
        secp256k1_xonly_pubkey_serialize(CTX, xonly_ser, &generated_outputs[i]);
        CHECK(secp256k1_memcmp_var(xonly_ser, (*sp_outputs[i]), 32) == 0);
    }
}

static void test_recipient_sort(void) {
    unsigned char (*sp_addresses[3])[2][33];
    unsigned char (*sp_outputs[3])[32];

    /* With a fixed set of addresses and a fixed set of inputs,
     * test that we always get the same outputs, regardless of the ordering
     * of the recipients
     */
    sp_addresses[0] = &CAROL_ADDRESS;
    sp_addresses[1] = &BOB_ADDRESS;
    sp_addresses[2] = &CAROL_ADDRESS;

    sp_outputs[0] = &CAROL_OUTPUT_ONE;
    sp_outputs[1] = &BOB_OUTPUT;
    sp_outputs[2] = &CAROL_OUTPUT_TWO;
    test_recipient_sort_helper(sp_addresses, sp_outputs);

    sp_addresses[0] = &CAROL_ADDRESS;
    sp_addresses[1] = &CAROL_ADDRESS;
    sp_addresses[2] = &BOB_ADDRESS;

    sp_outputs[0] = &CAROL_OUTPUT_ONE;
    sp_outputs[1] = &CAROL_OUTPUT_TWO;
    sp_outputs[2] = &BOB_OUTPUT;
    test_recipient_sort_helper(sp_addresses, sp_outputs);

    sp_addresses[0] = &BOB_ADDRESS;
    sp_addresses[1] = &CAROL_ADDRESS;
    sp_addresses[2] = &CAROL_ADDRESS;

    /* Note: in this case, the second output for Carol comes before the first.
     * This is because heapsort is an unstable sorting algorithm, i.e., the ordering
     * of identical elements is not guaranteed to be preserved
     */
    sp_outputs[0] = &BOB_OUTPUT;
    sp_outputs[1] = &CAROL_OUTPUT_TWO;
    sp_outputs[2] = &CAROL_OUTPUT_ONE;
    test_recipient_sort_helper(sp_addresses, sp_outputs);
}

static void test_send_api(void) {
    unsigned char (*sp_addresses[2])[2][33];
    unsigned char const *p[1];
    secp256k1_keypair const *t[1];
    secp256k1_silentpayments_recipient r[2];
    const secp256k1_silentpayments_recipient *rp[2];
    secp256k1_xonly_pubkey o[2];
    secp256k1_xonly_pubkey *op[2];
    secp256k1_keypair taproot;
    size_t i;

    /* Set up Bob and Carol as the recipients */
    sp_addresses[0] = &BOB_ADDRESS;
    sp_addresses[1] = &CAROL_ADDRESS;
    for (i = 0; i < 2; i++) {
        CHECK(secp256k1_ec_pubkey_parse(CTX, &r[i].scan_pubkey, (*sp_addresses[i])[0], 33));
        CHECK(secp256k1_ec_pubkey_parse(CTX, &r[i].spend_pubkey,(*sp_addresses[i])[1], 33));
        /* Set the index value incorrectly */
        r[i].index = 0;
        rp[i] = &r[i];
        op[i] = &o[i];
    }
    /* Set up a taproot key and a plain key for Alice */
    CHECK(secp256k1_keypair_create(CTX, &taproot, ALICE_SECKEY));
    t[0] = &taproot;
    p[0] = ALICE_SECKEY;

    /* Fails if the index is set incorrectly */
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, SMALLEST_OUTPOINT, NULL, 0, p, 1));

    /* Set the index correctly for the next tests */
    for (i = 0; i < 2; i++) {
        r[i].index = i;
    }
    CHECK(secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, SMALLEST_OUTPOINT, NULL, 0, p, 1));

    /* Check that null arguments are handled */
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_sender_create_outputs(CTX, NULL, rp, 2, SMALLEST_OUTPOINT, t, 1, p, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_sender_create_outputs(CTX, op, NULL, 2, SMALLEST_OUTPOINT, t, 1, p, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, NULL, t, 1, p, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, SMALLEST_OUTPOINT, NULL, 1, p, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, SMALLEST_OUTPOINT, t, 1, NULL, 1));

    /* Check correct context is used */
    CHECK_ILLEGAL(STATIC_CTX, secp256k1_silentpayments_sender_create_outputs(STATIC_CTX, op, rp, 2, SMALLEST_OUTPOINT, NULL, 0, p, 1));

    /* Check that array arguments are verified */
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, SMALLEST_OUTPOINT, NULL, 0, NULL, 0));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 0, SMALLEST_OUTPOINT, NULL, 0, p, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, SMALLEST_OUTPOINT, t, 0, p, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, SMALLEST_OUTPOINT, t, 1, p, 0));

    /* Create malformed keys for Alice by using a key that will overflow */
    CHECK(secp256k1_ec_seckey_verify(CTX, ORDERC) == 0);
    p[0] = ORDERC;
    CHECK(secp256k1_keypair_create(CTX, &taproot, ALICE_SECKEY));
    /* Malleate the keypair object so that the secret key is all zeros. We need to keep
     * public key as is since it is loaded first and would hit an ARG_CHECK if invalid.
     */
    memset(&taproot.data[0], 0, 32);
    /* Check that an invalid plain secret key is caught */
    CHECK(secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, SMALLEST_OUTPOINT, NULL, 0, p, 1) == 0);
    /* Check that an invalid keypair is caught */
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, SMALLEST_OUTPOINT, t, 1, NULL, 0));
    /* Create malformed keys for Alice by using a zero'd seckey */
    p[0] = MALFORMED_SECKEY;
    CHECK(secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, SMALLEST_OUTPOINT, NULL, 0, p, 1) == 0);
    p[0] = ALICE_SECKEY;
    /* Create malformed recipients by setting all of the public key bytes to zero.
     * Realistically, this would never happen since a bad public key would get caught when
     * trying to parse the public key with _ec_pubkey_parse
     */
    {
         secp256k1_pubkey tmp = r[1].spend_pubkey;
         memset(&r[1].spend_pubkey, 0, sizeof(r[1].spend_pubkey));
         CHECK_ILLEGAL(CTX, secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, SMALLEST_OUTPOINT, NULL, 0, p, 1));
         r[1].spend_pubkey = tmp;
    }
    {
        secp256k1_pubkey tmp = r[1].scan_pubkey;
        int32_t ecount = 0;

        memset(&r[1].scan_pubkey, 0, sizeof(r[1].scan_pubkey));
        secp256k1_context_set_illegal_callback(CTX, counting_callback_fn, &ecount);
        CHECK(secp256k1_silentpayments_sender_create_outputs(CTX, op, rp, 2, SMALLEST_OUTPOINT, NULL, 0, p, 1) == 0);
        CHECK(ecount == 2);
        secp256k1_context_set_illegal_callback(CTX, NULL, NULL);
        r[1].scan_pubkey = tmp;
    }
}

static void test_label_api(void) {
    secp256k1_pubkey l, s, ls, e; /* label pk, spend pk, labeled spend pk, expected labeled spend pk */
    unsigned char lt[32];         /* label tweak */
    const unsigned char expected[33] = {
        0x03, 0xdc, 0x7f, 0x09, 0x9a, 0xbe, 0x95, 0x7a,
        0x58, 0x43, 0xd2, 0xb6, 0xbb, 0x35, 0x79, 0x61,
        0x5c, 0x60, 0x36, 0xa4, 0x9b, 0x86, 0xf4, 0xbe,
        0x46, 0x38, 0x60, 0x28, 0xa8, 0x1a, 0x77, 0xd4,
        0x91
    };

    /* Create a label and labeled spend public key, verify we get the expected result */
    CHECK(secp256k1_ec_pubkey_parse(CTX, &s, BOB_ADDRESS[1], 33));
    CHECK(secp256k1_silentpayments_recipient_create_label(CTX, &l, lt, ALICE_SECKEY, 1));
    CHECK(secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(CTX, &ls, &s, &l));
    CHECK(secp256k1_ec_pubkey_parse(CTX, &e, expected, 33));
    CHECK(secp256k1_ec_pubkey_cmp(CTX, &ls, &e) == 0);

    /* Check null values are handled */
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_label(CTX, NULL, lt, ALICE_SECKEY, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_label(CTX, &l, NULL, ALICE_SECKEY, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_label(CTX, &l, lt, NULL, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(CTX, NULL, &s, &l));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(CTX, &ls, NULL, &l));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(CTX, &ls, &s, NULL));
    /* Check for malformed spend and label public keys, i.e., any single pubkey is malformed or the public
     * keys are valid but sum up to zero.
     */
    {
        secp256k1_pubkey neg_spend_pubkey = s;
        CHECK(secp256k1_ec_pubkey_negate(CTX, &neg_spend_pubkey));
        CHECK(secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(CTX, &ls, &s, &neg_spend_pubkey) == 0);
        /* Also test with a malformed spend public key. */
        memset(&s, 0, sizeof(s));
        CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(CTX, &ls, &s, &neg_spend_pubkey));
        /* Reset s back to a valid public key for the next test. */
        CHECK(secp256k1_ec_pubkey_parse(CTX, &s, BOB_ADDRESS[1], 33));
        memset(&l, 0, sizeof(l));
        CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(CTX, &ls, &s, &l));
        /* Reset l back to a valid public key for the next test */
        CHECK(secp256k1_silentpayments_recipient_create_label(CTX, &l, lt, ALICE_SECKEY, 1));
        memset(&s, 0, sizeof(s));
        CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(CTX, &ls, &s, &l));
    }
}

static void test_recipient_api(void) {
    secp256k1_silentpayments_recipient_prevouts_summary pd; /* public data */
    secp256k1_silentpayments_recipient_prevouts_summary md; /* malformed public data */
    secp256k1_silentpayments_found_output f;                /* a silent payment found output */
    secp256k1_silentpayments_found_output *fp[1];           /* array of pointers to found outputs */
    secp256k1_xonly_pubkey t;                               /* taproot x-only public key */
    secp256k1_xonly_pubkey malformed_t;                     /* malformed x-only public key */
    secp256k1_xonly_pubkey const *tp[1];                    /* array of pointers to xonly pks */
    secp256k1_pubkey p;                                     /* plain public key */
    secp256k1_pubkey malformed_p;                           /* malformed public key */
    secp256k1_pubkey const *pp[1];                          /* array of pointers to plain pks */
    unsigned char o[33];                                    /* serialized public data, serialized shared secret */
    unsigned char malformed[33] = { 0x01 };                 /* malformed public key serialization */
    size_t n_f;                                             /* number of found outputs */

    CHECK(secp256k1_ec_pubkey_parse(CTX, &p, BOB_ADDRESS[0], 33));
    memset(&malformed_p, 0, sizeof(malformed_p));
    memset(&malformed_t, 0, sizeof(malformed_t));
    memset(&md, 0, sizeof(md));
    md.data[4] = 1;
    CHECK(secp256k1_xonly_pubkey_parse(CTX, &t, &BOB_ADDRESS[0][1]));
    tp[0] = &t;
    pp[0] = &p;
    fp[0] = &f;
    CHECK(secp256k1_silentpayments_recipient_prevouts_summary_create(CTX, &pd, SMALLEST_OUTPOINT, tp, 1, pp, 1));
    /* Check that malformed input public keys are caught. Input public keys summing to zero is tested later,
     * in the BIP0352 test vectors.
     */
    pp[0] = &malformed_p;
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_create(CTX, &pd, SMALLEST_OUTPOINT, tp, 1, pp, 1));
    pp[0] = &p;
    /* Check that malformed x-only input public keys are caught. */
    tp[0] = &malformed_t;
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_create(CTX, &pd, SMALLEST_OUTPOINT, tp, 1, pp, 1));
    tp[0] = &t;

    /* Check null values are handled */
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_create(CTX, NULL, SMALLEST_OUTPOINT, tp, 1, pp, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_create(CTX, &pd, NULL, tp, 1, pp, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_create(CTX, &pd, SMALLEST_OUTPOINT, NULL, 1, pp, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_create(CTX, &pd, SMALLEST_OUTPOINT, tp, 1, NULL, 1));

    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_serialize(CTX, NULL, &pd));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_serialize(CTX, o, NULL));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_parse(CTX, NULL, o));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_parse(CTX, &pd, NULL));

    /* Check that serializing or using a malformed, i.e., not created by us, public data object fails */
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_serialize(CTX, o, &md));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_shared_secret(CTX, o, ALICE_SECKEY, &md));

    /* Check that malformed serializations are rejected */
    CHECK(secp256k1_silentpayments_recipient_prevouts_summary_parse(CTX, &pd, malformed) == 0);

    /* This prevouts_summary object was created with combined = 0, i.e., it has both the input hash and summed public keypair. */
    CHECK(secp256k1_silentpayments_recipient_create_shared_secret(CTX, o, ALICE_SECKEY, &pd));

    /* Parse a prevouts_summary object from a 33 byte serialization and check that this round trips into a valid serialization */
    CHECK(secp256k1_silentpayments_recipient_prevouts_summary_parse(CTX, &pd, BOB_ADDRESS[0]));
    CHECK(secp256k1_silentpayments_recipient_prevouts_summary_serialize(CTX, o, &pd));
    /* Try to create a shared secret with a malformed recipient scan key (all zeros) */
    CHECK(secp256k1_silentpayments_recipient_create_shared_secret(CTX, o, MALFORMED_SECKEY, &pd) == 0);
    /* Try to create a shared secret with a malformed public data (all zeros) */
    memset(&pd.data[1], 0, sizeof(pd.data) - 1);
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_shared_secret(CTX, o, ALICE_SECKEY, &pd));
    /* Reset pd to a valid public data object */
    CHECK(secp256k1_silentpayments_recipient_prevouts_summary_parse(CTX, &pd, BOB_ADDRESS[0]));

    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_create(CTX, &pd, SMALLEST_OUTPOINT, tp, 0, pp, 1));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_create(CTX, &pd, SMALLEST_OUTPOINT, tp, 1, pp, 0));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_create(CTX, &pd, SMALLEST_OUTPOINT, NULL, 0, pp, 0));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_prevouts_summary_create(CTX, &pd, SMALLEST_OUTPOINT, NULL, 0, NULL, 0));

    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_shared_secret(CTX, NULL, ALICE_SECKEY, &pd));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_shared_secret(CTX, o, NULL, &pd));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_shared_secret(CTX, o, ALICE_SECKEY, NULL));

    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_output_pubkey(CTX, NULL, o, &p, 0));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_output_pubkey(CTX, &t, NULL, &p, 0));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_create_output_pubkey(CTX, &t, o, NULL, 0));

    /* check the _recipient_create_output_pubkey cornercase where internal tweaking would fail;
       this is the case if the recipient spend public key is P = -(create_t_k(shared_secret, k))*G */
    {
        unsigned char shared_secret[33];
        const uint32_t k = 0;
        secp256k1_scalar t_k;
        unsigned char t_k_ser[32];
        secp256k1_pubkey fake_spend_pubkey;
        secp256k1_xonly_pubkey output_xonly;

        CHECK(secp256k1_silentpayments_recipient_create_shared_secret(CTX, shared_secret, ALICE_SECKEY, &pd));
        secp256k1_silentpayments_create_t_k(&t_k, shared_secret, k);
        secp256k1_scalar_get_b32(t_k_ser, &t_k);
        CHECK(secp256k1_ec_pubkey_create(CTX, &fake_spend_pubkey, t_k_ser));
        CHECK(secp256k1_ec_pubkey_negate(CTX, &fake_spend_pubkey));
        CHECK(secp256k1_silentpayments_recipient_create_output_pubkey(CTX, &output_xonly, shared_secret, &fake_spend_pubkey, k) == 0);
    }
    /* check the _recipient_scan_outputs cornercase where internal tweaking would fail;
       this is the case if the recipient spend public key is P = -(create_t_k(shared_secret, k))*G */
    {
        unsigned char malformed_spend_key[32] = {
                0xed, 0x52, 0x40, 0x3f, 0x47, 0x96, 0x62, 0xb0,
                0x80, 0xaa, 0xf3, 0xee, 0xf3, 0x9a, 0xfa, 0x86,
                0xc4, 0x7a, 0xb0, 0xed, 0x5f, 0xbc, 0xa9, 0x31,
                0xf8, 0x80, 0x4a, 0x0e, 0x0a, 0xed, 0x2a, 0x84,
        };
        secp256k1_pubkey neg_spend_pubkey;
        CHECK(secp256k1_ec_pubkey_create(CTX, &neg_spend_pubkey, malformed_spend_key));
        CHECK(secp256k1_ec_pubkey_negate(CTX, &neg_spend_pubkey));
        CHECK(secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, tp, 1, ALICE_SECKEY, &pd, &neg_spend_pubkey, &label_lookup, &labels_cache) == 0);
    }

    n_f = 0;
    labels_cache.entries_used = 0;
    CHECK(secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, tp, 1, ALICE_SECKEY, &pd, &p, &label_lookup, &labels_cache));
    CHECK(secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, tp, 1, ALICE_SECKEY, &pd, &p, &label_lookup, NULL));
    CHECK(secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, tp, 1, ALICE_SECKEY, &pd, &p, NULL, NULL));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_scan_outputs(CTX, NULL, &n_f, tp, 1, ALICE_SECKEY, &pd, &p, &label_lookup, &labels_cache));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, NULL, tp, 1, ALICE_SECKEY, &pd, &p, &label_lookup, &labels_cache));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, NULL, 1, ALICE_SECKEY, &pd, &p, &label_lookup, &labels_cache));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, tp, 1, NULL, &pd, &p, &label_lookup, &labels_cache));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, tp, 1, ALICE_SECKEY, NULL, &p, &label_lookup, &labels_cache));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, tp, 1, ALICE_SECKEY, &pd, NULL, &label_lookup, &labels_cache));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, tp, 0, ALICE_SECKEY, &pd, &p, &label_lookup, &labels_cache));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, tp, 1, ALICE_SECKEY, &pd, &p, NULL, &labels_cache));

    /* Check that malformed secret key, public key, and public data arguments are handled */
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, tp, 1, ALICE_SECKEY, &pd, &malformed_p, NULL, NULL));
    CHECK(secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, tp, 1, MALFORMED_SECKEY, &pd, &p, NULL, NULL) == 0);
    memset(&pd, 0, sizeof(pd));
    CHECK_ILLEGAL(CTX, secp256k1_silentpayments_recipient_scan_outputs(CTX, fp, &n_f, tp, 1, ALICE_SECKEY, &pd, &p, NULL, NULL));
}

void run_silentpayments_test_vector_send(const struct bip352_test_vector *test) {
    secp256k1_silentpayments_recipient recipients[MAX_OUTPUTS_PER_TEST_CASE];
    const secp256k1_silentpayments_recipient *recipient_ptrs[MAX_OUTPUTS_PER_TEST_CASE];
    secp256k1_xonly_pubkey generated_outputs[MAX_OUTPUTS_PER_TEST_CASE];
    secp256k1_xonly_pubkey *generated_output_ptrs[MAX_OUTPUTS_PER_TEST_CASE];
    secp256k1_keypair taproot_keypairs[MAX_INPUTS_PER_TEST_CASE];
    secp256k1_keypair const *taproot_keypair_ptrs[MAX_INPUTS_PER_TEST_CASE];
    unsigned char const *plain_seckeys[MAX_INPUTS_PER_TEST_CASE];
    unsigned char created_output[32];
    size_t i, j, k;
    int match, ret;

    /* Check that sender creates expected outputs */
    for (i = 0; i < test->num_outputs; i++) {
        CHECK(secp256k1_ec_pubkey_parse(CTX, &recipients[i].scan_pubkey, test->recipient_pubkeys[i].scan_pubkey, 33));
        CHECK(secp256k1_ec_pubkey_parse(CTX, &recipients[i].spend_pubkey, test->recipient_pubkeys[i].spend_pubkey, 33));
        recipients[i].index = i;
        recipient_ptrs[i] = &recipients[i];
        generated_output_ptrs[i] = &generated_outputs[i];
    }
    for (i = 0; i < test->num_plain_inputs; i++) {
        plain_seckeys[i] = test->plain_seckeys[i];
    }
    for (i = 0; i < test->num_taproot_inputs; i++) {
        CHECK(secp256k1_keypair_create(CTX, &taproot_keypairs[i], test->taproot_seckeys[i]));
        taproot_keypair_ptrs[i] = &taproot_keypairs[i];
    }
    {
        int32_t ecount = 0;
        secp256k1_context_set_illegal_callback(CTX, counting_callback_fn, &ecount);
        ret = secp256k1_silentpayments_sender_create_outputs(CTX,
            generated_output_ptrs,
            recipient_ptrs,
            test->num_outputs,
            test->outpoint_smallest,
            test->num_taproot_inputs > 0 ? taproot_keypair_ptrs : NULL, test->num_taproot_inputs,
            test->num_plain_inputs > 0 ? plain_seckeys : NULL, test->num_plain_inputs
        );
        secp256k1_context_set_illegal_callback(CTX, NULL, NULL);
        /* We expect exactly one ARG_CHECK if the number of input keys was 0. */
        CHECK(ecount == (test->num_taproot_inputs + test->num_plain_inputs == 0));
    }
    /* If we are unable to create outputs, e.g., the input keys sum to zero, check that the
     * expected number of recipient outputs for this test case is zero
     */
    if (!ret) {
        CHECK(test->num_recipient_outputs == 0);
        return;
    }

    match = 0;
    for (i = 0; i < test->num_output_sets; i++) {
        size_t n_matches = 0;
        for (j = 0; j < test->num_outputs; j++) {
            CHECK(secp256k1_xonly_pubkey_serialize(CTX, created_output, &generated_outputs[j]));
            /* Loop over both lists to ensure tests don't fail due to different orderings of outputs */
            for (k = 0; k < test->num_recipient_outputs; k++) {
                if (secp256k1_memcmp_var(created_output, test->recipient_outputs[i][k], 32) == 0) {
                    n_matches++;
                    break;
                }
            }
        }
        if (n_matches == test->num_recipient_outputs) {
            match = 1;
            break;
        }
    }
    CHECK(match);
}

void run_silentpayments_test_vector_receive(const struct bip352_test_vector *test) {
    secp256k1_pubkey plain_pubkeys_objs[MAX_INPUTS_PER_TEST_CASE];
    secp256k1_xonly_pubkey xonly_pubkeys_objs[MAX_INPUTS_PER_TEST_CASE];
    secp256k1_xonly_pubkey tx_output_objs[MAX_OUTPUTS_PER_TEST_CASE];
    secp256k1_silentpayments_found_output found_output_objs[MAX_OUTPUTS_PER_TEST_CASE];
    secp256k1_pubkey const *plain_pubkeys[MAX_INPUTS_PER_TEST_CASE];
    secp256k1_xonly_pubkey const *xonly_pubkeys[MAX_INPUTS_PER_TEST_CASE];
    secp256k1_xonly_pubkey const *tx_outputs[MAX_OUTPUTS_PER_TEST_CASE];
    secp256k1_silentpayments_found_output *found_outputs[MAX_OUTPUTS_PER_TEST_CASE];
    unsigned char found_outputs_light_client[MAX_OUTPUTS_PER_TEST_CASE][32];
    secp256k1_pubkey recipient_scan_pubkey;
    secp256k1_pubkey recipient_spend_pubkey;
    secp256k1_pubkey label;
    size_t len = 33;
    size_t i,j;
    int match, ret;
    size_t n_found = 0;
    unsigned char found_output[32];
    unsigned char found_signatures[10][64];
    secp256k1_silentpayments_recipient_prevouts_summary prevouts_summary, prevouts_summary_index;
    unsigned char shared_secret_lightclient[33];
    unsigned char light_client_data[33];


    /* prepare the inputs */
    for (i = 0; i < test->num_plain_inputs; i++) {
        CHECK(secp256k1_ec_pubkey_parse(CTX, &plain_pubkeys_objs[i], test->plain_pubkeys[i], 33));
        plain_pubkeys[i] = &plain_pubkeys_objs[i];
    }
    for (i = 0; i < test->num_taproot_inputs; i++) {
        CHECK(secp256k1_xonly_pubkey_parse(CTX, &xonly_pubkeys_objs[i], test->xonly_pubkeys[i]));
        xonly_pubkeys[i] = &xonly_pubkeys_objs[i];
    }
    {
        int32_t ecount = 0;
        secp256k1_context_set_illegal_callback(CTX, counting_callback_fn, &ecount);
        ret = secp256k1_silentpayments_recipient_prevouts_summary_create(CTX, &prevouts_summary,
            test->outpoint_smallest,
            test->num_taproot_inputs > 0 ? xonly_pubkeys : NULL, test->num_taproot_inputs,
            test->num_plain_inputs > 0 ? plain_pubkeys : NULL, test->num_plain_inputs
        );
        secp256k1_context_set_illegal_callback(CTX, NULL, NULL);
        /* We expect exactly one ARG_CHECK if the number of input keys was 0. */
        CHECK(ecount == (test->num_taproot_inputs + test->num_plain_inputs == 0));
    }
    /* If we are unable to create the public_data object, e.g., the input public keys sum to
     * zero, check that the expected number of recipient outputs for this test case is zero
     */
    if (!ret) {
        CHECK(test->num_found_output_pubkeys == 0);
        return;
    }
    /* prepare the outputs */
    for (i = 0; i < test->num_to_scan_outputs; i++) {
        CHECK(secp256k1_xonly_pubkey_parse(CTX, &tx_output_objs[i], test->to_scan_outputs[i]));
        tx_outputs[i] = &tx_output_objs[i];
    }
    for (i = 0; i < test->num_found_output_pubkeys; i++) {
        found_outputs[i] = &found_output_objs[i];
    }

    /* scan / spend pubkeys are not in the given data of the recipient part, so let's compute them */
    CHECK(secp256k1_ec_pubkey_create(CTX, &recipient_scan_pubkey, test->scan_seckey));
    CHECK(secp256k1_ec_pubkey_create(CTX, &recipient_spend_pubkey, test->spend_seckey));

    /* create labels cache */
    labels_cache.entries_used = 0;
    for (i = 0; i < test->num_labels; i++) {
        unsigned int m = test->label_integers[i];
        struct label_cache_entry *cache_entry = &labels_cache.entries[labels_cache.entries_used];
        CHECK(secp256k1_silentpayments_recipient_create_label(CTX, &label, cache_entry->label_tweak, test->scan_seckey, m));
        CHECK(secp256k1_ec_pubkey_serialize(CTX, cache_entry->label, &len, &label, SECP256K1_EC_COMPRESSED));
        labels_cache.entries_used++;
    }
    CHECK(secp256k1_silentpayments_recipient_scan_outputs(CTX,
        found_outputs, &n_found,
        tx_outputs, test->num_to_scan_outputs,
        test->scan_seckey,
        &prevouts_summary,
        &recipient_spend_pubkey,
        label_lookup, &labels_cache)
    );
    for (i = 0; i < n_found; i++) {
        unsigned char full_seckey[32];
        secp256k1_keypair keypair;
        unsigned char signature[64];
        memcpy(&full_seckey, test->spend_seckey, 32);
        CHECK(secp256k1_ec_seckey_tweak_add(CTX, full_seckey, found_outputs[i]->tweak));
        CHECK(secp256k1_keypair_create(CTX, &keypair, full_seckey));
        CHECK(secp256k1_schnorrsig_sign32(CTX, signature, MSG32, &keypair, AUX32));
        memcpy(found_signatures[i], signature, 64);
    }

    /* compare expected and scanned outputs (including calculated seckey tweaks and signatures) */
    match = 0;
    for (i = 0; i < n_found; i++) {
        CHECK(secp256k1_xonly_pubkey_serialize(CTX, found_output, &found_outputs[i]->output));
        for (j = 0; j < test->num_found_output_pubkeys; j++) {
            if (secp256k1_memcmp_var(&found_output, test->found_output_pubkeys[j], 32) == 0) {
                CHECK(secp256k1_memcmp_var(found_outputs[i]->tweak, test->found_seckey_tweaks[j], 32) == 0);
                CHECK(secp256k1_memcmp_var(found_signatures[i], test->found_signatures[j], 64) == 0);
                match = 1;
                break;
            }
        }
        CHECK(match);
    }
    CHECK(n_found == test->num_found_output_pubkeys);
    /* Scan as a light client
     * it is not recommended to use labels as a light client so here we are only
     * running this on tests that do not involve labels. Primarily, this test is to
     * ensure that _recipient_created_shared_secret and _create_shared_secret are the same
     */
    if (test->num_labels == 0) {
        CHECK(secp256k1_silentpayments_recipient_prevouts_summary_serialize(CTX, light_client_data, &prevouts_summary));
        CHECK(secp256k1_silentpayments_recipient_prevouts_summary_parse(CTX, &prevouts_summary_index, light_client_data));
        CHECK(secp256k1_silentpayments_recipient_create_shared_secret(CTX, shared_secret_lightclient, test->scan_seckey, &prevouts_summary_index));
        n_found = 0;
        {
            int found = 0;
            size_t k = 0;
            secp256k1_xonly_pubkey potential_output;

            while(1) {

                CHECK(secp256k1_silentpayments_recipient_create_output_pubkey(CTX,
                    &potential_output,
                    shared_secret_lightclient,
                    &recipient_spend_pubkey,
                    k
                ));
                /* At this point, we check that the utxo exists with a light client protocol.
                 * For this example, we'll just iterate through the list of pubkeys */
                found = 0;
                for (i = 0; i < test->num_to_scan_outputs; i++) {
                    if (secp256k1_xonly_pubkey_cmp(CTX, &potential_output, tx_outputs[i]) == 0) {
                        secp256k1_xonly_pubkey_serialize(CTX, found_outputs_light_client[n_found], &potential_output);
                        found = 1;
                        n_found++;
                        k++;
                        break;
                    }
                }
                if (!found) {
                    break;
                }
            }
        }
        CHECK(n_found == test->num_found_output_pubkeys);
        for (i = 0; i < n_found; i++) {
            match = 0;
            for (j = 0; j < test->num_found_output_pubkeys; j++) {
                if (secp256k1_memcmp_var(&found_outputs_light_client[i], test->found_output_pubkeys[j], 32) == 0) {
                    match = 1;
                    break;
                }
            }
            CHECK(match);
        }
    }
}

static void silentpayments_sha256_tag_test(void) {
    secp256k1_sha256 sha;
    {
        /* "BIP0352/Inputs" */
        static const unsigned char tag[] = {'B','I','P','0','3','5','2','/','I','n','p','u','t','s'};
        secp256k1_silentpayments_sha256_init_inputs(&sha);
        test_sha256_tag_midstate(&sha, tag, sizeof(tag));
    }
    {
        /* "BIP0352/SharedSecret" */
        static const unsigned char tag[] = {'B','I','P','0','3','5','2','/','S','h','a','r','e','d', 'S','e','c','r','e','t'};
        secp256k1_silentpayments_sha256_init_sharedsecret(&sha);
        test_sha256_tag_midstate(&sha, tag, sizeof(tag));
    }
    {
        /* "BIP0352/Label" */
        static const unsigned char tag[] = {'B','I','P','0','3','5','2','/','L','a','b','e','l'};
        secp256k1_silentpayments_sha256_init_label(&sha);
        test_sha256_tag_midstate(&sha, tag, sizeof(tag));
    }
}


void run_silentpayments_test_vectors(void) {
    size_t i;

    for (i = 0; i < sizeof(bip352_test_vectors) / sizeof(bip352_test_vectors[0]); i++) {
        const struct bip352_test_vector *test = &bip352_test_vectors[i];
        run_silentpayments_test_vector_send(test);
        run_silentpayments_test_vector_receive(test);
    }
}

void run_silentpayments_tests(void) {
    test_recipient_sort();
    test_send_api();
    test_label_api();
    test_recipient_api();
    run_silentpayments_test_vectors();
    silentpayments_sha256_tag_test();
}

#endif
