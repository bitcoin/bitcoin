// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/dmn_types.h>
#include <evo/providertx.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <hash.h>
#include <script/standard.h>
#include <tinyformat.h>
#include <util/underlying.h>

namespace ProTxVersion {
template <typename T>
[[nodiscard]] uint16_t GetMaxFromDeployment(gsl::not_null<const CBlockIndex*> pindexPrev,
                                            std::optional<bool> is_basic_override)
{
    constexpr bool is_extaddr_eligible{std::is_same_v<std::decay_t<T>, CProRegTx> || std::is_same_v<std::decay_t<T>, CProUpServTx>};
    return ProTxVersion::GetMax(is_basic_override ? *is_basic_override
                                                  : DeploymentActiveAfter(pindexPrev, Params().GetConsensus(),
                                                                          Consensus::DEPLOYMENT_V19),
                                is_extaddr_eligible ? DeploymentActiveAfter(pindexPrev, Params().GetConsensus(),
                                                                            Consensus::DEPLOYMENT_V24)
                                                    : false);
}
template uint16_t GetMaxFromDeployment<CProRegTx>(gsl::not_null<const CBlockIndex*> pindexPrev, std::optional<bool> is_basic_override);
template uint16_t GetMaxFromDeployment<CProUpServTx>(gsl::not_null<const CBlockIndex*> pindexPrev, std::optional<bool> is_basic_override);
template uint16_t GetMaxFromDeployment<CProUpRegTx>(gsl::not_null<const CBlockIndex*> pindexPrev, std::optional<bool> is_basic_override);
template uint16_t GetMaxFromDeployment<CProUpRevTx>(gsl::not_null<const CBlockIndex*> pindexPrev, std::optional<bool> is_basic_override);
} // namespace ProTxVersion

template <typename ProTx>
bool IsNetInfoTriviallyValid(const ProTx& proTx, TxValidationState& state)
{
    if (!proTx.netInfo->HasEntries(NetInfoPurpose::CORE_P2P)) {
        // Mandatory for all nodes
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-empty");
    }
    if (proTx.nType == MnType::Regular) {
        // Regular nodes shouldn't populate Platform-specific fields
        if (proTx.netInfo->HasEntries(NetInfoPurpose::PLATFORM_HTTPS) ||
            proTx.netInfo->HasEntries(NetInfoPurpose::PLATFORM_P2P)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-bad");
        }
    }
    if (proTx.netInfo->CanStorePlatform() && proTx.nType == MnType::Evo) {
        // Platform fields are mandatory for EvoNodes
        if (!proTx.netInfo->HasEntries(NetInfoPurpose::PLATFORM_HTTPS) ||
            !proTx.netInfo->HasEntries(NetInfoPurpose::PLATFORM_P2P)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-empty");
        }
    }
    return true;
}

bool CProRegTx::IsTriviallyValid(gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state) const
{
    if (nVersion == 0 || nVersion > ProTxVersion::GetMaxFromDeployment<decltype(*this)>(pindexPrev)) {
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
    if (netInfo->CanStorePlatform() != (nVersion == ProTxVersion::ExtAddr)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-netinfo-version");
    }
    if (!netInfo->IsEmpty() && !IsNetInfoTriviallyValid(*this, state)) {
        // pass the state returned by the function above
        return false;
    }
    for (const auto& entry : netInfo->GetEntries()) {
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

    return strprintf("CProRegTx(nVersion=%d, nType=%d, collateralOutpoint=%s, netInfo=%s, nOperatorReward=%f, "
                     "ownerAddress=%s, pubKeyOperator=%s, votingAddress=%s, scriptPayout=%s, platformNodeID=%s%s)\n",
                     nVersion, ToUnderlying(nType), collateralOutpoint.ToStringShort(), netInfo->ToString(),
                     (double)nOperatorReward / 100, EncodeDestination(PKHash(keyIDOwner)), pubKeyOperator.ToString(),
                     EncodeDestination(PKHash(keyIDVoting)), payee, platformNodeID.ToString(),
                     (nVersion >= ProTxVersion::ExtAddr
                          ? ""
                          : strprintf(", platformP2PPort=%d, platformHTTPPort=%d", platformP2PPort, platformHTTPPort)));
}

bool CProUpServTx::IsTriviallyValid(gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state) const
{
    if (nVersion == 0 || nVersion > ProTxVersion::GetMaxFromDeployment<decltype(*this)>(pindexPrev)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version");
    }
    if (nVersion < ProTxVersion::BasicBLS && nType == MnType::Evo) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-evo-version");
    }
    if (netInfo->CanStorePlatform() != (nVersion == ProTxVersion::ExtAddr)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-netinfo-version");
    }
    if (netInfo->IsEmpty()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-empty");
    }
    if (!IsNetInfoTriviallyValid(*this, state)) {
        // pass the state returned by the function above
        return false;
    }
    for (const auto& entry : netInfo->GetEntries()) {
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

    return strprintf("CProUpServTx(nVersion=%d, nType=%d, proTxHash=%s, netInfo=%s, operatorPayoutAddress=%s, "
                     "platformNodeID=%s%s)\n",
                     nVersion, ToUnderlying(nType), proTxHash.ToString(), netInfo->ToString(), payee,
                     platformNodeID.ToString(),
                     (nVersion >= ProTxVersion::ExtAddr
                          ? ""
                          : strprintf(", platformP2PPort=%d, platformHTTPPort=%d", platformP2PPort, platformHTTPPort)));
}

bool CProUpRegTx::IsTriviallyValid(gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state) const
{
    if (nVersion == 0 || nVersion > ProTxVersion::GetMaxFromDeployment<decltype(*this)>(pindexPrev)) {
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

bool CProUpRevTx::IsTriviallyValid(gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state) const
{
    if (nVersion == 0 || nVersion > ProTxVersion::GetMaxFromDeployment<decltype(*this)>(pindexPrev)) {
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
