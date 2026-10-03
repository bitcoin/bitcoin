// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// End-to-end liveness of opportunistic 1p1c package relay against adversarial peers, driving a real
// PeerManager with real P2P messages and real validation.
//
// Property: if an honest peer announces the child of an acceptable {low-fee parent, child} package,
// the child ends up in our mempool, whatever other peers do. Adversaries hold no keys: they may
// announce, deliver the parent, the child or the confirmed transaction funding the child with any
// witness (the fuzzer mutates witness stack items freely), deliver a fake child of the parent, send
// notfound, stall, disconnect and reconnect.
//
// Nothing below PeerManager is modelled: message handling, transaction download, validation ordering
// and result attribution are the production code paths, and the adversary's malleations of the cast
// transactions are not limited to a fixed set. The cast itself is fixed: P2WSH inputs, children with
// one confirmed fee input. The oracle is coarse (mempool membership) and each execution pays for real
// validation.

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
#include <primitives/transaction.h>
#include <protocol.h>
#include <script/script.h>
#include <script/solver.h>
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
#include <validation.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
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
    CTransactionRef parent;     //!< too low fee to be accepted alone
    CTransactionRef child;      //!< pays for the pair, from the parent's output and a confirmed output of FUND
    CTransactionRef fake_child; //!< spends the parent and an unknown outpoint
};
std::vector<Cast> CASTS;
/** Confirmed transaction whose outputs fund the children. Confirmed longer ago than the recently-confirmed
 * filter remembers (a fresh PeerManager per input has an empty one), with its outputs in the coins cache. */
CTransactionRef FUND;

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

    std::vector<COutPoint> coins;
    for (int i = 0; i < 5; ++i) {
        const CBlock block{g_setup->CreateAndProcessBlock({}, P2WSH_DROP_TRUE)};
        coins.emplace_back(block.vtx.at(0)->GetHash(), 0);
    }
    g_setup->mineBlocks(COINBASE_MATURITY);
    const CAmount coinbase_value{50 * COIN};
    // FUND: one output per cast, confirmed in its own block.
    constexpr CAmount FUND_OUTPUT{10 * COIN};
    FUND = MakeTx(2, {coins.at(4)}, std::vector<CAmount>(4, FUND_OUTPUT));
    const CBlock fund_block{g_setup->CreateAndProcessBlock({CMutableTransaction{*FUND}}, P2WSH_DROP_TRUE)};
    Assert(fund_block.vtx.size() == 2);

    size_t coin_index{0};
    for (const uint32_t version : {uint32_t{2}, uint32_t{3}}) {
        // A parent of ~112 vbytes with its witness and ~94 without: 10 sat is below the minimum relay
        // feerate with the witness and above it without.
        for (const CAmount parent_fee : {CAmount{0}, CAmount{10}}) {
            Cast cast;
            cast.parent = MakeTx(version, {coins.at(coin_index)}, {coinbase_value - parent_fee});
            cast.child = MakeTx(version, {COutPoint{cast.parent->GetHash(), 0}, COutPoint{FUND->GetHash(), static_cast<uint32_t>(coin_index)}},
                                {coinbase_value - parent_fee + FUND_OUTPUT - 100});
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
    const std::vector<CTransactionRef> cast_txs{cast.parent, cast.child, cast.fake_child, FUND};

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
    for (int i = 0; i < num_adversaries; ++i) peers.push_back(connect(ConnectionType::INBOUND));
    CNode* const honest{peers[0]};

    bool honest_announced{false};
    int reconnects{0};

    auto receive = [&](CNode& peer, CSerializedNetMsg&& msg) { Assert(connman.ReceiveMsgFrom(peer, std::move(msg))); };
    // Let the node process everything pending and send its messages. The honest peer answers our
    // requests immediately with the honest transactions; adversaries never answer here.
    auto process = [&]() EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex) {
        for (int rounds = 0; rounds < 32; ++rounds) {
            bool more{false};
            for (CNode* peer : peers) {
                if (!peer) continue;
                peer->fPauseSend = false;
                more |= connman.ProcessMessagesOnce(*peer);
                peerman->SendMessages(*peer);
                for (auto& [type, payload] : TakeSentMessages(*peer)) {
                    if (peer != honest || type != NetMsgType::GETDATA) continue;
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
    };
    auto announce_honest = [&]() {
        receive(*honest, NetMsg::Make(NetMsgType::INV, std::vector<CInv>{CInv{MSG_WTX, cast.child->GetWitnessHash().ToUint256()}}));
        honest_announced = true;
    };
    auto child_accepted = [&]() { return mempool.exists(cast.child->GetHash()); };

    // Adversarial prefix.
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool() && !child_accepted(), 200) {
        const size_t idx{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, peers.size() - 1)};
        CNode* adversary{peers[idx]};
        const auto& tx{PickValue(fuzzed_data_provider, cast_txs)};
        std::optional<size_t> reconnect_idx;
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
                receive(*adversary, NetMsg::Make(NetMsgType::INV, std::vector<CInv>{CInv{fuzzed_data_provider.ConsumeBool() ? MSG_WTX : MSG_TX, hash}}));
            },
            [&] {
                // Deliver a cast transaction or a witness-mutated version of it.
                if (!adversary) return;
                const auto delivered{fuzzed_data_provider.ConsumeBool() ? tx : MutateWitness(fuzzed_data_provider, tx)};
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
            });
        if (reconnect_idx) peers[*reconnect_idx] = connect(ConnectionType::INBOUND);
        process();
        // A peer the node decided to disconnect (e.g. for misbehaviour) is gone.
        for (auto& peer : peers) {
            if (peer && peer != honest && peer->fDisconnect) {
                peerman->FinalizeNode(*peer);
                peer = nullptr;
            }
        }
        Assert(!honest->fDisconnect);
    }
    if (!honest_announced) {
        announce_honest();
        process();
    }

    // Fair drain: adversaries go quiet (stalling any request made to them). Only one request per
    // txhash is in flight at a time, and a preferred (outbound) candidate is chosen over any
    // non-preferred one as soon as the in-flight request completes or times out. So for the child
    // and then for the parent, an outbound honest peer waits for at most the one request already in
    // flight, while an inbound one may have to wait for every (inbound) adversary candidate in turn.
    const int max_stalls{honest_outbound ? 2 : 2 * num_adversaries};
    const auto deadline{now + 10s + (max_stalls + 1) * (node::GETDATA_TX_INTERVAL + 1s)};
    while (!child_accepted() && now < deadline) {
        now += 1s;
        g_setup->m_clock.set(now);
        process();
    }
    Assert(child_accepted());

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
