// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "providertx.h"
#include "specialtx.h"
#include "deterministicmns.h"

#include "hash.h"
#include "clientversion.h"
#include "streams.h"
#include "messagesigner.h"
#include "chainparams.h"
#include "validation.h"
#include "univalue.h"
#include "core_io.h"
#include "script/standard.h"
#include "base58.h"

template <typename ProTx>
static bool CheckService(const uint256& proTxHash, const ProTx& proTx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (!proTx.addr.IsValid())
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-addr");
    if (Params().NetworkIDString() != CBaseChainParams::REGTEST && !proTx.addr.IsRoutable())
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-addr");

    if (!proTx.addr.IsIPv4())
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-addr");

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev->GetBlockHash());
        if (mnList.HasUniqueProperty(proTx.addr) && mnList.GetUniquePropertyMN(proTx.addr)->proTxHash != proTxHash) {
            return state.DoS(10, false, REJECT_DUPLICATE, "bad-protx-dup-addr");
        }
    }

    return true;
}

template <typename ProTx>
static bool CheckSig(const ProTx& proTx, const CKeyID& keyID, CValidationState& state)
{
    std::string strError;
    if (!CHashSigner::VerifyHash(::SerializeHash(proTx), keyID, proTx.vchSig, strError))
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-sig", false, strError);
    return true;
}

template <typename ProTx>
static bool CheckSig(const ProTx& proTx, const CBLSPublicKey& pubKey, CValidationState& state)
{
    if (!proTx.sig.VerifyInsecure(pubKey, ::SerializeHash(proTx)))
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-sig", false);
    return true;
}

template <typename ProTx>
static bool CheckInputsHash(const CTransaction &tx, const ProTx& proTx, CValidationState& state)
{
    uint256 inputsHash = CalcTxInputsHash(tx);
    if (inputsHash != proTx.inputsHash)
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-inputs-hash");

    return true;
}

template <typename ProTx, typename PubKey>
static bool CheckInputsHashAndSig(const CTransaction &tx, const ProTx& proTx, const PubKey& pubKey, CValidationState& state)
{
    if (!CheckInputsHash(tx, proTx, state))
        return false;

    if (!CheckSig(proTx, pubKey, state))
        return false;

    return true;
}

bool CheckProRegTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    AssertLockHeld(cs_main);

    CProRegTx ptx;
    if (!GetTxPayload(tx, ptx))
        return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

    if (ptx.nVersion > CProRegTx::CURRENT_VERSION)
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-version");
    if (ptx.nType != 0)
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-type");
    if (ptx.nMode != 0)
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-mode");

    // when collateralOutpoint refers to an external collateral, we check it further down
    if (ptx.collateralOutpoint.hash.IsNull()) {
        if (ptx.collateralOutpoint.n >= tx.vout.size())
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral-index");
        if (tx.vout[ptx.collateralOutpoint.n].nValue != 1000 * COIN)
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral");

        // This is a temporary restriction that will be lifted later
        // It is required while we are transitioning from the old MN list to the deterministic list
        if (tx.vout[ptx.collateralOutpoint.n].scriptPubKey != ptx.scriptPayout)
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee-collateral");
    }
    if (ptx.keyIDOwner.IsNull() || !ptx.pubKeyOperator.IsValid() || ptx.keyIDVoting.IsNull())
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-key-null");
    // we may support P2SH later, but restrict it for now (while in transitioning phase from old MN list to deterministic list)
    if (!ptx.scriptPayout.IsPayToPublicKeyHash())
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee");

    CTxDestination payoutDest;
    if (!ExtractDestination(ptx.scriptPayout, payoutDest)) {
        // should not happen as we checked script types before
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee-dest");
    }
    // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
    if (payoutDest == CTxDestination(ptx.keyIDOwner) || payoutDest == CTxDestination(ptx.keyIDVoting)) {
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee-reuse");
    }

    // It's allowed to set addr/protocolVersion to 0, which will put the MN into PoSe-banned state and require a ProUpServTx to be issues later
    // If any of both is set, it must be valid however
    if (ptx.addr != CService() && !CheckService(tx.GetHash(), ptx, pindexPrev, state))
        return false;

    if (ptx.nOperatorReward > 10000)
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-operator-reward");

    CKeyID keyForPayloadSig;

    if (!ptx.collateralOutpoint.hash.IsNull()) {
        Coin coin;
        if (!GetUTXOCoin(ptx.collateralOutpoint, coin) || coin.out.nValue != 1000 * COIN) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral");
        }

        CTxDestination txDest;
        if (!ExtractDestination(coin.out.scriptPubKey, txDest)) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral-dest");
        }

        // Extract key from collateral. This only works for P2PK and P2PKH collaterals and will fail for P2SH.
        // Issuer of this ProRegTx must prove ownership with this key by signing the ProRegTx
        CBitcoinAddress txAddr(txDest);
        if (!txAddr.GetKeyID(keyForPayloadSig)) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-collateral-pkh");
        }
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev->GetBlockHash());
        if (mnList.HasUniqueProperty(ptx.keyIDOwner) || mnList.HasUniqueProperty(ptx.pubKeyOperator)) {
            return state.DoS(10, false, REJECT_DUPLICATE, "bad-protx-dup-key");
        }

        // check for collaterals which are already used in other MNs
        if (!ptx.collateralOutpoint.hash.IsNull() && mnList.HasUniqueProperty(ptx.collateralOutpoint)) {
            return state.DoS(10, false, REJECT_DUPLICATE, "bad-protx-dup-collateral");
        }

        if (!deterministicMNManager->IsDeterministicMNsSporkActive(pindexPrev->nHeight)) {
            if (ptx.keyIDOwner != ptx.keyIDVoting) {
                return state.DoS(10, false, REJECT_INVALID, "bad-protx-key-not-same");
            }
        }
    }

    if (!keyForPayloadSig.IsNull()) {
        // collateral is not part of this ProRegTx, so we must verify ownership of the collateral
        if (!CheckInputsHashAndSig(tx, ptx, keyForPayloadSig, state)) {
            return false;
        }
    } else {
        // collateral is part of this ProRegTx, so we know the collateral is owned by the issuer
        if (!ptx.vchSig.empty()) {
            return state.DoS(100, false, REJECT_INVALID, "bad-protx-sig");
        }
        if (!CheckInputsHash(tx, ptx, state)) {
            return false;
        }
    }

    return true;
}

bool CheckProUpServTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    AssertLockHeld(cs_main);

    CProUpServTx ptx;
    if (!GetTxPayload(tx, ptx))
        return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

    if (ptx.nVersion > CProRegTx::CURRENT_VERSION)
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-version");

    if (!CheckService(ptx.proTxHash, ptx, pindexPrev, state))
        return false;

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev->GetBlockHash());
        auto mn = mnList.GetMN(ptx.proTxHash);
        if (!mn)
            return state.DoS(100, false, REJECT_INVALID, "bad-protx-hash");

        if (ptx.scriptOperatorPayout != CScript()) {
            if (mn->nOperatorReward == 0) {
                // don't allow to set operator reward payee in case no operatorReward was set
                return state.DoS(10, false, REJECT_INVALID, "bad-protx-operator-payee");
            }
            // we may support P2SH later, but restrict it for now (while in transitioning phase from old MN list to deterministic list)
            if (!ptx.scriptOperatorPayout.IsPayToPublicKeyHash())
                return state.DoS(10, false, REJECT_INVALID, "bad-protx-operator-payee");
        }

        // we can only check the signature if pindexPrev != NULL and the MN is known
        if (!CheckInputsHashAndSig(tx, ptx, mn->pdmnState->pubKeyOperator, state))
            return false;
    }

    return true;
}

bool CheckProUpRegTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    AssertLockHeld(cs_main);

    CProUpRegTx ptx;
    if (!GetTxPayload(tx, ptx))
        return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

    if (ptx.nVersion > CProRegTx::CURRENT_VERSION)
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-version");
    if (ptx.nMode != 0)
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-mode");

    if (!ptx.pubKeyOperator.IsValid() || ptx.keyIDVoting.IsNull())
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-key-null");
    // we may support P2SH later, but restrict it for now (while in transitioning phase from old MN list to deterministic list)
    if (!ptx.scriptPayout.IsPayToPublicKeyHash())
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee");

    CTxDestination payoutDest;
    if (!ExtractDestination(ptx.scriptPayout, payoutDest)) {
        // should not happen as we checked script types before
        return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee-dest");
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev->GetBlockHash());
        auto dmn = mnList.GetMN(ptx.proTxHash);
        if (!dmn)
            return state.DoS(100, false, REJECT_INVALID, "bad-protx-hash");

        // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
        if (payoutDest == CTxDestination(dmn->pdmnState->keyIDOwner) || payoutDest == CTxDestination(ptx.keyIDVoting)) {
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee-reuse");
        }

        // This is a temporary restriction that will be lifted later
        // It is required while we are transitioning from the old MN list to the deterministic list
        Coin coin;
        if (!GetUTXOCoin(dmn->collateralOutpoint, coin)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-protx-payee-collateral");
        }
        if (coin.out.scriptPubKey != ptx.scriptPayout)
            return state.DoS(10, false, REJECT_INVALID, "bad-protx-payee-collateral");

        if (mnList.HasUniqueProperty(ptx.pubKeyOperator)) {
            auto otherDmn = mnList.GetUniquePropertyMN(ptx.pubKeyOperator);
            if (ptx.proTxHash != otherDmn->proTxHash) {
                return state.DoS(10, false, REJECT_DUPLICATE, "bad-protx-dup-key");
            }
        }

        if (!deterministicMNManager->IsDeterministicMNsSporkActive(pindexPrev->nHeight)) {
            if (dmn->pdmnState->keyIDOwner != ptx.keyIDVoting) {
                return state.DoS(10, false, REJECT_INVALID, "bad-protx-key-not-same");
            }
        }

        if (!CheckInputsHashAndSig(tx, ptx, dmn->pdmnState->keyIDOwner, state))
            return false;
    }

    return true;
}

bool CheckProUpRevTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    AssertLockHeld(cs_main);

    CProUpRevTx ptx;
    if (!GetTxPayload(tx, ptx))
        return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

    if (ptx.nVersion > CProRegTx::CURRENT_VERSION)
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-version");

    // ptx.nReason < CProUpRevTx::REASON_NOT_SPECIFIED is always `false` since
    // ptx.nReason is unsigned and CProUpRevTx::REASON_NOT_SPECIFIED == 0
    if (ptx.nReason > CProUpRevTx::REASON_LAST)
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-reason");

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev->GetBlockHash());
        auto dmn = mnList.GetMN(ptx.proTxHash);
        if (!dmn)
            return state.DoS(100, false, REJECT_INVALID, "bad-protx-hash");

        if (!CheckInputsHashAndSig(tx, ptx, dmn->pdmnState->pubKeyOperator, state))
            return false;
    }

    return true;
}

std::string CProRegTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptPayout, dest)) {
        payee = CBitcoinAddress(dest).ToString();
    }

    return strprintf("CProRegTx(nVersion=%d, collateralOutpoint=%s, addr=%s, nOperatorReward=%f, keyIDOwner=%s, pubKeyOperator=%s, keyIDVoting=%s, scriptPayout=%s)",
        nVersion, collateralOutpoint.ToStringShort(), addr.ToString(), (double)nOperatorReward / 100, keyIDOwner.ToString(), pubKeyOperator.ToString(), keyIDVoting.ToString(), payee);
}

void CProRegTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("version", nVersion));
    obj.push_back(Pair("collateralHash", collateralOutpoint.hash.ToString()));
    obj.push_back(Pair("collateralIndex", (int)collateralOutpoint.n));
    obj.push_back(Pair("service", addr.ToString(false)));
    obj.push_back(Pair("keyIDOwner", keyIDOwner.ToString()));
    obj.push_back(Pair("pubKeyOperator", pubKeyOperator.ToString()));
    obj.push_back(Pair("keyIDVoting", keyIDVoting.ToString()));

    CTxDestination dest;
    if (ExtractDestination(scriptPayout, dest)) {
        CBitcoinAddress bitcoinAddress(dest);
        obj.push_back(Pair("payoutAddress", bitcoinAddress.ToString()));
    }
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

    return strprintf("CProUpRegTx(nVersion=%d, proTxHash=%s, pubKeyOperator=%s, keyIDVoting=%s, payoutAddress=%s)",
                     nVersion, proTxHash.ToString(), pubKeyOperator.ToString(), keyIDVoting.ToString(), payee);
}

void CProUpRegTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("version", nVersion));
    obj.push_back(Pair("proTxHash", proTxHash.ToString()));
    obj.push_back(Pair("pubKeyOperator", pubKeyOperator.ToString()));
    obj.push_back(Pair("keyIDVoting", keyIDVoting.ToString()));
    CTxDestination dest;
    if (ExtractDestination(scriptPayout, dest)) {
        CBitcoinAddress bitcoinAddress(dest);
        obj.push_back(Pair("payoutAddress", bitcoinAddress.ToString()));
    }
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
