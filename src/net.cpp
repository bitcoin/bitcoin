// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#include "irc.h"
#include "db.h"
#include "net.h"
#include "init.h"
#include "strlcpy.h"

#ifdef WIN32
#include <string.h>
#endif

#ifdef USE_UPNP
#include <miniupnpc/miniwget.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
#endif

using namespace std;
using namespace boost;

static const int MAX_OUTBOUND_CONNECTIONS = 8;

void ThreadMessageHandler2(void* parg);
void ThreadSocketHandler2(void* parg);
void ThreadOpenConnections2(void* parg);
void ThreadOpenAddedConnections2(void* parg);
#ifdef USE_UPNP
void ThreadMapPort2(void* parg);
#endif
void ThreadDNSAddressSeed2(void* parg);
bool OpenNetworkConnection(const CAddress& addrConnect);





//
// Global state variables
//
bool fClient = false;
bool fAllowDNS = false;
uint64 nLocalServices = (fClient ? 0 : NODE_NETWORK);
CAddress addrLocalHost(CService("0.0.0.0", 0), nLocalServices);
static CNode* pnodeLocalHost = NULL;
uint64 nLocalHostNonce = 0;
array<int, 10> vnThreadsRunning;
static SOCKET hListenSocket = INVALID_SOCKET;

vector<CNode*> vNodes;
CCriticalSection cs_vNodes;
map<vector<unsigned char>, CAddress> mapAddresses;
CCriticalSection cs_mapAddresses;
map<CInv, CDataStream> mapRelay;
deque<pair<int64, CInv> > vRelayExpiration;
CCriticalSection cs_mapRelay;
map<CInv, int64> mapAlreadyAskedFor;


set<CNetAddr> setservAddNodeAddresses;
CCriticalSection cs_setservAddNodeAddresses;




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
                if (strLine.find(pszKeyword) != -1)
                {
                    strLine = strLine.substr(strLine.find(pszKeyword) + strlen(pszKeyword));
                    break;
                }
            }
            closesocket(hSocket);
            if (strLine.find("<") != -1)
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

// We now get our external IP from the IRC server first and only use this as a backup
bool GetMyExternalIP(CNetAddr& ipRet)
{
    CService addrConnect;
    const char* pszGet;
    const char* pszKeyword;

    if (fUseProxy)
        return false;

    for (int nLookup = 0; nLookup <= 1; nLookup++)
    for (int nHost = 1; nHost <= 2; nHost++)
    {
        // We should be phasing out our use of sites like these.  If we need
        // replacements, we should ask for volunteers to put this simple
        // php file on their webserver that prints the client IP:
        //  <?php echo $_SERVER["REMOTE_ADDR"]; ?>
        if (nHost == 1)
        {
            addrConnect = CService("91.198.22.70",80); // checkip.dyndns.org

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
    // Wait for IRC to get it first
    if (!GetBoolArg("-noirc"))
    {
        for (int i = 0; i < 2 * 60; i++)
        {
            Sleep(1000);
            if (fGotExternalIP || fShutdown)
                return;
        }
    }

    // Fallback in case IRC fails to get it
    if (GetMyExternalIP(addrLocalHost))
    {
        printf("GetMyExternalIP() returned %s\n", addrLocalHost.ToStringIP().c_str());
        if (addrLocalHost.IsRoutable())
        {
            // If we already connected to a few before we had our IP, go back and addr them.
            // setAddrKnown automatically filters any duplicate sends.
            CAddress addr(addrLocalHost);
            addr.nTime = GetAdjustedTime();
            CRITICAL_BLOCK(cs_vNodes)
                BOOST_FOREACH(CNode* pnode, vNodes)
                    pnode->PushAddress(addr);
        }
    }
}





bool AddAddress(CAddress addr, int64 nTimePenalty, CAddrDB *pAddrDB)
{
    if (!addr.IsRoutable())
        return false;
    if ((CService)addr == (CService)addrLocalHost)
        return false;
    addr.nTime = max((int64)0, (int64)addr.nTime - nTimePenalty);
    bool fUpdated = false;
    bool fNew = false;
    CAddress addrFound = addr;

    CRITICAL_BLOCK(cs_mapAddresses)
    {
        map<vector<unsigned char>, CAddress>::iterator it = mapAddresses.find(addr.GetKey());
        if (it == mapAddresses.end())
        {
            // New address
            printf("AddAddress(%s)\n", addr.ToString().c_str());
            mapAddresses.insert(make_pair(addr.GetKey(), addr));
            fUpdated = true;
            fNew = true;
        }
        else
        {
            addrFound = (*it).second;
            if ((addrFound.nServices | addr.nServices) != addrFound.nServices)
            {
                // Services have been added
                addrFound.nServices |= addr.nServices;
                fUpdated = true;
            }
            bool fCurrentlyOnline = (GetAdjustedTime() - addr.nTime < 24 * 60 * 60);
            int64 nUpdateInterval = (fCurrentlyOnline ? 60 * 60 : 24 * 60 * 60);
            if (addrFound.nTime < addr.nTime - nUpdateInterval)
            {
                // Periodically update most recently seen time
                addrFound.nTime = addr.nTime;
                fUpdated = true;
            }
        }
    }
    // There is a nasty deadlock bug if this is done inside the cs_mapAddresses
    // CRITICAL_BLOCK:
    // Thread 1:  begin db transaction (locks inside-db-mutex)
    //            then AddAddress (locks cs_mapAddresses)
    // Thread 2:  AddAddress (locks cs_mapAddresses)
    //             ... then db operation hangs waiting for inside-db-mutex
    if (fUpdated)
    {
        if (pAddrDB)
            pAddrDB->WriteAddress(addrFound);
        else
            CAddrDB().WriteAddress(addrFound);
    }
    return fNew;
}

void AddressCurrentlyConnected(const CService& addr)
{
    CAddress *paddrFound = NULL;

    CRITICAL_BLOCK(cs_mapAddresses)
    {
        // Only if it's been published already
        map<vector<unsigned char>, CAddress>::iterator it = mapAddresses.find(addr.GetKey());
        if (it != mapAddresses.end())
            paddrFound = &(*it).second;
    }

    if (paddrFound)
    {
        int64 nUpdateInterval = 20 * 60;
        if (paddrFound->nTime < GetAdjustedTime() - nUpdateInterval)
        {
            // Periodically update most recently seen time
            paddrFound->nTime = GetAdjustedTime();
            CAddrDB addrdb;
            addrdb.WriteAddress(*paddrFound);
        }
    }
}





void AbandonRequests(void (*fn)(void*, CDataStream&), void* param1)
{
    // If the dialog might get closed before the reply comes back,
    // call this in the destructor so it doesn't get called after it's deleted.
    CRITICAL_BLOCK(cs_vNodes)
    {
        BOOST_FOREACH(CNode* pnode, vNodes)
        {
            CRITICAL_BLOCK(pnode->cs_mapRequests)
            {
                for (map<uint256, CRequestTracker>::iterator mi = pnode->mapRequests.begin(); mi != pnode->mapRequests.end();)
                {
                    CRequestTracker& tracker = (*mi).second;
                    if (tracker.fn == fn && tracker.param1 == param1)
                        pnode->mapRequests.erase(mi++);
                    else
                        mi++;
                }
            }
        }
    }
}







//
// Subscription methods for the broadcast and subscription system.
// Channel numbers are message numbers, i.e. MSG_TABLE and MSG_PRODUCT.
//
// The subscription system uses a meet-in-the-middle strategy.
// With 100,000 nodes, if senders broadcast to 1000 random nodes and receivers
// subscribe to 1000 random nodes, 99.995% (1 - 0.99^1000) of messages will get through.
//

bool AnySubscribed(unsigned int nChannel)
{
    if (pnodeLocalHost->IsSubscribed(nChannel))
        return true;
    CRITICAL_BLOCK(cs_vNodes)
        BOOST_FOREACH(CNode* pnode, vNodes)
            if (pnode->IsSubscribed(nChannel))
                return true;
    return false;
}

bool CNode::IsSubscribed(unsigned int nChannel)
{
    if (nChannel >= vfSubscribe.size())
        return false;
    return vfSubscribe[nChannel];
}

void CNode::Subscribe(unsigned int nChannel, unsigned int nHops)
{
    if (nChannel >= vfSubscribe.size())
        return;

    if (!AnySubscribed(nChannel))
    {
        // Relay subscribe
        CRITICAL_BLOCK(cs_vNodes)
            BOOST_FOREACH(CNode* pnode, vNodes)
                if (pnode != this)
                    pnode->PushMessage("subscribe", nChannel, nHops);
    }

    vfSubscribe[nChannel] = true;
}

void CNode::CancelSubscribe(unsigned int nChannel)
{
    if (nChannel >= vfSubscribe.size())
        return;

    // Prevent from relaying cancel if wasn't subscribed
    if (!vfSubscribe[nChannel])
        return;
    vfSubscribe[nChannel] = false;

    if (!AnySubscribed(nChannel))
    {
        // Relay subscription cancel
        CRITICAL_BLOCK(cs_vNodes)
            BOOST_FOREACH(CNode* pnode, vNodes)
                if (pnode != this)
                    pnode->PushMessage("sub-cancel", nChannel);
    }
}









CNode* FindNode(const CNetAddr& ip)
{
    CRITICAL_BLOCK(cs_vNodes)
    {
        BOOST_FOREACH(CNode* pnode, vNodes)
            if ((CNetAddr)pnode->addr == ip)
                return (pnode);
    }
    return NULL;
}

CNode* FindNode(const CService& addr)
{
    CRITICAL_BLOCK(cs_vNodes)
    {
        BOOST_FOREACH(CNode* pnode, vNodes)
            if ((CService)pnode->addr == addr)
                return (pnode);
    }
    return NULL;
}

CNode* ConnectNode(CAddress addrConnect, int64 nTimeout)
{
    if ((CNetAddr)addrConnect == (CNetAddr)addrLocalHost)
        return NULL;

    // Look for an existing connection
    CNode* pnode = FindNode((CService)addrConnect);
    if (pnode)
    {
        if (nTimeout != 0)
            pnode->AddRef(nTimeout);
        else
            pnode->AddRef();
        return pnode;
    }

    /// debug print
    printf("trying connection %s lastseen=%.1fhrs lasttry=%.1fhrs\n",
        addrConnect.ToString().c_str(),
        (double)(addrConnect.nTime - GetAdjustedTime())/3600.0,
        (double)(addrConnect.nLastTry - GetAdjustedTime())/3600.0);

    CRITICAL_BLOCK(cs_mapAddresses)
        mapAddresses[addrConnect.GetKey()].nLastTry = GetAdjustedTime();

    // Connect
    SOCKET hSocket;
    if (ConnectSocket(addrConnect, hSocket))
    {
        /// debug print
        printf("connected %s\n", addrConnect.ToString().c_str());

        // Set to nonblocking
#ifdef WIN32
        u_long nOne = 1;
        if (ioctlsocket(hSocket, FIONBIO, &nOne) == SOCKET_ERROR)
            printf("ConnectSocket() : ioctlsocket nonblocking setting failed, error %d\n", WSAGetLastError());
#else
        if (fcntl(hSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
            printf("ConnectSocket() : fcntl nonblocking setting failed, error %d\n", errno);
#endif

        // Add node
        CNode* pnode = new CNode(hSocket, addrConnect, false);
        if (nTimeout != 0)
            pnode->AddRef(nTimeout);
        else
            pnode->AddRef();
        CRITICAL_BLOCK(cs_vNodes)
            vNodes.push_back(pnode);

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
        if (fDebug)
            printf("%s ", DateTimeStrFormat("%x %H:%M:%S", GetTime()).c_str());
        printf("disconnecting node %s\n", addr.ToString().c_str());
        closesocket(hSocket);
        hSocket = INVALID_SOCKET;
    }
}

void CNode::Cleanup()
{
    // All of a nodes broadcasts and subscriptions are automatically torn down
    // when it goes down, so a node has to stay up to keep its broadcast going.

    // Cancel subscriptions
    for (unsigned int nChannel = 0; nChannel < vfSubscribe.size(); nChannel++)
        if (vfSubscribe[nChannel])
            CancelSubscribe(nChannel);
}


void CNode::PushVersion()
{
    /// when NTP implemented, change to just nTime = GetAdjustedTime()
    int64 nTime = (fInbound ? GetAdjustedTime() : GetTime());
    CAddress addrYou = (fUseProxy ? CAddress(CService("0.0.0.0",0)) : addr);
    CAddress addrMe = (fUseProxy ? CAddress(CService("0.0.0.0",0)) : addrLocalHost);
    RAND_bytes((unsigned char*)&nLocalHostNonce, sizeof(nLocalHostNonce));
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
    CRITICAL_BLOCK(cs_setBanned)
    {
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
        printf("Warning: local node %s misbehaving\n", addr.ToString().c_str());
        return false;
    }

    nMisbehavior += howmuch;
    if (nMisbehavior >= GetArg("-banscore", 100))
    {
        int64 banTime = GetTime()+GetArg("-bantime", 60*60*24);  // Default 24-hour ban
        CRITICAL_BLOCK(cs_setBanned)
            if (setBanned[addr] < banTime)
                setBanned[addr] = banTime;
        CloseSocketDisconnect();
        printf("Disconnected %s for misbehavior (score=%d)\n", addr.ToString().c_str(), nMisbehavior);
        return true;
    }
    return false;
}












void ThreadSocketHandler(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadSocketHandler(parg));
    try
    {
        vnThreadsRunning[0]++;
        ThreadSocketHandler2(parg);
        vnThreadsRunning[0]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[0]--;
        PrintException(&e, "ThreadSocketHandler()");
    } catch (...) {
        vnThreadsRunning[0]--;
        throw; // support pthread_cancel()
    }
    printf("ThreadSocketHandler exiting\n");
}

void ThreadSocketHandler2(void* parg)
{
    printf("ThreadSocketHandler started\n");
    list<CNode*> vNodesDisconnected;
    int nPrevNodeCount = 0;

    loop
    {
        //
        // Disconnect nodes
        //
        CRITICAL_BLOCK(cs_vNodes)
        {
            // Disconnect unused nodes
            vector<CNode*> vNodesCopy = vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
            {
                if (pnode->fDisconnect ||
                    (pnode->GetRefCount() <= 0 && pnode->vRecv.empty() && pnode->vSend.empty()))
                {
                    // remove from vNodes
                    vNodes.erase(remove(vNodes.begin(), vNodes.end(), pnode), vNodes.end());

                    // close socket and cleanup
                    pnode->CloseSocketDisconnect();
                    pnode->Cleanup();

                    // hold in disconnected pool until all refs are released
                    pnode->nReleaseTime = max(pnode->nReleaseTime, GetTime() + 15 * 60);
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
                    TRY_CRITICAL_BLOCK(pnode->cs_vSend)
                     TRY_CRITICAL_BLOCK(pnode->cs_vRecv)
                      TRY_CRITICAL_BLOCK(pnode->cs_mapRequests)
                       TRY_CRITICAL_BLOCK(pnode->cs_inventory)
                        fDelete = true;
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
            MainFrameRepaint();
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

        if(hListenSocket != INVALID_SOCKET)
            FD_SET(hListenSocket, &fdsetRecv);
        hSocketMax = max(hSocketMax, hListenSocket);
        CRITICAL_BLOCK(cs_vNodes)
        {
            BOOST_FOREACH(CNode* pnode, vNodes)
            {
                if (pnode->hSocket == INVALID_SOCKET)
                    continue;
                FD_SET(pnode->hSocket, &fdsetRecv);
                FD_SET(pnode->hSocket, &fdsetError);
                hSocketMax = max(hSocketMax, pnode->hSocket);
                TRY_CRITICAL_BLOCK(pnode->cs_vSend)
                    if (!pnode->vSend.empty())
                        FD_SET(pnode->hSocket, &fdsetSend);
            }
        }

        vnThreadsRunning[0]--;
        int nSelect = select(hSocketMax + 1, &fdsetRecv, &fdsetSend, &fdsetError, &timeout);
        vnThreadsRunning[0]++;
        if (fShutdown)
            return;
        if (nSelect == SOCKET_ERROR)
        {
            int nErr = WSAGetLastError();
            if (hSocketMax > -1)
            {
                printf("socket select error %d\n", nErr);
                for (int i = 0; i <= hSocketMax; i++)
                    FD_SET(i, &fdsetRecv);
            }
            FD_ZERO(&fdsetSend);
            FD_ZERO(&fdsetError);
            Sleep(timeout.tv_usec/1000);
        }


        //
        // Accept new connections
        //
        if (hListenSocket != INVALID_SOCKET && FD_ISSET(hListenSocket, &fdsetRecv))
        {
            struct sockaddr_in sockaddr;
            socklen_t len = sizeof(sockaddr);
            SOCKET hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);
            CAddress addr(sockaddr);
            int nInbound = 0;

            CRITICAL_BLOCK(cs_vNodes)
                BOOST_FOREACH(CNode* pnode, vNodes)
                if (pnode->fInbound)
                    nInbound++;
            if (hSocket == INVALID_SOCKET)
            {
                if (WSAGetLastError() != WSAEWOULDBLOCK)
                    printf("socket error accept failed: %d\n", WSAGetLastError());
            }
            else if (nInbound >= GetArg("-maxconnections", 125) - MAX_OUTBOUND_CONNECTIONS)
            {
                CRITICAL_BLOCK(cs_setservAddNodeAddresses)
                    if (!setservAddNodeAddresses.count(addr))
                        closesocket(hSocket);
            }
            else if (CNode::IsBanned(addr))
            {
                printf("connetion from %s dropped (banned)\n", addr.ToString().c_str());
                closesocket(hSocket);
            }
            else
            {
                printf("accepted connection %s\n", addr.ToString().c_str());
                CNode* pnode = new CNode(hSocket, addr, true);
                pnode->AddRef();
                CRITICAL_BLOCK(cs_vNodes)
                    vNodes.push_back(pnode);
            }
        }


        //
        // Service each socket
        //
        vector<CNode*> vNodesCopy;
        CRITICAL_BLOCK(cs_vNodes)
        {
            vNodesCopy = vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->AddRef();
        }
        BOOST_FOREACH(CNode* pnode, vNodesCopy)
        {
            if (fShutdown)
                return;

            //
            // Receive
            //
            if (pnode->hSocket == INVALID_SOCKET)
                continue;
            if (FD_ISSET(pnode->hSocket, &fdsetRecv) || FD_ISSET(pnode->hSocket, &fdsetError))
            {
                TRY_CRITICAL_BLOCK(pnode->cs_vRecv)
                {
                    CDataStream& vRecv = pnode->vRecv;
                    unsigned int nPos = vRecv.size();

                    if (nPos > ReceiveBufferSize()) {
                        if (!pnode->fDisconnect)
                            printf("socket recv flood control disconnect (%d bytes)\n", vRecv.size());
                        pnode->CloseSocketDisconnect();
                    }
                    else {
                        // typical socket buffer is 8K-64K
                        char pchBuf[0x10000];
                        int nBytes = recv(pnode->hSocket, pchBuf, sizeof(pchBuf), MSG_DONTWAIT);
                        if (nBytes > 0)
                        {
                            vRecv.resize(nPos + nBytes);
                            memcpy(&vRecv[nPos], pchBuf, nBytes);
                            pnode->nLastRecv = GetTime();
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
                TRY_CRITICAL_BLOCK(pnode->cs_vSend)
                {
                    CDataStream& vSend = pnode->vSend;
                    if (!vSend.empty())
                    {
                        int nBytes = send(pnode->hSocket, &vSend[0], vSend.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
                        if (nBytes > 0)
                        {
                            vSend.erase(vSend.begin(), vSend.begin() + nBytes);
                            pnode->nLastSend = GetTime();
                        }
                        else if (nBytes < 0)
                        {
                            // error
                            int nErr = WSAGetLastError();
                            if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
                            {
                                printf("socket send error %d\n", nErr);
                                pnode->CloseSocketDisconnect();
                            }
                        }
                        if (vSend.size() > SendBufferSize()) {
                            if (!pnode->fDisconnect)
                                printf("socket send flood control disconnect (%d bytes)\n", vSend.size());
                            pnode->CloseSocketDisconnect();
                        }
                    }
                }
            }

            //
            // Inactivity checking
            //
            if (pnode->vSend.empty())
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
        CRITICAL_BLOCK(cs_vNodes)
        {
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->Release();
        }

        Sleep(10);
    }
}









#ifdef USE_UPNP
void ThreadMapPort(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadMapPort(parg));
    try
    {
        vnThreadsRunning[5]++;
        ThreadMapPort2(parg);
        vnThreadsRunning[5]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[5]--;
        PrintException(&e, "ThreadMapPort()");
    } catch (...) {
        vnThreadsRunning[5]--;
        PrintException(NULL, "ThreadMapPort()");
    }
    printf("ThreadMapPort exiting\n");
}

void ThreadMapPort2(void* parg)
{
    printf("ThreadMapPort started\n");

    char port[6];
    sprintf(port, "%d", GetListenPort());

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
        string strDesc = "Bitcoin " + FormatFullVersion();
#ifndef UPNPDISCOVER_SUCCESS
    /* miniupnpc 1.5 */
        r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                            port, port, lanaddr, strDesc.c_str(), "TCP", 0);
#else
    /* miniupnpc 1.6 */
        r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                            port, port, lanaddr, strDesc.c_str(), "TCP", 0, "0");
#endif

        if(r!=UPNPCOMMAND_SUCCESS)
            printf("AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
                port, port, lanaddr, r, strupnperror(r));
        else
            printf("UPnP Port Mapping successful.\n");
        loop {
            if (fShutdown || !fUseUPnP)
            {
                r = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port, "TCP", 0);
                printf("UPNP_DeletePortMapping() returned : %d\n", r);
                freeUPNPDevlist(devlist); devlist = 0;
                FreeUPNPUrls(&urls);
                return;
            }
            Sleep(2000);
        }
    } else {
        printf("No valid UPnP IGDs found\n");
        freeUPNPDevlist(devlist); devlist = 0;
        if (r != 0)
            FreeUPNPUrls(&urls);
        loop {
            if (fShutdown || !fUseUPnP)
                return;
            Sleep(2000);
        }
    }
}

void MapPort(bool fMapPort)
{
    if (fUseUPnP != fMapPort)
    {
        fUseUPnP = fMapPort;
        WriteSetting("fUseUPnP", fUseUPnP);
    }
    if (fUseUPnP && vnThreadsRunning[5] < 1)
    {
        if (!CreateThread(ThreadMapPort, NULL))
            printf("Error: ThreadMapPort(ThreadMapPort) failed\n");
    }
}
#else
void MapPort(bool /* unused fMapPort */)
{
    // Intentionally left blank.
}
#endif










static const char *strDNSSeed[] = {
    "bitseed.xf2.org",
    "dnsseed.bluematt.me",
    "seed.bitcoin.sipa.be",
    "dnsseed.bitcoin.dashjr.org",
};

void ThreadDNSAddressSeed(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadDNSAddressSeed(parg));
    try
    {
        vnThreadsRunning[6]++;
        ThreadDNSAddressSeed2(parg);
        vnThreadsRunning[6]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[6]--;
        PrintException(&e, "ThreadDNSAddressSeed()");
    } catch (...) {
        vnThreadsRunning[6]--;
        throw; // support pthread_cancel()
    }
    printf("ThreadDNSAddressSeed exiting\n");
}

void ThreadDNSAddressSeed2(void* parg)
{
    printf("ThreadDNSAddressSeed started\n");
    int found = 0;

    if (!fTestNet)
    {
        printf("Loading addresses from DNS seeds (could take a while)\n");

        for (int seed_idx = 0; seed_idx < ARRAYLEN(strDNSSeed); seed_idx++) {
            vector<CNetAddr> vaddr;
            if (LookupHost(strDNSSeed[seed_idx], vaddr))
            {
                CAddrDB addrDB;
                addrDB.TxnBegin();
                BOOST_FOREACH (CNetAddr& ip, vaddr)
                {
                    if (ip.IsRoutable())
                    {
                        CAddress addr(CService(ip, GetDefaultPort()), NODE_NETWORK);
                        addr.nTime = 0;
                        AddAddress(addr, 0, &addrDB);
                        found++;
                    }
                }
                addrDB.TxnCommit();  // Save addresses (it's ok if this fails)
            }
        }
    }

    printf("%d addresses found from DNS seeds\n", found);
}












unsigned int pnSeed[] =
{
    0x959bd347, 0xf8de42b2, 0x73bc0518, 0xea6edc50, 0x21b00a4d, 0xc725b43d, 0xd665464d, 0x1a2a770e,
    0x27c93946, 0x65b2fa46, 0xb80ae255, 0x66b3b446, 0xb1877a3e, 0x6ee89e3e, 0xc3175b40, 0x2a01a83c,
    0x95b1363a, 0xa079ad3d, 0xe6ca801f, 0x027f4f4a, 0x34f7f03a, 0xf790f04a, 0x16ca801f, 0x2f4d5e40,
    0x3a4d5e40, 0xc43a322e, 0xc8159753, 0x14d4724c, 0x7919a118, 0xe0bdb34e, 0x68a16b2e, 0xff64b44d,
    0x6099115b, 0x9b57b05b, 0x7bd1b4ad, 0xdf95944f, 0x29d2b73d, 0xafa8db79, 0xe247ba41, 0x24078348,
    0xf722f03c, 0x33567ebc, 0xace64ed4, 0x984d3932, 0xb5f34e55, 0x27b7024d, 0x94579247, 0x8894042e,
    0x9357d34c, 0x1063c24b, 0xcaa228b1, 0xa3c5a8b2, 0x5dc64857, 0xa2c23643, 0xa8369a54, 0x31203077,
    0x00707c5c, 0x09fc0b3a, 0x272e9e2e, 0xf80f043e, 0x9449ca3e, 0x5512c33e, 0xd106b555, 0xe8024157,
    0xe288ec29, 0xc79c5461, 0xafb63932, 0xdb02ab4b, 0x0e512777, 0x8a145a4c, 0xb201ff4f, 0x5e09314b,
    0xcd9bfbcd, 0x1c023765, 0x4394e75c, 0xa728bd4d, 0x65331552, 0xa98420b1, 0x89ecf559, 0x6e80801f,
    0xf404f118, 0xefd62b51, 0x05918346, 0x9b186d5f, 0xacabab46, 0xf912e255, 0xc188ea62, 0xcc55734e,
    0xc668064d, 0xd77a4558, 0x46201c55, 0xf17dfc80, 0xf7142f2e, 0x87bfb718, 0x8aa54fb2, 0xc451d518,
    0xc4ae8831, 0x8dd44d55, 0x5bbd206c, 0x64536b5d, 0x5c667e60, 0x3b064242, 0xfe963a42, 0xa28e6dc8,
    0xe8a9604a, 0xc989464e, 0xd124a659, 0x50065140, 0xa44dfe5e, 0x1079e655, 0x3fb986d5, 0x47895b18,
    0x7d3ce4ad, 0x4561ba50, 0x296eec62, 0x255b41ad, 0xaed35ec9, 0x55556f12, 0xc7d3154d, 0x3297b65d,
    0x8930121f, 0xabf42e4e, 0x4a29e044, 0x1212685d, 0x676c1e40, 0xce009744, 0x383a8948, 0xa2dbd0ad,
    0xecc2564d, 0x07dbc252, 0x887ee24b, 0x5171644c, 0x6bb798c1, 0x847f495d, 0x4cbb7145, 0x3bb81c32,
    0x45eb262e, 0xc8015a4e, 0x250a361b, 0xf694f946, 0xd64a183e, 0xd4f1dd59, 0x8f20ffd4, 0x51d9e55c,
    0x09521763, 0x5e02002e, 0x32c8074d, 0xe685762e, 0x8290b0bc, 0x762a922e, 0xfc5ee754, 0x83a24829,
    0x775b224d, 0x6295bb4d, 0x38ec0555, 0xbffbba50, 0xe5560260, 0x86b16a7c, 0xd372234e, 0x49a3c24b,
    0x2f6a171f, 0x4d75ed60, 0xae94115b, 0xcb543744, 0x63080c59, 0x3f9c724c, 0xc977ce18, 0x532efb18,
    0x69dc3b2e, 0x5f94d929, 0x1732bb4d, 0x9c814b4d, 0xe6b3762e, 0xc024f662, 0x8face35b, 0x6b5b044d,
    0x798c7b57, 0x79a6b44c, 0x067d3057, 0xf9e94e5f, 0x91cbe15b, 0x71405eb2, 0x2662234e, 0xcbcc4a6d,
    0xbf69d54b, 0xa79b4e55, 0xec6d3e51, 0x7c0b3c02, 0x60f83653, 0x24c1e15c, 0x1110b62e, 0x10350f59,
    0xa56f1d55, 0x3509e7a9, 0xeb128354, 0x14268e2e, 0x934e28bc, 0x8e32692e, 0x8331a21f, 0x3e633932,
    0xc812b12e, 0xc684bf2e, 0x80112d2e, 0xe0ddc96c, 0xc630ca4a, 0x5c09b3b2, 0x0b580518, 0xc8e9d54b,
    0xd169aa43, 0x17d0d655, 0x1d029963, 0x7ff87559, 0xcb701f1f, 0x6fa3e85d, 0xe45e9a54, 0xf05d1802,
    0x44d03b2e, 0x837b692e, 0xccd4354e, 0x3d6da13c, 0x3423084d, 0xf707c34a, 0x55f6db3a, 0xad26e442,
    0x6233a21f, 0x09e80e59, 0x8caeb54d, 0xbe870941, 0xb407d20e, 0x20b51018, 0x56fb152e, 0x460d2a4e,
    0xbb9a2946, 0x560eb12e, 0xed83dd29, 0xd6724f53, 0xa50aafb8, 0x451346d9, 0x88348e2e, 0x7312fead,
    0x8ecaf96f, 0x1bda4e5f, 0xf1671e40, 0x3c8c3e3b, 0x4716324d, 0xdde24ede, 0xf98cd17d, 0xa91d4644,
    0x28124eb2, 0x147d5129, 0xd022042e, 0x61733d3b, 0xad0d5e02, 0x8ce2932e, 0xe5c18502, 0x549c1e32,
    0x9685801f, 0x86e217ad, 0xd948214b, 0x4110f462, 0x3a2e894e, 0xbd35492e, 0x87e0d558, 0x64b8ef7d,
    0x7c3eb962, 0x72a84b3e, 0x7cd667c9, 0x28370a2e, 0x4bc60e7b, 0x6fc1ec60, 0x14a6983f, 0x86739a4b,
    0x46954e5f, 0x32e2e15c, 0x2e9326cf, 0xe5801c5e, 0x379607b2, 0x32151145, 0xf0e39744, 0xacb54c55,
    0xa37dfb60, 0x83b55cc9, 0x388f7ca5, 0x15034f5f, 0x3e94965b, 0x68e0ffad, 0x35280f59, 0x8fe190cf,
    0x7c6ba5b2, 0xa5e9db43, 0x4ee1fc60, 0xd9d94e5f, 0x04040677, 0x0ea9b35e, 0x5961f14f, 0x67fda063,
    0xa48a5a31, 0xc6524e55, 0x283d325e, 0x3f37515f, 0x96b94b3e, 0xacce620e, 0x6481cc5b, 0xa4a06d4b,
    0x9e95d2d9, 0xe40c03d5, 0xc2f4514b, 0xb79aad44, 0xf64be843, 0xb2064070, 0xfca00455, 0x429dfa4e,
    0x2323f173, 0xeda4185e, 0xabd5227d, 0x9efd4d58, 0xb1104758, 0x4811e955, 0xbd9ab355, 0xe921f44b,
    0x9f166dce, 0x09e279b2, 0xe0c9ac7b, 0x7901a5ad, 0xa145d4b0, 0x79104671, 0xec31e35a, 0x4fe0b555,
    0xc7d9cbad, 0xad057f55, 0xe94cc759, 0x7fe0b043, 0xe4529f2e, 0x0d4dd4b2, 0x9f11a54d, 0x031e2e4e,
    0xe6014f5f, 0x11d1ca6c, 0x26bd7f61, 0xeb86854f, 0x4d347b57, 0x116bbe2e, 0xdba7234e, 0x7bcbfd2e,
    0x174dd4b2, 0x6686762e, 0xb089ba50, 0xc6258246, 0x087e767b, 0xc4a8cb4a, 0x595dba50, 0x7f0ae502,
    0x7b1dbd5a, 0xa0603492, 0x57d1af4b, 0x9e21ffd4, 0x6393064d, 0x7407376e, 0xe484762e, 0x122a4e53,
    0x4a37aa43, 0x3888a6be, 0xee77864e, 0x039c8dd5, 0x688d89af, 0x0e988f62, 0x08218246, 0xfc2f8246,
    0xd1d97040, 0xd64cd4b2, 0x5ae4a6b8, 0x7d0de9bc, 0x8d304d61, 0x06c5c672, 0xa4c8bd4d, 0xe0fd373b,
    0x575ebe4d, 0x72d26277, 0x55570f55, 0x77b154d9, 0xe214293a, 0xfc740f4b, 0xfe3f6a57, 0xa9c55f02,
    0xae4054db, 0x2394d918, 0xb511b24a, 0xb8741ab2, 0x0758e65e, 0xc7b5795b, 0xb0a30a4c, 0xaf7f170c,
    0xf3b4762e, 0x8179576d, 0x738a1581, 0x4b95b64c, 0x9829b618, 0x1bea932e, 0x7bdeaa4b, 0xcb5e0281,
    0x65618f54, 0x0658474b, 0x27066acf, 0x40556d65, 0x7d204d53, 0xf28bc244, 0xdce23455, 0xadc0ff54,
    0x3863c948, 0xcee34e5f, 0xdeb85e02, 0x2ed17a61, 0x6a7b094d, 0x7f0cfc40, 0x59603f54, 0x3220afbc,
    0xb5dfd962, 0x125d21c0, 0x13f8d243, 0xacfefb4e, 0x86c2c147, 0x3d8bbd59, 0xbd02a21f, 0x2593042e,
    0xc6a17a7c, 0x28925861, 0xb487ed44, 0xb5f4fd6d, 0x90c28a45, 0x5a14f74d, 0x43d71b4c, 0x728ebb5d,
    0x885bf950, 0x08134dd0, 0x38ec046e, 0xc575684b, 0x50082d2e, 0xa2f47757, 0x270f86ae, 0xf3ff6462,
    0x10ed3f4e, 0x4b58d462, 0xe01ce23e, 0x8c5b092e, 0x63e52f4e, 0x22c1e85d, 0xa908f54e, 0x8591624f,
    0x2c0fb94e, 0xa280ba3c, 0xb6f41b4c, 0x24f9aa47, 0x27201647, 0x3a3ea6dc, 0xa14fc3be, 0x3c34bdd5,
    0x5b8d4f5b, 0xaadeaf4b, 0xc71cab50, 0x15697a4c, 0x9a1a734c, 0x2a037d81, 0x2590bd59, 0x48ec2741,
    0x53489c5b, 0x7f00314b, 0x2170d362, 0xf2e92542, 0x42c10b44, 0x98f0f118, 0x883a3456, 0x099a932e,
    0xea38f7bc, 0x644e9247, 0xbb61b62e, 0x30e0863d, 0x5f51be54, 0x207215c7, 0x5f306c45, 0xaa7f3932,
    0x98da7d45, 0x4e339b59, 0x2e411581, 0xa808f618, 0xad2c0c59, 0x54476741, 0x09e99fd1, 0x5db8f752,
    0xc16df8bd, 0x1dd4b44f, 0x106edf2e, 0x9e15c180, 0x2ad6b56f, 0x633a5332, 0xff33787c, 0x077cb545,
    0x6610be6d, 0x75aad2c4, 0x72fb4d5b, 0xe81e0f59, 0x576f6332, 0x47333373, 0x351ed783, 0x2d90fb50,
    0x8d5e0f6c, 0x5b27a552, 0xdb293ebb, 0xe55ef950, 0x4b133ad8, 0x75df975a, 0x7b6a8740, 0xa899464b,
    0xfab15161, 0x10f8b64d, 0xd055ea4d, 0xee8e146b, 0x4b14afb8, 0x4bc1c44a, 0x9b961dcc, 0xd111ff43,
    0xfca0b745, 0xc800e412, 0x0afad9d1, 0xf751c350, 0xf9f0cccf, 0xa290a545, 0x8ef13763, 0x7ec70d59,
    0x2b066acf, 0x65496c45, 0xade02c1b, 0xae6eb077, 0x92c1e65b, 0xc064e6a9, 0xc649e56d, 0x5287a243,
    0x36de4f5b, 0x5b1df6ad, 0x65c39a59, 0xdba805b2, 0x20067aa8, 0x6457e56d, 0x3cee26cf, 0xfd3ff26d,
    0x04f86d4a, 0x06b8e048, 0xa93bcd5c, 0x91135852, 0xbe90a643, 0x8fa0094d, 0x06d8215f, 0x2677094d,
    0xd735685c, 0x164a00c9, 0x5209ac5f, 0xa9564c5c, 0x3b504f5f, 0xcc826bd0, 0x4615042e, 0x5fe13b4a,
    0x8c81b86d, 0x879ab68c, 0x1de564b8, 0x434487d8, 0x2dcb1b63, 0x82ab524a, 0xb0676abb, 0xa13d9c62,
    0xdbb5b86d, 0x5b7f4b59, 0xaddfb44d, 0xad773532, 0x3997054c, 0x72cebd89, 0xb194544c, 0xc5b8046e,
    0x6e1adeb2, 0xaa5abb51, 0xefb54b44, 0x15efc54f, 0xe9f1bc4d, 0x5f401b6c, 0x97f018ad, 0xc82f9252,
    0x2cdc762e, 0x8e52e56d, 0x1827175e, 0x9b7d7d80, 0xb2ad6845, 0x51065140, 0x71180a18, 0x5b27006c,
    0x0621e255, 0x721cbe58, 0x670c0cb8, 0xf8bd715d, 0xe0bdc5d9, 0xed843501, 0x4b84554d, 0x7f1a18bc,
    0x53bcaf47, 0x5729d35f, 0xf0dda246, 0x22382bd0, 0x4d641fb0, 0x316afcde, 0x50a22f1f, 0x73608046,
    0xc461d84a, 0xb2dbe247,
};



void ThreadOpenConnections(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadOpenConnections(parg));
    try
    {
        vnThreadsRunning[1]++;
        ThreadOpenConnections2(parg);
        vnThreadsRunning[1]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[1]--;
        PrintException(&e, "ThreadOpenConnections()");
    } catch (...) {
        vnThreadsRunning[1]--;
        PrintException(NULL, "ThreadOpenConnections()");
    }
    printf("ThreadOpenConnections exiting\n");
}

void ThreadOpenConnections2(void* parg)
{
    printf("ThreadOpenConnections started\n");

    // Connect to specific addresses
    if (mapArgs.count("-connect"))
    {
        for (int64 nLoop = 0;; nLoop++)
        {
            BOOST_FOREACH(string strAddr, mapMultiArgs["-connect"])
            {
                CAddress addr(CService(strAddr, GetDefaultPort(), fAllowDNS));
                if (addr.IsValid())
                    OpenNetworkConnection(addr);
                for (int i = 0; i < 10 && i < nLoop; i++)
                {
                    Sleep(500);
                    if (fShutdown)
                        return;
                }
            }
        }
    }

    // Initiate network connections
    int64 nStart = GetTime();
    loop
    {
        // Limit outbound connections
        vnThreadsRunning[1]--;
        Sleep(500);
        loop
        {
            int nOutbound = 0;
            CRITICAL_BLOCK(cs_vNodes)
                BOOST_FOREACH(CNode* pnode, vNodes)
                    if (!pnode->fInbound)
                        nOutbound++;
            int nMaxOutboundConnections = MAX_OUTBOUND_CONNECTIONS;
            nMaxOutboundConnections = min(nMaxOutboundConnections, (int)GetArg("-maxconnections", 125));
            if (nOutbound < nMaxOutboundConnections)
                break;
            Sleep(2000);
            if (fShutdown)
                return;
        }
        vnThreadsRunning[1]++;
        if (fShutdown)
            return;

        bool fAddSeeds = false;

        CRITICAL_BLOCK(cs_mapAddresses)
        {
            // Add seed nodes if IRC isn't working
            bool fTOR = (fUseProxy && addrProxy.GetPort() == 9050);
            if (mapAddresses.empty() && (GetTime() - nStart > 60 || fTOR) && !fTestNet)
                fAddSeeds = true;
        }

        if (fAddSeeds)
        {
            for (int i = 0; i < ARRAYLEN(pnSeed); i++)
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
                AddAddress(addr);
            }
        }

        //
        // Choose an address to connect to based on most recently seen
        //
        CAddress addrConnect;
        int64 nBest = std::numeric_limits<int64>::min();

        // Only connect to one address per a.b.?.? range.
        // Do this here so we don't have to critsect vNodes inside mapAddresses critsect.
        set<vector<unsigned char> > setConnected;
        CRITICAL_BLOCK(cs_vNodes)
            BOOST_FOREACH(CNode* pnode, vNodes)
                setConnected.insert(pnode->addr.GetGroup());

        int64 nANow = GetAdjustedTime();

        CRITICAL_BLOCK(cs_mapAddresses)
        {
            BOOST_FOREACH(const PAIRTYPE(vector<unsigned char>, CAddress)& item, mapAddresses)
            {
                const CAddress& addr = item.second;
                if (!addr.IsIPv4() || !addr.IsValid() || setConnected.count(addr.GetGroup()))
                    continue;
                int64 nSinceLastSeen = nANow - addr.nTime;
                int64 nSinceLastTry = nANow - addr.nLastTry;

                // Randomize the order in a deterministic way, putting the standard port first
                int64 nRandomizer = (uint64)(nStart * 4951 + addr.nLastTry * 9567851 + addr.GetHash()) % (2 * 60 * 60);
                if (addr.GetPort() != GetDefaultPort())
                    nRandomizer += 2 * 60 * 60;

                // Last seen  Base retry frequency
                //   <1 hour   10 min
                //    1 hour    1 hour
                //    4 hours   2 hours
                //   24 hours   5 hours
                //   48 hours   7 hours
                //    7 days   13 hours
                //   30 days   27 hours
                //   90 days   46 hours
                //  365 days   93 hours
                int64 nDelay = (int64)(3600.0 * sqrt(fabs((double)nSinceLastSeen) / 3600.0) + nRandomizer);

                // Fast reconnect for one hour after last seen
                if (nSinceLastSeen < 60 * 60)
                    nDelay = 10 * 60;

                // Limit retry frequency
                if (nSinceLastTry < nDelay)
                    continue;

                // If we have IRC, we'll be notified when they first come online,
                // and again every 24 hours by the refresh broadcast.
                if (nGotIRCAddresses > 0 && vNodes.size() >= 2 && nSinceLastSeen > 24 * 60 * 60)
                    continue;

                // Only try the old stuff if we don't have enough connections
                if (vNodes.size() >= 8 && nSinceLastSeen > 24 * 60 * 60)
                    continue;

                // If multiple addresses are ready, prioritize by time since
                // last seen and time since last tried.
                int64 nScore = min(nSinceLastTry, (int64)24 * 60 * 60) - nSinceLastSeen - nRandomizer;
                if (nScore > nBest)
                {
                    nBest = nScore;
                    addrConnect = addr;
                }
            }
        }

        if (addrConnect.IsValid())
            OpenNetworkConnection(addrConnect);
    }
}

void ThreadOpenAddedConnections(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadOpenAddedConnections(parg));
    try
    {
        vnThreadsRunning[7]++;
        ThreadOpenAddedConnections2(parg);
        vnThreadsRunning[7]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[7]--;
        PrintException(&e, "ThreadOpenAddedConnections()");
    } catch (...) {
        vnThreadsRunning[7]--;
        PrintException(NULL, "ThreadOpenAddedConnections()");
    }
    printf("ThreadOpenAddedConnections exiting\n");
}

void ThreadOpenAddedConnections2(void* parg)
{
    printf("ThreadOpenAddedConnections started\n");

    if (mapArgs.count("-addnode") == 0)
        return;

    vector<vector<CService> > vservAddressesToAdd(0);
    BOOST_FOREACH(string& strAddNode, mapMultiArgs["-addnode"])
    {
        vector<CService> vservNode(0);
        if(Lookup(strAddNode.c_str(), vservNode, GetDefaultPort(), fAllowDNS, 0))
        {
            vservAddressesToAdd.push_back(vservNode);
            CRITICAL_BLOCK(cs_setservAddNodeAddresses)
                BOOST_FOREACH(CService& serv, vservNode)
                    setservAddNodeAddresses.insert(serv);
        }
    }
    loop
    {
        vector<vector<CService> > vservConnectAddresses = vservAddressesToAdd;
        // Attempt to connect to each IP for each addnode entry until at least one is successful per addnode entry
        // (keeping in mind that addnode entries can have many IPs if fAllowDNS)
        CRITICAL_BLOCK(cs_vNodes)
            BOOST_FOREACH(CNode* pnode, vNodes)
                for (vector<vector<CService> >::iterator it = vservConnectAddresses.begin(); it != vservConnectAddresses.end(); it++)
                    BOOST_FOREACH(CService& addrNode, *(it))
                        if (pnode->addr == addrNode)
                        {
                            it = vservConnectAddresses.erase(it);
                            it--;
                            break;
                        }
        BOOST_FOREACH(vector<CService>& vserv, vservConnectAddresses)
        {
            OpenNetworkConnection(CAddress(*(vserv.begin())));
            Sleep(500);
            if (fShutdown)
                return;
        }
        if (fShutdown)
            return;
        vnThreadsRunning[7]--;
        Sleep(120000); // Retry every 2 minutes
        vnThreadsRunning[7]++;
        if (fShutdown)
            return;
    }
}

bool OpenNetworkConnection(const CAddress& addrConnect)
{
    //
    // Initiate outbound network connection
    //
    if (fShutdown)
        return false;
    if ((CNetAddr)addrConnect == (CNetAddr)addrLocalHost || !addrConnect.IsIPv4() ||
        FindNode((CNetAddr)addrConnect) || CNode::IsBanned(addrConnect))
        return false;

    vnThreadsRunning[1]--;
    CNode* pnode = ConnectNode(addrConnect);
    vnThreadsRunning[1]++;
    if (fShutdown)
        return false;
    if (!pnode)
        return false;
    pnode->fNetworkNode = true;

    return true;
}








void ThreadMessageHandler(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadMessageHandler(parg));
    try
    {
        vnThreadsRunning[2]++;
        ThreadMessageHandler2(parg);
        vnThreadsRunning[2]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[2]--;
        PrintException(&e, "ThreadMessageHandler()");
    } catch (...) {
        vnThreadsRunning[2]--;
        PrintException(NULL, "ThreadMessageHandler()");
    }
    printf("ThreadMessageHandler exiting\n");
}

void ThreadMessageHandler2(void* parg)
{
    printf("ThreadMessageHandler started\n");
    SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);
    while (!fShutdown)
    {
        vector<CNode*> vNodesCopy;
        CRITICAL_BLOCK(cs_vNodes)
        {
            vNodesCopy = vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->AddRef();
        }

        // Poll the connected nodes for messages
        CNode* pnodeTrickle = NULL;
        if (!vNodesCopy.empty())
            pnodeTrickle = vNodesCopy[GetRand(vNodesCopy.size())];
        BOOST_FOREACH(CNode* pnode, vNodesCopy)
        {
            // Receive messages
            TRY_CRITICAL_BLOCK(pnode->cs_vRecv)
                ProcessMessages(pnode);
            if (fShutdown)
                return;

            // Send messages
            TRY_CRITICAL_BLOCK(pnode->cs_vSend)
                SendMessages(pnode, pnode == pnodeTrickle);
            if (fShutdown)
                return;
        }

        CRITICAL_BLOCK(cs_vNodes)
        {
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->Release();
        }

        // Wait and allow messages to bunch up.
        // Reduce vnThreadsRunning so StopNode has permission to exit while
        // we're sleeping, but we must always check fShutdown after doing this.
        vnThreadsRunning[2]--;
        Sleep(100);
        if (fRequestShutdown)
            Shutdown(NULL);
        vnThreadsRunning[2]++;
        if (fShutdown)
            return;
    }
}






bool BindListenPort(string& strError)
{
    strError = "";
    int nOne = 1;
    addrLocalHost.SetPort(GetListenPort());

#ifdef WIN32
    // Initialize Windows Sockets
    WSADATA wsadata;
    int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (ret != NO_ERROR)
    {
        strError = strprintf("Error: TCP/IP socket library failed to start (WSAStartup returned error %d)", ret);
        printf("%s\n", strError.c_str());
        return false;
    }
#endif

    // Create socket for listening for incoming connections
    hListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
    // Set to nonblocking, incoming connections will also inherit this
    if (ioctlsocket(hListenSocket, FIONBIO, (u_long*)&nOne) == SOCKET_ERROR)
#else
    if (fcntl(hListenSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
#endif
    {
        strError = strprintf("Error: Couldn't set properties on socket for incoming connections (error %d)", WSAGetLastError());
        printf("%s\n", strError.c_str());
        return false;
    }

    // The sockaddr_in structure specifies the address family,
    // IP address, and port for the socket that is being bound
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY; // bind to all IPs on this computer
    sockaddr.sin_port = htons(GetListenPort());
    if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
    {
        int nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE)
            strError = strprintf(_("Unable to bind to port %d on this computer.  Bitcoin is probably already running."), ntohs(sockaddr.sin_port));
        else
            strError = strprintf("Error: Unable to bind to port %d on this computer (bind returned error %d)", ntohs(sockaddr.sin_port), nErr);
        printf("%s\n", strError.c_str());
        return false;
    }
    printf("Bound to port %d\n", ntohs(sockaddr.sin_port));

    // Listen for incoming connections
    if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        strError = strprintf("Error: Listening for incoming connections failed (listen returned error %d)", WSAGetLastError());
        printf("%s\n", strError.c_str());
        return false;
    }

    return true;
}

void StartNode(void* parg)
{
    if (pnodeLocalHost == NULL)
        pnodeLocalHost = new CNode(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), nLocalServices));

#ifdef WIN32
    // Get local host ip
    char pszHostName[1000] = "";
    if (gethostname(pszHostName, sizeof(pszHostName)) != SOCKET_ERROR)
    {
        vector<CNetAddr> vaddr;
        if (LookupHost(pszHostName, vaddr))
            BOOST_FOREACH (const CNetAddr &addr, vaddr)
                if (!addr.IsLocal())
                {
                    addrLocalHost.SetIP(addr);
                    break;
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
            char pszIP[100];
            if (ifa->ifa_addr->sa_family == AF_INET)
            {
                struct sockaddr_in* s4 = (struct sockaddr_in*)(ifa->ifa_addr);
                if (inet_ntop(ifa->ifa_addr->sa_family, (void*)&(s4->sin_addr), pszIP, sizeof(pszIP)) != NULL)
                    printf("ipv4 %s: %s\n", ifa->ifa_name, pszIP);

                // Take the first IP that isn't loopback 127.x.x.x
                CAddress addr(CService(s4->sin_addr, GetListenPort()), nLocalServices);
                if (addr.IsValid() && !addr.IsLocal())
                {
                    addrLocalHost = addr;
                    break;
                }
            }
            else if (ifa->ifa_addr->sa_family == AF_INET6)
            {
                struct sockaddr_in6* s6 = (struct sockaddr_in6*)(ifa->ifa_addr);
                if (inet_ntop(ifa->ifa_addr->sa_family, (void*)&(s6->sin6_addr), pszIP, sizeof(pszIP)) != NULL)
                    printf("ipv6 %s: %s\n", ifa->ifa_name, pszIP);
            }
        }
        freeifaddrs(myaddrs);
    }
#endif
    printf("addrLocalHost = %s\n", addrLocalHost.ToString().c_str());

    if (fUseProxy || mapArgs.count("-connect") || fNoListen)
    {
        // Proxies can't take incoming connections
        addrLocalHost.SetIP(CNetAddr("0.0.0.0"));
        printf("addrLocalHost = %s\n", addrLocalHost.ToString().c_str());
    }
    else
    {
        CreateThread(ThreadGetMyExternalIP, NULL);
    }

    //
    // Start threads
    //

    if (GetBoolArg("-nodnsseed"))
        printf("DNS seeding disabled\n");
    else
        if (!CreateThread(ThreadDNSAddressSeed, NULL))
            printf("Error: CreateThread(ThreadDNSAddressSeed) failed\n");

    // Map ports with UPnP
    if (fHaveUPnP)
        MapPort(fUseUPnP);

    // Get addresses from IRC and advertise ours
    if (!CreateThread(ThreadIRCSeed, NULL))
        printf("Error: CreateThread(ThreadIRCSeed) failed\n");

    // Send and receive from sockets, accept connections
    if (!CreateThread(ThreadSocketHandler, NULL))
        printf("Error: CreateThread(ThreadSocketHandler) failed\n");

    // Initiate outbound connections from -addnode
    if (!CreateThread(ThreadOpenAddedConnections, NULL))
        printf("Error: CreateThread(ThreadOpenAddedConnections) failed\n");

    // Initiate outbound connections
    if (!CreateThread(ThreadOpenConnections, NULL))
        printf("Error: CreateThread(ThreadOpenConnections) failed\n");

    // Process messages
    if (!CreateThread(ThreadMessageHandler, NULL))
        printf("Error: CreateThread(ThreadMessageHandler) failed\n");

    // Generate coins in the background
    GenerateBitcoins(fGenerateBitcoins, pwalletMain);
}

bool StopNode()
{
    printf("StopNode()\n");
    fShutdown = true;
    nTransactionsUpdated++;
    int64 nStart = GetTime();
    while (vnThreadsRunning[0] > 0 || vnThreadsRunning[2] > 0 || vnThreadsRunning[3] > 0 || vnThreadsRunning[4] > 0
        || (fHaveUPnP && vnThreadsRunning[5] > 0) || vnThreadsRunning[6] > 0 || vnThreadsRunning[7] > 0
    )
    {
        if (GetTime() - nStart > 20)
            break;
        Sleep(20);
    }
    if (vnThreadsRunning[0] > 0) printf("ThreadSocketHandler still running\n");
    if (vnThreadsRunning[1] > 0) printf("ThreadOpenConnections still running\n");
    if (vnThreadsRunning[2] > 0) printf("ThreadMessageHandler still running\n");
    if (vnThreadsRunning[3] > 0) printf("ThreadBitcoinMiner still running\n");
    if (vnThreadsRunning[4] > 0) printf("ThreadRPCServer still running\n");
    if (fHaveUPnP && vnThreadsRunning[5] > 0) printf("ThreadMapPort still running\n");
    if (vnThreadsRunning[6] > 0) printf("ThreadDNSAddressSeed still running\n");
    if (vnThreadsRunning[7] > 0) printf("ThreadOpenAddedConnections still running\n");
    while (vnThreadsRunning[2] > 0 || vnThreadsRunning[4] > 0)
        Sleep(20);
    Sleep(50);

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
        if (hListenSocket != INVALID_SOCKET)
            if (closesocket(hListenSocket) == SOCKET_ERROR)
                printf("closesocket(hListenSocket) failed with error %d\n", WSAGetLastError());

#ifdef WIN32
        // Shutdown Windows Sockets
        WSACleanup();
#endif
    }
}
instance_of_cnetcleanup;
