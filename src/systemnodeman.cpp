// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "systemnodeman.h"
#include "systemnode-sync.h"
#include "masternodeman.h"
#include "legacysigner.h"
#include "util.h"
#include "addrman.h"
#include "spork.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

/** Systemnode manager */
CSystemnodeMan snodeman;

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

struct CompareScoreSN
{
    bool operator()(const pair<int64_t, CSystemnode>& t1,
                    const pair<int64_t, CSystemnode>& t2) const
    {
        return t1.first < t2.first;
    }
};

//
// CSystemnodeDB
//

CSystemnodeDB::CSystemnodeDB()
{
    pathSN = GetDataDir() / "sncache.dat";
    strMagicMessage = "SystemnodeCache";
}

bool CSystemnodeDB::Write(const CSystemnodeMan& snodemanToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssSystemnodes(SER_DISK, CLIENT_VERSION);
    ssSystemnodes << strMagicMessage; // systemnode cache file specific magic message
    ssSystemnodes << FLATDATA(Params().MessageStart()); // network specific magic number
    ssSystemnodes << snodemanToSave;
    uint256 hash = Hash(ssSystemnodes.begin(), ssSystemnodes.end());
    ssSystemnodes << hash;

    // open output file, and associate with CAutoFile
    FILE *file = fopen(pathSN.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s : Failed to open file %s", __func__, pathSN.string());

    // Write and commit header, data
    try {
        fileout << ssSystemnodes;
    }
    catch (std::exception &e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
//    FileCommit(fileout);
    fileout.fclose();

    LogPrintf("Written info to sncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", snodemanToSave.ToString());

    return true;
}

CSystemnodeDB::ReadResult CSystemnodeDB::Read(CSystemnodeMan& snodemanToLoad, bool fDryRun)
{
    int64_t nStart = GetTimeMillis();
    // open input file, and associate with CAutoFile
    FILE *file = fopen(pathSN.string().c_str(), "rb");
    CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
    if (filein.IsNull())
    {
        error("%s : Failed to open file %s", __func__, pathSN.string());
        return FileError;
    }

    // use file size to size memory buffer
    int fileSize = boost::filesystem::file_size(pathSN);
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

    CDataStream ssSystemnodes(vchData, SER_DISK, CLIENT_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssSystemnodes.begin(), ssSystemnodes.end());
    if (hashIn != hashTmp)
    {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }

    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header (systemnode cache file specific magic message) and ..

        ssSystemnodes >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp)
        {
            error("%s : Invalid systemnode cache magic message", __func__);
            return IncorrectMagicMessage;
        }

        // de-serialize file header (network specific magic number) and ..
        ssSystemnodes >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
        {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }
        // de-serialize data into CSystemnodeMan object
        ssSystemnodes >> snodemanToLoad;
    }
    catch (std::exception &e) {
        snodemanToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    LogPrintf("Loaded info from sncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", snodemanToLoad.ToString());
    if(!fDryRun) {
        LogPrintf("Systemnode manager - cleaning....\n");
        snodemanToLoad.CheckAndRemove(true);
        LogPrintf("Systemnode manager - result:\n");
        LogPrintf("  %s\n", snodemanToLoad.ToString());
    }

    return Ok;
}

std::vector<pair<int, CSystemnode> > CSystemnodeMan::GetSystemnodeRanks(int64_t nBlockHeight, int minProtocol)
{
    std::vector<pair<int64_t, CSystemnode> > vecSystemnodeScores;
    std::vector<pair<int, CSystemnode> > vecSystemnodeRanks;

    //make sure we know about this block
    uint256 hash = uint256();
    if(!GetBlockHash(hash, nBlockHeight)) return vecSystemnodeRanks;

    // scan for winner
    BOOST_FOREACH(CSystemnode& mn, vSystemnodes) {

        mn.Check();

        if(mn.protocolVersion < minProtocol) continue;
        if(!mn.IsEnabled()) {
            continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        int64_t n2 = UintToArith256(n).GetCompact(false);

        vecSystemnodeScores.push_back(make_pair(n2, mn));
    }

    sort(vecSystemnodeScores.rbegin(), vecSystemnodeScores.rend(), CompareScoreSN());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CSystemnode)& s, vecSystemnodeScores){
        rank++;
        vecSystemnodeRanks.push_back(make_pair(rank, s.second));
    }

    return vecSystemnodeRanks;
}

void CSystemnodeMan::ProcessSystemnodeConnections()
{
    //we don't care about this for regtest
    if(Params().NetworkID() == CBaseChainParams::REGTEST) return;

    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes) {
        if(pnode->fSystemnode){
            if(legacySigner.pSubmittedToSystemnode != NULL && pnode->addr == legacySigner.pSubmittedToSystemnode->addr) continue;
            LogPrintf("Closing Systemnode connection %s \n", pnode->addr.ToString());
            pnode->fSystemnode = false;
            pnode->Release();
        }
    }
}

void CSystemnodeMan::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; //disable all Systemnode related functionality
    if(!systemnodeSync.IsBlockchainSynced()) return;

    LOCK(cs_process_message);

    if (strCommand == "snb") { //Systemnode Broadcast
        LogPrint("systemnode", "CSystemnodeMan::ProcessMessage - snb");
        CSystemnodeBroadcast snb;
        vRecv >> snb;

        int nDoS = 0;
        if (CheckSnbAndUpdateSystemnodeList(snb, nDoS)) {
            // use announced Systemnode as a peer
            addrman.Add(CAddress(snb.addr), pfrom->addr, 2*60*60);
        } else {
            if(nDoS > 0) Misbehaving(pfrom->GetId(), nDoS);
        }
    } else if (strCommand == "snp") { //Systemnode Ping
        CSystemnodePing snp;
        vRecv >> snp;

        LogPrint("systemnode", "snp - Systemnode ping, vin: %s\n", snp.vin.ToString());

        if(mapSeenSystemnodePing.count(snp.GetHash())) return; //seen
        mapSeenSystemnodePing.insert(make_pair(snp.GetHash(), snp));

        int nDoS = 0;
        if(snp.CheckAndUpdate(nDoS)) return;

        if(nDoS > 0) {
            // if anything significant failed, mark that node
            Misbehaving(pfrom->GetId(), nDoS);
        } else {
            // if nothing significant failed, search existing Systemnode list
            CSystemnode* pmn = Find(snp.vin);
            // if it's known, don't ask for the mnb, just return
            if(pmn != NULL) return;
        }

        // something significant is broken or mn is unknown,
        // we might have to ask for a systemnode entry once
        AskForSN(pfrom, snp.vin);

    } 
    else if (strCommand == "sndseg") { //Get Systemnode list or specific entry
        CTxIn vin;
        vRecv >> vin;

        if(vin == CTxIn()) { //only should ask for this once
            //local network
            bool isLocal = (pfrom->addr.IsRFC1918() || pfrom->addr.IsLocal());

            if(!isLocal && Params().NetworkID() == CBaseChainParams::MAIN) {
                std::map<CNetAddr, int64_t>::iterator i = mAskedUsForSystemnodeList.find(pfrom->addr);
                if (i != mAskedUsForSystemnodeList.end()){
                    int64_t t = (*i).second;
                    if (GetTime() < t) {
                        Misbehaving(pfrom->GetId(), 34);
                        LogPrintf("sndseg - peer already asked me for the list\n");
                        return;
                    }
                }
                int64_t askAgain = GetTime() + SYSTEMNODES_DSEG_SECONDS;
                mAskedUsForSystemnodeList[pfrom->addr] = askAgain;
            }
        } //else, asking for a specific node which is ok


        int nInvCount = 0;

        BOOST_FOREACH(CSystemnode& sn, vSystemnodes) {
            if(sn.addr.IsRFC1918()) continue; //local network

            if(sn.IsEnabled()) {
                LogPrint("systemnode", "sndseg - Sending Systemnode entry - %s \n", sn.addr.ToString());
                if(vin == CTxIn() || vin == sn.vin){
                    CSystemnodeBroadcast snb = CSystemnodeBroadcast(sn);
                    uint256 hash = snb.GetHash();
                    pfrom->PushInventory(CInv(MSG_SYSTEMNODE_ANNOUNCE, hash));
                    nInvCount++;

                    if(!mapSeenSystemnodeBroadcast.count(hash)) mapSeenSystemnodeBroadcast.insert(make_pair(hash, snb));

                    if(vin == sn.vin) {
                        LogPrintf("sndseg - Sent 1 Systemnode entries to %s\n", pfrom->addr.ToString());
                        return;
                    }
                }
            }
        }

        if(vin == CTxIn()) {
            pfrom->PushMessage("snssc", SYSTEMNODE_SYNC_LIST, nInvCount);
            LogPrintf("sndseg - Sent %d Systemnode entries to %s\n", nInvCount, pfrom->addr.ToString());
        }
    }

}

void CSystemnodeMan::AskForSN(CNode* pnode, CTxIn &vin)
{
    std::map<COutPoint, int64_t>::iterator i = mWeAskedForSystemnodeListEntry.find(vin.prevout);
    if (i != mWeAskedForSystemnodeListEntry.end())
    {
        int64_t t = (*i).second;
        if (GetTime() < t) return; // we've asked recently
    }

    // ask for the snb info once from the node that sent snp

    LogPrintf("CSystemnodeMan::AskForSN - Asking node for missing entry, vin: %s\n", vin.ToString());
    pnode->PushMessage("sndseg", vin);
    int64_t askAgain = GetTime() + SYSTEMNODE_MIN_SNP_SECONDS;
    mWeAskedForSystemnodeListEntry[vin.prevout] = askAgain;
}

bool CSystemnodeMan::CheckSnbAndUpdateSystemnodeList(CSystemnodeBroadcast snb, int& nDos)
{
    nDos = 0;
    LogPrint("systemnode", "CSystemnodeMan::CheckSnbAndUpdateSystemnodeList - Systemnode broadcast, vin: %s\n", snb.vin.ToString());

    if(mapSeenSystemnodeBroadcast.count(snb.GetHash())) { //seen
        systemnodeSync.AddedSystemnodeList(snb.GetHash());
        return true;
    }
    mapSeenSystemnodeBroadcast.insert(make_pair(snb.GetHash(), snb));

    LogPrint("systemnode", "CSystemnodeMan::CheckSnbAndUpdateSystemnodeList - Systemnode broadcast, vin: %s new\n", snb.vin.ToString());
    // We check addr before both initial snb and update
    if(!snb.IsValidNetAddr()) {
        LogPrintf("CMasternodeBroadcast::CheckSnbAndUpdateMasternodeList -- Invalid addr, rejected: masternode=%s  sigTime=%lld  addr=%s\n",
                    snb.vin.prevout.ToStringShort(), snb.sigTime, snb.addr.ToString());
        return false;
    }

    if (mnodeman.Find(snb.addr) != NULL) {
        LogPrintf("CSystemnodeMan::CheckSnbAndUpdateSystemnodeList - There is already a masternode with the same ip: %s\n", snb.addr.ToString());
        return false;
    }

    if(!snb.CheckAndUpdate(nDos)) {
        LogPrint("systemnode", "CSystemnodeMan::CheckSnbAndUpdateSystemnodeList - Systemnode broadcast, vin: %s CheckAndUpdate failed\n", snb.vin.ToString());
        return false;
    }

    // make sure the vout that was signed is related to the transaction that spawned the Systemnode
    //  - this is expensive, so it's only done once per Systemnode
    if(!legacySigner.IsVinAssociatedWithPubkey(snb.vin, snb.pubkey, SYSTEMNODE_COLLATERAL)) {
        LogPrintf("CSystemnodeMan::CheckSnbAndUpdateSystemnodeList - Got mismatched pubkey and vin\n");
        nDos = 33;
        return false;
    }

    // make sure it's still unspent
    //  - this is checked later by .check() in many places and by ThreadCheckDarkSendPool()
    if(snb.CheckInputsAndAdd(nDos)) {
        systemnodeSync.AddedSystemnodeList(snb.GetHash());
    } else {
        LogPrintf("CSystemnodeMan::CheckSnbAndUpdateSystemnodeList - Rejected Systemnode entry %s\n", snb.addr.ToString());
        return false;
    }

    return true;
}

CSystemnode *CSystemnodeMan::Find(const CTxIn &vin)
{
    LOCK(cs);

    BOOST_FOREACH(CSystemnode& sn, vSystemnodes)
    {
        if(sn.vin.prevout == vin.prevout)
            return &sn;
    }
    return NULL;
}

CSystemnode *CSystemnodeMan::Find(const CPubKey &pubKeySystemnode)
{
    LOCK(cs);

    BOOST_FOREACH(CSystemnode& mn, vSystemnodes)
    {
        if(mn.pubkey2 == pubKeySystemnode)
            return &mn;
    }
    return NULL;
}

CSystemnode *CSystemnodeMan::Find(const CService& addr)
{
    LOCK(cs);

    BOOST_FOREACH(CSystemnode& sn, vSystemnodes)
    {
        if(sn.addr == addr)
            return &sn;
    }
    return NULL;
}

// 
// Deterministically select the oldest/best systemnode to pay on the network
//
CSystemnode* CSystemnodeMan::GetNextSystemnodeInQueueForPayment(int nBlockHeight, bool fFilterSigTime, int& nCount)
{
    LOCK(cs);

    CSystemnode *pBestSystemnode = NULL;
    std::vector<pair<int64_t, CTxIn> > vecSystemnodeLastPaid;

    /*
        Make a vector with all of the last paid times
    */

    int nSnCount = CountEnabled();
    BOOST_FOREACH(CSystemnode &sn, vSystemnodes)
    {
        sn.Check();
        if(!sn.IsEnabled()) continue;

        // //check protocol version
        if(sn.protocolVersion < MIN_SYSTEMNODE_PAYMENT_PROTO_VERSION) continue;

        //it's in the list (up to 8 entries ahead of current block to allow propagation) -- so let's skip it
        if(systemnodePayments.IsScheduled(sn, nBlockHeight)) continue;

        //it's too new, wait for a cycle
        if(fFilterSigTime && sn.sigTime + (nSnCount*1*60) > GetAdjustedTime()) continue;

        //make sure it has as many confirmations as there are systemnodes
        if(sn.GetSystemnodeInputAge() < nSnCount) continue;

        vecSystemnodeLastPaid.push_back(make_pair(sn.SecondsSincePayment(), sn.vin));
    }

    nCount = (int)vecSystemnodeLastPaid.size();

    //when the network is in the process of upgrading, don't penalize nodes that recently restarted
    if(fFilterSigTime && nCount < nSnCount/3) return GetNextSystemnodeInQueueForPayment(nBlockHeight, false, nCount);

    // Sort them high to low
    sort(vecSystemnodeLastPaid.rbegin(), vecSystemnodeLastPaid.rend(), CompareLastPaid());

    // Look at 1/10 of the oldest nodes (by last payment), calculate their scores and pay the best one
    //  -- This doesn't look at who is being paid in the +8-10 blocks, allowing for double payments very rarely
    //  -- 1/100 payments should be a double payment on mainnet - (1/(3000/10))*2
    //  -- (chance per block * chances before IsScheduled will fire)
    int nTenthNetwork = CountEnabled()/10;
    int nCountTenth = 0; 
    arith_uint256 nHigh = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CTxIn)& s, vecSystemnodeLastPaid) {
        CSystemnode* pmn = Find(s.second);
        if(!pmn) break;

        arith_uint256 n = UintToArith256(pmn->CalculateScore(1, nBlockHeight-100));
        if(n > nHigh){
            nHigh = n;
            pBestSystemnode = pmn;
        }
        nCountTenth++;
        if(nCountTenth >= nTenthNetwork) break;
    }
    return pBestSystemnode;
}

void DumpSystemnodes()
{
    int64_t nStart = GetTimeMillis();

    CSystemnodeDB sndb;
    CSystemnodeMan tempMnodeman;

    LogPrintf("Verifying sncache.dat format...\n");
    CSystemnodeDB::ReadResult readResult = sndb.Read(tempMnodeman, true);
    // there was an error and it was not an error on file opening => do not proceed
    if (readResult == CSystemnodeDB::FileError)
        LogPrintf("Missing throne cache file - sncache.dat, will try to recreate\n");
    else if (readResult != CSystemnodeDB::Ok)
    {
        LogPrintf("Error reading sncache.dat: ");
        if(readResult == CSystemnodeDB::IncorrectFormat)
            LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
        else
        {
            LogPrintf("file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrintf("Writting info to sncache.dat...\n");
    sndb.Write(snodeman);

    LogPrintf("Systemnode dump finished  %dms\n", GetTimeMillis() - nStart);
}

bool CSystemnodeMan::Add(CSystemnode &sn)
{
    LOCK(cs);

    if (!sn.IsEnabled())
        return false;

    CSystemnode *psn = Find(sn.vin);
    if (psn == NULL)
    {
        LogPrint("systemnode", "CSystemnodeMan: Adding new Systemnode %s - %i now\n", sn.addr.ToString(), size() + 1);
        vSystemnodes.push_back(sn);
        return true;
    }

    return false;
}

void CSystemnodeMan::UpdateSystemnodeList(CSystemnodeBroadcast snb) {
    mapSeenSystemnodePing.insert(make_pair(snb.lastPing.GetHash(), snb.lastPing));
    mapSeenSystemnodeBroadcast.insert(make_pair(snb.GetHash(), snb));
    systemnodeSync.AddedSystemnodeList(snb.GetHash());

    LogPrintf("CSystemnodeMan::UpdateSystemnodeList() - addr: %s\n    vin: %s\n", snb.addr.ToString(), snb.vin.ToString());

    CSystemnode* psn = Find(snb.vin);
    if(psn == NULL)
    {
        CSystemnode sn(snb);
        Add(sn);
    } else {
        psn->UpdateFromNewBroadcast(snb);
    }
}

void CSystemnodeMan::Remove(CTxIn vin)
{
    LOCK(cs);

    vector<CSystemnode>::iterator it = vSystemnodes.begin();
    while(it != vSystemnodes.end()){
        if((*it).vin == vin){
            LogPrint("systemnode", "CSystemnodeMan: Removing Systemnode %s - %i now\n", (*it).addr.ToString(), size() - 1);
            vSystemnodes.erase(it);
            break;
        }
        ++it;
    }
}

int CSystemnodeMan::CountEnabled(int protocolVersion)
{
    int i = 0;
    protocolVersion = protocolVersion == -1 ? MIN_SYSTEMNODE_PAYMENT_PROTO_VERSION : protocolVersion;

    BOOST_FOREACH(CSystemnode& sn, vSystemnodes) {
        sn.Check();
        if(sn.protocolVersion < protocolVersion || !sn.IsEnabled()) continue;
        i++;
    }

    return i;
}

void CSystemnodeMan::DsegUpdate(CNode* pnode)
{
    LOCK(cs);

    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(!(pnode->addr.IsRFC1918() || pnode->addr.IsLocal())){
            std::map<CNetAddr, int64_t>::iterator it = mWeAskedForSystemnodeList.find(pnode->addr);
            if (it != mWeAskedForSystemnodeList.end())
            {
                if (GetTime() < (*it).second) {
                    LogPrintf("sndseg - we already asked %s for the list; skipping...\n", pnode->addr.ToString());
                    return;
                }
            }
        }
    }
    
    pnode->PushMessage("sndseg", CTxIn());
    int64_t askAgain = GetTime() + SYSTEMNODES_DSEG_SECONDS;
    mWeAskedForSystemnodeList[pnode->addr] = askAgain;
}

std::string CSystemnodeMan::ToString() const
{
    std::ostringstream info;

    info << "Systemnodes: " << (int)vSystemnodes.size() <<
            ", peers who asked us for Systemnode list: " << (int)mAskedUsForSystemnodeList.size() <<
            ", peers we asked for Systemnode list: " << (int)mWeAskedForSystemnodeList.size() <<
            ", entries in Systemnode list we asked for: " << (int)mWeAskedForSystemnodeListEntry.size();

    return info.str();
}

void CSystemnodeMan::Clear()
{
    LOCK(cs);
    vSystemnodes.clear();
    mAskedUsForSystemnodeList.clear();
    mWeAskedForSystemnodeList.clear();
    mWeAskedForSystemnodeListEntry.clear();
    mapSeenSystemnodeBroadcast.clear();
    mapSeenSystemnodePing.clear();
}

void CSystemnodeMan::Check()
{
    LOCK(cs);

    BOOST_FOREACH(CSystemnode& sn, vSystemnodes) {
        sn.Check();
    }
}

CSystemnode* CSystemnodeMan::GetCurrentSystemNode(int mod, int64_t nBlockHeight, int minProtocol)
{
    int64_t score = 0;
    CSystemnode* winner = NULL;

    // scan for winner
    BOOST_FOREACH(CSystemnode& mn, vSystemnodes) {
        mn.Check();
        if(mn.protocolVersion < minProtocol || !mn.IsEnabled()) continue;

        // calculate the score for each Systemnode
        uint256 n = mn.CalculateScore(mod, nBlockHeight);
        int64_t n2 = UintToArith256(n).GetCompact(false);

        // determine the winner
        if(n2 > score){
            score = n2;
            winner = &mn;
        }
    }

    return winner;
}

int CSystemnodeMan::GetSystemnodeRank(const CTxIn& vin, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<pair<int64_t, CTxIn> > vecSystemnodeScores;

    //make sure we know about this block
    uint256 hash = uint256();
    if(!GetBlockHash(hash, nBlockHeight)) return -1;

    // scan for winner
    BOOST_FOREACH(CSystemnode& sn, vSystemnodes) {
        if(sn.protocolVersion < minProtocol) continue;
        if(fOnlyActive) {
            sn.Check();
            if(!sn.IsEnabled()) continue;
        }
        uint256 n = sn.CalculateScore(1, nBlockHeight);
        int64_t n2 = UintToArith256(n).GetCompact(false);

        vecSystemnodeScores.push_back(make_pair(n2, sn.vin));
    }

    sort(vecSystemnodeScores.rbegin(), vecSystemnodeScores.rend(), CompareScoreTxIn());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CTxIn)& s, vecSystemnodeScores){
        rank++;
        if(s.second.prevout == vin.prevout) {
            return rank;
        }
    }

    return -1;
}

void CSystemnodeMan::CheckAndRemove(bool forceExpiredRemoval)
{
    Check();

    LOCK(cs);

    //remove inactive and outdated
    vector<CSystemnode>::iterator it = vSystemnodes.begin();
    while(it != vSystemnodes.end()){
        if((*it).activeState == CSystemnode::SYSTEMNODE_REMOVE ||
                (*it).activeState == CSystemnode::SYSTEMNODE_VIN_SPENT ||
                (forceExpiredRemoval && (*it).activeState == CSystemnode::SYSTEMNODE_EXPIRED) ||
                (*it).protocolVersion < MIN_SYSTEMNODE_PAYMENT_PROTO_VERSION) {
            LogPrint("systemnode", "CSystemnodeMan: Removing inactive Systemnode %s - %i now\n", (*it).addr.ToString(), size() - 1);

            //erase all of the broadcasts we've seen from this vin
            // -- if we missed a few pings and the node was removed, this will allow is to get it back without them 
            //    sending a brand new snb
            map<uint256, CSystemnodeBroadcast>::iterator it3 = mapSeenSystemnodeBroadcast.begin();
            while(it3 != mapSeenSystemnodeBroadcast.end()){
                if((*it3).second.vin == (*it).vin){
                    systemnodeSync.mapSeenSyncSNB.erase((*it3).first);
                    mapSeenSystemnodeBroadcast.erase(it3++);
                } else {
                    ++it3;
                }
            }

            // allow us to ask for this systemnode again if we see another ping
            map<COutPoint, int64_t>::iterator it2 = mWeAskedForSystemnodeListEntry.begin();
            while(it2 != mWeAskedForSystemnodeListEntry.end()){
                if((*it2).first == (*it).vin.prevout){
                    mWeAskedForSystemnodeListEntry.erase(it2++);
                } else {
                    ++it2;
                }
            }

            it = vSystemnodes.erase(it);
        } else {
            ++it;
        }
    }

    // check who's asked for the Systemnode list
    map<CNetAddr, int64_t>::iterator it1 = mAskedUsForSystemnodeList.begin();
    while(it1 != mAskedUsForSystemnodeList.end()){
        if((*it1).second < GetTime()) {
            mAskedUsForSystemnodeList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check who we asked for the Systemnode list
    it1 = mWeAskedForSystemnodeList.begin();
    while(it1 != mWeAskedForSystemnodeList.end()){
        if((*it1).second < GetTime()){
            mWeAskedForSystemnodeList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check which Systemnodes we've asked for
    map<COutPoint, int64_t>::iterator it2 = mWeAskedForSystemnodeListEntry.begin();
    while(it2 != mWeAskedForSystemnodeListEntry.end()){
        if((*it2).second < GetTime()){
            mWeAskedForSystemnodeListEntry.erase(it2++);
        } else {
            ++it2;
        }
    }

    // remove expired mapSeenSystemnodeBroadcast
    map<uint256, CSystemnodeBroadcast>::iterator it3 = mapSeenSystemnodeBroadcast.begin();
    while(it3 != mapSeenSystemnodeBroadcast.end()){
        if((*it3).second.lastPing.sigTime < GetTime() - SYSTEMNODE_REMOVAL_SECONDS*2){
            LogPrint("systemnode", "CSystemnodeMan::CheckAndRemove - Removing expired Systemnode broadcast %s\n", (*it3).second.GetHash().ToString());
            systemnodeSync.mapSeenSyncSNB.erase((*it3).second.GetHash());
            mapSeenSystemnodeBroadcast.erase(it3++);
        } else {
            ++it3;
        }
    }

    // remove expired mapSeenSystemnodePing
    map<uint256, CSystemnodePing>::iterator it4 = mapSeenSystemnodePing.begin();
    while(it4 != mapSeenSystemnodePing.end()){
        if((*it4).second.sigTime < GetTime()-(SYSTEMNODE_REMOVAL_SECONDS*2)){
            mapSeenSystemnodePing.erase(it4++);
        } else {
            ++it4;
        }
    }

}
