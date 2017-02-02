// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DARKSEND_H
#define DARKSEND_H

#include "masternode.h"
#include "wallet/wallet.h"

class CDarksendPool;
class CDarkSendSigner;
class CDarksendBroadcastTx;

// timeouts
static const int PRIVATESEND_AUTO_TIMEOUT_MIN       = 5;
static const int PRIVATESEND_AUTO_TIMEOUT_MAX       = 15;
static const int PRIVATESEND_QUEUE_TIMEOUT          = 30;
static const int PRIVATESEND_SIGNING_TIMEOUT        = 15;

//! minimum peer version accepted by mixing pool
static const int MIN_PRIVATESEND_PEER_PROTO_VERSION = 70206;

static const CAmount PRIVATESEND_COLLATERAL         = 0.001 * COIN;
static const CAmount PRIVATESEND_POOL_MAX           = 999.999 * COIN;
static const int DENOMS_COUNT_MAX                   = 100;

static const int DEFAULT_PRIVATESEND_ROUNDS         = 2;
static const int DEFAULT_PRIVATESEND_AMOUNT         = 1000;
static const int DEFAULT_PRIVATESEND_LIQUIDITY      = 0;
static const bool DEFAULT_PRIVATESEND_MULTISESSION  = false;

// Warn user if mixing in gui or try to create backup if mixing in daemon mode
// when we have only this many keys left
static const int PRIVATESEND_KEYS_THRESHOLD_WARNING = 100;
// Stop mixing completely, it's too dangerous to continue when we have only this many keys left
static const int PRIVATESEND_KEYS_THRESHOLD_STOP    = 50;

extern int nPrivateSendRounds;
extern int nPrivateSendAmount;
extern int nLiquidityProvider;
extern bool fEnablePrivateSend;
extern bool fPrivateSendMultiSession;

// The main object for accessing mixing
extern CDarksendPool darkSendPool;
// A helper object for signing messages from Masternodes
extern CDarkSendSigner darkSendSigner;

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

    CDarkSendEntry(const std::vector<CTxIn>& vecTxIn, const std::vector<CTxOut>& vecTxOut, const CTransaction& txCollateral) :
        txCollateral(txCollateral)
    {
        BOOST_FOREACH(CTxIn txin, vecTxIn)
            vecTxDSIn.push_back(txin);
        BOOST_FOREACH(CTxOut txout, vecTxOut)
            vecTxDSOut.push_back(txout);
    }

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

/** Helper object for signing and checking signatures
 */
class CDarkSendSigner
{
public:
    /// Is the input associated with this public key? (and there is 1000 DASH - checking if valid masternode)
    bool IsVinAssociatedWithPubkey(const CTxIn& vin, const CPubKey& pubkey);
    /// Set the private/public key values, returns true if successful
    bool GetKeysFromSecret(std::string strSecret, CKey& keyRet, CPubKey& pubkeyRet);
    /// Sign the message, returns true if successful
    bool SignMessage(std::string strMessage, std::vector<unsigned char>& vchSigRet, CKey key);
    /// Verify the message, returns true if succcessful
    bool VerifyMessage(CPubKey pubkey, const std::vector<unsigned char>& vchSig, std::string strMessage, std::string& strErrorRet);
};

/** Used to keep track of current status of mixing pool
 */
class CDarksendPool
{
private:
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
        ERR_NOT_A_MN,
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

    mutable CCriticalSection cs_darksend;

    // The current mixing sessions in progress on the network
    std::vector<CDarksendQueue> vecDarksendQueue;
    // Keep track of the used Masternodes
    std::vector<CTxIn> vecMasternodesUsed;

    std::vector<CAmount> vecDenominationsSkipped;
    std::vector<COutPoint> vecOutPointLocked;
    // Mixing uses collateral transactions to trust parties entering the pool
    // to behave honestly. If they don't it takes their money.
    std::vector<CTransaction> vecSessionCollaterals;
    std::vector<CDarkSendEntry> vecEntries; // Masternode/clients entries

    PoolState nState; // should be one of the POOL_STATE_XXX values
    int64_t nTimeLastSuccessfulStep; // the time when last successful mixing step was performed, in UTC milliseconds

    int nCachedLastSuccessBlock;
    int nMinBlockSpacing; //required blocks between mixes
    const CBlockIndex *pCurrentBlockIndex; // Keep track of current block index

    int nSessionID; // 0 if no mixing session is active

    int nEntriesCount;
    bool fLastEntryAccepted;

    std::string strLastMessage;
    std::string strAutoDenomResult;

    bool fUnitTest;

    CMutableTransaction txMyCollateral; // client side collateral
    CMutableTransaction finalMutableTransaction; // the finalized transaction ready for signing

    /// Add a clients entry to the pool
    bool AddEntry(const CDarkSendEntry& entryNew, PoolMessage& nMessageIDRet);
    /// Add signature to a txin
    bool AddScriptSig(const CTxIn& txin);

    /// Charge fees to bad actors (Charge clients a fee if they're abusive)
    void ChargeFees();
    /// Rarely charge fees to pay miners
    void ChargeRandomFees();

    /// Check for process
    void CheckPool();

    void CreateFinalTransaction();
    void CommitFinalTransaction();

    void CompletedTransaction(PoolMessage nMessageID);

    /// Get the denominations for a specific amount of dash.
    int GetDenominationsByAmounts(const std::vector<CAmount>& vecAmount);

    std::string GetMessageByID(PoolMessage nMessageID);

    /// Get the maximum number of transactions for the pool
    int GetMaxPoolTransactions() { return Params().PoolMaxTransactions(); }

    /// Is this nDenom and txCollateral acceptable?
    bool IsAcceptableDenomAndCollateral(int nDenom, CTransaction txCollateral, PoolMessage &nMessageIDRet);
    bool CreateNewSession(int nDenom, CTransaction txCollateral, PoolMessage &nMessageIDRet);
    bool AddUserToExistingSession(int nDenom, CTransaction txCollateral, PoolMessage &nMessageIDRet);
    /// Do we have enough users to take entries?
    bool IsSessionReady() { return (int)vecSessionCollaterals.size() >= GetMaxPoolTransactions(); }

    /// If the collateral is valid given by a client
    bool IsCollateralValid(const CTransaction& txCollateral);
    /// Check that all inputs are signed. (Are all inputs signed?)
    bool IsSignaturesComplete();
    /// Check to make sure a given input matches an input in the pool and its scriptSig is valid
    bool IsInputScriptSigValid(const CTxIn& txin);
    /// Are these outputs compatible with other client in the pool?
    bool IsOutputsCompatibleWithSessionDenom(const std::vector<CTxDSOut>& vecTxDSOut);

    bool IsDenomSkipped(CAmount nDenomValue) {
        return std::find(vecDenominationsSkipped.begin(), vecDenominationsSkipped.end(), nDenomValue) != vecDenominationsSkipped.end();
    }

    /// Create denominations
    bool CreateDenominated();
    bool CreateDenominated(const CompactTallyItem& tallyItem, bool fCreateMixingCollaterals);

    /// Split up large inputs or make fee sized inputs
    bool MakeCollateralAmounts();
    bool MakeCollateralAmounts(const CompactTallyItem& tallyItem);

    /// As a client, submit part of a future mixing transaction to a Masternode to start the process
    bool SubmitDenominate();
    /// step 1: prepare denominated inputs and outputs
    bool PrepareDenominate(int nMinRounds, int nMaxRounds, std::string& strErrorRet, std::vector<CTxIn>& vecTxInRet, std::vector<CTxOut>& vecTxOutRet);
    /// step 2: send denominated inputs and outputs prepared in step 1
    bool SendDenominate(const std::vector<CTxIn>& vecTxIn, const std::vector<CTxOut>& vecTxOut);

    /// Get Masternode updates about the progress of mixing
    bool CheckPoolStateUpdate(PoolState nStateNew, int nEntriesCountNew, PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID, int nSessionIDNew=0);
    // Set the 'state' value, with some logging and capturing when the state changed
    void SetState(PoolState nStateNew);

    /// As a client, check and sign the final transaction
    bool SignFinalTransaction(const CTransaction& finalTransactionNew, CNode* pnode);

    /// Relay mixing Messages
    void RelayFinalTransaction(const CTransaction& txFinal);
    void RelaySignaturesAnon(std::vector<CTxIn>& vin);
    void RelayInAnon(std::vector<CTxIn>& vin, std::vector<CTxOut>& vout);
    void RelayIn(const CDarkSendEntry& entry);
    void PushStatus(CNode* pnode, PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID);
    void RelayStatus(PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID = MSG_NOERR);
    void RelayCompletedTransaction(PoolMessage nMessageID);

    void SetNull();

public:
    CMasternode* pSubmittedToMasternode;
    int nSessionDenom; //Users must submit an denom matching this
    int nCachedNumBlocks; //used for the overview screen
    bool fCreateAutoBackups; //builtin support for automatic backups

    CDarksendPool() :
        nCachedLastSuccessBlock(0),
        nMinBlockSpacing(0),
        fUnitTest(false),
        txMyCollateral(CMutableTransaction()),
        nCachedNumBlocks(std::numeric_limits<int>::max()),
        fCreateAutoBackups(true) { SetNull(); }

    /** Process a mixing message using the protocol below
     * \param pfrom
     * \param strCommand lower case command string; valid values are:
     *        Command  | Description
     *        -------- | -----------------
     *        dsa      | Acceptable
     *        dsc      | Complete
     *        dsf      | Final tx
     *        dsi      | Vector of CTxIn
     *        dsq      | Queue
     *        dss      | Signal Final Tx
     *        dssu     | status update
     * \param vRecv
     */
    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    void InitDenominations();
    void ClearSkippedDenominations() { vecDenominationsSkipped.clear(); }

    /// Get the denominations for a list of outputs (returns a bitshifted integer)
    int GetDenominations(const std::vector<CTxOut>& vecTxOut, bool fSingleRandomDenom = false);
    int GetDenominations(const std::vector<CTxDSOut>& vecTxDSOut);
    std::string GetDenominationsToString(int nDenom);
    bool GetDenominationsBits(int nDenom, std::vector<int> &vecBitsRet);

    void SetMinBlockSpacing(int nMinBlockSpacingIn) { nMinBlockSpacing = nMinBlockSpacingIn; }

    void ResetPool();

    void UnlockCoins();

    int GetQueueSize() const { return vecDarksendQueue.size(); }
    int GetState() const { return nState; }
    std::string GetStateString() const;
    std::string GetStatus();

    int GetEntriesCount() const { return vecEntries.size(); }

    /// Passively run mixing in the background according to the configuration in settings
    bool DoAutomaticDenominating(bool fDryRun=false);

    void CheckTimeout();
    void CheckForCompleteQueue();

    /// Process a new block
    void NewBlock();

    void UpdatedBlockTip(const CBlockIndex *pindex);
};

void ThreadCheckDarkSendPool();

#endif
