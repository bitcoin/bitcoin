// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockrequest.h"

#include "chainparams.h"
#include "main.h"

#include <exception>

std::shared_ptr<CBlockRequest> currentBlockRequest; //thread-safe pointer (CBlockRequest, the object, is also lock-free)

CBlockRequest::CBlockRequest(std::vector<CBlockIndex*> vBlocksToDownloadIn, int64_t createdIn, const std::function<bool(std::shared_ptr<CBlockRequest>, CBlockIndex *pindex)> progressCallbackIn) : vBlocksToDownload(vBlocksToDownloadIn), created(createdIn), progressCallback(progressCallbackIn)
{
    fCancelled = false;
    requestedUpToSize = 0;
    processedUpToSize = 0;
}

CBlockRequest::~CBlockRequest()
{
    LogPrint("net", "Deallocating CBlockRequest\n");
}

void CBlockRequest::processWithPossibleBlock(const CBlock* pblock, CBlockIndex *pindex)
{
    // don't process anything if request was cancled
    if (this->fCancelled)
        return;

    int MAX_PROCESS = 5;
    int loop_processed = 0;
    for (unsigned int i = this->processedUpToSize; i < this->vBlocksToDownload.size() ; i++) {
        CBlockIndex *pindexRequest = this->vBlocksToDownload[i];
        CBlock loadBlock;
        const CBlock *currentBlock = &loadBlock;

        // if a block has been passed, check if is the next item in the sequence
        if (pindex && pblock && pindex == pindexRequest)
            currentBlock = pblock;
        else if (pindexRequest->nStatus & BLOCK_HAVE_DATA) {
            if (!ReadBlockFromDisk(loadBlock, pindexRequest, Params().GetConsensus()))
                throw std::runtime_error(std::string(__func__) + "Can't read block from disk");
        } else {
            break;
        }

        // fire signal with txns
        int cnt = 0;
        BOOST_FOREACH(const CTransaction &tx, currentBlock->vtx) {
            GetMainSignals().SyncTransaction(tx, pindexRequest, cnt, false);
            cnt++;
        }
        this->processedUpToSize++;

        // log some info
        LogPrint("net", "BlockRequest: proccessed up to %ld of total requested %ld blocks\n", this->processedUpToSize, this->vBlocksToDownload.size());

        if (progressCallback)
            if (!progressCallback(shared_from_this(), pindexRequest))
                this->cancel();

        // release global block request pointer if request has been completed
        if (this->processedUpToSize == this->vBlocksToDownload.size())
            currentBlockRequest = nullptr;

        if (loop_processed >= MAX_PROCESS)
            break;
        loop_processed++;
    }
}

void CBlockRequest::cancel()
{
    fCancelled = true;
    if (currentBlockRequest.get() == this) {
        // release shared pointer
        currentBlockRequest = nullptr;
    }
}

bool CBlockRequest::isCancelled()
{
    return fCancelled;
}

void CBlockRequest::setAsCurrentRequest()
{
    // if there is an existing block request, cancle it
    if (currentBlockRequest != nullptr)
        currentBlockRequest->fCancelled = true;

    currentBlockRequest = shared_from_this();
}

void CBlockRequest::fillInNextBlocks(std::vector<CBlockIndex*>& vBlocks, unsigned int count, std::function<bool(CBlockIndex*)> filterBlocksCallback)
{
    for (unsigned int i = this->processedUpToSize; i < this->vBlocksToDownload.size() ; i++) {
        CBlockIndex *pindex = this->vBlocksToDownload[i];
        if ( filterBlocksCallback(pindex) && !(pindex->nStatus & BLOCK_HAVE_DATA)) {
            // the block was accepted by the filter, add it to the download queue
            vBlocks.push_back(pindex);
            if (vBlocks.size() == count) {
                break;
            }
        }
    }

    //try to push already available blocks through the signal
    this->processWithPossibleBlock(NULL, NULL);
}

unsigned int CBlockRequest::amountOfBlocksLoaded()
{
    unsigned int haveData = 0;
    for (unsigned int i = 0; i < this->vBlocksToDownload.size() ; i++) {
        CBlockIndex *pindex = this->vBlocksToDownload[i];
        if (pindex->nStatus & BLOCK_HAVE_DATA)
            haveData++;
    }
    return haveData;
}

std::shared_ptr<CBlockRequest> CBlockRequest::GetCurrentRequest()
{
    return currentBlockRequest;
}
