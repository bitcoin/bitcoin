// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "throneman.h"
#include "throne.h"
#include "activethrone.h"
#include "darksend.h"
#include "core.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

CCriticalSection cs_process_message;

/** Throne manager */
CThroneMan mnodeman;

struct CompareValueOnly
{
    bool operator()(const pair<int64_t, CTxIn>& t1,
                    const pair<int64_t, CTxIn>& t2) const
    {
        return t1.first < t2.first;
    }
};
struct CompareValueOnlyMN
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
    CAutoFile fileout = CAutoFile(file, SER_DISK, CLIENT_VERSION);
    if (!fileout)
        return error("%s : Failed to open file %s", __func__, pathMN.string());

    // Write and commit header, data
    try {
        fileout << ssThrones;
    }
    catch (std::exception &e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
    FileCommit(fileout);
    fileout.fclose();

    LogPrintf("Written info to mncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", mnodemanToSave.ToString());

    return true;
}

CThroneDB::ReadResult CThroneDB::Read(CThroneMan& mnodemanToLoad)
{
    int64_t nStart = GetTimeMillis();
    // open input file, and associate with CAutoFile
    FILE *file = fopen(pathMN.string().c_str(), "rb");
    CAutoFile filein = CAutoFile(file, SER_DISK, CLIENT_VERSION);
    if (!filein)
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

    mnodemanToLoad.CheckAndRemove(); // clean out expired
    LogPrintf("Loaded info from mncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", mnodemanToLoad.ToString());

    return Ok;
}

void DumpThrones()
{
    int64_t nStart = GetTimeMillis();

    CThroneDB mndb;
    CThroneMan tempMnodeman;

    LogPrintf("Verifying mncache.dat format...\n");
    CThroneDB::ReadResult readResult = mndb.Read(tempMnodeman);
    // there was an error and it was not an error on file openning => do not proceed
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
        if(fDebug) LogPrintf("CThroneMan: Adding new Throne %s - %i now\n", mn.addr.ToString().c_str(), size() + 1);
        vThrones.push_back(mn);
        return true;
    }

    return false;
}

void CThroneMan::Check()
{
    LOCK(cs);

    BOOST_FOREACH(CThrone& mn, vThrones)
        mn.Check();
}

void CThroneMan::CheckAndRemove()
{
    LOCK(cs);

    Check();

    //remove inactive
    vector<CThrone>::iterator it = vThrones.begin();
    while(it != vThrones.end()){
        if((*it).activeState == CThrone::THRONE_REMOVE || (*it).activeState == CThrone::THRONE_VIN_SPENT){
            if(fDebug) LogPrintf("CThroneMan: Removing inactive Throne %s - %i now\n", (*it).addr.ToString().c_str(), size() - 1);
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

}

void CThroneMan::Clear()
{
    LOCK(cs);
    vThrones.clear();
    mAskedUsForThroneList.clear();
    mWeAskedForThroneList.clear();
    mWeAskedForThroneListEntry.clear();
    nDsqCount = 0;
}

int CThroneMan::CountEnabled()
{
    int i = 0;

    BOOST_FOREACH(CThrone& mn, vThrones) {
        mn.Check();
        if(mn.IsEnabled()) i++;
    }

    return i;
}

int CThroneMan::CountThronesAboveProtocol(int protocolVersion)
{
    int i = 0;

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

    std::map<CNetAddr, int64_t>::iterator it = mWeAskedForThroneList.find(pnode->addr);
    if (it != mWeAskedForThroneList.end())
    {
        if (GetTime() < (*it).second) {
            LogPrintf("dseg - we already asked %s for the list; skipping...\n", pnode->addr.ToString());
            return;
        }
    }
    pnode->PushMessage("dseg", CTxIn());
    int64_t askAgain = GetTime() + THRONES_DSEG_SECONDS;
    mWeAskedForThroneList[pnode->addr] = askAgain;
}

CThrone *CThroneMan::Find(const CTxIn &vin)
{
    LOCK(cs);

    BOOST_FOREACH(CThrone& mn, vThrones)
    {
        if(mn.vin == vin)
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


CThrone* CThroneMan::FindOldestNotInVec(const std::vector<CTxIn> &vVins, int nMinimumAge, int nMinimumActiveSeconds)
{
    LOCK(cs);

    CThrone *pOldestThrone = NULL;

    BOOST_FOREACH(CThrone &mn, vThrones)
    {
        mn.Check();
        if(!mn.IsEnabled()) continue;

        if(!RegTest()){
            if(mn.GetThroneInputAge() < nMinimumAge || mn.lastTimeSeen - mn.sigTime < nMinimumActiveSeconds) continue;
        }

        bool found = false;
        BOOST_FOREACH(const CTxIn& vin, vVins)
            if(mn.vin == vin)
            {
                found = true;
                break;
            }

        if(found) continue;

        if(pOldestThrone == NULL || pOldestThrone->GetThroneInputAge() < mn.GetThroneInputAge()){
            pOldestThrone = &mn;
        }
    }

    return pOldestThrone;
}

CThrone *CThroneMan::FindRandom()
{
    LOCK(cs);

    if(size() == 0) return NULL;

    return &vThrones[GetRandInt(vThrones.size())];
}

CThrone* CThroneMan::GetCurrentThroNe(int mod, int64_t nBlockHeight, int minProtocol)
{
    unsigned int score = 0;
    CThrone* winner = NULL;

    // scan for winner
    BOOST_FOREACH(CThrone& mn, vThrones) {
        mn.Check();
        if(mn.protocolVersion < minProtocol || !mn.IsEnabled()) continue;

        // calculate the score for each Throne
        uint256 n = mn.CalculateScore(mod, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

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
    std::vector<pair<unsigned int, CTxIn> > vecThroneScores;

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
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecThroneScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecThroneScores.rbegin(), vecThroneScores.rend(), CompareValueOnly());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CTxIn)& s, vecThroneScores){
        rank++;
        if(s.second == vin) {
            return rank;
        }
    }

    return -1;
}

std::vector<pair<int, CThrone> > CThroneMan::GetThroneRanks(int64_t nBlockHeight, int minProtocol)
{
    std::vector<pair<unsigned int, CThrone> > vecThroneScores;
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
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecThroneScores.push_back(make_pair(n2, mn));
    }

    sort(vecThroneScores.rbegin(), vecThroneScores.rend(), CompareValueOnlyMN());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CThrone)& s, vecThroneScores){
        rank++;
        vecThroneRanks.push_back(make_pair(rank, s.second));
    }

    return vecThroneRanks;
}

CThrone* CThroneMan::GetThroneByRank(int nRank, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<pair<unsigned int, CTxIn> > vecThroneScores;

    // scan for winner
    BOOST_FOREACH(CThrone& mn, vThrones) {

        if(mn.protocolVersion < minProtocol) continue;
        if(fOnlyActive) {
            mn.Check();
            if(!mn.IsEnabled()) continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecThroneScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecThroneScores.rbegin(), vecThroneScores.rend(), CompareValueOnly());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CTxIn)& s, vecThroneScores){
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
    if(RegTest()) return;

    LOCK(cs_vNodes);

    if(!darkSendPool.pSubmittedToThrone) return;

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if(darkSendPool.pSubmittedToThrone->addr == pnode->addr) continue;

        if(pnode->fDarkSendMaster){
            LogPrintf("Closing Throne connection %s \n", pnode->addr.ToString().c_str());
            pnode->CloseSocketDisconnect();
        }
    }
}

void CThroneMan::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{

    if(fLiteMode) return; //disable all Darksend/Throne related functionality
    if(IsInitialBlockDownload()) return;

    LOCK(cs_process_message);

    if (strCommand == "dsee") { //DarkSend Election Entry

        CTxIn vin;
        CService addr;
        CPubKey pubkey;
        CPubKey pubkey2;
        vector<unsigned char> vchSig;
        int64_t sigTime;
        int count;
        int current;
        int64_t lastUpdated;
        int protocolVersion;
        std::string strMessage;

        // 70047 and greater
        vRecv >> vin >> addr >> vchSig >> sigTime >> pubkey >> pubkey2 >> count >> current >> lastUpdated >> protocolVersion ;

        // make sure signature isn't in the future (past is OK)
        if (sigTime > GetAdjustedTime() + 60 * 60) {
            LogPrintf("dsee - Signature rejected, too far into the future %s\n", vin.ToString().c_str());
            return;
        }

        bool isLocal = addr.IsRFC1918() || addr.IsLocal();
        if(RegTest()) isLocal = false;

        std::string vchPubKey(pubkey.begin(), pubkey.end());
        std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

        strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion) ;

        if(protocolVersion < nThroneMinProtocol) {
            LogPrintf("dsee - ignoring outdated Throne %s protocol version %d\n", vin.ToString().c_str(), protocolVersion);
            return;
        }

        CScript pubkeyScript;
        pubkeyScript.SetDestination(pubkey.GetID());

        if(pubkeyScript.size() != 25) {
            LogPrintf("dsee - pubkey the wrong size\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        CScript pubkeyScript2;
        pubkeyScript2.SetDestination(pubkey2.GetID());

        if(pubkeyScript2.size() != 25) {
            LogPrintf("dsee - pubkey2 the wrong size\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        std::string errorMessage = "";
        if(!darkSendSigner.VerifyMessage(pubkey, vchSig, strMessage, errorMessage)){
            LogPrintf("dsee - Got bad Throne address signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        if(Params().NetworkID() == CChainParams::MAIN){
            if(addr.GetPort() != 9340) return;
        } else if(addr.GetPort() == 9340) return;

        //search existing Throne list, this is where we update existing Thrones with new dsee broadcasts
        CThrone* pmn = this->Find(vin);
        // if we are throne but with undefined vin and this dsee is ours (matches our Throne privkey) then just skip this part
        if(pmn != NULL && !(fThroNe && activeThrone.vin == CTxIn() && pubkey2 == activeThrone.pubKeyThrone))
        {
            // count == -1 when it's a new entry
            //   e.g. We don't want the entry relayed/time updated when we're syncing the list
            // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
            //   after that they just need to match
            if(count == -1 && pmn->pubkey == pubkey && !pmn->UpdatedWithin(THRONE_MIN_DSEE_SECONDS)){
                pmn->UpdateLastSeen();

                if(pmn->sigTime < sigTime){ //take the newest entry
                    LogPrintf("dsee - Got updated entry for %s\n", addr.ToString().c_str());
                    pmn->pubkey2 = pubkey2;
                    pmn->sigTime = sigTime;
                    pmn->sig = vchSig;
                    pmn->protocolVersion = protocolVersion;
                    pmn->addr = addr;
                    pmn->Check();
                    if(pmn->IsEnabled())
                        mnodeman.RelayThroneEntry(vin, addr, vchSig, sigTime, pubkey, pubkey2, count, current, lastUpdated, protocolVersion);
                }
            }

            return;
        }

        // make sure the vout that was signed is related to the transaction that spawned the Throne
        //  - this is expensive, so it's only done once per Throne
        if(!darkSendSigner.IsVinAssociatedWithPubkey(vin, pubkey)) {
            LogPrintf("dsee - Got mismatched pubkey and vin\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        if(fDebug) LogPrintf("dsee - Got NEW Throne entry %s\n", addr.ToString().c_str());

        // make sure it's still unspent
        //  - this is checked later by .check() in many places and by ThreadCheckDarkSendPool()

        CValidationState state;
        CTransaction tx = CTransaction();
        CTxOut vout = CTxOut(9999.99*COIN, darkSendPool.collateralPubKey);
        tx.vin.push_back(vin);
        tx.vout.push_back(vout);
        if(AcceptableInputs(mempool, state, tx)){
            if(fDebug) LogPrintf("dsee - Accepted Throne entry %i %i\n", count, current);

            if(GetInputAge(vin) < THRONE_MIN_CONFIRMATIONS){
                LogPrintf("dsee - Input must have least %d confirmations\n", THRONE_MIN_CONFIRMATIONS);
                Misbehaving(pfrom->GetId(), 20);
                return;
            }

            // verify that sig time is legit in past
            // should be at least not earlier than block when 1000 DASH tx got THRONE_MIN_CONFIRMATIONS
            uint256 hashBlock = 0;
            GetTransaction(vin.prevout.hash, tx, hashBlock, true);
            map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
            if (mi != mapBlockIndex.end() && (*mi).second)
            {
                CBlockIndex* pMNIndex = (*mi).second; // block for 1000 DASH tx -> 1 confirmation
                CBlockIndex* pConfIndex = chainActive[pMNIndex->nHeight + THRONE_MIN_CONFIRMATIONS - 1]; // block where tx got THRONE_MIN_CONFIRMATIONS
                if(pConfIndex->GetBlockTime() > sigTime)
                {
                    LogPrintf("dsee - Bad sigTime %d for Throne %20s %105s (%i conf block is at %d)\n",
                              sigTime, addr.ToString(), vin.ToString(), THRONE_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
                    return;
                }
            }


            // use this as a peer
            addrman.Add(CAddress(addr), pfrom->addr, 2*60*60);

            //doesn't support multisig addresses

            // add our Throne
            CThrone mn(addr, vin, pubkey, vchSig, sigTime, pubkey2, protocolVersion);
            mn.UpdateLastSeen(lastUpdated);
            this->Add(mn);

            // if it matches our Throne privkey, then we've been remotely activated
            if(pubkey2 == activeThrone.pubKeyThrone && protocolVersion == PROTOCOL_VERSION){
                activeThrone.EnableHotColdThroNe(vin, addr);
            }

            if(count == -1 && !isLocal)
                mnodeman.RelayThroneEntry(vin, addr, vchSig, sigTime, pubkey, pubkey2, count, current, lastUpdated, protocolVersion);

        } else {
            LogPrintf("dsee - Rejected Throne entry %s\n", addr.ToString().c_str());

            int nDoS = 0;
            if (state.IsInvalid(nDoS))
            {
                LogPrintf("dsee - %s from %s %s was not accepted into the memory pool\n", tx.GetHash().ToString().c_str(),
                    pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str());
                if (nDoS > 0)
                    Misbehaving(pfrom->GetId(), nDoS);
            }
        }
    }

    else if (strCommand == "dseep") { //DarkSend Election Entry Ping

        CTxIn vin;
        vector<unsigned char> vchSig;
        int64_t sigTime;
        bool stop;
        vRecv >> vin >> vchSig >> sigTime >> stop;

        //LogPrintf("dseep - Received: vin: %s sigTime: %lld stop: %s\n", vin.ToString().c_str(), sigTime, stop ? "true" : "false");

        if (sigTime > GetAdjustedTime() + 60 * 60) {
            LogPrintf("dseep - Signature rejected, too far into the future %s\n", vin.ToString().c_str());
            return;
        }

        if (sigTime <= GetAdjustedTime() - 60 * 60) {
            LogPrintf("dseep - Signature rejected, too far into the past %s - %d %d \n", vin.ToString().c_str(), sigTime, GetAdjustedTime());
            return;
        }

        // see if we have this Throne
        CThrone* pmn = this->Find(vin);
        if(pmn != NULL && pmn->protocolVersion >= nThroneMinProtocol)
        {
            // LogPrintf("dseep - Found corresponding mn for vin: %s\n", vin.ToString().c_str());
            // take this only if it's newer
            if(pmn->lastDseep < sigTime)
            {
                std::string strMessage = pmn->addr.ToString() + boost::lexical_cast<std::string>(sigTime) + boost::lexical_cast<std::string>(stop);

                std::string errorMessage = "";
                if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage))
                {
                    LogPrintf("dseep - Got bad Throne address signature %s \n", vin.ToString().c_str());
                    //Misbehaving(pfrom->GetId(), 100);
                    return;
                }

                pmn->lastDseep = sigTime;

                if(!pmn->UpdatedWithin(THRONE_MIN_DSEEP_SECONDS))
                {
                    if(stop) pmn->Disable();
                    else
                    {
                        pmn->UpdateLastSeen();
                        pmn->Check();
                        if(!pmn->IsEnabled()) return;
                    }
                    mnodeman.RelayThroneEntryPing(vin, vchSig, sigTime, stop);
                }
            }
            return;
        }

        if(fDebug) LogPrintf("dseep - Couldn't find Throne entry %s\n", vin.ToString().c_str());

        std::map<COutPoint, int64_t>::iterator i = mWeAskedForThroneListEntry.find(vin.prevout);
        if (i != mWeAskedForThroneListEntry.end())
        {
            int64_t t = (*i).second;
            if (GetTime() < t) return; // we've asked recently
        }

        // ask for the dsee info once from the node that sent dseep

        LogPrintf("dseep - Asking source node for missing entry %s\n", vin.ToString().c_str());
        pfrom->PushMessage("dseg", vin);
        int64_t askAgain = GetTime() + THRONE_MIN_DSEEP_SECONDS;
        mWeAskedForThroneListEntry[vin.prevout] = askAgain;

    } else if (strCommand == "mvote") { //Throne Vote

        CTxIn vin;
        vector<unsigned char> vchSig;
        int nVote;
        vRecv >> vin >> vchSig >> nVote;

        // see if we have this Throne
        CThrone* pmn = this->Find(vin);
        if(pmn != NULL)
        {
            if((GetAdjustedTime() - pmn->lastVote) > (60*60))
            {
                std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(nVote);

                std::string errorMessage = "";
                if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage))
                {
                    LogPrintf("mvote - Got bad Throne address signature %s \n", vin.ToString().c_str());
                    return;
                }

                pmn->nVote = nVote;
                pmn->lastVote = GetAdjustedTime();

                //send to all peers
                LOCK(cs_vNodes);
                BOOST_FOREACH(CNode* pnode, vNodes)
                    pnode->PushMessage("mvote", vin, vchSig, nVote);
            }

            return;
        }

    } else if (strCommand == "dseg") { //Get Throne list or specific entry

        CTxIn vin;
        vRecv >> vin;

        if(vin == CTxIn()) { //only should ask for this once
            //local network
            if(!pfrom->addr.IsRFC1918() && Params().NetworkID() == CChainParams::MAIN)
            {
                std::map<CNetAddr, int64_t>::iterator i = mAskedUsForThroneList.find(pfrom->addr);
                if (i != mAskedUsForThroneList.end())
                {
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

        int count = this->size();
        int i = 0;

        BOOST_FOREACH(CThrone& mn, vThrones) {

            if(mn.addr.IsRFC1918()) continue; //local network

            if(mn.IsEnabled())
            {
                if(fDebug) LogPrintf("dseg - Sending Throne entry - %s \n", mn.addr.ToString().c_str());
                if(vin == CTxIn()){
                    pfrom->PushMessage("dsee", mn.vin, mn.addr, mn.sig, mn.sigTime, mn.pubkey, mn.pubkey2, count, i, mn.lastTimeSeen, mn.protocolVersion);
                } else if (vin == mn.vin) {
                    pfrom->PushMessage("dsee", mn.vin, mn.addr, mn.sig, mn.sigTime, mn.pubkey, mn.pubkey2, count, i, mn.lastTimeSeen, mn.protocolVersion);
                    LogPrintf("dseg - Sent 1 Throne entries to %s\n", pfrom->addr.ToString().c_str());
                    return;
                }
                i++;
            }
        }

        LogPrintf("dseg - Sent %d Throne entries to %s\n", i, pfrom->addr.ToString().c_str());
    }

}

void CThroneMan::RelayThroneEntry(const CTxIn vin, const CService addr, const std::vector<unsigned char> vchSig, const int64_t nNow, const CPubKey pubkey, const CPubKey pubkey2, const int count, const int current, const int64_t lastUpdated, const int protocolVersion)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage("dsee", vin, addr, vchSig, nNow, pubkey, pubkey2, count, current, lastUpdated, protocolVersion);
}

void CThroneMan::RelayThroneEntryPing(const CTxIn vin, const std::vector<unsigned char> vchSig, const int64_t nNow, const bool stop)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage("dseep", vin, vchSig, nNow, stop);
}

void CThroneMan::Remove(CTxIn vin)
{
    LOCK(cs);

    vector<CThrone>::iterator it = vThrones.begin();
    while(it != vThrones.end()){
        if((*it).vin == vin){
            if(fDebug) LogPrintf("CThroneMan: Removing Throne %s - %i now\n", (*it).addr.ToString().c_str(), size() - 1);
            vThrones.erase(it);
            break;
        }
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
