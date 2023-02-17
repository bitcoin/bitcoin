// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/deterministicmns.h>
#include <evo/providertx.h>
#include <evo/specialtx.h>
#include <llmq/quorums_utils.h>

#include <base58.h>
#include <chainparams.h>
#include <clientversion.h>
#include <core_io.h>
#include <coins.h>
#include <hash.h>
#include <messagesigner.h>
#include <script/standard.h>
#include <validation.h>
#include <util/system.h>
bool CProRegTx::IsTriviallyValid(TxValidationState& state, bool is_basic_scheme_active) const
{
    if (nVersion == 0 || nVersion > GetVersion(is_basic_scheme_active)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version");
    }
    if (nType != 0) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-type");
    }
    if (nMode != 0) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-mode");
    }

    if (keyIDOwner.IsNull() || !pubKeyOperator.IsValid() || keyIDVoting.IsNull()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-key-null");
    }

    CTxDestination payoutDest;
    if (!ExtractDestination(scriptPayout, payoutDest)) {
        // should not happen as we checked script types before
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-payee-dest");
    }
    // don't allow reuse of payout key for other keys (don't allow people to put the payee key onto an online server)
    if (payoutDest == CTxDestination(WitnessV0KeyHash(keyIDOwner)) || payoutDest == CTxDestination(WitnessV0KeyHash(keyIDVoting))) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-payee-reuse");
    }

    if (nOperatorReward > 10000) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-operator-reward");
    }
    return true;
}
template <typename ProTx>
static bool CheckService(const ProTx& proTx, TxValidationState& state, bool fJustCheck)
{
    if (!proTx.addr.IsValid()) {
        return FormatSyscoinErrorMessage(state, "bad-protx-ipaddr", fJustCheck);
    }
    if (Params().RequireRoutableExternalIP() && !proTx.addr.IsRoutable()) {
        return FormatSyscoinErrorMessage(state, "bad-protx-ipaddr", fJustCheck);
    }
    ArgsManager args;
    static int mainnetDefaultPort = CreateChainParams(args, CBaseChainParams::MAIN)->GetDefaultPort();
    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (proTx.addr.GetPort() != mainnetDefaultPort) {
            return FormatSyscoinErrorMessage(state, "bad-protx-ipaddr-port", fJustCheck);
        }
    } else if (proTx.addr.GetPort() == mainnetDefaultPort) {
        return FormatSyscoinErrorMessage(state, "bad-protx-ipaddr-port", fJustCheck);
    }

    if (!proTx.addr.IsIPv4()) {
        return FormatSyscoinErrorMessage(state, "bad-protx-ipaddr", fJustCheck);
    }

    return true;
}

template <typename ProTx>
static bool CheckHashSig(const ProTx& proTx, const CKeyID& keyID, TxValidationState& state, bool fJustCheck)
{
    if (!CHashSigner::VerifyHash(::SerializeHash(proTx), keyID, proTx.vchSig)) {
        return FormatSyscoinErrorMessage(state, "bad-protx-sig", fJustCheck);
    }
    return true;
}

template <typename ProTx>
static bool CheckStringSig(const ProTx& proTx, const CKeyID& keyID, TxValidationState& state, bool fJustCheck)
{
    if (!CMessageSigner::VerifyMessage(keyID, proTx.vchSig, proTx.MakeSignString())) {
        return FormatSyscoinErrorMessage(state, "bad-protx-sig", fJustCheck);
    }
    return true;
}

template <typename ProTx>
static bool CheckHashSig(const ProTx& proTx, const CBLSPublicKey& pubKey, TxValidationState& state, bool fJustCheck)
{
    if (!proTx.sig.VerifyInsecure(pubKey, ::SerializeHash(proTx))) {
        return FormatSyscoinErrorMessage(state, "bad-protx-sig", fJustCheck);
    }
    return true;
}

template <typename ProTx>
static bool CheckInputsHash(const CTransaction& tx, const ProTx& proTx, TxValidationState& state, bool fJustCheck)
{
    uint256 inputsHash = CalcTxInputsHash(tx);
    if (inputsHash != proTx.inputsHash) {
        return FormatSyscoinErrorMessage(state, "bad-protx-inputs-hash", fJustCheck);
    }

    return true;
}

bool CheckProRegTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, CCoinsViewCache& view, bool fJustCheck)
{
    AssertLockHeld(cs_main);
    if (tx.nVersion != SYSCOIN_TX_VERSION_MN_REGISTER) {
        return FormatSyscoinErrorMessage(state, "bad-protx-type", fJustCheck);
    }

    CProRegTx ptx;
    if (!GetTxPayload(tx, ptx)) {
        return FormatSyscoinErrorMessage(state, "bad-protx-payload", fJustCheck);
    }

    if (!ptx.IsTriviallyValid(state, llmq::CLLMQUtils::IsV19Active(pindexPrev->nHeight))) {
        return FormatSyscoinErrorMessage(state, state.GetRejectReason(), fJustCheck);
    }

    // It's allowed to set addr to 0, which will put the MN into PoSe-banned state and require a ProUpServTx to be issues later
    // If any of both is set, it must be valid however
    if (ptx.addr != CService() && !CheckService(ptx, state, fJustCheck)) {
        // pass the state returned by the function above
        return false;
    }

    CTxDestination collateralTxDest;
    CKeyID keyForPayloadSig;
    COutPoint collateralOutpoint;

    if (!ptx.collateralOutpoint.hash.IsNull()) {
        Coin coin;
        if (!view.GetCoin(ptx.collateralOutpoint, coin) || coin.IsSpent() || coin.out.nValue != nMNCollateralRequired) {
            return FormatSyscoinErrorMessage(state, "bad-protx-collateral", fJustCheck);
        }

        if (!ExtractDestination(coin.out.scriptPubKey, collateralTxDest)) {
            return FormatSyscoinErrorMessage(state, "bad-protx-collateral-dest", fJustCheck);
        }

        // Extract key from collateral. This only works for P2PK and P2PKH collaterals and will fail for P2SH.
        // Issuer of this ProRegTx must prove ownership with this key by signing the ProRegTx
        if (auto witness_id = std::get_if<WitnessV0KeyHash>(&collateralTxDest)) {	
            keyForPayloadSig = ToKeyID(*witness_id);
        }	
        else if (auto key_id = std::get_if<PKHash>(&collateralTxDest)) {	
            keyForPayloadSig = ToKeyID(*key_id);
        }	
        if (keyForPayloadSig.IsNull()) {
            return FormatSyscoinErrorMessage(state, "bad-protx-collateral-pkh", fJustCheck);
        }

        collateralOutpoint = ptx.collateralOutpoint;
    } else {
        if (ptx.collateralOutpoint.n >= tx.vout.size()) {
            return FormatSyscoinErrorMessage(state, "bad-protx-collateral-index", fJustCheck);
        }
        if (tx.vout[ptx.collateralOutpoint.n].nValue != nMNCollateralRequired) {
            return FormatSyscoinErrorMessage(state, "bad-protx-collateral", fJustCheck);
        }

        if (!ExtractDestination(tx.vout[ptx.collateralOutpoint.n].scriptPubKey, collateralTxDest)) {
            return FormatSyscoinErrorMessage(state, "bad-protx-collateral-dest", fJustCheck);
        }

        collateralOutpoint = COutPoint(tx.GetHash(), ptx.collateralOutpoint.n);
    }

    // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
    // this check applies to internal and external collateral, but internal collaterals are not necessarily a P2PKH
    if (collateralTxDest == CTxDestination(WitnessV0KeyHash(ptx.keyIDOwner)) || collateralTxDest == CTxDestination(WitnessV0KeyHash(ptx.keyIDVoting))) {
        return FormatSyscoinErrorMessage(state, "bad-protx-collateral-reuse", fJustCheck);
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev);

        // only allow reusing of addresses when it's for the same collateral (which replaces the old MN)
        if (mnList.HasUniqueProperty(ptx.addr) && mnList.GetUniquePropertyMN(ptx.addr)->collateralOutpoint != collateralOutpoint) {
            return FormatSyscoinErrorMessage(state, "bad-protx-dup-addr", fJustCheck);
        }

        // never allow duplicate keys, even if this ProTx would replace an existing MN
        if (mnList.HasUniqueProperty(ptx.keyIDOwner) || mnList.HasUniqueProperty(ptx.pubKeyOperator)) {
            return FormatSyscoinErrorMessage(state, "bad-protx-dup-key", fJustCheck);
        }

    }

    if (!CheckInputsHash(tx, ptx, state, fJustCheck)) {
        return false;
    }

    if (!keyForPayloadSig.IsNull()) {
        // collateral is not part of this ProRegTx, so we must verify ownership of the collateral
        if (!CheckStringSig(ptx, keyForPayloadSig, state, fJustCheck)) {
            // pass the state returned by the function above
            return false;
        }
    } else {
        // collateral is part of this ProRegTx, so we know the collateral is owned by the issuer
        if (!ptx.vchSig.empty()) {
            return FormatSyscoinErrorMessage(state, "bad-protx-sig", fJustCheck);
        }
    }

    return true;
}

bool CheckProUpServTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, bool fJustCheck)
{
    if (tx.nVersion != SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE) {
        return FormatSyscoinErrorMessage(state, "bad-protx-type", fJustCheck);
    }

    CProUpServTx ptx;
    if (!GetTxPayload(tx, ptx)) {
        return FormatSyscoinErrorMessage(state, "bad-protx-payload", fJustCheck);
    }

    if (!ptx.IsTriviallyValid(state, llmq::CLLMQUtils::IsV19Active(pindexPrev->nHeight))) {
        return FormatSyscoinErrorMessage(state, state.GetRejectReason(), fJustCheck);
    }

    if (!CheckService(ptx, state, fJustCheck)) {
        // pass the state returned by the function above
        return false;
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev);
        auto mn = mnList.GetMN(ptx.proTxHash);
        if (!mn) {
            return FormatSyscoinErrorMessage(state, "bad-protx-hash", fJustCheck);
        }

        // don't allow updating to addresses already used by other MNs
        if (mnList.HasUniqueProperty(ptx.addr) && mnList.GetUniquePropertyMN(ptx.addr)->proTxHash != ptx.proTxHash) {
            return FormatSyscoinErrorMessage(state, "bad-protx-dup-addr", fJustCheck);
        }

        if (ptx.scriptOperatorPayout != CScript()) {
            if (mn->nOperatorReward == 0) {
                // don't allow to set operator reward payee in case no operatorReward was set
                return FormatSyscoinErrorMessage(state, "bad-protx-operator-payee", fJustCheck);
            }
            CTxDestination payoutDest;
            if (!ExtractDestination(ptx.scriptOperatorPayout, payoutDest)) {
                // should not happen as we checked script types before
                return FormatSyscoinErrorMessage(state, "bad-protx-operator-payee", fJustCheck);
            }
        }

        // we can only check the signature if pindexPrev != nullptr and the MN is known
        if (!CheckInputsHash(tx, ptx, state, fJustCheck)) {
            // pass the state returned by the function above
            return false;
        }
        if (!CheckHashSig(ptx, mn->pdmnState->pubKeyOperator.Get(), state, fJustCheck)) {
            // pass the state returned by the function above
            return false;
        }
    }

    return true;
}

bool CheckProUpRegTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, CCoinsViewCache& view, bool fJustCheck)
{
    if (tx.nVersion != SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR) {
        return FormatSyscoinErrorMessage(state, "bad-protx-type", fJustCheck);
    }

    CProUpRegTx ptx;
    if (!GetTxPayload(tx, ptx)) {
        return FormatSyscoinErrorMessage(state, "bad-protx-payload", fJustCheck);
    }
    if (!ptx.IsTriviallyValid(state, llmq::CLLMQUtils::IsV19Active(pindexPrev->nHeight))) {
        return FormatSyscoinErrorMessage(state, state.GetRejectReason(), fJustCheck);
    }
    

    CTxDestination payoutDest;
    if (!ExtractDestination(ptx.scriptPayout, payoutDest)) {
        // should not happen as we checked script types before
        return FormatSyscoinErrorMessage(state, "bad-protx-payee-dest", fJustCheck);
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev);
        auto dmn = mnList.GetMN(ptx.proTxHash);
        if (!dmn) {
            return FormatSyscoinErrorMessage(state, "bad-protx-hash", fJustCheck);
        }

        // don't allow reuse of payee key for other keys (don't allow people to put the payee key onto an online server)
        if (payoutDest == CTxDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner)) || payoutDest == CTxDestination(WitnessV0KeyHash(ptx.keyIDVoting))) {
            return FormatSyscoinErrorMessage(state, "bad-protx-payee-reuse", fJustCheck);
        }

        Coin coin;
        if (!view.GetCoin(dmn->collateralOutpoint, coin) || coin.IsSpent()) {
            // this should never happen (there would be no dmn otherwise)
            return FormatSyscoinErrorMessage(state, "bad-protx-collateral", fJustCheck);
        }

        // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
        CTxDestination collateralTxDest;
        if (!ExtractDestination(coin.out.scriptPubKey, collateralTxDest)) {
            return FormatSyscoinErrorMessage(state, "bad-protx-collateral-dest", fJustCheck);
        }
        if (collateralTxDest == CTxDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner)) || collateralTxDest == CTxDestination(WitnessV0KeyHash(ptx.keyIDVoting))) {
            return FormatSyscoinErrorMessage(state, "bad-protx-collateral-reuse", fJustCheck);
        }

        if (mnList.HasUniqueProperty(ptx.pubKeyOperator)) {
            auto otherDmn = mnList.GetUniquePropertyMN(ptx.pubKeyOperator);
            if (ptx.proTxHash != otherDmn->proTxHash) {
                return FormatSyscoinErrorMessage(state, "bad-protx-dup-key", fJustCheck);
            }
        }

        if (!CheckInputsHash(tx, ptx, state, fJustCheck)) {
            // pass the state returned by the function above
            return false;
        }
        if (!CheckHashSig(ptx, dmn->pdmnState->keyIDOwner, state, fJustCheck)) {
            // pass the state returned by the function above
            return false;
        }
    }

    return true;
}

bool CheckProUpRevTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, bool fJustCheck)
{
    if (tx.nVersion != SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE) {
        return FormatSyscoinErrorMessage(state, "bad-protx-type", fJustCheck);
    }

    CProUpRevTx ptx;
    if (!GetTxPayload(tx, ptx)) {
        return FormatSyscoinErrorMessage(state, "bad-protx-payload", fJustCheck);
    }

    if (!ptx.IsTriviallyValid(state, llmq::CLLMQUtils::IsV19Active(pindexPrev->nHeight))) {
        return FormatSyscoinErrorMessage(state, state.GetRejectReason(), fJustCheck);
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev);
        auto dmn = mnList.GetMN(ptx.proTxHash);
        if (!dmn)
            return FormatSyscoinErrorMessage(state, "bad-protx-hash", fJustCheck);

        if (!CheckInputsHash(tx, ptx, state, fJustCheck)) {
            // pass the state returned by the function above
            return false;
        }
        if (!CheckHashSig(ptx, dmn->pdmnState->pubKeyOperator.Get(), state, fJustCheck)) {
            // pass the state returned by the function above
            return false;
        }
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
    s += EncodeDestination(WitnessV0KeyHash(keyIDOwner)) + "|";
    s += EncodeDestination(WitnessV0KeyHash(keyIDVoting)) + "|";

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
    nVersion, collateralOutpoint.ToStringShort(), addr.ToStringAddr(), (double)nOperatorReward / 100, EncodeDestination(WitnessV0KeyHash(keyIDOwner)), pubKeyOperator.ToString(nVersion == LEGACY_BLS_VERSION), EncodeDestination(WitnessV0KeyHash(keyIDVoting)), payee);
}

bool CProUpServTx::IsTriviallyValid(TxValidationState& state, bool is_basic_scheme_active) const
{
    if (nVersion == 0 || nVersion > GetVersion(is_basic_scheme_active)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version");
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

    return strprintf("CProUpServTx(nVersion=%d, proTxHash=%s, addr=%s, operatorPayoutAddress=%s)",
        nVersion, proTxHash.ToString(), addr.ToStringAddr(), payee);
}

bool CProUpRegTx::IsTriviallyValid(TxValidationState& state, bool is_basic_scheme_active) const
{
    if (nVersion == 0 || nVersion > GetVersion(is_basic_scheme_active)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version");
    }
    if (nMode != 0) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-mode");
    }

    if (!pubKeyOperator.IsValid() || keyIDVoting.IsNull()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-key-null");
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
        nVersion, proTxHash.ToString(), pubKeyOperator.ToString(nVersion == LEGACY_BLS_VERSION), EncodeDestination(WitnessV0KeyHash(keyIDVoting)), payee);
}

bool CProUpRevTx::IsTriviallyValid(TxValidationState& state, bool is_basic_scheme_active) const
{
    if (nVersion == 0 || nVersion > GetVersion(is_basic_scheme_active)) {
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
