// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <blockfilter.h>
#include <chain.h>
#include <consensus/validation.h>
#include <index/blockfilterindex.h>
#include <interfaces/chain.h>
#include <kernel/types.h>
#include <key.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>
#include <validation.h>
#include <validationinterface.h>

#include <boost/test/unit_test.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>

namespace {

// A CValidationInterface that, when armed, stalls the validation-signals
// scheduler thread inside BlockConnected until Release() is called. Register
// this before the BlockFilterIndex so our callback fires first — at that point
// the filter index's CustomAppend has not yet run and the filter is not yet
// written.
class PreFilterBlocker final : public CValidationInterface
{
public:
    void Arm() { m_armed = true; }
    void WaitForEntry() { m_entered.get_future().wait(); }
    void Release()
    {
        if (!m_released.exchange(true)) m_release.set_value();
    }

protected:
    void BlockConnected(const kernel::ChainstateRole& role,
                        const std::shared_ptr<const CBlock>&,
                        const CBlockIndex*) override
    {
        if (!role.validated) return;
        if (m_armed.exchange(false)) {
            m_entered.set_value();
            m_release_fut.wait();
        }
    }

private:
    std::atomic_bool m_armed{false};
    std::atomic_bool m_released{false};
    std::promise<void> m_entered;
    std::promise<void> m_release;
    std::shared_future<void> m_release_fut{m_release.get_future()};
};

// Spin until `pred()` becomes true or `budget` elapses. Returns the final
// value of pred(). Used to give a worker thread a chance to enter a wait
// state we cannot observe directly.
template <typename Pred>
bool WaitUpTo(std::chrono::milliseconds budget, Pred pred)
{
    const auto deadline = std::chrono::steady_clock::now() + budget;
    while (std::chrono::steady_clock::now() < deadline) {
        if (pred()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return pred();
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(cfilter_race_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(cfilter_available_during_append_window)
{
    // Register the blocker BEFORE the filter index so our BlockConnected fires
    // first on the scheduler thread, parking it before the filter index gets to
    // run its own CustomAppend.
    auto blocker = std::make_shared<PreFilterBlocker>();
    m_node.validation_signals->RegisterSharedValidationInterface(blocker);

    BlockFilterIndex filter_index(interfaces::MakeChain(m_node),
                                  BlockFilterType::BASIC,
                                  /*n_cache_size=*/1_MiB,
                                  /*f_memory=*/true);
    BOOST_REQUIRE(filter_index.Init());
    filter_index.Sync(); // backfills filters for the pre-mined 100 blocks

    // Sanity: filter for the existing tip is present.
    const CBlockIndex* old_tip;
    {
        LOCK(cs_main);
        old_tip = m_node.chainman->ActiveChain().Tip();
    }
    BlockFilter dummy;
    BOOST_REQUIRE(filter_index.LookupFilter(old_tip, dummy));

    // Arm the gate: the next BlockConnected callback will park the scheduler
    // thread inside PreFilterBlocker::BlockConnected before the filter index
    // gets a chance to run CustomAppend.
    blocker->Arm();

    const CScript coinbase_script = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    const CBlock new_block = CreateAndProcessBlock({}, coinbase_script);

    // Block until the scheduler thread has reached our gate. At this point the
    // new block is on the chain with BLOCK_HAVE_DATA, but the filter index's
    // BlockConnected has NOT yet run (CustomAppend not called). This is the
    // exact window a peer's getcfilters could land in.
    blocker->WaitForEntry();

    const CBlockIndex* new_tip;
    {
        LOCK(cs_main);
        new_tip = m_node.chainman->m_blockman.LookupBlockIndex(new_block.GetHash());
        BOOST_REQUIRE(new_tip);
        BOOST_REQUIRE_EQUAL(new_tip->nHeight, old_tip->nHeight + 1);
        BOOST_REQUIRE(new_tip->nStatus & BLOCK_HAVE_DATA);
    }

    // Issue the lookup from a worker thread. The scheduler thread is parked
    // inside our blocker, so LookupFilter cannot succeed by reading from disk.
    // With the wait-and-retry fix it must:
    //   1) miss the initial DB lookup,
    //   2) see that new_tip is within the wait window ahead of the index's
    //      last processed block,
    //   3) call SyncWithValidationInterfaceQueue() and block,
    //   4) unblock only after we release the scheduler — by which time
    //      CustomAppend has run and the filter is on disk,
    //   5) retry and succeed.
    std::atomic_bool worker_finished{false};
    bool worker_result{false};
    BlockFilter filter;
    std::thread worker([&] {
        worker_result = filter_index.LookupFilter(new_tip, filter);
        worker_finished = true;
    });

    // The worker must NOT complete while the scheduler is parked. If it does,
    // either the wait was skipped (regression) or the filter showed up on disk
    // some other way (also a regression — CustomAppend has not run yet).
    BOOST_CHECK_MESSAGE(
        !WaitUpTo(std::chrono::milliseconds(250), [&] { return worker_finished.load(); }),
        "LookupFilter should block on the validation queue while the racing "
        "BlockConnected has not yet run");

    blocker->Release();
    worker.join();
    BOOST_CHECK_MESSAGE(worker_result,
                        "LookupFilter should succeed once the queued "
                        "BlockConnected has been processed");
    if (worker_result) {
        BOOST_CHECK(filter.GetBlockHash() == new_block.GetHash());
    }

    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    filter_index.Interrupt();
    filter_index.Stop();
}

BOOST_AUTO_TEST_CASE(cfilter_range_available_during_append_window)
{
    auto blocker = std::make_shared<PreFilterBlocker>();
    m_node.validation_signals->RegisterSharedValidationInterface(blocker);

    BlockFilterIndex filter_index(interfaces::MakeChain(m_node),
                                  BlockFilterType::BASIC,
                                  /*n_cache_size=*/1_MiB,
                                  /*f_memory=*/true);
    BOOST_REQUIRE(filter_index.Init());
    filter_index.Sync();

    const CBlockIndex* old_tip;
    {
        LOCK(cs_main);
        old_tip = m_node.chainman->ActiveChain().Tip();
    }

    // Baseline: range over fully-indexed history works.
    std::vector<BlockFilter> baseline;
    BOOST_REQUIRE(filter_index.LookupFilterRange(old_tip->nHeight - 2, old_tip, baseline));
    BOOST_REQUIRE_EQUAL(baseline.size(), 3U);

    blocker->Arm();
    const CScript coinbase_script = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    const CBlock new_block = CreateAndProcessBlock({}, coinbase_script);
    blocker->WaitForEntry();

    const CBlockIndex* new_tip;
    {
        LOCK(cs_main);
        new_tip = m_node.chainman->m_blockman.LookupBlockIndex(new_block.GetHash());
    }
    BOOST_REQUIRE(new_tip);
    BOOST_REQUIRE_EQUAL(new_tip->nHeight, old_tip->nHeight + 1);

    std::atomic_bool worker_finished{false};
    bool worker_result{false};
    std::vector<BlockFilter> filters;
    const int range_start = new_tip->nHeight - 3;
    std::thread worker([&] {
        worker_result = filter_index.LookupFilterRange(range_start, new_tip, filters);
        worker_finished = true;
    });

    BOOST_CHECK_MESSAGE(
        !WaitUpTo(std::chrono::milliseconds(250), [&] { return worker_finished.load(); }),
        "LookupFilterRange should block on the validation queue while the racing "
        "BlockConnected has not yet run");

    blocker->Release();
    worker.join();
    BOOST_CHECK_MESSAGE(worker_result,
                        "LookupFilterRange should return all filters once the "
                        "queued BlockConnected has been processed");
    BOOST_CHECK_EQUAL(filters.size(), 4U);
    if (filters.size() == 4U) {
        BlockFilter direct;
        BOOST_CHECK(filter_index.LookupFilter(new_tip, direct));
        BOOST_CHECK(filters.back().GetHash() == direct.GetHash());
    }

    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    filter_index.Interrupt();
    filter_index.Stop();
}

BOOST_AUTO_TEST_CASE(cfilter_header_available_during_append_window)
{
    auto blocker = std::make_shared<PreFilterBlocker>();
    m_node.validation_signals->RegisterSharedValidationInterface(blocker);

    BlockFilterIndex filter_index(interfaces::MakeChain(m_node),
                                  BlockFilterType::BASIC,
                                  /*n_cache_size=*/1_MiB,
                                  /*f_memory=*/true);
    BOOST_REQUIRE(filter_index.Init());
    filter_index.Sync();

    blocker->Arm();
    const CScript coinbase_script = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    const CBlock new_block = CreateAndProcessBlock({}, coinbase_script);
    blocker->WaitForEntry();

    const CBlockIndex* new_tip;
    {
        LOCK(cs_main);
        new_tip = m_node.chainman->m_blockman.LookupBlockIndex(new_block.GetHash());
    }
    BOOST_REQUIRE(new_tip);

    std::atomic_bool worker_finished{false};
    bool worker_result{false};
    uint256 header;
    std::thread worker([&] {
        worker_result = filter_index.LookupFilterHeader(new_tip, header);
        worker_finished = true;
    });

    BOOST_CHECK_MESSAGE(
        !WaitUpTo(std::chrono::milliseconds(250), [&] { return worker_finished.load(); }),
        "LookupFilterHeader should block on the validation queue while the racing "
        "BlockConnected has not yet run");

    blocker->Release();
    worker.join();
    BOOST_CHECK_MESSAGE(worker_result,
                        "LookupFilterHeader should succeed once the queued "
                        "BlockConnected has been processed");
    BOOST_CHECK(header != uint256{});

    // After the wait, the header is consistent with a direct re-read.
    uint256 from_disk;
    BOOST_CHECK(filter_index.LookupFilterHeader(new_tip, from_disk));
    BOOST_CHECK(from_disk == header);

    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    filter_index.Interrupt();
    filter_index.Stop();
}

// Same-height reorg: the index's last processed block A and the requested
// block B sit at the same height on different branches. When BlockConnected(B)
// is queued but not yet processed, LookupFilter(B) must wait for it. This
// exercises the `ahead == 0` branch of the helper.
BOOST_AUTO_TEST_CASE(cfilter_available_during_same_height_reorg)
{
    auto blocker = std::make_shared<PreFilterBlocker>();
    m_node.validation_signals->RegisterSharedValidationInterface(blocker);

    BlockFilterIndex filter_index(interfaces::MakeChain(m_node),
                                  BlockFilterType::BASIC,
                                  /*n_cache_size=*/1_MiB,
                                  /*f_memory=*/true);
    BOOST_REQUIRE(filter_index.Init());
    filter_index.Sync();

    // Mine A at height 101 and let the filter index fully process it, so
    // m_best_block_index == A.
    const CScript script_A = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    const CBlock block_A = CreateAndProcessBlock({}, script_A);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    CBlockIndex* a_index;
    {
        LOCK(cs_main);
        a_index = m_node.chainman->m_blockman.LookupBlockIndex(block_A.GetHash());
    }
    BOOST_REQUIRE(a_index);

    // Invalidate A to roll the active chain back to height 100. BaseIndex
    // does not act on BlockDisconnected, so the index's m_best_block_index
    // stays pointing at A — the divergence is only resolved by the next
    // BlockConnected, which is exactly the race we want to exercise.
    {
        BlockValidationState state;
        BOOST_REQUIRE(m_node.chainman->ActiveChainstate().InvalidateBlock(state, a_index));
    }
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    blocker->Arm();

    // Mine a sibling B at height 101 with a different coinbase so the hash differs.
    const CKey other_key{GenerateRandomKey()};
    const CScript script_B = GetScriptForDestination(PKHash(other_key.GetPubKey()));
    const CBlock block_B = CreateAndProcessBlock({}, script_B);
    BOOST_REQUIRE(block_B.GetHash() != block_A.GetHash());

    blocker->WaitForEntry();

    const CBlockIndex* b_index;
    {
        LOCK(cs_main);
        b_index = m_node.chainman->m_blockman.LookupBlockIndex(block_B.GetHash());
    }
    BOOST_REQUIRE(b_index);
    BOOST_REQUIRE_EQUAL(b_index->nHeight, a_index->nHeight);

    // Scheduler is parked inside BlockConnected(B). Index state:
    //   m_best_block_index = A  (stale branch, height 101)
    //   target             = B  (active branch, height 101)
    //   ahead              = 0  -> must trigger the wait.
    std::atomic_bool worker_finished{false};
    bool worker_result{false};
    BlockFilter filter;
    std::thread worker([&] {
        worker_result = filter_index.LookupFilter(b_index, filter);
        worker_finished = true;
    });
    BOOST_CHECK_MESSAGE(
        !WaitUpTo(std::chrono::milliseconds(250), [&] { return worker_finished.load(); }),
        "LookupFilter must block on the validation queue during a same-height reorg");

    blocker->Release();
    worker.join();
    BOOST_CHECK_MESSAGE(worker_result,
                        "LookupFilter must succeed once the queued BlockConnected for the sibling has been processed");
    if (worker_result) {
        BOOST_CHECK(filter.GetBlockHash() == block_B.GetHash());
    }

    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    filter_index.Interrupt();
    filter_index.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
