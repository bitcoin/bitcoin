// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/activemasternode.h>
#include <governance/governance-classes.h>
#include <masternode/masternode-payments.h>
#include <masternode/masternode-sync.h>
#include <evo/deterministicmns.h>
#include <netfulfilledman.h>
#include <netmessagemaker.h>
#include <validation.h>

#include <string>

CMasternodePayments mnpayments;

bool IsOldBudgetBlockValueValid(const CBlock& block, int nBlockHeight, CAmount blockReward, std::string& strErrorRet) {
    const Consensus::Params& consensusParams = Params().GetConsensus();
    bool isBlockRewardValueMet = (block.vtx[0]->GetValueOut() <= blockReward);

    if (nBlockHeight < consensusParams.nBudgetPaymentsStartBlock) {
        strErrorRet = strprintf("Incorrect block %d, old budgets are not activated yet", nBlockHeight);
        return false;
    }

    if (nBlockHeight >= consensusParams.nSuperblockStartBlock) {
        strErrorRet = strprintf("Incorrect block %d, old budgets are no longer active", nBlockHeight);
        return false;
    }

    // we are still using budgets, but we have no data about them anymore,
    // all we know is predefined budget cycle and window

    int nOffset = nBlockHeight % consensusParams.nBudgetPaymentsCycleBlocks;
    if(nBlockHeight >= consensusParams.nBudgetPaymentsStartBlock &&
       nOffset < consensusParams.nBudgetPaymentsWindowBlocks) {
        // NOTE: old budget system is disabled since 12.1
        if(masternodeSync.IsSynced()) {
            // no old budget blocks should be accepted here on mainnet,
            // testnet/devnet/regtest should produce regular blocks only
            LogPrint(BCLog::GOBJECT, "%s -- WARNING: Client synced but old budget system is disabled, checking block value against block reward\n", __func__);
            if(!isBlockRewardValueMet) {
                strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, old budgets are disabled",
                                        nBlockHeight, block.vtx[0]->GetValueOut(), blockReward);
            }
            return isBlockRewardValueMet;
        }
        // when not synced, rely on online nodes (all networks)
        LogPrint(BCLog::GOBJECT, "%s -- WARNING: Skipping old budget block value checks, accepting block\n", __func__);
        return true;
    }
    if(!isBlockRewardValueMet) {
        strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, block is not in old budget cycle window",
                                nBlockHeight, block.vtx[0]->GetValueOut(), blockReward);
    }
    return isBlockRewardValueMet;
}

/**
* IsBlockValueValid
*
*   Determine if coinbase outgoing created money is the correct value
*
*   Why is this needed?
*   - In Dash some blocks are superblocks, which output much higher amounts of coins
*   - Other blocks are 10% lower in outgoing value, so in total, no extra coins are created
*   - When non-superblocks are detected, the normal schedule should be maintained
*/

bool IsBlockValueValid(const CBlock& block, int nBlockHeight, CAmount blockReward, std::string& strErrorRet)
{
    const Consensus::Params& consensusParams = Params().GetConsensus();
    bool isBlockRewardValueMet = (block.vtx[0]->GetValueOut() <= blockReward);

    strErrorRet = "";

    if (nBlockHeight < consensusParams.nBudgetPaymentsStartBlock) {
        // old budget system is not activated yet, just make sure we do not exceed the regular block reward
        if(!isBlockRewardValueMet) {
            strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, old budgets are not activated yet",
                                    nBlockHeight, block.vtx[0]->GetValueOut(), blockReward);
        }
        return isBlockRewardValueMet;
    } else if (nBlockHeight < consensusParams.nSuperblockStartBlock) {
        // superblocks are not enabled yet, check if we can pass old budget rules
        return IsOldBudgetBlockValueValid(block, nBlockHeight, blockReward, strErrorRet);
    }

    LogPrint(BCLog::MNPAYMENTS, "block.vtx[0]->GetValueOut() %lld <= blockReward %lld\n", block.vtx[0]->GetValueOut(), blockReward);

    CAmount nSuperblockMaxValue =  blockReward + CSuperblock::GetPaymentsLimit(nBlockHeight);
    bool isSuperblockMaxValueMet = (block.vtx[0]->GetValueOut() <= nSuperblockMaxValue);

    LogPrint(BCLog::GOBJECT, "block.vtx[0]->GetValueOut() %lld <= nSuperblockMaxValue %lld\n", block.vtx[0]->GetValueOut(), nSuperblockMaxValue);

    if (!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        // can't possibly be a superblock, so lets just check for block reward limits
        if (!isBlockRewardValueMet) {
            strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, only regular blocks are allowed at this height",
                                    nBlockHeight, block.vtx[0]->GetValueOut(), blockReward);
        }
        return isBlockRewardValueMet;
    }

    // bail out in case superblock limits were exceeded
    if (!isSuperblockMaxValueMet) {
        strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded superblock max value",
                                nBlockHeight, block.vtx[0]->GetValueOut(), nSuperblockMaxValue);
        return false;
    }

    if(!masternodeSync.IsSynced() || fDisableGovernance) {
        LogPrint(BCLog::MNPAYMENTS, "%s -- WARNING: Not enough data, checked superblock max bounds only\n", __func__);
        // not enough data for full checks but at least we know that the superblock limits were honored.
        // We rely on the network to have followed the correct chain in this case
        return true;
    }

    // we are synced and possibly on a superblock now

    if (!AreSuperblocksEnabled()) {
        // should NOT allow superblocks at all, when superblocks are disabled
        // revert to block reward limits in this case
        LogPrint(BCLog::GOBJECT, "%s -- Superblocks are disabled, no superblocks allowed\n", __func__);
        if(!isBlockRewardValueMet) {
            strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, superblocks are disabled",
                                    nBlockHeight, block.vtx[0]->GetValueOut(), blockReward);
        }
        return isBlockRewardValueMet;
    }

    if (!CSuperblockManager::IsSuperblockTriggered(nBlockHeight)) {
        // we are on a valid superblock height but a superblock was not triggered
        // revert to block reward limits in this case
        if(!isBlockRewardValueMet) {
            strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, no triggered superblock detected",
                                    nBlockHeight, block.vtx[0]->GetValueOut(), blockReward);
        }
        return isBlockRewardValueMet;
    }

    // this actually also checks for correct payees and not only amount
    if (!CSuperblockManager::IsValid(*block.vtx[0], nBlockHeight, blockReward)) {
        // triggered but invalid? that's weird
        LogPrintf("%s -- ERROR: Invalid superblock detected at height %d: %s", __func__, nBlockHeight, block.vtx[0]->ToString()); /* Continued */
        // should NOT allow invalid superblocks, when superblocks are enabled
        strErrorRet = strprintf("invalid superblock detected at height %d", nBlockHeight);
        return false;
    }

    // we got a valid superblock
    return true;
}

bool IsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight, CAmount blockReward)
{
    if(fDisableGovernance) {
        //there is no budget data to use to check anything, let's just accept the longest chain
        LogPrint(BCLog::MNPAYMENTS, "%s -- WARNING: Not enough data, skipping block payee checks\n", __func__);
        return true;
    }

    // we are still using budgets, but we have no data about them anymore,
    // we can only check masternode payments

    const Consensus::Params& consensusParams = Params().GetConsensus();

    if(nBlockHeight < consensusParams.nSuperblockStartBlock) {
        // NOTE: old budget system is disabled since 12.1 and we should never enter this branch
        // anymore when sync is finished (on mainnet). We have no old budget data but these blocks
        // have tons of confirmations and can be safely accepted without payee verification
        LogPrint(BCLog::GOBJECT, "%s -- WARNING: Client synced but old budget system is disabled, accepting any payee\n", __func__);
        return true;
    }

    // superblocks started
    // SEE IF THIS IS A VALID SUPERBLOCK

    if(AreSuperblocksEnabled()) {
        if(CSuperblockManager::IsSuperblockTriggered(nBlockHeight)) {
            if(CSuperblockManager::IsValid(txNew, nBlockHeight, blockReward)) {
                LogPrint(BCLog::GOBJECT, "%s -- Valid superblock at height %d: %s", __func__, nBlockHeight, txNew.ToString()); /* Continued */
                // continue validation, should also pay MN
            } else {
                LogPrintf("%s -- ERROR: Invalid superblock detected at height %d: %s", __func__, nBlockHeight, txNew.ToString()); /* Continued */
                // should NOT allow such superblocks, when superblocks are enabled
                return false;
            }
        } else {
            LogPrint(BCLog::GOBJECT, "%s -- No triggered superblock detected at height %d\n", __func__, nBlockHeight);
        }
    } else {
        // should NOT allow superblocks at all, when superblocks are disabled
        LogPrint(BCLog::GOBJECT, "%s -- Superblocks are disabled, no superblocks allowed\n", __func__);
    }

    // Check for correct masternode payment
    if(CMasternodePayments::IsTransactionValid(txNew, nBlockHeight, blockReward)) {
        LogPrint(BCLog::MNPAYMENTS, "%s -- Valid masternode payment at height %d: %s", __func__, nBlockHeight, txNew.ToString()); /* Continued */
        return true;
    }

    LogPrintf("%s -- ERROR: Invalid masternode payment detected at height %d: %s", __func__, nBlockHeight, txNew.ToString()); /* Continued */
    return false;
}

void FillBlockPayments(CMutableTransaction& txNew, int nBlockHeight, CAmount blockReward, std::vector<CTxOut>& voutMasternodePaymentsRet, std::vector<CTxOut>& voutSuperblockPaymentsRet)
{
    // only create superblocks if spork is enabled AND if superblock is actually triggered
    // (height should be validated inside)
    if(AreSuperblocksEnabled() && CSuperblockManager::IsSuperblockTriggered(nBlockHeight)) {
        LogPrint(BCLog::GOBJECT, "%s -- triggered superblock creation at height %d\n", __func__, nBlockHeight);
        CSuperblockManager::GetSuperblockPayments(nBlockHeight, voutSuperblockPaymentsRet);
    }

    if (!CMasternodePayments::GetMasternodeTxOuts(nBlockHeight, blockReward, voutMasternodePaymentsRet)) {
        LogPrint(BCLog::MNPAYMENTS, "%s -- no masternode to pay (MN list probably empty)\n", __func__);
    }

    txNew.vout.insert(txNew.vout.end(), voutMasternodePaymentsRet.begin(), voutMasternodePaymentsRet.end());
    txNew.vout.insert(txNew.vout.end(), voutSuperblockPaymentsRet.begin(), voutSuperblockPaymentsRet.end());

    std::string voutMasternodeStr;
    for (const auto& txout : voutMasternodePaymentsRet) {
        // subtract MN payment from miner reward
        txNew.vout[0].nValue -= txout.nValue;
        if (!voutMasternodeStr.empty())
            voutMasternodeStr += ",";
        voutMasternodeStr += txout.ToString();
    }

    LogPrint(BCLog::MNPAYMENTS, "%s -- nBlockHeight %d blockReward %lld voutMasternodePaymentsRet \"%s\" txNew %s", __func__, /* Continued */
                            nBlockHeight, blockReward, voutMasternodeStr, txNew.ToString());
}

/**
*   GetMasternodeTxOuts
*
*   Get masternode payment tx outputs
*/

bool CMasternodePayments::GetMasternodeTxOuts(int nBlockHeight, CAmount blockReward, std::vector<CTxOut>& voutMasternodePaymentsRet)
{
    // make sure it's not filled yet
    voutMasternodePaymentsRet.clear();

    if(!GetBlockTxOuts(nBlockHeight, blockReward, voutMasternodePaymentsRet)) {
        LogPrintf("CMasternodePayments::%s -- no payee (deterministic masternode list empty)\n", __func__);
        return false;
    }

    for (const auto& txout : voutMasternodePaymentsRet) {
        CTxDestination dest;
        ExtractDestination(txout.scriptPubKey, dest);

        LogPrintf("CMasternodePayments::%s -- Masternode payment %lld to %s\n", __func__, txout.nValue, EncodeDestination(dest));
    }

    return true;
}

bool CMasternodePayments::GetBlockTxOuts(int nBlockHeight, CAmount blockReward, std::vector<CTxOut>& voutMasternodePaymentsRet)
{
    voutMasternodePaymentsRet.clear();

    const CBlockIndex* pindex;
    int nReallocActivationHeight{std::numeric_limits<int>::max()};

    {
        LOCK(cs_main);
        pindex = chainActive[nBlockHeight - 1];

        const Consensus::Params& consensusParams = Params().GetConsensus();
        if (VersionBitsState(pindex, consensusParams, Consensus::DEPLOYMENT_REALLOC, versionbitscache) == ThresholdState::ACTIVE) {
            nReallocActivationHeight = VersionBitsStateSinceHeight(pindex, consensusParams, Consensus::DEPLOYMENT_REALLOC, versionbitscache);
        }
    }

    CAmount masternodeReward = GetMasternodePayment(nBlockHeight, blockReward, nReallocActivationHeight);

    auto dmnPayee = deterministicMNManager->GetListForBlock(pindex).GetMNPayee();
    if (!dmnPayee) {
        return false;
    }

    CAmount operatorReward = 0;
    if (dmnPayee->nOperatorReward != 0 && dmnPayee->pdmnState->scriptOperatorPayout != CScript()) {
        // This calculation might eventually turn out to result in 0 even if an operator reward percentage is given.
        // This will however only happen in a few years when the block rewards drops very low.
        operatorReward = (masternodeReward * dmnPayee->nOperatorReward) / 10000;
        masternodeReward -= operatorReward;
    }

    if (masternodeReward > 0) {
        voutMasternodePaymentsRet.emplace_back(masternodeReward, dmnPayee->pdmnState->scriptPayout);
    }
    if (operatorReward > 0) {
        voutMasternodePaymentsRet.emplace_back(operatorReward, dmnPayee->pdmnState->scriptOperatorPayout);
    }

    return true;
}

bool CMasternodePayments::IsTransactionValid(const CTransaction& txNew, int nBlockHeight, CAmount blockReward)
{
    if (!deterministicMNManager->IsDIP3Enforced(nBlockHeight)) {
        // can't verify historical blocks here
        return true;
    }

    std::vector<CTxOut> voutMasternodePayments;
    if (!GetBlockTxOuts(nBlockHeight, blockReward, voutMasternodePayments)) {
        LogPrintf("CMasternodePayments::%s -- ERROR failed to get payees for block at height %s\n", __func__, nBlockHeight);
        return true;
    }

    for (const auto& txout : voutMasternodePayments) {
        bool found = false;
        for (const auto& txout2 : txNew.vout) {
            if (txout == txout2) {
                found = true;
                break;
            }
        }
        if (!found) {
            CTxDestination dest;
            if (!ExtractDestination(txout.scriptPubKey, dest))
                assert(false);
            LogPrintf("CMasternodePayments::%s -- ERROR failed to find expected payee %s in block at height %s\n", __func__, EncodeDestination(dest), nBlockHeight);
            return false;
        }
    }
    return true;
}
