
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
#define SERVICENODE_MIN_SNP_SECONDS             (10*60)
#define SERVICENODE_MIN_SNB_SECONDS             (5*60)
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
    CTxIn vin;
    uint256 blockHash;
    int64_t sigTime; //snb message times
    std::vector<unsigned char> vchSig;

    CServicenodePing();
    CServicenodePing(CTxIn& newVin);
    ADD_SERIALIZE_METHODS;
    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin;
        ss << sigTime;
        return ss.GetHash();
    }
    bool Sign(CKey& keyServicenode, CPubKey& pubKeyServicenode);

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vin);
        READWRITE(blockHash);
        READWRITE(sigTime);
        READWRITE(vchSig);
    }
    friend bool operator==(const CServicenodePing& a, const CServicenodePing& b)
    {
        return a.vin == b.vin && a.blockHash == b.blockHash;
    }
    friend bool operator!=(const CServicenodePing& a, const CServicenodePing& b)
    {
        return !(a == b);
    }
};


//
// The Servicenode Class. For managing the Darksend process. It contains the input of the 10000 CRW, signature to prove
// it's the one who own that ip address and code for calculating the payment election.
//
class CServicenode
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    int64_t lastTimeChecked;
public:
    enum state {
        SERVICENODE_ENABLED = 1,
        SERVICENODE_EXPIRED = 2,
        SERVICENODE_VIN_SPENT = 3,
        SERVICENODE_REMOVE = 4,
        SERVICENODE_POS_ERROR = 5
    };

    CTxIn vin;
    CService addr;
    CPubKey pubkey;
    CPubKey pubkey2;
    std::vector<unsigned char> sig;
    int activeState;
    int64_t sigTime; //snb message time
    bool unitTest;
    int protocolVersion;
    CServicenodePing lastPing;

    CServicenode();
    CServicenode(const CServicenode& other);
    CServicenode(const CServicenodeBroadcast& snb);

    void swap(CServicenode& first, CServicenode& second) // nothrow
    {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two classes,
        // the two classes are effectively swapped
        swap(first.vin, second.vin);
        swap(first.addr, second.addr);
        swap(first.pubkey, second.pubkey);
        swap(first.pubkey2, second.pubkey2);
        swap(first.sig, second.sig);
        swap(first.activeState, second.activeState);
        swap(first.sigTime, second.sigTime);
        swap(first.lastPing, second.lastPing);
        swap(first.unitTest, second.unitTest);
        swap(first.protocolVersion, second.protocolVersion);
    }

    CServicenode& operator=(CServicenode from)
    {
        swap(*this, from);
        return *this;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
            LOCK(cs);

            READWRITE(vin);
            READWRITE(addr);
            READWRITE(pubkey);
            READWRITE(pubkey2);
            READWRITE(sig);
            READWRITE(sigTime);
            READWRITE(protocolVersion);
            READWRITE(activeState);
            READWRITE(lastPing);
            READWRITE(unitTest);
    }
    bool IsValidNetAddr();
    bool IsEnabled()
    {
        return activeState == SERVICENODE_ENABLED;
    }
    bool IsBroadcastedWithin(int seconds)
    {
        return (GetAdjustedTime() - sigTime) < seconds;
    }
    bool UpdateFromNewBroadcast(CServicenodeBroadcast& snb);
    void Check(bool forceCheck = false);
    bool IsPingedWithin(int seconds, int64_t now = -1)
    {
        now == -1 ? now = GetAdjustedTime() : now;

        return (lastPing == CServicenodePing())
                ? false
                : now - lastPing.sigTime < seconds;
    }

};

//
// The Servicenode Broadcast Class : Contains a different serialize method for sending servicenodes through the network
//

class CServicenodeBroadcast : public CServicenode
{
public:
    CServicenodeBroadcast();
    CServicenodeBroadcast(CService newAddr, CTxIn newVin, CPubKey newPubkey, CPubKey newPubkey2, int protocolVersionIn);
    CServicenodeBroadcast(const CServicenode& mn);

    /// Create Servicenode broadcast, needs to be relayed manually after that
    static bool Create(CTxIn txin, CService service, CKey keyCollateral, CPubKey pubKeyCollateral, CKey keyServicenodeNew, CPubKey pubKeyServicenodeNew, std::string &strErrorMessage, CServicenodeBroadcast &mnb);
    static bool Create(std::string strService, std::string strKey, std::string strTxHash, std::string strOutputIndex, std::string& strErrorMessage, CServicenodeBroadcast &mnb, bool fOffline = false);

    bool CheckAndUpdate(int& nDoS);
    bool CheckInputsAndAdd(int& nDos);
    bool Sign(CKey& keyCollateralAddress);
    void Relay();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vin);
        READWRITE(addr);
        READWRITE(pubkey);
        READWRITE(pubkey2);
        READWRITE(sig);
        READWRITE(sigTime);
        READWRITE(protocolVersion);
        READWRITE(lastPing);
    }

    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << sigTime;
        ss << pubkey;
        return ss.GetHash();
    }
};

#endif
