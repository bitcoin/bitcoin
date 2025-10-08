// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/assetlocktx.h>
#include <evo/cbtx.h>
#include <evo/deterministicmns.h>
#include <evo/dmnstate.h>
#include <evo/mnhftx.h>
#include <evo/netinfo.h>
#include <evo/providertx.h>
#include <evo/simplifiedmns.h>
#include <evo/smldiff.h>
#include <llmq/commitment.h>
#include <rpc/evo_util.h>

#include <core_io.h>
#include <rpc/util.h>
#include <util/check.h>
#include <util/underlying.h>

#include <univalue.h>

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
    RESULT_MAP_ENTRY("collateralHash", RPCResult::Type::STR_HEX, "Collateral transaction hash"),
    RESULT_MAP_ENTRY("collateralIndex", RPCResult::Type::NUM, "Collateral transaction output index"),
    RESULT_MAP_ENTRY("consecutivePayments", RPCResult::Type::NUM, "Consecutive payments masternode has received in payment cycle"),
    RESULT_MAP_ENTRY("height", RPCResult::Type::NUM, "Block height"),
    RESULT_MAP_ENTRY("inputsHash", RPCResult::Type::STR_HEX, "Hash of all the outpoints of the transaction inputs"),
    RESULT_MAP_ENTRY("lastPaidHeight", RPCResult::Type::NUM, "Height masternode was last paid"),
    RESULT_MAP_ENTRY("llmqType", RPCResult::Type::NUM, "Quorum type"),
    RESULT_MAP_ENTRY("merkleRootMNList", RPCResult::Type::STR_HEX, "Merkle root of the masternode list"),
    RESULT_MAP_ENTRY("merkleRootQuorums", RPCResult::Type::STR_HEX, "Merkle root of the quorum list"),
    RESULT_MAP_ENTRY("operatorPayoutAddress", RPCResult::Type::STR, "Dash address used for operator reward payments"),
    RESULT_MAP_ENTRY("operatorReward", RPCResult::Type::NUM, "Fraction in %% of reward shared with the operator between 0 and 10000"),
    RESULT_MAP_ENTRY("ownerAddress", RPCResult::Type::STR, "Dash address used for payee updates and proposal voting"),
    RESULT_MAP_ENTRY("payoutAddress", RPCResult::Type::STR, "Dash address used for masternode reward payments"),
    RESULT_MAP_ENTRY("platformHTTPPort", RPCResult::Type::NUM, "(DEPRECATED) TCP port of Platform HTTP API"),
    RESULT_MAP_ENTRY("platformNodeID", RPCResult::Type::STR_HEX, "Node ID derived from P2P public key for Platform P2P"),
    RESULT_MAP_ENTRY("platformP2PPort", RPCResult::Type::NUM, "(DEPRECATED) TCP port of Platform P2P"),
    RESULT_MAP_ENTRY("PoSeBanHeight", RPCResult::Type::NUM, "Height masternode was banned for Proof of Service violations"),
    RESULT_MAP_ENTRY("PoSePenalty", RPCResult::Type::NUM, "Proof of Service penalty score"),
    RESULT_MAP_ENTRY("PoSeRevivedHeight", RPCResult::Type::NUM, "Height masternode recovered from Proof of Service violations"),
    RESULT_MAP_ENTRY("proTxHash", RPCResult::Type::STR_HEX, "Hash of the masternode's initial ProRegTx"),
    RESULT_MAP_ENTRY("pubKeyOperator", RPCResult::Type::STR, "BLS public key used for operator signing"),
    RESULT_MAP_ENTRY("quorumHash", RPCResult::Type::STR_HEX, "Hash of the quorum"),
    RESULT_MAP_ENTRY("quorumSig", RPCResult::Type::STR_HEX, "BLS recovered threshold signature of quorum"),
    RESULT_MAP_ENTRY("registeredHeight", RPCResult::Type::NUM, "Height masternode was registered"),
    RESULT_MAP_ENTRY("revocationReason", RPCResult::Type::NUM, "Reason for ProUpRegTx revocation"),
    RESULT_MAP_ENTRY("service", RPCResult::Type::STR, "(DEPRECATED) IP address and port of the masternode"),
    RESULT_MAP_ENTRY("type", RPCResult::Type::NUM, "Masternode type"),
    RESULT_MAP_ENTRY("version", RPCResult::Type::NUM, "Special transaction version"),
    RESULT_MAP_ENTRY("votingAddress", RPCResult::Type::STR, "Dash address used for voting"),
}};
#undef RESULT_MAP_ENTRY
} // anonymous namespace

RPCResult GetRpcResult(const std::string& key, bool optional)
{
    if (const auto it = RPCRESULT_MAP.find(key); it != RPCRESULT_MAP.end()) {
        const auto& ret{it->second};
        return RPCResult{ret.m_type, ret.m_key_name, optional, ret.m_description, ret.m_inner};
    }
    throw NonFatalCheckError(strprintf("Requested invalid RPCResult for nonexistent key \"%s\"", key).c_str(),
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
        }}}}}}
    }};
}

UniValue CAssetLockPayload::ToJson() const
{
    UniValue outputs(UniValue::VARR);
    for (const CTxOut& credit_output : creditOutputs) {
        UniValue out(UniValue::VOBJ);
        out.pushKV("value", ValueFromAmount(credit_output.nValue));
        out.pushKV("valueSat", credit_output.nValue);
        UniValue spk(UniValue::VOBJ);
        ScriptToUniv(credit_output.scriptPubKey, spk, /*include_hex=*/true, /*include_address=*/false);
        out.pushKV("scriptPubKey", spk);
        outputs.push_back(out);
    }

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("creditOutputs", outputs);
    return ret;
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

UniValue CAssetUnlockPayload::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("index", index);
    ret.pushKV("fee", fee);
    ret.pushKV("requestedHeight", requestedHeight);
    ret.pushKV("quorumHash", quorumHash.ToString());
    ret.pushKV("quorumSig", quorumSig.ToString());
    return ret;
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
    }};
}

UniValue CCbTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", ToUnderlying(nVersion));
    ret.pushKV("height", nHeight);
    ret.pushKV("merkleRootMNList", merkleRootMNList.ToString());
    if (nVersion >= CCbTx::Version::MERKLE_ROOT_QUORUMS) {
        ret.pushKV("merkleRootQuorums", merkleRootQuorums.ToString());
        if (nVersion >= CCbTx::Version::CLSIG_AND_BALANCE) {
            ret.pushKV("bestCLHeightDiff", bestCLHeightDiff);
            ret.pushKV("bestCLSignature", bestCLSignature.ToString());
            ret.pushKV("creditPoolBalance", ValueFromAmount(creditPoolBalance));
        }
    }
    return ret;
}

// CDeterministicMN::ToJson() defined in evo/deterministicmns.cpp
RPCResult CDeterministicMN::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode's details",
    {
        {RPCResult::Type::STR, "type", "Masternode type"},
        GetRpcResult("proTxHash"),
        GetRpcResult("collateralHash"),
        GetRpcResult("collateralIndex"),
        {RPCResult::Type::STR, "collateralAddress", /*optional=*/true, "Dash address used for collateral"},
        GetRpcResult("operatorReward"),
        CDeterministicMNState::GetJsonHelp(/*key=*/"state", /*optional=*/false),
    }};
}

RPCResult CDeterministicMNState::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode state",
    {
        {RPCResult::Type::NUM, "version", "Version of the masternode state"},
        GetRpcResult("service"),
        GetRpcResult("addresses"),
        GetRpcResult("registeredHeight"),
        GetRpcResult("lastPaidHeight"),
        GetRpcResult("consecutivePayments"),
        GetRpcResult("PoSePenalty"),
        GetRpcResult("PoSeRevivedHeight"),
        GetRpcResult("PoSeBanHeight"),
        GetRpcResult("revocationReason"),
        GetRpcResult("ownerAddress"),
        GetRpcResult("votingAddress"),
        GetRpcResult("platformNodeID", /*optional=*/true),
        GetRpcResult("platformP2PPort", /*optional=*/true),
        GetRpcResult("platformHTTPPort", /*optional=*/true),
        GetRpcResult("payoutAddress", /*optional=*/true),
        GetRpcResult("pubKeyOperator"),
        GetRpcResult("operatorPayoutAddress", /*optional=*/true),
    }};
}

UniValue CDeterministicMNState::ToJson(MnType nType) const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("version", nVersion);
    obj.pushKV("service", netInfo->GetPrimary().ToStringAddrPort());
    obj.pushKV("addresses", GetNetInfoWithLegacyFields(*this, nType));
    obj.pushKV("registeredHeight", nRegisteredHeight);
    obj.pushKV("lastPaidHeight", nLastPaidHeight);
    obj.pushKV("consecutivePayments", nConsecutivePayments);
    obj.pushKV("PoSePenalty", nPoSePenalty);
    obj.pushKV("PoSeRevivedHeight", nPoSeRevivedHeight);
    obj.pushKV("PoSeBanHeight", nPoSeBanHeight);
    obj.pushKV("revocationReason", nRevocationReason);
    obj.pushKV("ownerAddress", EncodeDestination(PKHash(keyIDOwner)));
    obj.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    if (nType == MnType::Evo) {
        obj.pushKV("platformNodeID", platformNodeID.ToString());
        obj.pushKV("platformP2PPort", GetPlatformPort</*is_p2p=*/true>(*this));
        obj.pushKV("platformHTTPPort", GetPlatformPort</*is_p2p=*/false>(*this));
    }

    CTxDestination dest;
    if (ExtractDestination(scriptPayout, dest)) {
        obj.pushKV("payoutAddress", EncodeDestination(dest));
    }
    obj.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        obj.pushKV("operatorPayoutAddress", EncodeDestination(dest));
    }
    return obj;
}

// CDeterministicMNStateDiff::ToJson() defined in evo/dmnstate.cpp
RPCResult CDeterministicMNStateDiff::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode state diff",
    {
        {RPCResult::Type::NUM, "version", "Version of the masternode state diff"},
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
        GetRpcResult("service"),
        GetRpcResult("addresses"),
        GetRpcResult("ownerAddress"),
        GetRpcResult("votingAddress"),
        GetRpcResult("payoutAddress", /*optional=*/true),
        GetRpcResult("pubKeyOperator"),
        GetRpcResult("operatorReward"),
        GetRpcResult("platformNodeID", /*optional=*/true),
        GetRpcResult("platformP2PPort", /*optional=*/true),
        GetRpcResult("platformHTTPPort", /*optional=*/true),
        GetRpcResult("inputsHash"),
    }};
}

UniValue CProRegTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("type", ToUnderlying(nType));
    ret.pushKV("collateralHash", collateralOutpoint.hash.ToString());
    ret.pushKV("collateralIndex", collateralOutpoint.n);
    ret.pushKV("service", netInfo->GetPrimary().ToStringAddrPort());
    ret.pushKV("addresses", GetNetInfoWithLegacyFields(*this, nType));
    ret.pushKV("ownerAddress", EncodeDestination(PKHash(keyIDOwner)));
    ret.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    if (CTxDestination dest; ExtractDestination(scriptPayout, dest)) {
        ret.pushKV("payoutAddress", EncodeDestination(dest));
    }
    ret.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    ret.pushKV("operatorReward", (double)nOperatorReward / 100);
    if (nType == MnType::Evo) {
        ret.pushKV("platformNodeID", platformNodeID.ToString());
        ret.pushKV("platformP2PPort", GetPlatformPort</*is_p2p=*/true>(*this));
        ret.pushKV("platformHTTPPort", GetPlatformPort</*is_p2p=*/false>(*this));
    }
    ret.pushKV("inputsHash", inputsHash.ToString());
    return ret;
}

RPCResult CProUpRegTx::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode update registrar special transaction",
    {
        GetRpcResult("version"),
        GetRpcResult("proTxHash"),
        GetRpcResult("votingAddress"),
        GetRpcResult("payoutAddress", /*optional=*/true),
        GetRpcResult("pubKeyOperator"),
        GetRpcResult("inputsHash"),
    }};
}

UniValue CProUpRegTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("proTxHash", proTxHash.ToString());
    ret.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    if (CTxDestination dest; ExtractDestination(scriptPayout, dest)) {
        ret.pushKV("payoutAddress", EncodeDestination(dest));
    }
    ret.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    ret.pushKV("inputsHash", inputsHash.ToString());
    return ret;
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

UniValue CProUpRevTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("proTxHash", proTxHash.ToString());
    ret.pushKV("reason", nReason);
    ret.pushKV("inputsHash", inputsHash.ToString());
    return ret;
}

RPCResult CProUpServTx::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode update service special transaction",
    {
        GetRpcResult("version"),
        GetRpcResult("type"),
        GetRpcResult("proTxHash"),
        GetRpcResult("service"),
        GetRpcResult("addresses"),
        GetRpcResult("operatorPayoutAddress", /*optional=*/true),
        GetRpcResult("platformNodeID", /*optional=*/true),
        GetRpcResult("platformP2PPort", /*optional=*/true),
        GetRpcResult("platformHTTPPort", /*optional=*/true),
        GetRpcResult("inputsHash"),
    }};
}

UniValue CProUpServTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("type", ToUnderlying(nType));
    ret.pushKV("proTxHash", proTxHash.ToString());
    ret.pushKV("service", netInfo->GetPrimary().ToStringAddrPort());
    ret.pushKV("addresses", GetNetInfoWithLegacyFields(*this, nType));
    if (CTxDestination dest; ExtractDestination(scriptOperatorPayout, dest)) {
        ret.pushKV("operatorPayoutAddress", EncodeDestination(dest));
    }
    if (nType == MnType::Evo) {
        ret.pushKV("platformNodeID", platformNodeID.ToString());
        ret.pushKV("platformP2PPort", GetPlatformPort</*is_p2p=*/true>(*this));
        ret.pushKV("platformHTTPPort", GetPlatformPort</*is_p2p=*/false>(*this));
    }
    ret.pushKV("inputsHash", inputsHash.ToString());
    return ret;
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
            {RPCResult::Type::OBJ, "", "", {
                {RPCResult::Type::ARR, "<sig_hex>", "Array of quorum indices, keyed by BLS signature", {
                    {RPCResult::Type::NUM, "", "Quorum index"}
        }}}}}},
    }};
}

UniValue CSimplifiedMNListDiff::ToJson(bool extended) const
{
    UniValue obj(UniValue::VOBJ);

    obj.pushKV("nVersion", nVersion);
    obj.pushKV("baseBlockHash", baseBlockHash.ToString());
    obj.pushKV("blockHash", blockHash.ToString());

    CDataStream ssCbTxMerkleTree(SER_NETWORK, PROTOCOL_VERSION);
    ssCbTxMerkleTree << cbTxMerkleTree;
    obj.pushKV("cbTxMerkleTree", HexStr(ssCbTxMerkleTree));

    obj.pushKV("cbTx", EncodeHexTx(CTransaction(cbTx)));

    UniValue deletedMNsArr(UniValue::VARR);
    for (const auto& h : deletedMNs) {
        deletedMNsArr.push_back(h.ToString());
    }
    obj.pushKV("deletedMNs", deletedMNsArr);

    UniValue mnListArr(UniValue::VARR);
    for (const auto& e : mnList) {
        mnListArr.push_back(e.ToJson(extended));
    }
    obj.pushKV("mnList", mnListArr);

    UniValue deletedQuorumsArr(UniValue::VARR);
    for (const auto& e : deletedQuorums) {
        UniValue eObj(UniValue::VOBJ);
        eObj.pushKV("llmqType", e.first);
        eObj.pushKV("quorumHash", e.second.ToString());
        deletedQuorumsArr.push_back(eObj);
    }
    obj.pushKV("deletedQuorums", deletedQuorumsArr);

    UniValue newQuorumsArr(UniValue::VARR);
    for (const auto& e : newQuorums) {
        newQuorumsArr.push_back(e.ToJson());
    }
    obj.pushKV("newQuorums", newQuorumsArr);

    // Do not assert special tx type here since this can be called prior to DIP0003 activation
    if (const auto opt_cbTxPayload = GetTxPayload<CCbTx>(cbTx, /*assert_type=*/false)) {
        obj.pushKV("merkleRootMNList", opt_cbTxPayload->merkleRootMNList.ToString());
        if (opt_cbTxPayload->nVersion >= CCbTx::Version::MERKLE_ROOT_QUORUMS) {
            obj.pushKV("merkleRootQuorums", opt_cbTxPayload->merkleRootQuorums.ToString());
        }
    }

    UniValue quorumsCLSigsArr(UniValue::VARR);
    for (const auto& [signature, quorumsIndexes] : quorumsCLSigs) {
        UniValue j(UniValue::VOBJ);
        UniValue idxArr(UniValue::VARR);
        for (const auto& idx : quorumsIndexes) {
            idxArr.push_back(idx);
        }
        j.pushKV(signature.ToString(), idxArr);
        quorumsCLSigsArr.push_back(j);
    }
    obj.pushKV("quorumsCLSigs", quorumsCLSigsArr);
    return obj;
}

RPCResult CSimplifiedMNListEntry::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The simplified masternode list entry",
    {
        {RPCResult::Type::NUM, "nVersion", "Version of the entry"},
        {RPCResult::Type::NUM, "nType", "Masternode type"},
        {RPCResult::Type::STR_HEX, "proRegTxHash", "Hash of the ProRegTx identifying the masternode"},
        {RPCResult::Type::STR_HEX, "confirmedHash", "Hash of the block where the masternode was confirmed"},
        GetRpcResult("service"),
        GetRpcResult("addresses"),
        GetRpcResult("pubKeyOperator"),
        GetRpcResult("votingAddress"),
        {RPCResult::Type::BOOL, "isValid", "Returns true if the masternode is not Proof-of-Service banned"},
        GetRpcResult("platformHTTPPort", /*optional=*/true),
        GetRpcResult("platformNodeID", /*optional=*/true),
        GetRpcResult("payoutAddress", /*optional=*/true),
        GetRpcResult("operatorPayoutAddress", /*optional=*/true),
    }};
}

UniValue CSimplifiedMNListEntry::ToJson(bool extended) const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("nVersion", nVersion);
    obj.pushKV("nType", ToUnderlying(nType));
    obj.pushKV("proRegTxHash", proRegTxHash.ToString());
    obj.pushKV("confirmedHash", confirmedHash.ToString());
    obj.pushKV("service", netInfo->GetPrimary().ToStringAddrPort());
    obj.pushKV("addresses", GetNetInfoWithLegacyFields(*this, nType));
    obj.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    obj.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    obj.pushKV("isValid", isValid);
    if (nType == MnType::Evo) {
        obj.pushKV("platformHTTPPort", GetPlatformPort</*is_p2p=*/false>(*this));
        obj.pushKV("platformNodeID", platformNodeID.ToString());
    }

    if (extended) {
        CTxDestination dest;
        if (ExtractDestination(scriptPayout, dest)) {
            obj.pushKV("payoutAddress", EncodeDestination(dest));
        }
        if (ExtractDestination(scriptOperatorPayout, dest)) {
            obj.pushKV("operatorPayoutAddress", EncodeDestination(dest));
        }
    }
    return obj;
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

UniValue MNHFTx::ToJson() const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("versionBit", versionBit);
    obj.pushKV("quorumHash", quorumHash.ToString());
    obj.pushKV("sig", sig.ToString());
    return obj;
}

RPCResult MNHFTxPayload::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "The masternode hard fork signal special transaction",
    {
        GetRpcResult("version"),
        MNHFTx::GetJsonHelp(/*key=*/"signal", /*optional=*/false),
    }};
}

UniValue MNHFTxPayload::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("signal", signal.ToJson());
    return ret;
}
