// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "deterministicmns.h"
#include "providertx.h"
#include "specialtx.h"

#include "base58.h"
#include "chainparams.h"
#include "clientversion.h"
#include "core_io.h"
#include "hash.h"
#include "messagesigner.h"
#include "script/standard.h"
#include "streams.h"
#include "univalue.h"
#include "validation.h"

template <typename ProTx>
static bool CheckService(const uint256& proTxHash, const ProTx& proTx, CValidationState& state)
{
    if (!proTx.addr.IsValid()) {
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-addr");
    }
    if (Params().NetworkIDString() != CBaseChainParams::REGTEST && !proTx.addr.IsRoutable()) {
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-addr");
    }

    int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (proTx.addr.GetPort() != mainnetDefaultPort) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-addr-port");
        }
    } else if (proTx.addr.GetPort() == mainnetDefaultPort) {
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-addr-port");
    }

    if (!proTx.addr.IsIPv4()) {
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-addr");
    }

    return true;
}

template <typename ProTx>
static bool CheckHashSig(const ProTx& proTx, const CKeyID& keyID, CValidationState& state)
{
    std::string strError;
    if (!CHashSigner::VerifyHash(::SerializeHash(proTx), keyID, proTx.vchSig, strError)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-sig", false, strError);
    }
    return true;
}

template <typename ProTx>
static bool CheckStringSig(const ProTx& proTx, const CKeyID& keyID, CValidationState& state)
{
    std::string strError;
    if (!CMessageSigner::VerifyMessage(keyID, proTx.vchSig, proTx.MakeSignString(), strError)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-sig", false, strError);
    }
    return true;
}

template <typename ProTx>
static bool CheckHashSig(const ProTx& proTx, const CBLSPublicKey& pubKey, CValidationState& state)
{
    if (!proTx.sig.VerifyInsecure(pubKey, ::SerializeHash(proTx))) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-sig", false);
    }
    return true;
}

template <typename ProTx>
static bool CheckInputsHash(const CTransaction& tx, const ProTx& proTx, CValidationState& state)
{
    uint256 inputsHash = CalcTxInputsHash(tx);
    if (inputsHash != proTx.inputsHash) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-inputs-hash");
    }

    return true;
}

bool CheckProRegTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx.nType != TRANSACTION_PROVIDER_REGISTER) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-type");
    }

    CProRegTx ptx;
    if (!GetTxPayload(tx, ptx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-payload");
    }

    if (ptx.nVersion == 0 || ptx.nVersion > CProRegTx::CURRENT_VERSION) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-version");
    }
    if (ptx.nType != 0) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-type");
    }
    if (ptx.nMode != 0) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-mode");
    }

    if (ptx.keyIDOwner.IsNull() || !ptx.pubKeyOperator.IsValid() || ptx.keyIDVoting.IsNull()) {
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-key-null");
    }
    if (!ptx.scriptPayout.IsPayToPublicKeyHash() && !ptx.scriptPayout.IsPayToScriptHash()) {
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee");
    }

    CTxDestination payoutDest;
    if (!ExtractDestination(ptx.scriptPayout, payoutDest)) {
        // should not happen as we checked script types before
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee-dest");
    }
    // don't allow reuse of payout key for other keys (don't allow people to put the payee key onto an online server)
    if (payoutDest == CTxDestination(ptx.keyIDOwner) || payoutDest == CTxDestination(ptx.keyIDVoting)) {
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee-reuse");
    }

    // It's allowed to set addr to 0, which will put the MN into PoSe-banned state and require a ProUpServTx to be issues later
    // If any of both is set, it must be valid however
    if (ptx.addr != CService() && !CheckService(tx.GetHash(), ptx, state)) {
        return false;
    }

    if (ptx.nOperatorReward > 10000) {
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-operator-reward");
    }

    CTxDestination collateralTxDest;
    CKeyID keyForPayloadSig;
    COutPoint collateralOutpoint;

    if (!ptx.collateralOutpoint.hash.IsNull()) {
        Coin coin;
        if (!GetUTXOCoin(ptx.collateralOutpoint, coin) || coin.out.nValue != 1000 * COIN) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral");
        }

        if (!ExtractDestination(coin.out.scriptPubKey, collateralTxDest)) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral-dest");
        }

        // Extract key from collateral. This only works for P2PK and P2PKH collaterals and will fail for P2SH.
        // Issuer of this ProRegTx must prove ownership with this key by signing the ProRegTx
        if (!CBitcoinAddress(collateralTxDest).GetKeyID(keyForPayloadSig)) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral-pkh");
        }

        collateralOutpoint = ptx.collateralOutpoint;
    } else {
        if (ptx.collateralOutpoint.n >= tx.vout.size()) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral-index");
        }
        if (tx.vout[ptx.collateralOutpoint.n].nValue != 1000 * COIN) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral");
        }

        if (!ExtractDestination(tx.vout[ptx.collateralOutpoint.n].scriptPubKey, collateralTxDest)) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral-dest");
        }

        collateralOutpoint = COutPoint(tx.GetHash(), ptx.collateralOutpoint.n);
    }

    // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
    // this check applies to internal and external collateral, but internal collaterals are not necessarely a P2PKH
    if (collateralTxDest == CTxDestination(ptx.keyIDOwner) || collateralTxDest == CTxDestination(ptx.keyIDVoting)) {
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral-reuse");
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev->GetBlockHash());

        // only allow reusing of addresses when it's for the same collateral (which replaces the old MN)
        if (mnList.HasUniqueProperty(ptx.addr) && mnList.GetUniquePropertyMN(ptx.addr)->collateralOutpoint != collateralOutpoint) {
            return state.DoS(10, false, REJECT_DUPLICATE, "bad-protx-dup-addr");
        }

        // never allow duplicate keys, even if this ProTx would replace an existing MN
        if (mnList.HasUniqueProperty(ptx.keyIDOwner) || mnList.HasUniqueProperty(ptx.pubKeyOperator)) {
            return state.DoS(10, false, REJECT_DUPLICATE, "bad-protx-dup-key");
        }

        if (!deterministicMNManager->IsDIP3Enforced(pindexPrev->nHeight)) {
            if (ptx.keyIDOwner != ptx.keyIDVoting) {
                return state.DoS(10, false, REJECT_INVALID, "bad-protx-key-not-same");
            }
        }
    }

    if (!CheckInputsHash(tx, ptx, state)) {
        return false;
    }

    if (!keyForPayloadSig.IsNull()) {
        // collateral is not part of this ProRegTx, so we must verify ownership of the collateral
        if (!CheckStringSig(ptx, keyForPayloadSig, state)) {
            return false;
        }
    } else {
        // collateral is part of this ProRegTx, so we know the collateral is owned by the issuer
        if (!ptx.vchSig.empty()) {
            return state.DoS(100, false, REJECT_INVALID, "bad-protx-sig");
        }
    }

    return true;
}

bool CheckProUpServTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx.nType != TRANSACTION_PROVIDER_UPDATE_SERVICE) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-type");
    }

    CProUpServTx ptx;
    if (!GetTxPayload(tx, ptx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-payload");
    }

    if (ptx.nVersion == 0 || ptx.nVersion > CProRegTx::CURRENT_VERSION) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-version");
    }

    if (!CheckService(ptx.proTxHash, ptx, state)) {
        return false;
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev->GetBlockHash());
        auto mn = mnList.GetMN(ptx.proTxHash);
        if (!mn) {
            return state.DoS(100, false, REJECT_INVALID, "bad-protx-hash");
        }

        // don't allow updating to addresses already used by other MNs
        if (mnList.HasUniqueProperty(ptx.addr) && mnList.GetUniquePropertyMN(ptx.addr)->proTxHash != ptx.proTxHash) {
            return state.DoS(10, false, REJECT_DUPLICATE, "bad-protx-dup-addr");
        }

        if (ptx.scriptOperatorPayout != CScript()) {
            if (mn->nOperatorReward == 0) {
                // don't allow to set operator reward payee in case no operatorReward was set
                return state.DoS(10, false, REJECT_INVALID, "bad-protx-operator-payee");
            }
            if (!ptx.scriptOperatorPayout.IsPayToPublicKeyHash() && !ptx.scriptOperatorPayout.IsPayToScriptHash()) {
                return state.DoS(10, false, REJECT_INVALID, "bad-protx-operator-payee");
            }
        }

        // we can only check the signature if pindexPrev != NULL and the MN is known
        if (!CheckInputsHash(tx, ptx, state)) {
            return false;
        }
        if (!CheckHashSig(ptx, mn->pdmnState->pubKeyOperator.Get(), state)) {
            return false;
        }
    }

    return true;
}

bool CheckProUpRegTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx.nType != TRANSACTION_PROVIDER_UPDATE_REGISTRAR) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-type");
    }

    CProUpRegTx ptx;
    if (!GetTxPayload(tx, ptx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-payload");
    }

    if (ptx.nVersion == 0 || ptx.nVersion > CProRegTx::CURRENT_VERSION) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-version");
    }
    if (ptx.nMode != 0) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-mode");
    }

    if (!ptx.pubKeyOperator.IsValid() || ptx.keyIDVoting.IsNull()) {
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-key-null");
    }
    if (!ptx.scriptPayout.IsPayToPublicKeyHash() && !ptx.scriptPayout.IsPayToScriptHash()) {
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee");
    }

    CTxDestination payoutDest;
    if (!ExtractDestination(ptx.scriptPayout, payoutDest)) {
        // should not happen as we checked script types before
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee-dest");
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev->GetBlockHash());
        auto dmn = mnList.GetMN(ptx.proTxHash);
        if (!dmn) {
            return state.DoS(100, false, REJECT_INVALID, "bad-protx-hash");
        }

        // don't allow reuse of payee key for other keys (don't allow people to put the payee key onto an online server)
        if (payoutDest == CTxDestination(dmn->pdmnState->keyIDOwner) || payoutDest == CTxDestination(ptx.keyIDVoting)) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee-reuse");
        }

        Coin coin;
        if (!GetUTXOCoin(dmn->collateralOutpoint, coin)) {
            // this should never happen (there would be no dmn otherwise)
            return state.DoS(100, false, REJECT_INVALID, "bad-protx-collateral");
        }

        // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
        CTxDestination collateralTxDest;
        if (!ExtractDestination(coin.out.scriptPubKey, collateralTxDest)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-protx-collateral-dest");
        }
        if (collateralTxDest == CTxDestination(dmn->pdmnState->keyIDOwner) || collateralTxDest == CTxDestination(ptx.keyIDVoting)) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral-reuse");
        }

        if (mnList.HasUniqueProperty(ptx.pubKeyOperator)) {
            auto otherDmn = mnList.GetUniquePropertyMN(ptx.pubKeyOperator);
            if (ptx.proTxHash != otherDmn->proTxHash) {
                return state.DoS(10, false, REJECT_DUPLICATE, "bad-protx-dup-key");
            }
        }

        if (!deterministicMNManager->IsDIP3Enforced(pindexPrev->nHeight)) {
            if (dmn->pdmnState->keyIDOwner != ptx.keyIDVoting) {
                return state.DoS(10, false, REJECT_INVALID, "bad-protx-key-not-same");
            }
        }

        if (!CheckInputsHash(tx, ptx, state)) {
            return false;
        }
        if (!CheckHashSig(ptx, dmn->pdmnState->keyIDOwner, state)) {
            return false;
        }
    }

    return true;
}

bool CheckProUpRevTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx.nType != TRANSACTION_PROVIDER_UPDATE_REVOKE) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-type");
    }

    CProUpRevTx ptx;
    if (!GetTxPayload(tx, ptx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-payload");
    }

    if (ptx.nVersion == 0 || ptx.nVersion > CProRegTx::CURRENT_VERSION) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-version");
    }

    // ptx.nReason < CProUpRevTx::REASON_NOT_SPECIFIED is always `false` since
    // ptx.nReason is unsigned and CProUpRevTx::REASON_NOT_SPECIFIED == 0
    if (ptx.nReason > CProUpRevTx::REASON_LAST) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-reason");
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev->GetBlockHash());
        auto dmn = mnList.GetMN(ptx.proTxHash);
        if (!dmn)
            return state.DoS(100, false, REJECT_INVALID, "bad-protx-hash");

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
    CBitcoinAddress addrPayout;
    std::string strPayout;
    if (ExtractDestination(scriptPayout, destPayout) && addrPayout.Set(destPayout)) {
        strPayout = addrPayout.ToString();
    } else {
        strPayout = HexStr(scriptPayout.begin(), scriptPayout.end());
    }

    s += strPayout + "|";
    s += strprintf("%d", nOperatorReward) + "|";
    s += CBitcoinAddress(keyIDOwner).ToString() + "|";
    s += CBitcoinAddress(keyIDVoting).ToString() + "|";

    // ... and also the full hash of the payload as a protection agains malleability and replays
    s += ::SerializeHash(*this).ToString();

    return s;
}

std::string CProRegTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptPayout, dest)) {
        payee = CBitcoinAddress(dest).ToString();
    }

    return strprintf("CProRegTx(nVersion=%d, collateralOutpoint=%s, addr=%s, nOperatorReward=%f, ownerAddress=%s, pubKeyOperator=%s, votingAddress=%s, scriptPayout=%s)",
        nVersion, collateralOutpoint.ToStringShort(), addr.ToString(), (double)nOperatorReward / 100, CBitcoinAddress(keyIDOwner).ToString(), pubKeyOperator.ToString(), CBitcoinAddress(keyIDVoting).ToString(), payee);
}

void CProRegTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("version", nVersion));
    obj.push_back(Pair("collateralHash", collateralOutpoint.hash.ToString()));
    obj.push_back(Pair("collateralIndex", (int)collateralOutpoint.n));
    obj.push_back(Pair("service", addr.ToString(false)));
    obj.push_back(Pair("ownerAddress", CBitcoinAddress(keyIDOwner).ToString()));
    obj.push_back(Pair("votingAddress", CBitcoinAddress(keyIDVoting).ToString()));

    CTxDestination dest;
    if (ExtractDestination(scriptPayout, dest)) {
        CBitcoinAddress bitcoinAddress(dest);
        obj.push_back(Pair("payoutAddress", bitcoinAddress.ToString()));
    }
    obj.push_back(Pair("pubKeyOperator", pubKeyOperator.ToString()));
    obj.push_back(Pair("operatorReward", (double)nOperatorReward / 100));

    obj.push_back(Pair("inputsHash", inputsHash.ToString()));
}

std::string CProUpServTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        payee = CBitcoinAddress(dest).ToString();
    }

    return strprintf("CProUpServTx(nVersion=%d, proTxHash=%s, addr=%s, operatorPayoutAddress=%s)",
        nVersion, proTxHash.ToString(), addr.ToString(), payee);
}

void CProUpServTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("version", nVersion));
    obj.push_back(Pair("proTxHash", proTxHash.ToString()));
    obj.push_back(Pair("service", addr.ToString(false)));
    CTxDestination dest;
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        CBitcoinAddress bitcoinAddress(dest);
        obj.push_back(Pair("operatorPayoutAddress", bitcoinAddress.ToString()));
    }
    obj.push_back(Pair("inputsHash", inputsHash.ToString()));
}

std::string CProUpRegTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptPayout, dest)) {
        payee = CBitcoinAddress(dest).ToString();
    }

    return strprintf("CProUpRegTx(nVersion=%d, proTxHash=%s, pubKeyOperator=%s, votingAddress=%s, payoutAddress=%s)",
        nVersion, proTxHash.ToString(), pubKeyOperator.ToString(), CBitcoinAddress(keyIDVoting).ToString(), payee);
}

void CProUpRegTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("version", nVersion));
    obj.push_back(Pair("proTxHash", proTxHash.ToString()));
    obj.push_back(Pair("votingAddress", CBitcoinAddress(keyIDVoting).ToString()));
    CTxDestination dest;
    if (ExtractDestination(scriptPayout, dest)) {
        CBitcoinAddress bitcoinAddress(dest);
        obj.push_back(Pair("payoutAddress", bitcoinAddress.ToString()));
    }
    obj.push_back(Pair("pubKeyOperator", pubKeyOperator.ToString()));
    obj.push_back(Pair("inputsHash", inputsHash.ToString()));
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
    obj.push_back(Pair("version", nVersion));
    obj.push_back(Pair("proTxHash", proTxHash.ToString()));
    obj.push_back(Pair("reason", (int)nReason));
    obj.push_back(Pair("inputsHash", inputsHash.ToString()));
}
