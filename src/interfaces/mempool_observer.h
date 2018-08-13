// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_MEMPOOL_OBSERVER_H
#define BITCOIN_INTERFACES_MEMPOOL_OBSERVER_H

#include <uint256.h>

#include <vector>

class CTxMemPoolEntry;

namespace interfaces {

//! Observer interface for mempool operation.
class MempoolObserver
{
public:
    virtual ~MempoolObserver() {}

    /** Called when a block is attached to the chain
      * @param[in] nBlockHeight height of the new block
      * @param[in] entries mempool entries included in the block
      */
    virtual void processBlock(unsigned int nBlockHeight, std::vector<const CTxMemPoolEntry*>& entries) = 0;

    /** Called when a transaction is added to the mempool
     * @param[in] entry entry added to the mempool
     * @param[in] validFeeEstimate whether this entry is a valid basis for estimating fees
     */
    virtual void processTransaction(const CTxMemPoolEntry& entry, bool validFeeEstimate) = 0;

    /** Called when a transaction is removed from the mempool, for reasons other than the tx being in a block
     * @param[in] hash the txid of the removed transaction
     */
    virtual void removeTx(const uint256& hash) = 0;
};

} // namespace interfaces

#endif // BITCOIN_INTERFACES_MEMPOOL_OBSERVER_H
