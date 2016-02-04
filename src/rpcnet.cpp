// Copyright (c) 2009-2012 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "net.h"
#include "rpcserver.h"
#include "alert.h"
#include "wallet.h"
#include "db.h"
#include "walletdb.h"

using namespace json_spirit;

Value getconnectioncount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "getconnectioncount\n"
            "Returns the number of connections to other nodes.");

    LOCK(cs_vNodes);
    return (int)vNodes.size();
}

static void CopyNodeStats(std::vector<CNodeStats>& vstats)
{
    vstats.clear();

    LOCK(cs_vNodes);
    vstats.reserve(vNodes.size());
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        CNodeStats stats;
        pnode->copyStats(stats);
        vstats.push_back(stats);
    }
}

Value getpeerinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "getpeerinfo\n"
            "Returns data about each connected network node.");

    std::vector<CNodeStats> vstats;
    CopyNodeStats(vstats);

    Array ret;

    BOOST_FOREACH(const CNodeStats& stats, vstats)
    {
        Object obj;
        obj.push_back(Pair("id", stats.nodeid));
        obj.push_back(Pair("addr", stats.addrName));
        obj.push_back(Pair("services", strprintf("%08x", stats.nServices)));
        obj.push_back(Pair("lastsend", (int64_t)stats.nLastSend));
        obj.push_back(Pair("lastrecv", (int64_t)stats.nLastRecv));
        obj.push_back(Pair("bytessent", stats.nSendBytes));
        obj.push_back(Pair("bytesrecv", stats.nRecvBytes));
        obj.push_back(Pair("conntime", (int64_t)stats.nTimeConnected));
        obj.push_back(Pair("version", stats.nVersion));
        obj.push_back(Pair("subver", stats.strSubVer));
        obj.push_back(Pair("inbound", stats.fInbound));
        obj.push_back(Pair("chainheight", stats.nChainHeight));
        obj.push_back(Pair("banscore", stats.nMisbehavior));
        
        ret.push_back(obj);
    }
    return ret;
}

Value addnode(const Array& params, bool fHelp)
{
    std::string strCommand;
    if (params.size() == 2)
        strCommand = params[1].get_str();
    if (fHelp || params.size() != 2 ||
        (strCommand != "onetry" && strCommand != "add" && strCommand != "remove"))
        throw std::runtime_error(
            "addnode <node> <add|remove|onetry>\n"
            "Attempts add or remove <node> from the addnode list or try a connection to <node> once.");
    
    std::string strNode = params[0].get_str();
    
    if (strCommand == "onetry")
    {
        CAddress addr;
        ConnectNode(addr, strNode.c_str());
        return Value::null;
    };
    
    LOCK(cs_vAddedNodes);
    std::vector<std::string>::iterator it = vAddedNodes.begin();
    for(; it != vAddedNodes.end(); it++)
        if (strNode == *it)
            break;
    
    if (strCommand == "add")
    {
        if (it != vAddedNodes.end())
            throw JSONRPCError(RPC_CLIENT_NODE_ALREADY_ADDED, "Error: Node already added");
        vAddedNodes.push_back(strNode);
    } else
    if(strCommand == "remove")
    {
        if (it == vAddedNodes.end())
            throw JSONRPCError(RPC_CLIENT_NODE_NOT_ADDED, "Error: Node has not been added.");
        vAddedNodes.erase(it);
    };
    
    return Value::null;
}

Value getaddednodeinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "getaddednodeinfo <dns> [node]\n"
            "Returns information about the given added node, or all added nodes\n"
            "(note that onetry addnodes are not listed here)\n"
            "If dns is false, only a list of added nodes will be provided,\n"
            "otherwise connected information will also be available.");
    
    bool fDns = params[0].get_bool();
    
    std::list<std::string> laddedNodes(0);
    if (params.size() == 1)
    {
        LOCK(cs_vAddedNodes);
        BOOST_FOREACH(std::string& strAddNode, vAddedNodes)
            laddedNodes.push_back(strAddNode);
    } else
    {
        std::string strNode = params[1].get_str();
        LOCK(cs_vAddedNodes);
        BOOST_FOREACH(std::string& strAddNode, vAddedNodes)
            if (strAddNode == strNode)
            {
                laddedNodes.push_back(strAddNode);
                break;
            };
        if (laddedNodes.size() == 0)
            throw JSONRPCError(RPC_CLIENT_NODE_NOT_ADDED, "Error: Node has not been added.");
    };
    
    if (!fDns)
    {
        Object ret;
        BOOST_FOREACH(std::string& strAddNode, laddedNodes)
            ret.push_back(Pair("addednode", strAddNode));
        return ret;
    };
    
    Array ret;
    
    std::list<std::pair<std::string, std::vector<CService> > > laddedAddreses(0);
    BOOST_FOREACH(std::string& strAddNode, laddedNodes)
    {
        std::vector<CService> vservNode(0);
        if (Lookup(strAddNode.c_str(), vservNode, Params().GetDefaultPort(), fNameLookup, 0))
            laddedAddreses.push_back(make_pair(strAddNode, vservNode));
        else
        {
            Object obj;
            obj.push_back(Pair("addednode", strAddNode));
            obj.push_back(Pair("connected", false));
            Array addresses;
            obj.push_back(Pair("addresses", addresses));
        };
    };
    
    LOCK(cs_vNodes);
    for (std::list<std::pair<std::string, std::vector<CService> > >::iterator it = laddedAddreses.begin(); it != laddedAddreses.end(); it++)
    {
        Object obj;
        obj.push_back(Pair("addednode", it->first));

        Array addresses;
        bool fConnected = false;
        BOOST_FOREACH(CService& addrNode, it->second)
        {
            bool fFound = false;
            Object node;
            node.push_back(Pair("address", addrNode.ToString()));
            BOOST_FOREACH(CNode* pnode, vNodes)
                if (pnode->addr == addrNode)
                {
                    fFound = true;
                    fConnected = true;
                    node.push_back(Pair("connected", pnode->fInbound ? "inbound" : "outbound"));
                    break;
                };
            if (!fFound)
                node.push_back(Pair("connected", "false"));
            addresses.push_back(node);
        };
        obj.push_back(Pair("connected", fConnected));
        obj.push_back(Pair("addresses", addresses));
        ret.push_back(obj);
    };
    
    return ret;
}
 
// ppcoin: send alert.  
// There is a known deadlock situation with ThreadMessageHandler
// ThreadMessageHandler: holds cs_vSend and acquiring cs_main in SendMessages()
// ThreadRPCServer: holds cs_main and acquiring cs_vSend in alert.RelayTo()/PushMessage()/BeginMessage()
Value sendalert(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 6)
        throw std::runtime_error(
            "sendalert <message> <privatekey> <minver> <maxver> <priority> <id> [cancelupto]\n"
            "<message> is the alert text message\n"
            "<privatekey> is hex string of alert master private key\n"
            "<minver> is the minimum applicable internal client version\n"
            "<maxver> is the maximum applicable internal client version\n"
            "<priority> is integer priority number\n"
            "<id> is the alert id\n"
            "[cancelupto] cancels all alert id's up to this number\n"
            "Returns true or false.");

    CAlert alert;
    CKey key;

    alert.strStatusBar = params[0].get_str();
    alert.nMinVer = params[2].get_int();
    alert.nMaxVer = params[3].get_int();
    alert.nPriority = params[4].get_int();
    alert.nID = params[5].get_int();
    if (params.size() > 6)
        alert.nCancel = params[6].get_int();
    alert.nVersion = PROTOCOL_VERSION;
    alert.nRelayUntil = GetAdjustedTime() + 365*24*60*60;
    alert.nExpiration = GetAdjustedTime() + 365*24*60*60;

    CDataStream sMsg(SER_NETWORK, PROTOCOL_VERSION);
    sMsg << (CUnsignedAlert)alert;
    alert.vchMsg = std::vector<unsigned char>(sMsg.begin(), sMsg.end());

    std::vector<unsigned char> vchPrivKey = ParseHex(params[1].get_str());
    key.SetPrivKey(CPrivKey(vchPrivKey.begin(), vchPrivKey.end()), false); // if key is not correct openssl may crash
    if (!key.Sign(Hash(alert.vchMsg.begin(), alert.vchMsg.end()), alert.vchSig))
        throw std::runtime_error(
            "Unable to sign alert, check private key?\n");  
    if (!alert.ProcessAlert()) 
        throw std::runtime_error(
            "Failed to process alert.\n");
    // Relay alert
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            alert.RelayTo(pnode);
    }

    Object result;
    result.push_back(Pair("strStatusBar", alert.strStatusBar));
    result.push_back(Pair("nVersion", alert.nVersion));
    result.push_back(Pair("nMinVer", alert.nMinVer));
    result.push_back(Pair("nMaxVer", alert.nMaxVer));
    result.push_back(Pair("nPriority", alert.nPriority));
    result.push_back(Pair("nID", alert.nID));
    if (alert.nCancel > 0)
        result.push_back(Pair("nCancel", alert.nCancel));
    return result;
}


static Array GetNetworksInfo()
{
    Array networks;
    for(int n=0; n<NET_MAX; ++n)
    {
        enum Network network = static_cast<enum Network>(n);
        if(network == NET_UNROUTABLE)
            continue;
        proxyType proxy;
        Object obj;
        GetProxy(network, proxy);
        obj.push_back(Pair("name", GetNetworkName(network)));
        obj.push_back(Pair("limited", IsLimited(network)));
        obj.push_back(Pair("reachable", IsReachable(network)));
        obj.push_back(Pair("proxy", proxy.IsValid() ? proxy.ToStringIPPort() : std::string()));
        networks.push_back(obj);
    }
    return networks;
}

Value getnetworkinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "getnetworkinfo\n"
            "Returns an object containing various state info regarding P2P networking.\n"
            "\nResult:\n"
            "{\n"
            "  \"version\": xxxxx,                      (numeric) the server version\n"
            "  \"subversion\": \"/Satoshi:x.x.x/\",     (string) the server subversion string\n"
            "  \"protocolversion\": xxxxx,              (numeric) the protocol version\n"
            "  \"localservices\": \"xxxxxxxxxxxxxxxx\", (string) the services we offer to the network\n"
            "  \"timeoffset\": xxxxx,                   (numeric) the time offset\n"
            "  \"connections\": xxxxx,                  (numeric) the number of connections\n"
            "  \"networks\": [                          (array) information per network\n"
            "  {\n"
            "    \"name\": \"xxx\",                     (string) network (ipv4, ipv6 or onion)\n"
            "    \"limited\": true|false,               (boolean) is the network limited using -onlynet?\n"
            "    \"reachable\": true|false,             (boolean) is the network reachable?\n"
            "    \"proxy\": \"host:port\"               (string) the proxy that is used for this network, or empty if none\n"
            "  }\n"
            "  ,...\n"
            "  ],\n"
            "  \"relayfee\": x.xxxxxxxx,                (numeric) minimum relay fee for non-free transactions in gameunits/kb\n"
            "  \"localaddresses\": [                    (array) list of local addresses\n"
            "  {\n"
            "    \"address\": \"xxxx\",                 (string) network address\n"
            "    \"port\": xxx,                         (numeric) network port\n"
            "    \"score\": xxx                         (numeric) relative score\n"
            "  }\n"
            "  ,...\n"
            "  ]\n"
            "}\n"
        );

    LOCK(cs_main);

    Object obj;
    obj.push_back(Pair("version",       CLIENT_VERSION));
    obj.push_back(Pair("subversion",
        FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, std::vector<std::string>())));
    obj.push_back(Pair("protocolversion",PROTOCOL_VERSION));
    obj.push_back(Pair("localservices",       strprintf("%016x", nLocalServices)));
    obj.push_back(Pair("timeoffset",    GetTimeOffset()));
    obj.push_back(Pair("connections",   (int)vNodes.size()));
    obj.push_back(Pair("networks",      GetNetworksInfo()));
    //obj.push_back(Pair("relayfee",      ValueFromAmount(::minRelayTxFee.GetFeePerK())));
    Array localAddresses;
    {
        LOCK(cs_mapLocalHost);
        BOOST_FOREACH(const PAIRTYPE(CNetAddr, LocalServiceInfo) &item, mapLocalHost)
        {
            Object rec;
            rec.push_back(Pair("address", item.first.ToString()));
            rec.push_back(Pair("port", item.second.nPort));
            rec.push_back(Pair("score", item.second.nScore));
            localAddresses.push_back(rec);
        }
    }
    obj.push_back(Pair("localaddresses", localAddresses));
    return obj;
}


