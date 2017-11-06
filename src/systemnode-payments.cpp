// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "systemnode-payments.h"
#include "systemnode-sync.h"
#include "systemnodeman.h"
#include "legacysigner.h"
#include "util.h"
#include "sync.h"
#include "spork.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include "masternode-budget.h"

/** Object for who's going to get paid on which blocks */
CSystemnodePayments systemnodePayments;

CCriticalSection cs_vecSNPayments;
CCriticalSection cs_mapSystemnodeBlocks;
CCriticalSection cs_mapSystemnodePayeeVotes;

//
// CSystemnodePaymentDB
//

CSystemnodePaymentDB::CSystemnodePaymentDB()
{
    pathDB = GetDataDir() / "snpayments.dat";
    strMagicMessage = "SystemnodePayments";
}

bool CSystemnodePaymentDB::Write(const CSystemnodePayments& objToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssObj(SER_DISK, CLIENT_VERSION);
    ssObj << strMagicMessage; // systemnode cache file specific magic message
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

    LogPrintf("Written info to snpayments.dat  %dms\n", GetTimeMillis() - nStart);

    return true;
}

CSystemnodePaymentDB::ReadResult CSystemnodePaymentDB::Read(CSystemnodePayments& objToLoad, bool fDryRun)
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
        // de-serialize file header (systemnode cache file specific magic message) and ..
        ssObj >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp)
        {
            error("%s : Invalid systemnode payement cache magic message", __func__);
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

        // de-serialize data into CSystemnodePayments object
        ssObj >> objToLoad;
    }
    catch (std::exception &e) {
        objToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    LogPrintf("Loaded info from snpayments.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", objToLoad.ToString());
    if(!fDryRun) {
        LogPrintf("Systemnode payments manager - cleaning....\n");
        objToLoad.CleanPaymentList();
        LogPrintf("Systemnode payments manager - result:\n");
        LogPrintf("  %s\n", objToLoad.ToString());
    }

    return Ok;
}

bool SNIsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight)
{
    if(!systemnodeSync.IsSynced()) { //there is no budget data to use to check anything -- find the longest chain
        LogPrint("snpayments", "Client not synced, skipping block payee checks\n");
        return true;
    }

    //check if it's a budget block
    if(IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS)){
        if(budget.IsBudgetPaymentBlock(nBlockHeight)){
            if(budget.IsTransactionValid(txNew, nBlockHeight)){
                return true;
            } else {
                LogPrintf("Invalid budget payment detected %s\n", txNew.ToString().c_str());
                if(IsSporkActive(SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT)){
                    return false;
                } else {
                    LogPrintf("Budget enforcement is disabled, accepting block\n");
                    return true;
                }
            }
        }
    }

    //check for systemnode payee
    if(systemnodePayments.IsTransactionValid(txNew, nBlockHeight))
    {
        return true;
    } else {
        LogPrintf("Invalid mn payment detected %s\n", txNew.ToString().c_str());
        if(IsSporkActive(SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT)){
            return false;
        } else {
            LogPrintf("Systemnode payment enforcement is disabled, accepting block\n");
            return true;
        }
    }

    return false;
}

bool CSystemnodePayments::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    LOCK(cs_mapSystemnodeBlocks);

    if(mapSystemnodeBlocks.count(nBlockHeight)){
        return mapSystemnodeBlocks[nBlockHeight].IsTransactionValid(txNew);
    }

    return true;
}

void DumpSystemnodePayments()
{
    int64_t nStart = GetTimeMillis();

    CSystemnodePaymentDB paymentdb;
    CSystemnodePayments tempPayments;

    LogPrintf("Verifying snpayments.dat format...\n");
    CSystemnodePaymentDB::ReadResult readResult = paymentdb.Read(tempPayments, true);
    // there was an error and it was not an error on file opening => do not proceed
    if (readResult == CSystemnodePaymentDB::FileError)
        LogPrintf("Missing budgets file - snpayments.dat, will try to recreate\n");
    else if (readResult != CSystemnodePaymentDB::Ok)
    {
        LogPrintf("Error reading snpayments.dat: ");
        if(readResult == CSystemnodePaymentDB::IncorrectFormat)
            LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
        else
        {
            LogPrintf("file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrintf("Writting info to snpayments.dat...\n");
    paymentdb.Write(systemnodePayments);

    LogPrintf("Budget dump finished  %dms\n", GetTimeMillis() - nStart);
}

void CSystemnodePayments::ProcessMessageSystemnodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(!systemnodeSync.IsBlockchainSynced()) return;

    if(fLiteMode) return; //disable all Systemnode related functionality


    if (strCommand == "snget") { //Systemnode Payments Request Sync
        if(fLiteMode) return; //disable all Systemnode related functionality

        int nCountNeeded;
        vRecv >> nCountNeeded;

        if(Params().NetworkID() == CBaseChainParams::MAIN){
            if(pfrom->HasFulfilledRequest("snget")) {
                LogPrintf("snget - peer already asked me for the list\n");
                Misbehaving(pfrom->GetId(), 20);
                return;
            }
        }

        pfrom->FulfilledRequest("snget");
        systemnodePayments.Sync(pfrom, nCountNeeded);
        LogPrintf("snget - Sent Systemnode winners to %s\n", pfrom->addr.ToString().c_str());
    }
    else if (strCommand == "snw") { //Systemnode Payments Declare Winner
        //this is required in litemodef
        CSystemnodePaymentWinner winner;
        vRecv >> winner;

        if(pfrom->nVersion < MIN_MNW_PEER_PROTO_VERSION) return;

        int nHeight;
        {
            TRY_LOCK(cs_main, locked);
            if(!locked || chainActive.Tip() == NULL) return;
            nHeight = chainActive.Tip()->nHeight;
        }

        if(systemnodePayments.mapSystemnodePayeeVotes.count(winner.GetHash())){
            LogPrint("snpayments", "snw - Already seen - %s bestHeight %d\n", winner.GetHash().ToString().c_str(), nHeight);
            systemnodeSync.AddedSystemnodeWinner(winner.GetHash());
            return;
        }

        int nFirstBlock = nHeight - (snodeman.CountEnabled()*1.25);
        if(winner.nBlockHeight < nFirstBlock || winner.nBlockHeight > nHeight+20){
            LogPrint("snpayments", "snw - winner out of range - FirstBlock %d Height %d bestHeight %d\n", nFirstBlock, winner.nBlockHeight, nHeight);
            return;
        }

        std::string strError = "";
        if(!winner.IsValid(pfrom, strError)){
            if(strError != "") LogPrintf("snw - invalid message - %s\n", strError);
            return;
        }

        if(!systemnodePayments.CanVote(winner.vinSystemnode.prevout, winner.nBlockHeight)){
            LogPrintf("snw - systemnode already voted - %s\n", winner.vinSystemnode.prevout.ToStringShort());
            return;
        }

        if(!winner.SignatureValid()){
            LogPrintf("snw - invalid signature\n");
            if(systemnodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced systemnode
            snodeman.AskForSN(pfrom, winner.vinSystemnode);
            return;
        }

        CTxDestination address1;
        ExtractDestination(winner.payee, address1);
        CBitcoinAddress address2(address1);

        LogPrint("snpayments", "snw - winning vote - Addr %s Height %d bestHeight %d - %s\n", address2.ToString().c_str(), winner.nBlockHeight, nHeight, winner.vinSystemnode.prevout.ToStringShort());

        if(systemnodePayments.AddWinningSystemnode(winner)){
            winner.Relay();
            systemnodeSync.AddedSystemnodeWinner(winner.GetHash());
        }
    }
}

bool CSystemnodePayments::CanVote(COutPoint outSystemnode, int nBlockHeight)
{
    LOCK(cs_mapSystemnodePayeeVotes);

    if (mapSystemnodesLastVote.count(outSystemnode) && mapSystemnodesLastVote[outSystemnode] == nBlockHeight) {
        return false;
    }

    //record this masternode voted
    mapSystemnodesLastVote[outSystemnode] = nBlockHeight;
    return true;
}

void SNFillBlockPayee(CMutableTransaction& txNew, int64_t nFees)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;

    if(IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS) && budget.IsBudgetPaymentBlock(pindexPrev->nHeight+1)){
        // Doesn't pay systemnode, miners get the full amount on these blocks
    } else {
        systemnodePayments.FillBlockPayee(txNew, nFees);
    }
}

void CSystemnodePayments::FillBlockPayee(CMutableTransaction& txNew, int64_t nFees)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;

    bool hasPayment = true;
    CScript payee;

    //spork
    if(!systemnodePayments.GetBlockPayee(pindexPrev->nHeight+1, payee)){
        //no systemnode detected
        CSystemnode* winningNode = snodeman.GetCurrentSystemNode(1);
        if(winningNode) {
            payee = GetScriptForDestination(winningNode->pubkey.GetID());
        } else {
            LogPrintf("CreateNewBlock: Failed to detect systemnode to pay\n");
            hasPayment = false;
        }
    }

    CAmount blockValue = GetBlockValue(pindexPrev->nBits, pindexPrev->nHeight, nFees);
    CAmount systemnodePayment = GetSystemnodePayment(pindexPrev->nHeight+1, blockValue);

    // This is already done in masternode
    //txNew.vout[0].nValue = blockValue;

    if(hasPayment) {
        txNew.vout.resize(3);

        // [0] is for miner, [1] masternode, [2] systemnode
        txNew.vout[2].scriptPubKey = payee;
        txNew.vout[2].nValue = systemnodePayment;

        txNew.vout[0].nValue -= systemnodePayment;

        CTxDestination address1;
        ExtractDestination(payee, address1);
        CBitcoinAddress address2(address1);

        LogPrintf("Systemnode payment to %s\n", address2.ToString().c_str());
    }
}

bool CSystemnodeBlockPayees::IsTransactionValid(const CTransaction& txNew)
{
    LOCK(cs_vecPayments);

    int nMaxSignatures = 0;
    std::string strPayeesPossible = "";

    CAmount systemnodePayment = GetSystemnodePayment(nBlockHeight, txNew.GetValueOut());

    //require at least 6 signatures

    BOOST_FOREACH(CSystemnodePayee& payee, vecPayments)
        if(payee.nVotes >= nMaxSignatures && payee.nVotes >= SNPAYMENTS_SIGNATURES_REQUIRED)
            nMaxSignatures = payee.nVotes;

    // if we don't have at least 6 signatures on a payee, approve whichever is the longest chain
    if(nMaxSignatures < SNPAYMENTS_SIGNATURES_REQUIRED) return true;

    BOOST_FOREACH(CSystemnodePayee& payee, vecPayments)
    {
        bool found = false;
        BOOST_FOREACH(CTxOut out, txNew.vout){
            if(payee.scriptPubKey == out.scriptPubKey && systemnodePayment == out.nValue){
                found = true;
            }
        }

        if(payee.nVotes >= SNPAYMENTS_SIGNATURES_REQUIRED){
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


    LogPrintf("CSystemnodePayments::IsTransactionValid - Missing required payment - %s\n", strPayeesPossible.c_str());
    return false;
}

bool CSystemnodePayments::GetBlockPayee(int nBlockHeight, CScript& payee)
{
    if(mapSystemnodeBlocks.count(nBlockHeight)){
        return mapSystemnodeBlocks[nBlockHeight].GetPayee(payee);
    }

    return false;
}

void CSystemnodePayments::CleanPaymentList()
{
    LOCK2(cs_mapSystemnodePayeeVotes, cs_mapSystemnodeBlocks);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if(!locked || chainActive.Tip() == NULL) return;
        nHeight = chainActive.Tip()->nHeight;
    }

    //keep up to five cycles for historical sake
    int nLimit = std::max(int(snodeman.size()*1.25), 1000);

    std::map<uint256, CSystemnodePaymentWinner>::iterator it = mapSystemnodePayeeVotes.begin();
    while(it != mapSystemnodePayeeVotes.end()) {
        CSystemnodePaymentWinner winner = (*it).second;

        if(nHeight - winner.nBlockHeight > nLimit){
            LogPrint("snpayments", "CSystemnodePayments::CleanPaymentList - Removing old Systemnode payment - block %d\n", winner.nBlockHeight);
            systemnodeSync.mapSeenSyncSNW.erase((*it).first);
            mapSystemnodePayeeVotes.erase(it++);
            mapSystemnodeBlocks.erase(winner.nBlockHeight);
        } else {
            ++it;
        }
    }
}

bool CSystemnodePaymentWinner::IsValid(CNode* pnode, std::string& strError)
{
    if(IsReferenceNode(vinSystemnode)) return true;

    CSystemnode* psn = snodeman.Find(vinSystemnode);

    if(!psn)
    {
        strError = strprintf("Unknown Systemnode %s", vinSystemnode.prevout.ToStringShort());
        LogPrintf ("CSystemnodePaymentWinner::IsValid - %s\n", strError);
        snodeman.AskForSN(pnode, vinSystemnode);
        return false;
    }

    if(psn->protocolVersion < MIN_MNW_PEER_PROTO_VERSION)
    {
        strError = strprintf("Systemnode protocol too old %d - req %d", psn->protocolVersion, MIN_MNW_PEER_PROTO_VERSION);
        LogPrintf ("CSystemnodePaymentWinner::IsValid - %s\n", strError);
        return false;
    }

    int n = snodeman.GetSystemnodeRank(vinSystemnode, nBlockHeight-100, MIN_MNW_PEER_PROTO_VERSION);

    if(n > MNPAYMENTS_SIGNATURES_TOTAL)
    {    
        //It's common to have systemnodes mistakenly think they are in the top 10
        // We don't want to print all of these messages, or punish them unless they're way off
        if(n > MNPAYMENTS_SIGNATURES_TOTAL*2)
        {
            strError = strprintf("Systemnode not in the top %d (%d)", MNPAYMENTS_SIGNATURES_TOTAL, n);
            LogPrintf("CSystemnodePaymentWinner::IsValid - %s\n", strError);
            if(systemnodeSync.IsSynced()) Misbehaving(pnode->GetId(), 20);
        }
        return false;
    }

    return true;
}

bool CSystemnodePayments::ProcessBlock(int nBlockHeight)
{
    if(!fSystemNode) return false;

    //reference node - hybrid mode

    if(!IsReferenceNode(activeSystemnode.vin)) {
        int n = snodeman.GetSystemnodeRank(activeSystemnode.vin, nBlockHeight - 100, MIN_MNW_PEER_PROTO_VERSION);

        if(n == -1)
        {
            LogPrint("snpayments", "CSystemnodePayments::ProcessBlock - Unknown Systemnode\n");
            return false;
        }

        if(n > SNPAYMENTS_SIGNATURES_TOTAL)
        {
            LogPrint("snpayments", "CSystemnodePayments::ProcessBlock - Systemnode not in the top %d (%d)\n", MNPAYMENTS_SIGNATURES_TOTAL, n);
            return false;
        }
    }

    if(nBlockHeight <= nLastBlockHeight) return false;

    CSystemnodePaymentWinner newWinner(activeSystemnode.vin);

    LogPrintf("CSystemnodePayments::ProcessBlock() Start nHeight %d - vin %s. \n", nBlockHeight, activeSystemnode.vin.ToString().c_str());
    // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
    int nCount = 0;
    CSystemnode *psn = snodeman.GetNextSystemnodeInQueueForPayment(nBlockHeight, true, nCount);

    if(psn != NULL)
    {
        LogPrintf("CSystemnodePayments::ProcessBlock() Found by FindOldestNotInVec \n");

        newWinner.nBlockHeight = nBlockHeight;

        CScript payee = GetScriptForDestination(psn->pubkey.GetID());
        newWinner.AddPayee(payee);

        CTxDestination address1;
        ExtractDestination(payee, address1);
        CBitcoinAddress address2(address1);

        LogPrintf("CSystemnodePayments::ProcessBlock() Winner payee %s nHeight %d. \n", address2.ToString().c_str(), newWinner.nBlockHeight);
    } else {
        LogPrintf("CSystemnodePayments::ProcessBlock() Failed to find systemnode to pay\n");
    }

    std::string errorMessage;
    CPubKey pubKeySystemnode;
    CKey keySystemnode;

    if(!legacySigner.SetKey(strSystemNodePrivKey, errorMessage, keySystemnode, pubKeySystemnode))
    {
        LogPrintf("CSystemnodePayments::ProcessBlock() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    LogPrintf("CSystemnodePayments::ProcessBlock() - Signing Winner\n");
    if(newWinner.Sign(keySystemnode, pubKeySystemnode))
    {
        LogPrintf("CSystemnodePayments::ProcessBlock() - AddWinningSystemnode\n");

        if(AddWinningSystemnode(newWinner))
        {
            newWinner.Relay();
            nLastBlockHeight = nBlockHeight;
            return true;
        }
    }

    return false;
}

bool CSystemnodePayments::AddWinningSystemnode(CSystemnodePaymentWinner& winnerIn)
{
    uint256 blockHash = uint256();
    if(!GetBlockHash(blockHash, winnerIn.nBlockHeight-100)) {
        return false;
    }

    {
        LOCK2(cs_mapSystemnodePayeeVotes, cs_mapSystemnodeBlocks);
    
        if(mapSystemnodePayeeVotes.count(winnerIn.GetHash())){
           return false;
        }

        mapSystemnodePayeeVotes[winnerIn.GetHash()] = winnerIn;

        if(!mapSystemnodeBlocks.count(winnerIn.nBlockHeight)){
           CSystemnodeBlockPayees blockPayees(winnerIn.nBlockHeight);
           mapSystemnodeBlocks[winnerIn.nBlockHeight] = blockPayees;
        }
    }

    int n = 1;
    if(IsReferenceNode(winnerIn.vinSystemnode)) n = 100;
    mapSystemnodeBlocks[winnerIn.nBlockHeight].AddPayee(winnerIn.payee, n);

    return true;
}

void CSystemnodePaymentWinner::Relay()
{
    CInv inv(MSG_SYSTEMNODE_WINNER, GetHash());
    RelayInv(inv);
}

bool CSystemnodePaymentWinner::SignatureValid()
{

    CSystemnode* psn = snodeman.Find(vinSystemnode);

    if(psn != NULL)
    {
        std::string strMessage =  vinSystemnode.prevout.ToStringShort() +
                    boost::lexical_cast<std::string>(nBlockHeight) +
                    payee.ToString();

        std::string errorMessage = "";
        if(!legacySigner.VerifyMessage(psn->pubkey2, vchSig, strMessage, errorMessage)){
            return error("CSystemnodePaymentWinner::SignatureValid() - Got bad Systemnode address signature %s \n", vinSystemnode.ToString().c_str());
        }

        return true;
    }

    return false;
}

void CSystemnodePayments::Sync(CNode* node, int nCountNeeded)
{
    LOCK(cs_mapSystemnodePayeeVotes);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if(!locked || chainActive.Tip() == NULL) return;
        nHeight = chainActive.Tip()->nHeight;
    }

    int nCount = (snodeman.CountEnabled()*1.25);
    if(nCountNeeded > nCount) nCountNeeded = nCount;

    int nInvCount = 0;
    std::map<uint256, CSystemnodePaymentWinner>::iterator it = mapSystemnodePayeeVotes.begin();
    while(it != mapSystemnodePayeeVotes.end()) {
        CSystemnodePaymentWinner winner = (*it).second;
        if(winner.nBlockHeight >= nHeight-nCountNeeded && winner.nBlockHeight <= nHeight + 20) {
            node->PushInventory(CInv(MSG_SYSTEMNODE_WINNER, winner.GetHash()));
            nInvCount++;
        }
        ++it;
    }
    node->PushMessage("snssc", SYSTEMNODE_SYNC_SNW, nInvCount);
}

// Is this systemnode scheduled to get paid soon? 
// -- Only look ahead up to 8 blocks to allow for propagation of the latest 2 winners
bool CSystemnodePayments::IsScheduled(CSystemnode& sn, int nNotBlockHeight)
{
    LOCK(cs_mapSystemnodeBlocks);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if(!locked || chainActive.Tip() == NULL) return false;
        nHeight = chainActive.Tip()->nHeight;
    }

    CScript snpayee;
    snpayee = GetScriptForDestination(sn.pubkey.GetID());

    CScript payee;
    for(int64_t h = nHeight; h <= nHeight+8; h++){
        if(h == nNotBlockHeight) continue;
        if(mapSystemnodeBlocks.count(h)){
            if(mapSystemnodeBlocks[h].GetPayee(payee)){
                if(snpayee == payee) {
                    return true;
                }
            }
        }
    }

    return false;
}

std::string CSystemnodePayments::ToString() const
{
    std::ostringstream info;

    info << "Votes: " << (int)mapSystemnodePayeeVotes.size() <<
            ", Blocks: " << (int)mapSystemnodeBlocks.size();

    return info.str();
}

bool CSystemnodePaymentWinner::Sign(CKey& keySystemnode, CPubKey& pubKeySystemnode)
{
    std::string errorMessage;
    std::string strSystemNodeSignMessage;

    std::string strMessage =  vinSystemnode.prevout.ToStringShort() +
                boost::lexical_cast<std::string>(nBlockHeight) +
                payee.ToString();

    if(!legacySigner.SignMessage(strMessage, errorMessage, vchSig, keySystemnode)) {
        LogPrintf("CSystemnodePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    if(!legacySigner.VerifyMessage(pubKeySystemnode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CSystemnodePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    return true;
}
