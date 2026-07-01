// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXRELAY_H
#define BITCOIN_NODE_TXRELAY_H

#include <common/bloom.h>
#include <consensus/amount.h>
#include <merkleblock.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <sync.h>
#include <uint256.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <utility>
#include <vector>

namespace node {

class TxRelay
{
private:
    mutable RecursiveMutex m_bloom_filter_mutex;
    /** Whether we relay transactions to this peer. */
    bool m_relay_txs GUARDED_BY(m_bloom_filter_mutex){false};
    /** A bloom filter for which transactions to announce to the peer. See BIP37. */
    std::unique_ptr<CBloomFilter> m_bloom_filter PT_GUARDED_BY(m_bloom_filter_mutex) GUARDED_BY(m_bloom_filter_mutex){nullptr};

    mutable RecursiveMutex m_tx_inventory_mutex;
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

public:
    bool GetRelayTxs() const EXCLUSIVE_LOCKS_REQUIRED(!m_bloom_filter_mutex)
    {
        return WITH_LOCK(m_bloom_filter_mutex, return m_relay_txs);
    }

    void SetRelayTxs(bool relay_txs) EXCLUSIVE_LOCKS_REQUIRED(!m_bloom_filter_mutex)
    {
        LOCK(m_bloom_filter_mutex);
        m_relay_txs = relay_txs;
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

    /** Compute a BIP37 merkle block from `block`, filtered through this
     *  peer's bloom filter, or std::nullopt if no filter is set. Matched
     *  transactions are added to the filter (see CMerkleBlock). */
    std::optional<CMerkleBlock> MakeMerkleBlock(const CBlock& block) EXCLUSIVE_LOCKS_REQUIRED(!m_bloom_filter_mutex)
    {
        LOCK(m_bloom_filter_mutex);
        if (!m_bloom_filter) return std::nullopt;
        return CMerkleBlock{block, *m_bloom_filter};
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

    void SetLastInvSequence(uint64_t sequence) EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        LOCK(m_tx_inventory_mutex);
        m_last_inv_sequence = sequence;
    }

    void SetSendMempool() EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        LOCK(m_tx_inventory_mutex);
        m_send_mempool = true;
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

    void SetNextInvSendTime(std::chrono::microseconds next_time) EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        LOCK(m_tx_inventory_mutex);
        m_next_inv_send_time = next_time;
    }

    /** Queue a transaction for relay if the peer doesn't already know about
     *  it. Only queue transactions for announcement once the version handshake
     *  is completed. The time of arrival for these transactions is otherwise
     *  at risk of leaking to a spy, if the spy is able to distinguish
     *  transactions received during the handshake from the rest in the
     *  announcement. */
    void PushInventory(const uint256& hash, const Wtxid& wtxid) EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        LOCK(m_tx_inventory_mutex);
        if (m_next_inv_send_time == std::chrono::microseconds{0}) return;
        if (!m_tx_inventory_known_filter.contains(hash)) {
            m_tx_inventory_to_send.insert(wtxid);
        }
    }

    /** Returns true if the inventory is empty and no send has been scheduled. */
    bool IsInventoryPristine() const EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        LOCK(m_tx_inventory_mutex);
        return m_tx_inventory_to_send.empty() && m_next_inv_send_time == std::chrono::microseconds{0};
    }

    class TxInventoryBatch
    {
        friend class TxRelay;

        TxInventoryBatch() = default;

        bool m_send_trickle{false};
        bool m_send_mempool{false};
        bool m_inv_send_time_reached{false};
        CAmount m_fee_filter_received{0};
        std::set<Wtxid> m_tx_inventory_to_send;

    public:
        TxInventoryBatch(const TxInventoryBatch&) = delete;
        TxInventoryBatch& operator=(const TxInventoryBatch&) = delete;
        TxInventoryBatch(TxInventoryBatch&&) noexcept = default;
        TxInventoryBatch& operator=(TxInventoryBatch&&) noexcept = default;

        bool SendTrickle() const { return m_send_trickle; }
        bool SendMempool() const { return m_send_mempool; }
        bool InvSendTimeReached() const { return m_inv_send_time_reached; }
        CAmount FeeFilterReceived() const { return m_fee_filter_received; }
        std::vector<Wtxid> QueuedCandidates() const { return {m_tx_inventory_to_send.begin(), m_tx_inventory_to_send.end()}; }
        void EraseQueued(const Wtxid& wtxid) { m_tx_inventory_to_send.erase(wtxid); }
    };

    TxInventoryBatch StartTxInventoryBatch(bool send_trickle, std::chrono::microseconds current_time)
        EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        TxInventoryBatch batch;
        batch.m_fee_filter_received = m_fee_filter_received.load();

        LOCK(m_tx_inventory_mutex);
        if (m_next_inv_send_time < current_time) {
            send_trickle = true;
            batch.m_inv_send_time_reached = true;
            m_next_inv_send_time = current_time;
        }
        batch.m_send_trickle = send_trickle;
        if (!send_trickle) return batch;

        {
            LOCK(m_bloom_filter_mutex);
            if (!m_relay_txs) m_tx_inventory_to_send.clear();
        }

        batch.m_send_mempool = m_send_mempool;
        m_send_mempool = false;
        // New inventory can be queued while this batch is processed. Unsent
        // entries from the batch are merged back by ReturnTxInventory(). If the
        // caller unwinds before that, the batch's unsent entries are dropped;
        // SendMessages() does not recover from that exceptional path.
        batch.m_tx_inventory_to_send.swap(m_tx_inventory_to_send);
        return batch;
    }

    TxInventoryBatch StartTxInventoryBatch(bool send_trickle)
        EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex, !m_bloom_filter_mutex)
    {
        return StartTxInventoryBatch(send_trickle, std::chrono::microseconds::min());
    }

    void ReturnTxInventory(TxInventoryBatch&& batch) EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        if (batch.m_tx_inventory_to_send.empty()) return;

        LOCK(m_tx_inventory_mutex);
        m_tx_inventory_to_send.merge(batch.m_tx_inventory_to_send);
    }

    bool TxInventoryKnownContains(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(!m_tx_inventory_mutex)
    {
        LOCK(m_tx_inventory_mutex);
        return m_tx_inventory_known_filter.contains(hash);
    }

    bool IsTxRelevantAndUpdate(const CTransaction& tx) EXCLUSIVE_LOCKS_REQUIRED(!m_bloom_filter_mutex)
    {
        LOCK(m_bloom_filter_mutex);
        return !m_bloom_filter || m_bloom_filter->IsRelevantAndUpdate(tx);
    }

    CAmount GetFeeFilterReceived() const
    {
        return m_fee_filter_received.load();
    }

    void SetFeeFilterReceived(CAmount fee_filter_received)
    {
        m_fee_filter_received = fee_filter_received;
    }
};

} // namespace node

#endif // BITCOIN_NODE_TXRELAY_H
