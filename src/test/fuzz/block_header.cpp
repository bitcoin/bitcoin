// Copyright (c) 2020 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::optional<CBlockHeader> block_header = ConsumeDeserializable<CBlockHeader>(fuzzed_data_provider);
    if (!block_header) {
        return;
    }
    {
        const uint256 hash = block_header->GetHash();
        static const uint256 u256_max(uint256S("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
        assert(hash != u256_max);
        assert(block_header->GetBlockTime() == block_header->nTime);
        assert(block_header->IsNull() == (block_header->nBits == 0));
    }
    {
        CBlockHeader mut_block_header = *block_header;
        mut_block_header.SetNull();
        assert(mut_block_header.IsNull());
        CBlock block{*block_header};
        assert(block.GetBlockHeader().GetHash() == block_header->GetHash());
        (void)block.ToString();
        block.SetNull();
        assert(block.GetBlockHeader().GetHash() == mut_block_header.GetHash());
    }
    {
        std::optional<CBlockLocator> block_locator = ConsumeDeserializable<CBlockLocator>(fuzzed_data_provider);
        if (block_locator) {
            (void)block_locator->IsNull();
            block_locator->SetNull();
            assert(block_locator->IsNull());
        }
    }
}
