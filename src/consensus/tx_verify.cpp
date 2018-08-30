// Copyright (c) 2017-2017 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assets/assets.h>
#include <script/standard.h>
#include <util.h>
#include <validation.h>
#include "tx_verify.h"

#include "consensus.h"
#include "primitives/transaction.h"
#include "script/interpreter.h"
#include "validation.h"
#include <cmath>

// TODO remove the following dependencies
#include "chain.h"
#include "coins.h"
#include "utilmoneystr.h"

bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime)
{
    if (tx.nLockTime == 0)
        return true;
    if ((int64_t)tx.nLockTime < ((int64_t)tx.nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
        return true;
    for (const auto& txin : tx.vin) {
        if (!(txin.nSequence == CTxIn::SEQUENCE_FINAL))
            return false;
    }
    return true;
}

std::pair<int, int64_t> CalculateSequenceLocks(const CTransaction &tx, int flags, std::vector<int>* prevHeights, const CBlockIndex& block)
{
    assert(prevHeights->size() == tx.vin.size());

    // Will be set to the equivalent height- and time-based nLockTime
    // values that would be necessary to satisfy all relative lock-
    // time constraints given our view of block chain history.
    // The semantics of nLockTime are the last invalid height/time, so
    // use -1 to have the effect of any height or time being valid.
    int nMinHeight = -1;
    int64_t nMinTime = -1;

    // tx.nVersion is signed integer so requires cast to unsigned otherwise
    // we would be doing a signed comparison and half the range of nVersion
    // wouldn't support BIP 68.
    bool fEnforceBIP68 = static_cast<uint32_t>(tx.nVersion) >= 2
                      && flags & LOCKTIME_VERIFY_SEQUENCE;

    // Do not enforce sequence numbers as a relative lock time
    // unless we have been instructed to
    if (!fEnforceBIP68) {
        return std::make_pair(nMinHeight, nMinTime);
    }

    for (size_t txinIndex = 0; txinIndex < tx.vin.size(); txinIndex++) {
        const CTxIn& txin = tx.vin[txinIndex];

        // Sequence numbers with the most significant bit set are not
        // treated as relative lock-times, nor are they given any
        // consensus-enforced meaning at this point.
        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG) {
            // The height of this input is not relevant for sequence locks
            (*prevHeights)[txinIndex] = 0;
            continue;
        }

        int nCoinHeight = (*prevHeights)[txinIndex];

        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG) {
            int64_t nCoinTime = block.GetAncestor(std::max(nCoinHeight-1, 0))->GetMedianTimePast();
            // NOTE: Subtract 1 to maintain nLockTime semantics
            // BIP 68 relative lock times have the semantics of calculating
            // the first block or time at which the transaction would be
            // valid. When calculating the effective block time or height
            // for the entire transaction, we switch to using the
            // semantics of nLockTime which is the last invalid block
            // time or height.  Thus we subtract 1 from the calculated
            // time or height.

            // Time-based relative lock-times are measured from the
            // smallest allowed timestamp of the block containing the
            // txout being spent, which is the median time past of the
            // block prior.
            nMinTime = std::max(nMinTime, nCoinTime + (int64_t)((txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) << CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) - 1);
        } else {
            nMinHeight = std::max(nMinHeight, nCoinHeight + (int)(txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) - 1);
        }
    }

    return std::make_pair(nMinHeight, nMinTime);
}

bool EvaluateSequenceLocks(const CBlockIndex& block, std::pair<int, int64_t> lockPair)
{
    assert(block.pprev);
    int64_t nBlockTime = block.pprev->GetMedianTimePast();
    if (lockPair.first >= block.nHeight || lockPair.second >= nBlockTime)
        return false;

    return true;
}

bool SequenceLocks(const CTransaction &tx, int flags, std::vector<int>* prevHeights, const CBlockIndex& block)
{
    return EvaluateSequenceLocks(block, CalculateSequenceLocks(tx, flags, prevHeights, block));
}

unsigned int GetLegacySigOpCount(const CTransaction& tx)
{
    unsigned int nSigOps = 0;
    for (const auto& txin : tx.vin)
    {
        nSigOps += txin.scriptSig.GetSigOpCount(false);
    }
    for (const auto& txout : tx.vout)
    {
        nSigOps += txout.scriptPubKey.GetSigOpCount(false);
    }
    return nSigOps;
}

unsigned int GetP2SHSigOpCount(const CTransaction& tx, const CCoinsViewCache& inputs)
{
    if (tx.IsCoinBase())
        return 0;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const Coin& coin = inputs.AccessCoin(tx.vin[i].prevout);
        assert(!coin.IsSpent());
        const CTxOut &prevout = coin.out;
        if (prevout.scriptPubKey.IsPayToScriptHash())
            nSigOps += prevout.scriptPubKey.GetSigOpCount(tx.vin[i].scriptSig);
    }
    return nSigOps;
}

int64_t GetTransactionSigOpCost(const CTransaction& tx, const CCoinsViewCache& inputs, int flags)
{
    int64_t nSigOps = GetLegacySigOpCount(tx) * WITNESS_SCALE_FACTOR;

    if (tx.IsCoinBase())
        return nSigOps;

    if (flags & SCRIPT_VERIFY_P2SH) {
        nSigOps += GetP2SHSigOpCount(tx, inputs) * WITNESS_SCALE_FACTOR;
    }

    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const Coin& coin = inputs.AccessCoin(tx.vin[i].prevout);
        assert(!coin.IsSpent());
        const CTxOut &prevout = coin.out;
        nSigOps += CountWitnessSigOps(tx.vin[i].scriptSig, prevout.scriptPubKey, &tx.vin[i].scriptWitness, flags);
    }
    return nSigOps;
}

bool CheckTransaction(const CTransaction& tx, CValidationState &state, CAssetsCache* assetCache, bool fCheckDuplicateInputs, bool fMemPoolCheck)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vout-empty");
    // Size limits (this doesn't take the witness into account, as that hasn't been checked for malleability)
    if (::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * WITNESS_SCALE_FACTOR > GetMaxBlockWeight())
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-oversize");

    // Check for negative or overflow output values
    CAmount nValueOut = 0;
    for (const auto& txout : tx.vout)
    {
        if (txout.nValue < 0)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-negative");
        if (txout.nValue > MAX_MONEY)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-toolarge");
        nValueOut += txout.nValue;
        if (!MoneyRange(nValueOut))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-txouttotal-toolarge");

        /** RVN START */
        bool isAsset = false;
        int nType;
        bool fIsOwner;
        if (txout.scriptPubKey.IsAssetScript(nType, fIsOwner))
            isAsset = true;

        // Make sure that all asset tx have a nValue of zero RVN
        if (isAsset && txout.nValue != 0)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-asset-tx-amount-isn't-zero");

        if (!AreAssetsDeployed() && isAsset && !fReindex)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-is-asset-and-asset-not-active");

        // Check for transfers that don't meet the assets units only if the assetCache is not null
        if (AreAssetsDeployed() && isAsset) {
            if (assetCache) {
                // Get the transfer transaction data from the scriptPubKey
                if ( nType == TX_TRANSFER_ASSET) {
                    CAssetTransfer transfer;
                    std::string address;
                    if (!TransferAssetFromScript(txout.scriptPubKey, transfer, address))
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-asset-bad-deserialize");

                    // Check asset name validity and get type
                    AssetType assetType;
                    if (!IsAssetNameValid(transfer.strName, assetType)) {
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-asset-name-invalid");
                    }

                    // If the transfer is an ownership asset. Check to make sure that it is OWNER_ASSET_AMOUNT
                    if (IsAssetNameAnOwner(transfer.strName)) {
                        if (transfer.nAmount != OWNER_ASSET_AMOUNT)
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-owner-amount-was-not-1");
                    }

                    // If the transfer is a unique asset. Check to make sure that it is UNIQUE_ASSET_AMOUNT
                    if (assetType == AssetType::UNIQUE) {
                        if (transfer.nAmount != UNIQUE_ASSET_AMOUNT)
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-unique-amount-was-not-1");
                    }

                }
            }
        }
    }
        /** RVN END */

    // Check for duplicate inputs - note that this check is slow so we skip it in CheckBlock
    if (fCheckDuplicateInputs) {
        std::set<COutPoint> vInOutPoints;
        for (const auto& txin : tx.vin)
        {
            if (!vInOutPoints.insert(txin.prevout).second)
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-duplicate");
        }
    }

    if (tx.IsCoinBase())
    {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-length");
    }
    else
    {
        for (const auto& txin : tx.vin)
            if (txin.prevout.IsNull())
                return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");
    }

    /** RVN START */
    if (AreAssetsDeployed()) {
        if (assetCache) {
            if (tx.IsNewAsset()) {
                if(!tx.VerifyNewAsset())
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-verifying-issue-asset");

                CNewAsset asset;
                std::string strAddress;
                if (!AssetFromTransaction(tx, asset, strAddress))
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-asset");

                // Validate the new assets information
                std::string strError = "";
                if (!IsNewOwnerTxValid(tx, asset.strName, strAddress, strError))
                    return state.DoS(100, false, REJECT_INVALID, strError);

                if (!asset.IsValid(strError, *assetCache, fMemPoolCheck, fCheckDuplicateInputs))
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-" + strError);

            } else if (tx.IsReissueAsset()) {

                CReissueAsset reissue;
                std::string strAddress;
                if (!ReissueAssetFromTransaction(tx, reissue, strAddress))
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-reissue-asset");

                bool foundOwnerAsset = false;
                for (auto out : tx.vout) {
                    CAssetTransfer transfer;
                    std::string transferAddress;
                    if (TransferAssetFromScript(out.scriptPubKey, transfer, transferAddress)) {
                        if (reissue.strName + OWNER_TAG == transfer.strName) {
                            foundOwnerAsset = true;
                            break;
                        }
                    }
                }

                if (!foundOwnerAsset)
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-reissue-asset-bad-owner-asset");
            } else if (tx.IsNewUniqueAsset()) {

                std::string assetRoot = "";
                int assetCount = 0;

                for (auto out : tx.vout) {
                    CNewAsset asset;
                    std::string strAddress;

                    if (IsScriptNewUniqueAsset(out.scriptPubKey)) {
                        if (!AssetFromScript(out.scriptPubKey, asset, strAddress))
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-unique-asset");

                        std::string strError = "";
                        if (!asset.IsValid(strError, *assetCache, fMemPoolCheck, fCheckDuplicateInputs))
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-" + strError);

                        std::string root = GetParentName(asset.strName);
                        if (assetRoot.compare("") == 0)
                            assetRoot = root;
                        if (assetRoot.compare(root) != 0)
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-unique-asset-mismatched-root");

                        assetCount += 1;
                    }
                }

                if (assetCount < 1)
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-unique-asset-no-outputs");

                bool foundOwnerAsset = false;
                for (auto out : tx.vout) {
                    CAssetTransfer transfer;
                    std::string transferAddress;
                    if (TransferAssetFromScript(out.scriptPubKey, transfer, transferAddress)) {
                        if (assetRoot + OWNER_TAG == transfer.strName) {
                            foundOwnerAsset = true;
                            break;
                        }
                    }
                }

                if (!foundOwnerAsset)
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-unique-asset-bad-owner-asset");
            }
        }
    }
    /** RVN END */

    return true;
}

bool Consensus::CheckTxInputs(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight, CAmount& txfee)
{
    // are the actual inputs available?
    if (!inputs.HaveInputs(tx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-missingorspent", false,
                         strprintf("%s: inputs missing/spent", __func__));
    }

    CAmount nValueIn = 0;
    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin& coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        // If prev is coinbase, check that it's matured
        if (coin.IsCoinBase() && nSpendHeight - coin.nHeight < COINBASE_MATURITY) {
            return state.Invalid(false,
                REJECT_INVALID, "bad-txns-premature-spend-of-coinbase",
                strprintf("tried to spend coinbase at depth %d", nSpendHeight - coin.nHeight));
        }

        // Check for negative or overflow input values
        nValueIn += coin.out.nValue;
        if (!MoneyRange(coin.out.nValue) || !MoneyRange(nValueIn)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputvalues-outofrange");
        }
    }

    const CAmount value_out = tx.GetValueOut();
    if (nValueIn < value_out) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-in-belowout", false,
            strprintf("value in (%s) < value out (%s)", FormatMoney(nValueIn), FormatMoney(value_out)));
    }

    // Tally transaction fees
    const CAmount txfee_aux = nValueIn - value_out;
    if (!MoneyRange(txfee_aux)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-fee-out-of-range");
    }

    txfee = txfee_aux;
    return true;
}

//! Check to make sure that the inputs and outputs CAmount match exactly.
bool Consensus::CheckTxAssets(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, std::vector<std::pair<std::string, uint256> >& vPairReissueAssets, const bool fRunningUnitTests)
{
    // are the actual inputs available?
    if (!inputs.HaveInputs(tx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-missing-or-spent", false,
                         strprintf("%s: inputs missing/spent", __func__));
    }

    // Create map that stores the amount of an asset transaction input. Used to verify no assets are burned
    std::map<std::string, CAmount> totalInputs;

    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin& coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        if (coin.IsAsset()) {
            std::string strName;
            CAmount nAmount;

            if (!GetAssetInfoFromCoin(coin, strName, nAmount))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-failed-to-get-asset-from-script");

            // Add to the total value of assets in the inputs
            if (totalInputs.count(strName))
                totalInputs.at(strName) += nAmount;
            else
                totalInputs.insert(make_pair(strName, nAmount));
        }
    }

    // Create map that stores the amount of an asset transaction output. Used to verify no assets are burned
    std::map<std::string, CAmount> totalOutputs;

    for (const auto& txout : tx.vout) {
        if (txout.scriptPubKey.IsTransferAsset()) {
            CAssetTransfer transfer;
            std::string address;
            if (!TransferAssetFromScript(txout.scriptPubKey, transfer, address))
                return state.DoS(100, false, REJECT_INVALID, "bad-tx-asset-transfer-bad-deserialize");

            // Add to the total value of assets in the outputs
            if (totalOutputs.count(transfer.strName))
                totalOutputs.at(transfer.strName) += transfer.nAmount;
            else
                totalOutputs.insert(make_pair(transfer.strName, transfer.nAmount));

            if (!fRunningUnitTests) {
                if (IsAssetNameAnOwner(transfer.strName)) {
                    if (transfer.nAmount != OWNER_ASSET_AMOUNT)
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-owner-amount-was-not-1");
                } else {
                    // For all other types of assets, make sure they are sending the right type of units
                    CNewAsset asset;
                    if (!passets->GetAssetIfExists(transfer.strName, asset))
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-asset-not-exist");

                    if (asset.strName != transfer.strName)
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-asset-database-corrupted");

                    if (!CheckAmountWithUnits(transfer.nAmount, asset.units))
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-asset-amount-not-match-units");
                }
            }
        } else if (txout.scriptPubKey.IsReissueAsset()) {
            CReissueAsset reissue;
            std::string address;
            if (!ReissueAssetFromScript(txout.scriptPubKey, reissue, address))
                return state.DoS(100, false, REJECT_INVALID, "bad-tx-asset-reissue-bad-deserialize");

            if (!fRunningUnitTests) {
                std::string strError;
                if (!reissue.IsValid(strError, *passets)) {
                    return state.DoS(100, false, REJECT_INVALID,
                                     "bad-txns" + strError);
                }
            }

            if (mapReissuedAssets.count(reissue.strName)) {
                if (mapReissuedAssets.at(reissue.strName) != tx.GetHash())
                    return state.DoS(100, false, REJECT_INVALID, "bad-tx-reissue-chaining-not-allowed");
            } else {
                vPairReissueAssets.emplace_back(std::make_pair(reissue.strName, tx.GetHash()));
            }
        }
    }

    for (const auto& outValue : totalOutputs) {
        if (!totalInputs.count(outValue.first)) {
            std::string errorMsg;
            errorMsg = strprintf("Bad Transaction - Trying to create outpoint for asset that you don't have: %s", outValue.first);
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-inputs-outputs-mismatch " + errorMsg);
        }

        if (totalInputs.at(outValue.first) != outValue.second) {
            std::string errorMsg;
            errorMsg = strprintf("Bad Transaction - Assets would be burnt %s", outValue.first);
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-inputs-outputs-mismatch " + errorMsg);
        }
    }

    // Check the input size and the output size
    if (totalOutputs.size() != totalInputs.size()) {
        return state.DoS(100, false, REJECT_INVALID, "bad-tx-asset-inputs-size-does-not-match-outputs-size");
    }

    return true;
}
