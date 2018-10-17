// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>

#include <chainparams.h>
#include <clientversion.h>
#include <core_io.h>
#include <net.h>
#include <net_processing.h>
#include <netbase.h>
#include <policy/policy.h>
#include <rpc/doc.h>
#include <rpc/protocol.h>
#include <sync.h>
#include <timedata.h>
#include <ui_interface.h>
#include <util.h>
#include <utilstrencodings.h>
#include <validation.h>
#include <version.h>
#include <warnings.h>

#include <univalue.h>

static UniValue getconnectioncount(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw RPCDoc("getconnectioncount")
            .Desc("Returns the number of connections to other nodes.")
            .Table("Result")
            .Row("n", {"numeric"}, "The connection count")
            .ExampleCli("")
            .ExampleRpc("")
            .AsError();

    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    return (int)g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL);
}

static UniValue ping(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw RPCDoc("ping")
            .Desc(
                "Requests that a ping be sent to all other nodes, to measure ping time.\n"
                "Results provided in getpeerinfo, pingtime and pingwait fields are decimal seconds.\n"
                "Ping command is handled in queue with all other commands, so it measures processing backlog, not just network ping.")
            .ExampleCli("")
            .ExampleRpc("")
            .AsError();

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
    if (request.fHelp || request.params.size() != 0)
        throw RPCDoc("getpeerinfo")
            .Desc("Returns data about each connected network node as a json array of objects.")
            .Table("Result")
            .Row("[")
            .Row("  {")
            .Row("    \"id\": n,", {"numeric"}, "Peer index")
            .Row("    \"addr\":\"host:port\",", {"string"}, "The IP address and port of the peer")
            .Row("    \"addrbind\":\"ip:port\",", {"string"}, "Bind address of the connection to the peer")
            .Row("    \"addrlocal\":\"ip:port\",", {"string"}, "Local address as reported by the peer")
            .Row("    \"services\":\"xxxxxxxxxxxxxxxx\",", {"string"}, "The services offered")
            .Row("    \"relaytxes\":true|false,", {"boolean"}, "Whether peer has asked us to relay transactions to it")
            .Row("    \"lastsend\": ttt,", {"numeric"}, "The time in seconds since epoch (Jan 1 1970 GMT) of the last send")
            .Row("    \"lastrecv\": ttt,", {"numeric"}, "The time in seconds since epoch (Jan 1 1970 GMT) of the last receive")
            .Row("    \"bytessent\": n,", {"numeric"}, "The total bytes sent")
            .Row("    \"bytesrecv\": n,", {"numeric"}, "The total bytes received")
            .Row("    \"conntime\": ttt,", {"numeric"}, "The connection time in seconds since epoch (Jan 1 1970 GMT)")
            .Row("    \"timeoffset\": ttt,", {"numeric"}, "The time offset in seconds")
            .Row("    \"pingtime\": n,", {"numeric"}, "ping time (if available)")
            .Row("    \"minping\": n,", {"numeric"}, "minimum observed ping time (if any at all)")
            .Row("    \"pingwait\": n,", {"numeric"}, "ping wait (if non-zero)")
            .Row("    \"version\": v,", {"numeric"}, "The peer version, such as 70001")
            .Row("    \"subver\": \"/Satoshi:0.8.5/\",", {"string"}, "The string version")
            .Row("    \"inbound\": true|false,", {"boolean"}, "Inbound (true) or Outbound (false)")
            .Row("    \"addnode\": true|false,", {"boolean"}, "Whether connection was due to addnode/-connect or if it was an automatic/inbound connection")
            .Row("    \"startingheight\": n,", {"numeric"}, "The starting height (block) of the peer")
            .Row("    \"banscore\": n,", {"numeric"}, "The ban score")
            .Row("    \"synced_headers\": n,", {"numeric"}, "The last header we have in common with this peer")
            .Row("    \"synced_blocks\": n,", {"numeric"}, "The last block we have in common with this peer")
            .Row("    \"inflight\": [")
            .Row("       n,", {"numeric"}, "The heights of blocks we're currently asking from this peer")
            .Row("       ...")
            .Row("    ],")
            .Row("    \"whitelisted\": true|false,", {"boolean"}, "Whether the peer is whitelisted")
            .Row("    \"minfeefilter\": n,", {"numeric"}, "The minimum fee rate for transactions this peer accepts")
            .Row("    \"bytessent_per_msg\": {")
            .Row("       \"addr\": n,", {"numeric"}, "The total bytes sent aggregated by message type")
            .Row("       ...")
            .Row("    },")
            .Row("    \"bytesrecv_per_msg\": {")
            .Row("       \"addr\": n,", {"numeric"}, "The total bytes received aggregated by message type")
            .Row("       ...")
            .Row("    }")
            .Row("  }")
            .Row("  ,...")
            .Row("]")
            .ExampleCli("")
            .ExampleRpc("")
            .AsError();

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
        obj.pushKV("whitelisted", stats.fWhitelisted);
        obj.pushKV("minfeefilter", ValueFromAmount(stats.minFeeFilter));

        UniValue sendPerMsgCmd(UniValue::VOBJ);
        for (const mapMsgCmdSize::value_type &i : stats.mapSendBytesPerMsgCmd) {
            if (i.second > 0)
                sendPerMsgCmd.pushKV(i.first, i.second);
        }
        obj.pushKV("bytessent_per_msg", sendPerMsgCmd);

        UniValue recvPerMsgCmd(UniValue::VOBJ);
        for (const mapMsgCmdSize::value_type &i : stats.mapRecvBytesPerMsgCmd) {
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
        throw RPCDoc("addnode", "\"node\" \"add|remove|onetry\"")
            .Desc(
                "Attempts to add or remove a node from the addnode list.\n"
                "Or try a connection to a node once.\n"
                "Nodes added using addnode (or -connect) are protected from DoS disconnection and are not required to be\n"
                "full nodes/support SegWit as other outbound peers are (though such peers will not be synced from).")
            .Table("Arguments")
            .Row("1. \"node\"", {"string", "required"}, "The node (see getpeerinfo for nodes)")
            .Row("2. \"command\"", {"string", "required"}, "'add' to add a node to the list, 'remove' to remove a node from the list, 'onetry' to try a connection to the node once")
            .ExampleCli("\"192.168.0.6:8333\" \"onetry\"")
            .ExampleRpc("\"192.168.0.6:8333\", \"onetry\"")
            .AsError();

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
    if (request.fHelp || request.params.size() == 0 || request.params.size() >= 3)
        throw RPCDoc("disconnectnode", "\"[address]\" [nodeid]")
            .Desc(
                "Immediately disconnects from the specified peer node.\n"
                "\n"
                "Strictly one out of 'address' and 'nodeid' can be provided to identify the node.\n"
                "\n"
                "To disconnect by nodeid, either set 'address' to the empty string, or call using the named 'nodeid' argument only.")
            .Table("Arguments")
            .Row("1. \"address\"", {"string", "optional"}, "The IP address/port of the node")
            .Row("2. \"nodeid\"", {"number", "optional"}, "The node ID (see getpeerinfo for node IDs)")
            .ExampleCli("\"192.168.0.6:8333\"")
            .ExampleCli("\"\" 1")
            .ExampleRpc("\"192.168.0.6:8333\"")
            .ExampleRpc("\"\", 1")
            .AsError();

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
    if (request.fHelp || request.params.size() > 1)
        throw RPCDoc("getaddednodeinfo", "( \"node\" )")
            .Desc(
                "Returns information about the given added node, or all added nodes\n"
                "(note that onetry addnodes are not listed here)")
            .Table("Arguments")
            .Row("1. \"node\"", {"string", "optional"}, "If provided, return information about this specific node, otherwise all nodes are returned.")
            .Table("Result")
            .Row("[")
            .Row("  {")
            .Row("    \"addednode\" : \"192.168.0.201\",", {"string"}, "The node IP address or name (as provided to addnode)")
            .Row("    \"connected\" : true|false,", {"boolean"}, "If connected")
            .Row("    \"addresses\" : [", {"list of objects"}, "Only when connected = true")
            .Row("       {")
            .Row("         \"address\" : \"192.168.0.201:8333\",", {"string"}, "The bitcoin server IP and port we're connected to")
            .Row("         \"connected\" : \"outbound\"", {"string"}, "connection, inbound or outbound")
            .Row("       }")
            .Row("     ]")
            .Row("  }")
            .Row("  ,...")
            .Row("]")
            .ExampleCli("getaddednodeinfo", "\"192.168.0.201\"")
            .ExampleRpc("getaddednodeinfo", "\"192.168.0.201\"")
            .AsError();

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
    if (request.fHelp || request.params.size() > 0)
        throw RPCDoc("getnettotals")
            .Desc(
                "Returns information about network traffic, including bytes in, bytes out,\n"
                "and current time.")
            .Table("Result")
            .Row("{")
            .Row("  \"totalbytesrecv\": n,", {"numeric"}, "Total bytes received")
            .Row("  \"totalbytessent\": n,", {"numeric"}, "Total bytes sent")
            .Row("  \"timemillis\": t,", {"numeric"}, "Current UNIX time in milliseconds")
            .Row("  \"uploadtarget\":")
            .Row("  {")
            .Row("    \"timeframe\": n,", {"numeric"}, "Length of the measuring timeframe in seconds")
            .Row("    \"target\": n,", {"numeric"}, "Target in bytes")
            .Row("    \"target_reached\": true|false,", {"boolean"}, "True if target is reached")
            .Row("    \"serve_historical_blocks\": true|false,", {"boolean"}, "True if serving historical blocks")
            .Row("    \"bytes_left_in_cycle\": t,", {"numeric"}, "Bytes left in current time cycle")
            .Row("    \"time_left_in_cycle\": t", {"numeric"}, "Seconds left in current time cycle")
            .Row("  }")
            .Row("}")
            .ExampleCli("")
            .ExampleRpc("")
            .AsError();

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
        obj.pushKV("limited", IsLimited(network));
        obj.pushKV("reachable", IsReachable(network));
        obj.pushKV("proxy", proxy.IsValid() ? proxy.proxy.ToStringIPPort() : std::string());
        obj.pushKV("proxy_randomize_credentials", proxy.randomize_credentials);
        networks.push_back(obj);
    }
    return networks;
}

static UniValue getnetworkinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw RPCDoc("getnetworkinfo")
            .Desc("Returns an object containing various state info regarding P2P networking.")
            .Table("Result")
            .Row("{")
            .Row("  \"version\": xxxxx,", {"numeric"}, "the server version")
            .Row("  \"subversion\": \"/Satoshi:x.x.x/\",", {"string"}, "the server subversion string")
            .Row("  \"protocolversion\": xxxxx,", {"numeric"}, "the protocol version")
            .Row("  \"localservices\": \"xxxxxxxxxxxxxxxx\",", {"string"}, "the services we offer to the network")
            .Row("  \"localrelay\": true|false,", {"bool"}, "true if transaction relay is requested from peers")
            .Row("  \"timeoffset\": xxxxx,", {"numeric"}, "the time offset")
            .Row("  \"connections\": xxxxx,", {"numeric"}, "the number of connections")
            .Row("  \"networkactive\": true|false,", {"bool"}, "whether p2p networking is enabled")
            .Row("  \"networks\": [", {"array"}, "information per network")
            .Row("  {")
            .Row("    \"name\": \"xxx\",", {"string"}, "network (ipv4, ipv6 or onion)")
            .Row("    \"limited\": true|false,", {"boolean"}, "is the network limited using -onlynet?")
            .Row("    \"reachable\": true|false,", {"boolean"}, "is the network reachable?")
            .Row("    \"proxy\": \"host:port\"", {"string"}, "the proxy that is used for this network, or empty if none")
            .Row("    \"proxy_randomize_credentials\": true|false,", {"string"}, "Whether randomized credentials are used")
            .Row("  }")
            .Row("  ,...")
            .Row("  ],")
            .Row("  \"relayfee\": x.xxxxxxxx,", {"numeric"}, "minimum relay fee for transactions in " + CURRENCY_UNIT + "/kB")
            .Row("  \"incrementalfee\": x.xxxxxxxx,", {"numeric"}, "minimum fee increment for mempool limiting or BIP 125 replacement in " + CURRENCY_UNIT + "/kB")
            .Row("  \"localaddresses\": [", {"array"}, "list of local addresses")
            .Row("  {")
            .Row("    \"address\": \"xxxx\",", {"string"}, "network address")
            .Row("    \"port\": xxx,", {"numeric"}, "network port")
            .Row("    \"score\": xxx", {"numeric"}, "relative score")
            .Row("  }")
            .Row("  ,...")
            .Row("  ]")
            .Row("  \"warnings\": \"...\"", {"string"}, "any network and blockchain warnings")
            .Row("}")
            .ExampleCli("")
            .ExampleRpc("")
            .AsError();

    LOCK(cs_main);
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("version",       CLIENT_VERSION);
    obj.pushKV("subversion",    strSubVersion);
    obj.pushKV("protocolversion",PROTOCOL_VERSION);
    if(g_connman)
        obj.pushKV("localservices", strprintf("%016x", g_connman->GetLocalServices()));
    obj.pushKV("localrelay",     fRelayTxes);
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
    std::string strCommand;
    if (!request.params[1].isNull())
        strCommand = request.params[1].get_str();
    if (request.fHelp || request.params.size() < 2 ||
        (strCommand != "add" && strCommand != "remove"))
        throw RPCDoc("setban", "\"subnet\" \"add|remove\" (bantime) (absolute)")
            .Desc("Attempts to add or remove an IP/Subnet from the banned list.")
            .Table("Arguments")
            .Row("1. \"subnet\"", {"string", "required"},
                "The IP/Subnet (see getpeerinfo for nodes IP) with an optional netmask (default is /32 = single IP)")
            .Row("2. \"command\"", {"string", "required"},
                "'add' to add an IP/Subnet to the list, 'remove' to remove an IP/Subnet from the list")
            .Row("3. \"bantime\"", {"numeric", "optional"},
                "time in seconds how long (or until when if [absolute] is set) the IP is banned (0 or empty means using the default time of 24h which can also be overwritten by the -bantime startup argument)")
            .Row("4. \"absolute\"", {"boolean", "optional"},
                "If set, the bantime must be an absolute timestamp in seconds since epoch (Jan 1 1970 GMT)")
            .ExampleCli("\"192.168.0.6\" \"add\" 86400")
            .ExampleCli("\"192.168.0.0/24\" \"add\"")
            .ExampleRpc("setban", "\"192.168.0.6\", \"add\", 86400")
            .AsError();

    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

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
        if (isSubnet ? g_connman->IsBanned(subNet) : g_connman->IsBanned(netAddr))
            throw JSONRPCError(RPC_CLIENT_NODE_ALREADY_ADDED, "Error: IP/Subnet already banned");

        int64_t banTime = 0; //use standard bantime if not specified
        if (!request.params[2].isNull())
            banTime = request.params[2].get_int64();

        bool absolute = false;
        if (request.params[3].isTrue())
            absolute = true;

        isSubnet ? g_connman->Ban(subNet, BanReasonManuallyAdded, banTime, absolute) : g_connman->Ban(netAddr, BanReasonManuallyAdded, banTime, absolute);
    }
    else if(strCommand == "remove")
    {
        if (!( isSubnet ? g_connman->Unban(subNet) : g_connman->Unban(netAddr) ))
            throw JSONRPCError(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Error: Unban failed. Requested address/subnet was not previously banned.");
    }
    return NullUniValue;
}

static UniValue listbanned(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw RPCDoc("listbanned")
            .Desc("List all banned IPs/Subnets.")
            .ExampleCli("")
            .ExampleRpc("")
            .AsError();

    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    banmap_t banMap;
    g_connman->GetBanned(banMap);

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
    if (request.fHelp || request.params.size() != 0)
        throw RPCDoc("clearbanned")
            .Desc("Clear all banned IPs.")
            .ExampleCli("")
            .ExampleRpc("")
            .AsError();

    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    g_connman->ClearBanned();

    return NullUniValue;
}

static UniValue setnetworkactive(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw RPCDoc("setnetworkactive", "true|false")
            .Desc("Disable/enable all p2p network activity.")
            .Table("Arguments")
            .Row("1. \"state\"", {"boolean", "required"}, "true to enable networking, false to disable")
            .AsError();
    }

    if (!g_connman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    }

    g_connman->SetNetworkActive(request.params[0].get_bool());

    return g_connman->GetNetworkActive();
}

static UniValue getnodeaddresses(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1) {
        throw RPCDoc("getnodeaddresses", "( count )")
            .Desc("Return known addresses which can potentially be used to find new nodes in the network")
            .Table("Arguments")
            .Row("1. \"count\"", {"numeric", "optional"},
                "How many addresses to return. Limited to the smaller of " + std::to_string(ADDRMAN_GETADDR_MAX) +
                    "or " + std::to_string(ADDRMAN_GETADDR_MAX_PCT) + "% of all known addresses. (default = 1)")
            .Table("Result")
            .Row("[")
            .Row("  {")
            .Row("    \"time\": ttt,", {"numeric"}, "Timestamp in seconds since epoch (Jan 1 1970 GMT) keeping track of when the node was last seen")
            .Row("    \"services\": n,", {"numeric"}, "The services offered")
            .Row("    \"address\": \"host\",", {"string"}, "The address of the node")
            .Row("    \"port\": n", {"numeric"}, "The port of the node")
            .Row("  }")
            .Row("  ,....")
            .Row("]")
            .ExampleCli("8")
            .ExampleRpc("8")
            .AsError();
    }
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
