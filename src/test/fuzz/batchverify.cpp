// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <batchverify.h>
#include <pubkey.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cassert>

void initialize_batchverify()
{
    static ECC_Context ecc_context{};
}

FUZZ_TARGET(batchverify, .init = initialize_batchverify)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    BatchSchnorrVerifier batch{};

    bool all_valid{true};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 32) {
        const CKey privkey{ConsumePrivateKey(fuzzed_data_provider, /*compressed=*/true)};
        if (!privkey.IsValid()) continue;
        const XOnlyPubKey pubkey{privkey.GetPubKey()};

        const uint256 sighash{ConsumeUInt256(fuzzed_data_provider)};
        std::vector<unsigned char> sig(64);
        if (!privkey.SignSchnorr(sighash, sig, /*merkle_root=*/nullptr, /*aux=*/uint256::ZERO)) continue;

        if (fuzzed_data_provider.ConsumeBool()) {
            const size_t bit{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, sig.size() * 8 - 1)};
            sig[bit / 8] ^= static_cast<unsigned char>(1u << (bit % 8));
            all_valid = false;
        }

        // The pubkey is derived from a valid private key, so it always parses.
        const bool added{batch.Add(sig, pubkey, sighash)};
        assert(added);
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        assert(batch.Verify() == all_valid);
    } else {
        batch.Reset();
    }
}
