// Copyright (c) 2016 The Talkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <auxiliaryblockrequest.h>

#include <chainparams.h>
#include <validation.h>
#include <validationinterface.h>
#include <ui_interface.h>
#include <logging.h>
#include <exception>

static const unsigned int MAX_BLOCK_TO_PROCESS_PER_ITERATION = 5;

std::shared_ptr<CAuxiliaryBlockRequest> currentBlockRequest; //thread-safe pointer (CAuxiliaryBlockRequest, the object, is also lock-free)

CAuxiliaryBlockRequest::CAuxiliaryBlockRequest(std::vector<const CBlockIndex*> vBlocksToDownloadIn, int64_t createdIn, bool passThroughSignalsIn, const std::function<bool(std::shared_ptr<CAuxiliaryBlockRequest>, const CBlockIndex *pindex)> progressCallbackIn) : vBlocksToDownload(vBlocksToDownloadIn), created(createdIn), passThroughSignals(passThroughSignalsIn), progressCallback(progressCallbackIn)
{
    fCancelled = false;
    requestedUpToSize = 0;
    processedUpToSize = 0;

    NotifyUI();
}

void CAuxiliaryBlockRequest::NotifyUI()
{
    // Notify UI
    uiInterface.NotifyAuxiliaryBlockRequestProgress(this->created, this->vBlocksToDownload.size(), this->amountOfBlocksLoaded(), this->processedUpToSize);
}

void CAuxiliaryBlockRequest::processWithPossibleBlock(const std::shared_ptr<const CBlock> pblock, CBlockIndex *pindex)
{
    // don't process anything if the request was cancled
    if (this->fCancelled)
        return;

    for (unsigned int i = this->processedUpToSize; i < this->vBlocksToDownload.size() ; i++) {
        const CBlockIndex *pindexRequest = this->vBlocksToDownload[i];
        std::shared_ptr<const CBlock> currentBlock;

        // if a block has been passed, check if it's the next item in the sequence
        if (pindex && pblock && pindex == pindexRequest)
            currentBlock = pblock;
        else if (pindexRequest->nStatus & BLOCK_HAVE_DATA) {
            CBlock loadBlock;
            // we should already have this block on disk, process it
            if (!ReadBlockFromDisk(loadBlock, pindexRequest, Params().GetConsensus()))
                throw std::runtime_error(std::string(__func__) + "Can't read block from disk");
            currentBlock = std::make_shared<const CBlock>(loadBlock);
        } else {
            break;
        }

        // fire signal with txns
        if (passThroughSignals) {
            //unsigned int cnt = 0;
            //for(const auto& tx : currentBlock->vtx)
            //{
                //bool valid = ((pindexRequest->nStatus & BLOCK_VALID_MASK) == BLOCK_VALID_MASK);
                GetMainSignals().BlockConnected(pblock, pindexRequest, nullptr);
           //     cnt++;
           // }
        }
        this->processedUpToSize++;

        // log some info
        LogPrint(BCLog::NET, "BlockRequest: proccessed up to %ld of total requested %ld blocks\n", this->processedUpToSize, this->vBlocksToDownload.size());

        NotifyUI();

        if (progressCallback)
            if (!progressCallback(shared_from_this(), pindexRequest))
                this->cancel();

        // release global block request pointer if request has been completed
        if (currentBlockRequest == shared_from_this() && isCompleted())
            currentBlockRequest = nullptr;

        if (i-this->processedUpToSize >= MAX_BLOCK_TO_PROCESS_PER_ITERATION)
            break;
    }
}

void CAuxiliaryBlockRequest::cancel()
{
    fCancelled = true;
    if (currentBlockRequest.get() == this) {
        // release shared pointer
        currentBlockRequest = nullptr;
    }
}

bool CAuxiliaryBlockRequest::isCancelled()
{
    return fCancelled;
}

void CAuxiliaryBlockRequest::setAsCurrentRequest()
{
    // if there is an existing block request, cancle it
    if (currentBlockRequest != nullptr)
        currentBlockRequest->fCancelled = true;

    currentBlockRequest = shared_from_this();
}

void CAuxiliaryBlockRequest::fillInNextBlocks(std::vector<const CBlockIndex*>& vBlocks, unsigned int count, std::function<bool(const CBlockIndex*)> filterBlocksCallback)
{
    for (unsigned int i = this->processedUpToSize; i < this->vBlocksToDownload.size() ; i++) {
        const CBlockIndex *pindex = this->vBlocksToDownload[i];
        if ( filterBlocksCallback(pindex) && !(pindex->nStatus & BLOCK_HAVE_DATA)) {
            // the block was accepted by the filter, add it to the download queue
            vBlocks.push_back(pindex);
            if (vBlocks.size() == count) {
                break;
            }
        }
    }

    //try to process already available blocks through the signal
    this->processWithPossibleBlock(NULL, NULL);
}

size_t CAuxiliaryBlockRequest::amountOfBlocksLoaded()
{
    size_t haveData = 0;
    for (size_t i = 0; i < this->vBlocksToDownload.size() ; i++) {
        const CBlockIndex *pindex = this->vBlocksToDownload[i];
        if (pindex->nStatus & BLOCK_HAVE_DATA)
            haveData++;
    }
    return haveData;
}

bool CAuxiliaryBlockRequest::isCompleted()
{
    return (this->processedUpToSize == this->vBlocksToDownload.size());
}

std::shared_ptr<CAuxiliaryBlockRequest> CAuxiliaryBlockRequest::GetCurrentRequest()
{
    return currentBlockRequest;
}
