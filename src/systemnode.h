// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSTEMNODE_H
#define SYSTEMNODE_H

#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "main.h"
#include "timedata.h"

#define SYSTEMNODE_MIN_CONFIRMATIONS           15
#define SYSTEMNODE_MIN_SNP_SECONDS             (10*60)
#define SYSTEMNODE_MIN_SNB_SECONDS             (5*60)
#define SYSTEMNODE_PING_SECONDS                (5*60)
#define SYSTEMNODE_EXPIRATION_SECONDS          (65*60)
#define SYSTEMNODE_REMOVAL_SECONDS             (75*60)
#define SYSTEMNODE_CHECK_SECONDS               5

using namespace std;

class CSystemnode;
class CSystemnodeBroadcast;
class CSystemnodePing;
extern map<int64_t, uint256> mapCacheBlockHashes;

bool GetBlockHash(uint256& hash, int nBlockHeight);


//
// The Systemnode Ping Class : Contains a different serialize method for sending pings from systemnodes throughout the network
//

class CSystemnodePing
{
public:
    CTxIn vin;
    uint256 blockHash;
    int64_t sigTime; //snb message times
    std::vector<unsigned char> vchSig;

    CSystemnodePing();
    CSystemnodePing(const CTxIn& newVin);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vin);
        READWRITE(blockHash);
        READWRITE(sigTime);
        READWRITE(vchSig);
    }

    bool CheckAndUpdate(int& nDos, bool fRequireEnabled = true, bool fCheckSigTimeOnly = false) const;
    bool Sign(const CKey& keySystemnode, const CPubKey& pubKeySystemnode);
    bool VerifySignature(const CPubKey& pubKeySystemnode, int &nDos) const;
    void Relay() const;

    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin;
        ss << sigTime;
        return ss.GetHash();
    }

    void swap(CSystemnodePing& first, CSystemnodePing& second) // nothrow
    {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two classes,
        // the two classes are effectively swapped
        swap(first.vin, second.vin);
        swap(first.blockHash, second.blockHash);
        swap(first.sigTime, second.sigTime);
        swap(first.vchSig, second.vchSig);
    }

    CSystemnodePing& operator=(CSystemnodePing from)
    {
        swap(*this, from);
        return *this;
    }
    friend bool operator==(const CSystemnodePing& a, const CSystemnodePing& b)
    {
        return a.vin == b.vin && a.blockHash == b.blockHash;
    }
    friend bool operator!=(const CSystemnodePing& a, const CSystemnodePing& b)
    {
        return !(a == b);
    }
};


//
// The Systemnode Class. For managing the Darksend process. It contains the input of the 10000 CRW, signature to prove
// it's the one who own that ip address and code for calculating the payment election.
//
class CSystemnode
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    int64_t lastTimeChecked;
public:
    enum state
    {
        SYSTEMNODE_ENABLED = 1,
        SYSTEMNODE_EXPIRED = 2,
        SYSTEMNODE_VIN_SPENT = 3,
        SYSTEMNODE_REMOVE = 4,
        SYSTEMNODE_POS_ERROR = 5
    };

    CTxIn vin;
    CService addr;
    CPubKey pubkey;
    CPubKey pubkey2;
    std::vector<unsigned char> sig;
    int activeState;
    int64_t sigTime; //snb message time
    int cacheInputAge;
    int cacheInputAgeBlock;
    bool unitTest;
    int protocolVersion;
    CSystemnodePing lastPing;

    CSystemnode();
    CSystemnode(const CSystemnode& other);
    CSystemnode(const CSystemnodeBroadcast& snb);

    void swap(CSystemnode& first, CSystemnode& second) // nothrow
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

    CSystemnode& operator=(CSystemnode from)
    {
        swap(*this, from);
        return *this;
    }
    friend bool operator==(const CSystemnode& a, const CSystemnode& b)
    {
        return a.vin == b.vin;
    }
    friend bool operator!=(const CSystemnode& a, const CSystemnode& b)
    {
        return !(a.vin == b.vin);
    }

    arith_uint256 CalculateScore(int64_t nBlockHeight=0) const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
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

    int64_t SecondsSincePayment() const;
    bool UpdateFromNewBroadcast(const CSystemnodeBroadcast& snb);
    void Check(bool forceCheck = false);
    bool IsBroadcastedWithin(int seconds) const
    {
        return (GetAdjustedTime() - sigTime) < seconds;
    }
    bool IsEnabled() const
    {
        return activeState == SYSTEMNODE_ENABLED;
    }
    bool IsValidNetAddr();
    int GetSystemnodeInputAge()
    {
        if(chainActive.Tip() == NULL) return 0;

        if(cacheInputAge == 0){
            cacheInputAge = GetInputAge(vin);
            cacheInputAgeBlock = chainActive.Tip()->nHeight;
        }

        return cacheInputAge+(chainActive.Tip()->nHeight-cacheInputAgeBlock);
    }
    bool IsPingedWithin(int seconds, int64_t now = -1) const
    {
        now == -1 ? now = GetAdjustedTime() : now;

        return (lastPing == CSystemnodePing())
                ? false
                : now - lastPing.sigTime < seconds;
    }
    std::string Status() const
    {
        std::string strStatus = "ACTIVE";

        if(activeState == CSystemnode::SYSTEMNODE_ENABLED) strStatus   = "ENABLED";
        if(activeState == CSystemnode::SYSTEMNODE_EXPIRED) strStatus   = "EXPIRED";
        if(activeState == CSystemnode::SYSTEMNODE_VIN_SPENT) strStatus = "VIN_SPENT";
        if(activeState == CSystemnode::SYSTEMNODE_REMOVE) strStatus    = "REMOVE";
        if(activeState == CSystemnode::SYSTEMNODE_POS_ERROR) strStatus = "POS_ERROR";

        return strStatus;
    }
    int64_t GetLastPaid() const;

    bool GetRecentPaymentBlocks(std::vector<const CBlockIndex*>& vPaymentBlocks, bool limitMostRecent = false) const;
};

//
// The Systemnode Broadcast Class : Contains a different serialize method for sending systemnodes through the network
//

class CSystemnodeBroadcast : public CSystemnode
{
public:
    CSystemnodeBroadcast();
    CSystemnodeBroadcast(CService newAddr, CTxIn newVin, CPubKey newPubkey, CPubKey newPubkey2, int protocolVersionIn);
    CSystemnodeBroadcast(const CSystemnode& sn);

    /// Create Systemnode broadcast, needs to be relayed manually after that
    static bool Create(CTxIn txin, CService service, CKey keyCollateral, CPubKey pubKeyCollateral, CKey keySystemnodeNew, CPubKey pubKeySystemnodeNew, std::string &strErrorMessage, CSystemnodeBroadcast &snb);
    static bool Create(std::string strService, std::string strKey, std::string strTxHash, std::string strOutputIndex, std::string& strErrorMessage, CSystemnodeBroadcast &snb, bool fOffline = false);

    bool CheckAndUpdate(int& nDoS) const;
    bool CheckInputsAndAdd(int& nDos) const;
    bool Sign(const CKey& keyCollateralAddress);
    bool VerifySignature() const;
    void Relay() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vin);
        READWRITE(addr);
        READWRITE(pubkey);
        READWRITE(pubkey2);
        READWRITE(sig);
        READWRITE(sigTime);
        READWRITE(protocolVersion);
        READWRITE(lastPing);
    }

    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << sigTime;
        ss << pubkey;
        return ss.GetHash();
    }
};

#endif
