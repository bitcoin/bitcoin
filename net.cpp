// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"

void ThreadMessageHandler2(void* parg);
void ThreadSocketHandler2(void* parg);
void ThreadOpenConnections2(void* parg);
bool OpenNetworkConnection(const CAddress& addrConnect);






//
// Global state variables
//
bool fClient = false;
uint64 nLocalServices = (fClient ? 0 : NODE_NETWORK);
CAddress addrLocalHost(0, DEFAULT_PORT, nLocalServices);
CNode* pnodeLocalHost = NULL;
uint64 nLocalHostNonce = 0;
bool fShutdown = false;
array<int, 10> vnThreadsRunning;
SOCKET hListenSocket = INVALID_SOCKET;

vector<CNode*> vNodes;
CCriticalSection cs_vNodes;
map<vector<unsigned char>, CAddress> mapAddresses;
CCriticalSection cs_mapAddresses;
map<CInv, CDataStream> mapRelay;
deque<pair<int64, CInv> > vRelayExpiration;
CCriticalSection cs_mapRelay;
map<CInv, int64> mapAlreadyAskedFor;

// Settings
int fUseProxy = false;
CAddress addrProxy("127.0.0.1:9050");



bool ConnectSocket(const CAddress& addrConnect, SOCKET& hSocketRet)
{
    hSocketRet = INVALID_SOCKET;

    SOCKET hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hSocket == INVALID_SOCKET)
        return false;
#if defined(__BSD__) || defined(__WXOSX__)
    int set = 1;
    setsockopt(hSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
#endif

    bool fRoutable = !(addrConnect.GetByte(3) == 10 || (addrConnect.GetByte(3) == 192 && addrConnect.GetByte(2) == 168));
    bool fProxy = (fUseProxy && fRoutable);
    struct sockaddr_in sockaddr = (fProxy ? addrProxy.GetSockAddr() : addrConnect.GetSockAddr());

    if (connect(hSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
    {
        closesocket(hSocket);
        return false;
    }

    if (fProxy)
    {
        printf("Proxy connecting %s\n", addrConnect.ToStringLog().c_str());
        char pszSocks4IP[] = "\4\1\0\0\0\0\0\0user";
        memcpy(pszSocks4IP + 2, &addrConnect.port, 2);
        memcpy(pszSocks4IP + 4, &addrConnect.ip, 4);
        char* pszSocks4 = pszSocks4IP;
        int nSize = sizeof(pszSocks4IP);

        int ret = send(hSocket, pszSocks4, nSize, MSG_NOSIGNAL);
        if (ret != nSize)
        {
            closesocket(hSocket);
            return error("Error sending to proxy");
        }
        char pchRet[8];
        if (recv(hSocket, pchRet, 8, 0) != 8)
        {
            closesocket(hSocket);
            return error("Error reading proxy response");
        }
        if (pchRet[1] != 0x5a)
        {
            closesocket(hSocket);
            return error("Proxy returned error %d", pchRet[1]);
        }
        printf("Proxy connection established %s\n", addrConnect.ToStringLog().c_str());
    }

    hSocketRet = hSocket;
    return true;
}



bool GetMyExternalIP2(const CAddress& addrConnect, const char* pszGet, const char* pszKeyword, unsigned int& ipRet)
{
    SOCKET hSocket;
    if (!ConnectSocket(addrConnect, hSocket))
        return error("GetMyExternalIP() : connection to %s failed", addrConnect.ToString().c_str());

    send(hSocket, pszGet, strlen(pszGet), MSG_NOSIGNAL);

    string strLine;
    while (RecvLine(hSocket, strLine))
    {
        if (strLine.empty())
        {
            loop
            {
                if (!RecvLine(hSocket, strLine))
                {
                    closesocket(hSocket);
                    return false;
                }
                if (strLine.find(pszKeyword) != -1)
                {
                    strLine = strLine.substr(strLine.find(pszKeyword) + strlen(pszKeyword));
                    break;
                }
            }
            closesocket(hSocket);
            if (strLine.find("<"))
                strLine = strLine.substr(0, strLine.find("<"));
            strLine = strLine.substr(strspn(strLine.c_str(), " \t\n\r"));
            while (strLine.size() > 0 && isspace(strLine[strLine.size()-1]))
                strLine.resize(strLine.size()-1);
            CAddress addr(strLine.c_str());
            printf("GetMyExternalIP() received [%s] %s\n", strLine.c_str(), addr.ToString().c_str());
            if (addr.ip == 0 || addr.ip == INADDR_NONE || !addr.IsRoutable())
                return false;
            ipRet = addr.ip;
            return true;
        }
    }
    closesocket(hSocket);
    return error("GetMyExternalIP() : connection closed");
}


bool GetMyExternalIP(unsigned int& ipRet)
{
    CAddress addrConnect;
    char* pszGet;
    char* pszKeyword;

    if (fUseProxy)
        return false;

    for (int nLookup = 0; nLookup <= 1; nLookup++)
    for (int nHost = 1; nHost <= 2; nHost++)
    {
        if (nHost == 1)
        {
            addrConnect = CAddress("70.86.96.218:80"); // www.ipaddressworld.com

            if (nLookup == 1)
            {
                struct hostent* phostent = gethostbyname("www.ipaddressworld.com");
                if (phostent && phostent->h_addr_list && phostent->h_addr_list[0])
                    addrConnect = CAddress(*(u_long*)phostent->h_addr_list[0], htons(80));
            }

            pszGet = "GET /ip.php HTTP/1.1\r\n"
                     "Host: www.ipaddressworld.com\r\n"
                     "User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)\r\n"
                     "Connection: close\r\n"
                     "\r\n";

            pszKeyword = "IP:";
        }
        else if (nHost == 2)
        {
            addrConnect = CAddress("208.78.68.70:80"); // checkip.dyndns.org

            if (nLookup == 1)
            {
                struct hostent* phostent = gethostbyname("checkip.dyndns.org");
                if (phostent && phostent->h_addr_list && phostent->h_addr_list[0])
                    addrConnect = CAddress(*(u_long*)phostent->h_addr_list[0], htons(80));
            }

            pszGet = "GET / HTTP/1.1\r\n"
                     "Host: checkip.dyndns.org\r\n"
                     "User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)\r\n"
                     "Connection: close\r\n"
                     "\r\n";

            pszKeyword = "Address:";
        }

        if (GetMyExternalIP2(addrConnect, pszGet, pszKeyword, ipRet))
            return true;
    }

    return false;
}





bool AddAddress(CAddrDB& addrdb, CAddress addr, bool fCurrentlyOnline)
{
    if (!addr.IsRoutable())
        return false;
    if (addr.ip == addrLocalHost.ip)
        return false;
    if (fCurrentlyOnline)
        addr.nTime = GetAdjustedTime();
    CRITICAL_BLOCK(cs_mapAddresses)
    {
        map<vector<unsigned char>, CAddress>::iterator it = mapAddresses.find(addr.GetKey());
        if (it == mapAddresses.end())
        {
            // New address
            mapAddresses.insert(make_pair(addr.GetKey(), addr));
            addrdb.WriteAddress(addr);
            return true;
        }
        else
        {
            bool fUpdated = false;
            CAddress& addrFound = (*it).second;
            if ((addrFound.nServices | addr.nServices) != addrFound.nServices)
            {
                // Services have been added
                addrFound.nServices |= addr.nServices;
                fUpdated = true;
            }
            int64 nUpdateInterval = (fCurrentlyOnline ? 60 * 60 : 24 * 60 * 60);
            if (addrFound.nTime < addr.nTime - nUpdateInterval)
            {
                // Periodically update most recently seen time
                addrFound.nTime = addr.nTime;
                fUpdated = true;
            }
            if (fUpdated)
                addrdb.WriteAddress(addrFound);
        }
    }
    return false;
}

void AddressCurrentlyConnected(const CAddress& addr)
{
    CRITICAL_BLOCK(cs_mapAddresses)
    {
        // Only if it's been published already
        map<vector<unsigned char>, CAddress>::iterator it = mapAddresses.find(addr.GetKey());
        if (it != mapAddresses.end())
        {
            CAddress& addrFound = (*it).second;
            int64 nUpdateInterval = 60 * 60;
            if (addrFound.nTime < GetAdjustedTime() - nUpdateInterval)
            {
                // Periodically update most recently seen time
                addrFound.nTime = GetAdjustedTime();
                CAddrDB addrdb;
                addrdb.WriteAddress(addrFound);
            }
        }
    }
}





void AbandonRequests(void (*fn)(void*, CDataStream&), void* param1)
{
    // If the dialog might get closed before the reply comes back,
    // call this in the destructor so it doesn't get called after it's deleted.
    CRITICAL_BLOCK(cs_vNodes)
    {
        foreach(CNode* pnode, vNodes)
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
        foreach(CNode* pnode, vNodes)
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
            foreach(CNode* pnode, vNodes)
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
            foreach(CNode* pnode, vNodes)
                if (pnode != this)
                    pnode->PushMessage("sub-cancel", nChannel);

        // Clear memory, no longer subscribed
        if (nChannel == MSG_PRODUCT)
            CRITICAL_BLOCK(cs_mapProducts)
                mapProducts.clear();
    }
}









CNode* FindNode(unsigned int ip)
{
    CRITICAL_BLOCK(cs_vNodes)
    {
        foreach(CNode* pnode, vNodes)
            if (pnode->addr.ip == ip)
                return (pnode);
    }
    return NULL;
}

CNode* FindNode(CAddress addr)
{
    CRITICAL_BLOCK(cs_vNodes)
    {
        foreach(CNode* pnode, vNodes)
            if (pnode->addr == addr)
                return (pnode);
    }
    return NULL;
}

CNode* ConnectNode(CAddress addrConnect, int64 nTimeout)
{
    if (addrConnect.ip == addrLocalHost.ip)
        return NULL;

    // Look for an existing connection
    CNode* pnode = FindNode(addrConnect.ip);
    if (pnode)
    {
        if (nTimeout != 0)
            pnode->AddRef(nTimeout);
        else
            pnode->AddRef();
        return pnode;
    }

    /// debug print
    printf("trying connection %s\n", addrConnect.ToStringLog().c_str());

    // Connect
    SOCKET hSocket;
    if (ConnectSocket(addrConnect, hSocket))
    {
        /// debug print
        printf("connected %s\n", addrConnect.ToStringLog().c_str());

        // Set to nonblocking
#ifdef __WXMSW__
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

        CRITICAL_BLOCK(cs_mapAddresses)
            mapAddresses[addrConnect.GetKey()].nLastFailed = 0;
        return pnode;
    }
    else
    {
        CRITICAL_BLOCK(cs_mapAddresses)
            mapAddresses[addrConnect.GetKey()].nLastFailed = GetAdjustedTime();
        return NULL;
    }
}

void CNode::DoDisconnect()
{
    printf("disconnecting node %s\n", addr.ToStringLog().c_str());

    closesocket(hSocket);

    // If outbound and never got version message, mark address as failed
    if (!fInbound && !fSuccessfullyConnected)
        CRITICAL_BLOCK(cs_mapAddresses)
            mapAddresses[addr.GetKey()].nLastFailed = GetAdjustedTime();

    // All of a nodes broadcasts and subscriptions are automatically torn down
    // when it goes down, so a node has to stay up to keep its broadcast going.

    CRITICAL_BLOCK(cs_mapProducts)
        for (map<uint256, CProduct>::iterator mi = mapProducts.begin(); mi != mapProducts.end();)
            AdvertRemoveSource(this, MSG_PRODUCT, 0, (*(mi++)).second);

    // Cancel subscriptions
    for (unsigned int nChannel = 0; nChannel < vfSubscribe.size(); nChannel++)
        if (vfSubscribe[nChannel])
            CancelSubscribe(nChannel);
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
        PrintException(NULL, "ThreadSocketHandler()");
    }

    foreach(CNode* pnode, vNodes)
        closesocket(pnode->hSocket);
    if (closesocket(hListenSocket) == SOCKET_ERROR)
        printf("closesocket(hListenSocket) failed with error %d\n", WSAGetLastError());

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
            foreach(CNode* pnode, vNodesCopy)
            {
                if (pnode->ReadyToDisconnect() && pnode->vRecv.empty() && pnode->vSend.empty())
                {
                    // remove from vNodes
                    vNodes.erase(remove(vNodes.begin(), vNodes.end(), pnode), vNodes.end());
                    pnode->DoDisconnect();

                    // hold in disconnected pool until all refs are released
                    pnode->nReleaseTime = max(pnode->nReleaseTime, GetTime() + 5 * 60);
                    if (pnode->fNetworkNode)
                        pnode->Release();
                    vNodesDisconnected.push_back(pnode);
                }
            }

            // Delete disconnected nodes
            list<CNode*> vNodesDisconnectedCopy = vNodesDisconnected;
            foreach(CNode* pnode, vNodesDisconnectedCopy)
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
        FD_ZERO(&fdsetRecv);
        FD_ZERO(&fdsetSend);
        SOCKET hSocketMax = 0;
        FD_SET(hListenSocket, &fdsetRecv);
        hSocketMax = max(hSocketMax, hListenSocket);
        CRITICAL_BLOCK(cs_vNodes)
        {
            foreach(CNode* pnode, vNodes)
            {
                FD_SET(pnode->hSocket, &fdsetRecv);
                hSocketMax = max(hSocketMax, pnode->hSocket);
                TRY_CRITICAL_BLOCK(pnode->cs_vSend)
                    if (!pnode->vSend.empty())
                        FD_SET(pnode->hSocket, &fdsetSend);
            }
        }

        vnThreadsRunning[0]--;
        int nSelect = select(hSocketMax + 1, &fdsetRecv, &fdsetSend, NULL, &timeout);
        vnThreadsRunning[0]++;
        if (fShutdown)
            return;
        if (nSelect == SOCKET_ERROR)
        {
            int nErr = WSAGetLastError();
            printf("select failed: %d\n", nErr);
            for (int i = 0; i <= hSocketMax; i++)
            {
                FD_SET(i, &fdsetRecv);
                FD_SET(i, &fdsetSend);
            }
            Sleep(timeout.tv_usec/1000);
        }

        //// debug print
        //foreach(CNode* pnode, vNodes)
        //{
        //    printf("vRecv = %-5d ", pnode->vRecv.size());
        //    printf("vSend = %-5d    ", pnode->vSend.size());
        //}
        //printf("\n");


        //
        // Accept new connections
        //
        if (FD_ISSET(hListenSocket, &fdsetRecv))
        {
            struct sockaddr_in sockaddr;
#ifdef __WXMSW__
            int len = sizeof(sockaddr);
#else
            socklen_t len = sizeof(sockaddr);
#endif
            SOCKET hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);
            CAddress addr(sockaddr);
            if (hSocket == INVALID_SOCKET)
            {
                if (WSAGetLastError() != WSAEWOULDBLOCK)
                    printf("ERROR ThreadSocketHandler accept failed: %d\n", WSAGetLastError());
            }
            else
            {
                printf("accepted connection %s\n", addr.ToStringLog().c_str());
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
            vNodesCopy = vNodes;
        foreach(CNode* pnode, vNodesCopy)
        {
            if (fShutdown)
                return;
            SOCKET hSocket = pnode->hSocket;

            //
            // Receive
            //
            if (FD_ISSET(hSocket, &fdsetRecv))
            {
                TRY_CRITICAL_BLOCK(pnode->cs_vRecv)
                {
                    CDataStream& vRecv = pnode->vRecv;
                    unsigned int nPos = vRecv.size();

                    // typical socket buffer is 8K-64K
                    const unsigned int nBufSize = 0x10000;
                    vRecv.resize(nPos + nBufSize);
                    int nBytes = recv(hSocket, &vRecv[nPos], nBufSize, 0);
                    vRecv.resize(nPos + max(nBytes, 0));
                    if (nBytes == 0)
                    {
                        // socket closed gracefully
                        if (!pnode->fDisconnect)
                            printf("recv: socket closed\n");
                        pnode->fDisconnect = true;
                    }
                    else if (nBytes < 0)
                    {
                        // socket error
                        int nErr = WSAGetLastError();
                        if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
                        {
                            if (!pnode->fDisconnect)
                                printf("recv failed: %d\n", nErr);
                            pnode->fDisconnect = true;
                        }
                    }
                }
            }

            //
            // Send
            //
            if (FD_ISSET(hSocket, &fdsetSend))
            {
                TRY_CRITICAL_BLOCK(pnode->cs_vSend)
                {
                    CDataStream& vSend = pnode->vSend;
                    if (!vSend.empty())
                    {
                        int nBytes = send(hSocket, &vSend[0], vSend.size(), MSG_NOSIGNAL);
                        if (nBytes > 0)
                        {
                            vSend.erase(vSend.begin(), vSend.begin() + nBytes);
                        }
                        else if (nBytes == 0)
                        {
                            if (pnode->ReadyToDisconnect())
                                pnode->vSend.clear();
                        }
                        else
                        {
                            printf("send error %d\n", nBytes);
                            if (pnode->ReadyToDisconnect())
                                pnode->vSend.clear();
                        }
                    }
                }
            }
        }


        Sleep(10);
    }
}










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

    // Connect to one specified address
    while (mapArgs.count("-connect"))
    {
        OpenNetworkConnection(CAddress(mapArgs["-connect"]));
        for (int i = 0; i < 10; i++)
        {
            Sleep(1000);
            if (fShutdown)
                return;
        }
    }

    // Connect to manually added nodes first
    if (mapArgs.count("-addnode"))
    {
        foreach(string strAddr, mapMultiArgs["-addnode"])
        {
            CAddress addr(strAddr, NODE_NETWORK);
            if (addr.IsValid())
            {
                OpenNetworkConnection(addr);
                Sleep(1000);
                if (fShutdown)
                    return;
            }
        }
    }

    // Initiate network connections
    loop
    {
        // Wait
        vnThreadsRunning[1]--;
        Sleep(500);
        const int nMaxConnections = 15;
        while (vNodes.size() >= nMaxConnections || vNodes.size() >= mapAddresses.size())
        {
            if (fShutdown)
                return;
            Sleep(2000);
        }
        vnThreadsRunning[1]++;
        if (fShutdown)
            return;

        //
        // Choose an address to connect to based on most recently seen
        //
        CAddress addrConnect;
        int64 nBestTime = 0;
        int64 nDelay = ((60 * 60) << vNodes.size());
        if (vNodes.size() >= 3)
            nDelay *= 4;
        if (nGotIRCAddresses > 0)
            nDelay *= 100;

        // Do this here so we don't have to critsect vNodes inside mapAddresses critsect
        set<unsigned int> setConnected;
        CRITICAL_BLOCK(cs_vNodes)
            foreach(CNode* pnode, vNodes)
                setConnected.insert(pnode->addr.ip);

        CRITICAL_BLOCK(cs_mapAddresses)
        {
            foreach(const PAIRTYPE(vector<unsigned char>, CAddress)& item, mapAddresses)
            {
                const CAddress& addr = item.second;
                if (!addr.IsIPv4() || !addr.IsValid() || setConnected.count(addr.ip))
                    continue;

                // Limit retry frequency
                if (GetAdjustedTime() < addr.nLastFailed + nDelay)
                    continue;

                // Try again only after all addresses had a first attempt
                int64 nTime = addr.nTime;
                if (addr.nLastFailed > addr.nTime)
                    nTime -= 365 * 24 * 60 * 60;

                // Randomize the order a little, putting the standard port first
                nTime += GetRand(1 * 60 * 60);
                if (addr.port != DEFAULT_PORT)
                    nTime -= 1 * 60 * 60;

                if (nTime > nBestTime)
                {
                    nBestTime = nTime;
                    addrConnect = addr;
                }
            }
        }

        if (addrConnect.IsValid())
            OpenNetworkConnection(addrConnect);
    }
}

bool OpenNetworkConnection(const CAddress& addrConnect)
{
    //
    // Initiate outbound network connection
    //
    if (fShutdown)
        return false;
    if (addrConnect.ip == addrLocalHost.ip || !addrConnect.IsIPv4() || FindNode(addrConnect.ip))
        return false;

    vnThreadsRunning[1]--;
    CNode* pnode = ConnectNode(addrConnect);
    vnThreadsRunning[1]++;
    if (fShutdown)
        return false;
    if (!pnode)
        return false;
    pnode->fNetworkNode = true;

    if (addrLocalHost.IsRoutable() && !fUseProxy)
    {
        // Advertise our address
        vector<CAddress> vAddrToSend;
        vAddrToSend.push_back(addrLocalHost);
        pnode->PushMessage("addr", vAddrToSend);
    }

    // Get as many addresses as we can
    pnode->PushMessage("getaddr");
    pnode->fGetAddr = true; // don't relay the results of the getaddr

    ////// should the one on the receiving end do this too?
    // Subscribe our local subscription list
    const unsigned int nHops = 0;
    for (unsigned int nChannel = 0; nChannel < pnodeLocalHost->vfSubscribe.size(); nChannel++)
        if (pnodeLocalHost->vfSubscribe[nChannel])
            pnode->PushMessage("subscribe", nChannel, nHops);

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
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    loop
    {
        // Poll the connected nodes for messages
        vector<CNode*> vNodesCopy;
        CRITICAL_BLOCK(cs_vNodes)
            vNodesCopy = vNodes;
        foreach(CNode* pnode, vNodesCopy)
        {
            pnode->AddRef();

            // Receive messages
            TRY_CRITICAL_BLOCK(pnode->cs_vRecv)
                ProcessMessages(pnode);
            if (fShutdown)
                return;

            // Send messages
            TRY_CRITICAL_BLOCK(pnode->cs_vSend)
                SendMessages(pnode);
            if (fShutdown)
                return;

            pnode->Release();
        }

        // Wait and allow messages to bunch up
        vnThreadsRunning[2]--;
        Sleep(100);
        vnThreadsRunning[2]++;
        if (fShutdown)
            return;
    }
}










bool StartNode(string& strError)
{
    if (pnodeLocalHost == NULL)
        pnodeLocalHost = new CNode(INVALID_SOCKET, CAddress("127.0.0.1", nLocalServices));
    strError = "";

#ifdef __WXMSW__
    // Sockets startup
    WSADATA wsadata;
    int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (ret != NO_ERROR)
    {
        strError = strprintf("Error: TCP/IP socket library failed to start (WSAStartup returned error %d)", ret);
        printf("%s\n", strError.c_str());
        return false;
    }
#endif

    // Get local host ip
    char pszHostName[255];
    if (gethostname(pszHostName, sizeof(pszHostName)) == SOCKET_ERROR)
    {
        strError = strprintf("Error: Unable to get IP address of this computer (gethostname returned error %d)", WSAGetLastError());
        printf("%s\n", strError.c_str());
        return false;
    }
    struct hostent* phostent = gethostbyname(pszHostName);
    if (!phostent)
    {
        strError = strprintf("Error: Unable to get IP address of this computer (gethostbyname returned error %d)", WSAGetLastError());
        printf("%s\n", strError.c_str());
        return false;
    }

    // Take the first IP that isn't loopback 127.x.x.x
    for (int i = 0; phostent->h_addr_list[i] != NULL; i++)
        printf("host ip %d: %s\n", i, CAddress(*(unsigned int*)phostent->h_addr_list[i]).ToStringIP().c_str());
    for (int i = 0; phostent->h_addr_list[i] != NULL; i++)
    {
        addrLocalHost = CAddress(*(unsigned int*)phostent->h_addr_list[i], DEFAULT_PORT, nLocalServices);
        if (addrLocalHost.IsValid() && addrLocalHost.GetByte(3) != 127)
            break;
    }
    printf("addrLocalHost = %s\n", addrLocalHost.ToString().c_str());

    // Create socket for listening for incoming connections
    hListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hListenSocket == INVALID_SOCKET)
    {
        strError = strprintf("Error: Couldn't open socket for incoming connections (socket returned error %d)", WSAGetLastError());
        printf("%s\n", strError.c_str());
        return false;
    }
#if defined(__BSD__) || defined(__WXOSX__)
    int set = 1;
    setsockopt(hSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
#endif

    // Set to nonblocking, incoming connections will also inherit this
#ifdef __WXMSW__
    u_long nOne = 1;
    if (ioctlsocket(hListenSocket, FIONBIO, &nOne) == SOCKET_ERROR)
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
    sockaddr.sin_port = DEFAULT_PORT;
    if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
    {
        int nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE)
            strError = strprintf("Error: Unable to bind to port %d on this computer. The program is probably already running.", ntohs(sockaddr.sin_port));
        else
            strError = strprintf("Error: Unable to bind to port %d on this computer (bind returned error %d)", ntohs(sockaddr.sin_port), nErr);
        printf("%s\n", strError.c_str());
        return false;
    }
    printf("bound to port %d\n", ntohs(sockaddr.sin_port));

    // Listen for incoming connections
    if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        strError = strprintf("Error: Listening for incoming connections failed (listen returned error %d)", WSAGetLastError());
        printf("%s\n", strError.c_str());
        return false;
    }

    // Get our external IP address for incoming connections
    if (fUseProxy)
    {
        // Proxies can't take incoming connections
        addrLocalHost.ip = CAddress("0.0.0.0").ip;
        printf("addrLocalHost = %s\n", addrLocalHost.ToString().c_str());
    }
    else
    {
        if (addrIncoming.ip)
            addrLocalHost.ip = addrIncoming.ip;

        if (GetMyExternalIP(addrLocalHost.ip))
        {
            addrIncoming = addrLocalHost;
            CWalletDB().WriteSetting("addrIncoming", addrIncoming);
        }
    }

    // Get addresses from IRC and advertise ours
    if (_beginthread(ThreadIRCSeed, 0, NULL) == -1)
        printf("Error: _beginthread(ThreadIRCSeed) failed\n");

    //
    // Start threads
    //
    if (_beginthread(ThreadSocketHandler, 0, NULL) == -1)
    {
        strError = "Error: _beginthread(ThreadSocketHandler) failed";
        printf("%s\n", strError.c_str());
        return false;
    }

    if (_beginthread(ThreadOpenConnections, 0, NULL) == -1)
    {
        strError = "Error: _beginthread(ThreadOpenConnections) failed";
        printf("%s\n", strError.c_str());
        return false;
    }

    if (_beginthread(ThreadMessageHandler, 0, NULL) == -1)
    {
        strError = "Error: _beginthread(ThreadMessageHandler) failed";
        printf("%s\n", strError.c_str());
        return false;
    }

    return true;
}

bool StopNode()
{
    printf("StopNode()\n");
    fShutdown = true;
    nTransactionsUpdated++;
    int64 nStart = GetTime();
    while (vnThreadsRunning[0] > 0 || vnThreadsRunning[2] > 0 || vnThreadsRunning[3] > 0)
    {
        if (GetTime() - nStart > 15)
            break;
        Sleep(20);
    }
    if (vnThreadsRunning[0] > 0) printf("ThreadSocketHandler still running\n");
    if (vnThreadsRunning[1] > 0) printf("ThreadOpenConnections still running\n");
    if (vnThreadsRunning[2] > 0) printf("ThreadMessageHandler still running\n");
    if (vnThreadsRunning[3] > 0) printf("ThreadBitcoinMiner still running\n");
    while (vnThreadsRunning[2] > 0)
        Sleep(20);
    Sleep(50);

    // Sockets shutdown
#ifdef __WXMSW__
    WSACleanup();
#endif
    return true;
}
