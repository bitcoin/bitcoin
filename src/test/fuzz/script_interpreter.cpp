// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/check.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

bool CastToBool(const std::vector<unsigned char>& vch);

FUZZ_TARGET(script_interpreter)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    {
        const CScript script_code = ConsumeScript(fuzzed_data_provider);
        const std::optional<CMutableTransaction> mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
        if (mtx) {
            const CTransaction tx_to{*mtx};
            const unsigned int in = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
            if (in < tx_to.vin.size()) {
                auto n_hash_type = fuzzed_data_provider.ConsumeIntegral<int>();
                auto amount = ConsumeMoney(fuzzed_data_provider);
                auto sigversion = fuzzed_data_provider.PickValueInArray({SigVersion::BASE, SigVersion::WITNESS_V0});
                (void)SignatureHash(script_code, tx_to, in, n_hash_type, amount, sigversion, nullptr);
                const std::optional<CMutableTransaction> mtx_precomputed = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
                if (mtx_precomputed) {
                    const CTransaction tx_precomputed{*mtx_precomputed};
                    const PrecomputedTransactionData precomputed_transaction_data{tx_precomputed};
                    n_hash_type = fuzzed_data_provider.ConsumeIntegral<int>();
                    amount = ConsumeMoney(fuzzed_data_provider);
                    sigversion = fuzzed_data_provider.PickValueInArray({SigVersion::BASE, SigVersion::WITNESS_V0});
                    (void)SignatureHash(script_code, tx_to, in, n_hash_type, amount, sigversion, &precomputed_transaction_data);
                }
            }
        }
    }
    {
        (void)CastToBool(ConsumeRandomLengthByteVector(fuzzed_data_provider));
    }
}

/** Differential fuzzing for SignatureHash with and without cache. */
FUZZ_TARGET(sighash_cache)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());

    // Get inputs to the sighash function that won't change across types.
    const auto scriptcode{ConsumeScript(provider)};
    const auto tx{ConsumeTransaction(provider, std::nullopt)};
    if (tx.vin.empty()) return;
    const auto in_index{provider.ConsumeIntegralInRange<uint32_t>(0, tx.vin.size() - 1)};
    const auto amount{ConsumeMoney(provider)};
    const auto sigversion{(SigVersion)provider.ConsumeIntegralInRange(0, 1)};

    // Check the sighash function will give the same result for 100 fuzzer-generated hash types whether or not a cache is
    // provided. The cache is conserved across types to exercise cache hits.
    SigHashCache sighash_cache{};
    for (int i{0}; i < 100; ++i) {
        const auto hash_type{((i & 2) == 0) ? provider.ConsumeIntegral<int8_t>() : provider.ConsumeIntegral<int32_t>()};
        const auto nocache_res{SignatureHash(scriptcode, tx, in_index, hash_type, amount, sigversion)};
        const auto cache_res{SignatureHash(scriptcode, tx, in_index, hash_type, amount, sigversion, nullptr, &sighash_cache)};
        Assert(nocache_res == cache_res);
    }
}
