// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <chainparams.h>
#include <consensus/validation.h>
#include <pow.h>
#include <primitives/block.h>
#include <scheduler.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <validation.h>
#include <validationinterface.h>

#include <atomic>
#include <memory>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(validationinterface_tests, ChainTestingSetup)

struct TestSubscriberNoop final : public CValidationInterface {
    void BlockChecked(const std::shared_ptr<const CBlock>&, const BlockValidationState&) override {}
};

struct StaleTipSubscriber final : CValidationInterface {
    std::vector<const CBlockIndex*> m_accepted_stale_tips;

    void AcceptedStaleTip(const CBlockIndex* pindex) override
    {
        m_accepted_stale_tips.push_back(pindex);
    }
};

/** Mine `count` connected headers on `prev_hash`, starting at `time`. */
static std::vector<CBlockHeader> MineHeaders(uint256 prev_hash, uint32_t time, uint32_t bits, size_t count)
{
    std::vector<CBlockHeader> headers;
    for (size_t i{0}; i < count; ++i) {
        CBlockHeader header;
        header.nVersion = 4;
        header.hashPrevBlock = prev_hash;
        header.nTime = time + i;
        header.nBits = bits;
        while (!CheckProofOfWork(header.GetHash(), header.nBits, Params().GetConsensus())) ++header.nNonce;
        prev_hash = header.GetHash();
        headers.push_back(header);
    }
    return headers;
}

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

BOOST_FIXTURE_TEST_CASE(processnewblockheaders_coalesces_accepted_stale_tip, TestChain100Setup)
{
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    const auto sub{std::make_shared<StaleTipSubscriber>()};
    m_node.validation_signals->RegisterSharedValidationInterface(sub);
    auto& chainman{*Assert(m_node.chainman)};

    const CBlockIndex* fork{WITH_LOCK(::cs_main, return Assert(chainman.ActiveChain().Tip())->GetAncestor(97))};
    const uint32_t time{static_cast<uint32_t>(fork->GetBlockTime() + 1)};
    const auto headers{MineHeaders(fork->GetBlockHash(), time, fork->nBits, 3)};

    BlockValidationState state;
    const CBlockIndex* tip{nullptr};
    BOOST_CHECK(chainman.ProcessNewBlockHeaders(headers, /*min_pow_checked=*/true, state, &tip));
    BOOST_REQUIRE(tip);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    BOOST_REQUIRE_EQUAL(sub->m_accepted_stale_tips.size(), 1U);
    BOOST_CHECK_EQUAL(sub->m_accepted_stale_tips.front(), tip);

    BlockValidationState duplicate_state;
    const CBlockIndex* duplicate_tip{nullptr};
    BOOST_CHECK(chainman.ProcessNewBlockHeaders(headers, /*min_pow_checked=*/true, duplicate_state, &duplicate_tip));
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(sub->m_accepted_stale_tips.size(), 1U);
    BOOST_CHECK_EQUAL(duplicate_tip, tip);

    // A header with more work than the active tip is not a stale tip, even
    // though header acceptance does not activate it.
    const auto better_work_headers{MineHeaders(tip->GetBlockHash(), time + 3, fork->nBits, 1)};
    BOOST_CHECK(chainman.ProcessNewBlockHeaders(better_work_headers, /*min_pow_checked=*/true, state));
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(sub->m_accepted_stale_tips.size(), 1U);
}

BOOST_FIXTURE_TEST_CASE(processnewblockheaders_notifies_accepted_prefix_before_failure, TestChain100Setup)
{
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    const auto sub{std::make_shared<StaleTipSubscriber>()};
    m_node.validation_signals->RegisterSharedValidationInterface(sub);
    auto& chainman{*Assert(m_node.chainman)};

    const CBlockIndex* fork{WITH_LOCK(::cs_main, return Assert(chainman.ActiveChain().Tip())->GetAncestor(97))};
    const uint32_t time{static_cast<uint32_t>(fork->GetBlockTime() + 1)};
    auto headers{MineHeaders(fork->GetBlockHash(), time, fork->nBits, 2)};

    CBlockHeader invalid_header;
    invalid_header.nVersion = 4;
    invalid_header.hashPrevBlock = headers.back().GetHash();
    invalid_header.hashMerkleRoot = uint256{uint8_t{12}};
    invalid_header.nTime = time + 2;
    invalid_header.nBits = 0; // Fails the proof-of-work check.
    headers.push_back(invalid_header);

    BlockValidationState state;
    const CBlockIndex* accepted_prefix_tip{nullptr};
    BOOST_CHECK(!chainman.ProcessNewBlockHeaders(headers, /*min_pow_checked=*/true, state, &accepted_prefix_tip));
    BOOST_CHECK(state.IsInvalid());
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    BOOST_REQUIRE_EQUAL(sub->m_accepted_stale_tips.size(), 1U);
    BOOST_CHECK_EQUAL(sub->m_accepted_stale_tips.front()->GetBlockHash(), headers[1].GetHash());
    BOOST_CHECK_EQUAL(Assert(accepted_prefix_tip)->GetBlockHash(), headers[1].GetHash());

    const std::vector<CBlockHeader> valid_prefix{headers.begin(), headers.end() - 1};
    BlockValidationState duplicate_state;
    BOOST_CHECK(chainman.ProcessNewBlockHeaders(valid_prefix, /*min_pow_checked=*/true, duplicate_state));
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(sub->m_accepted_stale_tips.size(), 1U);

    const std::vector<CBlockHeader> invalid_only{invalid_header};
    BlockValidationState invalid_only_state;
    BOOST_CHECK(!chainman.ProcessNewBlockHeaders(invalid_only, /*min_pow_checked=*/true, invalid_only_state));
    BOOST_CHECK(invalid_only_state.IsInvalid());
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(sub->m_accepted_stale_tips.size(), 1U);
}

BOOST_FIXTURE_TEST_CASE(stale_block_notifies_for_header_and_data, TestChain100Setup)
{
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    const auto sub{std::make_shared<StaleTipSubscriber>()};
    m_node.validation_signals->RegisterSharedValidationInterface(sub);
    auto& chainman{*Assert(m_node.chainman)};

    const auto process_block{[&](const CBlock& block) {
        BOOST_CHECK(chainman.ProcessNewBlock(std::make_shared<const CBlock>(block), /*force_processing=*/true, /*min_pow_checked=*/true, /*new_block=*/nullptr));
        m_node.validation_signals->SyncWithValidationInterfaceQueue();
    }};

    const CBlock active_block{CreateBlock({}, CScript() << OP_TRUE)};
    const CBlock block{CreateBlock({}, CScript() << OP_2)};
    const CBlock header_first{CreateBlock({}, CScript() << OP_3)};
    process_block(active_block);
    BOOST_CHECK_EQUAL(sub->m_accepted_stale_tips.size(), 0U);

    // A new stale block is notified for its header and again for its data.
    process_block(block);
    BOOST_REQUIRE_EQUAL(sub->m_accepted_stale_tips.size(), 2U);
    BOOST_CHECK_EQUAL(sub->m_accepted_stale_tips.front()->GetBlockHash(), block.GetHash());
    BOOST_CHECK_EQUAL(sub->m_accepted_stale_tips.back()->GetBlockHash(), block.GetHash());

    // If the header arrives first, the block is notified again when its data arrives.
    BlockValidationState header_state;
    BOOST_CHECK(chainman.ProcessNewBlockHeaders({{CBlockHeader{header_first}}}, /*min_pow_checked=*/true, header_state));
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_REQUIRE_EQUAL(sub->m_accepted_stale_tips.size(), 3U);
    BOOST_CHECK_EQUAL(sub->m_accepted_stale_tips.back()->GetBlockHash(), header_first.GetHash());
    process_block(header_first);
    BOOST_REQUIRE_EQUAL(sub->m_accepted_stale_tips.size(), 4U);
    BOOST_CHECK_EQUAL(sub->m_accepted_stale_tips.back()->GetBlockHash(), header_first.GetHash());
}

BOOST_AUTO_TEST_SUITE_END()
