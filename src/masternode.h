
// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MASTERNODE_H
#define MASTERNODE_H

#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "main.h"
#include "timedata.h"

#define MASTERNODE_MIN_MNP_SECONDS             (10*60)
#define MASTERNODE_MIN_MNB_SECONDS             (5*60)
#define MASTERNODE_PING_SECONDS                (5*60)
#define MASTERNODE_EXPIRATION_SECONDS          (65*60)
#define MASTERNODE_REMOVAL_SECONDS             (75*60)
#define MASTERNODE_CHECK_SECONDS               5

using namespace std;

class CMasternode;
class CMasternodeBroadcast;
class CMasternodePing;

//
// The Masternode Ping Class : Contains a different serialize method for sending pings from masternodes throughout the network
//

class CMasternodePing
{
public:

    CTxIn vin;
    uint256 blockHash;
    int64_t sigTime; //mnb message times
    std::vector<unsigned char> vchSig;
    //removed stop

    CMasternodePing();
    CMasternodePing(CTxIn& newVin);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vin);
        READWRITE(blockHash);
        READWRITE(sigTime);
        READWRITE(vchSig);
    }

    bool CheckAndUpdate(int& nDos, bool fRequireEnabled = true, bool fCheckSigTimeOnly = false);
    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool VerifySignature(CPubKey& pubKeyMasternode, int &nDos);
    void Relay();

    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin;
        ss << sigTime;
        return ss.GetHash();
    }

    void swap(CMasternodePing& first, CMasternodePing& second) // nothrow
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

    CMasternodePing& operator=(CMasternodePing from)
    {
        swap(*this, from);
        return *this;
    }
    friend bool operator==(const CMasternodePing& a, const CMasternodePing& b)
    {
        return a.vin == b.vin && a.blockHash == b.blockHash;
    }
    friend bool operator!=(const CMasternodePing& a, const CMasternodePing& b)
    {
        return !(a == b);
    }

};


//
// The Masternode Class. For managing the Darksend process. It contains the input of the 1000DRK, signature to prove
// it's the one who own that ip address and code for calculating the payment election.
//
class CMasternode
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    int64_t lastTimeChecked;
public:
    enum state {
        MASTERNODE_PRE_ENABLED,
        MASTERNODE_ENABLED,
        MASTERNODE_EXPIRED,
        MASTERNODE_VIN_SPENT,
        MASTERNODE_REMOVE,
        MASTERNODE_POS_ERROR
    };

    CTxIn vin;
    CService addr;
    CPubKey pubkey;
    CPubKey pubkey2;
    std::vector<unsigned char> vchSig;
    int activeState;
    int64_t sigTime; //mnb message time
    int64_t nTimeLastPaid;
    int nBlockLastPaid;
    int nCacheCollateralBlock;
    bool unitTest;
    bool allowFreeTx;
    int protocolVersion;
    int64_t nLastDsq; //the dsq count from the last dsq broadcast of this node
    int nScanningErrorCount;
    int nLastScanningErrorBlockHeight;
    CMasternodePing lastPing;

    // KEEP TRACK OF GOVERNANCE ITEMS EACH MASTERNODE HAS VOTE UPON FOR RECALCULATION
    std::map<uint256, int> mapGovernaceObjectsVotedOn;

    CMasternode();
    CMasternode(const CMasternode& other);
    CMasternode(const CMasternodeBroadcast& mnb);


    void swap(CMasternode& first, CMasternode& second) // nothrow
    {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two classes,
        // the two classes are effectively swapped
        swap(first.vin, second.vin);
        swap(first.addr, second.addr);
        swap(first.pubkey, second.pubkey);
        swap(first.pubkey2, second.pubkey2);
        swap(first.vchSig, second.vchSig);
        swap(first.activeState, second.activeState);
        swap(first.sigTime, second.sigTime);
        swap(first.lastPing, second.lastPing);
        swap(first.nTimeLastPaid, second.nTimeLastPaid);
        swap(first.nBlockLastPaid, second.nBlockLastPaid);
        swap(first.nCacheCollateralBlock, second.nCacheCollateralBlock);
        swap(first.unitTest, second.unitTest);
        swap(first.allowFreeTx, second.allowFreeTx);
        swap(first.protocolVersion, second.protocolVersion);
        swap(first.nLastDsq, second.nLastDsq);
        swap(first.nScanningErrorCount, second.nScanningErrorCount);
        swap(first.nLastScanningErrorBlockHeight, second.nLastScanningErrorBlockHeight);
        swap(first.mapGovernaceObjectsVotedOn, second.mapGovernaceObjectsVotedOn);
    }

    CMasternode& operator=(CMasternode from)
    {
        swap(*this, from);
        return *this;
    }
    friend bool operator==(const CMasternode& a, const CMasternode& b)
    {
        return a.vin == b.vin;
    }
    friend bool operator!=(const CMasternode& a, const CMasternode& b)
    {
        return !(a.vin == b.vin);
    }

    // CALCULATE A RANK AGAINST OF GIVEN BLOCK
    uint256 CalculateScore(int nBlockHeight = -1);

    // KEEP TRACK OF EACH GOVERNANCE ITEM INCASE THIS NODE GOES OFFLINE, SO WE CAN RECALC THEIR STATUS
    void AddGovernanceVote(uint256 nGovernanceObjectHash);

    // RECALCULATE CACHED STATUS FLAGS FOR ALL AFFECTED OBJECTS
    void FlagGovernanceItemsAsDirty();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
            LOCK(cs);

            READWRITE(vin);
            READWRITE(addr);
            READWRITE(pubkey);
            READWRITE(pubkey2);
            READWRITE(vchSig);
            READWRITE(sigTime);
            READWRITE(protocolVersion);
            READWRITE(activeState);
            READWRITE(lastPing);
            READWRITE(nTimeLastPaid);
            READWRITE(nBlockLastPaid);
            READWRITE(nCacheCollateralBlock);
            READWRITE(unitTest);
            READWRITE(allowFreeTx);
            READWRITE(nLastDsq);
            READWRITE(nScanningErrorCount);
            READWRITE(nLastScanningErrorBlockHeight);
            READWRITE(mapGovernaceObjectsVotedOn);
    }

    int64_t SecondsSincePayment();

    bool UpdateFromNewBroadcast(CMasternodeBroadcast& mnb);

    inline uint64_t SliceHash(uint256& hash, int slice)
    {
        uint64_t n = 0;
        memcpy(&n, &hash+slice*64, 64);
        return n;
    }

    void Check(bool forceCheck = false);

    bool IsBroadcastedWithin(int seconds)
    {
        return (GetAdjustedTime() - sigTime) < seconds;
    }

    bool IsPingedWithin(int seconds, int64_t now = -1)
    {
        now == -1 ? now = GetAdjustedTime() : now;

        return (lastPing == CMasternodePing())
                ? false
                : now - lastPing.sigTime < seconds;
    }

    void Disable()
    {
        sigTime = 0;
        lastPing = CMasternodePing();
    }

    bool IsEnabled()
    {
        return activeState == MASTERNODE_ENABLED;
    }

    bool IsPreEnabled()
    {
        return activeState == MASTERNODE_PRE_ENABLED;
    }

    std::string Status() {
        std::string strStatus = "unknown";

        if(activeState == CMasternode::MASTERNODE_PRE_ENABLED) strStatus = "PRE_ENABLED";
        if(activeState == CMasternode::MASTERNODE_ENABLED) strStatus     = "ENABLED";
        if(activeState == CMasternode::MASTERNODE_EXPIRED) strStatus     = "EXPIRED";
        if(activeState == CMasternode::MASTERNODE_VIN_SPENT) strStatus   = "VIN_SPENT";
        if(activeState == CMasternode::MASTERNODE_REMOVE) strStatus      = "REMOVE";
        if(activeState == CMasternode::MASTERNODE_POS_ERROR) strStatus   = "POS_ERROR";

        return strStatus;
    }

    int GetCollateralAge();

    int GetLastPaidTime() { return nTimeLastPaid; }
    int GetLastPaidBlock() { return nBlockLastPaid; }
    void UpdateLastPaid(const CBlockIndex *pindex, int nMaxBlocksToScanBack);

};


//
// The Masternode Broadcast Class : Contains a different serialize method for sending masternodes through the network
//

class CMasternodeBroadcast : public CMasternode
{
public:
    CMasternodeBroadcast();
    CMasternodeBroadcast(CService newAddr, CTxIn newVin, CPubKey newPubkey, CPubKey newPubkey2, int protocolVersionIn);
    CMasternodeBroadcast(const CMasternode& mn);

    /// Create Masternode broadcast, needs to be relayed manually after that
    static bool Create(CTxIn txin, CService service, CKey keyCollateral, CPubKey pubKeyCollateral, CKey keyMasternodeNew, CPubKey pubKeyMasternodeNew, std::string &strErrorMessage, CMasternodeBroadcast &mnb);
    static bool Create(std::string strService, std::string strKey, std::string strTxHash, std::string strOutputIndex, std::string& strErrorMessage, CMasternodeBroadcast &mnb, bool fOffline = false);

    bool CheckAndUpdate(int& nDos);
    bool CheckInputsAndAdd(int& nDos);
    bool Sign(CKey& keyCollateralAddress);
    bool VerifySignature(int& nDos);
    void Relay();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vin);
        READWRITE(addr);
        READWRITE(pubkey);
        READWRITE(pubkey2);
        READWRITE(vchSig);
        READWRITE(sigTime);
        READWRITE(protocolVersion);
        READWRITE(lastPing);
        READWRITE(nLastDsq);
    }

    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        //
        // REMOVE AFTER MIGRATION TO 12.1
        //
        if(protocolVersion < 70201) {
            ss << sigTime;
            ss << pubkey;
        } else {
        //
        // END REMOVE
        //
            ss << vin;
            ss << pubkey;
            ss << sigTime;
        }
        return ss.GetHash();
    }

};

#endif
