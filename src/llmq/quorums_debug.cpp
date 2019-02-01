// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums_debug.h"

#include "activemasternode.h"
#include "bls/bls_batchverifier.h"
#include "chainparams.h"
#include "net.h"
#include "net_processing.h"
#include "scheduler.h"
#include "spork.h"
#include "validation.h"

#include "evo/deterministicmns.h"
#include "quorums_utils.h"

namespace llmq
{
CDKGDebugManager* quorumDKGDebugManager;

UniValue CDKGDebugSessionStatus::ToJson(int detailLevel) const
{
    UniValue ret(UniValue::VOBJ);

    if (!Params().GetConsensus().llmqs.count((Consensus::LLMQType)llmqType) || quorumHash.IsNull()) {
        return ret;
    }

    std::vector<CDeterministicMNCPtr> dmnMembers;
    if (detailLevel == 2) {
        dmnMembers = CLLMQUtils::GetAllQuorumMembers((Consensus::LLMQType) llmqType, quorumHash);
    }

    ret.push_back(Pair("llmqType", llmqType));
    ret.push_back(Pair("quorumHash", quorumHash.ToString()));
    ret.push_back(Pair("quorumHeight", (int)quorumHeight));
    ret.push_back(Pair("phase", (int)phase));

    ret.push_back(Pair("sentContributions", sentContributions));
    ret.push_back(Pair("sentComplaint", sentComplaint));
    ret.push_back(Pair("sentJustification", sentJustification));
    ret.push_back(Pair("sentPrematureCommitment", sentPrematureCommitment));
    ret.push_back(Pair("aborted", aborted));

    struct ArrOrCount {
        int count{0};
        UniValue arr{UniValue::VARR};
    };

    ArrOrCount badMembers;
    ArrOrCount weComplain;
    ArrOrCount receivedContributions;
    ArrOrCount receivedComplaints;
    ArrOrCount receivedJustifications;
    ArrOrCount receivedPrematureCommitments;
    ArrOrCount complaintsFromMembers;

    auto add = [&](ArrOrCount& v, size_t idx, bool flag) {
        if (flag) {
            if (detailLevel == 0) {
                v.count++;
            } else if (detailLevel == 1) {
                v.arr.push_back((int)idx);
            } else if (detailLevel == 2) {
                UniValue a(UniValue::VOBJ);
                a.push_back(Pair("memberIndex", (int)idx));
                if (idx < dmnMembers.size()) {
                    a.push_back(Pair("proTxHash", dmnMembers[idx]->proTxHash.ToString()));
                }
                v.arr.push_back(a);
            }
        }
    };
    auto push = [&](ArrOrCount& v, const std::string& name) {
        if (detailLevel == 0) {
            ret.push_back(Pair(name, v.count));
        } else {
            ret.push_back(Pair(name, v.arr));
        }
    };

    for (size_t i = 0; i < members.size(); i++) {
        const auto& m = members[i];
        add(badMembers, i, m.bad);
        add(weComplain, i, m.weComplain);
        add(receivedContributions, i, m.receivedContribution);
        add(receivedComplaints, i, m.receivedComplaint);
        add(receivedJustifications, i, m.receivedJustification);
        add(receivedPrematureCommitments, i, m.receivedPrematureCommitment);
    }
    push(badMembers, "badMembers");
    push(weComplain, "weComplain");
    push(receivedContributions, "receivedContributions");
    push(receivedComplaints, "receivedComplaints");
    push(receivedJustifications, "receivedJustifications");
    push(receivedPrematureCommitments, "receivedPrematureCommitments");

    if (detailLevel == 2) {
        UniValue arr(UniValue::VARR);
        for (const auto& dmn : dmnMembers) {
            arr.push_back(dmn->proTxHash.ToString());
        }
        ret.push_back(Pair("allMembers", arr));
    }

    return ret;
}

CDKGDebugManager::CDKGDebugManager(CScheduler* _scheduler) :
    scheduler(_scheduler)
{
    if (scheduler) {
        scheduler->scheduleEvery([&]() {
            SendLocalStatus();
        }, 10);
    }
}

void CDKGDebugManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if (!sporkManager.IsSporkActive(SPORK_18_QUORUM_DEBUG_ENABLED)) {
        return;
    }

    if (strCommand == NetMsgType::QDEBUGSTATUS) {
        CDKGDebugStatus status;
        vRecv >> status;

        uint256 hash = ::SerializeHash(status);

        {
            LOCK(cs_main);
            connman.RemoveAskFor(hash);
        }

        bool ban = false;
        if (!PreVerifyDebugStatusMessage(hash, status, ban)) {
            if (ban) {
                LOCK(cs_main);
                Misbehaving(pfrom->id, 10);
                return;
            }
        }

        LOCK(cs);

        pendingIncomingStatuses.emplace(hash, std::make_pair(std::move(status), pfrom->id));

        ScheduleProcessPending();
    }
}

bool CDKGDebugManager::PreVerifyDebugStatusMessage(const uint256& hash, llmq::CDKGDebugStatus& status, bool& retBan)
{
    retBan = false;

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(status.proTxHash);
    if (!dmn) {
        retBan = true;
        return false;
    }

    {
        LOCK(cs);

        if (!seenStatuses.emplace(hash, GetTimeMillis()).second) {
            return false;
        }

        auto it = statusesForMasternodes.find(status.proTxHash);
        if (it != statusesForMasternodes.end()) {
            if (statuses[it->second].nTime >= status.nTime) {
                // we know a more recent status already
                return false;
            }
        }
    }

    // check if all present LLMQ types are valid
    for (const auto& p : status.sessions) {
        if (!Params().GetConsensus().llmqs.count((Consensus::LLMQType)p.first)) {
            retBan = true;
            return false;
        }
        const auto& params = Params().GetConsensus().llmqs.at((Consensus::LLMQType)p.first);
        if (p.second.llmqType != p.first || p.second.members.size() != (size_t)params.size) {
            retBan = true;
            return false;
        }
    }

    return true;
}

void CDKGDebugManager::ScheduleProcessPending()
{
    AssertLockHeld(cs);

    if (hasScheduledProcessPending) {
        return;
    }

    scheduler->schedule([&] {
        ProcessPending();
    }, boost::chrono::system_clock::now() + boost::chrono::milliseconds(100));
}

void CDKGDebugManager::ProcessPending()
{
    decltype(pendingIncomingStatuses) pend;

    {
        LOCK(cs);
        hasScheduledProcessPending = false;
        pend = std::move(pendingIncomingStatuses);
    }

    if (!sporkManager.IsSporkActive(SPORK_18_QUORUM_DEBUG_ENABLED)) {
        return;
    }

    CBLSInsecureBatchVerifier<NodeId, uint256> batchVerifier(true, 8);
    for (const auto& p : pend) {
        const auto& hash = p.first;
        const auto& status = p.second.first;
        auto nodeId = p.second.second;
        auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(status.proTxHash);
        if (!dmn) {
            continue;
        }
        batchVerifier.PushMessage(nodeId, hash, status.GetSignHash(), status.sig, dmn->pdmnState->pubKeyOperator);
    }

    batchVerifier.Verify();

    if (!batchVerifier.badSources.empty()) {
        LOCK(cs_main);
        for (auto& nodeId : batchVerifier.badSources) {
            Misbehaving(nodeId, 100);
        }
    }
    for (const auto& p : pend) {
        const auto& hash = p.first;
        const auto& status = p.second.first;
        auto nodeId = p.second.second;
        if (batchVerifier.badMessages.count(p.first)) {
            continue;
        }

        ProcessDebugStatusMessage(hash, status);
    }
}

// status must have a validated signature
void CDKGDebugManager::ProcessDebugStatusMessage(const uint256& hash, const llmq::CDKGDebugStatus& status)
{
    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(status.proTxHash);
    if (!dmn) {
        return;
    }

    LOCK(cs);
    auto it = statusesForMasternodes.find(status.proTxHash);
    if (it != statusesForMasternodes.end()) {
        statuses.erase(it->second);
        statusesForMasternodes.erase(it);
    }

    statuses[hash] = status;
    statusesForMasternodes[status.proTxHash] = hash;

    CInv inv(MSG_QUORUM_DEBUG_STATUS, hash);
    g_connman->RelayInv(inv, DMN_PROTO_VERSION);
}

UniValue CDKGDebugStatus::ToJson(int detailLevel) const
{
    UniValue ret(UniValue::VOBJ);

    ret.push_back(Pair("proTxHash", proTxHash.ToString()));
    ret.push_back(Pair("time", nTime));
    ret.push_back(Pair("timeStr", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nTime)));

    UniValue sessionsJson(UniValue::VOBJ);
    for (const auto& p : sessions) {
        if (!Params().GetConsensus().llmqs.count((Consensus::LLMQType)p.first)) {
            continue;
        }
        const auto& params = Params().GetConsensus().llmqs.at((Consensus::LLMQType)p.first);
        sessionsJson.push_back(Pair(params.name, p.second.ToJson(detailLevel)));
    }

    ret.push_back(Pair("session", sessionsJson));

    return ret;
}

bool CDKGDebugManager::AlreadyHave(const CInv& inv)
{
    LOCK(cs);

    if (!sporkManager.IsSporkActive(SPORK_18_QUORUM_DEBUG_ENABLED)) {
        return true;
    }

    return statuses.count(inv.hash) != 0 || seenStatuses.count(inv.hash) != 0;
}

bool CDKGDebugManager::GetDebugStatus(const uint256& hash, llmq::CDKGDebugStatus& ret)
{
    LOCK(cs);
    auto it = statuses.find(hash);
    if (it == statuses.end()) {
        return false;
    }
    ret = it->second;
    return true;
}

bool CDKGDebugManager::GetDebugStatusForMasternode(const uint256& proTxHash, llmq::CDKGDebugStatus& ret)
{
    LOCK(cs);
    auto it = statusesForMasternodes.find(proTxHash);
    if (it == statusesForMasternodes.end()) {
        return false;
    }
    ret = statuses.at(it->second);
    return true;
}

void CDKGDebugManager::GetLocalDebugStatus(llmq::CDKGDebugStatus& ret)
{
    LOCK(cs);
    ret = localStatus;
    ret.proTxHash = activeMasternodeInfo.proTxHash;
}

void CDKGDebugManager::ResetLocalSessionStatus(Consensus::LLMQType llmqType)
{
    LOCK(cs);

    auto it = localStatus.sessions.find(llmqType);
    if (it == localStatus.sessions.end()) {
        return;
    }

    localStatus.sessions.erase(it);
    localStatus.nTime = GetAdjustedTime();
}

void CDKGDebugManager::InitLocalSessionStatus(Consensus::LLMQType llmqType, const uint256& quorumHash, int quorumHeight)
{
    LOCK(cs);

    auto it = localStatus.sessions.find(llmqType);
    if (it == localStatus.sessions.end()) {
        it = localStatus.sessions.emplace((uint8_t)llmqType, CDKGDebugSessionStatus()).first;
    }

    auto& params = Params().GetConsensus().llmqs.at(llmqType);
    auto& session = it->second;
    session.llmqType = llmqType;
    session.quorumHash = quorumHash;
    session.quorumHeight = (uint32_t)quorumHeight;
    session.phase = 0;
    session.statusBitset = 0;
    session.members.clear();
    session.members.resize((size_t)params.size);
}

void CDKGDebugManager::UpdateLocalStatus(std::function<bool(CDKGDebugStatus& status)>&& func)
{
    LOCK(cs);
    if (func(localStatus)) {
        localStatus.nTime = GetAdjustedTime();
    }
}

void CDKGDebugManager::UpdateLocalSessionStatus(Consensus::LLMQType llmqType, std::function<bool(CDKGDebugSessionStatus& status)>&& func)
{
    LOCK(cs);

    auto it = localStatus.sessions.find(llmqType);
    if (it == localStatus.sessions.end()) {
        return;
    }

    if (func(it->second)) {
        localStatus.nTime = GetAdjustedTime();
    }
}

void CDKGDebugManager::UpdateLocalMemberStatus(Consensus::LLMQType llmqType, size_t memberIdx, std::function<bool(CDKGDebugMemberStatus& status)>&& func)
{
    LOCK(cs);

    auto it = localStatus.sessions.find(llmqType);
    if (it == localStatus.sessions.end()) {
        return;
    }

    if (func(it->second.members.at(memberIdx))) {
        localStatus.nTime = GetAdjustedTime();
    }
}

void CDKGDebugManager::SendLocalStatus()
{
    if (!fMasternodeMode) {
        return;
    }
    if (activeMasternodeInfo.proTxHash.IsNull()) {
        return;
    }
    if (!sporkManager.IsSporkActive(SPORK_18_QUORUM_DEBUG_ENABLED)) {
        return;
    }

    CDKGDebugStatus status;
    {
        LOCK(cs);
        status = localStatus;
    }

    int64_t nTime = status.nTime;
    status.proTxHash = activeMasternodeInfo.proTxHash;
    status.nTime = 0;
    status.sig = CBLSSignature();

    uint256 newHash = ::SerializeHash(status);
    if (newHash == lastStatusHash) {
        return;
    }
    lastStatusHash = newHash;

    status.nTime = nTime;
    status.sig = activeMasternodeInfo.blsKeyOperator->Sign(status.GetSignHash());

    ProcessDebugStatusMessage(newHash, status);
}

}
