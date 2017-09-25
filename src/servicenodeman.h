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

using namespace std;

class CServicenodeMan;

extern CServicenodeMan snodeman;

/** Access to the SN database (sncache.dat)
 */
class CServicenodeDB
{
};

class CServicenodeMan
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // critical section to protect the inner data structures specifically on messaging
    mutable CCriticalSection cs_process_message;

public:
    // Keep track of all broadcasts I've seen
    map<uint256, CServicenodeBroadcast> mapSeenServicenodeBroadcast;
    // Keep track of all pings I've seen
    map<uint256, CServicenodePing> mapSeenServicenodePing;

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    /// Perform complete check and only then update list and maps
    bool CheckSnbAndUpdateServicenodeList(CServicenodeBroadcast snb, int& nDos);
};

#endif
