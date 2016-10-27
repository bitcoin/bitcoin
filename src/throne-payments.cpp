// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "masternode-payments.h"
#include "masternode-budget.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "darksend.h"
#include "util.h"
#include "sync.h"
#include "spork.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

/** Object for who's going to get paid on which blocks */
CThronePayments masternodePayments;

CCriticalSection cs_vecPayments;
CCriticalSection cs_mapThroneBlocks;
CCriticalSection cs_mapThronePayeeVotes;

//
// CThronePaymentDB
//

CThronePaymentDB::CThronePaymentDB()
{
    pathDB = GetDataDir() / "mnpayments.dat";
    strMagicMessage = "ThronePayments";
}

bool CThronePaymentDB::Write(const CThronePayments& objToSave)
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

    LogPrintf("Written info to mnpayments.dat  %dms\n", GetTimeMillis() - nStart);

    return true;
}

CThronePaymentDB::ReadResult CThronePaymentDB::Read(CThronePayments& objToLoad, bool fDryRun)
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
            error("%s : Invalid masternode payement cache magic message", __func__);
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

        // de-serialize data into CThronePayments object
        ssObj >> objToLoad;
    }
    catch (std::exception &e) {
        objToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    LogPrintf("Loaded info from mnpayments.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", objToLoad.ToString());
    if(!fDryRun) {
        LogPrintf("Throne payments manager - cleaning....\n");
        objToLoad.CleanPaymentList();
        LogPrintf("Throne payments manager - result:\n");
        LogPrintf("  %s\n", objToLoad.ToString());
    }

    return Ok;
}

void DumpThronePayments()
{
    int64_t nStart = GetTimeMillis();

    CThronePaymentDB paymentdb;
    CThronePayments tempPayments;

    LogPrintf("Verifying mnpayments.dat format...\n");
    CThronePaymentDB::ReadResult readResult = paymentdb.Read(tempPayments, true);
    // there was an error and it was not an error on file opening => do not proceed
    if (readResult == CThronePaymentDB::FileError)
        LogPrintf("Missing budgets file - mnpayments.dat, will try to recreate\n");
    else if (readResult != CThronePaymentDB::Ok)
    {
        LogPrintf("Error reading mnpayments.dat: ");
        if(readResult == CThronePaymentDB::IncorrectFormat)
            LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
        else
        {
            LogPrintf("file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrintf("Writting info to mnpayments.dat...\n");
    paymentdb.Write(masternodePayments);

    LogPrintf("Budget dump finished  %dms\n", GetTimeMillis() - nStart);
}

bool IsBlockValueValid(const CBlock& block, int64_t nExpectedValue){
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return true;

    int nHeight = 0;
    if(pindexPrev->GetBlockHash() == block.hashPrevBlock)
    {
        nHeight = pindexPrev->nHeight+1;
    } else { //out of order
        BlockMap::iterator mi = mapBlockIndex.find(block.hashPrevBlock);
        if (mi != mapBlockIndex.end() && (*mi).second)
            nHeight = (*mi).second->nHeight+1;
    }

    if(nHeight == 0){
        LogPrintf("IsBlockValueValid() : WARNING: Couldn't find previous block");
    }

    if(!masternodeSync.IsSynced()) { //there is no budget data to use to check anything
        //super blocks will always be on these blocks, max 100 per budgeting
        if(nHeight % GetBudgetPaymentCycleBlocks() < 100){
            return true;
        } else {
            if(block.vtx[0].GetValueOut() > nExpectedValue) return false;
        }
    } else { // we're synced and have data so check the budget schedule

        //are these blocks even enabled
        if(!IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS)){
            return block.vtx[0].GetValueOut() <= nExpectedValue;
        }
        
        if(budget.IsBudgetPaymentBlock(nHeight)){
            //the value of the block is evaluated in CheckBlock
            return true;
        } else {
            if(block.vtx[0].GetValueOut() > nExpectedValue) return false;
        }
    }

    return true;
}

bool IsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight)
{
    if(!masternodeSync.IsSynced()) { //there is no budget data to use to check anything -- find the longest chain
        LogPrint("mnpayments", "Client not synced, skipping block payee checks\n");
        return true;
    }

    //check if it's a budget block
    if(IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS)){
        if(budget.IsBudgetPaymentBlock(nBlockHeight)){
            if(budget.IsTransactionValid(txNew, nBlockHeight)){
                return true;
            } else {
                LogPrintf("Invalid budget payment detected %s\n", txNew.ToString().c_str());
                if(IsSporkActive(SPORK_9_THRONE_BUDGET_ENFORCEMENT)){
                    return false;
                } else {
                    LogPrintf("Budget enforcement is disabled, accepting block\n");
                    return true;
                }
            }
        }
    }

    //check for masternode payee
    if(masternodePayments.IsTransactionValid(txNew, nBlockHeight))
    {
        return true;
    } else {
        LogPrintf("Invalid mn payment detected %s\n", txNew.ToString().c_str());
        if(IsSporkActive(SPORK_8_THRONE_PAYMENT_ENFORCEMENT)){
            return false;
        } else {
            LogPrintf("Throne payment enforcement is disabled, accepting block\n");
            return true;
        }
    }

    return false;
}


void FillBlockPayee(CMutableTransaction& txNew, int64_t nFees)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;

    if(IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS) && budget.IsBudgetPaymentBlock(pindexPrev->nHeight+1)){
        budget.FillBlockPayee(txNew, nFees);
    } else {
        masternodePayments.FillBlockPayee(txNew, nFees);
    }
}

std::string GetRequiredPaymentsString(int nBlockHeight)
{
    if(IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS) && budget.IsBudgetPaymentBlock(nBlockHeight)){
        return budget.GetRequiredPaymentsString(nBlockHeight);
    } else {
        return masternodePayments.GetRequiredPaymentsString(nBlockHeight);
    }
}

void CThronePayments::FillBlockPayee(CMutableTransaction& txNew, int64_t nFees)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;

    bool hasPayment = true;
    CScript payee;

    //spork
    if(!masternodePayments.GetBlockPayee(pindexPrev->nHeight+1, payee)){
        //no masternode detected
        CThrone* winningNode = mnodeman.GetCurrentThroNe(1);
        if(winningNode){
            payee = GetScriptForDestination(winningNode->pubkey.GetID());
        } else {
            LogPrintf("CreateNewBlock: Failed to detect masternode to pay\n");
            hasPayment = false;
        }
    }

    CAmount blockValue = GetBlockValue(pindexPrev->nBits, pindexPrev->nHeight, nFees);
    CAmount masternodePayment = GetThronePayment(pindexPrev->nHeight+1, blockValue);

    txNew.vout[0].nValue = blockValue;

    if(hasPayment){
        txNew.vout.resize(2);

        txNew.vout[1].scriptPubKey = payee;
        txNew.vout[1].nValue = masternodePayment;

        txNew.vout[0].nValue -= masternodePayment;

        CTxDestination address1;
        ExtractDestination(payee, address1);
        CBitcoinAddress address2(address1);

        LogPrintf("Throne payment to %s\n", address2.ToString().c_str());
    }
}

int CThronePayments::GetMinThronePaymentsProto() {
    return IsSporkActive(SPORK_10_THRONE_PAY_UPDATED_NODES)
            ? MIN_THRONE_PAYMENT_PROTO_VERSION_2
            : MIN_THRONE_PAYMENT_PROTO_VERSION_1;
}

void CThronePayments::ProcessMessageThronePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(!masternodeSync.IsBlockchainSynced()) return;

    if(fLiteMode) return; //disable all Darksend/Throne related functionality


    if (strCommand == "mnget") { //Throne Payments Request Sync
        if(fLiteMode) return; //disable all Darksend/Throne related functionality

        int nCountNeeded;
        vRecv >> nCountNeeded;

        if(Params().NetworkID() == CBaseChainParams::MAIN){
            if(pfrom->HasFulfilledRequest("mnget")) {
                LogPrintf("mnget - peer already asked me for the list\n");
                Misbehaving(pfrom->GetId(), 20);
                return;
            }
        }

        pfrom->FulfilledRequest("mnget");
        masternodePayments.Sync(pfrom, nCountNeeded);
        LogPrintf("mnget - Sent Throne winners to %s\n", pfrom->addr.ToString().c_str());
    }
    else if (strCommand == "mnw") { //Throne Payments Declare Winner
        //this is required in litemodef
        CThronePaymentWinner winner;
        vRecv >> winner;

        if(pfrom->nVersion < MIN_MNW_PEER_PROTO_VERSION) return;

        int nHeight;
        {
            TRY_LOCK(cs_main, locked);
            if(!locked || chainActive.Tip() == NULL) return;
            nHeight = chainActive.Tip()->nHeight;
        }

        if(masternodePayments.mapThronePayeeVotes.count(winner.GetHash())){
            LogPrint("mnpayments", "mnw - Already seen - %s bestHeight %d\n", winner.GetHash().ToString().c_str(), nHeight);
            masternodeSync.AddedThroneWinner(winner.GetHash());
            return;
        }

        int nFirstBlock = nHeight - (mnodeman.CountEnabled()*1.25);
        if(winner.nBlockHeight < nFirstBlock || winner.nBlockHeight > nHeight+20){
            LogPrint("mnpayments", "mnw - winner out of range - FirstBlock %d Height %d bestHeight %d\n", nFirstBlock, winner.nBlockHeight, nHeight);
            return;
        }

        std::string strError = "";
        if(!winner.IsValid(pfrom, strError)){
            if(strError != "") LogPrintf("mnw - invalid message - %s\n", strError);
            return;
        }

        if(!masternodePayments.CanVote(winner.vinThrone.prevout, winner.nBlockHeight)){
            LogPrintf("mnw - masternode already voted - %s\n", winner.vinThrone.prevout.ToStringShort());
            return;
        }

        if(!winner.SignatureValid()){
            LogPrintf("mnw - invalid signature\n");
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, winner.vinThrone);
            return;
        }

        CTxDestination address1;
        ExtractDestination(winner.payee, address1);
        CBitcoinAddress address2(address1);

        LogPrint("mnpayments", "mnw - winning vote - Addr %s Height %d bestHeight %d - %s\n", address2.ToString().c_str(), winner.nBlockHeight, nHeight, winner.vinThrone.prevout.ToStringShort());

        if(masternodePayments.AddWinningThrone(winner)){
            winner.Relay();
            masternodeSync.AddedThroneWinner(winner.GetHash());
        }
    }
}

bool CThronePaymentWinner::Sign(CKey& keyThrone, CPubKey& pubKeyThrone)
{
    std::string errorMessage;
    std::string strThroNeSignMessage;

    std::string strMessage =  vinThrone.prevout.ToStringShort() +
                boost::lexical_cast<std::string>(nBlockHeight) +
                payee.ToString();

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyThrone)) {
        LogPrintf("CThronePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyThrone, vchSig, strMessage, errorMessage)) {
        LogPrintf("CThronePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    return true;
}

bool CThronePayments::GetBlockPayee(int nBlockHeight, CScript& payee)
{
    if(mapThroneBlocks.count(nBlockHeight)){
        return mapThroneBlocks[nBlockHeight].GetPayee(payee);
    }

    return false;
}

// Is this masternode scheduled to get paid soon? 
// -- Only look ahead up to 8 blocks to allow for propagation of the latest 2 winners
bool CThronePayments::IsScheduled(CThrone& mn, int nNotBlockHeight)
{
    LOCK(cs_mapThroneBlocks);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if(!locked || chainActive.Tip() == NULL) return false;
        nHeight = chainActive.Tip()->nHeight;
    }

    CScript mnpayee;
    mnpayee = GetScriptForDestination(mn.pubkey.GetID());

    CScript payee;
    for(int64_t h = nHeight; h <= nHeight+8; h++){
        if(h == nNotBlockHeight) continue;
        if(mapThroneBlocks.count(h)){
            if(mapThroneBlocks[h].GetPayee(payee)){
                if(mnpayee == payee) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool CThronePayments::AddWinningThrone(CThronePaymentWinner& winnerIn)
{
    uint256 blockHash = 0;
    if(!GetBlockHash(blockHash, winnerIn.nBlockHeight-100)) {
        return false;
    }

    {
        LOCK2(cs_mapThronePayeeVotes, cs_mapThroneBlocks);
    
        if(mapThronePayeeVotes.count(winnerIn.GetHash())){
           return false;
        }

        mapThronePayeeVotes[winnerIn.GetHash()] = winnerIn;

        if(!mapThroneBlocks.count(winnerIn.nBlockHeight)){
           CThroneBlockPayees blockPayees(winnerIn.nBlockHeight);
           mapThroneBlocks[winnerIn.nBlockHeight] = blockPayees;
        }
    }

    int n = 1;
    if(IsReferenceNode(winnerIn.vinThrone)) n = 100;
    mapThroneBlocks[winnerIn.nBlockHeight].AddPayee(winnerIn.payee, n);

    return true;
}

bool CThroneBlockPayees::IsTransactionValid(const CTransaction& txNew)
{
    LOCK(cs_vecPayments);

    int nMaxSignatures = 0;
    std::string strPayeesPossible = "";

    CAmount masternodePayment = GetThronePayment(nBlockHeight, txNew.GetValueOut());

    //require at least 6 signatures

    BOOST_FOREACH(CThronePayee& payee, vecPayments)
        if(payee.nVotes >= nMaxSignatures && payee.nVotes >= MNPAYMENTS_SIGNATURES_REQUIRED)
            nMaxSignatures = payee.nVotes;

    // if we don't have at least 6 signatures on a payee, approve whichever is the longest chain
    if(nMaxSignatures < MNPAYMENTS_SIGNATURES_REQUIRED) return true;

    BOOST_FOREACH(CThronePayee& payee, vecPayments)
    {
        bool found = false;
        BOOST_FOREACH(CTxOut out, txNew.vout){
            if(payee.scriptPubKey == out.scriptPubKey && masternodePayment == out.nValue){
                found = true;
            }
        }

        if(payee.nVotes >= MNPAYMENTS_SIGNATURES_REQUIRED){
            if(found) return true;

            CTxDestination address1;
            ExtractDestination(payee.scriptPubKey, address1);
            CBitcoinAddress address2(address1);

            if(strPayeesPossible == ""){
                strPayeesPossible += address2.ToString();
            } else {
                strPayeesPossible += "," + address2.ToString();
            }
        }
    }


    LogPrintf("CThronePayments::IsTransactionValid - Missing required payment - %s\n", strPayeesPossible.c_str());
    return false;
}

std::string CThroneBlockPayees::GetRequiredPaymentsString()
{
    LOCK(cs_vecPayments);

    std::string ret = "Unknown";

    BOOST_FOREACH(CThronePayee& payee, vecPayments)
    {
        CTxDestination address1;
        ExtractDestination(payee.scriptPubKey, address1);
        CBitcoinAddress address2(address1);

        if(ret != "Unknown"){
            ret += ", " + address2.ToString() + ":" + boost::lexical_cast<std::string>(payee.nVotes);
        } else {
            ret = address2.ToString() + ":" + boost::lexical_cast<std::string>(payee.nVotes);
        }
    }

    return ret;
}

std::string CThronePayments::GetRequiredPaymentsString(int nBlockHeight)
{
    LOCK(cs_mapThroneBlocks);

    if(mapThroneBlocks.count(nBlockHeight)){
        return mapThroneBlocks[nBlockHeight].GetRequiredPaymentsString();
    }

    return "Unknown";
}

bool CThronePayments::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    LOCK(cs_mapThroneBlocks);

    if(mapThroneBlocks.count(nBlockHeight)){
        return mapThroneBlocks[nBlockHeight].IsTransactionValid(txNew);
    }

    return true;
}

void CThronePayments::CleanPaymentList()
{
    LOCK2(cs_mapThronePayeeVotes, cs_mapThroneBlocks);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if(!locked || chainActive.Tip() == NULL) return;
        nHeight = chainActive.Tip()->nHeight;
    }

    //keep up to five cycles for historical sake
    int nLimit = std::max(int(mnodeman.size()*1.25), 1000);

    std::map<uint256, CThronePaymentWinner>::iterator it = mapThronePayeeVotes.begin();
    while(it != mapThronePayeeVotes.end()) {
        CThronePaymentWinner winner = (*it).second;

        if(nHeight - winner.nBlockHeight > nLimit){
            LogPrint("mnpayments", "CThronePayments::CleanPaymentList - Removing old Throne payment - block %d\n", winner.nBlockHeight);
            masternodeSync.mapSeenSyncMNW.erase((*it).first);
            mapThronePayeeVotes.erase(it++);
            mapThroneBlocks.erase(winner.nBlockHeight);
        } else {
            ++it;
        }
    }
}

bool IsReferenceNode(CTxIn& vin)
{
    //reference node - hybrid mode
    if(vin.prevout.ToStringShort() == "099c01bea63abd1692f60806bb646fa1d288e2d049281225f17e499024084e28-0") return true; // mainnet
    if(vin.prevout.ToStringShort() == "fbc16ae5229d6d99181802fd76a4feee5e7640164dcebc7f8feb04a7bea026f8-0") return true; // testnet
    if(vin.prevout.ToStringShort() == "e466f5d8beb4c2d22a314310dc58e0ea89505c95409754d0d68fb874952608cc-1") return true; // regtest

    return false;
}

bool CThronePaymentWinner::IsValid(CNode* pnode, std::string& strError)
{
    if(IsReferenceNode(vinThrone)) return true;

    CThrone* pmn = mnodeman.Find(vinThrone);

    if(!pmn)
    {
        strError = strprintf("Unknown Throne %s", vinThrone.prevout.ToStringShort());
        LogPrintf ("CThronePaymentWinner::IsValid - %s\n", strError);
        mnodeman.AskForMN(pnode, vinThrone);
        return false;
    }

    if(pmn->protocolVersion < MIN_MNW_PEER_PROTO_VERSION)
    {
        strError = strprintf("Throne protocol too old %d - req %d", pmn->protocolVersion, MIN_MNW_PEER_PROTO_VERSION);
        LogPrintf ("CThronePaymentWinner::IsValid - %s\n", strError);
        return false;
    }

    int n = mnodeman.GetThroneRank(vinThrone, nBlockHeight-100, MIN_MNW_PEER_PROTO_VERSION);

    if(n > MNPAYMENTS_SIGNATURES_TOTAL)
    {    
        //It's common to have masternodes mistakenly think they are in the top 10
        // We don't want to print all of these messages, or punish them unless they're way off
        if(n > MNPAYMENTS_SIGNATURES_TOTAL*2)
        {
            strError = strprintf("Throne not in the top %d (%d)", MNPAYMENTS_SIGNATURES_TOTAL, n);
            LogPrintf("CThronePaymentWinner::IsValid - %s\n", strError);
            if(masternodeSync.IsSynced()) Misbehaving(pnode->GetId(), 20);
        }
        return false;
    }

    return true;
}

bool CThronePayments::ProcessBlock(int nBlockHeight)
{
    if(!fThroNe) return false;

    //reference node - hybrid mode

    if(!IsReferenceNode(activeThrone.vin)){
        int n = mnodeman.GetThroneRank(activeThrone.vin, nBlockHeight-100, MIN_MNW_PEER_PROTO_VERSION);

        if(n == -1)
        {
            LogPrint("mnpayments", "CThronePayments::ProcessBlock - Unknown Throne\n");
            return false;
        }

        if(n > MNPAYMENTS_SIGNATURES_TOTAL)
        {
            LogPrint("mnpayments", "CThronePayments::ProcessBlock - Throne not in the top %d (%d)\n", MNPAYMENTS_SIGNATURES_TOTAL, n);
            return false;
        }
    }

    if(nBlockHeight <= nLastBlockHeight) return false;

    CThronePaymentWinner newWinner(activeThrone.vin);

    if(budget.IsBudgetPaymentBlock(nBlockHeight)){
        //is budget payment block -- handled by the budgeting software
    } else {
        LogPrintf("CThronePayments::ProcessBlock() Start nHeight %d - vin %s. \n", nBlockHeight, activeThrone.vin.ToString().c_str());

        // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
        int nCount = 0;
        CThrone *pmn = mnodeman.GetNextThroneInQueueForPayment(nBlockHeight, true, nCount);
        
        if(pmn != NULL)
        {
            LogPrintf("CThronePayments::ProcessBlock() Found by FindOldestNotInVec \n");

            newWinner.nBlockHeight = nBlockHeight;

            CScript payee = GetScriptForDestination(pmn->pubkey.GetID());
            newWinner.AddPayee(payee);

            CTxDestination address1;
            ExtractDestination(payee, address1);
            CBitcoinAddress address2(address1);

            LogPrintf("CThronePayments::ProcessBlock() Winner payee %s nHeight %d. \n", address2.ToString().c_str(), newWinner.nBlockHeight);
        } else {
            LogPrintf("CThronePayments::ProcessBlock() Failed to find masternode to pay\n");
        }

    }

    std::string errorMessage;
    CPubKey pubKeyThrone;
    CKey keyThrone;

    if(!darkSendSigner.SetKey(strThroNePrivKey, errorMessage, keyThrone, pubKeyThrone))
    {
        LogPrintf("CThronePayments::ProcessBlock() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    LogPrintf("CThronePayments::ProcessBlock() - Signing Winner\n");
    if(newWinner.Sign(keyThrone, pubKeyThrone))
    {
        LogPrintf("CThronePayments::ProcessBlock() - AddWinningThrone\n");

        if(AddWinningThrone(newWinner))
        {
            newWinner.Relay();
            nLastBlockHeight = nBlockHeight;
            return true;
        }
    }

    return false;
}

void CThronePaymentWinner::Relay()
{
    CInv inv(MSG_THRONE_WINNER, GetHash());
    RelayInv(inv);
}

bool CThronePaymentWinner::SignatureValid()
{

    CThrone* pmn = mnodeman.Find(vinThrone);

    if(pmn != NULL)
    {
        std::string strMessage =  vinThrone.prevout.ToStringShort() +
                    boost::lexical_cast<std::string>(nBlockHeight) +
                    payee.ToString();

        std::string errorMessage = "";
        if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)){
            return error("CThronePaymentWinner::SignatureValid() - Got bad Throne address signature %s \n", vinThrone.ToString().c_str());
        }

        return true;
    }

    return false;
}

void CThronePayments::Sync(CNode* node, int nCountNeeded)
{
    LOCK(cs_mapThronePayeeVotes);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if(!locked || chainActive.Tip() == NULL) return;
        nHeight = chainActive.Tip()->nHeight;
    }

    int nCount = (mnodeman.CountEnabled()*1.25);
    if(nCountNeeded > nCount) nCountNeeded = nCount;

    int nInvCount = 0;
    std::map<uint256, CThronePaymentWinner>::iterator it = mapThronePayeeVotes.begin();
    while(it != mapThronePayeeVotes.end()) {
        CThronePaymentWinner winner = (*it).second;
        if(winner.nBlockHeight >= nHeight-nCountNeeded && winner.nBlockHeight <= nHeight + 20) {
            node->PushInventory(CInv(MSG_THRONE_WINNER, winner.GetHash()));
            nInvCount++;
        }
        ++it;
    }
    node->PushMessage("ssc", THRONE_SYNC_MNW, nInvCount);
}

std::string CThronePayments::ToString() const
{
    std::ostringstream info;

    info << "Votes: " << (int)mapThronePayeeVotes.size() <<
            ", Blocks: " << (int)mapThroneBlocks.size();

    return info.str();
}



int CThronePayments::GetOldestBlock()
{
    LOCK(cs_mapThroneBlocks);

    int nOldestBlock = std::numeric_limits<int>::max();

    std::map<int, CThroneBlockPayees>::iterator it = mapThroneBlocks.begin();
    while(it != mapThroneBlocks.end()) {
        if((*it).first < nOldestBlock) {
            nOldestBlock = (*it).first;
        }
        it++;
    }

    return nOldestBlock;
}



int CThronePayments::GetNewestBlock()
{
    LOCK(cs_mapThroneBlocks);

    int nNewestBlock = 0;

    std::map<int, CThroneBlockPayees>::iterator it = mapThroneBlocks.begin();
    while(it != mapThroneBlocks.end()) {
        if((*it).first > nNewestBlock) {
            nNewestBlock = (*it).first;
        }
        it++;
    }

    return nNewestBlock;
}
