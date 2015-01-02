
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DARKSEND_H
#define DARKSEND_H

#include "core.h"
#include "main.h"
#include "masternode.h"
#include "activemasternode.h"

class CTxIn;
class CDarkSendPool;
class CDarkSendSigner;
class CMasterNodeVote;
class CBitcoinAddress;
class CDarksendQueue;
class CDarksendBroadcastTx;
class CActiveMasternode;

#define POOL_MAX_TRANSACTIONS                  3 // wait for X transactions to merge and publish
#define POOL_STATUS_UNKNOWN                    0 // waiting for update
#define POOL_STATUS_IDLE                       1 // waiting for update
#define POOL_STATUS_QUEUE                      2 // waiting in a queue
#define POOL_STATUS_ACCEPTING_ENTRIES          3 // accepting entries
#define POOL_STATUS_FINALIZE_TRANSACTION       4 // master node will broadcast what it accepted
#define POOL_STATUS_SIGNING                    5 // check inputs/outputs, sign final tx
#define POOL_STATUS_TRANSMISSION               6 // transmit transaction
#define POOL_STATUS_ERROR                      7 // error
#define POOL_STATUS_SUCCESS                    8 // success

// status update message constants
#define MASTERNODE_ACCEPTED                    1
#define MASTERNODE_REJECTED                    0
#define MASTERNODE_RESET                       -1

#define DARKSEND_QUEUE_TIMEOUT                 120
#define DARKSEND_SIGNING_TIMEOUT               30

extern CDarkSendPool darkSendPool;
extern CDarkSendSigner darkSendSigner;
extern std::vector<CDarksendQueue> vecDarksendQueue;
extern std::string strMasterNodePrivKey;
extern map<uint256, CDarksendBroadcastTx> mapDarksendBroadcastTxes;
extern CActiveMasternode activeMasternode;

//specific messages for the Darksend protocol
void ProcessMessageDarksend(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

// get the darksend chain depth for a given input
int GetInputDarksendRounds(CTxIn in, int rounds=0);


// An input in the darksend pool
class CDarkSendEntryVin
{
public:
    bool isSigSet;
    CTxIn vin;

    CDarkSendEntryVin()
    {
        isSigSet = false;
        vin = CTxIn();
    }
};

// A clients transaction in the darksend pool
class CDarkSendEntry
{
public:
    bool isSet;
    std::vector<CDarkSendEntryVin> sev;
    int64_t amount;
    CTransaction collateral;
    std::vector<CTxOut> vout;
    CTransaction txSupporting;
    int64_t addedTime;

    CDarkSendEntry()
    {
        isSet = false;
        collateral = CTransaction();
        amount = 0;
    }

    bool Add(const std::vector<CTxIn> vinIn, int64_t amountIn, const CTransaction collateralIn, const std::vector<CTxOut> voutIn)
    {
        if(isSet){return false;}

        BOOST_FOREACH(const CTxIn v, vinIn) {
            CDarkSendEntryVin s = CDarkSendEntryVin();
            s.vin = v;
            sev.push_back(s);
        }
        vout = voutIn;
        amount = amountIn;
        collateral = collateralIn;
        isSet = true;
        addedTime = GetTime();

        return true;
    }

    bool AddSig(const CTxIn& vin)
    {
        BOOST_FOREACH(CDarkSendEntryVin& s, sev) {
            if(s.vin.prevout == vin.prevout && s.vin.nSequence == vin.nSequence){
                if(s.isSigSet){return false;}
                s.vin.scriptSig = vin.scriptSig;
                s.vin.prevPubKey = vin.prevPubKey;
                s.isSigSet = true;

                return true;
            }
        }

        return false;
    }

    bool IsExpired()
    {
        return (GetTime() - addedTime) > DARKSEND_QUEUE_TIMEOUT;// 120 seconds
    }
};

//
// A currently inprogress darksend merge and denomination information
//
class CDarksendQueue
{
public:
    CTxIn vin;
    int64_t time;
    int nDenom;
    bool ready; //ready for submit
    std::vector<unsigned char> vchSig;

    CDarksendQueue()
    {
        nDenom = 0;
        vin = CTxIn();
        time = 0;
        vchSig.clear();
        ready = false;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nDenom);
        READWRITE(vin);
        READWRITE(time);
        READWRITE(ready);
        READWRITE(vchSig);
    )

    bool GetAddress(CService &addr)
    {
        BOOST_FOREACH(CMasterNode mn, vecMasternodes) {
            if(mn.vin == vin){
                addr = mn.addr;
                return true;
            }
        }
        return false;
    }

    bool GetProtocolVersion(int &protocolVersion)
    {
        BOOST_FOREACH(CMasterNode mn, vecMasternodes) {
            if(mn.vin == vin){
                protocolVersion = mn.protocolVersion;
                return true;
            }
        }
        return false;
    }

    bool Sign();
    bool Relay();

    bool IsExpired()
    {
        return (GetTime() - time) > DARKSEND_QUEUE_TIMEOUT;// 120 seconds
    }

    bool CheckSignature();

};

// store darksend tx signature information
class CDarksendBroadcastTx
{
public:
    CTransaction tx;
    CTxIn vin;
    vector<unsigned char> vchSig;
    int64_t sigTime;
};

//
// Helper object for signing and checking signatures
//
class CDarkSendSigner
{
public:
    bool IsVinAssociatedWithPubkey(CTxIn& vin, CPubKey& pubkey);
    bool SetKey(std::string strSecret, std::string& errorMessage, CKey& key, CPubKey& pubkey);
    bool SignMessage(std::string strMessage, std::string& errorMessage, std::vector<unsigned char>& vchSig, CKey key);
    bool VerifyMessage(CPubKey pubkey, std::vector<unsigned char>& vchSig, std::string strMessage, std::string& errorMessage);
};

class CDarksendSession
{

};

//
// Used to keep track of current status of darksend pool
//
class CDarkSendPool
{
public:
    static const int MIN_PEER_PROTO_VERSION = 70052;

    // clients entries
    std::vector<CDarkSendEntry> myEntries;
    // masternode entries
    std::vector<CDarkSendEntry> entries;
    // the finalized transaction ready for signing
    CTransaction finalTransaction;

    int64_t lastTimeChanged;
    int64_t lastAutoDenomination;

    unsigned int state;
    unsigned int entriesCount;
    unsigned int lastEntryAccepted;
    unsigned int countEntriesAccepted;

    // where collateral should be made out to
    CScript collateralPubKey;

    std::vector<CTxIn> lockedCoins;

    uint256 masterNodeBlockHash;

    std::string lastMessage;
    bool completedTransaction;
    bool unitTest;
    CService submittedToMasternode;

    int sessionID;
    int sessionDenom; //Users must submit an denom matching this
    int sessionUsers; //N Users have said they'll join
    bool sessionFoundMasternode; //If we've found a compatible masternode
    int sessionTries;
    int sessionMinRounds;
    int sessionMaxRounds;
    int64_t sessionTotalValue; //used for autoDenom
    std::vector<CTransaction> vecSessionCollateral;

    int lastSplitUpBlock;
    int splitUpInARow; // how many splits we've done since a success?
    int cachedLastSuccess;
    int cachedNumBlocks; //used for the overview screen
    int minBlockSpacing; //required blocks between mixes
    CTransaction txCollateral;

    //debugging data
    std::string strAutoDenomResult;

    std::vector<int64_t> vecDisabledDenominations;

    //incremented whenever a DSQ comes through
    int64_t nDsqCount;

    CDarkSendPool()
    {
        /* DarkSend uses collateral addresses to trust parties entering the pool
            to behave themselves. If they don't it takes their money. */

        lastSplitUpBlock = 0;
        cachedLastSuccess = 0;
        cachedNumBlocks = 0;
        unitTest = false;
        splitUpInARow = 0;
        txCollateral = CTransaction();
        minBlockSpacing = 1;
        nDsqCount = 0;
        vecDisabledDenominations.clear();
        sessionMinRounds = 0;
        sessionMaxRounds = 0;

        SetNull();
    }

    void InitCollateralAddress(){
        std::string strAddress = "";
        if(Params().NetworkID() == CChainParams::MAIN) {
            strAddress = "Xq19GqFvajRrEdDHYRKGYjTsQfpV5jyipF";
        } else {
            strAddress = "y1EZuxhhNMAUofTBEeLqGE1bJrpC2TWRNp";
        }
        SetCollateralAddress(strAddress);
    }

    void SetMinBlockSpacing(int minBlockSpacingIn){
        minBlockSpacing = minBlockSpacingIn;
    }

    bool SetCollateralAddress(std::string strAddress);
    void Reset();
    void SetNull(bool clearEverything=false);

    void UnlockCoins();

    bool IsNull() const
    {
        return (state == POOL_STATUS_ACCEPTING_ENTRIES && entries.empty() && myEntries.empty());
    }

    int GetState() const
    {
        return state;
    }

    int GetEntriesCount() const
    {
        if(fMasterNode){
            return entries.size();
        } else {
            return entriesCount;
        }
    }

    int GetLastEntryAccepted() const
    {
        return lastEntryAccepted;
    }

    int GetCountEntriesAccepted() const
    {
        return countEntriesAccepted;
    }

    int GetMyTransactionCount() const
    {
        return myEntries.size();
    }

    void UpdateState(unsigned int newState)
    {
        if (fMasterNode && (newState == POOL_STATUS_ERROR || newState == POOL_STATUS_SUCCESS)){
            LogPrintf("CDarkSendPool::UpdateState() - Can't set state to ERROR or SUCCESS as a masternode. \n");
            return;
        }

        LogPrintf("CDarkSendPool::UpdateState() == %d | %d \n", state, newState);
        if(state != newState){
            lastTimeChanged = GetTimeMillis();
            if(fMasterNode) {
                RelayDarkSendStatus(darkSendPool.sessionID, darkSendPool.GetState(), darkSendPool.GetEntriesCount(), MASTERNODE_RESET);
            }
        }
        state = newState;
    }

    int GetMaxPoolTransactions()
    {
        //if we're on testnet, just use two transactions per merge
        if(Params().NetworkID() == CChainParams::TESTNET || Params().NetworkID() == CChainParams::REGTEST) return 2;

        //use the production amount
        return POOL_MAX_TRANSACTIONS;
    }

    //Do we have enough users to take entries?
    bool IsSessionReady(){
        return sessionUsers >= GetMaxPoolTransactions();
    }

    // Are these outputs compatible with other client in the pool?
    bool IsCompatibleWithEntries(std::vector<CTxOut> vout);
    // Is this amount compatible with other client in the pool?
    bool IsCompatibleWithSession(int64_t nAmount, CTransaction txCollateral, std::string& strReason);

    // Passively run Darksend in the background according to the configuration in settings (only for QT)
    bool DoAutomaticDenominating(bool fDryRun=false, bool ready=false);
    bool PrepareDarksendDenominate();


    // check for process in Darksend
    void Check();
    // charge fees to bad actors
    void ChargeFees();
    // rarely charge fees to pay miners
    void ChargeRandomFees();
    void CheckTimeout();
    // check to make sure a signature matches an input in the pool
    bool SignatureValid(const CScript& newSig, const CTxIn& newVin);
    // if the collateral is valid given by a client
    bool IsCollateralValid(const CTransaction& txCollateral);
    // add a clients entry to the pool
    bool AddEntry(const std::vector<CTxIn>& newInput, const int64_t& nAmount, const CTransaction& txCollateral, const std::vector<CTxOut>& newOutput, std::string& error);
    // add signature to a vin
    bool AddScriptSig(const CTxIn& newVin);
    // are all inputs signed?
    bool SignaturesComplete();
    // as a client, send a transaction to a masternode to start the denomination process
    void SendDarksendDenominate(std::vector<CTxIn>& vin, std::vector<CTxOut>& vout, int64_t amount);
    // get masternode updates about the progress of darksend
    bool StatusUpdate(int newState, int newEntriesCount, int newAccepted, std::string& error, int newSessionID=0);

    // as a client, check and sign the final transaction
    bool SignFinalTransaction(CTransaction& finalTransactionNew, CNode* node);

    // get block hash by height
    bool GetBlockHash(uint256& hash, int nBlockHeight);
    // get the last valid block hash for a given modulus
    bool GetLastValidBlockHash(uint256& hash, int mod=1, int nBlockHeight=0);
    // process a new block
    void NewBlock();
    void CompletedTransaction(bool error, std::string lastMessageNew);
    void ClearLastMessage();
    // used for liquidity providers
    bool SendRandomPaymentToSelf();
    // split up large inputs or make fee sized inputs
    bool SplitUpMoney(bool justCollateral=false);
    // get the denominations for a list of outputs (returns a bitshifted integer)
    int GetDenominations(const std::vector<CTxOut>& vout);
    void GetDenominationsToString(int nDenom, std::string& strDenom);
    // get the denominations for a specific amount of darkcoin.
    int GetDenominationsByAmount(int64_t nAmount, int nDenomTarget=0);

    int GetDenominationsByAmounts(std::vector<int64_t>& vecAmount);
};


void ConnectToDarkSendMasterNodeWinner();

void ThreadCheckDarkSendPool();

#endif
