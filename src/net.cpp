// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "db.h"
#include "net.h"
#include "init.h"
#include "addrman.h"
#include "ui_interface.h"
#include "script.h"

#ifdef WIN32
#include <string.h>
#endif

#ifdef USE_UPNP
#include <miniupnpc/miniwget.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
#endif

// Dump addresses to peers.dat every 15 minutes (900s)
#define DUMP_ADDRESSES_INTERVAL 900

using namespace std;
using namespace boost;

static const int MAX_OUTBOUND_CONNECTIONS = 8;

bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant *grantOutbound = NULL, const char *strDest = NULL, bool fOneShot = false);


struct LocalServiceInfo {
    int nScore;
    int nPort;
};

//
// Global state variables
//
bool fDiscover = true;
uint64 nLocalServices = NODE_NETWORK;
static CCriticalSection cs_mapLocalHost;
static map<CNetAddr, LocalServiceInfo> mapLocalHost;
static bool vfReachable[NET_MAX] = {};
static bool vfLimited[NET_MAX] = {};
static CNode* pnodeLocalHost = NULL;
static CNode* pnodeSync = NULL;
uint64 nLocalHostNonce = 0;
static std::vector<SOCKET> vhListenSocket;
CAddrMan addrman;
int nMaxConnections = 125;

vector<CNode*> vNodes;
CCriticalSection cs_vNodes;
map<CInv, CDataStream> mapRelay;
deque<pair<int64, CInv> > vRelayExpiration;
CCriticalSection cs_mapRelay;
limitedmap<CInv, int64> mapAlreadyAskedFor(MAX_INV_SZ);

static deque<string> vOneShots;
CCriticalSection cs_vOneShots;

set<CNetAddr> setservAddNodeAddresses;
CCriticalSection cs_setservAddNodeAddresses;

vector<std::string> vAddedNodes;
CCriticalSection cs_vAddedNodes;

static CSemaphore *semOutbound = NULL;

void AddOneShot(string strDest)
{
    LOCK(cs_vOneShots);
    vOneShots.push_back(strDest);
}

unsigned short GetListenPort()
{
    return (unsigned short)(GetArg("-port", GetDefaultPort()));
}

void CNode::PushGetBlocks(CBlockIndex* pindexBegin, uint256 hashEnd)
{
    // Filter out duplicate requests
    if (pindexBegin == pindexLastGetBlocksBegin && hashEnd == hashLastGetBlocksEnd)
        return;
    pindexLastGetBlocksBegin = pindexBegin;
    hashLastGetBlocksEnd = hashEnd;

    PushMessage("getblocks", CBlockLocator(pindexBegin), hashEnd);
}

// find 'best' local address for a particular peer
bool GetLocal(CService& addr, const CNetAddr *paddrPeer)
{
    if (fNoListen)
        return false;

    int nBestScore = -1;
    int nBestReachability = -1;
    {
        LOCK(cs_mapLocalHost);
        for (map<CNetAddr, LocalServiceInfo>::iterator it = mapLocalHost.begin(); it != mapLocalHost.end(); it++)
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
CAddress GetLocalAddress(const CNetAddr *paddrPeer)
{
    CAddress ret(CService("0.0.0.0",0),0);
    CService addr;
    if (GetLocal(addr, paddrPeer))
    {
        ret = CAddress(addr);
        ret.nServices = nLocalServices;
        ret.nTime = GetAdjustedTime();
    }
    return ret;
}

bool RecvLine(SOCKET hSocket, string& strLine)
{
    strLine = "";
    loop
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
                printf("socket closed\n");
                return false;
            }
            else
            {
                // socket error
                int nErr = WSAGetLastError();
                printf("recv failed: %d\n", nErr);
                return false;
            }
        }
    }
}

// used when scores of local addresses may have changed
// pushes better local address to peers
void static AdvertizeLocal()
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if (pnode->fSuccessfullyConnected)
        {
            CAddress addrLocal = GetLocalAddress(&pnode->addr);
            if (addrLocal.IsRoutable() && (CService)addrLocal != (CService)pnode->addrLocal)
            {
                pnode->PushAddress(addrLocal);
                pnode->addrLocal = addrLocal;
            }
        }
    }
}

void SetReachable(enum Network net, bool fFlag)
{
    LOCK(cs_mapLocalHost);
    vfReachable[net] = fFlag;
    if (net == NET_IPV6 && fFlag)
        vfReachable[NET_IPV4] = true;
}

// learn a new local address
bool AddLocal(const CService& addr, int nScore)
{
    if (!addr.IsRoutable())
        return false;

    if (!fDiscover && nScore < LOCAL_MANUAL)
        return false;

    if (IsLimited(addr))
        return false;

    printf("AddLocal(%s,%i)\n", addr.ToString().c_str(), nScore);

    {
        LOCK(cs_mapLocalHost);
        bool fAlready = mapLocalHost.count(addr) > 0;
        LocalServiceInfo &info = mapLocalHost[addr];
        if (!fAlready || nScore >= info.nScore) {
            info.nScore = nScore + (fAlready ? 1 : 0);
            info.nPort = addr.GetPort();
        }
        SetReachable(addr.GetNetwork());
    }

    AdvertizeLocal();

    return true;
}

bool AddLocal(const CNetAddr &addr, int nScore)
{
    return AddLocal(CService(addr, GetListenPort()), nScore);
}

/** Make a particular network entirely off-limits (no automatic connects to it) */
void SetLimited(enum Network net, bool fLimited)
{
    if (net == NET_UNROUTABLE)
        return;
    LOCK(cs_mapLocalHost);
    vfLimited[net] = fLimited;
}

bool IsLimited(enum Network net)
{
    LOCK(cs_mapLocalHost);
    return vfLimited[net];
}

bool IsLimited(const CNetAddr &addr)
{
    return IsLimited(addr.GetNetwork());
}

/** vote for a local address */
bool SeenLocal(const CService& addr)
{
    {
        LOCK(cs_mapLocalHost);
        if (mapLocalHost.count(addr) == 0)
            return false;
        mapLocalHost[addr].nScore++;
    }

    AdvertizeLocal();

    return true;
}

/** check whether a given address is potentially local */
bool IsLocal(const CService& addr)
{
    LOCK(cs_mapLocalHost);
    return mapLocalHost.count(addr) > 0;
}

/** check whether a given address is in a network we can probably connect to */
bool IsReachable(const CNetAddr& addr)
{
    LOCK(cs_mapLocalHost);
    enum Network net = addr.GetNetwork();
    return vfReachable[net] && !vfLimited[net];
}

bool GetMyExternalIP2(const CService& addrConnect, const char* pszGet, const char* pszKeyword, CNetAddr& ipRet)
{
    SOCKET hSocket;
    if (!ConnectSocket(addrConnect, hSocket))
        return error("GetMyExternalIP() : connection to %s failed", addrConnect.ToString().c_str());

    send(hSocket, pszGet, strlen(pszGet), MSG_NOSIGNAL);

    string strLine;
    while (RecvLine(hSocket, strLine))
    {
        if (strLine.empty()) // HTTP response is separated from headers by blank line
        {
            loop
            {
                if (!RecvLine(hSocket, strLine))
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
            printf("GetMyExternalIP() received [%s] %s\n", strLine.c_str(), addr.ToString().c_str());
            if (!addr.IsValid() || !addr.IsRoutable())
                return false;
            ipRet.SetIP(addr);
            return true;
        }
    }
    closesocket(hSocket);
    return error("GetMyExternalIP() : connection closed");
}

bool GetMyExternalIP(CNetAddr& ipRet)
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

        if (GetMyExternalIP2(addrConnect, pszGet, pszKeyword, ipRet))
            return true;
    }

    return false;
}

void ThreadGetMyExternalIP(void* parg)
{
    // Make this thread recognisable as the external IP detection thread
    RenameThread("bitcoin-ext-ip");

    CNetAddr addrLocalHost;
    if (GetMyExternalIP(addrLocalHost))
    {
        printf("GetMyExternalIP() returned %s\n", addrLocalHost.ToStringIP().c_str());
        AddLocal(addrLocalHost, LOCAL_HTTP);
    }
}





void AddressCurrentlyConnected(const CService& addr)
{
    addrman.Connected(addr);
}







CNode* FindNode(const CNetAddr& ip)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        if ((CNetAddr)pnode->addr == ip)
            return (pnode);
    return NULL;
}

CNode* FindNode(std::string addrName)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        if (pnode->addrName == addrName)
            return (pnode);
    return NULL;
}

CNode* FindNode(const CService& addr)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        if ((CService)pnode->addr == addr)
            return (pnode);
    return NULL;
}

CNode* ConnectNode(CAddress addrConnect, const char *pszDest)
{
    if (pszDest == NULL) {
        if (IsLocal(addrConnect))
            return NULL;

        // Look for an existing connection
        CNode* pnode = FindNode((CService)addrConnect);
        if (pnode)
        {
            pnode->AddRef();
            return pnode;
        }
    }


    /// debug print
    printf("trying connection %s lastseen=%.1fhrs\n",
        pszDest ? pszDest : addrConnect.ToString().c_str(),
        pszDest ? 0 : (double)(GetAdjustedTime() - addrConnect.nTime)/3600.0);

    // Connect
    SOCKET hSocket;
    if (pszDest ? ConnectSocketByName(addrConnect, hSocket, pszDest, GetDefaultPort()) : ConnectSocket(addrConnect, hSocket))
    {
        addrman.Attempt(addrConnect);

        /// debug print
        printf("connected %s\n", pszDest ? pszDest : addrConnect.ToString().c_str());

        // Set to non-blocking
#ifdef WIN32
        u_long nOne = 1;
        if (ioctlsocket(hSocket, FIONBIO, &nOne) == SOCKET_ERROR)
            printf("ConnectSocket() : ioctlsocket non-blocking setting failed, error %d\n", WSAGetLastError());
#else
        if (fcntl(hSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
            printf("ConnectSocket() : fcntl non-blocking setting failed, error %d\n", errno);
#endif

        // Add node
        CNode* pnode = new CNode(hSocket, addrConnect, pszDest ? pszDest : "", false);
        pnode->AddRef();

        {
            LOCK(cs_vNodes);
            vNodes.push_back(pnode);
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
        printf("disconnecting node %s\n", addrName.c_str());
        closesocket(hSocket);
        hSocket = INVALID_SOCKET;
    }

    // in case this fails, we'll empty the recv buffer when the CNode is deleted
    TRY_LOCK(cs_vRecvMsg, lockRecv);
    if (lockRecv)
        vRecvMsg.clear();

    // if this was the sync node, we'll need a new one
    if (this == pnodeSync)
        pnodeSync = NULL;
}

void CNode::Cleanup()
{
}


void CNode::PushVersion()
{
    /// when NTP implemented, change to just nTime = GetAdjustedTime()
    int64 nTime = (fInbound ? GetAdjustedTime() : GetTime());
    CAddress addrYou = (addr.IsRoutable() && !IsProxy(addr) ? addr : CAddress(CService("0.0.0.0",0)));
    CAddress addrMe = GetLocalAddress(&addr);
    RAND_bytes((unsigned char*)&nLocalHostNonce, sizeof(nLocalHostNonce));
    printf("send version message: version %d, blocks=%d, us=%s, them=%s, peer=%s\n", PROTOCOL_VERSION, nBestHeight, addrMe.ToString().c_str(), addrYou.ToString().c_str(), addr.ToString().c_str());
    PushMessage("version", PROTOCOL_VERSION, nLocalServices, nTime, addrYou, addrMe,
                nLocalHostNonce, FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, std::vector<string>()), nBestHeight);
}





std::map<CNetAddr, int64> CNode::setBanned;
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
        std::map<CNetAddr, int64>::iterator i = setBanned.find(ip);
        if (i != setBanned.end())
        {
            int64 t = (*i).second;
            if (GetTime() < t)
                fResult = true;
        }
    }
    return fResult;
}

bool CNode::Misbehaving(int howmuch)
{
    if (addr.IsLocal())
    {
        printf("Warning: Local node %s misbehaving (delta: %d)!\n", addrName.c_str(), howmuch);
        return false;
    }

    nMisbehavior += howmuch;
    if (nMisbehavior >= GetArg("-banscore", 100))
    {
        int64 banTime = GetTime()+GetArg("-bantime", 60*60*24);  // Default 24-hour ban
        printf("Misbehaving: %s (%d -> %d) DISCONNECTING\n", addr.ToString().c_str(), nMisbehavior-howmuch, nMisbehavior);
        {
            LOCK(cs_setBanned);
            if (setBanned[addr] < banTime)
                setBanned[addr] = banTime;
        }
        CloseSocketDisconnect();
        return true;
    } else
        printf("Misbehaving: %s (%d -> %d)\n", addr.ToString().c_str(), nMisbehavior-howmuch, nMisbehavior);
    return false;
}

#undef X
#define X(name) stats.name = name
void CNode::copyStats(CNodeStats &stats)
{
    X(nServices);
    X(nLastSend);
    X(nLastRecv);
    X(nTimeConnected);
    X(addrName);
    X(nVersion);
    X(cleanSubVer);
    X(fInbound);
    X(nStartingHeight);
    X(nMisbehavior);
    X(nSendBytes);
    X(nRecvBytes);
    X(nBlocksRequested);
    stats.fSyncNode = (this == pnodeSync);
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
                    printf("socket send error %d\n", nErr);
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

static list<CNode*> vNodesDisconnected;

void ThreadSocketHandler()
{
    unsigned int nPrevNodeCount = 0;
    loop
    {
        //
        // Disconnect nodes
        //
        {
            LOCK(cs_vNodes);
            // Disconnect unused nodes
            vector<CNode*> vNodesCopy = vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
            {
                if (pnode->fDisconnect ||
                    (pnode->GetRefCount() <= 0 && pnode->vRecvMsg.empty() && pnode->nSendSize == 0 && pnode->ssSend.empty()))
                {
                    // remove from vNodes
                    vNodes.erase(remove(vNodes.begin(), vNodes.end(), pnode), vNodes.end());

                    // release outbound grant (if any)
                    pnode->grantOutbound.Release();

                    // close socket and cleanup
                    pnode->CloseSocketDisconnect();
                    pnode->Cleanup();

                    // hold in disconnected pool until all refs are released
                    if (pnode->fNetworkNode || pnode->fInbound)
                        pnode->Release();
                    vNodesDisconnected.push_back(pnode);
                }
            }

            // Delete disconnected nodes
            list<CNode*> vNodesDisconnectedCopy = vNodesDisconnected;
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
                        vNodesDisconnected.remove(pnode);
                        delete pnode;
                    }
                }
            }
        }
        if (vNodes.size() != nPrevNodeCount)
        {
            nPrevNodeCount = vNodes.size();
            uiInterface.NotifyNumConnectionsChanged(vNodes.size());
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

        BOOST_FOREACH(SOCKET hListenSocket, vhListenSocket) {
            FD_SET(hListenSocket, &fdsetRecv);
            hSocketMax = max(hSocketMax, hListenSocket);
            have_fds = true;
        }
        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
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
                printf("socket select error %d\n", nErr);
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
        BOOST_FOREACH(SOCKET hListenSocket, vhListenSocket)
        if (hListenSocket != INVALID_SOCKET && FD_ISSET(hListenSocket, &fdsetRecv))
        {
#ifdef USE_IPV6
            struct sockaddr_storage sockaddr;
#else
            struct sockaddr sockaddr;
#endif
            socklen_t len = sizeof(sockaddr);
            SOCKET hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);
            CAddress addr;
            int nInbound = 0;

            if (hSocket != INVALID_SOCKET)
                if (!addr.SetSockAddr((const struct sockaddr*)&sockaddr))
                    printf("Warning: Unknown socket family\n");

            {
                LOCK(cs_vNodes);
                BOOST_FOREACH(CNode* pnode, vNodes)
                    if (pnode->fInbound)
                        nInbound++;
            }

            if (hSocket == INVALID_SOCKET)
            {
                int nErr = WSAGetLastError();
                if (nErr != WSAEWOULDBLOCK)
                    printf("socket error accept failed: %d\n", nErr);
            }
            else if (nInbound >= nMaxConnections - MAX_OUTBOUND_CONNECTIONS)
            {
                {
                    LOCK(cs_setservAddNodeAddresses);
                    if (!setservAddNodeAddresses.count(addr))
                        closesocket(hSocket);
                }
            }
            else if (CNode::IsBanned(addr))
            {
                printf("connection from %s dropped (banned)\n", addr.ToString().c_str());
                closesocket(hSocket);
            }
            else
            {
                printf("accepted connection %s\n", addr.ToString().c_str());
                CNode* pnode = new CNode(hSocket, addr, "", true);
                pnode->AddRef();
                {
                    LOCK(cs_vNodes);
                    vNodes.push_back(pnode);
                }
            }
        }


        //
        // Service each socket
        //
        vector<CNode*> vNodesCopy;
        {
            LOCK(cs_vNodes);
            vNodesCopy = vNodes;
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
                        }
                        else if (nBytes == 0)
                        {
                            // socket closed gracefully
                            if (!pnode->fDisconnect)
                                printf("socket closed\n");
                            pnode->CloseSocketDisconnect();
                        }
                        else if (nBytes < 0)
                        {
                            // error
                            int nErr = WSAGetLastError();
                            if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
                            {
                                if (!pnode->fDisconnect)
                                    printf("socket recv error %d\n", nErr);
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
                    printf("socket no message in first 60 seconds, %d %d\n", pnode->nLastRecv != 0, pnode->nLastSend != 0);
                    pnode->fDisconnect = true;
                }
                else if (GetTime() - pnode->nLastSend > 90*60 && GetTime() - pnode->nLastSendEmpty > 90*60)
                {
                    printf("socket not sending\n");
                    pnode->fDisconnect = true;
                }
                else if (GetTime() - pnode->nLastRecv > 90*60)
                {
                    printf("socket inactivity timeout\n");
                    pnode->fDisconnect = true;
                }
            }
        }
        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->Release();
        }

        MilliSleep(10);
    }
}









#ifdef USE_UPNP
void ThreadMapPort()
{
    std::string port = strprintf("%u", GetListenPort());
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
        if (fDiscover) {
            char externalIPAddress[40];
            r = UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIPAddress);
            if(r != UPNPCOMMAND_SUCCESS)
                printf("UPnP: GetExternalIPAddress() returned %d\n", r);
            else
            {
                if(externalIPAddress[0])
                {
                    printf("UPnP: ExternalIPAddress = %s\n", externalIPAddress);
                    AddLocal(CNetAddr(externalIPAddress), LOCAL_UPNP);
                }
                else
                    printf("UPnP: GetExternalIPAddress failed.\n");
            }
        }

        string strDesc = "Litecoin " + FormatFullVersion();

        try {
            loop {
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
                    printf("AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
                        port.c_str(), port.c_str(), lanaddr, r, strupnperror(r));
                else
                    printf("UPnP Port Mapping successful.\n");;

                MilliSleep(20*60*1000); // Refresh every 20 minutes
            }
        }
        catch (boost::thread_interrupted)
        {
            r = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port.c_str(), "TCP", 0);
            printf("UPNP_DeletePortMapping() returned : %d\n", r);
            freeUPNPDevlist(devlist); devlist = 0;
            FreeUPNPUrls(&urls);
            throw;
        }
    } else {
        printf("No valid UPnP IGDs found\n");
        freeUPNPDevlist(devlist); devlist = 0;
        if (r != 0)
            FreeUPNPUrls(&urls);
    }
}

void MapPort(bool fUseUPnP)
{
    static boost::thread* upnp_thread = NULL;

    if (fUseUPnP)
    {
        if (upnp_thread) {
            upnp_thread->interrupt();
            upnp_thread->join();
            delete upnp_thread;
        }
        upnp_thread = new boost::thread(boost::bind(&TraceThread<boost::function<void()> >, "upnp", &ThreadMapPort));
    }
    else if (upnp_thread) {
        upnp_thread->interrupt();
        upnp_thread->join();
        delete upnp_thread;
        upnp_thread = NULL;
    }
}

#else
void MapPort(bool)
{
    // Intentionally left blank.
}
#endif









// DNS seeds
// Each pair gives a source name and a seed name.
// The first name is used as information source for addrman.
// The second name should resolve to a list of seed addresses.
static const char *strMainNetDNSSeed[][2] = {
    {"litecointools.com", "dnsseed.litecointools.com"},
    {"litecoinpool.org", "dnsseed.litecoinpool.org"},
    {"xurious.com", "dnsseed.ltc.xurious.com"},
    {"koin-project.com", "dnsseed.koin-project.com"},
    {"weminemnc.com", "dnsseed.weminemnc.com"},
    {NULL, NULL}
};

static const char *strTestNetDNSSeed[][2] = {
    {"litecointools.com", "testnet-seed.litecointools.com"},
    {"xurious.com", "testnet-seed.ltc.xurious.com"},
    {"wemine-testnet.com", "dnsseed.wemine-testnet.com"},
    {NULL, NULL}
};

void ThreadDNSAddressSeed()
{
    static const char *(*strDNSSeed)[2] = fTestNet ? strTestNetDNSSeed : strMainNetDNSSeed;

    int found = 0;

    printf("Loading addresses from DNS seeds (could take a while)\n");

    for (unsigned int seed_idx = 0; strDNSSeed[seed_idx][0] != NULL; seed_idx++) {
        if (HaveNameProxy()) {
            AddOneShot(strDNSSeed[seed_idx][1]);
        } else {
            vector<CNetAddr> vaddr;
            vector<CAddress> vAdd;
            if (LookupHost(strDNSSeed[seed_idx][1], vaddr))
            {
                BOOST_FOREACH(CNetAddr& ip, vaddr)
                {
                    int nOneDay = 24*3600;
                    CAddress addr = CAddress(CService(ip, GetDefaultPort()));
                    addr.nTime = GetTime() - 3*nOneDay - GetRand(4*nOneDay); // use a random age between 3 and 7 days old
                    vAdd.push_back(addr);
                    found++;
                }
            }
            addrman.Add(vAdd, CNetAddr(strDNSSeed[seed_idx][0], true));
        }
    }

    printf("%d addresses found from DNS seeds\n", found);
}












unsigned int pnSeed[] =
{
    0x6ab5368e, 0x63109859, 0x6c47bb25, 0x87d888c1, 0x1d9b2e48, 0x63a6c545, 0xf3ca1a48, 0x23b6b242,
    0x8a67c7c6, 0x70a6f1c0, 0xc585c96d, 0x5cc4fd36, 0x239be836, 0x6660dc36, 0x2664cb36, 0xa602cd36,
    0xb83dfc36, 0x70a2fe18, 0x2c589d55, 0xf969ec2e, 0x702ab643, 0x6a97e655, 0xad9751ce, 0x57e5025a,
    0x9d065643, 0xe7b5ff60, 0x959d0d5a, 0x9667a318, 0xeba0c118, 0xb380d418, 0x170ccdde, 0x3548d9a2,
    0x3261ff53, 0x42df0676, 0x9d7445ae, 0x1db3e953, 0xafc0b1b7, 0x1648934b, 0x535ee644, 0x4b89b64f,
    0x56ed3060, 0x0810a15f, 0x03590ccf, 0x80752452, 0x0cc87162, 0xfec159d5, 0xcf0df618, 0x48865456,
    0xdec8e55a, 0x91e6ee5b, 0xc4739518, 0x24d45453, 0xd5e5be44, 0xdafcff57, 0x0e73ca18, 0x83c4d148,
    0x5a51ee53, 0x367e1c56, 0x2d651f54, 0x5eb4be43, 0x621589b2, 0x81dddedd, 0xd7499f56, 0xa48a694c,
    0xd408c762, 0x88d36144, 0x6891772e, 0x38e89c73, 0x8ce3a856, 0x52fa1fb0, 0xd9d5796d, 0x27318945,
    0x2e680452, 0x619bc445, 0xd0dc092e, 0x7192c962, 0xc2c9e405, 0xb9e3e747, 0xe459c9c1, 0x9d043b25,
    0xe2b235c6, 0xed2dcf18, 0xb3d03cae, 0x3d2dafb8, 0xd46a0e45, 0x4a32f1d8, 0x350be560, 0xab0a648a,
    0x87b6bc43, 0x6745629b, 0x72b9c362, 0x03a91c05, 0x7320bd3b, 0x32920b18, 0xbc2ca162, 0x216c4f7d,
    0x64151a4e, 0x4b63e20c, 0xe703c3be, 0xc2d5fab2, 0x2b28b762, 0xde47b0ba, 0xf2e19dd9, 0x99044fad,
    0x8252a5c0, 0x6e8be597, 0x4f0bb56c, 0xfaa52252, 0x8f764b52, 0xe5b21cad, 0x020cab4d, 0xc8e8f763,
    0x2eaacc6d, 0x2ce4dd4e, 0xfe27345c, 0xb21f09b0, 0x8b4199d9, 0x6b53c248, 0x42a8966d, 0xd5387145,
    0xc091c257, 0x2fe06751, 0xecf66552, 0x37dd0418, 0x8073c57c, 0x5471d54e, 0x56953360, 0x0d1ffcdf,
    0xb4554318, 0x16cb54c6, 0xa064d05e, 0x2b0fa93a, 0xec26d162, 0x9ca91b42, 0xdcfa4457, 0x1b5c0a7a,
    0x4561a17c, 0xcc52a662, 0xf9d47ad9, 0xfa418cb2, 0x88085618, 0x007aa55f, 0xfa2e4d4d, 0x8516a843,
    0x634ac658, 0x2c36f62e, 0x3f0cbe5e, 0xd0e87257, 0x37d5e6bc, 0x5941c1c7, 0x5ee001ae, 0xc2791e6c,
    0x0c6220b2, 0xab1ba258, 0x4719c076, 0xabb6a942, 0x6e588b52, 0x101a4352, 0x9604b362, 0x16607fc7,
    0x4a687d45, 0xc217feb2, 0xaebb6b18, 0x4439412e, 0x41330360, 0xca23946d, 0x1c699d76, 0xcd302344,
    0x4683afad, 0xad3c644f, 0x95ab4f05, 0xb34ec562, 0x57e3ff50, 0xcb950847, 0x743ced60, 0xb6734347,
    0x6237a2b2, 0x1517e244, 0xfd0b128b, 0xb7eb605f, 0x6a9717ad, 0x7e8d65ae, 0xdddc4cad, 0xc63530a6,
    0xfcd78559, 0xc779e25e, 0xf683de58, 0x7cd1ff50, 0x6e0931ad, 0x7c1b46ad, 0x9f4b4252, 0xf421482e,
    0xeca7e697, 0xfe466544, 0x46fa2240, 0xd33328bc, 0x7be6d7bd, 0xc205c27b, 0xa04c07d8, 0x5bcdb350,
    0x847bc74a, 0x14967f45, 0x59b341ad, 0x9ce4d783, 0xc38300c0, 0x4f1b4aad, 0x018daa32, 0x95d7f854,
    0x4bd58543, 0xfe8446ad, 0xc3288f18, 0xcda2e253, 0x662e4852, 0x9338e24e, 0xc2fd1218, 0xa7198252,
    0x481858b8, 0x55b74374, 0xbd458d5e, 0xb2cc6d54, 0x2ca11b4e, 0x62d1be58, 0xf0637fbc, 0xd9943950,
    0x0f6f4a47, 0x188a6144, 0x768ea0d8, 0xd2c31c54, 0x29462ed5, 0x2b546ad5, 0x8ba12d6c, 0x1a3f7255,
    0xa836e23c, 0xdcde0552, 0xda5b3945, 0x7e3720b2, 0x8c098018, 0x42cddd50, 0x0c52a65f, 0x8cce175e,
    0x96554242, 0x7309c779, 0xcb2537ca, 0xecfb70d9, 0x19c0b259, 0x7b559ed8, 0x2f03ae44, 0x1b2b0f05,
    0x9e72165e, 0x4ac7413e, 0xa22cbb6a, 0x9fc7624d, 0xbb101a54, 0xfdb0c23e, 0x84a65a47, 0x1efded62,
    0x1f47f554, 0xe5c16ad2, 0x76b62332, 0xe4613d46, 0xd74f6ec1, 0x3d0e2b32, 0x6725fe5e, 0xb62a8729,
    0x922e9925, 0x7efc2544, 0x12af60d4, 0x5e042f44, 0x5fcaff54, 0x144a3493, 0x5788ec47, 0x729175b2,
    0x493b316c, 0x949d4542, 0x4ffc8856, 0x18a95b61, 0xdaabc547, 0xc879d855, 0x7d8488c1, 0x3cb7b052,
    0xbe8ade6f, 0x6ef6a555, 0x737e5247, 0x7368e252, 0x5fbe1056, 0xb848e217, 0x44a44105, 0xb7610150,
    0x3c2eed62, 0x2b0c9e6d, 0x2a1698b4, 0x4e6fa443, 0xb6c3fe4d, 0xd86c8a43, 0x48b27125, 0x9d9c60cb,
    0x349a6d4c, 0xae336259, 0x81d7976d, 0x8e9fe2c0, 0x5f20c2dd, 0x6f289e52, 0xe954cf51, 0xc96eb248,
    0x1c29f562, 0xb01e3842, 0x119511b9, 0x4811856d, 0x172ff854, 0x7dab5451, 0x52520752, 0xc414ed52,
    0x368f4105, 0x7767b047, 0x1c459f32, 0x05b5e697, 0xb5273160, 0x99eaec69, 0x0cd7a951, 0x36e1f13e,
    0xee815148, 0x5773e8bc, 0xf61f35ad, 0xa0b903ae, 0x2d811660, 0x98aec3be, 0x851afa59, 0xce25a518,
    0xfcaaf2a2, 0x90bde760, 0x04cb16ad, 0x70a30432, 0xa6cd3e47, 0x0ccac672, 0xb238a018, 0xf7db8156,
    0xed1e0331, 0xe2590ccf, 0x0cde9e56, 0x5cc39744, 0x4e3c957c, 0xc57e67d3, 0x6b547662, 0x073cd243,
    0x670d9a18, 0x4666ae58, 0x661abd59, 0x64c94732, 0x8371cc82, 0x2f460843, 0x03bcfa45, 0xa23cac70,
    0x1fa47b46, 0xec86b4ad, 0x183e3105, 0x6dea2446, 0x0331bcd1, 0x2673cf18, 0x5ccbdaac, 0xa0483850,
    0x9b28c34a, 0xc69b9b62, 0xac944946, 0x3723b456, 0x4a53485d, 0x365fa8bc, 0x3adfbd5e, 0xdefbb143,
    0x6ca64746, 0xadea802e, 0x89c0bbad, 0xefcc854f, 0x38cca5bc, 0xacc61e5e, 0x5bc43ab8, 0x3003795b,
    0x391c2352, 0xe410f6ad, 0x3fa6f957, 0xd6b095d4, 0xeb00ba43, 0xf92f174c, 0xb45c0644, 0x6b299c53,
    0xff0cf172, 0xaafe4a4c, 0xeb4aea5c, 0xb61dae43, 0x22115347, 0xfd510e6c, 0xe037fc50, 0xa5498e3d,
    0xd36b8118, 0x0b6a0250, 0xd3260c5e, 0xe540424b, 0xbb630732, 0xa5cca46d, 0xdbb27a47, 0x5874ba6a,
    0x581d4342, 0xa68b7125, 0x0b3b9f58, 0x2b8a8d18, 0x3155e763, 0x455e6951, 0xe79c0260, 0x683a474b,
    0xfcabe697, 0xf3d1e13c, 0x8c0eb57c, 0x34a9e155, 0xc51d1e4c, 0x26fc8748, 0x3817ba6a, 0xbc36225f,
    0x8dc42e5a, 0x51676c4e, 0x0f77a843, 0xf45ebebc, 0x2dc05874, 0x9be1ac5f, 0x1471595c, 0xa6d260d8,
    0x3e2a008e, 0x0a2ff462, 0xfbe632c6, 0xf701af5d, 0x6cca155c, 0xf2e2af51, 0xb5c47a47, 0x322968d5,
    0xfe651b4e, 0x11bccecd, 0xc2140418, 0xa741a465, 0xc8f45f48, 0x0400b0b4, 0x0e36c14e, 0x1f028cb2,
    0x221db552, 0x4876ced5, 0x8e7c4abc, 0xd4b4734c, 0xce4b79d9, 0xf2879056, 0xa761794d, 0xc89c96b2,
    0xd042af44, 0xd5f63418, 0x744731ad, 0xa94e7cae, 0x52b85b4d, 0x01978932, 0xa6abd1d3, 0xd4fae247,
    0x71cd9344, 0xde60bb6a, 0xec358489, 0x6e054cdc, 0xc92728bc, 0x231ea8bc, 0x74a5a2bc, 0x6d6f135d,
    0x26dd6596, 0x9d5386d1, 0x268bf232, 0x3867324a, 0x315e5353, 0x1ed2a443, 0xd79a9d51, 0x39dfe550,
    0x3f1ef181, 0x2f65fe79, 0xf6caf04d, 0x3fd33644, 0xd9abf043, 0x1a84c94e, 0x61618445, 0x7bd81318,
    0xb4f8764c, 0x8c6ae963, 0x7d659c5c, 0x9f294257, 0x5711be58, 0x3ac6b247, 0xe4f07d60, 0xe6bf7162,
    0x96742752, 0x23329cca, 0xc219cd5c, 0xa6f23418, 0xf4908053, 0x63130652, 0xf68fc948, 0x028fb54b,
    0x83ec1a18, 0xe88b4e8b, 0xf057ffcf, 0x4b3efe60, 0x359fff60, 0xba675642, 0xd5fe664c, 0xc8a2b862,
    0x4f8f576d, 0xb7827462, 0x1cc0c954, 0x6655aa7c, 0x9bc71451, 0x1039b456, 0x91d5e37c, 0x1ccb6c53,
    0x2e36313e, 0x6afd5251, 0xee91107c, 0xe9c0844b, 0xf133e347, 0xc1cd1b41, 0x3a0ab052, 0x30dee25a,
    0xa75bd152, 0x319da1b8, 0xd8f97561, 0x2fa794d5, 0x92a70d6c, 0xd6b470d5, 0xb524c90e, 0x109a64ae,
    0x7038af89, 0x3205e418, 0x313a015e, 0xd2d3175e, 0xc07593b8, 0x179ee97a, 0x78fdec47, 0x775d6d3a,
    0xccc93a40, 0xdbbbd9ad, 0x803c6f12, 0xfd4bc14a, 0x8ed15250, 0x19c65958, 0xc8300c5e, 0x87757db2,
    0x82fe0444, 0x3ce0035e, 0x19f0e762, 0x160c4c57, 0x069b0418, 0x9c6e4356, 0xa10df12e, 0x1936654d,
    0xd30b2848, 0x464aa5b2, 0xbb0b1b55, 0x03f3d318, 0x0431e06c, 0x0d94f15b, 0xabc2ed47, 0xf23a00c2,
    0x3bee4954, 0xd1e72a52, 0xd464d054, 0xcf141118, 0xf57f4bc8, 0xd8b1ac6c, 0x136ea959, 0x63275d4a,
    0x391e8c18, 0xd4dd4832, 0x96610b6d, 0x9cfb5153, 0x4a80ee48, 0x345e7cbc, 0xde810b59, 0x134a4c90,
    0x4544ed5e, 0xdc801956, 0x9d6b3ac6, 0x0a037786, 0xdcaafd62, 0xd65ee05a, 0x158b754c, 0x53427dd5,
    0xa32774bc, 0xa22af3a2, 0x0c1f7157, 0xa0b58429, 0x349f3156, 0x9cd6a34d, 0x67eb1c56, 0x20b5e597,
    0xd40f7162, 0xa4cb0379, 0x0e1efc53, 0xe5c8e554, 0x3ebf8052, 0x6ee00847, 0x99206883, 0xc851767d
};

void DumpAddresses()
{
    int64 nStart = GetTimeMillis();

    CAddrDB adb;
    adb.Write(addrman);

    printf("Flushed %d addresses to peers.dat  %"PRI64d"ms\n",
           addrman.size(), GetTimeMillis() - nStart);
}

void static ProcessOneShot()
{
    string strDest;
    {
        LOCK(cs_vOneShots);
        if (vOneShots.empty())
            return;
        strDest = vOneShots.front();
        vOneShots.pop_front();
    }
    CAddress addr;
    CSemaphoreGrant grant(*semOutbound, true);
    if (grant) {
        if (!OpenNetworkConnection(addr, &grant, strDest.c_str(), true))
            AddOneShot(strDest);
    }
}

void ThreadOpenConnections()
{
    // Connect to specific addresses
    if (mapArgs.count("-connect") && mapMultiArgs["-connect"].size() > 0)
    {
        for (int64 nLoop = 0;; nLoop++)
        {
            ProcessOneShot();
            BOOST_FOREACH(string strAddr, mapMultiArgs["-connect"])
            {
                CAddress addr;
                OpenNetworkConnection(addr, NULL, strAddr.c_str());
                for (int i = 0; i < 10 && i < nLoop; i++)
                {
                    MilliSleep(500);
                }
            }
            MilliSleep(500);
        }
    }

    // Initiate network connections
    int64 nStart = GetTime();
    loop
    {
        ProcessOneShot();

        MilliSleep(500);

        CSemaphoreGrant grant(*semOutbound);
        boost::this_thread::interruption_point();

        // Add seed nodes if IRC isn't working
        if (addrman.size()==0 && (GetTime() - nStart > 60) && !fTestNet)
        {
            std::vector<CAddress> vAdd;
            for (unsigned int i = 0; i < ARRAYLEN(pnSeed); i++)
            {
                // It'll only connect to one or two seed nodes because once it connects,
                // it'll get a pile of addresses with newer timestamps.
                // Seed nodes are given a random 'last seen time' of between one and two
                // weeks ago.
                const int64 nOneWeek = 7*24*60*60;
                struct in_addr ip;
                memcpy(&ip, &pnSeed[i], sizeof(ip));
                CAddress addr(CService(ip, GetDefaultPort()));
                addr.nTime = GetTime()-GetRand(nOneWeek)-nOneWeek;
                vAdd.push_back(addr);
            }
            addrman.Add(vAdd, CNetAddr("127.0.0.1"));
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
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes) {
                if (!pnode->fInbound) {
                    setConnected.insert(pnode->addr.GetGroup());
                    nOutbound++;
                }
            }
        }

        int64 nANow = GetAdjustedTime();

        int nTries = 0;
        loop
        {
            // use an nUnkBias between 10 (no outgoing connections) and 90 (8 outgoing connections)
            CAddress addr = addrman.Select(10 + min(nOutbound,8)*10);

            // if we selected an invalid address, restart
            if (!addr.IsValid() || setConnected.count(addr.GetGroup()) || IsLocal(addr))
                break;

            // If we didn't find an appropriate destination after trying 100 addresses fetched from addrman,
            // stop this loop, and let the outer loop run again (which sleeps, adds seed nodes, recalculates
            // already-connected network ranges, ...) before trying new addrman addresses.
            nTries++;
            if (nTries > 100)
                break;

            if (IsLimited(addr))
                continue;

            // only consider very recently tried nodes after 30 failed attempts
            if (nANow - addr.nLastTry < 600 && nTries < 30)
                continue;

            // do not allow non-default ports, unless after 50 invalid addresses selected already
            if (addr.GetPort() != GetDefaultPort() && nTries < 50)
                continue;

            addrConnect = addr;
            break;
        }

        if (addrConnect.IsValid())
            OpenNetworkConnection(addrConnect, &grant);
    }
}

void ThreadOpenAddedConnections()
{
    {
        LOCK(cs_vAddedNodes);
        vAddedNodes = mapMultiArgs["-addnode"];
    }

    if (HaveNameProxy()) {
        while(true) {
            list<string> lAddresses(0);
            {
                LOCK(cs_vAddedNodes);
                BOOST_FOREACH(string& strAddNode, vAddedNodes)
                    lAddresses.push_back(strAddNode);
            }
            BOOST_FOREACH(string& strAddNode, lAddresses) {
                CAddress addr;
                CSemaphoreGrant grant(*semOutbound);
                OpenNetworkConnection(addr, &grant, strAddNode.c_str());
                MilliSleep(500);
            }
            MilliSleep(120000); // Retry every 2 minutes
        }
    }

    for (unsigned int i = 0; true; i++)
    {
        list<string> lAddresses(0);
        {
            LOCK(cs_vAddedNodes);
            BOOST_FOREACH(string& strAddNode, vAddedNodes)
                lAddresses.push_back(strAddNode);
        }

        list<vector<CService> > lservAddressesToAdd(0);
        BOOST_FOREACH(string& strAddNode, lAddresses)
        {
            vector<CService> vservNode(0);
            if(Lookup(strAddNode.c_str(), vservNode, GetDefaultPort(), fNameLookup, 0))
            {
                lservAddressesToAdd.push_back(vservNode);
                {
                    LOCK(cs_setservAddNodeAddresses);
                    BOOST_FOREACH(CService& serv, vservNode)
                        setservAddNodeAddresses.insert(serv);
                }
            }
        }
        // Attempt to connect to each IP for each addnode entry until at least one is successful per addnode entry
        // (keeping in mind that addnode entries can have many IPs if fNameLookup)
        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
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
            CSemaphoreGrant grant(*semOutbound);
            OpenNetworkConnection(CAddress(vserv[i % vserv.size()]), &grant);
            MilliSleep(500);
        }
        MilliSleep(120000); // Retry every 2 minutes
    }
}

// if successful, this moves the passed grant to the constructed node
bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant *grantOutbound, const char *strDest, bool fOneShot)
{
    //
    // Initiate outbound network connection
    //
    boost::this_thread::interruption_point();
    if (!strDest)
        if (IsLocal(addrConnect) ||
            FindNode((CNetAddr)addrConnect) || CNode::IsBanned(addrConnect) ||
            FindNode(addrConnect.ToStringIPPort().c_str()))
            return false;
    if (strDest && FindNode(strDest))
        return false;

    CNode* pnode = ConnectNode(addrConnect, strDest);
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
double static NodeSyncScore(const CNode *pnode) {
    return -pnode->nLastRecv;
}

void static StartSync(const vector<CNode*> &vNodes) {
    CNode *pnodeNewSync = NULL;
    double dBestScore = 0;

    // fImporting and fReindex are accessed out of cs_main here, but only
    // as an optimization - they are checked again in SendMessages.
    if (fImporting || fReindex)
        return;

    // Iterate over all nodes
    BOOST_FOREACH(CNode* pnode, vNodes) {
        // check preconditions for allowing a sync
        if (!pnode->fClient && !pnode->fOneShot &&
            !pnode->fDisconnect && pnode->fSuccessfullyConnected &&
            (pnode->nStartingHeight > (nBestHeight - 144)) &&
            (pnode->nVersion < NOBLKS_VERSION_START || pnode->nVersion >= NOBLKS_VERSION_END)) {
            // if ok, compare node's score with the best so far
            double dScore = NodeSyncScore(pnode);
            if (pnodeNewSync == NULL || dScore > dBestScore) {
                pnodeNewSync = pnode;
                dBestScore = dScore;
            }
        }
    }
    // if a new sync candidate was found, start sync!
    if (pnodeNewSync) {
        pnodeNewSync->fStartSync = true;
        pnodeSync = pnodeNewSync;
    }
}

void ThreadMessageHandler()
{
    SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);
    while (true)
    {
        bool fHaveSyncNode = false;

        vector<CNode*> vNodesCopy;
        {
            LOCK(cs_vNodes);
            vNodesCopy = vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy) {
                pnode->AddRef();
                if (pnode == pnodeSync)
                    fHaveSyncNode = true;
            }
        }

        if (!fHaveSyncNode)
            StartSync(vNodesCopy);

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
                    if (!ProcessMessages(pnode))
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
                    SendMessages(pnode, pnode == pnodeTrickle);
            }
            boost::this_thread::interruption_point();
        }

        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->Release();
        }

        if (fSleep)
            MilliSleep(100);
    }
}






bool BindListenPort(const CService &addrBind, string& strError)
{
    strError = "";
    int nOne = 1;

    // Create socket for listening for incoming connections
#ifdef USE_IPV6
    struct sockaddr_storage sockaddr;
#else
    struct sockaddr sockaddr;
#endif
    socklen_t len = sizeof(sockaddr);
    if (!addrBind.GetSockAddr((struct sockaddr*)&sockaddr, &len))
    {
        strError = strprintf("Error: bind address family for %s not supported", addrBind.ToString().c_str());
        printf("%s\n", strError.c_str());
        return false;
    }

    SOCKET hListenSocket = socket(((struct sockaddr*)&sockaddr)->sa_family, SOCK_STREAM, IPPROTO_TCP);
    if (hListenSocket == INVALID_SOCKET)
    {
        strError = strprintf("Error: Couldn't open socket for incoming connections (socket returned error %d)", WSAGetLastError());
        printf("%s\n", strError.c_str());
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
        strError = strprintf("Error: Couldn't set properties on socket for incoming connections (error %d)", WSAGetLastError());
        printf("%s\n", strError.c_str());
        return false;
    }

#ifdef USE_IPV6
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
#endif

    if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
    {
        int nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE)
            strError = strprintf(_("Unable to bind to %s on this computer. Litecoin is probably already running."), addrBind.ToString().c_str());
        else
            strError = strprintf(_("Unable to bind to %s on this computer (bind returned error %d, %s)"), addrBind.ToString().c_str(), nErr, strerror(nErr));
        printf("%s\n", strError.c_str());
        return false;
    }
    printf("Bound to %s\n", addrBind.ToString().c_str());

    // Listen for incoming connections
    if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        strError = strprintf("Error: Listening for incoming connections failed (listen returned error %d)", WSAGetLastError());
        printf("%s\n", strError.c_str());
        return false;
    }

    vhListenSocket.push_back(hListenSocket);

    if (addrBind.IsRoutable() && fDiscover)
        AddLocal(addrBind, LOCAL_BIND);

    return true;
}

void static Discover()
{
    if (!fDiscover)
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
                AddLocal(addr, LOCAL_IF);
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
                if (AddLocal(addr, LOCAL_IF))
                    printf("IPv4 %s: %s\n", ifa->ifa_name, addr.ToString().c_str());
            }
#ifdef USE_IPV6
            else if (ifa->ifa_addr->sa_family == AF_INET6)
            {
                struct sockaddr_in6* s6 = (struct sockaddr_in6*)(ifa->ifa_addr);
                CNetAddr addr(s6->sin6_addr);
                if (AddLocal(addr, LOCAL_IF))
                    printf("IPv6 %s: %s\n", ifa->ifa_name, addr.ToString().c_str());
            }
#endif
        }
        freeifaddrs(myaddrs);
    }
#endif

    // Don't use external IPv4 discovery, when -onlynet="IPv6"
    if (!IsLimited(NET_IPV4))
        NewThread(ThreadGetMyExternalIP, NULL);
}

void StartNode(boost::thread_group& threadGroup)
{
    if (semOutbound == NULL) {
        // initialize semaphore
        int nMaxOutbound = min(MAX_OUTBOUND_CONNECTIONS, nMaxConnections);
        semOutbound = new CSemaphore(nMaxOutbound);
    }

    if (pnodeLocalHost == NULL)
        pnodeLocalHost = new CNode(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), nLocalServices));

    Discover();

    //
    // Start threads
    //

    if (!GetBoolArg("-dnsseed", true))
        printf("DNS seeding disabled\n");
    else
        threadGroup.create_thread(boost::bind(&TraceThread<boost::function<void()> >, "dnsseed", &ThreadDNSAddressSeed));

#ifdef USE_UPNP
    // Map ports with UPnP
    MapPort(GetBoolArg("-upnp", USE_UPNP));
#endif

    // Send and receive from sockets, accept connections
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "net", &ThreadSocketHandler));

    // Initiate outbound connections from -addnode
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "addcon", &ThreadOpenAddedConnections));

    // Initiate outbound connections
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "opencon", &ThreadOpenConnections));

    // Process messages
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "msghand", &ThreadMessageHandler));

    // Dump network addresses
    threadGroup.create_thread(boost::bind(&LoopForever<void (*)()>, "dumpaddr", &DumpAddresses, DUMP_ADDRESSES_INTERVAL * 1000));
}

bool StopNode()
{
    printf("StopNode()\n");
    GenerateBitcoins(false, NULL);
    MapPort(false);
    nTransactionsUpdated++;
    if (semOutbound)
        for (int i=0; i<MAX_OUTBOUND_CONNECTIONS; i++)
            semOutbound->post();
    MilliSleep(50);
    DumpAddresses();

    return true;
}

class CNetCleanup
{
public:
    CNetCleanup()
    {
    }
    ~CNetCleanup()
    {
        // Close sockets
        BOOST_FOREACH(CNode* pnode, vNodes)
            if (pnode->hSocket != INVALID_SOCKET)
                closesocket(pnode->hSocket);
        BOOST_FOREACH(SOCKET hListenSocket, vhListenSocket)
            if (hListenSocket != INVALID_SOCKET)
                if (closesocket(hListenSocket) == SOCKET_ERROR)
                    printf("closesocket(hListenSocket) failed with error %d\n", WSAGetLastError());

        // clean up some globals (to help leak detection)
        BOOST_FOREACH(CNode *pnode, vNodes)
            delete pnode;
        BOOST_FOREACH(CNode *pnode, vNodesDisconnected)
            delete pnode;
        vNodes.clear();
        vNodesDisconnected.clear();
        delete semOutbound;
        semOutbound = NULL;
        delete pnodeLocalHost;
        pnodeLocalHost = NULL;

#ifdef WIN32
        // Shutdown Windows Sockets
        WSACleanup();
#endif
    }
}
instance_of_cnetcleanup;







void RelayTransaction(const CTransaction& tx, const uint256& hash)
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss.reserve(10000);
    ss << tx;
    RelayTransaction(tx, hash, ss);
}

void RelayTransaction(const CTransaction& tx, const uint256& hash, const CDataStream& ss)
{
    CInv inv(MSG_TX, hash);
    {
        LOCK(cs_mapRelay);
        // Expire old relay messages
        while (!vRelayExpiration.empty() && vRelayExpiration.front().first < GetTime())
        {
            mapRelay.erase(vRelayExpiration.front().second);
            vRelayExpiration.pop_front();
        }

        // Save original serialized message so newer versions are preserved
        mapRelay.insert(std::make_pair(inv, ss));
        vRelayExpiration.push_back(std::make_pair(GetTime() + 15 * 60, inv));
    }
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if(!pnode->fRelayTxes)
            continue;
        LOCK(pnode->cs_filter);
        if (pnode->pfilter)
        {
            if (pnode->pfilter->IsRelevantAndUpdate(tx, hash))
                pnode->PushInventory(inv);
        } else
            pnode->PushInventory(inv);
    }
}
