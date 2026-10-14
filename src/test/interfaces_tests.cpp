// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/validation.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <ipc/exception.h>
#include <node/types.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <script/solver.h>
#include <validation.h>
#include <validationinterface.h>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

using interfaces::FoundBlock;

BOOST_AUTO_TEST_SUITE(interfaces_tests)

BOOST_FIXTURE_TEST_CASE(findBlock, TestChain100Setup)
{
    LOCK(Assert(m_node.chainman)->GetMutex());
    auto& chain = m_node.chain;
    const CChain& active = Assert(m_node.chainman)->ActiveChain();

    uint256 hash;
    BOOST_CHECK(chain->findBlock(active[10]->GetBlockHash(), FoundBlock().hash(hash)));
    BOOST_CHECK_EQUAL(hash, active[10]->GetBlockHash());

    int height = -1;
    BOOST_CHECK(chain->findBlock(active[20]->GetBlockHash(), FoundBlock().height(height)));
    BOOST_CHECK_EQUAL(height, active[20]->nHeight);

    CBlock data;
    BOOST_CHECK(chain->findBlock(active[30]->GetBlockHash(), FoundBlock().data(data)));
    BOOST_CHECK_EQUAL(data.GetHash(), active[30]->GetBlockHash());

    int64_t time = -1;
    BOOST_CHECK(chain->findBlock(active[40]->GetBlockHash(), FoundBlock().time(time)));
    BOOST_CHECK_EQUAL(time, active[40]->GetBlockTime());

    int64_t max_time = -1;
    BOOST_CHECK(chain->findBlock(active[50]->GetBlockHash(), FoundBlock().maxTime(max_time)));
    BOOST_CHECK_EQUAL(max_time, active[50]->GetBlockTimeMax());

    int64_t mtp_time = -1;
    BOOST_CHECK(chain->findBlock(active[60]->GetBlockHash(), FoundBlock().mtpTime(mtp_time)));
    BOOST_CHECK_EQUAL(mtp_time, active[60]->GetMedianTimePast());

    bool cur_active{false}, next_active{false};
    uint256 next_hash;
    BOOST_CHECK_EQUAL(active.Height(), 100);
    BOOST_CHECK(chain->findBlock(active[99]->GetBlockHash(), FoundBlock().inActiveChain(cur_active).nextBlock(FoundBlock().inActiveChain(next_active).hash(next_hash))));
    BOOST_CHECK(cur_active);
    BOOST_CHECK(next_active);
    BOOST_CHECK_EQUAL(next_hash, active[100]->GetBlockHash());
    cur_active = next_active = false;
    BOOST_CHECK(chain->findBlock(active[100]->GetBlockHash(), FoundBlock().inActiveChain(cur_active).nextBlock(FoundBlock().inActiveChain(next_active))));
    BOOST_CHECK(cur_active);
    BOOST_CHECK(!next_active);

    BOOST_CHECK(!chain->findBlock({}, FoundBlock()));
}

BOOST_FIXTURE_TEST_CASE(findFirstBlockWithTimeAndHeight, TestChain100Setup)
{
    LOCK(Assert(m_node.chainman)->GetMutex());
    auto& chain = m_node.chain;
    const CChain& active = Assert(m_node.chainman)->ActiveChain();
    uint256 hash;
    int height;
    BOOST_CHECK(chain->findFirstBlockWithTimeAndHeight(/* min_time= */ 0, /* min_height= */ 5, FoundBlock().hash(hash).height(height)));
    BOOST_CHECK_EQUAL(hash, active[5]->GetBlockHash());
    BOOST_CHECK_EQUAL(height, 5);
    BOOST_CHECK(!chain->findFirstBlockWithTimeAndHeight(/* min_time= */ active.Tip()->GetBlockTimeMax() + 1, /* min_height= */ 0));
}

BOOST_FIXTURE_TEST_CASE(findAncestorByHeight, TestChain100Setup)
{
    LOCK(Assert(m_node.chainman)->GetMutex());
    auto& chain = m_node.chain;
    const CChain& active = Assert(m_node.chainman)->ActiveChain();
    uint256 hash;
    BOOST_CHECK(chain->findAncestorByHeight(active[20]->GetBlockHash(), 10, FoundBlock().hash(hash)));
    BOOST_CHECK_EQUAL(hash, active[10]->GetBlockHash());
    BOOST_CHECK(!chain->findAncestorByHeight(active[10]->GetBlockHash(), 20));
}

BOOST_FIXTURE_TEST_CASE(findAncestorByHash, TestChain100Setup)
{
    LOCK(Assert(m_node.chainman)->GetMutex());
    auto& chain = m_node.chain;
    const CChain& active = Assert(m_node.chainman)->ActiveChain();
    int height = -1;
    BOOST_CHECK(chain->findAncestorByHash(active[20]->GetBlockHash(), active[10]->GetBlockHash(), FoundBlock().height(height)));
    BOOST_CHECK_EQUAL(height, 10);
    BOOST_CHECK(!chain->findAncestorByHash(active[10]->GetBlockHash(), active[20]->GetBlockHash()));
    // Unknown block or ancestor hashes should return false.
    BOOST_CHECK(!chain->findAncestorByHash(uint256{}, active[0]->GetBlockHash()));
    BOOST_CHECK(!chain->findAncestorByHash(active[10]->GetBlockHash(), uint256{}));
}

BOOST_FIXTURE_TEST_CASE(findCommonAncestor, TestChain100Setup)
{
    auto& chain = m_node.chain;
    const CChain& active{*WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return &Assert(m_node.chainman)->ActiveChain())};
    auto* orig_tip = active.Tip();
    for (int i = 0; i < 10; ++i) {
        BlockValidationState state;
        m_node.chainman->ActiveChainstate().InvalidateBlock(state, active.Tip());
    }
    BOOST_CHECK_EQUAL(active.Height(), orig_tip->nHeight - 10);
    coinbaseKey.MakeNewKey(true);
    for (int i = 0; i < 20; ++i) {
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    }
    BOOST_CHECK_EQUAL(active.Height(), orig_tip->nHeight + 10);
    uint256 fork_hash;
    int fork_height;
    int orig_height;
    BOOST_CHECK(chain->findCommonAncestor(orig_tip->GetBlockHash(), active.Tip()->GetBlockHash(), FoundBlock().height(fork_height).hash(fork_hash), FoundBlock().height(orig_height)));
    BOOST_CHECK_EQUAL(orig_height, orig_tip->nHeight);
    BOOST_CHECK_EQUAL(fork_height, orig_tip->nHeight - 10);
    BOOST_CHECK_EQUAL(fork_hash, active[fork_height]->GetBlockHash());

    uint256 active_hash, orig_hash;
    BOOST_CHECK(!chain->findCommonAncestor(active.Tip()->GetBlockHash(), {}, {}, FoundBlock().hash(active_hash), {}));
    BOOST_CHECK(!chain->findCommonAncestor({}, orig_tip->GetBlockHash(), {}, {}, FoundBlock().hash(orig_hash)));
    BOOST_CHECK_EQUAL(active_hash, active.Tip()->GetBlockHash());
    BOOST_CHECK_EQUAL(orig_hash, orig_tip->GetBlockHash());
}

BOOST_FIXTURE_TEST_CASE(hasBlocks, TestChain100Setup)
{
    LOCK(::cs_main);
    auto& chain = m_node.chain;
    const CChain& active = Assert(m_node.chainman)->ActiveChain();

    // Test ranges
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, 90));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, {}));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 0, 90));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 0, {}));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), -1000, 1000));
    active[5]->nStatus &= ~BLOCK_HAVE_DATA;
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, 90));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), -1000, 1000));
    active[95]->nStatus &= ~BLOCK_HAVE_DATA;
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 10, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), -1000, 1000));
    active[50]->nStatus &= ~BLOCK_HAVE_DATA;
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 10, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 10, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), -1000, 1000));

    // Test edge cases
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 6, 49));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 5, 49));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 6, 50));
}

BOOST_FIXTURE_TEST_CASE(invalidArguments, TestChain100Setup)
{
    // Chain methods can be called over IPC by clients that don't enforce C++
    // preconditions, so they should return errors instead of crashing when
    // passed invalid arguments.
    auto& chain = m_node.chain;
    const CBlockIndex* tip{WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Tip())};
    BOOST_CHECK_EQUAL(chain->getBlockHash(tip->nHeight), tip->GetBlockHash());
    BOOST_CHECK_THROW(chain->getBlockHash(tip->nHeight + 1), std::runtime_error);
    BOOST_CHECK_THROW(chain->getBlockHash(-1), std::runtime_error);

    BOOST_CHECK(!chain->hasBlocks(tip->GetBlockHash(), 0, -1));

    BOOST_CHECK(!chain->checkChainLimits(nullptr));

    std::string err;
    BOOST_CHECK(!chain->broadcastTransaction(nullptr, /*max_tx_fee=*/0, CFeeRate{0}, node::TxBroadcast::MEMPOOL_NO_BROADCAST, err));
    BOOST_CHECK_EQUAL(err, "Transaction is null");
    const auto tx{MakeTransactionRef(CMutableTransaction{})};
    BOOST_CHECK(!chain->broadcastTransaction(tx, /*max_tx_fee=*/0, CFeeRate{0}, static_cast<node::TxBroadcast>(255), err));
    BOOST_CHECK_EQUAL(err, "Invalid broadcast method 255");
}

BOOST_FIXTURE_TEST_CASE(throwingNotifications, TestChain100Setup)
{
    // A notification handler that throws ipc::Exception, like an IPC client
    // returning an error or disconnecting, should not crash the node or stop
    // later notifications. Not covered: other exceptions thrown by in-process
    // handlers are intentionally not caught, but checking that would require
    // calling the notification proxy without the scheduler thread, since an
    // exception escaping that thread terminates the test.
    struct ThrowingNotifications : interfaces::Chain::Notifications {
        std::map<std::string, int> m_calls;
        void Throw(const std::string& name)
        {
            ++m_calls[name];
            throw ipc::Exception(name + " error");
        }
        void transactionAddedToMempool(const CTransactionRef&) override { Throw("transactionAddedToMempool"); }
        void transactionRemovedFromMempool(const CTransactionRef&, MemPoolRemovalReason) override { Throw("transactionRemovedFromMempool"); }
        void blockConnected(const kernel::ChainstateRole&, const interfaces::BlockInfo&) override { Throw("blockConnected"); }
        void blockDisconnected(const interfaces::BlockInfo&) override { Throw("blockDisconnected"); }
        void updatedBlockTip() override { Throw("updatedBlockTip"); }
        void chainStateFlushed(const kernel::ChainstateRole&, const CBlockLocator&) override { Throw("chainStateFlushed"); }
    };
    auto notifications{std::make_shared<ThrowingNotifications>()};
    auto handler{m_node.chain->handleNotifications(notifications)};
    auto& signals{*Assert(m_node.validation_signals)};
    auto& chainman{*Assert(m_node.chainman)};
    const CScript script{CScript() << OP_TRUE};
    const CScript& dest{m_coinbase_txns[0]->vout[0].scriptPubKey};

    // Add a transaction to the mempool, then connect a block with a
    // conflicting transaction, which removes it from the mempool.
    CreateValidMempoolTransaction(m_coinbase_txns[0], /*input_vout=*/0, /*input_height=*/1, coinbaseKey, dest, /*output_amount=*/1 * COIN, /*submit=*/true);
    signals.SyncWithValidationInterfaceQueue();
    const auto conflict{CreateValidMempoolTransaction(m_coinbase_txns[0], /*input_vout=*/0, /*input_height=*/1, coinbaseKey, dest, /*output_amount=*/2 * COIN, /*submit=*/false)};
    CreateAndProcessBlock({conflict}, script);

    // Disconnect that block, then flush the chainstate.
    BlockValidationState state;
    CBlockIndex* tip{WITH_LOCK(chainman.GetMutex(), return chainman.ActiveChain().Tip())};
    BOOST_CHECK(chainman.ActiveChainstate().InvalidateBlock(state, tip));
    chainman.ActiveChainstate().ForceFlushStateToDisk();
    signals.SyncWithValidationInterfaceQueue();

    for (const char* name : {"transactionAddedToMempool", "transactionRemovedFromMempool", "blockConnected",
                             "blockDisconnected", "updatedBlockTip", "chainStateFlushed"}) {
        BOOST_CHECK_MESSAGE(notifications->m_calls[name] > 0, name << " was not called");
    }

    // Notifications are still sent after the earlier ones failed.
    const auto calls{notifications->m_calls};
    CreateAndProcessBlock({}, script);
    BOOST_CHECK_EQUAL(notifications->m_calls["blockConnected"], calls.at("blockConnected") + 1);
    BOOST_CHECK_EQUAL(notifications->m_calls["updatedBlockTip"], calls.at("updatedBlockTip") + 1);
    handler->disconnect();
}

BOOST_AUTO_TEST_SUITE_END()
