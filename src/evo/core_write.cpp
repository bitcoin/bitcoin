// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/assetlocktx.h>
#include <evo/cbtx.h>
#include <evo/dmn_types.h>
#include <evo/dmnstate.h>
#include <evo/mnhftx.h>
#include <evo/netinfo.h>
#include <evo/providertx.h>
#include <evo/simplifiedmns.h>
#include <evo/smldiff.h>
#include <llmq/commitment.h>
#include <util/std23.h>

#include <core_io.h>
#include <key_io.h>
#include <netaddress.h>
#include <script/standard.h>
#include <util/check.h>

#include <univalue.h>

#include <cstdint>
#include <string>
#include <type_traits>

namespace {
template <typename Obj>
std::string GetDeprecatedServiceField(const Obj& obj)
{
    return CHECK_NONFATAL(obj.netInfo)->GetPrimary().ToStringAddrPort();
}
} // anonymous namespace

template <typename Obj>
UniValue GetNetInfoWithLegacyFields(const Obj& obj, const MnType& type)
{
    UniValue ret{CHECK_NONFATAL(obj.netInfo)->ToJson()};

    // Nothing to do here if not EvoNode, netInfo is empty or netInfo is capable of natively
    // storing Platform fields
    if (type != MnType::Evo || obj.netInfo->IsEmpty() || obj.netInfo->CanStorePlatform()) return ret;

    CNetAddr addr{obj.netInfo->GetPrimary()};
    ret.pushKV(PurposeToString(NetInfoPurpose::PLATFORM_HTTPS).data(),
               ArrFromService(CService(addr, obj.platformHTTPPort)));

    if constexpr (!std::is_same_v<std::decay_t<Obj>, CSimplifiedMNListEntry>) {
        // CSimplifiedMNListEntry doesn't store platformP2PPort
        ret.pushKV(PurposeToString(NetInfoPurpose::PLATFORM_P2P).data(),
                   ArrFromService(CService(addr, obj.platformP2PPort)));
    }

    return ret;
}
template UniValue GetNetInfoWithLegacyFields(const CDeterministicMNState& obj, const MnType& type);

template <bool is_p2p, typename Obj>
int32_t GetPlatformPort(const Obj& obj)
{
    // Until DEPLOYMENT_IPV4_OPTIONAL is active, consensus requires the first PLATFORM_{HTTPS,P2P} entries to
    // use the primary addr for CORE_P2P, as platform{HTTP,P2P}Port assume (i.e. all three are hosted with the
    // same addr). Once it is (always on testnet and devnets), the ports reported here may belong to another addr.
    //
    bool is_legacy{!(CHECK_NONFATAL(obj.netInfo)->CanStorePlatform())};
    if constexpr (is_p2p) {
        static_assert(!std::is_same_v<std::decay_t<Obj>, CSimplifiedMNListEntry>,
                      "CSimplifiedMNListEntry doesn't have platformP2PPort");
        if (is_legacy) {
            return obj.platformP2PPort;
        }
        if (obj.netInfo->IsEmpty()) {
            return -1; // Blank entry, nothing to report
        }
        CHECK_NONFATAL(obj.netInfo->HasEntries(NetInfoPurpose::PLATFORM_P2P));
        return obj.netInfo->GetEntries(NetInfoPurpose::PLATFORM_P2P)[0].GetPort();
    } else {
        if (is_legacy) {
            return obj.platformHTTPPort;
        }
        if (obj.netInfo->IsEmpty()) {
            return -1; // Blank entry, nothing to report
        }
        CHECK_NONFATAL(obj.netInfo->HasEntries(NetInfoPurpose::PLATFORM_HTTPS));
        return obj.netInfo->GetEntries(NetInfoPurpose::PLATFORM_HTTPS)[0].GetPort();
    }
}
template int32_t GetPlatformPort<true>(const CDeterministicMNState& obj);
template int32_t GetPlatformPort<false>(const CDeterministicMNState& obj);

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
        if (nVersion >= 2) {
            if (const PlatformDestination dest = PlatformDestinationFromScript(credit_output.scriptPubKey);
                IsValidPlatformDestination(dest)) {
                out.pushKV("address", EncodePlatformDestination(dest));
            }
        }
        outputs.push_back(out);
    }

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("creditOutputs", outputs);
    return ret;
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

UniValue CCbTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", std23::to_underlying(nVersion));
    ret.pushKV("height", nHeight);
    ret.pushKV("merkleRootMNList", merkleRootMNList.ToString());
    if (nVersion >= CCbTx::Version::MERKLE_ROOT_QUORUMS) {
        ret.pushKV("merkleRootQuorums", merkleRootQuorums.ToString());
        if (nVersion >= CCbTx::Version::CLSIG_AND_BALANCE) {
            ret.pushKV("bestCLHeightDiff", bestCLHeightDiff);
            ret.pushKV("bestCLSignature", bestCLSignature.ToString());
            ret.pushKV("creditPoolBalance", ValueFromAmount(creditPoolBalance));
            if (nVersion >= CCbTx::Version::MERKLE_ROOT_ASSETUNLOCKS) {
                ret.pushKV("merkleRootAssetUnlocks", merkleRootAssetUnlocks.ToString());
            }
        }
    }
    return ret;
}

UniValue CDeterministicMNState::ToJson(MnType nType) const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("version", nVersion);
    if (IsServiceDeprecatedRPCEnabled()) {
        obj.pushKV("service", GetDeprecatedServiceField(*this));
    }
    obj.pushKV("addresses", GetNetInfoWithLegacyFields(*this, nType));
    obj.pushKV("registeredHeight", nRegisteredHeight);
    obj.pushKV("lastPaidHeight", nLastPaidHeight);
    obj.pushKV("consecutivePayments", nConsecutivePayments);
    obj.pushKV("PoSePenalty", nPoSePenalty);
    obj.pushKV("PoSeRevivedHeight", nPoSeRevivedHeight);
    obj.pushKV("PoSeBanHeight", nPoSeBanHeight);
    obj.pushKV("revocationReason", nRevocationReason);
    if (!IsShared()) {
        obj.pushKV("ownerAddress", EncodeDestination(PKHash(keyIDOwner)));
    }
    obj.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    if (nType == MnType::Evo) {
        obj.pushKV("platformNodeID", platformNodeID.ToString());
        if (IsServiceDeprecatedRPCEnabled()) {
            obj.pushKV("platformP2PPort", GetPlatformPort</*is_p2p=*/true>(*this));
            obj.pushKV("platformHTTPPort", GetPlatformPort</*is_p2p=*/false>(*this));
        }
    }

    CTxDestination dest;
    if (IsShared()) {
        obj.pushKV("shares", ShareListToJson(shares));
        obj.pushKV("earlyPeriodBlocks", static_cast<int64_t>(nEarlyPeriodBlocks));
        obj.pushKV("earlyPenalty", nEarlyPenalty);
    } else if (nVersion >= ProTxVersion::ExtAddr) {
        obj.pushKV("payouts", PayoutListToJson(payouts));
    } else if (ExtractDestination(scriptPayout, dest)) {
        obj.pushKV("payoutAddress", EncodeDestination(dest));
    }
    obj.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        obj.pushKV("operatorPayoutAddress", EncodeDestination(dest));
    }
    return obj;
}

UniValue CProRegTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("type", std23::to_underlying(nType));
    ret.pushKV("collateralHash", collateralOutpoint.hash.ToString());
    ret.pushKV("collateralIndex", collateralOutpoint.n);
    if (IsServiceDeprecatedRPCEnabled()) {
        ret.pushKV("service", GetDeprecatedServiceField(*this));
    }
    ret.pushKV("addresses", GetNetInfoWithLegacyFields(*this, nType));
    if (!IsShared()) {
        ret.pushKV("ownerAddress", EncodeDestination(PKHash(keyIDOwner)));
    }
    ret.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    if (IsShared()) {
        ret.pushKV("shares", ShareListToJson(shares));
        ret.pushKV("earlyPeriodBlocks", static_cast<int64_t>(nEarlyPeriodBlocks));
        ret.pushKV("earlyPenalty", nEarlyPenalty);
    } else if (nVersion >= ProTxVersion::ExtAddr) {
        ret.pushKV("payouts", PayoutListToJson(payouts));
    } else if (CTxDestination dest; ExtractDestination(scriptPayout, dest)) {
        ret.pushKV("payoutAddress", EncodeDestination(dest));
    }
    ret.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    ret.pushKV("operatorReward", static_cast<double>(nOperatorReward) / 100);
    if (nType == MnType::Evo) {
        ret.pushKV("platformNodeID", platformNodeID.ToString());
        if (IsServiceDeprecatedRPCEnabled()) {
            ret.pushKV("platformP2PPort", GetPlatformPort</*is_p2p=*/true>(*this));
            ret.pushKV("platformHTTPPort", GetPlatformPort</*is_p2p=*/false>(*this));
        }
    }
    ret.pushKV("inputsHash", inputsHash.ToString());
    return ret;
}

UniValue CProUpRegTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("proTxHash", proTxHash.ToString());
    ret.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    if (nVersion >= ProTxVersion::ExtAddr) {
        ret.pushKV("payouts", PayoutListToJson(payouts));
    } else if (CTxDestination dest; ExtractDestination(scriptPayout, dest)) {
        ret.pushKV("payoutAddress", EncodeDestination(dest));
    }
    ret.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    ret.pushKV("inputsHash", inputsHash.ToString());
    return ret;
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

UniValue CProDisTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("proTxHash", proTxHash.ToString());
    ret.pushKV("actorIndex", actorIndex);
    ret.pushKV("sigCount", static_cast<uint64_t>(vchSigs.size()));
    return ret;
}

UniValue CProUpShareTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("proTxHash", proTxHash.ToString());
    ret.pushKV("shareIndex", shareIndex);
    if (CTxDestination dest; ExtractDestination(scriptReward, dest)) {
        ret.pushKV("rewardAddress", EncodeDestination(dest));
    }
    ret.pushKV("inputsHash", inputsHash.ToString());
    return ret;
}

UniValue CProUpSharedRegTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("proTxHash", proTxHash.ToString());
    ret.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    ret.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    ret.pushKV("inputsHash", inputsHash.ToString());
    ret.pushKV("sigCount", static_cast<uint64_t>(vchSigs.size()));
    return ret;
}

UniValue CProUpServTx::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("type", std23::to_underlying(nType));
    ret.pushKV("proTxHash", proTxHash.ToString());
    if (IsServiceDeprecatedRPCEnabled()) {
        ret.pushKV("service", GetDeprecatedServiceField(*this));
    }
    ret.pushKV("addresses", GetNetInfoWithLegacyFields(*this, nType));
    if (CTxDestination dest; ExtractDestination(scriptOperatorPayout, dest)) {
        ret.pushKV("operatorPayoutAddress", EncodeDestination(dest));
    }
    if (nType == MnType::Evo) {
        ret.pushKV("platformNodeID", platformNodeID.ToString());
        if (IsServiceDeprecatedRPCEnabled()) {
            ret.pushKV("platformP2PPort", GetPlatformPort</*is_p2p=*/true>(*this));
            ret.pushKV("platformHTTPPort", GetPlatformPort</*is_p2p=*/false>(*this));
        }
    }
    ret.pushKV("inputsHash", inputsHash.ToString());
    return ret;
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

UniValue CSimplifiedMNListEntry::ToJson(bool extended) const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("nVersion", nVersion);
    obj.pushKV("nType", std23::to_underlying(nType));
    obj.pushKV("proRegTxHash", proRegTxHash.ToString());
    obj.pushKV("confirmedHash", confirmedHash.ToString());
    if (IsServiceDeprecatedRPCEnabled()) {
        obj.pushKV("service", GetDeprecatedServiceField(*this));
    }
    obj.pushKV("addresses", GetNetInfoWithLegacyFields(*this, nType));
    obj.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    obj.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    obj.pushKV("isValid", isValid);
    if (nType == MnType::Evo) {
        if (IsServiceDeprecatedRPCEnabled()) {
            obj.pushKV("platformHTTPPort", GetPlatformPort</*is_p2p=*/false>(*this));
        }
        obj.pushKV("platformNodeID", platformNodeID.ToString());
    }

    if (extended) {
        CTxDestination dest;
        if (nVersion >= ProTxVersion::ExtAddr) {
            obj.pushKV("payouts", PayoutListToJson(payouts));
        } else if (ExtractDestination(scriptPayout, dest)) {
            obj.pushKV("payoutAddress", EncodeDestination(dest));
        }
        if (!shares.empty()) {
            obj.pushKV("shares", ShareListToJson(shares));
        }
        if (ExtractDestination(scriptOperatorPayout, dest)) {
            obj.pushKV("operatorPayoutAddress", EncodeDestination(dest));
        }
    }
    return obj;
}

UniValue MNHFTx::ToJson() const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("versionBit", versionBit);
    obj.pushKV("quorumHash", quorumHash.ToString());
    obj.pushKV("sig", sig.ToString());
    return obj;
}

UniValue MNHFTxPayload::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("version", nVersion);
    ret.pushKV("signal", signal.ToJson());
    return ret;
}
