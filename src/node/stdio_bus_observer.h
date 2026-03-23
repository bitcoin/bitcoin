// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_STDIO_BUS_OBSERVER_H
#define BITCOIN_NODE_STDIO_BUS_OBSERVER_H

#include <node/stdio_bus_hooks.h>
#include <validationinterface.h>

#include <memory>

namespace node {

/**
 * @brief CValidationInterface observer for stdio_bus hooks
 * 
 * This observer forwards validation events to StdioBusHooks.
 * It follows the same pattern as CZMQNotificationInterface.
 * 
 * Thread safety: Callbacks are invoked on background thread.
 * All hook calls must be non-blocking.
 */
class StdioBusValidationObserver : public CValidationInterface
{
public:
    explicit StdioBusValidationObserver(std::shared_ptr<StdioBusHooks> hooks);
    virtual ~StdioBusValidationObserver() = default;

    // Non-copyable
    StdioBusValidationObserver(const StdioBusValidationObserver&) = delete;
    StdioBusValidationObserver& operator=(const StdioBusValidationObserver&) = delete;

    /** Check if observer is enabled */
    bool Enabled() const { return m_hooks && m_hooks->Enabled(); }

protected:
    // CValidationInterface callbacks

    /**
     * Called after block validation completes (accepted or rejected).
     * This is the primary hook point for OnBlockValidated.
     */
    void BlockChecked(const std::shared_ptr<const CBlock>& block,
                      const BlockValidationState& state) override;

    /**
     * Called when a block is connected to the active chain.
     * Can be used for additional timing metrics.
     */
    void BlockConnected(const kernel::ChainstateRole& role,
                        const std::shared_ptr<const CBlock>& block,
                        const CBlockIndex* pindex) override;

    /**
     * Called when a transaction is added to mempool.
     * Used for OnTxAdmission hook.
     */
    void TransactionAddedToMempool(const NewMempoolTransactionInfo& tx,
                                   uint64_t mempool_sequence) override;

private:
    std::shared_ptr<StdioBusHooks> m_hooks;
    
    // Track block receive times for latency calculation
    // Key: block hash, Value: receive timestamp in microseconds
    // Note: This is a simplified approach; production code might use
    // a bounded LRU cache to prevent unbounded growth
    mutable std::map<uint256, int64_t> m_block_receive_times;
    mutable std::mutex m_receive_times_mutex;
};

} // namespace node

#endif // BITCOIN_NODE_STDIO_BUS_OBSERVER_H
