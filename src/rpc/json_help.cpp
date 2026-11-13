// Copyright (c) 2025-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/json_help.h>

#include <evo/assetlocktx.h>
#include <evo/cbtx.h>
#include <evo/deterministicmns.h>
#include <evo/dmnstate.h>
#include <evo/mnhftx.h>
#include <evo/providertx.h>
#include <evo/simplifiedmns.h>
#include <evo/smldiff.h>
#include <governance/common.h>
#include <governance/governance.h>
#include <governance/object.h>
#include <llmq/commitment.h>
#include <llmq/debug.h>
#include <llmq/signing.h>
#include <llmq/snapshot.h>
#include <rpc/util.h>
#include <tinyformat.h>
#include <util/check.h>

#include <map>
#include <string>

namespace {
#define RESULT_MAP_ENTRY(name, type, desc) {name, {type, name, desc}}
const std::map<std::string, RPCResult> RPCRESULT_MAP{{
    {"addresses",
        {RPCResult::Type::OBJ, "addresses", "Network addresses of the masternode",
    {
        {RPCResult::Type::ARR, "core_p2p", /*optional=*/true, "Addresses used for protocol P2P",
            {{RPCResult::Type::STR, "address", ""}}},
        {RPCResult::Type::ARR, "platform_p2p", /*optional=*/true, "Addresses used for Platform P2P",
            {{RPCResult::Type::STR, "address", ""}}},
        {RPCResult::Type::ARR, "platform_https", /*optional=*/true, "Addresses used for Platform HTTPS API",
            {{RPCResult::Type::STR, "address", ""}}},
    }}},
    RESULT_MAP_ENTRY("collateralAddress", RPCResult::Type::STR, "Dash address used for collateral"),
    RESULT_MAP_ENTRY("collateralHash", RPCResult::Type::STR_HEX, "Collateral transaction hash"),
    RESULT_MAP_ENTRY("collateralIndex", RPCResult::Type::NUM, "Collateral transaction output index"),
    RESULT_MAP_ENTRY("consecutivePayments", RPCResult::Type::NUM, "Consecutive payments masternode has received in payment cycle"),
    RESULT_MAP_ENTRY("earlyPenalty", RPCResult::Type::NUM, "Penalty in duffs for unilateral dissolution during the early period"),
    RESULT_MAP_ENTRY("earlyPeriodBlocks", RPCResult::Type::NUM, "Length in blocks of the early period during which unilateral dissolution is penalized"),
    RESULT_MAP_ENTRY("height", RPCResult::Type::NUM, "Block height"),
    RESULT_MAP_ENTRY("inputsHash", RPCResult::Type::STR_HEX, "Hash of all the outpoints of the transaction inputs"),
    RESULT_MAP_ENTRY("lastPaidHeight", RPCResult::Type::NUM, "Height masternode was last paid"),
    RESULT_MAP_ENTRY("llmqType", RPCResult::Type::NUM, "Quorum type"),
    RESULT_MAP_ENTRY("memberIndex", RPCResult::Type::NUM, "Quorum member index"),
    RESULT_MAP_ENTRY("merkleRootMNList", RPCResult::Type::STR_HEX, "Merkle root of the masternode list"),
    RESULT_MAP_ENTRY("merkleRootQuorums", RPCResult::Type::STR_HEX, "Merkle root of the quorum list"),
    RESULT_MAP_ENTRY("operatorPayoutAddress", RPCResult::Type::STR, "Dash address used for operator reward payments"),
    RESULT_MAP_ENTRY("operatorReward", RPCResult::Type::NUM, "Fraction in % of reward shared with the operator between 0 and 100"),
    RESULT_MAP_ENTRY("outpoint", RPCResult::Type::STR_HEX,"The outpoint of the masternode"),
    RESULT_MAP_ENTRY("ownerAddress", RPCResult::Type::STR, "Dash address used for payee updates and proposal voting"),
    RESULT_MAP_ENTRY("payoutAddress", RPCResult::Type::STR, "Dash address used for masternode reward payments"),
    {"payouts",
        {RPCResult::Type::ARR, "payouts", "Owner masternode reward payout shares",
    {
        {RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR, "address", "Dash address used for this owner payout"},
            {RPCResult::Type::STR_HEX, "script", "Owner payout scriptPubKey"},
            {RPCResult::Type::NUM, "reward", "Owner payout share in basis points"},
        }},
    }}},
    RESULT_MAP_ENTRY("platformHTTPPort", RPCResult::Type::NUM, "TCP port of Platform HTTP API (DEPRECATED, returned only if config option -deprecatedrpc=service is passed)"),
    RESULT_MAP_ENTRY("platformNodeID", RPCResult::Type::STR_HEX, "Node ID derived from P2P public key for Platform P2P"),
    RESULT_MAP_ENTRY("platformP2PPort", RPCResult::Type::NUM, "TCP port of Platform P2P (DEPRECATED, returned only if config option -deprecatedrpc=service is passed)"),
    RESULT_MAP_ENTRY("PoSeBanHeight", RPCResult::Type::NUM, "Height masternode was banned for Proof of Service violations"),
    RESULT_MAP_ENTRY("PoSePenalty", RPCResult::Type::NUM, "Proof of Service penalty score"),
    RESULT_MAP_ENTRY("PoSeRevivedHeight", RPCResult::Type::NUM, "Height masternode recovered from Proof of Service violations"),
    RESULT_MAP_ENTRY("proTxHash", RPCResult::Type::STR_HEX, "Hash of the masternode's initial ProRegTx"),
    RESULT_MAP_ENTRY("pubKeyOperator", RPCResult::Type::STR, "BLS public key used for operator signing"),
    RESULT_MAP_ENTRY("quorumIndex", RPCResult::Type::NUM, "Quorum index. Relevant for rotation quorums only, 0 for non-rotating quorums"),
    RESULT_MAP_ENTRY("quorumHash", RPCResult::Type::STR_HEX, "Hash of the quorum"),
    RESULT_MAP_ENTRY("quorumSig", RPCResult::Type::STR_HEX, "BLS recovered threshold signature of quorum"),
    RESULT_MAP_ENTRY("registeredHeight", RPCResult::Type::NUM, "Height masternode was registered"),
    RESULT_MAP_ENTRY("revocationReason", RPCResult::Type::NUM, "Reason for ProUpRegTx revocation"),
    RESULT_MAP_ENTRY("service", RPCResult::Type::STR, "IP address and port of the masternode (DEPRECATED, returned only if config option -deprecatedrpc=service is passed)"),
    {"shares",
        {RPCResult::Type::ARR, "shares", "Collateral shares of a shared masternode",
    {
        {RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::NUM, "amount", "Collateral contribution in duffs"},
            {RPCResult::Type::STR, "refundAddress", "Dash address the principal is refunded to at dissolution"},
            {RPCResult::Type::STR_HEX, "refundScript", "Refund scriptPubKey"},
            {RPCResult::Type::STR, "rewardAddress", "Dash address this share's owner rewards are paid to"},
            {RPCResult::Type::STR_HEX, "rewardScript", "Reward scriptPubKey"},
            {RPCResult::Type::STR, "ownerAddress", "Dash address of the share owner key"},
        }},
    }}},
    RESULT_MAP_ENTRY("type", RPCResult::Type::NUM, "Masternode type"),
    RESULT_MAP_ENTRY("type_str", RPCResult::Type::STR, "Masternode type (human-readable string)"),
    RESULT_MAP_ENTRY("version", RPCResult::Type::NUM, "Special transaction version"),
    RESULT_MAP_ENTRY("votingAddress", RPCResult::Type::STR, "Dash address used for voting"),
}};
#undef RESULT_MAP_ENTRY
} // anonymous namespace

RPCResult GetRpcResult(const std::string& key, bool optional, const std::string& override_name)
{
    if (const auto it = RPCRESULT_MAP.find(key); it != RPCRESULT_MAP.end()) {
        const auto& ret{it->second};
        return RPCResult{ret.m_type, override_name.empty() ? ret.m_key_name : override_name, optional, ret.m_description, ret.m_inner};
    }
    throw NonFatalCheckError(strprintf("Requested invalid RPCResult for nonexistent key \"%s\"", key),
                             __FILE__, __LINE__, __func__);
}

RPCResult CAssetLockPayload::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The asset lock special transaction",
    {
        GetRpcResult("version"),
        {RPCResult::Type::ARR, "creditOutputs", "", {
            {RPCResult::Type::OBJ, "", "", {
                {RPCResult::Type::NUM, "value", "The value in Dash"},
                {RPCResult::Type::NUM, "valueSat", "The value in duffs"},
                {RPCResult::Type::OBJ, "scriptPubKey", "", {
                    {RPCResult::Type::STR, "asm", "The asm"},
                    {RPCResult::Type::STR_HEX, "hex", "The hex"},
                    {RPCResult::Type::STR, "type", "The type, eg 'pubkeyhash'"},
                }},
                {RPCResult::Type::STR, "address", /*optional=*/true, "DIP-18 Platform address (version >= 2 only)"},
        }}}}
    }};
}

RPCResult CAssetUnlockPayload::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The asset unlock special transaction",
    {
        GetRpcResult("version"),
        {RPCResult::Type::NUM, "index", "Index of the transaction"},
        {RPCResult::Type::NUM, "fee", "Transaction fee in duffs awarded to the miner"},
        {RPCResult::Type::NUM, "requestedHeight", "Payment chain block height known by Platform when signing the withdrawal"},
        GetRpcResult("quorumHash"),
        GetRpcResult("quorumSig"),
    }};
}

RPCResult CCbTx::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The coinbase special transaction",
    {
        GetRpcResult("version"),
        GetRpcResult("height"),
        GetRpcResult("merkleRootMNList"),
        GetRpcResult("merkleRootQuorums", /*optional=*/true),
        {RPCResult::Type::NUM, "bestCLHeightDiff", /*optional=*/true, "Blocks between the current block and the last known block with a ChainLock"},
        {RPCResult::Type::STR_HEX, "bestCLSignature", /*optional=*/true, "Best ChainLock signature known by the miner"},
        {RPCResult::Type::NUM, "creditPoolBalance", /*optional=*/true, "Balance in the Platform credit pool"},
        {RPCResult::Type::STR_HEX, "merkleRootAssetUnlocks", /*optional=*/true, "Merkle root of the instance hashes of the block's version 2 asset unlock transactions"},
    }};
}

// CDeterministicMN::ToJson() defined in rpc/evo.cpp
RPCResult CDeterministicMN::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode's details",
    {
        GetRpcResult("type_str", /*optional=*/false, /*override_name=*/"type"),
        GetRpcResult("proTxHash"),
        GetRpcResult("collateralHash"),
        GetRpcResult("collateralIndex"),
        GetRpcResult("collateralAddress", /*optional=*/true),
        GetRpcResult("operatorReward"),
        CDeterministicMNState::GetJsonHelp(/*key=*/"state", /*optional=*/false),
    }};
}

RPCResult CDeterministicMNState::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode state",
    {
        {RPCResult::Type::NUM, "version", "Version of the masternode state"},
        GetRpcResult("service", /*optional=*/true),
        GetRpcResult("addresses"),
        GetRpcResult("registeredHeight"),
        GetRpcResult("lastPaidHeight"),
        GetRpcResult("consecutivePayments"),
        GetRpcResult("PoSePenalty"),
        GetRpcResult("PoSeRevivedHeight"),
        GetRpcResult("PoSeBanHeight"),
        GetRpcResult("revocationReason"),
        GetRpcResult("ownerAddress", /*optional=*/true),
        GetRpcResult("votingAddress"),
        GetRpcResult("platformNodeID", /*optional=*/true),
        GetRpcResult("platformP2PPort", /*optional=*/true),
        GetRpcResult("platformHTTPPort", /*optional=*/true),
        GetRpcResult("payoutAddress", /*optional=*/true),
        GetRpcResult("payouts", /*optional=*/true),
        GetRpcResult("shares", /*optional=*/true),
        GetRpcResult("earlyPeriodBlocks", /*optional=*/true),
        GetRpcResult("earlyPenalty", /*optional=*/true),
        GetRpcResult("pubKeyOperator"),
        GetRpcResult("operatorPayoutAddress", /*optional=*/true),
    }};
}

// CDeterministicMNStateDiff::ToJson() defined in evo/dmnstate.cpp
RPCResult CDeterministicMNStateDiff::GetJsonHelp(const std::string& key, bool optional)
{
    // Every field is emitted only when the diff actually carries it, version included.
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode state diff",
    {
        {RPCResult::Type::NUM, "version", /*optional=*/true, "Version of the masternode state diff"},
        GetRpcResult("service", /*optional=*/true),
        GetRpcResult("registeredHeight", /*optional=*/true),
        GetRpcResult("lastPaidHeight", /*optional=*/true),
        GetRpcResult("consecutivePayments", /*optional=*/true),
        GetRpcResult("PoSePenalty", /*optional=*/true),
        GetRpcResult("PoSeRevivedHeight", /*optional=*/true),
        GetRpcResult("PoSeBanHeight", /*optional=*/true),
        GetRpcResult("revocationReason", /*optional=*/true),
        GetRpcResult("ownerAddress", /*optional=*/true),
        GetRpcResult("votingAddress", /*optional=*/true),
        GetRpcResult("payoutAddress", /*optional=*/true),
        GetRpcResult("payouts", /*optional=*/true),
        GetRpcResult("shares", /*optional=*/true),
        GetRpcResult("earlyPeriodBlocks", /*optional=*/true),
        GetRpcResult("earlyPenalty", /*optional=*/true),
        GetRpcResult("operatorPayoutAddress", /*optional=*/true),
        GetRpcResult("pubKeyOperator", /*optional=*/true),
        GetRpcResult("platformNodeID", /*optional=*/true),
        GetRpcResult("platformP2PPort", /*optional=*/true),
        GetRpcResult("platformHTTPPort", /*optional=*/true),
        GetRpcResult("addresses", /*optional=*/true),
    }};
}

RPCResult CProRegTx::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode registration special transaction",
    {
        GetRpcResult("version"),
        GetRpcResult("type"),
        GetRpcResult("collateralHash"),
        GetRpcResult("collateralIndex"),
        GetRpcResult("service", /*optional=*/true),
        GetRpcResult("addresses"),
        GetRpcResult("ownerAddress", /*optional=*/true),
        GetRpcResult("votingAddress"),
        GetRpcResult("payoutAddress", /*optional=*/true),
        GetRpcResult("payouts", /*optional=*/true),
        GetRpcResult("shares", /*optional=*/true),
        GetRpcResult("earlyPeriodBlocks", /*optional=*/true),
        GetRpcResult("earlyPenalty", /*optional=*/true),
        GetRpcResult("pubKeyOperator"),
        GetRpcResult("operatorReward"),
        GetRpcResult("platformNodeID", /*optional=*/true),
        GetRpcResult("platformP2PPort", /*optional=*/true),
        GetRpcResult("platformHTTPPort", /*optional=*/true),
        GetRpcResult("inputsHash"),
    }};
}

RPCResult CProUpRegTx::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode update registrar special transaction",
    {
        GetRpcResult("version"),
        GetRpcResult("proTxHash"),
        GetRpcResult("votingAddress"),
        GetRpcResult("payoutAddress", /*optional=*/true),
        GetRpcResult("payouts", /*optional=*/true),
        GetRpcResult("pubKeyOperator"),
        GetRpcResult("inputsHash"),
    }};
}

RPCResult CProUpRevTx::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode operator revocation special transaction",
    {
        GetRpcResult("version"),
        GetRpcResult("proTxHash"),
        {RPCResult::Type::NUM, "reason", "Reason for masternode service revocation"},
        GetRpcResult("inputsHash", /*optional=*/true),
    }};
}

RPCResult CProDisTx::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The shared masternode dissolution special transaction",
    {
        GetRpcResult("version"),
        GetRpcResult("proTxHash"),
        {RPCResult::Type::NUM, "actorIndex", "Index into the share table of the participant paying the penalty (if any) and the fee"},
        {RPCResult::Type::NUM, "sigCount", "Number of signatures: 1 = unilateral, one per share = unanimous"},
    }};
}

RPCResult CProUpShareTx::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The shared masternode share update special transaction",
    {
        GetRpcResult("version"),
        GetRpcResult("proTxHash"),
        {RPCResult::Type::NUM, "shareIndex", "Index into the share table of the share being updated"},
        {RPCResult::Type::STR, "rewardAddress", /*optional=*/true, "New Dash address for this share's owner rewards (omitted only when decoding a payload whose reward script is not P2PKH/P2SH)"},
        GetRpcResult("inputsHash"),
    }};
}

RPCResult CProUpSharedRegTx::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The shared masternode registrar update special transaction",
    {
        GetRpcResult("version"),
        GetRpcResult("proTxHash"),
        GetRpcResult("votingAddress"),
        GetRpcResult("pubKeyOperator"),
        GetRpcResult("inputsHash"),
        {RPCResult::Type::NUM, "sigCount", "Number of signatures; must equal the share count"},
    }};
}

RPCResult CProUpServTx::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode update service special transaction",
    {
        GetRpcResult("version"),
        GetRpcResult("type"),
        GetRpcResult("proTxHash"),
        GetRpcResult("service", /*optional=*/true),
        GetRpcResult("addresses"),
        GetRpcResult("operatorPayoutAddress", /*optional=*/true),
        GetRpcResult("platformNodeID", /*optional=*/true),
        GetRpcResult("platformP2PPort", /*optional=*/true),
        GetRpcResult("platformHTTPPort", /*optional=*/true),
        GetRpcResult("inputsHash"),
    }};
}

RPCResult CSimplifiedMNListDiff::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The simplified masternode list diff",
    {
        {RPCResult::Type::NUM, "nVersion", "Version of the diff"},
        {RPCResult::Type::STR_HEX, "baseBlockHash", "Hash of the base block"},
        {RPCResult::Type::STR_HEX, "blockHash", "Hash of the ending block"},
        {RPCResult::Type::STR_HEX, "cbTxMerkleTree", "Coinbase transaction merkle tree"},
        {RPCResult::Type::STR_HEX, "cbTx", "Coinbase raw transaction"},
        {RPCResult::Type::ARR, "deletedMNs", "ProRegTx hashes of deleted masternodes",
            {{RPCResult::Type::STR_HEX, "hash", ""}}},
        {RPCResult::Type::ARR, "mnList", "Masternode list details",
            {CSimplifiedMNListEntry::GetJsonHelp(/*key=*/"", /*optional=*/false)}},
        {RPCResult::Type::ARR, "deletedQuorums", "Deleted quorums",
            {{RPCResult::Type::OBJ, "", "", {
                GetRpcResult("llmqType"),
                GetRpcResult("quorumHash"),
        }}}},
        {RPCResult::Type::ARR, "newQuorums", "New quorums",
            {llmq::CFinalCommitment::GetJsonHelp(/*key=*/"", /*optional=*/false)}},
        GetRpcResult("merkleRootMNList", /*optional=*/true),
        GetRpcResult("merkleRootQuorums", /*optional=*/true),
        {RPCResult::Type::ARR, "quorumsCLSigs", "ChainLock signature details", {
            {RPCResult::Type::OBJ_DYN, "", "json object with the BLS signature as key", {
                {RPCResult::Type::ARR, "<sig_hex>", "Array of quorum indices signed by this BLS signature", {
                    {RPCResult::Type::NUM, "", "Quorum index"}
        }}}}}},
    }};
}

RPCResult CSimplifiedMNListEntry::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The simplified masternode list entry",
    {
        {RPCResult::Type::NUM, "nVersion", "Version of the entry"},
        GetRpcResult("type", /*optional=*/false, /*override_name=*/"nType"),
        {RPCResult::Type::STR_HEX, "proRegTxHash", "Hash of the ProRegTx identifying the masternode"},
        {RPCResult::Type::STR_HEX, "confirmedHash", "Hash of the block where the masternode was confirmed"},
        GetRpcResult("service", /*optional=*/true),
        GetRpcResult("addresses"),
        GetRpcResult("pubKeyOperator"),
        GetRpcResult("votingAddress"),
        {RPCResult::Type::BOOL, "isValid", "Returns true if the masternode is not Proof-of-Service banned"},
        GetRpcResult("platformHTTPPort", /*optional=*/true),
        GetRpcResult("platformNodeID", /*optional=*/true),
        GetRpcResult("payoutAddress", /*optional=*/true),
        GetRpcResult("payouts", /*optional=*/true),
        GetRpcResult("shares", /*optional=*/true),
        GetRpcResult("operatorPayoutAddress", /*optional=*/true),
    }};
}

RPCResult MNHFTx::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode hard fork payload",
    {
        {RPCResult::Type::NUM, "versionBit", "Version bit associated with the hard fork"},
        GetRpcResult("quorumHash"),
        {RPCResult::Type::STR_HEX, "sig", "BLS signature by a quorum public key"},
    }};
}

RPCResult MNHFTxPayload::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode hard fork signal special transaction",
    {
        GetRpcResult("version"),
        MNHFTx::GetJsonHelp(/*key=*/"signal", /*optional=*/false),
    }};
}

namespace llmq {
// CDKGDebugSessionStatus::ToJson() defined in llmq/debug.cpp
RPCResult CDKGDebugSessionStatus::GetJsonHelp(const std::string& key, bool optional)
{
    // A member tally is a count for detail_level = 0, an array of member indexes for
    // detail_level = 1, and an array of {memberIndex, proTxHash} objects for
    // detail_level = 2. RPCResult can only express such a union as ANY: conditional
    // variants are only resolved for top-level results, so listing one variant per
    // detail level here would make every level fail the runtime doc check.
    const auto member_tally{[](const std::string& name, const std::string& what) {
        return RPCResult{RPCResult::Type::ANY, name,
                         strprintf("Number of %s (detail_level = 0), array of quorum member indexes "
                                   "(detail_level = 1), or array of {memberIndex, proTxHash} objects "
                                   "(detail_level = 2)",
                                   what)};
    }};
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The state of a DKG session. Empty for an unknown LLMQ type or quorum hash",
    {
        GetRpcResult("llmqType", /*optional=*/true),
        GetRpcResult("quorumHash", /*optional=*/true),
        {RPCResult::Type::NUM, "quorumHeight", /*optional=*/true, "Block height of the quorum"},
        {RPCResult::Type::NUM, "phase", /*optional=*/true, "Latest DKG phase whose messages this node has already sent"},
        {RPCResult::Type::BOOL, "sentContributions", /*optional=*/true, "Returns true if contributions sent"},
        {RPCResult::Type::BOOL, "sentComplaint", /*optional=*/true, "Returns true if complaints sent"},
        {RPCResult::Type::BOOL, "sentJustification", /*optional=*/true, "Returns true if justifications sent"},
        {RPCResult::Type::BOOL, "sentPrematureCommitment", /*optional=*/true, "Returns true if premature commitments sent"},
        {RPCResult::Type::BOOL, "aborted", /*optional=*/true, "Returns true if DKG session aborted"},
        member_tally("badMembers", "bad members"),
        member_tally("weComplain", "complaints sent"),
        member_tally("receivedContributions", "contributions received"),
        member_tally("receivedComplaints", "complaints received"),
        member_tally("receivedJustifications", "justifications received"),
        member_tally("receivedPrematureCommitments", "premature commitments received"),
        {RPCResult::Type::ARR, "allMembers", /*optional=*/true, "Provider registration transaction hash for all quorum members. Only present for detail_level = 2", {
            GetRpcResult("proTxHash")}},
    }};
}

// CDKGDebugManager::ToJson() defined in llmq/debug.cpp
RPCResult CDKGDebugManager::GetJsonHelp(const std::string& key, bool optional, bool inner_optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The state of the node's DKG sessions",
    {
        {RPCResult::Type::NUM, "time", inner_optional, "Adjusted time for the last update, timestamp"},
        {RPCResult::Type::STR, "timeStr", inner_optional, "Adjusted time for the last update, human friendly"},
        {RPCResult::Type::ARR, "session", inner_optional, "", {
            {RPCResult::Type::OBJ, "", "", {
                {RPCResult::Type::STR, "llmqType", "Name of quorum"},
                GetRpcResult("quorumIndex"),
                CDKGDebugSessionStatus::GetJsonHelp(/*key=*/"status", /*optional=*/false)
            }},
        }}
    }};
}

RPCResult CFinalCommitment::GetJsonHelp(const std::string& key, bool optional)
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

RPCResult CFinalCommitmentTxPayload::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The quorum commitment special transaction",
    {
        GetRpcResult("version"),
        GetRpcResult("height"),
        CFinalCommitment::GetJsonHelp(/*key=*/"commitment", /*optional=*/false),
    }};
}

RPCResult CQuorumRotationInfo::GetJsonHelp(const std::string& key, bool optional)
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

RPCResult CQuorumSnapshot::GetJsonHelp(const std::string& key, bool optional)
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

RPCResult CRecoveredSig::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The recovered signature",
    {
        GetRpcResult("llmqType"),
        GetRpcResult("quorumHash"),
        {RPCResult::Type::STR_HEX, "id", "Signing session ID"},
        {RPCResult::Type::STR_HEX, "msgHash", "Hash of message"},
        {RPCResult::Type::STR_HEX, "sig", "BLS signature recovered"},
        {RPCResult::Type::STR_HEX, "hash", "Hash of the BLS signature recovered"},
    }};
}
} // namespace llmq

RPCResult CGovernanceManager::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "Count of governance objects and votes",
    {
        {RPCResult::Type::NUM, "objects_total", "Total number of all governance objects"},
        {RPCResult::Type::NUM, "proposals", "Number of governance proposals"},
        {RPCResult::Type::NUM, "triggers", "Number of triggers"},
        {RPCResult::Type::NUM, "other", "Total number of unknown governance objects"},
        {RPCResult::Type::NUM, "erased", "Number of removed (expired) objects"},
        {RPCResult::Type::NUM, "votes", "Total number of votes"},
    }};
}

RPCResult CGovernanceObject::GetInnerJsonHelp(const std::string& key, bool optional)
{
    return Governance::Object::GetJsonHelp(key, optional);
}

// CGovernanceObject::GetStateJson() defined in governance/object.cpp
RPCResult CGovernanceObject::GetStateJsonHelp(const std::string& key, bool optional, const std::string& local_valid_key)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "Object state info",
    {
        {RPCResult::Type::STR_HEX, "DataHex", "Governance object (hex)"},
        {RPCResult::Type::STR, "DataString", "Governance object (string)"},
        {RPCResult::Type::STR_HEX, "Hash", "Hash of governance object"},
        GetRpcResult("collateralHash", /*optional=*/false, /*override_name=*/"CollateralHash"),
        {RPCResult::Type::NUM, "ObjectType", "Object types"},
        {RPCResult::Type::NUM, "CreationTime", "Object creation timestamp"},
        {RPCResult::Type::STR_HEX, "SigningMasternode", /*optional=*/true, "Signing masternode’s vin (for triggers only)"},
        {RPCResult::Type::BOOL, local_valid_key, "Returns true if valid"},
        {RPCResult::Type::STR, "IsValidReason", strprintf("%s error (human-readable string, empty if %s true)", local_valid_key, local_valid_key)},
        {RPCResult::Type::BOOL, "fCachedValid", "Returns true if minimum support has been reached flagging object as valid"},
        {RPCResult::Type::BOOL, "fCachedFunding", "Returns true if minimum support has been reached flagging object as fundable"},
        {RPCResult::Type::BOOL, "fCachedDelete", "Returns true if minimum support has been reached flagging object as marked for deletion"},
        {RPCResult::Type::BOOL, "fCachedEndorsed", "Returns true if minimum support has been reached flagging object as endorsed"},
    }};
}

RPCResult CGovernanceObject::GetVotesJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "Object vote counts",
    {
        {RPCResult::Type::NUM, "AbsoluteYesCount", "Number of Yes votes minus number of No votes"},
        {RPCResult::Type::NUM, "YesCount", "Number of Yes votes"},
        {RPCResult::Type::NUM, "NoCount", "Number of No votes"},
        {RPCResult::Type::NUM, "AbstainCount", "Number of Abstain votes"},
    }};
}

namespace Governance {
RPCResult Object::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "Object info",
    {
        {RPCResult::Type::STR_HEX, "objectHash", "Hash of proposal object"},
        {RPCResult::Type::STR_HEX, "parentHash", "Hash of the parent object (root node has a hash of 0)"},
        GetRpcResult("collateralHash"),
        {RPCResult::Type::NUM, "createdAt", "Proposal creation timestamp"},
        {RPCResult::Type::NUM, "revision", "Proposal revision number"},
        // Everything but "hex" is the submitter's own JSON echoed back verbatim, so neither the
        // set of keys nor their types is ours to promise: a proposal may carry extra fields and
        // may encode the numeric ones as strings. Hence the type check is skipped here.
        {RPCResult::Type::OBJ, "data", "", {
            // Fields emitted through GetDataAsPlainString(), read by CProposalValidator
            {RPCResult::Type::ANY, "end_epoch", /*optional=*/true, "Proposal end timestamp"},
            {RPCResult::Type::ANY, "name", /*optional=*/true, "Proposal name"},
            {RPCResult::Type::ANY, "payment_address", /*optional=*/true, "Proposal payment address"},
            {RPCResult::Type::ANY, "payment_amount", /*optional=*/true, "Proposal payment amount"},
            {RPCResult::Type::ANY, "start_epoch", /*optional=*/true, "Proposal start timestamp"},
            {RPCResult::Type::ANY, "type", /*optional=*/true, "Object type"},
            {RPCResult::Type::ANY, "url", /*optional=*/true, "Proposal URL"},
            // Failure case for GetDataAsPlainString()
            {RPCResult::Type::STR, "plain", /*optional=*/true, "Governance object data as string"},
            // Always emitted by ToJson()
            {RPCResult::Type::STR_HEX, "hex", "Governance object data as hex"},
        }, /*skip_type_check=*/true},
    }};
}
} // namespace Governance
