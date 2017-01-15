// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THRONEMAN_H
#define THRONEMAN_H

#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "main.h"
#include "throne.h"

#define THRONES_DUMP_SECONDS               (15*60)
#define THRONES_DSEG_SECONDS               (3*60*60)

using namespace std;

class CThroneMan;

extern CThroneMan mnodeman;
void DumpThrones();

/** Access to the MN database (mncache.dat)
 */
class CThroneDB
{
private:
    boost::filesystem::path pathMN;
    std::string strMagicMessage;
public:
    enum ReadResult {
        Ok,
        FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    CThroneDB();
    bool Write(const CThroneMan &mnodemanToSave);
    ReadResult Read(CThroneMan& mnodemanToLoad, bool fDryRun = false);
};

class CThroneMan
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // critical section to protect the inner data structures specifically on messaging
    mutable CCriticalSection cs_process_message;

    // map to hold all MNs
    std::vector<CThrone> vThrones;
    // who's asked for the Throne list and the last time
    std::map<CNetAddr, int64_t> mAskedUsForThroneList;
    // who we asked for the Throne list and the last time
    std::map<CNetAddr, int64_t> mWeAskedForThroneList;
    // which Thrones we've asked for
    std::map<COutPoint, int64_t> mWeAskedForThroneListEntry;

public:
    // Keep track of all broadcasts I've seen
    map<uint256, CThroneBroadcast> mapSeenThroneBroadcast;
    // Keep track of all pings I've seen
    map<uint256, CThronePing> mapSeenThronePing;
    
    // keep track of dsq count to prevent thrones from gaming darksend queue
    int64_t nDsqCount;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        LOCK(cs);
        READWRITE(vThrones);
        READWRITE(mAskedUsForThroneList);
        READWRITE(mWeAskedForThroneList);
        READWRITE(mWeAskedForThroneListEntry);
        READWRITE(nDsqCount);

        READWRITE(mapSeenThroneBroadcast);
        READWRITE(mapSeenThronePing);
    }

    CThroneMan();
    CThroneMan(CThroneMan& other);

    /// Add an entry
    bool Add(CThrone &mn);

    /// Ask (source) node for mnb
    void AskForMN(CNode *pnode, CTxIn &vin);

    /// Check all Thrones
    void Check();

    /// Check all Thrones and remove inactive
    void CheckAndRemove(bool forceExpiredRemoval = false);

    /// Clear Throne vector
    void Clear();

    int CountEnabled(int protocolVersion = -1);

    void DsegUpdate(CNode* pnode);

    /// Find an entry
    CThrone* Find(const CScript &payee);
    CThrone* Find(const CTxIn& vin);
    CThrone* Find(const CPubKey& pubKeyThrone);

    /// Find an entry in the throne list that is next to be paid
    CThrone* GetNextThroneInQueueForPayment(int nBlockHeight, bool fFilterSigTime, int& nCount);

    /// Find a random entry
    CThrone* FindRandomNotInVec(std::vector<CTxIn> &vecToExclude, int protocolVersion = -1);

    /// Get the current winner for this block
    CThrone* GetCurrentThroNe(int mod=1, int64_t nBlockHeight=0, int minProtocol=0);

    std::vector<CThrone> GetFullThroneVector() { Check(); return vThrones; }

    std::vector<pair<int, CThrone> > GetThroneRanks(int64_t nBlockHeight, int minProtocol=0);
    int GetThroneRank(const CTxIn &vin, int64_t nBlockHeight, int minProtocol=0, bool fOnlyActive=true);
    CThrone* GetThroneByRank(int nRank, int64_t nBlockHeight, int minProtocol=0, bool fOnlyActive=true);

    void ProcessThroneConnections();

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    /// Return the number of (unique) Thrones
    int size() { return vThrones.size(); }

    std::string ToString() const;

    void Remove(CTxIn vin);

    /// Update throne list and maps using provided CThroneBroadcast
    void UpdateThroneList(CThroneBroadcast mnb);
    /// Perform complete check and only then update list and maps
    bool CheckMnbAndUpdateThroneList(CThroneBroadcast mnb, int& nDos);

};

#endif
