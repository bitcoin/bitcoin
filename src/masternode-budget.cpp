// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "init.h"

#include "masternode-budget.h"
#include "masternode.h"
#include "darksend.h"
#include "masternodeman.h"
#include "masternode-sync.h"
#include "util.h"
#include "addrman.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

CBudgetManager budget;
CCriticalSection cs_budget;

std::map<uint256, CBudgetProposalBroadcast> mapSeenMasternodeBudgetProposals;
std::map<uint256, CBudgetVote> mapSeenMasternodeBudgetVotes;
std::map<uint256, CBudgetVote> mapOrphanMasternodeBudgetVotes;
std::map<uint256, CFinalizedBudgetBroadcast> mapSeenFinalizedBudgets;
std::map<uint256, CFinalizedBudgetVote> mapSeenFinalizedBudgetVotes;
std::map<uint256, CFinalizedBudgetVote> mapOrphanFinalizedBudgetVotes;

std::map<uint256, int64_t> askedForSourceProposalOrBudget;

int nSubmittedFinalBudget;

int GetBudgetPaymentCycleBlocks(){
    if(Params().NetworkID() == CBaseChainParams::MAIN) return 16616; //(60*24*30)/2.6

    //for testing purposes
    return 50;
}

bool IsBudgetCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError)
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
        strError = strprintf("Couldn't find opReturn %s", txCollateral.ToString());
        LogPrintf ("CBudgetProposalBroadcast::IsBudgetCollateralValid - %s\n", strError);
        return false;
    }

    int conf = GetIXConfirmations(nTxCollateralHash);
    if (nBlockHash != 0) {
        BlockMap::iterator mi = mapBlockIndex.find(nBlockHash);
        if (mi != mapBlockIndex.end() && (*mi).second) {
            CBlockIndex* pindex = (*mi).second;
            if (chainActive.Contains(pindex)) {
                conf += (1 + chainActive.Height() - pindex->nHeight);
            }
        }
    }

    if(conf < BUDGET_FEE_CONFIRMATIONS){
        strError = strprintf("Collateral requires at least %d confirmations - %d confirmations", BUDGET_FEE_CONFIRMATIONS, conf);
        LogPrintf ("CBudgetProposalBroadcast::IsBudgetCollateralValid - %s - %d confirmations\n", strError, conf);
        return false;
    }

    return true;
}

void CBudgetManager::CheckOrphanVotes()
{
    std::map<uint256, CBudgetVote>::iterator it1 = mapOrphanMasternodeBudgetVotes.begin();
    while(it1 != mapOrphanMasternodeBudgetVotes.end()){
        if(budget.UpdateProposal(((*it1).second), NULL)){
            LogPrintf("CheckOrphanVotes: Proposal/Budget is known, activating and removing orphan vote\n");
            mapOrphanMasternodeBudgetVotes.erase(it1++);
        } else {
            ++it1;
        }
    }
    std::map<uint256, CFinalizedBudgetVote>::iterator it2 = mapOrphanFinalizedBudgetVotes.begin();
    while(it2 != mapOrphanFinalizedBudgetVotes.end()){
        if(budget.UpdateFinalizedBudget(((*it2).second),NULL)){
            LogPrintf("CheckOrphanVotes: Proposal/Budget is known, activating and removing orphan vote\n");
            mapOrphanFinalizedBudgetVotes.erase(it2++);
        } else {
            ++it2;
        }
    }
}

void CBudgetManager::SubmitFinalBudget()
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;

    int nBlockStart = pindexPrev->nHeight-(pindexPrev->nHeight % GetBudgetPaymentCycleBlocks())+GetBudgetPaymentCycleBlocks();
    if(nSubmittedFinalBudget >= nBlockStart) return;
    if(nBlockStart - pindexPrev->nHeight > 100) return;

    std::vector<CBudgetProposal*> vBudgetProposals = budget.GetBudget();
    std::string strBudgetName = "main";
    std::vector<CTxBudgetPayment> vecTxBudgetPayments;

    for(unsigned int i = 0; i < vBudgetProposals.size(); i++){
        CTxBudgetPayment out;
        out.nProposalHash = vBudgetProposals[i]->GetHash();
        out.payee = vBudgetProposals[i]->GetPayee();
        out.nAmount = vBudgetProposals[i]->GetAmount();
        vecTxBudgetPayments.push_back(out);
    }

    if(vecTxBudgetPayments.size() < 1) {
        LogPrintf("SubmitFinalBudget - Found No Proposals For Period\n");
        return;
    }

    CPubKey pubKeyMasternode;
    CKey keyMasternode;
    std::string errorMessage;

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode)){
        LogPrintf("SubmitFinalBudget - Error upon calling SetKey\n");
        return;
    }

    CFinalizedBudgetBroadcast tempBudget(strBudgetName, nBlockStart, vecTxBudgetPayments, 0);

    //create fee tx
    CTransaction tx;
    if(!mapCollateral.count(tempBudget.GetHash())){
        CWalletTx wtx;
        if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, tempBudget.GetHash(), true)){
            LogPrintf("SubmitFinalBudget - Can't make collateral transaction\n");
            return;
        }
        
        // make our change address
        CReserveKey reservekey(pwalletMain);
        //send the tx to the network
        pwalletMain->CommitTransaction(wtx, reservekey, "ix");

        mapCollateral.insert(make_pair(tempBudget.GetHash(), (CTransaction)wtx));
        tx = (CTransaction)wtx;
    } else {
        tx = mapCollateral[tempBudget.GetHash()];
    }

    CTxIn in(COutPoint(tx.GetHash(), 0));
    int conf = GetInputAgeIX(tx.GetHash(), in);
    if(conf < BUDGET_FEE_CONFIRMATIONS+1){
        LogPrintf ("CBudgetProposalBroadcast::IsBudgetCollateralValid - Collateral requires at least 6 confirmations - %s - %d confirmations\n", tx.GetHash().ToString(), conf);
        return;
    }

    nSubmittedFinalBudget = nBlockStart;

    //create the proposal incase we're the first to make it
    CFinalizedBudgetBroadcast finalizedBudgetBroadcast(strBudgetName, nBlockStart, vecTxBudgetPayments, tx.GetHash());

    if(!finalizedBudgetBroadcast.IsValid()){
        LogPrintf("SubmitFinalBudget - Invalid finalized budget broadcast (are all the hashes correct?)\n");
        return;
    }

    mapSeenFinalizedBudgets.insert(make_pair(finalizedBudgetBroadcast.GetHash(), finalizedBudgetBroadcast));
    finalizedBudgetBroadcast.Relay();
    budget.AddFinalizedBudget(finalizedBudgetBroadcast);

    CFinalizedBudgetVote vote(activeMasternode.vin, finalizedBudgetBroadcast.GetHash());
    if(!vote.Sign(keyMasternode, pubKeyMasternode)){
        LogPrintf("SubmitFinalBudget - Failure to sign.\n");
        return;
    }

    mapSeenFinalizedBudgetVotes.insert(make_pair(vote.GetHash(), vote));
    vote.Relay();
    budget.UpdateFinalizedBudget(vote, NULL);
}

//
// CBudgetDB
//

CBudgetDB::CBudgetDB()
{
    pathDB = GetDataDir() / "budget.dat";
    strMagicMessage = "MasternodeBudget";
}

bool CBudgetDB::Write(const CBudgetManager& objToSave)
{
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

    return true;
}

CBudgetDB::ReadResult CBudgetDB::Read(CBudgetManager& objToLoad)
{

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

        // de-serialize data into CBudgetManager object
        ssObj >> objToLoad;
    }
    catch (std::exception &e) {
        objToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }


    objToLoad.CheckAndRemove(); // clean out expired
    LogPrintf("Loaded info from budget.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", objToLoad.ToString());

    return Ok;
}

void DumpBudgets()
{
    int64_t nStart = GetTimeMillis();

    CBudgetDB mndb;

    LogPrintf("Writting info to budget.dat...\n");
    mndb.Write(budget);

    LogPrintf("Budget dump finished  %dms\n", GetTimeMillis() - nStart);
}

void CBudgetManager::AddFinalizedBudget(CFinalizedBudget& finalizedBudget)
{
    LOCK(cs);
    if(!finalizedBudget.IsValid()) return;

    if(mapFinalizedBudgets.count(finalizedBudget.GetHash())) {
        return;
    }

    mapFinalizedBudgets.insert(make_pair(finalizedBudget.GetHash(), finalizedBudget));
}

void CBudgetManager::AddProposal(CBudgetProposal& budgetProposal)
{
    LOCK(cs);
    std::string strError = "";
    if(!budgetProposal.IsValid(strError)) {
        LogPrintf("mprop - invalid budget proposal - %s\n", strError.c_str());
        return;
    }

    if(mapProposals.count(budgetProposal.GetHash())) {
        return;
    }

    mapProposals.insert(make_pair(budgetProposal.GetHash(), budgetProposal));
}

void CBudgetManager::CheckAndRemove()
{
    std::string strError = "";
    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);
        if(!pfinalizedBudget->IsValid()){
            pfinalizedBudget->fValid = false;
        } else {
            pfinalizedBudget->fValid = true;
            pfinalizedBudget->AutoCheck();
        }
        ++it;
    }

    std::map<uint256, CBudgetProposal>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end())
    {
        CBudgetProposal* pbudgetProposal = &((*it2).second);
        if(!pbudgetProposal->IsValid(strError)){
            pbudgetProposal->fValid = false;
        } else {
            pbudgetProposal->fValid = true;
        }
        ++it2;
    }
}

void CBudgetManager::FillBlockPayee(CMutableTransaction& txNew, int64_t nFees)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;

    int nHighestCount = 0;
    CScript payee;
    int64_t nAmount = 0;

    // ------- Grab The Highest Count

    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);
        if(pfinalizedBudget->GetVoteCount() > nHighestCount){
            if(pindexPrev->nHeight+1 >= pfinalizedBudget->GetBlockStart() && pindexPrev->nHeight+1 <= pfinalizedBudget->GetBlockEnd()){
                if(pfinalizedBudget->GetPayeeAndAmount(pindexPrev->nHeight+1, payee, nAmount)){
                    nHighestCount = pfinalizedBudget->GetVoteCount();
                }
            }
        }

        it++;
    }

    CAmount blockValue = GetBlockValue(pindexPrev->nBits, pindexPrev->nHeight, nFees);

    //miners get the full amount on these blocks
    txNew.vout[0].nValue = blockValue;

    if(nHighestCount > 0){
        txNew.vout.resize(2);

        //these are super blocks, so their value can be much larger than normal
        txNew.vout[1].scriptPubKey = payee;
        txNew.vout[1].nValue = nAmount;

        CTxDestination address1;
        ExtractDestination(payee, address1);
        CBitcoinAddress address2(address1);

        LogPrintf("Budget payment to %s for %lld\n", address2.ToString().c_str(), nAmount);
    }

}

CFinalizedBudget *CBudgetManager::FindFinalizedBudget(uint256 nHash)
{
    if(mapFinalizedBudgets.count(nHash))
        return &mapFinalizedBudgets[nHash];

    return NULL;
}

CBudgetProposal *CBudgetManager::FindProposal(const std::string &strProposalName)
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
    if(mapProposals.count(nHash))
        return &mapProposals[nHash];

    return NULL;
}

bool CBudgetManager::IsBudgetPaymentBlock(int nBlockHeight){
    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);
        if(nBlockHeight >= pfinalizedBudget->GetBlockStart() && nBlockHeight <= pfinalizedBudget->GetBlockEnd()){
            return true;
        }

        it++;
    }

    return false;
}

bool CBudgetManager::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    int nHighestCount = 0;
    std::vector<CFinalizedBudget*> ret;

    // ------- Grab The Highest Count

    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);
        if(pfinalizedBudget->GetVoteCount() > nHighestCount){
            if(nBlockHeight >= pfinalizedBudget->GetBlockStart() && nBlockHeight <= pfinalizedBudget->GetBlockEnd()){
                nHighestCount = pfinalizedBudget->GetVoteCount();
            }
        }

        it++;
    }

    if(nHighestCount < mnodeman.CountEnabled()/20) return true;

    // check the highest finalized budgets (+/- 10% to assist in consensus)

    std::map<uint256, CFinalizedBudget>::iterator it2 = mapFinalizedBudgets.begin();
    while(it2 != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it2).second);

        if(pfinalizedBudget->GetVoteCount() > nHighestCount-(mnodeman.CountEnabled()/10)){
            if(nBlockHeight >= pfinalizedBudget->GetBlockStart() && nBlockHeight <= pfinalizedBudget->GetBlockEnd()){
                if(pfinalizedBudget->IsTransactionValid(txNew, nBlockHeight)){
                    return true;
                }
            }
        }

        it2++;
    }

    //we looked through all of the known budgets
    return false;
}

std::vector<CBudgetProposal*> CBudgetManager::GetAllProposals()
{
    std::vector<CBudgetProposal*> vBudgetProposalRet;

    std::map<uint256, CBudgetProposal>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end())
    {
        (*it2).second.CleanAndRemove();

        CBudgetProposal* pbudgetProposal = &((*it2).second);
        vBudgetProposalRet.push_back(pbudgetProposal);

        it2++;
    }

    return vBudgetProposalRet;
}

//Need to review this function
std::vector<CBudgetProposal*> CBudgetManager::GetBudget()
{
    // ------- Sort budgets by Yes Count

    std::map<uint256, int> mapList;

    std::map<uint256, CBudgetProposal>::iterator it = mapProposals.begin();
    while(it != mapProposals.end()){
        (*it).second.CleanAndRemove();
        mapList.insert(make_pair((*it).second.GetHash(), (*it).second.GetYeas()));
        ++it;
    }

    //sort the map and grab the highest count item
    std::vector<std::pair<uint256,int> > vecList(mapList.begin(), mapList.end());
    std::sort(vecList.begin(),vecList.end());

    // ------- Grab The Budgets In Order

    std::vector<CBudgetProposal*> vBudgetProposalRet;

    int64_t nBudgetAllocated = 0;
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return vBudgetProposalRet;

    int nBlockStart = pindexPrev->nHeight-(pindexPrev->nHeight % GetBudgetPaymentCycleBlocks())+GetBudgetPaymentCycleBlocks();
    int nBlockEnd  =  nBlockStart + GetBudgetPaymentCycleBlocks() -1;
    int64_t nTotalBudget = GetTotalBudget(nBlockStart);


    std::map<uint256, CBudgetProposal>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end())
    {
        CBudgetProposal* pbudgetProposal = &((*it2).second);

        //prop start/end should be inside this period
        if(pbudgetProposal->fValid && pbudgetProposal->nBlockStart <= nBlockStart && pbudgetProposal->nBlockEnd >= nBlockEnd && pbudgetProposal->GetYeas()-pbudgetProposal->GetNays() > mnodeman.CountEnabled()/10)
        {
            if(nTotalBudget == nBudgetAllocated){
                pbudgetProposal->SetAllotted(0);
            } else if(pbudgetProposal->GetAmount() + nBudgetAllocated <= nTotalBudget) {
                pbudgetProposal->SetAllotted(pbudgetProposal->GetAmount());
                nBudgetAllocated += pbudgetProposal->GetAmount();
            } else {
                //couldn't pay for the entire budget, so it'll be partially paid.
                pbudgetProposal->SetAllotted(nTotalBudget - nBudgetAllocated);
                nBudgetAllocated = nTotalBudget;
            }

            vBudgetProposalRet.push_back(pbudgetProposal);
        }

        it2++;
    }

    return vBudgetProposalRet;
}

std::vector<CFinalizedBudget*> CBudgetManager::GetFinalizedBudgets()
{
    std::vector<CFinalizedBudget*> vFinalizedBudgetRet;

    // ------- Grab The Budgets In Order

    std::map<uint256, CFinalizedBudget>::iterator it2 = mapFinalizedBudgets.begin();
    while(it2 != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it2).second);

        vFinalizedBudgetRet.push_back(pfinalizedBudget);
        it2++;
    }

    return vFinalizedBudgetRet;
}

std::string CBudgetManager::GetRequiredPaymentsString(int64_t nBlockHeight)
{
    std::string ret = "unknown-budget";

    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);
        if(nBlockHeight >= pfinalizedBudget->GetBlockStart() && nBlockHeight <= pfinalizedBudget->GetBlockEnd()){
            CTxBudgetPayment payment;
            if(pfinalizedBudget->GetProposalByBlock(nBlockHeight, payment)){
                if(ret == "unknown-budget"){
                    ret = payment.nProposalHash.ToString().c_str();
                } else {
                    ret += ",";
                    ret += payment.nProposalHash.ToString().c_str();
                }
            } else {
                LogPrintf("CBudgetManager::GetRequiredPaymentsString - Couldn't find budget payment for block %lld\n", nBlockHeight);
            }
        }

        it++;
    }

    return ret;
}

int64_t CBudgetManager::GetTotalBudget(int nHeight)
{
    if(chainActive.Tip() == NULL) return 0;

    //get min block value and calculate from that
    int64_t nSubsidy = 5 * COIN;

    if(Params().NetworkID() == CBaseChainParams::TESTNET){
        for(int i = 46200; i <= nHeight; i += 210240) nSubsidy -= nSubsidy/14;
    } else {
        // yearly decline of production by 7.1% per year, projected 21.3M coins max by year 2050.
        for(int i = 210240; i <= nHeight; i += 210240) nSubsidy -= nSubsidy/14;
    }

    return ((nSubsidy/100)*10)*576*30;
}

void CBudgetManager::NewBlock()
{
    budget.CheckAndRemove();

    if (strBudgetMode == "suggest") { //suggest the budget we see
        SubmitFinalBudget();
    }

    //this function should be called 1/6 blocks, allowing up to 100 votes per day on all proposals
    if(chainActive.Height() % 6 != 0) return;

    mnodeman.DecrementVotedTimes();

    //remove invalid votes once in a while (we have to check the signatures and validity of every vote, somewhat CPU intensive)

    std::map<uint256, int64_t>::iterator it = askedForSourceProposalOrBudget.begin();
    while(it != askedForSourceProposalOrBudget.end()){
        if((*it).second > GetTime() - (60*60*24)){
            ++it;
        } else {
            askedForSourceProposalOrBudget.erase(it++);
        }
    }

    std::map<uint256, CBudgetProposal>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end()){
        (*it2).second.CleanAndRemove();
        ++it2;
    }

    std::map<uint256, CFinalizedBudget>::iterator it3 = mapFinalizedBudgets.begin();
    while(it3 != mapFinalizedBudgets.end()){
        (*it3).second.CleanAndRemove();
        ++it3;
    }
}

void CBudgetManager::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    // lite mode is not supported
    if(IsInitialBlockDownload()) return;

    LOCK(cs_budget);

    if (strCommand == "mnvs") { //Masternode vote sync
        uint256 nProp;
        vRecv >> nProp;

        if(pfrom->HasFulfilledRequest("mnvs")) {
            LogPrintf("mnvs - peer already asked me for the list\n");
            Misbehaving(pfrom->GetId(), 20);
            return;
        }
        pfrom->FulfilledRequest("mnvs");

        budget.Sync(pfrom, nProp);
        LogPrintf("mnvs - Sent Masternode votes to %s\n", pfrom->addr.ToString());
    }

    if (strCommand == "mprop") { //Masternode Proposal
        CBudgetProposalBroadcast budgetProposalBroadcast;
        vRecv >> budgetProposalBroadcast;

        if(mapSeenMasternodeBudgetProposals.count(budgetProposalBroadcast.GetHash())){
            return;
        }

        //set time we first saw this prop
        budgetProposalBroadcast.nTime = GetAdjustedTime();

        mapSeenMasternodeBudgetProposals.insert(make_pair(budgetProposalBroadcast.GetHash(), budgetProposalBroadcast));

        std::string strError = "";
        if(!budgetProposalBroadcast.IsValid(strError)) {
            LogPrintf("mprop - invalid budget proposal - %s\n", strError);
            return;
        }

        CBudgetProposal budgetProposal(budgetProposalBroadcast);
        budget.AddProposal(budgetProposal);
        budgetProposalBroadcast.Relay();
        masternodeSync.AddedBudgetItem();

        //We might have active votes for this proposal that are valid now
        CheckOrphanVotes();
    }

    if (strCommand == "mvote") { //Masternode Vote
        CBudgetVote vote;
        vRecv >> vote;

        if(mapSeenMasternodeBudgetVotes.count(vote.GetHash())){
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vin);
        if(pmn == NULL) {
            if(fDebug) LogPrintf("mvote - unknown masternode - vin: %s\n", vote.vin.ToString());
            return;
        }

        mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));

        if(!vote.SignatureValid()){
            LogPrintf("mvote - signature invalid\n");
            Misbehaving(pfrom->GetId(), 20);
            return;
        }

        if(pmn->nVotedTimes < 100){
            budget.UpdateProposal(vote, pfrom);
            vote.Relay();
            if(!masternodeSync.IsSynced()) pmn->nVotedTimes++;
            masternodeSync.AddedBudgetItem();
        } else {
            LogPrintf("mvote - masternode can't vote again - vin: %s\n", pmn->vin.ToString());
            return;
        }
    }

    if (strCommand == "fbs") { //Finalized Budget Suggestion
        CFinalizedBudgetBroadcast finalizedBudgetBroadcast;
        vRecv >> finalizedBudgetBroadcast;

        if(mapSeenFinalizedBudgets.count(finalizedBudgetBroadcast.GetHash())){
            return;
        }

        std::string strError = "";
        if(!IsBudgetCollateralValid(finalizedBudgetBroadcast.nFeeTXHash, finalizedBudgetBroadcast.GetHash(), strError)){
            LogPrintf("Finalized Budget FeeTX is not valid - %s - %s\n", finalizedBudgetBroadcast.nFeeTXHash.ToString(), strError);
            return;
        }

        mapSeenFinalizedBudgets.insert(make_pair(finalizedBudgetBroadcast.GetHash(), finalizedBudgetBroadcast));

        if(!finalizedBudgetBroadcast.IsValid()) {
            LogPrintf("fbs - invalid finalized budget\n");
            return;
        }

        CFinalizedBudget finalizedBudget(finalizedBudgetBroadcast);
        budget.AddFinalizedBudget(finalizedBudget);
        finalizedBudgetBroadcast.Relay();
        masternodeSync.AddedBudgetItem();

        //we might have active votes for this budget that are now valid
        CheckOrphanVotes();
    }

    if (strCommand == "fbvote") { //Finalized Budget Vote
        CFinalizedBudgetVote vote;
        vRecv >> vote;

        if(mapSeenFinalizedBudgetVotes.count(vote.GetHash())){
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vin);
        if(pmn == NULL) {
            if(fDebug) LogPrintf("fbvote - unknown masternode - vin: %s\n", vote.vin.ToString());
            return;
        }

        if(!vote.SignatureValid()){
            LogPrintf("fbvote - signature invalid\n");
            Misbehaving(pfrom->GetId(), 20);
            return;
        }

        mapSeenFinalizedBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        if(pmn->nVotedTimes < 100){
            budget.UpdateFinalizedBudget(vote, pfrom);
            vote.Relay();
            if(!masternodeSync.IsSynced()) pmn->nVotedTimes++;
            masternodeSync.AddedBudgetItem();
        } else {
            LogPrintf("fbvote - masternode can't vote again - vin: %s\n", pmn->vin.ToString());
            return;
        }
    }
}

bool CBudgetManager::PropExists(uint256 nHash)
{
    if(mapProposals.count(nHash)) return true;
    return false;
}

void CBudgetManager::Sync(CNode* pfrom, uint256 nProp)
{
    /*
        Sync with a client on the network

        --

        This code checks each of the hash maps for all known budget proposals and finalized budget proposals, then checks them against the
        budget object to see if they're OK. If all checks pass, we'll send it to the peer.

    */

    std::map<uint256, CBudgetProposalBroadcast>::iterator it1 = mapSeenMasternodeBudgetProposals.begin();
    while(it1 != mapSeenMasternodeBudgetProposals.end()){
        CBudgetProposal* pbudgetProposal = budget.FindProposal((*it1).first);
        if(pbudgetProposal && (nProp == 0 || (*it1).first == nProp)){
            pfrom->PushMessage("mprop", ((*it1).second));
        
            //send votes
            std::map<uint256, CBudgetVote>::iterator it2 = pbudgetProposal->mapVotes.begin();
            while(it2 != pbudgetProposal->mapVotes.end()){
                if((*it2).second.fValid){
                    pfrom->PushMessage("mvote", ((*it2).second));
                }
                it2++;
            }
        }
        it1++;
    }


    std::map<uint256, CFinalizedBudgetBroadcast>::iterator it3 = mapSeenFinalizedBudgets.begin();
    while(it3 != mapSeenFinalizedBudgets.end()){
        CFinalizedBudget* pfinalizedBudget = budget.FindFinalizedBudget((*it3).first);
        if(pfinalizedBudget && (nProp == 0 || (*it3).first == nProp)){
            pfrom->PushMessage("fbs", ((*it3).second));

            //send votes
            std::map<uint256, CFinalizedBudgetVote>::iterator it4 = pfinalizedBudget->mapVotes.begin();
            while(it4 != pfinalizedBudget->mapVotes.end()){
                if((*it4).second.fValid)
                    pfrom->PushMessage("fbvote", ((*it4).second));
                it4++;
            }
        }
        it3++;
    }

}

bool CBudgetManager::UpdateProposal(CBudgetVote& vote, CNode* pfrom)
{
    LOCK(cs);

    if(!mapProposals.count(vote.nProposalHash)){
        if(pfrom){
            LogPrintf("Unknown proposal %d, Asking for source proposal\n", vote.nProposalHash.ToString().c_str());
            mapOrphanMasternodeBudgetVotes[vote.nProposalHash] = vote;

            if(!askedForSourceProposalOrBudget.count(vote.nProposalHash)){
                pfrom->PushMessage("mnvs", vote.nProposalHash);
                askedForSourceProposalOrBudget[vote.nProposalHash] = GetTime();
            }
        }

        return false;
    }


    mapProposals[vote.nProposalHash].AddOrUpdateVote(vote);
    return true;
}

bool CBudgetManager::UpdateFinalizedBudget(CFinalizedBudgetVote& vote, CNode* pfrom)
{
    LOCK(cs);

    if(!mapFinalizedBudgets.count(vote.nBudgetHash)){
        if(pfrom){
            LogPrintf("Unknown Finalized Proposal %s, Asking for source budget\n", vote.nBudgetHash.ToString().c_str());
            mapOrphanFinalizedBudgetVotes[vote.nBudgetHash] = vote;

            if(!askedForSourceProposalOrBudget.count(vote.nBudgetHash)){
                pfrom->PushMessage("mnvs", vote.nBudgetHash);
                askedForSourceProposalOrBudget[vote.nBudgetHash] = GetTime();
            }

        }
        return false;
    }

    mapFinalizedBudgets[vote.nBudgetHash].AddOrUpdateVote(vote);
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
    nTime = 0;
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

bool CBudgetProposal::IsValid(std::string& strError)
{
    if(GetYeas()-GetNays() < -(mnodeman.CountEnabled()/10)){
         strError = "Active removal";
         return false;
    }

    if(nBlockStart < 0) {
        strError = "Invalid Proposal";
        return false;
    }

    if(address == CScript()) {
        strError = "Invalid Payment Address";
        return false;
    }

    if(!IsBudgetCollateralValid(nFeeTXHash, GetHash(), strError)){
        return false;
    }

    //if proposal doesn't gain traction within 2 weeks, remove it
    // nTime not being saved correctly
    // if(nTime + (60*60*24*2) < GetAdjustedTime()) {
    //     if(GetYeas()-GetNays() < (mnodeman.CountEnabled()/10)) {
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

void CBudgetProposal::AddOrUpdateVote(CBudgetVote& vote)
{
    LOCK(cs);

    uint256 hash = vote.vin.prevout.GetHash();
    mapVotes[hash] = vote;
}

// If masternode voted for a proposal, but is now invalid -- remove the vote
void CBudgetProposal::CleanAndRemove()
{
    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        if ((*it).second.SignatureValid())
        {
            (*it).second.fValid = true;
        } else {
            (*it).second.fValid = false;
        }
        ++it;
    }
}

double CBudgetProposal::GetRatio()
{
    int yeas = 0;
    int nays = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        if ((*it).second.nVote == VOTE_YES) yeas++;
        if ((*it).second.nVote == VOTE_NO) nays++;
        ++it;
    }

    if(yeas+nays == 0) return 0.0f;

    return ((double)(yeas) / (double)(yeas+nays));
}

int CBudgetProposal::GetYeas()
{
    int ret = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end()){
        if ((*it).second.nVote == VOTE_YES && (*it).second.fValid) ret++;
        ++it;
    }

    return ret;
}

int CBudgetProposal::GetNays()
{
    int ret = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end()){
        if ((*it).second.nVote == VOTE_NO && (*it).second.fValid) ret++;
        ++it;
    }

    return ret;
}

int CBudgetProposal::GetAbstains()
{
    int ret = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end()){
        if ((*it).second.nVote == VOTE_ABSTAIN && (*it).second.fValid) ret++;
        ++it;
    }

    return ret;
}

int CBudgetProposal::GetBlockStartCycle()
{
    //end block is half way through the next cycle (so the proposal will be removed much after the payment is sent)

    return (nBlockStart-(nBlockStart % GetBudgetPaymentCycleBlocks()));
}

int CBudgetProposal::GetBlockCurrentCycle()
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return -1;

    if(pindexPrev->nHeight >= GetBlockEndCycle()) return -1;

    return (pindexPrev->nHeight-(pindexPrev->nHeight % GetBudgetPaymentCycleBlocks()));
}

int CBudgetProposal::GetBlockEndCycle()
{
    //end block is half way through the next cycle (so the proposal will be removed much after the payment is sent)

    return nBlockEnd-(GetBudgetPaymentCycleBlocks()/2);
}

int CBudgetProposal::GetTotalPaymentCount()
{
    return (GetBlockEndCycle()-GetBlockStartCycle())/GetBudgetPaymentCycleBlocks();
}

int CBudgetProposal::GetRemainingPaymentCount()
{
    return (GetBlockEndCycle()-GetBlockCurrentCycle())/GetBudgetPaymentCycleBlocks();
}

CBudgetProposalBroadcast::CBudgetProposalBroadcast()
{
    strProposalName = "unknown";
    strURL = "";
    nBlockStart = 0;
    nBlockEnd = 0;
    nAmount = 0;
    nTime = 0;
    nFeeTXHash = 0;
}

CBudgetProposalBroadcast::CBudgetProposalBroadcast(const CBudgetProposal& other)
{
    strProposalName = other.strProposalName;
    strURL = other.strURL;
    nBlockStart = other.nBlockStart;
    nBlockEnd = other.nBlockEnd;
    address = other.address;
    nAmount = other.nAmount;
    nFeeTXHash = other.nFeeTXHash;
}

CBudgetProposalBroadcast::CBudgetProposalBroadcast(std::string strProposalNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn, uint256 nFeeTXHashIn)
{
    strProposalName = strProposalNameIn;
    strURL = strURLIn;

    nBlockStart = nBlockStartIn;

    int nCycleStart = (nBlockStart-(nBlockStart % GetBudgetPaymentCycleBlocks()));
    //calculate the end of the cycle for this vote, add half a cycle (vote will be deleted after that block)
    nBlockEnd = nCycleStart + (GetBudgetPaymentCycleBlocks()*nPaymentCount) + GetBudgetPaymentCycleBlocks()/2;

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
    nProposalHash = 0;
    nVote = VOTE_ABSTAIN;
    nTime = 0;
    fValid = true;
}

CBudgetVote::CBudgetVote(CTxIn vinIn, uint256 nProposalHashIn, int nVoteIn)
{
    vin = vinIn;
    nProposalHash = nProposalHashIn;
    nVote = nVoteIn;
    nTime = GetAdjustedTime();
    fValid = true;
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

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode))
        return(" Error upon calling SignMessage");

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage))
        return(" Error upon calling VerifyMessage");

    return true;
}

bool CBudgetVote::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nProposalHash.ToString() + boost::lexical_cast<std::string>(nVote) + boost::lexical_cast<std::string>(nTime);

    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn == NULL)
    {
        LogPrintf("CBudgetVote::SignatureValid() - Unknown Masternode - %s\n", vin.ToString().c_str());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("CBudgetVote::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

CFinalizedBudget::CFinalizedBudget()
{
    strBudgetName = "";
    nBlockStart = 0;
    vecProposals.clear();
    mapVotes.clear();
    nFeeTXHash = 0;
    fValid = true;
}

CFinalizedBudget::CFinalizedBudget(const CFinalizedBudget& other)
{
    strBudgetName = other.strBudgetName;
    nBlockStart = other.nBlockStart;
    vecProposals = other.vecProposals;
    mapVotes = other.mapVotes;
    nFeeTXHash = other.nFeeTXHash;
    fValid = true;
}

void CFinalizedBudget::AddOrUpdateVote(CFinalizedBudgetVote& vote)
{
    LOCK(cs);

    uint256 hash = vote.vin.prevout.GetHash();
    mapVotes[hash] = vote;
}

//evaluate if we should vote for this. Masternode only
void CFinalizedBudget::AutoCheck()
{
    if(!fMasterNode || fAutoChecked) return;

    if(Params().NetworkID() == CBaseChainParams::MAIN){
        if(rand() % 100 > 5) return; //do this 1 in 20 blocks -- spread out the voting activity on mainnet
    }
    fAutoChecked = true; //we only need to check this once

    if(strBudgetMode == "auto") //only vote for exact matches
    {
        std::vector<CBudgetProposal*> props1 = budget.GetBudget();

        if(props1.size() == 0) {
            LogPrintf("CFinalizedBudget::AutoCheck - Can't get Budget, aborting\n");
            return;
        }

        for(unsigned int i = 0; i < vecProposals.size(); i++){
            if(i > props1.size()-1) {
                LogPrintf("CFinalizedBudget::AutoCheck - Vector size mismatch, aborting\n");
                return;
            }

            if(vecProposals[i].nProposalHash != props1[i]->GetHash()){
                LogPrintf("CFinalizedBudget::AutoCheck - item #%d doesn't match %s %s\n", i, vecProposals[i].nProposalHash.ToString().c_str(), props1[i]->GetHash().ToString().c_str());
                return;
            }

            if(vecProposals[i].payee != props1[i]->GetPayee()){
                LogPrintf("CFinalizedBudget::AutoCheck - item #%d payee doesn't match %s %s\n", i, vecProposals[i].payee.ToString().c_str(), props1[i]->GetPayee().ToString().c_str());
                return;
            }

            if(vecProposals[i].nAmount != props1[i]->GetAmount()){
                LogPrintf("CFinalizedBudget::AutoCheck - item #%d payee doesn't match %s %s\n", i, vecProposals[i].payee.ToString().c_str(), props1[i]->GetPayee().ToString().c_str());
                return;
            }

            LogPrintf("CFinalizedBudget::AutoCheck - Finalized Budget Matches! Submitting Vote.\n");
            SubmitVote();
        }

    }
}
// If masternode voted for a proposal, but is now invalid -- remove the vote
void CFinalizedBudget::CleanAndRemove()
{
    std::map<uint256, CFinalizedBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        if ((*it).second.SignatureValid())
        {
            (*it).second.fValid = true;
        } else {
            (*it).second.fValid = false;
        }
        ++it;
    }
}


int64_t CFinalizedBudget::GetTotalPayout()
{
    int64_t ret = 0;

    for(unsigned int i = 0; i < vecProposals.size(); i++){
        ret += vecProposals[i].nAmount;
    }

    return ret;
}

std::string CFinalizedBudget::GetProposals() {
    std::string ret = "";

    BOOST_FOREACH(CTxBudgetPayment& payment, vecProposals){
        CBudgetProposal* pbudgetProposal = budget.FindProposal(payment.nProposalHash);

        std::string token = payment.nProposalHash.ToString();

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
        CTxBudgetPayment prop1;
        if(!GetProposalByBlock(nBlockHeight, prop1)){
            LogPrintf("CFinalizedBudget::GetStatus - Couldn't find budget payment for block %lld\n", nBlockHeight);
            continue;
        }

        CBudgetProposal* prop2 =  budget.FindProposal(prop1.nProposalHash);
        if(!prop2){
            if(retBadHashes == ""){
                retBadHashes = "Unknown proposal hash! Check this proposal before voting" + prop1.nProposalHash.ToString();
            } else {
                retBadHashes += "," + prop1.nProposalHash.ToString();
            }
        } else {
            if(prop2->GetPayee() != prop1.payee || prop2->GetAmount() != prop1.nAmount)
            {
                if(retBadPayeeOrAmount == ""){
                    retBadPayeeOrAmount = "Budget payee/nAmount doesn't match our proposal! " + prop1.nProposalHash.ToString();
                } else {
                    retBadPayeeOrAmount += "," + prop1.nProposalHash.ToString();
                }
            }
        }
    }

    if(retBadHashes == "" && retBadPayeeOrAmount == "") return "OK";

    return retBadHashes + retBadPayeeOrAmount;
}

bool CFinalizedBudget::IsValid()
{
    //must be the correct block for payment to happen (once a month)
    if(nBlockStart % GetBudgetPaymentCycleBlocks() != 0) return false;
    if(GetBlockEnd() - nBlockStart > 100) return false;
    if(vecProposals.size() > 100) return false;
    if(strBudgetName == "") return false;
    if(nBlockStart == 0) return false;
    if(nFeeTXHash == 0) return false;

    //can only pay out 10% of the possible coins (min value of coins)
    if(GetTotalPayout() > budget.GetTotalBudget(nBlockStart)) return false;

    std::string strError = "";
    if(!IsBudgetCollateralValid(nFeeTXHash, GetHash(), strError)){
        return false;
    }

    //TODO: if N cycles old, invalid, invalid

    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return true;

    if(nBlockStart < pindexPrev->nHeight) return false;
    if(GetBlockEnd() < pindexPrev->nHeight - GetBudgetPaymentCycleBlocks()/2 ) return false;

    return true;
}

bool CFinalizedBudget::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    // TODO:
    // BOOST_FOREACH(CMasternodePayee& payee, vecPayments)
    // {
    //     bool found = false;
    //     BOOST_FOREACH(CTxOut out, txNew.vout)
    //     {
    //         if(payee.scriptPubKey == out.scriptPubKey && payee.nValue == out.nValue)
    //             found = true;
    //     }

    //     if(payee.nVotes >= MNPAYMENTS_SIGNATURES_REQUIRED && !found){

    //         CTxDestination address1;
    //         ExtractDestination(payee.scriptPubKey, address1);
    //         CBitcoinAddress address2(address1);

    //         LogPrintf("CFinalizedBudget::IsTransactionValid - Missing required payment - %s:%d\n", address2.ToString().c_str(), payee.nValue);
    //         return false;
    //     }
    // }

    return true;
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

    CFinalizedBudgetVote vote(activeMasternode.vin, GetHash());
    if(!vote.Sign(keyMasternode, pubKeyMasternode)){
        LogPrintf("CFinalizedBudget::SubmitVote - Failure to sign.");
        return;
    }

    mapSeenFinalizedBudgetVotes.insert(make_pair(vote.GetHash(), vote));
    vote.Relay();
    budget.UpdateFinalizedBudget(vote, NULL);
}

CFinalizedBudgetBroadcast::CFinalizedBudgetBroadcast()
{
    strBudgetName = "";
    nBlockStart = 0;
    vecProposals.clear();
    mapVotes.clear();
    vchSig.clear();
    nFeeTXHash = 0;
}

CFinalizedBudgetBroadcast::CFinalizedBudgetBroadcast(const CFinalizedBudget& other)
{
    strBudgetName = other.strBudgetName;
    nBlockStart = other.nBlockStart;
    BOOST_FOREACH(CTxBudgetPayment out, other.vecProposals) vecProposals.push_back(out);
    mapVotes = other.mapVotes;
    nFeeTXHash = other.nFeeTXHash;
}

CFinalizedBudgetBroadcast::CFinalizedBudgetBroadcast(std::string strBudgetNameIn, int nBlockStartIn, std::vector<CTxBudgetPayment> vecProposalsIn, uint256 nFeeTXHashIn)
{
    strBudgetName = strBudgetNameIn;
    nBlockStart = nBlockStartIn;
    BOOST_FOREACH(CTxBudgetPayment out, vecProposalsIn) vecProposals.push_back(out);
    mapVotes.clear();
    nFeeTXHash = nFeeTXHashIn;
}

void CFinalizedBudgetBroadcast::Relay()
{
    CInv inv(MSG_BUDGET_FINALIZED, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

CFinalizedBudgetVote::CFinalizedBudgetVote()
{
    vin = CTxIn();
    nBudgetHash = 0;
    nTime = 0;
    vchSig.clear();
    fValid = true;
}

CFinalizedBudgetVote::CFinalizedBudgetVote(CTxIn vinIn, uint256 nBudgetHashIn)
{
    vin = vinIn;
    nBudgetHash = nBudgetHashIn;
    nTime = GetAdjustedTime();
    vchSig.clear();
    fValid = true;
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

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode))
        return(" Error upon calling SignMessage");

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage))
        return(" Error upon calling VerifyMessage");

    return true;
}

bool CFinalizedBudgetVote::SignatureValid()
{
    std::string errorMessage;

    std::string strMessage = vin.prevout.ToStringShort() + nBudgetHash.ToString() + boost::lexical_cast<std::string>(nTime);

    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn == NULL)
    {
        LogPrintf("CFinalizedBudgetVote::SignatureValid() - Unknown Masternode\n");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("CFinalizedBudgetVote::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

std::string CBudgetManager::ToString() const
{
    std::ostringstream info;

    info << "Proposals: " << (int)mapProposals.size() <<
            ", Budgets: " << (int)mapFinalizedBudgets.size();

    return info.str();
}


