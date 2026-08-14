// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <consensus/validation.h>
#include <hash.h>
#include <node/blockdownloadman.h>
#include <node/context.h>
#include <pow.h>
#include <primitives/block.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <list>
#include <optional>
#include <vector>

using node::BlockDownloadConnectionInfo;
using node::BlockDownloadManager;
using node::BlockDownloadOptions;

namespace {

const TestingSetup* g_setup;

// Limit the total number of peers because we don't expect coverage to change much with lots more peers.
constexpr NodeId NUM_PEERS{8};

// All block index entries known to the fixture chainstate: the active chain
// (blocks with data), a headers-only extension of the tip, and a headers-only
// fork branching from below the tip.
std::vector<const CBlockIndex*> ALL_BLOCKS;

void MineHeadersOnlyChain(ChainstateManager& chainman, const CBlockIndex* fork_base, int length)
{
    std::vector<CBlockHeader> headers;
    uint256 prev_hash{fork_base->GetBlockHash()};
    uint32_t prev_time{static_cast<uint32_t>(fork_base->GetBlockTime())};
    for (int i = 0; i < length; ++i) {
        CBlockHeader header;
        header.nVersion = 0x20000000;
        header.hashPrevBlock = prev_hash;
        // Distinct merkle roots give each header chain distinct block hashes.
        header.hashMerkleRoot = (HashWriter{} << prev_hash << i).GetHash();
        header.nTime = ++prev_time;
        header.nBits = fork_base->nBits;
        header.nNonce = 0;
        while (!CheckProofOfWork(header.GetHash(), header.nBits, chainman.GetConsensus())) {
            ++header.nNonce;
        }
        prev_hash = header.GetHash();
        headers.push_back(header);
    }
    BlockValidationState state;
    Assert(chainman.ProcessNewBlockHeaders(headers, /*min_pow_checked=*/true, state));
    LOCK(cs_main);
    for (const auto& header : headers) {
        ALL_BLOCKS.push_back(Assert(chainman.m_blockman.LookupBlockIndex(header.GetHash())));
    }
}

void initialize_blockdownloadman()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    auto& chainman = *g_setup->m_node.chainman;
    SetMockTime(WITH_LOCK(cs_main, return chainman.ActiveChain().Tip()->Time()));

    // Mine 20 blocks (with data) on top of the genesis block.
    for (int i = 0; i < 20; ++i) {
        MineBlock(g_setup->m_node, {.coinbase_output_script = P2WSH_OP_TRUE});
    }
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();

    const CBlockIndex* tip = WITH_LOCK(cs_main, return chainman.ActiveChain().Tip());
    Assert(tip->nHeight == 20);
    {
        LOCK(cs_main);
        for (int height = 0; height <= tip->nHeight; ++height) {
            ALL_BLOCKS.push_back(chainman.ActiveChain()[height]);
        }
    }
    // A headers-only extension of the active tip and a headers-only fork from
    // below the tip: download candidates with more chain work than the tip.
    MineHeadersOnlyChain(chainman, tip, /*length=*/24);
    MineHeadersOnlyChain(chainman, tip->GetAncestor(14), /*length=*/12);

    // Freeze time so that runs are deterministic.
    SetMockTime(tip->Time());
}

FUZZ_TARGET(blockdownloadman, .init = initialize_blockdownloadman)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};

    auto& chainman = *g_setup->m_node.chainman;
    CTxMemPool* mempool = g_setup->m_node.mempool.get();

    BlockDownloadManager bdm{BlockDownloadOptions{chainman}};

    // Mirror of the registration state, used to respect the documented
    // preconditions (most per-peer methods require a registered peer) and to
    // cross-check the manager's global counters.
    struct PeerMirror {
        bool registered{false};
        bool preferred{false};
        bool sync_started{false};
    };
    std::array<PeerMirror, NUM_PEERS> peers;

    auto rand_block = [&]() -> const CBlockIndex* {
        return ALL_BLOCKS[fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, ALL_BLOCKS.size() - 1)];
    };
    // Usually a known block hash, sometimes an arbitrary one.
    auto rand_hash = [&]() -> uint256 {
        if (fuzzed_data_provider.ConsumeBool()) return ConsumeUInt256(fuzzed_data_provider);
        return rand_block()->GetBlockHash();
    };
    auto check_scheduled_blocks = [&](const std::vector<const CBlockIndex*>& blocks, unsigned int count) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        Assert(blocks.size() <= count);
        for (size_t i = 0; i < blocks.size(); ++i) {
            // Scheduled blocks are never already stored or already in flight,
            // and are returned in ascending height order.
            Assert(!(blocks[i]->nStatus & BLOCK_HAVE_DATA));
            Assert(!bdm.IsBlockRequested(blocks[i]->GetBlockHash()));
            if (i > 0) Assert(blocks[i - 1]->nHeight < blocks[i]->nHeight);
        }
    };

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 500)
    {
        NodeId rand_peer = fuzzed_data_provider.ConsumeIntegralInRange<NodeId>(0, NUM_PEERS - 1);
        auto& mirror = peers[rand_peer];

        CallOneOf(
            fuzzed_data_provider,
            [&] { // Initial registration or re-registration
                BlockDownloadConnectionInfo info{
                    .m_is_inbound = fuzzed_data_provider.ConsumeBool(),
                    .m_preferred_download = fuzzed_data_provider.ConsumeBool(),
                    .m_can_serve_witnesses = fuzzed_data_provider.ConsumeBool(),
                    .m_is_limited_peer = fuzzed_data_provider.ConsumeBool(),
                };
                LOCK(cs_main);
                bdm.ConnectedPeer(rand_peer, info);
                mirror.registered = true;
                // The preferred flag latches: re-registration can set but never clear it.
                mirror.preferred = mirror.preferred || info.m_preferred_download;
            },
            [&] {
                LOCK(cs_main);
                bdm.DisconnectedPeer(rand_peer);
                mirror = PeerMirror{};
            },
            [&] {
                if (!mirror.registered) return;
                LOCK(cs_main);
                if (fuzzed_data_provider.ConsumeBool()) {
                    bdm.UpdateBlockAvailability(rand_peer, rand_hash());
                } else {
                    bdm.ProcessBlockAvailability(rand_peer);
                }
            },
            [&] { // BlockRequested, respecting the net_processing call-site precondition
                if (!mirror.registered) return;
                const CBlockIndex& block{*rand_block()};
                LOCK(cs_main);
                const auto flight_info = bdm.FindBlockInFlight(block.GetBlockHash(), rand_peer);
                if (!flight_info.requested_from_peer &&
                    flight_info.already_in_flight >= MAX_CMPCTBLOCKS_INFLIGHT_PER_BLOCK) {
                    return;
                }
                if (fuzzed_data_provider.ConsumeBool()) {
                    std::list<node::QueuedBlock>::iterator* pit{nullptr};
                    bdm.BlockRequested(rand_peer, block, &pit, mempool);
                    Assert(pit != nullptr);
                    Assert((*pit)->pindex == &block);
                } else {
                    bdm.BlockRequested(rand_peer, block);
                }
                Assert(bdm.IsBlockRequested(block.GetBlockHash()));
            },
            [&] {
                std::optional<NodeId> from_peer;
                if (fuzzed_data_provider.ConsumeBool()) from_peer = rand_peer;
                LOCK(cs_main);
                bdm.RemoveBlockRequest(rand_hash(), from_peer);
            },
            [&] {
                if (!mirror.registered) return;
                std::vector<const CBlockIndex*> blocks;
                NodeId staller{-1};
                const unsigned int count{fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 32)};
                LOCK(cs_main);
                bdm.FindNextBlocksToDownload(rand_peer, count, blocks, staller);
                check_scheduled_blocks(blocks, count);
            },
            [&] {
                if (!mirror.registered) return;
                std::vector<const CBlockIndex*> blocks;
                const unsigned int count{fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 32)};
                // Mimic the call site: from_tip is the last common ancestor of
                // a chain tip and the target block.
                const CBlockIndex* target{rand_block()};
                const CBlockIndex* from_tip{LastCommonAncestor(rand_block(), target)};
                LOCK(cs_main);
                bdm.TryDownloadingHistoricalBlocks(rand_peer, count, blocks, from_tip, target);
                check_scheduled_blocks(blocks, count);
            },
            [&] { // Availability getters consistency
                LOCK(cs_main);
                const CBlockIndex* best_known = bdm.GetBestKnownBlock(rand_peer);
                if (best_known) {
                    Assert(mirror.registered);
                    Assert(bdm.PeerHasHeader(rand_peer, best_known));
                }
                (void)bdm.GetLastCommonBlock(rand_peer);
                (void)bdm.GetBestHeaderSent(rand_peer);
            },
            [&] {
                const CBlockIndex* header_sent{fuzzed_data_provider.ConsumeBool() ? rand_block() : nullptr};
                LOCK(cs_main);
                bdm.SetBestHeaderSent(rand_peer, header_sent);
                if (mirror.registered) Assert(bdm.GetBestHeaderSent(rand_peer) == header_sent);
            },
            [&] {
                if (!mirror.registered) return;
                const bool started{fuzzed_data_provider.ConsumeBool()};
                LOCK(cs_main);
                bdm.SetSyncStarted(rand_peer, started);
                mirror.sync_started = started;
                Assert(bdm.GetSyncStarted(rand_peer) == started);
            },
            [&] {
                const auto time{std::chrono::microseconds{fuzzed_data_provider.ConsumeIntegral<uint32_t>()}};
                LOCK(cs_main);
                bdm.SetStallingSince(rand_peer, time);
                if (mirror.registered) Assert(bdm.GetStallingSince(rand_peer) == time);
            },
            [&] {
                const uint256 hash{rand_hash()};
                LOCK(cs_main);
                bdm.SetBlockSource(hash, rand_peer, /*punish_on_invalid=*/fuzzed_data_provider.ConsumeBool());
                Assert(bdm.GetBlockSource(hash).has_value());
                if (fuzzed_data_provider.ConsumeBool()) {
                    bdm.EraseBlockSource(hash);
                    Assert(!bdm.GetBlockSource(hash).has_value());
                }
            },
            [&] {
                const auto now{std::chrono::seconds{fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, 4'000'000'000)}};
                if (fuzzed_data_provider.ConsumeBool()) bdm.SetLastTipUpdate(now);
                LOCK(cs_main);
                const bool stale{bdm.TipMayBeStale(now, /*n_pow_target_spacing=*/fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(1, 3600))};
                // A stale tip implies that nothing is in flight.
                if (stale) Assert(!bdm.HasBlocksInFlight());
            },
            [&] { // Stalling timeout CAS semantics
                auto expected{fuzzed_data_provider.ConsumeBool() ?
                                  bdm.GetBlockStallingTimeout() :
                                  std::chrono::seconds{fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, 100)}};
                const auto expected_initial{expected};
                const auto desired{std::chrono::seconds{fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(1, 64)}};
                const auto current{bdm.GetBlockStallingTimeout()};
                const bool success{bdm.CompareExchangeBlockStallingTimeout(expected, desired)};
                Assert(success == (expected_initial == current));
                if (success) {
                    Assert(bdm.GetBlockStallingTimeout() == desired);
                } else {
                    Assert(expected == current);
                }
            },
            [&] { // In-flight query consistency
                const uint256 hash{rand_hash()};
                LOCK(cs_main);
                const auto info = bdm.FindBlockInFlight(hash, rand_peer);
                Assert(info.already_in_flight == bdm.CountBlocksInFlight(hash));
                Assert((info.already_in_flight > 0) == bdm.IsBlockRequested(hash));
                if (info.queued_block) Assert(info.requested_from_peer);
                if (info.requested_from_peer) Assert(info.already_in_flight > 0);
                if (info.already_in_flight == 0) Assert(info.first_in_flight);
                if (bdm.IsBlockRequestedFromOutbound(hash)) Assert(bdm.IsBlockRequested(hash));
            });

        // The global counters must agree with the per-peer state.
        LOCK(cs_main);
        size_t total_in_flight{0};
        int downloading_from{0};
        int preferred{0};
        int sync_started{0};
        for (NodeId peer = 0; peer < NUM_PEERS; ++peer) {
            if (!peers[peer].registered) continue;
            const auto& in_flight = bdm.GetBlocksInFlight(peer);
            total_in_flight += in_flight.size();
            downloading_from += !in_flight.empty();
            preferred += peers[peer].preferred;
            sync_started += peers[peer].sync_started;
        }
        Assert(total_in_flight == bdm.GetTotalBlocksInFlight());
        Assert(downloading_from == bdm.GetPeersDownloadingFrom());
        Assert(preferred == bdm.GetNumPreferredDownload());
        Assert(sync_started == bdm.GetNumSyncStarted());
        Assert(bdm.HasBlocksInFlight() == (total_in_flight > 0));
    }
    // Disconnect everybody, check that all data structures are empty.
    LOCK(cs_main);
    for (NodeId peer = 0; peer < NUM_PEERS; ++peer) {
        bdm.DisconnectedPeer(peer);
    }
    bdm.CheckIsEmpty();
}

} // namespace
