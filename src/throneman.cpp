// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "throneman.h"
#include "throne.h"
#include "activethrone.h"
#include "darksend.h"
#include "util.h"
#include "addrman.h"
#include "spork.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

/** Throne manager */
CThroneMan mnodeman;

struct CompareLastPaid
{
    bool operator()(const pair<int64_t, CTxIn>& t1,
                    const pair<int64_t, CTxIn>& t2) const
    {
        return t1.first < t2.first;
    }
};

struct CompareScoreTxIn
{
    bool operator()(const pair<int64_t, CTxIn>& t1,
                    const pair<int64_t, CTxIn>& t2) const
    {
        return t1.first < t2.first;
    }
};

struct CompareScoreMN
{
    bool operator()(const pair<int64_t, CThrone>& t1,
                    const pair<int64_t, CThrone>& t2) const
    {
        return t1.first < t2.first;
    }
};

//
// CThroneDB
//

CThroneDB::CThroneDB()
{
    pathMN = GetDataDir() / "mncache.dat";
    strMagicMessage = "ThroneCache";
}

bool CThroneDB::Write(const CThroneMan& mnodemanToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssThrones(SER_DISK, CLIENT_VERSION);
    ssThrones << strMagicMessage; // throne cache file specific magic message
    ssThrones << FLATDATA(Params().MessageStart()); // network specific magic number
    ssThrones << mnodemanToSave;
    uint256 hash = Hash(ssThrones.begin(), ssThrones.end());
    ssThrones << hash;

    // open output file, and associate with CAutoFile
    FILE *file = fopen(pathMN.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s : Failed to open file %s", __func__, pathMN.string());

    // Write and commit header, data
    try {
        fileout << ssThrones;
    }
    catch (std::exception &e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
//    FileCommit(fileout);
    fileout.fclose();

    LogPrintf("Written info to mncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", mnodemanToSave.ToString());

    return true;
}

CThroneDB::ReadResult CThroneDB::Read(CThroneMan& mnodemanToLoad, bool fDryRun)
{
    int64_t nStart = GetTimeMillis();
    // open input file, and associate with CAutoFile
    FILE *file = fopen(pathMN.string().c_str(), "rb");
    CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
    if (filein.IsNull())
    {
        error("%s : Failed to open file %s", __func__, pathMN.string());
        return FileError;
    }

    // use file size to size memory buffer
    int fileSize = boost::filesystem::file_size(pathMN);
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

    CDataStream ssThrones(vchData, SER_DISK, CLIENT_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssThrones.begin(), ssThrones.end());
    if (hashIn != hashTmp)
    {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }

    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header (throne cache file specific magic message) and ..

        ssThrones >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp)
        {
            error("%s : Invalid throne cache magic message", __func__);
            return IncorrectMagicMessage;
        }

        // de-serialize file header (network specific magic number) and ..
        ssThrones >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
        {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }
        // de-serialize data into CThroneMan object
        ssThrones >> mnodemanToLoad;
    }
    catch (std::exception &e) {
        mnodemanToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    LogPrintf("Loaded info from mncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", mnodemanToLoad.ToString());
    if(!fDryRun) {
        LogPrintf("Throne manager - cleaning....\n");
        mnodemanToLoad.CheckAndRemove(true);
        LogPrintf("Throne manager - result:\n");
        LogPrintf("  %s\n", mnodemanToLoad.ToString());
    }

    return Ok;
}

void DumpThrones()
{
    int64_t nStart = GetTimeMillis();

    CThroneDB mndb;
    CThroneMan tempMnodeman;

    LogPrintf("Verifying mncache.dat format...\n");
    CThroneDB::ReadResult readResult = mndb.Read(tempMnodeman, true);
    // there was an error and it was not an error on file opening => do not proceed
    if (readResult == CThroneDB::FileError)
        LogPrintf("Missing throne cache file - mncache.dat, will try to recreate\n");
    else if (readResult != CThroneDB::Ok)
    {
        LogPrintf("Error reading mncache.dat: ");
        if(readResult == CThroneDB::IncorrectFormat)
            LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
        else
        {
            LogPrintf("file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrintf("Writting info to mncache.dat...\n");
    mndb.Write(mnodeman);

    LogPrintf("Throne dump finished  %dms\n", GetTimeMillis() - nStart);
}

CThroneMan::CThroneMan() {
    nDsqCount = 0;
}

bool CThroneMan::Add(CThrone &mn)
{
    LOCK(cs);

    if (!mn.IsEnabled())
        return false;

    CThrone *pmn = Find(mn.vin);
    if (pmn == NULL)
    {
        LogPrint("throne", "CThroneMan: Adding new Throne %s - %i now\n", mn.addr.ToString(), size() + 1);
        vThrones.push_back(mn);
        return true;
    }

    return false;
}

void CThroneMan::AskForMN(CNode* pnode, CTxIn &vin)
{
    std::map<COutPoint, int64_t>::iterator i = mWeAskedForThroneListEntry.find(vin.prevout);
    if (i != mWeAskedForThroneListEntry.end())
    {
        int64_t t = (*i).second;
        if (GetTime() < t) return; // we've asked recently
    }

    // ask for the mnb info once from the node that sent mnp

    LogPrintf("CThroneMan::AskForMN - Asking node for missing entry, vin: %s\n", vin.ToString());
    pnode->PushMessage("dseg", vin);
    int64_t askAgain = GetTime() + THRONE_MIN_MNP_SECONDS;
    mWeAskedForThroneListEntry[vin.prevout] = askAgain;
}

void CThroneMan::Check()
{
    LOCK(cs);

    BOOST_FOREACH(CThrone& mn, vThrones) {
        mn.Check();
    }
}

void CThroneMan::CheckAndRemove(bool forceExpiredRemoval)
{
    Check();

    LOCK(cs);

    //remove inactive and outdated
    vector<CThrone>::iterator it = vThrones.begin();
    while(it != vThrones.end()){
        if((*it).activeState == CThrone::THRONE_REMOVE ||
                (*it).activeState == CThrone::THRONE_VIN_SPENT ||
                (forceExpiredRemoval && (*it).activeState == CThrone::THRONE_EXPIRED) ||
                (*it).protocolVersion < thronePayments.GetMinThronePaymentsProto()) {
            LogPrint("throne", "CThroneMan: Removing inactive Throne %s - %i now\n", (*it).addr.ToString(), size() - 1);

            //erase all of the broadcasts we've seen from this vin
            // -- if we missed a few pings and the node was removed, this will allow is to get it back without them 
            //    sending a brand new mnb
            map<uint256, CThroneBroadcast>::iterator it3 = mapSeenThroneBroadcast.begin();
            while(it3 != mapSeenThroneBroadcast.end()){
                if((*it3).second.vin == (*it).vin){
                    throneSync.mapSeenSyncMNB.erase((*it3).first);
                    mapSeenThroneBroadcast.erase(it3++);
                } else {
                    ++it3;
                }
            }

            // allow us to ask for this throne again if we see another ping
            map<COutPoint, int64_t>::iterator it2 = mWeAskedForThroneListEntry.begin();
            while(it2 != mWeAskedForThroneListEntry.end()){
                if((*it2).first == (*it).vin.prevout){
                    mWeAskedForThroneListEntry.erase(it2++);
                } else {
                    ++it2;
                }
            }

            it = vThrones.erase(it);
        } else {
            ++it;
        }
    }

    // check who's asked for the Throne list
    map<CNetAddr, int64_t>::iterator it1 = mAskedUsForThroneList.begin();
    while(it1 != mAskedUsForThroneList.end()){
        if((*it1).second < GetTime()) {
            mAskedUsForThroneList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check who we asked for the Throne list
    it1 = mWeAskedForThroneList.begin();
    while(it1 != mWeAskedForThroneList.end()){
        if((*it1).second < GetTime()){
            mWeAskedForThroneList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check which Thrones we've asked for
    map<COutPoint, int64_t>::iterator it2 = mWeAskedForThroneListEntry.begin();
    while(it2 != mWeAskedForThroneListEntry.end()){
        if((*it2).second < GetTime()){
            mWeAskedForThroneListEntry.erase(it2++);
        } else {
            ++it2;
        }
    }

    // remove expired mapSeenThroneBroadcast
    map<uint256, CThroneBroadcast>::iterator it3 = mapSeenThroneBroadcast.begin();
    while(it3 != mapSeenThroneBroadcast.end()){
        if((*it3).second.lastPing.sigTime < GetTime() - THRONE_REMOVAL_SECONDS*2){
            LogPrint("throne", "CThroneMan::CheckAndRemove - Removing expired Throne broadcast %s\n", (*it3).second.GetHash().ToString());
            throneSync.mapSeenSyncMNB.erase((*it3).second.GetHash());
            mapSeenThroneBroadcast.erase(it3++);
        } else {
            ++it3;
        }
    }

    // remove expired mapSeenThronePing
    map<uint256, CThronePing>::iterator it4 = mapSeenThronePing.begin();
    while(it4 != mapSeenThronePing.end()){
        if((*it4).second.sigTime < GetTime()-(THRONE_REMOVAL_SECONDS*2)){
            mapSeenThronePing.erase(it4++);
        } else {
            ++it4;
        }
    }

}

void CThroneMan::Clear()
{
    LOCK(cs);
    vThrones.clear();
    mAskedUsForThroneList.clear();
    mWeAskedForThroneList.clear();
    mWeAskedForThroneListEntry.clear();
    mapSeenThroneBroadcast.clear();
    mapSeenThronePing.clear();
    nDsqCount = 0;
}

int CThroneMan::CountEnabled(int protocolVersion)
{
    int i = 0;
    protocolVersion = protocolVersion == -1 ? thronePayments.GetMinThronePaymentsProto() : protocolVersion;

    BOOST_FOREACH(CThrone& mn, vThrones) {
        mn.Check();
        if(mn.protocolVersion < protocolVersion || !mn.IsEnabled()) continue;
        i++;
    }

    return i;
}

void CThroneMan::DsegUpdate(CNode* pnode)
{
    LOCK(cs);

    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(!(pnode->addr.IsRFC1918() || pnode->addr.IsLocal())){
            std::map<CNetAddr, int64_t>::iterator it = mWeAskedForThroneList.find(pnode->addr);
            if (it != mWeAskedForThroneList.end())
            {
                if (GetTime() < (*it).second) {
                    LogPrintf("dseg - we already asked %s for the list; skipping...\n", pnode->addr.ToString());
                    return;
                }
            }
        }
    }
    
    pnode->PushMessage("dseg", CTxIn());
    int64_t askAgain = GetTime() + THRONES_DSEG_SECONDS;
    mWeAskedForThroneList[pnode->addr] = askAgain;
}

CThrone *CThroneMan::Find(const CScript &payee)
{
    LOCK(cs);
    CScript payee2;

    BOOST_FOREACH(CThrone& mn, vThrones)
    {
        payee2 = GetScriptForDestination(mn.pubkey.GetID());
        if(payee2 == payee)
            return &mn;
    }
    return NULL;
}

CThrone *CThroneMan::Find(const CTxIn &vin)
{
    LOCK(cs);

    BOOST_FOREACH(CThrone& mn, vThrones)
    {
        if(mn.vin.prevout == vin.prevout)
            return &mn;
    }
    return NULL;
}


CThrone *CThroneMan::Find(const CPubKey &pubKeyThrone)
{
    LOCK(cs);

    BOOST_FOREACH(CThrone& mn, vThrones)
    {
        if(mn.pubkey2 == pubKeyThrone)
            return &mn;
    }
    return NULL;
}

// 
// Deterministically select the oldest/best throne to pay on the network
//
CThrone* CThroneMan::GetNextThroneInQueueForPayment(int nBlockHeight, bool fFilterSigTime, int& nCount)
{
    LOCK(cs);

    CThrone *pBestThrone = NULL;
    std::vector<pair<int64_t, CTxIn> > vecThroneLastPaid;

    /*
        Make a vector with all of the last paid times
    */

    int nMnCount = CountEnabled();
    BOOST_FOREACH(CThrone &mn, vThrones)
    {
        mn.Check();
        if(!mn.IsEnabled()) continue;

        // //check protocol version
        if(mn.protocolVersion < thronePayments.GetMinThronePaymentsProto()) continue;

        //it's in the list (up to 8 entries ahead of current block to allow propagation) -- so let's skip it
        if(thronePayments.IsScheduled(mn, nBlockHeight)) continue;

        //it's too new, wait for a cycle
        if(fFilterSigTime && mn.sigTime + (nMnCount*2.6*60) > GetAdjustedTime()) continue;

        //make sure it has as many confirmations as there are thrones
        if(mn.GetThroneInputAge() < nMnCount) continue;

        vecThroneLastPaid.push_back(make_pair(mn.SecondsSincePayment(), mn.vin));
    }

    nCount = (int)vecThroneLastPaid.size();

    //when the network is in the process of upgrading, don't penalize nodes that recently restarted
    if(fFilterSigTime && nCount < nMnCount/3) return GetNextThroneInQueueForPayment(nBlockHeight, false, nCount);

    // Sort them high to low
    sort(vecThroneLastPaid.rbegin(), vecThroneLastPaid.rend(), CompareLastPaid());

    // Look at 1/10 of the oldest nodes (by last payment), calculate their scores and pay the best one
    //  -- This doesn't look at who is being paid in the +8-10 blocks, allowing for double payments very rarely
    //  -- 1/100 payments should be a double payment on mainnet - (1/(3000/10))*2
    //  -- (chance per block * chances before IsScheduled will fire)
    int nTenthNetwork = CountEnabled()/10;
    int nCountTenth = 0; 
    uint256 nHigh = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CTxIn)& s, vecThroneLastPaid){
        CThrone* pmn = Find(s.second);
        if(!pmn) break;

        uint256 n = pmn->CalculateScore(1, nBlockHeight-100);
        if(n > nHigh){
            nHigh = n;
            pBestThrone = pmn;
        }
        nCountTenth++;
        if(nCountTenth >= nTenthNetwork) break;
    }
    return pBestThrone;
}

CThrone *CThroneMan::FindRandomNotInVec(std::vector<CTxIn> &vecToExclude, int protocolVersion)
{
    LOCK(cs);

    protocolVersion = protocolVersion == -1 ? thronePayments.GetMinThronePaymentsProto() : protocolVersion;

    int nCountEnabled = CountEnabled(protocolVersion);
    LogPrintf("CThroneMan::FindRandomNotInVec - nCountEnabled - vecToExclude.size() %d\n", nCountEnabled - vecToExclude.size());
    if(nCountEnabled - vecToExclude.size() < 1) return NULL;

    int rand = GetRandInt(nCountEnabled - vecToExclude.size());
    LogPrintf("CThroneMan::FindRandomNotInVec - rand %d\n", rand);
    bool found;

    BOOST_FOREACH(CThrone &mn, vThrones) {
        if(mn.protocolVersion < protocolVersion || !mn.IsEnabled()) continue;
        found = false;
        BOOST_FOREACH(CTxIn &usedVin, vecToExclude) {
            if(mn.vin.prevout == usedVin.prevout) {
                found = true;
                break;
            }
        }
        if(found) continue;
        if(--rand < 1) {
            return &mn;
        }
    }

    return NULL;
}

CThrone* CThroneMan::GetCurrentThroNe(int mod, int64_t nBlockHeight, int minProtocol)
{
    int64_t score = 0;
    CThrone* winner = NULL;

    // scan for winner
    BOOST_FOREACH(CThrone& mn, vThrones) {
        mn.Check();
        if(mn.protocolVersion < minProtocol || !mn.IsEnabled()) continue;

        // calculate the score for each Throne
        uint256 n = mn.CalculateScore(mod, nBlockHeight);
        int64_t n2 = n.GetCompact(false);

        // determine the winner
        if(n2 > score){
            score = n2;
            winner = &mn;
        }
    }

    return winner;
}

int CThroneMan::GetThroneRank(const CTxIn& vin, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<pair<int64_t, CTxIn> > vecThroneScores;

    //make sure we know about this block
    uint256 hash = 0;
    if(!GetBlockHash(hash, nBlockHeight)) return -1;

    // scan for winner
    BOOST_FOREACH(CThrone& mn, vThrones) {
        if(mn.protocolVersion < minProtocol) continue;
        if(fOnlyActive) {
            mn.Check();
            if(!mn.IsEnabled()) continue;
        }
        uint256 n = mn.CalculateScore(1, nBlockHeight);
        int64_t n2 = n.GetCompact(false);

        vecThroneScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecThroneScores.rbegin(), vecThroneScores.rend(), CompareScoreTxIn());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CTxIn)& s, vecThroneScores){
        rank++;
        if(s.second.prevout == vin.prevout) {
            return rank;
        }
    }

    return -1;
}

std::vector<pair<int, CThrone> > CThroneMan::GetThroneRanks(int64_t nBlockHeight, int minProtocol)
{
    std::vector<pair<int64_t, CThrone> > vecThroneScores;
    std::vector<pair<int, CThrone> > vecThroneRanks;

    //make sure we know about this block
    uint256 hash = 0;
    if(!GetBlockHash(hash, nBlockHeight)) return vecThroneRanks;

    // scan for winner
    BOOST_FOREACH(CThrone& mn, vThrones) {

        mn.Check();

        if(mn.protocolVersion < minProtocol) continue;
        if(!mn.IsEnabled()) {
            continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        int64_t n2 = n.GetCompact(false);

        vecThroneScores.push_back(make_pair(n2, mn));
    }

    sort(vecThroneScores.rbegin(), vecThroneScores.rend(), CompareScoreMN());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CThrone)& s, vecThroneScores){
        rank++;
        vecThroneRanks.push_back(make_pair(rank, s.second));
    }

    return vecThroneRanks;
}

CThrone* CThroneMan::GetThroneByRank(int nRank, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<pair<int64_t, CTxIn> > vecThroneScores;

    // scan for winner
    BOOST_FOREACH(CThrone& mn, vThrones) {

        if(mn.protocolVersion < minProtocol) continue;
        if(fOnlyActive) {
            mn.Check();
            if(!mn.IsEnabled()) continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        int64_t n2 = n.GetCompact(false);

        vecThroneScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecThroneScores.rbegin(), vecThroneScores.rend(), CompareScoreTxIn());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CTxIn)& s, vecThroneScores){
        rank++;
        if(rank == nRank) {
            return Find(s.second);
        }
    }

    return NULL;
}

void CThroneMan::ProcessThroneConnections()
{
    //we don't care about this for regtest
    if(Params().NetworkID() == CBaseChainParams::REGTEST) return;

    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes) {
        if(pnode->fDarkSendMaster){
            if(darkSendPool.pSubmittedToThrone != NULL && pnode->addr == darkSendPool.pSubmittedToThrone->addr) continue;
            LogPrintf("Closing Throne connection %s \n", pnode->addr.ToString());
            pnode->fDarkSendMaster = false;
            pnode->Release();
        }
    }
}

void CThroneMan::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{

    if(fLiteMode) return; //disable all Darksend/Throne related functionality
    if(!throneSync.IsBlockchainSynced()) return;

    LOCK(cs_process_message);

    if (strCommand == "mnb") { //Throne Broadcast
        CThroneBroadcast mnb;
        vRecv >> mnb;

        int nDoS = 0;
        if (CheckMnbAndUpdateThroneList(mnb, nDoS)) {
            // use announced Throne as a peer
             addrman.Add(CAddress(mnb.addr), pfrom->addr, 2*60*60);
        } else {
            if(nDoS > 0) Misbehaving(pfrom->GetId(), nDoS);
        }
    }

    else if (strCommand == "mnp") { //Throne Ping
        CThronePing mnp;
        vRecv >> mnp;

        LogPrint("throne", "mnp - Throne ping, vin: %s\n", mnp.vin.ToString());

        if(mapSeenThronePing.count(mnp.GetHash())) return; //seen
        mapSeenThronePing.insert(make_pair(mnp.GetHash(), mnp));

        int nDoS = 0;
        if(mnp.CheckAndUpdate(nDoS)) return;

        if(nDoS > 0) {
            // if anything significant failed, mark that node
            Misbehaving(pfrom->GetId(), nDoS);
        } else {
            // if nothing significant failed, search existing Throne list
            CThrone* pmn = Find(mnp.vin);
            // if it's known, don't ask for the mnb, just return
            if(pmn != NULL) return;
        }

        // something significant is broken or mn is unknown,
        // we might have to ask for a throne entry once
        AskForMN(pfrom, mnp.vin);

    } else if (strCommand == "dseg") { //Get Throne list or specific entry

        CTxIn vin;
        vRecv >> vin;

        if(vin == CTxIn()) { //only should ask for this once
            //local network
            bool isLocal = (pfrom->addr.IsRFC1918() || pfrom->addr.IsLocal());

            if(!isLocal && Params().NetworkID() == CBaseChainParams::MAIN) {
                std::map<CNetAddr, int64_t>::iterator i = mAskedUsForThroneList.find(pfrom->addr);
                if (i != mAskedUsForThroneList.end()){
                    int64_t t = (*i).second;
                    if (GetTime() < t) {
                        Misbehaving(pfrom->GetId(), 34);
                        LogPrintf("dseg - peer already asked me for the list\n");
                        return;
                    }
                }
                int64_t askAgain = GetTime() + THRONES_DSEG_SECONDS;
                mAskedUsForThroneList[pfrom->addr] = askAgain;
            }
        } //else, asking for a specific node which is ok


        int nInvCount = 0;

        BOOST_FOREACH(CThrone& mn, vThrones) {
            if(mn.addr.IsRFC1918()) continue; //local network

            if(mn.IsEnabled()) {
                LogPrint("throne", "dseg - Sending Throne entry - %s \n", mn.addr.ToString());
                if(vin == CTxIn() || vin == mn.vin){
                    CThroneBroadcast mnb = CThroneBroadcast(mn);
                    uint256 hash = mnb.GetHash();
                    pfrom->PushInventory(CInv(MSG_THRONE_ANNOUNCE, hash));
                    nInvCount++;

                    if(!mapSeenThroneBroadcast.count(hash)) mapSeenThroneBroadcast.insert(make_pair(hash, mnb));

                    if(vin == mn.vin) {
                        LogPrintf("dseg - Sent 1 Throne entries to %s\n", pfrom->addr.ToString());
                        return;
                    }
                }
            }
        }

        if(vin == CTxIn()) {
            pfrom->PushMessage("ssc", THRONE_SYNC_LIST, nInvCount);
            LogPrintf("dseg - Sent %d Throne entries to %s\n", nInvCount, pfrom->addr.ToString());
        }
    }

}

void CThroneMan::Remove(CTxIn vin)
{
    LOCK(cs);

    vector<CThrone>::iterator it = vThrones.begin();
    while(it != vThrones.end()){
        if((*it).vin == vin){
            LogPrint("throne", "CThroneMan: Removing Throne %s - %i now\n", (*it).addr.ToString(), size() - 1);
            vThrones.erase(it);
            break;
        }
        ++it;
    }
}

std::string CThroneMan::ToString() const
{
    std::ostringstream info;

    info << "Thrones: " << (int)vThrones.size() <<
            ", peers who asked us for Throne list: " << (int)mAskedUsForThroneList.size() <<
            ", peers we asked for Throne list: " << (int)mWeAskedForThroneList.size() <<
            ", entries in Throne list we asked for: " << (int)mWeAskedForThroneListEntry.size() <<
            ", nDsqCount: " << (int)nDsqCount;

    return info.str();
}

void CThroneMan::UpdateThroneList(CThroneBroadcast mnb) {
    mapSeenThronePing.insert(make_pair(mnb.lastPing.GetHash(), mnb.lastPing));
    mapSeenThroneBroadcast.insert(make_pair(mnb.GetHash(), mnb));
    throneSync.AddedThroneList(mnb.GetHash());

    LogPrintf("CThroneMan::UpdateThroneList() - addr: %s\n    vin: %s\n", mnb.addr.ToString(), mnb.vin.ToString());

    CThrone* pmn = Find(mnb.vin);
    if(pmn == NULL)
    {
        CThrone mn(mnb);
        Add(mn);
    } else {
        pmn->UpdateFromNewBroadcast(mnb);
    }
}

bool CThroneMan::CheckMnbAndUpdateThroneList(CThroneBroadcast mnb, int& nDos) {
    nDos = 0;
    LogPrint("throne", "CThroneMan::CheckMnbAndUpdateThroneList - Throne broadcast, vin: %s\n", mnb.vin.ToString());

    if(mapSeenThroneBroadcast.count(mnb.GetHash())) { //seen
        throneSync.AddedThroneList(mnb.GetHash());
        return true;
    }
    mapSeenThroneBroadcast.insert(make_pair(mnb.GetHash(), mnb));

    LogPrint("throne", "CThroneMan::CheckMnbAndUpdateThroneList - Throne broadcast, vin: %s new\n", mnb.vin.ToString());

    if(!mnb.CheckAndUpdate(nDos)){
        LogPrint("throne", "CThroneMan::CheckMnbAndUpdateThroneList - Throne broadcast, vin: %s CheckAndUpdate failed\n", mnb.vin.ToString());
        return false;
    }

    // make sure the vout that was signed is related to the transaction that spawned the Throne
    //  - this is expensive, so it's only done once per Throne
    if(!darkSendSigner.IsVinAssociatedWithPubkey(mnb.vin, mnb.pubkey)) {
        LogPrintf("CThroneMan::CheckMnbAndUpdateThroneList - Got mismatched pubkey and vin\n");
        nDos = 33;
        return false;
    }

    // make sure it's still unspent
    //  - this is checked later by .check() in many places and by ThreadCheckDarkSendPool()
    if(mnb.CheckInputsAndAdd(nDos)) {
        throneSync.AddedThroneList(mnb.GetHash());
    } else {
        LogPrintf("CThroneMan::CheckMnbAndUpdateThroneList - Rejected Throne entry %s\n", mnb.addr.ToString());
        return false;
    }

    return true;
}