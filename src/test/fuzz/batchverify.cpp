// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <batchverify.h>
#include <pubkey.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

void initialize_batchverify()
{
    static ECC_Context ecc_context{};
}

FUZZ_TARGET(batchverify, .init = initialize_batchverify)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    BatchSchnorrVerifier batch{};

    while (fuzzed_data_provider.ConsumeBool()) {
        CKey privkey{ConsumePrivateKey(fuzzed_data_provider)};
        if (!privkey.IsValid()) continue;
        XOnlyPubKey pubkey{privkey.GetPubKey()};

        std::vector<unsigned char> sig{ConsumeFixedLengthByteVector(fuzzed_data_provider, 64)};
        uint256 sighash{ConsumeUInt256(fuzzed_data_provider)};

        (void)batch.Add(sig, pubkey, sighash);
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        (void)batch.Verify();
    } else {
        batch.Reset();
    }
}
