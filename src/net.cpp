// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "bitcredit-config.h"
#endif

#include "net.h"

#include "addrman.h"
#include "chainparams.h"
#include "core.h"
#include "ui_interface.h"
#include "main.h"

#ifdef WIN32
#include <string.h>
#else
#include <fcntl.h>
#endif

#ifdef USE_UPNP
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/miniwget.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
#endif

#include <boost/filesystem.hpp>

// Dump addresses to xxx_peers.dat every 15 minutes (900s)
#define DUMP_ADDRESSES_INTERVAL 900

#if !defined(HAVE_MSG_NOSIGNAL) && !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

using namespace std;
using namespace boost;

static const int MAX_OUTBOUND_CONNECTIONS = 8;

bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant* semOutbound, const char *strDest, bool fOneShot, CNetParams * netParams);


class CBitcreditNetParams : public CNetParams {
public:
	const CChainParams& Params() {
		return Bitcredit_Params();
	}
	const std::string LogPrefix() {
		return "Bitcredit:";
	}
	const char * DebugCategory() {
		return "bitcredit_net";
	}
	const std::string ClientName() {
		return BITCREDIT_CLIENT_NAME;
	}
	const int& ProtocolVersion() {
		return BITCREDIT_PROTOCOL_VERSION;
	}
	unsigned short GetListenPort() {
		return (unsigned short)(GetArg("-port", Params().GetDefaultPort()));
	}
	void RegisterNodeSignals() {
		Bitcredit_RegisterNodeSignals(g_signals);
	}
	void UnregisterNodeSignals() {
		Bitcredit_UnregisterNodeSignals(g_signals);
	}
};
static CBitcreditNetParams bitcredit_NetParams;

class CBitcoinNetParams : public CNetParams {
public:
	const CChainParams& Params() {
		return Bitcoin_Params();
	}
	const std::string LogPrefix() {
		return "Bitcoin:";
	}
	const char * DebugCategory() {
		return "bitcoin_net";
	}
	const std::string ClientName() {
		return BITCOIN_CLIENT_NAME;
	}
	const int& ProtocolVersion() {
		return BITCOIN_PROTOCOL_VERSION;
	}
	unsigned short GetListenPort() {
		return (unsigned short)(GetArg("-bitcoin_port", Params().GetDefaultPort()));
	}
	void RegisterNodeSignals() {
		Bitcoin_RegisterNodeSignals(g_signals);
	}
	void UnregisterNodeSignals() {
		Bitcoin_UnregisterNodeSignals(g_signals);
	}
};
static CBitcoinNetParams bitcoin_NetParams;

CNetParams * Bitcredit_NetParams() {
	return &bitcredit_NetParams;
}
CNetParams * Bitcoin_NetParams() {
	return &bitcoin_NetParams;
}

void AddOneShot(string strDest, CNetParams * netParams)
{
    LOCK(netParams->cs_vOneShots);
    netParams->vOneShots.push_back(strDest);
}

// find 'best' local address for a particular peer
bool GetLocal(CService& addr, const CNetAddr *paddrPeer, CNetParams * netParams)
{
    if (!netParams->fListen)
        return false;

    int nBestScore = -1;
    int nBestReachability = -1;
    {
        LOCK(netParams->cs_mapLocalHost);
        for (map<CNetAddr, LocalServiceInfo>::iterator it = netParams->mapLocalHost.begin(); it != netParams->mapLocalHost.end(); it++)
        {
            int nScore = (*it).second.nScore;
            int nReachability = (*it).first.GetReachabilityFrom(paddrPeer);
            if (nReachability > nBestReachability || (nReachability == nBestReachability && nScore > nBestScore))
            {
                addr = CService((*it).first, (*it).second.nPort);
                nBestReachability = nReachability;
                nBestScore = nScore;
            }
        }
    }
    return nBestScore >= 0;
}

// get best local address for a particular peer as a CAddress
CAddress GetLocalAddress(const CService *paddrPeer, CNetParams * netParams)
{
    CAddress ret(CService("0.0.0.0",0),0);
    CService addr;
    if (GetLocal(addr, paddrPeer, netParams))
    {
        ret = CAddress(addr);
        ret.nServices = netParams->nLocalServices;
        ret.nTime = GetAdjustedTime();
    }
    return ret;
}

bool RecvLine(SOCKET hSocket, string& strLine, CNetParams * netParams)
{
    strLine = "";
    while (true)
    {
        char c;
        int nBytes = recv(hSocket, &c, 1, 0);
        if (nBytes > 0)
        {
            if (c == '\n')
                continue;
            if (c == '\r')
                return true;
            strLine += c;
            if (strLine.size() >= 9000)
                return true;
        }
        else if (nBytes <= 0)
        {
            boost::this_thread::interruption_point();
            if (nBytes < 0)
            {
                int nErr = WSAGetLastError();
                if (nErr == WSAEMSGSIZE)
                    continue;
                if (nErr == WSAEWOULDBLOCK || nErr == WSAEINTR || nErr == WSAEINPROGRESS)
                {
                    MilliSleep(10);
                    continue;
                }
            }
            if (!strLine.empty())
                return true;
            if (nBytes == 0)
            {
                // socket closed
                LogPrint(netParams->DebugCategory(), "%s socket closed\n", netParams->LogPrefix());
                return false;
            }
            else
            {
                // socket error
                int nErr = WSAGetLastError();
                LogPrint(netParams->DebugCategory(), "%s recv failed: %s\n", netParams->LogPrefix(), NetworkErrorString(nErr));
                return false;
            }
        }
    }
}

// used when scores of local addresses may have changed
// pushes better local address to peers
void static AdvertizeLocal(CNetParams * netParams)
{
    LOCK(netParams->cs_vNodes);
    BOOST_FOREACH(CNode* pnode, netParams->vNodes)
    {
        if (pnode->fSuccessfullyConnected)
        {
            CAddress addrLocal = GetLocalAddress(&pnode->addr, netParams);
            if (addrLocal.IsRoutable() && (CService)addrLocal != (CService)pnode->addrLocal)
            {
                pnode->PushAddress(addrLocal);
                pnode->addrLocal = addrLocal;
            }
        }
    }
}

void SetReachable(enum Network net, bool fFlag, CNetParams * netParams)
{
    LOCK(netParams->cs_mapLocalHost);
    netParams->vfReachable[net] = fFlag;
    if (net == NET_IPV6 && fFlag)
    	netParams->vfReachable[NET_IPV4] = true;
}

// learn a new local address
bool AddLocal(const CService& addr, int nScore, CNetParams * netParams)
{
    if (!addr.IsRoutable())
        return false;

    if (!netParams->fDiscover && nScore < LOCAL_MANUAL)
        return false;

    if (IsLimited(addr, netParams))
        return false;

    LogPrintf("AddLocal(%s,%i)\n", addr.ToString(), nScore);

    {
        LOCK(netParams->cs_mapLocalHost);
        bool fAlready = netParams->mapLocalHost.count(addr) > 0;
        LocalServiceInfo &info = netParams->mapLocalHost[addr];
        if (!fAlready || nScore >= info.nScore) {
            info.nScore = nScore + (fAlready ? 1 : 0);
            info.nPort = addr.GetPort();
        }
        SetReachable(addr.GetNetwork(), true, netParams);
    }

    AdvertizeLocal(netParams);

    return true;
}

bool AddLocal(const CNetAddr &addr, int nScore, CNetParams * netParams)
{
    return AddLocal(CService(addr, netParams->GetListenPort()), nScore, netParams);
}

/** Make a particular network entirely off-limits (no automatic connects to it) */
void SetLimited(enum Network net, bool fLimited, CNetParams * netParams)
{
    if (net == NET_UNROUTABLE)
        return;
    LOCK(netParams->cs_mapLocalHost);
    netParams->vfLimited[net] = fLimited;
}

bool IsLimited(enum Network net, CNetParams * netParams)
{
    LOCK(netParams->cs_mapLocalHost);
    return netParams->vfLimited[net];
}

bool IsLimited(const CService &addr, CNetParams * netParams)
{
    return IsLimited(addr.GetNetwork(), netParams);
}

/** vote for a local address */
bool SeenLocal(const CService& addr, CNetParams * netParams)
{
    {
        LOCK(netParams->cs_mapLocalHost);
        if (netParams->mapLocalHost.count(addr) == 0)
            return false;
        netParams->mapLocalHost[addr].nScore++;
    }

    AdvertizeLocal(netParams);

    return true;
}

/** check whether a given address is potentially local */
bool IsLocal(const CService& addr, CNetParams * netParams)
{
    LOCK(netParams->cs_mapLocalHost);
    return netParams->mapLocalHost.count(addr) > 0;
}

/** check whether a given address is in a network we can probably connect to */
bool IsReachable(const CService& addr, CNetParams * netParams)
{
    LOCK(netParams->cs_mapLocalHost);
    enum Network net = addr.GetNetwork();
    return netParams->vfReachable[net] && !netParams->vfLimited[net];
}

bool GetMyExternalIP2(const CService& addrConnect, const char* pszGet, const char* pszKeyword, CNetAddr& ipRet, CNetParams * netParams)
{
    SOCKET hSocket;
    if (!ConnectSocket(addrConnect, hSocket, netParams->LogPrefix(), netParams->DebugCategory()))
        return error("%s GetMyExternalIP() : connection to %s failed", netParams->LogPrefix(), addrConnect.ToString());

    send(hSocket, pszGet, strlen(pszGet), MSG_NOSIGNAL);

    string strLine;
    while (RecvLine(hSocket, strLine, netParams))
    {
        if (strLine.empty()) // HTTP response is separated from headers by blank line
        {
            while (true)
            {
                if (!RecvLine(hSocket, strLine, netParams))
                {
                    closesocket(hSocket);
                    return false;
                }
                if (pszKeyword == NULL)
                    break;
                if (strLine.find(pszKeyword) != string::npos)
                {
                    strLine = strLine.substr(strLine.find(pszKeyword) + strlen(pszKeyword));
                    break;
                }
            }
            closesocket(hSocket);
            if (strLine.find("<") != string::npos)
                strLine = strLine.substr(0, strLine.find("<"));
            strLine = strLine.substr(strspn(strLine.c_str(), " \t\n\r"));
            while (strLine.size() > 0 && isspace(strLine[strLine.size()-1]))
                strLine.resize(strLine.size()-1);
            CService addr(strLine,0,true);
            LogPrintf("GetMyExternalIP() received [%s] %s\n", strLine, addr.ToString());
            if (!addr.IsValid() || !addr.IsRoutable())
                return false;
            ipRet.SetIP(addr);
            return true;
        }
    }
    closesocket(hSocket);
    return error("GetMyExternalIP() : connection closed");
}

bool GetMyExternalIP(CNetAddr& ipRet, CNetParams * netParams)
{
    CService addrConnect;
    const char* pszGet;
    const char* pszKeyword;

    for (int nLookup = 0; nLookup <= 1; nLookup++)
    for (int nHost = 1; nHost <= 2; nHost++)
    {
        // We should be phasing out our use of sites like these. If we need
        // replacements, we should ask for volunteers to put this simple
        // php file on their web server that prints the client IP:
        //  <?php echo $_SERVER["REMOTE_ADDR"]; ?>
        if (nHost == 1)
        {
            addrConnect = CService("91.198.22.70", 80); // checkip.dyndns.org

            if (nLookup == 1)
            {
                CService addrIP("checkip.dyndns.org", 80, true);
                if (addrIP.IsValid())
                    addrConnect = addrIP;
            }

            pszGet = "GET / HTTP/1.1\r\n"
                     "Host: checkip.dyndns.org\r\n"
                     "User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)\r\n"
                     "Connection: close\r\n"
                     "\r\n";

            pszKeyword = "Address:";
        }
        else if (nHost == 2)
        {
            addrConnect = CService("74.208.43.192", 80); // www.showmyip.com

            if (nLookup == 1)
            {
                CService addrIP("www.showmyip.com", 80, true);
                if (addrIP.IsValid())
                    addrConnect = addrIP;
            }

            pszGet = "GET /simple/ HTTP/1.1\r\n"
                     "Host: www.showmyip.com\r\n"
                     "User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)\r\n"
                     "Connection: close\r\n"
                     "\r\n";

            pszKeyword = NULL; // Returns just IP address
        }

        if (GetMyExternalIP2(addrConnect, pszGet, pszKeyword, ipRet, netParams))
            return true;
    }

    return false;
}

void GetMyExternalIP(CNetParams * netParams)
{
    CNetAddr addrLocalHost;
    if (GetMyExternalIP(addrLocalHost, netParams))
    {
        LogPrintf("GetMyExternalIP() returned %s\n", addrLocalHost.ToStringIP());
        AddLocal(addrLocalHost, LOCAL_HTTP, netParams);
    }
}
void Bitcredit_ThreadGetMyExternalIP() {
	GetMyExternalIP(Bitcredit_NetParams());
}
void Bitcoin_ThreadGetMyExternalIP() {
	GetMyExternalIP(Bitcoin_NetParams());
}



uint64_t CNode::bitcredit_nTotalBytesRecv = 0;
uint64_t CNode::bitcredit_nTotalBytesSent = 0;
CCriticalSection CNode::bitcredit_cs_totalBytesRecv;
CCriticalSection CNode::bitcredit_cs_totalBytesSent;
uint64_t CNode::bitcoin_nTotalBytesRecv = 0;
uint64_t CNode::bitcoin_nTotalBytesSent = 0;
CCriticalSection CNode::bitcoin_cs_totalBytesRecv;
CCriticalSection CNode::bitcoin_cs_totalBytesSent;

CNode* FindNode(const CNetAddr& ip, CNetParams * netParams)
{
    LOCK(netParams->cs_vNodes);
    BOOST_FOREACH(CNode* pnode, netParams->vNodes)
        if ((CNetAddr)pnode->addr == ip)
            return (pnode);
    return NULL;
}

CNode* FindNode(std::string addrName, CNetParams * netParams)
{
    LOCK(netParams->cs_vNodes);
    BOOST_FOREACH(CNode* pnode, netParams->vNodes)
        if (pnode->addrName == addrName)
            return (pnode);
    return NULL;
}

CNode* FindNode(const CService& addr, CNetParams * netParams)
{
    LOCK(netParams->cs_vNodes);
    BOOST_FOREACH(CNode* pnode, netParams->vNodes)
        if ((CService)pnode->addr == addr)
            return (pnode);
    return NULL;
}

CNode* ConnectNode(CAddress addrConnect, const char *pszDest, CNetParams * netParams)
{
    const int port = netParams->GetDefaultPort();

    if (pszDest == NULL) {
        if (IsLocal(addrConnect, netParams))
            return NULL;

        // Look for an existing connection
        CNode* pnode = FindNode((CService)addrConnect, netParams);
        if (pnode)
        {
            pnode->AddRef();
            return pnode;
        }
    }


    /// debug print
    LogPrint(netParams->DebugCategory(), "%s trying connection %s lastseen=%.1fhrs\n",
    	netParams->LogPrefix(),
        pszDest ? pszDest : addrConnect.ToString(),
        pszDest ? 0 : (double)(GetAdjustedTime() - addrConnect.nTime)/3600.0);

    // Connect
    SOCKET hSocket;
    if (pszDest ? ConnectSocketByName(addrConnect, hSocket, pszDest, netParams->LogPrefix(), netParams->DebugCategory(), port) : ConnectSocket(addrConnect, hSocket, netParams->LogPrefix(), netParams->DebugCategory()))
    {
    	netParams->addrman.Attempt(addrConnect);

        LogPrint(netParams->DebugCategory(), "%s connected %s\n", netParams->LogPrefix(), pszDest ? pszDest : addrConnect.ToString());

        // Set to non-blocking
#ifdef WIN32
        u_long nOne = 1;
        if (ioctlsocket(hSocket, FIONBIO, &nOne) == SOCKET_ERROR)
            LogPrintf("%s ConnectSocket() : ioctlsocket non-blocking setting failed, error %s\n", netParams->LogPrefix(), NetworkErrorString(WSAGetLastError()));
#else
        if (fcntl(hSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
            LogPrintf("%s ConnectSocket() : fcntl non-blocking setting failed, error %s\n", netParams->LogPrefix(), NetworkErrorString(errno));
#endif

        // Add node
        CNode* pnode = new CNode(hSocket, addrConnect, pszDest ? pszDest : "", false, netParams);
        pnode->AddRef();

        {
            LOCK(netParams->cs_vNodes);
            netParams->vNodes.push_back(pnode);
        }

        pnode->nTimeConnected = GetTime();
        return pnode;
    }
    else
    {
        return NULL;
    }
}

void CNode::CloseSocketDisconnect()
{
    fDisconnect = true;
    if (hSocket != INVALID_SOCKET)
    {
        LogPrint(netParams->DebugCategory(), "%s disconnecting node %s\n", netParams->LogPrefix(), addrName);
        closesocket(hSocket);
        hSocket = INVALID_SOCKET;
    }

    // in case this fails, we'll empty the recv buffer when the CNode is deleted
    TRY_LOCK(cs_vRecvMsg, lockRecv);
    if (lockRecv)
        vRecvMsg.clear();

    // if this was the sync node, we'll need a new one
    if (this == netParams->pnodeSync)
    	netParams->pnodeSync = NULL;
}

void CNode::Cleanup()
{
}


void CNode::PushVersion()
{
    int nBestHeight = netParams->GetNodeSignals().GetHeight().get_value_or(0);

    /// when NTP implemented, change to just nTime = GetAdjustedTime()
    int64_t nTime = (fInbound ? GetAdjustedTime() : GetTime());
    CAddress addrYou = (addr.IsRoutable() && !IsProxy(addr) ? addr : CAddress(CService("0.0.0.0",0)));
    CAddress addrMe = GetLocalAddress(&addr, netParams);
    RAND_bytes((unsigned char*)&(netParams->nLocalHostNonce), sizeof(netParams->nLocalHostNonce));
    LogPrint(netParams->DebugCategory(), "%s send version message: version %d, blocks=%d, us=%s, them=%s, peer=%s\n", netParams->LogPrefix(), netParams->ProtocolVersion(), nBestHeight, addrMe.ToString(), addrYou.ToString(), addr.ToString());
    PushMessage("version", netParams->ProtocolVersion(), netParams->nLocalServices, nTime, addrYou, addrMe,
    		netParams->nLocalHostNonce, FormatSubVersion(netParams->ClientName(), netParams->ClientVersion(), std::vector<string>()), nBestHeight, true);
}





std::map<CNetAddr, int64_t> CNode::setBanned;
CCriticalSection CNode::cs_setBanned;

void CNode::ClearBanned()
{
    setBanned.clear();
}

bool CNode::IsBanned(CNetAddr ip)
{
    bool fResult = false;
    {
        LOCK(cs_setBanned);
        std::map<CNetAddr, int64_t>::iterator i = setBanned.find(ip);
        if (i != setBanned.end())
        {
            int64_t t = (*i).second;
            if (GetTime() < t)
                fResult = true;
        }
    }
    return fResult;
}

bool CNode::Ban(const CNetAddr &addr) {
    int64_t banTime = GetTime()+GetArg("-bantime", 60*60*24);  // Default 24-hour ban
    {
        LOCK(cs_setBanned);
        if (setBanned[addr] < banTime)
            setBanned[addr] = banTime;
    }
    return true;
}

#undef X
#define X(name) stats.name = name
void CNode::copyStats(CNodeStats &stats)
{
    stats.nodeid = this->GetId();
    X(nServices);
    X(nLastSend);
    X(nLastRecv);
    X(nTimeConnected);
    X(addrName);
    X(nVersion);
    X(cleanSubVer);
    X(fInbound);
    X(nStartingHeight);
    X(nSendBytes);
    X(nRecvBytes);
    stats.fSyncNode = (this == netParams->pnodeSync);

    // It is common for nodes with good ping times to suddenly become lagged,
    // due to a new block arriving or other large transfer.
    // Merely reporting pingtime might fool the caller into thinking the node was still responsive,
    // since pingtime does not update until the ping is complete, which might take a while.
    // So, if a ping is taking an unusually long time in flight,
    // the caller can immediately detect that this is happening.
    int64_t nPingUsecWait = 0;
    if ((0 != nPingNonceSent) && (0 != nPingUsecStart)) {
        nPingUsecWait = GetTimeMicros() - nPingUsecStart;
    }

    // Raw ping time is in microseconds, but show it to user as whole seconds (Bitcoin users should be well used to small numbers with many decimal places by now :)
    stats.dPingTime = (((double)nPingUsecTime) / 1e6);
    stats.dPingWait = (((double)nPingUsecWait) / 1e6);

    // Leave string empty if addrLocal invalid (not filled in yet)
    stats.addrLocal = addrLocal.IsValid() ? addrLocal.ToString() : "";
}
#undef X

// requires LOCK(cs_vRecvMsg)
bool CNode::ReceiveMsgBytes(const char *pch, unsigned int nBytes)
{
    while (nBytes > 0) {

        // get current incomplete message, or create a new one
        if (vRecvMsg.empty() ||
            vRecvMsg.back().complete())
            vRecvMsg.push_back(CNetMessage(SER_NETWORK, nRecvVersion));

        CNetMessage& msg = vRecvMsg.back();

        // absorb network data
        int handled;
        if (!msg.in_data)
            handled = msg.readHeader(pch, nBytes);
        else
            handled = msg.readData(pch, nBytes);

        if (handled < 0)
                return false;

        pch += handled;
        nBytes -= handled;
    }

    return true;
}

int CNetMessage::readHeader(const char *pch, unsigned int nBytes)
{
    // copy data to temporary parsing buffer
    unsigned int nRemaining = 24 - nHdrPos;
    unsigned int nCopy = std::min(nRemaining, nBytes);

    memcpy(&hdrbuf[nHdrPos], pch, nCopy);
    nHdrPos += nCopy;

    // if header incomplete, exit
    if (nHdrPos < 24)
        return nCopy;

    // deserialize to CMessageHeader
    try {
        hdrbuf >> hdr;
    }
    catch (std::exception &e) {
        return -1;
    }

    // reject messages larger than MAX_SIZE
    if (hdr.nMessageSize > MAX_SIZE)
            return -1;

    // switch state to reading message data
    in_data = true;
    vRecv.resize(hdr.nMessageSize);

    return nCopy;
}

int CNetMessage::readData(const char *pch, unsigned int nBytes)
{
    unsigned int nRemaining = hdr.nMessageSize - nDataPos;
    unsigned int nCopy = std::min(nRemaining, nBytes);

    memcpy(&vRecv[nDataPos], pch, nCopy);
    nDataPos += nCopy;

    return nCopy;
}









// requires LOCK(cs_vSend)
void SocketSendData(CNode *pnode)
{
    std::deque<CSerializeData>::iterator it = pnode->vSendMsg.begin();

    while (it != pnode->vSendMsg.end()) {
        const CSerializeData &data = *it;
        assert(data.size() > pnode->nSendOffset);
        int nBytes = send(pnode->hSocket, &data[pnode->nSendOffset], data.size() - pnode->nSendOffset, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (nBytes > 0) {
            pnode->nLastSend = GetTime();
            pnode->nSendBytes += nBytes;
            pnode->nSendOffset += nBytes;
            if(pnode->netParams->MessageStart() == Bitcredit_Params().MessageStart()) {
				pnode->bitcredit_RecordBytesSent(nBytes);
            } else {
				pnode->bitcoin_RecordBytesSent(nBytes);
            }
            if (pnode->nSendOffset == data.size()) {
                pnode->nSendOffset = 0;
                pnode->nSendSize -= data.size();
                it++;
            } else {
                // could not send full message; stop sending more
                break;
            }
        } else {
            if (nBytes < 0) {
                // error
                int nErr = WSAGetLastError();
                if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
                {
                    LogPrintf("socket send error %s\n", NetworkErrorString(nErr));
                    pnode->CloseSocketDisconnect();
                }
            }
            // couldn't send anything at all
            break;
        }
    }

    if (it == pnode->vSendMsg.end()) {
        assert(pnode->nSendOffset == 0);
        assert(pnode->nSendSize == 0);
    }
    pnode->vSendMsg.erase(pnode->vSendMsg.begin(), it);
}

void SocketHandler(CNetParams * netParams)
{
    while (true)
    {
        //
        // Disconnect nodes
        //
        {
            LOCK(netParams->cs_vNodes);
            // Disconnect unused nodes
            vector<CNode*> vNodesCopy = netParams->vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
            {
                if (pnode->fDisconnect ||
                    (pnode->GetRefCount() <= 0 && pnode->vRecvMsg.empty() && pnode->nSendSize == 0 && pnode->ssSend.empty()))
                {
                    // remove from vNodes
                	netParams->vNodes.erase(remove(netParams->vNodes.begin(), netParams->vNodes.end(), pnode), netParams->vNodes.end());

                    // release outbound grant (if any)
                    pnode->grantOutbound.Release();

                    // close socket and cleanup
                    pnode->CloseSocketDisconnect();
                    pnode->Cleanup();

                    // hold in disconnected pool until all refs are released
                    if (pnode->fNetworkNode || pnode->fInbound)
                        pnode->Release();
                    netParams->vNodesDisconnected.push_back(pnode);
                }
            }
        }
        {
            // Delete disconnected nodes
            list<CNode*> vNodesDisconnectedCopy = netParams->vNodesDisconnected;
            BOOST_FOREACH(CNode* pnode, vNodesDisconnectedCopy)
            {
                // wait until threads are done using it
                if (pnode->GetRefCount() <= 0)
                {
                    bool fDelete = false;
                    {
                        TRY_LOCK(pnode->cs_vSend, lockSend);
                        if (lockSend)
                        {
                            TRY_LOCK(pnode->cs_vRecvMsg, lockRecv);
                            if (lockRecv)
                            {
                                TRY_LOCK(pnode->cs_inventory, lockInv);
                                if (lockInv)
                                    fDelete = true;
                            }
                        }
                    }
                    if (fDelete)
                    {
                    	netParams->vNodesDisconnected.remove(pnode);
                        delete pnode;
                    }
                }
            }
        }


        //
        // Find which sockets have data to receive
        //
        struct timeval timeout;
        timeout.tv_sec  = 0;
        timeout.tv_usec = 50000; // frequency to poll pnode->vSend

        fd_set fdsetRecv;
        fd_set fdsetSend;
        fd_set fdsetError;
        FD_ZERO(&fdsetRecv);
        FD_ZERO(&fdsetSend);
        FD_ZERO(&fdsetError);
        SOCKET hSocketMax = 0;
        bool have_fds = false;

        BOOST_FOREACH(SOCKET hListenSocket, netParams->vhListenSocket) {
            FD_SET(hListenSocket, &fdsetRecv);
            hSocketMax = max(hSocketMax, hListenSocket);
            have_fds = true;
        }
        {
            LOCK(netParams->cs_vNodes);
            BOOST_FOREACH(CNode* pnode, netParams->vNodes)
            {
                if (pnode->hSocket == INVALID_SOCKET)
                    continue;
                FD_SET(pnode->hSocket, &fdsetError);
                hSocketMax = max(hSocketMax, pnode->hSocket);
                have_fds = true;

                // Implement the following logic:
                // * If there is data to send, select() for sending data. As this only
                //   happens when optimistic write failed, we choose to first drain the
                //   write buffer in this case before receiving more. This avoids
                //   needlessly queueing received data, if the remote peer is not themselves
                //   receiving data. This means properly utilizing TCP flow control signalling.
                // * Otherwise, if there is no (complete) message in the receive buffer,
                //   or there is space left in the buffer, select() for receiving data.
                // * (if neither of the above applies, there is certainly one message
                //   in the receiver buffer ready to be processed).
                // Together, that means that at least one of the following is always possible,
                // so we don't deadlock:
                // * We send some data.
                // * We wait for data to be received (and disconnect after timeout).
                // * We process a message in the buffer (message handler thread).
                {
                    TRY_LOCK(pnode->cs_vSend, lockSend);
                    if (lockSend && !pnode->vSendMsg.empty()) {
                        FD_SET(pnode->hSocket, &fdsetSend);
                        continue;
                    }
                }
                {
                    TRY_LOCK(pnode->cs_vRecvMsg, lockRecv);
                    if (lockRecv && (
                        pnode->vRecvMsg.empty() || !pnode->vRecvMsg.front().complete() ||
                        pnode->GetTotalRecvSize() <= ReceiveFloodSize()))
                        FD_SET(pnode->hSocket, &fdsetRecv);
                }
            }
        }

        int nSelect = select(have_fds ? hSocketMax + 1 : 0,
                             &fdsetRecv, &fdsetSend, &fdsetError, &timeout);
        boost::this_thread::interruption_point();

        if (nSelect == SOCKET_ERROR)
        {
            if (have_fds)
            {
                int nErr = WSAGetLastError();
                LogPrintf("socket select error %s\n", NetworkErrorString(nErr));
                for (unsigned int i = 0; i <= hSocketMax; i++)
                    FD_SET(i, &fdsetRecv);
            }
            FD_ZERO(&fdsetSend);
            FD_ZERO(&fdsetError);
            MilliSleep(timeout.tv_usec/1000);
        }


        //
        // Accept new connections
        //
        BOOST_FOREACH(SOCKET hListenSocket, netParams->vhListenSocket)
        if (hListenSocket != INVALID_SOCKET && FD_ISSET(hListenSocket, &fdsetRecv))
        {
            struct sockaddr_storage sockaddr;
            socklen_t len = sizeof(sockaddr);
            SOCKET hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);
            CAddress addr;
            int nInbound = 0;

            if (hSocket != INVALID_SOCKET)
                if (!addr.SetSockAddr((const struct sockaddr*)&sockaddr))
                    LogPrintf("Warning: Unknown socket family\n");

            {
                LOCK(netParams->cs_vNodes);
                BOOST_FOREACH(CNode* pnode, netParams->vNodes)
                    if (pnode->fInbound)
                        nInbound++;
            }

            if (hSocket == INVALID_SOCKET)
            {
                int nErr = WSAGetLastError();
                if (nErr != WSAEWOULDBLOCK)
                    LogPrintf("socket error accept failed: %s\n", NetworkErrorString(nErr));
            }
            else if (nInbound >= netParams->nMaxConnections - MAX_OUTBOUND_CONNECTIONS)
            {
                closesocket(hSocket);
            }
            else if (CNode::IsBanned(addr))
            {
                LogPrintf("connection from %s dropped (banned)\n", addr.ToString());
                closesocket(hSocket);
            }
            else
            {
                LogPrint(netParams->DebugCategory(), "%s accepted connection %s\n", netParams->LogPrefix(), addr.ToString());
                CNode* pnode = new CNode(hSocket, addr, "", true, netParams);
                pnode->AddRef();
                {
                    LOCK(netParams->cs_vNodes);
                    netParams->vNodes.push_back(pnode);
                }
            }
        }


        //
        // Service each socket
        //
        vector<CNode*> vNodesCopy;
        {
            LOCK(netParams->cs_vNodes);
            vNodesCopy = netParams->vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->AddRef();
        }
        BOOST_FOREACH(CNode* pnode, vNodesCopy)
        {
            boost::this_thread::interruption_point();

            //
            // Receive
            //
            if (pnode->hSocket == INVALID_SOCKET)
                continue;
            if (FD_ISSET(pnode->hSocket, &fdsetRecv) || FD_ISSET(pnode->hSocket, &fdsetError))
            {
                TRY_LOCK(pnode->cs_vRecvMsg, lockRecv);
                if (lockRecv)
                {
                    {
                        // typical socket buffer is 8K-64K
                        char pchBuf[0x10000];
                        int nBytes = recv(pnode->hSocket, pchBuf, sizeof(pchBuf), MSG_DONTWAIT);
                        if (nBytes > 0)
                        {
                            if (!pnode->ReceiveMsgBytes(pchBuf, nBytes))
                                pnode->CloseSocketDisconnect();
                            pnode->nLastRecv = GetTime();
                            pnode->nRecvBytes += nBytes;
                            if(pnode->netParams->MessageStart() == Bitcredit_Params().MessageStart()) {
								pnode->bitcredit_RecordBytesRecv(nBytes);
                            } else {
								pnode->bitcoin_RecordBytesRecv(nBytes);
                            }
                        }
                        else if (nBytes == 0)
                        {
                            // socket closed gracefully
                            if (!pnode->fDisconnect)
                                LogPrint(netParams->DebugCategory(), "%s socket closed\n", netParams->LogPrefix());
                            pnode->CloseSocketDisconnect();
                        }
                        else if (nBytes < 0)
                        {
                            // error
                            int nErr = WSAGetLastError();
                            if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
                            {
                                if (!pnode->fDisconnect)
                                    LogPrintf("socket recv error %s\n", NetworkErrorString(nErr));
                                pnode->CloseSocketDisconnect();
                            }
                        }
                    }
                }
            }

            //
            // Send
            //
            if (pnode->hSocket == INVALID_SOCKET)
                continue;
            if (FD_ISSET(pnode->hSocket, &fdsetSend))
            {
                TRY_LOCK(pnode->cs_vSend, lockSend);
                if (lockSend)
                    SocketSendData(pnode);
            }

            //
            // Inactivity checking
            //
            if (pnode->vSendMsg.empty())
                pnode->nLastSendEmpty = GetTime();
            if (GetTime() - pnode->nTimeConnected > 60)
            {
                if (pnode->nLastRecv == 0 || pnode->nLastSend == 0)
                {
                    LogPrint(netParams->DebugCategory(), "%s socket no message in first 60 seconds, %d %d\n", netParams->LogPrefix(), pnode->nLastRecv != 0, pnode->nLastSend != 0);
                    pnode->fDisconnect = true;
                }
                else if (GetTime() - pnode->nLastSend > 90*60 && GetTime() - pnode->nLastSendEmpty > 90*60)
                {
                    LogPrintf("socket not sending\n");
                    pnode->fDisconnect = true;
                }
                else if (GetTime() - pnode->nLastRecv > 90*60)
                {
                    LogPrintf("socket inactivity timeout\n");
                    pnode->fDisconnect = true;
                }
            }
        }
        {
            LOCK(netParams->cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->Release();
        }

        MilliSleep(10);
    }
}

void Bitcredit_ThreadSocketHandler() {
	SocketHandler(Bitcredit_NetParams());
}
void Bitcoin_ThreadSocketHandler() {
	SocketHandler(Bitcoin_NetParams());
}









#ifdef USE_UPNP
void InternalThreadMapPort(CNetParams * netParams)
{
    std::string port = strprintf("%u", netParams->GetListenPort());
    const char * multicastif = 0;
    const char * minissdpdpath = 0;
    struct UPNPDev * devlist = 0;
    char lanaddr[64];

#ifndef UPNPDISCOVER_SUCCESS
    /* miniupnpc 1.5 */
    devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0);
#else
    /* miniupnpc 1.6 */
    int error = 0;
    devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0, 0, &error);
#endif

    struct UPNPUrls urls;
    struct IGDdatas data;
    int r;

    r = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
    if (r == 1)
    {
        if (netParams->fDiscover) {
            char externalIPAddress[40];
            r = UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIPAddress);
            if(r != UPNPCOMMAND_SUCCESS)
                LogPrintf("%s UPnP: GetExternalIPAddress() returned %d\n", netParams->LogPrefix(), r);
            else
            {
                if(externalIPAddress[0])
                {
                    LogPrintf("%s UPnP: ExternalIPAddress = %s\n", netParams->LogPrefix(), externalIPAddress);
                    AddLocal(CNetAddr(externalIPAddress), LOCAL_UPNP, netParams);
                }
                else
                    LogPrintf("%s UPnP: GetExternalIPAddress failed.\n", netParams->LogPrefix());
            }
        }

        string strDesc = "Credits " + FormatFullVersion();

        try {
            while (true) {
#ifndef UPNPDISCOVER_SUCCESS
                /* miniupnpc 1.5 */
                r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                    port.c_str(), port.c_str(), lanaddr, strDesc.c_str(), "TCP", 0);
#else
                /* miniupnpc 1.6 */
                r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                    port.c_str(), port.c_str(), lanaddr, strDesc.c_str(), "TCP", 0, "0");
#endif

                if(r!=UPNPCOMMAND_SUCCESS)
                    LogPrintf("%s AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
                    	netParams->LogPrefix(), port, port, lanaddr, r, strupnperror(r));
                else
                    LogPrintf("%s UPnP Port Mapping successful.\n", netParams->LogPrefix());

                MilliSleep(20*60*1000); // Refresh every 20 minutes
            }
        }
        catch (boost::thread_interrupted)
        {
            r = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port.c_str(), "TCP", 0);
            LogPrintf("%s UPNP_DeletePortMapping() returned : %d\n", netParams->LogPrefix(), r);
            freeUPNPDevlist(devlist); devlist = 0;
            FreeUPNPUrls(&urls);
            throw;
        }
    } else {
        LogPrintf("%s No valid UPnP IGDs found\n", netParams->LogPrefix());
        freeUPNPDevlist(devlist); devlist = 0;
        if (r != 0)
            FreeUPNPUrls(&urls);
    }
}
void Bitcredit_ThreadMapPort() {
	InternalThreadMapPort(Bitcredit_NetParams());
}
void Bitcoin_ThreadMapPort() {
	InternalThreadMapPort(Bitcoin_NetParams());
}

void MapPort(bool fUseUPnP, CNetParams * netParams)
{
    static boost::thread* upnp_thread = NULL;

    if (fUseUPnP)
    {
        if (upnp_thread) {
            upnp_thread->interrupt();
            upnp_thread->join();
            delete upnp_thread;
        }
        if(netParams->GetListenPort() == bitcredit_NetParams.GetListenPort()) {
        	upnp_thread = new boost::thread(boost::bind(&TraceThread<void (*)()>, "bitcredit_upnp", &Bitcredit_ThreadMapPort));
        } else if(netParams->GetListenPort() == bitcoin_NetParams.GetListenPort()) {
        	upnp_thread = new boost::thread(boost::bind(&TraceThread<void (*)()>, "bitcoin_upnp", &Bitcoin_ThreadMapPort));
        } else {
        	int clientVersion = -1;
        	if(netParams) {
        		clientVersion = netParams->ClientVersion();
        	}
        	LogPrintf("Something has gone very very wrong. params->GetListenPort() is not of any known type. This should never be able to happen. Client version is: %d\n", clientVersion);
        	LogPrintf("STOPPING NODE DUE TO ERROR IN NET PARAMS: %d\n", clientVersion);
        	StopNode();
        }
    }
    else if (upnp_thread) {
        upnp_thread->interrupt();
        upnp_thread->join();
        delete upnp_thread;
        upnp_thread = NULL;
    }
}

#else
void MapPort(bool fUseUPnP, CNetParams * netParams)
{
    // Intentionally left blank.
}
#endif






void DNSAddressSeed(std::string logPrefix, CNetParams * netParams)
{
    const vector<CDNSSeedData> &vSeeds = netParams->GetDNSSeeds();
    int found = 0;

    LogPrintf("%s: Loading addresses from DNS seeds (could take a while)\n", logPrefix);

    BOOST_FOREACH(const CDNSSeedData &seed, vSeeds) {
        if (HaveNameProxy()) {
            AddOneShot(seed.host, netParams);
        } else {
            vector<CNetAddr> vIPs;
            vector<CAddress> vAdd;
            if (LookupHost(seed.host.c_str(), vIPs))
            {
                BOOST_FOREACH(CNetAddr& ip, vIPs)
                {
                    LogPrintf("%s: Adding node with address %s\n", logPrefix, ip.ToString());

                    int nOneDay = 24*3600;
                    CAddress addr = CAddress(CService(ip, netParams->GetDefaultPort()));
                    addr.nTime = GetTime() - 3*nOneDay - GetRand(4*nOneDay); // use a random age between 3 and 7 days old
                    vAdd.push_back(addr);
                    found++;
                }
            }
            netParams->addrman.Add(vAdd, CNetAddr(seed.name, true));
        }
    }

    BOOST_FOREACH(const CDNSSeedData &seed, vSeeds) {
        LogPrintf("%s: Looked up DNS seed %s\n", logPrefix, seed.host);
    }
    LogPrintf("%s: %d addresses found from DNS seeds\n", logPrefix, found);
}
void Bitcredit_ThreadDNSAddressSeed() {
	DNSAddressSeed("Bitcredit", Bitcredit_NetParams());
}
void Bitcoin_ThreadDNSAddressSeed() {
	DNSAddressSeed("Bitcoin", Bitcoin_NetParams());
}












void DumpAddresses(CNetParams * netParams)
{
    int64_t nStart = GetTimeMillis();

    {
        const std::string fileName = netParams->AddrFileName();
    	CAddrDB adb(fileName, netParams->MessageStart(), netParams->ClientVersion());
    	adb.Write(netParams->addrman);

    	LogPrint(netParams->DebugCategory(), "%s Flushed %d addresses to %s  %dms\n",
    			netParams->LogPrefix(), netParams->addrman.size(), fileName, GetTimeMillis() - nStart);
    }
}
void Bitcredit_ThreadDumpAddresses() {
	DumpAddresses(Bitcredit_NetParams());
}
void Bitcoin_ThreadDumpAddresses() {
	DumpAddresses(Bitcoin_NetParams());
}

void static ProcessOneShot(CNetParams * netParams)
{
    string strDest;
    {
        LOCK(netParams->cs_vOneShots);
        if (netParams->vOneShots.empty())
            return;
        strDest = netParams->vOneShots.front();
        netParams->vOneShots.pop_front();
    }
    CAddress addr;
    CSemaphoreGrant grant(*netParams->semOutbound, true);
    if (grant) {
        if (!OpenNetworkConnection(addr, &grant, strDest.c_str(), true, netParams))
            AddOneShot(strDest, netParams);
    }
}

void OpenConnections(CNetParams * netParams, std::string connectKey)
{
    // Connect to specific addresses
    if (mapArgs.count(connectKey) && mapMultiArgs[connectKey].size() > 0)
    {
        for (int64_t nLoop = 0;; nLoop++)
        {
            ProcessOneShot(netParams);
            BOOST_FOREACH(string strAddr, mapMultiArgs[connectKey])
            {
                CAddress addr;
                OpenNetworkConnection(addr, NULL, strAddr.c_str(), true, netParams);
                for (int i = 0; i < 10 && i < nLoop; i++)
                {
                    MilliSleep(500);
                }
            }
            MilliSleep(500);
        }
    }

    // Initiate network connections
    int64_t nStart = GetTime();
    while (true)
    {
        ProcessOneShot(netParams);

        MilliSleep(500);

        CSemaphoreGrant grant(*netParams->semOutbound);
        boost::this_thread::interruption_point();

        // Add seed nodes if DNS seeds are all down (an infrastructure attack?).
        if (netParams->addrman.size() == 0 && (GetTime() - nStart > 60)) {
            static bool done = false;
            if (!done) {
                LogPrintf("Adding fixed seed nodes as DNS doesn't seem to be available.\n");
                netParams->addrman.Add(netParams->GetFixedSeeds(), CNetAddr("127.0.0.1"));
                done = true;
            }
        }

        //
        // Choose an address to connect to based on most recently seen
        //
        CAddress addrConnect;

        // Only connect out to one peer per network group (/16 for IPv4).
        // Do this here so we don't have to critsect vNodes inside mapAddresses critsect.
        int nOutbound = 0;
        set<vector<unsigned char> > setConnected;
        {
            LOCK(netParams->cs_vNodes);
            BOOST_FOREACH(CNode* pnode, netParams->vNodes) {
                if (!pnode->fInbound) {
                    setConnected.insert(pnode->addr.GetGroup());
                    nOutbound++;
                }
            }
        }

        int64_t nANow = GetAdjustedTime();

        int nTries = 0;
        while (true)
        {
            // use an nUnkBias between 10 (no outgoing connections) and 90 (8 outgoing connections)
            CAddress addr = netParams->addrman.Select(10 + min(nOutbound,8)*10);

            // if we selected an invalid address, restart
            if (!addr.IsValid() || setConnected.count(addr.GetGroup()) || IsLocal(addr, netParams))
                break;

            // If we didn't find an appropriate destination after trying 100 addresses fetched from addrman,
            // stop this loop, and let the outer loop run again (which sleeps, adds seed nodes, recalculates
            // already-connected network ranges, ...) before trying new addrman addresses.
            nTries++;
            if (nTries > 100)
                break;

            if (IsLimited(addr, netParams))
                continue;

            // only consider very recently tried nodes after 30 failed attempts
            if (nANow - addr.nLastTry < 600 && nTries < 30)
                continue;

            // do not allow non-default ports, unless after 50 invalid addresses selected already
            if (addr.GetPort() != netParams->GetDefaultPort() && nTries < 50)
                continue;

            addrConnect = addr;
            break;
        }

        if (addrConnect.IsValid())
            OpenNetworkConnection(addrConnect, &grant, NULL, false, netParams);
    }
}
void Bitcredit_ThreadOpenConnections() {
	OpenConnections(Bitcredit_NetParams(), "-connect");
}
void Bitcoin_ThreadOpenConnections() {
	OpenConnections(Bitcoin_NetParams(), "-bitcoin_connect");
}

void OpenAddedConnections(CNetParams * netParams, std::string addnodeKey)
{
    {
        LOCK(netParams->cs_vAddedNodes);
        netParams->vAddedNodes = mapMultiArgs[addnodeKey];
    }

    if (HaveNameProxy()) {
        while(true) {
            list<string> lAddresses(0);
            {
                LOCK(netParams->cs_vAddedNodes);
                BOOST_FOREACH(string& strAddNode, netParams->vAddedNodes)
                    lAddresses.push_back(strAddNode);
            }
            BOOST_FOREACH(string& strAddNode, lAddresses) {
                CAddress addr;
                CSemaphoreGrant grant(*netParams->semOutbound);
                OpenNetworkConnection(addr, &grant, strAddNode.c_str(), false, netParams);
                MilliSleep(500);
            }
            MilliSleep(120000); // Retry every 2 minutes
        }
    }

    for (unsigned int i = 0; true; i++)
    {
        list<string> lAddresses(0);
        {
            LOCK(netParams->cs_vAddedNodes);
            BOOST_FOREACH(string& strAddNode, netParams->vAddedNodes)
                lAddresses.push_back(strAddNode);
        }

        list<vector<CService> > lservAddressesToAdd(0);
        BOOST_FOREACH(string& strAddNode, lAddresses)
        {
            vector<CService> vservNode(0);
            if(Lookup(strAddNode.c_str(), vservNode, netParams->GetDefaultPort(), fNameLookup, 0))
            {
                lservAddressesToAdd.push_back(vservNode);
                {
                    LOCK(netParams->cs_setservAddNodeAddresses);
                    BOOST_FOREACH(CService& serv, vservNode)
                    netParams->setservAddNodeAddresses.insert(serv);
                }
            }
        }
        // Attempt to connect to each IP for each addnode entry until at least one is successful per addnode entry
        // (keeping in mind that addnode entries can have many IPs if fNameLookup)
        {
            LOCK(netParams->cs_vNodes);
            BOOST_FOREACH(CNode* pnode, netParams->vNodes)
                for (list<vector<CService> >::iterator it = lservAddressesToAdd.begin(); it != lservAddressesToAdd.end(); it++)
                    BOOST_FOREACH(CService& addrNode, *(it))
                        if (pnode->addr == addrNode)
                        {
                            it = lservAddressesToAdd.erase(it);
                            it--;
                            break;
                        }
        }
        BOOST_FOREACH(vector<CService>& vserv, lservAddressesToAdd)
        {
            CSemaphoreGrant grant(*netParams->semOutbound);
            OpenNetworkConnection(CAddress(vserv[i % vserv.size()]), &grant, NULL, false, netParams);
            MilliSleep(500);
        }
        MilliSleep(120000); // Retry every 2 minutes
    }
}
void Bitcredit_ThreadOpenAddedConnections() {
	OpenAddedConnections(Bitcredit_NetParams(), "-addnode");
}
void Bitcoin_ThreadOpenAddedConnections() {
	OpenAddedConnections(Bitcoin_NetParams(), "-bitcoin_addnode");
}


// if successful, this moves the passed grant to the constructed node
bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant *grantOutbound, const char *strDest, bool fOneShot, CNetParams * netParams)
{
    //
    // Initiate outbound network connection
    //
    boost::this_thread::interruption_point();
    if (!strDest)
        if (IsLocal(addrConnect, netParams) ||
            FindNode((CNetAddr)addrConnect, netParams) || CNode::IsBanned(addrConnect) ||
            FindNode(addrConnect.ToStringIPPort().c_str(), netParams))
            return false;
    if (strDest && FindNode(strDest, netParams))
        return false;

    CNode* pnode = ConnectNode(addrConnect, strDest, netParams);
    boost::this_thread::interruption_point();

    if (!pnode)
        return false;
    if (grantOutbound)
        grantOutbound->MoveTo(pnode->grantOutbound);
    pnode->fNetworkNode = true;
    if (fOneShot)
        pnode->fOneShot = true;

    return true;
}

// for now, use a very simple selection metric: the node from which we received
// most recently
static int64_t NodeSyncScore(const CNode *pnode) {
    return pnode->nLastRecv;
}

void static StartSync(const vector<CNode*> &vNodes, CNetParams * netParams) {
    CNode *pnodeNewSync = NULL;
    int64_t nBestScore = 0;

    int nBestHeight = netParams->GetNodeSignals().GetHeight().get_value_or(0);

    // Iterate over all nodes
    BOOST_FOREACH(CNode* pnode, vNodes) {
        // check preconditions for allowing a sync
        if (!pnode->fClient && !pnode->fOneShot &&
            !pnode->fDisconnect && pnode->fSuccessfullyConnected &&
            (pnode->nStartingHeight > (nBestHeight - 144)) &&
            (pnode->nVersion < NOBLKS_VERSION_START || pnode->nVersion >= NOBLKS_VERSION_END)) {
            // if ok, compare node's score with the best so far
            int64_t nScore = NodeSyncScore(pnode);
            if (pnodeNewSync == NULL || nScore > nBestScore) {
                pnodeNewSync = pnode;
                nBestScore = nScore;
            }
        }
    }
    // if a new sync candidate was found, start sync!
    if (pnodeNewSync) {
        pnodeNewSync->fStartSync = true;
        netParams->pnodeSync = pnodeNewSync;
    }
}

void MessageHandler(CNetParams * netParams)
{
    SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);
    while (true)
    {
        bool fHaveSyncNode = false;

        vector<CNode*> vNodesCopy;
        {
            LOCK(netParams->cs_vNodes);
            vNodesCopy = netParams->vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy) {
                pnode->AddRef();
                if (pnode == netParams->pnodeSync)
                    fHaveSyncNode = true;
            }
        }

        if (!fHaveSyncNode)
            StartSync(vNodesCopy, netParams);

        // Poll the connected nodes for messages
        CNode* pnodeTrickle = NULL;
        if (!vNodesCopy.empty())
            pnodeTrickle = vNodesCopy[GetRand(vNodesCopy.size())];

        bool fSleep = true;

        BOOST_FOREACH(CNode* pnode, vNodesCopy)
        {
            if (pnode->fDisconnect)
                continue;

            // Receive messages
            {
                TRY_LOCK(pnode->cs_vRecvMsg, lockRecv);
                if (lockRecv)
                {
                    if (!netParams->GetNodeSignals().ProcessMessages(pnode))
                        pnode->CloseSocketDisconnect();

                    if (pnode->nSendSize < SendBufferSize())
                    {
                        if (!pnode->vRecvGetData.empty() || (!pnode->vRecvMsg.empty() && pnode->vRecvMsg[0].complete()))
                        {
                            fSleep = false;
                        }
                    }
                }
            }
            boost::this_thread::interruption_point();

            // Send messages
            {
                TRY_LOCK(pnode->cs_vSend, lockSend);
                if (lockSend)
                	netParams->GetNodeSignals().SendMessages(pnode, pnode == pnodeTrickle);
            }
            boost::this_thread::interruption_point();
        }

        {
            LOCK(netParams->cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->Release();
        }

        if (fSleep)
            MilliSleep(100);
    }
}
void Bitcredit_ThreadMessageHandler() {
	MessageHandler(Bitcredit_NetParams());
}
void Bitcoin_ThreadMessageHandler() {
	MessageHandler(Bitcoin_NetParams());
}






bool BindListenPort(const CService &addrBind, string& strError, CNetParams * netParams)
{
    strError = "";
    int nOne = 1;

    // Create socket for listening for incoming connections
    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);
    if (!addrBind.GetSockAddr((struct sockaddr*)&sockaddr, &len))
    {
        strError = strprintf("Error: bind address family for %s not supported", addrBind.ToString());
        LogPrintf("%s\n", strError);
        return false;
    }

    SOCKET hListenSocket = socket(((struct sockaddr*)&sockaddr)->sa_family, SOCK_STREAM, IPPROTO_TCP);
    if (hListenSocket == INVALID_SOCKET)
    {
        strError = strprintf("Error: Couldn't open socket for incoming connections (socket returned error %s)", NetworkErrorString(WSAGetLastError()));
        LogPrintf("%s\n", strError);
        return false;
    }

#ifdef SO_NOSIGPIPE
    // Different way of disabling SIGPIPE on BSD
    setsockopt(hListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&nOne, sizeof(int));
#endif

#ifndef WIN32
    // Allow binding if the port is still in TIME_WAIT state after
    // the program was closed and restarted.  Not an issue on windows.
    setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int));
#endif


#ifdef WIN32
    // Set to non-blocking, incoming connections will also inherit this
    if (ioctlsocket(hListenSocket, FIONBIO, (u_long*)&nOne) == SOCKET_ERROR)
#else
    if (fcntl(hListenSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
#endif
    {
        strError = strprintf("Error: Couldn't set properties on socket for incoming connections (error %s)", NetworkErrorString(WSAGetLastError()));
        LogPrintf("%s\n", strError);
        return false;
    }

    // some systems don't have IPV6_V6ONLY but are always v6only; others do have the option
    // and enable it by default or not. Try to enable it, if possible.
    if (addrBind.IsIPv6()) {
#ifdef IPV6_V6ONLY
#ifdef WIN32
        setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&nOne, sizeof(int));
#else
        setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&nOne, sizeof(int));
#endif
#endif
#ifdef WIN32
        int nProtLevel = 10 /* PROTECTION_LEVEL_UNRESTRICTED */;
        int nParameterId = 23 /* IPV6_PROTECTION_LEVEl */;
        // this call is allowed to fail
        setsockopt(hListenSocket, IPPROTO_IPV6, nParameterId, (const char*)&nProtLevel, sizeof(int));
#endif
    }

    if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
    {
        int nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE)
            strError = strprintf(_("Unable to bind to %s on this computer. Credits Core is probably already running."), addrBind.ToString());
        else
            strError = strprintf(_("Unable to bind to %s on this computer (bind returned error %s)"), addrBind.ToString(), NetworkErrorString(nErr));
        LogPrintf("%s\n", strError);
        return false;
    }
    LogPrintf("Bound to %s\n", addrBind.ToString());

    // Listen for incoming connections
    if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        strError = strprintf(_("Error: Listening for incoming connections failed (listen returned error %s)"), NetworkErrorString(WSAGetLastError()));
        LogPrintf("%s\n", strError);
        return false;
    }

    netParams->vhListenSocket.push_back(hListenSocket);

    if (addrBind.IsRoutable() && netParams->fDiscover)
        AddLocal(addrBind, LOCAL_BIND, netParams);

    return true;
}

void static Discover(CNetParams * netParams)
{
    if (!netParams->fDiscover)
        return;

#ifdef WIN32
    // Get local host IP
    char pszHostName[1000] = "";
    if (gethostname(pszHostName, sizeof(pszHostName)) != SOCKET_ERROR)
    {
        vector<CNetAddr> vaddr;
        if (LookupHost(pszHostName, vaddr))
        {
            BOOST_FOREACH (const CNetAddr &addr, vaddr)
            {
                AddLocal(addr, LOCAL_IF, netParams);
            }
        }
    }
#else
    // Get local host ip
    struct ifaddrs* myaddrs;
    if (getifaddrs(&myaddrs) == 0)
    {
        for (struct ifaddrs* ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL) continue;
            if ((ifa->ifa_flags & IFF_UP) == 0) continue;
            if (strcmp(ifa->ifa_name, "lo") == 0) continue;
            if (strcmp(ifa->ifa_name, "lo0") == 0) continue;
            if (ifa->ifa_addr->sa_family == AF_INET)
            {
                struct sockaddr_in* s4 = (struct sockaddr_in*)(ifa->ifa_addr);
                CNetAddr addr(s4->sin_addr);
                if (AddLocal(addr, LOCAL_IF, netParams))
                    LogPrintf("IPv4 %s: %s\n", ifa->ifa_name, addr.ToString());
            }
            else if (ifa->ifa_addr->sa_family == AF_INET6)
            {
                struct sockaddr_in6* s6 = (struct sockaddr_in6*)(ifa->ifa_addr);
                CNetAddr addr(s6->sin6_addr);
                if (AddLocal(addr, LOCAL_IF, netParams))
                    LogPrintf("IPv6 %s: %s\n", ifa->ifa_name, addr.ToString());
            }
        }
        freeifaddrs(myaddrs);
    }
#endif
}

void InitializeNetParams(CNetParams * netParams) {
    // initialize semaphore
    if (netParams->semOutbound == NULL) {
    int nMaxOutbound = min(MAX_OUTBOUND_CONNECTIONS, netParams->nMaxConnections);
    netParams->semOutbound = new CSemaphore(nMaxOutbound);
    }
    if (netParams->pnodeLocalHost == NULL)
    	netParams->pnodeLocalHost = new CNode(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), netParams->nLocalServices), "", false, netParams);
}
void StartNode(boost::thread_group& threadGroup)
{
	CNetParams * bitcoin_netParams = Bitcoin_NetParams();
	CNetParams * bitcredit_netParams = Bitcredit_NetParams();

	InitializeNetParams(bitcoin_netParams);
	InitializeNetParams(bitcredit_netParams);

	Discover(bitcoin_netParams);
	Discover(bitcredit_netParams);

	// Don't use external IPv4 discovery, when -onlynet="IPv6"
	if (!IsLimited(NET_IPV4, bitcoin_netParams))
	    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "bitcoin_ext-ip", &Bitcoin_ThreadGetMyExternalIP));
	// Don't use external IPv4 discovery, when -onlynet="IPv6"
	if (!IsLimited(NET_IPV4, bitcredit_netParams))
	    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "bitcredit_ext-ip", &Bitcredit_ThreadGetMyExternalIP));

    //
    // Start threads
    //
    if (!GetBoolArg("-bitcoin_dnsseed", true))
        LogPrintf("DNS seeding disabled\n");
    else
        threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "bitcoin_dnsseed", &Bitcoin_ThreadDNSAddressSeed));
   if (!GetBoolArg("-dnsseed", true))
        LogPrintf("DNS seeding disabled\n");
    else
        threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "bitcredit_dnsseed", &Bitcredit_ThreadDNSAddressSeed));

    // Map ports with UPnP
    MapPort(GetBoolArg("-bitcoin_upnp", DEFAULT_UPNP), bitcoin_netParams);
    MapPort(GetBoolArg("-upnp", DEFAULT_UPNP), bitcredit_netParams);

    // Send and receive from sockets, accept connections
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "bitcoin_net", &Bitcoin_ThreadSocketHandler));
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "bitcredit_net", &Bitcredit_ThreadSocketHandler));

    // Initiate outbound connections from -addnode
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "bitcredit_addcon", &Bitcoin_ThreadOpenAddedConnections));
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "bitcredit_addcon", &Bitcredit_ThreadOpenAddedConnections));

    // Initiate outbound connections
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "bitcoin_opencon", &Bitcoin_ThreadOpenConnections));
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "bitcredit_opencon", &Bitcredit_ThreadOpenConnections));

    // Process messages
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "bitcoin_msghand", &Bitcoin_ThreadMessageHandler));
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "bitcredit_msghand", &Bitcredit_ThreadMessageHandler));

    // Dump network addresses
    threadGroup.create_thread(boost::bind(&LoopForever<void (*)()>, "bitcoin_dumpaddr", &Bitcoin_ThreadDumpAddresses, DUMP_ADDRESSES_INTERVAL * 1000));
    threadGroup.create_thread(boost::bind(&LoopForever<void (*)()>, "bitcredit_dumpaddr", &Bitcredit_ThreadDumpAddresses, DUMP_ADDRESSES_INTERVAL * 1000));
}

void StopConnections(CNetParams * netParams) {
    MapPort(false, netParams);
    if (netParams->semOutbound)
        for (int i=0; i<MAX_OUTBOUND_CONNECTIONS; i++)
        	netParams->semOutbound->post();
    MilliSleep(50);
}

bool StopNode()
{
    LogPrintf("StopNode()\n");

    StopConnections(Bitcredit_NetParams());
    StopConnections(Bitcoin_NetParams());

    DumpAddresses(Bitcredit_NetParams());
    DumpAddresses(Bitcoin_NetParams());

    return true;
}

class CNetCleanup
{
public:
	void CleanUpParams(CNetParams * netParams) {
	    // Close sockets
	    BOOST_FOREACH(CNode* pnode, netParams->vNodes)
	        if (pnode->hSocket != INVALID_SOCKET)
	            closesocket(pnode->hSocket);
	    BOOST_FOREACH(SOCKET hListenSocket, netParams->vhListenSocket)
	        if (hListenSocket != INVALID_SOCKET)
	            if (closesocket(hListenSocket) == SOCKET_ERROR)
	                LogPrintf("closesocket(hListenSocket) failed with error %d\n", WSAGetLastError());

	    // clean up some globals (to help leak detection)
	    BOOST_FOREACH(CNode *pnode, netParams->vNodes)
	        delete pnode;
	    BOOST_FOREACH(CNode *pnode, netParams->vNodesDisconnected)
	        delete pnode;
	    netParams->vNodes.clear();
	    netParams->vNodesDisconnected.clear();
	    delete netParams->semOutbound;
	    netParams->semOutbound = NULL;

        delete netParams->pnodeLocalHost;
        netParams->pnodeLocalHost = NULL;
	}

    CNetCleanup()
    {
    }
    ~CNetCleanup()
    {
        CleanUpParams(Bitcredit_NetParams());
        CleanUpParams(Bitcoin_NetParams());

#ifdef WIN32
        // Shutdown Windows Sockets
        WSACleanup();
#endif
    }
}
instance_of_cnetcleanup;







void Bitcredit_RelayTransaction(const Bitcredit_CTransaction& tx, const uint256& hash, CNetParams * netParams)
{
    CDataStream ss(SER_NETWORK, netParams->ProtocolVersion());
    ss.reserve(10000);
    ss << tx;
    Bitcredit_RelayTransaction(tx, hash, ss, netParams);
}
void Bitcredit_RelayTransaction(const Bitcredit_CTransaction& tx, const uint256& hash, const CDataStream& ss, CNetParams * netParams)
{
    CInv inv(MSG_TX, hash);
    {
        LOCK(netParams->cs_mapRelay);
        // Expire old relay messages
        while (!netParams->vRelayExpiration.empty() && netParams->vRelayExpiration.front().first < GetTime())
        {
        	netParams->mapRelay.erase(netParams->vRelayExpiration.front().second);
        	netParams->vRelayExpiration.pop_front();
        }

        // Save original serialized message so newer versions are preserved
        netParams->mapRelay.insert(std::make_pair(inv, ss));
        netParams->vRelayExpiration.push_back(std::make_pair(GetTime() + 15 * 60, inv));
    }
    LOCK(netParams->cs_vNodes);
    BOOST_FOREACH(CNode* pnode, netParams->vNodes)
    {
        if(!pnode->fRelayTxes)
            continue;
        LOCK(pnode->cs_filter);
        if (pnode->pfilter)
        {
            if (pnode->pfilter->bitcredit_IsRelevantAndUpdate(tx, hash))
                pnode->PushInventory(inv);
        } else
            pnode->PushInventory(inv);
    }
}

void Bitcoin_RelayTransaction(const Bitcoin_CTransaction& tx, const uint256& hash, CNetParams * netParams)
{
    CDataStream ss(SER_NETWORK, netParams->ProtocolVersion());
    ss.reserve(10000);
    ss << tx;
    Bitcoin_RelayTransaction(tx, hash, ss, netParams);
}
void Bitcoin_RelayTransaction(const Bitcoin_CTransaction& tx, const uint256& hash, const CDataStream& ss, CNetParams * netParams)
{
    CInv inv(MSG_TX, hash);
    {
        LOCK(netParams->cs_mapRelay);
        // Expire old relay messages
        while (!netParams->vRelayExpiration.empty() && netParams->vRelayExpiration.front().first < GetTime())
        {
        	netParams->mapRelay.erase(netParams->vRelayExpiration.front().second);
        	netParams->vRelayExpiration.pop_front();
        }

        // Save original serialized message so newer versions are preserved
        netParams->mapRelay.insert(std::make_pair(inv, ss));
        netParams->vRelayExpiration.push_back(std::make_pair(GetTime() + 15 * 60, inv));
    }
    LOCK(netParams->cs_vNodes);
    BOOST_FOREACH(CNode* pnode, netParams->vNodes)
    {
        if(!pnode->fRelayTxes)
            continue;
        LOCK(pnode->cs_filter);
        if (pnode->pfilter)
        {
            if (pnode->pfilter->bitcoin_IsRelevantAndUpdate(tx, hash))
                pnode->PushInventory(inv);
        } else
            pnode->PushInventory(inv);
    }
}

void CNode::bitcredit_RecordBytesRecv(uint64_t bytes)
{
    LOCK(bitcredit_cs_totalBytesRecv);
    bitcredit_nTotalBytesRecv += bytes;
}

void CNode::bitcredit_RecordBytesSent(uint64_t bytes)
{
    LOCK(bitcredit_cs_totalBytesSent);
    bitcredit_nTotalBytesSent += bytes;
}

uint64_t CNode::bitcredit_GetTotalBytesRecv()
{
    LOCK(bitcredit_cs_totalBytesRecv);
    return bitcredit_nTotalBytesRecv;
}

uint64_t CNode::bitcredit_GetTotalBytesSent()
{
    LOCK(bitcredit_cs_totalBytesSent);
    return bitcredit_nTotalBytesSent;
}

void CNode::bitcoin_RecordBytesRecv(uint64_t bytes)
{
    LOCK(bitcoin_cs_totalBytesRecv);
    bitcoin_nTotalBytesRecv += bytes;
}

void CNode::bitcoin_RecordBytesSent(uint64_t bytes)
{
    LOCK(bitcoin_cs_totalBytesSent);
    bitcoin_nTotalBytesSent += bytes;
}

uint64_t CNode::bitcoin_GetTotalBytesRecv()
{
    LOCK(bitcoin_cs_totalBytesRecv);
    return bitcoin_nTotalBytesRecv;
}

uint64_t CNode::bitcoin_GetTotalBytesSent()
{
    LOCK(bitcoin_cs_totalBytesSent);
    return bitcoin_nTotalBytesSent;
}

void CNode::Fuzz(int nChance)
{
    if (!fSuccessfullyConnected) return; // Don't fuzz initial handshake
    if (GetRand(nChance) != 0) return; // Fuzz 1 of every nChance messages

    switch (GetRand(3))
    {
    case 0:
        // xor a random byte with a random value:
        if (!ssSend.empty()) {
            CDataStream::size_type pos = GetRand(ssSend.size());
            ssSend[pos] ^= (unsigned char)(GetRand(256));
        }
        break;
    case 1:
        // delete a random byte:
        if (!ssSend.empty()) {
            CDataStream::size_type pos = GetRand(ssSend.size());
            ssSend.erase(ssSend.begin()+pos);
        }
        break;
    case 2:
        // insert a random byte at a random position
        {
            CDataStream::size_type pos = GetRand(ssSend.size());
            char ch = (char)GetRand(256);
            ssSend.insert(ssSend.begin()+pos, ch);
        }
        break;
    }
    // Chance of more than one change half the time:
    // (more changes exponentially less likely):
    Fuzz(2);
}

//
// CAddrDB
//

CAddrDB::CAddrDB(std::string fileNameIn, const MessageStartChars& messageStartIn, int clientVersionIn)
{
	fileName = fileNameIn;
	for(int i=0; i<MESSAGE_START_SIZE ; i++) {
		messageStart[i] = messageStartIn[i];
	}
	clientVersion = clientVersionIn;
    pathAddr = GetDataDir() / fileName;
}

bool CAddrDB::Write(const CAddrMan& addr)
{
    // Generate random temporary filename
    unsigned short randv = 0;
    RAND_bytes((unsigned char *)&randv, sizeof(randv));
    std::string tmpfn = strprintf("%s.%04x", fileName, randv);

    // serialize addresses, checksum data up to that point, then append csum
    CDataStream ssPeers(SER_DISK, clientVersion);
    ssPeers << FLATDATA(messageStart);
    ssPeers << addr;
    uint256 hash = Hash(ssPeers.begin(), ssPeers.end());
    ssPeers << hash;

    // open temp output file, and associate with CAutoFile
    boost::filesystem::path pathTmp = GetDataDir() / tmpfn;
    FILE *file = fopen(pathTmp.string().c_str(), "wb");
    CAutoFile fileout = CAutoFile(file, SER_DISK, clientVersion);
    if (!fileout)
        return error("%s : Failed to open file %s", __func__, pathTmp.string());

    // Write and commit header, data
    try {
        fileout << ssPeers;
    }
    catch (std::exception &e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
    FileCommit(fileout);
    fileout.fclose();

    // replace existing xxx_peers.dat, if any, with new peers.dat.XXXX
    if (!RenameOver(pathTmp, pathAddr))
        return error("%s : Rename-into-place failed", __func__);

    return true;
}

bool CAddrDB::Read(CAddrMan& addr)
{
    // open input file, and associate with CAutoFile
    FILE *file = fopen(pathAddr.string().c_str(), "rb");
    CAutoFile filein = CAutoFile(file, SER_DISK, clientVersion);
    if (!filein)
        return error("%s : Failed to open file %s", __func__, pathAddr.string());

    // use file size to size memory buffer
    int fileSize = boost::filesystem::file_size(pathAddr);
    int dataSize = fileSize - sizeof(uint256);
    // Don't try to resize to a negative number if file is small
    if (dataSize < 0)
        dataSize = 0;
    vector<unsigned char> vchData;
    vchData.resize(dataSize);
    uint256 hashIn;

    // read data and checksum from file
    try {
        filein.read((char *)&vchData[0], dataSize);
        filein >> hashIn;
    }
    catch (std::exception &e) {
        return error("%s : Deserialize or I/O error - %s", __func__, e.what());
    }
    filein.fclose();

    CDataStream ssPeers(vchData, SER_DISK, clientVersion);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssPeers.begin(), ssPeers.end());
    if (hashIn != hashTmp)
        return error("%s : Checksum mismatch, data corrupted", __func__);

    unsigned char pchMsgTmp[4];
    try {
        // de-serialize file header (network specific magic number) and ..
        ssPeers >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, messageStart, sizeof(pchMsgTmp)))
            return error("%s : Invalid network magic number", __func__);

        // de-serialize address data into one CAddrMan object
        ssPeers >> addr;
    }
    catch (std::exception &e) {
        return error("%s : Deserialize or I/O error - %s", __func__, e.what());
    }

    return true;
}
