// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SERVICENODEMAN_H
#define SERVICENODEMAN_H

#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "main.h"
#include "servicenode.h"

#define SERVICENODES_DUMP_SECONDS               (15*60)
#define SERVICENODES_DSEG_SECONDS               (3*60*60)

using namespace std;

class CServicenodeMan;

extern CServicenodeMan snodeman;

/** Access to the SN database (sncache.dat)
 */
class CServicenodeDB
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

    CServicenodeDB();
    bool Write(const CServicenodeMan &snodemanToSave);
    ReadResult Read(CServicenodeMan& snodemanToLoad, bool fDryRun = false);
};

class CServicenodeMan
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // critical section to protect the inner data structures specifically on messaging
    mutable CCriticalSection cs_process_message;

    // map to hold all SNs
    std::vector<CServicenode> vServicenodes;
    // who's asked for the Servicenode list and the last time
    std::map<CNetAddr, int64_t> mAskedUsForServicenodeList;
    // who we asked for the Servicenode list and the last time
    std::map<CNetAddr, int64_t> mWeAskedForServicenodeList;
    // which Servicenodes we've asked for
    std::map<COutPoint, int64_t> mWeAskedForServicenodeListEntry;

public:
    // Keep track of all broadcasts I've seen
    map<uint256, CServicenodeBroadcast> mapSeenServicenodeBroadcast;
    // Keep track of all pings I've seen
    map<uint256, CServicenodePing> mapSeenServicenodePing;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        LOCK(cs);
        READWRITE(vServicenodes);
        READWRITE(mAskedUsForServicenodeList);
        READWRITE(mWeAskedForServicenodeList);
        READWRITE(mWeAskedForServicenodeListEntry);

        READWRITE(mapSeenServicenodeBroadcast);
        READWRITE(mapSeenServicenodePing);
    }

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    /// Perform complete check and only then update list and maps
    bool CheckSnbAndUpdateServicenodeList(CServicenodeBroadcast snb, int& nDos);
    void DsegUpdate(CNode* pnode);
    /// Find an entry
    CServicenode* Find(const CTxIn& vin);

    /// Clear Servicenode vector
    void Clear();

    /// Check all Servicenodes
    void Check();

    /// Check all Servicenodes and remove inactive
    void CheckAndRemove(bool forceExpiredRemoval = false);

    void Remove(CTxIn vin);
    int CountEnabled(int protocolVersion = -1);

    std::string ToString() const;

    /// Add an entry
    bool Add(CServicenode &mn);
    /// Return the number of (unique) Servicenodes
    int size() { return vServicenodes.size(); }
    void UpdateServicenodeList(CServicenodeBroadcast snb);
};

#endif
