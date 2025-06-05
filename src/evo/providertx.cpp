// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/dmn_types.h>
#include <evo/providertx.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <hash.h>
#include <script/standard.h>
#include <tinyformat.h>
#include <util/underlying.h>

bool CProRegTx::IsTriviallyValid(bool is_basic_scheme_active, TxValidationState& state) const
{
    if (nVersion == 0 || nVersion > GetMaxVersion(is_basic_scheme_active)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version");
    }
    if (nVersion < ProTxVersion::BasicBLS && nType == MnType::Evo) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-evo-version");
    }
    if (!IsValidMnType(nType)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-type");
    }
    if (nMode != 0) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-mode");
    }

    if (keyIDOwner.IsNull() || !pubKeyOperator.Get().IsValid() || keyIDVoting.IsNull()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-key-null");
    }
    if (pubKeyOperator.IsLegacy() != (nVersion == ProTxVersion::LegacyBLS)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-operator-pubkey");
    }
    if (!scriptPayout.IsPayToPublicKeyHash() && !scriptPayout.IsPayToScriptHash()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-payee");
    }
    for (const NetInfoEntry& entry : netInfo->GetEntries()) {
        if (!entry.IsTriviallyValid()) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-bad");
        }
    }

    CTxDestination payoutDest;
    if (!ExtractDestination(scriptPayout, payoutDest)) {
        // should not happen as we checked script types before
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-payee-dest");
    }
    // don't allow reuse of payout key for other keys (don't allow people to put the payee key onto an online server)
    if (payoutDest == CTxDestination(PKHash(keyIDOwner)) || payoutDest == CTxDestination(PKHash(keyIDVoting))) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-payee-reuse");
    }

    if (nOperatorReward > 10000) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-operator-reward");
    }

    return true;
}

std::string CProRegTx::MakeSignString() const
{
    std::string s;

    // We only include the important stuff in the string form...

    CTxDestination destPayout;
    std::string strPayout;
    if (ExtractDestination(scriptPayout, destPayout)) {
        strPayout = EncodeDestination(destPayout);
    } else {
        strPayout = HexStr(scriptPayout);
    }

    s += strPayout + "|";
    s += strprintf("%d", nOperatorReward) + "|";
    s += EncodeDestination(PKHash(keyIDOwner)) + "|";
    s += EncodeDestination(PKHash(keyIDVoting)) + "|";

    // ... and also the full hash of the payload as a protection against malleability and replays
    s += ::SerializeHash(*this).ToString();

    return s;
}

std::string CProRegTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptPayout, dest)) {
        payee = EncodeDestination(dest);
    }

    return strprintf("CProRegTx(nVersion=%d, nType=%d, collateralOutpoint=%s, nOperatorReward=%f, "
                     "ownerAddress=%s, pubKeyOperator=%s, votingAddress=%s, scriptPayout=%s, platformNodeID=%s, "
                     "platformP2PPort=%d, platformHTTPPort=%d)\n"
                     "  %s",
                     nVersion, ToUnderlying(nType), collateralOutpoint.ToStringShort(), (double)nOperatorReward / 100,
                     EncodeDestination(PKHash(keyIDOwner)), pubKeyOperator.ToString(),
                     EncodeDestination(PKHash(keyIDVoting)), payee, platformNodeID.ToString(), platformP2PPort,
                     platformHTTPPort, netInfo->ToString());
}

bool CProUpServTx::IsTriviallyValid(bool is_basic_scheme_active, TxValidationState& state) const
{
    if (nVersion == 0 || nVersion > GetMaxVersion(is_basic_scheme_active)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version");
    }
    if (nVersion < ProTxVersion::BasicBLS && nType == MnType::Evo) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-evo-version");
    }
    if (netInfo->IsEmpty()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-empty");
    }
    for (const NetInfoEntry& entry : netInfo->GetEntries()) {
        if (!entry.IsTriviallyValid()) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-bad");
        }
    }

    return true;
}

std::string CProUpServTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        payee = EncodeDestination(dest);
    }

    return strprintf("CProUpServTx(nVersion=%d, nType=%d, proTxHash=%s, operatorPayoutAddress=%s, "
                     "platformNodeID=%s, platformP2PPort=%d, platformHTTPPort=%d)\n"
                     "  %s",
                     nVersion, ToUnderlying(nType), proTxHash.ToString(), payee, platformNodeID.ToString(),
                     platformP2PPort, platformHTTPPort, netInfo->ToString());
}

bool CProUpRegTx::IsTriviallyValid(bool is_basic_scheme_active, TxValidationState& state) const
{
    if (nVersion == 0 || nVersion > GetMaxVersion(is_basic_scheme_active)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version");
    }
    if (nMode != 0) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-mode");
    }

    if (!pubKeyOperator.Get().IsValid() || keyIDVoting.IsNull()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-key-null");
    }
    if (pubKeyOperator.IsLegacy() != (nVersion == ProTxVersion::LegacyBLS)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-operator-pubkey");
    }
    if (!scriptPayout.IsPayToPublicKeyHash() && !scriptPayout.IsPayToScriptHash()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-payee");
    }
    return true;
}

std::string CProUpRegTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptPayout, dest)) {
        payee = EncodeDestination(dest);
    }

    return strprintf("CProUpRegTx(nVersion=%d, proTxHash=%s, pubKeyOperator=%s, votingAddress=%s, payoutAddress=%s)",
        nVersion, proTxHash.ToString(), pubKeyOperator.ToString(), EncodeDestination(PKHash(keyIDVoting)), payee);
}

bool CProUpRevTx::IsTriviallyValid(bool is_basic_scheme_active, TxValidationState& state) const
{
    if (nVersion == 0 || nVersion > GetMaxVersion(is_basic_scheme_active)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version");
    }

    // nReason < CProUpRevTx::REASON_NOT_SPECIFIED is always `false` since
    // nReason is unsigned and CProUpRevTx::REASON_NOT_SPECIFIED == 0
    if (nReason > CProUpRevTx::REASON_LAST) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-reason");
    }
    return true;
}

std::string CProUpRevTx::ToString() const
{
    return strprintf("CProUpRevTx(nVersion=%d, proTxHash=%s, nReason=%d)",
        nVersion, proTxHash.ToString(), nReason);
}
