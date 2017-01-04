// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_AUXILIARYBLOCKREQUEST_H
#define BITCOIN_AUXILIARYBLOCKREQUEST_H

#include <atomic>
#include "chain.h"
#include "consensus/consensus.h"
#include "net.h"
#include <stdint.h>
#include <vector>

// "Lock free" auxiliary block request
class CAuxiliaryBlockRequest : public std::enable_shared_from_this<CAuxiliaryBlockRequest> {
public:
    std::atomic<size_t> requestedUpToSize; //requested up to this index in vBlocksToDownload
    std::atomic<size_t> processedUpToSize; //processed up to this index in vBlocksToDownload

    const std::vector<const CBlockIndex*> vBlocksToDownload;
    const int64_t created; //!timestamp when the block request was created
    const bool passThroughSignals; //!if passThroughSignals is set, the received blocks transaction will be passed through the SyncTransaction signal */

    /** Constructor of the lock free CAuxiliaryBlockRequest, vBlocksToDownloadIn remains constant */
    CAuxiliaryBlockRequest(std::vector<const CBlockIndex*> vBlocksToDownloadIn, int64_t created, bool passThroughSignalsIn, const std::function<bool(std::shared_ptr<CAuxiliaryBlockRequest>, const CBlockIndex *pindex)> progressCallbackIn);

    /** Process the request, check if there are blocks available to "stream"
        over the SyncTransaction signal 
        Allow to provide an optional block to avoid disk re-loading
     */
    void processWithPossibleBlock(const std::shared_ptr<const CBlock> pblock = nullptr, CBlockIndex *pindex = NULL);

    /** Cancel the block request */
    void cancel();
    bool isCancelled();

    /** Set as the current block request, invalidate/cancle the current one */
    void setAsCurrentRequest();

    /** Fill next available, not already requested blocks into vBlocks
        allow to provide a function to check if block is already in flight somewhere */
    void fillInNextBlocks(std::vector<const CBlockIndex*>& vBlocks, unsigned int count, std::function<bool(const CBlockIndex*)> filterBlocksCallback);

    /** returns the amount of already loaded/local-stored blocks from this blockrequest */
    size_t amountOfBlocksLoaded();

    /** returns true if all blocks have been downloaded & processed */
    bool isCompleted();

    /** Get the current main blockrequest, thread_safe */
    static std::shared_ptr<CAuxiliaryBlockRequest> GetCurrentRequest();

private:
    void NotifyUI();
    const std::function<bool(std::shared_ptr<CAuxiliaryBlockRequest>, const CBlockIndex *pindex)> progressCallback; //! progress callback, with optional cancle mechanism (return false == cancel)
    std::atomic<bool> fCancelled;
};

#endif // BITCOIN_AUXILIARYBLOCKREQUEST_H
