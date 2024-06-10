// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/muhash.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <vector>

FUZZ_TARGET(muhash)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<uint8_t> data{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
    std::vector<uint8_t> data2{ConsumeRandomLengthByteVector(fuzzed_data_provider)};

    MuHash3072 muhash;

    muhash.Insert(data);
    muhash.Insert(data2);

    const std::string initial_state_hash{"dd5ad2a105c2d29495f577245c357409002329b9f4d6182c0af3dc2f462555c8"};
    uint256 out;
    uint256 out2;
    CallOneOf(
        fuzzed_data_provider,
        [&] {
            // Test that MuHash result is consistent independent of order of operations
            muhash.Finalize(out);

            muhash = MuHash3072();
            muhash.Insert(data2);
            muhash.Insert(data);
            muhash.Finalize(out2);
        },
        [&] {
            // Test that multiplication with the initial state never changes the finalized result
            muhash.Finalize(out);
            MuHash3072 muhash3;
            muhash3 *= muhash;
            muhash3.Finalize(out2);
        },
        [&] {
            // Test that dividing a MuHash by itself brings it back to it's initial state

            // See note about clang + self-assignment in test/uint256_tests.cpp
            #if defined(__clang__)
            #    pragma clang diagnostic push
            #    pragma clang diagnostic ignored "-Wself-assign-overloaded"
            #endif

            muhash /= muhash;

            #if defined(__clang__)
            #    pragma clang diagnostic pop
            #endif

            muhash.Finalize(out);
            out2 = uint256S(initial_state_hash);
        },
        [&] {
            // Test that removing all added elements brings the object back to it's initial state
            muhash.Remove(data);
            muhash.Remove(data2);
            muhash.Finalize(out);
            out2 = uint256S(initial_state_hash);
        });
    assert(out == out2);
}
