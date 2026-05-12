// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <blockfilter.h>
#include <chain.h>
#include <index/blockfilterindex.h>
#include <interfaces/chain.h>
#include <kernel/types.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>
#include <validation.h>
#include <validationinterface.h>

#include <boost/test/unit_test.hpp>

#include <atomic>
#include <future>
#include <memory>

namespace {

// A CValidationInterface that, when armed, stalls the validation-signals
// scheduler thread inside BlockConnected until Release() is called. Register
// this before the BlockFilterIndex so our callback fires first — at that point
// the filter index's CustomAppend has not yet run and the filter is not yet
// written.
class PreFilterBlocker : public CValidationInterface
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

} // namespace

BOOST_FIXTURE_TEST_SUITE(cfilter_race_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(cfilter_available_during_append_window)
{
    // Register the blocker BEFORE the filter index so our BlockConnected fires
    // first on the scheduler thread.
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

    // Mine one block. ProcessNewBlock advances the chain synchronously and
    // enqueues BlockConnected onto the validation-signals scheduler thread.
    const CScript coinbase_script = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    const CBlock new_block = CreateAndProcessBlock({}, coinbase_script);

    // Block until the scheduler thread has reached our gate. At this point:
    //  - the new block is in m_blockman (chain has advanced),
    //  - BLOCK_HAVE_DATA is set on the new tip,
    //  - the filter index's BlockConnected has NOT yet run (CustomAppend not called).
    // This is the exact window a peer's getcfilters could land in.
    blocker->WaitForEntry();

    const CBlockIndex* new_tip;
    {
        LOCK(cs_main);
        new_tip = m_node.chainman->m_blockman.LookupBlockIndex(new_block.GetHash());
    }
    BOOST_REQUIRE(new_tip);
    BOOST_REQUIRE_EQUAL(new_tip->nHeight, old_tip->nHeight + 1);
    BOOST_REQUIRE(new_tip->nStatus & BLOCK_HAVE_DATA);

    // EXPECTED TO FAIL TODAY:
    // ProcessGetCFilters ultimately calls LookupFilter / LookupFilterRange,
    // which only read from the on-disk filter file. During the append window
    // the filter is not yet written, so the lookup returns false and a peer
    // would get no reply. After the planned fix (compute on the fly for a
    // known block with data when no on-disk filter exists), this should pass.
    BlockFilter filter;
    BOOST_CHECK_MESSAGE(
        filter_index.LookupFilter(new_tip, filter),
        "filter for connected tip should be retrievable in the window between "
        "block connection and filter write");

    // Release the scheduler thread and let the filter index write the filter.
    blocker->Release();
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    // After CustomAppend has run, the filter is on disk regardless of fix state.
    BlockFilter from_disk;
    BOOST_CHECK(filter_index.LookupFilter(new_tip, from_disk));

    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    filter_index.Interrupt();
    filter_index.Stop();
}

BOOST_AUTO_TEST_CASE(cfilter_range_available_during_append_window)
{
    // Same setup as the single-lookup case, but exercises LookupFilterRange,
    // which is the path actually used by ProcessGetCFilters.
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

    // Range covers indexed history plus the not-yet-written new tip. With the
    // on-the-fly fill, this must return one filter per block in the range.
    std::vector<BlockFilter> filters;
    const int range_start = new_tip->nHeight - 3;
    BOOST_CHECK_MESSAGE(
        filter_index.LookupFilterRange(range_start, new_tip, filters),
        "LookupFilterRange should fill the race-window tip from raw block data");
    BOOST_CHECK_EQUAL(filters.size(), 4U);

    // The last filter must match what we get by reading the new tip directly
    // (computed-and-persisted by the range call above).
    if (filters.size() == 4U) {
        BlockFilter direct;
        BOOST_CHECK(filter_index.LookupFilter(new_tip, direct));
        BOOST_CHECK(filters.back().GetHash() == direct.GetHash());
    }

    blocker->Release();
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    filter_index.Interrupt();
    filter_index.Stop();
}

BOOST_AUTO_TEST_CASE(cfilter_header_available_during_append_window)
{
    // Exercises LookupFilterHeader during the race window. This is the path
    // ProcessGetCFHeaders uses to fetch prev_filter_header at start-1 of a
    // requested range; for a getcfheaders [tip, tip] request, prev is at
    // tip-1, which can itself be inside the race window.
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

    // Filter header for the racing tip must be reachable; on-the-fly compute
    // fills the chain and persists the entry so the header is available.
    uint256 header;
    BOOST_CHECK_MESSAGE(
        filter_index.LookupFilterHeader(new_tip, header),
        "filter header for connected tip should be retrievable during the append window");
    BOOST_CHECK(header != uint256{});

    blocker->Release();
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    // After CustomAppend has run, the header is on disk and consistent.
    uint256 from_disk;
    BOOST_CHECK(filter_index.LookupFilterHeader(new_tip, from_disk));
    BOOST_CHECK(from_disk == header);

    m_node.validation_signals->UnregisterSharedValidationInterface(blocker);
    filter_index.Interrupt();
    filter_index.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
