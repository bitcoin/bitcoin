// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/debug.h>

#include <chainparams.h>
#include <timedata.h>
#include <validation.h>

#include <evo/deterministicmns.h>
#include <llmq/utils.h>

namespace llmq
{
CDKGDebugManager* quorumDKGDebugManager;

UniValue CDKGDebugSessionStatus::ToJson(int quorumIndex, int detailLevel) const
{
    UniValue ret(UniValue::VOBJ);

    if (!Params().HasLLMQ(llmqType) || quorumHash.IsNull()) {
        return ret;
    }

    std::vector<CDeterministicMNCPtr> dmnMembers;
    if (detailLevel == 2) {
        const CBlockIndex* pindex = WITH_LOCK(cs_main, return LookupBlockIndex(quorumHash));
        if (pindex != nullptr) {
            dmnMembers = CLLMQUtils::GetAllQuorumMembers(llmqType, pindex);
        }
    }

    ret.pushKV("llmqType", static_cast<uint8_t>(llmqType));
    ret.pushKV("quorumHash", quorumHash.ToString());
    ret.pushKV("quorumHeight", (int)quorumHeight);
    ret.pushKV("phase", (int)phase);

    ret.pushKV("sentContributions", sentContributions);
    ret.pushKV("sentComplaint", sentComplaint);
    ret.pushKV("sentJustification", sentJustification);
    ret.pushKV("sentPrematureCommitment", sentPrematureCommitment);
    ret.pushKV("aborted", aborted);

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
        ret.pushKV("allMembers", arr);
    }

    return ret;
}

CDKGDebugManager::CDKGDebugManager() = default;

UniValue CDKGDebugStatus::ToJson(int detailLevel) const
{
    UniValue ret(UniValue::VOBJ);

    ret.pushKV("time", nTime);
    ret.pushKV("timeStr", FormatISO8601DateTime(nTime));

    // TODO Support array of sessions
    UniValue sessionsArrJson(UniValue::VARR);
    for (const auto& p : sessions) {
        if (!Params().HasLLMQ(p.first.first)) {
            continue;
        }
        UniValue s(UniValue::VOBJ);
        s.pushKV("llmqType", std::string(GetLLMQParams(p.first.first).name));
        s.pushKV("quorumIndex", p.first.second);
        s.pushKV("status", p.second.ToJson(p.first.second, detailLevel));

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

void CDKGDebugManager::ResetLocalSessionStatus(Consensus::LLMQType llmqType, int quorumIndex)
{
    LOCK(cs);

    auto it = localStatus.sessions.find(std::make_pair(llmqType, quorumIndex));
    if (it == localStatus.sessions.end()) {
        return;
    }

    localStatus.sessions.erase(it);
    localStatus.nTime = GetAdjustedTime();
}

void CDKGDebugManager::InitLocalSessionStatus(const Consensus::LLMQParams& llmqParams, int quorumIndex, const uint256& quorumHash, int quorumHeight)
{
    LOCK(cs);

    auto it = localStatus.sessions.find(std::make_pair(llmqParams.type, quorumIndex));
    if (it == localStatus.sessions.end()) {
        it = localStatus.sessions.emplace(std::make_pair(llmqParams.type, quorumIndex), CDKGDebugSessionStatus()).first;
    }

    auto& session = it->second;
    session.llmqType = llmqParams.type;
    session.quorumHash = quorumHash;
    session.quorumHeight = (uint32_t)quorumHeight;
    session.phase = 0;
    session.statusBitset = 0;
    session.members.clear();
    session.members.resize((size_t)llmqParams.size);
}

void CDKGDebugManager::UpdateLocalSessionStatus(Consensus::LLMQType llmqType, int quorumIndex, std::function<bool(CDKGDebugSessionStatus& status)>&& func)
{
    LOCK(cs);

    auto it = localStatus.sessions.find(std::make_pair(llmqType, quorumIndex));
    if (it == localStatus.sessions.end()) {
        return;
    }

    if (func(it->second)) {
        localStatus.nTime = GetAdjustedTime();
    }
}

void CDKGDebugManager::UpdateLocalMemberStatus(Consensus::LLMQType llmqType, int quorumIndex, size_t memberIdx, std::function<bool(CDKGDebugMemberStatus& status)>&& func)
{
    LOCK(cs);

    auto it = localStatus.sessions.find(std::make_pair(llmqType, quorumIndex));
    if (it == localStatus.sessions.end()) {
        return;
    }

    if (func(it->second.members.at(memberIdx))) {
        localStatus.nTime = GetAdjustedTime();
    }
}

} // namespace llmq
