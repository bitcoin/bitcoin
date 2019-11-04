// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <special/providertx.h>
#include <special/deterministicmns.h>
#include <special/specialtx.h>
#include <special/util.h>

#include <base58.h>
#include <chainparams.h>
#include <clientversion.h>
#include <core_io.h>
#include <hash.h>
#include <key_io.h>
#include <messagesigner.h>
#include <script/standard.h>
#include <streams.h>
#include <univalue.h>
#include <validation.h>
#include <wallet/wallet.h>

template <typename ProTx>
static bool CheckService(const uint256& proTxHash, const ProTx& proTx, CValidationState& state)
{
    if (!proTx.addr.IsValid())
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-addr");

   if (Params().NetworkIDString() != CBaseChainParams::REGTEST && !proTx.addr.IsRoutable())
       return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-addr");

    int mainnetDefaultPort = Params().GetDefaultPort();
    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (proTx.addr.GetPort() != mainnetDefaultPort && !Params().AllowMultiplePorts()) {
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-addr-port");
        }
    } else if (proTx.addr.GetPort() == mainnetDefaultPort && !Params().AllowMultiplePorts()) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-addr-port");
    }

    if (!proTx.addr.IsIPv4()) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-addr");
    }

    return true;
}

template <typename ProTx>
static bool CheckHashSig(const ProTx& proTx, const CKeyID& keyID, CValidationState& state)
{
    std::string strError;
    if (!CHashSigner::VerifyHash(::SerializeHash(proTx), keyID, proTx.vchSig, strError)) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-sig", strError);
    }
    return true;
}

template <typename ProTx>
static bool CheckStringSig(const ProTx& proTx, const CKeyID& keyID, CValidationState& state)
{
    std::string strError;
    if (!CMessageSigner::VerifyMessage(keyID, proTx.vchSig, proTx.MakeSignString(), strError)) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-sig", strError);
    }
    return true;
}

template <typename ProTx>
static bool CheckHashSig(const ProTx& proTx, const CBLSPublicKey& pubKey, CValidationState& state)
{
    if (!proTx.sig.VerifyInsecure(pubKey, ::SerializeHash(proTx)))
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-sig");
    return true;
}

template <typename ProTx>
static bool CheckInputsHash(const CTransaction& tx, const ProTx& proTx, CValidationState& state)
{
    uint256 inputsHash = CalcTxInputsHash(tx);
    if (inputsHash != proTx.inputsHash) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-inputs-hash");
    }

    return true;
}

bool CheckProRegTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx.nType != TRANSACTION_PROVIDER_REGISTER)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-type");

    CProRegTx ptx;
    if (!GetTxPayload(tx, ptx))
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-payload");

    if (ptx.nVersion == 0 || ptx.nVersion > CProRegTx::CURRENT_VERSION)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-version");
    if (ptx.nType != 0)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-type");
    if (ptx.nMode != 0)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-mode");
    if (ptx.keyIDOwner.IsNull() || !ptx.pubKeyOperator.IsValid() || ptx.keyIDVoting.IsNull())
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-key-null");
    if (!ptx.scriptPayout.IsPayToScriptHash())
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-payee");

    CTxDestination payoutDest;
    // should not happen as we checked script types before
    if (!ExtractDestination(ptx.scriptPayout, payoutDest))
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-payee-dest");

    // don't allow reuse of payout key for other keys (don't allow people to put the payee key onto an online server)
    if (payoutDest == CTxDestination(PKHash(ptx.keyIDOwner)) || payoutDest == CTxDestination(PKHash(ptx.keyIDVoting)))
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-payee-reuse");

    // It's allowed to set addr to 0, which will put the MN into PoSe-banned state and require a ProUpServTx to be issues later
    // If any of both is set, it must be valid however
    if (ptx.addr != CService() && !CheckService(tx.GetHash(), ptx, state))
        return false;

    if (ptx.nOperatorReward > 10000)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-operator-reward");

    CTxDestination collateralTxDest;
    CKeyID keyForPayloadSig;
    COutPoint collateralOutpoint;

    if (!ptx.collateralOutpoint.hash.IsNull()) {
        Coin coin;
        if (!GetUTXOCoin(ptx.collateralOutpoint, coin) || coin.out.nValue != 2500 * COIN)
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-collateral");
        if (!ExtractDestination(coin.out.scriptPubKey, collateralTxDest))
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-collateral-dest");

        if (!GetWallets().front())
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-wallet");

        CWallet* const pwallet = GetWallets().front().get();

        // Extract key from collateral. This only works for P2PK and P2PKH collaterals and will fail for P2SH.
        // Issuer of this ProRegTx must prove ownership with this key by signing the ProRegTx
        keyForPayloadSig = GetKeyForDestination(*pwallet, collateralTxDest);
        if (keyForPayloadSig.IsNull())
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-collateral-pkh");

        collateralOutpoint = ptx.collateralOutpoint;
    } else {
        if (ptx.collateralOutpoint.n >= tx.vout.size())
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID,
                                 "bad-protx-collateral-index");
        if (tx.vout[ptx.collateralOutpoint.n].nValue != 2500 * COIN)
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-collateral");
        if (!ExtractDestination(tx.vout[ptx.collateralOutpoint.n].scriptPubKey, collateralTxDest))
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID,
                                 "bad-protx-collateral-dest");

        collateralOutpoint = COutPoint(tx.GetHash(), ptx.collateralOutpoint.n);
    }

    // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
    // this check applies to internal and external collateral, but internal collaterals are not necessarely a P2PKH
    if (collateralTxDest == CTxDestination(PKHash(ptx.keyIDOwner)) || collateralTxDest == CTxDestination(PKHash(ptx.keyIDVoting)))
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-collateral-reuse");

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev);

        // only allow reusing of addresses when it's for the same collateral (which replaces the old MN)
        if (mnList.HasUniqueProperty(ptx.addr) && mnList.GetUniquePropertyMN(ptx.addr)->collateralOutpoint != collateralOutpoint)
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_DUPLICATE, "bad-protx-dup-addr");

        // never allow duplicate keys, even if this ProTx would replace an existing MN
        if (mnList.HasUniqueProperty(ptx.keyIDOwner) || mnList.HasUniqueProperty(ptx.pubKeyOperator))
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_DUPLICATE, "bad-protx-dup-key");
    }

    if (!CheckInputsHash(tx, ptx, state))
        return false;

    if (!keyForPayloadSig.IsNull()) {
        // collateral is not part of this ProRegTx, so we must verify ownership of the collateral
        if (!CheckStringSig(ptx, keyForPayloadSig, state))
            return false;
    } else {
        // collateral is part of this ProRegTx, so we know the collateral is owned by the issuer
        if (!ptx.vchSig.empty())
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-sig");
    }

    return true;
}

bool CheckProUpServTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx.nType != TRANSACTION_PROVIDER_UPDATE_SERVICE) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-type");
    }

    CProUpServTx ptx;
    if (!GetTxPayload(tx, ptx)) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-payload");
    }

    if (ptx.nVersion == 0 || ptx.nVersion > CProRegTx::CURRENT_VERSION) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-version");
    }

    if (!CheckService(ptx.proTxHash, ptx, state)) {
        return false;
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev);
        auto mn = mnList.GetMN(ptx.proTxHash);
        if (!mn) {
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-hash");
        }

        // don't allow updating to addresses already used by other MNs
        if (mnList.HasUniqueProperty(ptx.addr) && mnList.GetUniquePropertyMN(ptx.addr)->proTxHash != ptx.proTxHash) {
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_DUPLICATE, "bad-protx-dup-addr");
        }

        if (ptx.scriptOperatorPayout != CScript()) {
            //    don't allow to set operator reward payee in case no operatorReward was set
            if (mn->nOperatorReward == 0)
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-operator-payee");
            if (!ptx.scriptOperatorPayout.IsPayToScriptHash())
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-operator-payee");
        }

        // we can only check the signature if pindexPrev != NULL and the MN is known
        if (!CheckInputsHash(tx, ptx, state))
            return false;
        if (!CheckHashSig(ptx, mn->pdmnState->pubKeyOperator.Get(), state))
            return false;
    }

    return true;
}

bool CheckProUpRegTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx.nType != TRANSACTION_PROVIDER_UPDATE_REGISTRAR)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-type");

    CProUpRegTx ptx;
    if (!GetTxPayload(tx, ptx))
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-payload");

    if (ptx.nVersion == 0 || ptx.nVersion > CProRegTx::CURRENT_VERSION)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-version");
    if (ptx.nMode != 0)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-mode");

    if (!ptx.pubKeyOperator.IsValid() || ptx.keyIDVoting.IsNull())
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-key-null");
    if (!ptx.scriptPayout.IsPayToScriptHash())
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-payee");

    CTxDestination payoutDest;
    if (!ExtractDestination(ptx.scriptPayout, payoutDest)) {
        // should not happen as we checked script types before
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-payee-dest");
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev);
        auto dmn = mnList.GetMN(ptx.proTxHash);
        if (!dmn)
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-hash");

        // don't allow reuse of payee key for other keys (don't allow people to put the payee key onto an online server)
        if (payoutDest == CTxDestination(PKHash(dmn->pdmnState->keyIDOwner)) || payoutDest == CTxDestination(PKHash(ptx.keyIDVoting)))
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-payee-reuse");

        Coin coin;
        if (!GetUTXOCoin(dmn->collateralOutpoint, coin)) {
            // this should never happen (there would be no dmn otherwise)
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-collateral");
        }

        // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
        CTxDestination collateralTxDest;
        if (!ExtractDestination(coin.out.scriptPubKey, collateralTxDest))
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-collateral-dest");

        if (collateralTxDest == CTxDestination(PKHash(dmn->pdmnState->keyIDOwner)) || collateralTxDest == CTxDestination(PKHash(ptx.keyIDVoting)))
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-collateral-reuse");

        if (mnList.HasUniqueProperty(ptx.pubKeyOperator)) {
            auto otherDmn = mnList.GetUniquePropertyMN(ptx.pubKeyOperator);
            if (ptx.proTxHash != otherDmn->proTxHash)
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_DUPLICATE, "bad-protx-dup-key");
        }

        if (!CheckInputsHash(tx, ptx, state))
            return false;
        if (!CheckHashSig(ptx, dmn->pdmnState->keyIDOwner, state))
            return false;
    }

    return true;
}

bool CheckProUpRevTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx.nType != TRANSACTION_PROVIDER_UPDATE_REVOKE)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-type");

    CProUpRevTx ptx;
    if (!GetTxPayload(tx, ptx))
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-payload");

    if (ptx.nVersion == 0 || ptx.nVersion > CProRegTx::CURRENT_VERSION)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-version");

    // ptx.nReason < CProUpRevTx::REASON_NOT_SPECIFIED is always `false` since
    // ptx.nReason is unsigned and CProUpRevTx::REASON_NOT_SPECIFIED == 0
    if (ptx.nReason > CProUpRevTx::REASON_LAST)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-reason");

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev);
        auto dmn = mnList.GetMN(ptx.proTxHash);
        if (!dmn)
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-protx-hash");

        if (!CheckInputsHash(tx, ptx, state))
            return false;
        if (!CheckHashSig(ptx, dmn->pdmnState->pubKeyOperator.Get(), state))
            return false;
    }

    return true;
}

std::string CProRegTx::MakeSignString() const
{
    std::string s;

    // We only include the important stuff in the string form...

    CTxDestination destPayout;
    std::string strPayout;
    if (ExtractDestination(scriptPayout, destPayout) && IsValidDestination(destPayout))
        strPayout = EncodeDestination(destPayout);
    else
        strPayout = HexStr(scriptPayout.begin(), scriptPayout.end());

    s += strPayout + "|";
    s += strprintf("%d", nOperatorReward) + "|";
    s += EncodeDestination(PKHash(keyIDOwner)) + "|";
    s += EncodeDestination(PKHash(keyIDVoting)) + "|";

    // ... and also the full hash of the payload as a protection agains malleability and replays
    s += ::SerializeHash(*this).ToString();

    return s;
}

std::string CProRegTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptPayout, dest))
        payee = EncodeDestination(dest);

    return strprintf("CProRegTx(nVersion=%d, collateralOutpoint=%s, addr=%s, nOperatorReward=%f, ownerAddress=%s, pubKeyOperator=%s, votingAddress=%s, scriptPayout=%s)",
        nVersion, collateralOutpoint.ToString().substr(0,64), addr.ToString(), (double)nOperatorReward / 100, EncodeDestination(PKHash(keyIDOwner)), pubKeyOperator.ToString(), EncodeDestination(PKHash(keyIDVoting)), payee);
}

void CProRegTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.pushKV("version", nVersion);
    obj.pushKV("collateralHash", collateralOutpoint.hash.ToString());
    obj.pushKV("collateralIndex", (int)collateralOutpoint.n);
    obj.pushKV("service", addr.ToString());
    obj.pushKV("ownerAddress", EncodeDestination(PKHash(keyIDOwner)));
    obj.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));

    CTxDestination dest;
    if (ExtractDestination(scriptPayout, dest))
        obj.pushKV("payoutAddress", EncodeDestination(dest));

    obj.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    obj.pushKV("operatorReward", (double)nOperatorReward / 100);
    obj.pushKV("inputsHash", inputsHash.ToString());
}

std::string CProUpServTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptOperatorPayout, dest))
        payee = EncodeDestination(dest);

    return strprintf("CProUpServTx(nVersion=%d, proTxHash=%s, addr=%s, operatorPayoutAddress=%s)",
        nVersion, proTxHash.ToString(), addr.ToString(), payee);
}

void CProUpServTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.pushKV("version", nVersion);
    obj.pushKV("proTxHash", proTxHash.ToString());
    obj.pushKV("service", addr.ToString());

    CTxDestination dest;
    if (ExtractDestination(scriptOperatorPayout, dest))
        obj.pushKV("operatorPayoutAddress", EncodeDestination(dest));

    obj.pushKV("inputsHash", inputsHash.ToString());
}

std::string CProUpRegTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptPayout, dest))
        payee = EncodeDestination(dest);

    return strprintf("CProUpRegTx(nVersion=%d, proTxHash=%s, pubKeyOperator=%s, votingAddress=%s, payoutAddress=%s)",
        nVersion, proTxHash.ToString(), pubKeyOperator.ToString(), EncodeDestination(PKHash(keyIDVoting)), payee);
}

void CProUpRegTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.pushKV("version", nVersion);
    obj.pushKV("proTxHash", proTxHash.ToString());
    obj.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));

    CTxDestination dest;
    if (ExtractDestination(scriptPayout, dest))
        obj.pushKV("payoutAddress", EncodeDestination(dest));

    obj.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    obj.pushKV("inputsHash", inputsHash.ToString());
}

std::string CProUpRevTx::ToString() const
{
    return strprintf("CProUpRevTx(nVersion=%d, proTxHash=%s, nReason=%d)",
        nVersion, proTxHash.ToString(), nReason);
}

void CProUpRevTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.pushKV("version", nVersion);
    obj.pushKV("proTxHash", proTxHash.ToString());
    obj.pushKV("reason", (int)nReason);
    obj.pushKV("inputsHash", inputsHash.ToString());
}
