// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/commitment.h>
#include <llmq/signing.h>
#include <llmq/snapshot.h>

#include <core_io.h>
#include <rpc/util.h>

#include <univalue.h>

#include <map>
#include <string>

namespace llmq {
[[nodiscard]] UniValue CFinalCommitment::ToJson() const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("version", nVersion);
    obj.pushKV("llmqType", ToUnderlying(llmqType));
    obj.pushKV("quorumHash", quorumHash.ToString());
    obj.pushKV("quorumIndex", quorumIndex);
    obj.pushKV("signersCount", CountSigners());
    obj.pushKV("signers", BitsVectorToHexStr(signers));
    obj.pushKV("validMembersCount", CountValidMembers());
    obj.pushKV("validMembers", BitsVectorToHexStr(validMembers));
    obj.pushKV("quorumPublicKey", quorumPublicKey.ToString(nVersion == LEGACY_BLS_NON_INDEXED_QUORUM_VERSION || nVersion == LEGACY_BLS_INDEXED_QUORUM_VERSION));
    obj.pushKV("quorumVvecHash", quorumVvecHash.ToString());
    obj.pushKV("quorumSig", quorumSig.ToString(nVersion == LEGACY_BLS_NON_INDEXED_QUORUM_VERSION || nVersion == LEGACY_BLS_INDEXED_QUORUM_VERSION));
    obj.pushKV("membersSig", membersSig.ToString(nVersion == LEGACY_BLS_NON_INDEXED_QUORUM_VERSION || nVersion == LEGACY_BLS_INDEXED_QUORUM_VERSION));
    return obj;
}

[[nodiscard]] RPCResult CFinalCommitmentTxPayload::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The quorum commitment special transaction",
    {
        GetRpcResult("version"),
        GetRpcResult("height"),
        // TODO: Add RPCResult for llmq::CFinalCommitment
    }};
}

[[nodiscard]] UniValue CFinalCommitmentTxPayload::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("height", nHeight);
    ret.pushKV("commitment", commitment.ToJson());
    return ret;
}

[[nodiscard]] UniValue CQuorumRotationInfo::ToJson() const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("extraShare", extraShare);

    obj.pushKV("quorumSnapshotAtHMinusC", quorumSnapshotAtHMinusC.ToJson());
    obj.pushKV("quorumSnapshotAtHMinus2C", quorumSnapshotAtHMinus2C.ToJson());
    obj.pushKV("quorumSnapshotAtHMinus3C", quorumSnapshotAtHMinus3C.ToJson());

    if (extraShare) {
        obj.pushKV("quorumSnapshotAtHMinus4C", quorumSnapshotAtHMinus4C.ToJson());
    }

    obj.pushKV("mnListDiffTip", mnListDiffTip.ToJson());
    obj.pushKV("mnListDiffH", mnListDiffH.ToJson());
    obj.pushKV("mnListDiffAtHMinusC", mnListDiffAtHMinusC.ToJson());
    obj.pushKV("mnListDiffAtHMinus2C", mnListDiffAtHMinus2C.ToJson());
    obj.pushKV("mnListDiffAtHMinus3C", mnListDiffAtHMinus3C.ToJson());

    if (extraShare) {
        obj.pushKV("mnListDiffAtHMinus4C", mnListDiffAtHMinus4C.ToJson());
    }
    UniValue hqclists(UniValue::VARR);
    for (const auto& qc : lastCommitmentPerIndex) {
        hqclists.push_back(qc.ToJson());
    }
    obj.pushKV("lastCommitmentPerIndex", hqclists);

    UniValue snapshotlist(UniValue::VARR);
    for (const auto& snap : quorumSnapshotList) {
        snapshotlist.push_back(snap.ToJson());
    }
    obj.pushKV("quorumSnapshotList", snapshotlist);

    UniValue mnlistdifflist(UniValue::VARR);
    for (const auto& mnlist : mnListDiffList) {
        mnlistdifflist.push_back(mnlist.ToJson());
    }
    obj.pushKV("mnListDiffList", mnlistdifflist);
    return obj;
}

[[nodiscard]] UniValue CQuorumSnapshot::ToJson() const
{
    UniValue obj(UniValue::VOBJ);
    UniValue activeQ(UniValue::VARR);
    for (const bool h : activeQuorumMembers) {
        // cppcheck-suppress useStlAlgorithm
        activeQ.push_back(h);
    }
    obj.pushKV("activeQuorumMembers", activeQ);
    obj.pushKV("mnSkipListMode", mnSkipListMode);
    UniValue skipList(UniValue::VARR);
    for (const auto& h : mnSkipList) {
        // cppcheck-suppress useStlAlgorithm
        skipList.push_back(h);
    }
    obj.pushKV("mnSkipList", skipList);
    return obj;
}

[[nodiscard]] UniValue CRecoveredSig::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("llmqType", ToUnderlying(llmqType));
    ret.pushKV("quorumHash", quorumHash.ToString());
    ret.pushKV("id", id.ToString());
    ret.pushKV("msgHash", msgHash.ToString());
    ret.pushKV("sig", sig.Get().ToString());
    ret.pushKV("hash", sig.Get().GetHash().ToString());
    return ret;
}
} // namespace llmq
