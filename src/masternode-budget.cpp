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
std::vector<BudgetDraftBroadcast> vecImmatureBudgetDrafts;

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

CAmount BlocksBeforeSuperblockToSubmitBudgetDraft()
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
        return BlocksBeforeSuperblockToSubmitBudgetDraft();
    else
        return BlocksBeforeSuperblockToSubmitBudgetDraft() / 4; // 10 blocks for 50-block cycle
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
        if(ReceiveProposalVote(((*it1).second), NULL, strError)){
            LogPrintf("CBudgetManager::CheckOrphanVotes - Proposal/Budget is known, activating and removing orphan vote\n");
            mapOrphanMasternodeBudgetVotes.erase(it1++);
        } else {
            ++it1;
        }
    }
    std::map<uint256, BudgetDraftVote>::iterator it2 = mapOrphanBudgetDraftVotes.begin();
    while(it2 != mapOrphanBudgetDraftVotes.end()){
        if(UpdateBudgetDraft(((*it2).second),NULL, strError)){
            LogPrintf("CBudgetManager::CheckOrphanVotes - Proposal/Budget is known, activating and removing orphan vote\n");
            mapOrphanBudgetDraftVotes.erase(it2++);
        } else {
            ++it2;
        }
    }
}

void CBudgetManager::SubmitBudgetDraft()
{
    LOCK(cs);

    static int nSubmittedHeight = 0; // height at which final budget was submitted last time
    int nCurrentHeight;

    {
        TRY_LOCK(cs_main, locked);
        if(!locked) return;
        if(!chainActive.Tip()) return;
        nCurrentHeight = chainActive.Height();
    }

    const int blockStart = GetNextSuperblock(nCurrentHeight);
    if(nSubmittedHeight >= blockStart)
        return;

    if(blockStart - nCurrentHeight > BlocksBeforeSuperblockToSubmitBudgetDraft())
        return; // allow submitting final budget only when 2 days left before payments

    std::vector<CBudgetProposal*> vBudgetProposals = GetBudget();
    std::vector<CTxBudgetPayment> vecTxBudgetPayments;

    for(unsigned int i = 0; i < vBudgetProposals.size(); i++){
        CTxBudgetPayment txBudgetPayment;
        txBudgetPayment.nProposalHash = vBudgetProposals[i]->GetHash();
        txBudgetPayment.payee = vBudgetProposals[i]->GetPayee();
        txBudgetPayment.nAmount = vBudgetProposals[i]->GetAllotted();
        vecTxBudgetPayments.push_back(txBudgetPayment);
    }

    if(vecTxBudgetPayments.size() < 1) {
        LogPrintf("CBudgetManager::SubmitBudgetDraft - Found No Proposals For Period\n");
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
                LogPrintf("CBudgetManager::SubmitBudgetDraft - ERROR: Invalid masternodeprivkey: '%s'\n", strError);
                return;
            }

            BudgetDraftBroadcast budgetDraftBroadcast(blockStart, vecTxBudgetPayments, activeMasternode.vin, key2);

            if(mapSeenBudgetDrafts.count(budgetDraftBroadcast.GetHash())) {
                LogPrintf("CBudgetManager::SubmitBudgetDraft - Budget already exists - %s\n", budgetDraftBroadcast.GetHash().ToString());
                nSubmittedHeight = nCurrentHeight;
                return; //already exists
            }

            if(!budgetDraftBroadcast.IsValid(strError)){
                LogPrintf("CBudgetManager::SubmitBudgetDraft - Invalid finalized budget - %s \n", strError);
                return;
            }

            LOCK(cs);
            mapSeenBudgetDrafts.insert(make_pair(budgetDraftBroadcast.GetHash(), budgetDraftBroadcast));
            budgetDraftBroadcast.Relay();
            AddBudgetDraft(budgetDraftBroadcast.Budget());
            nSubmittedHeight = nCurrentHeight;
            LogPrintf("CBudgetManager::SubmitBudgetDraft - Done! %s\n", budgetDraftBroadcast.GetHash().ToString());
        }
    }
    else
    {
        BudgetDraftBroadcast tempBudget(blockStart, vecTxBudgetPayments, uint256());
        if(mapSeenBudgetDrafts.count(tempBudget.GetHash())) {
            LogPrintf("CBudgetManager::SubmitBudgetDraft - Budget already exists - %s\n", tempBudget.GetHash().ToString());
            nSubmittedHeight = nCurrentHeight;
            return; //already exists
        }

        //create fee tx
        CTransaction tx;
        uint256 txidCollateral;

        if(!mapCollateralTxids.count(tempBudget.GetHash())){
            CWalletTx wtx;
            if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, tempBudget.GetHash())){
                LogPrintf("CBudgetManager::SubmitBudgetDraft - Can't make collateral transaction\n");
                return;
            }

            // make our change address
            CReserveKey reservekey(pwalletMain);
            //send the tx to the network
            pwalletMain->CommitTransaction(wtx, reservekey);
            tx = (CTransaction)wtx;
            txidCollateral = tx.GetHash();
            mapCollateralTxids.insert(make_pair(tempBudget.GetHash(), txidCollateral));
        } else {
            txidCollateral = mapCollateralTxids[tempBudget.GetHash()];
        }

        int conf = GetIXConfirmations(tx.GetHash());
        CTransaction txCollateral;
        uint256 nBlockHash;

        if(!GetTransaction(txidCollateral, txCollateral, nBlockHash, true)) {
            LogPrintf ("CBudgetManager::SubmitBudgetDraft - Can't find collateral tx %s", txidCollateral.ToString());
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
            LogPrintf ("CBudgetManager::SubmitBudgetDraft - Collateral requires at least %d confirmations - %s - %d confirmations\n", BUDGET_FEE_CONFIRMATIONS+1, txidCollateral.ToString(), conf);
            return;
        }

        //create the proposal incase we're the first to make it
        BudgetDraftBroadcast budgetDraftBroadcast(blockStart, vecTxBudgetPayments, txidCollateral);

        std::string strError = "";
        if(!budgetDraftBroadcast.IsValid(strError)){
            LogPrintf("CBudgetManager::SubmitBudgetDraft - Invalid finalized budget - %s \n", strError);
            return;
        }

        LOCK(cs);
        mapSeenBudgetDrafts.insert(make_pair(budgetDraftBroadcast.GetHash(), budgetDraftBroadcast));
        budgetDraftBroadcast.Relay();
        AddBudgetDraft(budgetDraftBroadcast.Budget());
        nSubmittedHeight = nCurrentHeight;
        LogPrintf("CBudgetManager::SubmitBudgetDraft - Done! %s\n", budgetDraftBroadcast.GetHash().ToString());
    }
}

bool CBudgetManager::AddBudgetDraft(const BudgetDraft &budgetDraft, bool checkCollateral)
{
    LOCK(cs);

    std::string strError = "";
    if(!budgetDraft.IsValid(strError, checkCollateral))
        return false;

    if(mapBudgetDrafts.count(budgetDraft.GetHash())) {
        return false;
    }

    mapBudgetDrafts.insert(make_pair(budgetDraft.GetHash(), budgetDraft));
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
    mapSeenMasternodeBudgetProposals.insert(make_pair(budgetProposal.GetHash(), budgetProposal));
    return true;
}

void CBudgetManager::CheckAndRemove()
{
    LOCK(cs);

    LogPrintf("CBudgetManager::CheckAndRemove\n");

    std::string strError = "";

    LogPrintf("CBudgetManager::CheckAndRemove - mapBudgetDrafts cleanup - size: %d\n", mapBudgetDrafts.size());
    std::map<uint256, BudgetDraft>::iterator it = mapBudgetDrafts.begin();
    while(it != mapBudgetDrafts.end())
    {
        BudgetDraft* pbudgetDraft = &((*it).second);

        bool isValid = pbudgetDraft->fValid = pbudgetDraft->IsValid(strError);
        LogPrintf("CBudgetManager::CheckAndRemove - pbudgetDraft->IsValid - strError: %s\n", strError);
        if(isValid) {
            if(Params().NetworkID() == CBaseChainParams::TESTNET || Params().NetworkID() == CBaseChainParams::MAIN && rand() % 4 == 0)
            {
                //do this 1 in 4 blocks -- spread out the voting activity on mainnet
                // -- this function is only called every sixth block, so this is really 1 in 24 blocks
                pbudgetDraft->AutoCheck();
            }
            else
            {
                LogPrintf("BudgetDraft::AutoCheck - waiting\n");
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

const BudgetDraft* CBudgetManager::GetMostVotedBudget(int height) const
{
    const BudgetDraft* budgetToPay = NULL;
    typedef std::map<uint256, BudgetDraft>::const_iterator BudgetDraftIterator;
    for (BudgetDraftIterator i = mapBudgetDrafts.begin(); i != mapBudgetDrafts.end(); ++i)
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

void CBudgetManager::FillBlockPayee(CMutableTransaction& txNew, CAmount nFees, std::vector<CTxOut>& voutSuperblockRet) const
{
    assert (txNew.vout.size() == 1); // There is a blank for block creator's reward

    LOCK(cs);

    const CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev)
        return;

    // Pay the miner

    txNew.vout[0].nValue = GetBlockValue(pindexPrev->nHeight, nFees);

    // Find finalized budgets with the most votes

    const BudgetDraft* budgetToPay = GetMostVotedBudget(pindexPrev->nHeight + 1);
    if (budgetToPay == NULL)
        return;

    // make sure it's empty, just in case
    voutSuperblockRet.clear();

    // Pay the proposals

    BOOST_FOREACH(const CTxBudgetPayment& payment, budgetToPay->GetBudgetPayments())
    {
        LogPrintf("CBudgetManager::FillBlockPayee - Budget payment to %s for %lld; proposal %s\n",
            ScriptToAddress(payment.payee).ToString(), payment.nAmount, payment.nProposalHash.ToString());

        CTxOut txout = CTxOut(payment.nAmount, payment.payee);
        txNew.vout.push_back(txout);
        voutSuperblockRet.push_back(txout);
    }
}

BudgetDraft* CBudgetManager::FindBudgetDraft(uint256 nHash)
{
    LOCK(cs);

    std::map<uint256, BudgetDraft>::iterator found = mapBudgetDrafts.find(nHash);
    if (found != mapBudgetDrafts.end())
        return NULL;

    return &found->second;
}

CBudgetProposal* CBudgetManager::FindProposal(const std::string &strProposalName)
{
    LOCK(cs);

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
    LOCK(cs);

    const BudgetDraft* bestBudget = GetMostVotedBudget(nBlockHeight);
    if (bestBudget == NULL)
        return false;

    // If budget doesn't have 5% of the masternode votes, we should not pay it
    return (20 * bestBudget->GetVoteCount() > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION));
}

bool CBudgetManager::IsTransactionValid(const CTransaction& txNew, int nBlockHeight) const
{
    LOCK(cs);

    const BudgetDraft* bestBudget = GetMostVotedBudget(nBlockHeight);
    const int mnodeCount = mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION);

    // If budget doesn't have 5% of the network votes, then we should not pay it
    if (bestBudget == NULL || 20 * bestBudget->GetVoteCount() < mnodeCount)
        return false;

    // Check the highest finalized budgets (+/- 10% to assist in consensus)
    for (std::map<uint256, BudgetDraft>::const_iterator it = mapBudgetDrafts.begin(); it != mapBudgetDrafts.end(); ++it)
    {
        const BudgetDraft& pbudgetDraft = it->second;

        if (10 * (bestBudget->GetVoteCount() - pbudgetDraft.GetVoteCount()) > mnodeCount)
            continue;

        if(pbudgetDraft.IsTransactionValid(txNew, nBlockHeight))
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

    const int blockStart = GetNextSuperblock(pindexPrev->nHeight);
    const int blockEnd  =  blockStart + GetBudgetPaymentCycleBlocks() - 1;
    CAmount totalBudget = GetTotalBudget(blockStart);


    std::vector<std::pair<CBudgetProposal*, int> >::iterator it2 = vBudgetPorposalsSort.begin();
    while(it2 != vBudgetPorposalsSort.end())
    {
        CBudgetProposal* pbudgetProposal = (*it2).first;

        //prop start/end should be inside this period
        if(pbudgetProposal->fValid && pbudgetProposal->nBlockStart <= blockStart &&
                pbudgetProposal->nBlockEnd >= blockEnd &&
                pbudgetProposal->GetYeas() - pbudgetProposal->GetNays() > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10 && 
                pbudgetProposal->IsEstablished())
        {
            if(pbudgetProposal->GetAmount() + nBudgetAllocated <= totalBudget) {
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

struct SortBudgetDraftsByVotes
{
    bool operator()(const std::pair<BudgetDraft*, int> &left, const std::pair<BudgetDraft*, int> &right) const
    {
        return left.second > right.second;
    }
};

std::vector<BudgetDraft*> CBudgetManager::GetBudgetDrafts()
{
    LOCK(cs);

    std::vector<BudgetDraft*> budgetDrafts;
    std::vector<std::pair<BudgetDraft*, int> > budgetDraftsSorted;

    // ------- Grab The Budgets In Order

    std::map<uint256, BudgetDraft>::iterator it = mapBudgetDrafts.begin();
    while(it != mapBudgetDrafts.end())
    {
        BudgetDraft* pbudgetDraft = &((*it).second);

        budgetDraftsSorted.push_back(make_pair(pbudgetDraft, pbudgetDraft->GetVoteCount()));
        ++it;
    }
    std::sort(budgetDraftsSorted.begin(), budgetDraftsSorted.end(), SortBudgetDraftsByVotes());

    std::vector<std::pair<BudgetDraft*, int> >::iterator it2 = budgetDraftsSorted.begin();
    while(it2 != budgetDraftsSorted.end())
    {
        budgetDrafts.push_back((*it2).first);
        ++it2;
    }

    return budgetDrafts;
}

std::string CBudgetManager::GetRequiredPaymentsString(int nBlockHeight) const
{
    LOCK(cs);

    std::string ret = "unknown-budget";

    const BudgetDraft* pbudgetDraft = GetMostVotedBudget(nBlockHeight);
    if (pbudgetDraft == NULL)
        return ret;

    BOOST_FOREACH(const CTxBudgetPayment& payment, pbudgetDraft->GetBudgetPayments())
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
        SubmitBudgetDraft();

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

    LogPrintf("CBudgetManager::NewBlock - mapBudgetDrafts cleanup - size: %d\n", mapBudgetDrafts.size());
    std::map<uint256, BudgetDraft>::iterator it3 = mapBudgetDrafts.begin();
    while(it3 != mapBudgetDrafts.end()){
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

    LogPrintf("CBudgetManager::NewBlock - vecImmatureBudgetDrafts cleanup - size: %d\n", vecImmatureBudgetDrafts.size());
    std::vector<BudgetDraftBroadcast>::iterator it5 = vecImmatureBudgetDrafts.begin();
    while(it5 != vecImmatureBudgetDrafts.end())
    {
        std::string strError = "";
        int nConf = 0;
        int64_t nTime = 0;
        if(it5->IsSubmittedManually() && !IsBudgetCollateralValid(it5->GetFeeTxHash(), it5->GetHash(), strError, nTime, nConf)){
            ++it5;
            continue;
        }

        const CMasternode* producer = mnodeman.Find(it5->MasternodeSubmittedId());
        if (producer == NULL)
            continue;

        if(!it5->IsValid(strError)) {
            LogPrintf("fbs (immature) - invalid finalized budget - %s\n", strError);
            it5 = vecImmatureBudgetDrafts.erase(it5);
            continue;
        }

        LogPrintf("fbs (immature) - new finalized budget - %s\n", it5->GetHash().ToString());

        if(AddBudgetDraft(it5->Budget()))
            it5->Relay();

        it5 = vecImmatureBudgetDrafts.erase(it5);
    }
    LogPrintf("CBudgetManager::NewBlock - PASSED\n");
}

void CBudgetManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    // lite mode is not supported
    if(fLiteMode) return;
    if(!masternodeSync.IsBlockchainSynced()) return;

    LOCK(cs);

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
        BudgetDraftBroadcast budgetDraftBroadcast;
        vRecv >> budgetDraftBroadcast;

        std::map<uint256, BudgetDraftBroadcast>::const_iterator found = mapSeenBudgetDrafts.find(budgetDraftBroadcast.GetHash());
        if(found != mapSeenBudgetDrafts.end())
        {
            // If someone submitted a valid budget automatically and another one submitted the same budget manually -
            // relay both no matter what order they got in
            if (found->second.IsSubmittedManually() != budgetDraftBroadcast.IsSubmittedManually())
                budgetDraftBroadcast.Relay();

            masternodeSync.AddedBudgetItem(budgetDraftBroadcast.GetHash());
            return;
        }

        std::string strError = "";
        int nConf = 0;
        int64_t nTime = 0;
        if(budgetDraftBroadcast.IsSubmittedManually() && !IsBudgetCollateralValid(budgetDraftBroadcast.GetFeeTxHash(), budgetDraftBroadcast.GetHash(), strError, nTime, nConf)){
            LogPrintf("Finalized Budget FeeTX is not valid - %s - %s\n", budgetDraftBroadcast.GetFeeTxHash().ToString(), strError);

            if(nConf >= 1) vecImmatureBudgetDrafts.push_back(budgetDraftBroadcast);
            return;
        }

        if (!budgetDraftBroadcast.IsSubmittedManually())
        {
            const CMasternode* producer = mnodeman.Find(budgetDraftBroadcast.MasternodeSubmittedId());
            if (producer == NULL)
            {
                LogPrintf("fbs - unknown masternode - vin: %s\n", budgetDraftBroadcast.MasternodeSubmittedId().ToString());
                mnodeman.AskForMN(pfrom, budgetDraftBroadcast.MasternodeSubmittedId());
                vecImmatureBudgetDrafts.push_back(budgetDraftBroadcast);
                return;
            }

            if (!budgetDraftBroadcast.Budget().VerifySignature(producer->pubkey2))
            {
                Misbehaving(pfrom->GetId(), 50);
            }
        }

        mapSeenBudgetDrafts.insert(make_pair(budgetDraftBroadcast.GetHash(), budgetDraftBroadcast));

        if(!budgetDraftBroadcast.IsValid(strError)) {
            LogPrintf("fbs - invalid finalized budget - %s\n", strError);
            return;
        }

        LogPrintf("fbs - new finalized budget - %s\n", budgetDraftBroadcast.GetHash().ToString());

        if(AddBudgetDraft(budgetDraftBroadcast.Budget()))
        {
            budgetDraftBroadcast.Relay();
            masternodeSync.AddedBudgetItem(budgetDraftBroadcast.GetHash());
        }

        //we might have active votes for this budget that are now valid
        CheckOrphanVotes();
    }

    if (strCommand == "fbvote") { //Finalized Budget Vote
        BudgetDraftVote vote;
        vRecv >> vote;
        vote.fValid = true;

        if(mapSeenBudgetDraftVotes.count(vote.GetHash())){
            masternodeSync.AddedBudgetItem(vote.GetHash());
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vin);
        if(pmn == NULL) {
            LogPrint("mnbudget", "fbvote - unknown masternode - vin: %s\n", vote.vin.ToString());
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        mapSeenBudgetDraftVotes.insert(make_pair(vote.GetHash(), vote));
        if(!vote.SignatureValid(true)){
            LogPrintf("fbvote - signature invalid\n");
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        std::string strError = "";
        if(UpdateBudgetDraft(vote, pfrom, strError)) {
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

    std::map<uint256, BudgetDraftBroadcast>::iterator it3 = mapSeenBudgetDrafts.begin();
    while(it3 != mapSeenBudgetDrafts.end()){
        BudgetDraft* pbudgetDraft = FindBudgetDraft((*it3).first);
        if (pbudgetDraft)
            pbudgetDraft->ResetSync();
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

    std::map<uint256, BudgetDraftBroadcast>::iterator it3 = mapSeenBudgetDrafts.begin();
    while(it3 != mapSeenBudgetDrafts.end()){
        BudgetDraft* pbudgetDraft = FindBudgetDraft((*it3).first);
        if(pbudgetDraft)
            pbudgetDraft->MarkSynced();
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

    std::map<uint256, BudgetDraftBroadcast>::const_iterator it3 = mapSeenBudgetDrafts.begin();
    while(it3 != mapSeenBudgetDrafts.end()){
        std::map<uint256, BudgetDraft>::const_iterator pbudgetDraft = mapBudgetDrafts.find((*it3).first);
        if(pbudgetDraft != mapBudgetDrafts.end() && (nProp.IsNull() || (*it3).first == nProp))
            pbudgetDraft->second.Sync(pfrom, fPartial);
        ++it3;
    }

    pfrom->PushMessage("ssc", MASTERNODE_SYNC_BUDGET_FIN, nInvCount);
    LogPrintf("CBudgetManager::Sync - sent %d items\n", nInvCount);

}

const BudgetDraftBroadcast* CBudgetManager::GetSeenBudgetDraft(uint256 hash) const
{
    LOCK(cs);

    std::map<uint256, BudgetDraftBroadcast>::const_iterator found = mapSeenBudgetDrafts.find(hash);
    if (found == mapSeenBudgetDrafts.end())
        return NULL;
    else
        return &found->second;
}

const BudgetDraftVote* CBudgetManager::GetSeenBudgetDraftVote(uint256 hash) const
{
    LOCK(cs);

    std::map<uint256, BudgetDraftVote>::const_iterator found = mapSeenBudgetDraftVotes.find(hash);
    if (found == mapSeenBudgetDraftVotes.end())
        return NULL;
    else
        return &found->second;
}

const CBudgetProposalBroadcast* CBudgetManager::GetSeenProposal(uint256 hash) const
{
    LOCK(cs);

    std::map<uint256, CBudgetProposalBroadcast>::const_iterator found = mapSeenMasternodeBudgetProposals.find(hash);
    if (found == mapSeenMasternodeBudgetProposals.end())
        return NULL;
    else
        return &found->second;
}
const CBudgetVote* CBudgetManager::GetSeenVote(uint256 hash) const
{
    LOCK(cs);

    std::map<uint256, CBudgetVote>::const_iterator found = mapSeenMasternodeBudgetVotes.find(hash);
    if (found == mapSeenMasternodeBudgetVotes.end())
        return NULL;
    else
        return &found->second;
}


bool CBudgetManager::SubmitProposalVote(const CBudgetVote& vote, std::string& strError)
{
    LOCK(cs);

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
    if (proposal.AddOrUpdateVote(vote, strError))
    {
        mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        return true;
    }
}

bool CBudgetManager::CanSubmitVotes(int blockStart, int blockEnd)
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
        for (map<uint256, BudgetDraft>::iterator i = mapBudgetDrafts.begin(); i != mapBudgetDrafts.end(); ++i) {
            BudgetDraft& budgetDraft = i->second;

            if (budgetDraft.IsValid() && !budgetDraft.IsVoteSubmitted())
                budgetDraft.ResetAutoChecked();
        }

    }

    return true;
}

bool CBudgetManager::UpdateBudgetDraft(const BudgetDraftVote& vote, CNode* pfrom, std::string& strError)
{
    LOCK(cs);

    if(!mapBudgetDrafts.count(vote.nBudgetHash)){
        if(pfrom){
            // only ask for missing items after our syncing process is complete -- 
            //   otherwise we'll think a full sync succeeded when they return a result
            if(!masternodeSync.IsSynced()) return false;

            LogPrintf("CBudgetManager::UpdateBudgetDraft - Unknown Finalized Proposal %s, asking for source budget\n", vote.nBudgetHash.ToString());
            mapOrphanBudgetDraftVotes[vote.nBudgetHash] = vote;

            if(!askedForSourceProposalOrBudget.count(vote.nBudgetHash)){
                pfrom->PushMessage("mnvs", vote.nBudgetHash);
                askedForSourceProposalOrBudget[vote.nBudgetHash] = GetTime();
            }

        }

        strError = "Finalized Budget not found!";
        return false;
    }

    bool isOldVote = false;

    for (std::map<uint256, BudgetDraft>::iterator i = mapBudgetDrafts.begin(); i != mapBudgetDrafts.end(); ++i)
    {
        const std::map<uint256, BudgetDraftVote>& votes = i->second.GetVotes();
        const std::map<uint256, BudgetDraftVote>::const_iterator found = votes.find(vote.vin.prevout.GetHash());
        if (found != votes.end() && found->second.nTime > vote.nTime)
            isOldVote = true;
    }

    if (!mapBudgetDrafts[vote.nBudgetHash].AddOrUpdateVote(isOldVote, vote, strError))
        return false;

    for (std::map<uint256, BudgetDraft>::iterator i = mapBudgetDrafts.begin(); i != mapBudgetDrafts.end(); ++i)
    {
        i->second.DiscontinueOlderVotes(vote);
    }

    mapSeenBudgetDraftVotes.insert(make_pair(vote.GetHash(), vote));
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
    LOCK(cs);
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

BudgetDraft::BudgetDraft()
{
    m_blockStart = 0;
    m_payments.clear();
    m_votes.clear();
    m_feeTransactionHash = uint256();
    fValid = true;
    m_autoChecked = false;
    m_voteSubmittedTime = boost::none;
}

BudgetDraft::BudgetDraft(int nBlockStart, const std::vector<CTxBudgetPayment>& vecBudgetPayments, uint256 nFeeTXHash)
    : fValid(true)
    , m_autoChecked(false)
    , m_payments(vecBudgetPayments)
    , m_blockStart(nBlockStart)
    , m_feeTransactionHash(nFeeTXHash)
{
    boost::sort(this->m_payments, ComparePayments);
}

BudgetDraft::BudgetDraft(int nBlockStart, const std::vector<CTxBudgetPayment>& vecBudgetPayments, const CTxIn& masternodeId, const CKey& keyMasternode)
    : fValid(true)
    , m_autoChecked(false)
    , m_payments(vecBudgetPayments)
    , m_blockStart(nBlockStart)
    , m_masternodeSubmittedId(masternodeId)
{
    boost::sort(this->m_payments, ComparePayments);

    if (!keyMasternode.SignCompact(GetHash(), m_signature))
        throw std::runtime_error("Cannot sign finalized budget: ");
}

BudgetDraft::BudgetDraft(int nBlockStart, const std::vector<CTxBudgetPayment>& vecBudgetPayments, const CTxIn& masternodeId, const std::vector<unsigned char>& signature)
    : fValid(true)
    , m_autoChecked(false)
    , m_payments(vecBudgetPayments)
    , m_blockStart(nBlockStart)
    , m_masternodeSubmittedId(masternodeId)
    , m_signature(signature)
{
    boost::sort(this->m_payments, ComparePayments);
}

BudgetDraft::BudgetDraft(const BudgetDraft& other)
    : fValid(true)
    , m_autoChecked(false)
    , m_payments(other.m_payments)
    , m_blockStart(other.m_blockStart)
    , m_votes(other.m_votes)
    , m_feeTransactionHash(other.m_feeTransactionHash)
    , m_signature(other.m_signature)
    , m_masternodeSubmittedId(other.m_masternodeSubmittedId)
    , m_voteSubmittedTime(boost::none)
{
    assert(boost::is_sorted(m_payments, ComparePayments));
}

void BudgetDraft::DiscontinueOlderVotes(const BudgetDraftVote& newerVote)
{
    LOCK(m_cs);

    std::map<uint256, BudgetDraftVote>::iterator found = m_votes.find(newerVote.vin.prevout.GetHash());
    if (found == m_votes.end())
        return;

    if (found->second.nTime < newerVote.nTime)
    {
        m_obsoleteVotes.insert(std::make_pair(newerVote.vin.prevout.GetHash(), found->second));
        m_votes.erase(found);
    }
}

bool BudgetDraft::AddOrUpdateVote(bool isOldVote, const BudgetDraftVote& vote, std::string& strError)
{
    LOCK(m_cs);

    if (isOldVote)
        return AddOrUpdateVote(m_obsoleteVotes, vote, strError);
    else
        return AddOrUpdateVote(m_votes, vote, strError);
}

bool BudgetDraft::AddOrUpdateVote(std::map<uint256, BudgetDraftVote>& votes, const BudgetDraftVote& vote, std::string& strError)
{
    uint256 masternodeHash = vote.vin.prevout.GetHash();
    map<uint256, BudgetDraftVote>::iterator found = votes.find(masternodeHash);

    if(found != votes.end()){
        const BudgetDraftVote& previousVote = found->second;
        if (previousVote.GetHash() == vote.GetHash()) {
            LogPrint("mnbudget", "BudgetDraft::AddOrUpdateVote - Already have the vote\n");
            return true;
        }
        if(previousVote.nTime > vote.nTime) {
            strError = strprintf("new vote older than existing vote - %s\n", vote.GetHash().ToString());
            LogPrint("mnbudget", "BudgetDraft::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        if(vote.nTime - previousVote.nTime < FINAL_BUDGET_VOTE_UPDATE_MIN) {
            strError = strprintf("time between votes is too soon - %s - %lli\n", vote.GetHash().ToString(), vote.nTime - previousVote.nTime);
            LogPrint("mnbudget", "BudgetDraft::AddOrUpdateVote - %s\n", strError);
            return false;
        }
    }

    if(vote.nTime > GetTime() + (60*60)){
        strError = strprintf("new vote is too far ahead of current time - %s - nTime %lli - Max Time %lli\n", vote.GetHash().ToString(), vote.nTime, GetTime() + (60*60));
        LogPrint("mnbudget", "BudgetDraft::AddOrUpdateVote - %s\n", strError);
        return false;
    }

    votes.insert(found, std::make_pair(masternodeHash, vote));

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
bool BudgetDraft::AutoCheck()
{
    {
        LOCK(m_cs);
        assert(boost::is_sorted(m_payments, ComparePayments));

        CBlockIndex* pindexPrev = chainActive.Tip();
        if(!pindexPrev)
            return false;

        LogPrintf("BudgetDraft::AutoCheck - %lli - %d\n", pindexPrev->nHeight, m_autoChecked);

        if(!fMasterNode || m_autoChecked)
            return false;

        // Auto-check votes with an interval that does not allow to submit votes to soon
        if (m_voteSubmittedTime && GetTime() - m_voteSubmittedTime.get() < FINAL_BUDGET_VOTE_UPDATE_MIN)
            return false;

        m_autoChecked = true; //we only need to check this once
    }

    std::vector<CBudgetProposal*> vBudgetProposals = budget.GetBudget();

    {
        LOCK(m_cs);

        boost::sort(vBudgetProposals, SortByAmount());

        for(unsigned int i = 0; i < m_payments.size(); i++){
            LogPrintf("BudgetDraft::AutoCheck - nProp %d %s\n", i, m_payments[i].nProposalHash.ToString());
            LogPrintf("BudgetDraft::AutoCheck - Payee %d %s\n", i, m_payments[i].payee.ToString());
            LogPrintf("BudgetDraft::AutoCheck - nAmount %d %lli\n", i, m_payments[i].nAmount);
        }

        for(unsigned int i = 0; i < vBudgetProposals.size(); i++){
            LogPrintf("BudgetDraft::AutoCheck - nProp %d %s\n", i, vBudgetProposals[i]->GetHash().ToString());
            LogPrintf("BudgetDraft::AutoCheck - Payee %d %s\n", i, vBudgetProposals[i]->GetPayee().ToString());
            LogPrintf("BudgetDraft::AutoCheck - nAmount %d %lli\n", i, vBudgetProposals[i]->GetAmount());
        }

        if(vBudgetProposals.size() == 0) {
            LogPrintf("BudgetDraft::AutoCheck - Can't get Budget, aborting\n");
            return false;
        }

        if(vBudgetProposals.size() != m_payments.size()) {
            LogPrintf("BudgetDraft::AutoCheck - Budget length doesn't match\n");
            return false;
        }


        for(unsigned int i = 0; i < m_payments.size(); i++){
            if(i > vBudgetProposals.size() - 1) {
                LogPrintf("BudgetDraft::AutoCheck - Vector size mismatch, aborting\n");
                return false;
            }

            if(m_payments[i].nProposalHash != vBudgetProposals[i]->GetHash()){
                LogPrintf("BudgetDraft::AutoCheck - item #%d doesn't match %s %s\n", i, m_payments[i].nProposalHash.ToString(), vBudgetProposals[i]->GetHash().ToString());
                return false;
            }

            if(m_payments[i].payee.ToString() != vBudgetProposals[i]->GetPayee().ToString()){
                LogPrintf("BudgetDraft::AutoCheck - item #%d payee doesn't match %s %s\n", i, m_payments[i].payee.ToString(), vBudgetProposals[i]->GetPayee().ToString());
                return false;
            }

            if(m_payments[i].nAmount != vBudgetProposals[i]->GetAmount()){
                LogPrintf("BudgetDraft::AutoCheck - item #%d payee doesn't match %lli %lli\n", i, m_payments[i].nAmount, vBudgetProposals[i]->GetAmount());
                return false;
            }
        }

        LogPrintf("BudgetDraft::AutoCheck - Finalized Budget Matches! Submitting Vote.\n");
        SubmitVote();
        return true;
    }
}
// If masternode voted for a proposal, but is now invalid -- remove the vote
void BudgetDraft::CleanAndRemove(bool fSignatureCheck)
{
    LOCK(m_cs);
    std::map<uint256, BudgetDraftVote>::iterator it = m_votes.begin();

    while(it != m_votes.end()) {
        (*it).second.fValid = (*it).second.SignatureValid(fSignatureCheck);
        ++it;
    }
}


CAmount BudgetDraft::GetTotalPayout() const
{
    LOCK(m_cs);

    CAmount ret = 0;

    for(unsigned int i = 0; i < m_payments.size(); i++){
        ret += m_payments[i].nAmount;
    }

    return ret;
}

std::string BudgetDraft::GetProposals() const
{
    std::vector<CTxBudgetPayment> payments = GetBudgetPayments();
    std::string ret = "";

    BOOST_FOREACH(const CTxBudgetPayment& budgetPayment, payments)
    {
        CBudgetProposal* pbudgetProposal = budget.FindProposal(budgetPayment.nProposalHash);

        std::string token = budgetPayment.nProposalHash.ToString();

        if(pbudgetProposal) token = pbudgetProposal->GetName();
        if(ret == "") {ret = token;}
        else {ret += "," + token;}
    }
    return ret;
}

bool BudgetDraft::ComparePayments(const CTxBudgetPayment& a, const CTxBudgetPayment& b)
{
    if (a.nAmount != b.nAmount)
        return a.nAmount > b.nAmount;
    else if (a.nProposalHash != b.nProposalHash)
        return a.nProposalHash < b.nProposalHash;
    else if (a.payee != b.payee)
        return a.payee < b.payee;
    else
        return false;
}

std::string BudgetDraft::GetStatus() const
{
    std::vector<CTxBudgetPayment> payments = GetBudgetPayments();

    std::string retBadHashes = "";
    std::string retBadPayeeOrAmount = "";

    BOOST_FOREACH(const CTxBudgetPayment& payment, payments)
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

uint256 BudgetDraft::GetHash() const
{
    CHashWriter stream(SER_GETHASH, PROTOCOL_VERSION);
    stream << m_blockStart;
    stream << m_payments;

    return stream.GetHash();
}

bool BudgetDraft::IsValid(std::string& strError, bool fCheckCollateral) const
{
    LOCK(m_cs);

    assert(boost::is_sorted(m_payments, ComparePayments));
    //must be the correct block for payment to happen (once a month)
    if(m_blockStart % GetBudgetPaymentCycleBlocks() != 0) {strError = "Invalid BlockStart"; return false;}
    if(GetBlockEnd() - m_blockStart > 100) {strError = "Invalid BlockEnd"; return false;}
    if((int)m_payments.size() > 100) {strError = "Invalid budget payments count (too many)"; return false;}
    if(m_blockStart == 0) {strError = "Invalid BlockStart == 0"; return false;}

    //can only pay out 10% of the possible coins (min value of coins)
    if(GetTotalPayout() > budget.GetTotalBudget(m_blockStart)) {strError = "Invalid Payout (more than max)"; return false;}

    std::string strError2 = "";
    if(fCheckCollateral && !m_feeTransactionHash.IsNull()){
        int nConf = 0;
        int64_t nTime;
        if(!IsBudgetCollateralValid(m_feeTransactionHash, GetHash(), strError2, nTime, nConf)){
            {strError = "Invalid Collateral : " + strError2; return false;}
        }
    }

    if (m_feeTransactionHash.IsNull())
    {
        const CMasternode* producer = mnodeman.Find(m_masternodeSubmittedId);
        if (producer == NULL)
        {
            strError = "Cannot find masternode : " + m_masternodeSubmittedId.ToString();
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

    if(m_blockStart < pindexPrev->nHeight-100) {strError = "Older than current blockHeight"; return false;}

    return true;
}

bool BudgetDraft::IsValid(bool fCheckCollateral) const
{
    std::string dummy;
    return IsValid(dummy, fCheckCollateral);
}

bool BudgetDraft::IsSubmittedManually() const
{
    LOCK(m_cs);

    return m_feeTransactionHash != uint256();
}

bool BudgetDraft::VerifySignature(const CPubKey& pubKey) const
{
    CPubKey result;
    if (!result.RecoverCompact(GetHash(), m_signature))
        return false;

    return result.GetID() == pubKey.GetID();
}

void BudgetDraft::ResetAutoChecked()
{
    if (!IsValid())
        return;

    m_autoChecked = false;
}

bool BudgetDraft::IsTransactionValid(const CTransaction& txNew, int nBlockHeight) const
{
    LOCK(m_cs);

    assert(boost::is_sorted(m_payments, ComparePayments));

    if(nBlockHeight != GetBlockStart()) {
        LogPrintf("BudgetDraft::IsTransactionValid - Invalid block - height: %d start: %d\n", nBlockHeight, GetBlockStart());
        return false;
    }

    BOOST_FOREACH(const CTxBudgetPayment& payment, m_payments)
    {
        bool found = false;
        BOOST_FOREACH(const CTxOut& out, txNew.vout)
        {
            if(payment.payee == out.scriptPubKey && payment.nAmount == out.nValue)
                found = true;
        }
        if(!found)
        {
            LogPrintf("BudgetDraft::IsTransactionValid - Missing required payment - %s: %d\n", ScriptToAddress(payment.payee).ToString(), payment.nAmount);
            return false;
        }
    }

    return true;
}

const std::vector<CTxBudgetPayment>& BudgetDraft::GetBudgetPayments() const
{
    LOCK(m_cs);
    assert(boost::is_sorted(m_payments, ComparePayments));

    return m_payments;
}

void BudgetDraft::SubmitVote()
{
    CPubKey pubKeyMasternode;
    CKey keyMasternode;
    std::string errorMessage;

    if(!legacySigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode)){
        LogPrintf("BudgetDraft::SubmitVote - Error upon calling SetKey\n");
        return;
    }

    BudgetDraftVote vote(activeMasternode.vin, GetHash());
    if(!vote.Sign(keyMasternode, pubKeyMasternode)){
        LogPrintf("BudgetDraft::SubmitVote - Failure to sign.");
        return;
    }

    std::string strError = "";
    if(budget.UpdateBudgetDraft(vote, NULL, strError)){
        LogPrintf("BudgetDraft::SubmitVote  - new finalized budget vote - %s\n", vote.GetHash().ToString());

        vote.Relay();
        m_voteSubmittedTime = GetTime();
    } else {
        LogPrintf("BudgetDraft::SubmitVote : Error submitting vote - %s\n", strError);
    }
}

void BudgetDraft::MarkSynced()
{
    LOCK(m_cs);

    if (!fValid)
        return;

    for(std::map<uint256,BudgetDraftVote>::iterator vote = m_votes.begin(); vote != m_votes.end(); ++vote)
    {
        if(vote->second.fValid)
            vote->second.fSynced = true;
    }
}

int BudgetDraft::Sync(CNode* pfrom, bool fPartial) const
{
    LOCK(m_cs);

    if (!fValid)
        return 0;

    int invCount = 0;
    pfrom->PushInventory(CInv(MSG_BUDGET_FINALIZED, GetHash()));
    ++invCount;

    //send votes
    for(std::map<uint256,BudgetDraftVote>::const_iterator vote = m_votes.begin(); vote != m_votes.end(); ++vote)
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

void BudgetDraft::ResetSync()
{
    LOCK(m_cs);

    if (!fValid)
        return;

    for(std::map<uint256,BudgetDraftVote>::iterator vote = m_votes.begin(); vote != m_votes.end(); ++vote)
    {
        vote->second.fSynced = false;
    }
}

BudgetDraftBroadcast::BudgetDraftBroadcast()
{
}

BudgetDraftBroadcast::BudgetDraftBroadcast(int nBlockStart, const std::vector<CTxBudgetPayment>& vecBudgetPayments, uint256 nFeeTXHash)
    : m_blockStart(nBlockStart)
    , m_payments(vecBudgetPayments)
    , m_feeTransactionHash(nFeeTXHash)
{
    boost::sort(this->m_payments, BudgetDraft::ComparePayments);
}

BudgetDraftBroadcast::BudgetDraftBroadcast(int nBlockStart, const std::vector<CTxBudgetPayment>& vecBudgetPayments, const CTxIn& masternodeId, const CKey& keyMasternode)
    : m_blockStart(nBlockStart)
    , m_payments(vecBudgetPayments)
    , m_masternodeSubmittedId(masternodeId)
{
    boost::sort(this->m_payments, BudgetDraft::ComparePayments);

    if (!keyMasternode.SignCompact(GetHash(), m_signature))
        throw std::runtime_error("Cannot sign finalized budget: ");
}

BudgetDraft BudgetDraftBroadcast::Budget() const
{
    if (IsSubmittedManually())
        return BudgetDraft(m_blockStart, m_payments, m_feeTransactionHash);
    else
        return BudgetDraft(m_blockStart, m_payments, m_masternodeSubmittedId, m_signature);
}

uint256 BudgetDraftBroadcast::GetHash() const
{
    CHashWriter stream(SER_GETHASH, PROTOCOL_VERSION);
    stream << m_blockStart;
    stream << m_payments;

    return stream.GetHash();
}

bool BudgetDraftBroadcast::IsValid(std::string& strError, bool fCheckCollateral) const
{
    return Budget().IsValid(strError, fCheckCollateral);
}

bool BudgetDraftBroadcast::IsValid(bool fCheckCollateral) const
{
    return Budget().IsValid(fCheckCollateral);
}

bool BudgetDraftBroadcast::IsSubmittedManually() const
{
    return m_feeTransactionHash != uint256();
}

uint256 BudgetDraftBroadcast::GetFeeTxHash() const
{
    return m_feeTransactionHash;
}

int BudgetDraftBroadcast::GetBlockStart() const
{
    return m_blockStart;
}

const CTxIn& BudgetDraftBroadcast::MasternodeSubmittedId() const
{
    return m_masternodeSubmittedId;
}

const std::vector<CTxBudgetPayment>& BudgetDraftBroadcast::GetBudgetPayments() const
{
    return m_payments;
}

void BudgetDraftBroadcast::Relay()
{
    assert(boost::is_sorted(m_payments, BudgetDraft::ComparePayments));
    CInv inv(MSG_BUDGET_FINALIZED, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

BudgetDraftVote::BudgetDraftVote()
{
    vin = CTxIn();
    nBudgetHash = uint256();
    nTime = 0;
    vchSig.clear();
    fValid = true;
    fSynced = false;
}

BudgetDraftVote::BudgetDraftVote(CTxIn vinIn, uint256 nBudgetHashIn)
{
    vin = vinIn;
    nBudgetHash = nBudgetHashIn;
    nTime = GetAdjustedTime();
    vchSig.clear();
    fValid = true;
    fSynced = false;
}

void BudgetDraftVote::Relay()
{
    CInv inv(MSG_BUDGET_FINALIZED_VOTE, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

uint256 BudgetDraftVote::GetHash() const
{
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin;
    ss << nBudgetHash;
    ss << nTime;
    return ss.GetHash();
}


bool BudgetDraftVote::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    // Choose coins to use
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;

    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nBudgetHash.ToString() + boost::lexical_cast<std::string>(nTime);

    if(!legacySigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrintf("BudgetDraftVote::Sign - Error upon calling SignMessage");
        return false;
    }

    if(!legacySigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrintf("BudgetDraftVote::Sign - Error upon calling VerifyMessage");
        return false;
    }

    return true;
}

bool BudgetDraftVote::SignatureValid(bool fSignatureCheck)
{
    std::string errorMessage;

    std::string strMessage = vin.prevout.ToStringShort() + nBudgetHash.ToString() + boost::lexical_cast<std::string>(nTime);

    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn == NULL)
    {
        LogPrint("mnbudget", "BudgetDraftVote::SignatureValid() - Unknown Masternode\n");
        return false;
    }

    if(!fSignatureCheck) return true;

    if(!legacySigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("BudgetDraftVote::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

std::string CBudgetManager::ToString() const
{
    LOCK(cs);

    std::ostringstream info;

    info << "Proposals: " << (int)mapProposals.size() <<
            ", Budgets: " << (int)mapBudgetDrafts.size() <<
            ", Seen Budgets: " << (int)mapSeenMasternodeBudgetProposals.size() <<
            ", Seen Budget Votes: " << (int)mapSeenMasternodeBudgetVotes.size() <<
            ", Seen Final Budgets: " << (int)mapSeenBudgetDrafts.size() <<
            ", Seen Final Budget Votes: " << (int)mapSeenBudgetDraftVotes.size();

    return info.str();
}


