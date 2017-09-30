// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "servicenodeman.h"
#include "servicenode-sync.h"
#include "darksend.h"
#include "util.h"
#include "addrman.h"
#include "spork.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

/** Servicenode manager */
CServicenodeMan snodeman;

//
// CServicenodeDB
//

CServicenodeDB::CServicenodeDB()
{
    pathSN = GetDataDir() / "sncache.dat";
    strMagicMessage = "ServicenodeCache";
}

bool CServicenodeDB::Write(const CServicenodeMan& snodemanToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssServicenodes(SER_DISK, CLIENT_VERSION);
    ssServicenodes << strMagicMessage; // servicenode cache file specific magic message
    ssServicenodes << FLATDATA(Params().MessageStart()); // network specific magic number
    ssServicenodes << snodemanToSave;
    uint256 hash = Hash(ssServicenodes.begin(), ssServicenodes.end());
    ssServicenodes << hash;

    // open output file, and associate with CAutoFile
    FILE *file = fopen(pathSN.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s : Failed to open file %s", __func__, pathSN.string());

    // Write and commit header, data
    try {
        fileout << ssServicenodes;
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

CServicenodeDB::ReadResult CServicenodeDB::Read(CServicenodeMan& snodemanToLoad, bool fDryRun)
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

    CDataStream ssServicenodes(vchData, SER_DISK, CLIENT_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssServicenodes.begin(), ssServicenodes.end());
    if (hashIn != hashTmp)
    {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }

    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header (servicenode cache file specific magic message) and ..

        ssServicenodes >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp)
        {
            error("%s : Invalid servicenode cache magic message", __func__);
            return IncorrectMagicMessage;
        }

        // de-serialize file header (network specific magic number) and ..
        ssServicenodes >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
        {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }
        // de-serialize data into CServicenodeMan object
        ssServicenodes >> snodemanToLoad;
    }
    catch (std::exception &e) {
        snodemanToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    LogPrintf("Loaded info from sncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", snodemanToLoad.ToString());
    if(!fDryRun) {
        LogPrintf("Servicenode manager - cleaning....\n");
        snodemanToLoad.CheckAndRemove(true);
        LogPrintf("Servicenode manager - result:\n");
        LogPrintf("  %s\n", snodemanToLoad.ToString());
    }

    return Ok;
}

void DumpServicenodes()
{
    int64_t nStart = GetTimeMillis();

    CServicenodeDB sndb;
    CServicenodeMan tempMnodeman;

    LogPrintf("Verifying sncache.dat format...\n");
    CServicenodeDB::ReadResult readResult = sndb.Read(tempMnodeman, true);
    // there was an error and it was not an error on file opening => do not proceed
    if (readResult == CServicenodeDB::FileError)
        LogPrintf("Missing servicenode cache file - sncache.dat, will try to recreate\n");
    else if (readResult != CServicenodeDB::Ok)
    {
        LogPrintf("Error reading sncache.dat: ");
        if(readResult == CServicenodeDB::IncorrectFormat)
            LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
        else
        {
            LogPrintf("file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrintf("Writting info to sncache.dat...\n");
    sndb.Write(snodeman);

    LogPrintf("Servicenode dump finished  %dms\n", GetTimeMillis() - nStart);
}


void CServicenodeMan::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; //disable all Darksend/Servicenode related functionality
    if(!servicenodeSync.IsBlockchainSynced()) return;

    LOCK(cs_process_message);

    if (strCommand == "snb") { //Servicenode Broadcast
        LogPrint("servicenode", "CServicenodeMan::ProcessMessage - snb");
        CServicenodeBroadcast snb;
        vRecv >> snb;

        int nDoS = 0;
        if (CheckSnbAndUpdateServicenodeList(snb, nDoS)) {
            // use announced Servicenode as a peer
             addrman.Add(CAddress(snb.addr), pfrom->addr, 2*60*60);
        } else {
            if(nDoS > 0) Misbehaving(pfrom->GetId(), nDoS);
        }
    }
}

bool CServicenodeMan::CheckSnbAndUpdateServicenodeList(CServicenodeBroadcast snb, int& nDos)
{
    nDos = 0;
    LogPrint("servicenode", "CServicenodeMan::CheckSnbAndUpdateServicenodeList - Servicenode broadcast, vin: %s\n", snb.vin.ToString());

    if(mapSeenServicenodeBroadcast.count(snb.GetHash())) { //seen
        servicenodeSync.AddedServicenodeList(snb.GetHash());
        return true;
    }
    mapSeenServicenodeBroadcast.insert(make_pair(snb.GetHash(), snb));

    LogPrint("servicenode", "CServicenodeMan::CheckSnbAndUpdateServicenodeList - Servicenode broadcast, vin: %s new\n", snb.vin.ToString());
    // We check addr before both initial snb and update
    if(!snb.IsValidNetAddr()) {
        LogPrintf("CMasternodeBroadcast::CheckSnbAndUpdateMasternodeList -- Invalid addr, rejected: masternode=%s  sigTime=%lld  addr=%s\n",
                    snb.vin.prevout.ToStringShort(), snb.sigTime, snb.addr.ToString());
        return false;
    }

    if(!snb.CheckAndUpdate(nDos)) {
        LogPrint("servicenode", "CServicenodeMan::CheckSnbAndUpdateServicenodeList - Servicenode broadcast, vin: %s CheckAndUpdate failed\n", snb.vin.ToString());
        return false;
    }

    // make sure the vout that was signed is related to the transaction that spawned the Servicenode
    //  - this is expensive, so it's only done once per Servicenode
    if(!darkSendSigner.IsVinAssociatedWithPubkey(snb.vin, snb.pubkey)) {
        LogPrintf("CServicenodeMan::CheckSnbAndUpdateServicenodeList - Got mismatched pubkey and vin\n");
        nDos = 33;
        return false;
    }

    // make sure it's still unspent
    //  - this is checked later by .check() in many places and by ThreadCheckDarkSendPool()
    if(snb.CheckInputsAndAdd(nDos)) {
        servicenodeSync.AddedServicenodeList(snb.GetHash());
    } else {
        LogPrintf("CServicenodeMan::CheckSnbAndUpdateServicenodeList - Rejected Servicenode entry %s\n", snb.addr.ToString());
        return false;
    }

    return true;
}

CServicenode *CServicenodeMan::Find(const CTxIn &vin)
{
    LOCK(cs);

    BOOST_FOREACH(CServicenode& sn, vServicenodes)
    {
        if(sn.vin.prevout == vin.prevout)
            return &sn;
    }
    return NULL;
}

bool CServicenodeMan::Add(CServicenode &sn)
{
    LOCK(cs);

    if (!sn.IsEnabled())
        return false;

    CServicenode *psn = Find(sn.vin);
    if (psn == NULL)
    {
        LogPrint("servicenode", "CServicenodeMan: Adding new Servicenode %s - %i now\n", sn.addr.ToString(), size() + 1);
        vServicenodes.push_back(sn);
        return true;
    }

    return false;
}

void CServicenodeMan::UpdateServicenodeList(CServicenodeBroadcast snb) {
    mapSeenServicenodePing.insert(make_pair(snb.lastPing.GetHash(), snb.lastPing));
    mapSeenServicenodeBroadcast.insert(make_pair(snb.GetHash(), snb));
    servicenodeSync.AddedServicenodeList(snb.GetHash());

    LogPrintf("CServicenodeMan::UpdateServicenodeList() - addr: %s\n    vin: %s\n", snb.addr.ToString(), snb.vin.ToString());

    CServicenode* psn = Find(snb.vin);
    if(psn == NULL)
    {
        CServicenode sn(snb);
        Add(sn);
    } else {
        psn->UpdateFromNewBroadcast(snb);
    }
}

void CServicenodeMan::Remove(CTxIn vin)
{
    LOCK(cs);

    vector<CServicenode>::iterator it = vServicenodes.begin();
    while(it != vServicenodes.end()){
        if((*it).vin == vin){
            LogPrint("servicenode", "CServicenodeMan: Removing Servicenode %s - %i now\n", (*it).addr.ToString(), size() - 1);
            vServicenodes.erase(it);
            break;
        }
        ++it;
    }
}

// TODO remove this later
int GetMinServicenodePaymentsProto() {
    return IsSporkActive(SPORK_10_THRONE_PAY_UPDATED_NODES)
                         ? MIN_THRONE_PAYMENT_PROTO_VERSION_2
                         : MIN_THRONE_PAYMENT_PROTO_VERSION_1;
}


int CServicenodeMan::CountEnabled(int protocolVersion)
{
    int i = 0;
    protocolVersion = protocolVersion == -1 ? GetMinServicenodePaymentsProto() : protocolVersion;

    BOOST_FOREACH(CServicenode& sn, vServicenodes) {
        sn.Check();
        if(sn.protocolVersion < protocolVersion || !sn.IsEnabled()) continue;
        i++;
    }

    return i;
}

void CServicenodeMan::DsegUpdate(CNode* pnode)
{
    LOCK(cs);

    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(!(pnode->addr.IsRFC1918() || pnode->addr.IsLocal())){
            std::map<CNetAddr, int64_t>::iterator it = mWeAskedForServicenodeList.find(pnode->addr);
            if (it != mWeAskedForServicenodeList.end())
            {
                if (GetTime() < (*it).second) {
                    LogPrintf("dseg - we already asked %s for the list; skipping...\n", pnode->addr.ToString());
                    return;
                }
            }
        }
    }
    
    pnode->PushMessage("dseg", CTxIn());
    int64_t askAgain = GetTime() + SERVICENODES_DSEG_SECONDS;
    mWeAskedForServicenodeList[pnode->addr] = askAgain;
}

std::string CServicenodeMan::ToString() const
{
    std::ostringstream info;

    info << "Servicenodes: " << (int)vServicenodes.size() <<
            ", peers who asked us for Servicenode list: " << (int)mAskedUsForServicenodeList.size() <<
            ", peers we asked for Servicenode list: " << (int)mWeAskedForServicenodeList.size() <<
            ", entries in Servicenode list we asked for: " << (int)mWeAskedForServicenodeListEntry.size();

    return info.str();
}

void CServicenodeMan::Clear()
{
    LOCK(cs);
    vServicenodes.clear();
    mAskedUsForServicenodeList.clear();
    mWeAskedForServicenodeList.clear();
    mWeAskedForServicenodeListEntry.clear();
    mapSeenServicenodeBroadcast.clear();
    mapSeenServicenodePing.clear();
}

void CServicenodeMan::Check()
{
    LOCK(cs);

    BOOST_FOREACH(CServicenode& sn, vServicenodes) {
        sn.Check();
    }
}

void CServicenodeMan::CheckAndRemove(bool forceExpiredRemoval)
{
    Check();

    LOCK(cs);

    //remove inactive and outdated
    vector<CServicenode>::iterator it = vServicenodes.begin();
    while(it != vServicenodes.end()){
        if((*it).activeState == CServicenode::SERVICENODE_REMOVE ||
                (*it).activeState == CServicenode::SERVICENODE_VIN_SPENT ||
                (forceExpiredRemoval && (*it).activeState == CServicenode::SERVICENODE_EXPIRED) ||
                (*it).protocolVersion < GetMinServicenodePaymentsProto()) {
            LogPrint("servicenode", "CServicenodeMan: Removing inactive Servicenode %s - %i now\n", (*it).addr.ToString(), size() - 1);

            //erase all of the broadcasts we've seen from this vin
            // -- if we missed a few pings and the node was removed, this will allow is to get it back without them 
            //    sending a brand new snb
            map<uint256, CServicenodeBroadcast>::iterator it3 = mapSeenServicenodeBroadcast.begin();
            while(it3 != mapSeenServicenodeBroadcast.end()){
                if((*it3).second.vin == (*it).vin){
                    servicenodeSync.mapSeenSyncMNB.erase((*it3).first);
                    mapSeenServicenodeBroadcast.erase(it3++);
                } else {
                    ++it3;
                }
            }

            // allow us to ask for this servicenode again if we see another ping
            map<COutPoint, int64_t>::iterator it2 = mWeAskedForServicenodeListEntry.begin();
            while(it2 != mWeAskedForServicenodeListEntry.end()){
                if((*it2).first == (*it).vin.prevout){
                    mWeAskedForServicenodeListEntry.erase(it2++);
                } else {
                    ++it2;
                }
            }

            it = vServicenodes.erase(it);
        } else {
            ++it;
        }
    }

    // check who's asked for the Servicenode list
    map<CNetAddr, int64_t>::iterator it1 = mAskedUsForServicenodeList.begin();
    while(it1 != mAskedUsForServicenodeList.end()){
        if((*it1).second < GetTime()) {
            mAskedUsForServicenodeList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check who we asked for the Servicenode list
    it1 = mWeAskedForServicenodeList.begin();
    while(it1 != mWeAskedForServicenodeList.end()){
        if((*it1).second < GetTime()){
            mWeAskedForServicenodeList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check which Servicenodes we've asked for
    map<COutPoint, int64_t>::iterator it2 = mWeAskedForServicenodeListEntry.begin();
    while(it2 != mWeAskedForServicenodeListEntry.end()){
        if((*it2).second < GetTime()){
            mWeAskedForServicenodeListEntry.erase(it2++);
        } else {
            ++it2;
        }
    }

    // remove expired mapSeenServicenodeBroadcast
    map<uint256, CServicenodeBroadcast>::iterator it3 = mapSeenServicenodeBroadcast.begin();
    while(it3 != mapSeenServicenodeBroadcast.end()){
        if((*it3).second.lastPing.sigTime < GetTime() - SERVICENODE_REMOVAL_SECONDS*2){
            LogPrint("servicenode", "CServicenodeMan::CheckAndRemove - Removing expired Servicenode broadcast %s\n", (*it3).second.GetHash().ToString());
            servicenodeSync.mapSeenSyncMNB.erase((*it3).second.GetHash());
            mapSeenServicenodeBroadcast.erase(it3++);
        } else {
            ++it3;
        }
    }

    // remove expired mapSeenServicenodePing
    map<uint256, CServicenodePing>::iterator it4 = mapSeenServicenodePing.begin();
    while(it4 != mapSeenServicenodePing.end()){
        if((*it4).second.sigTime < GetTime()-(SERVICENODE_REMOVAL_SECONDS*2)){
            mapSeenServicenodePing.erase(it4++);
        } else {
            ++it4;
        }
    }

}

