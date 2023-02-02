#include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#include <test/fuzz/proto/atmp.pb.h>
#include <test/fuzz/proto/convert.h>
#include <test/fuzz/proto/mempool_options.pb.h>
#include <test/fuzz/proto/transaction.pb.h>

#include <primitives/transaction.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <validation.h>
#include <validationinterface.h>

#include <chrono>
#include <random>

// WHYYYYY
// const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;
const std::function<void(const std::string&)> G_TEST_LOG_FUN;
const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS;

const TestingSetup* g_setup;
std::vector<COutPoint> g_outpoints_coinbase_init_mature;
std::vector<COutPoint> g_outpoints_coinbase_init_immature;

struct MockedTxPool : public CTxMemPool {
    void RollingFeeUpdate() EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        lastRollingFeeUpdate = GetTime();
        blockSinceLastRollingFeeBump = true;
    }
};

class MockedChainstate final : public Chainstate
{
public:
    void SetMempool(CTxMemPool* mempool)
    {
        m_mempool = mempool;
    }
};

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();

    for (int i = 0; i < 2 * COINBASE_MATURITY; ++i) {
        CTxIn in = MineBlock(g_setup->m_node, P2WSH_OP_TRUE);
        // Remember the txids to avoid expensive disk access later on
        auto& outpoints = i < COINBASE_MATURITY ?
                              g_outpoints_coinbase_init_mature :
                              g_outpoints_coinbase_init_immature;
        outpoints.push_back(in.prevout);
    }
    SyncWithValidationInterfaceQueue();
    return 0;
}

template <class Proto>
using PostProcessor =
    protobuf_mutator::libfuzzer::PostProcessorRegistration<Proto>;

static PostProcessor<common_fuzz::Transaction> post_proc_txid = {
    [](common_fuzz::Transaction* message, unsigned int seed) {
        // This is not its own post processor because changing the prevouts affects the txid.
        // TODO can this be its own post proc?
        auto post_proc_prev_out = [](common_fuzz::PrevOut* message, unsigned int seed) {
            std::minstd_rand rnd;
            rnd.seed(seed);

            if (!std::uniform_int_distribution<uint16_t>(0, 20)(rnd) || message->txid().empty()) {
                // TODO 5% chance is kinda random
                COutPoint& outpoint = rnd() % 2 == 0 ?
                                          g_outpoints_coinbase_init_mature[rnd() % g_outpoints_coinbase_init_mature.size()] :
                                          g_outpoints_coinbase_init_immature[rnd() % g_outpoints_coinbase_init_immature.size()];
                message->set_txid(outpoint.hash.ToString());
                message->set_n(outpoint.n);
            }
        };

        auto* inputs = message->mutable_inputs();
        for (auto& input : *inputs) {
            post_proc_prev_out(input.mutable_prev_out(), seed);
            auto* witnesses = input.mutable_witness_stack();
            witnesses->erase(witnesses->begin(), witnesses->end());
            *witnesses->Add() = {WITNESS_STACK_ELEM_OP_TRUE.begin(), WITNESS_STACK_ELEM_OP_TRUE.end()};
        }

        auto post_proc_tx_output = [](common_fuzz::TxOutput* message, unsigned int seed) {
            std::minstd_rand rnd;
            rnd.seed(seed);
            message->mutable_script_pub_key()->set_raw(
                {P2WSH_OP_TRUE.begin(), P2WSH_OP_TRUE.end()});
        };
        auto* outputs = message->mutable_outputs();
        for (auto& output : *outputs) {
            post_proc_tx_output(&output, seed);
        }

        CTransactionRef tx = ConvertTransaction(*message);
        uint256 txid{tx->GetHash()};
        message->set_txid(txid.ToString());
    }};

int64_t ClampTime(int64_t time)
{
    const auto* active_tip = WITH_LOCK(
        cs_main, return g_setup->m_node.chainman->ActiveChain().Tip());
    return std::min((int64_t)std::numeric_limits<decltype(active_tip->nTime)>::max(),
                    std::max(time, active_tip->GetMedianTimePast() + 1));
}
static PostProcessor<atmp_fuzz::AcceptToMemoryPool> post_proc_atmp_mock_time = {
    [](atmp_fuzz::AcceptToMemoryPool* message, unsigned int seed) {
        // Make sure the atmp mock time lies between a sensible minimum and maximum.
        if (message->has_mock_time()) {
            message->set_mock_time(ClampTime(message->mock_time()));
        }
    }};

DEFINE_PROTO_FUZZER(const atmp_fuzz::Mempool& mempool)
{
    const auto& node = g_setup->m_node;
    auto& chainstate{static_cast<MockedChainstate&>(node.chainman->ActiveChainstate())};

    SetMockTime(ClampTime(0));

    CTxMemPool tx_pool_{ConvertMempoolOptions(mempool.options())};
    MockedTxPool& tx_pool = *static_cast<MockedTxPool*>(&tx_pool_);

    chainstate.SetMempool(&tx_pool);

    for (const auto& atmp_event : mempool.atmp_events()) {
        if (atmp_event.has_mock_time()) {
            SetMockTime(atmp_event.mock_time());
        }

        if (atmp_event.rolling_fee_update()) {
            tx_pool.RollingFeeUpdate();
        }

        if (atmp_event.has_prioritise_delta()) {
            // TODO does this only make sense when also assembling a block?
        }

        if (atmp_event.has_transaction()) {
            auto tx = ConvertTransaction(atmp_event.transaction());
            WITH_LOCK(::cs_main,
                      return AcceptToMemoryPool(chainstate, tx, GetTime(),
                                                atmp_event.bypass_limits(),
                                                /*test_accept=*/false));
        }

        if (atmp_event.has_pkg()) {
            Package pkg;
            for (const auto& tx : atmp_event.pkg().transactions()) {
                pkg.push_back(ConvertTransaction(tx));
            }

            if (!pkg.empty()) {
                WITH_LOCK(::cs_main,
                          return ProcessNewPackage(chainstate, tx_pool, pkg,
                                                   /*test_accept=*/false));
            }
        }
    }

    if (GetMainSignals().CallbacksPending() > 0) {
        SyncWithValidationInterfaceQueue();
    }

    WITH_LOCK(::cs_main, tx_pool.check(chainstate.CoinsTip(), chainstate.m_chain.Height() + 1));
}
