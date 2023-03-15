// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>

#include <addrman.h>
#include <banman.h>
#include <chainparams.h>
#include <clientversion.h>
#include <core_io.h>
#include <net_permissions.h>
#include <net_processing.h>
#include <net_types.h> // For banmap_t
#include <netbase.h>
#include <node/context.h>
#include <policy/settings.h>
#include <rpc/blockchain.h>
#include <rpc/protocol.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <sync.h>
#include <timedata.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>
#include <version.h>
#include <warnings.h>

#include <optional>

#include <univalue.h>

using node::NodeContext;

const std::vector<std::string> CONNECTION_TYPE_DOC{
        "outbound-full-relay (default automatic connections)",
        "block-relay-only (does not relay transactions or addresses)",
        "inbound (initiated by the peer)",
        "manual (added via addnode RPC or -addnode/-connect configuration options)",
        "addr-fetch (short-lived automatic connection for soliciting addresses)",
        "feeler (short-lived automatic connection for testing addresses)"
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
        obj.pushKV("timeoffset", stats.nTimeOffset);
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
        obj.pushKV("inflight", heights);
        obj.pushKV("addr_relay_enabled", statestats.m_addr_relay_enabled);
        obj.pushKV("addr_processed", statestats.m_addr_processed);
        obj.pushKV("addr_rate_limited", statestats.m_addr_rate_limited);
        UniValue permissions(UniValue::VARR);
        for (const auto& permission : NetPermissions::ToStrings(stats.m_permission_flags)) {
            permissions.push_back(permission);
        }
        obj.pushKV("permissions", permissions);
        obj.pushKV("minfeefilter", ValueFromAmount(statestats.m_fee_filter_received));

        UniValue sendPerMsgType(UniValue::VOBJ);
        for (const auto& i : stats.mapSendBytesPerMsgType) {
            if (i.second > 0)
                sendPerMsgType.pushKV(i.first, i.second);
        }
        obj.pushKV("bytessent_per_msg", sendPerMsgType);

        UniValue recvPerMsgType(UniValue::VOBJ);
        for (const auto& i : stats.mapRecvBytesPerMsgType) {
            if (i.second > 0)
                recvPerMsgType.pushKV(i.first, i.second);
        }
        obj.pushKV("bytesrecv_per_msg", recvPerMsgType);
        obj.pushKV("connection_type", ConnectionTypeAsString(stats.m_conn_type));

        ret.push_back(obj);
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
                    {"node", RPCArg::Type::STR, RPCArg::Optional::NO, "The node (see getpeerinfo for nodes)"},
                    {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "'add' to add a node to the list, 'remove' to remove a node from the list, 'onetry' to try a connection to the node once"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
                    HelpExampleCli("addnode", "\"192.168.0.6:8333\" \"onetry\"")
            + HelpExampleRpc("addnode", "\"192.168.0.6:8333\", \"onetry\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strCommand;
    if (!request.params[1].isNull())
        strCommand = request.params[1].get_str();
    if (strCommand != "onetry" && strCommand != "add" && strCommand != "remove") {
        throw std::runtime_error(
            self.ToString());
    }

    NodeContext& node = EnsureAnyNodeContext(request.context);
    CConnman& connman = EnsureConnman(node);

    std::string strNode = request.params[0].get_str();

    if (strCommand == "onetry")
    {
        CAddress addr;
        connman.OpenNetworkConnection(addr, false, nullptr, strNode.c_str(), ConnectionType::MANUAL);
        return UniValue::VNULL;
    }

    if (strCommand == "add")
    {
        if (!connman.AddNode(strNode)) {
            throw JSONRPCError(RPC_CLIENT_NODE_ALREADY_ADDED, "Error: Node already added");
        }
    }
    else if(strCommand == "remove")
    {
        if (!connman.RemoveAddedNode(strNode)) {
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
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                { RPCResult::Type::STR, "address", "Address of newly added connection." },
                { RPCResult::Type::STR, "connection_type", "Type of connection opened." },
            }},
        RPCExamples{
            HelpExampleCli("addconnection", "\"192.168.0.6:8333\" \"outbound-full-relay\"")
            + HelpExampleRpc("addconnection", "\"192.168.0.6:8333\" \"outbound-full-relay\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (Params().NetworkIDString() != CBaseChainParams::REGTEST) {
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

    NodeContext& node = EnsureAnyNodeContext(request.context);
    CConnman& connman = EnsureConnman(node);

    const bool success = connman.AddConnection(address, conn_type);
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

    std::vector<AddedNodeInfo> vInfo = connman.GetAddedNodeInfo();

    if (!request.params[0].isNull()) {
        bool found = false;
        for (const AddedNodeInfo& info : vInfo) {
            if (info.strAddedNode == request.params[0].get_str()) {
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
        obj.pushKV("addednode", info.strAddedNode);
        obj.pushKV("connected", info.fConnected);
        UniValue addresses(UniValue::VARR);
        if (info.fConnected) {
            UniValue address(UniValue::VOBJ);
            address.pushKV("address", info.resolvedAddress.ToStringAddrPort());
            address.pushKV("connected", info.fInbound ? "inbound" : "outbound");
            addresses.push_back(address);
        }
        obj.pushKV("addresses", addresses);
        ret.push_back(obj);
    }

    return ret;
},
    };
}

static RPCHelpMan getnettotals()
{
    return RPCHelpMan{"getnettotals",
                "\nReturns information about network traffic, including bytes in, bytes out,\n"
                "and current time.\n",
                {},
                RPCResult{
                   RPCResult::Type::OBJ, "", "",
                   {
                       {RPCResult::Type::NUM, "totalbytesrecv", "Total bytes received"},
                       {RPCResult::Type::NUM, "totalbytessent", "Total bytes sent"},
                       {RPCResult::Type::NUM_TIME, "timemillis", "Current " + UNIX_EPOCH_TIME + " in milliseconds"},
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
    obj.pushKV("timemillis", GetTimeMillis());

    UniValue outboundLimit(UniValue::VOBJ);
    outboundLimit.pushKV("timeframe", count_seconds(connman.GetMaxOutboundTimeframe()));
    outboundLimit.pushKV("target", connman.GetMaxOutboundTarget());
    outboundLimit.pushKV("target_reached", connman.OutboundTargetReached(false));
    outboundLimit.pushKV("serve_historical_blocks", !connman.OutboundTargetReached(true));
    outboundLimit.pushKV("bytes_left_in_cycle", connman.GetOutboundTargetBytesLeft());
    outboundLimit.pushKV("time_left_in_cycle", count_seconds(connman.GetMaxOutboundTimeLeftInCycle()));
    obj.pushKV("uploadtarget", outboundLimit);
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
        obj.pushKV("limited", !IsReachable(network));
        obj.pushKV("reachable", IsReachable(network));
        obj.pushKV("proxy", proxy.IsValid() ? proxy.proxy.ToStringAddrPort() : std::string());
        obj.pushKV("proxy_randomize_credentials", proxy.randomize_credentials);
        networks.push_back(obj);
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
                        {RPCResult::Type::STR, "warnings", "any network and blockchain warnings"},
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
        obj.pushKV("localrelay", !node.peerman->IgnoresIncomingTxs());
    }
    obj.pushKV("timeoffset",    GetTimeOffset());
    if (node.connman) {
        obj.pushKV("networkactive", node.connman->GetNetworkActive());
        obj.pushKV("connections", node.connman->GetNodeCount(ConnectionDirection::Both));
        obj.pushKV("connections_in", node.connman->GetNodeCount(ConnectionDirection::In));
        obj.pushKV("connections_out", node.connman->GetNodeCount(ConnectionDirection::Out));
    }
    obj.pushKV("networks",      GetNetworksInfo());
    if (node.mempool) {
        // Those fields can be deprecated, to be replaced by the getmempoolinfo fields
        obj.pushKV("relayfee", ValueFromAmount(node.mempool->m_min_relay_feerate.GetFeePerK()));
        obj.pushKV("incrementalfee", ValueFromAmount(node.mempool->m_incremental_relay_feerate.GetFeePerK()));
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
            localAddresses.push_back(rec);
        }
    }
    obj.pushKV("localaddresses", localAddresses);
    obj.pushKV("warnings",       GetWarnings(false).original);
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
        CNetAddr resolved;
        LookupHost(request.params[0].get_str(), resolved, false);
        netAddr = resolved;
    }
    else
        LookupSubNet(request.params[0].get_str(), subNet);

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

        bannedAddresses.push_back(rec);
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
        ret.push_back(obj);
    }
    return ret;
},
    };
}

static RPCHelpMan addpeeraddress()
{
    return RPCHelpMan{"addpeeraddress",
        "\nAdd the address of a potential peer to the address manager. This RPC is for testing only.\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The IP address of the peer"},
            {"port", RPCArg::Type::NUM, RPCArg::Optional::NO, "The port of the peer"},
            {"tried", RPCArg::Type::BOOL, RPCArg::Default{false}, "If true, attempt to add the peer to the tried addresses table"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "success", "whether the peer address was successfully added to the address manager"},
            },
        },
        RPCExamples{
            HelpExampleCli("addpeeraddress", "\"1.2.3.4\" 8333 true")
    + HelpExampleRpc("addpeeraddress", "\"1.2.3.4\", 8333, true")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    if (!node.addrman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Address manager functionality missing or disabled");
    }

    const std::string& addr_string{request.params[0].get_str()};
    const auto port{request.params[1].getInt<uint16_t>()};
    const bool tried{request.params[2].isNull() ? false : request.params[2].get_bool()};

    UniValue obj(UniValue::VOBJ);
    CNetAddr net_addr;
    bool success{false};

    if (LookupHost(addr_string, net_addr, false)) {
        CService service{net_addr, port};
        CAddress address{MaybeFlipIPv6toCJDNS(service), ServiceFlags{NODE_NETWORK | NODE_WITNESS}};
        address.nTime = Now<NodeSeconds>();
        // The source address is set equal to the address. This is equivalent to the peer
        // announcing itself.
        if (node.addrman->Add({address}, address)) {
            success = true;
            if (tried) {
                // Attempt to move the address to the tried addresses table.
                node.addrman->Good(address);
            }
        }
    }

    obj.pushKV("success", success);
    return obj;
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
        {"hidden", &addconnection},
        {"hidden", &addpeeraddress},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
