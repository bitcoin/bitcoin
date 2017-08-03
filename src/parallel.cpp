// Copyright (c) 2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "parallel.h"
#include "chainparams.h"
#include "dosman.h"
#include "net.h"
#include "pow.h"
#include "timedata.h"
#include "unlimited.h"
#include "util.h"
#include "utiltime.h"
#include <map>
#include <string>
#include <vector>

#include <boost/thread/thread.hpp>

using namespace std;

static const unsigned int nScriptCheckQueues = 4;
std::unique_ptr<CParallelValidation> PV;

static void HandleBlockMessageThread(CNode *pfrom, const std::string strCommand, const CBlock block, const CInv inv);

static void AddScriptCheckThreads(int i, CCheckQueue<CScriptCheck> *pqueue)
{
    ostringstream tName;
    tName << "bitcoin-scriptchk" << i;
    RenameThread(tName.str().c_str());
    pqueue->Thread();
}

CParallelValidation::CParallelValidation(int threadCount, boost::thread_group *threadGroup)
    : semThreadCount(nScriptCheckQueues)
{
    // A single thread has no parallelism so just use the main thread.  Equivalent to parallel being turned off.
    if (threadCount <= 1)
        threadCount = 0;
    else if (threadCount > MAX_SCRIPTCHECK_THREADS)
        threadCount = MAX_SCRIPTCHECK_THREADS;
    nThreads = threadCount;

    LogPrintf("Using %d threads for script verification\n", threadCount);

    while (QueueCount() < nScriptCheckQueues)
    {
        auto queue = new CCheckQueue<CScriptCheck>(128);

        for (unsigned int i = 0; i < nThreads; i++)
            threadGroup->create_thread(boost::bind(&AddScriptCheckThreads, i + 1, queue));

        vQueues.push_back(queue);
    }
}

CParallelValidation::~CParallelValidation()
{
    for (auto queue : vQueues)
        delete queue;
}

unsigned int CParallelValidation::QueueCount()
{
    // Only modified in constructor so no lock currently needed
    return vQueues.size();
}

bool CParallelValidation::Initialize(const boost::thread::id this_id, const CBlockIndex *pindex, const bool fParallel)
{
    AssertLockHeld(cs_main);

    if (fParallel)
    {
        // If the chain tip has passed this block by, its an orphan.  It cannot be connected to the active chain, so
        // return.
        if (chainActive.Tip()->nChainWork > pindex->nChainWork)
        {
            LogPrintf("returning because chainactive tip is now ahead of chainwork for this block\n");
            return false;
        }

        LOCK(cs_blockvalidationthread);
        assert(mapBlockValidationThreads.count(this_id) > 0);
        CHandleBlockMsgThreads *pValidationThread = &mapBlockValidationThreads[this_id];
        pValidationThread->hash = pindex->GetBlockHash();

        // We need to place a Quit here because we do not want to assign a script queue to a thread of activity
        // if another thread has just won the race and has sent an fQuit.
        if (pValidationThread->fQuit)
        {
            LogPrint("parallel", "fQuit 0 called - Stopping validation of %s and returning\n",
                pValidationThread->hash.ToString());
            return false;
        }

        // Check whether a thread is aleady validating this very same block.  It can happen at times when a block
        // arrives while a previous blocks is still validating or just finishing it's validation and grabs the next
        // block to validate.
        map<boost::thread::id, CParallelValidation::CHandleBlockMsgThreads>::iterator iter =
            mapBlockValidationThreads.begin();
        while (iter != mapBlockValidationThreads.end())
        {
            if ((*iter).second.hash == pindex->GetBlockHash() && (*iter).second.fIsValidating &&
                !(*iter).second.fQuit && (*iter).first != this_id)
            {
                LogPrint("parallel", "Returning because another thread is already validating this block\n");
                return false;
            }
            iter++;
        }

        // Assign the nSequenceId for the block being validated in this thread. cs_main must be locked for lookup on
        // pindex.
        if (pindex->nSequenceId > 0)
            pValidationThread->nSequenceId = pindex->nSequenceId;

        pValidationThread->fIsValidating = true;
    }

    return true;
}

void CParallelValidation::Cleanup(const CBlock &block, CBlockIndex *pindex)
{
    // Swap the block index sequence id's such that the winning block has the lowest id and all other id's
    // are still in their same order relative to each other.
    LOCK(cs_blockvalidationthread);
    {
        boost::thread::id this_id(boost::this_thread::get_id()); // get this thread's id

        // Create a vector sorted by nSequenceId so that we can iterate through in desc order and adjust the
        // nSequenceId values according to which block won the validation race.
        std::vector<std::pair<uint32_t, uint256> > vSequenceId;
        map<boost::thread::id, CHandleBlockMsgThreads>::iterator mi = mapBlockValidationThreads.begin();
        while (mi != mapBlockValidationThreads.end())
        {
            if ((*mi).first != this_id && (*mi).second.hashPrevBlock == block.GetBlockHeader().hashPrevBlock)
                vSequenceId.push_back(make_pair((*mi).second.nSequenceId, (*mi).second.hash));
            mi++;
        }
        std::sort(vSequenceId.begin(), vSequenceId.end());

        std::vector<std::pair<uint32_t, uint256> >::reverse_iterator riter = vSequenceId.rbegin();
        while (riter != vSequenceId.rend())
        {
            // Swap the nSequenceId so that we end up with the lowest index for the winning block.  This is so
            // later if we need to look up pindexMostWork it will be pointing to this winning block.
            if (pindex->nSequenceId > (*riter).first)
            {
                uint32_t nId = pindex->nSequenceId;
                if (nId == 0)
                    nId = 1;
                if ((*riter).first == 0)
                    (*riter).first = 1;
                LogPrint("parallel", "swapping sequence id for block %s before %d after %d\n",
                    block.GetHash().ToString(), pindex->nSequenceId, (*riter).first);
                pindex->nSequenceId = (*riter).first;
                (*riter).first = nId;

                BlockMap::iterator it = mapBlockIndex.find((*riter).second);
                if (it != mapBlockIndex.end())
                    it->second->nSequenceId = nId;
            }
            riter++;
        }
    }
}

void CParallelValidation::QuitCompetingThreads(const uint256 &prevBlockHash)
{
    // Kill other competing threads but not this one.
    LOCK(cs_blockvalidationthread);
    {
        boost::thread::id this_id(boost::this_thread::get_id()); // get this thread's id

        map<boost::thread::id, CHandleBlockMsgThreads>::iterator iter = mapBlockValidationThreads.begin();
        while (iter != mapBlockValidationThreads.end())
        {
            map<boost::thread::id, CHandleBlockMsgThreads>::iterator mi =
                iter++; // increment to avoid iterator becoming
            // Interrupt threads:  We want to stop any threads that have lost the validation race. We have to compare
            //                     at the previous block hashes to make the determination.  If they match then it must
            //                     be a parallel block validation that was happening.
            if ((*mi).first != this_id && (*mi).second.hashPrevBlock == prevBlockHash)
            {
                if ((*mi).second.pScriptQueue != NULL)
                {
                    LogPrint("parallel", "Terminating script queue with blockhash %s and previous blockhash %s\n",
                        (*mi).second.hash.ToString(), prevBlockHash.ToString());
                    // Send Quit to any other scriptcheckques that were running a parallel validation for the same
                    // block.
                    // NOTE: the scriptcheckqueue may or may not have finished, but sending a quit here ensures
                    // that it breaks from its processing loop in the event that it is still at the control.Wait() step.
                    // This allows us to end any long running block validations and allow a smaller block to begin
                    // processing when/if all the queues have been jammed by large blocks during an attack.
                    LogPrint("parallel", "Sending Quit() to scriptcheckqueue\n");
                    (*mi).second.pScriptQueue->Quit();
                }
                (*mi).second.fQuit = true; // quit the thread
                LogPrint("parallel", "interrupting a thread with blockhash %s and previous blockhash %s\n",
                    (*mi).second.hash.ToString(), prevBlockHash.ToString());
            }
        }
    }
}

bool CParallelValidation::IsAlreadyValidating(const NodeId nodeid)
{
    // Don't allow a second thinblock to validate if this node is already in the process of validating a block.
    LOCK(cs_blockvalidationthread);
    map<boost::thread::id, CParallelValidation::CHandleBlockMsgThreads>::iterator iter =
        mapBlockValidationThreads.begin();
    while (iter != mapBlockValidationThreads.end())
    {
        if ((*iter).second.nodeid == nodeid)
        {
            return true;
        }
        iter++;
    }
    return false;
}

void CParallelValidation::StopAllValidationThreads(const boost::thread::id this_id)
{
    LOCK(cs_blockvalidationthread);
    map<boost::thread::id, CHandleBlockMsgThreads>::iterator mi = mapBlockValidationThreads.begin();
    while (mi != mapBlockValidationThreads.end())
    {
        if ((*mi).first != this_id) // we don't want to kill our own thread
        {
            if ((*mi).second.pScriptQueue != NULL)
                (*mi).second.pScriptQueue->Quit(); // quit any active script queue threads
            (*mi).second.fQuit = true; // quit the PV thread
        }
        mi++;
    }
}

void CParallelValidation::StopAllValidationThreads(const uint32_t nChainWork)
{
    boost::thread::id this_id(boost::this_thread::get_id());

    LOCK(cs_blockvalidationthread);
    map<boost::thread::id, CHandleBlockMsgThreads>::iterator mi = mapBlockValidationThreads.begin();
    while (mi != mapBlockValidationThreads.end())
    {
        // Kill any threads that have less than or equal to our own chain work we are working on.  We use
        // this method when we're mining our own block.  In that event we want to give priority to our own
        // block rather than any competing block or chain.
        if (((*mi).first != this_id) && (*mi).second.nChainWork <= nChainWork &&
            (*mi).second.nMostWorkOurFork <= nChainWork)
        {
            if ((*mi).second.pScriptQueue != NULL)
                (*mi).second.pScriptQueue->Quit(); // quit any active script queue threads
            (*mi).second.fQuit = true; // quit the PV thread
        }
        mi++;
    }
}

void CParallelValidation::WaitForAllValidationThreadsToStop()
{
    // Wait for threads to finish and cleanup
    while (true)
    {
        // We must unlock before sleeping so that any blockvalidation threads
        // that are quitting can grab the lock and cleanup
        {
            LOCK(cs_blockvalidationthread);
            if (mapBlockValidationThreads.size() == 0)
                break;
        }
        MilliSleep(100);
    }
}

bool CParallelValidation::Enabled() { return GetBoolArg("-parallel", true); }
void CParallelValidation::InitThread(const boost::thread::id this_id,
    const CNode *pfrom,
    const CBlock &block,
    const CInv &inv)
{
    const CBlockHeader &header = block.GetBlockHeader();

    LOCK(cs_blockvalidationthread);
    assert(mapBlockValidationThreads.count(this_id) == 0); // this id should not already be in use
    mapBlockValidationThreads[this_id].pScriptQueue = NULL;
    mapBlockValidationThreads[this_id].hash = inv.hash;
    mapBlockValidationThreads[this_id].hashPrevBlock = header.hashPrevBlock;
    mapBlockValidationThreads[this_id].nChainWork = header.nBits;
    mapBlockValidationThreads[this_id].nMostWorkOurFork = header.nBits;
    mapBlockValidationThreads[this_id].nSequenceId = INT_MAX;
    mapBlockValidationThreads[this_id].nStartTime = GetTimeMillis();
    mapBlockValidationThreads[this_id].nBlockSize = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
    mapBlockValidationThreads[this_id].fQuit = false;
    mapBlockValidationThreads[this_id].nodeid = pfrom->id;
    mapBlockValidationThreads[this_id].fIsValidating = false;
    mapBlockValidationThreads[this_id].fIsReorgInProgress = false;
    LogPrint("parallel", "Launching validation for %s with number of block validation threads running: %d\n",
        block.GetHash().ToString(), mapBlockValidationThreads.size());
}

void CParallelValidation::Erase(const boost::thread::id this_id)
{
    LOCK(cs_blockvalidationthread);
    if (mapBlockValidationThreads.count(this_id))
        mapBlockValidationThreads.erase(this_id);
}

bool CParallelValidation::QuitReceived(const boost::thread::id this_id, const bool fParallel)
{
    if (fParallel)
    {
        LOCK(cs_blockvalidationthread);
        if (mapBlockValidationThreads.count(this_id))
        {
            if (mapBlockValidationThreads[this_id].fQuit)
            {
                LogPrint("parallel", "fQuit called - Stopping validation of this block and returning\n");
                return true;
            }
        }
    }
    return false;
}

bool CParallelValidation::ChainWorkHasChanged(const arith_uint256 &nStartingChainWork)
{
    LOCK(cs_main);
    if (chainActive.Tip()->nChainWork != nStartingChainWork)
    {
        LogPrint("parallel", "Quitting - Chain Work %s is not the same as the starting Chain Work %s\n",
            chainActive.Tip()->nChainWork.ToString(), nStartingChainWork.ToString());
        return true;
    }
    return false;
}

void CParallelValidation::SetLocks(const bool fParallel)
{
    if (fParallel)
    {
        // cs_main must be re-locked before returning from ConnectBlock()
        ENTER_CRITICAL_SECTION(cs_main);
        boost::thread::id this_id(boost::this_thread::get_id());
        LOCK(cs_blockvalidationthread);
        if (mapBlockValidationThreads.count(this_id))
            mapBlockValidationThreads[this_id].pScriptQueue = NULL;
    }
}

void CParallelValidation::IsReorgInProgress(const boost::thread::id this_id, const bool fReorg, const bool fParallel)
{
    if (fParallel)
    {
        LOCK(cs_blockvalidationthread);
        if (mapBlockValidationThreads.count(this_id))
            mapBlockValidationThreads[this_id].fIsReorgInProgress = fReorg;
    }
}

bool CParallelValidation::IsReorgInProgress()
{
    LOCK(cs_blockvalidationthread);
    map<boost::thread::id, CHandleBlockMsgThreads>::iterator mi = mapBlockValidationThreads.begin();
    while (mi != mapBlockValidationThreads.end())
    {
        if ((*mi).second.fIsReorgInProgress)
            return true;
        mi++;
    }
    return false;
}

void CParallelValidation::UpdateMostWorkOurFork(const CBlockHeader &header)
{
    LOCK(cs_blockvalidationthread);
    map<boost::thread::id, CHandleBlockMsgThreads>::iterator mi = mapBlockValidationThreads.begin();
    while (mi != mapBlockValidationThreads.end())
    {
        // check if this new header connects to this block and if so then update the nMostWorkOurFork
        if ((*mi).second.hash == header.hashPrevBlock && (*mi).second.nMostWorkOurFork < header.nBits)
            (*mi).second.nMostWorkOurFork = header.nBits;
        mi++;
    }
}

uint32_t CParallelValidation::MaxWorkChainBeingProcessed()
{
    uint32_t nMaxWork = 0;
    LOCK(cs_blockvalidationthread);
    map<boost::thread::id, CHandleBlockMsgThreads>::iterator mi = mapBlockValidationThreads.begin();
    while (mi != mapBlockValidationThreads.end())
    {
        if ((*mi).second.nChainWork > nMaxWork)
            nMaxWork = (*mi).second.nChainWork;
        mi++;
    }
    return nMaxWork;
}

void CParallelValidation::ClearOrphanCache(const CBlock &block)
{
    if (!IsInitialBlockDownload())
    {
        LOCK(cs_orphancache);
        {
            // Erase any orphans that may have been in the previous block and arrived
            // after the previous block had already been processed.
            LOCK(cs_previousblock);
            for (unsigned int i = 0; i < vPreviousBlock.size(); i++)
            {
                EraseOrphanTx(vPreviousBlock[i]);
            }
            vPreviousBlock.clear();

            // Erase orphans from the current block that were already received.
            for (unsigned int i = 0; i < block.vtx.size(); i++)
            {
                uint256 hash = block.vtx[i].GetHash();
                vPreviousBlock.push_back(hash);
                EraseOrphanTx(hash);
            }
        }
    }
}


//  HandleBlockMessage launches a HandleBlockMessageThread.  And HandleBlockMessageThread processes each block and
//  updates
//  the UTXO if the block has been accepted and the tip updated. We cleanup and release the semaphore after the thread
//  has finished.
void CParallelValidation::HandleBlockMessage(CNode *pfrom,
    const string &strCommand,
    const CBlock &block,
    const CInv &inv)
{
    uint64_t nBlockSize = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);

    // NOTE: You must not have a cs_main lock before you aquire the semaphore grant or you can end up deadlocking
    // AssertLockNotHeld(cs_main); TODO: need to create this

    // Aquire semaphore grant
    if (IsChainNearlySyncd())
    {
        if (!semThreadCount.try_wait())
        {
            /** The following functionality is for the case when ALL thread queues and grants are in use, meaning
             * somehow an attacker
             *  has been able to craft blocks or sustain an attack in such a way as to use up every availabe thread
             * queue.
             *  When/If that should occur, we must assume we are under a sustained attack and we will have to make a
             * determination
             *  as to which of the currently running threads we should terminate based on the following critera:
             *     1) If all queues are in use and another block arrives, then terminate the running thread validating
             * the largest block
             */

            {
                LOCK(cs_blockvalidationthread);
                if (mapBlockValidationThreads.size() >= nScriptCheckQueues)
                {
                    uint64_t nLargestBlockSize = 0;
                    map<boost::thread::id, CParallelValidation::CHandleBlockMsgThreads>::iterator miLargestBlock =
                        mapBlockValidationThreads.end();
                    map<boost::thread::id, CParallelValidation::CHandleBlockMsgThreads>::iterator iter =
                        mapBlockValidationThreads.begin();
                    while (iter != mapBlockValidationThreads.end())
                    {
                        // Find largest block where the previous block hash matches. Meaning this is a new block and
                        // it's a competitor to to your block.
                        if ((*iter).second.hashPrevBlock == block.GetBlockHeader().hashPrevBlock)
                        {
                            if ((*iter).second.nBlockSize > nLargestBlockSize)
                            {
                                nLargestBlockSize = (*iter).second.nBlockSize;
                                miLargestBlock = iter;
                            }
                        }
                        iter++;
                    }

                    // if your block is the biggest or of equal size to the biggest then reject it.
                    if (nLargestBlockSize <= nBlockSize)
                    {
                        LogPrint("parallel",
                            "Block validation terminated - Too many blocks currently being validated: %s\n",
                            block.GetHash().ToString());
                        return; // new block is rejected and does not enter PV
                    }
                    else if (miLargestBlock != mapBlockValidationThreads.end())
                    { // terminate the chosen PV thread
                        (*miLargestBlock).second.pScriptQueue->Quit(); // terminate the script queue threads
                        LogPrint("parallel", "Sending Quit() to scriptcheckqueue\n");
                        (*miLargestBlock).second.fQuit = true; // terminate the PV thread
                        LogPrint("parallel", "Too many blocks being validated, interrupting thread with blockhash %s "
                                             "and previous blockhash %s\n",
                            (*miLargestBlock).second.hash.ToString(),
                            (*miLargestBlock).second.hashPrevBlock.ToString());
                    }
                }
            } // We must not hold the lock here because we could be waiting for a grant, below.

            // wait for semaphore grant
            semThreadCount.wait();
        }
    }
    else
    { // for IBD just wait for the next available
        semThreadCount.wait();
    }

    // Add a reference here because we are detaching a thread which may run for a long time and
    // we do not want CNode to be deleted if the node should disconnect while we are processing this block.
    // We will clean up this reference when the thread finishes.
    {
        LOCK(cs_vNodes);
        pfrom->AddRef();
    }

    // only launch block validation in a separate thread if PV is enabled.
    if (PV->Enabled())
    {
        boost::thread thread(boost::bind(&HandleBlockMessageThread, pfrom, strCommand, block, inv));
        thread.detach(); // Separate actual thread from the "thread" object so its fine to fall out of scope
    }
    else
    {
        HandleBlockMessageThread(pfrom, strCommand, block, inv);
    }
}

void HandleBlockMessageThread(CNode *pfrom, const string strCommand, const CBlock block, const CInv inv)
{
    int64_t startTime = GetTimeMicros();
    CValidationState state;
    uint64_t nSizeBlock = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);

    // Indicate that the thinblock was fully received. At this point we have either a block or a fully reconstructed
    // thinblock but we still need to maintain a mapThinBlocksInFlight entry so that we don't re-request a full block
    // from the same node while the block is processing.
    {
        LOCK(pfrom->cs_mapthinblocksinflight);
        if (pfrom->mapThinBlocksInFlight.count(inv.hash))
            pfrom->mapThinBlocksInFlight[inv.hash].fReceived = true;
    }


    boost::thread::id this_id(boost::this_thread::get_id());
    PV->InitThread(this_id, pfrom, block, inv); // initialize the mapBlockValidationThread entries

    // Process all blocks from whitelisted peers, even if not requested,
    // unless we're still syncing with the network.
    // Such an unrequested block may still be processed, subject to the
    // conditions in AcceptBlock().
    bool forceProcessing = pfrom->fWhitelisted && !IsInitialBlockDownload();
    const CChainParams &chainparams = Params();
    if (PV->Enabled())
    {
        ProcessNewBlock(state, chainparams, pfrom, &block, forceProcessing, NULL, true);
    }
    else
    {
        LOCK(cs_main); // locking cs_main here prevents any other thread from beginning starting a block validation.
        ProcessNewBlock(state, chainparams, pfrom, &block, forceProcessing, NULL, false);
    }

    int nDoS;
    if (state.IsInvalid(nDoS))
    {
        LogPrintf("Invalid block due to %s\n", state.GetRejectReason().c_str());
        if (!strCommand.empty())
        {
            pfrom->PushMessage("reject", strCommand, state.GetRejectCode(),
                state.GetRejectReason().substr(0, MAX_REJECT_MESSAGE_LENGTH), inv.hash);
            if (nDoS > 0)
                dosMan.Misbehaving(pfrom, nDoS);
        }

        // the current fork is bad due to this block so reset the best header to the best fully-validated block
        // so we can download another fork of headers (and blocks).
        CBlockIndex *mostWork = FindMostWorkChain();
        CBlockIndex *tip = chainActive.Tip();
        if (mostWork && tip && (mostWork->nChainWork > tip->nChainWork))
        {
            pindexBestHeader = mostWork;
        }
        else
        {
            pindexBestHeader = tip;
        }
    }
    else
    {
        LargestBlockSeen(nSizeBlock); // update largest block seen

        double nValidationTime = (double)(GetTimeMicros() - startTime) / 1000000.0;
        if (strCommand != NetMsgType::BLOCK)
        {
            LogPrint("thin", "Processed Block %s reconstructed from (%s) in %.2f seconds, peer=%s\n",
                inv.hash.ToString(), strCommand, (double)(GetTimeMicros() - startTime) / 1000000.0,
                pfrom->GetLogName());
            thindata.UpdateValidationTime(nValidationTime);
        }
        else
            LogPrint("thin", "Processed Regular Block %s in %.2f seconds, peer=%s\n", inv.hash.ToString(),
                (double)(GetTimeMicros() - startTime) / 1000000.0, pfrom->GetLogName());
    }

    // When we request a thinblock we may get back a regular block if it is smaller than a thinblock
    // Therefore we have to remove the thinblock in flight if it exists and we also need to check that
    // the block didn't arrive from some other peer.  This code ALSO cleans up the thin block that
    // was passed to us (&block), so do not use it after this.
    {
        int nTotalThinBlocksInFlight = 0;
        {
            LOCK2(cs_vNodes, pfrom->cs_mapthinblocksinflight);

            // Erase this thinblock from the tracking map now that we're done with it.
            if (pfrom->mapThinBlocksInFlight.count(inv.hash))
            {
                // Clear thinblock data and thinblock in flight
                thindata.ClearThinBlockData(pfrom, inv.hash);
            }

            // Count up any other remaining nodes with thinblocks in flight.
            BOOST_FOREACH (CNode *pnode, vNodes)
            {
                if (pnode->mapThinBlocksInFlight.size() > 0)
                    nTotalThinBlocksInFlight++;
            }
            pfrom->firstBlock += 1; // update statistics, requires cs_vNodes
        }

        // When we no longer have any thinblocks in flight then clear our any data
        // just to make sure we don't somehow get growth over time.
        if (nTotalThinBlocksInFlight == 0)
        {
            thindata.ResetThinBlockBytes();

            LOCK(cs_xval);
            setPreVerifiedTxHash.clear();
            setUnVerifiedOrphanTxHash.clear();
        }
    }

    // Erase any txns from the orphan cache that are no longer needed
    PV->ClearOrphanCache(block);

    // Clear thread data - this must be done before the thread completes or else some other new
    // thread may grab the same thread id and we would end up deleting the entry for the new thread instead.
    PV->Erase(this_id);

    // release semaphores depending on whether this was IBD or not.  We can not use IsChainNearlySyncd()
    // because the return value will switch over when IBD is nearly finished and we may end up not releasing
    // the correct semaphore.
    PV->Post();

    // Remove the CNode reference we aquired just before we launched this thread.
    {
        LOCK(cs_vNodes);
        pfrom->Release();
    }

    // If chain is nearly caught up then flush the state after a block is finished processing and the
    // performance timings have been updated.  This way we don't include the flush time in our time to
    // process the block advance the tip.
    if (IsChainNearlySyncd())
        FlushStateToDisk(state, FLUSH_STATE_ALWAYS);
}


// for newly mined block validation, return the first queue not in use.
CCheckQueue<CScriptCheck> *CParallelValidation::GetScriptCheckQueue()
{
    while (true)
    {
        {
            LOCK(cs_blockvalidationthread);

            for (unsigned int i = 0; i < vQueues.size(); i++)
            {
                auto pqueue(vQueues[i]);

                if (pqueue->IsIdle())
                {
                    bool inUse = false;
                    map<boost::thread::id, CParallelValidation::CHandleBlockMsgThreads>::iterator iter =
                        mapBlockValidationThreads.begin();
                    while (iter != mapBlockValidationThreads.end())
                    {
                        if ((*iter).second.pScriptQueue == pqueue)
                        {
                            inUse = true;
                            break;
                        }
                        iter++;
                    }
                    if (!inUse)
                    {
                        pqueue->Quit(false); // set to false because it still may be set to true from last run.

                        // Only assign a pqueue to a validation thread if a validation thread is actually running.
                        // When mining or when bitcoin is first starting there will be no validation threads so we
                        // don't want to assign a pqueue here if that is the case.
                        boost::thread::id this_id(boost::this_thread::get_id());
                        if (mapBlockValidationThreads.count(this_id))
                            mapBlockValidationThreads[this_id].pScriptQueue = pqueue;

                        LogPrint("parallel", "next scriptqueue not in use is %d\n", i);
                        return pqueue;
                    }
                }
            }
        }

        LogPrint("parallel", "Sleeping 50 millis\n");
        MilliSleep(50);
    }
}
