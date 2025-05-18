// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <evo/assetlocktx.h>
#include <evo/cbtx.h>
#include <evo/mnhftx.h>
#include <evo/netinfo.h>
#include <evo/providertx.h>
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
    ret.pushKV("version", int(nVersion));
    ret.pushKV("creditOutputs", outputs);
    return ret;
}

[[nodiscard]] UniValue CAssetUnlockPayload::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", int(nVersion));
    ret.pushKV("index", int(index));
    ret.pushKV("fee", int(fee));
    ret.pushKV("requestedHeight", int(requestedHeight));
    ret.pushKV("quorumHash", quorumHash.ToString());
    ret.pushKV("quorumSig", quorumSig.ToString());
    return ret;
}

[[nodiscard]] UniValue CCbTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", (int)nVersion);
    ret.pushKV("height", nHeight);
    ret.pushKV("merkleRootMNList", merkleRootMNList.ToString());
    if (nVersion >= CCbTx::Version::MERKLE_ROOT_QUORUMS) {
        ret.pushKV("merkleRootQuorums", merkleRootQuorums.ToString());
        if (nVersion >= CCbTx::Version::CLSIG_AND_BALANCE) {
            ret.pushKV("bestCLHeightDiff", static_cast<int>(bestCLHeightDiff));
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
    ret.pushKV("collateralIndex", (int)collateralOutpoint.n);
    ret.pushKV("service", netInfo->GetPrimary().ToStringAddrPort());
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
    ret.pushKV("reason", (int)nReason);
    ret.pushKV("inputsHash", inputsHash.ToString());
    return ret;
}

[[nodiscard]] UniValue CProUpServTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("type", ToUnderlying(nType));
    ret.pushKV("proTxHash", proTxHash.ToString());
    ret.pushKV("service", netInfo->GetPrimary().ToStringAddrPort());
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
    ret.pushKV("version", (int)nVersion);
    ret.pushKV("signal", signal.ToJson());
    return ret;
}

[[nodiscard]] UniValue llmq::CFinalCommitmentTxPayload::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", int{nVersion});
    ret.pushKV("height", int(nHeight));
    ret.pushKV("commitment", commitment.ToJson());
    return ret;
}
