/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_SILENTPAYMENTS_TESTS_H
#define SECP256K1_MODULE_SILENTPAYMENTS_TESTS_H

#include "../../../include/secp256k1_silentpayments.h"

void run_silentpayments_tests(void) {
    /* BIP-352 test vector
     * "Single recipient: taproot input with odd y-value and non-taproot input" */
    unsigned char outpoint_lowest[36] = {
        0x16,0x9e,0x1e,0x83,0xe9,0x30,0x85,0x33,0x91,0xbc,0x6f,0x35,0xf6,0x05,0xc6,0x75,
        0x4c,0xfe,0xad,0x57,0xcf,0x83,0x87,0x63,0x9d,0x3b,0x40,0x96,0xc5,0x4f,0x18,0xf4,
        0x00,0x00,0x00,0x00
    };
    unsigned char input_privkey_plain0[32] = {
        0x8d,0x47,0x51,0xf6,0xe8,0xa3,0x58,0x68,0x80,0xfb,0x66,0xc1,0x9a,0xe2,0x77,0x96,
        0x9b,0xd5,0xaa,0x06,0xf6,0x1c,0x4e,0xe2,0xf1,0xe2,0x48,0x6e,0xfd,0xf6,0x66,0xd3
    };
    unsigned char input_privkey_taproot0[32] = {
        0x1d,0x37,0x78,0x7c,0x2b,0x71,0x16,0xee,0x98,0x3e,0x9f,0x9c,0x13,0x26,0x9d,0xf2,
        0x90,0x91,0xb3,0x91,0xc0,0x4d,0xb9,0x42,0x39,0xe0,0xd2,0xbc,0x21,0x82,0xc3,0xbf
    };
    const unsigned char* input_privkeys_plain[1];
    const unsigned char* input_privkeys_taproot[1];
    unsigned char input_pubkey_plain0[33] = {0x03,
        0xe0,0xec,0x4f,0x64,0xb3,0xfa,0x2e,0x46,0x3c,0xcf,0xcf,0x4e,0x85,0x6e,0x37,0xd5,
        0xe1,0xe2,0x02,0x75,0xbc,0x89,0xec,0x1d,0xef,0x9e,0xb0,0x98,0xef,0xf1,0xf8,0x5d
    };
    unsigned char input_pubkey_xonly0[32] = {
        0x8c,0x8d,0x23,0xd4,0x76,0x4f,0xef,0xfc,0xd5,0xe7,0x2e,0x38,0x08,0x02,0x54,0x0f,
        0xa0,0xf8,0x8e,0x3d,0x62,0xad,0x5e,0x0b,0x47,0x95,0x5f,0x74,0xd7,0xb2,0x83,0xc4
    };
    const secp256k1_pubkey* input_pubkeys_plain[1];
    const secp256k1_xonly_pubkey* input_pubkeys_xonly[1];
    unsigned char receiver_scan_privkey[32] = {
        0x0f,0x69,0x4e,0x06,0x80,0x28,0xa7,0x17,0xf8,0xaf,0x6b,0x94,0x11,0xf9,0xa1,0x33,
        0xdd,0x35,0x65,0x25,0x87,0x14,0xcc,0x22,0x65,0x94,0xb3,0x4d,0xb9,0x0c,0x1f,0x2c
    };
    unsigned char receiver_scan_pubkey[33] = {0x02,
        0x20,0xbc,0xfa,0xc5,0xb9,0x9e,0x04,0xad,0x1a,0x06,0xdd,0xfb,0x01,0x6e,0xe1,0x35,
        0x82,0x60,0x9d,0x60,0xb6,0x29,0x1e,0x98,0xd0,0x1a,0x9b,0xc9,0xa1,0x6c,0x96,0xd4
    };
    unsigned char receiver_spend_privkey[32] = {
        0x9d,0x6a,0xd8,0x55,0xce,0x34,0x17,0xef,0x84,0xe8,0x36,0x89,0x2e,0x5a,0x56,0x39,
        0x2b,0xfb,0xa0,0x5f,0xa5,0xd9,0x7c,0xce,0xa3,0x0e,0x26,0x6f,0x54,0x0e,0x08,0xb3
    };
    unsigned char receiver_spend_pubkey[33] = {0x02,
        0x5c,0xc9,0x85,0x6d,0x6f,0x83,0x75,0x35,0x0e,0x12,0x39,0x78,0xda,0xac,0x20,0x0c,
        0x26,0x0c,0xb5,0xb5,0xae,0x83,0x10,0x6c,0xab,0x90,0x48,0x4d,0xcd,0x8f,0xcf,0x36
    };
    unsigned char output_expected[32] = {
        0x35,0x93,0x58,0xf5,0x9e,0xe9,0xe9,0xee,0xc3,0xf0,0x0b,0xdf,0x48,0x82,0x57,0x0f,
        0xd5,0xc1,0x82,0xe4,0x51,0xaa,0x26,0x50,0xb7,0x88,0x54,0x4a,0xff,0x01,0x2a,0x3a
    };
    unsigned char privkey_tweak_expected[32] = {
        0xa2,0xf9,0xdd,0x05,0xd1,0xd3,0x98,0x34,0x7c,0x88,0x5d,0x9c,0x61,0xa6,0x4d,0x18,
        0xa2,0x64,0xde,0x6d,0x49,0xce,0xa4,0x32,0x6b,0xaf,0xc2,0x79,0x1d,0x62,0x7f,0xa7
    };

    unsigned char shared_secret_sender[33];
    unsigned char shared_secret_receiver[33];
    unsigned char shared_secret_lightclient[33];
    secp256k1_pubkey public_keys_sum;
    secp256k1_pubkey tweaked_public_key;
    unsigned char private_keys_sum[32];
    unsigned char input_hash1[32];
    unsigned char input_hash2[32];
    secp256k1_xonly_pubkey output_expected_xonly_obj;
    secp256k1_xonly_pubkey output_calculated_xonly_obj;
    unsigned char output_calculated[32];
    unsigned char privkey_calculated[32];
    unsigned char privkey_expected[32];
    int direct_match;
    unsigned char t_k[32];

    /* convert raw key material into secp256k1 objects where necessary */
    secp256k1_pubkey input_pubkey_plain_obj, receiver_scan_pubkey_obj, receiver_spend_pubkey_obj;
    secp256k1_xonly_pubkey input_pubkey_xonly_obj;
    CHECK(secp256k1_ec_pubkey_parse(CTX, &input_pubkey_plain_obj, input_pubkey_plain0, 33));
    CHECK(secp256k1_ec_pubkey_parse(CTX, &receiver_scan_pubkey_obj, receiver_scan_pubkey, 33));
    CHECK(secp256k1_ec_pubkey_parse(CTX, &receiver_spend_pubkey_obj, receiver_spend_pubkey, 33));
    CHECK(secp256k1_xonly_pubkey_parse(CTX, &input_pubkey_xonly_obj, input_pubkey_xonly0));
    CHECK(secp256k1_xonly_pubkey_parse(CTX, &output_expected_xonly_obj, output_expected));
    input_pubkeys_plain[0] = &input_pubkey_plain_obj;
    input_pubkeys_xonly[0] = &input_pubkey_xonly_obj;

    /* create shared secret from sender and receiver perspective, and check that they match */
    input_privkeys_plain[0] = input_privkey_plain0;
    input_privkeys_taproot[0] = input_privkey_taproot0;
    CHECK(secp256k1_silentpayments_create_private_tweak_data(CTX, private_keys_sum, input_hash1,
        input_privkeys_plain, 1, input_privkeys_taproot, 1, outpoint_lowest));
    CHECK(secp256k1_silentpayments_create_shared_secret(CTX, shared_secret_sender,
        &receiver_scan_pubkey_obj, private_keys_sum, input_hash1));

    CHECK(secp256k1_silentpayments_create_public_tweak_data(CTX, &public_keys_sum, input_hash2,
        input_pubkeys_plain, 1, input_pubkeys_xonly, 1, outpoint_lowest));
    CHECK(secp256k1_silentpayments_create_shared_secret(CTX, shared_secret_receiver,
        &public_keys_sum, receiver_scan_privkey, input_hash2));

    /* light client case */
    CHECK(secp256k1_silentpayments_create_tweaked_pubkey(CTX, &tweaked_public_key, &public_keys_sum, input_hash2));
    CHECK(secp256k1_silentpayments_create_shared_secret(CTX, shared_secret_lightclient,
        &tweaked_public_key, receiver_scan_privkey, NULL));
    CHECK(secp256k1_memcmp_var(shared_secret_receiver, shared_secret_lightclient, 33) == 0);

    CHECK(secp256k1_memcmp_var(input_hash1, input_hash2, 32) == 0);
    CHECK(secp256k1_memcmp_var(shared_secret_sender, shared_secret_receiver, 33) == 0);

    /* check that calculated silent payments output matches */
    CHECK(secp256k1_silentpayments_sender_create_output_pubkey(CTX, &output_calculated_xonly_obj,
        shared_secret_sender, &receiver_spend_pubkey_obj, 0));
    CHECK(secp256k1_xonly_pubkey_serialize(CTX, output_calculated, &output_calculated_xonly_obj));
    CHECK(secp256k1_memcmp_var(output_calculated, output_expected, 32) == 0);

    CHECK(secp256k1_silentpayments_receiver_scan_output(CTX, &direct_match, t_k, NULL,
        shared_secret_receiver, &receiver_spend_pubkey_obj, 0, &output_expected_xonly_obj));
    CHECK(direct_match == 1);

    /* check that calculated silent payment output spending private key matches */
    memcpy(privkey_expected, receiver_spend_privkey, 32);
    CHECK(secp256k1_ec_seckey_tweak_add(CTX, privkey_expected, privkey_tweak_expected));
    CHECK(secp256k1_silentpayments_create_output_seckey(CTX, privkey_calculated,
        receiver_spend_privkey, t_k, NULL));
    CHECK(secp256k1_memcmp_var(privkey_calculated, privkey_expected, 32) == 0);
}

#endif
