// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSTEMNODEMAN_H
#define SYSTEMNODEMAN_H

#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "main.h"
#include "systemnode.h"

#define SYSTEMNODES_DUMP_SECONDS               (15*60)
#define SYSTEMNODES_DSEG_SECONDS               (3*60*60)

using namespace std;

class CSystemnodeMan;

extern CSystemnodeMan snodeman;
void DumpSystemnodes();

/** Access to the SN database (sncache.dat)
 */
class CSystemnodeDB
{
private:
    boost::filesystem::path pathSN;
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

    CSystemnodeDB();
    bool Write(const CSystemnodeMan &snodemanToSave);
    ReadResult Read(CSystemnodeMan& snodemanToLoad, bool fDryRun = false);
};

class CSystemnodeMan
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // critical section to protect the inner data structures specifically on messaging
    mutable CCriticalSection cs_process_message;

    // map to hold all SNs
    std::vector<CSystemnode> vSystemnodes;
    // who's asked for the Systemnode list and the last time
    std::map<CNetAddr, int64_t> mAskedUsForSystemnodeList;
    // who we asked for the Systemnode list and the last time
    std::map<CNetAddr, int64_t> mWeAskedForSystemnodeList;
    // which Systemnodes we've asked for
    std::map<COutPoint, int64_t> mWeAskedForSystemnodeListEntry;

public:
    // Keep track of all broadcasts I've seen
    map<uint256, CSystemnodeBroadcast> mapSeenSystemnodeBroadcast;
    // Keep track of all pings I've seen
    map<uint256, CSystemnodePing> mapSeenSystemnodePing;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        LOCK(cs);
        READWRITE(vSystemnodes);
        READWRITE(mAskedUsForSystemnodeList);
        READWRITE(mWeAskedForSystemnodeList);
        READWRITE(mWeAskedForSystemnodeListEntry);

        READWRITE(mapSeenSystemnodeBroadcast);
        READWRITE(mapSeenSystemnodePing);
    }

    //CSystemnodeMan();
    //CSystemnodeMan(CSystemnodeMan& other);

    /// Add an entry
    bool Add(CSystemnode &mn);

    /// Ask (source) node for mnb
    void AskForSN(CNode *pnode, CTxIn &vin);

    /// Check all Systemnodes
    void Check();

    /// Check all Systemnodes and remove inactive
    void CheckAndRemove(bool forceExpiredRemoval = false);

    /// Clear Systemnode vector
    void Clear();

    int CountEnabled(int protocolVersion = -1);

    void DsegUpdate(CNode* pnode);

    /// Find an entry
    CSystemnode* Find(const CTxIn& vin);
    CSystemnode* Find(const CPubKey& pubKeySystemnode);

    std::vector<CSystemnode> GetFullSystemnodeVector() { Check(); return vSystemnodes; }

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    /// Return the number of (unique) Systemnodes
    int size() { return vSystemnodes.size(); }

    std::string ToString() const;

    void Remove(CTxIn vin);

    /// Update systemnode list and maps using provided CSystemnodeBroadcast
    void UpdateSystemnodeList(CSystemnodeBroadcast snb);

    /// Perform complete check and only then update list and maps
    bool CheckSnbAndUpdateSystemnodeList(CSystemnodeBroadcast snb, int& nDos);
};

#endif
