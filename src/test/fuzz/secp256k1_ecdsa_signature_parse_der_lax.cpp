// Copyright (c) 2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <secp256k1.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

bool SigHasLowR(const secp256k1_ecdsa_signature* sig);
int ecdsa_signature_parse_der_lax(const secp256k1_context* ctx, secp256k1_ecdsa_signature* sig, const unsigned char* input, size_t inputlen);

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> signature_bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    if (signature_bytes.data() == nullptr) {
        return;
    }
    secp256k1_context* secp256k1_context_verify = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    secp256k1_ecdsa_signature sig_der_lax;
    const bool parsed_der_lax = ecdsa_signature_parse_der_lax(secp256k1_context_verify, &sig_der_lax, signature_bytes.data(), signature_bytes.size()) == 1;
    if (parsed_der_lax) {
        ECC_Start();
        (void)SigHasLowR(&sig_der_lax);
        ECC_Stop();
    }
    secp256k1_context_destroy(secp256k1_context_verify);
}
