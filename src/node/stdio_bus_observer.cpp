// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/stdio_bus_observer.h>

#include <chain.h>
#include <consensus/validation.h>
#include <kernel/mempool_entry.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <logging.h>

namespace node {

StdioBusValidationObserver::StdioBusValidationObserver(std::shared_ptr<StdioBusHooks> hooks)
    : m_hooks(std::move(hooks))
{
}

void StdioBusValidationObserver::BlockChecked(
    const std::shared_ptr<const CBlock>& block,
    const BlockValidationState& state)
{
    if (!m_hooks || !m_hooks->Enabled()) return;

    const int64_t validated_us = GetMonotonicTimeUs();
    const uint256 hash = block->GetHash();

    // Try to get receive time for latency calculation
    int64_t received_us = validated_us; // Default to validated time if not tracked
    {
        std::lock_guard<std::mutex> lock(m_receive_times_mutex);
        auto it = m_block_receive_times.find(hash);
        if (it != m_block_receive_times.end()) {
            received_us = it->second;
            m_block_receive_times.erase(it);
        }
    }

    BlockValidatedEvent ev{
        .hash = hash,
        .height = -1, // Height not available in BlockChecked, will be set in BlockConnected
        .tx_count = block->vtx.size(),
        .received_us = received_us,
        .validated_us = validated_us,
        .accepted = state.IsValid(),
        .reject_reason = state.IsValid() ? std::string{} : state.ToString()
    };

    try {
        m_hooks->OnBlockValidated(ev);
    } catch (...) {
        // Fail silently - hooks must not affect consensus
        LogDebug(BCLog::NET, "stdio_bus: OnBlockValidated hook threw exception\n");
    }
}

void StdioBusValidationObserver::BlockConnected(
    const kernel::ChainstateRole& role,
    const std::shared_ptr<const CBlock>& block,
    const CBlockIndex* pindex)
{
    // BlockConnected is called after BlockChecked for accepted blocks.
    // We could emit additional events here if needed, but for now
    // BlockChecked provides the validation result we need.
    
    // Note: pindex->nHeight is available here if we need height info
    // in future iterations.
}

void StdioBusValidationObserver::TransactionAddedToMempool(
    const NewMempoolTransactionInfo& tx_info,
    uint64_t mempool_sequence)
{
    if (!m_hooks || !m_hooks->Enabled()) return;

    const int64_t processed_us = GetMonotonicTimeUs();
    const CTransactionRef& tx = tx_info.info.m_tx;

    TxAdmissionEvent ev{
        .txid = tx->GetHash().ToUint256(),
        .wtxid = tx->GetWitnessHash().ToUint256(),
        .size_bytes = static_cast<size_t>(tx_info.info.m_virtual_transaction_size),
        .received_us = processed_us, // Approximate - actual receive time not available here
        .processed_us = processed_us,
        .accepted = true,
        .reject_reason = {}
    };

    try {
        m_hooks->OnTxAdmission(ev);
    } catch (...) {
        // Fail silently - hooks must not affect consensus
        LogDebug(BCLog::NET, "stdio_bus: OnTxAdmission hook threw exception\n");
    }
}

} // namespace node
