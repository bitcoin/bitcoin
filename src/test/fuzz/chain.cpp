// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <optional>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    std::optional<CDiskBlockIndex> disk_block_index = ConsumeDeserializable<CDiskBlockIndex>(fuzzed_data_provider);
    if (!disk_block_index) {
        return;
    }

    const uint256 zero{};
    disk_block_index->phashBlock = &zero;
    (void)disk_block_index->GetBlockHash();
    (void)disk_block_index->GetBlockPos();
    (void)disk_block_index->GetBlockTime();
    (void)disk_block_index->GetBlockTimeMax();
    (void)disk_block_index->GetMedianTimePast();
    (void)disk_block_index->GetUndoPos();
    (void)disk_block_index->HaveTxsDownloaded();
    (void)disk_block_index->IsValid();
    (void)disk_block_index->ToString();

    const CBlockHeader block_header = disk_block_index->GetBlockHeader();
    (void)CDiskBlockIndex{*disk_block_index};
    (void)disk_block_index->BuildSkip();

    while (fuzzed_data_provider.ConsumeBool()) {
        const BlockStatus block_status = fuzzed_data_provider.PickValueInArray({
            BlockStatus::BLOCK_VALID_UNKNOWN,
            BlockStatus::BLOCK_VALID_RESERVED,
            BlockStatus::BLOCK_VALID_TREE,
            BlockStatus::BLOCK_VALID_TRANSACTIONS,
            BlockStatus::BLOCK_VALID_CHAIN,
            BlockStatus::BLOCK_VALID_SCRIPTS,
            BlockStatus::BLOCK_VALID_MASK,
            BlockStatus::BLOCK_HAVE_DATA,
            BlockStatus::BLOCK_HAVE_UNDO,
            BlockStatus::BLOCK_HAVE_MASK,
            BlockStatus::BLOCK_FAILED_VALID,
            BlockStatus::BLOCK_FAILED_CHILD,
            BlockStatus::BLOCK_FAILED_MASK,
            BlockStatus::BLOCK_OPT_WITNESS,
        });
        if (block_status & ~BLOCK_VALID_MASK) {
            continue;
        }
        (void)disk_block_index->RaiseValidity(block_status);
    }

    CBlockIndex block_index{block_header};
    block_index.phashBlock = &zero;
    (void)block_index.GetBlockHash();
    (void)block_index.ToString();
}
