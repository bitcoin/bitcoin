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

    LogPrintf("Written info to mncache.dat  %dms\n", GetTimeMillis() - nStart);
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
    LogPrintf("Loaded info from mncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", budgetToLoad.ToString());

    return Ok;
}

void DumpBudgets()
{
    int64_t nStart = GetTimeMillis();

    CBudgetDB mndb;
    CBudgetManager tempbudget;

    LogPrintf("Verifying mncache.dat format...\n");
    CBudgetDB::ReadResult readResult = mndb.Read(tempbudget);
    // there was an error and it was not an error on file openning => do not proceed
    if (readResult == CBudgetDB::FileError)
        LogPrintf("Missing masternode cache file - mncache.dat, will try to recreate\n");
    else if (readResult != CBudgetDB::Ok)
    {
        LogPrintf("Error reading mncache.dat: ");
        if(readResult == CBudgetDB::IncorrectFormat)
            LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
        else
        {
            LogPrintf("file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrintf("Writting info to mncache.dat...\n");
    mndb.Write(budget);

    LogPrintf("Masternode dump finished  %dms\n", GetTimeMillis() - nStart);
}



CBudgetVote::CBudgetVote()
{
    vin = CTxIn();
    nVote = VOTE_ABSTAIN;
    strProposalName = "unknown";
    nBlockStart = 0;
    nBlockEnd = 0;
    nAmount = 0;
    nTime = 0;
}

CBudgetVote::CBudgetVote(CTxIn vinIn, std::string strProposalNameIn, int nBlockStartIn, int nBlockEndIn, CScript addressIn, CAmount nAmountIn, int nVoteIn)
{
	vin = vinIn;
	strProposalName = strProposalNameIn;
	nBlockStart = nBlockStartIn;
	nBlockEnd = nBlockEndIn;
	address = addressIn;
	nAmount = nAmountIn;
	nVote = nVoteIn;
	nTime = GetAdjustedTime();
}

bool CBudgetVote::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    // Choose coins to use
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;

    std::string errorMessage;
    std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(nVote) + boost::lexical_cast<std::string>(nTime);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode))
        return(" Error upon calling SignMessage");

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage))
        return(" Error upon calling VerifyMessage");

    return true;
}

void CBudgetVote::Relay()
{
    CInv inv(MSG_MASTERNODE_VOTE, GetHash());

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}

uint256 GetBudgetProposalHash(std::string strProposalName)
{
	return Hash(strProposalName.begin(), strProposalName.end());
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

    if (strCommand == "mvote") { //Masternode Vote
        CBudgetVote vote;
        vRecv >> vote;
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

        if(pmn->nVotedTimes < 100){
        	budget.AddOrUpdateProposal(vote);
        	vote.Relay();
        	pmn->nVotedTimes++;
        } else {
        	LogPrintf("mvote - masternode can't vote again - vin:%s \n", pmn->vin.ToString().c_str());
        	return;
        }
    }

}

bool CBudgetVote::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + boost::lexical_cast<std::string>(nVote) + boost::lexical_cast<std::string>(nTime);

    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn == NULL)
    {
        LogPrintf("CBudgetVote::SignatureValid() - Unknown Masternode\n");
        return false;
    }

    CScript pubkey;
    pubkey = GetScriptForDestination(pmn->pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CBitcoinAddress address2(address1);

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("CBudgetVote::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

void CBudgetManager::AddOrUpdateProposal(CBudgetVote& vote)
{
    LOCK(cs);

    uint256 hash = GetBudgetProposalHash(vote.strProposalName);

    if(!mapProposals.count(hash)){
    	CBudgetProposal prop(vote.strProposalName);
    	//mapProposals.insert(make_pair(hash, prop));
    	return;
    }

  	mapProposals[hash].AddOrUpdateVote(vote);
}

void CBudgetProposal::AddOrUpdateVote(CBudgetVote& vote)
{
    LOCK(cs);

    uint256 hash = vote.vin.prevout.GetHash();

    if(!mapVotes.count(hash)){
        mapVotes.insert(make_pair(hash, vote));
        return;
    }

    mapVotes[hash] = vote;    
}

inline void CBudgetManager::NewBlock()
{
	//this function should be called 1/6 blocks, allowing up to 100 votes per day on all proposals
	if(chainActive.Height() % 6 != 0) return;

	mnodeman.DecrementVotedTimes();
}

CBudgetProposal *CBudgetManager::Find(const std::string &strProposalName)
{
    uint256 hash = GetBudgetProposalHash(strProposalName);

    if(mapProposals.count(hash)){
    	return &mapProposals[hash];
    }

    return NULL;
}

CBudgetProposal::CBudgetProposal()
{
    strProposalName = "";
}

CBudgetProposal::CBudgetProposal(const CBudgetProposal& other)
{
    strProposalName = other.strProposalName;
}

CBudgetProposal::CBudgetProposal(std::string strProposalNameIn)
{
	strProposalName = strProposalNameIn;
}

std::string CBudgetProposal::GetName()
{
	return strProposalName;
}

int CBudgetProposal::GetBlockStart()
{   
	std::map<int, int> mapList;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        if ((*it).second.nVote != VOTE_YES) continue;

        if(mapList.count((*it).second.nBlockStart)){
        	mapList[(*it).second.nBlockStart]++;
        } else {
            mapList[(*it).second.nBlockStart] = 1;
        }
    }

    //sort the map and grab the highest count item
    std::vector<std::pair<int,int> > vecList(mapList.begin(), mapList.end());
    std::sort(vecList.begin(),vecList.end());
	return vecList.begin()->second;
}

int CBudgetProposal::GetBlockEnd()
{
	std::map<int, int> mapList;
    
    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        if ((*it).second.nVote != VOTE_YES) continue;

        if(mapList.count((*it).second.nBlockEnd)){
            mapList[(*it).second.nBlockEnd]++;
        } else {
            mapList[(*it).second.nBlockEnd] = 1;
        }
    }

    //sort the map and grab the highest count item
    std::vector<std::pair<int,int> > vecList(mapList.begin(), mapList.end());
    std::sort(vecList.begin(),vecList.end());
    return vecList.begin()->second;
}

int64_t CBudgetProposal::GetAmount()
{
	std::map<int64_t, int> mapList;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        if ((*it).second.nVote != VOTE_YES) continue;

        if(mapList.count((*it).second.nAmount)){
            mapList[(*it).second.nAmount]++;
        } else {
            mapList[(*it).second.nAmount] = 1;
        }
    }

    //sort the map and grab the highest count item
    std::vector<std::pair<int64_t,int> > vecList(mapList.begin(), mapList.end());
    std::sort(vecList.begin(),vecList.end());
    return vecList.begin()->second;
}

CScript CBudgetProposal::GetPayee()
{
	std::map<CScript, int> mapList;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        if ((*it).second.nVote != VOTE_YES) continue;

        CTxDestination address1;
        ExtractDestination((*it).second.address, address1);
        CBitcoinAddress address2(address1);

        if(mapList.count((*it).second.address)){
            mapList[(*it).second.address]++;
        } else {
            mapList[(*it).second.address] = 1;
        }

    }
    
    //sort the map and grab the highest count item
    std::vector<std::pair<CScript,int> > vecList(mapList.begin(), mapList.end());
    std::sort(vecList.begin(),vecList.end());
    return vecList.begin()->second;
}

double CBudgetProposal::GetRatio()
{
	int yeas = 0;
	int nays = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        if ((*it).second.nVote == VOTE_YES) yeas++;
        if ((*it).second.nVote == VOTE_NO) nays++;
    }

    return ((double)yeas / (double)yeas+nays);
}

int CBudgetProposal::GetYeas()
{
	int ret = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end())
        if ((*it).second.nVote == VOTE_YES) ret++;

    return ret;
}

int CBudgetProposal::GetNays()
{
	int ret = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end())
        if ((*it).second.nVote == VOTE_NO) ret++;

    return ret;
}

int CBudgetProposal::GetAbstains()
{
	int ret = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end())
        if ((*it).second.nVote == VOTE_ABSTAIN) ret++;

    return ret;
}

bool CBudgetManager::IsBudgetPaymentBlock(int nHeight)
{
    if(nHeight > 158000+((576*30)*15)) return (nHeight % 7) == 0  ; // 417200 - 57.5% - 2016-02-08 -- 12.5% of blocks
    if(nHeight > 158000+((576*30)*13)) return (nHeight % 10) == 0 ; // 382640 - 55.0% - 2015-12-07  -- 10.0% of blocks
    if(nHeight > 158000+((576*30)*11)) return (nHeight % 13) == 0 ; // 348080 - 52.5% - 2015-10-05 -- 7.5 of blocks
    if(nHeight > 158000+((576*30)* 9)) return (nHeight % 20) == 0 ; // 313520 - 50.0% - 2015-08-03 -- 5.0% of blocks
    if(nHeight > 158000+((576*30)* 7)) return (nHeight % 40) == 0 ; // 278960 - 47.5% - 2015-06-01 -- 2.5% of blocks

    return false;
}

int64_t CBudgetManager::GetTotalBudget()
{
	if(chainActive.Tip() == NULL) return 0;

	const CBlockIndex* pindex = chainActive.Tip();
	return (GetBlockValue(pindex->pprev->nBits, pindex->pprev->nHeight, 0)/100)*15;
}

//Need to review this function
std::vector<CBudgetProposal*> CBudgetManager::GetBudget()
{
    // ------- Sort budgets by Yes Count

	std::map<uint256, int> mapList;

    std::map<uint256, CBudgetProposal>::iterator it = mapProposals.begin();
    while(it != mapProposals.end())
        mapList[GetBudgetProposalHash((*it).second.strProposalName)] = (*it).second.GetYeas();

    //sort the map and grab the highest count item
    std::vector<std::pair<uint256,int> > vecList(mapList.begin(), mapList.end());
    std::sort(vecList.begin(),vecList.end());

    // ------- Grab The Budgets In Order
    
	std::vector<CBudgetProposal*> ret;
    
	int64_t nBudgetAllocated = 0;
	int64_t nTotalBudget = GetTotalBudget();

    std::map<uint256, CBudgetProposal>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end())
    {
        CBudgetProposal prop = (*it2).second;

        if(nTotalBudget == nBudgetAllocated){
            prop.SetAllotted(0);
        } else if(prop.GetAmount() + nBudgetAllocated <= nTotalBudget) {
            prop.SetAllotted(prop.GetAmount());
            nBudgetAllocated += prop.GetAmount();
        } else {
            //couldn't pay for the entire budget, so it'll be partially paid.
            prop.SetAllotted(nTotalBudget - nBudgetAllocated);
            nBudgetAllocated = nTotalBudget;
        }

    	ret.push_back(&prop);
    }

	return ret;
}


void CBudgetManager::Sync(CNode* node)
{
    std::map<uint256, CBudgetProposal>::iterator it = mapProposals.begin();
    while(it != mapProposals.end())
     	(*it).second.Sync(node);

}

void CBudgetProposal::Sync(CNode* node)
{
    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end())
        node->PushMessage("mvote", (*it).second);    
}