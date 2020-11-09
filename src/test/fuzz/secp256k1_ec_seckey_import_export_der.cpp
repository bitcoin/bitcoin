// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <secp256k1.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

int ec_seckey_import_der(const secp256k1_context* ctx, unsigned char* out32, const unsigned char* seckey, size_t seckeylen);
int ec_seckey_export_der(const secp256k1_context* ctx, unsigned char* seckey, size_t* seckeylen, const unsigned char* key32, bool compressed);

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    secp256k1_context* secp256k1_context_sign = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    {
        std::vector<uint8_t> out32(32);
        (void)ec_seckey_import_der(secp256k1_context_sign, out32.data(), ConsumeFixedLengthByteVector(fuzzed_data_provider, CKey::SIZE).data(), CKey::SIZE);
    }
    {
        std::vector<uint8_t> seckey(CKey::SIZE);
        const std::vector<uint8_t> key32 = ConsumeFixedLengthByteVector(fuzzed_data_provider, 32);
        size_t seckeylen = CKey::SIZE;
        const bool compressed = fuzzed_data_provider.ConsumeBool();
        const bool exported = ec_seckey_export_der(secp256k1_context_sign, seckey.data(), &seckeylen, key32.data(), compressed);
        if (exported) {
            std::vector<uint8_t> out32(32);
            const bool imported = ec_seckey_import_der(secp256k1_context_sign, out32.data(), seckey.data(), seckey.size()) == 1;
            assert(imported && key32 == out32);
        }
    }
    secp256k1_context_destroy(secp256k1_context_sign);
}
