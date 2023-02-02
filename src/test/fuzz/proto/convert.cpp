#include <test/fuzz/proto/convert.h>

#include <net.h>
#include <node/connection_types.h>
#include <primitives/transaction.h>
#include <txmempool.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/proto/mempool_options.pb.h>
#include <test/fuzz/proto/p2p.pb.h>
#include <test/fuzz/proto/transaction.pb.h>
#include <test/fuzz/util/net.h>

CTransactionRef ConvertTransaction(const common_fuzz::Transaction& proto_tx)
{
    CMutableTransaction mtx;
    mtx.nLockTime = proto_tx.lock_time();
    mtx.nVersion = proto_tx.version();

    for (const auto& proto_input : proto_tx.inputs()) {
        CTxIn input;
        input.nSequence = proto_input.sequence();
        // scriptWitness
        for (const auto& witness : proto_input.witness_stack()) {
            input.scriptWitness.stack.push_back({witness.begin(), witness.end()});
        }
        // scriptSig
        std::vector<uint8_t> script{proto_input.script_sig().raw().begin(),
                                    proto_input.script_sig().raw().end()};
        input.scriptSig = {script.begin(), script.end()};
        // prevout
        auto hex = proto_input.prev_out().txid();
        hex.resize(64);
        input.prevout.hash = uint256S(hex);
        input.prevout.n = proto_input.prev_out().n();

        mtx.vin.emplace_back(std::move(input));
    }

    for (const auto& proto_output : proto_tx.outputs()) {
        CTxOut output;
        output.nValue = proto_output.value();
        std::vector<uint8_t> script{proto_output.script_pub_key().raw().begin(),
                                    proto_output.script_pub_key().raw().end()};
        output.scriptPubKey = {script.begin(), script.end()};
        mtx.vout.emplace_back(std::move(output));
    }

    return MakeTransactionRef(mtx);
}

CTxMemPool::Options ConvertMempoolOptions(const common_fuzz::MempoolOptions& mempool_options)
{
    // Take the default options for tests...
    CTxMemPool::Options mempool_opts;

    // ...override specific options for this specific fuzz suite
    mempool_opts.estimator = nullptr;
    mempool_opts.check_ratio = 1;

    // TODO clean up the naming
    auto& options = mempool_options;
    if (options.has_max_size_bytes())
        mempool_opts.require_standard = options.require_standard();
    if (options.has_expiry_hours())
        mempool_opts.expiry = std::chrono::hours{options.expiry_hours()};
    if (options.has_incremental_relay_feerate())
        mempool_opts.incremental_relay_feerate = CFeeRate{options.incremental_relay_feerate()};
    if (options.has_min_relay_feerate())
        mempool_opts.min_relay_feerate = CFeeRate{options.min_relay_feerate()};
    if (options.has_dust_relay_feerate())
        mempool_opts.dust_relay_feerate = CFeeRate{options.dust_relay_feerate()};
    if (options.has_max_datacarrier_bytes())
        mempool_opts.max_datacarrier_bytes = options.max_datacarrier_bytes();
    if (options.has_permit_bare_multisig())
        mempool_opts.permit_bare_multisig = options.permit_bare_multisig();
    if (options.has_require_standard())
        mempool_opts.require_standard = options.require_standard();
    if (options.has_full_rbf())
        mempool_opts.full_rbf = options.full_rbf();

    auto& limits = options.limits();
    auto& mempool_limits = mempool_opts.limits;
    if (limits.has_ancestor_count_limit())
        mempool_limits.ancestor_count = limits.ancestor_count_limit();
    if (limits.has_ancestor_size_limit())
        mempool_limits.ancestor_size_vbytes = limits.ancestor_size_limit();
    if (limits.has_descendant_count_limit())
        mempool_limits.descendant_count = limits.descendant_count_limit();
    if (limits.has_ancestor_size_limit())
        mempool_limits.descendant_size_vbytes = limits.descendant_size_limit();

    return mempool_opts;
}

ConnectionType ConvertConnType(const common_fuzz::ConnectionType& type)
{
    switch (type) {
    case common_fuzz::MANUAL:
        return ConnectionType::MANUAL;
    case common_fuzz::BLOCK_RELAY:
        return ConnectionType::BLOCK_RELAY;
    case common_fuzz::OUTBOUND_FULL_RELAY:
        return ConnectionType::OUTBOUND_FULL_RELAY;
    case common_fuzz::INBOUND:
        return ConnectionType::OUTBOUND_FULL_RELAY;
    case common_fuzz::INBOUND_ONION:
        return ConnectionType::INBOUND;
    case common_fuzz::ADDR_FETCH:
        return ConnectionType::ADDR_FETCH;
    case common_fuzz::FEELER:
        return ConnectionType::FEELER;
    }
}

NetPermissionFlags ConvertPermFlags(const common_fuzz::NetPermissionFlags& flags)
{
    NetPermissionFlags perms{0};
    if (flags.addr()) perms = perms | NetPermissionFlags::Addr;
    if (flags.noban()) perms = perms | NetPermissionFlags::NoBan;
    if (flags.download()) perms = perms | NetPermissionFlags::Download;
    if (flags.relay()) perms = perms | NetPermissionFlags::Relay;
    if (flags.force_relay()) perms = perms | NetPermissionFlags::ForceRelay;
    if (flags.mempool()) perms = perms | NetPermissionFlags::Mempool;
    if (flags.implicit()) perms = perms | NetPermissionFlags::Implicit;
    if (flags.bloom_filter()) perms = perms | NetPermissionFlags::BloomFilter;
    return perms;
}

CNode& ConvertConnection(const common_fuzz::Connection& connection, FuzzedDataProvider& sock_data_provider)
{
    CNodeOptions options;
    if (connection.has_perm_flags()) {
        options.permission_flags = ConvertPermFlags(connection.perm_flags());
    }
    if (connection.has_prefer_evict()) {
        options.prefer_evict = connection.prefer_evict();
    }

    CNode* node = new CNode{
        /*id=*/0,
        /*sock=*/std::make_shared<FuzzedSock>(sock_data_provider),
        /*addrIn=*/CAddress(),
        /*nKeyedNetGroupIn=*/connection.keyed_net_group(),
        /*nLocalHostNonceIn=*/connection.local_host_nonce(),
        /*addrBindIn=*/CAddress(),
        /*addrNameIn=*/"",
        /*conn_type_in=*/ConvertConnType(connection.conn_type()),
        /*inbound_onion=*/connection.conn_type() == common_fuzz::INBOUND_ONION,
        std::move(options),
    };

    return *node;
}

ServiceFlags ConvertServiceFlags(const common_fuzz::ServiceFlags& flags)
{
    uint64_t service_flags{0};
    if (flags.bloom()) service_flags |= NODE_BLOOM;
    if (flags.network()) service_flags |= NODE_NETWORK;
    if (flags.network_limited()) service_flags |= NODE_NETWORK_LIMITED;
    if (flags.compact_filters()) service_flags |= NODE_COMPACT_FILTERS;
    if (flags.witness()) service_flags |= NODE_WITNESS;

    return ServiceFlags{service_flags};
}

std::tuple<std::string, std::vector<uint8_t>> ConvertHandshakeMsg(const common_fuzz::HandshakeMsg& msg)
{
    std::string type;
    CDataStream stream{SER_NETWORK, PROTOCOL_VERSION};
    if (msg.has_version()) {
        type = NetMsgType::VERSION;
        stream << msg.version().version()
               << (uint64_t)ConvertServiceFlags(msg.version().services())
               << msg.version().time()
               << uint64_t{0}
               << CService()
               << uint64_t{0} << uint64_t{0} << uint64_t{0} << uint16_t{0}
               << msg.version().nonce()
               << msg.version().sub_version()
               << msg.version().starting_height()
               << msg.version().tx_relay();
    } else if (msg.has_send_cmpct()) {
        type = NetMsgType::SENDCMPCT;
        stream << msg.send_cmpct().high_bw()
               << msg.send_cmpct().version();
    } else if (msg.has_verack()) {
        type = NetMsgType::VERACK;
    } else if (msg.has_sendaddrv2()) {
        type = NetMsgType::SENDADDRV2;
    } else if (msg.has_sendtxrcncl()) {
        type = NetMsgType::SENDTXRCNCL;
        stream << msg.sendtxrcncl().version()
               << msg.sendtxrcncl().salt();
    } else if (msg.has_wtxidrelay()) {
        type = NetMsgType::WTXIDRELAY;
    }

    std::vector<uint8_t> data(stream.size());
    if (stream.size() > 0)
        std::memcpy(data.data(), (uint8_t*)stream.data(), stream.size());
    return {type, std::move(data)};
}
