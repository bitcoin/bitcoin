#include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#include <test/fuzz/proto/validation.pb.h>
#include <test/fuzz/proto/convert.h>

#include <consensus/merkle.h>
#include <node/caches.h>
#include <node/chainstate.h>
#include <pow.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <timedata.h>
#include <validation.h>
#include <validationinterface.h>

#include <iostream>

namespace {
const TestingSetup* g_setup;
} // namespace

// WHYYYYY
// const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;
const std::function<void(const std::string&)> G_TEST_LOG_FUN;
const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    return 0;
}

int64_t ClampTime(int64_t time)
{
    static const int64_t time_min{946684801};  // 2000-01-01T00:00:01Z
    static const int64_t time_max{4133980799}; // 2100-12-31T23:59:59Z
    return std::min(time_max,
                    std::max(time, time_min));
}

template <class Proto>
using PostProcessor =
    protobuf_mutator::libfuzzer::PostProcessorRegistration<Proto>;

static PostProcessor<validation_fuzz::ValidationAction> clamp_action_mock_time = {
    [](validation_fuzz::ValidationAction* message, unsigned int seed) {
        // Make sure the atmp mock time lies between a sensible minimum and maximum.
        if (message->has_mock_time()) {
            message->set_mock_time(ClampTime(message->mock_time()));
        }
    }};

static PostProcessor<common_fuzz::Transaction> set_txid = {
    [](common_fuzz::Transaction* message, unsigned int seed) {
        auto tx = ConvertTransaction(*message);
        message->set_txid(tx->GetHash().ToString());
        // TODO sometimes create coinbases, the fuzzer struggles with the commitment
    }};

static PostProcessor<common_fuzz::Block> block_hash_and_merkle = {
    [](common_fuzz::Block* message, unsigned int seed) {
        auto block = ConvertBlock(*message);
        auto mut_header = message->mutable_header();
        mut_header->set_hash(block->GetHash().ToString());
        mut_header->set_merkle_root(BlockMerkleRoot(*block).ToString());

        if (mut_header->hash_prev_block().size() != 64) {
            auto& genesis{Params().GenesisBlock()};
            mut_header->set_hash_prev_block(genesis.GetHash().ToString());

            mut_header->set_bits(genesis.nBits);
            mut_header->set_nonce(genesis.nNonce);
            mut_header->set_version(genesis.nVersion);
            mut_header->set_time(genesis.nTime + 1);
        }

        int max_tries{1000};
        while (!CheckProofOfWork(block->GetHash(), block->nBits, g_setup->m_node.chainman->GetConsensus()) && max_tries-- > 0) {
            ++block->nNonce;
        }

        mut_header->set_nonce(block->nNonce);
    }};

DEFINE_PROTO_FUZZER(const validation_fuzz::FuzzValidation& fuzz_validation)
{
    SetMockTime(ClampTime(0));

    CTxMemPool tx_pool{fuzz_validation.has_mempool_options() ?
                           ConvertMempoolOptions(fuzz_validation.mempool_options()) :
                           CTxMemPool::Options{}};
    const CChainParams& params{Params()};
    ChainstateManager chainman{ChainstateManager::Options{
        .chainparams = params,
        .adjusted_time_callback = GetAdjustedTime,
    }};

    auto cache_sizes = node::CalculateCacheSizes(g_setup->m_args);
    chainman.m_blockman.m_block_tree_db = std::make_unique<CBlockTreeDB>(cache_sizes.block_tree_db, true);

    node::ChainstateLoadOptions options;
    options.mempool = &tx_pool;
    options.block_tree_db_in_memory = true;
    options.coins_db_in_memory = true;
    options.reindex = node::fReindex;
    options.reindex_chainstate = false;
    options.prune = node::fPruneMode;
    options.check_blocks = DEFAULT_CHECKBLOCKS;
    options.check_level = DEFAULT_CHECKLEVEL;
    auto [status, error] = LoadChainstate(chainman, cache_sizes, options);
    assert(status == node::ChainstateLoadStatus::SUCCESS);

    BlockValidationState state;
    assert(chainman.ActiveChainstate().ActivateBestChain(state));

    for (auto action : fuzz_validation.actions()) {
        if (action.has_mock_time()) {
            SetMockTime(action.mock_time());
        }

        if (action.has_process_new_block()) {
            bool new_block{false};
            (void)chainman.ProcessNewBlock(
                /*block=*/ConvertBlock(action.process_new_block().block()),
                /*force_processing=*/action.process_new_block().force(),
                /*min_pow_checked=*/action.process_new_block().min_pow_checked(),
                /*new_block=*/&new_block);
        } else if (action.has_process_new_headers()) {
            std::vector<CBlockHeader> headers;
            for (auto header : action.process_new_headers().headers()) {
                headers.push_back(ConvertHeader(header));
            }

            BlockValidationState state;
            (void)chainman.ProcessNewBlockHeaders(
                /*block=*/headers,
                /*min_pow_checked=*/action.process_new_headers().min_pow_checked(),
                /*state=*/state);
        } else if (action.has_process_transaction()) {
            LOCK(cs_main);
            (void)chainman.ProcessTransaction(
                /*tx=*/ConvertTransaction(action.process_transaction()),
                /*test_accept=*/false);
        }
    }

    chainman.ActiveChainstate().CheckBlockIndex();
}
