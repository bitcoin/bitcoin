// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKREQUEST_H
#define BITCOIN_BLOCKREQUEST_H

#include <atomic>
#include "chain.h"
#include "consensus/consensus.h"
#include "net.h"
#include <stdint.h>
#include <vector>

// "Lock free" block request
class CBlockRequest : public std::enable_shared_from_this<CBlockRequest> {
public:
    std::atomic<size_t> requestedUpToSize; //requested up to this element in vBlocksToDownload
    std::atomic<size_t> processedUpToSize; //processed up to this element in vBlocksToDownload

    const std::vector<CBlockIndex*> vBlocksToDownload;
    const int64_t created; //!When the request was started

    /** Constructor of the lock free CBlockRequest, vBlocksToDownloadIn remains constant */
    CBlockRequest(std::vector<CBlockIndex*> vBlocksToDownloadIn, int64_t created, const std::function<bool(std::shared_ptr<CBlockRequest>, CBlockIndex *pindex)> progressCallbackIn);
    ~CBlockRequest();

    /** Process the request, check if there are blocks available to "stream"
        over the SyncTransaction signal 
        Allow to provide an optional block to avoid disk re-loading
     */
    void processWithPossibleBlock(const CBlock* pblock = NULL, CBlockIndex *pindex = NULL);

    /** Cancel the block request */
    void cancel();
    bool isCancelled();

    /** Set as the current block request, invalidate/cancle the current one */
    void setAsCurrentRequest();

    /** Fill next available, not already requested blocks into vBlocks
        allow to provide a function to check if block is already in flight somewhere */
    void fillInNextBlocks(std::vector<CBlockIndex*>& vBlocks, unsigned int count, std::function<bool(CBlockIndex*)> filterBlocksCallback);

    unsigned int amountOfBlocksLoaded();

    /** Get the current main block request, thread_safe */
    static std::shared_ptr<CBlockRequest> GetCurrentRequest();

private:
    const std::function<bool(std::shared_ptr<CBlockRequest>, CBlockIndex *pindex)> progressCallback; //! progress callback, with optional cancle mechanism (return false == cancel)
    std::atomic<bool> fCancelled;
};

#endif // BITCOIN_BLOCKREQUEST_H
