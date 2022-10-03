// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_debug.h>

#include <chainparams.h>
#include <validation.h>

#include <evo/deterministicmns.h>
#include <llmq/quorums_utils.h>
#include <util/time.h>
namespace llmq
{
CDKGDebugManager* quorumDKGDebugManager;

UniValue CDKGDebugSessionStatus::ToJson(ChainstateManager &chainman, int detailLevel) const
{
    UniValue ret(UniValue::VOBJ);

    if (!Params().GetConsensus().llmqs.count(llmqType) || quorumHash.IsNull()) {
        return ret;
    }

    std::vector<CDeterministicMNCPtr> dmnMembers;
    if (detailLevel == 2) {
        const CBlockIndex* pindex = WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(quorumHash));
        if (pindex != nullptr) {
            dmnMembers = CLLMQUtils::GetAllQuorumMembers(GetLLMQParams(llmqType), pindex);
        }
    }

    ret.pushKV("llmqType", llmqType);
    ret.pushKV("quorumHash", quorumHash.ToString());
    ret.pushKV("quorumHeight", (int)quorumHeight);
    ret.pushKV("phase", (int)phase);

    ret.pushKV("sentContributions", statusBits.sentContributions);
    ret.pushKV("sentComplaint", statusBits.sentComplaint);
    ret.pushKV("sentJustification", statusBits.sentJustification);
    ret.pushKV("sentPrematureCommitment", statusBits.sentPrematureCommitment);
    ret.pushKV("aborted", statusBits.aborted);

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
                a.pushKV("memberIndex", (int)idx);
                if (idx < dmnMembers.size()) {
                    a.pushKV("proTxHash", dmnMembers[idx]->proTxHash.ToString());
                }
                v.arr.push_back(a);
            }
        }
    };
    auto push = [&](const ArrOrCount& v, const std::string& name) {
        if (detailLevel == 0) {
            ret.pushKV(name, v.count);
        } else {
            ret.pushKV(name, v.arr);
        }
    };

    for (size_t i = 0; i < members.size(); i++) {
        const auto& m = members[i];
        add(badMembers, i, m.statusBits.bad);
        add(weComplain, i, m.statusBits.weComplain);
        add(receivedContributions, i, m.statusBits.receivedContribution);
        add(receivedComplaints, i, m.statusBits.receivedComplaint);
        add(receivedJustifications, i, m.statusBits.receivedJustification);
        add(receivedPrematureCommitments, i, m.statusBits.receivedPrematureCommitment);
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
        ret.pushKV("allMembers", arr);
    }

    return ret;
}

CDKGDebugManager::CDKGDebugManager() {

}

UniValue CDKGDebugStatus::ToJson(ChainstateManager &chainman, int detailLevel) const
{
    UniValue ret(UniValue::VOBJ);

    ret.pushKV("time", nTime);
    ret.pushKV("timeStr", DurationToDHMS(nTime));

    UniValue sessionsArrJson(UniValue::VARR);
    for (const auto& p : sessions) {
        if (!Params().GetConsensus().llmqs.count(p.first)) {
            continue;
        }
        const auto& params = Params().GetConsensus().llmqs.at(p.first);
        UniValue s(UniValue::VOBJ);
        s.pushKV("llmqType", params.name);
        s.pushKV("status", p.second.ToJson(chainman,  detailLevel));
        sessionsArrJson.push_back(s);
    }

    ret.pushKV("session", sessionsArrJson);

    return ret;
}

void CDKGDebugManager::GetLocalDebugStatus(llmq::CDKGDebugStatus& ret) const
{
    LOCK(cs);
    ret = localStatus;
}

void CDKGDebugManager::ResetLocalSessionStatus(uint8_t llmqType)
{
    LOCK(cs);

    auto it = localStatus.sessions.find(llmqType);
    if (it == localStatus.sessions.end()) {
        return;
    }

    localStatus.sessions.erase(it);
    localStatus.nTime = GetTime();
}

void CDKGDebugManager::InitLocalSessionStatus(const Consensus::LLMQParams& llmqParams, const uint256& quorumHash, uint32_t quorumHeight)
{
    LOCK(cs);

    auto it = localStatus.sessions.find(llmqParams.type);
    if (it == localStatus.sessions.end()) {
        it = localStatus.sessions.emplace(llmqParams.type, CDKGDebugSessionStatus()).first;
    }

    auto& session = it->second;
    session.llmqType = llmqParams.type;
    session.quorumHash = quorumHash;
    session.quorumHeight = quorumHeight;
    session.phase = 0;
    session.statusBitset = 0;
    session.members.clear();
    session.members.resize((size_t)llmqParams.size);
}

void CDKGDebugManager::UpdateLocalSessionStatus(uint8_t llmqType, std::function<bool(CDKGDebugSessionStatus& status)>&& func)
{
    LOCK(cs);

    auto it = localStatus.sessions.find(llmqType);
    if (it == localStatus.sessions.end()) {
        return;
    }

    if (func(it->second)) {
        localStatus.nTime = GetTime();
    }
}

void CDKGDebugManager::UpdateLocalMemberStatus(uint8_t llmqType, size_t memberIdx, std::function<bool(CDKGDebugMemberStatus& status)>&& func)
{
    LOCK(cs);

    auto it = localStatus.sessions.find(llmqType);
    if (it == localStatus.sessions.end()) {
        return;
    }

    if (func(it->second.members.at(memberIdx))) {
        localStatus.nTime = GetTime();
    }
}

} // namespace llmq
