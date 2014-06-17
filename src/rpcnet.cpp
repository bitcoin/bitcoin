// Copyright (c) 2009-2012 Bitcoin Developers
// Copyright (c) 2013-2013 PPCoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "net.h"
#include "bitcoinrpc.h"
#include "alert.h"
#include "base58.h"

using namespace json_spirit;
using namespace std;

Value getconnectioncount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
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
    BOOST_FOREACH(CNode* pnode, vNodes) {
        CNodeStats stats;
        pnode->copyStats(stats);
        vstats.push_back(stats);
    }
}

Value getpeerinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getpeerinfo\n"
            "Returns data about each connected network node.");

    vector<CNodeStats> vstats;
    CopyNodeStats(vstats);

    Array ret;

    BOOST_FOREACH(const CNodeStats& stats, vstats) {
        Object obj;

        obj.push_back(Pair("addr", stats.addrName));
        obj.push_back(Pair("services", strprintf("%08"PRI64x, stats.nServices)));
        obj.push_back(Pair("lastsend", (boost::int64_t)stats.nLastSend));
        obj.push_back(Pair("lastrecv", (boost::int64_t)stats.nLastRecv));
        obj.push_back(Pair("bytessent", (boost::int64_t)stats.nSendBytes));
        obj.push_back(Pair("bytesrecv", (boost::int64_t)stats.nRecvBytes));
        obj.push_back(Pair("conntime", (boost::int64_t)stats.nTimeConnected));
        obj.push_back(Pair("version", stats.nVersion));
        // Use the sanitized form of subver here, to avoid tricksy remote peers from
        // corrupting or modifiying the JSON output by putting special characters in
        // their ver message.
        obj.push_back(Pair("subver", stats.cleanSubVer));
        obj.push_back(Pair("inbound", stats.fInbound));
        obj.push_back(Pair("startingheight", stats.nStartingHeight));
        obj.push_back(Pair("banscore", stats.nMisbehavior));
        if (stats.fSyncNode)
            obj.push_back(Pair("syncnode", true));

        ret.push_back(obj);
    }

    return ret;
}

Value addnode(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() == 2)
        strCommand = params[1].get_str();
    if (fHelp || params.size() != 2 ||
        (strCommand != "onetry" && strCommand != "add" && strCommand != "remove"))
        throw runtime_error(
            "addnode <node> <add|remove|onetry>\n"
            "Attempts add or remove <node> from the addnode list or try a connection to <node> once.");

    string strNode = params[0].get_str();

    if (strCommand == "onetry")
    {
        CAddress addr;
        ConnectNode(addr, strNode.c_str());
        return Value::null;
    }

    LOCK(cs_vAddedNodes);
    vector<string>::iterator it = vAddedNodes.begin();
    for(; it != vAddedNodes.end(); it++)
        if (strNode == *it)
            break;

    if (strCommand == "add")
    {
        if (it != vAddedNodes.end())
            throw JSONRPCError(-23, "Error: Node already added");
        vAddedNodes.push_back(strNode);
    }
    else if(strCommand == "remove")
    {
        if (it == vAddedNodes.end())
            throw JSONRPCError(-24, "Error: Node has not been added.");
        vAddedNodes.erase(it);
    }

    return Value::null;
}

Value getaddednodeinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getaddednodeinfo <dns> [node]\n"
            "Returns information about the given added node, or all added nodes\n"
            "(note that onetry addnodes are not listed here)\n"
            "If dns is false, only a list of added nodes will be provided,\n"
            "otherwise connected information will also be available.");

    bool fDns = params[0].get_bool();

    list<string> laddedNodes(0);
    if (params.size() == 1)
    {
        LOCK(cs_vAddedNodes);
        BOOST_FOREACH(string& strAddNode, vAddedNodes)
            laddedNodes.push_back(strAddNode);
    }
    else
    {
        string strNode = params[1].get_str();
        LOCK(cs_vAddedNodes);
        BOOST_FOREACH(string& strAddNode, vAddedNodes)
            if (strAddNode == strNode)
            {
                laddedNodes.push_back(strAddNode);
                break;
            }
        if (laddedNodes.size() == 0)
            throw JSONRPCError(-24, "Error: Node has not been added.");
    }

    if (!fDns)
    {
        Object ret;
        BOOST_FOREACH(string& strAddNode, laddedNodes)
            ret.push_back(Pair("addednode", strAddNode));
        return ret;
    }

    Array ret;

    list<pair<string, vector<CService> > > laddedAddreses(0);
    BOOST_FOREACH(string& strAddNode, laddedNodes)
    {
        vector<CService> vservNode(0);
        if(Lookup(strAddNode.c_str(), vservNode, GetDefaultPort(), fNameLookup, 0))
            laddedAddreses.push_back(make_pair(strAddNode, vservNode));
        else
        {
            Object obj;
            obj.push_back(Pair("addednode", strAddNode));
            obj.push_back(Pair("connected", false));
            Array addresses;
            obj.push_back(Pair("addresses", addresses));
        }
    }

    LOCK(cs_vNodes);
    for (list<pair<string, vector<CService> > >::iterator it = laddedAddreses.begin(); it != laddedAddreses.end(); it++)
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
                }
            if (!fFound)
                node.push_back(Pair("connected", "false"));
            addresses.push_back(node);
        }
        obj.push_back(Pair("connected", fConnected));
        obj.push_back(Pair("addresses", addresses));
        ret.push_back(obj);
    }

    return ret;
}

// ppcoin: make a public-private key pair
Value makekeypair(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "makekeypair [prefix]\n"
            "Make a public/private key pair.\n"
            "[prefix] is optional preferred prefix for the public key.\n");

    string strPrefix = "";
    if (params.size() > 0)
        strPrefix = params[0].get_str();

    CKey key;
    int nCount = 0;
    do
    {
        key.MakeNewKey(false);
        nCount++;
    } while (nCount < 10000 && strPrefix != HexStr(key.GetPubKey().Raw()).substr(0, strPrefix.size()));

    if (strPrefix != HexStr(key.GetPubKey().Raw()).substr(0, strPrefix.size()))
        return Value::null;

    Object result;
    result.push_back(Pair("PublicKey", HexStr(key.GetPubKey().Raw())));
    bool fCompressed;
    CSecret vchSecret = key.GetSecret(fCompressed);
    result.push_back(Pair("PrivateKey", CBitcoinSecret(vchSecret, fCompressed).ToString()));
    CPrivKey vchPrivKey = key.GetPrivKey();
    result.push_back(Pair("PrivateKeyHex", HexStr<CPrivKey::iterator>(vchPrivKey.begin(), vchPrivKey.end())));
    return result;
}

// ppcoin: display key pair from hex private key
Value showkeypair(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "showkeypair <hexprivkey>\n"
            "Display a public/private key pair with given hex private key.\n"
            "<hexprivkey> is the private key in hex form.\n");

    string strPrivKey = params[0].get_str();

    std::vector<unsigned char> vchPrivKey = ParseHex(strPrivKey);
    CKey key;
    key.SetPrivKey(CPrivKey(vchPrivKey.begin(), vchPrivKey.end())); // if key is not correct openssl may crash

    // Test signing some message
    string strMsg = "Test sign by showkeypair";
    std::vector<unsigned char> vchMsg(strMsg.begin(), strMsg.end());
    std::vector<unsigned char> vchSig;
    if (!key.Sign(Hash(vchMsg.begin(), vchMsg.end()), vchSig))
        throw runtime_error(
            "Failed to sign using the key, bad key?\n");

    Object result;
    result.push_back(Pair("PublicKey", HexStr(key.GetPubKey().Raw())));
    bool fCompressed;
    CSecret vchSecret = key.GetSecret(fCompressed);
    result.push_back(Pair("PrivateKey", CBitcoinSecret(vchSecret, fCompressed).ToString()));
    result.push_back(Pair("PrivateKeyHex", strPrivKey));
    return result;
}

// ppcoin: send alert.  
// There is a known deadlock situation with ThreadMessageHandler
// ThreadMessageHandler: holds cs_vSend and acquiring cs_main in SendMessages()
// ThreadRPCServer: holds cs_main and acquiring cs_vSend in alert.RelayTo()/PushMessage()/BeginMessage()
Value sendalert(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 6)
	throw runtime_error(
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
    alert.vchMsg = vector<unsigned char>(sMsg.begin(), sMsg.end());
    
    vector<unsigned char> vchPrivKey = ParseHex(params[1].get_str());
    key.SetPrivKey(CPrivKey(vchPrivKey.begin(), vchPrivKey.end())); // if key is not correct openssl may crash
    if (!key.Sign(Hash(alert.vchMsg.begin(), alert.vchMsg.end()), alert.vchSig))
        throw runtime_error(
            "Unable to sign alert, check private key?\n");  
    if(!alert.ProcessAlert()) 
        throw runtime_error(
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
