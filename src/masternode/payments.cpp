// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/payments.h>

#include <chain.h>
#include <consensus/amount.h>
#include <deploymentstatus.h>
#include <evo/deterministicmns.h>
#include <governance/classes.h>
#include <governance/governance.h>
#include <key_io.h>
#include <logging.h>
#include <masternode/sync.h>
#include <primitives/block.h>
#include <script/standard.h>
#include <tinyformat.h>
#include <util/ranges.h>
#include <validation.h>

#include <cassert>
#include <string>

CAmount PlatformShare(const CAmount reward)
{
    const CAmount platformReward = reward * 375 / 1000;
    bool ok = MoneyRange(platformReward);
    assert(ok);
    return platformReward;
}

[[nodiscard]] bool CMNPaymentsProcessor::GetBlockTxOuts(const CBlockIndex* pindexPrev, const CAmount blockSubsidy, const CAmount feeReward,
                                                        std::vector<CTxOut>& voutMasternodePaymentsRet)
{
    voutMasternodePaymentsRet.clear();

    const int nBlockHeight = pindexPrev  == nullptr ? 0 : pindexPrev->nHeight + 1;

    bool fV20Active = DeploymentActiveAfter(pindexPrev, m_consensus_params, Consensus::DEPLOYMENT_V20);
    CAmount masternodeReward = GetMasternodePayment(nBlockHeight, blockSubsidy + feeReward, fV20Active);

    // Credit Pool doesn't exist before V20. If any part of reward will re-allocated to credit pool before v20
    // activation these fund will be just permanently lost. Applyable for devnets, regtest, testnet
    if (fV20Active && DeploymentActiveAfter(pindexPrev, m_consensus_params, Consensus::DEPLOYMENT_MN_RR)) {
        CAmount masternodeSubsidyReward = GetMasternodePayment(nBlockHeight, blockSubsidy, fV20Active);
        const CAmount platformReward = PlatformShare(masternodeSubsidyReward);
        masternodeReward -= platformReward;

        assert(MoneyRange(masternodeReward));

        LogPrint(BCLog::MNPAYMENTS, "CMNPaymentsProcessor::%s -- MN reward %lld reallocated to credit pool\n", __func__, platformReward);
        voutMasternodePaymentsRet.emplace_back(platformReward, CScript() << OP_RETURN);
    }
    const auto mnList = m_dmnman.GetListForBlock(pindexPrev);
    if (mnList.GetAllMNsCount() == 0) {
        LogPrint(BCLog::MNPAYMENTS, "CMNPaymentsProcessor::%s -- no masternode registered to receive a payment\n", __func__);
        return true;
    }
    const auto dmnPayee = mnList.GetMNPayee(pindexPrev);
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

/**
*   GetMasternodeTxOuts
*
*   Get masternode payment tx outputs
*/
[[nodiscard]] bool CMNPaymentsProcessor::GetMasternodeTxOuts(const CBlockIndex* pindexPrev, const CAmount blockSubsidy, const CAmount feeReward,
                                                             std::vector<CTxOut>& voutMasternodePaymentsRet)
{
    // make sure it's not filled yet
    voutMasternodePaymentsRet.clear();

    if(!GetBlockTxOuts(pindexPrev, blockSubsidy, feeReward, voutMasternodePaymentsRet)) {
        LogPrintf("CMNPaymentsProcessor::%s -- ERROR Failed to get payee\n", __func__);
        return false;
    }

    for (const auto& txout : voutMasternodePaymentsRet) {
        CTxDestination dest;
        ExtractDestination(txout.scriptPubKey, dest);

        LogPrintf("CMNPaymentsProcessor::%s -- Masternode payment %lld to %s\n", __func__, txout.nValue, EncodeDestination(dest));
    }

    return true;
}

[[nodiscard]] bool CMNPaymentsProcessor::IsTransactionValid(const CTransaction& txNew, const CBlockIndex* pindexPrev, const CAmount blockSubsidy,
                                                            const CAmount feeReward)
{
    const int nBlockHeight = pindexPrev  == nullptr ? 0 : pindexPrev->nHeight + 1;
    if (!DeploymentDIP0003Enforced(nBlockHeight, m_consensus_params)) {
        // can't verify historical blocks here
        return true;
    }

    std::vector<CTxOut> voutMasternodePayments;
    if (!GetBlockTxOuts(pindexPrev, blockSubsidy, feeReward, voutMasternodePayments)) {
        LogPrintf("CMNPaymentsProcessor::%s -- ERROR! Failed to get payees for block at height %s\n", __func__, nBlockHeight);
        return true;
    }

    for (const auto& txout : voutMasternodePayments) {
        bool found = ranges::any_of(txNew.vout, [&txout](const auto& txout2) {return txout == txout2;});
        if (!found) {
            std::string str_payout;
            if (CTxDestination dest; ExtractDestination(txout.scriptPubKey, dest)) {
                str_payout = "address=" + EncodeDestination(dest);
            } else {
                str_payout = "scriptPubKey=" + HexStr(txout.scriptPubKey);
            }
            LogPrintf("CMNPaymentsProcessor::%s -- ERROR! Failed to find expected payee %s amount=%lld height=%d\n",
                      __func__, str_payout, txout.nValue, nBlockHeight);
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool CMNPaymentsProcessor::IsOldBudgetBlockValueValid(const CBlock& block, const int nBlockHeight, const CAmount blockReward, std::string& strErrorRet)
{
    bool isBlockRewardValueMet = (block.vtx[0]->GetValueOut() <= blockReward);

    if (nBlockHeight < m_consensus_params.nBudgetPaymentsStartBlock) {
        strErrorRet = strprintf("Incorrect block %d, old budgets are not activated yet", nBlockHeight);
        return false;
    }

    if (nBlockHeight >= m_consensus_params.nSuperblockStartBlock) {
        strErrorRet = strprintf("Incorrect block %d, old budgets are no longer active", nBlockHeight);
        return false;
    }

    // we are still using budgets, but we have no data about them anymore,
    // all we know is predefined budget cycle and window

    int nOffset = nBlockHeight % m_consensus_params.nBudgetPaymentsCycleBlocks;
    if (nOffset < m_consensus_params.nBudgetPaymentsWindowBlocks) {
        // NOTE: old budget system is disabled since 12.1
        if(m_mn_sync.IsSynced()) {
            // no old budget blocks should be accepted here on mainnet,
            // testnet/devnet/regtest should produce regular blocks only
            LogPrint(BCLog::GOBJECT, "CMNPaymentsProcessor::%s -- WARNING! Client synced but old budget system is disabled, checking block value against block reward\n", __func__);
            if(!isBlockRewardValueMet) {
                strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, old budgets are disabled",
                                        nBlockHeight, block.vtx[0]->GetValueOut(), blockReward);
            }
            return isBlockRewardValueMet;
        }
        // when not synced, rely on online nodes (all networks)
        LogPrint(BCLog::GOBJECT, "CMNPaymentsProcessor::%s -- WARNING! Skipping old budget block value checks, accepting block\n", __func__);
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
bool CMNPaymentsProcessor::IsBlockValueValid(const CBlock& block, const int nBlockHeight, const CAmount blockReward, std::string& strErrorRet, const bool check_superblock)
{
    bool isBlockRewardValueMet = (block.vtx[0]->GetValueOut() <= blockReward);

    strErrorRet = "";

    if (nBlockHeight < m_consensus_params.nBudgetPaymentsStartBlock) {
        // old budget system is not activated yet, just make sure we do not exceed the regular block reward
        if(!isBlockRewardValueMet) {
            strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, old budgets are not activated yet",
                                    nBlockHeight, block.vtx[0]->GetValueOut(), blockReward);
        }
        return isBlockRewardValueMet;
    } else if (nBlockHeight < m_consensus_params.nSuperblockStartBlock) {
        // superblocks are not enabled yet, check if we can pass old budget rules
        return IsOldBudgetBlockValueValid(block, nBlockHeight, blockReward, strErrorRet);
    }

    LogPrint(BCLog::MNPAYMENTS, "block.vtx[0]->GetValueOut() %lld <= blockReward %lld\n", block.vtx[0]->GetValueOut(), blockReward);

    CAmount nSuperblockMaxValue =  blockReward + CSuperblock::GetPaymentsLimit(m_chainman.ActiveChain(), nBlockHeight);
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

    if (!m_mn_sync.IsSynced() || !m_govman.IsValid()) {
        LogPrint(BCLog::MNPAYMENTS, "CMNPaymentsProcessor::%s -- WARNING! Not enough data, checked superblock max bounds only\n", __func__);
        // not enough data for full checks but at least we know that the superblock limits were honored.
        // We rely on the network to have followed the correct chain in this case
        return true;
    }

    // we are synced and possibly on a superblock now

    if (!AreSuperblocksEnabled(m_sporkman)) {
        // should NOT allow superblocks at all, when superblocks are disabled
        // revert to block reward limits in this case
        LogPrint(BCLog::GOBJECT, "CMNPaymentsProcessor::%s -- Superblocks are disabled, no superblocks allowed\n", __func__);
        if(!isBlockRewardValueMet) {
            strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, superblocks are disabled",
                                    nBlockHeight, block.vtx[0]->GetValueOut(), blockReward);
        }
        return isBlockRewardValueMet;
    }

    if (!check_superblock) return true;

    const auto tip_mn_list = m_dmnman.GetListAtChainTip();

    if (!m_govman.IsSuperblockTriggered(tip_mn_list, nBlockHeight)) {
        // we are on a valid superblock height but a superblock was not triggered
        // revert to block reward limits in this case
        if(!isBlockRewardValueMet) {
            strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, no triggered superblock detected",
                                    nBlockHeight, block.vtx[0]->GetValueOut(), blockReward);
        }
        return isBlockRewardValueMet;
    }

    // this actually also checks for correct payees and not only amount
    if (!m_govman.IsValidSuperblock(m_chainman.ActiveChain(), tip_mn_list, *block.vtx[0], nBlockHeight, blockReward)) {
        // triggered but invalid? that's weird
        LogPrintf("CMNPaymentsProcessor::%s -- ERROR! Invalid superblock detected at height %d: %s", __func__, nBlockHeight, block.vtx[0]->ToString()); /* Continued */
        // should NOT allow invalid superblocks, when superblocks are enabled
        strErrorRet = strprintf("invalid superblock detected at height %d", nBlockHeight);
        return false;
    }

    // we got a valid superblock
    return true;
}

bool CMNPaymentsProcessor::IsBlockPayeeValid(const CTransaction& txNew, const CBlockIndex* pindexPrev, const CAmount blockSubsidy, const CAmount feeReward, const bool check_superblock)
{
    const int nBlockHeight = pindexPrev  == nullptr ? 0 : pindexPrev->nHeight + 1;

    // Check for correct masternode payment
    if (IsTransactionValid(txNew, pindexPrev, blockSubsidy, feeReward)) {
        LogPrint(BCLog::MNPAYMENTS, "CMNPaymentsProcessor::%s -- Valid masternode payment at height %d: %s", __func__, nBlockHeight, txNew.ToString()); /* Continued */
    } else {
        LogPrintf("CMNPaymentsProcessor::%s -- ERROR! Invalid masternode payment detected at height %d: %s", __func__, nBlockHeight, txNew.ToString()); /* Continued */
        return false;
    }

    if (!m_mn_sync.IsSynced() || !m_govman.IsValid()) {
        // governance data is either incomplete or non-existent
        LogPrint(BCLog::MNPAYMENTS, "CMNPaymentsProcessor::%s -- WARNING! Not enough data, skipping superblock payee checks\n", __func__);
        return true;  // not an error
    }

    if (nBlockHeight < m_consensus_params.nSuperblockStartBlock) {
        // We are still using budgets, but we have no data about them anymore,
        // we can only check masternode payments.
        // NOTE: old budget system is disabled since 12.1 and we should never enter this branch
        // anymore when sync is finished (on mainnet). We have no old budget data but these blocks
        // have tons of confirmations and can be safely accepted without payee verification
        LogPrint(BCLog::GOBJECT, "CMNPaymentsProcessor::%s -- WARNING! Client synced but old budget system is disabled, accepting any payee\n", __func__);
        return true; // not an error
    }

    // superblocks started

    if (AreSuperblocksEnabled(m_sporkman)) {
        if (!check_superblock) return true;
        const auto tip_mn_list = m_dmnman.GetListAtChainTip();
        if (m_govman.IsSuperblockTriggered(tip_mn_list, nBlockHeight)) {
            if (m_govman.IsValidSuperblock(m_chainman.ActiveChain(), tip_mn_list, txNew, nBlockHeight,
                                           blockSubsidy + feeReward)) {
                LogPrint(BCLog::GOBJECT, "CMNPaymentsProcessor::%s -- Valid superblock at height %d: %s", __func__, nBlockHeight, txNew.ToString()); /* Continued */
                // continue validation, should also pay MN
            } else {
                LogPrintf("CMNPaymentsProcessor::%s -- ERROR! Invalid superblock detected at height %d: %s", __func__, nBlockHeight, txNew.ToString()); /* Continued */
                // should NOT allow such superblocks, when superblocks are enabled
                return false;
            }
        } else {
            LogPrint(BCLog::GOBJECT, "CMNPaymentsProcessor::%s -- No triggered superblock detected at height %d\n", __func__, nBlockHeight);
        }
    } else {
        // should NOT allow superblocks at all, when superblocks are disabled
        LogPrint(BCLog::GOBJECT, "CMNPaymentsProcessor::%s -- Superblocks are disabled, no superblocks allowed\n", __func__);
    }

    return true;
}

void CMNPaymentsProcessor::FillBlockPayments(CMutableTransaction& txNew, const CBlockIndex* pindexPrev, const CAmount blockSubsidy, const CAmount feeReward,
                                             std::vector<CTxOut>& voutMasternodePaymentsRet, std::vector<CTxOut>& voutSuperblockPaymentsRet)
{
    int nBlockHeight = pindexPrev == nullptr ? 0 : pindexPrev->nHeight + 1;

    // only create superblocks if spork is enabled AND if superblock is actually triggered
    // (height should be validated inside)
    const auto tip_mn_list = m_dmnman.GetListAtChainTip();
    if (AreSuperblocksEnabled(m_sporkman) && m_govman.IsSuperblockTriggered(tip_mn_list, nBlockHeight)) {
        LogPrint(BCLog::GOBJECT, "CMNPaymentsProcessor::%s -- Triggered superblock creation at height %d\n", __func__, nBlockHeight);
        m_govman.GetSuperblockPayments(tip_mn_list, nBlockHeight, voutSuperblockPaymentsRet);
    }

    if (!GetMasternodeTxOuts(pindexPrev, blockSubsidy, feeReward, voutMasternodePaymentsRet)) {
        LogPrint(BCLog::MNPAYMENTS, "CMNPaymentsProcessor::%s -- No masternode to pay (MN list probably empty)\n", __func__);
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

    LogPrint(BCLog::MNPAYMENTS, "CMNPaymentsProcessor::%s -- nBlockHeight %d blockReward %lld voutMasternodePaymentsRet \"%s\" txNew %s", __func__, /* Continued */
                            nBlockHeight, blockSubsidy + feeReward, voutMasternodeStr, txNew.ToString());
}
