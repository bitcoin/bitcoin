// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/dmnstate.h>

#include <script/standard.h>

#include <evo/netinfo.h>
#include <rpc/evo_util.h>

#include <univalue.h>

std::string CDeterministicMNState::ToString() const
{
    CTxDestination dest;
    std::string payoutAddress = "unknown";
    std::string operatorPayoutAddress = "none";
    if (ExtractDestination(scriptPayout, dest)) {
        payoutAddress = EncodeDestination(dest);
    }
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        operatorPayoutAddress = EncodeDestination(dest);
    }

    return strprintf("CDeterministicMNState(nVersion=%d, nRegisteredHeight=%d, nLastPaidHeight=%d, nPoSePenalty=%d, "
                     "nPoSeRevivedHeight=%d, nPoSeBanHeight=%d, nRevocationReason=%d, "
                     "ownerAddress=%s, pubKeyOperator=%s, votingAddress=%s, netInfo=%s, payoutAddress=%s, "
                     "operatorPayoutAddress=%s)\n",
                     nVersion, nRegisteredHeight, nLastPaidHeight, nPoSePenalty, nPoSeRevivedHeight, nPoSeBanHeight,
                     nRevocationReason, EncodeDestination(PKHash(keyIDOwner)), pubKeyOperator.ToString(),
                     EncodeDestination(PKHash(keyIDVoting)), netInfo->ToString(), payoutAddress, operatorPayoutAddress);
}

UniValue CDeterministicMNStateDiff::ToJson(MnType nType) const
{
    UniValue obj(UniValue::VOBJ);
    if (fields & Field_nVersion) {
        obj.pushKV("version", state.nVersion);
    }
    if (fields & Field_netInfo) {
        obj.pushKV("service", state.netInfo->GetPrimary().ToStringAddrPort());
    }
    if (fields & Field_nRegisteredHeight) {
        obj.pushKV("registeredHeight", state.nRegisteredHeight);
    }
    if (fields & Field_nLastPaidHeight) {
        obj.pushKV("lastPaidHeight", state.nLastPaidHeight);
    }
    if (fields & Field_nConsecutivePayments) {
        obj.pushKV("consecutivePayments", state.nConsecutivePayments);
    }
    if (fields & Field_nPoSePenalty) {
        obj.pushKV("PoSePenalty", state.nPoSePenalty);
    }
    if (fields & Field_nPoSeRevivedHeight) {
        obj.pushKV("PoSeRevivedHeight", state.nPoSeRevivedHeight);
    }
    if (fields & Field_nPoSeBanHeight) {
        obj.pushKV("PoSeBanHeight", state.nPoSeBanHeight);
    }
    if (fields & Field_nRevocationReason) {
        obj.pushKV("revocationReason", state.nRevocationReason);
    }
    if (fields & Field_keyIDOwner) {
        obj.pushKV("ownerAddress", EncodeDestination(PKHash(state.keyIDOwner)));
    }
    if (fields & Field_keyIDVoting) {
        obj.pushKV("votingAddress", EncodeDestination(PKHash(state.keyIDVoting)));
    }
    if (fields & Field_scriptPayout) {
        CTxDestination dest;
        if (ExtractDestination(state.scriptPayout, dest)) {
            obj.pushKV("payoutAddress", EncodeDestination(dest));
        }
    }
    if (fields & Field_scriptOperatorPayout) {
        CTxDestination dest;
        if (ExtractDestination(state.scriptOperatorPayout, dest)) {
            obj.pushKV("operatorPayoutAddress", EncodeDestination(dest));
        }
    }
    if (fields & Field_pubKeyOperator) {
        obj.pushKV("pubKeyOperator", state.pubKeyOperator.ToString());
    }
    if (nType == MnType::Evo) {
        if (fields & Field_platformNodeID) {
            obj.pushKV("platformNodeID", state.platformNodeID.ToString());
        }
        // v23.1.x adaptation: upstream gates these deprecated fields behind
        // IsServiceDeprecatedRPCEnabled(), but on this branch they deliberately
        // remain unenforced through gating (see bbcd9d543e6) and were always
        // returned in v23.1.7. Keep `if (true)` so the block structure stays
        // aligned with develop for future backports.
        if (true) {
            // platformP2PPort/platformHTTPPort are deprecated scalar duplicates of netInfo's
            // Platform entries. From ExtAddr onwards the scalar fields are unused (always 0), so
            // when the diff carries an ExtAddr netInfo report the live port from it to stay
            // consistent with the "addresses" output below.
            const bool has_ext_netinfo = (fields & Field_netInfo) && state.netInfo->CanStorePlatform();
            if (fields & Field_platformP2PPort) {
                obj.pushKV("platformP2PPort",
                           has_ext_netinfo && state.netInfo->HasEntries(NetInfoPurpose::PLATFORM_P2P)
                               ? state.netInfo->GetEntries(NetInfoPurpose::PLATFORM_P2P)[0].GetPort()
                               : state.platformP2PPort);
            }
            if (fields & Field_platformHTTPPort) {
                obj.pushKV("platformHTTPPort",
                           has_ext_netinfo && state.netInfo->HasEntries(NetInfoPurpose::PLATFORM_HTTPS)
                               ? state.netInfo->GetEntries(NetInfoPurpose::PLATFORM_HTTPS)[0].GetPort()
                               : state.platformHTTPPort);
            }
        }
    }
    {
        const bool has_netinfo = (fields & Field_netInfo);

        UniValue netInfoObj(UniValue::VOBJ);
        if (has_netinfo) {
            netInfoObj = state.netInfo->ToJson();
        }
        if (nType == MnType::Evo && (!has_netinfo || !state.netInfo->CanStorePlatform())) {
            auto unknownAddr = [](uint16_t port) -> UniValue {
                UniValue obj(UniValue::VARR);
                // We don't know what the address is because it wasn't changed in the
                // diff but we still need to report the port number in addr:port format
                obj.push_back(strprintf("255.255.255.255:%d", port));
                return obj;
            };
            if (fields & Field_platformP2PPort) {
                netInfoObj.pushKV(PurposeToString(NetInfoPurpose::PLATFORM_P2P).data(),
                                  (has_netinfo)
                                      ? ArrFromService(CService(state.netInfo->GetPrimary(), state.platformP2PPort))
                                      : unknownAddr(state.platformP2PPort));
            }
            if (fields & Field_platformHTTPPort) {
                netInfoObj.pushKV(PurposeToString(NetInfoPurpose::PLATFORM_HTTPS).data(),
                                  (has_netinfo)
                                      ? ArrFromService(CService(state.netInfo->GetPrimary(), state.platformHTTPPort))
                                      : unknownAddr(state.platformHTTPPort));
            }
        }
        if (!netInfoObj.empty()) {
            obj.pushKV("addresses", netInfoObj);
        }
    }
    return obj;
}
