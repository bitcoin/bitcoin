// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core_io.h"
#include "main.h"
#include "init.h"

#include "governance.h"
#include "masternode.h"
#include "darksend.h"
#include "masternodeman.h"
#include "masternode-sync.h"
#include "util.h"
#include "addrman.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

CGovernanceManager govman;
CCriticalSection cs_budget;

std::map<uint256, int64_t> askedForSourceProposalOrBudget;
std::vector<CGovernanceNode> vecImmatureGovernanceNodes;

int nSubmittedFinalBudget;


bool IsBudgetCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf)
{
    CTransaction txCollateral;
    uint256 nBlockHash;
    if(!GetTransaction(nTxCollateralHash, txCollateral, Params().GetConsensus(), nBlockHash, true)){
        strError = strprintf("Can't find collateral tx %s", txCollateral.ToString());
        LogPrintf ("CGovernanceNodeBroadcast::IsBudgetCollateralValid - %s\n", strError);
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
            LogPrintf ("CGovernanceNodeBroadcast::IsBudgetCollateralValid - %s\n", strError);
            return false;
        }
        if(o.scriptPubKey == findScript && o.nValue >= BUDGET_FEE_TX) foundOpReturn = true;

    }
    if(!foundOpReturn){
        strError = strprintf("Couldn't find opReturn %s in %s", nExpectedHash.ToString(), txCollateral.ToString());
        LogPrintf ("CGovernanceNodeBroadcast::IsBudgetCollateralValid - %s\n", strError);
        return false;
    }

    LOCK(cs_main);
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
        LogPrintf ("CGovernanceNodeBroadcast::IsBudgetCollateralValid - %s - %d confirmations\n", strError, conf);
        return false;
    }
}

void CGovernanceManager::CheckOrphanVotes()
{
    LOCK(cs);


    std::string strError = "";
    std::map<uint256, CGovernanceVote>::iterator it1 = mapOrphanGovernanceVotes.begin();
    while(it1 != mapOrphanGovernanceVotes.end()){
        if(govman.UpdateGovernanceObjectVotes(((*it1).second), NULL, strError)){
            LogPrintf("CGovernanceManager::CheckOrphanVotes - Proposal/Budget is known, activating and removing orphan vote\n");
            mapOrphanGovernanceVotes.erase(it1++);
        } else {
            ++it1;
        }
    }
}

void CGovernanceManager::SubmitFinalBudget()
{

    

    // if(!pCurrentBlockIndex) return;

    // int nBlockStart = pCurrentBlockIndex->nHeight - pCurrentBlockIndex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks + Params().GetConsensus().nBudgetPaymentsCycleBlocks;
    // if(nSubmittedFinalBudget >= nBlockStart) return;
    // if(nBlockStart - pCurrentBlockIndex->nHeight > 576*2) return; //submit final budget 2 days before payment

    // std::vector<CGovernanceNode*> vBudgetProposals = govman.GetBudget();
    // std::string strBudgetName = "main";
    // std::vector<CTxBudgetPayment> vecTxBudgetPayments;

    // for(unsigned int i = 0; i < vBudgetProposals.size(); i++){
    //     CTxBudgetPayment txBudgetPayment;
    //     txBudgetPayment.nProposalHash = vBudgetProposals[i]->GetHash();
    //     txBudgetPayment.payee = vBudgetProposals[i]->GetPayee();
    //     txBudgetPayment.nAmount = vBudgetProposals[i]->GetAllotted();
    //     vecTxBudgetPayments.push_back(txBudgetPayment);
    // }

    // if(vecTxBudgetPayments.size() < 1) {
    //     LogPrintf("CGovernanceManager::SubmitFinalBudget - Found No Proposals For Period\n");
    //     return;
    // }

    // CFinalizedBudgetBroadcast tempBudget(strBudgetName, nBlockStart, vecTxBudgetPayments, uint256());
    // if(mapSeenFinalizedBudgets.count(tempBudget.GetHash())) {
    //     LogPrintf("CGovernanceManager::SubmitFinalBudget - Budget already exists - %s\n", tempBudget.GetHash().ToString());    
    //     nSubmittedFinalBudget = pCurrentBlockIndex->nHeight;
    //     return; //already exists
    // }

    // //create fee tx
    // CTransaction tx;
    // if(!mapCollateral.count(tempBudget.GetHash())){
    //     CWalletTx wtx;
    //     if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, tempBudget.GetHash(), false)){
    //         LogPrintf("CGovernanceManager::SubmitFinalBudget - Can't make collateral transaction\n");
    //         return;
    //     }
        
    //     // make our change address
    //     CReserveKey reservekey(pwalletMain);
    //     //send the tx to the network
    //     pwalletMain->CommitTransaction(wtx, reservekey, NetMsgType::IX);

    //     mapCollateral.insert(make_pair(tempBudget.GetHash(), (CTransaction)wtx));
    //     tx = (CTransaction)wtx;
    // } else {
    //     tx = mapCollateral[tempBudget.GetHash()];
    // }

    // CTxIn in(COutPoint(tx.GetHash(), 0));
    // int conf = GetInputAgeIX(tx.GetHash(), in);
    // /*
    //     Wait will we have 1 extra confirmation, otherwise some clients might reject this feeTX
    //     -- This function is tied to NewBlock, so we will propagate this budget while the block is also propagating
    // */
    // if(conf < BUDGET_FEE_CONFIRMATIONS+1){
    //     LogPrintf ("CGovernanceManager::SubmitFinalBudget - Collateral requires at least %d confirmations - %s - %d confirmations\n", BUDGET_FEE_CONFIRMATIONS, tx.GetHash().ToString(), conf);
    //     return;
    // }

    // nSubmittedFinalBudget = nBlockStart;

    // //create the proposal incase we're the first to make it
    // CFinalizedBudgetBroadcast finalizedBudgetBroadcast(strBudgetName, nBlockStart, vecTxBudgetPayments, tx.GetHash());

    // std::string strError = "";
    // if(!finalizedBudgetBroadcast.IsValid(pCurrentBlockIndex, strError)){
    //     LogPrintf("CGovernanceManager::SubmitFinalBudget - Invalid finalized budget - %s \n", strError);
    //     return;
    // }

    // LOCK(cs);
    // mapSeenFinalizedBudgets.insert(make_pair(finalizedBudgetBroadcast.GetHash(), finalizedBudgetBroadcast));
    // finalizedBudgetBroadcast.Relay();
    // govman.AddFinalizedBudget(finalizedBudgetBroadcast);
}

//
// CBudgetDB
//

CBudgetDB::CBudgetDB()
{
    pathDB = GetDataDir() / "budget.dat";
    strMagicMessage = "MasternodeBudget";
}

bool CBudgetDB::Write(const CGovernanceManager& objToSave)
{
    LOCK(objToSave.cs);

    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssObj(SER_DISK, CLIENT_VERSION);
    ssObj << strMagicMessage; // masternode cache file specific magic message
    ssObj << FLATDATA(Params().MessageStart()); // network specific magic number
    ssObj << objToSave;
    uint256 hash = Hash(ssObj.begin(), ssObj.end());
    ssObj << hash;

    // open output file, and associate with CAutoFile
    FILE *file = fopen(pathDB.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s : Failed to open file %s", __func__, pathDB.string());

    // Write and commit header, data
    try {
        fileout << ssObj;
    }
    catch (std::exception &e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
    fileout.fclose();

    LogPrintf("Written info to budget.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("Budget manager - %s\n", objToSave.ToString());

    return true;
}

CBudgetDB::ReadResult CBudgetDB::Read(CGovernanceManager& objToLoad, bool fDryRun)
{
    //LOCK(objToLoad.cs);

    int64_t nStart = GetTimeMillis();
    // open input file, and associate with CAutoFile
    FILE *file = fopen(pathDB.string().c_str(), "rb");
    CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
    if (filein.IsNull())
    {
        error("%s : Failed to open file %s", __func__, pathDB.string());
        return FileError;
    }

    // use file size to size memory buffer
    int fileSize = boost::filesystem::file_size(pathDB);
    int dataSize = fileSize - sizeof(uint256);
    // Don't try to resize to a negative number if file is small
    if (dataSize < 0)
        dataSize = 0;
    vector<unsigned char> vchData;
    vchData.resize(dataSize);
    uint256 hashIn;

    // read data and checksum from file
    try {
        filein.read((char *)&vchData[0], dataSize);
        filein >> hashIn;
    }
    catch (std::exception &e) {
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return HashReadError;
    }
    filein.fclose();

    CDataStream ssObj(vchData, SER_DISK, CLIENT_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssObj.begin(), ssObj.end());
    if (hashIn != hashTmp)
    {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }


    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header (masternode cache file specific magic message) and ..
        ssObj >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp)
        {
            error("%s : Invalid masternode cache magic message", __func__);
            return IncorrectMagicMessage;
        }


        // de-serialize file header (network specific magic number) and ..
        ssObj >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
        {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }

        // de-serialize data into CGovernanceManager object
        ssObj >> objToLoad;
    }
    catch (std::exception &e) {
        objToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    LogPrintf("Loaded info from budget.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", objToLoad.ToString());
    if(!fDryRun) {
        LogPrintf("Budget manager - cleaning....\n");
        objToLoad.CheckAndRemove();
        LogPrintf("Budget manager - %s\n", objToLoad.ToString());
    }

    return Ok;
}

// bool CGovernanceManager::AddFinalizedBudget(CFinalizedBudget& finalizedBudget)
// {
//     std::string strError = "";
//     if(!finalizedBudget.IsValid(pCurrentBlockIndex, strError)) return false;

//     if(mapFinalizedBudgets.count(finalizedBudget.GetHash())) {
//         return false;
//     }

//     mapFinalizedBudgets.insert(make_pair(finalizedBudget.GetHash(), finalizedBudget));
//     return true;
// }

bool CGovernanceManager::AddGovernanceObject(CGovernanceNode& budgetProposal)
{
    LOCK(cs);
    std::string strError = "";
    if(!budgetProposal.IsValid(pCurrentBlockIndex, strError)) {
        LogPrintf("CGovernanceManager::AddGovernanceObject - invalid budget proposal - %s\n", strError);
        return false;
    }

    if(mapGovernanceObjects.count(budgetProposal.GetHash())) {
        return false;
    }

    mapGovernanceObjects.insert(make_pair(budgetProposal.GetHash(), budgetProposal));
    return true;
}

void CGovernanceManager::CheckAndRemove()
{
    LogPrintf("CGovernanceManager::CheckAndRemove \n");

    if(!pCurrentBlockIndex) return;

    std::string strError = "";
    
    // TODO 12.1
    // std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    // while(it != mapFinalizedBudgets.end())
    // {
    //     CFinalizedBudget* pfinalizedBudget = &((*it).second);

    //     pfinalizedBudget->fValid = pfinalizedBudget->IsValid(pCurrentBlockIndex, strError);
    //     if(pfinalizedBudget->fValid) {
    //         pfinalizedBudget->AutoCheckSuperBlockVoting();
    //         ++it;
    //         continue;
    //     } else if(pfinalizedBudget->nBlockStart != 0 && pfinalizedBudget->nBlockStart < pCurrentBlockIndex->nHeight - Params().GetConsensus().nBudgetPaymentsCycleBlocks) {
    //         // it's too old, remove it
    //         mapFinalizedBudgets.erase(it++);
    //         LogPrintf("CGovernanceManager::CheckAndRemove - removing budget %s\n", pfinalizedBudget->GetHash().ToString());
    //         continue;
    //     }
    //     // it's not valid already but it's not too old yet, keep it and move to the next one
    //     ++it;
    // }

    std::map<uint256, CGovernanceNode>::iterator it2 = mapGovernanceObjects.begin();
    while(it2 != mapGovernanceObjects.end())
    {
        CGovernanceNode* node = &((*it2).second);
        node->fValid = node->IsValid(pCurrentBlockIndex, strError);
        ++it2;
    }
}

void CGovernanceManager::FillBlockPayee(CMutableTransaction& txNew, CAmount nFees)
{
    LOCK(cs);

    AssertLockHeld(cs_main);
    if(!chainActive.Tip()) return;

    int nHighestCount = 0;
    CScript payee;
    CAmount nAmount = 0;

    // ------- Grab The Highest Count

    // TODO 12.1
    // std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    // while(it != mapFinalizedBudgets.end())
    // {
    //     CFinalizedBudget* pfinalizedBudget = &((*it).second);
    //     if(pfinalizedBudget->GetVoteCount() > nHighestCount &&
    //             chainActive.Tip()->nHeight + 1 >= pfinalizedBudget->GetBlockStart() &&
    //             chainActive.Tip()->nHeight + 1 <= pfinalizedBudget->GetBlockEnd() &&
    //             pfinalizedBudget->GetPayeeAndAmount(chainActive.Tip()->nHeight + 1, payee, nAmount)){
    //                 nHighestCount = pfinalizedBudget->GetVoteCount();
    //     }

    //     ++it;
    // }

    //miners get the full amount on these blocks
    txNew.vout[0].nValue = nFees + GetBlockSubsidy(chainActive.Tip()->nBits, chainActive.Tip()->nHeight, Params().GetConsensus());

    if(nHighestCount > 0){
        txNew.vout.resize(2);

        //these are super blocks, so their value can be much larger than normal
        txNew.vout[1].scriptPubKey = payee;
        txNew.vout[1].nValue = nAmount;

        CTxDestination address1;
        ExtractDestination(payee, address1);
        CBitcoinAddress address2(address1);

        LogPrintf("CGovernanceManager::FillBlockPayee - Budget payment to %s for %lld\n", address2.ToString(), nAmount);
    }

}

// CFinalizedBudget *CGovernanceManager::FindFinalizedBudget(uint256 nHash)
// {
//     if(mapFinalizedBudgets.count(nHash))
//         return &mapFinalizedBudgets[nHash];

//     return NULL;
// }

CGovernanceNode *CGovernanceManager::FindGovernanceObject(const std::string &strName)
{
    //find the prop with the highest yes count

    int nYesCount = -99999;
    CGovernanceNode* pbudgetProposal = NULL;

    std::map<uint256, CGovernanceNode>::iterator it = mapGovernanceObjects.begin();
    while(it != mapGovernanceObjects.end()){
        if((*it).second.strName == strName && (*it).second.GetYesCount() > nYesCount){
            pbudgetProposal = &((*it).second);
            nYesCount = pbudgetProposal->GetYesCount();
        }
        ++it;
    }

    if(nYesCount == -99999) return NULL;

    return pbudgetProposal;
}

CGovernanceNode *CGovernanceManager::FindGovernanceObject(uint256 nHash)
{
    LOCK(cs);

    if(mapGovernanceObjects.count(nHash))
        return &mapGovernanceObjects[nHash];

    return NULL;
}

bool CGovernanceManager::IsBudgetPaymentBlock(int nBlockHeight)
{
    int nHighestCount = -1;

    // TODO 12.1
    // std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    // while(it != mapFinalizedBudgets.end())
    // {
    //     CFinalizedBudget* pfinalizedBudget = &((*it).second);
    //     if(pfinalizedBudget->GetVoteCount() > nHighestCount && 
    //         nBlockHeight >= pfinalizedBudget->GetBlockStart() && 
    //         nBlockHeight <= pfinalizedBudget->GetBlockEnd()){
    //         nHighestCount = pfinalizedBudget->GetVoteCount();
    //     }

    //     ++it;
    // }

    // /*
    //     If budget doesn't have 5% of the network votes, then we should pay a masternode instead
    // */
    // if(nHighestCount > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/20) return true;

    return false;
}

bool CGovernanceManager::HasNextFinalizedBudget()
{
    if(!pCurrentBlockIndex) return false;

    if(masternodeSync.IsBudgetFinEmpty()) return true;

    int nBlockStart = pCurrentBlockIndex->nHeight - pCurrentBlockIndex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks + Params().GetConsensus().nBudgetPaymentsCycleBlocks;
    if(nBlockStart - pCurrentBlockIndex->nHeight > 576*2) return true; //we wouldn't have the budget yet

    if(govman.IsBudgetPaymentBlock(nBlockStart)) return true;

    LogPrintf("CGovernanceManager::HasNextFinalizedBudget() - Client is missing budget - %lli\n", nBlockStart);

    return false;
}

bool CGovernanceManager::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    LOCK(cs);

    // TODO 12.1

    // int nHighestCount = 0;
    // std::vector<CFinalizedBudget*> ret;

    // // ------- Grab The Highest Count

    // std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    // while(it != mapFinalizedBudgets.end())
    // {
    //     CFinalizedBudget* pfinalizedBudget = &((*it).second);
    //     if(pfinalizedBudget->GetVoteCount() > nHighestCount &&
    //             nBlockHeight >= pfinalizedBudget->GetBlockStart() &&
    //             nBlockHeight <= pfinalizedBudget->GetBlockEnd()){
    //                 nHighestCount = pfinalizedBudget->GetVoteCount();
    //     }

    //     ++it;
    // }

    // /*
    //     If budget doesn't have 5% of the network votes, then we should pay a masternode instead
    // */
    // if(nHighestCount < mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/20) return false;

    // // check the highest finalized budgets (+/- 10% to assist in consensus)

    // it = mapFinalizedBudgets.begin();
    // while(it != mapFinalizedBudgets.end())
    // {
    //     CFinalizedBudget* pfinalizedBudget = &((*it).second);

    //     if(pfinalizedBudget->GetVoteCount() > nHighestCount - mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10){
    //         if(nBlockHeight >= pfinalizedBudget->GetBlockStart() && nBlockHeight <= pfinalizedBudget->GetBlockEnd()){
    //             if(pfinalizedBudget->IsTransactionValid(txNew, nBlockHeight)){
    //                 return true;
    //             }
    //         }
    //     }

    //     ++it;
    // }

    //we looked through all of the known budgets
    return false;
}

std::vector<CGovernanceNode*> CGovernanceManager::FindMatchingGovernanceObjects(GovernanceObjectType type)
{
    LOCK(cs);


    std::vector<CGovernanceNode*> vBudgetProposalRet;

    // TODO 12.1

    // if(type == Proposal || type == Contract || type == Switch || type == Setting)
    // {
    //     std::map<uint256, CGovernanceNode>::iterator it = mapGovernanceObjects.begin();
    //     while(it != mapGovernanceObjects.end())
    //     {
    //         if((*it).second.GetGovernanceType() != type) {++it; continue;}

    //         (*it).second.CleanAndRemove(false);

    //         CGovernanceNode* pbudgetProposal = &((*it).second);
    //         vBudgetProposalRet.push_back(pbudgetProposal);

    //         ++it;
    //     }    
    // }    

    // Finalized Budgets should use

    return vBudgetProposalRet;
}

//
// Sort by votes, if there's a tie sort by their feeHash TX
//
struct sortProposalsByVotes {
    bool operator()(const std::pair<CGovernanceNode*, int> &left, const std::pair<CGovernanceNode*, int> &right) {
      if( left.second != right.second)
        return (left.second > right.second);
      return (UintToArith256(left.first->nFeeTXHash) > UintToArith256(right.first->nFeeTXHash));
    }
};

//Need to review this function
std::vector<CGovernanceNode*> CGovernanceManager::GetBudget()
{
    LOCK(cs);
    std::vector<CGovernanceNode*> vBudgetProposalsRet;


    // TODO 12.1

    // // ------- Sort budgets by Yes Count

    // std::vector<std::pair<CGovernanceNode*, int> > vBudgetPorposalsSort;

    // std::map<uint256, CGovernanceNode>::iterator it = mapGovernanceObjects.begin();
    // while(it != mapGovernanceObjects.end()){
    //     (*it).second.CleanAndRemove(false);

    //     /*
    //         list offset

    //         count * 0.00 : Proposal (marked charity)  :   0% to 05%
    //         count * 0.05 : Proposals                  :   5% to 100%
    //         count * 1.00 : Contracts                  :   100% to 200%
    //         count * 2.00 : High Priority Contracts    :   200%+
    //     */
    //     int nOffset = 0;

    //     if((*it).second.GetGovernanceType() == Setting) {it++; continue;}
    //     if((*it).second.GetGovernanceType() == Proposal) nOffset += 0;
    //     if((*it).second.GetGovernanceType() == Contract) nOffset += mnodeman.CountEnabled();
        
    //     vBudgetPorposalsSort.push_back(make_pair(&((*it).second), nOffset+((*it).second.GetYesCount()-(*it).second.GetNoCount())));

    //     ++it;
    // }

    // std::sort(vBudgetPorposalsSort.begin(), vBudgetPorposalsSort.end(), sortProposalsByVotes());

    // // ------- Grab The Budgets In Order

    // CAmount nBudgetAllocated = 0;
    // if(!pCurrentBlockIndex) return vBudgetProposalsRet;

    // int nBlockStart = pCurrentBlockIndex->nHeight - pCurrentBlockIndex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks + Params().GetConsensus().nBudgetPaymentsCycleBlocks;
    // int nBlockEnd  =  nBlockStart + Params().GetConsensus().nBudgetPaymentsWindowBlocks;
    // CAmount nTotalBudget = GetTotalBudget(nBlockStart);


    // std::vector<std::pair<CGovernanceNode*, int> >::iterator it2 = vBudgetPorposalsSort.begin();
    // while(it2 != vBudgetPorposalsSort.end())
    // {
    //     CGovernanceNode* pbudgetProposal = (*it2).first;


    //     printf("-> Budget Name : %s\n", pbudgetProposal->strName.c_str());
    //     printf("------- nBlockStart : %d\n", pbudgetProposal->nBlockStart);
    //     printf("------- nBlockEnd : %d\n", pbudgetProposal->nBlockEnd);
    //     printf("------- nBlockStart2 : %d\n", nBlockStart);
    //     printf("------- nBlockEnd2 : %d\n", nBlockEnd);

    //     printf("------- 1 : %d\n", pbudgetProposal->fValid && pbudgetProposal->nBlockStart <= nBlockStart);
    //     printf("------- 2 : %d\n", pbudgetProposal->nBlockEnd >= nBlockEnd);
    //     printf("------- 3 : %d\n", pbudgetProposal->GetYesCount() - pbudgetProposal->GetNoCount() > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10);
    //     printf("------- 4 : %d\n", pbudgetProposal->IsEstablished());

    //     //prop start/end should be inside this period
    //     if(pbudgetProposal->fValid && pbudgetProposal->nBlockStart <= nBlockStart &&
    //             pbudgetProposal->nBlockEnd >= nBlockEnd &&
    //             pbudgetProposal->GetYesCount() - pbudgetProposal->GetNoCount() > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10 && 
    //             pbudgetProposal->IsEstablished())
    //     {
    //         printf("------- In range \n");

    //         if(pbudgetProposal->GetAmount() + nBudgetAllocated <= nTotalBudget) {
    //             pbudgetProposal->SetAllotted(pbudgetProposal->GetAmount());
    //             nBudgetAllocated += pbudgetProposal->GetAmount();
    //             vBudgetProposalsRet.push_back(pbudgetProposal);
    //             printf("------- YES \n");
    //         } else {
    //             pbudgetProposal->SetAllotted(0);
    //         }
    //     }

    //     ++it2;
    // }

    return vBudgetProposalsRet;
}

// struct sortFinalizedBudgetsByVotes {
//     bool operator()(const std::pair<CFinalizedBudget*, int> &left, const std::pair<CFinalizedBudget*, int> &right) {
//         return left.second > right.second;
//     }
// };

std::vector<CFinalizedBudget*> CGovernanceManager::GetFinalizedBudgets()
{
    LOCK(cs);

    // TODO 12.1

    std::vector<CFinalizedBudget*> vFinalizedBudgetsRet;
    // std::vector<std::pair<CFinalizedBudget*, int> > vFinalizedBudgetsSort;

    // // ------- Grab The Budgets In Order

    // std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    // while(it != mapFinalizedBudgets.end())
    // {
    //     CFinalizedBudget* pfinalizedBudget = &((*it).second);

    //     vFinalizedBudgetsSort.push_back(make_pair(pfinalizedBudget, pfinalizedBudget->GetVoteCount()));
    //     ++it;
    // }
    // std::sort(vFinalizedBudgetsSort.begin(), vFinalizedBudgetsSort.end(), sortFinalizedBudgetsByVotes());

    // std::vector<std::pair<CFinalizedBudget*, int> >::iterator it2 = vFinalizedBudgetsSort.begin();
    // while(it2 != vFinalizedBudgetsSort.end())
    // {
    //     vFinalizedBudgetsRet.push_back((*it2).first);
    //     ++it2;
    // }

    return vFinalizedBudgetsRet;
}

std::string CGovernanceManager::GetRequiredPaymentsString(int nBlockHeight)
{
    LOCK(cs);

    std::string ret = "unknown-budget";

    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);
        if(nBlockHeight >= pfinalizedBudget->GetBlockStart() && nBlockHeight <= pfinalizedBudget->GetBlockEnd()){
            CTxBudgetPayment payment;
            if(pfinalizedBudget->GetBudgetPaymentByBlock(nBlockHeight, payment)){
                if(ret == "unknown-budget"){
                    ret = payment.nProposalHash.ToString();
                } else {
                    ret += ",";
                    ret += payment.nProposalHash.ToString();
                }
            } else {
                LogPrintf("CGovernanceManager::GetRequiredPaymentsString - Couldn't find budget payment for block %d\n", nBlockHeight);
            }
        }

        ++it;
    }

    return ret;
}

CAmount CGovernanceManager::GetTotalBudget(int nHeight)
{
    if(!pCurrentBlockIndex) return 0;

    //get min block value and calculate from that
    CAmount nSubsidy = 5 * COIN;

    const Consensus::Params consensusParams = Params().GetConsensus();

    // TODO: Remove this to further unify logic among mainnet/testnet/whatevernet,
    //       use single formula instead (the one that is for current mainnet).
    //       Probably a good idea to use a significally lower consensusParams.nSubsidyHalvingInterval
    //       for testnet (like 10 times for example) to see the effect of halving there faster.
    //       Will require testnet restart.
    if(Params().NetworkIDString() == CBaseChainParams::TESTNET){
        for(int i = 46200; i <= nHeight; i += consensusParams.nSubsidyHalvingInterval) nSubsidy -= nSubsidy/14;
    } else {
        // yearly decline of production by 7.1% per year, projected 21.3M coins max by year 2050.
        for(int i = consensusParams.nSubsidyHalvingInterval; i <= nHeight; i += consensusParams.nSubsidyHalvingInterval) nSubsidy -= nSubsidy/14;
    }

    // 10%
    return ((nSubsidy/100)*10)*consensusParams.nBudgetPaymentsCycleBlocks;
}

void CGovernanceManager::NewBlock()
{
    TRY_LOCK(cs, fBudgetNewBlock);
    if(!fBudgetNewBlock) return;

    if(!pCurrentBlockIndex) return;

    if (masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_BUDGET) return;

    if (strBudgetMode == "suggest") { //suggest the budget we see
        SubmitFinalBudget();
    }

    //this function should be called 1/6 blocks, allowing up to 100 votes per day on all proposals
    if(pCurrentBlockIndex->nHeight % 6 != 0) return;

    // incremental sync with our peers
    // if(masternodeSync.IsSynced()){
    //     LogPrintf("CGovernanceManager::NewBlock - incremental sync started\n");
    //     if(pCurrentBlockIndex->nHeight % 600 == rand() % 600) {
    //         ClearSeen();
    //         ResetSync();
    //     }

    //     LOCK(cs_vNodes);
    //     BOOST_FOREACH(CNode* pnode, vNodes)
    //         if(pnode->nVersion >= MIN_BUDGET_PEER_PROTO_VERSION) 
    //             Sync(pnode, uint256());

    //     MarkSynced();
    // }

    CheckAndRemove();

    //remove invalid votes once in a while (we have to check the signatures and validity of every vote, somewhat CPU intensive)

    std::map<uint256, int64_t>::iterator it = askedForSourceProposalOrBudget.begin();
    while(it != askedForSourceProposalOrBudget.end()){
        if((*it).second > GetTime() - (60*60*24)){
            ++it;
        } else {
            askedForSourceProposalOrBudget.erase(it++);
        }
    }

    std::map<uint256, CGovernanceNode>::iterator it2 = mapGovernanceObjects.begin();
    while(it2 != mapGovernanceObjects.end()){
        (*it2).second.CleanAndRemove(false);
        ++it2;
    }

    std::map<uint256, CFinalizedBudget>::iterator it3 = mapFinalizedBudgets.begin();
    while(it3 != mapFinalizedBudgets.end()){
        (*it3).second.CleanAndRemove(false);
        ++it3;
    }

    std::vector<CGovernanceNodeBroadcast>::iterator it4 = vecImmatureGovernanceNodes.begin();
    while(it4 != vecImmatureGovernanceNodes.end())
    {
        std::string strError = "";
        int nConf = 0;
        if(!IsBudgetCollateralValid((*it4).nFeeTXHash, (*it4).GetHash(), strError, (*it4).nTime, nConf)){
            ++it4;
            continue;
        }

        if(!(*it4).IsValid(pCurrentBlockIndex, strError)) {
            LogPrintf("mprop (immature) - invalid budget proposal - %s\n", strError);
            it4 = vecImmatureGovernanceNodes.erase(it4); 
            continue;
        }

        CGovernanceNode budgetProposal((*it4));
        if(AddGovernanceObject(budgetProposal)) {(*it4).Relay();}

        LogPrintf("mprop (immature) - new budget - %s\n", (*it4).GetHash().ToString());
        it4 = vecImmatureGovernanceNodes.erase(it4); 
    }

    // std::vector<CFinalizedBudgetBroadcast>::iterator it5 = vecImmatureFinalizedBudgets.begin();
    // while(it5 != vecImmatureFinalizedBudgets.end())
    // {
    //     std::string strError = "";
    //     int nConf = 0;
    //     if(!IsBudgetCollateralValid((*it5).nFeeTXHash, (*it5).GetHash(), strError, (*it5).nTime, nConf)){
    //         ++it5;
    //         continue;
    //     }

    //     if(!(*it5).IsValid(pCurrentBlockIndex, strError)) {
    //         LogPrintf("fbs (immature) - invalid finalized budget - %s\n", strError);
    //         it5 = vecImmatureFinalizedBudgets.erase(it5); 
    //         continue;
    //     }

    //     LogPrintf("fbs (immature) - new finalized budget - %s\n", (*it5).GetHash().ToString());

    //     CFinalizedBudget finalizedBudget((*it5));
    //     if(AddFinalizedBudget(finalizedBudget)) {(*it5).Relay();}

    //     it5 = vecImmatureFinalizedBudgets.erase(it5); 
    // }

}

void CGovernanceManager::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    // lite mode is not supported
    if(fLiteMode) return;
    if(!masternodeSync.IsBlockchainSynced()) return;

    LOCK(cs_budget);

    if (strCommand == NetMsgType::GOVERNANCE_VOTESYNC) { //Masternode vote sync
        uint256 nProp;
        vRecv >> nProp;

        if(Params().NetworkIDString() == CBaseChainParams::MAIN){
            if(nProp == uint256()) {
                if(pfrom->HasFulfilledRequest(NetMsgType::GOVERNANCE_VOTESYNC)) {
                    LogPrintf("mnvs - peer already asked me for the list\n");
                    Misbehaving(pfrom->GetId(), 20);
                    return;
                }
                pfrom->FulfilledRequest(NetMsgType::GOVERNANCE_VOTESYNC);
            }
        }

        Sync(pfrom, nProp);
        LogPrintf("mnvs - Sent Masternode votes to %s\n", pfrom->addr.ToString());
    }

    if (strCommand == NetMsgType::GOVERNANCE_OBJECT) { //Governance Object : Proposal, Contract, Switch, Setting (Finalized Budgets are elsewhere)

        CGovernanceNodeBroadcast budgetProposalBroadcast;
        vRecv >> budgetProposalBroadcast;

        if(mapSeenGovernanceObjects.count(budgetProposalBroadcast.GetHash())){
            masternodeSync.AddedBudgetItem(budgetProposalBroadcast.GetHash());
            return;
        }

        std::string strError = "";
        int nConf = 0;
        if(!IsBudgetCollateralValid(budgetProposalBroadcast.nFeeTXHash, budgetProposalBroadcast.GetHash(), strError, budgetProposalBroadcast.nTime, nConf)){
            LogPrintf("Proposal FeeTX is not valid - %s - %s\n", budgetProposalBroadcast.nFeeTXHash.ToString(), strError);
            if(nConf >= 1) vecImmatureGovernanceNodes.push_back(budgetProposalBroadcast);
            return;
        }

        mapSeenGovernanceObjects.insert(make_pair(budgetProposalBroadcast.GetHash(), budgetProposalBroadcast));

        if(!budgetProposalBroadcast.IsValid(pCurrentBlockIndex, strError)) {
            LogPrintf("mprop - invalid budget proposal - %s\n", strError);
            return;
        }

        CGovernanceNode budgetProposal(budgetProposalBroadcast);
        if(AddGovernanceObject(budgetProposal)) {budgetProposalBroadcast.Relay();}
        masternodeSync.AddedBudgetItem(budgetProposalBroadcast.GetHash());

        LogPrintf("mprop - new budget - %s\n", budgetProposalBroadcast.GetHash().ToString());

        //We might have active votes for this proposal that are valid now
        CheckOrphanVotes();
    }

    if (strCommand == NetMsgType::GOVERNANCE_VOTE) { //Masternode Vote
        CGovernanceVote vote;
        vRecv >> vote;
        vote.fValid = true;

        if(mapSeenGovernanceVotes.count(vote.GetHash())){
            masternodeSync.AddedBudgetItem(vote.GetHash());
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vin);
        if(pmn == NULL) {
            LogPrint("mnbudget", "mvote - unknown masternode - vin: %s\n", vote.vin.ToString());
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        // setup the parent object for the vote
        if(vote.GetGovernanceType() == FinalizedBudget)
        {
            CFinalizedBudget* obj = govman.FindFinalizedBudget(vote.nParentHash);
            if(!obj)
            {
                LogPrint("mnbudget", "mvote - unknown finalized budget - hash: %s\n", vote.nParentHash.ToString());
                govman.AddOrphanGovernanceVote(vote, pfrom);
                return;
            }
            vote.SetParent(obj);
        } else { // Proposal, Contract, Setting or Switch
            CGovernanceNode* obj = govman.FindGovernanceObject(vote.nParentHash);
            if(!obj)
            {
                LogPrint("mnbudget", "mvote - unknown finalized budget - hash: %s\n", vote.nParentHash.ToString());
                govman.AddOrphanGovernanceVote(vote, pfrom);
                return;
            }
            vote.SetParent(obj);
        }

        mapSeenGovernanceVotes.insert(make_pair(vote.GetHash(), vote));

        std::string strReason = "";
        if(!vote.IsValid(true, strReason)){
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            LogPrint("mnbudget", "mvote - signature invalid - %s\n", strReason);
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        std::string strError = "";
        if(UpdateGovernanceObjectVotes(vote, pfrom, strError)) {
            vote.Relay();
            masternodeSync.AddedBudgetItem(vote.GetHash());
            LogPrint("mnbudget", "mvote - new budget vote - %s\n", vote.GetHash().ToString());
        }

    }

    // TODO 12.1
    // if (strCommand == NetMsgType::MNBUDGETFINAL) { //Finalized Budget Suggestion
    //     CFinalizedBudgetBroadcast finalizedBudgetBroadcast;
    //     vRecv >> finalizedBudgetBroadcast;

    //     if(mapSeenFinalizedBudgets.count(finalizedBudgetBroadcast.GetHash())){
    //         masternodeSync.AddedBudgetItem(finalizedBudgetBroadcast.GetHash());
    //         return;
    //     }

    //     std::string strError = "";
    //     int nConf = 0;
    //     if(!IsBudgetCollateralValid(finalizedBudgetBroadcast.nFeeTXHash, finalizedBudgetBroadcast.GetHash(), strError, finalizedBudgetBroadcast.nTime, nConf)){
    //         LogPrintf("Finalized Budget FeeTX is not valid - %s - %s\n", finalizedBudgetBroadcast.nFeeTXHash.ToString(), strError);

    //         if(nConf >= 1) vecImmatureFinalizedBudgets.push_back(finalizedBudgetBroadcast);
    //         return;
    //     }

    //     mapSeenFinalizedBudgets.insert(make_pair(finalizedBudgetBroadcast.GetHash(), finalizedBudgetBroadcast));

    //     if(!finalizedBudgetBroadcast.IsValid(pCurrentBlockIndex, strError)) {
    //         // This shouldn't be a debug message, it's important
    //         LogPrintf("fbs - invalid finalized budget - %s\n", strError);
    //         return;
    //     }

    //     LogPrintf("fbs - new finalized budget - %s\n", finalizedBudgetBroadcast.GetHash().ToString());

    //     CFinalizedBudget finalizedBudget(finalizedBudgetBroadcast);
    //     if(AddFinalizedBudget(finalizedBudget)) {finalizedBudgetBroadcast.Relay();}
    //     masternodeSync.AddedBudgetItem(finalizedBudgetBroadcast.GetHash());

    //     //we might have active votes for this budget that are now valid
    //     CheckOrphanVotes();
    // }

    if (strCommand == NetMsgType::MNBUDGETFINALVOTE) { //Finalized Budget Vote
        CGovernanceVote vote;
        vRecv >> vote;
        vote.fValid = true;

        if(mapSeenGovernanceVotes.count(vote.GetHash())){
            masternodeSync.AddedBudgetItem(vote.GetHash());
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vin);
        if(pmn == NULL) {
            LogPrint("mnbudget", "fbvote - unknown masternode - vin: %s\n", vote.vin.ToString());
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        mapSeenGovernanceVotes.insert(make_pair(vote.GetHash(), vote));

        std::string strReason = "";
        if(!vote.IsValid(true, strReason)){
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);

            LogPrint("mnbudget", "fbvote - error: %s\n", strReason);

            //TODO: This needs to somehow only ask if the error was related to a missing masternode
            //      I would check for "unknown masternode" but the string includes the vin

            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        std::string strError = "";
        if(UpdateGovernanceObjectVotes(vote, pfrom, strError)) {
            vote.Relay();
            masternodeSync.AddedBudgetItem(vote.GetHash());

            LogPrintf("fbvote - new finalized budget vote - %s\n", vote.GetHash().ToString());
        } else {
            LogPrintf("fbvote - rejected finalized budget vote - %s - %s\n", vote.GetHash().ToString(), strError);
        }
    }
}

//mark that a full sync is needed
void CGovernanceManager::ResetSync()
{
    LOCK(cs);


    std::map<uint256, CGovernanceNodeBroadcast>::iterator it1 = mapSeenGovernanceObjects.begin();
    while(it1 != mapSeenGovernanceObjects.end()){
        CGovernanceNode* pbudgetProposal = FindGovernanceObject((*it1).first);
        if(pbudgetProposal && pbudgetProposal->fValid){
        
            //mark votes
            std::map<uint256, CGovernanceVote>::iterator it2 = pbudgetProposal->mapVotes.begin();
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
        if(pfinalizedBudget && pfinalizedBudget->fValid){

            //send votes
            std::map<uint256, CGovernanceVote>::iterator it4 = pfinalizedBudget->mapVotes.begin();
            while(it4 != pfinalizedBudget->mapVotes.end()){
                (*it4).second.fSynced = false;
                ++it4;
            }
        }
        ++it3;
    }
}

void CGovernanceManager::MarkSynced()
{
    LOCK(cs);

    /*
        Mark that we've sent all valid items
    */

    std::map<uint256, CGovernanceNodeBroadcast>::iterator it1 = mapSeenGovernanceObjects.begin();
    while(it1 != mapSeenGovernanceObjects.end()){
        CGovernanceNode* pbudgetProposal = FindGovernanceObject((*it1).first);
        if(pbudgetProposal && pbudgetProposal->fValid){
        
            //mark votes
            std::map<uint256, CGovernanceVote>::iterator it2 = pbudgetProposal->mapVotes.begin();
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
        if(pfinalizedBudget && pfinalizedBudget->fValid){

            //mark votes
            std::map<uint256, CGovernanceVote>::iterator it4 = pfinalizedBudget->mapVotes.begin();
            while(it4 != pfinalizedBudget->mapVotes.end()){
                if((*it4).second.fValid)
                    (*it4).second.fSynced = true;
                ++it4;
            }
        }
        ++it3;
    }

}


void CGovernanceManager::Sync(CNode* pfrom, uint256 nProp, bool fPartial)
{
    LOCK(cs);

    /*
        Sync with a client on the network

        --

        This code checks each of the hash maps for all known budget proposals and finalized budget proposals, then checks them against the
        budget object to see if they're OK. If all checks pass, we'll send it to the peer.

    */

    int nInvCount = 0;


    // todo - why does this code not always sync properly?
    // the next place this data arrives at is main.cpp:4024 and main.cpp:4030
    std::map<uint256, CGovernanceNodeBroadcast>::iterator it1 = mapSeenGovernanceObjects.begin();
    while(it1 != mapSeenGovernanceObjects.end()){
        CGovernanceNode* pbudgetProposal = FindGovernanceObject((*it1).first);
        if(pbudgetProposal && pbudgetProposal->fValid && (nProp == uint256() || (*it1).first == nProp)){
            // Push the inventory budget proposal message over to the other client
            pfrom->PushInventory(CInv(MSG_GOVERNANCE_OBJECT, (*it1).second.GetHash()));
            nInvCount++;
        
            //send votes, at the same time. We should collect votes and store them if we don't have the proposal yet on the other side
            std::map<uint256, CGovernanceVote>::iterator it2 = pbudgetProposal->mapVotes.begin();
            while(it2 != pbudgetProposal->mapVotes.end()){
                if((*it2).second.fValid){
                    if((fPartial && !(*it2).second.fSynced) || !fPartial) {
                        pfrom->PushInventory(CInv(MSG_GOVERNANCE_VOTE, (*it2).second.GetHash()));
                        nInvCount++;
                    }
                }
                ++it2;
            }
        }
        ++it1;
    }

    pfrom->PushMessage(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_BUDGET_PROP, nInvCount);

    LogPrintf("CGovernanceManager::Sync - sent %d items\n", nInvCount);

    nInvCount = 0;

    // finalized budget -- this code has no issues as far as we can tell
    std::map<uint256, CFinalizedBudgetBroadcast>::iterator it3 = mapSeenFinalizedBudgets.begin();
    while(it3 != mapSeenFinalizedBudgets.end()){
        CFinalizedBudget* pfinalizedBudget = FindFinalizedBudget((*it3).first);
        if(pfinalizedBudget && pfinalizedBudget->fValid && (nProp == uint256() || (*it3).first == nProp)){
            pfrom->PushInventory(CInv(MSG_GOVERNANCE_FINALIZED_BUDGET, (*it3).second.GetHash()));
            nInvCount++;

            //send votes
            std::map<uint256, CGovernanceVote>::iterator it4 = pfinalizedBudget->mapVotes.begin();
            while(it4 != pfinalizedBudget->mapVotes.end()){
                if((*it4).second.fValid) {
                    if((fPartial && !(*it4).second.fSynced) || !fPartial) {
                        pfrom->PushInventory(CInv(MSG_GOVERNANCE_VOTE, (*it4).second.GetHash()));
                        nInvCount++;
                    }
                }
                ++it4;
            }
        }
        ++it3;
    }

    pfrom->PushMessage(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_BUDGET_FIN, nInvCount);
    LogPrintf("CGovernanceManager::Sync - sent %d items\n", nInvCount);

}

// we will ask for this governance object and hold the vote
bool CGovernanceManager::AddOrphanGovernanceVote(CGovernanceVote& vote, CNode* pfrom)
{
    LogPrintf("CGovernanceManager::UpdateFinalizedBudget - Unknown Finalized Proposal %s, asking for source budget\n", vote.nParentHash.ToString());
    mapOrphanGovernanceVotes[vote.nParentHash] = vote;

    if(!askedForSourceProposalOrBudget.count(vote.nParentHash)){
        pfrom->PushMessage(NetMsgType::GOVERNANCE_VOTESYNC, vote.nParentHash);
        askedForSourceProposalOrBudget[vote.nParentHash] = GetTime();
        return true;
    }

    return false;   
}

bool CGovernanceManager::UpdateGovernanceObjectVotes(CGovernanceVote& vote, CNode* pfrom, std::string& strError)
{
    LOCK(cs);

    // is this a proposal? 
    if(vote.nGovernanceType == FinalizedBudget)
    {
        if(!mapFinalizedBudgets.count(vote.nParentHash)){
            if(pfrom){
                // only ask for missing items after our syncing process is complete -- 
                //   otherwise we'll think a full sync succeeded when they return a result
                if(!masternodeSync.IsSynced()) return false;

                AddOrphanGovernanceVote(vote, pfrom);
            }

            strError = "Finalized Budget not found!";
            return false;
        } else {
            CFinalizedBudget* obj = &mapFinalizedBudgets[vote.nParentHash];
            vote.SetParent(obj);
        }

        return mapFinalizedBudgets[vote.nParentHash].AddOrUpdateVote(vote, strError);
    }

    // is this a proposal? 
    if(vote.nGovernanceType == Proposal)
    {
        if(!mapGovernanceObjects.count(vote.nParentHash)){
            if(pfrom){
                // only ask for missing items after our syncing process is complete -- 
                //   otherwise we'll think a full sync succeeded when they return a result
                if(!masternodeSync.IsSynced()) return false;

                LogPrintf("CGovernanceManager::UpdateProposal - Unknown proposal %d, asking for source proposal\n", vote.nParentHash.ToString());
                mapOrphanGovernanceVotes[vote.nParentHash] = vote;

                if(!askedForSourceProposalOrBudget.count(vote.nParentHash)){
                    pfrom->PushMessage(NetMsgType::GOVERNANCE_VOTESYNC, vote.nParentHash);
                    askedForSourceProposalOrBudget[vote.nParentHash] = GetTime();
                }
            }

            strError = "Proposal not found!";
            return false;
        } else {
            CGovernanceNode* obj = &mapGovernanceObjects[vote.nParentHash];
            vote.SetParent(obj);
        }

        return mapGovernanceObjects[vote.nParentHash].AddOrUpdateVote(vote, strError);
    }

    return false;
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
}

CFinalizedBudget::CFinalizedBudget(const CFinalizedBudget& other)
{
    strBudgetName = other.strBudgetName;
    nBlockStart = other.nBlockStart;
    vecBudgetPayments = other.vecBudgetPayments;
    mapVotes = other.mapVotes;
    nFeeTXHash = other.nFeeTXHash;
    nTime = other.nTime;
    fValid = true;
    fAutoChecked = false;
}

// bool CFinalizedBudget::AddOrUpdateVote(CGovernanceVote& vote, std::string& strError)
// {
//     LOCK(cs);

//     uint256 hash = vote.vin.prevout.GetHash();
//     if(mapVotes.count(hash)){
//         if(mapVotes[hash].nTime > vote.nTime){
//             strError = strprintf("new vote older than existing vote - %s", vote.GetHash().ToString());
//             LogPrint("mnbudget", "CFinalizedBudget::AddOrUpdateVote - %s\n", strError);
//             return false;
//         }
//         if(vote.nTime - mapVotes[hash].nTime < BUDGET_VOTE_UPDATE_MIN){
//             strError = strprintf("time between votes is too soon - %s - %lli", vote.GetHash().ToString(), vote.nTime - mapVotes[hash].nTime);
//             LogPrint("mnbudget", "CFinalizedBudget::AddOrUpdateVote - %s\n", strError);
//             return false;
//         }
//     }

//     mapVotes[hash] = vote;
//     return true;
// }

//evaluate if we should vote for this. Masternode only
void CFinalizedBudget::AutoCheckSuperBlockVoting()
{
    LOCK(cs);

    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;

    LogPrintf("CFinalizedBudget::AutoCheck - %lli - %d\n", pindexPrev->nHeight, fAutoChecked);

    if(!fMasterNode || fAutoChecked) return;

    //do this 1 in 4 blocks -- spread out the voting activity on mainnet
    // -- this function is only called every sixth block, so this is really 1 in 24 blocks
    if(Params().NetworkIDString() == CBaseChainParams::MAIN && rand() % 4 != 0) {
        LogPrintf("CFinalizedBudget::AutoCheck - waiting\n");
        return;
    }

    fAutoChecked = true; //we only need to check this once


    if(strBudgetMode == "auto") //only vote for exact matches
    {
        std::vector<CGovernanceNode*> vBudgetProposals = govman.GetBudget();


        for(unsigned int i = 0; i < vecBudgetPayments.size(); i++){
            LogPrintf("CFinalizedBudget::AutoCheck - nProp %d %s\n", i, vecBudgetPayments[i].nProposalHash.ToString());
            LogPrintf("CFinalizedBudget::AutoCheck - Payee %d %s\n", i, ScriptToAsmStr(vecBudgetPayments[i].payee));
            LogPrintf("CFinalizedBudget::AutoCheck - nAmount %d %lli\n", i, vecBudgetPayments[i].nAmount);
        }

        for(unsigned int i = 0; i < vBudgetProposals.size(); i++){
            LogPrintf("CFinalizedBudget::AutoCheck - nProp %d %s\n", i, vBudgetProposals[i]->GetHash().ToString());
            LogPrintf("CFinalizedBudget::AutoCheck - Payee %d %s\n", i, ScriptToAsmStr(vBudgetProposals[i]->GetPayee()));
            LogPrintf("CFinalizedBudget::AutoCheck - nAmount %d %lli\n", i, vBudgetProposals[i]->GetAmount());
        }

        if(vBudgetProposals.size() == 0) {
            LogPrintf("CFinalizedBudget::AutoCheck - Can't get Budget, aborting\n");
            return;
        }

        if(vBudgetProposals.size() != vecBudgetPayments.size()) {
            LogPrintf("CFinalizedBudget::AutoCheck - Budget length doesn't match\n");
            return;
        }


        for(unsigned int i = 0; i < vecBudgetPayments.size(); i++){
            if(i > vBudgetProposals.size() - 1) {
                LogPrintf("CFinalizedBudget::AutoCheck - Vector size mismatch, aborting\n");
                return;
            }

            if(vecBudgetPayments[i].nProposalHash != vBudgetProposals[i]->GetHash()){
                LogPrintf("CFinalizedBudget::AutoCheck - item #%d doesn't match %s %s\n", i, vecBudgetPayments[i].nProposalHash.ToString(), vBudgetProposals[i]->GetHash().ToString());
                return;
            }

            // if(vecBudgetPayments[i].payee != vBudgetProposals[i]->GetPayee()){ -- triggered with false positive 
            if(vecBudgetPayments[i].payee != vBudgetProposals[i]->GetPayee()){
                LogPrintf("CFinalizedBudget::AutoCheck - item #%d payee doesn't match %s %s\n", i, ScriptToAsmStr(vecBudgetPayments[i].payee), ScriptToAsmStr(vBudgetProposals[i]->GetPayee()));
                return;
            }

            if(vecBudgetPayments[i].nAmount != vBudgetProposals[i]->GetAmount()){
                LogPrintf("CFinalizedBudget::AutoCheck - item #%d payee doesn't match %lli %lli\n", i, vecBudgetPayments[i].nAmount, vBudgetProposals[i]->GetAmount());
                return;
            }
        }

        LogPrintf("CFinalizedBudget::AutoCheck - Finalized Budget Matches! Submitting Vote.\n");
        SubmitVote();

    }
}
// If masternode voted for a proposal, but is now invalid -- remove the vote
void CFinalizedBudget::CleanAndRemove(bool fSignatureCheck)
{
    std::map<uint256, CGovernanceVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        std::string strReason = "";
        (*it).second.fValid = (*it).second.IsValid(fSignatureCheck, strReason);
        ++it;
    }
}


CAmount CFinalizedBudget::GetTotalPayout()
{
    CAmount ret = 0;

    for(unsigned int i = 0; i < vecBudgetPayments.size(); i++){
        ret += vecBudgetPayments[i].nAmount;
    }

    return ret;
}

std::string CFinalizedBudget::GetProposals() 
{
    LOCK(cs);
    std::string ret = "";

    BOOST_FOREACH(CTxBudgetPayment& budgetPayment, vecBudgetPayments){
        CGovernanceNode* pbudgetProposal = govman.FindGovernanceObject(budgetPayment.nProposalHash);

        std::string token = budgetPayment.nProposalHash.ToString();

        if(pbudgetProposal) token = pbudgetProposal->GetName();
        if(ret == "") {ret = token;}
        else {ret += "," + token;}
    }
    return ret;
}

std::string CFinalizedBudget::GetStatus()
{
    std::string retBadHashes = "";
    std::string retBadPayeeOrAmount = "";

    for(int nBlockHeight = GetBlockStart(); nBlockHeight <= GetBlockEnd(); nBlockHeight++)
    {
        CTxBudgetPayment budgetPayment;
        if(!GetBudgetPaymentByBlock(nBlockHeight, budgetPayment)){
            LogPrintf("CFinalizedBudget::GetStatus - Couldn't find budget payment for block %lld\n", nBlockHeight);
            continue;
        }

        CGovernanceNode* pbudgetProposal =  govman.FindGovernanceObject(budgetPayment.nProposalHash);
        if(!pbudgetProposal){
            if(retBadHashes == ""){
                retBadHashes = "Unknown proposal hash! Check this proposal before voting" + budgetPayment.nProposalHash.ToString();
            } else {
                retBadHashes += "," + budgetPayment.nProposalHash.ToString();
            }
        } else {
            if(pbudgetProposal->GetPayee() != budgetPayment.payee || pbudgetProposal->GetAmount() != budgetPayment.nAmount)
            {
                if(retBadPayeeOrAmount == ""){
                    retBadPayeeOrAmount = "Budget payee/nAmount doesn't match our proposal! " + budgetPayment.nProposalHash.ToString();
                } else {
                    retBadPayeeOrAmount += "," + budgetPayment.nProposalHash.ToString();
                }
            }
        }
    }

    if(retBadHashes == "" && retBadPayeeOrAmount == "") return "OK";

    return retBadHashes + retBadPayeeOrAmount;
}

bool CFinalizedBudget::IsValid(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral)
{
    //must be the correct block for payment to happen (once a month)
    if(nBlockStart % Params().GetConsensus().nBudgetPaymentsCycleBlocks != 0) {strError = "Invalid BlockStart"; return false;}
    if(GetBlockEnd() - nBlockStart > Params().GetConsensus().nBudgetPaymentsWindowBlocks) {strError = "Invalid BlockEnd"; return false;}
    if((int)vecBudgetPayments.size() > Params().GetConsensus().nBudgetPaymentsWindowBlocks) {strError = "Invalid budget payments count (too many)"; return false;}
    if(strBudgetName == "") {strError = "Invalid Budget Name"; return false;}
    if(nBlockStart == 0) {strError = "Invalid BlockStart == 0"; return false;}
    if(nFeeTXHash == uint256()) {strError = "Invalid FeeTx == 0"; return false;}

    //can only pay out 10% of the possible coins (min value of coins)
    if(GetTotalPayout() > govman.GetTotalBudget(nBlockStart)) {strError = "Invalid Payout (more than max)"; return false;}

    std::string strError2 = "";
    if(fCheckCollateral){
        int nConf = 0;
        if(!IsBudgetCollateralValid(nFeeTXHash, GetHash(), strError2, nTime, nConf)){
            strError = "Invalid Collateral : " + strError2;
            return false;
        }
    }

    //TODO: if N cycles old, invalid, invalid

    if(!pindex) return true;

    if(nBlockStart < pindex->nHeight - Params().GetConsensus().nBudgetPaymentsWindowBlocks) {
        strError = "Older than current blockHeight";
        return false;
    }

    return true;
}

bool CFinalizedBudget::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    int nCurrentBudgetPayment = nBlockHeight - GetBlockStart();
    if(nCurrentBudgetPayment < 0) {
        LogPrintf("CFinalizedBudget::IsTransactionValid - Invalid block - height: %d start: %d\n", nBlockHeight, GetBlockStart());
        return false;
    }

    if(nCurrentBudgetPayment > (int)vecBudgetPayments.size() - 1) {
        LogPrintf("CFinalizedBudget::IsTransactionValid - Invalid block - current budget payment: %d of %d\n", nCurrentBudgetPayment + 1, (int)vecBudgetPayments.size());
        return false;
    }

    bool found = false;
    BOOST_FOREACH(CTxOut out, txNew.vout)
    {
        if(vecBudgetPayments[nCurrentBudgetPayment].payee == out.scriptPubKey && vecBudgetPayments[nCurrentBudgetPayment].nAmount == out.nValue)
            found = true;
    }
    if(!found) {
        CTxDestination address1;
        ExtractDestination(vecBudgetPayments[nCurrentBudgetPayment].payee, address1);
        CBitcoinAddress address2(address1);

        LogPrintf("CFinalizedBudget::IsTransactionValid - Missing required payment - %s: %d\n", address2.ToString(), vecBudgetPayments[nCurrentBudgetPayment].nAmount);
    }
    
    return found;
}

void CFinalizedBudget::SubmitVote()
{
    CPubKey pubKeyMasternode;
    CKey keyMasternode;
    std::string errorMessage;

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode)){
        LogPrintf("CFinalizedBudget::SubmitVote - Error upon calling SetKey\n");
        return;
    }

    CGovernanceNode* goParent = govman.FindGovernanceObject(GetHash());
    if(!goParent)
    {
        return;
    }

    CGovernanceVote vote(goParent, activeMasternode.vin, GetHash(), VOTE_YES);
    if(!vote.Sign(keyMasternode, pubKeyMasternode)){
        LogPrintf("CFinalizedBudget::SubmitVote - Failure to sign.");
        return;
    }

    std::string strError = "";
    if(govman.UpdateGovernanceObjectVotes(vote, NULL, strError)){
        LogPrintf("CFinalizedBudget::SubmitVote  - new finalized budget vote - %s\n", vote.GetHash().ToString());

        govman.mapSeenGovernanceVotes.insert(make_pair(vote.GetHash(), vote));
        vote.Relay();
    } else {
        LogPrintf("CFinalizedBudget::SubmitVote : Error submitting vote - %s\n", strError);
    }
}

CFinalizedBudgetBroadcast::CFinalizedBudgetBroadcast()
{
    strBudgetName = "";
    nBlockStart = 0;
    vecBudgetPayments.clear();
    mapVotes.clear();
    vchSig.clear();
    nFeeTXHash = uint256();
}

CFinalizedBudgetBroadcast::CFinalizedBudgetBroadcast(const CFinalizedBudget& other)
{
    strBudgetName = other.strBudgetName;
    nBlockStart = other.nBlockStart;
    BOOST_FOREACH(CTxBudgetPayment out, other.vecBudgetPayments) vecBudgetPayments.push_back(out);
    mapVotes = other.mapVotes;
    nFeeTXHash = other.nFeeTXHash;
}

CFinalizedBudgetBroadcast::CFinalizedBudgetBroadcast(std::string strBudgetNameIn, int nBlockStartIn, std::vector<CTxBudgetPayment> vecBudgetPaymentsIn, uint256 nFeeTXHashIn)
{
    strBudgetName = strBudgetNameIn;
    nBlockStart = nBlockStartIn;
    BOOST_FOREACH(CTxBudgetPayment out, vecBudgetPaymentsIn) vecBudgetPayments.push_back(out);
    mapVotes.clear();
    nFeeTXHash = nFeeTXHashIn;
}

void CFinalizedBudgetBroadcast::Relay()
{
    CInv inv(MSG_GOVERNANCE_FINALIZED_BUDGET, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

std::string CGovernanceManager::ToString() const
{
    std::ostringstream info;

    info << "Proposals: " << (int)mapGovernanceObjects.size() <<
            ", Budgets: " << (int)mapFinalizedBudgets.size() <<
            ", Seen Budgets: " << (int)mapSeenGovernanceObjects.size() <<
            ", Seen Budget Votes: " << (int)mapSeenGovernanceVotes.size() <<
            ", Seen Final Budgets: " << (int)mapSeenFinalizedBudgets.size();

    return info.str();
}

void CGovernanceManager::UpdatedBlockTip(const CBlockIndex *pindex)
{
    pCurrentBlockIndex = pindex;
    LogPrint("mnbudget", "pCurrentBlockIndex->nHeight: %d\n", pCurrentBlockIndex->nHeight);

    if(!fLiteMode && masternodeSync.RequestedMasternodeAssets > MASTERNODE_SYNC_LIST)
        NewBlock();
}

CGovernanceNode::CGovernanceNode(UniValue obj)
{

        if 'type' == 'dash-network':
            CCategory cat(json['level']);
            if not cat.isSubcategoryOf("level"):
                return Error("Invalid subcategory");
            else:
                SetLevel(json['level']);

            //... etc ...
}

int64_t CGovernanceNode::GetValidStartTimestamp()
{
    //Proposals/Settings/Switches have no restrictions
    if(nGovernanceType == Proposal || nGovernanceType == Setting || nGovernanceType == Switch) return 0;

    //Contracts have a two week voting period, afterwhich is locked in
    if(nGovernanceType == Contract) return nTime - (60*60); //an hour earlier than when this was created

    return 0;
}

int64_t CGovernanceNode::GetValidEndTimestamp()
{
    //Proposals/Settings/Switches have no restrictions
    if(nGovernanceType == Proposal || nGovernanceType == Setting || nGovernanceType == Switch) return 32503680000;

    //Contracts have a two week voting period, afterwhich is locked in
    if(nGovernanceType == Contract) return nTime + (60*60*24*14); //two week window

    return 32503680000;
}

GovernanceObjectType CGovernanceManager::GetGovernanceTypeByHash(uint256 nHash)
{
    // covers proposals, contracts, switches and settings
    CGovernanceNode* obj = FindGovernanceObject(nHash);
    if(obj) return obj->GetGovernanceType();

    // finalized budgets
    CFinalizedBudget* obj2 = FindFinalizedBudget(nHash);
    if(obj2) return FinalizedBudget;

    return Error;
}

GovernanceObjectType CGovernanceNode::GetGovernanceType()
{
    return (GovernanceObjectType)nGovernanceType;   
}

std::string CGovernanceNode::GetGovernanceTypeAsString()
{
    return GovernanceTypeToString((GovernanceObjectType)nGovernanceType);   
}

void CGovernanceNode::SetNull()
{
    fValid = false;
    strName = "unknown";
    nGovernanceType = -1;
    
    strURL = "";
    nBlockStart = 0;
    nBlockEnd = 576*365*100;
    nAmount = 0;
    nTime = 0;
    nFeeTXHash = uint256();
}

void CGovernanceNodeBroadcast::CreateProposalOrContract(GovernanceObjectType nTypeIn, std::string strNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn, uint256 nFeeTXHashIn)
{
    nGovernanceType = (int)nTypeIn;

    strName = strNameIn;
    strURL = strURLIn;

    nBlockStart = nBlockStartIn;

    int nPaymentsStart = nBlockStart - nBlockStart % Params().GetConsensus().nBudgetPaymentsCycleBlocks;
    //calculate the end of the cycle for this vote, add half a cycle (vote will be deleted after that block)
    nBlockEnd = nPaymentsStart + Params().GetConsensus().nBudgetPaymentsCycleBlocks * nPaymentCount;

    address = addressIn;
    nAmount = nAmountIn;

    nFeeTXHash = nFeeTXHashIn;
}

void CGovernanceNodeBroadcast::CreateProposal(std::string strNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn, uint256 nFeeTXHashIn)
{
    //they are exactly the same parameter wise, all changed are in enforcement of the voting window and budgeting logic
    CreateProposalOrContract(Proposal, strNameIn, strURLIn, nPaymentCount, addressIn, nAmountIn, nBlockStartIn, nFeeTXHashIn);
}

void CGovernanceNodeBroadcast::CreateContract(std::string strNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn, uint256 nFeeTXHashIn)
{
    //they are exactly the same parameter wise, all changed are in enforcement of the voting window and budgeting logic
    CreateProposalOrContract(Contract, strNameIn, strURLIn, nPaymentCount, addressIn, nAmountIn, nBlockStartIn, nFeeTXHashIn);
}

void CGovernanceNodeBroadcast::CreateSetting(std::string strNameIn, std::string strURLIn, std::string strSuggestedValueIn, uint256 nFeeTXHashIn)
{
    GovernanceObjectType nType = Setting;
    nGovernanceType = (int)nType;

    strURL = strURLIn; //url where the argument for activation is
    strName = strNameIn; 
    strSuggestedValue = strSuggestedValueIn;
    nFeeTXHash = nFeeTXHashIn;
}

CGovernanceNode::CGovernanceNode()
{
    strName = "unknown";
    nBlockStart = 0;
    nBlockEnd = 0;
    nAmount = 0;
    nTime = 0;
    fValid = true;
    nGovernanceType = 0;
}

CGovernanceNode::CGovernanceNode(const CGovernanceNode& other)
{
    strName = other.strName;
    strURL = other.strURL;
    nBlockStart = other.nBlockStart;
    nBlockEnd = other.nBlockEnd;
    address = other.address;
    nAmount = other.nAmount;
    nTime = other.nTime;
    nFeeTXHash = other.nFeeTXHash;
    mapVotes = other.mapVotes;
    nGovernanceType = other.nGovernanceType;
    fValid = true;
}

bool CGovernanceNode::IsCategoryValid()
{    
    std::string strRootCat = govman.FindCategoryRoot(category);
    std::string strPrimaryTypeName = PrimaryTypeToString(GetType());
    return strRootCat == strPrimaryTypeName;
}

bool CGovernanceNode::IsValid(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral)
{

    if(GetNoCount() - GetYesCount() > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10){
         strError = "Active removal";
         return false;
    }

    if(nBlockStart < 0) {
        strError = "Invalid Proposal";
        return false;
    }

    if(!pindex) {
        strError = "Tip is NULL";
        return true;
    }

    if(nBlockStart % Params().GetConsensus().nBudgetPaymentsCycleBlocks != 0){
        int nNext = pindex->nHeight - pindex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks + Params().GetConsensus().nBudgetPaymentsCycleBlocks;
        strError = strprintf("Invalid block start - must be a budget cycle block. Next valid block: %d", nNext);
        return false;
    }

    if(nBlockEnd % Params().GetConsensus().nBudgetPaymentsCycleBlocks != Params().GetConsensus().nBudgetPaymentsCycleBlocks/2){
        strError = "Invalid block end";
        return false;
    }

    if(nBlockEnd < nBlockStart) {
        strError = "Invalid block end - must be greater then block start.";
        return false;
    }

    if(nAmount < 1*COIN) {
        strError = "Invalid proposal amount";
        return false;
    }

    if(strName.size() > 20) {
        strError = "Invalid proposal name, limit of 20 characters.";
        return false;
    }

    if(strName != SanitizeString(strName)) {
        strError = "Invalid proposal name, unsafe characters found.";
        return false;
    }

    if(strURL.size() > 64) {
        strError = "Invalid proposal url, limit of 64 characters.";
        return false;
    }

    if(strURL != SanitizeString(strURL)) {
        strError = "Invalid proposal url, unsafe characters found.";
        return false;
    }

    if(address == CScript()) {
        strError = "Invalid proposal Payment Address";
        return false;
    }

    if(IsCategoryValid()) {
        // NOTE: PRINT CATEGORY TO STRING
        strError = "Category type is invalid";
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

    // -- If GetAbsoluteYesCount is more than -10% of the network, flag as invalid
    if(GetAbsoluteYesCount() < -(mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10)) {
        strError = "Voted Down";
        return false;
    }

    //can only pay out 10% of the possible coins (min value of coins)
    if(nAmount > govman.GetTotalBudget(nBlockStart)) {
        strError = "Payment more than max";
        return false;
    }

    if(GetBlockEnd() + Params().GetConsensus().nBudgetPaymentsWindowBlocks < pindex->nHeight) return false;

    /*
        governance checking through the dynamic part of the system
    */

    // this verifies the node tree rules
    if(!pParent.CanAdd((*this), strError))
    {
        return false;
    }
    
    // we need to verify the user signature and the parent nodes key possible as well
    if(!CGovernanceBrain::IsPlacementValid(GetType(), pParent->GetType())
    {
        return false;
    }
 
    // if(!pParent.IsChildValid((*this), strError))
    // {
    //     return false;
    // }

    // in either case they should have signed this message correctly too
    //  -- isn't this done elsewhere? Should it be last?
    if(!VerifySignature(strError))
    {
        return false;
    }


    return true;
}

bool CGovernanceNode::IsEstablished() {
    //Proposals must be established to make it into a budget
    return (nTime < GetTime() - Params().GetConsensus().nBudgetProposalEstablishingTime);
}

bool CGovernanceNode::AddOrUpdateVote(CGovernanceVote& vote, std::string& strError)
{
    LOCK(cs);

    uint256 hash = vote.vin.prevout.GetHash();

    if(mapVotes.count(hash)){
        if(mapVotes[hash].nTime > vote.nTime){
            strError = strprintf("new vote older than existing vote - %s", vote.GetHash().ToString());
            LogPrint("mnbudget", "CGovernanceNode::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        if(vote.nTime - mapVotes[hash].nTime < BUDGET_VOTE_UPDATE_MIN){
            strError = strprintf("time between votes is too soon - %s - %lli", vote.GetHash().ToString(), vote.nTime - mapVotes[hash].nTime);
            LogPrint("mnbudget", "CGovernanceNode::AddOrUpdateVote - %s\n", strError);
            return false;
        }
    }

    mapVotes[hash] = vote;
    return true;
}

// If masternode voted for a proposal, but is now invalid -- remove the vote
void CGovernanceNode::CleanAndRemove(bool fSignatureCheck)
{
    std::map<uint256, CGovernanceVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        std::string strReason = "";
        (*it).second.fValid = (*it).second.IsValid(fSignatureCheck, strReason);
        ++it;
    }
}

double CGovernanceNode::GetRatio()
{
    int yeas = 0;
    int nays = 0;

    std::map<uint256, CGovernanceVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        if ((*it).second.nVote == VOTE_YES) yeas++;
        if ((*it).second.nVote == VOTE_NO) nays++;
        ++it;
    }

    if(yeas+nays == 0) return 0.0f;

    return ((double)(yeas) / (double)(yeas+nays));
}

int CGovernanceNode::GetAbsoluteYesCount()
{
    return GetYesCount() - GetNoCount();
}

int CGovernanceNode::GetYesCount()
{
    int ret = 0;

    std::map<uint256, CGovernanceVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end()){
        if ((*it).second.nVote == VOTE_YES && (*it).second.fValid) ret++;
        ++it;
    }

    return ret;
}

int CGovernanceNode::GetNoCount()
{
    int ret = 0;

    std::map<uint256, CGovernanceVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end()){
        if ((*it).second.nVote == VOTE_NO && (*it).second.fValid) ret++;
        ++it;
    }

    return ret;
}

int CGovernanceNode::GetAbstainCount()
{
    int ret = 0;

    std::map<uint256, CGovernanceVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end()){
        if ((*it).second.nVote == VOTE_ABSTAIN && (*it).second.fValid) ret++;
        ++it;
    }

    return ret;
}

int CGovernanceNode::GetBlockStartCycle()
{
    //end block is half way through the next cycle (so the proposal will be removed much after the payment is sent)

    return nBlockStart - nBlockStart % Params().GetConsensus().nBudgetPaymentsCycleBlocks;
}

int CGovernanceNode::GetBlockCurrentCycle(const CBlockIndex* pindex)
{
    if(!pindex) return -1;

    if(pindex->nHeight >= GetBlockEndCycle()) return -1;

    return pindex->nHeight - pindex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks;
}

int CGovernanceNode::GetBlockEndCycle()
{
    //end block is half way through the next cycle (so the proposal will be removed much after the payment is sent)

    return nBlockEnd - Params().GetConsensus().nBudgetPaymentsCycleBlocks/2;
}

int CGovernanceNode::GetTotalPaymentCount()
{
    return (GetBlockEndCycle() - GetBlockStartCycle()) / Params().GetConsensus().nBudgetPaymentsCycleBlocks;
}

int CGovernanceNode::GetRemainingPaymentCount(int nBlockHeight)
{
    int nPayments = 0;
    // printf("-> Budget Name : %s\n", strName.c_str());
    // printf("------- nBlockStart : %d\n", nBlockStart);
    // printf("------- nBlockEnd : %d\n", nBlockEnd);
    while(nBlockHeight + Params().GetConsensus().nBudgetPaymentsCycleBlocks < GetBlockEndCycle())
    {
        // printf("------- P : %d %d - %d < %d - %d\n", nBlockHeight, nPayments, nBlockHeight + Params().GetConsensus().nBudgetPaymentsCycleBlocks, nBlockEnd, nBlockHeight + Params().GetConsensus().nBudgetPaymentsCycleBlocks < nBlockEnd);
        nBlockHeight += Params().GetConsensus().nBudgetPaymentsCycleBlocks;
        nPayments++;
    }
    return nPayments;
}


// for contracts (should we subclass a contract object??)
int64_t CGovernanceNode::GetContractActivationTime()
{
    //contracts always have at least two weeks
    return nTime+CONTRACT_ACTIVATION_TIME;
}

bool CGovernanceNode::IsContractActivated()
{
    return GetTime() > GetContractActivationTime();
}

GovernanceObjectType CGovernanceNode::GetType()
{
    return (GovernanceObjectType)nGovernanceType;
}

int64_t CGovernanceNode::GetValidStartTimestamp()
{
    if(pParent1) {return pParent1->GetValidStartTimestamp();}
    if(pParent2) {return pParent2->GetValidStartTimestamp();}
    return -1;
}

int64_t CGovernanceNode::GetValidEndTimestamp()
{
    if(pParent1) {return pParent1->GetValidEndTimestamp();}
    if(pParent2) {return pParent2->GetValidEndTimestamp();}
    return -1;
}