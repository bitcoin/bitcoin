// Copyright (c) 2018-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/dmnstate.h>
#include <evo/providertx.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <validationinterface.h>

#include <univalue.h>
#include <messagesigner.h>

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

    return strprintf("CDeterministicMNState(nVersion=%d, nRegisteredHeight=%d, nLastPaidHeight=%d, nPoSePenalty=%d, nPoSeRevivedHeight=%d, nPoSeBanHeight=%d, nRevocationReason=%d, "
                     "ownerAddress=%s, pubKeyOperator=%s, votingAddress=%s, addr=%s, payoutAddress=%s, operatorPayoutAddress=%s)",
                     nVersion, nRegisteredHeight, nLastPaidHeight, nPoSePenalty, nPoSeRevivedHeight, nPoSeBanHeight, nRevocationReason,
                     EncodeDestination(WitnessV0KeyHash(keyIDOwner)), pubKeyOperator.ToString(), EncodeDestination(WitnessV0KeyHash(keyIDVoting)), addr.ToStringAddrPort(), payoutAddress, operatorPayoutAddress);
}

void CDeterministicMNState::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.pushKV("version", nVersion);
    obj.pushKV("service", addr.ToStringAddrPort());
    obj.pushKV("registeredHeight", nRegisteredHeight);
    obj.pushKV("lastPaidHeight", nLastPaidHeight);
    obj.pushKV("collateralHeight", nCollateralHeight);
    obj.pushKV("PoSePenalty", nPoSePenalty);
    obj.pushKV("PoSeRevivedHeight", nPoSeRevivedHeight);
    obj.pushKV("PoSeBanHeight", nPoSeBanHeight);
    obj.pushKV("revocationReason", nRevocationReason);
    obj.pushKV("ownerAddress", EncodeDestination(WitnessV0KeyHash(keyIDOwner)));
    obj.pushKV("votingAddress", EncodeDestination(WitnessV0KeyHash(keyIDVoting)));

    CTxDestination dest;
    if (ExtractDestination(scriptPayout, dest)) {
        obj.pushKV("payoutAddress", EncodeDestination(dest));
    }
    obj.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        obj.pushKV("operatorPayoutAddress", EncodeDestination(dest));
    }
}

UniValue CDeterministicMNStateDiff::ToJson() const
{
    UniValue obj;
    obj.setObject();
    if (fields & Field_nVersion) {
        obj.pushKV("version", state.nVersion);
    }
    if (fields & Field_addr) {
        obj.pushKV("service", state.addr.ToStringAddrPort());
    }
    if (fields & Field_nRegisteredHeight) {
        obj.pushKV("registeredHeight", state.nRegisteredHeight);
    }
    if (fields & Field_nLastPaidHeight) {
        obj.pushKV("lastPaidHeight", state.nLastPaidHeight);
    }
    if (fields & Field_nCollateralHeight) {
        obj.pushKV("collateralHight", state.nCollateralHeight);
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
        obj.pushKV("ownerAddress", EncodeDestination(WitnessV0KeyHash(state.keyIDOwner)));
    }
    if (fields & Field_keyIDVoting) {
        obj.pushKV("votingAddress", EncodeDestination(WitnessV0KeyHash(state.keyIDVoting)));
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
    return obj;
}
