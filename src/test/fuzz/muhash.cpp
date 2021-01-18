// Copyright (c) 2020 The Bitcoin Core developers
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
    std::vector<uint8_t> data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    std::vector<uint8_t> data2 = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    if (data.empty()) {
        data.resize(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 4096), fuzzed_data_provider.ConsumeIntegral<uint8_t>());
    }
    if (data2.empty()) {
        data2.resize(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 4096), fuzzed_data_provider.ConsumeIntegral<uint8_t>());
    }

    data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    data2 = ConsumeRandomLengthByteVector(fuzzed_data_provider);

    MuHash3072 muhash;

    // Test that MuHash result is consistent independent of order of operations
    muhash.Insert(data);
    muhash.Insert(data2);

    uint256 out;
    muhash.Finalize(out);

    muhash = MuHash3072();
    muhash.Insert(data2);
    muhash.Insert(data);

    uint256 out2;
    muhash.Finalize(out2);

    assert(out == out2);

    // Test that removing all added elements brings the object back to it's initial state
    muhash /= muhash;
    muhash.Finalize(out);

    MuHash3072 muhash2;
    muhash2.Finalize(out2);

    assert(out == out2);
}
