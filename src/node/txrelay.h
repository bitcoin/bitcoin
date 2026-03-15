// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXRELAY_H
#define BITCOIN_NODE_TXRELAY_H

#include <common/bloom.h>
#include <consensus/amount.h>
#include <primitives/transaction_identifier.h>
#include <sync.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <set>
#include <span>

namespace node {
/**
 * Per-peer state for transaction relay. Manages bloom filter and
 * transaction inventory data, each protected by its own mutex.
 */
struct TxRelay {
    mutable Mutex m_bloom_filter_mutex;
    /** Whether we relay transactions to this peer. */
    bool m_relay_txs GUARDED_BY(m_bloom_filter_mutex){false};
    /** A bloom filter for which transactions to announce to the peer. See BIP37. */
    std::unique_ptr<CBloomFilter> m_bloom_filter PT_GUARDED_BY(m_bloom_filter_mutex) GUARDED_BY(m_bloom_filter_mutex){nullptr};

    mutable Mutex m_tx_inventory_mutex;
    /** A filter of all the (w)txids that the peer has announced to
     *  us or we have announced to the peer. We use this to avoid announcing
     *  the same (w)txid to a peer that already has the transaction. */
    CRollingBloomFilter m_tx_inventory_known_filter GUARDED_BY(m_tx_inventory_mutex){50000, 0.000001};
    /** Set of wtxids we still have to announce. For non-wtxid-relay peers,
     *  we retrieve the txid from the corresponding mempool transaction when
     *  constructing the `inv` message. We use the mempool to sort transactions
     *  in dependency order before relay, so this does not have to be sorted. */
    std::set<Wtxid> m_tx_inventory_to_send GUARDED_BY(m_tx_inventory_mutex);
    /** Whether the peer has requested us to send our complete mempool. Only
     *  permitted if the peer has NetPermissionFlags::Mempool or we advertise
     *  NODE_BLOOM. See BIP35. */
    bool m_send_mempool GUARDED_BY(m_tx_inventory_mutex){false};
    /** The next time after which we will send an `inv` message containing
     *  transaction announcements to this peer. */
    std::chrono::microseconds m_next_inv_send_time GUARDED_BY(m_tx_inventory_mutex){0};
    /** The mempool sequence num at which we sent the last `inv` message to this peer.
     *  Can relay txs with lower sequence numbers than this (see CTxMempool::info_for_relay). */
    uint64_t m_last_inv_sequence GUARDED_BY(m_tx_inventory_mutex){1};

    /** Minimum fee rate with which to filter transaction announcements to this node. See BIP133. */
    std::atomic<CAmount> m_fee_filter_received{0};

    bool GetRelayTxs() const EXCLUSIVE_LOCKS_REQUIRED(!m_bloom_filter_mutex)
    {
        return WITH_LOCK(m_bloom_filter_mutex, return m_relay_txs);
    }

    void SetRelayTxs(bool relay_txs) EXCLUSIVE_LOCKS_REQUIRED(!m_bloom_filter_mutex)
    {
        LOCK(m_bloom_filter_mutex);
        m_relay_txs = relay_txs;
    }

    void AddKnownTx(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        LOCK(m_tx_inventory_mutex);
        m_tx_inventory_known_filter.insert(hash);
    }

    uint64_t GetLastInvSequence() const EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        return WITH_LOCK(m_tx_inventory_mutex, return m_last_inv_sequence);
    }

    void SetSendMempool() EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        LOCK(m_tx_inventory_mutex);
        m_send_mempool = true;
    }

    void SetBloomFilter(CBloomFilter filter) EXCLUSIVE_LOCKS_REQUIRED(!m_bloom_filter_mutex)
    {
        LOCK(m_bloom_filter_mutex);
        m_bloom_filter = std::make_unique<CBloomFilter>(std::move(filter));
        m_relay_txs = true;
    }

    /** Returns false if no bloom filter is set. */
    bool AddToBloomFilter(std::span<const unsigned char> data) EXCLUSIVE_LOCKS_REQUIRED(!m_bloom_filter_mutex)
    {
        LOCK(m_bloom_filter_mutex);
        if (!m_bloom_filter) return false;
        m_bloom_filter->insert(data);
        return true;
    }

    void ClearBloomFilter() EXCLUSIVE_LOCKS_REQUIRED(!m_bloom_filter_mutex)
    {
        LOCK(m_bloom_filter_mutex);
        m_bloom_filter = nullptr;
        m_relay_txs = true;
    }

    struct InventoryStats {
        uint64_t m_last_inv_seq;
        size_t m_inv_to_send;
    };

    InventoryStats GetInventoryStats() const EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        LOCK(m_tx_inventory_mutex);
        return {m_last_inv_sequence, m_tx_inventory_to_send.size()};
    }

    /** Queue a transaction for relay if the version handshake is complete
     *  and the peer doesn't already know about it. */
    void PushInventory(const uint256& hash, const Wtxid& wtxid) EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        LOCK(m_tx_inventory_mutex);
        if (m_next_inv_send_time == 0s) return;
        if (!m_tx_inventory_known_filter.contains(hash)) {
            m_tx_inventory_to_send.insert(wtxid);
        }
    }

    /** Returns true if the inventory is empty and no send has been scheduled. */
    bool VerifyInventoryPristine() const EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        LOCK(m_tx_inventory_mutex);
        return m_tx_inventory_to_send.empty() && m_next_inv_send_time == 0s;
    }
};
} // namespace node

#endif // BITCOIN_NODE_TXRELAY_H
