// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// End-to-end liveness of parent and child relay (opportunistic 1p1c package relay and orphan
// handling) against adversarial peers, driving a real PeerManager with real P2P messages and real
// validation.
//
// Property: if an honest peer announces the child of an acceptable parent and child, the child ends
// up in our mempool, whatever other peers do. The parent may be too low-fee on its own (1p1c relay)
// or not (the child is then an orphan until the parent arrives). Adversaries hold no keys: they may
// announce, deliver the parent, the child or the confirmed transaction funding the child as is,
// stripped of its witness, inflated or with a freely mutated witness, deliver a fake child of the
// parent, send notfound, stall, disconnect and reconnect. The parent may also leave the mempool
// before the child is accepted. Moves that open a known liveness hole turn off the check (see
// `holes`).
//
// Nothing below PeerManager is modelled: message handling, transaction download, validation ordering
// and result attribution are the production code paths, and the adversary's malleations of the cast
// transactions are not limited to a fixed set. The cast itself is fixed: P2WSH inputs, children with
// one confirmed fee input. The oracle is coarse (mempool membership) and each execution pays for real
// validation. Not modelled: the orphanage reaching its limits, and the tip changing during a run.

#include <addresstype.h>
#include <addrman.h>
#include <chainparams.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <crypto/common.h>
#include <kernel/mempool_removal_reason.h>
#include <net.h>
#include <net_processing.h>
#include <node/txdownloadman.h>
#include <netmessagemaker.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <script/script.h>
#include <streams.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/validation.h>
#include <txmempool.h>
#include <util/time.h>
#include <validation.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;

TestChain100Setup* g_setup;

/** Coins are P2WSH of {OP_DROP OP_TRUE}, spent with a 64-byte dummy item, so that the witness carries
 * enough weight for stripping it to change the feerate materially. */
const CScript WITNESS_SCRIPT_DROP_TRUE{CScript() << OP_DROP << OP_TRUE};
const CScript P2WSH_DROP_TRUE{GetScriptForDestination(WitnessV0ScriptHash(WITNESS_SCRIPT_DROP_TRUE))};
const std::vector<unsigned char> WITNESS_DUMMY(64, 0);

struct Cast {
    CTransactionRef parent;
    CAmount parent_fee;
    CTransactionRef child;      //!< pays for the pair, from the parent's output and an output of CONFIRMED_PARENT
    CTransactionRef fake_child; //!< spends the parent and an unknown outpoint
};
std::vector<Cast> CASTS;
/** Confirmed transaction whose outputs fund the children. Confirmed longer ago than the recently-confirmed
 * filter remembers (a fresh PeerManager per input has an empty one), with its outputs in the coins cache. */
CTransactionRef CONFIRMED_PARENT;

CTransactionRef MakeTx(uint32_t version, const std::vector<COutPoint>& inputs, const std::vector<CAmount>& amounts_out)
{
    CMutableTransaction mtx;
    mtx.version = version;
    for (const auto& outpoint : inputs) {
        mtx.vin.emplace_back(outpoint);
        mtx.vin.back().scriptWitness.stack = {WITNESS_DUMMY, std::vector<unsigned char>(WITNESS_SCRIPT_DROP_TRUE.begin(), WITNESS_SCRIPT_DROP_TRUE.end())};
    }
    for (const auto amount : amounts_out) mtx.vout.emplace_back(amount, P2WSH_DROP_TRUE);
    return MakeTransactionRef(mtx);
}

void initialize()
{
    static const auto testing_setup{MakeNoLogFileContext<TestChain100Setup>(ChainType::REGTEST)};
    g_setup = testing_setup.get();

    // A parent of ~112 vbytes with its witness and ~94 without: 10 sat is below the minimum relay
    // feerate with the witness and above it without, and 200 sat pays for the parent on its own.
    const std::vector<CAmount> parent_fees{0, 10, 200};
    const std::vector<uint32_t> versions{2, 3};
    const size_t num_casts{versions.size() * parent_fees.size()};

    std::vector<COutPoint> coins;
    for (size_t i = 0; i < num_casts + 1; ++i) {
        const CBlock block{g_setup->CreateAndProcessBlock({}, P2WSH_DROP_TRUE)};
        coins.emplace_back(block.vtx.at(0)->GetHash(), 0);
    }
    g_setup->mineBlocks(COINBASE_MATURITY);
    const CAmount coinbase_value{50 * COIN};
    // CONFIRMED_PARENT: one output per cast, confirmed in its own block.
    constexpr CAmount CONFIRMED_OUTPUT{5 * COIN};
    CONFIRMED_PARENT = MakeTx(2, {coins.at(num_casts)}, std::vector<CAmount>(num_casts, CONFIRMED_OUTPUT));
    const CBlock confirmed_block{g_setup->CreateAndProcessBlock({CMutableTransaction{*CONFIRMED_PARENT}}, P2WSH_DROP_TRUE)};
    Assert(confirmed_block.vtx.size() == 2);
    Assert(WITH_LOCK(cs_main, return g_setup->m_node.chainman->ActiveChainstate().CoinsTip().HaveCoin(COutPoint{CONFIRMED_PARENT->GetHash(), 0})));

    size_t coin_index{0};
    for (const uint32_t version : versions) {
        for (const CAmount parent_fee : parent_fees) {
            Cast cast;
            cast.parent_fee = parent_fee;
            cast.parent = MakeTx(version, {coins.at(coin_index)}, {coinbase_value - parent_fee});
            cast.child = MakeTx(version, {COutPoint{cast.parent->GetHash(), 0}, COutPoint{CONFIRMED_PARENT->GetHash(), static_cast<uint32_t>(coin_index)}},
                                {coinbase_value - parent_fee + CONFIRMED_OUTPUT - 100});
            cast.fake_child = MakeTx(version, {COutPoint{cast.parent->GetHash(), 0}, COutPoint{Txid::FromUint256(uint256::ONE), 0}}, {coinbase_value});
            CASTS.push_back(std::move(cast));
            ++coin_index;
        }
    }
}

CNode* MakePeer(NodeId id, ConnectionType conn_type)
{
    in_addr address{};
    address.s_addr = htonl(0x0a000001U + static_cast<uint32_t>(id));
    return new CNode{id,
                     /*sock=*/nullptr,
                     CAddress{CService{address, static_cast<uint16_t>(18444 + id)}, NODE_NETWORK},
                     /*nKeyedNetGroupIn=*/static_cast<uint64_t>(id),
                     /*nLocalHostNonceIn=*/0,
                     CService{},
                     /*addrNameIn=*/"",
                     conn_type,
                     /*inbound_onion=*/false,
                     /*network_key=*/static_cast<uint64_t>(id + 1)};
}

/** Version handshake with wtxid relay negotiated. */
void Handshake(ConnmanTestMsg& connman, PeerManager& peerman, CNode& peer) EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex)
{
    constexpr ServiceFlags services{NODE_NETWORK | NODE_WITNESS};
    connman.Handshake(peer, /*successfully_connected=*/false, services, services, PROTOCOL_VERSION, /*relay_txs=*/true);
    Assert(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::WTXIDRELAY)));
    peer.fPauseSend = false;
    (void)connman.ProcessMessagesOnce(peer);
    Assert(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::VERACK)));
    peer.fPauseSend = false;
    (void)connman.ProcessMessagesOnce(peer);
    Assert(peerman.SendMessages(peer));
    Assert(peer.fSuccessfullyConnected);
    Assert(!peer.fDisconnect);
    connman.FlushSendBuffer(peer);
}

/** Take everything we sent to this peer, as (message type, payload), and clear the send buffers. */
std::vector<std::pair<std::string, std::vector<unsigned char>>> TakeSentMessages(CNode& node)
{
    std::vector<std::pair<std::string, std::vector<unsigned char>>> ret;
    LOCK(node.cs_vSend);
    // Messages already handed to the (v1) transport: re-parse the framing.
    std::vector<unsigned char> bytes;
    while (true) {
        const auto& [to_send, _more, _msg_type] = node.m_transport->GetBytesToSend(/*have_next_message=*/false);
        if (to_send.empty()) break;
        bytes.insert(bytes.end(), to_send.begin(), to_send.end());
        node.m_transport->MarkBytesSent(to_send.size());
    }
    size_t pos{0};
    while (pos + CMessageHeader::HEADER_SIZE <= bytes.size()) {
        std::span<const unsigned char> header{bytes.data() + pos, CMessageHeader::HEADER_SIZE};
        const auto type_begin{header.begin() + std::tuple_size_v<MessageStartChars>};
        std::string type{type_begin, std::find(type_begin, type_begin + CMessageHeader::MESSAGE_TYPE_SIZE, '\0')};
        const uint32_t size{ReadLE32(header.data() + CMessageHeader::MESSAGE_SIZE_OFFSET)};
        pos += CMessageHeader::HEADER_SIZE;
        Assert(pos + size <= bytes.size());
        ret.emplace_back(std::move(type), std::vector<unsigned char>(bytes.begin() + pos, bytes.begin() + pos + size));
        pos += size;
    }
    Assert(pos == bytes.size());
    // Messages still queued.
    for (auto& msg : node.vSendMsg) ret.emplace_back(std::move(msg.m_type), std::move(msg.data));
    node.vSendMsg.clear();
    node.m_send_memusage = 0;
    return ret;
}

/** Mutate the witness of the first input in ways that keep the txid. */
CTransactionRef MutateWitness(FuzzedDataProvider& fuzzed_data_provider, const CTransactionRef& tx)
{
    CMutableTransaction mtx{*tx};
    auto& stack{mtx.vin[0].scriptWitness.stack};
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 4) {
        CallOneOf(
            fuzzed_data_provider,
            [&] { stack.clear(); },
            [&] {
                // Pad with items of a chosen size (standard size or not).
                const auto num{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 150)};
                const auto size{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 100)};
                stack.insert(stack.begin(), num, std::vector<unsigned char>(size, fuzzed_data_provider.ConsumeIntegral<uint8_t>()));
            },
            [&] {
                // Replace the witness script with a script of sigop-heavy or arbitrary bytes.
                std::vector<unsigned char> script;
                if (fuzzed_data_provider.ConsumeBool()) {
                    script.assign(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 1000), OP_CHECKMULTISIG);
                } else {
                    script = ConsumeRandomLengthByteVector(fuzzed_data_provider, 200);
                }
                if (stack.empty()) stack.push_back(script); else stack.back() = script;
            },
            [&] {
                // Replace or add an arbitrary item.
                auto item{ConsumeRandomLengthByteVector(fuzzed_data_provider, 100)};
                if (stack.empty() || fuzzed_data_provider.ConsumeBool()) {
                    stack.insert(stack.begin(), std::move(item));
                } else {
                    stack.at(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, stack.size() - 1)) = std::move(item);
                }
            },
            [&] {
                // Another valid witness: OP_DROP accepts any dummy.
                if (stack.size() == 2) stack.front() = ConsumeRandomLengthByteVector(fuzzed_data_provider, 80);
            });
    }
    return MakeTransactionRef(mtx);
}

} // namespace

FUZZ_TARGET(p2p_1p1c_liveness, .init = ::initialize)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    auto& node_ctx{g_setup->m_node};
    auto& chainman{static_cast<TestChainstateManager&>(*node_ctx.chainman)};
    auto& mempool{*node_ctx.mempool};
    Assert(mempool.size() == 0);
    chainman.ResetIbd();
    chainman.JumpOutOfIbd();
    const CBlockIndex* const active_tip{WITH_LOCK(chainman.GetMutex(), return chainman.ActiveChain().Tip())};
    NodeSeconds now{active_tip->Time() + 1s};
    g_setup->m_clock.set(now);

    const Cast& cast{PickValue(fuzzed_data_provider, CASTS)};
    const std::vector<CTransactionRef> cast_txs{cast.parent, cast.child, cast.fake_child, CONFIRMED_PARENT};

    AddrMan addrman{*node_ctx.netgroupman, /*deterministic=*/true, /*consistency_check_ratio=*/0};
    ConnmanTestMsg connman{0, 0, addrman, *node_ctx.netgroupman, Params()};
    auto peerman{PeerManager::make(connman, addrman, /*banman=*/nullptr, chainman, mempool, *node_ctx.warnings,
                                   PeerManager::Options{.deterministic_rng = true})};
    CConnman::Options connman_options;
    connman_options.m_msgproc = peerman.get();
    connman_options.m_peer_connect_timeout = 99999;
    connman.Init(connman_options);

    LOCK(NetEventsInterface::g_msgproc_mutex);
    NodeId next_id{0};
    const int num_adversaries{fuzzed_data_provider.ConsumeIntegralInRange(1, 3)};
    // peers[0] is the honest peer; the rest are adversaries. Disconnected adversaries are nullptr
    // until they reconnect (as a new node).
    std::vector<CNode*> peers;
    auto connect = [&](ConnectionType conn_type) EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex) {
        CNode* peer{MakePeer(next_id++, conn_type)};
        connman.AddTestNode(*peer);
        Handshake(connman, *peerman, *peer);
        return peer;
    };
    const bool honest_outbound{fuzzed_data_provider.ConsumeBool()};
    peers.push_back(connect(honest_outbound ? ConnectionType::OUTBOUND_FULL_RELAY : ConnectionType::INBOUND));
    // Adversaries may hold outbound slots too. Reconnections are inbound: an outbound slot an adversary
    // gives up is assumed refilled by a non-adversary.
    int adversary_outbounds{0};
    for (int i = 0; i < num_adversaries; ++i) {
        const bool outbound{fuzzed_data_provider.ConsumeBool()};
        adversary_outbounds += outbound;
        peers.push_back(connect(outbound ? ConnectionType::OUTBOUND_FULL_RELAY : ConnectionType::INBOUND));
    }
    CNode* const honest{peers[0]};

    bool honest_announced{false};
    // Known liveness holes. The property is not checked once a move has opened one of them.
    struct {
        //! A witnessless parent (wtxid == txid) not in the mempool and below the minimum relay feerate
        //! at its stripped size is rejected as TX_RECONSIDERABLE, and forgetting its wtxid in
        //! TxRequestTracker cancels every announcer's request for the parent, since orphan resolution
        //! requests the parent by txid.
        bool parent_stripped{false};
        //! While a version of the parent is in the mempool, another version that is witnessless or the
        //! honest one is rejected as TX_CONFLICT, and its wtxid (the parent's txid, or the honest wtxid)
        //! enters the reject filter. Once the parent then leaves the mempool, the child is dropped as an
        //! orphan with a rejected parent, or the honest parent is ignored.
        bool known_parent_replayed{false};
        //! A witnessless copy of CONFIRMED_PARENT is rejected as TX_CONFLICT, and its wtxid, also its
        //! txid, enters the reject filter. Orphan handling checks every input's parent against it,
        //! confirmed or not, so the child is dropped as having a rejected parent.
        bool confirmed_parent_stripped{false};
        //! A peer announced or delivered the child, and delivered another version of the parent that
        //! was not accepted. Their 1p1c package (the child is found in the orphanage as announced by
        //! that peer) fails the package feerate, and ProcessInvalidTx erases the child from the
        //! orphanage for all announcers and caches its wtxid as rejected. Only set once the child was
        //! dropped this way: it was in the orphanage (or delivered in that step) and afterwards is
        //! neither there nor accepted.
        bool package_with_other_parent{false};
        //! The parent left the mempool while the child was in the orphanage. Accepting the parent
        //! completed every request for it, and retrying the orphan with the input missing again
        //! requests nothing, so the parent is not fetched again.
        bool parent_left_with_orphan{false};
        bool Any() const
        {
            return parent_stripped || known_parent_replayed || confirmed_parent_stripped || package_with_other_parent || parent_left_with_orphan;
        }
    } holes;
    // Adversaries that announced or delivered the child, and that delivered another version of the
    // parent that was not accepted.
    std::set<NodeId> child_announcers, other_parent_senders;
    // Whether a witnessless or the honest version of the parent was rejected as TX_CONFLICT.
    bool parent_replay_rejected{false};
    int reconnects{0};

    auto receive = [&](CNode& peer, CSerializedNetMsg&& msg) { Assert(connman.ReceiveMsgFrom(peer, std::move(msg))); };
    // The honest peer is in sync with us (it announces our tip, and answers pings below), so that it
    // is not disconnected as an outbound peer with an old chain or for a ping timeout.
    receive(*honest, NetMsg::Make(NetMsgType::HEADERS, TX_WITH_WITNESS(std::vector<CBlock>{CBlock{active_tip->GetBlockHeader()}})));
    // Let the node process everything pending and send its messages. The honest peer answers our
    // requests immediately with the honest transactions; adversaries never answer here. Peers are
    // serviced round-robin from a fuzzer-chosen peer, as the message handler (which shuffles them) may
    // be anywhere in its loop when a message arrives: work created for a peer whose turn already
    // passed waits for the next round, or for the next call if no peer has more work. As there,
    // peers the node is disconnecting (e.g. for misbehaviour) are skipped, and then they are gone.
    auto process = [&]() EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex) {
        const size_t start{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, peers.size() - 1)};
        for (int rounds = 0; rounds < 32; ++rounds) {
            bool more{false};
            for (size_t i = 0; i < peers.size(); ++i) {
                CNode* peer{peers[(start + i) % peers.size()]};
                if (!peer || peer->fDisconnect) continue;
                peer->fPauseSend = false;
                more |= connman.ProcessMessagesOnce(*peer);
                peerman->SendMessages(*peer);
                for (auto& [type, payload] : TakeSentMessages(*peer)) {
                    if (peer != honest) continue;
                    if (type == NetMsgType::PING) {
                        DataStream stream{payload};
                        uint64_t nonce;
                        stream >> nonce;
                        receive(*honest, NetMsg::Make(NetMsgType::PONG, nonce));
                        more = true;
                        continue;
                    }
                    if (type != NetMsgType::GETDATA) continue;
                    DataStream stream{payload};
                    std::vector<CInv> invs;
                    stream >> invs;
                    std::vector<CInv> not_found;
                    for (const auto& inv : invs) {
                        if (!inv.IsGenTxMsg()) continue;
                        if (inv.hash == cast.parent->GetHash().ToUint256() || inv.hash == cast.parent->GetWitnessHash().ToUint256()) {
                            receive(*honest, NetMsg::Make(NetMsgType::TX, TX_WITH_WITNESS(*cast.parent)));
                        } else if (inv.hash == cast.child->GetHash().ToUint256() || inv.hash == cast.child->GetWitnessHash().ToUint256()) {
                            receive(*honest, NetMsg::Make(NetMsgType::TX, TX_WITH_WITNESS(*cast.child)));
                        } else {
                            not_found.push_back(inv);
                        }
                        more = true;
                    }
                    if (!not_found.empty()) receive(*honest, NetMsg::Make(NetMsgType::NOTFOUND, not_found));
                }
            }
            if (!more) break;
        }
        for (auto& peer : peers) {
            if (peer && peer != honest && peer->fDisconnect) {
                peerman->FinalizeNode(*peer);
                peer = nullptr;
            }
        }
        Assert(!honest->fDisconnect);
    };
    auto announce_honest = [&]() {
        receive(*honest, NetMsg::Make(NetMsgType::INV, std::vector<CInv>{CInv{MSG_WTX, cast.child->GetWitnessHash().ToUint256()}}));
        honest_announced = true;
    };
    auto child_accepted = [&]() { return mempool.exists(cast.child->GetHash()); };

    // One adversary step: an adversary acts (or the parent leaves the mempool, or time passes), then
    // the node processes it.
    auto child_orphaned = [&]() {
        return std::ranges::any_of(peerman->GetOrphanTransactions(), [&](const auto& o) { return o.tx->GetWitnessHash() == cast.child->GetWitnessHash(); });
    };
    auto adversary_step = [&]() EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex) {
        const bool child_was_orphaned{child_orphaned()};
        // Whether this step may have run the 1p1c package of another parent version and the child.
        bool other_parent_package{false};
        bool child_delivered{false};
        const size_t idx{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, peers.size() - 1)};
        CNode* adversary{peers[idx]};
        const auto& tx{PickValue(fuzzed_data_provider, cast_txs)};
        std::optional<size_t> reconnect_idx;
        std::optional<std::pair<NodeId, Wtxid>> other_parent;
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                if (!honest_announced) announce_honest();
            },
            [&] {
                // Announce a cast transaction, a mutated version of it, or an unrelated hash.
                if (!adversary) return;
                uint256 hash{fuzzed_data_provider.ConsumeBool() ? tx->GetWitnessHash().ToUint256() : MutateWitness(fuzzed_data_provider, tx)->GetWitnessHash().ToUint256()};
                if (fuzzed_data_provider.ConsumeBool()) hash = ConsumeUInt256(fuzzed_data_provider);
                const CInv inv{fuzzed_data_provider.ConsumeBool() ? MSG_WTX : MSG_TX, hash};
                if (inv.IsMsgWtx() && inv.hash == cast.child->GetWitnessHash().ToUint256()) child_announcers.insert(adversary->GetId());
                receive(*adversary, NetMsg::Make(NetMsgType::INV, std::vector<CInv>{inv}));
            },
            [&] {
                // Deliver a cast transaction as is, stripped of its witness, inflated, or otherwise
                // witness-mutated.
                if (!adversary) return;
                CTransactionRef delivered{tx};
                switch (fuzzed_data_provider.ConsumeIntegralInRange(0, 3)) {
                case 0: break;
                case 1: {
                    CMutableTransaction stripped{*tx};
                    for (auto& txin : stripped.vin) txin.scriptWitness.SetNull();
                    delivered = MakeTransactionRef(std::move(stripped));
                    break;
                }
                case 2: {
                    // A witness script of standard size and sigop count but sigop-heavy: invalid, but
                    // its sigop-adjusted size counts against the feerate, which is checked first.
                    CMutableTransaction inflated{*tx};
                    inflated.vin[0].scriptWitness.stack.back().assign(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, MAX_STANDARD_TX_SIGOPS_COST / MAX_PUBKEYS_PER_MULTISIG), OP_CHECKMULTISIG);
                    delivered = MakeTransactionRef(std::move(inflated));
                    break;
                }
                case 3: delivered = MutateWitness(fuzzed_data_provider, tx); break;
                }
                if (tx == cast.parent && (!delivered->HasWitness() || delivered->GetWitnessHash() == cast.parent->GetWitnessHash())) {
                    const auto in_mempool{mempool.get(cast.parent->GetHash())};
                    if (in_mempool && in_mempool->GetWitnessHash() != delivered->GetWitnessHash()) parent_replay_rejected = true;
                }
                if (!delivered->HasWitness()) {
                    if (tx == cast.parent && !mempool.exists(cast.parent->GetHash()) &&
                        mempool.m_opts.min_relay_feerate.GetFee(GetVirtualTransactionSize(*delivered)) > cast.parent_fee) {
                        holes.parent_stripped = true;
                    }
                    if (tx == CONFIRMED_PARENT) holes.confirmed_parent_stripped = true;
                } else if (tx == cast.parent && delivered->GetWitnessHash() != cast.parent->GetWitnessHash()) {
                    other_parent.emplace(adversary->GetId(), delivered->GetWitnessHash());
                } else if (delivered->GetWitnessHash() == cast.child->GetWitnessHash()) {
                    child_announcers.insert(adversary->GetId());
                    child_delivered = true;
                    if (other_parent_senders.contains(adversary->GetId())) other_parent_package = true;
                }
                receive(*adversary, NetMsg::Make(NetMsgType::TX, TX_WITH_WITNESS(*delivered)));
            },
            [&] {
                if (!adversary) return;
                receive(*adversary, NetMsg::Make(NetMsgType::NOTFOUND, std::vector<CInv>{CInv{fuzzed_data_provider.ConsumeBool() ? MSG_WTX : MSG_TX,
                                                                                            fuzzed_data_provider.ConsumeBool() ? tx->GetHash().ToUint256() : tx->GetWitnessHash().ToUint256()}}));
            },
            [&] {
                if (adversary) {
                    peerman->FinalizeNode(*adversary);
                    adversary->fDisconnect = true;
                    peers[idx] = nullptr;
                } else if (reconnects < 6) {
                    ++reconnects;
                    reconnect_idx = idx;
                }
            },
            [&] {
                now += std::chrono::seconds{fuzzed_data_provider.ConsumeIntegralInRange(0, 5)};
                g_setup->m_clock.set(now);
            },
            [&] {
                // The parent leaves the mempool, as through eviction, replacement or a reorg. The
                // child is not in it (or this loop would have ended).
                const bool orphaned{child_orphaned()};
                LOCK(mempool.cs);
                if (const auto parent{mempool.get(cast.parent->GetHash())}) {
                    if (orphaned) holes.parent_left_with_orphan = true;
                    if (parent_replay_rejected) holes.known_parent_replayed = true;
                    mempool.removeRecursive(*parent, MemPoolRemovalReason::SIZELIMIT);
                }
            });
        if (reconnect_idx) peers[*reconnect_idx] = connect(ConnectionType::INBOUND);
        process();
        if (other_parent && !mempool.exists(other_parent->second)) {
            other_parent_senders.insert(other_parent->first);
            if (child_announcers.contains(other_parent->first)) other_parent_package = true;
        }
        if (other_parent_package && (child_was_orphaned || child_delivered) && !child_orphaned() && !child_accepted()) {
            holes.package_with_other_parent = true;
        }
    };

    // Adversarial prefix.
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool() && !child_accepted() && !holes.Any(), 200) {
        adversary_step();
    }
    if (!honest_announced && !holes.Any()) {
        announce_honest();
        process();
    }

    // Drain. Only one request per txhash is in flight at a time, and a preferred (outbound) candidate
    // is chosen over any non-preferred one as soon as the in-flight request completes or times out.
    // So for the child and then for the parent, an outbound honest peer waits for at most one stalled
    // request per adversarial outbound (and the one already in flight), whatever the adversaries do:
    // they keep acting during the drain. An inbound honest peer may have to wait for a request to
    // every adversary connection in turn, which adversaries reconnecting forever could extend without
    // bound; for it the adversaries go quiet (stalling any request made to them) and every connection
    // made so far counts.
    const int max_stalls{honest_outbound ? 2 * (1 + adversary_outbounds) : 2 * (num_adversaries + reconnects)};
    const auto deadline{now + 10s + (max_stalls + 1) * (node::GETDATA_TX_INTERVAL + 1s)};
    while (!child_accepted() && !holes.Any() && now < deadline) {
        if (honest_outbound && fuzzed_data_provider.ConsumeBool()) {
            adversary_step();
            if (child_accepted()) break;
        }
        now += 1s;
        g_setup->m_clock.set(now);
        process();
    }
    Assert(holes.Any() || child_accepted());

    // Tear down. Descendants (any accepted child version) go with the parent.
    for (CNode* peer : peers) {
        if (peer) peerman->FinalizeNode(*peer);
    }
    connman.ClearTestNodes();
    connman.SetMsgProc(nullptr);
    peerman.reset();
    WITH_LOCK(mempool.cs, mempool.removeRecursive(*cast.parent, MemPoolRemovalReason::REPLACED));
    Assert(mempool.size() == 0);
    node_ctx.validation_signals->SyncWithValidationInterfaceQueue();
}
