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
[[nodiscard]] RPCResult CFinalCommitment::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The quorum commitment payload",
    {
        {RPCResult::Type::NUM, "version", "Quorum commitment payload version"},
        GetRpcResult("llmqType"),
        GetRpcResult("quorumHash"),
        {RPCResult::Type::NUM, "quorumIndex", "Index of the quorum"},
        {RPCResult::Type::NUM, "signersCount", "Number of signers for the quorum"},
        {RPCResult::Type::STR_HEX, "signers", "Bitset representing the aggregated signers"},
        {RPCResult::Type::NUM, "validMembersCount", "Number of valid members in the quorum"},
        {RPCResult::Type::STR_HEX, "validMembers", "Bitset of valid members"},
        {RPCResult::Type::STR_HEX, "quorumPublicKey", "BLS public key of the quorum"},
        {RPCResult::Type::STR_HEX, "quorumVvecHash", "Hash of the quorum verification vector"},
        GetRpcResult("quorumSig"),
        {RPCResult::Type::STR_HEX, "membersSig", "BLS signature from all included commitments"},
    }};
}

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
        CFinalCommitment::GetJsonHelp(/*key=*/"commitment", /*optional=*/false),
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

[[nodiscard]] RPCResult CQuorumRotationInfo::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The quorum rotation",
    {
        {RPCResult::Type::BOOL, "extraShare", "Returns true if an extra share is returned"},
        CQuorumSnapshot::GetJsonHelp(/*key=*/"quorumSnapshotAtHMinusC", /*optional=*/false),
        CQuorumSnapshot::GetJsonHelp(/*key=*/"quorumSnapshotAtHMinus2C", /*optional=*/false),
        CQuorumSnapshot::GetJsonHelp(/*key=*/"quorumSnapshotAtHMinus3C", /*optional=*/false),
        CQuorumSnapshot::GetJsonHelp(/*key=*/"quorumSnapshotAtHMinus4C", /*optional=*/true),
        CSimplifiedMNListDiff::GetJsonHelp(/*key=*/"mnListDiffTip", /*optional=*/false),
        CSimplifiedMNListDiff::GetJsonHelp(/*key=*/"mnListDiffH", /*optional=*/false),
        CSimplifiedMNListDiff::GetJsonHelp(/*key=*/"mnListDiffAtHMinusC", /*optional=*/false),
        CSimplifiedMNListDiff::GetJsonHelp(/*key=*/"mnListDiffAtHMinus2C", /*optional=*/false),
        CSimplifiedMNListDiff::GetJsonHelp(/*key=*/"mnListDiffAtHMinus3C", /*optional=*/false),
        CSimplifiedMNListDiff::GetJsonHelp(/*key=*/"mnListDiffAtHMinus4C", /*optional=*/true),
        {RPCResult::Type::ARR, "lastCommitmentPerIndex", "Most recent commitment for each quorumIndex", {
            CFinalCommitment::GetJsonHelp(/*key=*/"", /*optional=*/false),
        }},
        {RPCResult::Type::ARR, "quorumSnapshotList", "Snapshots required to reconstruct the quorums built at h' in lastCommitmentPerIndex", {
            CQuorumSnapshot::GetJsonHelp(/*key=*/"", /*optional=*/false),
        }},
        {RPCResult::Type::ARR, "mnListDiffList", "MnListDiffs required to calculate older quorums", {
            CSimplifiedMNListDiff::GetJsonHelp(/*key=*/"", /*optional=*/false),
        }},
    }};
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

[[nodiscard]] RPCResult CQuorumSnapshot::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The quorum snapshot",
    {
        {RPCResult::Type::ARR, "activeQuorumMembers", "Bitset of nodes already in quarters at the start of cycle", {
            {RPCResult::Type::BOOL, "bit", ""}
        }},
        {RPCResult::Type::NUM, "mnSkipListMode", "Mode of the skip list"},
        {RPCResult::Type::ARR, "mnSkipList", "Skiplist at height", {
            {RPCResult::Type::NUM, "height", ""}
        }},
    }};
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

[[nodiscard]] RPCResult CRecoveredSig::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The recovered signature",
    {
        GetRpcResult("llmqType"),
        GetRpcResult("quorumHash"),
        {RPCResult::Type::NUM, "id", "Signing session ID"},
        {RPCResult::Type::STR_HEX, "msgHash", "Hash of message"},
        {RPCResult::Type::STR_HEX, "sig", "BLS signature recovered"},
        {RPCResult::Type::STR_HEX, "hash", "Hash of the BLS signature recovered"},
    }};
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
