// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <util/underlying.h>

#include <evo/assetlocktx.h>
#include <evo/cbtx.h>
#include <evo/mnhftx.h>
#include <evo/netinfo.h>
#include <evo/providertx.h>
#include <evo/simplifiedmns.h>
#include <evo/smldiff.h>
#include <llmq/commitment.h>

#include <univalue.h>

[[nodiscard]] UniValue CAssetLockPayload::ToJson() const
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

[[nodiscard]] UniValue CAssetUnlockPayload::ToJson() const
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

[[nodiscard]] UniValue CCbTx::ToJson() const
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

[[nodiscard]] UniValue CProRegTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("type", ToUnderlying(nType));
    ret.pushKV("collateralHash", collateralOutpoint.hash.ToString());
    ret.pushKV("collateralIndex", collateralOutpoint.n);
    if (IsServiceDeprecatedRPCEnabled()) {
        ret.pushKV("service", netInfo->GetPrimary().ToStringAddrPort());
    }
    ret.pushKV("addresses", netInfo->ToJson());
    ret.pushKV("ownerAddress", EncodeDestination(PKHash(keyIDOwner)));
    ret.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    if (CTxDestination dest; ExtractDestination(scriptPayout, dest)) {
        ret.pushKV("payoutAddress", EncodeDestination(dest));
    }
    ret.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    ret.pushKV("operatorReward", (double)nOperatorReward / 100);
    if (nType == MnType::Evo) {
        ret.pushKV("platformNodeID", platformNodeID.ToString());
        ret.pushKV("platformP2PPort", platformP2PPort);
        ret.pushKV("platformHTTPPort", platformHTTPPort);
    }
    ret.pushKV("inputsHash", inputsHash.ToString());
    return ret;
}

[[nodiscard]] UniValue CProUpRegTx::ToJson() const
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

[[nodiscard]] UniValue CProUpRevTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("proTxHash", proTxHash.ToString());
    ret.pushKV("reason", nReason);
    ret.pushKV("inputsHash", inputsHash.ToString());
    return ret;
}

[[nodiscard]] UniValue CProUpServTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("type", ToUnderlying(nType));
    ret.pushKV("proTxHash", proTxHash.ToString());
    if (IsServiceDeprecatedRPCEnabled()) {
        ret.pushKV("service", netInfo->GetPrimary().ToStringAddrPort());
    }
    ret.pushKV("addresses", netInfo->ToJson());
    if (CTxDestination dest; ExtractDestination(scriptOperatorPayout, dest)) {
        ret.pushKV("operatorPayoutAddress", EncodeDestination(dest));
    }
    if (nType == MnType::Evo) {
        ret.pushKV("platformNodeID", platformNodeID.ToString());
        ret.pushKV("platformP2PPort", platformP2PPort);
        ret.pushKV("platformHTTPPort", platformHTTPPort);
    }
    ret.pushKV("inputsHash", inputsHash.ToString());
    return ret;
}

[[nodiscard]] UniValue MNHFTxPayload::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("signal", signal.ToJson());
    return ret;
}

[[nodiscard]] UniValue llmq::CFinalCommitmentTxPayload::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("height", nHeight);
    ret.pushKV("commitment", commitment.ToJson());
    return ret;
}

[[nodiscard]] UniValue CSimplifiedMNListEntry::ToJson(bool extended) const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("nVersion", nVersion);
    obj.pushKV("nType", ToUnderlying(nType));
    obj.pushKV("proRegTxHash", proRegTxHash.ToString());
    obj.pushKV("confirmedHash", confirmedHash.ToString());
    if (IsServiceDeprecatedRPCEnabled()) {
        obj.pushKV("service", netInfo->GetPrimary().ToStringAddrPort());
    }
    obj.pushKV("addresses", netInfo->ToJson());
    obj.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    obj.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    obj.pushKV("isValid", isValid);
    if (nType == MnType::Evo) {
        obj.pushKV("platformHTTPPort", platformHTTPPort);
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

[[nodiscard]] UniValue CSimplifiedMNListDiff::ToJson(bool extended) const
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
