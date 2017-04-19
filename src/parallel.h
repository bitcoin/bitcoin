// Copyright (c) 2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PARALLEL_H
#define BITCOIN_PARALLEL_H

#include "checkqueue.h"
#include "consensus/validation.h"
#include "main.h"
#include "primitives/block.h"
#include "protocol.h"
#include "serialize.h"
#include "stat.h"
#include "uint256.h"
#include "util.h"
#include <vector>

#include <boost/thread.hpp>


// The number of script check queues we have available.  For every script check queue we can run an
// additional parallel block validation.

extern CCriticalSection cs_blockvalidationthread;

// adds all the script check queues into the "allScriptCheckQueues" global variable and creates nScriptCheckThreads per
// check queue
void AddAllScriptCheckQueuesAndThreads(int nScriptCheckThreads, boost::thread_group *threadGroup);
// Entry point for the script check queue threads.
void AddScriptCheckThreads(int i, CCheckQueue<CScriptCheck> *pqueue);

extern CCriticalSection cs_semPV;
extern CSemaphore *semPV; // semaphore for parallel validation threads

/**
 * Closure representing one script verification
 * Note that this stores references to the spending transaction
 */
class CScriptCheck
{
private:
    ValidationResourceTracker *resourceTracker;
    CScript scriptPubKey;
    const CTransaction *ptxTo;
    unsigned int nIn;
    unsigned int nFlags;
    bool cacheStore;
    ScriptError error;

public:
    CScriptCheck()
        : resourceTracker(NULL), ptxTo(0), nIn(0), nFlags(0), cacheStore(false), error(SCRIPT_ERR_UNKNOWN_ERROR)
    {
    }
    CScriptCheck(ValidationResourceTracker *resourceTrackerIn,
        const CCoins &txFromIn,
        const CTransaction &txToIn,
        unsigned int nInIn,
        unsigned int nFlagsIn,
        bool cacheIn)
        : resourceTracker(resourceTrackerIn), scriptPubKey(txFromIn.vout[txToIn.vin[nInIn].prevout.n].scriptPubKey),
          ptxTo(&txToIn), nIn(nInIn), nFlags(nFlagsIn), cacheStore(cacheIn), error(SCRIPT_ERR_UNKNOWN_ERROR)
    {
    }

    bool operator()();

    void swap(CScriptCheck &check)
    {
        std::swap(resourceTracker, check.resourceTracker);
        scriptPubKey.swap(check.scriptPubKey);
        std::swap(ptxTo, check.ptxTo);
        std::swap(nIn, check.nIn);
        std::swap(nFlags, check.nFlags);
        std::swap(cacheStore, check.cacheStore);
        std::swap(error, check.error);
    }

    ScriptError GetScriptError() const { return error; }
};

/**
 * Hold pointers to all script check queues in one vector
 */
class CAllScriptCheckQueues
{
private:
    std::vector<CCheckQueue<CScriptCheck> *> vScriptCheckQueues;

    CCriticalSection cs;

public:
    CAllScriptCheckQueues() {}
    void Add(CCheckQueue<CScriptCheck> *pqueueIn)
    {
        LOCK(cs);
        vScriptCheckQueues.push_back(pqueueIn);
    }

    uint8_t Size()
    {
        LOCK(cs);
        return vScriptCheckQueues.size();
    }

    CCheckQueue<CScriptCheck> *GetScriptCheckQueue();
};
extern CAllScriptCheckQueues allScriptCheckQueues; // Singleton class

class CParallelValidation
{
private:
    // txn hashes that are in the previous block
    CCriticalSection cs_previousblock;
    std::vector<uint256> vPreviousBlock;


public:
    struct CHandleBlockMsgThreads
    {
        CCheckQueue<CScriptCheck> *pScriptQueue;
        uint256 hash;
        uint256 hashPrevBlock;
        uint32_t nChainWork;
        uint32_t nSequenceId;
        int64_t nStartTime;
        uint64_t nBlockSize;
        bool fQuit;
        NodeId nodeid;
        bool fIsValidating; // is the block currently in connectblock() and validating inputs
        bool fIsReorgInProgress; // has a re-org to another chain been triggered.
    };
    CCriticalSection cs_blockvalidationthread;
    std::map<boost::thread::id, CHandleBlockMsgThreads> mapBlockValidationThreads GUARDED_BY(cs_blockvalidationthread);


public:
    CParallelValidation();


    /* Initialize mapBlockValidationThreads*/
    void InitThread(const boost::thread::id this_id, const CNode *pfrom, const CBlock &block, const CInv &inv);

    /* Initialize a PV session */
    bool Initialize(const boost::thread::id this_id, const CBlockIndex *pindex, const bool fParallel);

    /* Cleanup PV threads after one has finished and won the validation race */
    void Cleanup(const CBlock &block, CBlockIndex *pindex);

    /* Send quit to competing threads */
    void QuitCompetingThreads(const uint256 &prevBlockHash);

    /* Is this block already running a validation thread? */
    bool IsAlreadyValidating(const NodeId id);

    /* Terminate all currently running Block Validation threads, except the passed thread */
    void StopAllValidationThreads(const boost::thread::id this_id = boost::thread::id());
    /* Terminate all currently running Block Validation threads whose chainWork is <= the passed parameter, except the
     * calling thread  */
    void StopAllValidationThreads(const uint32_t nChainWork);
    void WaitForAllValidationThreadsToStop();

    /* Has parallel block validation been turned on via the config settings */
    bool Enabled();

    /* Clear thread data from mapBlockValidationThreads */
    void Erase(const boost::thread::id this_id);

    /* Was the fQuit flag set to true which causes the PV thread to exit */
    bool QuitReceived(const boost::thread::id this_id, const bool fParallel);

    /* Used to determine if another thread has already updated the utxo and advance the chain tip */
    bool ChainWorkHasChanged(const arith_uint256 &nStartingChainWork);

    /* Set the correct locks and locking order before returning from a PV session */
    void SetLocks(const bool fParallel);

    /* Is there a re-org in progress */
    void IsReorgInProgress(const boost::thread::id this_id, const bool fReorg, const bool fParallel);
    bool IsReorgInProgress();

    /* Clear orphans from the orphan cache that are no longer needed*/
    void ClearOrphanCache(const CBlock &block);

    /* Process a block message */
    void HandleBlockMessage(CNode *pfrom, const std::string &strCommand, const CBlock &block, const CInv &inv);
};
extern CParallelValidation PV; // Singleton class


void HandleBlockMessageThread(CNode *pfrom, const std::string &strCommand, const CBlock &block, const CInv &inv);

#endif // BITCOIN_PARALLEL_H
