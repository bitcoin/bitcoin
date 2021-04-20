// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_utils.h>

#include <bls/bls.h>
#include <evo/deterministicmns.h>
#include <evo/evodb.h>
#include <chainparams.h>
#include <random.h>
#include <spork.h>
#include <validation.h>
#include <versionbits.h>

#include <masternode/masternode-meta.h>

namespace llmq
{

CCriticalSection cs_llmq_vbc;
VersionBitsCache llmq_versionbitscache;

std::vector<CDeterministicMNCPtr> CLLMQUtils::GetAllQuorumMembers(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum)
{
    static CCriticalSection cs_members;
    static std::map<Consensus::LLMQType, unordered_lru_cache<uint256, std::vector<CDeterministicMNCPtr>, StaticSaltedHasher>> mapQuorumMembers;

    if (!IsQuorumTypeEnabled(llmqType, pindexQuorum->pprev)) {
        return {};
    }
    std::vector<CDeterministicMNCPtr> quorumMembers;
    {
        LOCK(cs_members);
        if (mapQuorumMembers.empty()) {
            InitQuorumsCache(mapQuorumMembers);
        }
        if (mapQuorumMembers[llmqType].get(pindexQuorum->GetBlockHash(), quorumMembers)) {
            return quorumMembers;
        }
    }

    auto& params = Params().GetConsensus().llmqs.at(llmqType);
    auto allMns = deterministicMNManager->GetListForBlock(pindexQuorum);
    auto modifier = ::SerializeHash(std::make_pair(llmqType, pindexQuorum->GetBlockHash()));
    quorumMembers = allMns.CalculateQuorum(params.size, modifier);
    LOCK(cs_members);
    mapQuorumMembers[llmqType].insert(pindexQuorum->GetBlockHash(), quorumMembers);
    return quorumMembers;
}

uint256 CLLMQUtils::BuildCommitmentHash(Consensus::LLMQType llmqType, const uint256& blockHash, const std::vector<bool>& validMembers, const CBLSPublicKey& pubKey, const uint256& vvecHash)
{
    CHashWriter hw(SER_NETWORK, 0);
    hw << llmqType;
    hw << blockHash;
    hw << DYNBITSET(validMembers);
    hw << pubKey;
    hw << vvecHash;
    return hw.GetHash();
}

uint256 CLLMQUtils::BuildSignHash(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& id, const uint256& msgHash)
{
    CHashWriter h(SER_GETHASH, 0);
    h << llmqType;
    h << quorumHash;
    h << id;
    h << msgHash;
    return h.GetHash();
}

static bool EvalSpork(Consensus::LLMQType llmqType, int64_t spork_value)
{
    if (spork_value == 0) {
        return true;
    }
    if (spork_value == 1 && llmqType != Consensus::LLMQ_100_67 && llmqType != Consensus::LLMQ_400_60 && llmqType != Consensus::LLMQ_400_85) {
        return true;
    }
    return false;
}

bool CLLMQUtils::IsAllMembersConnectedEnabled(Consensus::LLMQType llmqType)
{
    return EvalSpork(llmqType, sporkManager.GetSporkValue(SPORK_21_QUORUM_ALL_CONNECTED));
}

bool CLLMQUtils::IsQuorumPoseEnabled(Consensus::LLMQType llmqType)
{
    return EvalSpork(llmqType, sporkManager.GetSporkValue(SPORK_23_QUORUM_POSE));
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

std::set<uint256> CLLMQUtils::GetQuorumConnections(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum, const uint256& forMember, bool onlyOutbound)
{
    if (IsAllMembersConnectedEnabled(llmqType)) {
        auto mns = GetAllQuorumMembers(llmqType, pindexQuorum);
        std::set<uint256> result;

        for (auto& dmn : mns) {
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
        return GetQuorumRelayMembers(llmqType, pindexQuorum, forMember, onlyOutbound);
    }
}

std::set<uint256> CLLMQUtils::GetQuorumRelayMembers(Consensus::LLMQType llmqType, const CBlockIndex *pindexQuorum, const uint256 &forMember, bool onlyOutbound)
{
    auto mns = GetAllQuorumMembers(llmqType, pindexQuorum);
    std::set<uint256> result;

    auto calcOutbound = [&](size_t i, const uint256 proTxHash) {
        // Relay to nodes at indexes (i+2^k)%n, where
        //   k: 0..max(1, floor(log2(n-1))-1)
        //   n: size of the quorum/ring
        std::set<uint256> r;
        int gap = 1;
        int gap_max = (int)mns.size() - 1;
        int k = 0;
        while ((gap_max >>= 1) || k <= 1) {
            size_t idx = (i + gap) % mns.size();
            auto& otherDmn = mns[idx];
            if (otherDmn->proTxHash == proTxHash) {
                continue;
            }
            r.emplace(otherDmn->proTxHash);
            gap <<= 1;
            k++;
        }
        return r;
    };

    for (size_t i = 0; i < mns.size(); i++) {
        auto& dmn = mns[i];
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

std::set<size_t> CLLMQUtils::CalcDeterministicWatchConnections(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum, size_t memberCount, size_t connectionCount)
{
    static uint256 qwatchConnectionSeed;
    static std::atomic<bool> qwatchConnectionSeedGenerated{false};
    static CCriticalSection qwatchConnectionSeedCs;
    if (!qwatchConnectionSeedGenerated) {
        LOCK(qwatchConnectionSeedCs);
        if (!qwatchConnectionSeedGenerated) {
            qwatchConnectionSeed = GetRandHash();
            qwatchConnectionSeedGenerated = true;
        }
    }

    std::set<size_t> result;
    uint256 rnd = qwatchConnectionSeed;
    for (size_t i = 0; i < connectionCount; i++) {
        rnd = ::SerializeHash(std::make_pair(rnd, std::make_pair(llmqType, pindexQuorum->GetBlockHash())));
        result.emplace(rnd.GetUint64(0) % memberCount);
    }
    return result;
}

bool CLLMQUtils::EnsureQuorumConnections(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum, const uint256& myProTxHash)
{
    auto members = GetAllQuorumMembers(llmqType, pindexQuorum);
    bool isMember = std::find_if(members.begin(), members.end(), [&](const CDeterministicMNCPtr& dmn) { return dmn->proTxHash == myProTxHash; }) != members.end();

    if (!isMember && !CLLMQUtils::IsWatchQuorumsEnabled()) {
        return false;
    }

    std::set<uint256> connections;
    std::set<uint256> relayMembers;
    if (isMember) {
        connections = CLLMQUtils::GetQuorumConnections(llmqType, pindexQuorum, myProTxHash, true);
        relayMembers = CLLMQUtils::GetQuorumRelayMembers(llmqType, pindexQuorum, myProTxHash, true);
    } else {
        auto cindexes = CLLMQUtils::CalcDeterministicWatchConnections(llmqType, pindexQuorum, members.size(), 1);
        for (auto idx : cindexes) {
            connections.emplace(members[idx]->proTxHash);
        }
        relayMembers = connections;
    }
    if (!connections.empty()) {
        if (!g_connman->HasMasternodeQuorumNodes(llmqType, pindexQuorum->GetBlockHash()) && LogAcceptCategory(BCLog::LLMQ)) {
            auto mnList = deterministicMNManager->GetListAtChainTip();
            std::string debugMsg = strprintf("CLLMQUtils::%s -- adding masternodes quorum connections for quorum %s:\n", __func__, pindexQuorum->GetBlockHash().ToString());
            for (auto& c : connections) {
                auto dmn = mnList.GetValidMN(c);
                if (!dmn) {
                    debugMsg += strprintf("  %s (not in valid MN set anymore)\n", c.ToString());
                } else {
                    debugMsg += strprintf("  %s (%s)\n", c.ToString(), dmn->pdmnState->addr.ToString(false));
                }
            }
            LogPrint(BCLog::NET_NETCONN, debugMsg.c_str()); /* Continued */
        }
        g_connman->SetMasternodeQuorumNodes(llmqType, pindexQuorum->GetBlockHash(), connections);
    }
    if (!relayMembers.empty()) {
        g_connman->SetMasternodeQuorumRelayMembers(llmqType, pindexQuorum->GetBlockHash(), relayMembers);
    }
    return true;
}

void CLLMQUtils::AddQuorumProbeConnections(Consensus::LLMQType llmqType, const CBlockIndex *pindexQuorum, const uint256 &myProTxHash)
{
    if (!CLLMQUtils::IsQuorumPoseEnabled(llmqType)) {
        return;
    }

    auto members = GetAllQuorumMembers(llmqType, pindexQuorum);
    auto curTime = GetAdjustedTime();

    std::set<uint256> probeConnections;
    for (auto& dmn : members) {
        if (dmn->proTxHash == myProTxHash) {
            continue;
        }
        auto lastOutbound = mmetaman.GetMetaInfo(dmn->proTxHash)->GetLastOutboundSuccess();
        // re-probe after 50 minutes so that the "good connection" check in the DKG doesn't fail just because we're on
        // the brink of timeout
        if (curTime - lastOutbound > 50 * 60) {
            probeConnections.emplace(dmn->proTxHash);
        }
    }

    if (!probeConnections.empty()) {
        if (LogAcceptCategory(BCLog::LLMQ)) {
            auto mnList = deterministicMNManager->GetListAtChainTip();
            std::string debugMsg = strprintf("CLLMQUtils::%s -- adding masternodes probes for quorum %s:\n", __func__, pindexQuorum->GetBlockHash().ToString());
            for (auto& c : probeConnections) {
                auto dmn = mnList.GetValidMN(c);
                if (!dmn) {
                    debugMsg += strprintf("  %s (not in valid MN set anymore)\n", c.ToString());
                } else {
                    debugMsg += strprintf("  %s (%s)\n", c.ToString(), dmn->pdmnState->addr.ToString(false));
                }
            }
            LogPrint(BCLog::NET_NETCONN, debugMsg.c_str()); /* Continued */
        }
        g_connman->AddPendingProbeConnections(probeConnections);
    }
}

bool CLLMQUtils::IsQuorumActive(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    // sig shares and recovered sigs are only accepted from recent/active quorums
    // we allow one more active quorum as specified in consensus, as otherwise there is a small window where things could
    // fail while we are on the brink of a new quorum
    auto quorums = quorumManager->ScanQuorums(llmqType, (int)params.signingActiveQuorumCount + 1);
    for (auto& q : quorums) {
        if (q->qc->quorumHash == quorumHash) {
            return true;
        }
    }
    return false;
}

bool CLLMQUtils::IsQuorumTypeEnabled(Consensus::LLMQType llmqType, const CBlockIndex* pindex)
{
    LOCK(cs_llmq_vbc);

    const Consensus::Params& consensusParams = Params().GetConsensus();
    bool f_v17_Active = VersionBitsState(pindex, consensusParams, Consensus::DEPLOYMENT_V17, llmq_versionbitscache) == ThresholdState::ACTIVE;

    switch (llmqType)
    {
        case Consensus::LLMQ_50_60:
        case Consensus::LLMQ_400_60:
        case Consensus::LLMQ_400_85:
            break;
        case Consensus::LLMQ_100_67:
        case Consensus::LLMQ_TEST_V17:
            if (!f_v17_Active) {
                return false;
            }
            break;
        case Consensus::LLMQ_TEST:
        case Consensus::LLMQ_DEVNET:
            break;
        default:
            throw std::runtime_error(strprintf("%s: Unknown LLMQ type %d", __func__, llmqType));
    }

    return true;
}

std::vector<Consensus::LLMQType> CLLMQUtils::GetEnabledQuorumTypes(const CBlockIndex* pindex)
{
    std::vector<Consensus::LLMQType> ret;
    for (const auto& p : Params().GetConsensus().llmqs) {
        if (IsQuorumTypeEnabled(p.first, pindex)) {
            ret.push_back(p.first);
        }
    }
    return ret;
}

bool CLLMQUtils::QuorumDataRecoveryEnabled()
{
    return gArgs.GetBoolArg("-llmq-data-recovery", DEFAULT_ENABLE_QUORUM_DATA_RECOVERY);
}

bool CLLMQUtils::IsWatchQuorumsEnabled()
{
    static bool fIsWatchQuroumsEnabled = gArgs.GetBoolArg("-watchquorums", DEFAULT_WATCH_QUORUMS);
    return fIsWatchQuroumsEnabled;
}

std::map<Consensus::LLMQType, QvvecSyncMode> CLLMQUtils::GetEnabledQuorumVvecSyncEntries()
{
    std::map<Consensus::LLMQType, QvvecSyncMode> mapQuorumVvecSyncEntries;
    for (const auto& strEntry : gArgs.GetArgs("-llmq-qvvec-sync")) {
        Consensus::LLMQType llmqType = Consensus::LLMQ_NONE;
        QvvecSyncMode mode{QvvecSyncMode::Invalid};
        std::istringstream ssEntry(strEntry);
        std::string strLLMQType, strMode, strTest;
        const bool fLLMQTypePresent = std::getline(ssEntry, strLLMQType, ':') && strLLMQType != "";
        const bool fModePresent = std::getline(ssEntry, strMode, ':') && strMode != "";
        const bool fTooManyEntries = static_cast<bool>(std::getline(ssEntry, strTest, ':'));
        if (!fLLMQTypePresent || !fModePresent || fTooManyEntries) {
            throw std::invalid_argument(strprintf("Invalid format in -llmq-qvvec-sync: %s", strEntry));
        }
        for (const auto& p : Params().GetConsensus().llmqs) {
            if (p.second.name == strLLMQType) {
                llmqType = p.first;
                break;
            }
        }
        if (llmqType == Consensus::LLMQ_NONE) {
            throw std::invalid_argument(strprintf("Invalid llmqType in -llmq-qvvec-sync: %s", strEntry));
        }
        if (mapQuorumVvecSyncEntries.count(llmqType) > 0) {
            throw std::invalid_argument(strprintf("Duplicated llmqType in -llmq-qvvec-sync: %s", strEntry));
        }

        int32_t nMode;
        if (ParseInt32(strMode, &nMode)) {
            switch (nMode) {
            case (int32_t)QvvecSyncMode::Always:
                mode = QvvecSyncMode::Always;
                break;
            case (int32_t)QvvecSyncMode::OnlyIfTypeMember:
                mode = QvvecSyncMode::OnlyIfTypeMember;
                break;
            default:
                mode = QvvecSyncMode::Invalid;
                break;
            }
        }
        if (mode == QvvecSyncMode::Invalid) {
            throw std::invalid_argument(strprintf("Invalid mode in -llmq-qvvec-sync: %s", strEntry));
        }
        mapQuorumVvecSyncEntries.emplace(llmqType, mode);
    }
    return mapQuorumVvecSyncEntries;
}

const Consensus::LLMQParams& GetLLMQParams(Consensus::LLMQType llmqType)
{
    return Params().GetConsensus().llmqs.at(llmqType);
}

} // namespace llmq
