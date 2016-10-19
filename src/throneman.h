// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THRONEMAN_H
#define THRONEMAN_H

#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "core.h"
#include "util.h"
#include "script.h"
#include "base58.h"
#include "main.h"
#include "throne.h"

#define THRONES_DUMP_SECONDS               (15*60)
#define THRONES_DSEG_SECONDS               (1*60*60)

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
    ReadResult Read(CThroneMan& mnodemanToLoad);
};

class CThroneMan
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // map to hold all MNs

    // who's asked for the Throne list and the last time
    std::map<CNetAddr, int64_t> mAskedUsForThroneList;
    // who we asked for the Throne list and the last time
    std::map<CNetAddr, int64_t> mWeAskedForThroneList;
    // which Thrones we've asked for
    std::map<COutPoint, int64_t> mWeAskedForThroneListEntry;

public:
    // keep track of dsq count to prevent thrones from gaming darksend queue
    int64_t nDsqCount;

    std::vector<CThrone> vThrones;

    IMPLEMENT_SERIALIZE
    (
        // serialized format:
        // * version byte (currently 0)
        // * thrones vector
        {
                LOCK(cs);
                unsigned char nVersion = 0;
                READWRITE(nVersion);
                READWRITE(vThrones);
                READWRITE(mAskedUsForThroneList);
                READWRITE(mWeAskedForThroneList);
                READWRITE(mWeAskedForThroneListEntry);
                READWRITE(nDsqCount);
        }
    )

    CThroneMan();
    CThroneMan(CThroneMan& other);

    /// Add an entry
    bool Add(CThrone &mn);

    /// Check all Thrones
    void Check();

    /// Check all Thrones and remove inactive
    void CheckAndRemove();

    /// Clear Throne vector
    void Clear();

    int CountEnabled();

    int CountThronesAboveProtocol(int protocolVersion);

    void DsegUpdate(CNode* pnode);

    /// Find an entry
    CThrone* Find(const CTxIn& vin);
    CThrone* Find(const CPubKey& pubKeyThrone);

    /// Find an entry thta do not match every entry provided vector
    CThrone* FindOldestNotInVec(const std::vector<CTxIn> &vVins, int nMinimumAge, int nMinimumActiveSeconds);

    /// Find a random entry
    CThrone* FindRandom();

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

    //
    // Relay Throne Messages
    //

    void RelayThroneEntry(const CTxIn vin, const CService addr, const std::vector<unsigned char> vchSig, const int64_t nNow, const CPubKey pubkey, const CPubKey pubkey2, const int count, const int current, const int64_t lastUpdated, const int protocolVersion);
    void RelayThroneEntryPing(const CTxIn vin, const std::vector<unsigned char> vchSig, const int64_t nNow, const bool stop);

    void Remove(CTxIn vin);

};

#endif
