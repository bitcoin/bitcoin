// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <core_memusage.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/chaintype.h>
#include <util/check.h>
#include <validation.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ios>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

void initialize_block()
{
    SelectParams(ChainType::REGTEST);
}

static void CheckStrippedBlock(const CBlock& block, std::span<const std::byte> stripped)
{
    assert(std::ranges::equal(stripped, DataStream{} << TX_NO_WITNESS(block)));
}

FUZZ_TARGET(block, .init = initialize_block)
{
    std::optional<std::vector<std::byte>> stripped;
    try {
        const auto bytes{std::as_bytes(buffer)};
        stripped = node::StripBlockWitness({bytes.begin(), bytes.end()});
    } catch (const std::ios_base::failure&) {
    }
    CBlock block;
    try {
        SpanReader{buffer} >> TX_WITH_WITNESS(block);
    } catch (const std::ios_base::failure&) {
        assert(!stripped);
        return;
    }
    CheckStrippedBlock(block, *Assert(stripped));
    const Consensus::Params& consensus_params = Params().GetConsensus();
    BlockValidationState validation_state_pow_and_merkle;
    const bool valid_incl_pow_and_merkle = CheckBlock(block, validation_state_pow_and_merkle, consensus_params, /* fCheckPOW= */ true, /* fCheckMerkleRoot= */ true);
    assert(validation_state_pow_and_merkle.IsValid() || validation_state_pow_and_merkle.IsInvalid() || validation_state_pow_and_merkle.IsError());
    (void)validation_state_pow_and_merkle.Error("");
    BlockValidationState validation_state_pow;
    const bool valid_incl_pow = CheckBlock(block, validation_state_pow, consensus_params, /* fCheckPOW= */ true, /* fCheckMerkleRoot= */ false);
    assert(validation_state_pow.IsValid() || validation_state_pow.IsInvalid() || validation_state_pow.IsError());
    BlockValidationState validation_state_merkle;
    const bool valid_incl_merkle = CheckBlock(block, validation_state_merkle, consensus_params, /* fCheckPOW= */ false, /* fCheckMerkleRoot= */ true);
    assert(validation_state_merkle.IsValid() || validation_state_merkle.IsInvalid() || validation_state_merkle.IsError());
    BlockValidationState validation_state_none;
    const bool valid_incl_none = CheckBlock(block, validation_state_none, consensus_params, /* fCheckPOW= */ false, /* fCheckMerkleRoot= */ false);
    assert(validation_state_none.IsValid() || validation_state_none.IsInvalid() || validation_state_none.IsError());
    if (valid_incl_pow_and_merkle) {
        assert(valid_incl_pow && valid_incl_merkle && valid_incl_none);
    } else if (valid_incl_merkle || valid_incl_pow) {
        assert(valid_incl_none);
    }
    (void)block.GetHash();
    (void)block.ToString();
    (void)BlockMerkleRoot(block);
    if (!block.vtx.empty()) {
        (void)BlockWitnessMerkleRoot(block);
    }
    (void)GetBlockWeight(block);
    (void)GetWitnessCommitmentIndex(block);
    const size_t raw_memory_size = RecursiveDynamicUsage(block);
    const size_t raw_memory_size_as_shared_ptr = RecursiveDynamicUsage(std::make_shared<CBlock>(block));
    assert(raw_memory_size_as_shared_ptr > raw_memory_size);
    CBlock block_copy = block;
    block_copy.SetNull();
    const bool is_null = block_copy.IsNull();
    assert(is_null);
}

FUZZ_TARGET(block_witness_roundtrip)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    CBlock block;
    LIMITED_WHILE (provider.ConsumeBool(), 16) {
        auto tx{ConsumeTransaction(provider, std::nullopt)};
        if (tx.vin.empty()) tx.vout.clear(); // Empty vin with nonempty vout is ambiguous in witness serialization
        block.vtx.push_back(MakeTransactionRef(std::move(tx)));
    }
    DataStream data;
    data << TX_WITH_WITNESS(block);
    data.write(ConsumeRandomLengthByteVector<std::byte>(provider)); // Decode-and-reserialize drops trailing bytes, so stripping must too
    CBlock decoded;
    SpanReader{data} >> TX_WITH_WITNESS(decoded);
    const auto stripped{node::StripBlockWitness({data.begin(), data.end()})};
    CheckStrippedBlock(block, stripped);
    CheckStrippedBlock(decoded, stripped);
    assert(node::StripBlockWitness(stripped) == stripped);
}
