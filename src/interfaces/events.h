// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_EVENTS_H
#define BITCOIN_INTERFACES_EVENTS_H

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <util/byte_units.h>
#include <util/time.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

class CBlock;
namespace node {
struct NodeContext;
} // namespace node

namespace interfaces {

enum class RemovalReason : uint16_t {
    EXPIRY,
    SIZELIMIT,
    REORG,
    CONFLICT,
    REPLACED,
    BLOCK,
};

struct BlockEvent {
    std::shared_ptr<const CBlock> data;
    uint256 hash;
    int height{-1};
};
struct BlockConnected : BlockEvent {};
struct BlockDisconnected : BlockEvent {};

struct TxAddedToMempool {
    CTransactionRef data;
    Txid txid;
    Wtxid wtxid;
    uint64_t mempool_sequence{0};
    CAmount fee{0};
    int64_t vsize{0};
};

struct TxRemovedFromMempool {
    CTransactionRef data;
    Txid txid;
    Wtxid wtxid;
    uint64_t mempool_sequence{0};
    RemovalReason reason;
};

using EventData = std::variant<BlockConnected, BlockDisconnected, TxAddedToMempool, TxRemovedFromMempool>;

struct Event {
    //! Position in this subscription's event stream, starting at 1 and
    //! incremented by 1 for every event. A jump means events were dropped.
    uint64_t sequence{0};
    EventData data;
};

struct EventOptions {
    bool include_data{false};
};

struct EventsOptions {
    std::optional<EventOptions> block_connected, block_disconnected, tx_added_to_mempool, tx_removed_from_mempool;
};

//! Pull-based stream of chain and mempool events. Events are buffered on the
//! node side from the moment the interface is created, so a slow client never
//! slows down the node. If the client falls too far behind, the oldest events
//! are dropped and the client sees a gap in Event::sequence.
class Events
{
public:
    virtual ~Events() = default;

    //! Discard buffered events with sequence <= after, then wait up to
    //! timeout for at least one event and return up to max_events of them,
    //! also limited to MAX_BATCH_BYTES. Returns an empty list on timeout,
    //! interrupt or shutdown. If the first event's sequence is greater than
    //! after + 1, events were dropped. Throws if after is greater than the
    //! last event's sequence.
    virtual std::vector<Event> waitNext(uint64_t after, uint32_t max_events, MillisecondsDouble timeout) = 0;

    //! Interrupt a pending waitNext call.
    virtual void interrupt() = 0;
};

//! Default -ipceventsbuffer: MiB buffered per subscription before the oldest
//! events are dropped. Memory use is roughly estimated as serialized
//! transaction and block bytes plus a fixed per-event overhead.
inline constexpr int64_t DEFAULT_EVENTS_BUFFER_MIB{128};

//! Max serialized transaction and block bytes plus a fixed per-event overhead
//! returned by one waitNext call, below the 64 MiB Cap'n Proto message limit.
inline constexpr size_t MAX_BATCH_BYTES{16_MiB};

std::unique_ptr<Events> MakeEvents(node::NodeContext& node, const EventsOptions& options);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_EVENTS_H
