// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "init.h"

#include "masternode-budget.h"
#include "masternode.h"
#include "legacysigner.h"
#include "masternodeman.h"
#include "masternode-sync.h"
#include "util.h"
#include "addrman.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <fstream>

CBudgetManager budget;
CCriticalSection cs_budget;

std::map<uint256, int64_t> askedForSourceProposalOrBudget;
std::vector<CBudgetProposalBroadcast> vecImmatureBudgetProposals;
std::vector<CFinalizedBudgetBroadcast> vecImmatureFinalizedBudgets;

namespace
{
    CBitcoinAddress ScriptToAddress(const CScript& script)
    {
        CTxDestination destination;
        ExtractDestination(script, destination);
        return destination;
    }

    void DebugLogBudget(
        int64_t currentTime,
        int64_t objectTime,
        const std::string& from,
        const std::string& proposalHash,
        const std::string& voteHash,
        const std::string& opcode
    )
    {
        if(GetArg("-budgetdebug", "") != "true")
            return;

        const boost::filesystem::path filename = GetDataDir() / "budgetdebuglog.csv";

        std::ofstream log(filename.string().c_str(), std::ios_base::out | std::ios_base::app);

        log << currentTime << "\t";
        log << objectTime << "\t";
        log << from << "\t";
        log << proposalHash << "\t";
        log << voteHash << "\t";
        log << opcode << std::endl;
    }

    void DebugLogBudget(const CBudgetVote& vote, const CAddress& from, const std::string& opcode)
    {
        DebugLogBudget(GetTime(), vote.nTime, from.ToString(), vote.nProposalHash.ToString(), vote.GetHash().ToString(), opcode);
    }

    void DebugLogBudget(const CBudgetProposal& proposal, const CAddress& from, const std::string& opcode)
    {
        DebugLogBudget(GetTime(), proposal.nTime, from.ToString(), proposal.GetHash().ToString(), "", opcode);
    }
}

CAmount BlocksBeforeSuperblockToSubmitFinalBudget()
{
    assert(GetBudgetPaymentCycleBlocks() > 10);

    // Relatively 43200 / 30 = 1440, for testnet  - equal to budget payment cycle

    if (Params().NetworkID() == CBaseChainParams::MAIN)
        return 1440 * 2;   // aprox 2 days
    else
        return GetBudgetPaymentCycleBlocks() - 10; // 40 blocks for 50-block cycle

}

int GetBudgetPaymentCycleBlocks()
{
    // Amount of blocks in a months period of time (using 1 minutes per) = (60*24*30)/1

    if(Params().NetworkID() == CBaseChainParams::MAIN)
        return 43200;
    else
        return 50; //for testing purposes
}

CAmount GetVotingThreshold()
{
    if (Params().NetworkID() == CBaseChainParams::MAIN)
        return BlocksBeforeSuperblockToSubmitFinalBudget();
    else
        return BlocksBeforeSuperblockToSubmitFinalBudget() / 4; // 10 blocks for 50-block cycle
}

int GetNextSuperblock(int height)
{
    return height - height % GetBudgetPaymentCycleBlocks() + GetBudgetPaymentCycleBlocks();
}

bool IsBudgetCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf)
{
    CTransaction txCollateral;
    uint256 nBlockHash;
    if(!GetTransaction(nTxCollateralHash, txCollateral, nBlockHash, true)){
        strError = strprintf("Can't find collateral tx %s", txCollateral.ToString());
        LogPrintf ("CBudgetProposalBroadcast::IsBudgetCollateralValid - %s\n", strError);
        return false;
    }

    if(txCollateral.vout.size() < 1) return false;
    if(txCollateral.nLockTime != 0) return false;

    CScript findScript;
    findScript << OP_RETURN << ToByteVector(nExpectedHash);

    bool foundOpReturn = false;
    BOOST_FOREACH(const CTxOut o, txCollateral.vout){
        if(!o.scriptPubKey.IsNormalPaymentScript() && !o.scriptPubKey.IsUnspendable()){
            strError = strprintf("Invalid Script %s", txCollateral.ToString());
            LogPrintf ("CBudgetProposalBroadcast::IsBudgetCollateralValid - %s\n", strError);
            return false;
        }
        if(o.scriptPubKey == findScript && o.nValue >= BUDGET_FEE_TX) foundOpReturn = true;

    }
    if(!foundOpReturn){
        strError = strprintf("Couldn't find opReturn %s in %s", nExpectedHash.ToString(), txCollateral.ToString());
        LogPrintf ("CBudgetProposalBroadcast::IsBudgetCollateralValid - %s\n", strError);
        return false;
    }

    // RETRIEVE CONFIRMATIONS AND NTIME
    /*
        - nTime starts as zero and is passed-by-reference out of this function and stored in the external proposal
        - nTime is never validated via the hashing mechanism and comes from a full-validated source (the blockchain)
    */

    int conf = GetIXConfirmations(nTxCollateralHash);
    if (nBlockHash != uint256()) {
        BlockMap::iterator mi = mapBlockIndex.find(nBlockHash);
        if (mi != mapBlockIndex.end() && (*mi).second) {
            CBlockIndex* pindex = (*mi).second;
            if (chainActive.Contains(pindex)) {
                conf += chainActive.Height() - pindex->nHeight + 1;
                nTime = pindex->nTime;
            }
        }
    }

    nConf = conf;

    //if we're syncing we won't have instantX information, so accept 1 confirmation 
    if(conf >= BUDGET_FEE_CONFIRMATIONS){
        return true;
    } else {
        strError = strprintf("Collateral requires at least %d confirmations - %d confirmations", BUDGET_FEE_CONFIRMATIONS, conf);
        LogPrintf ("CBudgetProposalBroadcast::IsBudgetCollateralValid - %s - %d confirmations\n", strError, conf);
        return false;
    }
}

void CBudgetManager::CheckOrphanVotes()
{
    LOCK(cs);


    std::string strError = "";
    std::map<uint256, CBudgetVote>::iterator it1 = mapOrphanMasternodeBudgetVotes.begin();
    while(it1 != mapOrphanMasternodeBudgetVotes.end()){
        if(budget.ReceiveProposalVote(((*it1).second), NULL, strError)){
            LogPrintf("CBudgetManager::CheckOrphanVotes - Proposal/Budget is known, activating and removing orphan vote\n");
            mapOrphanMasternodeBudgetVotes.erase(it1++);
        } else {
            ++it1;
        }
    }
    std::map<uint256, CFinalizedBudgetVote>::iterator it2 = mapOrphanFinalizedBudgetVotes.begin();
    while(it2 != mapOrphanFinalizedBudgetVotes.end()){
        if(budget.UpdateFinalizedBudget(((*it2).second),NULL, strError)){
            LogPrintf("CBudgetManager::CheckOrphanVotes - Proposal/Budget is known, activating and removing orphan vote\n");
            mapOrphanFinalizedBudgetVotes.erase(it2++);
        } else {
            ++it2;
        }
    }
}

void CBudgetManager::SubmitFinalBudget()
{
    static int nSubmittedHeight = 0; // height at which final budget was submitted last time
    int nCurrentHeight;

    {
        TRY_LOCK(cs_main, locked);
        if(!locked) return;
        if(!chainActive.Tip()) return;
        nCurrentHeight = chainActive.Height();
    }

    int nBlockStart = nCurrentHeight - nCurrentHeight % GetBudgetPaymentCycleBlocks() + GetBudgetPaymentCycleBlocks();
    if(nSubmittedHeight >= nBlockStart)
        return;

    if(nBlockStart - nCurrentHeight > BlocksBeforeSuperblockToSubmitFinalBudget())
        return; // allow submitting final budget only when 2 days left before payments

    std::vector<CBudgetProposal*> vBudgetProposals = budget.GetBudget();
    std::string strBudgetName = "main";
    std::vector<CTxBudgetPayment> vecTxBudgetPayments;

    for(unsigned int i = 0; i < vBudgetProposals.size(); i++){
        CTxBudgetPayment txBudgetPayment;
        txBudgetPayment.nProposalHash = vBudgetProposals[i]->GetHash();
        txBudgetPayment.payee = vBudgetProposals[i]->GetPayee();
        txBudgetPayment.nAmount = vBudgetProposals[i]->GetAllotted();
        vecTxBudgetPayments.push_back(txBudgetPayment);
    }

    if(vecTxBudgetPayments.size() < 1) {
        LogPrintf("CBudgetManager::SubmitFinalBudget - Found No Proposals For Period\n");
        return;
    }

    if (fMasterNode)
    {
        if (mnodeman.GetCurrentMasterNode()->vin == activeMasternode.vin)
        {
            std::string strError = "";
            CKey key2;
            CPubKey pubkey2;

            if(!legacySigner.SetKey(strMasterNodePrivKey, strError, key2, pubkey2))
            {
                LogPrintf("CBudgetManager::SubmitFinalBudget - ERROR: Invalid masternodeprivkey: '%s'\n", strError);
                return;
            }

            CFinalizedBudgetBroadcast finalizedBudgetBroadcast(nBlockStart, vecTxBudgetPayments, activeMasternode.vin, key2);

            if(mapSeenFinalizedBudgets.count(finalizedBudgetBroadcast.Budget().GetHash())) {
                LogPrintf("CBudgetManager::SubmitFinalBudget - Budget already exists - %s\n", finalizedBudgetBroadcast.Budget().GetHash().ToString());
                nSubmittedHeight = nCurrentHeight;
                return; //already exists
            }

            if(!finalizedBudgetBroadcast.Budget().IsValid(strError)){
                LogPrintf("CBudgetManager::SubmitFinalBudget - Invalid finalized budget - %s \n", strError);
                return;
            }

            LOCK(cs);
            mapSeenFinalizedBudgets.insert(make_pair(finalizedBudgetBroadcast.Budget().GetHash(), finalizedBudgetBroadcast));
            finalizedBudgetBroadcast.Relay();
            budget.AddFinalizedBudget(finalizedBudgetBroadcast.Budget());
            nSubmittedHeight = nCurrentHeight;
            LogPrintf("CBudgetManager::SubmitFinalBudget - Done! %s\n", finalizedBudgetBroadcast.Budget().GetHash().ToString());
        }
    }
    else
    {
        CFinalizedBudgetBroadcast tempBudget(nBlockStart, vecTxBudgetPayments, uint256());
        if(mapSeenFinalizedBudgets.count(tempBudget.Budget().GetHash())) {
            LogPrintf("CBudgetManager::SubmitFinalBudget - Budget already exists - %s\n", tempBudget.Budget().GetHash().ToString());
            nSubmittedHeight = nCurrentHeight;
            return; //already exists
        }

        //create fee tx
        CTransaction tx;
        uint256 txidCollateral;

        if(!mapCollateralTxids.count(tempBudget.Budget().GetHash())){
            CWalletTx wtx;
            if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, tempBudget.Budget().GetHash())){
                LogPrintf("CBudgetManager::SubmitFinalBudget - Can't make collateral transaction\n");
                return;
            }

            // make our change address
            CReserveKey reservekey(pwalletMain);
            //send the tx to the network
            pwalletMain->CommitTransaction(wtx, reservekey);
            tx = (CTransaction)wtx;
            txidCollateral = tx.GetHash();
            mapCollateralTxids.insert(make_pair(tempBudget.Budget().GetHash(), txidCollateral));
        } else {
            txidCollateral = mapCollateralTxids[tempBudget.Budget().GetHash()];
        }

        int conf = GetIXConfirmations(tx.GetHash());
        CTransaction txCollateral;
        uint256 nBlockHash;

        if(!GetTransaction(txidCollateral, txCollateral, nBlockHash, true)) {
            LogPrintf ("CBudgetManager::SubmitFinalBudget - Can't find collateral tx %s", txidCollateral.ToString());
            return;
        }

        if (nBlockHash != uint256()) {
            BlockMap::iterator mi = mapBlockIndex.find(nBlockHash);
            if (mi != mapBlockIndex.end() && (*mi).second) {
                CBlockIndex* pindex = (*mi).second;
                if (chainActive.Contains(pindex)) {
                    conf += chainActive.Height() - pindex->nHeight + 1;
                }
            }
        }

        /*
            Wait will we have 1 extra confirmation, otherwise some clients might reject this feeTX
            -- This function is tied to NewBlock, so we will propagate this budget while the block is also propagating
        */
        if(conf < BUDGET_FEE_CONFIRMATIONS+1){
            LogPrintf ("CBudgetManager::SubmitFinalBudget - Collateral requires at least %d confirmations - %s - %d confirmations\n", BUDGET_FEE_CONFIRMATIONS+1, txidCollateral.ToString(), conf);
            return;
        }

        //create the proposal incase we're the first to make it
        CFinalizedBudgetBroadcast finalizedBudgetBroadcast(nBlockStart, vecTxBudgetPayments, txidCollateral);

        std::string strError = "";
        if(!finalizedBudgetBroadcast.Budget().IsValid(strError)){
            LogPrintf("CBudgetManager::SubmitFinalBudget - Invalid finalized budget - %s \n", strError);
            return;
        }

        LOCK(cs);
        mapSeenFinalizedBudgets.insert(make_pair(finalizedBudgetBroadcast.Budget().GetHash(), finalizedBudgetBroadcast));
        finalizedBudgetBroadcast.Relay();
        budget.AddFinalizedBudget(finalizedBudgetBroadcast.Budget());
        nSubmittedHeight = nCurrentHeight;
        LogPrintf("CBudgetManager::SubmitFinalBudget - Done! %s\n", finalizedBudgetBroadcast.Budget().GetHash().ToString());
    }
}

bool CBudgetManager::AddFinalizedBudget(const CFinalizedBudget& finalizedBudget, bool checkCollateral)
{
    std::string strError = "";
    if(!finalizedBudget.IsValid(strError, checkCollateral))
        return false;

    if(mapFinalizedBudgets.count(finalizedBudget.GetHash())) {
        return false;
    }

    mapFinalizedBudgets.insert(make_pair(finalizedBudget.GetHash(), finalizedBudget));
    return true;
}

bool CBudgetManager::AddProposal(const CBudgetProposal& budgetProposal, bool checkCollateral)
{
    LOCK(cs);
    std::string strError = "";
    if(!budgetProposal.IsValid(strError, checkCollateral)) {
        LogPrintf("CBudgetManager::AddProposal - invalid budget proposal - %s\n", strError);
        return false;
    }

    if(mapProposals.count(budgetProposal.GetHash())) {
        return false;
    }

    mapProposals.insert(make_pair(budgetProposal.GetHash(), budgetProposal));
    return true;
}

void CBudgetManager::CheckAndRemove()
{
    LogPrintf("CBudgetManager::CheckAndRemove\n");

    std::string strError = "";

    LogPrintf("CBudgetManager::CheckAndRemove - mapFinalizedBudgets cleanup - size: %d\n", mapFinalizedBudgets.size());
    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);

        bool isValid = pfinalizedBudget->fValid = pfinalizedBudget->IsValid(strError);
        LogPrintf("CBudgetManager::CheckAndRemove - pfinalizedBudget->IsValid - strError: %s\n", strError);
        if(isValid) {
            if(Params().NetworkID() == CBaseChainParams::TESTNET || Params().NetworkID() == CBaseChainParams::MAIN && rand() % 4 == 0)
            {
                //do this 1 in 4 blocks -- spread out the voting activity on mainnet
                // -- this function is only called every sixth block, so this is really 1 in 24 blocks
                pfinalizedBudget->AutoCheck();
            }
            else
            {
                LogPrintf("CFinalizedBudget::AutoCheck - waiting\n");
            }
        }

        ++it;
    }

    LogPrintf("CBudgetManager::CheckAndRemove - mapProposals cleanup - size: %d\n", mapProposals.size());
    std::map<uint256, CBudgetProposal>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end())
    {
        CBudgetProposal* pbudgetProposal = &((*it2).second);
        pbudgetProposal->fValid = pbudgetProposal->IsValid(strError);
        ++it2;
    }

    LogPrintf("CBudgetManager::CheckAndRemove - PASSED\n");
}

const CFinalizedBudget* CBudgetManager::GetMostVotedBudget(int height) const
{
    const CFinalizedBudget* budgetToPay = NULL;
    typedef std::map<uint256, CFinalizedBudget>::const_iterator FinalizedBudgetIterator;
    for (FinalizedBudgetIterator i = mapFinalizedBudgets.begin(); i != mapFinalizedBudgets.end(); ++i)
    {
        if (height != i->second.GetBlockStart())
            continue;

        if (i->second.GetVoteCount() == 0) // discard budgets with no votes
            continue;

        if ((!budgetToPay || i->second.GetVoteCount() > budgetToPay->GetVoteCount()))
            budgetToPay = &i->second;
    }

    return budgetToPay;
}

void CBudgetManager::FillBlockPayee(CMutableTransaction& txNew, CAmount nFees) const
{
    assert (txNew.vout.size() == 1); // There is a blank for block creator's reward

    LOCK(cs);

    const CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev)
        return;

    // Pay the miner

    txNew.vout[0].nValue = GetBlockValue(pindexPrev->nHeight, nFees);

    // Find finalized budgets with the most votes

    const CFinalizedBudget* budgetToPay = GetMostVotedBudget(pindexPrev->nHeight + 1);
    if (budgetToPay == NULL)
        return;

    // Pay the proposals

    BOOST_FOREACH(const CTxBudgetPayment& payment, budgetToPay->GetBudgetPayments())
    {
        LogPrintf("CBudgetManager::FillBlockPayee - Budget payment to %s for %lld; proposal %s\n",
            ScriptToAddress(payment.payee).ToString(), payment.nAmount, payment.nProposalHash.ToString());

        txNew.vout.push_back(CTxOut(payment.nAmount, payment.payee));
    }
}

CFinalizedBudget* CBudgetManager::FindFinalizedBudget(uint256 nHash)
{
    std::map<uint256, CFinalizedBudget>::iterator found = mapFinalizedBudgets.find(nHash);
    if (found != mapFinalizedBudgets.end())
        return NULL;

    return &found->second;
}

CBudgetProposal* CBudgetManager::FindProposal(const std::string &strProposalName)
{
    //find the prop with the highest yes count

    int nYesCount = -99999;
    CBudgetProposal* pbudgetProposal = NULL;

    std::map<uint256, CBudgetProposal>::iterator it = mapProposals.begin();
    while(it != mapProposals.end()){
        if((*it).second.strProposalName == strProposalName && (*it).second.GetYeas() > nYesCount){
            pbudgetProposal = &((*it).second);
            nYesCount = pbudgetProposal->GetYeas();
        }
        ++it;
    }

    if(nYesCount == -99999) return NULL;

    return pbudgetProposal;
}

CBudgetProposal *CBudgetManager::FindProposal(uint256 nHash)
{
    LOCK(cs);

    std::map<uint256, CBudgetProposal>::iterator found = mapProposals.find(nHash);
    if (found == mapProposals.end())
        return NULL;

    return &found->second;
}

bool CBudgetManager::IsBudgetPaymentBlock(int nBlockHeight) const
{
    const CFinalizedBudget* bestBudget = GetMostVotedBudget(nBlockHeight);
    if (bestBudget == NULL)
        return false;

    // If budget doesn't have 5% of the masternode votes, we should not pay it
    return (20 * bestBudget->GetVoteCount() > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION));
}

bool CBudgetManager::IsTransactionValid(const CTransaction& txNew, int nBlockHeight) const
{
    LOCK(cs);

    const CFinalizedBudget* bestBudget = GetMostVotedBudget(nBlockHeight);
    const int mnodeCount = mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION);

    // If budget doesn't have 5% of the network votes, then we should not pay it
    if (bestBudget == NULL || 20 * bestBudget->GetVoteCount() < mnodeCount)
        return false;

    // Check the highest finalized budgets (+/- 10% to assist in consensus)
    for (std::map<uint256, CFinalizedBudget>::const_iterator it = mapFinalizedBudgets.begin(); it != mapFinalizedBudgets.end(); ++it)
    {
        const CFinalizedBudget& pfinalizedBudget = it->second;

        if (10 * (bestBudget->GetVoteCount() - pfinalizedBudget.GetVoteCount()) > mnodeCount)
            continue;

        if(pfinalizedBudget.IsTransactionValid(txNew, nBlockHeight))
            return true;
    }

    // We looked through all of the known budgets
    return false;
}

std::vector<CBudgetProposal*> CBudgetManager::GetAllProposals()
{
    LOCK(cs);

    std::vector<CBudgetProposal*> vBudgetProposalRet;

    std::map<uint256, CBudgetProposal>::iterator it = mapProposals.begin();
    while(it != mapProposals.end())
    {
        (*it).second.CleanAndRemove(false);

        CBudgetProposal* pbudgetProposal = &((*it).second);
        vBudgetProposalRet.push_back(pbudgetProposal);

        ++it;
    }

    return vBudgetProposalRet;
}

//
// Sort by votes, if there's a tie sort by their feeHash TX
//
struct sortProposalsByVotes {
    bool operator()(const std::pair<CBudgetProposal*, int> &left, const std::pair<CBudgetProposal*, int> &right) {
      if( left.second != right.second)
        return (left.second > right.second);
      return (UintToArith256(left.first->nFeeTXHash) > UintToArith256(right.first->nFeeTXHash));
    }
};

//Need to review this function

std::vector<CBudgetProposal*> CBudgetManager::GetBudget()
{
    LOCK(cs);

    // ------- Sort budgets by Yes Count

    std::vector<std::pair<CBudgetProposal*, int> > vBudgetPorposalsSort;

    std::map<uint256, CBudgetProposal>::iterator it = mapProposals.begin();
    while(it != mapProposals.end()){
        (*it).second.CleanAndRemove(false);
        vBudgetPorposalsSort.push_back(make_pair(&((*it).second), (*it).second.GetYeas()-(*it).second.GetNays()));
        ++it;
    }

    std::sort(vBudgetPorposalsSort.begin(), vBudgetPorposalsSort.end(), sortProposalsByVotes());

    // ------- Grab The Budgets In Order

    std::vector<CBudgetProposal*> vBudgetProposalsRet;

    CAmount nBudgetAllocated = 0;
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return vBudgetProposalsRet;

    int nBlockStart = pindexPrev->nHeight - pindexPrev->nHeight % GetBudgetPaymentCycleBlocks() + GetBudgetPaymentCycleBlocks();
    int nBlockEnd  =  nBlockStart + GetBudgetPaymentCycleBlocks() - 1;
    CAmount nTotalBudget = GetTotalBudget(nBlockStart);


    std::vector<std::pair<CBudgetProposal*, int> >::iterator it2 = vBudgetPorposalsSort.begin();
    while(it2 != vBudgetPorposalsSort.end())
    {
        CBudgetProposal* pbudgetProposal = (*it2).first;

        //prop start/end should be inside this period
        if(pbudgetProposal->fValid && pbudgetProposal->nBlockStart <= nBlockStart &&
                pbudgetProposal->nBlockEnd >= nBlockEnd &&
                pbudgetProposal->GetYeas() - pbudgetProposal->GetNays() > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10 && 
                pbudgetProposal->IsEstablished())
        {
            if(pbudgetProposal->GetAmount() + nBudgetAllocated <= nTotalBudget) {
                pbudgetProposal->SetAllotted(pbudgetProposal->GetAmount());
                nBudgetAllocated += pbudgetProposal->GetAmount();
                vBudgetProposalsRet.push_back(pbudgetProposal);
            } else {
                pbudgetProposal->SetAllotted(0);
            }
        }

        ++it2;
    }

    return vBudgetProposalsRet;
}

struct sortFinalizedBudgetsByVotes {
    bool operator()(const std::pair<CFinalizedBudget*, int> &left, const std::pair<CFinalizedBudget*, int> &right) {
        return left.second > right.second;
    }
};

std::vector<CFinalizedBudget*> CBudgetManager::GetFinalizedBudgets()
{
    LOCK(cs);

    std::vector<CFinalizedBudget*> vFinalizedBudgetsRet;
    std::vector<std::pair<CFinalizedBudget*, int> > vFinalizedBudgetsSort;

    // ------- Grab The Budgets In Order

    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);

        vFinalizedBudgetsSort.push_back(make_pair(pfinalizedBudget, pfinalizedBudget->GetVoteCount()));
        ++it;
    }
    std::sort(vFinalizedBudgetsSort.begin(), vFinalizedBudgetsSort.end(), sortFinalizedBudgetsByVotes());

    std::vector<std::pair<CFinalizedBudget*, int> >::iterator it2 = vFinalizedBudgetsSort.begin();
    while(it2 != vFinalizedBudgetsSort.end())
    {
        vFinalizedBudgetsRet.push_back((*it2).first);
        ++it2;
    }

    return vFinalizedBudgetsRet;
}

std::string CBudgetManager::GetRequiredPaymentsString(int nBlockHeight) const
{
    LOCK(cs);

    std::string ret = "unknown-budget";

    const CFinalizedBudget* pfinalizedBudget = GetMostVotedBudget(nBlockHeight);
    if (pfinalizedBudget == NULL)
        return ret;

    BOOST_FOREACH(const CTxBudgetPayment& payment, pfinalizedBudget->GetBudgetPayments())
    {
        if(ret == "unknown-budget"){
            ret = payment.nProposalHash.ToString();
        } else {
            ret += ",";
            ret += payment.nProposalHash.ToString();
        }
    }

    return ret;
}

CAmount CBudgetManager::GetTotalBudget(int nHeight)
{
    if(chainActive.Tip() == NULL) return 0;

    //get min block value and calculate from that
    CAmount nSubsidy = 10 * COIN;
    int halvings = nHeight / Params().SubsidyHalvingInterval();

    // Subsidy is cut in half every 2,100,000 blocks which will occur approximately every 4 years.
    nSubsidy >>= halvings;

    // Amount of blocks in a months period of time (using 1 minutes per) = (60*24*30)/1
    if(Params().NetworkID() == CBaseChainParams::MAIN) return ((nSubsidy/100)*10)*1440*30;

    //for testing purposes
    return ((nSubsidy/100)*10)*50;
}

void CBudgetManager::NewBlock()
{
    TRY_LOCK(cs, fBudgetNewBlock);
    if(!fBudgetNewBlock)
        return;

    if (masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_BUDGET)
        return;

    if (strBudgetMode == "suggest" || fMasterNode) //suggest the budget we see
        SubmitFinalBudget();

    //this function should be called 1/6 blocks, allowing up to 100 votes per day on all proposals
    if(chainActive.Height() % 6 != 0)
        return;

    // incremental sync with our peers
    if(masternodeSync.IsSynced()){
        LogPrintf("CBudgetManager::NewBlock - incremental sync started\n");
        if(chainActive.Height() % 600 == rand() % 600) {
            ClearSeen();
            ResetSync();
        }

        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            if(pnode->nVersion >= MIN_BUDGET_PEER_PROTO_VERSION) 
                Sync(pnode, uint256(), true);
        
        MarkSynced();
    }
     

    CheckAndRemove();

    //remove invalid votes once in a while (we have to check the signatures and validity of every vote, somewhat CPU intensive)

    LogPrintf("CBudgetManager::NewBlock - askedForSourceProposalOrBudget cleanup - size: %d\n", askedForSourceProposalOrBudget.size());
    std::map<uint256, int64_t>::iterator it = askedForSourceProposalOrBudget.begin();
    while(it != askedForSourceProposalOrBudget.end()){
        if((*it).second > GetTime() - (60*60*24)){
            ++it;
        } else {
            askedForSourceProposalOrBudget.erase(it++);
        }
    }

    LogPrintf("CBudgetManager::NewBlock - mapProposals cleanup - size: %d\n", mapProposals.size());
    std::map<uint256, CBudgetProposal>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end()){
        (*it2).second.CleanAndRemove(false);
        ++it2;
    }

    LogPrintf("CBudgetManager::NewBlock - mapFinalizedBudgets cleanup - size: %d\n", mapFinalizedBudgets.size());
    std::map<uint256, CFinalizedBudget>::iterator it3 = mapFinalizedBudgets.begin();
    while(it3 != mapFinalizedBudgets.end()){
        (*it3).second.CleanAndRemove(false);
        ++it3;
    }

    LogPrintf("CBudgetManager::NewBlock - vecImmatureBudgetProposals cleanup - size: %d\n", vecImmatureBudgetProposals.size());
    std::vector<CBudgetProposalBroadcast>::iterator it4 = vecImmatureBudgetProposals.begin();
    while(it4 != vecImmatureBudgetProposals.end())
    {
        std::string strError = "";
        int nConf = 0;
        if(!IsBudgetCollateralValid((*it4).nFeeTXHash, (*it4).GetHash(), strError, (*it4).nTime, nConf)){
            ++it4;
            continue;
        }

        if(!(*it4).IsValid(strError)) {
            LogPrintf("mprop (immature) - invalid budget proposal - %s\n", strError);
            it4 = vecImmatureBudgetProposals.erase(it4); 
            continue;
        }

        CBudgetProposal budgetProposal((*it4));
        if(AddProposal(budgetProposal)) {(*it4).Relay();}

        LogPrintf("mprop (immature) - new budget - %s\n", (*it4).GetHash().ToString());
        it4 = vecImmatureBudgetProposals.erase(it4); 
    }

    LogPrintf("CBudgetManager::NewBlock - vecImmatureFinalizedBudgets cleanup - size: %d\n", vecImmatureFinalizedBudgets.size());
    std::vector<CFinalizedBudgetBroadcast>::iterator it5 = vecImmatureFinalizedBudgets.begin();
    while(it5 != vecImmatureFinalizedBudgets.end())
    {
        std::string strError = "";
        int nConf = 0;
        int64_t nTime = 0;
        if(!IsBudgetCollateralValid(it5->Budget().GetFeeTxHash(), it5->Budget().GetHash(), strError, nTime, nConf)){
            ++it5;
            continue;
        }

        if(!it5->Budget().IsValid(strError)) {
            LogPrintf("fbs (immature) - invalid finalized budget - %s\n", strError);
            it5 = vecImmatureFinalizedBudgets.erase(it5); 
            continue;
        }

        LogPrintf("fbs (immature) - new finalized budget - %s\n", it5->Budget().GetHash().ToString());

        if(AddFinalizedBudget(it5->Budget()))
            it5->Relay();

        it5 = vecImmatureFinalizedBudgets.erase(it5); 
    }
    LogPrintf("CBudgetManager::NewBlock - PASSED\n");
}

void CBudgetManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    // lite mode is not supported
    if(fLiteMode) return;
    if(!masternodeSync.IsBlockchainSynced()) return;

    LOCK(cs_budget);

    if (strCommand == "mnvs") { //Masternode vote sync
        uint256 nProp;
        vRecv >> nProp;

        if(Params().NetworkID() == CBaseChainParams::MAIN){
            if(nProp.IsNull()) {
                if(pfrom->HasFulfilledRequest("mnvs")) {
                    LogPrintf("mnvs - peer already asked me for the list\n");
                    Misbehaving(pfrom->GetId(), 20);
                    return;
                }
                pfrom->FulfilledRequest("mnvs");
            }
        }

        Sync(pfrom, nProp);
        LogPrintf("mnvs - Sent Masternode votes to %s\n", pfrom->addr.ToString());
    }

    if (strCommand == "mprop") { //Masternode Proposal
        CBudgetProposalBroadcast budgetProposalBroadcast;
        vRecv >> budgetProposalBroadcast;

        DebugLogBudget(budgetProposalBroadcast, pfrom->addr, "PR");

        if(mapSeenMasternodeBudgetProposals.count(budgetProposalBroadcast.GetHash())){
            masternodeSync.AddedBudgetItem(budgetProposalBroadcast.GetHash());
            return;
        }

        std::string strError = "";
        int nConf = 0;
        if(!IsBudgetCollateralValid(budgetProposalBroadcast.nFeeTXHash, budgetProposalBroadcast.GetHash(), strError, budgetProposalBroadcast.nTime, nConf)){
            LogPrintf("Proposal FeeTX is not valid - %s - %s\n", budgetProposalBroadcast.nFeeTXHash.ToString(), strError);
            if(nConf >= 1) vecImmatureBudgetProposals.push_back(budgetProposalBroadcast);
            return;
        }

        mapSeenMasternodeBudgetProposals.insert(make_pair(budgetProposalBroadcast.GetHash(), budgetProposalBroadcast));

        if(!budgetProposalBroadcast.IsValid(strError)) {
            LogPrintf("mprop - invalid budget proposal - %s\n", strError);
            return;
        }

        CBudgetProposal budgetProposal(budgetProposalBroadcast);
        if(AddProposal(budgetProposal)) {budgetProposalBroadcast.Relay();}
        masternodeSync.AddedBudgetItem(budgetProposalBroadcast.GetHash());

        DebugLogBudget(budgetProposalBroadcast, pfrom->addr, "PA");
        LogPrintf("mprop - new budget - %s\n", budgetProposalBroadcast.GetHash().ToString());

        //We might have active votes for this proposal that are valid now
        CheckOrphanVotes();
    }

    if (strCommand == "mvote") { //Masternode Vote
        CBudgetVote vote;
        vRecv >> vote;
        vote.fValid = true;

        DebugLogBudget(vote, pfrom->addr, "VR");

        if(mapSeenMasternodeBudgetVotes.count(vote.GetHash())){
            masternodeSync.AddedBudgetItem(vote.GetHash());
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vin);
        if(pmn == NULL) {
            LogPrint("mnbudget", "mvote - unknown masternode - vin: %s\n", vote.vin.ToString());
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }


        mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        if(!vote.SignatureValid(true)){
            LogPrintf("mvote - signature invalid\n");
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }
        
        std::string strError = "";
        if(ReceiveProposalVote(vote, pfrom, strError)) {
            vote.Relay();
            masternodeSync.AddedBudgetItem(vote.GetHash());
        }

        DebugLogBudget(vote, pfrom->addr, "VA");
        LogPrintf("mvote - new budget vote - %s\n", vote.GetHash().ToString());
    }

    if (strCommand == "fbs") { //Finalized Budget Suggestion
        CFinalizedBudgetBroadcast finalizedBudgetBroadcast;
        vRecv >> finalizedBudgetBroadcast;

        if(mapSeenFinalizedBudgets.count(finalizedBudgetBroadcast.Budget().GetHash())){
            masternodeSync.AddedBudgetItem(finalizedBudgetBroadcast.Budget().GetHash());
            return;
        }

        std::string strError = "";
        int nConf = 0;
        int64_t nTime = 0;
        if(finalizedBudgetBroadcast.Budget().GetFeeTxHash() != uint256() && !IsBudgetCollateralValid(finalizedBudgetBroadcast.Budget().GetFeeTxHash(), finalizedBudgetBroadcast.Budget().GetHash(), strError, nTime, nConf)){
            LogPrintf("Finalized Budget FeeTX is not valid - %s - %s\n", finalizedBudgetBroadcast.Budget().GetFeeTxHash().ToString(), strError);

            if(nConf >= 1) vecImmatureFinalizedBudgets.push_back(finalizedBudgetBroadcast);
            return;
        }

        mapSeenFinalizedBudgets.insert(make_pair(finalizedBudgetBroadcast.Budget().GetHash(), finalizedBudgetBroadcast));

        if (finalizedBudgetBroadcast.Budget().GetFeeTxHash() == uint256())
        {
            const CMasternode* producer = mnodeman.Find(finalizedBudgetBroadcast.Budget().MasternodeSubmittedId());
            if (producer == NULL)
            {
                LogPrintf("fbs - unknown masternode - vin: %s\n", finalizedBudgetBroadcast.Budget().MasternodeSubmittedId().ToString());
                mnodeman.AskForMN(pfrom, finalizedBudgetBroadcast.Budget().MasternodeSubmittedId());
                return;
            }

            if (!finalizedBudgetBroadcast.Budget().VerifySignature(producer->pubkey2))
            {
                Misbehaving(pfrom->GetId(), 50);
            }
        }

        if(!finalizedBudgetBroadcast.Budget().IsValid(strError)) {
            LogPrintf("fbs - invalid finalized budget - %s\n", strError);
            return;
        }

        LogPrintf("fbs - new finalized budget - %s\n", finalizedBudgetBroadcast.Budget().GetHash().ToString());

        if(AddFinalizedBudget(finalizedBudgetBroadcast.Budget()))
        {
            finalizedBudgetBroadcast.Relay();
            masternodeSync.AddedBudgetItem(finalizedBudgetBroadcast.Budget().GetHash());
        }

        //we might have active votes for this budget that are now valid
        CheckOrphanVotes();
    }

    if (strCommand == "fbvote") { //Finalized Budget Vote
        CFinalizedBudgetVote vote;
        vRecv >> vote;
        vote.fValid = true;

        if(mapSeenFinalizedBudgetVotes.count(vote.GetHash())){
            masternodeSync.AddedBudgetItem(vote.GetHash());
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vin);
        if(pmn == NULL) {
            LogPrint("mnbudget", "fbvote - unknown masternode - vin: %s\n", vote.vin.ToString());
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        mapSeenFinalizedBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        if(!vote.SignatureValid(true)){
            LogPrintf("fbvote - signature invalid\n");
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        std::string strError = "";
        if(UpdateFinalizedBudget(vote, pfrom, strError)) {
            vote.Relay();
            masternodeSync.AddedBudgetItem(vote.GetHash());

            LogPrintf("fbvote - new finalized budget vote - %s\n", vote.GetHash().ToString());
        } else {
            LogPrintf("fbvote - rejected finalized budget vote - %s - %s\n", vote.GetHash().ToString(), strError);
        }
    }
}

//mark that a full sync is needed
void CBudgetManager::ResetSync()
{
    LOCK(cs);


    std::map<uint256, CBudgetProposalBroadcast>::iterator it1 = mapSeenMasternodeBudgetProposals.begin();
    while(it1 != mapSeenMasternodeBudgetProposals.end()){
        CBudgetProposal* pbudgetProposal = FindProposal((*it1).first);
        if(pbudgetProposal && pbudgetProposal->fValid){
        
            //mark votes
            std::map<uint256, CBudgetVote>::iterator it2 = pbudgetProposal->mapVotes.begin();
            while(it2 != pbudgetProposal->mapVotes.end()){
                (*it2).second.fSynced = false;
                ++it2;
            }
        }
        ++it1;
    }

    std::map<uint256, CFinalizedBudgetBroadcast>::iterator it3 = mapSeenFinalizedBudgets.begin();
    while(it3 != mapSeenFinalizedBudgets.end()){
        CFinalizedBudget* pfinalizedBudget = FindFinalizedBudget((*it3).first);
        if (pfinalizedBudget)
            pfinalizedBudget->ResetSync();
        ++it3;
    }
}

void CBudgetManager::MarkSynced()
{
    LOCK(cs);

    /*
        Mark that we've sent all valid items
    */

    std::map<uint256, CBudgetProposalBroadcast>::iterator it1 = mapSeenMasternodeBudgetProposals.begin();
    while(it1 != mapSeenMasternodeBudgetProposals.end()){
        CBudgetProposal* pbudgetProposal = FindProposal((*it1).first);
        if(pbudgetProposal && pbudgetProposal->fValid){
        
            //mark votes
            std::map<uint256, CBudgetVote>::iterator it2 = pbudgetProposal->mapVotes.begin();
            while(it2 != pbudgetProposal->mapVotes.end()){
                if((*it2).second.fValid)
                    (*it2).second.fSynced = true;
                ++it2;
            }
        }
        ++it1;
    }

    std::map<uint256, CFinalizedBudgetBroadcast>::iterator it3 = mapSeenFinalizedBudgets.begin();
    while(it3 != mapSeenFinalizedBudgets.end()){
        CFinalizedBudget* pfinalizedBudget = FindFinalizedBudget((*it3).first);
        if(pfinalizedBudget)
            pfinalizedBudget->MarkSynced();
        ++it3;
    }

}


void CBudgetManager::Sync(CNode* pfrom, uint256 nProp, bool fPartial) const
{
    LOCK(cs);

    /*
        Sync with a client on the network

        --

        This code checks each of the hash maps for all known budget proposals and finalized budget proposals, then checks them against the
        budget object to see if they're OK. If all checks pass, we'll send it to the peer.

    */

    int nInvCount = 0;

    std::map<uint256, CBudgetProposalBroadcast>::const_iterator it1 = mapSeenMasternodeBudgetProposals.begin();
    while(it1 != mapSeenMasternodeBudgetProposals.end())
    {
        std::map<uint256, CBudgetProposal>::const_iterator pbudgetProposal = mapProposals.find((*it1).first);
        if(pbudgetProposal != mapProposals.end() && pbudgetProposal->second.fValid && (nProp.IsNull() || (*it1).first == nProp)){
            pfrom->PushInventory(CInv(MSG_BUDGET_PROPOSAL, (*it1).second.GetHash()));
            nInvCount++;
        
            //send votes
            std::map<uint256, CBudgetVote>::const_iterator it2 = pbudgetProposal->second.mapVotes.begin();
            while(it2 != pbudgetProposal->second.mapVotes.end()){
                if((*it2).second.fValid){
                    if((fPartial && !(*it2).second.fSynced) || !fPartial) {
                        pfrom->PushInventory(CInv(MSG_BUDGET_VOTE, (*it2).second.GetHash()));
                        nInvCount++;
                    }
                }
                ++it2;
            }
        }
        ++it1;
    }

    pfrom->PushMessage("ssc", MASTERNODE_SYNC_BUDGET_PROP, nInvCount);

    LogPrintf("CBudgetManager::Sync - sent %d items\n", nInvCount);

    nInvCount = 0;

    std::map<uint256, CFinalizedBudgetBroadcast>::const_iterator it3 = mapSeenFinalizedBudgets.begin();
    while(it3 != mapSeenFinalizedBudgets.end()){
        std::map<uint256, CFinalizedBudget>::const_iterator pfinalizedBudget = mapFinalizedBudgets.find((*it3).first);
        if(pfinalizedBudget != mapFinalizedBudgets.end() && (nProp.IsNull() || (*it3).first == nProp))
            pfinalizedBudget->second.Sync(pfrom, fPartial);
        ++it3;
    }

    pfrom->PushMessage("ssc", MASTERNODE_SYNC_BUDGET_FIN, nInvCount);
    LogPrintf("CBudgetManager::Sync - sent %d items\n", nInvCount);

}

bool CBudgetManager::SubmitProposalVote(const CBudgetVote& vote, std::string& strError)
{
    DebugLogBudget(vote, CAddress(), "VR");
    map<uint256, CBudgetProposal>::iterator found = mapProposals.find(vote.nProposalHash);
    if (found == mapProposals.end())
    {
        strError = "Proposal not found!";
        return false;
    }

    CBudgetProposal& proposal = found->second;
    if (!CanSubmitVotes(proposal.nBlockStart, proposal.nBlockEnd))
    {
        strError = "The proposal voting is currently disabled as it is too close to the proposal payment";
        return false;
    }

    DebugLogBudget(vote, CAddress(), "VA");
    return proposal.AddOrUpdateVote(vote, strError);
}

bool CBudgetManager::CanSubmitVotes(int blockStart, int blockEnd) const
{
    const int height = chainActive.Height();

    if (blockStart > GetNextSuperblock(height) || blockEnd <= GetNextSuperblock(height)) // Not for the next SB
        return true;

    return GetNextSuperblock(height) - height > GetVotingThreshold();
}

bool CBudgetManager::ReceiveProposalVote(const CBudgetVote &vote, CNode *pfrom, std::string &strError)
{
    LOCK(cs);


    if(!mapProposals.count(vote.nProposalHash)){
        if(pfrom){
            DebugLogBudget(vote, pfrom->addr, "VO");
            // only ask for missing items after our syncing process is complete -- 
            //   otherwise we'll think a full sync succeeded when they return a result
            if(!masternodeSync.IsSynced()) return false;

            LogPrintf("CBudgetManager::ReceiveProposalVote - Unknown proposal %d, asking for source proposal\n", vote.nProposalHash.ToString());
            mapOrphanMasternodeBudgetVotes[vote.nProposalHash] = vote;

            if(!askedForSourceProposalOrBudget.count(vote.nProposalHash)){
                pfrom->PushMessage("mnvs", vote.nProposalHash);
                askedForSourceProposalOrBudget[vote.nProposalHash] = GetTime();
            }
        }

        strError = "Proposal not found!";
        return false;
    }


    CBudgetProposal& proposal = mapProposals[vote.nProposalHash];
    int height = chainActive.Height();
    if (proposal.nBlockStart <= GetNextSuperblock(height) && proposal.nBlockEnd > GetNextSuperblock(height))
    {
        const int votingThresholdTime = GetVotingThreshold() * Params().TargetSpacing() * 0.75;
        const int superblockProjectedTime = GetAdjustedTime() + (GetNextSuperblock(height) - height) * Params().TargetSpacing();

        if (superblockProjectedTime - vote.nTime <= votingThresholdTime)
        {
            strError = "Vote is too close to superblock.";
            return false;
        }
    }

    if(!proposal.AddOrUpdateVote(vote, strError))
        return false;

    if (fMasterNode)
    {
        for (map<uint256, CFinalizedBudget>::iterator i = mapFinalizedBudgets.begin(); i != mapFinalizedBudgets.end(); ++i) {
            CFinalizedBudget& finalBudget = i->second;

            if (finalBudget.IsValid() && !finalBudget.IsVoteSubmitted())
                finalBudget.ResetAutoChecked();
        }

    }

    return true;
}

bool CBudgetManager::UpdateFinalizedBudget(const CFinalizedBudgetVote& vote, CNode* pfrom, std::string& strError)
{
    LOCK(cs);

    if(!mapFinalizedBudgets.count(vote.nBudgetHash)){
        if(pfrom){
            // only ask for missing items after our syncing process is complete -- 
            //   otherwise we'll think a full sync succeeded when they return a result
            if(!masternodeSync.IsSynced()) return false;

            LogPrintf("CBudgetManager::UpdateFinalizedBudget - Unknown Finalized Proposal %s, asking for source budget\n", vote.nBudgetHash.ToString());
            mapOrphanFinalizedBudgetVotes[vote.nBudgetHash] = vote;

            if(!askedForSourceProposalOrBudget.count(vote.nBudgetHash)){
                pfrom->PushMessage("mnvs", vote.nBudgetHash);
                askedForSourceProposalOrBudget[vote.nBudgetHash] = GetTime();
            }

        }

        strError = "Finalized Budget not found!";
        return false;
    }

    bool isOldVote = false;

    for (std::map<uint256, CFinalizedBudget>::iterator i = mapFinalizedBudgets.begin(); i != mapFinalizedBudgets.end(); ++i)
    {
        const std::map<uint256, CFinalizedBudgetVote>& votes = i->second.GetVotes();
        const std::map<uint256, CFinalizedBudgetVote>::const_iterator found = votes.find(vote.vin.prevout.GetHash());
        if (found != votes.end() && found->second.nTime > vote.nTime)
            isOldVote = true;
    }

    if (!mapFinalizedBudgets[vote.nBudgetHash].AddOrUpdateVote(isOldVote, vote, strError))
        return false;

    for (std::map<uint256, CFinalizedBudget>::iterator i = mapFinalizedBudgets.begin(); i != mapFinalizedBudgets.end(); ++i)
    {
        i->second.DiscontinueOlderVotes(vote);
    }

    return true;
}

CBudgetProposal::CBudgetProposal()
{
    strProposalName = "unknown";
    nBlockStart = 0;
    nBlockEnd = 0;
    nAmount = 0;
    nTime = 0;
    fValid = true;
}

CBudgetProposal::CBudgetProposal(std::string strProposalNameIn, std::string strURLIn, int nBlockStartIn, int nBlockEndIn, CScript addressIn, CAmount nAmountIn, uint256 nFeeTXHashIn)
{
    strProposalName = strProposalNameIn;
    strURL = strURLIn;
    nBlockStart = nBlockStartIn;
    nBlockEnd = nBlockEndIn;
    address = addressIn;
    nAmount = nAmountIn;
    nFeeTXHash = nFeeTXHashIn;
    fValid = true;
}

CBudgetProposal::CBudgetProposal(const CBudgetProposal& other)
{
    strProposalName = other.strProposalName;
    strURL = other.strURL;
    nBlockStart = other.nBlockStart;
    nBlockEnd = other.nBlockEnd;
    address = other.address;
    nAmount = other.nAmount;
    nTime = other.nTime;
    nFeeTXHash = other.nFeeTXHash;
    mapVotes = other.mapVotes;
    fValid = true;
}

bool CBudgetProposal::IsValid(std::string& strError, bool fCheckCollateral) const
{
    if(GetNays() - GetYeas() > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10){
         strError = "Active removal";
         return false;
    }

    if(nBlockStart < 0) {
        strError = "Invalid Proposal";
        return false;
    }

    if(nBlockEnd < nBlockStart) {
        strError = "Invalid nBlockEnd";
        return false;
    }

    if(nAmount < 1*COIN) {
        strError = "Invalid nAmount";
        return false;
    }

    if(address == CScript()) {
        strError = "Invalid Payment Address";
        return false;
    }

    if(fCheckCollateral){
        int nConf = 0;
        if(!IsBudgetCollateralValid(nFeeTXHash, GetHash(), strError, nTime, nConf)){
            return false;
        }
    }

    /*
        TODO: There might be an issue with multisig in the coinbase on mainnet, we will add support for it in a future release.
    */
    if(address.IsPayToScriptHash()) {
        strError = "Multisig is not currently supported.";
        return false;
    }

    //if proposal doesn't gain traction within 2 weeks, remove it
    // nTime not being saved correctly
    // -- TODO: We should keep track of the last time the proposal was valid, if it's invalid for 2 weeks, erase it
    // if(nTime + (60*60*24*2) < GetAdjustedTime()) {
    //     if(GetYeas()-GetNays() < (mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10)) {
    //         strError = "Not enough support";
    //         return false;
    //     }
    // }

    //can only pay out 10% of the possible coins (min value of coins)
    if(nAmount > budget.GetTotalBudget(nBlockStart)) {
        strError = "Payment more than max";
        return false;
    }

    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) {strError = "Tip is NULL"; return true;}

    if(GetBlockEnd() < pindexPrev->nHeight - GetBudgetPaymentCycleBlocks()/2 ) return false;


    return true;
}

bool CBudgetProposal::AddOrUpdateVote(const CBudgetVote& vote, std::string& strError)
{
    LOCK(cs);

    uint256 hash = vote.vin.prevout.GetHash();

    if(mapVotes.count(hash)){
        if(mapVotes[hash].nTime > vote.nTime){
            strError = strprintf("new vote older than existing vote - %s\n", vote.GetHash().ToString());
            LogPrint("mnbudget", "CBudgetProposal::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        if(vote.nTime - mapVotes[hash].nTime < BUDGET_VOTE_UPDATE_MIN){
            strError = strprintf("time between votes is too soon - %s - %lli\n", vote.GetHash().ToString(), vote.nTime - mapVotes[hash].nTime);
            LogPrint("mnbudget", "CBudgetProposal::AddOrUpdateVote - %s\n", strError);
            return false;
        }
    }

    if(vote.nTime > GetTime() + (60*60)){
        strError = strprintf("new vote is too far ahead of current time - %s - nTime %lli - Max Time %lli\n", vote.GetHash().ToString(), vote.nTime, GetTime() + (60*60));
        LogPrint("mnbudget", "CBudgetProposal::AddOrUpdateVote - %s\n", strError);
        return false;
    }        

    mapVotes[hash] = vote;
    return true;
}

// If masternode voted for a proposal, but is now invalid -- remove the vote
void CBudgetProposal::CleanAndRemove(bool fSignatureCheck)
{
    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        (*it).second.fValid = (*it).second.SignatureValid(fSignatureCheck);
        ++it;
    }
}

double CBudgetProposal::GetRatio() const
{
    int yeas = 0;
    int nays = 0;

    for (std::map<uint256, CBudgetVote>::const_iterator i = mapVotes.begin(); i != mapVotes.end(); ++i) {
        if (i->second.nVote == VOTE_YES)
            ++yeas;
        if (i->second.nVote == VOTE_NO)
            ++nays;
    }

    if(yeas + nays == 0) return 0.0f;

    return ((double)(yeas) / (double)(yeas+nays));
}

int CBudgetProposal::GetYeas() const
{
    int ret = 0;

    for (std::map<uint256, CBudgetVote>::const_iterator i = mapVotes.begin(); i != mapVotes.end(); ++i) {
        if (i->second.nVote == VOTE_YES && i->second.fValid)
            ++ret;
    }

    return ret;
}

int CBudgetProposal::GetNays() const
{
    int ret = 0;

    for (std::map<uint256, CBudgetVote>::const_iterator i = mapVotes.begin(); i != mapVotes.end(); ++i) {
        if (i->second.nVote == VOTE_NO && i->second.fValid)
            ++ret;
    }

    return ret;
}

int CBudgetProposal::GetAbstains() const
{
    int ret = 0;

    for (std::map<uint256, CBudgetVote>::const_iterator i = mapVotes.begin(); i != mapVotes.end(); ++i) {
        if (i->second.nVote == VOTE_ABSTAIN && i->second.fValid)
            ++ret;
    }

    return ret;
}

int CBudgetProposal::GetBlockStartCycle() const
{
    //end block is half way through the next cycle (so the proposal will be removed much after the payment is sent)

    return nBlockStart - nBlockStart % GetBudgetPaymentCycleBlocks();
}

int CBudgetProposal::GetBlockCurrentCycle() const
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL)
        return -1;

    if(pindexPrev->nHeight >= GetBlockEndCycle())
        return -1;

    return pindexPrev->nHeight - pindexPrev->nHeight % GetBudgetPaymentCycleBlocks();
}

int CBudgetProposal::GetBlockEndCycle() const
{
    //end block is half way through the next cycle (so the proposal will be removed much after the payment is sent)

    return nBlockEnd - GetBudgetPaymentCycleBlocks() / 2;
}

int CBudgetProposal::GetTotalPaymentCount() const
{
    return (GetBlockEndCycle() - GetBlockStartCycle()) / GetBudgetPaymentCycleBlocks();
}

int CBudgetProposal::GetRemainingPaymentCount() const
{
    // If this budget starts in the future, this value will be wrong
    int nPayments = (GetBlockEndCycle() - GetBlockCurrentCycle()) / GetBudgetPaymentCycleBlocks() - 1;
    // Take the lowest value
    return std::min(nPayments, GetTotalPaymentCount());
}

CBudgetProposalBroadcast::CBudgetProposalBroadcast(std::string strProposalNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn, uint256 nFeeTXHashIn)
{
    strProposalName = strProposalNameIn;
    strURL = strURLIn;

    nBlockStart = nBlockStartIn;

    int nCycleStart = nBlockStart - nBlockStart % GetBudgetPaymentCycleBlocks();
    //calculate the end of the cycle for this vote, add half a cycle (vote will be deleted after that block)
    nBlockEnd = nCycleStart + GetBudgetPaymentCycleBlocks() * nPaymentCount + GetBudgetPaymentCycleBlocks()/2;

    address = addressIn;
    nAmount = nAmountIn;

    nFeeTXHash = nFeeTXHashIn;
}

void CBudgetProposalBroadcast::Relay()
{
    CInv inv(MSG_BUDGET_PROPOSAL, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

CBudgetVote::CBudgetVote()
{
    vin = CTxIn();
    nProposalHash = uint256();
    nVote = VOTE_ABSTAIN;
    nTime = 0;
    fValid = true;
    fSynced = false;
}

CBudgetVote::CBudgetVote(CTxIn vinIn, uint256 nProposalHashIn, int nVoteIn)
{
    vin = vinIn;
    nProposalHash = nProposalHashIn;
    nVote = nVoteIn;
    nTime = GetAdjustedTime();
    fValid = true;
    fSynced = false;
}

void CBudgetVote::Relay()
{
    CInv inv(MSG_BUDGET_VOTE, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

bool CBudgetVote::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    // Choose coins to use
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;

    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nProposalHash.ToString() + boost::lexical_cast<std::string>(nVote) + boost::lexical_cast<std::string>(nTime);

    if(!legacySigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrintf("CBudgetVote::Sign - Error upon calling SignMessage");
        return false;
    }

    if(!legacySigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CBudgetVote::Sign - Error upon calling VerifyMessage");
        return false;
    }

    return true;
}

bool CBudgetVote::SignatureValid(bool fSignatureCheck) const
{
    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nProposalHash.ToString() + boost::lexical_cast<std::string>(nVote) + boost::lexical_cast<std::string>(nTime);

    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn == NULL)
    {
        LogPrint("mnbudget", "CBudgetVote::SignatureValid() - Unknown Masternode - %s\n", vin.ToString());
        return false;
    }

    if(!fSignatureCheck) return true;

    if(!legacySigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("CBudgetVote::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

CFinalizedBudget::CFinalizedBudget()
{
    strBudgetName = "";
    nBlockStart = 0;
    vecBudgetPayments.clear();
    mapVotes.clear();
    nFeeTXHash = uint256();
    nTime = 0;
    fValid = true;
    fAutoChecked = false;
    voteSubmittedTime = boost::none;
}

CFinalizedBudget::CFinalizedBudget(int nBlockStart, const std::vector<CTxBudgetPayment>& vecBudgetPayments, uint256 nFeeTXHash)
    : fAutoChecked(false)
    , vecBudgetPayments(vecBudgetPayments)
    , nBlockStart(nBlockStart)
    , nFeeTXHash(nFeeTXHash)
    , fValid(true)
    , nTime(0)
{
    boost::sort(this->vecBudgetPayments, ComparePayments);
}

CFinalizedBudget::CFinalizedBudget(int nBlockStart, const std::vector<CTxBudgetPayment>& vecBudgetPayments, const CTxIn& masternodeId, const CKey& keyMasternode)
    : fAutoChecked(false)
    , vecBudgetPayments(vecBudgetPayments)
    , nBlockStart(nBlockStart)
    , masternodeSubmittedId(masternodeId)
    , fValid(true)
    , nTime(0)
{
    boost::sort(this->vecBudgetPayments, ComparePayments);

    if (!keyMasternode.SignCompact(GetHash(), signature))
        throw std::runtime_error("Cannot sign finalized budget: ");
}

CFinalizedBudget::CFinalizedBudget(int nBlockStart, const std::vector<CTxBudgetPayment>& vecBudgetPayments, const CTxIn& masternodeId, const std::vector<unsigned char>& signature)
    : fAutoChecked(false)
    , vecBudgetPayments(vecBudgetPayments)
    , nBlockStart(nBlockStart)
    , masternodeSubmittedId(masternodeId)
    , signature(signature)
    , fValid(true)
    , nTime(0)
{
    boost::sort(this->vecBudgetPayments, ComparePayments);
}

CFinalizedBudget::CFinalizedBudget(const CFinalizedBudget& other)
{
    assert(boost::is_sorted(vecBudgetPayments, ComparePayments));
    strBudgetName = other.strBudgetName;
    nBlockStart = other.nBlockStart;
    vecBudgetPayments = other.vecBudgetPayments;
    mapVotes = other.mapVotes;
    nFeeTXHash = other.nFeeTXHash;
    nTime = other.nTime;
    signature = other.signature;
    masternodeSubmittedId = other.masternodeSubmittedId;
    fValid = true;
    fAutoChecked = false;
    voteSubmittedTime = boost::none;
}

void CFinalizedBudget::DiscontinueOlderVotes(const CFinalizedBudgetVote& newerVote)
{
    std::map<uint256, CFinalizedBudgetVote>::const_iterator found = mapVotes.find(newerVote.vin.prevout.GetHash());
    if (found == mapVotes.end())
        return;

    if (found->second.nTime < newerVote.nTime)
    {
        mapObsoleteVotes.insert(std::make_pair(newerVote.vin.prevout.GetHash(), found->second));
        mapVotes.erase(found);
    }
}

bool CFinalizedBudget::AddOrUpdateVote(bool isOldVote, const CFinalizedBudgetVote& vote, std::string& strError)
{
    LOCK(cs);

    uint256 masternodeHash = vote.vin.prevout.GetHash();
    map<uint256, CFinalizedBudgetVote>::iterator found = mapVotes.find(masternodeHash);

    if(found != mapVotes.end()){
        const CFinalizedBudgetVote& previousVote = found->second;
        if (previousVote.GetHash() == vote.GetHash()) {
            LogPrint("mnbudget", "CFinalizedBudget::AddOrUpdateVote - Already have the vote\n");
            return true;
        }
        if(previousVote.nTime > vote.nTime) {
            strError = strprintf("new vote older than existing vote - %s\n", vote.GetHash().ToString());
            LogPrint("mnbudget", "CFinalizedBudget::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        if(vote.nTime - previousVote.nTime < FINAL_BUDGET_VOTE_UPDATE_MIN) {
            strError = strprintf("time between votes is too soon - %s - %lli\n", vote.GetHash().ToString(), vote.nTime - previousVote.nTime);
            LogPrint("mnbudget", "CFinalizedBudget::AddOrUpdateVote - %s\n", strError);
            return false;
        }
    }

    if(vote.nTime > GetTime() + (60*60)){
        strError = strprintf("new vote is too far ahead of current time - %s - nTime %lli - Max Time %lli\n", vote.GetHash().ToString(), vote.nTime, GetTime() + (60*60));
        LogPrint("mnbudget", "CFinalizedBudget::AddOrUpdateVote - %s\n", strError);
        return false;
    }

    if (isOldVote)
        mapObsoleteVotes.insert(found, std::make_pair(masternodeHash, vote));
    else
        mapVotes.insert(found, std::make_pair(masternodeHash, vote));

    return true;
}

class SortByAmount
{
public:
    template <class T1, class T2>
    bool operator()(const T1& a, const T2& b) const
    {
        if (a->GetAmount() != b->GetAmount())
            return a->GetAmount() > b->GetAmount();
        else if (a->GetHash() != b->GetHash())
            return a->GetHash() < b->GetHash();
        else if (a->GetPayee() != b->GetPayee())
            return a->GetPayee() < b->GetPayee();
        else
            return false;
    }    
};

//evaluate if we should vote for this. Masternode only
bool CFinalizedBudget::AutoCheck()
{
    LOCK(cs);
    assert(boost::is_sorted(vecBudgetPayments, ComparePayments));

    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev)
        return false;

    LogPrintf("CFinalizedBudget::AutoCheck - %lli - %d\n", pindexPrev->nHeight, fAutoChecked);

    if(!fMasterNode || fAutoChecked)
        return false;

    // Auto-check votes with an interval that does not allow to submit votes to soon
    if (voteSubmittedTime && GetTime() - voteSubmittedTime.get() < FINAL_BUDGET_VOTE_UPDATE_MIN)
        return false;

    fAutoChecked = true; //we only need to check this once


    std::vector<CBudgetProposal*> vBudgetProposals = budget.GetBudget();

    
    boost::sort(vBudgetProposals, SortByAmount());

    for(unsigned int i = 0; i < vecBudgetPayments.size(); i++){
        LogPrintf("CFinalizedBudget::AutoCheck - nProp %d %s\n", i, vecBudgetPayments[i].nProposalHash.ToString());
        LogPrintf("CFinalizedBudget::AutoCheck - Payee %d %s\n", i, vecBudgetPayments[i].payee.ToString());
        LogPrintf("CFinalizedBudget::AutoCheck - nAmount %d %lli\n", i, vecBudgetPayments[i].nAmount);
    }

    for(unsigned int i = 0; i < vBudgetProposals.size(); i++){
        LogPrintf("CFinalizedBudget::AutoCheck - nProp %d %s\n", i, vBudgetProposals[i]->GetHash().ToString());
        LogPrintf("CFinalizedBudget::AutoCheck - Payee %d %s\n", i, vBudgetProposals[i]->GetPayee().ToString());
        LogPrintf("CFinalizedBudget::AutoCheck - nAmount %d %lli\n", i, vBudgetProposals[i]->GetAmount());
    }

    if(vBudgetProposals.size() == 0) {
        LogPrintf("CFinalizedBudget::AutoCheck - Can't get Budget, aborting\n");
        return false;
    }

    if(vBudgetProposals.size() != vecBudgetPayments.size()) {
        LogPrintf("CFinalizedBudget::AutoCheck - Budget length doesn't match\n");
        return false;
    }


    for(unsigned int i = 0; i < vecBudgetPayments.size(); i++){
        if(i > vBudgetProposals.size() - 1) {
            LogPrintf("CFinalizedBudget::AutoCheck - Vector size mismatch, aborting\n");
            return false;
        }

        if(vecBudgetPayments[i].nProposalHash != vBudgetProposals[i]->GetHash()){
            LogPrintf("CFinalizedBudget::AutoCheck - item #%d doesn't match %s %s\n", i, vecBudgetPayments[i].nProposalHash.ToString(), vBudgetProposals[i]->GetHash().ToString());
            return false;
        }

        // if(vecBudgetPayments[i].payee != vBudgetProposals[i]->GetPayee()){ -- triggered with false positive
        if(vecBudgetPayments[i].payee.ToString() != vBudgetProposals[i]->GetPayee().ToString()){
            LogPrintf("CFinalizedBudget::AutoCheck - item #%d payee doesn't match %s %s\n", i, vecBudgetPayments[i].payee.ToString(), vBudgetProposals[i]->GetPayee().ToString());
            return false;
        }

        if(vecBudgetPayments[i].nAmount != vBudgetProposals[i]->GetAmount()){
            LogPrintf("CFinalizedBudget::AutoCheck - item #%d payee doesn't match %lli %lli\n", i, vecBudgetPayments[i].nAmount, vBudgetProposals[i]->GetAmount());
            return false;
        }
    }

    LogPrintf("CFinalizedBudget::AutoCheck - Finalized Budget Matches! Submitting Vote.\n");
    SubmitVote();
    return true;
}
// If masternode voted for a proposal, but is now invalid -- remove the vote
void CFinalizedBudget::CleanAndRemove(bool fSignatureCheck)
{
    std::map<uint256, CFinalizedBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        (*it).second.fValid = (*it).second.SignatureValid(fSignatureCheck);
        ++it;
    }
}


CAmount CFinalizedBudget::GetTotalPayout() const
{
    CAmount ret = 0;

    for(unsigned int i = 0; i < vecBudgetPayments.size(); i++){
        ret += vecBudgetPayments[i].nAmount;
    }

    return ret;
}

std::string CFinalizedBudget::GetProposals() const
{
    LOCK(cs);
    assert(boost::is_sorted(vecBudgetPayments, ComparePayments));

    std::string ret = "";

    BOOST_FOREACH(const CTxBudgetPayment& budgetPayment, vecBudgetPayments)
    {
        CBudgetProposal* pbudgetProposal = budget.FindProposal(budgetPayment.nProposalHash);

        std::string token = budgetPayment.nProposalHash.ToString();

        if(pbudgetProposal) token = pbudgetProposal->GetName();
        if(ret == "") {ret = token;}
        else {ret += "," + token;}
    }
    return ret;
}

std::string CFinalizedBudget::GetStatus() const
{
    std::string retBadHashes = "";
    std::string retBadPayeeOrAmount = "";

    BOOST_FOREACH(const CTxBudgetPayment& payment, GetBudgetPayments())
    {
        CBudgetProposal* pbudgetProposal =  budget.FindProposal(payment.nProposalHash);
        if(!pbudgetProposal){
            if(retBadHashes == ""){
                retBadHashes = "Unknown proposal hash! Check this proposal before voting" + payment.nProposalHash.ToString();
            } else {
                retBadHashes += "," + payment.nProposalHash.ToString();
            }
        } else {
            if(pbudgetProposal->GetPayee() != payment.payee || pbudgetProposal->GetAmount() != payment.nAmount)
            {
                if(retBadPayeeOrAmount == ""){
                    retBadPayeeOrAmount = "Budget payee/nAmount doesn't match our proposal! " + payment.nProposalHash.ToString();
                } else {
                    retBadPayeeOrAmount += "," + payment.nProposalHash.ToString();
                }
            }
        }
    }

    if(retBadHashes == "" && retBadPayeeOrAmount == "") return "OK";

    return retBadHashes + retBadPayeeOrAmount;
}

uint256 CFinalizedBudget::GetHash() const
{
    CHashWriter stream(SER_GETHASH, PROTOCOL_VERSION);
    stream << nBlockStart;
    stream << vecBudgetPayments;

    return stream.GetHash();
}

bool CFinalizedBudget::IsValid(std::string& strError, bool fCheckCollateral) const
{
    assert(boost::is_sorted(vecBudgetPayments, ComparePayments));
    //must be the correct block for payment to happen (once a month)
    if(nBlockStart % GetBudgetPaymentCycleBlocks() != 0) {strError = "Invalid BlockStart"; return false;}
    if(GetBlockEnd() - nBlockStart > 100) {strError = "Invalid BlockEnd"; return false;}
    if((int)vecBudgetPayments.size() > 100) {strError = "Invalid budget payments count (too many)"; return false;}
    if(nBlockStart == 0) {strError = "Invalid BlockStart == 0"; return false;}

    //can only pay out 10% of the possible coins (min value of coins)
    if(GetTotalPayout() > budget.GetTotalBudget(nBlockStart)) {strError = "Invalid Payout (more than max)"; return false;}

    std::string strError2 = "";
    if(fCheckCollateral && !nFeeTXHash.IsNull()){
        if (nTime == 0)
            LogPrintf("CFinalizedBudget::IsValid - ERROR: nTime == 0 is unexpected\n");
        int nConf = 0;
        int64_t nTime;
        if(!IsBudgetCollateralValid(nFeeTXHash, GetHash(), strError2, nTime, nConf)){
            {strError = "Invalid Collateral : " + strError2; return false;}
        }
    }

    if (nFeeTXHash.IsNull())
    {
        const CMasternode* producer = mnodeman.Find(masternodeSubmittedId);
        if (producer == NULL)
        {
            strError = "Cannot find masternode : " + masternodeSubmittedId.ToString();
            return false;
        }

        if (!VerifySignature(producer->pubkey2))
        {
            strError = "Masternode signature is not valid : " + GetHash().ToString();
            return false;
        }
    }

    //TODO: if N cycles old, invalid, invalid

    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return true;

    if(nBlockStart < pindexPrev->nHeight-100) {strError = "Older than current blockHeight"; return false;}

    return true;
}

bool CFinalizedBudget::IsValid(bool fCheckCollateral) const
{
    std::string dummy;
    return IsValid(dummy, fCheckCollateral);
}

bool CFinalizedBudget::VerifySignature(const CPubKey& pubKey) const
{
    CPubKey result;
    if (!result.RecoverCompact(GetHash(), signature))
        return false;

    return result.GetID() == pubKey.GetID();
}

void CFinalizedBudget::ResetAutoChecked()
{
    if (!IsValid())
        return;

    fAutoChecked = false;
}

bool CFinalizedBudget::IsTransactionValid(const CTransaction& txNew, int nBlockHeight) const
{
    assert(boost::is_sorted(vecBudgetPayments, ComparePayments));

    if(nBlockHeight != GetBlockStart()) {
        LogPrintf("CFinalizedBudget::IsTransactionValid - Invalid block - height: %d start: %d\n", nBlockHeight, GetBlockStart());
        return false;
    }

    BOOST_FOREACH(const CTxBudgetPayment& payment, vecBudgetPayments)
    {
        bool found = false;
        BOOST_FOREACH(const CTxOut& out, txNew.vout)
        {
            if(payment.payee == out.scriptPubKey && payment.nAmount == out.nValue)
                found = true;
        }
        if(!found)
        {
            LogPrintf("CFinalizedBudget::IsTransactionValid - Missing required payment - %s: %d\n", ScriptToAddress(payment.payee).ToString(), payment.nAmount);
            return false;
        }
    }

    return true;
}

const std::vector<CTxBudgetPayment>& CFinalizedBudget::GetBudgetPayments() const
{
    return vecBudgetPayments;
}

void CFinalizedBudget::SubmitVote()
{
    CPubKey pubKeyMasternode;
    CKey keyMasternode;
    std::string errorMessage;

    if(!legacySigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode)){
        LogPrintf("CFinalizedBudget::SubmitVote - Error upon calling SetKey\n");
        return;
    }

    CFinalizedBudgetVote vote(activeMasternode.vin, GetHash());
    if(!vote.Sign(keyMasternode, pubKeyMasternode)){
        LogPrintf("CFinalizedBudget::SubmitVote - Failure to sign.");
        return;
    }

    std::string strError = "";
    if(budget.UpdateFinalizedBudget(vote, NULL, strError)){
        LogPrintf("CFinalizedBudget::SubmitVote  - new finalized budget vote - %s\n", vote.GetHash().ToString());

        budget.mapSeenFinalizedBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        vote.Relay();
        voteSubmittedTime = GetTime();
    } else {
        LogPrintf("CFinalizedBudget::SubmitVote : Error submitting vote - %s\n", strError);
    }
}

void CFinalizedBudget::MarkSynced()
{
    if (!fValid)
        return;

    for(std::map<uint256,CFinalizedBudgetVote>::iterator vote = mapVotes.begin(); vote != mapVotes.end(); ++vote)
    {
        if(vote->second.fValid)
            vote->second.fSynced = true;
    }
}

int CFinalizedBudget::Sync(CNode* pfrom, bool fPartial) const
{
    if (!fValid)
        return 0;

    int invCount = 0;
    pfrom->PushInventory(CInv(MSG_BUDGET_FINALIZED, GetHash()));
    ++invCount;

    //send votes
    for(std::map<uint256,CFinalizedBudgetVote>::const_iterator vote = mapVotes.begin(); vote != mapVotes.end(); ++vote)
    {
        if(!vote->second.fValid)
            continue;

        if((fPartial && !vote->second.fSynced) || !fPartial)
        {
            pfrom->PushInventory(CInv(MSG_BUDGET_FINALIZED_VOTE, vote->second.GetHash()));
            ++invCount;
        }
    }

    return invCount;
}

void CFinalizedBudget::ResetSync()
{
    if (!fValid)
        return;

    for(std::map<uint256,CFinalizedBudgetVote>::iterator vote = mapVotes.begin(); vote != mapVotes.end(); ++vote)
    {
        vote->second.fSynced = false;
    }
}

CFinalizedBudgetBroadcast::CFinalizedBudgetBroadcast()
{
}

CFinalizedBudgetBroadcast::CFinalizedBudgetBroadcast(int nBlockStart, const std::vector<CTxBudgetPayment>& vecBudgetPayments, uint256 nFeeTXHash)
    : nBlockStart(nBlockStart)
    , vecBudgetPayments(vecBudgetPayments)
    , nFeeTXHash(nFeeTXHash)
{
    boost::sort(this->vecBudgetPayments, CFinalizedBudget::ComparePayments);
}

CFinalizedBudgetBroadcast::CFinalizedBudgetBroadcast(int nBlockStart, const std::vector<CTxBudgetPayment>& vecBudgetPayments, const CTxIn& masternodeId, const CKey& keyMasternode)
    : nBlockStart(nBlockStart)
    , vecBudgetPayments(vecBudgetPayments)
    , masternodeSubmittedId(masternodeId)
{
    boost::sort(this->vecBudgetPayments, CFinalizedBudget::ComparePayments);

    if (!keyMasternode.SignCompact(GetHash(), signature))
        throw std::runtime_error("Cannot sign finalized budget: ");
}

CFinalizedBudget CFinalizedBudgetBroadcast::Budget() const
{
    if (nFeeTXHash != uint256())
        return CFinalizedBudget(nBlockStart, vecBudgetPayments, nFeeTXHash);
    else
        return CFinalizedBudget(nBlockStart, vecBudgetPayments, masternodeSubmittedId, signature);
}

uint256 CFinalizedBudgetBroadcast::GetHash() const
{
    CHashWriter stream(SER_GETHASH, PROTOCOL_VERSION);
    stream << nBlockStart;
    stream << vecBudgetPayments;

    return stream.GetHash();
}

void CFinalizedBudgetBroadcast::swap(CFinalizedBudgetBroadcast& first, CFinalizedBudgetBroadcast& second) // nothrow
{
    // enable ADL (not necessary in our case, but good practice)
    using std::swap;

    // by swapping the members of two classes,
    // the two classes are effectively swapped
    swap(first.nBlockStart, second.nBlockStart);
    first.vecBudgetPayments.swap(second.vecBudgetPayments);
    swap(first.nFeeTXHash, second.nFeeTXHash);
}

CFinalizedBudgetBroadcast& CFinalizedBudgetBroadcast::operator=(CFinalizedBudgetBroadcast from)
{
    swap(*this, from);
    return *this;
}

void CFinalizedBudgetBroadcast::Relay()
{
    assert(boost::is_sorted(vecBudgetPayments, CFinalizedBudget::ComparePayments));
    CInv inv(MSG_BUDGET_FINALIZED, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

CFinalizedBudgetVote::CFinalizedBudgetVote()
{
    vin = CTxIn();
    nBudgetHash = uint256();
    nTime = 0;
    vchSig.clear();
    fValid = true;
    fSynced = false;
}

CFinalizedBudgetVote::CFinalizedBudgetVote(CTxIn vinIn, uint256 nBudgetHashIn)
{
    vin = vinIn;
    nBudgetHash = nBudgetHashIn;
    nTime = GetAdjustedTime();
    vchSig.clear();
    fValid = true;
    fSynced = false;
}

void CFinalizedBudgetVote::Relay()
{
    CInv inv(MSG_BUDGET_FINALIZED_VOTE, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

bool CFinalizedBudgetVote::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    // Choose coins to use
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;

    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nBudgetHash.ToString() + boost::lexical_cast<std::string>(nTime);

    if(!legacySigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrintf("CFinalizedBudgetVote::Sign - Error upon calling SignMessage");
        return false;
    }

    if(!legacySigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CFinalizedBudgetVote::Sign - Error upon calling VerifyMessage");
        return false;
    }

    return true;
}

bool CFinalizedBudgetVote::SignatureValid(bool fSignatureCheck)
{
    std::string errorMessage;

    std::string strMessage = vin.prevout.ToStringShort() + nBudgetHash.ToString() + boost::lexical_cast<std::string>(nTime);

    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn == NULL)
    {
        LogPrint("mnbudget", "CFinalizedBudgetVote::SignatureValid() - Unknown Masternode\n");
        return false;
    }

    if(!fSignatureCheck) return true;

    if(!legacySigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("CFinalizedBudgetVote::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

std::string CBudgetManager::ToString() const
{
    std::ostringstream info;

    info << "Proposals: " << (int)mapProposals.size() <<
            ", Budgets: " << (int)mapFinalizedBudgets.size() <<
            ", Seen Budgets: " << (int)mapSeenMasternodeBudgetProposals.size() <<
            ", Seen Budget Votes: " << (int)mapSeenMasternodeBudgetVotes.size() <<
            ", Seen Final Budgets: " << (int)mapSeenFinalizedBudgets.size() <<
            ", Seen Final Budget Votes: " << (int)mapSeenFinalizedBudgetVotes.size();

    return info.str();
}


