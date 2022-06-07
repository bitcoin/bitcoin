// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/providertx.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <hash.h>
#include <script/standard.h>

maybe_error CProRegTx::IsTriviallyValid() const
{
    if (nVersion == 0 || nVersion > CProRegTx::CURRENT_VERSION) {
        return {100, "bad-protx-version"};
    }
    if (nType != 0) {
        return {100, "bad-protx-type"};
    }
    if (nMode != 0) {
        return {100, "bad-protx-mode"};
    }

    if (keyIDOwner.IsNull() || !pubKeyOperator.IsValid() || keyIDVoting.IsNull()) {
        return {10, "bad-protx-key-null"};
    }
    if (!scriptPayout.IsPayToPublicKeyHash() && !scriptPayout.IsPayToScriptHash()) {
        return {10, "bad-protx-payee"};
    }

    CTxDestination payoutDest;
    if (!ExtractDestination(scriptPayout, payoutDest)) {
        // should not happen as we checked script types before
        return {10, "bad-protx-payee-dest"};
    }
    // don't allow reuse of payout key for other keys (don't allow people to put the payee key onto an online server)
    if (payoutDest == CTxDestination(keyIDOwner) || payoutDest == CTxDestination(keyIDVoting)) {
        return {10, "bad-protx-payee-reuse"};
    }

    if (nOperatorReward > 10000) {
        return {10, "bad-protx-operator-reward"};
    }

    return {};
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
    s += EncodeDestination(keyIDOwner) + "|";
    s += EncodeDestination(keyIDVoting) + "|";

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

    return strprintf("CProRegTx(nVersion=%d, collateralOutpoint=%s, addr=%s, nOperatorReward=%f, ownerAddress=%s, pubKeyOperator=%s, votingAddress=%s, scriptPayout=%s)",
        nVersion, collateralOutpoint.ToStringShort(), addr.ToString(), (double)nOperatorReward / 100, EncodeDestination(keyIDOwner), pubKeyOperator.ToString(), EncodeDestination(keyIDVoting), payee);
}

maybe_error CProUpServTx::IsTriviallyValid() const
{
    if (nVersion == 0 || nVersion > CProUpServTx::CURRENT_VERSION) {
        return {100, "bad-protx-version"};
    }

    return {};
}

std::string CProUpServTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        payee = EncodeDestination(dest);
    }

    return strprintf("CProUpServTx(nVersion=%d, proTxHash=%s, addr=%s, operatorPayoutAddress=%s)",
        nVersion, proTxHash.ToString(), addr.ToString(), payee);
}

maybe_error CProUpRegTx::IsTriviallyValid() const
{
    if (nVersion == 0 || nVersion > CProUpRegTx::CURRENT_VERSION) {
        return {100, "bad-protx-version"};
    }
    if (nMode != 0) {
        return {100, "bad-protx-mode"};
    }

    if (!pubKeyOperator.IsValid() || keyIDVoting.IsNull()) {
        return {10, "bad-protx-key-null"};
    }
    if (!scriptPayout.IsPayToPublicKeyHash() && !scriptPayout.IsPayToScriptHash()) {
        return {10, "bad-protx-payee"};
    }
    return {};
}

std::string CProUpRegTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptPayout, dest)) {
        payee = EncodeDestination(dest);
    }

    return strprintf("CProUpRegTx(nVersion=%d, proTxHash=%s, pubKeyOperator=%s, votingAddress=%s, payoutAddress=%s)",
        nVersion, proTxHash.ToString(), pubKeyOperator.ToString(), EncodeDestination(keyIDVoting), payee);
}

maybe_error CProUpRevTx::IsTriviallyValid() const
{
    if (nVersion == 0 || nVersion > CProUpRevTx::CURRENT_VERSION) {
        return {100, "bad-protx-version"};
    }

    // nReason < CProUpRevTx::REASON_NOT_SPECIFIED is always `false` since
    // nReason is unsigned and CProUpRevTx::REASON_NOT_SPECIFIED == 0
    if (nReason > CProUpRevTx::REASON_LAST) {
        return {100, "bad-protx-reason"};
    }
    return {};
}

std::string CProUpRevTx::ToString() const
{
    return strprintf("CProUpRevTx(nVersion=%d, proTxHash=%s, nReason=%d)",
        nVersion, proTxHash.ToString(), nReason);
}
