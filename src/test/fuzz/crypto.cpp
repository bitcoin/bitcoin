// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hmac_sha256.h>
#include <crypto/hmac_sha512.h>
#include <crypto/ripemd160.h>
#include <crypto/sha1.h>
#include <crypto/sha256.h>
#include <crypto/sha3.h>
#include <crypto/sha512.h>
#include <hash.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

FUZZ_TARGET(crypto)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<uint8_t> data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    if (data.empty()) {
        data.resize(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 4096), fuzzed_data_provider.ConsumeIntegral<uint8_t>());
    }

    CHash160 hash160;
    CHash256 hash256;
    CHMAC_SHA256 hmac_sha256{data.data(), data.size()};
    CHMAC_SHA512 hmac_sha512{data.data(), data.size()};
    CRIPEMD160 ripemd160;
    CSHA1 sha1;
    CSHA256 sha256;
    CSHA512 sha512;
    SHA3_256 sha3;
    CSipHasher sip_hasher{fuzzed_data_provider.ConsumeIntegral<uint64_t>(), fuzzed_data_provider.ConsumeIntegral<uint64_t>()};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 30)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                if (fuzzed_data_provider.ConsumeBool()) {
                    data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                    if (data.empty()) {
                        data.resize(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 4096), fuzzed_data_provider.ConsumeIntegral<uint8_t>());
                    }
                }

                (void)hash160.Write(data);
                (void)hash256.Write(data);
                (void)hmac_sha256.Write(data.data(), data.size());
                (void)hmac_sha512.Write(data.data(), data.size());
                (void)ripemd160.Write(data.data(), data.size());
                (void)sha1.Write(data.data(), data.size());
                (void)sha256.Write(data.data(), data.size());
                (void)sha3.Write(data);
                (void)sha512.Write(data.data(), data.size());
                (void)sip_hasher.Write(data.data(), data.size());

                (void)Hash(data);
                (void)Hash160(data);
                (void)sha512.Size();
            },
            [&] {
                (void)hash160.Reset();
                (void)hash256.Reset();
                (void)ripemd160.Reset();
                (void)sha1.Reset();
                (void)sha256.Reset();
                (void)sha3.Reset();
                (void)sha512.Reset();
            },
            [&] {
                CallOneOf(
                    fuzzed_data_provider,
                    [&] {
                        data.resize(CHash160::OUTPUT_SIZE);
                        hash160.Finalize(data);
                    },
                    [&] {
                        data.resize(CHash256::OUTPUT_SIZE);
                        hash256.Finalize(data);
                    },
                    [&] {
                        data.resize(CHMAC_SHA256::OUTPUT_SIZE);
                        hmac_sha256.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CHMAC_SHA512::OUTPUT_SIZE);
                        hmac_sha512.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CRIPEMD160::OUTPUT_SIZE);
                        ripemd160.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CSHA1::OUTPUT_SIZE);
                        sha1.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CSHA256::OUTPUT_SIZE);
                        sha256.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CSHA512::OUTPUT_SIZE);
                        sha512.Finalize(data.data());
                    },
                    [&] {
                        data.resize(1);
                        data[0] = sip_hasher.Finalize() % 256;
                    },
                    [&] {
                        data.resize(SHA3_256::OUTPUT_SIZE);
                        sha3.Finalize(data);
                    });
            });
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        uint64_t state[25];
        for (size_t i = 0; i < 25; ++i) {
            state[i] = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
        }
        KeccakF(state);
    }
}
