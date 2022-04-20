// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockfilter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

FUZZ_TARGET(blockfilter)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    GCSFilter::ElementSet elements;
    size_t num_elements = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 100000);
    for (size_t i = 0; i < num_elements; ++i) {
        elements.emplace(ConsumeRandomLengthIntegralVector<unsigned char>(fuzzed_data_provider, /*max_vector_size=*/32));
    }

    const uint256 block_hash = ConsumeUInt256(fuzzed_data_provider);
    GCSFilter filter({block_hash.GetUint64(0), block_hash.GetUint64(1), BASIC_FILTER_P, BASIC_FILTER_M}, elements);
    const BlockFilter block_filter(BlockFilterType::BASIC, block_hash, filter.GetEncoded());
    (void)block_filter.ComputeHeader(ConsumeUInt256(fuzzed_data_provider));
    (void)block_filter.GetBlockHash();
    (void)block_filter.GetEncodedFilter();
    (void)block_filter.GetHash();
    (void)BlockFilterTypeName(block_filter.GetFilterType());
    {
        const GCSFilter& gcs_filter = block_filter.GetFilter();
        (void)gcs_filter.GetN();
        (void)gcs_filter.GetParams();
        (void)gcs_filter.GetEncoded();
        (void)gcs_filter.Match(ConsumeRandomLengthByteVector(fuzzed_data_provider));
        GCSFilter::ElementSet element_set;
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 30000)
        {
            element_set.insert(ConsumeRandomLengthByteVector(fuzzed_data_provider));
        }
        gcs_filter.MatchAny(element_set);
    }
}
