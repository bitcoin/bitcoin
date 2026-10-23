// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <consensus/validation.h>
#include <interfaces/handler.h>
#include <net_processing.h>
#include <node/transaction.h>
#include <primitives/block.h>
#include <script/solver.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <util/check.h>
#include <validationinterface.h>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>

BOOST_FIXTURE_TEST_SUITE(validationinterface_tests, ChainTestingSetup)

struct TestSubscriberNoop final : public CValidationInterface {
    void BlockChecked(const std::shared_ptr<const CBlock>&, const BlockValidationState&) override {}
};

BOOST_AUTO_TEST_CASE(unregister_validation_interface_race)
{
    std::atomic<bool> generate{true};

    // Start thread to generate notifications
    std::thread gen{[&] {
        BlockValidationState state_dummy;
        while (generate) {
            m_node.validation_signals->BlockChecked(std::make_shared<const CBlock>(), state_dummy);
        }
    }};

    // Start thread to consume notifications
    std::thread sub{[&] {
        // keep going for about 1 sec, which is 250k iterations
        for (int i = 0; i < 250000; i++) {
            auto sub = std::make_shared<TestSubscriberNoop>();
            m_node.validation_signals->RegisterSharedValidationInterface(sub);
            m_node.validation_signals->UnregisterSharedValidationInterface(sub);
        }
        // tell the other thread we are done
        generate = false;
    }};

    gen.join();
    sub.join();
    BOOST_CHECK(!generate);
}

class TestInterface : public CValidationInterface
{
public:
    TestInterface(ValidationSignals& signals, std::function<void()> on_call = nullptr, std::function<void()> on_destroy = nullptr)
        : m_on_call(std::move(on_call)), m_on_destroy(std::move(on_destroy)), m_signals{signals}
    {
    }
    virtual ~TestInterface()
    {
        if (m_on_destroy) m_on_destroy();
    }
    void BlockChecked(const std::shared_ptr<const CBlock>& block, const BlockValidationState& state) override
    {
        if (m_on_call) m_on_call();
    }
    void Call()
    {
        BlockValidationState state;
        m_signals.BlockChecked(std::make_shared<const CBlock>(), state);
    }
    std::function<void()> m_on_call;
    std::function<void()> m_on_destroy;
    ValidationSignals& m_signals;
};

// Regression test to ensure UnregisterAllValidationInterfaces calls don't
// destroy a validation interface while it is being called. Bug:
// https://github.com/bitcoin/bitcoin/pull/18551
BOOST_AUTO_TEST_CASE(unregister_all_during_call)
{
    bool destroyed = false;
    auto shared{std::make_shared<TestInterface>(
        *m_node.validation_signals,
        [&] {
            // First call should decrements reference count 2 -> 1
            m_node.validation_signals->UnregisterAllValidationInterfaces();
            BOOST_CHECK(!destroyed);
            // Second call should not decrement reference count 1 -> 0
            m_node.validation_signals->UnregisterAllValidationInterfaces();
            BOOST_CHECK(!destroyed);
        },
        [&] { destroyed = true; })};
    m_node.validation_signals->RegisterSharedValidationInterface(shared);
    BOOST_CHECK(shared.use_count() == 2);
    shared->Call();
    BOOST_CHECK(shared.use_count() == 1);
    BOOST_CHECK(!destroyed);
    shared.reset();
    BOOST_CHECK(destroyed);
}

BOOST_FIXTURE_TEST_CASE(broadcast_transaction_notifications, TestChain100Setup)
{
    const auto tx{MakeTransactionRef(CreateValidMempoolTransaction(
        /*input_transaction=*/m_coinbase_txns[0], /*input_vout=*/0, /*input_height=*/1,
        /*input_signing_key=*/coinbaseKey, /*output_destination=*/GetScriptForRawPubKey(coinbaseKey.GetPubKey()),
        /*output_amount=*/49 * COIN, /*submit=*/false))};
    auto& pool{*m_node.mempool};
    auto& signals{*m_node.validation_signals};
    Mutex wallet_mutex;
    auto submit = [&](bool wait_callback) {
        auto result = std::async(std::launch::async, [&, wait_callback] {
            // Model a wallet caller holding its lock when it does not flush notifications.
            LOCK(wait_callback ? nullptr : &wallet_mutex);
            std::string err;
            return node::BroadcastTransaction(/*node=*/m_node, /*tx=*/tx, /*err_string=*/err,
                /*max_tx_fee=*/0, /*max_tx_fee_rate=*/CFeeRate{0},
                /*broadcast_method=*/node::TxBroadcast::NO_MEMPOOL_PRIVATE_BROADCAST,
                /*wait_callback=*/wait_callback, /*private_broadcast_timeout=*/5s);
        });
        const auto deadline{std::chrono::steady_clock::now() + 5s};
        while (m_node.peerman->GetPrivateBroadcastInfo().empty() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(1ms);
        }
        BOOST_REQUIRE(!m_node.peerman->GetPrivateBroadcastInfo().empty());
        return result;
    };

    auto first{submit(/*wait_callback=*/false)};
    // Clear the queue so its next entry confirms the second caller has subscribed.
    // Aborting the broadcast leaves the first caller's acceptance subscription active.
    m_node.peerman->AbortPrivateBroadcast(tx->GetHash().ToUint256());
    auto second{submit(/*wait_callback=*/true)};

    // A wallet notification blocks on the lock held by the first caller. Keep
    // the queue paused after that caller returns to also test wait_callback=true.
    std::promise<void> resume;
    auto paused{interfaces::MakeCleanupHandler([&] {
        resume.set_value();
        signals.SyncWithValidationInterfaceQueue();
    })};
    signals.CallFunctionInValidationInterfaceQueue([&, ready = resume.get_future().share()] {
        LOCK(wallet_mutex);
        ready.wait();
    });

    auto notify = [&](const CTransactionRef& accepted_tx) {
        LOCK2(cs_main, pool.cs);
        signals.TransactionAddedToMempool(NewMempoolTransactionInfo{
            /*tx=*/accepted_tx, /*fee=*/COIN, /*vsize=*/100, /*height=*/1,
            /*mempool_limit_bypassed=*/false, /*submitted_in_package=*/false,
            /*chainstate_is_current=*/true, /*has_no_mempool_parents=*/true}, pool.GetAndIncrementSequence());
    };
    // Acceptance of another txid must not complete the wait.
    notify(m_coinbase_txns[1]);
    BOOST_CHECK(first.wait_for(10ms) == std::future_status::timeout);

    // Insert locally to simulate an accepted echo. The private caller with
    // wait_callback=false must complete even while wallet notifications are blocked.
    std::string error;
    BOOST_REQUIRE(node::BroadcastTransaction(/*node=*/m_node, /*tx=*/tx, /*err_string=*/error,
        /*max_tx_fee=*/0, /*max_tx_fee_rate=*/CFeeRate{0},
        /*broadcast_method=*/node::TxBroadcast::MEMPOOL_NO_BROADCAST, /*wait_callback=*/false) == node::TransactionError::OK);
    WITH_LOCK(pool.cs, pool.removeRecursive(*tx, MemPoolRemovalReason::REPLACED));
    BOOST_CHECK(first.get() == node::TransactionError::OK);
    BOOST_CHECK(second.wait_for(10ms) == std::future_status::timeout);
    // The second callback is still registered; duplicate acceptance must be harmless.
    notify(tx);
    paused.reset();
    BOOST_CHECK(second.get() == node::TransactionError::OK);
}

BOOST_AUTO_TEST_SUITE_END()
