// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums_debug.h"

#include "activemasternode.h"
#include "chainparams.h"
#include "net.h"
#include "net_processing.h"
#include "scheduler.h"
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
                a.push_back(Pair("memberIndex", idx));
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

CDKGDebugManager::CDKGDebugManager(CScheduler* scheduler)
{
    for (const auto& p : Params().GetConsensus().llmqs) {
        ResetLocalSessionStatus(p.first, uint256(), 0);
    }

    if (scheduler) {
        scheduler->scheduleEvery([&]() {
            SendLocalStatus();
        }, 10);
    }
}

void CDKGDebugManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if (strCommand == NetMsgType::QDEBUGSTATUS) {
        CDKGDebugStatus status;
        vRecv >> status;

        {
            LOCK(cs_main);
            connman.RemoveAskFor(::SerializeHash(status));
        }

        ProcessDebugStatusMessage(pfrom->id, status);
    }
}

void CDKGDebugManager::ProcessDebugStatusMessage(NodeId nodeId, llmq::CDKGDebugStatus& status)
{
    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(status.proTxHash);
    if (!dmn) {
        if (nodeId != -1) {
            LOCK(cs_main);
            Misbehaving(nodeId, 10);
        }
        return;
    }

    {
        LOCK(cs);
        auto it = statusesForMasternodes.find(status.proTxHash);
        if (it != statusesForMasternodes.end()) {
            if (statuses[it->second].nTime >= status.nTime) {
                // we know a more recent status already
                return;
            }
        }
    }

    // check if all expected LLMQ types are present and valid
    std::set<Consensus::LLMQType> llmqTypes;
    for (const auto& p : status.sessions) {
        if (!Params().GetConsensus().llmqs.count((Consensus::LLMQType)p.first)) {
            if (nodeId != -1) {
                LOCK(cs_main);
                Misbehaving(nodeId, 10);
            }
            return;
        }
        const auto& params = Params().GetConsensus().llmqs.at((Consensus::LLMQType)p.first);
        if (p.second.llmqType != p.first || p.second.members.size() != (size_t)params.size) {
            if (nodeId != -1) {
                LOCK(cs_main);
                Misbehaving(nodeId, 10);
            }
            return;
        }
        llmqTypes.emplace((Consensus::LLMQType)p.first);
    }
    for (const auto& p : Params().GetConsensus().llmqs) {
        if (!llmqTypes.count(p.first)) {
            if (nodeId != -1) {
                LOCK(cs_main);
                Misbehaving(nodeId, 10);
            }
            return;
        }
    }

    // TODO batch verification/processing
    if (!status.sig.VerifyInsecure(dmn->pdmnState->pubKeyOperator, status.GetSignHash())) {
        if (nodeId != -1) {
            LOCK(cs_main);
            Misbehaving(nodeId, 10);
        }
        return;
    }

    LOCK(cs);
    auto it = statusesForMasternodes.find(status.proTxHash);
    if (it != statusesForMasternodes.end()) {
        statuses.erase(it->second);
        statusesForMasternodes.erase(it);
    }

    auto hash = ::SerializeHash(status);

    statuses[hash] = status;
    statusesForMasternodes[status.proTxHash] = hash;

    CInv inv(MSG_QUORUM_DEBUG_STATUS, hash);
    g_connman->RelayInv(inv, DMN_PROTO_VERSION);
}

UniValue CDKGDebugStatus::ToJson(int detailLevel) const
{
    UniValue ret(UniValue::VOBJ);

    ret.push_back(Pair("proTxHash", proTxHash.ToString()));
    ret.push_back(Pair("height", (int)nHeight));
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
    return statuses.count(inv.hash) != 0;
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

void CDKGDebugManager::ResetLocalSessionStatus(Consensus::LLMQType llmqType, const uint256& quorumHash, int quorumHeight)
{
    LOCK(cs);

    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    localStatus.nTime = GetAdjustedTime();

    auto& session = localStatus.sessions[llmqType];

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
    if (func(localStatus.sessions.at(llmqType))) {
        localStatus.nTime = GetAdjustedTime();
    }
}

void CDKGDebugManager::UpdateLocalMemberStatus(Consensus::LLMQType llmqType, size_t memberIdx, std::function<bool(CDKGDebugMemberStatus& status)>&& func)
{
    LOCK(cs);
    if (func(localStatus.sessions.at(llmqType).members.at(memberIdx))) {
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

    ProcessDebugStatusMessage(-1, status);
}

}
