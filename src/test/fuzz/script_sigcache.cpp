// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <key.h>
#include <pubkey.h>
#include <script/sigcache.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;
SignatureCache* g_signature_cache;
} // namespace

void initialize_script_sigcache()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    static SignatureCache signature_cache{DEFAULT_SIGNATURE_CACHE_BYTES};
    g_setup = testing_setup.get();
    g_signature_cache = &signature_cache;
}

FUZZ_TARGET(script_sigcache, .init = initialize_script_sigcache)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const std::optional<CMutableTransaction> mutable_transaction = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
    const CTransaction tx{mutable_transaction ? *mutable_transaction : CMutableTransaction{}};
    const unsigned int n_in = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    const CAmount amount = ConsumeMoney(fuzzed_data_provider);
    const bool store = fuzzed_data_provider.ConsumeBool();
    PrecomputedTransactionData tx_data;
    CachingTransactionSignatureChecker caching_transaction_signature_checker{mutable_transaction ? &tx : nullptr, n_in, amount, store, *g_signature_cache, tx_data};
    if (fuzzed_data_provider.ConsumeBool()) {
        const auto random_bytes = fuzzed_data_provider.ConsumeBytes<unsigned char>(64);
        const XOnlyPubKey pub_key(ConsumeUInt256(fuzzed_data_provider));
        if (random_bytes.size() == 64) {
            (void)caching_transaction_signature_checker.VerifySchnorrSignature(random_bytes, pub_key, ConsumeUInt256(fuzzed_data_provider));
        }
    } else {
        const auto random_bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
        const auto pub_key = ConsumeDeserializable<CPubKey>(fuzzed_data_provider);
        if (pub_key) {
            if (!random_bytes.empty()) {
                (void)caching_transaction_signature_checker.VerifyECDSASignature(random_bytes, *pub_key, ConsumeUInt256(fuzzed_data_provider));
            }
        }
    }
}
