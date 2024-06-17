// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>

#include <addrman.h>
#include <addrman_impl.h>
#include <banman.h>
#include <chainparams.h>
#include <clientversion.h>
#include <core_io.h>
#include <net_permissions.h>
#include <net_processing.h>
#include <net_types.h> // For banmap_t
#include <netbase.h>
#include <node/context.h>
#include <node/protocol_version.h>
#include <node/warnings.h>
#include <policy/settings.h>
#include <protocol.h>
#include <rpc/blockchain.h>
#include <rpc/protocol.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <sync.h>
#include <util/chaintype.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>

#include <optional>

#include <univalue.h>

using node::NodeContext;
using util::Join;
using util::TrimString;

const std::vector<std::string> CONNECTION_TYPE_DOC{
        "outbound-full-relay (default automatic connections)",
        "block-relay-only (does not relay transactions or addresses)",
        "inbound (initiated by the peer)",
        "manual (added via addnode RPC or -addnode/-connect configuration options)",
        "addr-fetch (short-lived automatic connection for soliciting addresses)",
        "feeler (short-lived automatic connection for testing addresses)"
};

const std::vector<std::string> TRANSPORT_TYPE_DOC{
    "detecting (peer could be v1 or v2)",
    "v1 (plaintext transport protocol)",
    "v2 (BIP324 encrypted transport protocol)"
};

static RPCHelpMan getconnectioncount()
{
    return RPCHelpMan{"getconnectioncount",
                "\nReturns the number of connections to other nodes.\n",
                {},
                RPCResult{
                    RPCResult::Type::NUM, "", "The connection count"
                },
                RPCExamples{
                    HelpExampleCli("getconnectioncount", "")
            + HelpExampleRpc("getconnectioncount", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    const CConnman& connman = EnsureConnman(node);

    return connman.GetNodeCount(ConnectionDirection::Both);
},
    };
}

static RPCHelpMan ping()
{
    return RPCHelpMan{"ping",
                "\nRequests that a ping be sent to all other nodes, to measure ping time.\n"
                "Results provided in getpeerinfo, pingtime and pingwait fields are decimal seconds.\n"
                "Ping command is handled in queue with all other commands, so it measures processing backlog, not just network ping.\n",
                {},
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
                    HelpExampleCli("ping", "")
            + HelpExampleRpc("ping", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    PeerManager& peerman = EnsurePeerman(node);

    // Request that each node send a ping during next message processing pass
    peerman.SendPings();
    return UniValue::VNULL;
},
    };
}

/** Returns, given services flags, a list of humanly readable (known) network services */
static UniValue GetServicesNames(ServiceFlags services)
{
    UniValue servicesNames(UniValue::VARR);

    for (const auto& flag : serviceFlagsToStr(services)) {
        servicesNames.push_back(flag);
    }

    return servicesNames;
}

static RPCHelpMan getpeerinfo()
{
    return RPCHelpMan{
        "getpeerinfo",
        "Returns data about each connected network peer as a json array of objects.",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {
                    {RPCResult::Type::NUM, "id", "Peer index"},
                    {RPCResult::Type::STR, "addr", "(host:port) The IP address and port of the peer"},
                    {RPCResult::Type::STR, "addrbind", /*optional=*/true, "(ip:port) Bind address of the connection to the peer"},
                    {RPCResult::Type::STR, "addrlocal", /*optional=*/true, "(ip:port) Local address as reported by the peer"},
                    {RPCResult::Type::STR, "network", "Network (" + Join(GetNetworkNames(/*append_unroutable=*/true), ", ") + ")"},
                    {RPCResult::Type::NUM, "mapped_as", /*optional=*/true, "The AS in the BGP route to the peer used for diversifying\n"
                                                        "peer selection (only available if the asmap config flag is set)"},
                    {RPCResult::Type::STR_HEX, "services", "The services offered"},
                    {RPCResult::Type::ARR, "servicesnames", "the services offered, in human-readable form",
                    {
                        {RPCResult::Type::STR, "SERVICE_NAME", "the service name if it is recognised"}
                    }},
                    {RPCResult::Type::BOOL, "relaytxes", "Whether we relay transactions to this peer"},
                    {RPCResult::Type::NUM_TIME, "lastsend", "The " + UNIX_EPOCH_TIME + " of the last send"},
                    {RPCResult::Type::NUM_TIME, "lastrecv", "The " + UNIX_EPOCH_TIME + " of the last receive"},
                    {RPCResult::Type::NUM_TIME, "last_transaction", "The " + UNIX_EPOCH_TIME + " of the last valid transaction received from this peer"},
                    {RPCResult::Type::NUM_TIME, "last_block", "The " + UNIX_EPOCH_TIME + " of the last block received from this peer"},
                    {RPCResult::Type::NUM, "bytessent", "The total bytes sent"},
                    {RPCResult::Type::NUM, "bytesrecv", "The total bytes received"},
                    {RPCResult::Type::NUM_TIME, "conntime", "The " + UNIX_EPOCH_TIME + " of the connection"},
                    {RPCResult::Type::NUM, "timeoffset", "The time offset in seconds"},
                    {RPCResult::Type::NUM, "pingtime", /*optional=*/true, "The last ping time in milliseconds (ms), if any"},
                    {RPCResult::Type::NUM, "minping", /*optional=*/true, "The minimum observed ping time in milliseconds (ms), if any"},
                    {RPCResult::Type::NUM, "pingwait", /*optional=*/true, "The duration in milliseconds (ms) of an outstanding ping (if non-zero)"},
                    {RPCResult::Type::NUM, "version", "The peer version, such as 70001"},
                    {RPCResult::Type::STR, "subver", "The string version"},
                    {RPCResult::Type::BOOL, "inbound", "Inbound (true) or Outbound (false)"},
                    {RPCResult::Type::BOOL, "bip152_hb_to", "Whether we selected peer as (compact blocks) high-bandwidth peer"},
                    {RPCResult::Type::BOOL, "bip152_hb_from", "Whether peer selected us as (compact blocks) high-bandwidth peer"},
                    {RPCResult::Type::NUM, "startingheight", "The starting height (block) of the peer"},
                    {RPCResult::Type::NUM, "presynced_headers", "The current height of header pre-synchronization with this peer, or -1 if no low-work sync is in progress"},
                    {RPCResult::Type::NUM, "synced_headers", "The last header we have in common with this peer"},
                    {RPCResult::Type::NUM, "synced_blocks", "The last block we have in common with this peer"},
                    {RPCResult::Type::ARR, "inflight", "",
                    {
                        {RPCResult::Type::NUM, "n", "The heights of blocks we're currently asking from this peer"},
                    }},
                    {RPCResult::Type::BOOL, "addr_relay_enabled", "Whether we participate in address relay with this peer"},
                    {RPCResult::Type::NUM, "addr_processed", "The total number of addresses processed, excluding those dropped due to rate limiting"},
                    {RPCResult::Type::NUM, "addr_rate_limited", "The total number of addresses dropped due to rate limiting"},
                    {RPCResult::Type::ARR, "permissions", "Any special permissions that have been granted to this peer",
                    {
                        {RPCResult::Type::STR, "permission_type", Join(NET_PERMISSIONS_DOC, ",\n") + ".\n"},
                    }},
                    {RPCResult::Type::NUM, "minfeefilter", "The minimum fee rate for transactions this peer accepts"},
                    {RPCResult::Type::OBJ_DYN, "bytessent_per_msg", "",
                    {
                        {RPCResult::Type::NUM, "msg", "The total bytes sent aggregated by message type\n"
                                                      "When a message type is not listed in this json object, the bytes sent are 0.\n"
                                                      "Only known message types can appear as keys in the object."}
                    }},
                    {RPCResult::Type::OBJ_DYN, "bytesrecv_per_msg", "",
                    {
                        {RPCResult::Type::NUM, "msg", "The total bytes received aggregated by message type\n"
                                                      "When a message type is not listed in this json object, the bytes received are 0.\n"
                                                      "Only known message types can appear as keys in the object and all bytes received\n"
                                                      "of unknown message types are listed under '"+NET_MESSAGE_TYPE_OTHER+"'."}
                    }},
                    {RPCResult::Type::STR, "connection_type", "Type of connection: \n" + Join(CONNECTION_TYPE_DOC, ",\n") + ".\n"
                                                              "Please note this output is unlikely to be stable in upcoming releases as we iterate to\n"
                                                              "best capture connection behaviors."},
                    {RPCResult::Type::STR, "transport_protocol_type", "Type of transport protocol: \n" + Join(TRANSPORT_TYPE_DOC, ",\n") + ".\n"},
                    {RPCResult::Type::STR, "session_id", "The session ID for this connection, or \"\" if there is none (\"v2\" transport protocol only).\n"},
                }},
            }},
        },
        RPCExamples{
            HelpExampleCli("getpeerinfo", "")
            + HelpExampleRpc("getpeerinfo", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    const CConnman& connman = EnsureConnman(node);
    const PeerManager& peerman = EnsurePeerman(node);

    std::vector<CNodeStats> vstats;
    connman.GetNodeStats(vstats);

    UniValue ret(UniValue::VARR);

    for (const CNodeStats& stats : vstats) {
        UniValue obj(UniValue::VOBJ);
        CNodeStateStats statestats;
        bool fStateStats = peerman.GetNodeStateStats(stats.nodeid, statestats);
        // GetNodeStateStats() requires the existence of a CNodeState and a Peer object
        // to succeed for this peer. These are created at connection initialisation and
        // exist for the duration of the connection - except if there is a race where the
        // peer got disconnected in between the GetNodeStats() and the GetNodeStateStats()
        // calls. In this case, the peer doesn't need to be reported here.
        if (!fStateStats) {
            continue;
        }
        obj.pushKV("id", stats.nodeid);
        obj.pushKV("addr", stats.m_addr_name);
        if (stats.addrBind.IsValid()) {
            obj.pushKV("addrbind", stats.addrBind.ToStringAddrPort());
        }
        if (!(stats.addrLocal.empty())) {
            obj.pushKV("addrlocal", stats.addrLocal);
        }
        obj.pushKV("network", GetNetworkName(stats.m_network));
        if (stats.m_mapped_as != 0) {
            obj.pushKV("mapped_as", uint64_t(stats.m_mapped_as));
        }
        ServiceFlags services{statestats.their_services};
        obj.pushKV("services", strprintf("%016x", services));
        obj.pushKV("servicesnames", GetServicesNames(services));
        obj.pushKV("relaytxes", statestats.m_relay_txs);
        obj.pushKV("lastsend", count_seconds(stats.m_last_send));
        obj.pushKV("lastrecv", count_seconds(stats.m_last_recv));
        obj.pushKV("last_transaction", count_seconds(stats.m_last_tx_time));
        obj.pushKV("last_block", count_seconds(stats.m_last_block_time));
        obj.pushKV("bytessent", stats.nSendBytes);
        obj.pushKV("bytesrecv", stats.nRecvBytes);
        obj.pushKV("conntime", count_seconds(stats.m_connected));
        obj.pushKV("timeoffset", Ticks<std::chrono::seconds>(statestats.time_offset));
        if (stats.m_last_ping_time > 0us) {
            obj.pushKV("pingtime", Ticks<SecondsDouble>(stats.m_last_ping_time));
        }
        if (stats.m_min_ping_time < std::chrono::microseconds::max()) {
            obj.pushKV("minping", Ticks<SecondsDouble>(stats.m_min_ping_time));
        }
        if (statestats.m_ping_wait > 0s) {
            obj.pushKV("pingwait", Ticks<SecondsDouble>(statestats.m_ping_wait));
        }
        obj.pushKV("version", stats.nVersion);
        // Use the sanitized form of subver here, to avoid tricksy remote peers from
        // corrupting or modifying the JSON output by putting special characters in
        // their ver message.
        obj.pushKV("subver", stats.cleanSubVer);
        obj.pushKV("inbound", stats.fInbound);
        obj.pushKV("bip152_hb_to", stats.m_bip152_highbandwidth_to);
        obj.pushKV("bip152_hb_from", stats.m_bip152_highbandwidth_from);
        obj.pushKV("startingheight", statestats.m_starting_height);
        obj.pushKV("presynced_headers", statestats.presync_height);
        obj.pushKV("synced_headers", statestats.nSyncHeight);
        obj.pushKV("synced_blocks", statestats.nCommonHeight);
        UniValue heights(UniValue::VARR);
        for (const int height : statestats.vHeightInFlight) {
            heights.push_back(height);
        }
        obj.pushKV("inflight", std::move(heights));
        obj.pushKV("addr_relay_enabled", statestats.m_addr_relay_enabled);
        obj.pushKV("addr_processed", statestats.m_addr_processed);
        obj.pushKV("addr_rate_limited", statestats.m_addr_rate_limited);
        UniValue permissions(UniValue::VARR);
        for (const auto& permission : NetPermissions::ToStrings(stats.m_permission_flags)) {
            permissions.push_back(permission);
        }
        obj.pushKV("permissions", std::move(permissions));
        obj.pushKV("minfeefilter", ValueFromAmount(statestats.m_fee_filter_received));

        UniValue sendPerMsgType(UniValue::VOBJ);
        for (const auto& i : stats.mapSendBytesPerMsgType) {
            if (i.second > 0)
                sendPerMsgType.pushKV(i.first, i.second);
        }
        obj.pushKV("bytessent_per_msg", std::move(sendPerMsgType));

        UniValue recvPerMsgType(UniValue::VOBJ);
        for (const auto& i : stats.mapRecvBytesPerMsgType) {
            if (i.second > 0)
                recvPerMsgType.pushKV(i.first, i.second);
        }
        obj.pushKV("bytesrecv_per_msg", std::move(recvPerMsgType));
        obj.pushKV("connection_type", ConnectionTypeAsString(stats.m_conn_type));
        obj.pushKV("transport_protocol_type", TransportTypeAsString(stats.m_transport_type));
        obj.pushKV("session_id", stats.m_session_id);

        ret.push_back(std::move(obj));
    }

    return ret;
},
    };
}

static RPCHelpMan addnode()
{
    return RPCHelpMan{"addnode",
                "\nAttempts to add or remove a node from the addnode list.\n"
                "Or try a connection to a node once.\n"
                "Nodes added using addnode (or -connect) are protected from DoS disconnection and are not required to be\n"
                "full nodes/support SegWit as other outbound peers are (though such peers will not be synced from).\n" +
                strprintf("Addnode connections are limited to %u at a time", MAX_ADDNODE_CONNECTIONS) +
                " and are counted separately from the -maxconnections limit.\n",
                {
                    {"node", RPCArg::Type::STR, RPCArg::Optional::NO, "The address of the peer to connect to"},
                    {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "'add' to add a node to the list, 'remove' to remove a node from the list, 'onetry' to try a connection to the node once"},
                    {"v2transport", RPCArg::Type::BOOL, RPCArg::DefaultHint{"set by -v2transport"}, "Attempt to connect using BIP324 v2 transport protocol (ignored for 'remove' command)"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
                    HelpExampleCli("addnode", "\"192.168.0.6:8333\" \"onetry\" true")
            + HelpExampleRpc("addnode", "\"192.168.0.6:8333\", \"onetry\" true")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const auto command{self.Arg<std::string>("command")};
    if (command != "onetry" && command != "add" && command != "remove") {
        throw std::runtime_error(
            self.ToString());
    }

    NodeContext& node = EnsureAnyNodeContext(request.context);
    CConnman& connman = EnsureConnman(node);

    const auto node_arg{self.Arg<std::string>("node")};
    bool node_v2transport = connman.GetLocalServices() & NODE_P2P_V2;
    bool use_v2transport = self.MaybeArg<bool>("v2transport").value_or(node_v2transport);

    if (use_v2transport && !node_v2transport) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Error: v2transport requested but not enabled (see -v2transport)");
    }

    if (command == "onetry")
    {
        CAddress addr;
        connman.OpenNetworkConnection(addr, /*fCountFailure=*/false, /*grant_outbound=*/{}, node_arg.c_str(), ConnectionType::MANUAL, use_v2transport);
        return UniValue::VNULL;
    }

    if (command == "add")
    {
        if (!connman.AddNode({node_arg, use_v2transport})) {
            throw JSONRPCError(RPC_CLIENT_NODE_ALREADY_ADDED, "Error: Node already added");
        }
    }
    else if (command == "remove")
    {
        if (!connman.RemoveAddedNode(node_arg)) {
            throw JSONRPCError(RPC_CLIENT_NODE_NOT_ADDED, "Error: Node could not be removed. It has not been added previously.");
        }
    }

    return UniValue::VNULL;
},
    };
}

static RPCHelpMan addconnection()
{
    return RPCHelpMan{"addconnection",
        "\nOpen an outbound connection to a specified node. This RPC is for testing only.\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The IP address and port to attempt connecting to."},
            {"connection_type", RPCArg::Type::STR, RPCArg::Optional::NO, "Type of connection to open (\"outbound-full-relay\", \"block-relay-only\", \"addr-fetch\" or \"feeler\")."},
            {"v2transport", RPCArg::Type::BOOL, RPCArg::Optional::NO, "Attempt to connect using BIP324 v2 transport protocol"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                { RPCResult::Type::STR, "address", "Address of newly added connection." },
                { RPCResult::Type::STR, "connection_type", "Type of connection opened." },
            }},
        RPCExamples{
            HelpExampleCli("addconnection", "\"192.168.0.6:8333\" \"outbound-full-relay\" true")
            + HelpExampleRpc("addconnection", "\"192.168.0.6:8333\" \"outbound-full-relay\" true")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (Params().GetChainType() != ChainType::REGTEST) {
        throw std::runtime_error("addconnection is for regression testing (-regtest mode) only.");
    }

    const std::string address = request.params[0].get_str();
    const std::string conn_type_in{TrimString(request.params[1].get_str())};
    ConnectionType conn_type{};
    if (conn_type_in == "outbound-full-relay") {
        conn_type = ConnectionType::OUTBOUND_FULL_RELAY;
    } else if (conn_type_in == "block-relay-only") {
        conn_type = ConnectionType::BLOCK_RELAY;
    } else if (conn_type_in == "addr-fetch") {
        conn_type = ConnectionType::ADDR_FETCH;
    } else if (conn_type_in == "feeler") {
        conn_type = ConnectionType::FEELER;
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, self.ToString());
    }
    bool use_v2transport{self.Arg<bool>("v2transport")};

    NodeContext& node = EnsureAnyNodeContext(request.context);
    CConnman& connman = EnsureConnman(node);

    if (use_v2transport && !(connman.GetLocalServices() & NODE_P2P_V2)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Error: Adding v2transport connections requires -v2transport init flag to be set.");
    }

    const bool success = connman.AddConnection(address, conn_type, use_v2transport);
    if (!success) {
        throw JSONRPCError(RPC_CLIENT_NODE_CAPACITY_REACHED, "Error: Already at capacity for specified connection type.");
    }

    UniValue info(UniValue::VOBJ);
    info.pushKV("address", address);
    info.pushKV("connection_type", conn_type_in);

    return info;
},
    };
}

static RPCHelpMan disconnectnode()
{
    return RPCHelpMan{"disconnectnode",
                "\nImmediately disconnects from the specified peer node.\n"
                "\nStrictly one out of 'address' and 'nodeid' can be provided to identify the node.\n"
                "\nTo disconnect by nodeid, either set 'address' to the empty string, or call using the named 'nodeid' argument only.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::DefaultHint{"fallback to nodeid"}, "The IP address/port of the node"},
                    {"nodeid", RPCArg::Type::NUM, RPCArg::DefaultHint{"fallback to address"}, "The node ID (see getpeerinfo for node IDs)"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
                    HelpExampleCli("disconnectnode", "\"192.168.0.6:8333\"")
            + HelpExampleCli("disconnectnode", "\"\" 1")
            + HelpExampleRpc("disconnectnode", "\"192.168.0.6:8333\"")
            + HelpExampleRpc("disconnectnode", "\"\", 1")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    CConnman& connman = EnsureConnman(node);

    bool success;
    const UniValue &address_arg = request.params[0];
    const UniValue &id_arg = request.params[1];

    if (!address_arg.isNull() && id_arg.isNull()) {
        /* handle disconnect-by-address */
        success = connman.DisconnectNode(address_arg.get_str());
    } else if (!id_arg.isNull() && (address_arg.isNull() || (address_arg.isStr() && address_arg.get_str().empty()))) {
        /* handle disconnect-by-id */
        NodeId nodeid = (NodeId) id_arg.getInt<int64_t>();
        success = connman.DisconnectNode(nodeid);
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Only one of address and nodeid should be provided.");
    }

    if (!success) {
        throw JSONRPCError(RPC_CLIENT_NODE_NOT_CONNECTED, "Node not found in connected nodes");
    }

    return UniValue::VNULL;
},
    };
}

static RPCHelpMan getaddednodeinfo()
{
    return RPCHelpMan{"getaddednodeinfo",
                "\nReturns information about the given added node, or all added nodes\n"
                "(note that onetry addnodes are not listed here)\n",
                {
                    {"node", RPCArg::Type::STR, RPCArg::DefaultHint{"all nodes"}, "If provided, return information about this specific node, otherwise all nodes are returned."},
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "",
                    {
                        {RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR, "addednode", "The node IP address or name (as provided to addnode)"},
                            {RPCResult::Type::BOOL, "connected", "If connected"},
                            {RPCResult::Type::ARR, "addresses", "Only when connected = true",
                            {
                                {RPCResult::Type::OBJ, "", "",
                                {
                                    {RPCResult::Type::STR, "address", "The bitcoin server IP and port we're connected to"},
                                    {RPCResult::Type::STR, "connected", "connection, inbound or outbound"},
                                }},
                            }},
                        }},
                    }
                },
                RPCExamples{
                    HelpExampleCli("getaddednodeinfo", "\"192.168.0.201\"")
            + HelpExampleRpc("getaddednodeinfo", "\"192.168.0.201\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    const CConnman& connman = EnsureConnman(node);

    std::vector<AddedNodeInfo> vInfo = connman.GetAddedNodeInfo(/*include_connected=*/true);

    if (!request.params[0].isNull()) {
        bool found = false;
        for (const AddedNodeInfo& info : vInfo) {
            if (info.m_params.m_added_node == request.params[0].get_str()) {
                vInfo.assign(1, info);
                found = true;
                break;
            }
        }
        if (!found) {
            throw JSONRPCError(RPC_CLIENT_NODE_NOT_ADDED, "Error: Node has not been added.");
        }
    }

    UniValue ret(UniValue::VARR);

    for (const AddedNodeInfo& info : vInfo) {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("addednode", info.m_params.m_added_node);
        obj.pushKV("connected", info.fConnected);
        UniValue addresses(UniValue::VARR);
        if (info.fConnected) {
            UniValue address(UniValue::VOBJ);
            address.pushKV("address", info.resolvedAddress.ToStringAddrPort());
            address.pushKV("connected", info.fInbound ? "inbound" : "outbound");
            addresses.push_back(std::move(address));
        }
        obj.pushKV("addresses", std::move(addresses));
        ret.push_back(std::move(obj));
    }

    return ret;
},
    };
}

static RPCHelpMan getnettotals()
{
    return RPCHelpMan{"getnettotals",
        "Returns information about network traffic, including bytes in, bytes out,\n"
        "and current system time.",
        {},
                RPCResult{
                   RPCResult::Type::OBJ, "", "",
                   {
                       {RPCResult::Type::NUM, "totalbytesrecv", "Total bytes received"},
                       {RPCResult::Type::NUM, "totalbytessent", "Total bytes sent"},
                       {RPCResult::Type::NUM_TIME, "timemillis", "Current system " + UNIX_EPOCH_TIME + " in milliseconds"},
                       {RPCResult::Type::OBJ, "uploadtarget", "",
                       {
                           {RPCResult::Type::NUM, "timeframe", "Length of the measuring timeframe in seconds"},
                           {RPCResult::Type::NUM, "target", "Target in bytes"},
                           {RPCResult::Type::BOOL, "target_reached", "True if target is reached"},
                           {RPCResult::Type::BOOL, "serve_historical_blocks", "True if serving historical blocks"},
                           {RPCResult::Type::NUM, "bytes_left_in_cycle", "Bytes left in current time cycle"},
                           {RPCResult::Type::NUM, "time_left_in_cycle", "Seconds left in current time cycle"},
                        }},
                    }
                },
                RPCExamples{
                    HelpExampleCli("getnettotals", "")
            + HelpExampleRpc("getnettotals", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    const CConnman& connman = EnsureConnman(node);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("totalbytesrecv", connman.GetTotalBytesRecv());
    obj.pushKV("totalbytessent", connman.GetTotalBytesSent());
    obj.pushKV("timemillis", TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));

    UniValue outboundLimit(UniValue::VOBJ);
    outboundLimit.pushKV("timeframe", count_seconds(connman.GetMaxOutboundTimeframe()));
    outboundLimit.pushKV("target", connman.GetMaxOutboundTarget());
    outboundLimit.pushKV("target_reached", connman.OutboundTargetReached(false));
    outboundLimit.pushKV("serve_historical_blocks", !connman.OutboundTargetReached(true));
    outboundLimit.pushKV("bytes_left_in_cycle", connman.GetOutboundTargetBytesLeft());
    outboundLimit.pushKV("time_left_in_cycle", count_seconds(connman.GetMaxOutboundTimeLeftInCycle()));
    obj.pushKV("uploadtarget", std::move(outboundLimit));
    return obj;
},
    };
}

static UniValue GetNetworksInfo()
{
    UniValue networks(UniValue::VARR);
    for (int n = 0; n < NET_MAX; ++n) {
        enum Network network = static_cast<enum Network>(n);
        if (network == NET_UNROUTABLE || network == NET_INTERNAL) continue;
        Proxy proxy;
        UniValue obj(UniValue::VOBJ);
        GetProxy(network, proxy);
        obj.pushKV("name", GetNetworkName(network));
        obj.pushKV("limited", !g_reachable_nets.Contains(network));
        obj.pushKV("reachable", g_reachable_nets.Contains(network));
        obj.pushKV("proxy", proxy.IsValid() ? proxy.ToString() : std::string());
        obj.pushKV("proxy_randomize_credentials", proxy.m_randomize_credentials);
        networks.push_back(std::move(obj));
    }
    return networks;
}

static RPCHelpMan getnetworkinfo()
{
    return RPCHelpMan{"getnetworkinfo",
                "Returns an object containing various state info regarding P2P networking.\n",
                {},
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::NUM, "version", "the server version"},
                        {RPCResult::Type::STR, "subversion", "the server subversion string"},
                        {RPCResult::Type::NUM, "protocolversion", "the protocol version"},
                        {RPCResult::Type::STR_HEX, "localservices", "the services we offer to the network"},
                        {RPCResult::Type::ARR, "localservicesnames", "the services we offer to the network, in human-readable form",
                        {
                            {RPCResult::Type::STR, "SERVICE_NAME", "the service name"},
                        }},
                        {RPCResult::Type::BOOL, "localrelay", "true if transaction relay is requested from peers"},
                        {RPCResult::Type::NUM, "timeoffset", "the time offset"},
                        {RPCResult::Type::NUM, "connections", "the total number of connections"},
                        {RPCResult::Type::NUM, "connections_in", "the number of inbound connections"},
                        {RPCResult::Type::NUM, "connections_out", "the number of outbound connections"},
                        {RPCResult::Type::BOOL, "networkactive", "whether p2p networking is enabled"},
                        {RPCResult::Type::ARR, "networks", "information per network",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR, "name", "network (" + Join(GetNetworkNames(), ", ") + ")"},
                                {RPCResult::Type::BOOL, "limited", "is the network limited using -onlynet?"},
                                {RPCResult::Type::BOOL, "reachable", "is the network reachable?"},
                                {RPCResult::Type::STR, "proxy", "(\"host:port\") the proxy that is used for this network, or empty if none"},
                                {RPCResult::Type::BOOL, "proxy_randomize_credentials", "Whether randomized credentials are used"},
                            }},
                        }},
                        {RPCResult::Type::NUM, "relayfee", "minimum relay fee rate for transactions in " + CURRENCY_UNIT + "/kvB"},
                        {RPCResult::Type::NUM, "incrementalfee", "minimum fee rate increment for mempool limiting or replacement in " + CURRENCY_UNIT + "/kvB"},
                        {RPCResult::Type::ARR, "localaddresses", "list of local addresses",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR, "address", "network address"},
                                {RPCResult::Type::NUM, "port", "network port"},
                                {RPCResult::Type::NUM, "score", "relative score"},
                            }},
                        }},
                        (IsDeprecatedRPCEnabled("warnings") ?
                            RPCResult{RPCResult::Type::STR, "warnings", "any network and blockchain warnings (DEPRECATED)"} :
                            RPCResult{RPCResult::Type::ARR, "warnings", "any network and blockchain warnings (run with `-deprecatedrpc=warnings` to return the latest warning as a single string)",
                            {
                                {RPCResult::Type::STR, "", "warning"},
                            }
                            }
                        ),
                    }
                },
                RPCExamples{
                    HelpExampleCli("getnetworkinfo", "")
            + HelpExampleRpc("getnetworkinfo", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    LOCK(cs_main);
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("version",       CLIENT_VERSION);
    obj.pushKV("subversion",    strSubVersion);
    obj.pushKV("protocolversion",PROTOCOL_VERSION);
    NodeContext& node = EnsureAnyNodeContext(request.context);
    if (node.connman) {
        ServiceFlags services = node.connman->GetLocalServices();
        obj.pushKV("localservices", strprintf("%016x", services));
        obj.pushKV("localservicesnames", GetServicesNames(services));
    }
    if (node.peerman) {
        auto peerman_info{node.peerman->GetInfo()};
        obj.pushKV("localrelay", !peerman_info.ignores_incoming_txs);
        obj.pushKV("timeoffset", Ticks<std::chrono::seconds>(peerman_info.median_outbound_time_offset));
    }
    if (node.connman) {
        obj.pushKV("networkactive", node.connman->GetNetworkActive());
        obj.pushKV("connections", node.connman->GetNodeCount(ConnectionDirection::Both));
        obj.pushKV("connections_in", node.connman->GetNodeCount(ConnectionDirection::In));
        obj.pushKV("connections_out", node.connman->GetNodeCount(ConnectionDirection::Out));
    }
    obj.pushKV("networks",      GetNetworksInfo());
    if (node.mempool) {
        // Those fields can be deprecated, to be replaced by the getmempoolinfo fields
        obj.pushKV("relayfee", ValueFromAmount(node.mempool->m_opts.min_relay_feerate.GetFeePerK()));
        obj.pushKV("incrementalfee", ValueFromAmount(node.mempool->m_opts.incremental_relay_feerate.GetFeePerK()));
    }
    UniValue localAddresses(UniValue::VARR);
    {
        LOCK(g_maplocalhost_mutex);
        for (const std::pair<const CNetAddr, LocalServiceInfo> &item : mapLocalHost)
        {
            UniValue rec(UniValue::VOBJ);
            rec.pushKV("address", item.first.ToStringAddr());
            rec.pushKV("port", item.second.nPort);
            rec.pushKV("score", item.second.nScore);
            localAddresses.push_back(std::move(rec));
        }
    }
    obj.pushKV("localaddresses", std::move(localAddresses));
    obj.pushKV("warnings", node::GetWarningsForRpc(*CHECK_NONFATAL(node.warnings), IsDeprecatedRPCEnabled("warnings")));
    return obj;
},
    };
}

static RPCHelpMan setban()
{
    return RPCHelpMan{"setban",
                "\nAttempts to add or remove an IP/Subnet from the banned list.\n",
                {
                    {"subnet", RPCArg::Type::STR, RPCArg::Optional::NO, "The IP/Subnet (see getpeerinfo for nodes IP) with an optional netmask (default is /32 = single IP)"},
                    {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "'add' to add an IP/Subnet to the list, 'remove' to remove an IP/Subnet from the list"},
                    {"bantime", RPCArg::Type::NUM, RPCArg::Default{0}, "time in seconds how long (or until when if [absolute] is set) the IP is banned (0 or empty means using the default time of 24h which can also be overwritten by the -bantime startup argument)"},
                    {"absolute", RPCArg::Type::BOOL, RPCArg::Default{false}, "If set, the bantime must be an absolute timestamp expressed in " + UNIX_EPOCH_TIME},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
                    HelpExampleCli("setban", "\"192.168.0.6\" \"add\" 86400")
                            + HelpExampleCli("setban", "\"192.168.0.0/24\" \"add\"")
                            + HelpExampleRpc("setban", "\"192.168.0.6\", \"add\", 86400")
                },
        [&](const RPCHelpMan& help, const JSONRPCRequest& request) -> UniValue
{
    std::string strCommand;
    if (!request.params[1].isNull())
        strCommand = request.params[1].get_str();
    if (strCommand != "add" && strCommand != "remove") {
        throw std::runtime_error(help.ToString());
    }
    NodeContext& node = EnsureAnyNodeContext(request.context);
    BanMan& banman = EnsureBanman(node);

    CSubNet subNet;
    CNetAddr netAddr;
    bool isSubnet = false;

    if (request.params[0].get_str().find('/') != std::string::npos)
        isSubnet = true;

    if (!isSubnet) {
        const std::optional<CNetAddr> addr{LookupHost(request.params[0].get_str(), false)};
        if (addr.has_value()) {
            netAddr = static_cast<CNetAddr>(MaybeFlipIPv6toCJDNS(CService{addr.value(), /*port=*/0}));
        }
    }
    else
        subNet = LookupSubNet(request.params[0].get_str());

    if (! (isSubnet ? subNet.IsValid() : netAddr.IsValid()) )
        throw JSONRPCError(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Error: Invalid IP/Subnet");

    if (strCommand == "add")
    {
        if (isSubnet ? banman.IsBanned(subNet) : banman.IsBanned(netAddr)) {
            throw JSONRPCError(RPC_CLIENT_NODE_ALREADY_ADDED, "Error: IP/Subnet already banned");
        }

        int64_t banTime = 0; //use standard bantime if not specified
        if (!request.params[2].isNull())
            banTime = request.params[2].getInt<int64_t>();

        const bool absolute{request.params[3].isNull() ? false : request.params[3].get_bool()};

        if (absolute && banTime < GetTime()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Error: Absolute timestamp is in the past");
        }

        if (isSubnet) {
            banman.Ban(subNet, banTime, absolute);
            if (node.connman) {
                node.connman->DisconnectNode(subNet);
            }
        } else {
            banman.Ban(netAddr, banTime, absolute);
            if (node.connman) {
                node.connman->DisconnectNode(netAddr);
            }
        }
    }
    else if(strCommand == "remove")
    {
        if (!( isSubnet ? banman.Unban(subNet) : banman.Unban(netAddr) )) {
            throw JSONRPCError(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Error: Unban failed. Requested address/subnet was not previously manually banned.");
        }
    }
    return UniValue::VNULL;
},
    };
}

static RPCHelpMan listbanned()
{
    return RPCHelpMan{"listbanned",
                "\nList all manually banned IPs/Subnets.\n",
                {},
        RPCResult{RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "address", "The IP/Subnet of the banned node"},
                        {RPCResult::Type::NUM_TIME, "ban_created", "The " + UNIX_EPOCH_TIME + " the ban was created"},
                        {RPCResult::Type::NUM_TIME, "banned_until", "The " + UNIX_EPOCH_TIME + " the ban expires"},
                        {RPCResult::Type::NUM_TIME, "ban_duration", "The ban duration, in seconds"},
                        {RPCResult::Type::NUM_TIME, "time_remaining", "The time remaining until the ban expires, in seconds"},
                    }},
            }},
                RPCExamples{
                    HelpExampleCli("listbanned", "")
                            + HelpExampleRpc("listbanned", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    BanMan& banman = EnsureAnyBanman(request.context);

    banmap_t banMap;
    banman.GetBanned(banMap);
    const int64_t current_time{GetTime()};

    UniValue bannedAddresses(UniValue::VARR);
    for (const auto& entry : banMap)
    {
        const CBanEntry& banEntry = entry.second;
        UniValue rec(UniValue::VOBJ);
        rec.pushKV("address", entry.first.ToString());
        rec.pushKV("ban_created", banEntry.nCreateTime);
        rec.pushKV("banned_until", banEntry.nBanUntil);
        rec.pushKV("ban_duration", (banEntry.nBanUntil - banEntry.nCreateTime));
        rec.pushKV("time_remaining", (banEntry.nBanUntil - current_time));

        bannedAddresses.push_back(std::move(rec));
    }

    return bannedAddresses;
},
    };
}

static RPCHelpMan clearbanned()
{
    return RPCHelpMan{"clearbanned",
                "\nClear all banned IPs.\n",
                {},
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
                    HelpExampleCli("clearbanned", "")
                            + HelpExampleRpc("clearbanned", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    BanMan& banman = EnsureAnyBanman(request.context);

    banman.ClearBanned();

    return UniValue::VNULL;
},
    };
}

static RPCHelpMan setnetworkactive()
{
    return RPCHelpMan{"setnetworkactive",
                "\nDisable/enable all p2p network activity.\n",
                {
                    {"state", RPCArg::Type::BOOL, RPCArg::Optional::NO, "true to enable networking, false to disable"},
                },
                RPCResult{RPCResult::Type::BOOL, "", "The value that was passed in"},
                RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    CConnman& connman = EnsureConnman(node);

    connman.SetNetworkActive(request.params[0].get_bool());

    return connman.GetNetworkActive();
},
    };
}

static RPCHelpMan getnodeaddresses()
{
    return RPCHelpMan{"getnodeaddresses",
                "Return known addresses, after filtering for quality and recency.\n"
                "These can potentially be used to find new peers in the network.\n"
                "The total number of addresses known to the node may be higher.",
                {
                    {"count", RPCArg::Type::NUM, RPCArg::Default{1}, "The maximum number of addresses to return. Specify 0 to return all known addresses."},
                    {"network", RPCArg::Type::STR, RPCArg::DefaultHint{"all networks"}, "Return only addresses of the specified network. Can be one of: " + Join(GetNetworkNames(), ", ") + "."},
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "",
                    {
                        {RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::NUM_TIME, "time", "The " + UNIX_EPOCH_TIME + " when the node was last seen"},
                            {RPCResult::Type::NUM, "services", "The services offered by the node"},
                            {RPCResult::Type::STR, "address", "The address of the node"},
                            {RPCResult::Type::NUM, "port", "The port number of the node"},
                            {RPCResult::Type::STR, "network", "The network (" + Join(GetNetworkNames(), ", ") + ") the node connected through"},
                        }},
                    }
                },
                RPCExamples{
                    HelpExampleCli("getnodeaddresses", "8")
                    + HelpExampleCli("getnodeaddresses", "4 \"i2p\"")
                    + HelpExampleCli("-named getnodeaddresses", "network=onion count=12")
                    + HelpExampleRpc("getnodeaddresses", "8")
                    + HelpExampleRpc("getnodeaddresses", "4, \"i2p\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    const CConnman& connman = EnsureConnman(node);

    const int count{request.params[0].isNull() ? 1 : request.params[0].getInt<int>()};
    if (count < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Address count out of range");

    const std::optional<Network> network{request.params[1].isNull() ? std::nullopt : std::optional<Network>{ParseNetwork(request.params[1].get_str())}};
    if (network == NET_UNROUTABLE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Network not recognized: %s", request.params[1].get_str()));
    }

    // returns a shuffled list of CAddress
    const std::vector<CAddress> vAddr{connman.GetAddresses(count, /*max_pct=*/0, network)};
    UniValue ret(UniValue::VARR);

    for (const CAddress& addr : vAddr) {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("time", int64_t{TicksSinceEpoch<std::chrono::seconds>(addr.nTime)});
        obj.pushKV("services", (uint64_t)addr.nServices);
        obj.pushKV("address", addr.ToStringAddr());
        obj.pushKV("port", addr.GetPort());
        obj.pushKV("network", GetNetworkName(addr.GetNetClass()));
        ret.push_back(std::move(obj));
    }
    return ret;
},
    };
}

static RPCHelpMan addpeeraddress()
{
    return RPCHelpMan{"addpeeraddress",
        "Add the address of a potential peer to an address manager table. This RPC is for testing only.",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The IP address of the peer"},
            {"port", RPCArg::Type::NUM, RPCArg::Optional::NO, "The port of the peer"},
            {"tried", RPCArg::Type::BOOL, RPCArg::Default{false}, "If true, attempt to add the peer to the tried addresses table"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "success", "whether the peer address was successfully added to the address manager table"},
                {RPCResult::Type::STR, "error", /*optional=*/true, "error description, if the address could not be added"},
            },
        },
        RPCExamples{
            HelpExampleCli("addpeeraddress", "\"1.2.3.4\" 8333 true")
    + HelpExampleRpc("addpeeraddress", "\"1.2.3.4\", 8333, true")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    AddrMan& addrman = EnsureAnyAddrman(request.context);

    const std::string& addr_string{request.params[0].get_str()};
    const auto port{request.params[1].getInt<uint16_t>()};
    const bool tried{request.params[2].isNull() ? false : request.params[2].get_bool()};

    UniValue obj(UniValue::VOBJ);
    std::optional<CNetAddr> net_addr{LookupHost(addr_string, false)};
    bool success{false};

    if (net_addr.has_value()) {
        CService service{net_addr.value(), port};
        CAddress address{MaybeFlipIPv6toCJDNS(service), ServiceFlags{NODE_NETWORK | NODE_WITNESS}};
        address.nTime = Now<NodeSeconds>();
        // The source address is set equal to the address. This is equivalent to the peer
        // announcing itself.
        if (addrman.Add({address}, address)) {
            success = true;
            if (tried) {
                // Attempt to move the address to the tried addresses table.
                if (!addrman.Good(address)) {
                    success = false;
                    obj.pushKV("error", "failed-adding-to-tried");
                }
            }
        } else {
            obj.pushKV("error", "failed-adding-to-new");
        }
    }

    obj.pushKV("success", success);
    return obj;
},
    };
}

static RPCHelpMan sendmsgtopeer()
{
    return RPCHelpMan{
        "sendmsgtopeer",
        "Send a p2p message to a peer specified by id.\n"
        "The message type and body must be provided, the message header will be generated.\n"
        "This RPC is for testing only.",
        {
            {"peer_id", RPCArg::Type::NUM, RPCArg::Optional::NO, "The peer to send the message to."},
            {"msg_type", RPCArg::Type::STR, RPCArg::Optional::NO, strprintf("The message type (maximum length %i)", CMessageHeader::COMMAND_SIZE)},
            {"msg", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The serialized message body to send, in hex, without a message header"},
        },
        RPCResult{RPCResult::Type::OBJ, "", "", std::vector<RPCResult>{}},
        RPCExamples{
            HelpExampleCli("sendmsgtopeer", "0 \"addr\" \"ffffff\"") + HelpExampleRpc("sendmsgtopeer", "0 \"addr\" \"ffffff\"")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            const NodeId peer_id{request.params[0].getInt<int64_t>()};
            const std::string& msg_type{request.params[1].get_str()};
            if (msg_type.size() > CMessageHeader::COMMAND_SIZE) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Error: msg_type too long, max length is %i", CMessageHeader::COMMAND_SIZE));
            }
            auto msg{TryParseHex<unsigned char>(request.params[2].get_str())};
            if (!msg.has_value()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Error parsing input for msg");
            }

            NodeContext& node = EnsureAnyNodeContext(request.context);
            CConnman& connman = EnsureConnman(node);

            CSerializedNetMsg msg_ser;
            msg_ser.data = msg.value();
            msg_ser.m_type = msg_type;

            bool success = connman.ForNode(peer_id, [&](CNode* node) {
                connman.PushMessage(node, std::move(msg_ser));
                return true;
            });

            if (!success) {
                throw JSONRPCError(RPC_MISC_ERROR, "Error: Could not send message to peer");
            }

            UniValue ret{UniValue::VOBJ};
            return ret;
        },
    };
}

static RPCHelpMan getaddrmaninfo()
{
    return RPCHelpMan{
        "getaddrmaninfo",
        "\nProvides information about the node's address manager by returning the number of "
        "addresses in the `new` and `tried` tables and their sum for all networks.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ_DYN, "", "json object with network type as keys", {
                {RPCResult::Type::OBJ, "network", "the network (" + Join(GetNetworkNames(), ", ") + ", all_networks)", {
                {RPCResult::Type::NUM, "new", "number of addresses in the new table, which represent potential peers the node has discovered but hasn't yet successfully connected to."},
                {RPCResult::Type::NUM, "tried", "number of addresses in the tried table, which represent peers the node has successfully connected to in the past."},
                {RPCResult::Type::NUM, "total", "total number of addresses in both new/tried tables"},
            }},
        }},
        RPCExamples{HelpExampleCli("getaddrmaninfo", "") + HelpExampleRpc("getaddrmaninfo", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            AddrMan& addrman = EnsureAnyAddrman(request.context);

            UniValue ret(UniValue::VOBJ);
            for (int n = 0; n < NET_MAX; ++n) {
                enum Network network = static_cast<enum Network>(n);
                if (network == NET_UNROUTABLE || network == NET_INTERNAL) continue;
                UniValue obj(UniValue::VOBJ);
                obj.pushKV("new", addrman.Size(network, true));
                obj.pushKV("tried", addrman.Size(network, false));
                obj.pushKV("total", addrman.Size(network));
                ret.pushKV(GetNetworkName(network), std::move(obj));
            }
            UniValue obj(UniValue::VOBJ);
            obj.pushKV("new", addrman.Size(std::nullopt, true));
            obj.pushKV("tried", addrman.Size(std::nullopt, false));
            obj.pushKV("total", addrman.Size());
            ret.pushKV("all_networks", std::move(obj));
            return ret;
        },
    };
}

UniValue AddrmanEntryToJSON(const AddrInfo& info, CConnman& connman)
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("address", info.ToStringAddr());
    const auto mapped_as{connman.GetMappedAS(info)};
    if (mapped_as != 0) {
        ret.pushKV("mapped_as", mapped_as);
    }
    ret.pushKV("port", info.GetPort());
    ret.pushKV("services", (uint64_t)info.nServices);
    ret.pushKV("time", int64_t{TicksSinceEpoch<std::chrono::seconds>(info.nTime)});
    ret.pushKV("network", GetNetworkName(info.GetNetClass()));
    ret.pushKV("source", info.source.ToStringAddr());
    ret.pushKV("source_network", GetNetworkName(info.source.GetNetClass()));
    const auto source_mapped_as{connman.GetMappedAS(info.source)};
    if (source_mapped_as != 0) {
        ret.pushKV("source_mapped_as", source_mapped_as);
    }
    return ret;
}

UniValue AddrmanTableToJSON(const std::vector<std::pair<AddrInfo, AddressPosition>>& tableInfos, CConnman& connman)
{
    UniValue table(UniValue::VOBJ);
    for (const auto& e : tableInfos) {
        AddrInfo info = e.first;
        AddressPosition location = e.second;
        std::ostringstream key;
        key << location.bucket << "/" << location.position;
        // Address manager tables have unique entries so there is no advantage
        // in using UniValue::pushKV, which checks if the key already exists
        // in O(N). UniValue::pushKVEnd is used instead which currently is O(1).
        table.pushKVEnd(key.str(), AddrmanEntryToJSON(info, connman));
    }
    return table;
}

static RPCHelpMan getrawaddrman()
{
    return RPCHelpMan{"getrawaddrman",
        "EXPERIMENTAL warning: this call may be changed in future releases.\n"
        "\nReturns information on all address manager entries for the new and tried tables.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ_DYN, "", "", {
                {RPCResult::Type::OBJ_DYN, "table", "buckets with addresses in the address manager table ( new, tried )", {
                    {RPCResult::Type::OBJ, "bucket/position", "the location in the address manager table (<bucket>/<position>)", {
                        {RPCResult::Type::STR, "address", "The address of the node"},
                        {RPCResult::Type::NUM, "mapped_as", /*optional=*/true, "The ASN mapped to the IP of this peer per our current ASMap"},
                        {RPCResult::Type::NUM, "port", "The port number of the node"},
                        {RPCResult::Type::STR, "network", "The network (" + Join(GetNetworkNames(), ", ") + ") of the address"},
                        {RPCResult::Type::NUM, "services", "The services offered by the node"},
                        {RPCResult::Type::NUM_TIME, "time", "The " + UNIX_EPOCH_TIME + " when the node was last seen"},
                        {RPCResult::Type::STR, "source", "The address that relayed the address to us"},
                        {RPCResult::Type::STR, "source_network", "The network (" + Join(GetNetworkNames(), ", ") + ") of the source address"},
                        {RPCResult::Type::NUM, "source_mapped_as", /*optional=*/true, "The ASN mapped to the IP of this peer's source per our current ASMap"}
                    }}
                }}
            }
        },
        RPCExamples{
            HelpExampleCli("getrawaddrman", "")
            + HelpExampleRpc("getrawaddrman", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            AddrMan& addrman = EnsureAnyAddrman(request.context);
            NodeContext& node_context = EnsureAnyNodeContext(request.context);
            CConnman& connman = EnsureConnman(node_context);

            UniValue ret(UniValue::VOBJ);
            ret.pushKV("new", AddrmanTableToJSON(addrman.GetEntries(false), connman));
            ret.pushKV("tried", AddrmanTableToJSON(addrman.GetEntries(true), connman));
            return ret;
        },
    };
}

void RegisterNetRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"network", &getconnectioncount},
        {"network", &ping},
        {"network", &getpeerinfo},
        {"network", &addnode},
        {"network", &disconnectnode},
        {"network", &getaddednodeinfo},
        {"network", &getnettotals},
        {"network", &getnetworkinfo},
        {"network", &setban},
        {"network", &listbanned},
        {"network", &clearbanned},
        {"network", &setnetworkactive},
        {"network", &getnodeaddresses},
        {"network", &getaddrmaninfo},
        {"hidden", &addconnection},
        {"hidden", &addpeeraddress},
        {"hidden", &sendmsgtopeer},
        {"hidden", &getrawaddrman},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
