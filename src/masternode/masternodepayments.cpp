// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/activemasternode.h>
#include <governance/governanceclasses.h>
#include <masternode/masternodepayments.h>
#include <masternode/masternodesync.h>
#include <netfulfilledman.h>
#include <netmessagemaker.h>
#include <spork.h>
#include <util/system.h>
#include <validation.h>

#include <evo/deterministicmns.h>

#include <string>

CMasternodePayments mnpayments;


/**
* IsBlockValueValid
*
*   Determine if coinbase outgoing created money is the correct value
*
*   Why is this needed?
*   - In Syscoin some blocks are superblocks, which output much higher amounts of coins
*   - Other blocks are lower in outgoing value, so in total, no extra coins are created
*   - When non-superblocks are detected, the normal schedule should be maintained
*/

bool IsBlockValueValid(const CBlock& block, int nBlockHeight, const CAmount &blockReward, std::string& strErrorRet)
{
    bool isBlockRewardValueMet = (block.vtx[0]->GetValueOut() <= blockReward);

    strErrorRet = "";
    
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

bool IsBlockPayeeValid(CChain& activeChain, const CTransaction& txNew, int nBlockHeight, const CAmount &blockReward, const CAmount &fees, CAmount& nMNSeniorityRet, CAmount& nMNFloorDiffRet)
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
    const CAmount &nHalfFee = fees / 2;

    // Check for correct masternode payment
    if(CMasternodePayments::IsTransactionValid(activeChain, txNew, nBlockHeight, blockReward, nHalfFee, nMNSeniorityRet, nMNFloorDiffRet)) {
        LogPrint(BCLog::MNPAYMENTS, "%s -- Valid masternode payment at height %d\n", __func__, nBlockHeight);
        return true;
    }
    // superblocks started
    // SEE IF THIS IS A VALID SUPERBLOCK

    if(AreSuperblocksEnabled()) {
        if(CSuperblockManager::IsSuperblockTriggered(nBlockHeight)) {
            if(CSuperblockManager::IsValid(txNew, nBlockHeight, blockReward+nMNSeniorityRet+nMNFloorDiffRet)) {
                LogPrint(BCLog::GOBJECT, "%s -- Valid superblock at height %d\n", __func__, nBlockHeight);
                // continue validation, should also pay MN
            } else {
                LogPrintf("%s -- ERROR: Invalid superblock detected at height %d\n", __func__, nBlockHeight); /* Continued */
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

    LogPrintf("%s -- ERROR: Invalid masternode payment detected at height %d\n", __func__, nBlockHeight); /* Continued */
    return false;
}

void FillBlockPayments(CChain& activeChain, CMutableTransaction& txNew, int nBlockHeight, const CAmount &blockReward, const CAmount &fees, std::vector<CTxOut>& voutMasternodePaymentsRet, std::vector<CTxOut>& voutSuperblockPaymentsRet)
{
    // only create superblocks if spork is enabled AND if superblock is actually triggered
    // (height should be validated inside)
    if(AreSuperblocksEnabled() &&
        CSuperblockManager::IsSuperblockTriggered(nBlockHeight)) {
            LogPrint(BCLog::GOBJECT, "%s -- triggered superblock creation at height %d\n", __func__, nBlockHeight);
            CSuperblockManager::GetSuperblockPayments(nBlockHeight, voutSuperblockPaymentsRet);
    }

    const CAmount &nHalfFee = fees / 2;
    if (!CMasternodePayments::GetMasternodeTxOuts(activeChain, nBlockHeight, blockReward, voutMasternodePaymentsRet, nHalfFee)) {
        LogPrint(BCLog::MNPAYMENTS, "%s -- no masternode to pay (MN list probably empty)\n", __func__);
        return;
    }
	// miner takes 25% of the reward and half fees
	txNew.vout[0].nValue = voutMasternodePaymentsRet.empty()? blockReward: (blockReward*0.25);
    if(nHalfFee > 0)
	    txNew.vout[0].nValue += nHalfFee;
    // mn is paid 75% of block reward plus any seniority
    txNew.vout.insert(txNew.vout.end(), voutMasternodePaymentsRet.begin(), voutMasternodePaymentsRet.end());
    // superblock governance amount is added as extra
    txNew.vout.insert(txNew.vout.end(), voutSuperblockPaymentsRet.begin(), voutSuperblockPaymentsRet.end());
    if(fTestSetting) {
        txNew.vout[0].nValue += 1*COIN;
    }
    std::string voutMasternodeStr;
    for (const auto& txout : voutMasternodePaymentsRet) {
        if (!voutMasternodeStr.empty())
            voutMasternodeStr += ",";
        voutMasternodeStr += txout.ToString();
    }

    LogPrint(BCLog::MNPAYMENTS, "%s -- nBlockHeight %d blockReward %lld voutMasternodePaymentsRet \"%s\"\n", __func__,
                            nBlockHeight, blockReward, voutMasternodeStr);
}

/**
*   GetMasternodeTxOuts
*
*   Get masternode payment tx outputs
*/

bool CMasternodePayments::GetMasternodeTxOuts(CChain& activeChain, int nBlockHeight, const CAmount &blockReward, std::vector<CTxOut>& voutMasternodePaymentsRet, const CAmount &nHalfFee)
{
    // make sure it's not filled yet
    voutMasternodePaymentsRet.clear();
    CAmount nMNSeniorityRet;
    CAmount nMNFloorDiffRet;
    int nCollateralHeight;
    if(!GetBlockTxOuts(activeChain, nBlockHeight, blockReward, voutMasternodePaymentsRet, nHalfFee, nMNSeniorityRet, nMNFloorDiffRet, nCollateralHeight)) {
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
CAmount GetBlockMNSubsidy(const CAmount &nBlockReward, unsigned int nHeight, const Consensus::Params& consensusParams, unsigned int nStartHeight, CAmount& nMNSeniorityRet, CAmount& nMNFloorDiffRet)
{
    // MN takes 75% of the subsidy
    CAmount nSubsidy = nBlockReward*0.75;
    
    // ensure that if subsidy is less than min amount then stick to min for long term MN full node/chainlock incentive
    const CAmount &nMinMN = consensusParams.nMinMNSubsidySats;
    if(nSubsidy < nMinMN) {
        nMNFloorDiffRet = nMinMN-nSubsidy;
        nSubsidy = nMinMN;
    }
    if (nHeight > 0 && nStartHeight > 0) {
        const double &fSubsidyAdjustmentPercentage = consensusParams.Seniority(nHeight, nStartHeight);
        if(fSubsidyAdjustmentPercentage > 0){
            nMNSeniorityRet = nSubsidy*fSubsidyAdjustmentPercentage;
            nSubsidy += nMNSeniorityRet;
        }
    }
    return nSubsidy;
}
bool CMasternodePayments::GetBlockTxOuts(CChain& activeChain, int nBlockHeight, const CAmount &blockReward, std::vector<CTxOut>& voutMasternodePaymentsRet, const CAmount &nHalfFee, CAmount& nMNSeniorityRet, CAmount &nMNFloorDiffRet, int& nCollateralHeightRet)
{
    voutMasternodePaymentsRet.clear();
    CDeterministicMNCPtr dmnPayee;
    {
        LOCK(cs_main);
        const CBlockIndex* pindex = activeChain[nBlockHeight - 1];
        if(!pindex)
            return false;
        dmnPayee = deterministicMNManager->GetListForBlock(pindex).GetMNPayee();
        if (!dmnPayee) {
            return false;
        }
    }
    nCollateralHeightRet = dmnPayee->pdmnState->nCollateralHeight;
    CAmount masternodeReward = GetBlockMNSubsidy(blockReward, nBlockHeight, Params().GetConsensus(), nCollateralHeightRet, nMNSeniorityRet, nMNFloorDiffRet) + nHalfFee;

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

bool CMasternodePayments::IsTransactionValid(CChain& activeChain, const CTransaction& txNew, int nBlockHeight, const CAmount &blockReward, const CAmount& nHalfFee, CAmount& nMNSeniorityRet, CAmount &nMNFloorDiffRet)
{
    if (!deterministicMNManager || !deterministicMNManager->IsDIP3Enforced(nBlockHeight)) {
        // can't verify historical blocks here
        return true;
    }

    std::vector<CTxOut> voutMasternodePayments;
    int nCollateralHeight;
    if (!GetBlockTxOuts(activeChain, nBlockHeight, blockReward, voutMasternodePayments, nHalfFee, nMNSeniorityRet, nMNFloorDiffRet, nCollateralHeight)) {
        LogPrintf("CMasternodePayments::%s -- ERROR failed to get payees for block at height %s\n", __func__, nBlockHeight);
        return true;
    }

    for (const auto& txout : voutMasternodePayments) {
        bool found = ranges::any_of(txNew.vout, [&txout](const auto& txout2) {return txout == txout2;});
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
