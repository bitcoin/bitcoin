// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRIVATESEND_H
#define PRIVATESEND_H

#include "chainparams.h"
#include "primitives/transaction.h"
#include "pubkey.h"
#include "utiltime.h"
#include "tinyformat.h"

class CPrivateSend;

// timeouts
static const int PRIVATESEND_AUTO_TIMEOUT_MIN       = 5;
static const int PRIVATESEND_AUTO_TIMEOUT_MAX       = 15;
static const int PRIVATESEND_QUEUE_TIMEOUT          = 30;
static const int PRIVATESEND_SIGNING_TIMEOUT        = 15;

//! minimum peer version accepted by mixing pool
static const int MIN_PRIVATESEND_PEER_PROTO_VERSION = 70206;

static const CAmount PRIVATESEND_COLLATERAL         = 0.001 * COIN;
static const CAmount PRIVATESEND_ENTRY_MAX_SIZE     = 9;

class CDarksendBroadcastTx;

// The main object for accessing mixing
// extern CPrivateSend privateSend;

extern std::map<uint256, CDarksendBroadcastTx> mapDarksendBroadcastTxes;
extern std::vector<CAmount> vecPrivateSendDenominations;

/** Holds an mixing input
 */
class CTxDSIn : public CTxIn
{
public:
    bool fHasSig; // flag to indicate if signed
    int nSentTimes; //times we've sent this anonymously

    CTxDSIn(const CTxIn& txin) :
        CTxIn(txin),
        fHasSig(false),
        nSentTimes(0)
        {}

    CTxDSIn() :
        CTxIn(),
        fHasSig(false),
        nSentTimes(0)
        {}
};

/** Holds an mixing output
 */
class CTxDSOut : public CTxOut
{
public:
    int nSentTimes; //times we've sent this anonymously

    CTxDSOut(const CTxOut& out) :
        CTxOut(out),
        nSentTimes(0)
        {}

    CTxDSOut() :
        CTxOut(),
        nSentTimes(0)
        {}
};

// A clients transaction in the mixing pool
class CDarkSendEntry
{
public:
    std::vector<CTxDSIn> vecTxDSIn;
    std::vector<CTxDSOut> vecTxDSOut;
    CTransaction txCollateral;

    CDarkSendEntry() :
        vecTxDSIn(std::vector<CTxDSIn>()),
        vecTxDSOut(std::vector<CTxDSOut>()),
        txCollateral(CTransaction())
        {}

    CDarkSendEntry(const std::vector<CTxIn>& vecTxIn, const std::vector<CTxOut>& vecTxOut, const CTransaction& txCollateral);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vecTxDSIn);
        READWRITE(txCollateral);
        READWRITE(vecTxDSOut);
    }

    bool AddScriptSig(const CTxIn& txin);
};


/**
 * A currently inprogress mixing merge and denomination information
 */
class CDarksendQueue
{
public:
    int nDenom;
    CTxIn vin;
    int64_t nTime;
    bool fReady; //ready for submit
    std::vector<unsigned char> vchSig;
    // memory only
    bool fTried;

    CDarksendQueue() :
        nDenom(0),
        vin(CTxIn()),
        nTime(0),
        fReady(false),
        vchSig(std::vector<unsigned char>()),
        fTried(false)
        {}

    CDarksendQueue(int nDenom, CTxIn vin, int64_t nTime, bool fReady) :
        nDenom(nDenom),
        vin(vin),
        nTime(nTime),
        fReady(fReady),
        vchSig(std::vector<unsigned char>()),
        fTried(false)
        {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nDenom);
        READWRITE(vin);
        READWRITE(nTime);
        READWRITE(fReady);
        READWRITE(vchSig);
    }

    /** Sign this mixing transaction
     *  \return true if all conditions are met:
     *     1) we have an active Masternode,
     *     2) we have a valid Masternode private key,
     *     3) we signed the message successfully, and
     *     4) we verified the message successfully
     */
    bool Sign();
    /// Check if we have a valid Masternode address
    bool CheckSignature(const CPubKey& pubKeyMasternode);

    bool Relay();

    /// Is this queue expired?
    bool IsExpired() { return GetTime() - nTime > PRIVATESEND_QUEUE_TIMEOUT; }

    std::string ToString()
    {
        return strprintf("nDenom=%d, nTime=%lld, fReady=%s, fTried=%s, masternode=%s",
                        nDenom, nTime, fReady ? "true" : "false", fTried ? "true" : "false", vin.prevout.ToStringShort());
    }

    friend bool operator==(const CDarksendQueue& a, const CDarksendQueue& b)
    {
        return a.nDenom == b.nDenom && a.vin.prevout == b.vin.prevout && a.nTime == b.nTime && a.fReady == b.fReady;
    }
};

/** Helper class to store mixing transaction (tx) information.
 */
class CDarksendBroadcastTx
{
public:
    CTransaction tx;
    CTxIn vin;
    std::vector<unsigned char> vchSig;
    int64_t sigTime;

    CDarksendBroadcastTx() :
        tx(CTransaction()),
        vin(CTxIn()),
        vchSig(std::vector<unsigned char>()),
        sigTime(0)
        {}

    CDarksendBroadcastTx(CTransaction tx, CTxIn vin, int64_t sigTime) :
        tx(tx),
        vin(vin),
        vchSig(std::vector<unsigned char>()),
        sigTime(sigTime)
        {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(tx);
        READWRITE(vin);
        READWRITE(vchSig);
        READWRITE(sigTime);
    }

    bool Sign();
    bool CheckSignature(const CPubKey& pubKeyMasternode);
};

// static class or base class or helper?
class CPrivateSend
{
protected:
    // pool responses
    enum PoolMessage {
        ERR_ALREADY_HAVE,
        ERR_DENOM,
        ERR_ENTRIES_FULL,
        ERR_EXISTING_TX,
        ERR_FEES,
        ERR_INVALID_COLLATERAL,
        ERR_INVALID_INPUT,
        ERR_INVALID_SCRIPT,
        ERR_INVALID_TX,
        ERR_MAXIMUM,
        ERR_MN_LIST,
        ERR_MODE,
        ERR_NON_STANDARD_PUBKEY,
        ERR_NOT_A_MN, // not used
        ERR_QUEUE_FULL,
        ERR_RECENT,
        ERR_SESSION,
        ERR_MISSING_TX,
        ERR_VERSION,
        MSG_NOERR,
        MSG_SUCCESS,
        MSG_ENTRIES_ADDED,
        MSG_POOL_MIN = ERR_ALREADY_HAVE,
        MSG_POOL_MAX = MSG_ENTRIES_ADDED
    };

    // pool states
    enum PoolState {
        POOL_STATE_IDLE,
        POOL_STATE_QUEUE,
        POOL_STATE_ACCEPTING_ENTRIES,
        POOL_STATE_SIGNING,
        POOL_STATE_ERROR,
        POOL_STATE_SUCCESS,
        POOL_STATE_MIN = POOL_STATE_IDLE,
        POOL_STATE_MAX = POOL_STATE_SUCCESS
    };

    // status update message constants
    enum PoolStatusUpdate {
        STATUS_REJECTED,
        STATUS_ACCEPTED
    };

    // The current mixing sessions in progress on the network
    std::vector<CDarksendQueue> vecDarksendQueue;

    std::vector<CDarkSendEntry> vecEntries; // Masternode/clients entries

    PoolState nState; // should be one of the POOL_STATE_XXX values
    int64_t nTimeLastSuccessfulStep; // the time when last successful mixing step was performed, in UTC milliseconds

    int nSessionID; // 0 if no mixing session is active

    CMutableTransaction finalMutableTransaction; // the finalized transaction ready for signing

    /// Get the denominations for a specific amount of dash.
    int GetDenominationsByAmounts(const std::vector<CAmount>& vecAmount);

    std::string GetMessageByID(PoolMessage nMessageID);

    /// Get the maximum number of transactions for the pool
    int GetMaxPoolTransactions() { return Params().PoolMaxTransactions(); }

    /// If the collateral is valid given by a client
    bool IsCollateralValid(const CTransaction& txCollateral);

    void SetNull();

public:
    int nSessionDenom; //Users must submit an denom matching this

    CPrivateSend() { SetNull(); }

    void InitDenominations();

    /// Get the denominations for a list of outputs (returns a bitshifted integer)
    int GetDenominations(const std::vector<CTxOut>& vecTxOut, bool fSingleRandomDenom = false);
    int GetDenominations(const std::vector<CTxDSOut>& vecTxDSOut);
    std::string GetDenominationsToString(int nDenom);
    bool GetDenominationsBits(int nDenom, std::vector<int> &vecBitsRet);

    CAmount GetMaxPoolAmount() { return vecPrivateSendDenominations.empty() ? 0 : PRIVATESEND_ENTRY_MAX_SIZE * vecPrivateSendDenominations.front(); }

    int GetQueueSize() const { return vecDarksendQueue.size(); }
    int GetState() const { return nState; }
    std::string GetStateString() const;

    int GetEntriesCount() const { return vecEntries.size(); }
};

void ThreadCheckPrivateSend();

#endif
