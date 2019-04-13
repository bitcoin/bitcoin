// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "masternodeman.h"
#include "masternode.h"
#include "systemnodeman.h"
#include "activemasternode.h"
#include "legacysigner.h"
#include "util.h"
#include "addrman.h"
#include "spork.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

/** Masternode manager */
CMasternodeMan mnodeman;

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
    bool operator()(const pair<int64_t, CMasternode>& t1,
                    const pair<int64_t, CMasternode>& t2) const
    {
        return t1.first < t2.first;
    }
};

CMasternodeMan::CMasternodeMan() {
    nDsqCount = 0;
}

bool CMasternodeMan::Add(const CMasternode &mn)
{
    LOCK(cs);

    if (!mn.IsEnabled())
        return false;

    CMasternode *pmn = Find(mn.vin);
    if (pmn == NULL)
    {
        LogPrint("masternode", "CMasternodeMan: Adding new Masternode %s - %i now\n", mn.addr.ToString(), size() + 1);
        vMasternodes.push_back(mn);
        return true;
    }

    return false;
}

void CMasternodeMan::AskForMN(CNode* pnode, const CTxIn &vin)
{
    std::map<COutPoint, int64_t>::iterator i = mWeAskedForMasternodeListEntry.find(vin.prevout);
    if (i != mWeAskedForMasternodeListEntry.end())
    {
        int64_t t = (*i).second;
        if (GetTime() < t) return; // we've asked recently
    }

    // ask for the mnb info once from the node that sent mnp

    LogPrintf("CMasternodeMan::AskForMN - Asking node for missing entry, vin: %s\n", vin.ToString());
    pnode->PushMessage("dseg", vin);
    int64_t askAgain = GetTime() + MASTERNODE_MIN_MNP_SECONDS;
    mWeAskedForMasternodeListEntry[vin.prevout] = askAgain;
}

void CMasternodeMan::Check()
{
    LOCK(cs);

    BOOST_FOREACH(CMasternode& mn, vMasternodes) {
        mn.Check();
    }
}

void CMasternodeMan::CheckAndRemove(bool forceExpiredRemoval)
{
    Check();

    LOCK(cs);

    //remove inactive and outdated
    vector<CMasternode>::iterator it = vMasternodes.begin();
    while(it != vMasternodes.end()){
        if((*it).activeState == CMasternode::MASTERNODE_REMOVE ||
                (*it).activeState == CMasternode::MASTERNODE_VIN_SPENT ||
                (forceExpiredRemoval && (*it).activeState == CMasternode::MASTERNODE_EXPIRED) ||
                (*it).protocolVersion < masternodePayments.GetMinMasternodePaymentsProto()) {
            LogPrint("masternode", "CMasternodeMan: Removing inactive Masternode %s - %i now\n", (*it).addr.ToString(), size() - 1);

            //erase all of the broadcasts we've seen from this vin
            // -- if we missed a few pings and the node was removed, this will allow is to get it back without them 
            //    sending a brand new mnb
            map<uint256, CMasternodeBroadcast>::iterator it3 = mapSeenMasternodeBroadcast.begin();
            while(it3 != mapSeenMasternodeBroadcast.end()){
                if((*it3).second.vin == (*it).vin){
                    masternodeSync.mapSeenSyncMNB.erase((*it3).first);
                    mapSeenMasternodeBroadcast.erase(it3++);
                } else {
                    ++it3;
                }
            }

            // allow us to ask for this masternode again if we see another ping
            map<COutPoint, int64_t>::iterator it2 = mWeAskedForMasternodeListEntry.begin();
            while(it2 != mWeAskedForMasternodeListEntry.end()){
                if((*it2).first == (*it).vin.prevout){
                    mWeAskedForMasternodeListEntry.erase(it2++);
                } else {
                    ++it2;
                }
            }

            it = vMasternodes.erase(it);
        } else {
            ++it;
        }
    }

    // check who's asked for the Masternode list
    map<CNetAddr, int64_t>::iterator it1 = mAskedUsForMasternodeList.begin();
    while(it1 != mAskedUsForMasternodeList.end()){
        if((*it1).second < GetTime()) {
            mAskedUsForMasternodeList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check who we asked for the Masternode list
    it1 = mWeAskedForMasternodeList.begin();
    while(it1 != mWeAskedForMasternodeList.end()){
        if((*it1).second < GetTime()){
            mWeAskedForMasternodeList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check which Masternodes we've asked for
    map<COutPoint, int64_t>::iterator it2 = mWeAskedForMasternodeListEntry.begin();
    while(it2 != mWeAskedForMasternodeListEntry.end()){
        if((*it2).second < GetTime()){
            mWeAskedForMasternodeListEntry.erase(it2++);
        } else {
            ++it2;
        }
    }

    // remove expired mapSeenMasternodeBroadcast
    map<uint256, CMasternodeBroadcast>::iterator it3 = mapSeenMasternodeBroadcast.begin();
    while(it3 != mapSeenMasternodeBroadcast.end()){
        if((*it3).second.lastPing.sigTime < GetTime() - MASTERNODE_REMOVAL_SECONDS*2){
            LogPrint("masternode", "CMasternodeMan::CheckAndRemove - Removing expired Masternode broadcast %s\n", (*it3).second.GetHash().ToString());
            masternodeSync.mapSeenSyncMNB.erase((*it3).second.GetHash());
            mapSeenMasternodeBroadcast.erase(it3++);
        } else {
            ++it3;
        }
    }

    // remove expired mapSeenMasternodePing
    map<uint256, CMasternodePing>::iterator it4 = mapSeenMasternodePing.begin();
    while(it4 != mapSeenMasternodePing.end()){
        if((*it4).second.sigTime < GetTime()-(MASTERNODE_REMOVAL_SECONDS*2)){
            mapSeenMasternodePing.erase(it4++);
        } else {
            ++it4;
        }
    }

}

void CMasternodeMan::Clear()
{
    LOCK(cs);
    vMasternodes.clear();
    mAskedUsForMasternodeList.clear();
    mWeAskedForMasternodeList.clear();
    mWeAskedForMasternodeListEntry.clear();
    mapSeenMasternodeBroadcast.clear();
    mapSeenMasternodePing.clear();
    nDsqCount = 0;
}

int CMasternodeMan::CountEnabled(int protocolVersion)
{
    int i = 0;
    protocolVersion = protocolVersion == -1 ? masternodePayments.GetMinMasternodePaymentsProto() : protocolVersion;

    BOOST_FOREACH(CMasternode& mn, vMasternodes) {
        mn.Check();
        if(mn.protocolVersion < protocolVersion || !mn.IsEnabled()) continue;
        i++;
    }

    return i;
}

void CMasternodeMan::DsegUpdate(CNode* pnode)
{
    LOCK(cs);

    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(!(pnode->addr.IsRFC1918() || pnode->addr.IsLocal())){
            std::map<CNetAddr, int64_t>::iterator it = mWeAskedForMasternodeList.find(pnode->addr);
            if (it != mWeAskedForMasternodeList.end())
            {
                if (GetTime() < (*it).second) {
                    LogPrintf("dseg - we already asked %s for the list; skipping...\n", pnode->addr.ToString());
                    return;
                }
            }
        }
    }
    
    pnode->PushMessage("dseg", CTxIn());
    int64_t askAgain = GetTime() + MASTERNODES_DSEG_SECONDS;
    mWeAskedForMasternodeList[pnode->addr] = askAgain;
}

CMasternode *CMasternodeMan::Find(const CScript &payee)
{
    LOCK(cs);
    CScript payee2;

    BOOST_FOREACH(CMasternode& mn, vMasternodes)
    {
        payee2 = GetScriptForDestination(mn.pubkey.GetID());
        if(payee2 == payee)
            return &mn;
    }
    return NULL;
}

CMasternode *CMasternodeMan::Find(const CTxIn &vin)
{
    LOCK(cs);

    BOOST_FOREACH(CMasternode& mn, vMasternodes)
    {
        if(mn.vin.prevout == vin.prevout)
            return &mn;
    }
    return NULL;
}


CMasternode *CMasternodeMan::Find(const CPubKey &pubKeyMasternode)
{
    LOCK(cs);

    BOOST_FOREACH(CMasternode& mn, vMasternodes)
    {
        if(mn.pubkey2 == pubKeyMasternode)
            return &mn;
    }
    return NULL;
}

CMasternode *CMasternodeMan::Find(const CService& addr)
{
    LOCK(cs);

    BOOST_FOREACH(CMasternode& mn, vMasternodes)
    {
        if(mn.addr == addr)
            return &mn;
    }
    return NULL;
}

// 
// Deterministically select the oldest/best masternode to pay on the network
//
CMasternode* CMasternodeMan::GetNextMasternodeInQueueForPayment(int nBlockHeight, bool fFilterSigTime, int& nCount)
{
    LOCK(cs);

    CMasternode *pBestMasternode = NULL;
    std::vector<pair<int64_t, CTxIn> > vecMasternodeLastPaid;

    /*
        Make a vector with all of the last paid times
    */

    int nMnCount = CountEnabled();
    BOOST_FOREACH(CMasternode &mn, vMasternodes)
    {
        mn.Check();
        if(!mn.IsEnabled()) continue;

        // //check protocol version
        if(mn.protocolVersion < masternodePayments.GetMinMasternodePaymentsProto()) continue;

        //it's in the list (up to 8 entries ahead of current block to allow propagation) -- so let's skip it
        if(masternodePayments.IsScheduled(mn, nBlockHeight)) continue;

        // For security reasons and for network stability there is a delay to get the first reward.
        // The time is calculated as a product of 60 block and node count.
        if(fFilterSigTime && mn.sigTime + (nMnCount * 1 * 60) > GetAdjustedTime()) continue;

        //make sure it has as many confirmations as there are masternodes
        if(mn.GetMasternodeInputAge() < nMnCount) continue;

        vecMasternodeLastPaid.push_back(make_pair(mn.SecondsSincePayment(), mn.vin));
    }

    nCount = (int)vecMasternodeLastPaid.size();

    //when the network is in the process of upgrading, don't penalize nodes that recently restarted
    if(fFilterSigTime && nCount < nMnCount/3) return GetNextMasternodeInQueueForPayment(nBlockHeight, false, nCount);

    // Sort them high to low
    sort(vecMasternodeLastPaid.rbegin(), vecMasternodeLastPaid.rend(), CompareLastPaid());

    // Look at 1/10 of the oldest nodes (by last payment), calculate their scores and pay the best one
    //  -- This doesn't look at who is being paid in the +8-10 blocks, allowing for double payments very rarely
    //  -- 1/100 payments should be a double payment on mainnet - (1/(3000/10))*2
    //  -- (chance per block * chances before IsScheduled will fire)
    int nTenthNetwork = CountEnabled()/10;
    int nCountTenth = 0; 
    arith_uint256 nHigh = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CTxIn)& s, vecMasternodeLastPaid){
        CMasternode* pmn = Find(s.second);
        if(!pmn) break;

        arith_uint256 n = pmn->CalculateScore(nBlockHeight - 100);
        if(n > nHigh){
            nHigh = n;
            pBestMasternode = pmn;
        }
        nCountTenth++;
        if(nCountTenth >= nTenthNetwork) break;
    }
    return pBestMasternode;
}

CMasternode *CMasternodeMan::FindRandomNotInVec(std::vector<CTxIn> &vecToExclude, int protocolVersion)
{
    LOCK(cs);

    protocolVersion = protocolVersion == -1 ? masternodePayments.GetMinMasternodePaymentsProto() : protocolVersion;

    int nCountEnabled = CountEnabled(protocolVersion);
    LogPrintf("CMasternodeMan::FindRandomNotInVec - nCountEnabled - vecToExclude.size() %d\n", nCountEnabled - vecToExclude.size());
    if(nCountEnabled - vecToExclude.size() < 1) return NULL;

    int rand = GetRandInt(nCountEnabled - vecToExclude.size());
    LogPrintf("CMasternodeMan::FindRandomNotInVec - rand %d\n", rand);
    bool found;

    BOOST_FOREACH(CMasternode &mn, vMasternodes) {
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

CMasternode* CMasternodeMan::GetCurrentMasterNode(int mod, int64_t nBlockHeight, int minProtocol)
{
    int64_t score = 0;
    CMasternode* winner = NULL;

    // scan for winner
    BOOST_FOREACH(CMasternode& mn, vMasternodes) {
        mn.Check();
        if(mn.protocolVersion < minProtocol || !mn.IsEnabled()) continue;

        // calculate the score for each Masternode
        arith_uint256 n = mn.CalculateScore(nBlockHeight);
        int64_t n2 = n.GetCompact(false);

        // determine the winner
        if(n2 > score){
            score = n2;
            winner = &mn;
        }
    }

    return winner;
}

int CMasternodeMan::GetMasternodeRank(const CTxIn& vin, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<pair<int64_t, CTxIn> > vecMasternodeScores;

    //make sure we know about this block
    uint256 hash = uint256();
    if(!GetBlockHash(hash, nBlockHeight)) return -1;

    // scan for winner
    BOOST_FOREACH(CMasternode& mn, vMasternodes) {
        if(mn.protocolVersion < minProtocol) continue;
        if(fOnlyActive) {
            mn.Check();
            if(!mn.IsEnabled()) continue;
        }
        arith_uint256 n = mn.CalculateScore(nBlockHeight);
        int64_t n2 = n.GetCompact(false);

        vecMasternodeScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecMasternodeScores.rbegin(), vecMasternodeScores.rend(), CompareScoreTxIn());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CTxIn)& s, vecMasternodeScores){
        rank++;
        if(s.second.prevout == vin.prevout) {
            return rank;
        }
    }

    return -1;
}

std::vector<pair<int, CMasternode> > CMasternodeMan::GetMasternodeRanks(int64_t nBlockHeight, int minProtocol)
{
    std::vector<pair<int64_t, CMasternode> > vecMasternodeScores;
    std::vector<pair<int, CMasternode> > vecMasternodeRanks;

    //make sure we know about this block
    uint256 hash = uint256();
    if(!GetBlockHash(hash, nBlockHeight)) return vecMasternodeRanks;

    // scan for winner
    BOOST_FOREACH(CMasternode& mn, vMasternodes) {

        mn.Check();

        if(mn.protocolVersion < minProtocol) continue;
        if(!mn.IsEnabled()) {
            continue;
        }

        arith_uint256 n = mn.CalculateScore(nBlockHeight);
        int64_t n2 = n.GetCompact(false);

        vecMasternodeScores.push_back(make_pair(n2, mn));
    }

    sort(vecMasternodeScores.rbegin(), vecMasternodeScores.rend(), CompareScoreMN());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CMasternode)& s, vecMasternodeScores){
        rank++;
        vecMasternodeRanks.push_back(make_pair(rank, s.second));
    }

    return vecMasternodeRanks;
}

CMasternode* CMasternodeMan::GetMasternodeByRank(int nRank, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<pair<int64_t, CTxIn> > vecMasternodeScores;

    // scan for winner
    BOOST_FOREACH(CMasternode& mn, vMasternodes) {

        if(mn.protocolVersion < minProtocol) continue;
        if(fOnlyActive) {
            mn.Check();
            if(!mn.IsEnabled()) continue;
        }

        arith_uint256 n = mn.CalculateScore(nBlockHeight);
        int64_t n2 = n.GetCompact(false);

        vecMasternodeScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecMasternodeScores.rbegin(), vecMasternodeScores.rend(), CompareScoreTxIn());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CTxIn)& s, vecMasternodeScores){
        rank++;
        if(rank == nRank) {
            return Find(s.second);
        }
    }

    return NULL;
}

void CMasternodeMan::ProcessMasternodeConnections()
{
    //we don't care about this for regtest
    if(Params().NetworkID() == CBaseChainParams::REGTEST) return;

    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes) {
        if(pnode->fMasternode){
            if(legacySigner.pSubmittedToMasternode != NULL && pnode->addr == legacySigner.pSubmittedToMasternode->addr) continue;
            LogPrintf("Closing Masternode connection %s \n", pnode->addr.ToString());
            pnode->fMasternode = false;
            pnode->Release();
        }
    }
}

void CMasternodeMan::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{

    if(fLiteMode) return; //disable all Masternode related functionality
    if (!GetBoolArg("-jumpstart", false))
    {
        if(!masternodeSync.IsBlockchainSynced()) return;
    }

    LOCK(cs_process_message);

    if (strCommand == "mnb" || strCommand == "mnb_new") { //Masternode Broadcast
        CMasternodeBroadcast mnb;
        if (strCommand == "mnb") {
            //Set to old version for old serialization
            mnb.lastPing.nVersion = 1;
        } else {
            mnb.lastPing.nVersion = 2;
        }
        vRecv >> mnb;

        int nDoS = 0;
        if (CheckMnbAndUpdateMasternodeList(mnb, nDoS)) {
            // use announced Masternode as a peer
             addrman.Add(CAddress(mnb.addr), pfrom->addr, 2*60*60);
        } else {
            if(nDoS > 0) Misbehaving(pfrom->GetId(), nDoS);
        }
    }

    else if (strCommand == "mnp" || strCommand == "mnp_new") { //Masternode Ping
        CMasternodePing mnp;
        if (strCommand == "mnp") {
            //Set to old version for old serialization
            mnp.nVersion = 1;
        }
        vRecv >> mnp;

        LogPrint("masternode", "mnp - Masternode ping, vin: %s\n", mnp.vin.ToString());

        if(mapSeenMasternodePing.count(mnp.GetHash())) return; //seen
        mapSeenMasternodePing.insert(make_pair(mnp.GetHash(), mnp));

        int nDoS = 0;
        LOCK(cs_main);
        if(mnp.CheckAndUpdate(nDoS)) return;

        if(nDoS > 0) {
            // if anything significant failed, mark that node
            Misbehaving(pfrom->GetId(), nDoS);
        } else {
            // if nothing significant failed, search existing Masternode list
            CMasternode* pmn = Find(mnp.vin);
            // if it's known, don't ask for the mnb, just return
            if(pmn != NULL) return;
        }

        // something significant is broken or mn is unknown,
        // we might have to ask for a masternode entry once
        AskForMN(pfrom, mnp.vin);

    } else if (strCommand == "dseg") { //Get Masternode list or specific entry

        CTxIn vin;
        vRecv >> vin;

        if(vin == CTxIn()) { //only should ask for this once
            //local network
            bool isLocal = (pfrom->addr.IsRFC1918() || pfrom->addr.IsLocal());

            if(!isLocal && Params().NetworkID() == CBaseChainParams::MAIN) {
                std::map<CNetAddr, int64_t>::iterator i = mAskedUsForMasternodeList.find(pfrom->addr);
                if (i != mAskedUsForMasternodeList.end()){
                    int64_t t = (*i).second;
                    if (GetTime() < t) {
                        Misbehaving(pfrom->GetId(), 34);
                        LogPrintf("dseg - peer already asked me for the list\n");
                        return;
                    }
                }
                int64_t askAgain = GetTime() + MASTERNODES_DSEG_SECONDS;
                mAskedUsForMasternodeList[pfrom->addr] = askAgain;
            }
        } //else, asking for a specific node which is ok


        int nInvCount = 0;

        BOOST_FOREACH(CMasternode& mn, vMasternodes) {
            if(mn.addr.IsRFC1918()) continue; //local network

            if(mn.IsEnabled()) {
                LogPrint("masternode", "dseg - Sending Masternode entry - %s \n", mn.addr.ToString());
                if(vin == CTxIn() || vin == mn.vin){
                    CMasternodeBroadcast mnb = CMasternodeBroadcast(mn);
                    uint256 hash = mnb.GetHash();
                    pfrom->PushInventory(CInv(MSG_MASTERNODE_ANNOUNCE, hash));
                    nInvCount++;

                    if(!mapSeenMasternodeBroadcast.count(hash)) mapSeenMasternodeBroadcast.insert(make_pair(hash, mnb));

                    if(vin == mn.vin) {
                        LogPrintf("dseg - Sent 1 Masternode entries to %s\n", pfrom->addr.ToString());
                        return;
                    }
                }
            }
        }

        if(vin == CTxIn()) {
            pfrom->PushMessage("ssc", MASTERNODE_SYNC_LIST, nInvCount);
            LogPrintf("dseg - Sent %d Masternode entries to %s\n", nInvCount, pfrom->addr.ToString());
        }
    }

}

void CMasternodeMan::Remove(CTxIn vin)
{
    LOCK(cs);

    vector<CMasternode>::iterator it = vMasternodes.begin();
    while(it != vMasternodes.end()){
        if((*it).vin == vin){
            LogPrint("masternode", "CMasternodeMan: Removing Masternode %s - %i now\n", (*it).addr.ToString(), size() - 1);
            vMasternodes.erase(it);
            break;
        }
        ++it;
    }
}

std::string CMasternodeMan::ToString() const
{
    std::ostringstream info;

    info << "Masternodes: " << (int)vMasternodes.size() <<
            ", peers who asked us for Masternode list: " << (int)mAskedUsForMasternodeList.size() <<
            ", peers we asked for Masternode list: " << (int)mWeAskedForMasternodeList.size() <<
            ", entries in Masternode list we asked for: " << (int)mWeAskedForMasternodeListEntry.size() <<
            ", nDsqCount: " << (int)nDsqCount;

    return info.str();
}

void CMasternodeMan::UpdateMasternodeList(CMasternodeBroadcast mnb) {
    mapSeenMasternodePing.insert(make_pair(mnb.lastPing.GetHash(), mnb.lastPing));
    mapSeenMasternodeBroadcast.insert(make_pair(mnb.GetHash(), mnb));
    masternodeSync.AddedMasternodeList(mnb.GetHash());

    LogPrintf("CMasternodeMan::UpdateMasternodeList() - addr: %s\n    vin: %s\n", mnb.addr.ToString(), mnb.vin.ToString());

    CMasternode* pmn = Find(mnb.vin);
    if(pmn == NULL)
    {
        CMasternode mn(mnb);
        Add(mn);
    } else {
        pmn->UpdateFromNewBroadcast(mnb);
    }
}

bool CMasternodeMan::CheckMnbAndUpdateMasternodeList(CMasternodeBroadcast mnb, int& nDos) {
    nDos = 0;
    LogPrint("masternode", "CMasternodeMan::CheckMnbAndUpdateMasternodeList - Masternode broadcast, vin: %s\n", mnb.vin.ToString());

    if(mapSeenMasternodeBroadcast.count(mnb.GetHash())) { //seen
        masternodeSync.AddedMasternodeList(mnb.GetHash());
        return true;
    }
    mapSeenMasternodeBroadcast.insert(make_pair(mnb.GetHash(), mnb));

    LogPrint("masternode", "CMasternodeMan::CheckMnbAndUpdateMasternodeList - Masternode broadcast, vin: %s new\n", mnb.vin.ToString());
    // We check addr before both initial mnb and update
    if(!mnb.IsValidNetAddr()) {
        LogPrintf("CMasternodeBroadcast::CheckMnbAndUpdateMasternodeList -- Invalid addr, rejected: masternode=%s  sigTime=%lld  addr=%s\n",
                    mnb.vin.prevout.ToStringShort(), mnb.sigTime, mnb.addr.ToString());
        return false;
    }

    if(!mnb.CheckAndUpdate(nDos)){
        LogPrint("masternode", "CMasternodeMan::CheckMnbAndUpdateMasternodeList - Masternode broadcast, vin: %s CheckAndUpdate failed\n", mnb.vin.ToString());
        return false;
    }

    if (snodeman.Find(mnb.addr) != NULL) {
        LogPrintf("CMasternodeMan::CheckMnbAndUpdateMasternodeList - There is already a systemnode with the same ip: %s\n", mnb.addr.ToString());
        return false;
    }

    // make sure the vout that was signed is related to the transaction that spawned the Masternode
    //  - this is expensive, so it's only done once per Masternode
    if(!legacySigner.IsVinAssociatedWithPubkey(mnb.vin, mnb.pubkey)) {
        LogPrintf("CMasternodeMan::CheckMnbAndUpdateMasternodeList - Got mismatched pubkey and vin\n");
        nDos = 33;
        return false;
    }

    // make sure it's still unspent
    //  - this is checked later by .check() in many places and by ThreadCheckLegacySigner()
    if(mnb.CheckInputsAndAdd(nDos)) {
        masternodeSync.AddedMasternodeList(mnb.GetHash());
    } else {
        LogPrintf("CMasternodeMan::CheckMnbAndUpdateMasternodeList - Rejected Masternode entry %s\n", mnb.addr.ToString());
        return false;
    }

    return true;
}
