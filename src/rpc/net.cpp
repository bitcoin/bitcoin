// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2014-2024 The Dash Core developers
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
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const CConnman& connman = EnsureConnman(node);

    return (int)connman.GetNodeCount(ConnectionDirection::Both);
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
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    PeerManager& peerman = EnsurePeerman(node);

    // Request that each node send a ping during next message processing pass
    peerman.SendPings();
    return NullUniValue;
},
    };
}

static RPCHelpMan getpeerinfo()
{
    return RPCHelpMan{
        "getpeerinfo",
        "\nReturns data about each connected network node as a json array of objects.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {
                    {RPCResult::Type::NUM, "id", "Peer index"},
                    {RPCResult::Type::STR, "addr", "(host:port) The IP address and port of the peer"},
                    {RPCResult::Type::STR, "addrbind", /* optional */ true, "(ip:port) Bind address of the connection to the peer"},
                    {RPCResult::Type::STR, "addrlocal", /* optional */ true, "(ip:port) Local address as reported by the peer"},
                    {RPCResult::Type::STR, "network", "Network (" + Join(GetNetworkNames(/* append_unroutable */ true), ", ") + ")"},
                    {RPCResult::Type::STR, "mapped_as", /* optional */ true, "The AS in the BGP route to the peer used for diversifying peer selection"},
                    {RPCResult::Type::STR_HEX, "services", "The services offered"},
                    {RPCResult::Type::ARR, "servicesnames", "the services offered, in human-readable form",
                    {
                        {RPCResult::Type::STR, "SERVICE_NAME", "the service name if it is recognised"}
                    }},
                    {RPCResult::Type::STR_HEX, "verified_proregtx_hash", true /*optional*/, "Only present when the peer is a masternode and successfully "
                                                                        "authenticated via MNAUTH. In this case, this field contains the "
                                                                        "protx hash of the masternode"},
                    {RPCResult::Type::STR_HEX, "verified_pubkey_hash", true /*optional*/, "Only present when the peer is a masternode and successfully "
                                                                        "authenticated via MNAUTH. In this case, this field contains the "
                                                                        "hash of the masternode's operator public key"},
                    {RPCResult::Type::BOOL, "relaytxes", "Whether peer has asked us to relay transactions to it"},
                    {RPCResult::Type::NUM_TIME, "lastsend", "The " + UNIX_EPOCH_TIME + " of the last send"},
                    {RPCResult::Type::NUM_TIME, "lastrecv", "The " + UNIX_EPOCH_TIME + " of the last receive"},
                    {RPCResult::Type::NUM_TIME, "last_transaction", "The " + UNIX_EPOCH_TIME + " of the last valid transaction received from this peer"},
                    {RPCResult::Type::NUM_TIME, "last_block", "The " + UNIX_EPOCH_TIME + " of the last block received from this peer"},
                    {RPCResult::Type::NUM, "bytessent", "The total bytes sent"},
                    {RPCResult::Type::NUM, "bytesrecv", "The total bytes received"},
                    {RPCResult::Type::NUM_TIME, "conntime", "The " + UNIX_EPOCH_TIME + " of the connection"},
                    {RPCResult::Type::NUM, "timeoffset", "The time offset in seconds"},
                    {RPCResult::Type::NUM, "pingtime", /* optional */ true, "ping time (if available)"},
                    {RPCResult::Type::NUM, "minping", /* optional */ true, "minimum observed ping time (if any at all)"},
                    {RPCResult::Type::NUM, "pingwait", /* optional */ true, "ping wait (if non-zero)"},
                    {RPCResult::Type::NUM, "version", "The peer version, such as 70001"},
                    {RPCResult::Type::STR, "subver", "The string version"},
                    {RPCResult::Type::BOOL, "inbound", "Inbound (true) or Outbound (false)"},
                    {RPCResult::Type::BOOL, "bip152_hb_to", "Whether we selected peer as (compact blocks) high-bandwidth peer"},
                    {RPCResult::Type::BOOL, "bip152_hb_from", "Whether peer selected us as (compact blocks) high-bandwidth peer"},
                    {RPCResult::Type::BOOL, "masternode", "Whether connection was due to masternode connection attempt"},
                    {RPCResult::Type::NUM, "banscore", "The ban score (DEPRECATED, returned only if config option -deprecatedrpc=banscore is passed)"},
                    {RPCResult::Type::NUM, "startingheight", /*optional=*/true, "The starting height (block) of the peer"},
                    {RPCResult::Type::NUM, "synced_headers", /*optional=*/true, "The last header we have in common with this peer"},
                    {RPCResult::Type::NUM, "synced_blocks", /*optional=*/true, "The last block we have in common with this peer"},
                    {RPCResult::Type::ARR, "inflight", /*optional=*/true, "",
                    {
                        {RPCResult::Type::NUM, "n", "The heights of blocks we're currently asking from this peer"},
                    }},
                    {RPCResult::Type::BOOL, "addr_relay_enabled", /*optional=*/true, "Whether we participate in address relay with this peer"},
                    {RPCResult::Type::NUM, "addr_processed", /*optional=*/true, "The total number of addresses processed, excluding those dropped due to rate limiting"},
                    {RPCResult::Type::NUM, "addr_rate_limited", /*optional=*/true, "The total number of addresses dropped due to rate limiting"},
                    {RPCResult::Type::ARR, "permissions", "Any special permissions that have been granted to this peer",
                    {
                        {RPCResult::Type::STR, "permission_type", Join(NET_PERMISSIONS_DOC, ",\n") + ".\n"},
                    }},
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
                                                      "Only known message types can appear as keys in the object and all bytes received of unknown message types are listed under '"+NET_MESSAGE_TYPE_OTHER+"'."}
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
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const CConnman& connman = EnsureConnman(node);
    const PeerManager& peerman = EnsurePeerman(node);

    std::vector<CNodeStats> vstats;
    connman.GetNodeStats(vstats);

    UniValue ret(UniValue::VARR);

    for (const CNodeStats& stats : vstats) {
        UniValue obj(UniValue::VOBJ);
        CNodeStateStats statestats;
        bool fStateStats = peerman.GetNodeStateStats(stats.nodeid, statestats);
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
        ServiceFlags services{fStateStats ? statestats.their_services : ServiceFlags::NODE_NONE};
        obj.pushKV("services", strprintf("%016x", services));
        obj.pushKV("servicesnames", GetServicesNames(services));
        if (!stats.verifiedProRegTxHash.IsNull()) {
            obj.pushKV("verified_proregtx_hash", stats.verifiedProRegTxHash.ToString());
        }
        if (!stats.verifiedPubKeyHash.IsNull()) {
            obj.pushKV("verified_pubkey_hash", stats.verifiedPubKeyHash.ToString());
        }
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
        if (fStateStats && statestats.m_ping_wait > 0s) {
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
        obj.pushKV("masternode", stats.m_masternode_connection);
        if (fStateStats) {
            if (IsDeprecatedRPCEnabled("banscore")) {
                // TODO: banscore is deprecated in v21 for removal in v22, maybe impossible due to usages in p2p_quorum_data.py
                obj.pushKV("banscore", statestats.m_misbehavior_score);
            }
            obj.pushKV("startingheight", statestats.m_starting_height);
            obj.pushKV("synced_headers", statestats.nSyncHeight);
            obj.pushKV("synced_blocks", statestats.nCommonHeight);
            UniValue heights(UniValue::VARR);
            for (const int height : statestats.vHeightInFlight) {
                heights.push_back(height);
            }
            obj.pushKV("inflight", heights);
            obj.pushKV("relaytxes", statestats.m_relay_txs);
            obj.pushKV("addr_relay_enabled", statestats.m_addr_relay_enabled);
            obj.pushKV("addr_processed", statestats.m_addr_processed);
            obj.pushKV("addr_rate_limited", statestats.m_addr_rate_limited);
        }
        UniValue permissions(UniValue::VARR);
        for (const auto& permission : NetPermissions::ToStrings(stats.m_permission_flags)) {
            permissions.push_back(permission);
        }
        obj.pushKV("permissions", permissions);

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
        obj.pushKV("transport_protocol_type", TransportTypeAsString(stats.m_transport_type));
        obj.pushKV("session_id", stats.m_session_id);

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
        "full nodes as other outbound peers are (though such peers will not be synced from).\n" +
                strprintf("Addnode connections are limited to %u at a time", MAX_ADDNODE_CONNECTIONS) +
                " and are counted separately from the -maxconnections limit.\n",
        {
            {"node", RPCArg::Type::STR, RPCArg::Optional::NO, "The address of the peer to connect to"},
            {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "'add' to add a node to the list, 'remove' to remove a node from the list, 'onetry' to try a connection to the node once"},
            {"v2transport", RPCArg::Type::BOOL, RPCArg::DefaultHint{"set by -v2transport"}, "Attempt to connect using BIP324 v2 transport protocol (ignored for 'remove' command)"},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
            HelpExampleCli("addnode", "\"192.168.0.6:9999\" \"onetry\" true")
    + HelpExampleRpc("addnode", "\"192.168.0.6:9999\", \"onetry\" true")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::string command{request.params[1].get_str()};
    if (command != "onetry" && command != "add" && command != "remove") {
        throw std::runtime_error(
            self.ToString());
    }

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    CConnman& connman = EnsureConnman(node);

    const std::string node_arg = request.params[0].get_str();
    bool node_v2transport = connman.GetLocalServices() & NODE_P2P_V2;
    bool use_v2transport = request.params[2].isNull() ? node_v2transport : request.params[2].get_bool();

    if (use_v2transport && !node_v2transport) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Error: v2transport requested but not enabled (see -v2transport)");
    }

    if (command == "onetry")
    {
        CAddress addr;
        connman.OpenNetworkConnection(addr, /*fCountFailure=*/false, /*grant_outbound=*/{}, node_arg.c_str(), ConnectionType::MANUAL, use_v2transport);
        return NullUniValue;
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

    return NullUniValue;
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
    if (Params().NetworkIDString() != CBaseChainParams::REGTEST) {
        throw std::runtime_error("addconnection is for regression testing (-regtest mode) only.");
    }

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VSTR});
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
    bool use_v2transport = !request.params[2].isNull() && request.params[2].get_bool();

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
            HelpExampleCli("disconnectnode", "\"192.168.0.6:9999\"")
    + HelpExampleCli("disconnectnode", "\"\" 1")
    + HelpExampleRpc("disconnectnode", "\"192.168.0.6:9999\"")
    + HelpExampleRpc("disconnectnode", "\"\", 1")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    CConnman& connman = EnsureConnman(node);

    bool success;
    const UniValue &address_arg = request.params[0];
    const UniValue &id_arg = request.params[1];

    if (!address_arg.isNull() && id_arg.isNull()) {
        /* handle disconnect-by-address */
        success = connman.DisconnectNode(address_arg.get_str());
    } else if (!id_arg.isNull() && (address_arg.isNull() || (address_arg.isStr() && address_arg.get_str().empty()))) {
        /* handle disconnect-by-id */
        NodeId nodeid = (NodeId) id_arg.get_int64();
        success = connman.DisconnectNode(nodeid);
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Only one of address and nodeid should be provided.");
    }

    if (!success) {
        throw JSONRPCError(RPC_CLIENT_NODE_NOT_CONNECTED, "Node not found in connected nodes");
    }

    return NullUniValue;
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
                                    {RPCResult::Type::STR, "address", "The Dash server IP and port we're connected to"},
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

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const CConnman& connman = EnsureConnman(node);;

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
    const NodeContext& node = EnsureAnyNodeContext(request.context);
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
        obj.pushKV("limited", !g_reachable_nets.Contains(network));
        obj.pushKV("reachable", g_reachable_nets.Contains(network));
        obj.pushKV("proxy", proxy.IsValid() ? proxy.ToString() : std::string());
        obj.pushKV("proxy_randomize_credentials", proxy.m_randomize_credentials);
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
                        {RPCResult::Type::STR, "buildversion", "the server build version including RC info or commit as relevant"},
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
                        {RPCResult::Type::NUM, "connections_mn", "the number of verified mn connections"},
                        {RPCResult::Type::NUM, "connections_mn_in", "the number of inbound verified mn connections"},
                        {RPCResult::Type::NUM, "connections_mn_out", "the number of outbound verified mn connections"},
                        {RPCResult::Type::BOOL, "networkactive", "whether p2p networking is enabled"},
                        {RPCResult::Type::STR, "socketevents", "the socket events mode, either kqueue, epoll, poll or select"},
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
                        {RPCResult::Type::NUM, "relayfee", "minimum relay fee for transactions in " + CURRENCY_UNIT + "/kB"},
                        {RPCResult::Type::NUM, "incrementalfee", "minimum fee increment for mempool limiting in " + CURRENCY_UNIT + "/kB"},
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
    obj.pushKV("buildversion",  FormatFullVersion());
    obj.pushKV("subversion",    strSubVersion);
    obj.pushKV("protocolversion",PROTOCOL_VERSION);
    const NodeContext& node = EnsureAnyNodeContext(request.context);
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
        obj.pushKV("connections",   (int)node.connman->GetNodeCount(ConnectionDirection::Both));
        obj.pushKV("connections_in",   (int)node.connman->GetNodeCount(ConnectionDirection::In));
        obj.pushKV("connections_out",   (int)node.connman->GetNodeCount(ConnectionDirection::Out));
        obj.pushKV("connections_mn",   (int)node.connman->GetNodeCount(ConnectionDirection::Verified));
        obj.pushKV("connections_mn_in",   (int)node.connman->GetNodeCount(ConnectionDirection::VerifiedIn));
        obj.pushKV("connections_mn_out",   (int)node.connman->GetNodeCount(ConnectionDirection::VerifiedOut));
        std::string_view sem_str = SEMToString(node.connman->GetSocketEventsMode());
        CHECK_NONFATAL(sem_str != "unknown");
        obj.pushKV("socketevents", std::string(sem_str));
    }
    obj.pushKV("networks",      GetNetworksInfo());
    obj.pushKV("relayfee",      ValueFromAmount(::minRelayTxFee.GetFeePerK()));
    obj.pushKV("incrementalfee", ValueFromAmount(::incrementalRelayFee.GetFeePerK()));
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
    const NodeContext& node = EnsureAnyNodeContext(request.context);
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
            banTime = request.params[2].get_int64();

        bool absolute = false;
        if (request.params[3].isTrue())
            absolute = true;

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
    return NullUniValue;
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

    return NullUniValue;
},
    };
}

static RPCHelpMan cleardiscouraged()
{
    return RPCHelpMan{"cleardiscouraged",
               "\nClear all discouraged nodes.\n",
               {},
               RPCResult{RPCResult::Type::NONE, "", ""},
               RPCExamples{
                       HelpExampleCli("cleardiscouraged", "")
                       + HelpExampleRpc("cleardiscouraged", "")
               },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    BanMan& banman = EnsureAnyBanman(request.context);

    banman.ClearDiscouraged();

    return NullUniValue;
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

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    CConnman& connman = EnsureConnman(node);

    connman.SetNetworkActive(request.params[0].get_bool(), node.mn_sync.get());

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
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const CConnman& connman = EnsureConnman(node);

    const int count{request.params[0].isNull() ? 1 : request.params[0].get_int()};
    if (count < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Address count out of range");

    const std::optional<Network> network{request.params[1].isNull() ? std::nullopt : std::optional<Network>{ParseNetwork(request.params[1].get_str())}};
    if (network == NET_UNROUTABLE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Network not recognized: %s", request.params[1].get_str()));
    }

    // returns a shuffled list of CAddress
    const std::vector<CAddress> vAddr{connman.GetAddresses(count, /* max_pct */ 0, network)};
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
            HelpExampleCli("addpeeraddress", "\"1.2.3.4\" 9999 true")
    + HelpExampleRpc("addpeeraddress", "\"1.2.3.4\", 9999, true")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    if (!node.addrman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Address manager functionality missing or disabled");
    }

    const std::string& addr_string{request.params[0].get_str()};
    const uint16_t port = request.params[1].get_int();
    const bool tried{request.params[2].isTrue()};

    UniValue obj(UniValue::VOBJ);
    std::optional<CNetAddr> net_addr{LookupHost(addr_string, false)};
    bool success{false};

    if (net_addr.has_value()) {
        CService service{net_addr.value(), port};
        CAddress address{MaybeFlipIPv6toCJDNS(service), ServiceFlags{NODE_NETWORK}};
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
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
            HelpExampleCli("sendmsgtopeer", "0 \"addr\" \"ffffff\"") + HelpExampleRpc("sendmsgtopeer", "0 \"addr\" \"ffffff\"")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            const NodeId peer_id{request.params[0].get_int()};
            const std::string& msg_type{request.params[1].get_str()};
            if (msg_type.size() > CMessageHeader::COMMAND_SIZE) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Error: msg_type too long, max length is %i", CMessageHeader::COMMAND_SIZE));
            }
            const std::string& msg{request.params[2].get_str()};
            if (!msg.empty() && !IsHex(msg)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Error parsing input for msg");
            }

            NodeContext& node = EnsureAnyNodeContext(request.context);
            CConnman& connman = EnsureConnman(node);

            CSerializedNetMsg msg_ser;
            msg_ser.data = ParseHex(msg);
            msg_ser.m_type = msg_type;

            bool success = connman.ForNode(peer_id, [&](CNode* node) {
                connman.PushMessage(node, std::move(msg_ser));
                return true;
            });

            if (!success) {
                throw JSONRPCError(RPC_MISC_ERROR, "Error: Could not send message to peer");
            }

            return NullUniValue;
        },
    };
}

static RPCHelpMan setmnthreadactive()
{
    return RPCHelpMan{"setmnthreadactive",
        "\nDisable/enable automatic masternode connections thread activity.\n",
        {
            {"state", RPCArg::Type::BOOL, RPCArg::Optional::NO, "true to enable the thread, false to disable"},
        },
        RPCResult{RPCResult::Type::BOOL, "", "The value that was passed in"},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    if (Params().NetworkIDString() != CBaseChainParams::REGTEST) {
        throw std::runtime_error("setmnthreadactive is for regression testing (-regtest mode) only.");
    }

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    CConnman& connman = EnsureConnman(node);

    connman.SetMasternodeThreadActive(request.params[0].get_bool());

    return connman.GetMasternodeThreadActive();
},
    };
}

void RegisterNetRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor
  //  --------------------- -----------------------
    { "network",             &getconnectioncount,      },
    { "network",             &ping,                    },
    { "network",             &getpeerinfo,             },
    { "network",             &addnode,                 },
    { "network",             &disconnectnode,          },
    { "network",             &getaddednodeinfo,        },
    { "network",             &getnettotals,            },
    { "network",             &getnetworkinfo,          },
    { "network",             &setban,                  },
    { "network",             &listbanned,              },
    { "network",             &clearbanned,             },
    { "network",             &setnetworkactive,        },
    { "network",             &getnodeaddresses,        },

    { "hidden",              &cleardiscouraged,        },
    { "hidden",              &addconnection,           },
    { "hidden",              &addpeeraddress,          },
    { "hidden",              &sendmsgtopeer            },
    { "hidden",              &setmnthreadactive        },
};
// clang-format on
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
