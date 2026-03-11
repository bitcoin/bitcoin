// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/blockdownloadman_impl.h>
#include <node/blockdownloadman.h>

#include <blockencodings.h>
#include <chain.h>
#include <deploymentstatus.h>
#include <logging.h>
#include <net_processing.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <txmempool.h>
#include <util/check.h>
#include <validation.h>

namespace node {

// ---------------------------------------------------------------------------
// BlockDownloadManager pimpl forwarding wrappers
// ---------------------------------------------------------------------------

BlockDownloadManager::BlockDownloadManager(const BlockDownloadOptions& options)
    : m_impl{std::make_unique<BlockDownloadManagerImpl>(options)}
{}

BlockDownloadManager::~BlockDownloadManager() = default;

void BlockDownloadManager::ConnectedPeer(NodeId nodeid, const BlockDownloadConnectionInfo& info)
{
    m_impl->ConnectedPeer(nodeid, info);
}

void BlockDownloadManager::DisconnectedPeer(NodeId nodeid)
{
    m_impl->DisconnectedPeer(nodeid);
}

bool BlockDownloadManager::IsBlockRequested(const uint256& hash) const
{
    return m_impl->IsBlockRequested(hash);
}

bool BlockDownloadManager::IsBlockRequestedFromOutbound(const uint256& hash) const
{
    return m_impl->IsBlockRequestedFromOutbound(hash);
}

void BlockDownloadManager::RemoveBlockRequest(const uint256& hash, std::optional<NodeId> from_peer)
{
    m_impl->RemoveBlockRequest(hash, from_peer);
}

bool BlockDownloadManager::BlockRequested(NodeId nodeid, const CBlockIndex& block,
                                          std::list<QueuedBlock>::iterator** pit,
                                          CTxMemPool* mempool)
{
    return m_impl->BlockRequested(nodeid, block, pit, mempool);
}

bool BlockDownloadManager::TipMayBeStale(std::chrono::seconds now, int64_t n_pow_target_spacing)
{
    return m_impl->TipMayBeStale(now, n_pow_target_spacing);
}

void BlockDownloadManager::SetLastTipUpdate(std::chrono::seconds time)
{
    m_impl->m_last_tip_update = time;
}

std::chrono::seconds BlockDownloadManager::GetLastTipUpdate() const
{
    return m_impl->m_last_tip_update.load();
}

void BlockDownloadManager::ProcessBlockAvailability(NodeId nodeid)
{
    m_impl->ProcessBlockAvailability(nodeid);
}

void BlockDownloadManager::UpdateBlockAvailability(NodeId nodeid, const uint256& hash)
{
    m_impl->UpdateBlockAvailability(nodeid, hash);
}

void BlockDownloadManager::FindNextBlocksToDownload(NodeId nodeid, unsigned int count,
                                                    std::vector<const CBlockIndex*>& vBlocks,
                                                    NodeId& nodeStaller)
{
    m_impl->FindNextBlocksToDownload(nodeid, count, vBlocks, nodeStaller);
}

void BlockDownloadManager::TryDownloadingHistoricalBlocks(NodeId nodeid, unsigned int count,
                                                          std::vector<const CBlockIndex*>& vBlocks,
                                                          const CBlockIndex* from_tip,
                                                          const CBlockIndex* target_block)
{
    m_impl->TryDownloadingHistoricalBlocks(nodeid, count, vBlocks, from_tip, target_block);
}

const CBlockIndex* BlockDownloadManager::GetBestKnownBlock(NodeId nodeid) const
{
    const auto* state = m_impl->GetPeerState(nodeid);
    return state ? state->pindexBestKnownBlock : nullptr;
}

const CBlockIndex* BlockDownloadManager::GetBestHeaderSent(NodeId nodeid) const
{
    const auto* state = m_impl->GetPeerState(nodeid);
    return state ? state->pindexBestHeaderSent : nullptr;
}

void BlockDownloadManager::SetBestHeaderSent(NodeId nodeid, const CBlockIndex* pindex)
{
    auto* state = m_impl->GetPeerState(nodeid);
    if (state) state->pindexBestHeaderSent = pindex;
}

const CBlockIndex* BlockDownloadManager::GetLastCommonBlock(NodeId nodeid) const
{
    const auto* state = m_impl->GetPeerState(nodeid);
    return state ? state->pindexLastCommonBlock : nullptr;
}

bool BlockDownloadManager::GetSyncStarted(NodeId nodeid) const
{
    const auto* state = m_impl->GetPeerState(nodeid);
    return state ? state->fSyncStarted : false;
}

void BlockDownloadManager::SetSyncStarted(NodeId nodeid, bool started)
{
    auto* state = m_impl->GetPeerState(nodeid);
    if (!state) return;
    if (state->fSyncStarted && !started) {
        m_impl->nSyncStarted--;
    } else if (!state->fSyncStarted && started) {
        m_impl->nSyncStarted++;
    }
    state->fSyncStarted = started;
}

int BlockDownloadManager::GetNumSyncStarted() const
{
    return m_impl->nSyncStarted;
}

std::chrono::seconds BlockDownloadManager::GetBlockStallingTimeout() const
{
    return m_impl->m_block_stalling_timeout.load();
}

bool BlockDownloadManager::CompareExchangeBlockStallingTimeout(std::chrono::seconds& expected, std::chrono::seconds desired)
{
    return m_impl->m_block_stalling_timeout.compare_exchange_strong(expected, desired);
}

int BlockDownloadManager::GetNumPreferredDownload() const
{
    return m_impl->m_num_preferred_download_peers;
}

int BlockDownloadManager::GetPeersDownloadingFrom() const
{
    return m_impl->m_peers_downloading_from;
}

bool BlockDownloadManager::HasBlocksInFlight() const
{
    return !m_impl->mapBlocksInFlight.empty();
}

size_t BlockDownloadManager::GetTotalBlocksInFlight() const
{
    return m_impl->mapBlocksInFlight.size();
}

const std::list<QueuedBlock>& BlockDownloadManager::GetBlocksInFlight(NodeId nodeid) const
{
    const auto* state = m_impl->GetPeerState(nodeid);
    Assert(state);
    return state->vBlocksInFlight;
}

std::chrono::microseconds BlockDownloadManager::GetDownloadingSince(NodeId nodeid) const
{
    const auto* state = m_impl->GetPeerState(nodeid);
    return state ? state->m_downloading_since : std::chrono::microseconds{0};
}

std::chrono::microseconds BlockDownloadManager::GetStallingSince(NodeId nodeid) const
{
    const auto* state = m_impl->GetPeerState(nodeid);
    return state ? state->m_stalling_since : std::chrono::microseconds{0};
}

void BlockDownloadManager::SetStallingSince(NodeId nodeid, std::chrono::microseconds time)
{
    auto* state = m_impl->GetPeerState(nodeid);
    if (state) state->m_stalling_since = time;
}

bool BlockDownloadManager::IsPreferredDownload(NodeId nodeid) const
{
    const auto* state = m_impl->GetPeerState(nodeid);
    return state ? state->fPreferredDownload : false;
}

std::optional<std::pair<NodeId, bool>> BlockDownloadManager::GetBlockSource(const uint256& hash) const
{
    auto it = m_impl->mapBlockSource.find(hash);
    if (it != m_impl->mapBlockSource.end()) {
        return it->second;
    }
    return std::nullopt;
}

void BlockDownloadManager::SetBlockSource(const uint256& hash, NodeId nodeid, bool punish_on_invalid)
{
    m_impl->mapBlockSource.emplace(hash, std::make_pair(nodeid, punish_on_invalid));
}

void BlockDownloadManager::EraseBlockSource(const uint256& hash)
{
    m_impl->mapBlockSource.erase(hash);
}

size_t BlockDownloadManager::CountBlocksInFlight(const uint256& hash) const
{
    return m_impl->mapBlocksInFlight.count(hash);
}

void BlockDownloadManager::CheckIsEmpty() const
{
    m_impl->CheckIsEmpty();
}

bool BlockDownloadManager::PeerHasHeader(NodeId nodeid, const CBlockIndex* pindex) const
{
    const auto* state = m_impl->GetPeerState(nodeid);
    if (!state) return false;
    return m_impl->PeerHasHeader(*state, pindex);
}

BlockDownloadManager::BlockInFlightInfo BlockDownloadManager::FindBlockInFlight(const uint256& hash, NodeId peer_id) const
{
    BlockInFlightInfo result;
    auto range = m_impl->mapBlocksInFlight.equal_range(hash);
    result.already_in_flight = std::distance(range.first, range.second);
    result.first_in_flight = result.already_in_flight == 0 || (range.first->second.first == peer_id);

    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.first == peer_id) {
            result.requested_from_peer = true;
            if (it->second.second->partialBlock) {
                result.queued_block = &*it->second.second;
            }
            break;
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// BlockDownloadManagerImpl
// ---------------------------------------------------------------------------

BlockDownloadManagerImpl::PeerBlockDownloadState* BlockDownloadManagerImpl::GetPeerState(NodeId nodeid)
{
    auto it = m_peer_info.find(nodeid);
    return it != m_peer_info.end() ? &it->second : nullptr;
}

const BlockDownloadManagerImpl::PeerBlockDownloadState* BlockDownloadManagerImpl::GetPeerState(NodeId nodeid) const
{
    auto it = m_peer_info.find(nodeid);
    return it != m_peer_info.end() ? &it->second : nullptr;
}

void BlockDownloadManagerImpl::ConnectedPeer(NodeId nodeid, const BlockDownloadConnectionInfo& info)
{
    auto [it, inserted] = m_peer_info.try_emplace(nodeid, info);
    auto& state = it->second;
    if (!inserted) {
        // Re-registration: update connection info.
        state.m_connection_info = info;
    }
    // Set preferred download flag and update counter (handles both first
    // registration and re-registration).
    if (info.m_preferred_download && !state.fPreferredDownload) {
        state.fPreferredDownload = true;
        m_num_preferred_download_peers++;
    }
}

void BlockDownloadManagerImpl::DisconnectedPeer(NodeId nodeid)
{
    auto it = m_peer_info.find(nodeid);
    if (it == m_peer_info.end()) return;

    auto& state = it->second;

    if (state.fSyncStarted) {
        nSyncStarted--;
    }

    // Remove all in-flight block entries for this peer from the global map.
    for (const QueuedBlock& entry : state.vBlocksInFlight) {
        auto range = mapBlocksInFlight.equal_range(entry.pindex->GetBlockHash());
        while (range.first != range.second) {
            auto [node_id, list_it] = range.first->second;
            if (node_id != nodeid) {
                range.first++;
            } else {
                range.first = mapBlocksInFlight.erase(range.first);
            }
        }
    }

    m_num_preferred_download_peers -= state.fPreferredDownload;
    m_peers_downloading_from -= (!state.vBlocksInFlight.empty());
    assert(m_peers_downloading_from >= 0);

    m_peer_info.erase(it);
}

bool BlockDownloadManagerImpl::IsBlockRequested(const uint256& hash) const
{
    return mapBlocksInFlight.contains(hash);
}

bool BlockDownloadManagerImpl::IsBlockRequestedFromOutbound(const uint256& hash) const
{
    for (auto range = mapBlocksInFlight.equal_range(hash); range.first != range.second; range.first++) {
        auto [nodeid, block_it] = range.first->second;
        const auto* state = GetPeerState(nodeid);
        if (state && !state->m_connection_info.m_is_inbound) return true;
    }
    return false;
}

void BlockDownloadManagerImpl::RemoveBlockRequest(const uint256& hash, std::optional<NodeId> from_peer)
{
    auto range = mapBlocksInFlight.equal_range(hash);
    if (range.first == range.second) {
        // Block was not requested from any peer
        return;
    }

    // We should not have requested too many of this block
    Assume(mapBlocksInFlight.count(hash) <= MAX_CMPCTBLOCKS_INFLIGHT_PER_BLOCK);

    while (range.first != range.second) {
        const auto& [node_id, list_it]{range.first->second};

        if (from_peer && *from_peer != node_id) {
            range.first++;
            continue;
        }

        auto* state = Assert(GetPeerState(node_id));

        if (state->vBlocksInFlight.begin() == list_it) {
            // First block on the queue was received, update the start download time for the next one
            state->m_downloading_since = std::max(state->m_downloading_since, GetTime<std::chrono::microseconds>());
        }
        state->vBlocksInFlight.erase(list_it);

        if (state->vBlocksInFlight.empty()) {
            // Last validated block on the queue for this peer was received.
            m_peers_downloading_from--;
        }
        state->m_stalling_since = 0us;

        range.first = mapBlocksInFlight.erase(range.first);
    }
}

bool BlockDownloadManagerImpl::BlockRequested(NodeId nodeid, const CBlockIndex& block,
                                              std::list<QueuedBlock>::iterator** pit,
                                              CTxMemPool* mempool)
{
    const uint256& hash{block.GetBlockHash()};

    auto* state = Assert(GetPeerState(nodeid));

    Assume(mapBlocksInFlight.count(hash) <= MAX_CMPCTBLOCKS_INFLIGHT_PER_BLOCK);

    // Short-circuit most stuff in case it is from the same node
    for (auto range = mapBlocksInFlight.equal_range(hash); range.first != range.second; range.first++) {
        if (range.first->second.first == nodeid) {
            if (pit) {
                *pit = &range.first->second.second;
            }
            return false;
        }
    }

    // Make sure it's not being fetched already from same peer.
    RemoveBlockRequest(hash, nodeid);

    std::list<QueuedBlock>::iterator it = state->vBlocksInFlight.insert(state->vBlocksInFlight.end(),
            {&block, std::unique_ptr<PartiallyDownloadedBlock>(pit ? new PartiallyDownloadedBlock(mempool) : nullptr)});
    if (state->vBlocksInFlight.size() == 1) {
        // We're starting a block download (batch) from this peer.
        state->m_downloading_since = GetTime<std::chrono::microseconds>();
        m_peers_downloading_from++;
    }
    auto itInFlight = mapBlocksInFlight.insert(std::make_pair(hash, std::make_pair(nodeid, it)));
    if (pit) {
        *pit = &itInFlight->second.second;
    }
    return true;
}

bool BlockDownloadManagerImpl::TipMayBeStale(std::chrono::seconds now, int64_t n_pow_target_spacing)
{
    if (m_last_tip_update.load() == 0s) {
        m_last_tip_update = now;
        return false;
    }
    return m_last_tip_update.load() < now - std::chrono::seconds{n_pow_target_spacing * 3} && mapBlocksInFlight.empty();
}

void BlockDownloadManagerImpl::ProcessBlockAvailability(NodeId nodeid)
{
    auto* state = Assert(GetPeerState(nodeid));

    if (!state->hashLastUnknownBlock.IsNull()) {
        const CBlockIndex* pindex = m_opts.m_chainman.m_blockman.LookupBlockIndex(state->hashLastUnknownBlock);
        if (pindex && pindex->nChainWork > 0) {
            if (state->pindexBestKnownBlock == nullptr || pindex->nChainWork >= state->pindexBestKnownBlock->nChainWork) {
                state->pindexBestKnownBlock = pindex;
            }
            state->hashLastUnknownBlock.SetNull();
        }
    }
}

void BlockDownloadManagerImpl::UpdateBlockAvailability(NodeId nodeid, const uint256& hash)
{
    auto* state = Assert(GetPeerState(nodeid));

    ProcessBlockAvailability(nodeid);

    const CBlockIndex* pindex = m_opts.m_chainman.m_blockman.LookupBlockIndex(hash);
    if (pindex && pindex->nChainWork > 0) {
        // An actually better block was announced.
        if (state->pindexBestKnownBlock == nullptr || pindex->nChainWork >= state->pindexBestKnownBlock->nChainWork) {
            state->pindexBestKnownBlock = pindex;
        }
    } else {
        // An unknown block was announced; just assume that the latest one is the best one.
        state->hashLastUnknownBlock = hash;
    }
}

void BlockDownloadManagerImpl::FindNextBlocksToDownload(NodeId nodeid, unsigned int count,
                                                        std::vector<const CBlockIndex*>& vBlocks,
                                                        NodeId& nodeStaller)
{
    if (count == 0) return;

    vBlocks.reserve(vBlocks.size() + count);
    auto* state = Assert(GetPeerState(nodeid));

    // Make sure pindexBestKnownBlock is up to date, we'll need it.
    ProcessBlockAvailability(nodeid);

    if (state->pindexBestKnownBlock == nullptr ||
        state->pindexBestKnownBlock->nChainWork < m_opts.m_chainman.ActiveChain().Tip()->nChainWork ||
        state->pindexBestKnownBlock->nChainWork < m_opts.m_chainman.MinimumChainWork()) {
        // This peer has nothing interesting.
        return;
    }

    // When syncing with AssumeUtxo and the snapshot has not yet been validated,
    // abort downloading blocks from peers that don't have the snapshot block in their best chain.
    // We can't reorg to this chain due to missing undo data until validation completes,
    // so downloading blocks from it would be futile.
    const CBlockIndex* snap_base{m_opts.m_chainman.CurrentChainstate().SnapshotBase()};
    if (snap_base && m_opts.m_chainman.CurrentChainstate().m_assumeutxo == Assumeutxo::UNVALIDATED &&
        state->pindexBestKnownBlock->GetAncestor(snap_base->nHeight) != snap_base) {
        LogDebug(BCLog::NET, "Not downloading blocks from peer=%d, which doesn't have the snapshot block in its best chain.\n", nodeid);
        return;
    }

    // Determine the forking point between the peer's chain and our chain:
    // pindexLastCommonBlock is required to be an ancestor of pindexBestKnownBlock, and will be used as a starting point.
    // It is being set to the fork point between the peer's best known block and the current tip, unless it is already set to
    // an ancestor with more work than the fork point.
    auto fork_point = LastCommonAncestor(state->pindexBestKnownBlock, m_opts.m_chainman.ActiveTip());
    if (state->pindexLastCommonBlock == nullptr ||
        fork_point->nChainWork > state->pindexLastCommonBlock->nChainWork ||
        state->pindexBestKnownBlock->GetAncestor(state->pindexLastCommonBlock->nHeight) != state->pindexLastCommonBlock) {
        state->pindexLastCommonBlock = fork_point;
    }
    if (state->pindexLastCommonBlock == state->pindexBestKnownBlock)
        return;

    const CBlockIndex* pindexWalk = state->pindexLastCommonBlock;
    // Never fetch further than the best block we know the peer has, or more than BLOCK_DOWNLOAD_WINDOW + 1 beyond the last
    // linked block we have in common with this peer. The +1 is so we can detect stalling, namely if we would be able to
    // download that next block if the window were 1 larger.
    int nWindowEnd = state->pindexLastCommonBlock->nHeight + BLOCK_DOWNLOAD_WINDOW;

    FindNextBlocks(vBlocks, nodeid, *state, pindexWalk, count, nWindowEnd, &m_opts.m_chainman.ActiveChain(), &nodeStaller);
}

void BlockDownloadManagerImpl::TryDownloadingHistoricalBlocks(NodeId nodeid, unsigned int count,
                                                              std::vector<const CBlockIndex*>& vBlocks,
                                                              const CBlockIndex* from_tip,
                                                              const CBlockIndex* target_block)
{
    Assert(from_tip);
    Assert(target_block);

    if (vBlocks.size() >= count) {
        return;
    }

    vBlocks.reserve(count);
    auto* state = Assert(GetPeerState(nodeid));

    if (state->pindexBestKnownBlock == nullptr ||
        state->pindexBestKnownBlock->GetAncestor(target_block->nHeight) != target_block) {
        // This peer can't provide us the complete series of blocks leading up to the
        // assumeutxo snapshot base.
        //
        // Presumably this peer's chain has less work than our ActiveChain()'s tip, or else we
        // will eventually crash when we try to reorg to it. Let other logic
        // deal with whether we disconnect this peer.
        //
        // TODO at some point in the future, we might choose to request what blocks
        // this peer does have from the historical chain, despite it not having a
        // complete history beneath the snapshot base.
        return;
    }

    FindNextBlocks(vBlocks, nodeid, *state, from_tip, count,
                   std::min<int>(from_tip->nHeight + BLOCK_DOWNLOAD_WINDOW, target_block->nHeight));
}

void BlockDownloadManagerImpl::FindNextBlocks(std::vector<const CBlockIndex*>& vBlocks,
                                              NodeId nodeid,
                                              PeerBlockDownloadState& state,
                                              const CBlockIndex* pindexWalk,
                                              unsigned int count, int nWindowEnd,
                                              const CChain* activeChain,
                                              NodeId* nodeStaller)
{
    std::vector<const CBlockIndex*> vToFetch;
    int nMaxHeight = std::min<int>(state.pindexBestKnownBlock->nHeight, nWindowEnd + 1);
    bool is_limited_peer = state.m_connection_info.m_is_limited_peer;
    NodeId waitingfor = -1;
    while (pindexWalk->nHeight < nMaxHeight) {
        // Read up to 128 (or more, if more blocks than that are needed) successors of pindexWalk (towards
        // pindexBestKnownBlock) into vToFetch. We fetch 128, because CBlockIndex::GetAncestor may be as expensive
        // as iterating over ~100 CBlockIndex* entries anyway.
        int nToFetch = std::min(nMaxHeight - pindexWalk->nHeight, std::max<int>(count - vBlocks.size(), 128));
        vToFetch.resize(nToFetch);
        pindexWalk = state.pindexBestKnownBlock->GetAncestor(pindexWalk->nHeight + nToFetch);
        vToFetch[nToFetch - 1] = pindexWalk;
        for (unsigned int i = nToFetch - 1; i > 0; i--) {
            vToFetch[i - 1] = vToFetch[i]->pprev;
        }

        // Iterate over those blocks in vToFetch (in forward direction), adding the ones that
        // are not yet downloaded and not in flight to vBlocks. In the meantime, update
        // pindexLastCommonBlock as long as all ancestors are already downloaded, or if it's
        // already part of our chain (and therefore don't need it even if pruned).
        for (const CBlockIndex* pindex : vToFetch) {
            if (!pindex->IsValid(BLOCK_VALID_TREE)) {
                // We consider the chain that this peer is on invalid.
                return;
            }

            if (!state.m_connection_info.m_can_serve_witnesses &&
                DeploymentActiveAt(*pindex, m_opts.m_chainman, Consensus::DEPLOYMENT_SEGWIT)) {
                // We wouldn't download this block or its descendants from this peer.
                return;
            }

            if (pindex->nStatus & BLOCK_HAVE_DATA || (activeChain && activeChain->Contains(pindex))) {
                if (activeChain && pindex->HaveNumChainTxs()) {
                    state.pindexLastCommonBlock = pindex;
                }
                continue;
            }

            // Is block in-flight?
            if (IsBlockRequested(pindex->GetBlockHash())) {
                if (waitingfor == -1) {
                    // This is the first already-in-flight block.
                    waitingfor = mapBlocksInFlight.lower_bound(pindex->GetBlockHash())->second.first;
                }
                continue;
            }

            // The block is not already downloaded, and not yet in flight.
            if (pindex->nHeight > nWindowEnd) {
                // We reached the end of the window.
                if (vBlocks.size() == 0 && waitingfor != nodeid) {
                    // We aren't able to fetch anything, but we would be if the download window was one larger.
                    if (nodeStaller) *nodeStaller = waitingfor;
                }
                return;
            }

            // Don't request blocks that go further than what limited peers can provide
            if (is_limited_peer && (state.pindexBestKnownBlock->nHeight - pindex->nHeight >= static_cast<int>(NODE_NETWORK_LIMITED_MIN_BLOCKS) - 2 /* two blocks buffer for possible races */)) {
                continue;
            }

            vBlocks.push_back(pindex);
            if (vBlocks.size() == count) {
                return;
            }
        }
    }
}

bool BlockDownloadManagerImpl::PeerHasHeader(const PeerBlockDownloadState& state, const CBlockIndex* pindex) const
{
    if (state.pindexBestKnownBlock && pindex == state.pindexBestKnownBlock->GetAncestor(pindex->nHeight))
        return true;
    if (state.pindexBestHeaderSent && pindex == state.pindexBestHeaderSent->GetAncestor(pindex->nHeight))
        return true;
    return false;
}

void BlockDownloadManagerImpl::CheckIsEmpty() const
{
    assert(mapBlocksInFlight.empty());
    assert(m_num_preferred_download_peers == 0);
    assert(m_peers_downloading_from == 0);
}

} // namespace node
