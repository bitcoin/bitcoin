// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>

#include <banman.h>
#include <clientversion.h>
#include <core_io.h>
#include <net.h>
#include <net_processing.h>
#include <net_permissions.h>
#include <netbase.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <rpc/protocol.h>
#include <rpc/util.h>
#include <sync.h>
#include <timedata.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <validation.h>
#include <version.h>
#include <warnings.h>

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

    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    return (int)g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL);
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

    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    // Request that each node send a ping during next message processing pass
    g_connman->ForEachNode([](CNode* pnode) {
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
            "    \"services\":\"xxxxxxxxxxxxxxxx\",   (string) The services offered\n"
            "    \"servicesnames\":[              (array) the services offered, in human-readable form\n"
            "        \"SERVICE_NAME\",         (string) the service name if it is recognised\n"
            "         ...\n"
            "     ],\n"
            "    \"relaytxes\":true|false,    (boolean) Whether peer has asked us to relay transactions to it\n"
            "    \"lastsend\": ttt,           (numeric) The time in seconds since epoch (Jan 1 1970 GMT) of the last send\n"
            "    \"lastrecv\": ttt,           (numeric) The time in seconds since epoch (Jan 1 1970 GMT) of the last receive\n"
            "    \"bytessent\": n,            (numeric) The total bytes sent\n"
            "    \"bytesrecv\": n,            (numeric) The total bytes received\n"
            "    \"conntime\": ttt,           (numeric) The connection time in seconds since epoch (Jan 1 1970 GMT)\n"
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

    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    std::vector<CNodeStats> vstats;
    g_connman->GetNodeStats(vstats);

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
                    HelpExampleCli("addnode", "\"192.168.0.6:3333\" \"onetry\"")
            + HelpExampleRpc("addnode", "\"192.168.0.6:3333\", \"onetry\"")
                },
            }.ToString());

    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    std::string strNode = request.params[0].get_str();

    if (strCommand == "onetry")
    {
        CAddress addr;
        g_connman->OpenNetworkConnection(addr, false, nullptr, strNode.c_str(), false, false, true);
        return NullUniValue;
    }

    if (strCommand == "add")
    {
        if(!g_connman->AddNode(strNode))
            throw JSONRPCError(RPC_CLIENT_NODE_ALREADY_ADDED, "Error: Node already added");
    }
    else if(strCommand == "remove")
    {
        if(!g_connman->RemoveAddedNode(strNode))
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
                    HelpExampleCli("disconnectnode", "\"192.168.0.6:3333\"")
            + HelpExampleCli("disconnectnode", "\"\" 1")
            + HelpExampleRpc("disconnectnode", "\"192.168.0.6:3333\"")
            + HelpExampleRpc("disconnectnode", "\"\", 1")
                },
            }.Check(request);

    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    bool success;
    const UniValue &address_arg = request.params[0];
    const UniValue &id_arg = request.params[1];

    if (!address_arg.isNull() && id_arg.isNull()) {
        /* handle disconnect-by-address */
        success = g_connman->DisconnectNode(address_arg.get_str());
    } else if (!id_arg.isNull() && (address_arg.isNull() || (address_arg.isStr() && address_arg.get_str().empty()))) {
        /* handle disconnect-by-id */
        NodeId nodeid = (NodeId) id_arg.get_int64();
        success = g_connman->DisconnectNode(nodeid);
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
            "         \"address\" : \"192.168.0.201:3333\",  (string) The bitcoin server IP and port we're connected to\n"
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

    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    std::vector<AddedNodeInfo> vInfo = g_connman->GetAddedNodeInfo();

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
    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("totalbytesrecv", g_connman->GetTotalBytesRecv());
    obj.pushKV("totalbytessent", g_connman->GetTotalBytesSent());
    obj.pushKV("timemillis", GetTimeMillis());

    UniValue outboundLimit(UniValue::VOBJ);
    outboundLimit.pushKV("timeframe", g_connman->GetMaxOutboundTimeframe());
    outboundLimit.pushKV("target", g_connman->GetMaxOutboundTarget());
    outboundLimit.pushKV("target_reached", g_connman->OutboundTargetReached(false));
    outboundLimit.pushKV("serve_historical_blocks", !g_connman->OutboundTargetReached(true));
    outboundLimit.pushKV("bytes_left_in_cycle", g_connman->GetOutboundTargetBytesLeft());
    outboundLimit.pushKV("time_left_in_cycle", g_connman->GetMaxOutboundTimeLeftInCycle());
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
            "{\n"
            "  \"version\": xxxxx,                      (numeric) the server version\n"
            "  \"subversion\": \"/Satoshi:x.x.x/\",     (string) the server subversion string\n"
            "  \"protocolversion\": xxxxx,              (numeric) the protocol version\n"
            "  \"localservices\": \"xxxxxxxxxxxxxxxx\", (string) the services we offer to the network\n"
            "  \"localservicesnames\": [                (array) the services we offer to the network, in human-readable form\n"
            "      \"SERVICE_NAME\",                    (string) the service name\n"
            "       ...\n"
            "   ],\n"
            "  \"localrelay\": true|false,              (bool) true if transaction relay is requested from peers\n"
            "  \"timeoffset\": xxxxx,                   (numeric) the time offset\n"
            "  \"connections\": xxxxx,                  (numeric) the number of connections\n"
            "  \"networkactive\": true|false,           (bool) whether p2p networking is enabled\n"
            "  \"networks\": [                          (array) information per network\n"
            "  {\n"
            "    \"name\": \"xxx\",                     (string) network (ipv4, ipv6 or onion)\n"
            "    \"limited\": true|false,               (boolean) is the network limited using -onlynet?\n"
            "    \"reachable\": true|false,             (boolean) is the network reachable?\n"
            "    \"proxy\": \"host:port\"               (string) the proxy that is used for this network, or empty if none\n"
            "    \"proxy_randomize_credentials\": true|false,  (string) Whether randomized credentials are used\n"
            "  }\n"
            "  ,...\n"
            "  ],\n"
            "  \"relayfee\": x.xxxxxxxx,                (numeric) minimum relay fee for transactions in " + CURRENCY_UNIT + "/kB\n"
            "  \"incrementalfee\": x.xxxxxxxx,          (numeric) minimum fee increment for mempool limiting or BIP 125 replacement in " + CURRENCY_UNIT + "/kB\n"
            "  \"localaddresses\": [                    (array) list of local addresses\n"
            "  {\n"
            "    \"address\": \"xxxx\",                 (string) network address\n"
            "    \"port\": xxx,                         (numeric) network port\n"
            "    \"score\": xxx                         (numeric) relative score\n"
            "  }\n"
            "  ,...\n"
            "  ]\n"
            "  \"warnings\": \"...\"                    (string) any network and blockchain warnings\n"
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
    if (g_connman) {
        ServiceFlags services = g_connman->GetLocalServices();
        obj.pushKV("localservices", strprintf("%016x", services));
        obj.pushKV("localservicesnames", GetServicesNames(services));
    }
    obj.pushKV("localrelay", g_relay_txes);
    obj.pushKV("timeoffset",    GetTimeOffset());
    if (g_connman) {
        obj.pushKV("networkactive", g_connman->GetNetworkActive());
        obj.pushKV("connections",   (int)g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL));
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
    obj.pushKV("warnings",       GetWarnings("statusbar"));
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
                    {"absolute", RPCArg::Type::BOOL, /* default */ "false", "If set, the bantime must be an absolute timestamp in seconds since epoch (Jan 1 1970 GMT)"},
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
    if (!g_banman) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Error: Ban database not loaded");
    }

    CSubNet subNet;
    CNetAddr netAddr;
    bool isSubnet = false;

    if (request.params[0].get_str().find('/') != std::string::npos)
        isSubnet = true;

    if (!isSubnet) {
        CNetAddr resolved;
        LookupHost(request.params[0].get_str().c_str(), resolved, false);
        netAddr = resolved;
    }
    else
        LookupSubNet(request.params[0].get_str().c_str(), subNet);

    if (! (isSubnet ? subNet.IsValid() : netAddr.IsValid()) )
        throw JSONRPCError(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Error: Invalid IP/Subnet");

    if (strCommand == "add")
    {
        if (isSubnet ? g_banman->IsBanned(subNet) : g_banman->IsBanned(netAddr)) {
            throw JSONRPCError(RPC_CLIENT_NODE_ALREADY_ADDED, "Error: IP/Subnet already banned");
        }

        int64_t banTime = 0; //use standard bantime if not specified
        if (!request.params[2].isNull())
            banTime = request.params[2].get_int64();

        bool absolute = false;
        if (request.params[3].isTrue())
            absolute = true;

        if (isSubnet) {
            g_banman->Ban(subNet, BanReasonManuallyAdded, banTime, absolute);
            if (g_connman) {
                g_connman->DisconnectNode(subNet);
            }
        } else {
            g_banman->Ban(netAddr, BanReasonManuallyAdded, banTime, absolute);
            if (g_connman) {
                g_connman->DisconnectNode(netAddr);
            }
        }
    }
    else if(strCommand == "remove")
    {
        if (!( isSubnet ? g_banman->Unban(subNet) : g_banman->Unban(netAddr) )) {
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

    if(!g_banman) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Error: Ban database not loaded");
    }

    banmap_t banMap;
    g_banman->GetBanned(banMap);

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
    if (!g_banman) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Error: Ban database not loaded");
    }

    g_banman->ClearBanned();

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

    if (!g_connman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    }

    g_connman->SetNetworkActive(request.params[0].get_bool());

    return g_connman->GetNetworkActive();
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
            "    \"time\": ttt,                (numeric) Timestamp in seconds since epoch (Jan 1 1970 GMT) keeping track of when the node was last seen\n"
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
    if (!g_connman) {
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
    std::vector<CAddress> vAddr = g_connman->GetAddresses();
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
};
// clang-format on

void RegisterNetRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
