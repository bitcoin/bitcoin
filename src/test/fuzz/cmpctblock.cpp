// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <node/miner.h>
#include <policy/truc_policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <script/script.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/mining.h>
#include <test/util/net.h>
#include <test/util/random.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <test/util/validation.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>

#include <boost/multi_index/detail/hash_index_iterator.hpp>
#include <boost/operators.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

TestingSetup* g_setup;

//! Fee each created tx will pay.
const CAmount AMOUNT_FEE{1000};
//! Cached coinbases that each iteration can copy and use.
std::vector<std::pair<COutPoint, CAmount>> g_mature_coinbase;

void ResetChainmanAndMempool(TestingSetup& setup)
{
    SetMockTime(Params().GenesisBlock().Time());

    bilingual_str error{};
    setup.m_node.mempool.reset();
    setup.m_node.mempool = std::make_unique<CTxMemPool>(MemPoolOptionsForTest(setup.m_node), error);
    Assert(error.empty());

    setup.m_node.chainman.reset();
    setup.m_make_chainman();
    setup.LoadVerifyActivateChainstate();

    node::BlockAssembler::Options options;
    options.coinbase_output_script = P2WSH_OP_TRUE;
    options.include_dummy_extranonce = true;

    g_mature_coinbase.clear();

    for (int i = 0; i < 2 * COINBASE_MATURITY; ++i) {
        COutPoint prevout{MineBlock(setup.m_node, options)};
        if (i < COINBASE_MATURITY) {
            LOCK(cs_main);
            CAmount subsidy{setup.m_node.chainman->ActiveChainstate().CoinsTip().GetCoin(prevout)->out.nValue};
            g_mature_coinbase.emplace_back(prevout, subsidy);
        }
    }
}

} // namespace

extern void MakeRandDeterministicDANGEROUS(const uint256& seed) noexcept;

void initialize_cmpctblock()
{
    static const auto testing_setup = MakeNoLogFileContext<TestingSetup>();
    g_setup = testing_setup.get();
    ResetChainmanAndMempool(*g_setup);
}

FUZZ_TARGET(cmpctblock, .init = initialize_cmpctblock)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    SetMockTime(1610000000);

    auto setup = g_setup;
    auto& mempool = *setup->m_node.mempool;
    auto& chainman = static_cast<TestChainstateManager&>(*setup->m_node.chainman);
    chainman.ResetIbd();
    chainman.DisableNextWrite();

    AddrMan addrman{*setup->m_node.netgroupman, /*deterministic=*/true, /*consistency_check_ratio=*/0};
    auto& connman = *static_cast<ConnmanTestMsg*>(setup->m_node.connman.get());
    auto peerman = PeerManager::make(connman, addrman,
                                     /*banman=*/nullptr, chainman,
                                     mempool, *setup->m_node.warnings,
                                     PeerManager::Options{
                                         .deterministic_rng = true,
                                     });
    connman.SetMsgProc(peerman.get());

    setup->m_node.validation_signals->RegisterValidationInterface(peerman.get());
    setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();

    LOCK(NetEventsInterface::g_msgproc_mutex);

    std::vector<CNode*> peers;
    for (int i = 0; i < 4; ++i) {
        peers.push_back(ConsumeNodeAsUniquePtr(fuzzed_data_provider, i).release());
        CNode& p2p_node = *peers.back();
        FillNode(fuzzed_data_provider, connman, p2p_node);
        connman.AddTestNode(p2p_node);
    }

    // Coinbase UTXOs for this iteration.
    std::vector<std::pair<COutPoint, CAmount>> mature_coinbase = g_mature_coinbase;

    const uint64_t initial_sequence{WITH_LOCK(mempool.cs, return mempool.GetSequence())};

    auto create_tx = [&]() -> CTransactionRef {
        CMutableTransaction tx_mut;
        tx_mut.version = fuzzed_data_provider.ConsumeBool() ? CTransaction::CURRENT_VERSION : TRUC_VERSION;
        tx_mut.nLockTime = fuzzed_data_provider.ConsumeBool() ? 0 : fuzzed_data_provider.ConsumeIntegral<uint32_t>();

        // If the mempool is non-empty, choose a mempool outpoint. Otherwise, choose a coinbase.
        CAmount amount_in;
        COutPoint outpoint;
        unsigned long mempool_size = mempool.size();
        if (fuzzed_data_provider.ConsumeBool() && mempool_size != 0) {
            size_t random_idx = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, mempool_size - 1);
            CTransactionRef tx = WITH_LOCK(mempool.cs, return mempool.txns_randomized[random_idx].second->GetSharedTx(););
            outpoint = COutPoint(tx->GetHash(), 0);
            amount_in = tx->vout[0].nValue;
        } else {
            auto coinbase_it = mature_coinbase.begin();
            std::advance(coinbase_it, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, mature_coinbase.size() - 1));
            outpoint = coinbase_it->first;
            amount_in = coinbase_it->second;
        }

        const auto sequence = ConsumeSequence(fuzzed_data_provider);
        const auto script_sig = CScript{};
        const auto script_wit_stack = std::vector<std::vector<uint8_t>>{WITNESS_STACK_ELEM_OP_TRUE};

        CTxIn in;
        in.prevout = outpoint;
        in.nSequence = sequence;
        in.scriptSig = script_sig;
        in.scriptWitness.stack = script_wit_stack;
        tx_mut.vin.push_back(in);

        const CAmount amount_out = amount_in - AMOUNT_FEE;
        tx_mut.vout.emplace_back(amount_out, P2WSH_OP_TRUE);

        auto tx = MakeTransactionRef(tx_mut);
        return tx;
    };

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 1000)
    {
        CSerializedNetMsg net_msg;
        bool sent_net_msg = true;
        bool requested_hb = false;
        bool sent_sendcmpct = false;
        bool valid_sendcmpct = false;

        CallOneOf(
            fuzzed_data_provider,
            [&]() {
                // Send a sendcmpct message, optionally setting hb mode.
                bool hb = fuzzed_data_provider.ConsumeBool();
                uint64_t version{fuzzed_data_provider.ConsumeBool() ? CMPCTBLOCKS_VERSION : fuzzed_data_provider.ConsumeIntegral<uint64_t>()};
                net_msg = NetMsg::Make(NetMsgType::SENDCMPCT, /*high_bandwidth=*/hb, /*version=*/version);
                requested_hb = hb;
                sent_sendcmpct = true;
                valid_sendcmpct = version == CMPCTBLOCKS_VERSION;
            },
            [&]() {
                // Send a transaction.
                CTransactionRef tx = create_tx();
                net_msg = NetMsg::Make(NetMsgType::TX, TX_WITH_WITNESS(*tx));
            },
            [&]() {
                // Set mock time randomly or to tip's time.
                if (fuzzed_data_provider.ConsumeBool()) {
                    SetMockTime(ConsumeTime(fuzzed_data_provider));
                } else {
                    const uint64_t tip_time = WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()->GetBlockTime());
                    SetMockTime(tip_time);
                }

                sent_net_msg = false;
            });

        if (!sent_net_msg) {
            continue;
        }

        CNode& random_node = *PickValue(fuzzed_data_provider, peers);
        connman.FlushSendBuffer(random_node);
        (void)connman.ReceiveMsgFrom(random_node, std::move(net_msg));

        bool more_work{true};
        while (more_work) {
            random_node.fPauseSend = false;

            more_work = connman.ProcessMessagesOnce(random_node);
            peerman->SendMessages(random_node);
        }

        std::vector<CNodeStats> stats;
        connman.GetNodeStats(stats);

        if (sent_sendcmpct && random_node.fSuccessfullyConnected && !random_node.fDisconnect) {
            // If the fuzzer sent SENDCMPCT with proper version, check the node's state matches what it sent.
            const CNodeStats& random_node_stats = stats[random_node.GetId()];
            if (valid_sendcmpct) assert(random_node_stats.m_bip152_highbandwidth_from == requested_hb);
        }
    }

    setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
    setup->m_node.validation_signals->UnregisterAllValidationInterfaces();
    connman.StopNodes();

    const uint64_t end_sequence{WITH_LOCK(mempool.cs, return mempool.GetSequence())};

    if (initial_sequence != end_sequence) {
        MakeRandDeterministicDANGEROUS(uint256::ZERO);
        ResetChainmanAndMempool(*g_setup);
    }
}
