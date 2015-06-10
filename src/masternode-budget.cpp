// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "masternode-budget.h"
#include "masternode.h"
#include "darksend.h"
#include "masternodeman.h"
#include "util.h"
#include "addrman.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

CBudgetManager budget;
CCriticalSection cs_budget;

std::map<uint256, CBudgetProposalBroadcast> mapSeenMasternodeBudgetProposals;
std::map<uint256, CBudgetVote> mapSeenMasternodeBudgetVotes;
std::map<uint256, CFinalizedBudgetBroadcast> mapSeenFinalizedBudgets;
std::map<uint256, CFinalizedBudgetVote> mapSeenFinalizedBudgetVotes;
int nSubmittedFinalBudget;

int GetBudgetPaymentCycleBlocks(){
    if(Params().NetworkID() == CBaseChainParams::MAIN) return 16616; //(60*24*30)/2.6

    //for testing purposes
    return 50;
}

void SubmitFinalBudget()
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;

    int nBlockStart = pindexPrev->nHeight-(pindexPrev->nHeight % GetBudgetPaymentCycleBlocks())+GetBudgetPaymentCycleBlocks();
    if(nSubmittedFinalBudget >= nBlockStart) return;
    if(nBlockStart - pindexPrev->nHeight > 100) return;

    std::vector<CBudgetProposal*> props1 = budget.GetBudget();

    std::string strBudgetName = "main";
    std::vector<CTxBudgetPayment> vecPayments;

    for(unsigned int i = 0; i < props1.size(); i++){
        CTxBudgetPayment out;
        out.nProposalHash = props1[i]->GetHash();
        out.payee = props1[i]->GetPayee();
        out.nAmount = props1[i]->GetAmount();
        vecPayments.push_back(out);
    }

    if(vecPayments.size() < 1) return;
    nSubmittedFinalBudget = nBlockStart;

    CPubKey pubKeyMasternode;
    CKey keyMasternode;
    std::string errorMessage;

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode)){
        LogPrintf("SubmitFinalBudget - Error upon calling SetKey");
    }

    //create the proposal incase we're the first to make it
    CFinalizedBudgetBroadcast prop(activeMasternode.vin, strBudgetName, nBlockStart, vecPayments);
    if(!prop.Sign(keyMasternode, pubKeyMasternode)){
        LogPrintf("SubmitFinalBudget - Failure to sign.");
    }

    if(!prop.IsValid()){
        LogPrintf("SubmitFinalBudget - Invalid prop (are all the hashes correct?)");
    }

    mapSeenFinalizedBudgets.insert(make_pair(prop.GetHash(), prop));
    prop.Relay();
    budget.AddFinalizedBudget(prop);


    CFinalizedBudgetVote vote(activeMasternode.vin, prop.GetHash());
    if(!vote.Sign(keyMasternode, pubKeyMasternode)){
        LogPrintf("SubmitFinalBudget - Failure to sign.");
    }

    mapSeenFinalizedBudgetVotes.insert(make_pair(vote.GetHash(), vote));
    vote.Relay();
    budget.UpdateFinalizedBudget(vote);
}

//
// CBudgetDB
//

CBudgetDB::CBudgetDB()
{
    pathDB = GetDataDir() / "budget.dat";
    strMagicMessage = "MasternodeBudget";
}

bool CBudgetDB::Write(const CBudgetManager& budgetToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssBudget(SER_DISK, CLIENT_VERSION);
    ssBudget << strMagicMessage; // masternode cache file specific magic message
    ssBudget << FLATDATA(Params().MessageStart()); // network specific magic number
    ssBudget << budgetToSave;
    uint256 hash = Hash(ssBudget.begin(), ssBudget.end());
    ssBudget << hash;

    // open output file, and associate with CAutoFile
    FILE *file = fopen(pathDB.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s : Failed to open file %s", __func__, pathDB.string());

    // Write and commit header, data
    try {
        fileout << ssBudget;
    }
    catch (std::exception &e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
//    FileCommit(fileout);
    fileout.fclose();

    LogPrintf("Written info to budget.dat  %dms\n", GetTimeMillis() - nStart);
    //LogPrintf("  %s\n", budgetToSave.ToString().c_str());

    return true;
}

CBudgetDB::ReadResult CBudgetDB::Read(CBudgetManager& budgetToLoad)
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

    CDataStream ssBudget(vchData, SER_DISK, CLIENT_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssBudget.begin(), ssBudget.end());
    if (hashIn != hashTmp)
    {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }

   
    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header (masternode cache file specific magic message) and ..
        ssBudget >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp)
        {
            error("%s : Invalid masternode cache magic message", __func__);
            return IncorrectMagicMessage;
        }


        // de-serialize file header (network specific magic number) and ..
        ssBudget >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
        {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }

        // de-serialize data into CBudgetManager object
        ssBudget >> budgetToLoad;
    }
    catch (std::exception &e) {
        budgetToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }


    budgetToLoad.CheckAndRemove(); // clean out expired
    LogPrintf("Loaded info from budget.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", budgetToLoad.ToString());

    return Ok;
}

void DumpBudgets()
{
    int64_t nStart = GetTimeMillis();

    CBudgetDB mndb;
    CBudgetManager tempbudget;

    LogPrintf("Verifying budget.dat format...\n");
    CBudgetDB::ReadResult readResult = mndb.Read(tempbudget);
    // there was an error and it was not an error on file openning => do not proceed
    if (readResult == CBudgetDB::FileError)
        LogPrintf("Missing budget cache file - budget.dat, will try to recreate\n");
    else if (readResult != CBudgetDB::Ok)
    {
        LogPrintf("Error reading budget.dat: ");
        if(readResult == CBudgetDB::IncorrectFormat)
            LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
        else
        {
            LogPrintf("file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrintf("Writting info to budget.dat...\n");
    mndb.Write(budget);

    LogPrintf("Masternode dump finished  %dms\n", GetTimeMillis() - nStart);
}

void CBudgetManager::AddFinalizedBudget(CFinalizedBudget& prop)
{
    LOCK(cs);
    if(mapFinalizedBudgets.count(prop.GetHash())) return;

    mapFinalizedBudgets.insert(make_pair(prop.GetHash(), prop));
}

void CBudgetManager::AddProposal(CBudgetProposal& prop)
{
    LOCK(cs);
    if(mapProposals.count(prop.GetHash())) return;

    mapProposals.insert(make_pair(prop.GetHash(), prop));
}

void CBudgetManager::CheckAndRemove()
{
    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* prop = &((*it).second);
        if(!prop->IsValid()){
            mapFinalizedBudgets.erase(it++);
        } else {
            prop->AutoCheck();
            ++it;
        }
    }

    std::map<uint256, CBudgetProposal>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end())
    {
        CBudgetProposal* prop = &((*it2).second);
        if(!prop->IsValid()){
            mapProposals.erase(it2++);
        } else {
            ++it2;
        }
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
        CFinalizedBudget* prop = &((*it).second);
        if(prop->GetVoteCount() > nHighestCount){
            if(pindexPrev->nHeight+1 >= prop->GetBlockStart() && pindexPrev->nHeight+1 <= prop->GetBlockEnd()){
                if(prop->GetPayeeAndAmount(pindexPrev->nHeight+1, payee, nAmount)){
                    nHighestCount = prop->GetVoteCount();
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

    int nYesCount = 0;
    CBudgetProposal* prop = NULL;

    std::map<uint256, CBudgetProposal>::iterator it = mapProposals.begin();
    while(it != mapProposals.end()){
        if((*it).second.strProposalName == strProposalName && (*it).second.GetYeas() > nYesCount){
            prop = &((*it).second);
            nYesCount = prop->GetYeas();
        }
        ++it;
    }

    if(nYesCount == 0) return NULL;

    return prop;
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
        CFinalizedBudget* prop = &((*it).second);
        if(nBlockHeight >= prop->GetBlockStart() && nBlockHeight <= prop->GetBlockEnd()){
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
        CFinalizedBudget* prop = &((*it).second);
        if(prop->GetVoteCount() > nHighestCount){
            if(nBlockHeight >= prop->GetBlockStart() && nBlockHeight <= prop->GetBlockEnd()){
                nHighestCount = prop->GetVoteCount();
            }
        }

        it++;
    }

    if(nHighestCount < mnodeman.CountEnabled()/20) return true;

    // check the highest finalized budgets (+/- 10% to assist in consensus)

    std::map<uint256, CFinalizedBudget>::iterator it2 = mapFinalizedBudgets.begin();
    while(it2 != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* prop = &((*it2).second);

        if(prop->GetVoteCount() > nHighestCount-(mnodeman.CountEnabled()/10)){
            if(nBlockHeight >= prop->GetBlockStart() && nBlockHeight <= prop->GetBlockEnd()){
                if(prop->IsTransactionValid(txNew, nBlockHeight)){
                    return true;
                }
            }
        }

        it2++;
    }

    //we looked through all of the known budgets
    return false;
}

//Need to review this function
std::vector<CBudgetProposal*> CBudgetManager::GetBudget()
{
    // ------- Sort budgets by Yes Count

    std::map<uint256, int> mapList;

    std::map<uint256, CBudgetProposal>::iterator it = mapProposals.begin();
    while(it != mapProposals.end()){
        mapList.insert(make_pair((*it).second.GetHash(), (*it).second.GetYeas()));
        ++it;
    }

    //sort the map and grab the highest count item
    std::vector<std::pair<uint256,int> > vecList(mapList.begin(), mapList.end());
    std::sort(vecList.begin(),vecList.end());

    // ------- Grab The Budgets In Order

    std::vector<CBudgetProposal*> ret;

    int64_t nBudgetAllocated = 0;
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return ret;

    int nBlockStart = pindexPrev->nHeight-(pindexPrev->nHeight % GetBudgetPaymentCycleBlocks())+GetBudgetPaymentCycleBlocks();
    int64_t nTotalBudget = GetTotalBudget(nBlockStart);


    std::map<uint256, CBudgetProposal>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end())
    {
        CBudgetProposal* prop = &((*it2).second);

        if(nTotalBudget == nBudgetAllocated){
            prop->SetAllotted(0);
        } else if(prop->GetAmount() + nBudgetAllocated <= nTotalBudget) {
            prop->SetAllotted(prop->GetAmount());
            nBudgetAllocated += prop->GetAmount();
        } else {
            //couldn't pay for the entire budget, so it'll be partially paid.
            prop->SetAllotted(nTotalBudget - nBudgetAllocated);
            nBudgetAllocated = nTotalBudget;
        }

        ret.push_back(prop);
        it2++;
    }

    return ret;
}

std::vector<CFinalizedBudget*> CBudgetManager::GetFinalizedBudgets()
{
    std::vector<CFinalizedBudget*> ret;

    // ------- Grab The Budgets In Order

    std::map<uint256, CFinalizedBudget>::iterator it2 = mapFinalizedBudgets.begin();
    while(it2 != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* prop = &((*it2).second);

        ret.push_back(prop);
        it2++;
    }

    return ret;
}

std::string CBudgetManager::GetRequiredPaymentsString(int64_t nBlockHeight)
{
    std::string ret = "unknown-budget";

    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* prop = &((*it).second);
        if(nBlockHeight >= prop->GetBlockStart() && nBlockHeight <= prop->GetBlockEnd()){
            CTxBudgetPayment payment;
            if(prop->GetProposalByBlock(nBlockHeight, payment)){
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
}

void CBudgetManager::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    // lite mode is not supported
    if(IsInitialBlockDownload()) return;

    LOCK(cs_budget);

    if (strCommand == "mnvs") { //Masternode vote sync
        if(pfrom->HasFulfilledRequest("mnvs")) {
            LogPrintf("mnvs - peer already asked me for the list\n");
            Misbehaving(pfrom->GetId(), 20);
            return;
        }

        pfrom->FulfilledRequest("mnvs");
        budget.Sync(pfrom);
        LogPrintf("mnvs - Sent Masternode votes to %s\n", pfrom->addr.ToString().c_str());
    }

    if (strCommand == "mprop") { //Masternode Proposal
        CBudgetProposalBroadcast prop;
        vRecv >> prop;

        if(mapSeenMasternodeBudgetProposals.count(prop.GetHash())){
            return;
        }

        if(!prop.SignatureValid()){
            LogPrintf("mprop - signature invalid\n");
            Misbehaving(pfrom->GetId(), 20);
            return;
        }

        if(!prop.IsValid()) {
            LogPrintf("mprop - invalid prop\n");
            return;
        }

        CMasternode* pmn = mnodeman.Find(prop.vin);
        if(pmn == NULL) {
            LogPrintf("mprop - unknown masternode - vin:%s \n", pmn->vin.ToString().c_str());
            return;
        }

        mapSeenMasternodeBudgetProposals.insert(make_pair(prop.GetHash(), prop));
        if(IsSyncingMasternodeAssets() || pmn->nVotedTimes < 100){
            CBudgetProposal p(prop);
            budget.AddProposal(p);
            prop.Relay();

            if(!IsSyncingMasternodeAssets()) pmn->nVotedTimes++;
        } else {
            LogPrintf("mvote - masternode can't vote again - vin:%s \n", pmn->vin.ToString().c_str());
            return;
        }
    }

    if (strCommand == "mvote") { //Masternode Vote
        CBudgetVote vote;
        vRecv >> vote;

        if(mapSeenMasternodeBudgetVotes.count(vote.GetHash())){
            return;
        }

        if(!vote.SignatureValid()){
            LogPrintf("mvote - signature invalid\n");
            Misbehaving(pfrom->GetId(), 20);
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vin);
        if(pmn == NULL) {
            LogPrintf("mvote - unknown masternode - vin:%s \n", pmn->vin.ToString().c_str());
            return;
        }

        mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        if(IsSyncingMasternodeAssets() || pmn->nVotedTimes < 100){
            budget.UpdateProposal(vote);
            vote.Relay();
            if(!IsSyncingMasternodeAssets()) pmn->nVotedTimes++;
        } else {
            LogPrintf("mvote - masternode can't vote again - vin:%s \n", pmn->vin.ToString().c_str());
            return;
        }
    }

    if (strCommand == "fbs") { //Finalized Budget Suggestion
        CFinalizedBudgetBroadcast prop;
        vRecv >> prop;

        if(mapSeenFinalizedBudgets.count(prop.GetHash())){
            return;
        }

        if(!prop.SignatureValid()){
            LogPrintf("fbs - signature invalid\n");
            Misbehaving(pfrom->GetId(), 20);
            return;
        }

        if(!prop.IsValid()) {
            LogPrintf("fbs - invalid prop\n");
            return;
        }

        CMasternode* pmn = mnodeman.Find(prop.vin);
        if(pmn == NULL) {
            LogPrintf("fbs - unknown masternode - vin:%s \n", pmn->vin.ToString().c_str());
            return;
        }

        mapSeenFinalizedBudgets.insert(make_pair(prop.GetHash(), prop));
        if(IsSyncingMasternodeAssets() || pmn->nVotedTimes < 100){
            CFinalizedBudget p(prop);
            budget.AddFinalizedBudget(p);
            prop.Relay();

            if(!IsSyncingMasternodeAssets()) pmn->nVotedTimes++;
        } else {
            LogPrintf("mvote - masternode can't vote again - vin:%s \n", pmn->vin.ToString().c_str());
            return;
        }
    }

    if (strCommand == "fbvote") { //Finalized Budget Vote
        CFinalizedBudgetVote vote;
        vRecv >> vote;

        if(mapSeenFinalizedBudgetVotes.count(vote.GetHash())){
            return;
        }

        if(!vote.SignatureValid()){
            LogPrintf("fbvote - signature invalid\n");
            Misbehaving(pfrom->GetId(), 20);
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vin);
        if(pmn == NULL) {
            LogPrintf("fbvote - unknown masternode - vin:%s \n", pmn->vin.ToString().c_str());
            return;
        }

        mapSeenFinalizedBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        if(IsSyncingMasternodeAssets() || pmn->nVotedTimes < 100){
            budget.UpdateFinalizedBudget(vote);
            vote.Relay();
            if(!IsSyncingMasternodeAssets()) pmn->nVotedTimes++;
        } else {
            LogPrintf("fbvote - masternode can't vote again - vin:%s \n", pmn->vin.ToString().c_str());
            return;
        }
    }
}

bool CBudgetManager::PropExists(uint256 nHash)
{
    if(mapProposals.count(nHash)) return true;
    return false;
}

void CBudgetManager::Sync(CNode* node)
{
    std::map<uint256, CBudgetProposal>::iterator it = mapProposals.begin();
    while(it != mapProposals.end()){
        (*it).second.Sync(node);
        ++it;
    }

}

void CBudgetManager::UpdateProposal(CBudgetVote& vote)
{
    LOCK(cs);
    if(!mapProposals.count(vote.nProposalHash)){
        LogPrintf("ERROR : Unknown proposal %d\n", vote.nProposalHash.ToString().c_str());
        return;
    }

    mapProposals[vote.nProposalHash].AddOrUpdateVote(vote);
}

void CBudgetManager::UpdateFinalizedBudget(CFinalizedBudgetVote& vote)
{
    LOCK(cs);

    if(!mapFinalizedBudgets.count(vote.nBudgetHash)){
        LogPrintf("ERROR: Unknown Finalized Proposal %s\n", vote.nBudgetHash.ToString().c_str());
        //should ask for it
        return;
    }

    mapFinalizedBudgets[vote.nBudgetHash].AddOrUpdateVote(vote);
}

CBudgetProposal::CBudgetProposal()
{
    vin = CTxIn();
    strProposalName = "unknown";
    nBlockStart = 0;
    nBlockEnd = 0;
    nAmount = 0;
}

CBudgetProposal::CBudgetProposal(CTxIn vinIn, std::string strProposalNameIn, std::string strURLIn, int nBlockStartIn, int nBlockEndIn, CScript addressIn, CAmount nAmountIn)
{
	vin = vinIn;
    strProposalName = strProposalNameIn;
    strURL = strURLIn;
	nBlockStart = nBlockStartIn;
	nBlockEnd = nBlockEndIn;
	address = addressIn;
	nAmount = nAmountIn;
}

CBudgetProposal::CBudgetProposal(const CBudgetProposal& other)
{
    vin = other.vin;
    strProposalName = other.strProposalName;
    strURL = other.strURL;
    nBlockStart = other.nBlockStart;
    nBlockEnd = other.nBlockEnd;
    address = other.address;
    nAmount = other.nAmount;
}

bool CBudgetProposal::IsValid()
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return true;

    //TODO: if < 20 votes and 2+ weeks old, invalid

    if(pindexPrev->nHeight < nBlockStart || pindexPrev->nHeight > nBlockEnd) return false;

    if(nBlockEnd - nBlockStart <= GetBudgetPaymentCycleBlocks()) return false;

    //can only pay out 10% of the possible coins (min value of coins)
    if(nAmount > budget.GetTotalBudget(nBlockStart)) {
        return false;
    }

    return true;
}

void CBudgetProposal::AddOrUpdateVote(CBudgetVote& vote)
{
    LOCK(cs);

    uint256 hash = vote.vin.prevout.GetHash();
    mapVotes[hash] = vote;
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
        if ((*it).second.nVote == VOTE_YES) ret++;
        ++it;
    }

    return ret;
}

int CBudgetProposal::GetNays()
{
    int ret = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end()){
        if ((*it).second.nVote == VOTE_NO) ret++;
        ++it;
    }

    return ret;
}

int CBudgetProposal::GetAbstains()
{
    int ret = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end()){
        if ((*it).second.nVote == VOTE_ABSTAIN) ret++;
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

void CBudgetProposal::Sync(CNode* node)
{
    //send the proposal
    node->PushMessage("mprop", (*this));

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end()){
        node->PushMessage("mvote", (*it).second);
        ++it;
    }
}

CBudgetProposalBroadcast::CBudgetProposalBroadcast()
{
    vin = CTxIn();
    strProposalName = "unknown";
    nBlockStart = 0;
    nBlockEnd = 0;
    nAmount = 0;
}

CBudgetProposalBroadcast::CBudgetProposalBroadcast(const CBudgetProposal& other)
{
    vin = other.vin;
    strProposalName = other.strProposalName;
    nBlockStart = other.nBlockStart;
    nBlockEnd = other.nBlockEnd;
    address = other.address;
    nAmount = other.nAmount;
}

CBudgetProposalBroadcast::CBudgetProposalBroadcast(CTxIn vinIn, std::string strProposalNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn)
{
    vin = vinIn;
    strProposalName = strProposalNameIn;
    strURL = strURLIn;

    nBlockStart = nBlockStartIn;

    int nCycleStart = (nBlockStart-(nBlockStart % GetBudgetPaymentCycleBlocks()));
    //calculate the end of the cycle for this vote, add half a cycle (vote will be deleted after that block)
    nBlockEnd = nCycleStart + (GetBudgetPaymentCycleBlocks()*nPaymentCount) + GetBudgetPaymentCycleBlocks()/2;

    address = addressIn;
    nAmount = nAmountIn;
}

bool CBudgetProposalBroadcast::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    // Choose coins to use
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;

    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + strProposalName + strURL +  boost::lexical_cast<std::string>(nBlockStart) +  
        boost::lexical_cast<std::string>(nBlockEnd) + address.ToString() + boost::lexical_cast<std::string>(nAmount);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode))
        return(" Error upon calling SignMessage");

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage))
        return(" Error upon calling VerifyMessage");

    return true;
}

void CBudgetProposalBroadcast::Relay()
{
    CInv inv(MSG_BUDGET_PROPOSAL, GetHash());
    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}

bool CBudgetProposalBroadcast::SignatureValid()
{
    std::string errorMessage;

    std::string strMessage = vin.prevout.ToStringShort() + strProposalName + strURL +  boost::lexical_cast<std::string>(nBlockStart) +  
        boost::lexical_cast<std::string>(nBlockEnd) + address.ToString() + boost::lexical_cast<std::string>(nAmount);

    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn == NULL)
    {
        LogPrintf("CBudgetProposalBroadcast::SignatureValid() - Unknown Masternode\n");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("CBudgetProposalBroadcast::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

CBudgetVote::CBudgetVote()
{
    vin = CTxIn();
    nProposalHash = 0;
    nVote = VOTE_ABSTAIN;
    nTime = 0;
}

CBudgetVote::CBudgetVote(CTxIn vinIn, uint256 nProposalHashIn, int nVoteIn)
{
    vin = vinIn;
    nProposalHash = nProposalHashIn;
    nVote = nVoteIn;
    nTime = GetAdjustedTime();
}

void CBudgetVote::Relay()
{
    CInv inv(MSG_BUDGET_VOTE, GetHash());
    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
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
        LogPrintf("CBudgetVote::SignatureValid() - Unknown Masternode\n");
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
    vin = CTxIn();
    strBudgetName = "";
    nBlockStart = 0;
    vecProposals.clear();
    mapVotes.clear();
}


CFinalizedBudget::CFinalizedBudget(const CFinalizedBudget& other)
{
    vin = other.vin;
    strBudgetName = other.strBudgetName;
    nBlockStart = other.nBlockStart;
    vecProposals = other.vecProposals;
    mapVotes = other.mapVotes;
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
    // Feature : Masternodes can delegate finalized budgets to a 3rd party simply by adding this option to the configuration
    else if (strBudgetMode == vin.prevout.ToStringShort())
    {
        SubmitVote();
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
    std::string ret = "aeu";

    BOOST_FOREACH(CTxBudgetPayment& payment, vecProposals){
        CFinalizedBudget* prop = budget.FindFinalizedBudget(payment.nProposalHash);

        std::string token = payment.nProposalHash.ToString();

        if(prop) token = prop->GetName();
        if(ret == "") {ret = token;}
        else {ret = "," + token;}
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

    //can only pay out 10% of the possible coins (min value of coins)
    if(GetTotalPayout() > budget.GetTotalBudget(nBlockStart)) return false;

    //TODO: if N cycles old, invalid, invalid

    return true;
}

bool CFinalizedBudget::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
   /* BOOST_FOREACH(CMasternodePayee& payee, vecPayments)
    {
        bool found = false;
        BOOST_FOREACH(CTxOut out, txNew.vout)
        {
            if(payee.scriptPubKey == out.scriptPubKey && payee.nValue == out.nValue) 
                found = true;
        }

        if(payee.nVotes >= MNPAYMENTS_SIGNATURES_REQUIRED && !found){

            CTxDestination address1;
            ExtractDestination(payee.scriptPubKey, address1);
            CBitcoinAddress address2(address1);

            LogPrintf("CMasternodePayments::IsTransactionValid - Missing required payment - %s:%d\n", address2.ToString().c_str(), payee.nValue);
            return false;
        }
    }*/

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
    budget.UpdateFinalizedBudget(vote);
}

CFinalizedBudgetBroadcast::CFinalizedBudgetBroadcast()
{
    vin = CTxIn();
    strBudgetName = "";
    nBlockStart = 0;
    vecProposals.clear();
    mapVotes.clear();
    vchSig.clear();
}

CFinalizedBudgetBroadcast::CFinalizedBudgetBroadcast(const CFinalizedBudget& other)
{
    vin = other.vin;
    strBudgetName = other.strBudgetName;
    nBlockStart = other.nBlockStart;
    BOOST_FOREACH(CTxBudgetPayment out, other.vecProposals) vecProposals.push_back(out);
    mapVotes = other.mapVotes;
}

CFinalizedBudgetBroadcast::CFinalizedBudgetBroadcast(CTxIn& vinIn, std::string strBudgetNameIn, int nBlockStartIn, std::vector<CTxBudgetPayment> vecProposalsIn)
{
    vin = vinIn;
    printf("%s\n", vin.ToString().c_str());
    strBudgetName = strBudgetNameIn;
    nBlockStart = nBlockStartIn;
    BOOST_FOREACH(CTxBudgetPayment out, vecProposalsIn) vecProposals.push_back(out);
    mapVotes.clear();
}

void CFinalizedBudgetBroadcast::Relay()
{
    CInv inv(MSG_BUDGET_FINALIZED, GetHash());
    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}

bool CFinalizedBudgetBroadcast::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    // Choose coins to use
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;

    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + strBudgetName + boost::lexical_cast<std::string>(nBlockStart);
    BOOST_FOREACH(CTxBudgetPayment& payment, vecProposals) strMessage += payment.nProposalHash.ToString().c_str();

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode))
        return(" Error upon calling SignMessage");

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage))
        return(" Error upon calling VerifyMessage");

    return true;
}

bool CFinalizedBudgetBroadcast::SignatureValid()
{
    std::string errorMessage;

    std::string strMessage = vin.prevout.ToStringShort() + strBudgetName + boost::lexical_cast<std::string>(nBlockStart);
    BOOST_FOREACH(CTxBudgetPayment& payment, vecProposals) strMessage += payment.nProposalHash.ToString().c_str();

    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn == NULL)
    {
        LogPrintf("CFinalizedBudgetBroadcast::SignatureValid() - Unknown Masternode\n");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("CFinalizedBudgetBroadcast::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

CFinalizedBudgetVote::CFinalizedBudgetVote()
{
    vin = CTxIn();
    nBudgetHash = 0;
    nTime = 0;
    vchSig.clear();
}

CFinalizedBudgetVote::CFinalizedBudgetVote(CTxIn vinIn, uint256 nBudgetHashIn)
{
    vin = vinIn;
    nBudgetHash = nBudgetHashIn;
    nTime = GetAdjustedTime();
    vchSig.clear();
}

void CFinalizedBudgetVote::Relay()
{
    CInv inv(MSG_BUDGET_FINALIZED_VOTE, GetHash());
    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
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
