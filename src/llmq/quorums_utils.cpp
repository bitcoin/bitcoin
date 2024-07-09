// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums.h>
#include <llmq/quorums_utils.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_init.h>
#include <timedata.h>
#include <bls/bls.h>
#include <evo/deterministicmns.h>
#include <evo/evodb.h>
#include <chainparams.h>
#include <random.h>
#include <spork.h>
#include <validation.h>
#include <versionbits.h>
#include <masternode/masternodemeta.h>
#include <util/ranges.h>
#include <common/args.h>
#include <logging.h>
#include <unordered_lru_cache.h>
namespace llmq
{
bool CLLMQUtils::IsV19Active(const int nHeight)
{
    return nHeight >= Params().GetConsensus().nV19StartBlock; 
}
const CBlockIndex* CLLMQUtils::V19ActivationIndex(const CBlockIndex* pindex)
{
    assert(pindex);
    if(!IsV19Active(pindex->nHeight)) {
        return nullptr;
    }
    return pindex->GetAncestor(Params().GetConsensus().nV19StartBlock);
}
std::vector<CDeterministicMNCPtr> CLLMQUtils::GetAllQuorumMembers(const CBlockIndex* pQuorumBaseBlockIndex)
{
    static RecursiveMutex cs_members;
    static unordered_lru_cache<uint256, std::vector<CDeterministicMNCPtr>, StaticSaltedHasher, 10> mapQuorumMembers;
    const Consensus::LLMQParams& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    std::vector<CDeterministicMNCPtr> quorumMembers;
    {
        LOCK(cs_members);
        if (mapQuorumMembers.get(pQuorumBaseBlockIndex->GetBlockHash(), quorumMembers)) {
            return quorumMembers;
        }
    }

    auto allMns = deterministicMNManager->GetListForBlock(pQuorumBaseBlockIndex);
    auto modifier = pQuorumBaseBlockIndex->GetBlockHash();
    quorumMembers = allMns.CalculateQuorum(llmqParams.size, modifier);
    LOCK(cs_members);
    mapQuorumMembers.insert(pQuorumBaseBlockIndex->GetBlockHash(), quorumMembers);
    return quorumMembers;
}

uint256 CLLMQUtils::BuildCommitmentHash(const uint256& blockHash, const std::vector<bool>& validMembers, const CBLSPublicKey& pubKey, const uint256& vvecHash)
{
    CHashWriter hw(SER_GETHASH, 0);
    hw << blockHash;
    hw << DYNBITSET(validMembers);
    hw << pubKey;
    hw << vvecHash;
    return hw.GetHash();
}

uint256 CLLMQUtils::BuildSignHash(const uint256& quorumHash, const uint256& id, const uint256& msgHash)
{
    CHashWriter h(SER_GETHASH, 0);
    h << quorumHash;
    h << id;
    h << msgHash;
    return h.GetHash();
}

static bool EvalSpork(int64_t spork_value)
{
    if (spork_value == 0) {
        return true;
    }
    if (spork_value == 1) {
        return true;
    }
    return false;
}

bool CLLMQUtils::IsAllMembersConnectedEnabled()
{
    return EvalSpork(sporkManager->GetSporkValue(SPORK_21_QUORUM_ALL_CONNECTED));
}

bool CLLMQUtils::IsQuorumPoseEnabled()
{
    return EvalSpork(sporkManager->GetSporkValue(SPORK_23_QUORUM_POSE));
}

uint256 CLLMQUtils::DeterministicOutboundConnection(const uint256& proTxHash1, const uint256& proTxHash2)
{
    // We need to deterministically select who is going to initiate the connection. The naive way would be to simply
    // return the min(proTxHash1, proTxHash2), but this would create a bias towards MNs with a numerically low
    // hash. To fix this, we return the proTxHash that has the lowest value of:
    //   hash(min(proTxHash1, proTxHash2), max(proTxHash1, proTxHash2), proTxHashX)
    // where proTxHashX is the proTxHash to compare
    uint256 h1;
    uint256 h2;
    if (proTxHash1 < proTxHash2) {
        h1 = ::SerializeHash(std::make_tuple(proTxHash1, proTxHash2, proTxHash1));
        h2 = ::SerializeHash(std::make_tuple(proTxHash1, proTxHash2, proTxHash2));
    } else {
        h1 = ::SerializeHash(std::make_tuple(proTxHash2, proTxHash1, proTxHash1));
        h2 = ::SerializeHash(std::make_tuple(proTxHash2, proTxHash1, proTxHash2));
    }
    if (h1 < h2) {
        return proTxHash1;
    }
    return proTxHash2;
}

std::set<uint256> CLLMQUtils::GetQuorumConnections(const CBlockIndex* pQuorumBaseBlockIndex, const uint256& forMember, bool onlyOutbound)
{
    if (IsAllMembersConnectedEnabled()) {
        auto mns = GetAllQuorumMembers(pQuorumBaseBlockIndex);
        std::set<uint256> result;

        for (const auto& dmn : mns) {
            if (dmn->proTxHash == forMember) {
                continue;
            }
            // Determine which of the two MNs (forMember vs dmn) should initiate the outbound connection and which
            // one should wait for the inbound connection. We do this in a deterministic way, so that even when we
            // end up with both connecting to each other, we know which one to disconnect
            uint256 deterministicOutbound = DeterministicOutboundConnection(forMember, dmn->proTxHash);
            if (!onlyOutbound || deterministicOutbound == dmn->proTxHash) {
                result.emplace(dmn->proTxHash);
            }
        }
        return result;
    } else {
        return GetQuorumRelayMembers(pQuorumBaseBlockIndex, forMember, onlyOutbound);
    }
}

std::set<uint256> CLLMQUtils::GetQuorumRelayMembers(const CBlockIndex* pQuorumBaseBlockIndex, const uint256 &forMember, bool onlyOutbound)
{
    auto mns = GetAllQuorumMembers(pQuorumBaseBlockIndex);
    std::set<uint256> result;

    auto calcOutbound = [&](size_t i, const uint256 &proTxHash) {
        // Relay to nodes at indexes (i+2^k)%n, where
        //   k: 0..max(1, floor(log2(n-1))-1)
        //   n: size of the quorum/ring
        std::set<uint256> r;
        int gap = 1;
        int gap_max = (int)mns.size() - 1;
        int k = 0;
        while ((gap_max >>= 1) || k <= 1) {
            size_t idx = (i + gap) % mns.size();
            const auto& otherDmn = mns[idx];
            if (otherDmn->proTxHash == proTxHash) {
                gap <<= 1;
                k++;
                continue;
            }
            r.emplace(otherDmn->proTxHash);
            gap <<= 1;
            k++;
        }
        return r;
    };

    for (size_t i = 0; i < mns.size(); i++) {
        const auto& dmn = mns[i];
        if (dmn->proTxHash == forMember) {
            auto r = calcOutbound(i, dmn->proTxHash);
            result.insert(r.begin(), r.end());
        } else if (!onlyOutbound) {
            auto r = calcOutbound(i, dmn->proTxHash);
            if (r.count(forMember)) {
                result.emplace(dmn->proTxHash);
            }
        }
    }

    return result;
}

std::set<size_t> CLLMQUtils::CalcDeterministicWatchConnections(const CBlockIndex* pQuorumBaseBlockIndex, size_t memberCount, size_t connectionCount)
{
    static uint256 qwatchConnectionSeed;
    static std::atomic<bool> qwatchConnectionSeedGenerated{false};
    static RecursiveMutex qwatchConnectionSeedCs;
    if (!qwatchConnectionSeedGenerated) {
        LOCK(qwatchConnectionSeedCs);
        qwatchConnectionSeed = GetRandHash();
        qwatchConnectionSeedGenerated = true;
    }

    std::set<size_t> result;
    uint256 rnd = qwatchConnectionSeed;
    for (size_t i = 0; i < connectionCount; i++) {
        rnd = ::SerializeHash(std::make_pair(rnd, pQuorumBaseBlockIndex->GetBlockHash()));
        result.emplace(rnd.GetUint64(0) % memberCount);
    }
    return result;
}

bool CLLMQUtils::EnsureQuorumConnections(const CBlockIndex *pQuorumBaseBlockIndex, const uint256& myProTxHash, CConnman& connman)
{
    if (!fMasternodeMode && !CLLMQUtils::IsWatchQuorumsEnabled()) return false;
    auto members = GetAllQuorumMembers(pQuorumBaseBlockIndex);
    bool isMember = std::find_if(members.begin(), members.end(), [&](const auto& dmn) { return dmn->proTxHash == myProTxHash; }) != members.end();

    if (!isMember && !CLLMQUtils::IsWatchQuorumsEnabled()) {
        return false;
    }

    std::set<uint256> connections;
    std::set<uint256> relayMembers;
    if (isMember) {
        connections = CLLMQUtils::GetQuorumConnections(pQuorumBaseBlockIndex, myProTxHash, true);
        relayMembers = CLLMQUtils::GetQuorumRelayMembers(pQuorumBaseBlockIndex, myProTxHash, true);
    } else {
        auto cindexes = CLLMQUtils::CalcDeterministicWatchConnections(pQuorumBaseBlockIndex, members.size(), 1);
        for (auto idx : cindexes) {
            connections.emplace(members[idx]->proTxHash);
        }
        relayMembers = connections;
    }
    if (!connections.empty()) {
        if (!connman.HasMasternodeQuorumNodes(pQuorumBaseBlockIndex->GetBlockHash()) && LogAcceptCategory(BCLog::LLMQ, BCLog::Level::Debug)) {
            auto mnList = deterministicMNManager->GetListAtChainTip();
            std::string debugMsg = strprintf("CLLMQUtils::%s -- adding masternodes quorum connections for quorum %s:", __func__, pQuorumBaseBlockIndex->GetBlockHash().ToString());
            for (auto& c : connections) {
                auto dmn = mnList.GetValidMN(c);
                if (!dmn) {
                    debugMsg += strprintf("  %s (not in valid MN set anymore)", c.ToString());
                } else {
                    debugMsg += strprintf("  %s (%s)", c.ToString(), dmn->pdmnState->addr.ToStringAddrPort());
                }
            }
            LogPrint(BCLog::NET, "%s\n", debugMsg.c_str());
        }
        connman.SetMasternodeQuorumNodes(pQuorumBaseBlockIndex->GetBlockHash(), connections);
    }
    if (!relayMembers.empty()) {
        connman.SetMasternodeQuorumRelayMembers(pQuorumBaseBlockIndex->GetBlockHash(), relayMembers);
    }
    return true;
}

bool CLLMQUtils::IsWatchQuorumsEnabled()
{
    static bool fIsWatchQuroumsEnabled = gArgs.GetBoolArg("-watchquorums", llmq::DEFAULT_WATCH_QUORUMS);
    return fIsWatchQuroumsEnabled;
}

void CLLMQUtils::AddQuorumProbeConnections(const CBlockIndex *pQuorumBaseBlockIndex, const uint256 &myProTxHash, CConnman& connman)
{
    if (!CLLMQUtils::IsQuorumPoseEnabled()) {
        return;
    }

    auto members = GetAllQuorumMembers(pQuorumBaseBlockIndex);
    auto curTime = TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime());

    std::set<uint256> probeConnections;
    for (const auto& dmn : members) {
        if (dmn->proTxHash == myProTxHash) {
            continue;
        }
        auto lastOutbound = mmetaman->GetMetaInfo(dmn->proTxHash)->GetLastOutboundSuccess();
        // re-probe after 10 minutes so that the "good connection" check in the DKG doesn't fail just because we're on
        // the brink of timeout
        if (curTime - lastOutbound > 10 * 60) {
            probeConnections.emplace(dmn->proTxHash);
        }
    }

    if (!probeConnections.empty()) {
        if (LogAcceptCategory(BCLog::LLMQ, BCLog::Level::Debug)) {
            auto mnList = deterministicMNManager->GetListAtChainTip();
            std::string debugMsg = strprintf("CLLMQUtils::%s -- adding masternodes probes for quorum %s:", __func__, pQuorumBaseBlockIndex->GetBlockHash().ToString());
            for (auto& c : probeConnections) {
                auto dmn = mnList.GetValidMN(c);
                if (!dmn) {
                    debugMsg += strprintf("  %s (not in valid MN set anymore)", c.ToString());
                } else {
                    debugMsg += strprintf("  %s (%s)", c.ToString(), dmn->pdmnState->addr.ToStringAddrPort());
                }
            }
            LogPrint(BCLog::NET, "%s\n", debugMsg.c_str());
        }
        connman.AddPendingProbeConnections(probeConnections);
    }
}

bool CLLMQUtils::IsQuorumActive(const uint256& quorumHash)
{
    auto& params = Params().GetConsensus().llmqTypeChainLocks;

    // sig shares and recovered sigs are only accepted from recent/active quorums
    // we allow one more active quorum as specified in consensus, as otherwise there is a small window where things could
    // fail while we are on the brink of a new quorum
    auto quorums = quorumManager->ScanQuorums((int)params.signingActiveQuorumCount + 1);
    return ranges::any_of(quorums, [&quorumHash](const auto& q){ return q->qc->quorumHash == quorumHash; });
}
} // namespace llmq
