// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <primitives/block.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

void initialize_pow()
{
    SelectParams(CBaseChainParams::MAIN);
}

FUZZ_TARGET_INIT(pow, initialize_pow)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const Consensus::Params& consensus_params = Params().GetConsensus();
    std::vector<CBlockIndex> blocks;
    const uint32_t fixed_time = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    const uint32_t fixed_bits = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    while (fuzzed_data_provider.remaining_bytes() > 0) {
        const std::optional<CBlockHeader> block_header = ConsumeDeserializable<CBlockHeader>(fuzzed_data_provider);
        if (!block_header) {
            continue;
        }
        CBlockIndex current_block{*block_header};
        {
            CBlockIndex* previous_block = blocks.empty() ? nullptr : &PickValue(fuzzed_data_provider, blocks);
            const int current_height = (previous_block != nullptr && previous_block->nHeight != std::numeric_limits<int>::max()) ? previous_block->nHeight + 1 : 0;
            if (fuzzed_data_provider.ConsumeBool()) {
                current_block.pprev = previous_block;
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                current_block.nHeight = current_height;
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                const uint32_t seconds = current_height * consensus_params.nPowTargetSpacing;
                if (!AdditionOverflow(fixed_time, seconds)) {
                    current_block.nTime = fixed_time + seconds;
                }
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                current_block.nBits = fixed_bits;
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                current_block.nChainWork = previous_block != nullptr ? previous_block->nChainWork + GetBlockProof(*previous_block) : arith_uint256{0};
            } else {
                current_block.nChainWork = ConsumeArithUInt256(fuzzed_data_provider);
            }
            blocks.push_back(current_block);
        }
        {
            (void)GetBlockProof(current_block);
            (void)CalculateNextWorkRequired(&current_block, fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, std::numeric_limits<int64_t>::max()), consensus_params);
            if (current_block.nHeight != std::numeric_limits<int>::max() && current_block.nHeight - (consensus_params.DifficultyAdjustmentInterval(current_block.nHeight) - 1) >= 0) {
                (void)GetNextWorkRequired(&current_block, &(*block_header), consensus_params);
            }
        }
        {
            const CBlockIndex* to = &PickValue(fuzzed_data_provider, blocks);
            const CBlockIndex* from = &PickValue(fuzzed_data_provider, blocks);
            const CBlockIndex* tip = &PickValue(fuzzed_data_provider, blocks);
            try {
                (void)GetBlockProofEquivalentTime(*to, *from, *tip, consensus_params);
            } catch (const uint_error&) {
            }
        }
        {
            const std::optional<uint256> hash = ConsumeDeserializable<uint256>(fuzzed_data_provider);
            if (hash) {
                (void)CheckProofOfWork(*hash, fuzzed_data_provider.ConsumeIntegral<unsigned int>(), consensus_params);
            }
        }
    }
}
