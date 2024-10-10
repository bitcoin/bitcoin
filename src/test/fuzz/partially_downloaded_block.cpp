#include <blockencodings.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/translation.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace {
const TestingSetup* g_setup;
} // namespace

void initialize_pdb()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

PartiallyDownloadedBlock::CheckBlockFn FuzzedCheckBlock(std::optional<BlockValidationResult> result)
{
    return [result](const CBlock&, BlockValidationState& state, const Consensus::Params&, bool, bool) {
        if (result) {
            return state.Invalid(*result);
        }

        return true;
    };
}

FUZZ_TARGET(partially_downloaded_block, .init = initialize_pdb)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    auto block{ConsumeDeserializable<CBlock>(fuzzed_data_provider, TX_WITH_WITNESS)};
    if (!block || block->vtx.size() == 0 ||
        block->vtx.size() >= std::numeric_limits<uint16_t>::max()) {
        return;
    }

    CBlockHeaderAndShortTxIDs cmpctblock{*block, fuzzed_data_provider.ConsumeIntegral<uint64_t>()};

    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    Assert(error.empty());
    PartiallyDownloadedBlock pdb{&pool};

    // Set of available transactions (mempool or extra_txn)
    std::set<uint16_t> available;
    // The coinbase is always available
    available.insert(0);

    std::vector<CTransactionRef> extra_txn;
    for (size_t i = 1; i < block->vtx.size(); ++i) {
        auto tx{block->vtx[i]};

        bool add_to_extra_txn{fuzzed_data_provider.ConsumeBool()};
        bool add_to_mempool{fuzzed_data_provider.ConsumeBool()};

        if (add_to_extra_txn) {
            extra_txn.emplace_back(tx);
            available.insert(i);
        }

        if (add_to_mempool && !pool.exists(GenTxid::Txid(tx->GetHash()))) {
            LOCK2(cs_main, pool.cs);
            AddToMempool(pool, ConsumeTxMemPoolEntry(fuzzed_data_provider, *tx));
            available.insert(i);
        }
    }

    auto init_status{pdb.InitData(cmpctblock, extra_txn)};

    std::vector<CTransactionRef> missing;
    // Whether we skipped a transaction that should be included in `missing`.
    // FillBlock should never return READ_STATUS_OK if that is the case.
    bool skipped_missing{false};
    for (size_t i = 0; i < cmpctblock.BlockTxCount(); i++) {
        // If init_status == READ_STATUS_OK then a available transaction in the
        // compact block (i.e. IsTxAvailable(i) == true) implies that we marked
        // that transaction as available above (i.e. available.count(i) > 0).
        // The reverse is not true, due to possible compact block short id
        // collisions (i.e. available.count(i) > 0 does not imply
        // IsTxAvailable(i) == true).
        if (init_status == READ_STATUS_OK) {
            assert(!pdb.IsTxAvailable(i) || available.count(i) > 0);
        }

        bool skip{fuzzed_data_provider.ConsumeBool()};
        if (!pdb.IsTxAvailable(i) && !skip) {
            missing.push_back(block->vtx[i]);
        }

        skipped_missing |= (!pdb.IsTxAvailable(i) && skip);
    }

    // Mock CheckBlock
    bool fail_check_block{fuzzed_data_provider.ConsumeBool()};
    auto validation_result =
        fuzzed_data_provider.PickValueInArray(
            {BlockValidationResult::BLOCK_RESULT_UNSET,
             BlockValidationResult::BLOCK_CONSENSUS,
             BlockValidationResult::BLOCK_RECENT_CONSENSUS_CHANGE,
             BlockValidationResult::BLOCK_CACHED_INVALID,
             BlockValidationResult::BLOCK_INVALID_HEADER,
             BlockValidationResult::BLOCK_MUTATED,
             BlockValidationResult::BLOCK_MISSING_PREV,
             BlockValidationResult::BLOCK_INVALID_PREV,
             BlockValidationResult::BLOCK_TIME_FUTURE,
             BlockValidationResult::BLOCK_CHECKPOINT,
             BlockValidationResult::BLOCK_HEADER_LOW_WORK});
    pdb.m_check_block_mock = FuzzedCheckBlock(
        fail_check_block ?
            std::optional<BlockValidationResult>{validation_result} :
            std::nullopt);

    CBlock reconstructed_block;
    auto fill_status{pdb.FillBlock(reconstructed_block, missing)};
    switch (fill_status) {
    case READ_STATUS_OK:
        assert(!skipped_missing);
        assert(!fail_check_block);
        assert(block->GetHash() == reconstructed_block.GetHash());
        break;
    case READ_STATUS_CHECKBLOCK_FAILED: [[fallthrough]];
    case READ_STATUS_FAILED:
        assert(fail_check_block);
        break;
    case READ_STATUS_INVALID:
        break;
    }
}
