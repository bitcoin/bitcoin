
// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SERVICENODE_H
#define SERVICENODE_H

#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "main.h"
#include "timedata.h"

#define SERVICENODE_MIN_CONFIRMATIONS           15
#define SERVICENODE_MIN_MNP_SECONDS             (10*60)
#define SERVICENODE_MIN_MNB_SECONDS             (5*60)
#define SERVICENODE_PING_SECONDS                (5*60)
#define SERVICENODE_EXPIRATION_SECONDS          (65*60)
#define SERVICENODE_REMOVAL_SECONDS             (75*60)
#define SERVICENODE_CHECK_SECONDS               5

using namespace std;

class CServicenode;
class CServicenodeBroadcast;
class CServicenodePing;
extern map<int64_t, uint256> mapCacheBlockHashes;

bool GetBlockHash(uint256& hash, int nBlockHeight);


//
// The Servicenode Ping Class : Contains a different serialize method for sending pings from servicenodes throughout the network
//

class CServicenodePing
{
public:
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
    }
};


//
// The Servicenode Class. For managing the Darksend process. It contains the input of the 10000 CRW, signature to prove
// it's the one who own that ip address and code for calculating the payment election.
//
class CServicenode
{
public:
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
    }

};


//
// The Servicenode Broadcast Class : Contains a different serialize method for sending servicenodes through the network
//

class CServicenodeBroadcast : public CServicenode
{
public:
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
    }

};

#endif
